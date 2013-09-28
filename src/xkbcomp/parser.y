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

%{
#include "xkbcomp-priv.h"
#include "ast-build.h"
#include "parser-priv.h"

struct parser_param {
    struct xkb_context *ctx;
    void *scanner;
    XkbFile *rtrn;
    bool more_maps;
};

static void
_xkbcommon_error(struct YYLTYPE *loc, struct parser_param *param, const char *msg)
{
    scanner_error(loc, param->scanner, msg);
}

#define scanner param->scanner
%}

%name-prefix    "_xkbcommon_"
%pure-parser
%locations
%lex-param      { struct scanner *scanner }
%parse-param    { struct parser_param *param }

%token
        END_OF_FILE     0
        ERROR_TOK       255
        XKB_KEYMAP      1
        XKB_KEYCODES    2
        XKB_TYPES       3
        XKB_SYMBOLS     4
        XKB_COMPATMAP   5
        XKB_GEOMETRY    6
        XKB_SEMANTICS   7
        XKB_LAYOUT      8
        INCLUDE         10
        OVERRIDE        11
        AUGMENT         12
        REPLACE         13
        ALTERNATE       14
        VIRTUAL_MODS    20
        TYPE            21
        INTERPRET       22
        ACTION_TOK      23
        KEY             24
        ALIAS           25
        GROUP           26
        MODIFIER_MAP    27
        INDICATOR       28
        SHAPE           29
        KEYS            30
        ROW             31
        SECTION         32
        OVERLAY         33
        TEXT            34
        OUTLINE         35
        SOLID           36
        LOGO            37
        VIRTUAL         38
        EQUALS          40
        PLUS            41
        MINUS           42
        DIVIDE          43
        TIMES           44
        OBRACE          45
        CBRACE          46
        OPAREN          47
        CPAREN          48
        OBRACKET        49
        CBRACKET        50
        DOT             51
        COMMA           52
        SEMI            53
        EXCLAM          54
        INVERT          55
        STRING          60
        INTEGER         61
        FLOAT           62
        IDENT           63
        KEYNAME         64
        PARTIAL         70
        DEFAULT         71
        HIDDEN          72
        ALPHANUMERIC_KEYS       73
        MODIFIER_KEYS           74
        KEYPAD_KEYS             75
        FUNCTION_KEYS           76
        ALTERNATE_GROUP         77

%right  EQUALS
%left   PLUS MINUS
%left   TIMES DIVIDE
%left   EXCLAM INVERT
%left   OPAREN

%start  XkbFile

%union  {
        int              ival;
        unsigned         uval;
        int64_t          num;
        enum xkb_file_type file_type;
        char            *str;
        xkb_atom_t      sval;
        enum merge_mode merge;
        enum xkb_map_flags mapFlags;
        ParseCommon     *any;
        ExprDef         *expr;
        VarDef          *var;
        VModDef         *vmod;
        InterpDef       *interp;
        KeyTypeDef      *keyType;
        SymbolsDef      *syms;
        ModMapDef       *modMask;
        GroupCompatDef  *groupCompat;
        LedMapDef       *ledMap;
        LedNameDef      *ledName;
        KeycodeDef      *keyCode;
        KeyAliasDef     *keyAlias;
        void            *geom;
        XkbFile         *file;
}

%type <num>     INTEGER FLOAT
%type <str>     IDENT STRING
%type <sval>    KEYNAME
%type <num>     KeyCode
%type <ival>    Number Integer Float SignedNumber
%type <merge>   MergeMode OptMergeMode
%type <file_type> XkbCompositeType FileType
%type <uval>    DoodadType
%type <mapFlags> Flag Flags OptFlags
%type <str>     MapName OptMapName KeySym
%type <sval>    FieldSpec Ident Element String
%type <any>     DeclList Decl
%type <expr>    OptExprList ExprList Expr Term Lhs Terminal ArrayInit KeySyms
%type <expr>    OptKeySymList KeySymList Action ActionList Coord CoordList
%type <var>     VarDecl VarDeclList SymbolsBody SymbolsVarDecl
%type <vmod>    VModDecl VModDefList VModDef
%type <interp>  InterpretDecl InterpretMatch
%type <keyType> KeyTypeDecl
%type <syms>    SymbolsDecl
%type <modMask> ModMapDecl
%type <groupCompat> GroupCompatDecl
%type <ledMap>  LedMapDecl
%type <ledName> LedNameDecl
%type <keyCode> KeyNameDecl
%type <keyAlias> KeyAliasDecl
%type <geom>    ShapeDecl SectionDecl SectionBody SectionBodyItem RowBody RowBodyItem
%type <geom>    Keys Key OverlayDecl OverlayKeyList OverlayKey OutlineList OutlineInList
%type <geom>    DoodadDecl
%type <file>    XkbFile XkbMapConfigList XkbMapConfig
%type <file>    XkbCompositeMap

