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

#include "action.h"
#include "keycodes.h"

static bool actionsInitialized;
static ExprDef constTrue;
static ExprDef constFalse;

/***====================================================================***/

static const LookupEntry actionStrings[] = {
    { "noaction",          XkbSA_NoAction       },
    { "setmods",           XkbSA_SetMods        },
    { "latchmods",         XkbSA_LatchMods      },
    { "lockmods",          XkbSA_LockMods       },
    { "setgroup",          XkbSA_SetGroup       },
    { "latchgroup",        XkbSA_LatchGroup     },
    { "lockgroup",         XkbSA_LockGroup      },
    { "moveptr",           XkbSA_MovePtr        },
    { "movepointer",       XkbSA_MovePtr        },
    { "ptrbtn",            XkbSA_PtrBtn         },
    { "pointerbutton",     XkbSA_PtrBtn         },
    { "lockptrbtn",        XkbSA_LockPtrBtn     },
    { "lockpointerbutton", XkbSA_LockPtrBtn     },
    { "lockptrbutton",     XkbSA_LockPtrBtn     },
    { "lockpointerbtn",    XkbSA_LockPtrBtn     },
    { "setptrdflt",        XkbSA_SetPtrDflt     },
    { "setpointerdefault", XkbSA_SetPtrDflt     },
    { "isolock",           XkbSA_ISOLock        },
    { "terminate",         XkbSA_Terminate      },
    { "terminateserver",   XkbSA_Terminate      },
    { "switchscreen",      XkbSA_SwitchScreen   },
    { "setcontrols",       XkbSA_SetControls    },
    { "lockcontrols",      XkbSA_LockControls   },
    { "actionmessage",     XkbSA_ActionMessage  },
    { "messageaction",     XkbSA_ActionMessage  },
    { "message",           XkbSA_ActionMessage  },
    { "redirect",          XkbSA_RedirectKey    },
    { "redirectkey",       XkbSA_RedirectKey    },
    { "devbtn",            XkbSA_DeviceBtn      },
    { "devicebtn",         XkbSA_DeviceBtn      },
    { "devbutton",         XkbSA_DeviceBtn      },
    { "devicebutton",      XkbSA_DeviceBtn      },
    { "lockdevbtn",        XkbSA_DeviceBtn      },
    { "lockdevicebtn",     XkbSA_LockDeviceBtn  },
    { "lockdevbutton",     XkbSA_LockDeviceBtn  },
    { "lockdevicebutton",  XkbSA_LockDeviceBtn  },
    { "devval",            XkbSA_DeviceValuator },
    { "deviceval",         XkbSA_DeviceValuator },
    { "devvaluator",       XkbSA_DeviceValuator },
    { "devicevaluator",    XkbSA_DeviceValuator },
    { "private",           PrivateAction        },
    { NULL,                0                    }
};

static const LookupEntry fieldStrings[] = {
    { "clearLocks",       F_ClearLocks  },
    { "latchToLock",      F_LatchToLock },
    { "genKeyEvent",      F_GenKeyEvent },
    { "generateKeyEvent", F_GenKeyEvent },
    { "report",           F_Report      },
    { "default",          F_Default     },
    { "affect",           F_Affect      },
    { "increment",        F_Increment   },
    { "modifiers",        F_Modifiers   },
    { "mods",             F_Modifiers   },
    { "group",            F_Group       },
    { "x",                F_X           },
    { "y",                F_Y           },
    { "accel",            F_Accel       },
    { "accelerate",       F_Accel       },
    { "repeat",           F_Accel       },
    { "button",           F_Button      },
    { "value",            F_Value       },
    { "controls",         F_Controls    },
    { "ctrls",            F_Controls    },
    { "type",             F_Type        },
    { "count",            F_Count       },
    { "screen",           F_Screen      },
    { "same",             F_Same        },
    { "sameServer",       F_Same        },
    { "data",             F_Data        },
    { "device",           F_Device      },
    { "dev",              F_Device      },
    { "key",              F_Keycode     },
    { "keycode",          F_Keycode     },
    { "kc",               F_Keycode     },
    { "clearmods",        F_ModsToClear },
    { "clearmodifiers",   F_ModsToClear },
    { NULL,               0             }
};

static bool
stringToValue(const LookupEntry tab[], const char *string,
              unsigned *value_rtrn)
{
    const LookupEntry *entry;

    if (!string)
        return false;

    for (entry = tab; entry->name != NULL; entry++) {
        if (strcasecmp(entry->name, string) == 0) {
            *value_rtrn = entry->result;
            return true;
        }
    }

    return false;
}

static const char *
valueToString(const LookupEntry tab[], unsigned value)
{
    const LookupEntry *entry;

    for (entry = tab; entry->name != NULL; entry++)
        if (entry->result == value)
            return entry->name;

    return "unknown";
}

