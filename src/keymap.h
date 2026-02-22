/*
 * For MIT-open-group:
 * Copyright 1985, 1987, 1990, 1998  The Open Group
 *
 * For HPND
 * Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.
 *
 * For MIT:
 * Copyright © 2009 Dan Nicholson
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita
 *
 * SPDX-License-Identifier: MIT-open-group AND HPND AND MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 * Author: Dan Nicholson <dbn.lists@gmail.com>
 */
#pragma once

 /* Don't use compat names in internal code. */
#define _XKBCOMMON_COMPAT_H

#include "config.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

#include "xkbcommon/xkbcommon.h"

#include "darray.h"
#include "rmlvo.h"
#include "context.h"
#include "utils.h"

/* Note: imposed by the size of the xkb_layout_mask_t type (32).
 * This is more than enough though. */
#define XKB_MAX_GROUPS 32
#define XKB_ALL_GROUPS ((UINT64_C(1) << XKB_MAX_GROUPS) - UINT64_C(1))
/* Limit imposed by X11 */
#define XKB_MAX_GROUPS_X11 4

static inline xkb_layout_index_t
format_max_groups(enum xkb_keymap_format format)
{
    return (format == XKB_KEYMAP_FORMAT_TEXT_V1)
        ? XKB_MAX_GROUPS_X11
        : XKB_MAX_GROUPS;
}

/* Don't allow more leds than we can hold in xkb_led_mask_t. */
#define XKB_MAX_LEDS ((xkb_led_index_t) (sizeof(xkb_led_mask_t) * CHAR_BIT))

/* Don't allow more modifiers than we can hold in xkb_mod_mask_t. */
#define XKB_MAX_MODS ((xkb_mod_index_t) (sizeof(xkb_mod_mask_t) * CHAR_BIT))

enum {
    /** Mask of all possible modifiers */
    XKB_MOD_ALL = UINT32_MAX,
};

/* Special value to handle modMap None {…} */
#define XKB_MOD_NONE 0xffffffffU

enum mod_type {
    /** X11 core modifier */
    MOD_REAL = (1 << 0),
    /** A non-X11 core modifier */
    MOD_VIRT = (1 << 1),
    /** Any modifier */
    MOD_BOTH = (MOD_REAL | MOD_VIRT)
};
#define MOD_REAL_MASK_ALL ((xkb_mod_mask_t) 0x000000ff)

/** Predefined (AKA real, core, X11) modifiers. The order is important! */
enum real_mod_index {
    XKB_MOD_INDEX_SHIFT = 0,
    XKB_MOD_INDEX_CAPS,
    XKB_MOD_INDEX_CTRL,
    XKB_MOD_INDEX_MOD1,
    XKB_MOD_INDEX_MOD2,
    XKB_MOD_INDEX_MOD3,
    XKB_MOD_INDEX_MOD4,
    XKB_MOD_INDEX_MOD5,
    _XKB_MOD_INDEX_NUM_ENTRIES
};
static_assert(_XKB_MOD_INDEX_NUM_ENTRIES == 8, "Invalid X11 core modifiers");

enum xkb_action_type {
    ACTION_TYPE_NONE = 0,
    ACTION_TYPE_VOID, /* libxkbcommon extension */
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
    ACTION_TYPE_REDIRECT_KEY,
    ACTION_TYPE_UNSUPPORTED_LEGACY,
    ACTION_TYPE_PRIVATE,
    ACTION_TYPE_INTERNAL, /* Action specific and internal to xkbcommon */
    _ACTION_TYPE_NUM_ENTRIES
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
    ACTION_ACCEL = (1 << 8),
    ACTION_SAME_SCREEN = (1 << 9),
    ACTION_LOCK_ON_RELEASE = (1 << 10),
    ACTION_UNLOCK_ON_PRESS = (1 << 11),
    ACTION_LATCH_ON_PRESS = (1 << 12),
    ACTION_PENDING_COMPUTATION = (1 << 13),
};

