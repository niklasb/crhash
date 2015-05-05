#include <atomic>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include <cstdio>
#include <cstdlib>

#include "io.h"
#include "timing.h"
#include "opencl.h"

// customization
#include "config.h"
using namespace std;

void print_hex(unsigned char *data, size_t len) {
  for (int i = 0; i < len; ++i)
    printf("%02x", data[i]);
}

template <typename I>
void print_repr_string(I a, I b) {
  cout << (char)'"';
  while (a != b) {
    char c = *a++;
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
bool use_opencl = false;

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

template <typename T, typename U>
void run_cpu(T cb_progress, U cb_match) {
  atomic<long long> current(0);
  enumerate(0, pattern, [&](const string& s) {
    if (verbose) {
      long long cur = ++current;
      if (cur % 2000000 == 0) {
        cb_progress(current);
      }
    }
    unsigned char hash[hash_size];
    compute_hash((const unsigned char*)s.c_str(), s.size(), hash);
    if (check(hash)) {
      cb_match(s);
    }
  });
}

class CLBruteForceApp {
  OpenCLApp app;
  cl::Kernel kernel;
  string prefix, suffix;
  int lo, hi;
  size_t pattern_size;
  size_t chunk_size;
  cl::Buffer buf_prefix, buf_suffix, buf_results;
  cl::Buffer buf_debug;

  string generate(size_t id) {
    string s = prefix;
    size_t num = id;
    size_t base = hi - lo + 1;
    for (int i = 0; i < pattern_size; ++i) {
      s += lo + num % base;
      num /= base;
    }
    s += suffix;
    return s;
  }

  void prepare() {
    buf_prefix = app.alloc<cl_uint>((prefix.size() + 3) / 4, CL_MEM_READ_ONLY);
    buf_suffix = app.alloc<cl_uint>((suffix.size() + 3) / 4, CL_MEM_READ_ONLY);
    buf_results = app.alloc<cl_uchar>(chunk_size, CL_MEM_WRITE_ONLY);
    //buf_debug = app.alloc<cl_uint>(16 * chunk_size, CL_MEM_WRITE_ONLY);
    app.write_async(buf_prefix, prefix.c_str(), prefix.size());
    app.write_async(buf_suffix, suffix.c_str(), suffix.size());
    kernel.setArg(0, buf_prefix);
    kernel.setArg(1, (cl_uint)prefix.size());
    kernel.setArg(2, buf_suffix);
    kernel.setArg(3, (cl_uint)suffix.size());
    kernel.setArg(4, (cl_uint)lo);
    kernel.setArg(5, (cl_uint)hi);
    kernel.setArg(6, (cl_uint)pattern_size);
    kernel.setArg(8, buf_results);
    //kernel.setArg(9, buf_debug);
  }

  template <typename t>
  void run_group(size_t offset, t cb) {
    kernel.setArg(7, (cl_uint)offset);
    assert(chunk_size % 256 == 0);
    app.run_kernel(kernel, cl::NDRange(chunk_size), cl::NDRange(256));
    // TODO use vector
    cl_uchar *results = new cl_uchar[chunk_size];
    //char *debug = new char[64 * chunk_size];
    //app.read_sync(buf_debug, debug, chunk_size * 16);
    app.read_sync(buf_results, results, chunk_size);
    for (int i = 0; i < chunk_size; ++i) {
      if (results[i]) {
        string s = generate(offset + i);
        cb(s);
      }
    }
    delete[] results;
  }

public:
  CLBruteForceApp(
      string kernel_source,
      string kernel_name,
      string prefix, string suffix,
      int lo, int hi,
      size_t pattern_size,
      size_t chunk_size)
    : app(), prefix(prefix), suffix(suffix), lo(lo), hi(hi)
    , pattern_size(pattern_size), chunk_size(chunk_size)
  {
    cl::Program prog = app.build_program(kernel_source);
    kernel = app.get_kernel(prog, kernel_name);
    if (prefix.size() + suffix.size() + pattern_size > 55) {
      cerr << "Only one-block messages supported with OpenCL, so the resulting strings "
        << "must be <= 55 bytes long" << endl;
      exit(1);
    }
    prepare();
  }

  template <typename T, typename U>
  void run(T cb_progress, U cb_match) {
    size_t total = 1, base = hi - lo + 1;
    for (int i = 0; i < pattern_size; ++i) {
      size_t new_total = total * base;
      if (new_total / base != total) {
        cerr << "Overflow when computing total number of possibilities, use less "
                "wildcards or smaller alphabets!" << endl;
        exit(1);
      }
      total = new_total;
    }
    for (size_t offset = 0; offset < total; offset += chunk_size) {
      cb_progress(offset);
      run_group(offset, cb_match);
    }
  }

  void print_cl_info() {
    app.print_cl_info();
  }
};

template <typename I>
bool is_contiguous(I a, I b) {
  for (I last = a++; a != b; last = a++)
    if (*last + 1 != *a)
      return false;
  return true;
}

template <typename T, typename U>
void run_gpu(T cb_progress, U cb_match) {
  if (!is_contiguous(begin(wildcard_positions), end(wildcard_positions))) {
    cerr << "Pattern not yet supported with OpenCL. Wildcards need to be contiguous" << endl;
    exit(1);
  }
  if (alphabets.size() > 1) {
    cerr << "Multiple alphabets not yet supported with OpenCL" << endl;
    exit(1);
  }
  for (int i = 1; i < alphabets[0].size(); ++i) {
    if ((unsigned char)alphabets[0][i-1] + 1 != (unsigned char)alphabets[0][i]) {
      cerr << "Alphabet not yet supported with OpenCL. Characters need to be contiguous" << endl;
      cerr << "Non-contiguous characters detected: " << alphabets[0][i-1] << " -> " << alphabets[0][i] << endl;
      exit(1);
    }
  }
  string prefix = pattern.substr(0, wildcard_positions[0]);
  string suffix = pattern.substr(wildcard_positions.back() + 1);
  size_t pattern_len = wildcard_positions.back() - wildcard_positions[0] + 1;
  int lo = (unsigned char)alphabets[0][0], hi = (unsigned char)alphabets[0].back();
  CLBruteForceApp app(
      cl_kernel_source, cl_kernel_name,
      prefix, suffix,
      lo, hi,
      pattern_len,
      cl_chunk_size
  );
  if (verbose)
    app.print_cl_info();
  app.run(cb_progress, cb_match);
}

template <typename T, typename U>
void run(T cb_progress, U cb_match) {
  if (use_opencl)
    run_gpu(cb_progress, cb_match);
  else
    run_cpu(cb_progress, cb_match);
}

void usage(char *argv0) {
  cerr << "Usage: " << argv0 << " [-t num_threads] [-s] [-a] pattern_string "
       << "alphabet0 [alphabet1 [...]]" << endl
       << endl
       << "pattern_string" << endl
       << "  is a string with ? chars as placeholders" << endl
       << endl
       << "alphabetX" << endl
       << "  is an alphabet specification in one of two formats:" << endl
       << "    =s     represents the explicit set of characters in s" << endl
       << "    :lo:hi represents the explicit set of ascii characters with" << endl
       << "           ASCII values in the inclusive range lo..hi" << endl
       << endl
       << "  You can provide different charsets for the different wildcard positions" << endl
       << "  in the pattern string. The last character set you provide will be used" << endl
       << "  for the remaining positions." << endl
       << endl
       << "OPTIONS" << endl
       << "  -t integer  Use the given number of threads" << endl
       << "  -s          No verbose output, just dump the result string" << endl
       << "  -a          Find all matching strings" << endl;
  if (HAVE_OPENCL)
    cerr << "  -c          Use OpenCL. Currently only supports a subset of patterns," << endl
         << "              specifically ones where the wildcards are all contiguous and" << endl
         << "              there is only one contiguous charset. I.e. <prefix>\?\?...\?\?<suffix>" << endl;
  cerr << "  -h          Show this help" << endl
       << endl
       << "EXAMPLES" << endl
       << "  " << argv0 << " -t 4 \"My name is \?\?\?\" :97:122" << endl;
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
    if (HAVE_OPENCL && string(argv[i]) == "-c") {
      use_opencl = true;
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
  if (num_threads > 1 && use_opencl) {
    cerr << "Can't use both multithreading and OpenCL" << endl;
    exit(1);
  }
}

int main(int argc, char **argv) {
  parse_opts(argc, argv);
  for (size_t i = 0; i < pattern.size(); ++i) {
    if (pattern[i] == '?')
      wildcard_positions.push_back(i);
  }
  if (wildcard_positions.empty()) {
    cerr << "No wildcards, not sure what you want to achieve..." << endl;
    exit(1);
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
      string alph = alphabets[i];
      print_repr_string(begin(alph), end(alph));
      cout << endl;
    }
  }
  size_t total = 1;
  for (size_t i = 0; i < wildcard_positions.size(); ++i) {
    size_t base = (i < alphabets.size()) ? alphabets[i].size() : alphabets.back().size();
    size_t new_total = total * base;
    if (new_total / base != total) {
      cerr << "Overflow when computing total number of possibilities, use less "
              "wildcards or smaller alphabets!" << endl;
      exit(1);
    }
    total = new_total;
  }
  double start_time = util::get_time();
  mutex mx;
  cout << fixed << setprecision(2);
  run(
    [&](size_t current) {
      lock_guard<mutex> lg(mx);
      double time = util::get_time() - start_time;
      cout << "\rPROGRESS " << current << " / " << total
          << " (" << (100.*current/total) << "%, "
          << time << " sec, "
          << (current/time/1e6) << "mh/s)       " << flush;
    },
    [&](const string& match) {
      lock_guard<mutex> lg(mx);
      unsigned char hash[hash_size];
      compute_hash((const unsigned char*)match.c_str(), match.size(), hash);
      if (verbose) {
        cout << endl << "MATCH" << endl;
        cout << "  Time: " << (util::get_time() - start_time) << " sec" << endl;
        cout << "  String: ";
      }
      print_repr_string(begin(match), end(match));
      cout << endl;
      if (verbose) {
        cout << "  Hash: ";
        print_hex(hash, 16);
        cout << endl;
      }
      if (!compute_all)
        exit(0);
    }
  );
}
