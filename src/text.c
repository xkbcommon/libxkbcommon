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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "X11/extensions/XKBcommon.h"

char *
XkbConfigText(unsigned config)
{
    switch (config) {
    case XkmSemanticsFile:
        return "Semantics";
    case XkmLayoutFile:
        return "Layout";
    case XkmKeymapFile:
        return "Keymap";
    case XkmGeometryFile:
    case XkmGeometryIndex:
        return "Geometry";
    case XkmTypesIndex:
        return "Types";
    case XkmCompatMapIndex:
        return "CompatMap";
    case XkmSymbolsIndex:
        return "Symbols";
    case XkmIndicatorsIndex:
        return "Indicators";
    case XkmKeyNamesIndex:
        return "KeyNames";
    case XkmVirtualModsIndex:
        return "VirtualMods";
    default:
        return "unknown";
    }
}

static char *actionTypeNames[XkbSA_NumActions]= {
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

char *
XkbActionTypeText(unsigned type)
{
    if (type <= XkbSA_LastAction)
        return actionTypeNames[type];
    return "Private";
}
