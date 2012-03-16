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

#include <X11/Xos.h>
#include "xkbcomp.h"
#include "xkballoc.h"
#include "xkbmisc.h"
#include "expr.h"
#include "vmod.h"
#include "misc.h"
#include "indicators.h"
#include "action.h"
#include "parseutils.h"

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
    struct xkb_desc * xkb;
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
                XkbcModMaskText(si->interp.mods, False));
    }
    return buf;
}

static void
InitCompatInfo(CompatInfo * info, struct xkb_desc * xkb)
{
    int i;

    info->xkb = xkb;
    info->name = NULL;
    info->fileID = 0;
    info->errorCount = 0;
    info->nInterps = 0;
    info->interps = NULL;
    info->act = NULL;
    info->dflt.defs.fileID = info->fileID;
    info->dflt.defs.defined = 0;
    info->dflt.defs.merge = MergeOverride;
    info->dflt.interp.flags = 0;
    info->dflt.interp.virtual_mod = XkbNoModifier;
    info->dflt.interp.act.type = XkbSA_NoAction;
    for (i = 0; i < sizeof(info->dflt.interp.act.any.data); i++)
        info->dflt.interp.act.any.data[i] = 0;
    ClearIndicatorMapInfo(&info->ledDflt);
    info->ledDflt.defs.fileID = info->fileID;
    info->ledDflt.defs.defined = 0;
    info->ledDflt.defs.merge = MergeOverride;
    memset(&info->groupCompat[0], 0,
           XkbNumKbdGroups * sizeof(GroupCompatInfo));
    info->leds = NULL;
    InitVModInfo(&info->vmods, xkb);
}

static void
ClearCompatInfo(CompatInfo * info, struct xkb_desc * xkb)
{
    int i;
    ActionInfo *next;

    free(info->name);
    info->name = NULL;
    info->dflt.defs.defined = 0;
    info->dflt.defs.merge = MergeAugment;
    info->dflt.interp.flags = 0;
    info->dflt.interp.virtual_mod = XkbNoModifier;
    info->dflt.interp.act.type = XkbSA_NoAction;
    for (i = 0; i < sizeof(info->dflt.interp.act.any.data); i++)
        info->dflt.interp.act.any.data[i] = 0;
    ClearIndicatorMapInfo(&info->ledDflt);
    info->nInterps = 0;
    info->interps = (SymInterpInfo *) ClearCommonInfo(&info->interps->defs);
    memset(&info->groupCompat[0], 0,
           XkbNumKbdGroups * sizeof(GroupCompatInfo));
    info->leds = (LEDInfo *) ClearCommonInfo(&info->leds->defs);
    while (info->act) {
            next = info->act->next;
            free(info->act);
            info->act = next;
    }
    ClearVModInfo(&info->vmods, xkb);
}

static SymInterpInfo *
NextInterp(CompatInfo * info)
{
    SymInterpInfo *si;

    si = uTypedAlloc(SymInterpInfo);
    if (si)
    {
        memset(si, 0, sizeof(SymInterpInfo));
        info->interps =
            (SymInterpInfo *) AddCommonInfo(&info->interps->defs,
                                            (CommonInfo *) si);
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

static Bool
AddInterp(CompatInfo * info, SymInterpInfo * new)
{
    unsigned collide;
    SymInterpInfo *old;

    collide = 0;
    old = FindMatchingInterp(info, new);
    if (old != NULL)
    {
        if (new->defs.merge == MergeReplace)
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
            return True;
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
                    (new->defs.merge != MergeAugment ? "last" : "first"));
        }
        return True;
    }
    old = new;
    if ((new = NextInterp(info)) == NULL)
        return False;
    *new = *old;
    new->defs.next = NULL;
    return True;
}

