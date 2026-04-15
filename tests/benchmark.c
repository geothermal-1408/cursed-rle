// benchmark.c  — add to Makefile same way as test_simd.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "rle_img.h"

#define WIDTH  3840
#define HEIGHT 2160
#define CH     4        /* change to 1 for grayscale */

int main(void)
{
    int px  = WIDTH * HEIGHT;
    int raw = px * CH;

    unsigned char *src = malloc(raw);
    unsigned char *enc = malloc(raw * 2);
    unsigned char *dec = malloc(raw);

    /* Simulate a real image: large uniform regions with occasional changes */
    memset(src, 0x7F, raw);
    for (int i = 0; i < px; i += 64)   /* noise every 64 pixels */
        src[i * CH] ^= 0xFF;

    int REPS = 20;
    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    int enc_len = 0;
    for (int r = 0; r < REPS; r++)
        enc_len = rle_encode_binary(src, raw, enc, raw * 2, CH);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double enc_ms = ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec))
                    / 1e6 / REPS;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int r = 0; r < REPS; r++)
        rle_decode_binary(enc, enc_len, dec, raw, CH);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double dec_ms = ((t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec))
                    / 1e6 / REPS;

    printf("Image: %dx%d ch=%d  (%d MB raw)\n", WIDTH, HEIGHT, CH, raw / 1024 / 1024);
    printf("Encode: %.2f ms/frame\n", enc_ms);
    printf("Decode: %.2f ms/frame\n", dec_ms);

    free(src); free(enc); free(dec);
    return 0;
}
