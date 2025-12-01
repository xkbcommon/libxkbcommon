/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 *
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include "xkbcommon/xkbcommon.h"
#include "enums.h"
#include "utils.h"

static int
is_supported_enum_value(const int *values, size_t size, int value)
{
    for (size_t v = 0; v < size; v++) {
        if (values[v] == value) {
            /* Valid value */
            return 0;
        }
    }
    /* Invalid value */
    return -2;
}

static int
is_supported_flag_value(const int *values, size_t size, int value)
{
    for (size_t v = 0; value && v < size; v++) {
        if (values[v] & value) {
            /* Remove flag */
            value &= ~values[v];
        }
    }
    /* Remaining flags are invalid */
    return value;
}

int
xkb_enumeration_is_valid(enum xkb_enumeration e, int value)
{
    switch (e) {
    case XKB_ENUM_ENUMERATION:
        return is_supported_enum_value(
            xkb_enumeration_values,
            ARRAY_SIZE(xkb_enumeration_values),
            value
        );
    case XKB_ENUM_RMLVO_BUILDER_FLAGS:
        return is_supported_flag_value(
            xkb_rmlvo_builder_flags_values,
            ARRAY_SIZE(xkb_rmlvo_builder_flags_values),
            value
        );
    case XKB_ENUM_KEYSYM_FLAGS:
        return is_supported_flag_value(
            xkb_keysym_flags_values,
            ARRAY_SIZE(xkb_keysym_flags_values),
            value
        );
    case XKB_ENUM_CONTEXT_FLAGS:
        return is_supported_flag_value(
            xkb_context_flags_values,
            ARRAY_SIZE(xkb_context_flags_values),
            value
        );
    case XKB_ENUM_LOG_LEVEL:
        return is_supported_enum_value(
            xkb_log_level_values,
            ARRAY_SIZE(xkb_log_level_values),
            value
        );
    case XKB_ENUM_KEYMAP_COMPILE_FLAGS:
        return is_supported_flag_value(
            xkb_keymap_compile_flags_values,
            ARRAY_SIZE(xkb_keymap_compile_flags_values),
            value
        );
    case XKB_ENUM_KEYMAP_FORMAT:
        return is_supported_enum_value(
            xkb_keymap_format_values,
            ARRAY_SIZE(xkb_keymap_format_values),
            value
        );
    case XKB_ENUM_KEYMAP_SERIALIZE_FLAGS:
        return is_supported_flag_value(
            xkb_keymap_serialize_flags_values,
            ARRAY_SIZE(xkb_keymap_serialize_flags_values),
            value
        );
    case XKB_ENUM_KEYMAP_KEY_ITERATOR_FLAGS:
        return is_supported_flag_value(
            xkb_keymap_key_iterator_flags_values,
            ARRAY_SIZE(xkb_keymap_key_iterator_flags_values),
            value
        );
    case XKB_ENUM_STATE_ACCESSIBILITY_FLAGS:
        return is_supported_flag_value(
            xkb_state_accessibility_flags_values,
            ARRAY_SIZE(xkb_state_accessibility_flags_values),
            value
        );
    case XKB_ENUM_EVENT_TYPE:
        return is_supported_enum_value(
            xkb_event_type_values,
            ARRAY_SIZE(xkb_event_type_values),
            value
        );
    case XKB_ENUM_STATE_COMPONENT:
        return is_supported_enum_value(
            xkb_state_component_values,
            ARRAY_SIZE(xkb_state_component_values),
            value
        );
    case XKB_ENUM_KEYBOARD_CONTROLS:
        return is_supported_enum_value(
            xkb_keyboard_controls_values,
            ARRAY_SIZE(xkb_keyboard_controls_values),
            value
        );
    case XKB_ENUM_KEY_DIRECTION:
        return is_supported_enum_value(
            xkb_key_direction_values,
            ARRAY_SIZE(xkb_key_direction_values),
            value
        );
    case XKB_ENUM_STATE_MATCH:
        return is_supported_enum_value(
            xkb_state_match_values,
            ARRAY_SIZE(xkb_state_match_values),
            value
        );
    case XKB_ENUM_CONSUMED_MODE:
        return is_supported_enum_value(
            xkb_consumed_mode_values,
            ARRAY_SIZE(xkb_consumed_mode_values),
            value
        );
    case XKB_ENUM_COMPOSE_COMPILE_FLAGS:
        return is_supported_flag_value(
            xkb_compose_compile_flags_values,
            ARRAY_SIZE(xkb_compose_compile_flags_values),
            value
        );
    case XKB_ENUM_COMPOSE_FORMAT:
        return is_supported_enum_value(
            xkb_compose_format_values,
            ARRAY_SIZE(xkb_compose_format_values),
            value
        );
    case XKB_ENUM_COMPOSE_STATE_FLAGS:
        return is_supported_flag_value(
            xkb_compose_state_flags_values,
            ARRAY_SIZE(xkb_compose_state_flags_values),
            value
        );
    case XKB_ENUM_COMPOSE_STATUS:
        return is_supported_enum_value(
            xkb_compose_status_values,
            ARRAY_SIZE(xkb_compose_status_values),
            value
        );
    case XKB_ENUM_COMPOSE_FEED_RESULT:
        return is_supported_enum_value(
            xkb_compose_feed_result_values,
            ARRAY_SIZE(xkb_compose_feed_result_values),
            value
        );
    default:
        /* Invalid enumeration */
        return -1;
    }

    return 0;
}
