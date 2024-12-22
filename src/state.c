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

/*
 * This is a bastardised version of xkbActions.c from the X server which
 * does not support, for the moment:
 *   - AccessX sticky/debounce/etc (will come later)
 *   - pointer keys (may come later)
 *   - key redirects (unlikely)
 *   - messages (very unlikely)
 */

#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "keymap.h"
#include "keysym.h"
#include "utf8.h"

struct xkb_filter {
    union xkb_action action;
    const struct xkb_key *key;
    uint32_t priv;
    bool (*func)(struct xkb_state *state,
                 struct xkb_filter *filter,
                 const struct xkb_key *key,
                 enum xkb_key_direction direction);
    int refcnt;
};

struct state_components {
    /* These may be negative, because of -1 group actions. */
    int32_t base_group; /**< depressed */
    int32_t latched_group;
    int32_t locked_group;
    xkb_layout_index_t group; /**< effective */

    xkb_mod_mask_t base_mods; /**< depressed */
    xkb_mod_mask_t latched_mods;
    xkb_mod_mask_t locked_mods;
    xkb_mod_mask_t mods; /**< effective */

    xkb_led_mask_t leds;
};

struct xkb_state {
    /*
     * Before updating the state, we keep a copy of just this struct. This
     * allows us to report which components of the state have changed.
     */
    struct state_components components;

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
    int16_t mod_key_count[XKB_MAX_MODS];

    int refcnt;
    darray(struct xkb_filter) filters;
    struct xkb_keymap *keymap;
};

static const struct xkb_key_type_entry *
get_entry_for_mods(const struct xkb_key_type *type, xkb_mod_mask_t mods)
{
    for (unsigned i = 0; i < type->num_entries; i++)
        if (entry_is_active(&type->entries[i]) &&
            type->entries[i].mods.mask == mods)
            return &type->entries[i];
    return NULL;
}

static const struct xkb_key_type_entry *
get_entry_for_key_state(struct xkb_state *state, const struct xkb_key *key,
                        xkb_layout_index_t group)
{
    const struct xkb_key_type *type = key->groups[group].type;
    xkb_mod_mask_t active_mods = state->components.mods & type->mods.mask;
    return get_entry_for_mods(type, active_mods);
}

/**
 * Returns the level to use for the given key and state, or
 * XKB_LEVEL_INVALID.
 */
xkb_level_index_t
xkb_state_key_get_level(struct xkb_state *state, xkb_keycode_t kc,
                        xkb_layout_index_t layout)
{
    const struct xkb_key *key = XkbKey(state->keymap, kc);
    const struct xkb_key_type_entry *entry;

    if (!key || layout >= key->num_groups)
        return XKB_LEVEL_INVALID;

    /* If we don't find an explicit match the default is 0. */
    entry = get_entry_for_key_state(state, key, layout);
    if (!entry)
        return 0;

    return entry->level;
}

static xkb_layout_index_t
xkb_state_xkb_key_get_layout(struct xkb_state *state, const struct xkb_key *key) {
    static_assert(XKB_MAX_GROUPS < INT32_MAX, "Max groups don't fit");
    return XkbWrapGroupIntoRange((int32_t) state->components.group,
                                 key->num_groups,
                                 key->out_of_range_group_action,
                                 key->out_of_range_group_number);
}

/**
 * Returns the layout to use for the given key and state, taking
 * wrapping/clamping/etc into account, or XKB_LAYOUT_INVALID.
 */
xkb_layout_index_t
xkb_state_key_get_layout(struct xkb_state *state, xkb_keycode_t kc)
{
    const struct xkb_key *key = XkbKey(state->keymap, kc);

    if (!key)
        return XKB_LAYOUT_INVALID;

    return xkb_state_xkb_key_get_layout(state, key);
}

/* Empty action used for empty levels */
static const union xkb_action dummy_action = { .type = ACTION_TYPE_NONE };

static unsigned int
xkb_key_get_actions(struct xkb_state *state, const struct xkb_key *key,
                    const union xkb_action **actions)
{
    const xkb_layout_index_t layout =
        xkb_state_key_get_layout(state, key->keycode);
    if (layout == XKB_LAYOUT_INVALID)
        goto err;

    const xkb_level_index_t level =
        xkb_state_key_get_level(state, key->keycode, layout);
    if (level == XKB_LEVEL_INVALID)
        goto err;

    const unsigned int count =
        xkb_keymap_key_get_actions_by_level(state->keymap, key->keycode,
                                            layout, level, actions);
    if (!count)
        goto err;

    return count;

err:
    /* Use a dummy action if no corresponding level was found or if it is empty.
     * This is required e.g. to handle latches properly. */
    *actions = &dummy_action;
    return 1;
}

static struct xkb_filter *
xkb_filter_new(struct xkb_state *state)
{
    struct xkb_filter *filter = NULL, *iter;

    darray_foreach(iter, state->filters) {
        if (iter->func)
            continue;
        /* Use available slot */
        filter = iter;
        break;
    }

    if (!filter) {
        /* No available slot: resize the filters array */
        darray_resize0(state->filters, darray_size(state->filters) + 1);
        filter = &darray_item(state->filters, darray_size(state->filters) -1);
    }

    filter->refcnt = 1;
    return filter;
}

/***====================================================================***/

enum xkb_filter_result {
    /*
     * The event is consumed by the filters.
     *
     * An event is always processed by all filters, but any filter can
     * prevent it from being processed further by consuming it.
     */
    XKB_FILTER_CONSUME,
    /*
     * The event may continue to be processed as far as this filter is
     * concerned.
     */
    XKB_FILTER_CONTINUE,
};

/* Modify a group component, depending on the ACTION_ABSOLUTE_SWITCH flag */
#define apply_group_delta(filter_, state_, component_)                   \
    if ((filter_)->action.group.flags & ACTION_ABSOLUTE_SWITCH)          \
        (state_)->components.component_ = (filter_)->action.group.group; \
    else                                                                 \
        (state_)->components.component_ += (filter_)->action.group.group

static void
xkb_filter_group_set_new(struct xkb_state *state, struct xkb_filter *filter)
{
    static_assert(sizeof(state->components.base_group) == sizeof(filter->priv),
                  "Max groups don't fit");
    filter->priv = state->components.base_group;
    apply_group_delta(filter, state, base_group);
}

static bool
xkb_filter_group_set_func(struct xkb_state *state,
                          struct xkb_filter *filter,
                          const struct xkb_key *key,
                          enum xkb_key_direction direction)
{
    if (key != filter->key) {
        filter->action.group.flags &= ~ACTION_LOCK_CLEAR;
        return XKB_FILTER_CONTINUE;
    }

    if (direction == XKB_KEY_DOWN) {
        filter->refcnt++;
        return XKB_FILTER_CONSUME;
    }
    else if (--filter->refcnt > 0) {
        return XKB_FILTER_CONSUME;
    }

