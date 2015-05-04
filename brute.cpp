#include <atomic>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include <cstdio>
#include <cstdlib>

#include "timing.h"

// customization
#include "config.h"
using namespace std;

void print_hex(unsigned char *data, size_t len) {
  for (int i = 0; i < len; ++i)
    printf("%02x", data[i]);
}

void print_repr_string(string s) {
  cout << (char)'"';
  for (char c: s) {
    if (c >= 0x20 && c <= 0x7e)
      cout << c;
    else
      printf("\\x%02x", (int)(unsigned char)c);
  }
  cout << (char)'"';
}

// options
string pattern;
vector<string> alphabets;
int num_threads = 1;
bool verbose = true;
bool compute_all = false;

vector<int> wildcard_positions;

template <typename T>
void enumerate(int idx, string& pattern, T cb) {
  if (idx == wildcard_positions.size()) {
    cb(pattern);
    return;
  }
  int p = wildcard_positions[idx];
  string alphabet = idx < alphabets.size() ? alphabets[idx] : alphabets.back();
  int alph_sz = alphabet.size();
  if (idx == 0) {
    vector<thread> threads;
    int first = 0;
    for (int t = 0; t < num_threads; ++t) {
      int last = first + alph_sz / num_threads + (t < alph_sz % num_threads) - 1;
      if (verbose) {
        cout << "  Thread " << t << " responsible for alphabet range "
            << first << ".." << last << endl;
      }
      threads.emplace_back([=]() {
        string local_pattern = pattern;
        for (int i = first; i <= last; ++i) {
          local_pattern[p] = alphabet[i];
          enumerate(idx + 1, local_pattern, cb);
        }
      });
      first = last + 1;
    }
    for (auto& t : threads)
      t.join();
  } else {
    for (int i = 0; i < alph_sz; ++i) {
      pattern[p] = alphabet[i];
      enumerate(idx + 1, pattern, cb);
    }
  }
}

void usage(char *argv0) {
  cerr << "Usage: " << argv0 << " [-t num_threads] [-s] [-a] pattern_string "
       << "alphabet [alphabet [...]]" << endl
       << endl
       << "pattern_string" << endl
       << "  is a string with ? chars as placeholders" << endl
       << endl
       << "alphabet" << endl
       << "  is an alphabet specification in one of two formats:" << endl
       << "    =s     represents the explicit set of characters in s" << endl
       << "    :lo:hi represents the explicit set of ascii characters with" << endl
       << "           ASCII values in the inclusive range lo..hi" << endl
       << endl
       << "OPTIONS" << endl
       << "  -t integer  Use the given number of threads" << endl
       << "  -s          No verbose output, just dump the result string" << endl
       << "  -a          Find all matching strings" << endl
       << "  -h          Show this help" << endl
       << endl
       << "EXAMPLES" << endl
       << "  " << argv0 << " -t 4 \"' UNION SELECT 1,1 -- ??????????\" '=0123456789'" << endl;
  exit(EXIT_FAILURE);
}

void parse_opts(int argc, char **argv) {
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
    if (string(argv[i]) == "-a") {
      compute_all = true;
      continue;
    }
    if (string(argv[i]) == "-s") {
      verbose = false;
      continue;
    }
    if (string(argv[i]) == "-s") {
      usage(argv[0]);
    }
    if (pos == 0) pattern = argv[i];
    if (pos >= 1) {
      if (argv[i][0] == '=') {
        alphabets.emplace_back(argv[i]+1);
      } else if (argv[i][0] == ':') {
        int lo, hi;
        if (sscanf(argv[i], ":%d:%d", &lo, &hi) != 2) {
          cerr << "Invalid alphabet specification: " << argv[i] << endl;
          usage(argv[0]);
        }
        string alph;
        for (int i = lo; i <= hi; ++i)
          alph += (char)i;
        alphabets.emplace_back(alph);
      } else {
        cerr << "Invalid alphabet specification: " << argv[i] << endl;
        usage(argv[0]);
      }
    }
    pos++;
  }
  if (pos < 2) {
    usage(argv[0]);
  }
}

int main(int argc, char **argv) {
  parse_opts(argc, argv);
  for (size_t i = 0; i < pattern.size(); ++i) {
    if (pattern[i] == '?')
      wildcard_positions.push_back(i);
  }
  if (verbose) {
    cout << "INFO" << endl;
    cout << "  Threads: " << num_threads << endl;
    cout << "  Pattern: " << pattern << endl;
    for (size_t i = 0; i < alphabets.size(); ++i) {
      if (i < alphabets.size() - 1 || alphabets.size() == wildcard_positions.size())
        cout << "  Alphabet for position " << wildcard_positions[i];
      else
        cout << "  Alphabet for positions "
            << wildcard_positions[i] << ".." << wildcard_positions.back();
      cout << ": ";
      print_repr_string(alphabets[i]);
      cout << endl;
    }
  }
  size_t total = 1;
  for (size_t i = 0; i < wildcard_positions.size(); ++i) {
    total *= (i < alphabets.size()) ? alphabets[i].size() : alphabets.back().size();
  }
  atomic<long long> current(0);
  double start_time = util::get_time();
  mutex mx;
  enumerate(0, pattern, [&](const string& s) {
    if (verbose)
      current++;
    if (verbose && current % 1000000 == 0) {
      lock_guard<mutex> lg(mx);
      cout << "\rPROGRESS " << current << " / " << total
           << fixed << setprecision(2)
           << " (" << (100.*current/total) << "%, "
           << (current/(util::get_time() - start_time)/1e6) << "mh/s)       " << flush;
    }
    unsigned char hash[hash_size];
    compute_hash((const unsigned char*)s.c_str(), s.size(), hash);
    if (check(hash)) {
      lock_guard<mutex> lg(mx);
      if (verbose) {
        cout << endl << "MATCH" << endl;
        cout << "  String: ";
      }
      print_repr_string(s);
      cout << endl;
      if (verbose) {
        cout << "  Hash: ";
        print_hex(hash, 16);
        cout << endl;
      }
      if (!compute_all)
        exit(0);
    }
  });
}
