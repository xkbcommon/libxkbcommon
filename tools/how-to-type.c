/*
 * Copyright © 2020 Ran Benita <ran@unusedvar.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-compose.h"
#include "xkbcommon/xkbcommon-keysyms.h"
#include "src/compose/constants.h"
#include "src/darray.h"
#include "src/keysym.h"
#include "src/keymap-formats.h"
#include "src/utils.h"
#include "src/utf8-decoding.h"
#include "tools-common.h"

#define ARRAY_SIZE(arr) ((sizeof(arr) / sizeof(*(arr))))
#define MAX_TYPE_MAP_ENTRIES 100

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
        int base = 10;
        /* Detect U+NNNN format standard Unicode code point format */
        if (raw_length >= 2 && raw[0] == 'U' && raw[1] == '+') {
            base = 16;
            raw += 2;
        }
        /* Use strtol with explicit bases instead of `0` in order to avoid
         * unexpected parsing as octal. */
        long val = -1;
        for (; base <= 16; base += 6) {
            errno = 0;
            char *endp;
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
    fprintf(fp, "Usage: %s [--help] [--version] [--verbose] [--keysym] [--disable-compose] "
                "[--rules <rules>] [--model <model>] "
                "[--layout <layout>] [--variant <variant>] "
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
        " --version\n"
        "    Print version information and exit\n"
        " --verbose\n"
        "    Enable verbose debugging output\n"
        " --keysym\n"
        "    Treat the argument only as a keysym\n"
        " --disable-compose\n"
        "    Disable Compose support to query combinations involving e.g. dead keys\n"
        "\n"
        "XKB-specific options:\n"
        " --include <path>\n"
        "    Add the given <path> to the include path list. This option is\n"
        "    order-dependent, include paths given first are searched first.\n"
        "    If an include path is given, the default include path list is\n"
        "    not used. Use --include-defaults to add the default include\n"
        "    paths.\n"
        " --include-defaults\n"
        "    Add the default set of include directories.\n"
        "    This option is order-dependent, include paths given first\n"
        "    are searched first.\n"
        " --format <format>\n"
        "    The keymap format to use (default: '%s')\n"
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
        xkb_keymap_get_format_label(DEFAULT_INPUT_KEYMAP_FORMAT),
        DEFAULT_XKB_RULES, DEFAULT_XKB_MODEL, DEFAULT_XKB_LAYOUT,
        DEFAULT_XKB_VARIANT ? DEFAULT_XKB_VARIANT : "<none>",
        DEFAULT_XKB_OPTIONS ? DEFAULT_XKB_OPTIONS : "<none>");
}

enum input_keymap_source {
    KEYMAP_SOURCE_AUTO,
    KEYMAP_SOURCE_RMLVO,
    KEYMAP_SOURCE_FILE
};

struct cli_input {
    const char *keymap_path;
    const char *includes[64];
    size_t num_includes;
    struct xkb_rule_names names;
    xkb_keysym_t keysym;
    enum input_keymap_source keymap_source;
    enum xkb_keymap_format keymap_format;
    bool verbose;
    bool use_env_names;
    bool keysym_mode;
    bool use_compose;
};