%%

/*
 * An actual file may contain more than one map. However, if we do things
 * in the normal yacc way, i.e. aggregate all of the maps into a list and
 * let the caller find the map it wants, we end up scanning and parsing a
 * lot of unneeded maps (in the end we always just need one).
 * Instead of doing that, we make yyparse return one map at a time, and
 * then call it repeatedly until we find the map we need. Once we find it,
 * we don't need to parse everything that follows in the file.
 * This does mean that if we e.g. always use the first map, the file may
 * contain complete garbage after that. But it's worth it.
 */

XkbFile         :       XkbCompositeMap
                        { $$ = param->rtrn = $1; param->more_maps = true; }
                |       XkbMapConfig
                        { $$ = param->rtrn = $1; param->more_maps = true; YYACCEPT; }
                |       END_OF_FILE
                        { $$ = param->rtrn = NULL; param->more_maps = false; }
                ;

XkbCompositeMap :       OptFlags XkbCompositeType OptMapName OBRACE
                            XkbMapConfigList
                        CBRACE SEMI
                        { $$ = XkbFileCreate(param->ctx, $2, $3, &$5->common, $1); }
                ;

XkbCompositeType:       XKB_KEYMAP      { $$ = FILE_TYPE_KEYMAP; }
                |       XKB_SEMANTICS   { $$ = FILE_TYPE_KEYMAP; }
                |       XKB_LAYOUT      { $$ = FILE_TYPE_KEYMAP; }
                ;

XkbMapConfigList :      XkbMapConfigList XkbMapConfig
                        {
                            if (!$2)
                                $$ = $1;
                            else
                                $$ = (XkbFile *)AppendStmt(&$1->common, &$2->common);
                        }
                |       XkbMapConfig
                        { $$ = $1; }
                ;

XkbMapConfig    :       OptFlags FileType OptMapName OBRACE
                            DeclList
                        CBRACE SEMI
                        {
                            if ($2 == FILE_TYPE_GEOMETRY) {
                                free($3);
                                FreeStmt($5);
                                $$ = NULL;
                            }
                            else {
                                $$ = XkbFileCreate(param->ctx, $2, $3, $5, $1);
                            }
                        }
                ;

FileType        :       XKB_KEYCODES            { $$ = FILE_TYPE_KEYCODES; }
                |       XKB_TYPES               { $$ = FILE_TYPE_TYPES; }
                |       XKB_COMPATMAP           { $$ = FILE_TYPE_COMPAT; }
                |       XKB_SYMBOLS             { $$ = FILE_TYPE_SYMBOLS; }
                |       XKB_GEOMETRY            { $$ = FILE_TYPE_GEOMETRY; }
                ;

OptFlags        :       Flags                   { $$ = $1; }
                |                               { $$ = 0; }
                ;

Flags           :       Flags Flag              { $$ = ($1 | $2); }
                |       Flag                    { $$ = $1; }
                ;

Flag            :       PARTIAL                 { $$ = MAP_IS_PARTIAL; }
                |       DEFAULT                 { $$ = MAP_IS_DEFAULT; }
                |       HIDDEN                  { $$ = MAP_IS_HIDDEN; }
                |       ALPHANUMERIC_KEYS       { $$ = MAP_HAS_ALPHANUMERIC; }
                |       MODIFIER_KEYS           { $$ = MAP_HAS_MODIFIER; }
                |       KEYPAD_KEYS             { $$ = MAP_HAS_KEYPAD; }
                |       FUNCTION_KEYS           { $$ = MAP_HAS_FN; }
                |       ALTERNATE_GROUP         { $$ = MAP_IS_ALTGR; }
                ;

DeclList        :       DeclList Decl
                        { $$ = AppendStmt($1, $2); }
                |       { $$ = NULL; }
                ;

