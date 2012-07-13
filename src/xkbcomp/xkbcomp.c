/*
Copyright 2009  Dan Nicholson

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the authors or their
institutions shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the authors.
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

    inc = IncludeCreate(ktcsg->keycodes, MERGE_DEFAULT);
    keycodes = CreateXKBFile(ctx, FILE_TYPE_KEYCODES, NULL,
                             (ParseCommon *)inc, 0);

    inc = IncludeCreate(ktcsg->types, MERGE_DEFAULT);
    types = CreateXKBFile(ctx, FILE_TYPE_TYPES, NULL,
                          (ParseCommon *)inc, 0);
    AppendStmt(&keycodes->common, &types->common);

    inc = IncludeCreate(ktcsg->compat, MERGE_DEFAULT);
    compat = CreateXKBFile(ctx, FILE_TYPE_COMPAT, NULL,
                           (ParseCommon *)inc, 0);
    AppendStmt(&keycodes->common, &compat->common);

    inc = IncludeCreate(ktcsg->symbols, MERGE_DEFAULT);
    symbols = CreateXKBFile(ctx, FILE_TYPE_SYMBOLS, NULL,
                            (ParseCommon *)inc, 0);
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
    unsigned have = 0;
    bool ok;
    enum xkb_file_type main_type;
    const char *main_name;
    struct xkb_keymap *keymap = XkbcAllocKeyboard(ctx);
    struct {
        XkbFile *keycodes;
        XkbFile *types;
        XkbFile *compat;
        XkbFile *symbols;
    } sections = { NULL };

    if (!keymap)
        return NULL;

    main_type = file->type;
    main_name = file->name ? file->name : "(unnamed)";

    /*
     * Other aggregate file types are converted to FILE_TYPE_KEYMAP
     * in the parser.
     */
    if (main_type != FILE_TYPE_KEYMAP) {
        ERROR("Cannot compile a %s file alone into a keymap\n",
              XkbcFileTypeText(main_type));
        goto err;
    }

    /* Check for duplicate entries in the input file */
    for (file = (XkbFile *)file->defs; file;
         file = (XkbFile *)file->common.next) {
        if (have & file->type) {
            ERROR("More than one %s section in a %s file\n",
                   XkbcFileTypeText(file->type), XkbcFileTypeText(main_type));
            ACTION("All sections after the first ignored\n");
            continue;
        }
        else if (!(file->type & LEGAL_FILE_TYPES)) {
            ERROR("Cannot define %s in a %s file\n",
                   XkbcFileTypeText(file->type), XkbcFileTypeText(main_type));
            continue;
        }

        switch (file->type) {
        case FILE_TYPE_KEYCODES:
            sections.keycodes = file;
            break;
        case FILE_TYPE_TYPES:
            sections.types = file;
            break;
        case FILE_TYPE_SYMBOLS:
            sections.symbols = file;
            break;
        case FILE_TYPE_COMPAT:
            sections.compat = file;
            break;
        default:
            continue;
        }

        if (!file->topName || strcmp(file->topName, main_name) != 0) {
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
                ERROR("Required section %s missing from keymap\n",
                      XkbcFileTypeText(bit));
                missing &= ~bit;
            }
        }

        goto err;
    }

    /* compile the sections we have in the file one-by-one, or fail. */
    if (sections.keycodes == NULL ||
        !CompileKeycodes(sections.keycodes, keymap, MERGE_OVERRIDE))
    {
        ERROR("Failed to compile keycodes\n");
        goto err;
    }
    if (sections.types == NULL ||
        !CompileKeyTypes(sections.types, keymap, MERGE_OVERRIDE))
    {
        ERROR("Failed to compile key types\n");
        goto err;
    }
    if (sections.compat == NULL ||
        !CompileCompatMap(sections.compat, keymap, MERGE_OVERRIDE))
    {
        ERROR("Failed to compile compat map\n");
        goto err;
    }
    if (sections.symbols == NULL ||
        !CompileSymbols(sections.symbols, keymap, MERGE_OVERRIDE))
    {
        ERROR("Failed to compile symbols\n");
        goto err;
    }

    ok = UpdateModifiersFromCompat(keymap);
    if (!ok)
        goto err;

    FreeXKBFile(file);
    return keymap;

