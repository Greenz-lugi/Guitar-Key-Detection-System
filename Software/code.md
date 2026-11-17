# Software

This directory contains the software components of the Guitar Key Detection System repository. It currently includes two independent projects involving audio processing, signal analysis, and note detection.

## Projects

### Vocal Note Detection using Computer Vision
**Directory:** `Vocal-Note-Detection-CV/`

A project exploring vocal note detection through audio processing and computer vision. The system uses a camera to detect a person's face while they sing, with the goal of identifying the musical notes being produced.

This was our initial approach at a higher abstraction level, whereas in embedded systems we work a bit more on the lower abstraction, dealing with the analog circuit which the other code introduces/works on. Both projects need more work and updates, however just putting it here to show our progress.

https://github.com/user-attachments/assets/01a16f5d-ef7f-4e2e-895d-b1913a67f139


###  Guitar Key Detection
**Directory:** `Guitar-Key-Detection/`

A project focused on detecting musical notes and chords played on an electric guitar. It processes guitar audio signals using digital signal processing techniques, including Fast Fourier Transform (FFT) analysis, to identify the notes and determine the musical key.

[Guitar Key Detection System (Officially Working)](https://drive.google.com/file/d/1wev5ISjBDpGWXXvGDHWWp2vrMQXHoieD/view?usp=sharing)

## External Libraries

### KissFFT
- **Purpose:** Fast Fourier Transform (FFT) processing.
- **Repository:** https://github.com/mborgerding/kissfft
- **License:** BSD-3-Clause
- **Author:** Mark Borgerding

KissFFT is an external library used for frequency-domain analysis. It is maintained separately from the project's original source code.
