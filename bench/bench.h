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

struct bench_time {
    long seconds;
    long nanoseconds;
};

struct bench {
    struct bench_time start;
    struct bench_time stop;
};

struct estimate {
    long long int elapsed;
    long long int stdev;
};

void
bench_start(struct bench *bench);
void
bench_stop(struct bench *bench);

#ifndef _WIN32
void
bench_start2(struct bench *bench);
void
bench_stop2(struct bench *bench);
#else
/* TODO: implement clock_getres for Windows */
#define bench_start2 bench_start
#define bench_stop2  bench_stop
#endif

void
bench_elapsed(const struct bench *bench, struct bench_time *result);

#define bench_time_elapsed_microseconds(elapsed) \
    ((elapsed)->nanoseconds / 1000 + 1000000 * (elapsed)->seconds)
#define bench_time_elapsed_nanoseconds(elapsed) \
    ((elapsed)->nanoseconds + 1000000000 * (elapsed)->seconds)

/* The caller is responsibile to free() the returned string. */
char *
bench_elapsed_str(const struct bench *bench);

/* Bench method adapted from: https://hackage.haskell.org/package/tasty-bench */
#define BENCH(target_stdev, n, time, est, ...) do {                               \
    struct bench _bench;                                                          \
    struct bench_time _t1;                                                        \
    struct bench_time _t2;                                                        \
    n = 1;                                                                        \
    bench_start2(&_bench);                                                        \
    do { __VA_ARGS__ } while (0);                                                 \
    bench_stop2(&_bench);                                                         \
    bench_elapsed(&_bench, &_t1);                                                 \
    do {                                                                          \
        bench_start2(&_bench);                                                    \
        for (int k = 0; k < 2 * n; k++) {                                         \
            do { __VA_ARGS__ } while (0);                                         \
        }                                                                         \
        bench_stop2(&_bench);                                                     \
        bench_elapsed(&_bench, &_t2);                                             \
        predictPerturbed(&_t1, &_t2, &est);                                       \
        if (est.stdev < (long long)(MAX(0, target_stdev * (double)est.elapsed))) {\
            scale_estimate(est, n);                                               \
            time = _t2;                                                           \
            n *= 2;                                                               \
            break;                                                                \
        }                                                                         \
        n *= 2;                                                                   \
        _t1 = _t2;                                                                \
    } while (1);                                                                  \
} while (0)

void
predictPerturbed(const struct bench_time *t1, const struct bench_time *t2,
                 struct estimate *est);

#define scale_estimate(est, n) do { \
    (est).elapsed /= (n);           \
    (est).stdev /= (n);             \
} while (0);

#endif /* LIBXKBCOMMON_BENCH_H */
