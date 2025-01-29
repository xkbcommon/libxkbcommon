/*
 * Copyright © 2015 Kazunobu Kuriyama <kazunobu.kuriyama@nifty.com>
 * Copyright © 2015 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
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
        for (unsigned int k = 0; k < 2 * n; k++) {                                \
            __VA_ARGS__                                                           \
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
