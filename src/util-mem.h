/*
 * Copyright © 2020 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include "xkbcommon/xkbcommon-errors.h"

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
 * @param v1_size      Size of the first version of the struct. A caller_size
 *                     below this indicates a buggy or uninitialized size field.
 * @param min_size     Size of the oldest version still acceptable to this call.
 *                     A caller_size below this triggers backward-compat error.
 * @param lib_size     sizeof() of the struct as known by the library.
 * @param caller_size  The size field provided by the caller.
 * @param caller_data  Pointer to the start of the struct.
 *
 * @returns `::XKB_SUCCESS` if the struct is valid and safe to use, otherwise
 * an error code indicating the kind of ABI violation.
 */
static inline enum xkb_error_code
xkb_check_versioned_struct_size_(size_t v1_size, size_t min_size,
                                 size_t lib_size, size_t caller_size,
                                 const void *caller_data)
{
    assert(v1_size <= min_size);
    assert(min_size <= lib_size);

    if (caller_size < v1_size)
        return XKB_ERROR_ABI_INVALID_STRUCT_SIZE;

    if (caller_size < min_size)
        return XKB_ERROR_ABI_BACKWARD_COMPAT;

    if (caller_size <= lib_size)
        /* Caller has older or same struct — safe, missing fields default to 0 */
        return XKB_SUCCESS;

    /* Caller has newer struct — only safe if unknown trailing bytes are zero */
    const unsigned char *p = (const unsigned char *)caller_data + lib_size;
    const unsigned char *end = (const unsigned char *)caller_data + caller_size;

    while (p < end) {
        if (*(p++) != 0)
            return XKB_ERROR_ABI_FORWARD_COMPAT;
    }

    return XKB_SUCCESS;
}

#define xkb_check_versioned_struct_size(v1_size, min_size, x) \
    xkb_check_versioned_struct_size_(v1_size, min_size, sizeof(*(x)), (x)->size, x)
