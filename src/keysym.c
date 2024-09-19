/*
 * Copyright 1985, 1987, 1990, 1998  The Open Group
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors or their
 * institutions shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the authors.
 */

/*
 * Copyright © 2009 Dan Nicholson
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

#include <stdlib.h>
#include "xkbcommon/xkbcommon.h"
#include "utils.h"
#include "keysym.h"
#include "ks_tables.h"

static ssize_t
find_keysym_index(xkb_keysym_t ks)
{
    /* Lower bound:
     * keysym_to_name[0].keysym
     * == XKB_KEYSYM_MIN_EXPLICIT == XKB_KEYSYM_MIN == 0
     * No need to check: xkb_keysym_t is unsigned.
     *
     * Upper bound:
     * keysym_to_name[ARRAY_SIZE(keysym_to_name) - 1].keysym
     * == XKB_KEYSYM_MAX_EXPLICIT <= XKB_KEYSYM_MAX
     */
    if (ks > XKB_KEYSYM_MAX_EXPLICIT)
        return -1;

    ssize_t lo = 0, hi = ARRAY_SIZE(keysym_to_name) - 1;
    while (hi >= lo) {
        ssize_t mid = (lo + hi) / 2;
        if (ks > keysym_to_name[mid].keysym) {
            lo = mid + 1;
        } else if (ks < keysym_to_name[mid].keysym) {
            hi = mid - 1;
        } else {
            return mid;
        }
    }

    return -1;
}

#define get_name_by_index(index) keysym_names + index

static inline const char *
get_name(const struct name_keysym *entry)
{
    return get_name_by_index(entry->offset);
}

/* Unnamed Unicode codepoint. */
static inline int
get_unicode_name(xkb_keysym_t ks, char *buffer, size_t size)
{
    const int width = (ks & 0xff0000UL) ? 8 : 4;
    return snprintf(buffer, size, "U%0*lX", width, ks & 0xffffffUL);
}

XKB_EXPORT int
xkb_keysym_get_name(xkb_keysym_t ks, char *buffer, size_t size)
{
    if (ks > XKB_KEYSYM_MAX) {
        snprintf(buffer, size, "Invalid");
        return -1;
    }

    ssize_t index = find_keysym_index(ks);
    if (index != -1)
        return snprintf(buffer, size, "%s", get_name(&keysym_to_name[index]));

    /* Unnamed Unicode codepoint. */
    if (ks >= XKB_KEYSYM_UNICODE_MIN && ks <= XKB_KEYSYM_UNICODE_MAX)
        return get_unicode_name(ks, buffer, size);

    /* Unnamed, non-Unicode, symbol (shouldn't generally happen). */
    return snprintf(buffer, size, "0x%08x", ks);
}

bool
xkb_keysym_is_assigned(xkb_keysym_t ks)
{
    return (XKB_KEYSYM_UNICODE_MIN <= ks && ks <= XKB_KEYSYM_UNICODE_MAX) ||
           find_keysym_index(ks) != -1;
}

struct xkb_keysym_iterator {
    bool explicit;       /* If true, traverse only explicitly named keysyms */
    int32_t index;       /* Current index in `keysym_to_name` */
    xkb_keysym_t keysym; /* Current keysym */
};

struct xkb_keysym_iterator*
xkb_keysym_iterator_new(bool iterate_only_explicit_keysyms)
{
    struct xkb_keysym_iterator* iter = calloc(1, sizeof(*iter));
    iter->explicit = iterate_only_explicit_keysyms;
    iter->index = -1;
    iter->keysym = XKB_KEYSYM_UNICODE_MAX;
    return iter;
}

struct xkb_keysym_iterator*
xkb_keysym_iterator_unref(struct xkb_keysym_iterator *iter)
{
    free(iter);
    return NULL;
}

xkb_keysym_t
xkb_keysym_iterator_get_keysym(struct xkb_keysym_iterator *iter)
{
    return iter->keysym;
}

bool
xkb_keysym_iterator_is_explicitly_named(struct xkb_keysym_iterator *iter)
{
    return iter->index >= 0 &&
           iter->index < (int32_t)ARRAY_SIZE(keysym_to_name) &&
           (iter->explicit ||
            iter->keysym == keysym_to_name[iter->index].keysym);
}

int
xkb_keysym_iterator_get_name(struct xkb_keysym_iterator *iter,
                             char *buffer, size_t size)
{
    if (iter->index < 0 || iter->index >= (int32_t)ARRAY_SIZE(keysym_to_name))
        return -1;
    if (iter->explicit || iter->keysym == keysym_to_name[iter->index].keysym)
        return snprintf(buffer, size, "%s",
                        get_name(&keysym_to_name[iter->index]));
    return get_unicode_name(iter->keysym, buffer, size);
}

