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

#include "map.h"
#include "text.h"

struct xkb_keymap *
xkb_map_new(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    keymap = calloc(1, sizeof(*keymap));
    if (!keymap)
        return NULL;

    keymap->refcnt = 1;
    keymap->ctx = xkb_context_ref(ctx);

    return keymap;
}

XKB_EXPORT struct xkb_keymap *
xkb_keymap_ref(struct xkb_keymap *keymap)
{
    keymap->refcnt++;
    return keymap;
}

XKB_EXPORT void
xkb_keymap_unref(struct xkb_keymap *keymap)
{
    unsigned int i;
    struct xkb_key *key;

    if (!keymap || --keymap->refcnt > 0)
        return;

    for (i = 0; i < keymap->num_types; i++) {
        free(keymap->types[i].map);
        free(keymap->types[i].level_names);
    }
    free(keymap->types);
    darray_foreach(key, keymap->keys) {
        free(key->sym_index);
        free(key->num_syms);
        free(key->syms);
        free(key->actions);
    }
    darray_free(keymap->keys);
    darray_free(keymap->sym_interpret);
    darray_free(keymap->key_aliases);
    free(keymap->keycodes_section_name);
    free(keymap->symbols_section_name);
    free(keymap->types_section_name);
    free(keymap->compat_section_name);
    xkb_context_unref(keymap->ctx);
    free(keymap);
}

/**
 * Returns the total number of modifiers active in the keymap.
 */
XKB_EXPORT xkb_mod_index_t
xkb_keymap_num_mods(struct xkb_keymap *keymap)
{
    xkb_mod_index_t i;

    for (i = 0; i < XKB_NUM_VIRTUAL_MODS; i++)
        if (!keymap->vmod_names[i])
            break;

    /* We always have all the core modifiers (for now), plus any virtual
     * modifiers we may have defined. */
    return i + XKB_NUM_CORE_MODS;
}

/**
 * Return the name for a given modifier.
 */
XKB_EXPORT const char *
xkb_keymap_mod_get_name(struct xkb_keymap *keymap, xkb_mod_index_t idx)
{
    const char *name;

    if (idx >= xkb_map_num_mods(keymap))
        return NULL;

    /* First try to find a legacy modifier name.  If that fails, try to
     * find a virtual mod name. */
    name = ModIndexToName(idx);
    if (!name)
        name = xkb_atom_text(keymap->ctx,
                             keymap->vmod_names[idx - XKB_NUM_CORE_MODS]);

    return name;
}

/**
 * Returns the index for a named modifier.
 */
XKB_EXPORT xkb_mod_index_t
xkb_keymap_mod_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_mod_index_t i;
    xkb_atom_t atom;

    i = ModNameToIndex(name);
    if (i != XKB_MOD_INVALID)
        return i;

    atom = xkb_atom_lookup(keymap->ctx, name);
    if (atom == XKB_ATOM_NONE)
        return XKB_MOD_INVALID;

    for (i = 0; i < XKB_NUM_VIRTUAL_MODS; i++) {
        if (keymap->vmod_names[i] == XKB_ATOM_NONE)
            break;
        if (keymap->vmod_names[i] == atom)
            return i + XKB_NUM_CORE_MODS;
    }

    return XKB_MOD_INVALID;
}

/**
 * Return the total number of active groups in the keymap.
 */
XKB_EXPORT xkb_layout_index_t
xkb_keymap_num_layouts(struct xkb_keymap *keymap)
{
    return keymap->num_groups;
}

/**
 * Returns the name for a given group.
 */
XKB_EXPORT const char *
xkb_keymap_layout_get_name(struct xkb_keymap *keymap, xkb_layout_index_t idx)
{
    if (idx >= xkb_keymap_num_layouts(keymap))
        return NULL;

    return xkb_atom_text(keymap->ctx, keymap->group_names[idx]);
}

/**
 * Returns the index for a named layout.
 */
XKB_EXPORT xkb_layout_index_t
xkb_keymap_layout_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_layout_index_t num_groups = xkb_keymap_num_layouts(keymap);
    xkb_atom_t atom = xkb_atom_lookup(keymap->ctx, name);
    xkb_layout_index_t i;

    if (atom == XKB_ATOM_NONE)
        return XKB_GROUP_INVALID;

    for (i = 0; i < num_groups; i++)
        if (keymap->group_names[i] == atom)
            return i;

    return XKB_GROUP_INVALID;
}

/**
 * Returns the number of layouts active for a particular key.
 */
XKB_EXPORT xkb_layout_index_t
xkb_keymap_num_layouts_for_key(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    const struct xkb_key *key = XkbKey(keymap, kc);
    if (!key)
        return 0;

    return key->num_groups;
}

/**
 * Return the total number of active LEDs in the keymap.
 */
XKB_EXPORT xkb_led_index_t
xkb_keymap_num_leds(struct xkb_keymap *keymap)
{
    xkb_led_index_t ret = 0;
    xkb_led_index_t i;

    for (i = 0; i < XKB_NUM_INDICATORS; i++)
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
xkb_keymap_led_get_name(struct xkb_keymap *keymap, xkb_led_index_t idx)
{
    if (idx >= xkb_map_num_leds(keymap))
        return NULL;

    return xkb_atom_text(keymap->ctx, keymap->indicators[idx].name);
}

/**
 * Returns the index for a named group.
 */
XKB_EXPORT xkb_group_index_t
xkb_keymap_led_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_led_index_t num_leds = xkb_map_num_leds(keymap);
    xkb_atom_t atom = xkb_atom_lookup(keymap->ctx, name);
    xkb_led_index_t i;

    if (atom == XKB_ATOM_NONE)
        return XKB_LED_INVALID;

    for (i = 0; i < num_leds; i++)
        if (keymap->indicators[i].name == atom)
            return i;

    return XKB_LED_INVALID;
}

