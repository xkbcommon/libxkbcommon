/*
 * For HPND:
 * Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.
 *
 * For MIT:
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * SPDX-License-Identifier: HPND AND MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "keymap.h"
#include "text.h"

struct xkb_keymap *
xkb_keymap_ref(struct xkb_keymap *keymap)
{
    keymap->refcnt++;
    return keymap;
}

void
clear_level(struct xkb_level *leveli)
{
    if (leveli->num_syms > 1)
        free(leveli->s.syms);
    if (leveli->num_actions > 1)
        free(leveli->a.actions);
}

static void
clear_interpret(struct xkb_sym_interpret *interp)
{
    if (interp->num_actions > 1)
        free(interp->a.actions);
}

void
xkb_keymap_unref(struct xkb_keymap *keymap)
{
    if (!keymap || --keymap->refcnt > 0)
        return;

    if (keymap->keys) {
        struct xkb_key *key;
        xkb_keys_foreach(key, keymap) {
            if (key->groups) {
                for (unsigned i = 0; i < key->num_groups; i++) {
                    if (key->groups[i].levels) {
                        for (xkb_level_index_t j = 0;
                             j < XkbKeyNumLevels(key, i);
                             j++)
                            clear_level(&key->groups[i].levels[j]);
                        free(key->groups[i].levels);
                    }
                }
                free(key->groups);
            }
        }
        free(keymap->keys);
    }
    if (keymap->types) {
        for (unsigned i = 0; i < keymap->num_types; i++) {
            free(keymap->types[i].entries);
            free(keymap->types[i].level_names);
        }
        free(keymap->types);
    }
    for (unsigned int k = 0; k < keymap->num_sym_interprets; k++) {
        clear_interpret(&keymap->sym_interprets[k]);
    }
    free(keymap->sym_interprets);
    free(keymap->key_aliases);
    free(keymap->group_names);
    free(keymap->keycodes_section_name);
    free(keymap->symbols_section_name);
    free(keymap->types_section_name);
    free(keymap->compat_section_name);
    xkb_context_unref(keymap->ctx);
    free(keymap);
}

static const struct xkb_keymap_format_ops *
get_keymap_format_ops(enum xkb_keymap_format format)
{
    static const struct xkb_keymap_format_ops *keymap_format_ops[] = {
        [XKB_KEYMAP_FORMAT_TEXT_V1] = &text_v1_keymap_format_ops,
    };

    if ((int) format < 0 || (int) format >= (int) ARRAY_SIZE(keymap_format_ops))
        return NULL;

    return keymap_format_ops[(int) format];
}

struct xkb_keymap *
xkb_keymap_new_from_names(struct xkb_context *ctx,
                          const struct xkb_rule_names *rmlvo_in,
                          enum xkb_keymap_compile_flags flags)
{
    struct xkb_keymap *keymap;
    struct xkb_rule_names rmlvo;
    const enum xkb_keymap_format format = XKB_KEYMAP_FORMAT_TEXT_V1;
    const struct xkb_keymap_format_ops *ops;

    ops = get_keymap_format_ops(format);
    if (!ops || !ops->keymap_new_from_names) {
        log_err_func(ctx, XKB_LOG_MESSAGE_NO_ID,
                     "unsupported keymap format: %d\n", format);
        return NULL;
    }

    if (flags & ~(XKB_KEYMAP_COMPILE_NO_FLAGS)) {
        log_err_func(ctx, XKB_LOG_MESSAGE_NO_ID,
                     "unrecognized flags: %#x\n", flags);
        return NULL;
    }

    keymap = xkb_keymap_new(ctx, format, flags);
    if (!keymap)
        return NULL;

    if (rmlvo_in)
        rmlvo = *rmlvo_in;
    else
        memset(&rmlvo, 0, sizeof(rmlvo));
    xkb_context_sanitize_rule_names(ctx, &rmlvo);

    if (!ops->keymap_new_from_names(keymap, &rmlvo)) {
        xkb_keymap_unref(keymap);
        return NULL;
    }

    return keymap;
}

struct xkb_keymap *
xkb_keymap_new_from_string(struct xkb_context *ctx,
                           const char *string,
                           enum xkb_keymap_format format,
                           enum xkb_keymap_compile_flags flags)
{
    return xkb_keymap_new_from_buffer(ctx, string, strlen(string),
                                      format, flags);
}

struct xkb_keymap *
xkb_keymap_new_from_buffer(struct xkb_context *ctx,
                           const char *buffer, size_t length,
                           enum xkb_keymap_format format,
                           enum xkb_keymap_compile_flags flags)
{
    struct xkb_keymap *keymap;
    const struct xkb_keymap_format_ops *ops;

    ops = get_keymap_format_ops(format);
    if (!ops || !ops->keymap_new_from_string) {
        log_err_func(ctx, XKB_LOG_MESSAGE_NO_ID,
                     "unsupported keymap format: %d\n", format);
        return NULL;
    }

    if (flags & ~(XKB_KEYMAP_COMPILE_NO_FLAGS)) {
        log_err_func(ctx, XKB_LOG_MESSAGE_NO_ID,
                     "unrecognized flags: %#x\n", flags);
        return NULL;
    }

    if (!buffer) {
        log_err_func1(ctx, XKB_LOG_MESSAGE_NO_ID,
                      "no buffer specified\n");
        return NULL;
    }

    keymap = xkb_keymap_new(ctx, format, flags);
    if (!keymap)
        return NULL;

    /* Allow a zero-terminated string as a buffer */
    if (length > 0 && buffer[length - 1] == '\0')
        length--;

    if (!ops->keymap_new_from_string(keymap, buffer, length)) {
        xkb_keymap_unref(keymap);
        return NULL;
    }

    return keymap;
}

