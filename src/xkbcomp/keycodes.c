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
#include "expr.h"
#include "parseutils.h"

typedef struct _AliasInfo {
    enum merge_mode merge;
    unsigned file_id;
    struct list entry;

    char alias[XkbKeyNameLength + 1];
    char real[XkbKeyNameLength + 1];
} AliasInfo;

typedef struct _IndicatorNameInfo {
    enum merge_mode merge;
    unsigned file_id;
    struct list entry;

    int ndx;
    xkb_atom_t name;
    bool virtual;
} IndicatorNameInfo;

typedef struct _KeyNamesInfo {
    char *name;     /* e.g. evdev+aliases(qwerty) */
    int errorCount;
    unsigned file_id;
    enum merge_mode merge;
    xkb_keycode_t computedMin; /* lowest keycode stored */
    xkb_keycode_t computedMax; /* highest keycode stored */
    xkb_keycode_t explicitMin;
    xkb_keycode_t explicitMax;
    darray(unsigned long) names;
    darray(unsigned int) files;
    struct list leds;
    struct list aliases;
} KeyNamesInfo;

static void
HandleKeycodesFile(XkbFile *file, struct xkb_keymap *keymap,
                   enum merge_mode merge,
                   KeyNamesInfo *info);

static void
ResizeKeyNameArrays(KeyNamesInfo *info, int newMax)
{
    if (newMax < darray_size(info->names))
        return;

    darray_resize0(info->names, newMax + 1);
    darray_resize0(info->files, newMax + 1);
}

static void
InitAliasInfo(AliasInfo *info, enum merge_mode merge, unsigned file_id,
              char *alias, char *real)
{
    memset(info, 0, sizeof(*info));
    info->merge = merge;
    info->file_id = file_id;
    strncpy(info->alias, alias, XkbKeyNameLength);
    strncpy(info->real, real, XkbKeyNameLength);
}

static void
InitIndicatorNameInfo(IndicatorNameInfo * ii, KeyNamesInfo * info)
{
    ii->merge = info->merge;
    ii->file_id = info->file_id;
    ii->ndx = 0;
    ii->name = XKB_ATOM_NONE;
    ii->virtual = false;
}

static IndicatorNameInfo *
NextIndicatorName(KeyNamesInfo * info)
{
    IndicatorNameInfo *ii;

    ii = malloc(sizeof(*ii));
    if (!ii)
        return NULL;

    InitIndicatorNameInfo(ii, info);
    list_append(&ii->entry, &info->leds);

    return ii;
}

static IndicatorNameInfo *
FindIndicatorByIndex(KeyNamesInfo * info, int ndx)
{
    IndicatorNameInfo *old;

    list_foreach(old, &info->leds, entry)
        if (old->ndx == ndx)
            return old;

    return NULL;
}

static IndicatorNameInfo *
FindIndicatorByName(KeyNamesInfo * info, xkb_atom_t name)
{
    IndicatorNameInfo *old;

    list_foreach(old, &info->leds, entry)
        if (old->name == name)
            return old;

    return NULL;
}

