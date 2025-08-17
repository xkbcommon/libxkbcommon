/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
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

#include "darray.h"
#include "tools-common.h"
#include "src/utils.h"
#include "src/keymap-formats.h"
#include "src/xkbcomp/file_iterator.h"

/*
 * The output is meant to be valid YAML; however we do not enforce it
 * because we expect the file and section names to be valid text values.
 */

static const unsigned int indent_size = 2;

static bool
print_included_section(struct xkb_context *ctx,
                       enum xkb_keymap_format keymap_input_format,
                       const char *path, const char *map, unsigned int index);

static void
print_flags(unsigned int indent, enum xkb_map_flags flags)
{
    printf("%*s  flags: [", indent, "");
    bool first = true;
    const char *flag_name = NULL;
    unsigned int index = 0;
    while ((flag_name = xkb_map_flags_string_iter(&index, flags))) {
        if (first)
            first = false;
        else
            printf(", ");
        printf("%s", flag_name);
    }
    printf("]\n");
}

static bool
print_included_sections(struct xkb_context *ctx, enum xkb_keymap_format format,
                        const struct xkb_file_section *section,
                        unsigned int include_depth, bool recursive)
{
    const unsigned int indent = (include_depth + 1) * indent_size;

    if (darray_size(section->includes)) {
        bool ok = true;
        printf("%*sincludes:\n", indent, "");
        struct xkb_file_include *inc;
        darray_foreach(inc, section->includes) {
                printf("%*s- merge mode: %s\n", indent, "",
                       xkb_merge_mode_name(inc->merge));
                printf("%*s  file: \"%s\"\n", indent, "",
                       xkb_file_section_get_string(section, inc->file));
                printf("%*s  section: \"%s\"\n", indent, "",
                       xkb_file_section_get_string(section, inc->section));
                printf("%*s  path: \"%s\"\n", indent, "",
                       xkb_file_section_get_string(section, inc->path));
                printf("%*s  modifier: \"%s\"\n", indent, "",
                       xkb_file_section_get_string(section, inc->modifier));
                if (recursive) {
                    ok = print_included_section(
                        ctx, format,
                        xkb_file_section_get_string(section, inc->path),
                        xkb_file_section_get_string(section, inc->section),
                        include_depth + 1
                    );
                    if (!ok)
                        break;
                }
        }
        return ok;
    } else {
        return true;
    }
}

static bool
print_included_section(struct xkb_context *ctx, enum xkb_keymap_format format,
                       const char *path, const char *map,
                       unsigned int include_depth)
{
    struct xkb_file_section section = {0};
    xkb_file_section_init(&section);
    if (isempty(map))
        map = NULL;

    bool ok = xkb_file_section_parse(ctx, format,
                                     XKB_KEYMAP_COMPILE_NO_FLAGS,
                                     path, map, include_depth, &section);

    if (!ok)
        goto out;

    const unsigned int indent = include_depth * indent_size;
    print_flags(indent, section.flags);
    ok = print_included_sections(ctx, format, &section, include_depth, true);

out:
    xkb_file_section_free(&section);
    return ok;
}

enum input_source {
    INPUT_SOURCE_AUTO = 0,
    INPUT_SOURCE_STDIN,
    INPUT_SOURCE_PATH
};

static int
print_sections(struct xkb_context *ctx, enum xkb_keymap_format format,
               enum input_source source, const char *path, const char *map,
               bool recursive)
{
    int ret = EXIT_FAILURE;
    FILE *file = NULL;
    if (source == INPUT_SOURCE_PATH) {
        /* Read from regular file */
        file = fopen(path, "rb");
        if (!file) {
            fprintf(stderr, "ERROR: Failed to open keymap file \"%s\": %s\n",
                    path ? path : "stdin", strerror(errno));
            return ret;
        }
    } else {
        /* Read from stdin */
        file = tools_read_stdin();
    }

    char *string;
    size_t string_len;
    if (!map_file(file, &string, &string_len)) {
        fprintf(stderr, "ERROR: cannot map file");
        goto map_error;
    }

    struct xkb_file_iterator * iter = xkb_file_iterator_new(
        ctx, format, XKB_KEYMAP_COMPILE_NO_FLAGS,
        (path ? path : "(stdin)"), map, string, string_len
    );

    if (!iter) {
        fprintf(stderr, "ERROR: cannot create iterator");
        goto iter_error;
    }

    bool ok = true;
    const struct xkb_file_section *section;
    while ((ok = xkb_file_iterator_next(iter, &section)) && section) {
        printf("- type: %s\n", xkb_file_type_name(section->file_type));
        printf("  section: \"%s\"\n",
               xkb_file_section_get_string(section, section->name));
        print_flags(0, section->flags);
        print_included_sections(ctx, format, section, 0, recursive);
    }

    xkb_file_iterator_free(iter);
    ret = EXIT_SUCCESS;

iter_error:
    unmap_file(string, string_len);

map_error:
    if (source == INPUT_SOURCE_PATH)
        fclose(file);

    return ret;
}

