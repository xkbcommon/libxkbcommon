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
#include "expr.h"
#include "vmod.h"

#include <ctype.h>

/***====================================================================***/

typedef Bool(*IdentLookupFunc) (const void * /* priv */ ,
                                xkb_atom_t /* field */ ,
                                unsigned /* type */ ,
                                ExprResult *    /* val_rtrn */
    );

/***====================================================================***/

const char *
exprOpText(unsigned type)
{
    static char buf[32];

    switch (type)
    {
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
        strcpy(buf, "plus sign");
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

    switch (type)
    {
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
ExprResolveLhs(ExprDef * expr,
               ExprResult * elem_rtrn,
               ExprResult * field_rtrn, ExprDef ** index_rtrn)
{
    switch (expr->op)
    {
    case ExprIdent:
        elem_rtrn->str = NULL;
        field_rtrn->str = XkbcAtomGetString(expr->value.str);
        *index_rtrn = NULL;
        return True;
    case ExprFieldRef:
        elem_rtrn->str = XkbcAtomGetString(expr->value.field.element);
        field_rtrn->str = XkbcAtomGetString(expr->value.field.field);
        *index_rtrn = NULL;
        return True;
    case ExprArrayRef:
        elem_rtrn->str = XkbcAtomGetString(expr->value.array.element);
        field_rtrn->str = XkbcAtomGetString(expr->value.array.field);
        *index_rtrn = expr->value.array.entry;
        return True;
    }
    WSGO("Unexpected operator %d in ResolveLhs\n", expr->op);
    return False;
}

static Bool
SimpleLookup(const void * priv, xkb_atom_t field, unsigned type,
             ExprResult * val_rtrn)
{
    const LookupEntry *entry;
    const char *str;

    if ((priv == NULL) || (field == XKB_ATOM_NONE) || (type != TypeInt))
    {
        return False;
    }
    str = XkbcAtomText(field);
    for (entry = priv; (entry != NULL) && (entry->name != NULL); entry++)
    {
        if (strcasecmp(str, entry->name) == 0)
        {
            val_rtrn->uval = entry->result;
            return True;
        }
    }
    return False;
}

static const LookupEntry modIndexNames[] = {
    {"shift", ShiftMapIndex},
    {"control", ControlMapIndex},
    {"lock", LockMapIndex},
    {"mod1", Mod1MapIndex},
    {"mod2", Mod2MapIndex},
    {"mod3", Mod3MapIndex},
    {"mod4", Mod4MapIndex},
    {"mod5", Mod5MapIndex},
    {"none", XkbNoModifier},
    {NULL, 0}
};

int
LookupModIndex(const void * priv, xkb_atom_t field, unsigned type,
               ExprResult * val_rtrn)
{
    return SimpleLookup(modIndexNames, field, type, val_rtrn);
}

int
LookupModMask(const void * priv, xkb_atom_t field, unsigned type,
              ExprResult * val_rtrn)
{
    const char *str;
    Bool ret = True;

    if (type != TypeInt)
        return False;
    str = XkbcAtomText(field);
    if (str == NULL)
        return False;
    if (strcasecmp(str, "all") == 0)
        val_rtrn->uval = 0xff;
    else if (strcasecmp(str, "none") == 0)
        val_rtrn->uval = 0;
    else if (LookupModIndex(priv, field, type, val_rtrn))
        val_rtrn->uval = (1 << val_rtrn->uval);
    else
        ret = False;
    return ret;
}

int
ExprResolveBoolean(ExprDef * expr,
                   ExprResult * val_rtrn)
{
    int ok = 0;
    const char *bogus = NULL;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type != TypeBoolean)
        {
            ERROR
                ("Found constant of type %s where boolean was expected\n",
                 exprTypeText(expr->type));
            return False;
        }
        val_rtrn->ival = expr->value.ival;
        return True;
    case ExprIdent:
        bogus = XkbcAtomText(expr->value.str);
        if (bogus)
        {
            if ((strcasecmp(bogus, "true") == 0) ||
                (strcasecmp(bogus, "yes") == 0) ||
                (strcasecmp(bogus, "on") == 0))
            {
                val_rtrn->uval = 1;
                return True;
            }
            else if ((strcasecmp(bogus, "false") == 0) ||
                     (strcasecmp(bogus, "no") == 0) ||
                     (strcasecmp(bogus, "off") == 0))
            {
                val_rtrn->uval = 0;
                return True;
            }
        }
        ERROR("Identifier \"%s\" of type int is unknown\n",
              XkbcAtomText(expr->value.str));
        return False;
    case ExprFieldRef:
        ERROR("Default \"%s.%s\" of type boolean is unknown\n",
              XkbcAtomText(expr->value.field.element),
              XkbcAtomText(expr->value.field.field));
        return False;
    case OpInvert:
    case OpNot:
        ok = ExprResolveBoolean(expr, val_rtrn);
        if (ok)
            val_rtrn->uval = !val_rtrn->uval;
        return ok;
    case OpAdd:
        if (bogus == NULL)
            bogus = "Addition";
    case OpSubtract:
        if (bogus == NULL)
            bogus = "Subtraction";
    case OpMultiply:
        if (bogus == NULL)
            bogus = "Multiplication";
    case OpDivide:
        if (bogus == NULL)
            bogus = "Division";
    case OpAssign:
        if (bogus == NULL)
            bogus = "Assignment";
    case OpNegate:
        if (bogus == NULL)
            bogus = "Negation";
        ERROR("%s of boolean values not permitted\n", bogus);
        break;
    case OpUnaryPlus:
        ERROR("Unary \"+\" operator not permitted for boolean values\n");
        break;
    default:
        WSGO("Unknown operator %d in ResolveBoolean\n", expr->op);
        break;
    }
    return False;
}

int
ExprResolveFloat(ExprDef * expr,
                 ExprResult * val_rtrn)
{
    int ok = 0;
    ExprResult leftRtrn, rightRtrn;
    ExprDef *left, *right;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type == TypeString)
        {
            const char *str;
            str = XkbcAtomText(expr->value.str);
            if ((str != NULL) && (strlen(str) == 1))
            {
                val_rtrn->uval = str[0] * XkbGeomPtsPerMM;
                return True;
            }
        }
        if (expr->type != TypeInt)
        {
            ERROR("Found constant of type %s, expected a number\n",
                   exprTypeText(expr->type));
            return False;
        }
        val_rtrn->ival = expr->value.ival;
        if (expr->type == TypeInt)
            val_rtrn->ival *= XkbGeomPtsPerMM;
        return True;
    case ExprIdent:
        ERROR("Numeric identifier \"%s\" unknown\n",
              XkbcAtomText(expr->value.str));
        return ok;
    case ExprFieldRef:
        ERROR("Numeric default \"%s.%s\" unknown\n",
              XkbcAtomText(expr->value.field.element),
              XkbcAtomText(expr->value.field.field));
        return False;
    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (ExprResolveFloat(left, &leftRtrn) &&
            ExprResolveFloat(right, &rightRtrn))
        {
            switch (expr->op)
            {
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
                val_rtrn->ival = leftRtrn.ival / rightRtrn.ival;
                break;
            }
            return True;
        }
        return False;
    case OpAssign:
        WSGO("Assignment operator not implemented yet\n");
        break;
    case OpNot:
        ERROR("The ! operator cannot be applied to a number\n");
        return False;
    case OpInvert:
    case OpNegate:
        left = expr->value.child;
        if (ExprResolveFloat(left, &leftRtrn))
        {
            if (expr->op == OpNegate)
                val_rtrn->ival = -leftRtrn.ival;
            else
                val_rtrn->ival = ~leftRtrn.ival;
            return True;
        }
        return False;
    case OpUnaryPlus:
        left = expr->value.child;
        return ExprResolveFloat(left, val_rtrn);
    default:
        WSGO("Unknown operator %d in ResolveFloat\n", expr->op);
        break;
    }
    return False;
}