static bool
AddIndicatorName(KeyNamesInfo *info, struct xkb_keymap *keymap,
                 enum merge_mode merge,
                 IndicatorNameInfo *new)
{
    IndicatorNameInfo *old;
    bool replace;

    replace = (merge == MERGE_REPLACE) || (merge == MERGE_OVERRIDE);

    old = FindIndicatorByName(info, new->name);
    if (old) {
        if ((old->file_id == new->file_id && warningLevel > 0) ||
            warningLevel > 9) {
            WARN("Multiple indicators named %s\n",
                 xkb_atom_text(keymap->ctx, new->name));
            if (old->ndx == new->ndx) {
                if (old->virtual != new->virtual) {
                    if (replace)
                        old->virtual = new->virtual;
                    ACTION("Using %s instead of %s\n",
                           (old->virtual ? "virtual" : "real"),
                           (old->virtual ? "real" : "virtual"));
                }
                else {
                    ACTION("Identical definitions ignored\n");
                }
                return true;
            }
            else {
                if (replace)
                    ACTION("Ignoring %d, using %d\n", old->ndx, new->ndx);
                else
                    ACTION("Using %d, ignoring %d\n", old->ndx, new->ndx);
            }

            if (replace) {
                list_del(&old->entry);
                free(old);
            }
        }
    }

    old = FindIndicatorByIndex(info, new->ndx);
    if (old) {
        if ((old->file_id == new->file_id && warningLevel > 0) ||
            warningLevel > 9) {
            WARN("Multiple names for indicator %d\n", new->ndx);
            if ((old->name == new->name) && (old->virtual == new->virtual))
                ACTION("Identical definitions ignored\n");
            else {
                const char *oldType, *newType;
                xkb_atom_t using, ignoring;
                if (old->virtual)
                    oldType = "virtual indicator";
                else
                    oldType = "real indicator";
                if (new->virtual)
                    newType = "virtual indicator";
                else
                    newType = "real indicator";
                if (replace) {
                    using = new->name;
                    ignoring = old->name;
                }
                else {
                    using = old->name;
                    ignoring = new->name;
                }
                ACTION("Using %s %s, ignoring %s %s\n",
                       oldType, xkb_atom_text(keymap->ctx, using),
                       newType, xkb_atom_text(keymap->ctx, ignoring));
            }
        }
        if (replace) {
            old->name = new->name;
            old->virtual = new->virtual;
        }
        return true;
    }
    old = new;
    new = NextIndicatorName(info);
    if (!new) {
        WSGO("Couldn't allocate name for indicator %d\n", old->ndx);
        ACTION("Ignored\n");
        return false;
    }
    new->name = old->name;
    new->ndx = old->ndx;
    new->virtual = old->virtual;
    return true;
}

static void
ClearKeyNamesInfo(KeyNamesInfo * info)
{
    AliasInfo *alias, *next_alias;
    IndicatorNameInfo *ii, *next_ii;

    free(info->name);
    info->name = NULL;
    info->computedMax = info->explicitMax = info->explicitMin = 0;
    info->computedMin = XKB_KEYCODE_MAX;
    darray_free(info->names);
    darray_free(info->files);
    list_foreach_safe(ii, next_ii, &info->leds, entry)
        free(ii);
    list_foreach_safe(alias, next_alias, &info->aliases, entry)
        free(alias);
    list_init(&info->aliases);
}

static void
InitKeyNamesInfo(KeyNamesInfo * info, unsigned file_id)
{
    info->name = NULL;
    list_init(&info->leds);
    list_init(&info->aliases);
    info->file_id = file_id;
    darray_init(info->names);
    darray_init(info->files);
    ClearKeyNamesInfo(info);
    info->errorCount = 0;
}

static int
FindKeyByLong(KeyNamesInfo * info, unsigned long name)
{
    uint64_t i;

    for (i = info->computedMin; i <= info->computedMax; i++)
        if (darray_item(info->names, i) == name)
            return i;

    return 0;
}

/**
 * Store the name of the key as a long in the info struct under the given
 * keycode. If the same keys is referred to twice, print a warning.
 * Note that the key's name is stored as a long, the keycode is the index.
 */