/**
 * This is the general version of the *public* `xkb_keyboard_controls` enum.
 * We do not expose the following enum, as it does not make sense to expose
 * controls whose effects we do not support.
 * However, we should enforce both enum to share the same values.
 */
enum xkb_action_controls {
    CONTROL_REPEAT = (1 << 0),
    CONTROL_SLOW = (1 << 1),
    CONTROL_DEBOUNCE = (1 << 2),
    CONTROL_STICKY_KEYS = (1 << 3),
    CONTROL_MOUSE_KEYS = (1 << 4),
    CONTROL_MOUSE_KEYS_ACCEL = (1 << 5),
    CONTROL_AX = (1 << 6),
    CONTROL_AX_TIMEOUT = (1 << 7),
    CONTROL_AX_FEEDBACK = (1 << 8),
    CONTROL_BELL = (1 << 9),
    CONTROL_IGNORE_GROUP_LOCK = (1 << 10),
    /**
     * All the XKB Controls. If we ever introduce *internal* controls, this mask
     * should not include them.
     */
    CONTROL_ALL = \
        (CONTROL_REPEAT | CONTROL_SLOW | CONTROL_DEBOUNCE | \
         CONTROL_STICKY_KEYS | CONTROL_MOUSE_KEYS | CONTROL_MOUSE_KEYS_ACCEL | \
         CONTROL_AX | CONTROL_AX_TIMEOUT | CONTROL_AX_FEEDBACK | \
         CONTROL_BELL | CONTROL_IGNORE_GROUP_LOCK)
};

static_assert(
    CONTROL_STICKY_KEYS ==
    (enum xkb_action_controls) XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS,
    "Private value should match public API"
);

enum xkb_match_operation {
    MATCH_NONE,
    MATCH_ANY_OR_NONE,
    MATCH_ANY,
    MATCH_ALL,
    MATCH_EXACTLY,
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
    enum xkb_action_controls ctrls;
};

struct xkb_pointer_default_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    int8_t value;
};

struct xkb_switch_screen_action {
    enum xkb_action_type type;
    enum xkb_action_flags flags;
    int8_t screen;
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
    uint8_t button;
};

struct xkb_redirect_key_action {
    enum xkb_action_type type;
    xkb_keycode_t keycode;
    /*
     * Next 2 fields are only used for parsing and serializing.
     *
     * We do not use `struct xkb_mods` here, because *both* fields denote
     * *virtual* modifier indices, contrary to `xkb_mods`, where `mods` and
     * `mask` denote respectively virtual and real mods.
     *
     * Ideally we would use 2 `xkb_mods` structs, but this would increase the
     * `xkb_action` union type.
     */
    /** Affected virtual modifiers */
    xkb_mod_mask_t affect;
    /** State of the affected virtual modifiers */
    xkb_mod_mask_t mods;
};

struct xkb_private_action {
    enum xkb_action_type type;
    uint8_t data[7];
};

enum xkb_internal_action_flags {
    INTERNAL_BREAKS_GROUP_LATCH = (1 << 0),
    INTERNAL_BREAKS_MOD_LATCH = (1 << 1),
};

/* Action specific and internal to xkbcommon */
struct xkb_internal_action {
    enum xkb_action_type type;
    enum xkb_internal_action_flags flags;
    union {
        /* flag == INTERNAL_BREAKS_MOD_LATCH */
        xkb_mod_mask_t clear_latched_mods;
    };
};

union xkb_action {
    enum xkb_action_type type;
    struct xkb_mod_action mods;
    struct xkb_group_action group;
    struct xkb_controls_action ctrls;
    struct xkb_pointer_default_action dflt;
    struct xkb_switch_screen_action screen;
    struct xkb_pointer_action ptr;
    struct xkb_pointer_button_action btn;
    struct xkb_redirect_key_action redirect;
    struct xkb_private_action priv;
    struct xkb_internal_action internal;
};

struct xkb_key_type_entry {
    xkb_level_index_t level;
    struct xkb_mods mods;
    struct xkb_mods preserve;
};

