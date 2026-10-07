/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#if WCHAR_MAX >= 0x7FFFFFFF
#include <wctype.h>
#define USE_ISWPRINT
#if HAVE_NEWLOCALE && HAVE_ISWPRINT_L
#define USE_ISWPRINT_L
#include <locale.h>
#endif
#else
#include <ctype.h>
#endif

#include "utils.h"
#include "xkbcommon/xkbcommon-status.h"
#include "xkbcommon/xkbcommon.h"
#include "darray.h"
#include "state-priv.h"
#include "typing.h"
#include "util-bits.h"
#include "util-mem.h"

enum xkb_status
xkb_key_set_init(struct xkb_key_set *set,
                 struct xkb_keymap *keymap,
                 const struct xkb_key_set_config *config)
{
    darray_init(set->printable);
    darray_init(set->modifiers);
    darray_init(set->misc);

    static const struct xkb_key_set_config default_config = {0};
    if (!config)
        config = &default_config;
    /* keycode = 0 denotes “default” */
    const xkb_keycode_t min = MAX(config->min, xkb_keymap_min_keycode(keymap));
    const xkb_keycode_t max = config->max
        ? MIN(config->max, xkb_keymap_max_keycode(keymap))
        : xkb_keymap_max_keycode(keymap);

    enum xkb_status status;
    struct xkb_state *state =
        xkb_state_new_with_mode(keymap, XKB_STATE_MODE_SERVER, &status);
    if (!state)
        return status;

    #ifdef USE_ISWPRINT_L
    locale_t loc = newlocale(LC_ALL_MASK, "C.UTF-8", (locale_t)0);
    #endif

    struct xkb_keymap_key_iterator iter;
    const struct xkb_keymap_key_iterator_config iter_config = {
        .size = sizeof(iter_config),
        .flags = XKB_KEYMAP_KEY_ITERATOR_NO_FLAGS,
        .start = min
    };
    status = xkb_keymap_key_iterator_init(&iter, keymap, &iter_config);
    if (status != XKB_SUCCESS)
        goto error;

    const xkb_mod_mask_t numlock =
        xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_NUM);

    const struct xkb_state_components_update components_update = {
        .size = sizeof(components_update),
        .components = XKB_STATE_MODS_LOCKED,
        .affect_locked_mods = numlock,
        .locked_mods = numlock,
    };

    const struct xkb_synthetic_update update = {
        .size = sizeof(update),
        .components = &components_update,
    };

    xkb_keycode_t k;
    while ((k = xkb_keymap_key_iterator_next(&iter)) != XKB_KEYCODE_INVALID) {
        if (k > max)
            break;

        xkb_state_reset(state);
        status = xkb_state_update_synthetic(state, &update, NULL);
        if (status != XKB_SUCCESS)
            goto error;

        const xkb_keysym_t *syms = NULL;
        const int count = xkb_state_key_get_syms(state, k, &syms);

        enum xkb_state_component changed =
            xkb_state_update_key(state, k, XKB_KEY_DOWN);
        xkb_mod_mask_t mods =
            xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        changed |= xkb_state_update_key(state, k, XKB_KEY_UP);
        mods |= xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);

        if (changed & XKB_STATE_MODS_EFFECTIVE) {
            /* Modifier key */
            enum xkb_modifier_kind kind;
            if (changed & XKB_STATE_MODS_LOCKED) {
                kind = XKB_MOD_LOCK;
            } else if (changed & XKB_STATE_MODS_LATCHED) {
                kind = XKB_MOD_LATCH;
            } else {
                kind = XKB_MOD_SET;
            }
            mods &= ~numlock;
            if (!mods)
                mods = numlock;
            darray_append(set->modifiers, (struct xkb_modifier_key) {
                .keycode = k,
                .mods = mods,
                .kind = kind,
            });
            continue;
        }

        if (count > 0) {
            uint32_t utf32 = 0;
            for (int s = 0; s < count; s++) {
                utf32 = xkb_keysym_to_utf32(syms[s]);
                const bool is_printable =
                    #ifdef USE_ISWPRINT_L
                    ((loc && iswprint_l((wint_t)utf32, loc)) || iswprint((wint_t)utf32))
                    #elif defined(USE_ISWPRINT)
                    iswprint((wint_t)utf32)
                    #else
                    utf32 <= 0x7f && isprint((int)utf32)
                    #endif
                    ;
                if (is_printable)
                    break;
                utf32 = 0;
            }
            if (utf32) {
                /* Printable keysyms */
                darray_append(set->printable, k);
                continue;
            }
        }

        /* Miscellaneous key */
        darray_append(set->misc, k);
    }

