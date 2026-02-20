/*
 * Copyright © 2018 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <locale.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "xkbcommon/xkbcommon.h"
#include "tools-common.h"
#include "src/utils.h"
#include "src/keymap-formats.h"

#define DEFAULT_INCLUDE_PATH_PLACEHOLDER "__defaults__"

enum input_format {
    INPUT_FORMAT_AUTO = 0,
    INPUT_FORMAT_RMLVO,
    INPUT_FORMAT_KEYMAP
};
enum output_format {
    OUTPUT_FORMAT_KEYMAP = 0,
    OUTPUT_FORMAT_RMLVO,
    OUTPUT_FORMAT_KCCGST,
    OUTPUT_FORMAT_KCCGST_YAML,
    OUTPUT_FORMAT_MODMAPS,
};
static bool verbose = false;
static const char *includes[64] = { 0 };
static size_t num_includes = 0;
static bool test = false;

static void
usage(FILE *file, const char *progname)
{
    fprintf(file,
           "Usage: %s [OPTIONS]\n"
           "\n"
           "Compile the given input to a keymap and print it\n"
           "\n"
           "General options:\n"
           " --help\n"
           "    Print this help and exit\n"
           " --verbose\n"
           "    Enable verbose debugging output\n"
           " --test\n"
           "    Test compilation but do not print the keymap.\n"
           "\n"
           "Input options:\n"
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
           " --input-format <format>\n"
           "    The keymap format to use for parsing (default: '%s')\n"
           " --output-format <format>\n"
           "    The keymap format to use for serializing (default: same as input)\n"
           " --format <format>\n"
           "    The keymap format to use for both parsing and serializing\n"
           " --no-pretty\n"
           "    Do not pretty-print when serializing a keymap\n"
           " --drop-unused\n"
           "    Disable unused bits serialization\n"
           " --keymap <file>\n"
           " --from-xkb <file>\n"
           "    Load the corresponding XKB file, ignore RMLVO options. If <file>\n"
           "    is \"-\" or missing, then load from stdin."
           "    This option must not be used with --kccgst.\n"
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
           " --enable-environment-names\n"
           "    Allow to set the default RMLVO values via the following environment variables:\n"
           "    - XKB_DEFAULT_RULES\n"
           "    - XKB_DEFAULT_MODEL\n"
           "    - XKB_DEFAULT_LAYOUT\n"
           "    - XKB_DEFAULT_VARIANT\n"
           "    - XKB_DEFAULT_OPTIONS\n"
           "    Note that this option may affect the default values of the previous options.\n"
           "    This option must not be used with --keymap.\n"
           "\n"
           "Output options:\n"
           " --output-format <format>\n"
           "    The keymap format to use for serializing (default: same as input)\n"
           " --no-pretty\n"
           "    Do not pretty-print when serializing a keymap\n"
           " --drop-unused\n"
           "    Disable unused bits serialization\n"
           " --explicit-values\n"
           "    Force serializing all values\n"
           " --kccgst\n"
           "    Print a keymap which only includes the KcCGST component names instead of the full keymap\n"
           " --kccgst-yaml\n"
           "    Print the KcCGST component names in YAML format\n"
           " --rmlvo\n"
           "    Print the full RMLVO with the defaults filled in for missing elements, in YAML format\n"
           " --modmaps\n"
           "    Print real and virtual key modmaps and modifiers encodings in YAML format\n"
           "\n",
           progname,
           xkb_keymap_get_format_label(DEFAULT_INPUT_KEYMAP_FORMAT),
           DEFAULT_XKB_RULES, DEFAULT_XKB_MODEL, DEFAULT_XKB_LAYOUT,
           DEFAULT_XKB_VARIANT ? DEFAULT_XKB_VARIANT : "<none>",
           DEFAULT_XKB_OPTIONS ? DEFAULT_XKB_OPTIONS : "<none>");
}

static inline bool
is_incompatible_with_keymap_input(enum output_format format)
{
    switch (format) {
        case OUTPUT_FORMAT_KCCGST:
        case OUTPUT_FORMAT_KCCGST_YAML:
        case OUTPUT_FORMAT_RMLVO:
            return true;
        default:
            return false;
    }
}

static bool
parse_options(int argc, char **argv,
              enum input_format *input_format_out,
              enum output_format *output_format_out,
              enum xkb_keymap_format *keymap_input_format,
              enum xkb_keymap_format *keymap_output_format,
              enum xkb_keymap_serialize_flags *serialize_flags,
              bool *use_env_names,
              char **path, struct xkb_rule_names *names)
{
    enum input_format input_format = INPUT_FORMAT_AUTO;
    enum output_format output_format = OUTPUT_FORMAT_KEYMAP;
    enum options {
        /* General */
        OPT_VERBOSE,
        OPT_TEST,
        /* Input */
        OPT_INCLUDE,
        OPT_INCLUDE_DEFAULTS,
        OPT_KEYMAP,
        OPT_ENABLE_ENV_NAMES,
        OPT_KEYMAP_INPUT_FORMAT,
        OPT_KEYMAP_OUTPUT_FORMAT,
        OPT_KEYMAP_FORMAT,
        OPT_KEYMAP_NO_PRETTY,
        OPT_KEYMAP_DROP_UNUSED,
        OPT_KEYMAP_EXPLICIT,
        OPT_RULES,
        OPT_MODEL,
        OPT_LAYOUT,
        OPT_VARIANT,
        OPT_OPTION,
        /* Output */
        OPT_KCCGST,
        OPT_KCCGST_YAML,
        OPT_RMLVO,
        OPT_MODMAPS,
    };
    static struct option opts[] = {
        /*
         * General
         */
        {"help",             no_argument,            0, 'h'},
        {"verbose",          no_argument,            0, OPT_VERBOSE},
        {"test",             no_argument,            0, OPT_TEST},
        /*
         * Input
         */
        {"include",          required_argument,      0, OPT_INCLUDE},
        {"include-defaults", no_argument,            0, OPT_INCLUDE_DEFAULTS},
        {"keymap",           optional_argument,      0, OPT_KEYMAP},
        /* Alias maintained for backward compatibility */
        {"from-xkb",         optional_argument,      0, OPT_KEYMAP},
        {"enable-environment-names", no_argument,    0, OPT_ENABLE_ENV_NAMES},
        {"input-format",     required_argument,      0, OPT_KEYMAP_INPUT_FORMAT},
        {"format",           required_argument,      0, OPT_KEYMAP_FORMAT},
        {"rules",            required_argument,      0, OPT_RULES},
        {"model",            required_argument,      0, OPT_MODEL},
        {"layout",           required_argument,      0, OPT_LAYOUT},
        {"variant",          required_argument,      0, OPT_VARIANT},
        {"options",          required_argument,      0, OPT_OPTION},
        /*
         * Output
         */
        {"output-format",    required_argument,      0, OPT_KEYMAP_OUTPUT_FORMAT},
        {"no-pretty",        no_argument,            0, OPT_KEYMAP_NO_PRETTY},
        {"drop-unused",      no_argument,            0, OPT_KEYMAP_DROP_UNUSED},
        {"explicit-values",  no_argument,            0, OPT_KEYMAP_EXPLICIT},
        {"kccgst",           no_argument,            0, OPT_KCCGST},
        {"kccgst-yaml",      no_argument,            0, OPT_KCCGST_YAML},
        {"rmlvo",            no_argument,            0, OPT_RMLVO},
        {"modmaps",          no_argument,            0, OPT_MODMAPS},
        {0, 0, 0, 0},
    };

    *use_env_names = false;
    int option_index = 0;
    while (1) {
        option_index = 0;
        int c = getopt_long(argc, argv, "h", opts, &option_index);
        if (c == -1)
            break;

        switch (c) {
        /* General */
        case 'h':
            usage(stdout, argv[0]);
            exit(0);
        case OPT_VERBOSE:
            verbose = true;
            break;
        case OPT_TEST:
            test = true;
            break;
        /* Input */
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
        case OPT_KEYMAP:
            if (*use_env_names)
                goto keymap_env_error;
            if (input_format == INPUT_FORMAT_RMLVO)
                goto input_format_error;
            if (is_incompatible_with_keymap_input(output_format))
                goto output_incompatible_with_keymap_input_error;
            input_format = INPUT_FORMAT_KEYMAP;
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
        case OPT_ENABLE_ENV_NAMES:
            if (input_format == INPUT_FORMAT_KEYMAP)
                goto keymap_env_error;
            *use_env_names = true;
            input_format = INPUT_FORMAT_RMLVO;
            break;
        case OPT_KEYMAP_INPUT_FORMAT:
            *keymap_input_format = xkb_keymap_parse_format(optarg);
            if (!(*keymap_input_format)) {
                fprintf(stderr, "ERROR: invalid --input-format: \"%s\"\n", optarg);
                usage(stderr, argv[0]);
                exit(EXIT_INVALID_USAGE);
            }
            break;
        case OPT_KEYMAP_OUTPUT_FORMAT:
            *keymap_output_format = xkb_keymap_parse_format(optarg);
            if (!(*keymap_output_format)) {
                fprintf(stderr, "ERROR: invalid --output-format: \"%s\"\n", optarg);
                usage(stderr, argv[0]);
                exit(EXIT_INVALID_USAGE);
            }
            break;
        case OPT_KEYMAP_FORMAT:
            *keymap_input_format = xkb_keymap_parse_format(optarg);
            if (!(*keymap_input_format)) {
                fprintf(stderr, "ERROR: invalid --format: \"%s\"\n", optarg);
                usage(stderr, argv[0]);
                exit(EXIT_INVALID_USAGE);
            }
            *keymap_output_format = *keymap_input_format;
            break;
        case OPT_KEYMAP_NO_PRETTY:
            *serialize_flags &= ~XKB_KEYMAP_SERIALIZE_PRETTY;
            break;
        case OPT_KEYMAP_DROP_UNUSED:
            *serialize_flags &= ~XKB_KEYMAP_SERIALIZE_KEEP_UNUSED;
            break;
        case OPT_KEYMAP_EXPLICIT:
            *serialize_flags |= XKB_KEYMAP_SERIALIZE_EXPLICIT;
            break;
        case OPT_RULES:
            if (input_format == INPUT_FORMAT_KEYMAP)
                goto input_format_error;
            names->rules = optarg;
            input_format = INPUT_FORMAT_RMLVO;
            break;
        case OPT_MODEL:
            if (input_format == INPUT_FORMAT_KEYMAP)
                goto input_format_error;
            names->model = optarg;
            input_format = INPUT_FORMAT_RMLVO;
            break;
        case OPT_LAYOUT:
            if (input_format == INPUT_FORMAT_KEYMAP)
                goto input_format_error;
            names->layout = optarg;
            input_format = INPUT_FORMAT_RMLVO;
            break;
        case OPT_VARIANT:
            if (input_format == INPUT_FORMAT_KEYMAP)
                goto input_format_error;
            names->variant = optarg;
            input_format = INPUT_FORMAT_RMLVO;
            break;
        case OPT_OPTION:
            if (input_format == INPUT_FORMAT_KEYMAP)
                goto input_format_error;
            names->options = optarg;
            input_format = INPUT_FORMAT_RMLVO;
            break;
        /* Output */
        case OPT_KCCGST:
            assert(is_incompatible_with_keymap_input(OUTPUT_FORMAT_KCCGST));
            if (input_format == INPUT_FORMAT_KEYMAP)
                goto output_incompatible_with_keymap_input_error;
            if (output_format != OUTPUT_FORMAT_KEYMAP &&
                output_format != OUTPUT_FORMAT_KCCGST)
                goto output_format_error;
            output_format = OUTPUT_FORMAT_KCCGST;
            break;
        case OPT_KCCGST_YAML:
            assert(is_incompatible_with_keymap_input(OUTPUT_FORMAT_KCCGST_YAML));
            if (input_format == INPUT_FORMAT_KEYMAP)
                goto output_incompatible_with_keymap_input_error;
            if (output_format != OUTPUT_FORMAT_KEYMAP &&
                output_format != OUTPUT_FORMAT_KCCGST_YAML)
                goto output_format_error;
            output_format = OUTPUT_FORMAT_KCCGST_YAML;
            break;
        case OPT_RMLVO:
            assert(is_incompatible_with_keymap_input(OUTPUT_FORMAT_RMLVO));
            if (input_format == INPUT_FORMAT_KEYMAP)
                goto output_incompatible_with_keymap_input_error;
            if (output_format != OUTPUT_FORMAT_KEYMAP &&
                output_format != OUTPUT_FORMAT_RMLVO)
                goto output_format_error;
            output_format = OUTPUT_FORMAT_RMLVO;
            break;
        case OPT_MODMAPS:
            assert(!is_incompatible_with_keymap_input(OUTPUT_FORMAT_MODMAPS));
            if (output_format != OUTPUT_FORMAT_KEYMAP &&
                output_format != OUTPUT_FORMAT_MODMAPS)
                goto output_format_error;
            output_format = OUTPUT_FORMAT_MODMAPS;
            break;
        default:
            goto invalid_usage;
        }
    }

    if (optind < argc && !isempty(argv[optind])) {
        /* Some positional arguments left: use as a keymap input */
        if (input_format != INPUT_FORMAT_AUTO ||
            is_incompatible_with_keymap_input(output_format)) {
            goto too_much_arguments;
        }
        input_format = INPUT_FORMAT_KEYMAP;
        *path = argv[optind++];
        if (optind < argc) {
            /* Further positional arguments is an error */
too_much_arguments:
            fprintf(stderr, "ERROR: Too many positional arguments\n");
            goto invalid_usage;
        }
    } else if (is_pipe_or_regular_file(STDIN_FILENO) &&
               input_format != INPUT_FORMAT_RMLVO &&
               !is_incompatible_with_keymap_input(output_format)) {
        /* No positional argument: detect piping */
        input_format = INPUT_FORMAT_KEYMAP;
    }

    if (isempty(*path) || strcmp(*path, "-") == 0)
        *path = NULL;

    *input_format_out = input_format;
    *output_format_out = output_format;
    return true;

