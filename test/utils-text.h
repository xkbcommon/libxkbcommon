/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

char *
strip_lines(const char *input, size_t input_length, const char *prefix);

char *
uncomment(const char *input, size_t input_length, const char *prefix);

struct text_line {
    const char *start;
    size_t length;
};

size_t
split_lines(const char *input, size_t input_length,
            struct text_line *output, size_t output_length);

size_t
concat_lines(struct text_line *lines, size_t length,
             const char *sep, char *output);

size_t
shuffle_lines(struct text_line *lines, size_t length, char *output);
