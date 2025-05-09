/*
 * For HPND:
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * For MIT:
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * SPDX-License-Identifier: HPND AND MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 * Author: Ran Benita <ran234@gmail.com>
 */

#include "config.h"

#include "context.h"
#include "xkbcomp-priv.h"
#include "text.h"
#include "expr.h"
#include "xkbcomp/ast.h"
#include "action.h"

static const ExprBoolean constTrue = {
    .common = { .type = STMT_EXPR_BOOLEAN_LITERAL, .next = NULL },
    .set = true,
};

static const ExprBoolean constFalse = {
    .common = { .type = STMT_EXPR_BOOLEAN_LITERAL, .next = NULL },
    .set = false,
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

void
InitActionsInfo(ActionsInfo *info)
{
    for (enum xkb_action_type type = 0; type < _ACTION_TYPE_NUM_ENTRIES; type++)
        info->actions[type].type = type;

    /* Apply some "factory defaults". */

    /* Increment default button. */
    info->actions[ACTION_TYPE_PTR_DEFAULT].dflt.flags = 0;
    info->actions[ACTION_TYPE_PTR_DEFAULT].dflt.value = 1;
    info->actions[ACTION_TYPE_PTR_MOVE].ptr.flags = ACTION_ACCEL;
    info->actions[ACTION_TYPE_SWITCH_VT].screen.flags = ACTION_SAME_SCREEN;
}

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
stringToActionType(const char *str, enum xkb_action_type *type_rtrn)
{
    unsigned int type = 0;
    const bool ret = LookupString(actionTypeNames, str, &type);
    *type_rtrn = (enum xkb_action_type) type;
    return ret;
}

static bool
stringToField(const char *str, enum action_field *field_rtrn)
{
    unsigned int field = 0;
    const bool ret = LookupString(fieldStrings, str, &field);
    *field_rtrn = (enum action_field) field;
    return ret;
}

static const char *
fieldText(enum action_field field)
{
    return LookupValue(fieldStrings, field);
}

/***====================================================================***/

static inline bool
ReportMismatch(struct xkb_context *ctx, xkb_message_code_t code,
               enum xkb_action_type action, enum action_field field,
               const char *type)
{
    log_err(ctx, code,
            "Value of %s field must be of type %s; "
            "Action %s definition ignored\n",
            fieldText(field), type, ActionTypeText(action));
    return false;
}

static inline bool
ReportIllegal(struct xkb_context *ctx, enum xkb_action_type action,
              enum action_field field)
{
    log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
            "Field %s is not defined for an action of type %s; "
            "Action definition ignored\n",
            fieldText(field), ActionTypeText(action));
    return false;
}

static inline bool
ReportActionNotArray(struct xkb_context *ctx, enum xkb_action_type action,
                     enum action_field field)
{
    log_err(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
            "The %s field in the %s action is not an array; "
            "Action definition ignored\n",
            fieldText(field), ActionTypeText(action));
    return false;
}

static bool
HandleNoAction(struct xkb_context *ctx, const struct xkb_mod_set *mods,
               union xkb_action *action, enum action_field field,
               const ExprDef *array_ndx, const ExprDef *value)

{
    return true;
}

static bool
CheckBooleanFlag(struct xkb_context *ctx, enum xkb_action_type action,
                 enum action_field field, enum xkb_action_flags flag,
                 const ExprDef *array_ndx, const ExprDef *value,
                 enum xkb_action_flags *flags_inout)
{
    bool set = false;

    if (array_ndx)
        return ReportActionNotArray(ctx, action, field);

    if (!ExprResolveBoolean(ctx, value, &set))
        return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                              action, field, "boolean");

    if (set)
        *flags_inout |= flag;
    else
        *flags_inout &= ~flag;

    return true;
}

static bool
CheckModifierField(struct xkb_context *ctx, const struct xkb_mod_set *mods,
                   enum xkb_action_type action, const ExprDef *array_ndx,
                   const ExprDef *value, enum xkb_action_flags *flags_inout,
                   xkb_mod_mask_t *mods_rtrn)
{
    if (array_ndx)
        return ReportActionNotArray(ctx, action, ACTION_FIELD_MODIFIERS);

    if (value->common.type == STMT_EXPR_IDENT) {
        const char *valStr;
        valStr = xkb_atom_text(ctx, value->ident.ident);
        if (valStr && (istreq(valStr, "usemodmapmods") ||
                       istreq(valStr, "modmapmods"))) {
            *mods_rtrn = 0;
            *flags_inout |= ACTION_MODS_LOOKUP_MODMAP;
            return true;
        }
    }

    if (!ExprResolveModMask(ctx, value, MOD_BOTH, mods, mods_rtrn))
        return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE, action,
                              ACTION_FIELD_MODIFIERS, "modifier mask");

    *flags_inout &= ~ACTION_MODS_LOOKUP_MODMAP;
    return true;
}

