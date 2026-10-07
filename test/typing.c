/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#if WCHAR_MAX >= 0x7FFFFFFF && HAVE_NEWLOCALE && HAVE_ISWPRINT_L
#define USE_ISWPRINT_L
#include <locale.h>
#include <wctype.h>
#endif

#include "utils.h"
#include "xkbcommon/xkbcommon.h"

#include "evdev-scancodes.h"
#include "src/darray.h"
#include "src/typing.h"
#include "test.h"
#if HAS_EVDEV_SCANCODES_LOOKUP
#include "evdev-scancodes-lookup.h"
#endif

static void
test_key_set(struct xkb_context *ctx)
{
    #ifdef USE_ISWPRINT_L
    locale_t loc = newlocale(LC_ALL_MASK, "C.UTF-8", (locale_t)0);
    if (loc != (locale_t)0) {
        assert(iswprint_l(0x000e4, loc)); // ä
        assert(iswprint_l(0x02000, loc)); // ‘ ’ EN QUAD
        assert(iswprint_l(0x02728, loc)); // ✨
        assert(iswprint_l(0x1F986, loc)); // 🦆
    }
    freelocale(loc);
    #endif

    static const xkb_keycode_t us_printable_keys[] = {
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,
        KEY_0,
        KEY_MINUS,
        KEY_EQUAL,
        // KEY_BACKSPACE,
    #ifdef _WIN32
        /* Windows has a buggy implementation of isprint() */
        KEY_TAB,
    #endif
        KEY_Q,
        KEY_W,
        KEY_E,
        KEY_R,
        KEY_T,
        KEY_Y,
        KEY_U,
        KEY_I,
        KEY_O,
        KEY_P,
        KEY_LEFTBRACE,
        KEY_RIGHTBRACE,
        // KEY_ENTER,
        KEY_A,
        KEY_S,
        KEY_D,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_SEMICOLON,
        KEY_APOSTROPHE,
        KEY_GRAVE,
        KEY_BACKSLASH,
        KEY_Z,
        KEY_X,
        KEY_C,
        KEY_V,
        KEY_B,
        KEY_N,
        KEY_M,
        KEY_COMMA,
        KEY_DOT,
        KEY_SLASH,
        KEY_KPASTERISK,
        KEY_SPACE,
        KEY_KP7,
        KEY_KP8,
        KEY_KP9,
        KEY_KPMINUS,
        KEY_KP4,
        KEY_KP5,
        KEY_KP6,
        KEY_KPPLUS,
        KEY_KP1,
        KEY_KP2,
        KEY_KP3,
        KEY_KP0,
        KEY_KPDOT,
        KEY_102ND,
        KEY_KPSLASH,
        KEY_KPEQUAL,
        KEY_KPCOMMA,
    };
    static const struct xkb_modifier_key us_modifiers_keys[] = {
        { KEY_LEFTCTRL, 0x04, XKB_MOD_SET },
        { KEY_LEFTSHIFT, 0x01, XKB_MOD_SET },
        { KEY_RIGHTSHIFT, 0x01, XKB_MOD_SET },
        { KEY_LEFTALT, 0x08, XKB_MOD_SET },
        { KEY_CAPSLOCK, 0x02, XKB_MOD_LOCK },
        { KEY_NUMLOCK, 0x10, XKB_MOD_LOCK },
        { KEY_LVL3, 0x80, XKB_MOD_SET },
        // { KEY_SCROLLLOCK, },
        { KEY_RIGHTCTRL, 0x0004, XKB_MOD_SET },
        { KEY_RIGHTALT, 0x08, XKB_MOD_SET },
        { KEY_LEFTMETA, 0x40, XKB_MOD_SET },
        { KEY_RIGHTMETA, 0x040, XKB_MOD_SET },
        // { KEY_LVL5, },
        // { KEY_ALT, },
        // { META, },
        // { SUPR, },
        // { HYPR, },
    };
    static const xkb_keycode_t us_misc_keys[] = {
        KEY_ESC,
        KEY_BACKSPACE,
    #ifndef _WIN32
        /* Windows has a buggy implementation of isprint() */
        KEY_TAB,
    #endif
        KEY_ENTER,
        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_SCROLLLOCK,
        // KEY_ZENKAKUHANKAKU,
        KEY_F11,
        KEY_F12,
        // KEY_RO,
        // KEY_KATAKANA,
        // KEY_HIRAGANA,
        // KEY_HENKAN,
        // KEY_KATAKANAHIRAGANA,
        // KEY_MUHENKAN,
        // KEY_KPJPCOMMA,
        KEY_KPENTER,
        KEY_SYSRQ,
        // KEY_LINEFEED,
        KEY_HOME,
        KEY_UP,
        KEY_PAGEUP,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_END,
        KEY_DOWN,
        KEY_PAGEDOWN,
        KEY_INSERT,
        KEY_DELETE,
        // KEY_MACRO,
        // KEY_MUTE,
        // KEY_VOLUMEDOWN,
        // KEY_VOLUMEUP,
        // KEY_POWER,
        // KEY_KPEQUAL,
        KEY_PAUSE,
        // KEY_SCALE,
        // KEY_KPCOMMA,
        // KEY_HANGEUL,
        // KEY_HANGUEL,
        // KEY_HANJA,
        // KEY_YEN,
        KEY_COMPOSE,
        KEY_LVL5,
        KEY_ALT,
        KEY_META,
        KEY_SUPR,
        KEY_HYPR,
    };

    enum { KEY_COUNT = 256 };
    static const struct {
        const char *keymap;
        const xkb_keycode_t *printable;
        const struct xkb_modifier_key *modifiers;
        const xkb_keycode_t *misc;
        darray_size_t num_printable;
        darray_size_t num_modifiers;
        darray_size_t num_misc;
    } tests[] = {
        {
            .keymap = "xkb_keymap {};",
            .num_printable = 0,
            .num_modifiers = 0,
            .num_misc = 0,
        },
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_keycodes { include \"evdev\" };\n"
                "  xkb_compat { include \"complete\" };\n"
                "  xkb_types { include \"complete\" };\n"
                "  xkb_symbols { include \"pc+us\" };\n"
                "};",
            .printable = us_printable_keys,
            .num_printable = ARRAY_SIZE(us_printable_keys),
            .modifiers = us_modifiers_keys,
            .num_modifiers = ARRAY_SIZE(us_modifiers_keys),
            .misc = us_misc_keys,
            .num_misc = ARRAY_SIZE(us_misc_keys),
        }
    };

    for (size_t t = 0; t < ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: #%zu ***\n", __func__, t);
        struct xkb_keymap *keymap =
            test_compile_buffer(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                                tests[t].keymap, strlen(tests[t].keymap));
        assert(keymap);

        static const struct xkb_key_set_config config = {
            .min = 0,
            .max = KEY_COUNT - 1,
        };
        struct xkb_key_set set = {0};
        const enum xkb_status status = xkb_key_set_init(&set, keymap, &config);
        assert(status == XKB_SUCCESS);

        if (tests[t].num_printable != darray_size(set.printable)) {
            fprintf(stderr, "Expected (printable):");
            for (darray_size_t k = 0; k < tests[t].num_printable; k++) {
                fprintf(stderr, " %"PRIu32, tests[t].printable[k] + EVDEV_OFFSET);
            }
            fprintf(stderr, "\nGot (printable):     ");
            for (darray_size_t k = 0; k < darray_size(set.printable); k++) {
                fprintf(stderr, " %"PRIu32, darray_item(set.printable, k));
            }
            fprintf(stderr, "\n");
            assert_eq("printable size", tests[t].num_printable,
                      darray_size(set.printable), "%u");
        }

        assert_eq("modifiers size", tests[t].num_modifiers,
                  darray_size(set.modifiers), "%u");

        if (tests[t].num_misc != darray_size(set.misc)) {
            fprintf(stderr, "Expected (misc):");
            for (darray_size_t k = 0; k < tests[t].num_misc; k++) {
                fprintf(stderr, " %"PRIu32, tests[t].misc[k] + EVDEV_OFFSET);
            }
            fprintf(stderr, "\nGot (misc):     ");
            for (darray_size_t k = 0; k < darray_size(set.misc); k++) {
                fprintf(stderr, " %"PRIu32, darray_item(set.misc, k));
            }
            fprintf(stderr, "\n");
            assert_eq("misc size", tests[t].num_misc,
                    darray_size(set.misc), "%u");
        }

        for (darray_size_t k = 0; k <  tests[t].num_printable; k++) {
            assert_eq("printable key #%u",
                      tests[t].printable[k] + EVDEV_OFFSET,
                      darray_item(set.printable, k),
                      "%"PRIu32, k);
        }

        for (darray_size_t k = 0; k <  tests[t].num_modifiers; k++) {
            assert_eq("modifier key #%u (keycode)",
                      tests[t].modifiers[k].keycode + EVDEV_OFFSET,
                      darray_item(set.modifiers, k).keycode,
                      "%"PRIu32, k);
            assert_eq("modifier key #%u (mods)",
                      tests[t].modifiers[k].mods,
                      darray_item(set.modifiers, k).mods,
                      "%d", k);
            assert_eq("modifier key #%u (kind)",
                      tests[t].modifiers[k].kind,
                      darray_item(set.modifiers, k).kind,
                      "%d", k);
        }

        for (darray_size_t k = 0; k <  tests[t].num_misc; k++) {
            assert_eq("misc key #%u",
                      tests[t].misc[k] + EVDEV_OFFSET,
                      darray_item(set.misc, k),
                      "%"PRIu32, k);
        }

        xkb_key_set_destroy(&set);

        xkb_keymap_unref(keymap);
    }
}