err:
    ACTION("Failed to compile keymap\n");
    xkb_map_unref(keymap);
    FreeXKBFile(file);
    return NULL;
}

struct xkb_keymap *
xkb_map_new_from_kccgst(struct xkb_context *ctx,
                        const struct xkb_component_names *kccgst,
                        enum xkb_map_compile_flags flags)
{
    XkbFile *file;

    if (!kccgst) {
        ERROR("no components specified\n");
        return NULL;
    }

    if (ISEMPTY(kccgst->keycodes)) {
        ERROR("keycodes required to generate XKB keymap\n");
        return NULL;
    }

    if (ISEMPTY(kccgst->compat)) {
        ERROR("compat map required to generate XKB keymap\n");
        return NULL;
    }

    if (ISEMPTY(kccgst->types)) {
        ERROR("types required to generate XKB keymap\n");
        return NULL;
    }

    if (ISEMPTY(kccgst->symbols)) {
        ERROR("symbols required to generate XKB keymap\n");
        return NULL;
    }

    file = keymap_file_from_components(ctx, kccgst);
    if (!file) {
        ERROR("failed to generate parsed XKB file from components\n");
        return NULL;
    }

    return compile_keymap(ctx, file);
}

_X_EXPORT struct xkb_keymap *
xkb_map_new_from_names(struct xkb_context *ctx,
                       const struct xkb_rule_names *rmlvo,
                       enum xkb_map_compile_flags flags)
{
    struct xkb_component_names *kkctgs;
    struct xkb_keymap *keymap;

    if (!rmlvo || ISEMPTY(rmlvo->rules) || ISEMPTY(rmlvo->layout)) {
        ERROR("rules and layout required to generate XKB keymap\n");
        return NULL;
    }

    kkctgs = xkb_components_from_rules(ctx, rmlvo);
    if (!kkctgs) {
        ERROR("failed to generate XKB components from rules \"%s\"\n",
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

_X_EXPORT struct xkb_keymap *
xkb_map_new_from_string(struct xkb_context *ctx,
                        const char *string,
                        enum xkb_keymap_format format,
                        enum xkb_map_compile_flags flags)
{
    XkbFile *file;

    if (format != XKB_KEYMAP_FORMAT_TEXT_V1) {
        ERROR("unsupported keymap format %d\n", format);
        return NULL;
    }

    if (!string) {
        ERROR("no string specified to generate XKB keymap\n");
        return NULL;
    }

    if (!XKBParseString(ctx, string, "input", &file)) {
        ERROR("failed to parse input xkb file\n");
        return NULL;
    }

    return compile_keymap(ctx, file);
}

_X_EXPORT struct xkb_keymap *
xkb_map_new_from_file(struct xkb_context *ctx,
                      FILE *file,
                      enum xkb_keymap_format format,
                      enum xkb_map_compile_flags flags)
{
    XkbFile *xkb_file;

    if (format != XKB_KEYMAP_FORMAT_TEXT_V1) {
        ERROR("unsupported keymap format %d\n", format);
        return NULL;
    }

    if (!file) {
        ERROR("no file specified to generate XKB keymap\n");
        return NULL;
    }

    if (!XKBParseFile(ctx, file, "(unknown file)", &xkb_file)) {
        ERROR("failed to parse input xkb file\n");
        return NULL;
    }

    return compile_keymap(ctx, xkb_file);
}

_X_EXPORT struct xkb_keymap *
xkb_map_ref(struct xkb_keymap *keymap)
{
    keymap->refcnt++;
    return keymap;
}

_X_EXPORT void
xkb_map_unref(struct xkb_keymap *keymap)
{
    if (--keymap->refcnt > 0)
        return;

    XkbcFreeKeyboard(keymap);
}
