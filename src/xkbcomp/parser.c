/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */

/* DO NOT EDIT DIRECTLY: This file is generated from src/xkbcomp/parser.y. */

/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         _xkbcommon_parse
#define yylex           _xkbcommon_lex
#define yyerror         _xkbcommon_error
#define yydebug         _xkbcommon_debug
#define yynerrs         _xkbcommon_nerrs

/* First part of user prologue.  */
#line 22 "src/xkbcomp/parser.y"

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
    struct parser_keymap_config config;
    bool more_maps;
};

#define parser_log_with_code(param, level, verbosity, log_msg_id, fmt, ...)   \
    scanner_log_with_code((param)->scanner, level, verbosity, log_msg_id, fmt,\
                          ##__VA_ARGS__)

#define parser_err(param, error_id, fmt, ...) \
    scanner_err((param)->scanner, error_id, fmt, ##__VA_ARGS__)

#define parser_warn(param, warning_id, fmt, ...) \
    scanner_warn((param)->scanner, warning_id, fmt, ##__VA_ARGS__)

#define parser_vrb(param, verbosity, warning_id, fmt, ...) \
    scanner_vrb((param)->scanner, verbosity, warning_id, fmt, ##__VA_ARGS__)

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

#line 150 "src/xkbcomp/parser.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_ERROR_TOK = 3,                  /* "invalid token"  */
  YYSYMBOL_XKB_KEYMAP = 4,                 /* "xkb_keymap"  */
  YYSYMBOL_XKB_KEYCODES = 5,               /* "xkb_keycodes"  */
  YYSYMBOL_XKB_TYPES = 6,                  /* "xkb_types"  */
  YYSYMBOL_XKB_SYMBOLS = 7,                /* "xkb_symbols"  */
  YYSYMBOL_XKB_COMPATMAP = 8,              /* "xkb_compatibility"  */
  YYSYMBOL_XKB_GEOMETRY = 9,               /* "xkb_geometry"  */
  YYSYMBOL_XKB_SEMANTICS = 10,             /* "xkb_semantics"  */
  YYSYMBOL_XKB_LAYOUT = 11,                /* "xkb_layout"  */
  YYSYMBOL_INCLUDE = 12,                   /* "include"  */
  YYSYMBOL_OVERRIDE = 13,                  /* "override"  */
  YYSYMBOL_AUGMENT = 14,                   /* "augment"  */
  YYSYMBOL_REPLACE = 15,                   /* "replace"  */
  YYSYMBOL_ALTERNATE = 16,                 /* "alternate"  */
  YYSYMBOL_VIRTUAL_MODS = 17,              /* "virtual_modifiers"  */
  YYSYMBOL_TYPE = 18,                      /* "type"  */
  YYSYMBOL_INTERPRET = 19,                 /* "interpret"  */
  YYSYMBOL_ACTION_TOK = 20,                /* "action"  */
  YYSYMBOL_KEY = 21,                       /* "key"  */
  YYSYMBOL_ALIAS = 22,                     /* "alias"  */
  YYSYMBOL_GROUP = 23,                     /* "group"  */
  YYSYMBOL_MODIFIER_MAP = 24,              /* "modifier_map"  */
  YYSYMBOL_INDICATOR = 25,                 /* "indicator"  */
  YYSYMBOL_SHAPE = 26,                     /* "shape"  */
  YYSYMBOL_KEYS = 27,                      /* "keys"  */
  YYSYMBOL_ROW = 28,                       /* "row"  */
  YYSYMBOL_SECTION = 29,                   /* "section"  */
  YYSYMBOL_OVERLAY = 30,                   /* "overlay"  */
  YYSYMBOL_TEXT = 31,                      /* "text"  */
  YYSYMBOL_OUTLINE = 32,                   /* "outline"  */
  YYSYMBOL_SOLID = 33,                     /* "solid"  */
  YYSYMBOL_LOGO = 34,                      /* "logo"  */
  YYSYMBOL_VIRTUAL = 35,                   /* "virtual"  */
  YYSYMBOL_EQUALS = 36,                    /* "="  */
  YYSYMBOL_PLUS = 37,                      /* "+"  */
  YYSYMBOL_MINUS = 38,                     /* "-"  */
  YYSYMBOL_DIVIDE = 39,                    /* "/"  */
  YYSYMBOL_TIMES = 40,                     /* "*"  */
  YYSYMBOL_OBRACE = 41,                    /* "{"  */
  YYSYMBOL_CBRACE = 42,                    /* "}"  */
  YYSYMBOL_OPAREN = 43,                    /* "("  */
  YYSYMBOL_CPAREN = 44,                    /* ")"  */
  YYSYMBOL_OBRACKET = 45,                  /* "["  */
  YYSYMBOL_CBRACKET = 46,                  /* "]"  */
  YYSYMBOL_DOT = 47,                       /* "."  */
  YYSYMBOL_COMMA = 48,                     /* ","  */
  YYSYMBOL_SEMI = 49,                      /* ";"  */
  YYSYMBOL_EXCLAM = 50,                    /* "!"  */
  YYSYMBOL_INVERT = 51,                    /* "~"  */
  YYSYMBOL_STRING = 52,                    /* "string literal"  */
  YYSYMBOL_DECIMAL_DIGIT = 53,             /* "decimal digit"  */
  YYSYMBOL_INTEGER = 54,                   /* "integer literal"  */
  YYSYMBOL_FLOAT = 55,                     /* "float literal"  */
  YYSYMBOL_IDENT = 56,                     /* "identifier"  */
  YYSYMBOL_KEYNAME = 57,                   /* "key name"  */
  YYSYMBOL_PARTIAL = 58,                   /* "partial"  */
  YYSYMBOL_DEFAULT = 59,                   /* "default"  */
  YYSYMBOL_HIDDEN = 60,                    /* "hidden"  */
  YYSYMBOL_ALPHANUMERIC_KEYS = 61,         /* "alphanumeric_keys"  */
  YYSYMBOL_MODIFIER_KEYS = 62,             /* "modifier_keys"  */
  YYSYMBOL_KEYPAD_KEYS = 63,               /* "keypad_keys"  */
  YYSYMBOL_FUNCTION_KEYS = 64,             /* "function_keys"  */
  YYSYMBOL_ALTERNATE_GROUP = 65,           /* "alternate_group"  */
  YYSYMBOL_DEPRECATED = 66,                /* "deprecated"  */
  YYSYMBOL_YYACCEPT = 67,                  /* $accept  */
  YYSYMBOL_XkbFile = 68,                   /* XkbFile  */
  YYSYMBOL_XkbCompositeMap = 69,           /* XkbCompositeMap  */
  YYSYMBOL_XkbCompositeType = 70,          /* XkbCompositeType  */
  YYSYMBOL_XkbMapConfigList = 71,          /* XkbMapConfigList  */
  YYSYMBOL_XkbMapConfig = 72,              /* XkbMapConfig  */
  YYSYMBOL_FileType = 73,                  /* FileType  */
  YYSYMBOL_OptFlags = 74,                  /* OptFlags  */
  YYSYMBOL_Flags = 75,                     /* Flags  */
  YYSYMBOL_Flag = 76,                      /* Flag  */
  YYSYMBOL_DeclList = 77,                  /* DeclList  */
  YYSYMBOL_Decl = 78,                      /* Decl  */
  YYSYMBOL_VarDecl = 79,                   /* VarDecl  */
  YYSYMBOL_KeyNameDecl = 80,               /* KeyNameDecl  */
  YYSYMBOL_KeyAliasDecl = 81,              /* KeyAliasDecl  */
  YYSYMBOL_VModDecl = 82,                  /* VModDecl  */
  YYSYMBOL_VModDefList = 83,               /* VModDefList  */
  YYSYMBOL_VModDef = 84,                   /* VModDef  */
  YYSYMBOL_InterpretDecl = 85,             /* InterpretDecl  */
  YYSYMBOL_InterpretMatch = 86,            /* InterpretMatch  */
  YYSYMBOL_VarDeclList = 87,               /* VarDeclList  */
  YYSYMBOL_KeyTypeDecl = 88,               /* KeyTypeDecl  */
  YYSYMBOL_SymbolsDecl = 89,               /* SymbolsDecl  */
  YYSYMBOL_OptSymbolsBody = 90,            /* OptSymbolsBody  */
  YYSYMBOL_SymbolsBody = 91,               /* SymbolsBody  */
  YYSYMBOL_SymbolsVarDecl = 92,            /* SymbolsVarDecl  */
  YYSYMBOL_MultiKeySymOrActionList = 93,   /* MultiKeySymOrActionList  */
  YYSYMBOL_NoSymbolOrActionList = 94,      /* NoSymbolOrActionList  */
  YYSYMBOL_GroupCompatDecl = 95,           /* GroupCompatDecl  */
  YYSYMBOL_ModMapDecl = 96,                /* ModMapDecl  */
  YYSYMBOL_KeyOrKeySymList = 97,           /* KeyOrKeySymList  */
  YYSYMBOL_KeyOrKeySym = 98,               /* KeyOrKeySym  */
  YYSYMBOL_LedMapDecl = 99,                /* LedMapDecl  */
  YYSYMBOL_LedNameDecl = 100,              /* LedNameDecl  */
  YYSYMBOL_UnknownDecl = 101,              /* UnknownDecl  */
  YYSYMBOL_UnknownCompoundStatementDecl = 102, /* UnknownCompoundStatementDecl  */
  YYSYMBOL_ShapeDecl = 103,                /* ShapeDecl  */
  YYSYMBOL_SectionDecl = 104,              /* SectionDecl  */
  YYSYMBOL_SectionBody = 105,              /* SectionBody  */
  YYSYMBOL_SectionBodyItem = 106,          /* SectionBodyItem  */
  YYSYMBOL_RowBody = 107,                  /* RowBody  */
  YYSYMBOL_RowBodyItem = 108,              /* RowBodyItem  */
  YYSYMBOL_Keys = 109,                     /* Keys  */
  YYSYMBOL_Key = 110,                      /* Key  */
  YYSYMBOL_OverlayDecl = 111,              /* OverlayDecl  */
  YYSYMBOL_OverlayKeyList = 112,           /* OverlayKeyList  */
  YYSYMBOL_OverlayKey = 113,               /* OverlayKey  */
  YYSYMBOL_OutlineList = 114,              /* OutlineList  */
  YYSYMBOL_OutlineInList = 115,            /* OutlineInList  */
  YYSYMBOL_CoordList = 116,                /* CoordList  */
  YYSYMBOL_Coord = 117,                    /* Coord  */
  YYSYMBOL_DoodadDecl = 118,               /* DoodadDecl  */
  YYSYMBOL_DoodadType = 119,               /* DoodadType  */
  YYSYMBOL_FieldSpec = 120,                /* FieldSpec  */
  YYSYMBOL_Element = 121,                  /* Element  */
  YYSYMBOL_OptMergeMode = 122,             /* OptMergeMode  */
  YYSYMBOL_MergeMode = 123,                /* MergeMode  */
  YYSYMBOL_ExprList = 124,                 /* ExprList  */
  YYSYMBOL_Expr = 125,                     /* Expr  */
  YYSYMBOL_Term = 126,                     /* Term  */
  YYSYMBOL_MultiActionList = 127,          /* MultiActionList  */
  YYSYMBOL_ActionList = 128,               /* ActionList  */
  YYSYMBOL_NonEmptyActions = 129,          /* NonEmptyActions  */
  YYSYMBOL_Actions = 130,                  /* Actions  */
  YYSYMBOL_Action = 131,                   /* Action  */
  YYSYMBOL_Lhs = 132,                      /* Lhs  */
  YYSYMBOL_OptTerminal = 133,              /* OptTerminal  */
  YYSYMBOL_Terminal = 134,                 /* Terminal  */
  YYSYMBOL_MultiKeySymList = 135,          /* MultiKeySymList  */
  YYSYMBOL_KeySymList = 136,               /* KeySymList  */
  YYSYMBOL_NonEmptyKeySyms = 137,          /* NonEmptyKeySyms  */
  YYSYMBOL_KeySyms = 138,                  /* KeySyms  */
  YYSYMBOL_KeySym = 139,                   /* KeySym  */
  YYSYMBOL_KeySymLit = 140,                /* KeySymLit  */
  YYSYMBOL_SignedNumber = 141,             /* SignedNumber  */
  YYSYMBOL_Number = 142,                   /* Number  */
  YYSYMBOL_Float = 143,                    /* Float  */
  YYSYMBOL_Integer = 144,                  /* Integer  */
  YYSYMBOL_KeyCode = 145,                  /* KeyCode  */
  YYSYMBOL_Ident = 146,                    /* Ident  */
  YYSYMBOL_String = 147,                   /* String  */
  YYSYMBOL_OptMapName = 148,               /* OptMapName  */
  YYSYMBOL_MapName = 149                   /* MapName  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  18
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   907

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  67
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  83
/* YYNRULES -- Number of rules.  */
#define YYNRULES  221
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  386

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   257


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     4,     5,     6,     7,     8,     9,    10,    11,     2,
      12,    13,    14,    15,    16,     2,     2,     2,     2,     2,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,     2,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,     2,     2,     2,     2,
      52,    53,    54,    55,    56,    57,     2,     2,     2,     2,
      58,    59,    60,    61,    62,    63,    64,    65,    66,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     3,     1,     2
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   275,   275,   277,   279,   283,   289,   290,   291,   297,
     309,   312,   325,   326,   327,   328,   329,   332,   333,   336,
     337,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   366,   381,   391,   394,   400,   405,   410,   415,   420,
     425,   430,   435,   440,   445,   446,   447,   448,   450,   452,
     459,   461,   463,   467,   471,   475,   479,   481,   485,   487,
     491,   497,   499,   503,   515,   518,   524,   530,   531,   534,
     536,   540,   541,   542,   543,   544,   559,   561,   579,   581,
     603,   609,   611,   613,   616,   620,   624,   626,   630,   632,
     636,   640,   642,   646,   655,   663,   665,   669,   673,   674,
     677,   679,   681,   683,   685,   689,   690,   693,   694,   698,
     699,   702,   704,   708,   712,   713,   716,   719,   721,   725,
     727,   729,   733,   735,   739,   743,   747,   748,   749,   750,
     753,   754,   757,   759,   761,   763,   765,   767,   769,   771,
     773,   775,   777,   781,   782,   785,   786,   787,   788,   789,
     801,   813,   815,   818,   820,   822,   824,   826,   828,   832,
     834,   836,   838,   840,   842,   844,   846,   848,   852,   858,
     860,   862,   866,   868,   872,   876,   878,   882,   886,   888,
     890,   892,   896,   898,   901,   903,   905,   907,   911,   917,
     919,   921,   925,   927,   934,   940,   952,   954,   966,   968,
     972,   974,   983,   996,   997,  1006,  1066,  1067,  1070,  1071,
    1072,  1075,  1078,  1079,  1082,  1083,  1086,  1087,  1090,  1093,
    1094,  1097
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  static const char *const yy_sname[] =
  {
  "end of file", "error", "invalid token", "invalid token", "xkb_keymap",
  "xkb_keycodes", "xkb_types", "xkb_symbols", "xkb_compatibility",
  "xkb_geometry", "xkb_semantics", "xkb_layout", "include", "override",
  "augment", "replace", "alternate", "virtual_modifiers", "type",
  "interpret", "action", "key", "alias", "group", "modifier_map",
  "indicator", "shape", "keys", "row", "section", "overlay", "text",
  "outline", "solid", "logo", "virtual", "=", "+", "-", "/", "*", "{", "}",
  "(", ")", "[", "]", ".", ",", ";", "!", "~", "string literal",
  "decimal digit", "integer literal", "float literal", "identifier",
  "key name", "partial", "default", "hidden", "alphanumeric_keys",
  "modifier_keys", "keypad_keys", "function_keys", "alternate_group",
  "deprecated", "$accept", "XkbFile", "XkbCompositeMap",
  "XkbCompositeType", "XkbMapConfigList", "XkbMapConfig", "FileType",
  "OptFlags", "Flags", "Flag", "DeclList", "Decl", "VarDecl",
  "KeyNameDecl", "KeyAliasDecl", "VModDecl", "VModDefList", "VModDef",
  "InterpretDecl", "InterpretMatch", "VarDeclList", "KeyTypeDecl",
  "SymbolsDecl", "OptSymbolsBody", "SymbolsBody", "SymbolsVarDecl",
  "MultiKeySymOrActionList", "NoSymbolOrActionList", "GroupCompatDecl",
  "ModMapDecl", "KeyOrKeySymList", "KeyOrKeySym", "LedMapDecl",
  "LedNameDecl", "UnknownDecl", "UnknownCompoundStatementDecl",
  "ShapeDecl", "SectionDecl", "SectionBody", "SectionBodyItem", "RowBody",
  "RowBodyItem", "Keys", "Key", "OverlayDecl", "OverlayKeyList",
  "OverlayKey", "OutlineList", "OutlineInList", "CoordList", "Coord",
  "DoodadDecl", "DoodadType", "FieldSpec", "Element", "OptMergeMode",
  "MergeMode", "ExprList", "Expr", "Term", "MultiActionList", "ActionList",
  "NonEmptyActions", "Actions", "Action", "Lhs", "OptTerminal", "Terminal",
  "MultiKeySymList", "KeySymList", "NonEmptyKeySyms", "KeySyms", "KeySym",
  "KeySymLit", "SignedNumber", "Number", "Float", "Integer", "KeyCode",
  "Ident", "String", "OptMapName", "MapName", YY_NULLPTR
  };
  return yy_sname[yysymbol];
}
#endif

#define YYPACT_NINF (-282)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-217)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
       7,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,    10,  -282,  -282,   581,   465,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,   -20,   -20,  -282,
    -282,   -24,  -282,    20,  -282,  -282,   836,    11,    -6,  -282,
     297,  -282,  -282,  -282,  -282,  -282,    25,  -282,   290,    52,
    -282,  -282,    35,    59,   172,  -282,    66,    94,    77,    35,
     142,    59,  -282,    59,   113,  -282,  -282,  -282,   167,    35,
     369,   163,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,    59,
      73,  -282,   168,   162,  -282,  -282,   112,  -282,   184,  -282,
     182,  -282,  -282,  -282,  -282,  -282,   186,   201,  -282,   202,
     213,  -282,  -282,   238,   222,   248,   234,   237,   251,    77,
     244,  -282,  -282,   258,   281,  -282,  -282,  -282,   115,   289,
     377,   308,   377,  -282,    35,  -282,   377,  -282,  -282,   377,
     600,   278,   377,    60,   377,  -282,    96,    26,   312,  -282,
    -282,   377,  -282,  -282,   292,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,   377,   377,   828,   377,   377,
     377,   -14,     2,  -282,  -282,  -282,   323,  -282,  -282,   317,
     233,  -282,   228,   642,   657,   228,   481,    35,   329,   324,
    -282,  -282,   338,   -30,   316,   305,  -282,   -27,  -282,  -282,
     341,   699,   331,    43,    42,  -282,    45,  -282,   356,    59,
     352,    59,  -282,  -282,   461,  -282,  -282,  -282,   377,   714,
     414,  -282,   756,  -282,  -282,  -282,  -282,   351,    67,  -282,
     596,  -282,  -282,   377,   377,   377,   377,   377,  -282,   377,
     377,  -282,   355,  -282,   358,   366,   523,  -282,   368,   170,
     196,  -282,  -282,   239,  -282,  -282,  -282,   370,   600,   332,
    -282,  -282,   376,    60,  -282,   386,    92,   153,  -282,  -282,
    -282,   393,  -282,   397,   -22,   400,   331,   419,   776,   417,
     406,  -282,   428,   410,  -282,   412,   377,  -282,   308,  -282,
      58,   228,   149,   149,  -282,  -282,   228,    40,  -282,  -282,
    -282,  -282,   108,  -282,  -282,   543,  -282,   848,  -282,   161,
    -282,  -282,  -282,   228,  -282,  -282,  -282,  -282,  -282,    43,
    -282,  -282,  -282,  -282,   799,   228,   423,  -282,   227,  -282,
     426,  -282,  -282,  -282,  -282,    95,  -282,  -282,   377,  -282,
    -282,   208,   585,   242,   320,  -282,  -282,   180,  -282,  -282,
    -282,   442,   128,   -35,   447,  -282,   462,   130,  -282,  -282,
     228,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,   377,
    -282,   131,  -282,  -282,   440,   459,   426,   139,   464,   -35,
    -282,  -282,  -282,  -282,  -282,  -282
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      18,     4,    30,    21,    22,    23,    24,    25,    26,    27,
      28,    29,     0,     2,     3,     0,    17,    20,     1,     6,
      12,    13,    15,    14,    16,     7,     8,   220,   220,    19,
     221,     0,   219,     0,    10,    33,    18,   144,     0,     9,
       0,   145,   147,   146,   148,   149,     0,    31,     0,   143,
       5,    11,     0,   134,   133,   132,   135,     0,   136,   137,
     138,   139,   140,   141,   142,   127,   128,   129,     0,     0,
     216,     0,   217,    34,    36,    37,    32,    35,    38,    39,
      41,    40,    42,    43,    47,    48,    44,    45,    46,     0,
     178,   131,     0,   130,    49,   216,     0,    57,    58,   218,
       0,   203,   201,   204,   205,   202,     0,    62,   200,     0,
       0,   213,   212,     0,     0,     0,     0,     0,     0,     0,
       0,   211,   187,     0,   182,   186,   185,   184,     0,     0,
       0,     0,     0,    51,     0,    55,     0,    64,    64,     0,
      68,     0,     0,     0,     0,    64,     0,     0,     0,    52,
      64,     0,   215,   214,     0,    64,   134,   133,   135,   136,
     137,   138,   139,   141,   142,     0,     0,     0,     0,     0,
       0,   178,     0,   158,   175,   165,   163,   166,   130,   179,
       0,    56,    59,     0,     0,    61,    83,     0,     0,    67,
      70,    75,     0,   130,     0,     0,    88,     0,    87,    89,
       0,     0,     0,     0,     0,   118,     0,   123,     0,   138,
     140,     0,   101,   103,     0,    99,   104,   102,     0,     0,
       0,    53,     0,   160,   163,   159,   176,     0,     0,   173,
       0,   161,   162,   152,     0,     0,     0,     0,   180,     0,
       0,    50,     0,    63,     0,   203,     0,   197,   202,     0,
       0,   171,   170,     0,   191,   190,    74,     0,     0,     0,
      54,    84,     0,     0,    91,     0,     0,     0,   209,   210,
     208,     0,   207,     0,     0,     0,     0,     0,     0,     0,
       0,    98,     0,     0,    93,     0,   152,   174,     0,   167,
       0,   151,   154,   155,   153,   156,   157,     0,    65,    60,
      82,   195,     0,   194,    80,     0,    78,     0,    76,     0,
      66,    69,    72,    71,    85,    86,    90,   119,   206,     0,
      95,   117,    96,   122,     0,   121,     0,   108,     0,   106,
       0,    97,    92,    94,   125,     0,   172,   164,     0,   181,
     196,     0,     0,     0,     0,   169,   168,     0,   198,   189,
     188,     0,     0,     0,     0,   105,     0,     0,   115,   177,
     150,   193,   192,    81,    79,    77,   199,   124,   120,   152,
     111,     0,   110,   100,     0,     0,     0,     0,     0,     0,
     116,   113,   114,   112,   107,   109
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -282,  -282,  -282,  -282,  -282,   478,  -282,   479,  -282,   500,
    -282,  -282,   -47,  -282,  -282,  -282,  -282,   384,  -282,  -282,
       4,  -282,  -282,  -282,  -282,   261,   273,  -282,  -282,  -282,
    -282,   275,   488,  -282,  -282,  -282,  -282,  -282,  -282,   325,
    -282,   217,  -282,   171,  -282,  -282,   177,  -282,   282,  -198,
     279,   509,  -282,   -48,  -282,  -282,  -282,  -281,    61,   247,
     253,  -282,  -178,   252,  -183,   -37,  -282,   490,   265,  -282,
     264,  -282,   524,  -184,   262,   313,  -282,   -44,  -282,   -39,
     -25,   555,  -282
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    12,    13,    27,    36,    14,    28,    15,    16,    17,
      37,    47,   243,    74,    75,    76,    96,    97,    77,   106,
     183,    78,    79,   188,   189,   190,   191,   249,    80,    81,
     197,   198,   213,    83,    84,    85,    86,    87,   214,   215,
     328,   329,   371,   372,   216,   357,   358,   204,   205,   206,
     207,   217,    89,   171,    91,    48,    49,   290,   291,   173,
     250,   228,   174,   175,   229,   176,   123,   177,   253,   302,
     254,   349,   199,   108,   271,   272,   125,   126,   154,   178,
     127,    31,    32
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      90,    73,   255,   252,   266,   335,   369,     1,   251,    93,
      18,    92,   -73,    98,   113,   262,   115,    34,   -73,   202,
     114,   263,   370,    41,    42,    43,    44,    45,   100,   233,
     120,   130,    30,   131,    95,   116,   117,    72,   118,   234,
     235,   236,   237,    50,   156,   157,    55,   158,   238,   159,
     160,   209,   162,    46,   210,   163,   211,    64,    65,    66,
      67,    35,   303,     2,   129,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    51,   148,    69,   234,   235,   236,
     237,   267,    95,   179,   273,    72,   339,   275,   377,   101,
     274,    95,    90,   276,    72,    98,   268,   269,   270,    90,
     212,   193,   337,   192,    94,   336,   338,   208,    93,   287,
      92,    99,   102,   103,   104,   288,   105,   196,   130,   227,
     131,   255,   252,   109,   346,   350,   352,   251,   224,   224,
     111,   112,   224,   224,   317,    90,    90,   202,   227,   359,
     276,   203,   184,   338,    93,    93,    92,    92,   256,   201,
     340,   110,    95,    90,   219,    72,   341,   362,   303,   222,
     134,   135,    93,   303,    92,  -126,    90,   212,   152,   153,
     368,    90,   375,   378,    90,    93,   276,    92,   376,   379,
      93,   383,    92,    93,   116,    92,   279,   338,   236,   237,
     101,   172,   119,   180,    99,   111,   112,   182,   227,   128,
     185,   101,   347,   195,   132,   200,   268,   269,   270,   101,
      90,   133,   220,   247,   103,   104,   304,   105,   305,   193,
     136,   192,   366,   137,   102,   103,   104,   138,   105,   230,
      90,   327,   301,   103,   104,   208,   105,   101,   139,    93,
     227,    92,   306,   140,   307,   156,   157,    55,   158,   141,
     159,   160,   161,   162,   326,    62,   163,   227,   164,   227,
     361,   103,   104,   143,   105,   234,   235,   236,   237,   354,
     234,   235,   236,   237,   142,   145,   227,    69,   146,   282,
      90,   327,   241,    95,   144,   308,    72,   309,   364,    93,
     307,    92,   147,   149,   227,   292,   293,   294,   295,   150,
     296,   297,    20,    21,    22,    23,    24,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,   151,    62,    63,
     313,    64,    65,    66,    67,    68,   156,   157,    55,   158,
     155,   159,   160,   161,   162,   194,    62,   163,   325,   164,
      69,   221,   234,   235,   236,   237,    70,    71,   218,    72,
     156,   157,    55,   158,   261,   159,   160,   161,   162,   239,
      62,   163,   240,   164,    95,   260,   365,    72,   309,   165,
     166,   257,   258,   167,   259,   168,   203,   186,   234,   235,
     236,   237,   169,   170,    99,   111,   112,   121,    95,   122,
     264,    72,   277,   278,   286,   156,   157,    55,   158,   360,
     159,   160,   161,   162,   298,    62,   163,   299,   164,  -141,
    -183,  -216,   223,   225,   165,   166,   231,   232,   167,   310,
     168,    99,   111,   112,   121,   314,   122,   169,   170,    99,
     111,   112,   121,    95,   122,   316,    72,   156,   157,    55,
     158,   319,   159,   160,   161,   162,   320,    62,   163,   322,
     164,   234,   235,   236,   237,   331,   165,   166,   330,   333,
     324,   334,   168,   284,   353,   234,   235,   236,   237,   169,
     170,    99,   111,   112,   121,    95,   122,   332,    72,   156,
     157,    55,   158,   356,   159,   160,   209,   162,   367,   210,
     163,   211,    64,    65,    66,    67,   373,   380,   374,   156,
     157,    55,   158,   280,   159,   160,   161,   162,   381,    62,
     245,    69,   164,   384,    39,    40,    29,    95,   181,   311,
      72,     2,   246,     3,     4,     5,     6,     7,     8,     9,
      10,    11,   312,   247,   103,   104,    82,   248,   315,   281,
      72,   156,   157,    55,   158,   355,   159,   160,   161,   162,
     385,    62,   245,   382,   164,   323,   321,    88,   343,   345,
     124,   156,   157,    55,   158,   300,   159,   160,   161,   162,
     344,    62,   245,   348,   164,   301,   103,   104,   107,   248,
     318,   351,    72,    33,   342,    19,    20,    21,    22,    23,
      24,    25,    26,     0,     0,   247,   103,   104,     0,   248,
       0,     0,    72,   156,   157,    55,   158,     0,   159,   160,
     161,   162,     0,    62,   245,     0,   164,     0,   156,   157,
      55,   158,     0,   159,   160,   161,   162,   363,    62,   163,
       0,   164,     0,   234,   235,   236,   237,   301,   103,   104,
     289,   248,     0,     0,    72,   186,     0,     0,     0,     0,
     187,     0,     0,     0,     0,     0,    95,     0,     0,    72,
     156,   157,    55,   158,     0,   159,   160,   161,   162,     0,
      62,   163,     0,   164,     0,   156,   157,    55,   158,     0,
     159,   160,   161,   162,   242,    62,   163,     0,   164,     0,
       0,     0,    69,     0,     0,     0,     0,     0,    95,   244,
       0,    72,     0,     0,     0,     0,     0,    69,     0,     0,
       0,     0,     0,    95,     0,     0,    72,   156,   157,    55,
     158,     0,   159,   160,   161,   162,     0,    62,   163,     0,
     164,     0,   156,   157,    55,   158,     0,   159,   160,   161,
     162,   265,    62,   163,     0,   164,     0,     0,     0,    69,
       0,     0,     0,     0,     0,    95,   283,     0,    72,     0,
       0,     0,     0,     0,    69,     0,     0,     0,     0,     0,
      95,     0,     0,    72,   156,   157,    55,   158,     0,   159,
     160,   161,   162,     0,    62,   163,     0,   164,     0,     0,
       0,     0,     0,     0,   156,   157,    55,   158,   285,   159,
     160,   161,   162,   326,    62,   163,    69,   164,     0,     0,
       0,     0,    95,     0,     0,    72,     0,   156,   157,    55,
     158,     0,   159,   160,   161,   162,    69,    62,   163,     0,
     164,     0,    95,     0,     0,    72,     0,     0,     0,     0,
       0,   226,     0,     0,   203,     0,   156,   157,    55,   158,
       0,   159,   160,   161,   162,    95,    62,   163,    72,   164,
       0,     0,     0,     0,     0,     0,   156,   157,    55,   158,
     226,   159,   160,   161,   162,     0,    62,   163,    38,   164,
       0,     0,     0,     0,    95,     0,     0,    72,     0,   167,
       0,     0,     2,     0,     3,     4,     5,     6,     7,     8,
       9,    10,    11,     0,    95,     0,     0,    72
};

static const yytype_int16 yycheck[] =
{
      48,    48,   186,   186,   202,   286,    41,     0,   186,    48,
       0,    48,    42,    52,    58,    42,    60,    41,    48,    41,
      59,    48,    57,    12,    13,    14,    15,    16,    53,    43,
      69,    45,    52,    47,    56,    60,    61,    59,    63,    37,
      38,    39,    40,    49,    18,    19,    20,    21,    46,    23,
      24,    25,    26,    42,    28,    29,    30,    31,    32,    33,
      34,    41,   246,    56,    89,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    49,   119,    50,    37,    38,    39,
      40,    38,    56,   131,    42,    59,    46,    42,   369,    29,
      48,    56,   140,    48,    59,   134,    53,    54,    55,   147,
     147,   140,    44,   140,    52,   288,    48,   146,   147,    42,
     147,    52,    52,    53,    54,    48,    56,    57,    45,   167,
      47,   305,   305,    57,   307,   309,   324,   305,   165,   166,
      53,    54,   169,   170,    42,   183,   184,    41,   186,    44,
      48,    45,   138,    48,   183,   184,   183,   184,   187,   145,
      42,    57,    56,   201,   150,    59,    48,   341,   342,   155,
      48,    49,   201,   347,   201,    52,   214,   214,    53,    54,
      42,   219,    42,    42,   222,   214,    48,   214,    48,    48,
     219,    42,   219,   222,   209,   222,   211,    48,    39,    40,
      29,   130,    25,   132,    52,    53,    54,   136,   246,    36,
     139,    29,    41,   142,    36,   144,    53,    54,    55,    29,
     258,    49,   151,    52,    53,    54,    46,    56,    48,   258,
      36,   258,    42,    41,    52,    53,    54,    41,    56,   168,
     278,   278,    52,    53,    54,   274,    56,    29,    37,   278,
     288,   278,    46,    41,    48,    18,    19,    20,    21,    36,
      23,    24,    25,    26,    27,    28,    29,   305,    31,   307,
      52,    53,    54,    41,    56,    37,    38,    39,    40,    42,
      37,    38,    39,    40,    36,    41,   324,    50,    41,   218,
     328,   328,    49,    56,    36,    46,    59,    48,    46,   328,
      48,   328,    41,    49,   342,   234,   235,   236,   237,    41,
     239,   240,     5,     6,     7,     8,     9,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    36,    28,    29,
     259,    31,    32,    33,    34,    35,    18,    19,    20,    21,
      41,    23,    24,    25,    26,    57,    28,    29,   277,    31,
      50,    49,    37,    38,    39,    40,    56,    57,    36,    59,
      18,    19,    20,    21,    49,    23,    24,    25,    26,    36,
      28,    29,    45,    31,    56,    49,    46,    59,    48,    37,
      38,    42,    48,    41,    36,    43,    45,    45,    37,    38,
      39,    40,    50,    51,    52,    53,    54,    55,    56,    57,
      49,    59,    36,    41,    43,    18,    19,    20,    21,   338,
      23,    24,    25,    26,    49,    28,    29,    49,    31,    43,
      41,    43,   165,   166,    37,    38,   169,   170,    41,    49,
      43,    52,    53,    54,    55,    49,    57,    50,    51,    52,
      53,    54,    55,    56,    57,    49,    59,    18,    19,    20,
      21,    48,    23,    24,    25,    26,    49,    28,    29,    49,
      31,    37,    38,    39,    40,    49,    37,    38,    41,    49,
      41,    49,    43,    49,    41,    37,    38,    39,    40,    50,
      51,    52,    53,    54,    55,    56,    57,    49,    59,    18,
      19,    20,    21,    57,    23,    24,    25,    26,    46,    28,
      29,    30,    31,    32,    33,    34,    49,    57,    36,    18,
      19,    20,    21,    42,    23,    24,    25,    26,    49,    28,
      29,    50,    31,    49,    36,    36,    16,    56,   134,   258,
      59,    56,    41,    58,    59,    60,    61,    62,    63,    64,
      65,    66,   259,    52,    53,    54,    48,    56,   263,   214,
      59,    18,    19,    20,    21,   328,    23,    24,    25,    26,
     379,    28,    29,   376,    31,   276,   274,    48,   305,   307,
      70,    18,    19,    20,    21,    42,    23,    24,    25,    26,
     305,    28,    29,   309,    31,    52,    53,    54,    54,    56,
     267,   319,    59,    28,    41,     4,     5,     6,     7,     8,
       9,    10,    11,    -1,    -1,    52,    53,    54,    -1,    56,
      -1,    -1,    59,    18,    19,    20,    21,    -1,    23,    24,
      25,    26,    -1,    28,    29,    -1,    31,    -1,    18,    19,
      20,    21,    -1,    23,    24,    25,    26,    42,    28,    29,
      -1,    31,    -1,    37,    38,    39,    40,    52,    53,    54,
      44,    56,    -1,    -1,    59,    45,    -1,    -1,    -1,    -1,
      50,    -1,    -1,    -1,    -1,    -1,    56,    -1,    -1,    59,
      18,    19,    20,    21,    -1,    23,    24,    25,    26,    -1,
      28,    29,    -1,    31,    -1,    18,    19,    20,    21,    -1,
      23,    24,    25,    26,    42,    28,    29,    -1,    31,    -1,
      -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,    56,    42,
      -1,    59,    -1,    -1,    -1,    -1,    -1,    50,    -1,    -1,
      -1,    -1,    -1,    56,    -1,    -1,    59,    18,    19,    20,
      21,    -1,    23,    24,    25,    26,    -1,    28,    29,    -1,
      31,    -1,    18,    19,    20,    21,    -1,    23,    24,    25,
      26,    42,    28,    29,    -1,    31,    -1,    -1,    -1,    50,
      -1,    -1,    -1,    -1,    -1,    56,    42,    -1,    59,    -1,
      -1,    -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,
      56,    -1,    -1,    59,    18,    19,    20,    21,    -1,    23,
      24,    25,    26,    -1,    28,    29,    -1,    31,    -1,    -1,
      -1,    -1,    -1,    -1,    18,    19,    20,    21,    42,    23,
      24,    25,    26,    27,    28,    29,    50,    31,    -1,    -1,
      -1,    -1,    56,    -1,    -1,    59,    -1,    18,    19,    20,
      21,    -1,    23,    24,    25,    26,    50,    28,    29,    -1,
      31,    -1,    56,    -1,    -1,    59,    -1,    -1,    -1,    -1,
      -1,    42,    -1,    -1,    45,    -1,    18,    19,    20,    21,
      -1,    23,    24,    25,    26,    56,    28,    29,    59,    31,
      -1,    -1,    -1,    -1,    -1,    -1,    18,    19,    20,    21,
      42,    23,    24,    25,    26,    -1,    28,    29,    42,    31,
      -1,    -1,    -1,    -1,    56,    -1,    -1,    59,    -1,    41,
      -1,    -1,    56,    -1,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    -1,    56,    -1,    -1,    59
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     0,    56,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    68,    69,    72,    74,    75,    76,     0,     4,
       5,     6,     7,     8,     9,    10,    11,    70,    73,    76,
      52,   148,   149,   148,    41,    41,    71,    77,    42,    72,
      74,    12,    13,    14,    15,    16,    42,    78,   122,   123,
      49,    49,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    28,    29,    31,    32,    33,    34,    35,    50,
      56,    57,    59,    79,    80,    81,    82,    85,    88,    89,
      95,    96,    99,   100,   101,   102,   103,   104,   118,   119,
     120,   121,   132,   146,    52,    56,    83,    84,   146,    52,
     147,    29,    52,    53,    54,    56,    86,   139,   140,    57,
      57,    53,    54,   144,   146,   144,   147,   147,   147,    25,
     146,    55,    57,   133,   134,   143,   144,   147,    36,   147,
      45,    47,    36,    49,    48,    49,    36,    41,    41,    37,
      41,    36,    36,    41,    36,    41,    41,    41,   144,    49,
      41,    36,    53,    54,   145,    41,    18,    19,    21,    23,
      24,    25,    26,    29,    31,    37,    38,    41,    43,    50,
      51,   120,   125,   126,   129,   130,   132,   134,   146,   120,
     125,    84,   125,    87,    87,   125,    45,    50,    90,    91,
      92,    93,   132,   146,    57,   125,    57,    97,    98,   139,
     125,    87,    41,    45,   114,   115,   116,   117,   146,    25,
      28,    30,    79,    99,   105,   106,   111,   118,    36,    87,
     125,    49,    87,   126,   132,   126,    42,   120,   128,   131,
     125,   126,   126,    43,    37,    38,    39,    40,    46,    36,
      45,    49,    42,    79,    42,    29,    41,    52,    56,    94,
     127,   129,   131,   135,   137,   140,   146,    42,    48,    36,
      49,    49,    42,    48,    49,    42,   116,    38,    53,    54,
      55,   141,   142,    42,    48,    42,    48,    36,    41,   147,
      42,   106,   125,    42,    49,    42,    43,    42,    48,    44,
     124,   125,   125,   125,   125,   125,   125,   125,    49,    49,
      42,    52,   136,   140,    46,    48,    46,    48,    46,    48,
      49,    92,    93,   125,    49,    98,    49,    42,   142,    48,
      49,   115,    49,   117,    41,   125,    27,    79,   107,   108,
      41,    49,    49,    49,    49,   124,   131,    44,    48,    46,
      42,    48,    41,   127,   135,   130,   131,    41,   137,   138,
     140,   141,   116,    41,    42,   108,    57,   112,   113,    44,
     125,    52,   140,    42,    46,    46,    42,    46,    42,    41,
      57,   109,   110,    49,    36,    42,    48,   124,    42,    48,
      57,    49,   113,    42,    49,   110
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    67,    68,    68,    68,    69,    70,    70,    70,    71,
      71,    72,    73,    73,    73,    73,    73,    74,    74,    75,
      75,    76,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    77,    77,    77,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      79,    79,    79,    80,    81,    82,    83,    83,    84,    84,
      85,    86,    86,    87,    87,    88,    89,    90,    90,    91,
      91,    92,    92,    92,    92,    92,    93,    93,    93,    93,
      93,    94,    94,    94,    95,    96,    97,    97,    98,    98,
      99,   100,   100,   101,   102,   103,   103,   104,   105,   105,
     106,   106,   106,   106,   106,   107,   107,   108,   108,   109,
     109,   110,   110,   111,   112,   112,   113,   114,   114,   115,
     115,   115,   116,   116,   117,   118,   119,   119,   119,   119,
     120,   120,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   121,   122,   122,   123,   123,   123,   123,   123,
     124,   124,   124,   125,   125,   125,   125,   125,   125,   126,
     126,   126,   126,   126,   126,   126,   126,   126,   127,   127,
     127,   127,   128,   128,   129,   130,   130,   131,   132,   132,
     132,   132,   133,   133,   134,   134,   134,   134,   135,   135,
     135,   135,   136,   136,   136,   136,   137,   137,   138,   138,
     139,   139,   140,   140,   140,   140,   141,   141,   142,   142,
     142,   143,   144,   144,   145,   145,   146,   146,   147,   148,
     148,   149
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     7,     1,     1,     1,     2,
       0,     7,     1,     1,     1,     1,     1,     1,     0,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     3,     0,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       4,     2,     3,     4,     5,     3,     3,     1,     1,     3,
       6,     3,     1,     2,     0,     6,     6,     1,     0,     3,
       1,     3,     3,     1,     2,     1,     3,     5,     3,     5,
       3,     4,     2,     0,     5,     6,     3,     1,     1,     1,
       6,     5,     6,     5,     6,     6,     6,     6,     2,     1,
       5,     1,     1,     1,     1,     2,     1,     5,     1,     3,
       1,     1,     3,     6,     3,     1,     3,     3,     1,     3,
       5,     3,     3,     1,     5,     6,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     0,     1,     1,     1,     1,     1,
       3,     1,     0,     3,     3,     3,     3,     3,     1,     2,
       2,     2,     2,     1,     4,     1,     1,     3,     3,     3,
       1,     1,     3,     1,     3,     1,     2,     4,     1,     3,
       4,     6,     1,     0,     1,     1,     1,     1,     3,     3,
       1,     1,     3,     3,     1,     1,     3,     1,     1,     2,
       1,     1,     1,     1,     1,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (param, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, param); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, struct parser_param *param)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (param);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, struct parser_param *param)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, param);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, struct parser_param *param)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], param);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, param); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif



static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yystrlen (yysymbol_name (yyarg[yyi]));
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp = yystpcpy (yyp, yysymbol_name (yyarg[yyi++]));
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, struct parser_param *param)
{
  YY_USE (yyvaluep);
  YY_USE (param);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_STRING: /* "string literal"  */
#line 259 "src/xkbcomp/parser.y"
            { free(((*yyvaluep).str)); }
#line 1632 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_XkbFile: /* XkbFile  */
#line 257 "src/xkbcomp/parser.y"
            { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1638 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_XkbCompositeMap: /* XkbCompositeMap  */
#line 257 "src/xkbcomp/parser.y"
            { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1644 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_XkbMapConfigList: /* XkbMapConfigList  */
#line 258 "src/xkbcomp/parser.y"
            { FreeXkbFile(((*yyvaluep).fileList).head); }
#line 1650 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_XkbMapConfig: /* XkbMapConfig  */
#line 257 "src/xkbcomp/parser.y"
            { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1656 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_DeclList: /* DeclList  */
#line 253 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).anyList).head); }
#line 1662 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Decl: /* Decl  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).any)); }
#line 1668 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VarDecl: /* VarDecl  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).var)); }
#line 1674 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyNameDecl: /* KeyNameDecl  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).keyCode)); }
#line 1680 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyAliasDecl: /* KeyAliasDecl  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).keyAlias)); }
#line 1686 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VModDecl: /* VModDecl  */
#line 253 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).vmodList).head); }
#line 1692 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VModDefList: /* VModDefList  */
#line 253 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).vmodList).head); }
#line 1698 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VModDef: /* VModDef  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).vmod)); }
#line 1704 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_InterpretDecl: /* InterpretDecl  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).interp)); }
#line 1710 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_InterpretMatch: /* InterpretMatch  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).interp)); }
#line 1716 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VarDeclList: /* VarDeclList  */
#line 253 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).varList).head); }
#line 1722 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyTypeDecl: /* KeyTypeDecl  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).keyType)); }
#line 1728 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_SymbolsDecl: /* SymbolsDecl  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).syms)); }
#line 1734 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_OptSymbolsBody: /* OptSymbolsBody  */
#line 253 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).varList).head); }
#line 1740 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_SymbolsBody: /* SymbolsBody  */
#line 253 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).varList).head); }
#line 1746 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_SymbolsVarDecl: /* SymbolsVarDecl  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).var)); }
#line 1752 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_MultiKeySymOrActionList: /* MultiKeySymOrActionList  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1758 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_GroupCompatDecl: /* GroupCompatDecl  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).groupCompat)); }
#line 1764 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_ModMapDecl: /* ModMapDecl  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).modMask)); }
#line 1770 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyOrKeySymList: /* KeyOrKeySymList  */
#line 253 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1776 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyOrKeySym: /* KeyOrKeySym  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1782 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_LedMapDecl: /* LedMapDecl  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).ledMap)); }
#line 1788 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_LedNameDecl: /* LedNameDecl  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).ledName)); }
#line 1794 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_CoordList: /* CoordList  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1800 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Coord: /* Coord  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1806 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_ExprList: /* ExprList  */
#line 253 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1812 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Expr: /* Expr  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1818 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Term: /* Term  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1824 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_MultiActionList: /* MultiActionList  */
#line 253 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1830 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_ActionList: /* ActionList  */
#line 253 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1836 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_NonEmptyActions: /* NonEmptyActions  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1842 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Actions: /* Actions  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1848 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Action: /* Action  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1854 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Lhs: /* Lhs  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1860 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_OptTerminal: /* OptTerminal  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1866 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Terminal: /* Terminal  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1872 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_MultiKeySymList: /* MultiKeySymList  */
#line 253 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1878 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeySymList: /* KeySymList  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1884 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_NonEmptyKeySyms: /* NonEmptyKeySyms  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1890 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeySyms: /* KeySyms  */
#line 250 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1896 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_OptMapName: /* OptMapName  */
#line 259 "src/xkbcomp/parser.y"
            { free(((*yyvaluep).str)); }
#line 1902 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_MapName: /* MapName  */
#line 259 "src/xkbcomp/parser.y"
            { free(((*yyvaluep).str)); }
#line 1908 "src/xkbcomp/parser.c"
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (struct parser_param *param)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, param_scanner);
    }

  if (yychar <= END_OF_FILE)
    {
      yychar = END_OF_FILE;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* XkbFile: XkbCompositeMap  */
#line 276 "src/xkbcomp/parser.y"
                        { (yyval.file) = param->rtrn = (yyvsp[0].file); param->more_maps = !!param->rtrn; (void) yynerrs; }
#line 2187 "src/xkbcomp/parser.c"
    break;

  case 3: /* XkbFile: XkbMapConfig  */
#line 278 "src/xkbcomp/parser.y"
                        { (yyval.file) = param->rtrn = (yyvsp[0].file); param->more_maps = !!param->rtrn; YYACCEPT; }
#line 2193 "src/xkbcomp/parser.c"
    break;

  case 4: /* XkbFile: "end of file"  */
#line 280 "src/xkbcomp/parser.y"
                        { (yyval.file) = param->rtrn = NULL; param->more_maps = false; }
#line 2199 "src/xkbcomp/parser.c"
    break;

  case 5: /* XkbCompositeMap: OptFlags XkbCompositeType OptMapName "{" XkbMapConfigList "}" ";"  */
#line 286 "src/xkbcomp/parser.y"
                        { (yyval.file) = XkbFileCreate((yyvsp[-5].file_type), (yyvsp[-4].str), (ParseCommon *) (yyvsp[-2].fileList).head, (yyvsp[-6].mapFlags)); }
#line 2205 "src/xkbcomp/parser.c"
    break;

  case 6: /* XkbCompositeType: "xkb_keymap"  */
#line 289 "src/xkbcomp/parser.y"
                                        { (yyval.file_type) = FILE_TYPE_KEYMAP; }
#line 2211 "src/xkbcomp/parser.c"
    break;

  case 7: /* XkbCompositeType: "xkb_semantics"  */
#line 290 "src/xkbcomp/parser.y"
                                        { (yyval.file_type) = FILE_TYPE_KEYMAP; }
#line 2217 "src/xkbcomp/parser.c"
    break;

  case 8: /* XkbCompositeType: "xkb_layout"  */
#line 291 "src/xkbcomp/parser.y"
                                        { (yyval.file_type) = FILE_TYPE_KEYMAP; }
#line 2223 "src/xkbcomp/parser.c"
    break;

  case 9: /* XkbMapConfigList: XkbMapConfigList XkbMapConfig  */
#line 298 "src/xkbcomp/parser.y"
                        {
                            if ((yyvsp[0].file)) {
                                if ((yyvsp[-1].fileList).head) {
                                    (yyval.fileList).head = (yyvsp[-1].fileList).head;
                                    (yyval.fileList).last->common.next = &(yyvsp[0].file)->common;
                                    (yyval.fileList).last = (yyvsp[0].file);
                                } else {
                                    (yyval.fileList).head = (yyval.fileList).last = (yyvsp[0].file);
                                }
                            }
                        }
#line 2239 "src/xkbcomp/parser.c"
    break;

  case 10: /* XkbMapConfigList: %empty  */
#line 309 "src/xkbcomp/parser.y"
                        { (yyval.fileList).head = (yyval.fileList).last = NULL; }
#line 2245 "src/xkbcomp/parser.c"
    break;

  case 11: /* XkbMapConfig: OptFlags FileType OptMapName "{" DeclList "}" ";"  */
#line 315 "src/xkbcomp/parser.y"
                        {
                            if ((yyvsp[-6].mapFlags) & MAP_IS_DEPRECATED) {
                                parser_warn(param, XKB_WARNING_DEPRECATED_SECTION,
                                            "deprecated section: \"%s\"",
                                            safe_map_name((yyvsp[-4].str)));
                            }
                            (yyval.file) = XkbFileCreate((yyvsp[-5].file_type), (yyvsp[-4].str), (yyvsp[-2].anyList).head, (yyvsp[-6].mapFlags));
                        }
#line 2258 "src/xkbcomp/parser.c"
    break;

  case 12: /* FileType: "xkb_keycodes"  */
#line 325 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_KEYCODES; }
#line 2264 "src/xkbcomp/parser.c"
    break;

  case 13: /* FileType: "xkb_types"  */
#line 326 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_TYPES; }
#line 2270 "src/xkbcomp/parser.c"
    break;

  case 14: /* FileType: "xkb_compatibility"  */
#line 327 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_COMPAT; }
#line 2276 "src/xkbcomp/parser.c"
    break;

  case 15: /* FileType: "xkb_symbols"  */
#line 328 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_SYMBOLS; }
#line 2282 "src/xkbcomp/parser.c"
    break;

  case 16: /* FileType: "xkb_geometry"  */
#line 329 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_GEOMETRY; }
#line 2288 "src/xkbcomp/parser.c"
    break;

  case 17: /* OptFlags: Flags  */
#line 332 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = (yyvsp[0].mapFlags); }
#line 2294 "src/xkbcomp/parser.c"
    break;

  case 18: /* OptFlags: %empty  */
#line 333 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = 0; }
#line 2300 "src/xkbcomp/parser.c"
    break;

  case 19: /* Flags: Flags Flag  */
#line 336 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = ((yyvsp[-1].mapFlags) | (yyvsp[0].mapFlags)); }
#line 2306 "src/xkbcomp/parser.c"
    break;

  case 20: /* Flags: Flag  */
#line 337 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = (yyvsp[0].mapFlags); }
#line 2312 "src/xkbcomp/parser.c"
    break;

  case 21: /* Flag: "partial"  */
#line 340 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_PARTIAL; }
#line 2318 "src/xkbcomp/parser.c"
    break;

  case 22: /* Flag: "default"  */
#line 341 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_DEFAULT; }
#line 2324 "src/xkbcomp/parser.c"
    break;

  case 23: /* Flag: "hidden"  */
#line 342 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_HIDDEN; }
#line 2330 "src/xkbcomp/parser.c"
    break;

  case 24: /* Flag: "alphanumeric_keys"  */
#line 343 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_HAS_ALPHANUMERIC; }
#line 2336 "src/xkbcomp/parser.c"
    break;

  case 25: /* Flag: "modifier_keys"  */
#line 344 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_HAS_MODIFIER; }
#line 2342 "src/xkbcomp/parser.c"
    break;

  case 26: /* Flag: "keypad_keys"  */
#line 345 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_HAS_KEYPAD; }
#line 2348 "src/xkbcomp/parser.c"
    break;

  case 27: /* Flag: "function_keys"  */
#line 346 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_HAS_FN; }
#line 2354 "src/xkbcomp/parser.c"
    break;

  case 28: /* Flag: "alternate_group"  */
#line 347 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_ALTGR; }
#line 2360 "src/xkbcomp/parser.c"
    break;

  case 29: /* Flag: "deprecated"  */
#line 348 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_DEPRECATED; }
#line 2366 "src/xkbcomp/parser.c"
    break;

  case 30: /* Flag: "identifier"  */
#line 350 "src/xkbcomp/parser.y"
                        {
                            const bool error = (param->config.strict & PARSER_NO_UNKNOWN_SECTION_FLAGS);
                            parser_log_with_code(
                                param, (error ? XKB_LOG_LEVEL_ERROR : XKB_LOG_LEVEL_WARNING),
                                XKB_LOG_VERBOSITY_MINIMAL,
                                XKB_ERROR_UNKNOWN_SECTION_FLAG,
                                "Unknown section flag \"%.*s\"%s",
                                (unsigned)(yyvsp[0].sval).len, (yyvsp[0].sval).start,
                                (error ? "" : "; ignored")
                            );
                            if (error)
                                YYABORT;
                            (yyval.mapFlags) = 0;
                        }
#line 2385 "src/xkbcomp/parser.c"
    break;

  case 31: /* DeclList: DeclList Decl  */
#line 367 "src/xkbcomp/parser.y"
                        {
                            if ((yyvsp[0].any)) {
                                if ((yyvsp[-1].anyList).head) {
                                    (yyval.anyList).head = (yyvsp[-1].anyList).head; (yyvsp[-1].anyList).last->next = (yyvsp[0].any); (yyval.anyList).last = (yyvsp[0].any);
                                } else {
                                    (yyval.anyList).head = (yyval.anyList).last = (yyvsp[0].any);
                                }
                            }
                        }
#line 2399 "src/xkbcomp/parser.c"
    break;

  case 32: /* DeclList: DeclList OptMergeMode VModDecl  */
#line 382 "src/xkbcomp/parser.y"
                        {
                            for (VModDef *vmod = (yyvsp[0].vmodList).head; vmod; vmod = (VModDef *) vmod->common.next)
                                vmod->merge = (yyvsp[-1].merge);
                            if ((yyvsp[-2].anyList).head) {
                                (yyval.anyList).head = (yyvsp[-2].anyList).head; (yyvsp[-2].anyList).last->next = &(yyvsp[0].vmodList).head->common; (yyval.anyList).last = &(yyvsp[0].vmodList).last->common;
                            } else {
                                (yyval.anyList).head = &(yyvsp[0].vmodList).head->common; (yyval.anyList).last = &(yyvsp[0].vmodList).last->common;
                            }
                        }
#line 2413 "src/xkbcomp/parser.c"
    break;

  case 33: /* DeclList: %empty  */
#line 391 "src/xkbcomp/parser.y"
                        { (yyval.anyList).head = (yyval.anyList).last = NULL; }
#line 2419 "src/xkbcomp/parser.c"
    break;

  case 34: /* Decl: OptMergeMode VarDecl  */
#line 395 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].var)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].var);
                        }
#line 2428 "src/xkbcomp/parser.c"
    break;

  case 35: /* Decl: OptMergeMode InterpretDecl  */
#line 401 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].interp)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].interp);
                        }
#line 2437 "src/xkbcomp/parser.c"
    break;

  case 36: /* Decl: OptMergeMode KeyNameDecl  */
#line 406 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].keyCode)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].keyCode);
                        }
