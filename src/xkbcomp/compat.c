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

#include "xkbcomp-priv.h"
#include "parseutils.h"
#include "action.h"
#include "vmod.h"

typedef struct _SymInterpInfo {
    unsigned short defined;
    unsigned file_id;
    enum merge_mode merge;
    struct list entry;

    struct xkb_sym_interpret interp;
} SymInterpInfo;

#define _SI_VirtualMod   (1 << 0)
#define _SI_Action       (1 << 1)
#define _SI_AutoRepeat   (1 << 2)
#define _SI_LockingKey   (1 << 3)
#define _SI_LevelOneOnly (1 << 4)

typedef struct _LEDInfo {
    unsigned short defined;
    unsigned file_id;
    enum merge_mode merge;
    struct list entry;

    xkb_atom_t name;
    unsigned char indicator;
    unsigned char flags;
    unsigned char which_mods;
    unsigned char real_mods;
    unsigned short vmods;
    unsigned char which_groups;
    uint32_t groups;
    unsigned int ctrls;
} LEDInfo;

#define _LED_Index      (1 << 0)
#define _LED_Mods       (1 << 1)
#define _LED_Groups     (1 << 2)
#define _LED_Ctrls      (1 << 3)
#define _LED_Explicit   (1 << 4)
#define _LED_Automatic  (1 << 5)
#define _LED_DrivesKbd  (1 << 6)

#define _LED_NotBound   255

typedef struct _GroupCompatInfo {
    unsigned file_id;
    enum merge_mode merge;
    bool defined;
    unsigned char real_mods;
    xkb_atom_t vmods;
} GroupCompatInfo;

typedef struct _CompatInfo {
    char *name;
    unsigned file_id;
    int errorCount;
    int nInterps;
    struct list interps;
    SymInterpInfo dflt;
    LEDInfo ledDflt;
    GroupCompatInfo groupCompat[XkbNumKbdGroups];
    struct list leds;
    VModInfo vmods;
    ActionInfo *act;
    struct xkb_keymap *keymap;
} CompatInfo;

static const char *
siText(SymInterpInfo * si, CompatInfo * info)
{
    static char buf[128];

    if (si == &info->dflt) {
        snprintf(buf, sizeof(buf), "default");
    }
    else {
        snprintf(buf, sizeof(buf), "%s+%s(%s)",
                 XkbcKeysymText(si->interp.sym),
                 XkbcSIMatchText(si->interp.match),
                 XkbcModMaskText(si->interp.mods, false));
    }
    return buf;
}

static inline bool
ReportSINotArray(SymInterpInfo *si, const char *field, CompatInfo *info)
{
    return ReportNotArray("symbol interpretation", field, siText(si, info));
}

static inline bool
ReportSIBadType(SymInterpInfo *si, const char *field, const char *wanted,
                CompatInfo *info)
{
    return ReportBadType("symbol interpretation", field, siText(si, info),
                         wanted);
}

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

static void
ClearIndicatorMapInfo(struct xkb_context *ctx, LEDInfo * info)
{
    info->name = xkb_atom_intern(ctx, "default");
    info->indicator = _LED_NotBound;
    info->flags = info->which_mods = info->real_mods = 0;
    info->vmods = 0;
    info->which_groups = info->groups = 0;
    info->ctrls = 0;
}

static void
InitCompatInfo(CompatInfo *info, struct xkb_keymap *keymap, unsigned file_id)
{
    unsigned int i;

    info->keymap = keymap;
    info->name = NULL;
    info->file_id = file_id;
    info->errorCount = 0;
    info->nInterps = 0;
    list_init(&info->interps);
    info->act = NULL;
    info->dflt.file_id = file_id;
    info->dflt.defined = 0;
    info->dflt.merge = MERGE_OVERRIDE;
    info->dflt.interp.flags = 0;
    info->dflt.interp.virtual_mod = XkbNoModifier;
    info->dflt.interp.act.type = XkbSA_NoAction;
    for (i = 0; i < sizeof(info->dflt.interp.act.any.data); i++)
        info->dflt.interp.act.any.data[i] = 0;
    ClearIndicatorMapInfo(keymap->ctx, &info->ledDflt);
    info->ledDflt.file_id = file_id;
    info->ledDflt.defined = 0;
    info->ledDflt.merge = MERGE_OVERRIDE;
    memset(&info->groupCompat[0], 0,
           XkbNumKbdGroups * sizeof(GroupCompatInfo));
    list_init(&info->leds);
    InitVModInfo(&info->vmods, keymap);
}

static void
ClearCompatInfo(CompatInfo *info, struct xkb_keymap *keymap)
{
    unsigned int i;
    ActionInfo *next_act;
    SymInterpInfo *si, *next_si;
    LEDInfo *led, *next_led;

    free(info->name);
    info->name = NULL;
    info->dflt.defined = 0;
    info->dflt.merge = MERGE_AUGMENT;
    info->dflt.interp.flags = 0;
    info->dflt.interp.virtual_mod = XkbNoModifier;
    info->dflt.interp.act.type = XkbSA_NoAction;
    for (i = 0; i < sizeof(info->dflt.interp.act.any.data); i++)
        info->dflt.interp.act.any.data[i] = 0;
    ClearIndicatorMapInfo(keymap->ctx, &info->ledDflt);
    info->nInterps = 0;
    list_foreach_safe(si, next_si, &info->interps, entry)
        free(si);
    memset(&info->groupCompat[0], 0,
           XkbNumKbdGroups * sizeof(GroupCompatInfo));
    list_foreach_safe(led, next_led, &info->leds, entry)
        free(led);
    while (info->act) {
        next_act = info->act->next;
        free(info->act);
        info->act = next_act;
    }
    ClearVModInfo(&info->vmods, keymap);
}

