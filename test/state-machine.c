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

#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon-names.h"
#include "xkbcommon/xkbcommon.h"

#include "context.h"
#include "evdev-scancodes.h"
#include "src/keysym.h"
#include "src/keymap.h"
#include "src/state-priv.h"
#include "state.h"
#include "test.h"
#include "utils.h"

static void
test_state_machine_options(struct xkb_context *ctx)
{
    struct xkb_state_machine_options *options =
        xkb_state_machine_options_new(ctx);
    assert(options);

    /* Invalid flags */
    assert(xkb_state_machine_options_update_a11y_flags(options, -1000, 0) == 1);
    assert(xkb_state_machine_options_update_a11y_flags(options, 1000, 0) == 1);

    /* Valid flags */
    static_assert(XKB_STATE_A11Y_NO_FLAGS == 0, "default flags");
    assert(xkb_state_machine_options_update_a11y_flags(
            options, XKB_STATE_A11Y_NO_FLAGS, 1000) == 0);

    struct xkb_keymap *keymap =
        xkb_keymap_new_from_names(ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    assert(keymap);

    struct xkb_state_machine *sm = xkb_state_machine_new(keymap, options);
    assert(sm);

    xkb_state_machine_unref(sm);
    xkb_keymap_unref(keymap);
    xkb_state_machine_options_destroy(options);
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

    struct xkb_state_machine * const sm = xkb_state_machine_new(keymap, NULL);
    assert(sm);
    struct xkb_state * const state = xkb_state_machine_get_state(sm);
    assert(xkb_state_led_name_is_active(state, XKB_LED_NAME_SCROLL));

    xkb_state_machine_unref(sm);
    xkb_keymap_unref(keymap);
}

static enum xkb_state_component
update_key(struct xkb_state_machine *sm,
           struct xkb_event_iterator *events,
           struct xkb_state *state,
           bool use_machine, xkb_keycode_t key,
           enum xkb_key_direction direction)
{
    if (use_machine) {
        assert(xkb_state_machine_update_key(sm, events, key, direction) == 0);
        const struct xkb_event *event;
        enum xkb_state_component changed = 0;
        while ((event = xkb_event_iterator_next(events))) {
            changed |= xkb_state_update_from_event(state, event);
        }
        return changed;
    } else {
        return xkb_state_update_key(state, key, direction);
    }
}

static enum xkb_state_component
update_controls(struct xkb_state_machine *sm,
                struct xkb_event_iterator *events,
                struct xkb_state *state,
                bool use_machine,
                enum xkb_keyboard_controls affect,
                enum xkb_keyboard_controls controls)
{
    if (use_machine) {
        assert(xkb_state_machine_update_controls(sm, events, affect, controls)
               == 0);
        const struct xkb_event *event;
        enum xkb_state_component changed = 0;
        while ((event = xkb_event_iterator_next(events))) {
            changed |= xkb_state_update_from_event(state, event);
        }
        return changed;
    } else {
        return xkb_state_update_controls(state, affect, controls);
    }
}

static void
test_sticky_keys(struct xkb_context *ctx)
{
    struct xkb_keymap * const keymap = test_compile_rmlvo(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
        "evdev", "pc104", "ca,cz,de", NULL, "controls,grp:lwin_switch,grp:menu_toggle"
    );
    assert(keymap);

    struct xkb_state_machine *sm = xkb_state_machine_new(keymap, NULL);
    assert(sm);
    struct xkb_event_iterator *events =
        xkb_event_iterator_new(ctx, XKB_EVENT_ITERATOR_NO_FLAGS);
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
    enum xkb_keyboard_controls controls;
    enum xkb_state_component changed;

    controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
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
            controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
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
            controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
            assert(controls == XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);
            break;
        }
        case STICKY_KEY_LEGACY_API:
            changed = xkb_state_update_controls(state,
                                                XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS,
                                                XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);
            assert(changed == XKB_STATE_CONTROLS);
            controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
            assert(controls == XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS);
            break;
        }
        controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
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
            changed = xkb_state_update_controls(state,
                                                XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS,
                                                0);
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
        controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
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

        controls = xkb_state_serialize_controls(state, XKB_STATE_CONTROLS);
        assert(controls == 0);
    }

    xkb_state_unref(state);
    xkb_event_iterator_destroy(events);
    xkb_state_machine_unref(sm);

    /*
     * Test latch-to-lock
     */

    struct xkb_state_machine_options * const sm_options =
        xkb_state_machine_options_new(ctx);
    assert(sm_options);
    assert(xkb_state_machine_options_update_a11y_flags(
                sm_options,
                XKB_STATE_A11Y_LATCH_TO_LOCK,
                XKB_STATE_A11Y_LATCH_TO_LOCK) == 0);
    sm = xkb_state_machine_new(keymap, sm_options);
    assert(sm);
    xkb_state_machine_options_destroy(sm_options);
    events = xkb_event_iterator_new(ctx, XKB_EVENT_ITERATOR_NO_FLAGS);
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
    xkb_event_iterator_destroy(events);
    xkb_state_machine_unref(sm);

    xkb_keymap_unref(keymap);
}

