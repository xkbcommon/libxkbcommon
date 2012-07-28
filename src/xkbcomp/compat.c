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
    xkb_led_index_t indicator;
    unsigned char flags;
    unsigned char which_mods;
    unsigned char real_mods;
    xkb_mod_mask_t vmods;
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
                 KeysymText(si->interp.sym),
                 SIMatchText(si->interp.match),
                 ModMaskText(si->interp.mods, false));
    }
    return buf;
}

static inline bool
ReportSINotArray(CompatInfo *info, SymInterpInfo *si, const char *field)
{
    return ReportNotArray(info->keymap, "symbol interpretation", field,
                          siText(si, info));
}

static inline bool
ReportSIBadType(CompatInfo *info, SymInterpInfo *si, const char *field,
                const char *wanted)
{
    return ReportBadType(info->keymap, "symbol interpretation", field,
                         siText(si, info), wanted);
}

static inline bool
ReportIndicatorBadType(CompatInfo *info, LEDInfo *led,
                       const char *field, const char *wanted)
{
    return ReportBadType(info->keymap, "indicator map", field,
                         xkb_atom_text(info->keymap->ctx, led->name),
                         wanted);
}

static inline bool
ReportIndicatorNotArray(CompatInfo *info, LEDInfo *led,
                        const char *field)
{
    return ReportNotArray(info->keymap, "indicator map", field,
                          xkb_atom_text(info->keymap->ctx, led->name));
}