struct xkb_key_type {
    xkb_atom_t name;
    struct xkb_mods mods;
    bool required;
    xkb_level_index_t num_levels;
    xkb_level_index_t num_level_names;
    xkb_atom_t *level_names;
    darray_size_t num_entries;
    struct xkb_key_type_entry *entries;
};

typedef uint16_t xkb_action_count_t;
#define MAX_ACTIONS_PER_LEVEL UINT16_MAX

struct xkb_sym_interpret {
    xkb_keysym_t sym;
    enum xkb_match_operation match;
    xkb_mod_mask_t mods;
    xkb_mod_index_t virtual_mod;
    bool level_one_only;
    bool repeat:1;
    bool required:1;
    xkb_action_count_t num_actions;
    union {
        /* num_actions <= 1 */
        union xkb_action action;
        /* num_actions >  1 */
        union xkb_action *actions;
    } a;
};

enum {XKB_STATE_COMPONENT_WIDTH = (sizeof(enum xkb_state_component) * CHAR_BIT)};
static_assert(
    (UINT64_C(1) << XKB_STATE_COMPONENT_WIDTH) - 1 > XKB_STATE_CONTROLS,
    "Cannot encode xkb_led::pending_groups"
);

struct xkb_led {
    xkb_atom_t name;
    enum xkb_state_component which_groups:(XKB_STATE_COMPONENT_WIDTH - 1);
    bool pending_groups:1;
    xkb_layout_mask_t groups;
    enum xkb_state_component which_mods;
    struct xkb_mods mods;
    enum xkb_action_controls ctrls;
};

struct xkb_key_alias {
    xkb_atom_t real;
    xkb_atom_t alias;
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

/* Such an awkward name.  Oh well. */
enum xkb_range_exceed_type {
    RANGE_WRAP = 0,
    RANGE_SATURATE,
    RANGE_REDIRECT,
};

enum xkb_explicit_components {
    EXPLICIT_SYMBOLS = (1 << 0),
    EXPLICIT_INTERP = (1 << 1),
    EXPLICIT_TYPES = (1 << 2),
    EXPLICIT_VMODMAP = (1 << 3),
    EXPLICIT_REPEAT = (1 << 4),
};

typedef uint16_t xkb_keysym_count_t;
#define MAX_KEYSYMS_PER_LEVEL UINT16_MAX

/** A key level */
struct xkb_level {
    /** Count of keysyms */
    xkb_keysym_count_t num_syms;
    /** Count of actions */
    xkb_action_count_t num_actions;
    /** Upper keysym */
    union {
        /** num_syms == 1: Upper keysym */
        xkb_keysym_t upper;
        /** num_syms >  1: Indicate if `syms` contains the upper case keysyms
         *                 after the lower ones. */
        bool has_upper;
    };
    /** Keysyms */
    union {
        xkb_keysym_t sym;          /* num_syms == 1 */
        xkb_keysym_t *syms;        /* num_syms > 1  */
    } s;
    /** Actions */
    union {
        union xkb_action action;   /* num_actions == 1 */
        union xkb_action *actions; /* num_actions >  1 */
    } a;
};

/**
 * Group in a key
 */
struct xkb_group {
    /**
     * Flag that indicates whether a group has explicit actions. In case it has,
     * compatibility interpretations will not be used on it.
     * See also EXPLICIT_INTERP flag at key level.
     */
    bool explicit_actions:1;
    /**
     * Flag that indicates whether a group has an explicit key type. In case it
     * has, type detection will not be used on it.
     */
    bool explicit_type:1;
    /**
     * Key type of the group. Points to an entry in keymap->types.
     */
    const struct xkb_key_type *type;
    /**
     * Array of group levels. Use `XkbKeyNumLevels` for the number of levels.
     */
    struct xkb_level *levels;
};

enum {
    DEFAULT_KEY_REPEAT = false,
    /**
     * Any key with no explicit key repeat value that trigger the default
     * interpret, e.g. some explicit symbols in a group with no action that do
     * not trigger any explicit interpret.
     */
    DEFAULT_INTERPRET_KEY_REPEAT = true,
    FALLBACK_INTERPRET_KEY_REPEAT = DEFAULT_KEY_REPEAT
};

enum {
    DEFAULT_KEY_VMODMAP = 0,
    DEFAULT_INTERPRET_VMOD = XKB_MOD_INVALID,
    DEFAULT_INTERPRET_VMODMAP = DEFAULT_KEY_VMODMAP,
    FALLBACK_INTERPRET_VMODMAP = DEFAULT_KEY_VMODMAP
};

struct xkb_key {
    xkb_keycode_t keycode;
    xkb_atom_t name;

