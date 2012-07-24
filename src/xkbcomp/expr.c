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

#include "expr.h"

typedef bool (*IdentLookupFunc)(struct xkb_context *ctx, const void *priv,
                                xkb_atom_t field, unsigned type,
                                ExprResult *val_rtrn);

const char *
exprOpText(unsigned type)
{
    static char buf[32];

    switch (type) {
    case ExprValue:
        strcpy(buf, "literal");
        break;
    case ExprIdent:
        strcpy(buf, "identifier");
        break;
    case ExprActionDecl:
        strcpy(buf, "action declaration");
        break;
    case ExprFieldRef:
        strcpy(buf, "field reference");
        break;
    case ExprArrayRef:
        strcpy(buf, "array reference");
        break;
    case ExprKeysymList:
        strcpy(buf, "list of keysyms");
        break;
    case ExprActionList:
        strcpy(buf, "list of actions");
        break;
    case OpAdd:
        strcpy(buf, "addition");
        break;
    case OpSubtract:
        strcpy(buf, "subtraction");
        break;
    case OpMultiply:
        strcpy(buf, "multiplication");
        break;
    case OpDivide:
        strcpy(buf, "division");
        break;
    case OpAssign:
        strcpy(buf, "assignment");
        break;
    case OpNot:
        strcpy(buf, "logical not");
        break;
    case OpNegate:
        strcpy(buf, "arithmetic negation");
        break;
    case OpInvert:
        strcpy(buf, "bitwise inversion");
        break;
    case OpUnaryPlus:
        strcpy(buf, "unary plus");
        break;
    default:
        snprintf(buf, sizeof(buf), "illegal(%d)", type);
        break;
    }
    return buf;
}

static const char *
exprTypeText(unsigned type)
{
    static char buf[20];

    switch (type) {
    case TypeUnknown:
        strcpy(buf, "unknown");
        break;
    case TypeBoolean:
        strcpy(buf, "boolean");
        break;
    case TypeInt:
        strcpy(buf, "int");
        break;
    case TypeString:
        strcpy(buf, "string");
        break;
    case TypeAction:
        strcpy(buf, "action");
        break;
    case TypeKeyName:
        strcpy(buf, "keyname");
        break;
    default:
        snprintf(buf, sizeof(buf), "illegal(%d)", type);
        break;
    }
    return buf;
}

int
ExprResolveLhs(struct xkb_keymap *keymap, ExprDef *expr,
               ExprResult *elem_rtrn, ExprResult *field_rtrn,
               ExprDef **index_rtrn)
{
    struct xkb_context *ctx = keymap->ctx;

    switch (expr->op) {
    case ExprIdent:
        elem_rtrn->str = NULL;
        field_rtrn->str = xkb_atom_text(ctx, expr->value.str);
        *index_rtrn = NULL;
        return true;
    case ExprFieldRef:
        elem_rtrn->str = xkb_atom_text(ctx, expr->value.field.element);
        field_rtrn->str = xkb_atom_text(ctx, expr->value.field.field);
        *index_rtrn = NULL;
        return true;
    case ExprArrayRef:
        elem_rtrn->str = xkb_atom_text(ctx, expr->value.array.element);
        field_rtrn->str = xkb_atom_text(ctx, expr->value.array.field);
        *index_rtrn = expr->value.array.entry;
        return true;
    }
    log_wsgo(keymap->ctx, "Unexpected operator %d in ResolveLhs\n", expr->op);
    return false;
}

static bool
SimpleLookup(struct xkb_context *ctx, const void *priv,
             xkb_atom_t field, unsigned type, ExprResult *val_rtrn)
{
    const LookupEntry *entry;
    const char *str;

    if ((priv == NULL) || (field == XKB_ATOM_NONE) || (type != TypeInt))
        return false;

    str = xkb_atom_text(ctx, field);
    for (entry = priv; (entry != NULL) && (entry->name != NULL); entry++) {
        if (istreq(str, entry->name)) {
            val_rtrn->uval = entry->result;
            return true;
        }
    }

    return false;
}

static const LookupEntry modIndexNames[] = {
    { "shift", ShiftMapIndex },
    { "control", ControlMapIndex },
    { "lock", LockMapIndex },
    { "mod1", Mod1MapIndex },
    { "mod2", Mod2MapIndex },
    { "mod3", Mod3MapIndex },
    { "mod4", Mod4MapIndex },
    { "mod5", Mod5MapIndex },
    { "none", XkbNoModifier },
    { NULL, 0 }
};

