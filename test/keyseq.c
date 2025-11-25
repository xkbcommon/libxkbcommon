/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon.h"

#include "evdev-scancodes.h"
#include "test.h"
#include "keymap.h"
#include "utils.h"

enum fake_keys {
    KEY_LVL3 = 84
};

static const enum xkb_keymap_format keymap_formats[] = {
    XKB_KEYMAP_FORMAT_TEXT_V1,
    XKB_KEYMAP_FORMAT_TEXT_V2,
};

static inline xkb_keysym_t
get_keysym(struct xkb_keymap *keymap, xkb_keysym_t v1, xkb_keysym_t v2)
{
    return (keymap->format == XKB_KEYMAP_FORMAT_TEXT_V1) ? v1 : v2;
}

static void
test_group_lock(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    /*
     * Group lock on press (all formats)
     * Implicit lockOnRelease=false (XKB spec)
     */
    for (unsigned int f = 0; f < ARRAY_SIZE(keymap_formats); f++) {
        keymap = test_compile_rules(ctx, keymap_formats[f],
                                    "evdev", "pc105",
                                    "us,de", "",
                                    "grp:alt_shift_toggle");
        assert(keymap);

#define test_group_lock_on_press(keymap) assert(                      \
    test_key_seq((keymap),                                            \
                  KEY_Y,         BOTH, XKB_KEY_y,              NEXT,  \
                  KEY_LEFTALT,   DOWN, XKB_KEY_Alt_L,          NEXT,  \
                  KEY_LEFTSHIFT, BOTH, XKB_KEY_ISO_Next_Group, NEXT,  \
                  /* Group change on press */                         \
                  KEY_Y,         BOTH, XKB_KEY_z,              NEXT,  \
                  KEY_LEFTSHIFT, DOWN, XKB_KEY_ISO_Next_Group, NEXT,  \
                  /* Group change on press */                         \
                  KEY_Y,         BOTH, XKB_KEY_y,              NEXT,  \
                  KEY_LEFTSHIFT, UP,   XKB_KEY_ISO_Next_Group, NEXT,  \
                  KEY_Y,         BOTH, XKB_KEY_y,              NEXT,  \
                  KEY_LEFTALT,   UP,   XKB_KEY_Alt_L,          FINISH)\
)

        test_group_lock_on_press(keymap);

        xkb_keymap_unref(keymap);
    }

    /*
     * Group lock on press for format V2
     * Explicit lockOnRelease=false (XKB spec)
     */
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                                "evdev", "pc105",
                                "us,de", "",
                                "grp:alt_shift_toggle,grp:lockOnPress");
    test_group_lock_on_press(keymap);
    xkb_keymap_unref(keymap);

#undef test_group_lock_on_press

    /*
     * Group lock on release for format V2
     * Explicit lockOnRelease=true (XKB extension)
     */
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                                "evdev", "pc105",
                                "us,de", "",
                                "grp:alt_shift_toggle,grp:lockOnRelease");

#define test_group_lock_on_release(keymap) assert(                   \
    test_key_seq((keymap),                                           \
                 KEY_Y,         BOTH, XKB_KEY_y,              NEXT,  \
                 KEY_LEFTALT,   DOWN, XKB_KEY_Alt_L,          NEXT,  \
                 KEY_LEFTSHIFT, BOTH, XKB_KEY_ISO_Next_Group, NEXT,  \
                 /* Group lock on release */                         \
                 KEY_Y,         BOTH, XKB_KEY_z,              NEXT,  \
                 KEY_LEFTSHIFT, DOWN, XKB_KEY_ISO_Next_Group, NEXT,  \
                 /* Key not released, no group change */             \
                 KEY_Y,         BOTH, XKB_KEY_z,              NEXT,  \
                 KEY_LEFTSHIFT, UP,   XKB_KEY_ISO_Next_Group, NEXT,  \
                 /* Group lock cancelled by intermediate key press */\
                 KEY_Y,         BOTH, XKB_KEY_z,              NEXT,  \
                 KEY_Y,         DOWN, XKB_KEY_z,              NEXT,  \
                 KEY_LEFTSHIFT, DOWN, XKB_KEY_ISO_Next_Group, NEXT,  \
                 /* Group lock not cancelled by intermediate key release */\
                 KEY_Y,         UP,   XKB_KEY_z,              NEXT,  \
                 KEY_LEFTSHIFT, UP,   XKB_KEY_ISO_Next_Group, NEXT,  \
                 /* Group lock on release */                         \
                 KEY_Y,         BOTH, XKB_KEY_y,              NEXT,  \
                 KEY_LEFTALT,   UP,   XKB_KEY_Alt_L,          FINISH)\
)

    test_group_lock_on_release(keymap);
    xkb_keymap_unref(keymap);

#undef test_group_lock_on_release
}