static const LookupEntry lockWhich[] = {
    { "both", 0 },
    { "lock", ACTION_LOCK_NO_UNLOCK },
    { "neither", (ACTION_LOCK_NO_LOCK | ACTION_LOCK_NO_UNLOCK) },
    { "unlock", ACTION_LOCK_NO_LOCK },
    { NULL, 0 }
};

static bool
CheckAffectField(struct xkb_context *ctx, enum xkb_action_type action,
                 const ExprDef *array_ndx, const ExprDef *value,
                 enum xkb_action_flags *flags_inout)
{
    if (array_ndx)
        return ReportActionNotArray(ctx, action, ACTION_FIELD_AFFECT);

    uint32_t flags = 0;
    if (!ExprResolveEnum(ctx, value, &flags, lockWhich))
        return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                              action, ACTION_FIELD_AFFECT,
                              "lock, unlock, both, neither");

    *flags_inout &= ~(ACTION_LOCK_NO_LOCK | ACTION_LOCK_NO_UNLOCK);
    *flags_inout |= (enum xkb_action_flags) flags;
    return true;
}

static bool
HandleSetLatchLockMods(struct xkb_context *ctx, const struct xkb_mod_set *mods,
                       union xkb_action *action, enum action_field field,
                       const ExprDef *array_ndx, const ExprDef *value)
{
    struct xkb_mod_action *act = &action->mods;
    const enum xkb_action_type type = action->type;

    if (field == ACTION_FIELD_MODIFIERS)
        return CheckModifierField(ctx, mods, action->type, array_ndx, value,
                                  &act->flags, &act->mods.mods);
    if ((type == ACTION_TYPE_MOD_SET || type == ACTION_TYPE_MOD_LATCH) &&
        field == ACTION_FIELD_CLEAR_LOCKS)
        return CheckBooleanFlag(ctx, action->type, field,
                                ACTION_LOCK_CLEAR, array_ndx, value,
                                &act->flags);
    if (type == ACTION_TYPE_MOD_LATCH &&
        field == ACTION_FIELD_LATCH_TO_LOCK)
        return CheckBooleanFlag(ctx, action->type, field,
                                ACTION_LATCH_TO_LOCK, array_ndx, value,
                                &act->flags);
    if (type == ACTION_TYPE_MOD_LOCK &&
        field == ACTION_FIELD_AFFECT)
        return CheckAffectField(ctx, action->type, array_ndx, value,
                                &act->flags);

    return ReportIllegal(ctx, action->type, field);
}

static bool
CheckGroupField(struct xkb_context *ctx, enum xkb_action_type action,
                const ExprDef *array_ndx, const ExprDef *value,
                enum xkb_action_flags *flags_inout, int32_t *group_rtrn)
{
    const ExprDef *spec;
    xkb_layout_index_t idx = 0;
    enum xkb_action_flags flags = *flags_inout;

    if (array_ndx)
        return ReportActionNotArray(ctx, action, ACTION_FIELD_GROUP);

    if (value->common.type == STMT_EXPR_NEGATE || value->common.type == STMT_EXPR_UNARY_PLUS) {
        flags &= ~ACTION_ABSOLUTE_SWITCH;
        spec = value->unary.child;
    }
    else {
        flags |= ACTION_ABSOLUTE_SWITCH;
        spec = value;
    }

    if (!ExprResolveGroup(ctx, spec, &idx))
        return ReportMismatch(ctx, XKB_ERROR_UNSUPPORTED_GROUP_INDEX, action,
                              ACTION_FIELD_GROUP, "integer (range 1..8)");

    /* +n, -n are relative, n is absolute. */
    if (value->common.type == STMT_EXPR_NEGATE || value->common.type == STMT_EXPR_UNARY_PLUS) {
        *group_rtrn = (int32_t) idx;
        if (value->common.type == STMT_EXPR_NEGATE)
            *group_rtrn = -*group_rtrn;
    }
    else {
        *group_rtrn = (int32_t) (idx - 1);
    }
    *flags_inout = flags;
    return true;
}

