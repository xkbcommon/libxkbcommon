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
    case 6: /* Cyrillic */
        if ((ks >= XKB_KEY_Serbian_DJE && ks <= XKB_KEY_Serbian_DZE) ||
            (ks >= XKB_KEY_Cyrillic_YU && ks <= XKB_KEY_Cyrillic_HARDSIGN))
            rtrn |= _XkbKSUpper;
        if ((ks >= XKB_KEY_Serbian_dje && ks <= XKB_KEY_Serbian_dze) ||
            (ks >= XKB_KEY_Cyrillic_yu && ks <= XKB_KEY_Cyrillic_hardsign))
            rtrn |= _XkbKSLower;
        break;
    case 7: /* Greek */
        if ((ks >= XKB_KEY_Greek_ALPHAaccent &&
             ks <= XKB_KEY_Greek_OMEGAaccent) ||
            (ks >= XKB_KEY_Greek_ALPHA && ks <= XKB_KEY_Greek_OMEGA))
            rtrn |= _XkbKSUpper;
        if ((ks >= XKB_KEY_Greek_alphaaccent &&
             ks <= XKB_KEY_Greek_omegaaccent) ||
            (ks >= XKB_KEY_Greek_alpha && ks <= XKB_KEY_Greek_OMEGA))
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
