/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "config.h"
#include "test-config.h"

#include <assert.h>

#include "xkbcommon/xkbcommon.h"
#include "src/state-priv.h"
#include "test.h"

static const enum xkb_keymap_format keymap_formats[] = {
    XKB_KEYMAP_FORMAT_TEXT_V1,
    XKB_KEYMAP_FORMAT_TEXT_V2
};

#define XKB_EVENT_TYPE_NONE 0

/* Offset between evdev keycodes (where KEY_ESCAPE is 1), and the evdev XKB
 * keycode set (where ESC is 9). */
#define EVDEV_OFFSET 8

static inline xkb_mod_mask_t
_xkb_keymap_mod_get_mask(struct xkb_keymap *keymap, const char *name)
{
    xkb_mod_mask_t mask = xkb_keymap_mod_get_mask(keymap, name);
    assert_printf(mask, "Modifier %s is not mapped or defined\n", name);
    return mask;
}

static inline xkb_mod_index_t
_xkb_keymap_mod_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_mod_index_t mod = xkb_keymap_mod_get_index(keymap, name);
    assert(mod != XKB_MOD_INVALID);
    return mod;
}

static inline xkb_led_index_t
_xkb_keymap_led_get_index(struct xkb_keymap *keymap, const char *name)
{
    xkb_led_index_t led = xkb_keymap_led_get_index(keymap, name);
    assert(led != XKB_LED_INVALID);
    return led;
}

static inline void
print_modifiers_serialization(struct xkb_state *state)
{
    xkb_mod_mask_t base = xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED);
    xkb_mod_mask_t latched = xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED);
    xkb_mod_mask_t locked = xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED);
    xkb_mod_mask_t effective = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    fprintf(stderr,
            "\tMods: Base: 0x%"PRIx32", Latched: 0x%"PRIx32", "
            "Locked: 0x%"PRIx32", Effective: 0x%"PRIx32"\n",
            base, latched, locked, effective);
}

static inline void
print_layout_serialization(struct xkb_state *state)
{
    xkb_mod_mask_t base = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_DEPRESSED);
    xkb_mod_mask_t latched = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED);
    xkb_mod_mask_t locked = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED);
    xkb_mod_mask_t effective = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE);
    fprintf(stderr,
            "\tLayout: Base: 0x%"PRIx32", Latched: 0x%"PRIx32", "
            "Locked: 0x%"PRIx32", Effective: 0x%"PRIx32"\n",
            base, latched, locked, effective);
}

static inline void
print_state(struct xkb_state *state)
{
    struct xkb_keymap *keymap;
    xkb_layout_index_t group;
    xkb_mod_index_t mod;
    xkb_led_index_t led;

    group = xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE);
    mod = xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
    /* led = xkb_state_serialize_leds(state, XKB_STATE_LEDS); */
    if (!group && !mod /* && !led */) {
        fprintf(stderr, "\tno state\n");
        return;
    }

    keymap = xkb_state_get_keymap(state);

    for (group = 0; group < xkb_keymap_num_layouts(keymap); group++) {
        if (xkb_state_layout_index_is_active(state, group,
                                             XKB_STATE_LAYOUT_EFFECTIVE |
                                             XKB_STATE_LAYOUT_DEPRESSED |
                                             XKB_STATE_LAYOUT_LATCHED |
                                             XKB_STATE_LAYOUT_LOCKED) <= 0)
            continue;
        fprintf(stderr, "\tgroup %s (%"PRIu32"): %s%s%s%s\n",
                xkb_keymap_layout_get_name(keymap, group),
                group,
                xkb_state_layout_index_is_active(state, group, XKB_STATE_LAYOUT_EFFECTIVE) > 0 ?
                    "effective " : "",
                xkb_state_layout_index_is_active(state, group, XKB_STATE_LAYOUT_DEPRESSED) > 0 ?
                    "depressed " : "",
                xkb_state_layout_index_is_active(state, group, XKB_STATE_LAYOUT_LATCHED) > 0 ?
                    "latched " : "",
                xkb_state_layout_index_is_active(state, group, XKB_STATE_LAYOUT_LOCKED) > 0 ?
                    "locked " : "");
    }

    for (mod = 0; mod < xkb_keymap_num_mods(keymap); mod++) {
        if (xkb_state_mod_index_is_active(state, mod,
                                          XKB_STATE_MODS_EFFECTIVE |
                                          XKB_STATE_MODS_DEPRESSED |
                                          XKB_STATE_MODS_LATCHED |
                                          XKB_STATE_MODS_LOCKED) <= 0)
            continue;
        fprintf(stderr, "\tmod %s (%"PRIu32"): %s%s%s%s\n",
                xkb_keymap_mod_get_name(keymap, mod),
                mod,
                xkb_state_mod_index_is_active(state, mod, XKB_STATE_MODS_EFFECTIVE) > 0 ?
                    "effective " : "",
                xkb_state_mod_index_is_active(state, mod, XKB_STATE_MODS_DEPRESSED) > 0 ?
                    "depressed " : "",
                xkb_state_mod_index_is_active(state, mod, XKB_STATE_MODS_LATCHED) > 0 ?
                    "latched " : "",
                xkb_state_mod_index_is_active(state, mod, XKB_STATE_MODS_LOCKED) > 0 ?
                    "locked " : "");
    }

    for (led = 0; led < xkb_keymap_num_leds(keymap); led++) {
        if (xkb_state_led_index_is_active(state, led) <= 0)
            continue;
        fprintf(stderr, "\tled %s (%"PRIu32"): active\n",
                xkb_keymap_led_get_name(keymap, led),
                led);
    }
}

