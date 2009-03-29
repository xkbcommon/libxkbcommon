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

#include "X11/extensions/XKBcommon.h"
#include "XKBcommonint.h"

int
XkbcAllocClientMap(XkbcDescPtr xkb, unsigned which, unsigned nTotalTypes)
{
    int i;
    XkbClientMapPtr map;

    if (!xkb || ((nTotalTypes > 0) && (nTotalTypes < XkbNumRequiredTypes)))
        return BadValue;

    if ((which & XkbKeySymsMask) &&
        ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
         (!XkbIsLegalKeycode(xkb->max_key_code)) ||
         (xkb->max_key_code < xkb->min_key_code))) {
#ifdef DEBUG
        fprintf(stderr, "bad keycode (%d,%d) in XkbAllocClientMap\n",
                xkb->min_key_code, xkb->max_key_code);
#endif
        return BadValue;
    }

    if (!xkb->map) {
        map = _XkbTypedCalloc(1, XkbClientMapRec);
        if (!map)
            return BadAlloc;
        xkb->map = map;
    }
    else
        map = xkb->map;

    if ((which & XkbKeyTypesMask) && (nTotalTypes > 0)) {
        if (!map->types) {
            map->types = _XkbTypedCalloc(nTotalTypes, XkbKeyTypeRec);
            if (!map->types)
                return BadAlloc;

            map->num_types = 0;
            map->size_types = nTotalTypes;
        }
        else if (map->size_types < nTotalTypes) {
            XkbKeyTypeRec *prev_types = map->types;

            map->types = _XkbTypedRealloc(map->types, nTotalTypes,
                                          XkbKeyTypeRec);
            if (!map->types) {
                _XkbFree(prev_types);
                map->num_types = map->size_types = 0;
                return BadAlloc;
            }

            map->size_types = nTotalTypes;
            bzero(&map->types[map->num_types],
                  (map->size_types - map->num_types) * sizeof(XkbKeyTypeRec));
        }
    }

    if (which & XkbKeySymsMask) {
        int nKeys = XkbNumKeys(xkb);

        if (!map->syms) {
            map->size_syms = (nKeys * 15) / 10;
            map->syms = _XkbTypedCalloc(map->size_syms, KeySym);
            if (!map->syms) {
                map->size_syms = 0;
                return BadAlloc;
            }
            map->num_syms = 1;
            map->syms[0] = NoSymbol;
        }

        if (!map->key_sym_map) {
            i = xkb->max_key_code + 1;
            map->key_sym_map = _XkbTypedCalloc(i, XkbSymMapRec);
            if (!map->key_sym_map)
                return BadAlloc;
        }
    }

    if (which & XkbModifierMapMask) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadMatch;

        if (!map->modmap) {
            i = xkb->max_key_code + 1;
            map->modmap = _XkbTypedCalloc(i, unsigned char);
            if (!map->modmap)
                return BadAlloc;
        }
    }

    return Success;
}

int
XkbcAllocServerMap(XkbcDescPtr xkb, unsigned which, unsigned nNewActions)
{
    int i;
    XkbServerMapPtr map;

    if (!xkb)
        return BadMatch;

    if (!xkb->server) {
        map = _XkbTypedCalloc(1, XkbServerMapRec);
        if (!map)
            return BadAlloc;

        for (i = 0; i < XkbNumVirtualMods; i++)
            map->vmods[i] = XkbNoModifierMask;

        xkb->server = map;
    }
    else
        map = xkb->server;

    if (which & XkbExplicitComponentsMask) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadMatch;

        if (!map->explicit) {
            i = xkb->max_key_code + 1;
            map->explicit = _XkbTypedCalloc(i, unsigned char);
            if (!map->explicit)
                return BadAlloc;
        }
    }

    if (which&XkbKeyActionsMask) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadMatch;

        if (nNewActions < 1)
            nNewActions = 1;

        if (!map->acts) {
            map->acts = _XkbTypedCalloc(nNewActions + 1, XkbAction);
            if (!map->acts)
                return BadAlloc;
            map->num_acts = 1;
            map->size_acts = nNewActions + 1;
        }
        else if ((map->size_acts - map->num_acts) < nNewActions) {
            unsigned need;
            XkbAction *prev_acts = map->acts;

            need = map->num_acts + nNewActions;
            map->acts = _XkbTypedRealloc(map->acts, need, XkbAction);
            if (!map->acts) {
                _XkbFree(prev_acts);
                map->num_acts = map->size_acts = 0;
                return BadAlloc;
            }

            map->size_acts = need;
            bzero(&map->acts[map->num_acts],
                  (map->size_acts - map->num_acts) * sizeof(XkbAction));
        }

        if (!map->key_acts) {
            i = xkb->max_key_code + 1;
            map->key_acts = _XkbTypedCalloc(i, unsigned short);
            if (!map->key_acts)
                return BadAlloc;
        }
    }

    if (which & XkbKeyBehaviorsMask) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadMatch;

        if (!map->behaviors) {
            i = xkb->max_key_code + 1;
            map->behaviors = _XkbTypedCalloc(i, XkbBehavior);
            if (!map->behaviors)
                return BadAlloc;
        }
    }

    if (which & XkbVirtualModMapMask) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadMatch;

        if (!map->vmodmap) {
            i = xkb->max_key_code + 1;
            map->vmodmap = _XkbTypedCalloc(i, unsigned short);
            if (!map->vmodmap)
                return BadAlloc;
        }
    }

    return Success;
}

