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

#include "xkbcomp.h"
#include "xkbmisc.h"
#include "tokens.h"
#include "expr.h"

#include "keycodes.h"
#include "vmod.h"
#include "misc.h"
#include "action.h"
#include "misc.h"

static Bool actionsInitialized;
static ExprDef constTrue;
static ExprDef constFalse;

/***====================================================================***/

static Bool
stringToAction(const char *str, unsigned *type_rtrn)
{
    if (str == NULL)
        return False;

    if (uStrCaseCmp(str, "noaction") == 0)
        *type_rtrn = XkbSA_NoAction;
    else if (uStrCaseCmp(str, "setmods") == 0)
        *type_rtrn = XkbSA_SetMods;
    else if (uStrCaseCmp(str, "latchmods") == 0)
        *type_rtrn = XkbSA_LatchMods;
    else if (uStrCaseCmp(str, "lockmods") == 0)
        *type_rtrn = XkbSA_LockMods;
    else if (uStrCaseCmp(str, "setgroup") == 0)
        *type_rtrn = XkbSA_SetGroup;
    else if (uStrCaseCmp(str, "latchgroup") == 0)
        *type_rtrn = XkbSA_LatchGroup;
    else if (uStrCaseCmp(str, "lockgroup") == 0)
        *type_rtrn = XkbSA_LockGroup;
    else if (uStrCaseCmp(str, "moveptr") == 0)
        *type_rtrn = XkbSA_MovePtr;
    else if (uStrCaseCmp(str, "movepointer") == 0)
        *type_rtrn = XkbSA_MovePtr;
    else if (uStrCaseCmp(str, "ptrbtn") == 0)
        *type_rtrn = XkbSA_PtrBtn;
    else if (uStrCaseCmp(str, "pointerbutton") == 0)
        *type_rtrn = XkbSA_PtrBtn;
    else if (uStrCaseCmp(str, "lockptrbtn") == 0)
        *type_rtrn = XkbSA_LockPtrBtn;
    else if (uStrCaseCmp(str, "lockpointerbutton") == 0)
        *type_rtrn = XkbSA_LockPtrBtn;
    else if (uStrCaseCmp(str, "lockptrbutton") == 0)
        *type_rtrn = XkbSA_LockPtrBtn;
    else if (uStrCaseCmp(str, "lockpointerbtn") == 0)
        *type_rtrn = XkbSA_LockPtrBtn;
    else if (uStrCaseCmp(str, "setptrdflt") == 0)
        *type_rtrn = XkbSA_SetPtrDflt;
    else if (uStrCaseCmp(str, "setpointerdefault") == 0)
        *type_rtrn = XkbSA_SetPtrDflt;
    else if (uStrCaseCmp(str, "isolock") == 0)
        *type_rtrn = XkbSA_ISOLock;
    else if (uStrCaseCmp(str, "terminate") == 0)
        *type_rtrn = XkbSA_Terminate;
    else if (uStrCaseCmp(str, "terminateserver") == 0)
        *type_rtrn = XkbSA_Terminate;
    else if (uStrCaseCmp(str, "switchscreen") == 0)
        *type_rtrn = XkbSA_SwitchScreen;
    else if (uStrCaseCmp(str, "setcontrols") == 0)
        *type_rtrn = XkbSA_SetControls;
    else if (uStrCaseCmp(str, "lockcontrols") == 0)
        *type_rtrn = XkbSA_LockControls;
    else if (uStrCaseCmp(str, "actionmessage") == 0)
        *type_rtrn = XkbSA_ActionMessage;
    else if (uStrCaseCmp(str, "messageaction") == 0)
        *type_rtrn = XkbSA_ActionMessage;
    else if (uStrCaseCmp(str, "message") == 0)
        *type_rtrn = XkbSA_ActionMessage;
    else if (uStrCaseCmp(str, "redirect") == 0)
        *type_rtrn = XkbSA_RedirectKey;
    else if (uStrCaseCmp(str, "redirectkey") == 0)
        *type_rtrn = XkbSA_RedirectKey;
    else if (uStrCaseCmp(str, "devbtn") == 0)
        *type_rtrn = XkbSA_DeviceBtn;
    else if (uStrCaseCmp(str, "devicebtn") == 0)
        *type_rtrn = XkbSA_DeviceBtn;
    else if (uStrCaseCmp(str, "devbutton") == 0)
        *type_rtrn = XkbSA_DeviceBtn;
    else if (uStrCaseCmp(str, "devicebutton") == 0)
        *type_rtrn = XkbSA_DeviceBtn;
    else if (uStrCaseCmp(str, "lockdevbtn") == 0)
        *type_rtrn = XkbSA_DeviceBtn;
    else if (uStrCaseCmp(str, "lockdevicebtn") == 0)
        *type_rtrn = XkbSA_LockDeviceBtn;
    else if (uStrCaseCmp(str, "lockdevbutton") == 0)
        *type_rtrn = XkbSA_LockDeviceBtn;
    else if (uStrCaseCmp(str, "lockdevicebutton") == 0)
        *type_rtrn = XkbSA_LockDeviceBtn;
    else if (uStrCaseCmp(str, "devval") == 0)
        *type_rtrn = XkbSA_DeviceValuator;
    else if (uStrCaseCmp(str, "deviceval") == 0)
        *type_rtrn = XkbSA_DeviceValuator;
    else if (uStrCaseCmp(str, "devvaluator") == 0)
        *type_rtrn = XkbSA_DeviceValuator;
    else if (uStrCaseCmp(str, "devicevaluator") == 0)
        *type_rtrn = XkbSA_DeviceValuator;
    else if (uStrCaseCmp(str, "private") == 0)
        *type_rtrn = PrivateAction;
    else
        return False;
    return True;
}

