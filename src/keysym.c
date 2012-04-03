/*
Copyright 1985, 1987, 1990, 1998  The Open Group
Copyright 2008  Dan Nicholson

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/keysymdef.h>
#include "xkbmisc.h"
#include "xkbcommon/xkbcommon.h"
#include <stdlib.h>
#include <string.h>

#include "ks_tables.h"

void
xkb_keysym_to_string(xkb_keysym_t ks, char *buffer, size_t size)
{
    int i, n, h, idx;
    const unsigned char *entry;
    unsigned char val1, val2, val3, val4;

    if ((ks & ((unsigned long) ~0x1fffffff)) != 0) {
        snprintf(buffer, size, "Invalid");
        return;
    }

    /* Not listed in keysymdef.h for hysterical raisins. */
    if (ks == XKB_KEYSYM_NO_SYMBOL) {
        snprintf(buffer, size, "NoSymbol");
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

xkb_keysym_t
xkb_string_to_keysym(const char *s)
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
            !strcmp(s, (const char *)entry + 6))
        {
            val = (entry[2] << 24) | (entry[3] << 16) |
                  (entry[4] << 8)  | entry[5];
            if (!val)
                val = XK_VoidSymbol;
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
            return XKB_KEYSYM_NO_SYMBOL;

        if (val < 0x20 || (val > 0x7e && val < 0xa0))
            return XKB_KEYSYM_NO_SYMBOL;
        if (val < 0x100)
            return val;
        if (val > 0x10ffff)
            return XKB_KEYSYM_NO_SYMBOL;
        return val | 0x01000000;
    }
    else if (s[0] == '0' && s[1] == 'x') {
        val = strtoul(&s[2], &tmp, 16);
        if (tmp && *tmp != '\0')
            return XKB_KEYSYM_NO_SYMBOL;

        return val;
    }

    /* Stupid inconsistency between the headers and XKeysymDB: the former has
     * no separating underscore, while some XF86* syms in the latter did.
     * As a last ditch effort, try without. */
    if (strncmp(s, "XF86_", 5) == 0) {
        xkb_keysym_t ret;
        tmp = strdup(s);
        if (!tmp)
            return XKB_KEYSYM_NO_SYMBOL;
        memmove(&tmp[4], &tmp[5], strlen(s) - 5 + 1);
        ret = xkb_string_to_keysym(tmp);
        free(tmp);
        return ret;
    }

    return XKB_KEYSYM_NO_SYMBOL;
}
