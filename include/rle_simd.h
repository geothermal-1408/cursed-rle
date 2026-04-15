#ifndef RLE_SIMD_H
#define RLE_SIMD_H


#include <stdint.h>
#include <string.h>


#ifndef RLE_HAVE_NEON
    #if defined (__ARM_NEON) || defined (__ARM_NEON__)
        #define RLE_HAVE_NEON 1
    #else
	#define RLE_HAVE_NEON 0
    #endif
#endif

#if RLE_HAVE_NEON
    #include <arm_neon.h>
#endif /* RLE_HAVE_NEON */

int rle_count_run_simd(const unsigned char *src,
		       int remaining,
		       int max_count,
		       int channels);

void rle_fill_pixel_simd(unsigned char *dst,
			 const unsigned char *pixel,
			 int count,
			 int channels);
  
#endif /* RLE_SIMD_H */
  
