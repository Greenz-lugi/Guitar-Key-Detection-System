
import wave
import math
import struct

SAMPLE_RATE = 12000
DURATION = 2.0
FREQUENCY = 440.0
AMPLITUDE = 0.5

with wave.open("test_440.wav", "wb") as wav:
    wav.setnchannels(1)
    wav.setsampwidth(2)
    wav.setframerate(SAMPLE_RATE)

    for n in range(int(SAMPLE_RATE * DURATION)):
        sample = AMPLITUDE * math.sin(
            2 * math.pi * FREQUENCY * n / SAMPLE_RATE
        )
        pcm = int(sample * 32767)
        wav.writeframesraw(struct.pack("<h", pcm))

print("Created test_440.wav")
print("Frequency: 440 Hz")
print("Sample rate: 12000 Hz")
print("Duration: 2 seconds")
