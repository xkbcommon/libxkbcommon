/*
 * Copyright © 2025 Pierre Le Marre
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "darray.h"
#include "utils-paths.h"
#include "xkbcommon/xkbcommon.h"
#include "src/xkbcomp/keymap-file-iterator.h"
#include "src/xkbcomp/ast.h"
#include "test.h"
#include "utils.h"

struct file_include_test_data {
    bool valid:1;
    bool explicit_section:1;
    enum merge_mode merge;
    const char *path; /* relative to test data */
    const char *file;
    const char *section;
    const char *modifier;
    enum xkb_map_flags flags;
};

struct section_test_data {
    const char *name;
    enum xkb_file_type file_type;
    enum xkb_map_flags flags;
    struct xkb_file_include_group include_groups[3];
    struct file_include_test_data includes[3];
    darray_size_t num_include_groups;
    darray_size_t num_includes;
};

static void
test_resolve_file(struct xkb_context *ctx)
{
    char *path;
    char resolved_path[PATH_MAX] = {0};
    char resolved_section[1024] = {0};
    FILE *file;

    /* Invalid paths */
    static const char invalid[] = "---invalid---";
    path = test_get_path(invalid);
    assert(path);
    assert(is_absolute_path(path));
    const char * paths[] = { path, invalid };
    static const char * sections[] = { NULL, "invalid" };
    /* Note: inferior or equal to _FILE_TYPE_NUM_ENTRIES is correct here */
    for (enum xkb_file_type t = 0; t <= _FILE_TYPE_NUM_ENTRIES; t++) {
        for (size_t p = 0; p < ARRAY_SIZE(paths); p++) {
            for (size_t s = 0; s < ARRAY_SIZE(sections); s++) {
                file = xkb_resolve_file(ctx, t, paths[p], sections[s],
                                        resolved_path, sizeof(resolved_path),
                                        resolved_section, sizeof(resolved_section));
                assert(!file);
            }
        }
    }
    free(path);

    /* Valid absolute path */
    path = test_get_path("types/numpad");
    assert(path);
    assert(is_absolute_path(path));

    const struct {
        const char *path;
        const char *section;
        const char *resolved_path;
        const char *resolved_section;
        enum xkb_file_type type;
        bool failure;
    } tests[] = {
        /* Absolute path */
        {
            .failure = false,
            .path = path,
            .section = NULL,
            .type = FILE_TYPE_INVALID,
            .resolved_path = path,
            .resolved_section = "pc" /* default */
        },
        {
            .failure = false,
            .path = path,
            .section = "shift3",
            .type = FILE_TYPE_INVALID,
            .resolved_path = path,
            .resolved_section = "shift3" /* non-default */
        },
        {
            .failure = true,
            .path = path,
            .section = NULL,
            .type = FILE_TYPE_SYMBOLS, /* invalid type */
            .resolved_path = NULL,
            .resolved_section = NULL
        },
        {
            .failure = false,
            .path = path,
            .section = NULL,
            .type = FILE_TYPE_TYPES, /* valid type */
            .resolved_path = path,
            .resolved_section = "pc" /* default */
        },
        {
            .failure = false,
            .path = path,
            .section = "shift3",
            .type = FILE_TYPE_TYPES, /* valid type */
            .resolved_path = path,
            .resolved_section = "shift3" /* non default */
        },

        /* Relative path */
        {
            .failure = true,
            .path = "numpad",
            .section = NULL,
            .type = FILE_TYPE_INVALID, /* type required */
            .resolved_path = NULL,
            .resolved_section = NULL
        },
        {
            .failure = true,
            .path = "numpad",
            .section = "shift3",
            .type = FILE_TYPE_INVALID, /* type required */
            .resolved_path = NULL,
            .resolved_section = NULL
        },
        {
            .failure = true,
            .path = "numpad",
            .section = NULL,
            .type = FILE_TYPE_KEYCODES, /* invalid type */
            .resolved_path = NULL,
            .resolved_section = NULL
        },
        {
            .failure = false,
            .path = "numpad",
            .section = NULL,
            .type = FILE_TYPE_TYPES, /* valid type */
            .resolved_path = path,
            .resolved_section = "pc" /* default */
        },
        {
            .failure = false,
            .path = "numpad",
            .section = "shift3",
            .type = FILE_TYPE_TYPES, /* valid type */
            .resolved_path = path,
            .resolved_section = "shift3" /* non default */
        },
    };
    for (size_t t = 0; t < ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: #%zu ***\n", __func__, t);
        file = xkb_resolve_file(ctx, tests[t].type,
                                tests[t].path, tests[t].section,
                                resolved_path, sizeof(resolved_path),
                                resolved_section, sizeof(resolved_section));
        if (tests[t].failure) {
            assert(!file);
        } else {
            assert(file);
            assert_streq_not_null("resolved path",
                                  resolved_path, tests[t].resolved_path);
            assert_streq_not_null("resolved section",
                                  resolved_section, tests[t].resolved_section);
        }
    }
    free(path);
}