input_format_error:
    fprintf(stderr, "ERROR: Cannot use RMLVO options with keymap input\n");
    goto invalid_usage;

keymap_env_error:
    fprintf(stderr, "ERROR: --keymap is not compatible with "
                    "--enable-environment-names\n");
    goto invalid_usage;

output_format_error:
    fprintf(stderr, "ERROR: Cannot mix output formats\n");
    goto invalid_usage;

output_incompatible_with_keymap_input_error:
    fprintf(stderr, "ERROR: Output format incompatible with keymap input\n");
    goto invalid_usage;

too_many_includes:
    fprintf(stderr, "ERROR: too many includes (max: %zu)\n",
            ARRAY_SIZE(includes));

invalid_usage:
    usage(stderr, argv[0]);
    exit(EXIT_INVALID_USAGE);
}

static int
print_rmlvo(struct xkb_context *ctx, struct xkb_rule_names *rmlvo)
{
    /* Resolve default RMLVO values */
    struct xkb_rule_names resolved = { NULL };
    xkb_components_names_from_rules(ctx, rmlvo, &resolved, NULL);

    if (test)
        return EXIT_SUCCESS;

    printf("rules: \"%s\"\nmodel: \"%s\"\nlayout: \"%s\"\nvariant: \"%s\"\n"
           "options: \"%s\"\n",
           resolved.rules, resolved.model, resolved.layout,
           resolved.variant ? resolved.variant : "",
           resolved.options ? resolved.options : "");
    return EXIT_SUCCESS;
}

