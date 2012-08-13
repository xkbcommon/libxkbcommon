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

#ifndef XKBCOMP_KEYCODES_H
#define XKBCOMP_KEYCODES_H

static inline unsigned long
KeyNameToLong(const char name[XkbKeyNameLength])
{
    return
        (((unsigned long)name[0]) << 24) |
        (((unsigned long)name[1]) << 16) |
        (((unsigned long)name[2]) << 8)  |
        (((unsigned long)name[3]) << 0);
}

static inline void
LongToKeyName(unsigned long val, char name[XkbKeyNameLength])
{
    name[0] = ((val >> 24) & 0xff);
    name[1] = ((val >> 16) & 0xff);
    name[2] = ((val >> 8) & 0xff);
    name[3] = ((val >> 0) & 0xff);
}

static inline const char *
LongKeyNameText(unsigned long val)
{
    char buf[XkbKeyNameLength];
    LongToKeyName(val, buf);
    return KeyNameText(buf);
}

struct xkb_key *
FindNamedKey(struct xkb_keymap *keymap, unsigned long name,
             bool use_aliases, xkb_keycode_t start_from);

bool
FindKeyNameForAlias(struct xkb_keymap *keymap, unsigned long lname,
                    unsigned long *real_name);

#endif
