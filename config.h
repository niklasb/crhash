#include <random>
#include "md5.h"

#define HAVE_OPENCL 1

const size_t hash_size = 16; // in bytes

const std::string cl_filename = "md5.cl";
const std::string cl_kernel_name = "GenerateAndCheck";
const size_t cl_chunk_size = 1<<24;

/*
void compute_hash(const unsigned char *data, size_t data_sz, unsigned char *res) {
  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, data, data_sz);
  MD5_Final(res, &ctx);
}
*/

void compute_hash(const unsigned char *data, size_t data_sz, unsigned char *res) {
  md5_hash(data, data_sz, (uint32_t*)res);
}

bool check(unsigned char *hash) {
  if (hash[0] != 0x0e)
    return 0;
  for (int i = 1; i < 16; ++i)
    if ((hash[i] & 0xf) > 9 || (hash[i]>>4) > 9)
      return 0;
  return 1;
}
