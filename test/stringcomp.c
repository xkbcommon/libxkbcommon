/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon.h"
#include "test.h"
#include "test/utils-text.h"
#include "utils.h"
#include "keymap-compare.h"

static void
test_keymap_comparison(struct xkb_context *ctx)
{
    static const struct {
        const char* keymap1;
        const char* keymap2;
        enum xkb_keymap_compare_property properties;
        bool same;
    } tests[] = {
        {
            .keymap1 = "xkb_keymap {};",
            .keymap2 = "xkb_keymap {};",
            .properties = XKB_KEYMAP_CMP_ALL,
            .same = true
        },
        {
            .keymap1 =
                "xkb_keymap {\n"
                "  xkb_keycodes { <> = 1; };\n"
                "};",
            .keymap2 = "xkb_keymap {};",
            .properties = XKB_KEYMAP_CMP_ALL,
            .same = false
        },
        {
            .keymap1 =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <a> = 1;\n"
                "    <b> = 2;\n"
                "  };\n"
                "  xkb_compat { include \"caps\" };\n"
                "  xkb_types { include \"basic+iso9995\" };\n"
                "  xkb_symbols {\n"
                "    key <a> { [a, A] };\n"
                "    key <b> { [Caps_Lock] };\n" /* implicit action */
                "  };\n"
                "};",
            .keymap2 =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <a> = 1;\n"
                "    <b> = 2;\n"
                "  };\n"
                "  xkb_compat {};\n" /* empty */
                "  xkb_types { include \"basic\" };\n" /* less types */
                "  xkb_symbols {\n"
                /* Change key order + explicit actions and types */
                "    key <b> {\n"
                "      [Caps_Lock],\n"
                "      [LockMods(modifiers = Lock)],\n"
                "      type=\"ONE_LEVEL\"\n"
                "    };\n"
                "    key <a> { [a, A], type=\"ALPHABETIC\" };\n"
                /* Definition in different section */
                "    virtual_modifiers LevelThree;\n"
                "  };\n"
                "};",
            .properties = (XKB_KEYMAP_CMP_ALL & ~(XKB_KEYMAP_CMP_POSSIBLY_DROPPED)),
            .same = true
        },
    };
    for (size_t t = 0; t < ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n%s: #%zu\n", __func__, t);
        struct xkb_keymap * const keymap1 = test_compile_string(
            ctx, XKB_KEYMAP_FORMAT_TEXT_V1, tests[t].keymap1
        );
        assert(keymap1);
        struct xkb_keymap * const keymap2 = test_compile_string(
            ctx, XKB_KEYMAP_FORMAT_TEXT_V1, tests[t].keymap2
        );
        assert(keymap2);
        assert(xkb_keymap_compare(ctx, keymap1, keymap2, tests[t].properties) ==
               tests[t].same);
        xkb_keymap_unref(keymap1);
        xkb_keymap_unref(keymap2);
    }
}

static void
test_explicit_actions(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    char *original = test_read_file("keymaps/explicit-actions.xkb");
    assert(original);
    char *tmp = uncomment(original, strlen(original), "//");
    assert(tmp);
    char *expected = strip_lines(tmp, strlen(tmp), "//");
    free(tmp);
    assert(expected);
    char *got = NULL;

    /* Try original */
    keymap = test_compile_string(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, original);
    free(original);
    assert(keymap);
    got = xkb_keymap_get_as_string2(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                    TEST_KEYMAP_SERIALIZE_FLAGS);
    xkb_keymap_unref(keymap);
    assert_streq_not_null("Check output from original", expected, got);
    free(got);

    /* Try round-trip */
    keymap = test_compile_string(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, expected);
    assert(keymap);
    got = xkb_keymap_get_as_string2(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                    TEST_KEYMAP_SERIALIZE_FLAGS);
    xkb_keymap_unref(keymap);
    assert_streq_not_null("Check roundtrip", expected, got);
    free(got);

    free(expected);
}

