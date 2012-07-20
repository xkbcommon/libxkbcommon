/*
 * Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "xkb-priv.h"
#include "alloc.h"

int
XkbcCopyKeyType(const struct xkb_key_type *from, struct xkb_key_type *into)
{
    int i;

    if (!from || !into)
        return BadMatch;

    darray_free(into->map);
    free(into->preserve);
    for (i = 0; i < into->num_levels; i++)
        free(into->level_names[i]);
    free(into->level_names);

    *into = *from;

    darray_copy(into->map, from->map);

    if (from->preserve && !darray_empty(into->map)) {
        into->preserve = calloc(darray_size(into->map),
                                sizeof(*into->preserve));
        if (!into->preserve)
            return BadAlloc;
        memcpy(into->preserve, from->preserve,
               darray_size(into->map) * sizeof(*into->preserve));
    }

    if (from->level_names && into->num_levels > 0) {
        into->level_names = calloc(into->num_levels,
                                   sizeof(*into->level_names));
        if (!into->level_names)
            return BadAlloc;

        for (i = 0; i < into->num_levels; i++)
            into->level_names[i] = strdup(from->level_names[i]);
    }

    return Success;
}

union xkb_action *
XkbcResizeKeyActions(struct xkb_keymap *keymap, struct xkb_key *key,
                     uint32_t needed)
{
    size_t old_ndx, old_num_acts, new_ndx;

    if (needed == 0) {
        key->acts_index = 0;
        return NULL;
    }

    if (XkbKeyHasActions(key) && key->width >= needed)
        return XkbKeyActionsPtr(keymap, key);

    /*
     * The key may already be in the array, but without enough space.
     * This should not happen often, so in order to avoid moving and
     * copying stuff from acts and key_acts, we just allocate new
     * space for the key at the end, and leave the old space alone.
     */

    old_ndx = key->acts_index;
    old_num_acts = XkbKeyNumActions(key);
    new_ndx = darray_size(keymap->acts);

    darray_resize0(keymap->acts, new_ndx + needed);
    key->acts_index = new_ndx;

    /*
     * The key was already in the array, copy the old actions to the
     * new space.
     */
    if (old_ndx != 0)
        memcpy(darray_mem(keymap->acts, new_ndx),
               darray_mem(keymap->acts, old_ndx),
               old_num_acts * sizeof(union xkb_action));

    return XkbKeyActionsPtr(keymap, key);
}

static void
free_types(struct xkb_keymap *keymap)
{
    struct xkb_key_type *type;

    darray_foreach(type, keymap->types) {
        int j;
        darray_free(type->map);
        free(type->preserve);
        for (j = 0; j < type->num_levels; j++)
            free(type->level_names[j]);
        free(type->level_names);
        free(type->name);
    }
    darray_free(keymap->types);
}

static void
free_keys(struct xkb_keymap *keymap)
{
    struct xkb_key *key;

    darray_foreach(key, keymap->keys) {
        free(key->sym_index);
        free(key->num_syms);
        darray_free(key->syms);
    }

    darray_free(keymap->keys);
}

static void
free_names(struct xkb_keymap *keymap)
{
    int i;

    for (i = 0; i < XkbNumVirtualMods; i++)
        free(keymap->vmod_names[i]);

    for (i = 0; i < XkbNumIndicators; i++)
        free(keymap->indicator_names[i]);

    for (i = 0; i < XkbNumKbdGroups; i++)
        free(keymap->group_names[i]);

    darray_free(keymap->key_aliases);

    free(keymap->keycodes_section_name);
    free(keymap->symbols_section_name);
    free(keymap->types_section_name);
    free(keymap->compat_section_name);
}

struct xkb_keymap *
XkbcAllocKeyboard(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    keymap = calloc(1, sizeof(*keymap));
    if (!keymap)
        return NULL;

    keymap->refcnt = 1;
    keymap->ctx = xkb_context_ref(ctx);

    return keymap;
}

void
XkbcFreeKeyboard(struct xkb_keymap *keymap)
{
    if (!keymap)
        return;

    free_types(keymap);
    darray_free(keymap->acts);
    darray_free(keymap->sym_interpret);
    free_names(keymap);
    free_keys(keymap);
    xkb_context_unref(keymap->ctx);
    free(keymap);
}