static int
parse_options(int argc, char **argv, struct cli_input *input)
{
    enum options {
        OPT_VERBOSE,
        OPT_KEYSYM,
        OPT_DISABLE_COMPOSE,
        OPT_INCLUDE,
        OPT_INCLUDE_DEFAULTS,
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
        {"version",              no_argument,            0, 'V'},
        {"verbose",              no_argument,            0, OPT_VERBOSE},
        {"keysym",               no_argument,            0, OPT_KEYSYM},
        {"disable-compose",      no_argument,            0, OPT_DISABLE_COMPOSE},
        {"include",              required_argument,      0, OPT_INCLUDE},
        {"include-defaults",     no_argument,            0, OPT_INCLUDE_DEFAULTS},
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

        opt = getopt_long(argc, argv, "hV", opts, &option_index);
        if (opt == -1)
            break;

        switch (opt) {
        case OPT_VERBOSE:
            input->verbose = true;
            break;
        case OPT_KEYSYM:
            input->keysym_mode = true;
            break;
        case OPT_DISABLE_COMPOSE:
            input->use_compose = false;
            break;
        case OPT_INCLUDE:
            if (input->num_includes >= ARRAY_SIZE(input->includes))
                goto too_many_includes;
            input->includes[input->num_includes++] = optarg;
            break;
        case OPT_INCLUDE_DEFAULTS:
            if (input->num_includes >= ARRAY_SIZE(input->includes))
                goto too_many_includes;
            input->includes[input->num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;
            break;
        case OPT_ENABLE_ENV_NAMES:
            if (input->keymap_source == KEYMAP_SOURCE_FILE)
                goto keymap_env_error;
            input->use_env_names = true;
            input->keymap_source = KEYMAP_SOURCE_RMLVO;
            break;
        case OPT_KEYMAP_FORMAT:
            input->keymap_format = xkb_keymap_parse_format(optarg);
            if (!input->keymap_format) {
                fprintf(stderr, "ERROR: invalid --format \"%s\"\n", optarg);
                goto invalid_usage;
            }
            break;
        case OPT_KEYMAP:
            if (input->keymap_source == KEYMAP_SOURCE_RMLVO)
                goto keymap_source_error;
            input->keymap_source = KEYMAP_SOURCE_FILE;
            input->keymap_path = optarg;
            break;
        case OPT_RULES:
            if (input->keymap_source == KEYMAP_SOURCE_FILE)
                goto keymap_source_error;
            input->names.rules = optarg;
            input->keymap_source = KEYMAP_SOURCE_RMLVO;
            break;
        case OPT_MODEL:
            if (input->keymap_source == KEYMAP_SOURCE_FILE)
                goto keymap_source_error;
            input->names.model = optarg;
            input->keymap_source = KEYMAP_SOURCE_RMLVO;
            break;
        case OPT_LAYOUT:
            if (input->keymap_source == KEYMAP_SOURCE_FILE)
                goto keymap_source_error;
            input->names.layout = optarg;
            input->keymap_source = KEYMAP_SOURCE_RMLVO;
            break;
        case OPT_VARIANT:
            if (input->keymap_source == KEYMAP_SOURCE_FILE)
                goto keymap_source_error;
            input->names.variant = optarg;
            input->keymap_source = KEYMAP_SOURCE_RMLVO;
            break;
        case OPT_OPTIONS:
            if (input->keymap_source == KEYMAP_SOURCE_FILE)
                goto keymap_source_error;
            input->names.options = optarg;
            input->keymap_source = KEYMAP_SOURCE_RMLVO;
            break;
        case 'h':
            usage(stdout, argv[0]);
            exit(EXIT_SUCCESS);
        case 'V':
            printf("%s\n", LIBXKBCOMMON_VERSION);
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
    if (input->keymap_source == KEYMAP_SOURCE_AUTO &&
        is_pipe_or_regular_file(STDIN_FILENO)) {
        /* Piping detected */
        input->keymap_source = KEYMAP_SOURCE_FILE;
    }
    if (isempty(input->keymap_path) || strcmp(input->keymap_path, "-") == 0)
        input->keymap_path = NULL;

    input->keysym = XKB_KEY_NoSymbol;
    if (!input->keysym_mode) {
        /* Try to parse code point */
        const uint32_t codepoint = parse_char_or_codepoint(argv[optind]);
        if (codepoint != INVALID_UTF8_CODE_POINT) {
            input->keysym = xkb_utf32_to_keysym(codepoint);
            if (input->keysym == XKB_KEY_NoSymbol) {
                fprintf(stderr,
                        "ERROR: Failed to convert code point to keysym\n");
                goto invalid_usage;
            }
        } else {
            /* Try to parse as keysym */
        }
    }
    if (input->keysym == XKB_KEY_NoSymbol) {
        /* Try to parse keysym name or hexadecimal value (0xNNNN) */
        input->keysym = xkb_keysym_from_name(argv[optind], XKB_KEYSYM_NO_FLAGS);
        if (input->keysym == XKB_KEY_NoSymbol) {
            /* Try to parse numeric keysym in base 10, without prefix */
            char *endp = NULL;
            errno = 0;
            const long int val = strtol(argv[optind], &endp, 10);
            if (errno != 0 || !isempty(endp) || val <= 0 || val > XKB_KEYSYM_MAX) {
                fprintf(stderr, "ERROR: Failed to convert argument to keysym\n");
                goto invalid_usage;
            }
            input->keysym = (uint32_t) val;
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

too_many_includes:
    fprintf(stderr, "ERROR: too many includes (max: %zu)\n",
            ARRAY_SIZE(input->includes));

invalid_usage:
    usage(stderr, argv[0]);
    return EXIT_INVALID_USAGE;
}

static struct xkb_keymap *
load_keymap(struct xkb_context *ctx, struct cli_input *input)
{
    if (input->keymap_source == KEYMAP_SOURCE_FILE) {
        FILE *file = NULL;
        if (input->keymap_path) {
            /* Read from regular file */
            file = fopen(input->keymap_path, "rb");
        } else {
            /* Read from stdin */
            file = tools_read_stdin();
        }
        if (!file) {
            fprintf(stderr, "ERROR: Failed to open keymap file \"%s\": %s\n",
                    input->keymap_path ? input->keymap_path : "stdin",
                    strerror(errno));
            return NULL;
        }
        return xkb_keymap_new_from_file(ctx, file, input->keymap_format,
                                        XKB_KEYMAP_COMPILE_NO_FLAGS);
    } else {
        return xkb_keymap_new_from_names2(ctx, &input->names, input->keymap_format,
                                          XKB_KEYMAP_COMPILE_NO_FLAGS);
    }
}

/** The left-hand side of a Compose sequence */
struct compose_lhs {
    size_t count;
    xkb_keysym_t keysyms[COMPOSE_MAX_LHS_LEN];
};
typedef darray(struct compose_lhs) darray_compose;

/** Given a keysym, gather the Compose sequences that produce it */
static bool
lookup_compose_sequences(struct xkb_compose_table *table,
                         darray_compose *entries, xkb_keysym_t keysym)
{
    struct xkb_compose_table_iterator *iter = xkb_compose_table_iterator_new(table);
    if (!iter) {
        fprintf(stderr, "ERROR: cannot iterate Compose table\n");
        return false;
    }

    char utf8[5] = "";
    const bool has_utf8 = xkb_keysym_to_utf8(keysym, utf8, sizeof(utf8)) > 0;

    struct xkb_compose_table_entry *entry;
    while ((entry = xkb_compose_table_iterator_next(iter))) {
        if (keysym != xkb_compose_table_entry_keysym(entry) &&
            /* Keysyms do not match, but maybe the UTF-8 strings do */
            (!has_utf8 ||
             strcmp(utf8, xkb_compose_table_entry_utf8(entry)) != 0))
        {
            continue;
        }

        size_t count = 0;
        const xkb_keysym_t * const seq =
            xkb_compose_table_entry_sequence(entry, &count);
        const darray_size_t idx = darray_size(*entries);
        darray_resize0(*entries, idx + 1);
        for (size_t k = 0; k < count; k++) {
            darray_item(*entries, idx).keysyms[k] = seq[k];
        }
        darray_item(*entries, idx).count = count;
    }

    xkb_compose_table_iterator_free(iter);
    return true;
}

/** The location in a keymap of a keysym used in a Compose sequence */
struct keysym_entry {
    xkb_keycode_t keycode;
    xkb_layout_index_t layout;
    xkb_level_index_t level;
    xkb_mod_mask_t mask;
};
/** A keysym used in a Compose sequence and all its location in the keymap */
struct keysym_entries {
    xkb_keysym_t keysym;
    darray(struct keysym_entry) entries;
};
typedef darray(struct keysym_entries) darray_keysym_entries;

/** Comparison function to sort entries and display user-friendly key combos */
static int
keysym_entry_compare(const void *a, const void *b)
{
    const struct keysym_entry * const e1 = a;
    const struct keysym_entry * const e2 = b;
    if (e1->layout < e2->layout)
        return -1;
    if (e1->layout > e2->layout)
        return 1;
    if (e1->level < e2->level)
        return -1;
    if (e1->level > e2->level)
        return 1;
    if (e1->keycode < e2->keycode)
        return -1;
    if (e1->keycode > e2->keycode)
        return 1;
    return 0;
}

static void
darray_keysym_entries_free(darray_keysym_entries *entries)
{
    struct keysym_entries *e;
    darray_foreach(e, *entries)
        darray_free(e->entries);
    darray_free(*entries);
}

/** Lookup for an existing keysym entry and possibly insert it if it is missing */
static struct keysym_entries *
lookup_keysym_entries(darray_keysym_entries *entries, xkb_keysym_t keysym,
                      bool insert)
{
    /* We do not expect a large set of keysym, so a simple linear search is fine */
    struct keysym_entries *e;
    darray_foreach(e, *entries) {
        if (e->keysym == keysym)
            return e;
    }

    if (insert) {
        const darray_size_t idx = darray_size(*entries);
        const struct keysym_entries new = {
            .keysym = keysym,
            .entries = darray_new()
        };
        darray_append(*entries, new);
        return &darray_item(*entries, idx);
    } else {
        return NULL;
    }
}

/* These are fixed */
#define SHIFT_MASK UINT32_C(0x1)
#define LOCK_MASK  UINT32_C(0x2)
#define SHIFT_LOCK_MASK (SHIFT_MASK | LOCK_MASK)

/**
 * Given a keysym, add its position in the keymap if it is used in a
 * Compose sequence
 */
static bool
add_compose_keysym_entry(struct xkb_keymap *keymap,
                         darray_keysym_entries *entries, xkb_keysym_t keysym,
                         xkb_keycode_t keycode, xkb_layout_index_t layout,
                         xkb_level_index_t level)
{
    struct keysym_entries * const entry =
        lookup_keysym_entries(entries, keysym, false);
    if (!entry)
        return false;

    /*
     * The keysym position may be reached by multiple modifiers combinations:
     * append an entry for all but combo including another one, so that we
     * avoid combinatorial explosion.
     */
    xkb_mod_mask_t masks[MAX_TYPE_MAP_ENTRIES];
    bool skip[MAX_TYPE_MAP_ENTRIES] = {0};
    const size_t num_masks = xkb_keymap_key_get_mods_for_level(
        keymap, keycode, layout, level, masks, ARRAY_SIZE(masks)
    );
    for (size_t j = 0; j < num_masks; j++) {
        for (size_t k = j + 1; k < num_masks; k++) {
            /* skip “shift cancel lock” */
            if (!masks[j]) {
                if (masks[k] == SHIFT_LOCK_MASK)
                    skip[k] = true;
                else
                    continue;
            } else if (!masks[k]) {
                if (masks[j] == SHIFT_LOCK_MASK)
                    skip[j] = true;
                else
                    continue;
            }

            /* skip any mask that contains another mask of the list */
            if ((masks[j] & masks[k]) == masks[j])
                skip[k] = true;
            else if ((masks[j] & masks[k]) == masks[k])
                skip[j] = true;
        }
        if (skip[j])
            continue;
        const struct keysym_entry new = {
            .keycode = keycode,
            .layout = layout,
            .level = level,
            .mask = masks[j]
        };
        darray_append(entry->entries, new);
    }
    return true;
}

enum { LAYOUT_NAME_MIN_WIDTH = 20 };

static void
print_combo(struct xkb_keymap *keymap, xkb_mod_index_t num_mods,
            xkb_keycode_t keycode, const char *key_name,
            xkb_layout_index_t layout, const char *layout_name,
            xkb_level_index_t level, xkb_mod_mask_t mask)
{
    if (xkb_keymap_num_layouts(keymap) > 1 &&
        xkb_keymap_num_layouts_for_key(keymap, keycode) == 1)
    {
        /*
         * All layouts would default to the same mapping for the key,
         * so prefer displaying this info.
         */
        static const char fallback_layout_name[] = "(any: fallback to 1)";
        static_assert(sizeof(fallback_layout_name) - 1 <=
                      (size_t)LAYOUT_NAME_MIN_WIDTH,
                      "fallback layout name must fit the layout name column");
        layout_name = fallback_layout_name;
    }

    printf("%-8u %-9s %-8u %-*s %-7u [ ",
           keycode, key_name, layout + 1,
           LAYOUT_NAME_MIN_WIDTH, layout_name, level + 1);

    for (xkb_mod_index_t mod = 0; mod < num_mods; mod++) {
        const xkb_mod_mask_t mod_mask = xkb_keymap_mod_get_mask2(keymap, mod);
        if ((mask & mod_mask) != mod_mask)
            continue;
        printf("%s ", xkb_keymap_mod_get_name(keymap, mod));
    }
    printf("]\n");
}

int
main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    struct xkb_context *ctx = NULL;
    struct xkb_keymap *keymap = NULL;

    struct cli_input input = {
        .verbose = false,
        .use_compose = true,
        .use_env_names = false,
        .keymap_source = KEYMAP_SOURCE_AUTO,
        .keymap_format = DEFAULT_INPUT_KEYMAP_FORMAT,
        .keymap_path = NULL,
        .keysym_mode = false,
        .keysym = XKB_KEY_NoSymbol,
        .names = {
            .rules = NULL,
            .model = NULL,
            .layout = NULL,
            .variant = NULL,
            .options = NULL,
        },
    };

    int ret = parse_options(argc, argv, &input);
    if (ret != EXIT_SUCCESS)
        goto err;

    char name[XKB_KEYSYM_NAME_MAX_SIZE];
    ret = xkb_keysym_get_name(input.keysym, name, sizeof(name));
    if (ret < 0 || (size_t) ret >= sizeof(name)) {
        fprintf(stderr, "ERROR: Failed to get name of keysym\n");
        ret = EXIT_FAILURE;
        goto err;
    }

    ret = EXIT_FAILURE;

    const enum xkb_context_flags ctx_flags = (
        XKB_CONTEXT_NO_DEFAULT_INCLUDES |
        ((input.use_env_names) ? XKB_CONTEXT_NO_FLAGS
                               : XKB_CONTEXT_NO_ENVIRONMENT_NAMES)
    );
    ctx = xkb_context_new(ctx_flags);
    if (!ctx) {
        fprintf(stderr, "ERROR: Failed to create XKB context\n");
        goto err;
    }

    if (input.verbose)
        tools_enable_verbose_logging(ctx);

    if (input.num_includes == 0)
        input.includes[input.num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;

    for (size_t i = 0; i < input.num_includes; i++) {
        const char *include = input.includes[i];
        if (strcmp(include, DEFAULT_INCLUDE_PATH_PLACEHOLDER) == 0) {
            if (!xkb_context_include_path_append_default(ctx)) {
                fprintf(stderr,
                        "ERROR: Failed to append default include paths\n");
                goto err;
            }
        } else {
            if (!xkb_context_include_path_append(ctx, include)) {
                fprintf(stderr,
                        "ERROR: Failed to append include path: %s\n",
                        include);
                goto err;
            }
        }
    }

    keymap = load_keymap(ctx, &input);
    if (!keymap) {
        fprintf(stderr, "ERROR: Failed to create XKB keymap\n");
        goto err;
    }

    /* Gather Compose sequences producing the given keysym */
    darray_compose compose_entries = darray_new();
    darray_keysym_entries keysym_entries = darray_new();
    if (input.use_compose) {
        /* Load the Compose table */
        const char *locale = setlocale(LC_CTYPE, NULL);
        struct xkb_compose_table * const compose_table =
            xkb_compose_table_new_from_locale(ctx, locale,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
        if (!compose_table) {
            fprintf(stderr, "Couldn't create compose from locale\n");
            goto compose_locale_error;
        }

        /* Get sequences producing the keysym */
        lookup_compose_sequences(compose_table, &compose_entries, input.keysym);

        xkb_compose_table_unref(compose_table);

        /* Get all the keysyms required in sequences */
        struct compose_lhs *compose_entry;
        darray_foreach(compose_entry, compose_entries) {
            for (size_t k = 0; k < compose_entry->count; k++) {
                lookup_keysym_entries(&keysym_entries,
                                      compose_entry->keysyms[k], true);
            }
        }
    }

    printf("keysym: %s (%#06"PRIx32")\n\n", name, input.keysym);
    printf("=== Direct access ===\n\n");
    printf("%-8s %-9s %-8s %-*s %-7s %-s\n",
           "KEYCODE", "KEY NAME", "LAYOUT",
           LAYOUT_NAME_MIN_WIDTH, "LAYOUT NAME",
           "LEVEL#", "MODIFIERS");

    /* Iterate over all the positions in the keymap */
    const xkb_keycode_t min_keycode = xkb_keymap_min_keycode(keymap);
    const xkb_keycode_t max_keycode = xkb_keymap_max_keycode(keymap);
    const xkb_mod_index_t num_mods = xkb_keymap_num_mods(keymap);
    darray(xkb_keysym_t) key_compose_keysyms = darray_new();
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
            darray_resize(key_compose_keysyms, 0);
            for (xkb_level_index_t level = 0; level < num_levels; level++) {
                const xkb_keysym_t *syms;
                const int num_syms = xkb_keymap_key_get_syms_by_level(
                    keymap, keycode, layout, level, &syms
                );
                if (num_syms != 1) {
                    /* We only deal with levels with exactly one keysym */
                    continue;
                }
                if (syms[0] != input.keysym) {
                    /*
                     * If it is not the keysym we look for, then check if it is
                     * a keysym that contribute to a Compose sequence that
                     * produce it. For our use case there is no point to list
                     * Compose sequences producing a keysym that is also in the
                     * the sequence producing it.
                     *
                     * Keep only the lowest level, so that we avoid combinatorial
                     * explosion.
                     */
                    /* Look for the current keysym in previous levels */
                    for (darray_size_t k = 0;
                         k < darray_size(key_compose_keysyms);
                         k++) {
                        if (darray_item(key_compose_keysyms, k) == syms[0]) {
                            /* Found it: skip this level */
                            goto next_level;
                        }
                    }
                    /* First occurrence of the keysym on the layout/key */
                    darray_append(key_compose_keysyms, syms[0]);
                    /* Add the keysym position if in a Compose sequence */
                    add_compose_keysym_entry(
                        keymap, &keysym_entries, syms[0],
                        keycode, layout, level
                    );
next_level:
                    continue;
                }

                /* Found our keysym: print the combo that generates it */
                xkb_mod_mask_t masks[MAX_TYPE_MAP_ENTRIES];
                const size_t num_masks = xkb_keymap_key_get_mods_for_level(
                    keymap, keycode, layout, level, masks, ARRAY_SIZE(masks)
                );
                for (size_t i = 0; i < num_masks; i++) {
                    print_combo(keymap, num_mods, keycode, key_name,
                                layout, layout_name, level, masks[i]);
                }
            }
        }
    }
    darray_free(key_compose_keysyms);

    /* Compose sequences */
    if (input.use_compose) {
        printf("\n=== Access via Compose sequences ===\n\n");
        printf("#  %-*s %-8s %-9s %-8s %-*s %-7s %-s\n",
               XKB_KEYSYM_NAME_MAX_SIZE - 1, "KEYSYM", "KEYCODE", "KEY NAME",
               "LAYOUT", LAYOUT_NAME_MIN_WIDTH, "LAYOUT NAME", "LEVEL#",
               "MODIFIERS");

        /* Sort keysyms positions */
        struct keysym_entries *entry;
        darray_foreach(entry, keysym_entries) {
            if (!darray_size(entry->entries))
                continue;
            qsort(darray_items(entry->entries), darray_size(entry->entries),
                  sizeof(*darray_items(entry->entries)), keysym_entry_compare);
        }
    }
    struct compose_lhs *compose_entry;
    size_t count = 0;
    darray_foreach(compose_entry, compose_entries) {
        /* Check we got all the keysyms required by the Compose sequence */
        struct {
            size_t index;
            struct keysym_entries *entry;
        } indexes[COMPOSE_MAX_LHS_LEN] = {0};
        bool enabled = true;
        for (size_t k = 0; k < compose_entry->count; k++) {
            struct keysym_entries *entry = lookup_keysym_entries(
                &keysym_entries, compose_entry->keysyms[k], false
            );
            if (!entry || !darray_size(entry->entries)) {
                enabled = false;
                break;
            }
            indexes[k].index = 0;
            indexes[k].entry = entry;
        }
        if (!enabled) {
            continue;
        }

        /*
         * Compose sequence is reachable with the keymap: iterate over the
         * cartesian product of the keysyms’ positions.
         */
        bool more_sequences = true;
        while (more_sequences) {
            /*
             * Discard combinations that mix layouts. Only apply to keys with
             * more that one layout.
             */
            xkb_layout_index_t layout = XKB_LAYOUT_INVALID;
            for (size_t k = 0; k < compose_entry->count; k++) {
                struct keysym_entries *entry = indexes[k].entry;
                struct keysym_entry *e =
                    &darray_item(entry->entries, indexes[k].index);
                if (layout == XKB_LAYOUT_INVALID &&
                    xkb_keymap_num_layouts_for_key(keymap, e->keycode) > 1) {
                    layout = e->layout;
                }
                if (layout != XKB_LAYOUT_INVALID &&
                    xkb_keymap_num_layouts_for_key(keymap, e->keycode) > 1 &&
                    e->layout != layout) {
                    /* Skip combinations that mix layouts */
                    goto skip;
                }
            }

            if (count++ > 0)
                printf("---\n");

            /* Display the Compose sequence */
            for (size_t k = 0; k < compose_entry->count; k++) {
                struct keysym_entries *entry = indexes[k].entry;
                struct keysym_entry *e =
                    &darray_item(entry->entries, indexes[k].index);

                const char* const key_name =
                    xkb_keymap_key_get_name(keymap, e->keycode);
                assert(key_name); /* Already skipped unamed keys above */
                const char * layout_name =
                    xkb_keymap_layout_get_name(keymap, e->layout);
                if (!layout_name) {
                    layout_name = "?";
                }
                char buf[XKB_KEYSYM_NAME_MAX_SIZE] = {0};
                xkb_keysym_get_name(entry->keysym, buf, ARRAY_SIZE(buf));

                if (k > 0)
                    printf("   ");
                else
                    printf("%02zu ", count);
                printf("%-*s ", XKB_KEYSYM_NAME_MAX_SIZE - 1, buf);
                print_combo(keymap, num_mods,
                            e->keycode, key_name,
                            e->layout, layout_name,
                            e->level, e->mask);
            }
skip:
            /* Cartesian product: find the rightmost list that can be advanced */
            {
                size_t k = compose_entry->count;
                while (k > 0) {
                    k--;
                    indexes[k].index++;
                    if (indexes[k].index <
                        darray_size(indexes[k].entry->entries)) {
                        k++;
                        break;
                    }
                    indexes[k].index = 0;
                }
                if (k == 0)
                    more_sequences = false;
            }
        }
    }

    ret = EXIT_SUCCESS;

compose_locale_error:
    darray_free(compose_entries);
    darray_keysym_entries_free(&keysym_entries);

err:

    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);
    return ret;
}