static struct xkb_keymap*
compile_string(struct xkb_context *context, enum xkb_keymap_format format,
               const char *buf, size_t len, void *private)
{
    return test_compile_string(context, format, buf);
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
    struct xkb_keymap *keymap;
    char *dump, *dump2;

    assert(ctx);

    /* Make sure we can't (falsely claim to) compile an empty string. */
    keymap = test_compile_string(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "");
    assert(!keymap);

    /* Load in a prebuilt keymap, make sure we can compile it from a string,
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
            .path = "keymaps/stringcomp-v1-no-flags.xkb",
            .format = XKB_KEYMAP_FORMAT_TEXT_V1,
            .serialize_flags = XKB_KEYMAP_SERIALIZE_NO_FLAGS
        },
        {
            .path = "keymaps/stringcomp-v2.xkb",
            .format = XKB_KEYMAP_FORMAT_TEXT_V2,
            .serialize_flags = TEST_KEYMAP_SERIALIZE_FLAGS
        },
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(data); k++) {
        fprintf(stderr, "------\nTest roundtrip of: %s\n", data[k].path);
        char *original = test_read_file(data[k].path);
        assert(original);

        /* Check round-trip with same serialize flags */
        assert(test_compile_output2(ctx, data[k].format,
                                    XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                    data[k].serialize_flags,
                                    compile_string, NULL,
                                    "Round-trip with same serialize flags",
                                    original, 0 /* unused */, data[k].path,
                                    update_output_files));

        /*
         * Check round-trip by dropping/keeping unused keymap items
         * We obviously cannot compare the serializations, so only keymap
         * objects are compared.
         */

        keymap = xkb_keymap_new_from_string(ctx, original, data[k].format,
                                            XKB_KEYMAP_COMPILE_NO_FLAGS);
        assert(keymap);
        free(original);
        const enum xkb_keymap_serialize_flags test_serialize_flags[] = {
            (data[k].serialize_flags & ~XKB_KEYMAP_SERIALIZE_KEEP_UNUSED),
            (data[k].serialize_flags |  XKB_KEYMAP_SERIALIZE_KEEP_UNUSED)
        };
        for (size_t f = 0; f < ARRAY_SIZE(test_serialize_flags); f++) {
            original = xkb_keymap_get_as_string2(keymap, data[k].format,
                                                 test_serialize_flags[f]);
            assert(original);
            struct xkb_keymap *keymap2 = xkb_keymap_new_from_string(
                ctx, original, data[k].format, XKB_KEYMAP_COMPILE_NO_FLAGS
            );
            assert(keymap2);
            free(original);
            assert(xkb_keymap_compare(ctx, keymap, keymap2,
                                      (XKB_KEYMAP_CMP_ALL &
                                       ~(XKB_KEYMAP_CMP_POSSIBLY_DROPPED))));
            xkb_keymap_unref(keymap2);
        }

        /*
         * Check pretty/no-pretty round-trip, i.e. check that changing the
         * pretty-print flags results in the same keymap, for both keymap
         * objects and serializations.
         */

        const enum xkb_keymap_serialize_flags serialize_flags =
            (data[k].serialize_flags & XKB_KEYMAP_SERIALIZE_PRETTY)
                ? (data[k].serialize_flags & ~XKB_KEYMAP_SERIALIZE_PRETTY)
                : (data[k].serialize_flags |  XKB_KEYMAP_SERIALIZE_PRETTY);
        original = xkb_keymap_get_as_string2(keymap, data[k].format,
                                             serialize_flags);
        assert(original);

        struct xkb_keymap *keymap2 = xkb_keymap_new_from_string(
            ctx, original, data[k].format, XKB_KEYMAP_COMPILE_NO_FLAGS
        );
        assert(keymap2);
        assert(xkb_keymap_compare(ctx, keymap, keymap2, XKB_KEYMAP_CMP_ALL));
        xkb_keymap_unref(keymap2);

        assert(test_compile_output2(ctx, data[k].format,
                                    XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                    data[k].serialize_flags,
                                    compile_string, NULL,
                                    "Round-trip with different serialize flags",
                                    original, 0 /* unused */, data[k].path,
                                    update_output_files));
        free(original);
        xkb_keymap_unref(keymap);
    }

    /* Make sure we can recompile our output for a normal keymap from rules. */
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, NULL,
                                NULL, "ru,ca,de,us", ",multix,neo,intl", NULL);
    assert(keymap);
    dump = xkb_keymap_get_as_string2(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                     TEST_KEYMAP_SERIALIZE_FLAGS);
    assert(dump);
    xkb_keymap_unref(keymap);
    keymap = test_compile_string(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, dump);
    assert(keymap);
    /* Now test that the dump of the dump is equal to the dump! */
    dump2 = xkb_keymap_get_as_string2(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                                      TEST_KEYMAP_SERIALIZE_FLAGS);
    assert(dump2);
    assert(streq(dump, dump2));

    /* Test response to invalid formats and flags. */
    assert(!xkb_keymap_new_from_string(ctx, dump, 0, 0));
    assert(!xkb_keymap_new_from_string(ctx, dump, -1, 0));
    assert(!xkb_keymap_new_from_string(ctx, dump, XKB_KEYMAP_USE_ORIGINAL_FORMAT, 0));
    assert(!xkb_keymap_new_from_string(ctx, dump, XKB_KEYMAP_FORMAT_TEXT_V2+1, 0));
    assert(!xkb_keymap_new_from_string(ctx, dump, XKB_KEYMAP_FORMAT_TEXT_V1, -1));
    assert(!xkb_keymap_new_from_string(ctx, dump, XKB_KEYMAP_FORMAT_TEXT_V1, 1414));
    assert(!xkb_keymap_get_as_string2(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT, -1));
    assert(!xkb_keymap_get_as_string2(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT, 3333));
    assert(!xkb_keymap_get_as_string2(keymap, 0, TEST_KEYMAP_SERIALIZE_FLAGS));
    assert(!xkb_keymap_get_as_string2(keymap, 4893, TEST_KEYMAP_SERIALIZE_FLAGS));

    xkb_keymap_unref(keymap);
    free(dump);
    free(dump2);

    test_keymap_comparison(ctx);
    test_explicit_actions(ctx);

    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
