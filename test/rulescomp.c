/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include <assert.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon.h"

#include "evdev-scancodes.h"
#include "src/keymap.h"
#include "src/keysym.h"
#include "test.h"
#include "utils.h"

/* xkb_rule_names API */
static int
test_rmlvo_va(struct xkb_context *context, enum xkb_keymap_format format,
              const char *rules, const char *model, const char *layout,
              const char *variant, const char *options, va_list ap)
{
    struct xkb_keymap * const keymap =
        test_compile_rules(context, format, rules, model, layout, variant, options);
    if (!keymap)
        return 0;

    fprintf(stderr, "Compiled '%s' '%s' '%s' '%s' '%s'\n",
            strnull(rules), strnull(model), strnull(layout),
            strnull(variant), strnull(options));

    const int ret = test_key_seq_va(keymap, NULL, NULL, ap);

    xkb_keymap_unref(keymap);

    return ret;
}

/* xkb_rmlvo_builder API */
static int
test_rmlvo_builder_va(struct xkb_context *context, enum xkb_keymap_format format,
                      const char *rules, const char *model, const char *layout,
                      const char *variant, const char *options, va_list ap)
{
    struct xkb_keymap * const keymap =
        test_compile_rmlvo(context, format, rules, model, layout, variant, options);
    if (!keymap)
        return 0;

    fprintf(stderr, "Compiled '%s' '%s' '%s' '%s' '%s'\n",
            strnull(rules), strnull(model), strnull(layout),
            strnull(variant), strnull(options));

    const int ret = test_key_seq_va(keymap, NULL, NULL, ap);

    xkb_keymap_unref(keymap);

    return ret;
}

static int
test_rmlvo(struct xkb_context *context, enum xkb_keymap_format format,
           const char *rules, const char *model, const char *layout,
           const char *variant, const char *options, ...)
{
    va_list ap;
    int ret;

    va_start(ap, options);
    ret = test_rmlvo_va(context, format, rules, model, layout, variant, options, ap);
    va_end(ap);

    va_start(ap, options);
    ret &= test_rmlvo_builder_va(context, format,
                                 rules, model, layout, variant, options, ap);
    va_end(ap);

    return ret;
}

static int
test_rmlvo_env(struct xkb_context *ctx, enum xkb_keymap_format format,
               const char *rules, const char *model, const char *layout,
               const char *variant, const char *options, ...)
{
    va_list ap;
    int ret;

    va_start (ap, options);

    if (!isempty(rules))
        setenv("XKB_DEFAULT_RULES", rules, 1);
    else
        unsetenv("XKB_DEFAULT_RULES");

    if (!isempty(model))
        setenv("XKB_DEFAULT_MODEL", model, 1);
    else
        unsetenv("XKB_DEFAULT_MODEL");

    if (!isempty(layout))
        setenv("XKB_DEFAULT_LAYOUT", layout, 1);
    else
        unsetenv("XKB_DEFAULT_LAYOUT");

    if (!isempty(variant))
        setenv("XKB_DEFAULT_VARIANT", variant, 1);
    else
        unsetenv("XKB_DEFAULT_VARIANT");

    if (!isempty(options))
        setenv("XKB_DEFAULT_OPTIONS", options, 1);
    else
        unsetenv("XKB_DEFAULT_OPTIONS");

    ret = test_rmlvo_va(ctx, format, NULL, NULL, NULL, NULL, NULL, ap);

    va_end(ap);

    return ret;
}