static int
print_kccgst(struct xkb_context *ctx, struct xkb_rule_names *rmlvo, bool yaml)
{
        struct xkb_component_names kccgst = { 0 };

        /* Resolve missing RMLVO values, then resolve the RMLVO names to
         * KcCGST components */
        if (!xkb_components_names_from_rules(ctx, rmlvo, NULL, &kccgst))
            return EXIT_FAILURE;
        if (test)
            goto out;

        if (yaml) {
            printf("keycodes: \"%s\"\n"
                   "types: \"%s\"\n"
                   "compat: \"%s\"\n"
                   "symbols: \"%s\"\n",
                   kccgst.keycodes, kccgst.types, kccgst.compatibility,
                   kccgst.symbols);
            /* Contrary to the previous components, geometry can be empty */
            if (!isempty(kccgst.geometry)) {
                printf("geometry: \"%s\"\n", kccgst.geometry);
            }
        } else {
            printf("xkb_keymap {\n"
                   "  xkb_keycodes { include \"%s\" };\n"
                   "  xkb_types { include \"%s\" };\n"
                   "  xkb_compat { include \"%s\" };\n"
                   "  xkb_symbols { include \"%s\" };\n",
                   kccgst.keycodes, kccgst.types, kccgst.compatibility,
                   kccgst.symbols);
            /* Contrary to the previous components, geometry can be empty */
            if (!isempty(kccgst.geometry)) {
                printf("  xkb_geometry { include \"%s\" };\n", kccgst.geometry);
            }
            printf("};\n");
        }
out:
        free(kccgst.keycodes);
        free(kccgst.types);
        free(kccgst.compatibility);
        free(kccgst.symbols);
        free(kccgst.geometry);

        return EXIT_SUCCESS;
}

