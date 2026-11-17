#include "key_detector.h"
#include "chromagram.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "kiss_fft.h"

typedef struct {
    double sum[12];
    int window_count;
} ChromaAccumulator;

static void add_chroma(ChromaAccumulator *acc,
                       const double chroma[12])
{
    for (int i = 0; i < 12; i++) {
        acc->sum[i] += chroma[i];
    }

    acc->window_count++;
}

static void get_average_chroma(
    const ChromaAccumulator *acc,
    double average[12])
{
    for (int i = 0; i < 12; i++) {
        average[i] = 0.0;

        if (acc->window_count > 0) {
            average[i] =
                acc->sum[i] / acc->window_count;
        }
    }
}

#define FFT_SIZE 8192
#define SAMPLE_RATE 12000
#define PI 3.14159265358979323846
#define NUM_PEAKS 3

static const char *note_names[] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B"
};

// Read little-endian WAV integers.
static uint16_t read_u16(FILE *file)
{
    unsigned char b[2];
    if (fread(b, 1, 2, file) != 2)
        return 0;

    return (uint16_t)b[0] |
           ((uint16_t)b[1] << 8);
}

static uint32_t read_u32(FILE *file)
{
    unsigned char b[4];
    if (fread(b, 1, 4, file) != 4)
        return 0;

    return (uint32_t)b[0] |
           ((uint32_t)b[1] << 8) |
           ((uint32_t)b[2] << 16) |
           ((uint32_t)b[3] << 24);
}

// Locate PCM data without assuming a 44-byte header.
static int find_wav_data(FILE *file,
                         long *data_offset,
                         uint32_t *data_size)
{
    char id[4];

    if (fread(id, 1, 4, file) != 4 ||
        memcmp(id, "RIFF", 4) != 0)
        return 0;

    (void)read_u32(file);

    if (fread(id, 1, 4, file) != 4 ||
        memcmp(id, "WAVE", 4) != 0)
        return 0;

    int format_ok = 0;
    int found_data = 0;

    while (fread(id, 1, 4, file) == 4) {
        uint32_t size = read_u32(file);
        long chunk_start = ftell(file);

        if (memcmp(id, "fmt ", 4) == 0) {
            if (size < 16)
                return 0;

            uint16_t format = read_u16(file);
            uint16_t channels = read_u16(file);
            uint32_t rate = read_u32(file);

            (void)read_u32(file);
            (void)read_u16(file);

            uint16_t bits = read_u16(file);

            format_ok = (format == 1 &&
                         channels == 1 &&
                         rate == SAMPLE_RATE &&
                         bits == 16);
        }

        if (memcmp(id, "data", 4) == 0) {
            *data_offset = chunk_start;
            *data_size = size;
            found_data = 1;
        }

        if (fseek(file, chunk_start + size + (size & 1),
                  SEEK_SET) != 0)
            return 0;

        if (format_ok && found_data)
            return 1;
    }

    return format_ok && found_data;
}

static void print_note(double frequency)
{
    if (frequency <= 0.0) {
        printf("Unknown");
        return;
    }

    int midi = (int)lround(
        69.0 + 12.0 * log2(frequency / 440.0)
    );

    int pitch_class = ((midi % 12) + 12) % 12;
    int octave = (int)floor(midi / 12.0) - 1;

    printf("%s%d", note_names[pitch_class], octave);
}


static void analyze_window(FILE *file,
                           long data_offset,
                           uint32_t data_size,
                           double start_time,
                           kiss_fft_cfg cfg,
                           kiss_fft_cpx *input,
                           kiss_fft_cpx *output,
                           ChromaAccumulator *acc)

