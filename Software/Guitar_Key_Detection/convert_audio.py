
import sys
import soundfile as sf
import numpy as np
from scipy.signal import resample_poly
from fractions import Fraction

if len(sys.argv) != 3:
    print("Usage: python3 convert_audio.py input.wav output.wav")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

audio, sample_rate = sf.read(input_file)

# Convert stereo to mono
if audio.ndim == 2:
    audio = audio.mean(axis=1)

# Resample to 12 kHz with anti-alias filtering
target_rate = 12000
ratio = Fraction(target_rate, sample_rate)

audio = resample_poly(
    audio,
    ratio.numerator,
    ratio.denominator
)

# Avoid clipping
peak = np.max(np.abs(audio))
if peak > 1.0:
    audio = audio / peak

# Save as mono 16-bit PCM WAV
sf.write(
    output_file,
    audio,
    target_rate,
    subtype="PCM_16"
)

print("Converted successfully!")
print("Output:", output_file)
print("Sample rate:", target_rate, "Hz")
print("Channels: Mono")
print("Format: 16-bit PCM WAV")
