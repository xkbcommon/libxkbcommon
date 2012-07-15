/*
 * Copyright 1985, 1987, 1990, 1998  The Open Group
 * Copyright 2008  Dan Nicholson
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

/************************************************************
 * Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.
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

/*
 * Copyright © 2012 Intel Corporation
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
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifndef XKB_PRIV_H
#define XKB_PRIV_H

#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <X11/extensions/XKB.h>
#include <X11/X.h>

#include "xkbcommon/xkbcommon.h"
#include "utils.h"
#include "darray.h"

enum xkb_file_type {
    /* The top level file which includes the other component files. */
    FILE_TYPE_KEYMAP = (1 << 0),

    /* Component files. */
    FILE_TYPE_TYPES = (1 << 1),
    FILE_TYPE_COMPAT = (1 << 2),
    FILE_TYPE_SYMBOLS = (1 << 3),
    FILE_TYPE_KEYCODES = (1 << 4),
    FILE_TYPE_GEOMETRY = (1 << 5),

    /* This one doesn't mix with the others, but useful here as well. */
    FILE_TYPE_RULES = (1 << 6),
};

/* Files needed for a complete keymap. */
#define REQUIRED_FILE_TYPES (FILE_TYPE_TYPES | FILE_TYPE_COMPAT | \
                             FILE_TYPE_SYMBOLS | FILE_TYPE_KEYCODES)
#define LEGAL_FILE_TYPES    REQUIRED_FILE_TYPES

/**
 * Legacy names for the components of an XKB keymap, also known as KcCGST.
 */
struct xkb_component_names {
    char *keycodes;
    char *types;
    char *compat;
    char *symbols;
};

struct xkb_any_action {
    uint8_t type;
    uint8_t data[7];
};

struct xkb_mod_action {
    uint8_t type;
    uint8_t flags;
    uint8_t mask;
    uint8_t real_mods;
    uint16_t vmods;
};

struct xkb_group_action {
    uint8_t type;
    uint8_t flags;
    int16_t group;
};

struct xkb_iso_action {
    uint8_t type;
    uint8_t flags;
    uint8_t mask;
    uint8_t real_mods;
    uint8_t group;
    uint8_t affect;
    uint16_t vmods;
};

struct xkb_controls_action {
    uint8_t type;
    uint8_t flags;
    uint32_t ctrls;
};

struct xkb_device_button_action {
    uint8_t type;
    uint8_t flags;
    uint8_t count;
    uint8_t button;
    uint8_t device;
};

struct xkb_device_valuator_action {
    uint8_t type;
    uint8_t device;
    uint8_t v1_what;
    uint8_t v1_index;
    int8_t v1_value;
    uint8_t v2_what;
    uint8_t v2_index;
    int8_t v2_value;
};

struct xkb_pointer_default_action {
    uint8_t type;
    uint8_t flags;
    uint8_t affect;
    int8_t value;
};

struct xkb_switch_screen_action {
    uint8_t type;
    uint8_t flags;
    int8_t screen;
};

struct xkb_redirect_key_action {
    uint8_t type;
    xkb_keycode_t new_kc;
    uint8_t mods_mask;
    uint8_t mods;
    uint16_t vmods_mask;
    uint16_t vmods;
};

struct xkb_pointer_action {
    uint8_t type;
    uint8_t flags;
    int16_t x;
    int16_t y;
};

struct xkb_message_action {
    uint8_t type;
    uint8_t flags;
    uint8_t message[6];
};

struct xkb_pointer_button_action {
    uint8_t type;
    uint8_t flags;
    uint8_t count;
    int8_t button;
};

union xkb_action {
    struct xkb_any_action any;
    struct xkb_mod_action mods;
    struct xkb_group_action group;
    struct xkb_iso_action iso;
    struct xkb_controls_action ctrls;
    struct xkb_device_button_action devbtn;
    struct xkb_device_valuator_action devval;
    struct xkb_pointer_default_action dflt;
    struct xkb_switch_screen_action screen;
    struct xkb_redirect_key_action redirect;    /* XXX wholly unnecessary? */
    struct xkb_pointer_action ptr;         /* XXX delete for DeviceValuator */
    struct xkb_pointer_button_action btn;  /* XXX delete for DeviceBtn */
    struct xkb_message_action msg;         /* XXX just delete */
    unsigned char type;
};

struct xkb_mods {
    uint32_t mask;              /* effective mods */
    uint32_t vmods;
    uint8_t real_mods;
};

struct xkb_kt_map_entry {
    uint16_t level;
    struct xkb_mods mods;
};

struct xkb_key_type {
    struct xkb_mods mods;
    uint16_t num_levels;
    darray(struct xkb_kt_map_entry) map;
    struct xkb_mods *             preserve;
    char *name;
    char **level_names;
};