static Bool
stringToField(char *str, unsigned *field_rtrn)
{

    if (str == NULL)
        return False;

    if (uStrCaseCmp(str, "clearlocks") == 0)
        *field_rtrn = F_ClearLocks;
    else if (uStrCaseCmp(str, "latchtolock") == 0)
        *field_rtrn = F_LatchToLock;
    else if (uStrCaseCmp(str, "genkeyevent") == 0)
        *field_rtrn = F_GenKeyEvent;
    else if (uStrCaseCmp(str, "generatekeyevent") == 0)
        *field_rtrn = F_GenKeyEvent;
    else if (uStrCaseCmp(str, "report") == 0)
        *field_rtrn = F_Report;
    else if (uStrCaseCmp(str, "default") == 0)
        *field_rtrn = F_Default;
    else if (uStrCaseCmp(str, "affect") == 0)
        *field_rtrn = F_Affect;
    else if (uStrCaseCmp(str, "increment") == 0)
        *field_rtrn = F_Increment;
    else if (uStrCaseCmp(str, "mods") == 0)
        *field_rtrn = F_Modifiers;
    else if (uStrCaseCmp(str, "modifiers") == 0)
        *field_rtrn = F_Modifiers;
    else if (uStrCaseCmp(str, "group") == 0)
        *field_rtrn = F_Group;
    else if (uStrCaseCmp(str, "x") == 0)
        *field_rtrn = F_X;
    else if (uStrCaseCmp(str, "y") == 0)
        *field_rtrn = F_Y;
    else if (uStrCaseCmp(str, "accel") == 0)
        *field_rtrn = F_Accel;
    else if (uStrCaseCmp(str, "accelerate") == 0)
        *field_rtrn = F_Accel;
    else if (uStrCaseCmp(str, "repeat") == 0)
        *field_rtrn = F_Accel;
    else if (uStrCaseCmp(str, "button") == 0)
        *field_rtrn = F_Button;
    else if (uStrCaseCmp(str, "value") == 0)
        *field_rtrn = F_Value;
    else if (uStrCaseCmp(str, "controls") == 0)
        *field_rtrn = F_Controls;
    else if (uStrCaseCmp(str, "ctrls") == 0)
        *field_rtrn = F_Controls;
    else if (uStrCaseCmp(str, "type") == 0)
        *field_rtrn = F_Type;
    else if (uStrCaseCmp(str, "count") == 0)
        *field_rtrn = F_Count;
    else if (uStrCaseCmp(str, "screen") == 0)
        *field_rtrn = F_Screen;
    else if (uStrCaseCmp(str, "same") == 0)
        *field_rtrn = F_Same;
    else if (uStrCaseCmp(str, "sameserver") == 0)
        *field_rtrn = F_Same;
    else if (uStrCaseCmp(str, "data") == 0)
        *field_rtrn = F_Data;
    else if (uStrCaseCmp(str, "device") == 0)
        *field_rtrn = F_Device;
    else if (uStrCaseCmp(str, "dev") == 0)
        *field_rtrn = F_Device;
    else if (uStrCaseCmp(str, "key") == 0)
        *field_rtrn = F_Keycode;
    else if (uStrCaseCmp(str, "keycode") == 0)
        *field_rtrn = F_Keycode;
    else if (uStrCaseCmp(str, "kc") == 0)
        *field_rtrn = F_Keycode;
    else if (uStrCaseCmp(str, "clearmods") == 0)
        *field_rtrn = F_ModsToClear;
    else if (uStrCaseCmp(str, "clearmodifiers") == 0)
        *field_rtrn = F_ModsToClear;
    else
        return False;
    return True;
}

