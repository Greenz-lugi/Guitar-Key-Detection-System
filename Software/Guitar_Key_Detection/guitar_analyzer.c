#include "guitar_analyzer.h"
#include "chromagram.h"
#include "kiss_fft.h"
#include <math.h>
#include <stdlib.h>

#define PI 3.14159265358979323846

struct GuitarAnalyzer {
    kiss_fft_cfg cfg;
    kiss_fft_cpx *input;
    kiss_fft_cpx *output;
};

void guitar_analyzer_destroy(GuitarAnalyzer *analyzer)
{
    if (!analyzer) return;
    free(analyzer->cfg);
    free(analyzer->input);
    free(analyzer->output);
    free(analyzer);
}

GuitarAnalyzer *guitar_analyzer_create(void)
{
    GuitarAnalyzer *analyzer = calloc(1, sizeof(*analyzer));
    if (!analyzer) return NULL;
    analyzer->input = calloc(GUITAR_FFT_SIZE, sizeof(*analyzer->input));
    analyzer->output = calloc(GUITAR_FFT_SIZE, sizeof(*analyzer->output));
    analyzer->cfg = kiss_fft_alloc(GUITAR_FFT_SIZE, 0, NULL, NULL);
    if (!analyzer->input || !analyzer->output || !analyzer->cfg) {
        guitar_analyzer_destroy(analyzer);
        return NULL;
    }
    return analyzer;
}

int guitar_analyzer_process(GuitarAnalyzer *analyzer,
                            const int16_t samples[GUITAR_FFT_SIZE],
                            GuitarAnalysis *result)
{
    if (!analyzer || !samples || !result) return -1;

    for (int i = 0; i < GUITAR_FFT_SIZE; ++i) {
        double hann = 0.5 * (1.0 - cos(2.0 * PI * i /
                                      (GUITAR_FFT_SIZE - 1)));
        analyzer->input[i].r = (samples[i] / 32768.0) * hann;
        analyzer->input[i].i = 0.0;
    }

    kiss_fft(analyzer->cfg, analyzer->input, analyzer->output);
    calculate_chromagram(analyzer->output, result->chroma);
    result->key = detect_key(result->chroma);

    double best = 0.0;
    int best_bin = 0;
    for (int k = 1; k <= GUITAR_FFT_SIZE / 2; ++k) {
        double frequency = (double)k * GUITAR_SAMPLE_RATE / GUITAR_FFT_SIZE;
        if (frequency < 70.0 || frequency > 2000.0) continue;
        double magnitude = hypot(analyzer->output[k].r,
                                 analyzer->output[k].i);
        if (magnitude > best) {
            best = magnitude;
            best_bin = k;
        }
    }
    result->strongest_frequency_hz =
        (double)best_bin * GUITAR_SAMPLE_RATE / GUITAR_FFT_SIZE;
    return 0;
}