    static_assert(sizeof(state->components.base_group) == sizeof(filter->priv),
                  "Max groups don't fit");
    state->components.base_group = (int32_t) filter->priv;

    if (filter->action.group.flags & ACTION_LOCK_CLEAR)
        state->components.locked_group = 0;

    filter->func = NULL;
    return XKB_FILTER_CONTINUE;
}

static void
xkb_filter_group_lock_new(struct xkb_state *state, struct xkb_filter *filter)
{
    apply_group_delta(filter, state, locked_group);
}

static bool
xkb_filter_group_lock_func(struct xkb_state *state,
                           struct xkb_filter *filter,
                           const struct xkb_key *key,
                           enum xkb_key_direction direction)
{
    if (key != filter->key)
        return XKB_FILTER_CONTINUE;

    if (direction == XKB_KEY_DOWN) {
        filter->refcnt++;
        return XKB_FILTER_CONSUME;
    }
    if (--filter->refcnt > 0)
        return XKB_FILTER_CONSUME;

    filter->func = NULL;
    return XKB_FILTER_CONTINUE;
}

/* Mod latches have additional break conditions not handled by this function */
static bool
xkb_action_breaks_latch(const union xkb_action *action)
{
    switch (action->type) {
    case ACTION_TYPE_NONE:
    case ACTION_TYPE_MOD_LOCK:
    case ACTION_TYPE_GROUP_LOCK:
    case ACTION_TYPE_PTR_MOVE:
    case ACTION_TYPE_PTR_BUTTON:
    case ACTION_TYPE_PTR_LOCK:
    case ACTION_TYPE_PTR_DEFAULT:
    case ACTION_TYPE_TERMINATE:
    case ACTION_TYPE_SWITCH_VT:
    case ACTION_TYPE_CTRL_SET:
    case ACTION_TYPE_CTRL_LOCK:
        return true;
    default:
        return false;
    }
}

enum xkb_key_latch_state {
    NO_LATCH = 0,
    LATCH_KEY_DOWN,
    LATCH_PENDING,
    _KEY_LATCH_STATE_NUM_ENTRIES
};

#define MAX_XKB_KEY_LATCH_STATE_LOG2 2
#if (_KEY_LATCH_STATE_NUM_ENTRIES > (1 << MAX_XKB_KEY_LATCH_STATE_LOG2)) || \
    (-XKB_MAX_GROUPS) < (INT32_MIN >> MAX_XKB_KEY_LATCH_STATE_LOG2) || \
      XKB_MAX_GROUPS > (INT32_MAX >> MAX_XKB_KEY_LATCH_STATE_LOG2)
#error "Cannot represent priv field of the group latch filter"
#endif

/* Hold the latch state *and* the group delta */
union group_latch_priv {
    uint32_t priv;
    struct {
        /* The type is really: enum xkb_key_latch_state, but it is problematic
         * on Windows, because it is interpreted as signed and leads to wrong
         * negative values. */
        unsigned int latch:MAX_XKB_KEY_LATCH_STATE_LOG2;
        int32_t group_delta:(32 - MAX_XKB_KEY_LATCH_STATE_LOG2);
    };
};

static void
xkb_filter_group_latch_new(struct xkb_state *state, struct xkb_filter *filter)
{
    const union group_latch_priv priv = {
        .latch = LATCH_KEY_DOWN,
        .group_delta = (filter->action.group.flags & ACTION_ABSOLUTE_SWITCH)
            ? filter->action.group.group - state->components.base_group
            : filter->action.group.group
    };
    filter->priv = priv.priv;
    /* Like group set */
    apply_group_delta(filter, state, base_group);
}

static bool
xkb_filter_group_latch_func(struct xkb_state *state,
                            struct xkb_filter *filter,
                            const struct xkb_key *key,
                            enum xkb_key_direction direction)
{
    union group_latch_priv priv = {.priv = filter->priv};
    enum xkb_key_latch_state latch = priv.latch;

    if (direction == XKB_KEY_DOWN) {
        const union xkb_action *actions = NULL;
        unsigned int count = xkb_key_get_actions(state, key, &actions);
        if (latch == LATCH_PENDING) {
            /* If this is a new keypress and we're awaiting our single latched
             * keypress, then either break the latch if any random key is pressed,
             * or promote it to a lock if it's the same group delta & flags and
             * latchToLock option is enabled. */
            for (unsigned int k = 0; k < count; k++) {
                if (actions[k].type == ACTION_TYPE_GROUP_LATCH &&
                    actions[k].group.group == filter->action.group.group &&
                    actions[k].group.flags == filter->action.group.flags) {
                    filter->action = actions[k];
                    if (filter->action.group.flags & ACTION_LATCH_TO_LOCK &&
                        filter->action.group.group != 0) {
                        /* Promote to lock */
                        filter->action.type = ACTION_TYPE_GROUP_LOCK;
                        filter->func = xkb_filter_group_lock_func;
                        xkb_filter_group_lock_new(state, filter);
                        state->components.latched_group -= priv.group_delta;
                        filter->key = key;
                        /* XXX beep beep! */
                        return XKB_FILTER_CONSUME;
                    }
                    /* Do nothing if latchToLock option is not activated; if the
                     * latch is not broken by the following actions and the key is
                     * not consumed, then another latch filter will be created.
                     */
                    continue;
                }
                else if (xkb_action_breaks_latch(&(actions[k]))) {
                    /* Breaks the latch */
                    state->components.latched_group -= priv.group_delta;
                    filter->func = NULL;
                    return XKB_FILTER_CONTINUE;
                }
            }
        }
        else if (latch == LATCH_KEY_DOWN) {
            /* Another key was pressed while we've still got the latching
            * key held down, so keep the base group active (from
            * xkb_filter_group_latch_new), but don't trip the latch Just clear
            * it as soon as the group key gets released, but only if the action
            * is eligible to break latches. */
            for (unsigned int k = 0; k < count; k++) {
                if (xkb_action_breaks_latch(&(actions[k]))) {
                    latch = NO_LATCH;
                    break;
                }
            }
        }
    }
    else if (direction == XKB_KEY_UP && key == filter->key) {
        /* Our key got released.  If we've set it to clear locks, and we
         * currently have a group locked, then release it and
         * don't actually latch.  Else we've actually hit the latching
         * stage, so set PENDING and move our group from base to
         * latched. */
        if (latch == NO_LATCH ||
            ((filter->action.group.flags & ACTION_LOCK_CLEAR) &&
             state->components.locked_group)) {
            if (latch == LATCH_PENDING)
                state->components.latched_group -= priv.group_delta;
            else
                state->components.base_group -= priv.group_delta;
            if (filter->action.group.flags & ACTION_LOCK_CLEAR)
                state->components.locked_group = 0;
            filter->func = NULL;
        }
        /* We may already have reached the latch state if pressing the
         * key multiple times without latch-to-lock enabled. */
        else if (latch == LATCH_KEY_DOWN) {
            latch = LATCH_PENDING;
            /* Switch from set to latch */
            state->components.base_group -= priv.group_delta;
            state->components.latched_group += priv.group_delta;
            /* XXX beep beep! */
        }
    }

