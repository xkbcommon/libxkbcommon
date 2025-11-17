/*
 * Copyright © 2013 Ran Benita
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include "x11-priv.h"
#include "xkbcommon/xkbcommon.h"

/* Some macros. Not very nice but it'd be worse without them. */

#define FAIL_IF_BAD_REPLY(ctx, reply, request_name) do {    \
    if (!(reply)) {                                         \
        log_err((ctx), XKB_LOG_MESSAGE_NO_ID,               \
                "x11: failed to get keymap from X server: " \
                "%s request failed\n",                      \
                (request_name));                            \
        goto fail;                                          \
    }                                                       \
} while (0)

static bool
update_initial_state(struct xkb_state *state, xcb_connection_t *conn,
                     uint16_t device_id, enum xkb_action_controls controls)
{
    xcb_xkb_get_state_cookie_t cookie =
        xcb_xkb_get_state(conn, device_id);
    xcb_xkb_get_state_reply_t *reply =
        xcb_xkb_get_state_reply(conn, cookie, NULL);

    if (!reply)
        return false;

    /* NOTE: Use the public API with private enum values */
    xkb_state_update_controls(state, (enum xkb_keyboard_controls) controls,
                              (enum xkb_keyboard_controls) controls);

    xkb_state_update_mask(state,
                          reply->baseMods,
                          reply->latchedMods,
                          reply->lockedMods,
                          reply->baseGroup,
                          reply->latchedGroup,
                          reply->lockedGroup);

    free(reply);
    return true;
}

static enum xkb_state_accessibility_flags
translate_state_accessibility_flags(const xcb_xkb_get_controls_reply_t *reply)
{
    enum xkb_state_accessibility_flags flags = XKB_STATE_A11Y_NO_FLAGS;
    if (reply->accessXOption & XCB_XKB_AX_OPTION_LATCH_TO_LOCK) {
        flags |= XKB_STATE_A11Y_LATCH_TO_LOCK;
    }
    return flags;
}

static bool
get_controls(struct xkb_context *ctx, xcb_connection_t *conn, int32_t device_id,
             struct xkb_state_options *options,
             enum xkb_action_controls *controls)
{
    xcb_xkb_get_controls_cookie_t cookie = xcb_xkb_get_controls(conn, device_id);
    xcb_xkb_get_controls_reply_t *reply =
        xcb_xkb_get_controls_reply(conn, cookie, NULL);

    FAIL_IF_BAD_REPLY(ctx, reply, "XkbGetControls");

    const enum xkb_state_accessibility_flags flags =
        translate_state_accessibility_flags(reply);
    if (xkb_state_options_update_a11y_flags(options, flags, flags) != 0)
        goto fail;

    *controls = translate_controls_mask(reply->enabledControls);

    free(reply);
    return true;

fail:
    free(reply);
    return false;
}

struct xkb_state *
xkb_x11_state_new_from_device(struct xkb_keymap *keymap,
                              xcb_connection_t *conn, int32_t device_id)
{
    if (device_id < 0 || device_id > 255) {
        log_err_func(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
                     "illegal device ID: %"PRId32, device_id);
        return NULL;
    }

    struct xkb_state_options * const options = xkb_state_options_new(keymap->ctx);
    if (options == NULL)
        return NULL;

    enum xkb_action_controls controls = 0;
    if (!get_controls(keymap->ctx, conn, device_id, options, &controls)) {
        xkb_state_options_destroy(options);
        return NULL;
    }

    struct xkb_state * const state = xkb_state_new2(keymap, options);
    xkb_state_options_destroy(options);
    if (!state)
        return NULL;

    if (!update_initial_state(state, conn, device_id, controls)) {
        xkb_state_unref(state);
        return NULL;
    }

    return state;
}