static void
test_group_latch(struct xkb_context *ctx)
{
    /* Absolute group, no lock */
    for (unsigned int f = 0; f < ARRAY_SIZE(keymap_formats); f++) {
        fprintf(stderr, "=== %s, format %u ===\n", __func__, keymap_formats[f]);

        struct xkb_keymap *keymap = test_compile_rules(
            ctx, keymap_formats[f], "evdev", "pc104",
            "us,il,ru,de", ",,phonetic,neo",
            "grp:menu_latch_group2,grp:sclk_toggle,grp:lctrl_rctrl_switch");
        assert(keymap);

    /* Set only */
#define test_set_only(keymap_)                                                 \
    assert(test_key_seq(keymap_,                                               \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_e,               NEXT,  \
                        KEY_COMPOSE,    DOWN,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     NEXT,  \
                        KEY_COMPOSE,    UP,    XKB_KEY_ISO_Group_Latch, NEXT,  \
                        /* Lock the second group */                            \
                        KEY_SCROLLLOCK, BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     NEXT,  \
                        KEY_COMPOSE,    DOWN,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        /* Event if the latch group is absolute, it sums with  \
                         * the locked group (see spec) */                      \
                        KEY_H,          BOTH,  XKB_KEY_Cyrillic_ha,     NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_Cyrillic_ie,     NEXT,  \
                        KEY_COMPOSE,    UP,    XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     FINISH))
    test_set_only(keymap);

    /* Latch only */
#define test_latch_only(keymap_)                                               \
    assert(test_key_seq(keymap_,                                               \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Empty level breaks latches */                       \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_YEN,        BOTH,  XKB_KEY_NoSymbol,        NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Unknown key does not break latches */               \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        UINT32_MAX,     BOTH,  XKB_KEY_NoSymbol,        NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Lock the second group */                            \
                        KEY_SCROLLLOCK, BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     NEXT,  \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        /* Event if the latch group is absolute, it sums with  \
                         * the locked group (see spec) */                      \
                        KEY_H,          BOTH,  XKB_KEY_Cyrillic_ha,     NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      FINISH))
    test_latch_only(keymap);

    /* Latch not broken by modifier */
#define test_latch_not_broken_by_modifier(keymap_)                           \
    assert(test_key_seq(keymap_,                                             \
                        KEY_H,        BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Sequential */                                     \
                        KEY_COMPOSE,  BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_LEFTALT,  BOTH,  XKB_KEY_Alt_L,           NEXT,  \
                        KEY_H,        BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_H,        BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Simultaneous */                                   \
                        KEY_COMPOSE,  DOWN,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_LEFTALT,  BOTH,  XKB_KEY_Alt_L,           NEXT,  \
                        KEY_COMPOSE,  UP,    XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,        BOTH,  get_keysym(keymap, XKB_KEY_h, XKB_KEY_hebrew_yod), NEXT,  \
                        /* Simultaneous */                                   \
                        KEY_LEFTALT,  DOWN,  XKB_KEY_Alt_L,           NEXT,  \
                        KEY_COMPOSE,  BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_LEFTALT,  UP,    XKB_KEY_Alt_L,           NEXT,  \
                        KEY_H,        BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_H,        BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Simultaneous */                                   \
                        KEY_LEFTALT,  DOWN,  XKB_KEY_Alt_L,           NEXT,  \
                        KEY_COMPOSE,  DOWN,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_LEFTALT,  UP,    XKB_KEY_Alt_L,           NEXT,  \
                        KEY_COMPOSE,  UP,    XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,        BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_H,        BOTH,  XKB_KEY_h,               FINISH))
    test_latch_not_broken_by_modifier(keymap);

    /* Simulatenous group actions */
#define test_simultaneous_group_latches(keymap_)                              \
    assert(test_key_seq(keymap_,                                              \
                        KEY_H,          BOTH, XKB_KEY_h,               NEXT,  \
                        /* Sequential */                                      \
                        KEY_COMPOSE,    BOTH, XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_SCROLLLOCK, BOTH, XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_Cyrillic_ha,     NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_LEFTCTRL,   BOTH, XKB_KEY_ISO_First_Group, NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_h,               NEXT,  \
                        /* Simultaneous */                                    \
                        KEY_COMPOSE,    DOWN, XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_SCROLLLOCK, BOTH, XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_COMPOSE,    UP,   XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH, get_keysym(keymap, XKB_KEY_hebrew_yod, XKB_KEY_Cyrillic_ha), NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_LEFTCTRL,   BOTH, XKB_KEY_ISO_First_Group, NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_h,               NEXT,  \
                        /* Simultaneous */                                    \
                        KEY_SCROLLLOCK, DOWN, XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_COMPOSE,    BOTH, XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_SCROLLLOCK, UP,   XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_Cyrillic_ha,     NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_LEFTCTRL,   BOTH, XKB_KEY_ISO_First_Group, NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_h,               NEXT,  \
                        /* Simultaneous */                                    \
                        KEY_SCROLLLOCK, DOWN, XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_COMPOSE,    DOWN, XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_SCROLLLOCK, UP,   XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_COMPOSE,    UP,   XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_Cyrillic_ha,     NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_LEFTCTRL,   BOTH, XKB_KEY_ISO_First_Group, NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_h,               FINISH))
    test_simultaneous_group_latches(keymap);

    /* No lock */
#define test_no_latch_to_lock(keymap_)                                         \
    assert(test_key_seq(keymap_,                                               \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        /* No latch-to-lock */                                 \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_Cyrillic_ha,     NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_e,               NEXT,  \
                        /* Lock the second group */                            \
                        KEY_SCROLLLOCK, BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     NEXT,  \
                        /* No latch-to-lock */                                 \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_s,               NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     NEXT,  \
                        /* Lock the third group */                             \
                        KEY_SCROLLLOCK, BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_Cyrillic_ha,     NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_Cyrillic_ie,     NEXT,  \
                        /* No latch-to-lock */                                 \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_Cyrillic_ie,     FINISH))
    test_no_latch_to_lock(keymap);

    xkb_keymap_unref(keymap);

    /* Absolute group, latch-to-lock */
    keymap = test_compile_rules(
        ctx, keymap_formats[f], "evdev", "pc104",
        "us,il,ru,de", ",,phonetic,neo",
        "grp:menu_latch_group2_lock,grp:sclk_toggle,grp:lctrl_rctrl_switch");
    assert(keymap);

    test_set_only(keymap);
    test_latch_only(keymap);
    test_latch_not_broken_by_modifier(keymap);
    test_simultaneous_group_latches(keymap);

    /* Lock */
    assert(test_key_seq(keymap,
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,
                        /* Lock the second group via latch-to-lock */
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     NEXT,
                        /* Lock the third group via usual lock */
                        KEY_SCROLLLOCK, BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_H,          BOTH,  XKB_KEY_Cyrillic_ha,     NEXT,
                        KEY_E,          BOTH,  XKB_KEY_Cyrillic_ie,     NEXT,
                        /* Lock the second group via latch-to-lock */
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     FINISH));

    xkb_keymap_unref(keymap);

    /* Relative group (positive), no lock */
    keymap = test_compile_rules(
        ctx, keymap_formats[f], "evdev", "pc104",
        "us,il,ru,de", ",,phonetic,neo",
        "grp:menu_latch,grp:sclk_toggle,grp:lctrl_rctrl_switch");
    assert(keymap);

    test_set_only(keymap);
    test_latch_only(keymap);
    test_latch_not_broken_by_modifier(keymap);
    test_simultaneous_group_latches(keymap);
    test_no_latch_to_lock(keymap);

    xkb_keymap_unref(keymap);

    /* Relative group (positive), latch-to-lock */
    keymap = test_compile_rules(
        ctx, keymap_formats[f], "evdev", "pc104",
        "us,il,ru,de", ",,phonetic,neo",
        "grp:menu_latch_lock,grp:sclk_toggle,grp:lctrl_rctrl_switch");
    assert(keymap);

    test_set_only(keymap);
    test_latch_only(keymap);
    test_latch_not_broken_by_modifier(keymap);
    test_simultaneous_group_latches(keymap);

    /* Lock */
    assert(test_key_seq(keymap,
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,
                        /* Lock the second group via latch-to-lock */
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     NEXT,
                        /* Lock the third group via usual lock */
                        KEY_SCROLLLOCK, BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_H,          BOTH,  XKB_KEY_Cyrillic_ha,     NEXT,
                        KEY_E,          BOTH,  XKB_KEY_Cyrillic_ie,     NEXT,
                        /* Lock the fourth group via latch-to-lock */
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,
                        KEY_H,          BOTH,  XKB_KEY_s,               NEXT,
                        KEY_E,          BOTH,  XKB_KEY_l,               FINISH));

    xkb_keymap_unref(keymap);

#undef test_set_only
#undef test_latch_only
#undef test_latch_not_broken_by_modifier
#undef test_simultaneous_group_latches

    /* Relative group (negative), no lock */
    keymap = test_compile_rules(
        ctx, keymap_formats[f], "evdev", "pc104",
        "us,il,ru,de", ",,phonetic,neo",
        "grp:menu_latch_negative,grp:sclk_toggle,grp:lctrl_rctrl_switch");
    assert(keymap);

    /* Set only */
#define test_set_only(keymap_)                                                 \
    assert(test_key_seq(keymap_,                                               \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_e,               NEXT,  \
                        KEY_COMPOSE,    DOWN,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_s,               NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_l,               NEXT,  \
                        KEY_COMPOSE,    UP,    XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Lock the second group */                            \
                        KEY_SCROLLLOCK, BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     NEXT,  \
                        KEY_COMPOSE,    DOWN,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_e,               NEXT,  \
                        KEY_COMPOSE,    UP,    XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     FINISH))
    test_set_only(keymap);

    /* Latch only */
#define test_latch_only(keymap_)                                               \
    assert(test_key_seq(keymap_,                                               \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_s,               NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Empty level breaks latches */                       \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_YEN,        BOTH,  XKB_KEY_NoSymbol,        NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Unknown key does not break latches */               \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        UINT32_MAX,     BOTH,  XKB_KEY_NoSymbol,        NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_s,               NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Lock the second group */                            \
                        KEY_SCROLLLOCK, BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     NEXT,  \
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,  \
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      FINISH))
    test_latch_only(keymap);

    /* Latch not broken by modifier */
#define test_latch_not_broken_by_modifier(keymap_)                           \
    assert(test_key_seq(keymap_,                                             \
                        KEY_H,        BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Sequential */                                     \
                        KEY_COMPOSE,  BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_LEFTALT,  BOTH,  XKB_KEY_Alt_L,           NEXT,  \
                        KEY_H,        BOTH,  XKB_KEY_s,               NEXT,  \
                        KEY_H,        BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Simultaneous */                                   \
                        KEY_COMPOSE,  DOWN,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_LEFTALT,  BOTH,  XKB_KEY_Alt_L,           NEXT,  \
                        KEY_COMPOSE,  UP,    XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,        BOTH,  get_keysym(keymap, XKB_KEY_h, XKB_KEY_s), NEXT,\
                        /* Simultaneous */                                   \
                        KEY_LEFTALT,  DOWN,  XKB_KEY_Alt_L,           NEXT,  \
                        KEY_COMPOSE,  BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_LEFTALT,  UP,    XKB_KEY_Alt_L,           NEXT,  \
                        KEY_H,        BOTH,  XKB_KEY_s,               NEXT,  \
                        KEY_H,        BOTH,  XKB_KEY_h,               NEXT,  \
                        /* Simultaneous */                                   \
                        KEY_LEFTALT,  DOWN,  XKB_KEY_Alt_L,           NEXT,  \
                        KEY_COMPOSE,  DOWN,  XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_LEFTALT,  UP,    XKB_KEY_Alt_L,           NEXT,  \
                        KEY_COMPOSE,  UP,    XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,        BOTH,  XKB_KEY_s,               NEXT,  \
                        KEY_H,        BOTH,  XKB_KEY_h,               FINISH))
    test_latch_not_broken_by_modifier(keymap);

    /* Simulatenous group actions */
#define test_simultaneous_group_latches(keymap_)                              \
    assert(test_key_seq(keymap_,                                              \
                        KEY_H,          BOTH, XKB_KEY_h,               NEXT,  \
                        KEY_RIGHTCTRL,  BOTH, XKB_KEY_ISO_Last_Group,  NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      NEXT,  \
                        /* Sequential */                                      \
                        KEY_COMPOSE,    BOTH, XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_SCROLLLOCK, BOTH, XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_Cyrillic_ha,     NEXT,  \
                        KEY_RIGHTCTRL,  BOTH, XKB_KEY_ISO_Last_Group,  NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      NEXT,  \
                        /* Simultaneous */                                    \
                        KEY_COMPOSE,    DOWN, XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_SCROLLLOCK, BOTH, XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_COMPOSE,    UP,   XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH, get_keysym(keymap, XKB_KEY_Cyrillic_ha, XKB_KEY_hebrew_yod), NEXT,\
                        KEY_H,          BOTH, XKB_KEY_Cyrillic_ha,     NEXT,  \
                        KEY_RIGHTCTRL,  BOTH, XKB_KEY_ISO_Last_Group,  NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      NEXT,  \
                        /* Simultaneous */                                    \
                        KEY_SCROLLLOCK, DOWN, XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_COMPOSE,    BOTH, XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_SCROLLLOCK, UP,   XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_Cyrillic_ha,     NEXT,  \
                        KEY_RIGHTCTRL,  BOTH, XKB_KEY_ISO_Last_Group,  NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      NEXT,  \
                        /* Simultaneous */                                    \
                        KEY_SCROLLLOCK, DOWN, XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_COMPOSE,    DOWN, XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_SCROLLLOCK, UP,   XKB_KEY_ISO_Next_Group,  NEXT,  \
                        KEY_COMPOSE,    UP,   XKB_KEY_ISO_Group_Latch, NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_Cyrillic_ha,     NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_Cyrillic_ha,     NEXT,  \
                        KEY_RIGHTCTRL,  BOTH, XKB_KEY_ISO_Last_Group,  NEXT,  \
                        KEY_H,          BOTH, XKB_KEY_hebrew_yod,      FINISH))
    test_simultaneous_group_latches(keymap);

    test_no_latch_to_lock(keymap);

    xkb_keymap_unref(keymap);

    /* Relative group (negative), no lock */
    keymap = test_compile_rules(
        ctx, keymap_formats[f], "evdev", "pc104",
        "us,il,ru,de", ",,phonetic,neo",
        "grp:menu_latch_negative_lock,grp:sclk_toggle,grp:lctrl_rctrl_switch");
    assert(keymap);

    test_set_only(keymap);
    test_latch_only(keymap);
    test_latch_not_broken_by_modifier(keymap);
    test_simultaneous_group_latches(keymap);

    /* Lock */
    assert(test_key_seq(keymap,
                        KEY_H,          BOTH,  XKB_KEY_h,               NEXT,
                        /* Lock the fourth group via latch-to-lock */
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,
                        KEY_H,          BOTH,  XKB_KEY_s,               NEXT,
                        KEY_E,          BOTH,  XKB_KEY_l,               NEXT,
                        /* Lock the third group via usual lock */
                        KEY_SCROLLLOCK, BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_SCROLLLOCK, BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_SCROLLLOCK, BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_H,          BOTH,  XKB_KEY_Cyrillic_ha,     NEXT,
                        KEY_E,          BOTH,  XKB_KEY_Cyrillic_ie,     NEXT,
                        /* Lock the second group via latch-to-lock */
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,
                        KEY_COMPOSE,    BOTH,  XKB_KEY_ISO_Group_Latch, NEXT,
                        KEY_H,          BOTH,  XKB_KEY_hebrew_yod,      NEXT,
                        KEY_E,          BOTH,  XKB_KEY_hebrew_qoph,     FINISH));

    xkb_keymap_unref(keymap);

#undef test_set_only
#undef test_latch_only
#undef test_latch_not_broken_by_modifier
#undef test_simultaneous_group_latches
#undef test_no_latch_to_lock
    }
}

static void
test_mod_set(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    /* Shift break caps: unlockOnPress=false */
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                                "evdev", "pc105", "us", "",
                                "shift:breaks_caps");
    assert(test_key_seq(keymap,
        KEY_CAPSLOCK,  BOTH, XKB_KEY_Caps_Lock, NEXT,
        KEY_A,         BOTH, XKB_KEY_A,         NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L,   NEXT,
        KEY_A,         BOTH, XKB_KEY_a,         NEXT,
        KEY_LEFTSHIFT, UP,   XKB_KEY_Shift_L,   NEXT,
        /* Caps still locked: key was operated before Shift release */
        KEY_A,         BOTH, XKB_KEY_A,         FINISH
    ));
    assert(keymap);

    xkb_keymap_unref(keymap);

    /* Shift break caps: unlockOnPress=true (XKB extension) */
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                                "evdev", "pc105", "us", "",
                                "shift:breaks_caps-v2");
    assert(keymap);
    assert(test_key_seq(keymap,
        KEY_CAPSLOCK,  BOTH, XKB_KEY_Caps_Lock, NEXT,
        KEY_A,         BOTH, XKB_KEY_A,         NEXT,
        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L,   NEXT,
        KEY_A,         BOTH, XKB_KEY_A,         NEXT,
        KEY_LEFTSHIFT, UP,   XKB_KEY_Shift_L,   NEXT,
        KEY_A,         BOTH, XKB_KEY_a,         FINISH
    ));
    xkb_keymap_unref(keymap);
}

static void
test_mod_lock(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    /*
     * Caps unlocks on release (all formats)
     * Implicit unlockOnPress=false (XKB spec)
     */
    for (unsigned int f = 0; f < ARRAY_SIZE(keymap_formats); f++) {
        keymap = test_compile_rules(ctx, keymap_formats[f],
                                    "evdev", "pc105", "us", "", "");
        assert(keymap);

#define test_caps_unlocks_on_release(keymap) assert(                 \
    test_key_seq((keymap),                                           \
                 KEY_Y,         BOTH, XKB_KEY_y,              NEXT,  \
                 /* Lock on press */                                 \
                 KEY_CAPSLOCK,  BOTH, XKB_KEY_Caps_Lock,      NEXT,  \
                 KEY_Y,         BOTH, XKB_KEY_Y,              NEXT,  \
                 KEY_CAPSLOCK,  DOWN, XKB_KEY_Caps_Lock,      NEXT,  \
                 /* No unlock on press */                            \
                 KEY_Y,         BOTH, XKB_KEY_Y,              NEXT,  \
                 KEY_CAPSLOCK,  UP,   XKB_KEY_Caps_Lock,      NEXT,  \
                 /* Unlock on release */                             \
                 KEY_Y,         BOTH, XKB_KEY_y,              FINISH)\
)

        test_caps_unlocks_on_release(keymap);

        xkb_keymap_unref(keymap);
    }

    /*
     * Caps unlocks on press for format V2
     * Explicit unlockOnPress=false (XKB spec)
     */
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                                "evdev", "pc105", "us", "",
                                "caps:unlock-on-release");
    assert(keymap);
    test_caps_unlocks_on_release(keymap);
    xkb_keymap_unref(keymap);

