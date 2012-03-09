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

#include "xkbcomp.h"
#include "xkballoc.h"
#include "xkbmisc.h"
#include "expr.h"
#include "keycodes.h"
#include "misc.h"
#include "alias.h"
#include "parseutils.h"

const char *
longText(unsigned long val)
{
    char buf[4];

    LongToKeyName(val, buf);
    return XkbcKeyNameText(buf);
}

/***====================================================================***/

void
LongToKeyName(unsigned long val, char *name)
{
    name[0] = ((val >> 24) & 0xff);
    name[1] = ((val >> 16) & 0xff);
    name[2] = ((val >> 8) & 0xff);
    name[3] = (val & 0xff);
}

/***====================================================================***/

typedef struct _IndicatorNameInfo
{
    CommonInfo defs;
    int ndx;
    uint32_t name;
    Bool virtual;
} IndicatorNameInfo;

typedef struct _KeyNamesInfo
{
    char *name;     /* e.g. evdev+aliases(qwerty) */
    int errorCount;
    unsigned fileID;
    unsigned merge;
    xkb_keycode_t computedMin; /* lowest keycode stored */
    xkb_keycode_t computedMax; /* highest keycode stored */
    xkb_keycode_t explicitMin;
    xkb_keycode_t explicitMax;
    xkb_keycode_t arraySize;
    unsigned long *names;
    unsigned *files;
    unsigned char *has_alt_forms;
    IndicatorNameInfo *leds;
    AliasInfo *aliases;
} KeyNamesInfo;

static void HandleKeycodesFile(XkbFile * file,
                               struct xkb_desc * xkb,
                               unsigned merge,
                               KeyNamesInfo * info);

static int
ResizeKeyNameArrays(KeyNamesInfo *info, int newMax)
{
    void *tmp;
    int i;

    tmp = _XkbTypedRealloc(info->names, newMax + 1, unsigned long);
    if (!tmp) {
        ERROR
            ("Couldn't reallocate for larger maximum keycode (%d)\n",
             newMax);
        ACTION("Maximum key value not changed\n");
        return 0;
    }
    info->names = tmp;
    for (i = info->arraySize + 1; i <= newMax; i++)
        info->names[i] = 0;

    tmp = _XkbTypedRealloc(info->files, newMax + 1, unsigned);
    if (!tmp) {
        ERROR
            ("Couldn't reallocate for larger maximum keycode (%d)\n",
             newMax);
        ACTION("Maximum key value not changed\n");
        return 0;
    }
    info->files = tmp;
    for (i = info->arraySize + 1; i <= newMax; i++)
        info->files[i] = 0;

    tmp = _XkbTypedRealloc(info->has_alt_forms, newMax + 1, unsigned char);
    if (!tmp) {
        ERROR
            ("Couldn't reallocate for larger maximum keycode (%d)\n",
             newMax);
        ACTION("Maximum key value not changed\n");
        return 0;
    }
    info->has_alt_forms = tmp;
    for (i = info->arraySize + 1; i <= newMax; i++)
        info->has_alt_forms[i] = 0;

    info->arraySize = newMax;

    return 1;
}

static void
InitIndicatorNameInfo(IndicatorNameInfo * ii, KeyNamesInfo * info)
{
    ii->defs.defined = 0;
    ii->defs.merge = info->merge;
    ii->defs.fileID = info->fileID;
    ii->defs.next = NULL;
    ii->ndx = 0;
    ii->name = None;
    ii->virtual = False;
}

static void
ClearIndicatorNameInfo(IndicatorNameInfo * ii, KeyNamesInfo * info)
{
    if (ii == info->leds)
    {
        ClearCommonInfo(&ii->defs);
        info->leds = NULL;
    }
}

