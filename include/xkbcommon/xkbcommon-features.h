/*
 * Copyright © 2026 Pierre Le Marre
 *
 * SPDX-License-Identifier: MIT
 *
 * Author: Pierre Le Marre <dev@wismill.eu>
 */

#ifndef _XKBCOMMON_FEATURES_H_
#define _XKBCOMMON_FEATURES_H_

#include <xkbcommon/xkbcommon.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup test-api Features availability
 * Test features availability
 *
 * @{
 */

/**
 * @enum xkb_feature
 *
 * Enumerate all the features testable with `xkb_has_feature()`.
 *
 * @since 1.14.0
 */
enum xkb_feature {
    /**
     * The enumeration @ref xkb_feature
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_FEATURE = 1000,
    /**
     * The enumeration @ref rmlvo_builder_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_RMLVO_BUILDER_FLAGS = 1001,
    /**
     * The enumeration @ref keysym_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYSYM_FLAGS = 1002,
    /**
     * The enumeration @ref context_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_CONTEXT_FLAGS = 1003,
    /**
     * The enumeration @ref log_level
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_LOG_LEVEL = 1004,
    /**
     * The enumeration @ref keymap_compile_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYMAP_COMPILE_FLAGS = 1005,
    /**
     * The enumeration @ref keymap_format
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYMAP_FORMAT = 1006,
    /**
     * The enumeration @ref keymap_serialize_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYMAP_SERIALIZE_FLAGS = 1007,
    /**
     * The enumeration @ref keymap_key_iterator_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYMAP_KEY_ITERATOR_FLAGS = 1008,
    /**
     * The enumeration @ref state_accessibility_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_STATE_ACCESSIBILITY_FLAGS = 1009,
    /**
     * The enumeration @ref event_type
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_EVENT_TYPE = 1010,
    /**
     * The enumeration @ref state_component
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_STATE_COMPONENT = 1011,
    /**
     * The enumeration @ref keyboard_controls
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYBOARD_CONTROLS = 1012,
    /**
     * The enumeration @ref key_direction
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEY_DIRECTION = 1013,
    /**
     * The enumeration @ref state_match
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_STATE_MATCH = 1014,
    /**
     * The enumeration @ref consumed_mode
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_CONSUMED_MODE = 1015,
    /**
     * The enumeration @ref compose_compile_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_COMPILE_FLAGS = 1016,
    /**
     * The enumeration @ref compose_format
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_FORMAT = 1017,
    /**
     * The enumeration @ref compose_state_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_STATE_FLAGS = 1018,
    /**
     * The enumeration @ref compose_status
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_STATUS = 1019,
    /**
     * The enumeration @ref compose_feed_result
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_FEED_RESULT = 1020,
};

/**
 * Test feature availability.
 *
 * This is useful when the library is dynamically linked.
 *
 * @param feature  The feature to check
 * @param value    The value to check if @p feature denotes an enumeration,
 * otherwise this argument is ignored.
 *
 * @returns `true` if the feature is supported, false otherwise.
 *
 * @since 1.14.0
 */
XKB_EXPORT bool
xkb_has_feature(enum xkb_feature feature, int value);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _XKBCOMMON_FEATURES_H_ */