#undef test_caps_unlocks_on_release

    /*
     * Caps unlocks on press for format V2
     * Explicit unlockOnPress=true (XKB extension)
     */
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V2,
                                "evdev", "pc105", "us", "",
                                "caps:unlock-on-press");
    assert(keymap);

    test_key_seq((keymap),
                 KEY_Y,         BOTH, XKB_KEY_y,              NEXT,
                 KEY_CAPSLOCK,  BOTH, XKB_KEY_Caps_Lock,      NEXT,
                 /* Lock on press */
                 KEY_Y,         BOTH, XKB_KEY_Y,              NEXT,
                 KEY_CAPSLOCK,  DOWN, XKB_KEY_Caps_Lock,      NEXT,
                 /* Unlock on press */
                 KEY_Y,         BOTH, XKB_KEY_y,              NEXT,
                 KEY_CAPSLOCK,  UP,   XKB_KEY_Caps_Lock,      NEXT,
                 KEY_Y,         BOTH, XKB_KEY_y,              FINISH);

    xkb_keymap_unref(keymap);

}

static void
test_mod_latch(struct xkb_context *context)
{
    struct xkb_keymap *keymap;

    for (unsigned int f = 0; f < ARRAY_SIZE(keymap_formats); f++) {
        fprintf(stderr, "=== %s, format %u ===\n", __func__, keymap_formats[f]);

        keymap = test_compile_rules(context, keymap_formats[f], "evdev",
                                    "pc104", "latch", "", "");
        assert(keymap);

        /* Set: basic */
        assert(test_key_seq(keymap,
            KEY_Q         , BOTH, XKB_KEY_q      , NEXT,
            KEY_1         , BOTH, XKB_KEY_1      , NEXT,

            /* Empty level */
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L , NEXT,
            KEY_YEN       , BOTH, XKB_KEY_NoSymbol, NEXT, /* Prevent latch */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L , NEXT,
            KEY_Q         , BOTH, XKB_KEY_q       , NEXT,

            /* Unknown key */
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L , NEXT,
            UINT32_MAX    , BOTH, XKB_KEY_NoSymbol, NEXT, /* Does not prevent latch */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L , NEXT,
            KEY_1         , BOTH, XKB_KEY_exclam  , NEXT,
            KEY_Q         , BOTH, XKB_KEY_q       , NEXT,

            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L, NEXT,
            KEY_Q         , BOTH, XKB_KEY_Q      , NEXT,  /* Prevent latch */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L, NEXT,
            KEY_Q         , BOTH, XKB_KEY_q      , NEXT,

            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L, NEXT,
            KEY_Q         , BOTH, XKB_KEY_Q      , NEXT,  /* Prevent latch */
            KEY_1         , BOTH, XKB_KEY_exclam , NEXT,  /* Set is still active */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L, NEXT,
            KEY_Q         , BOTH, XKB_KEY_q      , NEXT,

            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L        , NEXT,
            KEY_F1        , BOTH, XKB_KEY_XF86Switch_VT_1, NEXT, /* Prevent latch */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L        , NEXT,
            KEY_Q         , BOTH, XKB_KEY_q              , NEXT,

            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L         , NEXT,
            KEY_LVL3      , BOTH, XKB_KEY_ISO_Level3_Shift, NEXT, /* v1: Prevent latch */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L         , NEXT,
            KEY_Q         , BOTH, get_keysym(keymap, XKB_KEY_q, XKB_KEY_Q), NEXT,

            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L         , NEXT,
            KEY_CAPSLOCK  , BOTH, XKB_KEY_ISO_Group_Latch , NEXT, /* v1: Prevent latch */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L         , NEXT,
            KEY_Q         , BOTH, get_keysym(keymap, XKB_KEY_q, XKB_KEY_Q), FINISH
        ));

        /* Set: mix with regular set */
        assert(test_key_seq(keymap,
            KEY_LVL3      , DOWN, XKB_KEY_ISO_Level3_Shift, NEXT, /* Set Level3 (regular) */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L         , NEXT, /* Set Shift (latch) */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Prevent Shift latch */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* State unchanged */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L         , NEXT,
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_LVL3      , UP,   XKB_KEY_ISO_Level3_Shift, NEXT, /* Unset Level3 */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L         , NEXT, /* Set Shift (latch) */
            KEY_LVL3      , DOWN, XKB_KEY_ISO_Level3_Shift, NEXT, /* Set Level3 (regular) */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Prevent Shift latch */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* State unchanged */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L         , NEXT,
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_LVL3      , UP,   XKB_KEY_ISO_Level3_Shift, NEXT, /* Unset Level3 */
            KEY_1         , BOTH, XKB_KEY_1               , FINISH
        ));

        /* Set: mix with regular lock */
        assert(test_key_seq(keymap,
            /* Only Lock */
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock , NEXT, /* Lock Level3 */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L         , NEXT, /* Set Shift (latch) */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Prevent Shift latch */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* State unchanged */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L         , NEXT, /* Unset shift (latch) */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock , NEXT, /* Unlock Level3 */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L         , NEXT, /* Set Shift (latch) */
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock , NEXT, /* Lock Level3 */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Prevent Shift latch */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* State unchanged */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L         , NEXT, /* Unset shift (latch) */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock , NEXT, /* Unlock Level3 */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            /* Set, then Lock */
            KEY_102ND     , DOWN, XKB_KEY_ISO_Level3_Lock , NEXT, /* Set Level3 (lock) */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L         , NEXT, /* Set Shift (latch) */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Prevent Shift latch */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* State unchanged */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L         , NEXT, /* Unset shift (latch) */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_102ND     , UP,   XKB_KEY_ISO_Level3_Lock , NEXT, /* Unset and lock Level3 */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock , NEXT, /* Unlock Level3 */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L         , NEXT, /* Set Shift (latch) */
            KEY_102ND     , DOWN, XKB_KEY_ISO_Level3_Lock , NEXT, /* Set Level3 (lock) */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Prevent Shift latch */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* State unchanged */
            KEY_LEFTSHIFT , UP,   XKB_KEY_Shift_L         , NEXT, /* Unset shift (latch) */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_102ND     , UP,   XKB_KEY_ISO_Level3_Lock , NEXT, /* Unset and lock Level3 */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock , NEXT, /* Unlock Level3 */
            KEY_1         , BOTH, XKB_KEY_1               , FINISH
        ));

        /* Basic latch/unlatch: breaking/preventing latch */
        assert(test_key_seq(keymap,
            /* Latch break: sequential */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L, NEXT,  /* Latch Shift */
            KEY_Q         , BOTH, XKB_KEY_Q      , NEXT,  /* No action: unlatch Shift */
            KEY_Q         , BOTH, XKB_KEY_q      , NEXT,

            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L        , NEXT, /* Latch Shift */
            KEY_F1        , BOTH, XKB_KEY_XF86Switch_VT_1, NEXT, /* VT action: unlatch Shift */
            KEY_1         , BOTH, XKB_KEY_1              , NEXT,

            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L        , NEXT, /* Latch Shift */
            KEY_CAPSLOCK  , BOTH, XKB_KEY_ISO_Group_Latch, NEXT, /* Group actions do not break latches */
            KEY_1         , BOTH, XKB_KEY_exclam         , NEXT,
            KEY_1         , BOTH, XKB_KEY_1              , NEXT,

            /* Latch prevented (DOWN/UP events) */
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L, NEXT, /* Set Shift */
            KEY_Q         , BOTH, XKB_KEY_Q      , NEXT, /* Prevent latch on DOWN event */
            KEY_LEFTSHIFT , UP  , XKB_KEY_Shift_L, NEXT, /* Unset Shift */
            KEY_Q         , BOTH, XKB_KEY_q      , NEXT,

            /* Latch prevented (DOWN event) */
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L, NEXT, /* Set Shift */
            KEY_Q         , DOWN, XKB_KEY_Q      , NEXT, /* Prevent latch */
            KEY_LEFTSHIFT , UP  , XKB_KEY_Shift_L, NEXT, /* Unset Shift */
            KEY_Q         , UP  , XKB_KEY_q      , NEXT,

            /* Latch not prevented (UP event) */
            KEY_Q         , DOWN, XKB_KEY_q      , NEXT, /* Prevent latch */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L, NEXT, /* Latch Shift */
            KEY_Q         , UP  , XKB_KEY_Q      , NEXT, /* Do not prevent latch */
            KEY_Q         , BOTH, XKB_KEY_Q      , NEXT, /* Unlatch Shift */
            KEY_Q         , BOTH, XKB_KEY_q      , NEXT,

            KEY_Q         , DOWN, XKB_KEY_q      , NEXT,
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L, NEXT, /* Set Shift */
            KEY_Q         , UP  , XKB_KEY_Q      , NEXT, /* Do not prevent latch */
            KEY_LEFTSHIFT , UP  , XKB_KEY_Shift_L, NEXT, /* Latch Shift */
            KEY_Q         , BOTH, XKB_KEY_Q      , NEXT, /* Unlatch Shift */
            KEY_Q         , BOTH, XKB_KEY_q      , FINISH
        ));

        /* Basic latch/unlatch: not breaking nor preventing latch */
        assert(test_key_seq(keymap,
            /* No latch break: sequential */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L        , NEXT, /* Latch Shift */
            KEY_RIGHTCTRL , BOTH, XKB_KEY_Control_R      , NEXT, /* Modifier action does not break latches */
            KEY_Q         , BOTH, XKB_KEY_Q              , NEXT, /* Unlatch Shift */
            KEY_Q         , BOTH, XKB_KEY_q              , NEXT,

            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L        , NEXT, /* Latch Shift */
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock, NEXT, /* Modifier action does not break latches */
            KEY_1         , BOTH, XKB_KEY_exclamdown     , NEXT, /* Unlatch Shift */
            KEY_1         , BOTH, XKB_KEY_onesuperior    , NEXT,
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock, NEXT,
            KEY_1         , BOTH, XKB_KEY_1              , NEXT,

            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L        , NEXT, /* Latch Shift */
            KEY_F2        , BOTH, XKB_KEY_ISO_Group_Shift, NEXT, /* Group action does not break latches */
            KEY_Q         , BOTH, XKB_KEY_Q              , NEXT, /* Unlatch Shift */
            KEY_Q         , BOTH, XKB_KEY_q              , NEXT,

            /* Latch not prevented (DOWN/UP events) */
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L  , NEXT, /* Set Shift */
            KEY_RIGHTCTRL , BOTH, XKB_KEY_Control_R, NEXT,
            KEY_LEFTSHIFT , UP  , XKB_KEY_Shift_L  , NEXT, /* v2: Latch Shift */
            KEY_Q         , BOTH, get_keysym(keymap, XKB_KEY_q, XKB_KEY_Q), NEXT,

            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L        , NEXT, /* Set Shift */
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock, NEXT,
            KEY_LEFTSHIFT , UP  , XKB_KEY_Shift_L        , NEXT, /* v2: Latch Shift */
            KEY_1         , BOTH, get_keysym(keymap, XKB_KEY_onesuperior, XKB_KEY_exclamdown), NEXT,
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock, NEXT,
            KEY_1         , BOTH, XKB_KEY_1              , NEXT,

            /* Latch not prevented (DOWN event) */
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L  , NEXT, /* Set Shift */
            KEY_RIGHTCTRL , DOWN, XKB_KEY_Control_R, NEXT,
            KEY_LEFTSHIFT , UP  , XKB_KEY_Shift_L  , NEXT, /* v2: Latch Shift */
            KEY_RIGHTCTRL , UP  , XKB_KEY_Control_R, NEXT,
            KEY_Q         , BOTH, get_keysym(keymap, XKB_KEY_q, XKB_KEY_Q), NEXT,

            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L        , NEXT, /* Set Shift */
            KEY_102ND     , DOWN, XKB_KEY_ISO_Level3_Lock, NEXT,
            KEY_LEFTSHIFT , UP  , XKB_KEY_Shift_L        , NEXT, /* v2: Latch Shift */
            KEY_102ND     , UP  , XKB_KEY_ISO_Level3_Lock, NEXT,
            KEY_1         , BOTH, get_keysym(keymap, XKB_KEY_onesuperior, XKB_KEY_exclamdown), NEXT,
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock, NEXT,
            KEY_1         , BOTH, XKB_KEY_1              , NEXT,

            /* Latch not prevented (UP event) */
            KEY_RIGHTCTRL , DOWN, XKB_KEY_Control_R, NEXT,
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L  , NEXT, /* Latch Shift */
            KEY_RIGHTCTRL , UP  , XKB_KEY_Control_R, NEXT,
            KEY_Q         , BOTH, XKB_KEY_Q        , NEXT, /* Unlatch Shift */
            KEY_Q         , BOTH, XKB_KEY_q        , NEXT,

            KEY_RIGHTCTRL , DOWN, XKB_KEY_Control_R, NEXT,
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L  , NEXT, /* Set Shift */
            KEY_RIGHTCTRL , UP  , XKB_KEY_Control_R, NEXT,
            KEY_LEFTSHIFT , UP  , XKB_KEY_Shift_L  , NEXT, /* Latch Shift */
            KEY_Q         , BOTH, XKB_KEY_Q        , NEXT, /* Unlatch Shift */
            KEY_Q         , BOTH, XKB_KEY_q        , NEXT,

            KEY_102ND     , DOWN, XKB_KEY_ISO_Level3_Lock, NEXT,
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L        , NEXT, /* Latch Shift */
            KEY_102ND     , UP  , XKB_KEY_ISO_Level3_Lock, NEXT,
            KEY_1         , BOTH, XKB_KEY_exclamdown     , NEXT, /* Unlatch Shift */
            KEY_1         , BOTH, XKB_KEY_onesuperior    , NEXT,
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock, NEXT,
            KEY_1         , BOTH, XKB_KEY_1              , NEXT,

            KEY_102ND     , DOWN, XKB_KEY_ISO_Level3_Lock, NEXT,
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L        , NEXT, /* Latch Shift */
            KEY_102ND     , UP  , XKB_KEY_ISO_Level3_Lock, NEXT,
            KEY_LEFTSHIFT , UP  , XKB_KEY_Shift_L        , NEXT, /* Latch Shift */
            KEY_1         , BOTH, XKB_KEY_exclamdown     , NEXT, /* Unlatch Shift */
            KEY_1         , BOTH, XKB_KEY_onesuperior    , NEXT,
            KEY_102ND     , BOTH, XKB_KEY_ISO_Level3_Lock, NEXT,
            KEY_1         , BOTH, XKB_KEY_1              , FINISH
        ));

        /* Latch-to-lock */
        assert(test_key_seq(keymap,
            /* Lock */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L, NEXT,  /* Latch Shift */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L, NEXT,  /* Lock Shift */
            KEY_Q         , BOTH, XKB_KEY_Q      , NEXT,
            KEY_Q         , BOTH, XKB_KEY_Q      , NEXT,
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L, NEXT,  /* Unlock Shift */
            KEY_Q         , BOTH, XKB_KEY_q      , NEXT,
            KEY_Q         , BOTH, XKB_KEY_q      , NEXT,

            /* No lock; cancel latch */
            KEY_RIGHTSHIFT, BOTH, XKB_KEY_Shift_R, NEXT,  /* Latch Shift */
            KEY_RIGHTSHIFT, BOTH, XKB_KEY_Shift_R, NEXT,  /* Unlatch Shift */
            KEY_Q         , BOTH, XKB_KEY_q      , FINISH

            // TODO: mix with regular set and lock
        ));

        /* Sequential (at most one key down at a time) */
        assert(test_key_seq(keymap,
            /* Latch */
            KEY_LEFTCTRL  , BOTH, XKB_KEY_Control_L, NEXT, /* Latch Control */
            KEY_LEFTALT   , BOTH, XKB_KEY_Alt_L    , NEXT, /* Latch Alt */
            KEY_1         , BOTH, XKB_KEY_plus     , NEXT, /* Unlatch Control, Unlatch Alt */
            KEY_1         , BOTH, XKB_KEY_1        , NEXT,

            /* Latch (repeat, no latch-to-lock) */
            KEY_RIGHTSHIFT, BOTH, XKB_KEY_Shift_R         , NEXT, /* Latch Shift */
            KEY_RIGHTSHIFT, BOTH, XKB_KEY_Shift_R         , NEXT, /* Unlatch Shift (no lock) */
            KEY_RIGHTALT  , BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Latch Level3 */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT, /* Unlatch all */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            KEY_RIGHTSHIFT, BOTH, XKB_KEY_Shift_R         , NEXT, /* Latch Shift */
            KEY_RIGHTALT  , BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Latch Level3 */
            KEY_RIGHTSHIFT, BOTH, XKB_KEY_Shift_R         , NEXT, /* Unlatch Shift (no lock) */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT, /* Unlatch all */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            /* Lock one, latch the other */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Lock Shift */
            KEY_RIGHTALT  , BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Latch Level3 */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Unlatch Level3 */
            KEY_1         , BOTH, XKB_KEY_exclam          , NEXT,
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Unlock Shift */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_RIGHTALT  , BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Latch Level3 */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Lock Shift */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Unlatch Level3 */
            KEY_1         , BOTH, XKB_KEY_exclam          , NEXT,
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Unlock Shift */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            /* Lock both */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Lock Shift */
            KEY_RIGHTALT  , BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Latch Level3 */
            KEY_RIGHTALT  , BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Lock Level3 */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT,
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT,
            KEY_RIGHTALT  , BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Unlock Level3 */
            KEY_1         , BOTH, XKB_KEY_exclam          , NEXT,
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Unlock Shift */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_RIGHTALT  , BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Latch Level3 */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Lock Shift */
            KEY_RIGHTALT  , BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Lock Level3 */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT,
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT,
            KEY_RIGHTALT  , BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Unlock Level3 */
            KEY_1         , BOTH, XKB_KEY_exclam          , NEXT,
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Unlock Shift */
            KEY_1         , BOTH, XKB_KEY_1               , FINISH
        ));

        // TODO: Sequential with regular set & lock

        /* Simultaneous (multiple keys down) */
        assert(test_key_seq(keymap,
            /* Set */
            KEY_RIGHTALT  , DOWN, XKB_KEY_ISO_Level3_Latch, NEXT, /* Set Level3 */
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L         , NEXT, /* Set Shift */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Prevent latches */
            KEY_RIGHTALT  , UP  , XKB_KEY_ISO_Level3_Latch, NEXT, /* Unset Level3 */
            KEY_1         , BOTH, XKB_KEY_exclam          , NEXT, /* Shift still active */
            KEY_LEFTSHIFT , UP  , XKB_KEY_Shift_L         , NEXT, /* Unset Shift */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            /* Set one, latch the other */
            KEY_RIGHTALT  , DOWN, XKB_KEY_ISO_Level3_Latch, NEXT, /* Set Level3 */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Unlatch Shift, prevent Level3 latch */
            KEY_RIGHTALT  , UP  , XKB_KEY_ISO_Level3_Latch, NEXT, /* Unset Level3 */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            KEY_RIGHTALT  , DOWN, XKB_KEY_ISO_Level3_Latch, NEXT, /* Set Level3 */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Unlatch Shift, prevent Level3 latch */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT, /* Level 3 still active */
            KEY_RIGHTALT  , UP  , XKB_KEY_ISO_Level3_Latch, NEXT, /* Unset Level3 */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            /* Set both, latch both */
            KEY_RIGHTALT  , DOWN, XKB_KEY_ISO_Level3_Latch, NEXT, /* Set Level3 */
            KEY_LEFTSHIFT , DOWN, XKB_KEY_Shift_L         , NEXT, /* Set Shift */
            KEY_RIGHTALT  , UP  , XKB_KEY_ISO_Level3_Latch, NEXT, /* v2: Latch Level3 */
            KEY_LEFTSHIFT , UP  , XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_1         , BOTH, get_keysym(keymap, XKB_KEY_exclam, XKB_KEY_exclamdown), NEXT, /* Unlatch Shift */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            KEY_RIGHTALT  , DOWN, XKB_KEY_ISO_Level3_Latch, NEXT, /* Set Level3 */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_RIGHTALT  , UP  , XKB_KEY_ISO_Level3_Latch, NEXT, /* v2: Latch Level3 */
            KEY_1         , BOTH, get_keysym(keymap, XKB_KEY_exclam, XKB_KEY_exclamdown), NEXT, /* Unlatch Shift */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            /* Set one, lock the other */
            KEY_RIGHTALT  , DOWN, XKB_KEY_ISO_Level3_Latch, NEXT, /* Set Level3 */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Lock Shift */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT,
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT,
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Unlock Shift */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT,
            KEY_RIGHTALT  , UP  , XKB_KEY_ISO_Level3_Latch, NEXT, /* Unset Level3 */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            KEY_RIGHTALT  , DOWN, XKB_KEY_ISO_Level3_Latch, NEXT, /* Set Level3 */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Lock Shift */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT,
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT,
            KEY_RIGHTALT  , UP  , XKB_KEY_ISO_Level3_Latch, NEXT, /* Unset Level3 */
            KEY_1         , BOTH, XKB_KEY_exclam          , NEXT,
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Unlock Shift */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            /* Latch one, set the other */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_RIGHTALT  , DOWN, XKB_KEY_ISO_Level3_Latch, NEXT, /* Set Level3 */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Unlatch Shift, prevent Level3 latch */
            KEY_RIGHTALT  , UP  , XKB_KEY_ISO_Level3_Latch, NEXT, /* Unset Level3 */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_RIGHTALT  , DOWN, XKB_KEY_ISO_Level3_Latch, NEXT, /* Set Level3 */
            KEY_1         , BOTH, XKB_KEY_exclamdown      , NEXT, /* Unlatch Shift, prevent Level3 latch */
            KEY_1         , BOTH, XKB_KEY_onesuperior     , NEXT, /* Level3 still active */
            KEY_RIGHTALT  , UP  , XKB_KEY_ISO_Level3_Latch, NEXT, /* Unset Level3 */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            /* Latch one, lock the other */
            KEY_RIGHTALT  , DOWN, XKB_KEY_ISO_Level3_Latch, NEXT, /* Set Level3 */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Lock Shift */
            KEY_RIGHTALT  , UP  , XKB_KEY_ISO_Level3_Latch, NEXT, /* v2: Latch Level3 */
            KEY_1         , BOTH, get_keysym(keymap, XKB_KEY_exclam, XKB_KEY_exclamdown), NEXT,
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Unlock Shift */
            KEY_1         , BOTH, XKB_KEY_1               , NEXT,

            KEY_RIGHTALT  , DOWN, XKB_KEY_ISO_Level3_Latch, NEXT, /* Set Level3 */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Latch Shift */
            KEY_RIGHTALT  , UP  , XKB_KEY_ISO_Level3_Latch, NEXT, /* v2: Latch Level3 */
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Lock Shift */
            KEY_1         , BOTH, get_keysym(keymap, XKB_KEY_exclam, XKB_KEY_exclamdown), NEXT,
            KEY_LEFTSHIFT , BOTH, XKB_KEY_Shift_L         , NEXT, /* Unlock Shift */
            KEY_1         , BOTH, XKB_KEY_1               , FINISH
        ));

        xkb_keymap_unref(keymap);
    }

    /*
     * Mod latch on release (all formats)
     * Implicit latchOnPress=false (XKB spec)
     */
    for (unsigned int f = 0; f < ARRAY_SIZE(keymap_formats); f++) {
        keymap = test_compile_rules(context, keymap_formats[f], "evdev",
                                    "pc104", "de", "", "lv3:ralt_latch");
        assert(keymap);
#define test_mod_latch_on_release(keymap) assert(                     \
    test_key_seq((keymap),                                            \
                 KEY_A       , BOTH, XKB_KEY_a,                NEXT,  \
                 /* Regular latch */                                  \
                 KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level3_Latch, NEXT,  \
                 KEY_A       , BOTH, XKB_KEY_ae,               NEXT,  \
                 KEY_A       , BOTH, XKB_KEY_a,                NEXT,  \
                 /* Latch to lock */                                  \
                 KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level3_Latch, NEXT,  \
                 KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level3_Latch, NEXT,  \
                 KEY_A       , BOTH, XKB_KEY_ae,               NEXT,  \
                 KEY_A       , BOTH, XKB_KEY_ae,               NEXT,  \
                 /* Unlock on release */                              \
                 KEY_RIGHTALT, DOWN, XKB_KEY_ISO_Level3_Latch, NEXT,  \
                 KEY_A       , BOTH, XKB_KEY_ae,               NEXT,  \
                 KEY_A       , BOTH, XKB_KEY_ae,               NEXT,  \
                 KEY_RIGHTALT, UP,   XKB_KEY_ISO_Level3_Latch, NEXT,  \
                 KEY_A       , BOTH, XKB_KEY_a,                NEXT,  \
                 /* Maintained pressed */                             \
                 KEY_RIGHTALT, DOWN, XKB_KEY_ISO_Level3_Latch, NEXT,  \
                 /* Degrade to set */                                 \
                 KEY_A       , BOTH, XKB_KEY_ae,               NEXT,  \
                 KEY_A       , BOTH, XKB_KEY_ae,               NEXT,  \
                 KEY_RIGHTALT, UP,   XKB_KEY_ISO_Level3_Latch, NEXT,  \
                 KEY_A       , BOTH, XKB_KEY_a,                FINISH)\
)
        test_mod_latch_on_release(keymap);

        xkb_keymap_unref(keymap);
    }

    /*
     * Mod latch on release for format V2
     * Explicit latchOnPress=false (XKB spec)
     */
    keymap = test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V2, "evdev",
                                "pc104", "de", "", "lv3:ralt_latch,lv3:latchOnRelease");
    assert(keymap);
    test_mod_latch_on_release(keymap);
    xkb_keymap_unref(keymap);