static bool
HandleSetLatchLockGroup(struct xkb_context *ctx, const struct xkb_mod_set *mods,
                        union xkb_action *action, enum action_field field,
                        const ExprDef *array_ndx, const ExprDef *value)
{
    struct xkb_group_action *act = &action->group;
    const enum xkb_action_type type = action->type;

    if (field == ACTION_FIELD_GROUP)
        return CheckGroupField(ctx, action->type, array_ndx, value,
                               &act->flags, &act->group);
    if ((type == ACTION_TYPE_GROUP_SET || type == ACTION_TYPE_GROUP_LATCH) &&
        field == ACTION_FIELD_CLEAR_LOCKS)
        return CheckBooleanFlag(ctx, action->type, field,
                                ACTION_LOCK_CLEAR, array_ndx, value,
                                &act->flags);
    if (type == ACTION_TYPE_GROUP_LATCH &&
        field == ACTION_FIELD_LATCH_TO_LOCK)
        return CheckBooleanFlag(ctx, action->type, field,
                                ACTION_LATCH_TO_LOCK, array_ndx, value,
                                &act->flags);

    return ReportIllegal(ctx, action->type, field);
}

static bool
HandleMovePtr(struct xkb_context *ctx, const struct xkb_mod_set *mods,
              union xkb_action *action, enum action_field field,
              const ExprDef *array_ndx, const ExprDef *value)
{
    struct xkb_pointer_action *act = &action->ptr;

    if (field == ACTION_FIELD_X || field == ACTION_FIELD_Y) {
        int64_t val = 0;
        const bool absolute = (value->common.type != STMT_EXPR_NEGATE &&
                               value->common.type != STMT_EXPR_UNARY_PLUS);

        if (array_ndx)
            return ReportActionNotArray(ctx, action->type, field);

        if (!ExprResolveInteger(ctx, value, &val))
            return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE, action->type,
                                  field, "integer");

        if (val < INT16_MIN || val > INT16_MAX) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "The %s field in the %s action must be in range %d..%d, "
                    "but got %"PRId64". Action definition ignored\n",
                    fieldText(field), ActionTypeText(action->type),
                    INT16_MIN, INT16_MAX, val);
            return false;
        }

        if (field == ACTION_FIELD_X) {
            if (absolute)
                act->flags |= ACTION_ABSOLUTE_X;
            act->x = (int16_t) val;
        }
        else {
            if (absolute)
                act->flags |= ACTION_ABSOLUTE_Y;
            act->y = (int16_t) val;
        }

        return true;
    }
    else if (field == ACTION_FIELD_ACCEL) {
        return CheckBooleanFlag(ctx, action->type, field,
                                ACTION_ACCEL, array_ndx, value, &act->flags);
    }

    return ReportIllegal(ctx, action->type, field);
}

static bool
HandlePtrBtn(struct xkb_context *ctx, const struct xkb_mod_set *mods,
             union xkb_action *action, enum action_field field,
             const ExprDef *array_ndx, const ExprDef *value)
{
    struct xkb_pointer_button_action *act = &action->btn;

    if (field == ACTION_FIELD_BUTTON) {
        int64_t btn = 0;

        if (array_ndx)
            return ReportActionNotArray(ctx, action->type, field);

        if (!ExprResolveButton(ctx, value, &btn))
            return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE, action->type,
                                  field, "integer (range 1..5)");

        if (btn < 0 || btn > 5) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Button must specify default or be in the range 1..5; "
                    "Illegal button value %"PRId64" ignored\n", btn);
            return false;
        }

        act->button = (uint8_t) btn;
        return true;
    }
    else if (action->type == ACTION_TYPE_PTR_LOCK &&
             field == ACTION_FIELD_AFFECT) {
        return CheckAffectField(ctx, action->type, array_ndx, value,
                                &act->flags);
    }
    else if (field == ACTION_FIELD_COUNT) {
        int64_t val = 0;

        if (array_ndx)
            return ReportActionNotArray(ctx, action->type, field);

        if (!ExprResolveInteger(ctx, value, &val))
            return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE, action->type,
                                  field, "integer");

        if (val < 0 || val > 255) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "The count field must have a value in the range 0..255; "
                    "Illegal count %"PRId64" ignored\n", val);
            return false;
        }

        act->count = (uint8_t) val;
        return true;
    }

    return ReportIllegal(ctx, action->type, field);
}

static const LookupEntry ptrDflts[] = {
    { "dfltbtn", 1 },
    { "defaultbutton", 1 },
    { "button", 1 },
    { NULL, 0 }
};

