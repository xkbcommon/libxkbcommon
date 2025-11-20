/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */

#include "config.h"

#include <stdint.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcomp-priv.h"
#include "messages-codes.h"
#include "text.h"
#include "expr.h"
#include "keysym.h"
#include "xkbcomp/ast.h"
#include "utils.h"
#include "utils-numbers.h"
#include "utils-checked-arithmetic.h"

typedef bool (*IdentLookupFunc)(struct xkb_context *ctx, const void *priv,
                                xkb_atom_t field, uint32_t *val_rtrn);

bool
ExprResolveLhs(struct xkb_context *ctx, const ExprDef *expr,
               const char **elem_rtrn, const char **field_rtrn,
               ExprDef **index_rtrn)
{
    switch (expr->common.type) {
    case STMT_EXPR_IDENT:
        *elem_rtrn = NULL;
        *field_rtrn = xkb_atom_text(ctx, expr->ident.ident);
        *index_rtrn = NULL;
        return (*field_rtrn != NULL);
    case STMT_EXPR_FIELD_REF:
        *elem_rtrn = xkb_atom_text(ctx, expr->field_ref.element);
        *field_rtrn = xkb_atom_text(ctx, expr->field_ref.field);
        *index_rtrn = NULL;
        return (*elem_rtrn != NULL && *field_rtrn != NULL);
    case STMT_EXPR_ARRAY_REF:
        *elem_rtrn = xkb_atom_text(ctx, expr->array_ref.element);
        *field_rtrn = xkb_atom_text(ctx, expr->array_ref.field);
        *index_rtrn = expr->array_ref.entry;
        if (expr->array_ref.element != XKB_ATOM_NONE && *elem_rtrn == NULL)
            return false;
        if (*field_rtrn == NULL)
            return false;
        return true;
    default:
        break;
    }
    log_wsgo(ctx,
             XKB_ERROR_INVALID_XKB_SYNTAX,
             "Unexpected operator %d in ResolveLhs\n", expr->common.type);
    return false;
}

static bool
SimpleLookup(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
             uint32_t *val_rtrn)
{
    if (!priv || field == XKB_ATOM_NONE)
        return false;

    const char *str = xkb_atom_text(ctx, field);
    for (const LookupEntry *entry = priv; entry && entry->name; entry++) {
        if (istreq(str, entry->name)) {
            *val_rtrn = entry->value;
            return true;
        }
    }

    return false;
}

struct named_integer_pattern {
    const char *prefix;
    size_t prefix_length;
    uint32_t min;
    uint32_t max;
    const LookupEntry *entries;
    bool is_mask;
    enum xkb_message_code error_id;
};

/* Parse a number expressed with the pattern `<prefix><decimal number>` */
static bool
NamedIntegerPatternLookup(struct xkb_context *ctx, const void *priv,
                          xkb_atom_t field, uint32_t *val_rtrn)
{
    if (!priv || field == XKB_ATOM_NONE)
        return false;

    const char * const str = xkb_atom_text(ctx, field);
    const struct named_integer_pattern * const pattern = priv;

    const int count = (istrneq(str, pattern->prefix, pattern->prefix_length))
        ? parse_dec_to_uint32_t(str + pattern->prefix_length, SIZE_MAX, val_rtrn)
        : 0;

    if (count > 0 && *(str + pattern->prefix_length + count) == '\0') {
        if (*val_rtrn < pattern->min || *val_rtrn > pattern->max) {
            log_err(ctx, pattern->error_id,
                    "%s index %"PRIu32" is out of range (%"PRIu32"..%"PRIu32")\n",
                    pattern->prefix, *val_rtrn, pattern->min, pattern->max);
            return false;
        }
        if (pattern->is_mask) {
            /* Compute mask (bit 0 = min index) */
            assert(*val_rtrn - pattern->min < 32);
            *val_rtrn = UINT32_C(1) << (*val_rtrn - pattern->min);
        }
        return true;
    } else {
        return (
            pattern->entries &&
            SimpleLookup(ctx, pattern->entries, field, val_rtrn)
        );
    }
}

/* Data passed in the *priv argument for LookupModMask. */
typedef struct {
    const struct xkb_mod_set *mods;
    enum mod_type mod_type;
} LookupModMaskPriv;

