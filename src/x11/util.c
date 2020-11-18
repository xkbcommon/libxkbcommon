/*
 * Copyright © 2013 Ran Benita
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

#include "config.h"

#include "x11-priv.h"

XKB_EXPORT int
xkb_x11_setup_xkb_extension(xcb_connection_t *conn,
                            uint16_t major_xkb_version,
                            uint16_t minor_xkb_version,
                            enum xkb_x11_setup_xkb_extension_flags flags,
                            uint16_t *major_xkb_version_out,
                            uint16_t *minor_xkb_version_out,
                            uint8_t *base_event_out,
                            uint8_t *base_error_out)
{
    uint8_t base_event, base_error;
    uint16_t server_major, server_minor;

    if (flags & ~(XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS)) {
        /* log_err_func(ctx, "unrecognized flags: %#x\n", flags); */
        return 0;
    }

    {
        const xcb_query_extension_reply_t *reply =
            xcb_get_extension_data(conn, &xcb_xkb_id);
        if (!reply) {
            /* log_err_func(ctx, "failed to query for XKB extension\n"); */
            return 0;
        }

        if (!reply->present) {
            /* log_err_func(ctx, "failed to start using XKB extension: not available in server\n"); */
            return 0;
        }

        base_event = reply->first_event;
        base_error = reply->first_error;
    }

    {
        xcb_generic_error_t *error = NULL;
        xcb_xkb_use_extension_cookie_t cookie =
            xcb_xkb_use_extension(conn, major_xkb_version, minor_xkb_version);
        xcb_xkb_use_extension_reply_t *reply =
            xcb_xkb_use_extension_reply(conn, cookie, &error);

        if (!reply) {
            /* log_err_func(ctx, */
            /*              "failed to start using XKB extension: error code %d\n", */
            /*              error ? error->error_code : -1); */
            free(error);
            return 0;
        }

        if (!reply->supported) {
            /* log_err_func(ctx, */
            /*              "failed to start using XKB extension: server doesn't support version %d.%d\n", */
            /*              major_xkb_version, minor_xkb_version); */
            free(reply);
            return 0;
        }

        server_major = reply->serverMajor;
        server_minor = reply->serverMinor;

        free(reply);
    }

    /*
    * The XkbUseExtension() in libX11 has a *bunch* of legacy stuff, but
    * it doesn't seem like any of it is useful to us.
    */

    if (major_xkb_version_out)
        *major_xkb_version_out = server_major;
    if (minor_xkb_version_out)
        *minor_xkb_version_out = server_minor;
    if (base_event_out)
        *base_event_out = base_event;
    if (base_error_out)
        *base_error_out = base_error;

    return 1;
}

XKB_EXPORT int32_t
xkb_x11_get_core_keyboard_device_id(xcb_connection_t *conn)
{
    int32_t device_id;
    xcb_xkb_get_device_info_cookie_t cookie =
        xcb_xkb_get_device_info(conn, XCB_XKB_ID_USE_CORE_KBD,
                                0, 0, 0, 0, 0, 0);
    xcb_xkb_get_device_info_reply_t *reply =
        xcb_xkb_get_device_info_reply(conn, cookie, NULL);

    if (!reply)
        return -1;

    device_id = reply->deviceID;
    free(reply);
    return device_id;
}

bool
get_atom_name(xcb_connection_t *conn, xcb_atom_t atom, char **out)
{
    xcb_get_atom_name_cookie_t cookie;
    xcb_get_atom_name_reply_t *reply;
    int length;
    char *name;

    if (atom == 0) {
        *out = NULL;
        return true;
    }

    cookie = xcb_get_atom_name(conn, atom);
    reply = xcb_get_atom_name_reply(conn, cookie, NULL);
    if (!reply)
        return false;

    length = xcb_get_atom_name_name_length(reply);
    name = xcb_get_atom_name_name(reply);

    *out = strndup(name, length);
    if (!*out) {
        free(reply);
        return false;
    }

    free(reply);
    return true;
}

struct x11_atom_cache {
    /*
     * Invalidate the cache based on the XCB connection.
     * X11 atoms are actually not per connection or client, but per X server
     * session. But better be safe just in case we survive an X server restart.
     */
    xcb_connection_t *conn;
    struct {
        xcb_atom_t from;
        xkb_atom_t to;
    } cache[256];
    size_t len;
};

bool
adopt_atoms(struct xkb_context *ctx, xcb_connection_t *conn,
            const xcb_atom_t *from, xkb_atom_t *to, const size_t count)
{
    enum { SIZE = 128 };
    xcb_get_atom_name_cookie_t cookies[SIZE];
    const size_t num_batches = ROUNDUP(count, SIZE) / SIZE;

    if (!ctx->x11_atom_cache) {
        ctx->x11_atom_cache = calloc(1, sizeof(struct x11_atom_cache));
    }
    /* Can be NULL in case the malloc failed. */
    struct x11_atom_cache *cache = ctx->x11_atom_cache;
    if (cache && cache->conn != conn) {
        cache->conn = conn;
        cache->len = 0;
    }

    memset(to, 0, count * sizeof(*to));

    /* Send and collect the atoms in batches of reasonable SIZE. */
    for (size_t batch = 0; batch < num_batches; batch++) {
        const size_t start = batch * SIZE;
        const size_t stop = MIN((batch + 1) * SIZE, count);

        /* Send. */
        for (size_t i = start; i < stop; i++) {
            bool cache_hit = false;
            if (cache) {
                for (size_t c = 0; c < cache->len; c++) {
                    if (cache->cache[c].from == from[i]) {
                        to[i] = cache->cache[c].to;
                        cache_hit = true;
                        break;
                    }
                }
            }
            if (!cache_hit && from[i] != XCB_ATOM_NONE)
                cookies[i % SIZE] = xcb_get_atom_name(conn, from[i]);
        }

        /* Collect. */
        for (size_t i = start; i < stop; i++) {
            xcb_get_atom_name_reply_t *reply;

            if (from[i] == XCB_ATOM_NONE)
                continue;

            /* Was filled from cache. */
            if (to[i] != 0)
                continue;

            reply = xcb_get_atom_name_reply(conn, cookies[i % SIZE], NULL);
            if (!reply)
                goto err_discard;

            to[i] = xkb_atom_intern(ctx,
                                    xcb_get_atom_name_name(reply),
                                    xcb_get_atom_name_name_length(reply));
            free(reply);

            if (to[i] == XKB_ATOM_NONE)
                goto err_discard;

            if (cache && cache->len < ARRAY_SIZE(cache->cache)) {
                size_t idx = cache->len++;
                cache->cache[idx].from = from[i];
                cache->cache[idx].to = to[i];
            }

            continue;

            /*
             * If we don't discard the uncollected replies, they just
             * sit in the XCB queue waiting forever. Sad.
             */
err_discard:
            for (size_t j = i + 1; j < stop; j++)
                if (from[j] != XCB_ATOM_NONE)
                    xcb_discard_reply(conn, cookies[j % SIZE].sequence);
            return false;
        }
    }

    return true;
}

bool
adopt_atom(struct xkb_context *ctx, xcb_connection_t *conn, xcb_atom_t atom,
           xkb_atom_t *out)
{
    return adopt_atoms(ctx, conn, &atom, out, 1);
}
