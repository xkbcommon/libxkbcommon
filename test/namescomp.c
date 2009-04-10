/*
Copyright 2009 Dan Nicholson

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

#include <stdlib.h>
#include <stdio.h>
#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/extensions/XKBstrcommon.h>
#include <X11/extensions/XKBrulescommon.h>
#include "X11/extensions/XKBcommon.h"
#include "xkbcomp/utils.h"

int main(int argc, char *argv[])
{
    XkbComponentNamesRec ktcsg;
    XkbcDescPtr xkb;

    /* Require rmlvo */
    if (argc < 6) {
        fprintf(stderr, "Not enough arguments\n");
        fprintf(stderr, "Usage: %s KEYCODES TYPES COMPAT SYMBOLS GEOMETRY\n",
                argv[0]);
        exit(1);
    }

    ktcsg.keymap = NULL;
    ktcsg.keycodes = argv[1];
    ktcsg.types = argv[2];
    ktcsg.compat = argv[3];
    ktcsg.symbols = argv[4];
    ktcsg.geometry = argv[5];

    uSetErrorFile(NULL);
    XkbcInitAtoms();

    xkb = XkbcCompileKeymapFromComponents(&ktcsg);

    if (!xkb) {
        fprintf(stderr, "Failed to compile keymap\n");
        exit(1);
    }

    return 0;
}
