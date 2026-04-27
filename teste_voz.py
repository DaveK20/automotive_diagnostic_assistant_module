import tkinter as tk
import queue
import sounddevice as sd
import json
import pyttsx3
from vosk import Model, KaldiRecognizer
from threading import Thread

# =====================
# CONFIG
# =====================
MODEL_PATH = "model"

# =====================
# VOZ (fala)
# =====================
engine = pyttsx3.init()
engine.setProperty('rate', 180)

def speak(text):
    log(f"Assistente: {text}")
    engine.say(text)
    engine.runAndWait()

# =====================
# UI LOG
# =====================
def log(text):
    output.insert(tk.END, text + "\n")
    output.see(tk.END)

# =====================
# INTERPRETAÇÃO
# =====================
def handle_command(text):
    text = text.lower()
    log(f"Você: {text}")

    if "tocar musica" in text or "play" in text:
        speak("Tocando música")

    elif "pausar" in text:
        speak("Música pausada")

    elif "proxima" in text:
        speak("Próxima música")

    elif "anterior" in text:
        speak("Música anterior")

    elif "volume mais" in text:
        speak("Aumentando volume")

    elif "volume menos" in text:
        speak("Diminuindo volume")

    elif "status" in text:
        speak("Tudo funcionando corretamente")

    elif "sair" in text:
        speak("Encerrando sistema")
        root.quit()

    else:
        speak("Comando não reconhecido")

# =====================
# VOSK (escuta)
# =====================
q = queue.Queue()
listening = False

def audio_callback(indata, frames, time, status):
    if status:
        print(status)
    q.put(bytes(indata))

def listen_loop():
    global listening
    try:
        model = Model(MODEL_PATH)
    except:
        log("❌ Erro ao carregar modelo Vosk")
        return

    rec = KaldiRecognizer(model, 16000)

    log("🎤 Escutando...")

    with sd.RawInputStream(samplerate=16000, blocksize=8000,
                           dtype='int16', channels=1,
                           callback=audio_callback):

        while listening:
            data = q.get()
            if rec.AcceptWaveform(data):
                result = json.loads(rec.Result())
                text = result.get("text", "")
                if text.strip():
                    handle_command(text)

def toggle_listening():
    global listening
    listening = not listening

    if listening:
        btn_listen.config(text="🛑 Parar Escuta")
        Thread(target=listen_loop, daemon=True).start()
    else:
        btn_listen.config(text="🎤 Iniciar Escuta")
        log("🛑 Escuta parada")

# =====================
# TEXTO (entrada manual)
# =====================
def send_text():
    text = entry.get()
    entry.delete(0, tk.END)
    if text.strip():
        handle_command(text)

# =====================
# INTERFACE
# =====================
root = tk.Tk()
root.title("Assistente (Voz + Texto)")
root.geometry("450x500")

# Botão voz
btn_listen = tk.Button(root, text="🎤 Iniciar Escuta", command=toggle_listening)
btn_listen.pack(pady=10)

# Entrada texto
entry = tk.Entry(root, font=("Arial", 14))
entry.pack(padx=10, fill="x")

entry.bind("<Return>", lambda e: send_text())

btn_send = tk.Button(root, text="Enviar Comando", command=send_text)
btn_send.pack(pady=5)

# Output
output = tk.Text(root, height=20)
output.pack(padx=10, pady=10, fill="both", expand=True)

root.mainloop()