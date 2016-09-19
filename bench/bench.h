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

#ifndef LIBXKBCOMMON_BENCH_H
#define LIBXKBCOMMON_BENCH_H

#include <stdint.h>

struct bench_time {
    long seconds;
    long milliseconds;
};

struct bench_timer {
    struct bench_time start;
    struct bench_time stop;
#if defined(__MACH__) && __MACH__ == 1
    uint64_t scaling_factor;
#endif
};

void bench_timer_reset(struct bench_timer *self);

void bench_timer_start(struct bench_timer *self);
void bench_timer_stop(struct bench_timer *self);

void bench_timer_get_elapsed_time(struct bench_timer *self, struct bench_time *result);
/* It's caller's responsibility to release the returned string using free(). */
char *bench_timer_get_elapsed_time_str(struct bench_timer *self);

#endif /* LIBXKBCOMMON_BENCH_H */
