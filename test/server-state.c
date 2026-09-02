/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-errors.h"
#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon-names.h"

#include "abi-check.h"
#include "context.h"
#include "evdev-scancodes.h"
#include "src/keysym.h"
#include "src/keymap.h"
#include "src/state-priv.h"
#include "state.h"
#include "test.h"
#include "utils.h"

#define GOLDEN_TESTS_OUTPUTS "keymaps/"

#define U(cp) (XKB_KEYSYM_UNICODE_OFFSET + (cp))

enum key_directions {
    XKB_KEY_PRESS = (1 << 0),
    XKB_KEY_REPEAT = (1 << 1),
    XKB_KEY_RELEASE = (1 << 2),
    XKB_KEY_TAP = XKB_KEY_PRESS | XKB_KEY_RELEASE,
};

static enum xkb_state_component
xkb_state_update_enabled_controls(struct xkb_state *state,
                                  enum xkb_keyboard_control_flags affect,
                                  enum xkb_keyboard_control_flags controls)
{
    const struct xkb_state_components_update components = {
        .size = sizeof(components),
        .components = XKB_STATE_CONTROLS,
        .affect_controls = affect,
        .controls = controls,
    };
    const struct xkb_state_update update = {
        .size = sizeof(update),
        .components = &components,
    };
    enum xkb_state_component changed = 0;
    /* Note: no error handling */
    assert(xkb_state_update_synthetic(state, &update, &changed) == XKB_SUCCESS);
    return changed;
}

/**
 * Update the keyboard server state to change the *boolean*
 * [global keyboard controls].
 *
 * @param machine The keyboard server state object.
 * @param events  The event collection to store the events. It will be reset.
 * @param affect
 *     *Boolean* global keyboard controls to modify. @p controls contains the
 *     actual values.
 * @param controls
 *     *Boolean* global keyboard controls to lock or unlock. Only controls in
 *     @p affect are considered.
 *
 * @returns 0 on success, otherwise an error code.
 *
 * Non-boolean controls are ignored.
 *
 * @since 1.14.0
 *
 * @memberof xkb_machine
 *
 * [global keyboard controls]: @ref xkb_keyboard_control_flags
 */
static enum xkb_error_code
xkb_machine_update_enabled_controls(struct xkb_machine *machine,
                                    struct xkb_events *events,
                                    enum xkb_keyboard_control_flags affect,
                                    enum xkb_keyboard_control_flags controls)
{
    const enum xkb_state_component components
        = ((affect || controls) ? XKB_STATE_CONTROLS : 0);
    const struct xkb_state_components_update components_update = {
        .size = sizeof(components_update),
        .components = components,
        .affect_controls = affect,
        .controls = controls,
    };
    const struct xkb_state_update update = {
        .size = sizeof(update),
        .components = &components_update
    };
    return xkb_machine_process_synthetic(machine, &update, events);
}

/**
 * Update the keyboard server state to change the latched and locked state of
 * the modifiers and layout.
 *
 * Use this function to update the latched and locked state according to
 * out-of-band (non-device) inputs, such as UI layout switchers.
 *
 * @par Layout out of range
 * @parblock
 *
 * If the effective layout, after taking into account the depressed, latched and
 * locked layout, is out of range (negative or greater than the maximum layout),
 * it is brought into range. Currently, the layout is wrapped using integer
 * modulus (with negative values wrapping from the end). The wrapping behavior
 * may be made configurable in the future.
 *
 * @endparblock
 *
 * @param machine The keyboard server state object.
 * @param events  The event collection to store the events. It will be reset.
 * @param affect_latched_mods See @p latched_mods.
 * @param latched_mods
 *     Modifiers to set as latched or unlatched. Only modifiers in
 *     @p affect_latched_mods are considered.
 * @param affect_latched_layout See @p latched_layout.
 * @param latched_layout
 *     Layout to latch. Only considered if @p affect_latched_layout is true.
 *     Maybe be out of range (including negative) -- see note above.
 * @param affect_locked_mods See @p locked_mods.
 * @param locked_mods
 *     Modifiers to set as locked or unlocked. Only modifiers in
 *     @p affect_locked_mods are considered.
 * @param affect_locked_layout See @p locked_layout.
 * @param locked_layout
 *     Layout to lock. Only considered if @p affect_locked_layout is true.
 *     Maybe be out of range (including negative) -- see note above.
 *
 * @returns 0 on success, otherwise an error code.
 *
 * @memberof xkb_machine
 */
static enum xkb_error_code
xkb_machine_update_latched_locked(struct xkb_machine *machine,
                                  struct xkb_events *events,
                                  xkb_mod_mask_t affect_latched_mods,
                                  xkb_mod_mask_t latched_mods,
                                  bool affect_latched_layout,
                                  int32_t latched_layout,
                                  xkb_mod_mask_t affect_locked_mods,
                                  xkb_mod_mask_t locked_mods,
                                  bool affect_locked_layout,
                                  int32_t locked_layout)
{
    const enum xkb_state_component components
        = ((affect_latched_mods || latched_mods) ? XKB_STATE_MODS_LATCHED : 0)
        | ((affect_locked_mods || locked_mods) ? XKB_STATE_MODS_LOCKED : 0)
        | (affect_latched_layout ? XKB_STATE_LAYOUT_LATCHED : 0)
        | (affect_locked_layout ? XKB_STATE_LAYOUT_LOCKED : 0);
    const struct xkb_state_components_update components_update = {
        .size = sizeof(components_update),
        .components = components,
        .affect_latched_mods = affect_latched_mods,
        .latched_mods = latched_mods,
        .latched_layout = latched_layout,
        .affect_locked_mods = affect_locked_mods,
        .locked_mods = locked_mods,
        .locked_layout = locked_layout,
    };
    const struct xkb_state_update update = {
        .size = sizeof(update),
        .components = &components_update,
    };
    return xkb_machine_process_synthetic(machine, &update, events);
}

static void
test_machine_builder(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap =
        xkb_keymap_new_from_names(ctx, NULL, TEST_KEYMAP_COMPILE_FLAGS);
    assert(keymap);

    struct xkb_machine_builder *builder =
        xkb_machine_builder_new(keymap, NULL, NULL);
    assert(builder);
    xkb_machine_builder_unref(builder);
    enum xkb_error_code error;
    builder = xkb_machine_builder_new(keymap, NULL, &error);
    assert(builder && error == XKB_SUCCESS);
    xkb_machine_builder_unref(builder);
    struct xkb_machine_builder_config config = { .size = sizeof(config) };
    config.builder_flags = UINT32_MAX;
    assert(!xkb_machine_builder_new(keymap, &config, NULL));
    assert(!xkb_machine_builder_new(keymap, &config, &error) &&
           error == XKB_ERROR_UNSUPPORTED_MACHINE_BUILDER_FLAGS);
    config.builder_flags = 0;
    config.machine_flags = UINT32_MAX;
    assert(!xkb_machine_builder_new(keymap, &config, NULL));
    assert(!xkb_machine_builder_new(keymap, &config, &error) &&
           error == XKB_ERROR_UNSUPPORTED_MACHINE_FLAGS);
    config.machine_flags = 0;
    builder = xkb_machine_builder_new(keymap, &config, &error);
    assert(builder && error == XKB_SUCCESS);

    struct xkb_machine_builder_a11y_update a11y_update = {
        .size = sizeof(a11y_update)
    };

    /* Valid flags */
    static_assert(XKB_A11Y_NO_FLAGS == 0, "default flags");
    a11y_update.affect = XKB_A11Y_NO_FLAGS;
    a11y_update.flags = XKB_A11Y_NO_FLAGS;
    assert(xkb_machine_builder_update_a11y(builder, &a11y_update) == XKB_SUCCESS);

    /* Invalid flags */
    a11y_update.affect = -1000;
    a11y_update.flags = 0;
    assert(xkb_machine_builder_update_a11y(builder, &a11y_update) ==
           XKB_ERROR_UNSUPPORTED_A11Y_FLAGS);
    a11y_update.affect = 0;
    a11y_update.flags = -1000;
    assert(xkb_machine_builder_update_a11y(builder, &a11y_update) ==
           XKB_ERROR_UNSUPPORTED_A11Y_FLAGS);
    a11y_update.affect = 1000;
    a11y_update.flags = 0;
    assert(xkb_machine_builder_update_a11y(builder, &a11y_update) ==
           XKB_ERROR_UNSUPPORTED_A11Y_FLAGS);
    a11y_update.affect = 0;
    a11y_update.flags = 1000;
    assert(xkb_machine_builder_update_a11y(builder, &a11y_update) ==
           XKB_ERROR_UNSUPPORTED_A11Y_FLAGS);

    struct xkb_machine *sm = xkb_machine_new(builder, NULL);
    assert(sm);

    xkb_machine_unref(sm);
    xkb_machine_builder_unref(builder);
    xkb_keymap_unref(keymap);
}

/* Check that derived state is correctly initialized */
static void
test_initial_derived_values(struct xkb_context *ctx)
{
    struct xkb_keymap * const keymap = test_compile_rules(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
        "pc104", "us", NULL, "grp1_led:scroll"
    );
    assert(keymap);

    struct xkb_machine_builder *builder =
        xkb_machine_builder_new(keymap, NULL, NULL);
    assert(builder);

    struct xkb_machine * const sm = xkb_machine_new(builder, NULL);
    assert(sm);

    struct xkb_state * const state = xkb_machine_get_state(sm);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_SCROLL));

    xkb_machine_unref(sm);
    xkb_machine_builder_unref(builder);
    xkb_keymap_unref(keymap);
}

/* ABI checks */
static void
test_state_update(struct xkb_context *ctx)
{
    struct xkb_keymap * const keymap = test_compile_rules(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
        "empty", "empty", NULL, NULL
    );
    assert(keymap);

    struct xkb_state * const state = xkb_state_new(keymap);
    assert(state);
    struct xkb_machine_builder *builder =
        xkb_machine_builder_new(keymap, NULL, NULL);
    struct xkb_machine * const sm = xkb_machine_new(builder, NULL);
    assert(sm);
    xkb_machine_builder_unref(builder);
    struct xkb_events * const events = xkb_events_new(ctx, NULL, NULL);
    assert(events);

    /* Simulate a new version with some new fields */
    struct xkb_state_update_newer {
        struct xkb_state_update current;
        uint64_t extra;
    };
    assert_no_padding(struct xkb_state_update_newer, current, extra);

    /* Simulate a new version with some new fields */
    struct xkb_state_components_update_newer {
        struct xkb_state_components_update current;
        uint64_t extra;
    };
    assert_no_padding(struct xkb_state_components_update_newer, current, extra);

    /* Simulate a new version with some new fields */
    struct xkb_layout_policy_update_newer {
        struct xkb_layout_policy_update current;
        uint32_t extra;
    };
    assert_no_padding(struct xkb_layout_policy_update_newer, current, extra);

    static const struct {
        struct params {
            size_t size;
            uint32_t extra;
            uint32_t reserved0;
            bool enabled;
        } root;
        struct params components;
        struct params layout_policy;
        enum xkb_error_code error;
    } tests[] = {
        /*
         * Too small
         */

        {
            .root = { 0, 0 },
            .components = { .enabled = false },
            .layout_policy = { .enabled = false },
            .error = XKB_ERROR_ABI_INVALID_STRUCT_SIZE,
        },
        {
            .root = { 1, 0 },
            .components = { .enabled = false },
            .layout_policy = { .enabled = false },
            .error = XKB_ERROR_ABI_INVALID_STRUCT_SIZE,
        },
        {
            .root = { sizeof(struct xkb_state_update), 0 },
            .components = { .size = 0, .enabled = true },
            .layout_policy = { .enabled = false },
            .error = XKB_ERROR_ABI_INVALID_STRUCT_SIZE,
        },
        {
            .root = { sizeof(struct xkb_state_update), 0 },
            .components = { .size = 1, .enabled = true },
            .layout_policy = { .enabled = false },
            .error = XKB_ERROR_ABI_INVALID_STRUCT_SIZE,
        },
        {
            .root = { sizeof(struct xkb_state_update), 0 },
            .components = { .enabled = false },
            .layout_policy = { .size = 0, .enabled = true },
            .error = XKB_ERROR_ABI_INVALID_STRUCT_SIZE,
        },
        {
            .root = { sizeof(struct xkb_state_update), 0 },
            .components = { .enabled = false },
            .layout_policy = { .size = 1, .enabled = true },
            .error = XKB_ERROR_ABI_INVALID_STRUCT_SIZE,
        },

        /*
         * OK: current size
         */

        {
            .root = { sizeof(struct xkb_state_update), 0 },
            .components = { .enabled = false },
            .layout_policy = { .enabled = false },
            .error = XKB_SUCCESS,
        },
        {
            .root = { sizeof(struct xkb_state_update), 0 },
            .components = {
                .size = sizeof(struct xkb_state_components_update),
                .enabled = true
            },
            .layout_policy = { .enabled = false },
            .error = XKB_SUCCESS,
        },
        {
            .root = { sizeof(struct xkb_state_update), 0 },
            .components = { .enabled = false },
            .layout_policy = {
                .size = sizeof(struct xkb_layout_policy_update),
                .enabled = true
            },
            .error = XKB_SUCCESS,
        },

        /*
         * Invalid: reserved0 ≠ 0
         */

        {
            .root = { sizeof(struct xkb_state_update), 0, .reserved0 = 1 },
            .components = { .enabled = false },
            .layout_policy = { .enabled = false },
            .error = XKB_ERROR_ABI_FORWARD_COMPAT,
        },

        /*
         * OK: too big, but no new field is set
         */

        {
            .root = { sizeof(struct xkb_state_update_newer), 0 },
            .components = { .enabled = false },
            .layout_policy = { .enabled = false },
            .error = XKB_SUCCESS,
        },
        {
            .root = { sizeof(struct xkb_state_update), 0 },
            .components = {
                .size = sizeof(struct xkb_state_components_update_newer),
                .enabled = true
            },
            .layout_policy = { .enabled = false },
            .error = XKB_SUCCESS,
        },
        {
            .root = { sizeof(struct xkb_state_update), 0 },
            .components = { .enabled = false },
            .layout_policy = {
                .size = sizeof(struct xkb_layout_policy_update_newer),
                .enabled = true
            },
            .error = XKB_SUCCESS,
        },

        /*
         * Too big
         */

        {
            .root = {
                sizeof(struct xkb_state_update_newer),
                (UINT32_C(1) << 0)
            },
            .components = { .enabled = false },
            .layout_policy = { .enabled = false },
            .error = XKB_ERROR_ABI_FORWARD_COMPAT,
        },
        {
            .root = {
                sizeof(struct xkb_state_update_newer),
                (UINT32_C(1) << 31)
            },
            .components = { .enabled = false },
            .layout_policy = { .enabled = false },
            .error = XKB_ERROR_ABI_FORWARD_COMPAT,
        },
        {
            .root = { sizeof(struct xkb_state_update), 0 },
            .components = {
                .enabled = true,
                .size = sizeof(struct xkb_state_components_update_newer),
                .extra = (UINT32_C(1) << 31),
            },
            .layout_policy = { .enabled = false },
            .error = XKB_ERROR_ABI_FORWARD_COMPAT,
        },
        {
            .root = { sizeof(struct xkb_state_update), 0 },
            .components = { .enabled = false },
            .layout_policy = {
                .enabled = true,
                .size = sizeof(struct xkb_layout_policy_update_newer),
                .extra = (UINT32_C(1) << 31),
            },
            .error = XKB_ERROR_ABI_FORWARD_COMPAT,
        },
    };

    for (size_t s = 0; s < ARRAY_SIZE(tests); s++) {
        fprintf(stderr, "------\n*** %s: #%zu ***\n", __func__, s);
        const struct xkb_state_components_update_newer components = {
            .current = {
                .size = tests[s].components.size,
            },
            .extra = tests[s].components.extra
        };
        const struct xkb_layout_policy_update_newer layout_policy = {
            .current = {
                .size = tests[s].layout_policy.size,
            },
            .extra = tests[s].layout_policy.extra
        };
        const struct xkb_state_update_newer update = {
            .current = {
                .size = tests[s].root.size,
                .reserved0 = tests[s].root.reserved0,
                .components = (tests[s].components.enabled)
                    ? (struct xkb_state_components_update*)&components
                    : NULL,
                .layout_policy = (tests[s].layout_policy.enabled)
                    ? (struct xkb_layout_policy_update*)&layout_policy
                    : NULL,
            },
            .extra = tests[s].root.extra
        };
        assert_eq(
            "xkb_state_update_synthetic",
            tests[s].error,
            xkb_state_update_synthetic(state, (struct xkb_state_update *)&update,
                                       NULL),
            "%d"
        );
        assert_eq(
            "xkb_machine_process_synthetic",
            tests[s].error,
            xkb_machine_process_synthetic(sm, (struct xkb_state_update *)&update,
                                          events),
            "%d"
        );
    }

    xkb_events_unref(events);
    xkb_machine_unref(sm);
    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
}