#undef test_mod_latch_on_release

    /*
     * Mod latch on press for format V2
     * Explicit latchOnPress=true (XKB extension)
     */
    keymap = test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V2, "evdev",
                                "pc104", "de", "", "lv3:ralt_latch,lv3:latchOnPress");
    assert(keymap);

    assert(test_key_seq(keymap,
                        KEY_A       , BOTH, XKB_KEY_a,                NEXT,
                        /* Regular latch */
                        KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level3_Latch, NEXT,
                        KEY_A       , BOTH, XKB_KEY_ae,               NEXT,
                        KEY_A       , BOTH, XKB_KEY_a,                NEXT,
                        /* Latch to lock */
                        KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level3_Latch, NEXT,
                        KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level3_Latch, NEXT,
                        KEY_A       , BOTH, XKB_KEY_ae,               NEXT,
                        KEY_A       , BOTH, XKB_KEY_ae,               NEXT,
                        /* Unlock on press */
                        KEY_RIGHTALT, DOWN, XKB_KEY_ISO_Level3_Latch, NEXT,
                        KEY_A       , BOTH, XKB_KEY_a,                NEXT,
                        KEY_A       , BOTH, XKB_KEY_a,                NEXT,
                        KEY_RIGHTALT, UP,   XKB_KEY_ISO_Level3_Latch, NEXT,
                        KEY_A       , BOTH, XKB_KEY_a,                NEXT,
                        /* Maintained pressed: latched on press */
                        KEY_RIGHTALT, DOWN, XKB_KEY_ISO_Level3_Latch, NEXT,
                        KEY_A       , BOTH, XKB_KEY_ae,               NEXT,
                        /* Broken latch */
                        KEY_A       , BOTH, XKB_KEY_a,                NEXT,
                        KEY_RIGHTALT, UP,   XKB_KEY_ISO_Level3_Latch, NEXT,
                        KEY_A       , BOTH, XKB_KEY_a,                FINISH));

    xkb_keymap_unref(keymap);

    /*
     * If `Caps_Lock` is on the second level of some key, and `Shift` is
     * latched, pressing the key locks `Caps` while also breaking the `Shift`
     * latch, ensuring that the next character is properly uppercase.
     *
     * Implemented using: multiple actions per level + VoidAction()
     */
    const char lock_breaks_latch[] =
        "xkb_keymap {\n"
        "  xkb_keycodes { <lshift> = 50; <a> = 38; };\n"
        "  xkb_types { include \"basic\" };\n"
        "  xkb_compat {\n"
        "    interpret ISO_Level2_Latch {\n"
        "      action = LatchMods(modifiers=Shift,latchToLock,clearLocks);\n"
        "    };\n"
        /* Activating CapsLock will break all latches */
        "    interpret Caps_Lock {\n"
        "      action = {LockMods(modifiers=Lock), VoidAction()};\n"
        "    };\n"
        "  };\n"
        "  xkb_symbols {\n"
        "    key <lshift> { [ISO_Level2_Latch, Caps_Lock], type=\"ALPHABETIC\" };\n"
        "    key <a> { [a, A] };\n"
        "  };\n"
        "};";
    keymap = test_compile_buffer(context, XKB_KEYMAP_FORMAT_TEXT_V2,
                                 lock_breaks_latch, sizeof(lock_breaks_latch));
    assert(keymap);
    assert(test_key_seq(keymap,
                        KEY_A        , BOTH, XKB_KEY_a,                NEXT,
                        /* Regular latch */
                        KEY_LEFTSHIFT, BOTH, XKB_KEY_ISO_Level2_Latch, NEXT,
                        KEY_A        , BOTH, XKB_KEY_A,                NEXT,
                        KEY_A        , BOTH, XKB_KEY_a,                NEXT,
                        /* Trigger CapsLock */
                        KEY_LEFTSHIFT, BOTH, XKB_KEY_ISO_Level2_Latch, NEXT,
                        KEY_LEFTSHIFT, BOTH, XKB_KEY_Caps_Lock,        NEXT,
                        /* CapsLock active, latch broken */
                        KEY_A        , BOTH, XKB_KEY_A,                NEXT,
                        KEY_A        , BOTH, XKB_KEY_A,                NEXT,
                        /* Unlock Caps */
                        KEY_LEFTSHIFT, BOTH, XKB_KEY_Caps_Lock,        NEXT,
                        KEY_A        , BOTH, XKB_KEY_a,                NEXT,
                        KEY_A        , BOTH, XKB_KEY_a,                FINISH));
    xkb_keymap_unref(keymap);

    /*
     * Make a latch break a previous latch on the German E1 layout.
     *
     * Implemented using: multiple actions per level + VoidAction()
     */
    const char lv5_latch_breaks_lv3_latch[] =
        "xkb_keymap {\n"
        "  xkb_keycodes { <lshift> = 50; <ralt> = 108; <e> = 26; <f> = 41; };\n"
        "  xkb_types  { include \"complete\" };\n"
        "  xkb_compat { include \"complete\" };\n"
        "  xkb_symbols {\n"
        "    virtual_modifiers LevelFive;\n"
        "    key <lshift> { [ISO_Level2_Latch], [LatchMods(modifiers=Shift)]};\n"
        "    key <ralt> { [ISO_Level3_Latch] };\n"
        /* Excerpt from the German E1 `de(e1)` layout */
        "    key.type = \"EIGHT_LEVEL_SEMIALPHABETIC\";\n"
        "    key <e> { [e,          E,          EuroSign,         any, schwa, SCHWA] };\n"
        "    key <f> { [f,          F,          ISO_Level5_Latch, any, any,   any  ],\n"
        /* Use VoidAction() to break previous latches */
        "              [NoAction(), NoAction(), {VoidAction(), LatchMods(modifiers=LevelFive)}] };\n"
        "  };\n"
        "};";
    keymap = test_compile_buffer(context, XKB_KEYMAP_FORMAT_TEXT_V2,
                                 lv5_latch_breaks_lv3_latch,
                                 sizeof(lv5_latch_breaks_lv3_latch));
    assert(keymap);
    assert(test_key_seq(keymap,
                        KEY_E       , BOTH, XKB_KEY_e,                 NEXT,
                        /* Level 3 latch */
                        KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level3_Latch,  NEXT,
                        KEY_E       , BOTH, XKB_KEY_EuroSign,          NEXT,
                        KEY_E       , BOTH, XKB_KEY_e,                 NEXT,
                        /* Level 3 latch */
                        KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level3_Latch,  NEXT,
                        /* Level 5 latch */
                        KEY_F,        BOTH, XKB_KEY_ISO_Level5_Latch,  NEXT,
                        /* Level 3 latch broken, level 5 latch active */
                        KEY_E       , BOTH, XKB_KEY_schwa,             NEXT,
                        KEY_E       , BOTH, XKB_KEY_e,                 NEXT,
                        /* Level 3 latch */
                        KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level3_Latch,  NEXT,
                        /* Level 5 latch */
                        KEY_F,        BOTH, XKB_KEY_ISO_Level5_Latch,  NEXT,
                        /* Level 3 latch broken, level 5 latch active */
                        KEY_LEFTSHIFT, BOTH, XKB_KEY_ISO_Level2_Latch, NEXT,
                        /* Shift + level 5 latches */
                        KEY_E        , BOTH, XKB_KEY_SCHWA,            NEXT,
                        KEY_E        , BOTH, XKB_KEY_e,                FINISH));
    xkb_keymap_unref(keymap);
}

