/*
  Utility file to generate random BMP image
*/

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    PATTERN_SOLID = 0,
    PATTERN_CHECKER,
    PATTERN_GRADIENT,
    PATTERN_NOISE
} ImagePattern;

static void print_usage(const char *prog)
{
    printf("Usage: %s [output.bmp] [width] [height] [pattern]\n", prog);
    printf("Patterns: solid, checker, gradient, noise\n");
    printf("Example: %s sample.bmp 256 256 checker\n", prog);
}

static int parse_positive_int(const char *text, int fallback)
{
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (end == text || *end != '\0' || value <= 0 || value > 8192)
    {
        return fallback;
    }
    return (int)value;
}

static ImagePattern parse_pattern(const char *name)
{
    if (strcmp(name, "solid") == 0)
        return PATTERN_SOLID;
    if (strcmp(name, "checker") == 0)
        return PATTERN_CHECKER;
    if (strcmp(name, "gradient") == 0)
        return PATTERN_GRADIENT;
    if (strcmp(name, "noise") == 0)
        return PATTERN_NOISE;
    return PATTERN_SOLID;
}

static void write_pixel(unsigned char *pixels, int width, int x, int y,
                        unsigned char r, unsigned char g, unsigned char b)
{
    int idx = (y * width + x) * 3;
    pixels[idx] = r;
    pixels[idx + 1] = g;
    pixels[idx + 2] = b;
}

static void fill_solid(unsigned char *pixels, int width, int height)
{
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            write_pixel(pixels, width, x, y, 220, 80, 60);
        }
    }
}

static void fill_checker(unsigned char *pixels, int width, int height)
{
    const int cell = 16;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int is_light = ((x / cell) + (y / cell)) % 2;
            if (is_light)
            {
                write_pixel(pixels, width, x, y, 245, 210, 90);
            }
            else
            {
                write_pixel(pixels, width, x, y, 30, 35, 45);
            }
        }
    }
}

static void fill_gradient(unsigned char *pixels, int width, int height)
{
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            unsigned char r = (unsigned char)((x * 255) / (width > 1 ? (width - 1) : 1));
            unsigned char g = (unsigned char)((y * 255) / (height > 1 ? (height - 1) : 1));
            unsigned char b = (unsigned char)(255 - ((r + g) / 2));
            write_pixel(pixels, width, x, y, r, g, b);
        }
    }
}

static void fill_noise(unsigned char *pixels, int width, int height)
{
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            unsigned char r = (unsigned char)(rand() % 256);
            unsigned char g = (unsigned char)(rand() % 256);
            unsigned char b = (unsigned char)(rand() % 256);
            write_pixel(pixels, width, x, y, r, g, b);
        }
    }
}

int main(int argc, char **argv)
{
    const char *output = "test.bmp";
    int w = 64;
    int h = 64;
    int channels = 3;
    ImagePattern pattern = PATTERN_SOLID;

    if (argc > 1)
    {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        output = argv[1];
    }
    if (argc > 2)
        w = parse_positive_int(argv[2], w);
    if (argc > 3)
        h = parse_positive_int(argv[3], h);
    if (argc > 4)
        pattern = parse_pattern(argv[4]);

    unsigned char *pixels = (unsigned char *)malloc((size_t)w * (size_t)h * (size_t)channels);
    if (!pixels)
    {
        fprintf(stderr, "Failed to allocate image buffer (%dx%d).\n", w, h);
        return 1;
    }

    switch (pattern)
    {
    case PATTERN_SOLID:
        fill_solid(pixels, w, h);
        break;
    case PATTERN_CHECKER:
        fill_checker(pixels, w, h);
        break;
    case PATTERN_GRADIENT:
        fill_gradient(pixels, w, h);
        break;
    case PATTERN_NOISE:
        fill_noise(pixels, w, h);
        break;
    default:
        fill_solid(pixels, w, h);
        break;
    }

    if (!stbi_write_bmp(output, w, h, channels, pixels))
    {
        fprintf(stderr, "Failed to write BMP file: %s\n", output);
        free(pixels);
        return 1;
    }

    printf("Generated %s (%dx%d)\n", output, w, h);
    free(pixels);
    return 0;
}