static bool
stringToAction(const char *str, unsigned *type_rtrn)
{
    return stringToValue(actionStrings, str, type_rtrn);
}

static bool
stringToField(const char *str, unsigned *field_rtrn)
{
    return stringToValue(fieldStrings, str, field_rtrn);
}

static const char *
fieldText(unsigned field)
{
    return valueToString(fieldStrings, field);
}

/***====================================================================***/

static bool
ReportMismatch(unsigned action, unsigned field, const char *type)
{
    ERROR("Value of %s field must be of type %s\n", fieldText(field), type);
    ACTION("Action %s definition ignored\n", XkbcActionTypeText(action));
    return false;
}

static bool
ReportIllegal(unsigned action, unsigned field)
{
    ERROR("Field %s is not defined for an action of type %s\n",
           fieldText(field), XkbcActionTypeText(action));
    ACTION("Action definition ignored\n");
    return false;
}

static bool
ReportActionNotArray(unsigned action, unsigned field)
{
    ERROR("The %s field in the %s action is not an array\n",
           fieldText(field), XkbcActionTypeText(action));
    ACTION("Action definition ignored\n");
    return false;
}

static bool
ReportNotFound(unsigned action, unsigned field, const char *what,
               const char *bad)
{
    ERROR("%s named %s not found\n", what, bad);
    ACTION("Ignoring the %s field of an %s action\n", fieldText(field),
            XkbcActionTypeText(action));
    return false;
}

static bool
HandleNoAction(struct xkb_keymap *keymap, struct xkb_any_action *action,
               unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    return ReportIllegal(action->type, field);
}

static bool
CheckLatchLockFlags(unsigned action,
                    unsigned field, ExprDef * value, unsigned *flags_inout)
{
    unsigned tmp;
    ExprResult result;

    if (field == F_ClearLocks)
        tmp = XkbSA_ClearLocks;
    else if (field == F_LatchToLock)
        tmp = XkbSA_LatchToLock;
    else
        return false;           /* WSGO! */
    if (!ExprResolveBoolean(value, &result))
        return ReportMismatch(action, field, "boolean");
    if (result.uval)
        *flags_inout |= tmp;
    else
        *flags_inout &= ~tmp;
    return true;
}

static bool
CheckModifierField(struct xkb_keymap *keymap, unsigned action, ExprDef *value,
                   unsigned *flags_inout, unsigned *mods_rtrn)
{
    ExprResult rtrn;

    if (value->op == ExprIdent)
    {
        const char *valStr;
        valStr = XkbcAtomText(value->value.str);
        if (valStr && ((strcasecmp(valStr, "usemodmapmods") == 0) ||
                       (strcasecmp(valStr, "modmapmods") == 0)))
        {

            *mods_rtrn = 0;
            *flags_inout |= XkbSA_UseModMapMods;
            return true;
        }
    }
    if (!ExprResolveVModMask(value, &rtrn, keymap))
        return ReportMismatch(action, F_Modifiers, "modifier mask");
    *mods_rtrn = rtrn.uval;
    *flags_inout &= ~XkbSA_UseModMapMods;
    return true;
}

static bool
HandleSetLatchMods(struct xkb_keymap *keymap, struct xkb_any_action *action,
                   unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    struct xkb_mod_action *act;
    unsigned rtrn;
    unsigned t1, t2;

    act = (struct xkb_mod_action *) action;
    if (array_ndx != NULL)
    {
        switch (field)
        {
        case F_ClearLocks:
        case F_LatchToLock:
        case F_Modifiers:
            return ReportActionNotArray(action->type, field);
        }
    }
    switch (field)
    {
    case F_ClearLocks:
    case F_LatchToLock:
        rtrn = act->flags;
        if (CheckLatchLockFlags(action->type, field, value, &rtrn))
        {
            act->flags = rtrn;
            return true;
        }
        return false;
    case F_Modifiers:
        t1 = act->flags;
        if (CheckModifierField(keymap, action->type, value, &t1, &t2))
        {
            act->flags = t1;
            act->real_mods = act->mask = (t2 & 0xff);
            act->vmods = (t2 >> 8) & 0xffff;
            return true;
        }
        return false;
    }
    return ReportIllegal(action->type, field);
}

static bool
HandleLockMods(struct xkb_keymap *keymap, struct xkb_any_action *action,
               unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    struct xkb_mod_action *act;
    unsigned t1, t2;

    act = (struct xkb_mod_action *) action;
    if ((array_ndx != NULL) && (field == F_Modifiers))
        return ReportActionNotArray(action->type, field);
    switch (field)
    {
    case F_Modifiers:
        t1 = act->flags;
        if (CheckModifierField(keymap, action->type, value, &t1, &t2))
        {
            act->flags = t1;
            act->real_mods = act->mask = (t2 & 0xff);
            act->vmods = (t2 >> 8) & 0xffff;
            return true;
        }
        return false;
    }
    return ReportIllegal(action->type, field);
}

