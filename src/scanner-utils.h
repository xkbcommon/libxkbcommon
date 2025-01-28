/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef XKBCOMP_SCANNER_UTILS_H
#define XKBCOMP_SCANNER_UTILS_H

#include "config.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "context.h"
#include "darray.h"
#include "messages-codes.h"
#include "utils.h"

/* Point to some substring in the file; used to avoid copying. */
struct sval {
    const char *start;
    unsigned int len;
};
typedef darray(struct sval) darray_sval;

static inline bool
svaleq(struct sval s1, struct sval s2)
{
    return s1.len == s2.len && memcmp(s1.start, s2.start, s1.len) == 0;
}

static inline bool
svaleq_prefix(struct sval s1, struct sval s2)
{
    return s1.len <= s2.len && memcmp(s1.start, s2.start, s1.len) == 0;
}

/* A line:column location in the input string (1-based). */
struct scanner_loc {
    size_t line, column;
};

struct scanner {
    const char *s;
    size_t pos;
    size_t len;
    char buf[1024];
    size_t buf_pos;
    /* The position of the start of the current token. */
    size_t token_pos;
    const char *file_name;
    struct xkb_context *ctx;
    void *priv;
};

/* Compute the line:column location for the current token (slow). */
struct scanner_loc
scanner_token_location(struct scanner *s);

#define scanner_log_with_code(scanner, level, verbosity, log_msg_id, fmt, ...) do { \
    struct scanner_loc loc = scanner_token_location((scanner));                     \
    xkb_log_with_code((scanner)->ctx, (level), verbosity, log_msg_id,               \
                      "%s:%zu:%zu: " fmt "\n",                                      \
                      (scanner)->file_name,                                         \
                      loc.line,                                                     \
                      loc.column, ##__VA_ARGS__);                                   \
} while(0)

#define scanner_err(scanner, id, fmt, ...)                     \
    scanner_log_with_code(scanner, XKB_LOG_LEVEL_ERROR, 0, id, \
                          fmt, ##__VA_ARGS__)

#define scanner_warn(scanner, id, fmt, ...)                      \
    scanner_log_with_code(scanner, XKB_LOG_LEVEL_WARNING, 0, id, \
                          fmt, ##__VA_ARGS__)

#define scanner_vrb(scanner, verbosity, id, fmt, ...)                    \
    scanner_log_with_code(scanner, XKB_LOG_LEVEL_WARNING, verbosity, id, \
                          fmt, ##__VA_ARGS__)

static inline void
scanner_init(struct scanner *s, struct xkb_context *ctx,
             const char *string, size_t len, const char *file_name,
             void *priv)
{
    s->s = string;
    s->len = len;
    s->pos = 0;
    s->token_pos = 0;
    s->file_name = file_name;
    s->ctx = ctx;
    s->priv = priv;
}

static inline char
scanner_peek(struct scanner *s)
{
    if (unlikely(s->pos >= s->len))
        return '\0';
    return s->s[s->pos];
}

static inline bool
scanner_eof(struct scanner *s)
{
    return s->pos >= s->len;
}

static inline bool
scanner_eol(struct scanner *s)
{
    return scanner_peek(s) == '\n';
}

static inline void
scanner_skip_to_eol(struct scanner *s)
{
    const char *nl = memchr(s->s + s->pos, '\n', s->len - s->pos);
    const size_t new_pos = nl ? (size_t) (nl - s->s) : s->len;
    s->pos = new_pos;
}

static inline char
scanner_next(struct scanner *s)
{
    if (unlikely(scanner_eof(s)))
        return '\0';
    return s->s[s->pos++];
}

static inline bool
scanner_chr(struct scanner *s, char ch)
{
    if (likely(scanner_peek(s) != ch))
        return false;
    s->pos++;
    return true;
}

static inline bool
scanner_str(struct scanner *s, const char *string, size_t len)
{
    if (s->len - s->pos < len)
        return false;
    if (memcmp(s->s + s->pos, string, len) != 0)
        return false;
    s->pos += len;
    return true;
}

#define scanner_lit(s, literal) scanner_str(s, literal, sizeof(literal) - 1)

static inline bool
scanner_buf_append(struct scanner *s, char ch)
{
    if (s->buf_pos + 1 >= sizeof(s->buf))
        return false;
    s->buf[s->buf_pos++] = ch;
    return true;
}

static inline bool
scanner_buf_appends(struct scanner *s, const char *str)
{
    int ret;
    ret = snprintf(s->buf + s->buf_pos, sizeof(s->buf) - s->buf_pos, "%s", str);
    if (ret < 0 || (size_t) ret >= sizeof(s->buf) - s->buf_pos)
        return false;
    s->buf_pos += ret;
    return true;
}

static inline bool
scanner_oct(struct scanner *s, uint8_t *out)
{
    int i;
    for (i = 0, *out = 0; scanner_peek(s) >= '0' && scanner_peek(s) <= '7' && i < 3; i++)
        /* Test overflow */
        if (*out < 040) {
            *out = *out * 8 + scanner_next(s) - '0';
        } else {
            /* Consume valid digit, but mark result as invalid */
            scanner_next(s);
            return false;
        }
    return i > 0;
}

static inline bool
scanner_hex(struct scanner *s, uint8_t *out)
{
    int i;
    for (i = 0, *out = 0; is_xdigit(scanner_peek(s)) && i < 2; i++) {
        const char c = scanner_next(s);
        const char offset = (c >= '0' && c <= '9' ? '0' :
                             c >= 'a' && c <= 'f' ? 'a' - 10 : 'A' - 10);
        *out = *out * 16 + c - offset;
    }
    return i > 0;
}

/* Basic detection of wrong character encoding based on the first bytes */
static inline bool
scanner_check_supported_char_encoding(struct scanner *scanner)
{
    /* Skip UTF-8 encoded BOM (U+FEFF)
     * See: https://www.unicode.org/faq/utf_bom.html#bom5 */
    if (scanner_str(scanner, "\xef\xbb\xbf", 3) || scanner->len < 2) {
        /* Assume UTF-8 encoding or trivial short input */
        return true;
    }

    /* Early detection of wrong file encoding, e.g. UTF-16 or UTF-32 */
    if (scanner->s[0] == '\0' || scanner->s[1] == '\0') {
        scanner_err(scanner, XKB_LOG_MESSAGE_NO_ID,
                    "unexpected NULL character.");
        return false;
    }
    /* Enforce the first character to be ASCII.
       See the note before the use of this function, that explains the relevant
       parts of the grammars of rules, keymap components and Compose. */
    if (!is_ascii(scanner->s[0])) {
        scanner_err(scanner, XKB_LOG_MESSAGE_NO_ID,
                    "unexpected non-ASCII character.");
        return false;
    }

    return true;
}

#endif