    priv.latch = latch;
    filter->priv = priv.priv;

    return XKB_FILTER_CONTINUE;
}

static void
xkb_filter_mod_set_new(struct xkb_state *state, struct xkb_filter *filter)
{
    state->set_mods |= filter->action.mods.mods.mask;
}

static bool
xkb_filter_mod_set_func(struct xkb_state *state,
                        struct xkb_filter *filter,
                        const struct xkb_key *key,
                        enum xkb_key_direction direction)
{
    if (key != filter->key) {
        filter->action.mods.flags &= ~ACTION_LOCK_CLEAR;
        return XKB_FILTER_CONTINUE;
    }

    if (direction == XKB_KEY_DOWN) {
        filter->refcnt++;
        return XKB_FILTER_CONSUME;
    }
    else if (--filter->refcnt > 0) {
        return XKB_FILTER_CONSUME;
    }

    state->clear_mods |= filter->action.mods.mods.mask;
    if (filter->action.mods.flags & ACTION_LOCK_CLEAR)
        state->components.locked_mods &= ~filter->action.mods.mods.mask;

    filter->func = NULL;
    return XKB_FILTER_CONTINUE;
}

static void
xkb_filter_mod_lock_new(struct xkb_state *state, struct xkb_filter *filter)
{
    filter->priv = (state->components.locked_mods &
                    filter->action.mods.mods.mask);
    state->set_mods |= filter->action.mods.mods.mask;
    if (!(filter->action.mods.flags & ACTION_LOCK_NO_LOCK))
        state->components.locked_mods |= filter->action.mods.mods.mask;
}

static bool
xkb_filter_mod_lock_func(struct xkb_state *state,
                         struct xkb_filter *filter,
                         const struct xkb_key *key,
                         enum xkb_key_direction direction)
{
    if (key != filter->key)
        return XKB_FILTER_CONTINUE;

    if (direction == XKB_KEY_DOWN) {
        filter->refcnt++;
        return XKB_FILTER_CONSUME;
    }
    if (--filter->refcnt > 0)
        return XKB_FILTER_CONSUME;

    state->clear_mods |= filter->action.mods.mods.mask;
    if (!(filter->action.mods.flags & ACTION_LOCK_NO_UNLOCK))
        state->components.locked_mods &= ~filter->priv;

    filter->func = NULL;
    return XKB_FILTER_CONTINUE;
}

static void
xkb_filter_mod_latch_new(struct xkb_state *state, struct xkb_filter *filter)
{
    filter->priv = LATCH_KEY_DOWN;
    state->set_mods |= filter->action.mods.mods.mask;
}

static bool
xkb_filter_mod_latch_func(struct xkb_state *state,
                          struct xkb_filter *filter,
                          const struct xkb_key *key,
                          enum xkb_key_direction direction)
{
    enum xkb_key_latch_state latch = filter->priv;

    if (direction == XKB_KEY_DOWN) {
        const union xkb_action *actions = NULL;
        unsigned int count = xkb_key_get_actions(state, key, &actions);
        if (latch == LATCH_PENDING) {
            /* If this is a new keypress and we're awaiting our single latched
             * keypress, then either break the latch if any random key is pressed,
             * or promote it to a lock or plain base set if it's the same
             * modifier. */
            for (unsigned int k = 0; k < count; k++) {
                if (actions[k].type == ACTION_TYPE_MOD_LATCH &&
                    actions[k].mods.flags == filter->action.mods.flags &&
                    actions[k].mods.mods.mask == filter->action.mods.mods.mask) {
                    filter->action = actions[k];
                    if (filter->action.mods.flags & ACTION_LATCH_TO_LOCK) {
                        filter->action.type = ACTION_TYPE_MOD_LOCK;
                        filter->func = xkb_filter_mod_lock_func;
                        state->components.locked_mods |= filter->action.mods.mods.mask;
                    }
                    else {
                        filter->action.type = ACTION_TYPE_MOD_SET;
                        filter->func = xkb_filter_mod_set_func;
                        state->set_mods |= filter->action.mods.mods.mask;
                    }
                    filter->key = key;
                    state->components.latched_mods &= ~filter->action.mods.mods.mask;
                    /* XXX beep beep! */
                    return XKB_FILTER_CONSUME;
                }
                else if (xkb_action_breaks_latch(&(actions[k]))) {
                    /* XXX: This may be totally broken, we might need to break the
                     *      latch in the next run after this press? */
                    state->components.latched_mods &= ~filter->action.mods.mods.mask;
                    filter->func = NULL;
                    return XKB_FILTER_CONTINUE;
                }
                else if (actions->type == ACTION_TYPE_GROUP_LATCH ||
                         actions->type == ACTION_TYPE_MOD_LATCH) {
                    /* We break latches only if there is overlap with the type's mod mask. */
                    xkb_layout_index_t group = xkb_state_xkb_key_get_layout(state, key);
                    const struct xkb_key_type *type = key->groups[group].type;
                    xkb_mod_mask_t type_mod_mask = type->mods.mask;

                    const struct xkb_key_type_entry *entry = get_entry_for_mods(type, state->components.mods & type_mod_mask);
                    if (entry) {
                        type_mod_mask &= ~entry->preserve.mask;
                    }

                    xkb_mod_mask_t filter_mod_mask = filter->action.mods.mods.mask;
                    if (filter_mod_mask & type_mod_mask) {
                        state->components.latched_mods &= ~filter_mod_mask;
                        filter->func = NULL;
                        return XKB_FILTER_CONTINUE;
                    }
                }
            }
        }
        else if (latch == LATCH_KEY_DOWN) {
            /* Someone's pressed another key while we've still got the latching
            * key held down, so keep the base modifier state active (from
            * xkb_filter_mod_latch_new), but don't trip the latch, just clear
            * it as soon as the modifier gets released - but only if the action
            * is eligible to break latches. */
            for (unsigned int k = 0; k < count; k++) {
                if (xkb_action_breaks_latch(&(actions[k]))) {
                    latch = NO_LATCH;
                    break;
                } else if (actions->type == ACTION_TYPE_GROUP_LATCH ||
                           actions->type == ACTION_TYPE_MOD_LATCH) {
                    xkb_layout_index_t group = xkb_state_xkb_key_get_layout(state, key);
                    const struct xkb_key_type *type = key->groups[group].type;
                    xkb_mod_mask_t type_mod_mask = type->mods.mask;

                    const struct xkb_key_type_entry *entry = get_entry_for_mods(type, state->components.mods & type_mod_mask);
                    if (entry) {
                        type_mod_mask &= ~entry->preserve.mask;
                    }

                    xkb_mod_mask_t filter_mod_mask = filter->action.mods.mods.mask;
                    if (filter_mod_mask & type_mod_mask) {
                        latch = NO_LATCH;
                        break;
                    }
                }
            }
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
             (state->components.locked_mods & filter->action.mods.mods.mask) ==
             filter->action.mods.mods.mask)) {
            /* XXX: We might be a bit overenthusiastic about clearing
             *      mods other filters have set here? */
            if (latch == LATCH_PENDING)
                state->components.latched_mods &=
                    ~filter->action.mods.mods.mask;
            else
                state->clear_mods |= filter->action.mods.mods.mask;
            state->components.locked_mods &= ~filter->action.mods.mods.mask;
            filter->func = NULL;
        }
        else {
            latch = LATCH_PENDING;
            state->clear_mods |= filter->action.mods.mods.mask;
            state->components.latched_mods |= filter->action.mods.mods.mask;
            /* XXX beep beep! */
        }
    }

    filter->priv = latch;

    return XKB_FILTER_CONTINUE;
}