{
    long start_sample = (long)(start_time * SAMPLE_RATE);
    long total_samples = data_size / 2;

    if (start_sample + FFT_SIZE > total_samples) {
        printf("\n%.1f s: Not enough samples\n",
               start_time);
        return;
    }

    if (fseek(file, data_offset + start_sample * 2,
              SEEK_SET) != 0) {
        printf("Seek failed\n");
        return;
    }

    for (int i = 0; i < FFT_SIZE; i++) {
        unsigned char b[2];

        if (fread(b, 1, 2, file) != 2) {
            printf("Read failed\n");
            return;
        }

        uint16_t raw = (uint16_t)b[0] |
                       ((uint16_t)b[1] << 8);

        int32_t sample = raw < 32768
                       ? raw : (int32_t)raw - 65536;

        double window = 0.5 * (
            1.0 - cos(2.0 * PI * i / (FFT_SIZE - 1))
        );

        input[i].r = (sample / 32768.0) * window;
        input[i].i = 0.0;
    }
kiss_fft(cfg, input, output);
  kiss_fft(cfg, input, output);

// Calculate the 12-pitch-class chromagram
double chroma[12];

calculate_chromagram(output, chroma);
print_chromagram(chroma);
add_chroma(acc, chroma);
// Add this FFT window to the overall chromagram.
add_chroma(acc, chroma);
// Compare chromagram against 24 major/minor keys.
KeyResult key = detect_key(chroma);
print_key_result(&key);
    int selected[NUM_PEAKS];
    for (int i = 0; i < NUM_PEAKS; i++)
        selected[i] = -1;

    printf("\nTime: %.1f s\n", start_time);

    for (int pass = 0; pass < NUM_PEAKS; pass++) {
        double best = 0.0;
        int best_bin = -1;

        for (int k = 2; k < FFT_SIZE / 2; k++) {
            double freq =
                (double)k * SAMPLE_RATE / FFT_SIZE;

            if (freq < 70.0 || freq > 2000.0)
                continue;

            double left = hypot(output[k-1].r,
                                output[k-1].i);
            double current = hypot(output[k].r,
                                   output[k].i);
            double right = hypot(output[k+1].r,
                                 output[k+1].i);

            if (current <= left || current <= right)
                continue;

            int too_close = 0;

            for (int j = 0; j < pass; j++) {
                if (selected[j] >= 0 &&
                    abs(k - selected[j]) < 4) {
                    too_close = 1;
                    break;
                }
            }

            if (!too_close && current > best) {
                best = current;
                best_bin = k;
            }
        }

        if (best_bin < 0)
            break;

        selected[pass] = best_bin;

        double freq =
            (double)best_bin * SAMPLE_RATE / FFT_SIZE;

        printf("  Peak %d: %7.2f Hz -> ",
               pass + 1, freq);

        print_note(freq);
        printf("\n");
    }
}

int main(void)
{
  FILE *file = fopen("test_G_minor.wav", "rb");

    if (!file) {
        printf("Cannot open guitar_12k.wav\n");
        return 1;
    }

    long data_offset = 0;
    uint32_t data_size = 0;

    if (!find_wav_data(file, &data_offset, &data_size)) {
        printf("Unsupported or invalid WAV format.\n");
        fclose(file);
        return 1;
    } 

    kiss_fft_cpx *input =
        calloc(FFT_SIZE, sizeof(kiss_fft_cpx));

    kiss_fft_cpx *output =
        calloc(FFT_SIZE, sizeof(kiss_fft_cpx));

    kiss_fft_cfg cfg =
        kiss_fft_alloc(FFT_SIZE, 0, NULL, NULL);

    if (!input || !output || !cfg) {
        printf("Memory allocation failed.\n");
        free(input);
        free(output);
        free(cfg);
        fclose(file);
        return 1;
    }

    printf("ENSC 351 - Real Guitar FFT Analysis\n");
    printf("-----------------------------------\n");
    printf("Sample rate: %d Hz\n", SAMPLE_RATE);
    printf("FFT size: %d\n", FFT_SIZE);
    printf("Resolution: %.3f Hz\n",
           (double)SAMPLE_RATE / FFT_SIZE);
    printf("Duration: %.2f seconds\n",
           (double)data_size / (2 * SAMPLE_RATE));


ChromaAccumulator accumulator = {0};

// Analyze different chords throughout the recording.
double test_times[] = {
    0.5, 1.0,
    2.5, 3.0,
    4.5, 5.0,
    6.5, 7.0
};

int count = sizeof(test_times) / sizeof(test_times[0]);

for (int i = 0; i < count; i++) {
    analyze_window(file, data_offset, data_size,
                   test_times[i], cfg, input, output,
                   &accumulator);
}

// Calculate the average chromagram.
double average_chroma[12];
get_average_chroma(&accumulator, average_chroma);

printf("\n================================\n");
printf("OVERALL MUSICAL KEY DETECTION\n");
printf("================================\n");

printf("Windows analyzed: %d\n",
       accumulator.window_count);

print_chromagram(average_chroma);

KeyResult overall_key = detect_key(average_chroma);
print_key_result(&overall_key);



    free(input);
    free(output);
    free(cfg);
    fclose(file);

    return 0;
}
