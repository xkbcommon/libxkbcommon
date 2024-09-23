/*
 * Copyright © 2020 Ran Benita <ran@unusedvar.com>
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

#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "xkbcommon/xkbcommon.h"
#include "src/utils.h"
#include "src/keysym.h"
#include "src/utf8-decoding.h"

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
usage(const char *argv0, FILE *fp)
{
    fprintf(fp, "Usage: %s [--help] [--keysym] [--rules <rules>] [--model <model>] "
                "[--layout <layout>] [--variant <variant>] [--options <options>]"
                " <character/codepoint/keysym>\n", argv0);
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
        "- a keysym if --keysym is used: either a numeric value (hexadecimal if prefixed\n"
        "  with '0x' else decimal) or a keysym name.\n"
        "\n"
        "Options:\n"
        " --help\n"
        "    Print this help and exit\n"
        " --keysym\n"
        "    Treat the argument as a keysym, not a Unicode code point\n"
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
        DEFAULT_XKB_RULES, DEFAULT_XKB_MODEL, DEFAULT_XKB_LAYOUT,
        DEFAULT_XKB_VARIANT ? DEFAULT_XKB_VARIANT : "<none>",
        DEFAULT_XKB_OPTIONS ? DEFAULT_XKB_OPTIONS : "<none>");
}

int
main(int argc, char *argv[])
{
    const char *rules = NULL;
    const char *model = NULL;
    const char *layout_ = NULL;
    const char *variant = NULL;
    const char *options = NULL;
    bool keysym_mode = false;
    int err = EXIT_FAILURE;
    struct xkb_context *ctx = NULL;
    char *endp;
    long val;
    uint32_t codepoint;
    xkb_keysym_t keysym;
    int ret;
    char name[XKB_KEYSYM_NAME_MAX_SIZE];
    struct xkb_keymap *keymap = NULL;
    xkb_keycode_t min_keycode, max_keycode;
    xkb_mod_index_t num_mods;
    enum options {
        OPT_KEYSYM,
        OPT_RULES,
        OPT_MODEL,
        OPT_LAYOUT,
        OPT_VARIANT,
        OPT_OPTIONS,
    };
    static struct option opts[] = {
        {"help",                 no_argument,            0, 'h'},
        {"keysym",               no_argument,            0, OPT_KEYSYM},
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
        case OPT_KEYSYM:
            keysym_mode = true;
            break;
        case OPT_RULES:
            rules = optarg;
            break;
        case OPT_MODEL:
            model = optarg;
            break;
        case OPT_LAYOUT:
            layout_ = optarg;
            break;
        case OPT_VARIANT:
            variant = optarg;
            break;
        case OPT_OPTIONS:
            options = optarg;
            break;
        case 'h':
            usage(argv[0], stdout);
            exit(EXIT_SUCCESS);
        default:
            usage(argv[0], stderr);
            exit(EXIT_INVALID_USAGE);
        }
    }
    if (argc - optind != 1) {
        fprintf(stderr, "ERROR: missing positional parameter\n");
        goto parse_error;
    }

    if (keysym_mode) {
        // Try to parse keysym name or hexadecimal value (0xNNNN)
        keysym = xkb_keysym_from_name(argv[optind], XKB_KEYSYM_NO_FLAGS);
        if (keysym == XKB_KEY_NoSymbol) {
            // Try to parse numeric keysym in base 10, without prefix
            val = strtol(argv[optind], &endp, 10);
            if (errno != 0 || !isempty(endp) || val <= 0 || val > XKB_KEYSYM_MAX) {
                fprintf(stderr, "ERROR: Failed to convert argument to keysym\n");
                goto parse_error;
            }
            keysym = (uint32_t) val;
        }
    } else {
        codepoint = parse_char_or_codepoint(argv[optind]);
        if (codepoint == INVALID_UTF8_CODE_POINT)
            goto parse_error;

        keysym = xkb_utf32_to_keysym(codepoint);
        if (keysym == XKB_KEY_NoSymbol) {
            fprintf(stderr, "ERROR: Failed to convert code point to keysym\n");
            goto parse_error;
        }
    }

    ret = xkb_keysym_get_name(keysym, name, sizeof(name));
    if (ret < 0 || (size_t) ret >= sizeof(name)) {
        fprintf(stderr, "ERROR: Failed to get name of keysym\n");
        goto err;
    }

    ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx) {
        fprintf(stderr, "ERROR: Failed to create XKB context\n");
        goto err;
    }

    struct xkb_rule_names names = {
        .rules = rules,
        .model = model,
        .layout = layout_,
        .variant = variant,
        .options = options,
    };
    keymap = xkb_keymap_new_from_names(ctx, &names,
                                       XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        fprintf(stderr, "ERROR: Failed to create XKB keymap\n");
        goto err;
    }

    printf("keysym: %s (%#x)\n", name, keysym);
    printf("%-8s %-9s %-8s %-20s %-7s %-s\n",
           "KEYCODE", "KEY NAME", "LAYOUT", "LAYOUT NAME", "LEVEL#", "MODIFIERS");

    min_keycode = xkb_keymap_min_keycode(keymap);
    max_keycode = xkb_keymap_max_keycode(keymap);
    num_mods = xkb_keymap_num_mods(keymap);
    for (xkb_keycode_t keycode = min_keycode; keycode <= max_keycode; keycode++) {
        const char *key_name;
        xkb_layout_index_t num_layouts;

        key_name = xkb_keymap_key_get_name(keymap, keycode);
        if (!key_name) {
            continue;
        }

        num_layouts = xkb_keymap_num_layouts_for_key(keymap, keycode);
        for (xkb_layout_index_t layout = 0; layout < num_layouts; layout++) {
            const char *layout_name;
            xkb_level_index_t num_levels;

            layout_name = xkb_keymap_layout_get_name(keymap, layout);
            if (!layout_name) {
                layout_name = "?";
            }

            num_levels = xkb_keymap_num_levels_for_key(keymap, keycode, layout);
            for (xkb_level_index_t level = 0; level < num_levels; level++) {
                int num_syms;
                const xkb_keysym_t *syms;
                size_t num_masks;
                xkb_mod_mask_t masks[100];

                num_syms = xkb_keymap_key_get_syms_by_level(
                    keymap, keycode, layout, level, &syms
                );
                if (num_syms != 1) {
                    continue;
                }
                if (syms[0] != keysym) {
                    continue;
                }

                num_masks = xkb_keymap_key_get_mods_for_level(
                    keymap, keycode, layout, level, masks, ARRAY_SIZE(masks)
                );
                for (size_t i = 0; i < num_masks; i++) {
                    xkb_mod_mask_t mask = masks[i];

                    printf("%-8u %-9s %-8u %-20s %-7u [ ",
                           keycode, key_name, layout + 1, layout_name, level + 1);
                    for (xkb_mod_index_t mod = 0; mod < num_mods; mod++) {
                        if ((mask & (1 << mod)) == 0) {
                            continue;
                        }
                        printf("%s ", xkb_keymap_mod_get_name(keymap, mod));
                    }
                    printf("]\n");
                }
            }
        }
    }

    err = EXIT_SUCCESS;
err:
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);
    return err;
parse_error:
    usage(argv[0], stderr);
    exit(EXIT_INVALID_USAGE);
}
