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

/*
 * This is a bastardised version of xkbActions.c from the X server which
 * does not support, for the moment:
 *   - AccessX sticky/debounce/etc (will come later)
 *   - pointer keys (may come later)
 *   - key redirects (unlikely)
 *   - messages (very unlikely)
 */

#include "map.h"

struct xkb_filter {
    union xkb_action action;
    const struct xkb_key *key;
    uint32_t priv;
    int (*func)(struct xkb_state *state,
                struct xkb_filter *filter,
                const struct xkb_key *key,
                enum xkb_key_direction direction);
    int refcnt;
};

struct xkb_state {
    xkb_group_index_t base_group; /**< depressed */
    xkb_group_index_t latched_group;
    xkb_group_index_t locked_group;
    xkb_group_index_t group; /**< effective */

    xkb_mod_mask_t base_mods; /**< depressed */
    xkb_mod_mask_t latched_mods;
    xkb_mod_mask_t locked_mods;
    xkb_mod_mask_t mods; /**< effective */

    /*
     * At each event, we accumulate all the needed modifications to the base
     * modifiers, and apply them at the end. These keep track of this state.
     */
    xkb_mod_mask_t set_mods;
    xkb_mod_mask_t clear_mods;
    /*
     * We mustn't clear a base modifier if there's another depressed key
     * which affects it, e.g. given this sequence
     * < Left Shift down, Right Shift down, Left Shift Up >
     * the modifier should still be set. This keeps the count.
     */
    int16_t mod_key_count[sizeof(xkb_mod_mask_t) * 8];

    uint32_t leds;

    int refcnt;
    darray(struct xkb_filter) filters;
    struct xkb_keymap *keymap;
};

static const union xkb_action fake = { .type = ACTION_TYPE_NONE };

static const union xkb_action *
xkb_key_get_action(struct xkb_state *state, const struct xkb_key *key)
{
    xkb_layout_index_t layout;
    xkb_level_index_t level;

    if (!key->actions)
        return &fake;

    layout = xkb_state_key_get_layout(state, key->keycode);
    if (layout == XKB_LAYOUT_INVALID)
        return &fake;

    level = xkb_state_key_get_level(state, key->keycode, layout);
    if (level == XKB_LEVEL_INVALID)
        return &fake;

    return XkbKeyActionEntry(key, layout, level);
}

static struct xkb_filter *
xkb_filter_new(struct xkb_state *state)
{
    struct xkb_filter *filter = NULL, *iter;

    darray_foreach(iter, state->filters) {
        if (iter->func)
            continue;
        filter = iter;
        break;
    }

    if (!filter) {
        darray_resize0(state->filters, darray_size(state->filters) + 1);
        filter = &darray_item(state->filters, darray_size(state->filters) -1);
    }

    filter->refcnt = 1;
    return filter;
}

/***====================================================================***/

static int
xkb_filter_group_set_func(struct xkb_state *state,
                          struct xkb_filter *filter,
                          const struct xkb_key *key,
                          enum xkb_key_direction direction)
{
    if (key != filter->key) {
        filter->action.group.flags &= ~ACTION_LOCK_CLEAR;
        return 1;
    }

    if (direction == XKB_KEY_DOWN) {
        filter->refcnt++;
        return 0;
    }
    else if (--filter->refcnt > 0) {
        return 0;
    }

    state->base_group = filter->priv;

    if (filter->action.group.flags & ACTION_LOCK_CLEAR)
        state->locked_group = 0;

    filter->func = NULL;
    return 1;
}

static void
xkb_filter_group_set_new(struct xkb_state *state, struct xkb_filter *filter)
{
    filter->priv = state->base_group;
    if (filter->action.group.flags & ACTION_ABSOLUTE_SWITCH)
        state->base_group = filter->action.group.group;
    else
        state->base_group += filter->action.group.group;
}

static int
xkb_filter_group_lock_func(struct xkb_state *state,
                           struct xkb_filter *filter,
                           const struct xkb_key *key,
                           enum xkb_key_direction direction)
{
    if (key != filter->key)
        return 1;

    if (direction == XKB_KEY_DOWN) {
        filter->refcnt++;
        return 0;
    }
    if (--filter->refcnt > 0)
        return 0;

