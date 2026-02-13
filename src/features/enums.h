/*
 * NOTE: This file has been generated automatically by “scripts/export-enums.py”.
 *       Do not edit manually!
 */

/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "config.h"

#include <assert.h>
#include <limits.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-compose.h"
#include "xkbcommon/xkbcommon-features.h"

#ifndef INT_WIDTH
enum {
    INT_WIDTH = sizeof(int) * CHAR_BIT
};
#endif

/*
 * Enumerations whose values can be ORed
 */

static_assert(XKB_KEYMAP_FORMAT_TEXT_V1 >= 0 &&
              XKB_KEYMAP_FORMAT_TEXT_V1 < INT_WIDTH, "");
static_assert(XKB_KEYMAP_FORMAT_TEXT_V2 >= 0 &&
              XKB_KEYMAP_FORMAT_TEXT_V2 < INT_WIDTH, "");
static_assert(XKB_EVENT_TYPE_KEY_DOWN >= 0 &&
              XKB_EVENT_TYPE_KEY_DOWN < INT_WIDTH, "");
static_assert(XKB_EVENT_TYPE_KEY_UP >= 0 &&
              XKB_EVENT_TYPE_KEY_UP < INT_WIDTH, "");
static_assert(XKB_EVENT_TYPE_COMPONENTS_CHANGE >= 0 &&
              XKB_EVENT_TYPE_COMPONENTS_CHANGE < INT_WIDTH, "");
static_assert(XKB_KEY_UP >= 0 &&
              XKB_KEY_UP < INT_WIDTH, "");
static_assert(XKB_KEY_DOWN >= 0 &&
              XKB_KEY_DOWN < INT_WIDTH, "");
static_assert(XKB_CONSUMED_MODE_XKB >= 0 &&
              XKB_CONSUMED_MODE_XKB < INT_WIDTH, "");
static_assert(XKB_CONSUMED_MODE_GTK >= 0 &&
              XKB_CONSUMED_MODE_GTK < INT_WIDTH, "");
static_assert(XKB_COMPOSE_FORMAT_TEXT_V1 >= 0 &&
              XKB_COMPOSE_FORMAT_TEXT_V1 < INT_WIDTH, "");
static_assert(XKB_COMPOSE_NOTHING >= 0 &&
              XKB_COMPOSE_NOTHING < INT_WIDTH, "");
static_assert(XKB_COMPOSE_COMPOSING >= 0 &&
              XKB_COMPOSE_COMPOSING < INT_WIDTH, "");
static_assert(XKB_COMPOSE_COMPOSED >= 0 &&
              XKB_COMPOSE_COMPOSED < INT_WIDTH, "");
static_assert(XKB_COMPOSE_CANCELLED >= 0 &&
              XKB_COMPOSE_CANCELLED < INT_WIDTH, "");
static_assert(XKB_COMPOSE_FEED_IGNORED >= 0 &&
              XKB_COMPOSE_FEED_IGNORED < INT_WIDTH, "");
static_assert(XKB_COMPOSE_FEED_ACCEPTED >= 0 &&
              XKB_COMPOSE_FEED_ACCEPTED < INT_WIDTH, "");

