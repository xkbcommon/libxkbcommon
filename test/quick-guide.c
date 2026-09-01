/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon-errors.h"
#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon.h"

#include "context.h"
#include "evdev-scancodes.h"
#include "test.h"

#undef EVDEV_OFFSET

enum wl_keyboard_key_state {
	/**
	 * key is not pressed
	 */
	WL_KEYBOARD_KEY_STATE_RELEASED = 0,
	/**
	 * key is pressed
	 */
	WL_KEYBOARD_KEY_STATE_PRESSED = 1,
	/**
	 * key was repeated
	 * @since 10
	 */
	WL_KEYBOARD_KEY_STATE_REPEATED = 2,
};

// NOLINTBEGIN(readability-duplicate-include)
//! [wayland-server-example]
#include <xkbcommon/xkbcommon.h>

enum {
    EVDEV_OFFSET = 8,
};

struct my_keyboard {
    struct xkb_context *ctx;
    struct xkb_keymap *keymap;
    struct xkb_machine *machine;
    struct xkb_events *events;
};

static int
new_keyboard(struct my_keyboard *keyboard, const struct xkb_rule_names *names)
{
    assert(keyboard->ctx);

    /*
     * Initialize the keymap
     */

    struct xkb_keymap *keymap =
        xkb_keymap_new_from_names2(keyboard->ctx, names,
                                   XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        /* Handle error */
        // ...
        exit(EXIT_FAILURE);
    }

    /*
     * Initialize the state machine
     */

    enum xkb_error_code error;
    struct xkb_machine_builder *machine_builder =
        xkb_machine_builder_new(keymap, XKB_MACHINE_BUILDER_NO_FLAGS, &error);
    if (!machine_builder) {
        assert(error != XKB_SUCCESS);
        switch (error) {
        // ...
        default:
            exit(EXIT_FAILURE);
        }
    }
    struct xkb_machine *machine = xkb_machine_new(machine_builder, &error);
    xkb_machine_builder_unref(machine_builder);
    if (!machine) {
        assert(error != XKB_SUCCESS);
        switch (error) {
        // ...
        default:
            exit(EXIT_FAILURE);
        }
    }

    struct xkb_events *events = xkb_events_new(keyboard->ctx, NULL, &error);
    if (!events) {
        assert(error != XKB_SUCCESS);
        switch (error) {
        // ...
        default:
            exit(EXIT_FAILURE);
        }
    }

    char *keymap_string =
        xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);

    if (!keymap_string) {
        /* Handle error */
        // ...
        exit(EXIT_FAILURE);
    }

    /* Send keymap to the clients */
    // ...

    free(keymap_string);

    /* Save the objects for further use */
    keyboard->keymap = keymap;
    keyboard->machine = machine;
    keyboard->events = events;

    return EXIT_SUCCESS;
}

static int
destroy_keyboard(struct my_keyboard *keyboard)
{
    xkb_events_unref(keyboard->events);
    xkb_machine_unref(keyboard->machine);
    xkb_keymap_unref(keyboard->keymap);
    xkb_context_unref(keyboard->ctx);
    return EXIT_SUCCESS;
}

static int
handle_key(struct my_keyboard *keyboard, uint32_t key, uint32_t state)
{
    /*
     * Update the server state with the key event
     */

    const xkb_keycode_t keycode = key + EVDEV_OFFSET;
    enum xkb_key_direction direction = WL_KEYBOARD_KEY_STATE_RELEASED
        ? XKB_KEY_UP
        : WL_KEYBOARD_KEY_STATE_REPEATED
            ? XKB_KEY_REPEATED
            : XKB_KEY_DOWN;
    enum xkb_error_code error =
        xkb_machine_process_key(keyboard->machine, keycode, direction,
                                keyboard->events);
    if (error != XKB_SUCCESS) {
        /* Handle error */
        // ...
        exit(EXIT_FAILURE);
    }

    /*
     * Process the generated XKB events
     */

    const struct xkb_event *event;
    while ((event = xkb_events_next(keyboard->events)) != NULL) {
        const enum xkb_event_type event_type =
            xkb_event_get_type(event);
        switch (event_type) {
            case XKB_EVENT_TYPE_KEY: {
                xkb_keycode_t kc = XKB_KEYCODE_INVALID;
                error = xkb_event_get_keycode(event, &kc, &direction);
                if (error != XKB_SUCCESS) {
                    /* Handle error */
                    // ...
                    exit(EXIT_FAILURE);
                }
                /* Send key event to clients */
                // ...
                assert(kc != XKB_KEYCODE_INVALID);
                break;
            }
            case XKB_EVENT_TYPE_STATE_COMPONENTS: {
                struct xkb_event_components components = {
                    .size = sizeof(components)
                };
                error = xkb_event_serialize_components(event, &components);
                if (error != XKB_SUCCESS) {
                    /* Handle error */
                    // ...
                    exit(EXIT_FAILURE);
                }
                if (components.changed) {
                    /* Send component changes to clients */
                    // ...
                }
                break;
            }
            case XKB_EVENT_TYPE_POINTER_MOTION: {
                struct xkb_event_pointer_motion motion = {
                    .size = sizeof(motion)
                };
                error = xkb_event_get_pointer_motion(event, &motion);
                if (error != XKB_SUCCESS) {
                    /* Handle error */
                    // ...
                    exit(EXIT_FAILURE);
                }
                /* Move cursor */
                // ...
                break;
            }
            case XKB_EVENT_TYPE_POINTER_BUTTON: {
                struct xkb_event_pointer_button button = {
                    .size = sizeof(button)
                };
                error = xkb_event_get_pointer_button(event, &button);
                assert(error == XKB_SUCCESS);
                if (error != XKB_SUCCESS) {
                    /* Handle error */
                    // ...
                    exit(EXIT_FAILURE);
                }
                /* Operate button */
                // ...
                break;
            }
            default:
                /* Report unhandled event */
                // ...
                exit(EXIT_FAILURE);
        }
    }
    return EXIT_SUCCESS;
}
//! [wayland-server-example]
// NOLINTEND(readability-duplicate-include)

