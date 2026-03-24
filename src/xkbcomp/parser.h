/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */

/* DO NOT EDIT DIRECTLY: This file is generated from src/xkbcomp/parser.y. */

/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY__XKBCOMMON_SRC_XKBCOMP_PARSER_H_INCLUDED
# define YY__XKBCOMMON_SRC_XKBCOMP_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int _xkbcommon_debug;
#endif
/* "%code requires" blocks.  */
#line 14 "src/xkbcomp/parser.y"

#include "config.h"

#include "scanner-utils.h"
#include "xkbcomp/ast.h"

#line 56 "src/xkbcomp/parser.h"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    END_OF_FILE = 0,               /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    ERROR_TOK = 255,               /* "invalid token"  */
    XKB_KEYMAP = 1,                /* "xkb_keymap"  */
    XKB_KEYCODES = 2,              /* "xkb_keycodes"  */
    XKB_TYPES = 3,                 /* "xkb_types"  */
    XKB_SYMBOLS = 4,               /* "xkb_symbols"  */
    XKB_COMPATMAP = 5,             /* "xkb_compatibility"  */
    XKB_GEOMETRY = 6,              /* "xkb_geometry"  */
    XKB_SEMANTICS = 7,             /* "xkb_semantics"  */
    XKB_LAYOUT = 8,                /* "xkb_layout"  */
    INCLUDE = 10,                  /* "include"  */
    OVERRIDE = 11,                 /* "override"  */
    AUGMENT = 12,                  /* "augment"  */
    REPLACE = 13,                  /* "replace"  */
    ALTERNATE = 14,                /* "alternate"  */
    VIRTUAL_MODS = 20,             /* "virtual_modifiers"  */
    TYPE = 21,                     /* "type"  */
    INTERPRET = 22,                /* "interpret"  */
    ACTION_TOK = 23,               /* "action"  */
    KEY = 24,                      /* "key"  */
    ALIAS = 25,                    /* "alias"  */
    GROUP = 26,                    /* "group"  */
    MODIFIER_MAP = 27,             /* "modifier_map"  */
    INDICATOR = 28,                /* "indicator"  */
    SHAPE = 29,                    /* "shape"  */
    KEYS = 30,                     /* "keys"  */
    ROW = 31,                      /* "row"  */
    SECTION = 32,                  /* "section"  */
    OVERLAY = 33,                  /* "overlay"  */
    TEXT = 34,                     /* "text"  */
    OUTLINE = 35,                  /* "outline"  */
    SOLID = 36,                    /* "solid"  */
    LOGO = 37,                     /* "logo"  */
    VIRTUAL = 38,                  /* "virtual"  */
    EQUALS = 40,                   /* "="  */
    PLUS = 41,                     /* "+"  */
    MINUS = 42,                    /* "-"  */
    DIVIDE = 43,                   /* "/"  */
    TIMES = 44,                    /* "*"  */
    OBRACE = 45,                   /* "{"  */
    CBRACE = 46,                   /* "}"  */
    OPAREN = 47,                   /* "("  */
    CPAREN = 48,                   /* ")"  */
    OBRACKET = 49,                 /* "["  */
    CBRACKET = 50,                 /* "]"  */
    DOT = 51,                      /* "."  */
    COMMA = 52,                    /* ","  */
    SEMI = 53,                     /* ";"  */
    EXCLAM = 54,                   /* "!"  */
    INVERT = 55,                   /* "~"  */
    STRING = 60,                   /* "string literal"  */
    DECIMAL_DIGIT = 61,            /* "decimal digit"  */
    INTEGER = 62,                  /* "integer literal"  */
    FLOAT = 63,                    /* "float literal"  */
    IDENT = 64,                    /* "identifier"  */
    KEYNAME = 65,                  /* "key name"  */
    PARTIAL = 70,                  /* "partial"  */
    DEFAULT = 71,                  /* "default"  */
    HIDDEN = 72,                   /* "hidden"  */
    ALPHANUMERIC_KEYS = 73,        /* "alphanumeric_keys"  */
    MODIFIER_KEYS = 74,            /* "modifier_keys"  */
    KEYPAD_KEYS = 75,              /* "keypad_keys"  */
    FUNCTION_KEYS = 76,            /* "function_keys"  */
    ALTERNATE_GROUP = 77           /* "alternate_group"  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 170 "src/xkbcomp/parser.y"

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
        UnknownStatement *unknown;
        void            *geom;
        XkbFile         *file;
        struct { XkbFile *head; XkbFile *last; } fileList;

#line 171 "src/xkbcomp/parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif




int _xkbcommon_parse (struct parser_param *param);


#endif /* !YY__XKBCOMMON_SRC_XKBCOMP_PARSER_H_INCLUDED  */