struct key_properties {
    const char *name;
    bool repeats;
    xkb_mod_mask_t vmodmap;
};

static void
test_explicit_actions(struct xkb_context *ctx)
{
    struct xkb_keymap *original = test_compile_file(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "keymaps/explicit-actions.xkb"
    );
    assert(original);

    /* Reload keymap from its dump */
    char *dump = xkb_keymap_get_as_string(original,
                                          XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    assert(dump);
    struct xkb_keymap *roundtrip =
        test_compile_string(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, dump);
    free(dump);

    struct xkb_keymap *keymaps[] = { original, roundtrip };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        assert(keymaps[k]);

        /*
         * <LALT>: Groups 1 & 3 have no explicit actions while group 2 does.
         * We expect that groups 1 & 3 will have the corresponding interpret run
         * to set their actions.
         *
         * <LVL3> has explicit actions on group 2; dumping the keymap forces
         * explicit actions as well as the essential virtualMods=LevelThree field.
         *
		 * <AD05> has explicit actions on group 2; dumping the keymap forces
         * explicit actions as well as repeat=Yes.
         */
        const struct key_properties keys[] = {
            { .name = "LALT", .repeats = false, .vmodmap = 0 },
            { .name = "LVL3", .repeats = false, .vmodmap = UINT32_C(1) << 10 },
            { .name = "AD05", .repeats = true , .vmodmap = 0 },
            /* No explicit actions, check defaults */
            { .name = "AD06", .repeats = true , .vmodmap = 0 },
        };
        for (unsigned int i = 0; i < ARRAY_SIZE(keys); i++) {
            xkb_keycode_t kc = xkb_keymap_key_by_name(keymaps[k], keys[i].name);
            assert(kc != XKB_KEYCODE_INVALID);
            assert(keys[i].repeats == xkb_keymap_key_repeats(keymaps[k], kc));
            assert(keys[i].vmodmap == keymaps[k]->keys[kc].vmodmap);
        }
        assert(test_key_seq(
            keymaps[k],
            KEY_Y,         BOTH,  XKB_KEY_y,                NEXT,
            KEY_LEFTALT,   DOWN,  XKB_KEY_Shift_L,          NEXT,
            KEY_Y,         BOTH,  XKB_KEY_Y,                NEXT,
            KEY_LEFTALT,   UP,    XKB_KEY_Shift_L,          NEXT,
            KEY_COMPOSE,   BOTH,  XKB_KEY_ISO_Next_Group,   NEXT,
            KEY_Y,         BOTH,  XKB_KEY_z,                NEXT,
            KEY_LEFTALT,   DOWN,  XKB_KEY_ISO_Level3_Shift, NEXT,
            KEY_Y,         BOTH,  XKB_KEY_leftarrow,        NEXT,
            KEY_LEFTALT,   UP,    XKB_KEY_ISO_Level3_Shift, NEXT,
            KEY_COMPOSE,   BOTH,  XKB_KEY_ISO_Next_Group,   NEXT,
            KEY_Y,         BOTH,  XKB_KEY_k,                NEXT,
            KEY_LEFTALT,   DOWN,  XKB_KEY_ISO_Level5_Shift, NEXT,
            KEY_Y,         BOTH,  XKB_KEY_exclamdown,       NEXT,
            KEY_LEFTALT,   UP,    XKB_KEY_ISO_Level5_Shift, NEXT,
            KEY_LEFTSHIFT, DOWN,  XKB_KEY_Shift_L,          NEXT,
            KEY_LEFTALT,   DOWN,  XKB_KEY_ISO_Level3_Shift, NEXT,
            KEY_Y,         BOTH,  XKB_KEY_Greek_kappa,      NEXT,
            KEY_LEFTALT,   UP,    XKB_KEY_ISO_Level3_Shift, NEXT,
            KEY_LEFTSHIFT, UP,    XKB_KEY_Caps_Lock,        NEXT,
            KEY_Y,         BOTH,  XKB_KEY_k,                FINISH
        ));

        xkb_keymap_unref(keymaps[k]);
    }
}

