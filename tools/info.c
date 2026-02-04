/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <getopt.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon.h"
#include "src/utils.h"
#include "tools-common.h"

static void
usage(FILE *fp, char *progname)
{
    fprintf(fp, "Usage: %s\n", progname);
}

static bool
parse_options(int argc, char **argv)
{
    enum options {
        OPT_HELP = 'h',
    };

    static struct option opts[] = {
        {"help",                 no_argument,            0, OPT_HELP},
        {0, 0, 0, 0},
    };

    while (1) {
        int option_index = 0;
        int opt = getopt_long(argc, argv, "h", opts, &option_index);

        if (opt == -1)
            break;

        switch (opt) {
        case OPT_HELP:
            usage(stdout, argv[0]);
            exit(EXIT_SUCCESS);
        default:
// invalid_usage:
            usage(stderr, argv[0]);
            return false;
        }
    }

    return true;
}

int
main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    if (!parse_options(argc, argv)) {
        return EXIT_INVALID_USAGE;
    }

    printf("Version: \"%s\"\n", LIBXKBCOMMON_VERSION);
    printf("Website: https://xkbcommon.org\n");

    printf("Tools path: \"%s\"\n", LIBXKBCOMMON_TOOL_PATH);

    /* Features */
    printf("Features:\n");
    printf("  Extensions directories: "
#if HAVE_XKB_EXTENSIONS_DIRECTORIES
    "true"
#else
    "false"
#endif
    "\n");

    /* Built-in settings */
    printf("Built-in values:\n");
    static const struct {
        const char * var;
        const char * value;
    } builtins[] = {
        { "XKB_CONFIG_ROOT", DFLT_XKB_CONFIG_ROOT },
        { "XKB_CONFIG_LEGACY_ROOT", DFLT_XKB_LEGACY_ROOT },
        { "XKB_CONFIG_EXTRA_PATH", DFLT_XKB_CONFIG_EXTRA_PATH },
        { "XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH", DFLT_XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH },
        { "XKB_CONFIG_VERSIONED_EXTENSIONS_PATH", DFLT_XKB_CONFIG_VERSIONED_EXTENSIONS_PATH },
        { "XKB_DEFAULT_RULES", DEFAULT_XKB_RULES },
        { "XKB_DEFAULT_MODEL", DEFAULT_XKB_MODEL },
        { "XKB_DEFAULT_LAYOUT", DEFAULT_XKB_LAYOUT },
        { "XKB_DEFAULT_VARIANT", DEFAULT_XKB_VARIANT },
        { "XKB_DEFAULT_OPTIONS", DEFAULT_XKB_OPTIONS },
        { "XLOCALEDIR", XLOCALEDIR },
    };
    for (size_t v = 0; v < ARRAY_SIZE(builtins); v++) {
        printf("  %s: ", builtins[v].var);
        const char * const value = builtins[v].value;
        if (value)
            printf("\"%s\"\n", value);
        else
            printf("null\n");
    }

    struct xkb_context * const ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    /* Environment variable */
    printf("Environment variables:\n");
    static const char * vars[] = {
        "XKB_CONFIG_ROOT",
        "XKB_CONFIG_EXTRA_PATH",
        "XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH",
        "XKB_CONFIG_VERSIONED_EXTENSIONS_PATH",
        "XKB_DEFAULT_RULES",
        "XKB_DEFAULT_MODEL",
        "XKB_DEFAULT_LAYOUT",
        "XKB_DEFAULT_VARIANT",
        "XKB_DEFAULT_OPTIONS",
        "HOME",
        "XDG_CONFIG_HOME",
        "XLOCALEDIR",
        "XCOMPOSEFILE",
    };
    for (size_t v = 0; v < ARRAY_SIZE(vars); v++) {
        printf("  %s: ", vars[v]);
        const char * const value = secure_getenv(vars[v]);
        if (value)
            printf("\"%s\"\n", value);
        else
            printf("null\n");
    }

    /* Include paths */
    const unsigned int num_paths = xkb_context_num_include_paths(ctx);
    printf("XKB include paths:\n");
    for (unsigned int p = 0; p < num_paths; p++) {
        printf("- \"%s\"\n", xkb_context_include_path_get(ctx, p));
    }

    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
