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
#include <X11/Xos.h>

#include "tokens.h"
#include "utils.h"
#include "parseutils.h"

const char *yystring;
static char scanFileBuf[1024] = {0};
char *scanFile = scanFileBuf;
int lineNum = 0;

int scanInt;

static char *s;
char scanBuf[1024];

#define	BUFSIZE	4096

%}

%option case-insensitive
%option yylineno
%option noyywrap

%x S_STR S_KEY

%%

"//"[^\n]*
"#"[^\n]*

\"			s = scanBuf; BEGIN(S_STR);
\<			s = scanBuf; BEGIN(S_KEY);

<S_STR>\"		BEGIN(INITIAL); *s = '\0'; return STRING;
<S_KEY>\>		BEGIN(INITIAL); *s = '\0'; return KEYNAME;

<S_STR,S_KEY>\\[0-7]{1,3} {
			/* octal escape sequence */
			int result;

			(void) sscanf( yytext + 1, "%o", &result );

			if (result > 0xff) {
			    fprintf(stderr, "Illegal octal escape %s\n", yytext);
			    return ERROR_TOK;
			}

			*s++ = result;
		    }

<S_STR,S_KEY>\\[0-9]+ {
			fprintf(stderr, "Illegal octal escape %s\n", yytext);
			return ERROR_TOK;
		    }

<S_STR,S_KEY>\\n	*s++ = '\n';
<S_STR,S_KEY>\\t	*s++ = '\t';
<S_STR,S_KEY>\\r	*s++ = '\r';
<S_STR,S_KEY>\\b	*s++ = '\b';
<S_STR,S_KEY>\\f	*s++ = '\f';
<S_STR,S_KEY>\\v	*s++ = '\v';
<S_STR,S_KEY>\\e	*s++ = '\033';

<S_STR,S_KEY>.		*s++ = yytext[0];

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

[a-zA-Z_][a-zA-Z_0-9]*	memcpy(scanBuf, yytext, yyleng + 1); return IDENT;

0x[a-fA-F0-9]+		|
[0-9]+			{
			    char *end;
			    scanInt = strtol(yytext, &end, 0);

			    return INTEGER;
			}
[0-9]+\.[0-9]+ {
			    char *end;
			    scanInt = strtod(yytext, &end) * XkbGeomPtsPerMM;

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
yyerror(const char *s)
{
    if (warningLevel>0) {
	(void)fprintf(stderr,"%s: line %d of %s\n",s,yylineno,
					(scanFile?scanFile:"(unknown)"));
	if ((warningLevel>3))
	    (void)fprintf(stderr,"last scanned symbol is: %s\n",scanBuf);
    }
    return;
}

void setScanState(char *file, int lineno)
{
  yylineno = 1;
  scanFile = file;
}

int
XKBParseString(const char *string, XkbFile ** pRtrn)
{
    YY_BUFFER_STATE state;

    *pRtrn = NULL;
    if (string == NULL)
	return 1;

    state = yy_scan_string(string);
    rtrnValue = NULL;
    if (yyparse() != 0)
	return 0;

    yy_delete_buffer(state);
    yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
    *pRtrn = rtrnValue;
    CheckDefaultMap(rtrnValue);
    rtrnValue = NULL;

    return 1;
}

int
XKBParseFile(FILE * file, XkbFile ** pRtrn)
{
    if (file)
    {
        yyin = file;
	yystring = NULL;
        rtrnValue = NULL;
        if (yyparse() == 0)
        {
            *pRtrn = rtrnValue;
            CheckDefaultMap(rtrnValue);
            rtrnValue = NULL;
            return 1;
        }
        *pRtrn = NULL;
        return 0;
    }
    *pRtrn = NULL;
    return 1;
}