static SymInterpInfo *
NextInterp(CompatInfo * info)
{
    SymInterpInfo *si;

    si = calloc(1, sizeof(*si));
    if (!si)
        return NULL;

    list_append(&si->entry, &info->interps);
    info->nInterps++;

    return si;
}

static SymInterpInfo *
FindMatchingInterp(CompatInfo * info, SymInterpInfo * new)
{
    SymInterpInfo *old;

    list_foreach(old, &info->interps, entry)
        if (old->interp.sym == new->interp.sym &&
            old->interp.mods == new->interp.mods &&
            old->interp.match == new->interp.match)
            return old;

    return NULL;
}

static bool
AddInterp(CompatInfo * info, SymInterpInfo * new)
{
    unsigned collide;
    SymInterpInfo *old;
    struct list entry;

    collide = 0;
    old = FindMatchingInterp(info, new);
    if (old != NULL) {
        if (new->merge == MERGE_REPLACE) {
            entry = old->entry;
            if ((old->file_id == new->file_id && warningLevel > 0) ||
                warningLevel > 9) {
                WARN("Multiple definitions for \"%s\"\n", siText(new, info));
                ACTION("Earlier interpretation ignored\n");
            }
            *old = *new;
            old->entry = entry;
            return true;
        }

        if (use_new_field(_SI_VirtualMod, old->defined, old->file_id,
                          new->defined, new->file_id, new->merge, &collide)) {
            old->interp.virtual_mod = new->interp.virtual_mod;
            old->defined |= _SI_VirtualMod;
        }
        if (use_new_field(_SI_Action, old->defined, old->file_id, new->defined,
                          new->file_id, new->merge, &collide)) {
            old->interp.act = new->interp.act;
            old->defined |= _SI_Action;
        }
        if (use_new_field(_SI_AutoRepeat, old->defined, old->file_id,
                          new->defined, new->file_id, new->merge, &collide)) {
            old->interp.flags &= ~XkbSI_AutoRepeat;
            old->interp.flags |= (new->interp.flags & XkbSI_AutoRepeat);
            old->defined |= _SI_AutoRepeat;
        }
        if (use_new_field(_SI_LockingKey, old->defined, old->file_id,
                          new->defined, new->file_id, new->merge, &collide)) {
            old->interp.flags &= ~XkbSI_LockingKey;
            old->interp.flags |= (new->interp.flags & XkbSI_LockingKey);
            old->defined |= _SI_LockingKey;
        }
        if (use_new_field(_SI_LevelOneOnly, old->defined, old->file_id,
                          new->defined, new->file_id, new->merge, &collide)) {
            old->interp.match &= ~XkbSI_LevelOneOnly;
            old->interp.match |= (new->interp.match & XkbSI_LevelOneOnly);
            old->defined |= _SI_LevelOneOnly;
        }

        if (collide) {
            WARN("Multiple interpretations of \"%s\"\n", siText(new, info));
            ACTION("Using %s definition for duplicate fields\n",
                   (new->merge != MERGE_AUGMENT ? "last" : "first"));
        }

        return true;
    }

    old = new;
    if ((new = NextInterp(info)) == NULL)
        return false;
    entry = new->entry;
    *new = *old;
    new->entry = entry;
    return true;
}

static bool
AddGroupCompat(CompatInfo *info, xkb_group_index_t group, GroupCompatInfo *new)
{
    GroupCompatInfo *gc;

    gc = &info->groupCompat[group];
    if (gc->real_mods == new->real_mods && gc->vmods == new->vmods)
        return true;

    if ((gc->file_id == new->file_id && warningLevel > 0) ||
        warningLevel > 9) {
        WARN("Compat map for group %u redefined\n", group + 1);
        ACTION("Using %s definition\n",
               (new->merge == MERGE_AUGMENT ? "old" : "new"));
    }

    if (new->defined && (new->merge != MERGE_AUGMENT || !gc->defined))
        *gc = *new;

    return true;
}

/***====================================================================***/

static bool
ResolveStateAndPredicate(ExprDef * expr,
                         unsigned *pred_rtrn,
                         unsigned *mods_rtrn, CompatInfo * info)
{
    ExprResult result;

    if (expr == NULL) {
        *pred_rtrn = XkbSI_AnyOfOrNone;
        *mods_rtrn = ~0;
        return true;
    }

    *pred_rtrn = XkbSI_Exactly;
    if (expr->op == ExprActionDecl) {
        const char *pred_txt = xkb_atom_text(info->keymap->ctx,
                                             expr->value.action.name);
        if (strcasecmp(pred_txt, "noneof") == 0)
            *pred_rtrn = XkbSI_NoneOf;
        else if (strcasecmp(pred_txt, "anyofornone") == 0)
            *pred_rtrn = XkbSI_AnyOfOrNone;
        else if (strcasecmp(pred_txt, "anyof") == 0)
            *pred_rtrn = XkbSI_AnyOf;
        else if (strcasecmp(pred_txt, "allof") == 0)
            *pred_rtrn = XkbSI_AllOf;
        else if (strcasecmp(pred_txt, "exactly") == 0)
            *pred_rtrn = XkbSI_Exactly;
        else {
            ERROR("Illegal modifier predicate \"%s\"\n", pred_txt);
            ACTION("Ignored\n");
            return false;
        }
        expr = expr->value.action.args;
    }
    else if (expr->op == ExprIdent) {
        const char *pred_txt = xkb_atom_text(info->keymap->ctx,
                                             expr->value.str);
        if ((pred_txt) && (strcasecmp(pred_txt, "any") == 0)) {
            *pred_rtrn = XkbSI_AnyOf;
            *mods_rtrn = 0xff;
            return true;
        }
    }

    if (ExprResolveModMask(info->keymap->ctx, expr, &result)) {
        *mods_rtrn = result.uval;
        return true;
    }
    return false;
}

