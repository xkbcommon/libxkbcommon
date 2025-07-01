/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */

#include "config.h"

#include <assert.h>

#include "keymap.h"
#include "keysym.h"
#include "text.h"

bool
LookupString(const LookupEntry tab[], const char *string,
              unsigned int *value_rtrn)
{
    if (!string)
        return false;

    for (const LookupEntry *entry = tab; entry->name; entry++) {
        if (istreq(entry->name, string)) {
            *value_rtrn = entry->value;
            return true;
        }
    }

    return false;
}

const char *
LookupValue(const LookupEntry tab[], unsigned int value)
{
    for (const LookupEntry *entry = tab; entry->name; entry++)
        if (entry->value == value)
            return entry->name;

    return NULL;
}

const LookupEntry ctrlMaskNames[] = {
    { "RepeatKeys", CONTROL_REPEAT },
    { "Repeat", CONTROL_REPEAT },
    { "AutoRepeat", CONTROL_REPEAT },
    { "SlowKeys", CONTROL_SLOW },
    { "BounceKeys", CONTROL_DEBOUNCE },
    { "StickyKeys", CONTROL_STICKY },
    { "MouseKeys", CONTROL_MOUSEKEYS },
    { "MouseKeysAccel", CONTROL_MOUSEKEYS_ACCEL },
    { "AccessXKeys", CONTROL_AX },
    { "AccessXTimeout", CONTROL_AX_TIMEOUT },
    { "AccessXFeedback", CONTROL_AX_FEEDBACK },
    { "AudibleBell", CONTROL_BELL },
    { "IgnoreGroupLock", CONTROL_IGNORE_GROUP_LOCK },
    { "all", CONTROL_ALL },
    { "none", 0 },
    { "Overlay1", 0 },
    { "Overlay2", 0 },
    { NULL, 0 }
};

const LookupEntry modComponentMaskNames[] = {
    { "base", XKB_STATE_MODS_DEPRESSED },
    { "latched", XKB_STATE_MODS_LATCHED },
    { "locked", XKB_STATE_MODS_LOCKED },
    { "effective", XKB_STATE_MODS_EFFECTIVE },
    { "compat", XKB_STATE_MODS_EFFECTIVE },
    { "any", XKB_STATE_MODS_EFFECTIVE },
    { "none", 0 },
    { NULL, 0 }
};

const LookupEntry groupComponentMaskNames[] = {
    { "base", XKB_STATE_LAYOUT_DEPRESSED },
    { "latched", XKB_STATE_LAYOUT_LATCHED },
    { "locked", XKB_STATE_LAYOUT_LOCKED },
    { "effective", XKB_STATE_LAYOUT_EFFECTIVE },
    { "any", XKB_STATE_LAYOUT_EFFECTIVE },
    { "none", 0 },
    { NULL, 0 }
};

const LookupEntry groupMaskNames[] = {
    { "none", 0x00 },
    { "all", XKB_ALL_GROUPS },
    { NULL, 0 }
};

const LookupEntry levelNames[] = {
    { "Level1", 1 },
    { "Level2", 2 },
    { "Level3", 3 },
    { "Level4", 4 },
    { "Level5", 5 },
    { "Level6", 6 },
    { "Level7", 7 },
    { "Level8", 8 },
    { NULL, 0 }
};

const LookupEntry buttonNames[] = {
    { "Button1", 1 },
    { "Button2", 2 },
    { "Button3", 3 },
    { "Button4", 4 },
    { "Button5", 5 },
    { "default", 0 },
    { NULL, 0 }
};

const LookupEntry useModMapValueNames[] = {
    { "LevelOne", 1 },
    { "Level1", 1 },
    { "AnyLevel", 0 },
    { "any", 0 },
    { NULL, 0 }
};