Decl            :       OptMergeMode VarDecl
                        {
                            $2->merge = $1;
                            $$ = &$2->common;
                        }
                |       OptMergeMode VModDecl
                        {
                            $2->merge = $1;
                            $$ = &$2->common;
                        }
                |       OptMergeMode InterpretDecl
                        {
                            $2->merge = $1;
                            $$ = &$2->common;
                        }
                |       OptMergeMode KeyNameDecl
                        {
                            $2->merge = $1;
                            $$ = &$2->common;
                        }
                |       OptMergeMode KeyAliasDecl
                        {
                            $2->merge = $1;
                            $$ = &$2->common;
                        }
                |       OptMergeMode KeyTypeDecl
                        {
                            $2->merge = $1;
                            $$ = &$2->common;
                        }
                |       OptMergeMode SymbolsDecl
                        {
                            $2->merge = $1;
                            $$ = &$2->common;
                        }
                |       OptMergeMode ModMapDecl
                        {
                            $2->merge = $1;
                            $$ = &$2->common;
                        }
                |       OptMergeMode GroupCompatDecl
                        {
                            $2->merge = $1;
                            $$ = &$2->common;
                        }
                |       OptMergeMode LedMapDecl
                        {
                            $2->merge = $1;
                            $$ = &$2->common;
                        }
                |       OptMergeMode LedNameDecl
                        {
                            $2->merge = $1;
                            $$ = &$2->common;
                        }
                |       OptMergeMode ShapeDecl          { $$ = NULL; }
                |       OptMergeMode SectionDecl        { $$ = NULL; }
                |       OptMergeMode DoodadDecl         { $$ = NULL; }
                |       MergeMode STRING
                        {
                            $$ = &IncludeCreate(param->ctx, $2, $1)->common;
                            free($2);
                        }
                ;

VarDecl         :       Lhs EQUALS Expr SEMI
                        { $$ = VarCreate($1, $3); }
                |       Ident SEMI
                        { $$ = BoolVarCreate($1, 1); }
                |       EXCLAM Ident SEMI
                        { $$ = BoolVarCreate($2, 0); }
                ;

KeyNameDecl     :       KEYNAME EQUALS KeyCode SEMI
                        { $$ = KeycodeCreate($1, $3); }
                ;

KeyAliasDecl    :       ALIAS KEYNAME EQUALS KEYNAME SEMI
                        { $$ = KeyAliasCreate($2, $4); }
                ;

VModDecl        :       VIRTUAL_MODS VModDefList SEMI
                        { $$ = $2; }
                ;

VModDefList     :       VModDefList COMMA VModDef
                        { $$ = (VModDef *)AppendStmt(&$1->common, &$3->common); }
                |       VModDef
                        { $$ = $1; }
                ;

VModDef         :       Ident
                        { $$ = VModCreate($1, NULL); }
                |       Ident EQUALS Expr
                        { $$ = VModCreate($1, $3); }
                ;

InterpretDecl   :       INTERPRET InterpretMatch OBRACE
                            VarDeclList
                        CBRACE SEMI
                        { $2->def = $4; $$ = $2; }
                ;

InterpretMatch  :       KeySym PLUS Expr
                        { $$ = InterpCreate($1, $3); }
                |       KeySym
                        { $$ = InterpCreate($1, NULL); }
                ;

VarDeclList     :       VarDeclList VarDecl
                        { $$ = (VarDef *)AppendStmt(&$1->common, &$2->common); }
                |       VarDecl
                        { $$ = $1; }
                ;

KeyTypeDecl     :       TYPE String OBRACE
                            VarDeclList
                        CBRACE SEMI
                        { $$ = KeyTypeCreate($2, $4); }
                ;

SymbolsDecl     :       KEY KEYNAME OBRACE
                            SymbolsBody
                        CBRACE SEMI
                        { $$ = SymbolsCreate($2, (ExprDef *)$4); }
                ;

SymbolsBody     :       SymbolsBody COMMA SymbolsVarDecl
                        { $$ = (VarDef *)AppendStmt(&$1->common, &$3->common); }
                |       SymbolsVarDecl
                        { $$ = $1; }
                |       { $$ = NULL; }
                ;

SymbolsVarDecl  :       Lhs EQUALS Expr         { $$ = VarCreate($1, $3); }
                |       Lhs EQUALS ArrayInit    { $$ = VarCreate($1, $3); }
                |       Ident                   { $$ = BoolVarCreate($1, 1); }
                |       EXCLAM Ident            { $$ = BoolVarCreate($2, 0); }
                |       ArrayInit               { $$ = VarCreate(NULL, $1); }
                ;

