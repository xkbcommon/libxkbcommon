#include "config.h"

#include <assert.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#include "bump.h"

 // Size of initial chunk.
#define INITIAL_CHUNK_SIZE 4096
// Factor by which to grow chunk sizes.
#define GROWTH_FACTOR 2

static struct bump_chunk dummy_chunk = {
    .size = 0,
    .ptr = 0,
    .prev = NULL,
};

void
bump_init(struct bump *bump)
{
    // Create the first chunk
    bump->current = &dummy_chunk;
}

void *
bump_alloc(struct bump *bump, size_t size)
{
    assert(size <= INITIAL_CHUNK_SIZE);

    /* Align size to the max alignemnt. */
    const size_t max_alignment = alignof(max_align_t);
    size = (size + max_alignment - 1) & ~(max_alignment - 1);

    struct bump_chunk *chunk = bump->current;

    /* Check if there is enough space in the current chunk. */
    if ((size_t) (chunk->ptr - chunk->memory) + size > chunk->size) {
        /* Not enough space, create a new chunk. */
        size_t new_chunk_size = chunk->size == 0 ? INITIAL_CHUNK_SIZE : chunk->size * GROWTH_FACTOR;
        struct bump_chunk *new_chunk = aligned_alloc(max_alignment, sizeof(*new_chunk) + new_chunk_size);
        if (!new_chunk) {
            return NULL;
        }
        new_chunk->size = new_chunk_size;
        new_chunk->ptr = new_chunk->memory;
        new_chunk->prev = chunk;

        bump->current = new_chunk;
        chunk = new_chunk;
    }

    void *ptr = chunk->ptr;
    chunk->ptr += size;
    return ptr;
}

char *
bump_strdup(struct bump *bump, const char *s)
{
    size_t len = strlen(s) + 1;
    char *new = bump_alloc(bump, len);
    if (!new) {
        return NULL;
    }
    memcpy(new, s, len);
    return new;
}

void
bump_uninit(struct bump *bump)
{
    struct bump_chunk *chunk = bump->current;
    while (chunk != &dummy_chunk) {
        struct bump_chunk *prev = chunk->prev;
        free(chunk);
        chunk = prev;
    }
}
