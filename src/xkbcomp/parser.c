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
#line 21 "src/xkbcomp/parser.y"

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

#line 145 "src/xkbcomp/parser.c"

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
  YYSYMBOL_YYACCEPT = 66,                  /* $accept  */
  YYSYMBOL_XkbFile = 67,                   /* XkbFile  */
  YYSYMBOL_XkbCompositeMap = 68,           /* XkbCompositeMap  */
  YYSYMBOL_XkbCompositeType = 69,          /* XkbCompositeType  */
  YYSYMBOL_XkbMapConfigList = 70,          /* XkbMapConfigList  */
  YYSYMBOL_XkbMapConfig = 71,              /* XkbMapConfig  */
  YYSYMBOL_FileType = 72,                  /* FileType  */
  YYSYMBOL_OptFlags = 73,                  /* OptFlags  */
  YYSYMBOL_Flags = 74,                     /* Flags  */
  YYSYMBOL_Flag = 75,                      /* Flag  */
  YYSYMBOL_DeclList = 76,                  /* DeclList  */
  YYSYMBOL_Decl = 77,                      /* Decl  */
  YYSYMBOL_VarDecl = 78,                   /* VarDecl  */
  YYSYMBOL_KeyNameDecl = 79,               /* KeyNameDecl  */
  YYSYMBOL_KeyAliasDecl = 80,              /* KeyAliasDecl  */
  YYSYMBOL_VModDecl = 81,                  /* VModDecl  */
  YYSYMBOL_VModDefList = 82,               /* VModDefList  */
  YYSYMBOL_VModDef = 83,                   /* VModDef  */
  YYSYMBOL_InterpretDecl = 84,             /* InterpretDecl  */
  YYSYMBOL_InterpretMatch = 85,            /* InterpretMatch  */
  YYSYMBOL_VarDeclList = 86,               /* VarDeclList  */
  YYSYMBOL_KeyTypeDecl = 87,               /* KeyTypeDecl  */
  YYSYMBOL_SymbolsDecl = 88,               /* SymbolsDecl  */
  YYSYMBOL_OptSymbolsBody = 89,            /* OptSymbolsBody  */
  YYSYMBOL_SymbolsBody = 90,               /* SymbolsBody  */
  YYSYMBOL_SymbolsVarDecl = 91,            /* SymbolsVarDecl  */
  YYSYMBOL_MultiKeySymOrActionList = 92,   /* MultiKeySymOrActionList  */
  YYSYMBOL_NoSymbolOrActionList = 93,      /* NoSymbolOrActionList  */
  YYSYMBOL_GroupCompatDecl = 94,           /* GroupCompatDecl  */
  YYSYMBOL_ModMapDecl = 95,                /* ModMapDecl  */
  YYSYMBOL_KeyOrKeySymList = 96,           /* KeyOrKeySymList  */
  YYSYMBOL_KeyOrKeySym = 97,               /* KeyOrKeySym  */
  YYSYMBOL_LedMapDecl = 98,                /* LedMapDecl  */
  YYSYMBOL_LedNameDecl = 99,               /* LedNameDecl  */
  YYSYMBOL_UnknownDecl = 100,              /* UnknownDecl  */
  YYSYMBOL_UnknownCompoundStatementDecl = 101, /* UnknownCompoundStatementDecl  */
  YYSYMBOL_ShapeDecl = 102,                /* ShapeDecl  */
  YYSYMBOL_SectionDecl = 103,              /* SectionDecl  */
  YYSYMBOL_SectionBody = 104,              /* SectionBody  */
  YYSYMBOL_SectionBodyItem = 105,          /* SectionBodyItem  */
  YYSYMBOL_RowBody = 106,                  /* RowBody  */
  YYSYMBOL_RowBodyItem = 107,              /* RowBodyItem  */
  YYSYMBOL_Keys = 108,                     /* Keys  */
  YYSYMBOL_Key = 109,                      /* Key  */
  YYSYMBOL_OverlayDecl = 110,              /* OverlayDecl  */
  YYSYMBOL_OverlayKeyList = 111,           /* OverlayKeyList  */
  YYSYMBOL_OverlayKey = 112,               /* OverlayKey  */
  YYSYMBOL_OutlineList = 113,              /* OutlineList  */
  YYSYMBOL_OutlineInList = 114,            /* OutlineInList  */
  YYSYMBOL_CoordList = 115,                /* CoordList  */
  YYSYMBOL_Coord = 116,                    /* Coord  */
  YYSYMBOL_DoodadDecl = 117,               /* DoodadDecl  */
  YYSYMBOL_DoodadType = 118,               /* DoodadType  */
  YYSYMBOL_FieldSpec = 119,                /* FieldSpec  */
  YYSYMBOL_Element = 120,                  /* Element  */
  YYSYMBOL_OptMergeMode = 121,             /* OptMergeMode  */
  YYSYMBOL_MergeMode = 122,                /* MergeMode  */
  YYSYMBOL_ExprList = 123,                 /* ExprList  */
  YYSYMBOL_Expr = 124,                     /* Expr  */
  YYSYMBOL_Term = 125,                     /* Term  */
  YYSYMBOL_MultiActionList = 126,          /* MultiActionList  */
  YYSYMBOL_ActionList = 127,               /* ActionList  */
  YYSYMBOL_NonEmptyActions = 128,          /* NonEmptyActions  */
  YYSYMBOL_Actions = 129,                  /* Actions  */
  YYSYMBOL_Action = 130,                   /* Action  */
  YYSYMBOL_Lhs = 131,                      /* Lhs  */
  YYSYMBOL_OptTerminal = 132,              /* OptTerminal  */
  YYSYMBOL_Terminal = 133,                 /* Terminal  */
  YYSYMBOL_MultiKeySymList = 134,          /* MultiKeySymList  */
  YYSYMBOL_KeySymList = 135,               /* KeySymList  */
  YYSYMBOL_NonEmptyKeySyms = 136,          /* NonEmptyKeySyms  */
  YYSYMBOL_KeySyms = 137,                  /* KeySyms  */
  YYSYMBOL_KeySym = 138,                   /* KeySym  */
  YYSYMBOL_KeySymLit = 139,                /* KeySymLit  */
  YYSYMBOL_SignedNumber = 140,             /* SignedNumber  */
  YYSYMBOL_Number = 141,                   /* Number  */
  YYSYMBOL_Float = 142,                    /* Float  */
  YYSYMBOL_Integer = 143,                  /* Integer  */
  YYSYMBOL_KeyCode = 144,                  /* KeyCode  */
  YYSYMBOL_Ident = 145,                    /* Ident  */
  YYSYMBOL_String = 146,                   /* String  */
  YYSYMBOL_OptMapName = 147,               /* OptMapName  */
  YYSYMBOL_MapName = 148                   /* MapName  */
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
#define YYFINAL  16
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   928

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  66
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  83
/* YYNRULES -- Number of rules.  */
#define YYNRULES  219
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  384

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
      58,    59,    60,    61,    62,    63,    64,    65,     2,     2,
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
       0,   268,   268,   270,   272,   276,   282,   283,   284,   290,
     302,   305,   313,   314,   315,   316,   317,   320,   321,   324,
     325,   328,   329,   330,   331,   332,   333,   334,   335,   338,
     353,   363,   366,   372,   377,   382,   387,   392,   397,   402,
     407,   412,   417,   418,   419,   420,   422,   424,   431,   433,
     435,   439,   443,   447,   451,   453,   457,   459,   463,   469,
     471,   475,   487,   490,   496,   502,   503,   506,   508,   512,
     513,   514,   515,   516,   531,   533,   551,   553,   575,   581,
     583,   585,   588,   592,   596,   598,   602,   604,   608,   612,
     614,   618,   627,   635,   637,   641,   645,   646,   649,   651,
     653,   655,   657,   661,   662,   665,   666,   670,   671,   674,
     676,   680,   684,   685,   688,   691,   693,   697,   699,   701,
     705,   707,   711,   715,   719,   720,   721,   722,   725,   726,
     729,   731,   733,   735,   737,   739,   741,   743,   745,   747,
     749,   753,   754,   757,   758,   759,   760,   761,   773,   785,
     787,   790,   792,   794,   796,   798,   800,   804,   806,   808,
     810,   812,   814,   816,   818,   820,   824,   830,   832,   834,
     838,   840,   844,   848,   850,   854,   858,   860,   862,   864,
     868,   870,   873,   875,   877,   879,   883,   889,   891,   893,
     897,   899,   906,   912,   924,   926,   938,   940,   944,   946,
     955,   968,   969,   978,  1038,  1039,  1042,  1043,  1044,  1047,
    1050,  1051,  1054,  1055,  1058,  1059,  1062,  1065,  1066,  1069
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
  "$accept", "XkbFile", "XkbCompositeMap", "XkbCompositeType",
  "XkbMapConfigList", "XkbMapConfig", "FileType", "OptFlags", "Flags",
  "Flag", "DeclList", "Decl", "VarDecl", "KeyNameDecl", "KeyAliasDecl",
  "VModDecl", "VModDefList", "VModDef", "InterpretDecl", "InterpretMatch",
  "VarDeclList", "KeyTypeDecl", "SymbolsDecl", "OptSymbolsBody",
  "SymbolsBody", "SymbolsVarDecl", "MultiKeySymOrActionList",
  "NoSymbolOrActionList", "GroupCompatDecl", "ModMapDecl",
  "KeyOrKeySymList", "KeyOrKeySym", "LedMapDecl", "LedNameDecl",
  "UnknownDecl", "UnknownCompoundStatementDecl", "ShapeDecl",
  "SectionDecl", "SectionBody", "SectionBodyItem", "RowBody",
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

