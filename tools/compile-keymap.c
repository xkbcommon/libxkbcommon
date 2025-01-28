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

#include "xkbcommon/xkbcommon.h"
#if ENABLE_PRIVATE_APIS
#include "xkbcomp/xkbcomp-priv.h"
#include "xkbcomp/rules.h"
#endif
#include "tools-common.h"
#include "src/utils.h"

#define DEFAULT_INCLUDE_PATH_PLACEHOLDER "__defaults__"

static bool verbose = false;
static enum output_format {
    FORMAT_RMLVO,
    FORMAT_KCCGST,
    FORMAT_KEYMAP_FROM_RMLVO,
    FORMAT_KEYMAP_FROM_XKB,
} output_format = FORMAT_KEYMAP_FROM_RMLVO;
static const char *includes[64];
static size_t num_includes = 0;
static bool test = false;

static void
usage(FILE *file, const char *progname)
{
    fprintf(file,
           "Usage: %s [OPTIONS]\n"
           "\n"
           "Compile the given RMLVO to a keymap and print it\n"
           "\n"
           "Options:\n"
           " --help\n"
           "    Print this help and exit\n"
           " --verbose\n"
           "    Enable verbose debugging output\n"
           " --test\n"
           "    Test compilation but do not print the keymap.\n"
#if ENABLE_PRIVATE_APIS
           " --kccgst\n"
           "    Print a keymap which only includes the KcCGST component names instead of the full keymap\n"
#endif
           " --rmlvo\n"
           "    Print the full RMLVO with the defaults filled in for missing elements\n"
           " --keymap <file>\n"
           " --from-xkb <file>\n"
           "    Load the corresponding XKB file, ignore RMLVO options. If <file>\n"
           "    is \"-\" or missing, then load from stdin."
#if ENABLE_PRIVATE_APIS
           "    This option must not be used with --kccgst.\n"
#endif
           " --include\n"
           "    Add the given path to the include path list. This option is\n"
           "    order-dependent, include paths given first are searched first.\n"
           "    If an include path is given, the default include path list is\n"
           "    not used. Use --include-defaults to add the default include\n"
           "    paths\n"
           " --include-defaults\n"
           "    Add the default set of include directories.\n"
           "    This option is order-dependent, include paths given first\n"
           "    are searched first.\n"
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
           progname, DEFAULT_XKB_RULES,
           DEFAULT_XKB_MODEL, DEFAULT_XKB_LAYOUT,
           DEFAULT_XKB_VARIANT ? DEFAULT_XKB_VARIANT : "<none>",
           DEFAULT_XKB_OPTIONS ? DEFAULT_XKB_OPTIONS : "<none>");
}