static char *
fieldText(unsigned field)
{
    static char buf[32];

    switch (field)
    {
    case F_ClearLocks:
        strcpy(buf, "clearLocks");
        break;
    case F_LatchToLock:
        strcpy(buf, "latchToLock");
        break;
    case F_GenKeyEvent:
        strcpy(buf, "genKeyEvent");
        break;
    case F_Report:
        strcpy(buf, "report");
        break;
    case F_Default:
        strcpy(buf, "default");
        break;
    case F_Affect:
        strcpy(buf, "affect");
        break;
    case F_Increment:
        strcpy(buf, "increment");
        break;
    case F_Modifiers:
        strcpy(buf, "modifiers");
        break;
    case F_Group:
        strcpy(buf, "group");
        break;
    case F_X:
        strcpy(buf, "x");
        break;
    case F_Y:
        strcpy(buf, "y");
        break;
    case F_Accel:
        strcpy(buf, "accel");
        break;
    case F_Button:
        strcpy(buf, "button");
        break;
    case F_Value:
        strcpy(buf, "value");
        break;
    case F_Controls:
        strcpy(buf, "controls");
        break;
    case F_Type:
        strcpy(buf, "type");
        break;
    case F_Count:
        strcpy(buf, "count");
        break;
    case F_Screen:
        strcpy(buf, "screen");
        break;
    case F_Same:
        strcpy(buf, "sameServer");
        break;
    case F_Data:
        strcpy(buf, "data");
        break;
    case F_Device:
        strcpy(buf, "device");
        break;
    case F_Keycode:
        strcpy(buf, "keycode");
        break;
    case F_ModsToClear:
        strcpy(buf, "clearmods");
        break;
    default:
        strcpy(buf, "unknown");
        break;
    }
    return buf;
}

/***====================================================================***/

static Bool
ReportMismatch(unsigned action, unsigned field, const char *type)
{
    ERROR("Value of %s field must be of type %s\n", fieldText(field), type);
    ACTION("Action %s definition ignored\n", XkbcActionTypeText(action));
    return False;
}

static Bool
ReportIllegal(unsigned action, unsigned field)
{
    ERROR("Field %s is not defined for an action of type %s\n",
           fieldText(field), XkbcActionTypeText(action));
    ACTION("Action definition ignored\n");
    return False;
}

static Bool
ReportActionNotArray(unsigned action, unsigned field)
{
    ERROR("The %s field in the %s action is not an array\n",
           fieldText(field), XkbcActionTypeText(action));
    ACTION("Action definition ignored\n");
    return False;
}

static Bool
ReportNotFound(unsigned action, unsigned field, const char *what, char *bad)
{
    ERROR("%s named %s not found\n", what, bad);
    ACTION("Ignoring the %s field of an %s action\n", fieldText(field),
            XkbcActionTypeText(action));
    return False;
}

