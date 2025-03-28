/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include "xkbcomp-priv.h"
#include "parser-priv.h"

static bool
number(struct scanner *s, int64_t *out, int *out_tok)
{
    bool is_float = false, is_hex = false;
    const char *start = s->s + s->pos;
    char *end;

    if (scanner_lit(s, "0x")) {
        while (is_xdigit(scanner_peek(s))) scanner_next(s);
        is_hex = true;
    }
    else {
        while (is_digit(scanner_peek(s))) scanner_next(s);
        is_float = scanner_chr(s, '.');
        while (is_digit(scanner_peek(s))) scanner_next(s);
    }
    if (s->s + s->pos == start)
        return false;

    errno = 0;
    /* We use an intermediate variable, as we may have LLONG_MAX > INT64_MAX */
    long long int x;
    if (is_hex)
        x = strtoll(start, &end, 16);
    else if (is_float) {
        /* The parser currently just ignores floats, so the cast is
         * fine - the value doesn't matter. */
        #ifdef __GNUC__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wbad-function-cast"
        #endif
        x = (long long int) strtold(start, &end);
        #ifdef __GNUC__
        #pragma GCC diagnostic pop
        #endif
    }
    else
        x = strtoll(start, &end, 10);
    if (errno != 0 || s->s + s->pos != end || x > INT64_MAX)
        *out_tok = ERROR_TOK;
    else
        *out_tok = (is_float ? FLOAT : INTEGER);
    *out = (int64_t) x;
    return true;
}