static bool
parse_options(int argc, char **argv, char **path, struct xkb_rule_names *names)
{
    enum options {
        OPT_VERBOSE,
        OPT_TEST,
        OPT_KCCGST,
        OPT_RMLVO,
        OPT_FROM_XKB,
        OPT_INCLUDE,
        OPT_INCLUDE_DEFAULTS,
        OPT_RULES,
        OPT_MODEL,
        OPT_LAYOUT,
        OPT_VARIANT,
        OPT_OPTION,
    };
    static struct option opts[] = {
        {"help",             no_argument,            0, 'h'},
        {"verbose",          no_argument,            0, OPT_VERBOSE},
        {"test",             no_argument,            0, OPT_TEST},
#if ENABLE_PRIVATE_APIS
        {"kccgst",           no_argument,            0, OPT_KCCGST},
#endif
        {"rmlvo",            no_argument,            0, OPT_RMLVO},
        {"keymap",           optional_argument,      0, OPT_FROM_XKB},
        /* Alias maintained for backward compatibility */
        {"from-xkb",         optional_argument,      0, OPT_FROM_XKB},
        {"include",          required_argument,      0, OPT_INCLUDE},
        {"include-defaults", no_argument,            0, OPT_INCLUDE_DEFAULTS},
        {"rules",            required_argument,      0, OPT_RULES},
        {"model",            required_argument,      0, OPT_MODEL},
        {"layout",           required_argument,      0, OPT_LAYOUT},
        {"variant",          required_argument,      0, OPT_VARIANT},
        {"options",          required_argument,      0, OPT_OPTION},
        {0, 0, 0, 0},
    };

    bool has_rmlvo_options = false;
    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "h", opts, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage(stdout, argv[0]);
            exit(0);
        case OPT_VERBOSE:
            verbose = true;
            break;
        case OPT_TEST:
            test = true;
            break;
        case OPT_KCCGST:
            if (output_format != FORMAT_KEYMAP_FROM_RMLVO)
                goto output_format_error;
            output_format = FORMAT_KCCGST;
            break;
        case OPT_RMLVO:
            if (output_format != FORMAT_KEYMAP_FROM_RMLVO)
                goto output_format_error;
            output_format = FORMAT_RMLVO;
            break;
        case OPT_FROM_XKB:
            if (output_format != FORMAT_KEYMAP_FROM_RMLVO)
                goto output_format_error;
            if (has_rmlvo_options)
                goto input_format_error;
            output_format = FORMAT_KEYMAP_FROM_XKB;
            /* Optional arguments require `=`, but we want to make this
             * requirement optional too, so that both `--keymap=xxx` and
             * `--keymap xxx` work. */
            if (!optarg && argv[optind] &&
                (argv[optind][0] != '-' || strcmp(argv[optind], "-") == 0 )) {
                *path = argv[optind++];
            } else {
                *path = optarg;
            }
            break;
        case OPT_INCLUDE:
            if (num_includes >= ARRAY_SIZE(includes))
                goto too_many_includes;
            includes[num_includes++] = optarg;
            break;
        case OPT_INCLUDE_DEFAULTS:
            if (num_includes >= ARRAY_SIZE(includes))
                goto too_many_includes;
            includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;
            break;
        case OPT_RULES:
            if (output_format == FORMAT_KEYMAP_FROM_XKB)
                goto input_format_error;
            names->rules = optarg;
            has_rmlvo_options = true;
            break;
        case OPT_MODEL:
            if (output_format == FORMAT_KEYMAP_FROM_XKB)
                goto input_format_error;
            names->model = optarg;
            has_rmlvo_options = true;
            break;
        case OPT_LAYOUT:
            if (output_format == FORMAT_KEYMAP_FROM_XKB)
                goto input_format_error;
            names->layout = optarg;
            has_rmlvo_options = true;
            break;
        case OPT_VARIANT:
            if (output_format == FORMAT_KEYMAP_FROM_XKB)
                goto input_format_error;
            names->variant = optarg;
            has_rmlvo_options = true;
            break;
        case OPT_OPTION:
            if (output_format == FORMAT_KEYMAP_FROM_XKB)
                goto input_format_error;
            names->options = optarg;
            has_rmlvo_options = true;
            break;
        default:
            usage(stderr, argv[0]);
            exit(EXIT_INVALID_USAGE);
        }
    }

    if (optind < argc && !isempty(argv[optind])) {
        /* Some positional arguments left: use as a keymap input */
        if (output_format != FORMAT_KEYMAP_FROM_RMLVO)
            goto output_format_error;
        if (has_rmlvo_options)
            goto too_much_arguments;
        output_format = FORMAT_KEYMAP_FROM_XKB;
        *path = argv[optind++];
        if (optind < argc) {
too_much_arguments:
            fprintf(stderr, "ERROR: Too much positional arguments\n");
            usage(stderr, argv[0]);
            exit(EXIT_INVALID_USAGE);
        }
    } else if (is_pipe_or_regular_file(STDIN_FILENO) && !has_rmlvo_options &&
               output_format != FORMAT_KEYMAP_FROM_XKB) {
        /* No positional argument: detect piping */
        output_format = FORMAT_KEYMAP_FROM_XKB;
    }

    if (isempty(*path) || strcmp(*path, "-") == 0)
        *path = NULL;

    return true;

output_format_error:
    fprintf(stderr, "ERROR: Cannot mix output formats\n");
    usage(stderr, argv[0]);
    exit(EXIT_INVALID_USAGE);

input_format_error:
    fprintf(stderr, "ERROR: Cannot use RMLVO options with keymap input\n");
    usage(stderr, argv[0]);
    exit(EXIT_INVALID_USAGE);

too_many_includes:
    fprintf(stderr, "ERROR: too many includes (max: %zu)\n",
            ARRAY_SIZE(includes));
    exit(EXIT_INVALID_USAGE);
}

static int
print_rmlvo(struct xkb_context *ctx, const struct xkb_rule_names *rmlvo)
{
    printf("rules: \"%s\"\nmodel: \"%s\"\nlayout: \"%s\"\nvariant: \"%s\"\noptions: \"%s\"\n",
           rmlvo->rules, rmlvo->model, rmlvo->layout,
           rmlvo->variant ? rmlvo->variant : "",
           rmlvo->options ? rmlvo->options : "");
    return EXIT_SUCCESS;
}

