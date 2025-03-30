/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"
#include "merge_modes.h"

/* Our keymap compiler is the xkbcommon buffer compiler */
static struct xkb_keymap *
compile_buffer(struct xkb_context *context, const char *buf, size_t len,
                     void *private)
{
    return test_compile_buffer(context, buf, len);
}

static void
test_compat(struct xkb_context *ctx, enum update_files update_output_files)
{
    // Github Issue #566
    const char keymap_str[] =
        "xkb_keymap {\n"
        "  xkb_compat {\n"
        "    interpret A { repeat = true; };\n"
        "    interpret A { repeat = true; };\n"
        "    interpret A { action = SetMods(mods=Mod1); };\n"
        "    interpret B { repeat = true; };\n"
        "    interpret B { repeat = true; };\n"
        "    augment interpret B { action = SetMods(mods=Mod1); };\n"
        "    interpret C { repeat = true; };\n"
        "    interpret C { repeat = true; };\n"
        "    override interpret C { action = SetMods(mods=Mod1); };\n"
        "    interpret D { repeat = true; };\n"
        "    interpret D { repeat = true; };\n"
        "    replace interpret D { action = SetMods(mods=Mod1); };\n"
        "\n"
	    "    indicator \"A\" { modifiers=Shift; };\n"
	    "    indicator \"A\" { modifiers=Lock; };\n"
	    "    indicator \"A\" { groups= Group1; };\n"
	    "    indicator \"B\" { modifiers=Shift; };\n"
	    "    indicator \"B\" { modifiers=Lock; };\n"
	    "    augment indicator \"B\" { groups=Group1; };\n"
	    "    indicator \"C\" { modifiers=Shift; };\n"
	    "    indicator \"C\" { modifiers=Lock; };\n"
	    "    override indicator \"C\" { groups=Group1; };\n"
	    "    indicator \"D\" { modifiers=Shift; };\n"
	    "    indicator \"D\" { modifiers=Lock; };\n"
	    "    replace indicator \"D\" { groups=Group1; };\n"
        "  };\n"
        "};\n";

    assert(test_compile_output(ctx, compile_buffer, NULL,
                               "test_merge_mode: compat",
                               keymap_str, ARRAY_SIZE(keymap_str),
                               GOLDEN_TESTS_OUTPUTS "compat.xkb",
                               !!update_output_files));
}

static void
test_symbols(struct xkb_context *ctx, enum update_files update_output_files)
{
    make_symbols_tests("merge_modes", "", "",
                       compile_buffer, NULL, update_output_files);
}

int
main(int argc, char *argv[])
{
    test_init();

    /* Check if we run the tests or just update their outputs */
    enum update_files update_output_files = NO_UPDATE;
    if (argc > 1) {
        if (streq(argv[1], "update")) {
            /* Update files with *expected* results */
            update_output_files = UPDATE_USING_TEST_INPUT;
        } else if (streq(argv[1], "update-obtained")) {
            /* Update files with *obtained* results */
            update_output_files = UPDATE_USING_TEST_OUTPUT;
        } else {
            fprintf(stderr, "ERROR: unsupported argument: \"%s\".\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    test_compat(ctx, update_output_files);
    test_symbols(ctx, update_output_files);

    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
