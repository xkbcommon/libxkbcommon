/*
 * Copyright © 2023 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon.h"
#include "evdev-scancodes.h"
#include "test.h"
#include "keymap.h"
#include "utils.h"

/* Standard real modifier masks */
enum real_mod_mask {
    ShiftMask   = (UINT32_C(1) << XKB_MOD_INDEX_SHIFT),
    LockMask    = (UINT32_C(1) << XKB_MOD_INDEX_CAPS),
    ControlMask = (UINT32_C(1) << XKB_MOD_INDEX_CTRL),
    Mod1Mask    = (UINT32_C(1) << XKB_MOD_INDEX_MOD1),
    Mod2Mask    = (UINT32_C(1) << XKB_MOD_INDEX_MOD2),
    Mod3Mask    = (UINT32_C(1) << XKB_MOD_INDEX_MOD3),
    Mod4Mask    = (UINT32_C(1) << XKB_MOD_INDEX_MOD4),
    Mod5Mask    = (UINT32_C(1) << XKB_MOD_INDEX_MOD5),
    NoModifier  = 0
};

static bool
test_real_mod(struct xkb_keymap *keymap, const char* name,
              xkb_mod_index_t idx, xkb_mod_mask_t mapping)
{
    return xkb_keymap_mod_get_index(keymap, name) == idx &&
           (keymap->mods.mods[idx].type == MOD_REAL) &&
           mapping == keymap->mods.mods[idx].mapping &&
           mapping == (UINT32_C(1) << idx) &&
           xkb_keymap_mod_get_mask(keymap, name) == mapping;
}

static bool
test_virtual_mod(struct xkb_keymap *keymap, const char* name,
                 xkb_mod_index_t idx, xkb_mod_mask_t mapping)
{
    return xkb_keymap_mod_get_index(keymap, name) == idx &&
           (keymap->mods.mods[idx].type == MOD_VIRT) &&
           mapping == keymap->mods.mods[idx].mapping &&
           xkb_keymap_mod_get_mask(keymap, name) == mapping;
}

/* Check that the provided modifier names work */
static void
test_modifiers_names(struct xkb_context *context)
{
    struct xkb_keymap *keymap;

    keymap = test_compile_rules(context, "evdev", "pc104", "us", NULL, NULL);
    assert(keymap);

    /* Real modifiers
     * The indexes and masks are fixed and always valid */
    assert(test_real_mod(keymap, XKB_MOD_NAME_SHIFT, XKB_MOD_INDEX_SHIFT, ShiftMask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_CAPS, XKB_MOD_INDEX_CAPS, LockMask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_CTRL, XKB_MOD_INDEX_CTRL, ControlMask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_MOD1, XKB_MOD_INDEX_MOD1, Mod1Mask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_MOD2, XKB_MOD_INDEX_MOD2, Mod2Mask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_MOD3, XKB_MOD_INDEX_MOD3, Mod3Mask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_MOD4, XKB_MOD_INDEX_MOD4, Mod4Mask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_MOD5, XKB_MOD_INDEX_MOD5, Mod5Mask));

    /* Usual virtual mods mappings */
    assert(test_real_mod(keymap, XKB_MOD_NAME_ALT,  XKB_MOD_INDEX_MOD1, Mod1Mask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_NUM,  XKB_MOD_INDEX_MOD2, Mod2Mask));
    assert(test_real_mod(keymap, XKB_MOD_NAME_LOGO, XKB_MOD_INDEX_MOD4, Mod4Mask));

    /* Virtual modifiers
     * The indexes depends on the keymap files */
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_ALT,    XKB_MOD_INDEX_MOD5 + 2,  Mod1Mask));
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_META,   XKB_MOD_INDEX_MOD5 + 11, Mod1Mask));
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_NUM,    XKB_MOD_INDEX_MOD5 + 1,  Mod2Mask));
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_SUPER,  XKB_MOD_INDEX_MOD5 + 12, Mod4Mask));
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_HYPER,  XKB_MOD_INDEX_MOD5 + 13, Mod4Mask));
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_LEVEL3, XKB_MOD_INDEX_MOD5 + 3,  Mod5Mask));
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_SCROLL, XKB_MOD_INDEX_MOD5 + 8,  0       ));
    /* TODO: current xkeyboard-config maps LevelFive to Nod3 by default */
    assert(test_virtual_mod(keymap, XKB_VMOD_NAME_LEVEL5, XKB_MOD_INDEX_MOD5 + 9,  0));
    /* Legacy stuff, removed from xkeyboard-config */
    assert(keymap->mods.num_mods == 21);
    assert(test_virtual_mod(keymap, "LAlt",     XKB_MOD_INDEX_MOD5 + 4,  0       ));
    assert(test_virtual_mod(keymap, "RAlt",     XKB_MOD_INDEX_MOD5 + 5,  0       ));
    assert(test_virtual_mod(keymap, "LControl", XKB_MOD_INDEX_MOD5 + 7,  0       ));
    assert(test_virtual_mod(keymap, "RControl", XKB_MOD_INDEX_MOD5 + 6,  0       ));
    assert(test_virtual_mod(keymap, "AltGr",    XKB_MOD_INDEX_MOD5 + 10, Mod5Mask));

    xkb_keymap_unref(keymap);
}