static struct xkb_keymap*
load_keymap(struct xkb_context *ctx, enum xkb_keymap_format keymap_input_format,
            enum input_format format, const struct xkb_rule_names *rmlvo,
            const char *path)
{
    if (format == INPUT_FORMAT_KEYMAP) {
        FILE *file = NULL;
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
            return NULL;
        }
        return xkb_keymap_new_from_file(ctx, file,
                                        keymap_input_format,
                                        XKB_KEYMAP_COMPILE_NO_FLAGS);
    } else {
        return xkb_keymap_new_from_names2(ctx, rmlvo, keymap_input_format,
                                          XKB_KEYMAP_COMPILE_NO_FLAGS);
    }
}

static int
print_keymap(struct xkb_context *ctx,
             enum xkb_keymap_format keymap_input_format,
             enum xkb_keymap_format keymap_output_format,
             enum input_format format, enum xkb_keymap_serialize_flags flags,
             const struct xkb_rule_names *rmlvo,
             const char *path)
{
    int ret = EXIT_SUCCESS;
    struct xkb_keymap *keymap = load_keymap(ctx, keymap_input_format,
                                            format, rmlvo, path);

    if (!keymap) {
        fprintf(stderr, "ERROR: Couldn't create xkb keymap\n");
        ret = EXIT_FAILURE;
    } else if (!test) {
        char* keymap_string =
            xkb_keymap_get_as_string2(keymap, keymap_output_format, flags);
        if (!keymap_string) {
            fprintf(stderr, "ERROR: Couldn't get the keymap string\n");
        } else {
            fputs(keymap_string, stdout);
            free(keymap_string);
        }
    }
    xkb_keymap_unref(keymap);
    return ret;
}

