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

#include "xkbcomp-priv.h"
#include "text.h"
#include "expr.h"
#include "action.h"
#include "keycodes.h"

#define PrivateAction (XkbSA_LastAction + 1)

static const ExprDef constTrue = {
    .common = { .type = STMT_EXPR, .next = NULL },
    .op = EXPR_VALUE,
    .value_type = EXPR_TYPE_BOOLEAN,
    .value = { .ival = 1 },
};

static const ExprDef constFalse = {
    .common = { .type = STMT_EXPR, .next = NULL },
    .op = EXPR_VALUE,
    .value_type = EXPR_TYPE_BOOLEAN,
    .value = { .ival = 0 },
};

enum action_field {
    ACTION_FIELD_CLEAR_LOCKS,
    ACTION_FIELD_LATCH_TO_LOCK,
    ACTION_FIELD_GEN_KEY_EVENT,
    ACTION_FIELD_REPORT,
    ACTION_FIELD_DEFAULT,
    ACTION_FIELD_AFFECT,
    ACTION_FIELD_INCREMENT,
    ACTION_FIELD_MODIFIERS,
    ACTION_FIELD_GROUP,
    ACTION_FIELD_X,
    ACTION_FIELD_Y,
    ACTION_FIELD_ACCEL,
    ACTION_FIELD_BUTTON,
    ACTION_FIELD_VALUE,
    ACTION_FIELD_CONTROLS,
    ACTION_FIELD_TYPE,
    ACTION_FIELD_COUNT,
    ACTION_FIELD_SCREEN,
    ACTION_FIELD_SAME,
    ACTION_FIELD_DATA,
    ACTION_FIELD_DEVICE,
    ACTION_FIELD_KEYCODE,
    ACTION_FIELD_MODS_TO_CLEAR,
};

ActionsInfo *
NewActionsInfo(void)
{
    unsigned type;
    ActionsInfo *info;

    info = calloc(1, sizeof(*info));
    if (!info)
        return NULL;

    /* This includes PrivateAction. */
    for (type = 0; type < XkbSA_NumActions + 1; type++)
        info->actions[type].type = type;

    /* Apply some "factory defaults". */

    /* Increment default button. */
    info->actions[XkbSA_SetPtrDflt].dflt.affect = XkbSA_AffectDfltBtn;
    info->actions[XkbSA_SetPtrDflt].dflt.flags = 0;
    info->actions[XkbSA_SetPtrDflt].dflt.value = 1;

    info->actions[XkbSA_ISOLock].iso.mods.mods =
        (1 << ModNameToIndex(XKB_MOD_NAME_CAPS));

    return info;
}

void
FreeActionsInfo(ActionsInfo *info)
{
    free(info);
}

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
    { "clearLocks",       ACTION_FIELD_CLEAR_LOCKS   },
    { "latchToLock",      ACTION_FIELD_LATCH_TO_LOCK },
    { "genKeyEvent",      ACTION_FIELD_GEN_KEY_EVENT },
    { "generateKeyEvent", ACTION_FIELD_GEN_KEY_EVENT },
    { "report",           ACTION_FIELD_REPORT        },
    { "default",          ACTION_FIELD_DEFAULT       },
    { "affect",           ACTION_FIELD_AFFECT        },
    { "increment",        ACTION_FIELD_INCREMENT     },
    { "modifiers",        ACTION_FIELD_MODIFIERS     },
    { "mods",             ACTION_FIELD_MODIFIERS     },
    { "group",            ACTION_FIELD_GROUP         },
    { "x",                ACTION_FIELD_X             },
    { "y",                ACTION_FIELD_Y             },
    { "accel",            ACTION_FIELD_ACCEL         },
    { "accelerate",       ACTION_FIELD_ACCEL         },
    { "repeat",           ACTION_FIELD_ACCEL         },
    { "button",           ACTION_FIELD_BUTTON        },
    { "value",            ACTION_FIELD_VALUE         },
    { "controls",         ACTION_FIELD_CONTROLS      },
    { "ctrls",            ACTION_FIELD_CONTROLS      },
    { "type",             ACTION_FIELD_TYPE          },
    { "count",            ACTION_FIELD_COUNT         },
    { "screen",           ACTION_FIELD_SCREEN        },
    { "same",             ACTION_FIELD_SAME          },
    { "sameServer",       ACTION_FIELD_SAME          },
    { "data",             ACTION_FIELD_DATA          },
    { "device",           ACTION_FIELD_DEVICE        },
    { "dev",              ACTION_FIELD_DEVICE        },
    { "key",              ACTION_FIELD_KEYCODE       },
    { "keycode",          ACTION_FIELD_KEYCODE       },
    { "kc",               ACTION_FIELD_KEYCODE       },
    { "clearmods",        ACTION_FIELD_MODS_TO_CLEAR },
    { "clearmodifiers",   ACTION_FIELD_MODS_TO_CLEAR },
    { NULL,               0                          }
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
stringToField(const char *str, enum action_field *field_rtrn)
{
    return stringToValue(fieldStrings, str, field_rtrn);
}