#define YYPACT_NINF (-280)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-215)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
       7,  -280,  -280,  -280,  -280,  -280,  -280,  -280,  -280,  -280,
      32,  -280,  -280,   578,   847,  -280,  -280,  -280,  -280,  -280,
    -280,  -280,  -280,  -280,  -280,   -12,   -12,  -280,  -280,    22,
    -280,    36,  -280,  -280,   463,    10,    53,  -280,   458,  -280,
    -280,  -280,  -280,  -280,    57,  -280,    25,    34,  -280,  -280,
      64,    59,   172,  -280,    40,    61,   135,    64,   154,    59,
    -280,    59,    78,  -280,  -280,  -280,   114,    64,   324,   120,
    -280,  -280,  -280,  -280,  -280,  -280,  -280,  -280,  -280,  -280,
    -280,  -280,  -280,  -280,  -280,  -280,  -280,    59,   -18,  -280,
     134,   143,  -280,  -280,   -30,  -280,   175,  -280,   179,  -280,
    -280,  -280,  -280,  -280,   182,   190,  -280,   197,   213,  -280,
    -280,   248,   222,   263,   234,   237,   261,   135,   258,  -280,
    -280,   276,   293,  -280,  -280,  -280,   142,   289,   332,   869,
     332,  -280,    64,  -280,   332,  -280,  -280,   332,   597,   269,
     332,    60,   332,  -280,    35,   461,   296,  -280,  -280,   332,
    -280,  -280,   287,  -280,  -280,  -280,  -280,  -280,  -280,  -280,
    -280,  -280,  -280,   332,   332,   825,   332,   332,   332,    -6,
     228,  -280,  -280,  -280,   301,  -280,  -280,   294,   103,  -280,
     433,   639,   654,   433,   478,    64,   306,   311,  -280,  -280,
     318,   -27,   313,   233,  -280,    13,  -280,  -280,   285,   696,
     319,    96,    37,  -280,    45,  -280,   330,    59,   326,    59,
    -280,  -280,   419,  -280,  -280,  -280,   332,   711,   372,  -280,
     753,  -280,  -280,  -280,  -280,   325,    48,  -280,   418,  -280,
    -280,   332,   332,   332,   332,   332,  -280,   332,   332,  -280,
     322,  -280,   323,   331,   520,  -280,   337,   130,   133,  -280,
    -280,   170,  -280,  -280,  -280,   341,   597,   290,  -280,  -280,
     343,    60,  -280,   344,    56,   189,  -280,  -280,  -280,   346,
    -280,   355,   -25,   358,   319,   377,   773,   375,   364,  -280,
     386,   368,  -280,   370,   332,  -280,   869,  -280,   -38,   433,
     253,   253,  -280,  -280,   433,   266,  -280,  -280,  -280,  -280,
      67,  -280,  -280,   540,  -280,   845,  -280,   161,  -280,  -280,
    -280,   433,  -280,  -280,  -280,  -280,  -280,    96,  -280,  -280,
    -280,  -280,   796,   433,   381,  -280,   227,  -280,   384,  -280,
    -280,  -280,  -280,    30,  -280,  -280,   332,  -280,  -280,   208,
     582,   239,   242,  -280,  -280,   180,  -280,  -280,  -280,   400,
      89,   -24,   405,  -280,   423,   112,  -280,  -280,   433,  -280,
    -280,  -280,  -280,  -280,  -280,  -280,  -280,   332,  -280,   113,
    -280,  -280,   403,   425,   384,   117,   427,   -24,  -280,  -280,
    -280,  -280,  -280,  -280
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      18,     4,    21,    22,    23,    24,    25,    26,    27,    28,
       0,     2,     3,     0,    17,    20,     1,     6,    12,    13,
      15,    14,    16,     7,     8,   218,   218,    19,   219,     0,
     217,     0,    10,    31,    18,   142,     0,     9,     0,   143,
     145,   144,   146,   147,     0,    29,     0,   141,     5,    11,
       0,   132,   131,   130,   133,     0,   134,   135,   136,   137,
     138,   139,   140,   125,   126,   127,     0,     0,   214,     0,
     215,    32,    34,    35,    30,    33,    36,    37,    39,    38,
      40,    41,    45,    46,    42,    43,    44,     0,   176,   129,
       0,   128,    47,   214,     0,    55,    56,   216,     0,   201,
     199,   202,   203,   200,     0,    60,   198,     0,     0,   211,
     210,     0,     0,     0,     0,     0,     0,     0,     0,   209,
     185,     0,   180,   184,   183,   182,     0,     0,     0,     0,
       0,    49,     0,    53,     0,    62,    62,     0,    66,     0,
       0,     0,     0,    62,     0,     0,     0,    50,    62,     0,
     213,   212,     0,    62,   132,   131,   133,   134,   135,   136,
     137,   139,   140,     0,     0,     0,     0,     0,     0,   176,
       0,   156,   173,   163,   161,   164,   128,   177,     0,    54,
      57,     0,     0,    59,    81,     0,     0,    65,    68,    73,
       0,   128,     0,     0,    86,     0,    85,    87,     0,     0,
       0,     0,     0,   116,     0,   121,     0,   136,   138,     0,
      99,   101,     0,    97,   102,   100,     0,     0,     0,    51,
       0,   158,   161,   157,   174,     0,     0,   171,     0,   159,
     160,   150,     0,     0,     0,     0,   178,     0,     0,    48,
       0,    61,     0,   201,     0,   195,   200,     0,     0,   169,
     168,     0,   189,   188,    72,     0,     0,     0,    52,    82,
       0,     0,    89,     0,     0,     0,   207,   208,   206,     0,
     205,     0,     0,     0,     0,     0,     0,     0,     0,    96,
       0,     0,    91,     0,   150,   172,     0,   165,     0,   149,
     152,   153,   151,   154,   155,     0,    63,    58,    80,   193,
       0,   192,    78,     0,    76,     0,    74,     0,    64,    67,
      70,    69,    83,    84,    88,   117,   204,     0,    93,   115,
      94,   120,     0,   119,     0,   106,     0,   104,     0,    95,
      90,    92,   123,     0,   170,   162,     0,   179,   194,     0,
       0,     0,     0,   167,   166,     0,   196,   187,   186,     0,
       0,     0,     0,   103,     0,     0,   113,   175,   148,   191,
     190,    79,    77,    75,   197,   122,   118,   150,   109,     0,
     108,    98,     0,     0,     0,     0,     0,     0,   114,   111,
     112,   110,   105,   107
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -280,  -280,  -280,  -280,  -280,   434,  -280,   443,  -280,   469,
    -280,  -280,   -45,  -280,  -280,  -280,  -280,   356,  -280,  -280,
      51,  -280,  -280,  -280,  -280,   244,   251,  -280,  -280,  -280,
    -280,   249,   466,  -280,  -280,  -280,  -280,  -280,  -280,   302,
    -280,   187,  -280,   138,  -280,  -280,   144,  -280,   257,  -196,
     259,   470,  -280,   -46,  -280,  -280,  -280,  -279,    63,     5,
     232,  -280,  -176,   231,  -181,   -35,  -280,   474,   247,  -280,
     240,  -280,   500,  -182,   236,   291,  -280,   -44,  -280,   -37,
     -23,   528,  -280
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    10,    11,    25,    34,    12,    26,    13,    14,    15,
      35,    45,   241,    72,    73,    74,    94,    95,    75,   104,
     181,    76,    77,   186,   187,   188,   189,   247,    78,    79,
     195,   196,   211,    81,    82,    83,    84,    85,   212,   213,
     326,   327,   369,   370,   214,   355,   356,   202,   203,   204,
     205,   215,    87,   169,    89,    46,    47,   288,   289,   171,
     248,   226,   172,   173,   227,   174,   121,   175,   251,   300,
     252,   347,   197,   106,   269,   270,   123,   124,   152,   176,
     125,    29,    30
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      88,    71,   253,   250,   264,   333,   335,     1,   249,    91,
     336,    90,   111,    96,   113,   -71,   200,   367,   132,   133,
     112,   -71,    39,    40,    41,    42,    43,   128,    98,   129,
     118,    93,    16,   368,    70,   114,   115,   231,   116,   128,
      28,   129,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    44,    60,    61,   260,    62,    63,    64,    65,
      66,   261,   301,    32,   127,     2,     3,     4,     5,     6,
       7,     8,     9,   146,   357,    67,   200,    33,   336,   271,
     201,    68,    69,   177,    70,   272,    92,   273,   375,    99,
     285,    93,    88,   274,    70,    96,   286,   107,   315,    88,
     210,   191,    48,   190,   274,   334,    49,   206,    91,   338,
      90,    97,   100,   101,   102,   339,   103,   194,   108,   225,
      93,   253,   250,    70,   344,   348,   350,   249,   222,   222,
    -124,   366,   222,   222,   265,    88,    88,   274,   225,   117,
     232,   233,   234,   235,    91,    91,    90,    90,   254,   266,
     267,   268,   239,    88,   373,   376,   126,   360,   301,   381,
     374,   377,    91,   301,    90,   336,    88,   210,   221,   223,
     130,    88,   229,   230,    88,    91,   302,    90,   303,   304,
      91,   305,    90,    91,   114,    90,   277,   182,   109,   110,
      99,   170,   131,   178,   199,   150,   151,   180,   225,   217,
     183,    99,   345,   193,   220,   198,    97,   109,   110,    99,
      88,   134,   218,   245,   101,   102,   306,   103,   307,   191,
     135,   190,   364,   136,   100,   101,   102,   137,   103,   228,
      88,   325,   299,   101,   102,   206,   103,    99,   138,    91,
     225,    90,   266,   267,   268,   154,   155,    53,   156,   139,
     157,   158,   159,   160,   324,    60,   161,   225,   162,   225,
     359,   101,   102,   141,   103,   232,   233,   234,   235,   352,
     232,   233,   234,   235,   236,   143,   225,    67,   144,   280,
      88,   325,   259,    93,   140,   362,    70,   305,   363,    91,
     307,    90,   234,   235,   225,   290,   291,   292,   293,   142,
     294,   295,   145,   232,   233,   234,   235,   147,   154,   155,
      53,   156,   337,   157,   158,   159,   160,   148,    60,   161,
     311,   162,   232,   233,   234,   235,   192,   163,   164,   149,
     153,   165,   216,   166,   262,   184,   219,   237,   323,   238,
     167,   168,    97,   109,   110,   119,    93,   120,   255,    70,
     154,   155,    53,   156,   257,   157,   158,   159,   160,   256,
      60,   161,   258,   162,   201,  -181,   275,   276,   284,   163,
     164,   296,   297,   165,  -139,   166,    97,   109,   110,   119,
    -214,   120,   167,   168,    97,   109,   110,   119,    93,   120,
     308,    70,   312,   314,   317,   154,   155,    53,   156,   358,
     157,   158,   159,   160,   318,    60,   161,   320,   162,   232,
     233,   234,   235,   329,   163,   164,   328,   331,   322,   332,
     166,   282,   351,   232,   233,   234,   235,   167,   168,    97,
     109,   110,   119,    93,   120,   330,    70,   154,   155,    53,
     156,   354,   157,   158,   207,   160,   365,   208,   161,   209,
      62,    63,    64,    65,   371,   232,   233,   234,   235,   372,
     378,   278,   287,    18,    19,    20,    21,    22,    37,    67,
     232,   233,   234,   235,   379,    93,   382,    38,    70,   154,
     155,    53,   156,    27,   157,   158,   207,   160,   179,   208,
     161,   209,    62,    63,    64,    65,   154,   155,    53,   156,
     309,   157,   158,   159,   160,    36,    60,   243,   310,   162,
     313,    67,    80,   353,   279,   383,    86,    93,   380,   244,
      70,     2,     3,     4,     5,     6,     7,     8,     9,   319,
     245,   101,   102,   321,   246,   341,   343,    70,   154,   155,
      53,   156,   122,   157,   158,   159,   160,   346,    60,   243,
     342,   162,   105,   349,    31,     0,   316,     0,   154,   155,
      53,   156,   298,   157,   158,   159,   160,     0,    60,   243,
       0,   162,   299,   101,   102,     0,   246,     0,     0,    70,
       0,   340,    17,    18,    19,    20,    21,    22,    23,    24,
       0,     0,   245,   101,   102,     0,   246,     0,     0,    70,
     154,   155,    53,   156,     0,   157,   158,   159,   160,     0,
      60,   243,     0,   162,     0,   154,   155,    53,   156,     0,
     157,   158,   159,   160,   361,    60,   161,     0,   162,     0,
       0,     0,     0,     0,   299,   101,   102,     0,   246,     0,
       0,    70,   184,     0,     0,     0,     0,   185,     0,     0,
       0,     0,     0,    93,     0,     0,    70,   154,   155,    53,
     156,     0,   157,   158,   159,   160,     0,    60,   161,     0,
     162,     0,   154,   155,    53,   156,     0,   157,   158,   159,
     160,   240,    60,   161,     0,   162,     0,     0,     0,    67,
       0,     0,     0,     0,     0,    93,   242,     0,    70,     0,
       0,     0,     0,     0,    67,     0,     0,     0,     0,     0,
      93,     0,     0,    70,   154,   155,    53,   156,     0,   157,
     158,   159,   160,     0,    60,   161,     0,   162,     0,   154,
     155,    53,   156,     0,   157,   158,   159,   160,   263,    60,
     161,     0,   162,     0,     0,     0,    67,     0,     0,     0,
       0,     0,    93,   281,     0,    70,     0,     0,     0,     0,
       0,    67,     0,     0,     0,     0,     0,    93,     0,     0,
      70,   154,   155,    53,   156,     0,   157,   158,   159,   160,
       0,    60,   161,     0,   162,     0,     0,     0,     0,     0,
       0,   154,   155,    53,   156,   283,   157,   158,   159,   160,
     324,    60,   161,    67,   162,     0,     0,     0,     0,    93,
       0,     0,    70,     0,   154,   155,    53,   156,     0,   157,
     158,   159,   160,    67,    60,   161,     0,   162,     0,    93,
       0,     0,    70,     0,     0,     0,     0,     0,   224,     0,
       0,   201,     0,   154,   155,    53,   156,     0,   157,   158,
     159,   160,    93,    60,   161,    70,   162,     0,     0,     0,
       0,     0,     0,   154,   155,    53,   156,   224,   157,   158,
     159,   160,     0,    60,   161,     0,   162,     0,     0,     0,
       0,    93,     0,     0,    70,     0,   165,   154,   155,    53,
     156,     0,   157,   158,   159,   160,     0,    60,   161,     0,
     162,    93,     0,     0,    70,     2,     3,     4,     5,     6,
       7,     8,     9,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    93,     0,     0,    70
};

