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

typedef bool (*IdentLookupFunc)(struct xkb_context *ctx, const void *priv,
                                xkb_atom_t field, enum expr_value_type type,
                                unsigned int *val_rtrn);

const char *
exprOpText(enum expr_op_type op)
{
    static char buf[32];

    switch (op) {
    case EXPR_VALUE:
        strcpy(buf, "literal");
        break;
    case EXPR_IDENT:
        strcpy(buf, "identifier");
        break;
    case EXPR_ACTION_DECL:
        strcpy(buf, "action declaration");
        break;
    case EXPR_FIELD_REF:
        strcpy(buf, "field reference");
        break;
    case EXPR_ARRAY_REF:
        strcpy(buf, "array reference");
        break;
    case EXPR_KEYSYM_LIST:
        strcpy(buf, "list of keysyms");
        break;
    case EXPR_ACTION_LIST:
        strcpy(buf, "list of actions");
        break;
    case EXPR_ADD:
        strcpy(buf, "addition");
        break;
    case EXPR_SUBTRACT:
        strcpy(buf, "subtraction");
        break;
    case EXPR_MULTIPLY:
        strcpy(buf, "multiplication");
        break;
    case EXPR_DIVIDE:
        strcpy(buf, "division");
        break;
    case EXPR_ASSIGN:
        strcpy(buf, "assignment");
        break;
    case EXPR_NOT:
        strcpy(buf, "logical not");
        break;
    case EXPR_NEGATE:
        strcpy(buf, "arithmetic negation");
        break;
    case EXPR_INVERT:
        strcpy(buf, "bitwise inversion");
        break;
    case EXPR_UNARY_PLUS:
        strcpy(buf, "unary plus");
        break;
    default:
        snprintf(buf, sizeof(buf), "illegal(%d)", op);
        break;
    }
    return buf;
}

static const char *
exprValueTypeText(enum expr_value_type type)
{
    static char buf[20];

    switch (type) {
    case EXPR_TYPE_UNKNOWN:
        strcpy(buf, "unknown");
        break;
    case EXPR_TYPE_BOOLEAN:
        strcpy(buf, "boolean");
        break;
    case EXPR_TYPE_INT:
        strcpy(buf, "int");
        break;
    case EXPR_TYPE_STRING:
        strcpy(buf, "string");
        break;
    case EXPR_TYPE_ACTION:
        strcpy(buf, "action");
        break;
    case EXPR_TYPE_KEYNAME:
        strcpy(buf, "keyname");
        break;
    default:
        snprintf(buf, sizeof(buf), "illegal(%d)", type);
        break;
    }
    return buf;
}

bool
ExprResolveLhs(struct xkb_context *ctx, const ExprDef *expr,
               const char **elem_rtrn, const char **field_rtrn,
               ExprDef **index_rtrn)
{
    switch (expr->op) {
    case EXPR_IDENT:
        *elem_rtrn = NULL;
        *field_rtrn = xkb_atom_text(ctx, expr->value.str);
        *index_rtrn = NULL;
        return true;
    case EXPR_FIELD_REF:
        *elem_rtrn = xkb_atom_text(ctx, expr->value.field.element);
        *field_rtrn = xkb_atom_text(ctx, expr->value.field.field);
        *index_rtrn = NULL;
        return true;
    case EXPR_ARRAY_REF:
        *elem_rtrn = xkb_atom_text(ctx, expr->value.array.element);
        *field_rtrn = xkb_atom_text(ctx, expr->value.array.field);
        *index_rtrn = expr->value.array.entry;
        return true;
    default:
        break;
    }
    log_wsgo(ctx, "Unexpected operator %d in ResolveLhs\n", expr->op);
    return false;
}

static bool
SimpleLookup(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
             enum expr_value_type type, unsigned int *val_rtrn)
{
    const LookupEntry *entry;
    const char *str;

    if (!priv || field == XKB_ATOM_NONE || type != EXPR_TYPE_INT)
        return false;

    str = xkb_atom_text(ctx, field);
    for (entry = priv; entry && entry->name; entry++) {
        if (istreq(str, entry->name)) {
            *val_rtrn = entry->value;
            return true;
        }
    }

    return false;
}

bool
LookupModIndex(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
               enum expr_value_type type, xkb_mod_index_t *val_rtrn)
{
    const char *name = xkb_atom_text(ctx, field);

    if (istreq(name, "none")) {
        *val_rtrn = XkbNoModifier;
        return true;
    }