struct xkb_keymap *
xkb_keymap_new_from_file(struct xkb_context *ctx,
                         FILE *file,
                         enum xkb_keymap_format format,
                         enum xkb_keymap_compile_flags flags)
{
    struct xkb_keymap *keymap;
    const struct xkb_keymap_format_ops *ops;

    ops = get_keymap_format_ops(format);
    if (!ops || !ops->keymap_new_from_file) {
        log_err_func(ctx, XKB_LOG_MESSAGE_NO_ID,
                     "unsupported keymap format: %d\n", format);
        return NULL;
    }

    if (flags & ~(XKB_KEYMAP_COMPILE_NO_FLAGS)) {
        log_err_func(ctx, XKB_LOG_MESSAGE_NO_ID,
                     "unrecognized flags: %#x\n", flags);
        return NULL;
    }

    if (!file) {
        log_err_func1(ctx, XKB_LOG_MESSAGE_NO_ID,
                      "no file specified\n");
        return NULL;
    }

    keymap = xkb_keymap_new(ctx, format, flags);
    if (!keymap)
        return NULL;

    if (!ops->keymap_new_from_file(keymap, file)) {
        xkb_keymap_unref(keymap);
        return NULL;
    }

    return keymap;
}

char *
xkb_keymap_get_as_string(struct xkb_keymap *keymap,
                         enum xkb_keymap_format format)
{
    const struct xkb_keymap_format_ops *ops;

    if (format == XKB_KEYMAP_USE_ORIGINAL_FORMAT)
        format = keymap->format;

    ops = get_keymap_format_ops(format);
    if (!ops || !ops->keymap_get_as_string) {
        log_err_func(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
                     "unsupported keymap format: %d\n", format);
        return NULL;
    }

    return ops->keymap_get_as_string(keymap);
}

/**
 * Returns the total number of modifiers active in the keymap.
 */
xkb_mod_index_t
xkb_keymap_num_mods(struct xkb_keymap *keymap)
{
    return keymap->mods.num_mods;
}

/**
 * Return the name for a given modifier.
 */
const char *
xkb_keymap_mod_get_name(struct xkb_keymap *keymap, xkb_mod_index_t idx)
{
    if (idx >= keymap->mods.num_mods)
        return NULL;

    return xkb_atom_text(keymap->ctx, keymap->mods.mods[idx].name);
}

/**
 * Returns the index for a named modifier.
 */