static bool
CheckGroupField(unsigned action,
                ExprDef * value, unsigned *flags_inout, int *grp_rtrn)
{
    ExprDef *spec;
    ExprResult rtrn;

    if ((value->op == OpNegate) || (value->op == OpUnaryPlus))
    {
        *flags_inout &= ~XkbSA_GroupAbsolute;
        spec = value->value.child;
    }
    else
    {
        *flags_inout |= XkbSA_GroupAbsolute;
        spec = value;
    }

    if (!ExprResolveGroup(spec, &rtrn))
        return ReportMismatch(action, F_Group, "integer (range 1..8)");
    if (value->op == OpNegate)
        *grp_rtrn = -rtrn.ival;
    else if (value->op == OpUnaryPlus)
        *grp_rtrn = rtrn.ival;
    else
        *grp_rtrn = rtrn.ival - 1;
    return true;
}

static bool
HandleSetLatchGroup(struct xkb_keymap *keymap, struct xkb_any_action *action,
                    unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    struct xkb_group_action *act;
    unsigned rtrn;
    unsigned t1;
    int t2;

    act = (struct xkb_group_action *) action;
    if (array_ndx != NULL)
    {
        switch (field)
        {
        case F_ClearLocks:
        case F_LatchToLock:
        case F_Group:
            return ReportActionNotArray(action->type, field);
        }
    }
    switch (field)
    {
    case F_ClearLocks:
    case F_LatchToLock:
        rtrn = act->flags;
        if (CheckLatchLockFlags(action->type, field, value, &rtrn))
        {
            act->flags = rtrn;
            return true;
        }
        return false;
    case F_Group:
        t1 = act->flags;
        if (CheckGroupField(action->type, value, &t1, &t2))
        {
            act->flags = t1;
	    act->group = t2;
            return true;
        }
        return false;
    }
    return ReportIllegal(action->type, field);
}

static bool
HandleLockGroup(struct xkb_keymap *keymap, struct xkb_any_action *action,
                unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    struct xkb_group_action *act;
    unsigned t1;
    int t2;

    act = (struct xkb_group_action *) action;
    if ((array_ndx != NULL) && (field == F_Group))
        return ReportActionNotArray(action->type, field);
    if (field == F_Group)
    {
        t1 = act->flags;
        if (CheckGroupField(action->type, value, &t1, &t2))
        {
            act->flags = t1;
	    act->group = t2;
            return true;
        }
        return false;
    }
    return ReportIllegal(action->type, field);
}

static bool
HandleMovePtr(struct xkb_keymap *keymap, struct xkb_any_action *action,
              unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    ExprResult rtrn;
    struct xkb_pointer_action *act;
    bool absolute;

    act = (struct xkb_pointer_action *) action;
    if ((array_ndx != NULL) && ((field == F_X) || (field == F_Y)))
        return ReportActionNotArray(action->type, field);

    if ((field == F_X) || (field == F_Y))
    {
        if ((value->op == OpNegate) || (value->op == OpUnaryPlus))
            absolute = false;
        else
            absolute = true;
        if (!ExprResolveInteger(value, &rtrn))
            return ReportMismatch(action->type, field, "integer");
        if (field == F_X)
        {
            if (absolute)
                act->flags |= XkbSA_MoveAbsoluteX;
            act->x = rtrn.ival;
        }
        else
        {
            if (absolute)
                act->flags |= XkbSA_MoveAbsoluteY;
            act->y = rtrn.ival;
        }
        return true;
    }
    else if (field == F_Accel)
    {
        if (!ExprResolveBoolean(value, &rtrn))
            return ReportMismatch(action->type, field, "boolean");
        if (rtrn.uval)
            act->flags &= ~XkbSA_NoAcceleration;
        else
            act->flags |= XkbSA_NoAcceleration;
    }
    return ReportIllegal(action->type, field);
}

static const LookupEntry lockWhich[] = {
    {"both", 0},
    {"lock", XkbSA_LockNoUnlock},
    {"neither", (XkbSA_LockNoLock | XkbSA_LockNoUnlock)},
    {"unlock", XkbSA_LockNoLock},
    {NULL, 0}
};