/***====================================================================***/

static bool
AddIndicatorMap(struct xkb_keymap *keymap, CompatInfo *info, LEDInfo *new)
{
    LEDInfo *old;
    unsigned collide;

    list_foreach(old, &info->leds, entry) {
        if (old->name == new->name) {
            if ((old->real_mods == new->real_mods) &&
                (old->vmods == new->vmods) &&
                (old->groups == new->groups) &&
                (old->ctrls == new->ctrls) &&
                (old->which_mods == new->which_mods) &&
                (old->which_groups == new->which_groups)) {
                old->defined |= new->defined;
                return true;
            }

            if (new->merge == MERGE_REPLACE) {
                struct list entry = old->entry;
                if (((old->file_id == new->file_id)
                     && (warningLevel > 0)) || (warningLevel > 9)) {
                    WARN("Map for indicator %s redefined\n",
                          xkb_atom_text(keymap->ctx, old->name));
                    ACTION("Earlier definition ignored\n");
                }
                *old = *new;
                old->entry = entry;
                return true;
            }

            collide = 0;
            if (use_new_field(_LED_Index, old->defined, old->file_id,
                              new->defined, new->file_id, new->merge,
                              &collide)) {
                old->indicator = new->indicator;
                old->defined |= _LED_Index;
            }
            if (use_new_field(_LED_Mods, old->defined, old->file_id,
                              new->defined, new->file_id, new->merge,
                              &collide)) {
                old->which_mods = new->which_mods;
                old->real_mods = new->real_mods;
                old->vmods = new->vmods;
                old->defined |= _LED_Mods;
            }
            if (use_new_field(_LED_Groups, old->defined, old->file_id,
                              new->defined, new->file_id, new->merge,
                              &collide)) {
                old->which_groups = new->which_groups;
                old->groups = new->groups;
                old->defined |= _LED_Groups;
            }
            if (use_new_field(_LED_Ctrls, old->defined, old->file_id,
                              new->defined, new->file_id, new->merge,
                              &collide)) {
                old->ctrls = new->ctrls;
                old->defined |= _LED_Ctrls;
            }
            if (use_new_field(_LED_Explicit, old->defined, old->file_id,
                              new->defined, new->file_id, new->merge,
                              &collide)) {
                old->flags &= ~XkbIM_NoExplicit;
                old->flags |= (new->flags & XkbIM_NoExplicit);
                old->defined |= _LED_Explicit;
            }
            if (use_new_field(_LED_Automatic, old->defined, old->file_id,
                              new->defined, new->file_id, new->merge,
                              &collide)) {
                old->flags &= ~XkbIM_NoAutomatic;
                old->flags |= (new->flags & XkbIM_NoAutomatic);
                old->defined |= _LED_Automatic;
            }
            if (use_new_field(_LED_DrivesKbd, old->defined, old->file_id,
                              new->defined, new->file_id, new->merge,
                              &collide)) {
                old->flags &= ~XkbIM_LEDDrivesKB;
                old->flags |= (new->flags & XkbIM_LEDDrivesKB);
                old->defined |= _LED_DrivesKbd;
            }

            if (collide) {
                WARN("Map for indicator %s redefined\n",
                     xkb_atom_text(keymap->ctx, old->name));
                ACTION("Using %s definition for duplicate fields\n",
                        (new->merge == MERGE_AUGMENT ? "first" : "last"));
            }

            return true;
        }
    }

    /* new definition */
    old = malloc(sizeof(*old));
    if (!old) {
        WSGO("Couldn't allocate indicator map\n");
        ACTION("Map for indicator %s not compiled\n",
               xkb_atom_text(keymap->ctx, new->name));
        return false;
    }

    *old = *new;
    list_append(&old->entry, &info->leds);

    return true;
}

static void
MergeIncludedCompatMaps(CompatInfo * into, CompatInfo * from,
                        enum merge_mode merge)
{
    SymInterpInfo *si;
    LEDInfo *led, *next_led;
    GroupCompatInfo *gcm;
    xkb_group_index_t i;

    if (from->errorCount > 0) {
        into->errorCount += from->errorCount;
        return;
    }
    if (into->name == NULL) {
        into->name = from->name;
        from->name = NULL;
    }

    list_foreach(si, &from->interps, entry) {
        if (merge != MERGE_DEFAULT)
            si->merge = merge;
        if (!AddInterp(into, si))
            into->errorCount++;
    }

    for (i = 0, gcm = &from->groupCompat[0]; i < XkbNumKbdGroups;
         i++, gcm++) {
        if (merge != MERGE_DEFAULT)
            gcm->merge = merge;
        if (!AddGroupCompat(into, i, gcm))
            into->errorCount++;
    }

    list_foreach_safe(led, next_led, &from->leds, entry) {
        led->merge = (merge == MERGE_DEFAULT ? led->merge : merge);
        if (!AddIndicatorMap(from->keymap, into, led))
            into->errorCount++;
    }
}

static void
HandleCompatMapFile(XkbFile *file, struct xkb_keymap *keymap,
                    enum merge_mode merge,
                    CompatInfo *info);

