/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"
#include "utils.h"

#define GOLDEN_TESTS_OUTPUTS "keymaps/"

struct keymap_test_data {
    /** Keymap string to compile */
    const char * const keymap;
    /** Resulting file *path* to reference serialization,
     *  or NULL if the keymap string should not compile. */
    const char * const expected;
};

/* Our keymap compiler is the xkbcommon buffer compiler */
static struct xkb_keymap *
compile_buffer(struct xkb_context *context, const char *buf, size_t len,
                     void *private)
{
    return test_compile_buffer(context, buf, len);
}

static void
test_encodings(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    /* Accept UTF-8 encoded BOM (U+FEFF) */
    const char utf8_with_bom[] = "\xef\xbb\xbfxkb_keymap {};";
    keymap = test_compile_buffer(ctx, utf8_with_bom, sizeof(utf8_with_bom));
    assert(keymap);
    xkb_keymap_unref(keymap);

    /* Reject UTF-16LE encoded string */
    const char utf16_le[] = "x\0k\0b\0_\0k\0e\0y\0m\0a\0p\0 \0{\0}\0;\0";
    keymap = test_compile_buffer(ctx, utf16_le, sizeof(utf16_le));
    assert(!keymap);

    /* Reject UTF-16LE with BOM encoded string */
    const char utf16_le_with_bom[] =
        "\xff\xfex\0k\0b\0_\0k\0e\0y\0m\0a\0p\0 \0{\0}\0;\0";
    keymap = test_compile_buffer(ctx, utf16_le_with_bom, sizeof(utf16_le_with_bom));
    assert(!keymap);

    /* Reject UTF-16BE encoded string */
    const char utf16_be[] = "\0x\0k\0b\0_\0k\0e\0y\0m\0a\0p\0 \0{\0}\0;";
    keymap = test_compile_buffer(ctx, utf16_be, sizeof(utf16_be));
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
            test_compile_buffer(ctx, tests[k].keymap, strlen(tests[k].keymap));
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
            test_compile_buffer(ctx, keymaps[k], strlen(keymaps[k]));
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
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_recursive(struct xkb_context *ctx)
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
        keymap = test_compile_buffer(ctx, keymaps[k], strlen(keymaps[k]));
        assert(!keymap);
    }
}

/* Test some limits related to allocations */
static void
test_alloc_limits(struct xkb_context *ctx)
{
    const char * const keymaps[] = {
        /* Keycodes */
        "xkb_keymap {\n"
        /* Valid keycode value, but we should not handle it
         * with our *continuous* array! */
        "  xkb_keycodes { <> = 0xfffffffe; };\n"
        "  xkb_symbols { key <> {[a]}; };\n"
        "};",
        /* Key types */
        "xkb_keymap {\n"
        "  xkb_types {\n"
        "    type \"X\" { map[none] = 0xfffffffe; };\n" /* Invalid level index */
        "  };\n"
        "};",
        "xkb_keymap {\n"
        "  xkb_types {\n"
        "    type \"X\" {levelname[0xfffffffe]=\"x\";};\n" /* Invalid level index */
        "  };\n"
        "};"
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        const struct xkb_keymap *keymap =
            test_compile_buffer(ctx, keymaps[k], strlen(keymaps[k]));
        assert(!keymap);
    }
}

static void
test_integers(struct xkb_context *ctx, bool update_output_files) {
    /* Using a buffer without a terminating NULL. The following is a obvious
     * syntax error, but it should still exit *cleanly*, not as previously where
     * the use of strtoll would trigger a memory violation. */
    const char not_null_terminated[] = { '1' };
    assert(!test_compile_buffer(ctx, not_null_terminated,
                                sizeof(not_null_terminated)));

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
                /* FIXME: Unchecked overflow */
                "      [MovePointer(x=0xffffffff + 1,\n"
                "                   y=0x80000000 * 2)]\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "integers-overflow.xkb"
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
        }
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
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
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
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
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
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
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
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
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
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
                "    <B> = 1;\n"
                "    augment  <B> = 300;\n"
                "    override <A> = 1;\n"
                "  };\n"
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
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
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
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
                "  xkb_compat {\n"
                "    interpret.repeat= False;\n"
                "  };\n"
                "};",
            /* NOTE: same as previous */
            .expected = GOLDEN_TESTS_OUTPUTS "keycodes-bounds-multiple-1.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
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
        }
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
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
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   keymaps[k].keymap,
                                   strlen(keymaps[k].keymap),
                                   keymaps[k].expected,
                                   update_output_files));
    }
#undef make_keymaps_with
#undef make_keymap
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
            test_compile_buffer(ctx, keymaps[k], strlen(keymaps[k]));
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
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <CAPS> = 66;\n"
                "    alias <LOCK> = <CAPS>;\n"
                "    <0> = 0;"
                "    <1> = 1;"
                "    <2> = 2;"
                "    <3> = 3;"
                "    <any>  = 10;"
                "    <none> = 11;"
                "    <a> = 61;"
                "    <100> = 100;"
                "  };\n"
                "  xkb_types { include \"basic\" };\n"
                "  xkb_symbols {\n"
                "    key <CAPS> { [Caps_Lock] };\n"
                "    key <any>  { [any, A] };\n"
                "    key <none> { [none, N] };\n"
                "    key <0>    { [0] };\n"
                "    key <1>    { [1] };\n"
                "    key <2>    { [2] };\n"
                "    key <a>    { [a] };\n"
                "    key <3>    { [NotAKeysym, 3] };\n"
                "    key <100>  { [C] };\n"
                "    modifier_map Lock {\n"
                "      <100>, <LOCK>, any, none,\n"
                "      0, 1, 0x2, a, NotAKeysym\n"
                "    };\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "symbols-modifier_map.xkb"
        },
        /* Invalid: string (length = 0) */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_symbols { modifier_map Lock { \"\" }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid: string (length = 1) */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_symbols { modifier_map Lock { \"a\" }; };\n"
                "};",
            .expected = NULL
        },
        /* Invalid: string (length > 1) */
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
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
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
        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   keymaps[k].keymap, strlen(keymaps[k].keymap),
                                   keymaps[k].expected, update_output_files));
    }
}