static bool
HandlePtrBtn(struct xkb_keymap *keymap, struct xkb_any_action *action,
             unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    ExprResult rtrn;
    struct xkb_pointer_button_action *act;

    act = (struct xkb_pointer_button_action *) action;
    if (field == F_Button)
    {
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveButton(value, &rtrn))
            return ReportMismatch(action->type, field,
                                  "integer (range 1..5)");
        if ((rtrn.ival < 0) || (rtrn.ival > 5))
        {
            ERROR("Button must specify default or be in the range 1..5\n");
            ACTION("Illegal button value %d ignored\n", rtrn.ival);
            return false;
        }
        act->button = rtrn.ival;
        return true;
    }
    else if ((action->type == XkbSA_LockPtrBtn) && (field == F_Affect))
    {
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveEnum(value, &rtrn, lockWhich))
            return ReportMismatch(action->type, field, "lock or unlock");
        act->flags &= ~(XkbSA_LockNoLock | XkbSA_LockNoUnlock);
        act->flags |= rtrn.ival;
        return true;
    }
    else if (field == F_Count)
    {
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveButton(value, &rtrn))
            return ReportMismatch(action->type, field, "integer");
        if ((rtrn.ival < 0) || (rtrn.ival > 255))
        {
            ERROR("The count field must have a value in the range 0..255\n");
            ACTION("Illegal count %d ignored\n", rtrn.ival);
            return false;
        }
        act->count = rtrn.ival;
        return true;
    }
    return ReportIllegal(action->type, field);
}

static const LookupEntry ptrDflts[] = {
    {"dfltbtn", XkbSA_AffectDfltBtn},
    {"defaultbutton", XkbSA_AffectDfltBtn},
    {"button", XkbSA_AffectDfltBtn},
    {NULL, 0}
};

static bool
HandleSetPtrDflt(struct xkb_keymap *keymap, struct xkb_any_action *action,
                 unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    ExprResult rtrn;
    struct xkb_pointer_default_action *act;

    act = (struct xkb_pointer_default_action *) action;
    if (field == F_Affect)
    {
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveEnum(value, &rtrn, ptrDflts))
            return ReportMismatch(action->type, field, "pointer component");
        act->affect = rtrn.uval;
        return true;
    }
    else if ((field == F_Button) || (field == F_Value))
    {
        ExprDef *btn;
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if ((value->op == OpNegate) || (value->op == OpUnaryPlus))
        {
            act->flags &= ~XkbSA_DfltBtnAbsolute;
            btn = value->value.child;
        }
        else
        {
            act->flags |= XkbSA_DfltBtnAbsolute;
            btn = value;
        }

        if (!ExprResolveButton(btn, &rtrn))
            return ReportMismatch(action->type, field,
                                  "integer (range 1..5)");
        if ((rtrn.ival < 0) || (rtrn.ival > 5))
        {
            ERROR("New default button value must be in the range 1..5\n");
            ACTION("Illegal default button value %d ignored\n", rtrn.ival);
            return false;
        }
        if (rtrn.ival == 0)
        {
            ERROR("Cannot set default pointer button to \"default\"\n");
            ACTION("Illegal default button setting ignored\n");
            return false;
        }
        if (value->op == OpNegate)
	    act->value = -rtrn.ival;
        else
	    act->value = rtrn.ival;
        return true;
    }
    return ReportIllegal(action->type, field);
}

static const LookupEntry isoNames[] = {
    {"mods", XkbSA_ISONoAffectMods},
    {"modifiers", XkbSA_ISONoAffectMods},
    {"group", XkbSA_ISONoAffectGroup},
    {"groups", XkbSA_ISONoAffectGroup},
    {"ptr", XkbSA_ISONoAffectPtr},
    {"pointer", XkbSA_ISONoAffectPtr},
    {"ctrls", XkbSA_ISONoAffectCtrls},
    {"controls", XkbSA_ISONoAffectCtrls},
    {"all", ~((unsigned) 0)},
    {"none", 0},
    {NULL, 0},
};

static bool
HandleISOLock(struct xkb_keymap *keymap, struct xkb_any_action *action,
              unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    ExprResult rtrn;
    struct xkb_iso_action *act;
    unsigned flags, mods;
    int group;

    act = (struct xkb_iso_action *) action;
    switch (field)
    {
    case F_Modifiers:
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        flags = act->flags;
        if (CheckModifierField(keymap, action->type, value, &flags, &mods))
        {
            act->flags = flags & (~XkbSA_ISODfltIsGroup);
            act->real_mods = mods & 0xff;
            act->vmods = (mods >> 8) & 0xff;
            return true;
        }
        return false;
    case F_Group:
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        flags = act->flags;
        if (CheckGroupField(action->type, value, &flags, &group))
        {
            act->flags = flags | XkbSA_ISODfltIsGroup;
            act->group = group;
            return true;
        }
        return false;
    case F_Affect:
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveMask(value, &rtrn, isoNames))
            return ReportMismatch(action->type, field, "keyboard component");
        act->affect = (~rtrn.uval) & XkbSA_ISOAffectMask;
        return true;
    }
    return ReportIllegal(action->type, field);
}