error:
    #ifdef USE_ISWPRINT_L
    if (loc)
        freelocale(loc);
    #endif

    xkb_state_unref(state);

    return status;
}

void
xkb_key_set_destroy(struct xkb_key_set *set)
{
    if (!set)
        return;

    darray_free(set->modifiers);
    darray_free(set->printable);
    darray_free(set->misc);
}

static enum xkb_status
add_pending_events(struct xkb_typing_state * restrict state,
                   const struct xkb_pending_typing_event * restrict events,
                   size_t count)
{
    for (size_t e = 0; e < count; e++) {
        /* Look for a free slot */
        struct xkb_pending_typing_event *pending;
        darray_size_t idx = 0;
        darray_enumerate_from(idx, pending, state->pending, idx) {
            if (!pending->counter) {
                darray_item(state->pending, idx) = events[e];
                goto next;
            }
        }
        /* No free slot found: append new one */
        idx = darray_size(state->pending);
        darray_append(state->pending, events[e]);
    next:
        /* Avoid immediate consumption */
        darray_item(state->pending, idx).counter++;
    }
    state->num_pending += count;

    return XKB_SUCCESS;
}

static enum xkb_status
next_pending_events(struct xkb_typing_state * restrict state,
                    xkb_typing_events * restrict out)
{
    /* Reset the output */
    darray_size(*out) = 0;

    darray_size_t idx;
    struct xkb_pending_typing_event *pending;
    /* Stack: start from the end */
    darray_enumerate_reverse(idx, pending, state->pending) {
        if (!pending->counter)
            continue;
        pending->counter--;
        if (pending->counter)
            continue;
        darray_append(*out, pending->event);
        state->num_pending--;
        /* Resize array if it was the last element */
        if (idx + 1 == darray_size(state->pending))
            darray_size(state->pending)--;
        state->mods &= ~pending->mods;
    }

    return XKB_SUCCESS;
}

enum {
    /** Tries to avoid pressing a key already pressed */
    KEY_CHOICE_MAX_TRIES = 12,
    XKB_ERROR_NO_RELEASED_KEY = -1000,
};

static enum xkb_status
add_modifier_key(struct xkb_typing_state * restrict state, size_t * restrict e)
{
    /* Choose a random key */
    long idx = -1;
    for (int tries = 0; tries < KEY_CHOICE_MAX_TRIES; tries++) {
        idx = state->prng(state->prng_state)
            % darray_size(state->keys->modifiers);
        /*
         * Ensure key is not already down and its modifier mask is not active,
         * i.e. to avoid simultaneous keys with the same modifier.
         */
        if (!bit_array_get(state->keys_state,
                           darray_item(state->keys->modifiers, idx).keycode) &&
            !(state->mods & darray_item(state->keys->modifiers, idx).mods))
        {
            break;
        }
        idx = -1;
    }

    struct xkb_modifier_key key = {0};
    if (idx < 0) {
        /* No released key found: fall back to linear search */
        const struct xkb_modifier_key *key_;
        darray_foreach(key_, state->keys->modifiers) {
            /* See test doc above */
            if (!bit_array_get(state->keys_state, key_->keycode) &&
                !(state->mods & key_->mods))
            {
                key = *key_;
                idx = 0;
                break;
            }
        }
        if (idx < 0) {
            /* No fallback: skip */
            return (enum xkb_status)XKB_ERROR_NO_RELEASED_KEY;
        }
    } else {
        key = darray_item(state->keys->modifiers, idx);
    }

    const size_t remaining_events =
        state->num_events - state->num_pending - *e;
    size_t pending_turns = 0;

    if (remaining_events >= 2) {
        if (key.kind == XKB_MOD_SET ||
            (key.kind == XKB_MOD_LOCK && remaining_events >= 4))
        {
            /* Delay SetMods release or LockMods unlock */
            const size_t max_turns = MIN(
                (size_t)(
                    XKB_TYPING_EVENT_MODIFIER_MAX_PENDING_TURNS -
                    XKB_TYPING_EVENT_MODIFIER_MIN_PENDING_TURNS
                 ),
                remaining_events + state->num_pending
            );
            pending_turns = XKB_TYPING_EVENT_MODIFIER_MIN_PENDING_TURNS
                          + (size_t)state->prng(state->prng_state)
                          % (max_turns + 1);
        }
    } else {
        return XKB_ERROR_INVALID;
    }

    state->events[(*e)++] = (struct xkb_typing_event) {
        .keycode = key.keycode,
        .direction = XKB_KEY_DOWN,
    };

    struct xkb_pending_typing_event pending[2] = {
        {
            .event = {
                .keycode = key.keycode,
                .direction = XKB_KEY_DOWN,
            },
            .counter = (uint8_t)pending_turns,
        },
        {
            .event = {
                .keycode = key.keycode,
                .direction = XKB_KEY_UP,
            },
            .mods = key.mods,
            .counter = (uint8_t)pending_turns,
        },
    };

    if (pending_turns) {
        if (key.kind == XKB_MOD_LOCK) {
            /* Immediate release */
            state->events[(*e)++] = pending[1].event;
            /* Delayed unlock (press + release) */
            pending[1].counter++;
            const enum xkb_status status =
                add_pending_events(state, pending, ARRAY_SIZE(pending));
            if (status != XKB_SUCCESS)
                return status;
            /*
             * Register “locked” state, to avoid tapping the key before
             * delayed key released
             */
            bit_array_set(state->keys_state, key.keycode);
        } else {
            /* Delayed release */
            const enum xkb_status status =
                add_pending_events(state, &pending[1], 1);
            if (status != XKB_SUCCESS)
                return status;
            /* Register pressed state */
            bit_array_set(state->keys_state, key.keycode);
        }
        state->mods |= key.mods;
    } else {
        /* Immediate release */
        state->events[(*e)++] = pending[1].event;
        /* TODO: add latched modifier to mask state. */
    }

    return XKB_SUCCESS;
}

