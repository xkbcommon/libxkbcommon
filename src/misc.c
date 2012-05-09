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

#include "xkballoc.h"
#include "xkbmisc.h"
#include "xkbcommon/xkbcommon.h"
#include "XKBcommonint.h"

#define mapSize(m) (sizeof(m) / sizeof(struct xkb_kt_map_entry))
static struct xkb_kt_map_entry map2Level[]= {
    { true, ShiftMask, {1, ShiftMask, 0} }
};

static struct xkb_kt_map_entry mapAlpha[]= {
    { true, ShiftMask, { 1, ShiftMask, 0 } },
    { true, LockMask,  { 0, LockMask,  0 } }
};

static struct xkb_mods preAlpha[]= {
    { 0,        0,        0 },
    { LockMask, LockMask, 0 }
};

#define NL_VMOD_MASK 0
static  struct xkb_kt_map_entry mapKeypad[]= {
    { true,  ShiftMask, { 1, ShiftMask, 0 } },
    { false, 0,         { 1, 0, NL_VMOD_MASK } }
};

static struct xkb_key_type canonicalTypes[XkbNumRequiredTypes] = {
    { { 0, 0, 0 },
      1,        /* num_levels */
      0,        /* map_count */
      NULL, NULL,
      NULL, NULL
    },
    { { ShiftMask, ShiftMask, 0 },
      2,        /* num_levels */
      mapSize(map2Level),   /* map_count */
      map2Level, NULL,
      NULL,      NULL
    },
    { { ShiftMask|LockMask, ShiftMask|LockMask, 0 },
      2,        /* num_levels */
      mapSize(mapAlpha),    /* map_count */
      mapAlpha, preAlpha,
      NULL,     NULL
    },
    { { ShiftMask, ShiftMask, NL_VMOD_MASK },
      2,        /* num_levels */
      mapSize(mapKeypad),   /* map_count */
      mapKeypad, NULL,
      NULL,      NULL
    }
};

int
XkbcInitCanonicalKeyTypes(struct xkb_keymap * xkb, unsigned which, int keypadVMod)
{
    struct xkb_client_map * map;
    struct xkb_key_type *from, *to;
    int rtrn;

    if (!xkb)
        return BadMatch;

    rtrn= XkbcAllocClientMap(xkb, XkbKeyTypesMask, XkbNumRequiredTypes);
    if (rtrn != Success)
        return rtrn;

    map= xkb->map;
    if ((which & XkbAllRequiredTypes) == 0)
        return Success;

    rtrn = Success;
    from = canonicalTypes;
    to = map->types;

    if (which & XkbOneLevelMask)
        rtrn = XkbcCopyKeyType(&from[XkbOneLevelIndex], &to[XkbOneLevelIndex]);

    if ((which & XkbTwoLevelMask) && (rtrn == Success))
        rtrn = XkbcCopyKeyType(&from[XkbTwoLevelIndex], &to[XkbTwoLevelIndex]);

    if ((which & XkbAlphabeticMask) && (rtrn == Success))
        rtrn = XkbcCopyKeyType(&from[XkbAlphabeticIndex],
                               &to[XkbAlphabeticIndex]);

    if ((which & XkbKeypadMask) && (rtrn == Success)) {
        struct xkb_key_type * type;

        rtrn = XkbcCopyKeyType(&from[XkbKeypadIndex], &to[XkbKeypadIndex]);
        type = &to[XkbKeypadIndex];

        if ((keypadVMod >= 0) && (keypadVMod < XkbNumVirtualMods) &&
            (rtrn == Success)) {
            type->mods.vmods = (1 << keypadVMod);
            type->map[0].active = true;
            type->map[0].mods.mask = ShiftMask;
            type->map[0].mods.real_mods = ShiftMask;
            type->map[0].mods.vmods = 0;
            type->map[0].level = 1;
            type->map[1].active = false;
            type->map[1].mods.mask = 0;
            type->map[1].mods.real_mods = 0;
            type->map[1].mods.vmods = (1 << keypadVMod);
            type->map[1].level = 1;
        }
    }

    return Success;
}

bool
XkbcVirtualModsToReal(struct xkb_keymap * xkb, unsigned virtual_mask,
                      unsigned *mask_rtrn)
{
    int i, bit;
    unsigned mask;

    if (!xkb)
        return false;
    if (virtual_mask == 0) {
        *mask_rtrn = 0;
        return true;
    }
    if (!xkb->server)
        return false;

    for (i = mask = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1) {
        if (virtual_mask & bit)
            mask |= xkb->server->vmods[i];
    }

    *mask_rtrn = mask;
    return true;
}

/*
 * All latin-1 alphanumerics, plus parens, slash, minus, underscore and
 * wildcards.
 */
static const unsigned char componentSpecLegal[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xa7, 0xff, 0x83,
    0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0xff
};

void
XkbcEnsureSafeMapName(char *name)
{
    if (!name)
        return;

    while (*name!='\0') {
        if ((componentSpecLegal[(*name) / 8] & (1 << ((*name) % 8))) == 0)
            *name= '_';
        name++;
    }
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
