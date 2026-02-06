/*
 * For HPND:
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * For MIT:
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * SPDX-License-Identifier: HPND AND MIT
 */

#include "config.h"

#include <assert.h>
#include <limits.h>

#include "xkbcommon/xkbcommon.h"
#include "darray.h"
#include "keymap.h"
#include "xkbcomp-priv.h"
#include "ast.h"
#include "action.h"
#include "expr.h"
#include "include.h"
#include "text.h"
#include "vmod.h"
#include "util-mem.h"

enum si_field {
    SI_FIELD_VIRTUAL_MOD = (1 << 0),
    SI_FIELD_ACTION = (1 << 1),
    SI_FIELD_AUTO_REPEAT = (1 << 2),
    SI_FIELD_LEVEL_ONE_ONLY = (1 << 3),
};

typedef struct {
    enum si_field defined;
    enum merge_mode merge;

    struct xkb_sym_interpret interp;
} SymInterpInfo;

enum led_field {
    LED_FIELD_MODS = (1 << 0),
    LED_FIELD_GROUPS = (1 << 1),
    LED_FIELD_CTRLS = (1 << 2),
};

typedef struct {
    enum led_field defined;
    enum merge_mode merge;

    struct xkb_led led;
} LedInfo;

typedef struct {
    char *name;
    int errorCount;
    unsigned int include_depth;
    SymInterpInfo default_interp;
    darray(SymInterpInfo) interps;
    LedInfo default_led;
    LedInfo leds[XKB_MAX_LEDS];
    xkb_led_index_t num_leds;
    ActionsInfo default_actions;
    struct xkb_mod_set mods;

    const struct xkb_keymap_info *keymap_info;
    struct xkb_context *ctx;
} CompatInfo;

static const char *
siText(SymInterpInfo *si, CompatInfo *info)
{
    char *buf = xkb_context_get_buffer(info->ctx, 128);

    if (si == &info->default_interp)
        return "default";

    snprintf(buf, 128, "%s+%s(%s)",
             KeysymText(info->ctx, si->interp.sym),
             SIMatchText(si->interp.match),
             ModMaskText(info->ctx, MOD_BOTH, &info->mods, si->interp.mods));

    return buf;
}

static inline bool
ReportSINotArray(CompatInfo *info, SymInterpInfo *si, const char *field)
{
    return ReportNotArray(info->ctx, "symbol interpretation", field,
                          siText(si, info));
}

static inline bool
ReportSIBadType(CompatInfo *info, SymInterpInfo *si, const char *field,
                const char *wanted)
{
    return ReportBadType(info->ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                         "symbol interpretation", field,
                         siText(si, info), wanted);
}

static const char*
LEDText(CompatInfo *info, LedInfo *ledi)
{
    if (ledi == &info->default_led) {
        assert(xkb_atom_text(info->ctx, ledi->led.name) == NULL);
        return "default";
    } else {
        assert(xkb_atom_text(info->ctx, ledi->led.name) != NULL);
        return xkb_atom_text(info->ctx, ledi->led.name);
    }
}

static inline bool
ReportLedBadType(CompatInfo *info, LedInfo *ledi, const char *field,
                 const char *wanted)
{
    return ReportBadType(info->ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                         "indicator map",
                         field, LEDText(info, ledi), wanted);
}

static inline bool
ReportLedNotArray(CompatInfo *info, LedInfo *ledi, const char *field)
{
    return ReportNotArray(info->ctx, "indicator map", field,
                          LEDText(info, ledi));
}

static inline void
InitInterp(SymInterpInfo *info)
{
    info->merge = MERGE_DEFAULT; /* Unused */
    info->interp.virtual_mod = XKB_MOD_INVALID;
}

static inline void
InitLED(LedInfo *info)
{
    info->merge = MERGE_DEFAULT; /* Unused */
}

