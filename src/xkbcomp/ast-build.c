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

/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 *         Ran Benita <ran234@gmail.com>
 */

#include "bump.h"
#include "config.h"

#include "xkbcommon/xkbcommon.h"
#include "xkbcomp-priv.h"
#include "ast-build.h"
#include "include.h"

static ExprDef *
ExprCreate(struct bump *bump, enum expr_op_type op, enum expr_value_type type, size_t size)
{
    ExprDef *expr = bump_aligned_alloc(bump, alignof(ExprDef), size);
    if (!expr)
        return NULL;

    expr->common.type = STMT_EXPR;
    expr->common.next = NULL;
    expr->expr.op = op;
    expr->expr.value_type = type;

    return expr;
}

ExprDef *
ExprCreateString(struct bump *bump, xkb_atom_t str)
{
    ExprDef *expr = ExprCreate(bump, EXPR_VALUE, EXPR_TYPE_STRING, sizeof(ExprString));
    if (!expr)
        return NULL;
    expr->string.str = str;
    return expr;
}

ExprDef *
ExprCreateInteger(struct bump *bump, int ival)
{
    ExprDef *expr = ExprCreate(bump, EXPR_VALUE, EXPR_TYPE_INT, sizeof(ExprInteger));
    if (!expr)
        return NULL;
    expr->integer.ival = ival;
    return expr;
}

ExprDef *
ExprCreateFloat(struct bump *bump)
{
    ExprDef *expr = ExprCreate(bump, EXPR_VALUE, EXPR_TYPE_FLOAT, sizeof(ExprFloat));
    if (!expr)
        return NULL;
    return expr;
}

ExprDef *
ExprCreateBoolean(struct bump *bump, bool set)
{
    ExprDef *expr = ExprCreate(bump, EXPR_VALUE, EXPR_TYPE_BOOLEAN, sizeof(ExprBoolean));
    if (!expr)
        return NULL;
    expr->boolean.set = set;
    return expr;
}

ExprDef *
ExprCreateKeyName(struct bump *bump, xkb_atom_t key_name)
{
    ExprDef *expr = ExprCreate(bump, EXPR_VALUE, EXPR_TYPE_KEYNAME, sizeof(ExprKeyName));
    if (!expr)
        return NULL;
    expr->key_name.key_name = key_name;
    return expr;
}

ExprDef *
ExprCreateIdent(struct bump *bump, xkb_atom_t ident)
{
    ExprDef *expr = ExprCreate(bump, EXPR_IDENT, EXPR_TYPE_UNKNOWN, sizeof(ExprIdent));
    if (!expr)
        return NULL;
    expr->ident.ident = ident;
    return expr;
}

ExprDef *
ExprCreateUnary(struct bump *bump, enum expr_op_type op, enum expr_value_type type,
                ExprDef *child)
{
    ExprDef *expr = ExprCreate(bump, op, type, sizeof(ExprUnary));
    if (!expr)
        return NULL;
    expr->unary.child = child;
    return expr;
}

ExprDef *
ExprCreateBinary(struct bump *bump, enum expr_op_type op, ExprDef *left, ExprDef *right)
{
    ExprDef *expr = ExprCreate(bump, op, EXPR_TYPE_UNKNOWN, sizeof(ExprBinary));
    if (!expr)
        return NULL;

    if (op == EXPR_ASSIGN || left->expr.value_type == EXPR_TYPE_UNKNOWN)
        expr->expr.value_type = right->expr.value_type;
    else if (left->expr.value_type == right->expr.value_type ||
             right->expr.value_type == EXPR_TYPE_UNKNOWN)
        expr->expr.value_type = left->expr.value_type;
    expr->binary.left = left;
    expr->binary.right = right;

    return expr;
}

ExprDef *
ExprCreateFieldRef(struct bump *bump, xkb_atom_t element, xkb_atom_t field)
{
    ExprDef *expr = ExprCreate(bump, EXPR_FIELD_REF, EXPR_TYPE_UNKNOWN, sizeof(ExprFieldRef));
    if (!expr)
        return NULL;
    expr->field_ref.element = element;
    expr->field_ref.field = field;
    return expr;
}

ExprDef *
ExprCreateArrayRef(struct bump *bump, xkb_atom_t element, xkb_atom_t field, ExprDef *entry)
{
    ExprDef *expr = ExprCreate(bump, EXPR_ARRAY_REF, EXPR_TYPE_UNKNOWN, sizeof(ExprArrayRef));
    if (!expr)
        return NULL;
    expr->array_ref.element = element;
    expr->array_ref.field = field;
    expr->array_ref.entry = entry;
    return expr;
}