#line 2446 "src/xkbcomp/parser.c"
    break;

  case 37: /* Decl: OptMergeMode KeyAliasDecl  */
#line 411 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].keyAlias)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].keyAlias);
                        }
#line 2455 "src/xkbcomp/parser.c"
    break;

  case 38: /* Decl: OptMergeMode KeyTypeDecl  */
#line 416 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].keyType)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].keyType);
                        }
#line 2464 "src/xkbcomp/parser.c"
    break;

  case 39: /* Decl: OptMergeMode SymbolsDecl  */
#line 421 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].syms)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].syms);
                        }
#line 2473 "src/xkbcomp/parser.c"
    break;

  case 40: /* Decl: OptMergeMode ModMapDecl  */
#line 426 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].modMask)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].modMask);
                        }
#line 2482 "src/xkbcomp/parser.c"
    break;

  case 41: /* Decl: OptMergeMode GroupCompatDecl  */
#line 431 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].groupCompat)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].groupCompat);
                        }
#line 2491 "src/xkbcomp/parser.c"
    break;

  case 42: /* Decl: OptMergeMode LedMapDecl  */
#line 436 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].ledMap)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].ledMap);
                        }
#line 2500 "src/xkbcomp/parser.c"
    break;

  case 43: /* Decl: OptMergeMode LedNameDecl  */