static Bool
HandleNoAction(struct xkb_desc * xkb,
               struct xkb_any_action * action,
               unsigned field, ExprDef * array_ndx, ExprDef * value)
{
    return ReportIllegal(action->type, field);
}

static Bool
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
        return False;           /* WSGO! */
    if (!ExprResolveBoolean(value, &result))
        return ReportMismatch(action, field, "boolean");
    if (result.uval)
        *flags_inout |= tmp;
    else
        *flags_inout &= ~tmp;
    return True;
}

static Bool
CheckModifierField(struct xkb_desc * xkb,
                   unsigned action,
                   ExprDef * value,
                   unsigned *flags_inout, unsigned *mods_rtrn)
{
    ExprResult rtrn;

    if (value->op == ExprIdent)
    {
        const char *valStr;
        valStr = XkbcAtomText(value->value.str);
        if (valStr && ((uStrCaseCmp(valStr, "usemodmapmods") == 0) ||
                       (uStrCaseCmp(valStr, "modmapmods") == 0)))
        {

            *mods_rtrn = 0;
            *flags_inout |= XkbSA_UseModMapMods;
            return True;
        }
    }
    if (!ExprResolveVModMask(value, &rtrn, xkb))
        return ReportMismatch(action, F_Modifiers, "modifier mask");
    *mods_rtrn = rtrn.uval;
    *flags_inout &= ~XkbSA_UseModMapMods;
    return True;
}

static Bool
HandleSetLatchMods(struct xkb_desc * xkb,
                   struct xkb_any_action * action,
                   unsigned field, ExprDef * array_ndx, ExprDef * value)
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
            return True;
        }
        return False;
    case F_Modifiers:
        t1 = act->flags;
        if (CheckModifierField(xkb, action->type, value, &t1, &t2))
        {
            act->flags = t1;
            act->real_mods = act->mask = (t2 & 0xff);
            act->vmods = (t2 >> 8) & 0xffff;
            return True;
        }
        return False;
    }
    return ReportIllegal(action->type, field);
}

static Bool
HandleLockMods(struct xkb_desc * xkb,
               struct xkb_any_action * action,
               unsigned field, ExprDef * array_ndx, ExprDef * value)
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
        if (CheckModifierField(xkb, action->type, value, &t1, &t2))
        {
            act->flags = t1;
            act->real_mods = act->mask = (t2 & 0xff);
            act->vmods = (t2 >> 8) & 0xffff;
            return True;
        }
        return False;
    }
    return ReportIllegal(action->type, field);
}

static Bool
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
    if ((rtrn.ival < 1) || (rtrn.ival > XkbNumKbdGroups))
    {
        ERROR("Illegal group %d (must be in the range 1..%d)\n", rtrn.ival,
               XkbNumKbdGroups);
        ACTION("Action %s definition ignored\n", XkbcActionTypeText(action));
        return False;
    }
    if (value->op == OpNegate)
        *grp_rtrn = -rtrn.ival;
    else if (value->op == OpUnaryPlus)
        *grp_rtrn = rtrn.ival;
    else
        *grp_rtrn = rtrn.ival - 1;
    return True;
}

static Bool
HandleSetLatchGroup(struct xkb_desc * xkb,
                    struct xkb_any_action * action,
                    unsigned field, ExprDef * array_ndx, ExprDef * value)
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
            return True;
        }
        return False;
    case F_Group:
        t1 = act->flags;
        if (CheckGroupField(action->type, value, &t1, &t2))
        {
            act->flags = t1;
	    act->group = t2;
            return True;
        }
        return False;
    }
    return ReportIllegal(action->type, field);
}

static Bool
HandleLockGroup(struct xkb_desc * xkb,
                struct xkb_any_action * action,
                unsigned field, ExprDef * array_ndx, ExprDef * value)
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
            return True;
        }
        return False;
    }
    return ReportIllegal(action->type, field);
}

