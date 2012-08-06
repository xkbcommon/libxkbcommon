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
#include <syslog.h>
#include <X11/extensions/XKB.h>

#include "xkbcommon/xkbcommon.h"
#include "utils.h"
#include "darray.h"
#include "list.h"

typedef uint16_t xkb_level_index_t;

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
    int32_t group;
};

struct xkb_iso_action {
    uint8_t type;
    uint8_t flags;
    uint8_t mask;
    uint8_t real_mods;
    int32_t group;
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
    xkb_mod_mask_t mask;              /* effective mods */
    xkb_mod_mask_t vmods;
    uint8_t real_mods;
};

struct xkb_kt_map_entry {
    xkb_level_index_t level;
    struct xkb_mods mods;
    struct xkb_mods preserve;
};

struct xkb_key_type {
    struct xkb_mods mods;
    xkb_level_index_t num_levels;
    darray(struct xkb_kt_map_entry) map;
    const char *name;
    const char **level_names;
};

struct xkb_sym_interpret {
    xkb_keysym_t sym;
    unsigned char flags;
    unsigned char match;
    uint8_t mods;
    xkb_mod_index_t virtual_mod;
    union xkb_action act;
};

struct xkb_indicator_map {
    unsigned char flags;
    unsigned char which_groups;
    uint32_t groups;
    unsigned char which_mods;
    struct xkb_mods mods;
    unsigned int ctrls;
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

struct xkb_key {
    char name[XkbKeyNameLength];

    unsigned char explicit;

    unsigned char modmap;
    xkb_mod_mask_t vmodmap;

    bool repeats;

    /* Index into keymap->acts */
    size_t acts_index;

    unsigned char kt_index[XkbNumKbdGroups];

    xkb_group_index_t num_groups;
    /* How many levels the largest group has. */
    xkb_level_index_t width;

    uint8_t out_of_range_group_action;
    xkb_group_index_t out_of_range_group_number;

    /* per level/group index into 'syms' */
    int *sym_index;
    /* per level/group */
    unsigned int *num_syms;
    darray(xkb_keysym_t) syms;
};

/* Common keyboard description structure */
struct xkb_keymap {
    struct xkb_context *ctx;

    int refcnt;
    enum xkb_map_compile_flags flags;

    unsigned int enabled_ctrls;

    xkb_keycode_t min_key_code;
    xkb_keycode_t max_key_code;

    darray(struct xkb_key) keys;

    /* aliases in no particular order */
    darray(struct xkb_key_alias) key_aliases;

    darray(struct xkb_key_type) types;

    darray(struct xkb_sym_interpret) sym_interpret;

    /* vmod -> mod mapping */
    xkb_mod_index_t vmods[XkbNumVirtualMods];
    const char *vmod_names[XkbNumVirtualMods];

    struct xkb_mods groups[XkbNumKbdGroups];
    const char *group_names[XkbNumKbdGroups];

    darray(union xkb_action) acts;

    struct xkb_indicator_map indicators[XkbNumIndicators];
    const char *indicator_names[XkbNumIndicators];