static enum xkb_state_component
update_key(struct xkb_machine *sm,
           struct xkb_events *events,
           struct xkb_state *state,
           bool use_machine, xkb_keycode_t key,
           enum xkb_key_direction direction)
{
    if (!use_machine)
        return xkb_state_update_key(state, key, direction);

    assert(xkb_machine_process_key(sm, key, direction, events) == 0);
    const struct xkb_event *event;
    enum xkb_state_component all_changes = 0;
    while ((event = xkb_events_next(events))) {
        const enum xkb_state_component changed =
            xkb_state_update_event(state, event);
        all_changes |= changed;

        switch (xkb_event_get_type(event)) {
        case XKB_EVENT_TYPE_STATE_COMPONENTS: {
            struct xkb_event_components components = {
                .size = sizeof(components)
            };
            const enum xkb_error_code error =
                xkb_event_serialize_components(event, &components);
            assert(error == XKB_SUCCESS);
            assert_eq("changed", changed, components.changed, "%d");
            assert_eq("depressed mods",
                      xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED),
                      components.depressed_mods, "0x%"PRIx32);
            assert_eq("latched mods",
                      xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED),
                      components.latched_mods, "0x%"PRIx32);
            assert_eq("locked mods",
                      xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED),
                      components.locked_mods, "0x%"PRIx32);
            assert_eq("effective mods",
                      xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE),
                      components.mods, "0x%"PRIx32);
            assert_eq("depressed layout",
                      xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_DEPRESSED),
                      components.depressed_layout, "%"PRIu32);
            assert_eq("latched layout",
                      xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED),
                      components.latched_layout, "%"PRIu32);
            assert_eq("locked layout",
                      xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED),
                      components.locked_layout, "%"PRIu32);
            assert_eq("effective layout",
                      xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE),
                      components.layout, "%"PRIu32);
            assert_eq("LEDs",
                      xkb_state_serialize_leds(state, XKB_STATE_LEDS),
                      components.leds, "0x%"PRIx32);
            assert_eq("controls",
                      xkb_state_serialize_enabled_controls(state, XKB_STATE_CONTROLS),
                      components.controls, "0x%x");
            break;
        }
        default:
            ; /* Label followed by declaration requires C23 */
            static_assert(XKB_EVENT_TYPE_TERMINATE_DISPLAY_SERVER == 5 &&
                          XKB_EVENT_TYPE_TERMINATE_DISPLAY_SERVER ==
                          (enum xkb_event_type) _LAST_XKB_EVENT_TYPE,
                          "Missing state event type");
            /* ignore */
        }
    }
    return all_changes;
}

static enum xkb_state_component
update_controls(struct xkb_machine *sm,
                struct xkb_events *events,
                struct xkb_state *state,
                bool use_machine,
                enum xkb_keyboard_control_flags affect,
                enum xkb_keyboard_control_flags controls)
{
    if (use_machine) {
        assert(xkb_machine_update_enabled_controls(sm, events,
                                                   affect, controls)
               == XKB_SUCCESS);
        const struct xkb_event *event;
        enum xkb_state_component changed = 0;
        while ((event = xkb_events_next(events))) {
            changed |= xkb_state_update_event(state, event);
        }
        return changed;
    } else {
        return xkb_state_update_enabled_controls(state, affect, controls);
    }
}

static void
test_group_wrap(struct xkb_context *ctx)
{
    static const char keymap_str[] =
        "default xkb_keymap {\n"
        "    xkb_keycodes { <> = 1; };\n"
        "    xkb_types { type \"ONE_LEVEL\" { map[none] = 1; }; };\n"
        "    xkb_symbols {\n"
        "        key <> { [a], [b], [c], [d] };\n"
        "    };\n"
        "};";
    struct xkb_keymap * const keymap = test_compile_buffer(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1, keymap_str, sizeof(keymap_str)
    );
    assert(keymap);
    const xkb_layout_index_t num_layouts = xkb_keymap_num_layouts(keymap);
    assert(num_layouts == 4);

    struct xkb_machine_builder *builder =
        xkb_machine_builder_new(keymap, NULL, NULL);
    assert(builder);

    struct xkb_machine * const sm = xkb_machine_new(builder, NULL);
    assert(sm);
    xkb_machine_builder_unref(builder);

    struct xkb_state * const state = xkb_state_new(keymap);
    assert(state);

    struct xkb_events * const events = xkb_events_new(ctx, NULL, NULL);
    assert(events);

    const struct xkb_event *event;

    struct {
        enum xkb_layout_out_of_range_policy policy;
        xkb_layout_index_t redirect_group;
        xkb_layout_index_t locked_group;
        xkb_layout_index_t expected_group;
    } tests[] = {
        /*
         * Explicit: wrap
         */
        {
            .policy = XKB_LAYOUT_OUT_OF_RANGE_WRAP,
            .redirect_group = 0,
            .locked_group = (xkb_layout_index_t)-1,
            .expected_group = num_layouts - 1,
        },
        {
            .policy = XKB_LAYOUT_OUT_OF_RANGE_WRAP,
            .redirect_group = 0,
            .locked_group = num_layouts + 1,
            .expected_group = 1,
        },
        /*
         * Explicit: clamp
         */
        {
            .policy = XKB_LAYOUT_OUT_OF_RANGE_CLAMP,
            .redirect_group = 0,
            .locked_group = (xkb_layout_index_t)-1,
            .expected_group = 0,
        },
        {
            .policy = XKB_LAYOUT_OUT_OF_RANGE_CLAMP,
            .redirect_group = 0,
            .locked_group = num_layouts + 1,
            .expected_group = num_layouts - 1,
        },
        /*
         * Explicit: redirect
         */
        {
            .policy = XKB_LAYOUT_OUT_OF_RANGE_REDIRECT,
            .redirect_group = 2,
            .locked_group = (xkb_layout_index_t)-1,
            .expected_group = 2,
        },
        {
            .policy = XKB_LAYOUT_OUT_OF_RANGE_REDIRECT,
            .redirect_group = 2,
            .locked_group = num_layouts + 1,
            .expected_group = 2,
        },
    };

    for (uint8_t t = 0; t < (uint8_t) ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, t);
        struct xkb_layout_policy_update layout_policy = {
            .size = sizeof(layout_policy),
            .policy = tests[t].policy,
            .redirect = tests[t].redirect_group,
        };
        const struct xkb_state_components_update components = {
            .size = sizeof(components),
            .components = XKB_STATE_LAYOUT_LOCKED,
            .locked_layout = (int32_t)tests[t].locked_group
        };
        const struct xkb_state_update req = {
            .size = sizeof(req),
            .layout_policy = &layout_policy,
            .components = &components,
        };
        assert(xkb_machine_process_synthetic(sm, &req, events) == XKB_SUCCESS);
        while ((event = xkb_events_next(events)))
            xkb_state_update_event(state, event);
        assert_eq("unexpected effective group", tests[t].expected_group,
                  xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE),
                  "%"PRIu32);
    }

    xkb_events_unref(events);
    xkb_state_unref(state);
    xkb_machine_unref(sm);
    xkb_keymap_unref(keymap);
}

