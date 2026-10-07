/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "config.h"

#include <stddef.h>
#include <stdint.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-status.h"
#include "darray.h"

enum xkb_modifier_kind {
    /** Shift, Ctrl, Alt: press — other key(s) — release */
    XKB_MOD_SET,
    /** sticky-keys tap: press — release — exactly one other key */
    XKB_MOD_LATCH,
    /** CapsLock, NumLock: press — release, toggles persistent state */
    XKB_MOD_LOCK,
};

enum xkb_key_kind {
    XKB_KEY_PRINTABLE,
    XKB_KEY_MODIFIER,
    XKB_KEY_MISC,
    _NUM_XKB_KEY_KIND,
};

struct xkb_modifier_key {
    xkb_keycode_t keycode;
    xkb_mod_mask_t mods;
    enum xkb_modifier_kind kind;
};

typedef darray(xkb_keycode_t) xkb_keycodes_t;

/** Keys classified by category */
struct xkb_key_set {
    /** Printable keys: alphanum, punctuation, symbols, etc. */
    xkb_keycodes_t printable;
    /** Modifier keys */
    darray(struct xkb_modifier_key) modifiers;
    /** Miscellaneous: arrows, editing, functions, system, etc. */
    xkb_keycodes_t misc;
};

struct xkb_key_set_config {
    xkb_keycode_t min;
    xkb_keycode_t max;
};

enum xkb_status
xkb_key_set_init(struct xkb_key_set *set,
                 struct xkb_keymap *keymap,
                 const struct xkb_key_set_config *config);

void
xkb_key_set_destroy(struct xkb_key_set *set);

struct xkb_typing_event {
    xkb_keycode_t keycode;
    enum xkb_key_direction direction;
};

typedef darray(struct xkb_typing_event) xkb_typing_events;

struct xkb_pending_typing_event {
    struct xkb_typing_event event;
    xkb_mod_mask_t mods;
    uint8_t counter;
};

/** PRNG: return values must be between 0 and 2^31 - 1 (see random()) */
typedef long (*xkb_prng_t)(void *);

struct xkb_typing_state {
    /** Keys used for typing, classified by category. Borrowed */
    const struct xkb_key_set *keys;
    /** Keys state: pressed (1) or released (0) */
    uintptr_t *keys_state;
    xkb_prng_t prng;
    /** Borrowed */
    void *prng_state;
    /** Realistic typing events */
    struct xkb_typing_event *events;
    size_t num_events;
    /** Pending events */
    darray(struct xkb_pending_typing_event) pending;
    darray_size_t num_pending;
    /** Effective mods */
    xkb_mod_mask_t mods;
};

enum {
    XKB_TYPING_EVENT_PRINTABLE_IMMEDIATE_RELEASE_PERCENT = 75,
    XKB_TYPING_EVENT_PRINTABLE_MAX_PENDING_TURNS = 1,
    XKB_TYPING_EVENT_MODIFIER_MIN_PENDING_TURNS = 2,
    XKB_TYPING_EVENT_MODIFIER_MAX_PENDING_TURNS = 10,
    XKB_TYPING_EVENT_MISC_IMMEDIATE_RELEASE_PERCENT = 100,
    XKB_TYPING_EVENT_MISC_MAX_PENDING_TURNS = 0,
};

struct xkb_typing_input {
    struct xkb_typing_event *events;
    size_t num_events;
};

struct xkb_typing_input_config {
    xkb_prng_t prng;
    void *prng_state;
    size_t input_length;
};

enum xkb_status
xkb_typing_input_init(struct xkb_typing_input *typing,
                      const struct xkb_key_set *keys,
                      const struct xkb_typing_input_config *config);

void
xkb_typing_input_destroy(struct xkb_typing_input *typing);