static void
test_redirect_key(struct xkb_context *ctx)
{
    struct xkb_keymap * const keymap = test_compile_file(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "keymaps/redirect-key-1.xkb"
    );
    assert(keymap);

    struct xkb_state_machine *sm = xkb_state_machine_new(keymap, NULL);
    assert(sm);

    static const xkb_mod_mask_t shift = UINT32_C(1) << XKB_MOD_INDEX_SHIFT;
    static const xkb_mod_mask_t ctrl = UINT32_C(1) << XKB_MOD_INDEX_CTRL;

    struct xkb_event_iterator *events =
        xkb_event_iterator_new(ctx, XKB_EVENT_ITERATOR_NO_FLAGS);
    assert(events);

    assert(test_key_seq2(
        keymap, sm, events,
        KEY_A, BOTH, XKB_KEY_a, NEXT,
        KEY_S, BOTH, XKB_KEY_a, NEXT,
        KEY_D, BOTH, XKB_KEY_s, FINISH
    ));

    xkb_state_machine_update_latched_locked(sm, events, 0, 0, false, 0,
                                            ctrl, ctrl, false, 0);

    const struct {
        xkb_keycode_t keycode;
        struct test_events {
            struct xkb_event events[3];
            unsigned int events_count;
        } down;
        struct test_events up;
    } tests[] = {
        {
            .keycode = EVDEV_OFFSET + KEY_A,
            .down = {
                .events = {
                    {
                        .type = XKB_EVENT_TYPE_KEY_DOWN,
                        .keycode = EVDEV_OFFSET + KEY_A
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .type = XKB_EVENT_TYPE_KEY_UP,
                        .keycode = EVDEV_OFFSET + KEY_A
                    }
                },
                .events_count = 1
            }
        },
        {
            .keycode = EVDEV_OFFSET + KEY_S,
            .down = {
                .events = {
                    {
                        .type = XKB_EVENT_TYPE_KEY_DOWN,
                        .keycode = EVDEV_OFFSET + KEY_A
                    }
                },
                .events_count = 1
            },
            .up = {
                .events = {
                    {
                        .type = XKB_EVENT_TYPE_KEY_UP,
                        .keycode = EVDEV_OFFSET + KEY_A
                    }
                },
                .events_count = 1
            }
        },
        {
            .keycode = EVDEV_OFFSET + KEY_D,
            .down = {
                .events = {
                    {
                        .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
                        .type = XKB_EVENT_TYPE_KEY_DOWN,
                        .keycode = EVDEV_OFFSET + KEY_S
                    },
                    {
                        .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
                        .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
                        .type = XKB_EVENT_TYPE_KEY_UP,
                        .keycode = EVDEV_OFFSET + KEY_S
                    },
                    {
                        .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
        assert(0 == xkb_state_machine_update_key(sm, events, tests[t].keycode,
                                                 XKB_KEY_DOWN));
        assert(check_events(events, tests[t].down.events,
                            tests[t].down.events_count));
        assert(0 == xkb_state_machine_update_key(sm, events, tests[t].keycode,
                                                 XKB_KEY_UP));
        assert(check_events(events, tests[t].up.events,
                            tests[t].up.events_count));
    }

    xkb_event_iterator_destroy(events);
    xkb_state_machine_unref(sm);
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

    const xkb_mod_mask_t ctrl = UINT32_C(1) << XKB_MOD_INDEX_CTRL;
    const xkb_mod_mask_t alt = _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_ALT);
    const xkb_mod_mask_t level3 = _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_LEVEL3);
    const xkb_mod_mask_t level5 = _xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_LEVEL5);

    struct xkb_state_machine_options * const options =
        xkb_state_machine_options_new(context);
    assert(options);

    assert(xkb_state_machine_options_shortcuts_update_mods(options, ctrl, ctrl)
           == 0);
    assert(xkb_state_machine_options_shortcuts_set_mapping(options, 1, 2) == 0);
    assert(xkb_state_machine_options_shortcuts_set_mapping(options, 3, 0) == 0);

    struct xkb_state_machine * sm = xkb_state_machine_new(keymap, options);
    assert(sm);

    struct xkb_event_iterator * const events =
        xkb_event_iterator_new(context, XKB_EVENT_ITERATOR_NO_FLAGS);
    assert(events);

    /*
     * xkb_state_machine_update_key
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
     * xkb_state_machine_update_key
     */
    assert(xkb_state_machine_update_key(sm, events, KEY_Q + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_Q + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_Q + EVDEV_OFFSET,
                                        XKB_KEY_UP) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_KEY_UP,
            .keycode = KEY_Q + EVDEV_OFFSET
        }
    );

    assert(xkb_state_machine_update_key(sm, events, KEY_RIGHTCTRL + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_RIGHTCTRL + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_RIGHTCTRL + EVDEV_OFFSET,
                                        XKB_KEY_UP) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_UP,
            .keycode = KEY_RIGHTCTRL + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_102ND + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_102ND + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_Q + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_Q + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_Q + EVDEV_OFFSET,
                                        XKB_KEY_UP) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_KEY_UP,
            .keycode = KEY_Q + EVDEV_OFFSET
        }
    );

    assert(xkb_state_machine_update_key(sm, events, KEY_102ND + EVDEV_OFFSET,
                                        XKB_KEY_UP) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_KEY_UP,
            .keycode = KEY_102ND + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
     * xkb_state_machine_update_latched_locked
     */

    /* Layout 1 locked, Ctrl latched */
    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   ctrl, ctrl, false, 0,
                                                   0, 0, true, 1) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   ctrl, 0, false, 0,
                                                   ctrl, ctrl, false, 0) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    /* Layout 1 latched, layout 2 locked, Ctrl locked */
    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   0, 0, true, 1,
                                                   0, 0, true, 2) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   0, 0, false, 0,
                                                   ctrl, 0, false, 0) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   ctrl, ctrl, false, 0,
                                                   0, 0, false, 0) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
     * xkb_state_machine_update_controls
     */

    const enum xkb_keyboard_controls controls =
        XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS;

    /* Disable already disabled sticky keys: no change */
    assert(xkb_state_machine_update_controls(sm, events, controls, 0) == 0);
    check_events_(events, { .type = XKB_EVENT_TYPE_NONE });

    /* Enable disabled sticky keys: no change */
    assert(xkb_state_machine_update_controls(sm, events, controls, controls) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
    assert(xkb_state_machine_update_controls(sm, events, controls, controls) == 0);
    check_events_(events, { .type = XKB_EVENT_TYPE_NONE });

    /* Disable sticky keys: clear latches & locks */
    assert(xkb_state_machine_update_controls(sm, events, controls, 0) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_controls(sm, events, controls, 0) == 0);

    /*
     * Check RedirectKey()
     */

    /* Layout 4 locked, Ctrl locked */
    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   0, 0, false, 0,
                                                   ctrl, ctrl, true, 3) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_C + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_COPY + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    xkb_state_machine_unref(sm);

    /*
     * Use modifiers tweak in addition to the shortcuts tweak
     */

    assert(!xkb_state_machine_options_mods_set_mapping(options,
                                                       ctrl | alt, level3));

    sm = xkb_state_machine_new(keymap, options);
    assert(sm);

    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   0, 0, false, 0,
                                                   ctrl, ctrl, true, 3) == 0);

    assert(xkb_state_machine_update_key(sm, events, KEY_Q + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_Q + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_C + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_COPY + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   0, 0, false, 0,
                                                   alt, alt, false, 0) == 0);

    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_Q + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_Q + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   0, 0, false, 0,
                                                   0, 0, true, 0) == 0);

    assert(xkb_state_machine_update_key(sm, events, KEY_Q + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_Q + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_C + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_COPY + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    xkb_state_machine_unref(sm);
    xkb_event_iterator_destroy(events);
    xkb_state_machine_options_destroy(options);
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

    struct xkb_state_machine_options * const options =
        xkb_state_machine_options_new(context);
    assert(options);

    assert(!xkb_state_machine_options_mods_set_mapping(options, 0, 0));
    assert(xkb_state_machine_options_mods_set_mapping(options, 0, level3) == -1);
    assert(!xkb_state_machine_options_mods_set_mapping(options, scroll, alt));
    assert(!xkb_state_machine_options_mods_set_mapping(options, super, level3));
    assert(!xkb_state_machine_options_mods_set_mapping(options, alt, level5));
    assert(!xkb_state_machine_options_mods_set_mapping(options, ctrl | alt, level3));

    assert(!xkb_state_machine_options_mods_set_mapping(options, ctrl, shift));
    assert(!xkb_state_machine_options_mods_set_mapping(options, ctrl, 0));

    struct xkb_state_machine * const sm = xkb_state_machine_new(keymap, options);
    assert(sm);
    xkb_state_machine_options_destroy(options);

    struct xkb_event_iterator * const events =
        xkb_event_iterator_new(context, XKB_EVENT_ITERATOR_NO_FLAGS);
    assert(events);

#define U(cp) (XKB_KEYSYM_UNICODE_OFFSET + (cp))

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
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L     , NEXT,
        KEY_LEFTMETA, DOWN, XKB_KEY_Super_L       , NEXT,
        KEY_Y       , BOTH, XKB_KEY_dead_doubleacute, NEXT,
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

#undef U

    const xkb_led_mask_t num_led =
        UINT32_C(1) << xkb_keymap_led_get_index(keymap, XKB_LED_NAME_NUM);
    const xkb_led_mask_t scroll_led =
        UINT32_C(1) << xkb_keymap_led_get_index(keymap, XKB_LED_NAME_SCROLL);
    const xkb_led_mask_t group2_led =
        UINT32_C(1) << xkb_keymap_led_get_index(keymap, "Group 2");

    assert(xkb_state_machine_update_key(sm, events, KEY_LEFTALT + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_LEFTALT + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_Y + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_Y + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_Y + EVDEV_OFFSET,
                                        XKB_KEY_UP) == 0);

    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_UP,
            .keycode = KEY_Y + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   0, 0, false, 0,
                                                   ctrl | num, ctrl | num,
                                                   false, 0) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_Y + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_Y + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_Y + EVDEV_OFFSET,
                                        XKB_KEY_UP) == 0);

    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_UP,
            .keycode = KEY_Y + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
    assert(xkb_state_machine_update_key(sm, events, KEY_BACKSPACE + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_BACKSPACE + EVDEV_OFFSET
        }
    );

    assert(xkb_state_machine_update_key(sm, events, KEY_LEFTALT + EVDEV_OFFSET,
                                        XKB_KEY_UP) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_UP,
            .keycode = KEY_LEFTALT + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   0, 0, false, 0,
                                                   ctrl | scroll, scroll,
                                                   false, 0) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
    assert(xkb_state_machine_update_key(sm, events, KEY_CAPSLOCK + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_CAPSLOCK + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_latched_locked(sm, events,
                                                   0, 0, false, 0,
                                                   ctrl | alt | scroll, ctrl | alt,
                                                   false, 0) == 0);

    /* Key redirect */
    assert(xkb_state_machine_update_key(sm, events, KEY_C + EVDEV_OFFSET,
                                        XKB_KEY_DOWN) == 0);
    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_DOWN,
            .keycode = KEY_COPY + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    assert(xkb_state_machine_update_key(sm, events, KEY_C + EVDEV_OFFSET,
                                        XKB_KEY_UP) == 0);

    check_events_(
        events,
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_KEY_UP,
            .keycode = KEY_COPY + EVDEV_OFFSET
        },
        {
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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
            .type = XKB_EVENT_TYPE_COMPONENTS_CHANGE,
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

    xkb_event_iterator_destroy(events);
    xkb_state_machine_unref(sm);
    xkb_keymap_unref(keymap);
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
    xkb_state_machine_unref(NULL);
    xkb_state_machine_options_destroy(NULL);
    xkb_event_iterator_destroy(NULL);

    test_state_machine_options(context);
    test_initial_derived_values(context);

    assert(!xkb_event_iterator_new(context, -1));

    test_sticky_keys(context);
    test_redirect_key(context);
    test_modifiers_tweak(context);
    test_shortcuts_tweak(context);

    xkb_context_unref(context);
    return EXIT_SUCCESS;
}