static bool
LookupModMask(struct xkb_context *ctx, const void *priv, xkb_atom_t field,
              xkb_mod_mask_t *val_rtrn)
{
    const char *str = xkb_atom_text(ctx, field);
    if (!str)
        return false;

    if (istreq(str, "all")) {
        *val_rtrn  = MOD_REAL_MASK_ALL;
        return true;
    }

    if (istreq(str, "none")) {
        *val_rtrn = 0;
        return true;
    }

    const LookupModMaskPriv *arg = priv;
    const struct xkb_mod_set *mods = arg->mods;
    const enum mod_type mod_type = arg->mod_type;
    const xkb_mod_index_t ndx = XkbModNameToIndex(mods, field, mod_type);
    if (ndx == XKB_MOD_INVALID)
        return false;

    *val_rtrn = (UINT32_C(1) << ndx);
    return true;
}

bool
ExprResolveBoolean(struct xkb_context *ctx, const ExprDef *expr,
                   bool *set_rtrn)
{
    bool ok = false;
    const char *ident;

    switch (expr->common.type) {
    case STMT_EXPR_BOOLEAN_LITERAL:
        *set_rtrn = expr->boolean.set;
        return true;

    case STMT_EXPR_STRING_LITERAL:
    case STMT_EXPR_INTEGER_LITERAL:
    case STMT_EXPR_FLOAT_LITERAL:
    case STMT_EXPR_KEYNAME_LITERAL:
    case STMT_EXPR_KEYSYM_LITERAL:
        log_err(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                "Found %s where boolean was expected\n",
                stmt_type_to_string(expr->common.type));
        return false;

    case STMT_EXPR_IDENT:
        ident = xkb_atom_text(ctx, expr->ident.ident);
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
        log_err(ctx, XKB_ERROR_INVALID_IDENTIFIER,
                "Identifier \"%s\" of type boolean is unknown\n", ident);
        return false;

    case STMT_EXPR_FIELD_REF:
        log_err(ctx, XKB_ERROR_INVALID_EXPRESSION_TYPE,
                "Default \"%s.%s\" of type boolean is unknown\n",
                xkb_atom_text(ctx, expr->field_ref.element),
                xkb_atom_text(ctx, expr->field_ref.field));
        return false;

    case STMT_EXPR_INVERT:
    case STMT_EXPR_NOT:
        ok = ExprResolveBoolean(ctx, expr->unary.child, set_rtrn);
        if (ok)
            *set_rtrn = !*set_rtrn;
        return ok;
    case STMT_EXPR_ADD:
    case STMT_EXPR_SUBTRACT:
    case STMT_EXPR_MULTIPLY:
    case STMT_EXPR_DIVIDE:
    case STMT_EXPR_ASSIGN:
    case STMT_EXPR_NEGATE:
    case STMT_EXPR_UNARY_PLUS:
    case STMT_EXPR_EMPTY_LIST:
    case STMT_EXPR_ACTION_DECL:
    case STMT_EXPR_ACTION_LIST:
    case STMT_EXPR_KEYSYM_LIST:
        log_err(ctx, XKB_ERROR_INVALID_OPERATION,
                "%s of boolean values not permitted\n",
                stmt_type_to_string(expr->common.type));
        break;

    default:
        log_wsgo(ctx, XKB_ERROR_UNKNOWN_OPERATOR,
                 "Unknown operator %d in ResolveBoolean\n",
                 expr->common.type);
        break;
    }

    return false;
}

