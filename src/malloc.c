/************************************************************
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

********************************************************/

#include "utils.h"
#include "xkballoc.h"
#include "xkbcommon/xkbcommon.h"
#include "XKBcommonint.h"

int
XkbcAllocClientMap(struct xkb_desc * xkb, unsigned which, unsigned nTotalTypes)
{
    int i;
    struct xkb_client_map * map;

    if (!xkb || ((nTotalTypes > 0) && (nTotalTypes < XkbNumRequiredTypes)))
        return BadValue;

    if ((which & XkbKeySymsMask) && !xkb_keymap_keycode_range_is_legal(xkb)) {
#ifdef DEBUG
        fprintf(stderr, "bad keycode (%d,%d) in XkbAllocClientMap\n",
                xkb->min_key_code, xkb->max_key_code);
#endif
        return BadValue;
    }

    if (!xkb->map) {
        map = uTypedCalloc(1, struct xkb_client_map);
        if (!map)
            return BadAlloc;
        xkb->map = map;
    }
    else
        map = xkb->map;

    if ((which & XkbKeyTypesMask) && (nTotalTypes > 0)) {
        if (!map->types) {
            map->types = uTypedCalloc(nTotalTypes, struct xkb_key_type);
            if (!map->types)
                return BadAlloc;

            map->num_types = 0;
            map->size_types = nTotalTypes;
        }
        else if (map->size_types < nTotalTypes) {
            struct xkb_key_type *prev_types = map->types;

            map->types = uTypedRealloc(map->types, nTotalTypes,
                                          struct xkb_key_type);
            if (!map->types) {
                free(prev_types);
                map->num_types = map->size_types = 0;
                return BadAlloc;
            }

            map->size_types = nTotalTypes;
            memset(&map->types[map->num_types], 0,
                   (map->size_types - map->num_types) * sizeof(struct xkb_key_type));
        }
    }

    if (which & XkbKeySymsMask) {
        if (!map->key_sym_map) {
            map->key_sym_map = uTypedCalloc(xkb->max_key_code + 1,
                                            struct xkb_sym_map);
            if (!map->key_sym_map)
                return BadAlloc;
        }
    }

    if (which & XkbModifierMapMask) {
        if (!map->modmap) {
            map->modmap = uTypedCalloc(xkb->max_key_code + 1, unsigned char);
            if (!map->modmap)
                return BadAlloc;
        }
    }

    return Success;
}

int
XkbcAllocServerMap(struct xkb_desc * xkb, unsigned which, unsigned nNewActions)
{
    unsigned i;
    struct xkb_server_map * map;

    if (!xkb)
        return BadMatch;

    if (!xkb->server) {
        map = uTypedCalloc(1, struct xkb_server_map);
        if (!map)
            return BadAlloc;

        for (i = 0; i < XkbNumVirtualMods; i++)
            map->vmods[i] = XkbNoModifierMask;

        xkb->server = map;
    }
    else
        map = xkb->server;

    if (!which)
        return Success;

    if (!xkb_keymap_keycode_range_is_legal(xkb))
        return BadMatch;

    if (which & XkbExplicitComponentsMask) {
        if (!map->explicit) {
            i = xkb->max_key_code + 1;
            map->explicit = uTypedCalloc(i, unsigned char);
            if (!map->explicit)
                return BadAlloc;
        }
    }

    if (which&XkbKeyActionsMask) {
        if (nNewActions < 1)
            nNewActions = 1;

        if (!map->acts) {
            map->acts = uTypedCalloc(nNewActions + 1, union xkb_action);
            if (!map->acts)
                return BadAlloc;
            map->num_acts = 1;
            map->size_acts = nNewActions + 1;
        }
        else if ((map->size_acts - map->num_acts) < (int)nNewActions) {
            unsigned need;
            union xkb_action *prev_acts = map->acts;

            need = map->num_acts + nNewActions;
            map->acts = uTypedRealloc(map->acts, need, union xkb_action);
            if (!map->acts) {
                free(prev_acts);
                map->num_acts = map->size_acts = 0;
                return BadAlloc;
            }

            map->size_acts = need;
            memset(&map->acts[map->num_acts], 0,
                   (map->size_acts - map->num_acts) * sizeof(union xkb_action));
        }

        if (!map->key_acts) {
            i = xkb->max_key_code + 1;
            map->key_acts = uTypedCalloc(i, unsigned short);
            if (!map->key_acts)
                return BadAlloc;
        }
    }

    if (which & XkbKeyBehaviorsMask) {
        if (!map->behaviors) {
            i = xkb->max_key_code + 1;
            map->behaviors = uTypedCalloc(i, struct xkb_behavior);
            if (!map->behaviors)
                return BadAlloc;
        }
    }

    if (which & XkbVirtualModMapMask) {
        if (!map->vmodmap) {
            i = xkb->max_key_code + 1;
            map->vmodmap = uTypedCalloc(i, uint32_t);
            if (!map->vmodmap)
                return BadAlloc;
        }
    }

    return Success;
}

