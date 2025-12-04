/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 *
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-compose.h"

static const int xkb_rmlvo_builder_flags_values[] = {
    XKB_RMLVO_BUILDER_NO_FLAGS,
};
static const int xkb_keysym_flags_values[] = {
    XKB_KEYSYM_NO_FLAGS,
    XKB_KEYSYM_CASE_INSENSITIVE,
};
static const int xkb_context_flags_values[] = {
    XKB_CONTEXT_NO_FLAGS,
    XKB_CONTEXT_NO_DEFAULT_INCLUDES,
    XKB_CONTEXT_NO_ENVIRONMENT_NAMES,
    XKB_CONTEXT_NO_SECURE_GETENV,
};
static const int xkb_log_level_values[] = {
    XKB_LOG_LEVEL_CRITICAL,
    XKB_LOG_LEVEL_ERROR,
    XKB_LOG_LEVEL_WARNING,
    XKB_LOG_LEVEL_INFO,
    XKB_LOG_LEVEL_DEBUG,
};
static const int xkb_keymap_compile_flags_values[] = {
    XKB_KEYMAP_COMPILE_NO_FLAGS,
};
static const int xkb_keymap_format_values[] = {
    XKB_KEYMAP_FORMAT_TEXT_V1,
    XKB_KEYMAP_FORMAT_TEXT_V2,
};
static const int xkb_keymap_serialize_flags_values[] = {
    XKB_KEYMAP_SERIALIZE_NO_FLAGS,
    XKB_KEYMAP_SERIALIZE_PRETTY,
    XKB_KEYMAP_SERIALIZE_KEEP_UNUSED,
};
static const int xkb_keymap_key_iterator_flags_values[] = {
    XKB_KEYMAP_KEY_ITERATOR_NO_FLAGS,
    XKB_KEYMAP_KEY_ITERATOR_DESCENDING_ORDER,
    XKB_KEYMAP_KEY_ITERATOR_SKIP_UNBOUND,
};
static const int xkb_state_accessibility_flags_values[] = {
    XKB_STATE_A11Y_NO_FLAGS,
    XKB_STATE_A11Y_LATCH_TO_LOCK,
    XKB_STATE_A11Y_LATCH_SIMULTANEOUS_KEYS,
};
static const int xkb_event_type_values[] = {
    XKB_EVENT_TYPE_KEY_DOWN,
    XKB_EVENT_TYPE_KEY_UP,
    XKB_EVENT_TYPE_COMPONENTS_CHANGE,
};
static const int xkb_state_component_values[] = {
    XKB_STATE_MODS_DEPRESSED,
    XKB_STATE_MODS_LATCHED,
    XKB_STATE_MODS_LOCKED,
    XKB_STATE_MODS_EFFECTIVE,
    XKB_STATE_LAYOUT_DEPRESSED,
    XKB_STATE_LAYOUT_LATCHED,
    XKB_STATE_LAYOUT_LOCKED,
    XKB_STATE_LAYOUT_EFFECTIVE,
    XKB_STATE_LEDS,
    XKB_STATE_CONTROLS,
};
static const int xkb_keyboard_controls_values[] = {
    XKB_KEYBOARD_CONTROL_NONE,
    XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS,
};
static const int xkb_key_direction_values[] = {
    XKB_KEY_UP,
    XKB_KEY_DOWN,
};
static const int xkb_state_match_values[] = {
    XKB_STATE_MATCH_ANY,
    XKB_STATE_MATCH_ALL,
    XKB_STATE_MATCH_NON_EXCLUSIVE,
};
static const int xkb_consumed_mode_values[] = {
    XKB_CONSUMED_MODE_XKB,
    XKB_CONSUMED_MODE_GTK,
};
static const int xkb_enumeration_values[] = {
    XKB_ENUM_ENUMERATION,
    XKB_ENUM_RMLVO_BUILDER_FLAGS,
    XKB_ENUM_KEYSYM_FLAGS,
    XKB_ENUM_CONTEXT_FLAGS,
    XKB_ENUM_LOG_LEVEL,
    XKB_ENUM_KEYMAP_COMPILE_FLAGS,
    XKB_ENUM_KEYMAP_FORMAT,
    XKB_ENUM_KEYMAP_SERIALIZE_FLAGS,
    XKB_ENUM_KEYMAP_KEY_ITERATOR_FLAGS,
    XKB_ENUM_STATE_ACCESSIBILITY_FLAGS,
    XKB_ENUM_EVENT_TYPE,
    XKB_ENUM_STATE_COMPONENT,
    XKB_ENUM_KEYBOARD_CONTROLS,
    XKB_ENUM_KEY_DIRECTION,
    XKB_ENUM_STATE_MATCH,
    XKB_ENUM_CONSUMED_MODE,
    XKB_ENUM_COMPOSE_COMPILE_FLAGS,
    XKB_ENUM_COMPOSE_FORMAT,
    XKB_ENUM_COMPOSE_STATE_FLAGS,
    XKB_ENUM_COMPOSE_STATUS,
    XKB_ENUM_COMPOSE_FEED_RESULT,
};
static const int xkb_compose_compile_flags_values[] = {
    XKB_COMPOSE_COMPILE_NO_FLAGS,
};
static const int xkb_compose_format_values[] = {
    XKB_COMPOSE_FORMAT_TEXT_V1,
};
static const int xkb_compose_state_flags_values[] = {
    XKB_COMPOSE_STATE_NO_FLAGS,
};
static const int xkb_compose_status_values[] = {
    XKB_COMPOSE_NOTHING,
    XKB_COMPOSE_COMPOSING,
    XKB_COMPOSE_COMPOSED,
    XKB_COMPOSE_CANCELLED,
};
static const int xkb_compose_feed_result_values[] = {
    XKB_COMPOSE_FEED_IGNORED,
    XKB_COMPOSE_FEED_ACCEPTED,
};