static IndicatorNameInfo *
NextIndicatorName(KeyNamesInfo * info)
{
    IndicatorNameInfo *ii;

    ii = uTypedAlloc(IndicatorNameInfo);
    if (ii)
    {
        InitIndicatorNameInfo(ii, info);
        info->leds = (IndicatorNameInfo *) AddCommonInfo(&info->leds->defs,
                                                         (CommonInfo *) ii);
    }
    return ii;
}

static IndicatorNameInfo *
FindIndicatorByIndex(KeyNamesInfo * info, int ndx)
{
    IndicatorNameInfo *old;

    for (old = info->leds; old != NULL;
         old = (IndicatorNameInfo *) old->defs.next)
    {
        if (old->ndx == ndx)
            return old;
    }
    return NULL;
}

static IndicatorNameInfo *
FindIndicatorByName(KeyNamesInfo * info, uint32_t name)
{
    IndicatorNameInfo *old;

    for (old = info->leds; old != NULL;
         old = (IndicatorNameInfo *) old->defs.next)
    {
        if (old->name == name)
            return old;
    }
    return NULL;
}

static Bool
AddIndicatorName(KeyNamesInfo * info, IndicatorNameInfo * new)
{
    IndicatorNameInfo *old;
    Bool replace;

    replace = (new->defs.merge == MergeReplace) ||
        (new->defs.merge == MergeOverride);
    old = FindIndicatorByName(info, new->name);
    if (old)
    {
        if (((old->defs.fileID == new->defs.fileID) && (warningLevel > 0))
            || (warningLevel > 9))
        {
            WARN("Multiple indicators named %s\n", XkbcAtomText(new->name));
            if (old->ndx == new->ndx)
            {
                if (old->virtual != new->virtual)
                {
                    if (replace)
                        old->virtual = new->virtual;
                    ACTION("Using %s instead of %s\n",
                           (old->virtual ? "virtual" : "real"),
                           (old->virtual ? "real" : "virtual"));
                }
                else
                {
                    ACTION("Identical definitions ignored\n");
                }
                return True;
            }
            else
            {
                if (replace)
                    ACTION("Ignoring %d, using %d\n", old->ndx, new->ndx);
                else
                    ACTION("Using %d, ignoring %d\n", old->ndx, new->ndx);
            }
            if (replace)
            {
                if (info->leds == old)
                    info->leds = (IndicatorNameInfo *) old->defs.next;
                else
                {
                    IndicatorNameInfo *tmp;
                    tmp = info->leds;
                    for (; tmp != NULL;
                         tmp = (IndicatorNameInfo *) tmp->defs.next)
                    {
                        if (tmp->defs.next == (CommonInfo *) old)
                        {
                            tmp->defs.next = old->defs.next;
                            break;
                        }
                    }
                }
                free(old);
            }
        }
    }
    old = FindIndicatorByIndex(info, new->ndx);
    if (old)
    {
        if (((old->defs.fileID == new->defs.fileID) && (warningLevel > 0))
            || (warningLevel > 9))
        {
            WARN("Multiple names for indicator %d\n", new->ndx);
            if ((old->name == new->name) && (old->virtual == new->virtual))
                ACTION("Identical definitions ignored\n");
            else
            {
                const char *oldType, *newType;
                uint32_t using, ignoring;
                if (old->virtual)
                    oldType = "virtual indicator";
                else
                    oldType = "real indicator";
                if (new->virtual)
                    newType = "virtual indicator";
                else
                    newType = "real indicator";
                if (replace)
                {
                    using = new->name;
                    ignoring = old->name;
                }
                else
                {
                    using = old->name;
                    ignoring = new->name;
                }
                ACTION("Using %s %s, ignoring %s %s\n",
                        oldType, XkbcAtomText(using),
                        newType, XkbcAtomText(ignoring));
            }
        }
        if (replace)
        {
            old->name = new->name;
            old->virtual = new->virtual;
        }
        return True;
    }
    old = new;
    new = NextIndicatorName(info);
    if (!new)
    {
        WSGO("Couldn't allocate name for indicator %d\n", old->ndx);
        ACTION("Ignored\n");
        return False;
    }
    new->name = old->name;
    new->ndx = old->ndx;
    new->virtual = old->virtual;
    return True;
}