struct xkb_sym_interpret {
    xkb_keysym_t sym;
    unsigned char flags;
    unsigned char match;
    uint8_t mods;
    uint32_t virtual_mod;
    union xkb_action act;
};

struct xkb_sym_map {
    unsigned char kt_index[XkbNumKbdGroups];
    unsigned char group_info;
    unsigned char width;
    int              *sym_index;     /* per level/group index into 'syms' */
    unsigned int     *num_syms;     /* per level/group */
    darray(xkb_keysym_t) syms;
};

struct xkb_behavior {
    unsigned char type;
    unsigned char data;
};

struct xkb_indicator_map {
    unsigned char flags;
    unsigned char which_groups;
    unsigned char groups;
    unsigned char which_mods;
    struct xkb_mods mods;
    unsigned int ctrls;
};

struct xkb_key_name {
    char name[XkbKeyNameLength];
};

struct xkb_key_alias {
    char real[XkbKeyNameLength];
    char alias[XkbKeyNameLength];
};

struct xkb_controls {
    unsigned char groups_wrap;
    struct xkb_mods internal;
    struct xkb_mods ignore_lock;
    unsigned short repeat_delay;
    unsigned short repeat_interval;
    unsigned short slow_keys_delay;
    unsigned short debounce_delay;
    unsigned short ax_options;
    unsigned short ax_timeout;
    unsigned short axt_opts_mask;
    unsigned short axt_opts_values;
    unsigned int axt_ctrls_mask;
    unsigned int axt_ctrls_values;
};

/* Common keyboard description structure */
struct xkb_keymap {
    struct xkb_context *ctx;

    int refcnt;
    enum xkb_map_compile_flags flags;

    unsigned int enabled_ctrls;

    xkb_keycode_t min_key_code;
    xkb_keycode_t max_key_code;

    /* key -> key name mapping */
    darray(struct xkb_key_name) key_names;
    /* aliases in no particular order */
    darray(struct xkb_key_alias) key_aliases;

    /* key -> explicit flags mapping */
    unsigned char *explicit;

    darray(struct xkb_key_type) types;

    /* key -> symbols mapping */
    darray(struct xkb_sym_map) key_sym_map;

    darray(struct xkb_sym_interpret) sym_interpret;

    /* key -> mod mapping */
    unsigned char *modmap;
    /* key -> vmod mapping */
    uint32_t *vmodmap;
    /* vmod -> mod mapping */
    uint32_t vmods[XkbNumVirtualMods];
    char *vmod_names[XkbNumVirtualMods];

    struct xkb_mods groups[XkbNumKbdGroups];
    char *group_names[XkbNumKbdGroups];

    /* key -> actions mapping: acts[key_acts[keycode]] */
    darray(union xkb_action) acts;
    darray(size_t) key_acts;

    /* key -> behavior mapping */
    struct xkb_behavior *behaviors;

    /* key -> should repeat mapping - one bit per key */
    unsigned char *per_key_repeat;

    struct xkb_indicator_map indicators[XkbNumIndicators];
    char *indicator_names[XkbNumIndicators];

    char *keycodes_section_name;
    char *symbols_section_name;
    char *types_section_name;
    char *compat_section_name;
};

static inline unsigned char
XkbNumGroups(unsigned char group_info)
{
    return group_info & 0x0f;
}

static inline unsigned char
XkbOutOfRangeGroupInfo(unsigned char group_info)
{
    return group_info & 0xf0;
}

static inline unsigned char
XkbOutOfRangeGroupAction(unsigned char group_info)
{
    return group_info & 0xc0;
}

static inline unsigned char
XkbOutOfRangeGroupNumber(unsigned char group_info)
{
    return (group_info & 0x30) >> 4;
}

static inline unsigned char
XkbSetGroupInfo(unsigned char group_info,
                unsigned char out_of_range_group_action,
                unsigned char out_of_range_group_number)
{
    return
        (out_of_range_group_action & 0xc0) |
        ((out_of_range_group_number & 3) << 4) |
        (XkbNumGroups(group_info));
}

static inline unsigned char
XkbSetNumGroups(unsigned char group_info, unsigned char num_groups)
{
    return (group_info & 0xf0) | (num_groups & 0x0f);
}

static inline unsigned char
XkbKeyGroupInfo(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    return darray_item(keymap->key_sym_map, kc).group_info;
}

static inline unsigned char
XkbKeyNumGroups(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    return XkbNumGroups(XkbKeyGroupInfo(keymap, kc));
}