    *val_rtrn = ModNameToIndex(name);
    return (*val_rtrn != XKB_MOD_INVALID);
}

bool
LookupModMask(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
              enum expr_value_type type, xkb_mod_mask_t *val_rtrn)
{
    const char *str;
    xkb_mod_index_t ndx;

    if (type != EXPR_TYPE_INT)
        return false;

    str = xkb_atom_text(ctx, field);

    if (istreq(str, "all"))
        *val_rtrn  = 0xff;
    else if (istreq(str, "none"))
        *val_rtrn = 0;
    else if (LookupModIndex(ctx, priv, field, type, &ndx))
        *val_rtrn = (1 << ndx);
    else
        return false;

    return true;
}

bool
ExprResolveBoolean(struct xkb_context *ctx, const ExprDef *expr,
                   bool *set_rtrn)
{
    bool ok = false;
    const char *ident;

    switch (expr->op) {
    case EXPR_VALUE:
        if (expr->value_type != EXPR_TYPE_BOOLEAN) {
            log_err(ctx,
                    "Found constant of type %s where boolean was expected\n",
                    exprValueTypeText(expr->value_type));
            return false;
        }
        *set_rtrn = !!expr->value.ival;
        return true;

    case EXPR_IDENT:
        ident = xkb_atom_text(ctx, expr->value.str);
        if (ident) {
            if (istreq(ident, "true") ||
                istreq(ident, "yes") ||
                istreq(ident, "on")) {
                *set_rtrn = true;
                return true;
            }
            else if (istreq(ident, "false") ||
                     istreq(ident, "no") ||
                     istreq(ident, "off")) {
                *set_rtrn = false;
                return true;
            }
        }
        log_err(ctx, "Identifier \"%s\" of type boolean is unknown\n",
                xkb_atom_text(ctx, expr->value.str));
        return false;

    case EXPR_FIELD_REF:
        log_err(ctx, "Default \"%s.%s\" of type boolean is unknown\n",
                xkb_atom_text(ctx, expr->value.field.element),
                xkb_atom_text(ctx, expr->value.field.field));
        return false;

    case EXPR_INVERT:
    case EXPR_NOT:
        ok = ExprResolveBoolean(ctx, expr, set_rtrn);
        if (ok)
            *set_rtrn = !*set_rtrn;
        return ok;
    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_DIVIDE:
    case EXPR_ASSIGN:
    case EXPR_NEGATE:
    case EXPR_UNARY_PLUS:
        log_err(ctx, "%s of boolean values not permitted\n",
                exprOpText(expr->op));
        break;

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveBoolean\n", expr->op);
        break;
    }

    return false;
}

bool
ExprResolveKeyCode(struct xkb_context *ctx, const ExprDef *expr,
                   xkb_keycode_t *kc)
{
    xkb_keycode_t leftRtrn, rightRtrn;
    ExprDef *left, *right;

    switch (expr->op) {
    case EXPR_VALUE:
        if (expr->value_type != EXPR_TYPE_INT) {
            log_err(ctx,
                    "Found constant of type %s where an int was expected\n",
                    exprValueTypeText(expr->value_type));
            return false;
        }

        *kc = expr->value.uval;
        return true;

    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_DIVIDE:
        left = expr->value.binary.left;
        right = expr->value.binary.right;

        if (!ExprResolveKeyCode(ctx, left, &leftRtrn) ||
            !ExprResolveKeyCode(ctx, right, &rightRtrn))
            return false;

        switch (expr->op) {
        case EXPR_ADD:
            *kc = leftRtrn + rightRtrn;
            break;
        case EXPR_SUBTRACT:
            *kc = leftRtrn - rightRtrn;
            break;
        case EXPR_MULTIPLY:
            *kc = leftRtrn * rightRtrn;
            break;
        case EXPR_DIVIDE:
            if (rightRtrn == 0) {
                log_err(ctx, "Cannot divide by zero: %d / %d\n",
                        leftRtrn, rightRtrn);
                return false;
            }

            *kc = leftRtrn / rightRtrn;
            break;
        default:
            break;
        }

        return true;

    case EXPR_NEGATE:
        left = expr->value.child;
        if (!ExprResolveKeyCode(ctx, left, &leftRtrn))
            return false;

        *kc = ~leftRtrn;
        return true;

    case EXPR_UNARY_PLUS:
        left = expr->value.child;
        return ExprResolveKeyCode(ctx, left, kc);

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveKeyCode\n", expr->op);
        break;
    }

    return false;
}

