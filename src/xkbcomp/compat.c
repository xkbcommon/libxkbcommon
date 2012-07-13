/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

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

#include "xkbcomp-priv.h"
#include "parseutils.h"
#include "action.h"
#include "indicators.h"
#include "vmod.h"

typedef struct _SymInterpInfo
{
    CommonInfo defs;
    struct xkb_sym_interpret interp;
} SymInterpInfo;

#define	_SI_VirtualMod		(1<<0)
#define	_SI_Action		(1<<1)
#define	_SI_AutoRepeat		(1<<2)
#define	_SI_LockingKey		(1<<3)
#define	_SI_LevelOneOnly	(1<<4)

typedef struct _GroupCompatInfo
{
    unsigned char fileID;
    unsigned char merge;
    bool defined;
    unsigned char real_mods;
    xkb_atom_t vmods;
} GroupCompatInfo;

typedef struct _CompatInfo
{
    char *name;
    unsigned fileID;
    int errorCount;
    int nInterps;
    SymInterpInfo *interps;
    SymInterpInfo dflt;
    LEDInfo ledDflt;
    GroupCompatInfo groupCompat[XkbNumKbdGroups];
    LEDInfo *leds;
    VModInfo vmods;
    ActionInfo *act;
    struct xkb_keymap *keymap;
} CompatInfo;

/***====================================================================***/

#define	ReportSINotArray(si,f,i) \
	ReportNotArray("symbol interpretation",(f),siText((si),(i)))
#define	ReportSIBadType(si,f,w,i) \
	ReportBadType("symbol interpretation",(f),siText((si),(i)),(w))

/***====================================================================***/

static const char *
siText(SymInterpInfo * si, CompatInfo * info)
{
    static char buf[128];

    if (si == &info->dflt)
    {
        snprintf(buf, sizeof(buf), "default");
    }
    else
    {
        snprintf(buf, sizeof(buf), "%s+%s(%s)",
                XkbcKeysymText(si->interp.sym),
                XkbcSIMatchText(si->interp.match),
                XkbcModMaskText(si->interp.mods, false));
    }
    return buf;
}

static void
InitCompatInfo(CompatInfo *info, struct xkb_keymap *keymap)
{
    unsigned int i;

    info->keymap = keymap;
    info->name = NULL;
    info->fileID = 0;
    info->errorCount = 0;
    info->nInterps = 0;
    info->interps = NULL;
    info->act = NULL;
    info->dflt.defs.fileID = info->fileID;
    info->dflt.defs.defined = 0;
    info->dflt.defs.merge = MERGE_OVERRIDE;
    info->dflt.interp.flags = 0;
    info->dflt.interp.virtual_mod = XkbNoModifier;
    info->dflt.interp.act.type = XkbSA_NoAction;
    for (i = 0; i < sizeof(info->dflt.interp.act.any.data); i++)
        info->dflt.interp.act.any.data[i] = 0;
    ClearIndicatorMapInfo(keymap->ctx, &info->ledDflt);
    info->ledDflt.defs.fileID = info->fileID;
    info->ledDflt.defs.defined = 0;
    info->ledDflt.defs.merge = MERGE_OVERRIDE;
    memset(&info->groupCompat[0], 0,
           XkbNumKbdGroups * sizeof(GroupCompatInfo));
    info->leds = NULL;
    InitVModInfo(&info->vmods, keymap);
}

static void
ClearCompatInfo(CompatInfo *info, struct xkb_keymap *keymap)
{
    unsigned int i;
    ActionInfo *next;

    free(info->name);
    info->name = NULL;
    info->dflt.defs.defined = 0;
    info->dflt.defs.merge = MERGE_AUGMENT;
    info->dflt.interp.flags = 0;
    info->dflt.interp.virtual_mod = XkbNoModifier;
    info->dflt.interp.act.type = XkbSA_NoAction;
    for (i = 0; i < sizeof(info->dflt.interp.act.any.data); i++)
        info->dflt.interp.act.any.data[i] = 0;
    ClearIndicatorMapInfo(keymap->ctx, &info->ledDflt);
    info->nInterps = 0;
    info->interps = ClearCommonInfo(&info->interps->defs);
    memset(&info->groupCompat[0], 0,
           XkbNumKbdGroups * sizeof(GroupCompatInfo));
    info->leds = ClearCommonInfo(&info->leds->defs);
    while (info->act) {
            next = info->act->next;
            free(info->act);
            info->act = next;
    }
    ClearVModInfo(&info->vmods, keymap);
}

