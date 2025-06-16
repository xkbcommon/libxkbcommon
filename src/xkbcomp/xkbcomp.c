/*
 * Copyright © 2009 Dan Nicholson
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 *
 * Author: Dan Nicholson <dbn.lists@gmail.com>
 * Author: Ran Benita <ran234@gmail.com>
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#include "config.h"

#include <stdbool.h>

#include "darray.h"
#include "utils.h"
#include "xkbcommon/xkbcommon.h"
#include "xkbcomp-priv.h"
#include "keymap.h"
#include "rules.h"
#include "rmlvo.h"

bool
xkb_components_names_from_rules(struct xkb_context *ctx,
                                const struct xkb_rule_names *rmlvo_in,
                                struct xkb_rule_names *rmlvo_out,
                                struct xkb_component_names *components_out)
{
    /* Resolve default RMLVO values. We need a mutable copy of the input. */
    struct xkb_rule_names rmlvo = *rmlvo_in;
    xkb_context_sanitize_rule_names(ctx, &rmlvo);

    if (rmlvo_out) {
        /* Copy the sanitized RMLVO, if required. */
        *rmlvo_out = rmlvo;
    }
    if (!components_out) {
        /* KcCGST not required.
         * While RMLVO and KcCGST outputs are both optional, at least one must
         * be provided. */
        return !!rmlvo_out;
    }

    /* Resolve the RMLVO names to KcCGST components */
    *components_out = (struct xkb_component_names){ 0 };
    return xkb_components_from_rules_names(ctx, &rmlvo, components_out, NULL);
}

static bool
compile_keymap_file(struct xkb_keymap *keymap, XkbFile *file)
{
    if (file->file_type != FILE_TYPE_KEYMAP) {
        log_err(keymap->ctx, XKB_ERROR_KEYMAP_COMPILATION_FAILED,
                "Cannot compile a %s file alone into a keymap\n",
                xkb_file_type_to_string(file->file_type));
        return false;
    }

    if (!CompileKeymap(file, keymap)) {
        log_err(keymap->ctx, XKB_ERROR_KEYMAP_COMPILATION_FAILED,
                "Failed to compile keymap\n");
        return false;
    }

    return true;
}

static bool
text_v1_keymap_new_from_rmlvo(struct xkb_keymap *keymap,
                              const struct xkb_rmlvo_builder *rmlvo)
{
    bool ok;
    struct xkb_component_names kccgst;
    XkbFile *file;

    if (keymap->ctx->log_level >= XKB_LOG_LEVEL_DEBUG) {
        struct xkb_rule_names names = { 0 };
        const size_t buf_size = sizeof(rmlvo->ctx->text_buffer) - 1;
        char *buf = xkb_context_get_buffer(rmlvo->ctx, buf_size);
        if (unlikely(!buf))
            return false;
        ok = xkb_rmlvo_builder_to_rules_names(rmlvo, &names, buf, buf_size);
        if (unlikely(!ok))
            return false;
        log_dbg(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
                "Compiling from RMLVO builder: rules '%s', model '%s', layout '%s', "
                "variant '%s', options '%s'\n",
                names.rules, names.model, names.layout, names.variant,
                names.options);
    }

    /* Resolve the RMLVO components to KcCGST components and get the
     * expected number of layouts */
    ok = xkb_components_from_rmlvo_builder(rmlvo, &kccgst, &keymap->num_groups);
    if (!ok) {
        struct xkb_rule_names names = { 0 };
        const size_t buf_size = sizeof(rmlvo->ctx->text_buffer);
        char *buf = xkb_context_get_buffer(rmlvo->ctx, buf_size);
        if (unlikely(!buf))
            return false;
        ok = xkb_rmlvo_builder_to_rules_names(rmlvo, &names, buf, buf_size);
        if (unlikely(!ok))
            return false;
        log_err(keymap->ctx, XKB_ERROR_KEYMAP_COMPILATION_FAILED,
                "Couldn't look up rules '%s', model '%s', layout '%s', "
                "variant '%s', options '%s'\n",
                names.rules, names.model, names.layout, names.variant,
                names.options);
        return false;
    }

    const xkb_layout_index_t max_groups = format_max_groups(keymap->format);
    if (keymap->num_groups > max_groups)
        keymap->num_groups = max_groups;

    log_dbg(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
            "Compiling from KcCGST: keycodes '%s', types '%s', "
            "compat '%s', symbols '%s'\n",
            kccgst.keycodes, kccgst.types, kccgst.compatibility, kccgst.symbols);

    file = XkbFileFromComponents(keymap->ctx, &kccgst);

    free(kccgst.keycodes);
    free(kccgst.types);
    free(kccgst.compatibility);
    free(kccgst.symbols);
    free(kccgst.geometry);

    if (!file) {
        log_err(keymap->ctx, XKB_ERROR_KEYMAP_COMPILATION_FAILED,
                "Failed to generate parsed XKB file from components\n");
        return false;
    }

    ok = compile_keymap_file(keymap, file);
    FreeXkbFile(file);
    return ok;
}