    char *keycodes_section_name;
    char *symbols_section_name;
    char *types_section_name;
    char *compat_section_name;
};

static inline struct xkb_key *
XkbKey(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    return &darray_item(keymap->keys, kc);
}

static inline xkb_keycode_t
XkbKeyGetKeycode(struct xkb_keymap *keymap, struct xkb_key *key)
{
    /* Hack to avoid having to keep the keycode inside the xkb_key. */
    return (xkb_keycode_t)(key - keymap->keys.item);
}

#define xkb_foreach_key_from(iter, keymap, from) \
    darray_foreach_from(iter, keymap->keys, from)

#define xkb_foreach_key(iter, keymap) \
    xkb_foreach_key_from(iter, keymap, keymap->min_key_code)

static inline unsigned char
XkbKeyTypeIndex(struct xkb_key *key, xkb_group_index_t group)
{
    return key->kt_index[group & 0x3];
}

static inline struct xkb_key_type *
XkbKeyType(struct xkb_keymap *keymap, struct xkb_key *key,
           xkb_group_index_t group)
{
    return &darray_item(keymap->types, XkbKeyTypeIndex(key, group));
}

static inline xkb_level_index_t
XkbKeyGroupWidth(struct xkb_keymap *keymap, struct xkb_key *key,
                 xkb_group_index_t group)
{
    return XkbKeyType(keymap, key, group)->num_levels;
}

static inline unsigned int
XkbKeyNumSyms(struct xkb_key *key, xkb_group_index_t group,
              xkb_level_index_t level)
{
    return key->num_syms[group * key->width + level];
}

static inline xkb_keysym_t *
XkbKeySym(struct xkb_key *key, int ndx)
{
    return &darray_item(key->syms, ndx);
}

static inline int
XkbKeySymOffset(struct xkb_key *key, xkb_group_index_t group,
                xkb_level_index_t level)
{
    return key->sym_index[group * key->width + level];
}

static inline xkb_keysym_t *
XkbKeySymEntry(struct xkb_key *key, xkb_group_index_t group,
               xkb_level_index_t level)
{
    return XkbKeySym(key, XkbKeySymOffset(key, group, level));
}

static inline bool
XkbKeyHasActions(struct xkb_key *key)
{
    return key->acts_index != 0;
}

static inline unsigned int
XkbKeyNumActions(struct xkb_key *key)
{
    if (XkbKeyHasActions(key))
        return key->width * key->num_groups;
    return 1;
}

static inline union xkb_action *
XkbKeyActionsPtr(struct xkb_keymap *keymap, struct xkb_key *key)
{
    return darray_mem(keymap->acts, key->acts_index);
}

static inline union xkb_action *
XkbKeyActionEntry(struct xkb_keymap *keymap, struct xkb_key *key,
                  xkb_group_index_t group, xkb_level_index_t level)
{
    if (XkbKeyHasActions(key))
        return &XkbKeyActionsPtr(keymap, key)[key->width * group + level];
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

/**
 * If @string is dynamically allocated, free'd immediately after
 * being interned, and not used afterwards, use this function
 * instead of xkb_atom_intern to avoid some unnecessary allocations.
 * The caller should not use or free the passed in string afterwards.
 */
xkb_atom_t
xkb_atom_steal(struct xkb_context *ctx, char *string);

char *
xkb_atom_strdup(struct xkb_context *ctx, xkb_atom_t atom);

const char *
xkb_atom_text(struct xkb_context *ctx, xkb_atom_t atom);

xkb_group_index_t
xkb_key_get_group(struct xkb_state *state, xkb_keycode_t kc);

xkb_level_index_t
xkb_key_get_level(struct xkb_state *state, xkb_keycode_t kc,
                  xkb_group_index_t group);

extern int
xkb_key_get_syms_by_level(struct xkb_keymap *keymap, struct xkb_key *key,
                          xkb_group_index_t group, xkb_level_index_t level,
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

ATTR_PRINTF(3, 4) void
xkb_log(struct xkb_context *ctx, int priority, const char *fmt, ...);

#define xkb_log_cond(ctx, prio, ...) \
    do { \
        if (xkb_get_log_priority(ctx) >= (prio)) \
            xkb_log((ctx), (prio), __VA_ARGS__); \
    } while (0)

#define xkb_log_cond_lvl(ctx, prio, lvl, ...) \
    do { \
        if (xkb_get_log_verbosity(ctx) >= (lvl)) \
            xkb_log_cond((ctx), (prio), __VA_ARGS__); \
    } while (0)

/*
 * The format is not part of the argument list in order to avoid the
 * "ISO C99 requires rest arguments to be used" warning when only the
 * format is supplied without arguments. Not supplying it would still
 * result in an error, though.
 */
#define log_dbg(ctx, ...) xkb_log_cond((ctx), LOG_DEBUG, __VA_ARGS__)
#define log_info(ctx, ...) xkb_log_cond((ctx), LOG_INFO, __VA_ARGS__)
#define log_warn(ctx, ...) xkb_log_cond((ctx), LOG_WARNING, __VA_ARGS__)
#define log_err(ctx, ...) xkb_log_cond((ctx), LOG_ERR, __VA_ARGS__)
#define log_wsgo(ctx, ...) xkb_log_cond((ctx), LOG_CRIT, __VA_ARGS__)
#define log_lvl(ctx, lvl, ...) \
    xkb_log_cond_lvl((ctx), LOG_WARNING, (lvl), __VA_ARGS__)

#endif /* XKB_PRIV_H */