static bool
HandleSwitchScreen(struct xkb_keymap *keymap, struct xkb_any_action *action,
                   unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    ExprResult rtrn;
    struct xkb_switch_screen_action *act;

    act = (struct xkb_switch_screen_action *) action;
    if (field == F_Screen)
    {
        ExprDef *scrn;
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if ((value->op == OpNegate) || (value->op == OpUnaryPlus))
        {
            act->flags &= ~XkbSA_SwitchAbsolute;
            scrn = value->value.child;
        }
        else
        {
            act->flags |= XkbSA_SwitchAbsolute;
            scrn = value;
        }

        if (!ExprResolveInteger(scrn, &rtrn))
            return ReportMismatch(action->type, field, "integer (0..255)");
        if ((rtrn.ival < 0) || (rtrn.ival > 255))
        {
            ERROR("Screen index must be in the range 1..255\n");
            ACTION("Illegal screen value %d ignored\n", rtrn.ival);
            return false;
        }
        if (value->op == OpNegate)
	    act->screen = -rtrn.ival;
        else
	    act->screen = rtrn.ival;
        return true;
    }
    else if (field == F_Same)
    {
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveBoolean(value, &rtrn))
            return ReportMismatch(action->type, field, "boolean");
        if (rtrn.uval)
            act->flags &= ~XkbSA_SwitchApplication;
        else
            act->flags |= XkbSA_SwitchApplication;
        return true;
    }
    return ReportIllegal(action->type, field);
}

const LookupEntry ctrlNames[] = {
    {"repeatkeys", XkbRepeatKeysMask},
    {"repeat", XkbRepeatKeysMask},
    {"autorepeat", XkbRepeatKeysMask},
    {"slowkeys", XkbSlowKeysMask},
    {"bouncekeys", XkbBounceKeysMask},
    {"stickykeys", XkbStickyKeysMask},
    {"mousekeys", XkbMouseKeysMask},
    {"mousekeysaccel", XkbMouseKeysAccelMask},
    {"accessxkeys", XkbAccessXKeysMask},
    {"accessxtimeout", XkbAccessXTimeoutMask},
    {"accessxfeedback", XkbAccessXFeedbackMask},
    {"audiblebell", XkbAudibleBellMask},
    {"ignoregrouplock", XkbIgnoreGroupLockMask},
    {"all", XkbAllBooleanCtrlsMask},
    {"overlay1", 0},
    {"overlay2", 0},
    {"none", 0},
    {NULL, 0}
};

static bool
HandleSetLockControls(struct xkb_keymap *keymap, struct xkb_any_action *action,
                      unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    ExprResult rtrn;
    struct xkb_controls_action *act;

    act = (struct xkb_controls_action *) action;
    if (field == F_Controls)
    {
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveMask(value, &rtrn, ctrlNames))
            return ReportMismatch(action->type, field, "controls mask");
        act->ctrls = rtrn.uval;
        return true;
    }
    return ReportIllegal(action->type, field);
}

static const LookupEntry evNames[] = {
    {"press", XkbSA_MessageOnPress},
    {"keypress", XkbSA_MessageOnPress},
    {"release", XkbSA_MessageOnRelease},
    {"keyrelease", XkbSA_MessageOnRelease},
    {"all", XkbSA_MessageOnPress | XkbSA_MessageOnRelease},
    {"none", 0},
    {NULL, 0}
};

static bool
HandleActionMessage(struct xkb_keymap *keymap, struct xkb_any_action *action,
                    unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    ExprResult rtrn;
    struct xkb_message_action *act;

    act = (struct xkb_message_action *) action;
    switch (field)
    {
    case F_Report:
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveMask(value, &rtrn, evNames))
            return ReportMismatch(action->type, field, "key event mask");
        act->flags &= ~(XkbSA_MessageOnPress | XkbSA_MessageOnRelease);
        act->flags =
            rtrn.uval & (XkbSA_MessageOnPress | XkbSA_MessageOnRelease);
        return true;
    case F_GenKeyEvent:
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveBoolean(value, &rtrn))
            return ReportMismatch(action->type, field, "boolean");
        if (rtrn.uval)
            act->flags |= XkbSA_MessageGenKeyEvent;
        else
            act->flags &= ~XkbSA_MessageGenKeyEvent;
        return true;
    case F_Data:
        if (array_ndx == NULL)
        {
            if (!ExprResolveString(keymap, value, &rtrn))
                return ReportMismatch(action->type, field, "string");
            else
            {
                int len = strlen(rtrn.str);
                if ((len < 1) || (len > 6))
                {
                    WARN("An action message can hold only 6 bytes\n");
                    ACTION("Extra %d bytes ignored\n", len - 6);
                }
                strncpy((char *) act->message, rtrn.str, 6);
            }
            return true;
        }
        else
        {
            unsigned ndx;
            if (!ExprResolveInteger(array_ndx, &rtrn))
            {
                ERROR("Array subscript must be integer\n");
                ACTION("Illegal subscript ignored\n");
                return false;
            }
            ndx = rtrn.uval;
            if (ndx > 5)
            {
                ERROR("An action message is at most 6 bytes long\n");
                ACTION("Attempt to use data[%d] ignored\n", ndx);
                return false;
            }
            if (!ExprResolveInteger(value, &rtrn))
                return ReportMismatch(action->type, field, "integer");
            if ((rtrn.ival < 0) || (rtrn.ival > 255))
            {
                ERROR("Message data must be in the range 0..255\n");
                ACTION("Illegal datum %d ignored\n", rtrn.ival);
                return false;
            }
            act->message[ndx] = rtrn.uval;
        }
        return true;
    }
    return ReportIllegal(action->type, field);
}