ArrayInit       :       OBRACKET OptKeySymList CBRACKET
                        { $$ = $2; }
                |       OBRACKET ActionList CBRACKET
                        { $$ = ExprCreateUnary(EXPR_ACTION_LIST, EXPR_TYPE_ACTION, $2); }
                ;

GroupCompatDecl :       GROUP Integer EQUALS Expr SEMI
                        { $$ = GroupCompatCreate($2, $4); }
                ;

ModMapDecl      :       MODIFIER_MAP Ident OBRACE ExprList CBRACE SEMI
                        { $$ = ModMapCreate($2, $4); }
                ;

LedMapDecl:             INDICATOR String OBRACE VarDeclList CBRACE SEMI
                        { $$ = LedMapCreate($2, $4); }
                ;

LedNameDecl:            INDICATOR Integer EQUALS Expr SEMI
                        { $$ = LedNameCreate($2, $4, false); }
                |       VIRTUAL INDICATOR Integer EQUALS Expr SEMI
                        { $$ = LedNameCreate($3, $5, true); }
                ;

ShapeDecl       :       SHAPE String OBRACE OutlineList CBRACE SEMI
                        { $$ = NULL; }
                |       SHAPE String OBRACE CoordList CBRACE SEMI
                        { $$ = NULL; }
                ;

SectionDecl     :       SECTION String OBRACE SectionBody CBRACE SEMI
                        { $$ = NULL; }
                ;

SectionBody     :       SectionBody SectionBodyItem     { $$ = NULL;}
                |       SectionBodyItem                 { $$ = NULL; }
                ;

SectionBodyItem :       ROW OBRACE RowBody CBRACE SEMI
                        { $$ = NULL; }
                |       VarDecl
                        { FreeStmt(&$1->common); $$ = NULL; }
                |       DoodadDecl
                        { $$ = NULL; }
                |       LedMapDecl
                        { FreeStmt(&$1->common); $$ = NULL; }
                |       OverlayDecl
                        { $$ = NULL; }
                ;

RowBody         :       RowBody RowBodyItem     { $$ = NULL;}
                |       RowBodyItem             { $$ = NULL; }
                ;

RowBodyItem     :       KEYS OBRACE Keys CBRACE SEMI { $$ = NULL; }
                |       VarDecl
                        { FreeStmt(&$1->common); $$ = NULL; }
                ;

Keys            :       Keys COMMA Key          { $$ = NULL; }
                |       Key                     { $$ = NULL; }
                ;

Key             :       KEYNAME
                        { $$ = NULL; }
                |       OBRACE ExprList CBRACE
                        { FreeStmt(&$2->common); $$ = NULL; }
                ;

OverlayDecl     :       OVERLAY String OBRACE OverlayKeyList CBRACE SEMI
                        { $$ = NULL; }
                ;

OverlayKeyList  :       OverlayKeyList COMMA OverlayKey { $$ = NULL; }
                |       OverlayKey                      { $$ = NULL; }
                ;

OverlayKey      :       KEYNAME EQUALS KEYNAME          { $$ = NULL; }
                ;

OutlineList     :       OutlineList COMMA OutlineInList
                        { $$ = NULL;}
                |       OutlineInList
                        { $$ = NULL; }
                ;

OutlineInList   :       OBRACE CoordList CBRACE
                        { $$ = NULL; }
                |       Ident EQUALS OBRACE CoordList CBRACE
                        { $$ = NULL; }
                |       Ident EQUALS Expr
                        { FreeStmt(&$3->common); $$ = NULL; }
                ;

CoordList       :       CoordList COMMA Coord
                        { $$ = NULL; }
                |       Coord
                        { $$ = NULL; }
                ;

Coord           :       OBRACKET SignedNumber COMMA SignedNumber CBRACKET
                        { $$ = NULL; }
                ;

DoodadDecl      :       DoodadType String OBRACE VarDeclList CBRACE SEMI
                        { FreeStmt(&$4->common); $$ = NULL; }
                ;

DoodadType      :       TEXT    { $$ = 0; }
                |       OUTLINE { $$ = 0; }
                |       SOLID   { $$ = 0; }
                |       LOGO    { $$ = 0; }
                ;

FieldSpec       :       Ident   { $$ = $1; }
                |       Element { $$ = $1; }
                ;