static enum xkb_status
add_other_key(struct xkb_typing_state * restrict state,
              const xkb_keycodes_t * restrict keycodes, size_t * restrict e,
              uint8_t immediate_release_prob, uint8_t max_pending_turns)
{
    /* Choose a random key */
    long idx = -1;
    for (int tries = 0; tries < KEY_CHOICE_MAX_TRIES; tries++) {
        idx = state->prng(state->prng_state) % darray_size(*keycodes);
        /* Ensure key is not already down */
        if (!bit_array_get(state->keys_state,
                           darray_item(*keycodes, idx)))
        {
            break;
        }
        idx = -1;
    }

    xkb_keycode_t keycode = XKB_KEYCODE_INVALID;
    if (idx < 0) {
        /* No released key found: fall back to linear search */
        const xkb_keycode_t *keycode_;
        darray_foreach(keycode_, *keycodes) {
            if (!bit_array_get(state->keys_state, *keycode_)) {
                keycode = *keycode_;
                idx = 0;
                break;
            }
        }
        if (idx < 0) {
            /* No fallback: skip */
            return (enum xkb_status)XKB_ERROR_NO_RELEASED_KEY;
        }
    } else {
        keycode = darray_item(*keycodes, idx);
    }

    const size_t remaining_events =
        state->num_events - state->num_pending - *e;
    size_t pending_turns;

    if (remaining_events >= 2) {
        const size_t max_turns = MIN(
            (size_t)max_pending_turns,
            remaining_events + state->num_pending
        );
        const bool immediate_release = (
            (immediate_release_prob >= 100) ||
            !max_turns ||
            (state->prng(state->prng_state) % 100) < immediate_release_prob
        );
        if (immediate_release) {
            pending_turns = 0;
        } else {
            pending_turns = 1 + (size_t)state->prng(state->prng_state)
                          % max_turns;
        }
    } else {
        return XKB_ERROR_INVALID;
    }

    state->events[(*e)++] = (struct xkb_typing_event) {
        .keycode = keycode,
        .direction = XKB_KEY_DOWN,
    };

    const struct xkb_pending_typing_event pending = {
        .event = {
            .keycode = keycode,
            .direction = XKB_KEY_UP,
        },
        .counter = (uint8_t)pending_turns,
    };
    if (pending_turns) {
        const enum xkb_status status = add_pending_events(state, &pending, 1);
        if (status != XKB_SUCCESS)
            return status;
        /* Register pressed state */
        bit_array_set(state->keys_state, keycode);
    } else {
        state->events[(*e)++] = pending.event;
    }

    return XKB_SUCCESS;
}

static enum xkb_key_kind
pick_key_kind(xkb_prng_t prng, void *prng_state)
{
    static const uint8_t weights[_NUM_XKB_KEY_KIND] = {
        [XKB_KEY_PRINTABLE] = 60,
        [XKB_KEY_MODIFIER] = 30,
        [XKB_KEY_MISC] = 10,
    };
    static_assert(XKB_KEY_MISC == 2 &&
                  XKB_KEY_MISC == _NUM_XKB_KEY_KIND - 1, "");
    assert(weights[0] + weights[1] + weights[2] == 100);

    long r = prng(prng_state) % 100;
    for (enum xkb_key_kind k = 0; k < _NUM_XKB_KEY_KIND; k++)
        if ((r -= weights[k]) < 0)
            return k;
    return XKB_KEY_PRINTABLE;
}

