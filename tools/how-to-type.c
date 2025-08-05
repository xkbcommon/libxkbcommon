/*
 * Copyright © 2020 Ran Benita <ran@unusedvar.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <getopt.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-keysyms.h"
#include "src/utils.h"
#include "src/keysym.h"
#include "src/utf8-decoding.h"
#include "src/keymap-formats.h"
#include "tools-common.h"

#define ARRAY_SIZE(arr) ((sizeof(arr) / sizeof(*(arr))))

static uint32_t
parse_char_or_codepoint(const char *raw) {
    size_t raw_length = strlen_safe(raw);
    size_t length = 0;

    if (!raw_length)
        return INVALID_UTF8_CODE_POINT;

    /* Try to parse the parameter as a UTF-8 encoded single character */
    uint32_t codepoint = utf8_next_code_point(raw, raw_length, &length);

    /* If parsing failed or did not consume all the string, then try other formats */
    if (codepoint == INVALID_UTF8_CODE_POINT ||
        length == 0 || length != raw_length) {
        char *endp;
        long val;
        int base = 10;
        /* Detect U+NNNN format standard Unicode code point format */
        if (raw_length >= 2 && raw[0] == 'U' && raw[1] == '+') {
            base = 16;
            raw += 2;
        }
        /* Use strtol with explicit bases instead of `0` in order to avoid
         * unexpected parsing as octal. */
        for (; base <= 16; base += 6) {
            errno = 0;
            val = strtol(raw, &endp, base);
            if (errno != 0 || !isempty(endp) || val < 0 || val > 0x10FFFF) {
                val = -1;
            } else {
                break;
            }
        }
        if (val < 0) {
            fprintf(stderr, "ERROR: Failed to convert argument to Unicode code point\n");
            return INVALID_UTF8_CODE_POINT;
        }
        codepoint = (uint32_t) val;
    }
    return codepoint;
}

static void
usage(FILE *fp, const char *argv0)
{
    fprintf(fp, "Usage: %s [--help] [--verbose] [--keysym] [--rules <rules>] "
                "[--model <model>] [--layout <layout>] [--variant <variant>] "
                "[--options <options>] [--enable-environment-names] "
                "<character/codepoint/keysym>\n", argv0);
    fprintf(
        fp,
        "\n"
        "Prints the key combinations (keycode + modifiers) in the keymap's layouts which\n"
        "would produce the given Unicode code point or keysym.\n"
        "\n"
        "<character/codepoint/keysym> is either:\n"
        "- a single character (requires a terminal which uses UTF-8 character encoding);\n"
        "- a Unicode code point, interpreted as hexadecimal if prefixed with '0x' or 'U+'\n"
        "  else as decimal;\n"
        "- a keysym if either the previous interpretations failed or if --keysym is used. \n"
        "  The parameter is then either a keysym name or a numeric value (hexadecimal \n"
        "  if prefixed with '0x' else decimal). Note that values '0' .. '9' are special: \n"
        "  they are both names and numeric values. The default interpretation is names; \n"
        "  use the hexadecimal form '0x0' .. '0x9' in order to interpret as numeric values.\n"
        "\n"
        "Options:\n"
        " --help\n"
        "    Print this help and exit\n"
        " --verbose\n"
        "    Enable verbose debugging output\n"
        " --keysym\n"
        "    Treat the argument only as a keysym\n"
        "\n"
        "XKB-specific options:\n"
        " --format <format>\n"
        "    The keymap format to use (default: %d)\n"
        " --keymap=<file>\n"
        "    Load the corresponding XKB file, ignore RMLVO options. If <file>\n"
        "    is \"-\" or missing, then load from stdin.\n"
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
        "\n",
        DEFAULT_INPUT_KEYMAP_FORMAT,
        DEFAULT_XKB_RULES, DEFAULT_XKB_MODEL, DEFAULT_XKB_LAYOUT,
        DEFAULT_XKB_VARIANT ? DEFAULT_XKB_VARIANT : "<none>",
        DEFAULT_XKB_OPTIONS ? DEFAULT_XKB_OPTIONS : "<none>");
}

