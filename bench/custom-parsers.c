/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <fcntl.h>
#include <time.h>
#include <getopt.h>
#include <stdint.h>

#include "utils.h"
#include "utils-numbers.h"

#include "bench.h"

const double DEFAULT_STDEV = 0.05;

static void
usage(char **argv)
{
    printf("Usage: %s [OPTIONS]\n"
           "\n"
           "Benchmark compilation of the given RMLVO\n"
           "\n"
           "Options:\n"
           " --help\n"
           "    Print this help and exit\n"
           " --stdev\n"
           "    Minimal relative standard deviation (percentage) to reach.\n"
           "    (default: %f)\n"
           "\n",
           argv[0], DEFAULT_STDEV * 100);
}

static void
print_stats(double stdev, unsigned int max_iterations,
            struct bench_time *elapsed, struct bench *bench,
            struct estimate *est)
{
    struct bench_time total_elapsed;
    bench_elapsed(bench, &total_elapsed);
    fprintf(stderr,
            "mean: %lld µs; stdev: %Lf%% (target: %f%%); "
            "last run: parsed %u times in %ld.%06lds; "
            "total time: %ld.%06lds\n", est->elapsed / 1000,
            (long double) est->stdev * 100.0 / (long double) est->elapsed,
            stdev * 100,
            max_iterations, elapsed->seconds, elapsed->nanoseconds / 1000,
            total_elapsed.seconds, total_elapsed.nanoseconds / 1000);
}

/* NOTE: Old parser, for comparison */
static bool
parse_keysym_hex(const char *s, uint32_t *out)
{
    uint32_t result = 0;
    unsigned int i;
    for (i = 0; i < 8 && s[i] != '\0'; i++) {
        result <<= 4;
        if ('0' <= s[i] && s[i] <= '9')
            result += s[i] - '0';
        else if ('a' <= s[i] && s[i] <= 'f')
            result += 10 + s[i] - 'a';
        else if ('A' <= s[i] && s[i] <= 'F')
            result += 10 + s[i] - 'A';
        else
            return false;
    }
    *out = result;
    return s[i] == '\0' && i > 0;
}

