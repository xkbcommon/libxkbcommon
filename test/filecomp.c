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
#include <string.h>
#include <errno.h>
#include <X11/X.h>
#include <X11/Xdefs.h>
#include "xkbcommon/xkbcommon.h"
#include "xkbcomp/utils.h"

static char buffer[8192];

int main(int argc, char *argv[])
{
    char *path, *name;
    FILE *file;
    struct xkb_desc * xkb;
    int i, len, from_string = 0;

    /* Require xkb file */
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\n");
        fprintf(stderr, "Usage: %s [-s] XKBFILE [NAME]\n",
                argv[0]);
        exit(1);
    }

    i = 1;
    if (strcmp(argv[1], "-s") == 0) {
	from_string = 2;
	i++;
    }

    path = argv[i];
    name = (argc > i + 1) ? argv[i + 1] : NULL;

    file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file \"%s\": %s\n", path,
                strerror(errno));
        exit(1);
    }

    if (from_string) {
	len = fread(buffer, 1, sizeof buffer, file);
	buffer[len] = '\0';
	xkb = xkb_compile_keymap_from_string(buffer, name);
    } else {
	xkb = xkb_compile_keymap_from_file(file, name);
    }
    fclose(file);

    if (!xkb) {
        fprintf(stderr, "Failed to compile keymap\n");
        exit(1);
    }

    return 0;
}