static const struct {
    void (*new)(struct xkb_state *state, struct xkb_filter *filter);
    bool (*func)(struct xkb_state *state, struct xkb_filter *filter,
                 const struct xkb_key *key, enum xkb_key_direction direction);
} filter_action_funcs[_ACTION_TYPE_NUM_ENTRIES] = {
    [ACTION_TYPE_MOD_SET]     = { xkb_filter_mod_set_new,
                                  xkb_filter_mod_set_func },
    [ACTION_TYPE_MOD_LATCH]   = { xkb_filter_mod_latch_new,
                                  xkb_filter_mod_latch_func },
    [ACTION_TYPE_MOD_LOCK]    = { xkb_filter_mod_lock_new,
                                  xkb_filter_mod_lock_func },
    [ACTION_TYPE_GROUP_SET]   = { xkb_filter_group_set_new,
                                  xkb_filter_group_set_func },
    [ACTION_TYPE_GROUP_LATCH] = { xkb_filter_group_latch_new,
                                  xkb_filter_group_latch_func },
    [ACTION_TYPE_GROUP_LOCK]  = { xkb_filter_group_lock_new,
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
    /* First run through all the currently active filters and see if any of
     * them have consumed this event. */
    bool consumed = false;
    struct xkb_filter *filter;
    darray_foreach(filter, state->filters) {
        if (!filter->func)
            continue;

        if (filter->func(state, filter, key, direction) == XKB_FILTER_CONSUME)
            consumed = true;
    }
    if (consumed || direction == XKB_KEY_UP)
        return;

    /* No filter consumed this event, so proceed with the key actions */
    const union xkb_action *actions = NULL;
    const unsigned int count = xkb_key_get_actions(state, key, &actions);

    /*
     * Process actions sequentially.
     *
     * NOTE: We rely on the parser to disallow multiple modifier or group
     * actions (see `CheckMultipleActionsCategories`). Allowing multiple such
     * actions requires a refactor of the state handling.
     */
    for (unsigned int k = 0; k < count; k++) {
        /*
         * It's possible for the keymap to set action->type explicitly, like so:
         *     interpret XF86_Next_VMode {
         *         action = Private(type=0x86, data="+VMode");
         *     };
         * We don't handle those.
         */
        if (actions[k].type >= _ACTION_TYPE_NUM_ENTRIES)
            continue;

        /* Go to next action if no corresponding action handler */
        if (!filter_action_funcs[actions[k].type].new)
            continue;

        /* Add a new filter and run the corresponding initial action */
        filter = xkb_filter_new(state);
        filter->key = key;
        filter->func = filter_action_funcs[actions[k].type].func;
        filter->action = actions[k];
        filter_action_funcs[actions[k].type].new(state, filter);
    }
}

struct xkb_state *
xkb_state_new(struct xkb_keymap *keymap)
{
    struct xkb_state *ret;

    ret = calloc(1, sizeof(*ret));
    if (!ret)
        return NULL;

    ret->refcnt = 1;
    ret->keymap = xkb_keymap_ref(keymap);

    return ret;
}

struct xkb_state *
xkb_state_ref(struct xkb_state *state)
{
    state->refcnt++;
    return state;
}

void
xkb_state_unref(struct xkb_state *state)
{
    if (!state || --state->refcnt > 0)
        return;

    xkb_keymap_unref(state->keymap);
    darray_free(state->filters);
    free(state);
}

struct xkb_keymap *
xkb_state_get_keymap(struct xkb_state *state)
{
    return state->keymap;
}

/**
 * Update the LED state to match the rest of the xkb_state.
 */
static void
xkb_state_led_update_all(struct xkb_state *state)
{
    xkb_led_index_t idx;
    const struct xkb_led *led;

    state->components.leds = 0;

    xkb_leds_enumerate(idx, led, state->keymap) {

        if (led->which_mods != 0 && led->mods.mask != 0) {
            xkb_mod_mask_t mod_mask = 0;
            if (led->which_mods & XKB_STATE_MODS_EFFECTIVE)
                mod_mask |= state->components.mods;
            if (led->which_mods & XKB_STATE_MODS_DEPRESSED)
                mod_mask |= state->components.base_mods;
            if (led->which_mods & XKB_STATE_MODS_LATCHED)
                mod_mask |= state->components.latched_mods;
            if (led->which_mods & XKB_STATE_MODS_LOCKED)
                mod_mask |= state->components.locked_mods;

            if (led->mods.mask & mod_mask) {
                state->components.leds |= (UINT32_C(1) << idx);
                continue;
            }
        }

        if (led->which_groups != 0) {
            if (likely(led->groups) != 0) {
                xkb_layout_mask_t group_mask = 0;
                /* Effective and locked groups have been brought into range */
                assert(state->components.group < XKB_MAX_GROUPS);
                assert(state->components.locked_group >= 0 &&
                       state->components.locked_group < XKB_MAX_GROUPS);
                /* Effective and locked groups are used as mask */
                if (led->which_groups & XKB_STATE_LAYOUT_EFFECTIVE)
                    group_mask |= (UINT32_C(1) << state->components.group);
                if (led->which_groups & XKB_STATE_LAYOUT_LOCKED)
                    group_mask |= (UINT32_C(1) << state->components.locked_group);
                /* Base and latched groups only have to be non-zero */
                if ((led->which_groups & XKB_STATE_LAYOUT_DEPRESSED) &&
                    state->components.base_group != 0)
                    group_mask |= led->groups;
                if ((led->which_groups & XKB_STATE_LAYOUT_LATCHED) &&
                    state->components.latched_group != 0)
                    group_mask |= led->groups;

                if (led->groups & group_mask) {
                    state->components.leds |= (UINT32_C(1) << idx);
                    continue;
                }
            } else {
                /* Special case for Base and latched groups */
                if (((led->which_groups & XKB_STATE_LAYOUT_DEPRESSED) &&
                     state->components.base_group == 0) ||
                    ((led->which_groups & XKB_STATE_LAYOUT_LATCHED) &&
                     state->components.latched_group == 0)) {
                    state->components.leds |= (UINT32_C(1) << idx);
                    continue;
                }
            }
        }

        if (led->ctrls & state->keymap->enabled_ctrls) {
            state->components.leds |= (UINT32_C(1) << idx);
            continue;
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
    xkb_layout_index_t wrapped;

    state->components.mods = (state->components.base_mods |
                              state->components.latched_mods |
                              state->components.locked_mods);

    /* TODO: Use groups_wrap control instead of always RANGE_WRAP. */

    /* Lock group must be adjusted, but not base nor latched groups */
    wrapped = XkbWrapGroupIntoRange(state->components.locked_group,
                                    state->keymap->num_groups,
                                    RANGE_WRAP, 0);
    static_assert(XKB_MAX_GROUPS < INT32_MAX, "Max groups don't fit");
    state->components.locked_group =
        (int32_t) (wrapped == XKB_LAYOUT_INVALID ? 0 : wrapped);

    /* Effective group must be adjusted */
    wrapped = XkbWrapGroupIntoRange(state->components.base_group +
                                    state->components.latched_group +
                                    state->components.locked_group,
                                    state->keymap->num_groups,
                                    RANGE_WRAP, 0);
    state->components.group =
        (wrapped == XKB_LAYOUT_INVALID ? 0 : wrapped);

    xkb_state_led_update_all(state);
}

static enum xkb_state_component
get_state_component_changes(const struct state_components *a,
                            const struct state_components *b)
{
    xkb_mod_mask_t mask = 0;

    if (a->group != b->group)
        mask |= XKB_STATE_LAYOUT_EFFECTIVE;
    if (a->base_group != b->base_group)
        mask |= XKB_STATE_LAYOUT_DEPRESSED;
    if (a->latched_group != b->latched_group)
        mask |= XKB_STATE_LAYOUT_LATCHED;
    if (a->locked_group != b->locked_group)
        mask |= XKB_STATE_LAYOUT_LOCKED;
    if (a->mods != b->mods)
        mask |= XKB_STATE_MODS_EFFECTIVE;
    if (a->base_mods != b->base_mods)
        mask |= XKB_STATE_MODS_DEPRESSED;
    if (a->latched_mods != b->latched_mods)
        mask |= XKB_STATE_MODS_LATCHED;
    if (a->locked_mods != b->locked_mods)
        mask |= XKB_STATE_MODS_LOCKED;
    if (a->leds != b->leds)
        mask |= XKB_STATE_LEDS;

    return mask;
}

/**
 * Given a particular key event, updates the state structure to reflect the
 * new modifiers.
 */
enum xkb_state_component
xkb_state_update_key(struct xkb_state *state, xkb_keycode_t kc,
                     enum xkb_key_direction direction)
{
    xkb_mod_index_t i;
    xkb_mod_mask_t bit;
    struct state_components prev_components;
    const struct xkb_key *key = XkbKey(state->keymap, kc);

    if (!key)
        return 0;

    prev_components = state->components;

    state->set_mods = 0;
    state->clear_mods = 0;

    xkb_filter_apply_all(state, key, direction);

    for (i = 0, bit = 1; state->set_mods; i++, bit <<= 1) {
        if (state->set_mods & bit) {
            state->mod_key_count[i]++;
            state->components.base_mods |= bit;
            state->set_mods &= ~bit;
        }
    }

    for (i = 0, bit = 1; state->clear_mods; i++, bit <<= 1) {
        if (state->clear_mods & bit) {
            state->mod_key_count[i]--;
            if (state->mod_key_count[i] <= 0) {
                state->components.base_mods &= ~bit;
                state->mod_key_count[i] = 0;
            }
            state->clear_mods &= ~bit;
        }
    }

    xkb_state_update_derived(state);

    return get_state_component_changes(&prev_components, &state->components);
}

/**
 * Updates the state from a set of explicit masks as gained from
 * xkb_state_serialize_mods and xkb_state_serialize_groups.  As noted in the
 * documentation for these functions in xkbcommon.h, this round-trip is
 * lossy, and should only be used to update a slave state mirroring the
 * master, e.g. in a client/server window system.
 */
enum xkb_state_component
xkb_state_update_mask(struct xkb_state *state,
                      xkb_mod_mask_t base_mods,
                      xkb_mod_mask_t latched_mods,
                      xkb_mod_mask_t locked_mods,
                      xkb_layout_index_t base_group,
                      xkb_layout_index_t latched_group,
                      xkb_layout_index_t locked_group)
{
    struct state_components prev_components;
    xkb_mod_mask_t mask;

    prev_components = state->components;

    /* Only include modifiers which exist in the keymap. */
    mask = (xkb_mod_mask_t) ((UINT64_C(1) << xkb_keymap_num_mods(state->keymap))
         - UINT64_C(1));

    state->components.base_mods = base_mods & mask;
    state->components.latched_mods = latched_mods & mask;
    state->components.locked_mods = locked_mods & mask;

    /* Make sure the mods are fully resolved - since we get arbitrary
     * input, they might not be.
     *
     * It might seem more reasonable to do this only for components.mods
     * in xkb_state_update_derived(), rather than for each component
     * seperately.  That would allow to distinguish between "really"
     * depressed mods (would be in MODS_DEPRESSED) and indirectly
     * depressed to to a mapping (would only be in MODS_EFFECTIVE).
     * However, the traditional behavior of xkb_state_update_key() is that
     * if a vmod is depressed, its mappings are depressed with it; so we're
     * expected to do the same here.  Also, LEDs (usually) look if a real
     * mod is locked, not just effective; otherwise it won't be lit.
     *
     * We OR here because mod_mask_get_effective() drops vmods. */
    state->components.base_mods |=
        mod_mask_get_effective(state->keymap, state->components.base_mods);
    state->components.latched_mods |=
        mod_mask_get_effective(state->keymap, state->components.latched_mods);
    state->components.locked_mods |=
        mod_mask_get_effective(state->keymap, state->components.locked_mods);

    static_assert(XKB_MAX_GROUPS < INT32_MAX, "Max groups don't fit");
    state->components.base_group = (int32_t) base_group;
    state->components.latched_group = (int32_t) latched_group;
    state->components.locked_group = (int32_t) locked_group;

    xkb_state_update_derived(state);

    return get_state_component_changes(&prev_components, &state->components);
}

/**
 * Provides the symbols to use for the given key and state.  Returns the
 * number of symbols pointed to in syms_out.
 */
int
xkb_state_key_get_syms(struct xkb_state *state, xkb_keycode_t kc,
                       const xkb_keysym_t **syms_out)
{
    xkb_layout_index_t layout;
    xkb_level_index_t level;

    layout = xkb_state_key_get_layout(state, kc);
    if (layout == XKB_LAYOUT_INVALID)
        goto err;

    level = xkb_state_key_get_level(state, kc, layout);
    if (level == XKB_LEVEL_INVALID)
        goto err;

    return xkb_keymap_key_get_syms_by_level(state->keymap, kc, layout, level,
                                            syms_out);

err:
    *syms_out = NULL;
    return 0;
}

/*
 * https://www.x.org/releases/current/doc/kbproto/xkbproto.html#Interpreting_the_Lock_Modifier
 */
static bool
should_do_caps_transformation(struct xkb_state *state, xkb_keycode_t kc)
{
    xkb_mod_index_t caps =
        xkb_keymap_mod_get_index(state->keymap, XKB_MOD_NAME_CAPS);

    return
        xkb_state_mod_index_is_active(state, caps, XKB_STATE_MODS_EFFECTIVE) > 0 &&
        xkb_state_mod_index_is_consumed(state, kc, caps) == 0;
}

/*
 * https://www.x.org/releases/current/doc/kbproto/xkbproto.html#Interpreting_the_Control_Modifier
 */
static bool
should_do_ctrl_transformation(struct xkb_state *state, xkb_keycode_t kc)
{
    xkb_mod_index_t ctrl =
        xkb_keymap_mod_get_index(state->keymap, XKB_MOD_NAME_CTRL);

    return
        xkb_state_mod_index_is_active(state, ctrl, XKB_STATE_MODS_EFFECTIVE) > 0 &&
        xkb_state_mod_index_is_consumed(state, kc, ctrl) == 0;
}

/*
 * Verbatim from `libX11:src/xkb/XKBBind.c`.
 *
 * The basic transformations are defined in “[Interpreting the Control Modifier]”.
 * They correspond to the [caret notation], which maps the characters
 * `@ABC...XYZ[\]^_` by masking them with `0x1f`. Note that there is no
 * transformation for `?`, although `^?` is defined in the [caret notation].
 *
 * For convenience, the range ```abc...xyz{|}~`` and the space character ` `
 * are processed the same way. This allow to produce control characters without
 * requiring the use of the `Shift` modifier for letters.
 *
 * The transformation of the digits seems to originate from the [VT220 terminal],
 * as a compatibility for non-US keyboards. Indeed, these keyboards may not have
 * the punctuation characters available or in a convenient position. Some mnemonics:
 *
 * - ^2 maps to ^@ because @ is on the key 2 in the US layout.
 * - ^6 maps to ^^ because ^ is on the key 6 in the US layout.
 * - characters 3, 4, 5, 6, and 7 seems to align with the sequence `[\]^_`.
 * - 8 closes the sequence and so maps to the last control character.
 *
 * The `/` transformation seems to be defined for compatibility or convenience.
 *
 * [Interpreting the Control Modifier]: https://www.x.org/releases/current/doc/kbproto/xkbproto.html#Interpreting_the_Control_Modifier
 * [caret notation]: https://en.wikipedia.org/wiki/Caret_notation
 * [VT220 terminal]: https://vt100.net/docs/vt220-rm/chapter3.html#T3-5
 */
static char
XkbToControl(char ch)
{
    char c = ch;

    if ((c >= '@' && c < '\177') || c == ' ')
        c &= 0x1F;
    else if (c == '2')
        c = '\000';
    else if (c >= '3' && c <= '7')
        c -= ('3' - '\033');
    else if (c == '8')
        c = '\177';
    else if (c == '/')
        c = '_' & 0x1F;
    return c;
}

/**
 * Provides either exactly one symbol, or XKB_KEY_NoSymbol.
 */
xkb_keysym_t
xkb_state_key_get_one_sym(struct xkb_state *state, xkb_keycode_t kc)
{
    const xkb_keysym_t *syms;
    xkb_keysym_t sym;
    int num_syms;

    num_syms = xkb_state_key_get_syms(state, kc, &syms);
    if (num_syms != 1)
        return XKB_KEY_NoSymbol;

    sym = syms[0];

    if (should_do_caps_transformation(state, kc))
        sym = xkb_keysym_to_upper(sym);

    return sym;
}

/*
 * The caps and ctrl transformations require some special handling,
 * so we cannot simply use xkb_state_get_one_sym() for them.
 * In particular, if Control is set, we must try very hard to find
 * some layout in which the keysym is ASCII and thus can be (maybe)
 * converted to a control character. libX11 allows to disable this
 * behavior with the XkbLC_ControlFallback (see XkbSetXlibControls(3)),
 * but it is enabled by default, yippee.
 */
static xkb_keysym_t
get_one_sym_for_string(struct xkb_state *state, xkb_keycode_t kc)
{
    xkb_level_index_t level;
    xkb_layout_index_t layout, num_layouts;
    const xkb_keysym_t *syms;
    int nsyms;
    xkb_keysym_t sym;

    layout = xkb_state_key_get_layout(state, kc);
    num_layouts = xkb_keymap_num_layouts_for_key(state->keymap, kc);
    level = xkb_state_key_get_level(state, kc, layout);
    if (layout == XKB_LAYOUT_INVALID || num_layouts == 0 ||
        level == XKB_LEVEL_INVALID)
        return XKB_KEY_NoSymbol;

    nsyms = xkb_keymap_key_get_syms_by_level(state->keymap, kc,
                                             layout, level, &syms);
    if (nsyms != 1)
        return XKB_KEY_NoSymbol;
    sym = syms[0];

    if (should_do_ctrl_transformation(state, kc) && sym > 127u) {
        for (xkb_layout_index_t i = 0; i < num_layouts; i++) {
            level = xkb_state_key_get_level(state, kc, i);
            if (level == XKB_LEVEL_INVALID)
                continue;

            nsyms = xkb_keymap_key_get_syms_by_level(state->keymap, kc,
                                                     i, level, &syms);
            if (nsyms == 1 && syms[0] <= 127u) {
                sym = syms[0];
                break;
            }
        }
    }

    if (should_do_caps_transformation(state, kc)) {
        sym = xkb_keysym_to_upper(sym);
    }

    return sym;
}

int
xkb_state_key_get_utf8(struct xkb_state *state, xkb_keycode_t kc,
                       char *buffer, size_t size)
{
    xkb_keysym_t sym;
    const xkb_keysym_t *syms;
    int nsyms;
    int offset;
    char tmp[XKB_KEYSYM_UTF8_MAX_SIZE];

    sym = get_one_sym_for_string(state, kc);
    if (sym != XKB_KEY_NoSymbol) {
        nsyms = 1; syms = &sym;
    }
    else {
        nsyms = xkb_state_key_get_syms(state, kc, &syms);
    }

    /* Make sure not to truncate in the middle of a UTF-8 sequence. */
    offset = 0;
    for (int i = 0; i < nsyms; i++) {
        int ret = xkb_keysym_to_utf8(syms[i], tmp, sizeof(tmp));
        if (ret <= 0)
            goto err_bad;

        ret--;
        if ((size_t) offset + ret <= size)
            memcpy(buffer + offset, tmp, ret);
        offset += ret;
    }

    if ((size_t) offset >= size)
        goto err_trunc;
    buffer[offset] = '\0';

    if (!is_valid_utf8(buffer, offset))
        goto err_bad;

    if (offset == 1 && (unsigned int) buffer[0] <= 127u &&
        should_do_ctrl_transformation(state, kc))
        buffer[0] = XkbToControl(buffer[0]);

    return offset;

err_trunc:
    if (size > 0)
        buffer[size - 1] = '\0';
    return offset;

err_bad:
    if (size > 0)
        buffer[0] = '\0';
    return 0;
}

uint32_t
xkb_state_key_get_utf32(struct xkb_state *state, xkb_keycode_t kc)
{
    xkb_keysym_t sym;
    uint32_t cp;

    sym = get_one_sym_for_string(state, kc);
    cp = xkb_keysym_to_utf32(sym);

    if (cp <= 127u && should_do_ctrl_transformation(state, kc))
        cp = (uint32_t) XkbToControl((char) cp);

    return cp;
}

/**
 * Serialises the requested modifier state into an xkb_mod_mask_t, with all
 * the same disclaimers as in xkb_state_update_mask.
 */
xkb_mod_mask_t
xkb_state_serialize_mods(struct xkb_state *state,
                         enum xkb_state_component type)
{
    xkb_mod_mask_t ret = 0;

    if (type & XKB_STATE_MODS_EFFECTIVE)
        return state->components.mods;

    if (type & XKB_STATE_MODS_DEPRESSED)
        ret |= state->components.base_mods;
    if (type & XKB_STATE_MODS_LATCHED)
        ret |= state->components.latched_mods;
    if (type & XKB_STATE_MODS_LOCKED)
        ret |= state->components.locked_mods;

    return ret;
}

/**
 * Serialises the requested group state, with all the same disclaimers as
 * in xkb_state_update_mask.
 */
xkb_layout_index_t
xkb_state_serialize_layout(struct xkb_state *state,
                           enum xkb_state_component type)
{
    xkb_layout_index_t ret = 0;

    if (type & XKB_STATE_LAYOUT_EFFECTIVE)
        return state->components.group;

    if (type & XKB_STATE_LAYOUT_DEPRESSED)
        ret += state->components.base_group;
    if (type & XKB_STATE_LAYOUT_LATCHED)
        ret += state->components.latched_group;
    if (type & XKB_STATE_LAYOUT_LOCKED)
        ret += state->components.locked_group;

    return ret;
}

/**
 * Gets a modifier mask and returns the resolved effective mask; this
 * is needed because some modifiers can also map to other modifiers, e.g.
 * the "NumLock" modifier usually also sets the "Mod2" modifier.
 */
xkb_mod_mask_t
mod_mask_get_effective(struct xkb_keymap *keymap, xkb_mod_mask_t mods)
{
    const struct xkb_mod *mod;
    xkb_mod_index_t i;
    xkb_mod_mask_t mask;

    /* The effective mask is only real mods for now. */
    mask = mods & MOD_REAL_MASK_ALL;

    xkb_mods_enumerate(i, mod, &keymap->mods)
        if (mods & (UINT32_C(1) << i))
            mask |= mod->mapping;

    return mask;
}

/* Get the mapping of a modifier */
static inline xkb_mod_mask_t
mod_mapping(struct xkb_mod *mod, xkb_mod_index_t idx)
{
    /*
     * We cannot use `mod->mapping` directly, because it is
     * not set for real modifiers.
     */
    return (mod->type & MOD_REAL) ? (UINT32_C(1) << idx) : mod->mapping;
}

/**
 * Returns 1 if the given modifier is active with the specified type(s), 0 if
 * not, or -1 if the modifier is invalid.
 */
int
xkb_state_mod_index_is_active(struct xkb_state *state,
                              xkb_mod_index_t idx,
                              enum xkb_state_component type)
{
    if (unlikely(idx >= xkb_keymap_num_mods(state->keymap)))
        return -1;

    xkb_mod_mask_t mapping = mod_mapping(&state->keymap->mods.mods[idx], idx);
    if (!mapping) {
        /* Modifier not mapped */
        return 0;
    }
    /* WARNING: this may overmatch for virtual modifiers */
    return !!((xkb_state_serialize_mods(state, type) & mapping) == mapping);
}

/**
 * Helper function for xkb_state_mod_indices_are_active and
 * xkb_state_mod_names_are_active.
 */
static bool
match_mod_masks(struct xkb_state *state,
                enum xkb_state_component type,
                enum xkb_state_match match,
                xkb_mod_mask_t wanted)
{
    xkb_mod_mask_t active = xkb_state_serialize_mods(state, type);

    if (!(match & XKB_STATE_MATCH_NON_EXCLUSIVE) && (active & ~wanted))
        return false;

    if (match & XKB_STATE_MATCH_ANY)
        return active & wanted;

    return (active & wanted) == wanted;
}

/**
 * Returns 1 if the modifiers are active with the specified type(s), 0 if
 * not, or -1 if any of the modifiers are invalid.
 */
int
xkb_state_mod_indices_are_active(struct xkb_state *state,
                                 enum xkb_state_component type,
                                 enum xkb_state_match match,
                                 ...)
{
    va_list ap;
    xkb_mod_mask_t wanted = 0;
    int ret = 0;
    xkb_mod_index_t num_mods = xkb_keymap_num_mods(state->keymap);

    va_start(ap, match);
    while (1) {
        xkb_mod_index_t idx = va_arg(ap, xkb_mod_index_t);
        if (idx == XKB_MOD_INVALID)
            break;
        if (unlikely(idx >= num_mods)) {
            ret = -1;
            break;
        }
        wanted |= mod_mapping(&state->keymap->mods.mods[idx], idx);
    }
    va_end(ap);

    if (ret == -1)
        return ret;

    if (!wanted) {
        /* Modifiers not mapped */
        return 0;
    }

    return match_mod_masks(state, type, match, wanted);
}

/**
 * Returns 1 if the given modifier is active with the specified type(s), 0 if
 * not, or -1 if the modifier is invalid.
 */
int
xkb_state_mod_name_is_active(struct xkb_state *state, const char *name,
                             enum xkb_state_component type)
{
    xkb_mod_index_t idx = xkb_keymap_mod_get_index(state->keymap, name);

    if (idx == XKB_MOD_INVALID)
        return -1;

    return xkb_state_mod_index_is_active(state, idx, type);
}

/**
 * Returns 1 if the modifiers are active with the specified type(s), 0 if
 * not, or -1 if any of the modifiers are invalid.
 */
ATTR_NULL_SENTINEL int
xkb_state_mod_names_are_active(struct xkb_state *state,
                               enum xkb_state_component type,
                               enum xkb_state_match match,
                               ...)
{
    va_list ap;
    xkb_mod_mask_t wanted = 0;
    int ret = 0;

    va_start(ap, match);
    while (1) {
        xkb_mod_index_t idx;
        const char *str = va_arg(ap, const char *);
        if (str == NULL)
            break;
        idx = xkb_keymap_mod_get_index(state->keymap, str);
        if (idx == XKB_MOD_INVALID) {
            ret = -1;
            break;
        }
        wanted |= mod_mapping(&state->keymap->mods.mods[idx], idx);
    }
    va_end(ap);

    if (ret == -1)
        return ret;

    if (!wanted) {
        /* Modifiers not mapped */
        return 0;
    }

    return match_mod_masks(state, type, match, wanted);
}

/**
 * Returns 1 if the given group is active with the specified type(s), 0 if
 * not, or -1 if the group is invalid.
 */
int
xkb_state_layout_index_is_active(struct xkb_state *state,
                                 xkb_layout_index_t idx,
                                 enum xkb_state_component type)
{
    int ret = 0;

    if (idx >= state->keymap->num_groups)
        return -1;

    if (type & XKB_STATE_LAYOUT_EFFECTIVE)
        ret |= (state->components.group == idx);
    if (type & XKB_STATE_LAYOUT_DEPRESSED)
        ret |= (state->components.base_group == (int32_t) idx);
    if (type & XKB_STATE_LAYOUT_LATCHED)
        ret |= (state->components.latched_group == (int32_t) idx);
    if (type & XKB_STATE_LAYOUT_LOCKED)
        ret |= (state->components.locked_group == (int32_t) idx);

    return ret;
}

/**
 * Returns 1 if the given modifier is active with the specified type(s), 0 if
 * not, or -1 if the modifier is invalid.
 */
int
xkb_state_layout_name_is_active(struct xkb_state *state, const char *name,
                                enum xkb_state_component type)
{
    xkb_layout_index_t idx = xkb_keymap_layout_get_index(state->keymap, name);

    if (idx == XKB_LAYOUT_INVALID)
        return -1;

    return xkb_state_layout_index_is_active(state, idx, type);
}

/**
 * Returns 1 if the given LED is active, 0 if not, or -1 if the LED is invalid.
 */
int
xkb_state_led_index_is_active(struct xkb_state *state, xkb_led_index_t idx)
{
    if (idx >= state->keymap->num_leds ||
        state->keymap->leds[idx].name == XKB_ATOM_NONE)
        return -1;

    return !!(state->components.leds & (UINT32_C(1) << idx));
}

/**
 * Returns 1 if the given LED is active, 0 if not, or -1 if the LED is invalid.
 */
int
xkb_state_led_name_is_active(struct xkb_state *state, const char *name)
{
    xkb_led_index_t idx = xkb_keymap_led_get_index(state->keymap, name);

    if (idx == XKB_LED_INVALID)
        return -1;

    return xkb_state_led_index_is_active(state, idx);
}

/**
 * See:
 * - XkbTranslateKeyCode(3), mod_rtrn return value, from libX11.
 * - MyEnhancedXkbTranslateKeyCode(), a modification of the above, from GTK+.
 */
static xkb_mod_mask_t
key_get_consumed(struct xkb_state *state, const struct xkb_key *key,
                 enum xkb_consumed_mode mode)
{
    const struct xkb_key_type *type;
    const struct xkb_key_type_entry *matching_entry;
    xkb_mod_mask_t preserve = 0;
    xkb_layout_index_t group;
    xkb_mod_mask_t consumed = 0;

    group = xkb_state_key_get_layout(state, key->keycode);
    if (group == XKB_LAYOUT_INVALID)
        return 0;

    type = key->groups[group].type;

    matching_entry = get_entry_for_key_state(state, key, group);
    if (matching_entry)
        preserve = matching_entry->preserve.mask;

    switch (mode) {
    case XKB_CONSUMED_MODE_XKB:
        consumed = type->mods.mask;
        break;

    case XKB_CONSUMED_MODE_GTK: {
        const struct xkb_key_type_entry *no_mods_entry;
        xkb_level_index_t no_mods_leveli;
        const struct xkb_level *no_mods_level, *level;

        no_mods_entry = get_entry_for_mods(type, 0);
        no_mods_leveli = no_mods_entry ? no_mods_entry->level : 0;
        no_mods_level = &key->groups[group].levels[no_mods_leveli];

        for (unsigned i = 0; i < type->num_entries; i++) {
            const struct xkb_key_type_entry *entry = &type->entries[i];
            if (!entry_is_active(entry))
                continue;

            level = &key->groups[group].levels[entry->level];
            if (XkbLevelsSameSyms(level, no_mods_level))
                continue;

            if (entry == matching_entry || one_bit_set(entry->mods.mask))
                consumed |= entry->mods.mask & ~entry->preserve.mask;
        }
        break;
    }
    }

    return consumed & ~preserve;
}

int
xkb_state_mod_index_is_consumed2(struct xkb_state *state, xkb_keycode_t kc,
                                 xkb_mod_index_t idx,
                                 enum xkb_consumed_mode mode)
{
    const struct xkb_key *key = XkbKey(state->keymap, kc);

    if (unlikely(!key || idx >= xkb_keymap_num_mods(state->keymap)))
        return -1;

    xkb_mod_mask_t mapping = mod_mapping(&state->keymap->mods.mods[idx], idx);
    if (!mapping) {
        /* Modifier not mapped */
        return 0;
    }
    return !!((mapping & key_get_consumed(state, key, mode)) == mapping);
}

int
xkb_state_mod_index_is_consumed(struct xkb_state *state, xkb_keycode_t kc,
                                xkb_mod_index_t idx)
{
    return xkb_state_mod_index_is_consumed2(state, kc, idx,
                                            XKB_CONSUMED_MODE_XKB);
}

xkb_mod_mask_t
xkb_state_mod_mask_remove_consumed(struct xkb_state *state, xkb_keycode_t kc,
                                   xkb_mod_mask_t mask)
{
    const struct xkb_key *key = XkbKey(state->keymap, kc);

    if (!key)
        return 0;

    return mod_mask_get_effective(state->keymap, mask) &
           ~key_get_consumed(state, key, XKB_CONSUMED_MODE_XKB);
}

xkb_mod_mask_t
xkb_state_key_get_consumed_mods2(struct xkb_state *state, xkb_keycode_t kc,
                                 enum xkb_consumed_mode mode)
{
    const struct xkb_key *key;

    switch (mode) {
    case XKB_CONSUMED_MODE_XKB:
    case XKB_CONSUMED_MODE_GTK:
        break;
    default:
        log_err_func(state->keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
                     "unrecognized consumed modifiers mode: %d\n", mode);
        return 0;
    }

    key = XkbKey(state->keymap, kc);
    if (!key)
        return 0;

    return key_get_consumed(state, key, mode);
}

xkb_mod_mask_t
xkb_state_key_get_consumed_mods(struct xkb_state *state, xkb_keycode_t kc)
{
    return xkb_state_key_get_consumed_mods2(state, kc, XKB_CONSUMED_MODE_XKB);
}