static int
print_kccgst(struct xkb_context *ctx, const struct xkb_rule_names *rmlvo)
{
#if ENABLE_PRIVATE_APIS
        struct xkb_component_names kccgst;

        if (!xkb_components_from_rules(ctx, rmlvo, &kccgst, NULL))
            return EXIT_FAILURE;
        if (test)
            goto out;

        printf("xkb_keymap {\n"
               "  xkb_keycodes { include \"%s\" };\n"
               "  xkb_types { include \"%s\" };\n"
               "  xkb_compat { include \"%s\" };\n"
               "  xkb_symbols { include \"%s\" };\n"
               "};\n",
               kccgst.keycodes, kccgst.types, kccgst.compat, kccgst.symbols);
out:
        free(kccgst.keycodes);
        free(kccgst.types);
        free(kccgst.compat);
        free(kccgst.symbols);

        return EXIT_SUCCESS;
#else
        return EXIT_FAILURE;
#endif
}

static int
print_keymap_from_names(struct xkb_context *ctx, const struct xkb_rule_names *rmlvo)
{
    struct xkb_keymap *keymap;

    keymap = xkb_keymap_new_from_names(ctx, rmlvo, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (keymap == NULL)
        return EXIT_FAILURE;

    if (test)
        goto out;

    char *buf = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    printf("%s\n", buf);
    free(buf);

out:
    xkb_keymap_unref(keymap);
    return EXIT_SUCCESS;
}

static int
print_keymap_from_file(struct xkb_context *ctx, const char *path)
{
    struct xkb_keymap *keymap = NULL;
    char *keymap_string = NULL;
    FILE *file = NULL;
    int ret = EXIT_FAILURE;

    if (path) {
        /* Read from regular file */
        file = fopen(path, "rb");
    } else {
        /* Read from stdin */
        file = tools_read_stdin();
    }
    if (!file) {
        fprintf(stderr, "ERROR: Failed to open keymap file \"%s\": %s\n",
                path ? path : "stdin", strerror(errno));
        goto out;
    }
    keymap = xkb_keymap_new_from_file(ctx, file,
                                      XKB_KEYMAP_FORMAT_TEXT_V1,
                                      XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        fprintf(stderr, "ERROR: Couldn't create xkb keymap\n");
        goto out;
    } else if (test) {
        ret = EXIT_SUCCESS;
        goto out;
    }

    keymap_string = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    if (!keymap_string) {
        fprintf(stderr, "ERROR: Couldn't get the keymap string\n");
        goto out;
    }

    fputs(keymap_string, stdout);
    ret = EXIT_SUCCESS;

out:
    if (file)
        fclose(file);
    xkb_keymap_unref(keymap);
    free(keymap_string);

    return ret;
}

int
main(int argc, char **argv)
{
    struct xkb_context *ctx;
    char *keymap_path = NULL;
    struct xkb_rule_names names = {
        .rules = DEFAULT_XKB_RULES,
        .model = DEFAULT_XKB_MODEL,
        /* layout and variant are tied together, so we either get user-supplied for
         * both or default for both, see below */
        .layout = NULL,
        .variant = NULL,
        .options = DEFAULT_XKB_OPTIONS,
    };
    int rc = 1;

    if (argc < 1) {
        usage(stderr, argv[0]);
        return EXIT_INVALID_USAGE;
    }

    if (!parse_options(argc, argv, &keymap_path, &names))
        return EXIT_INVALID_USAGE;

    /* Now fill in the layout */
    if (!names.layout || !*names.layout) {
        if (names.variant && *names.variant) {
            fprintf(stderr, "ERROR: a variant requires a layout\n");
            return EXIT_INVALID_USAGE;
        }
        names.layout = DEFAULT_XKB_LAYOUT;
        names.variant = DEFAULT_XKB_VARIANT;
    }

    ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES);
    assert(ctx);

    if (verbose) {
        xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_DEBUG);
        xkb_context_set_log_verbosity(ctx, 10);
    }

    if (num_includes == 0)
        includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;

    for (size_t i = 0; i < num_includes; i++) {
        const char *include = includes[i];
        if (strcmp(include, DEFAULT_INCLUDE_PATH_PLACEHOLDER) == 0)
            xkb_context_include_path_append_default(ctx);
        else
            xkb_context_include_path_append(ctx, include);
    }

    switch (output_format) {
    case FORMAT_RMLVO:
        rc = print_rmlvo(ctx, &names);
        break;
    case FORMAT_KCCGST:
        rc = print_kccgst(ctx, &names);
        break;
    case FORMAT_KEYMAP_FROM_XKB:
        rc = print_keymap_from_file(ctx, keymap_path);
        break;
    default:
        rc = print_keymap_from_names(ctx, &names);
    }

    xkb_context_unref(ctx);

    return rc;
}
