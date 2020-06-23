/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
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
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"
#include "xkbcommon/xkbcommon.h"

int
main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    int opt;
    struct xkb_context *ctx = NULL;
    struct xkb_keymap *keymap = NULL;
    const char *keymap_path = NULL;
    FILE *file = NULL;
    char *dump;

    while ((opt = getopt(argc, argv, "h")) != -1) {
        switch (opt) {
        case 'h':
        case '?':
            fprintf(stderr, "Usage: %s <path to keymap file>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: missing path to keymap file\n");
        exit(EXIT_FAILURE);
    }

    keymap_path = argv[optind];

    ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx) {
        fprintf(stderr, "Couldn't create xkb context\n");
        goto out;
    }

    if (streq(keymap_path, "-")) {
        FILE *tmp;

        tmp = tmpfile();
        if (!tmp) {
            fprintf(stderr, "Failed to create tmpfile\n");
            goto out;
        }

        while (true) {
            char buf[4096];
            size_t len;

            len = fread(buf, 1, sizeof(buf), stdin);
            if (ferror(stdin)) {
                fprintf(stderr, "Failed to read from stdin\n");
                goto out;
            }
            if (len > 0) {
                size_t wlen = fwrite(buf, 1, len, tmp);
                if (wlen != len) {
                    fprintf(stderr, "Failed to write to tmpfile\n");
                    goto out;
                }
            }
            if (feof(stdin))
                break;
        }
        fseek(tmp, 0, SEEK_SET);
        file = tmp;
    } else {
        file = fopen(keymap_path, "rb");
        if (!file) {
            fprintf(stderr, "Failed to open path: %s\n", keymap_path);
            goto out;
        }
    }

    keymap = xkb_keymap_new_from_file(ctx, file,
                                      XKB_KEYMAP_FORMAT_TEXT_V1, 0);
    if (!keymap) {
        fprintf(stderr, "Couldn't create xkb keymap\n");
        goto out;
    }

    dump = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    if (!dump) {
        fprintf(stderr, "Couldn't get the keymap string\n");
        goto out;
    }

    fputs(dump, stdout);

    ret = EXIT_SUCCESS;
    free(dump);
out:
    if (file)
        fclose(file);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);
    return ret;
}
