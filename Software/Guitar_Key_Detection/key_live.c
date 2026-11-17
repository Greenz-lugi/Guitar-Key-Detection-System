/*
 * key_live.c  -  live guitar key detection on the BeagleY-AI
 *
 * Reads the analog front end through an MCP3208 (CH0) on /dev/spidev0.0,
 * samples at 12 kHz, converts 12-bit ADC counts to the 16-bit PCM that
 * guitar_analyzer_process() expects, and prints the detected key.
 *
 * Build (on the Beagle, from the project folder):
 *   gcc -O2 -Wall -Ikissfft key_live.c guitar_analyzer.c chromagram.c \
 *       key_detector.c kissfft/kiss_fft.c -lm -o key_live
 *
 * Run:
 *   sudo ./key_live            # run until Ctrl+C
 *   sudo ./key_live -n 5       # capture 5 blocks then stop
 *   sudo ./key_live -v         # also print the chromagram bars
 *   sudo ./key_live -d         # also save each block as capture_NNN.wav
 *
 * Desktop self-test (no hardware): build with -DFAKE_ADC and run
 *   ./key_live -f test_C_major.wav -n 4
 */
#define _GNU_SOURCE
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "guitar_analyzer.h"
#include "key_detector.h"

#ifndef FAKE_ADC
#include <fcntl.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#endif

/* ---------------- configuration ---------------- */
#define SPI_DEVICE    "/dev/spidev0.0"
#define SPI_SPEED_HZ  1000000   /* MCP3208 @ 3.3 V: keep <= ~1 MHz */
#define ADC_CHANNEL   0         /* MCP3208 CH0 */
#define ADC_MID       2048      /* 1.65 V bias = mid-scale */
#define PCM_SCALE     16        /* 12-bit counts -> 16-bit PCM */

#define SILENCE_PP    60        /* peak-to-peak counts below this = silence */
#define CLIP_LO       30        /* raw counts at/below = op-amp hit 0 V rail */
#define CLIP_HI       4065      /* raw counts at/above = hit 3.3 V rail */
#define BIAS_TOL      300       /* warn if block mean is this far from 2048 */
#define AVG_BLOCKS    4         /* average chroma over this many loud blocks */
#define RESET_QUIET   3         /* silent blocks in a row that clear history */

#define SAMPLE_RATE   GUITAR_SAMPLE_RATE   /* 12000 Hz, from partner's header */
#define BLOCK         GUITAR_FFT_SIZE      /* 8192 samples = 0.683 s */

static const char *NOTE[12] = {"C", "C#", "D", "D#", "E", "F",
                               "F#", "G", "G#", "A", "A#", "B"};

static volatile sig_atomic_t g_stop = 0;
static void on_sigint(int s) { (void)s; g_stop = 1; }

/* ---------------- ADC access ---------------- */
#ifdef FAKE_ADC
/* Test mode: play a 12 kHz mono 16-bit WAV through the same pipeline. */
static int16_t *g_fake = NULL;
static size_t g_fake_len = 0, g_fake_pos = 0;

static int fake_load(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return -1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc((size_t)size);
    if (!buf || fread(buf, 1, (size_t)size, f) != (size_t)size) {
        fclose(f); free(buf); return -1;
    }
    fclose(f);
    long p = 12;
    while (p + 8 <= size) {
        uint32_t csz = buf[p+4] | (buf[p+5] << 8) | (buf[p+6] << 16) |
                       ((uint32_t)buf[p+7] << 24);
        if (memcmp(buf + p, "data", 4) == 0) {
            g_fake_len = csz / 2;
            if ((long)(p + 8 + csz) > size) g_fake_len = (size - p - 8) / 2;
            g_fake = malloc(g_fake_len * sizeof(int16_t));
            memcpy(g_fake, buf + p + 8, g_fake_len * sizeof(int16_t));
            free(buf);
            return 0;
        }
        p += 8 + csz + (csz & 1);
    }
    free(buf);
    return -1;
}

static int adc_open(const char *wav)
{
    if (!wav) { fprintf(stderr, "FAKE_ADC build needs -f file.wav\n"); return -1; }
    return fake_load(wav);
}

static int adc_read(void)
{
    int v = ADC_MID + g_fake[g_fake_pos] / PCM_SCALE + (rand() % 5 - 2);
    g_fake_pos = (g_fake_pos + 1) % g_fake_len;
    return v < 0 ? 0 : (v > 4095 ? 4095 : v);
}
#else
static int g_spi = -1;

