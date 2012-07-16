/************************************************************
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
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
 *
 ********************************************************/

#include "indicators.h"
#include "expr.h"
#include "action.h"

/***====================================================================***/

static inline bool
ReportIndicatorBadType(struct xkb_keymap *keymap, LEDInfo *led,
                       const char *field, const char *wanted)
{
    return ReportBadType("indicator map", field,
                         xkb_atom_text(keymap->ctx, led->name), wanted);
}

static inline bool
ReportIndicatorNotArray(struct xkb_keymap *keymap, LEDInfo *led,
                        const char *field)
{
    return ReportNotArray("indicator map", field,
                          xkb_atom_text(keymap->ctx, led->name));
}

/***====================================================================***/

void
ClearIndicatorMapInfo(struct xkb_context *ctx, LEDInfo * info)
{
    info->name = xkb_atom_intern(ctx, "default");
    info->indicator = _LED_NotBound;
    info->flags = info->which_mods = info->real_mods = 0;
    info->vmods = 0;
    info->which_groups = info->groups = 0;
    info->ctrls = 0;
}

LEDInfo *
AddIndicatorMap(struct xkb_keymap *keymap, LEDInfo *oldLEDs, LEDInfo *new)
{
    LEDInfo *old, *last;
    unsigned collide;

    last = NULL;
    for (old = oldLEDs; old != NULL; old = (LEDInfo *) old->defs.next) {
        if (old->name == new->name) {
            if ((old->real_mods == new->real_mods) &&
                (old->vmods == new->vmods) &&
                (old->groups == new->groups) &&
                (old->ctrls == new->ctrls) &&
                (old->which_mods == new->which_mods) &&
                (old->which_groups == new->which_groups)) {
                old->defs.defined |= new->defs.defined;
                return oldLEDs;
            }
            if (new->defs.merge == MERGE_REPLACE) {
                CommonInfo *next = old->defs.next;
                if (((old->defs.file_id == new->defs.file_id)
                     && (warningLevel > 0)) || (warningLevel > 9)) {
                    WARN("Map for indicator %s redefined\n",
                         xkb_atom_text(keymap->ctx, old->name));
                    ACTION("Earlier definition ignored\n");
                }
                *old = *new;
                old->defs.next = next;
                return oldLEDs;
            }
            collide = 0;
            if (UseNewField(_LED_Index, &old->defs, &new->defs, &collide)) {
                old->indicator = new->indicator;
                old->defs.defined |= _LED_Index;
            }
            if (UseNewField(_LED_Mods, &old->defs, &new->defs, &collide)) {
                old->which_mods = new->which_mods;
                old->real_mods = new->real_mods;
                old->vmods = new->vmods;
                old->defs.defined |= _LED_Mods;
            }
            if (UseNewField(_LED_Groups, &old->defs, &new->defs, &collide)) {
                old->which_groups = new->which_groups;
                old->groups = new->groups;
                old->defs.defined |= _LED_Groups;
            }
            if (UseNewField(_LED_Ctrls, &old->defs, &new->defs, &collide)) {
                old->ctrls = new->ctrls;
                old->defs.defined |= _LED_Ctrls;
            }
            if (UseNewField(_LED_Explicit, &old->defs, &new->defs,
                            &collide)) {
                old->flags &= ~XkbIM_NoExplicit;
                old->flags |= (new->flags & XkbIM_NoExplicit);
                old->defs.defined |= _LED_Explicit;
            }
            if (UseNewField(_LED_Automatic, &old->defs, &new->defs,
                            &collide)) {
                old->flags &= ~XkbIM_NoAutomatic;
                old->flags |= (new->flags & XkbIM_NoAutomatic);
                old->defs.defined |= _LED_Automatic;
            }
            if (UseNewField(_LED_DrivesKbd, &old->defs, &new->defs,
                            &collide)) {
                old->flags &= ~XkbIM_LEDDrivesKB;
                old->flags |= (new->flags & XkbIM_LEDDrivesKB);
                old->defs.defined |= _LED_DrivesKbd;
            }
            if (collide) {
                WARN("Map for indicator %s redefined\n",
                     xkb_atom_text(keymap->ctx, old->name));
                ACTION("Using %s definition for duplicate fields\n",
                       (new->defs.merge == MERGE_AUGMENT ? "first" : "last"));
            }
            return oldLEDs;
        }
        if (old->defs.next == NULL)
            last = old;
    }
    /* new definition */
    old = uTypedAlloc(LEDInfo);
    if (!old) {
        WSGO("Couldn't allocate indicator map\n");
        ACTION("Map for indicator %s not compiled\n",
               xkb_atom_text(keymap->ctx, new->name));
        return NULL;
    }
    *old = *new;
    old->defs.next = NULL;
    if (last) {
        last->defs.next = &old->defs;
        return oldLEDs;
    }
    return old;
}

