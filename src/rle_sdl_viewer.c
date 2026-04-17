#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "rle_format.h"

#define IO_BUFFER_SIZE (1024 * 1024 * 4) /* 4MB chunks for fast I/O */
#define rle_sdl_error(fmt, ...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[RLE Error] " fmt, ##__VA_ARGS__)

#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600

/* Fast pixel fill; ARM64 path writes two pixels per 64-bit store. */
static inline void fast_fill_pixels_arm64(uint32_t *restrict dest, uint32_t color, size_t count)
{
#if defined(__aarch64__)
  uint64_t color_pair = ((uint64_t)color << 32) | (uint64_t)color;
  uint64_t *dest64 = (uint64_t *)dest;
  size_t pairs = count >> 1;

  while (pairs--)
  {
    *dest64++ = color_pair;
  }

  if (count & 1)
  {
    *((uint32_t *)dest64) = color;
  }
#else
  while (count--)
  {
    *dest++ = color;
  }
#endif
}

/* Core decoding routine */
static bool decode_rle_to_texture(FILE *file, SDL_Texture *texture, const RLEheader *header)
{
  void *pixels;
  int pitch;

  if (!SDL_LockTexture(texture, NULL, &pixels, &pitch))
  {
    rle_sdl_error("Failed to lock texture: %s", SDL_GetError());
    return false;
  }

  size_t total_pixels = (size_t)header->org_width * header->org_height;
  size_t pixels_written = 0;

  int chunk_size = 1 + header->channels;
  if (chunk_size > 5)
    chunk_size = 5;

  uint8_t chunk[5];
  while (pixels_written < total_pixels && (int)fread(chunk, 1, chunk_size, file) == chunk_size)
  {
    uint8_t count = chunk[0];
    uint32_t color = 0;

    if (header->channels == 3)
    {
      color = (0xFF << 24) | (chunk[1] << 16) | (chunk[2] << 8) | chunk[3];
    }
    else if (header->channels == 4)
    {
      color = (chunk[4] << 24) | (chunk[1] << 16) | (chunk[2] << 8) | chunk[3];
    }
    else if (header->channels == 1)
    {
      color = (0xFF << 24) | (chunk[1] << 16) | (chunk[1] << 8) | chunk[1];
    }

    size_t safe_count = (pixels_written + count > total_pixels)
                            ? (total_pixels - pixels_written)
                            : count;

    while (safe_count > 0)
    {
      size_t current_x = pixels_written % header->org_width;
      size_t current_y = pixels_written / header->org_width;

      uint32_t *row_ptr = (uint32_t *)((uint8_t *)pixels + (current_y * pitch)) + current_x;

      size_t pixels_left_in_row = header->org_width - current_x;
      size_t write_count = (safe_count < pixels_left_in_row) ? safe_count : pixels_left_in_row;

      fast_fill_pixels_arm64(row_ptr, color, write_count);

      safe_count -= write_count;
      pixels_written += write_count;
    }
  }

  SDL_UnlockTexture(texture);
  return pixels_written == total_pixels;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    SDL_Log("Usage: %s <encoded.rle>", argv[0]);
    return EXIT_FAILURE;
  }

  FILE *file = fopen(argv[1], "rb");
  if (!file)
  {
    rle_sdl_error("Failed to open encoded file: %s", argv[1]);
    return EXIT_FAILURE;
  }

  char *io_buf = (char *)malloc(IO_BUFFER_SIZE);
  if (io_buf)
  {
    setvbuf(file, io_buf, _IOFBF, IO_BUFFER_SIZE);
  }

  RLEheader header;
  if (fread(&header, sizeof(RLEheader), 1, file) != 1)
  {
    rle_sdl_error("Failed to read header from %s.", argv[1]);
    fclose(file);
    if (io_buf)
      free(io_buf);
    return EXIT_FAILURE;
  }

  if (header.magic[0] != RLE_MAGIC0 || header.magic[1] != RLE_MAGIC1 || header.magic[2] != RLE_MAGIC2)
  {
    rle_sdl_error("Invalid magic number. Not a valid RLE file.");
    fclose(file);
    if (io_buf)
      free(io_buf);
    return EXIT_FAILURE;
  }

  if (header.org_width == 0 || header.org_height == 0)
  {
    rle_sdl_error("Invalid image dimensions in header: %dx%d", header.org_width, header.org_height);
    fclose(file);
    if (io_buf)
      free(io_buf);
    return EXIT_FAILURE;
  }

  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    rle_sdl_error("SDL_Init failed: %s", SDL_GetError());
    fclose(file);
    if (io_buf)
      free(io_buf);
    return EXIT_FAILURE;
  }

  SDL_Window *window = SDL_CreateWindow(
      "RLE Direct Render",
      DEFAULT_WINDOW_WIDTH,
      DEFAULT_WINDOW_HEIGHT,
      SDL_WINDOW_RESIZABLE);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

  if (!window || !renderer)
  {
    rle_sdl_error("Window/Renderer creation failed: %s", SDL_GetError());
    SDL_Quit();
    fclose(file);
    if (io_buf)
      free(io_buf);
    return EXIT_FAILURE;
  }

  SDL_Texture *texture = SDL_CreateTexture(
      renderer,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STREAMING,
      header.org_width,
      header.org_height);

  if (!texture)
  {
    rle_sdl_error("Texture creation failed: %s", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    fclose(file);
    if (io_buf)
      free(io_buf);
    return EXIT_FAILURE;
  }

  if (!decode_rle_to_texture(file, texture, &header))
  {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Decoding completed with errors or EOF.");
  }

  bool running = true;
  SDL_Event event;

  while (running)
  {
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_EVENT_QUIT ||
          (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE))
      {
        running = false;
      }
    }

    int win_w = 0;
    int win_h = 0;
    SDL_GetWindowSize(window, &win_w, &win_h);

    float scale_x = (float)win_w / (float)header.org_width;
    float scale_y = (float)win_h / (float)header.org_height;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;

    /* Keep smaller images at native resolution; only downscale when needed. */
    if (scale > 1.0f)
    {
      scale = 1.0f;
    }

    float draw_w = (float)header.org_width * scale;
    float draw_h = (float)header.org_height * scale;

    SDL_FRect dst = {
        .x = ((float)win_w - draw_w) * 0.5f,
        .y = ((float)win_h - draw_h) * 0.5f,
        .w = draw_w,
        .h = draw_h};

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, &dst);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  fclose(file);
  if (io_buf)
    free(io_buf);

  return EXIT_SUCCESS;
}