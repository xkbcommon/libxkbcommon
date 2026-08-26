/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "config.h"

#include <assert.h>
#include <stddef.h>

#include "xkbcommon/xkbcommon.h"
#include "abi-check.h"

/******************************************************************************
 * xkb_keymap_serialize_config
 *****************************************************************************/

/**
 * Version 1 of `xkb_keymap_serialize_config`, used for ABI check only
 *
 * @since 1.14.0
 */
struct xkb_keymap_serialize_config_v1 {
    size_t size;
    uint32_t flags;
    uint32_t format;
    xkb_layout_mask_t layouts;
    uint32_t reserved;
};

/* Ensure there is no implicit padding */
assert_no_padding(struct xkb_keymap_serialize_config, size, flags);
assert_no_padding(struct xkb_keymap_serialize_config, flags, format);
assert_no_padding(struct xkb_keymap_serialize_config, format, layouts);
assert_no_padding(struct xkb_keymap_serialize_config, layouts, reserved);
assert_no_padding(struct xkb_keymap_serialize_config, reserved);

/* Current version is 1 */
static_assert(sizeof(struct xkb_keymap_serialize_config) ==
              sizeof(struct xkb_keymap_serialize_config_v1), "");

/* Ensure reasonable margin to the upper size limit */
static_assert(sizeof(struct xkb_keymap_serialize_config) * 30 <=
              (size_t)XKB_ABI_MAX_SIZE, "");

/******************************************************************************
 * xkb_keymap_serialize_result
 *****************************************************************************/

/**
 * Version 1 of `xkb_keymap_serialize_result`, used for ABI check only
 *
 * @since 1.14.0
 */
struct xkb_keymap_serialize_result_v1 {
    size_t size;
    char *serialized;
    size_t length;
    xkb_layout_mask_t layouts;
    uint32_t reserved;
};

/* Ensure there is no implicit padding */
assert_no_padding(struct xkb_keymap_serialize_result, size, serialized);
// NOLINTBEGIN(bugprone-sizeof-expression)
assert_no_padding(struct xkb_keymap_serialize_result, serialized, length);
// NOLINTEND(bugprone-sizeof-expression)
assert_no_padding(struct xkb_keymap_serialize_result, length, layouts);
assert_no_padding(struct xkb_keymap_serialize_result, layouts, reserved);
assert_no_padding(struct xkb_keymap_serialize_result, reserved);

/* Current version is 1 */
static_assert(sizeof(struct xkb_keymap_serialize_result) ==
              sizeof(struct xkb_keymap_serialize_result_v1), "");

/* Ensure reasonable margin to the upper size limit */
static_assert(sizeof(struct xkb_keymap_serialize_result) * 30 <=
              (size_t)XKB_ABI_MAX_SIZE, "");

/******************************************************************************
 * Utils
 *****************************************************************************/

/** Size of the *first version* of the struct */
#define xkb_versioned_struct_size_v1(x) _Generic(      \
    (x),                                               \
    const struct xkb_keymap_serialize_config *:        \
        sizeof(struct xkb_keymap_serialize_config_v1), \
    const struct xkb_keymap_serialize_result *:        \
        sizeof(struct xkb_keymap_serialize_result_v1)  \
)

/** Minimal *current* valid size of the struct */
#define xkb_versioned_struct_size_min(x) _Generic(     \
    (x),                                               \
    const struct xkb_keymap_serialize_config *:        \
        sizeof(struct xkb_keymap_serialize_config_v1), \
    const struct xkb_keymap_serialize_result *:        \
        sizeof(struct xkb_keymap_serialize_result_v1)  \
)

/** Offset of the last meaningful field of the current struct */
#define xkb_versioned_struct_reserved_offset(x) _Generic(      \
    (x),                                                       \
    const struct xkb_keymap_serialize_config *:                \
        offsetof(struct xkb_keymap_serialize_config, reserved),\
    const struct xkb_keymap_serialize_result *:                \
        offsetof(struct xkb_keymap_serialize_result, reserved) \
)

/* V1 is the smallest struct version */
static_assert(
    xkb_versioned_struct_size_v1(((const struct xkb_keymap_serialize_config *)NULL)) <=
    xkb_versioned_struct_size_min(((const struct xkb_keymap_serialize_config *)NULL)),
    ""
);

/* Minimal size is lower or equal to the current size */
static_assert(
    xkb_versioned_struct_size_min(((const struct xkb_keymap_serialize_config *)NULL)) <=
    sizeof(const struct xkb_keymap_serialize_config),
    ""
);

#define xkb_check_keymap_serialize_struct_size(x) xkb_check_versioned_struct_size( \
    xkb_versioned_struct_size_v1(x),                                               \
    xkb_versioned_struct_size_min(x),                                              \
    xkb_versioned_struct_reserved_offset(x),                                       \
    (x)                                                                            \
)
