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
#include "text.h"
#include "expr.h"
#include "action.h"
#include "vmod.h"
#include "include.h"

/*
 * The xkb_compat section
 * =====================
 * This section is the third to be processesed, after xkb_keycodes and
 * xkb_types.
 *
 * Interpret statements
 * --------------------
 * Statements of the form:
 *      interpret Num_Lock+Any { ... }
 *
 * The body of the statment may include statements of the following
 * forms:
 *
 * - action statement:
 *      action = LockMods(modifiers=NumLock);
 *
 * - virtual modifier statement:
 *      virtualModifier = NumLock;
 *
 * - repeat statement:
 *      repeat = True;
 *
 * - useModMapMods statement:
 *      useModMapMods = level1;
 *
 * Indicator map statements
 * ------------------------
 * Statements of the form:
 *      indicator "Shift Lock" { ... }
 *
 *   This statement specifies the behavior and binding of the indicator
 *   with the given name ("Shift Lock" above). The name should have been
 *   declared previously in the xkb_keycodes section (see Indicator name
 *   statement), and given an index there. If it wasn't, it is created
 *   with the next free index.
 *   The body of the statement describes the conditions of the keyboard
 *   state which will cause the indicator to be lit. It may include the
 *   following statements:
 *
 * - modifiers statment:
 *      modifiers = ScrollLock;
 *
 *   If the given modifiers are in the required state (see below), the
 *   led is lit.
 *
 * - whichModifierState statment:
 *      whichModState = Latched + Locked;
 *
 *   Can be any combination of:
 *      base, latched, locked, effective
 *      any (i.e. all of the above)
 *      none (i.e. none of the above)
 *      compat (this is legal, but unused)
 *   This will cause the respective portion of the modifer state (see
 *   struct xkb_state) to be matched against the modifiers given in the
 *   "modifiers" statement.
 *
 *   Here's a simple example:
 *      indicator "Num Lock" {
 *          modifiers = NumLock;
 *          whichModState = Locked;
 *      };
 *   Whenever the NumLock modifier is locked, the Num Lock indicator
 *   will light up.
 *
 * - groups statment:
 *      groups = All - group1;
 *
 *   If the given groups are in the required state (see below), the led
 *   is lit.
 *
 * - whichGroupState statment:
 *      whichGroupState = Effective;
 *
 *   Can be any combination of:
 *      base, latched, locked, effective
 *      any (i.e. all of the above)
 *      none (i.e. none of the above)
 *   This will cause the respective portion of the group state (see
 *   struct xkb_state) to be matched against the groups given in the
 *   "groups" statement.
 *
 *   Note: the above conditions are disjunctive, i.e. if any of them are
 *   satisfied the led is lit.
 *
 * Virtual modifier statements
 * ---------------------------
 * Statements of the form:
 *     virtual_modifiers LControl;
 *
 * Can appear in the xkb_types, xkb_compat, xkb_symbols sections.
 * TODO
 *
 * Effect on keymap
 * ----------------
 * After all of the xkb_compat sections have been compiled, the following
 * members of struct xkb_keymap are finalized:
 *      darray(struct xkb_sym_interpret) sym_interpret;
 *      struct xkb_indicator_map indicators[XkbNumIndicators];
 *      char *compat_section_name;
 * TODO: virtual modifiers.
 */

enum si_field {
    SI_FIELD_VIRTUAL_MOD    = (1 << 0),
    SI_FIELD_ACTION         = (1 << 1),
    SI_FIELD_AUTO_REPEAT    = (1 << 2),
    SI_FIELD_LEVEL_ONE_ONLY = (1 << 3),
};

typedef struct _SymInterpInfo {
    enum si_field defined;
    unsigned file_id;
    enum merge_mode merge;

    struct xkb_sym_interpret interp;
} SymInterpInfo;

enum led_field {
    LED_FIELD_MODS       = (1 << 0),
    LED_FIELD_GROUPS     = (1 << 1),
    LED_FIELD_CTRLS      = (1 << 2),
};

typedef struct _LEDInfo {
    enum led_field defined;
    unsigned file_id;
    enum merge_mode merge;

    xkb_atom_t name;
    unsigned char which_mods;
    xkb_mod_mask_t mods;
    unsigned char which_groups;
    uint32_t groups;
    unsigned int ctrls;
} LEDInfo;

