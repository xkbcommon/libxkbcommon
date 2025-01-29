 /*
  * For MIT-open-group:
  * Copyright 1985, 1987, 1990, 1998  The Open Group
  *
  * For MIT:
  * Copyright © 2009 Dan Nicholson
  *
  * SPDX-License-Identifier: MIT-open-group AND MIT
  */

#ifndef KEYSYM_H
#define KEYSYM_H

#include <stdbool.h>
#include "xkbcommon/xkbcommon.h"

/*
 * NOTE: this is not defined in xkbcommon.h, because if we did, it may add
 * overhead for library user: when handling keysyms they would also need to
 * check min keysym when previously there was no reason to.
 */
/** Minimum keysym value */
#define XKB_KEYSYM_MIN            0x00000000
/** Minimum keysym value assigned */
#define XKB_KEYSYM_MIN_ASSIGNED   ((xkb_keysym_t)0x00000000)
/** Maximum keysym value assigned */
#define XKB_KEYSYM_MAX_ASSIGNED   0x1008ffb8
/** Minimum keysym value with explicit name */
#define XKB_KEYSYM_MIN_EXPLICIT   0x00000000
/** Maximum keysym value with explicit name */
#define XKB_KEYSYM_MAX_EXPLICIT   0x1008ffb8
/** Count of keysym value with explicit name */
#define XKB_KEYSYM_COUNT_EXPLICIT 2449
/** Offset to use when converting a Unicode code point to a keysym */
#define XKB_KEYSYM_UNICODE_OFFSET 0x01000000
/** Minimum Unicode keysym. NOTE: code points in 0..0xff cannot be converted. */
#define XKB_KEYSYM_UNICODE_MIN    0x01000100
/** Maximum Unicode keysym, corresponding to the maximum Unicode code point */
#define XKB_KEYSYM_UNICODE_MAX    0x0110ffff
/** Unicode version used for case mappings */
#define XKB_KEYSYM_UNICODE_VERSION { 16, 0, 0, 0 }
/** Maximum keysym canonical name length, plus terminating NULL byte */
#define XKB_KEYSYM_NAME_MAX_SIZE  28
/** Longest keysym canonical name */
#define XKB_KEYSYM_LONGEST_CANONICAL_NAME ISO_Discontinuous_Underline
/** Longest keysym name */
#define XKB_KEYSYM_LONGEST_NAME ISO_Discontinuous_Underline
/** Maximum bytes to encode the Unicode representation of a keysym in UTF-8:
 * 4 bytes + NULL-terminating byte */
#define XKB_KEYSYM_UTF8_MAX_SIZE  5

bool
xkb_keysym_is_assigned(xkb_keysym_t ks);

struct xkb_keysym_iterator;

struct xkb_keysym_iterator*
xkb_keysym_iterator_new(bool explicit);

struct xkb_keysym_iterator*
xkb_keysym_iterator_unref(struct xkb_keysym_iterator *iter);

bool
xkb_keysym_iterator_next(struct xkb_keysym_iterator *iter);

xkb_keysym_t
xkb_keysym_iterator_get_keysym(struct xkb_keysym_iterator *iter);

int
xkb_keysym_iterator_get_name(struct xkb_keysym_iterator *iter,
                             char *buffer, size_t size);

bool
xkb_keysym_iterator_is_explicitly_named(struct xkb_keysym_iterator *iter);

bool
xkb_keysym_is_deprecated(xkb_keysym_t keysym,
                         const char *name,
                         const char **reference_name);

#define XKB_MIN_VERBOSITY_DEPRECATED_KEYSYM 2
#define check_deprecated_keysyms(log_func, log_param, ctx, keysym, name, token, format, end) \
    if (unlikely((ctx)->log_verbosity >= XKB_MIN_VERBOSITY_DEPRECATED_KEYSYM)) {             \
        const char *ref_name = NULL;                                                         \
        if (xkb_keysym_is_deprecated(keysym, name, &ref_name)) {                             \
            if (ref_name == NULL)                                                            \
                log_func(log_param, XKB_WARNING_DEPRECATED_KEYSYM,                           \
                         "deprecated keysym \"" format "\"." end, token);                    \
            else                                                                             \
                log_func(log_param, XKB_WARNING_DEPRECATED_KEYSYM_NAME,                      \
                         "deprecated keysym name \"" format "\"; "                           \
                         "please use \"%s\" instead." end,                                   \
                         token, ref_name);                                                   \
        }                                                                                    \
    }

bool
xkb_keysym_is_lower(xkb_keysym_t keysym);

bool
xkb_keysym_is_upper_or_title(xkb_keysym_t keysym);

bool
xkb_keysym_is_keypad(xkb_keysym_t keysym);

bool
xkb_keysym_is_modifier(xkb_keysym_t keysym);

#endif