static inline bool
xkb_event_eq(const struct xkb_event *event1, const struct xkb_event *event2)
{
    if (event1->type != event2->type)
        return false;
    switch (event1->type) {
    case XKB_EVENT_TYPE_KEY_DOWN:
    case XKB_EVENT_TYPE_KEY_REPEATED:
    case XKB_EVENT_TYPE_KEY_UP:
        return event1->keycode == event2->keycode;
    case XKB_EVENT_TYPE_COMPONENTS_CHANGE:
        return memcmp(&event1->components, &event2->components,
                      sizeof(event1->components)) == 0;
    default:
        {} /* Label followed by declaration requires C23 */
        static_assert(XKB_EVENT_TYPE_COMPONENTS_CHANGE == 4 &&
                      XKB_EVENT_TYPE_COMPONENTS_CHANGE ==
                      (enum xkb_event_type) _LAST_XKB_EVENT_TYPE,
                      "Missing state event type");
        return false;
    }
}

static inline void
print_event(const char *prefix, const struct xkb_event *event)
{
    fprintf(stderr, "%s", prefix);
    switch (event->type) {
    case XKB_EVENT_TYPE_KEY_DOWN:
    case XKB_EVENT_TYPE_KEY_REPEATED:
    case XKB_EVENT_TYPE_KEY_UP:
        fprintf(stderr, "type: key %s; keycode: %"PRIu32"\n",
                (event->type == XKB_EVENT_TYPE_KEY_UP)
                    ? "up"
                    : event->type == XKB_EVENT_TYPE_KEY_REPEATED
                        ? "repeat"
                        : "down",
                event->keycode);
        break;
    case XKB_EVENT_TYPE_COMPONENTS_CHANGE:
        fprintf(stderr, "type: components; changed: 0x%x\n"
                "\tgroup: %"PRId32" %"PRId32" %"PRId32" %"PRIu32"\n"
                "\tmods: 0x%08"PRIx32" 0x%08"PRIx32" 0x%08"PRIx32" %08"PRIx32"\n"
                "\tleds: 0x%08"PRIx32"\n"
                "\tcontrols: 0x%08"PRIx32"\n",
                event->components.changed,
                event->components.components.base_group,
                event->components.components.latched_group,
                event->components.components.locked_group,
                event->components.components.group,
                event->components.components.base_mods,
                event->components.components.latched_mods,
                event->components.components.locked_mods,
                event->components.components.mods,
                event->components.components.leds,
                event->components.components.controls);
        break;
    default:
        {} /* Label followed by declaration requires C23 */
        static_assert(XKB_EVENT_TYPE_COMPONENTS_CHANGE == 4 &&
                      XKB_EVENT_TYPE_COMPONENTS_CHANGE ==
                      (enum xkb_event_type) _LAST_XKB_EVENT_TYPE,
                      "Missing state event type");
    }
}

static inline bool
check_events(struct xkb_event_iterator *iter,
             const struct xkb_event *events, size_t count)
{
    const struct xkb_event *got = NULL;
    size_t got_count = 0;
    bool ok = true;
    if (count == 1 && events[0].type == XKB_EVENT_TYPE_NONE)
        count = 0;
    while ((got = xkb_event_iterator_next(iter))) {
        if (++got_count > count) {
            fprintf(stderr, "%s() error at event #%zu:\n", __func__, got_count);
            print_event("Unexpected event:\n", got);
            break;
        }
        const struct xkb_event *expected = &events[got_count - 1];
        if (!xkb_event_eq(got, expected)) {
            fprintf(stderr, "%s() error at event #%zu:\n", __func__, got_count);
            print_event("Expected: ", expected);
            print_event("Got: ", got);
            ok = false;
            break;
        }
    }
    if (got_count != count) {
        fprintf(stderr, "%s() count error: expected %zu, got: %zu\n",
                __func__, count, got_count);
        ok = false;
    }
    return ok;
}

#define check_events_(got, ...) do {                            \
    const struct xkb_event expected[] = { __VA_ARGS__ };        \
    assert(check_events((got), expected, ARRAY_SIZE(expected)));\
} while (0)