static bool
ExprResolveIntegerLookup(struct xkb_context *ctx, const ExprDef *expr,
                         int64_t *val_rtrn, IdentLookupFunc lookup,
                         const void *lookupPriv)
{
    bool ok = false;
    int64_t l = 0, r = 0;
    uint32_t u = 0;
    ExprDef *left, *right;

    switch (expr->common.type) {
    case STMT_EXPR_INTEGER_LITERAL:
        *val_rtrn = expr->integer.ival;
        return true;

    case STMT_EXPR_STRING_LITERAL:
    case STMT_EXPR_FLOAT_LITERAL:
    case STMT_EXPR_BOOLEAN_LITERAL:
    case STMT_EXPR_KEYNAME_LITERAL:
    case STMT_EXPR_KEYSYM_LITERAL:
        log_err(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                "Found %s where an int was expected\n",
                stmt_type_to_string(expr->common.type));
        return false;

    case STMT_EXPR_IDENT:
        if (lookup)
            ok = lookup(ctx, lookupPriv, expr->ident.ident, &u);

        if (!ok)
            log_err(ctx, XKB_ERROR_INVALID_IDENTIFIER,
                    "Identifier \"%s\" of type int is unknown\n",
                    xkb_atom_text(ctx, expr->ident.ident));
        else
            *val_rtrn = (int64_t) u;

        return ok;

    case STMT_EXPR_FIELD_REF:
        log_err(ctx, XKB_ERROR_INVALID_EXPRESSION_TYPE,
                "Default \"%s.%s\" of type int is unknown\n",
                xkb_atom_text(ctx, expr->field_ref.element),
                xkb_atom_text(ctx, expr->field_ref.field));
        return false;

    case STMT_EXPR_ADD:
    case STMT_EXPR_SUBTRACT:
    case STMT_EXPR_MULTIPLY:
    case STMT_EXPR_DIVIDE:
        left = expr->binary.left;
        right = expr->binary.right;
        if (!ExprResolveIntegerLookup(ctx, left, &l, lookup, lookupPriv) ||
            !ExprResolveIntegerLookup(ctx, right, &r, lookup, lookupPriv))
            return false;

        switch (expr->common.type) {
        case STMT_EXPR_ADD:
            if (ckd_add(val_rtrn, l, r)) {
                log_err(ctx, XKB_ERROR_INTEGER_OVERFLOW,
                        "Addition %"PRId64" + %"PRId64" has an invalid "
                        "mathematical result: %"PRId64"\n",
                        l, r, *val_rtrn);
                return false;
            }
            break;
        case STMT_EXPR_SUBTRACT:
            if (ckd_sub(val_rtrn, l, r)) {
                log_err(ctx, XKB_ERROR_INTEGER_OVERFLOW,
                        "Substraction %"PRId64" - %"PRId64" has an invalid "
                        "mathematical result: %"PRId64"\n",
                        l, r, *val_rtrn);
                return false;
            }
            break;
        case STMT_EXPR_MULTIPLY:
            if (ckd_mul(val_rtrn, l, r)) {
                log_err(ctx, XKB_ERROR_INTEGER_OVERFLOW,
                        "Multiplication %"PRId64" * %"PRId64" has an invalid "
                        "mathematical result: %"PRId64"\n",
                        l, r, *val_rtrn);
                return false;
            }
            break;
        case STMT_EXPR_DIVIDE:
            if (r == 0) {
                log_err(ctx, XKB_ERROR_INVALID_OPERATION,
                        "Cannot divide by zero: %"PRId64" / %"PRId64"\n", l, r);
                return false;
            }
            *val_rtrn = l / r;
            break;
        default:
            log_err(ctx, XKB_ERROR_INVALID_OPERATION,
                    "%s of integers not permitted\n",
                    stmt_type_to_string(expr->common.type));
            return false;
        }

        return true;

    case STMT_EXPR_ASSIGN:
        log_wsgo(ctx,
                 XKB_ERROR_INVALID_OPERATION,
                 "Assignment operator not implemented yet\n");
        break;

    case STMT_EXPR_NOT:
        log_err(ctx, XKB_ERROR_INVALID_OPERATION,
                "The ! operator cannot be applied to an integer\n");
        return false;

    case STMT_EXPR_INVERT:
    case STMT_EXPR_NEGATE:
        left = expr->unary.child;
        if (!ExprResolveIntegerLookup(ctx, left, &l, lookup, lookupPriv))
            return false;

        *val_rtrn = (expr->common.type == STMT_EXPR_NEGATE ? -l : ~l);
        return true;

    case STMT_EXPR_UNARY_PLUS:
        left = expr->unary.child;
        return ExprResolveIntegerLookup(ctx, left, val_rtrn, lookup,
                                        lookupPriv);

    default:
        log_wsgo(ctx, XKB_ERROR_UNKNOWN_OPERATOR,
                 "Unknown operator %d in ResolveInteger\n",
                 expr->common.type);
        break;
    }

    return false;
}

bool
ExprResolveInteger(struct xkb_context *ctx, const ExprDef *expr,
                   int64_t *val_rtrn)
{
    return ExprResolveIntegerLookup(ctx, expr, val_rtrn, NULL, NULL);
}

