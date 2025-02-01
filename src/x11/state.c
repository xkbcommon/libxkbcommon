/*
 * Copyright © 2013 Ran Benita
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include "x11-priv.h"

static bool
update_initial_state(struct xkb_state *state, xcb_connection_t *conn,
                     uint16_t device_id)
{
    xcb_xkb_get_state_cookie_t cookie =
        xcb_xkb_get_state(conn, device_id);
    xcb_xkb_get_state_reply_t *reply =
        xcb_xkb_get_state_reply(conn, cookie, NULL);

    if (!reply)
        return false;

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

XKB_EXPORT struct xkb_state *
xkb_x11_state_new_from_device(struct xkb_keymap *keymap,
                              xcb_connection_t *conn, int32_t device_id)
{
    struct xkb_state *state;

    if (device_id < 0 || device_id > 255) {
        log_err_func(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
                     "illegal device ID: %d", device_id);
        return NULL;
    }

    state = xkb_state_new(keymap);
    if (!state)
        return NULL;

    if (!update_initial_state(state, conn, device_id)) {
        xkb_state_unref(state);
        return NULL;
    }

    return state;
}