enum xkb_enumerations_values {
    XKB_RMLVO_BUILDER_FLAGS_VALUES
        = XKB_RMLVO_BUILDER_NO_FLAGS
    ,
    XKB_KEYSYM_FLAGS_VALUES
        = XKB_KEYSYM_NO_FLAGS
        | XKB_KEYSYM_CASE_INSENSITIVE
    ,
    XKB_CONTEXT_FLAGS_VALUES
        = XKB_CONTEXT_NO_FLAGS
        | XKB_CONTEXT_NO_DEFAULT_INCLUDES
        | XKB_CONTEXT_NO_ENVIRONMENT_NAMES
        | XKB_CONTEXT_NO_SECURE_GETENV
    ,
    XKB_KEYMAP_COMPILE_FLAGS_VALUES
        = XKB_KEYMAP_COMPILE_NO_FLAGS
    ,
    XKB_KEYMAP_FORMAT_VALUES
        = (1u << XKB_KEYMAP_FORMAT_TEXT_V1)
        | (1u << XKB_KEYMAP_FORMAT_TEXT_V2)
    ,
    XKB_KEYMAP_SERIALIZE_FLAGS_VALUES
        = XKB_KEYMAP_SERIALIZE_NO_FLAGS
        | XKB_KEYMAP_SERIALIZE_PRETTY
        | XKB_KEYMAP_SERIALIZE_KEEP_UNUSED
    ,
    XKB_KEYMAP_KEY_ITERATOR_FLAGS_VALUES
        = XKB_KEYMAP_KEY_ITERATOR_NO_FLAGS
        | XKB_KEYMAP_KEY_ITERATOR_DESCENDING_ORDER
        | XKB_KEYMAP_KEY_ITERATOR_SKIP_UNBOUND
    ,
    XKB_STATE_ACCESSIBILITY_FLAGS_VALUES
        = XKB_STATE_A11Y_NO_FLAGS
        | XKB_STATE_A11Y_LATCH_TO_LOCK
        | XKB_STATE_A11Y_LATCH_SIMULTANEOUS_KEYS
    ,
    XKB_EVENT_TYPE_VALUES
        = (1u << XKB_EVENT_TYPE_KEY_DOWN)
        | (1u << XKB_EVENT_TYPE_KEY_UP)
        | (1u << XKB_EVENT_TYPE_COMPONENTS_CHANGE)
    ,
    XKB_STATE_COMPONENT_VALUES
        = XKB_STATE_MODS_DEPRESSED
        | XKB_STATE_MODS_LATCHED
        | XKB_STATE_MODS_LOCKED
        | XKB_STATE_MODS_EFFECTIVE
        | XKB_STATE_LAYOUT_DEPRESSED
        | XKB_STATE_LAYOUT_LATCHED
        | XKB_STATE_LAYOUT_LOCKED
        | XKB_STATE_LAYOUT_EFFECTIVE
        | XKB_STATE_LEDS
        | XKB_STATE_CONTROLS
    ,
    XKB_KEYBOARD_CONTROLS_VALUES
        = XKB_KEYBOARD_CONTROL_NONE
        | XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS
    ,
    XKB_KEY_DIRECTION_VALUES
        = (1u << XKB_KEY_UP)
        | (1u << XKB_KEY_DOWN)
    ,
    XKB_STATE_MATCH_VALUES
        = XKB_STATE_MATCH_ANY
        | XKB_STATE_MATCH_ALL
        | XKB_STATE_MATCH_NON_EXCLUSIVE
    ,
    XKB_CONSUMED_MODE_VALUES
        = (1u << XKB_CONSUMED_MODE_XKB)
        | (1u << XKB_CONSUMED_MODE_GTK)
    ,
    XKB_COMPOSE_COMPILE_FLAGS_VALUES
        = XKB_COMPOSE_COMPILE_NO_FLAGS
    ,
    XKB_COMPOSE_FORMAT_VALUES
        = (1u << XKB_COMPOSE_FORMAT_TEXT_V1)
    ,
    XKB_COMPOSE_STATE_FLAGS_VALUES
        = XKB_COMPOSE_STATE_NO_FLAGS
    ,
    XKB_COMPOSE_STATUS_VALUES
        = (1u << XKB_COMPOSE_NOTHING)
        | (1u << XKB_COMPOSE_COMPOSING)
        | (1u << XKB_COMPOSE_COMPOSED)
        | (1u << XKB_COMPOSE_CANCELLED)
    ,
    XKB_COMPOSE_FEED_RESULT_VALUES
        = (1u << XKB_COMPOSE_FEED_IGNORED)
        | (1u << XKB_COMPOSE_FEED_ACCEPTED)
    ,
};

