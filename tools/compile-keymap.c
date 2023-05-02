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

#define DEFAULT_INCLUDE_PATH_PLACEHOLDER "__defaults__"

static bool verbose = false;
static enum output_format {
    FORMAT_RMLVO,
    FORMAT_KEYMAP,
    FORMAT_KCCGST,
    FORMAT_KEYMAP_FROM_XKB,
} output_format = FORMAT_KEYMAP;
static const char *includes[64];
static size_t num_includes = 0;
typedef struct {
    const char *name;
    enum xkb_warning_flags flag;
} WarningLookupEntry;

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
#if ENABLE_PRIVATE_APIS
           " --kccgst\n"
           "    Print a keymap which only includes the KcCGST component names instead of the full keymap\n"
#endif
           " --rmlvo\n"
           "    Print the full RMLVO with the defaults filled in for missing elements\n"
           " --from-xkb\n"
           "    Load the XKB file from stdin, ignore RMLVO options.\n"
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
           "\n"
           "Warnings options:\n"
           // TODO: the warnings are downgraded to INFO level, not discarded.
           // TODO: show default value for all warnings
           " -W<name>/-Wno-<name>\n"
           "   Activate/deactivate the warning <name>, where <name> is one of the following:\n"
           "   - unrecognized-keysym: report unrecognized keysyms.\n"
           "   - numeric-keysym: report numeric keysyms use, such as: '0x0030'.\n"
           // TODO: description
           "   - conflicting-key-type\n"
           "   - conflicting-key-action\n"
           "   - conflicting-key-keysym\n"
           "   - conflicting-key-symbol\n"
           "   - conflicting-modmap\n"
           "   - group-name-for-non-base-group\n"
           "   - multiple-group-at-once\n"
           "   - cannot-infer-key-type\n"
           "   - undefined-key-type\n"
           "   - unresolved-keymap-symbol\n"
           "   - undefined-keycode\n"
           "   - extra-symbols-ignored\n"
           "   - none: none of the previous warnings.\n"
           "   - all: all of the previous warnings.\n"
           " -E<name>/-Eno-<name>\n"
           "    Turn the corresponding warning (see above) into an error.\n"
           "\n",
           argv[0], DEFAULT_XKB_RULES,
           DEFAULT_XKB_MODEL, DEFAULT_XKB_LAYOUT,
           DEFAULT_XKB_VARIANT ? DEFAULT_XKB_VARIANT : "<none>",
           DEFAULT_XKB_OPTIONS ? DEFAULT_XKB_OPTIONS : "<none>");
}

const WarningLookupEntry warning_names[] = {
    {"unrecognized-keysym",               XKB_WARNING_UNRECOGNIZED_KEYSYM},
    {"numeric-keysym" ,                   XKB_WARNING_NUMERIC_KEYSYM},
    {"conflicting-key-type",              XKB_WARNING_CONFLICTING_KEY_TYPE},
    {"conflicting-key-action",            XKB_WARNING_CONFLICTING_KEY_ACTION},
    {"conflicting-key-keysym",            XKB_WARNING_CONFLICTING_KEY_KEYSYM},
    {"conflicting-key-symbol-map-fields", XKB_WARNING_CONFLICTING_KEY_SYMBOL_MAP_FIELDS},
    {"conflicting-modmap",                XKB_WARNING_CONFLICTING_MODMAP},
    {"group-name-for-non-base-group",     XKB_WARNING_GROUP_NAME_FOR_NON_BASE_GROUP},
    {"multiple-group-at-once",            XKB_WARNING_MULTIPLE_GROUP_AT_ONCE},
    {"cannot-infer-key-type",             XKB_WARNING_CANNOT_INFER_KEY_TYPE},
    {"undefined-key-type",                XKB_WARNING_UNDEFINED_KEY_TYPE},
    {"unresolved-keymap-symbol",          XKB_WARNING_UNRESOLVED_KEYMAP_SYMBOL},
    {"extra-symbols-ignored",             XKB_WARNING_EXTRA_SYMBOLS_IGNORED},
    {"undefined-keycode",                 XKB_WARNING_UNDEFINED_KEYCODE},
    {"all",                               XKB_WARNING_ALL},
    {"none",                              -XKB_WARNING_ALL},
    {NULL, 0}
};


