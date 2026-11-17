
import wave
import math
import struct

SAMPLE_RATE = 12000
DURATION = 3.0
AMPLITUDE = 0.25

# C major chord: C4, E4, G4
frequencies = [261.63, 329.63, 392.00]

with wave.open("test_C_major.wav", "wb") as wav:
    wav.setnchannels(1)
    wav.setsampwidth(2)
    wav.setframerate(SAMPLE_RATE)

    for n in range(int(SAMPLE_RATE * DURATION)):
        t = n / SAMPLE_RATE

        sample = sum(
            math.sin(2 * math.pi * f * t)
            for f in frequencies
        )

        sample *= AMPLITUDE

        pcm = int(sample * 32767)
        pcm = max(-32768, min(32767, pcm))

        wav.writeframesraw(struct.pack("<h", pcm))

print("Created test_C_major.wav")
print("Notes: C4, E4, G4")
print("Frequencies:", frequencies)
print("Sample rate:", SAMPLE_RATE, "Hz")
