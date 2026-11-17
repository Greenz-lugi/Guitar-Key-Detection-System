
#include <stdio.h>
#include <math.h>
#include "kiss_fft.h"

#define FFT_SIZE 8192
#define SAMPLE_RATE 12000

static const char *note_names[12] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
};

void calculate_chromagram(
    const kiss_fft_cpx *spectrum,
    double chroma[12])
{
    for (int i = 0; i < 12; i++)
        chroma[i] = 0.0;

    for (int k = 1; k <= FFT_SIZE / 2; k++) {
        double frequency =
            (double)k * SAMPLE_RATE / FFT_SIZE;

        if (frequency < 70.0 || frequency > 2000.0)
            continue;

        double magnitude = hypot(
            spectrum[k].r, spectrum[k].i
        );

        int midi = (int)lround(
            69.0 + 12.0 * log2(frequency / 440.0)
        );

        int pitch_class = ((midi % 12) + 12) % 12;

        chroma[pitch_class] += magnitude;
    }

    double total = 0.0;

    for (int i = 0; i < 12; i++)
        total += chroma[i];

    if (total > 0.0) {
        for (int i = 0; i < 12; i++)
            chroma[i] /= total;
    }
}

void print_chromagram(const double chroma[12])
{
    printf("\n12-Pitch-Class Chromagram:\n");
    printf("--------------------------\n");

    for (int i = 0; i < 12; i++) {
        printf("%-3s : %.4f\n",
               note_names[i], chroma[i]);
    }
}