#line 441 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].ledName)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].ledName);
                        }
#line 2509 "src/xkbcomp/parser.c"
    break;

  case 44: /* Decl: OptMergeMode ShapeDecl  */
#line 445 "src/xkbcomp/parser.y"
                                                        { (yyval.any) = NULL; }
#line 2515 "src/xkbcomp/parser.c"
    break;

  case 45: /* Decl: OptMergeMode SectionDecl  */
#line 446 "src/xkbcomp/parser.y"
                                                        { (yyval.any) = NULL; }
#line 2521 "src/xkbcomp/parser.c"
    break;

  case 46: /* Decl: OptMergeMode DoodadDecl  */
#line 447 "src/xkbcomp/parser.y"
                                                        { (yyval.any) = NULL; }
#line 2527 "src/xkbcomp/parser.c"
    break;

  case 47: /* Decl: OptMergeMode UnknownDecl  */
#line 449 "src/xkbcomp/parser.y"
                            { (yyval.any) = (ParseCommon *) (yyvsp[0].unknown); }
#line 2533 "src/xkbcomp/parser.c"
    break;

  case 48: /* Decl: OptMergeMode UnknownCompoundStatementDecl  */
#line 451 "src/xkbcomp/parser.y"
                            { (yyval.any) = (ParseCommon *) (yyvsp[0].unknown); }
