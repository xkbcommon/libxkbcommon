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

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "xkbcomp/xkbcomp-priv.h"
#include "xkbcomp/rules.h"
#include "xkbcommon/xkbcommon.h"

static bool verbose = false;
static enum output_format {
    FORMAT_KEYMAP,
    FORMAT_KCCGST,
} output_format = FORMAT_KEYMAP;

static void
usage(char **argv)
{
    printf("Usage: %s [OPTIONS]\n"
           "\n"
           "Compile the given RMLVO to a keymap and print it\n"
           "\n"
           "Options:\n"
           " --verbose\n"
           "    Enable verbose debugging output\n"
           " --kccgst\n"
           "    Print a keymap which only includes the KcCGST component names instead of the full keymap\n"
           "\n"
           "XKB-specific options:\n"
           " --rules <rules>\n"
           "    The XKB ruleset (default: '%s')\n"
           " --model <model>\n"
           "    The XKB model (default: '%s')\n"
           " --layout <layout>\n"
           "    The XKB layout (default: '%s')\n"
           " --variant <variant>\n"
           "    The XKB layout variant (default: '%s')\n"
           " --options <options>\n"
           "    The XKB options (default: '%s')\n"
           "\n",
           argv[0], DEFAULT_XKB_RULES,
           DEFAULT_XKB_MODEL, DEFAULT_XKB_LAYOUT,
           DEFAULT_XKB_VARIANT ? DEFAULT_XKB_VARIANT : "<none>",
           DEFAULT_XKB_OPTIONS ? DEFAULT_XKB_OPTIONS : "<none>");
}

static bool
parse_options(int argc, char **argv, struct xkb_rule_names *names)
{
    enum options {
        OPT_VERBOSE,
        OPT_KCCGST,
        OPT_RULES,
        OPT_MODEL,
        OPT_LAYOUT,
        OPT_VARIANT,
        OPT_OPTION,
    };
    static struct option opts[] = {
        {"help",        no_argument,            0, 'h'},
        {"verbose",     no_argument,            0, OPT_VERBOSE},
        {"kccgst",      no_argument,            0, OPT_KCCGST},
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
        case OPT_VERBOSE:
            verbose = true;
            break;
        case OPT_KCCGST:
            output_format = FORMAT_KCCGST;
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

static bool
print_kccgst(struct xkb_context *ctx, const struct xkb_rule_names *rmlvo)
{
        struct xkb_component_names kccgst;

        if (!xkb_components_from_rules(ctx, rmlvo, &kccgst))
            return false;

        printf("xkb_keymap {\n"
               "  xkb_keycodes { include \"%s\" };\n"
               "  xkb_types { include \"%s\" };\n"
               "  xkb_compat { include \"%s\" };\n"
               "  xkb_symbols { include \"%s\" };\n"
               "};\n",
               kccgst.keycodes, kccgst.types, kccgst.compat, kccgst.symbols);
        free(kccgst.keycodes);
        free(kccgst.types);
        free(kccgst.compat);
        free(kccgst.symbols);

        return true;
}

static bool
print_keymap(struct xkb_context *ctx, const struct xkb_rule_names *rmlvo)
{
    struct xkb_keymap *keymap;

    keymap = xkb_keymap_new_from_names(ctx, rmlvo, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (keymap == NULL)
        return false;

    printf("%s\n", xkb_keymap_get_as_string(keymap,
                                            XKB_KEYMAP_FORMAT_TEXT_V1));
    xkb_keymap_unref(keymap);
    return true;
}

int
main(int argc, char **argv)
{
    struct xkb_context *ctx;
    struct xkb_rule_names names = {
        .rules = NULL,
        .model = NULL,
        .layout = NULL,
        .variant = NULL,
        .options = NULL,
    };
    int rc = 1;

    if (argc <= 1) {
        usage(argv);
        return 1;
    }

    if (!parse_options(argc, argv, &names))
        return 1;

    ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES);
    assert(ctx);

    if (verbose) {
        xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_DEBUG);
        xkb_context_set_log_verbosity(ctx, 10);
    }

    xkb_context_sanitize_rule_names(ctx, &names);
    xkb_context_include_path_append_default(ctx);

    if (output_format == FORMAT_KEYMAP) {
        rc = print_keymap(ctx, &names) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (output_format == FORMAT_KCCGST) {
        rc = print_kccgst(ctx, &names) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    xkb_context_unref(ctx);

    return rc;
}
