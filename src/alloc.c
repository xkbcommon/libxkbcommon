/*
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "xkb-priv.h"
#include "alloc.h"

int
XkbcAllocClientMap(struct xkb_keymap *keymap, unsigned which,
                   size_t nTotalTypes)
{
    if (!keymap || ((nTotalTypes > 0) && (nTotalTypes < XkbNumRequiredTypes)))
        return BadValue;

    if (!keymap->map) {
        keymap->map = calloc(1, sizeof(*keymap->map));
        if (!keymap->map)
            return BadAlloc;
        darray_init(keymap->map->types);
    }

    if (which & XkbKeyTypesMask)
        darray_growalloc(keymap->map->types, nTotalTypes);

    if (which & XkbKeySymsMask)
        darray_resize0(keymap->map->key_sym_map, keymap->max_key_code + 1);

    if (which & XkbModifierMapMask) {
        if (!keymap->map->modmap) {
            keymap->map->modmap = uTypedCalloc(keymap->max_key_code + 1,
                                               unsigned char);
            if (!keymap->map->modmap)
                return BadAlloc;
        }
    }

    return Success;
}

int
XkbcAllocServerMap(struct xkb_keymap *keymap, unsigned which,
                   unsigned nNewActions)
{
    unsigned i;
    struct xkb_server_map * map;

    if (!keymap)
        return BadMatch;

    if (!keymap->server) {
        map = uTypedCalloc(1, struct xkb_server_map);
        if (!map)
            return BadAlloc;

        for (i = 0; i < XkbNumVirtualMods; i++)
            map->vmods[i] = XkbNoModifierMask;

        keymap->server = map;
    }
    else
        map = keymap->server;

    if (!which)
        return Success;

    if (!map->explicit) {
        i = keymap->max_key_code + 1;
        map->explicit = uTypedCalloc(i, unsigned char);
        if (!map->explicit)
            return BadAlloc;
    }

    if (nNewActions < 1)
        nNewActions = 1;

    darray_resize0(map->acts, darray_size(map->acts) + nNewActions + 1);
    darray_resize0(map->key_acts, keymap->max_key_code + 1);

    if (!map->behaviors) {
        i = keymap->max_key_code + 1;
        map->behaviors = uTypedCalloc(i, struct xkb_behavior);
        if (!map->behaviors)
            return BadAlloc;
    }

    if (!map->vmodmap) {
        i = keymap->max_key_code + 1;
        map->vmodmap = uTypedCalloc(i, uint32_t);
        if (!map->vmodmap)
            return BadAlloc;
    }

    return Success;
}

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

    darray_init(into->map);
    darray_from_items(into->map,
                      &darray_item(from->map, 0),
                      darray_size(from->map));

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

bool
XkbcResizeKeySyms(struct xkb_keymap *keymap, xkb_keycode_t key,
                  unsigned int needed)
{
    struct xkb_sym_map *sym_map =
        &darray_item(keymap->map->key_sym_map, key);

    if (sym_map->size_syms >= needed)
        return true;

    sym_map->syms = uTypedRecalloc(sym_map->syms, sym_map->size_syms,
                                   needed, xkb_keysym_t);
    if (!sym_map->syms) {
        sym_map->size_syms = 0;
        return false;
    }

    sym_map->size_syms = needed;
    return true;
}

union xkb_action *
XkbcResizeKeyActions(struct xkb_keymap *keymap, xkb_keycode_t key,
                     uint32_t needed)
{
    size_t old_ndx, old_num_acts, new_ndx;

    if (needed == 0) {
        darray_item(keymap->server->key_acts, key) = 0;
        return NULL;
    }

    if (XkbKeyHasActions(keymap, key) &&
        XkbKeyGroupsWidth(keymap, key) >= needed)
        return XkbKeyActionsPtr(keymap, key);

    /*
     * The key may already be in the array, but without enough space.
     * This should not happen often, so in order to avoid moving and
     * copying stuff from acts and key_acts, we just allocate new
     * space for the key at the end, and leave the old space alone.
     */

    old_ndx = darray_item(keymap->server->key_acts, key);
    old_num_acts = XkbKeyNumActions(keymap, key);
    new_ndx = darray_size(keymap->server->acts);

    darray_resize0(keymap->server->acts, new_ndx + needed);
    darray_item(keymap->server->key_acts, key) = new_ndx;

    /*
     * The key was already in the array, copy the old actions to the
     * new space.
     */
    if (old_ndx != 0)
        memcpy(&darray_item(keymap->server->acts, new_ndx),
               &darray_item(keymap->server->acts, old_ndx),
               old_num_acts * sizeof(union xkb_action));

    return XkbKeyActionsPtr(keymap, key);
}

void
XkbcFreeClientMap(struct xkb_keymap *keymap)
{
    struct xkb_client_map * map;
    struct xkb_key_type * type;
    struct xkb_sym_map *sym_map;

    if (!keymap || !keymap->map)
        return;

    map = keymap->map;

    darray_foreach(type, map->types) {
        int j;
        darray_free(type->map);
        free(type->preserve);
        for (j = 0; j < type->num_levels; j++)
            free(type->level_names[j]);
        free(type->level_names);
        free(type->name);
    }
    darray_free(map->types);

    darray_foreach(sym_map, map->key_sym_map) {
        free(sym_map->sym_index);
        free(sym_map->num_syms);
        free(sym_map->syms);
    }
    darray_free(map->key_sym_map);

    free(map->modmap);
    free(keymap->map);
    keymap->map = NULL;
}

