/*
 * Copyright © 2024-2026 Pierre Le Marre <dev@wismill.eu>
 * Copyright (c) 2014-2021 Steven G. Johnson, Jiahao Chen, Peter Colberg,
 * Tony Kelman, Scott P. Jones, and other contributors.
 * Copyright (c) 2009 Public Software Group e. V., Berlin, Germany
 *
 * SPDX-License-Identifier: MIT
 *
 * Adapted from `utf8proc_iterate()` of utf8proc; see:
 * https://github.com/JuliaStrings/utf8proc/blob/f72a4023617e2d879f2ca93086fd02bc7e3149fe/utf8proc.c#L125
 */

#include "config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "utf8-decoding.h"
#include "utils.h"

/* Doc only: keep this private */
enum xkb_utf8_error {
    XKB_UTF8_INVALID = -1,
};

/** Check that a byte is a UTF-8 continuation byte (0b10xxxxxx) */
static bool
is_utf8_continuation(uint8_t c)
{
    return ((c & 0xc0) == 0x80);
}

ssize_t
xkb_utf8_iterate(const char *buf, size_t buf_size, uint32_t *cp)
{
    if (!buf_size)
        return 0;

    const uint8_t * const b = (const uint8_t *)buf;

    uint32_t b0 = b[0];

    if (b0 < 0x80) {
        *cp = b0;
        return 1;
    }

    /* Must be between 0xc2 and 0xf4 inclusive to be valid */
    if (b0 - 0xc2 > 0xf4 - 0xc2)
        return XKB_UTF8_INVALID;

    if (b0 < 0xe0) {
        /* 2-byte sequence: must have a valid continuation byte */
        if (buf_size < 2 || !is_utf8_continuation(b[1]))
            return XKB_UTF8_INVALID;
        *cp = ((b0 & 0x1f) << 6) | (b[1] & 0x3f);
        return 2;
    }

    if (b0 < 0xf0) {
        /* 3-byte sequence: must have 2 valid continuation bytes */
        if (buf_size < 3 ||
            !is_utf8_continuation(b[1]) ||
            !is_utf8_continuation(b[2]))
        {
            return XKB_UTF8_INVALID;
        }
        /* Check for surrogates */
        if (b0 == 0xed && b[1] > 0x9f)
            return XKB_UTF8_INVALID;
        b0 = (
            ((b0 & 0xf) << 12) |
            ((b[1] & 0x3f) << 6) |
            (b[2] & 0x3f)
        );
        /* Check for overlong encodings */
        if (b0 < 0x800)
            return XKB_UTF8_INVALID;
        *cp = b0;
        return 3;
    }

    /* 4-byte sequence: must have 3 valid continuation bytes */
    if (buf_size < 4 ||
        !is_utf8_continuation(b[1]) ||
        !is_utf8_continuation(b[2]) ||
        !is_utf8_continuation(b[3]))
    {
        return XKB_UTF8_INVALID;
    }
    /* Make sure it is in the correct range (U+10000..U+10FFFF) */
    if (b0 == 0xf0) {
        if (b[1] < 0x90)
            return XKB_UTF8_INVALID;
    } else if (b0 == 0xf4) {
        if (b[1] > 0x8f)
            return XKB_UTF8_INVALID;
    }
    *cp = (
        ((b0 & 7) << 18) |
        ((b[1] & 0x3f) << 12) |
        ((b[2] & 0x3f) << 6) |
        (b[3] & 0x3f)
    );
    return 4;
}