/**
 * This function returns ... something.  It's a bit of a guess, really.
 *
 * If an integer is given in value ctx, it will be returned in ival.
 * If an ident or field reference is given, the lookup function (if given)
 * will be called.  At the moment, only SimpleLookup use this, and they both
 * return the results in uval.  And don't support field references.
 *
 * Cool.
 */
static bool
ExprResolveIntegerLookup(struct xkb_context *ctx, const ExprDef *expr,
                         int *val_rtrn, IdentLookupFunc lookup,
                         const void *lookupPriv)
{
    bool ok = false;
    int l, r;
    unsigned u;
    ExprDef *left, *right;

    switch (expr->op) {
    case EXPR_VALUE:
        if (expr->value_type != EXPR_TYPE_INT) {
            log_err(ctx,
                    "Found constant of type %s where an int was expected\n",
                    exprValueTypeText(expr->value_type));
            return false;
        }

        *val_rtrn = expr->value.ival;
        return true;

    case EXPR_IDENT:
        if (lookup)
            ok = lookup(ctx, lookupPriv, expr->value.str, EXPR_TYPE_INT, &u);

        if (!ok)
            log_err(ctx, "Identifier \"%s\" of type int is unknown\n",
                    xkb_atom_text(ctx, expr->value.str));
        else
            *val_rtrn = (int) u;

        return ok;

    case EXPR_FIELD_REF:
        log_err(ctx, "Default \"%s.%s\" of type int is unknown\n",
                xkb_atom_text(ctx, expr->value.field.element),
                xkb_atom_text(ctx, expr->value.field.field));
        return false;

    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_DIVIDE:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (!ExprResolveIntegerLookup(ctx, left, &l, lookup, lookupPriv) ||
            !ExprResolveIntegerLookup(ctx, right, &r, lookup, lookupPriv))
            return false;

        switch (expr->op) {
        case EXPR_ADD:
            *val_rtrn = l + r;
            break;
        case EXPR_SUBTRACT:
            *val_rtrn = l - r;
            break;
        case EXPR_MULTIPLY:
            *val_rtrn = l * r;
            break;
        case EXPR_DIVIDE:
            if (r == 0) {
                log_err(ctx, "Cannot divide by zero: %d / %d\n", l, r);
                return false;
            }
            *val_rtrn = l / r;
            break;
        default:
            break;
        }

        return true;

    case EXPR_ASSIGN:
        log_wsgo(ctx, "Assignment operator not implemented yet\n");
        break;

    case EXPR_NOT:
        log_err(ctx, "The ! operator cannot be applied to an integer\n");
        return false;

    case EXPR_INVERT:
    case EXPR_NEGATE:
        left = expr->value.child;
        if (!ExprResolveIntegerLookup(ctx, left, &l, lookup, lookupPriv))
            return false;

        *val_rtrn = (expr->op == EXPR_NEGATE ? -l : ~l);
        return true;

    case EXPR_UNARY_PLUS:
        left = expr->value.child;
        return ExprResolveIntegerLookup(ctx, left, val_rtrn, lookup,
                                        lookupPriv);

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveInteger\n", expr->op);
        break;
    }

    return false;
}

bool
ExprResolveInteger(struct xkb_context *ctx, const ExprDef *expr,
                   int *val_rtrn)
{
    return ExprResolveIntegerLookup(ctx, expr, val_rtrn, NULL, NULL);
}

bool
ExprResolveGroup(struct xkb_context *ctx, const ExprDef *expr,
                 xkb_group_index_t *group_rtrn)
{
    bool ok;
    int result;
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

    ok = ExprResolveIntegerLookup(ctx, expr, &result, SimpleLookup,
                                  group_names);
    if (!ok)
        return false;

    if (result <= 0 || result > XkbNumKbdGroups) {
        log_err(ctx, "Group index %u is out of range (1..%d)\n",
                result, XkbNumKbdGroups);
        return false;
    }

    *group_rtrn = (xkb_group_index_t) result;
    return true;
}

bool
ExprResolveLevel(struct xkb_context *ctx, const ExprDef *expr,
                 xkb_level_index_t *level_rtrn)
{
    bool ok;
    int result;
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

    ok = ExprResolveIntegerLookup(ctx, expr, &result, SimpleLookup,
                                  level_names);
    if (!ok)
        return false;

    if (result < 1 || result > XkbMaxShiftLevel) {
        log_err(ctx, "Shift level %d is out of range (1..%d)\n",
                result, XkbMaxShiftLevel);
        return false;
    }

    /* Level is zero-indexed from now on. */
    *level_rtrn = (unsigned int) (result - 1);
    return true;
}