/* Test more than 4 groups */
static void
test_extended_groups(struct xkb_context *ctx)
{
    struct {
        enum xkb_keymap_format format;
        xkb_layout_index_t num_layouts;
        const char *layouts;
    } tests[] = {
        /* v1: 4 groups */
        {
            .format = XKB_KEYMAP_FORMAT_TEXT_V1,
            .layouts = "cz,us,ca,de",
            .num_layouts = XKB_MAX_GROUPS_X11
        },
        /* v1: 5 groups, discard 1 group */
        {
            .format = XKB_KEYMAP_FORMAT_TEXT_V1,
            .layouts = "cz,us,ca,de,in",
            .num_layouts = XKB_MAX_GROUPS_X11
        },
        /* v2: 5 groups */
        {
            .format = XKB_KEYMAP_FORMAT_TEXT_V2,
            .layouts = "cz,us,ca,de,in",
            .num_layouts = XKB_MAX_GROUPS_X11 + 1
        },
        /* v2: 32 groups */
        {
            .format = XKB_KEYMAP_FORMAT_TEXT_V2,
            .layouts = "cz,us,ca,de,in,cz,us,ca,de,in,cz,us,ca,de,in,"
                       "cz,us,ca,de,in,cz,us,ca,de,in,cz,us,ca,de,in,"
                       "cz,us",
            .num_layouts = XKB_MAX_GROUPS
        },
        /* v2: 33 groups, discard 1 group */
        {
            .format = XKB_KEYMAP_FORMAT_TEXT_V2,
            .layouts = "cz,us,ca,de,in,cz,us,ca,de,in,cz,us,ca,de,in,"
                       "cz,us,ca,de,in,cz,us,ca,de,in,cz,us,ca,de,in,"
                       "cz,us,ca",
            .num_layouts = XKB_MAX_GROUPS
        },
    };

    for (size_t k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%zu ***\n", __func__, k);
        struct xkb_keymap *keymap =
            test_compile_rules(ctx, tests[k].format, "evdev-modern",
                               "pc105", tests[k].layouts, NULL, NULL);
        assert(keymap);
        assert(xkb_keymap_num_layouts(keymap) == tests[k].num_layouts);
        xkb_keymap_unref(keymap);
    }

#define U(cp) (XKB_KEYSYM_UNICODE_OFFSET + (cp))
    assert(test_rmlvo_env(ctx, XKB_KEYMAP_FORMAT_TEXT_V2, "evdev-modern",
                          "", "cz,us,ca,de,in,ru,il", ",,,,,phonetic,",
                          "grp:menu_toggle",
                          KEY_2,          BOTH, XKB_KEY_ecaron,           NEXT,
                          KEY_COMPOSE,    BOTH, XKB_KEY_ISO_Next_Group,   NEXT,
                          KEY_Y,          BOTH, XKB_KEY_y,                NEXT,
                          KEY_COMPOSE,    BOTH, XKB_KEY_ISO_Next_Group,   NEXT,
                          KEY_102ND,      BOTH, XKB_KEY_guillemetleft,    NEXT,
                          KEY_COMPOSE,    BOTH, XKB_KEY_ISO_Next_Group,   NEXT,
                          KEY_Y,          BOTH, XKB_KEY_z,                NEXT,
                          KEY_COMPOSE,    BOTH, XKB_KEY_ISO_Next_Group,   NEXT,
                          KEY_Y,          BOTH, U(0x092c),                NEXT,
                          KEY_COMPOSE,    BOTH, XKB_KEY_ISO_Next_Group,   NEXT,
                          KEY_Y,          BOTH, XKB_KEY_Cyrillic_ze,      NEXT,
                          KEY_COMPOSE,    BOTH, XKB_KEY_ISO_Next_Group,   NEXT,
                          KEY_Y,          BOTH, XKB_KEY_hebrew_tet,       FINISH));
#undef U
}