static const char *
fieldText(enum action_field field)
{
    return valueToString(fieldStrings, field);
}

/***====================================================================***/

static inline bool
ReportMismatch(struct xkb_keymap *keymap, unsigned action,
               enum action_field field, const char *type)
{
    log_err(keymap->ctx,
            "Value of %s field must be of type %s; "
            "Action %s definition ignored\n",
            fieldText(field), type, ActionTypeText(action));
    return false;
}

static inline bool
ReportIllegal(struct xkb_keymap *keymap, unsigned action,
              enum action_field field)
{
    log_err(keymap->ctx,
            "Field %s is not defined for an action of type %s; "
            "Action definition ignored\n",
            fieldText(field), ActionTypeText(action));
    return false;
}

static inline bool
ReportActionNotArray(struct xkb_keymap *keymap, unsigned action,
                     enum action_field field)
{
    log_err(keymap->ctx,
            "The %s field in the %s action is not an array; "
            "Action definition ignored\n",
            fieldText(field), ActionTypeText(action));
    return false;
}

static inline bool
ReportNotFound(struct xkb_keymap *keymap, unsigned action,
               enum action_field field, const char *what, const char *bad)
{
    log_err(keymap->ctx,
            "%s named %s not found; "
            "Ignoring the %s field of an %s action\n",
            what, bad, fieldText(field), ActionTypeText(action));
    return false;
}

static bool
HandleNoAction(struct xkb_keymap *keymap, union xkb_action *action,
               enum action_field field, const ExprDef *array_ndx,
               const ExprDef *value)

{
    return ReportIllegal(keymap, action->type, field);
}

static bool
CheckLatchLockFlags(struct xkb_keymap *keymap, unsigned action,
                    enum action_field field, const ExprDef * value,
                    unsigned *flags_inout)
{
    unsigned tmp;
    bool result;

    if (field == ACTION_FIELD_CLEAR_LOCKS)
        tmp = XkbSA_ClearLocks;
    else if (field == ACTION_FIELD_LATCH_TO_LOCK)
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
        return ReportMismatch(keymap, action,
                              ACTION_FIELD_MODIFIERS, "modifier mask");

    *flags_inout &= ~XkbSA_UseModMapMods;
    return true;
}

static bool
HandleSetLatchMods(struct xkb_keymap *keymap, union xkb_action *action,
                   enum action_field field, const ExprDef *array_ndx,
                   const ExprDef *value)
{
    struct xkb_mod_action *act = &action->mods;
    unsigned rtrn;
    unsigned t1;
    xkb_mod_mask_t t2;

    if (array_ndx != NULL) {
        switch (field) {
        case ACTION_FIELD_CLEAR_LOCKS:
        case ACTION_FIELD_LATCH_TO_LOCK:
        case ACTION_FIELD_MODIFIERS:
            return ReportActionNotArray(keymap, action->type, field);
        default:
            break;
        }
    }

    switch (field) {
    case ACTION_FIELD_CLEAR_LOCKS:
    case ACTION_FIELD_LATCH_TO_LOCK:
        rtrn = act->flags;
        if (CheckLatchLockFlags(keymap, action->type, field, value, &rtrn)) {
            act->flags = rtrn;
            return true;
        }
        return false;

    case ACTION_FIELD_MODIFIERS:
        t1 = act->flags;
        if (CheckModifierField(keymap, action->type, value, &t1, &t2)) {
            act->flags = t1;
            act->mods.mods = t2;
            return true;
        }
        return false;

    default:
        break;
    }

    return ReportIllegal(keymap, action->type, field);
}