    enum xkb_explicit_components explicit;

    xkb_mod_mask_t modmap;
    xkb_mod_mask_t vmodmap;

    bool repeats:1;

    bool out_of_range_pending_group:1;
    enum xkb_range_exceed_type out_of_range_group_action;
    xkb_layout_index_t out_of_range_group_number;

    xkb_layout_index_t num_groups;
    struct xkb_group *groups;
};

struct xkb_mod {
    xkb_atom_t name;
    enum mod_type type;
    xkb_mod_mask_t mapping; /* vmod -> real mod mapping */
};

struct xkb_mod_set {
    struct xkb_mod mods[XKB_MAX_MODS];
    xkb_mod_index_t num_mods;
    xkb_mod_mask_t explicit_vmods;
};

/*
 * Implementation with continuous arrays does not allow efficient mapping of
 * sparse keycodes. Indeed, allowing the API max valid keycode XKB_KEYCODE_MAX
 * could result in memory exhaustion or memory waste (sparse arrays) with huge
 * enough valid values. However usual keycodes form a contiguous range with a
 * that maximum that rarely exceeds 1000. In order to handle the whole keycode
 * range, we define a threshold for the continuous array storage, above which we
 * use a more space-efficient storage for sparse arrays.
 *
 * The current limit is arbitrary and serves as a minimal security.
 */
#define XKB_KEYCODE_MAX_CONTIGUOUS 0xfff
static_assert(XKB_KEYCODE_MAX_CONTIGUOUS < XKB_KEYCODE_MAX,
              "Valid keycodes");
static_assert(XKB_KEYCODE_MAX_CONTIGUOUS < darray_max_alloc(sizeof(xkb_atom_t)),
              "Max keycodes names");
static_assert(XKB_KEYCODE_MAX_CONTIGUOUS < darray_max_alloc(sizeof(struct xkb_key)),
              "Max keymap keys");

/*
 * Our current implementation with continuous arrays does not allow efficient
 * mapping of levels. Allowing the max valid level UINT32_MAX could result in
 * memory exhaustion or memory waste (sparse arrays) with huge enough valid
 * values. Let’s be more conservative for now. This value should be big enough
 * to satisfy automatically generated keymaps.
 */
#define XKB_LEVEL_MAX_IMPL 2048
static_assert(XKB_LEVEL_MAX_IMPL < XKB_LEVEL_INVALID,
              "Valid levels");
static_assert(XKB_LEVEL_MAX_IMPL < darray_max_alloc(sizeof(xkb_atom_t)),
              "Max key types level names");
static_assert(XKB_LEVEL_MAX_IMPL < darray_max_alloc(sizeof(struct xkb_level)),
              "Max keys levels");

static_assert((1LLU << (DARRAY_SIZE_T_WIDTH - 3)) - 1 >=
              XKB_KEYCODE_MAX_CONTIGUOUS,
              "Cannot store low keycodes");

/** Keycode store lookup result */
typedef union {
    struct {
        /** If true, the index is valid */
        bool found:1;
        bool:1;
        /** Whether the corresponding entry is an alias */
        bool is_alias:1;
        darray_size_t:(DARRAY_SIZE_T_WIDTH - 3);
    };
    /* Only if is_alias = false, for better semantics */
    struct {
        bool found:1;
        /**
         * Whether the corresponding entry is stored in the low or high table
         * of the keycode store
         */
        bool low:1;
        bool is_alias:1;
        /**
         * - xkb_keycodes: index of the entry in the keycode store
         * - xkb_symbols: index of the entry in xkb_keymap::keys array
         */
        darray_size_t index:(DARRAY_SIZE_T_WIDTH - 3);
    } key;
    /* Only if is_alias = true, for better semantics */
    struct {
        bool found:1;
        bool:1;
        bool is_alias:1;
        /** Real name of the target key */
        xkb_atom_t real:(DARRAY_SIZE_T_WIDTH - 3);
    } alias;
} KeycodeMatch;

/** Common keyboard description structure */
struct xkb_keymap {
    struct xkb_context *ctx;

