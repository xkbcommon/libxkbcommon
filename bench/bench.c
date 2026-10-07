/*
 * Copyright © 2015 Kazunobu Kuriyama <kazunobu.kuriyama@nifty.com>
 * Copyright © 2015 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "bench.h"
#include "../src/utils.h"

#ifndef _WIN32
#include <time.h>
#include <sys/time.h>
#else
#include <windows.h>
#include <stdint.h>

struct timeval {
    long tv_sec, tv_usec;
};

static int
gettimeofday(struct timeval *tv, void *unused)
{
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME system_time;
    FILETIME file_time;
    uint64_t t;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    t = (uint64_t) file_time.dwLowDateTime;
    t += ((uint64_t) file_time.dwHighDateTime) << 32;

    tv->tv_sec  = (long) ((t - EPOCH) / 10000000L);
    tv->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}
#endif

void
bench_start(struct bench *bench)
{
    struct timeval val;
    (void) gettimeofday(&val, NULL);
    bench->start = (struct bench_time) {
        .seconds = val.tv_sec,
        .picoseconds = val.tv_usec * 1000000LL,
    };
}

void
bench_stop(struct bench *bench)
{
    struct timeval val;
    (void) gettimeofday(&val, NULL);
    bench->stop = (struct bench_time) {
        .seconds = val.tv_sec,
        .picoseconds = val.tv_usec * 1000000LL,
    };
}

#ifndef _WIN32

static const clockid_t best_clock =
    #ifdef HAVE_CLOCK_PROCESS_CPUTIME_ID
        CLOCK_PROCESS_CPUTIME_ID
    #elif defined(HAVE_CLOCK_MONOTONIC)
        CLOCK_MONOTONIC
    #else
        CLOCK_REALTIME
    #endif
;

void
bench_start2(struct bench *bench)
{
    struct timespec t;
	(void) clock_gettime(best_clock, &t);
    bench->start = (struct bench_time) {
        .seconds = t.tv_sec,
        .picoseconds = t.tv_nsec * 1000LL,
    };
}

void
bench_stop2(struct bench *bench)
{
    struct timespec t;
	(void) clock_gettime(best_clock, &t);
    bench->stop = (struct bench_time) {
        .seconds = t.tv_sec,
        .picoseconds = t.tv_nsec * 1000LL,
    };
}
#endif

void
bench_elapsed(const struct bench *bench, struct bench_time *result)
{
    result->seconds = bench->stop.seconds - bench->start.seconds;
    result->picoseconds = bench->stop.picoseconds - bench->start.picoseconds;
    if (result->picoseconds < 0) {
        result->picoseconds += 1000000000000LL;
        result->seconds--;
    }
}

char *
bench_elapsed_str(const struct bench *bench)
{
    struct bench_time elapsed;
    char *buf;
    int ret;

    bench_elapsed(bench, &elapsed);
    ret = asprintf(&buf, "%ld.%06lld",
                   elapsed.seconds,
                   elapsed.picoseconds / 1000000);
    assert(ret >= 0);

    return buf;
}

/* Utils for bench method adapted from: https://hackage.haskell.org/package/tasty-bench */

#define fit(x1, x2) ((x1) / 5 + 2 * ((x2) / 5))

static void
predict(long long t1, long long t2, struct estimate *est)
{
    const long long t = fit(t1, t2);
    const long double d1 = (long double)(t1 - t);
    const long double d2 = (long double)(t2 - 2 * t);
    est->elapsed = t;
    est->stdev = llroundl(hypotl(d1, d2));
}

#define high(t, prec) ((t) + (prec))
#define low(t, prec) (((t) > (prec)) ? (t) - (prec) : 0)
#define MIN_PRECISION 1000000000LL /* 1ms */

void
predict_perturbed(const struct bench_time *b1, const struct bench_time *b2,
                  struct estimate *est)
{
    const long long t1 = bench_time_elapsed_picoseconds(b1);
    const long long t2 = bench_time_elapsed_picoseconds(b2);

#ifndef _WIN32
    struct timespec ts;
    (void) clock_getres(best_clock, &ts);
    long long precision =
        BENCH_MAX(ts.tv_sec * 1000000000000LL + ts.tv_nsec * 1000LL, MIN_PRECISION);
#else
    long long precision = MIN_PRECISION;
#endif

    struct estimate est1;
    struct estimate est2;
    predict(t1, t2, est);
    predict(low(t1, precision), high(t2, precision), &est1);
    predict(high(t1, precision), low(t2, precision), &est2);
    est->stdev = BENCH_MAX(est1.stdev, est2.stdev);
}
