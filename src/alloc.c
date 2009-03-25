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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "X11/extensions/XKBcommon.h"
#include "XKBcommonint.h"
#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/extensions/XKB.h>

int
XkbcAllocCompatMap(XkbcDescPtr xkb, unsigned which, unsigned nSI)
{
    XkbCompatMapPtr compat;
    XkbSymInterpretRec *prev_interpret;

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
        compat->sym_interpret = _XkbTypedRealloc(compat->sym_interpret,
                                                 nSI, XkbSymInterpretRec);
        if (!compat->sym_interpret) {
            _XkbFree(prev_interpret);
            compat->size_si = compat->num_si = 0;
            return BadAlloc;
        }

        if (compat->num_si != 0)
            _XkbClearElems(compat->sym_interpret, compat->num_si,
                           compat->size_si - 1, XkbSymInterpretRec);

        return Success;
    }

    compat = _XkbTypedCalloc(1, XkbCompatMapRec);
    if (!compat)
        return BadAlloc;

    if (nSI > 0) {
        compat->sym_interpret = _XkbTypedCalloc(nSI, XkbSymInterpretRec);
        if (!compat->sym_interpret) {
            _XkbFree(compat);
            return BadAlloc;
        }
    }
    compat->size_si = nSI;
    compat->num_si = 0;
    bzero(&compat->groups[0], XkbNumKbdGroups * sizeof(XkbModsRec));
    xkb->compat = compat;

    return Success;
}


void
XkbcFreeCompatMap(XkbcDescPtr xkb, unsigned which, Bool freeMap)
{
    XkbCompatMapPtr compat;

    if (!xkb || !xkb->compat)
        return;

    compat = xkb->compat;
    if (freeMap)
        which = XkbAllCompatMask;

    if (which & XkbGroupCompatMask)
        bzero(&compat->groups[0], XkbNumKbdGroups * sizeof(XkbModsRec));

    if (which & XkbSymInterpMask) {
        if (compat->sym_interpret && (compat->size_si > 0))
            _XkbFree(compat->sym_interpret);
        compat->size_si = compat->num_si = 0;
        compat->sym_interpret = NULL;
    }

    if (freeMap) {
        _XkbFree(compat);
        xkb->compat = NULL;
    }
}

int
XkbcAllocNames(XkbcDescPtr xkb, unsigned which, int nTotalRG, int nTotalAliases)
{
    XkbNamesPtr names;

    if (!xkb)
        return BadMatch;

    if (!xkb->names) {
        xkb->names = _XkbTypedCalloc(1, XkbNamesRec);
        if (!xkb->names)
            return BadAlloc;
    }
    names = xkb->names;

    if ((which & XkbKTLevelNamesMask) && xkb->map && xkb->map->types) {
        int i;
        XkbKeyTypePtr type;

        type = xkb->map->types;
        for (i = 0; i < xkb->map->num_types; i++, type++) {
            if (!type->level_names) {
                type->level_names = _XkbTypedCalloc(type->num_levels, Atom);
                if (!type->level_names)
                    return BadAlloc;
            }
        }
    }

    if ((which & XkbKeyNamesMask) && names->keys) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadValue;

        names->keys = _XkbTypedCalloc(xkb->max_key_code + 1, XkbKeyNameRec);
        if (!names->keys)
            return BadAlloc;
    }

    if ((which & XkbKeyAliasesMask) && (nTotalAliases > 0)) {
        if (!names->key_aliases)
            names->key_aliases = _XkbTypedCalloc(nTotalAliases,
                                                 XkbKeyAliasRec);
        else if (nTotalAliases > names->num_key_aliases) {
            XkbKeyAliasRec *prev_aliases = names->key_aliases;

            names->key_aliases = _XkbTypedRealloc(names->key_aliases,
                                                  nTotalAliases,
                                                  XkbKeyAliasRec);
            if (names->key_aliases)
                _XkbClearElems(names->key_aliases, names->num_key_aliases,
                               nTotalAliases - 1, XkbKeyAliasRec);
            else
                _XkbFree(prev_aliases);
        }

        if (!names->key_aliases) {
            names->num_key_aliases = 0;
            return BadAlloc;
        }

        names->num_key_aliases = nTotalAliases;
    }

    if ((which & XkbRGNamesMask) && (nTotalRG > 0)) {
        if (!names->radio_groups)
            names->radio_groups = _XkbTypedCalloc(nTotalRG, Atom);
        else if (nTotalRG > names->num_rg) {
            Atom *prev_radio_groups = names->radio_groups;

            names->radio_groups = _XkbTypedRealloc(names->radio_groups,
                                                   nTotalRG, Atom);
            if (names->radio_groups)
                _XkbClearElems(names->radio_groups, names->num_rg,
                               nTotalRG - 1, Atom);
            else
                _XkbFree(prev_radio_groups);
        }

        if (!names->radio_groups)
            return BadAlloc;

        names->num_rg = nTotalRG;
    }

    return Success;
}

