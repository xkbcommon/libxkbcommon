/*
 * Copyright © 2018 Red Hat, Inc.
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


#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "xkbcommon/xkbcommon.h"

static bool print = false;

static void
usage(char **argv)
{
    printf("Usage: %s [--print] [--rules <rules>] [--layout <layout>] [--variant <variant>] [--options <option>]\n",
           argv[0]);
    printf("This tool tests the compilation from RMLVO to a keymap.\n");
    printf("--print  print the resulting keymap\n");
}

static bool
parse_options(int argc, char **argv, struct xkb_rule_names *names)
{
    enum options {
        OPT_PRINT,
        OPT_RULES,
        OPT_MODEL,
        OPT_LAYOUT,
        OPT_VARIANT,
        OPT_OPTION,
    };
    static struct option opts[] = {
        {"help",        no_argument,            0, 'h'},
        {"print",       no_argument,            0, OPT_PRINT},
        {"rules",       required_argument,      0, OPT_RULES},
        {"model",       required_argument,      0, OPT_MODEL},
        {"layout",      required_argument,      0, OPT_LAYOUT},
        {"variant",     required_argument,      0, OPT_VARIANT},
        {"options",     required_argument,      0, OPT_OPTION},
        {0, 0, 0, 0},
    };

    while (1) {
        int c;
        int option_index = 0;
        c = getopt_long(argc, argv, "h", opts, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage(argv);
            exit(0);
        case OPT_PRINT:
            print = true;
            break;
        case OPT_RULES:
            names->rules = optarg;
            break;
        case OPT_MODEL:
            names->model = optarg;
            break;
        case OPT_LAYOUT:
            names->layout = optarg;
            break;
        case OPT_VARIANT:
            names->variant = optarg;
            break;
        case OPT_OPTION:
            names->options = optarg;
            break;
        default:
            usage(argv);
            exit(1);
        }

    }

    return true;
}

int
main(int argc, char **argv)
{
    struct xkb_context *ctx;
    struct xkb_keymap *keymap;
    struct xkb_rule_names names = {
        .rules = NULL,
        .model = NULL,
        .layout = NULL,
        .variant = NULL,
        .options = NULL,
    };
    int rc;

    if (argc <= 1) {
        usage(argv);
        return 1;
    }

    if (!parse_options(argc, argv, &names))
        return 1;

    ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(ctx);

    keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    rc = (keymap == NULL);

    if (rc == 0 && print)
        printf("%s\n", xkb_keymap_get_as_string(keymap,
                                                XKB_KEYMAP_FORMAT_TEXT_V1));

    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    return rc;
}