bool
ExprResolveButton(struct xkb_context *ctx, const ExprDef *expr, int *btn_rtrn)
{
    int result;
    static const LookupEntry button_names[] = {
        { "button1", 1 },
        { "button2", 2 },
        { "button3", 3 },
        { "button4", 4 },
        { "button5", 5 },
        { "default", 0 },
        { NULL, 0 }
    };

    if (!ExprResolveIntegerLookup(ctx, expr, &result, SimpleLookup,
                                  button_names))
        return false;

    *btn_rtrn = result;
    return true;
}

bool
ExprResolveString(struct xkb_context *ctx, const ExprDef *expr,
                  const char **val_rtrn)
{
    switch (expr->op) {
    case EXPR_VALUE:
        if (expr->value_type != EXPR_TYPE_STRING) {
            log_err(ctx, "Found constant of type %s, expected a string\n",
                    exprValueTypeText(expr->value_type));
            return false;
        }

        *val_rtrn = xkb_atom_text(ctx, expr->value.str);
        return true;

    case EXPR_IDENT:
        log_err(ctx, "Identifier \"%s\" of type string not found\n",
                xkb_atom_text(ctx, expr->value.str));
        return false;

    case EXPR_FIELD_REF:
        log_err(ctx, "Default \"%s.%s\" of type string not found\n",
                xkb_atom_text(ctx, expr->value.field.element),
                xkb_atom_text(ctx, expr->value.field.field));
        return false;

    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_DIVIDE:
    case EXPR_ASSIGN:
    case EXPR_NEGATE:
    case EXPR_INVERT:
    case EXPR_NOT:
    case EXPR_UNARY_PLUS:
        log_err(ctx, "%s of strings not permitted\n", exprOpText(expr->op));
        return false;

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveString\n", expr->op);
        break;
    }
    return false;
}

bool
ExprResolveKeyName(struct xkb_context *ctx, const ExprDef *expr,
                   char name[XkbKeyNameLength])
{
    switch (expr->op) {
    case EXPR_VALUE:
        if (expr->value_type != EXPR_TYPE_KEYNAME) {
            log_err(ctx, "Found constant of type %s, expected a key name\n",
                    exprValueTypeText(expr->value_type));
            return false;
        }
        strncpy(name, expr->value.keyName, XkbKeyNameLength);
        return true;

    case EXPR_IDENT:
        log_err(ctx, "Identifier \"%s\" of type string not found\n",
                xkb_atom_text(ctx, expr->value.str));
        return false;

    case EXPR_FIELD_REF:
        log_err(ctx, "Default \"%s.%s\" of type key name not found\n",
                xkb_atom_text(ctx, expr->value.field.element),
                xkb_atom_text(ctx, expr->value.field.field));
        return false;

    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_DIVIDE:
    case EXPR_ASSIGN:
    case EXPR_NEGATE:
    case EXPR_INVERT:
    case EXPR_NOT:
    case EXPR_UNARY_PLUS:
        log_err(ctx, "%s of key name values not permitted\n",
                exprOpText(expr->op));
        return false;

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveKeyName\n", expr->op);
        break;
    }
    return false;
}

bool
ExprResolveEnum(struct xkb_context *ctx, const ExprDef *expr,
                unsigned int *val_rtrn, const LookupEntry *values)
{
    if (expr->op != EXPR_IDENT) {
        log_err(ctx, "Found a %s where an enumerated value was expected\n",
                exprOpText(expr->op));
        return false;
    }

    if (!SimpleLookup(ctx, values, expr->value.str, EXPR_TYPE_INT,
                      val_rtrn)) {
        log_err(ctx, "Illegal identifier %s; expected one of:\n",
                xkb_atom_text(ctx, expr->value.str));
        while (values && values->name)
        {
            log_err(ctx, "\t%s\n", values->name);
            values++;
        }
        return false;
    }

    return true;
}