static void
test_modmap_none(struct xkb_context *context)
{
    struct xkb_keymap *keymap;
    const struct xkb_key *key;
    xkb_keycode_t keycode;

    keymap = test_compile_file(context, "keymaps/modmap-none.xkb");
    assert(keymap);

    keycode = xkb_keymap_key_by_name(keymap, "LVL3");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == NoModifier);

    keycode = xkb_keymap_key_by_name(keymap, "LFSH");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == NoModifier);

    keycode = xkb_keymap_key_by_name(keymap, "RTSH");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == NoModifier);

    keycode = xkb_keymap_key_by_name(keymap, "LWIN");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod4Mask);

    keycode = xkb_keymap_key_by_name(keymap, "RWIN");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod4Mask);

    keycode = xkb_keymap_key_by_name(keymap, "LCTL");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == ControlMask);

    keycode = xkb_keymap_key_by_name(keymap, "RCTL");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == ControlMask);

    keycode = xkb_keymap_key_by_name(keymap, "LALT");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod1Mask);

    keycode = xkb_keymap_key_by_name(keymap, "RALT");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == (Mod2Mask | Mod5Mask));

    keycode = xkb_keymap_key_by_name(keymap, "CAPS");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == LockMask);

    keycode = xkb_keymap_key_by_name(keymap, "AD01");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod1Mask);

    keycode = xkb_keymap_key_by_name(keymap, "AD02");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == NoModifier);

    keycode = xkb_keymap_key_by_name(keymap, "AD03");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == NoModifier);

    keycode = xkb_keymap_key_by_name(keymap, "AD04");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod1Mask);

    keycode = xkb_keymap_key_by_name(keymap, "AD05");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod2Mask);

    keycode = xkb_keymap_key_by_name(keymap, "AD06");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod3Mask);

    keycode = xkb_keymap_key_by_name(keymap, "AD07");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod1Mask);

    keycode = xkb_keymap_key_by_name(keymap, "AD08");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod2Mask);

    keycode = xkb_keymap_key_by_name(keymap, "AD09");
    assert(keycode != XKB_KEYCODE_INVALID);
    key = XkbKey(keymap, keycode);
    assert(key->modmap == Mod3Mask);

    xkb_keymap_unref(keymap);
}

