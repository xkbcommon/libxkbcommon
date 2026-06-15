/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "config.h"

#include "xkbcommon/xkbcommon.h"
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
    size_t size;
    enum xkb_state_component components;
    xkb_mod_mask_t affect_latched_mods;
    xkb_mod_mask_t latched_mods;
    xkb_mod_mask_t affect_locked_mods;
    xkb_mod_mask_t locked_mods;
    int32_t latched_layout;
    int32_t locked_layout;
    enum xkb_keyboard_control_flags affect_controls;
    enum xkb_keyboard_control_flags controls;

#if SIZE_MAX > UINT32_MAX
    /* Reserved for ABI alignment. Must be zero. */
    uint32_t _reserved;
#endif
};

/* Ensure there is no implicit padding */
static_assert(sizeof(struct xkb_state_components_update_v1) ==
              sizeof(size_t)                          /* size                */
            + sizeof(enum xkb_state_component)        /* components          */
            + sizeof(xkb_mod_mask_t)                  /* affect_latched_mods */
            + sizeof(xkb_mod_mask_t)                  /* latched_mods        */
            + sizeof(xkb_mod_mask_t)                  /* affect_locked_mods  */
            + sizeof(xkb_mod_mask_t)                  /* locked_mods         */
            + sizeof(int32_t)                         /* latched_layout      */
            + sizeof(int32_t)                         /* locked_layout       */
            + sizeof(enum xkb_keyboard_control_flags) /* affect_controls     */
            + sizeof(enum xkb_keyboard_control_flags) /* controls            */
#if SIZE_MAX > UINT32_MAX
            + sizeof(uint32_t)                        /* _reserved           */
#endif
            , "struct xkb_state_components_update_v1 has implicit padding");

/* Current version is 1 */
static_assert(sizeof(struct xkb_state_components_update) ==
              sizeof(struct xkb_state_components_update_v1), "");

/**
 * Version 1 of `xkb_layout_policy_update`, used for ABI check only
 *
 * @since 1.14.0
 */
struct xkb_layout_policy_update_v1 {
    size_t size;
    enum xkb_layout_out_of_range_policy policy;
    xkb_layout_index_t redirect;
};

/* Ensure there is no implicit padding */
static_assert(sizeof(struct xkb_layout_policy_update_v1) ==
              sizeof(size_t)                               /* size     */
            + sizeof(enum xkb_layout_out_of_range_policy)  /* policy   */
            + sizeof(xkb_layout_index_t),                  /* redirect */
              "struct xkb_layout_policy_update_v1 has implicit padding");

/* Current version is 1 */
static_assert(sizeof(struct xkb_layout_policy_update) ==
              sizeof(struct xkb_layout_policy_update_v1), "");

/**
 * Version 1 of `xkb_state_update`, used for ABI check only
 *
 * @since 1.14.0
 */
struct xkb_state_update_v1 {
    size_t size;
    const struct xkb_state_components_update_v1 *components;
    const struct xkb_layout_policy_update_v1 *layout_policy;
};

/* Ensure there is no implicit padding */
static_assert(sizeof(struct xkb_state_update_v1) ==
              sizeof(size_t)                          /* size          */
            + sizeof((void *)0)                       /* components    */
            + sizeof((void *)0),                      /* layout_policy */
              "struct xkb_state_update_v1 has implicit padding");

/* Current version is 1 */
static_assert(sizeof(struct xkb_state_update) ==
              sizeof(struct xkb_state_update_v1), "");

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