static bool
ExprResolveMaskLookup(struct xkb_context *ctx, const ExprDef *expr,
                      unsigned int *val_rtrn, IdentLookupFunc lookup,
                      const void *lookupPriv)
{
    bool ok = 0;
    unsigned int l, r;
    int v;
    ExprDef *left, *right;
    const char *bogus = NULL;

    switch (expr->op) {
    case EXPR_VALUE:
        if (expr->value_type != EXPR_TYPE_INT) {
            log_err(ctx,
                    "Found constant of type %s where a mask was expected\n",
                    exprValueTypeText(expr->value_type));
            return false;
        }
        *val_rtrn = (unsigned int) expr->value.ival;
        return true;

    case EXPR_IDENT:
        ok = lookup(ctx, lookupPriv, expr->value.str, EXPR_TYPE_INT,
                    val_rtrn);
        if (!ok)
            log_err(ctx, "Identifier \"%s\" of type int is unknown\n",
                    xkb_atom_text(ctx, expr->value.str));
        return ok;

    case EXPR_FIELD_REF:
        log_err(ctx, "Default \"%s.%s\" of type int is unknown\n",
                xkb_atom_text(ctx, expr->value.field.element),
                xkb_atom_text(ctx, expr->value.field.field));
        return false;

    case EXPR_ARRAY_REF:
        bogus = "array reference";

    case EXPR_ACTION_DECL:
        if (bogus == NULL)
            bogus = "function use";
        log_err(ctx,
                "Unexpected %s in mask expression; Expression Ignored\n",
                bogus);
        return false;

    case EXPR_ADD:
    case EXPR_SUBTRACT:
    case EXPR_MULTIPLY:
    case EXPR_DIVIDE:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (!ExprResolveMaskLookup(ctx, left, &l, lookup, lookupPriv) ||
            !ExprResolveMaskLookup(ctx, right, &r, lookup, lookupPriv))
            return false;

        switch (expr->op) {
        case EXPR_ADD:
            *val_rtrn = l | r;
            break;
        case EXPR_SUBTRACT:
            *val_rtrn = l & (~r);
            break;
        case EXPR_MULTIPLY:
        case EXPR_DIVIDE:
            log_err(ctx, "Cannot %s masks; Illegal operation ignored\n",
                    (expr->op == EXPR_DIVIDE ? "divide" : "multiply"));
            return false;
        default:
            break;
        }

        return true;

    case EXPR_ASSIGN:
        log_wsgo(ctx, "Assignment operator not implemented yet\n");
        break;

    case EXPR_INVERT:
        left = expr->value.child;
        if (!ExprResolveIntegerLookup(ctx, left, &v, lookup, lookupPriv))
            return false;

        *val_rtrn = ~v;
        return true;

    case EXPR_UNARY_PLUS:
    case EXPR_NEGATE:
    case EXPR_NOT:
        left = expr->value.child;
        if (!ExprResolveIntegerLookup(ctx, left, &v, lookup, lookupPriv))
            log_err(ctx, "The %s operator cannot be used with a mask\n",
                    (expr->op == EXPR_NEGATE ? "-" : "!"));
        return false;

    default:
        log_wsgo(ctx, "Unknown operator %d in ResolveMask\n", expr->op);
        break;
    }

    return false;
}

bool
ExprResolveMask(struct xkb_context *ctx, const ExprDef *expr,
                unsigned int *mask_rtrn, const LookupEntry *values)
{
    return ExprResolveMaskLookup(ctx, expr, mask_rtrn, SimpleLookup, values);
}

bool
ExprResolveModMask(struct xkb_context *ctx, const ExprDef *expr,
                   xkb_mod_mask_t *mask_rtrn)
{
    return ExprResolveMaskLookup(ctx, expr, mask_rtrn, LookupModMask, NULL);
}

bool
ExprResolveVModMask(struct xkb_keymap *keymap, const ExprDef *expr,
                    xkb_mod_mask_t *mask_rtrn)
{
    return ExprResolveMaskLookup(keymap->ctx, expr, mask_rtrn, LookupVModMask,
                                 keymap);
}

bool
ExprResolveKeySym(struct xkb_context *ctx, const ExprDef *expr,
                  xkb_keysym_t *sym_rtrn)
{
    int val;

    if (expr->op == EXPR_IDENT) {
        const char *str;
        str = xkb_atom_text(ctx, expr->value.str);
        *sym_rtrn = xkb_keysym_from_name(str);
        if (*sym_rtrn != XKB_KEY_NoSymbol)
            return true;
    }

    if (!ExprResolveInteger(ctx, expr, &val))
        return false;

    if (val < 0 || val >= 10)
        return false;

    *sym_rtrn = ((xkb_keysym_t) val) + '0';
    return true;
}
