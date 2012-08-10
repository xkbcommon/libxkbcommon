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

#define ISEMPTY(str) (!(str) || (strlen(str) == 0))

static XkbFile *
keymap_file_from_names(struct xkb_context *ctx,
                       const struct xkb_rule_names *rmlvo)
{
    struct xkb_component_names *kkctgs;
    XkbFile *keycodes, *types, *compat, *symbols;
    IncludeStmt *inc;

    kkctgs = xkb_components_from_rules(ctx, rmlvo);
    if (!kkctgs) {
        log_err(ctx,
                "couldn't look up rules '%s', model '%s', layout '%s', "
                "variant '%s', options '%s'\n",
                rmlvo->rules, rmlvo->model, rmlvo->layout, rmlvo->variant,
                rmlvo->options);
        return NULL;
    }

    inc = IncludeCreate(ctx, kkctgs->keycodes, MERGE_DEFAULT);
    keycodes = CreateXKBFile(ctx, FILE_TYPE_KEYCODES, NULL,
                             (ParseCommon *) inc, 0);

    inc = IncludeCreate(ctx, kkctgs->types, MERGE_DEFAULT);
    types = CreateXKBFile(ctx, FILE_TYPE_TYPES, NULL,
                          (ParseCommon *) inc, 0);
    AppendStmt(&keycodes->common, &types->common);

    inc = IncludeCreate(ctx, kkctgs->compat, MERGE_DEFAULT);
    compat = CreateXKBFile(ctx, FILE_TYPE_COMPAT, NULL,
                           (ParseCommon *) inc, 0);
    AppendStmt(&keycodes->common, &compat->common);

    inc = IncludeCreate(ctx, kkctgs->symbols, MERGE_DEFAULT);
    symbols = CreateXKBFile(ctx, FILE_TYPE_SYMBOLS, NULL,
                            (ParseCommon *) inc, 0);
    AppendStmt(&keycodes->common, &symbols->common);

    free(kkctgs->keycodes);
    free(kkctgs->types);
    free(kkctgs->compat);
    free(kkctgs->symbols);
    free(kkctgs);

    return CreateXKBFile(ctx, FILE_TYPE_KEYMAP, strdup(""),
                         &keycodes->common, 0);
}

static struct xkb_keymap *
new_keymap(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    keymap = calloc(1, sizeof(*keymap));
    if (!keymap)
        return NULL;

    keymap->refcnt = 1;
    keymap->ctx = xkb_context_ref(ctx);

    return keymap;
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

    keymap = new_keymap(ctx);
    if (!keymap)
        goto err;

    main_name = file->name ? file->name : "(unnamed)";

    /*
     * Other aggregate file types are converted to FILE_TYPE_KEYMAP
     * in the parser.
     */
    if (file->file_type != FILE_TYPE_KEYMAP) {
        log_err(ctx, "Cannot compile a %s file alone into a keymap\n",
                FileTypeText(file->file_type));
        goto err;
    }

    /* Check for duplicate entries in the input file */
    for (file = (XkbFile *) file->defs; file;
         file = (XkbFile *) file->common.next) {
        if (have & file->file_type) {
            log_err(ctx,
                    "More than one %s section in a keymap file; "
                    "All sections after the first ignored\n",
                    FileTypeText(file->file_type));
            continue;
        }

        switch (file->file_type) {
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
                    FileTypeText(file->file_type));
            continue;
        }

        if (!file->topName) {
            free(file->topName);
            file->topName = strdup(main_name);
        }

        have |= file->file_type;
    }

    if (REQUIRED_FILE_TYPES & (~have)) {
        enum xkb_file_type bit;
        enum xkb_file_type missing;

        missing = REQUIRED_FILE_TYPES & (~have);

        for (bit = 1; missing != 0; bit <<= 1) {
            if (missing & bit) {
                log_err(ctx, "Required section %s missing from keymap\n",
                        FileTypeText(bit));
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

XKB_EXPORT struct xkb_keymap *
xkb_map_new_from_names(struct xkb_context *ctx,
                       const struct xkb_rule_names *rmlvo_in,
                       enum xkb_map_compile_flags flags)
{
    struct xkb_keymap *keymap = NULL;
    struct xkb_rule_names rmlvo = *rmlvo_in;
    XkbFile *file;

    if (ISEMPTY(rmlvo.rules))
        rmlvo.rules = DEFAULT_XKB_RULES;
    if (ISEMPTY(rmlvo.model))
        rmlvo.model = DEFAULT_XKB_MODEL;
    if (ISEMPTY(rmlvo.layout))
        rmlvo.layout = DEFAULT_XKB_LAYOUT;

    file = keymap_file_from_names(ctx, &rmlvo);
    if (!file) {
        log_err(ctx, "Failed to generate parsed XKB file from components\n");
        return NULL;
    }

    keymap = compile_keymap(ctx, file);
    FreeXKBFile(file);

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
    unsigned int i;
    struct xkb_key *key;

    if (!keymap || --keymap->refcnt > 0)
        return;

    for (i = 0; i < keymap->num_types; i++) {
        free(keymap->types[i].map);
        free(keymap->types[i].level_names);
    }
    free(keymap->types);

    darray_foreach(key, keymap->keys) {
        free(key->sym_index);
        free(key->num_syms);
        darray_free(key->syms);
        free(key->actions);
    }
    darray_free(keymap->keys);

    darray_free(keymap->sym_interpret);

    darray_free(keymap->key_aliases);

    free(keymap->keycodes_section_name);
    free(keymap->symbols_section_name);
    free(keymap->types_section_name);
    free(keymap->compat_section_name);

    xkb_context_unref(keymap->ctx);

    free(keymap);
}