static void
usage(FILE *file, const char *progname)
{
    fprintf(file,
           "Usage: %s [OPTIONS]\n"
           "\n"
           "Introspect a XKB file and generate YAML output\n"
           "\n"
           "General options:\n"
           " --help\n"
           "    Print this help and exit\n"
           " --verbose\n"
           "    Enable verbose debugging output\n"
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
           " --format <format>\n"
           "    The keymap format to use for parsing (default: '%d')\n"
           " --section <name>\n"
           "    The name of a specific section to parse\n"
           " --recursive\n"
           "    Recursive analysis of the included sections\n"
           "\n",
           progname, DEFAULT_INPUT_KEYMAP_FORMAT);
}

#define DEFAULT_INCLUDE_PATH_PLACEHOLDER "__defaults__"
static const char *includes[64] = { 0 };
static size_t num_includes = 0;

static bool
parse_options(int argc, char **argv, bool *verbose,
              enum input_source *input_source,
              enum xkb_keymap_format *keymap_input_format, char **path,
              char **section, bool *recursive)
{
    enum input_source input_format = INPUT_SOURCE_AUTO;
    enum options {
        /* General */
        OPT_VERBOSE,
        /* Input */
        OPT_INCLUDE,
        OPT_INCLUDE_DEFAULTS,
        OPT_KEYMAP_FORMAT,
        OPT_SECTION_NAME,
        OPT_RECURSIVE,
    };
    static struct option opts[] = {
        /*
         * General
         */
        {"help",             no_argument,            0, 'h'},
        {"verbose",          no_argument,            0, OPT_VERBOSE},
        /*
         * Input
         */
        {"include",          required_argument,      0, OPT_INCLUDE},
        {"include-defaults", no_argument,            0, OPT_INCLUDE_DEFAULTS},
        {"format",           required_argument,      0, OPT_KEYMAP_FORMAT},
        {"section",          required_argument,      0, OPT_SECTION_NAME},
        {"recursive",        no_argument,            0, OPT_RECURSIVE},
        {0, 0, 0, 0},
    };

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
            *verbose = true;
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
        case OPT_KEYMAP_FORMAT:
            *keymap_input_format = xkb_keymap_parse_format(optarg);
            if (!(*keymap_input_format)) {
                fprintf(stderr, "ERROR: invalid --format: \"%s\"\n", optarg);
                usage(stderr, argv[0]);
                exit(EXIT_INVALID_USAGE);
            }
            break;
        case OPT_SECTION_NAME:
            *section = optarg;
            break;
        case OPT_RECURSIVE:
            *recursive = true;
            break;
        default:
            goto invalid_usage;
        }
    }

    if (optind < argc && !isempty(argv[optind])) {
        /* Some positional arguments left: use as a file input */
        if (input_format != INPUT_SOURCE_AUTO) {
            goto too_much_arguments;
        }
        input_format = INPUT_SOURCE_PATH;
        *path = argv[optind++];
        if (optind < argc) {
            /* Further positional arguments is an error */
too_much_arguments:
            fprintf(stderr, "ERROR: Too many positional arguments\n");
            goto invalid_usage;
        }
    } else if (is_pipe_or_regular_file(STDIN_FILENO) &&
               input_format == INPUT_SOURCE_AUTO) {
        /* No positional argument: detect piping */
        input_format = INPUT_SOURCE_STDIN;
    }

    if (isempty(*path) || strcmp(*path, "-") == 0)
        *path = NULL;

    *input_source = input_format;
    return true;

too_many_includes:
    fprintf(stderr, "ERROR: too many includes (max: %zu)\n",
            ARRAY_SIZE(includes));

invalid_usage:
    usage(stderr, argv[0]);
    exit(EXIT_INVALID_USAGE);
}

int
main(int argc, char **argv)
{
    struct xkb_context *ctx;
    bool verbose = false;
    char *path = NULL;
    char *map = NULL;
    enum xkb_keymap_format keymap_input_format = DEFAULT_INPUT_KEYMAP_FORMAT;
    bool recursive = false;
    int rc = 1;

    setlocale(LC_ALL, "");

    if (argc < 1) {
        usage(stderr, argv[0]);
        return EXIT_INVALID_USAGE;
    }

    enum input_source input_source = INPUT_SOURCE_AUTO;
    if (!parse_options(argc, argv, &verbose, &input_source,
                       &keymap_input_format, &path, &map, &recursive))
        return EXIT_INVALID_USAGE;

    enum xkb_context_flags ctx_flags = XKB_CONTEXT_NO_DEFAULT_INCLUDES;

    ctx = xkb_context_new(ctx_flags);
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

    rc = print_sections(ctx, keymap_input_format, input_source,
                        path, map, recursive);

    xkb_context_unref(ctx);

    return rc;
}
