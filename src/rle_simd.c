#include "rle_simd.h"

static int scalar_count_run(const unsigned char *src,
			    int remaining,
			    int max_count,
			    int channels)
{
  int limit = (remaining < max_count)? remaining : max_count;
  int count = 1;
  while(count < limit &&
	memcmp(src, src + (size_t) count * (size_t)channels, (size_t)channels) == 0) {

    count++;
  }
  return count;
}

static void scalar_fill_pixel(unsigned char *dst,
			      const unsigned char *pixel,
			      int count,
			      int channels)
{
  for(int j = 0; j < count; ++j) {
    memcpy(dst + (size_t)j * (size_t)channels, pixel, (size_t)channels);
  }
}

#if RLE_HAVE_NEON

// ** channels = 4 **

static int count_run_neon_4ch(const unsigned char *src,
			      int remaining,
			      int max_count)
{
  const uint32_t *pixels = (const uint32_t *)src;
  uint32_t ref = pixels[0];
  uint32x4_t vref = vdupq_n_u32(ref);

  int limit = (remaining < max_count) ? remaining : max_count;
  int count = 1;
  int j = 1;

  while(j + 4 <= limit) {
    uint32x4_t vcand = vld1q_u32(pixels + j);
    uint32x4_t veq = vceqq_u32(vcand, vref);

    if(vminvq_u32(veq) == 0xFFFFFFFFU) {
      count += 4;
      j += 4;
    } else {
      uint32_t lanes[4];
      vst1q_u32(lanes, veq);
      for(int k = 0; k < 4; ++k, ++j) {
	if(lanes[k] != 0xFFFFFFFFU) return count;
	count++;
      }
      return count;
    }
  }

  while (j < limit && pixels[j] == ref) {
    count++;
    j++;
  }
  return count;
}

static void fill_run_neon_4ch(unsigned char *dst,
			      const unsigned char *pixel,
			      int count)
{
  uint32_t ref		= *(const uint32_t *)pixel;
  uint32x4_t vpixel	= vdupq_n_u32(ref);
  uint32_t *out		= (uint32_t*)dst;
  int j			= 0;

  while(j + 4 <= count) {
    vst1q_u32(out + j, vpixel);
    j += 4;
  }
  while(j < count) {
    out[j++] = ref;
  }
}

// ** channels = 1 //

static int count_run_neon_1ch(const unsigned char *src,
			      int remaining,
			      int max_count)
{
  uint32_t ref = src[0];
  uint32x4_t vref = vdupq_n_u8(ref);

  int limit = (remaining < max_count) ? remaining : max_count;
  int count = 1;
  int j = 1;

  while(j + 16 <= limit) {
    uint8x16_t vcand = vld1q_u8(src + j);
    uint8x16_t veq = vceqq_u8(vcand, vref);

    if(vminvq_u8(veq) == 0xFFU) {
      count += 16;
      j += 16;
    } else {
      uint8_t lanes[16];
      vst1q_u8(lanes, veq);
      for(int k = 0; k < 16; ++k, ++j) {
	if(lanes[k] != 0xFFU) return count;
	count++;
      }
      return count;
    }
  }

  if (j + 8 < limit) {
    uint8x8_t vcand8 = vld1_u8(src + j);
    uint8x8_t vref8 = vget_low_u8(vref);
    uint8x8_t veq8 = vceq_u8(vcand8, vref8);

    uint64_t res64 = vget_lane_u64(vreinterpret_u64_u8(veq8), 0);
    if(res64 == 0xFFFFFFFFFFFFFFFFULL) {
      count += 8;
      j += 8;
    } else {
      uint8_t lanes[8];
      vst1_u8(lanes, veq8);
      for(int k = 0; k < 8; ++k, ++j) {
	if(lanes[k] != 0xFFU) return count;
	count++;
      }
      return count;
    }
  }

  while(j < limit && src[j] == ref) {
    count++;
    j++;
  }
  return count;
}

static void fill_run_neon_1ch(unsigned char *dst,
			      const unsigned char *pixel,
			      int count)
{
  uint8x16_t vpixel	= vdupq_n_u8(pixel[0]);
  int j			= 0;

  while(j + 16 <= count) {
    vst1q_u8(dst + j, vpixel);
    j += 16;
  }
  while(j < count) {
    dst[j++] = pixel[0];
  }
}

#endif /* RLE_HAVE_NEON */

int rle_count_run_simd(const unsigned char *src,
		       int remaining,
		       int max_count,
		       int channels)
{
#if RLE_HAVE_NEON
  
  if(channels == 4) return count_run_neon_4ch(src, remaining, max_count);
  if(channels == 1) return count_run_neon_1ch(src, remaining, max_count);
  
#endif /* RLE_HAVE_NEON */

  return scalar_count_run(src, remaining, max_count, channels);
}

void rle_fill_pixel_simd(unsigned char *dst,
			  const unsigned char *pixel,
			  int count,
			  int channels)
{
#if RLE_HAVE_NEON
  if(channels == 4) {
    fill_run_neon_4ch(dst, pixel, count);
    return;
  }
  if(channels == 1) {
    fill_run_neon_1ch(dst, pixel, count);
    return;
  }
  
#endif /* RLE_HAVE_NEON */
  scalar_fill_pixel(dst, pixel, count, channels);
  
}
