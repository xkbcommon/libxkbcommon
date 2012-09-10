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

#include "xkbcommon/xkbcommon.h"
#include "utils.h"
#include "darray.h"
#include "list.h"

typedef uint32_t xkb_level_index_t;
typedef uint32_t xkb_atom_t;

#define XKB_ATOM_NONE 0
#define XKB_LEVEL_INVALID 0xffffffff

#define XKB_KEY_NAME_LENGTH 4

/* These should all be dynamic. */
#define XKB_NUM_GROUPS 4
#define XKB_NUM_INDICATORS 32
#define XKB_NUM_VIRTUAL_MODS 16
#define XKB_NUM_CORE_MODS 8

struct xkb_context {
    int refcnt;

    ATTR_PRINTF(3, 0) void (*log_fn)(struct xkb_context *ctx,
                                     enum xkb_log_level level,
                                     const char *fmt, va_list args);
    enum xkb_log_level log_level;
    int log_verbosity;
    void *user_data;

    darray(char *) includes;
    darray(char *) failed_includes;

    /* xkbcomp needs to assign sequential IDs to XkbFile's it creates. */
    unsigned file_id;

    struct atom_table *atom_table;
};

/**
 * Legacy names for the components of an XKB keymap, also known as KcCGST.
 */
struct xkb_component_names {
    char *keycodes;
    char *types;
    char *compat;
    char *symbols;
};

enum xkb_action_type {
    ACTION_TYPE_NONE = 0,
    ACTION_TYPE_MOD_SET,
    ACTION_TYPE_MOD_LATCH,
    ACTION_TYPE_MOD_LOCK,
    ACTION_TYPE_GROUP_SET,
    ACTION_TYPE_GROUP_LATCH,
    ACTION_TYPE_GROUP_LOCK,
    ACTION_TYPE_PTR_MOVE,
    ACTION_TYPE_PTR_BUTTON,
    ACTION_TYPE_PTR_LOCK,
    ACTION_TYPE_PTR_DEFAULT,
    ACTION_TYPE_TERMINATE,
    ACTION_TYPE_SWITCH_VT,
    ACTION_TYPE_CTRL_SET,
    ACTION_TYPE_CTRL_LOCK,
    ACTION_TYPE_KEY_REDIRECT,
    ACTION_TYPE_PRIVATE,
    ACTION_TYPE_LAST
};

enum xkb_action_flags {
    ACTION_LOCK_CLEAR = (1 << 0),
    ACTION_LATCH_TO_LOCK = (1 << 1),
    ACTION_LOCK_NO_LOCK = (1 << 2),
    ACTION_LOCK_NO_UNLOCK = (1 << 3),
    ACTION_MODS_LOOKUP_MODMAP = (1 << 4),
    ACTION_ABSOLUTE_SWITCH = (1 << 5),
    ACTION_ABSOLUTE_X = (1 << 6),
    ACTION_ABSOLUTE_Y = (1 << 7),
    ACTION_NO_ACCEL = (1 << 8),
    ACTION_SAME_SCREEN = (1 << 9),
};

struct xkb_mods {
    xkb_mod_mask_t mods;       /* original real+virtual mods in definition */
    xkb_mod_mask_t mask;       /* computed effective mask */
};

struct xkb_mod_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    struct xkb_mods mods;
};

struct xkb_group_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    int32_t group;
};

struct xkb_controls_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    uint32_t ctrls;
};

struct xkb_pointer_default_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    uint8_t affect;
    int8_t value;
};

struct xkb_switch_screen_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    int8_t screen;
};

struct xkb_redirect_key_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    xkb_keycode_t new_kc;
    uint8_t mods_mask;
    uint8_t mods;
    uint16_t vmods_mask;
    uint16_t vmods;
};

struct xkb_pointer_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    int16_t x;
    int16_t y;
};

struct xkb_pointer_button_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    uint8_t count;
    int8_t button;
};

struct xkb_private_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    uint8_t data[7];
};

union xkb_action {
    enum xkb_action_type type;
    struct xkb_mod_action mods;
    struct xkb_group_action group;
    struct xkb_controls_action ctrls;
    struct xkb_pointer_default_action dflt;
    struct xkb_switch_screen_action screen;
    struct xkb_redirect_key_action redirect;    /* XXX wholly unnecessary? */
    struct xkb_pointer_action ptr;
    struct xkb_pointer_button_action btn;
    struct xkb_private_action priv;
};

struct xkb_kt_map_entry {
    xkb_level_index_t level;
    struct xkb_mods mods;
    struct xkb_mods preserve;
};