static Bool
AddGroupCompat(CompatInfo * info, unsigned group, GroupCompatInfo * newGC)
{
    GroupCompatInfo *gc;
    unsigned merge;

    merge = newGC->merge;
    gc = &info->groupCompat[group];
    if (((gc->real_mods == newGC->real_mods) && (gc->vmods == newGC->vmods)))
    {
        return True;
    }
    if (((gc->fileID == newGC->fileID) && (warningLevel > 0))
        || (warningLevel > 9))
    {
        WARN("Compat map for group %d redefined\n", group + 1);
        ACTION("Using %s definition\n",
                (merge == MergeAugment ? "old" : "new"));
    }
    if (merge != MergeAugment)
        *gc = *newGC;
    return True;
}

/***====================================================================***/

static Bool
ResolveStateAndPredicate(ExprDef * expr,
                         unsigned *pred_rtrn,
                         unsigned *mods_rtrn, CompatInfo * info)
{
    ExprResult result;

    if (expr == NULL)
    {
        *pred_rtrn = XkbSI_AnyOfOrNone;
        *mods_rtrn = ~0;
        return True;
    }

    *pred_rtrn = XkbSI_Exactly;
    if (expr->op == ExprActionDecl)
    {
        const char *pred_txt = XkbcAtomText(expr->value.action.name);
        if (uStrCaseCmp(pred_txt, "noneof") == 0)
            *pred_rtrn = XkbSI_NoneOf;
        else if (uStrCaseCmp(pred_txt, "anyofornone") == 0)
            *pred_rtrn = XkbSI_AnyOfOrNone;
        else if (uStrCaseCmp(pred_txt, "anyof") == 0)
            *pred_rtrn = XkbSI_AnyOf;
        else if (uStrCaseCmp(pred_txt, "allof") == 0)
            *pred_rtrn = XkbSI_AllOf;
        else if (uStrCaseCmp(pred_txt, "exactly") == 0)
            *pred_rtrn = XkbSI_Exactly;
        else
        {
            ERROR("Illegal modifier predicate \"%s\"\n", pred_txt);
            ACTION("Ignored\n");
            return False;
        }
        expr = expr->value.action.args;
    }
    else if (expr->op == ExprIdent)
    {
        const char *pred_txt = XkbcAtomText(expr->value.str);
        if ((pred_txt) && (uStrCaseCmp(pred_txt, "any") == 0))
        {
            *pred_rtrn = XkbSI_AnyOf;
            *mods_rtrn = 0xff;
            return True;
        }
    }

    if (ExprResolveModMask(expr, &result))
    {
        *mods_rtrn = result.uval;
        return True;
    }
    return False;
}

/***====================================================================***/

static void
MergeIncludedCompatMaps(CompatInfo * into, CompatInfo * from, unsigned merge)
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
        if (merge != MergeDefault)
            si->defs.merge = merge;
        if (!AddInterp(into, si))
            into->errorCount++;
    }
    for (i = 0, gcm = &from->groupCompat[0]; i < XkbNumKbdGroups; i++, gcm++)
    {
        if (merge != MergeDefault)
            gcm->merge = merge;
        if (!AddGroupCompat(into, i, gcm))
            into->errorCount++;
    }
    for (led = from->leds; led != NULL; led = next)
    {
        next = (LEDInfo *) led->defs.next;
        if (merge != MergeDefault)
            led->defs.merge = merge;
        rtrn = AddIndicatorMap(into->leds, led);
        if (rtrn != NULL)
            into->leds = rtrn;
        else
            into->errorCount++;
    }
}

typedef void (*FileHandler) (XkbFile * /* rtrn */ ,
                             struct xkb_desc * /* xkb */ ,
                             unsigned /* merge */ ,
                             CompatInfo *       /* info */
    );

static Bool
HandleIncludeCompatMap(IncludeStmt * stmt,
                       struct xkb_desc * xkb, CompatInfo * info, FileHandler hndlr)
{
    unsigned newMerge;
    XkbFile *rtrn;
    CompatInfo included;
    Bool haveSelf;