    int refcnt;
    enum xkb_keymap_compile_flags flags;
    enum xkb_keymap_format format;

    enum xkb_action_controls enabled_ctrls;

    xkb_keycode_t min_key_code;
    xkb_keycode_t max_key_code;
    xkb_keycode_t num_keys;
    /*
     * Keys are divided into 2 ordered (possibly empty) lists:
     *
     *   Low keycodes (≤ XKB_KEYCODE_MAX_CONTIGUOUS)
     *     Stored contiguously at indexes [0..num_keys_low).
     *     Fast O(1) access.
     *   High keycodes (> XKB_KEYCODE_MAX_CONTIGUOUS)
     *     Stored noncontiguously at indexes [num_keys_low..num_keys).
     *     Slow access via a binary search.
     */
    xkb_keycode_t num_keys_low;
    struct xkb_key *keys;

    union {
        /**
         * Keycode name atom -> key index lookup table
         *
         * ⚠️ Used only *during* compilation
         *
         * Given that:
         * - the number of atoms is usually < 1k;
         * - the keycode section usually appears first;
         * then the first atoms will be mostly the keycodes and their aliases,
         * so we can achieve O(1) lookup of the key names by using a simple array.
         */
        struct {
            darray_size_t num_key_names;
            KeycodeMatch *key_names;
        };
        /**
         * Aliases in no particular order
         *
         * ⚠️ Used only *after* compilation
         */
        struct {
            darray_size_t num_key_aliases;
            struct xkb_key_alias *key_aliases;
        };
    };

    struct xkb_key_type *types;
    darray_size_t num_types;

    darray_size_t num_sym_interprets;
    struct xkb_sym_interpret *sym_interprets;

    /**
     * Modifiers configuration.
     * This is *internal* to the keymap; other implementations may use different
     * virtual modifiers indices. Ours depends on:
     *   1. the order of the parsing of the keymap components;
     *   2. the order of the virtual modifiers declarations;
     */
    struct xkb_mod_set mods;
    /**
     * Modifier mask of the *canonical* state, i.e. the mask with the *smallest*
     * population count that denotes all bits used to encode the modifiers in
     * the keyboard state. It is equal to the bitwise OR of *real* modifiers and
     * all *virtual* modifiers mappings.
     *
     * [WARNING] The bits that do not correspond to *real* modifiers should
     * *not* be interpreted as corresponding to indices of virtual modifiers of
     * the keymap. Indeed, one may use explicit vmod mapping with an arbitrary
     * value.
     *
     * E.g. if M1 is the only vmod and it is defined by:
     *
     *     virtual_modifiers M1=0x80000000; // 1 << (32 - 1)
     *
     * then the 32th bit of a modifier mask input does *not* denote the 32th
     * virtual modifier of the keymap, but merely the encoding of the mapping of
     * M1.
     *
     * In the API, any input mask should be preprocessed to resolve the bits
     * that do not match the canonical mask.
     */
    xkb_mod_mask_t canonical_state_mask;

    /** Encode the “auto” special value of RedirectKey() */
    xkb_keycode_t redirect_key_auto;

