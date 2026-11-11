/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "config.h"

#include <stddef.h>
#include <stdint.h>

#include "utils.h"

/**
 * Reads the next UTF-8 sequence in a string.
 *
 * @note Only well-formed sequences are accepted: no overlong encodings,
 * no surrogates and no code point beyond U+10FFFF.
 *
 * @note Never reads more than @p buf_size bytes.
 *
 * @param[in]  buf
 *   The buffer to read from.
 * @param[in]  buf_size
 *   Number of readable bytes in @p buf.
 * @param[out] cp
 *   The destination of the decoded code point.
 *
 * @return The outcome of the operation:
 * - **a negative value** if the input is ill-formed or truncated;
 * - **0** if @p buf_size is 0;
 * - **1 to 4** (the length in bytes of the sequence) otherwise.
 *
 * @post The decoded code point is written to @p cp if and only if the
 * return value is in 1..4.
 */
ssize_t
xkb_utf8_iterate(const char *buf, size_t buf_size, uint32_t *cp);
