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
#include "parseutils.h"

/* Global warning level */
unsigned int warningLevel = 0;

#define ISEMPTY(str) (!(str) || (strlen(str) == 0))

static XkbFile *
keymap_file_from_components(struct xkb_context *ctx,
                            const struct xkb_component_names *ktcsg)
{
    XkbFile *keycodes, *types, *compat, *symbols;
    IncludeStmt *inc;

    inc = IncludeCreate(ctx, ktcsg->keycodes, MERGE_DEFAULT);
    keycodes = CreateXKBFile(ctx, FILE_TYPE_KEYCODES, NULL,
                             (ParseCommon *) inc, 0);

    inc = IncludeCreate(ctx, ktcsg->types, MERGE_DEFAULT);
    types = CreateXKBFile(ctx, FILE_TYPE_TYPES, NULL,
                          (ParseCommon *) inc, 0);
    AppendStmt(&keycodes->common, &types->common);

    inc = IncludeCreate(ctx, ktcsg->compat, MERGE_DEFAULT);
    compat = CreateXKBFile(ctx, FILE_TYPE_COMPAT, NULL,
                           (ParseCommon *) inc, 0);
    AppendStmt(&keycodes->common, &compat->common);

    inc = IncludeCreate(ctx, ktcsg->symbols, MERGE_DEFAULT);
    symbols = CreateXKBFile(ctx, FILE_TYPE_SYMBOLS, NULL,
                            (ParseCommon *) inc, 0);
    AppendStmt(&keycodes->common, &symbols->common);

    return CreateXKBFile(ctx, FILE_TYPE_KEYMAP, strdup(""),
                         &keycodes->common, 0);
}

/**
 * Compile the given file and store the output in keymap.
 * @param file A list of XkbFiles, each denoting one type (e.g.
 * FILE_TYPE_KEYCODES, etc.)
 */
