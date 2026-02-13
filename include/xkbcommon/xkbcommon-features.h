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
     * The enumeration @ref xkb_context_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_CONTEXT_FLAGS = 1100,
    /**
     * The enumeration @ref xkb_log_level
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_LOG_LEVEL = 1110,
    /**
     * The enumeration @ref xkb_keymap_format
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYMAP_FORMAT = 2000,
    /**
     * The enumeration @ref xkb_keymap_compile_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYMAP_COMPILE_FLAGS = 2010,
    /**
     * The enumeration @ref xkb_rmlvo_builder_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_RMLVO_BUILDER_FLAGS = 2020,
    /**
     * The enumeration @ref xkb_keymap_serialize_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYMAP_SERIALIZE_FLAGS = 2100,
    /**
     * The enumeration @ref xkb_keymap_key_iterator_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYMAP_KEY_ITERATOR_FLAGS = 2200,
    /**
     * The enumeration @ref xkb_keysym_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYSYM_FLAGS = 2300,
    /**
     * The enumeration @ref xkb_state_component
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_STATE_COMPONENT = 3000,
    /**
     * The enumeration @ref xkb_keyboard_controls
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYBOARD_CONTROLS = 3020,
    /**
     * The enumeration @ref xkb_state_accessibility_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_STATE_ACCESSIBILITY_FLAGS = 3100,
    /**
     * The enumeration @ref xkb_event_type
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_EVENT_TYPE = 3200,
    /**
     * The enumeration @ref xkb_key_direction
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEY_DIRECTION = 3300,
    /**
     * The enumeration @ref xkb_state_match
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_STATE_MATCH = 3400,
    /**
     * The enumeration @ref xkb_consumed_mode
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_CONSUMED_MODE = 3410,
    /**
     * The enumeration @ref xkb_compose_format
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_FORMAT = 4000,
    /**
     * The enumeration @ref xkb_compose_compile_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_COMPILE_FLAGS = 4010,
    /**
     * The enumeration @ref xkb_compose_state_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_STATE_FLAGS = 4100,
    /**
     * The enumeration @ref xkb_compose_status
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_STATUS = 4200,
    /**
     * The enumeration @ref xkb_compose_feed_result
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_FEED_RESULT = 4210,
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