static void
test_explicit_virtual_modifiers(struct xkb_context *context)
{
    const struct {
        const char* keymap;
        struct mod_props {
            enum mod_type type;
            xkb_mod_mask_t mapping;
            xkb_mod_mask_t mapping_effective;
        } m1;
        struct mod_props m2;
    } tests[] = {
        /* Test virtual modifiers with canonical mappings */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat {\n"
                /* Vmods map to themselves */
                "    virtual_modifiers M1 = 0x100, M2 = 0x200;\n"
                "  };\n"
                "};",
            .m1 = {
                .type = MOD_VIRT,
                .mapping = 0x100,
                .mapping_effective = 0x100
            },
            .m2 = {
                .type = MOD_VIRT,
                .mapping = 0x200,
                .mapping_effective = 0x200
            }
        },
        /* Test virtual modifiers overlapping: identical */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat {\n"
                "    virtual_modifiers M1 = 0x100, M2 = 0x100;\n"
                "  };\n"
                "};",
            .m1 = {
                .type = MOD_VIRT,
                .mapping = 0x100,
                .mapping_effective = 0x100
            },
            .m2 = {
                .type = MOD_VIRT,
                .mapping = 0x100,
                .mapping_effective = 0x100
            }
        },
        /* Test virtual modifiers overlapping: non identical */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat {\n"
                "    virtual_modifiers M1 = 0x100, M2 = 0x300;\n"
                "  };\n"
                "};",
            .m1 = {
                .type = MOD_VIRT,
                .mapping = 0x100,
                .mapping_effective = 0x100
            },
            .m2 = {
                .type = MOD_VIRT,
                .mapping = 0x300,
                .mapping_effective = 0x300
            }
        },
        /* Test virtual modifiers with swapped mappings */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat {\n"
                /* The mapping of each modifier is the mask of the other */
                "    virtual_modifiers M1 = 0x200, M2 = 0x100;\n"
                "  };\n"
                "};",
            .m1 = {
                .type = MOD_VIRT,
                .mapping = 0x200,
                .mapping_effective = 0x100 /* different from mapping! */
            },
            .m2 = {
                .type = MOD_VIRT,
                .mapping = 0x100,
                .mapping_effective = 0x200 /* different from mapping! */
            }
        },
        /* Test virtual modifiers mapping to undefined modifiers */
        {
            .keymap =
                "xkb_keymap {\n"
                "  xkb_compat {\n"
                "    virtual_modifiers M1 = 0x400, M2 = 0x800;\n"
                "  };\n"
                "};",
            .m1 = {
                .type = MOD_VIRT,
                .mapping = 0x400,
                .mapping_effective = 0 /* no mod entry */
            },
            .m2 = {
                .type = MOD_VIRT,
                .mapping = 0x800,
                .mapping_effective = 0 /* no mod entry */
            }
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        struct xkb_keymap *keymap = test_compile_buffer(
            context, tests[k].keymap, strlen(tests[k].keymap)
        );
        assert(keymap);

        const xkb_mod_index_t m1_idx = xkb_keymap_mod_get_index(keymap, "M1");
        const xkb_mod_index_t m2_idx = xkb_keymap_mod_get_index(keymap, "M2");
        assert(m1_idx == 8);
        assert(m2_idx == 9);
        assert(keymap->mods.mods[m1_idx].type == tests[k].m1.type);
        assert(keymap->mods.mods[m2_idx].type == tests[k].m2.type);

        const xkb_mod_mask_t m1 = UINT32_C(1) << m1_idx;
        const xkb_mod_mask_t m2 = UINT32_C(1) << m2_idx;
        const xkb_mod_mask_t m1_mapping = mod_mask_get_effective(keymap, m1);
        const xkb_mod_mask_t m2_mapping = mod_mask_get_effective(keymap, m2);
        assert(m1_mapping == tests[k].m1.mapping);
        assert(m2_mapping == tests[k].m2.mapping);
        /* mod_mask_get_effective is not idempotent */
        assert(mod_mask_get_effective(keymap, m1_mapping) ==
               tests[k].m1.mapping_effective);
        assert(mod_mask_get_effective(keymap, m2_mapping) ==
               tests[k].m2.mapping_effective);

        struct xkb_state *state = xkb_state_new(keymap);
        assert(state);

        /* Not in the canonical modifier mask nor denotes a *known* virtual
         * modifiers, so it will be discarded. */
        const xkb_mod_mask_t noise = UINT32_C(0x8000);
        assert((keymap->canonical_state_mask & noise) == 0);

        /* Update the state, then check round-trip and mods state */
        const xkb_mod_mask_t set_masks[] = {m1_mapping, m2_mapping};
        for (unsigned int m = 0; m < ARRAY_SIZE(set_masks); m++) {
            const xkb_mod_mask_t expected = set_masks[m];
            xkb_state_update_mask(state, expected | noise, 0, noise, 0, 0, 0);
            const xkb_mod_mask_t got =
                xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
            assert_printf(got == expected, /* round-trip */
                          "expected %#"PRIx32", got %#"PRIx32"\n",
                          expected, got);
            assert(
                xkb_state_mod_index_is_active(state, m1_idx,
                                              XKB_STATE_MODS_EFFECTIVE) ==
                ((expected & m1_mapping) == m1_mapping)
            );
            assert(
                xkb_state_mod_index_is_active(state, m2_idx,
                                              XKB_STATE_MODS_EFFECTIVE) ==
                ((expected & m2_mapping) == m2_mapping)
            );
        }

        xkb_state_unref(state);
        xkb_keymap_unref(keymap);
    }
}

/*
 * Test the hack documented in the FAQ to get virtual modifiers mapping using
 * `xkb_state_update_mask`/`xkb_state_serialize_mods`.
 *
 * This should work without problem for keymap using only real mods to map
 * virtual modifiers.
 *
 * [NOTE] If the test requires an update, do not forget to update the FAQ as well!
 */
static void
test_virtual_modifiers_mapping_hack(struct xkb_context *context)
{
    struct xkb_keymap *keymap = test_compile_rules(context, "evdev",
                                                   "pc104", "us", NULL, NULL);
    assert(keymap);

    struct xkb_state* state = xkb_state_new(keymap);
    assert(state);

    static const struct {
        const char* name;
        xkb_mod_index_t index;
        xkb_mod_mask_t mapping;
    } mods[] = {
        /* Real modifiers */
        {
            .name = XKB_MOD_NAME_SHIFT,
            .index = XKB_MOD_INDEX_SHIFT,
            .mapping = UINT32_C(1) << XKB_MOD_INDEX_SHIFT
        },
        {
            .name = XKB_MOD_NAME_CAPS,
            .index = XKB_MOD_INDEX_CAPS,
            .mapping = UINT32_C(1) << XKB_MOD_INDEX_CAPS
        },
        {
            .name = XKB_MOD_NAME_CTRL,
            .index = XKB_MOD_INDEX_CTRL,
            .mapping = UINT32_C(1) << XKB_MOD_INDEX_CTRL
        },
        {
            .name = XKB_MOD_NAME_MOD1,
            .index = XKB_MOD_INDEX_MOD1,
            .mapping = UINT32_C(1) << XKB_MOD_INDEX_MOD1
        },
        {
            .name = XKB_MOD_NAME_MOD2,
            .index = XKB_MOD_INDEX_MOD2,
            .mapping = UINT32_C(1) << XKB_MOD_INDEX_MOD2
        },
        {
            .name = XKB_MOD_NAME_MOD3,
            .index = XKB_MOD_INDEX_MOD3,
            .mapping = UINT32_C(1) << XKB_MOD_INDEX_MOD3
        },
        {
            .name = XKB_MOD_NAME_MOD4,
            .index = XKB_MOD_INDEX_MOD4,
            .mapping = UINT32_C(1) << XKB_MOD_INDEX_MOD4
        },
        {
            .name = XKB_MOD_NAME_MOD5,
            .index = XKB_MOD_INDEX_MOD5,
            .mapping = UINT32_C(1) << XKB_MOD_INDEX_MOD5
        },
        /* Virtual modifiers
         * The indexes depends on the keymap files */
        {
            .name = XKB_VMOD_NAME_ALT,
            .index = XKB_MOD_INDEX_MOD5 + 2,
            .mapping = Mod1Mask
        },
        {
            .name = XKB_VMOD_NAME_META,
            .index = XKB_MOD_INDEX_MOD5 + 11,
            .mapping = Mod1Mask
        },
        {
            .name = XKB_VMOD_NAME_NUM,
            .index = XKB_MOD_INDEX_MOD5 + 1,
            .mapping = Mod2Mask
        },
        {
            .name = XKB_VMOD_NAME_SUPER,
            .index = XKB_MOD_INDEX_MOD5 + 12,
            .mapping = Mod4Mask
        },
        {
            .name = XKB_VMOD_NAME_HYPER,
            .index = XKB_MOD_INDEX_MOD5 + 13,
            .mapping = Mod4Mask
        },
        {
            .name = XKB_VMOD_NAME_LEVEL3,
            .index = XKB_MOD_INDEX_MOD5 + 3,
            .mapping = Mod5Mask
        },
        {
            .name = XKB_VMOD_NAME_SCROLL,
            .index = XKB_MOD_INDEX_MOD5 + 8,
            .mapping = 0
        },
        {
            .name = XKB_VMOD_NAME_LEVEL5,
            .index = XKB_MOD_INDEX_MOD5 + 9,
            .mapping = 0
        },
        /* Legacy stuff, removed from xkeyboard-config */
        {
            .name = "LAlt",
            .index = XKB_MOD_INDEX_MOD5 + 4,
            .mapping = 0
        },
        {
            .name = "RAlt",
            .index = XKB_MOD_INDEX_MOD5 + 5,
            .mapping = 0
        },
        {
            .name = "LControl",
            .index = XKB_MOD_INDEX_MOD5 + 7,
            .mapping = 0
        },
        {
            .name = "RControl",
            .index = XKB_MOD_INDEX_MOD5 + 6,
            .mapping = 0
        },
        {
            .name = "AltGr",
            .index = XKB_MOD_INDEX_MOD5 + 10,
            .mapping = Mod5Mask
        },
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(mods); k++) {
        const xkb_mod_mask_t index =
            xkb_keymap_mod_get_index(keymap, mods[k].name);
        assert_printf(index == mods[k].index,
                      "%s: expected %"PRIu32", got: %"PRIu32"\n",
                      mods[k].name, mods[k].index, index);
        const xkb_mod_mask_t mask = UINT32_C(1) << index;
        xkb_state_update_mask(state, mask, 0, 0, 0, 0, 0);
        const xkb_mod_mask_t mapping =
            xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        assert_printf(mapping == mods[k].mapping,
                      "%s: expected %#"PRIx32", got: %#"PRIx32"\n",
                      mods[k].name, mods[k].mapping, mapping);
        assert(mapping == xkb_keymap_mod_get_mask(keymap, mods[k].name));
    }

    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
}

static void
test_pure_virtual_modifiers(struct xkb_context *context)
{
    struct xkb_keymap *keymap;

    /* Test definition of >20 pure virtual modifiers.
     * We superate the X11 limit of 16 virtual modifiers. */
    keymap = test_compile_file(context, "keymaps/pure-virtual-mods.xkb");
    assert(keymap);

    assert(test_key_seq(keymap,
                        KEY_W,          BOTH,  XKB_KEY_w,        NEXT,
                        KEY_A,          DOWN,  XKB_KEY_a,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_a,        NEXT,
                        KEY_A,          UP,    XKB_KEY_a,        NEXT,
                        KEY_B,          DOWN,  XKB_KEY_b,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_b,        NEXT,
                        KEY_B,          UP,    XKB_KEY_b,        NEXT,
                        KEY_C,          DOWN,  XKB_KEY_c,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_c,        NEXT,
                        KEY_C,          UP,    XKB_KEY_c,        NEXT,
                        KEY_D,          DOWN,  XKB_KEY_d,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_d,        NEXT,
                        KEY_D,          UP,    XKB_KEY_d,        NEXT,
                        KEY_E,          DOWN,  XKB_KEY_e,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_e,        NEXT,
                        KEY_E,          UP,    XKB_KEY_e,        NEXT,
                        KEY_F,          DOWN,  XKB_KEY_f,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_f,        NEXT,
                        KEY_F,          UP,    XKB_KEY_f,        NEXT,
                        KEY_G,          DOWN,  XKB_KEY_g,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_g,        NEXT,
                        KEY_G,          UP,    XKB_KEY_g,        NEXT,
                        KEY_H,          DOWN,  XKB_KEY_h,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_h,        NEXT,
                        KEY_H,          UP,    XKB_KEY_h,        NEXT,
                        KEY_I,          DOWN,  XKB_KEY_i,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_i,        NEXT,
                        KEY_I,          UP,    XKB_KEY_i,        NEXT,
                        KEY_J,          DOWN,  XKB_KEY_j,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_j,        NEXT,
                        KEY_J,          UP,    XKB_KEY_j,        NEXT,
                        KEY_K,          DOWN,  XKB_KEY_k,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_k,        NEXT,
                        KEY_K,          UP,    XKB_KEY_k,        NEXT,
                        KEY_L,          DOWN,  XKB_KEY_l,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_l,        NEXT,
                        KEY_L,          UP,    XKB_KEY_l,        NEXT,
                        KEY_M,          DOWN,  XKB_KEY_m,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_m,        NEXT,
                        KEY_M,          UP,    XKB_KEY_m,        NEXT,
                        KEY_N,          DOWN,  XKB_KEY_n,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_n,        NEXT,
                        KEY_N,          UP,    XKB_KEY_n,        NEXT,
                        KEY_O,          DOWN,  XKB_KEY_o,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_o,        NEXT,
                        KEY_O,          UP,    XKB_KEY_o,        NEXT,
                        KEY_P,          DOWN,  XKB_KEY_p,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_p,        NEXT,
                        KEY_P,          UP,    XKB_KEY_p,        NEXT,
                        KEY_Q,          DOWN,  XKB_KEY_q,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_q,        NEXT,
                        KEY_Q,          UP,    XKB_KEY_q,        NEXT,
                        KEY_R,          DOWN,  XKB_KEY_r,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_r,        NEXT,
                        KEY_R,          UP,    XKB_KEY_r,        NEXT,
                        KEY_S,          DOWN,  XKB_KEY_s,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_s,        NEXT,
                        KEY_S,          UP,    XKB_KEY_s,        NEXT,
                        KEY_T,          DOWN,  XKB_KEY_t,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_t,        NEXT,
                        KEY_T,          UP,    XKB_KEY_t,        NEXT,
                        KEY_U,          DOWN,  XKB_KEY_u,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_u,        NEXT,
                        KEY_U,          UP,    XKB_KEY_u,        NEXT,
                        KEY_V,          DOWN,  XKB_KEY_v,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_v,        NEXT,
                        KEY_LEFTSHIFT,  DOWN,  XKB_KEY_Shift_L,  NEXT,
                        KEY_W,          BOTH,  XKB_KEY_V,        NEXT,
                        KEY_LEFTSHIFT,  UP,    XKB_KEY_Shift_L,  NEXT,
                        KEY_V,          UP,    XKB_KEY_v,        NEXT,
                        KEY_A,          DOWN,  XKB_KEY_a,        NEXT,
                        KEY_S,          DOWN,  XKB_KEY_s,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_1,        NEXT,
                        KEY_RIGHTALT,   DOWN,  XKB_KEY_ISO_Level3_Shift, NEXT,
                        KEY_W,          BOTH,  XKB_KEY_4,        NEXT,
                        KEY_S,          UP,    XKB_KEY_s,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_3,        NEXT,
                        KEY_RIGHTALT,   UP,    XKB_KEY_ISO_Level3_Shift, NEXT,
                        KEY_Q,          DOWN,  XKB_KEY_q,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_2,        NEXT,
                        KEY_Q,          UP,    XKB_KEY_q,        NEXT,
                        KEY_B,          DOWN,  XKB_KEY_b,        NEXT,
                        KEY_C,          DOWN,  XKB_KEY_c,        NEXT,
                        KEY_W,          BOTH,  XKB_KEY_5,        NEXT,
                        KEY_C,          UP,    XKB_KEY_c,        NEXT,
                        KEY_B,          UP,    XKB_KEY_b,        NEXT,
                        KEY_A,          UP,    XKB_KEY_a,        NEXT,
                        KEY_Y,          BOTH,  XKB_KEY_y,        FINISH));
    xkb_keymap_unref(keymap);

    /* Test invalid interpret using a virtual modifier */
    const char keymap_str[] =
        "xkb_keymap {"
        "  xkb_keycodes { include \"evdev\" };"
        "  xkb_types { include \"complete\" };"
        "  xkb_compat { include \"complete+basic(invalid-pure-virtual-modifiers)\" };"
        "  xkb_symbols { include \"pc(pc105-pure-virtual-modifiers)\" };"
        "};";
    keymap = test_compile_string(context, keymap_str);
    assert(!keymap);
}

int
main(void)
{
    test_init();

    struct xkb_context *context = test_get_context(CONTEXT_NO_FLAG);
    assert(context);

    test_modmap_none(context);
    test_modifiers_names(context);
    test_explicit_virtual_modifiers(context);
    test_virtual_modifiers_mapping_hack(context);
    test_pure_virtual_modifiers(context);

    xkb_context_unref(context);

    return 0;
}
