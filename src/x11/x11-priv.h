/*
 * Copyright © 2013 Ran Benita
 * SPDX-License-Identifier: MIT
 */

#ifndef _XKBCOMMON_X11_PRIV_H
#define _XKBCOMMON_X11_PRIV_H

#include <xcb/xkb.h>

#include "keymap.h"
#include "xkbcommon/xkbcommon-x11.h"

struct x11_atom_interner {
    struct xkb_context *ctx;
    xcb_connection_t *conn;
    bool had_error;
    /* Atoms for which we send a GetAtomName request */
    struct {
        xcb_atom_t from;
        xkb_atom_t *out;
        xcb_get_atom_name_cookie_t cookie;
    } pending[128];
    size_t num_pending;
    /* Atoms which were already pending but queried again */
    struct {
        xcb_atom_t from;
        xkb_atom_t *out;
    } copies[128];
    size_t num_copies;
    /* These are not interned, but saved directly (after XkbEscapeMapName) */
    struct {
        xcb_get_atom_name_cookie_t cookie;
        char **out;
    } escaped[4];
    size_t num_escaped;
};

void
x11_atom_interner_init(struct x11_atom_interner *interner,
                       struct xkb_context *ctx, xcb_connection_t *conn);

void
x11_atom_interner_round_trip(struct x11_atom_interner *interner);

/*
 * Make a xkb_atom_t's from X atoms. The actual write is delayed until the next
 * call to x11_atom_interner_round_trip() or when too many atoms are pending.
 */
void
x11_atom_interner_adopt_atom(struct x11_atom_interner *interner,
                             const xcb_atom_t atom, xkb_atom_t *out);

/*
 * Get a strdup'd and XkbEscapeMapName'd name of an X atom. The actual write is
 * delayed until the next call to x11_atom_interner_round_trip().
 */
void
x11_atom_interner_get_escaped_atom_name(struct x11_atom_interner *interner,
                                        xcb_atom_t atom, char **out);

#endif