static void
ClearKeyNamesInfo(KeyNamesInfo * info)
{
    free(info->name);
    info->name = NULL;
    info->computedMax = info->explicitMax = info->explicitMin = 0;
    info->computedMin = XKB_KEYCODE_MAX;
    info->arraySize = 0;
    free(info->names);
    info->names = NULL;
    free(info->files);
    info->files = NULL;
    free(info->has_alt_forms);
    info->has_alt_forms = NULL;
    if (info->leds)
        ClearIndicatorNameInfo(info->leds, info);
    if (info->aliases)
        ClearAliases(&info->aliases);
}

static void
InitKeyNamesInfo(KeyNamesInfo * info)
{
    info->name = NULL;
    info->leds = NULL;
    info->aliases = NULL;
    info->names = NULL;
    info->files = NULL;
    info->has_alt_forms = NULL;
    ClearKeyNamesInfo(info);
    info->errorCount = 0;
}

static int
FindKeyByLong(KeyNamesInfo * info, unsigned long name)
{
    int i;

    for (i = info->computedMin; i <= info->computedMax; i++)
    {
        if (info->names[i] == name)
            return i;
    }
    return 0;
}

/**
 * Store the name of the key as a long in the info struct under the given
 * keycode. If the same keys is referred to twice, print a warning.
 * Note that the key's name is stored as a long, the keycode is the index.
 */
static Bool
AddKeyName(KeyNamesInfo * info,
           xkb_keycode_t kc,
           char *name, unsigned merge, unsigned fileID, Bool reportCollisions)
{
    int old;
    unsigned long lval;

    if (kc > info->arraySize && !ResizeKeyNameArrays(info, kc)) {
        ERROR("Couldn't resize KeyNames arrays for keycode %d\n", kc);
        ACTION("Ignoring key %d\n", kc);
        return False;
    }
    if (kc < info->computedMin)
        info->computedMin = kc;
    if (kc > info->computedMax)
        info->computedMax = kc;
    lval = KeyNameToLong(name);

    if (reportCollisions)
    {
        reportCollisions = ((warningLevel > 7) ||
                            ((warningLevel > 0)
                             && (fileID == info->files[kc])));
    }

    if (info->names[kc] != 0)
    {
        char buf[6];

        LongToKeyName(info->names[kc], buf);
        buf[4] = '\0';
        if (info->names[kc] == lval)
        {
            if (info->has_alt_forms[kc] || (merge == MergeAltForm))
            {
                info->has_alt_forms[kc] = True;
            }
            else if (reportCollisions)
            {
                WARN("Multiple identical key name definitions\n");
                ACTION("Later occurences of \"<%s> = %d\" ignored\n",
                        buf, kc);
            }
            return True;
        }
        if (merge == MergeAugment)
        {
            if (reportCollisions)
            {
                WARN("Multiple names for keycode %d\n", kc);
                ACTION("Using <%s>, ignoring <%s>\n", buf, name);
            }
            return True;
        }
        else
        {
            if (reportCollisions)
            {
                WARN("Multiple names for keycode %d\n", kc);
                ACTION("Using <%s>, ignoring <%s>\n", name, buf);
            }
            info->names[kc] = 0;
            info->files[kc] = 0;
        }
    }
    old = FindKeyByLong(info, lval);
    if ((old != 0) && (old != kc))
    {
        if (merge == MergeOverride)
        {
            info->names[old] = 0;
            info->files[old] = 0;
            info->has_alt_forms[old] = True;
            if (reportCollisions)
            {
                WARN("Key name <%s> assigned to multiple keys\n", name);
                ACTION("Using %d, ignoring %d\n", kc, old);
            }
        }
        else if (merge != MergeAltForm)
        {
            if ((reportCollisions) && (warningLevel > 3))
            {
                WARN("Key name <%s> assigned to multiple keys\n", name);
                ACTION("Using %d, ignoring %d\n", old, kc);
                ACTION
                    ("Use 'alternate' keyword to assign the same name to multiple keys\n");
            }
            return True;
        }
        else
        {
            info->has_alt_forms[old] = True;
        }
    }
    info->names[kc] = lval;
    info->files[kc] = fileID;
    info->has_alt_forms[kc] = (merge == MergeAltForm);
    return True;
}