static bool
parse_xkb_warning(const char *name, enum xkb_warning_flags *flags)
{
    bool unset_flag = false;
    enum xkb_warning_flags parsed_flag;
    const char *unprefixed_name = name;
    // Handle “no-” prefix
    if (strncmp(name, "no-", 3) == 0) {
        unprefixed_name += 3;
        unset_flag = true;
    }
    // Lookup warning name and process it
    for (const WarningLookupEntry *entry = warning_names; entry->name; entry++)
        if (strcmp(entry->name, unprefixed_name) == 0) {
            parsed_flag = entry->flag;
            if (parsed_flag < 0) {
                unset_flag = true;
                parsed_flag = -parsed_flag;
            }
            if (unset_flag) {
                *flags &= ~parsed_flag;
            } else {
                *flags |= parsed_flag;
            }
            return true;
        }
    // Not found
    return false;
}

static bool
parse_options(int argc, char **argv, struct xkb_rule_names *names,
              enum xkb_warning_flags *warning_flags,
              enum xkb_warning_flags *error_flags)
{
    enum options {
        OPT_VERBOSE = 1,
        OPT_KCCGST,
        OPT_RMLVO,
        OPT_FROM_XKB,
        OPT_INCLUDE,
        OPT_INCLUDE_DEFAULTS,
        OPT_RULES,
        OPT_MODEL,
        OPT_LAYOUT,
        OPT_VARIANT,
        OPT_OPTION
    };
    const struct option opts[] = {
        {"help",             no_argument,            NULL, 'h'},
        {"verbose",          no_argument,            NULL, OPT_VERBOSE},
#if ENABLE_PRIVATE_APIS
        {"kccgst",           no_argument,            NULL, OPT_KCCGST},
#endif
        {"rmlvo",            no_argument,            NULL, OPT_RMLVO},
        {"from-xkb",         no_argument,            NULL, OPT_FROM_XKB},
        {"include",          required_argument,      NULL, OPT_INCLUDE},
        {"include-defaults", no_argument,            NULL, OPT_INCLUDE_DEFAULTS},
        {"rules",            required_argument,      NULL, OPT_RULES},
        {"model",            required_argument,      NULL, OPT_MODEL},
        {"layout",           required_argument,      NULL, OPT_LAYOUT},
        {"variant",          required_argument,      NULL, OPT_VARIANT},
        {"options",          required_argument,      NULL, OPT_OPTION},
        {0, 0, 0, 0}
    };

    *warning_flags = XKB_WARNING_DEFAULT;
    *error_flags = XKB_ERROR_DEFAULT;

    while (1) {
        int c;
        int option_index = 0;
        c = getopt_long(argc, argv, "E:hW:", opts, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage(argv);
            exit(0);
        case 'E':
            if (parse_xkb_warning(optarg, error_flags)) {
                break;
            } else {
                fprintf(stderr, "error: unsupported warning: \"%s\".\n", optarg);
                usage(argv);
                exit(EXIT_INVALID_USAGE);
            }
        case 'W':
            if (parse_xkb_warning(optarg, warning_flags)) {
                break;
            } else {
                fprintf(stderr, "error: unsupported warning: \"%s\".\n", optarg);
                usage(argv);
                exit(EXIT_INVALID_USAGE);
            }
        case OPT_VERBOSE:
            verbose = true;
            break;
        case OPT_KCCGST:
            output_format = FORMAT_KCCGST;
            break;
        case OPT_RMLVO:
            output_format = FORMAT_RMLVO;
            break;
        case OPT_FROM_XKB:
            output_format = FORMAT_KEYMAP_FROM_XKB;
            break;
        case OPT_INCLUDE:
            if (num_includes >= ARRAY_SIZE(includes)) {
                fprintf(stderr, "error: too many includes\n");
                exit(EXIT_INVALID_USAGE);
            }
            includes[num_includes++] = optarg;
            break;
        case OPT_INCLUDE_DEFAULTS:
            if (num_includes >= ARRAY_SIZE(includes)) {
                fprintf(stderr, "error: too many includes\n");
                exit(EXIT_INVALID_USAGE);
            }
            includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;
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
            exit(EXIT_INVALID_USAGE);
        }

    }