static void
test_simultaneous_modifier_clear(struct xkb_context *context)
{
    struct xkb_keymap *keymap;

    keymap = test_compile_rules(context, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                                "pc104", "simultaneous-mods-latches", "", "");
    assert(keymap);

    /*
     * Github issue #583: simultaneous latches of *different* modifiers should
     * not affect each other when clearing their mods.
     */

    /* Original key sequence reported in the issue */
    assert(test_key_seq(keymap,
        KEY_LEFTCTRL, DOWN, XKB_KEY_Control_L       , NEXT, /* Set Control                    */
        KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level5_Latch, NEXT, /* Latch Level5                   */
        KEY_LEFTCTRL, UP  , XKB_KEY_Control_L       , NEXT, /* Unset Control                  */
        KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Latch Level3                   */
        KEY_Z       , BOTH, XKB_KEY_ydiaeresis      , NEXT, /* Unlatch Level3, unlatch Level5 */
        KEY_Z       , BOTH, XKB_KEY_z               , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z               , FINISH
    ));

    /* Alternative key sequence with only mod latches */
    assert(test_key_seq(keymap,
        KEY_RIGHTCTRL, BOTH, XKB_KEY_Control_R       , NEXT, /* Latch Control                      */
        KEY_RIGHTALT,  BOTH, XKB_KEY_ISO_Level5_Latch, NEXT, /* Latch Level5                       */
        KEY_LEFTMETA,  BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Latch Level3                       */
        KEY_Z       ,  BOTH, XKB_KEY_ydiaeresis      , NEXT, /* Unlatch Control, Level3 and Level5 */
        KEY_Z       ,  BOTH, XKB_KEY_z               , NEXT,
        KEY_Z       ,  BOTH, XKB_KEY_z               , NEXT,
        KEY_X       ,  BOTH, XKB_KEY_x               , FINISH
    ));

    /* Alternative simplier key sequence */
    assert(test_key_seq(keymap,
        KEY_LEFTMETA,  BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Latch Level3                   */
        KEY_RIGHTMETA, BOTH, XKB_KEY_ISO_Level5_Latch, NEXT, /* Latch Level5                   */
        KEY_Z       ,  BOTH, XKB_KEY_ydiaeresis      , NEXT, /* Unlatch Level3, unlatch Level5 */
        KEY_Z       ,  BOTH, XKB_KEY_z               , NEXT,
        KEY_Z       ,  BOTH, XKB_KEY_z               , FINISH
    ));

    /*
     * Test same modifier latch but on a different key
     */

    /* Level 3 */
    assert(test_key_seq(keymap,
        KEY_LEFTMETA, BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Latch Level3          */
        KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Lock Level3 via latch */
        KEY_Z       , BOTH, XKB_KEY_y               , NEXT, /* Locked Level3         */
        KEY_Z       , BOTH, XKB_KEY_y               , NEXT,
        KEY_RIGHTALT, BOTH, XKB_KEY_ISO_Level3_Latch, NEXT, /* Unlock Level3 via latch */
        KEY_Z       , BOTH, XKB_KEY_z               , NEXT,
        KEY_Z       , BOTH, XKB_KEY_z               , FINISH
    ));

    /* Level 5, via Control latch */
    assert(test_key_seq(keymap,
        KEY_RIGHTCTRL, BOTH, XKB_KEY_Control_R       , NEXT, /* Latch Control        */
        KEY_RIGHTALT,  BOTH, XKB_KEY_ISO_Level5_Latch, NEXT, /* Lock Level5 via latch */
        KEY_RIGHTMETA, BOTH, XKB_KEY_ISO_Level5_Latch, NEXT, /* Latch Level5          */
        KEY_Z       ,  BOTH, XKB_KEY_ezh             , NEXT, /* Locked Level5         */
        KEY_Z       ,  BOTH, XKB_KEY_ezh             , NEXT,
        KEY_RIGHTMETA, BOTH, XKB_KEY_ISO_Level5_Latch, NEXT, /* Unlock Level5 via latch */
        KEY_Z       ,  BOTH, XKB_KEY_z               , NEXT,
        KEY_Z       ,  BOTH, XKB_KEY_z               , NEXT,
        KEY_X       ,  BOTH, XKB_KEY_x               , FINISH
    ));

    xkb_keymap_unref(keymap);
}

static void
test_keymaps(struct xkb_context *ctx, const char* rules)
{
    struct xkb_keymap *keymap;

    assert(ctx);


    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                                "evdev", "us,il,ru,de", ",,phonetic,neo",
                                "grp:alt_shift_toggle,grp:menu_toggle");
    assert(keymap);

    assert(test_key_seq(keymap,
                        KEY_H,  BOTH,  XKB_KEY_h,  NEXT,
                        KEY_E,  BOTH,  XKB_KEY_e,  NEXT,
                        KEY_L,  BOTH,  XKB_KEY_l,  NEXT,
                        KEY_L,  BOTH,  XKB_KEY_l,  NEXT,
                        KEY_O,  BOTH,  XKB_KEY_o,  FINISH));

    /* Simple shifted level. */
    assert(test_key_seq(keymap,
                        KEY_H,          BOTH,  XKB_KEY_h,        NEXT,
                        KEY_LEFTSHIFT,  DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_E,          BOTH,  XKB_KEY_E,        NEXT,
                        KEY_L,          BOTH,  XKB_KEY_L,        NEXT,
                        KEY_LEFTSHIFT,  UP,    XKB_KEY_Shift_L,  NEXT,
                        KEY_L,          BOTH,  XKB_KEY_l,        NEXT,
                        KEY_O,          BOTH,  XKB_KEY_o,        FINISH));

    /* Key repeat shifted and unshifted in the middle. */
    assert(test_key_seq(keymap,
                        KEY_H,           DOWN,    XKB_KEY_h,        NEXT,
                        KEY_H,           REPEAT,  XKB_KEY_h,        NEXT,
                        KEY_H,           REPEAT,  XKB_KEY_h,        NEXT,
                        KEY_LEFTSHIFT,   DOWN,    XKB_KEY_Shift_L,  NEXT,
                        KEY_H,           REPEAT,  XKB_KEY_H,        NEXT,
                        KEY_H,           REPEAT,  XKB_KEY_H,        NEXT,
                        KEY_LEFTSHIFT,   UP,      XKB_KEY_Shift_L,  NEXT,
                        KEY_H,           REPEAT,  XKB_KEY_h,        NEXT,
                        KEY_H,           REPEAT,  XKB_KEY_h,        NEXT,
                        KEY_H,           UP,      XKB_KEY_h,        NEXT,
                        KEY_H,           BOTH,    XKB_KEY_h,        FINISH));

    /* Base modifier cleared on key release... */
    assert(test_key_seq(keymap,
                        KEY_H,          BOTH,  XKB_KEY_h,        NEXT,
                        KEY_LEFTSHIFT,  DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_E,          BOTH,  XKB_KEY_E,        NEXT,
                        KEY_L,          BOTH,  XKB_KEY_L,        NEXT,
                        KEY_LEFTSHIFT,  DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_L,          BOTH,  XKB_KEY_L,        NEXT,
                        KEY_O,          BOTH,  XKB_KEY_O,        FINISH));

    /* ... But only by the keycode that set it. */
    assert(test_key_seq(keymap,
                        KEY_H,           BOTH,  XKB_KEY_h,        NEXT,
                        KEY_LEFTSHIFT,   DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_E,           BOTH,  XKB_KEY_E,        NEXT,
                        KEY_L,           BOTH,  XKB_KEY_L,        NEXT,
                        KEY_RIGHTSHIFT,  UP,    XKB_KEY_Shift_R,  NEXT,
                        KEY_L,           BOTH,  XKB_KEY_L,        NEXT,
                        KEY_O,           BOTH,  XKB_KEY_O,        FINISH));

    /*
     * A base modifier should only be cleared when no other key affecting
     * the modifier is down.
     */
    assert(test_key_seq(keymap,
                        KEY_H,           BOTH,  XKB_KEY_h,        NEXT,
                        KEY_LEFTSHIFT,   DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_E,           BOTH,  XKB_KEY_E,        NEXT,
                        KEY_RIGHTSHIFT,  DOWN,  XKB_KEY_Shift_R,  NEXT,
                        KEY_L,           BOTH,  XKB_KEY_L,        NEXT,
                        KEY_RIGHTSHIFT,  UP,    XKB_KEY_Shift_R,  NEXT,
                        KEY_L,           BOTH,  XKB_KEY_L,        NEXT,
                        KEY_LEFTSHIFT,   UP,    XKB_KEY_Shift_L,  NEXT,
                        KEY_O,           BOTH,  XKB_KEY_o,        FINISH));

    /*
     * Two key presses from the same key (e.g. if two keyboards use the
     * same xkb_state) should only be released after two releases.
     */
    assert(test_key_seq(keymap,
                        KEY_H,           BOTH,  XKB_KEY_h,        NEXT,
                        KEY_LEFTSHIFT,   DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_H,           BOTH,  XKB_KEY_H,        NEXT,
                        KEY_LEFTSHIFT,   DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_H,           BOTH,  XKB_KEY_H,        NEXT,
                        KEY_LEFTSHIFT,   UP,    XKB_KEY_Shift_L,  NEXT,
                        KEY_H,           BOTH,  XKB_KEY_H,        NEXT,
                        KEY_LEFTSHIFT,   UP,    XKB_KEY_Shift_L,  NEXT,
                        KEY_H,           BOTH,  XKB_KEY_h,        FINISH));

    /* Same as above with locked modifiers. */
    assert(test_key_seq(keymap,
                        KEY_H,           BOTH,  XKB_KEY_h,          NEXT,
                        KEY_CAPSLOCK,    DOWN,  XKB_KEY_Caps_Lock,  NEXT,
                        KEY_H,           BOTH,  XKB_KEY_H,          NEXT,
                        KEY_CAPSLOCK,    DOWN,  XKB_KEY_Caps_Lock,  NEXT,
                        KEY_H,           BOTH,  XKB_KEY_H,          NEXT,
                        KEY_CAPSLOCK,    UP,    XKB_KEY_Caps_Lock,  NEXT,
                        KEY_H,           BOTH,  XKB_KEY_H,          NEXT,
                        KEY_CAPSLOCK,    UP,    XKB_KEY_Caps_Lock,  NEXT,
                        KEY_H,           BOTH,  XKB_KEY_H,          NEXT,
                        KEY_CAPSLOCK,    BOTH,  XKB_KEY_Caps_Lock,  NEXT,
                        KEY_H,           BOTH,  XKB_KEY_h,          FINISH));

    /* Group switching / locking. */
    assert(test_key_seq(keymap,
                        KEY_H,        BOTH,  XKB_KEY_h,               NEXT,
                        KEY_E,        BOTH,  XKB_KEY_e,               NEXT,
                        KEY_COMPOSE,  BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_K,        BOTH,  XKB_KEY_hebrew_lamed,    NEXT,
                        KEY_F,        BOTH,  XKB_KEY_hebrew_kaph,     NEXT,
                        KEY_COMPOSE,  BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_COMPOSE,  BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_COMPOSE,  BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_O,        BOTH,  XKB_KEY_o,               FINISH));

    assert(test_key_seq(keymap,
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L,        NEXT,
                        KEY_LEFTALT,   DOWN, XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_LEFTALT,   UP,   XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_LEFTSHIFT, UP,   XKB_KEY_Shift_L,        FINISH));

    assert(test_key_seq(keymap,
                        KEY_LEFTALT,   DOWN, XKB_KEY_Alt_L,          NEXT,
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_LEFTSHIFT, UP,   XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_LEFTALT,   UP,   XKB_KEY_Alt_L,          FINISH));

    /* Locked modifiers. */
    assert(test_key_seq(keymap,
                        KEY_CAPSLOCK,  BOTH,  XKB_KEY_Caps_Lock,  NEXT,
                        KEY_H,         BOTH,  XKB_KEY_H,          NEXT,
                        KEY_E,         BOTH,  XKB_KEY_E,          NEXT,
                        KEY_L,         BOTH,  XKB_KEY_L,          NEXT,
                        KEY_L,         BOTH,  XKB_KEY_L,          NEXT,
                        KEY_O,         BOTH,  XKB_KEY_O,          FINISH));

    assert(test_key_seq(keymap,
                        KEY_H,         BOTH,  XKB_KEY_h,          NEXT,
                        KEY_E,         BOTH,  XKB_KEY_e,          NEXT,
                        KEY_CAPSLOCK,  BOTH,  XKB_KEY_Caps_Lock,  NEXT,
                        KEY_L,         BOTH,  XKB_KEY_L,          NEXT,
                        KEY_L,         BOTH,  XKB_KEY_L,          NEXT,
                        KEY_CAPSLOCK,  BOTH,  XKB_KEY_Caps_Lock,  NEXT,
                        KEY_O,         BOTH,  XKB_KEY_o,          FINISH));

    assert(test_key_seq(keymap,
                        KEY_H,         BOTH,  XKB_KEY_h,          NEXT,
                        KEY_CAPSLOCK,  DOWN,  XKB_KEY_Caps_Lock,  NEXT,
                        KEY_E,         BOTH,  XKB_KEY_E,          NEXT,
                        KEY_L,         BOTH,  XKB_KEY_L,          NEXT,
                        KEY_L,         BOTH,  XKB_KEY_L,          NEXT,
                        KEY_CAPSLOCK,  UP,    XKB_KEY_Caps_Lock,  NEXT,
                        KEY_O,         BOTH,  XKB_KEY_O,          FINISH));

    assert(test_key_seq(keymap,
                        KEY_H,         BOTH,  XKB_KEY_h,          NEXT,
                        KEY_E,         BOTH,  XKB_KEY_e,          NEXT,
                        KEY_CAPSLOCK,  UP,    XKB_KEY_Caps_Lock,  NEXT,
                        KEY_L,         BOTH,  XKB_KEY_l,          NEXT,
                        KEY_L,         BOTH,  XKB_KEY_l,          NEXT,
                        KEY_O,         BOTH,  XKB_KEY_o,          FINISH));

    /*
     * A key release affecting a locked modifier should clear it
     * regardless of the key press.
     */
    /* assert(test_key_seq(keymap, */
    /*                     KEY_H,         BOTH,  XKB_KEY_h,          NEXT, */
    /*                     KEY_CAPSLOCK,  DOWN,  XKB_KEY_Caps_Lock,  NEXT, */
    /*                     KEY_E,         BOTH,  XKB_KEY_E,          NEXT, */
    /*                     KEY_L,         BOTH,  XKB_KEY_L,          NEXT, */
    /*                     KEY_CAPSLOCK,  UP,    XKB_KEY_Caps_Lock,  NEXT, */
    /*                     KEY_L,         BOTH,  XKB_KEY_L,          NEXT, */
    /*                     KEY_CAPSLOCK,  UP,    XKB_KEY_Caps_Lock,  NEXT, */
    /*                     KEY_O,         BOTH,  XKB_KEY_o,          FINISH)); */

    /* Simple Num Lock sanity check. */
    assert(test_key_seq(keymap,
                        KEY_KP1,      BOTH,  XKB_KEY_KP_End,    NEXT,
                        KEY_NUMLOCK,  BOTH,  XKB_KEY_Num_Lock,  NEXT,
                        KEY_KP1,      BOTH,  XKB_KEY_KP_1,      NEXT,
                        KEY_KP2,      BOTH,  XKB_KEY_KP_2,      NEXT,
                        KEY_NUMLOCK,  BOTH,  XKB_KEY_Num_Lock,  NEXT,
                        KEY_KP2,      BOTH,  XKB_KEY_KP_Down,   FINISH));

    /* Test that the aliases in the ru(phonetic) symbols map work. */
    assert(test_key_seq(keymap,
                        KEY_COMPOSE,     BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_COMPOSE,     BOTH,  XKB_KEY_ISO_Next_Group,  NEXT,
                        KEY_1,           BOTH,  XKB_KEY_1,               NEXT,
                        KEY_Q,           BOTH,  XKB_KEY_Cyrillic_ya,     NEXT,
                        KEY_LEFTSHIFT,   DOWN,  XKB_KEY_Shift_L,         NEXT,
                        KEY_1,           BOTH,  XKB_KEY_exclam,          NEXT,
                        KEY_Q,           BOTH,  XKB_KEY_Cyrillic_YA,     NEXT,
                        KEY_LEFTSHIFT,   UP,    XKB_KEY_Shift_L,         NEXT,
                        KEY_V,           BOTH,  XKB_KEY_Cyrillic_zhe,    NEXT,
                        KEY_CAPSLOCK,    BOTH,  XKB_KEY_Caps_Lock,       NEXT,
                        KEY_1,           BOTH,  XKB_KEY_1,               NEXT,
                        KEY_V,           BOTH,  XKB_KEY_Cyrillic_ZHE,    NEXT,
                        KEY_RIGHTSHIFT,  DOWN,  XKB_KEY_Shift_R,         NEXT,
                        KEY_V,           BOTH,  XKB_KEY_Cyrillic_zhe,    NEXT,
                        KEY_RIGHTSHIFT,  UP,    XKB_KEY_Shift_R,         NEXT,
                        KEY_V,           BOTH,  XKB_KEY_Cyrillic_ZHE,    FINISH));

