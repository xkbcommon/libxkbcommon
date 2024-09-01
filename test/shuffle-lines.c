/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include "src/utils.h"
#include "test/shuffle-lines.h"

/* Split string into lines */
size_t
split_lines(const char *input, size_t input_length,
            struct text_line *output, size_t output_length)
{
    const char *start = input;
    char *next;
    size_t l;
    size_t i = 0;

    for (l = 0; i < input_length && l < output_length && *start != '\0'; l++) {
        /* Look for newline character */
        next = strchr(start, 0x0a);
        output[l].start = start;
        if (next == NULL) {
            /* Not found: add the rest of the string */
            output[l++].length = strlen(start);
            break;
        }
        output[l].length = (size_t)(next - start) + 1;
        start = next + 1;
        i += output[l].length;
    }
    return l;
}

size_t
shuffle_lines(struct text_line *lines, size_t length, char *output)
{
    /* Shuffle lines in-place using Fisher–Yates algorithm.
     * See: https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle */

    assert(length < RAND_MAX);
    char *out = output;
    if (length > 1) {
        /* 1. Set the current i to the last line.
         * 2. Take a random line j before the current line i.
         * 3. Swap the lines i and j.
         * 4. Append line i to the output.
         * 5. If i is the first line, stop. Else decrease i and go to 2).
         */
        for (size_t i = length - 1; i > 0; i--) {
            /* Swap current line with random line before it */
            size_t j = (size_t)(rand() % (i+1));
            struct text_line tmp = lines[j];
            lines[j] = lines[i];
            lines[i] = tmp;
            /* Append current line */
            memcpy(out, lines[i].start, lines[i].length);
            out += lines[i].length;
            /* Ensure line ends with newline */
            if (out[-1] != '\n') {
                out[0] = '\n';
                out++;
            }
        }
    }
    return (size_t)(out - output);
}
