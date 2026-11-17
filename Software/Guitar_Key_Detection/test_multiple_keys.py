
import wave
import math
import struct

SAMPLE_RATE = 12000
DURATION = 3

tests = {
    "G_major": [196.00, 246.94, 293.66],
    "D_major": [293.66, 369.99, 440.00],
    "E_minor": [164.81, 196.00, 246.94],
    "G_minor": [196.00, 233.08, 293.66]
}

for name, frequencies in tests.items():
    with wave.open(f"test_{name}.wav", "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)

        for i in range(SAMPLE_RATE * DURATION):
            t = i / SAMPLE_RATE
            sample = sum(
                math.sin(2 * math.pi * f * t)
                for f in frequencies
            ) / len(frequencies)

            value = int(0.7 * 32767 * sample)
            wav.writeframesraw(struct.pack("<h", value))

    print(f"Created test_{name}.wav")
