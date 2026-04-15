/*
  Utility file to generate random BMP image
*/

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

int main(void) {
    int w = 16, h = 16, channels = 3;
    unsigned char pixels[w * h * channels];

    // Fill with a solid red color
    for (int i = 0; i < w * h * channels; i += 3) {
        pixels[i]   = 220;  // R
        pixels[i+1] = 80;   // G
        pixels[i+2] = 60;   // B
    }

    stbi_write_bmp("test.bmp", w, h, channels, pixels);

    return 0;
}