static bool
AddKeyName(KeyNamesInfo * info,
           xkb_keycode_t kc, char *name, enum merge_mode merge,
           unsigned file_id, bool reportCollisions)
{
    xkb_keycode_t old;
    unsigned long lval;

    ResizeKeyNameArrays(info, kc);

    if (kc < info->computedMin)
        info->computedMin = kc;
    if (kc > info->computedMax)
        info->computedMax = kc;
    lval = KeyNameToLong(name);

    if (reportCollisions) {
        reportCollisions = (warningLevel > 7 ||
                            (warningLevel > 0 &&
                             file_id == darray_item(info->files, kc)));
    }

    if (darray_item(info->names, kc) != 0) {
        char buf[6];

        LongToKeyName(darray_item(info->names, kc), buf);
        buf[4] = '\0';
        if (darray_item(info->names, kc) == lval && reportCollisions) {
            WARN("Multiple identical key name definitions\n");
            ACTION("Later occurences of \"<%s> = %d\" ignored\n",
                   buf, kc);
            return true;
        }
        if (merge == MERGE_AUGMENT) {
            if (reportCollisions) {
                WARN("Multiple names for keycode %d\n", kc);
                ACTION("Using <%s>, ignoring <%s>\n", buf, name);
            }
            return true;
        }
        else {
            if (reportCollisions) {
                WARN("Multiple names for keycode %d\n", kc);
                ACTION("Using <%s>, ignoring <%s>\n", name, buf);
            }
            darray_item(info->names, kc) = 0;
            darray_item(info->files, kc) = 0;
        }
    }
    old = FindKeyByLong(info, lval);
    if ((old != 0) && (old != kc)) {
        if (merge == MERGE_OVERRIDE) {
            darray_item(info->names, old) = 0;
            darray_item(info->files, old) = 0;
            if (reportCollisions) {
                WARN("Key name <%s> assigned to multiple keys\n", name);
                ACTION("Using %d, ignoring %d\n", kc, old);
            }
        }
        else {
            if ((reportCollisions) && (warningLevel > 3)) {
                WARN("Key name <%s> assigned to multiple keys\n", name);
                ACTION("Using %d, ignoring %d\n", old, kc);
            }
            return true;
        }
    }
    darray_item(info->names, kc) = lval;
    darray_item(info->files, kc) = file_id;
    return true;
}

/***====================================================================***/

static int
HandleAliasDef(KeyAliasDef *def, enum merge_mode merge, unsigned file_id,
               KeyNamesInfo *info);

static bool
MergeAliases(KeyNamesInfo *into, KeyNamesInfo *from, enum merge_mode merge)
{
    AliasInfo *alias, *next;
    KeyAliasDef def;

    if (list_empty(&from->aliases))
        return true;

    if (list_empty(&into->aliases)) {
        list_replace(&from->aliases, &into->aliases);
        list_init(&from->aliases);
        return true;
    }

    memset(&def, 0, sizeof(def));

    list_foreach_safe(alias, next, &from->aliases, entry) {
        def.merge = (merge == MERGE_DEFAULT) ? alias->merge : merge;
        memcpy(def.alias, alias->alias, XkbKeyNameLength);
        memcpy(def.real, alias->real, XkbKeyNameLength);

        if (!HandleAliasDef(&def, def.merge, alias->file_id, into))
            return false;
    }

    return true;
}

static void
MergeIncludedKeycodes(KeyNamesInfo *into, struct xkb_keymap *keymap,
                      KeyNamesInfo *from, enum merge_mode merge)
{
    uint64_t i;
    char buf[5];
    IndicatorNameInfo *led;

    if (from->errorCount > 0) {
        into->errorCount += from->errorCount;
        return;
    }
    if (into->name == NULL) {
        into->name = from->name;
        from->name = NULL;
    }

    ResizeKeyNameArrays(into, from->computedMax);

    for (i = from->computedMin; i <= from->computedMax; i++) {
        if (darray_item(from->names, i) == 0)
            continue;
        LongToKeyName(darray_item(from->names, i), buf);
        buf[4] = '\0';
        if (!AddKeyName(into, i, buf, merge, from->file_id, false))
            into->errorCount++;
    }

    list_foreach(led, &from->leds, entry) {
        led->merge = (merge == MERGE_DEFAULT ? led->merge : merge);
        if (!AddIndicatorName(into, keymap, led->merge, led))
            into->errorCount++;
    }

    if (!MergeAliases(into, from, merge))
        into->errorCount++;
    if (from->explicitMin != 0) {
        if ((into->explicitMin == 0)
            || (into->explicitMin > from->explicitMin))
            into->explicitMin = from->explicitMin;
    }
    if (from->explicitMax > 0) {
        if ((into->explicitMax == 0)
            || (into->explicitMax < from->explicitMax))
            into->explicitMax = from->explicitMax;
    }
}

