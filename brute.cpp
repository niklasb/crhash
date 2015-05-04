#include <atomic>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include <cstdio>

#include "timing.h"

// customization
#include "config.h"
using namespace std;

void print_hex(unsigned char *data, size_t len) {
  for (int i = 0; i < len; ++i)
    printf("%02x", data[i]);
}

string pattern;
vector<int> wildcard_positions;
int lo, hi;
int num_threads;

template <typename T>
void enumerate(int i, string& pattern, T cb) {
  if (i == wildcard_positions.size()) {
    cb(pattern);
    return;
  }
  int p = wildcard_positions[i];
  if (i == 0) {
    vector<thread> threads;
    int range = hi - lo + 1;
    int first = lo;
    for (int t = 0; t < num_threads; ++t) {
      int last = first + range / num_threads + (t < range % num_threads) - 1;
      cout << "Thread " << t << " responsible for range "
           << first << ".." << last << endl;
      threads.emplace_back([=]() {
        string local_pattern = pattern;
        for (local_pattern[p] = first; ; ++local_pattern[p]) {
          enumerate(i+1, local_pattern, cb);
          if (local_pattern[p] == last) break;
        }
      });
      first = last + 1;
    }
    for (auto& t : threads)
      t.join();
  } else {
    for (pattern[p] = lo; ; ++pattern[p]) {
      enumerate(i+1, pattern, cb);
      if (pattern[p] == hi) break;
    }
  }
}

void usage(char *argv0) {
  cerr << "Usage: " << argv0 << " [-t num_threads] pattern_string ascii_lo ascii_hi" << endl;
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  if (argc < 4)
    usage(argv[0]);
  num_threads = 1;
  int pos = 0;
  for (int i = 1; i < argc; ++i) {
    if (string(argv[i]) == "-t") {
      if (i + 1 < argc)
        num_threads = atoi(argv[i+1]);
      else
        usage(argv[0]);
      i++;
      continue;
    }
    if (pos == 0) pattern = argv[i];
    if (pos == 1) lo = atoi(argv[i]);
    if (pos == 2) hi = atoi(argv[i]);
    pos++;
  }
  cout << "Threads: " << num_threads << endl;
  cout << "Pattern: " << pattern << endl;
  cout << "Range: " << lo << ".." << hi << endl;
  long long total = 1;
  for (size_t i = 0; i < pattern.size(); ++i) {
    if (pattern[i] == '?') {
      wildcard_positions.push_back(i);
      total *= (hi - lo + 1);
    }
  }
  unsigned char hash[hash_size];
  atomic<long long> current(0);
  double start_time = util::get_time();
  mutex mx;
  enumerate(0, pattern, [&](const string& s) {
    current++;
    if (current % 1000000 == 0) {
      lock_guard<mutex> lg(mx);
      cout << "\rProgress: " << current << " / " << total
           << fixed << setprecision(2)
           << " (" << (100.*current/total) << "%, "
           << (current/(util::get_time() - start_time)/1e6) << "mh/s)       " << flush;
    }
    compute_hash((const unsigned char*)s.c_str(), s.size(), hash);
    if (check(hash)) {
      lock_guard<mutex> lg(mx);
      cout << endl;
      cout << "String: " << s << endl << "Hash: ";
      print_hex(hash, 16);
      cout << endl;
      exit(0);
    }
  });
  cout << endl;
}
