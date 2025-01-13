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

enum update_files {
    NO_UPDATE = 0,
    UPDATE_USING_TEST_INPUT,
    UPDATE_USING_TEST_OUTPUT
};

#define GOLDEN_TESTS_OUTPUTS "keymaps/merge-modes/"

#ifdef XKBCOMP_COMPATIBILITY
#define xkb_keycodes_body "\n    include\"evdev\""
#define xkb_types_body "    include \"numpad\"\n"
#define xkb_compat_body                                                    \
    "    interpret Any + Any { action=SetMods(modifiers=modMapMods); };\n" \
    "    indicator \"XXX\" { !allowExplicit; };\n"
#define xkb_symbols_body "    key <I255> { [End] };\n"
#else
#define xkb_keycodes_body
#define xkb_types_body
#define xkb_compat_body
#define xkb_symbols_body
#endif


#define keymap_common                                                         \
    "xkb_keymap {\n"                                                          \
    "  xkb_keycodes { include \"merge_modes\"" xkb_keycodes_body " };\n"      \
    "  xkb_types {\n"                                                         \
    "    include \"basic\"\n"                                                 \
    /* Add only used levels; `types/extra` has some types uneeded here */     \
    "    virtual_modifiers LevelThree;\n"                                     \
    "    type \"FOUR_LEVEL\" {\n"                                             \
	"        modifiers = Shift+LevelThree;\n"                                 \
	"        map[None] = Level1;\n"                                           \
	"        map[Shift] = Level2;\n"                                          \
	"        map[LevelThree] = Level3;\n"                                     \
	"        map[Shift+LevelThree] = Level4;\n"                               \
	"        level_name[Level1] = \"Base\";\n"                                \
	"        level_name[Level2] = \"Shift\";\n"                               \
	"        level_name[Level3] = \"Alt Base\";\n"                            \
	"        level_name[Level4] = \"Shift Alt\";\n"                           \
    "    };\n"                                                                \
    "    type \"FOUR_LEVEL_ALPHABETIC\" {\n"                                  \
	"        modifiers = Shift+Lock+LevelThree;\n"                            \
	"        map[None] = Level1;\n"                                           \
	"        map[Shift] = Level2;\n"                                          \
	"        map[Lock]  = Level2;\n"                                          \
	"        map[LevelThree] = Level3;\n"                                     \
	"        map[Shift+LevelThree] = Level4;\n"                               \
	"        map[Lock+LevelThree] =  Level4;\n"                               \
	"        map[Lock+Shift+LevelThree] =  Level3;\n"                         \
	"        level_name[Level1] = \"Base\";\n"                                \
	"        level_name[Level2] = \"Shift\";\n"                               \
	"        level_name[Level3] = \"Alt Base\";\n"                            \
	"        level_name[Level4] = \"Shift Alt\";\n"                           \
    "    };\n"                                                                \
    "    type \"FOUR_LEVEL_SEMIALPHABETIC\" {\n"                              \
	"        modifiers = Shift+Lock+LevelThree;\n"                            \
	"        map[None] = Level1;\n"                                           \
	"        map[Shift] = Level2;\n"                                          \
	"        map[Lock]  = Level2;\n"                                          \
	"        map[LevelThree] = Level3;\n"                                     \
	"        map[Shift+LevelThree] = Level4;\n"                               \
	"        map[Lock+LevelThree] =  Level3;\n"                               \
	"        map[Lock+Shift+LevelThree] = Level4;\n"                          \
	"        preserve[Lock+LevelThree] = Lock;\n"                             \
	"        preserve[Lock+Shift+LevelThree] = Lock;\n"                       \
	"        level_name[Level1] = \"Base\";\n"                                \
	"        level_name[Level2] = \"Shift\";\n"                               \
	"        level_name[Level3] = \"Alt Base\";\n"                            \
	"        level_name[Level4] = \"Shift Alt\";\n"                           \
    "    };\n"                                                                \
         xkb_types_body                                                       \
    "  };\n"                                                                  \
    "  xkb_compat {" xkb_compat_body "};\n"

/* Helper to create a keymap string to initialize output files */
#define make_ref_keymap(file, map, suffix)           \
    keymap_common                                    \
    "  xkb_symbols {\n"                              \
    "    include \"" file "(" map suffix ")\"\n"     \
         xkb_symbols_body                            \
    "  };\n"                                         \
    "};"

/* Helper to create a keymap string to test */
#define make_test_keymap(file, map, merge_mode)                               \
    keymap_common                                                             \
    "  xkb_symbols {\n"                                                       \
    /*                                                                        \
     * NOTE: Separate statements so that *all* the merge modes *really* work. \
     *       Using + and | separators downgrades `replace key` to `override/  \
     *       augment key`.                                                    \
     */                                                                       \
    "    include \"" file "(" map "base)\"\n"                                 \
    "    " merge_mode " \"" file "(" map "new)\"\n"                           \
         xkb_symbols_body                                                     \
    "  };\n"                                                                  \
    "};"

/* Helper to create a test for a single keymap string */
#define make_symbols_test(merge_mode, file, map, map_suffix, suffix, compile, priv, update) do {\
    const char keymap_ref_str[] = make_ref_keymap(file, map, map_suffix);                       \
    const char keymap_test_str[] = make_test_keymap(file, map, merge_mode);                     \
    assert(test_compile_output(                                                                 \
        ctx, compile, priv,                                                                     \
        "test_merge_mode: " map ", " merge_mode,                                                \
        (update == UPDATE_USING_TEST_INPUT) ? keymap_ref_str : keymap_test_str,                 \
        (update == UPDATE_USING_TEST_INPUT) ? ARRAY_SIZE(keymap_ref_str)                        \
                                            : ARRAY_SIZE(keymap_test_str),                      \
        GOLDEN_TESTS_OUTPUTS merge_mode "-" map suffix ".xkb",                                  \
        !!update));                                                                             \
} while (0)

/* Helper to create a test for each merge mode */
#define make_symbols_tests(file, map, suffix, compile, priv, update)                     \
    /* Mode: Default */                                                                  \
    make_symbols_test("include", file, map, "override", suffix, compile, priv, update);  \
    /* Mode: Augment */                                                                  \
    make_symbols_test("augment", file, map, "augment", suffix, compile, priv, update);   \
    /* Mode: Override */                                                                 \
    make_symbols_test("override", file, map, "override", suffix, compile, priv, update); \
    /* Mode: Replace */                                                                  \
    make_symbols_test("replace", file, map, "new", suffix, compile, priv, update)