static bool
HandleRedirectKey(struct xkb_keymap *keymap, struct xkb_any_action *action,
                  unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    ExprResult rtrn;
    struct xkb_redirect_key_action *act;
    unsigned t1, t2;
    xkb_keycode_t kc;
    unsigned long tmp;

    if (array_ndx != NULL)
        return ReportActionNotArray(action->type, field);

    act = (struct xkb_redirect_key_action *) action;
    switch (field)
    {
    case F_Keycode:
        if (!ExprResolveKeyName(value, &rtrn))
            return ReportMismatch(action->type, field, "key name");
        tmp = KeyNameToLong(rtrn.keyName.name);
        if (!FindNamedKey(keymap, tmp, &kc, true, CreateKeyNames(keymap), 0))
        {
            return ReportNotFound(action->type, field, "Key",
                                  XkbcKeyNameText(rtrn.keyName.name));
        }
        act->new_key = kc;
        return true;
    case F_ModsToClear:
    case F_Modifiers:
        t1 = 0;
        if (CheckModifierField(keymap, action->type, value, &t1, &t2))
        {
            act->mods_mask |= (t2 & 0xff);
            if (field == F_Modifiers)
                act->mods |= (t2 & 0xff);
            else
                act->mods &= ~(t2 & 0xff);

            t2 = (t2 >> 8) & 0xffff;
            act->vmods_mask |= t2;
            if (field == F_Modifiers)
                act->vmods |= t2;
            else
                act->vmods &= ~t2;
            return true;
        }
        return true;
    }
    return ReportIllegal(action->type, field);
}

static bool
HandleDeviceBtn(struct xkb_keymap *keymap, struct xkb_any_action *action,
                unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    ExprResult rtrn;
    struct xkb_device_button_action *act;

    act = (struct xkb_device_button_action *) action;
    if (field == F_Button)
    {
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveInteger(value, &rtrn))
            return ReportMismatch(action->type, field,
                                  "integer (range 1..255)");
        if ((rtrn.ival < 0) || (rtrn.ival > 255))
        {
            ERROR("Button must specify default or be in the range 1..255\n");
            ACTION("Illegal button value %d ignored\n", rtrn.ival);
            return false;
        }
        act->button = rtrn.ival;
        return true;
    }
    else if ((action->type == XkbSA_LockDeviceBtn) && (field == F_Affect))
    {
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveEnum(value, &rtrn, lockWhich))
            return ReportMismatch(action->type, field, "lock or unlock");
        act->flags &= ~(XkbSA_LockNoLock | XkbSA_LockNoUnlock);
        act->flags |= rtrn.ival;
        return true;
    }
    else if (field == F_Count)
    {
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveButton(value, &rtrn))
            return ReportMismatch(action->type, field, "integer");
        if ((rtrn.ival < 0) || (rtrn.ival > 255))
        {
            ERROR("The count field must have a value in the range 0..255\n");
            ACTION("Illegal count %d ignored\n", rtrn.ival);
            return false;
        }
        act->count = rtrn.ival;
        return true;
    }
    else if (field == F_Device)
    {
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveInteger(value, &rtrn))
            return ReportMismatch(action->type, field,
                                  "integer (range 1..255)");
        if ((rtrn.ival < 0) || (rtrn.ival > 255))
        {
            ERROR("Device must specify default or be in the range 1..255\n");
            ACTION("Illegal device value %d ignored\n", rtrn.ival);
            return false;
        }
        act->device = rtrn.ival;
        return true;
    }
    return ReportIllegal(action->type, field);
}

static bool
HandleDeviceValuator(struct xkb_keymap *keymap, struct xkb_any_action *action,
                     unsigned field, ExprDef *array_ndx, ExprDef *value)
{
#if 0
    ExprResult rtrn;
    struct xkb_device_valuator_action *act;

    act = (struct xkb_device_valuator_action *) action;
    /*  XXX - Not yet implemented */
#endif
    return false;
}

