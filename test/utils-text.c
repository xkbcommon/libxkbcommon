/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include "src/darray.h"
#include "src/utils.h"
#include "test/utils-text.h"

/* For each line, drop substring starting from a given needle, then drop
 * the line if the rest are only whitespaces. The needle must not contain
 * "\n". */
char *
strip_lines(const char *input, size_t input_length, const char *prefix)
{
    darray_char buf = darray_new();
    const size_t prefix_len = strlen(prefix);
    const char *start = input;
    const char *end = input + input_length;

    const char *next = strstr(start, prefix);
    size_t count;
    while (start < end && next != NULL) {
        count = (size_t)(next - start);
        next = start + count + prefix_len;
        /* Find previous non-space */
        size_t i;
        for (i = count; i > 0; i--) {
            if (start[i - 1] != ' ' && start[i - 1] != '\t')
                break;
        }

        bool dropped = false;
        /* Drop line if only whitespaces */
        if (i == 0 || start[i - 1] == '\n') {
            count = i;
            dropped = true;
        }

        /* Append string */
        darray_append_items(buf, start, (darray_size_t) count);

        /* Find end of line */
        if (next >= end) {
            start = end;
            break;
        }
        start = strchr(next, 0x0a);
        if (start == NULL) {
            start = end;
            break;
        }

        if (dropped)
            start++;

        next = strstr(start, prefix);
    }

    /* Append remaining */
    if (start < end) {
        count = (size_t)(end - start);
        darray_append_items(buf, start, (darray_size_t) count);
    }

    darray_append(buf, '\0');
    return buf.item;
}

char *
uncomment(const char *input, size_t input_length, const char *prefix)
{
    darray_char buf = darray_new();
    const size_t prefix_len = strlen(prefix);
    const char *start = input;
    const char *end = input + input_length;

    char *next = strstr(start, prefix);
    size_t count;
    while (start < end && next != NULL) {
        count = (size_t)(next - start);
        darray_append_items(buf, start, (darray_size_t) count);

        /* Skip prefix */
        start += count + prefix_len;

        /* Find end of line */
        if (start >= end)
            break;
        next = strchr(start, 0x0a);
        if (next == NULL)
            break;
        next = strstr(next, prefix);
    }

    /* Append remaining */
    if (start < end) {
        count = (size_t)(end - start);
        darray_append_items(buf, start, (darray_size_t) count);
    }

    darray_append(buf, '\0');
    return buf.item;
}

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
concat_lines(struct text_line *lines, size_t length,
             const char *sep, char *output)
{
    char *out = output;
    size_t sep_len = strlen(sep);
    for (size_t i = 0; i < length; i++) {
        if (i > 0) {
            memcpy(out, sep, sep_len);
            out += sep_len;
        }
        memcpy(out, lines[i].start, lines[i].length);
        out += lines[i].length;
    }
    *out = '\0';
    return (size_t)(out - output);
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
            size_t j = rand() % (i+1);
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