/**
 * FNV-1a hash
 * See: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
 */
static uint64_t
fnv_1a(const char *str, size_t length)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    while (*str) {
        hash ^= (uint8_t)(*str++);
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

/** Donald Knuth's MMIX */
static long
mmix_prng(void *state)
{
    uint64_t *state_priv = (uint64_t*)state;
    *state_priv = *state_priv * UINT64_C(6364136223846793005)
                + UINT64_C(1442695040888963407);
    /* Extract the top 31 bits */
    return (long)(*state_priv >> 33);
}

static void
test_typing(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap = test_compile_rules(
        ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
        "evdev", "pc105", "us", "", ""
    );
    assert(keymap);

    struct xkb_key_set set;
    struct xkb_key_set_config set_config = {
        .min = 0,
        .max = KEY_RIGHTMETA + 1,
    };
    assert(xkb_key_set_init(&set, keymap, &set_config) ==
           XKB_SUCCESS);

    enum { INPUT_LENGTH = 42 };
    struct xkb_typing_input input;
    const char seed[] = "xkbcommon";
    uint64_t prng_state = fnv_1a(seed, sizeof(seed) - 1);
    struct xkb_typing_input_config input_config = {
        .prng = &mmix_prng,
        .prng_state = &prng_state,
        .input_length = INPUT_LENGTH,
    };

    struct xkb_typing_event tests[][INPUT_LENGTH] = {
        {
            { KEY_1, XKB_KEY_DOWN },
            { KEY_1, XKB_KEY_UP },
            { KEY_TAB, XKB_KEY_DOWN },
            { KEY_TAB, XKB_KEY_UP },
            { KEY_5, XKB_KEY_DOWN },
            { KEY_5, XKB_KEY_UP },
            { KEY_2, XKB_KEY_DOWN },
            { KEY_2, XKB_KEY_UP },
            { KEY_MINUS, XKB_KEY_DOWN },
            { KEY_MINUS, XKB_KEY_UP },
            { KEY_R, XKB_KEY_DOWN },
            { KEY_R, XKB_KEY_UP },
            { KEY_RIGHTCTRL, XKB_KEY_DOWN },
            { KEY_E, XKB_KEY_DOWN },
            { KEY_E, XKB_KEY_UP },
            { KEY_KPPLUSMINUS, XKB_KEY_DOWN },
            { KEY_RIGHTSHIFT, XKB_KEY_DOWN },
            { KEY_KPPLUSMINUS, XKB_KEY_UP },
            { KEY_RIGHTCTRL, XKB_KEY_UP },
            { KEY_F2, XKB_KEY_DOWN },
            { KEY_F2, XKB_KEY_UP },
            { KEY_P, XKB_KEY_DOWN },
            { KEY_P, XKB_KEY_UP },
            { KEY_LEFTALT, XKB_KEY_DOWN },
            { KEY_1, XKB_KEY_DOWN },
            { KEY_1, XKB_KEY_UP },
            { KEY_I, XKB_KEY_DOWN },
            { KEY_I, XKB_KEY_UP },
            { KEY_RIGHTSHIFT, XKB_KEY_UP },
            { KEY_RIGHTCTRL, XKB_KEY_DOWN },
            { KEY_S, XKB_KEY_DOWN },
            { KEY_S, XKB_KEY_UP },
            { KEY_LEFTALT, XKB_KEY_UP },
            { KEY_KP4, XKB_KEY_DOWN },
            { KEY_KP4, XKB_KEY_UP },
            { KEY_LVL3, XKB_KEY_DOWN },
            { KEY_W, XKB_KEY_DOWN },
            { KEY_SCROLLLOCK, XKB_KEY_DOWN },
            { KEY_SCROLLLOCK, XKB_KEY_UP },
            { KEY_W, XKB_KEY_UP },
            { KEY_RIGHTCTRL, XKB_KEY_UP },
            { KEY_LVL3, XKB_KEY_UP },
        },
    };

    for (size_t t = 0; t < ARRAY_SIZE(tests); t++) {
        fprintf(stderr, "------\n*** %s: #%zu ***\n", __func__, t);

        assert(xkb_typing_input_init(&input, &set, &input_config) ==
            XKB_SUCCESS);

        assert(input.num_events == ARRAY_SIZE(*tests));

        for (size_t e = 0; e < ARRAY_SIZE(tests[t]); e++) {
        #define UPDATE_ENTRIES 0
        #if UPDATE_ENTRIES
            const uint32_t evdev_scancode =
                input.events[e].keycode - EVDEV_OFFSET;
            const char *name = evdev_scancode_name(evdev_scancode);
            if (!name) {
                fprintf(stderr, "Code not found: %"PRIu32"\n",
                        evdev_scancode);
                assert(!"no evdev key name");
            }
            const char *direction =
                (input.events[e].direction == XKB_KEY_UP)
                    ? "XKB_KEY_UP"
                    : "XKB_KEY_DOWN";
            fprintf(stderr, "{ KEY_%s, %s },\n", name, direction);
        #else
            assert_eq("event #%zu (keycode)",
                      tests[t][e].keycode + EVDEV_OFFSET,
                      input.events[e].keycode,
                      "%"PRIu32, e);
            assert_eq("event #%zu (direction)",
                      tests[t][e].direction,
                      input.events[e].direction,
                      "%d", e);
        #endif
        }

        xkb_typing_input_destroy(&input);
    }

    xkb_key_set_destroy(&set);
    xkb_keymap_unref(keymap);
}

int
main(void)
{
    test_init();

    struct xkb_context * const ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    test_key_set(ctx);
#ifdef _WIN32
    /* no test: Windows has a buggy implementation of isprint() */
#else
    test_typing(ctx);
#endif

    xkb_context_unref(ctx);

    return EXIT_SUCCESS;
}