static const yytype_int16 yycheck[] =
{
      46,    46,   184,   184,   200,   284,    44,     0,   184,    46,
      48,    46,    56,    50,    58,    42,    41,    41,    48,    49,
      57,    48,    12,    13,    14,    15,    16,    45,    51,    47,
      67,    56,     0,    57,    59,    58,    59,    43,    61,    45,
      52,    47,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    42,    28,    29,    42,    31,    32,    33,    34,
      35,    48,   244,    41,    87,    58,    59,    60,    61,    62,
      63,    64,    65,   117,    44,    50,    41,    41,    48,    42,
      45,    56,    57,   129,    59,    48,    52,    42,   367,    29,
      42,    56,   138,    48,    59,   132,    48,    57,    42,   145,
     145,   138,    49,   138,    48,   286,    49,   144,   145,    42,
     145,    52,    52,    53,    54,    48,    56,    57,    57,   165,
      56,   303,   303,    59,   305,   307,   322,   303,   163,   164,
      52,    42,   167,   168,    38,   181,   182,    48,   184,    25,
      37,    38,    39,    40,   181,   182,   181,   182,   185,    53,
      54,    55,    49,   199,    42,    42,    36,   339,   340,    42,
      48,    48,   199,   345,   199,    48,   212,   212,   163,   164,
      36,   217,   167,   168,   220,   212,    46,   212,    48,    46,
     217,    48,   217,   220,   207,   220,   209,   136,    53,    54,
      29,   128,    49,   130,   143,    53,    54,   134,   244,   148,
     137,    29,    41,   140,   153,   142,    52,    53,    54,    29,
     256,    36,   149,    52,    53,    54,    46,    56,    48,   256,
      41,   256,    42,    41,    52,    53,    54,    37,    56,   166,
     276,   276,    52,    53,    54,   272,    56,    29,    41,   276,
     286,   276,    53,    54,    55,    18,    19,    20,    21,    36,
      23,    24,    25,    26,    27,    28,    29,   303,    31,   305,
      52,    53,    54,    41,    56,    37,    38,    39,    40,    42,
      37,    38,    39,    40,    46,    41,   322,    50,    41,   216,
     326,   326,    49,    56,    36,    46,    59,    48,    46,   326,
      48,   326,    39,    40,   340,   232,   233,   234,   235,    36,
     237,   238,    41,    37,    38,    39,    40,    49,    18,    19,
      20,    21,    46,    23,    24,    25,    26,    41,    28,    29,
     257,    31,    37,    38,    39,    40,    57,    37,    38,    36,
      41,    41,    36,    43,    49,    45,    49,    36,   275,    45,
      50,    51,    52,    53,    54,    55,    56,    57,    42,    59,
      18,    19,    20,    21,    36,    23,    24,    25,    26,    48,
      28,    29,    49,    31,    45,    41,    36,    41,    43,    37,
      38,    49,    49,    41,    43,    43,    52,    53,    54,    55,
      43,    57,    50,    51,    52,    53,    54,    55,    56,    57,
      49,    59,    49,    49,    48,    18,    19,    20,    21,   336,
      23,    24,    25,    26,    49,    28,    29,    49,    31,    37,
      38,    39,    40,    49,    37,    38,    41,    49,    41,    49,
      43,    49,    41,    37,    38,    39,    40,    50,    51,    52,
      53,    54,    55,    56,    57,    49,    59,    18,    19,    20,
      21,    57,    23,    24,    25,    26,    46,    28,    29,    30,
      31,    32,    33,    34,    49,    37,    38,    39,    40,    36,
      57,    42,    44,     5,     6,     7,     8,     9,    34,    50,
      37,    38,    39,    40,    49,    56,    49,    34,    59,    18,
      19,    20,    21,    14,    23,    24,    25,    26,   132,    28,
      29,    30,    31,    32,    33,    34,    18,    19,    20,    21,
     256,    23,    24,    25,    26,    42,    28,    29,   257,    31,
     261,    50,    46,   326,   212,   377,    46,    56,   374,    41,
      59,    58,    59,    60,    61,    62,    63,    64,    65,   272,
      52,    53,    54,   274,    56,   303,   305,    59,    18,    19,
      20,    21,    68,    23,    24,    25,    26,   307,    28,    29,
     303,    31,    52,   317,    26,    -1,   265,    -1,    18,    19,
      20,    21,    42,    23,    24,    25,    26,    -1,    28,    29,
      -1,    31,    52,    53,    54,    -1,    56,    -1,    -1,    59,
      -1,    41,     4,     5,     6,     7,     8,     9,    10,    11,
      -1,    -1,    52,    53,    54,    -1,    56,    -1,    -1,    59,
      18,    19,    20,    21,    -1,    23,    24,    25,    26,    -1,
      28,    29,    -1,    31,    -1,    18,    19,    20,    21,    -1,
      23,    24,    25,    26,    42,    28,    29,    -1,    31,    -1,
      -1,    -1,    -1,    -1,    52,    53,    54,    -1,    56,    -1,
      -1,    59,    45,    -1,    -1,    -1,    -1,    50,    -1,    -1,
      -1,    -1,    -1,    56,    -1,    -1,    59,    18,    19,    20,
      21,    -1,    23,    24,    25,    26,    -1,    28,    29,    -1,
      31,    -1,    18,    19,    20,    21,    -1,    23,    24,    25,
      26,    42,    28,    29,    -1,    31,    -1,    -1,    -1,    50,
      -1,    -1,    -1,    -1,    -1,    56,    42,    -1,    59,    -1,
      -1,    -1,    -1,    -1,    50,    -1,    -1,    -1,    -1,    -1,
      56,    -1,    -1,    59,    18,    19,    20,    21,    -1,    23,
      24,    25,    26,    -1,    28,    29,    -1,    31,    -1,    18,
      19,    20,    21,    -1,    23,    24,    25,    26,    42,    28,
      29,    -1,    31,    -1,    -1,    -1,    50,    -1,    -1,    -1,
      -1,    -1,    56,    42,    -1,    59,    -1,    -1,    -1,    -1,
      -1,    50,    -1,    -1,    -1,    -1,    -1,    56,    -1,    -1,
      59,    18,    19,    20,    21,    -1,    23,    24,    25,    26,
      -1,    28,    29,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    18,    19,    20,    21,    42,    23,    24,    25,    26,
      27,    28,    29,    50,    31,    -1,    -1,    -1,    -1,    56,
      -1,    -1,    59,    -1,    18,    19,    20,    21,    -1,    23,
      24,    25,    26,    50,    28,    29,    -1,    31,    -1,    56,
      -1,    -1,    59,    -1,    -1,    -1,    -1,    -1,    42,    -1,
      -1,    45,    -1,    18,    19,    20,    21,    -1,    23,    24,
      25,    26,    56,    28,    29,    59,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    18,    19,    20,    21,    42,    23,    24,
      25,    26,    -1,    28,    29,    -1,    31,    -1,    -1,    -1,
      -1,    56,    -1,    -1,    59,    -1,    41,    18,    19,    20,
      21,    -1,    23,    24,    25,    26,    -1,    28,    29,    -1,
      31,    56,    -1,    -1,    59,    58,    59,    60,    61,    62,
      63,    64,    65,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    56,    -1,    -1,    59
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     0,    58,    59,    60,    61,    62,    63,    64,    65,
      67,    68,    71,    73,    74,    75,     0,     4,     5,     6,
       7,     8,     9,    10,    11,    69,    72,    75,    52,   147,
     148,   147,    41,    41,    70,    76,    42,    71,    73,    12,
      13,    14,    15,    16,    42,    77,   121,   122,    49,    49,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      28,    29,    31,    32,    33,    34,    35,    50,    56,    57,
      59,    78,    79,    80,    81,    84,    87,    88,    94,    95,
      98,    99,   100,   101,   102,   103,   117,   118,   119,   120,
     131,   145,    52,    56,    82,    83,   145,    52,   146,    29,
      52,    53,    54,    56,    85,   138,   139,    57,    57,    53,
      54,   143,   145,   143,   146,   146,   146,    25,   145,    55,
      57,   132,   133,   142,   143,   146,    36,   146,    45,    47,
      36,    49,    48,    49,    36,    41,    41,    37,    41,    36,
      36,    41,    36,    41,    41,    41,   143,    49,    41,    36,
      53,    54,   144,    41,    18,    19,    21,    23,    24,    25,
      26,    29,    31,    37,    38,    41,    43,    50,    51,   119,
     124,   125,   128,   129,   131,   133,   145,   119,   124,    83,
     124,    86,    86,   124,    45,    50,    89,    90,    91,    92,
     131,   145,    57,   124,    57,    96,    97,   138,   124,    86,
      41,    45,   113,   114,   115,   116,   145,    25,    28,    30,
      78,    98,   104,   105,   110,   117,    36,    86,   124,    49,
      86,   125,   131,   125,    42,   119,   127,   130,   124,   125,
     125,    43,    37,    38,    39,    40,    46,    36,    45,    49,
      42,    78,    42,    29,    41,    52,    56,    93,   126,   128,
     130,   134,   136,   139,   145,    42,    48,    36,    49,    49,
      42,    48,    49,    42,   115,    38,    53,    54,    55,   140,
     141,    42,    48,    42,    48,    36,    41,   146,    42,   105,
     124,    42,    49,    42,    43,    42,    48,    44,   123,   124,
     124,   124,   124,   124,   124,   124,    49,    49,    42,    52,
     135,   139,    46,    48,    46,    48,    46,    48,    49,    91,
      92,   124,    49,    97,    49,    42,   141,    48,    49,   114,
      49,   116,    41,   124,    27,    78,   106,   107,    41,    49,
      49,    49,    49,   123,   130,    44,    48,    46,    42,    48,
      41,   126,   134,   129,   130,    41,   136,   137,   139,   140,
     115,    41,    42,   107,    57,   111,   112,    44,   124,    52,
     139,    42,    46,    46,    42,    46,    42,    41,    57,   108,
     109,    49,    36,    42,    48,   123,    42,    48,    57,    49,
     112,    42,    49,   109
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    66,    67,    67,    67,    68,    69,    69,    69,    70,
      70,    71,    72,    72,    72,    72,    72,    73,    73,    74,
      74,    75,    75,    75,    75,    75,    75,    75,    75,    76,
      76,    76,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    78,    78,
      78,    79,    80,    81,    82,    82,    83,    83,    84,    85,
      85,    86,    86,    87,    88,    89,    89,    90,    90,    91,
      91,    91,    91,    91,    92,    92,    92,    92,    92,    93,
      93,    93,    94,    95,    96,    96,    97,    97,    98,    99,
      99,   100,   101,   102,   102,   103,   104,   104,   105,   105,
     105,   105,   105,   106,   106,   107,   107,   108,   108,   109,
     109,   110,   111,   111,   112,   113,   113,   114,   114,   114,
     115,   115,   116,   117,   118,   118,   118,   118,   119,   119,
     120,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   121,   121,   122,   122,   122,   122,   122,   123,   123,
     123,   124,   124,   124,   124,   124,   124,   125,   125,   125,
     125,   125,   125,   125,   125,   125,   126,   126,   126,   126,
     127,   127,   128,   129,   129,   130,   131,   131,   131,   131,
     132,   132,   133,   133,   133,   133,   134,   134,   134,   134,
     135,   135,   135,   135,   136,   136,   137,   137,   138,   138,
     139,   139,   139,   139,   140,   140,   141,   141,   141,   142,
     143,   143,   144,   144,   145,   145,   146,   147,   147,   148
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     7,     1,     1,     1,     2,
       0,     7,     1,     1,     1,     1,     1,     1,     0,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       3,     0,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     4,     2,
       3,     4,     5,     3,     3,     1,     1,     3,     6,     3,
       1,     2,     0,     6,     6,     1,     0,     3,     1,     3,
       3,     1,     2,     1,     3,     5,     3,     5,     3,     4,
       2,     0,     5,     6,     3,     1,     1,     1,     6,     5,
       6,     5,     6,     6,     6,     6,     2,     1,     5,     1,
       1,     1,     1,     2,     1,     5,     1,     3,     1,     1,
       3,     6,     3,     1,     3,     3,     1,     3,     5,     3,
       3,     1,     5,     6,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     1,     1,     1,     1,     1,     3,     1,
       0,     3,     3,     3,     3,     3,     1,     2,     2,     2,
       2,     1,     4,     1,     1,     3,     3,     3,     1,     1,
       3,     1,     3,     1,     2,     4,     1,     3,     4,     6,
       1,     0,     1,     1,     1,     1,     3,     3,     1,     1,
       3,     3,     1,     1,     3,     1,     1,     2,     1,     1,
       1,     1,     1,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     1
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
#line 252 "src/xkbcomp/parser.y"
            { free(((*yyvaluep).str)); }
#line 1627 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_XkbFile: /* XkbFile  */
#line 250 "src/xkbcomp/parser.y"
            { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1633 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_XkbCompositeMap: /* XkbCompositeMap  */
#line 250 "src/xkbcomp/parser.y"
            { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1639 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_XkbMapConfigList: /* XkbMapConfigList  */
#line 251 "src/xkbcomp/parser.y"
            { FreeXkbFile(((*yyvaluep).fileList).head); }
#line 1645 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_XkbMapConfig: /* XkbMapConfig  */
#line 250 "src/xkbcomp/parser.y"
            { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1651 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_DeclList: /* DeclList  */
#line 246 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).anyList).head); }
#line 1657 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Decl: /* Decl  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).any)); }
#line 1663 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VarDecl: /* VarDecl  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).var)); }
#line 1669 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyNameDecl: /* KeyNameDecl  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).keyCode)); }
#line 1675 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyAliasDecl: /* KeyAliasDecl  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).keyAlias)); }
#line 1681 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VModDecl: /* VModDecl  */
#line 246 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).vmodList).head); }
#line 1687 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VModDefList: /* VModDefList  */
#line 246 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).vmodList).head); }
#line 1693 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VModDef: /* VModDef  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).vmod)); }
#line 1699 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_InterpretDecl: /* InterpretDecl  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).interp)); }
#line 1705 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_InterpretMatch: /* InterpretMatch  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).interp)); }
#line 1711 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VarDeclList: /* VarDeclList  */
#line 246 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).varList).head); }
#line 1717 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyTypeDecl: /* KeyTypeDecl  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).keyType)); }
#line 1723 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_SymbolsDecl: /* SymbolsDecl  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).syms)); }
#line 1729 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_OptSymbolsBody: /* OptSymbolsBody  */
#line 246 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).varList).head); }
#line 1735 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_SymbolsBody: /* SymbolsBody  */
#line 246 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).varList).head); }
#line 1741 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_SymbolsVarDecl: /* SymbolsVarDecl  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).var)); }
#line 1747 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_MultiKeySymOrActionList: /* MultiKeySymOrActionList  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1753 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_GroupCompatDecl: /* GroupCompatDecl  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).groupCompat)); }
#line 1759 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_ModMapDecl: /* ModMapDecl  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).modMask)); }
#line 1765 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyOrKeySymList: /* KeyOrKeySymList  */
#line 246 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1771 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyOrKeySym: /* KeyOrKeySym  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1777 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_LedMapDecl: /* LedMapDecl  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).ledMap)); }
#line 1783 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_LedNameDecl: /* LedNameDecl  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).ledName)); }
#line 1789 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_CoordList: /* CoordList  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1795 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Coord: /* Coord  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1801 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_ExprList: /* ExprList  */
#line 246 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1807 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Expr: /* Expr  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1813 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Term: /* Term  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1819 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_MultiActionList: /* MultiActionList  */
#line 246 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1825 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_ActionList: /* ActionList  */
#line 246 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1831 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_NonEmptyActions: /* NonEmptyActions  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1837 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Actions: /* Actions  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1843 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Action: /* Action  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1849 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Lhs: /* Lhs  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1855 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_OptTerminal: /* OptTerminal  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1861 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Terminal: /* Terminal  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1867 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_MultiKeySymList: /* MultiKeySymList  */
#line 246 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1873 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeySymList: /* KeySymList  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1879 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_NonEmptyKeySyms: /* NonEmptyKeySyms  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1885 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeySyms: /* KeySyms  */
#line 243 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1891 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_OptMapName: /* OptMapName  */
#line 252 "src/xkbcomp/parser.y"
            { free(((*yyvaluep).str)); }
#line 1897 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_MapName: /* MapName  */
#line 252 "src/xkbcomp/parser.y"
            { free(((*yyvaluep).str)); }
#line 1903 "src/xkbcomp/parser.c"
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
#line 269 "src/xkbcomp/parser.y"
                        { (yyval.file) = param->rtrn = (yyvsp[0].file); param->more_maps = !!param->rtrn; (void) yynerrs; }
#line 2182 "src/xkbcomp/parser.c"
    break;

  case 3: /* XkbFile: XkbMapConfig  */
#line 271 "src/xkbcomp/parser.y"
                        { (yyval.file) = param->rtrn = (yyvsp[0].file); param->more_maps = !!param->rtrn; YYACCEPT; }
#line 2188 "src/xkbcomp/parser.c"
    break;

  case 4: /* XkbFile: "end of file"  */
#line 273 "src/xkbcomp/parser.y"
                        { (yyval.file) = param->rtrn = NULL; param->more_maps = false; }
#line 2194 "src/xkbcomp/parser.c"
    break;

  case 5: /* XkbCompositeMap: OptFlags XkbCompositeType OptMapName "{" XkbMapConfigList "}" ";"  */
#line 279 "src/xkbcomp/parser.y"
                        { (yyval.file) = XkbFileCreate((yyvsp[-5].file_type), (yyvsp[-4].str), (ParseCommon *) (yyvsp[-2].fileList).head, (yyvsp[-6].mapFlags)); }
#line 2200 "src/xkbcomp/parser.c"
    break;

  case 6: /* XkbCompositeType: "xkb_keymap"  */
#line 282 "src/xkbcomp/parser.y"
                                        { (yyval.file_type) = FILE_TYPE_KEYMAP; }
#line 2206 "src/xkbcomp/parser.c"
    break;

  case 7: /* XkbCompositeType: "xkb_semantics"  */
#line 283 "src/xkbcomp/parser.y"
                                        { (yyval.file_type) = FILE_TYPE_KEYMAP; }
#line 2212 "src/xkbcomp/parser.c"
    break;

  case 8: /* XkbCompositeType: "xkb_layout"  */
#line 284 "src/xkbcomp/parser.y"
                                        { (yyval.file_type) = FILE_TYPE_KEYMAP; }
#line 2218 "src/xkbcomp/parser.c"
    break;

  case 9: /* XkbMapConfigList: XkbMapConfigList XkbMapConfig  */
#line 291 "src/xkbcomp/parser.y"
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
#line 2234 "src/xkbcomp/parser.c"
    break;

  case 10: /* XkbMapConfigList: %empty  */
#line 302 "src/xkbcomp/parser.y"
                        { (yyval.fileList).head = (yyval.fileList).last = NULL; }
#line 2240 "src/xkbcomp/parser.c"
    break;

  case 11: /* XkbMapConfig: OptFlags FileType OptMapName "{" DeclList "}" ";"  */
#line 308 "src/xkbcomp/parser.y"
                        {
                            (yyval.file) = XkbFileCreate((yyvsp[-5].file_type), (yyvsp[-4].str), (yyvsp[-2].anyList).head, (yyvsp[-6].mapFlags));
                        }
#line 2248 "src/xkbcomp/parser.c"
    break;

  case 12: /* FileType: "xkb_keycodes"  */
#line 313 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_KEYCODES; }
#line 2254 "src/xkbcomp/parser.c"
    break;

  case 13: /* FileType: "xkb_types"  */
#line 314 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_TYPES; }
#line 2260 "src/xkbcomp/parser.c"
    break;

  case 14: /* FileType: "xkb_compatibility"  */
#line 315 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_COMPAT; }
#line 2266 "src/xkbcomp/parser.c"
    break;

  case 15: /* FileType: "xkb_symbols"  */
#line 316 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_SYMBOLS; }
#line 2272 "src/xkbcomp/parser.c"
    break;

  case 16: /* FileType: "xkb_geometry"  */
#line 317 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_GEOMETRY; }
#line 2278 "src/xkbcomp/parser.c"
    break;

  case 17: /* OptFlags: Flags  */
#line 320 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = (yyvsp[0].mapFlags); }
#line 2284 "src/xkbcomp/parser.c"
    break;

  case 18: /* OptFlags: %empty  */
#line 321 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = 0; }
#line 2290 "src/xkbcomp/parser.c"
    break;

  case 19: /* Flags: Flags Flag  */
#line 324 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = ((yyvsp[-1].mapFlags) | (yyvsp[0].mapFlags)); }
#line 2296 "src/xkbcomp/parser.c"
    break;

  case 20: /* Flags: Flag  */
#line 325 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = (yyvsp[0].mapFlags); }
#line 2302 "src/xkbcomp/parser.c"
    break;

  case 21: /* Flag: "partial"  */
#line 328 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_PARTIAL; }
#line 2308 "src/xkbcomp/parser.c"
    break;

  case 22: /* Flag: "default"  */
#line 329 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_DEFAULT; }
#line 2314 "src/xkbcomp/parser.c"
    break;

  case 23: /* Flag: "hidden"  */
#line 330 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_HIDDEN; }
#line 2320 "src/xkbcomp/parser.c"
    break;

  case 24: /* Flag: "alphanumeric_keys"  */
#line 331 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_HAS_ALPHANUMERIC; }
#line 2326 "src/xkbcomp/parser.c"
    break;

  case 25: /* Flag: "modifier_keys"  */
#line 332 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_HAS_MODIFIER; }
#line 2332 "src/xkbcomp/parser.c"
    break;

  case 26: /* Flag: "keypad_keys"  */
#line 333 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_HAS_KEYPAD; }
#line 2338 "src/xkbcomp/parser.c"
    break;

  case 27: /* Flag: "function_keys"  */
#line 334 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_HAS_FN; }
#line 2344 "src/xkbcomp/parser.c"
    break;

  case 28: /* Flag: "alternate_group"  */
#line 335 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_ALTGR; }
#line 2350 "src/xkbcomp/parser.c"
    break;

  case 29: /* DeclList: DeclList Decl  */
#line 339 "src/xkbcomp/parser.y"
                        {
                            if ((yyvsp[0].any)) {
                                if ((yyvsp[-1].anyList).head) {
                                    (yyval.anyList).head = (yyvsp[-1].anyList).head; (yyvsp[-1].anyList).last->next = (yyvsp[0].any); (yyval.anyList).last = (yyvsp[0].any);
                                } else {
                                    (yyval.anyList).head = (yyval.anyList).last = (yyvsp[0].any);
                                }
                            }
                        }
#line 2364 "src/xkbcomp/parser.c"
    break;

  case 30: /* DeclList: DeclList OptMergeMode VModDecl  */
#line 354 "src/xkbcomp/parser.y"
                        {
                            for (VModDef *vmod = (yyvsp[0].vmodList).head; vmod; vmod = (VModDef *) vmod->common.next)
                                vmod->merge = (yyvsp[-1].merge);
                            if ((yyvsp[-2].anyList).head) {
                                (yyval.anyList).head = (yyvsp[-2].anyList).head; (yyvsp[-2].anyList).last->next = &(yyvsp[0].vmodList).head->common; (yyval.anyList).last = &(yyvsp[0].vmodList).last->common;
                            } else {
                                (yyval.anyList).head = &(yyvsp[0].vmodList).head->common; (yyval.anyList).last = &(yyvsp[0].vmodList).last->common;
                            }
                        }
#line 2378 "src/xkbcomp/parser.c"
    break;

  case 31: /* DeclList: %empty  */
#line 363 "src/xkbcomp/parser.y"
                        { (yyval.anyList).head = (yyval.anyList).last = NULL; }
#line 2384 "src/xkbcomp/parser.c"
    break;

  case 32: /* Decl: OptMergeMode VarDecl  */
#line 367 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].var)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].var);
                        }
#line 2393 "src/xkbcomp/parser.c"
    break;

  case 33: /* Decl: OptMergeMode InterpretDecl  */
#line 373 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].interp)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].interp);
                        }
#line 2402 "src/xkbcomp/parser.c"
    break;

  case 34: /* Decl: OptMergeMode KeyNameDecl  */
#line 378 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].keyCode)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].keyCode);
                        }
#line 2411 "src/xkbcomp/parser.c"
    break;

  case 35: /* Decl: OptMergeMode KeyAliasDecl  */
#line 383 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].keyAlias)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].keyAlias);
                        }
