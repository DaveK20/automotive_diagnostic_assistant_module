"""
main.py — Console de Comando
FastAPI + WebSocket + Simulador OBD + persistência JSON + Multimídia BT

Estrutura esperada:
  projeto/
    main.py
    static/index.html
    data/             ← criada automaticamente

Rodar:
  pip install fastapi uvicorn[standard]
  python main.py

Variável de ambiente:
  DATA_DIR  — pasta dos JSONs (default: ./data)
"""

import asyncio
import json
import math
import os
import random
import subprocess
import logging
from pathlib import Path
from typing import Set

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger("main")

# ── Caminhos ─────────────────────────────────────────────────────
BASE_DIR   = Path(__file__).parent
STATIC_DIR = BASE_DIR / "static"
DATA_DIR   = Path(os.environ.get("DATA_DIR", BASE_DIR / "data"))

STATIC_DIR.mkdir(parents=True, exist_ok=True)
DATA_DIR.mkdir(parents=True, exist_ok=True)

SETTINGS_FILE = DATA_DIR / "settings.json"
VEHICLE_FILE  = DATA_DIR / "vehicle.json"
HISTORY_FILE  = DATA_DIR / "history.json"

# ── Constantes ───────────────────────────────────────────────────
WARN_THRESHOLD = 0.10

MAINT_NAMES = [
    "Troca de Oleo", "Correia Dentada", "Pneus",
    "Fluido de Freio", "Filtro de Ar", "Velas de Ignicao", "Arrefecimento",
]
MAINT_DEFAULT_INTERVALS = [5000, 60000, 20000, 30000, 15000, 30000, 40000]

# Penalidades no readiness por status
READINESS_PENALTY = {"DUE": 20, "WARN": 10, "NO_RECORD": 5, "OK": 0}


# ── Defaults ─────────────────────────────────────────────────────
def default_settings() -> dict:
    return {
        "units": 0, "brightness": 80, "screen_off_s": 0,
        "alert_sound": False, "alert_type": 0,
        "led_enabled": True, "led_brightness": 70,
        "led_r": 0, "led_g": 200, "led_b": 50,
        "led_mode": 0, "led_speed": 50,
        # Pinout
        "pin_buzzer": 17,
        "pin_led_red": 27,
        "pin_led_yellow": 22,
        "pin_led_orange": 23,
        "pin_led_debug": 24,
    }

def default_vehicle() -> dict:
    return {
        "total_km": 45230,
        "items": [
            {
                "name": name, "interval_km": MAINT_DEFAULT_INTERVALS[i],
                "last_km": 0, "last_day": 1, "last_month": 1, "last_year": 2024,
                "alert_active": False, "valid": False,
            }
            for i, name in enumerate(MAINT_NAMES)
        ],
    }

def default_history() -> list:
    return []


# ── Persistência JSON (escrita atômica) ──────────────────────────
def load_json(path: Path, default_fn):
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except (FileNotFoundError, json.JSONDecodeError):
        data = default_fn()
        save_json(path, data)
        return data

def save_json(path: Path, data):
    tmp = path.with_suffix(".tmp")
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    tmp.replace(path)


# ── Estado global ────────────────────────────────────────────────
settings: dict = load_json(SETTINGS_FILE, default_settings)
vehicle:  dict = load_json(VEHICLE_FILE,  default_vehicle)
history:  list = load_json(HISTORY_FILE,  default_history)

obd_data: dict = {
    "rpm": 820, "speed_kmh": 0, "temp_c": 55, "fuel_pct": 62,
    "total_km": vehicle["total_km"], "engine_hours": 0,
    "readiness_pct": 100, "source": "simulator",
}

# Estado multimídia (local — não persiste)
media_state: dict = {
    "playing": False,
    "muted":   False,
    "volume":  65,       # 0-100
    "track":   "---",
    "artist":  "---",
    "shuffle": False,
    "repeat":  False,
}

_sim_t    = 0
_km_tick  = 0


# ── Simulador OBD ────────────────────────────────────────────────
def simulate_obd() -> None:
    global _sim_t, _km_tick
    _sim_t += 1
    t = _sim_t

    cycle = (t % 120) / 120.0
    if cycle < 0.25:
        tspd, trpm = 0, 820
    elif cycle < 0.55:
        tspd = int(70 + 40 * math.sin(cycle * math.pi * 4))
        trpm = 1400 + tspd * 22
    else:
        tspd = int(35 + 20 * math.sin(cycle * math.pi * 3))
        trpm = 1200 + tspd * 18

    obd_data["speed_kmh"] = max(0, min(180, int(
        obd_data["speed_kmh"] * 0.7 + tspd * 0.3 + random.randint(-2, 2)
    )))
    obd_data["rpm"] = max(700, min(6000, int(
        obd_data["rpm"] * 0.8 + trpm * 0.2 + random.randint(-50, 50)
    )))

    if obd_data["temp_c"] < 88:
        obd_data["temp_c"] = min(88, obd_data["temp_c"] + 1)
    else:
        obd_data["temp_c"] = 88 + random.randint(-2, 2)

    if t % 60 == 0 and obd_data["fuel_pct"] > 0:
        obd_data["fuel_pct"] -= 1

    if obd_data["speed_kmh"] > 5 and t % 4 == 0:
        obd_data["total_km"] += 1

    _km_tick += 1
    if _km_tick >= 60:
        _km_tick = 0
        vehicle["total_km"] = obd_data["total_km"]
        save_json(VEHICLE_FILE, vehicle)


