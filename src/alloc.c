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

#include "utils.h"
#include "xkballoc.h"
#include "xkbcommon/xkbcommon.h"
#include "XKBcommonint.h"

int
XkbcAllocClientMap(struct xkb_keymap * xkb, unsigned which, unsigned nTotalTypes)
{
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
XkbcAllocServerMap(struct xkb_keymap * xkb, unsigned which, unsigned nNewActions)
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

    if (!map->explicit) {
        i = xkb->max_key_code + 1;
        map->explicit = uTypedCalloc(i, unsigned char);
        if (!map->explicit)
            return BadAlloc;
    }

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

    if (!map->behaviors) {
        i = xkb->max_key_code + 1;
        map->behaviors = uTypedCalloc(i, struct xkb_behavior);
        if (!map->behaviors)
            return BadAlloc;
    }

    if (!map->vmodmap) {
        i = xkb->max_key_code + 1;
        map->vmodmap = uTypedCalloc(i, uint32_t);
        if (!map->vmodmap)
            return BadAlloc;
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

bool
XkbcResizeKeySyms(struct xkb_keymap * xkb, xkb_keycode_t key,
                  unsigned int needed)
{
    if (xkb->map->key_sym_map[key].size_syms >= needed)
        return true;

    xkb->map->key_sym_map[key].syms =
        uTypedRecalloc(xkb->map->key_sym_map[key].syms,
                       xkb->map->key_sym_map[key].size_syms,
                       needed,
                       xkb_keysym_t);
    if (!xkb->map->key_sym_map[key].syms) {
        xkb->map->key_sym_map[key].size_syms = 0;
        return false;
    }
    xkb->map->key_sym_map[key].size_syms = needed;

    return true;
}

union xkb_action *
XkbcResizeKeyActions(struct xkb_keymap * xkb, xkb_keycode_t key, uint32_t needed)
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
XkbcFreeClientMap(struct xkb_keymap * xkb)
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

    if (map->key_sym_map) {
        for (key = xkb->min_key_code; key < xkb->max_key_code; key++) {
            free(map->key_sym_map[key].sym_index);
            free(map->key_sym_map[key].num_syms);
            free(map->key_sym_map[key].syms);
        }
    }
    free(map->key_sym_map);

    free(map->modmap);
    free(xkb->map);
    xkb->map = NULL;
}

void
XkbcFreeServerMap(struct xkb_keymap * xkb)
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

int
XkbcAllocCompatMap(struct xkb_keymap *xkb, unsigned nSI)
{
    struct xkb_compat_map * compat;
    struct xkb_sym_interpret *prev_interpret;

    if (!xkb)
        return BadMatch;

    if (xkb->compat) {
        if (xkb->compat->size_si >= nSI)
            return Success;

        compat = xkb->compat;
        compat->size_si = nSI;
        if (!compat->sym_interpret)
            compat->num_si = 0;

        prev_interpret = compat->sym_interpret;
        compat->sym_interpret = uTypedRecalloc(compat->sym_interpret,
                                               compat->num_si, nSI,
                                               struct xkb_sym_interpret);
        if (!compat->sym_interpret) {
            free(prev_interpret);
            compat->size_si = compat->num_si = 0;
            return BadAlloc;
        }

        return Success;
    }

    compat = uTypedCalloc(1, struct xkb_compat_map);
    if (!compat)
        return BadAlloc;

    if (nSI > 0) {
        compat->sym_interpret = uTypedCalloc(nSI, struct xkb_sym_interpret);
        if (!compat->sym_interpret) {
            free(compat);
            return BadAlloc;
        }
    }
    compat->size_si = nSI;
    compat->num_si = 0;
    memset(&compat->groups[0], 0, XkbNumKbdGroups * sizeof(struct xkb_mods));
    xkb->compat = compat;

    return Success;
}


static void
XkbcFreeCompatMap(struct xkb_keymap * xkb)
{
    struct xkb_compat_map * compat;

    if (!xkb || !xkb->compat)
        return;

    compat = xkb->compat;

    free(compat->sym_interpret);
    free(compat);
    xkb->compat = NULL;
}

int
XkbcAllocNames(struct xkb_keymap * xkb, unsigned which, unsigned nTotalAliases)
{
    struct xkb_names * names;

    if (!xkb)
        return BadMatch;

    if (!xkb->names) {
        xkb->names = uTypedCalloc(1, struct xkb_names);
        if (!xkb->names)
            return BadAlloc;
    }
    names = xkb->names;

    if ((which & XkbKTLevelNamesMask) && xkb->map && xkb->map->types) {
        int i;
        struct xkb_key_type * type;

        type = xkb->map->types;
        for (i = 0; i < xkb->map->num_types; i++, type++) {
            if (!type->level_names) {
                type->level_names = uTypedCalloc(type->num_levels, const char *);
                if (!type->level_names)
                    return BadAlloc;
            }
        }
    }

    if ((which & XkbKeyNamesMask) && !names->keys) {
        if (!xkb_keymap_keycode_range_is_legal(xkb))
            return BadMatch;

        names->keys = uTypedCalloc(xkb->max_key_code + 1, struct xkb_key_name);
        if (!names->keys)
            return BadAlloc;
    }

    if ((which & XkbKeyAliasesMask) && (nTotalAliases > 0)) {
        if (!names->key_aliases)
            names->key_aliases = uTypedCalloc(nTotalAliases,
                                                 struct xkb_key_alias);
        else if (nTotalAliases > names->num_key_aliases) {
            struct xkb_key_alias *prev_aliases = names->key_aliases;

            names->key_aliases = uTypedRecalloc(names->key_aliases,
                                                names->num_key_aliases,
                                                nTotalAliases,
                                                struct xkb_key_alias);
            if (!names->key_aliases)
                free(prev_aliases);
        }

        if (!names->key_aliases) {
            names->num_key_aliases = 0;
            return BadAlloc;
        }

        names->num_key_aliases = nTotalAliases;
    }

    return Success;
}

static void
XkbcFreeNames(struct xkb_keymap * xkb)
{
    struct xkb_names * names;
    struct xkb_client_map * map;
    int i;

    if (!xkb || !xkb->names)
        return;

    names = xkb->names;
    map = xkb->map;

    if (map && map->types) {
        struct xkb_key_type * type = map->types;

        for (i = 0; i < map->num_types; i++, type++) {
            int j;
            for (j = 0; j < type->num_levels; j++)
                free(UNCONSTIFY(type->level_names[i]));
            free(type->level_names);
            type->level_names = NULL;
        }
    }

    for (i = 0; i < XkbNumVirtualMods; i++)
        free(UNCONSTIFY(names->vmods[i]));
    for (i = 0; i < XkbNumIndicators; i++)
        free(UNCONSTIFY(names->indicators[i]));
    for (i = 0; i < XkbNumKbdGroups; i++)
        free(UNCONSTIFY(names->groups[i]));

    free(names->keys);
    free(names->key_aliases);
    free(names);
    xkb->names = NULL;
}

int
XkbcAllocControls(struct xkb_keymap * xkb)
{
    if (!xkb)
        return BadMatch;

    if (!xkb->ctrls) {
        xkb->ctrls = uTypedCalloc(1, struct xkb_controls);
        if (!xkb->ctrls)
            return BadAlloc;
    }

    xkb->ctrls->per_key_repeat = uTypedCalloc(xkb->max_key_code << 3,
                                              unsigned char);
    if (!xkb->ctrls->per_key_repeat)
        return BadAlloc;

    return Success;
}

static void
XkbcFreeControls(struct xkb_keymap * xkb)
{
    if (xkb && xkb->ctrls) {
        free(xkb->ctrls->per_key_repeat);
        free(xkb->ctrls);
        xkb->ctrls = NULL;
    }
}

int
XkbcAllocIndicatorMaps(struct xkb_keymap * xkb)
{
    if (!xkb)
        return BadMatch;

    if (!xkb->indicators) {
        xkb->indicators = uTypedCalloc(1, struct xkb_indicator);
        if (!xkb->indicators)
            return BadAlloc;
    }

    return Success;
}

static void
XkbcFreeIndicatorMaps(struct xkb_keymap * xkb)
{
    if (xkb) {
        free(xkb->indicators);
        xkb->indicators = NULL;
    }
}

struct xkb_keymap *
XkbcAllocKeyboard(struct xkb_context *context)
{
    struct xkb_keymap *xkb;

    xkb = uTypedCalloc(1, struct xkb_keymap);
    if (!xkb)
        return NULL;

    xkb->refcnt = 1;
    xkb->context = xkb_context_ref(context);

    return xkb;
}

void
XkbcFreeKeyboard(struct xkb_keymap * xkb)
{
    if (!xkb)
        return;

    XkbcFreeClientMap(xkb);
    XkbcFreeServerMap(xkb);
    XkbcFreeCompatMap(xkb);
    XkbcFreeIndicatorMaps(xkb);
    XkbcFreeNames(xkb);
    XkbcFreeControls(xkb);
    xkb_context_unref(xkb->context);
    free(xkb);
}