/* Iterate over the *assigned* keysyms.
 *
 * Use:
 *
 * ```c
 * struct xkb_keysym_iterator *iter = xkb_keysym_iterator_new(true);
 * while (xkb_keysym_iterator_next(iter)) {
 *   ...
 * }
 * iter = xkb_keysym_iterator_unref(iter);
 * ```
 */
bool
xkb_keysym_iterator_next(struct xkb_keysym_iterator *iter)
{
    if (iter->index >= (int32_t)ARRAY_SIZE(keysym_to_name) - 1)
        return false;

    /* Next keysym */
    if (iter->explicit || iter->keysym >= XKB_KEYSYM_UNICODE_MAX ||
        keysym_to_name[iter->index + 1].keysym < XKB_KEYSYM_UNICODE_MIN) {
        /* Explicitly named keysyms only */
        iter->keysym = keysym_to_name[++iter->index].keysym;
        assert(iter->explicit ||
               iter->keysym <= XKB_KEYSYM_UNICODE_MIN ||
               iter->keysym >= XKB_KEYSYM_UNICODE_MAX);
    } else {
        /* Unicode keysyms
         * NOTE: Unicode keysyms are within keysym_to_name keysyms range. */
        if (iter->keysym >= keysym_to_name[iter->index].keysym)
            iter->index++;
        if (iter->keysym >= XKB_KEYSYM_UNICODE_MIN) {
            /* Continue Unicode keysyms */
            iter->keysym++;
        } else {
            /* Start Unicode keysyms */
            iter->keysym = XKB_KEYSYM_UNICODE_MIN;
        }
    }
    return true;
}

/*
 * Parse the numeric part of a 0xXXXX and UXXXX keysym.
 * Not using strtoul -- it's slower and accepts a bunch of stuff
 * we don't want to allow, like signs, spaces, even locale stuff.
 */
static bool
parse_keysym_hex(const char *s, uint32_t *out)
{
    uint32_t result = 0;
    int i;
    for (i = 0; i < 8 && s[i] != '\0'; i++) {
        result <<= 4;
        if ('0' <= s[i] && s[i] <= '9')
            result += s[i] - '0';
        else if ('a' <= s[i] && s[i] <= 'f')
            result += 10 + s[i] - 'a';
        else if ('A' <= s[i] && s[i] <= 'F')
            result += 10 + s[i] - 'A';
        else
            return false;
    }
    *out = result;
    return s[i] == '\0' && i > 0;
}

XKB_EXPORT xkb_keysym_t
xkb_keysym_from_name(const char *name, enum xkb_keysym_flags flags)
{
    const struct name_keysym *entry = NULL;
    char *tmp;
    uint32_t val;
    bool icase = (flags & XKB_KEYSYM_CASE_INSENSITIVE);

    if (flags & ~XKB_KEYSYM_CASE_INSENSITIVE)
        return XKB_KEY_NoSymbol;

    /*
     * We need to !icase case to be fast, for e.g. Compose file parsing.
     * So do it in a fast path.
     */
    if (!icase) {
        size_t pos = keysym_name_perfect_hash(name);
        if (pos < ARRAY_SIZE(name_to_keysym)) {
            const char *s = get_name(&name_to_keysym[pos]);
            if (strcmp(name, s) == 0)
                return name_to_keysym[pos].keysym;
        }
    }
    /*
    * Find the correct keysym for case-insensitive match.
    *
    * The name_to_keysym table is sorted by istrcmp(). So the binary
    * search may return _any_ of all possible case-insensitive duplicates. The
    * duplicates are sorted so that the "best" case-insensitive match comes
    * last. So the code searches the entry and all next entries that match by
    * case-insensitive comparison and returns the "best" case-insensitive match.
    *
    * The "best" case-insensitive match is the lower-case keysym name. Most
    * keysyms names that only differ by letter-case are keysyms that are
    * available as “small” and “big” variants. For example:
    *
    * - Bicameral scripts: Lower-case and upper-case variants,
    *   e.g. KEY_a and KEY_A.
    * - Non-bicameral scripts: e.g. KEY_kana_a and KEY_kana_A.
    *
    * There are some exceptions, e.g. `XF86Screensaver` and `XF86ScreenSaver`.
    */
    else {
        int32_t lo = 0, hi = ARRAY_SIZE(name_to_keysym) - 1;
        while (hi >= lo) {
            int32_t mid = (lo + hi) / 2;
            int cmp = istrcmp(name, get_name(&name_to_keysym[mid]));
            if (cmp > 0) {
                lo = mid + 1;
            } else if (cmp < 0) {
                hi = mid - 1;
            } else {
                entry = &name_to_keysym[mid];
                break;
            }
        }
        if (entry) {
            const struct name_keysym *last;
            last = name_to_keysym + ARRAY_SIZE(name_to_keysym) - 1;
            /* Keep going until we reach end of array
             * or non case-insensitve match */
            while (entry < last &&
                   istrcmp(get_name(entry + 1), get_name(entry)) == 0) {
                entry++;
            }
            return entry->keysym;
        }
    }

    if (*name == 'U' || (icase && *name == 'u')) {
        if (!parse_keysym_hex(&name[1], &val))
            return XKB_KEY_NoSymbol;

        if (val < 0x20 || (val > 0x7e && val < 0xa0))
            return XKB_KEY_NoSymbol;
        if (val < 0x100)
            return (xkb_keysym_t) val;
        if (val > 0x10ffff)
            return XKB_KEY_NoSymbol;
        return (xkb_keysym_t) val | XKB_KEYSYM_UNICODE_OFFSET;
    }
    else if (name[0] == '0' && (name[1] == 'x' || (icase && name[1] == 'X'))) {
        if (!parse_keysym_hex(&name[2], &val) || val > XKB_KEYSYM_MAX)
            return XKB_KEY_NoSymbol;
        return (xkb_keysym_t) val;
    }

    /* Stupid inconsistency between the headers and XKeysymDB: the former has
     * no separating underscore, while some XF86* syms in the latter did.
     * As a last ditch effort, try without. */
    if (strncmp(name, "XF86_", 5) == 0 ||
        (icase && istrncmp(name, "XF86_", 5) == 0)) {
        xkb_keysym_t ret;
        tmp = strdup(name);
        if (!tmp)
            return XKB_KEY_NoSymbol;
        memmove(&tmp[4], &tmp[5], strlen(name) - 5 + 1);
        ret = xkb_keysym_from_name(tmp, flags);
        free(tmp);
        return ret;
    }

    return XKB_KEY_NoSymbol;
}

