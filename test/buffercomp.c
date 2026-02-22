/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "keymap.h"
#include "src/keysym.h"
#include "test/keysym.h"
#include "test.h"
#include "utils.h"
#include "xkbcommon/xkbcommon.h"
#include "test-config.h"

#define GOLDEN_TESTS_OUTPUTS "keymaps/"

struct keymap_test_data {
    /** Keymap string to compile */
    const char * const keymap;
    /** Resulting file *path* to reference serialization,
     *  or NULL if the keymap string should not compile. */
    const char * const expected;
    /** Optionally skip the test */
    bool skip;
};

/* Our keymap compiler is the xkbcommon buffer compiler */
static struct xkb_keymap *
compile_buffer(struct xkb_context *context, enum xkb_keymap_format format,
               const char *buf, size_t len, void *private)
{
    return test_compile_buffer(context, format, buf, len);
}

static void
test_encodings(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    /* Accept UTF-8 encoded BOM (U+FEFF) */
    const char utf8_with_bom[] = "\xef\xbb\xbfxkb_keymap {};";
    keymap = test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                 utf8_with_bom, sizeof(utf8_with_bom));
    assert(keymap);
    xkb_keymap_unref(keymap);

    /* Reject UTF-16LE encoded string */
    const char utf16_le[] = "x\0k\0b\0_\0k\0e\0y\0m\0a\0p\0 \0{\0}\0;\0";
    keymap = test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                 utf16_le, sizeof(utf16_le));
    assert(!keymap);

    /* Reject UTF-16LE with BOM encoded string */
    const char utf16_le_with_bom[] =
        "\xff\xfex\0k\0b\0_\0k\0e\0y\0m\0a\0p\0 \0{\0}\0;\0";
    keymap = test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                 utf16_le_with_bom, sizeof(utf16_le_with_bom));
    assert(!keymap);

    /* Reject UTF-16BE encoded string */
    const char utf16_be[] = "\0x\0k\0b\0_\0k\0e\0y\0m\0a\0p\0 \0{\0}\0;";
    keymap = test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                 utf16_be, sizeof(utf16_be));
    assert(!keymap);
}

struct keymap_simple_test_data {
    const char * const keymap;
    bool valid;
};

static void
test_floats(struct xkb_context *ctx)
{
    const struct keymap_simple_test_data tests[] = {
        /* Valid floats */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_geometry {\n"
                "    width=123.456;\n"
                "    width=123.0;\n"
                "    width=123.;\n"
                "    width=01.234;\n"
                "    width=01.0;\n"
                "    width=01.;\n"
                "    width=001.234;\n"
                "    width=001.0;\n"
                "    width=001.;\n"
                "  };"
                "};",
            .valid = true
        },
        /* Invalid: missing integer part */
        {
            .keymap = "xkb_keymap { xkb_geometry { width=.123; }; };",
            .valid = false
        },
        /* Invalid: comma decimal separator */
        {
            .keymap = "xkb_keymap { xkb_geometry { width=1,23; }; };",
            .valid = false
        },
        /* Invalid: exponent */
        {
            .keymap = "xkb_keymap { xkb_geometry { width=1.23e2; }; };",
            .valid = false
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        struct xkb_keymap *keymap =
            test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                tests[k].keymap, strlen(tests[k].keymap));
        assert(tests[k].valid ^ !keymap);
        xkb_keymap_unref(keymap);
    }
}

static void
test_component_syntax_error(struct xkb_context *ctx)
{
    /* The following used to trigger memory leak */
    const char* const keymaps[] = {
        "xkb_keymap {"
        "  xkb_keycodes {};"
        "};"
        "};", /* Syntax error, keymap “complete” */
        "xkb_keymap {"
        "  xkb_keycodes {};"
        "  xkb_types {};"
        "  xkb_compat {};"
        "  xkb_symbols {};"
        "};"
        "};"/* Syntax error, keymap complete */,
        "xkb_keymap {"
        "  xkb_keycodes {};"
        "}" /* Syntax error, incomplete map */
        "  xkb_types {};"
        "  xkb_compat {};"
        "  xkb_symbols {};"
        "};",
        "xkb_keymap {"
        "  xkb_keycodes {};"
        ";" /* Syntax error, incomplete map */
        "  xkb_types {};"
        "  xkb_compat {};"
        "  xkb_symbols {};"
        "};",
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        const struct xkb_keymap *keymap =
            test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                keymaps[k], strlen(keymaps[k]));
        assert(!keymap);
    }
}

/** Test that any component is optional and can be empty */
static void
test_optional_components(struct xkb_context *ctx, bool update_output_files)
{
    const struct keymap_test_data keymaps[] = {
        /*
         * Optional or empty
         */
        {
            .keymap = "xkb_keymap {};",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "xkb_keymap { xkb_keycodes {}; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "xkb_keymap { xkb_types {}; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "xkb_keymap { xkb_compat {}; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "xkb_keymap { xkb_symbols {}; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        /*
         * Some content, to check we handle missing data correctly
         */
        {
            /* Indicator not defined in keycodes */
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat { indicator \"XXX\" { modifiers=Lock; }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-no-real-led.xkb"
        },
        {
            /* Key not defined */
            .keymap =
                "xkb_keymap {\n"
                "  xkb_symbols { key <> { [a] }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            /* Key type not defined */
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };"
                "  xkb_symbols { key <> { [a], type=\"XXX\" }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-basic.xkb"
        },
        {
            /* Virtual modifier not defined */
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };"
                "  xkb_symbols { key <> { vmods=XXX, [a] }; };\n"
                "};",
            .expected = NULL
        },
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_bidi_chars(struct xkb_context *ctx, bool update_output_files)
{
    const struct keymap_test_data keymaps[] = {
        /* Invalid: first char must be ASCII */
        {
            .keymap = u8"\u200Exkb_keymap {};",
            .expected = NULL
        },
        {
            .keymap = u8"\u200Fxkb_keymap {};",
            .expected = NULL
        },
        /* Valid */
        {
            .keymap =
                u8" \u200Fxkb_keymap\u200E\u200F\n\u200E{ "
                u8"\u200Exkb_keycodes \u200F{ "
                u8"<>\u200E= \u200F1\u200E;\u200F"
                u8"}\u200E ;"
                u8"};\u200E",
            .expected = GOLDEN_TESTS_OUTPUTS "bidi.xkb"
        }
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_recursive_includes(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    const char* const keymaps[] = {
        /* Recursive keycodes */
        "Keycodes: recursive",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev+recursive\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"pc\" };"
        "};",
        "Keycodes: recursive(bar)",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev+recursive(bar)\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"pc\" };"
        "};",
        /* Recursive key types */
        "Key types: recursive",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"recursive\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"pc\" };"
        "};",
        "Key types: recursive(bar)",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"recursive(bar)\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"pc\" };"
        "};",
        /* Recursive compat */
        "Compat: recursive",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"recursive\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"pc\" };"
        "};",
        "Compat: recursive(bar)",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"recursive(bar)\" };"
        "  xkb_symbols { include \"pc\" };"
        "};",
        /* Recursive symbols */
        "Symbols: recursive",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"recursive\" };"
        "};",
        "Symbols: recursive(bar)",
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"complete\" };"
        "  xkb_symbols { include \"recursive(bar)\" };"
        "};"
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u %s ***\n", __func__, k, keymaps[k]);
        k++;
        keymap = test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                     keymaps[k], strlen(keymaps[k]));
        assert(!keymap);
    }
}

static void
test_include_paths(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;
    const char simple_str[] =
        "xkb_keymap {\n"
        "  xkb_keycodes \"test\" { include \"evdev\" };\n"
        "  xkb_types    \"test\" { include \"complete\" };\n"
        "  xkb_symbols  \"test\" { include \"pc+us(basic)+"
                                            "level5\" };\n"
        "};";
    keymap = test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                 simple_str, sizeof(simple_str));
    char *expected =
        xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    xkb_keymap_unref(keymap);

    static const char* keymaps[] = {
        "xkb_keymap {\n"
        "  xkb_keycodes \"test\" { include \"%S/evdev\" };\n"
        "  xkb_types    \"test\" { include \"%S/complete\" };\n"
        "  xkb_symbols  \"test\" { include \"%S/pc+%S/us(basic)+"
                                            "%S/level5\" };\n"
        "};",
/* Windows absolute path contains “:”, which has special meaning in XKB includes,
 * so skip it. */
#ifndef _WIN32
        "xkb_keymap {\n"
        "  xkb_keycodes \"test\" { include \"" TEST_XKB_CONFIG_ROOT "/keycodes/evdev\" };\n"
        "  xkb_types    \"test\" { include \"" TEST_XKB_CONFIG_ROOT "/types/complete\" };\n"
        "  xkb_symbols  \"test\" { include \"" TEST_XKB_CONFIG_ROOT "/symbols/pc+"
                                               TEST_XKB_CONFIG_ROOT "/symbols/us(basic)+"
                                               TEST_XKB_CONFIG_ROOT "/symbols/level5\" };\n"
        "};",
#endif
    };

    /* Relative path, which does not work as expected, because it is
     * not resolved before %-expansion. */
    setenv("XKB_CONFIG_ROOT", "..", 1);
    keymap = test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                 keymaps[0], strlen(keymaps[0]));
    assert(!keymap);

    setenv("XKB_CONFIG_ROOT", TEST_XKB_CONFIG_ROOT, 1);

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        keymap = test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                     keymaps[k], strlen(keymaps[k]));
        assert(keymap);
        char *got =
            xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
        assert_streq_not_null("test_include_paths", expected, got);
        free(got);
        xkb_keymap_unref(keymap);
    }

    free(expected);
}

static void
test_include_default_maps(bool update_output_files)
{
    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES);
    assert(ctx);
    /* “User” config */
    char *include_path = test_get_path("extra");
    assert(include_path);
    assert(xkb_context_include_path_append(ctx, include_path));
    free(include_path);
    /* “System” config */
    include_path = test_get_path("");
    assert(include_path);
    assert(xkb_context_include_path_append(ctx, include_path));
    free(include_path);

    const struct keymap_test_data keymaps[] = {
        {
            /* Use system explicit default map */
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <CAPS> = 66; };\n"
                "  xkb_symbols { include \"capslock\" };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "include-capslock-system.xkb"
        },
        {
            /* Use custom named map */
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <CAPS> = 66; };\n"
                "  xkb_symbols { include \"capslock(custom)\" };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "include-capslock-custom.xkb"
        },
        {
            /* Use custom *explicit* default map */
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <RALT> = 108; <LVL3> = 92; };\n"
                "  xkb_types { include \"basic\" };\n"
                "  xkb_symbols { include \"level3\" };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "include-level3-explicit-default.xkb"
        },
        {
            /* Use custom *implicit* default map */
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols { include \"banana\" };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "include-banana-implicit-default.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }

    xkb_context_unref(ctx);
}