void
XkbcFreeNames(XkbcDescPtr xkb, unsigned which, Bool freeMap)
{
    XkbNamesPtr names;

    if (!xkb || !xkb->names)
        return;

    names = xkb->names;
    if (freeMap)
        which = XkbAllNamesMask;

    if (which & XkbKTLevelNamesMask) {
        XkbClientMapPtr map = xkb->map;

        if (map && map->types) {
            int i;
            XkbKeyTypePtr type = map->types;

            for (i = 0; i < map->num_types; i++, type++) {
                if (type->level_names) {
                    _XkbFree(type->level_names);
                    type->level_names = NULL;
                }
            }
        }
    }

    if ((which & XkbKeyNamesMask) && names->keys) {
        _XkbFree(names->keys);
        names->keys = NULL;
        names->num_keys = 0;
    }

    if ((which & XkbKeyAliasesMask) && names->key_aliases) {
        _XkbFree(names->key_aliases);
        names->key_aliases = NULL;
        names->num_key_aliases = 0;
    }

    if ((which & XkbRGNamesMask) && names->radio_groups) {
        _XkbFree(names->radio_groups);
        names->radio_groups = NULL;
        names->num_rg = 0;
    }

    if (freeMap) {
        _XkbFree(names);
        xkb->names = NULL;
    }
}

int
XkbcAllocControls(XkbcDescPtr xkb, unsigned which)
{
    if (!xkb)
        return BadMatch;

    if (!xkb->ctrls) {
        xkb->ctrls = _XkbTypedCalloc(1, XkbControlsRec);
        if (!xkb->ctrls)
            return BadAlloc;
    }

    return Success;
}

void
XkbcFreeControls(XkbcDescPtr xkb, unsigned which, Bool freeMap)
{
    if (freeMap && xkb && xkb->ctrls) {
        _XkbFree(xkb->ctrls);
        xkb->ctrls = NULL;
    }
}

int
XkbcAllocIndicatorMaps(XkbcDescPtr xkb)
{
    if (!xkb)
        return BadMatch;

    if (!xkb->indicators) {
        xkb->indicators = _XkbTypedCalloc(1, XkbIndicatorRec);
        if (!xkb->indicators)
            return BadAlloc;
    }

    return Success;
}

void
XkbcFreeIndicatorMaps(XkbcDescPtr xkb)
{
    if (xkb && xkb->indicators) {
        _XkbFree(xkb->indicators);
        xkb->indicators = NULL;
    }
}

XkbcDescRec *
XkbcAllocKeyboard(void)
{
    XkbcDescRec *xkb;

    xkb = _XkbTypedCalloc(1, XkbcDescRec);
    if (xkb)
        xkb->device_spec = XkbUseCoreKbd;
    return xkb;
}

void
XkbcFreeKeyboard(XkbcDescPtr xkb, unsigned which, Bool freeAll)
{
    if (!xkb)
        return;

    if (freeAll)
        which = XkbAllComponentsMask;

    if (which & XkbClientMapMask)
        XkbcFreeClientMap(xkb, XkbAllClientInfoMask, True);
    if (which & XkbServerMapMask)
        XkbcFreeServerMap(xkb, XkbAllServerInfoMask, True);
    if (which & XkbCompatMapMask)
        XkbcFreeCompatMap(xkb, XkbAllCompatMask, True);
    if (which & XkbIndicatorMapMask)
        XkbcFreeIndicatorMaps(xkb);
    if (which & XkbNamesMask)
        XkbcFreeNames(xkb, XkbAllNamesMask, True);
    if ((which & XkbGeometryMask) && xkb->geom)
        XkbcFreeGeometry(xkb->geom, XkbGeomAllMask, True);
    if (which & XkbControlsMask)
        XkbcFreeControls(xkb, XkbAllControlsMask, True);
    if (freeAll)
        _XkbFree(xkb);
}

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
