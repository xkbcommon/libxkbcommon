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

#include "action.h"

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

static const char *
valueToString(const LookupEntry tab[], unsigned int value)
{
    const LookupEntry *entry;

    for (entry = tab; entry->name; entry++)
        if (entry->value == value)
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

static inline bool
ReportMismatch(struct xkb_keymap *keymap, unsigned action, unsigned field,
               const char *type)
{
    log_err(keymap->ctx,
            "Value of %s field must be of type %s; "
            "Action %s definition ignored\n",
            fieldText(field), type, ActionTypeText(action));
    return false;
}

static inline bool
ReportIllegal(struct xkb_keymap *keymap, unsigned action, unsigned field)
{
    log_err(keymap->ctx,
            "Field %s is not defined for an action of type %s; "
            "Action definition ignored\n",
            fieldText(field), ActionTypeText(action));
    return false;
}

static inline bool
ReportActionNotArray(struct xkb_keymap *keymap, unsigned action,
                     unsigned field)
{
    log_err(keymap->ctx,
            "The %s field in the %s action is not an array; "
            "Action definition ignored\n",
            fieldText(field), ActionTypeText(action));
    return false;
}

static inline bool
ReportNotFound(struct xkb_keymap *keymap, unsigned action, unsigned field,
               const char *what, const char *bad)
{
    log_err(keymap->ctx,
            "%s named %s not found; "
            "Ignoring the %s field of an %s action\n",
            what, bad, fieldText(field), ActionTypeText(action));
    return false;
}

static bool
HandleNoAction(struct xkb_keymap *keymap, struct xkb_any_action *action,
               unsigned field, const ExprDef *array_ndx, const ExprDef *value)

{
    return ReportIllegal(keymap, action->type, field);
}

static bool
CheckLatchLockFlags(struct xkb_keymap *keymap, unsigned action,
                    unsigned field, const ExprDef * value,
                    unsigned *flags_inout)
{
    unsigned tmp;
    bool result;

    if (field == F_ClearLocks)
        tmp = XkbSA_ClearLocks;
    else if (field == F_LatchToLock)
        tmp = XkbSA_LatchToLock;
    else
        return false;           /* WSGO! */

    if (!ExprResolveBoolean(keymap->ctx, value, &result))
        return ReportMismatch(keymap, action, field, "boolean");

    if (result)
        *flags_inout |= tmp;
    else
        *flags_inout &= ~tmp;

    return true;
}

static bool
CheckModifierField(struct xkb_keymap *keymap, unsigned action,
                   const ExprDef *value, unsigned *flags_inout,
                   xkb_mod_mask_t *mods_rtrn)
{
    if (value->op == EXPR_IDENT) {
        const char *valStr;
        valStr = xkb_atom_text(keymap->ctx, value->value.str);
        if (valStr && (istreq(valStr, "usemodmapmods") ||
                       istreq(valStr, "modmapmods"))) {

            *mods_rtrn = 0;
            *flags_inout |= XkbSA_UseModMapMods;
            return true;
        }
    }

    if (!ExprResolveVModMask(keymap, value, mods_rtrn))
        return ReportMismatch(keymap, action, F_Modifiers, "modifier mask");

    *flags_inout &= ~XkbSA_UseModMapMods;
    return true;
}

static bool
HandleSetLatchMods(struct xkb_keymap *keymap, struct xkb_any_action *action,
                   unsigned field, const ExprDef *array_ndx,
                   const ExprDef *value)
{
    struct xkb_mod_action *act;
    unsigned rtrn;
    unsigned t1;
    xkb_mod_mask_t t2;

    act = (struct xkb_mod_action *) action;
    if (array_ndx != NULL) {
        switch (field) {
        case F_ClearLocks:
        case F_LatchToLock:
        case F_Modifiers:
            return ReportActionNotArray(keymap, action->type, field);
        }
    }
    switch (field) {
    case F_ClearLocks:
    case F_LatchToLock:
        rtrn = act->flags;
        if (CheckLatchLockFlags(keymap, action->type, field, value, &rtrn)) {
            act->flags = rtrn;
            return true;
        }
        return false;

    case F_Modifiers:
        t1 = act->flags;
        if (CheckModifierField(keymap, action->type, value, &t1, &t2)) {
            act->flags = t1;
            act->real_mods = act->mask = (t2 & 0xff);
            act->vmods = (t2 >> 8) & 0xffff;
            return true;
        }
        return false;
    }
    return ReportIllegal(keymap, action->type, field);
}

static bool
HandleLockMods(struct xkb_keymap *keymap, struct xkb_any_action *action,
               unsigned field, const ExprDef *array_ndx, const ExprDef *value)
{
    struct xkb_mod_action *act;
    unsigned t1;
    xkb_mod_mask_t t2;

    act = (struct xkb_mod_action *) action;
    if ((array_ndx != NULL) && (field == F_Modifiers))
        return ReportActionNotArray(keymap, action->type, field);
    switch (field) {
    case F_Modifiers:
        t1 = act->flags;
        if (CheckModifierField(keymap, action->type, value, &t1, &t2)) {
            act->flags = t1;
            act->real_mods = act->mask = (t2 & 0xff);
            act->vmods = (t2 >> 8) & 0xffff;
            return true;
        }
        return false;
    }
    return ReportIllegal(keymap, action->type, field);
}

static bool
CheckGroupField(struct xkb_keymap *keymap, unsigned action,
                const ExprDef *value, unsigned *flags_inout,
                xkb_group_index_t *grp_rtrn)
{
    const ExprDef *spec;

    if (value->op == EXPR_NEGATE || value->op == EXPR_UNARY_PLUS) {
        *flags_inout &= ~XkbSA_GroupAbsolute;
        spec = value->value.child;
    }
    else {
        *flags_inout |= XkbSA_GroupAbsolute;
        spec = value;
    }

    if (!ExprResolveGroup(keymap->ctx, spec, grp_rtrn))
        return ReportMismatch(keymap, action, F_Group,
                              "integer (range 1..8)");

    if (value->op == EXPR_NEGATE)
        *grp_rtrn = -*grp_rtrn;
    else if (value->op != EXPR_UNARY_PLUS)
        (*grp_rtrn)--;

    return true;
}

static bool
HandleSetLatchGroup(struct xkb_keymap *keymap, struct xkb_any_action *action,
                    unsigned field, const ExprDef *array_ndx,
                    const ExprDef *value)
{
    struct xkb_group_action *act;
    unsigned rtrn;
    unsigned t1;
    xkb_group_index_t t2;

    act = (struct xkb_group_action *) action;
    if (array_ndx != NULL) {
        switch (field) {
        case F_ClearLocks:
        case F_LatchToLock:
        case F_Group:
            return ReportActionNotArray(keymap, action->type, field);
        }
    }
    switch (field) {
    case F_ClearLocks:
    case F_LatchToLock:
        rtrn = act->flags;
        if (CheckLatchLockFlags(keymap, action->type, field, value, &rtrn)) {
            act->flags = rtrn;
            return true;
        }
        return false;

    case F_Group:
        t1 = act->flags;
        if (CheckGroupField(keymap, action->type, value, &t1, &t2)) {
            act->flags = t1;
            act->group = t2;
            return true;
        }
        return false;
    }
    return ReportIllegal(keymap, action->type, field);
}

static bool
HandleLockGroup(struct xkb_keymap *keymap, struct xkb_any_action *action,
                unsigned field, const ExprDef *array_ndx,
                const ExprDef *value)
{
    struct xkb_group_action *act;
    unsigned t1;
    xkb_group_index_t t2;

    act = (struct xkb_group_action *) action;
    if ((array_ndx != NULL) && (field == F_Group))
        return ReportActionNotArray(keymap, action->type, field);
    if (field == F_Group) {
        t1 = act->flags;
        if (CheckGroupField(keymap, action->type, value, &t1, &t2)) {
            act->flags = t1;
            act->group = t2;
            return true;
        }
        return false;
    }
    return ReportIllegal(keymap, action->type, field);
}

static bool
HandleMovePtr(struct xkb_keymap *keymap, struct xkb_any_action *action,
              unsigned field, const ExprDef *array_ndx, const ExprDef *value)
{
    struct xkb_pointer_action *act;
    bool absolute;

    act = (struct xkb_pointer_action *) action;
    if ((array_ndx != NULL) && ((field == F_X) || (field == F_Y)))
        return ReportActionNotArray(keymap, action->type, field);

    if (field == F_X || field == F_Y) {
        int val;

        if (value->op == EXPR_NEGATE || value->op == EXPR_UNARY_PLUS)
            absolute = false;
        else
            absolute = true;

        if (!ExprResolveInteger(keymap->ctx, value, &val))
            return ReportMismatch(keymap, action->type, field, "integer");

        if (field == F_X) {
            if (absolute)
                act->flags |= XkbSA_MoveAbsoluteX;
            act->x = val;
        }
        else {
            if (absolute)
                act->flags |= XkbSA_MoveAbsoluteY;
            act->y = val;
        }

        return true;
    }
    else if (field == F_Accel) {
        bool set;

        if (!ExprResolveBoolean(keymap->ctx, value, &set))
            return ReportMismatch(keymap, action->type, field, "boolean");

        if (set)
            act->flags &= ~XkbSA_NoAcceleration;
        else
            act->flags |= XkbSA_NoAcceleration;
    }

    return ReportIllegal(keymap, action->type, field);
}

static const LookupEntry lockWhich[] = {
    { "both", 0 },
    { "lock", XkbSA_LockNoUnlock },
    { "neither", (XkbSA_LockNoLock | XkbSA_LockNoUnlock) },
    { "unlock", XkbSA_LockNoLock },
    { NULL, 0 }
};

static bool
HandlePtrBtn(struct xkb_keymap *keymap, struct xkb_any_action *action,
             unsigned field, const ExprDef *array_ndx, const ExprDef *value)
{
    struct xkb_pointer_button_action *act;

    act = (struct xkb_pointer_button_action *) action;
    if (field == F_Button) {
        int btn;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (!ExprResolveButton(keymap->ctx, value, &btn))
            return ReportMismatch(keymap, action->type, field,
                                  "integer (range 1..5)");

        if (btn < 0 || btn > 5) {
            log_err(keymap->ctx,
                    "Button must specify default or be in the range 1..5; "
                    "Illegal button value %d ignored\n", btn);
            return false;
        }

        act->button = btn;
        return true;
    }
    else if ((action->type == XkbSA_LockPtrBtn) && (field == F_Affect)) {
        unsigned int val;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (!ExprResolveEnum(keymap->ctx, value, &val, lockWhich))
            return ReportMismatch(keymap, action->type, field,
                                  "lock or unlock");

        act->flags &= ~(XkbSA_LockNoLock | XkbSA_LockNoUnlock);
        act->flags |= val;
        return true;
    }
    else if (field == F_Count) {
        int btn;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        /* XXX: Should this actually be ResolveButton? */
        if (!ExprResolveButton(keymap->ctx, value, &btn))
            return ReportMismatch(keymap, action->type, field, "integer");

        if (btn < 0 || btn > 255) {
            log_err(keymap->ctx,
                    "The count field must have a value in the range 0..255; "
                    "Illegal count %d ignored\n", btn);
            return false;
        }

        act->count = btn;
        return true;
    }
    return ReportIllegal(keymap, action->type, field);
}

static const LookupEntry ptrDflts[] = {
    { "dfltbtn", XkbSA_AffectDfltBtn },
    { "defaultbutton", XkbSA_AffectDfltBtn },
    { "button", XkbSA_AffectDfltBtn },
    { NULL, 0 }
};

static bool
HandleSetPtrDflt(struct xkb_keymap *keymap, struct xkb_any_action *action,
                 unsigned field, const ExprDef *array_ndx,
                 const ExprDef *value)
{
    struct xkb_pointer_default_action *act;

    act = (struct xkb_pointer_default_action *) action;
    if (field == F_Affect) {
        unsigned int val;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (!ExprResolveEnum(keymap->ctx, value, &val, ptrDflts))
            return ReportMismatch(keymap, action->type, field,
                                  "pointer component");
        act->affect = val;
        return true;
    }
    else if ((field == F_Button) || (field == F_Value)) {
        const ExprDef *button;
        int btn;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (value->op == EXPR_NEGATE || value->op == EXPR_UNARY_PLUS) {
            act->flags &= ~XkbSA_DfltBtnAbsolute;
            button = value->value.child;
        }
        else {
            act->flags |= XkbSA_DfltBtnAbsolute;
            button = value;
        }

        if (!ExprResolveButton(keymap->ctx, button, &btn))
            return ReportMismatch(keymap, action->type, field,
                                  "integer (range 1..5)");

        if (btn < 0 || btn > 5) {
            log_err(keymap->ctx,
                    "New default button value must be in the range 1..5; "
                    "Illegal default button value %d ignored\n", btn);
            return false;
        }
        if (btn == 0) {
            log_err(keymap->ctx,
                    "Cannot set default pointer button to \"default\"; "
                    "Illegal default button setting ignored\n");
            return false;
        }

        act->value = (value->op == EXPR_NEGATE ? -btn: btn);
        return true;
    }

    return ReportIllegal(keymap, action->type, field);
}

static const LookupEntry isoNames[] = {
    { "mods", XkbSA_ISONoAffectMods },
    { "modifiers", XkbSA_ISONoAffectMods },
    { "group", XkbSA_ISONoAffectGroup },
    { "groups", XkbSA_ISONoAffectGroup },
    { "ptr", XkbSA_ISONoAffectPtr },
    { "pointer", XkbSA_ISONoAffectPtr },
    { "ctrls", XkbSA_ISONoAffectCtrls },
    { "controls", XkbSA_ISONoAffectCtrls },
    { "all", ~((unsigned) 0) },
    { "none", 0 },
    { NULL, 0 },
};

static bool
HandleISOLock(struct xkb_keymap *keymap, struct xkb_any_action *action,
              unsigned field, const ExprDef *array_ndx, const ExprDef *value)
{
    struct xkb_iso_action *act;

    act = (struct xkb_iso_action *) action;
    if (field == F_Modifiers) {
        unsigned flags;
        xkb_mod_mask_t mods;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        flags = act->flags;
        if (!CheckModifierField(keymap, action->type, value, &flags, &mods))
            return false;

        act->flags = flags & (~XkbSA_ISODfltIsGroup);
        act->real_mods = mods & 0xff;
        act->vmods = (mods >> 8) & 0xff;
        return true;
    }
    else if (field == F_Group) {
        xkb_group_index_t group;
        unsigned flags;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        flags = act->flags;
        if (!CheckGroupField(keymap, action->type, value, &flags, &group))
            return false;

        act->flags = flags | XkbSA_ISODfltIsGroup;
        act->group = group;
        return true;
    } else if (F_Affect) {
        xkb_mod_mask_t mask;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (!ExprResolveMask(keymap->ctx, value, &mask, isoNames))
            return ReportMismatch(keymap, action->type, field,
                                  "keyboard component");

        act->affect = (~mask) & XkbSA_ISOAffectMask;
        return true;
    }

    return ReportIllegal(keymap, action->type, field);
}

static bool
HandleSwitchScreen(struct xkb_keymap *keymap, struct xkb_any_action *action,
                   unsigned field, const ExprDef *array_ndx,
                   const ExprDef *value)
{
    struct xkb_switch_screen_action *act;

    act = (struct xkb_switch_screen_action *) action;
    if (field == F_Screen) {
        const ExprDef *scrn;
        int val;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (value->op == EXPR_NEGATE || value->op == EXPR_UNARY_PLUS) {
            act->flags &= ~XkbSA_SwitchAbsolute;
            scrn = value->value.child;
        }
        else {
            act->flags |= XkbSA_SwitchAbsolute;
            scrn = value;
        }

        if (!ExprResolveInteger(keymap->ctx, scrn, &val))
            return ReportMismatch(keymap, action->type, field,
                                  "integer (0..255)");

        if (val < 0 || val > 255) {
            log_err(keymap->ctx,
                    "Screen index must be in the range 1..255; "
                    "Illegal screen value %d ignored\n", val);
            return false;
        }

        act->screen = (value->op == EXPR_NEGATE ? -val : val);
        return true;
    }
    else if (field == F_Same) {
        bool set;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (!ExprResolveBoolean(keymap->ctx, value, &set))
            return ReportMismatch(keymap, action->type, field, "boolean");

        if (set)
            act->flags &= ~XkbSA_SwitchApplication;
        else
            act->flags |= XkbSA_SwitchApplication;

        return true;
    }

    return ReportIllegal(keymap, action->type, field);
}

const LookupEntry ctrlNames[] = {
    { "repeatkeys", XkbRepeatKeysMask },
    { "repeat", XkbRepeatKeysMask },
    { "autorepeat", XkbRepeatKeysMask },
    { "slowkeys", XkbSlowKeysMask },
    { "bouncekeys", XkbBounceKeysMask },
    { "stickykeys", XkbStickyKeysMask },
    { "mousekeys", XkbMouseKeysMask },
    { "mousekeysaccel", XkbMouseKeysAccelMask },
    { "accessxkeys", XkbAccessXKeysMask },
    { "accessxtimeout", XkbAccessXTimeoutMask },
    { "accessxfeedback", XkbAccessXFeedbackMask },
    { "audiblebell", XkbAudibleBellMask },
    { "ignoregrouplock", XkbIgnoreGroupLockMask },
    { "all", XkbAllBooleanCtrlsMask },
    { "overlay1", 0 },
    { "overlay2", 0 },
    { "none", 0 },
    { NULL, 0 }
};

static bool
HandleSetLockControls(struct xkb_keymap *keymap,
                      struct xkb_any_action *action,
                      unsigned field, const ExprDef *array_ndx,
                      const ExprDef *value)
{
    struct xkb_controls_action *act;

    act = (struct xkb_controls_action *) action;
    if (field == F_Controls) {
        unsigned int mask;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (!ExprResolveMask(keymap->ctx, value, &mask, ctrlNames))
            return ReportMismatch(keymap, action->type, field,
                                  "controls mask");

        act->ctrls = mask;
        return true;
    }

    return ReportIllegal(keymap, action->type, field);
}

static const LookupEntry evNames[] = {
    { "press", XkbSA_MessageOnPress },
    { "keypress", XkbSA_MessageOnPress },
    { "release", XkbSA_MessageOnRelease },
    { "keyrelease", XkbSA_MessageOnRelease },
    { "all", XkbSA_MessageOnPress | XkbSA_MessageOnRelease },
    { "none", 0 },
    { NULL, 0 }
};

static bool
HandleActionMessage(struct xkb_keymap *keymap, struct xkb_any_action *action,
                    unsigned field, const ExprDef *array_ndx,
                    const ExprDef *value)
{
    struct xkb_message_action *act;

    act = (struct xkb_message_action *) action;
    if (field == F_Report) {
        unsigned int mask;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (!ExprResolveMask(keymap->ctx, value, &mask, evNames))
            return ReportMismatch(keymap, action->type, field,
                                  "key event mask");

        /* FIXME: Something seems wrong here... */
        act->flags &= ~(XkbSA_MessageOnPress | XkbSA_MessageOnRelease);
        act->flags = mask & (XkbSA_MessageOnPress | XkbSA_MessageOnRelease);
        return true;
    }
    else if (field == F_GenKeyEvent) {
        bool set;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (!ExprResolveBoolean(keymap->ctx, value, &set))
            return ReportMismatch(keymap, action->type, field, "boolean");

        if (set)
            act->flags |= XkbSA_MessageGenKeyEvent;
        else
            act->flags &= ~XkbSA_MessageGenKeyEvent;

        return true;
    }
    else if (field == F_Data && !array_ndx) {
        const char *str;
        int len;

        if (!ExprResolveString(keymap->ctx, value, &str))
            return ReportMismatch(keymap, action->type, field, "string");

        len = strlen(str);
        if (len < 1 || len > 6) {
            log_warn(keymap->ctx,
                     "An action message can hold only 6 bytes; "
                     "Extra %d bytes ignored\n", len - 6);
        }

        strncpy((char *) act->message, str, 6);
        return true;
    }
    else if (field == F_Data && array_ndx) {
        int ndx, datum;

        if (!ExprResolveInteger(keymap->ctx, array_ndx, &ndx)) {
            log_err(keymap->ctx,
                    "Array subscript must be integer; "
                    "Illegal subscript ignored\n");
            return false;
        }

        if (ndx < 0 || ndx > 5) {
            log_err(keymap->ctx,
                    "An action message is at most 6 bytes long; "
                    "Attempt to use data[%d] ignored\n", ndx);
            return false;
        }

        if (!ExprResolveInteger(keymap->ctx, value, &datum))
            return ReportMismatch(keymap, action->type, field, "integer");

        if (datum < 0 || datum > 255) {
            log_err(keymap->ctx,
                    "Message data must be in the range 0..255; "
                    "Illegal datum %d ignored\n", datum);
            return false;
        }

        act->message[ndx] = (uint8_t) datum;
        return true;
    }

    return ReportIllegal(keymap, action->type, field);
}

static bool
HandleRedirectKey(struct xkb_keymap *keymap, struct xkb_any_action *action,
                  unsigned field, const ExprDef *array_ndx,
                  const ExprDef *value)
{
    struct xkb_key *key;
    struct xkb_redirect_key_action *act;
    unsigned t1;
    xkb_mod_mask_t t2;
    unsigned long tmp;
    char key_name[XkbKeyNameLength];

    if (array_ndx != NULL)
        return ReportActionNotArray(keymap, action->type, field);

    act = (struct xkb_redirect_key_action *) action;
    switch (field) {
    case F_Keycode:
        if (!ExprResolveKeyName(keymap->ctx, value, key_name))
            return ReportMismatch(keymap, action->type, field, "key name");

        tmp = KeyNameToLong(key_name);
        key = FindNamedKey(keymap, tmp, true, CreateKeyNames(keymap), 0);
        if (!key)
            return ReportNotFound(keymap, action->type, field, "Key",
                                  KeyNameText(key_name));
        act->new_kc = XkbKeyGetKeycode(keymap, key);
        return true;

    case F_ModsToClear:
    case F_Modifiers:
        t1 = 0;
        if (CheckModifierField(keymap, action->type, value, &t1, &t2)) {
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
    return ReportIllegal(keymap, action->type, field);
}

static bool
HandleDeviceBtn(struct xkb_keymap *keymap, struct xkb_any_action *action,
                unsigned field, const ExprDef *array_ndx,
                const ExprDef *value)
{
    struct xkb_device_button_action *act;

    act = (struct xkb_device_button_action *) action;
    if (field == F_Button) {
        int val;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (!ExprResolveInteger(keymap->ctx, value, &val))
            return ReportMismatch(keymap, action->type, field,
                                  "integer (range 1..255)");

        if (val < 0 || val > 255) {
            log_err(keymap->ctx,
                    "Button must specify default or be in the range 1..255; "
                    "Illegal button value %d ignored\n", val);
            return false;
        }

        act->button = val;
        return true;
    }
    else if (action->type == XkbSA_LockDeviceBtn && field == F_Affect) {
        unsigned int val;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (!ExprResolveEnum(keymap->ctx, value, &val, lockWhich))
            return ReportMismatch(keymap, action->type, field,
                                  "lock or unlock");

        act->flags &= ~(XkbSA_LockNoLock | XkbSA_LockNoUnlock);
        act->flags |= val;
        return true;
    }
    else if (field == F_Count) {
        int btn;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        /* XXX: Should this actually be ResolveButton? */
        if (!ExprResolveButton(keymap->ctx, value, &btn))
            return ReportMismatch(keymap, action->type, field, "integer");

        if (btn < 0 || btn > 255) {
            log_err(keymap->ctx,
                    "The count field must have a value in the range 0..255; "
                    "Illegal count %d ignored\n", btn);
            return false;
        }

        act->count = btn;
        return true;
    }
    else if (field == F_Device) {
        int val;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (!ExprResolveInteger(keymap->ctx, value, &val))
            return ReportMismatch(keymap, action->type, field,
                                  "integer (range 1..255)");

        if (val < 0 || val > 255) {
            log_err(keymap->ctx,
                    "Device must specify default or be in the range 1..255; "
                    "Illegal device value %d ignored\n", val);
            return false;
        }

        act->device = val;
        return true;
    }

    return ReportIllegal(keymap, action->type, field);
}

static bool
HandleDeviceValuator(struct xkb_keymap *keymap, struct xkb_any_action *action,
                     unsigned field, const ExprDef *array_ndx,
                     const ExprDef *value)
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
              unsigned field, const ExprDef *array_ndx, const ExprDef *value)
{
    if (field == F_Type) {
        int type;

        if (!ExprResolveInteger(keymap->ctx, value, &type))
            return ReportMismatch(keymap, PrivateAction, field, "integer");

        if (type < 0 || type > 255) {
            log_err(keymap->ctx,
                    "Private action type must be in the range 0..255; "
                    "Illegal type %d ignored\n", type);
            return false;
        }

        action->type = (uint8_t) type;
        return true;
    }
    else if (field == F_Data) {
        if (array_ndx == NULL) {
            const char *str;
            int len;

            if (!ExprResolveString(keymap->ctx, value, &str))
                return ReportMismatch(keymap, action->type, field, "string");

            len = strlen(str);
            if (len < 1 || len > 7) {
                log_warn(keymap->ctx,
                         "A private action has 7 data bytes; "
                         "Extra %d bytes ignored\n", len - 6);
                return false;
            }

            strncpy((char *) action->data, str, sizeof(action->data));
            return true;
        }
        else {
            int ndx, datum;

            if (!ExprResolveInteger(keymap->ctx, array_ndx, &ndx)) {
                log_err(keymap->ctx,
                        "Array subscript must be integer; "
                        "Illegal subscript ignored\n");
                return false;
            }

            if (ndx < 0 || ndx >= sizeof(action->data)) {
                log_err(keymap->ctx,
                        "The data for a private action is %zu bytes long; "
                        "Attempt to use data[%d] ignored\n",
                        sizeof(action->data), ndx);
                return false;
            }

            if (!ExprResolveInteger(keymap->ctx, value, &datum))
                return ReportMismatch(keymap, action->type, field, "integer");

            if (datum < 0 || datum > 255) {
                log_err(keymap->ctx,
                        "All data for a private action must be 0..255; "
                        "Illegal datum %d ignored\n", datum);
                return false;
            }

            action->data[ndx] = (uint8_t) datum;
            return true;
        }
    }

    return ReportIllegal(keymap, PrivateAction, field);
}

typedef bool (*actionHandler)(struct xkb_keymap *keymap,
                              struct xkb_any_action *action, unsigned field,
                              const ExprDef *array_ndx, const ExprDef *value);

static const actionHandler handleAction[XkbSA_NumActions + 1] = {
    [XkbSA_NoAction] = HandleNoAction,
    [XkbSA_SetMods] = HandleSetLatchMods,
    [XkbSA_LatchMods] = HandleSetLatchMods,
    [XkbSA_LockMods] = HandleLockMods,
    [XkbSA_SetGroup] = HandleSetLatchGroup,
    [XkbSA_LatchGroup] = HandleSetLatchGroup,
    [XkbSA_LockGroup] = HandleLockGroup,
    [XkbSA_MovePtr] = HandleMovePtr,
    [XkbSA_PtrBtn] = HandlePtrBtn,
    [XkbSA_LockPtrBtn] = HandlePtrBtn,
    [XkbSA_SetPtrDflt] = HandleSetPtrDflt,
    [XkbSA_ISOLock] = HandleISOLock,
    [XkbSA_Terminate] = HandleNoAction,
    [XkbSA_SwitchScreen] = HandleSwitchScreen,
    [XkbSA_SetControls] = HandleSetLockControls,
    [XkbSA_LockControls] = HandleSetLockControls,
    [XkbSA_ActionMessage] = HandleActionMessage,
    [XkbSA_RedirectKey] = HandleRedirectKey,
    [XkbSA_DeviceBtn] = HandleDeviceBtn,
    [XkbSA_LockDeviceBtn] = HandleDeviceBtn,
    [XkbSA_DeviceValuator] = HandleDeviceValuator,
    [PrivateAction] = HandlePrivate,
};

/***====================================================================***/

static void
ApplyActionFactoryDefaults(union xkb_action * action)
{
    if (action->type == XkbSA_SetPtrDflt) { /* increment default button */
        action->dflt.affect = XkbSA_AffectDfltBtn;
        action->dflt.flags = 0;
        action->dflt.value = 1;
    }
    else if (action->type == XkbSA_ISOLock) {
        action->iso.real_mods = LockMask;
    }
}

static void
ActionsInit(struct xkb_context *ctx);

int
HandleActionDef(ExprDef * def,
                struct xkb_keymap *keymap,
                struct xkb_any_action *action, ActionInfo *info)
{
    ExprDef *arg;
    const char *str;
    unsigned tmp, hndlrType;

    if (!actionsInitialized)
        ActionsInit(keymap->ctx);

    if (def->op != EXPR_ACTION_DECL) {
        log_err(keymap->ctx, "Expected an action definition, found %s\n",
                exprOpText(def->op));
        return false;
    }
    str = xkb_atom_text(keymap->ctx, def->value.action.name);
    if (!str) {
        log_wsgo(keymap->ctx, "Missing name in action definition!!\n");
        return false;
    }
    if (!stringToAction(str, &tmp)) {
        log_err(keymap->ctx, "Unknown action %s\n", str);
        return false;
    }
    action->type = hndlrType = tmp;
    if (action->type != XkbSA_NoAction) {
        ApplyActionFactoryDefaults((union xkb_action *) action);
        while (info)
        {
            if ((info->action == XkbSA_NoAction)
                || (info->action == hndlrType)) {
                if (!(*handleAction[hndlrType])(keymap, action,
                                                info->field,
                                                info->array_ndx,
                                                info->value)) {
                    return false;
                }
            }
            info = info->next;
        }
    }
    for (arg = def->value.action.args; arg != NULL;
         arg = (ExprDef *) arg->common.next) {
        const ExprDef *value;
        ExprDef *field, *arrayRtrn;
        const char *elemRtrn, *fieldRtrn;
        unsigned fieldNdx;

        if (arg->op == EXPR_ASSIGN) {
            field = arg->value.binary.left;
            value = arg->value.binary.right;
        }
        else {
            if (arg->op == EXPR_NOT || arg->op == EXPR_INVERT) {
                field = arg->value.child;
                constFalse.value.str = xkb_atom_intern(keymap->ctx, "false");
                value = &constFalse;
            }
            else {
                field = arg;
                constTrue.value.str = xkb_atom_intern(keymap->ctx, "true");
                value = &constTrue;
            }
        }
        if (!ExprResolveLhs(keymap->ctx, field, &elemRtrn, &fieldRtrn,
                            &arrayRtrn))
            return false;       /* internal error -- already reported */

        if (elemRtrn != NULL) {
            log_err(keymap->ctx,
                    "Cannot change defaults in an action definition; "
                    "Ignoring attempt to change %s.%s\n",
                    elemRtrn, fieldRtrn);
            return false;
        }
        if (!stringToField(fieldRtrn, &fieldNdx)) {
            log_err(keymap->ctx, "Unknown field name %s\n", fieldRtrn);
            return false;
        }
        if (!handleAction[hndlrType](keymap, action, fieldNdx, arrayRtrn,
                                     value))
            return false;
    }
    return true;
}

/***====================================================================***/

int
SetActionField(struct xkb_keymap *keymap, const char *elem, const char *field,
               ExprDef *array_ndx, ExprDef *value, ActionInfo **info_rtrn)
{
    ActionInfo *new, *old;

    if (!actionsInitialized)
        ActionsInit(keymap->ctx);

    new = malloc(sizeof(*new));
    if (!new) {
        log_wsgo(keymap->ctx, "Couldn't allocate space for action default\n");
        return false;
    }

    if (istreq(elem, "action"))
        new->action = XkbSA_NoAction;
    else {
        if (!stringToAction(elem, &new->action)) {
            free(new);
            return false;
        }
        if (new->action == XkbSA_NoAction) {
            log_err(keymap->ctx,
                    "\"%s\" is not a valid field in a NoAction action\n",
                    field);
            free(new);
            return false;
        }
    }
    if (!stringToField(field, &new->field)) {
        log_err(keymap->ctx, "\"%s\" is not a legal field name\n", field);
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
ActionsInit(struct xkb_context *ctx)
{
    if (!actionsInitialized) {
        memset(&constTrue, 0, sizeof(constTrue));
        memset(&constFalse, 0, sizeof(constFalse));
        constTrue.common.type = STMT_EXPR;
        constTrue.common.next = NULL;
        constTrue.op = EXPR_IDENT;
        constTrue.value_type = EXPR_TYPE_BOOLEAN;
        constTrue.value.str = xkb_atom_intern(ctx, "true");
        constFalse.common.type = STMT_EXPR;
        constFalse.common.next = NULL;
        constFalse.op = EXPR_IDENT;
        constFalse.value_type = EXPR_TYPE_BOOLEAN;
        constFalse.value.str = xkb_atom_intern(ctx, "false");
        actionsInitialized = 1;
    }
}

union xkb_action *
ResizeKeyActions(struct xkb_keymap *keymap, struct xkb_key *key,
                 uint32_t needed)
{
    size_t old_ndx, old_num_acts, new_ndx;

    if (needed == 0) {
        key->acts_index = 0;
        return NULL;
    }

    if (XkbKeyHasActions(key) && key->width >= needed)
        return XkbKeyActionsPtr(keymap, key);

    /*
     * The key may already be in the array, but without enough space.
     * This should not happen often, so in order to avoid moving and
     * copying stuff from acts and key_acts, we just allocate new
     * space for the key at the end, and leave the old space alone.
     */

    old_ndx = key->acts_index;
    old_num_acts = XkbKeyNumActions(key);
    new_ndx = darray_size(keymap->acts);

    darray_resize0(keymap->acts, new_ndx + needed);
    key->acts_index = new_ndx;

    /*
     * The key was already in the array, copy the old actions to the
     * new space.
     */
    if (old_ndx != 0)
        memcpy(darray_mem(keymap->acts, new_ndx),
               darray_mem(keymap->acts, old_ndx),
               old_num_acts * sizeof(union xkb_action));

    return XkbKeyActionsPtr(keymap, key);
}
