/*
 * For HPND
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

#include "xkbcomp-priv.h"
#include "ast-build.h"
#include "include.h"

static ExprDef *
ExprCreate(enum stmt_type op)
{
    ExprDef *expr = malloc(sizeof(*expr));
    if (!expr)
        return NULL;

    expr->common.type = op;
    expr->common.next = NULL;

    return expr;
}

ExprDef *
ExprCreateString(xkb_atom_t str)
{
    ExprDef *expr = ExprCreate(STMT_EXPR_STRING_LITERAL);
    if (!expr)
        return NULL;
    expr->string.str = str;
    return expr;
}

ExprDef *
ExprCreateInteger(int ival)
{
    ExprDef *expr = ExprCreate(STMT_EXPR_INTEGER_LITERAL);
    if (!expr)
        return NULL;
    expr->integer.ival = ival;
    return expr;
}

ExprDef *
ExprCreateFloat(void)
{
    ExprDef *expr = ExprCreate(STMT_EXPR_FLOAT_LITERAL);
    if (!expr)
        return NULL;
    return expr;
}

ExprDef *
ExprCreateBoolean(bool set)
{
    ExprDef *expr = ExprCreate(STMT_EXPR_BOOLEAN_LITERAL);
    if (!expr)
        return NULL;
    expr->boolean.set = set;
    return expr;
}

ExprDef *
ExprCreateKeyName(xkb_atom_t key_name)
{
    ExprDef *expr = ExprCreate(STMT_EXPR_KEYNAME_LITERAL);
    if (!expr)
        return NULL;
    expr->key_name.key_name = key_name;
    return expr;
}

ExprDef *
ExprCreateIdent(xkb_atom_t ident)
{
    ExprDef *expr = ExprCreate(STMT_EXPR_IDENT);
    if (!expr)
        return NULL;
    expr->ident.ident = ident;
    return expr;
}

ExprDef *
ExprCreateUnary(enum stmt_type op, ExprDef *child)
{
    ExprDef *expr = ExprCreate(op);
    if (!expr)
        return NULL;
    expr->unary.child = child;
    return expr;
}

ExprDef *
ExprCreateBinary(enum stmt_type op, ExprDef *left, ExprDef *right)
{
    ExprDef *expr = ExprCreate(op);
    if (!expr)
        return NULL;

    expr->binary.left = left;
    expr->binary.right = right;

    return expr;
}

ExprDef *
ExprCreateFieldRef(xkb_atom_t element, xkb_atom_t field)
{
    ExprDef *expr = ExprCreate(STMT_EXPR_FIELD_REF);
    if (!expr)
        return NULL;
    expr->field_ref.element = element;
    expr->field_ref.field = field;
    return expr;
}

ExprDef *
ExprCreateArrayRef(xkb_atom_t element, xkb_atom_t field, ExprDef *entry)
{
    ExprDef *expr = ExprCreate(STMT_EXPR_ARRAY_REF);
    if (!expr)
        return NULL;
    expr->array_ref.element = element;
    expr->array_ref.field = field;
    expr->array_ref.entry = entry;
    return expr;
}

ExprDef *
ExprEmptyList(void)
{
    return ExprCreate(STMT_EXPR_EMPTY_LIST);
}

ExprDef *
ExprCreateAction(xkb_atom_t name, ExprDef *args)
{
    ExprDef *expr = ExprCreate(STMT_EXPR_ACTION_DECL);
    if (!expr)
        return NULL;
    expr->action.name = name;
    expr->action.args = args;
    return expr;
}

ExprDef *
ExprCreateActionList(ExprDef *actions)
{
    ExprDef *expr = ExprCreate(STMT_EXPR_ACTION_LIST);
    if (!expr)
        return NULL;
    expr->actions.actions = actions;
    return expr;
}

ExprDef *
ExprCreateKeysymList(xkb_keysym_t sym)
{
    ExprDef *expr = ExprCreate(STMT_EXPR_KEYSYM_LIST);
    if (!expr)
        return NULL;
    darray_init(expr->keysym_list.syms);
    if (sym == XKB_KEY_NoSymbol) {
        /* Discard NoSymbol */
    } else {
        darray_append(expr->keysym_list.syms, sym);
    }
    return expr;
}

ExprDef *
ExprAppendKeysymList(ExprDef *expr, xkb_keysym_t sym)
{
    if (sym == XKB_KEY_NoSymbol) {
        /* Discard NoSymbol */
    } else {
        darray_append(expr->keysym_list.syms, sym);
    }
    return expr;
}

KeycodeDef *
KeycodeCreate(xkb_atom_t name, int64_t value)
{
    KeycodeDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_KEYCODE;
    def->common.next = NULL;
    def->name = name;
    def->value = value;

    return def;
}

KeyAliasDef *
KeyAliasCreate(xkb_atom_t alias, xkb_atom_t real)
{
    KeyAliasDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_ALIAS;
    def->common.next = NULL;
    def->alias = alias;
    def->real = real;

    return def;
}