int
main(int argc, char *argv[])
{
    test_init();

    struct xkb_context *ctx = test_get_context(CONTEXT_ALLOW_ENVIRONMENT_NAMES);
    assert(ctx);

    /* Reject invalid flags */
    assert(!xkb_rmlvo_builder_new(ctx, NULL, NULL, -1));
    assert(!xkb_rmlvo_builder_new(ctx, NULL, NULL, 0xffff));
    static const struct xkb_rule_names rmlvo = { NULL };
    assert(!xkb_keymap_new_from_names2(ctx, &rmlvo,
                                        XKB_KEYMAP_FORMAT_TEXT_V1, -1));
    assert(!xkb_keymap_new_from_names2(ctx, &rmlvo,
                                        XKB_KEYMAP_FORMAT_TEXT_V1, 5453));

    /* Test “Last” group constant as an array index */
    struct xkb_keymap *keymap =
        test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev-modern",
                           "pc105", "last-group", NULL, NULL);
    assert(keymap);
    assert(xkb_keymap_num_layouts(keymap) == 1);
    const xkb_keysym_t *syms = NULL;
    int nsyms = xkb_keymap_key_get_syms_by_level(
        keymap, KEY_Q + EVDEV_OFFSET, 0, 0, &syms
    );
    assert(nsyms == 1);
    assert(XKB_KEY_a == syms[0]); /* `Last` works: there is only one group */
    xkb_keymap_unref(keymap);

    keymap =
        test_compile_rules(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev-modern",
                           "pc105", "last-group,us", NULL, NULL);
    assert(keymap);
    assert(xkb_keymap_num_layouts(keymap) == 2);
    syms = NULL;
    nsyms = xkb_keymap_key_get_syms_by_level(
        keymap, KEY_Q + EVDEV_OFFSET, 0, 0, &syms
    );
    assert(nsyms == 0); /* `Last` does not work: there are multiple groups */
    nsyms = xkb_keymap_key_get_syms_by_level(
        keymap, KEY_Q + EVDEV_OFFSET, 1, 0, &syms
    );
    assert(nsyms == 1); /* Layout 2 is not impacted */
    assert(XKB_KEY_q == syms[0]);
    xkb_keymap_unref(keymap);

#define KS(name) xkb_keysym_from_name(name, XKB_KEYSYM_NO_FLAGS)

    assert(test_rmlvo(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                      "pc105", "us,il,ru,ca", ",,,multix",
                      "grp:alts_toggle,ctrl:nocaps,compose:rwin",
                      KEY_Q,          BOTH, XKB_KEY_q,                    NEXT,
                      KEY_LEFTALT,    DOWN, XKB_KEY_Alt_L,                NEXT,
                      KEY_RIGHTALT,   DOWN, XKB_KEY_ISO_Next_Group,       NEXT,
                      KEY_RIGHTALT,   UP,   XKB_KEY_ISO_Level3_Shift,     NEXT,
                      KEY_LEFTALT,    UP,   XKB_KEY_Alt_L,                NEXT,
                      KEY_Q,          BOTH, XKB_KEY_slash,                NEXT,
                      KEY_LEFTSHIFT,  DOWN, XKB_KEY_Shift_L,              NEXT,
                      KEY_Q,          BOTH, XKB_KEY_Q,                    NEXT,
                      KEY_RIGHTMETA,  BOTH, XKB_KEY_Multi_key,            FINISH));
    assert(test_rmlvo(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                      "pc105", "us,in", "", "grp:alts_toggle",
                      KEY_A,          BOTH, XKB_KEY_a,                    NEXT,
                      KEY_LEFTALT,    DOWN, XKB_KEY_Alt_L,                NEXT,
                      KEY_RIGHTALT,   DOWN, XKB_KEY_ISO_Next_Group,       NEXT,
                      KEY_RIGHTALT,   UP,   XKB_KEY_ISO_Level3_Shift,     NEXT,
                      KEY_LEFTALT,    UP,   XKB_KEY_Alt_L,                NEXT,
                      KEY_A,          BOTH, KS("U094b"),                  FINISH));
    assert(test_rmlvo(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                      "pc105", "us", "intl", "",
                      KEY_GRAVE,      BOTH,  XKB_KEY_dead_grave,          FINISH));
    assert(test_rmlvo(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                      "pc105", "us", "intl", "grp:alts_toggle",
                      KEY_GRAVE,      BOTH,  XKB_KEY_dead_grave,          FINISH));

    /* 33 is not a legal group; make sure this is handled gracefully. */
#define EXCESSIVE_GROUPS 33
    static_assert(EXCESSIVE_GROUPS > XKB_MAX_GROUPS, "Test upgrade required");
    assert(test_rmlvo(ctx, XKB_KEYMAP_FORMAT_TEXT_V2, "evdev",
                      "", "us:" STRINGIFY(EXCESSIVE_GROUPS), "", "",
                      KEY_A,          BOTH, XKB_KEY_a,                    FINISH));
