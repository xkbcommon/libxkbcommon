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

#include <X11/keysym.h>

#include "xkb-priv.h"
#include "alloc.h"

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

unsigned
_XkbcKSCheckCase(xkb_keysym_t ks)
{
    unsigned set = (ks & (~0xff)) >> 8;
    unsigned rtrn = 0;

    switch (set) {
    case 0: /* latin 1 */
        if ((ks >= XK_A && ks <= XK_Z) ||
            (ks >= XK_Agrave && ks <= XK_THORN && ks != XK_multiply))
            rtrn |= _XkbKSUpper;
        if ((ks >= XK_a && ks <= XK_z) ||
            (ks >= XK_agrave && ks <= XK_ydiaeresis))
            rtrn |= _XkbKSLower;
        break;
    case 1: /* latin 2 */
        if ((ks >= XK_Aogonek && ks <= XK_Zabovedot && ks != XK_breve) ||
            (ks >= XK_Racute && ks<=XK_Tcedilla))
            rtrn |= _XkbKSUpper;
        if ((ks >= XK_aogonek && ks <= XK_zabovedot && ks != XK_caron) ||
            (ks >= XK_racute && ks <= XK_tcedilla))
            rtrn |= _XkbKSLower;
        break;
    case 2: /* latin 3 */
        if ((ks >= XK_Hstroke && ks <= XK_Jcircumflex) ||
            (ks >= XK_Cabovedot && ks <= XK_Scircumflex))
            rtrn |= _XkbKSUpper;
        if ((ks >= XK_hstroke && ks <= XK_jcircumflex) ||
            (ks >= XK_cabovedot && ks <= XK_scircumflex))
            rtrn |= _XkbKSLower;
        break;
    case 3: /* latin 4 */
        if ((ks >= XK_Rcedilla && ks <= XK_Tslash) ||
            (ks == XK_ENG) ||
            (ks >= XK_Amacron && ks <= XK_Umacron))
            rtrn |= _XkbKSUpper;
        if ((ks >= XK_rcedilla && ks <= XK_tslash) ||
            (ks == XK_eng) ||
            (ks >= XK_amacron && ks <= XK_umacron))
            rtrn |= _XkbKSLower;
        break;
    case 18: /* latin 8 */
        if ((ks == XK_Wcircumflex) ||
            (ks == XK_Ycircumflex) ||
            (ks == XK_Babovedot) ||
            (ks == XK_Dabovedot) ||
            (ks == XK_Fabovedot) ||
            (ks == XK_Mabovedot) ||
            (ks == XK_Pabovedot) ||
            (ks == XK_Sabovedot) ||
            (ks == XK_Tabovedot) ||
            (ks == XK_Wdiaeresis) ||
            (ks == XK_Ygrave))
            rtrn |= _XkbKSUpper;
        if ((ks == XK_wcircumflex) ||
            (ks == XK_ycircumflex) ||
            (ks == XK_babovedot) ||
            (ks == XK_dabovedot) ||
            (ks == XK_fabovedot) ||
            (ks == XK_mabovedot) ||
            (ks == XK_pabovedot) ||
            (ks == XK_sabovedot) ||
            (ks == XK_tabovedot) ||
            (ks == XK_wdiaeresis) ||
            (ks == XK_ygrave))
            rtrn |= _XkbKSLower;
        break;
    case 19: /* latin 9 */
        if (ks == XK_OE || ks == XK_Ydiaeresis)
            rtrn |= _XkbKSUpper;
        if (ks == XK_oe)
            rtrn |= _XkbKSLower;
        break;
    }

    return rtrn;
}