static inline unsigned char
XkbKeyTypeIndex(struct xkb_keymap *keymap, xkb_keycode_t kc,
                unsigned int group)
{
    return darray_item(keymap->key_sym_map, kc).kt_index[group & 0x3];
}

static inline struct xkb_key_type *
XkbKeyType(struct xkb_keymap *keymap, xkb_keycode_t kc, unsigned int group)
{
    return &darray_item(keymap->types, XkbKeyTypeIndex(keymap, kc, group));
}

static inline uint16_t
XkbKeyGroupWidth(struct xkb_keymap *keymap, xkb_keycode_t kc,
                 unsigned int group)
{
    return XkbKeyType(keymap, kc, group)->num_levels;
}

static inline unsigned char
XkbKeyGroupsWidth(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    return darray_item(keymap->key_sym_map, kc).width;
}

static inline unsigned int
XkbKeyNumSyms(struct xkb_keymap *keymap, xkb_keycode_t kc,
              unsigned int group, unsigned int level)
{
    unsigned char width = XkbKeyGroupsWidth(keymap, kc);
    return darray_item(keymap->key_sym_map,
                       kc).num_syms[group * width + level];
}

static inline xkb_keysym_t *
XkbKeySym(struct xkb_keymap *keymap, xkb_keycode_t kc, int ndx)
{
    return &darray_item(darray_item(keymap->key_sym_map, kc).syms, ndx);
}

static inline int
XkbKeySymOffset(struct xkb_keymap *keymap, xkb_keycode_t kc,
                unsigned group, unsigned int level)
{
    unsigned char width = XkbKeyGroupsWidth(keymap, kc);
    return darray_item(keymap->key_sym_map,
                       kc).sym_index[group * width + level];
}

static inline xkb_keysym_t *
XkbKeySymEntry(struct xkb_keymap *keymap, xkb_keycode_t kc,
               unsigned group, unsigned int level)
{
    return XkbKeySym(keymap, kc, XkbKeySymOffset(keymap, kc, group, level));
}

static inline bool
XkbKeyHasActions(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    return darray_item(keymap->key_acts, kc) != 0;
}

static inline unsigned char
XkbKeyNumActions(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    if (XkbKeyHasActions(keymap, kc))
        return XkbKeyGroupsWidth(keymap, kc) * XkbKeyNumGroups(keymap, kc);
    return 1;
}

static inline union xkb_action *
XkbKeyActionsPtr(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    return darray_mem(keymap->acts, darray_item(keymap->key_acts, kc));
}

static inline union xkb_action *
XkbKeyActionEntry(struct xkb_keymap *keymap, xkb_keycode_t kc,
                  unsigned int group, unsigned int level)
{
    unsigned char width = XkbKeyGroupsWidth(keymap, kc);
    if (XkbKeyHasActions(keymap, kc))
        return &XkbKeyActionsPtr(keymap, kc)[width * group + level];
    return NULL;
}

static inline bool
XkbKeycodeInRange(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    return kc >= keymap->min_key_code && kc <= keymap->max_key_code;
}

typedef uint32_t xkb_atom_t;

#define XKB_ATOM_NONE 0

xkb_atom_t
xkb_atom_intern(struct xkb_context *ctx, const char *string);

char *
xkb_atom_strdup(struct xkb_context *ctx, xkb_atom_t atom);

const char *
xkb_atom_text(struct xkb_context *ctx, xkb_atom_t atom);

extern unsigned int
xkb_key_get_group(struct xkb_state *state, xkb_keycode_t kc);

extern unsigned int
xkb_key_get_level(struct xkb_state *state, xkb_keycode_t kc,
                  unsigned int group);

extern int
xkb_key_get_syms_by_level(struct xkb_keymap *keymap, xkb_keycode_t kc,
                          unsigned int group, unsigned int level,
                          const xkb_keysym_t **syms_out);

/**
 * Deprecated entrypoint for legacy users who need to be able to compile
 * XKB keymaps by KcCGST (Keycodes + Compat + Geometry + Symbols + Types)
 * names.
 *
 * You should not use this unless you are the X server.  This entrypoint
 * may well disappear in future releases.  Please, please, don't use it.
 *
 * Geometry will be ignored since xkbcommon does not support it in any way.
 */
struct xkb_keymap *
xkb_map_new_from_kccgst(struct xkb_context *ctx,
                        const struct xkb_component_names *kccgst,
                        enum xkb_map_compile_flags flags);

extern unsigned
xkb_context_take_file_id(struct xkb_context *ctx);

bool
xkb_keysym_is_lower(xkb_keysym_t keysym);

bool
xkb_keysym_is_upper(xkb_keysym_t keysym);

bool
xkb_keysym_is_keypad(xkb_keysym_t keysym);

#endif /* XKB_PRIV_H */