ExprDef *
ExprEmptyList(struct bump *bump)
{
    return ExprCreate(bump, EXPR_EMPTY_LIST, EXPR_TYPE_UNKNOWN, sizeof(ExprCommon));
}

ExprDef *
ExprCreateAction(struct bump *bump, xkb_atom_t name, ExprDef *args)
{
    ExprDef *expr = ExprCreate(bump, EXPR_ACTION_DECL, EXPR_TYPE_UNKNOWN, sizeof(ExprAction));
    if (!expr)
        return NULL;
    expr->action.name = name;
    expr->action.args = args;
    return expr;
}

ExprDef *
ExprCreateActionList(struct bump *bump, ExprDef *actions)
{
    ExprDef *expr = ExprCreate(bump, EXPR_ACTION_LIST, EXPR_TYPE_ACTIONS, sizeof(ExprActionList));
    if (!expr)
        return NULL;
    expr->actions.actions = actions;
    return expr;
}

ExprDef *
ExprCreateKeysymList(struct bump *bump, xkb_keysym_t sym)
{
    ExprDef *expr = ExprCreate(bump, EXPR_KEYSYM_LIST, EXPR_TYPE_SYMBOLS, sizeof(ExprKeysymList));
    if (!expr)
        return NULL;
    expr->keysym_list.syms = bump_alloc(bump, *expr->keysym_list.syms);
    if (!expr->keysym_list.syms) {
        return NULL;
    }
    expr->keysym_list.num_syms = 1;
    expr->keysym_list.syms[0] = sym;
    return expr;
}

ExprDef *
ExprAppendKeysymList(struct bump *bump, ExprDef *expr, xkb_keysym_t sym)
{
    ExprKeysymList *kl = &expr->keysym_list;
    xkb_keysym_t *old = kl->syms;
    kl->syms = bump_aligned_alloc(bump, alignof(xkb_keysym_t), (kl->num_syms + 1) * sizeof(*kl->syms));
    for (unsigned i = 0; i < kl->num_syms; i++)
        kl->syms[i] = old[i];
    kl->syms[kl->num_syms++] = sym;
    return expr;
}

KeycodeDef *
KeycodeCreate(struct bump *bump, xkb_atom_t name, int64_t value)
{
    KeycodeDef *def = bump_alloc(bump, *def);
    if (!def)
        return NULL;

    def->common.type = STMT_KEYCODE;
    def->common.next = NULL;
    def->name = name;
    def->value = value;

    return def;
}

KeyAliasDef *
KeyAliasCreate(struct bump *bump, xkb_atom_t alias, xkb_atom_t real)
{
    KeyAliasDef *def = bump_alloc(bump, *def);
    if (!def)
        return NULL;

    def->common.type = STMT_ALIAS;
    def->common.next = NULL;
    def->alias = alias;
    def->real = real;

    return def;
}

VModDef *
VModCreate(struct bump *bump, xkb_atom_t name, ExprDef *value)
{
    VModDef *def = bump_alloc(bump, *def);
    if (!def)
        return NULL;

    def->common.type = STMT_VMOD;
    def->common.next = NULL;
    def->name = name;
    def->value = value;

    return def;
}

VarDef *
VarCreate(struct bump *bump, ExprDef *name, ExprDef *value)
{
    VarDef *def = bump_alloc(bump, *def);
    if (!def)
        return NULL;

    def->common.type = STMT_VAR;
    def->common.next = NULL;
    def->name = name;
    def->value = value;

    return def;
}

VarDef *
BoolVarCreate(struct bump *bump, xkb_atom_t ident, bool set)
{
    ExprDef *name, *value;
    VarDef *def;
    if (!(name = ExprCreateIdent(bump, ident))) {
        return NULL;
    }
    if (!(value = ExprCreateBoolean(bump, set))) {
        return NULL;
    }
    if (!(def = VarCreate(bump, name, value))) {
        return NULL;
    }
    return def;
}

InterpDef *
InterpCreate(struct bump *bump, xkb_keysym_t sym, ExprDef *match)
{
    InterpDef *def = bump_alloc(bump, *def);
    if (!def)
        return NULL;

    def->common.type = STMT_INTERP;
    def->common.next = NULL;
    def->sym = sym;
    def->match = match;
    def->def = NULL;

    return def;
}

KeyTypeDef *
KeyTypeCreate(struct bump *bump, xkb_atom_t name, VarDef *body)
{
    KeyTypeDef *def = bump_alloc(bump, *def);
    if (!def)
        return NULL;

    def->common.type = STMT_TYPE;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->name = name;
    def->body = body;

    return def;
}

