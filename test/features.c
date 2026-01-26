/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 *
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include "xkbcommon/xkbcommon-features.h"
#include "test.h"
#include "src/features/enums.h"

static void
test_libxkbcommon_enums(void)
{
    /*
     * All enums, all valid values
     */

#define test_enum(values, enum_name)                 \
    for (size_t v = 0; v < ARRAY_SIZE(values); v++) {\
        const int value = (values)[v];               \
        assert(xkb_has_feature((enum_name), value)); \
    }

    test_enum(xkb_rmlvo_builder_flags_values, XKB_FEATURE_ENUM_RMLVO_BUILDER_FLAGS);
    test_enum(xkb_keysym_flags_values, XKB_FEATURE_ENUM_KEYSYM_FLAGS);
    test_enum(xkb_context_flags_values, XKB_FEATURE_ENUM_CONTEXT_FLAGS);
    test_enum(xkb_log_level_values, XKB_FEATURE_ENUM_LOG_LEVEL);
    test_enum(xkb_keymap_compile_flags_values, XKB_FEATURE_ENUM_KEYMAP_COMPILE_FLAGS);
    test_enum(xkb_keymap_format_values, XKB_FEATURE_ENUM_KEYMAP_FORMAT);
    test_enum(xkb_keymap_serialize_flags_values, XKB_FEATURE_ENUM_KEYMAP_SERIALIZE_FLAGS);
    test_enum(xkb_keymap_key_iterator_flags_values, XKB_FEATURE_ENUM_KEYMAP_KEY_ITERATOR_FLAGS);
    test_enum(xkb_state_accessibility_flags_values, XKB_FEATURE_ENUM_STATE_ACCESSIBILITY_FLAGS);
    test_enum(xkb_event_type_values, XKB_FEATURE_ENUM_EVENT_TYPE);
    test_enum(xkb_state_component_values, XKB_FEATURE_ENUM_STATE_COMPONENT);
    test_enum(xkb_keyboard_controls_values, XKB_FEATURE_ENUM_KEYBOARD_CONTROLS);
    test_enum(xkb_key_direction_values, XKB_FEATURE_ENUM_KEY_DIRECTION);
    test_enum(xkb_state_match_values, XKB_FEATURE_ENUM_STATE_MATCH);
    test_enum(xkb_consumed_mode_values, XKB_FEATURE_ENUM_CONSUMED_MODE);
    test_enum(xkb_compose_compile_flags_values, XKB_FEATURE_ENUM_COMPOSE_COMPILE_FLAGS);
    test_enum(xkb_compose_format_values, XKB_FEATURE_ENUM_COMPOSE_FORMAT);
    test_enum(xkb_compose_state_flags_values, XKB_FEATURE_ENUM_COMPOSE_STATE_FLAGS);
    test_enum(xkb_compose_status_values, XKB_FEATURE_ENUM_COMPOSE_STATUS);
    test_enum(xkb_compose_feed_result_values, XKB_FEATURE_ENUM_COMPOSE_FEED_RESULT);

#undef test_enum

    /*
     * Reference enum
     */

    /* Invalid feature */
    assert(!xkb_has_feature(-1, 0));
    assert(!xkb_has_feature(0xffff, 0));
    /* Invalid enum value */
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_FEATURE, -1));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_FEATURE, 0xffff));

    /*
     * Flag enums
     */

    assert(!xkb_has_feature(XKB_FEATURE_ENUM_RMLVO_BUILDER_FLAGS, -1000));
    assert(xkb_has_feature(XKB_FEATURE_ENUM_RMLVO_BUILDER_FLAGS, 0));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_RMLVO_BUILDER_FLAGS, 0xf000));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_KEYSYM_FLAGS, -1000));
    assert(xkb_has_feature(XKB_FEATURE_ENUM_KEYSYM_FLAGS, 0));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_KEYSYM_FLAGS, 0xf000));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_KEYSYM_FLAGS,
                            0xf000 | XKB_KEYSYM_CASE_INSENSITIVE));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_STATE_COMPONENT, -1000));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_STATE_COMPONENT, 0));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_STATE_COMPONENT, 0xf000));

    /*
     * Non-flag enums
     */

    assert(!xkb_has_feature(XKB_FEATURE_ENUM_KEY_DIRECTION, -1000));
    assert(xkb_has_feature(XKB_FEATURE_ENUM_KEY_DIRECTION, 0));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_KEY_DIRECTION, 0xf000));

    assert(!xkb_has_feature(XKB_FEATURE_ENUM_LOG_LEVEL, -1000));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_LOG_LEVEL, 0));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_LOG_LEVEL, 1000));

    assert(!xkb_has_feature(XKB_FEATURE_ENUM_KEYMAP_FORMAT, -1000));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_KEYMAP_FORMAT, 0));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_KEYMAP_FORMAT, 1000));

    assert(!xkb_has_feature(XKB_FEATURE_ENUM_COMPOSE_FORMAT, -1000));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_COMPOSE_FORMAT, 0));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_COMPOSE_FORMAT, 1000));

    assert(!xkb_has_feature(XKB_FEATURE_ENUM_COMPOSE_STATUS, -1000));
    assert(xkb_has_feature(XKB_FEATURE_ENUM_COMPOSE_STATUS, 0));
    assert(!xkb_has_feature(XKB_FEATURE_ENUM_COMPOSE_STATUS, 1000));
}

int
main(void)
{
    test_init();

    test_libxkbcommon_enums();
}