static Bool
HandleMovePtr(struct xkb_desc * xkb,
              struct xkb_any_action * action,
              unsigned field, ExprDef * array_ndx, ExprDef * value)
{
    ExprResult rtrn;
    struct xkb_pointer_action *act;
    Bool absolute;

    act = (struct xkb_pointer_action *) action;
    if ((array_ndx != NULL) && ((field == F_X) || (field == F_Y)))
        return ReportActionNotArray(action->type, field);

    if ((field == F_X) || (field == F_Y))
    {
        if ((value->op == OpNegate) || (value->op == OpUnaryPlus))
            absolute = False;
        else
            absolute = True;
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
        return True;
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

static LookupEntry lockWhich[] = {
    {"both", 0},
    {"lock", XkbSA_LockNoUnlock},
    {"neither", (XkbSA_LockNoLock | XkbSA_LockNoUnlock)},
    {"unlock", XkbSA_LockNoLock},
    {NULL, 0}
};

static Bool
HandlePtrBtn(struct xkb_desc * xkb,
             struct xkb_any_action * action,
             unsigned field, ExprDef * array_ndx, ExprDef * value)
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
            return False;
        }
        act->button = rtrn.ival;
        return True;
    }
    else if ((action->type == XkbSA_LockPtrBtn) && (field == F_Affect))
    {
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveEnum(value, &rtrn, lockWhich))
            return ReportMismatch(action->type, field, "lock or unlock");
        act->flags &= ~(XkbSA_LockNoLock | XkbSA_LockNoUnlock);
        act->flags |= rtrn.ival;
        return True;
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
            return False;
        }
        act->count = rtrn.ival;
        return True;
    }
    return ReportIllegal(action->type, field);
}

static LookupEntry ptrDflts[] = {
    {"dfltbtn", XkbSA_AffectDfltBtn},
    {"defaultbutton", XkbSA_AffectDfltBtn},
    {"button", XkbSA_AffectDfltBtn},
    {NULL, 0}
};

static Bool
HandleSetPtrDflt(struct xkb_desc * xkb,
                 struct xkb_any_action * action,
                 unsigned field, ExprDef * array_ndx, ExprDef * value)
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
        return True;
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
            return False;
        }
        if (rtrn.ival == 0)
        {
            ERROR("Cannot set default pointer button to \"default\"\n");
            ACTION("Illegal default button setting ignored\n");
            return False;
        }
        if (value->op == OpNegate)
	    act->value = -rtrn.ival;
        else
	    act->value = rtrn.ival;
        return True;
    }
    return ReportIllegal(action->type, field);
}

static LookupEntry isoNames[] = {
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

static Bool
HandleISOLock(struct xkb_desc * xkb,
              struct xkb_any_action * action,
              unsigned field, ExprDef * array_ndx, ExprDef * value)
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
        if (CheckModifierField(xkb, action->type, value, &flags, &mods))
        {
            act->flags = flags & (~XkbSA_ISODfltIsGroup);
            act->real_mods = mods & 0xff;
            act->vmods = (mods >> 8) & 0xff;
            return True;
        }
        return False;
    case F_Group:
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        flags = act->flags;
        if (CheckGroupField(action->type, value, &flags, &group))
        {
            act->flags = flags | XkbSA_ISODfltIsGroup;
            act->group = group;
            return True;
        }
        return False;
    case F_Affect:
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveMask(value, &rtrn, isoNames))
            return ReportMismatch(action->type, field, "keyboard component");
        act->affect = (~rtrn.uval) & XkbSA_ISOAffectMask;
        return True;
    }
    return ReportIllegal(action->type, field);
}

static Bool
HandleSwitchScreen(struct xkb_desc * xkb,
                   struct xkb_any_action * action,
                   unsigned field, ExprDef * array_ndx, ExprDef * value)
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
            return False;
        }
        if (value->op == OpNegate)
	    act->screen = -rtrn.ival;
        else
	    act->screen = rtrn.ival;
        return True;
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
        return True;
    }
    return ReportIllegal(action->type, field);
}

