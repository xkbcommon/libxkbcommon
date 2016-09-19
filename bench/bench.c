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

#if defined(HAVE_CLOCK_GETTIME)
#define USE_CLOCK_GETTIME
#include <time.h>
#elif defined(__MACH__) && __MACH__ == 1
#define USE_MACH_ABSOLUTE_TIME
#include <mach/mach_time.h>
#else
/* gettimeofday() - a last resort */
#include <sys/time.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "bench.h"

static void
set_bench_time(struct bench_time *dest, long seconds, long milliseconds)
{
    dest->seconds = seconds;
    dest->milliseconds = milliseconds;
}

static void
normalize_bench_time(struct bench_time *obj)
{
    if (obj->milliseconds >= 0) {
        return;
    }
    obj->milliseconds += 1000000;
    obj->seconds--;
}

void
bench_timer_reset(struct bench_timer *self)
{
#if defined(USE_MACH_ABSOLUTE_TIME)
    mach_timebase_info_data_t info;
    if (mach_timebase_info(&info) == 0) {
        self->scaling_factor = info.numer / info.denom;
    }
#endif
    self->start.seconds = 0L;
    self->start.milliseconds = 0L;
    self->stop.seconds = 0L;
    self->stop.milliseconds = 0L;
}

void
bench_timer_start(struct bench_timer *self)
{
#if defined(USE_CLOCK_GETTIME)
    struct timespec val;

    (void) clock_gettime(CLOCK_MONOTONIC, &val);

    /* With conversion from nanosecond to millisecond */
    set_bench_time(&self->start, val.tv_sec, val.tv_nsec / 1000);
#elif defined(USE_MACH_ABSOLUTE_TIME)
    uint64_t val;

    val = mach_absolute_time();

    /* With conversion from nanosecond to millisecond */
    set_bench_time(&self->start,
                   self->scaling_factor * val / 1000000000,
                   self->scaling_factor * val % 1000000000 / 1000);
#else
    struct timeval val;

    (void) gettimeofday(&val, NULL);

    set_bench_time(&self->start, val.tv_sec, val.tv_usec);
#endif
}

void
bench_timer_stop(struct bench_timer *self)
{
#if defined(USE_CLOCK_GETTIME)
    struct timespec val;

    (void) clock_gettime(CLOCK_MONOTONIC, &val);

    /* With conversion from nanosecond to millisecond */
    set_bench_time(&self->stop, val.tv_sec, val.tv_nsec / 1000);
#elif defined(USE_MACH_ABSOLUTE_TIME)
    uint64_t val;

    val = mach_absolute_time();

    /* With conversion from nanosecond to millisecond */
    set_bench_time(&self->stop,
                   self->scaling_factor * val / 1000000000,
                   self->scaling_factor * val % 1000000000 / 1000);
#else
    struct timeval val;

    (void) gettimeofday(&val, NULL);

    set_bench_time(&self->stop, val.tv_sec, val.tv_usec);
#endif
}

void
bench_timer_get_elapsed_time(struct bench_timer *self, struct bench_time *result)
{
    result->seconds = self->stop.seconds - self->start.seconds;
    result->milliseconds = self->stop.milliseconds - self->start.milliseconds;
}

char *
bench_timer_get_elapsed_time_str(struct bench_timer *self)
{
    struct bench_time elapsed;
    char *buf;
    int ret;

    bench_timer_get_elapsed_time(self, &elapsed);
    normalize_bench_time(&elapsed);
    ret = asprintf(&buf, "%ld.%06ld", elapsed.seconds, elapsed.milliseconds);
    assert(ret >= 0);

    return buf;
}