#line 2420 "src/xkbcomp/parser.c"
    break;

  case 36: /* Decl: OptMergeMode KeyTypeDecl  */
#line 388 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].keyType)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].keyType);
                        }
#line 2429 "src/xkbcomp/parser.c"
    break;

  case 37: /* Decl: OptMergeMode SymbolsDecl  */
#line 393 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].syms)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].syms);
                        }
#line 2438 "src/xkbcomp/parser.c"
    break;

  case 38: /* Decl: OptMergeMode ModMapDecl  */
#line 398 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].modMask)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].modMask);
                        }
#line 2447 "src/xkbcomp/parser.c"
    break;

  case 39: /* Decl: OptMergeMode GroupCompatDecl  */
#line 403 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].groupCompat)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].groupCompat);
                        }
#line 2456 "src/xkbcomp/parser.c"
    break;

  case 40: /* Decl: OptMergeMode LedMapDecl  */
#line 408 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].ledMap)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].ledMap);
                        }
#line 2465 "src/xkbcomp/parser.c"
    break;

  case 41: /* Decl: OptMergeMode LedNameDecl  */
#line 413 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].ledName)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].ledName);
                        }
#line 2474 "src/xkbcomp/parser.c"
    break;

  case 42: /* Decl: OptMergeMode ShapeDecl  */