static bool
HandlePrivate(struct xkb_keymap *keymap, struct xkb_any_action *action,
              unsigned field, ExprDef *array_ndx, ExprDef *value)
{
    ExprResult rtrn;

    switch (field)
    {
    case F_Type:
        if (!ExprResolveInteger(value, &rtrn))
            return ReportMismatch(PrivateAction, field, "integer");
        if ((rtrn.ival < 0) || (rtrn.ival > 255))
        {
            ERROR("Private action type must be in the range 0..255\n");
            ACTION("Illegal type %d ignored\n", rtrn.ival);
            return false;
        }
        action->type = rtrn.uval;
        return true;
    case F_Data:
        if (array_ndx == NULL)
        {
            if (!ExprResolveString(keymap, value, &rtrn))
                return ReportMismatch(action->type, field, "string");
            else
            {
                int len = strlen(rtrn.str);
                if ((len < 1) || (len > 7))
                {
                    WARN("A private action has 7 data bytes\n");
                    ACTION("Extra %d bytes ignored\n", len - 6);
                    return false;
                }
                strncpy((char *) action->data, rtrn.str, sizeof action->data);
            }
            free(rtrn.str);
            return true;
        }
        else
        {
            unsigned ndx;
            if (!ExprResolveInteger(array_ndx, &rtrn))
            {
                ERROR("Array subscript must be integer\n");
                ACTION("Illegal subscript ignored\n");
                return false;
            }
            ndx = rtrn.uval;
            if (ndx >= sizeof action->data)
            {
                ERROR("The data for a private action is 18 bytes long\n");
                ACTION("Attempt to use data[%d] ignored\n", ndx);
                return false;
            }
            if (!ExprResolveInteger(value, &rtrn))
                return ReportMismatch(action->type, field, "integer");
            if ((rtrn.ival < 0) || (rtrn.ival > 255))
            {
                ERROR("All data for a private action must be 0..255\n");
                ACTION("Illegal datum %d ignored\n", rtrn.ival);
                return false;
            }
            action->data[ndx] = rtrn.uval;
            return true;
        }
    }
    return ReportIllegal(PrivateAction, field);
}

typedef bool (*actionHandler) (struct xkb_keymap *keymap,
                               struct xkb_any_action *action, unsigned field,
                               ExprDef *array_ndx, ExprDef *value);

static const actionHandler handleAction[XkbSA_NumActions + 1] = {
    [XkbSA_NoAction]       = HandleNoAction,
    [XkbSA_SetMods]        = HandleSetLatchMods,
    [XkbSA_LatchMods]      = HandleSetLatchMods,
    [XkbSA_LockMods]       = HandleLockMods,
    [XkbSA_SetGroup]       = HandleSetLatchGroup,
    [XkbSA_LatchGroup]     = HandleSetLatchGroup,
    [XkbSA_LockGroup]      = HandleLockGroup,
    [XkbSA_MovePtr]        = HandleMovePtr,
    [XkbSA_PtrBtn]         = HandlePtrBtn,
    [XkbSA_LockPtrBtn]     = HandlePtrBtn,
    [XkbSA_SetPtrDflt]     = HandleSetPtrDflt,
    [XkbSA_ISOLock]        = HandleISOLock,
    [XkbSA_Terminate]      = HandleNoAction,
    [XkbSA_SwitchScreen]   = HandleSwitchScreen,
    [XkbSA_SetControls]    = HandleSetLockControls,
    [XkbSA_LockControls]   = HandleSetLockControls,
    [XkbSA_ActionMessage]  = HandleActionMessage,
    [XkbSA_RedirectKey]    = HandleRedirectKey,
    [XkbSA_DeviceBtn]      = HandleDeviceBtn,
    [XkbSA_LockDeviceBtn]  = HandleDeviceBtn,
    [XkbSA_DeviceValuator] = HandleDeviceValuator,
    [PrivateAction]        = HandlePrivate,
};

/***====================================================================***/

static void
ApplyActionFactoryDefaults(union xkb_action * action)
{
    if (action->type == XkbSA_SetPtrDflt)
    {                           /* increment default button */
        action->dflt.affect = XkbSA_AffectDfltBtn;
        action->dflt.flags = 0;
        action->dflt.value = 1;
    }
    else if (action->type == XkbSA_ISOLock)
    {
        action->iso.real_mods = LockMask;
    }
}

static void
ActionsInit(struct xkb_context *context);

int
HandleActionDef(ExprDef * def,
                struct xkb_keymap *keymap,
                struct xkb_any_action *action, ActionInfo *info)
{
    ExprDef *arg;
    const char *str;
    unsigned tmp, hndlrType;

