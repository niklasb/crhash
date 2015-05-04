#ifndef _TIMING_H
#define _TIMING_H

#include <sys/time.h>

namespace util {

double get_time() {
  struct timezone tzp;
  timeval now;
  gettimeofday(&now, &tzp);
  return now.tv_sec + now.tv_usec / 1000000.;
}

}

#endif
