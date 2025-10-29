/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon.h"
#include "evdev-scancodes.h"
#include "test.h"
#include "context.h"
#include "keymap.h"
#include "utils.h"

static void
test_layouts(const char* xkb_root, bool update_output_files)
{
    struct xkb_context *ctx;
    struct xkb_keymap *keymap;
    const xkb_keysym_t *keysyms;

    char *unversioned_extensions_path = test_get_path("extensions/without-rules");
    assert(unversioned_extensions_path);
    char *versioned_extensions_path = test_get_path("extensions/without-rules-2");
    assert(versioned_extensions_path);
    setenv("XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH", unversioned_extensions_path, 1);
    setenv("XKB_CONFIG_VERSIONED_EXTENSIONS_PATH", versioned_extensions_path, 1);
    ctx = xkb_context_new(XKB_CONTEXT_NO_ENVIRONMENT_NAMES);
    assert(ctx);

    char buf[PATH_MAX] = "";
    assert(xkb_context_num_include_paths(ctx) == 4);
    assert(snprintf_safe(buf, sizeof(buf), "%s/%s",
                         versioned_extensions_path, "p1"));
    assert(strcmp(xkb_context_include_path_get(ctx, 0), buf) == 0);
    assert(snprintf_safe(buf, sizeof(buf), "%s/%s",
                         versioned_extensions_path, "p2"));
    assert(strcmp(xkb_context_include_path_get(ctx, 1), buf) == 0);
    assert(snprintf_safe(buf, sizeof(buf), "%s/%s",
                         unversioned_extensions_path, "p3"));
    assert(strcmp(xkb_context_include_path_get(ctx, 2), buf) == 0);
    assert(strcmp(xkb_context_include_path_get(ctx, 3), xkb_root) == 0);

    free(unversioned_extensions_path);
    free(versioned_extensions_path);

    /* New layouts (example from documentation: “Packaging keyboard layouts”) */
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                "evdev", "pc104", "a,b,c", NULL, NULL);
    assert(keymap);
    assert(1 == xkb_keymap_key_get_syms_by_level(keymap, EVDEV_OFFSET + KEY_A,
                                                 0, 0, &keysyms));
    assert(keysyms[0] == XKB_KEY_Greek_alpha); /* from versioned dir */
    assert(1 == xkb_keymap_key_get_syms_by_level(keymap, EVDEV_OFFSET + KEY_A,
                                                 1, 0, &keysyms));
    assert(keysyms[0] == XKB_KEY_aacute); /* from versioned dir */
    assert(1 == xkb_keymap_key_get_syms_by_level(keymap, EVDEV_OFFSET + KEY_A,
                                                 2, 0, &keysyms));
    assert(keysyms[0] == XKB_KEY_adiaeresis); /* from unversioned dir */
    xkb_keymap_unref(keymap);

    xkb_context_unref(ctx);
}

static void
test_options(const char *xkb_root, bool update_output_files)
{
    struct xkb_context *ctx;
    struct xkb_keymap *keymap;
    const xkb_keysym_t *keysyms;

    char *unversioned_extensions_path = test_get_path("extensions/with-rules");
    assert(unversioned_extensions_path);
    char *versioned_extensions_path = test_get_path("extensions/with-rules-2");
    assert(versioned_extensions_path);
    setenv("XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH", unversioned_extensions_path, 1);
    setenv("XKB_CONFIG_VERSIONED_EXTENSIONS_PATH", versioned_extensions_path, 1);
    ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(ctx);

    char buf[PATH_MAX] = "";
    assert(xkb_context_num_include_paths(ctx) == 4);
    assert(snprintf_safe(buf, sizeof(buf), "%s/%s",
                         versioned_extensions_path, "p1"));
    assert(strcmp(xkb_context_include_path_get(ctx, 0), buf) == 0);
    assert(snprintf_safe(buf, sizeof(buf), "%s/%s",
                         versioned_extensions_path, "p2"));
    assert(strcmp(xkb_context_include_path_get(ctx, 1), buf) == 0);
    assert(snprintf_safe(buf, sizeof(buf), "%s/%s",
                         unversioned_extensions_path, "p3"));
    assert(strcmp(xkb_context_include_path_get(ctx, 2), buf) == 0);
    assert(strcmp(xkb_context_include_path_get(ctx, 3), xkb_root) == 0);

    free(unversioned_extensions_path);
    free(versioned_extensions_path);

    /* New options */
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                "evdev", "pc104", "cz,ca,de", NULL,
                                "opt:1,opt:2,opt:3!2");
    assert(keymap);
    assert(1 == xkb_keymap_key_get_syms_by_level(keymap, EVDEV_OFFSET + KEY_A,
                                                 0, 0, &keysyms));
    assert(keysyms[0] == XKB_KEY_Greek_alpha);
    assert(1 == xkb_keymap_key_get_syms_by_level(keymap, EVDEV_OFFSET + KEY_S,
                                                 0, 0, &keysyms));
    assert(keysyms[0] == XKB_KEY_sacute);
    assert(1 == xkb_keymap_key_get_syms_by_level(keymap, EVDEV_OFFSET + KEY_A,
                                                 1, 0, &keysyms));
    assert(keysyms[0] == XKB_KEY_adiaeresis);
    assert(1 == xkb_keymap_key_get_syms_by_level(keymap, EVDEV_OFFSET + KEY_S,
                                                 1, 0, &keysyms));
    assert(keysyms[0] == XKB_KEY_sacute);
    assert(1 == xkb_keymap_key_get_syms_by_level(keymap, EVDEV_OFFSET + KEY_A,
                                                 2, 0, &keysyms));
    assert(keysyms[0] == XKB_KEY_a);
    assert(1 == xkb_keymap_key_get_syms_by_level(keymap, EVDEV_OFFSET + KEY_S,
                                                 2, 0, &keysyms));
    assert(keysyms[0] == XKB_KEY_sacute);
    xkb_keymap_unref(keymap);

    xkb_context_unref(ctx);
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

    unsetenv("HOME");
    unsetenv("XDG_CONFIG_HOME");
    unsetenv("XDG_CONFIG_DIR");
    setenv("XKB_CONFIG_EXTRA_PATH", "¡SKIP!", 1);

    char * const xkb_root = test_get_path("");
    assert(xkb_root);
    setenv("XKB_CONFIG_ROOT", xkb_root, 1);

    test_layouts(xkb_root, update_output_files);
    test_options(xkb_root, update_output_files);

    free(xkb_root);
    return EXIT_SUCCESS;
}