static struct xkb_kt_map_entry *
get_entry_for_key_state(struct xkb_state *state, const struct xkb_key *key,
                        xkb_group_index_t group)
{
    struct xkb_key_type *type;
    xkb_mod_mask_t active_mods;
    unsigned int i;

    type = XkbKeyType(xkb_state_get_map(state), key, group);
    active_mods = xkb_state_serialize_mods(state, XKB_STATE_EFFECTIVE);
    active_mods &= type->mods.mask;

    for (i = 0; i < type->num_entries; i++)
        if (type->map[i].mods.mask == active_mods)
            return &type->map[i];

    return NULL;
}

/**
 * Returns the level to use for the given key and state, or
 * XKB_LEVEL_INVALID.
 */
xkb_level_index_t
xkb_key_get_level(struct xkb_state *state, const struct xkb_key *key,
                  xkb_group_index_t group)
{
    struct xkb_kt_map_entry *entry;

    /* If we don't find an explicit match the default is 0. */
    entry = get_entry_for_key_state(state, key, group);
    if (!entry)
        return 0;

    return entry->level;
}

/**
 * Returns the group to use for the given key and state, taking
 * wrapping/clamping/etc into account, or XKB_GROUP_INVALID.
 */
xkb_group_index_t
xkb_key_get_group(struct xkb_state *state, const struct xkb_key *key)
{
    xkb_group_index_t ret = xkb_state_serialize_group(state,
                                                      XKB_STATE_EFFECTIVE);

    if (key->num_groups == 0)
        return XKB_GROUP_INVALID;

    if (ret < key->num_groups)
        return ret;

    switch (key->out_of_range_group_action) {
    case RANGE_REDIRECT:
        ret = key->out_of_range_group_number;
        if (ret >= key->num_groups)
            ret = 0;
        break;

    case RANGE_SATURATE:
        ret = key->num_groups - 1;
        break;

    case RANGE_WRAP:
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
xkb_key_get_syms_by_level(struct xkb_keymap *keymap,
                          const struct xkb_key *key,
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
xkb_state_key_get_syms(struct xkb_state *state, xkb_keycode_t kc,
                       const xkb_keysym_t **syms_out)
{
    struct xkb_keymap *keymap = xkb_state_get_map(state);
    xkb_group_index_t group;
    xkb_level_index_t level;
    const struct xkb_key *key = XkbKey(keymap, kc);
    if (!key)
        return -1;

    group = xkb_key_get_group(state, key);
    if (group == XKB_GROUP_INVALID)
        goto err;

    level = xkb_key_get_level(state, key, group);
    if (level == XKB_LEVEL_INVALID)
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
xkb_keymap_key_repeats(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    const struct xkb_key *key = XkbKey(keymap, kc);
    if (!key)
        return 0;

    return key->repeats;
}

static xkb_mod_mask_t
key_get_consumed(struct xkb_state *state, const struct xkb_key *key)
{
    struct xkb_kt_map_entry *entry;
    xkb_group_index_t group;

    group = xkb_key_get_group(state, key);
    if (group == XKB_GROUP_INVALID)
        return 0;

    entry = get_entry_for_key_state(state, key, group);
    if (!entry)
        return 0;

    return entry->mods.mask & ~entry->preserve.mask;
}

/**
 * Tests to see if a modifier is used up by our translation of a
 * keycode to keysyms, taking note of the current modifier state and
 * the appropriate key type's preserve information, if any. This allows
 * the user to mask out the modifier in later processing of the
 * modifiers, e.g. when implementing hot keys or accelerators.
 *
 * See also, for example:
 * - XkbTranslateKeyCode(3), mod_rtrn retrun value, from libX11.
 * - gdk_keymap_translate_keyboard_state, consumed_modifiers return value,
 *   from gtk+.
 */
XKB_EXPORT int
xkb_state_mod_index_is_consumed(struct xkb_state *state, xkb_keycode_t kc,
                                xkb_mod_index_t idx)
{
    const struct xkb_key *key = XkbKey(xkb_state_get_map(state), kc);
    if (!key)
        return 0;

    return !!((1 << idx) & key_get_consumed(state, key));
}

/**
 * Calculates which modifiers should be consumed during key processing,
 * and returns the mask with all these modifiers removed.  e.g. if
 * given a state of Alt and Shift active for a two-level alphabetic
 * key containing plus and equal on the first and second level
 * respectively, will return a mask of only Alt, as Shift has been
 * consumed by the type handling.
 */
XKB_EXPORT xkb_mod_mask_t
xkb_state_mod_mask_remove_consumed(struct xkb_state *state, xkb_keycode_t kc,
                                   xkb_mod_mask_t mask)
{
    const struct xkb_key *key = XkbKey(xkb_state_get_map(state), kc);
    if (!key)
        return 0;

    return mask & ~key_get_consumed(state, key);
}
