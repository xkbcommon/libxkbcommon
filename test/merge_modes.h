/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */
#pragma once

/* Use macro sorcery */
#include "src/messages-codes.h"

enum update_files {
    NO_UPDATE = 0,
    UPDATE_USING_TEST_INPUT,
    UPDATE_USING_TEST_OUTPUT
};

#define GOLDEN_TESTS_OUTPUTS "keymaps/merge-modes/"

#define keymap_common                                                         \
    "xkb_keymap {\n"                                                          \
    "  xkb_keycodes { include \"merge_modes\" };\n"                           \
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
    "  };\n"                                                                  \
    "  xkb_compat { };\n"

/* Macro sorcery: see "src/messages-codes.h" */
#define MATCHdefault             unused,default
#define CHECK_MERGE_MODE(value)  SECOND(JOIN(MATCH, value), other, unused)

#define FORMAT_MERGE_MODE_global_default(mode) "include"
#define FORMAT_MERGE_MODE_global_other(mode)   #mode
#define FORMAT_MERGE_MODE_global(mode) \
        JOIN(FORMAT_MERGE_MODE_global_, CHECK_MERGE_MODE(mode))(mode)

/* Helper to create a keymap string to initialize output files */
#define make_ref_keymap(file, map, suffix)           \
    keymap_common                                    \
    "  xkb_symbols \"\" {\n"                         \
    "    include \"" file "(" map suffix ")\"\n"     \
    "  };\n"                                         \
    "};"

/* Helper to create a keymap string to test (global) */
#define make_test_keymap_global(file, map, merge_mode)                        \
    keymap_common                                                             \
    "  xkb_symbols \"\" {\n"                                                  \
    /*                                                                        \
     * NOTE: Separate statements so that *all* the merge modes *really* work. \
     *       Using + and | separators downgrades `replace key` to `override/  \
     *       augment key`.                                                    \
     */                                                                       \
    "    include \"" file "(" map "base)\"\n"                                 \
    "    " FORMAT_MERGE_MODE_global(merge_mode) " \"" file "(" map "new)\"\n" \
    "  };\n"                                                                  \
    "};"

/* Helper to create a keymap string to test (local) */
#define make_test_keymap_local(file, map, merge_mode)                         \
    keymap_common                                                             \
    "  xkb_symbols \"\" {\n"                                                  \
    /*                                                                        \
     * NOTE: Separate statements so that *all* the merge modes *really* work. \
     *       Using + and | separators downgrades `replace key` to `override/  \
     *       augment key`.                                                    \
     */                                                                       \
    "    include \"" file "(" map "new-" #merge_mode ")\"\n"                  \
    "  };\n"                                                                  \
    "};"

/* Helper to create a test for a single keymap string */
#define make_symbols_test(localness, merge_mode, file, map, map_suffix,                \
                          file_suffix, compile, priv, update) do {                     \
    const char keymap_ref_str[] = make_ref_keymap(file, map, map_suffix);              \
    const char keymap_test_str[] = make_test_keymap_##localness(file, map, merge_mode);\
    assert(test_compile_output(                                                        \
        ctx, compile, priv,                                                            \
        "test_merge_mode: " map ", " #localness " " #merge_mode,                       \
        ((update) == UPDATE_USING_TEST_INPUT) ? keymap_ref_str : keymap_test_str,      \
        ((update) == UPDATE_USING_TEST_INPUT) ? ARRAY_SIZE(keymap_ref_str)             \
                                              : ARRAY_SIZE(keymap_test_str),           \
        /* Local and global merge modes use the same result file */                    \
        GOLDEN_TESTS_OUTPUTS #merge_mode map file_suffix ".xkb",                       \
        !!(update)));                                                                  \
} while (0)

/* Helper to create a test for each merge mode */
#define make_symbols_tests(file, map, suffix, compile, priv, update)                          \
    /* Mode: Default */                                                                       \
    make_symbols_test(local, default, file, map, "override", suffix, compile, priv, update);  \
    make_symbols_test(global, default, file, map, "override", suffix, compile, priv, update); \
    /* Mode: Augment */                                                                       \
    make_symbols_test(local, augment, file, map, "augment", suffix, compile, priv, update);   \
    make_symbols_test(global, augment, file, map, "augment", suffix, compile, priv, update);  \
    /* Mode: Override */                                                                      \
    make_symbols_test(local, override, file, map, "override", suffix, compile, priv, update); \
    make_symbols_test(global, override, file, map, "override", suffix, compile, priv, update);\
    /* Mode: Replace */                                                                       \
    make_symbols_test(local, replace, file, map, "new", suffix, compile, priv, update);       \
    make_symbols_test(global, replace, file, map, "new", suffix, compile, priv, update)
