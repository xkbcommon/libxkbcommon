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
 * XkmKeyNamesIdx, etc.)
 */
struct xkb_keymap *
CompileKeymap(struct xkb_context *context, XkbFile *file)
{
    unsigned have;
    bool ok;
    unsigned required, legal;
    unsigned mainType;
    const char *mainName;
    LEDInfo *unbound = NULL, *next;
    struct xkb_keymap *keymap = XkbcAllocKeyboard(context);
    struct {
        XkbFile *keycodes;
        XkbFile *types;
        XkbFile *compat;
        XkbFile *symbols;
    } sections;

    if (!keymap)
        return NULL;

    memset(&sections, 0, sizeof(sections));
    mainType = file->type;
    mainName = file->name ? file->name : "(unnamed)";
    switch (mainType)
    {
    case XkmSemanticsFile:
        required = XkmSemanticsRequired;
        legal = XkmSemanticsLegal;
        break;
    case XkmLayoutFile:        /* standard type  if setxkbmap -print */
        required = XkmLayoutRequired;
        legal = XkmKeymapLegal;
        break;
    case XkmKeymapFile:
        required = XkmKeyNamesIndex | XkmTypesIndex | XkmSymbolsIndex |
                   XkmCompatMapIndex;
        legal = XkmKeymapLegal;
        break;
    default:
        ERROR("Cannot compile %s alone into an XKM file\n",
               XkbcConfigText(mainType));
        return false;
    }
    have = 0;
    ok = 1;
    /* Check for duplicate entries in the input file */
    for (file = (XkbFile *) file->defs; file; file = (XkbFile *) file->common.next)
    {
        if ((have & (1 << file->type)) != 0)
        {
            ERROR("More than one %s section in a %s file\n",
                   XkbcConfigText(file->type), XkbcConfigText(mainType));
            ACTION("All sections after the first ignored\n");
            continue;
        }
        else if ((1 << file->type) & (~legal))
        {
            ERROR("Cannot define %s in a %s file\n",
                   XkbcConfigText(file->type), XkbcConfigText(mainType));
            continue;
        }

        switch (file->type)
        {
        case XkmKeyNamesIndex:
            sections.keycodes = file;
            break;
        case XkmTypesIndex:
            sections.types = file;
            break;
        case XkmSymbolsIndex:
            sections.symbols = file;
            break;
        case XkmCompatMapIndex:
            sections.compat = file;
            break;
        case XkmGeometryIndex:
            continue;
        default:
            WSGO("Unknown file type %d\n", file->type);
            ACTION("Ignored\n");
            continue;
        case XkmSemanticsFile:
        case XkmLayoutFile:
        case XkmKeymapFile:
            WSGO("Illegal %s configuration in a %s file\n",
                  XkbcConfigText(file->type), XkbcConfigText(mainType));
            ACTION("Ignored\n");
            continue;
        }

        if (!file->topName || strcmp(file->topName, mainName) != 0) {
            free(file->topName);
            file->topName = strdup(mainName);
        }

        have |= (1 << file->type);
    }

    if (required & (~have))
    {
        int i, bit;
        unsigned missing;
        missing = required & (~have);
        for (i = 0, bit = 1; missing != 0; i++, bit <<= 1)
        {
            if (missing & bit)
            {
                ERROR("Required section %s missing from keymap\n", XkbcConfigText(i));
                missing &= ~bit;
            }
        }
        goto err;
    }

    /* compile the sections we have in the file one-by-one, or fail. */
    if (sections.keycodes == NULL ||
        !CompileKeycodes(sections.keycodes, keymap, MergeOverride))
    {
        ERROR("Failed to compile keycodes\n");
        goto err;
    }
    if (sections.types == NULL ||
        !CompileKeyTypes(sections.types, keymap, MergeOverride))
    {
        ERROR("Failed to compile key types\n");
        goto err;
    }
    if (sections.compat == NULL ||
        !CompileCompatMap(sections.compat, keymap, MergeOverride, &unbound))
    {
        ERROR("Failed to compile compat map\n");
        goto err;
    }
    if (sections.symbols == NULL ||
        !CompileSymbols(sections.symbols, keymap, MergeOverride))
    {
        ERROR("Failed to compile symbols\n");
        goto err;
    }

    ok = BindIndicators(keymap, true, unbound, NULL);
    if (!ok)
        goto err;

    ok = UpdateModifiersFromCompat(keymap);
    if (!ok)
        goto err;

    return keymap;

err:
    ACTION("Failed to compile keymap\n");
    xkb_map_unref(keymap);
    while (unbound) {
        next = (LEDInfo *) unbound->defs.next;
        free(unbound);
        unbound = next;
    }
    return NULL;
}
