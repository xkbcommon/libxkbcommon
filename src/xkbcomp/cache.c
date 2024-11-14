/*
 * Copyright © 2024 Pierre Le Marre <dev@wismill.eu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <stdlib.h>
#include <pthread.h>

#include "cache.h"
#include "xkbcomp-priv.h"

static pthread_mutex_t keymap_cache_mutext = PTHREAD_MUTEX_INITIALIZER;

struct xkb_keymap_cache*
xkb_keymap_cache_new(void)
{
    struct xkb_keymap_cache* cache = calloc(1, sizeof(struct xkb_keymap_cache));
    if (!cache)
        return NULL;
    if (!hcreate_r(XKB_KEYMAP_CACHE_SIZE, &cache->index))
        return NULL;
    darray_init(cache->data);
    return cache;
}

void
xkb_keymap_cache_free(struct xkb_keymap_cache *cache)
{
    if (!cache)
        return;
    hdestroy_r(&cache->index);
    struct xkb_keymap_cache_entry* entry;
    darray_foreach(entry, cache->data) {
        free(entry->key);
        FreeXkbFile(entry->data);
    }
    darray_free(cache->data);
    free(cache);
}

bool
xkb_keymap_cache_add(struct xkb_keymap_cache *cache, char *key, XkbFile *data)
{
    struct xkb_keymap_cache_entry entry = {
        .key = strdup(key),
        .data = DupXkbFile(data)
    };
    if (!entry.key || !entry.data)
        goto error;
    size_t idx = darray_size(cache->data);
    pthread_mutex_lock(&keymap_cache_mutext);
    darray_append(cache->data, entry);
    if (darray_size(cache->data) == idx) {
        pthread_mutex_unlock(&keymap_cache_mutext);
        goto error;
    }
    ENTRY input = {
        .key = entry.key,
        .data = entry.data
    };
    ENTRY *output;
    bool ret = !hsearch_r(input, ENTER, &output, &cache->index);
    pthread_mutex_unlock(&keymap_cache_mutext);
    return ret;
error:
    if (entry.key)
        free(entry.key);
    if (entry.data)
        FreeXkbFile(entry.data);
    return false;
}

XkbFile*
xkb_keymap_cache_search(struct xkb_keymap_cache *cache, char *key)
{
    ENTRY input = {
        .key = key,
        .data = NULL
    };
    ENTRY *output;
    if (!hsearch_r(input, FIND, &output, &cache->index))
        return NULL;
    return DupXkbFile(output->data);
}