int
ExprResolveKeyCode(ExprDef * expr,
                   ExprResult * val_rtrn)
{
    ExprResult leftRtrn, rightRtrn;
    ExprDef *left, *right;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type != TypeInt)
        {
            ERROR
                ("Found constant of type %s where an int was expected\n",
                 exprTypeText(expr->type));
            return False;
        }
        val_rtrn->uval = expr->value.uval;
        return True;
    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (ExprResolveKeyCode(left, &leftRtrn) &&
            ExprResolveKeyCode(right, &rightRtrn))
        {
            switch (expr->op)
            {
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
                val_rtrn->uval = leftRtrn.uval / rightRtrn.uval;
                break;
            }
            return True;
        }
        return False;
    case OpNegate:
        left = expr->value.child;
        if (ExprResolveKeyCode(left, &leftRtrn))
        {
            val_rtrn->uval = ~leftRtrn.uval;
            return True;
        }
        return False;
    case OpUnaryPlus:
        left = expr->value.child;
        return ExprResolveKeyCode(left, val_rtrn);
    default:
        WSGO("Unknown operator %d in ResolveKeyCode\n", expr->op);
        break;
    }
    return False;
}

/**
 * This function returns ... something.  It's a bit of a guess, really.
 *
 * If a string is given in value context, its first character will be
 * returned in uval.  If an integer is given in value context, it will be
 * returned in ival.  If a float is given in value context, it will be
 * returned as millimetres (rather than points) in ival.
 *
 * If an ident or field reference is given, the lookup function (if given)
 * will be called.  At the moment, only SimpleLookup use this, and they both
 * return the results in uval.  And don't support field references.
 *
 * Cool.
 */
