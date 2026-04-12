#ifndef RLE_IMG_H
#define RLE_IMG_H

#include "rle_format.h"

/*
  function name starts with '_' are basically private functions which are meant to use only for internal purpose. and not need to use as public API.
*/

void rle_make_filename(const char *input, char *output, int out_size);

int rle_write_file(const char *output,
		   int width, int height, int channels,
		   const unsigned char *enc_data,  int enc_len);

int rle_read_file(const char *input,
		  RLEheader *hdr_out,
		  unsigned char **data_out, int *data_len_out);

int rle_encode_binary(const unsigned char *input, int input_len, unsigned char *output, int output_max, int channels);

int rle_decode_binary(const unsigned char *input, int input_len, unsigned char *output, int output_max, int channels);

int _file_exists(const char *path);

int _file_is_writable(const char *path);

int _is_valid_bmp(const char *path);

int _open_file_picker(char *out_path, int out_size, const char *file_type);

#endif