static void
xkb_typing_state_destroy(struct xkb_typing_state *state)
{
    free(state->keys_state);
    darray_free(state->pending);
    free(state->events);
}

enum xkb_status
xkb_typing_input_init(struct xkb_typing_input * restrict typing,
                      const struct xkb_key_set * restrict keys,
                      const struct xkb_typing_input_config * restrict config)
{
    typing->events = NULL;
    typing->num_events = 0;

    if (!config->input_length)
        return XKB_ERROR_INVALID;

    if (darray_empty(keys->printable) ||
        darray_empty(keys->modifiers) ||
        darray_empty(keys->misc))
    {
        return XKB_ERROR_INVALID;
    }

    enum xkb_status status = XKB_SUCCESS;

    /* NOTE: arrays are sorted because they were built using the key iterator */
    xkb_keycode_t keycode_max = darray_last(keys->printable, 0);
    keycode_max = MAX(
        darray_last(keys->modifiers, (struct xkb_modifier_key){0}).keycode,
        keycode_max
    );
    keycode_max = MAX(darray_last(keys->misc, 0), keycode_max);
    const size_t keys_state_num = bit_array_words((size_t)keycode_max + 1);
    uintptr_t *keys_state = calloc(keys_state_num, sizeof(*keys_state));

    struct xkb_typing_event *events = calloc(config->input_length, sizeof(*events));

    struct xkb_typing_state state = {
        .keys = keys,
        .keys_state = keys_state,
        .prng = config->prng,
        .prng_state = config->prng_state,
        .events = events,
        .num_events = config->input_length,
        .pending = darray_new(),
        .num_pending = 0,
    };

    xkb_typing_events pending = darray_new();

    if (!keys_state || !events) {
        status = XKB_ERROR_ALLOCATION_FAILURE;
        goto error;
    }

    /*
     * Invariant: e + state.num_pending <= state.num_events
     */

    for (size_t e = 0; e < state.num_events;) {
        /*
         * Process pending events
         */
        status = next_pending_events(&state, &pending);
        if (status != XKB_SUCCESS) {
            goto error;
        }
        struct xkb_typing_event *event;
        darray_foreach(event, pending) {
            assert(e < state.num_events);
            events[e] = *event;
            bit_array_assign(keys_state, event->keycode, (bool)event->direction);
            e++;
        }

        if (!state.num_pending && e == state.num_events - 1) {
            /*
             * Cannot add press/release *pair*, so just use an invalid keycode
             * for the last event.
             */
            events[e] = (struct xkb_typing_event) {
                .keycode = XKB_KEYCODE_INVALID,
                .direction = XKB_KEY_DOWN,
            };
            break;
        }

        if (e + 1 + state.num_pending >= state.num_events) {
            /* Cannot add new pair of events: require flushing pending events */
            continue;
        }

        /*
         * Create new events
         */

        /* Choose kind of key */
        static_assert(XKB_KEY_PRINTABLE == 0, "");
        const enum xkb_key_kind key_kind =
            pick_key_kind(config->prng, config->prng_state);
        switch (key_kind) {
        case XKB_KEY_PRINTABLE:
            status = add_other_key(
                &state, &state.keys->printable, &e,
                XKB_TYPING_EVENT_PRINTABLE_IMMEDIATE_RELEASE_PERCENT,
                XKB_TYPING_EVENT_PRINTABLE_MAX_PENDING_TURNS
            );
            break;
        case XKB_KEY_MODIFIER:
            status = add_modifier_key(&state, &e);
            break;
        case XKB_KEY_MISC:
            status = add_other_key(
                &state, &state.keys->misc, &e,
                XKB_TYPING_EVENT_MISC_IMMEDIATE_RELEASE_PERCENT,
                XKB_TYPING_EVENT_MISC_MAX_PENDING_TURNS
            );
            break;
        default: {
            static_assert(XKB_KEY_MISC == 2 &&
                          XKB_KEY_MISC == _NUM_XKB_KEY_KIND - 1, "");
            status = XKB_ERROR_INVALID;
        }}

        if (status != XKB_SUCCESS &&
            status != (enum xkb_status)XKB_ERROR_NO_RELEASED_KEY)
        {
            goto error;
        }
    }

    assert(!state.num_pending);

    typing->events = steal(&state.events);
    typing->num_events = state.num_events;

    status = XKB_SUCCESS;

error:
    darray_free(pending);
    xkb_typing_state_destroy(&state);

    return status;
}

void
xkb_typing_input_destroy(struct xkb_typing_input *typing)
{
    if (!typing)
        return;

    free(typing->events);
    typing->events = NULL;
}