static int
ExprResolveIntegerLookup(ExprDef * expr,
                         ExprResult * val_rtrn,
                         IdentLookupFunc lookup, const void * lookupPriv)
{
    int ok = 0;
    ExprResult leftRtrn, rightRtrn;
    ExprDef *left, *right;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type == TypeString)
        {
            const char *str;
            str = XkbcAtomText(expr->value.str);
            if (str != NULL)
                switch (strlen(str))
                {
                case 0:
                    val_rtrn->uval = 0;
                    return True;
                case 1:
                    val_rtrn->uval = str[0];
                    return True;
                default:
                    break;
                }
        }
        if (expr->type != TypeInt)
        {
            ERROR
                ("Found constant of type %s where an int was expected\n",
                 exprTypeText(expr->type));
            return False;
        }
        val_rtrn->ival = expr->value.ival;
        return True;
    case ExprIdent:
        if (lookup)
        {
            ok = (*lookup) (lookupPriv, expr->value.str, TypeInt, val_rtrn);
        }
        if (!ok)
            ERROR("Identifier \"%s\" of type int is unknown\n",
                   XkbcAtomText(expr->value.str));
        return ok;
    case ExprFieldRef:
        ERROR("Default \"%s.%s\" of type int is unknown\n",
              XkbcAtomText(expr->value.field.element),
              XkbcAtomText(expr->value.field.field));
        return False;
    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (ExprResolveIntegerLookup(left, &leftRtrn, lookup, lookupPriv) &&
            ExprResolveIntegerLookup(right, &rightRtrn, lookup, lookupPriv))
        {
            switch (expr->op)
            {
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
                val_rtrn->ival = leftRtrn.ival / rightRtrn.ival;
                break;
            }
            return True;
        }
        return False;
    case OpAssign:
        WSGO("Assignment operator not implemented yet\n");
        break;
    case OpNot:
        ERROR("The ! operator cannot be applied to an integer\n");
        return False;
    case OpInvert:
    case OpNegate:
        left = expr->value.child;
        if (ExprResolveIntegerLookup(left, &leftRtrn, lookup, lookupPriv))
        {
            if (expr->op == OpNegate)
                val_rtrn->ival = -leftRtrn.ival;
            else
                val_rtrn->ival = ~leftRtrn.ival;
            return True;
        }
        return False;
    case OpUnaryPlus:
        left = expr->value.child;
        return ExprResolveIntegerLookup(left, val_rtrn, lookup, lookupPriv);
    default:
        WSGO("Unknown operator %d in ResolveInteger\n", expr->op);
        break;
    }
    return False;
}

int
ExprResolveInteger(ExprDef * expr,
                   ExprResult * val_rtrn)
{
    return ExprResolveIntegerLookup(expr, val_rtrn, NULL, NULL);
}

int
ExprResolveGroup(ExprDef * expr,
                 ExprResult * val_rtrn)
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

    ret = ExprResolveIntegerLookup(expr, val_rtrn, SimpleLookup, group_names);
    if (ret == False)
        return ret;

    if (val_rtrn->uval == 0 || val_rtrn->uval > XkbNumKbdGroups) {
        ERROR("Group index %d is out of range (1..%d)\n",
              val_rtrn->uval, XkbNumKbdGroups);
        return False;
    }

    return True;
}

int
ExprResolveLevel(ExprDef * expr,
                 ExprResult * val_rtrn)
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

    ret = ExprResolveIntegerLookup(expr, val_rtrn, SimpleLookup, level_names);
    if (ret == False)
        return ret;

    if (val_rtrn->ival < 1 || val_rtrn->ival > XkbMaxShiftLevel) {
        ERROR("Shift level %d is out of range (1..%d)\n", val_rtrn->ival,
              XkbMaxShiftLevel);
        return False;
    }

    return True;
}

int
ExprResolveButton(ExprDef * expr,
                  ExprResult * val_rtrn)
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

    return ExprResolveIntegerLookup(expr, val_rtrn, SimpleLookup,
                                    button_names);
}

