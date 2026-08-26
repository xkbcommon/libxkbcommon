/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "config.h"

#include <assert.h>
#include <stdalign.h>
#include <stddef.h>

#include "xkbcommon/xkbcommon.h"
#include "abi-check.h"
#include "keymap.h"

/** Core keyboard state components */
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

    enum xkb_action_controls controls; /**< effective */
};

struct xkb_event {
    enum xkb_event_type type;
    union {
        xkb_keycode_t keycode;
        struct {
            struct state_components components;
            enum xkb_state_component changed;
        } components;
    };
};

/**
 * Version 1 of `xkb_state_components_update`, used for ABI check only
 *
 * @since 1.14.0
 */
struct xkb_state_components_update_v1 {
    uint32_t size;
    uint32_t components;
    xkb_mod_mask_t affect_latched_mods;
    xkb_mod_mask_t latched_mods;
    xkb_mod_mask_t affect_locked_mods;
    xkb_mod_mask_t locked_mods;
    int32_t latched_layout;
    int32_t locked_layout;
    uint32_t affect_controls;
    uint32_t controls;
};

/* Ensure there is no implicit padding */
assert_no_padding(struct xkb_state_components_update, size, components);
assert_no_padding(struct xkb_state_components_update, components, affect_latched_mods);
assert_no_padding(struct xkb_state_components_update, affect_latched_mods, latched_mods);
assert_no_padding(struct xkb_state_components_update, latched_mods, affect_locked_mods);
assert_no_padding(struct xkb_state_components_update, affect_locked_mods, locked_mods);
assert_no_padding(struct xkb_state_components_update, locked_mods, latched_layout);
assert_no_padding(struct xkb_state_components_update, latched_layout, locked_layout);
assert_no_padding(struct xkb_state_components_update, locked_layout, affect_controls);
assert_no_padding(struct xkb_state_components_update, affect_controls, controls);
assert_no_padding(struct xkb_state_components_update, controls);

/* Current version is 1 */
static_assert(sizeof(struct xkb_state_components_update) ==
              sizeof(struct xkb_state_components_update_v1), "");
assert_same_field(struct xkb_state_components_update, _v1, size);
assert_same_field(struct xkb_state_components_update, _v1, components);
assert_same_field(struct xkb_state_components_update, _v1, affect_latched_mods);
assert_same_field(struct xkb_state_components_update, _v1, latched_mods);
assert_same_field(struct xkb_state_components_update, _v1, affect_locked_mods);
assert_same_field(struct xkb_state_components_update, _v1, locked_mods);
assert_same_field(struct xkb_state_components_update, _v1, latched_layout);
assert_same_field(struct xkb_state_components_update, _v1, locked_layout);
assert_same_field(struct xkb_state_components_update, _v1, affect_controls);
assert_same_field(struct xkb_state_components_update, _v1, controls);

/* Ensure reasonable margin to the upper size limit */
static_assert(sizeof(struct xkb_state_components_update) * 30 <=
              (size_t)XKB_ABI_MAX_SIZE, "");

/**
 * Version 1 of `xkb_layout_policy_update`, used for ABI check only
 *
 * @since 1.14.0
 */
struct xkb_layout_policy_update_v1 {
    uint32_t size;
    uint32_t policy;
    xkb_layout_index_t redirect;
};

/* Ensure there is no implicit padding */
assert_no_padding(struct xkb_layout_policy_update, size, policy);
assert_no_padding(struct xkb_layout_policy_update, policy, redirect);
assert_no_padding(struct xkb_layout_policy_update, redirect);

/* Current version is 1 */
static_assert(sizeof(struct xkb_layout_policy_update) ==
              sizeof(struct xkb_layout_policy_update_v1), "");
assert_same_field(struct xkb_layout_policy_update, _v1, size);
assert_same_field(struct xkb_layout_policy_update, _v1, policy);
assert_same_field(struct xkb_layout_policy_update, _v1, redirect);

/* Ensure reasonable margin to the upper size limit */
static_assert(sizeof(struct xkb_layout_policy_update) * 30 <=
              (size_t)XKB_ABI_MAX_SIZE, "");

/**
 * Version 1 of `xkb_state_update`, used for ABI check only
 *
 * @since 1.14.0
 */