/***====================================================================***/

static void
MergeIncludedKeycodes(KeyNamesInfo * into, KeyNamesInfo * from,
                      unsigned merge)
{
    int i;
    char buf[5];

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
    if (from->computedMax > into->arraySize &&
        !ResizeKeyNameArrays(into, from->computedMax)) {
        ERROR("Couldn't resize KeyNames arrays for key %d\n",
              from->computedMax);
        ACTION("Ignoring include file %s\n", from->name);
        into->errorCount += 10;
        return;
    }
    for (i = from->computedMin; i <= from->computedMax; i++)
    {
        unsigned thisMerge;
        if (from->names[i] == 0)
            continue;
        LongToKeyName(from->names[i], buf);
        buf[4] = '\0';
        if (from->has_alt_forms[i])
            thisMerge = MergeAltForm;
        else
            thisMerge = merge;
        if (!AddKeyName(into, i, buf, thisMerge, from->fileID, False))
            into->errorCount++;
    }
    if (from->leds)
    {
        IndicatorNameInfo *led, *next;
        for (led = from->leds; led != NULL; led = next)
        {
            if (merge != MergeDefault)
                led->defs.merge = merge;
            if (!AddIndicatorName(into, led))
                into->errorCount++;
            next = (IndicatorNameInfo *) led->defs.next;
        }
    }
    if (!MergeAliases(&into->aliases, &from->aliases, merge))
        into->errorCount++;
    if (from->explicitMin != 0)
    {
        if ((into->explicitMin == 0)
            || (into->explicitMin > from->explicitMin))
            into->explicitMin = from->explicitMin;
    }
    if (from->explicitMax > 0)
    {
        if ((into->explicitMax == 0)
            || (into->explicitMax < from->explicitMax))
            into->explicitMax = from->explicitMax;
    }
}

/**
 * Handle the given include statement (e.g. "include "evdev+aliases(qwerty)").
 *
 * @param stmt The include statement from the keymap file.
 * @param xkb Unused for all but the xkb->flags.
 * @param info Struct to store the key info in.
 */