static void
InitCompatInfo(CompatInfo *info, const struct xkb_keymap_info *keymap_info,
               unsigned int include_depth, const struct xkb_mod_set *mods)
{
    memset(info, 0, sizeof(*info));
    info->ctx = keymap_info->keymap.ctx;
    info->keymap_info = keymap_info;
    info->include_depth = include_depth;
    InitActionsInfo(&info->default_actions);
    InitVMods(&info->mods, mods, include_depth > 0);
    InitInterp(&info->default_interp);
    InitLED(&info->default_led);
}

static void
ClearCompatInfo(CompatInfo *info)
{
    free(info->name);
    darray_free(info->interps);
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
UseNewInterpField(enum si_field field, enum si_field old, enum si_field new,
                  bool clobber, bool report, enum si_field *collide)
{
    if (!(old & field))
        return (new & field);

    if (new & field) {
        if (report)
            *collide |= field;

        return clobber;
    }

    return false;
}

static bool
MergeInterp(CompatInfo *info, SymInterpInfo *old, SymInterpInfo *new,
            bool same_file)
{
    const bool clobber = (new->merge != MERGE_AUGMENT);
    const int verbosity = xkb_context_get_log_verbosity(info->ctx);
    const bool report = (same_file && verbosity > 0) || verbosity > 9;
    enum si_field collide = 0;

    if (new->merge == MERGE_REPLACE) {
        if (report)
            log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                     "Multiple definitions for \"%s\"; "
                     "Earlier interpretation ignored\n",
                     siText(new, info));
        *old = *new;
        return true;
    }

    if (UseNewInterpField(SI_FIELD_VIRTUAL_MOD, old->defined, new->defined,
                          clobber, report, &collide)) {
        old->interp.virtual_mod = new->interp.virtual_mod;
        old->defined |= SI_FIELD_VIRTUAL_MOD;
    }
    if (UseNewInterpField(SI_FIELD_ACTION, old->defined, new->defined,
                          clobber, report, &collide)) {
        if (old->interp.num_actions > 1) {
                free(old->interp.a.actions);
            }
            old->interp.num_actions = new->interp.num_actions;
            if (new->interp.num_actions > 1) {
                old->interp.a.actions = new->interp.a.actions;
                new->interp.a.action =
                    (union xkb_action) { .type = ACTION_TYPE_NONE };
                new->interp.num_actions = 0;
            } else {
                old->interp.a.action = new->interp.a.action;
            }
        old->defined |= SI_FIELD_ACTION;
    }
    if (UseNewInterpField(SI_FIELD_AUTO_REPEAT, old->defined, new->defined,
                          clobber, report, &collide)) {
        old->interp.repeat = new->interp.repeat;
        old->defined |= SI_FIELD_AUTO_REPEAT;
    }
    if (UseNewInterpField(SI_FIELD_LEVEL_ONE_ONLY, old->defined, new->defined,
                          clobber, report, &collide)) {
        old->interp.level_one_only = new->interp.level_one_only;
        old->defined |= SI_FIELD_LEVEL_ONE_ONLY;
    }

    if (collide) {
        log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                 "Multiple interpretations of \"%s\"; "
                 "Using %s definition for duplicate fields\n",
                 siText(old, info),
                 (clobber ? "last" : "first"));
    }

    return true;
}

static bool
AddInterp(CompatInfo *info, SymInterpInfo *new, bool same_file)
{
    SymInterpInfo *old = FindMatchingInterp(info, new);
    if (old)
        return MergeInterp(info, old, new, same_file);

    darray_append(info->interps, *new);
    return true;
}

/***====================================================================***/