static void
test_sticky_keys(struct xkb_context *ctx)
{
    struct xkb_keymap * const keymap = test_compile_rmlvo(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
        "evdev", "pc104", "ca,cz,de", NULL, "controls,grp:lwin_switch,grp:menu_toggle"
    );
    assert(keymap);

    struct xkb_machine_builder *builder =
        xkb_machine_builder_new(keymap, NULL, NULL);
    assert(builder);
    struct xkb_machine *sm = xkb_machine_new(builder, NULL);
    assert(sm);
    xkb_machine_builder_unref(builder);
    struct xkb_events *events = xkb_events_new(ctx, NULL, NULL);
    assert(events);
    struct xkb_state *state = xkb_state_new(keymap);
    assert(state);

    const xkb_mod_mask_t shift = xkb_keymap_mod_get_mask2(keymap,
                                                          XKB_MOD_INDEX_SHIFT);
    const xkb_mod_mask_t caps = xkb_keymap_mod_get_mask2(keymap,
                                                          XKB_MOD_INDEX_CAPS);
    const xkb_mod_mask_t ctrl = xkb_keymap_mod_get_mask2(keymap,
                                                         XKB_MOD_INDEX_CTRL);

    xkb_mod_mask_t mods = 0;
    enum xkb_keyboard_control_flags controls;
    enum xkb_state_component changed;

    controls = xkb_state_serialize_enabled_controls(state, XKB_STATE_CONTROLS);
    assert(controls == 0);

    enum sticky_key_activation {
        STICKY_KEY_ACTION_SETCONTROLS = 0,
        STICKY_KEY_ACTION_LOCKCONTROLS,
        STICKY_KEY_EVENTS_API,
        STICKY_KEY_LEGACY_API,
    };

    enum sticky_key_activation tests[] = {
        STICKY_KEY_ACTION_SETCONTROLS,
        STICKY_KEY_ACTION_LOCKCONTROLS,
        STICKY_KEY_EVENTS_API,
        STICKY_KEY_LEGACY_API,
    };

    for (uint8_t t = 0; t < (uint8_t) ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, t);

        const bool use_events = tests[t] == STICKY_KEY_EVENTS_API;

        /* Enable controls */
        switch (tests[t]) {
        case STICKY_KEY_ACTION_SETCONTROLS:
            /* SetControls() */
            changed = update_key(sm, events, state, use_events,
                                 KEY_F1 + EVDEV_OFFSET, XKB_KEY_DOWN);
            assert(changed == XKB_STATE_CONTROLS);
            break;
        case STICKY_KEY_ACTION_LOCKCONTROLS:
            /* LockControls() */
            changed = update_key(sm, events, state, use_events,
                                 KEY_F2 + EVDEV_OFFSET, XKB_KEY_DOWN);
            assert(changed == XKB_STATE_CONTROLS);
            controls = xkb_state_serialize_enabled_controls(state, XKB_STATE_CONTROLS);
            assert(controls == XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);
            changed = update_key(sm, events, state, use_events,
                                 KEY_F2 + EVDEV_OFFSET, XKB_KEY_UP);
            assert(changed == 0);
            break;
        case STICKY_KEY_EVENTS_API: {
            changed = update_controls(sm, events, state, true,
                                      XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS,
                                      XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);
            assert(changed == XKB_STATE_CONTROLS);
            controls = xkb_state_serialize_enabled_controls(state, XKB_STATE_CONTROLS);
            assert(controls == XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);
            break;
        }
        case STICKY_KEY_LEGACY_API:
            changed = xkb_state_update_enabled_controls(
                state,
                XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS,
                XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS
            );
            assert(changed == XKB_STATE_CONTROLS);
            controls = xkb_state_serialize_enabled_controls(state, XKB_STATE_CONTROLS);
            assert(controls == XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);
            break;
        }
        controls = xkb_state_serialize_enabled_controls(state, XKB_STATE_CONTROLS);
        assert(controls == XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);

        /* Latch shift (sticky) */
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == shift);

        /* No latch-to-lock */
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == 0);

        /* Latch shift (sticky) and control */
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == shift);
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTCTRL + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == (shift | ctrl));
        changed = update_key(sm, events, state, use_events,
                             KEY_Q + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_Q + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == 0);

        /* Latch (sticky) & lock groups */
        changed = update_key(sm, events, state, use_events,
                             KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE |
                           XKB_STATE_LEDS));
        changed = update_key(sm, events, state, use_events,
                             KEY_COMPOSE + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == 0);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED) == 1);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 1);
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTMETA + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTMETA + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_LATCHED));
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED) == 1);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED) == 1);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 2);

        /* Latch shift (sticky) and lock Caps */
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == shift);
        changed = update_key(sm, events, state, use_events,
                             KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED |
                           XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS));
        changed = update_key(sm, events, state, use_events,
                             KEY_CAPSLOCK + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == (shift | caps));

        /* Disable controls */
        switch (tests[t]) {
        case STICKY_KEY_ACTION_SETCONTROLS:
            /* SetControls() */
            changed = update_key(sm, events, state, use_events,
                                 KEY_F1 + EVDEV_OFFSET, XKB_KEY_UP);
            assert(changed == (XKB_STATE_CONTROLS |
                               XKB_STATE_LAYOUT_LATCHED |
                               XKB_STATE_LAYOUT_LOCKED |
                               XKB_STATE_LAYOUT_EFFECTIVE |
                               XKB_STATE_MODS_LATCHED |
                               XKB_STATE_MODS_LOCKED |
                               XKB_STATE_MODS_EFFECTIVE |
                               XKB_STATE_LEDS));
            break;
        case STICKY_KEY_ACTION_LOCKCONTROLS:
            /* LockControls() */
            changed = update_key(sm, events, state, use_events,
                                 KEY_F2 + EVDEV_OFFSET, XKB_KEY_DOWN);
            assert(changed == (XKB_STATE_MODS_LATCHED |
                               XKB_STATE_MODS_EFFECTIVE |
                               XKB_STATE_LAYOUT_LATCHED |
                               XKB_STATE_LAYOUT_EFFECTIVE));
            mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
            assert(mods == caps);
            changed = update_key(sm, events, state, use_events,
                                 KEY_F2 + EVDEV_OFFSET, XKB_KEY_UP);
            assert(changed == (XKB_STATE_CONTROLS |
                               XKB_STATE_LAYOUT_LOCKED |
                               XKB_STATE_LAYOUT_EFFECTIVE |
                               XKB_STATE_MODS_LOCKED |
                               XKB_STATE_MODS_EFFECTIVE |
                               XKB_STATE_LEDS));
            break;
        case STICKY_KEY_EVENTS_API: {
            changed = update_controls(sm, events, state, true,
                                      XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS, 0);
            assert(changed == (XKB_STATE_CONTROLS |
                               XKB_STATE_LAYOUT_LATCHED |
                               XKB_STATE_LAYOUT_LOCKED |
                               XKB_STATE_LAYOUT_EFFECTIVE |
                               XKB_STATE_MODS_LATCHED |
                               XKB_STATE_MODS_LOCKED |
                               XKB_STATE_MODS_EFFECTIVE |
                               XKB_STATE_LEDS));
            break;
        }
        case STICKY_KEY_LEGACY_API:
            changed = xkb_state_update_enabled_controls(
                state, XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS, 0
            );
            assert(changed == (XKB_STATE_CONTROLS |
                               XKB_STATE_LAYOUT_LATCHED |
                               XKB_STATE_LAYOUT_LOCKED |
                               XKB_STATE_LAYOUT_EFFECTIVE |
                               XKB_STATE_MODS_LATCHED |
                               XKB_STATE_MODS_LOCKED |
                               XKB_STATE_MODS_EFFECTIVE |
                               XKB_STATE_LEDS));
            break;
        }
        controls = xkb_state_serialize_enabled_controls(state, XKB_STATE_CONTROLS);
        assert(controls == 0);
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == 0);
        assert(xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE) == 0);

        /* Mods are not latched anymore */
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        changed = update_key(sm, events, state, use_events,
                             KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
        assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
        mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert(mods == 0);

        controls = xkb_state_serialize_enabled_controls(state, XKB_STATE_CONTROLS);
        assert(controls == 0);
    }

    xkb_state_unref(state);
    xkb_events_unref(events);
    xkb_machine_unref(sm);

    /*
     * Test latch-to-lock
     */

    struct xkb_machine_builder * const sm_builder =
        xkb_machine_builder_new(keymap, NULL, NULL);
    assert(sm_builder);
    const struct xkb_machine_builder_a11y_update a11y_update = {
        .size = sizeof(a11y_update),
        .affect = XKB_A11Y_STICKY_KEYS_LATCH_TO_LOCK,
        .flags = XKB_A11Y_STICKY_KEYS_LATCH_TO_LOCK
    };
    assert(xkb_machine_builder_update_a11y(sm_builder, &a11y_update) ==
           XKB_SUCCESS);
    sm = xkb_machine_new(sm_builder, NULL);
    assert(sm);
    xkb_machine_builder_unref(sm_builder);
    events = xkb_events_new(ctx, NULL, NULL);
    assert(events);
    state = xkb_state_new(keymap);
    assert(state);
    update_controls(sm, events, state, true,
                    XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS,
                    XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);

    /* Latch shift */
    changed = update_key(sm, events, state, true,
                         KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE));
    changed = update_key(sm, events, state, true,
                         KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(changed == (XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED));
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
    assert(mods == shift);
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    assert(mods == shift);
    /* Lock shift */
    changed = update_key(sm, events, state, true,
                         KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(changed == ( XKB_STATE_MODS_DEPRESSED |
                        XKB_STATE_MODS_LATCHED |
                        XKB_STATE_MODS_LOCKED |
                        XKB_STATE_LEDS /* shift-lock */));
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    assert(mods == shift);
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
    assert(mods == 0);
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED);
    assert(mods == shift);
    changed = update_key(sm, events, state, true,
                         KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(changed == XKB_STATE_MODS_DEPRESSED);
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    assert(mods == shift);
    /* Unlock shift */
    changed = update_key(sm, events, state, true,
                         KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_DOWN);
    assert(changed == XKB_STATE_MODS_DEPRESSED);
    changed = update_key(sm, events, state, true,
                         KEY_LEFTSHIFT + EVDEV_OFFSET, XKB_KEY_UP);
    assert(changed == ( XKB_STATE_MODS_DEPRESSED |
                        XKB_STATE_MODS_LOCKED |
                        XKB_STATE_MODS_EFFECTIVE |
                        XKB_STATE_LEDS /* shift-lock */));
    mods = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    assert(mods == 0);

    xkb_state_unref(state);
    xkb_events_unref(events);
    xkb_machine_unref(sm);

    xkb_keymap_unref(keymap);
}

static void
test_redirect_key(struct xkb_context *ctx)
{
    struct xkb_keymap * const keymap = test_compile_file(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "keymaps/redirect-key-1.xkb"
    );
    assert(keymap);

    struct xkb_machine_builder *builder =
        xkb_machine_builder_new(keymap, NULL, NULL);
    assert(builder);

    struct xkb_machine *sm = xkb_machine_new(builder, NULL);
    assert(sm);
    xkb_machine_builder_unref(builder);

    static const xkb_mod_mask_t shift = UINT32_C(1) << XKB_MOD_INDEX_SHIFT;
    static const xkb_mod_mask_t ctrl = UINT32_C(1) << XKB_MOD_INDEX_CTRL;

    struct xkb_events *events = xkb_events_new(ctx, NULL, NULL);
    assert(events);

    assert(test_key_seq2(
        keymap, sm, events,
        KEY_A, BOTH,   XKB_KEY_a, NEXT,
        KEY_A, DOWN,   XKB_KEY_a, NEXT,
        KEY_A, REPEAT, XKB_KEY_a, NEXT,
        KEY_A, UP,     XKB_KEY_a, NEXT,
        KEY_S, BOTH,   XKB_KEY_a, NEXT,
        KEY_S, DOWN,   XKB_KEY_a, NEXT,
        KEY_S, REPEAT, XKB_KEY_a, NEXT,
        KEY_S, UP,     XKB_KEY_a, NEXT,
        KEY_D, BOTH,   XKB_KEY_s, NEXT,
        KEY_D, DOWN,   XKB_KEY_s, NEXT,
        KEY_D, REPEAT, XKB_KEY_s, NEXT,
        KEY_D, UP,     XKB_KEY_s, FINISH
    ));

    xkb_machine_update_latched_locked(sm, events, 0, 0, false, 0,
                                      ctrl, ctrl, false, 0);

    const struct {
        xkb_keycode_t keycode;
        bool repeats;
        struct test_events {
            struct xkb_event events[3];
            unsigned int events_count;
        } down;
        struct test_events up;
    } tests[] = {
        {
            .keycode = KEY_A + EVDEV_OFFSET,
            .repeats = false,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_A + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_A + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    }
                },
                .events_count = 1
            }
        },
        {
            .keycode = KEY_S + EVDEV_OFFSET,
            .repeats = true,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_A + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_A + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    }
                },
                .events_count = 1
            }
        },
        {
            .keycode = KEY_D + EVDEV_OFFSET,
            .repeats = true,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .components = {
                                .base_mods = shift,
                                .latched_mods = shift,
                                .locked_mods = shift,
                                .mods = shift,
                            },
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_LOCKED
                                     | XKB_STATE_MODS_EFFECTIVE
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_S + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .components = {
                                .base_mods = 0,
                                .latched_mods = 0,
                                .locked_mods = ctrl,
                                .mods = ctrl,
                            },
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_LOCKED
                                     | XKB_STATE_MODS_EFFECTIVE
                        }
                    },
                },
                .events_count = 3
            },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .components = {
                                .base_mods = shift,
                                .latched_mods = shift,
                                .locked_mods = shift,
                                .mods = shift,
                            },
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_LOCKED
                                     | XKB_STATE_MODS_EFFECTIVE
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_S + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .components = {
                                .base_mods = 0,
                                .latched_mods = 0,
                                .locked_mods = ctrl,
                                .mods = ctrl,
                            },
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_LOCKED
                                     | XKB_STATE_MODS_EFFECTIVE
                        }
                    },
                },
                .events_count = 3
            }
        },
    };

    for (uint8_t t = 0; t < (uint8_t) ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: #%u, keycode: %"PRIu32" ***\n",
                __func__, t, tests[t].keycode);
        assert(xkb_keymap_key_repeats(keymap, tests[t].keycode) ==
               tests[t].repeats);
        fprintf(stderr, "+++ Key down +++\n");
        assert(xkb_machine_process_key(sm, tests[t].keycode,
                                       XKB_KEY_DOWN, events) == XKB_SUCCESS);
        assert(check_events(events, tests[t].down.events,
                            tests[t].down.events_count));
        fprintf(stderr, "+++ Key repeat +++\n");
        assert(xkb_machine_process_key(sm, tests[t].keycode,
                                       XKB_KEY_REPEATED, events) == XKB_SUCCESS);
        if (tests[t].repeats) {
            struct xkb_event ref[ARRAY_SIZE(tests->down.events)] = {0};
            memcpy(ref, tests[t].down.events, sizeof(tests->down.events));
            ref[tests[t].down.events_count == 3].key.direction =
                XKB_KEY_REPEATED;
            assert(check_events(events, ref, tests[t].down.events_count));
        } else {
            assert(check_events(events, NULL, 0));
        }
        fprintf(stderr, "+++ Key up +++\n");
        assert(xkb_machine_process_key(sm, tests[t].keycode,
                                       XKB_KEY_UP, events) == XKB_SUCCESS);
        assert(check_events(events, tests[t].up.events,
                            tests[t].up.events_count));
    }

    xkb_events_unref(events);
    xkb_machine_unref(sm);
    xkb_keymap_unref(keymap);
}