int
ExprResolveString(ExprDef * expr,
                  ExprResult * val_rtrn)
{
    ExprResult leftRtrn, rightRtrn;
    ExprDef *left;
    ExprDef *right;
    const char *bogus = NULL;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type != TypeString)
        {
            ERROR("Found constant of type %s, expected a string\n",
                   exprTypeText(expr->type));
            return False;
        }
        val_rtrn->str = XkbcAtomGetString(expr->value.str);
        if (val_rtrn->str == NULL)
            val_rtrn->str = strdup("");
        return True;
    case ExprIdent:
        ERROR("Identifier \"%s\" of type string not found\n",
              XkbcAtomText(expr->value.str));
        return False;
    case ExprFieldRef:
        ERROR("Default \"%s.%s\" of type string not found\n",
              XkbcAtomText(expr->value.field.element),
              XkbcAtomText(expr->value.field.field));
        return False;
    case OpAdd:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (ExprResolveString(left, &leftRtrn) &&
            ExprResolveString(right, &rightRtrn))
        {
            int len;
            char *new;
            len = strlen(leftRtrn.str) + strlen(rightRtrn.str) + 1;
            new = malloc(len);
            if (new)
            { sprintf(new, "%s%s", leftRtrn.str, rightRtrn.str);
                free(leftRtrn.str);
                free(rightRtrn.str);
                val_rtrn->str = new;
                return True;
            }
            free(leftRtrn.str);
            free(rightRtrn.str);
        }
        return False;
    case OpSubtract:
        if (bogus == NULL)
            bogus = "Subtraction";
    case OpMultiply:
        if (bogus == NULL)
            bogus = "Multiplication";
    case OpDivide:
        if (bogus == NULL)
            bogus = "Division";
    case OpAssign:
        if (bogus == NULL)
            bogus = "Assignment";
    case OpNegate:
        if (bogus == NULL)
            bogus = "Negation";
    case OpInvert:
        if (bogus == NULL)
            bogus = "Bitwise complement";
        ERROR("%s of string values not permitted\n", bogus);
        return False;
    case OpNot:
        ERROR("The ! operator cannot be applied to a string\n");
        return False;
    case OpUnaryPlus:
        ERROR("The + operator cannot be applied to a string\n");
        return False;
    default:
        WSGO("Unknown operator %d in ResolveString\n", expr->op);
        break;
    }
    return False;
}

int
ExprResolveKeyName(ExprDef * expr,
                   ExprResult * val_rtrn)
{
    const char *bogus = NULL;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type != TypeKeyName)
        {
            ERROR("Found constant of type %s, expected a key name\n",
                   exprTypeText(expr->type));
            return False;
        }
        memcpy(val_rtrn->keyName.name, expr->value.keyName, XkbKeyNameLength);
        return True;
    case ExprIdent:
        ERROR("Identifier \"%s\" of type string not found\n",
              XkbcAtomText(expr->value.str));
        return False;
    case ExprFieldRef:
        ERROR("Default \"%s.%s\" of type key name not found\n",
              XkbcAtomText(expr->value.field.element),
              XkbcAtomText(expr->value.field.field));
        return False;
    case OpAdd:
        if (bogus == NULL)
            bogus = "Addition";
    case OpSubtract:
        if (bogus == NULL)
            bogus = "Subtraction";
    case OpMultiply:
        if (bogus == NULL)
            bogus = "Multiplication";
    case OpDivide:
        if (bogus == NULL)
            bogus = "Division";
    case OpAssign:
        if (bogus == NULL)
            bogus = "Assignment";
    case OpNegate:
        if (bogus == NULL)
            bogus = "Negation";
    case OpInvert:
        if (bogus == NULL)
            bogus = "Bitwise complement";
        ERROR("%s of key name values not permitted\n", bogus);
        return False;
    case OpNot:
        ERROR("The ! operator cannot be applied to a key name\n");
        return False;
    case OpUnaryPlus:
        ERROR("The + operator cannot be applied to a key name\n");
        return False;
    default:
        WSGO("Unknown operator %d in ResolveKeyName\n", expr->op);
        break;
    }
    return False;
}

/***====================================================================***/

int
ExprResolveEnum(ExprDef * expr, ExprResult * val_rtrn, const LookupEntry * values)
{
    if (expr->op != ExprIdent)
    {
        ERROR("Found a %s where an enumerated value was expected\n",
               exprOpText(expr->op));
        return False;
    }
    if (!SimpleLookup(values, expr->value.str, TypeInt, val_rtrn))
    {
        int nOut = 0;
        ERROR("Illegal identifier %s (expected one of: ",
               XkbcAtomText(expr->value.str));
        while (values && values->name)
        {
            if (nOut != 0)
                INFO(", %s", values->name);
            else
                INFO("%s", values->name);
            values++;
            nOut++;
        }
        INFO(")\n");
        return False;
    }
    return True;
}