static SymInterpInfo *
NextInterp(CompatInfo * info)
{
    SymInterpInfo *si;

    si = uTypedAlloc(SymInterpInfo);
    if (si)
    {
        memset(si, 0, sizeof(SymInterpInfo));
        info->interps = AddCommonInfo(&info->interps->defs, &si->defs);
        info->nInterps++;
    }
    return si;
}

static SymInterpInfo *
FindMatchingInterp(CompatInfo * info, SymInterpInfo * new)
{
    SymInterpInfo *old;

    for (old = info->interps; old != NULL;
         old = (SymInterpInfo *) old->defs.next)
    {
        if ((old->interp.sym == new->interp.sym) &&
            (old->interp.mods == new->interp.mods) &&
            (old->interp.match == new->interp.match))
        {
            return old;
        }
    }
    return NULL;
}

static bool
AddInterp(CompatInfo * info, SymInterpInfo * new)
{
    unsigned collide;
    SymInterpInfo *old;

    collide = 0;
    old = FindMatchingInterp(info, new);
    if (old != NULL)
    {
        if (new->defs.merge == MERGE_REPLACE)
        {
            SymInterpInfo *next = (SymInterpInfo *) old->defs.next;
            if (((old->defs.fileID == new->defs.fileID)
                 && (warningLevel > 0)) || (warningLevel > 9))
            {
                WARN("Multiple definitions for \"%s\"\n", siText(new, info));
                ACTION("Earlier interpretation ignored\n");
            }
            *old = *new;
            old->defs.next = &next->defs;
            return true;
        }
        if (UseNewField(_SI_VirtualMod, &old->defs, &new->defs, &collide))
        {
            old->interp.virtual_mod = new->interp.virtual_mod;
            old->defs.defined |= _SI_VirtualMod;
        }
        if (UseNewField(_SI_Action, &old->defs, &new->defs, &collide))
        {
            old->interp.act = new->interp.act;
            old->defs.defined |= _SI_Action;
        }
        if (UseNewField(_SI_AutoRepeat, &old->defs, &new->defs, &collide))
        {
            old->interp.flags &= ~XkbSI_AutoRepeat;
            old->interp.flags |= (new->interp.flags & XkbSI_AutoRepeat);
            old->defs.defined |= _SI_AutoRepeat;
        }
        if (UseNewField(_SI_LockingKey, &old->defs, &new->defs, &collide))
        {
            old->interp.flags &= ~XkbSI_LockingKey;
            old->interp.flags |= (new->interp.flags & XkbSI_LockingKey);
            old->defs.defined |= _SI_LockingKey;
        }
        if (UseNewField(_SI_LevelOneOnly, &old->defs, &new->defs, &collide))
        {
            old->interp.match &= ~XkbSI_LevelOneOnly;
            old->interp.match |= (new->interp.match & XkbSI_LevelOneOnly);
            old->defs.defined |= _SI_LevelOneOnly;
        }
        if (collide)
        {
            WARN("Multiple interpretations of \"%s\"\n", siText(new, info));
            ACTION("Using %s definition for duplicate fields\n",
                    (new->defs.merge != MERGE_AUGMENT ? "last" : "first"));
        }
        return true;
    }
    old = new;
    if ((new = NextInterp(info)) == NULL)
        return false;
    *new = *old;
    new->defs.next = NULL;
    return true;
}