static void
test_mouse_keys(struct xkb_context *ctx)
{
    struct xkb_keymap * const keymap =
        test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                           "evdev", "pc104", "us", "",
                           "keypad:pointer-extra,lv3:ralt_latch");
    assert(keymap);

    struct xkb_machine_builder * const builder =
        xkb_machine_builder_new(keymap, NULL, NULL);
    assert(builder);

    struct xkb_machine * const sm = xkb_machine_new(builder, NULL);
    assert(sm);
    xkb_machine_builder_unref(builder);

    struct xkb_events *events = xkb_events_new(ctx, NULL, NULL);
    assert(events);

    const xkb_mod_mask_t level3 =
        _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_LEVEL3);

    const xkb_led_index_t mouse_keys_idx =
        _xkb_keymap_led_get_index(keymap, "Mouse Keys");
    const xkb_led_mask_t mouse_keys = (UINT32_C(1) << mouse_keys_idx);

    const struct {
        xkb_keycode_t keycode;
        enum key_directions directions;
        bool repeats;
        struct test_events {
            struct xkb_event events[2];
            unsigned int events_count;
        } down;
        struct test_events repeat;
        struct test_events up;
    } tests[] = {
        /*
         * Mouse keys: off
         */

        /* Latch LevelThree */
        {
            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .base_mods = level3,
                                .latched_mods = 0,
                                .mods = level3,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_REPEATED
                        }
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED,
                            .components = {
                                .base_mods = 0,
                                .latched_mods = level3,
                                .mods = level3,
                            }
                        }
                    }
                },
                .events_count = 2
            }
        },

        /* Deactivated: move pointer (breaks latch) */
        {
            .keycode = KEY_KP7 + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KP7 + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .latched_mods = 0,
                                .mods = 0,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KP7 + EVDEV_OFFSET,
                            .direction = XKB_KEY_REPEATED
                        }
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KP7 + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    }
                },
                .events_count = 1
            }
        },

        /* Latch LevelThree */
        {
            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .base_mods = level3,
                                .latched_mods = 0,
                                .mods = level3,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED,
                            .components = {
                                .base_mods = 0,
                                .latched_mods = level3,
                                .mods = level3,
                            }
                        }
                    }
                },
                .events_count = 2
            }
        },

        /* Deactivated: button (breaks latch) */
        {
            .keycode = KEY_KP5 + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KP5 + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .latched_mods = 0,
                                .mods = 0,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KP5 + EVDEV_OFFSET,
                            .direction = XKB_KEY_REPEATED
                        }
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KP5 + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    }
                },
                .events_count = 1
            }
        },

        /* Latch LevelThree */
        {
            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .base_mods = level3,
                                .latched_mods = 0,
                                .mods = level3,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED,
                            .components = {
                                .base_mods = 0,
                                .latched_mods = level3,
                                .mods = level3,
                            }
                        }
                    }
                },
                .events_count = 2
            }
        },

        /* Deactivated: lock button (breaks latch) */
        {
            .keycode = KEY_KP0 + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KP0 + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .latched_mods = 0,
                                .mods = 0,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KP0 + EVDEV_OFFSET,
                            .direction = XKB_KEY_REPEATED
                        }
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KP0 + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    }
                },
                .events_count = 1
            }
        },

        /* Latch LevelThree */
        {
            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .base_mods = level3,
                                .latched_mods = 0,
                                .mods = level3,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED,
                            .components = {
                                .base_mods = 0,
                                .latched_mods = level3,
                                .mods = level3,
                            }
                        }
                    }
                },
                .events_count = 2
            }
        },

        /* Deactivated: set default button (breaks latch) */
        {
            .keycode = KEY_KPASTERISK + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KPASTERISK + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .latched_mods = 0,
                                .mods = 0,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KPASTERISK + EVDEV_OFFSET,
                            .direction = XKB_KEY_REPEATED
                        }
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KPASTERISK + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    }
                },
                .events_count = 1
            }
        },

        /*
         * Enable mouse keys
         */
        {
            .keycode = KEY_KPPLUSMINUS + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KPPLUSMINUS + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_CONTROLS
                                     | XKB_STATE_LEDS,
                            .components = {
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            },
                        }
                    }
                },
                .events_count = 2
            },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KPPLUSMINUS + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                },
                .events_count = 1
            },
        },

        /* Latch LevelThree */
        {
            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .base_mods = level3,
                                .mods = level3,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_REPEATED
                        }
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED,
                            .components = {
                                .base_mods = 0,
                                .latched_mods = level3,
                                .mods = level3,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            }
        },

        /*
         * Mouse keys: on
         */

        /* Motion */
        {
            .keycode = KEY_KP7 + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_MOTION,
                        .pointer_motion = {
                            .size = sizeof(tests->down.events->pointer_motion),
                            .flags = XKB_POINTER_MOTION_REPEATS,
                            .x = -1,
                            .y = -1,
                        }
                    }
                },
                .events_count = 1
            },
            .repeat = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_MOTION,
                        .pointer_motion = {
                            .size = sizeof(tests->down.events->pointer_motion),
                            .flags = XKB_POINTER_MOTION_REPEATS,
                            .x = -1,
                            .y = -1,
                        }
                    }
                },
                .events_count = 1
            },
            .up = { .events_count = 0 }
        },

        /* Button: separate press/release */
        {
            .keycode = KEY_KP5 + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 1,
                            .direction = XKB_POINTER_BUTTON_DOWN,
                            .count = 1,
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .latched_mods = 0,
                                .mods = 0,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 1,
                            .direction = XKB_POINTER_BUTTON_UP,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            }
        },

        /* Latch LevelThree */
        {
            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .base_mods = level3,
                                .mods = level3,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED,
                            .components = {
                                .base_mods = 0,
                                .latched_mods = level3,
                                .mods = level3,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            }
        },

        /* Button: double click */
        {
            .keycode = KEY_KPPLUS + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 1,
                            .direction = XKB_POINTER_BUTTON_CLICK,
                            .count = 2,
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .latched_mods = 0,
                                .mods = 0,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },

        /* Latch LevelThree */
        {
            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .base_mods = level3,
                                .mods = level3,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED,
                            .components = {
                                .base_mods = 0,
                                .latched_mods = level3,
                                .mods = level3,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            }
        },

        /* Button: lock 1 */
        {
            .keycode = KEY_KP0 + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 1,
                            .direction = XKB_POINTER_BUTTON_DOWN,
                            .count = 1,
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .latched_mods = 0,
                                .mods = 0,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },

        /* Latch LevelThree */
        {
            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .base_mods = level3,
                                .mods = level3,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED,
                            .components = {
                                .base_mods = 0,
                                .latched_mods = level3,
                                .mods = level3,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            }
        },

        /* Button: unlock 1 */
        {
            .keycode = KEY_KPDOT + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .latched_mods = 0,
                                .mods = 0,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 1
            },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 1,
                            .direction = XKB_POINTER_BUTTON_UP,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            },
        },

        /* Button drag */
        {
            .keycode = KEY_KPEQUAL + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 1,
                            .direction = XKB_POINTER_BUTTON_DOWN,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },
        {
            .keycode = KEY_KPEQUAL + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = { .events_count = 0 },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 1,
                            .direction = XKB_POINTER_BUTTON_UP,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            },
        },

        /* Latch LevelThree */
        {
            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .base_mods = level3,
                                .mods = level3,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_LATCHED,
                            .components = {
                                .base_mods = 0,
                                .latched_mods = level3,
                                .mods = level3,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            }
        },

        /* Button: set default to 2 */
        {
            .keycode = KEY_KPASTERISK + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = { .events_count = 0 },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },
        {
            .keycode = KEY_KPPLUS + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 2,
                            .direction = XKB_POINTER_BUTTON_CLICK,
                            .count = 2,
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_LATCHED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .latched_mods = 0,
                                .mods = 0,
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },
        {
            .keycode = KEY_KP0 + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 2,
                            .direction = XKB_POINTER_BUTTON_DOWN,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },

        /* Button: set default to 3 */
        {
            .keycode = KEY_KPMINUS + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = { .events_count = 0 },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },
        {
            .keycode = KEY_KPPLUS + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 3,
                            .direction = XKB_POINTER_BUTTON_CLICK,
                            .count = 2,
                        }
                    }
                },
                .events_count = 1
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },
        {
            .keycode = KEY_KPDOT + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = { .events_count = 0 },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 }, /* Cannot unlock button 3 */
        },
        {
            .keycode = KEY_KP0 + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 3,
                            .direction = XKB_POINTER_BUTTON_DOWN,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },

        /* Button: set default to 2 */
        {
            .keycode = KEY_KPASTERISK + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = { .events_count = 0 },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },
        {
            .keycode = KEY_KPDOT + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = { .events_count = 0 },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 2,
                            .direction = XKB_POINTER_BUTTON_UP,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            },
        },
        {
            .keycode = KEY_KPPLUS + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 2,
                            .direction = XKB_POINTER_BUTTON_CLICK,
                            .count = 2,
                        }
                    }
                },
                .events_count = 1
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },

        /*
         * Check disabling mouse keys control: press keys with pointer actions
         */
        {
            .keycode = KEY_KP7 + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_PRESS,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_MOTION,
                        .pointer_motion = {
                            .size = sizeof(tests->down.events->pointer_motion),
                            .flags = XKB_POINTER_MOTION_REPEATS,
                            .x = -1,
                            .y = -1,
                        }
                    }
                },
                .events_count = 1
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 }
        },
        {
            .keycode = KEY_KP5 + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_PRESS,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 2,
                            .direction = XKB_POINTER_BUTTON_DOWN,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 }
        },
        {
            .keycode = KEY_KPEQUAL + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 2,
                            .direction = XKB_POINTER_BUTTON_DOWN,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },
        {
            .keycode = KEY_KPEQUAL + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_PRESS,
            .down = { .events_count = 0 },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },
        {
            /*
             * This key is only tapped before deactivating mouse keys, in order
             * to check that locked buttons are reset at mouse key reactivation.
             */
            .keycode = KEY_KPLEFTPAREN + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 1,
                            .direction = XKB_POINTER_BUTTON_DOWN,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 },
        },

        /*
         * Check disabling mouse keys control: switch mouse keys off
         */
        {
            .keycode = KEY_KPPLUSMINUS + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KPPLUSMINUS + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KPPLUSMINUS + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_CONTROLS
                                     | XKB_STATE_LEDS,
                            .components = {
                                .controls = 0,
                                .leds = 0,
                            },
                        }
                    }
                },
                .events_count = 2
            },
        },

        /*
         * Check disabling mouse keys control: release keys with pointer actions
         */
        {
            .keycode = KEY_KP7 + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_RELEASE,
            .down = { .events_count = 0 },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 }
        },
        {
            .keycode = KEY_KP5 + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_RELEASE,
            .down = { .events_count = 0 },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 2,
                            .direction = XKB_POINTER_BUTTON_UP,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            }
        },
        {
            .keycode = KEY_KPEQUAL + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_RELEASE,
            .down = { .events_count = 0 },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 2,
                            .direction = XKB_POINTER_BUTTON_UP,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            },
        },

        /*
         * Check disabling mouse keys control: switch mouse keys on
         */
        {
            .keycode = KEY_KPPLUSMINUS + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KPPLUSMINUS + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_CONTROLS
                                     | XKB_STATE_LEDS,
                            .components = {
                                .controls = CONTROL_MOUSE_KEYS,
                                .leds = mouse_keys,
                            },
                        }
                    }
                },
                .events_count = 2
            },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_KPPLUSMINUS + EVDEV_OFFSET,
                            .direction = XKB_KEY_UP
                        }
                    },
                },
                .events_count = 1
            },
        },

        /*
         * Check disabling mouse keys control: locked controls value
         */
        {
            .keycode = KEY_KPLEFTPAREN + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEAT,
            .down = { .events_count = 0 },
            .repeat = { .events_count = 0 },
            .up = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_POINTER_BUTTON,
                        .pointer_button = {
                            .size = sizeof(tests->down.events->pointer_button),
                            .button = 1,
                            .direction = XKB_POINTER_BUTTON_UP,
                            .count = 1,
                        }
                    }
                },
                .events_count = 1
            },
        },
    };

    for (size_t t = 0; t < ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: #%zu, keycode: %"PRIu32" ***\n",
                __func__, t, tests[t].keycode);
        assert(xkb_keymap_key_repeats(keymap, tests[t].keycode) ==
               tests[t].repeats);
        if (tests[t].directions & XKB_KEY_PRESS) {
            fprintf(stderr, "+++ Press +++\n");
            assert(xkb_machine_process_key(sm, tests[t].keycode,
                                        XKB_KEY_DOWN, events) == XKB_SUCCESS);
            assert(check_events(events, tests[t].down.events,
                                tests[t].down.events_count));
        }
        if (tests[t].directions & XKB_KEY_REPEAT) {
            fprintf(stderr, "+++ Repeat +++\n");
            assert(xkb_machine_process_key(sm, tests[t].keycode,
                                        XKB_KEY_REPEATED, events) == XKB_SUCCESS);
            if (tests[t].repeats) {
                assert(check_events(events, tests[t].repeat.events,
                                    tests[t].repeat.events_count));
            } else {
                assert(check_events(events, NULL, 0));
            }
        }
        if (tests[t].directions & XKB_KEY_RELEASE) {
            fprintf(stderr, "+++ Release +++\n");
            assert(xkb_machine_process_key(sm, tests[t].keycode,
                                        XKB_KEY_UP, events) == XKB_SUCCESS);
            assert(check_events(events, tests[t].up.events,
                                tests[t].up.events_count));
        }
    }

    xkb_events_unref(events);
    xkb_machine_unref(sm);
    xkb_keymap_unref(keymap);
}


static void
test_server_actions(struct xkb_context *ctx)
{
    struct xkb_keymap * const keymap =
        test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                           "evdev", "pc104", "us", "",
                           "terminate:ctrl_alt_bksp");
    assert(keymap);

    const struct xkb_machine_builder_config config = {
        .size = sizeof(config),
        .builder_flags = XKB_MACHINE_BUILDER_NO_FLAGS,
        .machine_flags = XKB_MACHINE_SERVER_ACTIONS,
    };
    struct xkb_machine_builder * const builder =
        xkb_machine_builder_new(keymap, &config, NULL);
    assert(builder);

    struct xkb_machine * const sm = xkb_machine_new(builder, NULL);
    assert(sm);
    xkb_machine_builder_unref(builder);

    struct xkb_events *events = xkb_events_new(ctx, NULL, NULL);
    assert(events);

    const xkb_mod_mask_t ctrl =
        _xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_CTRL);
    const xkb_mod_mask_t alt =
        _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_ALT);

    const struct {
        xkb_keycode_t keycode;
        enum key_directions directions;
        bool repeats;
        struct test_events {
            struct xkb_event events[2];
            unsigned int events_count;
        } down;
        struct test_events repeat;
        struct test_events up;
    } tests[] = {
        /*
         * Terminate server
         */

        {
            .keycode = KEY_RIGHTCTRL + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_PRESS,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTCTRL + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .base_mods = ctrl,
                                .mods = ctrl,
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 }
        },
        {
            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
            .repeats = false,
            .directions = XKB_KEY_PRESS,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_KEY,
                        .key = {
                            .keycode = KEY_RIGHTALT + EVDEV_OFFSET,
                            .direction = XKB_KEY_DOWN
                        }
                    },
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
                        .components = {
                            .changed = XKB_STATE_MODS_DEPRESSED
                                     | XKB_STATE_MODS_EFFECTIVE,
                            .components = {
                                .base_mods = (ctrl | alt),
                                .mods = (ctrl | alt),
                            }
                        }
                    }
                },
                .events_count = 2
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 }
        },
        {
            .keycode = KEY_BACKSPACE + EVDEV_OFFSET,
            .repeats = true,
            .directions = XKB_KEY_TAP | XKB_KEY_REPEATED,
            .down = {
                .events = {
                    {
                        .ctx = ctx,
                        .type = XKB_EVENT_TYPE_TERMINATE_DISPLAY_SERVER,
                    },
                },
                .events_count = 1
            },
            .repeat = { .events_count = 0 },
            .up = { .events_count = 0 }
        },
    };

    for (size_t t = 0; t < ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: #%zu, keycode: %"PRIu32" ***\n",
                __func__, t, tests[t].keycode);
        assert(xkb_keymap_key_repeats(keymap, tests[t].keycode) ==
               tests[t].repeats);
        if (tests[t].directions & XKB_KEY_PRESS) {
            fprintf(stderr, "+++ Press +++\n");
            assert(xkb_machine_process_key(sm, tests[t].keycode,
                                        XKB_KEY_DOWN, events) == XKB_SUCCESS);
            assert(check_events(events, tests[t].down.events,
                                tests[t].down.events_count));
        }
        if (tests[t].directions & XKB_KEY_REPEAT) {
            fprintf(stderr, "+++ Repeat +++\n");
            assert(xkb_machine_process_key(sm, tests[t].keycode,
                                        XKB_KEY_REPEATED, events) == XKB_SUCCESS);
            if (tests[t].repeats) {
                assert(check_events(events, tests[t].repeat.events,
                                    tests[t].repeat.events_count));
            } else {
                assert(check_events(events, NULL, 0));
            }
        }
        if (tests[t].directions & XKB_KEY_RELEASE) {
            fprintf(stderr, "+++ Release +++\n");
            assert(xkb_machine_process_key(sm, tests[t].keycode,
                                        XKB_KEY_UP, events) == XKB_SUCCESS);
            assert(check_events(events, tests[t].up.events,
                                tests[t].up.events_count));
        }
    }

    xkb_events_unref(events);
    xkb_machine_unref(sm);
    xkb_keymap_unref(keymap);
}