static int
print_modmaps(struct xkb_context *ctx,
              enum xkb_keymap_format keymap_input_format,
              enum input_format format, const struct xkb_rule_names *rmlvo,
              const char *path)
{
    int ret = EXIT_SUCCESS;
    struct xkb_keymap *keymap = load_keymap(ctx, keymap_input_format,
                                            format, rmlvo, path);
    if (!keymap) {
        fprintf(stderr, "ERROR: Couldn't create xkb keymap\n");
        ret = EXIT_FAILURE;
    } else if (!test) {
        print_modifiers_encodings(keymap);
        printf("\n");
        print_keys_modmaps(keymap);
    }
    xkb_keymap_unref(keymap);
    return ret;
}

int
main(int argc, char **argv)
{
    struct xkb_context *ctx;
    char *keymap_path = NULL;
    struct xkb_rule_names names = { 0 };
    bool use_env_names = false;
    enum xkb_keymap_format keymap_input_format = DEFAULT_INPUT_KEYMAP_FORMAT;
    static_assert(DEFAULT_OUTPUT_KEYMAP_FORMAT == XKB_KEYMAP_USE_ORIGINAL_FORMAT,
                  "Out of sync usage()");
    enum xkb_keymap_format keymap_output_format = DEFAULT_OUTPUT_KEYMAP_FORMAT;
    enum xkb_keymap_serialize_flags serialize_flags =
        (enum xkb_keymap_serialize_flags) DEFAULT_KEYMAP_SERIALIZE_FLAGS;
    int rc = 1;

    setlocale(LC_ALL, "");

    if (argc < 1) {
        usage(stderr, argv[0]);
        return EXIT_INVALID_USAGE;
    }

    enum input_format input_format = INPUT_FORMAT_AUTO;
    enum output_format output_format = OUTPUT_FORMAT_KEYMAP;
    if (!parse_options(argc, argv, &input_format, &output_format,
                       &keymap_input_format, &keymap_output_format,
                       &serialize_flags, &use_env_names, &keymap_path, &names))
        return EXIT_INVALID_USAGE;

    enum xkb_context_flags ctx_flags = XKB_CONTEXT_NO_DEFAULT_INCLUDES;
    if (!use_env_names)
        ctx_flags |= XKB_CONTEXT_NO_ENVIRONMENT_NAMES;

    ctx = xkb_context_new(ctx_flags);
    assert(ctx);

    if (verbose)
        tools_enable_verbose_logging(ctx);

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
    case OUTPUT_FORMAT_RMLVO:
        assert(input_format != INPUT_FORMAT_KEYMAP);
        rc = print_rmlvo(ctx, &names);
        break;
    case OUTPUT_FORMAT_KCCGST:
        assert(input_format != INPUT_FORMAT_KEYMAP);
        rc = print_kccgst(ctx, &names, false);
        break;
    case OUTPUT_FORMAT_KCCGST_YAML:
        assert(input_format != INPUT_FORMAT_KEYMAP);
        rc = print_kccgst(ctx, &names, true);
        break;
    case OUTPUT_FORMAT_MODMAPS:
        rc = print_modmaps(ctx, keymap_input_format,
                           input_format, &names, keymap_path);
        break;
    default:
        rc = print_keymap(ctx, keymap_input_format, keymap_output_format,
                          input_format, serialize_flags, &names, keymap_path);
    }

    xkb_context_unref(ctx);

    return rc;
}
