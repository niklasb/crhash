#include <iostream>
#include <vector>
#include <cstring>

#include "md5.h"
using namespace std;

void md5(const unsigned char *data, size_t data_sz, unsigned char res[16]) {
  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, data, data_sz);
  MD5_Final(res, &ctx);
}

string pattern;
vector<int> wildcard_positions;
int lo, hi;
template <typename T>
void enumerate(int i, T cb) {
  if (i == wildcard_positions.size()) {
    cb(pattern);
    return;
  }
  int p = wildcard_positions[i];
  for (pattern[p] = lo; ; ++pattern[p]) {
    enumerate(i+1, cb);
    if (pattern[p] == hi) break;
  }
}

bool check(unsigned char hash[16]) {
  if (hash[0] != 0x0e)
    return 0;
  for (int i = 1; i < 16; ++i)
    if ((hash[i] & ((1<<4)-1)) > 9 || (hash[i]>>4) > 9)
      return 0;
  return 1;
}

void print_hex(unsigned char *data, size_t len) {
  for (int i = 0; i < len; ++i)
    printf("%02x", data[i]);
}

int main(int argc, char **argv) {
  if (argc != 4) {
    cerr << "Usage: pattern_string ascii_lo ascii_hi" << endl;
    return EXIT_FAILURE;
  }
  pattern = argv[1];
  lo = atoi(argv[2]);
  hi = atoi(argv[3]);
  long long total = 1;
  for (size_t i = 0; i < pattern.size(); ++i) {
    if (pattern[i] == '?') {
      wildcard_positions.push_back(i);
      total *= (hi - lo + 1);
    }
  }
  unsigned char hash[16];
  long long current = 0;
  enumerate(0, [&](const string& s) {
    current++;
    if (current % 1000000 == 0) {
      cout << "Progress: " << current << " / " << total << " (" << (100.*current/total) << "%)" << endl;
    }
    //cout << s << endl;
    md5((const unsigned char*)s.c_str(), s.size(), hash);
    if (check(hash)) {
      cout << s << endl;
      print_hex(hash, 16);
      exit(0);
    }
  });
}