static bool
HandleSetPtrDflt(struct xkb_context *ctx, const struct xkb_mod_set *mods,
                 union xkb_action *action, enum action_field field,
                 const ExprDef *array_ndx, const ExprDef *value)
{
    struct xkb_pointer_default_action *act = &action->dflt;

    if (field == ACTION_FIELD_AFFECT) {
        uint32_t val = 0;

        if (array_ndx)
            return ReportActionNotArray(ctx, action->type, field);

        if (!ExprResolveEnum(ctx, value, &val, ptrDflts))
            return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE, action->type,
                                  field, "pointer component");
        return true;
    }
    else if (field == ACTION_FIELD_BUTTON || field == ACTION_FIELD_VALUE) {
        const ExprDef *button;
        int64_t btn = 0;

        if (array_ndx)
            return ReportActionNotArray(ctx, action->type, field);

        if (value->common.type == STMT_EXPR_NEGATE ||
            value->common.type == STMT_EXPR_UNARY_PLUS) {
            act->flags &= ~ACTION_ABSOLUTE_SWITCH;
            button = value->unary.child;
        }
        else {
            act->flags |= ACTION_ABSOLUTE_SWITCH;
            button = value;
        }

        if (!ExprResolveButton(ctx, button, &btn))
            return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE, action->type,
                                  field, "integer (range 1..5)");

        if (btn < 0 || btn > 5) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "New default button value must be in the range 1..5; "
                    "Illegal default button value %"PRId64" ignored\n", btn);
            return false;
        }
        if (btn == 0) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Cannot set default pointer button to \"default\"; "
                    "Illegal default button setting ignored\n");
            return false;
        }

        act->value = (int8_t) (value->common.type == STMT_EXPR_NEGATE ? -btn: btn);
        return true;
    }

    return ReportIllegal(ctx, action->type, field);
}

static bool
HandleSwitchScreen(struct xkb_context *ctx, const struct xkb_mod_set *mods,
                   union xkb_action *action, enum action_field field,
                   const ExprDef *array_ndx, const ExprDef *value)
{
    struct xkb_switch_screen_action *act = &action->screen;

    if (field == ACTION_FIELD_SCREEN) {
        const ExprDef *scrn;
        int64_t val = 0;

        if (array_ndx)
            return ReportActionNotArray(ctx, action->type, field);

        if (value->common.type == STMT_EXPR_NEGATE ||
            value->common.type == STMT_EXPR_UNARY_PLUS) {
            act->flags &= ~ACTION_ABSOLUTE_SWITCH;
            scrn = value->unary.child;
        }
        else {
            act->flags |= ACTION_ABSOLUTE_SWITCH;
            scrn = value;
        }

        if (!ExprResolveInteger(ctx, scrn, &val))
            return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE, action->type,
                                  field, "integer (-128..127)");

        val = (value->common.type == STMT_EXPR_NEGATE ? -val : val);
        if (val < INT8_MIN || val > INT8_MAX) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Screen index must be in the range %d..%d; "
                    "Illegal screen value %"PRId64" ignored\n",
                    INT8_MIN, INT8_MAX, val);
            return false;
        }

        act->screen = (int8_t) val;
        return true;
    }
    else if (field == ACTION_FIELD_SAME) {
        return CheckBooleanFlag(ctx, action->type, field,
                                ACTION_SAME_SCREEN, array_ndx, value,
                                &act->flags);
    }

    return ReportIllegal(ctx, action->type, field);
}

static bool
HandleSetLockControls(struct xkb_context *ctx, const struct xkb_mod_set *mods,
                      union xkb_action *action, enum action_field field,
                      const ExprDef *array_ndx, const ExprDef *value)
{
    struct xkb_controls_action *act = &action->ctrls;

    if (field == ACTION_FIELD_CONTROLS) {
        if (array_ndx)
            return ReportActionNotArray(ctx, action->type, field);

        uint32_t mask = 0;
        if (!ExprResolveMask(ctx, value, &mask, ctrlMaskNames))
            return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE, action->type,
                                  field, "controls mask");

        act->ctrls = (enum xkb_action_controls) mask;
        return true;
    }
    else if (field == ACTION_FIELD_AFFECT) {
        return CheckAffectField(ctx, action->type, array_ndx, value,
                                &act->flags);
    }

    return ReportIllegal(ctx, action->type, field);
}

