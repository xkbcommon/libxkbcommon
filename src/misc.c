/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

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

#include "xkb-priv.h"
#include "alloc.h"

static struct xkb_kt_map_entry map2Level[]= {
    {
        .active = true,
        .level = ShiftMask,
        .mods = {.mask = 1, .vmods = ShiftMask, .real_mods = 0 }
    }
};

static struct xkb_kt_map_entry mapAlpha[]= {
    {
        .active = true,
        .level = ShiftMask,
        .mods = { .mask = 1, .vmods = ShiftMask, .real_mods = 0 }
    },
    {
        .active = true,
        .level = LockMask,
        .mods = { .mask = 0, .vmods = LockMask,  .real_mods = 0 }
    }
};

static struct xkb_mods preAlpha[]= {
    { .mask = 0,        .vmods = 0,        .real_mods = 0 },
    { .mask = LockMask, .vmods = LockMask, .real_mods = 0 }
};

#define NL_VMOD_MASK 0
static struct xkb_kt_map_entry mapKeypad[]= {
    {
        .active = true,
        .level = ShiftMask,
        .mods = { .mask = 1, .vmods = ShiftMask, .real_mods = 0 }
    },
    {
        .active = false,
        .level = 0,
        .mods = { .mask = 1, .vmods = 0, .real_mods = NL_VMOD_MASK }
    }
};

static const struct xkb_key_type canonicalTypes[XkbNumRequiredTypes] = {
    {
        .mods = { .mask = 0, .vmods = 0, .real_mods = 0 },
        .num_levels = 1,
        .preserve = NULL,
        .name = NULL,
        .level_names = NULL
    },
    {
        .mods = { .mask = ShiftMask, .vmods = ShiftMask, .real_mods = 0 },
        .num_levels = 2,
        .map = darray_lit(map2Level),
        .preserve = NULL,
        .name = NULL,
        .level_names = NULL
    },
    {
        .mods = { .mask = ShiftMask|LockMask, .vmods = ShiftMask|LockMask, .real_mods = 0 },
        .num_levels = 2,
        .map = darray_lit(mapAlpha),
        .preserve = preAlpha,
        .name = NULL,
        .level_names = NULL
    },
    {
        .mods = { .mask = ShiftMask, .vmods = ShiftMask, .real_mods = NL_VMOD_MASK },
        .num_levels = 2,
        .map = darray_lit(mapKeypad),
        .preserve = NULL,
        .name = NULL,
        .level_names = NULL
    }
};

int
XkbcInitCanonicalKeyTypes(struct xkb_keymap *keymap, unsigned which,
                          int keypadVMod)
{
    struct xkb_client_map * map;
    const struct xkb_key_type *from;
    int rtrn;

    if (!keymap)
        return BadMatch;

    rtrn = XkbcAllocClientMap(keymap, XkbKeyTypesMask, XkbNumRequiredTypes);
    if (rtrn != Success)
        return rtrn;

    map = keymap->map;
    if ((which & XkbAllRequiredTypes) == 0)
        return Success;

    rtrn = Success;
    from = canonicalTypes;

    if (which & XkbOneLevelMask)
        rtrn = XkbcCopyKeyType(&from[XkbOneLevelIndex],
                               &darray_item(map->types, XkbOneLevelIndex));

    if ((which & XkbTwoLevelMask) && (rtrn == Success))
        rtrn = XkbcCopyKeyType(&from[XkbTwoLevelIndex],
                               &darray_item(map->types, XkbTwoLevelIndex));

    if ((which & XkbAlphabeticMask) && (rtrn == Success))
        rtrn = XkbcCopyKeyType(&from[XkbAlphabeticIndex],
                               &darray_item(map->types, XkbAlphabeticIndex));

    if ((which & XkbKeypadMask) && (rtrn == Success)) {
        struct xkb_key_type * type;

        rtrn = XkbcCopyKeyType(&from[XkbKeypadIndex],
                               &darray_item(map->types, XkbKeypadIndex));
        type = &darray_item(map->types, XkbKeypadIndex);

        if ((keypadVMod >= 0) && (keypadVMod < XkbNumVirtualMods) &&
            (rtrn == Success)) {
            struct xkb_kt_map_entry *entry;
            type->mods.vmods = (1 << keypadVMod);

            entry = &darray_item(type->map, 0);
            entry->active = true;
            entry->mods.mask = ShiftMask;
            entry->mods.real_mods = ShiftMask;
            entry->mods.vmods = 0;
            entry->level = 1;

            entry = &darray_item(type->map, 1);
            entry->active = false;
            entry->mods.mask = 0;
            entry->mods.real_mods = 0;
            entry->mods.vmods = (1 << keypadVMod);
            entry->level = 1;
        }
    }

    return Success;
}

