"""
main.py — Console de Comando
FastAPI + WebSocket + Simulador OBD + persistência JSON + Multimídia BT + Assistente de Voz

Estrutura esperada:
  projeto/
    main.py
    static/index.html
    static/skull.png        ← imagem do assistente
    data/                   ← criada automaticamente
    model/                  ← modelo Vosk em português (vosk-model-small-pt-0.3)

Dependências:
  pip install fastapi uvicorn[standard] vosk sounddevice pyttsx3
  python main.py
"""

import asyncio
import json
import math
import os
import random
import subprocess
import logging
import queue
import threading
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
MODEL_PATH = str(BASE_DIR / "model")

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

READINESS_PENALTY = {"DUE": 20, "WARN": 10, "NO_RECORD": 5, "OK": 0}


# ── Defaults ─────────────────────────────────────────────────────
def default_settings() -> dict:
    return {
        "units": 0, "brightness": 80, "screen_off_s": 0,
        "alert_sound": False, "alert_type": 0,
        "led_enabled": True, "led_brightness": 70,
        "led_r": 0, "led_g": 200, "led_b": 50,
        "led_mode": 0, "led_speed": 50,
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


# ── Persistência JSON ─────────────────────────────────────────────
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
# Garante que chaves novas dos defaults existam em arquivos antigos
for _k, _v in default_settings().items():
    settings.setdefault(_k, _v)

vehicle:  dict = load_json(VEHICLE_FILE,  default_vehicle)
history:  list = load_json(HISTORY_FILE,  default_history)

obd_data: dict = {
    "rpm": 820, "speed_kmh": 0, "temp_c": 55, "fuel_pct": 62,
    "total_km": vehicle["total_km"], "engine_hours": 0,
    "readiness_pct": 100, "source": "simulator",
}

media_state: dict = {
    "playing": False, "muted": False, "volume": 65,
    "track": "---", "artist": "---", "shuffle": False, "repeat": False,
}

_sim_t   = 0
_km_tick = 0


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

    # Incrementa horas de motor a cada 3600 ticks (1 hora real = 1 hora simulada)
    if t % 3600 == 0:
        obd_data["engine_hours"] += 1

    _km_tick += 1
    if _km_tick >= 60:
        _km_tick = 0
        vehicle["total_km"] = obd_data["total_km"]
        save_json(VEHICLE_FILE, vehicle)


# ── Manutenção ───────────────────────────────────────────────────
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
    alerts = []
    for m in maint_rows:
        if m["status"] == "DUE":
            alerts.append({"level": "CRITICAL", "text": f"{m['name']}: VENCIDA por {abs(m['km_remaining'])} km"})
        elif m["status"] == "WARN":
            alerts.append({"level": "WARNING",  "text": f"{m['name']}: faltam {m['km_remaining']} km"})
        elif m["status"] == "NO_RECORD":
            alerts.append({"level": "INFO",     "text": f"{m['name']}: nenhum registro encontrado"})
    order = {"CRITICAL": 0, "WARNING": 1, "INFO": 2}
    alerts.sort(key=lambda a: order.get(a["level"], 9))
    return alerts


def calc_readiness(maint_rows: list) -> int:
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


# ── Multimídia ───────────────────────────────────────────────────
def _run(cmd: list) -> str:
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
            _run(["playerctl", "shuffle", "On" if media_state["shuffle"] else "Off"])
        case "repeat":
            media_state["repeat"] = not media_state["repeat"]
            _run(["playerctl", "loop", "Track" if media_state["repeat"] else "None"])
        case "vol_up":
            media_state["volume"] = min(100, media_state["volume"] + 5)
            _run(["pactl", "set-sink-volume", "@DEFAULT_SINK@", f"{media_state['volume']}%"])
        case "vol_down":
            media_state["volume"] = max(0, media_state["volume"] - 5)
            _run(["pactl", "set-sink-volume", "@DEFAULT_SINK@", f"{media_state['volume']}%"])
        case "mute":
            media_state["muted"] = not media_state["muted"]
            _run(["pactl", "set-sink-mute", "@DEFAULT_SINK@", "toggle"])

def media_poll() -> None:
    status = _run(["playerctl", "status"])
    if status:
        media_state["playing"] = status.lower() == "playing"
        media_state["track"]   = _run(["playerctl", "metadata", "title"])  or "---"
        media_state["artist"]  = _run(["playerctl", "metadata", "artist"]) or "---"


# ═══════════════════════════════════════════════════════════════════
# ── ASSISTENTE DE VOZ ────────────────────────────────────────────
# ═══════════════════════════════════════════════════════════════════

_voice_listening = False
_voice_thread: threading.Thread | None = None
_audio_queue: queue.Queue = queue.Queue()

# Comandos customizados (sincronizados pelo frontend via voice_sync_commands)
_custom_commands: list = []


def _tts_speak(text: str) -> None:
    try:
        import pyttsx3
        engine = pyttsx3.init()
        engine.setProperty("rate", 180)
        engine.say(text)
        engine.runAndWait()
    except Exception as e:
        log.warning("TTS erro: %s", e)


def _execute_media_action(action: str) -> None:
    match action:
        case "play_pause": _run(["playerctl", "play-pause"])
        case "next":       _run(["playerctl", "next"])
        case "prev":       _run(["playerctl", "previous"])
        case "vol_up":
            media_state["volume"] = min(100, media_state["volume"] + 5)
            _run(["pactl", "set-sink-volume", "@DEFAULT_SINK@", f"{media_state['volume']}%"])
        case "vol_down":
            media_state["volume"] = max(0, media_state["volume"] - 5)
            _run(["pactl", "set-sink-volume", "@DEFAULT_SINK@", f"{media_state['volume']}%"])
        case "mute":
            media_state["muted"] = not media_state["muted"]
            _run(["pactl", "set-sink-mute", "@DEFAULT_SINK@", "toggle"])


def _resolve_action_response(action: str) -> str:
    if action == "status_report":
        fuel = obd_data.get("fuel_pct", 0)
        temp = obd_data.get("temp_c", 0)
        rpm  = obd_data.get("rpm", 0)
        spd  = obd_data.get("speed_kmh", 0)
        rdy  = obd_data.get("readiness_pct", 100)
        return (f"Sistema pronto. Combustível {fuel}%, temperatura {temp} graus, "
                f"rotação {rpm} RPM, velocidade {spd} quilômetros por hora, "
                f"prontidão do veículo {rdy}%")
    if action == "fuel_report":
        return f"Combustível em {obd_data.get('fuel_pct', 0)} por cento"
    if action == "temp_report":
        return f"Temperatura do motor: {obd_data.get('temp_c', 0)} graus Celsius"
    if action == "km_report":
        return f"Quilometragem total: {obd_data.get('total_km', 0):,} quilômetros".replace(",", ".")
    if action == "maint_report":
        due = [m["name"] for m in
               [calc_maint(i, obd_data["total_km"]) for i in vehicle["items"]]
               if m["status"] in ("DUE", "WARN")]
        if due:
            return f"Atenção: {', '.join(due[:3])} precisam de revisão"
        return "Todas as manutenções estão em dia"
    return ""


def _handle_voice_command(text: str) -> str:
    """
    1. Tenta casar com comandos customizados (do frontend)
    2. Cai nos comandos built-in
    Retorna texto da resposta.
    """
    text_lower = text.lower().strip()

    # 1. Comandos customizados (enviados pelo frontend)
    for cmd in _custom_commands:
        keywords = cmd.get("keywords", [])
        if any(kw in text_lower for kw in keywords):
            action   = cmd.get("action", "none")
            response = cmd.get("response", "")
            if action and action != "none":
                if action in ("play_pause", "next", "prev", "vol_up", "vol_down", "mute"):
                    _execute_media_action(action)
                else:
                    response = _resolve_action_response(action) or response
            return response or "Comando executado"

    # 2. Comandos built-in
    if any(w in text_lower for w in ("tocar", "play", "música", "musica")):
        _run(["playerctl", "play"])
        media_state["playing"] = True
        return "Tocando música"
    if any(w in text_lower for w in ("pausar", "pausa")):
        _run(["playerctl", "pause"])
        media_state["playing"] = False
        return "Música pausada"
    if any(w in text_lower for w in ("próxima", "proxima", "avançar")):
        _run(["playerctl", "next"])
        return "Próxima música"
    if any(w in text_lower for w in ("anterior", "voltar")):
        _run(["playerctl", "previous"])
        return "Música anterior"
    if any(w in text_lower for w in ("volume mais", "aumentar volume")):
        _execute_media_action("vol_up")
        return f"Volume aumentado para {media_state['volume']}%"
    if any(w in text_lower for w in ("volume menos", "diminuir volume")):
        _execute_media_action("vol_down")
        return f"Volume reduzido para {media_state['volume']}%"
    if any(w in text_lower for w in ("mudo", "silêncio", "silencio")):
        _execute_media_action("mute")
        return "Mudo ativado" if media_state["muted"] else "Mudo desativado"
    if any(w in text_lower for w in ("status", "situação", "situacao")):
        return _resolve_action_response("status_report")
    if any(w in text_lower for w in ("combustível", "combustivel", "gasolina")):
        return _resolve_action_response("fuel_report")
    if any(w in text_lower for w in ("temperatura", "motor")):
        return _resolve_action_response("temp_report")
    if any(w in text_lower for w in ("quilometragem", "quilômetros", "km")):
        return _resolve_action_response("km_report")
    if any(w in text_lower for w in ("manutenção", "manutencao", "revisão")):
        return _resolve_action_response("maint_report")
    if any(w in text_lower for w in ("sair", "desligar", "encerrar")):
        return "Encerrando assistente de voz"

    return "Comando não reconhecido. Tente: status, música, volume, manutenção"


def _build_startup_greeting() -> str:
    import datetime
    hora = datetime.datetime.now().hour
    if hora < 12:   saud = "Bom dia"
    elif hora < 18: saud = "Boa tarde"
    else:           saud = "Boa noite"

    fuel = obd_data.get("fuel_pct", 0)
    temp = obd_data.get("temp_c", 0)
    rdy  = obd_data.get("readiness_pct", 100)

    maint_rows = [calc_maint(item, obd_data["total_km"]) for item in vehicle["items"]]
    due_items  = [m["name"] for m in maint_rows if m["status"] == "DUE"]
    warn_items = [m["name"] for m in maint_rows if m["status"] == "WARN"]

    parts = [f"{saud}. Sistema de bordo iniciado."]
    parts.append(f"Combustível: {fuel}%. Temperatura do motor: {temp} graus.")

    if rdy < 60:
        parts.append(f"Atenção: prontidão do veículo em apenas {rdy}%.")
    else:
        parts.append(f"Prontidão do veículo: {rdy}%.")

    if due_items:
        parts.append(f"ALERTA CRÍTICO: {', '.join(due_items[:2])} vencida{'s' if len(due_items) > 1 else ''}.")
    if warn_items:
        parts.append(f"Atenção: {', '.join(warn_items[:2])} próxima{'s' if len(warn_items) > 1 else ''} do vencimento.")
    if not due_items and not warn_items:
        parts.append("Todas as manutenções estão em dia.")

    parts.append("Aguardando seus comandos.")
    return " ".join(parts)


def _audio_callback(indata, frames, time, status):
    if status:
        log.debug("audio status: %s", status)
    _audio_queue.put(bytes(indata))


def _voice_listen_loop(loop: asyncio.AbstractEventLoop) -> None:
    global _voice_listening

    try:
        import sounddevice as sd
        from vosk import Model, KaldiRecognizer
    except ImportError:
        log.error("Vosk ou sounddevice não instalados. Execute: pip install vosk sounddevice")
        asyncio.run_coroutine_threadsafe(
            broadcast({"type": "voice_error", "msg": "Vosk não instalado", "fatal": True}), loop
        )
        return

    try:
        model = Model(MODEL_PATH)
    except Exception as e:
        log.error("Modelo Vosk não encontrado em '%s': %s", MODEL_PATH, e)
        asyncio.run_coroutine_threadsafe(
            broadcast({"type": "voice_error", "msg": f"Modelo não encontrado: {MODEL_PATH}", "fatal": True}), loop
        )
        return

    rec = KaldiRecognizer(model, 16000)
    log.info("🎤 Assistente de voz iniciado")

    # Notifica o frontend que o modelo carregou e está pronto para escutar
    asyncio.run_coroutine_threadsafe(
        broadcast({"type": "voice_ready"}), loop
    )

    try:
        with sd.RawInputStream(samplerate=16000, blocksize=8000,
                               dtype="int16", channels=1,
                               callback=_audio_callback):
            while _voice_listening:
                try:
                    data = _audio_queue.get(timeout=0.5)
                except queue.Empty:
                    continue

                if rec.AcceptWaveform(data):
                    result = json.loads(rec.Result())
                    text   = result.get("text", "").strip()
                    if not text:
                        continue

                    log.info("🗣  Reconhecido: %s", text)

                    # 1. Notifica UI: processando
                    asyncio.run_coroutine_threadsafe(
                        broadcast({"type": "voice_processing", "heard": text}), loop
                    )

                    # 2. Interpreta e fala
                    response = _handle_voice_command(text)
                    threading.Thread(target=_tts_speak, args=(response,), daemon=True).start()

                    # 3. Notifica UI: resposta pronta
                    asyncio.run_coroutine_threadsafe(
                        broadcast({"type": "voice_response", "heard": text, "response": response}), loop
                    )

    except Exception as e:
        log.error("Erro no loop de voz: %s", e)
        asyncio.run_coroutine_threadsafe(
            broadcast({"type": "voice_error", "msg": str(e)}), loop
        )
    finally:
        _voice_listening = False
        log.info("🛑 Assistente de voz encerrado")


def start_voice(loop: asyncio.AbstractEventLoop) -> dict:
    global _voice_listening, _voice_thread
    if _voice_listening:
        return {"ok": False, "msg": "Já escutando"}
    # Descarta áudio residual de sessões anteriores
    while not _audio_queue.empty():
        try:
            _audio_queue.get_nowait()
        except queue.Empty:
            break
    _voice_listening = True
    _voice_thread = threading.Thread(
        target=_voice_listen_loop, args=(loop,), daemon=True
    )
    _voice_thread.start()
    return {"ok": True, "msg": "Assistente iniciado"}


def stop_voice() -> dict:
    global _voice_listening
    _voice_listening = False
    return {"ok": True, "msg": "Assistente parado"}


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

async def handle_message(msg: dict, ws: WebSocket | None = None) -> None:
    action = msg.get("action")

    # ── Voz ──────────────────────────────────────────────────────
    if action == "voice_start":
        loop = asyncio.get_running_loop()
        result = start_voice(loop)
        if ws:
            await ws.send_text(json.dumps({"type": "voice_status", **result}))
        return

    if action == "voice_stop":
        result = stop_voice()
        if ws:
            await ws.send_text(json.dumps({"type": "voice_status", **result}))
        return

    if action == "voice_startup":
        await asyncio.sleep(1.2)
        greeting = _build_startup_greeting()
        threading.Thread(target=_tts_speak, args=(greeting,), daemon=True).start()
        await broadcast({"type": "voice_response", "heard": "", "response": greeting})
        return

    if action == "voice_sync_commands":
        global _custom_commands
        _custom_commands = msg.get("commands", [])
        log.info("Comandos customizados sincronizados: %d entradas", len(_custom_commands))
        return

    if action == "voice_command":
        text = msg.get("text", "").strip()
        if not text:
            return
        await broadcast({"type": "voice_processing", "heard": text})
        response = _handle_voice_command(text)
        threading.Thread(target=_tts_speak, args=(response,), daemon=True).start()
        await broadcast({"type": "voice_response", "heard": text, "response": response})
        return

    # ── Manutenção / configurações ────────────────────────────────
    if action == "register":
        idx, km = int(msg["idx"]), int(msg["km"])
        day, month, year = int(msg["day"]), int(msg["month"]), int(msg["year"])
        item = vehicle["items"][idx]
        item.update(last_km=km, last_day=day, last_month=month,
                    last_year=year, alert_active=False, valid=True)
        history.append({"date": f"{day:02d}/{month:02d}/{year:04d}", "item": item["name"], "km": str(km)})
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
        loop = asyncio.get_running_loop()
        cmd  = msg.get("cmd", "")
        if cmd == "vol_set":
            vol = max(0, min(100, int(msg.get("value", media_state["volume"]))))
            media_state["volume"] = vol
            media_state["muted"]  = False
            await loop.run_in_executor(
                None, _run, ["pactl", "set-sink-volume", "@DEFAULT_SINK@", f"{vol}%"]
            )
        else:
            await loop.run_in_executor(None, media_action, cmd)
            if cmd in ("play_pause", "next", "prev"):
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
            await handle_message(json.loads(raw), ws)
    except WebSocketDisconnect:
        clients.discard(ws)

@app.on_event("startup")
async def startup() -> None:
    asyncio.create_task(obd_loop())

@app.on_event("shutdown")
async def shutdown() -> None:
    stop_voice()
    vehicle["total_km"] = obd_data["total_km"]
    save_json(VEHICLE_FILE, vehicle)

async def obd_loop() -> None:
    loop = asyncio.get_running_loop()
    tick = 0
    while True:
        simulate_obd()
        if tick % 5 == 0:
            await loop.run_in_executor(None, media_poll)
        tick += 1
        await broadcast(build_full_state())
        await asyncio.sleep(1.0)


if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=False, log_level="info")