static int adc_open(const char *unused)
{
    (void)unused;
    g_spi = open(SPI_DEVICE, O_RDWR);
    if (g_spi < 0) { perror("open " SPI_DEVICE); return -1; }
    uint8_t mode = 0, bits = 8;
    uint32_t speed = SPI_SPEED_HZ;
    if (ioctl(g_spi, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(g_spi, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(g_spi, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("SPI setup");
        return -1;
    }
    return 0;
}

/* One single-ended conversion: 3-byte frame, 12-bit result. */
static int adc_read(void)
{
    uint8_t tx[3] = {(uint8_t)(0x06 | ((ADC_CHANNEL >> 2) & 1)),
                     (uint8_t)((ADC_CHANNEL & 3) << 6), 0};
    uint8_t rx[3] = {0, 0, 0};
    struct spi_ioc_transfer t;
    memset(&t, 0, sizeof t);
    t.tx_buf = (unsigned long)tx;
    t.rx_buf = (unsigned long)rx;
    t.len = 3;
    t.speed_hz = SPI_SPEED_HZ;
    t.bits_per_word = 8;
    if (ioctl(g_spi, SPI_IOC_MESSAGE(1), &t) < 0) return -1;
    return ((rx[1] & 0x0F) << 8) | rx[2];
}
#endif

/* ---------------- timing ---------------- */
static int64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

#ifndef FAKE_ADC
static void sleep_until_ns(int64_t t)
{
    struct timespec ts = {.tv_sec = t / 1000000000LL,
                          .tv_nsec = t % 1000000000LL};
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) == EINTR)
        ;
}
#endif

/* Capture BLOCK samples at SAMPLE_RATE. Sample i is scheduled at
 * start + i/SAMPLE_RATE (no drift). Returns 0 on success. */
static int capture_block(int *raw, int *late, double *elapsed_s)
{
    const int64_t period = 1000000000LL / SAMPLE_RATE;
    int64_t base = now_ns();
    *late = 0;
    for (int i = 0; i < BLOCK; i++) {
        int64_t target = base + (int64_t)i * 1000000000LL / SAMPLE_RATE;
        (void)target; (void)period;
#ifndef FAKE_ADC
        sleep_until_ns(target);
        if (now_ns() > target + period / 2) (*late)++;
#endif
        int v = adc_read();
        if (v < 0) { fprintf(stderr, "SPI read failed\n"); return -1; }
        raw[i] = v;
        if (g_stop) return -1;
    }
    *elapsed_s = (now_ns() - base) / 1e9;
    return 0;
}

/* ---------------- WAV dump (for offline checks) ---------------- */
static void wav_write(const char *path, const int16_t *s, int n)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return; }
    uint32_t data = (uint32_t)n * 2, riff = 36 + data;
    uint32_t rate = SAMPLE_RATE, brate = SAMPLE_RATE * 2, fmtsz = 16;
    uint16_t fmt = 1, ch = 1, align = 2, bits = 16;
    fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmtsz, 4, 1, f);
    fwrite(&fmt, 2, 1, f); fwrite(&ch, 2, 1, f);
    fwrite(&rate, 4, 1, f); fwrite(&brate, 4, 1, f);
    fwrite(&align, 2, 1, f); fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&data, 4, 1, f);
    fwrite(s, 2, (size_t)n, f);
    fclose(f);
}

/* ---------------- main ---------------- */
int main(int argc, char **argv)
{
    int max_blocks = 0, verbose = 0, dump = 0;
    const char *wav = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "n:vdf:")) != -1) {
        switch (opt) {
        case 'n': max_blocks = atoi(optarg); break;
        case 'v': verbose = 1; break;
        case 'd': dump = 1; break;
        case 'f': wav = optarg; break;
        default:
            fprintf(stderr, "usage: %s [-n blocks] [-v] [-d] [-f file.wav]\n",
                    argv[0]);
            return 2;
        }
    }

    signal(SIGINT, on_sigint);
    if (adc_open(wav) != 0) return 1;

#ifndef FAKE_ADC
    /* Best-effort real-time scheduling for steady 12 kHz timing. */
    struct sched_param sp = {.sched_priority = 50};
    if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0)
        fprintf(stderr, "note: no real-time priority (run with sudo)\n");
    mlockall(MCL_CURRENT | MCL_FUTURE);
