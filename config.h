#include "md5.c"

const size_t hash_size = 16;

void compute_hash(const unsigned char *data, size_t data_sz, unsigned char *res) {
  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, data, data_sz);
  MD5_Final(res, &ctx);
}

bool check(unsigned char *hash) {
  if (hash[0] != 0x0e)
    return 0;
  for (int i = 1; i < 16; ++i)
    if ((hash[i] & ((1<<4)-1)) > 9 || (hash[i]>>4) > 9)
      return 0;
  return 1;
}