static bool
HandleIncludeCompatMap(IncludeStmt *stmt, struct xkb_keymap *keymap,
                       CompatInfo *info)
{
    enum merge_mode merge = MERGE_DEFAULT;
    XkbFile *rtrn;
    CompatInfo included, next_incl;

    InitCompatInfo(&included, keymap, info->file_id);
    if (stmt->stmt) {
        free(included.name);
        included.name = stmt->stmt;
        stmt->stmt = NULL;
    }

    for (; stmt; stmt = stmt->next) {
        if (!ProcessIncludeFile(keymap->ctx, stmt, FILE_TYPE_COMPAT,
                                &rtrn, &merge)) {
            info->errorCount += 10;
            ClearCompatInfo(&included, keymap);
            return false;
        }

        InitCompatInfo(&next_incl, keymap, rtrn->id);
        next_incl.file_id = rtrn->id;
        next_incl.dflt = info->dflt;
        next_incl.dflt.file_id = rtrn->id;
        next_incl.dflt.merge = merge;
        next_incl.ledDflt.file_id = rtrn->id;
        next_incl.ledDflt.merge = merge;
        next_incl.act = info->act;

        HandleCompatMapFile(rtrn, keymap, MERGE_OVERRIDE, &next_incl);

        MergeIncludedCompatMaps(&included, &next_incl, merge);
        if (info->act)
            next_incl.act = NULL;

        ClearCompatInfo(&next_incl, keymap);
        FreeXKBFile(rtrn);
    }

    MergeIncludedCompatMaps(info, &included, merge);
    ClearCompatInfo(&included, keymap);

    return (info->errorCount == 0);
}

static const LookupEntry useModMapValues[] = {
    { "levelone", 1 },
    { "level1", 1 },
    { "anylevel", 0 },
    { "any", 0 },
    { NULL, 0 }
};

static int
SetInterpField(SymInterpInfo *si, struct xkb_keymap *keymap, char *field,
               ExprDef *arrayNdx, ExprDef *value, CompatInfo *info)
{
    int ok = 1;
    ExprResult tmp;

    if (strcasecmp(field, "action") == 0) {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = HandleActionDef(value, keymap, &si->interp.act.any, info->act);
        if (ok)
            si->defined |= _SI_Action;
    }
    else if ((strcasecmp(field, "virtualmodifier") == 0) ||
             (strcasecmp(field, "virtualmod") == 0)) {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = ResolveVirtualModifier(value, keymap, &tmp, &info->vmods);
        if (ok) {
            si->interp.virtual_mod = tmp.uval;
            si->defined |= _SI_VirtualMod;
        }
        else
            return ReportSIBadType(si, field, "virtual modifier", info);
    }
    else if (strcasecmp(field, "repeat") == 0) {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = ExprResolveBoolean(keymap->ctx, value, &tmp);
        if (ok) {
            if (tmp.uval)
                si->interp.flags |= XkbSI_AutoRepeat;
            else
                si->interp.flags &= ~XkbSI_AutoRepeat;
            si->defined |= _SI_AutoRepeat;
        }
        else
            return ReportSIBadType(si, field, "boolean", info);
    }
    else if (strcasecmp(field, "locking") == 0) {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = ExprResolveBoolean(keymap->ctx, value, &tmp);
        if (ok) {
            if (tmp.uval)
                si->interp.flags |= XkbSI_LockingKey;
            else
                si->interp.flags &= ~XkbSI_LockingKey;
            si->defined |= _SI_LockingKey;
        }
        else
            return ReportSIBadType(si, field, "boolean", info);
    }
    else if ((strcasecmp(field, "usemodmap") == 0) ||
             (strcasecmp(field, "usemodmapmods") == 0)) {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = ExprResolveEnum(keymap->ctx, value, &tmp, useModMapValues);
        if (ok) {
            if (tmp.uval)
                si->interp.match |= XkbSI_LevelOneOnly;
            else
                si->interp.match &= ~XkbSI_LevelOneOnly;
            si->defined |= _SI_LevelOneOnly;
        }
        else
            return ReportSIBadType(si, field, "level specification", info);
    }
    else {
        ok = ReportBadField("symbol interpretation", field, siText(si, info));
    }
    return ok;
}

static const LookupEntry modComponentNames[] = {
    {"base", XkbIM_UseBase},
    {"latched", XkbIM_UseLatched},
    {"locked", XkbIM_UseLocked},
    {"effective", XkbIM_UseEffective},
    {"compat", XkbIM_UseCompat},
    {"any", XkbIM_UseAnyMods},
    {"none", 0},
    {NULL, 0}
};
static const LookupEntry groupComponentNames[] = {
    {"base", XkbIM_UseBase},
    {"latched", XkbIM_UseLatched},
    {"locked", XkbIM_UseLocked},
    {"effective", XkbIM_UseEffective},
    {"any", XkbIM_UseAnyGroup},
    {"none", 0},
    {NULL, 0}
};

static const LookupEntry groupNames[] = {
    {"group1", 0x01},
    {"group2", 0x02},
    {"group3", 0x04},
    {"group4", 0x08},
    {"group5", 0x10},
    {"group6", 0x20},
    {"group7", 0x40},
    {"group8", 0x80},
    {"none", 0x00},
    {"all", 0xff},
    {NULL, 0}
};