static bool
ResolveStateAndPredicate(ExprDef *expr, enum xkb_match_operation *pred_rtrn,
                         xkb_mod_mask_t *mods_rtrn, CompatInfo *info)
{
    if (expr == NULL) {
        *pred_rtrn = MATCH_ANY_OR_NONE;
        *mods_rtrn = MOD_REAL_MASK_ALL;
        return true;
    }

    *pred_rtrn = MATCH_EXACTLY;
    if (expr->common.type == STMT_EXPR_ACTION_DECL) {
        const char *pred_txt = xkb_atom_text(info->ctx, expr->action.name);
        unsigned int pred = 0;
        if (!LookupString(symInterpretMatchMaskNames, pred_txt, &pred) ||
            !expr->action.args || expr->action.args->common.next) {
            log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Illegal modifier predicate \"%s\"; Ignored\n", pred_txt);
            return false;
        }
        *pred_rtrn = (enum xkb_match_operation) pred;
        expr = expr->action.args;
    }
    else if (expr->common.type == STMT_EXPR_IDENT) {
        const char *pred_txt = xkb_atom_text(info->ctx, expr->ident.ident);
        if (pred_txt && istreq(pred_txt, "any")) {
            *pred_rtrn = MATCH_ANY;
            *mods_rtrn = MOD_REAL_MASK_ALL;
            return true;
        }
    }

    return ExprResolveModMask(info->ctx, expr, MOD_REAL, &info->mods,
                              mods_rtrn);
}

/***====================================================================***/

static bool
UseNewLEDField(enum led_field field, enum led_field old, enum led_field new,
               bool clobber, bool report, enum led_field *collide)
{
    if (!(old & field))
        return (new & field);

    if (new & field) {
        if (report)
            *collide |= field;

        return clobber;
    }

    return false;
}

static bool
MergeLedMap(CompatInfo *info, LedInfo *old, LedInfo *new, bool same_file)
{
    enum led_field collide;
    const bool clobber = (new->merge != MERGE_AUGMENT);
    const int verbosity = xkb_context_get_log_verbosity(info->ctx);
    const bool report = (same_file && verbosity > 0) || verbosity > 9;

    if (old->led.mods.mods == new->led.mods.mods &&
        old->led.pending_groups == new->led.pending_groups &&
        old->led.groups == new->led.groups &&
        old->led.ctrls == new->led.ctrls &&
        old->led.which_mods == new->led.which_mods &&
        old->led.which_groups == new->led.which_groups) {
        old->defined |= new->defined;
        return true;
    }

    if (new->merge == MERGE_REPLACE) {
        if (report)
            log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                     "Map for indicator %s redefined; "
                     "Earlier definition ignored\n",
                     LEDText(info, old));
        *old = *new;
        return true;
    }

    collide = 0;
    if (UseNewLEDField(LED_FIELD_MODS, old->defined, new->defined,
                       clobber, report, &collide)) {
        old->led.which_mods = new->led.which_mods;
        old->led.mods = new->led.mods;
        old->defined |= LED_FIELD_MODS;
    }
    if (UseNewLEDField(LED_FIELD_GROUPS, old->defined, new->defined,
                       clobber, report, &collide)) {
        old->led.which_groups = new->led.which_groups;
        old->led.groups = new->led.groups;
        old->led.pending_groups = new->led.pending_groups;
        old->defined |= LED_FIELD_GROUPS;
    }
    if (UseNewLEDField(LED_FIELD_CTRLS, old->defined, new->defined,
                       clobber, report, &collide)) {
        old->led.ctrls = new->led.ctrls;
        old->defined |= LED_FIELD_CTRLS;
    }

    if (collide) {
        log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                 "Map for indicator %s redefined; "
                 "Using %s definition for duplicate fields\n",
                 LEDText(info, old), (clobber ? "last" : "first"));
    }

    return true;
}

static bool
AddLedMap(CompatInfo *info, LedInfo *new, bool same_file)
{
    for (xkb_led_index_t i = 0; i < info->num_leds; i++) {
        LedInfo *old = &info->leds[i];

        if (old->led.name != new->led.name)
            continue;

        return MergeLedMap(info, old, new, same_file);
    }

    if (info->num_leds >= XKB_MAX_LEDS) {
        log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                "Too many LEDs defined (maximum %"PRIu32")\n",
                XKB_MAX_LEDS);
        return false;
    }
    info->leds[info->num_leds++] = *new;
    return true;
}

static void
MergeIncludedCompatMaps(CompatInfo *into, CompatInfo *from,
                        enum merge_mode merge)
{
    if (from->errorCount > 0) {
        into->errorCount += from->errorCount;
        return;
    }

