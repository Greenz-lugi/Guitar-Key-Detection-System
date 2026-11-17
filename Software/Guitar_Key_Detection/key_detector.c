
#include <stdio.h>
#include <math.h>
#include "key_detector.h"

#define NUM_NOTES 12

static const char *note_names[NUM_NOTES] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
};

// Krumhansl-Schmuckler major/minor key profiles.
static const double major_profile[NUM_NOTES] = {
    6.35, 2.23, 3.48, 2.33,
    4.38, 4.09, 2.52, 5.19,
    2.39, 3.66, 2.29, 2.88
};

static const double minor_profile[NUM_NOTES] = {
    6.33, 2.68, 3.52, 5.38,
    2.60, 3.53, 2.54, 4.75,
    3.98, 2.69, 3.34, 3.17
};

// Pearson correlation between chroma and a key profile.
static double correlation(const double chroma[12],
                          const double profile[12],
                          int root)
{
    double mean_chroma = 0.0;
    double mean_profile = 0.0;

    for (int i = 0; i < NUM_NOTES; i++) {
        mean_chroma += chroma[i];
        mean_profile += profile[i];
    }

    mean_chroma /= NUM_NOTES;
    mean_profile /= NUM_NOTES;

    double numerator = 0.0;
    double denominator_a = 0.0;
    double denominator_b = 0.0;

    for (int i = 0; i < NUM_NOTES; i++) {
        int index = (i - root + NUM_NOTES) % NUM_NOTES;

        double a = chroma[i] - mean_chroma;
        double b = profile[index] - mean_profile;

        numerator += a * b;
        denominator_a += a * a;
        denominator_b += b * b;
    }

    double denominator =
        sqrt(denominator_a * denominator_b);

    if (denominator < 1e-12)
        return 0.0;

    return numerator / denominator;
}

KeyResult detect_key(const double chroma[12])
{
    KeyResult result = {
        .root = -1,
        .is_minor = 0,
        .score = -2.0,
        .second_score = -2.0,
        .confidence = 0.0
    };

    double total = 0.0;
    for (int i = 0; i < NUM_NOTES; i++)
        total += fabs(chroma[i]);

    // Avoid reporting a key for silence.
    if (total < 1e-9)
        return result;

    for (int root = 0; root < NUM_NOTES; root++) {
        for (int mode = 0; mode < 2; mode++) {
            const double *profile =
                mode ? minor_profile : major_profile;

            double score =
                correlation(chroma, profile, root);

            if (score > result.score) {
                result.second_score = result.score;
                result.score = score;
                result.root = root;
                result.is_minor = mode;
            }
            else if (score > result.second_score) {
                result.second_score = score;
            }
        }
    }

    // Heuristic separation between best and second-best.
    // This is NOT a calibrated probability.
    double gap = result.score - result.second_score;

    result.confidence = 100.0 * gap / 2.0;

    if (result.confidence < 0.0)
        result.confidence = 0.0;

    if (result.confidence > 100.0)
        result.confidence = 100.0;

    return result;
}

void print_key_result(const KeyResult *result)
{
    printf("\nMusical Key Detection:\n");
    printf("----------------------\n");

    if (result->root < 0) {
        printf("Detected key: Unknown\n");
        return;
    }

    printf("Detected key: %s %s\n",
           note_names[result->root],
           result->is_minor ? "minor" : "major");

    printf("Correlation score: %.4f\n",
           result->score);

    printf("Second-best score: %.4f\n",
           result->second_score);

    printf("Confidence (heuristic): %.2f%%\n",
           result->confidence);
}
