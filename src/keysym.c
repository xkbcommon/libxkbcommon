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
#include <X11/X.h>
#include <X11/keysymdef.h>
#include "X11/extensions/XKBcommon.h"
#include <stdlib.h>
#include <string.h>

#include "ks_tables.h"

char *
XkbcKeysymToString(KeySym ks)
{
    int i, n, h, idx;
    const unsigned char *entry;
    unsigned char val1, val2, val3, val4;

    if (!ks || (ks & ((unsigned long) ~0x1fffffff)) != 0)
        return NULL;

    if (ks == XK_VoidSymbol)
        ks = 0;

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
                (entry[2] == val3) && (entry[3] == val4))
                return ((char *)entry + 4);

            if (!--n)
                break;

            i += h;
            if (i >= VTABLESIZE)
                i -= VTABLESIZE;
        }
    }

    if (ks >= 0x01000100 && ks <= 0x0110ffff) {
        KeySym val = ks & 0xffffff;
        char *s;
        int i;

        if (val & 0xff0000)
            i = 10;
        else
            i = 6;

        s = malloc(i);
        if (!s)
            return s;

        i--;
        s[i--] = '\0';
        for (; i; i--) {
            val1 = val & 0xf;
            val >>= 4;
            if (val1 < 10)
                s[i] = '0' + val1;
            else
                s[i] = 'A' + val1 - 10;
        }
        s[i] = 'U';
        return s;
    }

    return NULL;
}

KeySym
XkbcStringToKeysym(const char *s)
{
    int i, n, h, c, idx;
    unsigned long sig = 0;
    const char *p = s;
    const unsigned char *entry;
    unsigned char sig1, sig2;
    KeySym val;

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
            !strcmp(s, (char *)entry + 6))
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
        val = 0;

        for (p = &s[1]; *p; p++) {
            c = *p;

            if ('0' <= c && c <= '9')
                val = (val << 4) + c - '0';
            else if ('a' <= c && c <= 'f')
                val = (val << 4) + c - 'a' + 10;
            else if ('A' <= c && c <= 'F')
                val = (val << 4) + c - 'A' + 10;
            else
                return NoSymbol;

            if (val > 0x10ffff)
                return NoSymbol;
        }

        if (val < 0x20 || (val > 0x7e && val < 0xa0))
            return NoSymbol;
        if (val < 0x100)
            return val;
        return val | 0x01000000;
    }

    return NoSymbol;
}