    MergeModSets(into->ctx, &into->mods, &from->mods, merge);

    if (into->name == NULL) {
        into->name = steal(&from->name);
    }

    if (darray_empty(into->interps)) {
        into->interps = from->interps;
        darray_init(from->interps);
    }
    else {
        SymInterpInfo *si;
        darray_foreach(si, from->interps) {
            si->merge = merge;
            if (!AddInterp(into, si, false))
                into->errorCount++;
        }
    }

    if (into->num_leds == 0) {
        memcpy(into->leds, from->leds, sizeof(*from->leds) * from->num_leds);
        into->num_leds = from->num_leds;
        from->num_leds = 0;
    }
    else {
        for (xkb_led_index_t i = 0; i < from->num_leds; i++) {
            LedInfo *ledi = &from->leds[i];
            ledi->merge = merge;
            if (!AddLedMap(into, ledi, false))
                into->errorCount++;
        }
    }
}

static void
HandleCompatMapFile(CompatInfo *info, XkbFile *file);

static bool
HandleIncludeCompatMap(CompatInfo *info, IncludeStmt *include)
{
    CompatInfo included;

    if (ExceedsIncludeMaxDepth(info->ctx, info->include_depth)) {
        info->errorCount += 10;
        return false;
    }

    InitCompatInfo(&included, info->keymap_info, info->include_depth + 1,
                   &info->mods);
    included.name = steal(&include->stmt);

    for (IncludeStmt *stmt = include; stmt; stmt = stmt->next_incl) {
        CompatInfo next_incl;
        XkbFile *file;

        char path[PATH_MAX];
        file = ProcessIncludeFile(info->ctx, stmt, FILE_TYPE_COMPAT,
                                  path, sizeof(path));
        if (!file) {
            info->errorCount += 10;
            ClearCompatInfo(&included);
            return false;
        }

        InitCompatInfo(&next_incl, info->keymap_info, info->include_depth + 1,
                       &included.mods);
        next_incl.default_interp = info->default_interp;
        next_incl.default_led = info->default_led;

        HandleCompatMapFile(&next_incl, file);

        MergeIncludedCompatMaps(&included, &next_incl, stmt->merge);

        ClearCompatInfo(&next_incl);
        FreeXkbFile(file);
    }

    MergeIncludedCompatMaps(info, &included, include->merge);
    ClearCompatInfo(&included);

    return (info->errorCount == 0);
}