bool
ExprResolveGroup(const struct xkb_keymap_info *keymap_info,
                 const ExprDef *expr, xkb_layout_index_t *group_rtrn)
{
    const struct named_integer_pattern group_name_pattern = {
        /* Prefix is title-cased, because it is also used in error messages */
        .prefix = "Group",
        .prefix_length = sizeof("Group") - 1,
        .min = 1,
        .max = keymap_info->features.max_groups,
        .is_mask = false,
        .entries = keymap_info->groupIndicesNames,
        .error_id = XKB_ERROR_UNSUPPORTED_GROUP_INDEX,
    };
    int64_t result = 0;
    if (!ExprResolveIntegerLookup(keymap_info->keymap.ctx, expr, &result,
                                  NamedIntegerPatternLookup, &group_name_pattern))
        return false;

    if (result < 1 || result > keymap_info->features.max_groups) {
        log_err(keymap_info->keymap.ctx, XKB_ERROR_UNSUPPORTED_GROUP_INDEX,
                "Group index %"PRId64" is out of range (1..%"PRIu32")\n",
                result, keymap_info->features.max_groups);
        return false;
    }

    *group_rtrn = (xkb_layout_index_t) result;
    return true;
}

bool
ExprResolveLevel(struct xkb_context *ctx, const ExprDef *expr,
                 xkb_level_index_t *level_rtrn)
{
    static const struct named_integer_pattern level_name_pattern = {
        /* Prefix is title-cased, because it is also used in error messages */
        .prefix = "Level",
        .prefix_length = sizeof("Level") - 1,
        .min = 1,
        .max = XKB_LEVEL_MAX_IMPL,
        .is_mask = false,
        .entries = NULL,
        .error_id = XKB_ERROR_UNSUPPORTED_SHIFT_LEVEL,
    };
    int64_t result = 0;
    if (!ExprResolveIntegerLookup(ctx, expr, &result, NamedIntegerPatternLookup,
                                  &level_name_pattern))
        return false;

    if (result < 1 || result > XKB_LEVEL_MAX_IMPL) {
        log_err(ctx, XKB_ERROR_UNSUPPORTED_SHIFT_LEVEL,
                "Shift level %"PRId64" is out of range (1..%u)\n",
                result, XKB_LEVEL_MAX_IMPL);
        return false;
    }

    /* Level is zero-indexed from now on. */
    *level_rtrn = (xkb_level_index_t) (result - 1);
    return true;
}

bool
ExprResolveButton(struct xkb_context *ctx, const ExprDef *expr, int64_t *btn_rtrn)
{
    return ExprResolveIntegerLookup(ctx, expr, btn_rtrn, SimpleLookup,
                                    buttonNames);
}

bool
ExprResolveString(struct xkb_context *ctx, const ExprDef *expr,
                  xkb_atom_t *val_rtrn)
{
    switch (expr->common.type) {
    case STMT_EXPR_STRING_LITERAL:
        *val_rtrn = expr->string.str;
        return true;

    case STMT_EXPR_INTEGER_LITERAL:
    case STMT_EXPR_FLOAT_LITERAL:
    case STMT_EXPR_BOOLEAN_LITERAL:
    case STMT_EXPR_KEYNAME_LITERAL:
    case STMT_EXPR_KEYSYM_LITERAL:
        log_err(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                "Found %s, expected a string\n",
                stmt_type_to_string(expr->common.type));
        return false;

    case STMT_EXPR_IDENT:
        log_err(ctx, XKB_ERROR_INVALID_IDENTIFIER,
                "Identifier \"%s\" of type string not found\n",
                xkb_atom_text(ctx, expr->ident.ident));
        return false;

    case STMT_EXPR_FIELD_REF:
        log_err(ctx, XKB_ERROR_INVALID_EXPRESSION_TYPE,
                "Default \"%s.%s\" of type string not found\n",
                xkb_atom_text(ctx, expr->field_ref.element),
                xkb_atom_text(ctx, expr->field_ref.field));
        return false;

    case STMT_EXPR_ADD:
    case STMT_EXPR_SUBTRACT:
    case STMT_EXPR_MULTIPLY:
    case STMT_EXPR_DIVIDE:
    case STMT_EXPR_ASSIGN:
    case STMT_EXPR_NEGATE:
    case STMT_EXPR_INVERT:
    case STMT_EXPR_NOT:
    case STMT_EXPR_UNARY_PLUS:
    case STMT_EXPR_EMPTY_LIST:
    case STMT_EXPR_ACTION_DECL:
    case STMT_EXPR_ACTION_LIST:
    case STMT_EXPR_KEYSYM_LIST:
        log_err(ctx, XKB_ERROR_INVALID_XKB_SYNTAX,
                "%s of strings not permitted\n",
                stmt_type_to_string(expr->common.type));
        return false;

    default:
        log_wsgo(ctx, XKB_ERROR_UNKNOWN_OPERATOR,
                 "Unknown operator %d in ResolveString\n",
                 expr->common.type);
        break;
    }
    return false;
}

