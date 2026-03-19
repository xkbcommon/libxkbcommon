/*
 * Copyright © 2020 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "config.h"

#include <stdlib.h>
#include <stdbool.h>

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

/**
 * Check the size field of a versioned struct for forward-compatibility.
 *
 * @param lib_size     sizeof() of the struct as known by the library.
 * @param caller_size  The size field provided by the caller.
 * @param caller_data  Pointer to the start of the struct.
 *
 * @returns true if the struct is valid and safe to use, false otherwise.
 */
static inline bool
__xkb_check_struct_size(size_t lib_size, size_t caller_size,
                      const void *caller_data)
{
    if (caller_size == 0)
        return false;

    if (caller_size <= lib_size)
        /* Caller has older or same struct — safe, missing fields default to 0 */
        return true;

    /* Caller has newer struct — only safe if unknown trailing bytes are zero */
    const unsigned char *p = (const unsigned char *)caller_data + lib_size;
    const unsigned char *end = (const unsigned char *)caller_data + caller_size;

    while (p < end) {
        if (*(p++) != 0)
            return false;
    }

    return true;
}

#define xkb_check_struct_size(x) \
    __xkb_check_struct_size(sizeof(*(x)), (x)->size, x)