    filter->func = NULL;
    return 1;
}

static void
xkb_filter_group_lock_new(struct xkb_state *state, struct xkb_filter *filter)
{
    if (filter->action.group.flags & ACTION_ABSOLUTE_SWITCH)
        state->locked_group = filter->action.group.group;
    else
        state->locked_group += filter->action.group.group;
}

static int
xkb_filter_mod_set_func(struct xkb_state *state,
                        struct xkb_filter *filter,
                        const struct xkb_key *key,
                        enum xkb_key_direction direction)
{
    if (key != filter->key) {
        filter->action.mods.flags &= ~ACTION_LOCK_CLEAR;
        return 1;
    }

    if (direction == XKB_KEY_DOWN) {
        filter->refcnt++;
        return 0;
    }
    else if (--filter->refcnt > 0) {
        return 0;
    }

    state->clear_mods = filter->action.mods.mods.mask;
    if (filter->action.mods.flags & ACTION_LOCK_CLEAR)
        state->locked_mods &= ~filter->action.mods.mods.mask;

    filter->func = NULL;
    return 1;
}

static void
xkb_filter_mod_set_new(struct xkb_state *state, struct xkb_filter *filter)
{
    state->set_mods = filter->action.mods.mods.mask;
}

static int
xkb_filter_mod_lock_func(struct xkb_state *state,
                         struct xkb_filter *filter,
                         const struct xkb_key *key,
                         enum xkb_key_direction direction)
{
    if (key != filter->key)
        return 1;

    if (direction == XKB_KEY_DOWN) {
        filter->refcnt++;
        return 0;
    }
    if (--filter->refcnt > 0)
        return 0;

    state->clear_mods |= filter->action.mods.mods.mask;
    if (!(filter->action.mods.flags & ACTION_LOCK_NO_UNLOCK))
        state->locked_mods &= ~filter->priv;

    filter->func = NULL;
    return 1;
}

static void
xkb_filter_mod_lock_new(struct xkb_state *state, struct xkb_filter *filter)
{
    filter->priv = state->locked_mods & filter->action.mods.mods.mask;
    state->set_mods |= filter->action.mods.mods.mask;
    if (!(filter->action.mods.flags & ACTION_LOCK_NO_LOCK))
        state->locked_mods |= filter->action.mods.mods.mask;
}

enum xkb_key_latch_state {
    NO_LATCH,
    LATCH_KEY_DOWN,
    LATCH_PENDING,
};

static bool
xkb_action_breaks_latch(const union xkb_action *action)
{
    switch (action->type) {
    case ACTION_TYPE_NONE:
    case ACTION_TYPE_PTR_BUTTON:
    case ACTION_TYPE_PTR_LOCK:
    case ACTION_TYPE_CTRL_SET:
    case ACTION_TYPE_CTRL_LOCK:
    case ACTION_TYPE_KEY_REDIRECT:
    case ACTION_TYPE_SWITCH_VT:
    case ACTION_TYPE_TERMINATE:
        return true;
    default:
        return false;
    }
}

static int
xkb_filter_mod_latch_func(struct xkb_state *state,
                          struct xkb_filter *filter,
                          const struct xkb_key *key,
                          enum xkb_key_direction direction)
{
    enum xkb_key_latch_state latch = filter->priv;