bool
ExprResolveEnum(struct xkb_context *ctx, const ExprDef *expr,
                uint32_t *val_rtrn, const LookupEntry *values)
{
    if (expr->common.type != STMT_EXPR_IDENT) {
        log_err(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                "Found a %s where an enumerated value was expected\n",
                stmt_type_to_string(expr->common.type));
        return false;
    }

    if (!SimpleLookup(ctx, values, expr->ident.ident, val_rtrn)) {
        log_err(ctx, XKB_ERROR_INVALID_IDENTIFIER,
                "Illegal identifier %s; expected one of:\n",
                xkb_atom_text(ctx, expr->ident.ident));
        while (values && values->name)
        {
            log_err(ctx, XKB_ERROR_INVALID_IDENTIFIER, "\t%s\n", values->name);
            values++;
        }
        return false;
    }

    return true;
}

static bool
ExprResolveMaskLookup(struct xkb_context *ctx, const ExprDef *expr,
                      uint32_t *val_rtrn, IdentLookupFunc lookup,
                      const void *lookupPriv)
{
    bool ok = false;
    uint32_t l = 0, r = 0;
    int64_t v = 0;
    ExprDef *left, *right;
    const char *bogus = NULL;

    switch (expr->common.type) {
    case STMT_EXPR_INTEGER_LITERAL:
        if (expr->integer.ival < 0 || expr->integer.ival > UINT32_MAX) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Mask %s%#"PRIxMAX" is out of range (0..%#"PRIx32")\n",
                    expr->integer.ival < 0 ? "-" : "",
                    imaxabs(expr->integer.ival),
                    UINT32_MAX);
            return false;
        }
        *val_rtrn = (uint32_t) expr->integer.ival;
        return true;

    case STMT_EXPR_STRING_LITERAL:
    case STMT_EXPR_FLOAT_LITERAL:
    case STMT_EXPR_BOOLEAN_LITERAL:
    case STMT_EXPR_KEYNAME_LITERAL:
    case STMT_EXPR_KEYSYM_LITERAL:
        log_err(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                "Found %s where a mask was expected\n",
                stmt_type_to_string(expr->common.type));
        return false;

    case STMT_EXPR_IDENT:
        ok = lookup(ctx, lookupPriv, expr->ident.ident, val_rtrn);
        if (!ok)
            log_err(ctx, XKB_ERROR_INVALID_IDENTIFIER,
                    "Identifier \"%s\" of type int is unknown\n",
                    xkb_atom_text(ctx, expr->ident.ident));
        return ok;

    case STMT_EXPR_FIELD_REF:
        log_err(ctx, XKB_ERROR_INVALID_EXPRESSION_TYPE,
                "Default \"%s.%s\" of type int is unknown\n",
                xkb_atom_text(ctx, expr->field_ref.element),
                xkb_atom_text(ctx, expr->field_ref.field));
        return false;

    case STMT_EXPR_ARRAY_REF:
        bogus = "array reference";
        /* fallthrough */
    case STMT_EXPR_ACTION_DECL:
        if (bogus == NULL)
            bogus = "function use";
        log_err(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                "Unexpected %s in mask expression; Expression Ignored\n",
                bogus);
        return false;

    case STMT_EXPR_ADD:
    case STMT_EXPR_SUBTRACT:
    case STMT_EXPR_MULTIPLY:
    case STMT_EXPR_DIVIDE:
        left = expr->binary.left;
        right = expr->binary.right;
        if (!ExprResolveMaskLookup(ctx, left, &l, lookup, lookupPriv) ||
            !ExprResolveMaskLookup(ctx, right, &r, lookup, lookupPriv))
            return false;

        switch (expr->common.type) {
        case STMT_EXPR_ADD:
            *val_rtrn = l | r;
            break;
        case STMT_EXPR_SUBTRACT:
            *val_rtrn = l & (~r);
            break;
        case STMT_EXPR_MULTIPLY:
        case STMT_EXPR_DIVIDE:
            log_err(ctx, XKB_ERROR_INVALID_OPERATION,
                    "Cannot %s masks; Illegal operation ignored\n",
                    (expr->common.type == STMT_EXPR_DIVIDE ? "divide" : "multiply"));
            return false;
        default:
            break;
        }

        return true;

    case STMT_EXPR_ASSIGN:
        log_wsgo(ctx, XKB_ERROR_INVALID_OPERATION,
                 "Assignment operator not implemented yet\n");
        break;

    case STMT_EXPR_INVERT:
        left = expr->unary.child;
        if (!ExprResolveIntegerLookup(ctx, left, &v, lookup, lookupPriv))
            return false;
        if (v < 0 || v > UINT32_MAX) {
            log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Mask %s%#"PRIxMAX" is out of range (0..%#"PRIx32")\n",
                    v < 0 ? "-" : "", imaxabs(v), UINT32_MAX);
            return false;
        }
        *val_rtrn = ~(uint32_t) v;
        return true;

    case STMT_EXPR_UNARY_PLUS:
    case STMT_EXPR_NEGATE:
    case STMT_EXPR_NOT:
        left = expr->unary.child;
        if (!ExprResolveIntegerLookup(ctx, left, &v, lookup, lookupPriv))
            return false;
        log_err(ctx, XKB_ERROR_INVALID_OPERATION,
                "The '%c' unary operator cannot be used with a mask\n",
                stmt_type_to_operator_char(expr->common.type));
        return false;

    default:
        log_wsgo(ctx, XKB_ERROR_UNKNOWN_OPERATOR,
                 "Unknown operator type %d in ResolveMask\n",
                 expr->common.type);
        break;
    }

    return false;
}