static bool
SetInterpField(CompatInfo *info, SymInterpInfo *si, const char *field,
               ExprDef *arrayNdx, ExprDef *value)
{
    if (istreq(field, "action")) {
        if (arrayNdx)
            return ReportSINotArray(info, si, field);

        if (value->common.type == STMT_EXPR_ACTION_LIST) {
            unsigned int num_actions = 0;
            for (ExprDef *act = value->actions.actions;
                 act; act = (ExprDef *) act->common.next)
                 num_actions++;

            if (num_actions > MAX_ACTIONS_PER_LEVEL) {
                log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                        "Interpret %s has too many actions; "
                        "expected max %u, got: %u\n",
                        siText(si, info), MAX_ACTIONS_PER_LEVEL, num_actions);
                return false;
            }

            si->interp.num_actions = 0;
            si->interp.a.action.type = ACTION_TYPE_NONE;

            /* Parse actions and add only defined actions */
            darray(union xkb_action) actions = darray_new();
            for (ExprDef *act = value->actions.actions;
                 act; act = (ExprDef *) act->common.next) {
                union xkb_action toAct = { 0 };
                if (!HandleActionDef(info->keymap_info, &info->default_actions,
                                     &info->mods, act, &toAct)) {
                    darray_free(actions);
                    return false;
                }
                if (toAct.type == ACTION_TYPE_NONE) {
                    /* Drop action */
                } else if (likely(num_actions == 1)) {
                    /* Only one action: do not allocate */
                    si->interp.num_actions = 1;
                    si->interp.a.action = toAct;
                } else {
                    darray_append(actions, toAct);
                }
            }
            switch (darray_size(actions)) {
            case 0:
                /* No action or exactly one action: already processed */
                assert(si->interp.num_actions <= 1);
                break;
            case 1:
                /* One action: some actions were dropped */
                si->interp.num_actions = 1;
                si->interp.a.action = darray_item(actions, 1);
                darray_free(actions);
                break;
            default:
                /* Multiple actions; no NoAction() left */
                darray_shrink(actions);
                si->interp.num_actions =
                    (xkb_action_count_t) darray_size(actions);
                darray_steal(actions, &si->interp.a.actions, NULL);
            }
        }
        else if (HandleActionDef(info->keymap_info, &info->default_actions,
                                 &info->mods, value, &si->interp.a.action))
            si->interp.num_actions =
                (si->interp.a.action.type != ACTION_TYPE_NONE);
        else
            return false;

        si->defined |= SI_FIELD_ACTION;
    }
    else if (istreq(field, "virtualmodifier") ||
             istreq(field, "virtualmod")) {
        if (arrayNdx)
            return ReportSINotArray(info, si, field);

        xkb_mod_index_t ndx = 0;
        if (!ExprResolveMod(info->ctx, value, MOD_VIRT, &info->mods, &ndx))
            return ReportSIBadType(info, si, field, "virtual modifier");

        si->interp.virtual_mod = ndx;
        si->defined |= SI_FIELD_VIRTUAL_MOD;
    }
    else if (istreq(field, "repeat")) {
        bool set = false;

        if (arrayNdx)
            return ReportSINotArray(info, si, field);

        if (!ExprResolveBoolean(info->ctx, value, &set))
            return ReportSIBadType(info, si, field, "boolean");

        si->interp.repeat = set;

        si->defined |= SI_FIELD_AUTO_REPEAT;
    }
    else if (istreq(field, "locking")) {
        log_dbg(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                "The \"locking\" field in symbol interpretation is unsupported; "
                "Ignored\n");
    }
    else if (istreq(field, "usemodmap") ||
             istreq(field, "usemodmapmods")) {
        uint32_t val = 0;

        if (arrayNdx)
            return ReportSINotArray(info, si, field);

        if (!ExprResolveEnum(info->ctx, value, &val, useModMapValueNames))
            return ReportSIBadType(info, si, field, "level specification");

        si->interp.level_one_only = val;
        si->defined |= SI_FIELD_LEVEL_ONE_ONLY;
    }
    else {
        return ReportBadField(info->ctx, "symbol interpretation", field,
                              siText(si, info));
    }

    return true;
}