static bool
text_v1_keymap_new_from_names(struct xkb_keymap *keymap,
                              const struct xkb_rule_names *rmlvo)
{
    bool ok;
    struct xkb_component_names kccgst;
    XkbFile *file;

    log_dbg(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
            "Compiling from RMLVO: rules '%s', model '%s', layout '%s', "
            "variant '%s', options '%s'\n",
            rmlvo->rules, rmlvo->model, rmlvo->layout, rmlvo->variant,
            rmlvo->options);

    /* Resolve the RMLVO components to KcCGST components and get the
     * expected number of layouts */
    ok = xkb_components_from_rules_names(keymap->ctx, rmlvo, &kccgst,
                                   &keymap->num_groups);
    if (!ok) {
        log_err(keymap->ctx, XKB_ERROR_KEYMAP_COMPILATION_FAILED,
                "Couldn't look up rules '%s', model '%s', layout '%s', "
                "variant '%s', options '%s'\n",
                rmlvo->rules, rmlvo->model, rmlvo->layout, rmlvo->variant,
                rmlvo->options);
        return false;
    }

    const xkb_layout_index_t max_groups = format_max_groups(keymap->format);
    if (keymap->num_groups > max_groups)
        keymap->num_groups = max_groups;

    log_dbg(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
            "Compiling from KcCGST: keycodes '%s', types '%s', "
            "compat '%s', symbols '%s'\n",
            kccgst.keycodes, kccgst.types, kccgst.compatibility, kccgst.symbols);

    file = XkbFileFromComponents(keymap->ctx, &kccgst);

    free(kccgst.keycodes);
    free(kccgst.types);
    free(kccgst.compatibility);
    free(kccgst.symbols);
    free(kccgst.geometry);

    if (!file) {
        log_err(keymap->ctx, XKB_ERROR_KEYMAP_COMPILATION_FAILED,
                "Failed to generate parsed XKB file from components\n");
        return false;
    }

    ok = compile_keymap_file(keymap, file);
    FreeXkbFile(file);
    return ok;
}

static bool
text_v1_keymap_new_from_string(struct xkb_keymap *keymap,
                               const char *string, size_t len)
{
    bool ok;
    XkbFile *xkb_file;

    xkb_file = XkbParseString(keymap->ctx, string, len, "(input string)", NULL);
    if (!xkb_file) {
        log_err(keymap->ctx, XKB_ERROR_KEYMAP_COMPILATION_FAILED,
                "Failed to parse input xkb string\n");
        return false;
    }

    ok = compile_keymap_file(keymap, xkb_file);
    FreeXkbFile(xkb_file);
    return ok;
}

static bool
text_v1_keymap_new_from_file(struct xkb_keymap *keymap, FILE *file)
{
    bool ok;
    XkbFile *xkb_file;

    xkb_file = XkbParseFile(keymap->ctx, file, "(unknown file)", NULL);
    if (!xkb_file) {
        log_err(keymap->ctx, XKB_ERROR_KEYMAP_COMPILATION_FAILED,
                "Failed to parse input xkb file\n");
        return false;
    }

    ok = compile_keymap_file(keymap, xkb_file);
    FreeXkbFile(xkb_file);
    return ok;
}

const struct xkb_keymap_format_ops text_v1_keymap_format_ops = {
    .keymap_new_from_rmlvo = text_v1_keymap_new_from_rmlvo,
    .keymap_new_from_names = text_v1_keymap_new_from_names,
    .keymap_new_from_string = text_v1_keymap_new_from_string,
    .keymap_new_from_file = text_v1_keymap_new_from_file,
    .keymap_get_as_string = text_v1_keymap_get_as_string,
};
