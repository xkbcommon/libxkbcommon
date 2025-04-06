/*
 * Copyright © 2012 Intel Corporation
 * SPDX-License-Identifier: MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifndef _XKBCOMMON_NAMES_H
#define _XKBCOMMON_NAMES_H

#define XKB_INTERNAL_STRINGIFY(x) #x
#if defined(__GNUC__) || defined(__clang__)
    #define XKB_INTERNAL_PRAGMA(x) _Pragma(#x)
    #define XKB_INTERNAL_DEPRECATION_WARNING(x) \
        XKB_INTERNAL_PRAGMA(GCC warning #x)
    #define XKB_INTERNAL_DEPRECATED_MOD_NAME(msg, str) \
        (XKB_INTERNAL_DEPRECATION_WARNING(msg) str)
#else
    #define XKB_INTERNAL_DEPRECATED_MOD_NAME(msg, str) str
#endif

/**
 * @file
 * @brief Predefined names for common modifiers and LEDs.
 */

/* *Real* modifiers names are hardcoded in libxkbcommon */

#define XKB_MOD_NAME_SHIFT      "Shift"
#define XKB_MOD_NAME_CAPS       "Lock"
#define XKB_MOD_NAME_CTRL       "Control"
#define XKB_MOD_NAME_MOD1       "Mod1"
#define XKB_MOD_NAME_MOD2       "Mod2"
#define XKB_MOD_NAME_MOD3       "Mod3"
#define XKB_MOD_NAME_MOD4       "Mod4"
#define XKB_MOD_NAME_MOD5       "Mod5"

/* Usual virtual modifiers mappings to real modifiers */

/**
 * Usual *real* modifier for the *virtual* modifier `Alt`.
 *
 * @deprecated
 * Use `XKB_VMOD_NAME_ALT` instead.
 */
#define XKB_MOD_NAME_ALT                                        \
    XKB_INTERNAL_DEPRECATED_MOD_NAME(                           \
        XKB_INTERNAL_STRINGIFY(XKB_MOD_NAME_ALT) is deprecated: \
        use XKB_INTERNAL_STRINGIFY(XKB_VMOD_NAME_ALT) instead,  \
        "Mod1"                                                  \
    )
/**
 * Usual *real* modifier for the *virtual* modifier `Super`.
 *
 * @deprecated
 * Use `XKB_VMOD_NAME_SUPER` instead.
 */
#define XKB_MOD_NAME_LOGO                                       \
    XKB_INTERNAL_DEPRECATED_MOD_NAME(                           \
        XKB_INTERNAL_STRINGIFY(XKB_MOD_NAME_LOGO) is deprecated:\
        use XKB_INTERNAL_STRINGIFY(XKB_VMOD_NAME_SUPER) instead,\
        "Mod4"                                                  \
    )
/**
 * Usual *real* modifier for the *virtual* modifier `NumLock`.
 *
 * @deprecated
 * Use `XKB_VMOD_NAME_NUM` instead.
 */
#define XKB_MOD_NAME_NUM                                        \
    XKB_INTERNAL_DEPRECATED_MOD_NAME(                           \
        XKB_INTERNAL_STRINGIFY(XKB_MOD_NAME_NUM) is deprecated: \
        use XKB_INTERNAL_STRINGIFY(XKB_VMOD_NAME_NUM) instead,  \
        "Mod2"                                                  \
    )

/* Common *virtual* modifiers, encoded in xkeyboard-config in the compat and
 * symbols files. They have been stable since the beginning of the project and
 * are unlikely to ever change. */
#define XKB_VMOD_NAME_ALT       "Alt"
#define XKB_VMOD_NAME_HYPER     "Hyper"
#define XKB_VMOD_NAME_LEVEL3    "LevelThree"
#define XKB_VMOD_NAME_LEVEL5    "LevelFive"
#define XKB_VMOD_NAME_META      "Meta"
#define XKB_VMOD_NAME_NUM       "NumLock"
#define XKB_VMOD_NAME_SCROLL    "ScrollLock"
#define XKB_VMOD_NAME_SUPER     "Super"

/* LEDs names are encoded in xkeyboard-config, in the keycodes and compat files.
 * They have been stable since the beginning of the project and are unlikely to
 * ever change. */
#define XKB_LED_NAME_NUM        "Num Lock"
#define XKB_LED_NAME_CAPS       "Caps Lock"
#define XKB_LED_NAME_SCROLL     "Scroll Lock"
#define XKB_LED_NAME_COMPOSE    "Compose"
#define XKB_LED_NAME_KANA       "Kana"

#endif
