#include <stdint.h>
#include "crc32_nibble.h"

static uint8_t mirror8(uint8_t byte);
static uint32_t mirror32(uint32_t hw);

static uint8_t nibb[] =
{
  0, // 0  = 0000 = 0000 = 0
  8, // 1  = 0001 = 1000 = 8
  4, // 2  = 0010 = 0100 = 4
 12, // 3  = 0011 = 1100 = C
  2, // 4  = 0100 = 0010 = 2
 10, // 5  = 0101 = 1010 = A
  6, // 6  = 0110 = 0110 = 6
 14, // 7  = 0111 = 1110 = E
  1, // 8  = 1000 = 0001 = 1
  9, // 9  = 1001 = 1001 = 9
  5, // 10 = 1010 = 0101 = 5
 13, // 11 = 1011 = 1101 = D
  3, // 12 = 1100 = 0011 = 3
 11, // 13 = 1101 = 1011 = E
  7, // 14 = 1110 = 0111 = 7
 15  // 15 = 1111 = 1111 = F
};

static const uint32_t crc32_t[16] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
};

/* A nibble based crc32 computation routine */
uint32_t crc32(uint32_t sum, char const * p, uint32_t len)
{
    while (len--) {
        uint8_t byte = mirror8(*p++);
        // hi nibble
        sum = crc32_t[(sum>>28)^(byte >> 4)]^(sum<<4);
        // lo nibble
        sum = crc32_t[(sum>>28)^(byte & 0xF)]^(sum<<4);
    }

    return ~mirror32(sum);
}

static uint8_t mirror8(uint8_t byte)
{
  return (nibb[byte & 0xF] << 4) | (nibb[byte >> 4]);
}

static uint32_t mirror32(uint32_t hw)
{
  uint8_t b1 = mirror8(hw >> 24);
  uint8_t b2 = mirror8((hw >> 16) & 0xFF);
  uint8_t b3 = mirror8((hw >> 8) & 0xFF);
  uint8_t b4 = mirror8(hw & 0xFF);
  return (b4 << 24) | (b3 << 16) | (b2 << 8) | b1;
}


