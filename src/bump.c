#include "config.h"

#include <assert.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bump.h"
#include "utils.h"

/* Size of initial chunk. */
const size_t INITIAL_CHUNK_SIZE = 4096;
/* Factor by which to grow chunk sizes. */
const size_t GROWTH_FACTOR = 2;

static struct bump_chunk dummy_chunk = {
    .end = dummy_chunk.memory,
    .ptr = dummy_chunk.memory,
    .prev = NULL,
};

void
bump_init(struct bump *bump)
{
    // Create the first chunk
    bump->current = &dummy_chunk;
}

static inline char *
align_up(char *ptr, size_t alignment)
{
    uintptr_t addr = (uintptr_t) ptr;
    uintptr_t aligned_addr = ((addr + alignment - 1) & ~(alignment - 1));
    /*
     * Roundabout way to avoid messing the pointer provenance.
     * The compiler optimizes it away.
     */
    return ptr + (aligned_addr - addr);
}

ATTR_NOINLINE ATTR_MALLOC ATTR_ALLOC_ALIGN(2) static void *
bump_aligned_alloc_slow(struct bump *bump, size_t alignment, size_t size)
{
    struct bump_chunk *prev_chunk = bump->current;
    size_t new_chunk_size;
    if (prev_chunk == &dummy_chunk) {
        new_chunk_size = INITIAL_CHUNK_SIZE;
    } else {
        new_chunk_size = (prev_chunk->end - prev_chunk->memory) * GROWTH_FACTOR;
    }
    struct bump_chunk *new_chunk = malloc(sizeof(*new_chunk) + new_chunk_size);
    if (!new_chunk) {
        return NULL;
    }
    new_chunk->end = new_chunk->memory + new_chunk_size;
    new_chunk->prev = prev_chunk;
    char *ptr = align_up(new_chunk->memory, alignment);
    if ((uintptr_t) ptr + size >= (uintptr_t) new_chunk->end) {
        free(new_chunk);
        return NULL;
    }
    new_chunk->ptr = ptr + size;
    bump->current = new_chunk;
    return ptr;
}

ATTR_MALLOC ATTR_ALLOC_ALIGN(2) void *
bump_aligned_alloc(struct bump *bump, size_t alignment, size_t size)
{
    struct bump_chunk *chunk = bump->current;
    char *ptr = align_up(chunk->ptr, alignment);
    if (unlikely((uintptr_t) ptr + size >= (uintptr_t) chunk->end)) {
        /* Not enough space, slow path allocating new chunk. */
        return bump_aligned_alloc_slow(bump, alignment, size);
    }
    chunk->ptr = ptr + size;
    return ptr;
}

char *
bump_strdup(struct bump *bump, const char *s)
{
    size_t len = strlen(s) + 1;
    char *new = bump_aligned_alloc(bump, 1, len);
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