static bool
AddGroupCompat(CompatInfo * info, unsigned group, GroupCompatInfo * newGC)
{
    GroupCompatInfo *gc;
    enum merge_mode merge;

    merge = newGC->merge;
    gc = &info->groupCompat[group];
    if (((gc->real_mods == newGC->real_mods) && (gc->vmods == newGC->vmods)))
    {
        return true;
    }
    if (((gc->fileID == newGC->fileID) && (warningLevel > 0))
        || (warningLevel > 9))
    {
        WARN("Compat map for group %d redefined\n", group + 1);
        ACTION("Using %s definition\n",
                (merge == MERGE_AUGMENT ? "old" : "new"));
    }
    if (newGC->defined && (merge != MERGE_AUGMENT || !gc->defined))
        *gc = *newGC;
    return true;
}

/***====================================================================***/

static bool
ResolveStateAndPredicate(ExprDef * expr,
                         unsigned *pred_rtrn,
                         unsigned *mods_rtrn, CompatInfo * info)
{
    ExprResult result;

    if (expr == NULL)
    {
        *pred_rtrn = XkbSI_AnyOfOrNone;
        *mods_rtrn = ~0;
        return true;
    }

    *pred_rtrn = XkbSI_Exactly;
    if (expr->op == ExprActionDecl)
    {
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
        else
        {
            ERROR("Illegal modifier predicate \"%s\"\n", pred_txt);
            ACTION("Ignored\n");
            return false;
        }
        expr = expr->value.action.args;
    }
    else if (expr->op == ExprIdent)
    {
        const char *pred_txt = xkb_atom_text(info->keymap->ctx,
                                             expr->value.str);
        if ((pred_txt) && (strcasecmp(pred_txt, "any") == 0))
        {
            *pred_rtrn = XkbSI_AnyOf;
            *mods_rtrn = 0xff;
            return true;
        }
    }

    if (ExprResolveModMask(info->keymap->ctx, expr, &result))
    {
        *mods_rtrn = result.uval;
        return true;
    }
    return false;
}

/***====================================================================***/

static void
MergeIncludedCompatMaps(CompatInfo * into, CompatInfo * from, enum merge_mode merge)
{
    SymInterpInfo *si;
    LEDInfo *led, *rtrn, *next;
    GroupCompatInfo *gcm;
    int i;

    if (from->errorCount > 0)
    {
        into->errorCount += from->errorCount;
        return;
    }
    if (into->name == NULL)
    {
        into->name = from->name;
        from->name = NULL;
    }
    for (si = from->interps; si; si = (SymInterpInfo *) si->defs.next)
    {
        if (merge != MERGE_DEFAULT)
            si->defs.merge = merge;
        if (!AddInterp(into, si))
            into->errorCount++;
    }
    for (i = 0, gcm = &from->groupCompat[0]; i < XkbNumKbdGroups; i++, gcm++)
    {
        if (merge != MERGE_DEFAULT)
            gcm->merge = merge;
        if (!AddGroupCompat(into, i, gcm))
            into->errorCount++;
    }
    for (led = from->leds; led != NULL; led = next)
    {
        next = (LEDInfo *) led->defs.next;
        if (merge != MERGE_DEFAULT)
            led->defs.merge = merge;
        rtrn = AddIndicatorMap(from->keymap, into->leds, led);
        if (rtrn != NULL)
            into->leds = rtrn;
        else
            into->errorCount++;
    }
}

static void
HandleCompatMapFile(XkbFile *file, struct xkb_keymap *keymap, enum merge_mode merge,
                    CompatInfo *info);