bool
LookupModIndex(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
               unsigned type, ExprResult *val_rtrn)
{
    return SimpleLookup(ctx, modIndexNames, field, type, val_rtrn);
}

bool
LookupModMask(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
              unsigned type, ExprResult *val_rtrn)
{
    const char *str;
    bool ret = true;

    if (type != TypeInt)
        return false;
    str = xkb_atom_text(ctx, field);
    if (str == NULL)
        return false;
    if (istreq(str, "all"))
        val_rtrn->uval = 0xff;
    else if (istreq(str, "none"))
        val_rtrn->uval = 0;
    else if (LookupModIndex(ctx, priv, field, type, val_rtrn))
        val_rtrn->uval = (1 << val_rtrn->uval);
    else
        ret = false;
    return ret;
}

int
ExprResolveBoolean(struct xkb_context *ctx, ExprDef *expr,
                   ExprResult *val_rtrn)
{
    int ok = 0;
    const char *ident;

    switch (expr->op) {
    case ExprValue:
        if (expr->type != TypeBoolean) {
            log_err(ctx,
                    "Found constant of type %s where boolean was expected\n",
                    exprTypeText(expr->type));
            return false;
        }
        val_rtrn->ival = expr->value.ival;
        return true;

    case ExprIdent:
        ident = xkb_atom_text(ctx, expr->value.str);
        if (ident) {
            if (istreq(ident, "true") ||
                istreq(ident, "yes") ||
                istreq(ident, "on")) {
                val_rtrn->uval = 1;
                return true;
            }
            else if (istreq(ident, "false") ||
                     istreq(ident, "no") ||
                     istreq(ident, "off")) {
                val_rtrn->uval = 0;
                return true;
            }
        }
        log_err(ctx, "Identifier \"%s\" of type boolean is unknown\n",
                xkb_atom_text(ctx, expr->value.str));
        return false;

    case ExprFieldRef:
        log_err(ctx, "Default \"%s.%s\" of type boolean is unknown\n",
                xkb_atom_text(ctx, expr->value.field.element),
                xkb_atom_text(ctx, expr->value.field.field));
        return false;

    case OpInvert:
    case OpNot:
        ok = ExprResolveBoolean(ctx, expr, val_rtrn);
        if (ok)
            val_rtrn->uval = !val_rtrn->uval;
        return ok;
    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
    case OpAssign:
    case OpNegate:
    case OpUnaryPlus:
        log_err(ctx, "%s of boolean values not permitted\n",
                exprOpText(expr->op));
        break;

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveBoolean\n", expr->op);
        break;
    }
    return false;
}

int
ExprResolveKeyCode(struct xkb_context *ctx, ExprDef *expr,
                   ExprResult *val_rtrn)
{
    ExprResult leftRtrn, rightRtrn;
    ExprDef *left, *right;

    switch (expr->op) {
    case ExprValue:
        if (expr->type != TypeInt) {
            log_err(ctx,
                    "Found constant of type %s where an int was expected\n",
                    exprTypeText(expr->type));
            return false;
        }
        val_rtrn->uval = expr->value.uval;
        return true;

    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (ExprResolveKeyCode(ctx, left, &leftRtrn) &&
            ExprResolveKeyCode(ctx, right, &rightRtrn)) {
            switch (expr->op) {
            case OpAdd:
                val_rtrn->uval = leftRtrn.uval + rightRtrn.uval;
                break;
            case OpSubtract:
                val_rtrn->uval = leftRtrn.uval - rightRtrn.uval;
                break;
            case OpMultiply:
                val_rtrn->uval = leftRtrn.uval * rightRtrn.uval;
                break;
            case OpDivide:
                if (rightRtrn.uval == 0) {
                    log_err(ctx, "Cannot divide by zero: %d / %d\n",
                            leftRtrn.uval, rightRtrn.uval);
                    return false;
                }
                val_rtrn->uval = leftRtrn.uval / rightRtrn.uval;
                break;
            }
            return true;
        }
        return false;

    case OpNegate:
        left = expr->value.child;
        if (ExprResolveKeyCode(ctx, left, &leftRtrn)) {
            val_rtrn->uval = ~leftRtrn.uval;
            return true;
        }
        return false;

    case OpUnaryPlus:
        left = expr->value.child;
        return ExprResolveKeyCode(ctx, left, val_rtrn);

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveKeyCode\n", expr->op);
        break;
    }
    return false;
}