#define KS(name) xkb_keysym_from_name(name, 0)

    /* Test that levels (1-5) in de(neo) symbols map work. */
    assert(test_key_seq(keymap,
                        /* Switch to the group. */
                        KEY_COMPOSE,     BOTH,  XKB_KEY_ISO_Next_Group,    NEXT,
                        KEY_COMPOSE,     BOTH,  XKB_KEY_ISO_Next_Group,    NEXT,
                        KEY_COMPOSE,     BOTH,  XKB_KEY_ISO_Next_Group,    NEXT,

                        /* Level 1. */
                        KEY_1,           BOTH,  XKB_KEY_1,                 NEXT,
                        KEY_Q,           BOTH,  XKB_KEY_x,                 NEXT,
                        KEY_KP7,         BOTH,  XKB_KEY_KP_7,              NEXT,
                        KEY_ESC,         BOTH,  XKB_KEY_Escape,            NEXT,

                        /* Level 2 with Shift. */
                        KEY_LEFTSHIFT,   DOWN,  XKB_KEY_Shift_L,           NEXT,
                        KEY_1,           BOTH,  XKB_KEY_degree,            NEXT,
                        KEY_Q,           BOTH,  XKB_KEY_X,                 NEXT,
                        KEY_KP7,         BOTH,  KS("U2714"),               NEXT,
                        KEY_ESC,         BOTH,  XKB_KEY_Escape,            NEXT,
                        /*
                         * XXX: de(neo) uses shift(both_capslock) which causes
                         * the interesting result in the next line. Since it's
                         * a key release, it doesn't actually lock the modifier,
                         * and applications by-and-large ignore the keysym on
                         * release(?). Is this a problem?
                         */
                        KEY_LEFTSHIFT,   UP,    XKB_KEY_Caps_Lock,         NEXT,

                        /* Level 2 with the Lock modifier. */
                        KEY_LEFTSHIFT,   DOWN,  XKB_KEY_Shift_L,           NEXT,
                        KEY_RIGHTSHIFT,  BOTH,  XKB_KEY_Caps_Lock,         NEXT,
                        KEY_LEFTSHIFT,   UP,    XKB_KEY_Caps_Lock,         NEXT,
                        KEY_6,           BOTH,  XKB_KEY_6,                 NEXT,
                        KEY_H,           BOTH,  XKB_KEY_S,                 NEXT,
                        KEY_KP3,         BOTH,  XKB_KEY_KP_3,              NEXT,
                        KEY_ESC,         BOTH,  XKB_KEY_Escape,            NEXT,
                        KEY_LEFTSHIFT,   DOWN,  XKB_KEY_Shift_L,           NEXT,
                        KEY_RIGHTSHIFT,  BOTH,  XKB_KEY_Caps_Lock,         NEXT,
                        KEY_LEFTSHIFT,   UP,    XKB_KEY_Caps_Lock,         NEXT,

                        /* Level 3. */
                        KEY_CAPSLOCK,    DOWN,  XKB_KEY_ISO_Level3_Shift,  NEXT,
                        KEY_6,           BOTH,  XKB_KEY_cent,              NEXT,
                        KEY_Q,           BOTH,  XKB_KEY_ellipsis,          NEXT,
                        KEY_KP7,         BOTH,  KS("U2195"),               NEXT,
                        KEY_ESC,         BOTH,  XKB_KEY_Escape,            NEXT,
                        KEY_CAPSLOCK,    UP,    XKB_KEY_ISO_Level3_Shift,  NEXT,

                        /* Level 4. */
                        KEY_CAPSLOCK,    DOWN,  XKB_KEY_ISO_Level3_Shift,  NEXT,
                        KEY_LEFTSHIFT,   DOWN,  XKB_KEY_Shift_L,           NEXT,
                        KEY_5,           BOTH,  XKB_KEY_malesymbol,        NEXT,
                        KEY_E,           BOTH,  XKB_KEY_Greek_lambda,      NEXT,
                        KEY_SPACE,       BOTH,  XKB_KEY_nobreakspace,      NEXT,
                        KEY_KP8,         BOTH,  XKB_KEY_intersection,      NEXT,
                        KEY_ESC,         BOTH,  XKB_KEY_Escape,            NEXT,
                        KEY_LEFTSHIFT,   UP,    XKB_KEY_Caps_Lock,         NEXT,
                        KEY_CAPSLOCK,    UP,    XKB_KEY_ISO_Level3_Shift,  NEXT,

                        /* Level 5. */
                        KEY_RIGHTALT,    DOWN,  XKB_KEY_ISO_Level5_Shift,  NEXT,
                        /* XXX: xkeyboard-config is borked when de(neo) is
                         *      not the first group - not our fault. We test
                         *      Level5 separately below with only de(neo). */
                        /* KEY_5,           BOTH,  XKB_KEY_periodcentered,    NEXT, */
                        /* KEY_E,           BOTH,  XKB_KEY_Up,                NEXT, */
                        /* KEY_SPACE,       BOTH,  XKB_KEY_KP_0,              NEXT, */
                        /* KEY_KP8,         BOTH,  XKB_KEY_KP_Up,             NEXT, */
                        KEY_ESC,         BOTH,  XKB_KEY_Escape,            NEXT,
                        KEY_RIGHTALT,    UP,    XKB_KEY_ISO_Level5_Shift,  NEXT,

                        KEY_V,           BOTH,  XKB_KEY_p,               FINISH));

    xkb_keymap_unref(keymap);
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, rules,
                                "", "de", "neo", "");
    assert(keymap);
    assert(test_key_seq(keymap,
                        /* Level 5. */
                        KEY_RIGHTALT,    DOWN,  XKB_KEY_ISO_Level5_Shift,  NEXT,
                        KEY_5,           BOTH,  XKB_KEY_periodcentered,    NEXT,
                        KEY_E,           BOTH,  XKB_KEY_Up,                NEXT,
                        KEY_SPACE,       BOTH,  XKB_KEY_KP_0,              NEXT,
                        KEY_KP8,         BOTH,  XKB_KEY_KP_Up,             NEXT,
                        KEY_ESC,         BOTH,  XKB_KEY_Escape,            NEXT,
                        KEY_RIGHTALT,    UP,    XKB_KEY_ISO_Level5_Lock,   NEXT,

                        /* Level 6. */
                        KEY_RIGHTALT,    DOWN,  XKB_KEY_ISO_Level5_Shift,  NEXT,
                        KEY_RIGHTSHIFT,  DOWN,  XKB_KEY_Shift_R,           NEXT,
                        KEY_5,           BOTH,  XKB_KEY_NoSymbol,          NEXT,
                        KEY_8,           BOTH,  XKB_KEY_ISO_Left_Tab,      NEXT,
                        KEY_E,           BOTH,  XKB_KEY_Up,                NEXT,
                        KEY_SPACE,       BOTH,  XKB_KEY_KP_0,              NEXT,
                        KEY_KP8,         BOTH,  XKB_KEY_KP_Up,             NEXT,
                        KEY_ESC,         BOTH,  XKB_KEY_Escape,            NEXT,
                        KEY_RIGHTSHIFT,  UP,    XKB_KEY_Caps_Lock,         NEXT,
                        KEY_RIGHTALT,    UP,    XKB_KEY_ISO_Level5_Lock,   NEXT,

                        /* Level 7. */
                        KEY_RIGHTALT,    DOWN,  XKB_KEY_ISO_Level5_Shift,  NEXT,
                        KEY_CAPSLOCK,    DOWN,  XKB_KEY_ISO_Level3_Shift,  NEXT,
                        KEY_5,           BOTH,  KS("U2221"),               NEXT,
                        KEY_E,           BOTH,  XKB_KEY_Greek_LAMBDA,      NEXT,
                        KEY_SPACE,       BOTH,  KS("U202F"),               NEXT,
                        KEY_KP8,         BOTH,  KS("U22C2"),               NEXT,
                        KEY_ESC,         BOTH,  XKB_KEY_Escape,            NEXT,
                        KEY_CAPSLOCK,    UP,    XKB_KEY_ISO_Level3_Shift,  NEXT,
                        KEY_RIGHTALT,    UP,    XKB_KEY_ISO_Level5_Lock,   NEXT,

                        /* Level 8. */
                        KEY_RIGHTALT,    DOWN,  XKB_KEY_ISO_Level5_Shift,  NEXT,
                        KEY_CAPSLOCK,    DOWN,  XKB_KEY_ISO_Level3_Shift,  NEXT,
                        KEY_RIGHTSHIFT,  DOWN,  XKB_KEY_Shift_R,           NEXT,
                        KEY_TAB,         BOTH,  XKB_KEY_ISO_Level5_Lock,   NEXT,
                        KEY_V,           BOTH,  XKB_KEY_Greek_pi,          NEXT,
                        KEY_RIGHTSHIFT,  UP,    XKB_KEY_Caps_Lock,         NEXT,
                        KEY_V,           BOTH,  XKB_KEY_asciitilde,        NEXT,
                        KEY_CAPSLOCK,    UP,    XKB_KEY_ISO_Level3_Shift,  NEXT,
                        KEY_V,           BOTH,  XKB_KEY_p,                 NEXT,
                        KEY_RIGHTALT,    UP,    XKB_KEY_ISO_Level5_Lock,   NEXT,
                        /* Locks Level 5. */

                        KEY_V,           BOTH,  XKB_KEY_Return,            FINISH));


    xkb_keymap_unref(keymap);
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, rules,
                                "", "us,il,ru", "",
                                "grp:alt_shift_toggle_bidir,grp:menu_toggle");
    assert(keymap);

    assert(test_key_seq(keymap,
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L,        NEXT,
                        KEY_LEFTALT,   DOWN, XKB_KEY_ISO_Prev_Group, NEXT,
                        KEY_LEFTALT,   UP,   XKB_KEY_ISO_Prev_Group, NEXT,
                        KEY_LEFTSHIFT, UP,   XKB_KEY_Shift_L,        FINISH));

    assert(test_key_seq(keymap,
                        KEY_LEFTALT,   DOWN, XKB_KEY_Alt_L,          NEXT,
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_ISO_Prev_Group, NEXT,
                        KEY_LEFTSHIFT, UP,   XKB_KEY_ISO_Prev_Group, NEXT,
                        KEY_LEFTALT,   UP,   XKB_KEY_Alt_L,          FINISH));

    /* Check backwards (negative) group switching and wrapping. */
    assert(test_key_seq(keymap,
                        KEY_H,         BOTH, XKB_KEY_h,              NEXT,
                        KEY_COMPOSE,   BOTH, XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_H,         BOTH, XKB_KEY_hebrew_yod,     NEXT,
                        KEY_COMPOSE,   BOTH, XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_H,         BOTH, XKB_KEY_Cyrillic_er,    NEXT,
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L,        NEXT,
                        KEY_LEFTALT,   BOTH, XKB_KEY_ISO_Prev_Group, NEXT,
                        KEY_LEFTSHIFT, UP,   XKB_KEY_Shift_L,        NEXT,
                        KEY_H,         BOTH, XKB_KEY_hebrew_yod,     NEXT,
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L,        NEXT,
                        KEY_LEFTALT,   BOTH, XKB_KEY_ISO_Prev_Group, NEXT,
                        KEY_LEFTSHIFT, UP,   XKB_KEY_Shift_L,        NEXT,
                        KEY_H,         BOTH, XKB_KEY_h,              NEXT,
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L,        NEXT,
                        KEY_LEFTALT,   BOTH, XKB_KEY_ISO_Prev_Group, NEXT,
                        KEY_LEFTSHIFT, UP,   XKB_KEY_Shift_L,        NEXT,
                        KEY_H,         BOTH, XKB_KEY_Cyrillic_er,    NEXT,
                        KEY_COMPOSE,   BOTH, XKB_KEY_ISO_Next_Group, NEXT,
                        KEY_H,         BOTH, XKB_KEY_h,              FINISH));

    xkb_keymap_unref(keymap);
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, rules,
                                 "", "us,il,ru", "",
                                 "grp:switch,grp:lswitch,grp:menu_toggle");
    assert(keymap);

    /* Test depressed group works (Mode_switch). */
    assert(test_key_seq(keymap,
                        KEY_H,         BOTH, XKB_KEY_h,                 NEXT,
                        KEY_RIGHTALT,  DOWN, XKB_KEY_Mode_switch,       NEXT,
                        KEY_H,         BOTH, XKB_KEY_hebrew_yod,        NEXT,
                        KEY_RIGHTALT,  UP,   XKB_KEY_ISO_Level3_Shift,  NEXT,
                        KEY_H,         BOTH, XKB_KEY_h,                 NEXT,
                        KEY_RIGHTALT,  DOWN, XKB_KEY_Mode_switch,       NEXT,
                        KEY_H,         BOTH, XKB_KEY_hebrew_yod,        NEXT,
                        KEY_RIGHTALT,  UP,   XKB_KEY_ISO_Level3_Shift,  NEXT,
                        KEY_H,         BOTH, XKB_KEY_h,                 FINISH));

    /* Test locked+depressed group works, with wrapping and accumulation. */
    assert(test_key_seq(keymap,
                        KEY_H,         BOTH, XKB_KEY_h,                 NEXT,
                        KEY_COMPOSE,   BOTH, XKB_KEY_ISO_Next_Group,    NEXT,
                        KEY_LEFTALT,   DOWN, XKB_KEY_Mode_switch,       NEXT,
                        KEY_H,         BOTH, XKB_KEY_Cyrillic_er,       NEXT,
                        KEY_LEFTALT,   UP,   XKB_KEY_Mode_switch,       NEXT,
                        KEY_H,         BOTH, XKB_KEY_hebrew_yod,        NEXT,
                        KEY_COMPOSE,   BOTH, XKB_KEY_ISO_Next_Group,    NEXT,
                        KEY_LEFTALT,   DOWN, XKB_KEY_Mode_switch,       NEXT,
                        /* Should wrap back to first group. */
                        KEY_H,         BOTH, XKB_KEY_h,                 NEXT,
                        KEY_LEFTALT,   UP,   XKB_KEY_Mode_switch,       NEXT,
                        KEY_H,         BOTH, XKB_KEY_Cyrillic_er,       NEXT,
                        KEY_COMPOSE,   BOTH, XKB_KEY_ISO_Next_Group,    NEXT,
                        KEY_H,         BOTH, XKB_KEY_h,                 NEXT,
                        /* Two SetGroup(+1)'s should add up. */
                        KEY_RIGHTALT,  DOWN, XKB_KEY_Mode_switch,       NEXT,
                        KEY_H,         BOTH, XKB_KEY_hebrew_yod,        NEXT,
                        KEY_LEFTALT,   DOWN, XKB_KEY_Mode_switch,       NEXT,
                        KEY_H,         BOTH, XKB_KEY_Cyrillic_er,       NEXT,
                        KEY_LEFTALT,   UP,   XKB_KEY_Mode_switch,       NEXT,
                        KEY_H,         BOTH, XKB_KEY_hebrew_yod,        NEXT,
                        KEY_RIGHTALT,  UP,   XKB_KEY_ISO_Level3_Shift,  NEXT,
                        KEY_H,         BOTH, XKB_KEY_h,                 FINISH));

    xkb_keymap_unref(keymap);
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, rules,
                                "", "us", "euro", "");
    assert(keymap);

    assert(test_key_seq(keymap,
                        KEY_5,         BOTH, XKB_KEY_5,                 NEXT,
                        KEY_RIGHTALT,  DOWN, XKB_KEY_ISO_Level3_Shift,  NEXT,
                        KEY_5,         BOTH, XKB_KEY_EuroSign,          NEXT,
                        KEY_RIGHTALT,  UP,   XKB_KEY_ISO_Level3_Shift,  FINISH));

    xkb_keymap_unref(keymap);
    keymap = test_compile_file(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "keymaps/unbound-vmod.xkb"
    );
    assert(keymap);

    assert(test_key_seq(keymap,
                        KEY_H,         BOTH, XKB_KEY_h,                 NEXT,
                        KEY_Z,         BOTH, XKB_KEY_y,                 NEXT,
                        KEY_MINUS,     BOTH, XKB_KEY_ssharp,            NEXT,
                        KEY_Z,         BOTH, XKB_KEY_y,                 FINISH));

    xkb_keymap_unref(keymap);
    keymap = test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, rules,
                                "applealu_ansi", "us", "",
                                "terminate:ctrl_alt_bksp");
    assert(keymap);

    assert(test_key_seq(keymap,
                        KEY_5,         BOTH, XKB_KEY_5,                 NEXT,
                        KEY_KP1,       BOTH, XKB_KEY_KP_1,              NEXT,
                        KEY_NUMLOCK,   BOTH, XKB_KEY_Clear,             NEXT,
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L,           NEXT,
                        KEY_KP1,       BOTH, XKB_KEY_KP_1,              NEXT,
                        KEY_LEFTSHIFT, UP,   XKB_KEY_Shift_L,           NEXT,
                        KEY_CAPSLOCK,  BOTH, XKB_KEY_Caps_Lock,         NEXT,
                        KEY_KP1,       BOTH, XKB_KEY_KP_1,              NEXT,
                        KEY_LEFTSHIFT, DOWN, XKB_KEY_Shift_L,           NEXT,
                        KEY_KP1,       BOTH, XKB_KEY_KP_1,              NEXT,
                        KEY_LEFTSHIFT, UP,   XKB_KEY_Shift_L,           NEXT,
                        KEY_CAPSLOCK,  BOTH, XKB_KEY_Caps_Lock,         NEXT,
                        KEY_A,         BOTH, XKB_KEY_a,                 FINISH));

    xkb_keymap_unref(keymap);
}

int
main(void)
{
    test_init();

    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    /* Usual rules */
    test_keymaps(ctx, "evdev");
    /* Special rules to make no use of modmaps */
    test_keymaps(ctx, "evdev-pure-virtual-mods");

    test_simultaneous_modifier_clear(ctx);
    test_group_lock(ctx);
    test_group_latch(ctx);
    test_mod_set(ctx);
    test_mod_lock(ctx);
    test_mod_latch(ctx);
    test_explicit_actions(ctx);

    xkb_context_unref(ctx);
    return EXIT_SUCCESS;
}