/**
 * Handle the given include statement (e.g. "include "evdev+aliases(qwerty)").
 *
 * @param stmt The include statement from the keymap file.
 * @param keymap Unused for all but the keymap->flags.
 * @param info Struct to store the key info in.
 */
static bool
HandleIncludeKeycodes(IncludeStmt *stmt, struct xkb_keymap *keymap,
                      KeyNamesInfo *info)
{
    enum merge_mode newMerge;
    XkbFile *rtrn;
    KeyNamesInfo included;

    /* XXX: What's that? */
    if (stmt->file && strcmp(stmt->file, "computed") == 0) {
        keymap->flags |= AutoKeyNames;
        info->explicitMin = 0;
        info->explicitMax = XKB_KEYCODE_MAX;
        return (info->errorCount == 0);
    }

    if (!ProcessIncludeFile(keymap->ctx, stmt, FILE_TYPE_KEYCODES, &rtrn,
                            &newMerge)) {
        info->errorCount += 10;
        return false;
    }

    InitKeyNamesInfo(&included, rtrn->id);
    HandleKeycodesFile(rtrn, keymap, MERGE_OVERRIDE, &included);
    if (stmt->stmt) {
        free(included.name);
        included.name = stmt->stmt;
        stmt->stmt = NULL;
    }
    FreeXKBFile(rtrn);

    if (stmt->next && included.errorCount < 1) {
        IncludeStmt *next;
        unsigned op;
        KeyNamesInfo next_incl;

        for (next = stmt->next; next != NULL; next = next->next) {
            if (!ProcessIncludeFile(keymap->ctx, next, FILE_TYPE_KEYCODES,
                                    &rtrn, &op)) {
                info->errorCount += 10;
                ClearKeyNamesInfo(&included);
                return false;
            }

            InitKeyNamesInfo(&next_incl, rtrn->id);
            HandleKeycodesFile(rtrn, keymap, MERGE_OVERRIDE, &next_incl);
            MergeIncludedKeycodes(&included, keymap, &next_incl, op);
            ClearKeyNamesInfo(&next_incl);
            FreeXKBFile(rtrn);
        }
    }

    MergeIncludedKeycodes(info, keymap, &included, newMerge);
    ClearKeyNamesInfo(&included);

    return (info->errorCount == 0);
}

/**
 * Parse the given statement and store the output in the info struct.
 * e.g. <ESC> = 9
 */
static int
HandleKeycodeDef(KeycodeDef *stmt, enum merge_mode merge, KeyNamesInfo *info)
{
    if ((info->explicitMin != 0 && stmt->value < info->explicitMin) ||
        (info->explicitMax != 0 && stmt->value > info->explicitMax)) {
        ERROR("Illegal keycode %lu for name <%s>\n", stmt->value, stmt->name);
        ACTION("Must be in the range %d-%d inclusive\n",
               info->explicitMin,
               info->explicitMax ? info->explicitMax : XKB_KEYCODE_MAX);
        return 0;
    }
    if (stmt->merge != MERGE_DEFAULT) {
        if (stmt->merge == MERGE_REPLACE)
            merge = MERGE_OVERRIDE;
        else
            merge = stmt->merge;
    }
    return AddKeyName(info, stmt->value, stmt->name, merge, info->file_id,
                      true);
}