int
XkbcCopyKeyType(XkbKeyTypePtr from, XkbKeyTypePtr into)
{
    if (!from || !into)
        return BadMatch;

    if (into->map) {
        _XkbFree(into->map);
        into->map = NULL;
    }
    if (into->preserve) {
       _XkbFree(into->preserve);
       into->preserve= NULL;
    }
    if (into->level_names) {
        _XkbFree(into->level_names);
        into->level_names = NULL;
    }

    *into = *from;

    if (from->map && (into->map_count > 0)) {
        into->map = _XkbTypedCalloc(into->map_count, XkbKTMapEntryRec);
        if (!into->map)
            return BadAlloc;
        memcpy(into->map, from->map,
               into->map_count * sizeof(XkbKTMapEntryRec));
    }

    if (from->preserve && (into->map_count > 0)) {
        into->preserve = _XkbTypedCalloc(into->map_count, XkbModsRec);
        if (!into->preserve)
            return BadAlloc;
        memcpy(into->preserve, from->preserve,
               into->map_count * sizeof(XkbModsRec));
    }

    if (from->level_names && (into->num_levels > 0)) {
        into->level_names = _XkbTypedCalloc(into->num_levels, Atom);
        if (!into->level_names)
            return BadAlloc;
        memcpy(into->level_names, from->level_names,
               into->num_levels * sizeof(Atom));
    }

    return Success;
}

int
XkbcCopyKeyTypes(XkbKeyTypePtr from, XkbKeyTypePtr into, int num_types)
{
    int i, rtrn;

    if (!from || !into || (num_types < 0))
        return BadMatch;

    for (i = 0; i < num_types; i++) {
        if ((rtrn = XkbcCopyKeyType(from++, into++)) != Success)
            return rtrn;
    }

    return Success;
}

void
XkbcFreeClientMap(XkbcDescPtr xkb, unsigned what, Bool freeMap)
{
    XkbClientMapPtr map;

    if (!xkb || !xkb->map)
        return;

    if (freeMap)
        what = XkbAllClientInfoMask;
    map = xkb->map;

    if (what & XkbKeyTypesMask) {
        if (map->types) {
            if (map->num_types > 0) {
                int i;
                XkbKeyTypePtr type;

                for (i = 0, type = map->types; i < map->num_types; i++, type++) {
                    if (type->map) {
                        _XkbFree(type->map);
                        type->map = NULL;
                    }
                    if (type->preserve) {
                        _XkbFree(type->preserve);
                        type->preserve = NULL;
                    }
                    type->map_count = 0;
                    if (type->level_names) {
                        _XkbFree(type->level_names);
                        type->level_names = NULL;
                    }
                }
            }
            _XkbFree(map->types);
            map->num_types = map->size_types = 0;
            map->types = NULL;
        }
    }

    if (what & XkbKeySymsMask) {
        if (map->key_sym_map) {
            _XkbFree(map->key_sym_map);
            map->key_sym_map = NULL;
        }
        if (map->syms) {
            _XkbFree(map->syms);
            map->size_syms = map->num_syms = 0;
            map->syms = NULL;
        }
    }

    if ((what & XkbModifierMapMask) && map->modmap) {
        _XkbFree(map->modmap);
        map->modmap = NULL;
    }

    if (freeMap) {
        _XkbFree(xkb->map);
        xkb->map = NULL;
    }
}

void
XkbcFreeServerMap(XkbcDescPtr xkb, unsigned what, Bool freeMap)
{
    XkbServerMapPtr map;

    if (!xkb || !xkb->server)
        return;

    if (freeMap)
        what = XkbAllServerInfoMask;
    map = xkb->server;

    if ((what & XkbExplicitComponentsMask) && map->explicit) {
        _XkbFree(map->explicit);
        map->explicit = NULL;
    }

    if (what & XkbKeyActionsMask) {
        if (map->key_acts) {
            _XkbFree(map->key_acts);
            map->key_acts = NULL;
        }
        if (map->acts) {
            _XkbFree(map->acts);
            map->num_acts = map->size_acts = 0;
            map->acts = NULL;
        }
    }

    if ((what & XkbKeyBehaviorsMask) && map->behaviors) {
        _XkbFree(map->behaviors);
        map->behaviors = NULL;
    }

    if ((what & XkbVirtualModMapMask) && map->vmodmap) {
        _XkbFree(map->vmodmap);
        map->vmodmap = NULL;
    }

    if (freeMap) {
        _XkbFree(xkb->server);
        xkb->server = NULL;
    }
}