    haveSelf = False;
    if ((stmt->file == NULL) && (stmt->map == NULL))
    {
        haveSelf = True;
        included = *info;
        memset(info, 0, sizeof(CompatInfo));
    }
    else if (ProcessIncludeFile(stmt, XkmCompatMapIndex, &rtrn, &newMerge))
    {
        InitCompatInfo(&included, xkb);
        included.fileID = rtrn->id;
        included.dflt = info->dflt;
        included.dflt.defs.fileID = rtrn->id;
        included.dflt.defs.merge = newMerge;
        included.ledDflt.defs.fileID = rtrn->id;
        included.ledDflt.defs.merge = newMerge;
        included.act = info->act;
        (*hndlr) (rtrn, xkb, MergeOverride, &included);
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
        return False;
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
                haveSelf = True;
                MergeIncludedCompatMaps(&included, info, next->merge);
                ClearCompatInfo(info, xkb);
            }
            else if (ProcessIncludeFile(next, XkmCompatMapIndex, &rtrn, &op))
            {
                InitCompatInfo(&next_incl, xkb);
                next_incl.fileID = rtrn->id;
                next_incl.dflt = info->dflt;
                next_incl.dflt.defs.fileID = rtrn->id;
                next_incl.dflt.defs.merge = op;
                next_incl.ledDflt.defs.fileID = rtrn->id;
                next_incl.ledDflt.defs.merge = op;
                next_incl.act = info->act;
                (*hndlr) (rtrn, xkb, MergeOverride, &next_incl);
                MergeIncludedCompatMaps(&included, &next_incl, op);
                if (info->act != NULL)
                        next_incl.act = NULL;
                ClearCompatInfo(&next_incl, xkb);
                FreeXKBFile(rtrn);
            }
            else
            {
                info->errorCount += 10;
                return False;
            }
        }
    }
    if (haveSelf)
        *info = included;
    else
    {
        MergeIncludedCompatMaps(info, &included, newMerge);
        ClearCompatInfo(&included, xkb);
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
SetInterpField(SymInterpInfo * si,
               struct xkb_desc * xkb,
               char *field,
               ExprDef * arrayNdx, ExprDef * value, CompatInfo * info)
{
    int ok = 1;
    ExprResult tmp;

    if (uStrCaseCmp(field, "action") == 0)
    {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = HandleActionDef(value, xkb, &si->interp.act.any, si->defs.merge,
                             info->act);
        if (ok)
            si->defs.defined |= _SI_Action;
    }
    else if ((uStrCaseCmp(field, "virtualmodifier") == 0) ||
             (uStrCaseCmp(field, "virtualmod") == 0))
    {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = ResolveVirtualModifier(value, xkb, &tmp, &info->vmods);
        if (ok)
        {
            si->interp.virtual_mod = tmp.uval;
            si->defs.defined |= _SI_VirtualMod;
        }
        else
            return ReportSIBadType(si, field, "virtual modifier", info);
    }
    else if (uStrCaseCmp(field, "repeat") == 0)
    {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = ExprResolveBoolean(value, &tmp);
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
    else if (uStrCaseCmp(field, "locking") == 0)
    {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = ExprResolveBoolean(value, &tmp);
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
    else if ((uStrCaseCmp(field, "usemodmap") == 0) ||
             (uStrCaseCmp(field, "usemodmapmods") == 0))
    {
        if (arrayNdx != NULL)
            return ReportSINotArray(si, field, info);
        ok = ExprResolveEnum(value, &tmp, useModMapValues);
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
HandleInterpVar(VarDef * stmt, struct xkb_desc * xkb, CompatInfo * info)
{
    ExprResult elem, field;
    ExprDef *ndx;
    int ret;

    if (ExprResolveLhs(stmt->name, &elem, &field, &ndx) == 0)
        ret = 0;               /* internal error, already reported */
    else if (elem.str && (uStrCaseCmp(elem.str, "interpret") == 0))
        ret = SetInterpField(&info->dflt, xkb, field.str, ndx, stmt->value,
                              info);
    else if (elem.str && (uStrCaseCmp(elem.str, "indicator") == 0))
        ret = SetIndicatorMapField(&info->ledDflt, xkb, field.str, ndx,
                                  stmt->value);
    else
        ret = SetActionField(xkb, elem.str, field.str, ndx, stmt->value,
                            &info->act);
    free(elem.str);
    free(field.str);
    return ret;
}

static int
HandleInterpBody(VarDef * def, struct xkb_desc * xkb, SymInterpInfo * si,
                 CompatInfo * info)
{
    int ok = 1;
    ExprResult tmp, field;
    ExprDef *arrayNdx;

    for (; def != NULL; def = (VarDef *) def->common.next)
    {
        if ((def->name) && (def->name->type == ExprFieldRef))
        {
            ok = HandleInterpVar(def, xkb, info);
            continue;
        }
        ok = ExprResolveLhs(def->name, &tmp, &field, &arrayNdx);
        if (ok) {
            ok = SetInterpField(si, xkb, field.str, arrayNdx, def->value,
                                info);
            free(field.str);
        }
    }
    return ok;
}

static int
HandleInterpDef(InterpDef * def, struct xkb_desc * xkb, unsigned merge,
                CompatInfo * info)
{
    unsigned pred, mods;
    SymInterpInfo si;

    if (!ResolveStateAndPredicate(def->match, &pred, &mods, info))
    {
        ERROR("Couldn't determine matching modifiers\n");
        ACTION("Symbol interpretation ignored\n");
        return False;
    }
    if (def->merge != MergeDefault)
        merge = def->merge;

    si = info->dflt;
    si.defs.merge = merge;
    if (!LookupKeysym(def->sym, &si.interp.sym))
    {
        ERROR("Could not resolve keysym %s\n", def->sym);
        ACTION("Symbol interpretation ignored\n");
        return False;
    }
    si.interp.match = pred & XkbSI_OpMask;
    si.interp.mods = mods;
    if (!HandleInterpBody(def->def, xkb, &si, info))
    {
        info->errorCount++;
        return False;
    }

    if (!AddInterp(info, &si))
    {
        info->errorCount++;
        return False;
    }
    return True;
}

static int
HandleGroupCompatDef(GroupCompatDef * def,
                     struct xkb_desc * xkb, unsigned merge, CompatInfo * info)
{
    ExprResult val;
    GroupCompatInfo tmp;

    if (def->merge != MergeDefault)
        merge = def->merge;
    if (!XkbIsLegalGroup(def->group - 1))
    {
        ERROR("Keyboard group must be in the range 1..%d\n",
               XkbNumKbdGroups + 1);
        ACTION("Compatibility map for illegal group %d ignored\n",
                def->group);
        return False;
    }
    tmp.fileID = info->fileID;
    tmp.merge = merge;
    if (!ExprResolveVModMask(def->def, &val, xkb))
    {
        ERROR("Expected a modifier mask in group compatibility definition\n");
        ACTION("Ignoring illegal compatibility map for group %d\n",
                def->group);
        return False;
    }
    tmp.real_mods = val.uval & 0xff;
    tmp.vmods = (val.uval >> 8) & 0xffff;
    return AddGroupCompat(info, def->group - 1, &tmp);
}

static void
HandleCompatMapFile(XkbFile * file,
                    struct xkb_desc * xkb, unsigned merge, CompatInfo * info)
{
    ParseCommon *stmt;

    if (merge == MergeDefault)
        merge = MergeAugment;
    free(info->name);
    info->name = _XkbDupString(file->name);
    stmt = file->defs;
    while (stmt)
    {
        switch (stmt->stmtType)
        {
        case StmtInclude:
            if (!HandleIncludeCompatMap((IncludeStmt *) stmt, xkb, info,
                                        HandleCompatMapFile))
                info->errorCount++;
            break;
        case StmtInterpDef:
            if (!HandleInterpDef((InterpDef *) stmt, xkb, merge, info))
                info->errorCount++;
            break;
        case StmtGroupCompatDef:
            if (!HandleGroupCompatDef
                ((GroupCompatDef *) stmt, xkb, merge, info))
                info->errorCount++;
            break;
        case StmtIndicatorMapDef:
        {
            LEDInfo *rtrn;
            rtrn = HandleIndicatorMapDef((IndicatorMapDef *) stmt, xkb,
                                         &info->ledDflt, info->leds, merge);
            if (rtrn != NULL)
                info->leds = rtrn;
            else
                info->errorCount++;
        }
            break;
        case StmtVarDef:
            if (!HandleInterpVar((VarDef *) stmt, xkb, info))
                info->errorCount++;
            break;
        case StmtVModDef:
            if (!HandleVModDef((VModDef *) stmt, xkb, merge, &info->vmods))
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
            struct xkb_compat_map * compat, Bool needSymbol, unsigned pred)
{
    SymInterpInfo *si;

    for (si = info->interps; si; si = (SymInterpInfo *) si->defs.next)
    {
        if (((si->interp.match & XkbSI_OpMask) != pred) ||
            (needSymbol && (si->interp.sym == NoSymbol)) ||
            ((!needSymbol) && (si->interp.sym != NoSymbol)))
            continue;
        if (compat->num_si >= compat->size_si)
        {
            WSGO("No room to merge symbol interpretations\n");
            ACTION("Symbol interpretations lost\n");
            return;
        }
        compat->sym_interpret[compat->num_si++] = si->interp;
    }
}

Bool
CompileCompatMap(XkbFile *file, struct xkb_desc * xkb, unsigned merge,
                 LEDInfoPtr *unboundLEDs)
{
    int i;
    CompatInfo info;
    GroupCompatInfo *gcm;

    InitCompatInfo(&info, xkb);
    info.dflt.defs.merge = merge;
    info.ledDflt.defs.merge = merge;
    HandleCompatMapFile(file, xkb, merge, &info);

    if (info.errorCount == 0)
    {
        int size;
        if (XkbcAllocCompatMap(xkb, XkbAllCompatMask, info.nInterps) !=
            Success)
        {
            WSGO("Couldn't allocate compatibility map\n");
            return False;
        }
        size = info.nInterps * sizeof(struct xkb_sym_interpret);
        if (size > 0)
        {
            CopyInterps(&info, xkb->compat, True, XkbSI_Exactly);
            CopyInterps(&info, xkb->compat, True, XkbSI_AllOf | XkbSI_NoneOf);
            CopyInterps(&info, xkb->compat, True, XkbSI_AnyOf);
            CopyInterps(&info, xkb->compat, True, XkbSI_AnyOfOrNone);
            CopyInterps(&info, xkb->compat, False, XkbSI_Exactly);
            CopyInterps(&info, xkb->compat, False,
                        XkbSI_AllOf | XkbSI_NoneOf);
            CopyInterps(&info, xkb->compat, False, XkbSI_AnyOf);
            CopyInterps(&info, xkb->compat, False, XkbSI_AnyOfOrNone);
        }
        for (i = 0, gcm = &info.groupCompat[0]; i < XkbNumKbdGroups;
             i++, gcm++)
        {
            if ((gcm->fileID != 0) || (gcm->real_mods != 0)
                || (gcm->vmods != 0))
            {
                xkb->compat->groups[i].mask = gcm->real_mods;
                xkb->compat->groups[i].real_mods = gcm->real_mods;
                xkb->compat->groups[i].vmods = gcm->vmods;
            }
        }
        if (info.leds != NULL)
        {
            if (!CopyIndicatorMapDefs(xkb, info.leds, unboundLEDs))
                info.errorCount++;
            info.leds = NULL;
        }
        ClearCompatInfo(&info, xkb);
        return True;
    }
    free(info.interps);
    return False;
}

static uint32_t
VModsToReal(struct xkb_desc *xkb, uint32_t vmodmask)
{
    uint32_t ret = 0;
    int i;

    if (!vmodmask)
        return 0;

    for (i = 0; i < XkbNumVirtualMods; i++) {
        if (!(vmodmask & (1 << i)))
            continue;
        ret |= xkb->server->vmods[i];
    }

    return ret;
}

static void
UpdateActionMods(struct xkb_desc *xkb, union xkb_action *act, uint32_t rmodmask)
{
    switch (act->type) {
    case XkbSA_SetMods:
    case XkbSA_LatchMods:
    case XkbSA_LockMods:
        if (act->mods.flags & XkbSA_UseModMapMods) {
            act->mods.real_mods = rmodmask;
            act->mods.mask = act->mods.real_mods;
        }
        else {
            act->mods.mask = 0;
        }
        act->mods.mask |= VModsToReal(xkb, act->mods.vmods);
        break;
    case XkbSA_ISOLock:
        if (act->iso.flags & XkbSA_UseModMapMods) {
            act->iso.real_mods = rmodmask;
            act->iso.mask = act->iso.real_mods;
        }
        else {
            act->iso.mask = 0;
        }
        act->iso.mask |= VModsToReal(xkb, act->iso.vmods);
        break;
    default:
        break;
    }
}

/**
 * Find an interpretation which applies to this particular level, either by
 * finding an exact match for the symbol and modifier combination, or a
 * generic NoSymbol match.
 */
static struct xkb_sym_interpret *
FindInterpForKey(struct xkb_desc *xkb, xkb_keycode_t key, uint32_t group, uint32_t level)
{
    struct xkb_sym_interpret *ret = NULL;
    xkb_keysym_t *syms;
    int num_syms;
    int i;

    num_syms = xkb_key_get_syms_by_level(xkb, key, group, level, &syms);
    if (num_syms == 0)
        return NULL;

    for (i = 0; i < xkb->compat->num_si; i++) {
        struct xkb_sym_interpret *interp = &xkb->compat->sym_interpret[i];
        uint32_t mods;
        Bool found;

        if ((num_syms != 1 || interp->sym != syms[0]) &&
            interp->sym != NoSymbol)
            continue;

        if (level == 0 || !(interp->match & XkbSI_LevelOneOnly))
            mods = xkb->map->modmap[key];
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
            found = ((interp->mods & mods) == mods);
            break;
        case XkbSI_Exactly:
            found = (interp->mods == mods);
            break;
        default:
            found = False;
            break;
        }

        if (found && interp->sym != NoSymbol)
            return interp;
        else if (found && !ret)
            ret = interp;
    }

    return ret;
}

/**
 */
static Bool
ApplyInterpsToKey(struct xkb_desc *xkb, xkb_keycode_t key)
{
#define INTERP_SIZE (8 * 4)
    struct xkb_sym_interpret *interps[INTERP_SIZE];
    union xkb_action *acts;
    uint32_t vmodmask = 0;
    int num_acts = 0;
    int group, level;
    int width = XkbKeyGroupsWidth(xkb, key);
    int i;

    /* If we've been told not to bind interps to this key, then don't. */
    if (xkb->server->explicit[key] & XkbExplicitInterpretMask)
        return True;

    for (i = 0; i < INTERP_SIZE; i++)
        interps[i] = NULL;

    for (group = 0; group < XkbKeyNumGroups(xkb, key); group++) {
        for (level = 0; level < XkbKeyGroupWidth(xkb, key, group); level++) {
            i = (group * width) + level;
            if (i >= INTERP_SIZE) /* XXX FIXME */
                return False;
            interps[i] = FindInterpForKey(xkb, key, group, level);
            if (interps[i])
                num_acts++;
        }
    }

    if (num_acts)
        num_acts = XkbKeyNumGroups(xkb, key) * width;
    acts = XkbcResizeKeyActions(xkb, key, num_acts);
    if (!num_acts)
        return True;
    else if (!acts)
        return False;

    for (group = 0; group < XkbKeyNumGroups(xkb, key); group++) {
        for (level = 0; level < XkbKeyGroupWidth(xkb, key, group); level++) {
            struct xkb_sym_interpret *interp;

            i = (group * width) + level;
            interp = interps[i];

            /* Infer default key behaviours from the base level. */
            if (group == 0 && level == 0) {
                if (!(xkb->server->explicit[key] & XkbExplicitAutoRepeatMask) &&
                    (!interp || interp->flags & XkbSI_AutoRepeat))
                    xkb->ctrls->per_key_repeat[key / 8] |= (1 << (key % 8));
                if (!(xkb->server->explicit[key] & XkbExplicitBehaviorMask) &&
                    interp && (interp->flags & XkbSI_LockingKey))
                    xkb->server->behaviors[key].type = XkbKB_Lock;
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

    if (!(xkb->server->explicit[key] & XkbExplicitVModMapMask))
        xkb->server->vmodmap[key] = vmodmask;

    return True;
#undef INTERP_SIZE
}

/**
 * This collects a bunch of disparate functions which was done in the server
 * at various points that really should've been done within xkbcomp.  Turns out
 * your actions and types are a lot more useful when any of your modifiers
 * other than Shift actually do something ...
 */
Bool
UpdateModifiersFromCompat(struct xkb_desc *xkb)
{
    xkb_keycode_t key;
    int i;

    /* Find all the interprets for the key and bind them to actions,
     * which will also update the vmodmap. */
    for (key = xkb->min_key_code; key <= xkb->max_key_code; key++)
        if (!ApplyInterpsToKey(xkb, key))
            return False;

    /* Update xkb->server->vmods, the virtual -> real mod mapping. */
    for (i = 0; i < XkbNumVirtualMods; i++)
        xkb->server->vmods[i] = 0;
    for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
        if (!xkb->server->vmodmap[key])
            continue;
        for (i = 0; i < XkbNumVirtualMods; i++) {
            if (!(xkb->server->vmodmap[key] & (1 << i)))
                continue;
            xkb->server->vmods[i] |= xkb->map->modmap[key];
        }
    }

    /* Now update the level masks for all the types to reflect the vmods. */
    for (i = 0; i < xkb->map->num_types; i++) {
        struct xkb_key_type *type = &xkb->map->types[i];
        uint32_t mask = 0;
        int j;
        type->mods.mask = type->mods.real_mods;
        type->mods.mask |= VModsToReal(xkb, type->mods.vmods);
        for (j = 0; j < XkbNumVirtualMods; j++) {
            if (!(type->mods.vmods & (1 << j)))
                continue;
            mask |= xkb->server->vmods[j];
        }
        for (j = 0; j < type->map_count; j++) {
            struct xkb_mods *mods = &type->map[j].mods;
            mods->mask = mods->real_mods | VModsToReal(xkb, mods->vmods);
        }
    }

    /* Update action modifiers. */
    for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
        union xkb_action *acts = XkbKeyActionsPtr(xkb, key);
        for (i = 0; i < XkbKeyNumActions(xkb, key); i++) {
            if (acts[i].any.type == XkbSA_NoAction)
                continue;
            UpdateActionMods(xkb, &acts[i], xkb->map->modmap[key]);
        }
    }

    /* Update group modifiers. */
    for (i = 0; i < XkbNumKbdGroups; i++) {
        struct xkb_mods *group = &xkb->compat->groups[i];
        group->mask = group->real_mods | VModsToReal(xkb, group->vmods);
    }

    /* Update vmod -> indicator maps. */
    for (i = 0; i < XkbNumIndicators; i++) {
        struct xkb_mods *led = &xkb->indicators->maps[i].mods;
        led->mask = led->real_mods | VModsToReal(xkb, led->vmods);
    }

    return True;
}
