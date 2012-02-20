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
#include "tokens.h"
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
    uint32_t vmods;
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

static char *
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
    register int i;

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
    for (i = 0; i < sizeof info->dflt.interp.act.data; i++)
    {
        info->dflt.interp.act.data[i] = 0;
    }
    ClearIndicatorMapInfo(&info->ledDflt);
    info->ledDflt.defs.fileID = info->fileID;
    info->ledDflt.defs.defined = 0;
    info->ledDflt.defs.merge = MergeOverride;
    bzero((char *) &info->groupCompat[0],
          XkbNumKbdGroups * sizeof(GroupCompatInfo));
    info->leds = NULL;
    InitVModInfo(&info->vmods, xkb);
    return;
}

static void
ClearCompatInfo(CompatInfo * info, struct xkb_desc * xkb)
{
    register int i;

    if (info->name != NULL)
        free(info->name);
    info->name = NULL;
    info->dflt.defs.defined = 0;
    info->dflt.defs.merge = MergeAugment;
    info->dflt.interp.flags = 0;
    info->dflt.interp.virtual_mod = XkbNoModifier;
    info->dflt.interp.act.type = XkbSA_NoAction;
    for (i = 0; i < sizeof info->dflt.interp.act.data; i++)
    {
        info->dflt.interp.act.data[i] = 0;
    }
    ClearIndicatorMapInfo(&info->ledDflt);
    info->nInterps = 0;
    info->interps = (SymInterpInfo *) ClearCommonInfo(&info->interps->defs);
    bzero((char *) &info->groupCompat[0],
          XkbNumKbdGroups * sizeof(GroupCompatInfo));
    info->leds = (LEDInfo *) ClearCommonInfo(&info->leds->defs);
    /* 3/30/94 (ef) -- XXX! Should free action info here */
    ClearVModInfo(&info->vmods, xkb);
    return;
}

static SymInterpInfo *
NextInterp(CompatInfo * info)
{
    SymInterpInfo *si;

    si = uTypedAlloc(SymInterpInfo);
    if (si)
    {
        bzero((char *) si, sizeof(SymInterpInfo));
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
    register int i;

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
    return;
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
        bzero(info, sizeof(CompatInfo));
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
            if (included.name != NULL)
                free(included.name);
            included.name = stmt->stmt;
            stmt->stmt = NULL;
        }
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
                ClearCompatInfo(&next_incl, xkb);
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

static LookupEntry useModMapValues[] = {
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
        ok = HandleActionDef(value, xkb, &si->interp.act, si->defs.merge,
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
        WARN("Could not resolve keysym %s\n", def->sym);
        info->errorCount++;
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
    return;
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
    return;
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
        if (info.name != NULL)
        {
            if (XkbcAllocNames(xkb, XkbCompatNameMask, 0, 0) == Success)
                xkb->names->compat =
                    xkb_intern_atom(info.name);
	    else
            {
                WSGO("Couldn't allocate space for compat name\n");
                ACTION("Name \"%s\" (from %s) NOT assigned\n",
                        scanFile, info.name);
            }
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
    if (info.interps != NULL)
        free(info.interps);
    return False;
}
