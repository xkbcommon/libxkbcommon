/*
 * Copyright (C) 2011 Joseph Adams <joeyadams3.14159@gmail.com>
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "config.h"

/* Originally taken from: https://ccodearchive.net/info/darray.html
 * But modified for libxkbcommon. */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

typedef unsigned int darray_size_t;
#define DARRAY_SIZE_T_WIDTH (sizeof(darray_size_t) * CHAR_BIT)
#define DARRAY_SIZE_MAX UINT_MAX

#define darray(type) struct {       \
    /** Array of items */           \
    type *item;                     \
    /** Current size */             \
    darray_size_t size;             \
    /** Count of allocated items */ \
    darray_size_t alloc;            \
}

#define darray_new() { 0, 0, 0 }

#define darray_init(arr) do { \
    (arr).item = 0; (arr).size = 0; (arr).alloc = 0; \
} while (0)

#define darray_free(arr) do { \
    free((arr).item); \
    darray_init(arr); \
} while (0)

#define darray_steal(arr, to, to_size) do { \
    *(to) = (arr).item; \
    if (to_size) \
        *(darray_size_t *) (to_size) = (arr).size; \
    darray_init(arr); \
} while (0)

/*
 * Typedefs for darrays of common types.  These are useful
 * when you want to pass a pointer to an darray(T) around.
 *
 * The following will produce an incompatible pointer warning:
 *
 *     void foo(darray(int) *arr);
 *     darray(int) arr = darray_new();
 *     foo(&arr);
 *
 * The workaround:
 *
 *     void foo(darray_int *arr);
 *     darray_int arr = darray_new();
 *     foo(&arr);
 */

typedef darray (char)           darray_char;
typedef darray (signed char)    darray_schar;
typedef darray (unsigned char)  darray_uchar;

typedef darray (char *)         darray_string;

typedef darray (short)          darray_short;
typedef darray (int)            darray_int;
typedef darray (long)           darray_long;

typedef darray (unsigned short) darray_ushort;
typedef darray (unsigned int)   darray_uint;
typedef darray (unsigned long)  darray_ulong;

/*** Access ***/

#define darray_item(arr, i)     ((arr).item[i])
#define darray_items(arr)       ((arr).item)
#define darray_size(arr)        ((arr).size)
#define darray_empty(arr)       ((arr).size == 0)

/*** Insertion (single item) ***/

#define darray_append(arr, ...)  do { \
    darray_resize(arr, (arr).size + 1); \
    (arr).item[(arr).size - 1] = (__VA_ARGS__); \
} while (0)

#define darray_insert(arr, i, ...) do { \
    darray_size_t __index = (i); \
    darray_resize(arr, (arr).size+1); \
    memmove( \
        (arr).item + __index + 1, \
        (arr).item + __index, \
        ((arr).size - __index - 1) * sizeof(*(arr).item) \
    ); \
    (arr).item[__index] = (__VA_ARGS__); \
} while(0)

/*** Insertion (multiple items) ***/

#define darray_append_items(arr, items, count) do { \
    darray_size_t __count = (count), __oldSize = (arr).size; \
    darray_resize(arr, __oldSize + __count); \
    memcpy((arr).item + __oldSize, items, __count * sizeof(*(arr).item)); \
} while (0)

#define darray_from_items(arr, items, count) do { \
    darray_size_t __count = (count); \
    darray_resize(arr, __count); \
    if (__count != 0) \
        memcpy((arr).item, items, __count * sizeof(*(arr).item)); \
} while (0)

#define darray_copy(arr_to, arr_from) \
    darray_from_items((arr_to), (arr_from).item, (arr_from).size)

#define darray_concat(arr_to, arr_from) do { \
    if ((arr_from).size > 0) \
        darray_append_items((arr_to), (arr_from).item, (arr_from).size); \
} while (0)

/*** Removal ***/

/* Warning: Do not call darray_remove_last on an empty darray. */
#define darray_remove_last(arr) (--(arr).size)

/* Warning, slow: Requires copying all elements after removed item. */
#define darray_remove(arr, i) do { \
    darray_size_t __index = (i); \
    if (__index < (arr).size-1) \
        memmove( \
            &(arr).item[__index], \
            &(arr).item[__index + 1], \
            ((arr).size -1 - __index) * sizeof(*(arr).item) \
        ); \
    (arr).size--; \
} while(0)