void
XkbcFreeServerMap(struct xkb_keymap *keymap)
{
    struct xkb_server_map * map;

    if (!keymap || !keymap->server)
        return;

    map = keymap->server;

    free(map->explicit);
    darray_free(map->key_acts);
    darray_free(map->acts);
    free(map->behaviors);
    free(map->vmodmap);
    free(keymap->server);
    keymap->server = NULL;
}

int
XkbcAllocCompatMap(struct xkb_keymap *keymap, unsigned nSI)
{
    if (!keymap)
        return BadMatch;

    if (!keymap->compat) {
        keymap->compat = calloc(1, sizeof(*keymap->compat));
        if (!keymap->compat)
            return BadAlloc;
        darray_init(keymap->compat->sym_interpret);
    }

    darray_growalloc(keymap->compat->sym_interpret, nSI);
    darray_resize(keymap->compat->sym_interpret, 0);

    memset(keymap->compat->groups, 0,
           XkbNumKbdGroups * sizeof(*keymap->compat->groups));

    return Success;
}


static void
XkbcFreeCompatMap(struct xkb_keymap *keymap)
{
    if (!keymap || !keymap->compat)
        return;

    darray_free(keymap->compat->sym_interpret);
    free(keymap->compat);
    keymap->compat = NULL;
}

int
XkbcAllocNames(struct xkb_keymap *keymap, unsigned which, size_t nTotalAliases)
{
    if (!keymap)
        return BadMatch;

    if (!keymap->names) {
        keymap->names = calloc(1, sizeof(*keymap->names));
        if (!keymap->names)
            return BadAlloc;

        darray_init(keymap->names->keys);
        darray_init(keymap->names->key_aliases);
    }

    if ((which & XkbKTLevelNamesMask) && keymap->map) {
        struct xkb_key_type * type;

        darray_foreach(type, keymap->map->types) {
            if (!type->level_names) {
                type->level_names = calloc(type->num_levels,
                                           sizeof(*type->level_names));
                if (!type->level_names)
                    return BadAlloc;
            }
        }
    }

    if (which & XkbKeyNamesMask)
        darray_resize0(keymap->names->keys, keymap->max_key_code + 1);

    if (which & XkbKeyAliasesMask)
        darray_resize0(keymap->names->key_aliases, nTotalAliases);

    return Success;
}

static void
XkbcFreeNames(struct xkb_keymap *keymap)
{
    struct xkb_names * names;
    struct xkb_client_map * map;
    struct xkb_key_type *type;
    int i;

    if (!keymap || !keymap->names)
        return;

    names = keymap->names;
    map = keymap->map;

    if (map) {
        darray_foreach(type, map->types) {
            int j;
            for (j = 0; j < type->num_levels; j++)
                free(type->level_names[j]);
            free(type->level_names);
            type->level_names = NULL;
        }
    }

    for (i = 0; i < XkbNumVirtualMods; i++)
        free(names->vmods[i]);
    for (i = 0; i < XkbNumIndicators; i++)
        free(names->indicators[i]);
    for (i = 0; i < XkbNumKbdGroups; i++)
        free(names->groups[i]);

    darray_free(names->keys);
    darray_free(names->key_aliases);
    free(names);
    keymap->names = NULL;
}

int
XkbcAllocControls(struct xkb_keymap *keymap)
{
    if (!keymap)
        return BadMatch;

    if (!keymap->ctrls) {
        keymap->ctrls = uTypedCalloc(1, struct xkb_controls);
        if (!keymap->ctrls)
            return BadAlloc;
    }

    keymap->ctrls->per_key_repeat = uTypedCalloc(keymap->max_key_code >> 3,
                                                 unsigned char);
    if (!keymap->ctrls->per_key_repeat)
        return BadAlloc;

    return Success;
}

static void
XkbcFreeControls(struct xkb_keymap *keymap)
{
    if (keymap && keymap->ctrls) {
        free(keymap->ctrls->per_key_repeat);
        free(keymap->ctrls);
        keymap->ctrls = NULL;
    }
}

int
XkbcAllocIndicatorMaps(struct xkb_keymap *keymap)
{
    if (!keymap)
        return BadMatch;

    if (!keymap->indicators) {
        keymap->indicators = uTypedCalloc(1, struct xkb_indicator);
        if (!keymap->indicators)
            return BadAlloc;
    }

    return Success;
}

static void
XkbcFreeIndicatorMaps(struct xkb_keymap *keymap)
{
    if (keymap) {
        free(keymap->indicators);
        keymap->indicators = NULL;
    }
}

struct xkb_keymap *
XkbcAllocKeyboard(struct xkb_context *ctx)
{
    struct xkb_keymap *keymap;

    keymap = uTypedCalloc(1, struct xkb_keymap);
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

    XkbcFreeClientMap(keymap);
    XkbcFreeServerMap(keymap);
    XkbcFreeCompatMap(keymap);
    XkbcFreeIndicatorMaps(keymap);
    XkbcFreeNames(keymap);
    XkbcFreeControls(keymap);
    xkb_context_unref(keymap->ctx);
    free(keymap);
}
