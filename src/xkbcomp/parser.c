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

#line 149 "src/xkbcomp/parser.c"

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
#define YYFINAL  17
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   928

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  66
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  83
/* YYNRULES -- Number of rules.  */
#define YYNRULES  220
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  385

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
       0,   273,   273,   275,   277,   281,   287,   288,   289,   295,
     307,   310,   318,   319,   320,   321,   322,   325,   326,   329,
     330,   333,   334,   335,   336,   337,   338,   339,   340,   341,
     358,   373,   383,   386,   392,   397,   402,   407,   412,   417,
     422,   427,   432,   437,   438,   439,   440,   442,   444,   451,
     453,   455,   459,   463,   467,   471,   473,   477,   479,   483,
     489,   491,   495,   507,   510,   516,   522,   523,   526,   528,
     532,   533,   534,   535,   536,   551,   553,   571,   573,   595,
     601,   603,   605,   608,   612,   616,   618,   622,   624,   628,
     632,   634,   638,   647,   655,   657,   661,   665,   666,   669,
     671,   673,   675,   677,   681,   682,   685,   686,   690,   691,
     694,   696,   700,   704,   705,   708,   711,   713,   717,   719,
     721,   725,   727,   731,   735,   739,   740,   741,   742,   745,
     746,   749,   751,   753,   755,   757,   759,   761,   763,   765,
     767,   769,   773,   774,   777,   778,   779,   780,   781,   793,
     805,   807,   810,   812,   814,   816,   818,   820,   824,   826,
     828,   830,   832,   834,   836,   838,   840,   844,   850,   852,
     854,   858,   860,   864,   868,   870,   874,   878,   880,   882,
     884,   888,   890,   893,   895,   897,   899,   903,   909,   911,
     913,   917,   919,   926,   932,   944,   946,   958,   960,   964,
     966,   975,   988,   989,   998,  1058,  1059,  1062,  1063,  1064,
    1067,  1070,  1071,  1074,  1075,  1078,  1079,  1082,  1085,  1086,
    1089
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

#define YYPACT_NINF (-281)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-216)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
       7,  -281,  -281,  -281,  -281,  -281,  -281,  -281,  -281,  -281,
    -281,     6,  -281,  -281,   517,   524,  -281,  -281,  -281,  -281,
    -281,  -281,  -281,  -281,  -281,  -281,   -42,   -42,  -281,  -281,
     -23,  -281,    36,  -281,  -281,   849,    10,    74,  -281,   458,
    -281,  -281,  -281,  -281,  -281,    81,  -281,    25,    34,  -281,
    -281,   -27,    59,   172,  -281,    40,    82,   102,   -27,   -13,
      59,  -281,    59,   120,  -281,  -281,  -281,   144,   -27,   324,
     163,  -281,  -281,  -281,  -281,  -281,  -281,  -281,  -281,  -281,
    -281,  -281,  -281,  -281,  -281,  -281,  -281,  -281,    59,    73,
    -281,   175,   171,  -281,  -281,   147,  -281,   187,  -281,   186,
    -281,  -281,  -281,  -281,  -281,   197,   206,  -281,   222,   213,
    -281,  -281,   239,   237,   251,   247,   249,   252,   102,   243,
    -281,  -281,   258,   271,  -281,  -281,  -281,   154,   261,   332,
     869,   332,  -281,   -27,  -281,   332,  -281,  -281,   332,   597,
     260,   332,    60,   332,  -281,    35,   461,   293,  -281,  -281,
     332,  -281,  -281,   277,  -281,  -281,  -281,  -281,  -281,  -281,
    -281,  -281,  -281,  -281,   332,   332,   825,   332,   332,   332,
     -28,   228,  -281,  -281,  -281,   294,  -281,  -281,   287,   103,
    -281,   433,   639,   654,   433,   478,   -27,   295,   288,  -281,
    -281,   303,   -15,   299,   233,  -281,    13,  -281,  -281,   285,
     696,   309,    96,    37,  -281,    45,  -281,   323,    59,   321,
      59,  -281,  -281,   419,  -281,  -281,  -281,   332,   711,   372,
    -281,   753,  -281,  -281,  -281,  -281,   331,    48,  -281,   418,
    -281,  -281,   332,   332,   332,   332,   332,  -281,   332,   332,
    -281,   315,  -281,   317,   337,   520,  -281,   347,   133,   146,
    -281,  -281,   158,  -281,  -281,  -281,   343,   597,   290,  -281,
    -281,   344,    60,  -281,   345,    56,   134,  -281,  -281,  -281,
     356,  -281,   358,   -25,   364,   309,   377,   773,   375,   368,
    -281,   386,   370,  -281,   373,   332,  -281,   869,  -281,    30,
     433,   245,   245,  -281,  -281,   433,   266,  -281,  -281,  -281,
    -281,    67,  -281,  -281,   540,  -281,   845,  -281,   161,  -281,
    -281,  -281,   433,  -281,  -281,  -281,  -281,  -281,    96,  -281,
    -281,  -281,  -281,   796,   433,   400,  -281,   227,  -281,   389,
    -281,  -281,  -281,  -281,    58,  -281,  -281,   332,  -281,  -281,
     208,   582,   170,   196,  -281,  -281,   180,  -281,  -281,  -281,
     408,    89,   -20,   410,  -281,   424,   112,  -281,  -281,   433,
    -281,  -281,  -281,  -281,  -281,  -281,  -281,  -281,   332,  -281,
     117,  -281,  -281,   411,   425,   389,   128,   427,   -20,  -281,
    -281,  -281,  -281,  -281,  -281
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
      18,     4,    29,    21,    22,    23,    24,    25,    26,    27,
      28,     0,     2,     3,     0,    17,    20,     1,     6,    12,
      13,    15,    14,    16,     7,     8,   219,   219,    19,   220,
       0,   218,     0,    10,    32,    18,   143,     0,     9,     0,
     144,   146,   145,   147,   148,     0,    30,     0,   142,     5,
      11,     0,   133,   132,   131,   134,     0,   135,   136,   137,
     138,   139,   140,   141,   126,   127,   128,     0,     0,   215,
       0,   216,    33,    35,    36,    31,    34,    37,    38,    40,
      39,    41,    42,    46,    47,    43,    44,    45,     0,   177,
     130,     0,   129,    48,   215,     0,    56,    57,   217,     0,
     202,   200,   203,   204,   201,     0,    61,   199,     0,     0,
     212,   211,     0,     0,     0,     0,     0,     0,     0,     0,
     210,   186,     0,   181,   185,   184,   183,     0,     0,     0,
       0,     0,    50,     0,    54,     0,    63,    63,     0,    67,
       0,     0,     0,     0,    63,     0,     0,     0,    51,    63,
       0,   214,   213,     0,    63,   133,   132,   134,   135,   136,
     137,   138,   140,   141,     0,     0,     0,     0,     0,     0,
     177,     0,   157,   174,   164,   162,   165,   129,   178,     0,
      55,    58,     0,     0,    60,    82,     0,     0,    66,    69,
      74,     0,   129,     0,     0,    87,     0,    86,    88,     0,
       0,     0,     0,     0,   117,     0,   122,     0,   137,   139,
       0,   100,   102,     0,    98,   103,   101,     0,     0,     0,
      52,     0,   159,   162,   158,   175,     0,     0,   172,     0,
     160,   161,   151,     0,     0,     0,     0,   179,     0,     0,
      49,     0,    62,     0,   202,     0,   196,   201,     0,     0,
     170,   169,     0,   190,   189,    73,     0,     0,     0,    53,
      83,     0,     0,    90,     0,     0,     0,   208,   209,   207,
       0,   206,     0,     0,     0,     0,     0,     0,     0,     0,
      97,     0,     0,    92,     0,   151,   173,     0,   166,     0,
     150,   153,   154,   152,   155,   156,     0,    64,    59,    81,
     194,     0,   193,    79,     0,    77,     0,    75,     0,    65,
      68,    71,    70,    84,    85,    89,   118,   205,     0,    94,
     116,    95,   121,     0,   120,     0,   107,     0,   105,     0,
      96,    91,    93,   124,     0,   171,   163,     0,   180,   195,
       0,     0,     0,     0,   168,   167,     0,   197,   188,   187,
       0,     0,     0,     0,   104,     0,     0,   114,   176,   149,
     192,   191,    80,    78,    76,   198,   123,   119,   151,   110,
       0,   109,    99,     0,     0,     0,     0,     0,     0,   115,
     112,   113,   111,   106,   108
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -281,  -281,  -281,  -281,  -281,   442,  -281,   448,  -281,   473,
    -281,  -281,   -46,  -281,  -281,  -281,  -281,   367,  -281,  -281,
      24,  -281,  -281,  -281,  -281,   248,   250,  -281,  -281,  -281,
    -281,   253,   463,  -281,  -281,  -281,  -281,  -281,  -281,   300,
    -281,   185,  -281,   136,  -281,  -281,   141,  -281,   256,  -197,
     267,   471,  -281,   -47,  -281,  -281,  -281,  -280,    62,   203,
     229,  -281,  -177,   230,  -182,   -36,  -281,   466,   246,  -281,
     244,  -281,   494,  -183,   235,   289,  -281,   -45,  -281,   -38,
     -24,   527,  -281
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,    11,    12,    26,    35,    13,    27,    14,    15,    16,
      36,    46,   242,    73,    74,    75,    95,    96,    76,   105,
     182,    77,    78,   187,   188,   189,   190,   248,    79,    80,
     196,   197,   212,    82,    83,    84,    85,    86,   213,   214,
     327,   328,   370,   371,   215,   356,   357,   203,   204,   205,
     206,   216,    88,   170,    90,    47,    48,   289,   290,   172,
     249,   227,   173,   174,   228,   175,   122,   176,   252,   301,
     253,   348,   198,   107,   270,   271,   124,   125,   153,   177,
     126,    30,    31
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      89,    72,   254,   251,   265,   334,    17,     1,   250,    92,
      29,    91,   112,    97,   114,   232,   201,   129,    33,   130,
     113,   368,    40,    41,    42,    43,    44,   -72,    99,    94,
     119,    94,    71,   -72,    71,   115,   116,   369,   117,    98,
     110,   111,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    45,    61,    62,   261,    63,    64,    65,    66,
      67,   262,   302,     2,   128,     3,     4,     5,     6,     7,
       8,     9,    10,   147,   336,    68,   201,    34,   337,   272,
     202,    69,    70,   178,    71,   273,    93,   274,   376,   100,
     286,    94,    89,   275,    71,    97,   287,   108,   316,    89,
     211,   192,   358,   191,   275,   335,   337,   207,    92,   339,
      91,    98,   101,   102,   103,   340,   104,   195,   129,   226,
     130,   254,   251,    49,   345,   349,   351,   250,   223,   223,
      50,   367,   223,   223,   266,    89,    89,   275,   226,   109,
     233,   234,   235,   236,    92,    92,    91,    91,   255,   267,
     268,   269,   240,    89,   374,   110,   111,   361,   302,   377,
     375,   183,    92,   302,    91,   378,    89,   211,   200,   118,
     382,    89,  -125,   218,    89,    92,   337,    91,   221,   303,
      92,   304,    91,    92,   115,    91,   278,   267,   268,   269,
     100,   171,   305,   179,   306,   133,   134,   181,   226,   127,
     184,   100,   346,   194,   307,   199,   308,   151,   152,   100,
      89,   131,   219,   246,   102,   103,   363,   104,   306,   192,
     132,   191,   365,   135,   101,   102,   103,   136,   104,   229,
      89,   326,   300,   102,   103,   207,   104,   100,   137,    92,
     226,    91,   364,   138,   308,   155,   156,    54,   157,   140,
     158,   159,   160,   161,   325,    61,   162,   226,   163,   226,
     360,   102,   103,   139,   104,   233,   234,   235,   236,   353,
     233,   234,   235,   236,   237,   141,   226,    68,   142,   281,
      89,   326,   260,    94,   235,   236,    71,   143,   144,    92,
     145,    91,   148,   146,   226,   291,   292,   293,   294,   149,
     295,   296,   154,   233,   234,   235,   236,   150,   155,   156,
      54,   157,   338,   158,   159,   160,   161,   193,    61,   162,
     312,   163,   233,   234,   235,   236,   220,   164,   165,   217,
     238,   166,   239,   167,   263,   185,   257,   256,   324,   258,
     168,   169,    98,   110,   111,   120,    94,   121,   259,    71,
     155,   156,    54,   157,   202,   158,   159,   160,   161,   276,
      61,   162,   277,   163,   297,  -182,   298,   222,   224,   164,
     165,   230,   231,   166,   285,   167,    98,   110,   111,   120,
    -140,   121,   168,   169,    98,   110,   111,   120,    94,   121,
    -215,    71,   309,   313,   315,   155,   156,    54,   157,   359,
     158,   159,   160,   161,   318,    61,   162,   319,   163,   233,
     234,   235,   236,   321,   164,   165,   329,   330,   323,   332,
     167,   283,   333,   233,   234,   235,   236,   168,   169,    98,
     110,   111,   120,    94,   121,   331,    71,   155,   156,    54,
     157,   352,   158,   159,   208,   161,   355,   209,   162,   210,
      63,    64,    65,    66,   366,   233,   234,   235,   236,   372,
     373,   279,   288,    19,    20,    21,    22,    23,   379,    68,
     233,   234,   235,   236,   380,    94,   383,    38,    71,   155,
     156,    54,   157,    39,   158,   159,   208,   161,    28,   209,
     162,   210,    63,    64,    65,    66,   155,   156,    54,   157,
     180,   158,   159,   160,   161,   310,    61,   244,   311,   163,
      81,    68,   354,   280,   384,   314,   381,    94,    87,   245,
      71,    18,    19,    20,    21,    22,    23,    24,    25,   320,
     246,   102,   103,   342,   247,   123,   344,    71,   155,   156,
      54,   157,   322,   158,   159,   160,   161,   106,    61,   244,
     343,   163,   347,   350,    32,   317,     0,     0,   155,   156,
      54,   157,   299,   158,   159,   160,   161,     0,    61,   244,
       0,   163,   300,   102,   103,     0,   247,     0,     0,    71,
       2,   341,     3,     4,     5,     6,     7,     8,     9,    10,
       0,     0,   246,   102,   103,     0,   247,     0,     0,    71,
     155,   156,    54,   157,     0,   158,   159,   160,   161,     0,
      61,   244,     0,   163,     0,   155,   156,    54,   157,     0,
     158,   159,   160,   161,   362,    61,   162,     0,   163,     0,
       0,     0,     0,     0,   300,   102,   103,     0,   247,     0,
       0,    71,   185,     0,     0,     0,     0,   186,     0,     0,
       0,     0,     0,    94,     0,     0,    71,   155,   156,    54,
     157,     0,   158,   159,   160,   161,     0,    61,   162,     0,
     163,     0,   155,   156,    54,   157,     0,   158,   159,   160,
     161,   241,    61,   162,     0,   163,     0,     0,     0,    68,
       0,     0,     0,     0,     0,    94,   243,     0,    71,     0,
       0,     0,     0,     0,    68,     0,     0,     0,     0,     0,
      94,     0,     0,    71,   155,   156,    54,   157,     0,   158,
     159,   160,   161,     0,    61,   162,     0,   163,     0,   155,
     156,    54,   157,     0,   158,   159,   160,   161,   264,    61,
     162,     0,   163,     0,     0,     0,    68,     0,     0,     0,
       0,     0,    94,   282,     0,    71,     0,     0,     0,     0,
       0,    68,     0,     0,     0,     0,     0,    94,     0,     0,
      71,   155,   156,    54,   157,     0,   158,   159,   160,   161,
       0,    61,   162,     0,   163,     0,     0,     0,     0,     0,
       0,   155,   156,    54,   157,   284,   158,   159,   160,   161,
     325,    61,   162,    68,   163,     0,     0,     0,     0,    94,
       0,     0,    71,     0,   155,   156,    54,   157,     0,   158,
     159,   160,   161,    68,    61,   162,     0,   163,     0,    94,
       0,     0,    71,     0,     0,     0,     0,     0,   225,     0,
       0,   202,     0,   155,   156,    54,   157,     0,   158,   159,
     160,   161,    94,    61,   162,    71,   163,     0,     0,     0,
       0,     0,     0,   155,   156,    54,   157,   225,   158,   159,
     160,   161,     0,    61,   162,     0,   163,     0,     0,     0,
       0,    94,     0,     0,    71,     0,   166,   155,   156,    54,
     157,    37,   158,   159,   160,   161,     0,    61,   162,     0,
     163,    94,     0,     0,    71,     2,     0,     3,     4,     5,
       6,     7,     8,     9,    10,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    94,     0,     0,    71
};