/* Test some limits related to allocations */
static void
test_alloc_limits(struct xkb_context *ctx, bool update_output_files)
{
    const struct keymap_test_data keymaps[] = {
        /* Keycodes */
        {
            .keymap =
                "xkb_keymap {\n"
                /* Valid & supported high keycode value */
                "  xkb_keycodes { <> = 0xfffffffe; };\n"
                "  xkb_symbols { key <> {[a]}; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "high-keycodes-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <xxx> = 0xfffffffe;\n"
                "    <> = 0xfffffffd;\n"
                "    <> = 0xfffffffe;\n"
                "  };\n"
                "  xkb_symbols { key <> {[a]}; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "high-keycodes-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <> = 7;\n"
                "    <> = 8;\n"
                "    <> = 0xfffffffd;\n"
                "    <> = 0xfffffffe;\n"
                "  };\n"
                "  xkb_symbols { key <> {[a]}; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "high-keycodes-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <> = 0xfffffffd;\n"
                "    <> = 0xfffffffe;\n"
                "    <> = 7;\n"
                "    <> = 8;\n"
                "  };\n"
                "  xkb_symbols { key <> {[a]}; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "high-keycodes-2.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <min> = 0xfffffffd;\n"
                "    <max> = 0xfffffffe;\n"
                "  };\n"
                "  xkb_symbols {\n"
                "    key <min> {[a]};\n"
                "    key <max> {[b]};\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "high-keycodes-3.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <max> = 0xfffffffe;\n"
                "    <min> = 0xfffffffd;\n"
                "  };\n"
                "  xkb_symbols {\n"
                "    key <min> {[a]};\n"
                "    key <max> {[b]};\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "high-keycodes-3.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <a> = 0xf;\n"
                "    <d> = 0x4000;\n"
                "    <e> = 0xfffffffe;\n"
                "    <c> = 0x2000;\n"
                "    <b> = 0xfff;\n"
                "  };\n"
                "  xkb_symbols {\n"
                "    key <a> {[a]};\n"
                "    key <b> {[b]};\n"
                "    key <c> {[c]};\n"
                "    key <d> {[d]};\n"
                "    key <e> {[e]};\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "high-keycodes-4.xkb"
        },
        /* Key types */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_types {\n"
                /* Invalid level index */
                "    type \"X\" { map[none] = 0xfffffffe; };\n"
                "  };\n"
                "};",
            .expected = NULL
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_types {\n"
                /* Invalid level index */
                "    type \"X\" {levelname[0xfffffffe]=\"x\";};\n"
                "  };\n"
                "};",
            .expected = NULL
        }
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_integers(struct xkb_context *ctx, bool update_output_files) {
    /* Using a buffer without a terminating NULL. The following is a obvious
     * syntax error, but it should still exit *cleanly*, not as previously where
     * the use of strtoll would trigger a memory violation. */
    const char not_null_terminated[] = { '1' };
    assert(!test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                not_null_terminated, sizeof(not_null_terminated)));
    const bool skipOverflow = sizeof(int64_t) >= sizeof (long long);
    if (skipOverflow) {
        fprintf(stderr,
                "[WARNING] %s: cannot use checked arithmetic\n", __func__);
    }

    const struct keymap_test_data keymaps[] = {
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                /* Out of range (expect 32 bits, got > 64 bits) */
                "    <> = 0x10000000000000000;\n"
                "  };\n"
                "};",
            .expected = NULL
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_compat {\n"
                "  };\n"
                "  xkb_symbols {\n"
                "    key <> {\n"
                /* Checked int64_t underflow */
                "      [MovePointer(x=-0x7fffffffffffffff - 2,\n"
                "                   y=0)]\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "integers-overflow.xkb",
            .skip = skipOverflow
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_compat {\n"
                "  };\n"
                "  xkb_symbols {\n"
                "    key <> {\n"
                /* Checked int64_t underflow */
                "      [MovePointer(x=-0x7fffffffffffffff * 2,\n"
                "                   y=0)]\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "integers-overflow.xkb",
            .skip = skipOverflow
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_compat {\n"
                "  };\n"
                "  xkb_symbols {\n"
                "    key <> {\n"
                "      [MovePointer(x=0,\n"
                /* Checked int64_t overflow */
                "                   y=0x7fffffffffffffff + 1)]\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "integers-overflow.xkb",
            .skip = skipOverflow
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_compat {\n"
                "  };\n"
                "  xkb_symbols {\n"
                "    key <> {\n"
                "      [MovePointer(x=0,\n"
                /* Checked int64_t overflow */
                "                   y=0x7fffffffffffffff * 2)]\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "integers-overflow.xkb",
            .skip = skipOverflow
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <> = 1;\n"
                "    indicator 32 = \"xxx\";\n"
                "  };\n"
                "  xkb_compat {\n"
                "    group 0xffffffff = Mod5;\n"
                "  };\n"
                "  xkb_symbols {\n"
                /* Computations with 64 bit ints that then fit into 16 bits */
                "    key <> {\n"
                "      actions[1 + -~0x100000001 / 0x100000000]=\n"
                "      [MovePointer(x=0x100000000 - 0xfffffffe,\n"
                "                   y=~-0x7fff * 0x30000 / 0x2ffff)],\n"
                /* Test (INT64_MIN + 1) and INT64_MAX */
                "      [MovePointer(x=-9223372036854775807\n"
                "                     +9223372036854775807)]\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "integers.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols {\n"
                /* We cannot parse INT64_MIN literal.
                 * If we could, the following should fit into 16 bits. */
                "    key <> {\n"
                "      [MovePointer(x=-9223372036854775808\n"
                "                     +9223372036854775807)]\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = NULL
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <0> = 10;\n"
                "    <1> = 11;\n"
                "    <2> = 12;\n"
                "    <a> = 20;\n"
                "    <b> = 21;\n"
                "    <c> = 22;\n"
                "  };\n"
                "  xkb_types {\n"
                "    type \"FOUR_LEVEL\" {\n"
                "      modifiers = Shift + Control;\n"
                "      map[None] = 01;\n"
                "      map[Shift] = 2;\n"
                "      map[Control] = 3;\n"
                "    };\n"
                "  }\n;"
                "  xkb_compat {\n"
                "    interpret 0   { action = LockGroup(group=2); };\n"
                "    interpret 00  { action = LockGroup(group=3); };\n"
                "    interpret 0x0 { action = LockGroup(group=4); };\n"
                "    interpret 1   { action = LockGroup(group=-1); };\n"
                "    interpret 01  { action = LockGroup(group=1); };\n"
                "    interpret 2   { action = LockGroup(group=-2); };\n"
                "    interpret 02  { action = LockGroup(group=2); };\n"
                "    interpret 3   { action = LockGroup(group=-3); };\n"
                "    interpret 0x3 { action = LockGroup(group=3); };\n"
                "  };\n"
                "  xkb_symbols {\n"
                /* Various numeric keysyms forms.
                 * Single digits have a special interpretation in symbols */
                "    key <0> { [ 0, 00, 0x0] };\n"
                "    key <1> { [ 1, 01, 0x1] };\n"
                "    key <2> { [ 2, 02, 0x2] };\n"
                "    key <a> { [ 3   ] };\n"
                "    key <b> { [ 04  ] };\n"
                "    key <c> { [ 0x5 ] };\n"
                "    modifier_map Shift   { 0,   3   };\n"
                "    modifier_map Lock    { 00,  04  };\n"
                "    modifier_map Control { 0x0, 0x5 };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "digits.xkb"
        },
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        if (keymaps[k].skip) {
            fprintf(stderr, "INFO: skip test\n");
            continue;
        }
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_keycodes(struct xkb_context *ctx, bool update_output_files) {
    const struct keymap_test_data keymaps[] = {
        /*
         * Ensure keycodes bound are correctly updated.
         *
         * Since expanding bounds is tested in the rest of the code, we test
         * only shrinking here.
         */

        /* Single keycode */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 0;\n"
                "    override <A> = 1;\n"
                "    augment  <A> = 300;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 300;\n"
                "    override <A> = 1;\n"
                "    augment  <A> = 0;\n"
                "  };\n"
                "};",
            /* NOTE: same as previous */
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 0;\n"
                "    override <A> = 1;\n"
                "    override <A> = 301;\n"
                "    override <A> = 300;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-2.xkb"
        },

        /* Multiple keycodes to single keycode */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 300;\n"
                "    <B> = 1;\n"
                "    augment  <B> = 301;\n"
                "    override <A> = 1;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 0;\n"
                "    <B> = 1;\n"
                "    augment  <B> = 300;\n"
                "    override <A> = 1;\n"
                "  };\n"
                "};",
            /* NOTE: same as previous */
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-1.xkb"
        },

        /* Multiple keycodes to multiple keycodes */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 1;\n"
                "    <B> = 0;\n"
                "    override <B> = 300;\n"
                "    augment  <A> = 0;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-multiple-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 301;\n"
                "    <B> = 300;\n"
                "    override <A> = 1;\n"
                "    augment  <B> = 302;\n"
                "  };\n"
                "};",
            /* NOTE: same as previous */
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-multiple-1.xkb"
        },

        /* High keycodes */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 1;\n"
                "    include \"high\"\n"
                "    <B> = 2;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-multiple-2.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 0x10000;\n"
                "    <B> = 0x10001;\n"
                "    <A> = 1;\n"
                "    <B> = 0x10002;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-multiple-3.xkb"
        },

        /* Aliases */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <B> = 1;\n"
                "    <A> = 1;\n"
                "    alias <C> = <B>;\n" /* C target is invalid */
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    alias <C> = <B>;\n" /* C target is invalid */
                "    <B> = 1;\n"
                "    <A> = 1;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <B> = 1;\n"
                "    <A> = 1;\n"
                "    alias <C> = <B>;\n" /* C target is valid */
                "    <B> = 2;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-aliases-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    alias <C> = <B>;\n" /* C target is valid */
                "    <B> = 1;\n"
                "    <A> = 1;\n"
                "    <B> = 2;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-aliases-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    alias <A> = <B>;\n"
                "    <A> = 1;\n" /* Override alias */
                "    <B> = 300;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-multiple-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    alias <C> = <B>;\n"
                "    augment <C> = 3;\n" /* Do not override alias */
                "    <A> = 1;\n"
                "    <B> = 2;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-aliases-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 1;\n"
                "    <C> = 3;\n"
                "    alias <C> = <B>;\n" /* Override key */
                "    <B> = 2;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-aliases-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 1;\n"
                "    augment alias <A> = <B>;\n" /* Do not override key */
                "    <B> = 300;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-multiple-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <A> = 1;\n"
                "    <B> = 2;\n"
                "    alias <B> = <C>;\n" /* Override key, even if invalid entry */
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <B> = 1;\n"
                "    alias <A> = <B>;\n"
                "    <A> = 1;\n" /* Override both alias and key */
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-single-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                /*
                 * Multiple aliases *before* names: check that the key name LUT
                 * is replaced with new allocation.
                 */
                "    alias <A> = <X>;\n"
                "    alias <B> = <X>;\n"
                "    alias <C> = <X>;\n"
                "    alias <D> = <X>;\n"
                "    alias <E> = <X>;\n"
                "    alias <F> = <X>;\n"
                "    alias <G> = <X>;\n"
                "    alias <H> = <X>;\n"
                "    alias <I> = <X>;\n"
                "    alias <J> = <X>;\n"
                "    alias <K> = <X>;\n"
                "    alias <L> = <X>;\n"
                "    alias <M> = <X>;\n"
                "    alias <N> = <X>;\n"
                "    alias <O> = <X>;\n"
                "    alias <P> = <X>;\n"
                "    <X> = 1;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-aliases-2.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                /*
                 * Multiple aliases *before* names: check that the key name LUT
                 * overwrite is done properly after the last alias.
                 */
                "    alias <A> = <1>;\n"
                "    alias <B> = <1>;\n"
                "    alias <C> = <1>;\n"
                "    alias <D> = <1>;\n"
                "    alias <E> = <1>;\n"
                "    <1> = 1;\n"
                "    <2> = 2;\n"
                "    <3> = 3;\n"
                "    <4> = 4;\n"
                "    <5> = 5;\n"
                "    <6> = 6;\n"
                "    <7> = 7;\n"
                "    <8> = 8;\n"
                "    <9> = 9;\n"
                "    <10> = 10;\n"
                "    <11> = 11;\n"
                "    <12> = 12;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-aliases-3.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: key bounds #%u ***\n", __func__, k);
        /*
         * We use a new context because we want to check key name LUT is
         * correctly implemented: it requires an empty atom table.
         */
        struct xkb_context * const ctx2 = test_get_context(CONTEXT_NO_FLAG);
        assert(ctx2);
        assert(test_compile_output(ctx2, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
        xkb_context_unref(ctx2);
    }

    /* Test random high keycodes do not trigger xkbcomp asserts */
    static const unsigned int max_iterations = 1000;
    char buffer[2048] = "";
    for (unsigned int i = 0; i < max_iterations; i++) {
        size_t available = sizeof(buffer);
        char *buf = buffer;
        int count = snprintf(buf, available,
                             "default xkb_keymap { xkb_keycodes {\n");
        assert(count > 0 && (size_t) count < available);
        available -= (size_t) count;
        buf += count;

        static const struct {
            xkb_keycode_t min;
            xkb_keycode_t max;
            unsigned int max_count;
        } bounds[] = {
            {
                .min = 10 << 3, /* Avoid issue with digits */
                .max = XKB_KEYCODE_MAX_CONTIGUOUS,
                .max_count = 3
            },
            {
                .min = XKB_KEYCODE_MAX_CONTIGUOUS + 1,
                .max = XKB_KEYCODE_MAX,
                .max_count = 10
            },
        };

        xkb_keycode_t keycodes[13] = {0};
        unsigned int keycode_index = 0;

        for (size_t b = 0; b < ARRAY_SIZE(bounds); b++) {
            assert(bounds[b].min < bounds[b].max);
            const unsigned int keycode_count = rand() % (bounds[b].max_count + 1);
            for (unsigned int k = 0; k < keycode_count; k++) {
                /* Note: we do not care about keycode uniqueness */
                const xkb_keycode_t kc =
                    bounds[b].min + (rand() % (bounds[b].max - bounds[b].min + 1));
                assert(keycode_index < ARRAY_SIZE(keycodes));
                keycodes[keycode_index++] = kc;
                count = snprintf(buf, available,
                                "<%"PRIu32"> = 0x%"PRIx32";\n", kc, kc);
                assert(count > 0 && (size_t) count < available);
                available -= (size_t) count;
                buf += count;
            }
        }

        count = snprintf(buf, available, "}; xkb_symbols {\n");
        assert(count > 0 && (size_t) count < available);
        available -= (size_t) count;
        buf += count;

        for (unsigned int k = 0; k < keycode_index; k++) {
            const xkb_keycode_t kc = keycodes[k];
            count = snprintf(buf, available,
                             "key <%"PRIu32"> { [ 0x%"PRIx32" ] };\n",
                             kc, (kc >> 3));
            assert(count > 0 && (size_t) count < available);
            available -= (size_t) count;
            buf += count;
        }

        count = snprintf(buf, available, "}; };");
        assert(count > 0 && (size_t) count < available);

        struct xkb_keymap * const keymap = xkb_keymap_new_from_string(
            ctx, buffer, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS
        );
        assert(keymap);
        for (unsigned int k = 0; k < keycode_index; k++) {
            const xkb_keycode_t kc = keycodes[k];
            const xkb_keysym_t *ks = NULL;
            count = xkb_keymap_key_get_syms_by_level(keymap, kc, 0, 0, &ks);
            assert_printf(count == 1, "%d\n", count);
            const xkb_keysym_t expected = kc >> 3;
            assert_printf(*ks == expected,
                          "<%"PRIu32"> 0x%"PRIx32" != 0x%"PRIx32"\n",
                          kc, *ks, expected);
        }
        xkb_keymap_unref(keymap);
    }

    /* Ensure long key names have a fallback in V1 format */
    static const char long_names_1[] =
        "xkb_keymap {\n"
        "  xkb_keycodes {\n"
        "    <0008> = 8;\n" /* name conflict (count = 4): prefix ‘0’ skipped */
        "    <!09>  = 9;\n" /* no name conflict (count = 3): prefix ‘!’ available */
        "    <long-name-000axxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> = 0x000a;\n"
        "    <long-name-0fffxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> = 0x0fff;\n"
        "    <long-name-ffffxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> = 0xffff;\n"
        "    alias <long-alias-000axxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> = "
        "          <long-name-000axxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx>;\n"
        "    alias <long-alias-0fffxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> = "
        "          <long-name-0fffxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx>;\n"
        "    alias <long-alias-ffffxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> = "
        "          <long-name-ffffxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx>;\n"
        "  };\n"
        "  xkb_symbols {\n"
        "    key <0008> { [0x8] };\n"
        "    key <!09>  { [0x9] };\n"
        "    key <long-name-000axxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> {[0x000a]};\n"
        "    key <long-name-0fffxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> {[0x0fff]};\n"
        "    key <long-name-ffffxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> {[0xffff]};\n"
        "    modmap Shift { <0008>, <!09> };\n"
        "    modmap Lock {\n"
        "      <long-alias-000axxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx>,\n"
        "      <long-alias-0fffxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx>,\n"
        "      <long-alias-ffffxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx>\n"
        "    };\n"
        "  };\n"
        "};";

    static const char long_names_2[] =
        "xkb_keymap {\n"
        "  xkb_keycodes {\n"
        "    <!008> = 8;\n"
        "    <#009> = 9;\n"
        "    <$010> = 10;\n"
        "    <%011> = 11;\n"
        "    <&012> = 12;\n"
        "    <'013> = 13;\n"
        "    <(014> = 14;\n"
        "    <)015> = 15;\n"
        "    <*016> = 16;\n"
        "    <+017> = 17;\n"
        "    <,018> = 18;\n"
        "    <-019> = 19;\n"
        "    <.020> = 20;\n"
        "    </021> = 21;\n"
        "    <0022> = 22;\n"
        /* No prefix available: the following keys names will stay unchanged */
        "    <long-name-0100xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> = 100;\n"
        "    <long-name-0101xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> = 101;\n"
        "    alias <long-alias-0100xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> = "
        "          <long-name-0100xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx>;\n"
        "    alias <long-alias-0101xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx> = "
        "          <long-name-0101xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx>;\n"
        "  };\n"
        "};";
    static const struct {
        const char* keymap;
        const char* path;
        enum xkb_keymap_format format;
    } tests[] = {
        {
            .keymap = long_names_1,
            .path = GOLDEN_TESTS_OUTPUTS "keycodes-long-names-1-v1.xkb",
            .format = XKB_KEYMAP_FORMAT_TEXT_V1
        },
        {
            .keymap = long_names_1,
            .path = GOLDEN_TESTS_OUTPUTS "keycodes-long-names-1-v2.xkb",
            .format = XKB_KEYMAP_FORMAT_TEXT_V2
        },
        {
            .keymap = long_names_2,
            .path = GOLDEN_TESTS_OUTPUTS "keycodes-long-names-2.xkb",
            .format = XKB_KEYMAP_FORMAT_TEXT_V1
        },
        {
            .keymap = long_names_2,
            .path = GOLDEN_TESTS_OUTPUTS "keycodes-long-names-2.xkb",
            .format = XKB_KEYMAP_FORMAT_TEXT_V2
        },
    };
    for (unsigned int t = 0; t < ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: long key names #%u ***\n", __func__, t);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   tests[t].format,
                                   compile_buffer, NULL, __func__,
                                   tests[t].keymap, strlen(tests[t].keymap),
                                   tests[t].path,
                                   update_output_files));
    }
}

static void
test_masks(struct xkb_context *ctx, bool update_output_files) {
    const struct keymap_test_data keymaps[] = {
        {
                .keymap =
                    "xkb_keymap {\n"
                    "  xkb_compat {\n"
                    /* Cannot be negative */
                    "    virtual_modifiers Test1 = -1;\n"
                    "  };\n"
                    "};",
                .expected = NULL
        },
        {
                .keymap =
                    "xkb_keymap {\n"
                    "  xkb_compat {\n"
                    /* Out of range (expect 32bits) */
                    "    virtual_modifiers Test1 = 0x100000000;\n"
                    "  };\n"
                    "};",
                .expected = NULL
        },
        {
                .keymap =
                    "xkb_keymap {\n"
                    "  xkb_compat {\n"
                    /* Out of range (expect 32bits) */
                    "    virtual_modifiers Test1 = ~0x100000000;\n"
                    "  };\n"
                    "};",
                .expected = NULL
        },
        {
                .keymap =
                    "xkb_keymap {\n"
                    "  xkb_compat {\n"
                    /* Unsupported operator */
                    "    virtual_modifiers Test1 = !Mod1;\n"
                    "  };\n"
                    "};",
                .expected = NULL
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_types {\n"
                /* Try range */
                "    virtual_modifiers Test01 = 0;\n"
                "    virtual_modifiers Test02 = 0xffffffff;\n"
                /* Try various operations on masks */
                "    virtual_modifiers Test11 = 0xf0 + 0x0f;\n"
                "    virtual_modifiers Test12 = 0xff - 0x0f;\n"
                "    virtual_modifiers Test13 = ~0xf;\n"
                "    virtual_modifiers Test14 = ~none;\n"
                "    virtual_modifiers Test15 = ~all;\n"
                "    virtual_modifiers Test16 = ~0xffffffff;\n"
                "    virtual_modifiers Test17 = all - ~Mod1 + Mod2;\n"
                "    type \"XXX\" {\n"
                "      modifiers = Test12;\n"
                /* Masks mappings are not resolved here, so:
                 *   map[Test12 - Mod5] <=> map[Test12] */
                "      map[Test12 - Mod5] = 2;\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "masks.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "    xkb_keycodes { <a> = 38; };\n"
                "    xkb_symbols {\n"
                "        virtual_modifiers X = 0xf0000000;\n"
                "        key <a> { [ SetMods(mods = 0x00001100) ] };\n"
                "    };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "masks-extra-bits.xkb"
        }
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_interpret(struct xkb_context *ctx, bool update_output_files)
{
    const struct keymap_test_data keymaps[] = {
        /* Invalid: empty string */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat { interpret \"\" { repeat = false; } };\n"
                "};",
            .expected = NULL
        },
        /* Invalid UTF-8 encoding */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat { interpret \"\xff\" { repeat = false; }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid single Unicode code point */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat { interpret \"\\u{0}\" { repeat = false; }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid multiple Unicode code points */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat { interpret \"ab\" { repeat = false; }; };\n"
                "};",
            .expected = NULL
        },
        /* Valid */
        {
            .keymap =
                u8"xkb_keymap {\n"
                u8"  xkb_compat {\n"
                u8"   interpret 0x1     { repeat = false; };\n"
                u8"   interpret 0xB     { repeat = false; };\n"
                u8"   interpret Shift_L { repeat = false; };\n"
                u8"   interpret a       { repeat = false; };\n"
                u8"   interpret \"ä\"   { repeat = false; };\n"
                u8"   interpret \"✨\"  { repeat = false; };\n"
                u8"   interpret \"🎺\"  { repeat = false; };\n"
                u8"  };\n"
                u8"};",
            .expected = GOLDEN_TESTS_OUTPUTS "compat-interpret.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_group_indices_names(struct xkb_context *ctx, bool update_output_files)
{
    const struct {
        const char* keymap;
        const char* expected_v1;
        const char* expected_v2;
    } keymaps[] = {
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <> = 1;\n"
                "    indicator 1 = \"1\";\n"
                "    indicator 2 = \"2\";\n"
                "  };\n"
                "  xkb_compat {\n"
                "    interpret a { action = SetGroup(group=Group1);  };\n"
                "    interpret b { action = SetGroup(group=-Group2); };\n"
                "    interpret c { action = SetGroup(group=+Group3); };\n"
                "    indicator \"1\" { groups = Group4; };\n"
                "    indicator \"2\" { groups = All-Group1; };\n"
                "  };\n"
                "  xkb_symbols {\n"
                "    name[Group1] = \"1\";\n"
                "    name[Group2] = \"2\";\n"
                "    name[Group3] = \"3\";\n"
                "    name[Group4] = \"4\";\n"
                "    key <> {\n"
                "      groupsredirect=Group4,\n"
                "      symbols[Group1]=[a],\n"
                "      symbols[Group2]=[b],\n"
                "      symbols[Group3]=[c],\n"
                "      symbols[Group4]=[d]\n"
                "    };\n"
                "  };\n"
                "};",
            .expected_v1 = GOLDEN_TESTS_OUTPUTS "group-index-names-1.xkb",
            .expected_v2 = GOLDEN_TESTS_OUTPUTS "group-index-names-1.xkb"
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <> = 1;\n"
                "    indicator 1 = \"1\";\n"
                "    indicator 2 = \"2\";\n"
                "  };\n"
                "  xkb_compat {\n"
                "    interpret a { action = SetGroup(group=Group32);  };\n"
                "    interpret b { action = SetGroup(group=-Group32); };\n"
                "    interpret c { action = SetGroup(group=+Group32); };\n"
                "    indicator \"1\" { groups = Group32; };\n"
                "    indicator \"2\" { groups = All-Group32; };\n"
                "  };\n"
                "  xkb_symbols {\n"
                "    name[Group1] = \"1\";\n"
                "    name[Group4] = \"4\";\n"
                "    name[Group5] = \"5\";\n"
                "    name[Group8] = \"8\";\n"
                "    name[Group32] = \"32\";\n"
                "    key <> {\n"
                "      groupsredirect=Group32,\n"
                "      symbols[Group1]=[a],\n"
                "      symbols[Group4]=[b],\n"
                "      symbols[Group5]=[c],\n"
                "      symbols[Group8]=[d],\n"
                "      symbols[Group32]=[e]\n"
                "    };\n"
                "  };\n"
                "};",
            .expected_v1 = NULL,
            .expected_v2 = GOLDEN_TESTS_OUTPUTS "group-index-names-2.xkb"
        },
        /* No names for groups above 32 */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat { interpret a { action = SetGroup(group=Group33); }; };\n"
                "};",
            .expected_v1 = NULL,
            .expected_v2 = NULL
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { indicator 1 = \"1\"; };\n"
                "  xkb_compat { indicator \"1\" { groups = Group33; }; };\n"
                "};",
            .expected_v1 = NULL,
            .expected_v2 = NULL
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols { name[Group33] = \"1\"; };\n"
                "};",
            .expected_v1 = NULL,
            .expected_v2 = NULL
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols { key <> { groupsredirect=Group33 };\n"
                "};",
            .expected_v1 = NULL,
            .expected_v2 = NULL
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols { key <> { symbols[Group33] = [a] };\n"
                "};",
            .expected_v1 = NULL,
            .expected_v2 = NULL
        },
        /* First and Last: test default values */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat {\n"
                "    interpret ISO_First_Group {\n"
                "      action= LockGroup(group=First);\n"
                "    };\n"
                "    interpret ISO_Last_Group {\n"
                "      action= LockGroup(group=Last);\n"
                "    };\n"
                "    indicator \"First group\" {\n"
                "      groups = First;\n"
                "    };\n"
                "    indicator \"Last group\" {\n"
                "      groups = Last;\n"
                "    };\n"
                "  };\n"
                "};",
            .expected_v1 = GOLDEN_TESTS_OUTPUTS "group-index-names-3.xkb",
            .expected_v2 = GOLDEN_TESTS_OUTPUTS "group-index-names-3.xkb",
        },
        /* First and Last: test all properties */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_compat {\n"
                "    SetGroup.group = Last;\n"
                "    interpret ISO_First_Group {\n"
                "      action= LockGroup(group=First);\n"
                "    };\n"
                "    interpret ISO_Last_Group {\n"
                "      action= SetGroup();\n"
                "    };\n"
                "    indicator \"Later groups\" {\n"
                "      groups = All - First;\n"
                "    };\n"
                "    indicator \"Last group\" {\n"
                "      groups = Last;\n"
                "    };\n"
                "    indicator \"All but last group\" {\n"
                "      groups = All - Last;\n"
                "    };\n"
                "    indicator \"Y\" {\n"
                "      groups = Last + First;\n"
                "    };\n"
                "  };\n"
                "  xkb_types {\n"
                "    type \"ONE_LEVEL\" {};\n"
                "    type \"TWO_LEVEL\" {\n"
                "      modifiers = Shift;\n"
                "      map[Shift] = 2;\n"
                "    };\n"
                "  };"
                "  xkb_symbols {\n"
                "    name[first] = \"1\";\n"
                "    SetGroup.group = +(Last + 1);\n"
                "    key.groupsRedirect = Last;\n"
                "    key <> {\n"
                "      symbols[FIRST] = [1],\n"
                "      actions[First] = [SetGroup(group=Last), SetGroup(group=-Last)],\n"
                "      symbols[2]     = [2],\n"
                "      actions[2]     = [SetGroup(group=Last - First), SetGroup()],\n"
                "      actions[3]     = [SetGroup(group=First), SetGroup(group=-First)]\n"
                "    };\n"
                "  };\n"
                "};",
            .expected_v1 = GOLDEN_TESTS_OUTPUTS "group-index-names-4.xkb",
            .expected_v2 = GOLDEN_TESTS_OUTPUTS "group-index-names-4.xkb",
        },
        /* “Last” is not supported as a group index */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols { key <> { symbols[Last] = [a] }; };\n"
                "};",
            .expected_v1 = NULL,
            .expected_v2 = NULL
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected_v1, update_output_files));
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected_v2, update_output_files));
    }
}

static void
test_level_indices_names(struct xkb_context *ctx,
                            bool update_output_files)
{
    const struct {
        const char* keymap;
        const char* expected;
    } keymaps[] = {
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_types {\n"
                "    type \"X\" {\n"
                "      modifiers = Shift;\n"
                "      map[Shift] = Level2048;\n"
                "      level_name[Level2048] = \"x\";\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "level-index-names.xkb"
        },
        /* No names above 2048 */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_types {\n"
                "    type \"X\" {\n"
                "      modifiers = Shift;\n"
                "      map[Shift] = Level2049;\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = NULL,
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_types {\n"
                "    type \"X\" {\n"
                "      modifiers = Shift;\n"
                "      level_name[Level2049] = \"x\";\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = NULL,
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

/* Test various multi-{keysym,action} syntaxes */
static void
test_multi_keysyms_actions(struct xkb_context *ctx, bool update_output_files)
{
    /* Macro to create keymap for failing tests */
#define make_keymap(xs)                         \
        "xkb_keymap {\n"                        \
        "  xkb_keycodes {\n"                    \
        "    <AE01> = 10;\n"                    \
        "  };\n"                                \
        "  xkb_types { include \"basic\" };\n"  \
        "  xkb_compat { include \"basic\" };\n" \
        "  xkb_symbols {\n"                     \
        "    key <AE01> { [" xs "] };\n"        \
        "  };\n"                                \
        "};"

#define make_keymaps_with(func, name, a, b, c, d)                           \
    /* Test: valid keymaps */                                               \
    {                                                                       \
        .keymap =                                                           \
            "xkb_keymap {\n"                                                \
            "  xkb_keycodes {\n"                                            \
            "    <01> = 1;\n"                                               \
            "    <02> = 2;\n"                                               \
            "    <03> = 3;\n"                                               \
            "    <04> = 4;\n"                                               \
            "    <05> = 5;\n"                                               \
            "    <06> = 6;\n"                                               \
            "    <07> = 7;\n"                                               \
            "    <08> = 8;\n"                                               \
            "    <09> = 9;\n"                                               \
            "    <10> = 10;\n"                                              \
            "    <11> = 11;\n"                                              \
            "    <12> = 12;\n"                                              \
            "    <13> = 13;\n"                                              \
            "    <14> = 14;\n"                                              \
            "    <15> = 15;\n"                                              \
            "  };\n"                                                        \
            "  xkb_types { include \"basic+extra\" };\n"                    \
            "  xkb_compat { include \"basic\" };\n"                         \
            "  xkb_symbols {\n"                                             \
            "    key <01> { [ "a "] };\n"                                   \
            "    key <02> { [ "a", "b" ] };\n"                              \
            "    key <03> { [ {} ] };\n"                                    \
            "    key <04> { [ {}, "b" ] };\n"                               \
            "    key <05> { [ "a", {} ] };\n"                               \
            "    key <06> { [ {}, {} ] };\n"                                \
            "    key <07> { [ { "a" } ] };\n"                               \
            "    key <08> { [ { "a" }, { "b" } ] };\n"                      \
            "    key <09> { [ { "a", "b" } ] };\n"                          \
            "    key <10> { [ { "a", "b", "c" } ] };\n"                     \
            "    key <11> { [ "a", { "b", "c" } ] };\n"                     \
            "    key <12> { [ { "a", "b" }, "c" ] };\n"                     \
            "    key <13> { [ { "a", "b" }, { "c", "d" } ] };\n"            \
            "    key <14> { [ { "a", "b" }, "c", { "d", "a" } ] };\n"       \
            "    key <15> { [ { "a", "b" }, { "c", "d" }, "a" ] };\n"       \
            "  };\n"                                                        \
            "};",                                                           \
        .expected = GOLDEN_TESTS_OUTPUTS "symbols-multi-" name ".xkb"       \
    },                                                                      \
    /* Test: invalid keymaps */                                             \
    { .keymap = func("{ {} }")                        , .expected = NULL }, \
    { .keymap = func("{ "a", {} }")                   , .expected = NULL }, \
    { .keymap = func("{ {}, "b" }")                   , .expected = NULL }, \
    { .keymap = func("{ {}, {} }")                    , .expected = NULL }, \
    { .keymap = func("{ "a", { "b" } }")              , .expected = NULL }, \
    { .keymap = func("{ { "a" }, "b" }")              , .expected = NULL }, \
    { .keymap = func("{ { "a", "b" }, "c" }")         , .expected = NULL }, \
    { .keymap = func("{ "a", { "b", "c" } }")         , .expected = NULL }, \
    { .keymap = func("{ "a", {}, "c" }")              , .expected = NULL }, \
    { .keymap = func("{ "a", "b", {} }")              , .expected = NULL }, \
    { .keymap = func("{ {}, "b", "c" }")              , .expected = NULL }, \
    { .keymap = func("{ { "a", "b" }, "c", "d" }")    , .expected = NULL }, \
    { .keymap = func("{ "a", { "b", "c" }, "d" }")    , .expected = NULL }, \
    { .keymap = func("{ "a", "b", { "c", "d" } }")    , .expected = NULL }, \
    { .keymap = func("{ { "a", "b" }, { "c", "d" } }"), .expected = NULL }

    const struct keymap_test_data keymaps[] = {
        make_keymaps_with(make_keymap, "keysyms",
                          "a", "b", "c", "d"),
        make_keymaps_with(make_keymap, "actions",
                          "SetMods(modifiers=Control)",
                          "SetGroup(group=+1)",
                          "Private(data=\"foo\")",
                          "Private(data=\"bar\")"),
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <10> = 10;\n"
                "    <11> = 11;\n"
                "    <12> = 12;\n"
                "    <13> = 13;\n"
                "    <14> = 14;\n"
                "    <15> = 15;\n"
                "    <16> = 16;\n"
                "    <17> = 17;\n"
                "    <18> = 18;\n"
                "    <19> = 19;\n"
                "    <20> = 20;\n"
                "    <21> = 21;\n"
                "    <22> = 22;\n"
                "    <23> = 23;\n"
                "    <30> = 30;\n"
                "    <31> = 31;\n"
                "    <32> = 32;\n"
                "    <33> = 33;\n"
                "    <34> = 34;\n"
                "    <35> = 35;\n"
                "    <36> = 36;\n"
                "    <37> = 37;\n"
                "    <38> = 38;\n"
                "    <39> = 39;\n"
                "  };\n"
                "  xkb_types { include \"basic+extra\" };\n"
                "  xkb_compat {\n"
                "    interpret 1 { action = {}; };\n"
                "    interpret 2 { action = {NoAction()}; };\n"
                "    interpret 3 { action = {SetMods()}; };\n"
                "    interpret 4 { action = {SetMods(), SetGroup(group=1)}; };\n"
                "  };\n"
                "  xkb_symbols {\n"
                /* Empty keysyms */
                "    key <10> { [any, any ] };\n"
                "    key <11> { [{} , {}  ] };\n"
                "    key <12> { [any, any ], [SetMods(modifiers=Shift)] };\n"
                "    key <13> { [{} , {}  ], [SetMods(modifiers=Shift)] };\n"
                "    key <14> { [any, any ], type = \"TWO_LEVEL\" };\n"
                "    key <15> { [{} , {}  ], type = \"TWO_LEVEL\" };\n"
                "    key <16> { [a, A, any] };\n"
                "    key <17> { [a, A, {} ] };\n"
                "    key <18> { [a, A, any], type = \"FOUR_LEVEL_SEMIALPHABETIC\" };\n"
                "    key <19> { [a, A, {} ], type = \"FOUR_LEVEL_SEMIALPHABETIC\" };\n"
                "    key <20> { [a, A, ae, any] };\n"
                "    key <21> { [a, A, ae, {} ] };\n"
                "    key <22> { [a, A, ae, AE, any] };\n"
                "    key <23> { [a, A, ae, AE, {} ] };\n"
                /* Empty actions */
                "    key <30> { [NoAction(), NoAction() ] };\n"
                "    key <31> { actions=[{}, {}         ] };\n"
                "    key <32> { [NoAction(), NoAction() ], [a] };\n"
                "    key <33> { actions=[{}, {}         ], [a] };\n"
                "    key <34> { [NoAction(), NoAction() ], type = \"TWO_LEVEL\" };\n"
                "    key <35> { actions=[{}, {}         ], type = \"TWO_LEVEL\" };\n"
                "    key <38> { [NoAction(), NoAction() ], type = \"FOUR_LEVEL_SEMIALPHABETIC\" };\n"
                "    key <39> { actions=[{}, {}         ], type = \"FOUR_LEVEL_SEMIALPHABETIC\" };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "symbols-multi-keysyms-empty.xkb"
        }
    };
    for (size_t k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%zu ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap,
                                   strlen(keymaps[k].keymap),
                                   keymaps[k].expected,
                                   update_output_files));
    }
#undef make_keymaps_with
#undef make_keymap

    /* V1 serialization */
    const char keymap_str[] =
        "xkb_keymap {\n"
        "  xkb_keycodes { <> = 1; };\n"
        "  xkb_compat { interpret A { action = {SetMods(), SetGroup()}; }; };\n"
        "  xkb_symbols  { key <> { [{SetMods(), SetGroup()}] }; };\n"
        "};";
    assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                               XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                               compile_buffer, NULL, __func__,
                               keymap_str, ARRAY_SIZE(keymap_str),
                               GOLDEN_TESTS_OUTPUTS "symbols-multi-actions-v1.xkb",
                               update_output_files));
}

/* Test keysyms as strings */
static void
test_key_keysyms_as_strings(struct xkb_context *ctx, bool update_output_files)
{
    const struct keymap_test_data keymaps[] = {
        /* Invalid UTF-8 encoding */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 10; };\n"
                "  xkb_symbols {\n"
                /* Invalid byte at string index 2 */
                "    key <> { [\"\xC3\xBC\xff\"] };\n"
                "  };\n"
                "};",
            .expected = NULL
        },
        /* Valid */
        {
            .keymap =
                u8"xkb_keymap {\n"
                u8"  xkb_keycodes {\n"
                u8"    <10> = 10;\n"
                u8"    <11> = 11;\n"
                u8"    <12> = 12;\n"
                u8"    <20> = 20;\n"
                u8"    <21> = 21;\n"
                u8"    <22> = 22;\n"
                u8"    <23> = 23;\n"
                u8"    <24> = 24;\n"
                u8"    <25> = 25;\n"
                u8"    <30> = 30;\n"
                u8"    <31> = 31;\n"
                u8"    <32> = 32;\n"
                u8"    <33> = 33;\n"
                u8"    <34> = 34;\n"
                u8"    <35> = 35;\n"
                u8"    <40> = 40;\n"
                u8"    <41> = 41;\n"
                u8"    <42> = 42;\n"
                u8"    <50> = 50;\n"
                u8"    <51> = 51;\n"
                u8"    <52> = 52;\n"
                u8"    <60> = 60;\n"
                u8"    <61> = 61;\n"
                u8"    <62> = 62;\n"
                u8"    <63> = 63;\n"
                u8"    <64> = 64;\n"
                u8"    <65> = 65;\n"
                u8"    <66> = 66;\n"
                u8"    <67> = 67;\n"
                u8"    <68> = 68;\n"
                u8"    <69> = 69;\n"
                u8"    <70> = 70;\n"
                u8"    <71> = 71;\n"
                u8"    <72> = 72;\n"
                u8"    <73> = 73;\n"
                u8"    <74> = 74;\n"
                u8"    <AD08> = 80;\n"
                u8"    <AC05> = 81;\n"
                u8"    <AB05> = 82;\n"
                u8"    <AD01> = 83;\n"
                u8"  };\n"
                u8"  xkb_types { include \"basic\" };\n"
                u8"  xkb_compat {\n"
                u8"   interpret.action = SetMods();\n"
                u8"   interpret \"ä\"           {};\n"
                u8"   interpret \"✨\"          {};\n"
                u8"   interpret \"🎺\"          {};\n"
                u8"   interpret \"\\u{1F54A}\"  {};\n"
                u8"   interpret \"\\u{1}\"      {};\n"
                u8"   interpret \"\\u{a}\"      {};\n"
                u8"   interpret \"\\u{1f}\"     {};\n"
                u8"   interpret \"\\u{20}\"     {};\n"
                u8"   interpret \"\\u{7e}\"     {};\n"
                u8"   interpret \"\\u{7f}\"     {};\n"
                u8"   interpret \"\\u{80}\"     {};\n"
                u8"   interpret \"\\u{9f}\"     {};\n"
                u8"   interpret \"\\u{a0}\"     {};\n"
                u8"   interpret \"\\u{ff}\"     {};\n"
                // u8"   interpret \"\\u{d800}\"   {};\n"
                // u8"   interpret \"\\u{dfff}\"   {};\n"
                u8"   interpret \"\\u{fdd0}\"   {};\n"
                u8"   interpret \"\\u{fdef}\"   {};\n"
                u8"   interpret \"\\u{fffe}\"   {};\n"
                u8"   interpret \"\\u{ffff}\"   {};\n"
                u8"   interpret \"\\u{10000}\"  {};\n"
                u8"   interpret \"\\u{1ffff}\"  {};\n"
                u8"   interpret \"\\u{10ffff}\" {};\n"
                u8"  };\n"
                u8"  xkb_symbols {\n"
                /* Empty string */
                u8"    key <10> { [\"\", {b, \"\", c}] };\n"
                u8"    key <11> { [{a, \"\"}, {b, \"\"}] };\n"
                u8"    key <12> { [{\"\"}, {\"\", \"\"}] };\n"
                /* Single string: Plain */
                u8"    key <20> { [\"a\", \"bc\"] };\n"
                u8"    key <23> { [\"✨\", \"🎺\"] };\n" // U+2728 ✨, U+1F3BA 🎺
                u8"    key <24> { [\"u\u0308\"] };\n" // u + U+0308 ◌̈ COMBINING DIAERESIS
                u8"    key <25> { [\"∀∂∈ℝ∧∪≡∞ ↑↗↨↻⇣ ┐┼╔╘░►☺♀ ﬁ�⑀₂ἠḂӥẄɐː⍎אԱა\"] };\n"
                /* Single string: Nested */
                u8"    key <30> { [{\"a\"      }, {\"bc\"      }] };\n"
                u8"    key <31> { [{\"a\", \"\"}, {\"bc\", \"\"}] };\n"
                u8"    key <32> { [{\"\", \"a\"}, {\"\", \"bc\"}] };\n"
                u8"    key <33> { [{\"\\u{1F54A}\"}, {\"\\u{1f3f3}\\u{fe0f}\"}] };\n" // U+1F54A 🕊️, 🏳️
                u8"    key <34> { [{\"u\\u{0308}\"}] };\n" // u + U+0308 ◌̈ COMBINING DIAERESIS
                u8"    key <35> { [{\"∀\\u{0}∂∈ℝ∧∪≡∞ ↑↗↨↻⇣ ┐┼╔╘░►☺♀ ﬁ�⑀₂ἠḂӥẄɐː⍎אԱა\"}] };\n"
                /* Multi: string, literal */
                u8"    key <40> { [{\"a\",       b}, {\"cde\",       f}] };\n"
                u8"    key <41> { [{\"a\", \"\", b}, {\"cde\", \"\", f}] };\n"
                u8"    key <42> { [{\"a\", b, \"\"}, {\"cde\", f, \"\"}] };\n"
                /* Multi: literal, string */
                u8"    key <50> { [{a,       \"b\"}, {c,       \"def\"}] };\n"
                u8"    key <51> { [{a, \"\", \"b\"}, {c, \"\", \"def\"}] };\n"
                u8"    key <52> { [{a, \"b\", \"\"}, {c, \"def\", \"\"}] };\n"
                /* Multi: string, string */
                u8"    key <60> { [{\"a\",       \"b\"}, {\"cd\",       \"ef\"}] };\n"
                u8"    key <61> { [{\"a\", \"\", \"b\"}, {\"cd\", \"\", \"ef\"}] };\n"
                u8"    key <63> { [{\"a\",       \"bcd\"}, {\"efg\",       \"h\"}] };\n"
                u8"    key <64> { [{\"a\", \"\", \"bcd\"}, {\"efg\", \"\", \"h\"}] };\n"
                /* Some special Unicode code points */
                u8"    key <65> { [\"\\u{0}\", \"\\u{10ffff}\"] };\n"
                u8"    key <66> { [\"\\u{1}\", \"\\u{a}\"] };\n"
                u8"    key <67> { [\"\\u{1f}\", \"\\u{20}\"] };\n"
                u8"    key <68> { [\"\\u{7e}\", \"\\u{7f}\"] };\n"
                u8"    key <69> { [\"\\u{80}\", \"\\u{9f}\"] };\n"
                u8"    key <70> { [\"\\u{a0}\", \"\\u{ff}\"] };\n"
                u8"    key <71> { [\"\\u{d800}\", \"\\u{dfff}\"] };\n"
                u8"    key <72> { [\"\\u{fdd0}\", \"\\u{fdef}\"] };\n"
                u8"    key <73> { [\"\\u{fffe}\", \"\\u{ffff}\"] };\n"
                u8"    key <74> { [\"\\u{10000}\", \"\\u{1ffff}\"] };\n"
                /* Example from the doc */
                u8"    key <AD08> { [ \"ij\" , \"Ĳ\"   ] }; // IJ Dutch digraph\n"
                u8"    key <AC05> { [ \"g̃\"  , \"G̃\"   ] }; // G̃ Guarani letter\n"
                /* NOTE: We use U+200E LEFT-TO-RIGHT MARK in order to display the strings in
                 *       in the proper order. */
                u8"    key <AB05> { [ \"لا\"‎  , \"لآ\"‎   ] }; // ⁧لا⁩ ⁧لآ⁩ Arabic Lam-Alef ligatures decomposed\n"
                u8"    key <AD01> { [ \"c’h\", \"C’h\" ] }; // C’H Breton trigraph\n"
                u8"    modifier_map Mod1 { \"✨\", \"\\u{1F54A}\" };\n"
                u8"  };\n"
                u8"};",
            .expected = GOLDEN_TESTS_OUTPUTS "string-as-keysyms.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_invalid_symbols_fields(struct xkb_context *ctx)
{
    /* Any of the following is invalid syntax, but also use to trigger
     * a NULL pointer deference, thus this regression test */
    const char * const keymaps[] = {
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_symbols { key <> { vmods = [] }; };\n"
        "};",
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_symbols { key <> { repeat = [] }; };\n"
        "};",
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_symbols { key <> { type = [] }; };\n"
        "};",
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_symbols { key <> { groupswrap = [] }; };\n"
        "};",
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_symbols { key <> { groupsredirect = [] }; };\n"
        "};",
        /* Used to parse without error because the “repeats” key field is valid,
         * but we should fail in the following 2 keymaps */
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_symbols { key <> { vmods=[], repeats=false }; };\n"
        "};",
        "xkb_keymap {\n"
        "    xkb_keycodes { <> = 9; };\n"
        "    xkb_symbols { key <> { repeats=false, vmods=[] }; };\n"
        "};"
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        const struct xkb_keymap *keymap =
            test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                keymaps[k], strlen(keymaps[k]));
        assert(!keymap);
    }
}

static void
test_modifier_maps(struct xkb_context *ctx, bool update_output_files)
{
    /* Only accept key and keysyms in the modifier_map list */
    struct keymap_test_data keymaps[] = {
        {
            .keymap =
                u8"xkb_keymap {\n"
                u8"  xkb_keycodes {\n"
                u8"    <CAPS> = 66;\n"
                u8"    alias <LOCK> = <CAPS>;\n"
                u8"    <0> = 0;"
                u8"    <1> = 1;"
                u8"    <2> = 2;"
                u8"    <3> = 3;"
                u8"    <4> = 4;"
                u8"    <5> = 5;"
                u8"    <6> = 6;"
                u8"    <7> = 7;"
                u8"    <any>  = 10;"
                u8"    <none> = 11;"
                u8"    <a> = 61;"
                u8"    <b> = 62;"
                u8"    <c> = 63;"
                u8"    <100> = 100;"
                u8"  };\n"
                u8"  xkb_types { include \"basic\" };\n"
                u8"  xkb_symbols {\n"
                u8"    key <CAPS> { [Caps_Lock] };\n"
                u8"    key <any>  { [any, A] };\n"
                u8"    key <none> { [none, N] };\n"
                u8"    key <0>    { [0] };\n"
                u8"    key <1>    { [1] };\n"
                u8"    key <2>    { [2] };\n"
                u8"    key <a>    { [a] };\n"
                u8"    key <b>    { [b] };\n"
                u8"    key <c>    { [\"🎺\"] };\n"
                u8"    key <3>    { [NotAKeysym, 3] };\n"
                u8"    key <4>    { [\"\\u{0000001F54A}\"]};\n"
                u8"    key <5>    { [\"\\u{d800}\", \"\\u{dfff}\"]};\n"
                u8"    key <6>    { [\"\\u{fdd0}\", \"\\u{fdef}\"]};\n"
                u8"    key <7>    { [\"\\u{fffe}\", \"\\u{ffff}\"]};\n"
                u8"    key <100>  { [C] };\n"
                u8"    modifier_map Lock {\n"
                u8"      <100>, <LOCK>, any, none,\n"
                u8"      0, 1, 0x2, a, \"b\", \"🎺\", \"\\u{1F54A}\",\n"
                u8"      \"\\u{d800}\", \"\\u{dfff}\",\n"
                u8"      \"\\u{fdd0}\", \"\\u{fdef}\",\n"
                u8"      \"\\u{fffe}\", \"\\u{ffff}\",\n"
                u8"      NotAKeysym\n"
                u8"    };\n"
                u8"  };\n"
                u8"};",
            .expected = GOLDEN_TESTS_OUTPUTS "symbols-modifier_map.xkb"
        },
        /* Invalid: empty string */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_symbols { modifier_map Lock { \"\" }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid Unicode encoding */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_symbols { modifier_map Lock { \"\xff\" }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid single Unicode code point */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_symbols { modifier_map Lock { \"\\u{0}\" }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid multiple Unicode code points */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_symbols { modifier_map Lock { \"ab\" }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid type: list */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_symbols { modifier_map Lock { [a] }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid type: list */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_symbols { modifier_map Lock { {a, b} }; };\n"
                "};",
            .expected = NULL
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_empty_compound_statements(struct xkb_context *ctx, bool update_output_files)
{
    struct keymap_test_data keymaps[] = {
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <Q> = 24;\n"
                "    <W> = 25;\n"
                "    <E> = 26;\n"
                "    <R> = 27;\n"
                "    <T> = 28;\n"
                "    <Y> = 29;\n"
                "    <U> = 30;\n"
                "    <I> = 31;\n"
                "    <O> = 32;\n"
                "    <P> = 33;\n"
                "    <A> = 38;\n"
                "    <S> = 39;\n"
                "    <D> = 40;\n"
                "    <F> = 41;\n"
                "    <G> = 42;\n"
                "    <H> = 43;\n"
                "    <Z> = 52;\n"
                "    <X> = 53;\n"
                "    <C> = 54;\n"
                "    <V> = 55;\n"
                "    <B> = 56;\n"
                "    <N> = 57;\n"
                "    <M> = 58;\n"
                "  };\n"
                "  xkb_types {\n"
                "    type \"t1\" {};\n"
                "    type \"t2\" { modifiers = Shift; map[Shift] = 2; };\n"
                "  };\n"
                "  xkb_compat {\n"
                "    virtual_modifiers M1, M2;\n"
                "    indicator \"xxx\" {};\n"
                "    indicator.modifiers = Shift;"
                "    indicator \"yyy\" {};\n"
                "\n"
                "    interpret q {};\n"
                "    interpret.repeat = true;\n"
                "    interpret w {};\n"
                "  };\n"
                "  xkb_symbols {\n"
                "    key <Q> { [q] };\n"
                "    key <W> { [SetMods()] };\n"
                "    key <E> { [e], type = \"t1\" };\n"
                /* Empty key */
                "    key <R> {};\n"
                /* Empty key, same as previous */
                "    key <Y> { [] };\n"
                /* Override empty key with another */
                "    key <T> {};\n"
                "    key <T> { [] };\n"
                /* Override empty key with some symbols */
                "    key <U> {};\n"
                "    key <U> { [], [u] };\n"
                /* Override non-empty key with an empty key */
                "    key <I> { [i] };\n"
                "    key <I> {};\n"
                /* Empty symbols/actions */
                "    key <O> { [NoSymbol] };\n"
                "    key <P> { [NoAction()] };\n"
                /* No symbols nor actions: other properties */
                "    key <A> { vmods = M1 };\n"
                "    key <S> { repeat = false };\n"
                "    key <D> { type = \"t2\" };\n"
                "    key <F> { [], type = \"t2\" };\n"
                "    key <G> { type[1] = \"t2\" };\n"
                "    key <H> { type[1] = \"t1\", type[2] = \"t2\" };\n"
                /* Test key defaults / modmaps */
                "    key <Z> {};\n"
                "    key.vmods = M1;\n"
                "    key <X> {};\n"
                "    key <C> { vmods = M2 };\n"
                "    key.type = \"t2\";\n"
                "    key <V> { vmods = 0 };\n"
                "    key <B> { [], vmods = 0 };\n"
                "    key.type[1] = \"t1\";\n"
                "    key <N> { vmods = 0 };\n"
                "    key <M> { [], [], vmods = 0 };\n"
                "    modmap Shift   { <Z> };\n"
                "    modmap Lock    { <X> };\n"
                "    modmap Control { <C> };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "empty-compound-statements.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_escape_sequences(struct xkb_context *ctx, bool update_output_files)
{
    /* Similarly to `test_integers`, test that escape sequences at the end of
     * a buffer raise a syntax error but no memory violation */
    const char bad_octal[] = { '"', '\\', '1' };
    assert(!test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                bad_octal, sizeof(bad_octal)));
    const char bad_unicode[] = { '"', '\\', 'u', '{', '1' };
    assert(!test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                bad_unicode, sizeof(bad_unicode)));

    const char keymap[] =
        "default xkb_keymap \"\" {\n"
        "  xkb_keycodes "
        "\"\\u{0}La paix est la seule\\tbataille "
        "qui vaille la peine d\\u{02019}\\u{Ea}tre men\\303\\251e.\\n\" {\n"
        "    <> = 1;\n"
        "    indicator 1 = \"\\0\\n\\u{2328}\\u{fe0f}\";\n"
        "    indicator 2 = \"surrogates: \\u{d800} \\u{dfff}\";\n"
        "    indicator 3 = \"noncharacters: \\u{fdd0} \\u{fdef} \\u{fffe} \\u{ffff}\";\n"
        "    indicator 4 = \"noncharacters: \\u{1fffe} \\u{1ffff} \\u{10fffe} \\u{10ffff}\";\n"
        "    indicator 5 = \"out of range: \\u{0} a \\u{110000} \\u{ffffffffffff}\";\n"
        "    indicator 6 = \"invalid: \\u a \\uA b \\u{} c \\u{ d \\u} e \\u{1\";\n"
        "    indicator 7 = \"invalid: \\u{1234x\\\" y \";\n"
        "    indicator 8 = \"invalid: \\u{ 2} x \\u{3 } y\";\n"
	    "    indicator 9 = \"\\u{+1} \\u{-1} \\u{x} \\u{#} \\u{\\0} \\u{\\}\";\n"
        "  };\n"
        "  xkb_types \"\\00001\\\\\\00427\\u{22}\\r\\n\" {\n"
        "    type \"\\0\\00451\\u{1F3BA}\\u{2728}\\u{01F54a}\\u{0fE0f}\\f\" {\n"
        "      modifiers = Shift;\n"
        "      map[Shift] = 2;\n"
        "      level_name[1] = "
        "\"O\\u{f9} ils ont fait un \\u{22}d\\303\\251sert\\u{22}, \\e"
        "ils disent qu\\u{002019}ils \\12ont fait la \\42paix\\042.\\b\\n\";\n"
        "      level_name[2] = "
        "\"\\u{0000}Science \\u{73}\\141ns conscience "
        "n\\u{2019}est que rui\\\\ne\\u{A} de l\\u{02019}\\u{E2}me.\\r\";\n"
        "    };\n"
        "  };\n"
        "  xkb_compat "
        "\"Le c\\u{153}ur a \\v ses raisons "
        "que la raison ne con\\u{5C}na\\u{EE}t point\" {\n"
        "    indicator \"\\n\\u{2328}\\0\\u{fe0f}\" { modifiers = Shift; };\n"
        "  };\n"
        "  xkb_symbols "
        "\"La libert\\u{00e9} commence "
        "o\\u{0000f9} l\\342\\200\\231ignorance finit.\" {\n"
        "    name[1] = \"\\n\\0377\\3760\";\n"
        "    name[2] = \"\\00427\";\n"
        "    key <> {\n"
        "      symbols[1]=[a, A],\n"
        "      type[1]=\"%1\\u{1F3BA}\\u{2728}\\u{00001F54a}\\u{0fE0f}\\u{0C}\",\n"
        "      actions[2]=[Private(type=123, data=\"\0abcdefg\")]"
        "    };\n"
        "  };\n"
        "};";
    const char expected[] = GOLDEN_TESTS_OUTPUTS "escape-sequences.xkb";
    assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                               XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                               compile_buffer, NULL, __func__,
                               keymap, ARRAY_SIZE(keymap),
                               expected, update_output_files));
}

static void
test_unicode_keysyms(struct xkb_context *ctx, bool update_output_files)
{
    struct keymap_test_data keymaps[] = {
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { include \"evdev\" };\n"
                "  xkb_types { include \"basic\" };\n"
                "  xkb_symbols {\n"
                /* C0 Control characters */
                "    key <AE01> { [U0000, 0x01000000 ] };\n"
                "    key <AE02> { [U0001, 0x01000001 ] };\n"
                "    key <AE03> { [U000A, 0x0100000a ] };\n"
                "    key <AE04> { [U001F, 0x0100001f ] };\n"
                /* Printable ASCII characters */
                "    key <AE05> { [U0020, 0x01000020 ] };\n"
                "    key <AE06> { [U007E, 0x0100007e ] };\n"
                /* C0/C1 control characters */
                "    key <AE07> { [U007F, 0x0100007f ] };\n"
                "    key <AE08> { [U0080, 0x01000080 ] };\n"
                "    key <AE09> { [U009F, 0x0100009f ] };\n"
                /* Latin-1 printable characters */
                "    key <AE10> { [U00A0, 0x010000a0 ] };\n"
                "    key <AE11> { [U00FF, 0x010000ff ] };\n"
                /* Misc: bounds */
                "    key <AD01> { [U0100, 0x01000100 ] };\n"
                "    key <AD02> { [UD7FF, 0x0100d7ff ] };\n"
                /* Surrogates */
                "    key <AD03> { [UD800, 0x0100d800 ] };\n"
                "    key <AD04> { [UDFFF, 0x0100dfff ] };\n"
                /* Misc: bounds */
                "    key <AD05> { [UE000, 0x0100e000 ] };\n"
                "    key <AD06> { [UFDCF, 0x0100fdcf ] };\n"
                /* Noncharacters */
                "    key <AD07> { [UFDD0, 0x0100fdd0 ] };\n"
                "    key <AD08> { [UFDEF, 0x0100fdef ] };\n"
                /* Misc: bounds */
                "    key <AD09> { [UFDF0, 0x0100fdf0 ] };\n"
                "    key <AD10> { [UFFFD, 0x0100fffd ] };\n"
                /* Noncharacters */
                "    key <AD11> { [UFFFE, 0x0100fffe ] };\n"
                "    key <AD12> { [UFFFF, 0x0100ffff ] };\n"
                /* Misc: bounds */
                "    key <AC01> { [U10000, 0x01010000 ] };\n"
                /* Noncharacters */
                "    key <AC08> { [U1FFFE , 0x0101fffe ] };\n"
                "    key <AC09> { [U1FFFF , 0x0101ffff ] };\n"
                "    key <AC10> { [U10FFFE, 0x0110fffe ] };\n"
                /* Max Unicode */
                "    key <AC11> { [U10FFFF, 0x0110ffff ] };\n"
                /* Max Unicode + 1 */
                "    key <AC12> { [U110000, 0x01110000 ] };\n"
                /* Misc */
                "    key <AB01> { [U0174, 0x01000174 ] };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "unicode-keysyms.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_no_action_void_action(struct xkb_context *ctx, bool update_output_files)
{
    const char keymap_str[] =
        "xkb_keymap {\n"
        "  xkb_keycodes { <1> = 1; <2> = 2; <3> = 3; };\n"
        "  xkb_symbols {\n"
        /* These actions take no argument */
        "   key <1> { [x], [NoAction(mods=1)] };\n"
        "   key <2> { [y], [VoidAction(mods=1)] };\n"
        /* Check that we can overwrite NoAction, but not the contrary */
        "   key <3> { [NoAction()] };\n"
        "   key <3> { [VoidAction()] };\n"
        "   key <3> { [NoAction()] };\n"
        "  };\n"
        "};";
    /* v1 → v1 */
    assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                               XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                               compile_buffer, NULL, __func__,
                               keymap_str, sizeof(keymap_str),
                               GOLDEN_TESTS_OUTPUTS "no_void_action-v1.xkb",
                               update_output_files));
    /* v1 → v2 */
    assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                               XKB_KEYMAP_FORMAT_TEXT_V2,
                               compile_buffer, NULL, __func__,
                               keymap_str, sizeof(keymap_str),
                               GOLDEN_TESTS_OUTPUTS "no_void_action-v2.xkb",
                               update_output_files));
    /* v2 → v2 */
    assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                               XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                               compile_buffer, NULL, __func__,
                               keymap_str, sizeof(keymap_str),
                               GOLDEN_TESTS_OUTPUTS "no_void_action-v2.xkb",
                               update_output_files));
    /* v2 → v1 */
    assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                               XKB_KEYMAP_FORMAT_TEXT_V1,
                               compile_buffer, NULL, __func__,
                               keymap_str, sizeof(keymap_str),
                               GOLDEN_TESTS_OUTPUTS "no_void_action-v1.xkb",
                               update_output_files));
}

static void
test_prebuilt_keymap_roundtrip(struct xkb_context *ctx, bool update_output_files)
{
    /* Load in a prebuilt keymap, make sure we can compile it from memory,
     * then compare it to make sure we get the same result when dumping it
     * to a string. */
    static const struct {
        const char* path;
        enum xkb_keymap_format format;
        enum xkb_keymap_serialize_flags serialize_flags;
    } data[] = {
        {
            .path = "keymaps/stringcomp-v1.xkb",
            .format = XKB_KEYMAP_FORMAT_TEXT_V1,
            .serialize_flags = TEST_KEYMAP_SERIALIZE_FLAGS
        },
        {
            .path = "keymaps/stringcomp-v1-no-prettyfied.xkb",
            .format = XKB_KEYMAP_FORMAT_TEXT_V1,
            .serialize_flags = TEST_KEYMAP_SERIALIZE_FLAGS
                             & ~XKB_KEYMAP_SERIALIZE_PRETTY
        },
        {
            .path = "keymaps/stringcomp-v2.xkb",
            .format = XKB_KEYMAP_FORMAT_TEXT_V2,
            .serialize_flags = TEST_KEYMAP_SERIALIZE_FLAGS
        },
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(data); k++) {
        char *original = test_read_file(data[k].path);
        assert(original);
        /* Load a prebuild keymap, once without, once with the trailing \0 */
        for (unsigned int i = 0; i <= 1; i++) {
            assert(test_compile_output2(ctx, data[k].format,
                                        XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                        data[k].serialize_flags,
                                        compile_buffer, NULL, "Round-trip",
                                        original, strlen(original) + i,
                                        data[k].path, update_output_files));
        }
        free(original);
    }
}

static void
test_keymap_from_rules(struct xkb_context *ctx)
{
    /* Make sure we can recompile our output for a normal keymap from rules. */
    fprintf(stderr, "------\n*** %s ***\n", __func__);
    struct xkb_keymap *keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                                   NULL, NULL,
                                                   "ru,ca,de,us",
                                                   ",multix,neo,intl", NULL);
    assert(keymap);
    char *dump = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    assert(dump);
    xkb_keymap_unref(keymap);
    keymap = test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                 dump, strlen(dump));
    assert(keymap);
    xkb_keymap_unref(keymap);
    free(dump);
}

static void
test_redirect_key(struct xkb_context *ctx, bool update_output_files)
{
    const struct keymap_test_data tests[] = {
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <A> = 38; <S> = 39; <D> = 40; <F> = 41; <G> = 42;};\n"
                "  xkb_symbols {\n"
                /* Inexistent key */
                "    key <A> { [a], [RedirectKey(key=<?>)] };\n"
                /* Multiple RedirectKey() on the same level */
                "    key <S> { [s], [RedirectKey(key=<A>), RedirectKey(key=<D>)] };\n"
                /* OK! */
                "    key <D> { [d], [RedirectKey(key=<S>,mods=Shift,clearMods=Control)] };\n"
                /* No parameters (implicit auto) */
                "    key <F> { [f], [RedirectKey()] };\n"
                /* Explicit auto keycode */
                "    key <G> { [g], [RedirectKey(keycode=auto)] };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "redirect-key-1.xkb"
        },
    };
    for (unsigned int t = 0; t < ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, t);
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   tests[t].keymap, strlen(tests[t].keymap),
                                   tests[t].expected, update_output_files));
    }
}