    /* This field has 2 uses:
     * • During parsing: Expected layouts count after RMLVO resolution, if any;
     * • After parsing: Number of groups in the key with the most groups. */
    xkb_layout_index_t num_groups;
    /* Not all groups must have names. */
    xkb_layout_index_t num_group_names;
    xkb_atom_t *group_names;

    struct xkb_led leds[XKB_MAX_LEDS];
    xkb_led_index_t num_leds;

    char *keycodes_section_name;
    char *symbols_section_name;
    char *types_section_name;
    char *compat_section_name;
};

enum {
    _LAST_XKB_EVENT_TYPE = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
};

#define xkb_keys_foreach(iter, keymap) \
    /*
     * Start at the first defined low or high keycode:
     * - if there are some low keycodes, the index is min_key_code, because we
     *   skip the previous undefined keycodes;
     * - else the first item is the first high keycode.
     */ \
    for ((iter) = (keymap)->keys + \
                  ((keymap)->num_keys_low == 0 ? 0 : (keymap)->min_key_code); \
         (iter) < (keymap)->keys + (keymap)->num_keys; \
         (iter)++)

#define xkb_mods_foreach(iter, mods_) \
    for ((iter) = (mods_)->mods; \
         (iter) < (mods_)->mods + (mods_)->num_mods; \
         (iter)++)

#define xkb_mods_mask_foreach(mask, iter, mods_) \
    for ((iter) = (mods_)->mods; \
         (mask) && (iter) < (mods_)->mods + (mods_)->num_mods; \
         (iter)++, (mask) >>= 1) \
        if ((mask) & 0x1)

/** Enumerate all modifiers */
#define xkb_mods_enumerate(idx, iter, mods_) \
    for ((idx) = 0, (iter) = (mods_)->mods; \
         (idx) < (mods_)->num_mods; \
         (idx)++, (iter)++)

/** Enumerate only real modifiers */
#define xkb_rmods_enumerate(idx, iter, mods_) \
    for ((idx) = 0, (iter) = (mods_)->mods; \
         (idx) < _XKB_MOD_INDEX_NUM_ENTRIES; \
         (idx)++, (iter)++)

/** Enumerate only virtual modifiers */
#define xkb_vmods_enumerate(idx, iter, mods_) \
    for ((idx) = _XKB_MOD_INDEX_NUM_ENTRIES, \
         (iter) = &(mods_)->mods[_XKB_MOD_INDEX_NUM_ENTRIES]; \
         (idx) < (mods_)->num_mods; \
         (idx)++, (iter)++)

#define xkb_leds_foreach(iter, keymap) \
    for ((iter) = (keymap)->leds; \
         (iter) < (keymap)->leds + (keymap)->num_leds; \
         (iter)++)

#define xkb_leds_enumerate(idx, iter, keymap) \
    for ((idx) = 0, (iter) = (keymap)->leds; \
         (idx) < (keymap)->num_leds; \
         (idx)++, (iter)++)

void
clear_level(struct xkb_level *leveli);

/* ⚠️ Only valid before copying symbols to keymap */
static inline struct xkb_key *
XkbKeyByName(const struct xkb_keymap *keymap, xkb_atom_t name, bool use_aliases)
{
    if (name < keymap->num_key_names) {
        const KeycodeMatch match = keymap->key_names[name];
        if (match.found) {
            if (!match.is_alias) {
                assert(name == keymap->keys[match.key.index].name);
                return &keymap->keys[match.key.index];
            } else if (use_aliases) {
                assert(match.alias.real ==
                       keymap->keys[keymap->key_names[match.alias.real].key.index].name);
                return &keymap->keys[keymap->key_names[match.alias.real].key.index];
            }
        }
    }
    return NULL;
}

static inline const struct xkb_key *
XkbKey(struct xkb_keymap *keymap, xkb_keycode_t kc)
{
    if (kc < keymap->min_key_code || kc > keymap->max_key_code) {
        /* Unsupported keycode */
        return NULL;
    } else if (kc < keymap->num_keys_low) {
        /* Low keycodes */
        return &keymap->keys[kc];
    } else {
        /* High keycodes: use binary search */
        xkb_keycode_t lower = keymap->num_keys_low;
        xkb_keycode_t upper = keymap->num_keys;
        while (lower < upper) {
            const xkb_keycode_t mid = lower + (upper - 1 - lower) / 2;
            const struct xkb_key * const key = &keymap->keys[mid];
            if (key->keycode < kc) {
                lower = mid + 1;
            } else if (key->keycode > kc) {
                upper = mid;
            } else {
                return key;
            }
        }
        return NULL;
    }
}

static inline xkb_level_index_t
XkbKeyNumLevels(const struct xkb_key *key, xkb_layout_index_t layout)
{
    return key->groups[layout].type->num_levels;
}

/*
 * Map entries which specify unbound virtual modifiers are not considered.
 * See: the XKB protocol, section “Determining the KeySym Associated with a Key
 * Event”
 *
 * xserver does this with cached entry->active field.
 */
static inline bool
entry_is_active(const struct xkb_key_type_entry *entry)
{
    return entry->mods.mods == 0 || entry->mods.mask != 0;
}

struct xkb_keymap *
xkb_keymap_new(struct xkb_context *ctx, const char* func,
               enum xkb_keymap_format format,
               enum xkb_keymap_compile_flags flags);

void
XkbEscapeMapName(char *name);

xkb_mod_index_t
XkbModNameToIndex(const struct xkb_mod_set *mods, xkb_atom_t name,
                  enum mod_type type);

bool
XkbLevelsSameSyms(const struct xkb_level *a, const struct xkb_level *b);

bool
action_equal(const union xkb_action *a, const union xkb_action *b);

bool
XkbLevelsSameActions(const struct xkb_level *a, const struct xkb_level *b);

xkb_layout_index_t
XkbWrapGroupIntoRange(int32_t group,
                      xkb_layout_index_t num_groups,
                      enum xkb_range_exceed_type out_of_range_group_action,
                      xkb_layout_index_t out_of_range_group_number);

XKB_EXPORT_PRIVATE xkb_mod_mask_t
mod_mask_get_effective(struct xkb_keymap *keymap, xkb_mod_mask_t mods);

struct xkb_level *
xkb_keymap_key_get_level(struct xkb_keymap *keymap, const struct xkb_key *key,
                         xkb_layout_index_t layout, xkb_level_index_t level);

xkb_action_count_t
xkb_keymap_key_get_actions_by_level(struct xkb_keymap *keymap,
                                    const struct xkb_key *key,
                                    xkb_layout_index_t layout,
                                    xkb_level_index_t level,
                                    const union xkb_action **actions);

struct xkb_keymap_format_ops {
    bool (*keymap_new_from_rmlvo)(struct xkb_keymap *keymap,
                                  const struct xkb_rmlvo_builder *rmlvo);
    bool (*keymap_new_from_names)(struct xkb_keymap *keymap,
                                  const struct xkb_rule_names *names);
    bool (*keymap_new_from_string)(struct xkb_keymap *keymap,
                                   const char *string, size_t length);
    bool (*keymap_new_from_file)(struct xkb_keymap *keymap, FILE *file);
    char *(*keymap_get_as_string)(struct xkb_keymap *keymap,
                                  enum xkb_keymap_format format,
                                  enum xkb_keymap_serialize_flags flags);
};

extern const struct xkb_keymap_format_ops text_v1_keymap_format_ops;

static inline bool
isModsUnLockOnPressSupported(enum xkb_keymap_format format)
{
    /* Lax bound */
    return format >= XKB_KEYMAP_FORMAT_TEXT_V2;
}

static inline bool
isGroupLockOnReleaseSupported(enum xkb_keymap_format format)
{
    /* Lax bound */
    return format >= XKB_KEYMAP_FORMAT_TEXT_V2;
}

static inline bool
isModsLatchOnPressSupported(enum xkb_keymap_format format)
{
    /* Lax bound */
    return format >= XKB_KEYMAP_FORMAT_TEXT_V2;
}
