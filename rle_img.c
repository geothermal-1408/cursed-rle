#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "rle_img.h"
#include "rle_format.h"

int _file_exists(const char *path)
{
  struct stat st;
  return (stat(path, &st) == 0);
}

int _file_is_writable(const char *path)
{
  FILE *f = fopen(path, "rb");
  if(!f) return 0;
  fclose(f);
  return 1;
}

int _is_valid_bmp(const char *path)
{
  FILE *f = fopen(path, "rb");
  if(!f) return 0;
  unsigned char magic[2];
  fread(magic, 1, 2, f);
  fclose(f);

  return (magic[0] == 'B' && magic[1] == 'M');
}

int rle_write_file(const char *output,
		   int width,int height, int channels,
		   const unsigned char *enc_data,
		   int enc_len)
{
  FILE *f = fopen(output, "wb");
  
  if (!f) return -1;

  RLEheader hdr;
  hdr.magic[0] = RLE_MAGIC0;
  hdr.magic[1] = RLE_MAGIC1;
  hdr.magic[2] = RLE_MAGIC2;
  hdr.version  = RLE_VERSION;
  hdr.org_width = (uint32_t)width;
  hdr.org_height = (uint32_t)height;
  hdr.channels = (uint8_t)channels;
  hdr.org_filesize = (uint32_t)(width * height * channels);

  fwrite(&hdr, sizeof(RLEheader), 1, f);
  fwrite(enc_data, 1, enc_len, f);

  fclose(f);
  return 0;
}

int rle_read_file(const char *input,
		  RLEheader *hdr_out,
		  unsigned char **data_out,
		  int *data_len_out)
{
  FILE *f = fopen(input, "rb");
  if (!f) return -1;

  fread(hdr_out, sizeof(RLEheader), 1, f);

  if(hdr_out->magic[0] != RLE_MAGIC0 ||
     hdr_out->magic[1] != RLE_MAGIC1 ||
     hdr_out->magic[2] != RLE_MAGIC2 ) {
    printf("ERROR: not a vaild .rle file\n");
    fclose(f);
    return -2;
  }

  if(hdr_out->version != RLE_VERSION) {
    printf("ERROR: unsupported version\n");
    fclose(f);
    return -3;
  }
  
  fseek(f, 0, SEEK_END);
  long file_size = ftell(f);
  long data_size = file_size - sizeof(RLEheader);
  fseek(f, sizeof(RLEheader), SEEK_SET);

  *data_out = malloc(data_size);
  if(!*data_out) {
    fclose(f);
    return -4;
  }
  
  fread(*data_out, 1, data_size, f);
  *data_len_out = (int)data_size;

  fclose(f);
  return 0;
}

void rle_make_filename(const char *input, char *output, int out_size)
{
    snprintf(output, out_size, "%s.rle", input);
}

int rle_encode_binary(const unsigned char *input,
		      int input_len,
		      unsigned char *output,
		      int output_max,
		      int channels)
{
  int i = 0;
  int offset = 0;
  int pixel_count = input_len / channels;
  
  while(i < pixel_count) {
    //unsigned char cur = input[i];
    int count = 1;

    while(i + count < pixel_count &&
	  count < 255 &&
	  memcmp(&input[i * channels],
		 &input[(i + count) * channels],
		 channels) == 0) {
      count++;
    }
    if(offset + 1 + channels > output_max) return -1;

    output[offset++] = (unsigned char)count;
    memcpy(&output[offset], &input[i * channels], channels);
    offset += channels;
    i+= count;
  }
  return offset;
}

int rle_decode_binary(const unsigned char *input,
		      int input_len,
		      unsigned char *output,
		      int output_max,
		      int channels)
{
  int i = 0;
  int offset = 0;

  while(i + 1 < input_len) {
    int count = input[i++];
    
    //unsigned char byte = input[i + 1];

    if(i + channels > input_len) return -1;

    if(offset + count * channels > output_max) return -1;

    for(int j = 0; j < count; ++j) {
      memcpy(&output[offset], &input[i], channels);
      offset += channels;
    }
    i += channels;
  }
  return offset;
}