static void
test_server(void)
{
    struct my_keyboard keyboard = {
        .ctx = test_get_context(CONTEXT_NO_FLAG),
    };
    assert(keyboard.ctx);

    /* Example RMLVO for Canadian Dvorak. */
    static const struct xkb_rule_names names = {
        .rules = NULL,
        .model = "pc105",
        .layout = "ca",
        .variant = "fr-dvorak",
        .options = "terminate:ctrl_alt_bksp"
    };

    assert(new_keyboard(&keyboard, &names) == EXIT_SUCCESS);
    assert(handle_key(&keyboard, KEY_A, WL_KEYBOARD_KEY_STATE_PRESSED) ==
           EXIT_SUCCESS);
    assert(handle_key(&keyboard, KEY_A, WL_KEYBOARD_KEY_STATE_RELEASED) ==
           EXIT_SUCCESS);

    destroy_keyboard(&keyboard);
}

static void
handle_keysym(xkb_keysym_t keysym)
{
    assert(keysym != XKB_KEY_NoSymbol);
}

static void
test_evdev(struct xkb_keymap *keymap)
{
 //! [quick-guide-evdev-client-state-example]
    struct xkb_state *state = xkb_state_new(keymap);
    if (!state) {
        /* Handle error */
        // ...
        exit(EXIT_FAILURE);
    }
//! [quick-guide-evdev-client-state-example]

    const xkb_keycode_t keycode = KEY_SPACE + EVDEV_OFFSET;
    int32_t direction = 2;

//! [quick-guide-evdev-client-key-update-example]
    enum xkb_state_component changed;
    enum {
        KEY_STATE_RELEASE = 0,
        KEY_STATE_PRESS = 1,
        KEY_STATE_REPEAT = 2,
    };
    switch (direction) {
        case KEY_STATE_RELEASE:
            changed = xkb_state_update_key(state, keycode, XKB_KEY_UP);
            break;
        case KEY_STATE_PRESS:
            changed = xkb_state_update_key(state, keycode, XKB_KEY_DOWN);
            break;
        case KEY_STATE_REPEAT:
            changed = xkb_state_update_key(state, keycode, XKB_KEY_REPEATED);
            break;
        default:
            /* Handle error */
            // ...
            exit(EXIT_FAILURE);
    }
//! [quick-guide-evdev-client-key-update-example]

    assert(changed == 0);

//! [quick-guide-evdev-client-key-repeated-example]
    if (direction == KEY_STATE_REPEAT &&
        !xkb_keymap_key_repeats(keymap, keycode)) {
        /* Discard event */
        // ...
    }
//! [quick-guide-evdev-client-key-repeated-example]

    xkb_state_unref(state);
}

static int
kbd_keymap(struct xkb_context *ctx,
           struct xkb_keymap **keymap,
           enum xkb_keymap_format format,
           const char *keymap_buffer,
           size_t keymap_buffer_size)
{
//! [quick-guide-wayland-client-compilation-example]
    /*
     * From the wl_keyboard::keymap event:
     * - format,
     * - keymap_buffer,
     * - keymap_buffer_size
     */
    *keymap = xkb_keymap_new_from_buffer(ctx,
                                        keymap_buffer,
                                        keymap_buffer_size,
                                        format,
                                        XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!*keymap) {
        /* Handle error */
        // ...
        exit(EXIT_FAILURE);
    }
//! [quick-guide-wayland-client-compilation-example]

    return EXIT_SUCCESS;
}

static void
test_client(void)
{
// NOLINTBEGIN(readability-duplicate-include)
//! [quick-guide-context-example]
    #include <xkbcommon/xkbcommon.h>
    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx) {
        /* Handle error */
        // ...
        exit(EXIT_FAILURE);
    }
//! [quick-guide-context-example]
// NOLINTEND(readability-duplicate-include)

    /* We need the test context */
    xkb_context_unref(ctx);
    ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