static const LookupEntry modComponentNames[] = {
    { "base", XkbIM_UseBase },
    { "latched", XkbIM_UseLatched },
    { "locked", XkbIM_UseLocked },
    { "effective", XkbIM_UseEffective },
    { "compat", XkbIM_UseCompat },
    { "any", XkbIM_UseAnyMods },
    { "none", 0 },
    { NULL, 0 }
};
static const LookupEntry groupComponentNames[] = {
    { "base", XkbIM_UseBase },
    { "latched", XkbIM_UseLatched },
    { "locked", XkbIM_UseLocked },
    { "effective", XkbIM_UseEffective },
    { "any", XkbIM_UseAnyGroup },
    { "none", 0 },
    { NULL, 0 }
};

static const LookupEntry groupNames[] = {
    { "group1", 0x01 },
    { "group2", 0x02 },
    { "group3", 0x04 },
    { "group4", 0x08 },
    { "group5", 0x10 },
    { "group6", 0x20 },
    { "group7", 0x40 },
    { "group8", 0x80 },
    { "none", 0x00 },
    { "all", 0xff },
    { NULL, 0 }
};

int
SetIndicatorMapField(LEDInfo *led, struct xkb_keymap *keymap,
                     char *field, ExprDef *arrayNdx, ExprDef *value)
{
    ExprResult rtrn;
    bool ok;

    ok = true;
    if ((strcasecmp(field, "modifiers") == 0) ||
        (strcasecmp(field, "mods") == 0)) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);
        if (!ExprResolveVModMask(value, &rtrn, keymap))
            return ReportIndicatorBadType(keymap, led, field, "modifier mask");
        led->real_mods = rtrn.uval & 0xff;
        led->vmods = (rtrn.uval >> 8) & 0xff;
        led->defs.defined |= _LED_Mods;
    }
    else if (strcasecmp(field, "groups") == 0) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);
        if (!ExprResolveMask(keymap->ctx, value, &rtrn, groupNames))
            return ReportIndicatorBadType(keymap, led, field, "group mask");
        led->groups = rtrn.uval;
        led->defs.defined |= _LED_Groups;
    }
    else if ((strcasecmp(field, "controls") == 0) ||
             (strcasecmp(field, "ctrls") == 0)) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);
        if (!ExprResolveMask(keymap->ctx, value, &rtrn, ctrlNames))
            return ReportIndicatorBadType(keymap, led, field,
                                          "controls mask");
        led->ctrls = rtrn.uval;
        led->defs.defined |= _LED_Ctrls;
    }
    else if (strcasecmp(field, "allowexplicit") == 0) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);
        if (!ExprResolveBoolean(keymap->ctx, value, &rtrn))
            return ReportIndicatorBadType(keymap, led, field, "boolean");
        if (rtrn.uval)
            led->flags &= ~XkbIM_NoExplicit;
        else
            led->flags |= XkbIM_NoExplicit;
        led->defs.defined |= _LED_Explicit;
    }
    else if ((strcasecmp(field, "whichmodstate") == 0) ||
             (strcasecmp(field, "whichmodifierstate") == 0)) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);
        if (!ExprResolveMask(keymap->ctx, value, &rtrn, modComponentNames)) {
            return ReportIndicatorBadType(keymap, led, field,
                                          "mask of modifier state components");
        }
        led->which_mods = rtrn.uval;
    }
    else if (strcasecmp(field, "whichgroupstate") == 0) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);
        if (!ExprResolveMask(keymap->ctx, value, &rtrn,
                             groupComponentNames)) {
            return ReportIndicatorBadType(keymap, led, field,
                                          "mask of group state components");
        }
        led->which_groups = rtrn.uval;
    }
    else if ((strcasecmp(field, "driveskbd") == 0) ||
             (strcasecmp(field, "driveskeyboard") == 0) ||
             (strcasecmp(field, "leddriveskbd") == 0) ||
             (strcasecmp(field, "leddriveskeyboard") == 0) ||
             (strcasecmp(field, "indicatordriveskbd") == 0) ||
             (strcasecmp(field, "indicatordriveskeyboard") == 0)) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);
        if (!ExprResolveBoolean(keymap->ctx, value, &rtrn))
            return ReportIndicatorBadType(keymap, led, field, "boolean");
        if (rtrn.uval)
            led->flags |= XkbIM_LEDDrivesKB;
        else
            led->flags &= ~XkbIM_LEDDrivesKB;
        led->defs.defined |= _LED_DrivesKbd;
    }
    else if (strcasecmp(field, "index") == 0) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);
        if (!ExprResolveInteger(keymap->ctx, value, &rtrn))
            return ReportIndicatorBadType(keymap, led, field,
                                          "indicator index");
        if ((rtrn.uval < 1) || (rtrn.uval > 32)) {
            ERROR("Illegal indicator index %d (range 1..%d)\n",
                  rtrn.uval, XkbNumIndicators);
            ACTION("Index definition for %s indicator ignored\n",
                   xkb_atom_text(keymap->ctx, led->name));
            return false;
        }
        led->indicator = rtrn.uval;
        led->defs.defined |= _LED_Index;
    }
    else {
        ERROR("Unknown field %s in map for %s indicator\n", field,
              xkb_atom_text(keymap->ctx, led->name));
        ACTION("Definition ignored\n");
        ok = false;
    }
    return ok;
}

