
#ifndef CHROMAGRAM_H
#define CHROMAGRAM_H

#include "kiss_fft.h"

void calculate_chromagram(
    const kiss_fft_cpx *spectrum,
    double chroma[12]);

void print_chromagram(const double chroma[12]);

#endif