static int
SetIndicatorMapField(LEDInfo *led, struct xkb_keymap *keymap,
                     char *field, ExprDef *arrayNdx, ExprDef *value)
{
    ExprResult rtrn;
    bool ok = true;

    if (strcasecmp(field, "modifiers") == 0 || strcasecmp(field, "mods") == 0) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);

        if (!ExprResolveVModMask(value, &rtrn, keymap))
            return ReportIndicatorBadType(keymap, led, field, "modifier mask");

        led->real_mods = rtrn.uval & 0xff;
        led->vmods = (rtrn.uval >> 8) & 0xff;
        led->defined |= _LED_Mods;
    }
    else if (strcasecmp(field, "groups") == 0) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);

        if (!ExprResolveMask(keymap->ctx, value, &rtrn, groupNames))
            return ReportIndicatorBadType(keymap, led, field, "group mask");

        led->groups = rtrn.uval;
        led->defined |= _LED_Groups;
    }
    else if (strcasecmp(field, "controls") == 0 ||
             strcasecmp(field, "ctrls") == 0) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);

        if (!ExprResolveMask(keymap->ctx, value, &rtrn, ctrlNames))
            return ReportIndicatorBadType(keymap, led, field,
                                          "controls mask");

        led->ctrls = rtrn.uval;
        led->defined |= _LED_Ctrls;
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
        led->defined |= _LED_Explicit;
    }
    else if (strcasecmp(field, "whichmodstate") == 0 ||
             strcasecmp(field, "whichmodifierstate") == 0) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);

        if (!ExprResolveMask(keymap->ctx, value, &rtrn, modComponentNames))
            return ReportIndicatorBadType(keymap, led, field,
                                          "mask of modifier state components");

        led->which_mods = rtrn.uval;
    }
    else if (strcasecmp(field, "whichgroupstate") == 0) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);

        if (!ExprResolveMask(keymap->ctx, value, &rtrn, groupComponentNames))
            return ReportIndicatorBadType(keymap, led, field,
                                          "mask of group state components");

        led->which_groups = rtrn.uval;
    }
    else if (strcasecmp(field, "driveskbd") == 0 ||
             strcasecmp(field, "driveskeyboard") == 0 ||
             strcasecmp(field, "leddriveskbd") == 0 ||
             strcasecmp(field, "leddriveskeyboard") == 0 ||
             strcasecmp(field, "indicatordriveskbd") == 0 ||
             strcasecmp(field, "indicatordriveskeyboard") == 0) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);

        if (!ExprResolveBoolean(keymap->ctx, value, &rtrn))
            return ReportIndicatorBadType(keymap, led, field, "boolean");

        if (rtrn.uval)
            led->flags |= XkbIM_LEDDrivesKB;
        else
            led->flags &= ~XkbIM_LEDDrivesKB;
        led->defined |= _LED_DrivesKbd;
    }
    else if (strcasecmp(field, "index") == 0) {
        if (arrayNdx != NULL)
            return ReportIndicatorNotArray(keymap, led, field);

        if (!ExprResolveInteger(keymap->ctx, value, &rtrn))
            return ReportIndicatorBadType(keymap, led, field,
                                          "indicator index");

        if (rtrn.uval < 1 || rtrn.uval > 32) {
            ERROR("Illegal indicator index %d (range 1..%d)\n",
                   rtrn.uval, XkbNumIndicators);
            ACTION("Index definition for %s indicator ignored\n",
                    xkb_atom_text(keymap->ctx, led->name));
            return false;
        }

        led->indicator = rtrn.uval;
        led->defined |= _LED_Index;
    }
    else {
        ERROR("Unknown field %s in map for %s indicator\n", field,
               xkb_atom_text(keymap->ctx, led->name));
        ACTION("Definition ignored\n");
        ok = false;
    }

    return ok;
}

static int
HandleInterpVar(VarDef * stmt, struct xkb_keymap *keymap, CompatInfo * info)
{
    ExprResult elem, field;
    ExprDef *ndx;
    int ret;

    if (ExprResolveLhs(keymap, stmt->name, &elem, &field, &ndx) == 0)
        ret = 0;               /* internal error, already reported */
    else if (elem.str && (strcasecmp(elem.str, "interpret") == 0))
        ret = SetInterpField(&info->dflt, keymap, field.str, ndx, stmt->value,
                             info);
    else if (elem.str && (strcasecmp(elem.str, "indicator") == 0))
        ret = SetIndicatorMapField(&info->ledDflt, keymap, field.str, ndx,
                                   stmt->value);
    else
        ret = SetActionField(keymap, elem.str, field.str, ndx, stmt->value,
                             &info->act);
    free(elem.str);
    free(field.str);
    return ret;
}

static int
HandleInterpBody(VarDef *def, struct xkb_keymap *keymap, SymInterpInfo *si,
                 CompatInfo *info)
{
    int ok = 1;
    ExprResult tmp, field;
    ExprDef *arrayNdx;

    for (; def != NULL; def = (VarDef *) def->common.next) {
        if ((def->name) && (def->name->type == ExprFieldRef)) {
            ok = HandleInterpVar(def, keymap, info);
            continue;
        }
        ok = ExprResolveLhs(keymap, def->name, &tmp, &field, &arrayNdx);
        if (ok) {
            ok = SetInterpField(si, keymap, field.str, arrayNdx, def->value,
                                info);
            free(field.str);
        }
    }
    return ok;
}

static int
HandleInterpDef(InterpDef *def, struct xkb_keymap *keymap,
                enum merge_mode merge,
                CompatInfo *info)
{
    unsigned pred, mods;
    SymInterpInfo si;

    if (!ResolveStateAndPredicate(def->match, &pred, &mods, info)) {
        ERROR("Couldn't determine matching modifiers\n");
        ACTION("Symbol interpretation ignored\n");
        return false;
    }
    if (def->merge != MERGE_DEFAULT)
        merge = def->merge;

    si = info->dflt;
    si.merge = merge;
    if (!LookupKeysym(def->sym, &si.interp.sym)) {
        ERROR("Could not resolve keysym %s\n", def->sym);
        ACTION("Symbol interpretation ignored\n");
        return false;
    }
    si.interp.match = pred & XkbSI_OpMask;
    si.interp.mods = mods;
    if (!HandleInterpBody(def->def, keymap, &si, info)) {
        info->errorCount++;
        return false;
    }

    if (!AddInterp(info, &si)) {
        info->errorCount++;
        return false;
    }
    return true;
}

