#ifndef GUITAR_ANALYZER_H
#define GUITAR_ANALYZER_H

#include <stdint.h>
#include "key_detector.h"

#define GUITAR_SAMPLE_RATE 12000
#define GUITAR_FFT_SIZE 8192
#define GUITAR_PITCH_CLASSES 12

typedef struct GuitarAnalyzer GuitarAnalyzer;

typedef struct {
    double chroma[GUITAR_PITCH_CLASSES];
    KeyResult key;
    double strongest_frequency_hz;
} GuitarAnalysis;

/* Allocate FFT resources once. Returns NULL on allocation failure. */
GuitarAnalyzer *guitar_analyzer_create(void);

/* Process exactly 8192 mono, signed 16-bit PCM samples at 12000 Hz.
   Returns 0 on success, -1 for invalid arguments. */
int guitar_analyzer_process(GuitarAnalyzer *analyzer,
                            const int16_t samples[GUITAR_FFT_SIZE],
                            GuitarAnalysis *result);

void guitar_analyzer_destroy(GuitarAnalyzer *analyzer);

#endif