static const yytype_int16 yycheck[] =
{
      47,    47,   185,   185,   201,   285,     0,     0,   185,    47,
      52,    47,    57,    51,    59,    43,    41,    45,    41,    47,
      58,    41,    12,    13,    14,    15,    16,    42,    52,    56,
      68,    56,    59,    48,    59,    59,    60,    57,    62,    52,
      53,    54,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    42,    28,    29,    42,    31,    32,    33,    34,
      35,    48,   245,    56,    88,    58,    59,    60,    61,    62,
      63,    64,    65,   118,    44,    50,    41,    41,    48,    42,
      45,    56,    57,   130,    59,    48,    52,    42,   368,    29,
      42,    56,   139,    48,    59,   133,    48,    57,    42,   146,
     146,   139,    44,   139,    48,   287,    48,   145,   146,    42,
     146,    52,    52,    53,    54,    48,    56,    57,    45,   166,
      47,   304,   304,    49,   306,   308,   323,   304,   164,   165,
      49,    42,   168,   169,    38,   182,   183,    48,   185,    57,
      37,    38,    39,    40,   182,   183,   182,   183,   186,    53,
      54,    55,    49,   200,    42,    53,    54,   340,   341,    42,
      48,   137,   200,   346,   200,    48,   213,   213,   144,    25,
      42,   218,    52,   149,   221,   213,    48,   213,   154,    46,
     218,    48,   218,   221,   208,   221,   210,    53,    54,    55,
      29,   129,    46,   131,    48,    48,    49,   135,   245,    36,
     138,    29,    41,   141,    46,   143,    48,    53,    54,    29,
     257,    36,   150,    52,    53,    54,    46,    56,    48,   257,
      49,   257,    42,    36,    52,    53,    54,    41,    56,   167,
     277,   277,    52,    53,    54,   273,    56,    29,    41,   277,
     287,   277,    46,    37,    48,    18,    19,    20,    21,    36,
      23,    24,    25,    26,    27,    28,    29,   304,    31,   306,
      52,    53,    54,    41,    56,    37,    38,    39,    40,    42,
      37,    38,    39,    40,    46,    36,   323,    50,    41,   217,
     327,   327,    49,    56,    39,    40,    59,    36,    41,   327,
      41,   327,    49,    41,   341,   233,   234,   235,   236,    41,
     238,   239,    41,    37,    38,    39,    40,    36,    18,    19,
      20,    21,    46,    23,    24,    25,    26,    57,    28,    29,
     258,    31,    37,    38,    39,    40,    49,    37,    38,    36,
      36,    41,    45,    43,    49,    45,    48,    42,   276,    36,
      50,    51,    52,    53,    54,    55,    56,    57,    49,    59,
      18,    19,    20,    21,    45,    23,    24,    25,    26,    36,
      28,    29,    41,    31,    49,    41,    49,   164,   165,    37,
      38,   168,   169,    41,    43,    43,    52,    53,    54,    55,
      43,    57,    50,    51,    52,    53,    54,    55,    56,    57,
      43,    59,    49,    49,    49,    18,    19,    20,    21,   337,
      23,    24,    25,    26,    48,    28,    29,    49,    31,    37,
      38,    39,    40,    49,    37,    38,    41,    49,    41,    49,
      43,    49,    49,    37,    38,    39,    40,    50,    51,    52,
      53,    54,    55,    56,    57,    49,    59,    18,    19,    20,
      21,    41,    23,    24,    25,    26,    57,    28,    29,    30,
      31,    32,    33,    34,    46,    37,    38,    39,    40,    49,
      36,    42,    44,     5,     6,     7,     8,     9,    57,    50,
      37,    38,    39,    40,    49,    56,    49,    35,    59,    18,
      19,    20,    21,    35,    23,    24,    25,    26,    15,    28,
      29,    30,    31,    32,    33,    34,    18,    19,    20,    21,
     133,    23,    24,    25,    26,   257,    28,    29,   258,    31,
      47,    50,   327,   213,   378,   262,   375,    56,    47,    41,
      59,     4,     5,     6,     7,     8,     9,    10,    11,   273,
      52,    53,    54,   304,    56,    69,   306,    59,    18,    19,
      20,    21,   275,    23,    24,    25,    26,    53,    28,    29,
     304,    31,   308,   318,    27,   266,    -1,    -1,    18,    19,
      20,    21,    42,    23,    24,    25,    26,    -1,    28,    29,
      -1,    31,    52,    53,    54,    -1,    56,    -1,    -1,    59,
      56,    41,    58,    59,    60,    61,    62,    63,    64,    65,
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
      21,    42,    23,    24,    25,    26,    -1,    28,    29,    -1,
      31,    56,    -1,    -1,    59,    56,    -1,    58,    59,    60,
      61,    62,    63,    64,    65,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    56,    -1,    -1,    59
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     0,    56,    58,    59,    60,    61,    62,    63,    64,
      65,    67,    68,    71,    73,    74,    75,     0,     4,     5,
       6,     7,     8,     9,    10,    11,    69,    72,    75,    52,
     147,   148,   147,    41,    41,    70,    76,    42,    71,    73,
      12,    13,    14,    15,    16,    42,    77,   121,   122,    49,
      49,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    28,    29,    31,    32,    33,    34,    35,    50,    56,
      57,    59,    78,    79,    80,    81,    84,    87,    88,    94,
      95,    98,    99,   100,   101,   102,   103,   117,   118,   119,
     120,   131,   145,    52,    56,    82,    83,   145,    52,   146,
      29,    52,    53,    54,    56,    85,   138,   139,    57,    57,
      53,    54,   143,   145,   143,   146,   146,   146,    25,   145,
      55,    57,   132,   133,   142,   143,   146,    36,   146,    45,
      47,    36,    49,    48,    49,    36,    41,    41,    37,    41,
      36,    36,    41,    36,    41,    41,    41,   143,    49,    41,
      36,    53,    54,   144,    41,    18,    19,    21,    23,    24,
      25,    26,    29,    31,    37,    38,    41,    43,    50,    51,
     119,   124,   125,   128,   129,   131,   133,   145,   119,   124,
      83,   124,    86,    86,   124,    45,    50,    89,    90,    91,
      92,   131,   145,    57,   124,    57,    96,    97,   138,   124,
      86,    41,    45,   113,   114,   115,   116,   145,    25,    28,
      30,    78,    98,   104,   105,   110,   117,    36,    86,   124,
      49,    86,   125,   131,   125,    42,   119,   127,   130,   124,
     125,   125,    43,    37,    38,    39,    40,    46,    36,    45,
      49,    42,    78,    42,    29,    41,    52,    56,    93,   126,
     128,   130,   134,   136,   139,   145,    42,    48,    36,    49,
      49,    42,    48,    49,    42,   115,    38,    53,    54,    55,
     140,   141,    42,    48,    42,    48,    36,    41,   146,    42,
     105,   124,    42,    49,    42,    43,    42,    48,    44,   123,
     124,   124,   124,   124,   124,   124,   124,    49,    49,    42,
      52,   135,   139,    46,    48,    46,    48,    46,    48,    49,
      91,    92,   124,    49,    97,    49,    42,   141,    48,    49,
     114,    49,   116,    41,   124,    27,    78,   106,   107,    41,
      49,    49,    49,    49,   123,   130,    44,    48,    46,    42,
      48,    41,   126,   134,   129,   130,    41,   136,   137,   139,
     140,   115,    41,    42,   107,    57,   111,   112,    44,   124,
      52,   139,    42,    46,    46,    42,    46,    42,    41,    57,
     108,   109,    49,    36,    42,    48,   123,    42,    48,    57,
      49,   112,    42,    49,   109
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_uint8 yyr1[] =
{
       0,    66,    67,    67,    67,    68,    69,    69,    69,    70,
      70,    71,    72,    72,    72,    72,    72,    73,    73,    74,
      74,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      76,    76,    76,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    78,
      78,    78,    79,    80,    81,    82,    82,    83,    83,    84,
      85,    85,    86,    86,    87,    88,    89,    89,    90,    90,
      91,    91,    91,    91,    91,    92,    92,    92,    92,    92,
      93,    93,    93,    94,    95,    96,    96,    97,    97,    98,
      99,    99,   100,   101,   102,   102,   103,   104,   104,   105,
     105,   105,   105,   105,   106,   106,   107,   107,   108,   108,
     109,   109,   110,   111,   111,   112,   113,   113,   114,   114,
     114,   115,   115,   116,   117,   118,   118,   118,   118,   119,
     119,   120,   120,   120,   120,   120,   120,   120,   120,   120,
     120,   120,   121,   121,   122,   122,   122,   122,   122,   123,
     123,   123,   124,   124,   124,   124,   124,   124,   125,   125,
     125,   125,   125,   125,   125,   125,   125,   126,   126,   126,
     126,   127,   127,   128,   129,   129,   130,   131,   131,   131,
     131,   132,   132,   133,   133,   133,   133,   134,   134,   134,
     134,   135,   135,   135,   135,   136,   136,   137,   137,   138,
     138,   139,   139,   139,   139,   140,   140,   141,   141,   141,
     142,   143,   143,   144,   144,   145,   145,   146,   147,   147,
     148
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     1,     7,     1,     1,     1,     2,
       0,     7,     1,     1,     1,     1,     1,     1,     0,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     3,     0,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     4,
       2,     3,     4,     5,     3,     3,     1,     1,     3,     6,
       3,     1,     2,     0,     6,     6,     1,     0,     3,     1,
       3,     3,     1,     2,     1,     3,     5,     3,     5,     3,
       4,     2,     0,     5,     6,     3,     1,     1,     1,     6,
       5,     6,     5,     6,     6,     6,     6,     2,     1,     5,
       1,     1,     1,     1,     2,     1,     5,     1,     3,     1,
       1,     3,     6,     3,     1,     3,     3,     1,     3,     5,
       3,     3,     1,     5,     6,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     1,     1,     1,     1,     1,     3,
       1,     0,     3,     3,     3,     3,     3,     1,     2,     2,
       2,     2,     1,     4,     1,     1,     3,     3,     3,     1,
       1,     3,     1,     3,     1,     2,     4,     1,     3,     4,
       6,     1,     0,     1,     1,     1,     1,     3,     3,     1,
       1,     3,     3,     1,     1,     3,     1,     1,     2,     1,
       1,     1,     1,     1,     1,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       1
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
#line 257 "src/xkbcomp/parser.y"
            { free(((*yyvaluep).str)); }
#line 1634 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_XkbFile: /* XkbFile  */
#line 255 "src/xkbcomp/parser.y"
            { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1640 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_XkbCompositeMap: /* XkbCompositeMap  */
#line 255 "src/xkbcomp/parser.y"
            { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1646 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_XkbMapConfigList: /* XkbMapConfigList  */
#line 256 "src/xkbcomp/parser.y"
            { FreeXkbFile(((*yyvaluep).fileList).head); }
#line 1652 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_XkbMapConfig: /* XkbMapConfig  */
#line 255 "src/xkbcomp/parser.y"
            { if (!param->rtrn) FreeXkbFile(((*yyvaluep).file)); }
#line 1658 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_DeclList: /* DeclList  */
#line 251 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).anyList).head); }
#line 1664 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Decl: /* Decl  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).any)); }
#line 1670 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VarDecl: /* VarDecl  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).var)); }
#line 1676 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyNameDecl: /* KeyNameDecl  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).keyCode)); }
#line 1682 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyAliasDecl: /* KeyAliasDecl  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).keyAlias)); }
#line 1688 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VModDecl: /* VModDecl  */
#line 251 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).vmodList).head); }
#line 1694 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VModDefList: /* VModDefList  */
#line 251 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).vmodList).head); }
#line 1700 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VModDef: /* VModDef  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).vmod)); }
#line 1706 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_InterpretDecl: /* InterpretDecl  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).interp)); }
#line 1712 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_InterpretMatch: /* InterpretMatch  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).interp)); }
#line 1718 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_VarDeclList: /* VarDeclList  */
#line 251 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).varList).head); }
#line 1724 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyTypeDecl: /* KeyTypeDecl  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).keyType)); }
#line 1730 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_SymbolsDecl: /* SymbolsDecl  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).syms)); }
#line 1736 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_OptSymbolsBody: /* OptSymbolsBody  */
#line 251 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).varList).head); }
#line 1742 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_SymbolsBody: /* SymbolsBody  */
#line 251 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).varList).head); }
#line 1748 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_SymbolsVarDecl: /* SymbolsVarDecl  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).var)); }
#line 1754 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_MultiKeySymOrActionList: /* MultiKeySymOrActionList  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1760 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_GroupCompatDecl: /* GroupCompatDecl  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).groupCompat)); }
#line 1766 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_ModMapDecl: /* ModMapDecl  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).modMask)); }
#line 1772 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyOrKeySymList: /* KeyOrKeySymList  */
#line 251 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1778 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeyOrKeySym: /* KeyOrKeySym  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1784 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_LedMapDecl: /* LedMapDecl  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).ledMap)); }
#line 1790 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_LedNameDecl: /* LedNameDecl  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).ledName)); }
#line 1796 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_CoordList: /* CoordList  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1802 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Coord: /* Coord  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1808 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_ExprList: /* ExprList  */
#line 251 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1814 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Expr: /* Expr  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1820 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Term: /* Term  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1826 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_MultiActionList: /* MultiActionList  */
#line 251 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1832 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_ActionList: /* ActionList  */
#line 251 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1838 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_NonEmptyActions: /* NonEmptyActions  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1844 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Actions: /* Actions  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1850 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Action: /* Action  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1856 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Lhs: /* Lhs  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1862 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_OptTerminal: /* OptTerminal  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1868 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_Terminal: /* Terminal  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1874 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_MultiKeySymList: /* MultiKeySymList  */
#line 251 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).exprList).head); }
#line 1880 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeySymList: /* KeySymList  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1886 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_NonEmptyKeySyms: /* NonEmptyKeySyms  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1892 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_KeySyms: /* KeySyms  */
#line 248 "src/xkbcomp/parser.y"
            { FreeStmt((ParseCommon *) ((*yyvaluep).expr)); }
#line 1898 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_OptMapName: /* OptMapName  */
#line 257 "src/xkbcomp/parser.y"
            { free(((*yyvaluep).str)); }
#line 1904 "src/xkbcomp/parser.c"
        break;

    case YYSYMBOL_MapName: /* MapName  */
#line 257 "src/xkbcomp/parser.y"
            { free(((*yyvaluep).str)); }
#line 1910 "src/xkbcomp/parser.c"
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
#line 274 "src/xkbcomp/parser.y"
                        { (yyval.file) = param->rtrn = (yyvsp[0].file); param->more_maps = !!param->rtrn; (void) yynerrs; }
#line 2189 "src/xkbcomp/parser.c"
    break;

  case 3: /* XkbFile: XkbMapConfig  */
#line 276 "src/xkbcomp/parser.y"
                        { (yyval.file) = param->rtrn = (yyvsp[0].file); param->more_maps = !!param->rtrn; YYACCEPT; }
#line 2195 "src/xkbcomp/parser.c"
    break;

  case 4: /* XkbFile: "end of file"  */
#line 278 "src/xkbcomp/parser.y"
                        { (yyval.file) = param->rtrn = NULL; param->more_maps = false; }
#line 2201 "src/xkbcomp/parser.c"
    break;

  case 5: /* XkbCompositeMap: OptFlags XkbCompositeType OptMapName "{" XkbMapConfigList "}" ";"  */
#line 284 "src/xkbcomp/parser.y"
                        { (yyval.file) = XkbFileCreate((yyvsp[-5].file_type), (yyvsp[-4].str), (ParseCommon *) (yyvsp[-2].fileList).head, (yyvsp[-6].mapFlags)); }
#line 2207 "src/xkbcomp/parser.c"
    break;

  case 6: /* XkbCompositeType: "xkb_keymap"  */
#line 287 "src/xkbcomp/parser.y"
                                        { (yyval.file_type) = FILE_TYPE_KEYMAP; }
#line 2213 "src/xkbcomp/parser.c"
    break;

  case 7: /* XkbCompositeType: "xkb_semantics"  */
#line 288 "src/xkbcomp/parser.y"
                                        { (yyval.file_type) = FILE_TYPE_KEYMAP; }
#line 2219 "src/xkbcomp/parser.c"
    break;

  case 8: /* XkbCompositeType: "xkb_layout"  */
#line 289 "src/xkbcomp/parser.y"
                                        { (yyval.file_type) = FILE_TYPE_KEYMAP; }
#line 2225 "src/xkbcomp/parser.c"
    break;

  case 9: /* XkbMapConfigList: XkbMapConfigList XkbMapConfig  */
#line 296 "src/xkbcomp/parser.y"
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
#line 2241 "src/xkbcomp/parser.c"
    break;

  case 10: /* XkbMapConfigList: %empty  */
#line 307 "src/xkbcomp/parser.y"
                        { (yyval.fileList).head = (yyval.fileList).last = NULL; }
#line 2247 "src/xkbcomp/parser.c"
    break;

  case 11: /* XkbMapConfig: OptFlags FileType OptMapName "{" DeclList "}" ";"  */
#line 313 "src/xkbcomp/parser.y"
                        {
                            (yyval.file) = XkbFileCreate((yyvsp[-5].file_type), (yyvsp[-4].str), (yyvsp[-2].anyList).head, (yyvsp[-6].mapFlags));
                        }
#line 2255 "src/xkbcomp/parser.c"
    break;

  case 12: /* FileType: "xkb_keycodes"  */
#line 318 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_KEYCODES; }
#line 2261 "src/xkbcomp/parser.c"
    break;

  case 13: /* FileType: "xkb_types"  */
#line 319 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_TYPES; }
#line 2267 "src/xkbcomp/parser.c"
    break;

  case 14: /* FileType: "xkb_compatibility"  */
#line 320 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_COMPAT; }
#line 2273 "src/xkbcomp/parser.c"
    break;

  case 15: /* FileType: "xkb_symbols"  */
#line 321 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_SYMBOLS; }
#line 2279 "src/xkbcomp/parser.c"
    break;

  case 16: /* FileType: "xkb_geometry"  */
#line 322 "src/xkbcomp/parser.y"
                                                { (yyval.file_type) = FILE_TYPE_GEOMETRY; }
#line 2285 "src/xkbcomp/parser.c"
    break;

  case 17: /* OptFlags: Flags  */
#line 325 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = (yyvsp[0].mapFlags); }
#line 2291 "src/xkbcomp/parser.c"
    break;

  case 18: /* OptFlags: %empty  */
#line 326 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = 0; }
#line 2297 "src/xkbcomp/parser.c"
    break;

  case 19: /* Flags: Flags Flag  */
#line 329 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = ((yyvsp[-1].mapFlags) | (yyvsp[0].mapFlags)); }
#line 2303 "src/xkbcomp/parser.c"
    break;

  case 20: /* Flags: Flag  */
#line 330 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = (yyvsp[0].mapFlags); }
#line 2309 "src/xkbcomp/parser.c"
    break;

  case 21: /* Flag: "partial"  */
#line 333 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_PARTIAL; }
#line 2315 "src/xkbcomp/parser.c"
    break;

  case 22: /* Flag: "default"  */
#line 334 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_DEFAULT; }
#line 2321 "src/xkbcomp/parser.c"
    break;

  case 23: /* Flag: "hidden"  */
#line 335 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_HIDDEN; }
#line 2327 "src/xkbcomp/parser.c"
    break;

  case 24: /* Flag: "alphanumeric_keys"  */
#line 336 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_HAS_ALPHANUMERIC; }
#line 2333 "src/xkbcomp/parser.c"
    break;

  case 25: /* Flag: "modifier_keys"  */
#line 337 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_HAS_MODIFIER; }
#line 2339 "src/xkbcomp/parser.c"
    break;

  case 26: /* Flag: "keypad_keys"  */
#line 338 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_HAS_KEYPAD; }
#line 2345 "src/xkbcomp/parser.c"
    break;

  case 27: /* Flag: "function_keys"  */
#line 339 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_HAS_FN; }
#line 2351 "src/xkbcomp/parser.c"
    break;

  case 28: /* Flag: "alternate_group"  */
#line 340 "src/xkbcomp/parser.y"
                                                { (yyval.mapFlags) = MAP_IS_ALTGR; }
#line 2357 "src/xkbcomp/parser.c"
    break;

  case 29: /* Flag: "identifier"  */
#line 342 "src/xkbcomp/parser.y"
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
#line 2376 "src/xkbcomp/parser.c"
    break;

  case 30: /* DeclList: DeclList Decl  */
#line 359 "src/xkbcomp/parser.y"
                        {
                            if ((yyvsp[0].any)) {
                                if ((yyvsp[-1].anyList).head) {
                                    (yyval.anyList).head = (yyvsp[-1].anyList).head; (yyvsp[-1].anyList).last->next = (yyvsp[0].any); (yyval.anyList).last = (yyvsp[0].any);
                                } else {
                                    (yyval.anyList).head = (yyval.anyList).last = (yyvsp[0].any);
                                }
                            }
                        }
#line 2390 "src/xkbcomp/parser.c"
    break;

  case 31: /* DeclList: DeclList OptMergeMode VModDecl  */
#line 374 "src/xkbcomp/parser.y"
                        {
                            for (VModDef *vmod = (yyvsp[0].vmodList).head; vmod; vmod = (VModDef *) vmod->common.next)
                                vmod->merge = (yyvsp[-1].merge);
                            if ((yyvsp[-2].anyList).head) {
                                (yyval.anyList).head = (yyvsp[-2].anyList).head; (yyvsp[-2].anyList).last->next = &(yyvsp[0].vmodList).head->common; (yyval.anyList).last = &(yyvsp[0].vmodList).last->common;
                            } else {
                                (yyval.anyList).head = &(yyvsp[0].vmodList).head->common; (yyval.anyList).last = &(yyvsp[0].vmodList).last->common;
                            }
                        }
#line 2404 "src/xkbcomp/parser.c"
    break;

  case 32: /* DeclList: %empty  */
#line 383 "src/xkbcomp/parser.y"
                        { (yyval.anyList).head = (yyval.anyList).last = NULL; }
#line 2410 "src/xkbcomp/parser.c"
    break;

  case 33: /* Decl: OptMergeMode VarDecl  */
#line 387 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].var)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].var);
                        }
#line 2419 "src/xkbcomp/parser.c"
    break;

  case 34: /* Decl: OptMergeMode InterpretDecl  */
#line 393 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].interp)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].interp);
                        }
#line 2428 "src/xkbcomp/parser.c"
    break;

  case 35: /* Decl: OptMergeMode KeyNameDecl  */
#line 398 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].keyCode)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].keyCode);
                        }
#line 2437 "src/xkbcomp/parser.c"
    break;

  case 36: /* Decl: OptMergeMode KeyAliasDecl  */
#line 403 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].keyAlias)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].keyAlias);
                        }