static Bool
HandleIncludeKeycodes(IncludeStmt * stmt, struct xkb_desc * xkb, KeyNamesInfo * info)
{
    unsigned newMerge;
    XkbFile *rtrn;
    KeyNamesInfo included = {NULL};
    Bool haveSelf;

    haveSelf = False;
    if ((stmt->file == NULL) && (stmt->map == NULL))
    {
        haveSelf = True;
        included = *info;
        memset(info, 0, sizeof(KeyNamesInfo));
    }
    else if (stmt->file && strcmp(stmt->file, "computed") == 0)
    {
        xkb->flags |= AutoKeyNames;
        info->explicitMin = 0;
        info->explicitMax = XKB_KEYCODE_MAX;
        return (info->errorCount == 0);
    } /* parse file, store returned info in the xkb struct */
    else if (ProcessIncludeFile(stmt, XkmKeyNamesIndex, &rtrn, &newMerge))
    {
        InitKeyNamesInfo(&included);
        HandleKeycodesFile(rtrn, xkb, MergeOverride, &included);
        if (stmt->stmt != NULL)
        {
            free(included.name);
            included.name = stmt->stmt;
            stmt->stmt = NULL;
        }
        FreeXKBFile(rtrn);
    }
    else
    {
        info->errorCount += 10; /* XXX: why 10?? */
        return False;
    }
    /* Do we have more than one include statement? */
    if ((stmt->next != NULL) && (included.errorCount < 1))
    {
        IncludeStmt *next;
        unsigned op;
        KeyNamesInfo next_incl;

        for (next = stmt->next; next != NULL; next = next->next)
        {
            if ((next->file == NULL) && (next->map == NULL))
            {
                haveSelf = True;
                MergeIncludedKeycodes(&included, info, next->merge);
                ClearKeyNamesInfo(info);
            }
            else if (ProcessIncludeFile(next, XkmKeyNamesIndex, &rtrn, &op))
            {
                InitKeyNamesInfo(&next_incl);
                HandleKeycodesFile(rtrn, xkb, MergeOverride, &next_incl);
                MergeIncludedKeycodes(&included, &next_incl, op);
                ClearKeyNamesInfo(&next_incl);
                FreeXKBFile(rtrn);
            }
            else
            {
                info->errorCount += 10; /* XXX: Why 10?? */
                ClearKeyNamesInfo(&included);
                return False;
            }
        }
    }
    if (haveSelf)
        *info = included;
    else
    {
        MergeIncludedKeycodes(info, &included, newMerge);
        ClearKeyNamesInfo(&included);
    }
    return (info->errorCount == 0);
}

/**
 * Parse the given statement and store the output in the info struct.
 * e.g. <ESC> = 9
 */
static int
HandleKeycodeDef(KeycodeDef * stmt, unsigned merge, KeyNamesInfo * info)
{
    if ((info->explicitMin != 0 && stmt->value < info->explicitMin) ||
        (info->explicitMax != 0 && stmt->value > info->explicitMax))
    {
        ERROR("Illegal keycode %lu for name <%s>\n", stmt->value, stmt->name);
        ACTION("Must be in the range %d-%d inclusive\n",
                info->explicitMin,
                info->explicitMax ? info->explicitMax : XKB_KEYCODE_MAX);
        return 0;
    }
    if (stmt->merge != MergeDefault)
    {
        if (stmt->merge == MergeReplace)
            merge = MergeOverride;
        else
            merge = stmt->merge;
    }
    return AddKeyName(info, stmt->value, stmt->name, merge, info->fileID,
                      True);
}

#define	MIN_KEYCODE_DEF		0
#define	MAX_KEYCODE_DEF		1

/**
 * Handle the minimum/maximum statement of the xkb file.
 * Sets explicitMin/Max of the info struct.
 *
 * @return 1 on success, 0 otherwise.
 */
