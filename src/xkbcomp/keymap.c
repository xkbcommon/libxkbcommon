/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#include "xkbcomp-priv.h"
#include "indicators.h"

/**
 * Compile the given file and store the output in xkb.
 * @param file A list of XkbFiles, each denoting one type (e.g.
 * FILE_TYPE_KEYCODES, etc.)
 */
struct xkb_keymap *
CompileKeymap(struct xkb_context *ctx, XkbFile *file)
{
    unsigned have = 0;
    bool ok;
    enum xkb_file_type mainType;
    const char *mainName;
    struct xkb_keymap *keymap = XkbcAllocKeyboard(ctx);
    struct {
        XkbFile *keycodes;
        XkbFile *types;
        XkbFile *compat;
        XkbFile *symbols;
    } sections = { NULL };

    if (!keymap)
        return NULL;

    mainType = file->type;
    mainName = file->name ? file->name : "(unnamed)";

    /*
     * Other aggregate file types are converted to FILE_TYPE_KEYMAP
     * in the parser.
     */
    if (mainType != FILE_TYPE_KEYMAP) {
        ERROR("Cannot compile a %s file alone into a keymap\n",
              XkbcFileTypeText(mainType));
        goto err;
    }

    /* Check for duplicate entries in the input file */
    for (file = (XkbFile *) file->defs; file; file = (XkbFile *) file->common.next)
    {
        if (have & file->type) {
            ERROR("More than one %s section in a %s file\n",
                   XkbcFileTypeText(file->type), XkbcFileTypeText(mainType));
            ACTION("All sections after the first ignored\n");
            continue;
        }
        else if (!(file->type & LEGAL_FILE_TYPES)) {
            ERROR("Cannot define %s in a %s file\n",
                   XkbcFileTypeText(file->type), XkbcFileTypeText(mainType));
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
        case FILE_TYPE_GEOMETRY:
            continue;
        default:
            WSGO("Unknown file type %d\n", file->type);
            ACTION("Ignored\n");
            continue;
        case FILE_TYPE_KEYMAP:
            WSGO("Illegal %s configuration in a %s file\n",
                  XkbcFileTypeText(file->type), XkbcFileTypeText(mainType));
            ACTION("Ignored\n");
            continue;
        }

        if (!file->topName || strcmp(file->topName, mainName) != 0) {
            free(file->topName);
            file->topName = strdup(mainName);
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

    return keymap;

err:
    ACTION("Failed to compile keymap\n");
    xkb_map_unref(keymap);
    return NULL;
}