# ── Lógica de manutenção ─────────────────────────────────────────
def calc_maint(item: dict, current_km: int) -> dict:
    next_km      = item["last_km"] + item["interval_km"]
    km_remaining = next_km - current_km
    warn_thresh  = item["interval_km"] * WARN_THRESHOLD

    if not item["valid"]:
        status = "NO_RECORD"
    elif km_remaining <= 0:
        status = "DUE"
    elif km_remaining <= warn_thresh:
        status = "WARN"
    else:
        status = "OK"

    progress = 0
    if item["valid"] and item["interval_km"] > 0:
        progress = min(100, max(0, int(
            (current_km - item["last_km"]) * 100 / item["interval_km"]
        )))

    last_date = "---"
    if item["valid"]:
        last_date = f"{item['last_day']:02d}/{item['last_month']:02d}/{item['last_year']:04d}"

    return {
        "name": item["name"], "status": status,
        "km_remaining": km_remaining, "last_km": item["last_km"],
        "next_km": next_km, "interval_km": item["interval_km"],
        "progress_pct": progress, "last_date": last_date, "valid": item["valid"],
    }


def build_alerts(maint_rows: list) -> list:
    """
    Inclui DUE, WARN e NO_RECORD como alertas.
    DUE → CRITICAL | WARN → WARNING | NO_RECORD → INFO
    """
    alerts = []
    for m in maint_rows:
        if m["status"] == "DUE":
            alerts.append({
                "level": "CRITICAL",
                "text":  f"{m['name']}: VENCIDA por {abs(m['km_remaining'])} km",
            })
        elif m["status"] == "WARN":
            alerts.append({
                "level": "WARNING",
                "text":  f"{m['name']}: faltam {m['km_remaining']} km",
            })
        elif m["status"] == "NO_RECORD":
            alerts.append({
                "level": "INFO",
                "text":  f"{m['name']}: nenhum registro encontrado",
            })
    # Ordenação: CRITICAL → WARNING → INFO
    order = {"CRITICAL": 0, "WARNING": 1, "INFO": 2}
    alerts.sort(key=lambda a: order.get(a["level"], 9))
    return alerts


def calc_readiness(maint_rows: list) -> int:
    """
    Começa em 100%.
    Cada item penaliza conforme seu status:
      DUE       → -20%
      WARN      → -10%
      NO_RECORD → -5%
      OK        → 0%
    """
    penalty = sum(READINESS_PENALTY.get(m["status"], 0) for m in maint_rows)
    return max(0, 100 - penalty)


def build_full_state() -> dict:
    current_km  = obd_data["total_km"]
    maint_rows  = [calc_maint(item, current_km) for item in vehicle["items"]]
    alerts      = build_alerts(maint_rows)
    readiness   = calc_readiness(maint_rows)
    obd_data["readiness_pct"] = readiness

    return {
        "type":     "state",
        "obd":      obd_data,
        "maint":    maint_rows,
        "history":  list(reversed(history[-50:])),
        "alerts":   alerts,
        "settings": settings,
        "media":    media_state,
    }


# ── Multimídia — playerctl / pactl (Linux / Pi) ──────────────────
def _run(cmd: list) -> str:
    """Executa comando shell; retorna stdout ou '' em caso de erro."""
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=2)
        return r.stdout.strip()
    except Exception as e:
        log.debug("media cmd %s: %s", cmd, e)
        return ""

def media_action(action: str) -> None:
    match action:
        case "play_pause": _run(["playerctl", "play-pause"])
        case "next":       _run(["playerctl", "next"])
        case "prev":       _run(["playerctl", "previous"])
        case "shuffle":
            media_state["shuffle"] = not media_state["shuffle"]
            _run(["playerctl", "shuffle",
                  "On" if media_state["shuffle"] else "Off"])
        case "repeat":
            media_state["repeat"] = not media_state["repeat"]
            _run(["playerctl", "loop",
                  "Track" if media_state["repeat"] else "None"])
        case "vol_up":
            media_state["volume"] = min(100, media_state["volume"] + 5)
            _run(["pactl", "set-sink-volume", "@DEFAULT_SINK@",
                  f"{media_state['volume']}%"])
        case "vol_down":
            media_state["volume"] = max(0, media_state["volume"] - 5)
            _run(["pactl", "set-sink-volume", "@DEFAULT_SINK@",
                  f"{media_state['volume']}%"])
        case "mute":
            media_state["muted"] = not media_state["muted"]
            _run(["pactl", "set-sink-mute", "@DEFAULT_SINK@",
                  "toggle"])

