/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 *
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include <limits.h>
#include <stdint.h>

#include "xkbcommon/xkbcommon-features.h"
#include "test.h"
#include "utils.h"
#include "src/features/enums.h"
#include "xkbcommon/xkbcommon.h"

static void
test_libxkbcommon_enums(void)
{
    /*
     * All enums, all valid values
     */

    enum enum_property {
        ENUM_NONE = 0,
        ENUM_FLAG = (1u << 0),
    };

    static const struct {
        enum xkb_feature feature;
        enum enum_property properties;
        const char * name;
        const uint32_t * values;
        size_t count;
    } tests[] = {
#define ENUM(feature, values, flag) { (feature), (flag), STRINGIFY(feature), (values), ARRAY_SIZE(values) }
        ENUM(XKB_FEATURE_ENUM_FEATURE, xkb_feature_values, ENUM_NONE),
        ENUM(XKB_FEATURE_ENUM_ERROR_CODE, xkb_error_code_values, ENUM_NONE),
        ENUM(XKB_FEATURE_ENUM_CONTEXT_FLAGS, xkb_context_flags_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_LOG_LEVEL, xkb_log_level_values, ENUM_NONE),
        ENUM(XKB_FEATURE_ENUM_KEYSYM_FLAGS, xkb_keysym_flags_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_RMLVO_BUILDER_FLAGS, xkb_rmlvo_builder_flags_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_KEYMAP_FORMAT, xkb_keymap_format_values, ENUM_NONE),
        ENUM(XKB_FEATURE_ENUM_KEYMAP_COMPILE_FLAGS, xkb_keymap_compile_flags_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_KEYMAP_SERIALIZE_FLAGS, xkb_keymap_serialize_flags_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_KEYMAP_KEY_ITERATOR_FLAGS, xkb_keymap_key_iterator_flags_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_STATE_COMPONENT, xkb_state_component_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_LAYOUT_OUT_OF_RANGE_POLICY, xkb_layout_out_of_range_policy_values, ENUM_NONE),
        ENUM(XKB_FEATURE_ENUM_A11Y_FLAGS, xkb_a11y_flags_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_KEYBOARD_CONTROL_FLAGS, xkb_keyboard_control_flags_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_STATE_MODE, xkb_state_mode_values, ENUM_NONE),
        ENUM(XKB_FEATURE_ENUM_STATE_MATCH, xkb_state_match_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_CONSUMED_MODE, xkb_consumed_mode_values, ENUM_NONE),
        ENUM(XKB_FEATURE_ENUM_MACHINE_BUILDER_FLAGS, xkb_machine_builder_flags_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_EVENT_TYPE, xkb_event_type_values, ENUM_NONE),
        ENUM(XKB_FEATURE_ENUM_KEY_DIRECTION, xkb_key_direction_values, ENUM_NONE),
        ENUM(XKB_FEATURE_ENUM_EVENTS_FLAGS, xkb_events_flags_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_COMPOSE_FORMAT, xkb_compose_format_values, ENUM_NONE),
        ENUM(XKB_FEATURE_ENUM_COMPOSE_COMPILE_FLAGS, xkb_compose_compile_flags_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_COMPOSE_STATUS, xkb_compose_status_values, ENUM_NONE),
        ENUM(XKB_FEATURE_ENUM_COMPOSE_STATE_FLAGS, xkb_compose_state_flags_values, ENUM_FLAG),
        ENUM(XKB_FEATURE_ENUM_COMPOSE_FEED_RESULT, xkb_compose_feed_result_values, ENUM_NONE),
#undef  ENUM
    };

    /* Test all enums values */
    for (size_t t = 0; t < ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: #%zu %s ***\n", __func__, t, tests[t].name);

        /* Ensure to test all the enums */
        static_assert(ARRAY_SIZE(tests) == ARRAY_SIZE(xkb_feature_values),
                      "Enum test count mismatch");
        assert(t == 0 || tests[t].feature > tests[t - 1].feature);

        /* Explicit values */
        bool has_zero = false;
        int32_t min = INT32_MAX;
        int32_t max = INT32_MIN;
        assert(tests[t].count > 0);
        for (size_t v = 0; v < tests[t].count; v++) {
            assert(xkb_feature_supported(tests[t].feature, tests[t].values[v]));
            if (tests[t].values[v] == 0)
                has_zero = true;
            min = MIN(min, (int32_t)tests[t].values[v]);
            max = MAX(max, (int32_t)tests[t].values[v]);
        }

        const enum xkb_feature feature = tests[t].feature;
        if (tests[t].properties & ENUM_FLAG) {
            /* Flag enum */

            /* Explicit zero */
            assert(has_zero ^ !xkb_feature_supported(feature, 0));
            /* No negative values */
            assert(min >= 0);
            assert(!xkb_feature_supported(feature, -1));
            assert(!xkb_feature_supported(feature, INT_MIN));
            assert(!xkb_feature_supported(feature, INT32_MIN));
            /* No high positive values */
            #define ENUM_HIGHEST_VALUE XKB_STATE_MATCH_NON_EXCLUSIVE
            assert(max <= ENUM_HIGHEST_VALUE);
            /* Invalid mask */
            static_assert(ENUM_HIGHEST_VALUE < (INT32_MAX >> 1), "");
            assert(!xkb_feature_supported(feature, (ENUM_HIGHEST_VALUE << 1)));
            #undef ENUM_HIGHEST_VALUE
            if (max > 0) {
                assert(!xkb_feature_supported(feature, (max << 1)));
                assert(!xkb_feature_supported(feature, max | (max << 1)));
            }
        } else {
            /* Non-flag enum */

            /* Explicit zero */
            assert(has_zero ^ !xkb_feature_supported(feature, 0));
            /* No high values */
            assert(min > -2);
            assert(!xkb_feature_supported(feature, -2));
            assert(!xkb_feature_supported(feature, min - 1));
            assert(max < 0xf000);
            assert(!xkb_feature_supported(feature, 0xf000));
            assert(!xkb_feature_supported(feature, max + 1));
        }
    }

    /*
     * Reference enum
     */

    /* Invalid feature */
    assert(!xkb_feature_supported(-1, 0));
    assert(!xkb_feature_supported(0xffff, 0));
    /* Invalid enum value */
    assert(!xkb_feature_supported(XKB_FEATURE_ENUM_FEATURE, -1));
    assert(!xkb_feature_supported(XKB_FEATURE_ENUM_FEATURE, 0xffff));

    /*
     * Specific values
     */

    assert(!xkb_feature_supported(XKB_FEATURE_ENUM_KEYMAP_FORMAT,
                            XKB_KEYMAP_USE_ORIGINAL_FORMAT));
    assert(!xkb_feature_supported(XKB_FEATURE_ENUM_KEYMAP_FORMAT, 0));
    assert(!xkb_feature_supported(XKB_FEATURE_ENUM_KEYMAP_FORMAT, 3));
}

int
main(void)
{
    test_init();

    test_libxkbcommon_enums();
}
