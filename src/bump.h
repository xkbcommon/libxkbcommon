#pragma once

#include <stddef.h>
#include <stdalign.h>

struct bump_chunk {
    // Size of the chunk.
    size_t size;
    // Next free address in memory.
    char *ptr;
    // Pointer to the previous chunk.
    struct bump_chunk *prev;
    // Pointer to the allocated memory.
    alignas(max_align_t) char memory[0];
};

struct bump {
    struct bump_chunk *current;
};

void
bump_init(struct bump *bump);
void
bump_uninit(struct bump *bump);
void
*bump_alloc(struct bump *bump, size_t size);
char *
bump_strdup(struct bump *bump, const char *s);
