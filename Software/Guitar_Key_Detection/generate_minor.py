
import wave
import math
import struct

SAMPLE_RATE = 12000
DURATION = 3.0

# A minor chord: A3, C4, E4
frequencies = [220.00, 261.63, 329.63]

with wave.open("test_A_minor.wav", "wb") as wav:
    wav.setnchannels(1)
    wav.setsampwidth(2)
    wav.setframerate(SAMPLE_RATE)

    for n in range(int(SAMPLE_RATE * DURATION)):
        t = n / SAMPLE_RATE

        sample = sum(
            math.sin(2 * math.pi * f * t)
            for f in frequencies
        ) / len(frequencies)

        sample *= 0.7
        pcm = int(sample * 32767)

        wav.writeframesraw(struct.pack("<h", pcm))

print("Created test_A_minor.wav")
print("Notes: A3, C4, E4")
print("Sample rate: 12000 Hz")
print("Duration: 3 seconds")