LEDInfo *
HandleIndicatorMapDef(IndicatorMapDef *def, struct xkb_keymap *keymap,
                      LEDInfo *dflt, LEDInfo *oldLEDs, enum merge_mode merge)
{
    LEDInfo led, *rtrn;
    VarDef *var;
    bool ok;

    if (def->merge != MERGE_DEFAULT)
        merge = def->merge;

    led = *dflt;
    led.defs.merge = merge;
    led.name = def->name;

    ok = true;
    for (var = def->body; var != NULL; var = (VarDef *) var->common.next) {
        ExprResult elem, field;
        ExprDef *arrayNdx;
        if (!ExprResolveLhs(keymap, var->name, &elem, &field, &arrayNdx)) {
            ok = false;
            continue;
        }
        if (elem.str != NULL) {
            ERROR
                ("Cannot set defaults for \"%s\" element in indicator map\n",
                elem.str);
            ACTION("Assignment to %s.%s ignored\n", elem.str, field.str);
            ok = false;
        }
        else {
            ok = SetIndicatorMapField(&led, keymap, field.str, arrayNdx,
                                      var->value) && ok;
        }
        free(elem.str);
        free(field.str);
    }
    if (ok) {
        rtrn = AddIndicatorMap(keymap, oldLEDs, &led);
        return rtrn;
    }
    return NULL;
}

