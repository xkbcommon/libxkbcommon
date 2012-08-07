/**
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
 * ********************************************************/

#include "xkb-priv.h"
#include "text.h"

/**
 * Returns the total number of modifiers active in the keymap.
 */
XKB_EXPORT xkb_mod_index_t
xkb_map_num_mods(struct xkb_keymap *keymap)
{
    xkb_mod_index_t i;

    for (i = 0; i < XkbNumVirtualMods; i++)
        if (!keymap->vmod_names[i])
            break;

    /* We always have all the core modifiers (for now), plus any virtual
     * modifiers we may have defined. */
    return i + XkbNumModifiers;
}

/**
 * Return the name for a given modifier.
 */
XKB_EXPORT const char *
xkb_map_mod_get_name(struct xkb_keymap *keymap, xkb_mod_index_t idx)
{
    const char *name;

    if (idx >= xkb_map_num_mods(keymap))
        return NULL;

    /* First try to find a legacy modifier name.  If that fails, try to
     * find a virtual mod name. */
    name = ModIndexToName(idx);
    if (!name)
        name = keymap->vmod_names[idx - XkbNumModifiers];

    return name;
}

/**
 * Returns the index for a named modifier.
 */
XKB_EXPORT xkb_mod_index_t
xkb_map_mod_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_mod_index_t i;

    i = ModNameToIndex(name);
    if (i != XKB_MOD_INVALID)
        return i;

    for (i = 0; i < XkbNumVirtualMods && keymap->vmod_names[i]; i++) {
        if (istreq(name, keymap->vmod_names[i]))
            return i + XkbNumModifiers;
    }

    return XKB_MOD_INVALID;
}

/**
 * Return the total number of active groups in the keymap.
 */
XKB_EXPORT xkb_group_index_t
xkb_map_num_groups(struct xkb_keymap *keymap)
{
    xkb_group_index_t ret = 0;
    xkb_group_index_t i;

    for (i = 0; i < XkbNumKbdGroups; i++)
        if (keymap->groups[i].mask)
            ret++;

    return ret;
}

/**
 * Returns the name for a given group.
 */
XKB_EXPORT const char *
xkb_map_group_get_name(struct xkb_keymap *keymap, xkb_group_index_t idx)
{
    if (idx >= xkb_map_num_groups(keymap))
        return NULL;

    return keymap->group_names[idx];
}

/**
 * Returns the index for a named group.
 */
XKB_EXPORT xkb_group_index_t
xkb_map_group_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_group_index_t num_groups = xkb_map_num_groups(keymap);
    xkb_group_index_t i;

    for (i = 0; i < num_groups; i++)
        if (istreq(keymap->group_names[i], name))
            return i;

    return XKB_GROUP_INVALID;
}

/**
 * Returns the number of groups active for a particular key.
 */
XKB_EXPORT xkb_group_index_t
xkb_key_num_groups(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    if (XkbKeycodeInRange(keymap, kc))
        return XkbKey(keymap, kc)->num_groups;
    return 0;
}

/**
 * Return the total number of active LEDs in the keymap.
 */
XKB_EXPORT xkb_led_index_t
xkb_map_num_leds(struct xkb_keymap *keymap)
{
    xkb_led_index_t ret = 0;
    xkb_led_index_t i;

    for (i = 0; i < XkbNumIndicators; i++)
        if (keymap->indicators[i].which_groups ||
            keymap->indicators[i].which_mods ||
            keymap->indicators[i].ctrls)
            ret++;

    return ret;
}

/**
 * Returns the name for a given group.
 */
XKB_EXPORT const char *
xkb_map_led_get_name(struct xkb_keymap *keymap, xkb_led_index_t idx)
{
    if (idx >= xkb_map_num_leds(keymap))
        return NULL;

    return keymap->indicator_names[idx];
}

/**
 * Returns the index for a named group.
 */
