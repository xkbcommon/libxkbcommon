/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2014 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 */

#ifndef XKBCOMMON_UTF8_H
#define XKBCOMMON_UTF8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int
utf32_to_utf8(uint32_t unichar, char *buffer);

bool
is_valid_utf8(const char *ss, size_t len);

#endif
