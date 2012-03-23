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
#include "utils.h"
#include "xkballoc.h"
#include "xkbcommon/xkbcommon.h"
#include "XKBcommonint.h"
#include <X11/extensions/XKB.h>

int
XkbcAllocCompatMap(struct xkb_desc * xkb, unsigned which, unsigned nSI)
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
XkbcFreeCompatMap(struct xkb_desc * xkb)
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
XkbcAllocNames(struct xkb_desc * xkb, unsigned which, int nTotalAliases)
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
XkbcFreeNames(struct xkb_desc * xkb)
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
XkbcAllocControls(struct xkb_desc * xkb, unsigned which)
{
    if (!xkb)
        return BadMatch;

    if (!xkb->ctrls) {
        xkb->ctrls = uTypedCalloc(1, struct xkb_controls);
        if (!xkb->ctrls)
            return BadAlloc;
    }

    if (!xkb->ctrls->per_key_repeat) {
        xkb->ctrls->per_key_repeat = uTypedCalloc(xkb->max_key_code << 3,
                                                  unsigned char);
        if (!xkb->ctrls->per_key_repeat)
            return BadAlloc;
    }

    return Success;
}

static void
XkbcFreeControls(struct xkb_desc * xkb)
{
    if (xkb && xkb->ctrls) {
        free(xkb->ctrls->per_key_repeat);
        free(xkb->ctrls);
        xkb->ctrls = NULL;
    }
}

int
XkbcAllocIndicatorMaps(struct xkb_desc * xkb)
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
XkbcFreeIndicatorMaps(struct xkb_desc * xkb)
{
    if (xkb) {
        free(xkb->indicators);
        xkb->indicators = NULL;
    }
}

struct xkb_desc *
XkbcAllocKeyboard(void)
{
    struct xkb_desc *xkb;

    xkb = uTypedCalloc(1, struct xkb_desc);
    if (xkb) {
        xkb->device_spec = XkbUseCoreKbd;
        xkb->refcnt = 1;
    }

    return xkb;
}

void
XkbcFreeKeyboard(struct xkb_desc * xkb)
{
    if (!xkb)
        return;

    XkbcFreeClientMap(xkb);
    XkbcFreeServerMap(xkb);
    XkbcFreeCompatMap(xkb);
    XkbcFreeIndicatorMaps(xkb);
    XkbcFreeNames(xkb);
    XkbcFreeControls(xkb);
    free(xkb);
}