static bool
HandlePrivate(struct xkb_context *ctx, const struct xkb_mod_set *mods,
              union xkb_action *action, enum action_field field,
              const ExprDef *array_ndx, const ExprDef *value)
{
    struct xkb_private_action *act = &action->priv;

    if (field == ACTION_FIELD_TYPE) {
        int64_t type = 0;

        if (array_ndx)
            return ReportActionNotArray(ctx, action->type, field);

        if (!ExprResolveInteger(ctx, value, &type))
            return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                                  ACTION_TYPE_PRIVATE, field, "integer");

        if (type < 0 || type > 255) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Private action type must be in the range 0..255; "
                    "Illegal type %"PRId64" ignored\n", type);
            return false;
        }

        /*
         * It's possible for someone to write something like this:
         *      actions = [ Private(type=3,data[0]=1,data[1]=3,data[2]=3) ]
         * where the type refers to some existing action type, e.g. LockMods.
         * This assumes that this action's struct is laid out in memory
         * exactly as described in the XKB specification and libraries.
         * We, however, have changed these structs in various ways, so this
         * assumption is no longer true. Since this is a lousy "feature", we
         * make actions like these no-ops for now.
         */
        if (type < ACTION_TYPE_PRIVATE) {
            log_info(ctx, XKB_LOG_MESSAGE_NO_ID,
                     "Private actions of type %s are not supported; Ignored\n",
                     ActionTypeText((enum xkb_action_type) type));
            act->type = ACTION_TYPE_NONE;
        }
        else {
            act->type = (enum xkb_action_type) type;
        }

        return true;
    }
    else if (field == ACTION_FIELD_DATA) {
        if (array_ndx == NULL) {
            xkb_atom_t val = XKB_ATOM_NONE;

            if (!ExprResolveString(ctx, value, &val))
                return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                                      action->type, field, "string");

            const char *str = xkb_atom_text(ctx, val);
            size_t len = strlen(str);
            if (len < 1 || len > sizeof(act->data)) {
                log_warn(ctx, XKB_LOG_MESSAGE_NO_ID,
                         "A private action has %ld data bytes; "
                         "Illegal data ignored\n", sizeof(act->data));
                return false;
            }

            /* act->data may not be null-terminated, this is intentional */
            memset(act->data, 0, sizeof(act->data));
            memcpy(act->data, str, len);
            return true;
        }
        else {
            int64_t ndx = 0, datum = 0;

            if (!ExprResolveInteger(ctx, array_ndx, &ndx)) {
                log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                        "Array subscript must be integer; "
                        "Illegal subscript ignored\n");
                return false;
            }

            if (ndx < 0 || (size_t) ndx >= sizeof(act->data)) {
                log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                        "The data for a private action is %zu bytes long; "
                        "Attempt to use data[%"PRId64"] ignored\n",
                        sizeof(act->data), ndx);
                return false;
            }

            if (!ExprResolveInteger(ctx, value, &datum))
                return ReportMismatch(ctx, XKB_ERROR_WRONG_FIELD_TYPE, act->type,
                                      field, "integer");

            if (datum < 0 || datum > 255) {
                log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                        "All data for a private action must be 0..255; "
                        "Illegal datum %"PRId64" ignored\n", datum);
                return false;
            }

            act->data[ndx] = (uint8_t) datum;
            return true;
        }
    }

    return ReportIllegal(ctx, ACTION_TYPE_NONE, field);
}

typedef bool (*actionHandler)(struct xkb_context *ctx,
                              const struct xkb_mod_set *mods,
                              union xkb_action *action,
                              enum action_field field,
                              const ExprDef *array_ndx,
                              const ExprDef *value);

static const actionHandler handleAction[_ACTION_TYPE_NUM_ENTRIES] = {
    [ACTION_TYPE_NONE] = HandleNoAction,
    [ACTION_TYPE_VOID] = HandleNoAction,
    [ACTION_TYPE_MOD_SET] = HandleSetLatchLockMods,
    [ACTION_TYPE_MOD_LATCH] = HandleSetLatchLockMods,
    [ACTION_TYPE_MOD_LOCK] = HandleSetLatchLockMods,
    [ACTION_TYPE_GROUP_SET] = HandleSetLatchLockGroup,
    [ACTION_TYPE_GROUP_LATCH] = HandleSetLatchLockGroup,
    [ACTION_TYPE_GROUP_LOCK] = HandleSetLatchLockGroup,
    [ACTION_TYPE_PTR_MOVE] = HandleMovePtr,
    [ACTION_TYPE_PTR_BUTTON] = HandlePtrBtn,
    [ACTION_TYPE_PTR_LOCK] = HandlePtrBtn,
    [ACTION_TYPE_PTR_DEFAULT] = HandleSetPtrDflt,
    [ACTION_TYPE_TERMINATE] = HandleNoAction,
    [ACTION_TYPE_SWITCH_VT] = HandleSwitchScreen,
    [ACTION_TYPE_CTRL_SET] = HandleSetLockControls,
    [ACTION_TYPE_CTRL_LOCK] = HandleSetLockControls,
    [ACTION_TYPE_PRIVATE] = HandlePrivate,
};

