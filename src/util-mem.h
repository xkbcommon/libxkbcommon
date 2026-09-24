/*
 * Copyright © 2020 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "config.h"

#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#if !HAVE_MAX_ALIGN_T
/* Fallback for legacy compilers or older C modes on MSVC */
typedef struct {
    long long __max_align_ll;
    long double __max_align_ld;
} max_align_t;
#endif

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

#define XKB_POINTER_TAG_BITS  2
#define XKB_POINTER_TAG_MASK  ((uintptr_t)((1u << XKB_POINTER_TAG_BITS) - 1u))

static_assert(alignof(max_align_t) >= 4,
              "Pointer alignment insufficient for tagging");

/** Pack @p ptr and a 0-3 @p tag. */
static inline const void *
xkb_pointer_tag(const void *ptr, uintptr_t tag)
{
    uintptr_t addr = (uintptr_t)ptr;
    assert(tag <= XKB_POINTER_TAG_MASK &&
           "tag exceeds available bits");
    assert((addr & XKB_POINTER_TAG_MASK) == 0 &&
           "pointer not sufficiently aligned for tagging");
    /* NOLINTNEXTLINE(performance-no-int-to-ptr) */
    return (const void *)(addr | tag);
}

/** Recover the original pointer from a tagged pointer. */
static inline const void *
xkb_pointer_untag(const void *ptr)
{
    /* NOLINTNEXTLINE(performance-no-int-to-ptr) */
    return (const void *)((uintptr_t)ptr & ~XKB_POINTER_TAG_MASK);
}

/* Recover just the tag bits from a tagged pointer. */
static inline uintptr_t
xkb_pointer_get_tag(const void *ptr)
{
    return ((uintptr_t)ptr & XKB_POINTER_TAG_MASK);
}
