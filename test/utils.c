/*
 * Copyright © 2019 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "test.h"
#include "utils.h"
#include "utils-numbers.h"
#include "utils-paths.h"
#include "test/utils-text.h"

static void
test_string_functions(void)
{
    char buffer[10];

    assert(!snprintf_safe(buffer, 0, "foo"));
    assert(!snprintf_safe(buffer, 1, "foo"));
    assert(!snprintf_safe(buffer, 3, "foo"));

    assert(snprintf_safe(buffer, 10, "foo"));
    assert(streq(buffer, "foo"));

    assert(!snprintf_safe(buffer, 10, "%s", "1234567890"));
    assert(snprintf_safe(buffer, 10, "%s", "123456789"));

    assert(streq_null("foo", "foo"));
    assert(!streq_null("foobar", "foo"));
    assert(!streq_null("foobar", NULL));
    assert(!streq_null(NULL, "foobar"));
    assert(streq_null(NULL, NULL));

    const char text[] =
        "123; // abc\n"
        "  // def\n"
        "456 // ghi // jkl\n"
        "// mno\n"
        "//\n"
        "ok; // pqr\n"
        "foo\n";

    char *out;
    out = strip_lines(text, ARRAY_SIZE(text), "//");
    assert_streq_not_null(
        "strip_lines",
        "123; \n"
        "456 \n"
        "ok; \n"
        "foo\n",
        out
    );
    free(out);

    out = uncomment(text, ARRAY_SIZE(text), "//");
    assert_streq_not_null(
        "uncomment",
        "123;  abc\n"
        "   def\n"
        "456  ghi // jkl\n"
        " mno\n"
        "\n"
        "ok;  pqr\n"
        "foo\n",
        out
    );
    free(out);
}

static void
test_path_functions(void)
{
    /* Absolute paths */
    assert(!is_absolute(""));
#ifdef _WIN32
    assert(!is_absolute("path\\test"));
    assert(is_absolute("c:\\test"));
    assert(!is_absolute("c:test"));
    assert(is_absolute("c:\\"));
    assert(is_absolute("c:/"));
    assert(!is_absolute("c:"));
    assert(is_absolute("\\\\foo"));
    assert(is_absolute("\\\\?\\foo"));
    assert(is_absolute("\\\\?\\UNC\\foo"));
    assert(is_absolute("/foo"));
    assert(is_absolute("\\foo"));
#else
    assert(!is_absolute("test/path"));
    assert(is_absolute("/test" ));
    assert(is_absolute("/" ));
#endif
}

static uint32_t
rand_uint32(void)
{
    /* First decide how many bits we’ll actually use (1-32) */
    int bits = 1 + (rand() % 32);

    /* Generate a number with that many bits */
    uint32_t result = 0;
    while (bits > 0) {
        int bits_this_round = bits > 16 ? 16 : bits;
        result = (result << bits_this_round)
               | (rand() & ((1U << bits_this_round) - 1));
        bits -= bits_this_round;
    }

    return result;
}

static uint64_t
rand_uint64(void)
{
    /* First decide how many bits we’ll actually use (1-64) */
    int bits = 1 + (rand() % 64);

    /* Generate a number with that many bits */
    uint64_t result = 0;
    while (bits > 0) {
        int bits_this_round = bits > 16 ? 16 : bits;
        result = (result << bits_this_round)
               | (rand() & ((1U << bits_this_round) - 1));
        bits -= bits_this_round;
    }

    return result;
}

