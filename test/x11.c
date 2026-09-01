/*
 * Copyright © 2013 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include <stdlib.h>

#include "test.h"
#include "xkbcommon/xkbcommon.h"
#include "xvfb-wrapper.h"
#include "xkbcommon/xkbcommon-x11.h"

X11_TEST(test_basic)
{
    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    xcb_connection_t *conn;
    int ret;
    int32_t device_id;
    struct xkb_keymap *keymap;
    struct xkb_state *state;
    char *dump;
    int exit_code = EXIT_SUCCESS;

    /*
     * The next two steps depend on a running X server with XKB support.
     * If it fails, it's not necessarily an actual problem with the code.
     * So we don't want a FAIL here.
     */
    conn = xcb_connect(display, NULL);
    if (!conn || xcb_connection_has_error(conn)) {
        exit_code = TEST_SETUP_FAILURE;
        goto err_conn;
    }

    /* Reject invalid flags */
    assert(!xkb_x11_setup_xkb_extension(conn,
                                        XKB_X11_MIN_MAJOR_XKB_VERSION,
                                        XKB_X11_MIN_MINOR_XKB_VERSION,
                                        -1, NULL, NULL, NULL, NULL));
    assert(!xkb_x11_setup_xkb_extension(conn,
                                        XKB_X11_MIN_MAJOR_XKB_VERSION,
                                        XKB_X11_MIN_MINOR_XKB_VERSION,
                                        0xffff, NULL, NULL, NULL, NULL));

    ret = xkb_x11_setup_xkb_extension(conn,
                                      XKB_X11_MIN_MAJOR_XKB_VERSION,
                                      XKB_X11_MIN_MINOR_XKB_VERSION,
                                      XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
                                      NULL, NULL, NULL, NULL);
    if (!ret) {
        exit_code = TEST_SETUP_FAILURE;
        goto err_conn;
    }

    device_id = xkb_x11_get_core_keyboard_device_id(conn);
    assert(device_id != -1);

    keymap = xkb_x11_keymap_new_from_device(ctx, conn, device_id,
                                            TEST_KEYMAP_COMPILE_FLAGS);
    assert(keymap);

    state = xkb_x11_state_new_from_device(keymap, conn, device_id);
    assert(state);

    dump = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    assert(dump);
    fputs(dump, stdout);

    /* TODO: Write some X11-specific tests. */

    free(dump);
    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
err_conn:
    xcb_disconnect(conn);
    xkb_context_unref(ctx);

    return exit_code;
}

X11_TEST(test_quick_guide_examples)
{
    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);

// NOLINTBEGIN(readability-duplicate-include)
//! [quick-guide-x11-client-init-example]
    #include <xkbcommon/xkbcommon-x11.h>

    int ret = EXIT_SUCCESS;
    xcb_connection_t *conn = xcb_connect(display, NULL);
    if (!conn || xcb_connection_has_error(conn)) {
        /* Handle connection error */
        // ...
        ret = EXIT_FAILURE;
        goto connection_error;
    }

    ret = xkb_x11_setup_xkb_extension(conn,
                                      XKB_X11_MIN_MAJOR_XKB_VERSION,
                                      XKB_X11_MIN_MINOR_XKB_VERSION,
                                      XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
                                      NULL, NULL, NULL, NULL);
    if (!ret) {
        goto connection_error;
    }

    int32_t device_id = xkb_x11_get_core_keyboard_device_id(conn);
    if (device_id == -1) {
        /* Handle device error */
        // ...
        ret = EXIT_FAILURE;
        goto device_error;
    }

    struct xkb_keymap *keymap = xkb_x11_keymap_new_from_device(
        ctx, conn, device_id, XKB_KEYMAP_COMPILE_NO_FLAGS
    );
    if (!keymap) {
        /* Handle keymap error */
        // ...
        ret = EXIT_FAILURE;
        goto keymap_error;
    }
//! [quick-guide-x11-client-init-example]
// NOLINTEND(readability-duplicate-include)

//! [quick-guide-x11-client-state-example]
    struct xkb_state *state = xkb_x11_state_new_from_device(keymap, conn, device_id);
    if (!state)  {
        /* Handle error */
        // ...
        ret = EXIT_FAILURE;
        goto state_error;
    }
//! [quick-guide-x11-client-state-example]

    ret = EXIT_SUCCESS;

    xkb_state_unref(state);
state_error:
    xkb_keymap_unref(keymap);
keymap_error:
device_error:
connection_error:
    xcb_disconnect(conn);
    xkb_context_unref(ctx);
    return ret;
}

int main(void) {
    test_init();

    return x11_tests_run();
}