#endif

    GuitarAnalyzer *an = guitar_analyzer_create();
    if (!an) { fprintf(stderr, "analyzer alloc failed\n"); return 1; }

    static int raw[BLOCK];
    static int16_t pcm[BLOCK];
    double hist[AVG_BLOCKS][12];
    int hist_n = 0, hist_pos = 0, quiet_run = 0;

    printf("Live key detection: MCP3208 CH0, %d Hz, %d-sample blocks (%.2f s)\n",
           SAMPLE_RATE, BLOCK, (double)BLOCK / SAMPLE_RATE);
    printf("Play a few chords. Ctrl+C to stop.\n\n");

    for (int blk = 1; !g_stop && (max_blocks == 0 || blk <= max_blocks); blk++) {
        int late = 0;
        double elapsed = 0.0;
        if (capture_block(raw, &late, &elapsed) != 0) break;

        /* block statistics on the raw ADC counts */
        long sum = 0;
        int mn = 4095, mx = 0, clip = 0;
        for (int i = 0; i < BLOCK; i++) {
            sum += raw[i];
            if (raw[i] < mn) mn = raw[i];
            if (raw[i] > mx) mx = raw[i];
            if (raw[i] <= CLIP_LO || raw[i] >= CLIP_HI) clip++;
        }
        double mean = (double)sum / BLOCK;
        int pp = mx - mn;
        double rate = BLOCK / elapsed;
        (void)rate;

        /* 12-bit counts -> DC-free signed 16-bit PCM */
        for (int i = 0; i < BLOCK; i++) {
            long v = lround((raw[i] - mean) * PCM_SCALE);
            pcm[i] = (int16_t)(v > 32767 ? 32767 : (v < -32768 ? -32768 : v));
        }
        if (dump) {
            char name[64];
            snprintf(name, sizeof name, "capture_%03d.wav", blk);
            wav_write(name, pcm, BLOCK);
        }

        if (pp < SILENCE_PP) {
            printf("#%03d  silence  (pp %d, mean %.0f)\n", blk, pp, mean);
            if (++quiet_run >= RESET_QUIET && hist_n > 0) {
                hist_n = hist_pos = 0;
                printf("      (history cleared)\n");
            }
            continue;
        }
        quiet_run = 0;

        GuitarAnalysis res;
        if (guitar_analyzer_process(an, pcm, &res) != 0) {
            fprintf(stderr, "analysis failed\n");
            break;
        }

        /* rolling average of the last AVG_BLOCKS loud blocks */
        memcpy(hist[hist_pos], res.chroma, sizeof hist[0]);
        hist_pos = (hist_pos + 1) % AVG_BLOCKS;
        if (hist_n < AVG_BLOCKS) hist_n++;
        double avg[12] = {0};
        for (int b = 0; b < hist_n; b++)
            for (int k = 0; k < 12; k++) avg[k] += hist[b][k] / hist_n;
        KeyResult key = detect_key(avg);

        printf("#%03d  key %-2s %-5s  conf %3.0f%%  (avg of %d)  "
               "loudest %6.1f Hz  pp %4d\n",
               blk,
               key.root >= 0 ? NOTE[key.root] : "?",
               key.root >= 0 ? (key.is_minor ? "minor" : "major") : "",
               key.confidence, hist_n, res.strongest_frequency_hz, pp);

        if (verbose) {
            for (int k = 0; k < 12; k++) {
                printf("      %-2s %5.3f ", NOTE[k], avg[k]);
                for (int j = 0; j < (int)(avg[k] * 100); j++) putchar('#');
                putchar('\n');
            }
        }
        if (clip > 0)
            printf("      WARNING: %d clipped samples - lower the gain trimmer\n",
                   clip);
        if (fabs(mean - ADC_MID) > BIAS_TOL)
            printf("      WARNING: DC level %.0f, expected ~%d - check bias\n",
                   mean, ADC_MID);
#ifndef FAKE_ADC
        if (late > BLOCK / 100 || fabs(rate - SAMPLE_RATE) > SAMPLE_RATE * 0.01)
            printf("      WARNING: timing off - %d late samples, %.0f Hz "
                   "effective\n", late, rate);
#endif
    }

    guitar_analyzer_destroy(an);
    printf("\nDone.\n");
    return 0;
}