static bool
HandleIncludeCompatMap(IncludeStmt *stmt, struct xkb_keymap *keymap,
                       CompatInfo *info)
{
    unsigned newMerge;
    XkbFile *rtrn;
    CompatInfo included;
    bool haveSelf;

    haveSelf = false;
    if ((stmt->file == NULL) && (stmt->map == NULL))
    {
        haveSelf = true;
        included = *info;
        memset(info, 0, sizeof(CompatInfo));
    }
    else if (ProcessIncludeFile(keymap->ctx, stmt, FILE_TYPE_COMPAT, &rtrn,
                                &newMerge))
    {
        InitCompatInfo(&included, keymap);
        included.fileID = rtrn->id;
        included.dflt = info->dflt;
        included.dflt.defs.fileID = rtrn->id;
        included.dflt.defs.merge = newMerge;
        included.ledDflt.defs.fileID = rtrn->id;
        included.ledDflt.defs.merge = newMerge;
        included.act = info->act;
        HandleCompatMapFile(rtrn, keymap, MERGE_OVERRIDE, &included);
        if (stmt->stmt != NULL)
        {
            free(included.name);
            included.name = stmt->stmt;
            stmt->stmt = NULL;
        }
        if (info->act != NULL)
                included.act = NULL;
        FreeXKBFile(rtrn);
    }
    else
    {
        info->errorCount += 10;
        return false;
    }
    if ((stmt->next != NULL) && (included.errorCount < 1))
    {
        IncludeStmt *next;
        unsigned op;
        CompatInfo next_incl;

        for (next = stmt->next; next != NULL; next = next->next)
        {
            if ((next->file == NULL) && (next->map == NULL))
            {
                haveSelf = true;
                MergeIncludedCompatMaps(&included, info, next->merge);
                ClearCompatInfo(info, keymap);
            }
            else if (ProcessIncludeFile(keymap->ctx, next, FILE_TYPE_COMPAT,
                                        &rtrn, &op))
            {
                InitCompatInfo(&next_incl, keymap);
                next_incl.fileID = rtrn->id;
                next_incl.dflt = info->dflt;
                next_incl.dflt.defs.fileID = rtrn->id;
                next_incl.dflt.defs.merge = op;
                next_incl.ledDflt.defs.fileID = rtrn->id;
                next_incl.ledDflt.defs.merge = op;
                next_incl.act = info->act;
                HandleCompatMapFile(rtrn, keymap, MERGE_OVERRIDE, &next_incl);
                MergeIncludedCompatMaps(&included, &next_incl, op);
                if (info->act != NULL)
                        next_incl.act = NULL;
                ClearCompatInfo(&next_incl, keymap);
                FreeXKBFile(rtrn);
            }
            else
            {
                info->errorCount += 10;
                return false;
            }
        }
    }
    if (haveSelf)
        *info = included;
    else
    {
        MergeIncludedCompatMaps(info, &included, newMerge);
        ClearCompatInfo(&included, keymap);
    }
    return (info->errorCount == 0);
}

static const LookupEntry useModMapValues[] = {
    {"levelone", 1},
    {"level1", 1},
    {"anylevel", 0},
    {"any", 0},
    {NULL, 0}
};