unsigned
_XkbcKSCheckCase(xkb_keysym_t ks)
{
    unsigned set = (ks & (~0xff)) >> 8;
    unsigned rtrn = 0;

    switch (set) {
    case 0: /* latin 1 */
        if ((ks >= XKB_KEY_A && ks <= XKB_KEY_Z) ||
            (ks >= XKB_KEY_Agrave && ks <= XKB_KEY_THORN && ks != XKB_KEY_multiply))
            rtrn |= _XkbKSUpper;
        if ((ks >= XKB_KEY_a && ks <= XKB_KEY_z) ||
            (ks >= XKB_KEY_agrave && ks <= XKB_KEY_ydiaeresis))
            rtrn |= _XkbKSLower;
        break;
    case 1: /* latin 2 */
        if ((ks >= XKB_KEY_Aogonek && ks <= XKB_KEY_Zabovedot && ks != XKB_KEY_breve) ||
            (ks >= XKB_KEY_Racute && ks<=XKB_KEY_Tcedilla))
            rtrn |= _XkbKSUpper;
        if ((ks >= XKB_KEY_aogonek && ks <= XKB_KEY_zabovedot && ks != XKB_KEY_caron) ||
            (ks >= XKB_KEY_racute && ks <= XKB_KEY_tcedilla))
            rtrn |= _XkbKSLower;
        break;
    case 2: /* latin 3 */
        if ((ks >= XKB_KEY_Hstroke && ks <= XKB_KEY_Jcircumflex) ||
            (ks >= XKB_KEY_Cabovedot && ks <= XKB_KEY_Scircumflex))
            rtrn |= _XkbKSUpper;
        if ((ks >= XKB_KEY_hstroke && ks <= XKB_KEY_jcircumflex) ||
            (ks >= XKB_KEY_cabovedot && ks <= XKB_KEY_scircumflex))
            rtrn |= _XkbKSLower;
        break;
    case 3: /* latin 4 */
        if ((ks >= XKB_KEY_Rcedilla && ks <= XKB_KEY_Tslash) ||
            (ks == XKB_KEY_ENG) ||
            (ks >= XKB_KEY_Amacron && ks <= XKB_KEY_Umacron))
            rtrn |= _XkbKSUpper;
        if ((ks >= XKB_KEY_rcedilla && ks <= XKB_KEY_tslash) ||
            (ks == XKB_KEY_eng) ||
            (ks >= XKB_KEY_amacron && ks <= XKB_KEY_umacron))
            rtrn |= _XkbKSLower;
        break;
    case 18: /* latin 8 */
        if ((ks == XKB_KEY_Wcircumflex) ||
            (ks == XKB_KEY_Ycircumflex) ||
            (ks == XKB_KEY_Babovedot) ||
            (ks == XKB_KEY_Dabovedot) ||
            (ks == XKB_KEY_Fabovedot) ||
            (ks == XKB_KEY_Mabovedot) ||
            (ks == XKB_KEY_Pabovedot) ||
            (ks == XKB_KEY_Sabovedot) ||
            (ks == XKB_KEY_Tabovedot) ||
            (ks == XKB_KEY_Wdiaeresis) ||
            (ks == XKB_KEY_Ygrave))
            rtrn |= _XkbKSUpper;
        if ((ks == XKB_KEY_wcircumflex) ||
            (ks == XKB_KEY_ycircumflex) ||
            (ks == XKB_KEY_babovedot) ||
            (ks == XKB_KEY_dabovedot) ||
            (ks == XKB_KEY_fabovedot) ||
            (ks == XKB_KEY_mabovedot) ||
            (ks == XKB_KEY_pabovedot) ||
            (ks == XKB_KEY_sabovedot) ||
            (ks == XKB_KEY_tabovedot) ||
            (ks == XKB_KEY_wdiaeresis) ||
            (ks == XKB_KEY_ygrave))
            rtrn |= _XkbKSLower;
        break;
    case 19: /* latin 9 */
        if (ks == XKB_KEY_OE || ks == XKB_KEY_Ydiaeresis)
            rtrn |= _XkbKSUpper;
        if (ks == XKB_KEY_oe)
            rtrn |= _XkbKSLower;
        break;
    }

    return rtrn;
}

xkb_keycode_t
XkbcFindKeycodeByName(struct xkb_keymap *keymap, const char *name,
                      bool use_aliases)
{
    struct xkb_key_alias *alias;
    xkb_keycode_t i;

    for (i = keymap->min_key_code; i <= keymap->max_key_code; i++) {
        if (strncmp(darray_item(keymap->names->keys, i).name, name,
                    XkbKeyNameLength) == 0)
            return i;
    }

    if (!use_aliases)
        return 0;


    darray_foreach(alias, keymap->names->key_aliases)
        if (strncmp(name, alias->alias, XkbKeyNameLength) == 0)
            return XkbcFindKeycodeByName(keymap, alias->real, false);

    return 0;
}