#line 2446 "src/xkbcomp/parser.c"
    break;

  case 37: /* Decl: OptMergeMode KeyTypeDecl  */
#line 408 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].keyType)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].keyType);
                        }
#line 2455 "src/xkbcomp/parser.c"
    break;

  case 38: /* Decl: OptMergeMode SymbolsDecl  */
#line 413 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].syms)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].syms);
                        }
#line 2464 "src/xkbcomp/parser.c"
    break;

  case 39: /* Decl: OptMergeMode ModMapDecl  */
#line 418 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].modMask)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].modMask);
                        }
#line 2473 "src/xkbcomp/parser.c"
    break;

  case 40: /* Decl: OptMergeMode GroupCompatDecl  */
#line 423 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].groupCompat)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].groupCompat);
                        }
#line 2482 "src/xkbcomp/parser.c"
    break;

  case 41: /* Decl: OptMergeMode LedMapDecl  */
#line 428 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].ledMap)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].ledMap);
                        }
#line 2491 "src/xkbcomp/parser.c"
    break;

  case 42: /* Decl: OptMergeMode LedNameDecl  */
#line 433 "src/xkbcomp/parser.y"
                        {
                            (yyvsp[0].ledName)->merge = (yyvsp[-1].merge);
                            (yyval.any) = (ParseCommon *) (yyvsp[0].ledName);
                        }