    if (direction == XKB_KEY_DOWN && latch == LATCH_PENDING) {
        /* If this is a new keypress and we're awaiting our single latched
         * keypress, then either break the latch if any random key is pressed,
         * or promote it to a lock or plain base set if it's the same
         * modifier. */
        const union xkb_action *action = xkb_key_get_action(state, key);
        if (action->type == ACTION_TYPE_MOD_LATCH &&
            action->mods.flags == filter->action.mods.flags &&
            action->mods.mods.mask == filter->action.mods.mods.mask) {
            filter->action = *action;
            if (filter->action.mods.flags & ACTION_LATCH_TO_LOCK) {
                filter->action.type = ACTION_TYPE_MOD_LOCK;
                filter->func = xkb_filter_mod_lock_func;
                state->locked_mods |= filter->action.mods.mods.mask;
            }
            else {
                filter->action.type = ACTION_TYPE_MOD_SET;
                filter->func = xkb_filter_mod_set_func;
                state->set_mods = filter->action.mods.mods.mask;
            }
            filter->key = key;
            state->latched_mods &= ~filter->action.mods.mods.mask;
            /* XXX beep beep! */
            return 0;
        }
        else if (xkb_action_breaks_latch(action)) {
            /* XXX: This may be totally broken, we might need to break the
             *      latch in the next run after this press? */
            state->latched_mods &= ~filter->action.mods.mods.mask;
            filter->func = NULL;
            return 1;
        }
    }
    else if (direction == XKB_KEY_UP && key == filter->key) {
        /* Our key got released.  If we've set it to clear locks, and we
         * currently have the same modifiers locked, then release them and
         * don't actually latch.  Else we've actually hit the latching
         * stage, so set PENDING and move our modifier from base to
         * latched. */
        if (latch == NO_LATCH ||
            ((filter->action.mods.flags & ACTION_LOCK_CLEAR) &&
             (state->locked_mods & filter->action.mods.mods.mask) ==
             filter->action.mods.mods.mask)) {
            /* XXX: We might be a bit overenthusiastic about clearing
             *      mods other filters have set here? */
            if (latch == LATCH_PENDING)
                state->latched_mods &= ~filter->action.mods.mods.mask;
            else
                state->clear_mods = filter->action.mods.mods.mask;
            state->locked_mods &= ~filter->action.mods.mods.mask;
            filter->func = NULL;
        }
        else {
            latch = LATCH_PENDING;
            state->clear_mods = filter->action.mods.mods.mask;
            state->latched_mods |= filter->action.mods.mods.mask;
            /* XXX beep beep! */
        }
    }
    else if (direction == XKB_KEY_DOWN && latch == LATCH_KEY_DOWN) {
        /* Someone's pressed another key while we've still got the latching
         * key held down, so keep the base modifier state active (from
         * xkb_filter_mod_latch_new), but don't trip the latch, just clear
         * it as soon as the modifier gets released. */
        latch = NO_LATCH;
    }

    filter->priv = latch;

    return 1;
}

static void
xkb_filter_mod_latch_new(struct xkb_state *state, struct xkb_filter *filter)
{
    filter->priv = LATCH_KEY_DOWN;
    state->set_mods = filter->action.mods.mods.mask;
}

static const struct {
    void (*new)(struct xkb_state *state, struct xkb_filter *filter);
    int (*func)(struct xkb_state *state, struct xkb_filter *filter,
                const struct xkb_key *key, enum xkb_key_direction direction);
} filter_action_funcs[_ACTION_TYPE_NUM_ENTRIES] = {
    [ACTION_TYPE_MOD_SET]    = { xkb_filter_mod_set_new,
                                 xkb_filter_mod_set_func },
    [ACTION_TYPE_MOD_LATCH]  = { xkb_filter_mod_latch_new,
                                 xkb_filter_mod_latch_func },
    [ACTION_TYPE_MOD_LOCK]   = { xkb_filter_mod_lock_new,
                                 xkb_filter_mod_lock_func },
    [ACTION_TYPE_GROUP_SET]  = { xkb_filter_group_set_new,
                                 xkb_filter_group_set_func },
    [ACTION_TYPE_GROUP_LOCK] = { xkb_filter_group_lock_new,
                                 xkb_filter_group_lock_func },
};

/**
 * Applies any relevant filters to the key, first from the list of filters
 * that are currently active, then if no filter has claimed the key, possibly
 * apply a new filter from the key action.
 */
static void
xkb_filter_apply_all(struct xkb_state *state,
                     const struct xkb_key *key,
                     enum xkb_key_direction direction)
{
    struct xkb_filter *filter;
    const union xkb_action *action;
    int send = 1;

    /* First run through all the currently active filters and see if any of
     * them have claimed this event. */
    darray_foreach(filter, state->filters) {
        if (!filter->func)
            continue;
        send &= filter->func(state, filter, key, direction);
    }

    if (!send || direction == XKB_KEY_UP)
        return;

    action = xkb_key_get_action(state, key);
    if (!filter_action_funcs[action->type].new)
        return;

    filter = xkb_filter_new(state);
    if (!filter)
        return; /* WSGO */

