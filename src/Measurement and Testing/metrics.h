#pragma once
#include <time.h>

typedef struct {
    double elapsed_sec;
    long max_rss_kb;
} metrics_t;

/* =============================
   Platform-independent timer
   ============================= */

static inline double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* =============================
   Peak memory (platform-dependent)
   ============================= */

#if defined(__linux__) || defined(__APPLE__)
  #include <sys/resource.h>

  static inline long get_max_rss_kb(void) {
      struct rusage usage;
      getrusage(RUSAGE_SELF, &usage);
  #if defined(__APPLE__)
      return usage.ru_maxrss / 1024; // bytes → KB
  #else
      return usage.ru_maxrss;        // already KB
  #endif
  }

#else
  /* Windows / MSYS / UCRT fallback */
  static inline long get_max_rss_kb(void) {
      return 0;  // Not supported on this platform
  }
#endif

static inline void metrics_begin(double *t0) {
    *t0 = now_sec();
}

static inline void metrics_end(metrics_t *m, double t0) {
    m->elapsed_sec = now_sec() - t0;
    m->max_rss_kb  = get_max_rss_kb();
}
