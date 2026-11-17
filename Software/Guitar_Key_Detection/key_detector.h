
#ifndef KEY_DETECTOR_H
#define KEY_DETECTOR_H

typedef struct {
    int root;                // 0=C, 1=C#, ..., 11=B
    int is_minor;            // 0=major, 1=minor
    double score;            // Best correlation
    double second_score;     // Runner-up correlation
    double confidence;       // Score separation, 0 to 100
} KeyResult;

KeyResult detect_key(const double chroma[12]);

void print_key_result(const KeyResult *result);

#endif