static void
test_shortcuts_tweak(struct xkb_context *context)
{
    struct xkb_keymap * const keymap =
        test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V2,
                           "evdev", "pc104", "us,il,de,ru", ",,neo,",
                           "grp:menu_toggle,grp:win_switch,ctrl:rctrl_latch,ctrl:copy");
    assert(keymap);

    const xkb_mod_mask_t ctrl = _xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_CTRL);
    const xkb_mod_mask_t alt = _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_ALT);
    const xkb_mod_mask_t level3 = _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_LEVEL3);
    const xkb_mod_mask_t level5 = _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_LEVEL5);

    struct xkb_machine_builder *builder =
        xkb_machine_builder_new(keymap, NULL, NULL);
    assert(builder);

    struct xkb_machine_builder_shortcut_layout_update update = {
        .size = sizeof(update),
    };
    update.source = 1;
    update.target = 2;
    assert(xkb_machine_builder_update_shortcut_layout(builder, &update) == XKB_SUCCESS);
    update.source = 3;
    update.target = 0;
    assert(xkb_machine_builder_update_shortcut_layout(builder, &update) == XKB_SUCCESS);
    update.source = XKB_LAYOUT_INVALID;
    update.mods_affect = ctrl;
    update.mods = ctrl;
    assert(xkb_machine_builder_update_shortcut_layout(builder, &update) == XKB_SUCCESS);

    struct xkb_machine * sm = xkb_machine_new(builder, NULL);
    assert(sm);

    struct xkb_events * const events = xkb_events_new(context, NULL, NULL);
    assert(events);

    /*
     * xkb_machine_process_key
     */

    assert(test_key_seq2(
        keymap, sm, events,
        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z             , NEXT,
        KEY_C       , BOTH, XKB_KEY_c             , NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z             , NEXT,
        KEY_C       , BOTH, XKB_KEY_XF86Copy      , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_LEFTALT , DOWN, XKB_KEY_Alt_L         , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z             , NEXT,
        KEY_LEFTALT , UP  , XKB_KEY_Alt_L         , NEXT,

        /* Layout 2: set */

        KEY_LEFTMETA, DOWN, XKB_KEY_ISO_Group_Shift, NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash          , NEXT, /* Layout 2 */
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain    , NEXT,
        KEY_LEFTMETA, UP  , XKB_KEY_ISO_Group_Shift, NEXT,

        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L      , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q              , NEXT, /* Layout 1 (unchanged) */
        KEY_Z       , BOTH, XKB_KEY_z              , NEXT,
        KEY_LEFTMETA, DOWN, XKB_KEY_ISO_Group_Shift, NEXT, /* Layout 2 */
        KEY_Q       , BOTH, XKB_KEY_x              , NEXT, /* Redirect to layout 3 */
        KEY_Z       , BOTH, XKB_KEY_udiaeresis     , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L      , NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash          , NEXT, /* Layout 2 */
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain    , NEXT,
        KEY_LEFTALT , DOWN, XKB_KEY_Alt_L          , NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash          , NEXT, /* No redirection with Alt */
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain    , NEXT,
        KEY_LEFTALT , UP  , XKB_KEY_Alt_L          , NEXT,
        KEY_LEFTMETA, UP  , XKB_KEY_ISO_Group_Shift, NEXT, /* Layout 1 */

        /* Layout 2: lock */

        KEY_COMPOSE , BOTH, XKB_KEY_ISO_Next_Group, NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash         , NEXT,
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain   , NEXT,
        KEY_102ND   , BOTH, XKB_KEY_less          , NEXT,
        /* Match mask: redirect to layout 3 */
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_x             , NEXT,
        KEY_Q       , DOWN, XKB_KEY_x             , NEXT,
        KEY_Q       , REPEAT, XKB_KEY_x           , NEXT,
        KEY_Q       , UP  , XKB_KEY_x             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_udiaeresis    , NEXT,
        KEY_102ND   , DOWN, XKB_KEY_ISO_Level5_Shift, NEXT,
        KEY_102ND   , UP  , XKB_KEY_ISO_Level5_Lock, NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash         , NEXT,
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain   , NEXT,
        /* No match: no redirect */
        KEY_LEFTALT , DOWN, XKB_KEY_Alt_L         , NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash         , NEXT,
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain   , NEXT,
        KEY_102ND   , BOTH, XKB_KEY_less          , NEXT,
        /* Match mask: redirect to layout 3 */
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_x             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_udiaeresis    , NEXT,
        KEY_102ND   , DOWN, XKB_KEY_ISO_Level5_Shift, NEXT,
        KEY_102ND   , UP  , XKB_KEY_ISO_Level5_Lock, NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash         , NEXT,
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain   , NEXT,
        KEY_102ND   , BOTH, XKB_KEY_less          , NEXT,
        KEY_LEFTALT , UP  , XKB_KEY_Alt_L         , NEXT,
        KEY_COMPOSE , BOTH, XKB_KEY_ISO_Next_Group, NEXT,

        /* Layout 3 */

        KEY_Q       , BOTH, XKB_KEY_x             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_udiaeresis    , NEXT,
        KEY_C       , BOTH, XKB_KEY_adiaeresis    , NEXT,
        KEY_102ND   , DOWN, XKB_KEY_ISO_Level5_Shift, NEXT,
        KEY_102ND   , UP  , XKB_KEY_ISO_Level5_Lock, NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_x             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_udiaeresis    , NEXT,
        KEY_C       , BOTH, XKB_KEY_adiaeresis    , NEXT,
        KEY_102ND   , DOWN, XKB_KEY_ISO_Level5_Shift, NEXT,
        KEY_102ND   , UP  , XKB_KEY_ISO_Level5_Lock, NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_LEFTALT , DOWN, XKB_KEY_Alt_L         , NEXT,
        KEY_Q       , BOTH, XKB_KEY_x             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_udiaeresis    , NEXT,
        KEY_102ND   , DOWN, XKB_KEY_ISO_Level5_Shift, NEXT,
        KEY_102ND   , UP  , XKB_KEY_ISO_Level5_Lock, NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Q       , BOTH, XKB_KEY_x             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_udiaeresis    , NEXT,
        KEY_C       , BOTH, XKB_KEY_adiaeresis    , NEXT,
        KEY_102ND   , DOWN, XKB_KEY_ISO_Level5_Shift, NEXT,
        KEY_102ND   , UP  , XKB_KEY_ISO_Level5_Lock, NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_LEFTALT , UP  , XKB_KEY_Alt_L         , NEXT,
        KEY_COMPOSE , BOTH, XKB_KEY_ISO_Next_Group, NEXT,

        /* Layout 4 */

        KEY_Q       , BOTH, XKB_KEY_Cyrillic_shorti, NEXT,
        KEY_Z       , BOTH, XKB_KEY_Cyrillic_ya    , NEXT,
        KEY_C       , BOTH, XKB_KEY_Cyrillic_es    , NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L      , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q              , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z              , NEXT,
        KEY_C       , BOTH, XKB_KEY_XF86Copy       , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L      , NEXT,
        KEY_LEFTALT , DOWN, XKB_KEY_Alt_L          , NEXT,
        KEY_Q       , BOTH, XKB_KEY_Cyrillic_shorti, NEXT,
        KEY_Z       , BOTH, XKB_KEY_Cyrillic_ya    , NEXT,
        KEY_C       , BOTH, XKB_KEY_Cyrillic_es    , NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L      , NEXT,
        KEY_Q       , BOTH, XKB_KEY_q              , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z              , NEXT,
        KEY_C       , BOTH, XKB_KEY_XF86Copy       , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L      , NEXT,
        KEY_LEFTALT , UP  , XKB_KEY_Alt_L          , NEXT,
        KEY_COMPOSE , BOTH, XKB_KEY_ISO_Next_Group , NEXT,

        /* Layout 1 */

        KEY_Q       , BOTH, XKB_KEY_q             , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z             , NEXT,

        /* Layout 2 */

        KEY_COMPOSE , BOTH, XKB_KEY_ISO_Next_Group, NEXT,
        KEY_Q       , BOTH, XKB_KEY_slash         , NEXT,
        KEY_Z       , BOTH, XKB_KEY_hebrew_zain   , NEXT,
        KEY_102ND   , BOTH, XKB_KEY_less          , NEXT,
        KEY_RIGHTCTRL,BOTH, XKB_KEY_Control_R     , FINISH
    ));

    const xkb_led_mask_t group2 =
        UINT32_C(1) << xkb_keymap_led_get_index(keymap, "Group 2");

    /*
     * xkb_machine_process_key
     */
    assert(xkb_machine_process_key(sm, KEY_Q + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = 1,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 2,
                    .leds = group2,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Q + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = 0,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_Q + EVDEV_OFFSET,
                                   XKB_KEY_REPEATED, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Q + EVDEV_OFFSET,
                .direction = XKB_KEY_REPEATED
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_Q + EVDEV_OFFSET,
                                   XKB_KEY_UP, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Q + EVDEV_OFFSET,
                .direction = XKB_KEY_UP
            }
        }
    );

    assert(xkb_machine_process_key(sm, KEY_RIGHTCTRL + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_RIGHTCTRL + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = ctrl,
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_RIGHTCTRL + EVDEV_OFFSET,
                                   XKB_KEY_REPEATED, events) == 0);
    check_events_(
        events,
        { .ctx = context, .type = XKB_EVENT_TYPE_NONE }, /* does not repeat */
    );

    assert(xkb_machine_process_key(sm, KEY_RIGHTCTRL + EVDEV_OFFSET,
                                   XKB_KEY_UP, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .base_mods = ctrl,
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = 1,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 2,
                    .leds = group2,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_RIGHTCTRL + EVDEV_OFFSET,
                .direction = XKB_KEY_UP
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .base_mods = ctrl,
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED,
                .components = {
                    .base_mods = 0,
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_102ND + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = 1,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 2,
                    .leds = group2,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_102ND + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl | level5,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_Q + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl | level5,
                    .base_group = 1,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 2,
                    .leds = group2,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Q + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl | level5,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = level5,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_Q + EVDEV_OFFSET,
                                   XKB_KEY_UP, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Q + EVDEV_OFFSET,
                .direction = XKB_KEY_UP
            }
        }
    );

    assert(xkb_machine_process_key(sm, KEY_102ND + EVDEV_OFFSET,
                                   XKB_KEY_UP, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_102ND + EVDEV_OFFSET,
                .direction = XKB_KEY_UP
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = 0,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
    );

    /*
     * xkb_machine_update_latched_locked
     */

    /* Layout 1 locked, Ctrl latched */
    assert(xkb_machine_update_latched_locked(sm, events,
                                             ctrl, ctrl, false, 0,
                                             0, 0, true, 1) == XKB_SUCCESS);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        }
    );

    /* Layout 1 locked, Ctrl locked */
    assert(xkb_machine_update_latched_locked(sm, events,
                                             ctrl, 0, false, 0,
                                             ctrl, ctrl, false, 0) == XKB_SUCCESS);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_LOCKED,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        }
    );

    assert(xkb_machine_process_key(sm, KEY_Q + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 1,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 2,
                    .leds = group2,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Q + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_Q + EVDEV_OFFSET,
                                   XKB_KEY_REPEATED, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 1,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 2,
                    .leds = group2,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Q + EVDEV_OFFSET,
                .direction = XKB_KEY_REPEATED
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_Q + EVDEV_OFFSET,
                                   XKB_KEY_UP, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 1,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 2,
                    .leds = group2,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Q + EVDEV_OFFSET,
                .direction = XKB_KEY_UP
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2,
                }
            }
        },
    );

    /* Layout 1 latched, layout 2 locked, Ctrl locked */
    assert(xkb_machine_update_latched_locked(sm, events,
                                             0, 0, true, 1,
                                             0, 0, true, 2) == XKB_SUCCESS);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_LOCKED
                         | XKB_STATE_LAYOUT_EFFECTIVE,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 1,
                    .locked_group = 2,
                    .group = 3,
                    .leds = group2,
                }
            }
        }
    );

    /* Layout 1 latched, layout 2 locked, Ctrl disabled */
    assert(xkb_machine_update_latched_locked(sm, events,
                                             0, 0, false, 0,
                                             ctrl, 0, false, 0) == XKB_SUCCESS);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = 0,
                    .base_group = 0,
                    .latched_group = 1,
                    .locked_group = 2,
                    .group = 3,
                    .leds = group2,
                }
            }
        }
    );

    /* Layout 1 latched, layout 2 locked, Ctrl latched */
    assert(xkb_machine_update_latched_locked(sm, events,
                                             ctrl, ctrl, false, 0,
                                             0, 0, false, 0) == XKB_SUCCESS);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 1,
                    .locked_group = 2,
                    .group = 3,
                    .leds = group2,
                }
            }
        }
    );

    /*
     * xkb_machine_update_enabled_controls
     */

    const enum xkb_keyboard_control_flags controls =
        XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS;

    /* Disable already disabled sticky keys: no change */
    assert(xkb_machine_update_enabled_controls(sm, events, controls, 0)
           == XKB_SUCCESS);
    check_events_(events, { .type = XKB_EVENT_TYPE_NONE });

    /* Enable disabled sticky keys: no change */
    assert(xkb_machine_update_enabled_controls(sm, events, controls, controls)
           == XKB_SUCCESS);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_CONTROLS,
                .components = {
                    .latched_mods = ctrl,
                    .locked_mods = 0,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 1,
                    .locked_group = 2,
                    .group = 3,
                    .leds = group2,
                    .controls = CONTROL_STICKY_KEYS,
                }
            }
        }
    );

    /* Enable already enabled sticky keys: no change */
    assert(xkb_machine_update_enabled_controls(sm, events, controls, controls)
           == XKB_SUCCESS);
    check_events_(events, { .ctx = context, .type = XKB_EVENT_TYPE_NONE });

    /* Disable sticky keys: clear latches & locks */
    assert(xkb_machine_update_enabled_controls(sm, events, controls, 0)
           == XKB_SUCCESS);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LATCHED | XKB_STATE_MODS_EFFECTIVE
                         | XKB_STATE_LAYOUT_LATCHED | XKB_STATE_LAYOUT_LOCKED
                         | XKB_STATE_LAYOUT_EFFECTIVE | XKB_STATE_CONTROLS
                         | XKB_STATE_LEDS,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = 0,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 0,
                    .group = 0,
                    .leds = 0,
                    .controls = 0,
                }
            }
        }
    );

    assert(xkb_machine_update_enabled_controls(sm, events, controls, 0)
           == XKB_SUCCESS);

    /*
     * Check RedirectKey()
     */

    /* Layout 4 locked, Ctrl locked */
    assert(xkb_machine_update_latched_locked(sm, events,
                                             0, 0, false, 0,
                                             ctrl, ctrl, true, 3) == XKB_SUCCESS);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE
                         | XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 3,
                    .leds = group2,
                }
            }
        }
    );

    assert(xkb_machine_process_key(sm, KEY_C + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = -3,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 0,
                    .leds = 0,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = 0,
                    .base_group = -3,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 0,
                    .leds = 0,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_COPY + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = -3,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 0,
                    .leds = 0,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 3,
                    .leds = group2,
                }
            }
        },
    );

    xkb_machine_unref(sm);

    /*
     * Use modifiers tweak in addition to the shortcuts tweak
     */

    const struct xkb_machine_builder_mods_remap_update mods_remap_update = {
        .size = sizeof(mods_remap_update),
        .source = ctrl | alt,
        .target = level3
    };
    assert(xkb_machine_builder_update_mods_remap(builder, &mods_remap_update) ==
           XKB_SUCCESS);

    sm = xkb_machine_new(builder, NULL);
    assert(sm);

    assert(xkb_machine_update_latched_locked(sm, events,
                                             0, 0, false, 0,
                                             ctrl, ctrl, true, 3) == XKB_SUCCESS);

    assert(xkb_machine_process_key(sm, KEY_Q + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = -3,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 0,
                    .leds = 0,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Q + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 3,
                    .leds = group2,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_C + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = -3,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 0,
                    .leds = 0,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = 0,
                    .base_group = -3,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 0,
                    .leds = 0,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_COPY + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = -3,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 0,
                    .leds = 0,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_DEPRESSED | XKB_STATE_LAYOUT_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl,
                    .mods = ctrl,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 3,
                    .leds = group2,
                }
            }
        },
    );

    assert(xkb_machine_update_latched_locked(sm, events,
                                             0, 0, false, 0,
                                             alt, alt, false, 0) == XKB_SUCCESS);

    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl | alt,
                    .mods = ctrl | alt,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 3,
                    .leds = group2,
                }
            }
        }
    );

    assert(xkb_machine_process_key(sm, KEY_Q + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level3,
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = level3,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 3,
                    .leds = group2,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Q + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .latched_mods = 0,
                    .locked_mods = ctrl | alt,
                    .mods = ctrl | alt,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 3,
                    .group = 3,
                    .leds = group2,
                }
            }
        },
    );

    assert(xkb_machine_update_latched_locked(sm, events,
                                             0, 0, false, 0,
                                             0, 0, true, 0) == XKB_SUCCESS);

    assert(xkb_machine_process_key(sm, KEY_Q + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level3,
                    .latched_mods = 0,
                    .locked_mods = 0,
                    .mods = level3,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 0,
                    .group = 0,
                    .leds = 0,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Q + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .latched_mods = 0,
                    .locked_mods = ctrl | alt,
                    .mods = ctrl | alt,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 0,
                    .group = 0,
                    .leds = 0,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_C + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = alt,
                    .mods = alt,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 0,
                    .group = 0,
                    .leds = 0,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_COPY + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .latched_mods = 0,
                    .locked_mods = ctrl | alt,
                    .mods = ctrl | alt,
                    .base_group = 0,
                    .latched_group = 0,
                    .locked_group = 0,
                    .group = 0,
                    .leds = 0,
                }
            }
        },
    );

    xkb_machine_unref(sm);
    xkb_events_unref(events);
    xkb_machine_builder_unref(builder);
    xkb_keymap_unref(keymap);
}