struct xkb_key_type {
    struct xkb_mods mods;
    xkb_level_index_t num_levels;
    struct xkb_kt_map_entry *map;
    unsigned int num_entries;
    xkb_atom_t name;
    xkb_atom_t *level_names;
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
    xkb_atom_t name;
    enum xkb_state_component which_groups;
    uint32_t groups;
    enum xkb_state_component which_mods;
    struct xkb_mods mods;
    unsigned int ctrls;
};

struct xkb_key_alias {
    char real[XKB_KEY_NAME_LENGTH];
    char alias[XKB_KEY_NAME_LENGTH];
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
    char name[XKB_KEY_NAME_LENGTH];

    unsigned char explicit;

    unsigned char modmap;
    xkb_mod_mask_t vmodmap;

    bool repeats;

    union xkb_action *actions;

    unsigned kt_index[XKB_NUM_GROUPS];

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

    struct xkb_key_type *types;
    unsigned int num_types;

    darray(struct xkb_sym_interpret) sym_interpret;

    /* vmod -> mod mapping */
    xkb_mod_mask_t vmods[XKB_NUM_VIRTUAL_MODS];
    xkb_atom_t vmod_names[XKB_NUM_VIRTUAL_MODS];

    /* Number of groups in the key with the most groups. */
    xkb_group_index_t num_groups;
    xkb_atom_t group_names[XKB_NUM_GROUPS];

    struct xkb_indicator_map indicators[XKB_NUM_INDICATORS];

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

static inline struct xkb_key_type *
XkbKeyType(struct xkb_keymap *keymap, struct xkb_key *key,
           xkb_group_index_t group)
{
    return &keymap->types[key->kt_index[group]];
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
XkbKeySymEntry(struct xkb_key *key, xkb_group_index_t group,
               xkb_level_index_t level)
{
    return &darray_item(key->syms,
                        key->sym_index[group * key->width + level]);
}

static inline union xkb_action *
XkbKeyActionEntry(struct xkb_key *key, xkb_group_index_t group,
                  xkb_level_index_t level)
{
    return &key->actions[key->width * group + level];
}

static inline bool
XkbKeycodeInRange(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    return kc >= keymap->min_key_code && kc <= keymap->max_key_code;
}

struct xkb_keymap *
xkb_map_new(struct xkb_context *ctx);

/*
 * Returns XKB_ATOM_NONE if @string was not previously interned,
 * otherwise returns the atom.
 */
xkb_atom_t
xkb_atom_lookup(struct xkb_context *ctx, const char *string);

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

extern unsigned
xkb_context_take_file_id(struct xkb_context *ctx);

bool
xkb_keysym_is_lower(xkb_keysym_t keysym);

bool
xkb_keysym_is_upper(xkb_keysym_t keysym);

bool
xkb_keysym_is_keypad(xkb_keysym_t keysym);

ATTR_PRINTF(3, 4) void
xkb_log(struct xkb_context *ctx, enum xkb_log_level level,
        const char *fmt, ...);

#define xkb_log_cond_level(ctx, level, ...) do { \
    if (xkb_get_log_level(ctx) >= (level)) \
    xkb_log((ctx), (level), __VA_ARGS__); \
} while (0)

#define xkb_log_cond_verbosity(ctx, level, vrb, ...) do { \
    if (xkb_get_log_verbosity(ctx) >= (vrb)) \
    xkb_log_cond_level((ctx), (level), __VA_ARGS__); \
} while (0)

/*
 * The format is not part of the argument list in order to avoid the
 * "ISO C99 requires rest arguments to be used" warning when only the
 * format is supplied without arguments. Not supplying it would still
 * result in an error, though.
 */
#define log_dbg(ctx, ...) \
    xkb_log_cond_level((ctx), XKB_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define log_info(ctx, ...) \
    xkb_log_cond_level((ctx), XKB_LOG_LEVEL_INFO, __VA_ARGS__)
#define log_warn(ctx, ...) \
    xkb_log_cond_level((ctx), XKB_LOG_LEVEL_WARNING, __VA_ARGS__)
#define log_err(ctx, ...) \
    xkb_log_cond_level((ctx), XKB_LOG_LEVEL_ERROR, __VA_ARGS__)
#define log_wsgo(ctx, ...) \
    xkb_log_cond_level((ctx), XKB_LOG_LEVEL_CRITICAL, __VA_ARGS__)
#define log_vrb(ctx, vrb, ...) \
    xkb_log_cond_verbosity((ctx), XKB_LOG_LEVEL_WARNING, (vrb), __VA_ARGS__)

#endif /* XKB_PRIV_H */
