/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifndef _XKBCOMMON_NAMES_H
#define _XKBCOMMON_NAMES_H

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
#define XKB_MOD_NAME_ALT        "Mod1" /* Alt */
#define XKB_MOD_NAME_LOGO       "Mod4" /* Super */
#define XKB_MOD_NAME_NUM        "Mod2" /* NumLock */

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