/*** String buffer ***/

#define darray_append_string(arr, str) do { \
    const char *__str = (str); \
    darray_append_items(arr, __str, (darray_size_t) strlen(__str) + 1); \
    (arr).size--; \
} while (0)

/* Same as `darray_append_string` but do count the final '\0' in the size */
#define darray_append_string0(arr, string) \
    darray_append_items((arr), (string), strlen(string) + 1)

#define darray_append_lit(arr, stringLiteral) do { \
    darray_append_items(arr, stringLiteral, \
                        (darray_size_t) sizeof(stringLiteral)); \
    (arr).size--; \
} while (0)

#define darray_appends_nullterminate(arr, items, count) do { \
    darray_size_t __count = (count), __oldSize = (arr).size; \
    darray_resize(arr, __oldSize + __count + 1); \
    memcpy((arr).item + __oldSize, items, __count * sizeof(*(arr).item)); \
    (arr).item[--(arr).size] = 0; \
} while (0)

#define darray_prepends_nullterminate(arr, items, count) do { \
    darray_size_t __count = (count), __oldSize = (arr).size; \
    darray_resize(arr, __count + __oldSize + 1); \
    memmove((arr).item + __count, (arr).item, \
            __oldSize * sizeof(*(arr).item)); \
    memcpy((arr).item, items, __count * sizeof(*(arr).item)); \
    (arr).item[--(arr).size] = 0; \
} while (0)

/*** Size management ***/

#define darray_resize(arr, newSize) \
    darray_growalloc(arr, (arr).size = (newSize))

#define darray_resize0(arr, newSize) do { \
    darray_size_t __oldSize = (arr).size, __newSize = (newSize); \
    (arr).size = __newSize; \
    if (__newSize > __oldSize) { \
        darray_growalloc(arr, __newSize); \
        memset(&(arr).item[__oldSize], 0, \
               (__newSize - __oldSize) * sizeof(*(arr).item)); \
    } \
} while (0)

#define darray_realloc(arr, newAlloc) do { \
    (arr).item = realloc((arr).item, \
                         ((arr).alloc = (newAlloc)) * sizeof(*(arr).item)); \
} while (0)

#define darray_growalloc(arr, need)   do { \
    darray_size_t __need = (need); \
    if (__need > (arr).alloc) \
        darray_realloc(arr, darray_next_alloc((arr).alloc, __need, \
                                              sizeof(*(arr).item))); \
} while (0)

#define darray_shrink(arr) do { \
    if ((arr).size > 0) \
        (arr).item = realloc((arr).item, \
                             ((arr).alloc = (arr).size) * sizeof(*(arr).item)); \
} while (0)

#define darray_max_alloc(itemSize) (UINT_MAX / (itemSize))

static inline darray_size_t
darray_next_alloc(darray_size_t alloc, darray_size_t need, size_t itemSize)
{
    assert(need < darray_max_alloc(itemSize) / 2); /* Overflow. */
    if (alloc == 0)
        alloc = 4;
    while (alloc < need)
        alloc *= 2;
    return alloc;
}

/*** Traversal ***/

#define darray_foreach(i, arr) \
    if ((arr).item) \
    for ((i) = &(arr).item[0]; (i) < &(arr).item[(arr).size]; (i)++)

#define darray_foreach_from(i, arr, from) \
    if ((arr).item) \
    for ((i) = &(arr).item[from]; (i) < &(arr).item[(arr).size]; (i)++)

/* Iterate on index and value at the same time, like Python's enumerate. */
#define darray_enumerate(idx, val, arr) \
    if ((arr).item) \
    for ((idx) = 0, (val) = &(arr).item[0]; \
         (idx) < (arr).size; \
         (idx)++, (val)++)

#define darray_enumerate_from(idx, val, arr, from) \
    if ((arr).item) \
    for ((idx) = (from), (val) = &(arr).item[from]; \
         (idx) < (arr).size; \
         (idx)++, (val)++)

#define darray_foreach_reverse(i, arr) \
    if ((arr).item) \
    for ((i) = &(arr).item[(arr).size - 1]; (arr).size > 0 && (i) >= &(arr).item[0]; (i)--)
