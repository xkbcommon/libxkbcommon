/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "features/enums.h"
#include "test-config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon.h"
#include "test.h"
#include "utils.h"

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

static const struct {
    const char* name;
    enum xkb_keymap_compile_flags flags;
    bool fail;
} modes[] = {
    {
        "strict",
        (TEST_KEYMAP_COMPILE_FLAGS | XKB_KEYMAP_COMPILE_STRICT_MODE),
        true
    },
    {
        "lax",
        (TEST_KEYMAP_COMPILE_FLAGS & ~XKB_KEYMAP_COMPILE_STRICT_MODE),
        false
    },
};

/* Our keymap compiler is the xkbcommon buffer compiler */
static struct xkb_keymap *
compile_buffer(struct xkb_context *context, enum xkb_keymap_format format,
               const char *buf, size_t len, void *private)
{
    return test_compile_buffer2(context, format,
                                *(enum xkb_keymap_compile_flags*)private,
                                buf, len);
}

static void
test_unknown_declaration_statements(struct xkb_context *ctx, bool update_files)
{
    static const struct keymap_test_data tests[] = {
        /*
         * Keycodes
         */

        {
            .keymap = "default xkb_keymap { xkb_keycodes { xxx \"1\" = \"xxx\"; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_keycodes { xxx 1 = yyy; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_keycodes { xxx 1.23 = Group1; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_keycodes { xxx <> = 1 + 1; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },

        /*
         * Types
         */

        {
            .keymap = "default xkb_keymap { xkb_types { xxx \"1\" = \"xxx\"; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_types { xxx 1 = yyy; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_types { xxx 1.23 = Group1; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_types { xxx <> = 1 + 1; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },

        /*
         * Compat
         */

        {
            .keymap = "default xkb_keymap { xkb_compat { xxx \"1\" = \"xxx\"; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_compat { xxx 1 = yyy; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_compat { xxx 1.23 = Group1; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_compat { xxx <> = 1 + 1; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },

        /*
         * Symbols
         */

        {
            .keymap = "default xkb_keymap { xkb_symbols { xxx \"1\" = \"xxx\"; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_symbols { xxx 1 = yyy; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_symbols { xxx 1.23 = Group1; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_symbols { xxx <> = 1 + 1; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        for (size_t m = 0; m < ARRAY_SIZE(modes); m++) {
            assert(test_compile_output(
                ctx, XKB_KEYMAP_FORMAT_TEXT_V2, XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                compile_buffer, (void*)&modes[m].flags, modes[m].name,
                tests[k].keymap, strlen(tests[k].keymap),
                (modes[m].fail ? NULL : tests[k].expected), update_files
            ));
        }
    }
}

static void
test_unknown_compound_statements(struct xkb_context *ctx, bool update_files)
{
    static const struct keymap_test_data tests[] = {
        /*
         * Keycodes
         */

        {
            .keymap = "default xkb_keymap { xkb_keycodes { xxx {}; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_keycodes { xxx \"yyy\" {}; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_keycodes { xxx.yyy = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },

        /*
         * Types
         */

        {
            .keymap = "default xkb_keymap { xkb_types { xxx { yyy = 1; }; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_types { xxx 1 { yyy[1] = true; }; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_types { xxx.yyy = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },

        /*
         * Compat
         */

        {
            .keymap = "default xkb_keymap { xkb_compat { xxx 1.23 {}; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_compat { xxx.yyy = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },

        /*
         * Symbols
         */

        {
            .keymap = "default xkb_keymap { xkb_symbols { xxx <> { a[<>] = {}; }; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_symbols { xxx.yyy = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        for (size_t m = 0; m < ARRAY_SIZE(modes); m++) {
            assert(test_compile_output(
                ctx, XKB_KEYMAP_FORMAT_TEXT_V2, XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                compile_buffer, (void*)&modes[m].flags, modes[m].name,
                tests[k].keymap, strlen(tests[k].keymap),
                (modes[m].fail ? NULL : tests[k].expected), update_files
            ));
        }
    }
}

static void
test_unknown_fields(struct xkb_context *ctx, bool update_files)
{
    static const struct keymap_test_data tests[] = {
        /*
         * Keycodes
         */

        {
            .keymap = "default xkb_keymap { xkb_keycodes { xxx = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },

        /*
         * Types
         */

        {
            .keymap = "default xkb_keymap { xkb_types { xxx = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_types { type \"1\" { xxx = 0; }; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-types-unknown-type-field.xkb"
        },

        /*
         * Compat
         */

        {
            .keymap = "default xkb_keymap { xkb_compat { xxx = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_compat { interpret A { xxx = 0; }; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-compat-unknown-interpret-field.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_compat { indicator \"1\" { xxx = 0; }; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-compat-unknown-indicator-field.xkb"
        },

        /*
         * Symbols
         */

        {
            .keymap = "default xkb_keymap { xkb_symbols { xxx = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols { key <> { xxx = 0, [none], [NoAction()] }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-symbols-unknown-key-field.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        for (size_t m = 0; m < ARRAY_SIZE(modes); m++) {
            assert(test_compile_output(
                ctx, XKB_KEYMAP_FORMAT_TEXT_V2, XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                compile_buffer, (void*)&modes[m].flags, modes[m].name,
                tests[k].keymap, strlen(tests[k].keymap),
                (modes[m].fail ? NULL : tests[k].expected), update_files
            ));
        }
    }
}

static void
test_unknown_values(struct xkb_context *ctx, bool update_files)
{
    static const struct keymap_test_data tests[] = {
        /*
         * Keycodes
         */

        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    minimum = xxx;\n"
                "    maximum = \"x\";\n"
                "    maximum[0] = 1;\n"
                "  };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },

        /*
         * Types
         */

        // TODO: types fields
        // {
        //     .keymap =
        //         "default xkb_keymap {\n"
        //         "  xkb_types {};\n"
        //         "};",
        //     .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        // },

        /*
         * Compat
         */

        // TODO: interpret & LEDs fields
        // {
        //     .keymap =
        //         "default xkb_keymap {\n"
        //         "  xkb_compat {\n"
        //         "    interpret. = \"\";\n"
        //         "  };\n"
        //         "};",
        //     .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        // },

        /*
         * Symbols
         */

        // {
        //     .keymap =
        //         "default xkb_keymap {\n"
        //         "  xkb_symbols {};\n"
        //         "};",
        //     .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        // },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        for (size_t m = 0; m < ARRAY_SIZE(modes); m++) {
            assert(test_compile_output(
                ctx, XKB_KEYMAP_FORMAT_TEXT_V2, XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                compile_buffer, (void*)&modes[m].flags, modes[m].name,
                tests[k].keymap, strlen(tests[k].keymap),
                (modes[m].fail ? NULL : tests[k].expected), update_files
            ));
        }
    }
}

static void
test_actions(struct xkb_context *ctx, bool update_files)
{
    static const struct keymap_test_data tests[] = {
        /*
         * Compat
         */

        {
            .keymap = "default xkb_keymap { xkb_compat { SetMods.xxx = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_compat { SetMods.mods = xxx; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_compat { SetMods.mods = \"0\"; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_compat { SetMods.mods[0] = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_compat { XXX.xxx = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_compat { interpret A { action = XXX(); }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-compat-unknown-interpret-field.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_compat { interpret A { action = XXX(xxx=0); }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-compat-unknown-interpret-field.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_compat { interpret A { action = SetMods(xxx=0); }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-compat-unknown-action-field.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_compat { interpret A { action = SetMods(mods=xxx); }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-compat-unknown-action-value.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_compat { interpret A { action = SetMods(mods=\"0\"); }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-compat-unknown-action-value.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_compat { interpret A { action = SetMods(mods[0]=0); }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-compat-unknown-action-value.xkb"
        },

        /*
         * Symbols
         */

        {
            .keymap = "default xkb_keymap { xkb_symbols { SetMods.xxx = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_symbols { SetMods.mods = xxx; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_symbols { SetMods.mods = \"0\"; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_symbols { SetMods.mods[0] = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap = "default xkb_keymap { xkb_symbols { XXX.xxx = 0; }; };",
            .expected = GOLDEN_TESTS_OUTPUTS "optional-components-none.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols { key <> { [none], [XXX()] }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-symbols-unknown-key-field.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols { key <> { [none], [XXX(xxx=0)] }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-symbols-unknown-key-field.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols { key <> { [none], [SetMods(xxx=0)] }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-symbols-unknown-action-field.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols { key <> { [none], [SetMods(mods=xxx)] }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-symbols-unknown-action-value.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols { key <> { [none], [SetMods(mods=\"0\")] }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-symbols-unknown-action-value.xkb"
        },
        {
            .keymap =
                "default xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "  xkb_symbols { key <> { [none], [SetMods(mods[0]=0)] }; };\n"
                "};",
            .expected = GOLDEN_TESTS_OUTPUTS "lax-symbols-unknown-action-value.xkb"
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        for (size_t m = 0; m < ARRAY_SIZE(modes); m++) {
            assert(test_compile_output(
                ctx, XKB_KEYMAP_FORMAT_TEXT_V2, XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                compile_buffer, (void*)&modes[m].flags, modes[m].name,
                tests[k].keymap, strlen(tests[k].keymap),
                (modes[m].fail ? NULL : tests[k].expected), update_files
            ));
        }
    }
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

    test_unknown_declaration_statements(ctx, update_output_files);
    test_unknown_compound_statements(ctx, update_output_files);
    test_unknown_fields(ctx, update_output_files);
    test_unknown_values(ctx, update_output_files);
    test_actions(ctx, update_output_files);

    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