/***====================================================================***/

bool
HandleActionDef(struct xkb_context *ctx, ActionsInfo *info,
                const struct xkb_mod_set *mods, ExprDef *def,
                union xkb_action *action)
{
    if (def->common.type != STMT_EXPR_ACTION_DECL) {
        log_err(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                "Expected an action definition, found %s\n",
                stmt_type_to_string(def->common.type));
        return false;
    }

    const char *str = xkb_atom_text(ctx, def->action.name);
    enum xkb_action_type handler_type;
    if (!stringToActionType(str, &handler_type)) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID, "Unknown action %s\n", str);
        return false;
    }

    /*
     * Get the default values for this action type, as modified by
     * statements such as:
     *     latchMods.clearLocks = True;
     */
    *action = info->actions[handler_type];

    /*
     * Now change the action properties as specified for this
     * particular instance, e.g. "modifiers" and "clearLocks" in:
     *     SetMods(modifiers=Alt,clearLocks);
     */
    for (ExprDef *arg = def->action.args; arg != NULL;
         arg = (ExprDef *) arg->common.next) {
        const ExprDef *value;
        ExprDef *field, *arrayRtrn;
        const char *elemRtrn, *fieldRtrn;

        if (arg->common.type == STMT_EXPR_ASSIGN) {
            field = arg->binary.left;
            value = arg->binary.right;
        }
        else if (arg->common.type == STMT_EXPR_NOT || arg->common.type == STMT_EXPR_INVERT) {
            field = arg->unary.child;
            value = (const ExprDef *) &constFalse;
        }
        else {
            field = arg;
            value = (const ExprDef *) &constTrue;
        }

        if (!ExprResolveLhs(ctx, field, &elemRtrn, &fieldRtrn, &arrayRtrn))
            return false;

        if (elemRtrn) {
            log_err(ctx, XKB_ERROR_GLOBAL_DEFAULTS_WRONG_SCOPE,
                    "Cannot change defaults in an action definition; "
                    "Ignoring attempt to change \"%s.%s\".\n",
                    elemRtrn, fieldRtrn);
            return false;
        }

        enum action_field fieldNdx;
        if (!stringToField(fieldRtrn, &fieldNdx)) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Unknown field name %s\n", fieldRtrn);
            return false;
        }

        if (!handleAction[handler_type](ctx, mods, action, fieldNdx,
                                        arrayRtrn, value))
            return false;
    }

    return true;
}

bool
SetDefaultActionField(struct xkb_context *ctx, ActionsInfo *info,
                      struct xkb_mod_set *mods, const char *elem,
                      const char *field, ExprDef *array_ndx,
                      ExprDef *value, enum merge_mode merge)
{
    enum xkb_action_type action;
    if (!stringToActionType(elem, &action))
        return false;

    enum action_field action_field;
    if (!stringToField(field, &action_field)) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "\"%s\" is not a legal field name\n", field);
        return false;
    }

    union xkb_action* const into = &info->actions[action];
    /* Initialize with current defaults to enable comparison, see aftwerwards */
    union xkb_action from = *into;

    /* Parse action */
    if (!handleAction[action](ctx, mods, &from, action_field, array_ndx, value))
        return false;

    /*
     * Merge action with its corresponding default
     *
     * NOTE: Contrary to other items, actions do not have a “defined” field, so
     * we fallback to compare all the actions fields. The drawback is that it
     * overmatches, as even setting an *explicit* default value for the first
     * time (and different from the “factory” default) would *always* display a
     * warning. So we guard the logging with a high verbosity, as best effort
     * mitigation.
     */
    if (!action_equal(into, &from)) {
        const bool replace = (merge != MERGE_AUGMENT);
        log_vrb(ctx, 9, XKB_LOG_MESSAGE_NO_ID,
                "Conflicting field \"%s\" for default action \"%s\"; "
                "Using %s, ignore %s\n",
                fieldText(action_field), ActionTypeText(action),
                (replace ? "from" : "into"), (replace ? "into" : "from"));
        if (replace)
            *into = from;
    }
    return true;
}
