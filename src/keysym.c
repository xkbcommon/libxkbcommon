/*
 * Copyright 1985, 1987, 1990, 1998  The Open Group
 * Copyright 2008  Dan Nicholson
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xkb-priv.h"
#include "ks_tables.h"

XKB_EXPORT void
xkb_keysym_get_name(xkb_keysym_t ks, char *buffer, size_t size)
{
    int i, n, h, idx;
    const unsigned char *entry;
    unsigned char val1, val2, val3, val4;

    if ((ks & ((unsigned long) ~0x1fffffff)) != 0) {
        snprintf(buffer, size, "Invalid");
        return;
    }

    /* Try to find it in our hash table. */
    if (ks <= 0x1fffffff) {
        val1 = ks >> 24;
        val2 = (ks >> 16) & 0xff;
        val3 = (ks >> 8) & 0xff;
        val4 = ks & 0xff;
        i = ks % VTABLESIZE;
        h = i + 1;
        n = VMAXHASH;

        while ((idx = hashKeysym[i])) {
            entry = &_XkeyTable[idx];

            if ((entry[0] == val1) && (entry[1] == val2) &&
                (entry[2] == val3) && (entry[3] == val4)) {
                snprintf(buffer, size, "%s", entry + 4);
                return;
            }

            if (!--n)
                break;

            i += h;
            if (i >= VTABLESIZE)
                i -= VTABLESIZE;
        }
    }

    if (ks >= 0x01000100 && ks <= 0x0110ffff)
        /* Unnamed Unicode codepoint. */
        snprintf(buffer, size, "U%lx", ks & 0xffffffUL);
    else
        /* Unnamed, non-Unicode, symbol (shouldn't generally happen). */
        snprintf(buffer, size, "0x%08x", ks);
}

XKB_EXPORT xkb_keysym_t
xkb_keysym_from_name(const char *s)
{
    int i, n, h, c, idx;
    uint32_t sig = 0;
    const char *p = s;
    char *tmp;
    const unsigned char *entry;
    unsigned char sig1, sig2;
    xkb_keysym_t val;

    while ((c = *p++))
        sig = (sig << 1) + c;

    i = sig % KTABLESIZE;
    h = i + 1;
    sig1 = (sig >> 8) & 0xff;
    sig2 = sig & 0xff;
    n = KMAXHASH;

    while ((idx = hashString[i])) {
        entry = &_XkeyTable[idx];

        if ((entry[0] == sig1) && (entry[1] == sig2) &&
            streq(s, (const char *) entry + 6)) {
            val = (entry[2] << 24) | (entry[3] << 16) |
                  (entry[4] << 8) | entry[5];
            if (!val)
                val = XKB_KEY_VoidSymbol;
            return val;
        }

        if (!--n)
            break;

        i += h;
        if (i >= KTABLESIZE)
            i -= KTABLESIZE;
    }

    if (*s == 'U') {
        val = strtoul(&s[1], &tmp, 16);
        if (tmp && *tmp != '\0')
            return XKB_KEY_NoSymbol;

        if (val < 0x20 || (val > 0x7e && val < 0xa0))
            return XKB_KEY_NoSymbol;
        if (val < 0x100)
            return val;
        if (val > 0x10ffff)
            return XKB_KEY_NoSymbol;
        return val | 0x01000000;
    }
    else if (s[0] == '0' && s[1] == 'x') {
        val = strtoul(&s[2], &tmp, 16);
        if (tmp && *tmp != '\0')
            return XKB_KEY_NoSymbol;

        return val;
    }

    /* Stupid inconsistency between the headers and XKeysymDB: the former has
     * no separating underscore, while some XF86* syms in the latter did.
     * As a last ditch effort, try without. */
    if (strncmp(s, "XF86_", 5) == 0) {
        xkb_keysym_t ret;
        tmp = strdup(s);
        if (!tmp)
            return XKB_KEY_NoSymbol;
        memmove(&tmp[4], &tmp[5], strlen(s) - 5 + 1);
        ret = xkb_keysym_from_name(tmp);
        free(tmp);
        return ret;
    }

    return XKB_KEY_NoSymbol;
}

enum keysym_case {
    NONE,
    LOWERCASE,
    UPPERCASE,
};

