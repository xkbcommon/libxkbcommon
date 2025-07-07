/*
 * Copyright © 2020 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "config.h"

#include <stdlib.h>

static inline void*
_steal(void *ptr) {
    void **original = (void**)ptr;
    void *swapped = *original;
    *original = NULL;
    return swapped;
}

/**
 * Resets the pointer content and resets the data to NULL.
 */
#ifdef _WIN32
#define steal(ptr_) \
    _steal(ptr_)
#else
#define steal(ptr_) \
    (__typeof__(*(ptr_)))_steal(ptr_)
#endif