static bool
test_section(const struct section_test_data *data,
             const struct xkb_file_section *section)
{
    assert(section->file_type == data->file_type);
    assert_streq_not_null("", data->name,
                          xkb_file_section_get_string(section, section->name));
    assert_eq("Section flags", data->flags, section->flags, "%#x");
    assert_eq("Num include groups", data->num_include_groups,
              darray_size(section->include_groups), "%u");
    assert_eq("Num includes", data->num_includes,
              darray_size(section->includes), "%u");
    for (darray_size_t k = 0; k < data->num_include_groups; k++) {
        fprintf(stderr, "... %s: include group #%u ...\n", __func__, k);
        const struct xkb_file_include_group * const expected = &data->include_groups[k];
        const struct xkb_file_include_group * const got = &darray_item(section->include_groups, k);
        assert_eq("Start", expected->start, got->start, "%d");
        assert_eq("End", expected->end, got->end, "%d");
    }
    for (darray_size_t k = 0; k < data->num_includes; k++) {
        fprintf(stderr, "... %s: include #%u ...\n", __func__, k);
        const struct file_include_test_data * const expected = &data->includes[k];
        const struct xkb_file_include * const got = &darray_item(section->includes, k);
        assert_eq("Valid", expected->valid, got->valid, "%d");
        assert_eq("Merge mode", expected->merge, got->merge, "%d");
        char *path = test_get_path(expected->path);
        assert(path);
        assert_streq_not_null("File", path,
                              xkb_file_section_get_string(section, got->path));
        free(path);
        assert_streq_not_null("File", expected->file,
                              xkb_file_section_get_string(section, got->file));
        assert_streq_not_null("Section", expected->section,
                              xkb_file_section_get_string(section, got->section));
        assert_eq("Explicit section",
                  expected->explicit_section, got->explicit_section, "%d");
        assert_streq_not_null("Modifier", expected->modifier,
                              xkb_file_section_get_string(section, got->modifier));
        assert_eq("Section flags", expected->flags, got->flags, "%#x");
    }
    return true;
}

static void
test_file_section_parse(struct xkb_context *ctx)
{
    char *path = test_get_path("symbols/pc");
    assert(path);

    struct xkb_file_section section = {0};

    struct section_test_data tests[] = {
        {
            .name = "editing",
            .file_type = FILE_TYPE_SYMBOLS,
            .flags = MAP_IS_HIDDEN | MAP_IS_PARTIAL | MAP_HAS_ALPHANUMERIC,
            .includes = {},
            .num_includes = 0
        },
        {
            .name = "pc105",
            .file_type = FILE_TYPE_SYMBOLS,
            .flags = MAP_IS_DEFAULT | MAP_IS_PARTIAL | MAP_HAS_ALPHANUMERIC
                   | MAP_HAS_MODIFIER,
            .num_include_groups = 1,
            .include_groups = {
                { .start = 0, .end = 0 },
            },
            .num_includes = 1,
            .includes = {
                {
                    .valid = true,
                    .merge = MERGE_DEFAULT,
                    .path = "symbols/pc",
                    .file = "pc",
                    .section = "pc105-pure-virtual-modifiers",
                    .explicit_section = true,
                    .modifier = "",
                    .flags = 0
                }
            },
        },
        {
            .name = "pc105-pure-virtual-modifiers",
            .file_type = FILE_TYPE_SYMBOLS,
            .flags = 0,
            .num_include_groups = 3,
            .include_groups = {
                { .start = 0, .end = 0 },
                { .start = 1, .end = 1 },
                { .start = 2, .end = 2 },
            },
            .num_includes = 3,
            .includes = {
                {
                    .valid = true,
                    .merge = MERGE_DEFAULT,
                    .path = "symbols/srvr_ctrl",
                    .file = "srvr_ctrl",
                    .section = "fkey2vt",
                    .explicit_section = true,
                    .modifier = "",
                    .flags = MAP_IS_PARTIAL | MAP_HAS_FN,
                },
                {
                    .valid = true,
                    .merge = MERGE_DEFAULT,
                    .path = "symbols/pc",
                    .file = "pc",
                    .section = "editing",
                    .explicit_section = true,
                    .modifier = "",
                    .flags = MAP_IS_HIDDEN | MAP_IS_PARTIAL
                           | MAP_HAS_ALPHANUMERIC,
                },
                {
                    .valid = true,
                    .merge = MERGE_DEFAULT,
                    .path = "symbols/keypad",
                    .file = "keypad",
                    .section = "x11",
                    .explicit_section = true,
                    .modifier = "",
                    .flags = MAP_IS_DEFAULT | MAP_IS_HIDDEN | MAP_IS_PARTIAL
                           | MAP_HAS_KEYPAD,
                }
            },
        }
    };

    for (size_t k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%zu ***\n", __func__, k);
        xkb_file_section_init(&section);
        const bool is_default = tests[k].flags & MAP_IS_DEFAULT;
        assert(xkb_file_section_parse(
            ctx, XKB_FILE_ITERATOR_FAIL_ON_INCLUDE_ERROR,
            XKB_KEYMAP_FORMAT_TEXT_V2, XKB_KEYMAP_COMPILE_NO_FLAGS,
            0, path, (is_default ? NULL : tests[k].name), &section
        ));
        test_section(&tests[k], &section);
        xkb_file_section_free(&section);
    }
    free(path);
}

