/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
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

/* Use macro sorcery */
#include "src/messages-codes.h"

enum update_files {
    NO_UPDATE = 0,
    UPDATE_USING_TEST_INPUT,
    UPDATE_USING_TEST_OUTPUT
};

#define GOLDEN_TESTS_OUTPUTS "keymaps/merge-modes/"

/* Macro sorcery: see "src/messages-codes.h" */
#define MATCHdefault             unused,default
#define CHECK_MERGE_MODE(value)  SECOND(JOIN(MATCH, value), other, unused)

#define FORMAT_MERGE_MODE_global_default(mode) "include"
#define FORMAT_MERGE_MODE_global_other(mode)   #mode
#define FORMAT_MERGE_MODE_global(mode) \
        JOIN(FORMAT_MERGE_MODE_global_, CHECK_MERGE_MODE(mode))(mode)

/******************************************************************************
 * General tests
 ******************************************************************************/

/* Helper to create a keymap string to initialize output files */
#define make_ref_keymap(file, map, suffix)                       \
    "xkb_keymap {\n"                                             \
    "  xkb_keycodes { include \"" file "(" map suffix ")\" };\n" \
    "  xkb_types    { include \"" file "(" map suffix ")\" };\n" \
    "  xkb_compat   { include \"" file "(" map suffix ")\" };\n" \
    "  xkb_symbols  { include \"" file "(" map suffix ")\" };\n" \
    "};"

/* Helper to create a keymap string to test (global) */
#define make_test_keymap_global(file, map, merge_mode)                        \
    "xkb_keymap {\n"                                                          \
    /*                                                                        \
     * NOTE: Separate statements so that *all* the merge modes *really* work. \
     *       Using + and | separators downgrades `replace key` to `override/  \
     *       augment key`.                                                    \
     */                                                                       \
    "  xkb_keycodes {\n"                                                      \
    "    include \"" file "(" map "base)\"\n"                                 \
    "    " FORMAT_MERGE_MODE_global(merge_mode) " \"" file "(" map "new)\"\n" \
    "  };\n"                                                                  \
    "  xkb_types {\n"                                                         \
    "    include \"" file "(" map "base)\"\n"                                 \
    "    " FORMAT_MERGE_MODE_global(merge_mode) " \"" file "(" map "new)\"\n" \
    "  };\n"                                                                  \
    "  xkb_compat {\n"                                                        \
    "    include \"" file "(" map "base)\"\n"                                 \
    "    " FORMAT_MERGE_MODE_global(merge_mode) " \"" file "(" map "new)\"\n" \
    "  };\n"                                                                  \
    "  xkb_symbols {\n"                                                       \
    "    include \"" file "(" map "base)\"\n"                                 \
    "    " FORMAT_MERGE_MODE_global(merge_mode) " \"" file "(" map "new)\"\n" \
    "  };\n"                                                                  \
    "};"

/* Helper to create a keymap string to test (local) */
#define make_test_keymap_local(file, map, merge_mode)                        \
    "xkb_keymap {\n"                                                         \
    "  xkb_keycodes { include \"" file "(" map "new-" #merge_mode ")\" };\n" \
    "  xkb_types    { include \"" file "(" map "new-" #merge_mode ")\" };\n" \
    "  xkb_compat   { include \"" file "(" map "new-" #merge_mode ")\" };\n" \
    "  xkb_symbols  { include \"" file "(" map "new-" #merge_mode ")\" };\n" \
    "};"

/* Helper to create a test for a single keymap string */
#define make_test(localness, merge_mode, file, map, map_suffix,                \
                  file_suffix, compile, priv, update) do {                     \
    const char keymap_ref_str[] = make_ref_keymap(file, map, map_suffix);      \
    const char keymap_test_str[] =                                             \
        make_test_keymap_##localness(file, map, merge_mode);                   \
    assert(test_compile_output(                                                \
        ctx, compile, priv,                                                    \
        "test_merge_mode: " map ", " #localness " " #merge_mode,               \
        (update == UPDATE_USING_TEST_INPUT) ? keymap_ref_str : keymap_test_str,\
        (update == UPDATE_USING_TEST_INPUT) ? ARRAY_SIZE(keymap_ref_str)       \
                                            : ARRAY_SIZE(keymap_test_str),     \
        /* Local and global merge modes use the same result file */            \
        GOLDEN_TESTS_OUTPUTS #merge_mode map file_suffix ".xkb",               \
        !!update));                                                            \
} while (0)

/* Helper to create a test for each merge mode */
#define make_tests(file, map, suffix, compile, priv, update)                          \
    /* Mode: Default */                                                               \
    make_test(local, default, file, map, "override", suffix, compile, priv, update);  \
    make_test(global, default, file, map, "override", suffix, compile, priv, update); \
    /* Mode: Augment */                                                               \
    make_test(local, augment, file, map, "augment", suffix, compile, priv, update);   \
    make_test(global, augment, file, map, "augment", suffix, compile, priv, update);  \
    /* Mode: Override */                                                              \
    make_test(local, override, file, map, "override", suffix, compile, priv, update); \
    make_test(global, override, file, map, "override", suffix, compile, priv, update);\
    /* Mode: Replace */                                                               \
    make_test(local, replace, file, map, "replace", suffix, compile, priv, update);   \
    make_test(global, replace, file, map, "replace", suffix, compile, priv, update)
