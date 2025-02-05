/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2014 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 */

#include "config.h"

#include "utils.h"
#include "utf8.h"

/* Conformant encoding form conversion from UTF-32 to UTF-8.
 *
 * See https://www.unicode.org/versions/Unicode15.0.0/ch03.pdf#G28875
 * for further details.
*/
int
utf32_to_utf8(uint32_t unichar, char *buffer)
{
    int count, shift, length;
    uint8_t head;

    if (unichar <= 0x007f) {
        buffer[0] = (char) unichar;
        buffer[1] = '\0';
        return 2;
    }
    else if (unichar <= 0x07FF) {
        length = 2;
        head = 0xc0;
    }
    /* Handle surrogates */
    else if (0xd800 <= unichar && unichar <= 0xdfff) {
        goto ill_formed_code_unit_subsequence;
    }
    else if (unichar <= 0xffff) {
        length = 3;
        head = 0xe0;
    }
    else if (unichar <= 0x10ffff) {
        length = 4;
        head = 0xf0;
    }
    else {
        goto ill_formed_code_unit_subsequence;
    }

    for (count = length - 1, shift = 0; count > 0; count--, shift += 6)
        buffer[count] = (char)(0x80 | ((unichar >> shift) & 0x3f));

    buffer[0] = (char)(head | ((unichar >> shift) & 0x3f));
    buffer[length] = '\0';

    return length + 1;

ill_formed_code_unit_subsequence:
    buffer[0] = '\0';
    return 0;
}

bool
is_valid_utf8(const char *ss, size_t len)
{
    size_t i = 0;
    size_t tail_bytes = 0;
    const uint8_t *s = (const uint8_t *) ss;

    /* This beauty is from:
     *  The Unicode Standard Version 6.2 - Core Specification, Table 3.7
     *  https://www.unicode.org/versions/Unicode6.2.0/ch03.pdf#G7404
     * We can optimize if needed. */
    while (i < len)
    {
        if (s[i] <= 0x7F) {
            tail_bytes = 0;
        }
        else if (s[i] >= 0xC2 && s[i] <= 0xDF) {
            tail_bytes = 1;
        }
        else if (s[i] == 0xE0) {
            i++;
            if (i >= len || !(s[i] >= 0xA0 && s[i] <= 0xBF))
                return false;
            tail_bytes = 1;
        }
        else if (s[i] >= 0xE1 && s[i] <= 0xEC) {
            tail_bytes = 2;
        }
        else if (s[i] == 0xED) {
            i++;
            if (i >= len || !(s[i] >= 0x80 && s[i] <= 0x9F))
                return false;
            tail_bytes = 1;
        }
        else if (s[i] >= 0xEE && s[i] <= 0xEF) {
            tail_bytes = 2;
        }
        else if (s[i] == 0xF0) {
            i++;
            if (i >= len || !(s[i] >= 0x90 && s[i] <= 0xBF))
                return false;
            tail_bytes = 2;
        }
        else if (s[i] >= 0xF1 && s[i] <= 0xF3) {
            tail_bytes = 3;
        }
        else if (s[i] == 0xF4) {
            i++;
            if (i >= len || !(s[i] >= 0x80 && s[i] <= 0x8F))
                return false;
            tail_bytes = 2;
        }
        else {
            return false;
        }

        i++;

        while (i < len && tail_bytes > 0 && s[i] >= 0x80 && s[i] <= 0xBF) {
            i++;
            tail_bytes--;
        }

        if (tail_bytes != 0)
            return false;
    }

    return true;
}
