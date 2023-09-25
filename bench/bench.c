/*
 * Copyright © 2015 Kazunobu Kuriyama <kazunobu.kuriyama@nifty.com>
 *                  Ran Benita <ran234@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
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
        .nanoseconds = val.tv_usec * 1000,
    };
}

void
bench_stop(struct bench *bench)
{
    struct timeval val;
    (void) gettimeofday(&val, NULL);
    bench->stop = (struct bench_time) {
        .seconds = val.tv_sec,
        .nanoseconds = val.tv_usec * 1000,
    };
}

#ifndef _WIN32
void
bench_start2(struct bench *bench)
{
    struct timespec t;
	(void) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
	// (void) clock_gettime(CLOCK_MONOTONIC, &t);
    bench->start = (struct bench_time) {
        .seconds = t.tv_sec,
        .nanoseconds = t.tv_nsec,
    };
}

void
bench_stop2(struct bench *bench)
{
    struct timespec t;
	(void) clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
	// (void) clock_gettime(CLOCK_MONOTONIC, &t);
    bench->stop = (struct bench_time) {
        .seconds = t.tv_sec,
        .nanoseconds = t.tv_nsec,
    };
}
#endif

void
bench_elapsed(const struct bench *bench, struct bench_time *result)
{
    result->seconds = bench->stop.seconds - bench->start.seconds;
    result->nanoseconds = bench->stop.nanoseconds - bench->start.nanoseconds;
    if (result->nanoseconds < 0) {
        result->nanoseconds += 1000000000;
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
    ret = asprintf(&buf, "%ld.%06ld", elapsed.seconds, elapsed.nanoseconds / 1000);
    assert(ret >= 0);

    return buf;
}

/* Utils for bench method adapted from: https://hackage.haskell.org/package/tasty-bench */

#define fit(x1, x2) (x1) / 5 + 2 * ((x2) / 5)
#define sqr(x) (x) * (x)

static void
predict(long long t1, long long t2, struct estimate *est)
{
    const long long t = fit(t1, t2);
    est->elapsed = t;
    est->stdev = trunc(sqrt((double)sqr(t1 - t) + (double)sqr(t2 - 2 * t)));
}

#define high(t, prec) t + prec
#define low(t, prec) t - prec
#define MIN_PRECISION 1000000 /* 1ms */

void
predictPerturbed(const struct bench_time *b1, const struct bench_time *b2,
                 struct estimate *est)
{
    const long long t1 = bench_time_elapsed_nanoseconds(b1);
    const long long t2 = bench_time_elapsed_nanoseconds(b2);

#ifndef _WIN32
    struct timespec ts;
    (void) clock_getres(CLOCK_PROCESS_CPUTIME_ID, &ts);
    long long precision = MAX(ts.tv_sec * 1000000000 + ts.tv_nsec, MIN_PRECISION);
#else
    long long precision = MIN_PRECISION;
#endif

    struct estimate est1;
    struct estimate est2;
    predict(t1, t2, est);
    predict(low(t1, precision), high(t2, precision), &est1);
    predict(high(t1, precision), low(t2, precision), &est2);
    est->stdev = MAX(est1.stdev, est2.stdev);
}