static void
test_overlays(struct xkb_context *context)
{
    /* Check controls → overlay mask conversion */
    static const struct {
        enum xkb_action_controls controls;
        xkb_overlay_mask_t overlays;
    } controls_tests[] = {
        { 0, 0x0 },
        { CONTROL_BELL, 0x0 },
        { CONTROL_OVERLAY1, 0x1 },
        { CONTROL_OVERLAY1 | CONTROL_OVERLAY2, 0x3 },
        { CONTROL_OVERLAY3, (1u << 2) },
        { CONTROL_OVERLAY8, (1u << 7) },
        { CONTROL_ALL, 0xff },
        { CONTROL_ALL_V1, 0x3 },
        { CONTROL_ALL_BOOLEAN, 0xff },
        { CONTROL_ALL_BOOLEAN_V1, 0x3 },
    };

    for (size_t t = 0; t < ARRAY_SIZE(controls_tests); t++) {
        fprintf(stderr, "------\n*** %s: controls #%zu ***\n", __func__, t);
        assert_eq("", controls_tests[t].overlays,
                  (uint8_t)OVERLAYS_FROM_CONTROLS(controls_tests[t].controls),
                  "0x%02x");
    }

    /* Check overlapping overlays */
    struct xkb_keymap * const keymap = test_compile_file(
        context, XKB_KEYMAP_FORMAT_TEXT_V2,
        GOLDEN_TESTS_OUTPUTS "overlays-v2-2.xkb");
    assert(keymap);

    struct xkb_machine_builder *builder =
        xkb_machine_builder_new(keymap, NULL, NULL);
    assert(builder);
    struct xkb_machine * const sm = xkb_machine_new(builder, NULL);
    assert(sm);
    xkb_machine_builder_unref(builder);
    struct xkb_events * events = xkb_events_new(context, NULL, NULL);
    assert(events);

    static const struct {
        enum xkb_keyboard_control_flags controls;
        xkb_keycode_t kc;
        enum xkb_key_direction direction;
    } keycode_tests[] = {
        /* No overlay */
        { 0, KEY_J, XKB_KEY_DOWN },
        { 0, KEY_J, XKB_KEY_UP },
        { 0, KEY_J, XKB_KEY_DOWN },
        /* Overlay enabled while key pressed: no effect */
        { XKB_KEYBOARD_CONTROL_OVERLAY1, KEY_J, XKB_KEY_UP },
        /* Overlay enabled before and after key press: effectual */
        { XKB_KEYBOARD_CONTROL_OVERLAY1, KEY_KP1, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY1, KEY_KP1, XKB_KEY_UP },
        /* Overlay enabled before key press and disable before release : effectual */
        { XKB_KEYBOARD_CONTROL_OVERLAY1, KEY_KP1, XKB_KEY_DOWN },
        { 0, KEY_KP1, XKB_KEY_UP },
        /* Key does not belong to overlay: not effect */
        { XKB_KEYBOARD_CONTROL_OVERLAY8, KEY_J, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY8, KEY_J, XKB_KEY_UP },
        /* Overlay activation order matters */
        { XKB_KEYBOARD_CONTROL_OVERLAY1, 0, 0 },
        { XKB_KEYBOARD_CONTROL_OVERLAY1 | XKB_KEYBOARD_CONTROL_OVERLAY2, KEY_LEFT, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY1 | XKB_KEYBOARD_CONTROL_OVERLAY2, KEY_LEFT, XKB_KEY_UP },
        { XKB_KEYBOARD_CONTROL_OVERLAY1 | XKB_KEYBOARD_CONTROL_OVERLAY2, KEY_LEFT, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY1, KEY_LEFT, XKB_KEY_UP },
        { XKB_KEYBOARD_CONTROL_OVERLAY1 | XKB_KEYBOARD_CONTROL_OVERLAY2, KEY_LEFT, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY2, KEY_LEFT, XKB_KEY_UP },
        /* If multiple overlays are activated simultaneously, they are stacked
         * in ascending order */
        { XKB_KEYBOARD_CONTROL_OVERLAY1 | XKB_KEYBOARD_CONTROL_OVERLAY2,
          KEY_KP1, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY1 | XKB_KEYBOARD_CONTROL_OVERLAY2,
          KEY_KP1, XKB_KEY_UP },
        { XKB_KEYBOARD_CONTROL_OVERLAY2, KEY_LEFT, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY2, KEY_LEFT, XKB_KEY_UP },
        { XKB_KEYBOARD_CONTROL_OVERLAY1 | XKB_KEYBOARD_CONTROL_OVERLAY2 |
          XKB_KEYBOARD_CONTROL_OVERLAY3 | XKB_KEYBOARD_CONTROL_OVERLAY4 |
          XKB_KEYBOARD_CONTROL_OVERLAY8,
          KEY_F10, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY1 | XKB_KEYBOARD_CONTROL_OVERLAY2 |
          XKB_KEYBOARD_CONTROL_OVERLAY3 | XKB_KEYBOARD_CONTROL_OVERLAY4 |
          XKB_KEYBOARD_CONTROL_OVERLAY8,
          KEY_F10, XKB_KEY_UP },
        { XKB_KEYBOARD_CONTROL_OVERLAY2 | XKB_KEYBOARD_CONTROL_OVERLAY3 |
          XKB_KEYBOARD_CONTROL_OVERLAY4 | XKB_KEYBOARD_CONTROL_OVERLAY8,
          KEY_F10, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY2 | XKB_KEYBOARD_CONTROL_OVERLAY3 |
          XKB_KEYBOARD_CONTROL_OVERLAY4 | XKB_KEYBOARD_CONTROL_OVERLAY8,
          KEY_F10, XKB_KEY_UP },
        { XKB_KEYBOARD_CONTROL_OVERLAY1 | XKB_KEYBOARD_CONTROL_OVERLAY2 |
          XKB_KEYBOARD_CONTROL_OVERLAY3 | XKB_KEYBOARD_CONTROL_OVERLAY8,
          KEY_KP1, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY1 | XKB_KEYBOARD_CONTROL_OVERLAY2 |
          XKB_KEYBOARD_CONTROL_OVERLAY3 | XKB_KEYBOARD_CONTROL_OVERLAY8,
          KEY_KP1, XKB_KEY_UP },
        { XKB_KEYBOARD_CONTROL_OVERLAY2 | XKB_KEYBOARD_CONTROL_OVERLAY3,
          KEY_F1, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY2 | XKB_KEYBOARD_CONTROL_OVERLAY3,
          KEY_F1, XKB_KEY_UP },
        { XKB_KEYBOARD_CONTROL_OVERLAY2 | XKB_KEYBOARD_CONTROL_OVERLAY3 |
          XKB_KEYBOARD_CONTROL_OVERLAY4, 0, 0},
        { XKB_KEYBOARD_CONTROL_OVERLAY1 | XKB_KEYBOARD_CONTROL_OVERLAY2 |
          XKB_KEYBOARD_CONTROL_OVERLAY3 | XKB_KEYBOARD_CONTROL_OVERLAY4,
          KEY_KP1, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY1 | XKB_KEYBOARD_CONTROL_OVERLAY2 |
          XKB_KEYBOARD_CONTROL_OVERLAY3 | XKB_KEYBOARD_CONTROL_OVERLAY4,
          KEY_KP1, XKB_KEY_UP },
        /* Multiple physical keys with same keycode */
        { XKB_KEYBOARD_CONTROL_OVERLAY1, KEY_KP1, XKB_KEY_DOWN },
        { 0, KEY_KP1, XKB_KEY_DOWN }, /* key still uses overlay 1 */
        { 0, KEY_KP1, XKB_KEY_UP },   /* key still uses overlay 1 */
        { 0, KEY_KP1, XKB_KEY_UP },   /* key still uses overlay 1 */
        { 0, KEY_J, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY1, KEY_J, XKB_KEY_DOWN }, /* No effect: key already down */
        { XKB_KEYBOARD_CONTROL_OVERLAY1, KEY_J, XKB_KEY_UP },   /* No effect: all keys must be depressed */
        { XKB_KEYBOARD_CONTROL_OVERLAY1, KEY_J, XKB_KEY_UP },   /* No effect: all keys must be depressed */
        { XKB_KEYBOARD_CONTROL_OVERLAY1, KEY_KP1, XKB_KEY_DOWN },
        { XKB_KEYBOARD_CONTROL_OVERLAY2, KEY_KP1, XKB_KEY_DOWN }, /* key still uses overlay 1 */
        { XKB_KEYBOARD_CONTROL_OVERLAY2, KEY_KP1, XKB_KEY_UP },   /* key still uses overlay 1 */
        { XKB_KEYBOARD_CONTROL_OVERLAY2, KEY_KP1, XKB_KEY_UP },   /* key still uses overlay 1 */
    };

    for (size_t t = 0; t < ARRAY_SIZE(keycode_tests); t++) {
        fprintf(stderr, "------\n*** %s: keycodes #%zu ***\n", __func__, t);
        assert(xkb_machine_update_enabled_controls(
                sm, events, 0xffff, keycode_tests[t].controls
        ) == XKB_SUCCESS);

        if (!keycode_tests[t].kc)
            continue;

        assert(xkb_machine_process_key(
            sm, KEY_J + EVDEV_OFFSET, keycode_tests[t].direction, events
        ) == XKB_SUCCESS);
        const struct xkb_event *event;
        while ((event = xkb_events_next(events))) {
            switch(xkb_event_get_type(event)) {
            case XKB_EVENT_TYPE_KEY: {
                xkb_keycode_t kc;
                enum xkb_key_direction direction;
                assert(xkb_event_get_keycode(event, &kc, &direction) == XKB_SUCCESS);
                assert_eq("keycode", keycode_tests[t].kc + EVDEV_OFFSET,
                          kc, "%"PRIu32);
                assert_eq("direction", keycode_tests[t].direction,
                          direction, "%d");
                break;
            }
            default:
                ;
            }
        }
    }

    xkb_events_unref(events);
    xkb_machine_unref(sm);
    xkb_keymap_unref(keymap);
}