/*
 * Explicit values of enumerations
 */

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_rmlvo_builder_flags_values[] = {
    XKB_RMLVO_BUILDER_NO_FLAGS,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_keysym_flags_values[] = {
    XKB_KEYSYM_NO_FLAGS,
    XKB_KEYSYM_CASE_INSENSITIVE,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_context_flags_values[] = {
    XKB_CONTEXT_NO_FLAGS,
    XKB_CONTEXT_NO_DEFAULT_INCLUDES,
    XKB_CONTEXT_NO_ENVIRONMENT_NAMES,
    XKB_CONTEXT_NO_SECURE_GETENV,
};
#endif

static const int xkb_log_level_values[] = {
    XKB_LOG_LEVEL_CRITICAL,
    XKB_LOG_LEVEL_ERROR,
    XKB_LOG_LEVEL_WARNING,
    XKB_LOG_LEVEL_INFO,
    XKB_LOG_LEVEL_DEBUG,
};

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_keymap_compile_flags_values[] = {
    XKB_KEYMAP_COMPILE_NO_FLAGS,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_keymap_format_values[] = {
    XKB_KEYMAP_FORMAT_TEXT_V1,
    XKB_KEYMAP_FORMAT_TEXT_V2,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_keymap_serialize_flags_values[] = {
    XKB_KEYMAP_SERIALIZE_NO_FLAGS,
    XKB_KEYMAP_SERIALIZE_PRETTY,
    XKB_KEYMAP_SERIALIZE_KEEP_UNUSED,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_keymap_key_iterator_flags_values[] = {
    XKB_KEYMAP_KEY_ITERATOR_NO_FLAGS,
    XKB_KEYMAP_KEY_ITERATOR_DESCENDING_ORDER,
    XKB_KEYMAP_KEY_ITERATOR_SKIP_UNBOUND,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_state_accessibility_flags_values[] = {
    XKB_STATE_A11Y_NO_FLAGS,
    XKB_STATE_A11Y_LATCH_TO_LOCK,
    XKB_STATE_A11Y_LATCH_SIMULTANEOUS_KEYS,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_event_type_values[] = {
    XKB_EVENT_TYPE_KEY_DOWN,
    XKB_EVENT_TYPE_KEY_UP,
    XKB_EVENT_TYPE_COMPONENTS_CHANGE,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
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
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_keyboard_controls_values[] = {
    XKB_KEYBOARD_CONTROL_NONE,
    XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_key_direction_values[] = {
    XKB_KEY_UP,
    XKB_KEY_DOWN,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_state_match_values[] = {
    XKB_STATE_MATCH_ANY,
    XKB_STATE_MATCH_ALL,
    XKB_STATE_MATCH_NON_EXCLUSIVE,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_consumed_mode_values[] = {
    XKB_CONSUMED_MODE_XKB,
    XKB_CONSUMED_MODE_GTK,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_compose_compile_flags_values[] = {
    XKB_COMPOSE_COMPILE_NO_FLAGS,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_compose_format_values[] = {
    XKB_COMPOSE_FORMAT_TEXT_V1,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_compose_state_flags_values[] = {
    XKB_COMPOSE_STATE_NO_FLAGS,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_compose_status_values[] = {
    XKB_COMPOSE_NOTHING,
    XKB_COMPOSE_COMPOSING,
    XKB_COMPOSE_COMPOSED,
    XKB_COMPOSE_CANCELLED,
};
#endif

#ifdef ENABLE_PRIVATE_APIS
static const int xkb_compose_feed_result_values[] = {
    XKB_COMPOSE_FEED_IGNORED,
    XKB_COMPOSE_FEED_ACCEPTED,
};
#endif

static const int xkb_feature_values[] = {
    XKB_FEATURE_ENUM_FEATURE,
    XKB_FEATURE_ENUM_CONTEXT_FLAGS,
    XKB_FEATURE_ENUM_LOG_LEVEL,
    XKB_FEATURE_ENUM_KEYMAP_FORMAT,
    XKB_FEATURE_ENUM_KEYMAP_COMPILE_FLAGS,
    XKB_FEATURE_ENUM_RMLVO_BUILDER_FLAGS,
    XKB_FEATURE_ENUM_KEYMAP_SERIALIZE_FLAGS,
    XKB_FEATURE_ENUM_KEYMAP_KEY_ITERATOR_FLAGS,
    XKB_FEATURE_ENUM_KEYSYM_FLAGS,
    XKB_FEATURE_ENUM_STATE_COMPONENT,
    XKB_FEATURE_ENUM_KEYBOARD_CONTROLS,
    XKB_FEATURE_ENUM_STATE_ACCESSIBILITY_FLAGS,
    XKB_FEATURE_ENUM_EVENT_TYPE,
    XKB_FEATURE_ENUM_KEY_DIRECTION,
    XKB_FEATURE_ENUM_STATE_MATCH,
    XKB_FEATURE_ENUM_CONSUMED_MODE,
    XKB_FEATURE_ENUM_COMPOSE_FORMAT,
    XKB_FEATURE_ENUM_COMPOSE_COMPILE_FLAGS,
    XKB_FEATURE_ENUM_COMPOSE_STATE_FLAGS,
    XKB_FEATURE_ENUM_COMPOSE_STATUS,
    XKB_FEATURE_ENUM_COMPOSE_FEED_RESULT,
};