/*
 * Check whether a keysym with code "keysym" and name "name" is deprecated.
 * • If the keysym is not deprecated itself and has no deprecated names,
 *   then return false and write NULL in "reference_name".
 * • If there is a non-deprecated name for the given keysym, then write this
 *   name in "reference_name", else write NULL and return true.
 * • If "name" is NULL, then returns false: the keysym itself is not deprecated.
 * • If "name" is not NULL, then check if "name" is deprecated.
 *
 * WARNING: this function is unsafe because it does not test if "name" is
 * actually a correct name for "keysym". It is intended to be used just after
 * keysym resolution.
 */
bool
xkb_keysym_is_deprecated(xkb_keysym_t keysym,
                         const char *name,
                         const char **reference_name)
{
    if (keysym > XKB_KEYSYM_MAX) {
        /* Invalid keysym */
        *reference_name = NULL;
        return false;
    }
    /* [WARNING] We do not check that name, if defined, is a valid for keysym */

    int32_t lo = 0, hi = ARRAY_SIZE(deprecated_keysyms) - 1;
    while (hi >= lo) {
        int32_t mid = (lo + hi) / 2;
        if (keysym > deprecated_keysyms[mid].keysym) {
            lo = mid + 1;
        } else if (keysym < deprecated_keysyms[mid].keysym) {
            hi = mid - 1;
        } else {
            /* Keysym have some deprecated names */
            if (deprecated_keysyms[mid].offset == DEPRECATED_KEYSYM) {
                /* All names are deprecated */
                *reference_name = NULL;
                return true;
            }
            /* There is a reference name that is not deprecated */
            *reference_name = get_name_by_index(deprecated_keysyms[mid].offset);
            if (name == NULL)
                /* No name to check: indicate not deprecated */
                return false;
            if (deprecated_keysyms[mid].explicit_count) {
                /* Only some explicit names are deprecated */
                uint8_t k = deprecated_keysyms[mid].explicit_index;
                const uint8_t k_max = deprecated_keysyms[mid].explicit_index
                                    + deprecated_keysyms[mid].explicit_count;
                /* Check every deprecated alias */
                for (; k < k_max; k++) {
                    const char *alias =
                        get_name_by_index(explicit_deprecated_aliases[k]);
                    if (strcmp(name, alias) == 0)
                        return true;
                }
                return false;
            } else {
                /* All names but the reference one are deprecated */
                return strcmp(name, *reference_name) != 0;
            }
        }
    }

    /* Keysym has no deprecated names */
    *reference_name = NULL;
    return false;
}

bool
xkb_keysym_is_keypad(xkb_keysym_t keysym)
{
    return keysym >= XKB_KEY_KP_Space && keysym <= XKB_KEY_KP_Equal;
}


bool
xkb_keysym_is_modifier(xkb_keysym_t keysym)
{
    return
        (keysym >= XKB_KEY_Shift_L && keysym <= XKB_KEY_Hyper_R) ||
        (keysym >= XKB_KEY_ISO_Lock && keysym <= XKB_KEY_ISO_Level5_Lock) ||
        keysym == XKB_KEY_Mode_switch ||
        keysym == XKB_KEY_Num_Lock;
}
