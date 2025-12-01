/*
 * NOTE: This file has been generated automatically by “scripts/export-enums.py”.
 *       Do not edit manually!
 */

/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 *
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include "xkbcommon/xkbcommon-features.h"
#include "features/enums.h"
#include "utils.h"

static bool
is_supported_enum_value_mask(enum xkb_enumerations_values values, int value)
{
    return (value >= 0 && value < INT_WIDTH && (values & (1u << value)));
}

static bool
is_supported_enum_value_array(const int *values, size_t size, int value)
{
    for (size_t v = 0; v < size; v++) {
        if (values[v] == value) {
            return true;
        }
    }
    return false;
}

static inline bool
is_supported_flag_value(enum xkb_enumerations_values values,
                        bool accept_zero, int value)
{
    return (accept_zero || value) && ((int) values & value) == value;
}

bool
xkb_has_feature(enum xkb_feature feature, int value)
{
    switch (feature) {
    case XKB_FEATURE_ENUM_FEATURE:
        return is_supported_enum_value_array(
            xkb_feature_values, ARRAY_SIZE(xkb_feature_values), value
        );
    case XKB_FEATURE_ENUM_RMLVO_BUILDER_FLAGS:
        return is_supported_flag_value(
            XKB_RMLVO_BUILDER_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_KEYSYM_FLAGS:
        return is_supported_flag_value(
            XKB_KEYSYM_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_CONTEXT_FLAGS:
        return is_supported_flag_value(
            XKB_CONTEXT_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_LOG_LEVEL:
        return is_supported_enum_value_array(
            xkb_log_level_values, ARRAY_SIZE(xkb_log_level_values), value
        );
    case XKB_FEATURE_ENUM_KEYMAP_COMPILE_FLAGS:
        return is_supported_flag_value(
            XKB_KEYMAP_COMPILE_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_KEYMAP_FORMAT:
        return is_supported_enum_value_mask(XKB_KEYMAP_FORMAT_VALUES, value);
    case XKB_FEATURE_ENUM_KEYMAP_SERIALIZE_FLAGS:
        return is_supported_flag_value(
            XKB_KEYMAP_SERIALIZE_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_KEYMAP_KEY_ITERATOR_FLAGS:
        return is_supported_flag_value(
            XKB_KEYMAP_KEY_ITERATOR_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_STATE_ACCESSIBILITY_FLAGS:
        return is_supported_flag_value(
            XKB_STATE_ACCESSIBILITY_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_EVENT_TYPE:
        return is_supported_enum_value_mask(XKB_EVENT_TYPE_VALUES, value);
    case XKB_FEATURE_ENUM_STATE_COMPONENT:
        return is_supported_flag_value(
            XKB_STATE_COMPONENT_VALUES, false, value
        );
    case XKB_FEATURE_ENUM_KEYBOARD_CONTROLS:
        return is_supported_flag_value(
            XKB_KEYBOARD_CONTROLS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_KEY_DIRECTION:
        return is_supported_enum_value_mask(XKB_KEY_DIRECTION_VALUES, value);
    case XKB_FEATURE_ENUM_STATE_MATCH:
        return is_supported_flag_value(
            XKB_STATE_MATCH_VALUES, false, value
        );
    case XKB_FEATURE_ENUM_CONSUMED_MODE:
        return is_supported_enum_value_mask(XKB_CONSUMED_MODE_VALUES, value);
    case XKB_FEATURE_ENUM_COMPOSE_COMPILE_FLAGS:
        return is_supported_flag_value(
            XKB_COMPOSE_COMPILE_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_COMPOSE_FORMAT:
        return is_supported_enum_value_mask(XKB_COMPOSE_FORMAT_VALUES, value);
    case XKB_FEATURE_ENUM_COMPOSE_STATE_FLAGS:
        return is_supported_flag_value(
            XKB_COMPOSE_STATE_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_COMPOSE_STATUS:
        return is_supported_enum_value_mask(XKB_COMPOSE_STATUS_VALUES, value);
    case XKB_FEATURE_ENUM_COMPOSE_FEED_RESULT:
        return is_supported_enum_value_mask(XKB_COMPOSE_FEED_RESULT_VALUES, value);
    default:
        /* Invalid feature */
        return false;
    }
}