xkb_mod_index_t
xkb_keymap_mod_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_atom_t atom;

    atom = xkb_atom_lookup(keymap->ctx, name);
    if (atom == XKB_ATOM_NONE)
        return XKB_MOD_INVALID;

    return XkbModNameToIndex(&keymap->mods, atom, MOD_BOTH);
}

/**
 * Return the total number of active groups in the keymap.
 */
xkb_layout_index_t
xkb_keymap_num_layouts(struct xkb_keymap *keymap)
{
    return keymap->num_groups;
}

/**
 * Returns the name for a given group.
 */
const char *
xkb_keymap_layout_get_name(struct xkb_keymap *keymap, xkb_layout_index_t idx)
{
    if (idx >= keymap->num_group_names)
        return NULL;

    return xkb_atom_text(keymap->ctx, keymap->group_names[idx]);
}

/**
 * Returns the index for a named layout.
 */
xkb_layout_index_t
xkb_keymap_layout_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_atom_t atom = xkb_atom_lookup(keymap->ctx, name);
    xkb_layout_index_t i;

    if (atom == XKB_ATOM_NONE)
        return XKB_LAYOUT_INVALID;

    for (i = 0; i < keymap->num_group_names; i++)
        if (keymap->group_names[i] == atom)
            return i;

    return XKB_LAYOUT_INVALID;
}

/**
 * Returns the number of layouts active for a particular key.
 */
xkb_layout_index_t
xkb_keymap_num_layouts_for_key(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    const struct xkb_key *key = XkbKey(keymap, kc);

    if (!key)
        return 0;

    return key->num_groups;
}

/**
 * Returns the number of levels active for a particular key and layout.
 */
xkb_level_index_t
xkb_keymap_num_levels_for_key(struct xkb_keymap *keymap, xkb_keycode_t kc,
                              xkb_layout_index_t layout)
{
    const struct xkb_key *key = XkbKey(keymap, kc);

    if (!key)
        return 0;

    static_assert(XKB_MAX_GROUPS < INT32_MAX, "Max groups don't fit");
    layout = XkbWrapGroupIntoRange((int32_t) layout, key->num_groups,
                                   key->out_of_range_group_action,
                                   key->out_of_range_group_number);
    if (layout == XKB_LAYOUT_INVALID)
        return 0;

    return XkbKeyNumLevels(key, layout);
}

/**
 * Return the total number of LEDs in the keymap.
 */
xkb_led_index_t
xkb_keymap_num_leds(struct xkb_keymap *keymap)
{
    return keymap->num_leds;
}

/**
 * Returns the name for a given LED.
 */
const char *
xkb_keymap_led_get_name(struct xkb_keymap *keymap, xkb_led_index_t idx)
{
    if (idx >= keymap->num_leds)
        return NULL;

    return xkb_atom_text(keymap->ctx, keymap->leds[idx].name);
}

/**
 * Returns the index for a named LED.
 */
xkb_led_index_t
xkb_keymap_led_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_atom_t atom = xkb_atom_lookup(keymap->ctx, name);
    xkb_led_index_t i;
    const struct xkb_led *led;

    if (atom == XKB_ATOM_NONE)
        return XKB_LED_INVALID;

    xkb_leds_enumerate(i, led, keymap)
        if (led->name == atom)
            return i;

    return XKB_LED_INVALID;
}