static bool
HandleLockMods(struct xkb_keymap *keymap, union xkb_action *action,
               enum action_field field, const ExprDef *array_ndx,
               const ExprDef *value)
{
    struct xkb_mod_action *act = &action->mods;
    unsigned t1;
    xkb_mod_mask_t t2;

    if (array_ndx && field == ACTION_FIELD_MODIFIERS)
        return ReportActionNotArray(keymap, action->type, field);

    switch (field) {
    case ACTION_FIELD_MODIFIERS:
        t1 = act->flags;
        if (CheckModifierField(keymap, action->type, value, &t1, &t2)) {
            act->flags = t1;
            act->mods.mods = t2;
            return true;
        }
        return false;

    default:
        break;
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
        return ReportMismatch(keymap, action, ACTION_FIELD_GROUP,
                              "integer (range 1..8)");

    if (value->op == EXPR_NEGATE)
        *grp_rtrn = -*grp_rtrn;
    else if (value->op != EXPR_UNARY_PLUS)
        (*grp_rtrn)--;

    return true;
}

static bool
HandleSetLatchGroup(struct xkb_keymap *keymap, union xkb_action *action,
                    enum action_field field, const ExprDef *array_ndx,
                    const ExprDef *value)
{
    struct xkb_group_action *act = &action->group;
    unsigned rtrn;
    unsigned t1;
    xkb_group_index_t t2;

    if (array_ndx != NULL) {
        switch (field) {
        case ACTION_FIELD_CLEAR_LOCKS:
        case ACTION_FIELD_LATCH_TO_LOCK:
        case ACTION_FIELD_GROUP:
            return ReportActionNotArray(keymap, action->type, field);

        default:
            break;
        }
    }

    switch (field) {
    case ACTION_FIELD_CLEAR_LOCKS:
    case ACTION_FIELD_LATCH_TO_LOCK:
        rtrn = act->flags;
        if (CheckLatchLockFlags(keymap, action->type, field, value, &rtrn)) {
            act->flags = rtrn;
            return true;
        }
        return false;

    case ACTION_FIELD_GROUP:
        t1 = act->flags;
        if (CheckGroupField(keymap, action->type, value, &t1, &t2)) {
            act->flags = t1;
            act->group = t2;
            return true;
        }
        return false;

    default:
        break;
    }

    return ReportIllegal(keymap, action->type, field);
}

static bool
HandleLockGroup(struct xkb_keymap *keymap, union xkb_action *action,
                enum action_field field, const ExprDef *array_ndx,
                const ExprDef *value)
{
    struct xkb_group_action *act = &action->group;
    unsigned t1;
    xkb_group_index_t t2;

    if ((array_ndx != NULL) && (field == ACTION_FIELD_GROUP))
        return ReportActionNotArray(keymap, action->type, field);
    if (field == ACTION_FIELD_GROUP) {
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
HandleMovePtr(struct xkb_keymap *keymap, union xkb_action *action,
              enum action_field field, const ExprDef *array_ndx,
              const ExprDef *value)
{
    struct xkb_pointer_action *act = &action->ptr;
    bool absolute;

    if (array_ndx && (field == ACTION_FIELD_X || field == ACTION_FIELD_Y))
        return ReportActionNotArray(keymap, action->type, field);

    if (field == ACTION_FIELD_X || field == ACTION_FIELD_Y) {
        int val;

        if (value->op == EXPR_NEGATE || value->op == EXPR_UNARY_PLUS)
            absolute = false;
        else
            absolute = true;

        if (!ExprResolveInteger(keymap->ctx, value, &val))
            return ReportMismatch(keymap, action->type, field, "integer");

        if (field == ACTION_FIELD_X) {
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
    else if (field == ACTION_FIELD_ACCEL) {
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
HandlePtrBtn(struct xkb_keymap *keymap, union xkb_action *action,
             enum action_field field, const ExprDef *array_ndx,
             const ExprDef *value)
{
    struct xkb_pointer_button_action *act = &action->btn;

    if (field == ACTION_FIELD_BUTTON) {
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
    else if (action->type == XkbSA_LockPtrBtn &&
             field == ACTION_FIELD_AFFECT) {
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
    else if (field == ACTION_FIELD_COUNT) {
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
HandleSetPtrDflt(struct xkb_keymap *keymap, union xkb_action *action,
                 enum action_field field, const ExprDef *array_ndx,
                 const ExprDef *value)
{
    struct xkb_pointer_default_action *act = &action->dflt;

    if (field == ACTION_FIELD_AFFECT) {
        unsigned int val;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        if (!ExprResolveEnum(keymap->ctx, value, &val, ptrDflts))
            return ReportMismatch(keymap, action->type, field,
                                  "pointer component");
        act->affect = val;
        return true;
    }
    else if (field == ACTION_FIELD_BUTTON || field == ACTION_FIELD_VALUE) {
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
HandleISOLock(struct xkb_keymap *keymap, union xkb_action *action,
              enum action_field field, const ExprDef *array_ndx,
              const ExprDef *value)
{
    struct xkb_iso_action *act = &action->iso;

    if (field == ACTION_FIELD_MODIFIERS) {
        unsigned flags;
        xkb_mod_mask_t mods;

        if (array_ndx)
            return ReportActionNotArray(keymap, action->type, field);

        flags = act->flags;
        if (!CheckModifierField(keymap, action->type, value, &flags, &mods))
            return false;

        act->flags = flags & (~XkbSA_ISODfltIsGroup);
        act->mods.mods = mods;
        return true;
    }
    else if (field == ACTION_FIELD_GROUP) {
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
    } else if (ACTION_FIELD_AFFECT) {
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
HandleSwitchScreen(struct xkb_keymap *keymap, union xkb_action *action,
                   enum action_field field, const ExprDef *array_ndx,
                   const ExprDef *value)
{
    struct xkb_switch_screen_action *act = &action->screen;

    if (field == ACTION_FIELD_SCREEN) {
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
    else if (field == ACTION_FIELD_SAME) {
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
HandleSetLockControls(struct xkb_keymap *keymap, union xkb_action *action,
                      enum action_field field, const ExprDef *array_ndx,
                      const ExprDef *value)
{
    struct xkb_controls_action *act = &action->ctrls;

    if (field == ACTION_FIELD_CONTROLS) {
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
HandleActionMessage(struct xkb_keymap *keymap, union xkb_action *action,
                    enum action_field field, const ExprDef *array_ndx,
                    const ExprDef *value)
{
    struct xkb_message_action *act = &action->msg;

    if (field == ACTION_FIELD_REPORT) {
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
    else if (field == ACTION_FIELD_GEN_KEY_EVENT) {
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
    else if (field == ACTION_FIELD_DATA && !array_ndx) {
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
    else if (field == ACTION_FIELD_DATA && array_ndx) {
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
HandleRedirectKey(struct xkb_keymap *keymap, union xkb_action *action,
                  enum action_field field, const ExprDef *array_ndx,
                  const ExprDef *value)
{
    struct xkb_key *key;
    struct xkb_redirect_key_action *act = &action->redirect;
    unsigned t1;
    xkb_mod_mask_t t2;
    unsigned long tmp;
    char key_name[XkbKeyNameLength];

    if (array_ndx != NULL)
        return ReportActionNotArray(keymap, action->type, field);

    switch (field) {
    case ACTION_FIELD_KEYCODE:
        if (!ExprResolveKeyName(keymap->ctx, value, key_name))
            return ReportMismatch(keymap, action->type, field, "key name");

        tmp = KeyNameToLong(key_name);
        key = FindNamedKey(keymap, tmp, true, 0);
        if (!key)
            return ReportNotFound(keymap, action->type, field, "Key",
                                  KeyNameText(key_name));
        act->new_kc = XkbKeyGetKeycode(keymap, key);
        return true;

    case ACTION_FIELD_MODS_TO_CLEAR:
    case ACTION_FIELD_MODIFIERS:
        t1 = 0;
        if (CheckModifierField(keymap, action->type, value, &t1, &t2)) {
            act->mods_mask |= (t2 & 0xff);
            if (field == ACTION_FIELD_MODIFIERS)
                act->mods |= (t2 & 0xff);
            else
                act->mods &= ~(t2 & 0xff);

            t2 = (t2 >> XkbNumModifiers) & 0xffff;
            act->vmods_mask |= t2;
            if (field == ACTION_FIELD_MODIFIERS)
                act->vmods |= t2;
            else
                act->vmods &= ~t2;
            return true;
        }
        return true;

    default:
        break;
    }

    return ReportIllegal(keymap, action->type, field);
}

static bool
HandleDeviceBtn(struct xkb_keymap *keymap, union xkb_action *action,
                enum action_field field, const ExprDef *array_ndx,
                const ExprDef *value)
{
    struct xkb_device_button_action *act = &action->devbtn;

    if (field == ACTION_FIELD_BUTTON) {
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
    else if (action->type == XkbSA_LockDeviceBtn &&
             field == ACTION_FIELD_AFFECT) {
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
    else if (field == ACTION_FIELD_COUNT) {
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
    else if (field == ACTION_FIELD_DEVICE) {
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
HandleDeviceValuator(struct xkb_keymap *keymap, union xkb_action *action,
                     enum action_field field, const ExprDef *array_ndx,
                     const ExprDef *value)
{
    struct xkb_device_valuator_action *act = &action->devval;
    (void) act;

    /*  XXX - Not yet implemented */
    return false;
}

static bool
HandlePrivate(struct xkb_keymap *keymap, union xkb_action *action,
              enum action_field field, const ExprDef *array_ndx,
              const ExprDef *value)
{
    struct xkb_private_action *act = &action->priv;

    if (field == ACTION_FIELD_TYPE) {
        int type;

        if (!ExprResolveInteger(keymap->ctx, value, &type))
            return ReportMismatch(keymap, PrivateAction, field, "integer");

        if (type < 0 || type > 255) {
            log_err(keymap->ctx,
                    "Private action type must be in the range 0..255; "
                    "Illegal type %d ignored\n", type);
            return false;
        }

        act->type = (uint8_t) type;
        return true;
    }
    else if (field == ACTION_FIELD_DATA) {
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

            strncpy((char *) act->data, str, sizeof(act->data));
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

            if (ndx < 0 || ndx >= sizeof(act->data)) {
                log_err(keymap->ctx,
                        "The data for a private action is %zu bytes long; "
                        "Attempt to use data[%d] ignored\n",
                        sizeof(act->data), ndx);
                return false;
            }

            if (!ExprResolveInteger(keymap->ctx, value, &datum))
                return ReportMismatch(keymap, act->type, field, "integer");

            if (datum < 0 || datum > 255) {
                log_err(keymap->ctx,
                        "All data for a private action must be 0..255; "
                        "Illegal datum %d ignored\n", datum);
                return false;
            }

            act->data[ndx] = (uint8_t) datum;
            return true;
        }
    }

    return ReportIllegal(keymap, PrivateAction, field);
}

typedef bool (*actionHandler)(struct xkb_keymap *keymap,
                              union xkb_action *action,
                              enum action_field field,
                              const ExprDef *array_ndx,
                              const ExprDef *value);

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

bool
HandleActionDef(ExprDef *def, struct xkb_keymap *keymap,
                union xkb_action *action, ActionsInfo *info)
{
    ExprDef *arg;
    const char *str;
    unsigned hndlrType;

    if (def->op != EXPR_ACTION_DECL) {
        log_err(keymap->ctx, "Expected an action definition, found %s\n",
                expr_op_type_to_string(def->op));
        return false;
    }

    str = xkb_atom_text(keymap->ctx, def->value.action.name);
    if (!stringToAction(str, &hndlrType)) {
        log_err(keymap->ctx, "Unknown action %s\n", str);
        return false;
    }

    /*
     * Get the default values for this action type, as modified by
     * statements such as:
     *     latchMods.clearLocks = True;
     */
    *action = info->actions[hndlrType];

    /*
     * Now change the action properties as specified for this
     * particular instance, e.g. "modifiers" and "clearLocks" in:
     *     SetMods(modifiers=Alt,clearLocks);
     */
    for (arg = def->value.action.args; arg != NULL;
         arg = (ExprDef *) arg->common.next) {
        const ExprDef *value;
        ExprDef *field, *arrayRtrn;
        const char *elemRtrn, *fieldRtrn;
        enum action_field fieldNdx;

        if (arg->op == EXPR_ASSIGN) {
            field = arg->value.binary.left;
            value = arg->value.binary.right;
        }
        else if (arg->op == EXPR_NOT || arg->op == EXPR_INVERT) {
            field = arg->value.child;
            value = &constFalse;
        }
        else {
            field = arg;
            value = &constTrue;
        }

        if (!ExprResolveLhs(keymap->ctx, field, &elemRtrn, &fieldRtrn,
                            &arrayRtrn))
            return false;

        if (elemRtrn) {
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


bool
SetActionField(struct xkb_keymap *keymap, const char *elem, const char *field,
               ExprDef *array_ndx, ExprDef *value, ActionsInfo *info)
{
    unsigned action;
    enum action_field action_field;

    if (!stringToAction(elem, &action))
        return false;

    if (action == XkbSA_NoAction) {
        log_err(keymap->ctx,
                "\"%s\" is not a valid field in a NoAction action\n",
                field);
        return false;
    }

    if (!stringToField(field, &action_field)) {
        log_err(keymap->ctx, "\"%s\" is not a legal field name\n", field);
        return false;
    }

    return handleAction[action](keymap, &info->actions[action],
                                action_field, array_ndx, value);
}