    filter->key = key;
    filter->func = filter_action_funcs[action->type].func;
    filter->action = *action;
    filter_action_funcs[action->type].new(state, filter);
}

XKB_EXPORT struct xkb_state *
xkb_state_new(struct xkb_keymap *keymap)
{
    struct xkb_state *ret;

    ret = calloc(sizeof(*ret), 1);
    if (!ret)
        return NULL;

    ret->refcnt = 1;
    ret->keymap = xkb_map_ref(keymap);

    return ret;
}

XKB_EXPORT struct xkb_state *
xkb_state_ref(struct xkb_state *state)
{
    state->refcnt++;
    return state;
}

XKB_EXPORT void
xkb_state_unref(struct xkb_state *state)
{
    if (--state->refcnt > 0)
        return;

    xkb_map_unref(state->keymap);
    darray_free(state->filters);
    free(state);
}

XKB_EXPORT struct xkb_keymap *
xkb_state_get_map(struct xkb_state *state)
{
    return state->keymap;
}

/**
 * Update the LED state to match the rest of the xkb_state.
 */
static void
xkb_state_led_update_all(struct xkb_state *state)
{
    xkb_led_index_t led;

    state->leds = 0;

    for (led = 0; led < XKB_NUM_INDICATORS; led++) {
        struct xkb_indicator_map *map = &state->keymap->indicators[led];
        xkb_mod_mask_t mod_mask = 0;
        uint32_t group_mask = 0;

        if (map->which_mods & XKB_STATE_DEPRESSED)
            mod_mask |= state->base_mods;
        if (map->which_mods & XKB_STATE_LATCHED)
            mod_mask |= state->latched_mods;
        if (map->which_mods & XKB_STATE_LOCKED)
            mod_mask |= state->locked_mods;
        if ((map->mods.mask & mod_mask))
            state->leds |= (1 << led);

        if (map->which_groups & XKB_STATE_DEPRESSED)
            group_mask |= (1 << state->base_group);
        if (map->which_groups & XKB_STATE_LATCHED)
            group_mask |= (1 << state->latched_group);
        if (map->which_groups & XKB_STATE_LOCKED)
            group_mask |= (1 << state->locked_group);
        if ((map->groups & group_mask))
            state->leds |= (1 << led);

        if (map->ctrls) {
            if ((map->ctrls & state->keymap->enabled_ctrls))
                state->leds |= (1 << led);
        }
    }
}

/**
 * Calculates the derived state (effective mods/group and LEDs) from an
 * up-to-date xkb_state.
 */
static void
xkb_state_update_derived(struct xkb_state *state)
{
    state->mods =
        (state->base_mods | state->latched_mods | state->locked_mods);
    /* FIXME: Clamp/wrap locked_group */
    state->group = state->locked_group + state->base_group +
                   state->latched_group;
    /* FIXME: Clamp/wrap effective group */

    xkb_state_led_update_all(state);
}

/**
 * Given a particular key event, updates the state structure to reflect the
 * new modifiers.
 */
XKB_EXPORT void
xkb_state_update_key(struct xkb_state *state, xkb_keycode_t kc,
                     enum xkb_key_direction direction)
{
    xkb_mod_index_t i;
    xkb_mod_mask_t bit;
    const struct xkb_key *key = XkbKey(state->keymap, kc);
    if (!key)
        return;

    state->set_mods = 0;
    state->clear_mods = 0;

    xkb_filter_apply_all(state, key, direction);

    for (i = 0, bit = 1; state->set_mods; i++, bit <<= 1) {
        if (state->set_mods & bit) {
            state->mod_key_count[i]++;
            state->base_mods |= bit;
            state->set_mods &= ~bit;
        }
    }

    for (i = 0, bit = 1; state->clear_mods; i++, bit <<= 1) {
        if (state->clear_mods & bit) {
            state->mod_key_count[i]--;
            if (state->mod_key_count[i] <= 0) {
                state->base_mods &= ~bit;
                state->mod_key_count[i] = 0;
            }
            state->clear_mods &= ~bit;
        }
    }

    xkb_state_update_derived(state);
}