int
XkbcCopyKeyType(struct xkb_key_type * from, struct xkb_key_type * into)
{
    int i;

    if (!from || !into)
        return BadMatch;

    free(into->map);
    into->map = NULL;
    free(into->preserve);
    into->preserve= NULL;
    for (i = 0; i < into->num_levels; i++)
        free(UNCONSTIFY(into->level_names[i]));
    free(into->level_names);
    into->level_names = NULL;

    *into = *from;

    if (from->map && (into->map_count > 0)) {
        into->map = uTypedCalloc(into->map_count, struct xkb_kt_map_entry);
        if (!into->map)
            return BadAlloc;
        memcpy(into->map, from->map,
               into->map_count * sizeof(struct xkb_kt_map_entry));
    }

    if (from->preserve && (into->map_count > 0)) {
        into->preserve = uTypedCalloc(into->map_count, struct xkb_mods);
        if (!into->preserve)
            return BadAlloc;
        memcpy(into->preserve, from->preserve,
               into->map_count * sizeof(struct xkb_mods));
    }

    if (from->level_names && (into->num_levels > 0)) {
        into->level_names = uTypedCalloc(into->num_levels, const char *);
        if (!into->level_names)
            return BadAlloc;
        for (i = 0; i < into->num_levels; i++)
            into->level_names[i] = strdup(from->level_names[i]);
    }

    return Success;
}

Bool
XkbcResizeKeySyms(struct xkb_desc * xkb, xkb_keycode_t key,
                  unsigned int needed)
{
    if (xkb->map->key_sym_map[key].size_syms >= needed)
        return True;

    xkb->map->key_sym_map[key].syms =
        uTypedRecalloc(xkb->map->key_sym_map[key].syms,
                       xkb->map->key_sym_map[key].size_syms,
                       needed,
                       xkb_keysym_t);
    if (!xkb->map->key_sym_map[key].syms) {
        xkb->map->key_sym_map[key].size_syms = 0;
        return False;
    }
    xkb->map->key_sym_map[key].size_syms = needed;

    return True;
}

union xkb_action *
XkbcResizeKeyActions(struct xkb_desc * xkb, xkb_keycode_t key, uint32_t needed)
{
    xkb_keycode_t i, nActs;
    union xkb_action *newActs;

    if (needed == 0) {
        xkb->server->key_acts[key] = 0;
        return NULL;
    }

    if (XkbKeyHasActions(xkb, key) &&
        (XkbKeyGroupsWidth(xkb, key) >= needed))
        return XkbKeyActionsPtr(xkb, key);

    if (xkb->server->size_acts - xkb->server->num_acts >= (int)needed) {
        xkb->server->key_acts[key] = xkb->server->num_acts;
        xkb->server->num_acts += needed;

        return &xkb->server->acts[xkb->server->key_acts[key]];
    }

    xkb->server->size_acts = xkb->server->num_acts + needed + 8;
    newActs = uTypedCalloc(xkb->server->size_acts, union xkb_action);
    if (!newActs)
        return NULL;
    newActs[0].type = XkbSA_NoAction;
    nActs = 1;

    for (i = xkb->min_key_code; i <= xkb->max_key_code; i++) {
        xkb_keycode_t nKeyActs, nCopy;

        if ((xkb->server->key_acts[i] == 0) && (i != key))
            continue;

        nCopy = nKeyActs = XkbKeyNumActions(xkb, i);
        if (i == key) {
            nKeyActs= needed;
            if (needed < nCopy)
                nCopy = needed;
        }

        if (nCopy > 0)
            memcpy(&newActs[nActs], XkbKeyActionsPtr(xkb, i),
                   nCopy * sizeof(union xkb_action));
        if (nCopy < nKeyActs)
            memset(&newActs[nActs + nCopy], 0,
                   (nKeyActs - nCopy) * sizeof(union xkb_action));

        xkb->server->key_acts[i] = nActs;
        nActs += nKeyActs;
    }

    free(xkb->server->acts);
    xkb->server->acts = newActs;
    xkb->server->num_acts = nActs;

    return &xkb->server->acts[xkb->server->key_acts[key]];
}

void
XkbcFreeClientMap(struct xkb_desc * xkb)
{
    struct xkb_client_map * map;
    struct xkb_key_type * type;
    xkb_keycode_t key;
    int i;

    if (!xkb || !xkb->map)
        return;

    map = xkb->map;

    for (i = 0, type = map->types; i < map->num_types && type; i++, type++) {
        int j;
        free(type->map);
        free(type->preserve);
        for (j = 0; j < type->num_levels; j++)
            free(UNCONSTIFY(type->level_names[j]));
        free(type->level_names);
        free(UNCONSTIFY(type->name));
    }
    free(map->types);

    for (key = xkb->min_key_code; key < xkb->max_key_code; key++) {
        free(map->key_sym_map[key].sym_index);
        free(map->key_sym_map[key].num_syms);
        free(map->key_sym_map[key].syms);
    }
    free(map->key_sym_map);

    free(map->modmap);
    free(xkb->map);
    xkb->map = NULL;
}

void
XkbcFreeServerMap(struct xkb_desc * xkb)
{
    struct xkb_server_map * map;

    if (!xkb || !xkb->server)
        return;

    map = xkb->server;

    free(map->explicit);
    free(map->key_acts);
    free(map->acts);
    free(map->behaviors);
    free(map->vmodmap);
    free(xkb->server);
    xkb->server = NULL;
}