Element         :       ACTION_TOK
                        { $$ = xkb_atom_intern_literal(param->ctx, "action"); }
                |       INTERPRET
                        { $$ = xkb_atom_intern_literal(param->ctx, "interpret"); }
                |       TYPE
                        { $$ = xkb_atom_intern_literal(param->ctx, "type"); }
                |       KEY
                        { $$ = xkb_atom_intern_literal(param->ctx, "key"); }
                |       GROUP
                        { $$ = xkb_atom_intern_literal(param->ctx, "group"); }
                |       MODIFIER_MAP
                        {$$ = xkb_atom_intern_literal(param->ctx, "modifier_map");}
                |       INDICATOR
                        { $$ = xkb_atom_intern_literal(param->ctx, "indicator"); }
                |       SHAPE
                        { $$ = XKB_ATOM_NONE; }
                |       ROW
                        { $$ = XKB_ATOM_NONE; }
                |       SECTION
                        { $$ = XKB_ATOM_NONE; }
                |       TEXT
                        { $$ = XKB_ATOM_NONE; }
                ;

OptMergeMode    :       MergeMode       { $$ = $1; }
                |                       { $$ = MERGE_DEFAULT; }
                ;

MergeMode       :       INCLUDE         { $$ = MERGE_DEFAULT; }
                |       AUGMENT         { $$ = MERGE_AUGMENT; }
                |       OVERRIDE        { $$ = MERGE_OVERRIDE; }
                |       REPLACE         { $$ = MERGE_REPLACE; }
                |       ALTERNATE
                {
                    /*
                     * This used to be MERGE_ALT_FORM. This functionality was
                     * unused and has been removed.
                     */
                    $$ = MERGE_DEFAULT;
                }
                ;

OptExprList     :       ExprList        { $$ = $1; }
                |                       { $$ = NULL; }
                ;

ExprList        :       ExprList COMMA Expr
                        { $$ = (ExprDef *)AppendStmt(&$1->common, &$3->common); }
                |       Expr
                        { $$ = $1; }
                ;

Expr            :       Expr DIVIDE Expr
                        { $$ = ExprCreateBinary(EXPR_DIVIDE, $1, $3); }
                |       Expr PLUS Expr
                        { $$ = ExprCreateBinary(EXPR_ADD, $1, $3); }
                |       Expr MINUS Expr
                        { $$ = ExprCreateBinary(EXPR_SUBTRACT, $1, $3); }
                |       Expr TIMES Expr
                        { $$ = ExprCreateBinary(EXPR_MULTIPLY, $1, $3); }
                |       Lhs EQUALS Expr
                        { $$ = ExprCreateBinary(EXPR_ASSIGN, $1, $3); }
                |       Term
                        { $$ = $1; }
                ;

Term            :       MINUS Term
                        { $$ = ExprCreateUnary(EXPR_NEGATE, $2->value_type, $2); }
                |       PLUS Term
                        { $$ = ExprCreateUnary(EXPR_UNARY_PLUS, $2->value_type, $2); }
                |       EXCLAM Term
                        { $$ = ExprCreateUnary(EXPR_NOT, EXPR_TYPE_BOOLEAN, $2); }
                |       INVERT Term
                        { $$ = ExprCreateUnary(EXPR_INVERT, $2->value_type, $2); }
                |       Lhs
                        { $$ = $1;  }
                |       FieldSpec OPAREN OptExprList CPAREN %prec OPAREN
                        { $$ = ActionCreate($1, $3); }
                |       Terminal
                        { $$ = $1;  }
                |       OPAREN Expr CPAREN
                        { $$ = $2;  }
                ;

ActionList      :       ActionList COMMA Action
                        { $$ = (ExprDef *)AppendStmt(&$1->common, &$3->common); }
                |       Action
                        { $$ = $1; }
                ;

Action          :       FieldSpec OPAREN OptExprList CPAREN
                        { $$ = ActionCreate($1, $3); }
                ;

Lhs             :       FieldSpec
                        {
                            ExprDef *expr;
                            expr = ExprCreate(EXPR_IDENT, EXPR_TYPE_UNKNOWN);
                            expr->value.str = $1;
                            $$ = expr;
                        }
                |       FieldSpec DOT FieldSpec
                        {
                            ExprDef *expr;
                            expr = ExprCreate(EXPR_FIELD_REF, EXPR_TYPE_UNKNOWN);
                            expr->value.field.element = $1;
                            expr->value.field.field = $3;
                            $$ = expr;
                        }
                |       FieldSpec OBRACKET Expr CBRACKET
                        {
                            ExprDef *expr;
                            expr = ExprCreate(EXPR_ARRAY_REF, EXPR_TYPE_UNKNOWN);
                            expr->value.array.element = XKB_ATOM_NONE;
                            expr->value.array.field = $1;
                            expr->value.array.entry = $3;
                            $$ = expr;
                        }
                |       FieldSpec DOT FieldSpec OBRACKET Expr CBRACKET
                        {
                            ExprDef *expr;
                            expr = ExprCreate(EXPR_ARRAY_REF, EXPR_TYPE_UNKNOWN);
                            expr->value.array.element = $1;
                            expr->value.array.field = $3;
                            expr->value.array.entry = $5;
                            $$ = expr;
                        }
                ;