static bool
SetLedMapField(CompatInfo *info, LedInfo *ledi, const char *field,
               ExprDef *arrayNdx, ExprDef **value_ptr)
{
    ExprDef * restrict const value = *value_ptr;
    bool ok = true;

    if (istreq(field, "modifiers") || istreq(field, "mods")) {
        if (arrayNdx)
            return ReportLedNotArray(info, ledi, field);

        if (!ExprResolveModMask(info->ctx, value, MOD_BOTH,
                                &info->mods, &ledi->led.mods.mods))
            return ReportLedBadType(info, ledi, field, "modifier mask");

        ledi->defined |= LED_FIELD_MODS;
    }
    else if (istreq(field, "groups")) {
        xkb_layout_mask_t mask = 0;

        if (arrayNdx)
            return ReportLedNotArray(info, ledi, field);

        bool pending = false;
        if (!ExprResolveGroupMask(info->keymap_info, value, &mask, &pending)) {
            if (pending) {
                ledi->led.pending_groups = true;
                const darray_size_t pending_index =
                    darray_size(*info->keymap_info->pending_computations);
                darray_append(
                    *info->keymap_info->pending_computations,
                    (struct pending_computation) {
                        .expr = *value_ptr,
                        .computed = false,
                        .value = 0,
                    }
                );
                *value_ptr = NULL;
                static_assert(sizeof(pending_index) == sizeof(mask),
                              "Cannot save pending computation");
                mask = pending_index;
            } else {
                return ReportLedBadType(info, ledi, field, "group mask");
            }
        } else {
            ledi->led.pending_groups = false;
        }

        ledi->led.groups = mask;
        ledi->defined |= LED_FIELD_GROUPS;
    }
    else if (istreq(field, "controls") || istreq(field, "ctrls")) {
        uint32_t mask = 0;

        if (arrayNdx)
            return ReportLedNotArray(info, ledi, field);

        if (!ExprResolveMask(info->ctx, value, &mask, ctrlMaskNames))
            return ReportLedBadType(info, ledi, field, "controls mask");

        ledi->led.ctrls = mask;
        ledi->defined |= LED_FIELD_CTRLS;
    }
    else if (istreq(field, "allowexplicit")) {
        log_dbg(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                "The \"allowExplicit\" field in indicator statements is unsupported; "
                "Ignored\n");
    }
    else if (istreq(field, "whichmodstate") ||
             istreq(field, "whichmodifierstate")) {
        uint32_t mask = 0;

        if (arrayNdx)
            return ReportLedNotArray(info, ledi, field);

        if (!ExprResolveMask(info->ctx, value, &mask,
                             modComponentMaskNames))
            return ReportLedBadType(info, ledi, field,
                                    "mask of modifier state components");

        ledi->led.which_mods = mask;
    }
    else if (istreq(field, "whichgroupstate")) {
        uint32_t mask = 0;

        if (arrayNdx)
            return ReportLedNotArray(info, ledi, field);

        if (!ExprResolveMask(info->ctx, value, &mask,
                             groupComponentMaskNames))
            return ReportLedBadType(info, ledi, field,
                                    "mask of group state components");

        ledi->led.which_groups = mask;
    }
    else if (istreq(field, "driveskbd") ||
             istreq(field, "driveskeyboard") ||
             istreq(field, "leddriveskbd") ||
             istreq(field, "leddriveskeyboard") ||
             istreq(field, "indicatordriveskbd") ||
             istreq(field, "indicatordriveskeyboard")) {
        log_dbg(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                "The \"%s\" field in indicator statements is unsupported; "
                "Ignored\n", field);
    }
    else if (istreq(field, "index")) {
        /* Users should see this, it might cause unexpected behavior. */
        log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                "The \"index\" field in indicator statements is unsupported; "
                "Ignored\n");
    }
    else {
        log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                "Unknown field \"%s\" in map for %s indicator; "
                "Definition ignored\n",
                field, LEDText(info, ledi));
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

    if (!ExprResolveLhs(info->ctx, stmt->name, &elem, &field, &ndx))
        ret = false;
    else if (elem && istreq(elem, "interpret")) {
        SymInterpInfo temp = {0};
        InitInterp(&temp);
        /* Do not replace the whole interpret, only the current field */
        temp.merge = (temp.merge == MERGE_REPLACE)
            ? MERGE_OVERRIDE
            : stmt->merge;
        ret = SetInterpField(info, &temp, field, ndx, stmt->value);
        MergeInterp(info, &info->default_interp, &temp, true);
    }
    else if (elem && istreq(elem, "indicator")) {
        LedInfo temp = {0};
        InitLED(&temp);
        /* Do not replace the whole LED, only the current field */
        temp.merge = (temp.merge == MERGE_REPLACE)
            ? MERGE_OVERRIDE
            : stmt->merge;
        ret = SetLedMapField(info, &temp, field, ndx, &stmt->value);
        MergeLedMap(info, &info->default_led, &temp, true);
    }
    else if (elem) {
        ret = SetDefaultActionField(info->keymap_info, &info->default_actions,
                                    &info->mods, elem, field, ndx,
                                    &stmt->value, stmt->merge);
    } else {
        log_err(info->ctx, XKB_ERROR_UNKNOWN_DEFAULT_FIELD,
                "Default defined for unknown field \"%s\"; Ignored\n", field);
        return false;
    }
    return ret;
}