XKB_EXPORT xkb_group_index_t
xkb_map_led_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_led_index_t num_leds = xkb_map_num_leds(keymap);
    xkb_led_index_t i;

    for (i = 0; i < num_leds; i++)
        if (istreq(keymap->indicator_names[i], name))
            return i;

    return XKB_LED_INVALID;
}

/**
 * Returns the level to use for the given key and state, or -1 if invalid.
 */
xkb_level_index_t
xkb_key_get_level(struct xkb_state *state, xkb_keycode_t kc,
                  xkb_group_index_t group)
{
    struct xkb_keymap *keymap = xkb_state_get_map(state);
    struct xkb_key_type *type;
    unsigned int i;
    xkb_mod_mask_t active_mods;

    if (!XkbKeycodeInRange(keymap, kc))
        return 0;

    type = XkbKeyType(keymap, XkbKey(keymap, kc), group);
    active_mods = xkb_state_serialize_mods(state, XKB_STATE_EFFECTIVE);
    active_mods &= type->mods.mask;

    for (i = 0; i < type->num_entries; i++)
        if (type->map[i].mods.mask == active_mods)
            return type->map[i].level;

    return 0;
}

/**
 * Returns the group to use for the given key and state, taking
 * wrapping/clamping/etc into account.
 */
xkb_group_index_t
xkb_key_get_group(struct xkb_state *state, xkb_keycode_t kc)
{
    struct xkb_keymap *keymap = xkb_state_get_map(state);
    xkb_group_index_t ret = xkb_state_serialize_group(state,
                                                      XKB_STATE_EFFECTIVE);
    struct xkb_key *key;

    if (!XkbKeycodeInRange(keymap, kc))
        return 0;
    key = XkbKey(keymap, kc);

    if (ret < key->num_groups)
        return ret;

    switch (key->out_of_range_group_action) {
    case XkbRedirectIntoRange:
        ret = key->out_of_range_group_number;
        if (ret >= key->num_groups)
            ret = 0;
        break;

    case XkbClampIntoRange:
        ret = key->num_groups - 1;
        break;

    case XkbWrapIntoRange:
    default:
        ret %= key->num_groups;
        break;
    }

    return ret;
}

/**
 * As below, but takes an explicit group/level rather than state.
 */
int
xkb_key_get_syms_by_level(struct xkb_keymap *keymap, struct xkb_key *key,
                          xkb_group_index_t group, xkb_level_index_t level,
                          const xkb_keysym_t **syms_out)
{
    int num_syms;

    if (group >= key->num_groups)
        goto err;
    if (level >= XkbKeyGroupWidth(keymap, key, group))
        goto err;

    num_syms = XkbKeyNumSyms(key, group, level);
    if (num_syms == 0)
        goto err;

    *syms_out = XkbKeySymEntry(key, group, level);
    return num_syms;

err:
    *syms_out = NULL;
    return 0;
}

/**
 * Provides the symbols to use for the given key and state.  Returns the
 * number of symbols pointed to in syms_out.
 */
XKB_EXPORT int
xkb_key_get_syms(struct xkb_state *state, xkb_keycode_t kc,
                 const xkb_keysym_t **syms_out)
{
    struct xkb_keymap *keymap = xkb_state_get_map(state);
    struct xkb_key *key;
    xkb_group_index_t group;
    xkb_level_index_t level;

    if (!state || !XkbKeycodeInRange(keymap, kc))
        return -1;

    key = XkbKey(keymap, kc);

    group = xkb_key_get_group(state, kc);
    if (group == -1)
        goto err;

    level = xkb_key_get_level(state, kc, group);
    if (level == -1)
        goto err;

    return xkb_key_get_syms_by_level(keymap, key, group, level, syms_out);

err:
    *syms_out = NULL;
    return 0;
}

/**
 * Simple boolean specifying whether or not the key should repeat.
 */
XKB_EXPORT int
xkb_key_repeats(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    if (!XkbKeycodeInRange(keymap, kc))
        return 0;
    return XkbKey(keymap, kc)->repeats;
}
