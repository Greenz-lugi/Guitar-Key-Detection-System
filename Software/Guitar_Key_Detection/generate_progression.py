
import wave
import math
import struct

SAMPLE_RATE = 12000
CHORD_DURATION = 2.0

# Frequencies for each chord
progression = [
    ("C major", [261.63, 329.63, 392.00]),
    ("F major", [349.23, 440.00, 523.25]),
    ("G major", [392.00, 493.88, 587.33]),
    ("C major", [261.63, 329.63, 392.00])
]

with wave.open("test_C_progression.wav", "wb") as wav:
    wav.setnchannels(1)
    wav.setsampwidth(2)
    wav.setframerate(SAMPLE_RATE)

    for chord_name, frequencies in progression:
        frames = bytearray()

        for n in range(int(SAMPLE_RATE * CHORD_DURATION)):
            t = n / SAMPLE_RATE

            sample = sum(
                math.sin(2 * math.pi * f * t)
                for f in frequencies
            ) / len(frequencies)

            # Short fade-in/out to avoid clicks
            fade = min(1.0, t / 0.02,
                       (CHORD_DURATION - t) / 0.02)
            sample *= 0.7 * max(0.0, fade)

            pcm = int(max(-1.0, min(1.0, sample)) * 32767)
            frames.extend(struct.pack("<h", pcm))

        wav.writeframes(frames)
        print("Generated:", chord_name)

print("Created test_C_progression.wav")
print("Duration: 8 seconds")