static bool
HandleInterpBody(CompatInfo *info, VarDef *def, SymInterpInfo *si)
{
    bool ok = true;
    const char *elem, *field;
    ExprDef *arrayNdx;

    for (; def; def = (VarDef *) def->common.next) {
        ok = ExprResolveLhs(info->ctx, def->name, &elem, &field, &arrayNdx);
        if (!ok)
            continue;
        if (elem) {
            log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Cannot set a global default value for \"%s\" element from "
                    "within an interpret statement; "
                    "Move assignment to \"%s.%s\" to the global file scope\n",
                    elem, elem, field);
            ok = false;
            continue;
        }
        ok = SetInterpField(info, si, field, arrayNdx, def->value);
    }

    return ok;
}

static bool
HandleInterpDef(CompatInfo *info, InterpDef *def)
{
    enum xkb_match_operation pred;
    xkb_mod_mask_t mods;
    SymInterpInfo si;

    if (!ResolveStateAndPredicate(def->match, &pred, &mods, info)) {
        log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                "Couldn't determine matching modifiers; "
                "Symbol interpretation ignored\n");
        return false;
    }

    si = info->default_interp;
    si.merge = def->merge;
    si.interp.sym = def->sym;
    si.interp.match = pred;
    si.interp.mods = mods;

    if (!HandleInterpBody(info, def->def, &si)) {
        info->errorCount++;
        return false;
    }

    if (!AddInterp(info, &si, true)) {
        info->errorCount++;
        return false;
    }

    return true;
}

static bool
HandleLedMapDef(CompatInfo *info, LedMapDef *def)
{
    LedInfo ledi;
    VarDef *var;
    bool ok;

    ledi = info->default_led;
    ledi.merge = def->merge;
    ledi.led.name = def->name;

    ok = true;
    for (var = def->body; var != NULL; var = (VarDef *) var->common.next) {
        const char *elem, *field;
        ExprDef *arrayNdx;
        if (!ExprResolveLhs(info->ctx, var->name, &elem, &field, &arrayNdx)) {
            ok = false;
            continue;
        }

        if (elem) {
            log_err(info->ctx, XKB_ERROR_GLOBAL_DEFAULTS_WRONG_SCOPE,
                    "Cannot set defaults for \"%s\" element in indicator map; "
                    "Assignment to %s.%s ignored\n", elem, elem, field);
            ok = false;
        }
        else {
            ok = SetLedMapField(info, &ledi, field, arrayNdx, &var->value) && ok;
        }
    }

    if (ok)
        return AddLedMap(info, &ledi, true);

    return false;
}

static void
HandleCompatMapFile(CompatInfo *info, XkbFile *file)
{
    bool ok;

    free(info->name);
    info->name = strdup_safe(file->name);

    for (ParseCommon *stmt = file->defs; stmt; stmt = stmt->next) {
        switch (stmt->type) {
        case STMT_INCLUDE:
            ok = HandleIncludeCompatMap(info, (IncludeStmt *) stmt);
            break;
        case STMT_INTERP:
            ok = HandleInterpDef(info, (InterpDef *) stmt);
            break;
        case STMT_GROUP_COMPAT:
            log_dbg(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                    "The \"group\" statement in compat is unsupported; "
                    "Ignored\n");
            ok = true;
            break;
        case STMT_LED_MAP:
            ok = HandleLedMapDef(info, (LedMapDef *) stmt);
            break;
        case STMT_VAR:
            ok = HandleGlobalVar(info, (VarDef *) stmt);
            break;
        case STMT_VMOD:
            ok = HandleVModDef(info->ctx, &info->mods, (VModDef *) stmt);
            break;
        default:
            log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Compat files may not include other types; "
                    "Ignoring %s\n", stmt_type_to_string(stmt->type));
            ok = false;
            break;
        }

        if (!ok)
            info->errorCount++;

        if (info->errorCount > 10) {
            log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Abandoning compatibility map \"%s\"\n",
                    safe_map_name(file));
            break;
        }
    }
}

/* Temporary struct for CopyInterps. */
struct collect {
    darray(struct xkb_sym_interpret) sym_interprets;
};