size_t
xkb_keymap_key_get_mods_for_level(struct xkb_keymap *keymap,
                                  xkb_keycode_t kc,
                                  xkb_layout_index_t layout,
                                  xkb_level_index_t level,
                                  xkb_mod_mask_t *masks_out,
                                  size_t masks_size)
{
    const struct xkb_key *key = XkbKey(keymap, kc);
    if (!key)
        return 0;

    static_assert(XKB_MAX_GROUPS < INT32_MAX, "Max groups don't fit");
    layout = XkbWrapGroupIntoRange((int32_t) layout, key->num_groups,
                                   key->out_of_range_group_action,
                                   key->out_of_range_group_number);
    if (layout == XKB_LAYOUT_INVALID)
        return 0;

    if (level >= XkbKeyNumLevels(key, layout))
        return 0;

    const struct xkb_key_type *type = key->groups[layout].type;

    size_t count = 0;

    /*
     * If the active set of modifiers doesn't match any explicit entry of
     * the key type, the resulting level is 0 (i.e. Level 1).
     * So, if we are asked to find the modifiers for level==0, we can offer
     * an ~infinite supply, which is not very workable.
     * What we do instead, is special case the empty set of modifiers for
     * this purpose. If the empty set isn't explicit mapped to a level, we
     * take it to map to Level 1.
     * This is almost always what we want. If applicable, given it priority
     * over other ways to generate the level.
     */
    if (level == 0) {
        bool empty_mapped = false;
        for (unsigned i = 0; i < type->num_entries && count < masks_size; i++)
            if (entry_is_active(&type->entries[i]) &&
                type->entries[i].mods.mask == 0) {
                empty_mapped = true;
                break;
            }
        if (!empty_mapped && count < masks_size) {
            masks_out[count++] = 0;
        }
    }

    /* Now search explicit mappings. */
    for (unsigned i = 0; i < type->num_entries && count < masks_size; i++) {
        if (entry_is_active(&type->entries[i]) &&
            type->entries[i].level == level) {
            masks_out[count++] = type->entries[i].mods.mask;
        }
    }

    return count;
}

/**
 * As below, but takes an explicit layout/level rather than state.
 */
int
xkb_keymap_key_get_syms_by_level(struct xkb_keymap *keymap,
                                 xkb_keycode_t kc,
                                 xkb_layout_index_t layout,
                                 xkb_level_index_t level,
                                 const xkb_keysym_t **syms_out)
{
    const struct xkb_key *key = XkbKey(keymap, kc);

    if (!key)
        goto err;

    static_assert(XKB_MAX_GROUPS < INT32_MAX, "Max groups don't fit");
    layout = XkbWrapGroupIntoRange((int32_t) layout, key->num_groups,
                                   key->out_of_range_group_action,
                                   key->out_of_range_group_number);
    if (layout == XKB_LAYOUT_INVALID)
        goto err;

    if (level >= XkbKeyNumLevels(key, layout))
        goto err;

    const unsigned int num_syms = key->groups[layout].levels[level].num_syms;
    if (num_syms == 0)
        goto err;

    if (num_syms == 1)
        *syms_out = &key->groups[layout].levels[level].s.sym;
    else
        *syms_out = key->groups[layout].levels[level].s.syms;

    return (int) num_syms;

err:
    *syms_out = NULL;
    return 0;
}

xkb_keycode_t
xkb_keymap_min_keycode(struct xkb_keymap *keymap)
{
    return keymap->min_key_code;
}

xkb_keycode_t
xkb_keymap_max_keycode(struct xkb_keymap *keymap)
{
    return keymap->max_key_code;
}

void
xkb_keymap_key_for_each(struct xkb_keymap *keymap, xkb_keymap_key_iter_t iter,
                        void *data)
{
    struct xkb_key *key;

    xkb_keys_foreach(key, keymap)
        iter(keymap, key->keycode, data);
}

const char *
xkb_keymap_key_get_name(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    const struct xkb_key *key = XkbKey(keymap, kc);

    if (!key)
        return NULL;

    return xkb_atom_text(keymap->ctx, key->name);
}

xkb_keycode_t
xkb_keymap_key_by_name(struct xkb_keymap *keymap, const char *name)
{
    struct xkb_key *key;
    xkb_atom_t atom;

    atom = xkb_atom_lookup(keymap->ctx, name);
    if (atom) {
        xkb_atom_t ratom = XkbResolveKeyAlias(keymap, atom);
        if (ratom)
            atom = ratom;
    }
    if (!atom)
        return XKB_KEYCODE_INVALID;

    xkb_keys_foreach(key, keymap) {
        if (key->name == atom)
            return key->keycode;
    }

    return XKB_KEYCODE_INVALID;
}

/**
 * Simple boolean specifying whether or not the key should repeat.
 */
int
xkb_keymap_key_repeats(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    const struct xkb_key *key = XkbKey(keymap, kc);

    if (!key)
        return 0;

    return key->repeats;
}