/**
 * This function returns ... something.  It's a bit of a guess, really.
 *
 * If a string is given in value ctx, its first character will be
 * returned in uval.  If an integer is given in value ctx, it will be
 * returned in ival.  If a float is given in value ctx, it will be
 * returned as millimetres (rather than points) in ival.
 *
 * If an ident or field reference is given, the lookup function (if given)
 * will be called.  At the moment, only SimpleLookup use this, and they both
 * return the results in uval.  And don't support field references.
 *
 * Cool.
 */
static int
ExprResolveIntegerLookup(struct xkb_context *ctx, ExprDef *expr,
                         ExprResult *val_rtrn, IdentLookupFunc lookup,
                         const void *lookupPriv)
{
    int ok = 0;
    ExprResult leftRtrn, rightRtrn;
    ExprDef *left, *right;

    switch (expr->op) {
    case ExprValue:
        if (expr->type == TypeString) {
            const char *str;
            str = xkb_atom_text(ctx, expr->value.str);
            if (str != NULL)
                switch (strlen(str)) {
                case 0:
                    val_rtrn->uval = 0;
                    return true;
                case 1:
                    val_rtrn->uval = str[0];
                    return true;
                default:
                    break;
                }
        }
        if (expr->type != TypeInt) {
            log_err(ctx,
                    "Found constant of type %s where an int was expected\n",
                    exprTypeText(expr->type));
            return false;
        }
        val_rtrn->ival = expr->value.ival;
        return true;

    case ExprIdent:
        if (lookup)
            ok = lookup(ctx, lookupPriv, expr->value.str,
                        TypeInt, val_rtrn);
        if (!ok)
            log_err(ctx, "Identifier \"%s\" of type int is unknown\n",
                    xkb_atom_text(ctx, expr->value.str));
        return ok;

    case ExprFieldRef:
        log_err(ctx, "Default \"%s.%s\" of type int is unknown\n",
                xkb_atom_text(ctx, expr->value.field.element),
                xkb_atom_text(ctx, expr->value.field.field));
        return false;

    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (ExprResolveIntegerLookup(ctx, left, &leftRtrn, lookup,
                                     lookupPriv) &&
            ExprResolveIntegerLookup(ctx, right, &rightRtrn, lookup,
                                     lookupPriv)) {
            switch (expr->op) {
            case OpAdd:
                val_rtrn->ival = leftRtrn.ival + rightRtrn.ival;
                break;
            case OpSubtract:
                val_rtrn->ival = leftRtrn.ival - rightRtrn.ival;
                break;
            case OpMultiply:
                val_rtrn->ival = leftRtrn.ival * rightRtrn.ival;
                break;
            case OpDivide:
                if (rightRtrn.ival == 0) {
                    log_err(ctx, "Cannot divide by zero: %d / %d\n",
                            leftRtrn.ival, rightRtrn.ival);
                    return false;
                }
                val_rtrn->ival = leftRtrn.ival / rightRtrn.ival;
                break;
            }
            return true;
        }
        return false;

    case OpAssign:
        log_wsgo(ctx, "Assignment operator not implemented yet\n");
        break;

    case OpNot:
        log_err(ctx, "The ! operator cannot be applied to an integer\n");
        return false;

    case OpInvert:
    case OpNegate:
        left = expr->value.child;
        if (ExprResolveIntegerLookup(ctx, left, &leftRtrn, lookup,
                                     lookupPriv)) {
            if (expr->op == OpNegate)
                val_rtrn->ival = -leftRtrn.ival;
            else
                val_rtrn->ival = ~leftRtrn.ival;
            return true;
        }
        return false;

    case OpUnaryPlus:
        left = expr->value.child;
        return ExprResolveIntegerLookup(ctx, left, val_rtrn, lookup,
                                        lookupPriv);

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveInteger\n", expr->op);
        break;
    }
    return false;
}

int
ExprResolveInteger(struct xkb_context *ctx, ExprDef *expr,
                   ExprResult *val_rtrn)
{
    return ExprResolveIntegerLookup(ctx, expr, val_rtrn, NULL, NULL);
}

int
ExprResolveGroup(struct xkb_context *ctx, ExprDef *expr,
                 ExprResult *val_rtrn)
{
    int ret;
    static const LookupEntry group_names[] = {
        { "group1", 1 },
        { "group2", 2 },
        { "group3", 3 },
        { "group4", 4 },
        { "group5", 5 },
        { "group6", 6 },
        { "group7", 7 },
        { "group8", 8 },
        { NULL, 0 }
    };

    ret = ExprResolveIntegerLookup(ctx, expr, val_rtrn, SimpleLookup,
                                   group_names);
    if (ret == false)
        return ret;

    if (val_rtrn->uval == 0 || val_rtrn->uval > XkbNumKbdGroups) {
        log_err(ctx, "Group index %u is out of range (1..%d)\n",
                val_rtrn->uval, XkbNumKbdGroups);
        return false;
    }

    return true;
}