VModDef *
VModCreate(xkb_atom_t name, ExprDef *value)
{
    VModDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_VMOD;
    def->common.next = NULL;
    def->name = name;
    def->value = value;

    return def;
}

VarDef *
VarCreate(ExprDef *name, ExprDef *value)
{
    VarDef *def = malloc(sizeof(*def));
    if (!def)
        return NULL;

    def->common.type = STMT_VAR;
    def->common.next = NULL;
    def->name = name;
    def->value = value;

    return def;
}

VarDef *
BoolVarCreate(xkb_atom_t ident, bool set)
{
    ExprDef *name, *value;
    VarDef *def;
    if (!(name = ExprCreateIdent(ident))) {
        return NULL;
    }
    if (!(value = ExprCreateBoolean(set))) {
        FreeStmt((ParseCommon *) name);
        return NULL;
    }
    if (!(def = VarCreate(name, value))) {
        FreeStmt((ParseCommon *) name);
        FreeStmt((ParseCommon *) value);
        return NULL;
    }
    return def;
}

InterpDef *
InterpCreate(xkb_keysym_t sym, ExprDef *match)
{
    InterpDef *def = malloc(sizeof(*def));
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
KeyTypeCreate(xkb_atom_t name, VarDef *body)
{
    KeyTypeDef *def = malloc(sizeof(*def));
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
SymbolsCreate(xkb_atom_t keyName, VarDef *symbols)
{
    SymbolsDef *def = malloc(sizeof(*def));
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
GroupCompatCreate(unsigned group, ExprDef *val)
{
    GroupCompatDef *def = malloc(sizeof(*def));
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
ModMapCreate(xkb_atom_t modifier, ExprDef *keys)
{
    ModMapDef *def = malloc(sizeof(*def));
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
LedMapCreate(xkb_atom_t name, VarDef *body)
{
    LedMapDef *def = malloc(sizeof(*def));
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
LedNameCreate(unsigned ndx, ExprDef *name, bool virtual)
{
    LedNameDef *def = malloc(sizeof(*def));
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

static void
FreeInclude(IncludeStmt *incl);

IncludeStmt *
IncludeCreate(struct xkb_context *ctx, char *str, enum merge_mode merge)
{
    IncludeStmt *incl, *first;
    char *stmt, *tmp;
    char nextop;

    incl = first = NULL;
    tmp = str;
    stmt = strdup_safe(str);
    while (tmp && *tmp)
    {
        char *file = NULL, *map = NULL, *extra_data = NULL;

        if (!ParseIncludeMap(&tmp, &file, &map, &nextop, &extra_data))
            goto err;

        /*
         * Given an RMLVO (here layout) like 'us,,fr', the rules parser
         * will give out something like 'pc+us+:2+fr:3+inet(evdev)'.
         * We should just skip the ':2' in this case and leave it to the
         * appropriate section to deal with the empty group.
         */
        if (isempty(file)) {
            free(file);
            free(map);
            free(extra_data);
            continue;
        }

        if (first == NULL) {
            first = incl = malloc(sizeof(*first));
        } else {
            incl->next_incl = malloc(sizeof(*first));
            incl = incl->next_incl;
        }

        if (!incl) {
            free(file);
            free(map);
            free(extra_data);
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
    else
        free(stmt);

    return first;

err:
    log_err(ctx, XKB_ERROR_INVALID_INCLUDE_STATEMENT,
            "Illegal include statement \"%s\"; Ignored\n", stmt);
    FreeInclude(first);
    free(stmt);
    return NULL;
}

XkbFile *
XkbFileCreate(enum xkb_file_type type, char *name, ParseCommon *defs,
              enum xkb_map_flags flags)
{
    XkbFile *file;

    file = calloc(1, sizeof(*file));
    if (!file)
        return NULL;

    XkbEscapeMapName(name);
    file->file_type = type;
    file->name = name ? name : strdup("(unnamed)");
    file->defs = defs;
    file->flags = flags;

    return file;
}

XkbFile *
XkbFileFromComponents(struct xkb_context *ctx,
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
        include = IncludeCreate(ctx, components[type], MERGE_DEFAULT);
        if (!include)
            goto err;

        file = XkbFileCreate(type, NULL, (ParseCommon *) include, 0);
        if (!file) {
            FreeInclude(include);
            goto err;
        }

        if (!defs)
            defsLast = defs = &file->common;
        else
            defsLast = defsLast->next = &file->common;
    }

    file = XkbFileCreate(FILE_TYPE_KEYMAP, NULL, defs, 0);
    if (!file)
        goto err;

    return file;

err:
    FreeXkbFile((XkbFile *) defs);
    return NULL;
}

static void
FreeInclude(IncludeStmt *incl)
{
    IncludeStmt *next;

    while (incl)
    {
        next = incl->next_incl;

        free(incl->file);
        free(incl->map);
        free(incl->modifier);
        free(incl->stmt);

        free(incl);
        incl = next;
    }
}

void
FreeStmt(ParseCommon *stmt)
{
    ParseCommon *next;

    while (stmt)
    {
        next = stmt->next;

        switch (stmt->type) {
        case STMT_INCLUDE:
            FreeInclude((IncludeStmt *) stmt);
            /* stmt is already free'd here. */
            stmt = NULL;
            break;

        case STMT_EXPR_NEGATE:
        case STMT_EXPR_UNARY_PLUS:
        case STMT_EXPR_NOT:
        case STMT_EXPR_INVERT:
            FreeStmt((ParseCommon *) ((ExprUnary *) stmt)->child);
            break;

        case STMT_EXPR_DIVIDE:
        case STMT_EXPR_ADD:
        case STMT_EXPR_SUBTRACT:
        case STMT_EXPR_MULTIPLY:
        case STMT_EXPR_ASSIGN:
            FreeStmt((ParseCommon *) ((ExprBinary *) stmt)->left);
            FreeStmt((ParseCommon *) ((ExprBinary *) stmt)->right);
            break;

        case STMT_EXPR_ACTION_DECL:
            FreeStmt((ParseCommon *) ((ExprAction *) stmt)->args);
            break;

        case STMT_EXPR_ACTION_LIST:
            FreeStmt((ParseCommon *) ((ExprActionList *) stmt)->actions);
            break;

        case STMT_EXPR_ARRAY_REF:
            FreeStmt((ParseCommon *) ((ExprArrayRef *) stmt)->entry);
            break;

        case STMT_EXPR_KEYSYM_LIST:
            darray_free(((ExprKeysymList *) stmt)->syms);
            break;

        case STMT_VAR:
            FreeStmt((ParseCommon *) ((VarDef *) stmt)->name);
            FreeStmt((ParseCommon *) ((VarDef *) stmt)->value);
            break;
        case STMT_TYPE:
            FreeStmt((ParseCommon *) ((KeyTypeDef *) stmt)->body);
            break;
        case STMT_INTERP:
            FreeStmt((ParseCommon *) ((InterpDef *) stmt)->match);
            FreeStmt((ParseCommon *) ((InterpDef *) stmt)->def);
            break;
        case STMT_VMOD:
            FreeStmt((ParseCommon *) ((VModDef *) stmt)->value);
            break;
        case STMT_SYMBOLS:
            FreeStmt((ParseCommon *) ((SymbolsDef *) stmt)->symbols);
            break;
        case STMT_MODMAP:
            FreeStmt((ParseCommon *) ((ModMapDef *) stmt)->keys);
            break;
        case STMT_GROUP_COMPAT:
            FreeStmt((ParseCommon *) ((GroupCompatDef *) stmt)->def);
            break;
        case STMT_LED_MAP:
            FreeStmt((ParseCommon *) ((LedMapDef *) stmt)->body);
            break;
        case STMT_LED_NAME:
            FreeStmt((ParseCommon *) ((LedNameDef *) stmt)->name);
            break;
        default:
            break;
        }

        free(stmt);
        stmt = next;
    }
}

void
FreeXkbFile(XkbFile *file)
{
    XkbFile *next;

    while (file)
    {
        next = (XkbFile *) file->common.next;

        switch (file->file_type) {
        case FILE_TYPE_KEYMAP:
            FreeXkbFile((XkbFile *) file->defs);
            break;

        case FILE_TYPE_TYPES:
        case FILE_TYPE_COMPAT:
        case FILE_TYPE_SYMBOLS:
        case FILE_TYPE_KEYCODES:
        case FILE_TYPE_GEOMETRY:
            FreeStmt(file->defs);
            break;

        default:
            break;
        }

        free(file->name);
        free(file);
        file = next;
    }
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
    [STMT_EXPR_STRING_LITERAL] = "string literal expression",
    [STMT_EXPR_INTEGER_LITERAL] = "integer literal expression",
    [STMT_EXPR_FLOAT_LITERAL] = "float literal expression",
    [STMT_EXPR_BOOLEAN_LITERAL] = "boolean literal expression",
    [STMT_EXPR_KEYNAME_LITERAL] = "key name expression",
    [STMT_EXPR_IDENT] = "identifier expression",
    [STMT_EXPR_ACTION_DECL] = "action declaration expression",
    [STMT_EXPR_FIELD_REF] = "field reference expression",
    [STMT_EXPR_ARRAY_REF] = "array reference expression",
    [STMT_EXPR_EMPTY_LIST] = "empty list expression",
    [STMT_EXPR_KEYSYM_LIST] = "keysym list expression",
    [STMT_EXPR_ACTION_LIST] = "action list expression",
    [STMT_EXPR_ADD] = "addition expression",
    [STMT_EXPR_SUBTRACT] = "substraction expression",
    [STMT_EXPR_MULTIPLY] = "multiplication expression",
    [STMT_EXPR_DIVIDE] = "division expression",
    [STMT_EXPR_ASSIGN] = "assignment expression",
    [STMT_EXPR_NOT] = "logical negation expression",
    [STMT_EXPR_NEGATE] = "arithmetic negation expression",
    [STMT_EXPR_INVERT] = "bitwise inversion expression",
    [STMT_EXPR_UNARY_PLUS] = "unary plus expression",
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
