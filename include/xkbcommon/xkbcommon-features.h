/*
 * Copyright © 2026 Pierre Le Marre
 *
 * SPDX-License-Identifier: MIT
 *
 * Author: Pierre Le Marre <dev@wismill.eu>
 */

#ifndef _XKBCOMMON_FEATURES_H_
#define _XKBCOMMON_FEATURES_H_

#include <stdint.h>

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

/*
 * Numeric namespace for `xkb_feature` enumerators
 * ───────────────────────────────────────────────
 *
 * Values are opaque constants. The segmented decimal namespace is purely
 * organizational and is never meant to be parsed or decomposed at runtime.
 *
 * Values follow the pattern `AABCD`:
 *
 * | Numeric pattern | Segment    | Description                                                  |
 * | --------------- | ---------- | ------------------------------------------------------------ |
 * | AA000           | Module     | Top-level object; unallocated slots are independent spares   |
 * | 00B00           | Sub-module | Functional category; primary slots at ×200, companions +100  |
 * | 000CD           | Feature    | Fine-grained index within a sub-module; related pairs at +20 |
 *
 * Current module assignments:
 *
 *   Slot    Module
 *   ──────────────────────────
 *      0    Core / global
 *      1    Error
 *      2    (reserved for error or context-related object)
 *      3    Context
 *      4    (reserved for context-related object)
 *      5    Logging
 *    6-8    (reserved for logging or core keymap-related objects)
 *      9    Keysym
 *  10-17    (reserved for core keymap-related objects and keymap builders)
 *     18    RMLVO
 *  19-20    (reserved for core keymap-related objects and keymap builders)
 *     21    Keymap
 *  22-23    (reserved for keymap and state -related objects)
 *     24    Keyboard state components
 *     25    Keyboard state machine (reserved)
 *     26    (reserved for state-related object)
 *     27    Keyboard events / input
 *  28-29    (spare)
 *     30    Compose table
 *     31    Compose state
 *     32    (spare)
 *
 * Cross sub-modules:
 *
 *   Slot    Sub-module
 *   ──────────────────────────
 *      0    General / components / properties
 *    100    General / components / properties
 *    200    Creation / parsing
 *    300    Update
 *    400    Serialization
 *    500    (spare / module-specific)
 *    600    Iterator
 *    700    (spare / module-specific)
 *    800    Query
 *    900    (spare / module-specific)
 *
 * The current values are meant to leave enough room for future additions.
 */

/**
 * @enum xkb_feature
 *
 * Enumerate all the features testable with `xkb_feature_supported()`.
 *
 * @since 1.14.0
 */
enum xkb_feature {
    /**
     * The enumeration @ref xkb_feature
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_FEATURE = 1,
    /**
     * The enumeration @ref xkb_error_code
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_ERROR_CODE = 1000,
    /**
     * The enumeration @ref xkb_context_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_CONTEXT_FLAGS = 3200,
    /**
     * The enumeration @ref xkb_log_level
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_LOG_LEVEL = 5100,
    /**
     * The enumeration @ref xkb_keysym_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYSYM_FLAGS = 9200,
    /**
     * The enumeration @ref xkb_rmlvo_builder_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_RMLVO_BUILDER_FLAGS = 18200,
    /**
     * The enumeration @ref xkb_keymap_format
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYMAP_FORMAT = 21000,
    /**
     * The enumeration @ref xkb_keymap_compile_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYMAP_COMPILE_FLAGS = 21200,
    /**
     * The enumeration @ref xkb_keymap_serialize_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYMAP_SERIALIZE_FLAGS = 21400,
    /**
     * The enumeration @ref xkb_keymap_key_iterator_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYMAP_KEY_ITERATOR_FLAGS = 21600,
    /**
     * The enumeration @ref xkb_state_component
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_STATE_COMPONENT = 24000,
    /**
     * The enumeration @ref xkb_layout_out_of_range_policy
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_LAYOUT_OUT_OF_RANGE_POLICY = 24020,
    /**
     * The enumeration @ref xkb_a11y_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_A11Y_FLAGS = 24040,
    /**
     * The enumeration @ref xkb_keyboard_control_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEYBOARD_CONTROL_FLAGS = 24060,
    /**
     * The enumeration @ref xkb_state_match
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_STATE_MATCH = 24820,
    /**
     * The enumeration @ref xkb_consumed_mode
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_CONSUMED_MODE = 24840,
    /**
     * The enumeration @ref xkb_event_type
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_EVENT_TYPE = 27000,
    /**
     * The enumeration @ref xkb_key_direction
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_KEY_DIRECTION = 27020,
    /**
     * The enumeration @ref xkb_events_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_EVENTS_FLAGS = 27600,
    /**
     * The enumeration @ref xkb_compose_format
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_FORMAT = 30000,
    /**
     * The enumeration @ref xkb_compose_compile_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_COMPILE_FLAGS = 30200,
    /**
     * The enumeration @ref xkb_compose_status
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_STATUS = 31000,
    /**
     * The enumeration @ref xkb_compose_state_flags
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_STATE_FLAGS = 31200,
    /**
     * The enumeration @ref xkb_compose_feed_result
     *
     * @since 1.14.0
     */
    XKB_FEATURE_ENUM_COMPOSE_FEED_RESULT = 31300,
};

/**
 * Test feature support.
 *
 * This is useful when the library is dynamically linked.
 *
 * @param feature  The feature to check
 * @param value    The value to check. Ignored if @p feature does not use
 *                 a value parameter; refer to the feature’s description.
 *
 * @returns `true` if the feature is supported, false otherwise.
 *
 * @since 1.14.0
 */
XKB_EXPORT bool
xkb_feature_supported(enum xkb_feature feature, uint32_t value);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _XKBCOMMON_FEATURES_H_ */
