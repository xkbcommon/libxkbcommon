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

#ifndef XKBCOMP_H
#define XKBCOMP_H 1

#include "xkb-priv.h"

enum stmt_type {
    STMT_UNKNOWN = 0,
    STMT_INCLUDE,
    STMT_KEYCODE,
    STMT_ALIAS,
    STMT_EXPR,
    STMT_VAR,
    STMT_TYPE,
    STMT_INTERP,
    STMT_VMOD,
    STMT_SYMBOLS,
    STMT_MODMAP,
    STMT_GROUP_COMPAT,
    STMT_INDICATOR_MAP,
    STMT_INDICATOR_NAME,
    _STMT_NUM_VALUES
};

enum expr_value_type {
    EXPR_TYPE_UNKNOWN = 0,
    EXPR_TYPE_BOOLEAN,
    EXPR_TYPE_INT,
    EXPR_TYPE_STRING,
    EXPR_TYPE_ACTION,
    EXPR_TYPE_KEYNAME,
    EXPR_TYPE_SYMBOLS,
};

enum expr_op_type {
    EXPR_VALUE = 0,
    EXPR_IDENT,
    EXPR_ACTION_DECL,
    EXPR_FIELD_REF,
    EXPR_ARRAY_REF,
    EXPR_KEYSYM_LIST,
    EXPR_ACTION_LIST,
    EXPR_ADD,
    EXPR_SUBTRACT,
    EXPR_MULTIPLY,
    EXPR_DIVIDE,
    EXPR_ASSIGN,
    EXPR_NOT,
    EXPR_NEGATE,
    EXPR_INVERT,
    EXPR_UNARY_PLUS,
};

enum merge_mode {
    MERGE_DEFAULT,
    MERGE_AUGMENT,
    MERGE_OVERRIDE,
    MERGE_REPLACE,
};

typedef struct _ParseCommon {
    enum stmt_type type;
    struct _ParseCommon *next;
} ParseCommon;

typedef struct _IncludeStmt {
    ParseCommon common;
    enum merge_mode merge;
    char *stmt;
    char *file;
    char *map;
    char *modifier;
    struct _IncludeStmt *next_incl;
} IncludeStmt;

typedef struct _Expr {
    ParseCommon common;
    enum expr_op_type op;
    enum expr_value_type value_type;
    union {
        struct {
            struct _Expr *left;
            struct _Expr *right;
        } binary;
        struct {
            xkb_atom_t element;
            xkb_atom_t field;
        } field;
        struct {
            xkb_atom_t element;
            xkb_atom_t field;
            struct _Expr *entry;
        } array;
        struct {
            xkb_atom_t name;
            struct _Expr *args;
        } action;
        struct {
            darray(char *) syms;
            darray(int) symsMapIndex;
            darray(unsigned int) symsNumEntries;
        } list;
        struct _Expr *child;
        xkb_atom_t str;
        unsigned uval;
        int ival;
        char keyName[XkbKeyNameLength];
    } value;
} ExprDef;

typedef struct _VarDef {
    ParseCommon common;
    enum merge_mode merge;
    ExprDef *name;
    ExprDef *value;
} VarDef;

typedef struct _VModDef {
    ParseCommon common;
    enum merge_mode merge;
    xkb_atom_t name;
    ExprDef *value;
} VModDef;

typedef struct _KeycodeDef {
    ParseCommon common;
    enum merge_mode merge;
    char name[XkbKeyNameLength];
    unsigned long value;
} KeycodeDef;

typedef struct _KeyAliasDef {
    ParseCommon common;
    enum merge_mode merge;
    char alias[XkbKeyNameLength];
    char real[XkbKeyNameLength];
} KeyAliasDef;

typedef struct _KeyTypeDef {
    ParseCommon common;
    enum merge_mode merge;
    xkb_atom_t name;
    VarDef *body;
} KeyTypeDef;

typedef struct _SymbolsDef {
    ParseCommon common;
    enum merge_mode merge;
    char keyName[XkbKeyNameLength];
    ExprDef *symbols;
} SymbolsDef;

typedef struct _ModMapDef {
    ParseCommon common;
    enum merge_mode merge;
    xkb_atom_t modifier;
    ExprDef *keys;
} ModMapDef;

typedef struct _GroupCompatDef {
    ParseCommon common;
    enum merge_mode merge;
    int group;
    ExprDef *def;
} GroupCompatDef;

typedef struct _InterpDef {
    ParseCommon common;
    enum merge_mode merge;
    char *sym;
    ExprDef *match;
    VarDef *def;
} InterpDef;

typedef struct _IndicatorNameDef {
    ParseCommon common;
    enum merge_mode merge;
    int ndx;
    ExprDef *name;
    bool virtual;
} IndicatorNameDef;

typedef struct _IndicatorMapDef {
    ParseCommon common;
    enum merge_mode merge;
    xkb_atom_t name;
    VarDef *body;
} IndicatorMapDef;

typedef struct _XkbFile {
    ParseCommon common;
    enum xkb_file_type file_type;
    char *topName;
    char *name;
    ParseCommon *defs;
    int id;
    unsigned flags;
} XkbFile;

extern bool
CompileKeycodes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge);

extern bool
CompileKeyTypes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge);

extern bool
CompileCompatMap(XkbFile *file, struct xkb_keymap *keymap,
                 enum merge_mode merge);

extern bool
CompileSymbols(XkbFile *file, struct xkb_keymap *keymap,
               enum merge_mode merge);

#endif /* XKBCOMP_H */