#line 2500 "src/xkbcomp/parser.c"
    break;

  case 43: /* Decl: OptMergeMode ShapeDecl  */
#line 437 "src/xkbcomp/parser.y"
                                                        { (yyval.any) = NULL; }
#line 2506 "src/xkbcomp/parser.c"
    break;

  case 44: /* Decl: OptMergeMode SectionDecl  */
#line 438 "src/xkbcomp/parser.y"
                                                        { (yyval.any) = NULL; }
#line 2512 "src/xkbcomp/parser.c"
    break;

  case 45: /* Decl: OptMergeMode DoodadDecl  */
#line 439 "src/xkbcomp/parser.y"
                                                        { (yyval.any) = NULL; }
#line 2518 "src/xkbcomp/parser.c"
    break;

  case 46: /* Decl: OptMergeMode UnknownDecl  */
#line 441 "src/xkbcomp/parser.y"
                            { (yyval.any) = (ParseCommon *) (yyvsp[0].unknown); }
#line 2524 "src/xkbcomp/parser.c"
    break;

  case 47: /* Decl: OptMergeMode UnknownCompoundStatementDecl  */
#line 443 "src/xkbcomp/parser.y"
                            { (yyval.any) = (ParseCommon *) (yyvsp[0].unknown); }
#line 2530 "src/xkbcomp/parser.c"
    break;

  case 48: /* Decl: MergeMode "string literal"  */
