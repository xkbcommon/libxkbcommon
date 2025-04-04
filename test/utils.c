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
    static int initialized = 0;
    if (!initialized) {
        srand((unsigned int)time(NULL));
        initialized = 1;
    }

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
    static int initialized = 0;
    if (!initialized) {
        srand((unsigned int)time(NULL));
        initialized = 1;
    }

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
        const struct {
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
                .input = "x",
                .dec = { 0, UINT64_C(0) },
                .hex = { 0, UINT64_C(0) }
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
                .input = "184467440737095516150",
                .dec = { -1, UINT64_MAX                  },
                .hex = { -1, UINT64_C(0x1844674407370955)}
            },
            {
                .input = "ffffffffffffffff",
                .dec = { 0 , 0         },
                .hex = { 16, UINT64_MAX}
            },
            {
                .input = "fffffffffffffffff",
                .dec = { 0 , 0         },
                .hex = { -1, UINT64_MAX}
            },

        };
        for (size_t k = 0; k < ARRAY_SIZE(tests); k++) {
            uint64_t dec = 0;
            int count;
            count = parse_dec_to_uint64_t(tests[k].input, SIZE_MAX, &dec);
            assert_printf(count == tests[k].dec.count,
                          "SIZE_MAX #%zu, expected: %d, got: %d\n",
                          k, tests[k].dec.count, count);
            assert_printf(dec == tests[k].dec.val,
                          "SIZE_MAX #%zu, expected: %"PRIu64", got: %"PRIu64"\n",
                          k, tests[k].dec.val, dec);
            uint64_t hex = 0;
            count = parse_hex_to_uint64_t(tests[k].input, SIZE_MAX, &hex);
            assert_printf(count == tests[k].hex.count,
                          "SIZE_MAX #%zu, expected: %d, got: %d\n",
                          k, tests[k].hex.count, count);
            assert_printf(hex == tests[k].hex.val,
                          "SIZE_MAX #%zu, expected: %#"PRIx64", got: %#"PRIx64"\n",
                          k, tests[k].hex.val, hex);
        }
    }

#define str_safe_len(input) \
    (sizeof(input) == 0 ?   \
        0                   \
        : sizeof(input) - ((input)[sizeof(input) - 1] == '\0' ? -1 : 0))
#define PRIuint64_t PRIx64
#define PRIuint32_t PRIx32
#define test_parse_to(type, format, input, count, expected) do {             \
    type n = 0;                                                              \
    int r = parse_##format##_to_##type(input, str_safe_len(input), &n);      \
    assert_printf(r == (count),                                              \
                  "expected count: %d, got: %d (value: %#"PRI##type", string: %s)\n", \
                  count, r, n, input);                                       \
    assert_printf(n == (expected),                                           \
                  "expected value: %#"PRI##type", got: %#"PRI##type"\n", expected, n);         \
} while (0)

    char empty[] = {};
    char not_null_terminated_0[] = { '0' };
    char not_null_terminated_1[] = { '1' };
    char not_null_terminated_dec_max[] = {
        '1', '8', '4', '4', '6', '7', '4', '4', '0', '7', '3', '7', '0', '9',
        '5', '5', '1', '6', '1', '5'
    };
    char buffer[30] = {0};
    int count;

    test_parse_to(uint64_t, dec, empty, 0, UINT64_C(0));
    test_parse_to(uint64_t, dec, not_null_terminated_0, 1, UINT64_C(0));
    test_parse_to(uint64_t, dec, not_null_terminated_1, 1, UINT64_C(1));
    test_parse_to(uint64_t, dec, not_null_terminated_dec_max,
                  (int) sizeof(not_null_terminated_dec_max), UINT64_MAX);
    test_parse_to(uint64_t, dec, empty, 0, UINT64_C(0));
    test_parse_to(uint64_t, dec, "", 0, UINT64_C(0));
    test_parse_to(uint64_t, dec, "/", 0, UINT64_C(0));
    test_parse_to(uint64_t, dec, ";", 0, UINT64_C(0));
    test_parse_to(uint64_t, dec, "x", 0, UINT64_C(0));
    test_parse_to(uint64_t, dec, "/0", 0, UINT64_C(0));
    test_parse_to(uint64_t, dec, ";0", 0, UINT64_C(0));
    test_parse_to(uint64_t, dec, "x0", 0, UINT64_C(0));
    test_parse_to(uint64_t, dec, "18446744073709551616", -1, UINT64_MAX / 10);
    test_parse_to(uint64_t, dec, "184467440737095516150", -1, UINT64_MAX);
    test_parse_to(uint64_t, dec, "99999999999999999999", -1,
                  UINT64_C(9999999999999999999));

    char not_null_terminated_hex_max[] = {
        'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
        'f', 'f', 'f', 'f', 'f', 'f', 'f', 'f',
    };

    test_parse_to(uint64_t, hex, empty, 0, UINT64_C(0x0));
    test_parse_to(uint64_t, hex, not_null_terminated_0, 1, UINT64_C(0x0));
    test_parse_to(uint64_t, hex, not_null_terminated_1, 1, UINT64_C(0x1));
    test_parse_to(uint64_t, hex, not_null_terminated_hex_max,
                  (int) sizeof(not_null_terminated_hex_max), UINT64_MAX);
    test_parse_to(uint64_t, hex, "", 0, UINT64_C(0x0));
    test_parse_to(uint64_t, hex, "/", 0, UINT64_C(0x0));
    test_parse_to(uint64_t, hex, ";", 0, UINT64_C(0x0));
    test_parse_to(uint64_t, hex, "x", 0, UINT64_C(0x0));
    test_parse_to(uint64_t, hex, "xf", 0, UINT64_C(0x0));
    test_parse_to(uint64_t, hex, "00000000000000000", 17, UINT64_C(0x0));
    test_parse_to(uint64_t, hex, "00000000000000001", 17, UINT64_C(0x1));
    test_parse_to(uint64_t, hex, "ffffffffffffffff0", -1, UINT64_MAX);
    test_parse_to(uint64_t, hex, "10000000000000000", -1,
                  UINT64_C(0x1000000000000000));

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
    for (size_t k = 0; k < ARRAY_SIZE(values); k++) {
        /* Basic: decimal */
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
    srand(time(NULL));
    for (unsigned k = 0; k < 10000; k++) {
        const uint32_t x32 = rand_uint32();
        const uint64_t x64 = rand_uint64();
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
        /* Decimal */
        count = snprintf(buffer, sizeof(buffer), "%"PRIu64, x64);
        assert(count > 0);
        test_parse_to(uint64_t, dec, buffer, count, x64);
        /* Decimal: some garbage after */
        buffer[count] = (char) (((unsigned) rand()) % '0');
        test_parse_to(uint64_t, dec, buffer, count, x64);
    }
}
/* NOLINTEND(google-readability-function-size) */

int
main(void)
{
    test_init();
    test_string_functions();
    test_path_functions();
    test_number_parsers();

    return EXIT_SUCCESS;
}
