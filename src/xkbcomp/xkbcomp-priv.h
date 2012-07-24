/************************************************************
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ********************************************************/

#ifndef XKBCOMP_PRIV_H
#define XKBCOMP_PRIV_H

#include "xkbcomp.h"
#include "text.h"

extern bool
ProcessIncludeFile(struct xkb_context *ctx, IncludeStmt *stmt,
                   enum xkb_file_type file_type, XkbFile **file_rtrn,
                   enum merge_mode *merge_rtrn);

struct xkb_key *
FindNamedKey(struct xkb_keymap *keymap, unsigned long name,
             bool use_aliases, bool create, xkb_keycode_t start_from);

extern bool
FindKeyNameForAlias(struct xkb_keymap *keymap, unsigned long lname,
                    unsigned long *real_name);

extern bool
UpdateModifiersFromCompat(struct xkb_keymap *keymap);

xkb_mod_mask_t
VModsToReal(struct xkb_keymap *keymap, xkb_mod_mask_t vmodmask);

static inline unsigned long
KeyNameToLong(const char *name)
{
    return
        (((unsigned long)name[0]) << 24) |
        (((unsigned long)name[1]) << 16) |
        (((unsigned long)name[2]) << 8)  |
        (((unsigned long)name[3]) << 0);
}

static inline void
LongToKeyName(unsigned long val, char *name)
{
    name[0] = ((val >> 24) & 0xff);
    name[1] = ((val >> 16) & 0xff);
    name[2] = ((val >> 8) & 0xff);
    name[3] = ((val >> 0) & 0xff);
}

static inline bool
ReportNotArray(struct xkb_keymap *keymap, const char *type, const char *field,
               const char *name)
{
    log_err(keymap->ctx,
            "The %s %s field is not an array; "
            "Ignoring illegal assignment in %s\n",
            type, field, name);
    return false;
}

static inline bool
ReportShouldBeArray(struct xkb_keymap *keymap, const char *type,
                    const char *field, const char *name)
{
    log_err(keymap->ctx,
            "Missing subscript for %s %s; "
            "Ignoring illegal assignment in %s\n",
            type, field, name);
    return false;
}

static inline bool
ReportBadType(struct xkb_keymap *keymap, const char *type, const char *field,
              const char *name, const char *wanted)
{
    log_err(keymap->ctx,
            "The %s %s field must be a %s; "
            "Ignoring illegal assignment in %s\n",
            type, field, wanted, name);
    return false;
}

static inline bool
ReportBadField(struct xkb_keymap *keymap, const char *type, const char *field,
               const char *name)
{
    log_err(keymap->ctx,
            "Unknown %s field %s in %s; "
            "Ignoring assignment to unknown field in %s\n",
            type, field, name, name);
    return false;
}

#endif /* XKBCOMP_PRIV_H */
