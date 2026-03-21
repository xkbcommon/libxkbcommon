/*
 * Copyright © 2026 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "config.h"

#include <assert.h>

#include "xkbcommon/xkbcommon.h"

enum {
    /** Use a reasonable upper size limit to avoid malicious caller values */
    XKB_ABI_MAX_SIZE = 4096,
};

/**
 * Check the size field of a versioned struct for forward-compatibility.
 *
 * @param v1_size      Size of the first version of the struct. A caller_size
 *                     below this indicates a buggy or uninitialized size field.
 * @param min_size     Size of the oldest version still acceptable to this call.
 *                     A caller_size below this triggers backward-compat error.
 * @param callee_meaningful_bytes
 *                     Number of leading bytes, as known by the callee, that
 *                     hold meaningful/known data. Bytes from here up to
 *                     `caller_size` must be zero.
 *                     Use `offsetof(struct, reserved)` if there is `reserved`
 *                     field, otherwise `sizeof(struct)`.
 * @param callee_size  The size field provided by the callee.
 * @param caller_size  The size field provided by the caller.
 * @param caller_data  Pointer to the start of the struct.
 *
 * @returns `::XKB_SUCCESS` if the struct is valid and safe to use, otherwise
 * an error code indicating the kind of ABI violation.
 */
static inline enum xkb_error_code
xkb_check_versioned_struct_size_(size_t v1_size, size_t min_size,
                                 size_t callee_meaningful_bytes,
                                 size_t callee_size, size_t caller_size,
                                 const void *caller_data)
{
    static const size_t max_size = (size_t)XKB_ABI_MAX_SIZE;

    /* Check internal misuse */
    assert(v1_size <= min_size);
    assert(min_size <= callee_size);
    assert(callee_meaningful_bytes <= callee_size);
    assert(callee_size <= max_size);

    if (caller_size < v1_size || caller_size > max_size || !caller_data)
        return XKB_ERROR_ABI_INVALID_STRUCT_SIZE;

    if (caller_size < min_size)
        return XKB_ERROR_ABI_BACKWARD_COMPAT;

    if (caller_size <= callee_meaningful_bytes) {
        /*
         * Caller's data does not reach beyond the callee meaningful region:
         * nothing to check, missing bytes default to 0.
         *
         * E.g.: caller has older or same callee struct, without explicit
         * trailing padding.
         */
        return XKB_SUCCESS;
    }

    /*
     * Reserved and/or unknown trailing bytes must be zero, whether they
     * fall inside the callee struct size or beyond it.
     *
     * E.g.: callee struct with explicit trailing padding, new struct using
     * callee reserved field, caller newer struct with greater size.
     */
    const unsigned char *p =
        (const unsigned char *)caller_data + callee_meaningful_bytes;
    const unsigned char *end = (const unsigned char *)caller_data + caller_size;

    while (p < end) {
        if (*(p++) != 0)
            return XKB_ERROR_ABI_FORWARD_COMPAT;
    }

    return XKB_SUCCESS;
}

#define xkb_check_versioned_struct_size(v1_size, min_size, \
                                        callee_meaningful_bytes, x) \
    xkb_check_versioned_struct_size_(v1_size, min_size, \
                                     callee_meaningful_bytes, \
                                     sizeof(*(x)), (x)->size, x)

void
xkb_log_abi_error(struct xkb_context * restrict ctx,
                  const char * restrict func, enum xkb_error_code error);