SymbolsDef *
SymbolsCreate(struct bump *bump, xkb_atom_t keyName, VarDef *symbols)
{
    SymbolsDef *def = bump_alloc(bump, *def);
    if (!def)
        return NULL;

    def->common.type = STMT_SYMBOLS;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->keyName = keyName;
    def->symbols = symbols;

    return def;
}

GroupCompatDef *
GroupCompatCreate(struct bump *bump, unsigned group, ExprDef *val)
{
    GroupCompatDef *def = bump_alloc(bump, *def);
    if (!def)
        return NULL;

    def->common.type = STMT_GROUP_COMPAT;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->group = group;
    def->def = val;

    return def;
}

ModMapDef *
ModMapCreate(struct bump *bump, xkb_atom_t modifier, ExprDef *keys)
{
    ModMapDef *def = bump_alloc(bump, *def);
    if (!def)
        return NULL;

    def->common.type = STMT_MODMAP;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->modifier = modifier;
    def->keys = keys;

    return def;
}

LedMapDef *
LedMapCreate(struct bump *bump, xkb_atom_t name, VarDef *body)
{
    LedMapDef *def = bump_alloc(bump, *def);
    if (!def)
        return NULL;

    def->common.type = STMT_LED_MAP;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->name = name;
    def->body = body;

    return def;
}

LedNameDef *
LedNameCreate(struct bump *bump, unsigned ndx, ExprDef *name, bool virtual)
{
    LedNameDef *def = bump_alloc(bump, *def);
    if (!def)
        return NULL;

    def->common.type = STMT_LED_NAME;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->ndx = ndx;
    def->name = name;
    def->virtual = virtual;

    return def;
}

IncludeStmt *
IncludeCreate(struct bump *bump, struct xkb_context *ctx, char *str, enum merge_mode merge)
{
    IncludeStmt *incl, *first;
    char *stmt, *tmp;
    char nextop;

    incl = first = NULL;
    tmp = str;
    stmt = str ? bump_strdup(bump, str) : NULL;
    while (tmp && *tmp)
    {
        char *file = NULL, *map = NULL, *extra_data = NULL;

        if (!ParseIncludeMap(bump, &tmp, &file, &map, &nextop, &extra_data))
            goto err;

        /*
         * Given an RMLVO (here layout) like 'us,,fr', the rules parser
         * will give out something like 'pc+us+:2+fr:3+inet(evdev)'.
         * We should just skip the ':2' in this case and leave it to the
         * appropriate section to deal with the empty group.
         */
        if (isempty(file)) {
            continue;
        }

        if (first == NULL) {
            first = incl = bump_alloc(bump, *first);
        } else {
            incl->next_incl = bump_alloc(bump, *first);
            incl = incl->next_incl;
        }

        if (!incl) {
            break;
        }

        incl->common.type = STMT_INCLUDE;
        incl->common.next = NULL;
        incl->merge = merge;
        incl->stmt = NULL;
        incl->file = file;
        incl->map = map;
        incl->modifier = extra_data;
        incl->next_incl = NULL;

        if (nextop == MERGE_AUGMENT_PREFIX)
            merge = MERGE_AUGMENT;
        else
            merge = MERGE_OVERRIDE;
    }

    if (first)
        first->stmt = stmt;

    return first;

err:
    log_err(ctx, XKB_ERROR_INVALID_INCLUDE_STATEMENT,
            "Illegal include statement \"%s\"; Ignored\n", stmt);
    return NULL;
}

XkbFile *
XkbFileCreate(struct bump *bump, enum xkb_file_type type, char *name,
              ParseCommon *defs, enum xkb_map_flags flags)
{
    XkbFile *file;

    file = bump_alloc(bump, *file);
    if (!file)
        return NULL;
    memset(file, 0, sizeof(*file));

    XkbEscapeMapName(name);
    file->bump = bump;
    file->file_type = type;
    if (name) {
        file->name = name;
    } else {
        file->name = bump_strdup(bump, "(unnamed)");
    }
    file->defs = defs;
    file->flags = flags;

    return file;
}

