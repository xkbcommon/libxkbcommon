#include <stdint.h>

struct text_line {
    const char *start;
    size_t length;
};

size_t
split_lines(const char *input, size_t input_length,
            struct text_line *output, size_t output_length);

size_t
shuffle_lines(struct text_line *lines, size_t length, char *output);
