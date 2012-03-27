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

#include <stdio.h>
#include <ctype.h>

#include "utils.h"
#include "parseutils.h"


extern int yyparse(struct parser_param *param);

#define YY_USER_ACTION {                \
        yylloc->first_line = yylineno;  \
        yylloc->last_line = yylineno;   \
}

%}

%option reentrant
%option extra-type="struct scanner_extra *"
%option bison-bridge bison-locations
%option never-interactive nounistd
%option case-insensitive
%option yylineno
%option noyywrap
%option never-interactive
%option nowarn
%option noinput
%option nounput

%x S_STR S_KEY

%%

"//"[^\n]*
"#"[^\n]*

\"			yyextra->s = yyextra->scanBuf; BEGIN(S_STR);
\<			yyextra->s = yyextra->scanBuf; BEGIN(S_KEY);

<S_STR>\" {
			BEGIN(INITIAL);
			*yyextra->s = '\0';
			yylval->str = strdup(yyextra->scanBuf);
			return STRING;
		    }
<S_KEY>\> {
			BEGIN(INITIAL);
			*yyextra->s = '\0';
			yylval->str = strdup(yyextra->scanBuf);
			return KEYNAME;
		    }

<S_STR,S_KEY>\\[0-7]{1,3} {
			/* octal escape sequence */
			unsigned int result;

			(void) sscanf( yytext + 1, "%o", &result );

			if (result > 0xff) {
			    fprintf(stderr, "Illegal octal escape %s\n", yytext);
			    return ERROR_TOK;
			}

			*yyextra->s++ = result;
		    }

<S_STR,S_KEY>\\[0-9]+ {
			fprintf(stderr, "Illegal octal escape %s\n", yytext);
			return ERROR_TOK;
		    }

<S_STR,S_KEY>\\n	*yyextra->s++ = '\n';
<S_STR,S_KEY>\\t	*yyextra->s++ = '\t';
<S_STR,S_KEY>\\r	*yyextra->s++ = '\r';
<S_STR,S_KEY>\\b	*yyextra->s++ = '\b';
<S_STR,S_KEY>\\f	*yyextra->s++ = '\f';
<S_STR,S_KEY>\\v	*yyextra->s++ = '\v';
<S_STR,S_KEY>\\e	*yyextra->s++ = '\033';

<S_STR,S_KEY>.		*yyextra->s++ = yytext[0];

xkb_keymap		return XKB_KEYMAP;
xkb_keycodes		return XKB_KEYCODES;
xkb_types		return XKB_TYPES;
xkb_symbols		return XKB_SYMBOLS;
xkb_compat		return XKB_COMPATMAP;
xkb_compat_map		return XKB_COMPATMAP;
xkb_compatibility	return XKB_COMPATMAP;
xkb_compatibility_map	return XKB_COMPATMAP;
xkb_geometry		return XKB_GEOMETRY;
xkb_semantics		return XKB_SEMANTICS;
xkb_layout		return XKB_LAYOUT;
include			return INCLUDE;
override		return OVERRIDE;
augment			return AUGMENT;
replace			return REPLACE;
alternate		return ALTERNATE;
partial			return PARTIAL;
default			return DEFAULT;
hidden			return HIDDEN;
virtual_modifiers	return VIRTUAL_MODS;
type			return TYPE;
interpret		return INTERPRET;
action			return ACTION_TOK;
key			return KEY;
alias			return ALIAS;
group			return GROUP;
modmap			return MODIFIER_MAP;
mod_map			return MODIFIER_MAP;
modifier_map		return MODIFIER_MAP;
indicator		return INDICATOR;
shape			return SHAPE;
row			return ROW;
keys			return KEYS;
section			return SECTION;
overlay			return OVERLAY;
text			return TEXT;
outline			return OUTLINE;
solid			return SOLID;
logo			return LOGO;
virtual			return VIRTUAL;
alphanumeric_keys	return ALPHANUMERIC_KEYS;
modifier_keys		return MODIFIER_KEYS;
keypad_keys		return KEYPAD_KEYS;
function_keys		return FUNCTION_KEYS;
alternate_group		return ALTERNATE_GROUP;

[a-zA-Z_][a-zA-Z_0-9]*	yylval->str = strdup(yytext); return IDENT;

0x[a-fA-F0-9]+		|
[0-9]+			{
			    char *end;
			    yylval->num = strtoul(yytext, &end, 0);

			    return INTEGER;
			}
[0-9]+\.[0-9]+ {
			    char *end;
			    yylval->num = strtod(yytext, &end) * XkbGeomPtsPerMM;

			    return FLOAT;
			}

"="			return EQUALS;
"+"			return PLUS;
"-"			return MINUS;
"/"			return DIVIDE;
"*"			return TIMES;
"{"			return OBRACE;
"}"			return CBRACE;
"("			return OPAREN;
")"			return CPAREN;
"["			return OBRACKET;
"]"			return CBRACKET;
"."			return DOT;
","			return COMMA;
";"			return SEMI;
"!"			return EXCLAM;
"~"			return INVERT;

[ \t\r\n\v]+		

<<EOF>>			return END_OF_FILE;

.			return ERROR_TOK;

%%

void
yyerror(YYLTYPE *loc, void *scanner, const char *msg)
{
    struct scanner_extra *extra = yyget_extra(scanner);

    if (warningLevel > 0) {
        fprintf(stderr, "%s: line %d of %s\n", msg, loc->first_line,
                extra->scanFile ? extra->scanFile : "(unknown)");
        if (warningLevel > 3)
            fprintf(stderr, "last scanned symbol is: %s\n", extra->scanBuf);
    }
    return;
}

int
XKBParseString(const char *string, const char *fileName, XkbFile **pRtrn)
{
    YY_BUFFER_STATE state;
    struct parser_param param;
    struct scanner_extra extra;
    int ret;

    *pRtrn = NULL;
    if (string == NULL)
        return 1;

    memset(&extra, 0, sizeof(extra));
    ret = yylex_init_extra(&extra, &param.scanner);
    if (ret != 0)
        return 0;
    extra.scanFile = strdup(fileName);

    state = yy_scan_string(string, param.scanner);
    ret = yyparse(&param);
    yy_delete_buffer(state, param.scanner);
    yylex_destroy(param.scanner);
    free(extra.scanFile);
    if (ret)
        return 0;

    CheckDefaultMap(param.rtrn, fileName);
    *pRtrn = param.rtrn;

    return 1;
}

int
XKBParseFile(FILE * file, const char *fileName, XkbFile ** pRtrn)
{
    int ret;
    struct parser_param param;
    struct scanner_extra extra;

    *pRtrn = NULL;
    if (!file)
        return 1;

    memset(&extra, 0, sizeof(extra));
    if (yylex_init_extra(&extra, &param.scanner) != 0)
        return 0;
    extra.scanFile = strdup(fileName);

    yyset_in(file, param.scanner);
    ret = yyparse(&param);
    yylex_destroy(param.scanner);
    free(extra.scanFile);
    if (ret)
        return 0;

    CheckDefaultMap(param.rtrn, fileName);
    *pRtrn = param.rtrn;
    return 1;
}