static int
SetInterpField(SymInterpInfo *si, struct xkb_keymap *keymap, char *field,
               ExprDef *arrayNdx, ExprDef *value, CompatInfo *info)
{
    int ok = 1;
    ExprResult tmp;

    if (strcasecmp(field, "action") == 0)
    {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = HandleActionDef(value, keymap, &si->interp.act.any, info->act);
        if (ok)
            si->defs.defined |= _SI_Action;
    }
    else if ((strcasecmp(field, "virtualmodifier") == 0) ||
             (strcasecmp(field, "virtualmod") == 0))
    {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = ResolveVirtualModifier(value, keymap, &tmp, &info->vmods);
        if (ok)
        {
            si->interp.virtual_mod = tmp.uval;
            si->defs.defined |= _SI_VirtualMod;
        }
        else
            return ReportSIBadType(si, field, "virtual modifier", info);
    }
    else if (strcasecmp(field, "repeat") == 0)
    {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = ExprResolveBoolean(keymap->ctx, value, &tmp);
        if (ok)
        {
            if (tmp.uval)
                si->interp.flags |= XkbSI_AutoRepeat;
            else
                si->interp.flags &= ~XkbSI_AutoRepeat;
            si->defs.defined |= _SI_AutoRepeat;
        }
        else
            return ReportSIBadType(si, field, "boolean", info);
    }
    else if (strcasecmp(field, "locking") == 0)
    {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = ExprResolveBoolean(keymap->ctx, value, &tmp);
        if (ok)
        {
            if (tmp.uval)
                si->interp.flags |= XkbSI_LockingKey;
            else
                si->interp.flags &= ~XkbSI_LockingKey;
            si->defs.defined |= _SI_LockingKey;
        }
        else
            return ReportSIBadType(si, field, "boolean", info);
    }
    else if ((strcasecmp(field, "usemodmap") == 0) ||
             (strcasecmp(field, "usemodmapmods") == 0))
    {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = ExprResolveEnum(keymap->ctx, value, &tmp, useModMapValues);
        if (ok)
        {
            if (tmp.uval)
                si->interp.match |= XkbSI_LevelOneOnly;
            else
                si->interp.match &= ~XkbSI_LevelOneOnly;
            si->defs.defined |= _SI_LevelOneOnly;
        }
        else
            return ReportSIBadType(si, field, "level specification", info);
    }
    else
    {
        ok = ReportBadField("symbol interpretation", field, siText(si, info));
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

    for (; def != NULL; def = (VarDef *) def->common.next)
    {
        if ((def->name) && (def->name->type == ExprFieldRef))
        {
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
HandleInterpDef(InterpDef *def, struct xkb_keymap *keymap, enum merge_mode merge,
                CompatInfo *info)
{
    unsigned pred, mods;
    SymInterpInfo si;

    if (!ResolveStateAndPredicate(def->match, &pred, &mods, info))
    {
        ERROR("Couldn't determine matching modifiers\n");
        ACTION("Symbol interpretation ignored\n");
        return false;
    }
    if (def->merge != MERGE_DEFAULT)
        merge = def->merge;

    si = info->dflt;
    si.defs.merge = merge;
    if (!LookupKeysym(def->sym, &si.interp.sym))
    {
        ERROR("Could not resolve keysym %s\n", def->sym);
        ACTION("Symbol interpretation ignored\n");
        return false;
    }
    si.interp.match = pred & XkbSI_OpMask;
    si.interp.mods = mods;
    if (!HandleInterpBody(def->def, keymap, &si, info))
    {
        info->errorCount++;
        return false;
    }

    if (!AddInterp(info, &si))
    {
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
    if (!XkbIsLegalGroup(def->group - 1))
    {
        ERROR("Keyboard group must be in the range 1..%d\n",
               XkbNumKbdGroups + 1);
        ACTION("Compatibility map for illegal group %d ignored\n",
                def->group);
        return false;
    }
    tmp.fileID = info->fileID;
    tmp.merge = merge;
    if (!ExprResolveVModMask(def->def, &val, keymap))
    {
        ERROR("Expected a modifier mask in group compatibility definition\n");
        ACTION("Ignoring illegal compatibility map for group %d\n",
                def->group);
        return false;
    }
    tmp.real_mods = val.uval & 0xff;
    tmp.vmods = (val.uval >> 8) & 0xffff;
    tmp.defined = true;
    return AddGroupCompat(info, def->group - 1, &tmp);
}

static void
HandleCompatMapFile(XkbFile *file, struct xkb_keymap *keymap, enum merge_mode merge,
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
        switch (stmt->stmtType)
        {
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
        {
            LEDInfo *rtrn;
            rtrn = HandleIndicatorMapDef((IndicatorMapDef *) stmt, keymap,
                                         &info->ledDflt, info->leds, merge);
            if (rtrn != NULL)
                info->leds = rtrn;
            else
                info->errorCount++;
        }
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
        if (info->errorCount > 10)
        {
#ifdef NOISY
            ERROR("Too many errors\n");
#endif
            ACTION("Abandoning compatibility map \"%s\"\n", file->topName);
            break;
        }
    }
}

static void
CopyInterps(CompatInfo * info,
            struct xkb_compat_map * compat, bool needSymbol, unsigned pred)
{
    SymInterpInfo *si;

    for (si = info->interps; si; si = (SymInterpInfo *) si->defs.next)
    {
        if (((si->interp.match & XkbSI_OpMask) != pred) ||
            (needSymbol && (si->interp.sym == XKB_KEY_NoSymbol)) ||
            ((!needSymbol) && (si->interp.sym != XKB_KEY_NoSymbol)))
            continue;
        darray_append(compat->sym_interpret, si->interp);
    }
}

bool
CompileCompatMap(XkbFile *file, struct xkb_keymap *keymap, enum merge_mode merge)
{
    int i;
    CompatInfo info;
    GroupCompatInfo *gcm;

    InitCompatInfo(&info, keymap);
    info.dflt.defs.merge = merge;
    info.ledDflt.defs.merge = merge;

    HandleCompatMapFile(file, keymap, merge, &info);

    if (info.errorCount != 0)
        goto err_info;

    if (XkbcAllocCompatMap(keymap, info.nInterps) != Success) {
        WSGO("Couldn't allocate compatibility map\n");
        goto err_info;
    }

    if (info.name) {
        if (XkbcAllocNames(keymap, 0, 0) != Success)
            goto err_info;
        keymap->names->compat = strdup(info.name);
    }

    if (info.nInterps > 0) {
        CopyInterps(&info, keymap->compat, true, XkbSI_Exactly);
        CopyInterps(&info, keymap->compat, true, XkbSI_AllOf | XkbSI_NoneOf);
        CopyInterps(&info, keymap->compat, true, XkbSI_AnyOf);
        CopyInterps(&info, keymap->compat, true, XkbSI_AnyOfOrNone);
        CopyInterps(&info, keymap->compat, false, XkbSI_Exactly);
        CopyInterps(&info, keymap->compat, false, XkbSI_AllOf | XkbSI_NoneOf);
        CopyInterps(&info, keymap->compat, false, XkbSI_AnyOf);
        CopyInterps(&info, keymap->compat, false, XkbSI_AnyOfOrNone);
    }

    for (i = 0, gcm = &info.groupCompat[0]; i < XkbNumKbdGroups; i++, gcm++) {
        if ((gcm->fileID != 0) || (gcm->real_mods != 0) || (gcm->vmods != 0)) {
            keymap->compat->groups[i].mask = gcm->real_mods;
            keymap->compat->groups[i].real_mods = gcm->real_mods;
            keymap->compat->groups[i].vmods = gcm->vmods;
        }
    }

    if (info.leds != NULL) {
        if (!CopyIndicatorMapDefs(keymap, info.leds))
            info.errorCount++;
        info.leds = NULL;
    }

    ClearCompatInfo(&info, keymap);
    return true;

err_info:
    ClearCompatInfo(&info, keymap);
    return false;
}

static uint32_t
VModsToReal(struct xkb_keymap *keymap, uint32_t vmodmask)
{
    uint32_t ret = 0;
    int i;

    if (!vmodmask)
        return 0;

    for (i = 0; i < XkbNumVirtualMods; i++) {
        if (!(vmodmask & (1 << i)))
            continue;
        ret |= keymap->server->vmods[i];
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
FindInterpForKey(struct xkb_keymap *keymap, xkb_keycode_t key,
                 uint32_t group, uint32_t level)
{
    struct xkb_sym_interpret *ret = NULL;
    struct xkb_sym_interpret *interp;
    const xkb_keysym_t *syms;
    int num_syms;

    num_syms = xkb_key_get_syms_by_level(keymap, key, group, level, &syms);
    if (num_syms == 0)
        return NULL;

    darray_foreach(interp, keymap->compat->sym_interpret) {
        uint32_t mods;
        bool found;

        if ((num_syms > 1 || interp->sym != syms[0]) &&
            interp->sym != XKB_KEY_NoSymbol)
            continue;

        if (level == 0 || !(interp->match & XkbSI_LevelOneOnly))
            mods = keymap->map->modmap[key];
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
ApplyInterpsToKey(struct xkb_keymap *keymap, xkb_keycode_t key)
{
#define INTERP_SIZE (8 * 4)
    struct xkb_sym_interpret *interps[INTERP_SIZE];
    union xkb_action *acts;
    uint32_t vmodmask = 0;
    int num_acts = 0;
    int group, level;
    int width = XkbKeyGroupsWidth(keymap, key);
    int i;

    /* If we've been told not to bind interps to this key, then don't. */
    if (keymap->server->explicit[key] & XkbExplicitInterpretMask)
        return true;

    for (i = 0; i < INTERP_SIZE; i++)
        interps[i] = NULL;

    for (group = 0; group < XkbKeyNumGroups(keymap, key); group++) {
        for (level = 0; level < XkbKeyGroupWidth(keymap, key, group); level++) {
            i = (group * width) + level;
            if (i >= INTERP_SIZE) /* XXX FIXME */
                return false;
            interps[i] = FindInterpForKey(keymap, key, group, level);
            if (interps[i])
                num_acts++;
        }
    }

    if (num_acts)
        num_acts = XkbKeyNumGroups(keymap, key) * width;
    acts = XkbcResizeKeyActions(keymap, key, num_acts);
    if (num_acts && !acts)
        return false;

    for (group = 0; group < XkbKeyNumGroups(keymap, key); group++) {
        for (level = 0; level < XkbKeyGroupWidth(keymap, key, group); level++) {
            struct xkb_sym_interpret *interp;

            i = (group * width) + level;
            interp = interps[i];

            /* Infer default key behaviours from the base level. */
            if (group == 0 && level == 0) {
                if (!(keymap->server->explicit[key] & XkbExplicitAutoRepeatMask) &&
                    (!interp || interp->flags & XkbSI_AutoRepeat))
                        keymap->ctrls->per_key_repeat[key / 8] |= (1 << (key % 8));
                if (!(keymap->server->explicit[key] & XkbExplicitBehaviorMask) &&
                    interp && (interp->flags & XkbSI_LockingKey))
                    keymap->server->behaviors[key].type = XkbKB_Lock;
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

    if (!(keymap->server->explicit[key] & XkbExplicitVModMapMask))
        keymap->server->vmodmap[key] = vmodmask;

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
    xkb_keycode_t key;
    int i;
    struct xkb_key_type *type;
    struct xkb_kt_map_entry *entry;

    /* Find all the interprets for the key and bind them to actions,
     * which will also update the vmodmap. */
    for (key = keymap->min_key_code; key <= keymap->max_key_code; key++)
        if (!ApplyInterpsToKey(keymap, key))
            return false;

    /* Update keymap->server->vmods, the virtual -> real mod mapping. */
    for (i = 0; i < XkbNumVirtualMods; i++)
        keymap->server->vmods[i] = 0;
    for (key = keymap->min_key_code; key <= keymap->max_key_code; key++) {
        if (!keymap->server->vmodmap[key])
            continue;
        for (i = 0; i < XkbNumVirtualMods; i++) {
            if (!(keymap->server->vmodmap[key] & (1 << i)))
                continue;
            keymap->server->vmods[i] |= keymap->map->modmap[key];
        }
    }

    /* Now update the level masks for all the types to reflect the vmods. */
    darray_foreach(type, keymap->map->types) {
        uint32_t mask = 0;
        int j;
        type->mods.mask = type->mods.real_mods;
        type->mods.mask |= VModsToReal(keymap, type->mods.vmods);
        for (j = 0; j < XkbNumVirtualMods; j++) {
            if (!(type->mods.vmods & (1 << j)))
                continue;
            mask |= keymap->server->vmods[j];
        }

        darray_foreach(entry, type->map)
            entry->mods.mask = entry->mods.real_mods |
                                VModsToReal(keymap, entry->mods.vmods);
    }

    /* Update action modifiers. */
    for (key = keymap->min_key_code; key <= keymap->max_key_code; key++) {
        union xkb_action *acts = XkbKeyActionsPtr(keymap, key);
        for (i = 0; i < XkbKeyNumActions(keymap, key); i++) {
            if (acts[i].any.type == XkbSA_NoAction)
                continue;
            UpdateActionMods(keymap, &acts[i], keymap->map->modmap[key]);
        }
    }

    /* Update group modifiers. */
    for (i = 0; i < XkbNumKbdGroups; i++) {
        struct xkb_mods *group = &keymap->compat->groups[i];
        group->mask = group->real_mods | VModsToReal(keymap, group->vmods);
    }

    /* Update vmod -> indicator maps. */
    for (i = 0; i < XkbNumIndicators; i++) {
        struct xkb_mods *led = &keymap->indicators->maps[i].mods;
        led->mask = led->real_mods | VModsToReal(keymap, led->vmods);
    }

    return true;
}