enum input_keymap_source {
    KEYMAP_SOURCE_AUTO,
    KEYMAP_SOURCE_RMLVO,
    KEYMAP_SOURCE_FILE
};

static int
parse_options(int argc, char **argv, bool *verbose,
              bool *keysym_mode, xkb_keysym_t *keysym,
              enum input_keymap_source *keymap_source,
              enum xkb_keymap_format *keymap_input_format,
              const char **keymap_path,
              bool *use_env_names, struct xkb_rule_names *names)
{
    enum options {
        OPT_VERBOSE,
        OPT_KEYSYM,
        OPT_ENABLE_ENV_NAMES,
        OPT_KEYMAP_FORMAT,
        OPT_KEYMAP,
        OPT_RULES,
        OPT_MODEL,
        OPT_LAYOUT,
        OPT_VARIANT,
        OPT_OPTIONS,
    };
    static struct option opts[] = {
        {"help",                 no_argument,            0, 'h'},
        {"verbose",              no_argument,            0, OPT_VERBOSE},
        {"keysym",               no_argument,            0, OPT_KEYSYM},
        {"enable-environment-names", no_argument,        0, OPT_ENABLE_ENV_NAMES},
        {"format",               required_argument,      0, OPT_KEYMAP_FORMAT},
        {"keymap",               optional_argument,      0, OPT_KEYMAP},
        {"rules",                required_argument,      0, OPT_RULES},
        {"model",                required_argument,      0, OPT_MODEL},
        {"layout",               required_argument,      0, OPT_LAYOUT},
        {"variant",              required_argument,      0, OPT_VARIANT},
        {"options",              required_argument,      0, OPT_OPTIONS},
        {0, 0, 0, 0},
    };

    while (1) {
        int opt;
        int option_index = 0;

        opt = getopt_long(argc, argv, "h", opts, &option_index);
        if (opt == -1)
            break;

        switch (opt) {
        case OPT_VERBOSE:
            *verbose = true;
            break;
        case OPT_KEYSYM:
            *keysym_mode = true;
            break;
        case OPT_ENABLE_ENV_NAMES:
            if (*keymap_source == KEYMAP_SOURCE_FILE)
                goto keymap_env_error;
            *use_env_names = true;
            *keymap_source = KEYMAP_SOURCE_RMLVO;
            break;
        case OPT_KEYMAP_FORMAT:
            *keymap_input_format = xkb_keymap_parse_format(optarg);
            if (!*keymap_input_format) {
                fprintf(stderr, "ERROR: invalid --format \"%s\"\n", optarg);
                goto invalid_usage;
            }
            break;
        case OPT_KEYMAP:
            if (*keymap_source == KEYMAP_SOURCE_RMLVO)
                goto keymap_source_error;
            *keymap_source = KEYMAP_SOURCE_FILE;
            *keymap_path = optarg;
            break;
        case OPT_RULES:
            if (*keymap_source == KEYMAP_SOURCE_FILE)
                goto keymap_source_error;
            names->rules = optarg;
            *keymap_source = KEYMAP_SOURCE_RMLVO;
            break;
        case OPT_MODEL:
            if (*keymap_source == KEYMAP_SOURCE_FILE)
                goto keymap_source_error;
            names->model = optarg;
            *keymap_source = KEYMAP_SOURCE_RMLVO;
            break;
        case OPT_LAYOUT:
            if (*keymap_source == KEYMAP_SOURCE_FILE)
                goto keymap_source_error;
            names->layout = optarg;
            *keymap_source = KEYMAP_SOURCE_RMLVO;
            break;
        case OPT_VARIANT:
            if (*keymap_source == KEYMAP_SOURCE_FILE)
                goto keymap_source_error;
            names->variant = optarg;
            *keymap_source = KEYMAP_SOURCE_RMLVO;
            break;
        case OPT_OPTIONS:
            if (*keymap_source == KEYMAP_SOURCE_FILE)
                goto keymap_source_error;
            names->options = optarg;
            *keymap_source = KEYMAP_SOURCE_RMLVO;
            break;
        case 'h':
            usage(stdout, argv[0]);
            exit(EXIT_SUCCESS);
        default:
            goto invalid_usage;
        }
    }

    if (argc - optind != 1) {
        fprintf(stderr, "ERROR: missing positional parameter\n");
        goto invalid_usage;
    }

    /* Check for keymap input */
    if (*keymap_source == KEYMAP_SOURCE_AUTO &&
        is_pipe_or_regular_file(STDIN_FILENO)) {
        /* Piping detected */
        *keymap_source = KEYMAP_SOURCE_FILE;
    }
    if (isempty(*keymap_path) || strcmp(*keymap_path, "-") == 0)
        *keymap_path = NULL;

    *keysym = XKB_KEY_NoSymbol;
    if (!*keysym_mode) {
        /* Try to parse code point */
        const uint32_t codepoint = parse_char_or_codepoint(argv[optind]);
        if (codepoint != INVALID_UTF8_CODE_POINT) {
            *keysym = xkb_utf32_to_keysym(codepoint);
            if (*keysym == XKB_KEY_NoSymbol) {
                fprintf(stderr,
                        "ERROR: Failed to convert code point to keysym\n");
                goto invalid_usage;
            }
        } else {
            /* Try to parse as keysym */
        }
    }
    if (*keysym == XKB_KEY_NoSymbol) {
        /* Try to parse keysym name or hexadecimal value (0xNNNN) */
        *keysym = xkb_keysym_from_name(argv[optind], XKB_KEYSYM_NO_FLAGS);
        if (*keysym == XKB_KEY_NoSymbol) {
            /* Try to parse numeric keysym in base 10, without prefix */
            char *endp = NULL;
            errno = 0;
            const long int val = strtol(argv[optind], &endp, 10);
            if (errno != 0 || !isempty(endp) || val <= 0 || val > XKB_KEYSYM_MAX) {
                fprintf(stderr, "ERROR: Failed to convert argument to keysym\n");
                goto invalid_usage;
            }
            *keysym = (uint32_t) val;
        }
    }

    return EXIT_SUCCESS;

keymap_env_error:
    fprintf(stderr, "ERROR: --keymap is not compatible with "
                    "--enable-environment-names\n");
    goto invalid_usage;

keymap_source_error:
    fprintf(stderr, "ERROR: Cannot use RMLVO options with keymap input\n");
    goto invalid_usage;

invalid_usage:
    usage(stderr, argv[0]);
    return EXIT_INVALID_USAGE;
}