static void
test_unsupported_legacy_x11_actions(struct xkb_context *ctx,
                                    bool update_output_files)
{
    const char keymap_str[] =
        "xkb_keymap {\n"
        "  xkb_keycodes {\n"
        "    <1> = 1;\n"
        "    <2> = 2;\n"
        "    <3> = 3;\n"
        "    <4> = 4;\n"
        "    <5> = 5;\n"
        "  };\n"
        "  xkb_compat {\n"
        "    ISOLock.modifiers = modMapMods;\n"
        "    DeviceButton.data = <1>;\n" /* invalid field */
        "    LockDeviceButton.data = <1>;\n" /* invalid field */
        "    DeviceValuator.data = <1>;\n" /* invalid field */
        "    ActionMessage.data = <1>;\n" /* invalid field */
        "    interpret ISO_Lock {\n"
        "      action=ISOLock(affect=all);\n"
        "    };\n"
        "    interpret 0x10000 {\n"
        "      action=DeviceButton(data=all);\n" /* invalid field */
        "    };\n"
        "    interpret 0x10001 {\n"
        "      action=LockDeviceButton(data=all);\n" /* invalid field */
        "    };\n"
        "    interpret 0x10002 {\n"
        "      action=DeviceValuator(data=all);\n" /* invalid field */
        "    };\n"
        "    interpret 0x10003 {\n"
        "      action=ActionMessage(data=all);\n" /* invalid field */
        "    };\n"
        "  };\n"
        "  xkb_symbols {\n"
        "   key <1> { [ISOLock(affect=all)] };\n"
        "   key <2> { [DeviceButton(data=<1>)] };\n" /* invalid field */
        "   key <3> { [LockDeviceButton(data=<1>)] };\n" /* invalid field */
        "   key <4> { [DeviceValuator(data=<1>)] };\n" /* invalid field */
        "   key <5> { [ActionMessage(data=<1>)] };\n" /* invalid field */
        "  };\n"
        "};";
    assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                               XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                               compile_buffer, NULL, __func__,
                               keymap_str, sizeof(keymap_str),
                               GOLDEN_TESTS_OUTPUTS "unsupported-x11-actions",
                               update_output_files));
}