static void
CopyInterps(CompatInfo *info, bool needSymbol, enum xkb_match_operation pred,
            struct collect *collect)
{
    SymInterpInfo *si;

    darray_foreach(si, info->interps)
        if (si->interp.match == pred &&
            (si->interp.sym != XKB_KEY_NoSymbol) == needSymbol)
            darray_append(collect->sym_interprets, si->interp);
}

static void
CopyLedMapDefsToKeymap(struct xkb_keymap *keymap, CompatInfo *info)
{
    for (xkb_led_index_t idx = 0; idx < info->num_leds; idx++) {
        LedInfo *ledi = &info->leds[idx];
        xkb_led_index_t i;
        struct xkb_led *led;

        /*
         * Find the LED with the given name, if it was already declared
         * in keycodes.
         */
        xkb_leds_enumerate(i, led, keymap)
            if (led->name == ledi->led.name)
                break;

        /* Not previously declared; create it with next free index. */
        if (i >= keymap->num_leds) {
            log_dbg(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Indicator name \"%s\" was not declared in the keycodes "
                    "section; Adding new indicator\n",
                    LEDText(info, ledi));

            xkb_leds_enumerate(i, led, keymap)
                if (led->name == XKB_ATOM_NONE)
                    break;

            if (i >= keymap->num_leds) {
                /* Not place to put it; ignore. */
                if (i >= XKB_MAX_LEDS) {
                    log_err(keymap->ctx, XKB_LOG_MESSAGE_NO_ID,
                            "Too many indicators (maximum is %"PRIu32"); "
                            "Indicator name \"%s\" ignored\n",
                            XKB_MAX_LEDS, LEDText(info, ledi));
                    continue;
                }

                /* Add a new LED. */
                led = &keymap->leds[keymap->num_leds++];
            }
        }

        *led = ledi->led;
        /* Assume pending `groups` computation does not result in 0 */
        if (led->which_groups == 0 && (led->groups != 0 || led->pending_groups))
            led->which_groups = XKB_STATE_LAYOUT_EFFECTIVE;
        if (led->which_mods == 0 && led->mods.mods != 0)
            led->which_mods = XKB_STATE_MODS_EFFECTIVE;
    }
}

static bool
CopyCompatToKeymap(struct xkb_keymap *keymap, CompatInfo *info)
{
    keymap->compat_section_name = strdup_safe(info->name);
    XkbEscapeMapName(keymap->compat_section_name);

    keymap->mods = info->mods;

    if (!darray_empty(info->interps)) {
        struct collect collect;
        darray_init(collect.sym_interprets);

        /* Most specific to least specific. */
        CopyInterps(info, true, MATCH_EXACTLY, &collect);
        CopyInterps(info, true, MATCH_ALL, &collect);
        CopyInterps(info, true, MATCH_NONE, &collect);
        CopyInterps(info, true, MATCH_ANY, &collect);
        CopyInterps(info, true, MATCH_ANY_OR_NONE, &collect);
        CopyInterps(info, false, MATCH_EXACTLY, &collect);
        CopyInterps(info, false, MATCH_ALL, &collect);
        CopyInterps(info, false, MATCH_NONE, &collect);
        CopyInterps(info, false, MATCH_ANY, &collect);
        CopyInterps(info, false, MATCH_ANY_OR_NONE, &collect);

        darray_steal(collect.sym_interprets,
                     &keymap->sym_interprets, &keymap->num_sym_interprets);
    }

    CopyLedMapDefsToKeymap(keymap, info);

    return true;
}

bool
CompileCompatMap(XkbFile *file, struct xkb_keymap_info *keymap_info)
{
    CompatInfo info;

    InitCompatInfo(&info, keymap_info, 0, &keymap_info->keymap.mods);

    if (file != NULL)
        HandleCompatMapFile(&info, file);

    if (info.errorCount != 0)
        goto err_info;

    if (!CopyCompatToKeymap(&keymap_info->keymap, &info))
        goto err_info;

    ClearCompatInfo(&info);
    return true;

err_info:
    ClearCompatInfo(&info);
    return false;
}
