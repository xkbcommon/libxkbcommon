/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <stdbool.h>

#include "xkbcommon/xkbcommon.h"
#include "atom.h"
#include "keymap.h"

typedef struct {
    const char *name;
    uint32_t value;
} LookupEntry;

bool
LookupString(const LookupEntry tab[], const char *string,
              unsigned int *value_rtrn);

const char *
LookupValue(const LookupEntry tab[], unsigned int value);

extern const LookupEntry ctrlMaskNames[];
extern const LookupEntry modComponentMaskNames[];
extern const LookupEntry groupComponentMaskNames[];
extern const LookupEntry groupMaskNames[];
extern const LookupEntry groupNames[];
extern const LookupEntry levelNames[];
extern const LookupEntry buttonNames[];
extern const LookupEntry useModMapValueNames[];
extern const LookupEntry actionTypeNames[];
extern const LookupEntry symInterpretMatchMaskNames[];

const char *
ModMaskText(struct xkb_context *ctx, enum mod_type type,
            const struct xkb_mod_set *mods, xkb_mod_mask_t mask);

const char *
ModIndexText(struct xkb_context *ctx, const struct xkb_mod_set *mods,
             xkb_mod_index_t ndx);

const char *
ActionTypeText(enum xkb_action_type type);

const char *
KeysymText(struct xkb_context *ctx, xkb_keysym_t sym);

const char *
KeyNameText(struct xkb_context *ctx, xkb_atom_t name);

const char *
SIMatchText(enum xkb_match_operation type);

const char *
LedStateMaskText(struct xkb_context *ctx, const LookupEntry *lookup,
                 enum xkb_state_component mask);

const char *
ControlMaskText(struct xkb_context *ctx, enum xkb_action_controls mask);
