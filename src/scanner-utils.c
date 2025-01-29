/*
 * Copyright © 2025 Ran Benita <ran@unusedvar.com>
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

#include "scanner-utils.h"

struct scanner_loc
scanner_token_location(struct scanner *s)
{
    /*
     * The following article and accompying code compares some algorithms:
     * https://lemire.me/blog/2017/02/14/how-fast-can-you-count-lines/
     * https://github.com/lemire/Code-used-on-Daniel-Lemire-s-blog/blob/master/2017/02/14/newlines.c
     * This is the fastest portable one for me, and it's simple.
     *
     * To avoid rescanning on each call, cache the previously found position
     * and start from that on the next call. This is effective as long as the
     * tokens go forward.
     */
    size_t line, column;
    size_t line_pos = 0;
    if (s->cached_pos > s->token_pos) {
        s->cached_pos = 0;
        s->cached_loc.line = s->cached_loc.column = 1;
    }
    line = s->cached_loc.line;
    const char *ptr = s->s + s->cached_pos;
    const char *last = s->s + s->token_pos;
    while ((ptr = memchr(ptr, '\n', last - ptr))) {
        line++;
        ptr++;
        line_pos = ptr - s->s;
    }
    if (line == s->cached_loc.line) {
        column = s->cached_loc.column + (s->token_pos - s->cached_pos);
    } else {
        column = s->token_pos - line_pos + 1;
    }
    struct scanner_loc loc = {.line = line, .column = column};
    s->cached_pos = s->token_pos;
    s->cached_loc = loc;
    return loc;
}