static void
test_modifiers_tweak(struct xkb_context *context)
{
    struct xkb_keymap * const keymap =
        test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V2,
                           "evdev", "pc104", "us,de", ",T3",
                           "grp:menu_toggle,grp:alt_caps_toggle,"
                           "terminate:ctrl_alt_bksp,ctrl:copy");
    assert(keymap);

    const xkb_mod_mask_t shift = _xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_SHIFT);
    const xkb_mod_mask_t ctrl = _xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_CTRL);
    const xkb_mod_mask_t alt = _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_ALT);
    const xkb_mod_mask_t super = _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_SUPER);
    const xkb_mod_mask_t scroll = _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_SCROLL);
    const xkb_mod_mask_t level3 = _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_LEVEL3);
    const xkb_mod_mask_t level5 = _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_LEVEL5);
    const xkb_mod_mask_t num = _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_NUM);

    struct xkb_machine_builder *builder =
        xkb_machine_builder_new(keymap, NULL, NULL);
    assert(builder);

    struct xkb_machine_builder_mods_remap_update update = {
        .size = sizeof(update)
    };

    assert(xkb_machine_builder_update_mods_remap(builder, &update) == XKB_SUCCESS);
    update.source = 0;
    update.target = level3;
    assert(xkb_machine_builder_update_mods_remap(builder, &update) ==
           XKB_ERROR_UNSUPPORTED_MODIFIER_MASK);
    update.source = scroll;
    update.target = alt;
    assert(xkb_machine_builder_update_mods_remap(builder, &update) == XKB_SUCCESS);
    update.source = super;
    update.target = level3;
    assert(xkb_machine_builder_update_mods_remap(builder, &update) == XKB_SUCCESS);
    update.source = alt;
    update.target = level5;
    assert(xkb_machine_builder_update_mods_remap(builder, &update) == XKB_SUCCESS);
    update.source = ctrl | alt;
    update.target = level3;
    assert(xkb_machine_builder_update_mods_remap(builder, &update) == XKB_SUCCESS);

    update.source = ctrl;
    update.target = shift;
    assert(xkb_machine_builder_update_mods_remap(builder, &update) == XKB_SUCCESS);
    update.source = ctrl;
    update.target = 0;
    assert(xkb_machine_builder_update_mods_remap(builder, &update) == XKB_SUCCESS);

    struct xkb_machine * const sm = xkb_machine_new(builder, NULL);
    assert(sm);
    xkb_machine_builder_unref(builder);

    struct xkb_events * const events = xkb_events_new(context, NULL, NULL);
    assert(events);

    assert(test_key_seq2(
        keymap, sm, events,

        /* Layout: US */
        KEY_Y       , BOTH, XKB_KEY_y             , NEXT,
        KEY_C       , BOTH, XKB_KEY_c             , NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Y       , BOTH, XKB_KEY_y             , NEXT,
        KEY_C       , BOTH, XKB_KEY_XF86Copy      , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_LEFTMETA, DOWN, XKB_KEY_Super_L       , NEXT,
        KEY_Y       , BOTH, XKB_KEY_y             , NEXT,
        KEY_C       , BOTH, XKB_KEY_c             , NEXT,
        KEY_LEFTMETA, UP  , XKB_KEY_Super_L       , NEXT,
        KEY_LEFTALT , DOWN, XKB_KEY_Alt_L         , NEXT,
        KEY_Y       , BOTH, XKB_KEY_y             , NEXT,
        KEY_LEFTMETA, DOWN, XKB_KEY_Super_L       , NEXT,
        KEY_Y       , BOTH, XKB_KEY_y             , NEXT,
        KEY_LEFTMETA, UP  , XKB_KEY_Super_L       , NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Y       , BOTH, XKB_KEY_y             , NEXT,
        KEY_C       , BOTH, XKB_KEY_XF86Copy      , NEXT,
        KEY_BACKSPACE,BOTH, XKB_KEY_Terminate_Server, NEXT, /* No remap */
        KEY_LEFTALT , UP  , XKB_KEY_Alt_L         , NEXT,
        KEY_Y       , BOTH, XKB_KEY_y             , NEXT,
        KEY_C       , BOTH, XKB_KEY_XF86Copy      , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,

        KEY_COMPOSE , BOTH, XKB_KEY_ISO_Next_Group, NEXT,

        /* Layout: T3 */
        KEY_Y       , BOTH, XKB_KEY_z             , NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Y       , BOTH, XKB_KEY_z             , NEXT,
        KEY_Y       , DOWN, XKB_KEY_z             , NEXT,
        KEY_Y       , REPEAT, XKB_KEY_z           , NEXT,
        KEY_Y       , UP,   XKB_KEY_z             , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_LEFTMETA, DOWN, XKB_KEY_Super_L       , NEXT,
        KEY_Y       , BOTH, XKB_KEY_dead_doubleacute, NEXT,
        KEY_Y       , DOWN, XKB_KEY_dead_doubleacute, NEXT,
        KEY_Y       , REPEAT, XKB_KEY_dead_doubleacute, NEXT,
        KEY_Y       , UP,   XKB_KEY_dead_doubleacute, NEXT,
        KEY_LEFTMETA, UP  , XKB_KEY_Super_L       , NEXT,
        KEY_LEFTALT , DOWN, XKB_KEY_Alt_L         , NEXT,
        KEY_Y       , BOTH, U(0x027C)             , NEXT,
        KEY_LEFTMETA, DOWN, XKB_KEY_Super_L       , NEXT,
        KEY_Y       , BOTH, XKB_KEY_dead_invertedbreve, NEXT,
        KEY_LEFTMETA, UP  , XKB_KEY_Super_L       , NEXT,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L     , NEXT,
        KEY_Y       , BOTH, XKB_KEY_dead_doubleacute, NEXT,
        KEY_BACKSPACE,BOTH, XKB_KEY_Terminate_Server, NEXT, /* No remap */
        KEY_LEFTALT , UP  , XKB_KEY_Alt_L         , NEXT,
        KEY_Y       , BOTH, XKB_KEY_z             , NEXT,
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , FINISH
    ));

    const xkb_led_mask_t num_led =
        UINT32_C(1) << xkb_keymap_led_get_index(keymap, XKB_LED_NAME_NUM);
    const xkb_led_mask_t scroll_led =
        UINT32_C(1) << xkb_keymap_led_get_index(keymap, XKB_LED_NAME_SCROLL);
    const xkb_led_mask_t group2_led =
        UINT32_C(1) << xkb_keymap_led_get_index(keymap, "Group 2");

    assert(xkb_machine_process_key(sm, KEY_LEFTALT + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_LEFTALT + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = alt,
                    .mods = alt,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led,
                }
            }
        }
    );

    assert(xkb_machine_process_key(sm, KEY_Y + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .mods = level5,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Y + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = alt,
                    .mods = alt,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_Y + EVDEV_OFFSET,
                                   XKB_KEY_REPEATED, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .mods = level5,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Y + EVDEV_OFFSET,
                .direction = XKB_KEY_REPEATED
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = alt,
                    .mods = alt,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_Y + EVDEV_OFFSET,
                                   XKB_KEY_UP, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .mods = level5,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Y + EVDEV_OFFSET,
                .direction = XKB_KEY_UP
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = alt,
                    .mods = alt,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led,
                }
            }
        },
    );

    assert(xkb_machine_update_latched_locked(sm, events,
                                             0, 0, false, 0,
                                             ctrl | num, ctrl | num,
                                             false, 0) == XKB_SUCCESS);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .base_mods = alt,
                    .locked_mods = ctrl | num,
                    .mods = ctrl | alt | num,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led | num_led,
                }
            }
        }
    );

    assert(xkb_machine_process_key(sm, KEY_Y + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level3,
                    .locked_mods = num,
                    .mods = level3 | num,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led | num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Y + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = alt,
                    .locked_mods = ctrl | num,
                    .mods = ctrl | alt | num,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led | num_led,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_Y + EVDEV_OFFSET,
                                   XKB_KEY_UP, events) == 0);

    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level3,
                    .locked_mods = num,
                    .mods = level3 | num,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led | num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_Y + EVDEV_OFFSET,
                .direction = XKB_KEY_UP
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = alt,
                    .locked_mods = ctrl | num,
                    .mods = ctrl | alt | num,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led | num_led,
                }
            }
        },
    );

    /* Key type `CTRL+ALT` partially matches the remapping source: no remap */
    assert(xkb_machine_process_key(sm, KEY_BACKSPACE + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_BACKSPACE + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        }
    );

    assert(xkb_machine_process_key(sm, KEY_LEFTALT + EVDEV_OFFSET,
                                   XKB_KEY_UP, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level3,
                    .locked_mods = num,
                    .mods = level3 | num,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led | num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_LEFTALT + EVDEV_OFFSET,
                .direction = XKB_KEY_UP
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = alt,
                    .locked_mods = ctrl | num,
                    .mods = ctrl | alt | num,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led | num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .locked_mods = ctrl | num,
                    .mods = ctrl | num,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led | num_led,
                }
            }
        }
    );

    assert(xkb_machine_update_latched_locked(sm, events,
                                             0, 0, false, 0,
                                             ctrl | scroll, scroll,
                                             false, 0) == XKB_SUCCESS);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .base_mods = 0,
                    .locked_mods = num | scroll,
                    .mods = num | scroll,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led | num_led | scroll_led,
                }
            }
        }
    );

    /* Ensure CAPS action is triggered */
    assert(xkb_machine_process_key(sm, KEY_CAPSLOCK + EVDEV_OFFSET,
                                   XKB_KEY_DOWN, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS,
                .components = {
                    .base_mods = alt,
                    .locked_mods = num,
                    .mods = alt | num,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led | num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_CAPSLOCK + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LEDS,
                .components = {
                    .base_mods = 0,
                    .locked_mods = num | scroll,
                    .mods = num | scroll,
                    .locked_group = 1,
                    .group = 1,
                    .leds = group2_led | num_led | scroll_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_LAYOUT_LOCKED | XKB_STATE_LAYOUT_EFFECTIVE
                         | XKB_STATE_LEDS,
                .components = {
                    .base_mods = 0,
                    .locked_mods = num | scroll,
                    .mods = num | scroll,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led | scroll_led,
                }
            }
        },
    );

    assert(xkb_machine_update_latched_locked(sm, events,
                                             0, 0, false, 0,
                                             ctrl | alt | scroll, ctrl | alt,
                                             false, 0) == XKB_SUCCESS);

    /* Key redirect */
    assert(xkb_machine_process_key(sm, KEY_C + EVDEV_OFFSET, XKB_KEY_DOWN,
                                   events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .locked_mods = ctrl | num,
                    .mods = ctrl | level5 | num,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .locked_mods = num,
                    .mods = level5 | num,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_COPY + EVDEV_OFFSET,
                .direction = XKB_KEY_DOWN
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .locked_mods = ctrl | num,
                    .mods = ctrl | level5 | num,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .locked_mods = ctrl | alt | num,
                    .mods = ctrl | alt | num,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_C + EVDEV_OFFSET,
                                   XKB_KEY_REPEATED, events) == 0);
    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .locked_mods = ctrl | num,
                    .mods = ctrl | level5 | num,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .locked_mods = num,
                    .mods = level5 | num,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_COPY + EVDEV_OFFSET,
                .direction = XKB_KEY_REPEATED
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .locked_mods = ctrl | num,
                    .mods = ctrl | level5 | num,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .locked_mods = ctrl | alt | num,
                    .mods = ctrl | alt | num,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led,
                }
            }
        },
    );

    assert(xkb_machine_process_key(sm, KEY_C + EVDEV_OFFSET,
                                   XKB_KEY_UP, events) == 0);

    check_events_(
        events,
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .locked_mods = ctrl | num,
                    .mods = ctrl | level5 | num,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .locked_mods = num,
                    .mods = level5 | num,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_KEY,
            .key = {
                .keycode = KEY_COPY + EVDEV_OFFSET,
                .direction = XKB_KEY_UP
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_LOCKED | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = level5,
                    .locked_mods = ctrl | num,
                    .mods = ctrl | level5 | num,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led,
                }
            }
        },
        {
            .ctx = context,
            .type = XKB_EVENT_TYPE_STATE_COMPONENTS,
            .components = {
                .changed = XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LOCKED
                         | XKB_STATE_MODS_EFFECTIVE,
                .components = {
                    .base_mods = 0,
                    .locked_mods = ctrl | alt | num,
                    .mods = ctrl | alt | num,
                    .locked_group = 0,
                    .group = 0,
                    .leds = num_led,
                }
            }
        },
    );

    xkb_events_unref(events);
    xkb_machine_unref(sm);
    xkb_keymap_unref(keymap);
}

/* Example from the public header */
static void
test_xkb_machine_builder_mods_remap_update(struct xkb_context *context)
{
    struct xkb_keymap * const keymap_ =
        test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V2,
                           "evdev", "pc104", "us,de", ",T3",
                           "grp:menu_toggle,grp:alt_caps_toggle,"
                           "terminate:ctrl_alt_bksp,ctrl:copy");
    assert(keymap_);

    struct xkb_machine_builder *builder =
        xkb_machine_builder_new(keymap_, NULL, NULL);
    assert(builder);

    /* Use another variable `keymap` for the snippet */
//! [xkb_machine_builder_mods_remap_update_example]
    struct xkb_keymap *keymap = xkb_machine_builder_get_keymap(builder);

    const xkb_mod_mask_t ctrl = xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_CTRL);
    const xkb_mod_mask_t alt = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_ALT);
    const xkb_mod_mask_t level3 = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_LEVEL3);

    const struct xkb_machine_builder_mods_remap_update update = {
        .size = sizeof(update),
        .source = ctrl | alt,
        .target = level3
    };
    const enum xkb_error_code error =
        xkb_machine_builder_update_mods_remap(builder, &update);
    if (error != XKB_SUCCESS) {
        // handle error
        assert(!"error");
    }
