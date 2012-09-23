/*
 * Copyright © 2009 Dan Nicholson
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
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
 *
 * Authors: Dan Nicholson <dbn.lists@gmail.com>
 *          Ran Benita <ran234@gmail.com>
 *          Daniel Stone <daniel@fooishbar.org>
 */

#include "xkbcomp-priv.h"
#include "rules.h"

static struct xkb_keymap *
compile_keymap_file(struct xkb_context *ctx, XkbFile *file,
                    enum xkb_keymap_format format,
                    enum xkb_keymap_compile_flags flags)
{
    struct xkb_keymap *keymap;

    keymap = xkb_keymap_new(ctx, format, flags);
    if (!keymap)
        goto err;

    if (file->file_type != FILE_TYPE_KEYMAP) {
        log_err(ctx, "Cannot compile a %s file alone into a keymap\n",
                xkb_file_type_to_string(file->file_type));
        goto err;
    }

    if (!CompileKeymap(file, keymap, MERGE_OVERRIDE)) {
        log_err(ctx, "Failed to compile keymap\n");
        goto err;
    }

    return keymap;

err:
    xkb_keymap_unref(keymap);
    return NULL;
}

XKB_EXPORT struct xkb_keymap *
xkb_keymap_new_from_names(struct xkb_context *ctx,
                          const struct xkb_rule_names *rmlvo_in,
                          enum xkb_keymap_compile_flags flags)
{
    bool ok;
    struct xkb_component_names kccgst;
    struct xkb_rule_names rmlvo = *rmlvo_in;
    XkbFile *file;
    struct xkb_keymap *keymap;

    if (isempty(rmlvo.rules))
        rmlvo.rules = DEFAULT_XKB_RULES;
    if (isempty(rmlvo.model))
        rmlvo.model = DEFAULT_XKB_MODEL;
    if (isempty(rmlvo.layout))
        rmlvo.layout = DEFAULT_XKB_LAYOUT;

    log_dbg(ctx,
            "Compiling from RMLVO: rules '%s', model '%s', layout '%s', "
            "variant '%s', options '%s'\n",
            strnull(rmlvo.rules), strnull(rmlvo.model),
            strnull(rmlvo.layout), strnull(rmlvo.variant),
            strnull(rmlvo.options));

    ok = xkb_components_from_rules(ctx, &rmlvo, &kccgst);
    if (!ok) {
        log_err(ctx,
                "Couldn't look up rules '%s', model '%s', layout '%s', "
                "variant '%s', options '%s'\n",
                strnull(rmlvo.rules), strnull(rmlvo.model),
                strnull(rmlvo.layout), strnull(rmlvo.variant),
                strnull(rmlvo.options));
        return NULL;
    }

    log_dbg(ctx,
            "Compiling from KcCGST: keycodes '%s', types '%s', "
            "compat '%s', symbols '%s'\n",
            kccgst.keycodes, kccgst.types, kccgst.compat, kccgst.symbols);

    file = XkbFileFromComponents(ctx, &kccgst);

    free(kccgst.keycodes);
    free(kccgst.types);
    free(kccgst.compat);
    free(kccgst.symbols);

    if (!file) {
        log_err(ctx,
                "Failed to generate parsed XKB file from components\n");
        return NULL;
    }

    keymap = compile_keymap_file(ctx, file, XKB_KEYMAP_FORMAT_TEXT_V1, flags);
    FreeXkbFile(file);
    return keymap;
}

XKB_EXPORT struct xkb_keymap *
xkb_keymap_new_from_string(struct xkb_context *ctx,
                           const char *string,
                           enum xkb_keymap_format format,
                           enum xkb_keymap_compile_flags flags)
{
    bool ok;
    XkbFile *file;
    struct xkb_keymap *keymap;

    if (format != XKB_KEYMAP_FORMAT_TEXT_V1) {
        log_err(ctx, "Unsupported keymap format %d\n", format);
        return NULL;
    }

    if (!string) {
        log_err(ctx, "No string specified to generate XKB keymap\n");
        return NULL;
    }

    ok = XkbParseString(ctx, string, "input", &file);
    if (!ok) {
        log_err(ctx, "Failed to parse input xkb file\n");
        return NULL;
    }

    keymap = compile_keymap_file(ctx, file, format, flags);
    FreeXkbFile(file);
    return keymap;
}

XKB_EXPORT struct xkb_keymap *
xkb_keymap_new_from_file(struct xkb_context *ctx,
                         FILE *file,
                         enum xkb_keymap_format format,
                         enum xkb_keymap_compile_flags flags)
{
    bool ok;
    XkbFile *xkb_file;
    struct xkb_keymap *keymap;

    if (format != XKB_KEYMAP_FORMAT_TEXT_V1) {
        log_err(ctx, "Unsupported keymap format %d\n", format);
        return NULL;
    }

    if (!file) {
        log_err(ctx, "No file specified to generate XKB keymap\n");
        return NULL;
    }

    ok = XkbParseFile(ctx, file, "(unknown file)", &xkb_file);
    if (!ok) {
        log_err(ctx, "Failed to parse input xkb file\n");
        return NULL;
    }

    keymap = compile_keymap_file(ctx, xkb_file, format, flags);
    FreeXkbFile(xkb_file);
    return keymap;
}