/**
 * Updates the state from a set of explicit masks as gained from
 * xkb_state_serialize_mods and xkb_state_serialize_groups.  As noted in the
 * documentation for these functions in xkbcommon.h, this round-trip is
 * lossy, and should only be used to update a slave state mirroring the
 * master, e.g. in a client/server window system.
 */
XKB_EXPORT void
xkb_state_update_mask(struct xkb_state *state,
                      xkb_mod_mask_t base_mods,
                      xkb_mod_mask_t latched_mods,
                      xkb_mod_mask_t locked_mods,
                      xkb_group_index_t base_group,
                      xkb_group_index_t latched_group,
                      xkb_group_index_t locked_group)
{
    xkb_mod_index_t num_mods;
    xkb_mod_index_t idx;

    state->base_mods = 0;
    state->latched_mods = 0;
    state->locked_mods = 0;
    num_mods = xkb_map_num_mods(state->keymap);

    for (idx = 0; idx < num_mods; idx++) {
        xkb_mod_mask_t mod = (1 << idx);
        if (base_mods & mod)
            state->base_mods |= mod;
        if (latched_mods & mod)
            state->latched_mods |= mod;
        if (locked_mods & mod)
            state->locked_mods |= mod;
    }

    state->base_group = base_group;
    state->latched_group = latched_group;
    state->locked_group = locked_group;

    xkb_state_update_derived(state);
}

/**
 * Serialises the requested modifier state into an xkb_mod_mask_t, with all
 * the same disclaimers as in xkb_state_update_mask.
 */
XKB_EXPORT xkb_mod_mask_t
xkb_state_serialize_mods(struct xkb_state *state,
                         enum xkb_state_component type)
{
    xkb_mod_mask_t ret = 0;

    if (type == XKB_STATE_EFFECTIVE)
        return state->mods;

    if (type & XKB_STATE_DEPRESSED)
        ret |= state->base_mods;
    if (type & XKB_STATE_LATCHED)
        ret |= state->latched_mods;
    if (type & XKB_STATE_LOCKED)
        ret |= state->locked_mods;

    return ret;
}

/**
 * Serialises the requested group state, with all the same disclaimers as
 * in xkb_state_update_mask.
 */
XKB_EXPORT xkb_group_index_t
xkb_state_serialize_group(struct xkb_state *state,
                          enum xkb_state_component type)
{
    xkb_group_index_t ret = 0;

    if (type == XKB_STATE_EFFECTIVE)
        return state->group;

    if (type & XKB_STATE_DEPRESSED)
        ret += state->base_group;
    if (type & XKB_STATE_LATCHED)
        ret += state->latched_group;
    if (type & XKB_STATE_LOCKED)
        ret += state->locked_group;

    return ret;
}

/**
 * Returns 1 if the given modifier is active with the specified type(s), 0 if
 * not, or -1 if the modifier is invalid.
 */
XKB_EXPORT int
xkb_state_mod_index_is_active(struct xkb_state *state,
                              xkb_mod_index_t idx,
                              enum xkb_state_component type)
{
    int ret = 0;

    if (idx >= xkb_map_num_mods(state->keymap))
        return -1;

    if (type & XKB_STATE_DEPRESSED)
        ret |= (state->base_mods & (1 << idx));
    if (type & XKB_STATE_LATCHED)
        ret |= (state->latched_mods & (1 << idx));
    if (type & XKB_STATE_LOCKED)
        ret |= (state->locked_mods & (1 << idx));

    return !!ret;
}

/**
 * Helper function for xkb_state_mod_indices_are_active and
 * xkb_state_mod_names_are_active.
 */
static int
match_mod_masks(struct xkb_state *state, enum xkb_state_match match,
                uint32_t wanted)
{
    uint32_t active = xkb_state_serialize_mods(state, XKB_STATE_EFFECTIVE);

    if (!(match & XKB_STATE_MATCH_NON_EXCLUSIVE) && (active & ~wanted))
        return 0;

    if (match & XKB_STATE_MATCH_ANY)
        return !!(active & wanted);
    else
        return (active & wanted) == wanted;

    return 0;
}

/**
 * Returns 1 if the modifiers are active with the specified type(s), 0 if
 * not, or -1 if any of the modifiers are invalid.
 */