static int
HandleGroupCompatDef(GroupCompatDef *def, struct xkb_keymap *keymap,
                     enum merge_mode merge, CompatInfo *info)
{
    ExprResult val;
    GroupCompatInfo tmp;

    if (def->merge != MERGE_DEFAULT)
        merge = def->merge;
    if (def->group < 1 || def->group > XkbNumKbdGroups) {
        ERROR("Keyboard group must be in the range 1..%u\n",
               XkbNumKbdGroups);
        ACTION("Compatibility map for illegal group %u ignored\n", def->group);
        return false;
    }
    tmp.file_id = info->file_id;
    tmp.merge = merge;
    if (!ExprResolveVModMask(def->def, &val, keymap)) {
        ERROR("Expected a modifier mask in group compatibility definition\n");
        ACTION("Ignoring illegal compatibility map for group %u\n",
               def->group);
        return false;
    }
    tmp.real_mods = val.uval & 0xff;
    tmp.vmods = (val.uval >> 8) & 0xffff;
    tmp.defined = true;
    return AddGroupCompat(info, def->group - 1, &tmp);
}

static bool
HandleIndicatorMapDef(IndicatorMapDef *def, struct xkb_keymap *keymap,
                      enum merge_mode merge, CompatInfo *info)
{
    LEDInfo led;
    VarDef *var;
    bool ok;

    if (def->merge != MERGE_DEFAULT)
        merge = def->merge;