static void
test_extended_layout_indices(struct xkb_context *ctx,
                             bool update_output_files)
{
    const struct {
        const char* keymap;
        bool no_output;
        union {
            struct { bool compiles_v1; bool compiles_v2; };
            struct {
                const char* expected_v1;
                const char* expected_v1_2;
                const char* expected_v2;
                const char* expected_v2_1;
            };
        };
    } tests[] = {
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat { interpret a { action = SetGroup(group=5); }; };\n"
                "};",
            .no_output = true,
            .compiles_v1 = false,
            .compiles_v2 = true,
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { indicator 1 = \"a\"; };\n"
                "  xkb_compat   { indicator \"a\" { groups=All-5; }; };\n"
                "};",
            .no_output = true,
            .compiles_v1 = true,
            .compiles_v2 = true,
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols  { name[5] = \"5\"; };\n"
                "};",
            .no_output = true,
            .compiles_v1 = false,
            .compiles_v2 = true,
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols  { key <> { [1], [2], [3], [4], [5] }; };\n"
                "};",
            .no_output = true,
            .compiles_v1 = false,
            .compiles_v2 = true,
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <a> = 1;\n"
                "    <b> = 2;\n"
                "    <c> = 3;\n"
                "    indicator 1 = \"a\";\n"
                "  };\n"
                "  xkb_compat {\n"
                "    interpret a { action = SetGroup(group=5); };\n"
                "    indicator \"a\" { groups=All-5; };\n"
                "  };\n"
                "  xkb_symbols {\n"
                "    name[5] = \"G5\";\n"
                "    name[1] = \"G1\";\n"
                "    key <a> { symbols[1]=[a], symbols[5]=[Greek_alpha] };\n"
                "    key <b> { actions[1]=[Latchgroup(group=-5)], actions[5]=[LockGroup(group=5)], [SetGroup(+9)] };\n"
                "    key <c> { [1], [2], [3], [4], [5], [6], [7], [8], [9] };\n"
                "  };\n"
                "};",
            .expected_v1 = NULL,
            .expected_v1_2 = NULL,
            .expected_v2 = GOLDEN_TESTS_OUTPUTS "extended-layout-indices-v2.xkb",
            .expected_v2_1 = GOLDEN_TESTS_OUTPUTS "extended-layout-indices-v1.xkb",
        }
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        if (tests[k].no_output) {
            struct xkb_keymap *keymap =
                test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                    tests[k].keymap, strlen(tests[k].keymap));
            assert(!!keymap == tests[k].compiles_v1);
            xkb_keymap_unref(keymap);
            keymap =
                test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                                    tests[k].keymap, strlen(tests[k].keymap));
            assert(!!keymap == tests[k].compiles_v2);
            xkb_keymap_unref(keymap);
            continue;
        }
        /* v1 → v1 */
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   tests[k].keymap, strlen(tests[k].keymap),
                                   tests[k].expected_v1, update_output_files));
        /* v1 → v2 */
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_FORMAT_TEXT_V2,
                                   compile_buffer, NULL, __func__,
                                   tests[k].keymap, strlen(tests[k].keymap),
                                   tests[k].expected_v1_2, update_output_files));
        /* v2 → v2 */
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                                   XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                   compile_buffer, NULL, __func__,
                                   tests[k].keymap, strlen(tests[k].keymap),
                                   tests[k].expected_v2, update_output_files));
        /* v2 → v1 */
        assert(test_compile_output(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                                   XKB_KEYMAP_FORMAT_TEXT_V1,
                                   compile_buffer, NULL, __func__,
                                   tests[k].keymap, strlen(tests[k].keymap),
                                   tests[k].expected_v2_1, update_output_files));
    }
}