bool
ExprResolveMask(struct xkb_context *ctx, const ExprDef *expr,
                uint32_t *mask_rtrn, const LookupEntry *values)
{
    return ExprResolveMaskLookup(ctx, expr, mask_rtrn, SimpleLookup, values);
}

bool
ExprResolveModMask(struct xkb_context *ctx, const ExprDef *expr,
                   enum mod_type mod_type, const struct xkb_mod_set *mods,
                   xkb_mod_mask_t *mask_rtrn)
{
    LookupModMaskPriv priv = { .mods = mods, .mod_type = mod_type };
    return ExprResolveMaskLookup(ctx, expr, mask_rtrn, LookupModMask, &priv);
}

bool
ExprResolveMod(struct xkb_context *ctx, const ExprDef *def,
               enum mod_type mod_type, const struct xkb_mod_set *mods,
               xkb_mod_index_t *ndx_rtrn)
{
    if (def->common.type != STMT_EXPR_IDENT) {
        log_err(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                "Cannot resolve virtual modifier: "
                "found %s where a virtual modifier name was expected\n",
                stmt_type_to_string(def->common.type));
        return false;
    }

    xkb_atom_t name = def->ident.ident;
    xkb_mod_index_t ndx = XkbModNameToIndex(mods, name, mod_type);
    if (ndx == XKB_MOD_INVALID) {
        log_err(ctx, XKB_ERROR_UNDECLARED_VIRTUAL_MODIFIER,
                "Cannot resolve virtual modifier: "
                "\"%s\" was not previously declared\n",
                xkb_atom_text(ctx, name));
        return false;
    }

    *ndx_rtrn = ndx;
    return true;
}

bool
ExprResolveGroupMask(const struct xkb_keymap_info *keymap_info,
                     const ExprDef *expr, xkb_layout_index_t *group_rtrn)
{
    const struct named_integer_pattern group_name_pattern = {
        /* Prefix is title-cased, because it is also used in error messages */
        .prefix = "Group",
        .prefix_length = sizeof("Group") - 1,
        .min = 1,
        .max = keymap_info->features.max_groups,
        .is_mask = true,
        .entries = keymap_info->groupMaskNames,
        .error_id = XKB_ERROR_UNSUPPORTED_GROUP_INDEX
    };
    return ExprResolveMaskLookup(keymap_info->keymap.ctx, expr, group_rtrn,
                                 NamedIntegerPatternLookup, &group_name_pattern);
}
