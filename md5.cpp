#include <cstdint>
#include <cstring>
#include "md5.h"

void md5_hash(const uint8_t *message, uint32_t len, uint32_t hash[4]) {
  hash[0] = UINT32_C(0x67452301);
  hash[1] = UINT32_C(0xEFCDAB89);
  hash[2] = UINT32_C(0x98BADCFE);
  hash[3] = UINT32_C(0x10325476);

  uint32_t i;
  for (i = 0; len - i >= 64; i += 64)
    md5_compress(hash, (uint32_t *)(message + i));  // Type-punning

  uint32_t block[16];
  uint8_t *byteBlock = (uint8_t *)block;  // Type-punning

  uint32_t rem = len - i;
  memcpy(byteBlock, message + i, rem);

  byteBlock[rem] = 0x80;
  rem++;
  if (64 - rem >= 8)
    memset(byteBlock + rem, 0, 56 - rem);
  else {
    memset(byteBlock + rem, 0, 64 - rem);
    md5_compress(hash, block);
    memset(block, 0, 56);
  }
  block[14] = len << 3;
  block[15] = len >> 29;
  md5_compress(hash, block);
}
