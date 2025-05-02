/*
 * Copyright © 2014 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <signal.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "test.h"
#include "evdev-scancodes.h"
#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon-names.h"
#include "xkbcommon/xkbcommon-x11.h"
#include "xvfb-wrapper.h"

/* Offset between evdev keycodes (where KEY_ESCAPE is 1), and the evdev XKB
 * keycode set (where ESC is 9). */
#define EVDEV_OFFSET 8

X11_TEST(test_basic)
{
    struct xkb_keymap *keymap = NULL;
    struct xkb_state *state = NULL;
    xcb_connection_t *conn;
    int32_t device_id;
    int ret, status;
    char *xkb_path;
    char *original, *dump;
    char *envp[] = { NULL };
    char *xkbcomp_argv[] = {
        (char *) "xkbcomp", (char *) "-I", NULL /* xkb_path */, display, NULL
    };
    pid_t xkbcomp_pid;

    conn = xcb_connect(display, NULL);
    if (xcb_connection_has_error(conn)) {
        ret = TEST_SETUP_FAILURE;
        goto err_conn;
    }
    ret = xkb_x11_setup_xkb_extension(conn,
                                      XKB_X11_MIN_MAJOR_XKB_VERSION,
                                      XKB_X11_MIN_MINOR_XKB_VERSION,
                                      XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
                                      NULL, NULL, NULL, NULL);
    if (!ret) {
        ret = TEST_SETUP_FAILURE;
        goto err_xcb;
    }
    device_id = xkb_x11_get_core_keyboard_device_id(conn);
    assert(device_id != -1);

    xkb_path = test_get_path("keymaps/host.xkb");
    assert(ret >= 0);
    xkbcomp_argv[2] = xkb_path;
    ret = posix_spawnp(&xkbcomp_pid, "xkbcomp", NULL, NULL, xkbcomp_argv, envp);
    free(xkb_path);
    if (ret != 0) {
        fprintf(stderr,
                "[ERROR] Cannot run xkbcomp. posix_spawnp error %d: %s\n",
                ret, strerror(ret));
        if (ret == ENOENT) {
            fprintf(stderr,
                    "[ERROR] xkbcomp may be missing. "
                    "Please install the corresponding package, "
                    "e.g. \"xkbcomp\" or \"x11-xkb-utils\".\n");
        }
        ret = TEST_SETUP_FAILURE;
        goto err_xcb;
    }
    ret = waitpid(xkbcomp_pid, &status, 0);
    if (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        ret = TEST_SETUP_FAILURE;
        goto err_xcb;
    }

    struct xkb_context *ctx = test_get_context(0);
    keymap = xkb_x11_keymap_new_from_device(ctx, conn, device_id,
                                            XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(keymap);

    original = test_read_file("keymaps/host.xkb");
    assert(original);

    dump = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    assert(dump);

    if (!streq(original, dump)) {
        fprintf(stderr,
                "round-trip test failed: dumped map differs from original\n");
        fprintf(stderr, "length: dumped %zu, original %zu\n",
                strlen(dump),
                strlen(original));
        fprintf(stderr, "dumped map:\n");
        fprintf(stderr, "%s\n", dump);
        ret = 1;
        goto err_dump;
    }

    state = xkb_x11_state_new_from_device(keymap, conn, device_id);
    assert(state);

    /* Check capitalization transformation */
    xkb_keysym_t sym;
    sym = xkb_state_key_get_one_sym(state, KEY_A + EVDEV_OFFSET);
    assert(sym == XKB_KEY_a);
    sym = xkb_state_key_get_one_sym(state, KEY_LEFT + EVDEV_OFFSET);
    assert(sym == XKB_KEY_Left);
    const xkb_mod_index_t caps_idx =
        xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
    assert(caps_idx != XKB_MOD_INVALID);
    const xkb_mod_mask_t caps = UINT32_C(1) << caps_idx;
    xkb_state_update_mask(state, 0, 0, caps, 0, 0, 0);
    sym = xkb_state_key_get_one_sym(state, KEY_A + EVDEV_OFFSET);
    assert(sym == XKB_KEY_A);
    sym = xkb_state_key_get_one_sym(state, KEY_LEFT + EVDEV_OFFSET);
    assert(sym == XKB_KEY_Left);

    ret = EXIT_SUCCESS;
err_dump:
    free(original);
    free(dump);
    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);
err_xcb:
    xcb_disconnect(conn);
err_conn:
    return ret;
}

int main(void) {
    test_init();

    return x11_tests_run();
}