static void
test_escape_sequences(struct xkb_context *ctx, bool update_output_files)
{
    const char keymap[] =
        "default xkb_keymap \"\" {\n"
        "  xkb_keycodes "
        "\"\\0La paix est la seule\\tbataille "
        "qui vaille la peine d\\342\\200\\231\\303\\252tre men\\303\\251e.\\n\" {\n"
        "    <> = 1;\n"
        "    indicator 1 = \"\\0\\n\\342\\214\\250\\357\\270\\217\";\n"
        "  };\n"
        "  xkb_types \"\\0\\61\\\\\\0427\\r\\n\" {\n"
        "    type \"\\0\\45\\61\\360\\237\\216\\272\\342\\234\\250\\360\\237\\225\\212\\357\\270\\217\\f\" {\n"
        "      modifiers = Shift;\n"
        "      map[Shift] = 2;\n"
        "      level_name[1] = "
        "\"O\\303\\271 ils ont fait un d\\303\\251sert, \\e"
        "ils disent qu\\342\\200\\231ils \\12ont fait la \\42paix\\42.\\b\\n\";\n"
        "      level_name[2] = "
        "\"\\0Science \\163\\141ns conscience "
        "n\\342\\200\\231est que rui\\\\ne\\12 de l\\342\\200\\231\\303\\242me.\\r\";\n"
        "    };\n"
        "  };\n"
        "  xkb_compat "
        "\"Le c\\305\\223ur a \\v ses raisons "
        "que la raison ne con\\134na\\303\\251t point\" {\n"
        "    indicator \"\\n\\342\\214\\250\\0\\357\\270\\217\" { modifiers = Shift; };\n"
        "  };\n"
        "  xkb_symbols "
        "\"La libert\\303\\251 commence "
        "o\\303\\271 l\\342\\200\\231ignorance finit.\" {\n"
        "    name[1] = \"\\n\";\n"
        "    name[2] = \"\\0427\";\n"
        "    key <> {\n"
        "      symbols[1]=[a, A],\n"
        "      type[1]=\"%1\\360\\237\\216\\272\\342\\234\\250\\360\\237\\225\\212\\357\\270\\217\\14\",\n"
        "      actions[2]=[Private(type=123, data=\"\0abcdefg\")]"
        "    };\n"
        "  };\n"
        "};";
    const char expected[] = GOLDEN_TESTS_OUTPUTS "escape-sequences.xkb";
    assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                               keymap, ARRAY_SIZE(keymap),
                               expected, update_output_files));
}

static void
test_prebuilt_keymap_roundtrip(struct xkb_context *ctx, bool update_output_files)
{
    /* Load in a prebuilt keymap, make sure we can compile it from memory,
     * then compare it to make sure we get the same result when dumping it
     * to a string. */
    const char * const path = GOLDEN_TESTS_OUTPUTS "stringcomp.data";
    char *original = test_read_file(path);
    assert(original);

    /* Load a prebuild keymap, once without, once with the trailing \0 */
    for (unsigned int i = 0; i <= 1; i++) {
        fprintf(stderr, "------\n*** %s, trailing '\\0': %d ***\n", __func__, i);

        assert(test_compile_output(ctx, compile_buffer, NULL, __func__,
                                   original, strlen(original) + i, path,
                                   update_output_files));
    }

    free(original);
}

static void
test_keymap_from_rules(struct xkb_context *ctx)
{
    /* Make sure we can recompile our output for a normal keymap from rules. */
    fprintf(stderr, "------\n*** %s ***\n", __func__);
    struct xkb_keymap *keymap = test_compile_rules(ctx, NULL, NULL,
                                                   "ru,ca,de,us",
                                                   ",multix,neo,intl", NULL);
    assert(keymap);
    char *dump = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    assert(dump);
    xkb_keymap_unref(keymap);
    keymap = test_compile_buffer(ctx, dump, strlen(dump));
    assert(keymap);
    xkb_keymap_unref(keymap);
    free(dump);
}

int
main(int argc, char *argv[])
{
    test_init();

    bool update_output_files = false;
    if (argc > 1) {
        if (streq(argv[1], "update")) {
            /* Update files with *obtained* results */
            update_output_files = true;
        } else {
            fprintf(stderr, "ERROR: unsupported argument: \"%s\".\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    /* Make sure we can't (falsely claim to) compile an empty string. */
    struct xkb_keymap *keymap = test_compile_buffer(ctx, "", 0);
    assert(!keymap);

    test_encodings(ctx);
    test_floats(ctx);
    test_component_syntax_error(ctx);
    test_optional_components(ctx, update_output_files);
    test_recursive(ctx);
    test_alloc_limits(ctx);
    test_integers(ctx, update_output_files);
    test_keycodes(ctx, update_output_files);
    test_masks(ctx, update_output_files);
    test_multi_keysyms_actions(ctx, update_output_files);
    test_invalid_symbols_fields(ctx);
    test_modifier_maps(ctx, update_output_files);
    test_empty_compound_statements(ctx, update_output_files);
    test_escape_sequences(ctx, update_output_files);
    test_prebuilt_keymap_roundtrip(ctx, update_output_files);
    test_keymap_from_rules(ctx);

    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