typedef struct _CompatInfo {
    char *name;
    unsigned file_id;
    int errorCount;
    SymInterpInfo dflt;
    darray(SymInterpInfo) interps;
    LEDInfo ledDflt;
    darray(LEDInfo) leds;
    VModInfo vmods;
    ActionInfo *act;
    struct xkb_keymap *keymap;
} CompatInfo;

static const char *
siText(SymInterpInfo *si, CompatInfo *info)
{
    static char buf[128];

    if (si == &info->dflt)
        return "default";

    snprintf(buf, sizeof(buf), "%s+%s(%s)",
             KeysymText(si->interp.sym),
             SIMatchText(si->interp.match),
             ModMaskText(si->interp.mods));
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
    return ReportBadType(info->keymap->ctx, "symbol interpretation", field,
                         siText(si, info), wanted);
}

static inline bool
ReportIndicatorBadType(CompatInfo *info, LEDInfo *led,
                       const char *field, const char *wanted)
{
    return ReportBadType(info->keymap->ctx, "indicator map", field,
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
ClearIndicatorMapInfo(struct xkb_context *ctx, LEDInfo *info)
{
    info->name = xkb_atom_intern(ctx, "default");
    info->which_mods = 0;
    info->mods = 0;
    info->which_groups = info->groups = 0;
    info->ctrls = 0;
}

static void
InitCompatInfo(CompatInfo *info, struct xkb_keymap *keymap, unsigned file_id)
{
    info->keymap = keymap;
    info->name = NULL;
    info->file_id = file_id;
    info->errorCount = 0;
    darray_init(info->interps);
    info->act = NULL;
    info->dflt.file_id = file_id;
    info->dflt.defined = 0;
    info->dflt.merge = MERGE_OVERRIDE;
    info->dflt.interp.flags = 0;
    info->dflt.interp.virtual_mod = XkbNoModifier;
    memset(&info->dflt.interp.act, 0, sizeof(info->dflt.interp.act));
    info->dflt.interp.act.type = XkbSA_NoAction;
    ClearIndicatorMapInfo(keymap->ctx, &info->ledDflt);
    info->ledDflt.file_id = file_id;
    info->ledDflt.defined = 0;
    info->ledDflt.merge = MERGE_OVERRIDE;
    darray_init(info->leds);
    InitVModInfo(&info->vmods, keymap);
}

static void
ClearCompatInfo(CompatInfo *info)
{
    struct xkb_keymap *keymap = info->keymap;

    free(info->name);
    info->name = NULL;
    info->dflt.defined = 0;
    info->dflt.merge = MERGE_AUGMENT;
    info->dflt.interp.flags = 0;
    info->dflt.interp.virtual_mod = XkbNoModifier;
    memset(&info->dflt.interp.act, 0, sizeof(info->dflt.interp.act));
    info->dflt.interp.act.type = XkbSA_NoAction;
    ClearIndicatorMapInfo(keymap->ctx, &info->ledDflt);
    darray_free(info->interps);
    darray_free(info->leds);
    FreeActionInfo(info->act);
    info->act = NULL;
    info->keymap = NULL;
    ClearVModInfo(&info->vmods);
}

static SymInterpInfo *
FindMatchingInterp(CompatInfo *info, SymInterpInfo *new)
{
    SymInterpInfo *old;

    darray_foreach(old, info->interps)
        if (old->interp.sym == new->interp.sym &&
            old->interp.mods == new->interp.mods &&
            old->interp.match == new->interp.match)
            return old;

    return NULL;
}

static bool
UseNewInterpField(enum si_field field, SymInterpInfo *old, SymInterpInfo *new,
                  bool report, enum si_field *collide)
{
    if (!(old->defined & field))
        return true;

    if (new->defined & field) {
        if (report)
            *collide |= field;

        if (new->merge != MERGE_AUGMENT)
            return true;
    }

    return false;
}

static bool
AddInterp(CompatInfo *info, SymInterpInfo *new)
{
    enum si_field collide = 0;
    SymInterpInfo *old;

    old = FindMatchingInterp(info, new);
    if (old) {
        int verbosity = xkb_get_log_verbosity(info->keymap->ctx);
        bool report = ((old->file_id == new->file_id && verbosity > 0) ||
                       verbosity > 9);

        if (new->merge == MERGE_REPLACE) {
            if (report)
                log_warn(info->keymap->ctx,
                         "Multiple definitions for \"%s\"; "
                         "Earlier interpretation ignored\n",
                         siText(new, info));
            *old = *new;
            return true;
        }

        if (UseNewInterpField(SI_FIELD_VIRTUAL_MOD, old, new, report,
                              &collide)) {
            old->interp.virtual_mod = new->interp.virtual_mod;
            old->defined |= SI_FIELD_VIRTUAL_MOD;
        }
        if (UseNewInterpField(SI_FIELD_ACTION, old, new, report,
                              &collide)) {
            old->interp.act = new->interp.act;
            old->defined |= SI_FIELD_ACTION;
        }
        if (UseNewInterpField(SI_FIELD_AUTO_REPEAT, old, new, report,
                              &collide)) {
            old->interp.flags &= ~XkbSI_AutoRepeat;
            old->interp.flags |= (new->interp.flags & XkbSI_AutoRepeat);
            old->defined |= SI_FIELD_AUTO_REPEAT;
        }
        if (UseNewInterpField(SI_FIELD_LEVEL_ONE_ONLY, old, new, report,
                              &collide)) {
            old->interp.match &= ~XkbSI_LevelOneOnly;
            old->interp.match |= (new->interp.match & XkbSI_LevelOneOnly);
            old->defined |= SI_FIELD_LEVEL_ONE_ONLY;
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

    darray_append(info->interps, *new);
    return true;
}


/***====================================================================***/

static bool
ResolveStateAndPredicate(ExprDef *expr, unsigned *pred_rtrn,
                         xkb_mod_mask_t *mods_rtrn, CompatInfo *info)
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
UseNewLEDField(enum led_field field, LEDInfo *old, LEDInfo *new,
               bool report, enum led_field *collide)
{
    if (!(old->defined & field))
        return true;

    if (new->defined & field) {
        if (report)
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
    enum led_field collide;
    struct xkb_context *ctx = info->keymap->ctx;
    int verbosity = xkb_get_log_verbosity(ctx);

    darray_foreach(old, info->leds) {
        bool report;

        if (old->name != new->name)
            continue;

        if (old->mods == new->mods &&
            old->groups == new->groups &&
            old->ctrls == new->ctrls &&
            old->which_mods == new->which_mods &&
            old->which_groups == new->which_groups) {
            old->defined |= new->defined;
            return true;
        }

        report = ((old->file_id == new->file_id && verbosity > 0) ||
                  verbosity > 9);

        if (new->merge == MERGE_REPLACE) {
            if (report)
                log_warn(info->keymap->ctx,
                         "Map for indicator %s redefined; "
                         "Earlier definition ignored\n",
                         xkb_atom_text(ctx, old->name));
            *old = *new;
            return true;
        }

        collide = 0;
        if (UseNewLEDField(LED_FIELD_MODS, old, new, report, &collide)) {
            old->which_mods = new->which_mods;
            old->mods = new->mods;
            old->defined |= LED_FIELD_MODS;
        }
        if (UseNewLEDField(LED_FIELD_GROUPS, old, new, report, &collide)) {
            old->which_groups = new->which_groups;
            old->groups = new->groups;
            old->defined |= LED_FIELD_GROUPS;
        }
        if (UseNewLEDField(LED_FIELD_CTRLS, old, new, report, &collide)) {
            old->ctrls = new->ctrls;
            old->defined |= LED_FIELD_CTRLS;
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

    darray_append(info->leds, *new);
    return true;
}

static void
MergeIncludedCompatMaps(CompatInfo *into, CompatInfo *from,
                        enum merge_mode merge)
{
    SymInterpInfo *si;
    LEDInfo *led;

    if (from->errorCount > 0) {
        into->errorCount += from->errorCount;
        return;
    }

    if (into->name == NULL) {
        into->name = from->name;
        from->name = NULL;
    }

    darray_foreach(si, from->interps) {
        si->merge = (merge == MERGE_DEFAULT ? si->merge : merge);
        if (!AddInterp(into, si))
            into->errorCount++;
    }

    darray_foreach(led, from->leds) {
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
        FreeXkbFile(rtrn);
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

static bool
SetInterpField(CompatInfo *info, SymInterpInfo *si, const char *field,
               ExprDef *arrayNdx, ExprDef *value)
{
    struct xkb_keymap *keymap = info->keymap;
    xkb_mod_index_t ndx;

    if (istreq(field, "action")) {
        if (arrayNdx)
            return ReportSINotArray(info, si, field);

        if (!HandleActionDef(value, keymap, &si->interp.act, info->act))
            return false;

        si->defined |= SI_FIELD_ACTION;
    }
    else if (istreq(field, "virtualmodifier") ||
             istreq(field, "virtualmod")) {
        if (arrayNdx)
            return ReportSINotArray(info, si, field);

        if (!ResolveVirtualModifier(value, keymap, &ndx, &info->vmods))
            return ReportSIBadType(info, si, field, "virtual modifier");

        si->interp.virtual_mod = ndx;
        si->defined |= SI_FIELD_VIRTUAL_MOD;
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

        si->defined |= SI_FIELD_AUTO_REPEAT;
    }
    else if (istreq(field, "locking")) {
        log_dbg(info->keymap->ctx,
                "The \"locking\" field in symbol interpretation is unsupported; "
                "Ignored\n");
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

        si->defined |= SI_FIELD_LEVEL_ONE_ONLY;
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

static bool
SetIndicatorMapField(CompatInfo *info, LEDInfo *led,
                     const char *field, ExprDef *arrayNdx, ExprDef *value)
{
    bool ok = true;
    struct xkb_keymap *keymap = info->keymap;

    if (istreq(field, "modifiers") || istreq(field, "mods")) {
        if (arrayNdx)
            return ReportIndicatorNotArray(info, led, field);

        if (!ExprResolveVModMask(keymap, value, &led->mods))
            return ReportIndicatorBadType(info, led, field, "modifier mask");

        led->defined |= LED_FIELD_MODS;
    }
    else if (istreq(field, "groups")) {
        unsigned int mask;

        if (arrayNdx)
            return ReportIndicatorNotArray(info, led, field);

        if (!ExprResolveMask(keymap->ctx, value, &mask, groupNames))
            return ReportIndicatorBadType(info, led, field, "group mask");

        led->groups = mask;
        led->defined |= LED_FIELD_GROUPS;
    }
    else if (istreq(field, "controls") || istreq(field, "ctrls")) {
        unsigned int mask;

        if (arrayNdx)
            return ReportIndicatorNotArray(info, led, field);

        if (!ExprResolveMask(keymap->ctx, value, &mask, ctrlNames))
            return ReportIndicatorBadType(info, led, field,
                                          "controls mask");

        led->ctrls = mask;
        led->defined |= LED_FIELD_CTRLS;
    }
    else if (istreq(field, "allowexplicit")) {
        log_dbg(info->keymap->ctx,
                "The \"allowExplicit\" field in indicator statements is unsupported; "
                "Ignored\n");
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
        log_dbg(info->keymap->ctx,
                "The \"%s\" field in indicator statements is unsupported; "
                "Ignored\n", field);
    }
    else if (istreq(field, "index")) {
        /* Users should see this, it might cause unexpected behavior. */
        log_err(info->keymap->ctx,
                "The \"index\" field in indicator statements is unsupported; "
                "Ignored\n");
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

static bool
HandleGlobalVar(CompatInfo *info, VarDef *stmt)
{
    const char *elem, *field;
    ExprDef *ndx;
    bool ret;

    if (!ExprResolveLhs(info->keymap->ctx, stmt->name, &elem, &field, &ndx))
        ret = false;
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

static bool
HandleInterpBody(CompatInfo *info, VarDef *def, SymInterpInfo *si)
{
    bool ok = true;
    const char *elem, *field;
    ExprDef *arrayNdx;

    for (; def; def = (VarDef *) def->common.next) {
        if (def->name && def->name->op == EXPR_FIELD_REF) {
            log_err(info->keymap->ctx,
                    "Cannot set a global default value from within an interpret statement; "
                    "Move statements to the global file scope\n");
            ok = false;
            continue;
        }

        ok = ExprResolveLhs(info->keymap->ctx, def->name, &elem, &field,
                            &arrayNdx);
        if (!ok)
            continue;

        ok = SetInterpField(info, si, field, arrayNdx, def->value);
    }

    return ok;
}

static bool
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

    si = info->dflt;

    si.merge = merge = (def->merge == MERGE_DEFAULT ? merge : def->merge);

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
    bool ok;
    ParseCommon *stmt;

    merge = (merge == MERGE_DEFAULT ? MERGE_AUGMENT : merge);

    free(info->name);
    info->name = strdup_safe(file->name);

    for (stmt = file->defs; stmt; stmt = stmt->next) {
        switch (stmt->type) {
        case STMT_INCLUDE:
            ok = HandleIncludeCompatMap(info, (IncludeStmt *) stmt);
            break;
        case STMT_INTERP:
            ok = HandleInterpDef(info, (InterpDef *) stmt, merge);
            break;
        case STMT_GROUP_COMPAT:
            log_dbg(info->keymap->ctx,
                    "The \"group\" statement in compat is unsupported; "
                    "Ignored\n");
            ok = true;
            break;
        case STMT_INDICATOR_MAP:
            ok = HandleIndicatorMapDef(info, (IndicatorMapDef *) stmt, merge);
            break;
        case STMT_VAR:
            ok = HandleGlobalVar(info, (VarDef *) stmt);
            break;
        case STMT_VMOD:
            ok = HandleVModDef((VModDef *) stmt, info->keymap, merge,
                               &info->vmods);
            break;
        default:
            log_err(info->keymap->ctx,
                    "Interpretation files may not include other types; "
                    "Ignoring %s\n", stmt_type_to_string(stmt->type));
            ok = false;
            break;
        }

        if (!ok)
            info->errorCount++;

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

    darray_foreach(si, info->interps) {
        if (((si->interp.match & XkbSI_OpMask) != pred) ||
            (needSymbol && si->interp.sym == XKB_KEY_NoSymbol) ||
            (!needSymbol && si->interp.sym != XKB_KEY_NoSymbol))
            continue;

        darray_append(info->keymap->sym_interpret, si->interp);
    }
}

static void
CopyIndicatorMapDefs(CompatInfo *info)
{
    LEDInfo *led;
    struct xkb_indicator_map *im;
    xkb_led_index_t i;
    struct xkb_keymap *keymap = info->keymap;

    darray_foreach(led, info->leds) {
        const char *name = xkb_atom_text(keymap->ctx, led->name);

        /*
         * Find the indicator with the given name, if it was already
         * declared in keycodes.
         */
        im = NULL;
        for (i = 0; i < XkbNumIndicators; i++) {
            if (streq_not_null(keymap->indicator_names[i], name)) {
                im = &keymap->indicators[i];
                break;
            }
        }

        /* Not previously declared; create it with next free index. */
        if (!im) {
            log_dbg(keymap->ctx,
                    "Indicator name \"%s\" was not declared in the keycodes section; "
                    "Adding new indicator\n", name);

            for (i = 0; i < XkbNumIndicators; i++) {
                if (keymap->indicator_names[i])
                    continue;

                keymap->indicator_names[i] = name;
                im = &keymap->indicators[i];
                break;
            }

            /* Not place to put the it; ignore. */
            if (!im) {
                log_err(keymap->ctx,
                        "Too many indicators (maximum is %d); "
                        "Indicator name \"%s\" ignored\n",
                        XkbNumIndicators, name);
                continue;
            }
        }

        if (led->groups != 0 && led->which_groups == 0)
            im->which_groups = XkbIM_UseEffective;
        else
            im->which_groups = led->which_groups;
        im->groups = led->groups;
        if (led->mods != 0 && led->which_mods == 0)
            im->which_mods = XkbIM_UseEffective;
        else
            im->which_mods = led->which_mods;
        im->mods.mods = led->mods;
        im->ctrls = led->ctrls;
    }
}

static bool
CopyCompatToKeymap(struct xkb_keymap *keymap, CompatInfo *info)
{
    keymap->compat_section_name = strdup_safe(info->name);

    if (!darray_empty(info->interps)) {
        CopyInterps(info, true, XkbSI_Exactly);
        CopyInterps(info, true, XkbSI_AllOf | XkbSI_NoneOf);
        CopyInterps(info, true, XkbSI_AnyOf);
        CopyInterps(info, true, XkbSI_AnyOfOrNone);
        CopyInterps(info, false, XkbSI_Exactly);
        CopyInterps(info, false, XkbSI_AllOf | XkbSI_NoneOf);
        CopyInterps(info, false, XkbSI_AnyOf);
        CopyInterps(info, false, XkbSI_AnyOfOrNone);
    }

    CopyIndicatorMapDefs(info);

    return true;
}

bool
CompileCompatMap(XkbFile *file, struct xkb_keymap *keymap,
                 enum merge_mode merge)
{
    CompatInfo info;

    InitCompatInfo(&info, keymap, file->id);
    info.dflt.merge = merge;
    info.ledDflt.merge = merge;

    HandleCompatMapFile(&info, file, merge);
    if (info.errorCount != 0)
        goto err_info;

    if (!CopyCompatToKeymap(keymap, &info))
        goto err_info;

    ClearCompatInfo(&info);
    return true;

err_info:
    ClearCompatInfo(&info);
    return false;
}