static enum keysym_case
keysym_get_case(xkb_keysym_t ks)
{
    unsigned set = (ks & (~0xff)) >> 8;

    switch (set) {
    case 0: /* latin 1 */
        if ((ks >= XKB_KEY_A && ks <= XKB_KEY_Z) ||
            (ks >= XKB_KEY_Agrave && ks <= XKB_KEY_THORN && ks !=
             XKB_KEY_multiply))
            return UPPERCASE;
        if ((ks >= XKB_KEY_a && ks <= XKB_KEY_z) ||
            (ks >= XKB_KEY_agrave && ks <= XKB_KEY_ydiaeresis))
            return LOWERCASE;
        break;

    case 1: /* latin 2 */
        if ((ks >= XKB_KEY_Aogonek && ks <= XKB_KEY_Zabovedot && ks !=
             XKB_KEY_breve) ||
            (ks >= XKB_KEY_Racute && ks <= XKB_KEY_Tcedilla))
            return UPPERCASE;
        if ((ks >= XKB_KEY_aogonek && ks <= XKB_KEY_zabovedot && ks !=
             XKB_KEY_caron) ||
            (ks >= XKB_KEY_racute && ks <= XKB_KEY_tcedilla))
            return LOWERCASE;
        break;

    case 2: /* latin 3 */
        if ((ks >= XKB_KEY_Hstroke && ks <= XKB_KEY_Jcircumflex) ||
            (ks >= XKB_KEY_Cabovedot && ks <= XKB_KEY_Scircumflex))
            return UPPERCASE;
        if ((ks >= XKB_KEY_hstroke && ks <= XKB_KEY_jcircumflex) ||
            (ks >= XKB_KEY_cabovedot && ks <= XKB_KEY_scircumflex))
            return LOWERCASE;
        break;

    case 3: /* latin 4 */
        if ((ks >= XKB_KEY_Rcedilla && ks <= XKB_KEY_Tslash) ||
            (ks == XKB_KEY_ENG) ||
            (ks >= XKB_KEY_Amacron && ks <= XKB_KEY_Umacron))
            return UPPERCASE;
        if ((ks >= XKB_KEY_rcedilla && ks <= XKB_KEY_tslash) ||
            (ks == XKB_KEY_eng) ||
            (ks >= XKB_KEY_amacron && ks <= XKB_KEY_umacron))
            return LOWERCASE;
        break;

    case 6: /* Cyrillic */
        if ((ks >= XKB_KEY_Serbian_DJE && ks <= XKB_KEY_Serbian_DZE) ||
            (ks >= XKB_KEY_Cyrillic_YU && ks <= XKB_KEY_Cyrillic_HARDSIGN))
            return UPPERCASE;
        if ((ks >= XKB_KEY_Serbian_dje && ks <= XKB_KEY_Serbian_dze) ||
            (ks >= XKB_KEY_Cyrillic_yu && ks <= XKB_KEY_Cyrillic_hardsign))
            return LOWERCASE;
        break;

    case 7: /* Greek */
        if ((ks >= XKB_KEY_Greek_ALPHAaccent &&
             ks <= XKB_KEY_Greek_OMEGAaccent) ||
            (ks >= XKB_KEY_Greek_ALPHA && ks <= XKB_KEY_Greek_OMEGA))
            return UPPERCASE;
        if ((ks >= XKB_KEY_Greek_alphaaccent &&
             ks <= XKB_KEY_Greek_omegaaccent) ||
            (ks >= XKB_KEY_Greek_alpha && ks <= XKB_KEY_Greek_OMEGA))
            return LOWERCASE;
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
            return UPPERCASE;
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
            return LOWERCASE;
        break;

    case 19: /* latin 9 */
        if (ks == XKB_KEY_OE || ks == XKB_KEY_Ydiaeresis)
            return UPPERCASE;
        if (ks == XKB_KEY_oe)
            return LOWERCASE;
        break;
    }

    return NONE;
}

bool
xkb_keysym_is_lower(xkb_keysym_t keysym)
{
    return keysym_get_case(keysym) == LOWERCASE;
}

bool
xkb_keysym_is_upper(xkb_keysym_t keysym)
{
    return keysym_get_case(keysym) == UPPERCASE;
}

bool
xkb_keysym_is_keypad(xkb_keysym_t keysym)
{
    return keysym >= XKB_KEY_KP_Space && keysym <= XKB_KEY_KP_Equal;
}
