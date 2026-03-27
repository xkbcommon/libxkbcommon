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

#include <stdint.h>

#include "xkbcommon/xkbcommon-features.h"
#include "features/enums.h"
#include "utils.h"

static bool
is_supported_enum_value_mask(enum xkb_enumerations_values values, uint32_t value)
{
    return (value < UINT32_WIDTH && (values & (UINT32_C(1) << value)));
}

static bool
is_supported_enum_value_array(const uint32_t *values, size_t size, uint32_t value)
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
                        bool accept_zero, uint32_t value)
{
    return (accept_zero || value) && ((uint32_t) values & value) == value;
}

bool
xkb_feature_supported(enum xkb_feature feature, uint32_t value)
{
    switch (feature) {
    case XKB_FEATURE_ENUM_FEATURE:
        return is_supported_enum_value_array(
            xkb_feature_values, ARRAY_SIZE(xkb_feature_values), value
        );
    case XKB_FEATURE_ENUM_ERROR_CODE:
        return is_supported_enum_value_array(
            xkb_error_code_values, ARRAY_SIZE(xkb_error_code_values), value
        );
    case XKB_FEATURE_ENUM_CONTEXT_FLAGS:
        return is_supported_flag_value(
            XKB_CONTEXT_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_LOG_LEVEL:
        return is_supported_enum_value_array(
            xkb_log_level_values, ARRAY_SIZE(xkb_log_level_values), value
        );
    case XKB_FEATURE_ENUM_KEYSYM_FLAGS:
        return is_supported_flag_value(
            XKB_KEYSYM_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_RMLVO_BUILDER_FLAGS:
        return is_supported_flag_value(
            XKB_RMLVO_BUILDER_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_KEYMAP_FORMAT:
        return is_supported_enum_value_mask(XKB_KEYMAP_FORMAT_VALUES, value);
    case XKB_FEATURE_ENUM_KEYMAP_COMPILE_FLAGS:
        return is_supported_flag_value(
            XKB_KEYMAP_COMPILE_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_KEYMAP_SERIALIZE_FLAGS:
        return is_supported_flag_value(
            XKB_KEYMAP_SERIALIZE_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_KEYMAP_KEY_ITERATOR_FLAGS:
        return is_supported_flag_value(
            XKB_KEYMAP_KEY_ITERATOR_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_STATE_COMPONENT:
        return is_supported_flag_value(
            XKB_STATE_COMPONENT_VALUES, false, value
        );
    case XKB_FEATURE_ENUM_LAYOUT_OUT_OF_RANGE_POLICY:
        return is_supported_enum_value_mask(XKB_LAYOUT_OUT_OF_RANGE_POLICY_VALUES, value);
    case XKB_FEATURE_ENUM_A11Y_FLAGS:
        return is_supported_flag_value(
            XKB_A11Y_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_KEYBOARD_CONTROL_FLAGS:
        return is_supported_flag_value(
            XKB_KEYBOARD_CONTROL_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_STATE_MATCH:
        return is_supported_flag_value(
            XKB_STATE_MATCH_VALUES, false, value
        );
    case XKB_FEATURE_ENUM_CONSUMED_MODE:
        return is_supported_enum_value_mask(XKB_CONSUMED_MODE_VALUES, value);
    case XKB_FEATURE_ENUM_EVENT_TYPE:
        return is_supported_enum_value_mask(XKB_EVENT_TYPE_VALUES, value);
    case XKB_FEATURE_ENUM_KEY_DIRECTION:
        return is_supported_enum_value_mask(XKB_KEY_DIRECTION_VALUES, value);
    case XKB_FEATURE_ENUM_EVENTS_FLAGS:
        return is_supported_flag_value(
            XKB_EVENTS_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_COMPOSE_FORMAT:
        return is_supported_enum_value_mask(XKB_COMPOSE_FORMAT_VALUES, value);
    case XKB_FEATURE_ENUM_COMPOSE_COMPILE_FLAGS:
        return is_supported_flag_value(
            XKB_COMPOSE_COMPILE_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_COMPOSE_STATUS:
        return is_supported_enum_value_mask(XKB_COMPOSE_STATUS_VALUES, value);
    case XKB_FEATURE_ENUM_COMPOSE_STATE_FLAGS:
        return is_supported_flag_value(
            XKB_COMPOSE_STATE_FLAGS_VALUES, true, value
        );
    case XKB_FEATURE_ENUM_COMPOSE_FEED_RESULT:
        return is_supported_enum_value_mask(XKB_COMPOSE_FEED_RESULT_VALUES, value);
    default:
        /* Invalid feature */
        return false;
    }
}
