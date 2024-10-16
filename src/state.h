#ifndef STATE_H
#define STATE_H

#include "xkbcommon/xkbcommon.h"
#include "keymap.h"

void
xkb_state_set_components(
    struct xkb_state *state,
    int32_t base_group,
    int32_t latched_group,
    int32_t locked_group,
    xkb_layout_index_t group,
    xkb_mod_mask_t base_mods,
    xkb_mod_mask_t latched_mods,
    xkb_mod_mask_t locked_mods,
    xkb_mod_mask_t mods,
    xkb_led_mask_t leds);

#endif