static int
ExprResolveMaskLookup(ExprDef * expr,
                      ExprResult * val_rtrn,
                      IdentLookupFunc lookup,
                      const void * lookupPriv)
{
    int ok = 0;
    ExprResult leftRtrn, rightRtrn;
    ExprDef *left, *right;
    const char *bogus = NULL;

    switch (expr->op)
    {
    case ExprValue:
        if (expr->type != TypeInt)
        {
            ERROR
                ("Found constant of type %s where a mask was expected\n",
                 exprTypeText(expr->type));
            return False;
        }
        val_rtrn->ival = expr->value.ival;
        return True;
    case ExprIdent:
        ok = (*lookup) (lookupPriv, expr->value.str, TypeInt, val_rtrn);
        if (!ok)
            ERROR("Identifier \"%s\" of type int is unknown\n",
                   XkbcAtomText(expr->value.str));
        return ok;
    case ExprFieldRef:
        ERROR("Default \"%s.%s\" of type int is unknown\n",
              XkbcAtomText(expr->value.field.element),
              XkbcAtomText(expr->value.field.field));
        return False;
    case ExprArrayRef:
        bogus = "array reference";
    case ExprActionDecl:
        if (bogus == NULL)
            bogus = "function use";
        ERROR("Unexpected %s in mask expression\n", bogus);
        ACTION("Expression ignored\n");
        return False;
    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpDivide:
        left = expr->value.binary.left;
        right = expr->value.binary.right;
        if (ExprResolveMaskLookup(left, &leftRtrn, lookup, lookupPriv) &&
            ExprResolveMaskLookup(right, &rightRtrn, lookup, lookupPriv))
        {
            switch (expr->op)
            {
            case OpAdd:
                val_rtrn->ival = leftRtrn.ival | rightRtrn.ival;
                break;
            case OpSubtract:
                val_rtrn->ival = leftRtrn.ival & (~rightRtrn.ival);
                break;
            case OpMultiply:
            case OpDivide:
                ERROR("Cannot %s masks\n",
                       expr->op == OpDivide ? "divide" : "multiply");
                ACTION("Illegal operation ignored\n");
                return False;
            }
            return True;
        }
        return False;
    case OpAssign:
        WSGO("Assignment operator not implemented yet\n");
        break;
    case OpInvert:
        left = expr->value.child;
        if (ExprResolveIntegerLookup(left, &leftRtrn, lookup, lookupPriv))
        {
            val_rtrn->ival = ~leftRtrn.ival;
            return True;
        }
        return False;
    case OpUnaryPlus:
    case OpNegate:
    case OpNot:
        left = expr->value.child;
        if (ExprResolveIntegerLookup(left, &leftRtrn, lookup, lookupPriv))
        {
            ERROR("The %s operator cannot be used with a mask\n",
                   (expr->op == OpNegate ? "-" : "!"));
        }
        return False;
    default:
        WSGO("Unknown operator %d in ResolveMask\n", expr->op);
        break;
    }
    return False;
}

int
ExprResolveMask(ExprDef * expr,
                ExprResult * val_rtrn,
                const LookupEntry * values)
{
    return ExprResolveMaskLookup(expr, val_rtrn, SimpleLookup, values);
}

int
ExprResolveModMask(ExprDef * expr,
                   ExprResult * val_rtrn)
{
    return ExprResolveMaskLookup(expr, val_rtrn, LookupModMask, NULL);
}

int
ExprResolveVModMask(ExprDef * expr,
                    ExprResult * val_rtrn,
                    struct xkb_desc *xkb)
{
    return ExprResolveMaskLookup(expr, val_rtrn, LookupVModMask, xkb);
}

int
ExprResolveKeySym(ExprDef * expr,
                  ExprResult * val_rtrn)
{
    int ok = 0;
    xkb_keysym_t sym;

    if (expr->op == ExprIdent)
    {
        const char *str;
        str = XkbcAtomText(expr->value.str);
        if (str) {
            sym = xkb_string_to_keysym(str);
            if (sym != XKB_KEYSYM_NO_SYMBOL) {
                val_rtrn->uval = sym;
                return True;
            }
        }
    }
    ok = ExprResolveInteger(expr, val_rtrn);
    if ((ok) && (val_rtrn->uval < 10))
        val_rtrn->uval += '0';
    return ok;
}