    led = info->ledDflt;
    led.merge = merge;
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
            ERROR("Cannot set defaults for \"%s\" element in indicator map\n",
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

    if (ok)
        return AddIndicatorMap(keymap, info, &led);

    return false;
}

static void
HandleCompatMapFile(XkbFile *file, struct xkb_keymap *keymap,
                    enum merge_mode merge,
                    CompatInfo *info)
{
    ParseCommon *stmt;

    if (merge == MERGE_DEFAULT)
        merge = MERGE_AUGMENT;
    free(info->name);
    info->name = uDupString(file->name);
    stmt = file->defs;
    while (stmt)
    {
        switch (stmt->stmtType) {
        case StmtInclude:
            if (!HandleIncludeCompatMap((IncludeStmt *) stmt, keymap, info))
                info->errorCount++;
            break;
        case StmtInterpDef:
            if (!HandleInterpDef((InterpDef *) stmt, keymap, merge, info))
                info->errorCount++;
            break;
        case StmtGroupCompatDef:
            if (!HandleGroupCompatDef
                    ((GroupCompatDef *) stmt, keymap, merge, info))
                info->errorCount++;
            break;
        case StmtIndicatorMapDef:
            if (!HandleIndicatorMapDef((IndicatorMapDef *) stmt, keymap,
                                       merge, info))
                info->errorCount++;
            break;
        case StmtVarDef:
            if (!HandleInterpVar((VarDef *) stmt, keymap, info))
                info->errorCount++;
            break;
        case StmtVModDef:
            if (!HandleVModDef((VModDef *) stmt, keymap, merge, &info->vmods))
                info->errorCount++;
            break;
        case StmtKeycodeDef:
            ERROR("Interpretation files may not include other types\n");
            ACTION("Ignoring definition of key name\n");
            info->errorCount++;
            break;
        default:
            WSGO("Unexpected statement type %d in HandleCompatMapFile\n",
                 stmt->stmtType);
            break;
        }
        stmt = stmt->next;
        if (info->errorCount > 10) {
#ifdef NOISY
            ERROR("Too many errors\n");
#endif
            ACTION("Abandoning compatibility map \"%s\"\n", file->topName);
            break;
        }
    }
}

static void
CopyInterps(CompatInfo *info, struct xkb_keymap *keymap,
            bool needSymbol, unsigned pred)
{
    SymInterpInfo *si;

    list_foreach(si, &info->interps, entry) {
        if (((si->interp.match & XkbSI_OpMask) != pred) ||
            (needSymbol && si->interp.sym == XKB_KEY_NoSymbol) ||
            (!needSymbol && si->interp.sym != XKB_KEY_NoSymbol))
            continue;

        darray_append(keymap->sym_interpret, si->interp);
    }
}

static void
BindIndicators(struct xkb_keymap *keymap, struct list *unbound_leds)
{
    int i;
    LEDInfo *led, *next_led;
    struct xkb_indicator_map *map;

    list_foreach(led, unbound_leds, entry) {
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

    list_foreach(led, unbound_leds, entry) {
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

    list_foreach_safe(led, next_led, unbound_leds, entry) {
        if (led->indicator == _LED_NotBound) {
            free(led);
            continue;
        }

        if (strcmp(keymap->indicator_names[led->indicator - 1],
                   xkb_atom_text(keymap->ctx, led->name)) != 0) {
            const char *old = keymap->indicator_names[led->indicator - 1];
            ERROR("Multiple names bound to indicator %d\n", led->indicator);
            ACTION("Using %s, ignoring %s\n", old,
                   xkb_atom_text(keymap->ctx, led->name));
            free(led);
            continue;
        }

        map = &keymap->indicators[led->indicator - 1];
        map->flags = led->flags;
        map->which_groups = led->which_groups;
        map->groups = led->groups;
        map->which_mods = led->which_mods;
        map->mods.mask = led->real_mods;
        map->mods.real_mods = led->real_mods;
        map->mods.vmods = led->vmods;
        map->ctrls = led->ctrls;
        free(led);
    }

    list_init(unbound_leds);
}

static bool
CopyIndicatorMapDefs(struct xkb_keymap *keymap, CompatInfo *info)
{
    LEDInfo *led, *next_led;
    struct list unbound_leds;
    struct xkb_indicator_map *im;

    list_init(&unbound_leds);

    list_foreach_safe(led, next_led, &info->leds, entry) {
        if (led->groups != 0 && led->which_groups == 0)
            led->which_groups = XkbIM_UseEffective;

        if (led->which_mods == 0 && (led->real_mods || led->vmods))
            led->which_mods = XkbIM_UseEffective;

        if (led->indicator == _LED_NotBound) {
            list_append(&led->entry, &unbound_leds);
            continue;
        }

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
    list_init(&info->leds);

    BindIndicators(keymap, &unbound_leds);

    return true;
}

bool
CompileCompatMap(XkbFile *file, struct xkb_keymap *keymap,
                 enum merge_mode merge)
{
    xkb_group_index_t i;
    CompatInfo info;
    GroupCompatInfo *gcm;

    InitCompatInfo(&info, keymap, file->id);
    info.dflt.merge = merge;
    info.ledDflt.merge = merge;

    HandleCompatMapFile(file, keymap, merge, &info);

    if (info.errorCount != 0)
        goto err_info;

    darray_init(keymap->sym_interpret);
    darray_growalloc(keymap->sym_interpret, info.nInterps);

    if (info.name)
        keymap->compat_section_name = strdup(info.name);

    if (info.nInterps > 0) {
        CopyInterps(&info, keymap, true, XkbSI_Exactly);
        CopyInterps(&info, keymap, true, XkbSI_AllOf | XkbSI_NoneOf);
        CopyInterps(&info, keymap, true, XkbSI_AnyOf);
        CopyInterps(&info, keymap, true, XkbSI_AnyOfOrNone);
        CopyInterps(&info, keymap, false, XkbSI_Exactly);
        CopyInterps(&info, keymap, false, XkbSI_AllOf | XkbSI_NoneOf);
        CopyInterps(&info, keymap, false, XkbSI_AnyOf);
        CopyInterps(&info, keymap, false, XkbSI_AnyOfOrNone);
    }

    for (i = 0, gcm = &info.groupCompat[0]; i < XkbNumKbdGroups;
         i++, gcm++) {
        if (gcm->file_id != 0 || gcm->real_mods != 0 || gcm->vmods != 0) {
            keymap->groups[i].mask = gcm->real_mods;
            keymap->groups[i].real_mods = gcm->real_mods;
            keymap->groups[i].vmods = gcm->vmods;
        }
    }

    if (!CopyIndicatorMapDefs(keymap, &info))
        info.errorCount++;

    ClearCompatInfo(&info, keymap);
    return true;

err_info:
    ClearCompatInfo(&info, keymap);
    return false;
}

uint32_t
VModsToReal(struct xkb_keymap *keymap, uint32_t vmodmask)
{
    uint32_t ret = 0;
    int i;

    if (!vmodmask)
        return 0;

    for (i = 0; i < XkbNumVirtualMods; i++) {
        if (!(vmodmask & (1 << i)))
            continue;
        ret |= keymap->vmods[i];
    }

    return ret;
}

static void
UpdateActionMods(struct xkb_keymap *keymap, union xkb_action *act,
                 uint32_t rmodmask)
{
    switch (act->type) {
    case XkbSA_SetMods:
    case XkbSA_LatchMods:
    case XkbSA_LockMods:
        if (act->mods.flags & XkbSA_UseModMapMods)
            act->mods.real_mods = rmodmask;
        act->mods.mask = act->mods.real_mods;
        act->mods.mask |= VModsToReal(keymap, act->mods.vmods);
        break;

    case XkbSA_ISOLock:
        if (act->iso.flags & XkbSA_UseModMapMods)
            act->iso.real_mods = rmodmask;
        act->iso.mask = act->iso.real_mods;
        act->iso.mask |= VModsToReal(keymap, act->iso.vmods);
        break;

    default:
        break;
    }
}

/**
 * Find an interpretation which applies to this particular level, either by
 * finding an exact match for the symbol and modifier combination, or a
 * generic XKB_KEY_NoSymbol match.
 */
static struct xkb_sym_interpret *
FindInterpForKey(struct xkb_keymap *keymap, struct xkb_key *key,
                 xkb_group_index_t group, uint32_t level)
{
    struct xkb_sym_interpret *ret = NULL;
    struct xkb_sym_interpret *interp;
    const xkb_keysym_t *syms;
    int num_syms;

    num_syms = xkb_key_get_syms_by_level(keymap, key, group, level, &syms);
    if (num_syms == 0)
        return NULL;

    darray_foreach(interp, keymap->sym_interpret) {
        uint32_t mods;
        bool found;

        if ((num_syms > 1 || interp->sym != syms[0]) &&
            interp->sym != XKB_KEY_NoSymbol)
            continue;

        if (level == 0 || !(interp->match & XkbSI_LevelOneOnly))
            mods = key->modmap;
        else
            mods = 0;

        switch (interp->match & XkbSI_OpMask) {
        case XkbSI_NoneOf:
            found = !(interp->mods & mods);
            break;
        case XkbSI_AnyOfOrNone:
            found = (!mods || (interp->mods & mods));
            break;
        case XkbSI_AnyOf:
            found = !!(interp->mods & mods);
            break;
        case XkbSI_AllOf:
            found = ((interp->mods & mods) == interp->mods);
            break;
        case XkbSI_Exactly:
            found = (interp->mods == mods);
            break;
        default:
            found = false;
            break;
        }

        if (found && interp->sym != XKB_KEY_NoSymbol)
            return interp;
        else if (found && !ret)
            ret = interp;
    }

    return ret;
}

/**
 */
static bool
ApplyInterpsToKey(struct xkb_keymap *keymap, struct xkb_key *key)
{
#define INTERP_SIZE (8 * 4)
    struct xkb_sym_interpret *interps[INTERP_SIZE];
    union xkb_action *acts;
    uint32_t vmodmask = 0;
    int num_acts = 0;
    xkb_group_index_t group;
    int level;
    int i;

    /* If we've been told not to bind interps to this key, then don't. */
    if (key->explicit & XkbExplicitInterpretMask)
        return true;

    for (i = 0; i < INTERP_SIZE; i++)
        interps[i] = NULL;

    for (group = 0; group < key->num_groups; group++) {
        for (level = 0; level < XkbKeyGroupWidth(keymap, key, group);
             level++) {
            i = (group * key->width) + level;
            if (i >= INTERP_SIZE) /* XXX FIXME */
                return false;
            interps[i] = FindInterpForKey(keymap, key, group, level);
            if (interps[i])
                num_acts++;
        }
    }

    if (num_acts)
        num_acts = key->num_groups * key->width;
    acts = XkbcResizeKeyActions(keymap, key, num_acts);
    if (num_acts && !acts)
        return false;

    for (group = 0; group < key->num_groups; group++) {
        for (level = 0; level < XkbKeyGroupWidth(keymap, key, group);
             level++) {
            struct xkb_sym_interpret *interp;

            i = (group * key->width) + level;
            interp = interps[i];

            /* Infer default key behaviours from the base level. */
            if (group == 0 && level == 0) {
                if (!(key->explicit & XkbExplicitAutoRepeatMask) &&
                    (!interp || interp->flags & XkbSI_AutoRepeat))
                    key->repeats = true;
                if (!(key->explicit & XkbExplicitBehaviorMask) &&
                    interp && (interp->flags & XkbSI_LockingKey))
                    key->behavior.type = XkbKB_Lock;
            }

            if (!interp)
                continue;

            if ((group == 0 && level == 0) ||
                !(interp->match & XkbSI_LevelOneOnly)) {
                if (interp->virtual_mod != XkbNoModifier)
                    vmodmask |= (1 << interp->virtual_mod);
            }
            acts[i] = interp->act;
        }
    }

    if (!(key->explicit & XkbExplicitVModMapMask))
        key->vmodmap = vmodmask;

    return true;
#undef INTERP_SIZE
}

/**
 * This collects a bunch of disparate functions which was done in the server
 * at various points that really should've been done within xkbcomp.  Turns out
 * your actions and types are a lot more useful when any of your modifiers
 * other than Shift actually do something ...
 */
bool
UpdateModifiersFromCompat(struct xkb_keymap *keymap)
{
    struct xkb_key *key;
    int i;
    struct xkb_key_type *type;
    struct xkb_kt_map_entry *entry;

    /* Find all the interprets for the key and bind them to actions,
     * which will also update the vmodmap. */
    xkb_foreach_key(key, keymap)
        if (!ApplyInterpsToKey(keymap, key))
            return false;

    /* Update keymap->vmods, the virtual -> real mod mapping. */
    for (i = 0; i < XkbNumVirtualMods; i++)
        keymap->vmods[i] = 0;
    xkb_foreach_key(key, keymap) {
        if (!key->vmodmap)
            continue;

        for (i = 0; i < XkbNumVirtualMods; i++) {
            if (!(key->vmodmap & (1 << i)))
                continue;
            keymap->vmods[i] |= key->modmap;
        }
    }

    /* Now update the level masks for all the types to reflect the vmods. */
    darray_foreach(type, keymap->types) {
        uint32_t mask = 0;
        int j;
        type->mods.mask = type->mods.real_mods;
        type->mods.mask |= VModsToReal(keymap, type->mods.vmods);
        for (j = 0; j < XkbNumVirtualMods; j++) {
            if (!(type->mods.vmods & (1 << j)))
                continue;
            mask |= keymap->vmods[j];
        }

        darray_foreach(entry, type->map)
        entry->mods.mask = entry->mods.real_mods |
                           VModsToReal(keymap, entry->mods.vmods);
    }

    /* Update action modifiers. */
    xkb_foreach_key(key, keymap) {
        union xkb_action *acts = XkbKeyActionsPtr(keymap, key);
        for (i = 0; i < XkbKeyNumActions(key); i++) {
            if (acts[i].any.type == XkbSA_NoAction)
                continue;
            UpdateActionMods(keymap, &acts[i], key->modmap);
        }
    }

    /* Update group modifiers. */
    for (i = 0; i < XkbNumKbdGroups; i++) {
        struct xkb_mods *group = &keymap->groups[i];
        group->mask = group->real_mods | VModsToReal(keymap, group->vmods);
    }

    /* Update vmod -> indicator maps. */
    for (i = 0; i < XkbNumIndicators; i++) {
        struct xkb_mods *led = &keymap->indicators[i].mods;
        led->mask = led->real_mods | VModsToReal(keymap, led->vmods);
    }

    return true;
}