static void
BindIndicators(struct xkb_keymap *keymap, LEDInfo *unbound)
{
    int i;
    LEDInfo *led, *next, *last;

    for (led = unbound; led != NULL; led = (LEDInfo *) led->defs.next) {
        if (led->indicator == _LED_NotBound) {
            for (i = 0; i < XkbNumIndicators; i++) {
                if (keymap->indicator_names[i] &&
                    strcmp(keymap->indicator_names[i],
                           xkb_atom_text(keymap->ctx, led->name)) == 0) {
                    led->indicator = i + 1;
                    break;
                }
            }
        }
    }

    for (led = unbound; led != NULL; led = (LEDInfo *) led->defs.next) {
        if (led->indicator == _LED_NotBound) {
            for (i = 0; i < XkbNumIndicators; i++) {
                if (keymap->indicator_names[i] == NULL) {
                    keymap->indicator_names[i] =
                        xkb_atom_strdup(keymap->ctx, led->name);
                    led->indicator = i + 1;
                    break;
                }
            }
            if (led->indicator == _LED_NotBound) {
                ERROR("No unnamed indicators found\n");
                ACTION("Virtual indicator map \"%s\" not bound\n",
                       xkb_atom_text(keymap->ctx, led->name));
                continue;
            }
        }
    }
    for (last = NULL, led = unbound; led != NULL; led = next) {
        next = (LEDInfo *) led->defs.next;
        if (led->indicator == _LED_NotBound) {
            unbound = next;
            free(led);
        }
        else {
            if (strcmp(keymap->indicator_names[led->indicator - 1],
                       xkb_atom_text(keymap->ctx, led->name)) != 0) {
                const char *old = keymap->indicator_names[led->indicator - 1];
                ERROR("Multiple names bound to indicator %d\n",
                      (unsigned int) led->indicator);
                ACTION("Using %s, ignoring %s\n", old,
                       xkb_atom_text(keymap->ctx, led->name));
                led->indicator = _LED_NotBound;
                unbound = next;
                free(led);
            }
            else {
                struct xkb_indicator_map * map;
                map = &keymap->indicators[led->indicator - 1];
                map->flags = led->flags;
                map->which_groups = led->which_groups;
                map->groups = led->groups;
                map->which_mods = led->which_mods;
                map->mods.mask = led->real_mods;
                map->mods.real_mods = led->real_mods;
                map->mods.vmods = led->vmods;
                map->ctrls = led->ctrls;
                if (last)
                    last->defs.next = &next->defs;
                else
                    unbound = next;
                led->defs.next = NULL;
                free(led);
            }
        }
    }

    for (led = unbound; led != NULL; led = next) {
        next = led ? (LEDInfo *) led->defs.next : NULL;
        free(led);
    }
}

bool
CopyIndicatorMapDefs(struct xkb_keymap *keymap, LEDInfo *leds)
{
    LEDInfo *led, *next;
    LEDInfo *unbound = NULL, *last = NULL;

    for (led = leds; led != NULL; led = next) {
        next = (LEDInfo *) led->defs.next;
        if ((led->groups != 0) && (led->which_groups == 0))
            led->which_groups = XkbIM_UseEffective;
        if ((led->which_mods == 0) && ((led->real_mods) || (led->vmods)))
            led->which_mods = XkbIM_UseEffective;
        if ((led->indicator == _LED_NotBound) || (!keymap->indicators)) {
            led->defs.next = NULL;
            if (last != NULL)
                last->defs.next = (CommonInfo *) led;
            else
                unbound = led;
            last = led;
        }
        else {
            struct xkb_indicator_map * im;
            im = &keymap->indicators[led->indicator - 1];
            im->flags = led->flags;
            im->which_groups = led->which_groups;
            im->groups = led->groups;
            im->which_mods = led->which_mods;
            im->mods.mask = led->real_mods;
            im->mods.real_mods = led->real_mods;
            im->mods.vmods = led->vmods;
            im->ctrls = led->ctrls;
            free(keymap->indicator_names[led->indicator - 1]);
            keymap->indicator_names[led->indicator - 1] =
                xkb_atom_strdup(keymap->ctx, led->name);
            free(led);
        }
    }

    if (unbound)
        BindIndicators(keymap, unbound);

    return true;
}