    return true;
}

static bool
print_rmlvo(struct xkb_context *ctx, const struct xkb_rule_names *rmlvo)
{
    printf("rules: \"%s\"\nmodel: \"%s\"\nlayout: \"%s\"\nvariant: \"%s\"\noptions: \"%s\"\n",
           rmlvo->rules, rmlvo->model, rmlvo->layout,
           rmlvo->variant ? rmlvo->variant : "",
           rmlvo->options ? rmlvo->options : "");
    return true;
}

static bool
print_kccgst(struct xkb_context *ctx, const struct xkb_rule_names *rmlvo)
{
#if ENABLE_PRIVATE_APIS
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
#else
        return false;
#endif
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

static bool
print_keymap_from_file(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap = NULL;
    char *keymap_string = NULL;
    FILE *file = NULL;
    bool success = false;

    file = tmpfile();
    if (!file) {
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
            size_t wlen = fwrite(buf, 1, len, file);
            if (wlen != len) {
                fprintf(stderr, "Failed to write to tmpfile\n");
                goto out;
            }
        }
        if (feof(stdin))
            break;
    }
    fseek(file, 0, SEEK_SET);
    keymap = xkb_keymap_new_from_file(ctx, file,
                                      XKB_KEYMAP_FORMAT_TEXT_V1, 0);
    if (!keymap) {
        fprintf(stderr, "Couldn't create xkb keymap\n");
        goto out;
    }

    keymap_string = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    if (!keymap_string) {
        fprintf(stderr, "Couldn't get the keymap string\n");
        goto out;
    }

    fputs(keymap_string, stdout);
    success = true;

out:
    if (file)
        fclose(file);
    xkb_keymap_unref(keymap);
    free(keymap_string);

    return success;
}

int
main(int argc, char **argv)
{
    struct xkb_context *ctx;
    struct xkb_rule_names names = {
        .rules = DEFAULT_XKB_RULES,
        .model = DEFAULT_XKB_MODEL,
        /* layout and variant are tied together, so we either get user-supplied for
         * both or default for both, see below */
        .layout = NULL,
        .variant = NULL,
        .options = DEFAULT_XKB_OPTIONS,
    };
    enum xkb_warning_flags warning_flags;
    enum xkb_warning_flags error_flags;
    int rc = 1;

    if (argc <= 1) {
        usage(argv);
        return EXIT_INVALID_USAGE;
    }

    if (!parse_options(argc, argv, &names, &warning_flags, &error_flags))
        return EXIT_INVALID_USAGE;

    /* Now fill in the layout */
    if (!names.layout || !*names.layout) {
        if (names.variant && *names.variant) {
            fprintf(stderr, "Error: a variant requires a layout\n");
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
    xkb_context_set_warning_flags(ctx, warning_flags);
    xkb_context_set_error_flags(ctx, error_flags);

    if (num_includes == 0)
        includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;

    for (size_t i = 0; i < num_includes; i++) {
        const char *include = includes[i];
        if (strcmp(include, DEFAULT_INCLUDE_PATH_PLACEHOLDER) == 0)
            xkb_context_include_path_append_default(ctx);
        else
            xkb_context_include_path_append(ctx, include);
    }

    if (output_format == FORMAT_RMLVO) {
        rc = print_rmlvo(ctx, &names) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (output_format == FORMAT_KEYMAP) {
        rc = print_keymap(ctx, &names) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (output_format == FORMAT_KCCGST) {
        rc = print_kccgst(ctx, &names) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else if (output_format == FORMAT_KEYMAP_FROM_XKB) {
        rc = print_keymap_from_file(ctx);
    }

    xkb_context_unref(ctx);

    return rc;
}
