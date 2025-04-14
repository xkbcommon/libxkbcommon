/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "xkbcommon/xkbcommon-names.h"
#include "keymap.h"

static void
update_builtin_keymap_fields(struct xkb_keymap *keymap)
{
    /* Predefined (AKA real, core, X11) modifiers. The order is important! */
    static const char *const builtin_mods[] = {
        [XKB_MOD_INDEX_SHIFT] = XKB_MOD_NAME_SHIFT,
        [XKB_MOD_INDEX_CAPS]  = XKB_MOD_NAME_CAPS,
        [XKB_MOD_INDEX_CTRL]  = XKB_MOD_NAME_CTRL,
        [XKB_MOD_INDEX_MOD1]  = XKB_MOD_NAME_MOD1,
        [XKB_MOD_INDEX_MOD2]  = XKB_MOD_NAME_MOD2,
        [XKB_MOD_INDEX_MOD3]  = XKB_MOD_NAME_MOD3,
        [XKB_MOD_INDEX_MOD4]  = XKB_MOD_NAME_MOD4,
        [XKB_MOD_INDEX_MOD5]  = XKB_MOD_NAME_MOD5
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
action_equal(const union xkb_action *a, const union xkb_action *b)
{
    if (a->type != b->type)
        return false;

    /* Ensure we support all action types */
    static_assert(ACTION_TYPE_PRIVATE == 15 &&
                  ACTION_TYPE_PRIVATE + 1 == _ACTION_TYPE_NUM_ENTRIES,
                  "Missing action type");

    switch (a->type) {
    case ACTION_TYPE_NONE:
        return true;
    case ACTION_TYPE_MOD_SET:
    case ACTION_TYPE_MOD_LATCH:
    case ACTION_TYPE_MOD_LOCK:
        return (a->mods.flags == b->mods.flags &&
                a->mods.mods.mask == b->mods.mods.mask &&
                a->mods.mods.mods == b->mods.mods.mods);
    case ACTION_TYPE_GROUP_SET:
    case ACTION_TYPE_GROUP_LATCH:
    case ACTION_TYPE_GROUP_LOCK:
        return (a->group.flags == b->group.flags &&
                a->group.group == b->group.group);
    case ACTION_TYPE_PTR_MOVE:
        return (a->ptr.flags == b->ptr.flags &&
                a->ptr.x == b->ptr.x &&
                a->ptr.y == b->ptr.y);
    case ACTION_TYPE_PTR_BUTTON:
    case ACTION_TYPE_PTR_LOCK:
        return (a->btn.flags == b->btn.flags &&
                a->btn.button == b->btn.button &&
                a->btn.count == b->btn.count);
    case ACTION_TYPE_PTR_DEFAULT:
        return (a->dflt.flags == b->dflt.flags &&
                a->dflt.value == b->dflt.value);
    case ACTION_TYPE_TERMINATE:
        return true;
    case ACTION_TYPE_SWITCH_VT:
        return (a->screen.flags == b->screen.flags &&
                a->screen.screen == b->screen.screen);
    case ACTION_TYPE_CTRL_SET:
    case ACTION_TYPE_CTRL_LOCK:
        return (a->ctrls.flags == b->ctrls.flags &&
                a->ctrls.ctrls == b->ctrls.ctrls);
    case ACTION_TYPE_PRIVATE:
        return (a->priv.data == b->priv.data);
    default:
        assert(!"Unsupported action");
        return false;
    }
}

bool
XkbLevelsSameActions(const struct xkb_level *a, const struct xkb_level *b)
{
    if (a->num_actions != b->num_actions)
        return false;
    if (a->num_actions <= 1)
        return action_equal(&a->a.action, &b->a.action);
    for (xkb_action_count_t k = 0; k < a->num_actions; k++) {
        if (!action_equal(&a->a.actions[k], &b->a.actions[k]))
            return false;
    }
    return true;
}

/* See: XkbAdjustGroup in Xorg xserver */
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
    default: {
        /*
         * Ensure conversion xkb_layout_index_t → int32_t is lossless.
         *
         * NOTE: C11 requires a compound statement because it does not allow
         * a declaration after a label (C23 does allow it).
         */
        static_assert(XKB_MAX_GROUPS < INT32_MAX, "Max groups max don't fit");
        /*
         * % returns the *remainder* of the division, not the modulus (see C11
         * standard 6.5.5 “Multiplicative operators”: a % b = a - (a/b)*b. While
         * both operators return the same result for positive operands, they do
         * not for e.g. a negative dividend: remainder may be negative (in the
         * open interval ]-num_groups, num_groups[) while the modulus is always
         * positive. So if the remainder is negative, we must add `num_groups`
         * to get the modulus.
         */
        const int32_t rem = group % (int32_t) num_groups;
        return (rem >= 0) ? rem : rem + (int32_t) num_groups;
    }
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

    layout = XkbWrapGroupIntoRange((int32_t) layout, key->num_groups,
                                   key->out_of_range_group_action,
                                   key->out_of_range_group_number);
    if (layout == XKB_LAYOUT_INVALID)
        goto err;

    if (level >= XkbKeyNumLevels(key, layout))
        goto err;

    const xkb_action_count_t count =
        key->groups[layout].levels[level].num_actions;
    switch (count) {
        case 0:
            goto err;
        case 1:
            *actions = &key->groups[layout].levels[level].a.action;
            break;
        default:
            *actions = key->groups[layout].levels[level].a.actions;
    }
    return (int) count;

err:
    *actions = NULL;
    return 0;
}