#undef EXCESSIVE_GROUPS

    /* Don't choke on missing values in RMLVO. Should just skip them.
       Currently generates us,us,ca. */
    assert(test_rmlvo(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                      "", "us,,ca", "", "grp:alts_toggle",
                      KEY_A,          BOTH, XKB_KEY_a,                    NEXT,
                      KEY_LEFTALT,    DOWN, XKB_KEY_Alt_L,                NEXT,
                      KEY_RIGHTALT,   DOWN, XKB_KEY_ISO_Next_Group,       NEXT,
                      KEY_RIGHTALT,   UP,   XKB_KEY_ISO_Next_Group,       NEXT,
                      KEY_LEFTALT,    UP,   XKB_KEY_Alt_L,                NEXT,
                      KEY_LEFTALT,    DOWN, XKB_KEY_Alt_L,                NEXT,
                      KEY_RIGHTALT,   DOWN, XKB_KEY_ISO_Next_Group,       NEXT,
                      KEY_RIGHTALT,   UP,   XKB_KEY_ISO_Level3_Shift,     NEXT,
                      KEY_LEFTALT,    UP,   XKB_KEY_Alt_L,                NEXT,
                      KEY_APOSTROPHE, BOTH, XKB_KEY_dead_grave,           FINISH));

    assert(test_rmlvo(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                      "", "", "", "", "",
                      KEY_A,          BOTH, XKB_KEY_a,                    FINISH));

    assert(!test_rmlvo(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                       "does-not-exist", "", "", "", "",
                       KEY_A,          BOTH, XKB_KEY_a,                   FINISH));

    assert(test_rmlvo_env(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                          "", "us", "", "",
                          KEY_A,          BOTH, XKB_KEY_a,                FINISH));
    assert(test_rmlvo_env(ctx, XKB_KEYMAP_FORMAT_TEXT_V1,
                          "evdev", "", "us", "", "ctrl:nocaps",
                          KEY_CAPSLOCK,   BOTH, XKB_KEY_Control_L,        FINISH));

    /* Ignores multix and generates us,ca. */
    assert(test_rmlvo_env(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                          "", "us,ca", ",,,multix", "grp:alts_toggle",
                          KEY_A,          BOTH, XKB_KEY_a,                NEXT,
                          KEY_LEFTALT,    DOWN, XKB_KEY_Alt_L,            NEXT,
                          KEY_RIGHTALT,   DOWN, XKB_KEY_ISO_Next_Group,   NEXT,
                          KEY_RIGHTALT,   UP,   XKB_KEY_ISO_Level3_Shift, NEXT,
                          KEY_LEFTALT,    UP,   XKB_KEY_Alt_L,            NEXT,
                          KEY_GRAVE,      UP,   XKB_KEY_numbersign,       FINISH));

    assert(!test_rmlvo_env(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "broken", "what-on-earth", "invalid", "", "",
                           KEY_A,          BOTH, XKB_KEY_a,               FINISH));

    /* Ensure a keymap with an empty xkb_keycodes compiles fine. */
    assert(test_rmlvo_env(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "base", "empty", "empty", "", "",
                          KEY_A,          BOTH, XKB_KEY_NoSymbol,         FINISH));

    /* Check replace merge mode: it should replace the whole <RALT> key */
    const char* replace_options[] = {
        "replace:single,grp:menu_toggle",
        "replace:first,grp:menu_toggle",
        "replace:later,grp:menu_toggle",
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(replace_options); k++) {
        const char* const options = replace_options[k];
        assert(test_rmlvo(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "merge-mode-replace",
                          "", "us,de", "", options,
                          KEY_RIGHTALT,        BOTH, XKB_KEY_Alt_R,           NEXT,
                          KEY_COMPOSE,         BOTH, XKB_KEY_ISO_Next_Group,  NEXT,
                          KEY_RIGHTALT,        BOTH, XKB_KEY_Alt_R,           FINISH));
    }

    /* Has an illegal escape sequence, but shouldn't fail. */
    assert(test_rmlvo_env(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "evdev",
                          "", "cz", "bksl", "",
                          KEY_A,          BOTH, XKB_KEY_a,                FINISH));

    test_extended_groups(ctx);

    xkb_context_unref(ctx);

    ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(test_rmlvo_env(ctx, XKB_KEYMAP_FORMAT_TEXT_V1, "broken",
                          "but", "ignored", "per", "ctx flags",
                          KEY_A,          BOTH, XKB_KEY_a,                FINISH));

    xkb_context_unref(ctx);
    return EXIT_SUCCESS;
}
