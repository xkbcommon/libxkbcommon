#ifndef STATE_H
#define STATE_H

#include "keymap.h"

xkb_overlay_mask_t
xkb_state_serialize_overlays(struct xkb_state *state,
                             enum xkb_state_component type);

#endif