Terminal        :       String
                        {
                            ExprDef *expr;
                            expr = ExprCreate(EXPR_VALUE, EXPR_TYPE_STRING);
                            expr->value.str = $1;
                            $$ = expr;
                        }
                |       Integer
                        {
                            ExprDef *expr;
                            expr = ExprCreate(EXPR_VALUE, EXPR_TYPE_INT);
                            expr->value.ival = $1;
                            $$ = expr;
                        }
                |       Float
                        {
                            $$ = NULL;
                        }
                |       KEYNAME
                        {
                            ExprDef *expr;
                            expr = ExprCreate(EXPR_VALUE, EXPR_TYPE_KEYNAME);
                            expr->value.keyName = $1;
                            $$ = expr;
                        }
                ;

OptKeySymList   :       KeySymList      { $$ = $1; }
                |                       { $$ = NULL; }
                ;

KeySymList      :       KeySymList COMMA KeySym
                        { $$ = AppendKeysymList($1, $3); }
                |       KeySymList COMMA KeySyms
                        { $$ = AppendMultiKeysymList($1, $3); }
                |       KeySym
                        { $$ = CreateKeysymList($1); }
                |       KeySyms
                        { $$ = CreateMultiKeysymList($1); }
                ;

KeySyms         :       OBRACE KeySymList CBRACE
                        { $$ = $2; }
                ;

KeySym          :       IDENT   { $$ = $1; }
                |       SECTION { $$ = strdup("section"); }
                |       Integer
                        {
                            if ($1 < 10) {      /* XK_0 .. XK_9 */
                                $$ = malloc(2);
                                $$[0] = $1 + '0';
                                $$[1] = '\0';
                            }
                            else {
                                $$ = malloc(17);
                                snprintf($$, 17, "0x%x", $1);
                            }
                        }
                ;

SignedNumber    :       MINUS Number    { $$ = -$2; }
                |       Number          { $$ = $1; }
                ;

Number          :       FLOAT   { $$ = $1; }
                |       INTEGER { $$ = $1; }
                ;

Float           :       FLOAT   { $$ = 0; }
                ;

Integer         :       INTEGER { $$ = $1; }
                ;

KeyCode         :       INTEGER { $$ = $1; }
                ;

Ident           :       IDENT   { $$ = xkb_atom_steal(param->ctx, $1); }
                |       DEFAULT { $$ = xkb_atom_intern_literal(param->ctx, "default"); }
                ;

String          :       STRING  { $$ = xkb_atom_steal(param->ctx, $1); }
                ;

OptMapName      :       MapName { $$ = $1; }
                |               { $$ = NULL; }
                ;

MapName         :       STRING  { $$ = $1; }
                ;

%%

#undef scanner

XkbFile *
parse(struct xkb_context *ctx, void *scanner, const char *map)
{
    struct parser_param param;
    int ret;
    XkbFile *first = NULL;

    param.scanner = scanner;
    param.ctx = ctx;

    /*
     * If we got a specific map, we look for it exclusively and return
     * immediately upon finding it. Otherwise, we need to get the
     * default map. If we find a map marked as default, we return it
     * immediately. If there are no maps marked as default, we return
     * the first map in the file.
     */

    while ((ret = yyparse(&param)) == 0 && param.more_maps) {
        if (map) {
            if (streq_not_null(map, param.rtrn->name))
                return param.rtrn;
            else
                FreeXkbFile(param.rtrn);
        }
        else {
            if (param.rtrn->flags & MAP_IS_DEFAULT) {
                FreeXkbFile(first);
                return param.rtrn;
            }
            else if (!first) {
                first = param.rtrn;
            }
            else {
                FreeXkbFile(param.rtrn);
            }
        }
    }

    if (ret != 0) {
        FreeXkbFile(first);
        return NULL;
    }

    return first;
}
