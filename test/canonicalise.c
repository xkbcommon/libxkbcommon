/*
 * Copyright © 2009 Daniel Stone
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#include "X11/extensions/XKBcommon.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    XkbComponentNamesRec *new, *old = NULL;

    if (argc != 6 && argc != 11) {
        fprintf(stderr, "usage: canonicalise (new kccgst) [old kccgst]\n");
        return 1;
    }


    new = calloc(1, sizeof(*new));
    if (!new) {
        fprintf(stderr, "failed to calloc new\n");
        return 1;
    }
    new->keycodes = strdup(argv[1]);
    new->compat = strdup(argv[2]);
    new->geometry = strdup(argv[3]);
    new->symbols = strdup(argv[4]);
    new->types = strdup(argv[5]);

    if (argc == 11) {
        old = calloc(1, sizeof(*old));
        if (!old) {
            fprintf(stderr, "failed to calloc old\n");
            return 1;
        }
        old->keycodes = strdup(argv[6]);
        old->compat = strdup(argv[7]);
        old->geometry = strdup(argv[8]);
        old->symbols = strdup(argv[9]);
        old->types = strdup(argv[10]);
    }

    XkbcCanonicaliseComponents(new, old);

    printf("%s %s %s %s %s\n", new->keycodes, new->compat, new->geometry,
           new->symbols, new->types);

    free(new->keycodes);
    free(new->compat);
    free(new->geometry);
    free(new->symbols);
    free(new->types);
    free(new);

    if (old) {
        free(old->keycodes);
        free(old->compat);
        free(old->geometry);
        free(old->symbols);
        free(old->types);
        free(old);
    }

    return 0;
}