static struct xkb_keymap *
load_keymap(struct xkb_context *ctx, enum input_keymap_source keymap_source,
            enum xkb_keymap_format keymap_format, const char *keymap_path,
            struct xkb_rule_names *names)
{
    if (keymap_source == KEYMAP_SOURCE_FILE) {
        FILE *file = NULL;
        if (keymap_path) {
            /* Read from regular file */
            file = fopen(keymap_path, "rb");
        } else {
            /* Read from stdin */
            file = tools_read_stdin();
        }
        if (!file) {
            fprintf(stderr, "ERROR: Failed to open keymap file \"%s\": %s\n",
                    keymap_path ? keymap_path : "stdin", strerror(errno));
            return NULL;
        }
        return xkb_keymap_new_from_file(ctx, file, keymap_format,
                                        XKB_KEYMAP_COMPILE_NO_FLAGS);
    } else {
        return xkb_keymap_new_from_names2(ctx, names, keymap_format,
                                          XKB_KEYMAP_COMPILE_NO_FLAGS);
    }
}

int
main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    struct xkb_context *ctx = NULL;
    struct xkb_keymap *keymap = NULL;

    bool verbose = false;
    bool use_env_names = false;
    enum input_keymap_source keymap_source = KEYMAP_SOURCE_AUTO;
    enum xkb_keymap_format keymap_format = DEFAULT_INPUT_KEYMAP_FORMAT;
    const char *keymap_path = NULL;
    bool keysym_mode = false;
    xkb_keysym_t keysym = XKB_KEY_NoSymbol;
    struct xkb_rule_names names = {
        .rules = NULL,
        .model = NULL,
        .layout = NULL,
        .variant = NULL,
        .options = NULL,
    };

    int ret = parse_options(argc, argv, &verbose, &keysym_mode, &keysym,
                            &keymap_source, &keymap_format, &keymap_path,
                            &use_env_names, &names);
    if (ret != EXIT_SUCCESS)
        goto err;

    char name[XKB_KEYSYM_NAME_MAX_SIZE];
    ret = xkb_keysym_get_name(keysym, name, sizeof(name));
    if (ret < 0 || (size_t) ret >= sizeof(name)) {
        fprintf(stderr, "ERROR: Failed to get name of keysym\n");
        ret = EXIT_FAILURE;
        goto err;
    }
    ret = EXIT_FAILURE;

    const enum xkb_context_flags ctx_flags = (use_env_names)
                                           ? XKB_CONTEXT_NO_FLAGS
                                           : XKB_CONTEXT_NO_ENVIRONMENT_NAMES;
    ctx = xkb_context_new(ctx_flags);
    if (!ctx) {
        fprintf(stderr, "ERROR: Failed to create XKB context\n");
        goto err;
    }

    if (verbose)
        tools_enable_verbose_logging(ctx);

    keymap = load_keymap(ctx, keymap_source, keymap_format, keymap_path, &names);
    if (!keymap) {
        fprintf(stderr, "ERROR: Failed to create XKB keymap\n");
        goto err;
    }

    printf("keysym: %s (%#06"PRIx32")\n", name, keysym);
    printf("%-8s %-9s %-8s %-20s %-7s %-s\n",
           "KEYCODE", "KEY NAME", "LAYOUT", "LAYOUT NAME", "LEVEL#", "MODIFIERS");

    const xkb_keycode_t min_keycode = xkb_keymap_min_keycode(keymap);
    const xkb_keycode_t max_keycode = xkb_keymap_max_keycode(keymap);
    const xkb_mod_index_t num_mods = xkb_keymap_num_mods(keymap);
    for (xkb_keycode_t keycode = min_keycode; keycode <= max_keycode; keycode++) {
        const char* const key_name = xkb_keymap_key_get_name(keymap, keycode);
        if (!key_name) {
            continue;
        }

        const xkb_layout_index_t num_layouts =
            xkb_keymap_num_layouts_for_key(keymap, keycode);
        for (xkb_layout_index_t layout = 0; layout < num_layouts; layout++) {
            const char *layout_name = xkb_keymap_layout_get_name(keymap, layout);
            if (!layout_name) {
                layout_name = "?";
            }

            const xkb_level_index_t num_levels =
                xkb_keymap_num_levels_for_key(keymap, keycode, layout);
            for (xkb_level_index_t level = 0; level < num_levels; level++) {
                const xkb_keysym_t *syms;
                const int num_syms = xkb_keymap_key_get_syms_by_level(
                    keymap, keycode, layout, level, &syms
                );
                if (num_syms != 1) {
                    continue;
                }
                if (syms[0] != keysym) {
                    continue;
                }

                xkb_mod_mask_t masks[100];
                const size_t num_masks = xkb_keymap_key_get_mods_for_level(
                    keymap, keycode, layout, level, masks, ARRAY_SIZE(masks)
                );
                for (size_t i = 0; i < num_masks; i++) {
                     const xkb_mod_mask_t mask = masks[i];

                    printf("%-8u %-9s %-8u %-20s %-7u [ ",
                           keycode, key_name, layout + 1, layout_name, level + 1);
                    for (xkb_mod_index_t mod = 0; mod < num_mods; mod++) {
                        if ((mask & (UINT32_C(1) << mod)) == 0) {
                            continue;
                        }
                        printf("%s ", xkb_keymap_mod_get_name(keymap, mod));
                    }
                    printf("]\n");
                }
            }
        }
    }

    ret = EXIT_SUCCESS;
err:
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);
    return ret;
}
