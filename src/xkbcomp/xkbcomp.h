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

#define TypeUnknown          0
#define TypeBoolean          1
#define TypeInt              2
#define TypeString           4
#define TypeAction           5
#define TypeKeyName          6
#define TypeSymbols          7

#define StmtUnknown          0
#define StmtInclude          1
#define StmtKeycodeDef       2
#define StmtKeyAliasDef      3
#define StmtExpr             4
#define StmtVarDef           5
#define StmtKeyTypeDef       6
#define StmtInterpDef        7
#define StmtVModDef          8
#define StmtSymbolsDef       9
#define StmtModMapDef        10
#define StmtGroupCompatDef   11
#define StmtIndicatorMapDef  12
#define StmtIndicatorNameDef 13

typedef struct _ParseCommon {
    unsigned stmtType;
    struct _ParseCommon *next;
} ParseCommon;

#define ExprValue      0
#define ExprIdent      1
#define ExprActionDecl 2
#define ExprFieldRef   3
#define ExprArrayRef   4
#define ExprKeysymList 5
#define ExprActionList 6

#define OpAdd          20
#define OpSubtract     21
#define OpMultiply     22
#define OpDivide       23
#define OpAssign       24
#define OpNot          25
#define OpNegate       26
#define OpInvert       27
#define OpUnaryPlus    28

enum merge_mode {
    MERGE_DEFAULT,
    MERGE_AUGMENT,
    MERGE_OVERRIDE,
    MERGE_REPLACE,
};

#define AutoKeyNames (1L << 0)
#define CreateKeyNames(x) ((x)->flags & AutoKeyNames)

typedef struct _IncludeStmt {
    ParseCommon common;
    enum merge_mode merge;
    char *stmt;
    char *file;
    char *map;
    char *modifier;
    char *path;
    struct _IncludeStmt *next;
} IncludeStmt;

typedef struct _Expr {
    ParseCommon common;
    unsigned op;
    unsigned type;
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
        char keyName[5];
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
    char name[5];
    unsigned long value;
} KeycodeDef;

typedef struct _KeyAliasDef {
    ParseCommon common;
    enum merge_mode merge;
    char alias[5];
    char real[5];
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
    char keyName[5];
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
    unsigned type;
    xkb_atom_t name;
    VarDef *body;
} IndicatorMapDef;

typedef struct _XkbFile {
    ParseCommon common;
    enum xkb_file_type type;
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
