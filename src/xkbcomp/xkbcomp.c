/*
 * Copyright 2009  Dan Nicholson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors or their
 * institutions shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the authors.
 */

#include "xkbcomp-priv.h"
#include "rules.h"

typedef bool (*compile_file_fn)(XkbFile *file,
                                struct xkb_keymap *keymap,
                                enum merge_mode merge);

static const compile_file_fn compile_file_fns[LAST_KEYMAP_FILE_TYPE + 1] = {
    [FILE_TYPE_KEYCODES] = CompileKeycodes,
    [FILE_TYPE_TYPES] = CompileKeyTypes,
    [FILE_TYPE_COMPAT] = CompileCompatMap,
    [FILE_TYPE_SYMBOLS] = CompileSymbols,
};

static struct xkb_keymap *
compile_keymap_file(struct xkb_context *ctx, XkbFile *file)
{
    bool ok;
    const char *main_name;
    struct xkb_keymap *keymap;
    XkbFile *files[LAST_KEYMAP_FILE_TYPE + 1] = { NULL };
    enum xkb_file_type type;

    keymap = xkb_map_new(ctx);
    if (!keymap)
        goto err;

    main_name = file->name ? file->name : "(unnamed)";

    if (file->file_type != FILE_TYPE_KEYMAP) {
        log_err(ctx, "Cannot compile a %s file alone into a keymap\n",
                xkb_file_type_to_string(file->file_type));
        goto err;
    }

    /* Collect section files and check for duplicates. */
    for (file = (XkbFile *) file->defs; file;
         file = (XkbFile *) file->common.next) {
        if (file->file_type < FIRST_KEYMAP_FILE_TYPE ||
            file->file_type > LAST_KEYMAP_FILE_TYPE) {
            log_err(ctx, "Cannot define %s in a keymap file\n",
                    xkb_file_type_to_string(file->file_type));
            continue;
        }

        if (files[file->file_type]) {
            log_err(ctx,
                    "More than one %s section in keymap file; "
                    "All sections after the first ignored\n",
                    xkb_file_type_to_string(file->file_type));
            continue;
        }

        if (!file->topName) {
            free(file->topName);
            file->topName = strdup(main_name);
        }

        files[file->file_type] = file;
    }

    /*
     * Check that all required section were provided.
     * Report everything before failing.
     */
    ok = true;
    for (type = FIRST_KEYMAP_FILE_TYPE;
         type <= LAST_KEYMAP_FILE_TYPE;
         type++) {
        if (files[type] == NULL) {
            log_err(ctx, "Required section %s missing from keymap\n",
                    xkb_file_type_to_string(type));
            ok = false;
        }
    }
    if (!ok)
        goto err;

    /* Compile sections. */
    for (type = FIRST_KEYMAP_FILE_TYPE;
         type <= LAST_KEYMAP_FILE_TYPE;
         type++) {
        log_dbg(ctx, "Compiling %s \"%s\"\n",
                xkb_file_type_to_string(type), files[type]->topName);
        ok = compile_file_fns[type](files[type], keymap, MERGE_OVERRIDE);
        if (!ok) {
            log_err(ctx, "Failed to compile %s\n",
                    xkb_file_type_to_string(type));
            goto err;
        }
    }

    ok = UpdateModifiersFromCompat(keymap);
    if (!ok)
        goto err;

    return keymap;

err:
    log_err(ctx, "Failed to compile keymap\n");
    xkb_map_unref(keymap);
    return NULL;
}

XKB_EXPORT struct xkb_keymap *
xkb_map_new_from_names(struct xkb_context *ctx,
                       const struct xkb_rule_names *rmlvo_in,
                       enum xkb_map_compile_flags flags)
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

    ok = xkb_components_from_rules(ctx, &rmlvo, &kccgst);
    if (!ok) {
        log_err(ctx,
                "Couldn't look up rules '%s', model '%s', layout '%s', "
                "variant '%s', options '%s'\n",
                rmlvo.rules, rmlvo.model, rmlvo.layout, rmlvo.variant,
                rmlvo.options);
        return NULL;
    }

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

    keymap = compile_keymap_file(ctx, file);
    FreeXkbFile(file);
    return keymap;
}

XKB_EXPORT struct xkb_keymap *
xkb_map_new_from_string(struct xkb_context *ctx,
                        const char *string,
                        enum xkb_keymap_format format,
                        enum xkb_map_compile_flags flags)
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

    keymap = compile_keymap_file(ctx, file);
    FreeXkbFile(file);
    return keymap;
}

XKB_EXPORT struct xkb_keymap *
xkb_map_new_from_file(struct xkb_context *ctx,
                      FILE *file,
                      enum xkb_keymap_format format,
                      enum xkb_map_compile_flags flags)
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

    keymap = compile_keymap_file(ctx, xkb_file);
    FreeXkbFile(xkb_file);
    return keymap;
}