static void
HandleAliasCollision(AliasInfo *old, AliasInfo *new)
{
    if (strncmp(new->real, old->real, XkbKeyNameLength) == 0) {
        if ((new->file_id == old->file_id && warningLevel > 0) ||
            warningLevel > 9) {
            WARN("Alias of %s for %s declared more than once\n",
                  XkbcKeyNameText(new->alias), XkbcKeyNameText(new->real));
            ACTION("First definition ignored\n");
        }
    }
    else {
        char *use, *ignore;

        if (new->merge == MERGE_AUGMENT) {
            use = old->real;
            ignore = new->real;
        }
        else {
            use = new->real;
            ignore = old->real;
        }

        if ((old->file_id == new->file_id && warningLevel > 0) ||
            warningLevel > 9) {
            WARN("Multiple definitions for alias %s\n",
                 XkbcKeyNameText(old->alias));
            ACTION("Using %s, ignoring %s\n",
                   XkbcKeyNameText(use), XkbcKeyNameText(ignore));
        }

        if (use != old->real)
            memcpy(old->real, use, XkbKeyNameLength);
    }

    old->file_id = new->file_id;
    old->merge = new->merge;
}

static int
HandleAliasDef(KeyAliasDef *def, enum merge_mode merge, unsigned file_id,
               KeyNamesInfo *info)
{
    AliasInfo *alias;

    list_foreach(alias, &info->aliases, entry) {
        if (strncmp(alias->alias, def->alias, XkbKeyNameLength) == 0) {
            AliasInfo new;
            InitAliasInfo(&new, merge, file_id, def->alias, def->real);
            HandleAliasCollision(alias, &new);
            return true;
        }
    }

    alias = calloc(1, sizeof(*alias));
    if (!alias) {
        WSGO("Allocation failure in HandleAliasDef\n");
        return false;
    }

    alias->file_id = file_id;
    alias->merge = merge;
    memcpy(alias->alias, def->alias, XkbKeyNameLength);
    memcpy(alias->real, def->real, XkbKeyNameLength);
    list_append(&alias->entry, &info->aliases);

    return true;
}

#define MIN_KEYCODE_DEF 0
#define MAX_KEYCODE_DEF 1

/**
 * Handle the minimum/maximum statement of the xkb file.
 * Sets explicitMin/Max of the info struct.
 *
 * @return 1 on success, 0 otherwise.
 */
