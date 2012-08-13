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

struct scanner_extra {
    struct xkb_context *ctx;
    char *scanFile;
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

%option reentrant
%option extra-type="struct scanner_extra *"
%option bison-bridge bison-locations
%option yylineno
%option nounistd noyywrap noinput nounput
%option never-interactive
%option case-insensitive

%x S_STR S_KEY

%%

"//"[^\n]*
"#"[^\n]*

\"                      yyextra->s = yyextra->scanBuf; BEGIN(S_STR);
\<                      yyextra->s = yyextra->scanBuf; BEGIN(S_KEY);

<S_STR>\" {
                        BEGIN(INITIAL);
                        *yyextra->s = '\0';
                        yylval->str = strdup(yyextra->scanBuf);
                        return STRING;
                    }
<S_KEY>\> {
                        BEGIN(INITIAL);

                        *yyextra->s = '\0';
                        if (yyextra->s - yyextra->scanBuf > XkbKeyNameLength) {
                            scanner_error_extra(yylloc, yyextra,
                                                "Key name too long");
                            return ERROR_TOK;
                        }

                        strncpy(yylval->keyName, yyextra->scanBuf, XkbKeyNameLength);
                        return KEYNAME;
                    }

<S_STR,S_KEY>\\[0-7]{1,3} {
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

<S_STR,S_KEY>\\[0-9]+ {
                        scanner_error_extra(yylloc, yyextra,
                                            "Illegal octal escape");
                        return ERROR_TOK;
                    }

<S_STR,S_KEY>\\n        APPEND_S('\n');
<S_STR,S_KEY>\\t        APPEND_S('\t');
<S_STR,S_KEY>\\r        APPEND_S('\r');
<S_STR,S_KEY>\\b        APPEND_S('\b');
<S_STR,S_KEY>\\f        APPEND_S('\f');
<S_STR,S_KEY>\\v        APPEND_S('\v');
<S_STR,S_KEY>\\e        APPEND_S('\033');

<S_STR,S_KEY>.          APPEND_S(yytext[0]);

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
                            yylval->num = strtod(yytext, &end) * XkbGeomPtsPerMM;

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

static void
scanner_error_extra(struct YYLTYPE *loc, struct scanner_extra *extra,
                    const char *msg)
{
    log_err(extra->ctx, "%s: line %d of %s\n", msg,
            loc->first_line,
            extra->scanFile ? extra->scanFile : "(unknown)");

    log_err(extra->ctx, "last scanned symbol was: %s\n",
            extra->scanBuf);
}

void
scanner_error(struct YYLTYPE *loc, void *scanner, const char *msg)
{
    struct scanner_extra *extra = yyget_extra(scanner);
    scanner_error_extra(loc, extra, msg);
}

static void
CheckDefaultMap(struct xkb_context *ctx, XkbFile *maps, const char *fileName)
{
    XkbFile *dflt = NULL, *tmp;

    for (tmp = maps; tmp; tmp = (XkbFile *) tmp->common.next) {
        if (!(tmp->flags & XkbLC_Default))
            continue;

        if (!dflt) {
            dflt = tmp;
            continue;
        }

        log_lvl(ctx, 3,
                "Multiple default components in %s; "
                "Using %s, ignoring %s\n",
                (fileName ? fileName : "(unknown)"),
                (dflt->name ? dflt->name : "(first)"),
                (tmp->name ? tmp->name : "(subsequent)"));

        tmp->flags &= (~XkbLC_Default);
    }
}

bool
XkbParseString(struct xkb_context *ctx, const char *string,
               const char *file_name, XkbFile **out)
{
    int ret;
    struct parser_param param;
    struct scanner_extra extra;
    YY_BUFFER_STATE state;

    if (string == NULL)
        return false;

    param.ctx = ctx;

    memset(&extra, 0, sizeof(extra));
    ret = yylex_init_extra(&extra, &param.scanner);
    if (ret != 0)
        return false;

    extra.ctx = ctx;

    extra.scanFile = strdup(file_name);
    if (!extra.scanFile)
        return false;

    state = yy_scan_string(string, param.scanner);
    ret = yyparse(&param);
    yy_delete_buffer(state, param.scanner);
    yylex_destroy(param.scanner);
    free(extra.scanFile);
    if (ret != 0)
        return false;

    CheckDefaultMap(param.ctx, param.rtrn, file_name);
    *out = param.rtrn;
    return true;
}

bool
XkbParseFile(struct xkb_context *ctx, FILE *file,
             const char *file_name, XkbFile **out)
{
    int ret;
    struct parser_param param;
    struct scanner_extra extra;

    if (!file)
        return false;

    param.ctx = ctx;

    memset(&extra, 0, sizeof(extra));
    ret = yylex_init_extra(&extra, &param.scanner);
    if (ret != 0)
        return false;

    extra.ctx = ctx;

    extra.scanFile = strdup(file_name);
    if (!extra.scanFile)
        return false;

    yyset_in(file, param.scanner);
    ret = yyparse(&param);
    yylex_destroy(param.scanner);
    free(extra.scanFile);
    if (ret != 0)
        return false;

    CheckDefaultMap(param.ctx, param.rtrn, file_name);
    *out = param.rtrn;
    return true;
}