static int
HandleKeyNameVar(VarDef * stmt, KeyNamesInfo * info)
{
    ExprResult tmp, field;
    ExprDef *arrayNdx;
    int which;

    if (ExprResolveLhs(stmt->name, &tmp, &field, &arrayNdx) == 0)
        return 0;               /* internal error, already reported */

    if (tmp.str != NULL)
    {
        ERROR("Unknown element %s encountered\n", tmp.str);
        ACTION("Default for field %s ignored\n", field.str);
        goto err_out;
    }
    if (uStrCaseCmp(field.str, "minimum") == 0)
        which = MIN_KEYCODE_DEF;
    else if (uStrCaseCmp(field.str, "maximum") == 0)
        which = MAX_KEYCODE_DEF;
    else
    {
        ERROR("Unknown field encountered\n");
        ACTION("Assigment to field %s ignored\n", field.str);
        goto err_out;
    }
    if (arrayNdx != NULL)
    {
        ERROR("The %s setting is not an array\n", field.str);
        ACTION("Illegal array reference ignored\n");
        goto err_out;
    }

    if (ExprResolveKeyCode(stmt->value, &tmp) == 0)
    {
        ACTION("Assignment to field %s ignored\n", field.str);
        goto err_out;
    }
    if (tmp.uval > XKB_KEYCODE_MAX)
    {
        ERROR
            ("Illegal keycode %d (must be in the range %d-%d inclusive)\n",
             tmp.uval, 0, XKB_KEYCODE_MAX);
        ACTION("Value of \"%s\" not changed\n", field.str);
        goto err_out;
    }
    if (which == MIN_KEYCODE_DEF)
    {
        if ((info->explicitMax > 0) && (info->explicitMax < tmp.uval))
        {
            ERROR
                ("Minimum key code (%d) must be <= maximum key code (%d)\n",
                 tmp.uval, info->explicitMax);
            ACTION("Minimum key code value not changed\n");
            goto err_out;
        }
        if ((info->computedMax > 0) && (info->computedMin < tmp.uval))
        {
            ERROR
                ("Minimum key code (%d) must be <= lowest defined key (%d)\n",
                 tmp.uval, info->computedMin);
            ACTION("Minimum key code value not changed\n");
            goto err_out;
        }
        info->explicitMin = tmp.uval;
    }
    if (which == MAX_KEYCODE_DEF)
    {
        if ((info->explicitMin > 0) && (info->explicitMin > tmp.uval))
        {
            ERROR("Maximum code (%d) must be >= minimum key code (%d)\n",
                   tmp.uval, info->explicitMin);
            ACTION("Maximum code value not changed\n");
            goto err_out;
        }
        if ((info->computedMax > 0) && (info->computedMax > tmp.uval))
        {
            ERROR
                ("Maximum code (%d) must be >= highest defined key (%d)\n",
                 tmp.uval, info->computedMax);
            ACTION("Maximum code value not changed\n");
            goto err_out;
        }
        info->explicitMax = tmp.uval;
    }

    free(field.str);
    return 1;

err_out:
    free(field.str);
    return 0;
}

static int
HandleIndicatorNameDef(IndicatorNameDef * def,
                       unsigned merge, KeyNamesInfo * info)
{
    IndicatorNameInfo ii;
    ExprResult tmp;

    if ((def->ndx < 1) || (def->ndx > XkbNumIndicators))
    {
        info->errorCount++;
        ERROR("Name specified for illegal indicator index %d\n", def->ndx);
        ACTION("Ignored\n");
        return False;
    }
    InitIndicatorNameInfo(&ii, info);
    ii.ndx = def->ndx;
    if (!ExprResolveString(def->name, &tmp))
    {
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", def->ndx);
        info->errorCount++;
        return ReportBadType("indicator", "name", buf, "string");
    }
    ii.name = xkb_intern_atom(tmp.str);
    free(tmp.str);
    ii.virtual = def->virtual;
    if (!AddIndicatorName(info, &ii))
        return False;
    return True;
}

/**
 * Handle the xkb_keycodes section of a xkb file.
 * All information about parsed keys is stored in the info struct.
 *
 * Such a section may have include statements, in which case this function is
 * semi-recursive (it calls HandleIncludeKeycodes, which may call
 * HandleKeycodesFile again).
 *
 * @param file The input file (parsed xkb_keycodes section)
 * @param xkb Necessary to pass down, may have flags changed.
 * @param merge Merge strategy (MergeOverride, etc.)
 * @param info Struct to contain the fully parsed key information.
 */
static void
HandleKeycodesFile(XkbFile * file,
                   struct xkb_desc * xkb, unsigned merge, KeyNamesInfo * info)
{
    ParseCommon *stmt;

