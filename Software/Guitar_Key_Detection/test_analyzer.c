
#include <stdio.h>
#include <stdint.h>
#include "guitar_analyzer.h"

int main(void)
{
    FILE *file = fopen("test_C_major.wav", "rb");

    if (!file) {
        printf("Cannot open WAV file.\n");
        return 1;
    }

    // Test WAV files have a standard 44-byte header.
    if (fseek(file, 44, SEEK_SET) != 0) {
        fclose(file);
        return 1;
    }

    int16_t samples[GUITAR_FFT_SIZE];

    if (fread(samples, sizeof(int16_t),
              GUITAR_FFT_SIZE, file) != GUITAR_FFT_SIZE) {
        printf("Not enough audio samples.\n");
        fclose(file);
        return 1;
    }

    fclose(file);

    GuitarAnalyzer *analyzer = guitar_analyzer_create();

    if (!analyzer) {
        printf("Failed to create analyzer.\n");
        return 1;
    }

    GuitarAnalysis result;

    if (guitar_analyzer_process(analyzer, samples, &result) != 0) {
        printf("Analysis failed.\n");
        guitar_analyzer_destroy(analyzer);
        return 1;
    }

    printf("\nENSC 351 - Guitar Analyzer Test\n");
    printf("-------------------------------\n");

    printf("Strongest frequency: %.2f Hz\n",
           result.strongest_frequency_hz);

    printf("\nChromagram:\n");

    const char *notes[] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };

    for (int i = 0; i < GUITAR_PITCH_CLASSES; i++) {
        printf("%-2s : %.4f\n", notes[i], result.chroma[i]);
    }

const char *note_names[12] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
};

printf("\nMusical Key Detection:\n");
printf("----------------------\n");

if (result.key.root >= 0 && result.key.root < 12) {
    printf("Detected key: %s %s\n",
           note_names[result.key.root],
           result.key.is_minor ? "minor" : "major");

    printf("Correlation score: %.4f\n",
           result.key.score);

    printf("Second-best score: %.4f\n",
           result.key.second_score);

    printf("Confidence (heuristic): %.2f%%\n",
           result.key.confidence);
} else {
    printf("No valid musical key detected.\n");
}

    guitar_analyzer_destroy(analyzer);

    return 0;
}