XKB_EXPORT int
xkb_state_mod_indices_are_active(struct xkb_state *state,
                                 enum xkb_state_component type,
                                 enum xkb_state_match match,
                                 ...)
{
    va_list ap;
    xkb_mod_index_t idx = 0;
    uint32_t wanted = 0;
    int ret = 0;
    xkb_mod_index_t num_mods = xkb_map_num_mods(state->keymap);

    va_start(ap, match);
    while (1) {
        idx = va_arg(ap, xkb_mod_index_t);
        if (idx == XKB_MOD_INVALID)
            break;
        if (idx >= num_mods) {
            ret = -1;
            break;
        }
        wanted |= (1 << idx);
    }
    va_end(ap);

    if (ret == -1)
        return ret;

    return match_mod_masks(state, match, wanted);
}

/**
 * Returns 1 if the given modifier is active with the specified type(s), 0 if
 * not, or -1 if the modifier is invalid.
 */
XKB_EXPORT int
xkb_state_mod_name_is_active(struct xkb_state *state, const char *name,
                             enum xkb_state_component type)
{
    xkb_mod_index_t idx = xkb_map_mod_get_index(state->keymap, name);

    if (idx == XKB_MOD_INVALID)
        return -1;

    return xkb_state_mod_index_is_active(state, idx, type);
}

/**
 * Returns 1 if the modifiers are active with the specified type(s), 0 if
 * not, or -1 if any of the modifiers are invalid.
 */
XKB_EXPORT ATTR_NULL_SENTINEL int
xkb_state_mod_names_are_active(struct xkb_state *state,
                               enum xkb_state_component type,
                               enum xkb_state_match match,
                               ...)
{
    va_list ap;
    xkb_mod_index_t idx = 0;
    const char *str;
    uint32_t wanted = 0;
    int ret = 0;

    va_start(ap, match);
    while (1) {
        str = va_arg(ap, const char *);
        if (str == NULL)
            break;
        idx = xkb_map_mod_get_index(state->keymap, str);
        if (idx == XKB_MOD_INVALID) {
            ret = -1;
            break;
        }
        wanted |= (1 << idx);
    }
    va_end(ap);

    if (ret == -1)
        return ret;

    return match_mod_masks(state, match, wanted);
}

/**
 * Returns 1 if the given group is active with the specified type(s), 0 if
 * not, or -1 if the group is invalid.
 */
XKB_EXPORT int
xkb_state_group_index_is_active(struct xkb_state *state,
                                xkb_group_index_t idx,
                                enum xkb_state_component type)
{
    int ret = 0;

    if (idx >= xkb_map_num_groups(state->keymap))
        return -1;

    if (type & XKB_STATE_DEPRESSED)
        ret |= (state->base_group == idx);
    if (type & XKB_STATE_LATCHED)
        ret |= (state->latched_group == idx);
    if (type & XKB_STATE_LOCKED)
        ret |= (state->locked_group == idx);

    return ret;
}

/**
 * Returns 1 if the given modifier is active with the specified type(s), 0 if
 * not, or -1 if the modifier is invalid.
 */
XKB_EXPORT int
xkb_state_group_name_is_active(struct xkb_state *state, const char *name,
                               enum xkb_state_component type)
{
    xkb_group_index_t idx = xkb_map_group_get_index(state->keymap, name);

    if (idx == XKB_GROUP_INVALID)
        return -1;

    return xkb_state_group_index_is_active(state, idx, type);
}

/**
 * Returns 1 if the given LED is active, 0 if not, or -1 if the LED is invalid.
 */
XKB_EXPORT int
xkb_state_led_index_is_active(struct xkb_state *state, xkb_led_index_t idx)
{
    if (idx >= xkb_map_num_leds(state->keymap))
        return -1;

    return !!(state->leds & (1 << idx));
}

/**
 * Returns 1 if the given LED is active, 0 if not, or -1 if the LED is invalid.
 */
XKB_EXPORT int
xkb_state_led_name_is_active(struct xkb_state *state, const char *name)
{
    xkb_led_index_t idx = xkb_map_led_get_index(state->keymap, name);

    if (idx == XKB_LED_INVALID)
        return -1;

    return xkb_state_led_index_is_active(state, idx);
}