//! [xkb_machine_builder_mods_remap_update_example]

    xkb_machine_builder_unref(builder);
    xkb_keymap_unref(keymap_);
}

/* Example from the public header */
static void
test_machine_builder_shortcut_layout_update(struct xkb_context *context)
{
    enum {
        SHORTCUT_LAYOUT_UPDATE_EXAMPLE_1 = 0,
        SHORTCUT_LAYOUT_UPDATE_EXAMPLE_2,
        SHORTCUT_LAYOUT_UPDATE_EXAMPLE_3,
        SHORTCUT_LAYOUT_UPDATE_EXAMPLE_COUNT
    };
    struct xkb_keymap *keymap_ = NULL;
    struct xkb_machine_builder *builder = NULL;
    static const struct {
        xkb_keysym_t unmatched;
        xkb_keysym_t ctrl_alt;
        xkb_keysym_t super;
        xkb_keysym_t some;
    } ys[][2] = {
        [SHORTCUT_LAYOUT_UPDATE_EXAMPLE_1] = {
            {
                .unmatched = XKB_KEY_z,
                .ctrl_alt = XKB_KEY_y,
                .super = XKB_KEY_y,
                .some = XKB_KEY_y,
            },
            {
                .unmatched = XKB_KEY_y,
                .ctrl_alt = XKB_KEY_y,
                .super = XKB_KEY_y,
                .some = XKB_KEY_y,
            }
        },
        [SHORTCUT_LAYOUT_UPDATE_EXAMPLE_2] = {
            {
                .unmatched = XKB_KEY_y,
                .ctrl_alt = XKB_KEY_y,
                .super = XKB_KEY_y,
                .some = XKB_KEY_y,
            },
            {
                .unmatched = XKB_KEY_z,
                .ctrl_alt = XKB_KEY_y,
                .super = XKB_KEY_y,
                .some = XKB_KEY_y,
            }
        },
        [SHORTCUT_LAYOUT_UPDATE_EXAMPLE_3] = {
            {
                .unmatched = XKB_KEY_f,
                .ctrl_alt = XKB_KEY_y,
                .super = XKB_KEY_f,
                .some = XKB_KEY_y,
            },
            {
                .unmatched = XKB_KEY_z,
                .ctrl_alt = XKB_KEY_z,
                .super = U(0x92c),
                .some = U(0x92c),
            }
        },
    };

    for (int example = SHORTCUT_LAYOUT_UPDATE_EXAMPLE_1;
         example < SHORTCUT_LAYOUT_UPDATE_EXAMPLE_COUNT;
         example++) {
        if (example == SHORTCUT_LAYOUT_UPDATE_EXAMPLE_1) {
            keymap_ = test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V2,
                                         "evdev", "pc104", "de,us", NULL,
                                         "grp:menu_toggle");
            assert(keymap_);

            builder = xkb_machine_builder_new(keymap_, NULL, NULL);
            assert(builder);

            /* Use another variable `keymap` for the snippet */
//! [shortcut_layout_update_example_1]
            struct xkb_keymap *keymap = xkb_machine_builder_get_keymap(builder);

            const xkb_mod_mask_t ctrl = xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_CTRL);
            const xkb_mod_mask_t alt = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_ALT);
            const xkb_mod_mask_t super = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_SUPER);
            const xkb_mod_mask_t mods = ctrl | alt | super;

            const struct xkb_machine_builder_shortcut_layout_update update = {
                .size = sizeof(update),
                .source = 0,
                .target = 1,
                .mods_affect = mods,
                .mods = mods,
            };
            const enum xkb_error_code error =
                xkb_machine_builder_update_shortcut_layout(builder, &update);
            if (error != XKB_SUCCESS) {
                // handle error
                assert(!"error");
            }
//! [shortcut_layout_update_example_1]
        }

        if (example == SHORTCUT_LAYOUT_UPDATE_EXAMPLE_2) {
            keymap_ = test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V2,
                                         "evdev", "pc104", "us,de", NULL,
                                         "grp:menu_toggle");
            assert(keymap_);

            builder = xkb_machine_builder_new(keymap_, NULL, NULL);
            assert(builder);

            /* Use another variable `keymap` for the snippet */
//! [shortcut_layout_update_example_2]
            struct xkb_keymap *keymap = xkb_machine_builder_get_keymap(builder);

            const xkb_mod_mask_t ctrl = xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_CTRL);
            const xkb_mod_mask_t alt = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_ALT);
            const xkb_mod_mask_t super = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_SUPER);
            const xkb_mod_mask_t mods = ctrl | alt | super;

            const xkb_layout_index_t num_layouts = xkb_keymap_num_layouts(keymap);
            for (xkb_layout_index_t source = num_layouts; source-- > 1;) {
                const struct xkb_machine_builder_shortcut_layout_update update = {
                    .size = sizeof(update),
                    .source = source,
                    .target = 0,
                    .mods_affect = mods,
                    .mods = mods,
                };
                const enum xkb_error_code error =
                    xkb_machine_builder_update_shortcut_layout(builder, &update);
                if (error != XKB_SUCCESS) {
                    // handle error
                    assert(!"error");
                }
            }
//! [shortcut_layout_update_example_2]
        }

        if (example == SHORTCUT_LAYOUT_UPDATE_EXAMPLE_3) {
            keymap_ = test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V2,
                                         "evdev", "pc104", "us,de,us,in", "dvorak,,,",
                                         "grp:menu_toggle");
            assert(keymap_);

            builder = xkb_machine_builder_new(keymap_, NULL, NULL);
            assert(builder);

            /* Use another variable `keymap` for the snippet */
//! [shortcut_layout_update_example_3]
            struct xkb_keymap *keymap = xkb_machine_builder_get_keymap(builder);

            const xkb_mod_mask_t ctrl = xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_CTRL);
            const xkb_mod_mask_t alt = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_ALT);
            const xkb_mod_mask_t super = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_SUPER);

            struct xkb_machine_builder_shortcut_layout_update update = {
                .size = sizeof(update),
                .source = 0,
                .target = 2,
                .mods_affect = ctrl | alt,
                .mods = ctrl | alt,
            };
            enum xkb_error_code error =
                xkb_machine_builder_update_shortcut_layout(builder, &update);
            if (error != XKB_SUCCESS) {
                // handle error
                assert(!"error");
            }
            update = (struct xkb_machine_builder_shortcut_layout_update) {
                .size = sizeof(update),
                .source = 1,
                .target = 3,
                .mods_affect = super,
                .mods = super,
            };
            error = xkb_machine_builder_update_shortcut_layout(builder, &update);
            if (error != XKB_SUCCESS) {
                // handle error
                assert(!"error");
            }
//! [shortcut_layout_update_example_3]
        }

        struct xkb_machine * sm = xkb_machine_new(builder, NULL);
        assert(sm);

        struct xkb_events * const events = xkb_events_new(context, NULL, NULL);
        assert(events);

        assert(test_key_seq2(
            keymap_, sm, events,
            /* Layout #0 */
            KEY_Y        , BOTH, ys[example][0].unmatched, NEXT,
            KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L         , NEXT,
            KEY_Y        , BOTH, xkb_keysym_to_upper(ys[example][0].unmatched), NEXT,
            KEY_LEFTSHIFT, UP  , XKB_KEY_Shift_L         , NEXT,
            KEY_Y        , BOTH, ys[example][0].unmatched, NEXT,
            KEY_LEFTCTRL , DOWN, XKB_KEY_Control_L       , NEXT,
            KEY_Y        , BOTH, ys[example][0].ctrl_alt , NEXT,
            KEY_LEFTCTRL , UP  , XKB_KEY_Control_L       , NEXT,
            KEY_Y        , BOTH, ys[example][0].unmatched, NEXT,
            KEY_LEFTALT  , DOWN, XKB_KEY_Alt_L           , NEXT,
            KEY_Y        , BOTH, ys[example][0].ctrl_alt , NEXT,
            KEY_LEFTALT  , UP  , XKB_KEY_Alt_L           , NEXT,
            KEY_Y        , BOTH, ys[example][0].unmatched, NEXT,
            KEY_LEFTMETA , DOWN, XKB_KEY_Super_L         , NEXT,
            KEY_Y        , BOTH, ys[example][0].super    , NEXT,
            KEY_LEFTMETA , UP  , XKB_KEY_Super_L         , NEXT,
            KEY_Y        , BOTH, ys[example][0].unmatched, NEXT,
            KEY_LEFTCTRL , DOWN, XKB_KEY_Control_L       , NEXT,
            KEY_Y        , BOTH, ys[example][0].ctrl_alt , NEXT,
            KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L         , NEXT,
            KEY_Y        , BOTH, xkb_keysym_to_upper(ys[example][0].ctrl_alt), NEXT,
            KEY_LEFTSHIFT, UP  , XKB_KEY_Shift_L         , NEXT,
            KEY_Y        , BOTH, ys[example][0].ctrl_alt , NEXT,
            KEY_LEFTALT  , DOWN, XKB_KEY_Alt_L           , NEXT,
            KEY_Y        , BOTH, ys[example][0].ctrl_alt , NEXT,
            KEY_LEFTALT  , UP  , XKB_KEY_Alt_L           , NEXT,
            KEY_Y        , BOTH, ys[example][0].ctrl_alt , NEXT,
            KEY_LEFTMETA , DOWN, XKB_KEY_Super_L         , NEXT,
            KEY_Y        , BOTH, ys[example][0].some     , NEXT,
            KEY_LEFTMETA , UP  , XKB_KEY_Super_L         , NEXT,
            KEY_Y        , BOTH, ys[example][0].ctrl_alt , NEXT,
            KEY_LEFTCTRL , UP  , XKB_KEY_Control_L       , NEXT,
            KEY_Y        , BOTH, ys[example][0].unmatched, NEXT,

            KEY_COMPOSE , BOTH, XKB_KEY_ISO_Next_Group, NEXT,

            /* Layout #1 */
            KEY_Y        , BOTH, ys[example][1].unmatched, NEXT,
            KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L         , NEXT,
            KEY_Y        , BOTH, xkb_keysym_to_upper(ys[example][1].unmatched), NEXT,
            KEY_LEFTSHIFT, UP  , XKB_KEY_Shift_L         , NEXT,
            KEY_Y        , BOTH, ys[example][1].unmatched, NEXT,
            KEY_LEFTCTRL , DOWN, XKB_KEY_Control_L       , NEXT,
            KEY_Y        , BOTH, ys[example][1].ctrl_alt , NEXT,
            KEY_LEFTCTRL , UP  , XKB_KEY_Control_L       , NEXT,
            KEY_Y        , BOTH, ys[example][1].unmatched, NEXT,
            KEY_LEFTALT  , DOWN, XKB_KEY_Alt_L           , NEXT,
            KEY_Y        , BOTH, ys[example][1].ctrl_alt , NEXT,
            KEY_LEFTALT  , UP  , XKB_KEY_Alt_L           , NEXT,
            KEY_Y        , BOTH, ys[example][1].unmatched, NEXT,
            KEY_LEFTMETA , DOWN, XKB_KEY_Super_L         , NEXT,
            KEY_Y        , BOTH, ys[example][1].super    , NEXT,
            KEY_LEFTMETA , UP  , XKB_KEY_Super_L         , NEXT,
            KEY_Y        , BOTH, ys[example][1].unmatched, NEXT,
            KEY_LEFTCTRL , DOWN, XKB_KEY_Control_L       , NEXT,
            KEY_Y        , BOTH, ys[example][1].ctrl_alt , NEXT,
            KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L         , NEXT,
            KEY_Y        , BOTH, xkb_keysym_to_upper(ys[example][1].ctrl_alt), NEXT,
            KEY_LEFTSHIFT, UP  , XKB_KEY_Shift_L         , NEXT,
            KEY_Y        , BOTH, ys[example][1].ctrl_alt , NEXT,
            KEY_LEFTALT  , DOWN, XKB_KEY_Alt_L           , NEXT,
            KEY_Y        , BOTH, ys[example][1].ctrl_alt , NEXT,
            KEY_LEFTALT  , UP  , XKB_KEY_Alt_L           , NEXT,
            KEY_Y        , BOTH, ys[example][1].ctrl_alt , NEXT,
            KEY_LEFTMETA , DOWN, XKB_KEY_Super_L         , NEXT,
            KEY_Y        , BOTH, ys[example][1].some     , NEXT,
            KEY_LEFTMETA , UP  , XKB_KEY_Super_L         , NEXT,
            KEY_Y        , BOTH, ys[example][1].ctrl_alt , NEXT,
            KEY_LEFTCTRL , UP  , XKB_KEY_Control_L       , NEXT,
            KEY_Y        , BOTH, ys[example][1].unmatched, FINISH
        ));

        xkb_events_unref(events);
        xkb_machine_unref(sm);
        xkb_machine_builder_unref(builder);
        xkb_keymap_unref(keymap_);
    }
}

int
main(void)
{
    test_init();

    struct xkb_context *context = test_get_context(CONTEXT_NO_FLAG);
    assert(context);

    /* Make sure these are allowed. */
    xkb_context_unref(NULL);
    xkb_keymap_unref(NULL);
    xkb_state_unref(NULL);
    xkb_machine_unref(NULL);
    xkb_machine_builder_unref(NULL);
    xkb_events_unref(NULL);

    test_machine_builder(context);
    test_initial_derived_values(context);

    enum xkb_error_code error;
    static const struct xkb_events_config config = {
        .size = sizeof(config),
        .flags = UINT32_MAX
    };
    assert(!xkb_events_new(context, &config, NULL));
    assert(!xkb_events_new(context, &config, &error) &&
           error == XKB_ERROR_UNSUPPORTED_EVENTS_FLAGS);
    struct xkb_events *events = xkb_events_new(context, NULL, &error);
    assert(events && error == XKB_SUCCESS);
    xkb_events_unref(events);

    test_state_update(context);
    test_group_wrap(context);
    test_sticky_keys(context);
    test_redirect_key(context);
    test_mouse_keys(context);
    test_server_actions(context);
    test_overlays(context);
    test_modifiers_tweak(context);
    test_shortcuts_tweak(context);
    test_xkb_machine_builder_mods_remap_update(context);
    test_machine_builder_shortcut_layout_update(context);

    xkb_context_unref(context);
    return EXIT_SUCCESS;
}
