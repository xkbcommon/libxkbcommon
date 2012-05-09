/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#include "text.h"

#define BUFFER_SIZE 1024
static char textBuffer[BUFFER_SIZE];
static unsigned int tbNext = 0;

static char *
tbGetBuffer(unsigned int size)
{
    char *rtrn;

    if (size >= BUFFER_SIZE)
        return NULL;

    if ((BUFFER_SIZE - tbNext) <= size)
        tbNext = 0;

    rtrn = &textBuffer[tbNext];
    tbNext += size;

    return rtrn;
}

static const char *
XkbcVModIndexText(struct xkb_keymap *keymap, unsigned ndx)
{
    int len;
    char *rtrn;
    const char *tmp = NULL;
    char buf[20];

    if (ndx >= XkbNumVirtualMods)
         tmp = "illegal";
    else if (keymap && keymap->names)
         tmp = keymap->names->vmods[ndx];

    if (!tmp) {
        snprintf(buf, sizeof(buf) - 1, "%d", ndx);
        tmp = buf;
    }

    len = strlen(tmp) + 1;
    if (len >= BUFFER_SIZE)
        len = BUFFER_SIZE - 1;

    rtrn = tbGetBuffer(len);
    strncpy(rtrn, tmp, len);

    return rtrn;
}

const char *
XkbcVModMaskText(struct xkb_keymap *keymap, unsigned modMask, unsigned mask)
{
    int i, bit, len, rem;
    const char *mm = NULL;
    char *rtrn, *str;
    char buf[BUFFER_SIZE];

    if ((modMask == 0) && (mask == 0))
        return "none";

    if (modMask != 0)
        mm = XkbcModMaskText(modMask, false);

    str = buf;
    buf[0]= '\0';
    rem = BUFFER_SIZE;

    if (mask) {
        for (i = 0, bit = 1; i < XkbNumVirtualMods && rem > 1; i++, bit <<= 1)
        {
            if (!(mask & bit))
                continue;

            len = snprintf(str, rem, "%s%s",
                           (str != buf) ? "+" : "",
                           XkbcVModIndexText(keymap, i));
            rem -= len;
            str += len;
        }

        str = buf;
    }
    else
        str = NULL;

    len = ((str) ? strlen(str) : 0) + ((mm) ? strlen(mm) : 0) +
          ((str && mm) ? 1 : 0);
    if (len >= BUFFER_SIZE)
        len = BUFFER_SIZE - 1;

    rtrn = tbGetBuffer(len + 1);
    rtrn[0] = '\0';

    snprintf(rtrn, len + 1, "%s%s%s", (mm) ? mm : "",
             (mm && str) ? "+" : "", (str) ? str : "");

    return rtrn;
}

static const char *modNames[XkbNumModifiers] = {
    "Shift",
    "Lock",
    "Control",
    "Mod1",
    "Mod2",
    "Mod3",
    "Mod4",
    "Mod5"
};

const char *
XkbcModIndexText(unsigned ndx)
{
    char *buf;

    if (ndx < XkbNumModifiers)
        return modNames[ndx];
    else if (ndx == XkbNoModifier)
        return "none";

    buf = tbGetBuffer(32);
    snprintf(buf, 32, "ILLEGAL_%02x", ndx);

    return buf;
}

const char *
XkbcModMaskText(unsigned mask, bool cFormat)
{
    int i, rem, bit;
    char *str, *buf;

    if ((mask & 0xff) == 0xff)
        return (cFormat ? "0xff" : "all");

    if ((mask & 0xff) == 0)
        return (cFormat ? "0" : "none");

    rem = 64;
    buf = tbGetBuffer(rem);
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
XkbcConfigText(unsigned config)
{
    switch (config) {
    case XkmSemanticsFile:
        return "Semantics";
    case XkmLayoutFile:
        return "Layout";
    case XkmKeymapFile:
        return "Keymap";
    case XkmTypesIndex:
        return "Types";
    case XkmCompatMapIndex:
        return "CompatMap";
    case XkmSymbolsIndex:
        return "Symbols";
    case XkmKeyNamesIndex:
        return "KeyNames";
    default:
        return "unknown";
    }
}

static const char *actionTypeNames[XkbSA_NumActions]= {
    "NoAction",         /* XkbSA_NoAction */
    "SetMods",          /* XkbSA_SetMods */
    "LatchMods",        /* XkbSA_LatchMods */
    "LockMods",         /* XkbSA_LockMods */
    "SetGroup",         /* XkbSA_SetGroup */
    "LatchGroup",       /* XkbSA_LatchGroup */
    "LockGroup",        /* XkbSA_LockGroup */
    "MovePtr",          /* XkbSA_MovePtr */
    "PtrBtn",           /* XkbSA_PtrBtn */
    "LockPtrBtn",       /* XkbSA_LockPtrBtn */
    "SetPtrDflt",       /* XkbSA_SetPtrDflt */
    "ISOLock",          /* XkbSA_ISOLock */
    "Terminate",        /* XkbSA_Terminate */
    "SwitchScreen",     /* XkbSA_SwitchScreen */
    "SetControls",      /* XkbSA_SetControls */
    "LockControls",     /* XkbSA_LockControls */
    "ActionMessage",    /* XkbSA_ActionMessage */
    "RedirectKey",      /* XkbSA_RedirectKey */
    "DeviceBtn",        /* XkbSA_DeviceBtn */
    "LockDeviceBtn",    /* XkbSA_LockDeviceBtn */
    "DeviceValuator"    /* XkbSA_DeviceValuator */
};

const char *
XkbcActionTypeText(unsigned type)
{
    if (type <= XkbSA_LastAction)
        return actionTypeNames[type];
    return "Private";
}

const char *
XkbcKeysymText(xkb_keysym_t sym)
{
    static char buffer[16];

    xkb_keysym_get_name(sym, buffer, sizeof buffer);

    return buffer;
}

const char *
XkbcKeyNameText(char *name)
{
    char *buf;
    int len;

    buf = tbGetBuffer(7);
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
XkbcSIMatchText(unsigned type)
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
        buf = tbGetBuffer(40);
        snprintf(buf, 40, "0x%x", type & XkbSI_OpMask);
        return buf;
    }
}
