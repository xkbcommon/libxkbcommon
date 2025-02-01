/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */

#ifndef XKBCOMP_AST_BUILD_H
#define XKBCOMP_AST_BUILD_H

#include "ast.h"

ExprDef *
ExprCreateString(xkb_atom_t str);

ExprDef *
ExprCreateInteger(int ival);

ExprDef *
ExprCreateFloat(void);

ExprDef *
ExprCreateBoolean(bool set);

ExprDef *
ExprCreateKeyName(xkb_atom_t key_name);

ExprDef *
ExprCreateIdent(xkb_atom_t ident);

ExprDef *
ExprCreateUnary(enum expr_op_type op, enum expr_value_type type,
                ExprDef *child);

ExprDef *
ExprCreateBinary(enum expr_op_type op, ExprDef *left, ExprDef *right);

ExprDef *
ExprCreateFieldRef(xkb_atom_t element, xkb_atom_t field);

ExprDef *
ExprCreateArrayRef(xkb_atom_t element, xkb_atom_t field, ExprDef *entry);

ExprDef *
ExprEmptyList(void);

ExprDef *
ExprCreateAction(xkb_atom_t name, ExprDef *args);

ExprDef *
ExprCreateActionList(ExprDef *actions);

ExprDef *
ExprCreateKeysymList(xkb_keysym_t sym);

ExprDef *
ExprAppendKeysymList(ExprDef *list, xkb_keysym_t sym);

KeycodeDef *
KeycodeCreate(xkb_atom_t name, int64_t value);

KeyAliasDef *
KeyAliasCreate(xkb_atom_t alias, xkb_atom_t real);

VModDef *
VModCreate(xkb_atom_t name, ExprDef *value);

VarDef *
VarCreate(ExprDef *name, ExprDef *value);

VarDef *
BoolVarCreate(xkb_atom_t ident, bool set);

InterpDef *
InterpCreate(xkb_keysym_t sym, ExprDef *match);

KeyTypeDef *
KeyTypeCreate(xkb_atom_t name, VarDef *body);

SymbolsDef *
SymbolsCreate(xkb_atom_t keyName, VarDef *symbols);

GroupCompatDef *
GroupCompatCreate(unsigned group, ExprDef *def);

ModMapDef *
ModMapCreate(xkb_atom_t modifier, ExprDef *keys);

LedMapDef *
LedMapCreate(xkb_atom_t name, VarDef *body);

LedNameDef *
LedNameCreate(unsigned ndx, ExprDef *name, bool virtual);

IncludeStmt *
IncludeCreate(struct xkb_context *ctx, char *str, enum merge_mode merge);

XkbFile *
XkbFileCreate(enum xkb_file_type type, char *name, ParseCommon *defs,
              enum xkb_map_flags flags);

void
FreeStmt(ParseCommon *stmt);

#endif
