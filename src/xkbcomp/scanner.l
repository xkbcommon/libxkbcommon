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
#include "parser-priv.h"

#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC diagnostic push

struct scanner_extra {
    struct xkb_context *ctx;
    const char *file_name;
    char scanBuf[1024];
    char *s;
};

static void
scanner_error_extra(struct YYLTYPE *loc, struct scanner_extra *extra,
                    const char *msg);

#define YY_USER_ACTION {                \
        yylloc->first_line = yylineno;  \
        yylloc->last_line = yylineno;   \
}

#define APPEND_S(ch) do {                                               \
    if (yyextra->s - yyextra->scanBuf >= sizeof(yyextra->scanBuf) - 1)  \
        return ERROR_TOK;                                               \
    *yyextra->s++ = ch;                                                 \
} while (0)
%}

%option prefix="_xkbcommon_"
%option reentrant
%option extra-type="struct scanner_extra *"
%option bison-bridge bison-locations
%option yylineno
%option nounistd noyywrap noinput nounput
%option never-interactive
%option case-insensitive

%x S_STR

%%

"//"[^\n]*
"#"[^\n]*

\"                      yyextra->s = yyextra->scanBuf; BEGIN(S_STR);

<S_STR>\" {
                        BEGIN(INITIAL);
                        *yyextra->s = '\0';
                        yylval->str = strdup(yyextra->scanBuf);
                        return STRING;
                    }

<S_STR>\\[0-7]{1,3} {
                        /* octal escape sequence */
                        unsigned int result;

                        (void) sscanf( yytext + 1, "%o", &result );

                        if (result > 0xff) {
                            scanner_error_extra(yylloc, yyextra,
                                                "Illegal octal escape");
                            return ERROR_TOK;
                        }

                        APPEND_S(result);
                    }

<S_STR>\\[0-9]+ {
                        scanner_error_extra(yylloc, yyextra,
                                            "Illegal octal escape");
                        return ERROR_TOK;
                    }

<S_STR>\\n              APPEND_S('\n');
<S_STR>\\t              APPEND_S('\t');
<S_STR>\\r              APPEND_S('\r');
<S_STR>\\b              APPEND_S('\b');
<S_STR>\\f              APPEND_S('\f');
<S_STR>\\v              APPEND_S('\v');
<S_STR>\\e              APPEND_S('\033');

<S_STR>.                APPEND_S(yytext[0]);

\<[a-zA-Z0-9_+-]+\> {
                        /* We don't want the brackets. */
                        yytext[yyleng - 1] = '\0';
                        yytext++;
                        yylval->sval = xkb_atom_intern(yyextra->ctx, yytext);
                        return KEYNAME;
                    }

xkb_keymap              return XKB_KEYMAP;
xkb_keycodes            return XKB_KEYCODES;
xkb_types               return XKB_TYPES;
xkb_symbols             return XKB_SYMBOLS;
xkb_compat              return XKB_COMPATMAP;
xkb_compat_map          return XKB_COMPATMAP;
xkb_compatibility       return XKB_COMPATMAP;
xkb_compatibility_map   return XKB_COMPATMAP;
xkb_geometry            return XKB_GEOMETRY;
xkb_semantics           return XKB_SEMANTICS;
xkb_layout              return XKB_LAYOUT;
include                 return INCLUDE;
override                return OVERRIDE;
augment                 return AUGMENT;
replace                 return REPLACE;
alternate               return ALTERNATE;
partial                 return PARTIAL;
default                 return DEFAULT;
hidden                  return HIDDEN;
virtual_modifiers       return VIRTUAL_MODS;
type                    return TYPE;
interpret               return INTERPRET;
action                  return ACTION_TOK;
key                     return KEY;
alias                   return ALIAS;
group                   return GROUP;
modmap                  return MODIFIER_MAP;
mod_map                 return MODIFIER_MAP;
modifier_map            return MODIFIER_MAP;
indicator               return INDICATOR;
shape                   return SHAPE;
row                     return ROW;
keys                    return KEYS;
section                 return SECTION;
overlay                 return OVERLAY;
text                    return TEXT;
outline                 return OUTLINE;
solid                   return SOLID;
logo                    return LOGO;
virtual                 return VIRTUAL;
alphanumeric_keys       return ALPHANUMERIC_KEYS;
modifier_keys           return MODIFIER_KEYS;
keypad_keys             return KEYPAD_KEYS;
function_keys           return FUNCTION_KEYS;
alternate_group         return ALTERNATE_GROUP;

[a-zA-Z_][a-zA-Z_0-9]*  yylval->str = strdup(yytext); return IDENT;

0x[a-fA-F0-9]+          |
[0-9]+                  {
                            char *end;
                            yylval->num = strtoul(yytext, &end, 0);

                            return INTEGER;
                        }
[0-9]+\.[0-9]+ {
                            char *end;
                            yylval->num = strtod(yytext, &end);

                            return FLOAT;
                        }

"="                     return EQUALS;
"+"                     return PLUS;
"-"                     return MINUS;
"/"                     return DIVIDE;
"*"                     return TIMES;
"{"                     return OBRACE;
"}"                     return CBRACE;
"("                     return OPAREN;
")"                     return CPAREN;
"["                     return OBRACKET;
"]"                     return CBRACKET;
"."                     return DOT;
","                     return COMMA;
";"                     return SEMI;
"!"                     return EXCLAM;
"~"                     return INVERT;

[ \t\r\n\v]+

<<EOF>>                 return END_OF_FILE;

.                       return ERROR_TOK;

%%

#pragma GCC diagnostic pop

static void
scanner_error_extra(struct YYLTYPE *loc, struct scanner_extra *extra,
                    const char *msg)
{
    log_err(extra->ctx, "%s: line %d of %s\n", msg,
            loc->first_line,
            extra->file_name ? extra->file_name : "(unknown)");
}

void
scanner_error(struct YYLTYPE *loc, void *scanner, const char *msg)
{
    struct scanner_extra *extra = yyget_extra(scanner);
    scanner_error_extra(loc, extra, msg);
}

static bool
init_scanner(yyscan_t *scanner, struct scanner_extra *extra,
             struct xkb_context *ctx, const char *file_name)
{
    memset(extra, 0, sizeof(*extra));

    if (yylex_init_extra(extra, scanner) != 0)
        return false;

    extra->ctx = ctx;
    extra->file_name = file_name;

    return true;
}

static void
clear_scanner(yyscan_t scanner)
{
    yylex_destroy(scanner);
}

XkbFile *
XkbParseString(struct xkb_context *ctx, const char *string,
               const char *file_name)
{
    yyscan_t scanner;
    struct scanner_extra extra;
    YY_BUFFER_STATE state;
    XkbFile *xkb_file;

    if (!init_scanner(&scanner, &extra, ctx, file_name))
        return NULL;

    state = yy_scan_string(string, scanner);

    xkb_file = parse(ctx, scanner, NULL);

    yy_delete_buffer(state, scanner);
    clear_scanner(scanner);

    return xkb_file;
}

XkbFile *
XkbParseFile(struct xkb_context *ctx, FILE *file,
             const char *file_name, const char *map)
{
    yyscan_t scanner;
    struct scanner_extra extra;
    YY_BUFFER_STATE state;
    XkbFile *xkb_file;

    if (!init_scanner(&scanner, &extra, ctx, file_name))
        return NULL;

    state = yy_create_buffer(file, YY_BUF_SIZE, scanner);
    yy_switch_to_buffer(state, scanner);

    xkb_file = parse(ctx, scanner, map);

    yy_delete_buffer(state, scanner);
    clear_scanner(scanner);

    return xkb_file;
}