#line 445 "src/xkbcomp/parser.y"
                        {
                            (yyval.any) = (ParseCommon *) IncludeCreate(param->ctx, (yyvsp[0].str), (yyvsp[-1].merge));
                            free((yyvsp[0].str));
                        }
#line 2539 "src/xkbcomp/parser.c"
    break;

  case 49: /* VarDecl: Lhs "=" Expr ";"  */
#line 452 "src/xkbcomp/parser.y"
                        { (yyval.var) = VarCreate((yyvsp[-3].expr), (yyvsp[-1].expr)); }
#line 2545 "src/xkbcomp/parser.c"
    break;

  case 50: /* VarDecl: Ident ";"  */
#line 454 "src/xkbcomp/parser.y"
                        { (yyval.var) = BoolVarCreate((yyvsp[-1].atom), true); }
#line 2551 "src/xkbcomp/parser.c"
    break;

  case 51: /* VarDecl: "!" Ident ";"  */
#line 456 "src/xkbcomp/parser.y"
                        { (yyval.var) = BoolVarCreate((yyvsp[-1].atom), false); }
#line 2557 "src/xkbcomp/parser.c"
    break;

  case 52: /* KeyNameDecl: "key name" "=" KeyCode ";"  */
#line 460 "src/xkbcomp/parser.y"
                        { (yyval.keyCode) = KeycodeCreate((yyvsp[-3].atom), (yyvsp[-1].num)); }
#line 2563 "src/xkbcomp/parser.c"
    break;

  case 53: /* KeyAliasDecl: "alias" "key name" "=" "key name" ";"  */
#line 464 "src/xkbcomp/parser.y"
                        { (yyval.keyAlias) = KeyAliasCreate((yyvsp[-3].atom), (yyvsp[-1].atom)); }
#line 2569 "src/xkbcomp/parser.c"
    break;

  case 54: /* VModDecl: "virtual_modifiers" VModDefList ";"  */
#line 468 "src/xkbcomp/parser.y"
                        { (yyval.vmodList) = (yyvsp[-1].vmodList); }
#line 2575 "src/xkbcomp/parser.c"
    break;

  case 55: /* VModDefList: VModDefList "," VModDef  */
#line 472 "src/xkbcomp/parser.y"
                        { (yyval.vmodList).head = (yyvsp[-2].vmodList).head; (yyval.vmodList).last->common.next = &(yyvsp[0].vmod)->common; (yyval.vmodList).last = (yyvsp[0].vmod); }
#line 2581 "src/xkbcomp/parser.c"
    break;

  case 56: /* VModDefList: VModDef  */
#line 474 "src/xkbcomp/parser.y"
                        { (yyval.vmodList).head = (yyval.vmodList).last = (yyvsp[0].vmod); }
#line 2587 "src/xkbcomp/parser.c"
    break;

  case 57: /* VModDef: Ident  */
#line 478 "src/xkbcomp/parser.y"
                        { (yyval.vmod) = VModCreate((yyvsp[0].atom), NULL); }
#line 2593 "src/xkbcomp/parser.c"
    break;

  case 58: /* VModDef: Ident "=" Expr  */
#line 480 "src/xkbcomp/parser.y"
                        { (yyval.vmod) = VModCreate((yyvsp[-2].atom), (yyvsp[0].expr)); }
#line 2599 "src/xkbcomp/parser.c"
    break;

  case 59: /* InterpretDecl: "interpret" InterpretMatch "{" VarDeclList "}" ";"  */
#line 486 "src/xkbcomp/parser.y"
                        { (yyvsp[-4].interp)->def = (yyvsp[-2].varList).head; (yyval.interp) = (yyvsp[-4].interp); }
#line 2605 "src/xkbcomp/parser.c"
    break;

  case 60: /* InterpretMatch: KeySym "+" Expr  */
#line 490 "src/xkbcomp/parser.y"
                        { (yyval.interp) = InterpCreate((yyvsp[-2].keysym), (yyvsp[0].expr)); }
#line 2611 "src/xkbcomp/parser.c"
    break;

  case 61: /* InterpretMatch: KeySym  */
#line 492 "src/xkbcomp/parser.y"
                        { (yyval.interp) = InterpCreate((yyvsp[0].keysym), NULL); }
#line 2617 "src/xkbcomp/parser.c"
    break;

  case 62: /* VarDeclList: VarDeclList VarDecl  */
#line 496 "src/xkbcomp/parser.y"
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
#line 2633 "src/xkbcomp/parser.c"
    break;

  case 63: /* VarDeclList: %empty  */
#line 507 "src/xkbcomp/parser.y"
                        { (yyval.varList).head = (yyval.varList).last = NULL; }
#line 2639 "src/xkbcomp/parser.c"
    break;

  case 64: /* KeyTypeDecl: "type" String "{" VarDeclList "}" ";"  */
#line 513 "src/xkbcomp/parser.y"
                        { (yyval.keyType) = KeyTypeCreate((yyvsp[-4].atom), (yyvsp[-2].varList).head); }
#line 2645 "src/xkbcomp/parser.c"
    break;

  case 65: /* SymbolsDecl: "key" "key name" "{" OptSymbolsBody "}" ";"  */
#line 519 "src/xkbcomp/parser.y"
                        { (yyval.syms) = SymbolsCreate((yyvsp[-4].atom), (yyvsp[-2].varList).head); }
#line 2651 "src/xkbcomp/parser.c"
    break;

  case 66: /* OptSymbolsBody: SymbolsBody  */
#line 522 "src/xkbcomp/parser.y"
                                    { (yyval.varList) = (yyvsp[0].varList); }
#line 2657 "src/xkbcomp/parser.c"
    break;

  case 67: /* OptSymbolsBody: %empty  */
#line 523 "src/xkbcomp/parser.y"
                                    { (yyval.varList).head = (yyval.varList).last = NULL; }
#line 2663 "src/xkbcomp/parser.c"
    break;

  case 68: /* SymbolsBody: SymbolsBody "," SymbolsVarDecl  */
#line 527 "src/xkbcomp/parser.y"
                        { (yyval.varList).head = (yyvsp[-2].varList).head; (yyval.varList).last->common.next = &(yyvsp[0].var)->common; (yyval.varList).last = (yyvsp[0].var); }
#line 2669 "src/xkbcomp/parser.c"
    break;

  case 69: /* SymbolsBody: SymbolsVarDecl  */
#line 529 "src/xkbcomp/parser.y"
                        { (yyval.varList).head = (yyval.varList).last = (yyvsp[0].var); }
#line 2675 "src/xkbcomp/parser.c"
    break;

  case 70: /* SymbolsVarDecl: Lhs "=" Expr  */