#line 2539 "src/xkbcomp/parser.c"
    break;

  case 49: /* Decl: MergeMode "string literal"  */
#line 453 "src/xkbcomp/parser.y"
                        {
                            (yyval.any) = (ParseCommon *) IncludeCreate(param->ctx, (yyvsp[0].str), (yyvsp[-1].merge));
                            free((yyvsp[0].str));
                        }
#line 2548 "src/xkbcomp/parser.c"
    break;

  case 50: /* VarDecl: Lhs "=" Expr ";"  */
#line 460 "src/xkbcomp/parser.y"
                        { (yyval.var) = VarCreate((yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 2554 "src/xkbcomp/parser.c"
    break;

  case 51: /* VarDecl: Ident ";"  */
#line 462 "src/xkbcomp/parser.y"
                        { (yyval.var) = BoolVarCreate((yyvsp[-1].atom), true); }
#line 2560 "src/xkbcomp/parser.c"
    break;

  case 52: /* VarDecl: "!" Ident ";"  */
#line 464 "src/xkbcomp/parser.y"
                        { (yyval.var) = BoolVarCreate((yyvsp[-1].atom), false); }
#line 2566 "src/xkbcomp/parser.c"
    break;

  case 53: /* KeyNameDecl: "key name" "=" KeyCode ";"  */
#line 468 "src/xkbcomp/parser.y"
                        { (yyval.keyCode) = KeycodeCreate((yyvsp[-3].atom), (yyvsp[-1].num)); }
#line 2572 "src/xkbcomp/parser.c"
    break;

  case 54: /* KeyAliasDecl: "alias" "key name" "=" "key name" ";"  */
#line 472 "src/xkbcomp/parser.y"
                        { (yyval.keyAlias) = KeyAliasCreate((yyvsp[-3].atom), (yyvsp[-1].atom)); }
#line 2578 "src/xkbcomp/parser.c"
    break;

  case 55: /* VModDecl: "virtual_modifiers" VModDefList ";"  */
#line 476 "src/xkbcomp/parser.y"
                        { (yyval.vmodList) = (yyvsp[-1].vmodList); }
#line 2584 "src/xkbcomp/parser.c"
    break;

  case 56: /* VModDefList: VModDefList "," VModDef  */
#line 480 "src/xkbcomp/parser.y"
                        { (yyval.vmodList).head = (yyvsp[-2].vmodList).head; (yyval.vmodList).last->common.next = &(yyvsp[0].vmod)->common; (yyval.vmodList).last = (yyvsp[0].vmod); }
#line 2590 "src/xkbcomp/parser.c"
    break;

  case 57: /* VModDefList: VModDef  */
#line 482 "src/xkbcomp/parser.y"
                        { (yyval.vmodList).head = (yyval.vmodList).last = (yyvsp[0].vmod); }
#line 2596 "src/xkbcomp/parser.c"
    break;

  case 58: /* VModDef: Ident  */
#line 486 "src/xkbcomp/parser.y"
                        { (yyval.vmod) = VModCreate((yyvsp[0].atom), NULL); }
#line 2602 "src/xkbcomp/parser.c"
    break;

  case 59: /* VModDef: Ident "=" Expr  */
#line 488 "src/xkbcomp/parser.y"
                        { (yyval.vmod) = VModCreate((yyvsp[-2].atom), (yyvsp[0].expr)); }
#line 2608 "src/xkbcomp/parser.c"
    break;

  case 60: /* InterpretDecl: "interpret" InterpretMatch "{" VarDeclList "}" ";"  */
#line 494 "src/xkbcomp/parser.y"
                        { (yyvsp[-4].interp)->def = (yyvsp[-2].varList).head; (yyval.interp) = (yyvsp[-4].interp); }
#line 2614 "src/xkbcomp/parser.c"
    break;

  case 61: /* InterpretMatch: KeySym "+" Expr  */
#line 498 "src/xkbcomp/parser.y"
                        { (yyval.interp) = InterpCreate((yyvsp[-2].keysym), (yyvsp[0].expr)); }
#line 2620 "src/xkbcomp/parser.c"
    break;

  case 62: /* InterpretMatch: KeySym  */
#line 500 "src/xkbcomp/parser.y"
                        { (yyval.interp) = InterpCreate((yyvsp[0].keysym), NULL); }
#line 2626 "src/xkbcomp/parser.c"
    break;

  case 63: /* VarDeclList: VarDeclList VarDecl  */
#line 504 "src/xkbcomp/parser.y"
                        {
                            if ((yyvsp[0].var)) {
                                if ((yyvsp[-1].varList).head) {
                                    (yyval.varList).head = (yyvsp[-1].varList).head;
                                    (yyval.varList).last->common.next = &(yyvsp[0].var)->common;
                                    (yyval.varList).last = (yyvsp[0].var);
                                } else {
                                    (yyval.varList).head = (yyval.varList).last = (yyvsp[0].var);
                                }
                            }
                        }
#line 2642 "src/xkbcomp/parser.c"
    break;

  case 64: /* VarDeclList: %empty  */
#line 515 "src/xkbcomp/parser.y"
                        { (yyval.varList).head = (yyval.varList).last = NULL; }
#line 2648 "src/xkbcomp/parser.c"
    break;

  case 65: /* KeyTypeDecl: "type" String "{" VarDeclList "}" ";"  */
#line 521 "src/xkbcomp/parser.y"
                        { (yyval.keyType) = KeyTypeCreate((yyvsp[-4].atom), (yyvsp[-2].varList).head); }
#line 2654 "src/xkbcomp/parser.c"
    break;

  case 66: /* SymbolsDecl: "key" "key name" "{" OptSymbolsBody "}" ";"  */
#line 527 "src/xkbcomp/parser.y"
                        { (yyval.syms) = SymbolsCreate((yyvsp[-4].atom), (yyvsp[-2].varList).head); }
#line 2660 "src/xkbcomp/parser.c"
    break;

  case 67: /* OptSymbolsBody: SymbolsBody  */
#line 530 "src/xkbcomp/parser.y"
                                    { (yyval.varList) = (yyvsp[0].varList); }
#line 2666 "src/xkbcomp/parser.c"
    break;

  case 68: /* OptSymbolsBody: %empty  */
#line 531 "src/xkbcomp/parser.y"
                                    { (yyval.varList).head = (yyval.varList).last = NULL; }
#line 2672 "src/xkbcomp/parser.c"
    break;

  case 69: /* SymbolsBody: SymbolsBody "," SymbolsVarDecl  */
#line 535 "src/xkbcomp/parser.y"
                        { (yyval.varList).head = (yyvsp[-2].varList).head; (yyval.varList).last->common.next = &(yyvsp[0].var)->common; (yyval.varList).last = (yyvsp[0].var); }
#line 2678 "src/xkbcomp/parser.c"
    break;

  case 70: /* SymbolsBody: SymbolsVarDecl  */
#line 537 "src/xkbcomp/parser.y"
                        { (yyval.varList).head = (yyval.varList).last = (yyvsp[0].var); }
#line 2684 "src/xkbcomp/parser.c"
    break;

  case 71: /* SymbolsVarDecl: Lhs "=" Expr  */
#line 540 "src/xkbcomp/parser.y"
                                                { (yyval.var) = VarCreate((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2690 "src/xkbcomp/parser.c"
    break;

  case 72: /* SymbolsVarDecl: Lhs "=" MultiKeySymOrActionList  */
#line 541 "src/xkbcomp/parser.y"
                                                           { (yyval.var) = VarCreate((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2696 "src/xkbcomp/parser.c"
    break;

  case 73: /* SymbolsVarDecl: Ident  */
#line 542 "src/xkbcomp/parser.y"
                                                { (yyval.var) = BoolVarCreate((yyvsp[0].atom), true); }
#line 2702 "src/xkbcomp/parser.c"
    break;

  case 74: /* SymbolsVarDecl: "!" Ident  */
#line 543 "src/xkbcomp/parser.y"
                                                { (yyval.var) = BoolVarCreate((yyvsp[0].atom), false); }
#line 2708 "src/xkbcomp/parser.c"
    break;

  case 75: /* SymbolsVarDecl: MultiKeySymOrActionList  */
#line 544 "src/xkbcomp/parser.y"
                                                { (yyval.var) = VarCreate(NULL, (yyvsp[0].expr)); }
#line 2714 "src/xkbcomp/parser.c"
    break;

  case 76: /* MultiKeySymOrActionList: "[" MultiKeySymList "]"  */
#line 560 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[-1].exprList).head; }
#line 2720 "src/xkbcomp/parser.c"
    break;

  case 77: /* MultiKeySymOrActionList: "[" NoSymbolOrActionList "," MultiKeySymList "]"  */
#line 562 "src/xkbcomp/parser.y"
                        {
                            /* Prepend n times NoSymbol */
                            struct {ExprDef *head; ExprDef *last;} list = {
                                .head = (yyvsp[-1].exprList).head, .last = (yyvsp[-1].exprList).last
                            };
                            for (uint32_t k = 0; k < (yyvsp[-3].noSymbolOrActionList); k++) {
                                ExprDef* const syms =
                                    ExprCreateKeySymList(XKB_KEY_NoSymbol);
                                if (!syms) {
                                    /* TODO: Use Bison’s more appropriate YYNOMEM */
                                    YYABORT;
                                }
                                syms->common.next = &list.head->common;
                                list.head = syms;
                            }
                            (yyval.expr) = list.head;
                        }
#line 2742 "src/xkbcomp/parser.c"
    break;

  case 78: /* MultiKeySymOrActionList: "[" MultiActionList "]"  */
#line 580 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[-1].exprList).head; }
#line 2748 "src/xkbcomp/parser.c"
    break;

  case 79: /* MultiKeySymOrActionList: "[" NoSymbolOrActionList "," MultiActionList "]"  */
#line 582 "src/xkbcomp/parser.y"
                        {
                            /* Prepend n times NoAction() */
                            struct {ExprDef *head; ExprDef *last;} list = {
                                .head = (yyvsp[-1].exprList).head, .last = (yyvsp[-1].exprList).last
                            };
                            for (uint32_t k = 0; k < (yyvsp[-3].noSymbolOrActionList); k++) {
                                ExprDef* const acts = ExprCreateActionList(NULL);
                                if (!acts) {
                                    /* TODO: Use Bison’s more appropriate YYNOMEM */
                                    YYABORT;
                                }
                                acts->common.next = &list.head->common;
                                list.head = acts;
                            }
                            (yyval.expr) = list.head;
                        }
#line 2769 "src/xkbcomp/parser.c"
    break;

  case 80: /* MultiKeySymOrActionList: "[" NoSymbolOrActionList "]"  */
#line 604 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprEmptyList(); }
#line 2775 "src/xkbcomp/parser.c"
    break;

  case 81: /* NoSymbolOrActionList: NoSymbolOrActionList "," "{" "}"  */
#line 610 "src/xkbcomp/parser.y"
                        { (yyval.noSymbolOrActionList) = (yyvsp[-3].noSymbolOrActionList) + 1; }
#line 2781 "src/xkbcomp/parser.c"
    break;

  case 82: /* NoSymbolOrActionList: "{" "}"  */
#line 612 "src/xkbcomp/parser.y"
                        { (yyval.noSymbolOrActionList) = 1; }
#line 2787 "src/xkbcomp/parser.c"
    break;

  case 83: /* NoSymbolOrActionList: %empty  */
#line 613 "src/xkbcomp/parser.y"
                        { (yyval.noSymbolOrActionList) = 0; }
#line 2793 "src/xkbcomp/parser.c"
    break;

  case 84: /* GroupCompatDecl: "group" Integer "=" Expr ";"  */
#line 617 "src/xkbcomp/parser.y"
                        { (yyval.groupCompat) = GroupCompatCreate((yyvsp[-3].num), (yyvsp[-1].expr)); }
#line 2799 "src/xkbcomp/parser.c"
    break;

  case 85: /* ModMapDecl: "modifier_map" Ident "{" KeyOrKeySymList "}" ";"  */
#line 621 "src/xkbcomp/parser.y"
                        { (yyval.modMask) = ModMapCreate((yyvsp[-4].atom), (yyvsp[-2].exprList).head); }
#line 2805 "src/xkbcomp/parser.c"
    break;

  case 86: /* KeyOrKeySymList: KeyOrKeySymList "," KeyOrKeySym  */
#line 625 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyvsp[-2].exprList).head; (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common; (yyval.exprList).last = (yyvsp[0].expr); }
#line 2811 "src/xkbcomp/parser.c"
    break;

  case 87: /* KeyOrKeySymList: KeyOrKeySym  */
#line 627 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 2817 "src/xkbcomp/parser.c"
    break;

  case 88: /* KeyOrKeySym: "key name"  */
#line 631 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateKeyName((yyvsp[0].atom)); }
#line 2823 "src/xkbcomp/parser.c"
    break;

  case 89: /* KeyOrKeySym: KeySym  */
#line 633 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateKeySym((yyvsp[0].keysym)); }
#line 2829 "src/xkbcomp/parser.c"
    break;

  case 90: /* LedMapDecl: "indicator" String "{" VarDeclList "}" ";"  */
#line 637 "src/xkbcomp/parser.y"
                        { (yyval.ledMap) = LedMapCreate((yyvsp[-4].atom), (yyvsp[-2].varList).head); }
#line 2835 "src/xkbcomp/parser.c"
    break;

  case 91: /* LedNameDecl: "indicator" Integer "=" Expr ";"  */
#line 641 "src/xkbcomp/parser.y"
                        { (yyval.ledName) = LedNameCreate((yyvsp[-3].num), (yyvsp[-1].expr), false); }
#line 2841 "src/xkbcomp/parser.c"
    break;

  case 92: /* LedNameDecl: "virtual" "indicator" Integer "=" Expr ";"  */
#line 643 "src/xkbcomp/parser.y"
                        { (yyval.ledName) = LedNameCreate((yyvsp[-3].num), (yyvsp[-1].expr), true); }
#line 2847 "src/xkbcomp/parser.c"
    break;

  case 93: /* UnknownDecl: "identifier" Terminal "=" Expr ";"  */
#line 647 "src/xkbcomp/parser.y"
                        {
                            FreeStmt((ParseCommon *) (yyvsp[-3].expr));
                            FreeStmt((ParseCommon *) (yyvsp[-1].expr));
                            (yyval.unknown) = UnknownStatementCreate(STMT_UNKNOWN_DECLARATION, (yyvsp[-4].sval));
                        }
#line 2857 "src/xkbcomp/parser.c"
    break;

  case 94: /* UnknownCompoundStatementDecl: "identifier" OptTerminal "{" VarDeclList "}" ";"  */
#line 656 "src/xkbcomp/parser.y"
                        {
                            FreeStmt((ParseCommon *) (yyvsp[-4].expr));
                            FreeStmt((ParseCommon *) (yyvsp[-2].varList).head);
                            (yyval.unknown) = UnknownStatementCreate(STMT_UNKNOWN_COMPOUND, (yyvsp[-5].sval));
                        }
#line 2867 "src/xkbcomp/parser.c"
    break;

  case 95: /* ShapeDecl: "shape" String "{" OutlineList "}" ";"  */
#line 664 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2873 "src/xkbcomp/parser.c"
    break;

  case 96: /* ShapeDecl: "shape" String "{" CoordList "}" ";"  */
#line 666 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[-2].expr); (yyval.geom) = NULL; }
#line 2879 "src/xkbcomp/parser.c"
    break;

  case 97: /* SectionDecl: "section" String "{" SectionBody "}" ";"  */
#line 670 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2885 "src/xkbcomp/parser.c"
    break;

  case 98: /* SectionBody: SectionBody SectionBodyItem  */
#line 673 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL;}
#line 2891 "src/xkbcomp/parser.c"
    break;

  case 99: /* SectionBody: SectionBodyItem  */
#line 674 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL; }
#line 2897 "src/xkbcomp/parser.c"
    break;

  case 100: /* SectionBodyItem: "row" "{" RowBody "}" ";"  */
#line 678 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2903 "src/xkbcomp/parser.c"
    break;

  case 101: /* SectionBodyItem: VarDecl  */
#line 680 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[0].var)); (yyval.geom) = NULL; }
#line 2909 "src/xkbcomp/parser.c"
    break;

  case 102: /* SectionBodyItem: DoodadDecl  */
#line 682 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2915 "src/xkbcomp/parser.c"
    break;

  case 103: /* SectionBodyItem: LedMapDecl  */
#line 684 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[0].ledMap)); (yyval.geom) = NULL; }
#line 2921 "src/xkbcomp/parser.c"
    break;

  case 104: /* SectionBodyItem: OverlayDecl  */
#line 686 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2927 "src/xkbcomp/parser.c"
    break;

  case 105: /* RowBody: RowBody RowBodyItem  */
#line 689 "src/xkbcomp/parser.y"
                                                { (yyval.geom) = NULL;}
#line 2933 "src/xkbcomp/parser.c"
    break;

  case 106: /* RowBody: RowBodyItem  */
#line 690 "src/xkbcomp/parser.y"
                                                { (yyval.geom) = NULL; }
#line 2939 "src/xkbcomp/parser.c"
    break;

  case 107: /* RowBodyItem: "keys" "{" Keys "}" ";"  */
#line 693 "src/xkbcomp/parser.y"
                                                     { (yyval.geom) = NULL; }
#line 2945 "src/xkbcomp/parser.c"
    break;

  case 108: /* RowBodyItem: VarDecl  */
#line 695 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[0].var)); (yyval.geom) = NULL; }
#line 2951 "src/xkbcomp/parser.c"
    break;

  case 109: /* Keys: Keys "," Key  */
#line 698 "src/xkbcomp/parser.y"
                                                { (yyval.geom) = NULL; }
#line 2957 "src/xkbcomp/parser.c"
    break;

  case 110: /* Keys: Key  */
#line 699 "src/xkbcomp/parser.y"
                                                { (yyval.geom) = NULL; }
#line 2963 "src/xkbcomp/parser.c"
    break;

  case 111: /* Key: "key name"  */
#line 703 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2969 "src/xkbcomp/parser.c"
    break;

  case 112: /* Key: "{" ExprList "}"  */
#line 705 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[-1].exprList).head); (yyval.geom) = NULL; }
#line 2975 "src/xkbcomp/parser.c"
    break;

  case 113: /* OverlayDecl: "overlay" String "{" OverlayKeyList "}" ";"  */
#line 709 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2981 "src/xkbcomp/parser.c"
    break;

  case 114: /* OverlayKeyList: OverlayKeyList "," OverlayKey  */
#line 712 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL; }
#line 2987 "src/xkbcomp/parser.c"
    break;

  case 115: /* OverlayKeyList: OverlayKey  */
#line 713 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL; }
#line 2993 "src/xkbcomp/parser.c"
    break;

  case 116: /* OverlayKey: "key name" "=" "key name"  */
#line 716 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL; }
#line 2999 "src/xkbcomp/parser.c"
    break;

  case 117: /* OutlineList: OutlineList "," OutlineInList  */
#line 720 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL;}
#line 3005 "src/xkbcomp/parser.c"
    break;

  case 118: /* OutlineList: OutlineInList  */
#line 722 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 3011 "src/xkbcomp/parser.c"
    break;

  case 119: /* OutlineInList: "{" CoordList "}"  */
#line 726 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[-1].expr); (yyval.geom) = NULL; }
#line 3017 "src/xkbcomp/parser.c"
    break;

  case 120: /* OutlineInList: Ident "=" "{" CoordList "}"  */
#line 728 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[-1].expr); (yyval.geom) = NULL; }
#line 3023 "src/xkbcomp/parser.c"
    break;

  case 121: /* OutlineInList: Ident "=" Expr  */
#line 730 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[0].expr)); (yyval.geom) = NULL; }
#line 3029 "src/xkbcomp/parser.c"
    break;

  case 122: /* CoordList: CoordList "," Coord  */
#line 734 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[-2].expr); (void) (yyvsp[0].expr); (yyval.expr) = NULL; }
#line 3035 "src/xkbcomp/parser.c"
    break;

  case 123: /* CoordList: Coord  */
#line 736 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[0].expr); (yyval.expr) = NULL; }
#line 3041 "src/xkbcomp/parser.c"
    break;

  case 124: /* Coord: "[" SignedNumber "," SignedNumber "]"  */
#line 740 "src/xkbcomp/parser.y"
                        { (yyval.expr) = NULL; }
#line 3047 "src/xkbcomp/parser.c"
    break;

  case 125: /* DoodadDecl: DoodadType String "{" VarDeclList "}" ";"  */
#line 744 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[-2].varList).head); (yyval.geom) = NULL; }
#line 3053 "src/xkbcomp/parser.c"
    break;

  case 126: /* DoodadType: "text"  */
#line 747 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3059 "src/xkbcomp/parser.c"
    break;

  case 127: /* DoodadType: "outline"  */
#line 748 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3065 "src/xkbcomp/parser.c"
    break;

  case 128: /* DoodadType: "solid"  */
#line 749 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3071 "src/xkbcomp/parser.c"
    break;

  case 129: /* DoodadType: "logo"  */
#line 750 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3077 "src/xkbcomp/parser.c"
    break;

  case 130: /* FieldSpec: Ident  */
#line 753 "src/xkbcomp/parser.y"
                                { (yyval.atom) = (yyvsp[0].atom); }
#line 3083 "src/xkbcomp/parser.c"
    break;

  case 131: /* FieldSpec: Element  */
#line 754 "src/xkbcomp/parser.y"
                                { (yyval.atom) = (yyvsp[0].atom); }
#line 3089 "src/xkbcomp/parser.c"
    break;

  case 132: /* Element: "action"  */
#line 758 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "action"); }
#line 3095 "src/xkbcomp/parser.c"
    break;

  case 133: /* Element: "interpret"  */
#line 760 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "interpret"); }
#line 3101 "src/xkbcomp/parser.c"
    break;

  case 134: /* Element: "type"  */
#line 762 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "type"); }
#line 3107 "src/xkbcomp/parser.c"
    break;

  case 135: /* Element: "key"  */
#line 764 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "key"); }
#line 3113 "src/xkbcomp/parser.c"
    break;

  case 136: /* Element: "group"  */
#line 766 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "group"); }
#line 3119 "src/xkbcomp/parser.c"
    break;

  case 137: /* Element: "modifier_map"  */
#line 768 "src/xkbcomp/parser.y"
                        {(yyval.atom) = xkb_atom_intern_literal(param->ctx, "modifier_map");}
#line 3125 "src/xkbcomp/parser.c"
    break;

  case 138: /* Element: "indicator"  */
#line 770 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "indicator"); }
#line 3131 "src/xkbcomp/parser.c"
    break;

  case 139: /* Element: "shape"  */
#line 772 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "shape"); }
#line 3137 "src/xkbcomp/parser.c"
    break;

  case 140: /* Element: "row"  */
#line 774 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "row"); }
#line 3143 "src/xkbcomp/parser.c"
    break;

  case 141: /* Element: "section"  */
#line 776 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "section"); }
#line 3149 "src/xkbcomp/parser.c"
    break;

  case 142: /* Element: "text"  */
#line 778 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "text"); }
#line 3155 "src/xkbcomp/parser.c"
    break;

  case 143: /* OptMergeMode: MergeMode  */
#line 781 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = (yyvsp[0].merge); }
#line 3161 "src/xkbcomp/parser.c"
    break;

  case 144: /* OptMergeMode: %empty  */
#line 782 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_DEFAULT; }
#line 3167 "src/xkbcomp/parser.c"
    break;

  case 145: /* MergeMode: "include"  */
#line 785 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_DEFAULT; }
#line 3173 "src/xkbcomp/parser.c"
    break;

  case 146: /* MergeMode: "augment"  */
#line 786 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_AUGMENT; }
#line 3179 "src/xkbcomp/parser.c"
    break;

  case 147: /* MergeMode: "override"  */
#line 787 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_OVERRIDE; }
#line 3185 "src/xkbcomp/parser.c"
    break;

  case 148: /* MergeMode: "replace"  */
#line 788 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_REPLACE; }
#line 3191 "src/xkbcomp/parser.c"
    break;

  case 149: /* MergeMode: "alternate"  */
#line 790 "src/xkbcomp/parser.y"
                {
                    /*
                     * This used to be MERGE_ALT_FORM. This functionality was
                     * unused and has been removed.
                     */
                    parser_warn(param, XKB_LOG_MESSAGE_NO_ID,
                                "ignored unsupported legacy merge mode \"alternate\"");
                    (yyval.merge) = MERGE_DEFAULT;
                }
#line 3205 "src/xkbcomp/parser.c"
    break;

  case 150: /* ExprList: ExprList "," Expr  */
#line 802 "src/xkbcomp/parser.y"
                        {
                            if ((yyvsp[0].expr)) {
                                if ((yyvsp[-2].exprList).head) {
                                    (yyval.exprList).head = (yyvsp[-2].exprList).head;
                                    (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common;
                                    (yyval.exprList).last = (yyvsp[0].expr);
                                } else {
                                    (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr);
                                }
                            }
                        }
#line 3221 "src/xkbcomp/parser.c"
    break;

  case 151: /* ExprList: Expr  */
#line 814 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 3227 "src/xkbcomp/parser.c"
    break;

  case 152: /* ExprList: %empty  */
#line 815 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = NULL; }
#line 3233 "src/xkbcomp/parser.c"
    break;

  case 153: /* Expr: Expr "/" Expr  */
#line 819 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_DIVIDE, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3239 "src/xkbcomp/parser.c"
    break;

  case 154: /* Expr: Expr "+" Expr  */
#line 821 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3245 "src/xkbcomp/parser.c"
    break;

  case 155: /* Expr: Expr "-" Expr  */
#line 823 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_SUBTRACT, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3251 "src/xkbcomp/parser.c"
    break;

  case 156: /* Expr: Expr "*" Expr  */
#line 825 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_MULTIPLY, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3257 "src/xkbcomp/parser.c"
    break;

  case 157: /* Expr: Lhs "=" Expr  */
#line 827 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3263 "src/xkbcomp/parser.c"
    break;

  case 158: /* Expr: Term  */
#line 829 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3269 "src/xkbcomp/parser.c"
    break;

  case 159: /* Term: "-" Term  */
#line 833 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateUnary(STMT_EXPR_NEGATE, (yyvsp[0].expr)); }
#line 3275 "src/xkbcomp/parser.c"
    break;

  case 160: /* Term: "+" Term  */
#line 835 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateUnary(STMT_EXPR_UNARY_PLUS, (yyvsp[0].expr)); }
#line 3281 "src/xkbcomp/parser.c"
    break;

  case 161: /* Term: "!" Term  */
#line 837 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateUnary(STMT_EXPR_NOT, (yyvsp[0].expr)); }
#line 3287 "src/xkbcomp/parser.c"
    break;

  case 162: /* Term: "~" Term  */
#line 839 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateUnary(STMT_EXPR_INVERT, (yyvsp[0].expr)); }
#line 3293 "src/xkbcomp/parser.c"
    break;

  case 163: /* Term: Lhs  */
#line 841 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3299 "src/xkbcomp/parser.c"
    break;

  case 164: /* Term: FieldSpec "(" ExprList ")"  */
#line 843 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateAction((yyvsp[-3].atom), (yyvsp[-1].exprList).head); }
#line 3305 "src/xkbcomp/parser.c"
    break;

  case 165: /* Term: Actions  */
#line 845 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3311 "src/xkbcomp/parser.c"
    break;

  case 166: /* Term: Terminal  */
#line 847 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3317 "src/xkbcomp/parser.c"
    break;

  case 167: /* Term: "(" Expr ")"  */
#line 849 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[-1].expr); }
#line 3323 "src/xkbcomp/parser.c"
    break;

  case 168: /* MultiActionList: MultiActionList "," Action  */
#line 853 "src/xkbcomp/parser.y"
                        {
                            ExprDef *expr = ExprCreateActionList((yyvsp[0].expr));
                            (yyval.exprList) = (yyvsp[-2].exprList);
                            (yyval.exprList).last->common.next = &expr->common; (yyval.exprList).last = expr;
                        }
#line 3333 "src/xkbcomp/parser.c"
    break;

  case 169: /* MultiActionList: MultiActionList "," Actions  */
#line 859 "src/xkbcomp/parser.y"
                        { (yyval.exprList) = (yyvsp[-2].exprList); (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common; (yyval.exprList).last = (yyvsp[0].expr); }
#line 3339 "src/xkbcomp/parser.c"
    break;

  case 170: /* MultiActionList: Action  */
#line 861 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = ExprCreateActionList((yyvsp[0].expr)); }
#line 3345 "src/xkbcomp/parser.c"
    break;

  case 171: /* MultiActionList: NonEmptyActions  */
#line 863 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 3351 "src/xkbcomp/parser.c"
    break;

  case 172: /* ActionList: ActionList "," Action  */
#line 867 "src/xkbcomp/parser.y"
                        { (yyval.exprList) = (yyvsp[-2].exprList); (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common; (yyval.exprList).last = (yyvsp[0].expr); }
#line 3357 "src/xkbcomp/parser.c"
    break;

  case 173: /* ActionList: Action  */
#line 869 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 3363 "src/xkbcomp/parser.c"
    break;

  case 174: /* NonEmptyActions: "{" ActionList "}"  */
#line 873 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateActionList((yyvsp[-1].exprList).head); }
#line 3369 "src/xkbcomp/parser.c"
    break;

  case 175: /* Actions: NonEmptyActions  */
#line 877 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3375 "src/xkbcomp/parser.c"
    break;

  case 176: /* Actions: "{" "}"  */
#line 879 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateActionList(NULL); }
#line 3381 "src/xkbcomp/parser.c"
    break;

  case 177: /* Action: FieldSpec "(" ExprList ")"  */
#line 883 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateAction((yyvsp[-3].atom), (yyvsp[-1].exprList).head); }
#line 3387 "src/xkbcomp/parser.c"
    break;

  case 178: /* Lhs: FieldSpec  */
#line 887 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateIdent((yyvsp[0].atom)); }
#line 3393 "src/xkbcomp/parser.c"
    break;

  case 179: /* Lhs: FieldSpec "." FieldSpec  */
#line 889 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateFieldRef((yyvsp[-2].atom), (yyvsp[0].atom)); }
#line 3399 "src/xkbcomp/parser.c"
    break;

  case 180: /* Lhs: FieldSpec "[" Expr "]"  */
#line 891 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateArrayRef(XKB_ATOM_NONE, (yyvsp[-3].atom), (yyvsp[-1].expr)); }
#line 3405 "src/xkbcomp/parser.c"
    break;

  case 181: /* Lhs: FieldSpec "." FieldSpec "[" Expr "]"  */
#line 893 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateArrayRef((yyvsp[-5].atom), (yyvsp[-3].atom), (yyvsp[-1].expr)); }
#line 3411 "src/xkbcomp/parser.c"
    break;

  case 182: /* OptTerminal: Terminal  */
#line 897 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3417 "src/xkbcomp/parser.c"
    break;

  case 183: /* OptTerminal: %empty  */
#line 898 "src/xkbcomp/parser.y"
                        { (yyval.expr) = NULL; }
#line 3423 "src/xkbcomp/parser.c"
    break;

  case 184: /* Terminal: String  */
#line 902 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateString((yyvsp[0].atom)); }
#line 3429 "src/xkbcomp/parser.c"
    break;

  case 185: /* Terminal: Integer  */
#line 904 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateInteger((yyvsp[0].num)); }
#line 3435 "src/xkbcomp/parser.c"
    break;

  case 186: /* Terminal: Float  */
#line 906 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateFloat(/* Discard $1 */); }
#line 3441 "src/xkbcomp/parser.c"
    break;

  case 187: /* Terminal: "key name"  */
#line 908 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateKeyName((yyvsp[0].atom)); }
#line 3447 "src/xkbcomp/parser.c"
    break;

  case 188: /* MultiKeySymList: MultiKeySymList "," KeySymLit  */
#line 912 "src/xkbcomp/parser.y"
                        {
                            ExprDef *expr = ExprCreateKeySymList((yyvsp[0].keysym));
                            (yyval.exprList) = (yyvsp[-2].exprList);
                            (yyval.exprList).last->common.next = &expr->common; (yyval.exprList).last = expr;
                        }
#line 3457 "src/xkbcomp/parser.c"
    break;

  case 189: /* MultiKeySymList: MultiKeySymList "," KeySyms  */
#line 918 "src/xkbcomp/parser.y"
                        { (yyval.exprList) = (yyvsp[-2].exprList); (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common; (yyval.exprList).last = (yyvsp[0].expr); }
#line 3463 "src/xkbcomp/parser.c"
    break;

  case 190: /* MultiKeySymList: KeySymLit  */
#line 920 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = ExprCreateKeySymList((yyvsp[0].keysym)); }
#line 3469 "src/xkbcomp/parser.c"
    break;

  case 191: /* MultiKeySymList: NonEmptyKeySyms  */
#line 922 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 3475 "src/xkbcomp/parser.c"
    break;

  case 192: /* KeySymList: KeySymList "," KeySymLit  */
#line 926 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprAppendKeySymList((yyvsp[-2].expr), (yyvsp[0].keysym)); }
#line 3481 "src/xkbcomp/parser.c"
    break;

  case 193: /* KeySymList: KeySymList "," "string literal"  */
#line 928 "src/xkbcomp/parser.y"
                        {
                            (yyval.expr) = ExprKeySymListAppendString(param->scanner, (yyvsp[-2].expr), (yyvsp[0].str));
                            free((yyvsp[0].str));
                            if (!(yyval.expr))
                                YYERROR;
                        }
#line 3492 "src/xkbcomp/parser.c"
    break;

  case 194: /* KeySymList: KeySymLit  */
#line 935 "src/xkbcomp/parser.y"
                        {
                            (yyval.expr) = ExprCreateKeySymList((yyvsp[0].keysym));
                            if (!(yyval.expr))
                                YYERROR;
                        }
#line 3502 "src/xkbcomp/parser.c"
    break;

  case 195: /* KeySymList: "string literal"  */
#line 941 "src/xkbcomp/parser.y"
                        {
                            (yyval.expr) = ExprCreateKeySymList(XKB_KEY_NoSymbol);
                            if (!(yyval.expr))
                                YYERROR;
                            (yyval.expr) = ExprKeySymListAppendString(param->scanner, (yyval.expr), (yyvsp[0].str));
                            free((yyvsp[0].str));
                            if (!(yyval.expr))
                                YYERROR;
                        }
#line 3516 "src/xkbcomp/parser.c"
    break;

  case 196: /* NonEmptyKeySyms: "{" KeySymList "}"  */
#line 953 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[-1].expr); }
#line 3522 "src/xkbcomp/parser.c"
    break;

  case 197: /* NonEmptyKeySyms: "string literal"  */
#line 955 "src/xkbcomp/parser.y"
                        {
                            (yyval.expr) = ExprCreateKeySymList(XKB_KEY_NoSymbol);
                            if (!(yyval.expr))
                                YYERROR;
                            (yyval.expr) = ExprKeySymListAppendString(param->scanner, (yyval.expr), (yyvsp[0].str));
                            free((yyvsp[0].str));
                            if (!(yyval.expr))
                                YYERROR;
                        }
#line 3536 "src/xkbcomp/parser.c"
    break;

  case 198: /* KeySyms: NonEmptyKeySyms  */
#line 967 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3542 "src/xkbcomp/parser.c"
    break;

  case 199: /* KeySyms: "{" "}"  */
#line 969 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateKeySymList(XKB_KEY_NoSymbol); }
#line 3548 "src/xkbcomp/parser.c"
    break;

  case 200: /* KeySym: KeySymLit  */
#line 973 "src/xkbcomp/parser.y"
                        { (yyval.keysym) = (yyvsp[0].keysym); }
#line 3554 "src/xkbcomp/parser.c"
    break;

  case 201: /* KeySym: "string literal"  */
#line 975 "src/xkbcomp/parser.y"
                        {
                            (yyval.keysym) = KeysymParseString(param->scanner, (yyvsp[0].str));
                            free((yyvsp[0].str));
                            if ((yyval.keysym) == XKB_KEY_NoSymbol)
                                YYERROR;
                        }
#line 3565 "src/xkbcomp/parser.c"
    break;

  case 202: /* KeySymLit: "identifier"  */
#line 984 "src/xkbcomp/parser.y"
                        {
                            if (!resolve_keysym(param, (yyvsp[0].sval), &(yyval.keysym))) {
                                parser_warn(
                                    param,
                                    XKB_WARNING_UNRECOGNIZED_KEYSYM,
                                    "unrecognized keysym \"%.*s\"",
                                    (unsigned int) (yyvsp[0].sval).len, (yyvsp[0].sval).start
                                );
                                (yyval.keysym) = XKB_KEY_NoSymbol;
                            }
                        }
#line 3581 "src/xkbcomp/parser.c"
    break;

  case 203: /* KeySymLit: "section"  */
#line 996 "src/xkbcomp/parser.y"
                                { (yyval.keysym) = XKB_KEY_section; }
#line 3587 "src/xkbcomp/parser.c"
    break;

  case 204: /* KeySymLit: "decimal digit"  */
#line 998 "src/xkbcomp/parser.y"
                        {
                            /*
                             * Special case for digits 0..9:
                             * map to XKB_KEY_0 .. XKB_KEY_9, consistent with
                             * other keysym names: <name> → XKB_KEY_<name>.
                             */
                            (yyval.keysym) = XKB_KEY_0 + (xkb_keysym_t) (yyvsp[0].num);
                        }
#line 3600 "src/xkbcomp/parser.c"
    break;

  case 205: /* KeySymLit: "integer literal"  */
#line 1007 "src/xkbcomp/parser.y"
                        {
                            if ((yyvsp[0].num) < XKB_KEYSYM_MIN) {
                                /* Negative value */
                                static_assert(XKB_KEYSYM_MIN == 0,
                                              "Keysyms are positive");
                                parser_warn(
                                    param,
                                    XKB_ERROR_INVALID_NUMERIC_KEYSYM,
                                    "unrecognized keysym \"-%#06"PRIx64"\""
                                    " (%"PRId64")",
                                    -(yyvsp[0].num), (yyvsp[0].num)
                                );
                                (yyval.keysym) = XKB_KEY_NoSymbol;
                            }
                            /*
                             * Integers 0..9 are handled with DECIMAL_DIGIT if
                             * they were formatted as single characters '0'..'9'.
                             * Otherwise they are handled here as raw keysyms
                             * values. E.g. `01` and `0x1` are interpreted as
                             * the keysym 0x0001, while `1` is interpreted as
                             * XKB_KEY_1.
                             */
                            else {
                                /* Any other numeric value */
                                if ((yyvsp[0].num) <= XKB_KEYSYM_MAX) {
                                    /*
                                     * Valid keysym
                                     * No normalization is performed and value
                                     * is used as is.
                                     */
                                    (yyval.keysym) = (xkb_keysym_t) (yyvsp[0].num);
                                    check_deprecated_keysyms(
                                        parser_warn, param, param->ctx,
                                        (yyval.keysym), NULL, (yyval.keysym), "%#06"PRIx32, "");
                                } else {
                                    /* Invalid keysym */
                                    parser_warn(
                                        param, XKB_ERROR_INVALID_NUMERIC_KEYSYM,
                                        "unrecognized keysym \"%#06"PRIx64"\" "
                                        "(%"PRId64")", (yyvsp[0].num), (yyvsp[0].num)
                                    );
                                    (yyval.keysym) = XKB_KEY_NoSymbol;
                                }
                                parser_vrb(
                                    /*
                                     * Require an extra high verbosity, because
                                     * keysyms are formatted as number unless
                                     * enabling pretty-pretting for the
                                     * serialization.
                                     */
                                    param, XKB_LOG_VERBOSITY_COMPREHENSIVE,
                                    XKB_WARNING_NUMERIC_KEYSYM,
                                    "numeric keysym \"%#06"PRIx64"\" (%"PRId64")",
                                    (yyvsp[0].num), (yyvsp[0].num)
                                );
                            }
                        }
#line 3662 "src/xkbcomp/parser.c"
    break;

  case 206: /* SignedNumber: "-" Number  */
#line 1066 "src/xkbcomp/parser.y"
                                        { (yyval.num) = -(yyvsp[0].num); }
#line 3668 "src/xkbcomp/parser.c"
    break;

  case 207: /* SignedNumber: Number  */
#line 1067 "src/xkbcomp/parser.y"
                                        { (yyval.num) = (yyvsp[0].num); }
#line 3674 "src/xkbcomp/parser.c"
    break;

  case 208: /* Number: "float literal"  */
#line 1070 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3680 "src/xkbcomp/parser.c"
    break;

  case 209: /* Number: "decimal digit"  */
#line 1071 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3686 "src/xkbcomp/parser.c"
    break;

  case 210: /* Number: "integer literal"  */
#line 1072 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3692 "src/xkbcomp/parser.c"
    break;

  case 211: /* Float: "float literal"  */
#line 1075 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3698 "src/xkbcomp/parser.c"
    break;

  case 212: /* Integer: "integer literal"  */
#line 1078 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3704 "src/xkbcomp/parser.c"
    break;

  case 213: /* Integer: "decimal digit"  */
#line 1079 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3710 "src/xkbcomp/parser.c"
    break;

  case 214: /* KeyCode: "integer literal"  */
#line 1082 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3716 "src/xkbcomp/parser.c"
    break;

  case 215: /* KeyCode: "decimal digit"  */
#line 1083 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3722 "src/xkbcomp/parser.c"
    break;

  case 216: /* Ident: "identifier"  */
#line 1086 "src/xkbcomp/parser.y"
                                { (yyval.atom) = xkb_atom_intern(param->ctx, (yyvsp[0].sval).start, (yyvsp[0].sval).len); }
#line 3728 "src/xkbcomp/parser.c"
    break;

  case 217: /* Ident: "default"  */
#line 1087 "src/xkbcomp/parser.y"
                                { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "default"); }
#line 3734 "src/xkbcomp/parser.c"
    break;

  case 218: /* String: "string literal"  */
#line 1090 "src/xkbcomp/parser.y"
                                { (yyval.atom) = xkb_atom_intern(param->ctx, (yyvsp[0].str), strlen((yyvsp[0].str))); free((yyvsp[0].str)); }
#line 3740 "src/xkbcomp/parser.c"
    break;

  case 219: /* OptMapName: MapName  */
#line 1093 "src/xkbcomp/parser.y"
                                { (yyval.str) = (yyvsp[0].str); }
#line 3746 "src/xkbcomp/parser.c"
    break;

  case 220: /* OptMapName: %empty  */
#line 1094 "src/xkbcomp/parser.y"
                                { (yyval.str) = NULL; }
#line 3752 "src/xkbcomp/parser.c"
    break;

  case 221: /* MapName: "string literal"  */
#line 1097 "src/xkbcomp/parser.y"
                                { (yyval.str) = (yyvsp[0].str); }
#line 3758 "src/xkbcomp/parser.c"
    break;


#line 3762 "src/xkbcomp/parser.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (param, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= END_OF_FILE)
        {
          /* Return failure if at end of input.  */
          if (yychar == END_OF_FILE)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, param);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, param);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (param, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, param);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, param);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 1100 "src/xkbcomp/parser.y"


/* Parse a specific section */
XkbFile *
parse(struct xkb_context *ctx, const struct parser_keymap_config *config,
      struct scanner *scanner, const char *map)
{
    int ret;
    XkbFile *first = NULL;
    struct parser_param param = {
        .scanner = scanner,
        .ctx = ctx,
        .rtrn = NULL,
        .config = *config,
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

    /*
     * Warn about implicit default section,
     * but only if not a keymap: multiple keymaps per file not supported
     */
    if (first && first->file_type != FILE_TYPE_KEYMAP)
        log_vrb(ctx, XKB_LOG_VERBOSITY_DETAILED,
                XKB_WARNING_MISSING_DEFAULT_SECTION,
                "No section name in include statement, but \"%s\" contains several; "
                "Using first defined section, \"%s\"\n",
                scanner->file_name, safe_map_name(first->name));

    return first;
}

/* Parse the next section */
bool
parse_next(struct xkb_context *ctx,
           const struct parser_keymap_config *config,
           struct scanner *scanner, XkbFile **xkb_file)
{
    int ret;
    struct parser_param param = {
        .scanner = scanner,
        .ctx = ctx,
        .rtrn = NULL,
        .config = *config,
        .more_maps = false,
    };

    if ((ret = yyparse(&param)) == 0 && param.more_maps) {
        *xkb_file = param.rtrn;
        return true;
    } else {
        FreeXkbFile(param.rtrn);
        *xkb_file = NULL;
        return (ret == 0);
    }
}
