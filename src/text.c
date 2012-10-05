/************************************************************
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ********************************************************/

#include "keymap.h"
#include "text.h"

bool
LookupString(const LookupEntry tab[], const char *string,
              unsigned int *value_rtrn)
{
    const LookupEntry *entry;

    if (!string)
        return false;

    for (entry = tab; entry->name; entry++) {
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
    const LookupEntry *entry;

    for (entry = tab; entry->name; entry++)
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
    {"base", XKB_STATE_DEPRESSED},
    {"latched", XKB_STATE_LATCHED},
    {"locked", XKB_STATE_LOCKED},
    {"effective", XKB_STATE_EFFECTIVE},
    {"compat", XKB_STATE_EFFECTIVE},
    {"any", XKB_STATE_EFFECTIVE},
    {"none", 0},
    {NULL, 0}
};

const LookupEntry groupComponentMaskNames[] = {
    {"base", XKB_STATE_DEPRESSED},
    {"latched", XKB_STATE_LATCHED},
    {"locked", XKB_STATE_LOCKED},
    {"effective", XKB_STATE_EFFECTIVE},
    {"any", XKB_STATE_EFFECTIVE},
    {"none", 0},
    {NULL, 0}
};

const LookupEntry groupMaskNames[] = {
    {"group1", 0x01},
    {"group2", 0x02},
    {"group3", 0x04},
    {"group4", 0x08},
    {"group5", 0x10},
    {"group6", 0x20},
    {"group7", 0x40},
    {"group8", 0x80},
    {"none", 0x00},
    {"all", 0xff},
    {NULL, 0}
};

const LookupEntry groupNames[] = {
    {"group1", 1},
    {"group2", 2},
    {"group3", 3},
    {"group4", 4},
    {"group5", 5},
    {"group6", 6},
    {"group7", 7},
    {"group8", 8},
    {NULL, 0}
};

const LookupEntry levelNames[] = {
    { "level1", 1 },
    { "level2", 2 },
    { "level3", 3 },
    { "level4", 4 },
    { "level5", 5 },
    { "level6", 6 },
    { "level7", 7 },
    { "level8", 8 },
    { NULL, 0 }
};

const LookupEntry buttonNames[] = {
    { "button1", 1 },
    { "button2", 2 },
    { "button3", 3 },
    { "button4", 4 },
    { "button5", 5 },
    { "default", 0 },
    { NULL, 0 }
};

const LookupEntry useModMapValueNames[] = {
    { "levelone", 1 },
    { "level1", 1 },
    { "anylevel", 0 },
    { "any", 0 },
    { NULL, 0 }
};

const LookupEntry actionTypeNames[] = {
    { "NoAction", ACTION_TYPE_NONE },
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
    { "RedirectKey", ACTION_TYPE_KEY_REDIRECT },
    { "Redirect", ACTION_TYPE_KEY_REDIRECT },
    { "Private", ACTION_TYPE_PRIVATE },
    /* deprecated actions below here - unused */
    { "ISOLock", ACTION_TYPE_NONE },
    { "ActionMessage", ACTION_TYPE_NONE },
    { "MessageAction", ACTION_TYPE_NONE },
    { "Message", ACTION_TYPE_NONE },
    { "DeviceBtn", ACTION_TYPE_NONE },
    { "DevBtn", ACTION_TYPE_NONE },
    { "DevButton", ACTION_TYPE_NONE },
    { "DeviceButton", ACTION_TYPE_NONE },
    { "LockDeviceBtn", ACTION_TYPE_NONE },
    { "LockDevBtn", ACTION_TYPE_NONE },
    { "LockDevButton", ACTION_TYPE_NONE },
    { "LockDeviceButton", ACTION_TYPE_NONE },
    { "DeviceValuator", ACTION_TYPE_NONE },
    { "DevVal", ACTION_TYPE_NONE },
    { "DeviceVal", ACTION_TYPE_NONE },
    { "DevValuator", ACTION_TYPE_NONE },
    { NULL, 0 },
};

const LookupEntry symInterpretMatchMaskNames[] = {
    { "NoneOf", MATCH_NONE },
    { "AnyOfOrNone", MATCH_ANY_OR_NONE },
    { "AnyOf", MATCH_ANY },
    { "AllOf", MATCH_ALL },
    { "Exactly", MATCH_EXACTLY },
};

#define BUFFER_SIZE 1024

static char *
GetBuffer(size_t size)
{
    static char buffer[BUFFER_SIZE];
    static size_t next;
    char *rtrn;

    if (size >= BUFFER_SIZE)
        return NULL;

    if (BUFFER_SIZE - next <= size)
        next = 0;

    rtrn = &buffer[next];
    next += size;

    return rtrn;
}

/* Get a mod mask's text, where the mask is in rmods+vmods format. */
const char *
VModMaskText(struct xkb_keymap *keymap, xkb_mod_mask_t mask)
{
    xkb_mod_index_t i;
    xkb_mod_mask_t rmask, vmask;
    int len, rem;
    const char *mm = NULL;
    char *rtrn, *str;
    char buf[BUFFER_SIZE];
    const struct xkb_mod *mod;

    rmask = mask & 0xff;
    vmask = mask & (~0xff);

    if (rmask == 0 && vmask == 0)
        return "none";

    if (rmask != 0)
        mm = ModMaskText(keymap, rmask);

    if (vmask == 0)
        return mm;

    str = buf;
    buf[0] = '\0';
    rem = BUFFER_SIZE;

    darray_enumerate(i, mod, keymap->mods) {
        if (mod->type != MOD_VIRT || !(vmask & (1 << i)))
            continue;

        len = snprintf(str, rem, "%s%s",
                       (str != buf) ? "+" : "",
                       xkb_atom_text(keymap->ctx, mod->name));
        rem -= len;
        str += len;

        if (rem <= 1)
            break;
    }

    str = buf;

    len = (str ? strlen(str) : 0) + (mm ? strlen(mm) : 0) +
          (str && mm ? 1 : 0);
    if (len >= BUFFER_SIZE)
        len = BUFFER_SIZE - 1;

    rtrn = GetBuffer(len + 1);
    rtrn[0] = '\0';

    snprintf(rtrn, len + 1, "%s%s%s", (mm ? mm : ""),
             (mm && str ? "+" : ""), (str ? str : ""));

    return rtrn;
}

xkb_mod_index_t
ModNameToIndex(const struct xkb_keymap *keymap, xkb_atom_t name)
{
    xkb_mod_index_t i;
    const struct xkb_mod *mod;

    darray_enumerate(i, mod, keymap->mods)
        if (mod->type == MOD_REAL && name == mod->name)
            return i;

    return XKB_MOD_INVALID;
}

xkb_atom_t
ModIndexToName(struct xkb_keymap *keymap, xkb_mod_index_t ndx)
{
    if (ndx >= darray_size(keymap->mods) ||
        darray_item(keymap->mods, ndx).type != MOD_REAL)
        return XKB_ATOM_NONE;

    return darray_item(keymap->mods, ndx).name;
}

const char *
ModIndexText(struct xkb_keymap *keymap, xkb_mod_index_t ndx)
{
    xkb_atom_t name;

    name = ModIndexToName(keymap, ndx);
    if (name)
        return xkb_atom_text(keymap->ctx, name);

    if (ndx == XKB_MOD_INVALID)
        return "none";

    return "illegal";
}

const char *
ModMaskText(struct xkb_keymap *keymap, xkb_mod_mask_t mask)
{
    xkb_mod_index_t i;
    int len, rem;
    char *str, *buf;
    const struct xkb_mod *mod;

    if ((mask & 0xff) == 0xff)
        return "all";

    if ((mask & 0xff) == 0)
        return "none";

    rem = 64;
    buf = GetBuffer(rem);
    str = buf;
    buf[0] = '\0';
    darray_enumerate(i, mod, keymap->mods) {
        if (mod->type != MOD_REAL || !(mask & (1 << i)))
            continue;

        len = snprintf(str, rem, "%s%s",
                       (str != buf ?  "+" : ""),
                       ModIndexText(keymap, i));
        rem -= len;
        str += len;

        if (rem <= 1)
            break;
    }

    return buf;
}

const char *
ActionTypeText(unsigned type)
{
    const char *name = LookupValue(actionTypeNames, type);
    return name ? name : "Private";
}

const char *
KeysymText(xkb_keysym_t sym)
{
    static char buffer[64];

    xkb_keysym_get_name(sym, buffer, sizeof buffer);

    return buffer;
}

const char *
KeyNameText(struct xkb_context *ctx, xkb_atom_t name)
{
    const char *sname = xkb_atom_text(ctx, name);
    size_t len = strlen(sname) + 3;
    char *buf = GetBuffer(len);
    snprintf(buf, len, "<%s>", sname);
    return buf;
}

const char *
SIMatchText(enum xkb_match_operation type)
{
    const char *name;
    char *buf;

    type &= MATCH_OP_MASK;

    name = LookupValue(symInterpretMatchMaskNames, type);
    if (name)
        return name;

    buf = GetBuffer(40);
    snprintf(buf, 40, "0x%x", type);
    return buf;
}
