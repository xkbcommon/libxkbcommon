/*
 * Copyright © 2015 Kazunobu Kuriyama <kazunobu.kuriyama@nifty.com>
 * Copyright © 2015 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <limits.h>
#include <math.h>

#define BENCH_MAX(a, b) ((a) > (b) ? (a) : (b))

enum { BENCH_ITER_MAX = ((UINT_MAX >> 2) + 1u) };

struct bench_time {
    long int seconds;
    long long int picoseconds;
};

struct bench {
    struct bench_time start;
    struct bench_time stop;
};

struct estimate {
    long long int elapsed; /* picoseconds */
    long long int stdev;   /* picoseconds */
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

#define bench_pico_to_micro(t) ((t) / 1000000)
#define bench_pico_to_nano(t)  ((t) / 1000)

#define bench_pico_to_micro_rounded(t) llroundl((long double)(t) / 1000000)
#define bench_pico_to_nano_rounded(t)  llroundl((long double)(t) / 1000)

#define bench_time_elapsed_microseconds(elapsed) \
    ((elapsed)->picoseconds / 1000000 + 1000000LL * (elapsed)->seconds)
#define bench_time_elapsed_nanoseconds(elapsed) \
    ((elapsed)->picoseconds / 1000 + 1000000000LL * (elapsed)->seconds)
#define bench_time_elapsed_picoseconds(elapsed) \
    ((elapsed)->picoseconds + 1000000000000LL * (elapsed)->seconds)

/* The caller is responsibile to free() the returned string. */
char *
bench_elapsed_str(const struct bench *bench);

/**
 * Bench method adapted from: https://hackage.haskell.org/package/tasty-bench
 *
 * @param[in] target_stdev
 *   Relative stdev to reach.
 * @param[out] n
 *   Number of iterations of the last run.
 * @param[out] n_time
 *   Duration of the last run.
 * @param[out] est
 *   Average duration of an iteration.
 * @param[in] pre
 *   Run once per round (not timed).
 * @param[in] consume
 *   Run once per round (timed).
 * @param[in] post
 *   Run once per round (not timed).
 * @param[in] ...
 *   The measured body, which runs once per iteration.
 *
 * The benchmark stops when the relative stdev reaches the target or n reaches
 * BENCH_ITER_MAX. In the second case the target may not be met.
 */
#define BENCH(target_stdev, n, n_time, est, pre, consume, post, ...) do {\
    struct bench _bench;                                                 \
    struct bench_time _t1;                                               \
    struct bench_time _t2;                                               \
    n = 1;                                                               \
    /* First round */                                                    \
    pre;                                                                 \
    bench_start2(&_bench);                                               \
    do { __VA_ARGS__ } while (0);                                        \
    consume;                                                             \
    bench_stop2(&_bench);                                                \
    post;                                                                \
    bench_elapsed(&_bench, &_t1);                                        \
    do {                                                                 \
        pre;                                                             \
        bench_start2(&_bench);                                           \
        for (unsigned int _k = 0; _k < 2 * n; _k++) {                    \
            __VA_ARGS__                                                  \
        }                                                                \
        consume;                                                         \
        bench_stop2(&_bench);                                            \
        post;                                                            \
        bench_elapsed(&_bench, &_t2);                                    \
        predict_perturbed(&_t1, &_t2, &est);                             \
        if (est.stdev <                                                  \
            (long long)(BENCH_MAX(0, target_stdev *                      \
                                     (long double)est.elapsed)) ||       \
            n >= BENCH_ITER_MAX)                                         \
        {                                                                \
            break;                                                       \
        }                                                                \
        n *= 2;                                                          \
        _t1 = _t2;                                                       \
    } while (1);                                                         \
    scale_estimate(est, n);                                              \
    n_time = _t2;                                                        \
    n *= 2;                                                              \
} while (0)

void
predict_perturbed(const struct bench_time *t1, const struct bench_time *t2,
                  struct estimate *est);

#define scale_estimate(est, n) do { \
    (est).elapsed /= (n);           \
    (est).stdev /= (n);             \
} while (0)
