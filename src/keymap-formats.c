/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 *
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <string.h>

#include "utils.h"
#include "keymap-formats.h"

/* [WARNING] Must be in ascending order */
static const enum xkb_keymap_format keymap_formats[] = {
    XKB_KEYMAP_FORMAT_TEXT_V1,
    XKB_KEYMAP_FORMAT_TEXT_V2,
};

/*
 * Human-friendly formats labels
 *
 * They are aimed to be used in the CLI tools and to be *stable*. While the
 * current labels are simply “v” + corresponding `xkb_keymap_format` values, it
 * may change in the future. Indeed, `xkb_keymap_format` encoding may change
 * (although we avoit it) while the corresponding labels remain unchanged.
 */
struct format_label {
    const char* label;
    enum xkb_keymap_format format;
};
static const struct format_label keymap_formats_labels [] = {
    { "v1", XKB_KEYMAP_FORMAT_TEXT_V1 },
    { "v2", XKB_KEYMAP_FORMAT_TEXT_V2 },
};

size_t
xkb_keymap_supported_formats(const enum xkb_keymap_format **formats)
{
    *formats = keymap_formats;
    return ARRAY_SIZE(keymap_formats);
}

bool
xkb_keymap_is_supported_format(enum xkb_keymap_format format)
{
    for (size_t k = 0; k < ARRAY_SIZE(keymap_formats); k++) {
        if (keymap_formats[k] == format)
            return true;
        /* Short-circuit because array is sorted */
        if (keymap_formats[k] > format)
            return false;
    }
    return false;
}

enum xkb_keymap_format
xkb_keymap_parse_format(const char *raw)
{
    if (!raw)
        return 0;

    if (raw[0] != 'v') {
        /* Parse numeric format */
        const int format = atoi(raw);
        return (xkb_keymap_is_supported_format(format)) ? format : 0;
    } else {
        /* Parse format label: “vXXX” */
        for (size_t k = 0; k < ARRAY_SIZE(keymap_formats_labels); k++) {
            if (strcmp(raw, keymap_formats_labels[k].label) == 0)
                return keymap_formats_labels[k].format;
        }
        return 0;
    }
}