const LookupEntry actionTypeNames[] = {
    { "NoAction", ACTION_TYPE_NONE },
    { "VoidAction", ACTION_TYPE_VOID },
    { "SetMods", ACTION_TYPE_MOD_SET },
    { "LatchMods", ACTION_TYPE_MOD_LATCH },
    { "LockMods", ACTION_TYPE_MOD_LOCK },
    { "SetGroup", ACTION_TYPE_GROUP_SET },
    { "LatchGroup", ACTION_TYPE_GROUP_LATCH },
    { "LockGroup", ACTION_TYPE_GROUP_LOCK },
    { "MovePtr", ACTION_TYPE_PTR_MOVE },
    { "MovePointer", ACTION_TYPE_PTR_MOVE },
    { "PtrBtn", ACTION_TYPE_PTR_BUTTON },
    { "PointerButton", ACTION_TYPE_PTR_BUTTON },
    { "LockPtrBtn", ACTION_TYPE_PTR_LOCK },
    { "LockPtrButton", ACTION_TYPE_PTR_LOCK },
    { "LockPointerButton", ACTION_TYPE_PTR_LOCK },
    { "LockPointerBtn", ACTION_TYPE_PTR_LOCK },
    { "SetPtrDflt", ACTION_TYPE_PTR_DEFAULT },
    { "SetPointerDefault", ACTION_TYPE_PTR_DEFAULT },
    { "Terminate", ACTION_TYPE_TERMINATE },
    { "TerminateServer", ACTION_TYPE_TERMINATE },
    { "SwitchScreen", ACTION_TYPE_SWITCH_VT },
    { "SetControls", ACTION_TYPE_CTRL_SET },
    { "LockControls", ACTION_TYPE_CTRL_LOCK },
    { "Private", ACTION_TYPE_PRIVATE },
    /* deprecated actions below here - unused */
    { "RedirectKey", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "Redirect", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "ISOLock", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "ActionMessage", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "MessageAction", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "Message", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "DeviceBtn", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "DevBtn", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "DevButton", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "DeviceButton", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "LockDeviceBtn", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "LockDevBtn", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "LockDevButton", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "LockDeviceButton", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "DeviceValuator", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "DevVal", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "DeviceVal", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { "DevValuator", ACTION_TYPE_UNSUPPORTED_LEGACY },
    { NULL, 0 },
};

const LookupEntry symInterpretMatchMaskNames[] = {
    { "NoneOf", MATCH_NONE },
    { "AnyOfOrNone", MATCH_ANY_OR_NONE },
    { "AnyOf", MATCH_ANY },
    { "AllOf", MATCH_ALL },
    { "Exactly", MATCH_EXACTLY },
    { NULL, 0 },
};

const char *
ModIndexText(struct xkb_context *ctx, const struct xkb_mod_set *mods,
             xkb_mod_index_t ndx)
{
    if (ndx == XKB_MOD_INVALID)
        return "none";

    if (ndx == XKB_MOD_NONE)
        return "None";

    if (ndx >= mods->num_mods)
        return NULL;

    return xkb_atom_text(ctx, mods->mods[ndx].name);
}

const char *
ActionTypeText(enum xkb_action_type type)
{
    const char *name = LookupValue(actionTypeNames, type);
    return name ? name : "Private";
}

const char *
KeysymText(struct xkb_context *ctx, xkb_keysym_t sym)
{
    char *buffer = xkb_context_get_buffer(ctx, XKB_KEYSYM_NAME_MAX_SIZE);
    xkb_keysym_get_name(sym, buffer, XKB_KEYSYM_NAME_MAX_SIZE);
    return buffer;
}

const char *
KeyNameText(struct xkb_context *ctx, xkb_atom_t name)
{
    const char *sname = xkb_atom_text(ctx, name);
    size_t len = strlen_safe(sname) + 3;
    char *buf = xkb_context_get_buffer(ctx, len);
    snprintf(buf, len, "<%s>", strempty(sname));
    return buf;
}

const char *
SIMatchText(enum xkb_match_operation type)
{
    return LookupValue(symInterpretMatchMaskNames, type);
}

const char *
ModMaskText(struct xkb_context *ctx, enum mod_type type,
            const struct xkb_mod_set *mods, xkb_mod_mask_t mask)
{
    char buf[1024] = {0};
    size_t pos = 0;
    const struct xkb_mod *mod;

    /* We want to avoid boolean blindness, but we expected only 2 values */
    assert(type == MOD_REAL || type == MOD_BOTH);

    if (mask == 0)
        return "none";

    if (mask == MOD_REAL_MASK_ALL)
        return "all";

    if ((type == MOD_REAL && (mask & ~MOD_REAL_MASK_ALL)) ||
        unlikely(mask & ~((UINT64_C(1) << mods->num_mods) - 1))) {
        /* If we get a mask that cannot be expressed with the known modifiers
         * of the given type, print it as hexadecimal */
        const int ret = snprintf(buf, sizeof(buf), "0x%"PRIx32, mask);
        static_assert(sizeof(mask) == 4 && sizeof(buf) >= sizeof("0xffffffff"),
                      "Buffer too small");
        assert(ret >= 0 && (size_t) ret < sizeof(buf));
        pos = (size_t) ret;
    } else {
        /* Print known mods */
        xkb_mods_mask_foreach(mask, mod, mods) {
            int ret = snprintf(buf + pos, sizeof(buf) - pos, "%s%s",
                               pos == 0 ? "" : "+",
                               xkb_atom_text(ctx, mod->name));
            if (ret <= 0 || pos + ret >= sizeof(buf))
                break;
            else
                pos += ret;
        }
    }
    pos++;

    return memcpy(xkb_context_get_buffer(ctx, pos), buf, pos);
}

const char *
LedStateMaskText(struct xkb_context *ctx, const LookupEntry *lookup,
                 enum xkb_state_component mask)
{
    char buf[1024];
    size_t pos = 0;

    if (mask == 0)
        return "0";

    for (unsigned int i = 0; mask; i++) {
        int ret;

        if (!(mask & (1u << i)))
            continue;

        mask &= ~(1u << i);

        const char* const maskText = LookupValue(lookup, 1u << i);
        assert(maskText != NULL);
        ret = snprintf(buf + pos, sizeof(buf) - pos, "%s%s",
                       pos == 0 ? "" : "+", maskText);
        if (ret <= 0 || pos + ret >= sizeof(buf))
            break;
        else
            pos += ret;
    }
    pos++;

    return memcpy(xkb_context_get_buffer(ctx, pos), buf, pos);
}

const char *
ControlMaskText(struct xkb_context *ctx, enum xkb_action_controls mask)
{
    char buf[1024];
    size_t pos = 0;

    if (mask == 0)
        return "none";

    if (mask == CONTROL_ALL)
        return "all";

    for (unsigned int i = 0; mask; i++) {
        int ret;

        if (!(mask & (1u << i)))
            continue;

        mask &= ~(1u << i);

        const char* const maskText = LookupValue(ctrlMaskNames, 1u << i);
        assert(maskText != NULL);
        ret = snprintf(buf + pos, sizeof(buf) - pos, "%s%s",
                       pos == 0 ? "" : "+", maskText);
        if (ret <= 0 || pos + ret >= sizeof(buf))
            break;
        else
            pos += ret;
    }
    pos++;

    return memcpy(xkb_context_get_buffer(ctx, pos), buf, pos);
}
