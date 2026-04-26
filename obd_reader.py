"""
obd_reader.py — Leitura OBD-II real (ELM327) com fallback para simulador.

Dependências (instalar no Pi):
  pip install obd

O adaptador ELM327 USB normalmente aparece em /dev/ttyUSB0.
Bluetooth: parear primeiro, depois usar /dev/rfcomm0.

Uso:
  reader = OBDReader(port="/dev/ttyUSB0")  # None = auto-detect
  reader.start()
  data = reader.get_data()   # dict com os valores OBD
  reader.stop()
"""

import logging
import math
import random
import threading
import time
from typing import Optional

logger = logging.getLogger("obd_reader")

# ── Tenta importar python-obd ────────────────────────────────────
try:
    import obd
    OBD_AVAILABLE = True
except ImportError:
    OBD_AVAILABLE = False
    logger.warning("python-obd não instalado — usando simulador OBD.")


# ================================================================
# Simulador (usado quando não há adaptador)
# ================================================================
class _Simulator:
    def __init__(self):
        self._t       = 0
        self._rpm     = 820
        self._spd     = 0
        self._temp    = 55
        self._fuel    = 62
        self._km      = 0           # incrementado externamente via set_km()
        self._hours   = 1243
        self._ready   = 100
        self._lock    = threading.Lock()

    def tick(self, current_km: int) -> dict:
        with self._lock:
            self._t += 1
            t = self._t

            # Ciclo de condução 120 s
            cycle = (t % 120) / 120.0
            if cycle < 0.25:
                tspd, trpm = 0, 820
            elif cycle < 0.55:
                tspd = int(70 + 40 * math.sin(cycle * math.pi * 4))
                trpm = 1400 + tspd * 22
            else:
                tspd = int(35 + 20 * math.sin(cycle * math.pi * 3))
                trpm = 1200 + tspd * 18

            self._spd = max(0, min(180, int(self._spd * 0.7 + tspd * 0.3
                                            + random.randint(-2, 2))))
            self._rpm = max(700, min(6000, int(self._rpm * 0.8 + trpm * 0.2
                                               + random.randint(-50, 50))))

            # Temperatura aquece até ~88 °C
            if self._temp < 88:
                self._temp = min(88, self._temp + 1)
            else:
                self._temp = 88 + random.randint(-2, 2)

            # Combustível cai lentamente
            if t % 60 == 0 and self._fuel > 0:
                self._fuel -= 1

            return {
                "rpm":          self._rpm,
                "speed_kmh":    self._spd,
                "temp_c":       self._temp,
                "fuel_pct":     self._fuel,
                "total_km":     current_km,
                "engine_hours": self._hours,
                "readiness_pct": self._ready,
                "source":       "simulator",
            }

    def set_readiness(self, pct: int):
        with self._lock:
            self._ready = pct


# ================================================================
# Leitor OBD real
# ================================================================
class _RealOBD:
    """
    Lê dados do ELM327 em background.
    Os comandos OBD são padronizados — funcionam com qualquer carro
    com porta OBD-II (pós 1996).
    """

    CMDS = None  # preenchido após importar obd

    def __init__(self, port: Optional[str], baudrate: int):
        self._port     = port
        self._baudrate = baudrate
        self._conn     = None
        self._data     = {}
        self._lock     = threading.Lock()
        self._stop_evt = threading.Event()
        self._thread   = None

    def _connect(self) -> bool:
        try:
            kwargs = dict(fast=False, timeout=30)
            if self._port:
                kwargs["portstr"]  = self._port
                kwargs["baudrate"] = self._baudrate
            self._conn = obd.OBD(**kwargs)
            if self._conn.status() == obd.OBDStatus.NOT_CONNECTED:
                logger.error("OBD: falha na conexão com o adaptador.")
                return False
            logger.info("OBD: conectado em %s", self._conn.port_name())
            return True
        except Exception as exc:
            logger.error("OBD connect error: %s", exc)
            return False

    def _read_loop(self):
        while not self._stop_evt.is_set():
            if self._conn is None or not self._conn.is_connected():
                if not self._connect():
                    time.sleep(5)
                    continue

            snap = {}
            try:
                def q(cmd):
                    r = self._conn.query(cmd)
                    return r.value if r and not r.is_null() else None

                rpm  = q(obd.commands.RPM)
                spd  = q(obd.commands.SPEED)
                temp = q(obd.commands.COOLANT_TEMP)
                fuel = q(obd.commands.FUEL_LEVEL)

                snap["rpm"]       = int(rpm.magnitude)       if rpm  else 0
                snap["speed_kmh"] = int(spd.magnitude)       if spd  else 0
                snap["temp_c"]    = int(temp.magnitude)      if temp else 0
                snap["fuel_pct"]  = int(fuel.magnitude)      if fuel else 0

                # Quilometragem via OBD não é padrão em todos os carros;
                # usa valor persistido (gerenciado pelo main.py).
                snap["source"] = "obd"

            except Exception as exc:
                logger.warning("OBD read error: %s", exc)
                snap["source"] = "obd_error"
                self._conn = None

            with self._lock:
                self._data = snap

            time.sleep(1)

    def start(self):
        self._thread = threading.Thread(target=self._read_loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._stop_evt.set()

    def get_snapshot(self) -> dict:
        with self._lock:
            return dict(self._data)


# ================================================================
# Interface pública
# ================================================================
class OBDReader:
    """
    Abstração única: usa ELM327 real se disponível, senão simula.

    Parâmetros
    ----------
    port       : porta serial (ex: "/dev/ttyUSB0") ou None para auto-detect
    baudrate   : velocidade serial (padrão 38400)
    force_sim  : força simulador mesmo se o adaptador estiver disponível
    """

    def __init__(
        self,
        port: Optional[str] = None,
        baudrate: int = 38400,
        force_sim: bool = False,
    ):
        self._sim      = _Simulator()
        self._real: Optional[_RealOBD] = None
        self._use_real = False
        self._lock     = threading.Lock()

        if OBD_AVAILABLE and not force_sim:
            self._real     = _RealOBD(port, baudrate)
            self._use_real = True
            logger.info("OBD: modo REAL (ELM327) ativado.")
        else:
            logger.info("OBD: modo SIMULADOR ativado.")

    def start(self):
        if self._real:
            self._real.start()

    def stop(self):
        if self._real:
            self._real.stop()

    def tick(self, current_km: int) -> dict:
        """
        Retorna snapshot dos dados OBD.
        Deve ser chamado a cada 1 s pelo main.py.
        """
        if self._use_real and self._real:
            snap = self._real.get_snapshot()
            if snap.get("source") == "obd":
                # km vem do banco de dados (não do OBD)
                snap["total_km"]     = current_km
                snap["engine_hours"] = 0   # calcule externamente se desejar
                snap["readiness_pct"] = 100
                return snap
            # fallback para simulador se OBD falhar
        return self._sim.tick(current_km)

    def set_readiness(self, pct: int):
        self._sim.set_readiness(pct)