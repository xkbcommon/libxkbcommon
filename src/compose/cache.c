/*
 * Copyright © 2021 Emmanuel Gil Peyrot <linkmauve@linkmauve.fr>
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

#if !HAVE_XXHASH
#error "libxxhash is required for the compose cache"
#endif

#include <string.h>
#include <sys/stat.h>
#define XXH_INLINE_ALL
#include <xxhash.h>

#include "table.h"
#include "cache.h"

static bool
hash_string(const char *string, size_t len, XXH128_canonical_t *out)
{
    XXH3_state_t *state;
    XXH_errorcode err;
    XXH128_hash_t hash;

    state = XXH3_createState();
    if (!state)
        return false;

    err = XXH3_128bits_reset(state);
    if (err != XXH_OK) {
        XXH3_freeState(state);
        return false;
    }

    err = XXH3_128bits_update(state, string, len);
    if (err != XXH_OK) {
        XXH3_freeState(state);
        return false;
    }

    hash = XXH3_128bits_digest(state);
    XXH3_freeState(state);
    XXH128_canonicalFromHash(out, hash);
    return true;
}

bool
cache_get_path_from_string(const char *string, size_t len, char **cache_path)
{
    const char *home, *xdg;
    char *path = NULL;
    char *hash_part;
    XXH128_canonical_t hash;

    /* Mostly copied over from xkb_context_include_path_append_default(). */
    home = secure_getenv("HOME");
    xdg = secure_getenv("XDG_CACHE_HOME");
    if (xdg != NULL) {
        path = asprintf_safe("%s/xkb/XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", xdg);
    } else if (home != NULL) {
        /* XDG_CACHE_HOME fallback is $HOME/.cache/ */
        path = asprintf_safe("%s/.cache/xkb/XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", home);
    }

    if (!path)
        return false;

    if (!hash_string(string, len, &hash)) {
        free(path);
        return false;
    }

    hash_part = strrchr(path, '/') + 1;
    *hash_part = '\0';
    mkdir(path, 0755);

    for (int i = 0; i < 16; i++) {
        sprintf(hash_part, "%2.2x", hash.digest[i]);
        hash_part += 2;
    }

    *cache_path = path;
    return true;
}

bool
cache_read(struct xkb_compose_table *table, const char *path)
{
    FILE *f;
    unsigned length;

#define FREAD(ptr, size, nmemb, stream) \
    do { \
        if (fread(ptr, size, nmemb, stream) < nmemb) \
            return false; \
    } while (0)
#define FREAD_n(ptr, nmemb, stream) \
    FREAD(ptr, sizeof(*ptr), nmemb, stream)
#define FREAD_1(value, stream) \
    FREAD_n(&value, 1, stream)
#define FREAD_DARRAY(array, stream) \
    do { \
        FREAD_1(length, f); \
        darray_resize(array, length); \
        FREAD_n(array.item, length, stream); \
    } while (0)

    f = fopen(path, "rb");
    if (!f)
        return false;

    FREAD_1(table->format, f);
    if (table->format != XKB_COMPOSE_FORMAT_TEXT_V1)
        goto error;

    FREAD_1(table->flags, f);
    if (table->flags != XKB_COMPOSE_COMPILE_NO_FLAGS)
        goto error;

    FREAD_1(length, f);
    table->locale = realloc(table->locale, length + 1);
    if (!table->locale)
        goto error;
    FREAD(table->locale, length, 1, f);
    table->locale[length] = '\0';
    fseek(f, (4 - (length % 4)) % 4, SEEK_CUR);

    FREAD_DARRAY(table->utf8, f);
    FREAD_DARRAY(table->nodes, f);

#undef FREAD_DARRAY
#undef FREAD_1
#undef FREAD_n
#undef FREAD

    fclose(f);
    return true;

error:
    fclose(f);
    unlink(path);
    return false;
}

void
cache_write(struct xkb_compose_table *table, const char *path)
{
    FILE *f;
    unsigned locale_len = strlen(table->locale);

#define FWRITE(ptr, size, nmemb, stream) \
    do { \
        if (fwrite(ptr, size, nmemb, stream) < nmemb) \
            goto error; \
    } while (0)
#define FWRITE_n(ptr, nmemb, stream) \
    FWRITE(ptr, sizeof(*ptr), nmemb, stream)
#define FWRITE_1(value, stream) \
    FWRITE_n(&value, 1, stream)
#define FWRITE_DARRAY(array, stream) \
    do { \
        FWRITE_1(array.size, f); \
        FWRITE_n(array.item, array.size, stream); \
    } while (0)

    f = fopen(path, "wb");
    if (!f)
        return;

    FWRITE_1(table->format, f);
    FWRITE_1(table->flags, f);
    FWRITE_1(locale_len, f);
    FWRITE(table->locale, locale_len, 1, f);
    FWRITE("\0\0", (4 - (locale_len % 4)) % 4, 1, f);
    FWRITE_DARRAY(table->utf8, f);
    FWRITE_DARRAY(table->nodes, f);

    if (fclose(f) != 0)
        goto fclose_error;

    return;

#undef FWRITE_DARRAY
#undef FWRITE_1
#undef FWRITE_n
#undef FWRITE

error:
    fclose(f);
fclose_error:
    unlink(path);
}
