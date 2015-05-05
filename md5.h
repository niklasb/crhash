#ifndef _MD5_H
#define _MD5_H

#include <stdint.h>

void md5_compress(uint32_t state[4], const uint32_t block[16]);
void md5_hash(const uint8_t *message, uint32_t len, uint32_t hash[4]);

#endif