#line 417 "src/xkbcomp/parser.y"
                                                        { (yyval.any) = NULL; }
#line 2480 "src/xkbcomp/parser.c"
    break;

  case 43: /* Decl: OptMergeMode SectionDecl  */
#line 418 "src/xkbcomp/parser.y"
                                                        { (yyval.any) = NULL; }
#line 2486 "src/xkbcomp/parser.c"
    break;

  case 44: /* Decl: OptMergeMode DoodadDecl  */
#line 419 "src/xkbcomp/parser.y"
                                                        { (yyval.any) = NULL; }
#line 2492 "src/xkbcomp/parser.c"
    break;

  case 45: /* Decl: OptMergeMode UnknownDecl  */
#line 421 "src/xkbcomp/parser.y"
                            { (yyval.any) = (ParseCommon *) (yyvsp[0].unknown); }
#line 2498 "src/xkbcomp/parser.c"
    break;

  case 46: /* Decl: OptMergeMode UnknownCompoundStatementDecl  */
#line 423 "src/xkbcomp/parser.y"
                            { (yyval.any) = (ParseCommon *) (yyvsp[0].unknown); }
#line 2504 "src/xkbcomp/parser.c"
    break;

  case 47: /* Decl: MergeMode "string literal"  */
#line 425 "src/xkbcomp/parser.y"
                        {
                            (yyval.any) = (ParseCommon *) IncludeCreate(param->ctx, (yyvsp[0].str), (yyvsp[-1].merge));
                            free((yyvsp[0].str));
                        }
#line 2513 "src/xkbcomp/parser.c"
    break;

  case 48: /* VarDecl: Lhs "=" Expr ";"  */