def media_poll() -> None:
    """Lê estado real do playerctl (se disponível)."""
    status = _run(["playerctl", "status"])
    if status:
        media_state["playing"] = status.lower() == "playing"
        media_state["track"]   = _run(["playerctl", "metadata", "title"])  or "---"
        media_state["artist"]  = _run(["playerctl", "metadata", "artist"]) or "---"


# ── WebSocket ────────────────────────────────────────────────────
clients: Set[WebSocket] = set()

async def broadcast(data: dict) -> None:
    msg  = json.dumps(data, ensure_ascii=False)
    dead: Set[WebSocket] = set()
    for ws in list(clients):
        try:
            await ws.send_text(msg)
        except Exception:
            dead.add(ws)
    clients.difference_update(dead)

async def handle_message(msg: dict) -> None:
    action = msg.get("action")

    if action == "register":
        idx, km = int(msg["idx"]), int(msg["km"])
        day, month, year = int(msg["day"]), int(msg["month"]), int(msg["year"])
        item = vehicle["items"][idx]
        item.update(last_km=km, last_day=day, last_month=month,
                    last_year=year, alert_active=False, valid=True)
        history.append({
            "date": f"{day:02d}/{month:02d}/{year:04d}",
            "item": item["name"],
            "km":   str(km),
        })
        save_json(VEHICLE_FILE, vehicle)
        save_json(HISTORY_FILE, history)
        await broadcast(build_full_state())

    elif action == "add_maint":
        name = str(msg.get("name", "Novo Item")).strip()[:40]
        interval_km = max(1000, min(200_000, int(msg.get("interval_km", 10000))))
        if name and len(vehicle["items"]) < 20:
            vehicle["items"].append({
                "name": name, "interval_km": interval_km,
                "last_km": 0, "last_day": 1, "last_month": 1, "last_year": 2024,
                "alert_active": False, "valid": False,
            })
            save_json(VEHICLE_FILE, vehicle)
        await broadcast(build_full_state())

    elif action == "remove_maint":
        idx = int(msg["idx"])
        if 0 <= idx < len(vehicle["items"]) and len(vehicle["items"]) > 1:
            vehicle["items"].pop(idx)
            save_json(VEHICLE_FILE, vehicle)
        await broadcast(build_full_state())

    elif action == "rename_maint":
        idx = int(msg["idx"])
        name = str(msg.get("name", "")).strip()[:40]
        if 0 <= idx < len(vehicle["items"]) and name:
            vehicle["items"][idx]["name"] = name
            save_json(VEHICLE_FILE, vehicle)
        await broadcast(build_full_state())

    elif action == "save_settings":
        for key, val in msg.get("settings", {}).items():
            if key in settings:
                settings[key] = val
        save_json(SETTINGS_FILE, settings)
        await broadcast(build_full_state())

    elif action == "set_interval":
        idx = int(msg["idx"])
        km  = max(1000, min(200_000, int(msg["km"])))
        vehicle["items"][idx]["interval_km"] = km
        save_json(VEHICLE_FILE, vehicle)
        await broadcast(build_full_state())

    elif action == "media":
        loop = asyncio.get_event_loop()
        await loop.run_in_executor(None, media_action, msg.get("cmd", ""))
        # Atualiza playing state após comandos de faixa
        if msg.get("cmd") in ("play_pause", "next", "prev"):
            await asyncio.sleep(0.3)
            await loop.run_in_executor(None, media_poll)
        await broadcast(build_full_state())


# ── FastAPI ──────────────────────────────────────────────────────
app = FastAPI(title="Console de Comando")
app.mount("/static", StaticFiles(directory=str(STATIC_DIR)), name="static")

@app.get("/")
async def root() -> HTMLResponse:
    index = STATIC_DIR / "index.html"
    if not index.exists():
        return HTMLResponse(
            "<h2>Coloque <b>index.html</b> dentro de <b>static/</b></h2>",
            status_code=404,
        )
    return HTMLResponse(index.read_text(encoding="utf-8"))

@app.websocket("/ws")
async def ws_endpoint(ws: WebSocket) -> None:
    await ws.accept()
    clients.add(ws)
    await ws.send_text(json.dumps(build_full_state(), ensure_ascii=False))
    try:
        while True:
            raw = await ws.receive_text()
            await handle_message(json.loads(raw))
    except WebSocketDisconnect:
        clients.discard(ws)

@app.on_event("startup")
async def startup() -> None:
    asyncio.create_task(obd_loop())

@app.on_event("shutdown")
async def shutdown() -> None:
    vehicle["total_km"] = obd_data["total_km"]
    save_json(VEHICLE_FILE, vehicle)

async def obd_loop() -> None:
    tick = 0
    while True:
        simulate_obd()
        # Atualiza estado do playerctl a cada 5 s
        if tick % 5 == 0:
            loop = asyncio.get_event_loop()
            await loop.run_in_executor(None, media_poll)
        tick += 1
        await broadcast(build_full_state())
        await asyncio.sleep(1.0)


if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=False, log_level="info")