    free(info->name);
    info->name = _XkbDupString(file->name);
    stmt = file->defs;
    while (stmt)
    {
        switch (stmt->stmtType)
        {
        case StmtInclude:    /* e.g. include "evdev+aliases(qwerty)" */
            if (!HandleIncludeKeycodes((IncludeStmt *) stmt, xkb, info))
                info->errorCount++;
            break;
        case StmtKeycodeDef: /* e.g. <ESC> = 9; */
            if (!HandleKeycodeDef((KeycodeDef *) stmt, merge, info))
                info->errorCount++;
            break;
        case StmtKeyAliasDef: /* e.g. alias <MENU> = <COMP>; */
            if (!HandleAliasDef((KeyAliasDef *) stmt,
                                merge, info->fileID, &info->aliases))
                info->errorCount++;
            break;
        case StmtVarDef: /* e.g. minimum, maximum */
            if (!HandleKeyNameVar((VarDef *) stmt, info))
                info->errorCount++;
            break;
        case StmtIndicatorNameDef: /* e.g. indicator 1 = "Caps Lock"; */
            if (!HandleIndicatorNameDef((IndicatorNameDef *) stmt,
                                        merge, info))
            {
                info->errorCount++;
            }
            break;
        case StmtInterpDef:
        case StmtVModDef:
            ERROR("Keycode files may define key and indicator names only\n");
            ACTION("Ignoring definition of %s\n",
                    ((stmt->stmtType ==
                      StmtInterpDef) ? "a symbol interpretation" :
                     "virtual modifiers"));
            info->errorCount++;
            break;
        default:
            WSGO("Unexpected statement type %d in HandleKeycodesFile\n",
                  stmt->stmtType);
            break;
        }
        stmt = stmt->next;
        if (info->errorCount > 10)
        {
#ifdef NOISY
            ERROR("Too many errors\n");
#endif
            ACTION("Abandoning keycodes file \"%s\"\n", file->topName);
            break;
        }
    }
}

/**
 * Compile the xkb_keycodes section, parse it's output, return the results.
 *
 * @param file The parsed XKB file (may have include statements requiring
 * further parsing)
 * @param result The effective keycodes, as gathered from the file.
 * @param merge Merge strategy.
 *
 * @return True on success, False otherwise.
 */
Bool
CompileKeycodes(XkbFile *file, struct xkb_desc * xkb, unsigned merge)
{
    KeyNamesInfo info; /* contains all the info after parsing */

    InitKeyNamesInfo(&info);
    HandleKeycodesFile(file, xkb, merge, &info);

    /* all the keys are now stored in info */

    if (info.errorCount == 0)
    {
        if (info.explicitMin > 0) /* if "minimum" statement was present */
            xkb->min_key_code = info.explicitMin;
        else
            xkb->min_key_code = info.computedMin;
        if (info.explicitMax > 0) /* if "maximum" statement was present */
            xkb->max_key_code = info.explicitMax;
        else
            xkb->max_key_code = info.computedMax;
        if (XkbcAllocNames(xkb, XkbKeyNamesMask | XkbIndicatorNamesMask, 0)
                == Success)
        {
            int i;
            for (i = info.computedMin; i <= info.computedMax; i++)
                LongToKeyName(info.names[i], xkb->names->keys[i].name);
        }
        else
        {
            WSGO("Cannot create struct xkb_names in CompileKeycodes\n");
            return False;
        }
        if (info.leds)
        {
            IndicatorNameInfo *ii;
            if (XkbcAllocIndicatorMaps(xkb) != Success)
            {
                WSGO("Couldn't allocate IndicatorRec in CompileKeycodes\n");
                ACTION("Physical indicators not set\n");
            }
            for (ii = info.leds; ii != NULL;
                 ii = (IndicatorNameInfo *) ii->defs.next)
            {
                xkb->names->indicators[ii->ndx - 1] = ii->name;
                if (xkb->indicators != NULL)
                {
                    unsigned bit;
                    bit = 1 << (ii->ndx - 1);
                    if (ii->virtual)
                        xkb->indicators->phys_indicators &= ~bit;
                    else
                        xkb->indicators->phys_indicators |= bit;
                }
            }
        }
        if (info.aliases)
            ApplyAliases(xkb, &info.aliases);
        ClearKeyNamesInfo(&info);
        return True;
    }
    ClearKeyNamesInfo(&info);
    return False;
}
