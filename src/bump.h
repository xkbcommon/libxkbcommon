#pragma once

#include "config.h"

#include <stdalign.h>
#include <stddef.h>

struct bump_chunk {
    /* One past the end of memory. */
    char *end;
    /* Next free address in memory. */
    char *ptr;
    /* Pointer to the previous chunk. */
    struct bump_chunk *prev;
    /* Pointer to the allocated memory. */
    char memory[0];
};

struct bump {
    struct bump_chunk *current;
};

void
bump_init(struct bump *bump);
void
bump_uninit(struct bump *bump);
void *
bump_aligned_alloc(struct bump *bump, size_t alignment, size_t size);
char *
bump_strdup(struct bump *bump, const char *s);

#define bump_alloc(bump, of) bump_aligned_alloc((bump), alignof(__typeof__(of)), sizeof(of))