int
main(int argc, char *argv[])
{
    test_init();

    bool update_output_files = false;
    /* Default seed for pseudo-random generator */
    unsigned int seed = (unsigned int) time(NULL);

    int arg_index = 0;
    while (++arg_index < argc) {
        if (streq(argv[arg_index], "update")) {
            /* Update files with *obtained* results */
            update_output_files = true;
        } else if (streq(argv[arg_index], "--seed") && arg_index + 1 < argc) {
            seed = (unsigned int) atoi(argv[arg_index + 1]);
        } else {
            fprintf(stderr, "ERROR: unsupported argument: \"%s\".\n",
                    argv[arg_index]);
            exit(EXIT_INVALID_USAGE);
        }
    }

    /* Initialize pseudo-random generator with program arg or current time */
    fprintf(stderr, "Seed for the pseudo-random generator: %u\n", seed);
    srand(seed);

    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    /* Reject unsupported flags */
    static const char buf[] = "xkb_keymap {};";
    assert(!xkb_keymap_new_from_buffer(ctx, buf, sizeof(buf),
                                       XKB_KEYMAP_FORMAT_TEXT_V1, -1));
    assert(!xkb_keymap_new_from_buffer(ctx, buf, sizeof(buf),
                                       XKB_KEYMAP_FORMAT_TEXT_V1, 0xffff));

    /* Make sure we can't (falsely claim to) compile an empty string. */
    struct xkb_keymap *keymap =
        test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "", 0);
    assert(!keymap);

    test_encodings(ctx);
    test_floats(ctx);
    test_component_syntax_error(ctx);
    test_optional_components(ctx, update_output_files);
    test_bidi_chars(ctx, update_output_files);
    test_recursive_includes(ctx);
    test_include_paths(ctx);
    test_include_default_maps(update_output_files);
    test_alloc_limits(ctx, update_output_files);
    test_integers(ctx, update_output_files);
    test_keycodes(ctx, update_output_files);
    test_masks(ctx, update_output_files);
    test_interpret(ctx, update_output_files);
    test_group_indices_names(ctx, update_output_files);
    test_level_indices_names(ctx, update_output_files);
    test_multi_keysyms_actions(ctx, update_output_files);
    test_key_keysyms_as_strings(ctx, update_output_files);
    test_invalid_symbols_fields(ctx);
    test_modifier_maps(ctx, update_output_files);
    test_empty_compound_statements(ctx, update_output_files);
    test_escape_sequences(ctx, update_output_files);
    test_unicode_keysyms(ctx, update_output_files);
    test_no_action_void_action(ctx, update_output_files);
    test_prebuilt_keymap_roundtrip(ctx, update_output_files);
    test_keymap_from_rules(ctx);
    test_redirect_key(ctx, update_output_files);
    test_unsupported_legacy_x11_actions(ctx, update_output_files);
    test_extended_layout_indices(ctx, update_output_files);

    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
