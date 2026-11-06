"""Recreate this project's deterministic PCM effects, including the squad alert."""
from pathlib import Path
import math
import random
import struct
import wave

ROOT = Path(__file__).resolve().parent / "AudioSource"
RATE = 22050


def effect(name, duration, start, end, noise=0.0, harmony=1.5):
    rng = random.Random(name)
    frames = bytearray()
    phase = 0.0
    count = int(RATE * duration)
    for i in range(count):
        position = i / max(1, count - 1)
        phase += 2 * math.pi * (start + (end - start) * position) / RATE
        envelope = min(1.0, i / (RATE * 0.008)) * (1.0 - position) ** 1.6
        tone = 0.65 * math.sin(phase) + 0.22 * math.sin(phase * harmony)
        sample = envelope * ((1 - noise) * tone + noise * rng.uniform(-1, 1))
        frames.extend(struct.pack("<h", int(max(-1, min(1, sample)) * 22000)))
    with wave.open(str(ROOT / (name + ".wav")), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(RATE)
        output.writeframes(frames)


def main():
    ROOT.mkdir(parents=True, exist_ok=True)
    effect("S_PlayerShot", 0.13, 700, 130, noise=0.32)
    effect("S_EnemyFire", 0.19, 850, 260, noise=0.10)
    effect("S_EnemyHit", 0.11, 500, 190, noise=0.24)
    effect("S_EnemyDown", 0.48, 250, 55, noise=0.45)
    effect("S_PlayerHit", 0.20, 180, 65, noise=0.15)
    effect("S_Checkpoint", 0.65, 440, 880, harmony=1.25)
    effect("S_Win", 1.20, 440, 1100, harmony=1.25)
    wave_alert()
    print("Generated 8 original PCM sound effects.")


def wave_alert():
    """Two short alarm pulses distinguish incoming squads from checkpoint saves."""
    ROOT.mkdir(parents=True, exist_ok=True)
    frames = bytearray()
    for i in range(int(RATE * 0.6)):
        t = i / RATE
        local = t % 0.3
        envelope = max(0.0, math.sin(math.pi * local / 0.22)) if local < 0.22 else 0.0
        frequency = 540 if t < 0.3 else 720
        value = 0.35 * envelope * math.sin(2 * math.pi * frequency * t)
        frames.extend(struct.pack("<h", int(value * 32767)))
    with wave.open(str(ROOT / "S_WaveAlert.wav"), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(RATE)
        output.writeframes(frames)


if __name__ == "__main__":
    main()