LookupEntry ctrlNames[] = {
    {"repeatkeys", XkbRepeatKeysMask}
    ,
    {"repeat", XkbRepeatKeysMask}
    ,
    {"autorepeat", XkbRepeatKeysMask}
    ,
    {"slowkeys", XkbSlowKeysMask}
    ,
    {"bouncekeys", XkbBounceKeysMask}
    ,
    {"stickykeys", XkbStickyKeysMask}
    ,
    {"mousekeys", XkbMouseKeysMask}
    ,
    {"mousekeysaccel", XkbMouseKeysAccelMask}
    ,
    {"accessxkeys", XkbAccessXKeysMask}
    ,
    {"accessxtimeout", XkbAccessXTimeoutMask}
    ,
    {"accessxfeedback", XkbAccessXFeedbackMask}
    ,
    {"audiblebell", XkbAudibleBellMask}
    ,
    {"overlay1", XkbOverlay1Mask}
    ,
    {"overlay2", XkbOverlay2Mask}
    ,
    {"ignoregrouplock", XkbIgnoreGroupLockMask}
    ,
    {"all", XkbAllBooleanCtrlsMask}
    ,
    {"none", 0}
    ,
    {NULL, 0}
};

static Bool
HandleSetLockControls(struct xkb_desc * xkb,
                      struct xkb_any_action * action,
                      unsigned field, ExprDef * array_ndx, ExprDef * value)
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
        return True;
    }
    return ReportIllegal(action->type, field);
}

static LookupEntry evNames[] = {
    {"press", XkbSA_MessageOnPress},
    {"keypress", XkbSA_MessageOnPress},
    {"release", XkbSA_MessageOnRelease},
    {"keyrelease", XkbSA_MessageOnRelease},
    {"all", XkbSA_MessageOnPress | XkbSA_MessageOnRelease},
    {"none", 0},
    {NULL, 0}
};

static Bool
HandleActionMessage(struct xkb_desc * xkb,
                    struct xkb_any_action * action,
                    unsigned field, ExprDef * array_ndx, ExprDef * value)
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
        return True;
    case F_GenKeyEvent:
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveBoolean(value, &rtrn))
            return ReportMismatch(action->type, field, "boolean");
        if (rtrn.uval)
            act->flags |= XkbSA_MessageGenKeyEvent;
        else
            act->flags &= ~XkbSA_MessageGenKeyEvent;
        return True;
    case F_Data:
        if (array_ndx == NULL)
        {
            if (!ExprResolveString(value, &rtrn))
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
            return True;
        }
        else
        {
            unsigned ndx;
            if (!ExprResolveInteger(array_ndx, &rtrn))
            {
                ERROR("Array subscript must be integer\n");
                ACTION("Illegal subscript ignored\n");
                return False;
            }
            ndx = rtrn.uval;
            if (ndx > 5)
            {
                ERROR("An action message is at most 6 bytes long\n");
                ACTION("Attempt to use data[%d] ignored\n", ndx);
                return False;
            }
            if (!ExprResolveInteger(value, &rtrn))
                return ReportMismatch(action->type, field, "integer");
            if ((rtrn.ival < 0) || (rtrn.ival > 255))
            {
                ERROR("Message data must be in the range 0..255\n");
                ACTION("Illegal datum %d ignored\n", rtrn.ival);
                return False;
            }
            act->message[ndx] = rtrn.uval;
        }
        return True;
    }
    return ReportIllegal(action->type, field);
}

static Bool
HandleRedirectKey(struct xkb_desc * xkb,
                  struct xkb_any_action * action,
                  unsigned field, ExprDef * array_ndx, ExprDef * value)
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
        if (!FindNamedKey(xkb, tmp, &kc, True, CreateKeyNames(xkb), 0))
        {
            return ReportNotFound(action->type, field, "Key",
                                  XkbcKeyNameText(rtrn.keyName.name));
        }
        act->new_key = kc;
        return True;
    case F_ModsToClear:
    case F_Modifiers:
        t1 = 0;
        if (CheckModifierField(xkb, action->type, value, &t1, &t2))
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
            return True;
        }
        return True;
    }
    return ReportIllegal(action->type, field);
}