int
ExprResolveLevel(struct xkb_context *ctx, ExprDef *expr,
                 ExprResult *val_rtrn)
{
    int ret;
    static const LookupEntry level_names[] = {
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

    ret = ExprResolveIntegerLookup(ctx, expr, val_rtrn, SimpleLookup,
                                   level_names);
    if (ret == false)
        return ret;

    if (val_rtrn->ival < 1 || val_rtrn->ival > XkbMaxShiftLevel) {
        log_err(ctx, "Shift level %d is out of range (1..%d)\n",
                val_rtrn->ival, XkbMaxShiftLevel);
        return false;
    }

    return true;
}

int
ExprResolveButton(struct xkb_context *ctx, ExprDef *expr,
                  ExprResult *val_rtrn)
{
    static const LookupEntry button_names[] = {
        { "button1", 1 },
        { "button2", 2 },
        { "button3", 3 },
        { "button4", 4 },
        { "button5", 5 },
        { "default", 0 },
        { NULL, 0 }
    };

    return ExprResolveIntegerLookup(ctx, expr, val_rtrn, SimpleLookup,
                                    button_names);
}

int
ExprResolveString(struct xkb_context *ctx, ExprDef *expr,
                  ExprResult *val_rtrn)
{
    switch (expr->op) {
    case ExprValue:
        if (expr->type != TypeString) {
            log_err(ctx, "Found constant of type %s, expected a string\n",
                    exprTypeText(expr->type));
            return false;
        }
        val_rtrn->str = xkb_atom_text(ctx, expr->value.str);
        return true;

    case ExprIdent:
        log_err(ctx, "Identifier \"%s\" of type string not found\n",
                xkb_atom_text(ctx, expr->value.str));
        return false;

    case ExprFieldRef:
        log_err(ctx, "Default \"%s.%s\" of type string not found\n",
                xkb_atom_text(ctx, expr->value.field.element),
                xkb_atom_text(ctx, expr->value.field.field));
        return false;

    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
    case OpAssign:
    case OpNegate:
    case OpInvert:
    case OpNot:
    case OpUnaryPlus:
        log_err(ctx, "%s of strings not permitted\n", exprOpText(expr->op));
        return false;

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveString\n", expr->op);
        break;
    }
    return false;
}

int
ExprResolveKeyName(struct xkb_context *ctx, ExprDef *expr,
                   ExprResult *val_rtrn)
{
    switch (expr->op) {
    case ExprValue:
        if (expr->type != TypeKeyName) {
            log_err(ctx, "Found constant of type %s, expected a key name\n",
                    exprTypeText(expr->type));
            return false;
        }
        memcpy(val_rtrn->name, expr->value.keyName, XkbKeyNameLength);
        return true;

    case ExprIdent:
        log_err(ctx, "Identifier \"%s\" of type string not found\n",
                xkb_atom_text(ctx, expr->value.str));
        return false;

    case ExprFieldRef:
        log_err(ctx, "Default \"%s.%s\" of type key name not found\n",
                xkb_atom_text(ctx, expr->value.field.element),
                xkb_atom_text(ctx, expr->value.field.field));
        return false;

    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
    case OpAssign:
    case OpNegate:
    case OpInvert:
    case OpNot:
    case OpUnaryPlus:
        log_err(ctx, "%s of key name values not permitted\n",
                exprOpText(expr->op));
        return false;

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveKeyName\n", expr->op);
        break;
    }
    return false;
}

/***====================================================================***/

int
ExprResolveEnum(struct xkb_context *ctx, ExprDef *expr,
                ExprResult *val_rtrn, const LookupEntry *values)
{
    if (expr->op != ExprIdent) {
        log_err(ctx, "Found a %s where an enumerated value was expected\n",
                exprOpText(expr->op));
        return false;
    }
    if (!SimpleLookup(ctx, values, expr->value.str, TypeInt, val_rtrn)) {
        int nOut = 0;
        log_err(ctx, "Illegal identifier %s (expected one of: ",
                xkb_atom_text(ctx, expr->value.str));
        while (values && values->name)
        {
            if (nOut != 0)
                log_info(ctx, ", %s", values->name);
            else
                log_info(ctx, "%s", values->name);
            values++;
            nOut++;
        }
        log_info(ctx, ")\n");
        return false;
    }
    return true;
}

static int
ExprResolveMaskLookup(struct xkb_context *ctx, ExprDef *expr,
                      ExprResult *val_rtrn, IdentLookupFunc lookup,
                      const void *lookupPriv)
{
    int ok = 0;
    ExprResult leftRtrn, rightRtrn;
    ExprDef *left, *right;
    const char *bogus = NULL;

    switch (expr->op) {
    case ExprValue:
        if (expr->type != TypeInt) {
            log_err(ctx,
                    "Found constant of type %s where a mask was expected\n",
                    exprTypeText(expr->type));
            return false;
        }
        val_rtrn->ival = expr->value.ival;
        return true;

    case ExprIdent:
        ok = lookup(ctx, lookupPriv, expr->value.str, TypeInt, val_rtrn);
        if (!ok)
            log_err(ctx, "Identifier \"%s\" of type int is unknown\n",
                    xkb_atom_text(ctx, expr->value.str));
        return ok;

    case ExprFieldRef:
        log_err(ctx, "Default \"%s.%s\" of type int is unknown\n",
                xkb_atom_text(ctx, expr->value.field.element),
                xkb_atom_text(ctx, expr->value.field.field));
        return false;

    case ExprArrayRef:
        bogus = "array reference";

    case ExprActionDecl:
        if (bogus == NULL)
            bogus = "function use";
        log_err(ctx,
                "Unexpected %s in mask expression; Expression Ignored\n",
                bogus);
        return false;

    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (ExprResolveMaskLookup(ctx, left, &leftRtrn, lookup,
                                  lookupPriv) &&
            ExprResolveMaskLookup(ctx, right, &rightRtrn, lookup,
                                  lookupPriv)) {
            switch (expr->op) {
            case OpAdd:
                val_rtrn->ival = leftRtrn.ival | rightRtrn.ival;
                break;
            case OpSubtract:
                val_rtrn->ival = leftRtrn.ival & (~rightRtrn.ival);
                break;
            case OpMultiply:
            case OpDivide:
                log_err(ctx, "Cannot %s masks; Illegal operation ignored\n",
                        (expr->op == OpDivide ? "divide" : "multiply"));
                return false;
            }
            return true;
        }
        return false;

    case OpAssign:
        log_wsgo(ctx, "Assignment operator not implemented yet\n");
        break;

    case OpInvert:
        left = expr->value.child;
        if (ExprResolveIntegerLookup(ctx, left, &leftRtrn, lookup,
                                     lookupPriv)) {
            val_rtrn->ival = ~leftRtrn.ival;
            return true;
        }
        return false;

    case OpUnaryPlus:
    case OpNegate:
    case OpNot:
        left = expr->value.child;
        if (ExprResolveIntegerLookup(ctx, left, &leftRtrn, lookup,
                                     lookupPriv)) {
            log_err(ctx, "The %s operator cannot be used with a mask\n",
                    (expr->op == OpNegate ? "-" : "!"));
        }
        return false;

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveMask\n", expr->op);
        break;
    }
    return false;
}

int
ExprResolveMask(struct xkb_context *ctx, ExprDef *expr,
                ExprResult *val_rtrn, const LookupEntry *values)
{
    return ExprResolveMaskLookup(ctx, expr, val_rtrn, SimpleLookup, values);
}

int
ExprResolveModMask(struct xkb_context *ctx, ExprDef *expr,
                   ExprResult *val_rtrn)
{
    return ExprResolveMaskLookup(ctx, expr, val_rtrn, LookupModMask, NULL);
}

int
ExprResolveVModMask(struct xkb_keymap *keymap, ExprDef *expr,
                    ExprResult *val_rtrn)
{
    return ExprResolveMaskLookup(keymap->ctx, expr, val_rtrn, LookupVModMask,
                                 keymap);
}

int
ExprResolveKeySym(struct xkb_context *ctx, ExprDef *expr,
                  ExprResult *val_rtrn)
{
    int ok = 0;
    xkb_keysym_t sym;

    if (expr->op == ExprIdent) {
        const char *str;
        str = xkb_atom_text(ctx, expr->value.str);
        if (str) {
            sym = xkb_keysym_from_name(str);
            if (sym != XKB_KEY_NoSymbol) {
                val_rtrn->uval = sym;
                return true;
            }
        }
    }
    ok = ExprResolveInteger(ctx, expr, val_rtrn);
    if ((ok) && (val_rtrn->uval < 10))
        val_rtrn->uval += '0';
    return ok;
}