    if (!actionsInitialized)
        ActionsInit(keymap->context);

    if (def->op != ExprActionDecl)
    {
        ERROR("Expected an action definition, found %s\n",
               exprOpText(def->op));
        return false;
    }
    str = XkbcAtomText(def->value.action.name);
    if (!str)
    {
        WSGO("Missing name in action definition!!\n");
        return false;
    }
    if (!stringToAction(str, &tmp))
    {
        ERROR("Unknown action %s\n", str);
        return false;
    }
    action->type = hndlrType = tmp;
    if (action->type != XkbSA_NoAction)
    {
        ApplyActionFactoryDefaults((union xkb_action *) action);
        while (info)
        {
            if ((info->action == XkbSA_NoAction)
                || (info->action == hndlrType))
            {
                if (!(*handleAction[hndlrType]) (keymap, action,
                                                 info->field,
                                                 info->array_ndx,
                                                 info->value))
                {
                    return false;
                }
            }
            info = info->next;
        }
    }
    for (arg = def->value.action.args; arg != NULL;
         arg = (ExprDef *) arg->common.next)
    {
        ExprDef *field, *value, *arrayRtrn;
        ExprResult elemRtrn, fieldRtrn;
        unsigned fieldNdx;

        if (arg->op == OpAssign)
        {
            field = arg->value.binary.left;
            value = arg->value.binary.right;
        }
        else
        {
            if ((arg->op == OpNot) || (arg->op == OpInvert))
            {
                field = arg->value.child;
                constFalse.value.str = xkb_atom_intern(keymap->context, "false");
                value = &constFalse;
            }
            else
            {
                field = arg;
                constTrue.value.str = xkb_atom_intern(keymap->context, "true");
                value = &constTrue;
            }
        }
        if (!ExprResolveLhs(keymap, field, &elemRtrn, &fieldRtrn, &arrayRtrn))
            return false;       /* internal error -- already reported */

        if (elemRtrn.str != NULL)
        {
            ERROR("Cannot change defaults in an action definition\n");
            ACTION("Ignoring attempt to change %s.%s\n", elemRtrn.str,
                    fieldRtrn.str);
            free(elemRtrn.str);
            free(fieldRtrn.str);
            return false;
        }
        if (!stringToField(fieldRtrn.str, &fieldNdx))
        {
            ERROR("Unknown field name %s\n", uStringText(fieldRtrn.str));
            free(elemRtrn.str);
            free(fieldRtrn.str);
            return false;
        }
        free(elemRtrn.str);
        free(fieldRtrn.str);
        if (!(*handleAction[hndlrType])(keymap, action, fieldNdx, arrayRtrn,
                                        value))
            return false;
    }
    return true;
}

/***====================================================================***/

int
SetActionField(struct xkb_keymap *keymap,
               char *elem,
               char *field,
               ExprDef * array_ndx, ExprDef * value, ActionInfo ** info_rtrn)
{
    ActionInfo *new, *old;

    if (!actionsInitialized)
        ActionsInit(keymap->context);

    new = uTypedAlloc(ActionInfo);
    if (new == NULL)
    {
        WSGO("Couldn't allocate space for action default\n");
        return false;
    }
    if (strcasecmp(elem, "action") == 0)
        new->action = XkbSA_NoAction;
    else
    {
        if (!stringToAction(elem, &new->action))
        {
            free(new);
            return false;
        }
        if (new->action == XkbSA_NoAction)
        {
            ERROR("\"%s\" is not a valid field in a NoAction action\n",
                   field);
            free(new);
            return false;
        }
    }
    if (!stringToField(field, &new->field))
    {
        ERROR("\"%s\" is not a legal field name\n", field);
        free(new);
        return false;
    }
    new->array_ndx = array_ndx;
    new->value = value;
    new->next = NULL;
    old = *info_rtrn;
    while ((old) && (old->next))
        old = old->next;
    if (old == NULL)
        *info_rtrn = new;
    else
        old->next = new;
    return true;
}

/***====================================================================***/

static void
ActionsInit(struct xkb_context *context)
{
    if (!actionsInitialized)
    {
        memset(&constTrue, 0, sizeof(constTrue));
        memset(&constFalse, 0, sizeof(constFalse));
        constTrue.common.stmtType = StmtExpr;
        constTrue.common.next = NULL;
        constTrue.op = ExprIdent;
        constTrue.type = TypeBoolean;
        constTrue.value.str = xkb_atom_intern(context, "true");
        constFalse.common.stmtType = StmtExpr;
        constFalse.common.next = NULL;
        constFalse.op = ExprIdent;
        constFalse.type = TypeBoolean;
        constFalse.value.str = xkb_atom_intern(context, "false");
        actionsInitialized = 1;
    }
}
