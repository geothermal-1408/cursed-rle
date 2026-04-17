#ifndef RLE_FORMAT_H
#define RLE_FORMAT_H

#include <stdint.h>

#define RLE_MAGIC0 'R'
#define RLE_MAGIC1 'L'
#define RLE_MAGIC2 'E'
#define RLE_VERSION 1

#pragma pack(1) // no need padding
typedef struct
{
  uint8_t magic[3];
  uint8_t version;
  uint32_t org_height;
  uint32_t org_width;
  uint8_t channels;
  uint32_t org_filesize;

} RLEheader;
#pragma pack()

#endif
