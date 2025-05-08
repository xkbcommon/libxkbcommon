/*
 * Copyright © 2012 Intel Corporation
 * SPDX-License-Identifier: MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifndef _XKBCOMMON_NAMES_H
#define _XKBCOMMON_NAMES_H

/**
 * @file
 * @brief Predefined names for common modifiers and LEDs.
 */

/**
 * @defgroup modifier-names Predefined names for common modifiers and LEDs
 *
 * @{
 */

/**
 * @defgroup real-modifier-names Real modifiers names
 *
 * [*Real* modifiers][real modifier] names are hardcoded in libxkbcommon.
 *
 * [real modifier]: @ref real-modifier-def
 *
 * @{
 */
#define XKB_MOD_NAME_SHIFT      "Shift"
#define XKB_MOD_NAME_CAPS       "Lock"
#define XKB_MOD_NAME_CTRL       "Control"
#define XKB_MOD_NAME_MOD1       "Mod1"
#define XKB_MOD_NAME_MOD2       "Mod2"
#define XKB_MOD_NAME_MOD3       "Mod3"
#define XKB_MOD_NAME_MOD4       "Mod4"
#define XKB_MOD_NAME_MOD5       "Mod5"
/** @} */

/**
 * @defgroup virtual-modifier-names Virtual modifiers names
 *
 * Common [*virtual* modifiers][virtual modifiers], encoded in [xkeyboard-config]
 * in the [compat] and [symbols] files. They have been stable since the beginning
 * of the project and are unlikely to ever change.
 *
 * [virtual modifiers]: @ref virtual-modifier-def
 * [xkeyboard-config]: https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config
 * [compat]: @ref the-xkb_compat-section
 * [symbols]: @ref the-xkb_symbols-section
 *
 * @{
 */
/** @since 1.8.0 */
#define XKB_VMOD_NAME_ALT       "Alt"
/** @since 1.8.0 */
#define XKB_VMOD_NAME_HYPER     "Hyper"
/** @since 1.8.0 */
#define XKB_VMOD_NAME_LEVEL3    "LevelThree"
/** @since 1.8.0 */
#define XKB_VMOD_NAME_LEVEL5    "LevelFive"
/** @since 1.8.0 */
#define XKB_VMOD_NAME_META      "Meta"
/** @since 1.8.0 */
#define XKB_VMOD_NAME_NUM       "NumLock"
/** @since 1.8.0 */
#define XKB_VMOD_NAME_SCROLL    "ScrollLock"
/** @since 1.8.0 */
#define XKB_VMOD_NAME_SUPER     "Super"
/** @} */

/**
 * @defgroup legacy-virtual-modifier-names Legacy virtual modifier names
 *
 * Usual [*virtual* modifiers][virtual modifiers] mappings to
 * [*real* modifiers][real modifiers].
 *
 * @deprecated Use @ref virtual-modifier-names instead
 *
 * [virtual modifiers]: @ref virtual-modifier-def
 * [real modifiers]: @ref real-modifier-def
 *
 * @{
 */
/**
 * Usual [*real* modifier][real modifier] for the
 * [*virtual* modifier][virtual modifier] `Alt`.
 *
 * @deprecated Use `::XKB_VMOD_NAME_ALT` instead.
 * @since 1.10: deprecated
 *
 * [virtual modifier]: @ref virtual-modifier-def
 * [real modifier]: @ref real-modifier-def
 */
#define XKB_MOD_NAME_ALT        "Mod1"
/**
 * Usual [*real* modifier][real modifier] for the
 * [*virtual* modifier][virtual modifier] `Super`.
 *
 * @deprecated Use `::XKB_VMOD_NAME_SUPER` instead.
 * @since 1.10: deprecated
 *
 * [virtual modifier]: @ref virtual-modifier-def
 * [real modifier]: @ref real-modifier-def
 */
#define XKB_MOD_NAME_LOGO       "Mod4"
/**
 * Usual [*real* modifier][real modifier] for the
 * [*virtual* modifier][virtual modifier] `NumLock`.
 *
 * @deprecated Use `::XKB_VMOD_NAME_NUM` instead.
 * @since 1.10: deprecated
 *
 * [virtual modifier]: @ref virtual-modifier-def
 * [real modifier]: @ref real-modifier-def
 */
#define XKB_MOD_NAME_NUM        "Mod2"
/** @} */

/**
 * @defgroup led-names LEDs names
 *
 * LEDs names are encoded in [xkeyboard-config], in the [keycodes] and [compat]
 * files. They have been stable since the beginning of the project and are
 * unlikely to ever change.
 *
 * [xkeyboard-config]: https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config
 * [keycodes]: @ref the-xkb_keycodes-section
 * [compat]: @ref the-xkb_compat-section
 *
 * @{
 */
#define XKB_LED_NAME_NUM        "Num Lock"
#define XKB_LED_NAME_CAPS       "Caps Lock"
#define XKB_LED_NAME_SCROLL     "Scroll Lock"
/** @since 1.8.0 */
#define XKB_LED_NAME_COMPOSE    "Compose"
#define XKB_LED_NAME_KANA       "Kana"
/** @} */

/** @} */

#endif
