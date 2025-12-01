/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 *
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include "xkbcommon/xkbcommon.h"
#include "test.h"
#include "src/enums.h"

static void
test_libxkbcommon_enums(void)
{
    /*
     * All enums, all valid values
     */

#define test_enum(values, enum_name)                              \
    for (size_t v = 0; v < ARRAY_SIZE(values); v++) {             \
        const int value = (values)[v];                            \
        assert(xkb_enumeration_is_valid((enum_name), value) == 0);\
    }

    test_enum(xkb_rmlvo_builder_flags_values, XKB_ENUM_RMLVO_BUILDER_FLAGS);
    test_enum(xkb_enumeration_values, XKB_ENUM_ENUMERATION);
    test_enum(xkb_keysym_flags_values, XKB_ENUM_KEYSYM_FLAGS);
    test_enum(xkb_context_flags_values, XKB_ENUM_CONTEXT_FLAGS);
    test_enum(xkb_log_level_values, XKB_ENUM_LOG_LEVEL);
    test_enum(xkb_keymap_compile_flags_values, XKB_ENUM_KEYMAP_COMPILE_FLAGS);
    test_enum(xkb_keymap_format_values, XKB_ENUM_KEYMAP_FORMAT);
    test_enum(xkb_keymap_serialize_flags_values, XKB_ENUM_KEYMAP_SERIALIZE_FLAGS);
    test_enum(xkb_keymap_key_iterator_flags_values, XKB_ENUM_KEYMAP_KEY_ITERATOR_FLAGS);
    test_enum(xkb_state_accessibility_flags_values, XKB_ENUM_STATE_ACCESSIBILITY_FLAGS);
    test_enum(xkb_event_type_values, XKB_ENUM_EVENT_TYPE);
    test_enum(xkb_state_component_values, XKB_ENUM_STATE_COMPONENT);
    test_enum(xkb_keyboard_controls_values, XKB_ENUM_KEYBOARD_CONTROLS);
    test_enum(xkb_key_direction_values, XKB_ENUM_KEY_DIRECTION);
    test_enum(xkb_state_match_values, XKB_ENUM_STATE_MATCH);
    test_enum(xkb_consumed_mode_values, XKB_ENUM_CONSUMED_MODE);
    test_enum(xkb_compose_compile_flags_values, XKB_ENUM_COMPOSE_COMPILE_FLAGS);
    test_enum(xkb_compose_format_values, XKB_ENUM_COMPOSE_FORMAT);
    test_enum(xkb_compose_state_flags_values, XKB_ENUM_COMPOSE_STATE_FLAGS);
    test_enum(xkb_compose_status_values, XKB_ENUM_COMPOSE_STATUS);
    test_enum(xkb_compose_feed_result_values, XKB_ENUM_COMPOSE_FEED_RESULT);

#undef test_enum

    /*
     * Reference enum
     */

    /* Invalid enum */
    assert(xkb_enumeration_is_valid(-1, 0) == -1);
    assert(xkb_enumeration_is_valid(1000, 0) == -1);
    /* Invalid enum value */
    assert(xkb_enumeration_is_valid(XKB_ENUM_ENUMERATION, -1)
           == -2);
    assert(xkb_enumeration_is_valid(XKB_ENUM_ENUMERATION, 1000)
           == -2);

    /*
     * Flag enum: return invalid flags
     */

    assert(xkb_enumeration_is_valid(XKB_ENUM_RMLVO_BUILDER_FLAGS, -1000)
           == -1000);
    assert(xkb_enumeration_is_valid(XKB_ENUM_RMLVO_BUILDER_FLAGS, 0xf000)
           == 0xf000);
    assert(xkb_enumeration_is_valid(XKB_ENUM_KEYSYM_FLAGS, -1000)
           == -1000);
    assert(xkb_enumeration_is_valid(XKB_ENUM_KEYSYM_FLAGS, 0xf000)
           == 0xf000);
    assert(xkb_enumeration_is_valid(XKB_ENUM_KEYSYM_FLAGS, 0xf000 |
                                    XKB_KEYSYM_CASE_INSENSITIVE)
           == 0xf000); // Only invalid flags are returned

    /*
     * Non-flag enum
     */

    assert(xkb_enumeration_is_valid(XKB_ENUM_LOG_LEVEL, -1000)
           == -2);
    assert(xkb_enumeration_is_valid(XKB_ENUM_LOG_LEVEL, 1000)
           == -2);
}

int
main(void)
{
    test_init();

    test_libxkbcommon_enums();
}