#line 532 "src/xkbcomp/parser.y"
                                                { (yyval.var) = VarCreate((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2681 "src/xkbcomp/parser.c"
    break;

  case 71: /* SymbolsVarDecl: Lhs "=" MultiKeySymOrActionList  */
#line 533 "src/xkbcomp/parser.y"
                                                           { (yyval.var) = VarCreate((yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 2687 "src/xkbcomp/parser.c"
    break;

  case 72: /* SymbolsVarDecl: Ident  */
#line 534 "src/xkbcomp/parser.y"
                                                { (yyval.var) = BoolVarCreate((yyvsp[0].atom), true); }
#line 2693 "src/xkbcomp/parser.c"
    break;

  case 73: /* SymbolsVarDecl: "!" Ident  */
#line 535 "src/xkbcomp/parser.y"
                                                { (yyval.var) = BoolVarCreate((yyvsp[0].atom), false); }
#line 2699 "src/xkbcomp/parser.c"
    break;

  case 74: /* SymbolsVarDecl: MultiKeySymOrActionList  */
#line 536 "src/xkbcomp/parser.y"
                                                { (yyval.var) = VarCreate(NULL, (yyvsp[0].expr)); }
#line 2705 "src/xkbcomp/parser.c"
    break;

  case 75: /* MultiKeySymOrActionList: "[" MultiKeySymList "]"  */
#line 552 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[-1].exprList).head; }
#line 2711 "src/xkbcomp/parser.c"
    break;

  case 76: /* MultiKeySymOrActionList: "[" NoSymbolOrActionList "," MultiKeySymList "]"  */
#line 554 "src/xkbcomp/parser.y"
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
#line 2733 "src/xkbcomp/parser.c"
    break;

  case 77: /* MultiKeySymOrActionList: "[" MultiActionList "]"  */
#line 572 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[-1].exprList).head; }
#line 2739 "src/xkbcomp/parser.c"
    break;

  case 78: /* MultiKeySymOrActionList: "[" NoSymbolOrActionList "," MultiActionList "]"  */
#line 574 "src/xkbcomp/parser.y"
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
#line 2760 "src/xkbcomp/parser.c"
    break;

  case 79: /* MultiKeySymOrActionList: "[" NoSymbolOrActionList "]"  */
#line 596 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprEmptyList(); }
#line 2766 "src/xkbcomp/parser.c"
    break;

  case 80: /* NoSymbolOrActionList: NoSymbolOrActionList "," "{" "}"  */
#line 602 "src/xkbcomp/parser.y"
                        { (yyval.noSymbolOrActionList) = (yyvsp[-3].noSymbolOrActionList) + 1; }
#line 2772 "src/xkbcomp/parser.c"
    break;

  case 81: /* NoSymbolOrActionList: "{" "}"  */
#line 604 "src/xkbcomp/parser.y"
                        { (yyval.noSymbolOrActionList) = 1; }
#line 2778 "src/xkbcomp/parser.c"
    break;

  case 82: /* NoSymbolOrActionList: %empty  */
#line 605 "src/xkbcomp/parser.y"
                        { (yyval.noSymbolOrActionList) = 0; }
#line 2784 "src/xkbcomp/parser.c"
    break;

  case 83: /* GroupCompatDecl: "group" Integer "=" Expr ";"  */
#line 609 "src/xkbcomp/parser.y"
                        { (yyval.groupCompat) = GroupCompatCreate((yyvsp[-3].num), (yyvsp[-1].expr)); }
#line 2790 "src/xkbcomp/parser.c"
    break;

  case 84: /* ModMapDecl: "modifier_map" Ident "{" KeyOrKeySymList "}" ";"  */
#line 613 "src/xkbcomp/parser.y"
                        { (yyval.modMask) = ModMapCreate((yyvsp[-4].atom), (yyvsp[-2].exprList).head); }
#line 2796 "src/xkbcomp/parser.c"
    break;

  case 85: /* KeyOrKeySymList: KeyOrKeySymList "," KeyOrKeySym  */
#line 617 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyvsp[-2].exprList).head; (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common; (yyval.exprList).last = (yyvsp[0].expr); }
#line 2802 "src/xkbcomp/parser.c"
    break;

  case 86: /* KeyOrKeySymList: KeyOrKeySym  */
#line 619 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 2808 "src/xkbcomp/parser.c"
    break;

  case 87: /* KeyOrKeySym: "key name"  */
#line 623 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateKeyName((yyvsp[0].atom)); }
#line 2814 "src/xkbcomp/parser.c"
    break;

  case 88: /* KeyOrKeySym: KeySym  */
#line 625 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateKeySym((yyvsp[0].keysym)); }
#line 2820 "src/xkbcomp/parser.c"
    break;

  case 89: /* LedMapDecl: "indicator" String "{" VarDeclList "}" ";"  */
#line 629 "src/xkbcomp/parser.y"
                        { (yyval.ledMap) = LedMapCreate((yyvsp[-4].atom), (yyvsp[-2].varList).head); }
#line 2826 "src/xkbcomp/parser.c"
    break;

  case 90: /* LedNameDecl: "indicator" Integer "=" Expr ";"  */
#line 633 "src/xkbcomp/parser.y"
                        { (yyval.ledName) = LedNameCreate((yyvsp[-3].num), (yyvsp[-1].expr), false); }
#line 2832 "src/xkbcomp/parser.c"
    break;

  case 91: /* LedNameDecl: "virtual" "indicator" Integer "=" Expr ";"  */
#line 635 "src/xkbcomp/parser.y"
                        { (yyval.ledName) = LedNameCreate((yyvsp[-3].num), (yyvsp[-1].expr), true); }
#line 2838 "src/xkbcomp/parser.c"
    break;

  case 92: /* UnknownDecl: "identifier" Terminal "=" Expr ";"  */
#line 639 "src/xkbcomp/parser.y"
                        {
                            FreeStmt((ParseCommon *) (yyvsp[-3].expr));
                            FreeStmt((ParseCommon *) (yyvsp[-1].expr));
                            (yyval.unknown) = UnknownStatementCreate(STMT_UNKNOWN_DECLARATION, (yyvsp[-4].sval));
                        }
#line 2848 "src/xkbcomp/parser.c"
    break;

  case 93: /* UnknownCompoundStatementDecl: "identifier" OptTerminal "{" VarDeclList "}" ";"  */
#line 648 "src/xkbcomp/parser.y"
                        {
                            FreeStmt((ParseCommon *) (yyvsp[-4].expr));
                            FreeStmt((ParseCommon *) (yyvsp[-2].varList).head);
                            (yyval.unknown) = UnknownStatementCreate(STMT_UNKNOWN_COMPOUND, (yyvsp[-5].sval));
                        }
#line 2858 "src/xkbcomp/parser.c"
    break;

  case 94: /* ShapeDecl: "shape" String "{" OutlineList "}" ";"  */
#line 656 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2864 "src/xkbcomp/parser.c"
    break;

  case 95: /* ShapeDecl: "shape" String "{" CoordList "}" ";"  */
#line 658 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[-2].expr); (yyval.geom) = NULL; }
#line 2870 "src/xkbcomp/parser.c"
    break;

  case 96: /* SectionDecl: "section" String "{" SectionBody "}" ";"  */
#line 662 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2876 "src/xkbcomp/parser.c"
    break;

  case 97: /* SectionBody: SectionBody SectionBodyItem  */
#line 665 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL;}
#line 2882 "src/xkbcomp/parser.c"
    break;

  case 98: /* SectionBody: SectionBodyItem  */
#line 666 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL; }
#line 2888 "src/xkbcomp/parser.c"
    break;

  case 99: /* SectionBodyItem: "row" "{" RowBody "}" ";"  */
#line 670 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2894 "src/xkbcomp/parser.c"
    break;

  case 100: /* SectionBodyItem: VarDecl  */
#line 672 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[0].var)); (yyval.geom) = NULL; }
#line 2900 "src/xkbcomp/parser.c"
    break;

  case 101: /* SectionBodyItem: DoodadDecl  */
#line 674 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2906 "src/xkbcomp/parser.c"
    break;

  case 102: /* SectionBodyItem: LedMapDecl  */
#line 676 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[0].ledMap)); (yyval.geom) = NULL; }
#line 2912 "src/xkbcomp/parser.c"
    break;

  case 103: /* SectionBodyItem: OverlayDecl  */
#line 678 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2918 "src/xkbcomp/parser.c"
    break;

  case 104: /* RowBody: RowBody RowBodyItem  */
#line 681 "src/xkbcomp/parser.y"
                                                { (yyval.geom) = NULL;}
#line 2924 "src/xkbcomp/parser.c"
    break;

  case 105: /* RowBody: RowBodyItem  */
#line 682 "src/xkbcomp/parser.y"
                                                { (yyval.geom) = NULL; }
#line 2930 "src/xkbcomp/parser.c"
    break;

  case 106: /* RowBodyItem: "keys" "{" Keys "}" ";"  */
#line 685 "src/xkbcomp/parser.y"
                                                     { (yyval.geom) = NULL; }
#line 2936 "src/xkbcomp/parser.c"
    break;

  case 107: /* RowBodyItem: VarDecl  */
#line 687 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[0].var)); (yyval.geom) = NULL; }
#line 2942 "src/xkbcomp/parser.c"
    break;

  case 108: /* Keys: Keys "," Key  */
#line 690 "src/xkbcomp/parser.y"
                                                { (yyval.geom) = NULL; }
#line 2948 "src/xkbcomp/parser.c"
    break;

  case 109: /* Keys: Key  */
#line 691 "src/xkbcomp/parser.y"
                                                { (yyval.geom) = NULL; }
#line 2954 "src/xkbcomp/parser.c"
    break;

  case 110: /* Key: "key name"  */
#line 695 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2960 "src/xkbcomp/parser.c"
    break;

  case 111: /* Key: "{" ExprList "}"  */
#line 697 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[-1].exprList).head); (yyval.geom) = NULL; }
#line 2966 "src/xkbcomp/parser.c"
    break;

  case 112: /* OverlayDecl: "overlay" String "{" OverlayKeyList "}" ";"  */
#line 701 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 2972 "src/xkbcomp/parser.c"
    break;

  case 113: /* OverlayKeyList: OverlayKeyList "," OverlayKey  */
#line 704 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL; }
#line 2978 "src/xkbcomp/parser.c"
    break;

  case 114: /* OverlayKeyList: OverlayKey  */
#line 705 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL; }
#line 2984 "src/xkbcomp/parser.c"
    break;

  case 115: /* OverlayKey: "key name" "=" "key name"  */
#line 708 "src/xkbcomp/parser.y"
                                                        { (yyval.geom) = NULL; }
#line 2990 "src/xkbcomp/parser.c"
    break;

  case 116: /* OutlineList: OutlineList "," OutlineInList  */
#line 712 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL;}
#line 2996 "src/xkbcomp/parser.c"
    break;

  case 117: /* OutlineList: OutlineInList  */
#line 714 "src/xkbcomp/parser.y"
                        { (yyval.geom) = NULL; }
#line 3002 "src/xkbcomp/parser.c"
    break;

  case 118: /* OutlineInList: "{" CoordList "}"  */
#line 718 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[-1].expr); (yyval.geom) = NULL; }
#line 3008 "src/xkbcomp/parser.c"
    break;

  case 119: /* OutlineInList: Ident "=" "{" CoordList "}"  */
#line 720 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[-1].expr); (yyval.geom) = NULL; }
#line 3014 "src/xkbcomp/parser.c"
    break;

  case 120: /* OutlineInList: Ident "=" Expr  */
#line 722 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[0].expr)); (yyval.geom) = NULL; }
#line 3020 "src/xkbcomp/parser.c"
    break;

  case 121: /* CoordList: CoordList "," Coord  */
#line 726 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[-2].expr); (void) (yyvsp[0].expr); (yyval.expr) = NULL; }
#line 3026 "src/xkbcomp/parser.c"
    break;

  case 122: /* CoordList: Coord  */
#line 728 "src/xkbcomp/parser.y"
                        { (void) (yyvsp[0].expr); (yyval.expr) = NULL; }
#line 3032 "src/xkbcomp/parser.c"
    break;

  case 123: /* Coord: "[" SignedNumber "," SignedNumber "]"  */
#line 732 "src/xkbcomp/parser.y"
                        { (yyval.expr) = NULL; }
#line 3038 "src/xkbcomp/parser.c"
    break;

  case 124: /* DoodadDecl: DoodadType String "{" VarDeclList "}" ";"  */
#line 736 "src/xkbcomp/parser.y"
                        { FreeStmt((ParseCommon *) (yyvsp[-2].varList).head); (yyval.geom) = NULL; }
#line 3044 "src/xkbcomp/parser.c"
    break;

  case 125: /* DoodadType: "text"  */
#line 739 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3050 "src/xkbcomp/parser.c"
    break;

  case 126: /* DoodadType: "outline"  */
#line 740 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3056 "src/xkbcomp/parser.c"
    break;

  case 127: /* DoodadType: "solid"  */
#line 741 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3062 "src/xkbcomp/parser.c"
    break;

  case 128: /* DoodadType: "logo"  */
#line 742 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3068 "src/xkbcomp/parser.c"
    break;

  case 129: /* FieldSpec: Ident  */
#line 745 "src/xkbcomp/parser.y"
                                { (yyval.atom) = (yyvsp[0].atom); }
#line 3074 "src/xkbcomp/parser.c"
    break;

  case 130: /* FieldSpec: Element  */
#line 746 "src/xkbcomp/parser.y"
                                { (yyval.atom) = (yyvsp[0].atom); }
#line 3080 "src/xkbcomp/parser.c"
    break;

  case 131: /* Element: "action"  */
#line 750 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "action"); }
#line 3086 "src/xkbcomp/parser.c"
    break;

  case 132: /* Element: "interpret"  */
#line 752 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "interpret"); }
#line 3092 "src/xkbcomp/parser.c"
    break;

  case 133: /* Element: "type"  */
#line 754 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "type"); }
#line 3098 "src/xkbcomp/parser.c"
    break;

  case 134: /* Element: "key"  */
#line 756 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "key"); }
#line 3104 "src/xkbcomp/parser.c"
    break;

  case 135: /* Element: "group"  */
#line 758 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "group"); }
#line 3110 "src/xkbcomp/parser.c"
    break;

  case 136: /* Element: "modifier_map"  */
#line 760 "src/xkbcomp/parser.y"
                        {(yyval.atom) = xkb_atom_intern_literal(param->ctx, "modifier_map");}
#line 3116 "src/xkbcomp/parser.c"
    break;

  case 137: /* Element: "indicator"  */
#line 762 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "indicator"); }
#line 3122 "src/xkbcomp/parser.c"
    break;

  case 138: /* Element: "shape"  */
#line 764 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "shape"); }
#line 3128 "src/xkbcomp/parser.c"
    break;

  case 139: /* Element: "row"  */
#line 766 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "row"); }
#line 3134 "src/xkbcomp/parser.c"
    break;

  case 140: /* Element: "section"  */
#line 768 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "section"); }
#line 3140 "src/xkbcomp/parser.c"
    break;

  case 141: /* Element: "text"  */
#line 770 "src/xkbcomp/parser.y"
                        { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "text"); }
#line 3146 "src/xkbcomp/parser.c"
    break;

  case 142: /* OptMergeMode: MergeMode  */
#line 773 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = (yyvsp[0].merge); }
#line 3152 "src/xkbcomp/parser.c"
    break;

  case 143: /* OptMergeMode: %empty  */
#line 774 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_DEFAULT; }
#line 3158 "src/xkbcomp/parser.c"
    break;

  case 144: /* MergeMode: "include"  */
#line 777 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_DEFAULT; }
#line 3164 "src/xkbcomp/parser.c"
    break;

  case 145: /* MergeMode: "augment"  */
#line 778 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_AUGMENT; }
#line 3170 "src/xkbcomp/parser.c"
    break;

  case 146: /* MergeMode: "override"  */
#line 779 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_OVERRIDE; }
#line 3176 "src/xkbcomp/parser.c"
    break;

  case 147: /* MergeMode: "replace"  */
#line 780 "src/xkbcomp/parser.y"
                                        { (yyval.merge) = MERGE_REPLACE; }
#line 3182 "src/xkbcomp/parser.c"
    break;

  case 148: /* MergeMode: "alternate"  */
#line 782 "src/xkbcomp/parser.y"
                {
                    /*
                     * This used to be MERGE_ALT_FORM. This functionality was
                     * unused and has been removed.
                     */
                    parser_warn(param, XKB_LOG_MESSAGE_NO_ID,
                                "ignored unsupported legacy merge mode \"alternate\"");
                    (yyval.merge) = MERGE_DEFAULT;
                }
#line 3196 "src/xkbcomp/parser.c"
    break;

  case 149: /* ExprList: ExprList "," Expr  */
#line 794 "src/xkbcomp/parser.y"
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
#line 3212 "src/xkbcomp/parser.c"
    break;

  case 150: /* ExprList: Expr  */
#line 806 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 3218 "src/xkbcomp/parser.c"
    break;

  case 151: /* ExprList: %empty  */
#line 807 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = NULL; }
#line 3224 "src/xkbcomp/parser.c"
    break;

  case 152: /* Expr: Expr "/" Expr  */
#line 811 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_DIVIDE, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3230 "src/xkbcomp/parser.c"
    break;

  case 153: /* Expr: Expr "+" Expr  */
#line 813 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_ADD, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3236 "src/xkbcomp/parser.c"
    break;

  case 154: /* Expr: Expr "-" Expr  */
#line 815 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_SUBTRACT, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3242 "src/xkbcomp/parser.c"
    break;

  case 155: /* Expr: Expr "*" Expr  */
#line 817 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_MULTIPLY, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3248 "src/xkbcomp/parser.c"
    break;

  case 156: /* Expr: Lhs "=" Expr  */
#line 819 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateBinary(STMT_EXPR_ASSIGN, (yyvsp[-2].expr), (yyvsp[0].expr)); }
#line 3254 "src/xkbcomp/parser.c"
    break;

  case 157: /* Expr: Term  */
#line 821 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3260 "src/xkbcomp/parser.c"
    break;

  case 158: /* Term: "-" Term  */
#line 825 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateUnary(STMT_EXPR_NEGATE, (yyvsp[0].expr)); }
#line 3266 "src/xkbcomp/parser.c"
    break;

  case 159: /* Term: "+" Term  */
#line 827 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateUnary(STMT_EXPR_UNARY_PLUS, (yyvsp[0].expr)); }
#line 3272 "src/xkbcomp/parser.c"
    break;

  case 160: /* Term: "!" Term  */
#line 829 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateUnary(STMT_EXPR_NOT, (yyvsp[0].expr)); }
#line 3278 "src/xkbcomp/parser.c"
    break;

  case 161: /* Term: "~" Term  */
#line 831 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateUnary(STMT_EXPR_INVERT, (yyvsp[0].expr)); }
#line 3284 "src/xkbcomp/parser.c"
    break;

  case 162: /* Term: Lhs  */
#line 833 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3290 "src/xkbcomp/parser.c"
    break;

  case 163: /* Term: FieldSpec "(" ExprList ")"  */
#line 835 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateAction((yyvsp[-3].atom), (yyvsp[-1].exprList).head); }
#line 3296 "src/xkbcomp/parser.c"
    break;

  case 164: /* Term: Actions  */
#line 837 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3302 "src/xkbcomp/parser.c"
    break;

  case 165: /* Term: Terminal  */
#line 839 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3308 "src/xkbcomp/parser.c"
    break;

  case 166: /* Term: "(" Expr ")"  */
#line 841 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[-1].expr); }
#line 3314 "src/xkbcomp/parser.c"
    break;

  case 167: /* MultiActionList: MultiActionList "," Action  */
#line 845 "src/xkbcomp/parser.y"
                        {
                            ExprDef *expr = ExprCreateActionList((yyvsp[0].expr));
                            (yyval.exprList) = (yyvsp[-2].exprList);
                            (yyval.exprList).last->common.next = &expr->common; (yyval.exprList).last = expr;
                        }
#line 3324 "src/xkbcomp/parser.c"
    break;

  case 168: /* MultiActionList: MultiActionList "," Actions  */
#line 851 "src/xkbcomp/parser.y"
                        { (yyval.exprList) = (yyvsp[-2].exprList); (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common; (yyval.exprList).last = (yyvsp[0].expr); }
#line 3330 "src/xkbcomp/parser.c"
    break;

  case 169: /* MultiActionList: Action  */
#line 853 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = ExprCreateActionList((yyvsp[0].expr)); }
#line 3336 "src/xkbcomp/parser.c"
    break;

  case 170: /* MultiActionList: NonEmptyActions  */
#line 855 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 3342 "src/xkbcomp/parser.c"
    break;

  case 171: /* ActionList: ActionList "," Action  */
#line 859 "src/xkbcomp/parser.y"
                        { (yyval.exprList) = (yyvsp[-2].exprList); (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common; (yyval.exprList).last = (yyvsp[0].expr); }
#line 3348 "src/xkbcomp/parser.c"
    break;

  case 172: /* ActionList: Action  */
#line 861 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 3354 "src/xkbcomp/parser.c"
    break;

  case 173: /* NonEmptyActions: "{" ActionList "}"  */
#line 865 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateActionList((yyvsp[-1].exprList).head); }
#line 3360 "src/xkbcomp/parser.c"
    break;

  case 174: /* Actions: NonEmptyActions  */
#line 869 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3366 "src/xkbcomp/parser.c"
    break;

  case 175: /* Actions: "{" "}"  */
#line 871 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateActionList(NULL); }
#line 3372 "src/xkbcomp/parser.c"
    break;

  case 176: /* Action: FieldSpec "(" ExprList ")"  */
#line 875 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateAction((yyvsp[-3].atom), (yyvsp[-1].exprList).head); }
#line 3378 "src/xkbcomp/parser.c"
    break;

  case 177: /* Lhs: FieldSpec  */
#line 879 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateIdent((yyvsp[0].atom)); }
#line 3384 "src/xkbcomp/parser.c"
    break;

  case 178: /* Lhs: FieldSpec "." FieldSpec  */
#line 881 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateFieldRef((yyvsp[-2].atom), (yyvsp[0].atom)); }
#line 3390 "src/xkbcomp/parser.c"
    break;

  case 179: /* Lhs: FieldSpec "[" Expr "]"  */
#line 883 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateArrayRef(XKB_ATOM_NONE, (yyvsp[-3].atom), (yyvsp[-1].expr)); }
#line 3396 "src/xkbcomp/parser.c"
    break;

  case 180: /* Lhs: FieldSpec "." FieldSpec "[" Expr "]"  */
#line 885 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateArrayRef((yyvsp[-5].atom), (yyvsp[-3].atom), (yyvsp[-1].expr)); }
#line 3402 "src/xkbcomp/parser.c"
    break;

  case 181: /* OptTerminal: Terminal  */
#line 889 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3408 "src/xkbcomp/parser.c"
    break;

  case 182: /* OptTerminal: %empty  */
#line 890 "src/xkbcomp/parser.y"
                        { (yyval.expr) = NULL; }
#line 3414 "src/xkbcomp/parser.c"
    break;

  case 183: /* Terminal: String  */
#line 894 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateString((yyvsp[0].atom)); }
#line 3420 "src/xkbcomp/parser.c"
    break;

  case 184: /* Terminal: Integer  */
#line 896 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateInteger((yyvsp[0].num)); }
#line 3426 "src/xkbcomp/parser.c"
    break;

  case 185: /* Terminal: Float  */
#line 898 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateFloat(/* Discard $1 */); }
#line 3432 "src/xkbcomp/parser.c"
    break;

  case 186: /* Terminal: "key name"  */
#line 900 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateKeyName((yyvsp[0].atom)); }
#line 3438 "src/xkbcomp/parser.c"
    break;

  case 187: /* MultiKeySymList: MultiKeySymList "," KeySymLit  */
#line 904 "src/xkbcomp/parser.y"
                        {
                            ExprDef *expr = ExprCreateKeySymList((yyvsp[0].keysym));
                            (yyval.exprList) = (yyvsp[-2].exprList);
                            (yyval.exprList).last->common.next = &expr->common; (yyval.exprList).last = expr;
                        }
#line 3448 "src/xkbcomp/parser.c"
    break;

  case 188: /* MultiKeySymList: MultiKeySymList "," KeySyms  */
#line 910 "src/xkbcomp/parser.y"
                        { (yyval.exprList) = (yyvsp[-2].exprList); (yyval.exprList).last->common.next = &(yyvsp[0].expr)->common; (yyval.exprList).last = (yyvsp[0].expr); }
#line 3454 "src/xkbcomp/parser.c"
    break;

  case 189: /* MultiKeySymList: KeySymLit  */
#line 912 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = ExprCreateKeySymList((yyvsp[0].keysym)); }
#line 3460 "src/xkbcomp/parser.c"
    break;

  case 190: /* MultiKeySymList: NonEmptyKeySyms  */
#line 914 "src/xkbcomp/parser.y"
                        { (yyval.exprList).head = (yyval.exprList).last = (yyvsp[0].expr); }
#line 3466 "src/xkbcomp/parser.c"
    break;

  case 191: /* KeySymList: KeySymList "," KeySymLit  */
#line 918 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprAppendKeySymList((yyvsp[-2].expr), (yyvsp[0].keysym)); }
#line 3472 "src/xkbcomp/parser.c"
    break;

  case 192: /* KeySymList: KeySymList "," "string literal"  */
#line 920 "src/xkbcomp/parser.y"
                        {
                            (yyval.expr) = ExprKeySymListAppendString(param->scanner, (yyvsp[-2].expr), (yyvsp[0].str));
                            free((yyvsp[0].str));
                            if (!(yyval.expr))
                                YYERROR;
                        }
#line 3483 "src/xkbcomp/parser.c"
    break;

  case 193: /* KeySymList: KeySymLit  */
#line 927 "src/xkbcomp/parser.y"
                        {
                            (yyval.expr) = ExprCreateKeySymList((yyvsp[0].keysym));
                            if (!(yyval.expr))
                                YYERROR;
                        }
#line 3493 "src/xkbcomp/parser.c"
    break;

  case 194: /* KeySymList: "string literal"  */
#line 933 "src/xkbcomp/parser.y"
                        {
                            (yyval.expr) = ExprCreateKeySymList(XKB_KEY_NoSymbol);
                            if (!(yyval.expr))
                                YYERROR;
                            (yyval.expr) = ExprKeySymListAppendString(param->scanner, (yyval.expr), (yyvsp[0].str));
                            free((yyvsp[0].str));
                            if (!(yyval.expr))
                                YYERROR;
                        }
#line 3507 "src/xkbcomp/parser.c"
    break;

  case 195: /* NonEmptyKeySyms: "{" KeySymList "}"  */
#line 945 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[-1].expr); }
#line 3513 "src/xkbcomp/parser.c"
    break;

  case 196: /* NonEmptyKeySyms: "string literal"  */
#line 947 "src/xkbcomp/parser.y"
                        {
                            (yyval.expr) = ExprCreateKeySymList(XKB_KEY_NoSymbol);
                            if (!(yyval.expr))
                                YYERROR;
                            (yyval.expr) = ExprKeySymListAppendString(param->scanner, (yyval.expr), (yyvsp[0].str));
                            free((yyvsp[0].str));
                            if (!(yyval.expr))
                                YYERROR;
                        }
#line 3527 "src/xkbcomp/parser.c"
    break;

  case 197: /* KeySyms: NonEmptyKeySyms  */
#line 959 "src/xkbcomp/parser.y"
                        { (yyval.expr) = (yyvsp[0].expr); }
#line 3533 "src/xkbcomp/parser.c"
    break;

  case 198: /* KeySyms: "{" "}"  */
#line 961 "src/xkbcomp/parser.y"
                        { (yyval.expr) = ExprCreateKeySymList(XKB_KEY_NoSymbol); }
#line 3539 "src/xkbcomp/parser.c"
    break;

  case 199: /* KeySym: KeySymLit  */
#line 965 "src/xkbcomp/parser.y"
                        { (yyval.keysym) = (yyvsp[0].keysym); }
#line 3545 "src/xkbcomp/parser.c"
    break;

  case 200: /* KeySym: "string literal"  */
#line 967 "src/xkbcomp/parser.y"
                        {
                            (yyval.keysym) = KeysymParseString(param->scanner, (yyvsp[0].str));
                            free((yyvsp[0].str));
                            if ((yyval.keysym) == XKB_KEY_NoSymbol)
                                YYERROR;
                        }
#line 3556 "src/xkbcomp/parser.c"
    break;

  case 201: /* KeySymLit: "identifier"  */
#line 976 "src/xkbcomp/parser.y"
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
#line 3572 "src/xkbcomp/parser.c"
    break;

  case 202: /* KeySymLit: "section"  */
#line 988 "src/xkbcomp/parser.y"
                                { (yyval.keysym) = XKB_KEY_section; }
#line 3578 "src/xkbcomp/parser.c"
    break;

  case 203: /* KeySymLit: "decimal digit"  */
#line 990 "src/xkbcomp/parser.y"
                        {
                            /*
                             * Special case for digits 0..9:
                             * map to XKB_KEY_0 .. XKB_KEY_9, consistent with
                             * other keysym names: <name> → XKB_KEY_<name>.
                             */
                            (yyval.keysym) = XKB_KEY_0 + (xkb_keysym_t) (yyvsp[0].num);
                        }
#line 3591 "src/xkbcomp/parser.c"
    break;

  case 204: /* KeySymLit: "integer literal"  */
#line 999 "src/xkbcomp/parser.y"
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
#line 3653 "src/xkbcomp/parser.c"
    break;

  case 205: /* SignedNumber: "-" Number  */
#line 1058 "src/xkbcomp/parser.y"
                                        { (yyval.num) = -(yyvsp[0].num); }
#line 3659 "src/xkbcomp/parser.c"
    break;

  case 206: /* SignedNumber: Number  */
#line 1059 "src/xkbcomp/parser.y"
                                        { (yyval.num) = (yyvsp[0].num); }
#line 3665 "src/xkbcomp/parser.c"
    break;

  case 207: /* Number: "float literal"  */
#line 1062 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3671 "src/xkbcomp/parser.c"
    break;

  case 208: /* Number: "decimal digit"  */
#line 1063 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3677 "src/xkbcomp/parser.c"
    break;

  case 209: /* Number: "integer literal"  */
#line 1064 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3683 "src/xkbcomp/parser.c"
    break;

  case 210: /* Float: "float literal"  */
#line 1067 "src/xkbcomp/parser.y"
                                { (yyval.num) = 0; }
#line 3689 "src/xkbcomp/parser.c"
    break;

  case 211: /* Integer: "integer literal"  */
#line 1070 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3695 "src/xkbcomp/parser.c"
    break;

  case 212: /* Integer: "decimal digit"  */
#line 1071 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3701 "src/xkbcomp/parser.c"
    break;

  case 213: /* KeyCode: "integer literal"  */
#line 1074 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3707 "src/xkbcomp/parser.c"
    break;

  case 214: /* KeyCode: "decimal digit"  */
#line 1075 "src/xkbcomp/parser.y"
                                      { (yyval.num) = (yyvsp[0].num); }
#line 3713 "src/xkbcomp/parser.c"
    break;

  case 215: /* Ident: "identifier"  */
#line 1078 "src/xkbcomp/parser.y"
                                { (yyval.atom) = xkb_atom_intern(param->ctx, (yyvsp[0].sval).start, (yyvsp[0].sval).len); }
#line 3719 "src/xkbcomp/parser.c"
    break;

  case 216: /* Ident: "default"  */
#line 1079 "src/xkbcomp/parser.y"
                                { (yyval.atom) = xkb_atom_intern_literal(param->ctx, "default"); }
#line 3725 "src/xkbcomp/parser.c"
    break;

  case 217: /* String: "string literal"  */
#line 1082 "src/xkbcomp/parser.y"
                                { (yyval.atom) = xkb_atom_intern(param->ctx, (yyvsp[0].str), strlen((yyvsp[0].str))); free((yyvsp[0].str)); }
#line 3731 "src/xkbcomp/parser.c"
    break;

  case 218: /* OptMapName: MapName  */
#line 1085 "src/xkbcomp/parser.y"
                                { (yyval.str) = (yyvsp[0].str); }
#line 3737 "src/xkbcomp/parser.c"
    break;

  case 219: /* OptMapName: %empty  */
#line 1086 "src/xkbcomp/parser.y"
                                { (yyval.str) = NULL; }
#line 3743 "src/xkbcomp/parser.c"
    break;

  case 220: /* MapName: "string literal"  */
#line 1089 "src/xkbcomp/parser.y"
                                { (yyval.str) = (yyvsp[0].str); }
#line 3749 "src/xkbcomp/parser.c"
    break;


#line 3753 "src/xkbcomp/parser.c"

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

#line 1092 "src/xkbcomp/parser.y"


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