/* NOLINTBEGIN(google-readability-function-size) */
static void
test_number_parsers(void)
{
    /* Check the claim that it always works on normal strings using SIZE_MAX and
     * that it always stops on the first NULL byte */
    {
        static const struct {
            const char* input;
            struct { int count; uint64_t val; } dec;
            struct { int count; uint64_t val; } hex;
        } tests[] = {
            {
                .input = "",
                .dec = { 0, UINT64_C(0) },
                .hex = { 0, UINT64_C(0) }
            },
            {
                .input = "\0""123",
                .dec = { 0, UINT64_C(0) },
                .hex = { 0, UINT64_C(0) }
            },
            {
                .input = "/",
                .dec = { 0, UINT64_C(0) },
                .hex = { 0, UINT64_C(0) }
            },
            {
                .input = ";",
                .dec = { 0, UINT64_C(0) },
                .hex = { 0, UINT64_C(0) }
            },
            {
                .input = "x",
                .dec = { 0, UINT64_C(0) },
                .hex = { 0, UINT64_C(0) }
            },
            {
                .input = "/1",
                .dec = { 0, UINT64_C(0) },
                .hex = { 0, UINT64_C(0) }
            },
            {
                .input = ";1",
                .dec = { 0, UINT64_C(0) },
                .hex = { 0, UINT64_C(0) }
            },
            {
                .input = "x1",
                .dec = { 0, UINT64_C(0) },
                .hex = { 0, UINT64_C(0) }
            },
            {
                .input = "0",
                .dec = { 1, UINT64_C(0) },
                .hex = { 1, UINT64_C(0) }
            },
            {
                .input = "1",
                .dec = { 1, UINT64_C(1) },
                .hex = { 1, UINT64_C(1) }
            },
            {
                .input = "123",
                .dec = { 3, UINT64_C(123)  },
                .hex = { 3, UINT64_C(0x123)}
            },
            {
                .input = "123x",
                .dec = { 3, UINT64_C(123)  },
                .hex = { 3, UINT64_C(0x123)}
            },
            {
                .input = "123""\0""456",
                .dec = { 3, UINT64_C(123)  },
                .hex = { 3, UINT64_C(0x123)}
            },
            {
                .input = "18446744073709551615",
                .dec = { 20, UINT64_MAX                  },
                .hex = { -1, UINT64_C(0x1844674407370955)}
            },
            {
                .input = "18446744073709551616",
                .dec = { -1, UINT64_C(1844674407370955161)},
                .hex = { -1, UINT64_C(0x1844674407370955) }
            },
            {
                .input = "99999999999999999999",
                .dec = { -1, UINT64_C(9999999999999999999)},
                .hex = { -1, UINT64_C(0x9999999999999999) }
            },
            {
                .input = "184467440737095516150",
                .dec = { -1, UINT64_MAX                  },
                .hex = { -1, UINT64_C(0x1844674407370955)}
            },
            {
                .input = "00000000000000000",
                .dec = { 17, UINT64_C(0) },
                .hex = { 17, UINT64_C(0) }
            },
            {
                .input = "00000000000000001",
                .dec = { 17, UINT64_C(1) },
                .hex = { 17, UINT64_C(1) }
            },
            {
                .input = "ffffffffffffffff",
                .dec = { 0 , 0         },
                .hex = { 16, UINT64_MAX}
            },
            {
                .input = "ffffffffffffffff0",
                .dec = { 0 , 0         },
                .hex = { -1, UINT64_MAX}
            },
            {
                .input = "10000000000000000",
                .dec = { 17, UINT64_C(10000000000000000) },
                .hex = { -1, UINT64_C(0x1000000000000000)}
            },
            {
                .input = "fffffffffffffffff",
                .dec = { 0 , 0         },
                .hex = { -1, UINT64_MAX}
            },

        };
        for (size_t k = 0; k < ARRAY_SIZE(tests); k++) {
            const size_t len = strlen(tests[k].input);
            /* Try different lengths */
            const struct {const char* label; size_t len;} sizes[] = {
                { .label = "buffer"  , .len = len     },
                { .label = "string"  , .len = len + 1 },
                { .label = "SIZE_MAX", .len = SIZE_MAX}
            };
            for (unsigned int s = 0; s < ARRAY_SIZE(sizes); s++) {
                int count;
                /* Decimal */
                uint64_t dec = 0;
                count =
                    parse_dec_to_uint64_t(tests[k].input, sizes[s].len, &dec);
                assert_printf(count == tests[k].dec.count,
                              "Dec %s #%zu \"%s\" (%zu), "
                              "expected: %d, got: %d\n",
                              sizes[s].label, k, tests[k].input, sizes[s].len,
                              tests[k].dec.count, count);
                assert_printf(dec == tests[k].dec.val,
                              "Dec %s #%zu \"%s\", "
                              "expected: %"PRIu64", got: %"PRIu64"\n",
                              sizes[s].label, k, tests[k].input,
                              tests[k].dec.val, dec);
                /* Hexadecimal */
                uint64_t hex = 0;
                count =
                    parse_hex_to_uint64_t(tests[k].input, sizes[s].len, &hex);
                assert_printf(count == tests[k].hex.count,
                              "Hex %s #%zu \"%s\" (%zu), "
                              "expected: %d, got: %d\n",
                              sizes[s].label, k, tests[k].input, sizes[s].len,
                              tests[k].hex.count, count);
                assert_printf(hex == tests[k].hex.val,
                              "Hex %s #%zu \"%s\", "
                              "expected: %#"PRIx64", got: %#"PRIx64"\n",
                              sizes[s].label, k, tests[k].input,
                              tests[k].hex.val, hex);
            }
        }
    }

#define PRIuint64_t PRIx64
#define PRIuint32_t PRIx32
#define test_parse_to(type, format, input, count, expected) do {     \
    type n = 0;                                                      \
    int r = parse_##format##_to_##type(input, ARRAY_SIZE(input), &n);\
    assert_printf(r == (count),                                      \
                  "Buffer: expected count: %d, "                     \
                  "got: %d (value: %#"PRI##type", string: %.*s)\n",  \
                  count, r, n, (int) ARRAY_SIZE(input), (input));    \
    assert_printf(n == (expected),                                   \
                  "Buffer: expected value: %#"PRI##type", got: "     \
                  "%#"PRI##type"\n", expected, n);                   \
} while (0)

    /* Test syntax variants */
    const uint64_t values[] = {
        UINT64_C(0),
        UINT64_C(1),
        UINT64_C(10),
        UINT64_C(0xA),
        UINT64_C(0xF),
        UINT64_C(123),
        UINT32_MAX / 10,
        UINT32_MAX / 10 + 9,
        UINT32_MAX >> 4,
        UINT32_MAX >> 4 | 0xf,
        UINT32_MAX - 1,
        UINT32_MAX,
        UINT32_MAX + UINT64_C(1),
        UINT64_C(9999999999999999999),
        UINT64_MAX / 10,
        UINT64_MAX / 10 + 9,
        UINT64_MAX >> 4,
        UINT64_MAX >> 4 | 0xf,
        UINT64_MAX - 1,
        UINT64_MAX,
    };
    char buffer[30] = {0};
    for (size_t k = 0; k < ARRAY_SIZE(values); k++) {
        int count;
        /* Basic: decimal */
        count = snprintf(buffer, sizeof(buffer), "%"PRIu32, (uint32_t) values[k]);
        assert(count > 0);
        test_parse_to(uint32_t, dec, buffer, count, (uint32_t) values[k]);
        count = snprintf(buffer, sizeof(buffer), "%"PRIu64, values[k]);
        assert(count > 0);
        test_parse_to(uint64_t, dec, buffer, count, values[k]);
        /* Basic: hex lower case */
        count = snprintf(buffer, sizeof(buffer), "%"PRIx32, (uint32_t) values[k]);
        assert(count > 0);
        test_parse_to(uint32_t, hex, buffer, count, (uint32_t) values[k]);
        count = snprintf(buffer, sizeof(buffer), "%"PRIx64, values[k]);
        assert(count > 0);
        test_parse_to(uint64_t, hex, buffer, count, values[k]);
        /* Basic: hex upper case */
        count = snprintf(buffer, sizeof(buffer), "%"PRIX32, (uint32_t) values[k]);
        assert(count > 0);
        test_parse_to(uint32_t, hex, buffer, count, (uint32_t) values[k]);
        count = snprintf(buffer, sizeof(buffer), "%"PRIX64, values[k]);
        assert(count > 0);
        test_parse_to(uint64_t, hex, buffer, count, values[k]);
        /* Prefix with some zeroes */
        for (int z = 0; z < 10 ; z++) {
            /* Decimal */
            count = snprintf(buffer, sizeof(buffer), "%0*"PRIu32,
                             z, (uint32_t) values[k]);
            assert(count > 0);
            test_parse_to(uint32_t, dec, buffer, count, (uint32_t) values[k]);
            count = snprintf(buffer, sizeof(buffer), "%0*"PRIu64,
                             z, values[k]);
            assert(count > 0);
            test_parse_to(uint64_t, dec, buffer, count, values[k]);
            /* Hexadecimal */
            count = snprintf(buffer, sizeof(buffer), "%0*u%"PRIx32,
                             z, 0, (uint32_t) values[k]);
            assert(count > 0);
            test_parse_to(uint32_t, hex, buffer, count, (uint32_t) values[k]);
            count = snprintf(buffer, sizeof(buffer), "%0*u%"PRIx64,
                             z, 0, values[k]);
            assert(count > 0);
            test_parse_to(uint64_t, hex, buffer, count, values[k]);
            /* Append some garbage */
            for (int c = 0; c < 0x100 ; c++) {
                if (c < '0' || c > '9') {
                    /* Decimal */
                    count = snprintf(buffer, sizeof(buffer), "%0*u%"PRIu32"%c",
                                     z, 0, (uint32_t) values[k], (char) c);
                    assert(count > 0);
                    test_parse_to(uint32_t, dec, buffer, count - 1, (uint32_t) values[k]);
                    count = snprintf(buffer, sizeof(buffer), "%0*u%"PRIu64"%c",
                                     z, 0, values[k], (char) c);
                    assert(count > 0);
                    test_parse_to(uint64_t, dec, buffer, count - 1, values[k]);
                }
                if (!is_xdigit((char) c)) {
                    /* Hexadecimal */
                    count = snprintf(buffer, sizeof(buffer), "%0*u%"PRIx32"%c",
                                     z, 0, (uint32_t) values[k], (char) c);
                    assert(count > 0);
                    test_parse_to(uint32_t, hex, buffer, count - 1, (uint32_t) values[k]);
                    count = snprintf(buffer, sizeof(buffer), "%0*u%"PRIx64"%c",
                                     z, 0, values[k], (char) c);
                    assert(count > 0);
                    test_parse_to(uint64_t, hex, buffer, count - 1, values[k]);
                }
            }
        }
    }

    /* Random */
    for (unsigned k = 0; k < 10000; k++) {
        const uint32_t x32 = rand_uint32();
        const uint64_t x64 = rand_uint64();
        int count;
        /* Hex: Lower case */
        count = snprintf(buffer, sizeof(buffer), "%"PRIx32, x32);
        assert(count > 0);
        test_parse_to(uint32_t, hex, buffer, count, x32);
        count = snprintf(buffer, sizeof(buffer), "%"PRIx64, x64);
        assert(count > 0);
        test_parse_to(uint64_t, hex, buffer, count, x64);
        /* Hex: Upper case (32 bits) */
        count = snprintf(buffer, sizeof(buffer), "%"PRIX32, x32);
        assert(count > 0);
        test_parse_to(uint32_t, hex, buffer, count, x32);
        /* Hex: some garbage after */
        buffer[count] = (char) (((unsigned) rand()) % '0');
        test_parse_to(uint32_t, hex, buffer, count, x32);
        /* Hex: Upper case (64 bits) */
        count = snprintf(buffer, sizeof(buffer), "%"PRIX64, x64);
        assert(count > 0);
        test_parse_to(uint64_t, hex, buffer, count, x64);
        /* Hex: some garbage after */
        buffer[count] = (char) (((unsigned) rand()) % '0');
        test_parse_to(uint64_t, hex, buffer, count, x64);
        /* Decimal (32 bits) */
        count = snprintf(buffer, sizeof(buffer), "%"PRIu32, x32);
        assert(count > 0);
        test_parse_to(uint32_t, dec, buffer, count, x32);
        /* Decimal: some garbage after */
        buffer[count] = (char) (((unsigned) rand()) % '0');
        test_parse_to(uint32_t, dec, buffer, count, x32);
        /* Decimal (64 bits) */
        count = snprintf(buffer, sizeof(buffer), "%"PRIu64, x64);
        assert(count > 0);
        test_parse_to(uint64_t, dec, buffer, count, x64);
        buffer[count] = (char) (((unsigned) rand()) % '0');
        test_parse_to(uint64_t, dec, buffer, count, x64);
    }
}
/* NOLINTEND(google-readability-function-size) */

/* CLI positional arguments:
 * 1. Seed for the pseudo-random generator:
 *    - Leave it unset or set it to “-” to use current time.
 *    - Use an integer to set it explicitly.
 */
int
main(int argc, char *argv[])
{
    test_init();

    /* Initialize pseudo-random generator with program arg or current time */
    unsigned int seed;
    if (argc >= 2 && !streq(argv[1], "-")) {
        seed = (unsigned int) atoi(argv[1]);
    } else {
        seed = (unsigned int) time(NULL);
    }
    fprintf(stderr, "Seed for the pseudo-random generator: %u\n", seed);
    srand(seed);

    test_string_functions();
    test_path_functions();
    test_number_parsers();

    return EXIT_SUCCESS;
}