int
_xkbcommon_lex(YYSTYPE *yylval, struct scanner *s)
{
skip_more_whitespace_and_comments:
    /* Skip spaces. */
    while (is_space(scanner_peek(s))) scanner_next(s);

    /* Skip comments. */
    if (scanner_lit(s, "//") || scanner_chr(s, '#')) {
        scanner_skip_to_eol(s);
        goto skip_more_whitespace_and_comments;
    }

    /* See if we're done. */
    if (scanner_eof(s)) return END_OF_FILE;

    /* New token. */
    s->token_pos = s->pos;
    s->buf_pos = 0;

    /* String literal. */
    if (scanner_chr(s, '\"')) {
        while (!scanner_eof(s) && !scanner_eol(s) && scanner_peek(s) != '\"') {
            if (scanner_chr(s, '\\')) {
                uint8_t o;
                size_t start_pos = s->pos;
                if      (scanner_chr(s, '\\')) scanner_buf_append(s, '\\');
                else if (scanner_chr(s, 'n'))  scanner_buf_append(s, '\n');
                else if (scanner_chr(s, 't'))  scanner_buf_append(s, '\t');
                else if (scanner_chr(s, 'r'))  scanner_buf_append(s, '\r');
                else if (scanner_chr(s, 'b'))  scanner_buf_append(s, '\b');
                else if (scanner_chr(s, 'f'))  scanner_buf_append(s, '\f');
                else if (scanner_chr(s, 'v'))  scanner_buf_append(s, '\v');
                else if (scanner_chr(s, 'e'))  scanner_buf_append(s, '\033');
                else if (scanner_oct(s, &o) && is_valid_char((char) o))
                    scanner_buf_append(s, (char) o);
                else if (s->pos > start_pos)
                    scanner_warn(s, XKB_WARNING_INVALID_ESCAPE_SEQUENCE,
                                 "invalid octal escape sequence (%.*s) "
                                 "in string literal",
                                 (int) (s->pos - start_pos + 1),
                                 &s->s[start_pos - 1]);
                    /* Ignore. */
                else {
                    scanner_warn(s, XKB_WARNING_UNKNOWN_CHAR_ESCAPE_SEQUENCE,
                                 "unknown escape sequence (\\%c) in string literal",
                                 scanner_peek(s));
                    /* Ignore. */
                }
            } else {
                scanner_buf_append(s, scanner_next(s));
            }
        }
        if (!scanner_buf_append(s, '\0') || !scanner_chr(s, '\"')) {
            scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                        "unterminated string literal");
            return ERROR_TOK;
        }
        yylval->str = strdup(s->buf);
        if (!yylval->str)
            return ERROR_TOK;
        return STRING;
    }

    /* Key name literal. */
    if (scanner_chr(s, '<')) {
        while (is_graph(scanner_peek(s)) && scanner_peek(s) != '>')
            scanner_next(s);
        if (!scanner_chr(s, '>')) {
            scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                        "unterminated key name literal");
            return ERROR_TOK;
        }
        /* Empty key name literals are allowed. */
        const char *start = s->s + s->token_pos + 1;
        const size_t len = s->pos - s->token_pos - 2;
        yylval->atom = xkb_atom_intern(s->ctx, start, len);
        return KEYNAME;
    }

    /* Operators and punctuation. */
    if (scanner_chr(s, ';')) return SEMI;
    if (scanner_chr(s, '{')) return OBRACE;
    if (scanner_chr(s, '}')) return CBRACE;
    if (scanner_chr(s, '=')) return EQUALS;
    if (scanner_chr(s, '[')) return OBRACKET;
    if (scanner_chr(s, ']')) return CBRACKET;
    if (scanner_chr(s, '(')) return OPAREN;
    if (scanner_chr(s, ')')) return CPAREN;
    if (scanner_chr(s, '.')) return DOT;
    if (scanner_chr(s, ',')) return COMMA;
    if (scanner_chr(s, '+')) return PLUS;
    if (scanner_chr(s, '-')) return MINUS;
    if (scanner_chr(s, '*')) return TIMES;
    if (scanner_chr(s, '/')) return DIVIDE;
    if (scanner_chr(s, '!')) return EXCLAM;
    if (scanner_chr(s, '~')) return INVERT;

    int tok = ERROR_TOK;

    /* Identifier. */
    if (is_alpha(scanner_peek(s)) || scanner_peek(s) == '_') {
        while (is_alnum(scanner_peek(s)) || scanner_peek(s) == '_')
            scanner_next(s);

        const char *start = s->s + s->token_pos;
        const size_t len = s->pos - s->token_pos;

        /* Keyword. */
        tok = keyword_to_token(start, len);
        if (tok >= 0) return tok;

        yylval->sval = SVAL(start, len);
        return IDENT;
    }

    /* Number literal (hexadecimal / decimal / float). */
    if (number(s, &yylval->num, &tok)) {
        if (tok == ERROR_TOK) {
            scanner_err(s, XKB_ERROR_MALFORMED_NUMBER_LITERAL,
                        "malformed number literal");
            return ERROR_TOK;
        }
        return tok;
    }

    scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                "unrecognized token");
    return ERROR_TOK;
}

XkbFile *
XkbParseString(struct xkb_context *ctx, const char *string, size_t len,
               const char *file_name, const char *map)
{
    struct scanner scanner;
    scanner_init(&scanner, ctx, string, len, file_name, NULL);

    /* Basic detection of wrong character encoding.
       The first character relevant to the grammar must be ASCII:
       whitespace, section, comment */
    if (!scanner_check_supported_char_encoding(&scanner)) {
        scanner_err(&scanner, XKB_ERROR_INVALID_FILE_ENCODING,
                    "This could be a file encoding issue. "
                    "Supported encodings must be backward compatible with ASCII.");
        scanner_err(&scanner, XKB_ERROR_INVALID_FILE_ENCODING,
                    "E.g. ISO/CEI 8859 and UTF-8 are supported "
                    "but UTF-16, UTF-32 and CP1026 are not.");
        return NULL;
    }

    return parse(ctx, &scanner, map);
}

XkbFile *
XkbParseFile(struct xkb_context *ctx, FILE *file,
             const char *file_name, const char *map)
{
    bool ok;
    XkbFile *xkb_file;
    char *string;
    size_t size;

    ok = map_file(file, &string, &size);
    if (!ok) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Couldn't read XKB file %s: %s\n",
                file_name, strerror(errno));
        return NULL;
    }

    xkb_file = XkbParseString(ctx, string, size, file_name, map);
    unmap_file(string, size);
    return xkb_file;
}