static Bool
HandleDeviceBtn(struct xkb_desc * xkb,
                struct xkb_any_action * action,
                unsigned field, ExprDef * array_ndx, ExprDef * value)
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
            return False;
        }
        act->button = rtrn.ival;
        return True;
    }
    else if ((action->type == XkbSA_LockDeviceBtn) && (field == F_Affect))
    {
        if (array_ndx != NULL)
            return ReportActionNotArray(action->type, field);
        if (!ExprResolveEnum(value, &rtrn, lockWhich))
            return ReportMismatch(action->type, field, "lock or unlock");
        act->flags &= ~(XkbSA_LockNoLock | XkbSA_LockNoUnlock);
        act->flags |= rtrn.ival;
        return True;
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
            return False;
        }
        act->count = rtrn.ival;
        return True;
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
            return False;
        }
        act->device = rtrn.ival;
        return True;
    }
    return ReportIllegal(action->type, field);
}

static Bool
HandleDeviceValuator(struct xkb_desc * xkb,
                     struct xkb_any_action * action,
                     unsigned field, ExprDef * array_ndx, ExprDef * value)
{
#if 0
    ExprResult rtrn;
    struct xkb_device_valuator_action *act;

    act = (struct xkb_device_valuator_action *) action;
    /*  XXX - Not yet implemented */
#endif
    return False;
}

static Bool
HandlePrivate(struct xkb_desc * xkb,
              struct xkb_any_action * action,
              unsigned field, ExprDef * array_ndx, ExprDef * value)
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
            return False;
        }
        action->type = rtrn.uval;
        return True;
    case F_Data:
        if (array_ndx == NULL)
        {
            if (!ExprResolveString(value, &rtrn))
                return ReportMismatch(action->type, field, "string");
            else
            {
                int len = strlen(rtrn.str);
                if ((len < 1) || (len > 7))
                {
                    WARN("A private action has 7 data bytes\n");
                    ACTION("Extra %d bytes ignored\n", len - 6);
                    return False;
                }
                strncpy((char *) action->data, rtrn.str, sizeof action->data);
            }
            free(rtrn.str);
            return True;
        }
        else
        {
            unsigned ndx;
            if (!ExprResolveInteger(array_ndx, &rtrn))
            {
                ERROR("Array subscript must be integer\n");
                ACTION("Illegal subscript ignored\n");
                return False;
            }
            ndx = rtrn.uval;
            if (ndx >= sizeof action->data)
            {
                ERROR("The data for a private action is 18 bytes long\n");
                ACTION("Attempt to use data[%d] ignored\n", ndx);
                return False;
            }
            if (!ExprResolveInteger(value, &rtrn))
                return ReportMismatch(action->type, field, "integer");
            if ((rtrn.ival < 0) || (rtrn.ival > 255))
            {
                ERROR("All data for a private action must be 0..255\n");
                ACTION("Illegal datum %d ignored\n", rtrn.ival);
                return False;
            }
            action->data[ndx] = rtrn.uval;
            return True;
        }
    }
    return ReportIllegal(PrivateAction, field);
}

typedef Bool(*actionHandler) (struct xkb_desc * /* xkb */ ,
                              struct xkb_any_action * /* action */ ,
                              unsigned /* field */ ,
                              ExprDef * /* array_ndx */ ,
                              ExprDef * /* value */
    );

static actionHandler handleAction[XkbSA_NumActions + 1] = {
    HandleNoAction /* NoAction     */ ,
    HandleSetLatchMods /* SetMods      */ ,
    HandleSetLatchMods /* LatchMods    */ ,
    HandleLockMods /* LockMods     */ ,
    HandleSetLatchGroup /* SetGroup     */ ,
    HandleSetLatchGroup /* LatchGroup   */ ,
    HandleLockGroup /* LockGroup    */ ,
    HandleMovePtr /* MovePtr      */ ,
    HandlePtrBtn /* PtrBtn       */ ,
    HandlePtrBtn /* LockPtrBtn   */ ,
    HandleSetPtrDflt /* SetPtrDflt   */ ,
    HandleISOLock /* ISOLock      */ ,
    HandleNoAction /* Terminate    */ ,
    HandleSwitchScreen /* SwitchScreen */ ,
    HandleSetLockControls /* SetControls  */ ,
    HandleSetLockControls /* LockControls */ ,
    HandleActionMessage /* ActionMessage */ ,
    HandleRedirectKey /* RedirectKey  */ ,
    HandleDeviceBtn /* DeviceBtn    */ ,
    HandleDeviceBtn /* LockDeviceBtn */ ,
    HandleDeviceValuator /* DeviceValuatr */ ,
    HandlePrivate               /* Private      */
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
    return;
}

