/**
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
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

#include "config.h"

#include "xkbcommon/xkbcommon-names.h"
#include "keymap.h"

static void
update_builtin_keymap_fields(struct xkb_keymap *keymap)
{
    /* Predefined (AKA real, core, X11) modifiers. The order is important! */
    static const char *const builtin_mods[] = {
        [0] = XKB_MOD_NAME_SHIFT,
        [1] = XKB_MOD_NAME_CAPS,
        [2] = XKB_MOD_NAME_CTRL,
        [3] = XKB_MOD_NAME_MOD1,
        [4] = XKB_MOD_NAME_MOD2,
        [5] = XKB_MOD_NAME_MOD3,
        [6] = XKB_MOD_NAME_MOD4,
        [7] = XKB_MOD_NAME_MOD5
    };

    for (unsigned i = 0; i < ARRAY_SIZE(builtin_mods); i++) {
        keymap->mods.mods[i].name = xkb_atom_intern(keymap->ctx,
                                                    builtin_mods[i],
                                                    strlen(builtin_mods[i]));
        keymap->mods.mods[i].type = MOD_REAL;
    }
    keymap->mods.num_mods = ARRAY_SIZE(builtin_mods);
}

struct xkb_keymap *
xkb_keymap_new(struct xkb_context *ctx,
               enum xkb_keymap_format format,
               enum xkb_keymap_compile_flags flags)
{
    struct xkb_keymap *keymap;

    keymap = calloc(1, sizeof(*keymap));
    if (!keymap)
        return NULL;

    keymap->refcnt = 1;
    keymap->ctx = xkb_context_ref(ctx);

    keymap->format = format;
    keymap->flags = flags;

    update_builtin_keymap_fields(keymap);

    return keymap;
}

struct xkb_key *
XkbKeyByName(struct xkb_keymap *keymap, xkb_atom_t name, bool use_aliases)
{
    struct xkb_key *key;

    xkb_keys_foreach(key, keymap)
        if (key->name == name)
            return key;

    if (use_aliases) {
        xkb_atom_t new_name = XkbResolveKeyAlias(keymap, name);
        if (new_name != XKB_ATOM_NONE)
            return XkbKeyByName(keymap, new_name, false);
    }

    return NULL;
}

xkb_atom_t
XkbResolveKeyAlias(const struct xkb_keymap *keymap, xkb_atom_t name)
{
    for (unsigned i = 0; i < keymap->num_key_aliases; i++)
        if (keymap->key_aliases[i].alias == name)
            return keymap->key_aliases[i].real;

    return XKB_ATOM_NONE;
}

void
XkbEscapeMapName(char *name)
{
    /*
     * All latin-1 alphanumerics, plus parens, slash, minus, underscore and
     * wildcards.
     */
    static const unsigned char legal[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0xa7, 0xff, 0x83,
        0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x07,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0xff
    };

    if (!name)
        return;

    while (*name) {
        unsigned char c = *name;
        if (!(legal[c / 8] & (1u << (c % 8))))
            *name = '_';
        name++;
    }
}

xkb_mod_index_t
XkbModNameToIndex(const struct xkb_mod_set *mods, xkb_atom_t name,
                  enum mod_type type)
{
    xkb_mod_index_t i;
    const struct xkb_mod *mod;

    xkb_mods_enumerate(i, mod, mods)
        if ((mod->type & type) && name == mod->name)
            return i;

    return XKB_MOD_INVALID;
}

bool
XkbLevelsSameSyms(const struct xkb_level *a, const struct xkb_level *b)
{
    if (a->num_syms != b->num_syms)
        return false;
    if (a->num_syms <= 1)
        return a->s.sym == b->s.sym;
    return memcmp(a->s.syms, b->s.syms, sizeof(*a->s.syms) * a->num_syms) == 0;
}

bool
XkbLevelHasNoKeysym(const struct xkb_level *level)
{
    if (level->num_syms == 0)
        return true;
    if (level->num_syms == 1)
        return level->s.sym == XKB_KEY_NoSymbol;
    for (unsigned int k = 0; k < level->num_syms; k++) {
        if (level->s.syms[k] != XKB_KEY_NoSymbol)
            return false;
    }
    return true;
}

bool
XkbLevelsSameActions(const struct xkb_level *a, const struct xkb_level *b)
{
    if (a->num_syms != b->num_syms)
        return false;
    if (a->num_syms <= 1)
        return memcmp(&a->a.action, &b->a.action, sizeof(a->a.action)) == 0;
    return memcmp(a->a.actions, b->a.actions, sizeof(*a->a.actions) * a->num_syms) == 0;
}

bool
XkbLevelHasNoAction(const struct xkb_level *level)
{
    if (level->num_syms == 0)
        return true;
    if (level->num_syms == 1)
        return level->a.action.type == ACTION_TYPE_NONE;
    for (unsigned int k = 0; k < level->num_syms; k++) {
        if (level->a.actions[k].type != ACTION_TYPE_NONE)
            return false;
    }
    return true;
}

xkb_layout_index_t
XkbWrapGroupIntoRange(int32_t group,
                      xkb_layout_index_t num_groups,
                      enum xkb_range_exceed_type out_of_range_group_action,
                      xkb_layout_index_t out_of_range_group_number)
{
    if (num_groups == 0)
        return XKB_LAYOUT_INVALID;

    if (group >= 0 && (xkb_layout_index_t) group < num_groups)
        return group;

    switch (out_of_range_group_action) {
    case RANGE_REDIRECT:
        if (out_of_range_group_number >= num_groups)
            return 0;
        return out_of_range_group_number;

    case RANGE_SATURATE:
        if (group < 0)
            return 0;
        else
            return num_groups - 1;

    case RANGE_WRAP:
    default:
        /*
         * C99 says a negative dividend in a modulo operation always
         * gives a negative result.
         */
        if (group < 0)
            return ((int) num_groups + (group % (int) num_groups));
        else
            return group % num_groups;
    }
}

unsigned int
xkb_keymap_key_get_actions_by_level(struct xkb_keymap *keymap,
                                    xkb_keycode_t kc,
                                    xkb_layout_index_t layout,
                                    xkb_level_index_t level,
                                    const union xkb_action **actions)
{
    const struct xkb_key *key = XkbKey(keymap, kc);
    if (!key)
        goto err;

    layout = XkbWrapGroupIntoRange(layout, key->num_groups,
                                   key->out_of_range_group_action,
                                   key->out_of_range_group_number);
    if (layout == XKB_LAYOUT_INVALID)
        goto err;

    if (level >= XkbKeyNumLevels(key, layout))
        goto err;

    const unsigned int count = key->groups[layout].levels[level].num_syms;
    switch (count) {
        case 0:
            goto err;
        case 1:
            *actions = &key->groups[layout].levels[level].a.action;
            break;
        default:
            *actions = key->groups[layout].levels[level].a.actions;
    }
    return count;

err:
    *actions = NULL;
    return 0;
}
