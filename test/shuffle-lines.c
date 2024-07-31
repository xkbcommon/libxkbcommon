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
    /* Shuffle in-place using Fisher–Yates algorithm, then append lines.
     * See: https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle */

    assert(length < RAND_MAX);
    char *out = output;
    if (length > 1) {
        for (size_t i = length - 1; i > 0; i--) {
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