XkbFile *
XkbFileFromComponents(struct bump *bump, struct xkb_context *ctx,
                      const struct xkb_component_names *kkctgs)
{
    char *const components[] = {
        kkctgs->keycodes, kkctgs->types,
        kkctgs->compat, kkctgs->symbols,
    };
    enum xkb_file_type type;
    IncludeStmt *include = NULL;
    XkbFile *file = NULL;
    ParseCommon *defs = NULL, *defsLast = NULL;

    for (type = FIRST_KEYMAP_FILE_TYPE; type <= LAST_KEYMAP_FILE_TYPE; type++) {
        include = IncludeCreate(bump, ctx, components[type], MERGE_DEFAULT);
        if (!include)
            goto err;

        file = XkbFileCreate(bump, type, NULL, (ParseCommon *) include, 0);
        if (!file) {
            goto err;
        }

        if (!defs)
            defsLast = defs = &file->common;
        else
            defsLast = defsLast->next = &file->common;
    }

    file = XkbFileCreate(bump, FILE_TYPE_KEYMAP, NULL, defs, 0);
    if (!file)
        goto err;

    return file;

err:
    return NULL;
}

static const char *xkb_file_type_strings[_FILE_TYPE_NUM_ENTRIES] = {
    [FILE_TYPE_KEYCODES] = "xkb_keycodes",
    [FILE_TYPE_TYPES] = "xkb_types",
    [FILE_TYPE_COMPAT] = "xkb_compatibility",
    [FILE_TYPE_SYMBOLS] = "xkb_symbols",
    [FILE_TYPE_GEOMETRY] = "xkb_geometry",
    [FILE_TYPE_KEYMAP] = "xkb_keymap",
    [FILE_TYPE_RULES] = "rules",
};

const char *
xkb_file_type_to_string(enum xkb_file_type type)
{
    if (type >= _FILE_TYPE_NUM_ENTRIES)
        return "unknown";
    return xkb_file_type_strings[type];
}

static const char *stmt_type_strings[_STMT_NUM_VALUES] = {
    [STMT_UNKNOWN] = "unknown statement",
    [STMT_INCLUDE] = "include statement",
    [STMT_KEYCODE] = "key name definition",
    [STMT_ALIAS] = "key alias definition",
    [STMT_EXPR] = "expression",
    [STMT_VAR] = "variable definition",
    [STMT_TYPE] = "key type definition",
    [STMT_INTERP] = "symbol interpretation definition",
    [STMT_VMOD] = "virtual modifiers definition",
    [STMT_SYMBOLS] = "key symbols definition",
    [STMT_MODMAP] = "modifier map declaration",
    [STMT_GROUP_COMPAT] = "group declaration",
    [STMT_LED_MAP] = "indicator map declaration",
    [STMT_LED_NAME] = "indicator name declaration",
};

const char *
stmt_type_to_string(enum stmt_type type)
{
    if (type >= _STMT_NUM_VALUES)
        return NULL;
    return stmt_type_strings[type];
}

static const char *expr_op_type_strings[_EXPR_NUM_VALUES] = {
    [EXPR_VALUE] = "literal",
    [EXPR_IDENT] = "identifier",
    [EXPR_ACTION_DECL] = "action declaration",
    [EXPR_FIELD_REF] = "field reference",
    [EXPR_ARRAY_REF] = "array reference",
    [EXPR_EMPTY_LIST] = "empty list",
    [EXPR_KEYSYM_LIST] = "list of keysyms",
    [EXPR_ACTION_LIST] = "list of actions",
    [EXPR_ADD] = "addition",
    [EXPR_SUBTRACT] = "subtraction",
    [EXPR_MULTIPLY] = "multiplication",
    [EXPR_DIVIDE] = "division",
    [EXPR_ASSIGN] = "assignment",
    [EXPR_NOT] = "logical negation",
    [EXPR_NEGATE] = "arithmetic negation",
    [EXPR_INVERT] = "bitwise inversion",
    [EXPR_UNARY_PLUS] = "unary plus",
};

const char *
expr_op_type_to_string(enum expr_op_type type)
{
    if (type >= _EXPR_NUM_VALUES)
        return NULL;
    return expr_op_type_strings[type];
}

static const char *expr_value_type_strings[_EXPR_TYPE_NUM_VALUES] = {
    [EXPR_TYPE_UNKNOWN] = "unknown",
    [EXPR_TYPE_BOOLEAN] = "boolean",
    [EXPR_TYPE_INT] = "int",
    [EXPR_TYPE_FLOAT] = "float",
    [EXPR_TYPE_STRING] = "string",
    [EXPR_TYPE_ACTION] = "action",
    [EXPR_TYPE_ACTIONS] = "actions",
    [EXPR_TYPE_KEYNAME] = "keyname",
    [EXPR_TYPE_SYMBOLS] = "symbols",
};

const char *
expr_value_type_to_string(enum expr_value_type type)
{
    if (type >= _EXPR_TYPE_NUM_VALUES)
        return NULL;
    return expr_value_type_strings[type];
}