//! [quick-guide-evdev-keymap-example]
    /* Example RMLVO for Canadian Dvorak. */
    const struct xkb_rule_names names = {
        .rules = NULL,
        .model = "pc105",
        .layout = "ca",
        .variant = "fr-dvorak",
        .options = "terminate:ctrl_alt_bksp"
    };

    struct xkb_keymap *keymap = xkb_keymap_new_from_names2(
        ctx, &names, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS
    );
    if (!keymap) {
        /* Handle error */
        // ...
        exit(EXIT_FAILURE);
    }
//! [quick-guide-evdev-keymap-example]

    const enum xkb_keymap_format format = XKB_KEYMAP_FORMAT_TEXT_V1;
    static const struct xkb_keymap_serialize_config config = {
        .size = sizeof(config),
        .format = XKB_KEYMAP_FORMAT_TEXT_V1
    };
    struct xkb_keymap_serialize_result result = {
        .size = sizeof(result),
    };
    enum xkb_error_code ret = xkb_keymap_serialize(keymap, &config, &result);
    assert(ret == XKB_SUCCESS);

    xkb_keymap_unref(keymap);

    assert(kbd_keymap(ctx, &keymap, format, result.serialized, result.length - 1) ==
        EXIT_SUCCESS);
    assert(keymap);

    free(result.serialized);

//! [quick-guide-wayland-client-state-example]
    enum xkb_error_code error;
    struct xkb_state *state;

    state = xkb_state_new_with_mode(keymap, XKB_STATE_MODE_CLIENT, &error);
    if (!state) {
        /* Handle error */
        assert(error != XKB_SUCCESS);
        switch (error) {
        // ...
        default:
            exit(EXIT_FAILURE);
        }
    }
//! [quick-guide-wayland-client-state-example]

    struct key_event {
        xkb_keycode_t keycode;
        xkb_mod_mask_t depressed_mods;
        xkb_mod_mask_t latched_mods;
        xkb_mod_mask_t locked_mods;
        xkb_layout_index_t depressed_layout;
        xkb_layout_index_t latched_layout;
        xkb_layout_index_t locked_layout;
    };
    struct key_event *event = &(struct key_event) {
        .keycode = KEY_SPACE + EVDEV_OFFSET
    };

//! [quick-guide-client-keysym-example]
    /* Key event was delivered in `*event` */
    xkb_keycode_t keycode = event->keycode;
    xkb_keysym_t keysym;
    keysym = xkb_state_key_get_one_sym(state, keycode);
//! [quick-guide-client-keysym-example]

    assert(keysym == XKB_KEY_space);

//! [quick-guide-client-keysym-name-example]
    switch(keysym) {
        case XKB_KEY_space:
            /* Got a space */
            // ...
            break;
        // ...
        default:
            /* Default handle */
            // ...
            assert(!"unhandled keysym");
    }

    char keysym_name[64];
    xkb_keysym_get_name(keysym, keysym_name, sizeof(keysym_name));
//! [quick-guide-client-keysym-name-example]

    assert_streq("keysym name", "space", keysym_name);

//! [quick-guide-client-keysyms-example]
    const xkb_keysym_t *keysyms;
    const int num_keysyms = xkb_state_key_get_syms(state, keycode, &keysyms);
    for (int k = 0; k < num_keysyms; k++) {
        /* Handle keysym */
        // ...
        handle_keysym(keysyms[k]);
    }
//! [quick-guide-client-keysyms-example]

//! [quick-guide-client-keysym-utf8-example]
    /* First find the needed size; return value is the same as snprintf(3). */
    int size = xkb_state_key_get_utf8(state, keycode, NULL, 0) + 1;
    if (size <= 1) {
        /* Handle keysym with no UTF-8 representation */
        assert(!"no UTF-8 representation");
    } else {
        char *utf8 = malloc((size_t)size);
        if (!utf8) {
            /* Handle error */
            // ...
            exit(EXIT_FAILURE);
        }
        xkb_state_key_get_utf8(state, keycode, utf8, size);
        assert(*utf8 != '\0');
        // ...
        free(utf8);
    }
//! [quick-guide-client-keysym-utf8-example]

//! [quick-guide-client-update-mask-example]
    enum xkb_state_component changed = xkb_state_update_mask(
        state,
        event->depressed_mods,
        event->latched_mods,
        event->locked_mods,
        event->depressed_layout,
        event->latched_layout,
        event->locked_layout
    );
//! [quick-guide-client-update-mask-example]

//! [quick-guide-client-mods-example]
    if (changed & XKB_STATE_MODS_EFFECTIVE) {
        if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CTRL,
                                        XKB_STATE_MODS_EFFECTIVE) > 0) {
            /* Handle active Control modifier */
            // ...
        }
    }
    if (changed & XKB_STATE_LEDS) {
        if (xkb_state_led_name_is_active(state, XKB_LED_NAME_NUM) > 0) {
            /* Handle active Num Lock LED */
            // ...
        }
    }
//! [quick-guide-client-mods-example]

    test_evdev(keymap);

//! [quick-guide-client-unref-example]
    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);
//! [quick-guide-client-unref-example]
}


int
main(void)
{
    test_init();

    test_client();
    test_server();

    return EXIT_SUCCESS;
}