static struct xkb_keymap *
compile_keymap(struct xkb_context *ctx, XkbFile *file)
{
    bool ok;
    unsigned have = 0;
    const char *main_name;
    struct xkb_keymap *keymap;
    XkbFile *keycodes = NULL;
    XkbFile *types = NULL;
    XkbFile *compat = NULL;
    XkbFile *symbols = NULL;

    keymap = XkbcAllocKeyboard(ctx);
    if (!keymap)
        goto err;

    main_name = file->name ? file->name : "(unnamed)";

    /*
     * Other aggregate file types are converted to FILE_TYPE_KEYMAP
     * in the parser.
     */
    if (file->type != FILE_TYPE_KEYMAP) {
        log_err(ctx, "Cannot compile a %s file alone into a keymap\n",
                XkbcFileTypeText(file->type));
        goto err;
    }

    /* Check for duplicate entries in the input file */
    for (file = (XkbFile *) file->defs; file;
         file = (XkbFile *) file->common.next) {
        if (have & file->type) {
            log_err(ctx,
                    "More than one %s section in a keymap file; "
                    "All sections after the first ignored\n",
                    XkbcFileTypeText(file->type));
            continue;
        }

        switch (file->type) {
        case FILE_TYPE_KEYCODES:
            keycodes = file;
            break;

        case FILE_TYPE_TYPES:
            types = file;
            break;

        case FILE_TYPE_SYMBOLS:
            symbols = file;
            break;

        case FILE_TYPE_COMPAT:
            compat = file;
            break;

        default:
            log_err(ctx, "Cannot define %s in a keymap file\n",
                    XkbcFileTypeText(file->type));
            continue;
        }

        if (!file->topName) {
            free(file->topName);
            file->topName = strdup(main_name);
        }

        have |= file->type;
    }

    if (REQUIRED_FILE_TYPES & (~have)) {
        enum xkb_file_type bit;
        enum xkb_file_type missing;

        missing = REQUIRED_FILE_TYPES & (~have);

        for (bit = 1; missing != 0; bit <<= 1) {
            if (missing & bit) {
                log_err(ctx, "Required section %s missing from keymap\n",
                        XkbcFileTypeText(bit));
                missing &= ~bit;
            }
        }

        goto err;
    }

    ok = CompileKeycodes(keycodes, keymap, MERGE_OVERRIDE);
    if (!ok) {
        log_err(ctx, "Failed to compile keycodes\n");
        goto err;
    }
    ok = CompileKeyTypes(types, keymap, MERGE_OVERRIDE);
    if (!ok) {
        log_err(ctx, "Failed to compile key types\n");
        goto err;
    }
    ok = CompileCompatMap(compat, keymap, MERGE_OVERRIDE);
    if (!ok) {
        log_err(ctx, "Failed to compile compat map\n");
        goto err;
    }
    ok = CompileSymbols(symbols, keymap, MERGE_OVERRIDE);
    if (!ok) {
        log_err(ctx, "Failed to compile symbols\n");
        goto err;
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

struct xkb_keymap *
xkb_map_new_from_kccgst(struct xkb_context *ctx,
                        const struct xkb_component_names *kccgst,
                        enum xkb_map_compile_flags flags)
{
    XkbFile *file;
    struct xkb_keymap *keymap;

    if (!kccgst) {
        log_err(ctx, "No components specified\n");
        return NULL;
    }

    if (ISEMPTY(kccgst->keycodes)) {
        log_err(ctx, "Keycodes required to generate XKB keymap\n");
        return NULL;
    }

    if (ISEMPTY(kccgst->compat)) {
        log_err(ctx, "Compat map required to generate XKB keymap\n");
        return NULL;
    }

    if (ISEMPTY(kccgst->types)) {
        log_err(ctx, "Types required to generate XKB keymap\n");
        return NULL;
    }

    if (ISEMPTY(kccgst->symbols)) {
        log_err(ctx, "Symbols required to generate XKB keymap\n");
        return NULL;
    }

    file = keymap_file_from_components(ctx, kccgst);
    if (!file) {
        log_err(ctx, "Failed to generate parsed XKB file from components\n");
        return NULL;
    }

    keymap = compile_keymap(ctx, file);
    FreeXKBFile(file);
    return keymap;
}

XKB_EXPORT struct xkb_keymap *
xkb_map_new_from_names(struct xkb_context *ctx,
                       const struct xkb_rule_names *rmlvo,
                       enum xkb_map_compile_flags flags)
{
    struct xkb_component_names *kkctgs;
    struct xkb_keymap *keymap;

    if (!rmlvo || ISEMPTY(rmlvo->rules) || ISEMPTY(rmlvo->layout)) {
        log_err(ctx, "rules and layout required to generate XKB keymap\n");
        return NULL;
    }

    kkctgs = xkb_components_from_rules(ctx, rmlvo);
    if (!kkctgs) {
        log_err(ctx, "failed to generate XKB components from rules \"%s\"\n",
                rmlvo->rules);
        return NULL;
    }

    keymap = xkb_map_new_from_kccgst(ctx, kkctgs, 0);

    free(kkctgs->keycodes);
    free(kkctgs->types);
    free(kkctgs->compat);
    free(kkctgs->symbols);
    free(kkctgs);

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
        log_err(ctx, "unsupported keymap format %d\n", format);
        return NULL;
    }

    if (!string) {
        log_err(ctx, "No string specified to generate XKB keymap\n");
        return NULL;
    }

    ok = XKBParseString(ctx, string, "input", &file);
    if (!ok) {
        log_err(ctx, "Failed to parse input xkb file\n");
        return NULL;
    }

    keymap = compile_keymap(ctx, file);
    FreeXKBFile(file);
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

    ok = XKBParseFile(ctx, file, "(unknown file)", &xkb_file);
    if (!ok) {
        log_err(ctx, "Failed to parse input xkb file\n");
        return NULL;
    }

    keymap = compile_keymap(ctx, xkb_file);
    FreeXKBFile(xkb_file);
    return keymap;
}

XKB_EXPORT struct xkb_keymap *
xkb_map_ref(struct xkb_keymap *keymap)
{
    keymap->refcnt++;
    return keymap;
}

XKB_EXPORT void
xkb_map_unref(struct xkb_keymap *keymap)
{
    if (!keymap || --keymap->refcnt > 0)
        return;

    XkbcFreeKeyboard(keymap);
}