int
main(int argc, char **argv)
{
    struct bench bench;
    struct bench_time elapsed;
    struct estimate est;
    int ret = 0;
    double stdev = DEFAULT_STDEV;

    enum options {
        OPT_STDEV,
    };

    static struct option opts[] = {
        {"help",             no_argument,            0, 'h'},
        {"stdev",            required_argument,      0, OPT_STDEV},
        {0, 0, 0, 0},
    };

    while (1) {
        int c;
        int option_index = 0;
        c = getopt_long(argc, argv, "h", opts, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage(argv);
            exit(EXIT_SUCCESS);
        case OPT_STDEV:
            stdev = atof(optarg) / 100;
            if (stdev <= 0)
                stdev = DEFAULT_STDEV;
            break;
        default:
            usage(argv);
            exit(EXIT_INVALID_USAGE);
        }
    }

    FILE *file = fopen(__FILE__, "r");
    assert(file);
    size_t size = 0;
    char *content = NULL;
    map_file(file, &content, &size);
    assert(content);

    /*
     * Some numbers for the parsers, do not delete.
     *
     * 0x0000000000000000   0x0000000000000002   0x0000000000000003
     * 0x0000000000000001   0x00000000000000FE   0x00000000000001FE
     * 0x000000000000000A   0x0000000000000200   0x0000000000000400
     * 0x00000000000000FF   0x0000000000020000   0x0000000000040000
     * 0x0000000000000100   0x0000000002000000   0x0000000004000000
     * 0x0000000000001000   0x0000000200000000   0x0000000400000000
     * 0x0000000000010000   0x0000020000000000   0x0000040000000000
     * 0x0000000001000000   0x0002000000000000   0x0004000000000000
     * 0x0000000100000000   0x0200000000000000   0x0400000000000000
     * 0x0000010000000000   0x2000000000000000   0x4000000000000001
     * 0x0001000000000000   0x4000000000000000   0x3FFFFFFFFFFFFFFF
     * 0x0100000000000000   0x6FFFFFFFFFFFFFFF   0xA000000000000000
     * 0x1000000000000000   0x9000000000000000   0xCFFFFFFFFFFFFFFF
     * 0x7FFFFFFFFFFFFFFF   0xEFFFFFFFFFFFFFFF   0xD000000000000000
     * 0x8000000000000000   0xF000000000000000   0xE000000000000000
     * 0xFFFFFFFFFFFFFFFF   0x1A2B3C4D5E6F7089   0x0807060504030201
     * 0x123456789ABCDEF0   0x89706F5E4D3C2B1A   0xF1E2D3C4B5A69788
     * 0xFEDCBA9876543210   0x5A5A5A5A5A5A5A5A   0x6B6B6B6B6B6B6B6B
     * 0xABABABABABABABAB   0xA5A5A5A5A5A5A5A5   0xB6B6B6B6B6B6B6B6
     * 0xCDCDCDCDCDCDCDCD   0xC3D2E1F00F1E2D3C   0x1122334455667788
     * 0x0123456789ABCDEF   0x3C2D1E0F0FE1D2C3   0x8877665544332211
     * 0x9876543210FEDCBA   0x0000000080000000   0x0000000040000000
     * 0x00000000FFFFFFFF   0x8000000000000000   0x4000000000000000
     * 0xFFFFFFFF00000000   0x6666666666666666   0x7777777777777777
     * 0x5555555555555555   0x9999999999999999   0x8888888888888888
     * 0x0AAAAAAAAAAAAAAA   0x0000000200000002   0x0000000300000003
     * 0x0000000100000001   0x4444444444444444   0x5F5F5F5F5F5F5F5F
     * 0x1111111111111111   0xBBBBBBBBBBBBBBBB   0xC0C0C0C0C0C0C0C0
     * 0x2222222222222222   0xCCCCCCCCCCCCCCCC   0xE1E1E1E1E1E1E1E1
     * 0x3333333333333333   0xDDDDDDDDDDDDDDDD   0xF2F2F2F2F2F2F2F2
     * 0x1A3F5C7E9D2B4A68   0x8E6D4C2B1A0F9E7D   0x3F9A8B7C6D5E4F2A
     * 0x7B6C5D4E3F2A1B09   0x2D4E6F8A9C0B1D3E   0x5A4B3C2D1E0F9A8B
     * 0x9E8D7C6B5A4F3E2D   0x1C3E5F7A9D0B2E4F   0x6D5E4F3A2B1C0D9E
     * 0xA0B1C2D3E4F56789   0x3B4D5F6E7A8C9D0E   0x7E8F9A0B1C2D3E4F
     * 0x2C3D4E5F6A7B8C9D   0x9A8B7C6D5E4F3A2B   0x1E2D3C4B5A6F7E8D
     * 0x4D5E6F7A8B9C0D1E   0x3A2B1C0D9E8F7A6B   0x8C9D0E1F2A3B4C5D
     * 0x5F6E7D8C9B0A1F2E   0x0A1B2C3D4E5F6A7B   0x9C0D1E2F3A4B5C6D
     * 0x6A7B8C9D0E1F2A3B   0x2E3F4A5B6C7D8E9F   0x1D2C3B4A5F6E7D8C
     * 0x7D8E9F0A1B2C3D4E   0x4B5C6D7E8F9A0B1C   0x0F1E2D3C4B5A6F7E
     * 0x8B9C0D1E2F3A4B5C   0x5A6B7C8D9E0F1A2B   0x3C4D5E6F7A8B9C0D
     * 0x9D0E1F2A3B4C5D6E   0x6B7C8D9E0F1A2B3C   0x2A3B4C5D6E7F8A9B
     * 0x0E1F2A3B4C5D6E7F   0x7C8D9E0F1A2B3C4D   0x4A5B6C7D8E9F0A1B
     * 0x1F2A3B4C5D6E7F8A   0x8D9E0F1A2B3C4D5E   0x5B6C7D8E9F0A1B2C
     * 0x2B3C4D5E6F7A8B9C   0x9E0F1A2B3C4D5E6F   0x6C7D8E9F0A1B2C3D
     * 0x0A1B2C3D4E5F6A7B   0x7E8F9A0B1C2D3E4F   0x3D4E5F6A7B8C9D0E
     * 0x1B2C3D4E5F6A7B8C   0x8F9A0B1C2D3E4F5A   0x4E5F6A7B8C9D0E1F
     * 0x3A4B5C6D7E8F9A0B   0x9F0A1B2C3D4E5F6A   0x5C6D7E8F9A0B1C2D
     * 0x0B1C2D3E4F5A6B7C   0x7F8A9B0C1D2E3F4A   0x2D3E4F5A6B7C8D9E
     * 0x1C2D3E4F5A6B7C8D   0x9A0B1C2D3E4F5A6B   0x4F5A6B7C8D9E0F1A
     * 0x6D7E8F9A0B1C2D3E   0x0C1D2E3F4A5B6C7D   0x3B4C5D6E7F8A9B0C
     * 0x8E9F0A1B2C3D4E5F   0x5D6E7F8A9B0C1D2E   0x1A2B3C4D5E6F7A8B
     * 0x0D1E2F3A4B5C6D7E   0x7A8B9C0D1E2F3A4B   0x2E3F4A5B6C7D8E9F
     * 0x9B0C1D2E3F4A5B6C   0x6E7F8A9B0C1D2E3F   0x0F1A2B3C4D5E6F7A
     */

    volatile uint32_t __attribute__((unused)) dummy32 = 0;
    volatile uint64_t __attribute__((unused)) dummy64 = 0;
    unsigned int max_iterations;

    printf("*** parse_hex_to_uint32_t ***\n");
    bench_start2(&bench);
        BENCH(stdev, max_iterations, elapsed, est,
            for (size_t n = 0; n < size; n++) {
                uint32_t val = 0;
                parse_hex_to_uint32_t(content + n, 8, &val);
                dummy32 += val;
            }
        );
    bench_stop2(&bench);
    print_stats(stdev, max_iterations, &elapsed, &bench, &est);

    printf("*** parse_keysym_hex ***\n");
    bench_start2(&bench);
        BENCH(stdev, max_iterations, elapsed, est,
            for (size_t n = 0; n < size; n++) {
                uint32_t val = 0;
                parse_keysym_hex(content + n, &val);
                dummy32 += val;
            }
        );
    bench_stop2(&bench);
    print_stats(stdev, max_iterations, &elapsed, &bench, &est);

    printf("*** parse_dec_to_uint64_t ***\n");
    bench_start2(&bench);
        BENCH(stdev, max_iterations, elapsed, est,
            for (size_t n = 0; n < size; n++) {
                uint64_t val = 0;
                parse_dec_to_uint64_t(content + n, size - n, &val);
                dummy64 += val;
            }
        );
    bench_stop2(&bench);
    print_stats(stdev, max_iterations, &elapsed, &bench, &est);

    printf("*** strtol, base 10 ***\n");
    bench_start2(&bench);
        BENCH(stdev, max_iterations, elapsed, est,
            for (size_t n = 0; n < size; n++) {
                dummy64 += strtol(content + n, NULL, 10);
            }
        );
    bench_stop2(&bench);
    print_stats(stdev, max_iterations, &elapsed, &bench, &est);


    printf("*** parse_hex_to_uint64_t ***\n");
    bench_start2(&bench);
        BENCH(stdev, max_iterations, elapsed, est,
            for (size_t n = 0; n < size; n++) {
                uint64_t val = 0;
                parse_hex_to_uint64_t(content + n, size - n, &val);
                dummy64 += val;
            }
        );
    bench_stop2(&bench);
    print_stats(stdev, max_iterations, &elapsed, &bench, &est);


    printf("*** strtol, base 16 ***\n");
    bench_start2(&bench);
        BENCH(stdev, max_iterations, elapsed, est,
            for (size_t n = 0; n < size; n++) {
                dummy64 += strtol(content + n, NULL, 16);
            }
        );
    bench_stop2(&bench);
    print_stats(stdev, max_iterations, &elapsed, &bench, &est);

    unmap_file(content, size);
    fclose(file);

    return ret;
}