#line 432 "src/xkbcomp/parser.y"
                        { (yyval.var) = VarCreate((yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 2519 "src/xkbcomp/parser.c"
    break;

  case 49: /* VarDecl: Ident ";"  */
#line 434 "src/xkbcomp/parser.y"
                        { (yyval.var) = BoolVarCreate((yyvsp[-1].atom), true); }
#line 2525 "src/xkbcomp/parser.c"
    break;

  case 50: /* VarDecl: "!" Ident ";"  */
#line 436 "src/xkbcomp/parser.y"
                        { (yyval.var) = BoolVarCreate((yyvsp[-1].atom), false); }
#line 2531 "src/xkbcomp/parser.c"
    break;

  case 51: /* KeyNameDecl: "key name" "=" KeyCode ";"  */
#line 440 "src/xkbcomp/parser.y"
                        { (yyval.keyCode) = KeycodeCreate((yyvsp[-3].atom), (yyvsp[-1].num)); }
#line 2537 "src/xkbcomp/parser.c"
    break;

  case 52: /* KeyAliasDecl: "alias" "key name" "=" "key name" ";"  */
#line 444 "src/xkbcomp/parser.y"
                        { (yyval.keyAlias) = KeyAliasCreate((yyvsp[-3].atom), (yyvsp[-1].atom)); }
#line 2543 "src/xkbcomp/parser.c"
    break;

  case 53: /* VModDecl: "virtual_modifiers" VModDefList ";"  */
#line 448 "src/xkbcomp/parser.y"
                        { (yyval.vmodList) = (yyvsp[-1].vmodList); }
#line 2549 "src/xkbcomp/parser.c"
    break;

  case 54: /* VModDefList: VModDefList "," VModDef  */
#line 452 "src/xkbcomp/parser.y"
                        { (yyval.vmodList).head = (yyvsp[-2].vmodList).head; (yyval.vmodList).last->common.next = &(yyvsp[0].vmod)->common; (yyval.vmodList).last = (yyvsp[0].vmod); }
#line 2555 "src/xkbcomp/parser.c"
    break;

  case 55: /* VModDefList: VModDef  */
#line 454 "src/xkbcomp/parser.y"
                        { (yyval.vmodList).head = (yyval.vmodList).last = (yyvsp[0].vmod); }
#line 2561 "src/xkbcomp/parser.c"
    break;

  case 56: /* VModDef: Ident  */
#line 458 "src/xkbcomp/parser.y"
                        { (yyval.vmod) = VModCreate((yyvsp[0].atom), NULL); }
#line 2567 "src/xkbcomp/parser.c"
    break;

  case 57: /* VModDef: Ident "=" Expr  */
#line 460 "src/xkbcomp/parser.y"
                        { (yyval.vmod) = VModCreate((yyvsp[-2].atom), (yyvsp[0].expr)); }
#line 2573 "src/xkbcomp/parser.c"
    break;

  case 58: /* InterpretDecl: "interpret" InterpretMatch "{" VarDeclList "}" ";"  */
#line 466 "src/xkbcomp/parser.y"
                        { (yyvsp[-4].interp)->def = (yyvsp[-2].varList).head; (yyval.interp) = (yyvsp[-4].interp); }
#line 2579 "src/xkbcomp/parser.c"
    break;

  case 59: /* InterpretMatch: KeySym "+" Expr  */
#line 470 "src/xkbcomp/parser.y"
                        { (yyval.interp) = InterpCreate((yyvsp[-2].keysym), (yyvsp[0].expr)); }
#line 2585 "src/xkbcomp/parser.c"
    break;

  case 60: /* InterpretMatch: KeySym  */
#line 472 "src/xkbcomp/parser.y"
                        { (yyval.interp) = InterpCreate((yyvsp[0].keysym), NULL); }
#line 2591 "src/xkbcomp/parser.c"
    break;

  case 61: /* VarDeclList: VarDeclList VarDecl  */
#line 476 "src/xkbcomp/parser.y"
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
#line 2607 "src/xkbcomp/parser.c"
    break;

  case 62: /* VarDeclList: %empty  */
#line 487 "src/xkbcomp/parser.y"
                        { (yyval.varList).head = (yyval.varList).last = NULL; }
#line 2613 "src/xkbcomp/parser.c"
    break;

  case 63: /* KeyTypeDecl: "type" String "{" VarDeclList "}" ";"  */
#line 493 "src/xkbcomp/parser.y"
                        { (yyval.keyType) = KeyTypeCreate((yyvsp[-4].atom), (yyvsp[-2].varList).head); }
#line 2619 "src/xkbcomp/parser.c"
    break;

  case 64: /* SymbolsDecl: "key" "key name" "{" OptSymbolsBody "}" ";"  */
#line 499 "src/xkbcomp/parser.y"
                        { (yyval.syms) = SymbolsCreate((yyvsp[-4].atom), (yyvsp[-2].varList).head); }
#line 2625 "src/xkbcomp/parser.c"
    break;

  case 65: /* OptSymbolsBody: SymbolsBody  */
#line 502 "src/xkbcomp/parser.y"
                                    { (yyval.varList) = (yyvsp[0].varList); }
#line 2631 "src/xkbcomp/parser.c"
    break;

  case 66: /* OptSymbolsBody: %empty  */
#line 503 "src/xkbcomp/parser.y"
                                    { (yyval.varList).head = (yyval.varList).last = NULL; }
#line 2637 "src/xkbcomp/parser.c"
    break;

  case 67: /* SymbolsBody: SymbolsBody "," SymbolsVarDecl  */
#line 507 "src/xkbcomp/parser.y"
                        { (yyval.varList).head = (yyvsp[-2].varList).head; (yyval.varList).last->common.next = &(yyvsp[0].var)->common; (yyval.varList).last = (yyvsp[0].var); }
#line 2643 "src/xkbcomp/parser.c"
    break;

  case 68: /* SymbolsBody: SymbolsVarDecl  */
#line 509 "src/xkbcomp/parser.y"
                        { (yyval.varList).head = (yyval.varList).last = (yyvsp[0].var); }
#line 2649 "src/xkbcomp/parser.c"
    break;

  case 69: /* SymbolsVarDecl: Lhs "=" Expr  */
#line 512 "src/xkbcomp/parser.y"
                                                { (yyval.var) = VarCreate((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2655 "src/xkbcomp/parser.c"
    break;

  case 70: /* SymbolsVarDecl: Lhs "=" MultiKeySymOrActionList  */
#line 513 "src/xkbcomp/parser.y"
                                                           { (yyval.var) = VarCreate((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2661 "src/xkbcomp/parser.c"
    break;

  case 71: /* SymbolsVarDecl: Ident  */
#line 514 "src/xkbcomp/parser.y"
                                                { (yyval.var) = BoolVarCreate((yyvsp[0].atom), true); }
#line 2667 "src/xkbcomp/parser.c"
    break;

  case 72: /* SymbolsVarDecl: "!" Ident  */
#line 515 "src/xkbcomp/parser.y"
                                                { (yyval.var) = BoolVarCreate((yyvsp[0].atom), false); }
#line 2673 "src/xkbcomp/parser.c"
    break;

  case 73: /* SymbolsVarDecl: MultiKeySymOrActionList  */
#line 516 "src/xkbcomp/parser.y"
                                                { (yyval.var) = VarCreate(NULL, (yyvsp[0].expr)); }
#line 2679 "src/xkbcomp/parser.c"
    break;

  case 74: /* MultiKeySymOrActionList: "[" MultiKeySymList "]"  */
#line 532 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[-1].exprList).head; }
#line 2685 "src/xkbcomp/parser.c"
    break;

  case 75: /* MultiKeySymOrActionList: "[" NoSymbolOrActionList "," MultiKeySymList "]"  */
#line 534 "src/xkbcomp/parser.y"
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
#line 2707 "src/xkbcomp/parser.c"
    break;

  case 76: /* MultiKeySymOrActionList: "[" MultiActionList "]"  */
#line 552 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[-1].exprList).head; }
#line 2713 "src/xkbcomp/parser.c"
    break;

  case 77: /* MultiKeySymOrActionList: "[" NoSymbolOrActionList "," MultiActionList "]"  */
#line 554 "src/xkbcomp/parser.y"
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
#line 2734 "src/xkbcomp/parser.c"
    break;

  case 78: /* MultiKeySymOrActionList: "[" NoSymbolOrActionList "]"  */
#line 576 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprEmptyList(); }
#line 2740 "src/xkbcomp/parser.c"
    break;

  case 79: /* NoSymbolOrActionList: NoSymbolOrActionList "," "{" "}"  */
#line 582 "src/xkbcomp/parser.y"
                        { (yyval.noSymbolOrActionList) = (yyvsp[-3].noSymbolOrActionList) + 1; }
#line 2746 "src/xkbcomp/parser.c"
    break;

  case 80: /* NoSymbolOrActionList: "{" "}"  */
#line 584 "src/xkbcomp/parser.y"
                        { (yyval.noSymbolOrActionList) = 1; }
#line 2752 "src/xkbcomp/parser.c"
    break;

  case 81: /* NoSymbolOrActionList: %empty  */
#line 585 "src/xkbcomp/parser.y"
                        { (yyval.noSymbolOrActionList) = 0; }
#line 2758 "src/xkbcomp/parser.c"
    break;

  case 82: /* GroupCompatDecl: "group" Integer "=" Expr ";"  */
#line 589 "src/xkbcomp/parser.y"
                        { (yyval.groupCompat) = GroupCompatCreate((yyvsp[-3].num), (yyvsp[-1].expr)); }
#line 2764 "src/xkbcomp/parser.c"
    break;

  case 83: /* ModMapDecl: "modifier_map" Ident "{" KeyOrKeySymList "}" ";"  */
#line 593 "src/xkbcomp/parser.y"
                        { (yyval.modMask) = ModMapCreate((yyvsp[-4].atom), (yyvsp[-2].exprList).head); }
#line 2770 "src/xkbcomp/parser.c"
    break;

  case 84: /* KeyOrKeySymList: KeyOrKeySymList "," KeyOrKeySym  */
#line 597 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyvsp[-2].exprList).head; (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common; (yyval.exprList).last = (yyvsp[0].expr); }
#line 2776 "src/xkbcomp/parser.c"
    break;

  case 85: /* KeyOrKeySymList: KeyOrKeySym  */
#line 599 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 2782 "src/xkbcomp/parser.c"
    break;

  case 86: /* KeyOrKeySym: "key name"  */
#line 603 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateKeyName((yyvsp[0].atom)); }
#line 2788 "src/xkbcomp/parser.c"
    break;

  case 87: /* KeyOrKeySym: KeySym  */
#line 605 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateKeySym((yyvsp[0].keysym)); }
#line 2794 "src/xkbcomp/parser.c"
    break;

  case 88: /* LedMapDecl: "indicator" String "{" VarDeclList "}" ";"  */
#line 609 "src/xkbcomp/parser.y"
                        { (yyval.ledMap) = LedMapCreate((yyvsp[-4].atom), (yyvsp[-2].varList).head); }
#line 2800 "src/xkbcomp/parser.c"
    break;

  case 89: /* LedNameDecl: "indicator" Integer "=" Expr ";"  */
#line 613 "src/xkbcomp/parser.y"
                        { (yyval.ledName) = LedNameCreate((yyvsp[-3].num), (yyvsp[-1].expr), false); }
#line 2806 "src/xkbcomp/parser.c"
    break;

  case 90: /* LedNameDecl: "virtual" "indicator" Integer "=" Expr ";"  */
#line 615 "src/xkbcomp/parser.y"
                        { (yyval.ledName) = LedNameCreate((yyvsp[-3].num), (yyvsp[-1].expr), true); }
#line 2812 "src/xkbcomp/parser.c"
    break;

  case 91: /* UnknownDecl: "identifier" Terminal "=" Expr ";"  */
#line 619 "src/xkbcomp/parser.y"
                        {
                            FreeStmt((ParseCommon *) (yyvsp[-3].expr));
                            FreeStmt((ParseCommon *) (yyvsp[-1].expr));
                            (yyval.unknown) = UnknownStatementCreate(STMT_UNKNOWN_DECLARATION, (yyvsp[-4].sval));
                        }
#line 2822 "src/xkbcomp/parser.c"
    break;

  case 92: /* UnknownCompoundStatementDecl: "identifier" OptTerminal "{" VarDeclList "}" ";"  */
#line 628 "src/xkbcomp/parser.y"
                        {
                            FreeStmt((ParseCommon *) (yyvsp[-4].expr));
                            FreeStmt((ParseCommon *) (yyvsp[-2].varList).head);
                            (yyval.unknown) = UnknownStatementCreate(STMT_UNKNOWN_COMPOUND, (yyvsp[-5].sval));
                        }
#line 2832 "src/xkbcomp/parser.c"
    break;

  case 93: /* ShapeDecl: "shape" String "{" OutlineList "}" ";"  */
#line 636 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2838 "src/xkbcomp/parser.c"
    break;

  case 94: /* ShapeDecl: "shape" String "{" CoordList "}" ";"  */
#line 638 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[-2].expr); (yyval.geom) = NULL; }
#line 2844 "src/xkbcomp/parser.c"
    break;

  case 95: /* SectionDecl: "section" String "{" SectionBody "}" ";"  */
#line 642 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2850 "src/xkbcomp/parser.c"
    break;

  case 96: /* SectionBody: SectionBody SectionBodyItem  */
#line 645 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL;}
#line 2856 "src/xkbcomp/parser.c"
    break;

  case 97: /* SectionBody: SectionBodyItem  */
#line 646 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL; }
#line 2862 "src/xkbcomp/parser.c"
    break;

  case 98: /* SectionBodyItem: "row" "{" RowBody "}" ";"  */
#line 650 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2868 "src/xkbcomp/parser.c"
    break;

  case 99: /* SectionBodyItem: VarDecl  */
#line 652 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[0].var)); (yyval.geom) = NULL; }
#line 2874 "src/xkbcomp/parser.c"
    break;

  case 100: /* SectionBodyItem: DoodadDecl  */
#line 654 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2880 "src/xkbcomp/parser.c"
    break;

  case 101: /* SectionBodyItem: LedMapDecl  */
#line 656 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[0].ledMap)); (yyval.geom) = NULL; }
#line 2886 "src/xkbcomp/parser.c"
    break;

  case 102: /* SectionBodyItem: OverlayDecl  */
#line 658 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2892 "src/xkbcomp/parser.c"
    break;

  case 103: /* RowBody: RowBody RowBodyItem  */
#line 661 "src/xkbcomp/parser.y"
                                                { (yyval.geom) = NULL;}
#line 2898 "src/xkbcomp/parser.c"
    break;

  case 104: /* RowBody: RowBodyItem  */
#line 662 "src/xkbcomp/parser.y"
                                                { (yyval.geom) = NULL; }
#line 2904 "src/xkbcomp/parser.c"
    break;

  case 105: /* RowBodyItem: "keys" "{" Keys "}" ";"  */
#line 665 "src/xkbcomp/parser.y"
                                                     { (yyval.geom) = NULL; }
#line 2910 "src/xkbcomp/parser.c"
    break;

  case 106: /* RowBodyItem: VarDecl  */
#line 667 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[0].var)); (yyval.geom) = NULL; }
#line 2916 "src/xkbcomp/parser.c"
    break;

  case 107: /* Keys: Keys "," Key  */
#line 670 "src/xkbcomp/parser.y"
                                                { (yyval.geom) = NULL; }
#line 2922 "src/xkbcomp/parser.c"
    break;

  case 108: /* Keys: Key  */
#line 671 "src/xkbcomp/parser.y"
                                                { (yyval.geom) = NULL; }
#line 2928 "src/xkbcomp/parser.c"
    break;

  case 109: /* Key: "key name"  */
#line 675 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2934 "src/xkbcomp/parser.c"
    break;

  case 110: /* Key: "{" ExprList "}"  */
#line 677 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[-1].exprList).head); (yyval.geom) = NULL; }
#line 2940 "src/xkbcomp/parser.c"
    break;

  case 111: /* OverlayDecl: "overlay" String "{" OverlayKeyList "}" ";"  */
#line 681 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2946 "src/xkbcomp/parser.c"
    break;

  case 112: /* OverlayKeyList: OverlayKeyList "," OverlayKey  */
#line 684 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL; }
#line 2952 "src/xkbcomp/parser.c"
    break;

  case 113: /* OverlayKeyList: OverlayKey  */
#line 685 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL; }
#line 2958 "src/xkbcomp/parser.c"
    break;

  case 114: /* OverlayKey: "key name" "=" "key name"  */
#line 688 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL; }
#line 2964 "src/xkbcomp/parser.c"
    break;

  case 115: /* OutlineList: OutlineList "," OutlineInList  */
#line 692 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL;}
#line 2970 "src/xkbcomp/parser.c"
    break;

  case 116: /* OutlineList: OutlineInList  */
#line 694 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2976 "src/xkbcomp/parser.c"
    break;

  case 117: /* OutlineInList: "{" CoordList "}"  */
#line 698 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[-1].expr); (yyval.geom) = NULL; }
#line 2982 "src/xkbcomp/parser.c"
    break;

  case 118: /* OutlineInList: Ident "=" "{" CoordList "}"  */
#line 700 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[-1].expr); (yyval.geom) = NULL; }
#line 2988 "src/xkbcomp/parser.c"
    break;

  case 119: /* OutlineInList: Ident "=" Expr  */
#line 702 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[0].expr)); (yyval.geom) = NULL; }
#line 2994 "src/xkbcomp/parser.c"
    break;

  case 120: /* CoordList: CoordList "," Coord  */
#line 706 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[-2].expr); (void) (yyvsp[0].expr); (yyval.expr) = NULL; }
#line 3000 "src/xkbcomp/parser.c"
    break;

  case 121: /* CoordList: Coord  */
#line 708 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[0].expr); (yyval.expr) = NULL; }
#line 3006 "src/xkbcomp/parser.c"
    break;

  case 122: /* Coord: "[" SignedNumber "," SignedNumber "]"  */
#line 712 "src/xkbcomp/parser.y"
                        { (yyval.expr) = NULL; }
#line 3012 "src/xkbcomp/parser.c"
    break;

  case 123: /* DoodadDecl: DoodadType String "{" VarDeclList "}" ";"  */
#line 716 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[-2].varList).head); (yyval.geom) = NULL; }
#line 3018 "src/xkbcomp/parser.c"
    break;

  case 124: /* DoodadType: "text"  */
#line 719 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3024 "src/xkbcomp/parser.c"
    break;

  case 125: /* DoodadType: "outline"  */
#line 720 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3030 "src/xkbcomp/parser.c"
    break;

  case 126: /* DoodadType: "solid"  */
#line 721 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3036 "src/xkbcomp/parser.c"
    break;

  case 127: /* DoodadType: "logo"  */
#line 722 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3042 "src/xkbcomp/parser.c"
    break;

  case 128: /* FieldSpec: Ident  */
#line 725 "src/xkbcomp/parser.y"
                                { (yyval.atom) = (yyvsp[0].atom); }
#line 3048 "src/xkbcomp/parser.c"
    break;

  case 129: /* FieldSpec: Element  */
#line 726 "src/xkbcomp/parser.y"
                                { (yyval.atom) = (yyvsp[0].atom); }
#line 3054 "src/xkbcomp/parser.c"
    break;

  case 130: /* Element: "action"  */
#line 730 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "action"); }
#line 3060 "src/xkbcomp/parser.c"
    break;

  case 131: /* Element: "interpret"  */
#line 732 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "interpret"); }
#line 3066 "src/xkbcomp/parser.c"
    break;

  case 132: /* Element: "type"  */
#line 734 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "type"); }
#line 3072 "src/xkbcomp/parser.c"
    break;

  case 133: /* Element: "key"  */
#line 736 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "key"); }
#line 3078 "src/xkbcomp/parser.c"
    break;

  case 134: /* Element: "group"  */
#line 738 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "group"); }
#line 3084 "src/xkbcomp/parser.c"
    break;

  case 135: /* Element: "modifier_map"  */
#line 740 "src/xkbcomp/parser.y"
                        {(yyval.atom) = xkb_atom_intern_literal(param->ctx, "modifier_map");}
#line 3090 "src/xkbcomp/parser.c"
    break;

  case 136: /* Element: "indicator"  */
#line 742 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "indicator"); }
#line 3096 "src/xkbcomp/parser.c"
    break;

  case 137: /* Element: "shape"  */
#line 744 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "shape"); }
#line 3102 "src/xkbcomp/parser.c"
    break;

  case 138: /* Element: "row"  */
#line 746 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "row"); }
#line 3108 "src/xkbcomp/parser.c"
    break;

  case 139: /* Element: "section"  */
#line 748 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "section"); }
#line 3114 "src/xkbcomp/parser.c"
    break;

  case 140: /* Element: "text"  */
#line 750 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "text"); }
#line 3120 "src/xkbcomp/parser.c"
    break;

  case 141: /* OptMergeMode: MergeMode  */
#line 753 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = (yyvsp[0].merge); }
#line 3126 "src/xkbcomp/parser.c"
    break;

  case 142: /* OptMergeMode: %empty  */
#line 754 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_DEFAULT; }
#line 3132 "src/xkbcomp/parser.c"
    break;

  case 143: /* MergeMode: "include"  */
#line 757 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_DEFAULT; }
#line 3138 "src/xkbcomp/parser.c"
    break;

  case 144: /* MergeMode: "augment"  */
#line 758 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_AUGMENT; }
#line 3144 "src/xkbcomp/parser.c"
    break;

  case 145: /* MergeMode: "override"  */
#line 759 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_OVERRIDE; }
#line 3150 "src/xkbcomp/parser.c"
    break;

  case 146: /* MergeMode: "replace"  */
#line 760 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_REPLACE; }
#line 3156 "src/xkbcomp/parser.c"
    break;

  case 147: /* MergeMode: "alternate"  */
#line 762 "src/xkbcomp/parser.y"
                {
                    /*
                     * This used to be MERGE_ALT_FORM. This functionality was
                     * unused and has been removed.
                     */
                    parser_warn(param, XKB_LOG_MESSAGE_NO_ID,
                                "ignored unsupported legacy merge mode \"alternate\"");
                    (yyval.merge) = MERGE_DEFAULT;
                }
#line 3170 "src/xkbcomp/parser.c"
    break;

  case 148: /* ExprList: ExprList "," Expr  */
#line 774 "src/xkbcomp/parser.y"
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
#line 3186 "src/xkbcomp/parser.c"
    break;

  case 149: /* ExprList: Expr  */
#line 786 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 3192 "src/xkbcomp/parser.c"
    break;

  case 150: /* ExprList: %empty  */
#line 787 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = NULL; }
#line 3198 "src/xkbcomp/parser.c"
    break;

  case 151: /* Expr: Expr "/" Expr  */
#line 791 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_DIVIDE, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3204 "src/xkbcomp/parser.c"
    break;

  case 152: /* Expr: Expr "+" Expr  */
#line 793 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3210 "src/xkbcomp/parser.c"
    break;

  case 153: /* Expr: Expr "-" Expr  */
#line 795 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_SUBTRACT, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3216 "src/xkbcomp/parser.c"
    break;

  case 154: /* Expr: Expr "*" Expr  */
#line 797 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_MULTIPLY, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3222 "src/xkbcomp/parser.c"
    break;

  case 155: /* Expr: Lhs "=" Expr  */
#line 799 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3228 "src/xkbcomp/parser.c"
    break;

  case 156: /* Expr: Term  */
#line 801 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3234 "src/xkbcomp/parser.c"
    break;

  case 157: /* Term: "-" Term  */
#line 805 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateUnary(STMT_EXPR_NEGATE, (yyvsp[0].expr)); }
#line 3240 "src/xkbcomp/parser.c"
    break;

  case 158: /* Term: "+" Term  */
#line 807 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateUnary(STMT_EXPR_UNARY_PLUS, (yyvsp[0].expr)); }
#line 3246 "src/xkbcomp/parser.c"
    break;

  case 159: /* Term: "!" Term  */
#line 809 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateUnary(STMT_EXPR_NOT, (yyvsp[0].expr)); }
#line 3252 "src/xkbcomp/parser.c"
    break;

  case 160: /* Term: "~" Term  */
#line 811 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateUnary(STMT_EXPR_INVERT, (yyvsp[0].expr)); }
#line 3258 "src/xkbcomp/parser.c"
    break;

  case 161: /* Term: Lhs  */
#line 813 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3264 "src/xkbcomp/parser.c"
    break;

  case 162: /* Term: FieldSpec "(" ExprList ")"  */
#line 815 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateAction((yyvsp[-3].atom), (yyvsp[-1].exprList).head); }
#line 3270 "src/xkbcomp/parser.c"
    break;

  case 163: /* Term: Actions  */
#line 817 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3276 "src/xkbcomp/parser.c"
    break;

  case 164: /* Term: Terminal  */
#line 819 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3282 "src/xkbcomp/parser.c"
    break;

  case 165: /* Term: "(" Expr ")"  */
#line 821 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[-1].expr); }
#line 3288 "src/xkbcomp/parser.c"
    break;

  case 166: /* MultiActionList: MultiActionList "," Action  */
#line 825 "src/xkbcomp/parser.y"
                        {
                            ExprDef *expr = ExprCreateActionList((yyvsp[0].expr));
                            (yyval.exprList) = (yyvsp[-2].exprList);
                            (yyval.exprList).last->common.next = &expr->common; (yyval.exprList).last = expr;
                        }
#line 3298 "src/xkbcomp/parser.c"
    break;

  case 167: /* MultiActionList: MultiActionList "," Actions  */
#line 831 "src/xkbcomp/parser.y"
                        { (yyval.exprList) = (yyvsp[-2].exprList); (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common; (yyval.exprList).last = (yyvsp[0].expr); }
#line 3304 "src/xkbcomp/parser.c"
    break;

  case 168: /* MultiActionList: Action  */
#line 833 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = ExprCreateActionList((yyvsp[0].expr)); }
#line 3310 "src/xkbcomp/parser.c"
    break;

  case 169: /* MultiActionList: NonEmptyActions  */
#line 835 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 3316 "src/xkbcomp/parser.c"
    break;

  case 170: /* ActionList: ActionList "," Action  */
#line 839 "src/xkbcomp/parser.y"
                        { (yyval.exprList) = (yyvsp[-2].exprList); (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common; (yyval.exprList).last = (yyvsp[0].expr); }
#line 3322 "src/xkbcomp/parser.c"
    break;

  case 171: /* ActionList: Action  */
#line 841 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 3328 "src/xkbcomp/parser.c"
    break;

  case 172: /* NonEmptyActions: "{" ActionList "}"  */
#line 845 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateActionList((yyvsp[-1].exprList).head); }
#line 3334 "src/xkbcomp/parser.c"
    break;

  case 173: /* Actions: NonEmptyActions  */
#line 849 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3340 "src/xkbcomp/parser.c"
    break;

  case 174: /* Actions: "{" "}"  */
#line 851 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateActionList(NULL); }
#line 3346 "src/xkbcomp/parser.c"
    break;

  case 175: /* Action: FieldSpec "(" ExprList ")"  */
#line 855 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateAction((yyvsp[-3].atom), (yyvsp[-1].exprList).head); }
#line 3352 "src/xkbcomp/parser.c"
    break;

  case 176: /* Lhs: FieldSpec  */
#line 859 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateIdent((yyvsp[0].atom)); }
#line 3358 "src/xkbcomp/parser.c"
    break;

  case 177: /* Lhs: FieldSpec "." FieldSpec  */
#line 861 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateFieldRef((yyvsp[-2].atom), (yyvsp[0].atom)); }
#line 3364 "src/xkbcomp/parser.c"
    break;

  case 178: /* Lhs: FieldSpec "[" Expr "]"  */
#line 863 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateArrayRef(XKB_ATOM_NONE, (yyvsp[-3].atom), (yyvsp[-1].expr)); }
#line 3370 "src/xkbcomp/parser.c"
    break;

  case 179: /* Lhs: FieldSpec "." FieldSpec "[" Expr "]"  */
#line 865 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateArrayRef((yyvsp[-5].atom), (yyvsp[-3].atom), (yyvsp[-1].expr)); }
#line 3376 "src/xkbcomp/parser.c"
    break;

  case 180: /* OptTerminal: Terminal  */
#line 869 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3382 "src/xkbcomp/parser.c"
    break;

  case 181: /* OptTerminal: %empty  */
#line 870 "src/xkbcomp/parser.y"
                        { (yyval.expr) = NULL; }
#line 3388 "src/xkbcomp/parser.c"
    break;

  case 182: /* Terminal: String  */
#line 874 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateString((yyvsp[0].atom)); }
#line 3394 "src/xkbcomp/parser.c"
    break;

  case 183: /* Terminal: Integer  */
#line 876 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateInteger((yyvsp[0].num)); }
#line 3400 "src/xkbcomp/parser.c"
    break;

  case 184: /* Terminal: Float  */
#line 878 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateFloat(/* Discard $1 */); }
#line 3406 "src/xkbcomp/parser.c"
    break;

  case 185: /* Terminal: "key name"  */
#line 880 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateKeyName((yyvsp[0].atom)); }
#line 3412 "src/xkbcomp/parser.c"
    break;

  case 186: /* MultiKeySymList: MultiKeySymList "," KeySymLit  */
#line 884 "src/xkbcomp/parser.y"
                        {
                            ExprDef *expr = ExprCreateKeySymList((yyvsp[0].keysym));
                            (yyval.exprList) = (yyvsp[-2].exprList);
                            (yyval.exprList).last->common.next = &expr->common; (yyval.exprList).last = expr;
                        }
#line 3422 "src/xkbcomp/parser.c"
    break;

  case 187: /* MultiKeySymList: MultiKeySymList "," KeySyms  */
#line 890 "src/xkbcomp/parser.y"
                        { (yyval.exprList) = (yyvsp[-2].exprList); (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common; (yyval.exprList).last = (yyvsp[0].expr); }
#line 3428 "src/xkbcomp/parser.c"
    break;

  case 188: /* MultiKeySymList: KeySymLit  */
#line 892 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = ExprCreateKeySymList((yyvsp[0].keysym)); }
#line 3434 "src/xkbcomp/parser.c"
    break;

  case 189: /* MultiKeySymList: NonEmptyKeySyms  */
#line 894 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 3440 "src/xkbcomp/parser.c"
    break;

  case 190: /* KeySymList: KeySymList "," KeySymLit  */
#line 898 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprAppendKeySymList((yyvsp[-2].expr), (yyvsp[0].keysym)); }
#line 3446 "src/xkbcomp/parser.c"
    break;

  case 191: /* KeySymList: KeySymList "," "string literal"  */
#line 900 "src/xkbcomp/parser.y"
                        {
                            (yyval.expr) = ExprKeySymListAppendString(param->scanner, (yyvsp[-2].expr), (yyvsp[0].str));
                            free((yyvsp[0].str));
                            if (!(yyval.expr))
                                YYERROR;
                        }
#line 3457 "src/xkbcomp/parser.c"
    break;

  case 192: /* KeySymList: KeySymLit  */
#line 907 "src/xkbcomp/parser.y"
                        {
                            (yyval.expr) = ExprCreateKeySymList((yyvsp[0].keysym));
                            if (!(yyval.expr))
                                YYERROR;
                        }
#line 3467 "src/xkbcomp/parser.c"
    break;

  case 193: /* KeySymList: "string literal"  */
#line 913 "src/xkbcomp/parser.y"
                        {
                            (yyval.expr) = ExprCreateKeySymList(XKB_KEY_NoSymbol);
                            if (!(yyval.expr))
                                YYERROR;
                            (yyval.expr) = ExprKeySymListAppendString(param->scanner, (yyval.expr), (yyvsp[0].str));
                            free((yyvsp[0].str));
                            if (!(yyval.expr))
                                YYERROR;
                        }
#line 3481 "src/xkbcomp/parser.c"
    break;

  case 194: /* NonEmptyKeySyms: "{" KeySymList "}"  */
#line 925 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[-1].expr); }
#line 3487 "src/xkbcomp/parser.c"
    break;

  case 195: /* NonEmptyKeySyms: "string literal"  */
#line 927 "src/xkbcomp/parser.y"
                        {
                            (yyval.expr) = ExprCreateKeySymList(XKB_KEY_NoSymbol);
                            if (!(yyval.expr))
                                YYERROR;
                            (yyval.expr) = ExprKeySymListAppendString(param->scanner, (yyval.expr), (yyvsp[0].str));
                            free((yyvsp[0].str));
                            if (!(yyval.expr))
                                YYERROR;
                        }
#line 3501 "src/xkbcomp/parser.c"
    break;

  case 196: /* KeySyms: NonEmptyKeySyms  */
#line 939 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3507 "src/xkbcomp/parser.c"
    break;

  case 197: /* KeySyms: "{" "}"  */
#line 941 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateKeySymList(XKB_KEY_NoSymbol); }
#line 3513 "src/xkbcomp/parser.c"
    break;

  case 198: /* KeySym: KeySymLit  */
#line 945 "src/xkbcomp/parser.y"
                        { (yyval.keysym) = (yyvsp[0].keysym); }
#line 3519 "src/xkbcomp/parser.c"
    break;

  case 199: /* KeySym: "string literal"  */
#line 947 "src/xkbcomp/parser.y"
                        {
                            (yyval.keysym) = KeysymParseString(param->scanner, (yyvsp[0].str));
                            free((yyvsp[0].str));
                            if ((yyval.keysym) == XKB_KEY_NoSymbol)
                                YYERROR;
                        }
#line 3530 "src/xkbcomp/parser.c"
    break;

  case 200: /* KeySymLit: "identifier"  */
#line 956 "src/xkbcomp/parser.y"
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
#line 3546 "src/xkbcomp/parser.c"
    break;

  case 201: /* KeySymLit: "section"  */
#line 968 "src/xkbcomp/parser.y"
                                { (yyval.keysym) = XKB_KEY_section; }
#line 3552 "src/xkbcomp/parser.c"
    break;

  case 202: /* KeySymLit: "decimal digit"  */
#line 970 "src/xkbcomp/parser.y"
                        {
                            /*
                             * Special case for digits 0..9:
                             * map to XKB_KEY_0 .. XKB_KEY_9, consistent with
                             * other keysym names: <name> → XKB_KEY_<name>.
                             */
                            (yyval.keysym) = XKB_KEY_0 + (xkb_keysym_t) (yyvsp[0].num);
                        }
#line 3565 "src/xkbcomp/parser.c"
    break;

  case 203: /* KeySymLit: "integer literal"  */
#line 979 "src/xkbcomp/parser.y"
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
#line 3627 "src/xkbcomp/parser.c"
    break;

  case 204: /* SignedNumber: "-" Number  */
#line 1038 "src/xkbcomp/parser.y"
                                        { (yyval.num) = -(yyvsp[0].num); }
#line 3633 "src/xkbcomp/parser.c"
    break;

  case 205: /* SignedNumber: Number  */
#line 1039 "src/xkbcomp/parser.y"
                                        { (yyval.num) = (yyvsp[0].num); }
#line 3639 "src/xkbcomp/parser.c"
    break;

  case 206: /* Number: "float literal"  */
#line 1042 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3645 "src/xkbcomp/parser.c"
    break;

  case 207: /* Number: "decimal digit"  */
#line 1043 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3651 "src/xkbcomp/parser.c"
    break;

  case 208: /* Number: "integer literal"  */
#line 1044 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3657 "src/xkbcomp/parser.c"
    break;

  case 209: /* Float: "float literal"  */
#line 1047 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3663 "src/xkbcomp/parser.c"
    break;

  case 210: /* Integer: "integer literal"  */
#line 1050 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3669 "src/xkbcomp/parser.c"
    break;

  case 211: /* Integer: "decimal digit"  */
#line 1051 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3675 "src/xkbcomp/parser.c"
    break;

  case 212: /* KeyCode: "integer literal"  */
#line 1054 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3681 "src/xkbcomp/parser.c"
    break;

  case 213: /* KeyCode: "decimal digit"  */
#line 1055 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3687 "src/xkbcomp/parser.c"
    break;

  case 214: /* Ident: "identifier"  */
#line 1058 "src/xkbcomp/parser.y"
                                { (yyval.atom) = xkb_atom_intern(param->ctx, (yyvsp[0].sval).start, (yyvsp[0].sval).len); }
#line 3693 "src/xkbcomp/parser.c"
    break;

  case 215: /* Ident: "default"  */
#line 1059 "src/xkbcomp/parser.y"
                                { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "default"); }
#line 3699 "src/xkbcomp/parser.c"
    break;

  case 216: /* String: "string literal"  */
#line 1062 "src/xkbcomp/parser.y"
                                { (yyval.atom) = xkb_atom_intern(param->ctx, (yyvsp[0].str), strlen((yyvsp[0].str))); free((yyvsp[0].str)); }
#line 3705 "src/xkbcomp/parser.c"
    break;

  case 217: /* OptMapName: MapName  */
#line 1065 "src/xkbcomp/parser.y"
                                { (yyval.str) = (yyvsp[0].str); }
#line 3711 "src/xkbcomp/parser.c"
    break;

  case 218: /* OptMapName: %empty  */
#line 1066 "src/xkbcomp/parser.y"
                                { (yyval.str) = NULL; }
#line 3717 "src/xkbcomp/parser.c"
    break;

  case 219: /* MapName: "string literal"  */
#line 1069 "src/xkbcomp/parser.y"
                                { (yyval.str) = (yyvsp[0].str); }
#line 3723 "src/xkbcomp/parser.c"
    break;


#line 3727 "src/xkbcomp/parser.c"

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

#line 1072 "src/xkbcomp/parser.y"


/* Parse a specific section */
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
        log_vrb(ctx, XKB_LOG_VERBOSITY_DETAILED,
                XKB_WARNING_MISSING_DEFAULT_SECTION,
                "No map in include statement, but \"%s\" contains several; "
                "Using first defined map, \"%s\"\n",
                scanner->file_name, safe_map_name(first));

    return first;
}

/* Parse the next section */
bool
parse_next(struct xkb_context *ctx, struct scanner *scanner, XkbFile **xkb_file)
{
    int ret;
    struct parser_param param = {
        .scanner = scanner,
        .ctx = ctx,
        .rtrn = NULL,
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