struct iterator_test_data {
    const char *string;
    const char *map;
    struct section_test_data sections[3];
    unsigned int num_sections;
    bool error;
};

static void
test_file_iterator(struct xkb_context *ctx)
{
    static const struct iterator_test_data tests[] = {
        {
            .string = "",
            .map = NULL,
            .num_sections = 0,
            .sections = {},
            .error = false,
        },
        {
            .string = "xkb_symbols \"1\" {};",
            .map = NULL,
            .num_sections = 1,
            .sections = {
                {
                    .name = "1",
                    .file_type = FILE_TYPE_SYMBOLS,
                    .flags = 0,
                    .includes = {},
                    .num_includes = 0,
                }
            },
            .error = false,
        },
        {
            .string =
                "xkb_symbols \"1\" {\n"
                "  include \"pc\"\n"
                "  replace \"+de:1|cz:2\"\n"
                "};",
            .map = NULL,
            .num_sections = 1,
            .sections = {
                {
                    .name = "1",
                    .file_type = FILE_TYPE_SYMBOLS,
                    .flags = 0,
                    .num_include_groups = 2,
                    .include_groups = {
                        { .start = 0, .end = 0 },
                        { .start = 1, .end = 2 },
                    },
                    .num_includes = 3,
                    .includes = {
                        {
                            .valid = true,
                            .merge = MERGE_DEFAULT,
                            .path = "symbols/pc",
                            .file = "pc",
                            .section = "pc105",
                            .explicit_section = false,
                            .modifier = "",
                            .flags = MAP_IS_DEFAULT | MAP_IS_PARTIAL
                                    | MAP_HAS_ALPHANUMERIC | MAP_HAS_MODIFIER,
                        },
                        {
                            .valid = true,
                            /* First include uses the merge mode of the statement */
                            .merge = MERGE_REPLACE,
                            .path = "symbols/de",
                            .file = "de",
                            .section = "basic",
                            .explicit_section = false,
                            .modifier = "1",
                            .flags = MAP_IS_DEFAULT
                        },
                        {
                            .valid = true,
                            .merge = MERGE_AUGMENT,
                            .path = "symbols/cz",
                            .file = "cz",
                            .section = "basic",
                            .explicit_section = false,
                            .modifier = "2",
                            .flags = MAP_IS_DEFAULT | MAP_IS_PARTIAL
                                   | MAP_HAS_ALPHANUMERIC,
                        }
                    },
                }
            },
            .error = false,
        },
        {
            .string =
                "xkb_symbols \"1\" {};\n"
                "xkb_symbols \"2\" {};\n",
            .map = NULL,
            .num_sections = 2,
            .sections = {
                {
                    .name = "1",
                    .file_type = FILE_TYPE_SYMBOLS,
                    .flags = 0,
                    .num_include_groups = 0,
                    .include_groups = {},
                    .num_includes = 0,
                    .includes = {},
                },
                {
                    .name = "2",
                    .file_type = FILE_TYPE_SYMBOLS,
                    .flags = 0,
                    .num_include_groups = 0,
                    .include_groups = {},
                    .num_includes = 0,
                    .includes = {},
                },
            },
            .error = false,
        },
        {
            .string =
                "xkb_keymap \"1\" {\n"
                "  xkb_types \"2\" {};\n"
                "  xkb_symbols \"3\" {};\n"
                "};",
            .map = NULL,
            .num_sections = 3,
            .sections = {
                {
                    .name = "1",
                    .file_type = FILE_TYPE_KEYMAP,
                    .flags = 0,
                    .num_include_groups = 0,
                    .include_groups = {},
                    .num_includes = 0,
                    .includes = {},
                },
                {
                    .name = "2",
                    .file_type = FILE_TYPE_TYPES,
                    .flags = 0,
                    .num_include_groups = 0,
                    .include_groups = {},
                    .num_includes = 0,
                    .includes = {},
                },
                {
                    .name = "3",
                    .file_type = FILE_TYPE_SYMBOLS,
                    .flags = 0,
                    .num_include_groups = 0,
                    .include_groups = {},
                    .num_includes = 0,
                    .includes = {},
                },
            },
            .error = false,
        },
        {
            .string =
                "xkb_keymap \"10\" {\n"
                "  xkb_types \"11\" {};\n"
                "  xkb_symbols \"12\" {};\n"
                "};\n"
                "default xkb_keymap \"20\" {\n"
                "  xkb_types \"21\" {};\n"
                "  xkb_symbols \"22\" {};\n"
                "};",
            .map = NULL,
            .sections = {},
            .num_sections = 0,
            .error = true,
            // TODO: enable multiple keymap per file
            // .num_sections = 6,
            // .sections = {
            //     {
            //         .path = NULL,
            //         .name = "10",
            //         .file_type = FILE_TYPE_KEYMAP,
            //         .flags = 0,
            //         .includes = {},
            //         .num_include = 0
            //     },
            //     {
            //         .path = NULL,
            //         .name = "11",
            //         .file_type = FILE_TYPE_TYPES,
            //         .flags = 0,
            //         .includes = {},
            //         .num_include = 0
            //     },
            //     {
            //         .path = NULL,
            //         .name = "12",
            //         .file_type = FILE_TYPE_SYMBOLS,
            //         .flags = 0,
            //         .includes = {},
            //         .num_include = 0
            //     },
            //     {
            //         .path = NULL,
            //         .name = "20",
            //         .file_type = FILE_TYPE_KEYMAP,
            //         .flags = MAP_IS_DEFAULT,
            //         .includes = {},
            //         .num_include = 0
            //     },
            //     {
            //         .path = NULL,
            //         .name = "21",
            //         .file_type = FILE_TYPE_TYPES,
            //         .flags = 0,
            //         .includes = {},
            //         .num_include = 0
            //     },
            //     {
            //         .path = NULL,
            //         .name = "22",
            //         .file_type = FILE_TYPE_SYMBOLS,
            //         .flags = 0,
            //         .includes = {},
            //         .num_include = 0
            //     },
            // },
            // .error = false,
        },
    };
    for (size_t k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%zu ***\n", __func__, k);
        struct xkb_file_iterator * const iter =
            xkb_file_iterator_new_from_buffer(
                ctx, XKB_FILE_ITERATOR_FAIL_ON_INCLUDE_ERROR,
                XKB_KEYMAP_FORMAT_TEXT_V2, XKB_KEYMAP_COMPILE_NO_FLAGS,
                "(string)", tests[k].map, FILE_TYPE_INVALID,
                tests[k].string, strlen(tests[k].string)
            );
        assert(iter);

        const struct xkb_file_section *section;
        unsigned int s = 0;
        bool ok = true;
        while ((ok = xkb_file_iterator_next(iter, &section)) && section) {
            fprintf(stderr, "section #%u\n", s);
            if (s >= tests[k].num_sections) {
                assert_eq("Section count", tests[k].num_sections, s, "%u");
                break;
            }
            assert(test_section(&tests[k].sections[s], section));
            s++;
        }
        assert_eq("Error", tests[k].error, !ok, "%d");
        assert_eq("Section count", tests[k].num_sections, s, "%u");
        xkb_file_iterator_free(iter);
    }
}

int
main(void)
{
    test_init();

    struct xkb_context * const context = test_get_context(CONTEXT_NO_FLAG);

    test_resolve_file(context);
    test_file_section_parse(context);
    test_file_iterator(context);

    xkb_context_unref(context);

    return EXIT_SUCCESS;
}
