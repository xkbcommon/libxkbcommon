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

#include "text.h"

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

static const char *
VModIndexText(struct xkb_keymap *keymap, xkb_mod_index_t ndx)
{
    int len;
    char *rtrn;
    const char *tmp = NULL;
    char buf[20];

    if (ndx >= XkbNumVirtualMods)
         tmp = "illegal";
    else
         tmp = keymap->vmod_names[ndx];

    if (!tmp) {
        snprintf(buf, sizeof(buf) - 1, "%d", ndx);
        tmp = buf;
    }

    len = strlen(tmp) + 1;
    if (len >= BUFFER_SIZE)
        len = BUFFER_SIZE - 1;

    rtrn = GetBuffer(len);
    strncpy(rtrn, tmp, len);

    return rtrn;
}

const char *
VModMaskText(struct xkb_keymap *keymap, xkb_mod_mask_t modMask,
             xkb_mod_mask_t mask)
{
    xkb_mod_index_t i;
    xkb_mod_mask_t bit;
    int len, rem;
    const char *mm = NULL;
    char *rtrn, *str;
    char buf[BUFFER_SIZE];

    if (modMask == 0 && mask == 0)
        return "none";

    if (modMask != 0)
        mm = ModMaskText(modMask, false);

    str = buf;
    buf[0] = '\0';
    rem = BUFFER_SIZE;

    if (mask) {
        for (i = 0, bit = 1; i < XkbNumVirtualMods && rem > 1; i++, bit <<=
                 1) {
            if (!(mask & bit))
                continue;

            len = snprintf(str, rem, "%s%s",
                           (str != buf) ? "+" : "",
                           VModIndexText(keymap, i));
            rem -= len;
            str += len;
        }

        str = buf;
    }
    else
        str = NULL;

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

/*
 * IMPORTATNT
 * The indices used for the legacy core modifiers is derived from
 * the order of the names in this table. It matches the values
 * ShiftMapIndex, LockMapIndex, etc. from X11/X.h. Take note before
 * changing.
 */
static const char *modNames[XkbNumModifiers] = {
    "Shift",
    "Lock",
    "Control",
    "Mod1",
    "Mod2",
    "Mod3",
    "Mod4",
    "Mod5",
};

xkb_mod_index_t
ModNameToIndex(const char *name)
{
    xkb_mod_index_t i;

    for (i = 0; i < XkbNumModifiers; i++)
        if (istreq(name, modNames[i]))
            return i;

    return XKB_MOD_INVALID;
}

const char *
ModIndexToName(xkb_mod_index_t ndx)
{
    if (ndx < XkbNumModifiers)
        return modNames[ndx];
    return NULL;
}

const char *
ModIndexText(xkb_mod_index_t ndx)
{
    const char *name;
    char *buf;

    name = ModIndexToName(ndx);
    if (name)
        return name;

    if (ndx == XkbNoModifier)
        return "none";

    buf = GetBuffer(32);
    snprintf(buf, 32, "ILLEGAL_%02x", ndx);

    return buf;
}

const char *
ModMaskText(xkb_mod_mask_t mask, bool cFormat)
{
    int i, rem;
    xkb_mod_index_t bit;
    char *str, *buf;

    if ((mask & 0xff) == 0xff)
        return (cFormat ? "0xff" : "all");

    if ((mask & 0xff) == 0)
        return (cFormat ? "0" : "none");

    rem = 64;
    buf = GetBuffer(rem);
    str = buf;
    buf[0] = '\0';
    for (i = 0, bit = 1; i < XkbNumModifiers && rem > 1; i++, bit <<= 1) {
        int len;

        if (!(mask & bit))
            continue;

        len = snprintf(str, rem, "%s%s%s",
                       (str != buf) ? (cFormat ? "|" : "+") : "",
                       modNames[i],
                       cFormat ? "Mask" : "");
        rem -= len;
        str += len;
    }

    return buf;
}

const char *
FileTypeText(enum xkb_file_type type)
{
    switch (type) {
    case FILE_TYPE_KEYMAP:
        return "xkb_keymap";
    case FILE_TYPE_TYPES:
        return "xkb_types";
    case FILE_TYPE_COMPAT:
        return "xkb_compatability";
    case FILE_TYPE_SYMBOLS:
        return "xkb_symbols";
    case FILE_TYPE_KEYCODES:
        return "xkb_keycodes";
    case FILE_TYPE_RULES:
        return "rules";
    default:
        return "unknown";
    }
}

static const char *actionTypeNames[XkbSA_NumActions] = {
    [XkbSA_NoAction]       = "NoAction",
    [XkbSA_SetMods]        = "SetMods",
    [XkbSA_LatchMods]      = "LatchMods",
    [XkbSA_LockMods]       = "LockMods",
    [XkbSA_SetGroup]       = "SetGroup",
    [XkbSA_LatchGroup]     = "LatchGroup",
    [XkbSA_LockGroup]      = "LockGroup",
    [XkbSA_MovePtr]        = "MovePtr",
    [XkbSA_PtrBtn]         = "PtrBtn",
    [XkbSA_LockPtrBtn]     = "LockPtrBtn",
    [XkbSA_SetPtrDflt]     = "SetPtrDflt",
    [XkbSA_ISOLock]        = "ISOLock",
    [XkbSA_Terminate]      = "Terminate",
    [XkbSA_SwitchScreen]   = "SwitchScreen",
    [XkbSA_SetControls]    = "SetControls",
    [XkbSA_LockControls]   = "LockControls",
    [XkbSA_ActionMessage]  = "ActionMessage",
    [XkbSA_RedirectKey]    = "RedirectKey",
    [XkbSA_DeviceBtn]      = "DeviceBtn",
    [XkbSA_LockDeviceBtn]  = "LockDeviceBtn",
    [XkbSA_DeviceValuator] = "DeviceValuator"
};

const char *
ActionTypeText(unsigned type)
{
    if (type <= XkbSA_LastAction)
        return actionTypeNames[type];
    return "Private";
}

const char *
KeysymText(xkb_keysym_t sym)
{
    static char buffer[64];

    xkb_keysym_get_name(sym, buffer, sizeof buffer);

    return buffer;
}

const char *
KeyNameText(const char name[XkbKeyNameLength])
{
    char *buf;
    int len;

    buf = GetBuffer(7);
    buf[0] = '<';
    strncpy(&buf[1], name, 4);
    buf[5] = '\0';
    len = strlen(buf);
    buf[len++] = '>';
    buf[len] = '\0';

    return buf;
}

static const char *siMatchText[5] = {
    "NoneOf",       /* XkbSI_NoneOf */
    "AnyOfOrNone",  /* XkbSI_AnyOfOrNone */
    "AnyOf",        /* XkbSI_AnyOf */
    "AllOf",        /* XkbSI_AllOf */
    "Exactly"       /* XkbSI_Exactly */
};

const char *
SIMatchText(unsigned type)
{
    char *buf;

    switch (type & XkbSI_OpMask) {
    case XkbSI_NoneOf:
        return siMatchText[0];
    case XkbSI_AnyOfOrNone:
        return siMatchText[1];
    case XkbSI_AnyOf:
        return siMatchText[2];
    case XkbSI_AllOf:
        return siMatchText[3];
    case XkbSI_Exactly:
        return siMatchText[4];
    default:
        buf = GetBuffer(40);
        snprintf(buf, 40, "0x%x", type & XkbSI_OpMask);
        return buf;
    }
}