static void
ClearIndicatorMapInfo(struct xkb_context *ctx, LEDInfo * info)
{
    info->name = xkb_atom_intern(ctx, "default");
    info->indicator = XKB_LED_INVALID;
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
ClearCompatInfo(CompatInfo *info)
{
    unsigned int i;
    ActionInfo *next_act;
    SymInterpInfo *si, *next_si;
    LEDInfo *led, *next_led;
    struct xkb_keymap *keymap = info->keymap;

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
    info->keymap = NULL;
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
UseNewInterpField(unsigned field, SymInterpInfo *old, SymInterpInfo *new,
                  int verbosity, unsigned *collide)
{
    if (!(old->defined & field))
        return true;

    if (new->defined & field) {
        if ((old->file_id == new->file_id && verbosity > 0) || verbosity > 9)
            *collide |= field;

        if (new->merge != MERGE_AUGMENT)
            return true;
    }

    return false;
}

static bool
AddInterp(CompatInfo * info, SymInterpInfo * new)
{
    unsigned collide;
    SymInterpInfo *old;
    struct list entry;
    int verbosity = xkb_get_log_verbosity(info->keymap->ctx);

    collide = 0;
    old = FindMatchingInterp(info, new);
    if (old != NULL) {
        if (new->merge == MERGE_REPLACE) {
            entry = old->entry;
            if ((old->file_id == new->file_id && verbosity > 0) ||
                verbosity > 9)
                log_warn(info->keymap->ctx,
                         "Multiple definitions for \"%s\"; "
                         "Earlier interpretation ignored\n",
                         siText(new, info));
            *old = *new;
            old->entry = entry;
            return true;
        }

        if (UseNewInterpField(_SI_VirtualMod, old, new, verbosity,
                              &collide)) {
            old->interp.virtual_mod = new->interp.virtual_mod;
            old->defined |= _SI_VirtualMod;
        }
        if (UseNewInterpField(_SI_Action, old, new, verbosity,
                              &collide)) {
            old->interp.act = new->interp.act;
            old->defined |= _SI_Action;
        }
        if (UseNewInterpField(_SI_AutoRepeat, old, new, verbosity,
                              &collide)) {
            old->interp.flags &= ~XkbSI_AutoRepeat;
            old->interp.flags |= (new->interp.flags & XkbSI_AutoRepeat);
            old->defined |= _SI_AutoRepeat;
        }
        if (UseNewInterpField(_SI_LockingKey, old, new, verbosity,
                              &collide)) {
            old->interp.flags &= ~XkbSI_LockingKey;
            old->interp.flags |= (new->interp.flags & XkbSI_LockingKey);
            old->defined |= _SI_LockingKey;
        }
        if (UseNewInterpField(_SI_LevelOneOnly, old, new, verbosity,
                              &collide)) {
            old->interp.match &= ~XkbSI_LevelOneOnly;
            old->interp.match |= (new->interp.match & XkbSI_LevelOneOnly);
            old->defined |= _SI_LevelOneOnly;
        }

        if (collide) {
            log_warn(info->keymap->ctx,
                     "Multiple interpretations of \"%s\"; "
                     "Using %s definition for duplicate fields\n",
                     siText(new, info),
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
    int verbosity = xkb_get_log_verbosity(info->keymap->ctx);

    gc = &info->groupCompat[group];
    if (gc->real_mods == new->real_mods && gc->vmods == new->vmods)
        return true;

    if ((gc->file_id == new->file_id && verbosity > 0) || verbosity > 9)
        log_warn(info->keymap->ctx,
                 "Compat map for group %u redefined; "
                 "Using %s definition\n",
                 group + 1, (new->merge == MERGE_AUGMENT ? "old" : "new"));

    if (new->defined && (new->merge != MERGE_AUGMENT || !gc->defined))
        *gc = *new;

    return true;
}

/***====================================================================***/

static bool
ResolveStateAndPredicate(ExprDef * expr,
                         unsigned *pred_rtrn,
                         xkb_mod_mask_t *mods_rtrn, CompatInfo * info)
{
    if (expr == NULL) {
        *pred_rtrn = XkbSI_AnyOfOrNone;
        *mods_rtrn = ~0;
        return true;
    }

    *pred_rtrn = XkbSI_Exactly;
    if (expr->op == EXPR_ACTION_DECL) {
        const char *pred_txt = xkb_atom_text(info->keymap->ctx,
                                             expr->value.action.name);
        if (istreq(pred_txt, "noneof"))
            *pred_rtrn = XkbSI_NoneOf;
        else if (istreq(pred_txt, "anyofornone"))
            *pred_rtrn = XkbSI_AnyOfOrNone;
        else if (istreq(pred_txt, "anyof"))
            *pred_rtrn = XkbSI_AnyOf;
        else if (istreq(pred_txt, "allof"))
            *pred_rtrn = XkbSI_AllOf;
        else if (istreq(pred_txt, "exactly"))
            *pred_rtrn = XkbSI_Exactly;
        else {
            log_err(info->keymap->ctx,
                    "Illegal modifier predicate \"%s\"; Ignored\n", pred_txt);
            return false;
        }
        expr = expr->value.action.args;
    }
    else if (expr->op == EXPR_IDENT) {
        const char *pred_txt = xkb_atom_text(info->keymap->ctx,
                                             expr->value.str);
        if (pred_txt && istreq(pred_txt, "any")) {
            *pred_rtrn = XkbSI_AnyOf;
            *mods_rtrn = 0xff;
            return true;
        }
    }

    return ExprResolveModMask(info->keymap->ctx, expr, mods_rtrn);
}

/***====================================================================***/

static bool
UseNewLEDField(unsigned field, LEDInfo *old, LEDInfo *new,
               int verbosity, unsigned *collide)
{
    if (!(old->defined & field))
        return true;

    if (new->defined & field) {
        if ((old->file_id == new->file_id && verbosity > 0) || verbosity > 9)
            *collide |= field;

        if (new->merge != MERGE_AUGMENT)
            return true;
    }

    return false;
}

static bool
AddIndicatorMap(CompatInfo *info, LEDInfo *new)
{
    LEDInfo *old;
    unsigned collide;
    struct xkb_context *ctx = info->keymap->ctx;
    int verbosity = xkb_get_log_verbosity(ctx);

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
                if ((old->file_id == new->file_id && verbosity > 0) ||
                    verbosity > 9)
                    log_warn(info->keymap->ctx,
                             "Map for indicator %s redefined; "
                             "Earlier definition ignored\n",
                             xkb_atom_text(ctx, old->name));
                *old = *new;
                old->entry = entry;
                return true;
            }

            collide = 0;
            if (UseNewLEDField(_LED_Index, old, new, verbosity,
                               &collide)) {
                old->indicator = new->indicator;
                old->defined |= _LED_Index;
            }
            if (UseNewLEDField(_LED_Mods, old, new, verbosity,
                               &collide)) {
                old->which_mods = new->which_mods;
                old->real_mods = new->real_mods;
                old->vmods = new->vmods;
                old->defined |= _LED_Mods;
            }
            if (UseNewLEDField(_LED_Groups, old, new, verbosity,
                               &collide)) {
                old->which_groups = new->which_groups;
                old->groups = new->groups;
                old->defined |= _LED_Groups;
            }
            if (UseNewLEDField(_LED_Ctrls, old, new, verbosity,
                               &collide)) {
                old->ctrls = new->ctrls;
                old->defined |= _LED_Ctrls;
            }
            if (UseNewLEDField(_LED_Explicit, old, new, verbosity,
                               &collide)) {
                old->flags &= ~XkbIM_NoExplicit;
                old->flags |= (new->flags & XkbIM_NoExplicit);
                old->defined |= _LED_Explicit;
            }
            if (UseNewLEDField(_LED_Automatic, old, new, verbosity,
                               &collide)) {
                old->flags &= ~XkbIM_NoAutomatic;
                old->flags |= (new->flags & XkbIM_NoAutomatic);
                old->defined |= _LED_Automatic;
            }
            if (UseNewLEDField(_LED_DrivesKbd, old, new, verbosity,
                               &collide)) {
                old->flags &= ~XkbIM_LEDDrivesKB;
                old->flags |= (new->flags & XkbIM_LEDDrivesKB);
                old->defined |= _LED_DrivesKbd;
            }

            if (collide) {
                log_warn(info->keymap->ctx,
                         "Map for indicator %s redefined; "
                         "Using %s definition for duplicate fields\n",
                         xkb_atom_text(ctx, old->name),
                         (new->merge == MERGE_AUGMENT ? "first" : "last"));
            }

            return true;
        }
    }

    /* new definition */
    old = malloc(sizeof(*old));
    if (!old) {
        log_wsgo(info->keymap->ctx,
                 "Couldn't allocate indicator map; "
                 "Map for indicator %s not compiled\n",
                 xkb_atom_text(ctx, new->name));
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
        if (!AddIndicatorMap(into, led))
            into->errorCount++;
    }
}

static void
HandleCompatMapFile(CompatInfo *info, XkbFile *file, enum merge_mode merge);

static bool
HandleIncludeCompatMap(CompatInfo *info, IncludeStmt *stmt)
{
    enum merge_mode merge = MERGE_DEFAULT;
    XkbFile *rtrn;
    CompatInfo included, next_incl;

    InitCompatInfo(&included, info->keymap, info->file_id);
    if (stmt->stmt) {
        free(included.name);
        included.name = stmt->stmt;
        stmt->stmt = NULL;
    }

    for (; stmt; stmt = stmt->next_incl) {
        if (!ProcessIncludeFile(info->keymap->ctx, stmt, FILE_TYPE_COMPAT,
                                &rtrn, &merge)) {
            info->errorCount += 10;
            ClearCompatInfo(&included);
            return false;
        }

        InitCompatInfo(&next_incl, info->keymap, rtrn->id);
        next_incl.file_id = rtrn->id;
        next_incl.dflt = info->dflt;
        next_incl.dflt.file_id = rtrn->id;
        next_incl.dflt.merge = merge;
        next_incl.ledDflt.file_id = rtrn->id;
        next_incl.ledDflt.merge = merge;
        next_incl.act = info->act;

        HandleCompatMapFile(&next_incl, rtrn, MERGE_OVERRIDE);

        MergeIncludedCompatMaps(&included, &next_incl, merge);
        if (info->act)
            next_incl.act = NULL;

        ClearCompatInfo(&next_incl);
        FreeXKBFile(rtrn);
    }

    MergeIncludedCompatMaps(info, &included, merge);
    ClearCompatInfo(&included);

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
SetInterpField(CompatInfo *info, SymInterpInfo *si, const char *field,
               ExprDef *arrayNdx, ExprDef *value)
{
    struct xkb_keymap *keymap = info->keymap;
    xkb_mod_index_t ndx;

    if (istreq(field, "action")) {
        if (arrayNdx)
            return ReportSINotArray(info, si, field);

        if (!HandleActionDef(value, keymap, &si->interp.act.any, info->act))
            return false;

        si->defined |= _SI_Action;
    }
    else if (istreq(field, "virtualmodifier") ||
             istreq(field, "virtualmod")) {
        if (arrayNdx)
            return ReportSINotArray(info, si, field);

        if (!ResolveVirtualModifier(value, keymap, &ndx, &info->vmods))
            return ReportSIBadType(info, si, field, "virtual modifier");

        si->interp.virtual_mod = ndx;
        si->defined |= _SI_VirtualMod;
    }
    else if (istreq(field, "repeat")) {
        bool set;

        if (arrayNdx)
            return ReportSINotArray(info, si, field);

        if (!ExprResolveBoolean(keymap->ctx, value, &set))
            return ReportSIBadType(info, si, field, "boolean");

        if (set)
            si->interp.flags |= XkbSI_AutoRepeat;
        else
            si->interp.flags &= ~XkbSI_AutoRepeat;

        si->defined |= _SI_AutoRepeat;
    }
    else if (istreq(field, "locking")) {
        bool set;

        if (arrayNdx)
            return ReportSINotArray(info, si, field);

        if (!ExprResolveBoolean(keymap->ctx, value, &set))
            return ReportSIBadType(info, si, field, "boolean");

        if (set)
            si->interp.flags |= XkbSI_LockingKey;
        else
            si->interp.flags &= ~XkbSI_LockingKey;

        si->defined |= _SI_LockingKey;
    }
    else if (istreq(field, "usemodmap") ||
             istreq(field, "usemodmapmods")) {
        unsigned int val;

        if (arrayNdx)
            return ReportSINotArray(info, si, field);

        if (!ExprResolveEnum(keymap->ctx, value, &val, useModMapValues))
            return ReportSIBadType(info, si, field, "level specification");

        if (val)
            si->interp.match |= XkbSI_LevelOneOnly;
        else
            si->interp.match &= ~XkbSI_LevelOneOnly;

        si->defined |= _SI_LevelOneOnly;
    }
    else {
        return ReportBadField(keymap, "symbol interpretation", field,
                              siText(si, info));
    }

    return true;
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
SetIndicatorMapField(CompatInfo *info, LEDInfo *led,
                     const char *field, ExprDef *arrayNdx, ExprDef *value)
{
    bool ok = true;
    struct xkb_keymap *keymap = info->keymap;

    if (istreq(field, "modifiers") || istreq(field, "mods")) {
        xkb_mod_mask_t mask;

        if (arrayNdx)
            return ReportIndicatorNotArray(info, led, field);

        if (!ExprResolveVModMask(keymap, value, &mask))
            return ReportIndicatorBadType(info, led, field, "modifier mask");

        led->real_mods = mask & 0xff;
        led->vmods = (mask >> 8) & 0xff;
        led->defined |= _LED_Mods;
    }
    else if (istreq(field, "groups")) {
        unsigned int mask;

        if (arrayNdx)
            return ReportIndicatorNotArray(info, led, field);

        if (!ExprResolveMask(keymap->ctx, value, &mask, groupNames))
            return ReportIndicatorBadType(info, led, field, "group mask");

        led->groups = mask;
        led->defined |= _LED_Groups;
    }
    else if (istreq(field, "controls") || istreq(field, "ctrls")) {
        unsigned int mask;

        if (arrayNdx)
            return ReportIndicatorNotArray(info, led, field);

        if (!ExprResolveMask(keymap->ctx, value, &mask, ctrlNames))
            return ReportIndicatorBadType(info, led, field,
                                          "controls mask");

        led->ctrls = mask;
        led->defined |= _LED_Ctrls;
    }
    else if (istreq(field, "allowexplicit")) {
        bool set;

        if (arrayNdx)
            return ReportIndicatorNotArray(info, led, field);

        if (!ExprResolveBoolean(keymap->ctx, value, &set))
            return ReportIndicatorBadType(info, led, field, "boolean");

        if (set)
            led->flags &= ~XkbIM_NoExplicit;
        else
            led->flags |= XkbIM_NoExplicit;

        led->defined |= _LED_Explicit;
    }
    else if (istreq(field, "whichmodstate") ||
             istreq(field, "whichmodifierstate")) {
        unsigned int mask;

        if (arrayNdx)
            return ReportIndicatorNotArray(info, led, field);

        if (!ExprResolveMask(keymap->ctx, value, &mask, modComponentNames))
            return ReportIndicatorBadType(info, led, field,
                                          "mask of modifier state components");

        led->which_mods = mask;
    }
    else if (istreq(field, "whichgroupstate")) {
        unsigned mask;

        if (arrayNdx)
            return ReportIndicatorNotArray(info, led, field);

        if (!ExprResolveMask(keymap->ctx, value, &mask, groupComponentNames))
            return ReportIndicatorBadType(info, led, field,
                                          "mask of group state components");

        led->which_groups = mask;
    }
    else if (istreq(field, "driveskbd") ||
             istreq(field, "driveskeyboard") ||
             istreq(field, "leddriveskbd") ||
             istreq(field, "leddriveskeyboard") ||
             istreq(field, "indicatordriveskbd") ||
             istreq(field, "indicatordriveskeyboard")) {
        bool set;

        if (arrayNdx)
            return ReportIndicatorNotArray(info, led, field);

        if (!ExprResolveBoolean(keymap->ctx, value, &set))
            return ReportIndicatorBadType(info, led, field, "boolean");

        if (set)
            led->flags |= XkbIM_LEDDrivesKB;
        else
            led->flags &= ~XkbIM_LEDDrivesKB;

        led->defined |= _LED_DrivesKbd;
    }
    else if (istreq(field, "index")) {
        int ndx;

        if (arrayNdx)
            return ReportIndicatorNotArray(info, led, field);

        if (!ExprResolveInteger(keymap->ctx, value, &ndx))
            return ReportIndicatorBadType(info, led, field,
                                          "indicator index");

        if (ndx < 1 || ndx > 32) {
            log_err(info->keymap->ctx,
                    "Illegal indicator index %d (range 1..%d); "
                    "Index definition for %s indicator ignored\n",
                    ndx, XkbNumIndicators,
                    xkb_atom_text(keymap->ctx, led->name));
            return false;
        }

        led->indicator = (xkb_led_index_t) ndx;
        led->defined |= _LED_Index;
    }
    else {
        log_err(info->keymap->ctx,
                "Unknown field %s in map for %s indicator; "
                "Definition ignored\n",
                field, xkb_atom_text(keymap->ctx, led->name));
        ok = false;
    }

    return ok;
}

static int
HandleInterpVar(CompatInfo *info, VarDef *stmt)
{
    const char *elem, *field;
    ExprDef *ndx;
    int ret;

    if (!ExprResolveLhs(info->keymap->ctx, stmt->name, &elem, &field, &ndx))
        ret = 0;               /* internal error, already reported */
    else if (elem && istreq(elem, "interpret"))
        ret = SetInterpField(info, &info->dflt, field, ndx, stmt->value);
    else if (elem && istreq(elem, "indicator"))
        ret = SetIndicatorMapField(info, &info->ledDflt, field, ndx,
                                   stmt->value);
    else
        ret = SetActionField(info->keymap, elem, field, ndx, stmt->value,
                             &info->act);
    return ret;
}

static int
HandleInterpBody(CompatInfo *info, VarDef *def, SymInterpInfo *si)
{
    int ok = 1;
    const char *elem, *field;
    ExprDef *arrayNdx;

    for (; def != NULL; def = (VarDef *) def->common.next) {
        if (def->name && def->name->op == EXPR_FIELD_REF) {
            ok = HandleInterpVar(info, def);
            continue;
        }
        ok = ExprResolveLhs(info->keymap->ctx, def->name, &elem, &field,
                            &arrayNdx);
        if (ok) {
            ok = SetInterpField(info, si, field, arrayNdx, def->value);
        }
    }
    return ok;
}

static int
HandleInterpDef(CompatInfo *info, InterpDef *def, enum merge_mode merge)
{
    unsigned pred, mods;
    SymInterpInfo si;

    if (!ResolveStateAndPredicate(def->match, &pred, &mods, info)) {
        log_err(info->keymap->ctx,
                "Couldn't determine matching modifiers; "
                "Symbol interpretation ignored\n");
        return false;
    }
    if (def->merge != MERGE_DEFAULT)
        merge = def->merge;

    si = info->dflt;
    si.merge = merge;
    if (!LookupKeysym(def->sym, &si.interp.sym)) {
        log_err(info->keymap->ctx,
                "Could not resolve keysym %s; "
                "Symbol interpretation ignored\n",
                def->sym);
        return false;
    }
    si.interp.match = pred & XkbSI_OpMask;
    si.interp.mods = mods;
    if (!HandleInterpBody(info, def->def, &si)) {
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
HandleGroupCompatDef(CompatInfo *info, GroupCompatDef *def,
                     enum merge_mode merge)
{
    xkb_mod_mask_t mask;
    GroupCompatInfo tmp;

    merge = (def->merge == MERGE_DEFAULT ? merge : def->merge);

    if (def->group < 1 || def->group > XkbNumKbdGroups) {
        log_err(info->keymap->ctx,
                "Keyboard group must be in the range 1..%u; "
                "Compatibility map for illegal group %u ignored\n",
                XkbNumKbdGroups, def->group);
        return false;
    }

    tmp.file_id = info->file_id;
    tmp.merge = merge;

    if (!ExprResolveVModMask(info->keymap, def->def, &mask)) {
        log_err(info->keymap->ctx,
                "Expected a modifier mask in group compatibility definition; "
                "Ignoring illegal compatibility map for group %u\n",
                def->group);
        return false;
    }

    tmp.real_mods = mask & 0xff;
    tmp.vmods = (mask >> 8) & 0xffff;
    tmp.defined = true;
    return AddGroupCompat(info, def->group - 1, &tmp);
}

static bool
HandleIndicatorMapDef(CompatInfo *info, IndicatorMapDef *def,
                      enum merge_mode merge)
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
        const char *elem, *field;
        ExprDef *arrayNdx;
        if (!ExprResolveLhs(info->keymap->ctx, var->name, &elem, &field,
                            &arrayNdx)) {
            ok = false;
            continue;
        }

        if (elem) {
            log_err(info->keymap->ctx,
                    "Cannot set defaults for \"%s\" element in indicator map; "
                    "Assignment to %s.%s ignored\n", elem, elem, field);
            ok = false;
        }
        else {
            ok = SetIndicatorMapField(info, &led, field, arrayNdx,
                                      var->value) && ok;
        }
    }

    if (ok)
        return AddIndicatorMap(info, &led);

    return false;
}

static void
HandleCompatMapFile(CompatInfo *info, XkbFile *file, enum merge_mode merge)
{
    ParseCommon *stmt;

    if (merge == MERGE_DEFAULT)
        merge = MERGE_AUGMENT;
    free(info->name);
    info->name = strdup_safe(file->name);
    stmt = file->defs;
    while (stmt)
    {
        switch (stmt->type) {
        case STMT_INCLUDE:
            if (!HandleIncludeCompatMap(info, (IncludeStmt *) stmt))
                info->errorCount++;
            break;
        case STMT_INTERP:
            if (!HandleInterpDef(info, (InterpDef *) stmt, merge))
                info->errorCount++;
            break;
        case STMT_GROUP_COMPAT:
            if (!HandleGroupCompatDef(info, (GroupCompatDef *) stmt, merge))
                info->errorCount++;
            break;
        case STMT_INDICATOR_MAP:
            if (!HandleIndicatorMapDef(info, (IndicatorMapDef *) stmt, merge))
                info->errorCount++;
            break;
        case STMT_VAR:
            if (!HandleInterpVar(info, (VarDef *) stmt))
                info->errorCount++;
            break;
        case STMT_VMOD:
            if (!HandleVModDef((VModDef *) stmt, info->keymap, merge,
                               &info->vmods))
                info->errorCount++;
            break;
        case STMT_KEYCODE:
            log_err(info->keymap->ctx,
                    "Interpretation files may not include other types; "
                    "Ignoring definition of key name\n");
            info->errorCount++;
            break;
        default:
            log_wsgo(info->keymap->ctx,
                     "Unexpected statement type %d in HandleCompatMapFile\n",
                     stmt->type);
            break;
        }
        stmt = stmt->next;
        if (info->errorCount > 10) {
            log_err(info->keymap->ctx,
                    "Abandoning compatibility map \"%s\"\n", file->topName);
            break;
        }
    }
}

static void
CopyInterps(CompatInfo *info, bool needSymbol, unsigned pred)
{
    SymInterpInfo *si;

    list_foreach(si, &info->interps, entry) {
        if (((si->interp.match & XkbSI_OpMask) != pred) ||
            (needSymbol && si->interp.sym == XKB_KEY_NoSymbol) ||
            (!needSymbol && si->interp.sym != XKB_KEY_NoSymbol))
            continue;

        darray_append(info->keymap->sym_interpret, si->interp);
    }
}

static void
BindIndicators(CompatInfo *info, struct list *unbound_leds)
{
    xkb_led_index_t i;
    LEDInfo *led, *next_led;
    struct xkb_indicator_map *map;
    struct xkb_keymap *keymap = info->keymap;

    list_foreach(led, unbound_leds, entry) {
        if (led->indicator == XKB_LED_INVALID) {
            for (i = 0; i < XkbNumIndicators; i++) {
                if (keymap->indicator_names[i] &&
                    streq(keymap->indicator_names[i],
                          xkb_atom_text(keymap->ctx, led->name))) {
                    led->indicator = i + 1;
                    break;
                }
            }
        }
    }

    list_foreach(led, unbound_leds, entry) {
        if (led->indicator == XKB_LED_INVALID) {
            for (i = 0; i < XkbNumIndicators; i++) {
                if (keymap->indicator_names[i] == NULL) {
                    keymap->indicator_names[i] =
                        xkb_atom_text(keymap->ctx, led->name);
                    led->indicator = i + 1;
                    break;
                }
            }

            if (led->indicator == XKB_LED_INVALID) {
                log_err(info->keymap->ctx,
                        "No unnamed indicators found; "
                        "Virtual indicator map \"%s\" not bound\n",
                        xkb_atom_text(keymap->ctx, led->name));
                continue;
            }
        }
    }

    list_foreach_safe(led, next_led, unbound_leds, entry) {
        if (led->indicator == XKB_LED_INVALID) {
            free(led);
            continue;
        }

        if (!streq(keymap->indicator_names[led->indicator - 1],
                   xkb_atom_text(keymap->ctx, led->name))) {
            const char *old = keymap->indicator_names[led->indicator - 1];
            log_err(info->keymap->ctx,
                    "Multiple names bound to indicator %d; "
                    "Using %s, ignoring %s\n",
                    led->indicator, old,
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
CopyIndicatorMapDefs(CompatInfo *info)
{
    LEDInfo *led, *next_led;
    struct list unbound_leds;
    struct xkb_indicator_map *im;
    struct xkb_keymap *keymap = info->keymap;

    list_init(&unbound_leds);

    list_foreach_safe(led, next_led, &info->leds, entry) {
        if (led->groups != 0 && led->which_groups == 0)
            led->which_groups = XkbIM_UseEffective;

        if (led->which_mods == 0 && (led->real_mods || led->vmods))
            led->which_mods = XkbIM_UseEffective;

        if (led->indicator == XKB_LED_INVALID) {
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
        keymap->indicator_names[led->indicator - 1] =
            xkb_atom_text(keymap->ctx, led->name);
        free(led);
    }
    list_init(&info->leds);

    BindIndicators(info, &unbound_leds);

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

    HandleCompatMapFile(&info, file, merge);

    if (info.errorCount != 0)
        goto err_info;

    darray_init(keymap->sym_interpret);
    darray_growalloc(keymap->sym_interpret, info.nInterps);

    if (info.name)
        keymap->compat_section_name = strdup(info.name);

    if (info.nInterps > 0) {
        CopyInterps(&info, true, XkbSI_Exactly);
        CopyInterps(&info, true, XkbSI_AllOf | XkbSI_NoneOf);
        CopyInterps(&info, true, XkbSI_AnyOf);
        CopyInterps(&info, true, XkbSI_AnyOfOrNone);
        CopyInterps(&info, false, XkbSI_Exactly);
        CopyInterps(&info, false, XkbSI_AllOf | XkbSI_NoneOf);
        CopyInterps(&info, false, XkbSI_AnyOf);
        CopyInterps(&info, false, XkbSI_AnyOfOrNone);
    }

    for (i = 0, gcm = &info.groupCompat[0]; i < XkbNumKbdGroups;
         i++, gcm++) {
        if (gcm->file_id != 0 || gcm->real_mods != 0 || gcm->vmods != 0) {
            keymap->groups[i].mask = gcm->real_mods;
            keymap->groups[i].real_mods = gcm->real_mods;
            keymap->groups[i].vmods = gcm->vmods;
        }
    }

    if (!CopyIndicatorMapDefs(&info))
        info.errorCount++;

    ClearCompatInfo(&info);
    return true;

err_info:
    ClearCompatInfo(&info);
    return false;
}

xkb_mod_mask_t
VModsToReal(struct xkb_keymap *keymap, xkb_mod_mask_t vmodmask)
{
    xkb_mod_mask_t ret = 0;
    xkb_mod_index_t i;

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
                 xkb_mod_mask_t rmodmask)
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
    xkb_mod_mask_t vmodmask = 0;
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
    acts = ResizeKeyActions(keymap, key, num_acts);
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
                    (!interp || (interp->flags & XkbSI_AutoRepeat)))
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
    xkb_mod_index_t vmod;
    xkb_group_index_t grp;
    xkb_led_index_t led;
    int i;
    struct xkb_key *key;
    struct xkb_key_type *type;
    struct xkb_kt_map_entry *entry;

    /* Find all the interprets for the key and bind them to actions,
     * which will also update the vmodmap. */
    xkb_foreach_key(key, keymap)
        if (!ApplyInterpsToKey(keymap, key))
            return false;

    /* Update keymap->vmods, the virtual -> real mod mapping. */
    for (vmod = 0; vmod < XkbNumVirtualMods; vmod++)
        keymap->vmods[vmod] = 0;

    xkb_foreach_key(key, keymap) {
        if (!key->vmodmap)
            continue;

        for (vmod = 0; vmod < XkbNumVirtualMods; vmod++) {
            if (!(key->vmodmap & (1 << vmod)))
                continue;
            keymap->vmods[vmod] |= key->modmap;
        }
    }

    /* Now update the level masks for all the types to reflect the vmods. */
    darray_foreach(type, keymap->types) {
        xkb_mod_mask_t mask = 0;
        type->mods.mask = type->mods.real_mods;
        type->mods.mask |= VModsToReal(keymap, type->mods.vmods);

        /* FIXME: We compute the mask with doing anything with it? */
        for (vmod = 0; vmod < XkbNumVirtualMods; vmod++) {
            if (!(type->mods.vmods & (1 << vmod)))
                continue;
            mask |= keymap->vmods[vmod];
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
    for (grp = 0; grp < XkbNumKbdGroups; grp++) {
        struct xkb_mods *group = &keymap->groups[grp];
        group->mask = group->real_mods | VModsToReal(keymap, group->vmods);
    }

    /* Update vmod -> indicator maps. */
    for (led = 0; led < XkbNumIndicators; led++) {
        struct xkb_mods *mods = &keymap->indicators[led].mods;
        mods->mask = mods->real_mods | VModsToReal(keymap, mods->vmods);
    }

    return true;
}