struct xkb_state_update_v1 {
    uint32_t size;
    uint32_t reserved0;
    const struct xkb_state_components_update_v1 *components;
    const struct xkb_layout_policy_update_v1 *layout_policy;
};

/* Ensure there is no implicit padding */
assert_no_padding(struct xkb_state_update, size, reserved0);
assert_no_padding(struct xkb_state_update, reserved0, components);
// NOLINTBEGIN(bugprone-sizeof-expression)
assert_no_padding(struct xkb_state_update, components, layout_policy);
assert_no_padding(struct xkb_state_update, layout_policy);
// NOLINTEND(bugprone-sizeof-expression)

/* Current version is 1 */
static_assert(sizeof(struct xkb_state_update) ==
              sizeof(struct xkb_state_update_v1), "");
assert_same_field(struct xkb_state_update, _v1, size);
assert_same_field(struct xkb_state_update, _v1, reserved0);
// NOLINTBEGIN(bugprone-sizeof-expression)
assert_same_field(struct xkb_state_update, _v1, components);
assert_same_field(struct xkb_state_update, _v1, layout_policy);
// NOLINTEND(bugprone-sizeof-expression)

/* Ensure reasonable margin to the upper size limit */
static_assert(sizeof(struct xkb_state_update) * 30 <=
              (size_t)XKB_ABI_MAX_SIZE, "");

/** Size of the *first version* of the struct */
#define xkb_versioned_struct_size_v1(x) _Generic(      \
    (x),                                               \
    const struct xkb_state_update *:                   \
        sizeof(struct xkb_state_update_v1),            \
    const struct xkb_state_components_update *:        \
        sizeof(struct xkb_state_components_update_v1), \
    const struct xkb_layout_policy_update *:           \
        sizeof(struct xkb_layout_policy_update_v1)     \
)

/** Minimal *current* valid size of the struct */
#define xkb_versioned_struct_size_min(x) _Generic(     \
    (x),                                               \
    const struct xkb_state_update *:                   \
        sizeof(struct xkb_state_update_v1),            \
    const struct xkb_state_components_update *:        \
        sizeof(struct xkb_state_components_update_v1), \
    const struct xkb_layout_policy_update *:           \
        sizeof(struct xkb_layout_policy_update_v1)     \
)

/** Offset of the last meaningful field of the current struct */
#define xkb_versioned_struct_reserved_offset(x) _Generic( \
    (x),                                                  \
    const struct xkb_state_update *:                      \
        sizeof(struct xkb_state_update),                  \
    const struct xkb_state_components_update *:           \
        sizeof(struct xkb_state_components_update),       \
    const struct xkb_layout_policy_update *:              \
        sizeof(struct xkb_layout_policy_update)           \
)

/* V1 is the smallest struct version */
static_assert(
    xkb_versioned_struct_size_v1(((const struct xkb_state_update *)NULL)) <=
    xkb_versioned_struct_size_min(((const struct xkb_state_update *)NULL)),
    ""
);
static_assert(
    xkb_versioned_struct_size_v1(((const struct xkb_state_components_update *)NULL)) <=
    xkb_versioned_struct_size_min(((const struct xkb_state_components_update *)NULL)),
    ""
);
static_assert(
    xkb_versioned_struct_size_v1(((const struct xkb_layout_policy_update *)NULL)) <=
    xkb_versioned_struct_size_min(((const struct xkb_layout_policy_update *)NULL)),
    ""
);

/* Minimal size is lower or equal to the current size */
static_assert(
    xkb_versioned_struct_size_min(((const struct xkb_state_update *)NULL)) <=
    sizeof(const struct xkb_state_update),
    ""
);
static_assert(
    xkb_versioned_struct_size_min(((const struct xkb_state_components_update *)NULL)) <=
    sizeof(const struct xkb_state_components_update),
    ""
);
static_assert(
    xkb_versioned_struct_size_min(((const struct xkb_layout_policy_update *)NULL)) <=
    sizeof(const struct xkb_layout_policy_update),
    ""
);

/** Reference count is not updated */
XKB_EXPORT_PRIVATE struct xkb_state *
xkb_machine_get_state(struct xkb_machine *machine);