static int
HandleKeyNameVar(VarDef *stmt, struct xkb_keymap *keymap, KeyNamesInfo *info)
{
    ExprResult tmp, field;
    ExprDef *arrayNdx;
    int which;

    if (ExprResolveLhs(keymap, stmt->name, &tmp, &field, &arrayNdx) == 0)
        return 0;               /* internal error, already reported */

    if (tmp.str != NULL) {
        ERROR("Unknown element %s encountered\n", tmp.str);
        ACTION("Default for field %s ignored\n", field.str);
        goto err_out;
    }
    if (strcasecmp(field.str, "minimum") == 0)
        which = MIN_KEYCODE_DEF;
    else if (strcasecmp(field.str, "maximum") == 0)
        which = MAX_KEYCODE_DEF;
    else {
        ERROR("Unknown field encountered\n");
        ACTION("Assigment to field %s ignored\n", field.str);
        goto err_out;
    }
    if (arrayNdx != NULL) {
        ERROR("The %s setting is not an array\n", field.str);
        ACTION("Illegal array reference ignored\n");
        goto err_out;
    }

    if (ExprResolveKeyCode(keymap->ctx, stmt->value, &tmp) == 0) {
        ACTION("Assignment to field %s ignored\n", field.str);
        goto err_out;
    }
    if (tmp.uval > XKB_KEYCODE_MAX) {
        ERROR
            ("Illegal keycode %d (must be in the range %d-%d inclusive)\n",
            tmp.uval, 0, XKB_KEYCODE_MAX);
        ACTION("Value of \"%s\" not changed\n", field.str);
        goto err_out;
    }
    if (which == MIN_KEYCODE_DEF) {
        if ((info->explicitMax > 0) && (info->explicitMax < tmp.uval)) {
            ERROR
                ("Minimum key code (%d) must be <= maximum key code (%d)\n",
                tmp.uval, info->explicitMax);
            ACTION("Minimum key code value not changed\n");
            goto err_out;
        }
        if ((info->computedMax > 0) && (info->computedMin < tmp.uval)) {
            ERROR
                ("Minimum key code (%d) must be <= lowest defined key (%d)\n",
                tmp.uval, info->computedMin);
            ACTION("Minimum key code value not changed\n");
            goto err_out;
        }
        info->explicitMin = tmp.uval;
    }
    if (which == MAX_KEYCODE_DEF) {
        if ((info->explicitMin > 0) && (info->explicitMin > tmp.uval)) {
            ERROR("Maximum code (%d) must be >= minimum key code (%d)\n",
                  tmp.uval, info->explicitMin);
            ACTION("Maximum code value not changed\n");
            goto err_out;
        }
        if ((info->computedMax > 0) && (info->computedMax > tmp.uval)) {
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
HandleIndicatorNameDef(IndicatorNameDef *def, struct xkb_keymap *keymap,
                       enum merge_mode merge, KeyNamesInfo *info)
{
    IndicatorNameInfo ii;
    ExprResult tmp;

    if ((def->ndx < 1) || (def->ndx > XkbNumIndicators)) {
        info->errorCount++;
        ERROR("Name specified for illegal indicator index %d\n", def->ndx);
        ACTION("Ignored\n");
        return false;
    }
    InitIndicatorNameInfo(&ii, info);
    ii.ndx = def->ndx;
    if (!ExprResolveString(keymap->ctx, def->name, &tmp)) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", def->ndx);
        info->errorCount++;
        return ReportBadType("indicator", "name", buf, "string");
    }
    ii.name = xkb_atom_intern(keymap->ctx, tmp.str);
    free(tmp.str);
    ii.virtual = def->virtual;
    if (!AddIndicatorName(info, keymap, merge, &ii))
        return false;
    return true;
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
 * @param merge Merge strategy (MERGE_OVERRIDE, etc.)
 * @param info Struct to contain the fully parsed key information.
 */
static void
HandleKeycodesFile(XkbFile *file, struct xkb_keymap *keymap,
                   enum merge_mode merge, KeyNamesInfo *info)
{
    ParseCommon *stmt;

    free(info->name);
    info->name = uDupString(file->name);
    stmt = file->defs;
    while (stmt)
    {
        switch (stmt->stmtType) {
        case StmtInclude:    /* e.g. include "evdev+aliases(qwerty)" */
            if (!HandleIncludeKeycodes((IncludeStmt *) stmt, keymap, info))
                info->errorCount++;
            break;
        case StmtKeycodeDef: /* e.g. <ESC> = 9; */
            if (!HandleKeycodeDef((KeycodeDef *) stmt, merge, info))
                info->errorCount++;
            break;
        case StmtKeyAliasDef: /* e.g. alias <MENU> = <COMP>; */
            if (!HandleAliasDef((KeyAliasDef *) stmt, merge, info->file_id,
                                info))
                info->errorCount++;
            break;
        case StmtVarDef: /* e.g. minimum, maximum */
            if (!HandleKeyNameVar((VarDef *) stmt, keymap, info))
                info->errorCount++;
            break;
        case StmtIndicatorNameDef: /* e.g. indicator 1 = "Caps Lock"; */
            if (!HandleIndicatorNameDef((IndicatorNameDef *) stmt, keymap,
                                        merge, info))
                info->errorCount++;
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
        if (info->errorCount > 10) {
#ifdef NOISY
            ERROR("Too many errors\n");
#endif
            ACTION("Abandoning keycodes file \"%s\"\n", file->topName);
            break;
        }
    }
}

static int
ApplyAliases(struct xkb_keymap *keymap, KeyNamesInfo *info)
{
    int i;
    struct xkb_key *key;
    struct xkb_key_alias *old, *a;
    AliasInfo *alias, *next;
    int nNew = 0, nOld;

    nOld = darray_size(keymap->key_aliases);
    old = &darray_item(keymap->key_aliases, 0);

    list_foreach(alias, &info->aliases, entry) {
        unsigned long lname;

        lname = KeyNameToLong(alias->real);
        key = FindNamedKey(keymap, lname, false, CreateKeyNames(keymap), 0);
        if (!key) {
            if (warningLevel > 4) {
                WARN("Attempt to alias %s to non-existent key %s\n",
                     XkbcKeyNameText(alias->alias),
                     XkbcKeyNameText(alias->real));
                ACTION("Ignored\n");
            }
            alias->alias[0] = '\0';
            continue;
        }

        lname = KeyNameToLong(alias->alias);
        key = FindNamedKey(keymap, lname, false, false, 0);
        if (key) {
            if (warningLevel > 4) {
                WARN("Attempt to create alias with the name of a real key\n");
                ACTION("Alias \"%s = %s\" ignored\n",
                       XkbcKeyNameText(alias->alias),
                       XkbcKeyNameText(alias->real));
            }
            alias->alias[0] = '\0';
            continue;
        }

        nNew++;

        if (!old)
            continue;

        for (i = 0, a = old; i < nOld; i++, a++) {
            AliasInfo old_alias;

            if (strncmp(a->alias, alias->alias, XkbKeyNameLength) != 0)
                continue;

            InitAliasInfo(&old_alias, MERGE_AUGMENT, 0, a->alias, a->real);
            HandleAliasCollision(&old_alias, alias);
            memcpy(old_alias.real, a->real, XkbKeyNameLength);
            alias->alias[0] = '\0';
            nNew--;
            break;
        }
    }

    if (nNew == 0)
        goto out;

    darray_resize0(keymap->key_aliases, nOld + nNew);

    a = &darray_item(keymap->key_aliases, nOld);
    list_foreach(alias, &info->aliases, entry) {
        if (alias->alias[0] != '\0') {
            strncpy(a->alias, alias->alias, XkbKeyNameLength);
            strncpy(a->real, alias->real, XkbKeyNameLength);
            a++;
        }
    }

out:
    list_foreach_safe(alias, next, &info->aliases, entry)
        free(alias);
    list_init(&info->aliases);
    return true;
}

/**
 * Compile the xkb_keycodes section, parse it's output, return the results.
 *
 * @param file The parsed XKB file (may have include statements requiring
 * further parsing)
 * @param result The effective keycodes, as gathered from the file.
 * @param merge Merge strategy.
 *
 * @return true on success, false otherwise.
 */
bool
CompileKeycodes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge)
{
    xkb_keycode_t kc;
    KeyNamesInfo info; /* contains all the info after parsing */
    IndicatorNameInfo *ii;

    InitKeyNamesInfo(&info, file->id);

    HandleKeycodesFile(file, keymap, merge, &info);

    /* all the keys are now stored in info */

    if (info.errorCount != 0)
        goto err_info;

    if (info.explicitMin > 0) /* if "minimum" statement was present */
        keymap->min_key_code = info.explicitMin;
    else
        keymap->min_key_code = info.computedMin;

    if (info.explicitMax > 0) /* if "maximum" statement was present */
        keymap->max_key_code = info.explicitMax;
    else
        keymap->max_key_code = info.computedMax;

    darray_resize0(keymap->keys, keymap->max_key_code + 1);
    for (kc = info.computedMin; kc <= info.computedMax; kc++)
        LongToKeyName(darray_item(info.names, kc),
                      XkbKey(keymap, kc)->name);

    if (info.name)
        keymap->keycodes_section_name = strdup(info.name);

    list_foreach(ii, &info.leds, entry) {
        free(keymap->indicator_names[ii->ndx - 1]);
        keymap->indicator_names[ii->ndx - 1] =
            xkb_atom_strdup(keymap->ctx, ii->name);
    }

    ApplyAliases(keymap, &info);

    ClearKeyNamesInfo(&info);
    return true;

err_info:
    ClearKeyNamesInfo(&info);
    return false;
}
