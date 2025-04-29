/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */

/*
 * The parser should work with reasonably recent versions of bison.
 *
 * byacc support was dropped, because it does not support the following features:
 * - `%define parse.error detailed`
 */

/* The following goes in the header */
%code requires {
#include "config.h"

#include "scanner-utils.h"
#include "xkbcomp/ast.h"
}

%{
#include "config.h"

#include <assert.h>

#include "keysym.h"
#include "xkbcomp/xkbcomp-priv.h"
#include "xkbcomp/parser-priv.h"
#include "xkbcomp/ast-build.h"

struct parser_param {
    struct xkb_context *ctx;
    struct scanner *scanner;
    XkbFile *rtrn;
    bool more_maps;
};

#define parser_err(param, error_id, fmt, ...) \
    scanner_err((param)->scanner, error_id, fmt, ##__VA_ARGS__)

#define parser_warn(param, warning_id, fmt, ...) \
    scanner_warn((param)->scanner, warning_id, fmt, ##__VA_ARGS__)

static void
_xkbcommon_error(struct parser_param *param, const char *msg)
{
    parser_err(param, XKB_ERROR_INVALID_XKB_SYNTAX, "%s", msg);
}

static bool
resolve_keysym(struct parser_param *param, struct sval name, xkb_keysym_t *sym_rtrn)
{
    xkb_keysym_t sym;

    if (isvaleq(name, SVAL_LIT("any")) || isvaleq(name, SVAL_LIT("nosymbol"))) {
        *sym_rtrn = XKB_KEY_NoSymbol;
        return true;
    }

    if (isvaleq(name, SVAL_LIT("none")) || isvaleq(name, SVAL_LIT("voidsymbol"))) {
        *sym_rtrn = XKB_KEY_VoidSymbol;
        return true;
    }

    /* xkb_keysym_from_name needs a C string. */
    char buf[XKB_KEYSYM_NAME_MAX_SIZE];
    if (name.len >= sizeof(buf)) {
        return false;
    }
    memcpy(buf, name.start, name.len);
    buf[name.len] = '\0';

    sym = xkb_keysym_from_name(buf, XKB_KEYSYM_NO_FLAGS);
    if (sym != XKB_KEY_NoSymbol) {
        *sym_rtrn = sym;
        check_deprecated_keysyms(parser_warn, param, param->ctx,
                                 sym, buf, buf, "%s", "");
        return true;
    }

    return false;
}

#define param_scanner param->scanner
%}

%define api.pure
%lex-param      { struct scanner *param_scanner }
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
        int64_t          num;
        enum xkb_file_type file_type;
        char            *str;
        struct sval     sval;
        xkb_atom_t      atom;
        enum merge_mode merge;
        enum xkb_map_flags mapFlags;
        xkb_keysym_t    keysym;
        ParseCommon     *any;
        struct { ParseCommon *head; ParseCommon *last; } anyList;
        uint32_t        noSymbolOrActionList;
        ExprDef         *expr;
        struct { ExprDef *head; ExprDef *last; } exprList;
        VarDef          *var;
        struct { VarDef *head; VarDef *last; } varList;
        VModDef         *vmod;
        struct { VModDef *head; VModDef *last; } vmodList;
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
        struct { XkbFile *head; XkbFile *last; } fileList;
}

%type <num>     INTEGER FLOAT
%type <str>     STRING
%type <sval>    IDENT
%type <atom>    KEYNAME
%type <num>     KeyCode Number Integer Float SignedNumber DoodadType
%type <merge>   MergeMode OptMergeMode
%type <file_type> XkbCompositeType FileType
%type <mapFlags> Flag Flags OptFlags
%type <str>     MapName OptMapName
%type <atom>    FieldSpec Ident Element String
%type <keysym>  KeySym KeySymLit
%type <noSymbolOrActionList> NoSymbolOrActionList
%type <any>     Decl
%type <anyList> DeclList
%type <expr>    Expr Term Lhs Terminal Coord CoordList
%type <expr>    MultiKeySymOrActionList NonEmptyActions Actions Action
%type <expr>    KeySyms NonEmptyKeySyms KeySymList KeyOrKeySym
%type <exprList> ExprList KeyOrKeySymList
%type <exprList> ActionList MultiActionList MultiKeySymList
%type <var>     VarDecl SymbolsVarDecl
%type <varList> VarDeclList SymbolsBody OptSymbolsBody
%type <vmod>    VModDef
%type <vmodList> VModDefList VModDecl
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
%type <file>    XkbFile XkbMapConfig
%type <fileList> XkbMapConfigList
%type <file>    XkbCompositeMap

%destructor { FreeStmt((ParseCommon *) $$); }
    <any> <expr> <var> <vmod> <interp> <keyType> <syms> <modMask> <groupCompat>
    <ledMap> <ledName> <keyCode> <keyAlias>
%destructor { FreeStmt((ParseCommon *) $$.head); }
    <anyList> <exprList> <varList> <vmodList>
/* The destructor also runs on the start symbol when the parser *succeeds*.
 * The `if` here catches this case. */
%destructor { if (!param->rtrn) FreeXkbFile($$); } <file>
%destructor { FreeXkbFile($$.head); } <fileList>
%destructor { free($$); } <str>

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
                        { $$ = param->rtrn = $1; param->more_maps = !!param->rtrn; (void) yynerrs; }
                |       XkbMapConfig
                        { $$ = param->rtrn = $1; param->more_maps = !!param->rtrn; YYACCEPT; }
                |       END_OF_FILE
                        { $$ = param->rtrn = NULL; param->more_maps = false; }
                ;

XkbCompositeMap :       OptFlags XkbCompositeType OptMapName OBRACE
                            XkbMapConfigList
                        CBRACE SEMI
                        { $$ = XkbFileCreate($2, $3, (ParseCommon *) $5.head, $1); }
                ;

XkbCompositeType:       XKB_KEYMAP      { $$ = FILE_TYPE_KEYMAP; }
                |       XKB_SEMANTICS   { $$ = FILE_TYPE_KEYMAP; }
                |       XKB_LAYOUT      { $$ = FILE_TYPE_KEYMAP; }
                ;

/*
 * NOTE: Any component is optional
 */
XkbMapConfigList :      XkbMapConfigList XkbMapConfig
                        {
                            if ($2) {
                                if ($1.head) {
                                    $$.head = $1.head;
                                    $$.last->common.next = &$2->common;
                                    $$.last = $2;
                                } else {
                                    $$.head = $$.last = $2;
                                }
                            }
                        }
                |       { $$.head = $$.last = NULL; }
                ;

XkbMapConfig    :       OptFlags FileType OptMapName OBRACE
                            DeclList
                        CBRACE SEMI
                        {
                            $$ = XkbFileCreate($2, $3, $5.head, $1);
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
                        {
                            if ($2) {
                                if ($1.head) {
                                    $$.head = $1.head; $1.last->next = $2; $$.last = $2;
                                } else {
                                    $$.head = $$.last = $2;
                                }
                            }
                        }
                        /*
                         * VModDecl is "inlined" directly into DeclList, i.e.
                         * each VModDef in the VModDecl is a separate Decl in
                         * the File.
                         */
                |       DeclList OptMergeMode VModDecl
                        {
                            for (VModDef *vmod = $3.head; vmod; vmod = (VModDef *) vmod->common.next)
                                vmod->merge = $2;
                            if ($1.head) {
                                $$.head = $1.head; $1.last->next = &$3.head->common; $$.last = &$3.last->common;
                            } else {
                                $$.head = &$3.head->common; $$.last = &$3.last->common;
                            }
                        }
                |       { $$.head = $$.last = NULL; }
                ;

Decl            :       OptMergeMode VarDecl
                        {
                            $2->merge = $1;
                            $$ = (ParseCommon *) $2;
                        }
                /*      OptMergeMode VModDecl - see above. */
                |       OptMergeMode InterpretDecl
                        {
                            $2->merge = $1;
                            $$ = (ParseCommon *) $2;
                        }
                |       OptMergeMode KeyNameDecl
                        {
                            $2->merge = $1;
                            $$ = (ParseCommon *) $2;
                        }
                |       OptMergeMode KeyAliasDecl
                        {
                            $2->merge = $1;
                            $$ = (ParseCommon *) $2;
                        }
                |       OptMergeMode KeyTypeDecl
                        {
                            $2->merge = $1;
                            $$ = (ParseCommon *) $2;
                        }
                |       OptMergeMode SymbolsDecl
                        {
                            $2->merge = $1;
                            $$ = (ParseCommon *) $2;
                        }
                |       OptMergeMode ModMapDecl
                        {
                            $2->merge = $1;
                            $$ = (ParseCommon *) $2;
                        }
                |       OptMergeMode GroupCompatDecl
                        {
                            $2->merge = $1;
                            $$ = (ParseCommon *) $2;
                        }
                |       OptMergeMode LedMapDecl
                        {
                            $2->merge = $1;
                            $$ = (ParseCommon *) $2;
                        }
                |       OptMergeMode LedNameDecl
                        {
                            $2->merge = $1;
                            $$ = (ParseCommon *) $2;
                        }
                |       OptMergeMode ShapeDecl          { $$ = NULL; }
                |       OptMergeMode SectionDecl        { $$ = NULL; }
                |       OptMergeMode DoodadDecl         { $$ = NULL; }
                |       MergeMode STRING
                        {
                            $$ = (ParseCommon *) IncludeCreate(param->ctx, $2, $1);
                            free($2);
                        }
                ;

VarDecl         :       Lhs EQUALS Expr SEMI
                        { $$ = VarCreate($1, $3); }
                |       Ident SEMI
                        { $$ = BoolVarCreate($1, true); }
                |       EXCLAM Ident SEMI
                        { $$ = BoolVarCreate($2, false); }
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
                        { $$.head = $1.head; $$.last->common.next = &$3->common; $$.last = $3; }
                |       VModDef
                        { $$.head = $$.last = $1; }
                ;

VModDef         :       Ident
                        { $$ = VModCreate($1, NULL); }
                |       Ident EQUALS Expr
                        { $$ = VModCreate($1, $3); }
                ;

InterpretDecl   :       INTERPRET InterpretMatch OBRACE
                            VarDeclList
                        CBRACE SEMI
                        { $2->def = $4.head; $$ = $2; }
                ;

InterpretMatch  :       KeySym PLUS Expr
                        { $$ = InterpCreate($1, $3); }
                |       KeySym
                        { $$ = InterpCreate($1, NULL); }
                ;

VarDeclList     :       VarDeclList VarDecl
                        {
                            if ($2) {
                                if ($1.head) {
                                    $$.head = $1.head;
                                    $$.last->common.next = &$2->common;
                                    $$.last = $2;
                                } else {
                                    $$.head = $$.last = $2;
                                }
                            }
                        }
                |       { $$.head = $$.last = NULL; }
                ;

KeyTypeDecl     :       TYPE String OBRACE
                            VarDeclList
                        CBRACE SEMI
                        { $$ = KeyTypeCreate($2, $4.head); }
                ;

SymbolsDecl     :       KEY KEYNAME OBRACE
                            OptSymbolsBody
                        CBRACE SEMI
                        { $$ = SymbolsCreate($2, $4.head); }
                ;

OptSymbolsBody  :       SymbolsBody { $$ = $1; }
                |                   { $$.head = $$.last = NULL; }
                ;

SymbolsBody     :       SymbolsBody COMMA SymbolsVarDecl
                        { $$.head = $1.head; $$.last->common.next = &$3->common; $$.last = $3; }
                |       SymbolsVarDecl
                        { $$.head = $$.last = $1; }
                ;

SymbolsVarDecl  :       Lhs EQUALS Expr         { $$ = VarCreate($1, $3); }
                |       Lhs EQUALS MultiKeySymOrActionList { $$ = VarCreate($1, $3); }
                |       Ident                   { $$ = BoolVarCreate($1, true); }
                |       EXCLAM Ident            { $$ = BoolVarCreate($2, false); }
                |       MultiKeySymOrActionList { $$ = VarCreate(NULL, $1); }
                ;

/*
 * A list of keysym/action lists
 *
 * There is some ambiguity because we use `{}` to denote both an empty list of
 * keysyms and an empty list of actions. But as soon as we get a keysym or an
 * action, we know whether it is a `MultiKeySymList` or a `MultiActionList`.
 * So we just count the `{}` at the *beginning* using `NoSymbolOrActionList`,
 * then replace it by the relevant count of `NoSymbol` or `NoAction()` once the
 * ambiguity is solved. If not, this is a list of empties of *some* type: we
 * drop those empties and delegate the type resolution using `ExprEmptyList()`.
 */
MultiKeySymOrActionList
                :       OBRACKET MultiKeySymList CBRACKET
                        { $$ = $2.head; }
                |       OBRACKET NoSymbolOrActionList COMMA MultiKeySymList CBRACKET
                        {
                            /* Prepend n times NoSymbol */
                            struct {ExprDef *head; ExprDef *last;} list = {
                                .head = $4.head, .last = $4.last
                            };
                            for (uint32_t k = 0; k < $2; k++) {
                                ExprDef* const syms =
                                    ExprCreateKeySymList(XKB_KEY_NoSymbol);
                                if (!syms) {
                                    /* TODO: Use Bison’s more appropriate YYNOMEM */
                                    YYABORT;
                                }
                                syms->common.next = &list.head->common;
                                list.head = syms;
                            }
                            $$ = list.head;
                        }
                |       OBRACKET MultiActionList CBRACKET
                        { $$ = $2.head; }
                |       OBRACKET NoSymbolOrActionList COMMA MultiActionList CBRACKET
                        {
                            /* Prepend n times NoAction() */
                            struct {ExprDef *head; ExprDef *last;} list = {
                                .head = $4.head, .last = $4.last
                            };
                            for (uint32_t k = 0; k < $2; k++) {
                                ExprDef* const acts = ExprCreateActionList(NULL);
                                if (!acts) {
                                    /* TODO: Use Bison’s more appropriate YYNOMEM */
                                    YYABORT;
                                }
                                acts->common.next = &list.head->common;
                                list.head = acts;
                            }
                            $$ = list.head;
                        }
                        /*
                         * A list of *empty* keysym/action lists `{}`.
                         * There is not enough context here to decide if it is
                         * a `MultiKeySymList` or a `MultiActionList`.
                         */
                |       OBRACKET NoSymbolOrActionList CBRACKET
                        { $$ = ExprEmptyList(); }
                ;

/* A list of `{}`, which remains ambiguous until reaching a keysym or action list */
NoSymbolOrActionList
                :       NoSymbolOrActionList COMMA OBRACE CBRACE
                        { $$ = $1 + 1; }
                |       OBRACE CBRACE
                        { $$ = 1; }
                |       { $$ = 0; }
                ;

GroupCompatDecl :       GROUP Integer EQUALS Expr SEMI
                        { $$ = GroupCompatCreate($2, $4); }
                ;

ModMapDecl      :       MODIFIER_MAP Ident OBRACE KeyOrKeySymList CBRACE SEMI
                        { $$ = ModMapCreate($2, $4.head); }
                ;

KeyOrKeySymList :       KeyOrKeySymList COMMA KeyOrKeySym
                        { $$.head = $1.head; $$.last->common.next = &$3->common; $$.last = $3; }
                |       KeyOrKeySym
                        { $$.head = $$.last = $1; }
                ;

KeyOrKeySym     :       KEYNAME
                        { $$ = ExprCreateKeyName($1); }
                |       KeySym
                        { $$ = ExprCreateKeySym($1); }
                ;

LedMapDecl:             INDICATOR String OBRACE VarDeclList CBRACE SEMI
                        { $$ = LedMapCreate($2, $4.head); }
                ;

LedNameDecl:            INDICATOR Integer EQUALS Expr SEMI
                        { $$ = LedNameCreate($2, $4, false); }
                |       VIRTUAL INDICATOR Integer EQUALS Expr SEMI
                        { $$ = LedNameCreate($3, $5, true); }
                ;

ShapeDecl       :       SHAPE String OBRACE OutlineList CBRACE SEMI
                        { $$ = NULL; }
                |       SHAPE String OBRACE CoordList CBRACE SEMI
                        { (void) $4; $$ = NULL; }
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
                        { FreeStmt((ParseCommon *) $1); $$ = NULL; }
                |       DoodadDecl
                        { $$ = NULL; }
                |       LedMapDecl
                        { FreeStmt((ParseCommon *) $1); $$ = NULL; }
                |       OverlayDecl
                        { $$ = NULL; }
                ;

RowBody         :       RowBody RowBodyItem     { $$ = NULL;}
                |       RowBodyItem             { $$ = NULL; }
                ;

RowBodyItem     :       KEYS OBRACE Keys CBRACE SEMI { $$ = NULL; }
                |       VarDecl
                        { FreeStmt((ParseCommon *) $1); $$ = NULL; }
                ;

Keys            :       Keys COMMA Key          { $$ = NULL; }
                |       Key                     { $$ = NULL; }
                ;

Key             :       KEYNAME
                        { $$ = NULL; }
                |       OBRACE ExprList CBRACE
                        { FreeStmt((ParseCommon *) $2.head); $$ = NULL; }
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
                        { (void) $2; $$ = NULL; }
                |       Ident EQUALS OBRACE CoordList CBRACE
                        { (void) $4; $$ = NULL; }
                |       Ident EQUALS Expr
                        { FreeStmt((ParseCommon *) $3); $$ = NULL; }
                ;

CoordList       :       CoordList COMMA Coord
                        { (void) $1; (void) $3; $$ = NULL; }
                |       Coord
                        { (void) $1; $$ = NULL; }
                ;

Coord           :       OBRACKET SignedNumber COMMA SignedNumber CBRACKET
                        { $$ = NULL; }
                ;

DoodadDecl      :       DoodadType String OBRACE VarDeclList CBRACE SEMI
                        { FreeStmt((ParseCommon *) $4.head); $$ = NULL; }
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
                        { $$ = xkb_atom_intern_literal(param->ctx, "shape"); }
                |       ROW
                        { $$ = xkb_atom_intern_literal(param->ctx, "row"); }
                |       SECTION
                        { $$ = xkb_atom_intern_literal(param->ctx, "section"); }
                |       TEXT
                        { $$ = xkb_atom_intern_literal(param->ctx, "text"); }
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
                    parser_warn(param, XKB_LOG_MESSAGE_NO_ID,
                                "ignored unsupported legacy merge mode \"alternate\"");
                    $$ = MERGE_DEFAULT;
                }
                ;

ExprList        :       ExprList COMMA Expr
                        {
                            if ($3) {
                                if ($1.head) {
                                    $$.head = $1.head;
                                    $$.last->common.next = &$3->common;
                                    $$.last = $3;
                                } else {
                                    $$.head = $$.last = $3;
                                }
                            }
                        }
                |       Expr
                        { $$.head = $$.last = $1; }
                |       { $$.head = $$.last = NULL; }
                ;

Expr            :       Expr DIVIDE Expr
                        { $$ = ExprCreateBinary(STMT_EXPR_DIVIDE, $1, $3); }
                |       Expr PLUS Expr
                        { $$ = ExprCreateBinary(STMT_EXPR_ADD, $1, $3); }
                |       Expr MINUS Expr
                        { $$ = ExprCreateBinary(STMT_EXPR_SUBTRACT, $1, $3); }
                |       Expr TIMES Expr
                        { $$ = ExprCreateBinary(STMT_EXPR_MULTIPLY, $1, $3); }
                |       Lhs EQUALS Expr
                        { $$ = ExprCreateBinary(STMT_EXPR_ASSIGN, $1, $3); }
                |       Term
                        { $$ = $1; }
                ;

Term            :       MINUS Term
                        { $$ = ExprCreateUnary(STMT_EXPR_NEGATE, $2); }
                |       PLUS Term
                        { $$ = ExprCreateUnary(STMT_EXPR_UNARY_PLUS, $2); }
                |       EXCLAM Term
                        { $$ = ExprCreateUnary(STMT_EXPR_NOT, $2); }
                |       INVERT Term
                        { $$ = ExprCreateUnary(STMT_EXPR_INVERT, $2); }
                |       Lhs
                        { $$ = $1; }
                |       FieldSpec OPAREN ExprList CPAREN %prec OPAREN
                        { $$ = ExprCreateAction($1, $3.head); }
                |       Actions
                        { $$ = $1; }
                |       Terminal
                        { $$ = $1; }
                |       OPAREN Expr CPAREN
                        { $$ = $2; }
                ;

MultiActionList :       MultiActionList COMMA Action
                        {
                            ExprDef *expr = ExprCreateActionList($3);
                            $$ = $1;
                            $$.last->common.next = &expr->common; $$.last = expr;
                        }
                |       MultiActionList COMMA Actions
                        { $$ = $1; $$.last->common.next = &$3->common; $$.last = $3; }
                |       Action
                        { $$.head = $$.last = ExprCreateActionList($1); }
                |       NonEmptyActions
                        { $$.head = $$.last = $1; }
                ;

ActionList      :       ActionList COMMA Action
                        { $$ = $1; $$.last->common.next = &$3->common; $$.last = $3; }
                |       Action
                        { $$.head = $$.last = $1; }
                ;

NonEmptyActions :       OBRACE ActionList CBRACE
                        { $$ = ExprCreateActionList($2.head); }
                ;

Actions         :       NonEmptyActions
                        { $$ = $1; }
                |       OBRACE CBRACE
                        { $$ = ExprCreateActionList(NULL); }
                ;

Action          :       FieldSpec OPAREN ExprList CPAREN
                        { $$ = ExprCreateAction($1, $3.head); }
                ;

Lhs             :       FieldSpec
                        { $$ = ExprCreateIdent($1); }
                |       FieldSpec DOT FieldSpec
                        { $$ = ExprCreateFieldRef($1, $3); }
                |       FieldSpec OBRACKET Expr CBRACKET
                        { $$ = ExprCreateArrayRef(XKB_ATOM_NONE, $1, $3); }
                |       FieldSpec DOT FieldSpec OBRACKET Expr CBRACKET
                        { $$ = ExprCreateArrayRef($1, $3, $5); }
                ;

Terminal        :       String
                        { $$ = ExprCreateString($1); }
                |       Integer
                        { $$ = ExprCreateInteger($1); }
                |       Float
                        { $$ = ExprCreateFloat(/* Discard $1 */); }
                |       KEYNAME
                        { $$ = ExprCreateKeyName($1); }
                ;

MultiKeySymList :       MultiKeySymList COMMA KeySymLit
                        {
                            ExprDef *expr = ExprCreateKeySymList($3);
                            $$ = $1;
                            $$.last->common.next = &expr->common; $$.last = expr;
                        }
                |       MultiKeySymList COMMA KeySyms
                        { $$ = $1; $$.last->common.next = &$3->common; $$.last = $3; }
                |       KeySymLit
                        { $$.head = $$.last = ExprCreateKeySymList($1); }
                |       NonEmptyKeySyms
                        { $$.head = $$.last = $1; }
                ;

KeySymList      :       KeySymList COMMA KeySymLit
                        { $$ = ExprAppendKeySymList($1, $3); }
                |       KeySymList COMMA STRING
                        {
                            $$ = ExprKeySymListAppendString(param->scanner, $1, $3);
                            free($3);
                            if (!$$)
                                YYERROR;
                        }
                |       KeySymLit
                        {
                            $$ = ExprCreateKeySymList($1);
                            if (!$$)
                                YYERROR;
                        }
                |       STRING
                        {
                            $$ = ExprCreateKeySymList(XKB_KEY_NoSymbol);
                            if (!$$)
                                YYERROR;
                            $$ = ExprKeySymListAppendString(param->scanner, $$, $1);
                            free($1);
                            if (!$$)
                                YYERROR;
                        }
                ;

NonEmptyKeySyms :       OBRACE KeySymList CBRACE
                        { $$ = $2; }
                |       STRING
                        {
                            $$ = ExprCreateKeySymList(XKB_KEY_NoSymbol);
                            if (!$$)
                                YYERROR;
                            $$ = ExprKeySymListAppendString(param->scanner, $$, $1);
                            free($1);
                            if (!$$)
                                YYERROR;
                        }
                ;

KeySyms         :       NonEmptyKeySyms
                        { $$ = $1; }
                |       OBRACE CBRACE
                        { $$ = ExprCreateKeySymList(XKB_KEY_NoSymbol); }
                ;

KeySym          :       KeySymLit
                        { $$ = $1; }
                |       STRING
                        {
                            $$ = KeysymParseString(param->scanner, $1);
                            free($1);
                            if ($$ == XKB_KEY_NoSymbol)
                                YYERROR;
                        }
                ;

KeySymLit       :       IDENT
                        {
                            if (!resolve_keysym(param, $1, &$$)) {
                                parser_warn(
                                    param,
                                    XKB_WARNING_UNRECOGNIZED_KEYSYM,
                                    "unrecognized keysym \"%.*s\"",
                                    $1.len, $1.start
                                );
                                $$ = XKB_KEY_NoSymbol;
                            }
                        }
                        /* Handle keysym that is also a keyword  */
                |       SECTION { $$ = XKB_KEY_section; }
                |       Integer
                        {
                            if ($1 < XKB_KEYSYM_MIN) {
                                /* Negative value */
                                static_assert(XKB_KEYSYM_MIN == 0,
                                              "Keysyms are positive");
                                parser_warn(
                                    param,
                                    XKB_ERROR_INVALID_NUMERIC_KEYSYM,
                                    "unrecognized keysym \"-%#06"PRIx64"\""
                                    " (%"PRId64")",
                                    -$1, $1
                                );
                                $$ = XKB_KEY_NoSymbol;
                            }
                            else if ($1 < 10) {
                                /* Special case for digits 0..9:
                                 * map to XKB_KEY_0 .. XKB_KEY_9 */
                                $$ = XKB_KEY_0 + (xkb_keysym_t) $1;
                            }
                            else {
                                /* Any other numeric value */
                                if ($1 <= XKB_KEYSYM_MAX) {
                                    /*
                                     * Valid keysym
                                     * No normalization is performed and value
                                     * is used as is.
                                     */
                                    $$ = (xkb_keysym_t) $1;
                                    check_deprecated_keysyms(
                                        parser_warn, param, param->ctx,
                                        $$, NULL, $$, "%#06"PRIx32, "");
                                } else {
                                    /* Invalid keysym */
                                    parser_warn(
                                        param, XKB_ERROR_INVALID_NUMERIC_KEYSYM,
                                        "unrecognized keysym \"%#06"PRIx64"\" "
                                        "(%"PRId64")", $1, $1
                                    );
                                    $$ = XKB_KEY_NoSymbol;
                                }
                                parser_warn(
                                    param, XKB_WARNING_NUMERIC_KEYSYM,
                                    "numeric keysym \"%#06"PRIx64"\" (%"PRId64")",
                                    $1, $1
                                );
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

Ident           :       IDENT   { $$ = xkb_atom_intern(param->ctx, $1.start, $1.len); }
                |       DEFAULT { $$ = xkb_atom_intern_literal(param->ctx, "default"); }
                ;

String          :       STRING  { $$ = xkb_atom_intern(param->ctx, $1, strlen($1)); free($1); }
                ;

OptMapName      :       MapName { $$ = $1; }
                |               { $$ = NULL; }
                ;

MapName         :       STRING  { $$ = $1; }
                ;

%%

XkbFile *
parse(struct xkb_context *ctx, struct scanner *scanner, const char *map)
{
    int ret;
    XkbFile *first = NULL;
    struct parser_param param = {
        .scanner = scanner,
        .ctx = ctx,
        .rtrn = NULL,
        .more_maps = false,
    };

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
        param.rtrn = NULL;
    }

    if (ret != 0) {
        /* Some error happend; clear the Xkbfiles parsed so far */
        FreeXkbFile(first);
        FreeXkbFile(param.rtrn);
        return NULL;
    }

    if (first)
        log_vrb(ctx, 5,
                XKB_WARNING_MISSING_DEFAULT_SECTION,
                "No map in include statement, but \"%s\" contains several; "
                "Using first defined map, \"%s\"\n",
                scanner->file_name, safe_map_name(first));

    return first;
}