static void
ActionsInit(void);

int
HandleActionDef(ExprDef * def,
                struct xkb_desc * xkb,
                struct xkb_any_action * action, unsigned mergeMode, ActionInfo * info)
{
    ExprDef *arg;
    const char *str;
    unsigned tmp, hndlrType;

    if (!actionsInitialized)
        ActionsInit();

    if (def->op != ExprActionDecl)
    {
        ERROR("Expected an action definition, found %s\n",
               exprOpText(def->op));
        return False;
    }
    str = XkbcAtomText(def->value.action.name);
    if (!str)
    {
        WSGO("Missing name in action definition!!\n");
        return False;
    }
    if (!stringToAction(str, &tmp))
    {
        ERROR("Unknown action %s\n", str);
        return False;
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
                if (!(*handleAction[hndlrType]) (xkb, action,
                                                 info->field,
                                                 info->array_ndx,
                                                 info->value))
                {
                    return False;
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
                value = &constFalse;
            }
            else
            {
                field = arg;
                value = &constTrue;
            }
        }
        if (!ExprResolveLhs(field, &elemRtrn, &fieldRtrn, &arrayRtrn))
            return False;       /* internal error -- already reported */

        if (elemRtrn.str != NULL)
        {
            ERROR("Cannot change defaults in an action definition\n");
            ACTION("Ignoring attempt to change %s.%s\n", elemRtrn.str,
                    fieldRtrn.str);
            free(elemRtrn.str);
            free(fieldRtrn.str);
            return False;
        }
        if (!stringToField(fieldRtrn.str, &fieldNdx))
        {
            ERROR("Unknown field name %s\n", uStringText(fieldRtrn.str));
            free(elemRtrn.str);
            free(fieldRtrn.str);
            return False;
        }
        free(elemRtrn.str);
        free(fieldRtrn.str);
        if (!(*handleAction[hndlrType])
            (xkb, action, fieldNdx, arrayRtrn, value))
        {
            return False;
        }
    }
    return True;
}

/***====================================================================***/

int
SetActionField(struct xkb_desc * xkb,
               char *elem,
               char *field,
               ExprDef * array_ndx, ExprDef * value, ActionInfo ** info_rtrn)
{
    ActionInfo *new, *old;

    if (!actionsInitialized)
        ActionsInit();

    new = uTypedAlloc(ActionInfo);
    if (new == NULL)
    {
        WSGO("Couldn't allocate space for action default\n");
        return False;
    }
    if (uStrCaseCmp(elem, "action") == 0)
        new->action = XkbSA_NoAction;
    else
    {
        if (!stringToAction(elem, &new->action))
        {
            free(new);
            return False;
        }
        if (new->action == XkbSA_NoAction)
        {
            ERROR("\"%s\" is not a valid field in a NoAction action\n",
                   field);
            free(new);
            return False;
        }
    }
    if (!stringToField(field, &new->field))
    {
        ERROR("\"%s\" is not a legal field name\n", field);
        free(new);
        return False;
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
    return True;
}

/***====================================================================***/

static void
ActionsInit(void)
{
    if (!actionsInitialized)
    {
        bzero((char *) &constTrue, sizeof(constTrue));
        bzero((char *) &constFalse, sizeof(constFalse));
        constTrue.common.stmtType = StmtExpr;
        constTrue.common.next = NULL;
        constTrue.op = ExprIdent;
        constTrue.type = TypeBoolean;
        constTrue.value.str = xkb_intern_atom("true");
        constFalse.common.stmtType = StmtExpr;
        constFalse.common.next = NULL;
        constFalse.op = ExprIdent;
        constFalse.type = TypeBoolean;
        constFalse.value.str = xkb_intern_atom("false");
        actionsInitialized = 1;
    }
    return;
}
