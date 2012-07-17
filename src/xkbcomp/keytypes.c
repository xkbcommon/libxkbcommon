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
#include "vmod.h"

typedef struct _PreserveInfo {
    CommonInfo defs;
    short matchingMapIndex;
    unsigned char indexMods;
    unsigned char preMods;
    unsigned short indexVMods;
    unsigned short preVMods;
} PreserveInfo;

#define _KT_Name       (1 << 0)
#define _KT_Mask       (1 << 1)
#define _KT_Map        (1 << 2)
#define _KT_Preserve   (1 << 3)
#define _KT_LevelNames (1 << 4)

typedef struct _KeyTypeInfo {
    CommonInfo defs;
    xkb_atom_t name;
    unsigned file_id;
    unsigned mask;
    unsigned vmask;
    unsigned numLevels;
    darray(struct xkb_kt_map_entry) entries;
    PreserveInfo *preserve;
    darray(xkb_atom_t) lvlNames;
} KeyTypeInfo;

typedef struct _KeyTypesInfo {
    char *name;
    int errorCount;
    unsigned file_id;
    unsigned stdPresent;
    unsigned nTypes;
    KeyTypeInfo *types;
    KeyTypeInfo dflt;
    VModInfo vmods;
} KeyTypesInfo;

static xkb_atom_t tok_ONE_LEVEL;
static xkb_atom_t tok_TWO_LEVEL;
static xkb_atom_t tok_ALPHABETIC;
static xkb_atom_t tok_KEYPAD;

/***====================================================================***/

#define ReportTypeShouldBeArray(keymap, t, f) \
    ReportShouldBeArray("key type", (f), TypeTxt((keymap), (t)))
#define ReportTypeBadType(keymap, t, f, w) \
    ReportBadType("key type", (f), TypeTxt((keymap), (t)), (w))

/***====================================================================***/

#define MapEntryTxt(x, e) \
    XkbcVModMaskText((x), (e)->mods.real_mods, (e)->mods.vmods)
#define PreserveIndexTxt(x, p) \
    XkbcVModMaskText((x), (p)->indexMods, (p)->indexVMods)
#define PreserveTxt(x, p) \
    XkbcVModMaskText((x), (p)->preMods, (p)->preVMods)
#define TypeTxt(keymap, t) \
    xkb_atom_text((keymap)->ctx, (t)->name)
#define TypeMaskTxt(t, x) \
    XkbcVModMaskText((x), (t)->mask, (t)->vmask)

/***====================================================================***/

static void
InitKeyTypesInfo(KeyTypesInfo *info, struct xkb_keymap *keymap,
                 KeyTypesInfo *from, unsigned file_id)
{
    tok_ONE_LEVEL = xkb_atom_intern(keymap->ctx, "ONE_LEVEL");
    tok_TWO_LEVEL = xkb_atom_intern(keymap->ctx, "TWO_LEVEL");
    tok_ALPHABETIC = xkb_atom_intern(keymap->ctx, "ALPHABETIC");
    tok_KEYPAD = xkb_atom_intern(keymap->ctx, "KEYPAD");
    info->name = strdup("default");
    info->errorCount = 0;
    info->stdPresent = 0;
    info->nTypes = 0;
    info->types = NULL;
    info->file_id = file_id;
    info->dflt.defs.defined = 0;
    info->dflt.defs.file_id = file_id;
    info->dflt.defs.merge = MERGE_OVERRIDE;
    info->dflt.defs.next = NULL;
    info->dflt.name = XKB_ATOM_NONE;
    info->dflt.mask = 0;
    info->dflt.vmask = 0;
    info->dflt.numLevels = 1;
    darray_init(info->dflt.entries);
    darray_init(info->dflt.lvlNames);
    info->dflt.preserve = NULL;
    InitVModInfo(&info->vmods, keymap);
    if (from != NULL) {
        info->dflt = from->dflt;

        darray_copy(info->dflt.entries, from->dflt.entries);
        darray_copy(info->dflt.lvlNames, from->dflt.lvlNames);

        if (from->dflt.preserve) {
            PreserveInfo *old, *new, *last;
            last = NULL;
            old = from->dflt.preserve;
            for (; old; old = (PreserveInfo *) old->defs.next) {
                new = uTypedAlloc(PreserveInfo);
                if (!new)
                    return;
                *new = *old;
                new->defs.next = NULL;
                if (last)
                    last->defs.next = (CommonInfo *) new;
                else
                    info->dflt.preserve = new;
                last = new;
            }
        }
    }
}

static void
FreeKeyTypeInfo(KeyTypeInfo * type)
{
    darray_free(type->entries);
    darray_free(type->lvlNames);
    if (type->preserve != NULL) {
        ClearCommonInfo(&type->preserve->defs);
        type->preserve = NULL;
    }
}

static void
FreeKeyTypesInfo(KeyTypesInfo * info)
{
    free(info->name);
    info->name = NULL;
    if (info->types) {
        KeyTypeInfo *type;
        for (type = info->types; type; type =
                 (KeyTypeInfo *) type->defs.next) {
            FreeKeyTypeInfo(type);
        }
        info->types = ClearCommonInfo(&info->types->defs);
    }
    FreeKeyTypeInfo(&info->dflt);
}

static KeyTypeInfo *
NextKeyType(KeyTypesInfo * info)
{
    KeyTypeInfo *type;

    type = uTypedAlloc(KeyTypeInfo);
    if (type != NULL) {
        memset(type, 0, sizeof(KeyTypeInfo));
        type->defs.file_id = info->file_id;
        info->types = AddCommonInfo(&info->types->defs, &type->defs);
        info->nTypes++;
    }
    return type;
}

static KeyTypeInfo *
FindMatchingKeyType(KeyTypesInfo * info, KeyTypeInfo * new)
{
    KeyTypeInfo *old;

    for (old = info->types; old; old = (KeyTypeInfo *) old->defs.next) {
        if (old->name == new->name)
            return old;
    }
    return NULL;
}

static bool
ReportTypeBadWidth(const char *type, int has, int needs)
{
    ERROR("Key type \"%s\" has %d levels, must have %d\n", type, has, needs);
    ACTION("Illegal type definition ignored\n");
    return false;
}

static bool
AddKeyType(struct xkb_keymap *keymap, KeyTypesInfo *info, KeyTypeInfo *new)
{
    KeyTypeInfo *old;

    if (new->name == tok_ONE_LEVEL) {
        if (new->numLevels > 1)
            return ReportTypeBadWidth("ONE_LEVEL", new->numLevels, 1);
        info->stdPresent |= XkbOneLevelMask;
    }
    else if (new->name == tok_TWO_LEVEL) {
        if (new->numLevels > 2)
            return ReportTypeBadWidth("TWO_LEVEL", new->numLevels, 2);
        else if (new->numLevels < 2)
            new->numLevels = 2;
        info->stdPresent |= XkbTwoLevelMask;
    }
    else if (new->name == tok_ALPHABETIC) {
        if (new->numLevels > 2)
            return ReportTypeBadWidth("ALPHABETIC", new->numLevels, 2);
        else if (new->numLevels < 2)
            new->numLevels = 2;
        info->stdPresent |= XkbAlphabeticMask;
    }
    else if (new->name == tok_KEYPAD) {
        if (new->numLevels > 2)
            return ReportTypeBadWidth("KEYPAD", new->numLevels, 2);
        else if (new->numLevels < 2)
            new->numLevels = 2;
        info->stdPresent |= XkbKeypadMask;
    }

    old = FindMatchingKeyType(info, new);
    if (old != NULL) {
        bool report;
        if ((new->defs.merge == MERGE_REPLACE)
            || (new->defs.merge == MERGE_OVERRIDE)) {
            KeyTypeInfo *next = (KeyTypeInfo *) old->defs.next;
            if (((old->defs.file_id == new->defs.file_id)
                 && (warningLevel > 0)) || (warningLevel > 9)) {
                WARN("Multiple definitions of the %s key type\n",
                     xkb_atom_text(keymap->ctx, new->name));
                ACTION("Earlier definition ignored\n");
            }
            FreeKeyTypeInfo(old);
            *old = *new;
            darray_init(new->entries);
            darray_init(new->lvlNames);
            new->preserve = NULL;
            old->defs.next = &next->defs;
            return true;
        }
        report = (old->defs.file_id == new->defs.file_id) &&
                 (warningLevel > 0);
        if (report) {
            WARN("Multiple definitions of the %s key type\n",
                 xkb_atom_text(keymap->ctx, new->name));
            ACTION("Later definition ignored\n");
        }
        FreeKeyTypeInfo(new);
        return true;
    }
    old = NextKeyType(info);
    if (old == NULL)
        return false;
    *old = *new;
    old->defs.next = NULL;
    darray_init(new->entries);
    darray_init(new->lvlNames);
    new->preserve = NULL;
    return true;
}

/***====================================================================***/

static void
MergeIncludedKeyTypes(KeyTypesInfo *into, KeyTypesInfo *from,
                      enum merge_mode merge, struct xkb_keymap *keymap)
{
    KeyTypeInfo *type;

    if (from->errorCount > 0) {
        into->errorCount += from->errorCount;
        return;
    }
    if (into->name == NULL) {
        into->name = from->name;
        from->name = NULL;
    }
    for (type = from->types; type; type = (KeyTypeInfo *) type->defs.next) {
        if (merge != MERGE_DEFAULT)
            type->defs.merge = merge;
        if (!AddKeyType(keymap, into, type))
            into->errorCount++;
    }
    into->stdPresent |= from->stdPresent;
}

static void
HandleKeyTypesFile(XkbFile *file, struct xkb_keymap *keymap,
                   enum merge_mode merge,
                   KeyTypesInfo *info);

static bool
HandleIncludeKeyTypes(IncludeStmt *stmt, struct xkb_keymap *keymap,
                      KeyTypesInfo *info)
{
    enum merge_mode newMerge;
    XkbFile *rtrn;
    KeyTypesInfo included;
    bool haveSelf;

    haveSelf = false;
    if ((stmt->file == NULL) && (stmt->map == NULL)) {
        haveSelf = true;
        included = *info;
        memset(info, 0, sizeof(KeyTypesInfo));
    }
    else if (ProcessIncludeFile(keymap->ctx, stmt, FILE_TYPE_TYPES, &rtrn,
                                &newMerge)) {
        InitKeyTypesInfo(&included, keymap, info, rtrn->id);
        included.dflt.defs.merge = newMerge;

        HandleKeyTypesFile(rtrn, keymap, newMerge, &included);
        if (stmt->stmt != NULL) {
            free(included.name);
            included.name = stmt->stmt;
            stmt->stmt = NULL;
        }
        FreeXKBFile(rtrn);
    }
    else {
        info->errorCount += 10;
        return false;
    }
    if ((stmt->next != NULL) && (included.errorCount < 1)) {
        IncludeStmt *next;
        unsigned op;
        KeyTypesInfo next_incl;

        for (next = stmt->next; next != NULL; next = next->next) {
            if ((next->file == NULL) && (next->map == NULL)) {
                haveSelf = true;
                MergeIncludedKeyTypes(&included, info, next->merge, keymap);
                FreeKeyTypesInfo(info);
            }
            else if (ProcessIncludeFile(keymap->ctx, next, FILE_TYPE_TYPES,
                                        &rtrn, &op)) {
                InitKeyTypesInfo(&next_incl, keymap, &included, rtrn->id);
                next_incl.dflt.defs.merge = op;
                HandleKeyTypesFile(rtrn, keymap, op, &next_incl);
                MergeIncludedKeyTypes(&included, &next_incl, op, keymap);
                FreeKeyTypesInfo(&next_incl);
                FreeXKBFile(rtrn);
            }
            else {
                info->errorCount += 10;
                FreeKeyTypesInfo(&included);
                return false;
            }
        }
    }
    if (haveSelf)
        *info = included;
    else {
        MergeIncludedKeyTypes(info, &included, newMerge, keymap);
        FreeKeyTypesInfo(&included);
    }
    return (info->errorCount == 0);
}

/***====================================================================***/

static struct xkb_kt_map_entry *
FindMatchingMapEntry(KeyTypeInfo * type, unsigned mask, unsigned vmask)
{
    struct xkb_kt_map_entry *entry;

    darray_foreach(entry, type->entries)
    if (entry->mods.real_mods == mask && entry->mods.vmods == vmask)
        return entry;

    return NULL;
}

static void
DeleteLevel1MapEntries(KeyTypeInfo * type)
{
    unsigned int i, n;

    /* TODO: Be just a bit more clever here. */
    for (i = 0; i < darray_size(type->entries); i++) {
        if (darray_item(type->entries, i).level == 0) {
            for (n = i; n < darray_size(type->entries) - 1; n++)
                darray_item(type->entries, n) =
                    darray_item(type->entries, n + 1);
            (void) darray_pop(type->entries);
        }
    }
}

static struct xkb_kt_map_entry *
NextMapEntry(struct xkb_keymap *keymap, KeyTypeInfo * type)
{
    darray_resize0(type->entries, darray_size(type->entries) + 1);
    return &darray_item(type->entries, darray_size(type->entries) - 1);
}

static bool
AddPreserve(struct xkb_keymap *keymap, KeyTypeInfo *type,
            PreserveInfo *new, bool clobber, bool report)
{
    PreserveInfo *old;

    old = type->preserve;
    while (old != NULL)
    {
        if ((old->indexMods != new->indexMods) ||
            (old->indexVMods != new->indexVMods)) {
            old = (PreserveInfo *) old->defs.next;
            continue;
        }
        if ((old->preMods == new->preMods)
            && (old->preVMods == new->preVMods)) {
            if (warningLevel > 9) {
                WARN("Identical definitions for preserve[%s] in %s\n",
                     PreserveIndexTxt(keymap, old), TypeTxt(keymap, type));
                ACTION("Ignored\n");
            }
            return true;
        }
        if (report && (warningLevel > 0)) {
            const char *str;
            WARN("Multiple definitions for preserve[%s] in %s\n",
                 PreserveIndexTxt(keymap, old), TypeTxt(keymap, type));

            if (clobber)
                str = PreserveTxt(keymap, new);
            else
                str = PreserveTxt(keymap, old);
            ACTION("Using %s, ", str);
            if (clobber)
                str = PreserveTxt(keymap, old);
            else
                str = PreserveTxt(keymap, new);
            INFO("ignoring %s\n", str);
        }
        if (clobber) {
            old->preMods = new->preMods;
            old->preVMods = new->preVMods;
        }
        return true;
    }
    old = uTypedAlloc(PreserveInfo);
    if (!old) {
        WSGO("Couldn't allocate preserve in %s\n", TypeTxt(keymap, type));
        ACTION("Preserve[%s] lost\n", PreserveIndexTxt(keymap, new));
        return false;
    }
    *old = *new;
    old->matchingMapIndex = -1;
    type->preserve = AddCommonInfo(&type->preserve->defs, &old->defs);
    return true;
}

/**
 * Add a new KTMapEntry to the given key type. If an entry with the same mods
 * already exists, the level is updated (if clobber is TRUE). Otherwise, a new
 * entry is created.
 *
 * @param clobber Overwrite existing entry.
 * @param report true if a warning is to be printed on.
 */
static bool
AddMapEntry(struct xkb_keymap *keymap, KeyTypeInfo *type,
            struct xkb_kt_map_entry *new, bool clobber, bool report)
{
    struct xkb_kt_map_entry * old;

    if ((old =
             FindMatchingMapEntry(type, new->mods.real_mods,
                                  new->mods.vmods))) {
        if (report && (old->level != new->level)) {
            unsigned use, ignore;
            if (clobber) {
                use = new->level + 1;
                ignore = old->level + 1;
            }
            else {
                use = old->level + 1;
                ignore = new->level + 1;
            }
            WARN("Multiple map entries for %s in %s\n",
                 MapEntryTxt(keymap, new), TypeTxt(keymap, type));
            ACTION("Using %d, ignoring %d\n", use, ignore);
        }
        else if (warningLevel > 9) {
            WARN("Multiple occurences of map[%s]= %d in %s\n",
                 MapEntryTxt(keymap, new), new->level + 1,
                 TypeTxt(keymap, type));
            ACTION("Ignored\n");
            return true;
        }
        if (clobber)
            old->level = new->level;
        return true;
    }
    if ((old = NextMapEntry(keymap, type)) == NULL)
        return false;           /* allocation failure, already reported */
    if (new->level >= type->numLevels)
        type->numLevels = new->level + 1;
    old->mods.mask = new->mods.real_mods;
    old->mods.real_mods = new->mods.real_mods;
    old->mods.vmods = new->mods.vmods;
    old->level = new->level;
    return true;
}

static bool
SetMapEntry(KeyTypeInfo *type, struct xkb_keymap *keymap, ExprDef *arrayNdx,
            ExprDef *value)
{
    ExprResult rtrn;
    struct xkb_kt_map_entry entry;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(keymap, type, "map entry");
    if (!ExprResolveVModMask(arrayNdx, &rtrn, keymap))
        return ReportTypeBadType(keymap, type, "map entry", "modifier mask");
    entry.mods.real_mods = rtrn.uval & 0xff;      /* modifiers < 512 */
    entry.mods.vmods = (rtrn.uval >> 8) & 0xffff; /* modifiers > 512 */
    if ((entry.mods.real_mods & (~type->mask)) ||
        ((entry.mods.vmods & (~type->vmask)) != 0)) {
        if (warningLevel > 0) {
            WARN("Map entry for unused modifiers in %s\n",
                 TypeTxt(keymap, type));
            ACTION("Using %s instead of ",
                   XkbcVModMaskText(keymap,
                                    entry.mods.real_mods & type->mask,
                                    entry.mods.vmods & type->vmask));
            INFO("%s\n", MapEntryTxt(keymap, &entry));
        }
        entry.mods.real_mods &= type->mask;
        entry.mods.vmods &= type->vmask;
    }
    if (!ExprResolveLevel(keymap->ctx, value, &rtrn)) {
        ERROR("Level specifications in a key type must be integer\n");
        ACTION("Ignoring malformed level specification\n");
        return false;
    }
    entry.level = rtrn.ival - 1;
    return AddMapEntry(keymap, type, &entry, true, true);
}

static bool
SetPreserve(KeyTypeInfo *type, struct xkb_keymap *keymap,
            ExprDef *arrayNdx, ExprDef *value)
{
    ExprResult rtrn;
    PreserveInfo new;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(keymap, type, "preserve entry");
    if (!ExprResolveVModMask(arrayNdx, &rtrn, keymap))
        return ReportTypeBadType(keymap, type, "preserve entry",
                                 "modifier mask");
    new.defs = type->defs;
    new.defs.next = NULL;
    new.indexMods = rtrn.uval & 0xff;
    new.indexVMods = (rtrn.uval >> 8) & 0xffff;
    if ((new.indexMods & (~type->mask)) ||
        (new.indexVMods & (~type->vmask))) {
        if (warningLevel > 0) {
            WARN("Preserve for modifiers not used by the %s type\n",
                 TypeTxt(keymap, type));
            ACTION("Index %s converted to ", PreserveIndexTxt(keymap, &new));
        }
        new.indexMods &= type->mask;
        new.indexVMods &= type->vmask;
        if (warningLevel > 0)
            INFO("%s\n", PreserveIndexTxt(keymap, &new));
    }
    if (!ExprResolveVModMask(value, &rtrn, keymap)) {
        ERROR("Preserve value in a key type is not a modifier mask\n");
        ACTION("Ignoring preserve[%s] in type %s\n",
               PreserveIndexTxt(keymap, &new), TypeTxt(keymap, type));
        return false;
    }
    new.preMods = rtrn.uval & 0xff;
    new.preVMods = (rtrn.uval >> 16) & 0xffff;
    if ((new.preMods & (~new.indexMods))
        || (new.preVMods & (~new.indexVMods))) {
        if (warningLevel > 0) {
            WARN("Illegal value for preserve[%s] in type %s\n",
                 PreserveTxt(keymap, &new), TypeTxt(keymap, type));
            ACTION("Converted %s to ", PreserveIndexTxt(keymap, &new));
        }
        new.preMods &= new.indexMods;
        new.preVMods &= new.indexVMods;
        if (warningLevel > 0) {
            INFO("%s\n", PreserveIndexTxt(keymap, &new));
        }
    }
    return AddPreserve(keymap, type, &new, true, true);
}

/***====================================================================***/

static bool
AddLevelName(struct xkb_keymap *keymap, KeyTypeInfo *type,
             unsigned level, xkb_atom_t name, bool clobber)
{
    if (level >= darray_size(type->lvlNames))
        darray_resize0(type->lvlNames, level + 1);

    if (darray_item(type->lvlNames, level) == name) {
        if (warningLevel > 9) {
            WARN("Duplicate names for level %d of key type %s\n",
                 level + 1, TypeTxt(keymap, type));
            ACTION("Ignored\n");
        }
        return true;
    }
    else if (darray_item(type->lvlNames, level) != XKB_ATOM_NONE) {
        if (warningLevel > 0) {
            const char *old, *new;
            old = xkb_atom_text(keymap->ctx,
                                darray_item(type->lvlNames, level));
            new = xkb_atom_text(keymap->ctx, name);
            WARN("Multiple names for level %d of key type %s\n",
                 level + 1, TypeTxt(keymap, type));
            if (clobber)
                ACTION("Using %s, ignoring %s\n", new, old);
            else
                ACTION("Using %s, ignoring %s\n", old, new);
        }

        if (!clobber)
            return true;
    }

    darray_item(type->lvlNames, level) = name;
    return true;
}

static bool
SetLevelName(KeyTypeInfo *type, struct xkb_keymap *keymap, ExprDef *arrayNdx,
             ExprDef *value)
{
    ExprResult rtrn;
    unsigned level;
    xkb_atom_t level_name;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(keymap, type, "level name");
    if (!ExprResolveLevel(keymap->ctx, arrayNdx, &rtrn))
        return ReportTypeBadType(keymap, type, "level name", "integer");
    level = rtrn.ival - 1;
    if (!ExprResolveString(keymap->ctx, value, &rtrn)) {
        ERROR("Non-string name for level %d in key type %s\n", level + 1,
              xkb_atom_text(keymap->ctx, type->name));
        ACTION("Ignoring illegal level name definition\n");
        return false;
    }
    level_name = xkb_atom_intern(keymap->ctx, rtrn.str);
    free(rtrn.str);
    return AddLevelName(keymap, type, level, level_name, true);
}

/***====================================================================***/

/**
 * Parses the fields in a type "..." { } description.
 *
 * @param field The field to parse (e.g. modifiers, map, level_name)
 */
static bool
SetKeyTypeField(KeyTypeInfo *type, struct xkb_keymap *keymap,
                char *field, ExprDef *arrayNdx, ExprDef *value,
                KeyTypesInfo *info)
{
    ExprResult tmp;

    if (strcasecmp(field, "modifiers") == 0) {
        unsigned mods, vmods;
        if (arrayNdx != NULL) {
            WARN("The modifiers field of a key type is not an array\n");
            ACTION("Illegal array subscript ignored\n");
        }
        /* get modifier mask for current type */
        if (!ExprResolveVModMask(value, &tmp, keymap)) {
            ERROR("Key type mask field must be a modifier mask\n");
            ACTION("Key type definition ignored\n");
            return false;
        }
        mods = tmp.uval & 0xff; /* core mods */
        vmods = (tmp.uval >> 8) & 0xffff; /* xkb virtual mods */
        if (type->defs.defined & _KT_Mask) {
            WARN("Multiple modifier mask definitions for key type %s\n",
                 xkb_atom_text(keymap->ctx, type->name));
            ACTION("Using %s, ", TypeMaskTxt(type, keymap));
            INFO("ignoring %s\n", XkbcVModMaskText(keymap, mods, vmods));
            return false;
        }
        type->mask = mods;
        type->vmask = vmods;
        type->defs.defined |= _KT_Mask;
        return true;
    }
    else if (strcasecmp(field, "map") == 0) {
        type->defs.defined |= _KT_Map;
        return SetMapEntry(type, keymap, arrayNdx, value);
    }
    else if (strcasecmp(field, "preserve") == 0) {
        type->defs.defined |= _KT_Preserve;
        return SetPreserve(type, keymap, arrayNdx, value);
    }
    else if ((strcasecmp(field, "levelname") == 0) ||
             (strcasecmp(field, "level_name") == 0)) {
        type->defs.defined |= _KT_LevelNames;
        return SetLevelName(type, keymap, arrayNdx, value);
    }
    ERROR("Unknown field %s in key type %s\n", field, TypeTxt(keymap, type));
    ACTION("Definition ignored\n");
    return false;
}

static bool
HandleKeyTypeVar(VarDef *stmt, struct xkb_keymap *keymap, KeyTypesInfo *info)
{
    ExprResult elem, field;
    ExprDef *arrayNdx;

    if (!ExprResolveLhs(keymap, stmt->name, &elem, &field, &arrayNdx))
        return false;           /* internal error, already reported */
    if (elem.str && (strcasecmp(elem.str, "type") == 0))
        return SetKeyTypeField(&info->dflt, keymap, field.str, arrayNdx,
                               stmt->value, info);
    if (elem.str != NULL) {
        ERROR("Default for unknown element %s\n", uStringText(elem.str));
        ACTION("Value for field %s ignored\n", uStringText(field.str));
    }
    else if (field.str != NULL) {
        ERROR("Default defined for unknown field %s\n",
              uStringText(field.str));
        ACTION("Ignored\n");
    }
    return false;
}

static int
HandleKeyTypeBody(VarDef *def, struct xkb_keymap *keymap,
                  KeyTypeInfo *type, KeyTypesInfo *info)
{
    int ok = 1;
    ExprResult tmp, field;
    ExprDef *arrayNdx;

    for (; def != NULL; def = (VarDef *) def->common.next) {
        if ((def->name) && (def->name->type == ExprFieldRef)) {
            ok = HandleKeyTypeVar(def, keymap, info);
            continue;
        }
        ok = ExprResolveLhs(keymap, def->name, &tmp, &field, &arrayNdx);
        if (ok) {
            ok = SetKeyTypeField(type, keymap, field.str, arrayNdx,
                                 def->value, info);
            free(field.str);
        }
    }
    return ok;
}

/**
 * Process a type "XYZ" { } specification in the xkb_types section.
 *
 */
static int
HandleKeyTypeDef(KeyTypeDef *def, struct xkb_keymap *keymap,
                 enum merge_mode merge, KeyTypesInfo *info)
{
    unsigned int i;
    KeyTypeInfo type;
    struct xkb_kt_map_entry *entry;

    if (def->merge != MERGE_DEFAULT)
        merge = def->merge;

    type.defs.defined = 0;
    type.defs.file_id = info->file_id;
    type.defs.merge = merge;
    type.defs.next = NULL;
    type.name = def->name;
    type.mask = info->dflt.mask;
    type.vmask = info->dflt.vmask;
    type.numLevels = 1;
    darray_init(type.entries);
    darray_init(type.lvlNames);
    type.preserve = NULL;

    /* Parse the actual content. */
    if (!HandleKeyTypeBody(def->body, keymap, &type, info)) {
        info->errorCount++;
        return false;
    }

    /* now copy any appropriate map, preserve or level names from the */
    /* default type */
    darray_foreach(entry, info->dflt.entries) {
        if ((entry->mods.real_mods & type.mask) == entry->mods.real_mods &&
            (entry->mods.vmods & type.vmask) == entry->mods.vmods)
            AddMapEntry(keymap, &type, entry, false, false);
    }
    if (info->dflt.preserve) {
        PreserveInfo *dflt = info->dflt.preserve;
        while (dflt)
        {
            if (((dflt->indexMods & type.mask) == dflt->indexMods) &&
                ((dflt->indexVMods & type.vmask) == dflt->indexVMods)) {
                AddPreserve(keymap, &type, dflt, false, false);
            }
            dflt = (PreserveInfo *) dflt->defs.next;
        }
    }

    for (i = 0; i < darray_size(info->dflt.lvlNames); i++) {
        if (i < type.numLevels &&
            darray_item(info->dflt.lvlNames, i) != XKB_ATOM_NONE) {
            AddLevelName(keymap, &type, i,
                         darray_item(info->dflt.lvlNames, i), false);
        }
    }

    /* Now add the new keytype to the info struct */
    if (!AddKeyType(keymap, info, &type)) {
        info->errorCount++;
        return false;
    }
    return true;
}

/**
 * Process an xkb_types section.
 *
 * @param file The parsed xkb_types section.
 * @param merge Merge Strategy (e.g. MERGE_OVERRIDE)
 * @param info Pointer to memory where the outcome will be stored.
 */
static void
HandleKeyTypesFile(XkbFile *file, struct xkb_keymap *keymap,
                   enum merge_mode merge, KeyTypesInfo *info)
{
    ParseCommon *stmt;

    free(info->name);
    info->name = uDupString(file->name);
    stmt = file->defs;
    while (stmt)
    {
        switch (stmt->stmtType) {
        case StmtInclude:
            if (!HandleIncludeKeyTypes((IncludeStmt *) stmt, keymap, info))
                info->errorCount++;
            break;
        case StmtKeyTypeDef: /* e.g. type "ONE_LEVEL" */
            if (!HandleKeyTypeDef((KeyTypeDef *) stmt, keymap, merge, info))
                info->errorCount++;
            break;
        case StmtVarDef:
            if (!HandleKeyTypeVar((VarDef *) stmt, keymap, info))
                info->errorCount++;
            break;
        case StmtVModDef: /* virtual_modifiers NumLock, ... */
            if (!HandleVModDef((VModDef *) stmt, keymap, merge, &info->vmods))
                info->errorCount++;
            break;
        case StmtKeyAliasDef:
            ERROR("Key type files may not include other declarations\n");
            ACTION("Ignoring definition of key alias\n");
            info->errorCount++;
            break;
        case StmtKeycodeDef:
            ERROR("Key type files may not include other declarations\n");
            ACTION("Ignoring definition of key name\n");
            info->errorCount++;
            break;
        case StmtInterpDef:
            ERROR("Key type files may not include other declarations\n");
            ACTION("Ignoring definition of symbol interpretation\n");
            info->errorCount++;
            break;
        default:
            WSGO("Unexpected statement type %d in HandleKeyTypesFile\n",
                 stmt->stmtType);
            break;
        }
        stmt = stmt->next;
        if (info->errorCount > 10) {
#ifdef NOISY
            ERROR("Too many errors\n");
#endif
            ACTION("Abandoning keytypes file \"%s\"\n", file->topName);
            break;
        }
    }
}

static bool
ComputeEffectiveMap(struct xkb_keymap *keymap, struct xkb_key_type *type)
{
    uint32_t tmp;
    struct xkb_kt_map_entry *entry = NULL;

    if (type->mods.vmods != 0) {
        tmp = VModsToReal(keymap, type->mods.vmods);
        type->mods.mask = tmp | type->mods.real_mods;
        darray_foreach(entry, type->map) {
            tmp = 0;
            if (entry->mods.vmods != 0) {
                tmp = VModsToReal(keymap, entry->mods.vmods);
                if (tmp == 0)
                    continue;
            }
            entry->mods.mask =
                (entry->mods.real_mods | tmp) & type->mods.mask;
        }
    }
    else
        type->mods.mask = type->mods.real_mods;

    return true;
}

static bool
CopyDefToKeyType(struct xkb_keymap *keymap, struct xkb_key_type *type,
                 KeyTypeInfo *def)
{
    unsigned int i;
    PreserveInfo *pre;

    for (pre = def->preserve; pre != NULL;
         pre = (PreserveInfo *) pre->defs.next) {
        struct xkb_kt_map_entry * match;
        struct xkb_kt_map_entry tmp;
        tmp.mods.real_mods = pre->indexMods;
        tmp.mods.vmods = pre->indexVMods;
        tmp.level = 0;
        AddMapEntry(keymap, def, &tmp, false, false);
        match = FindMatchingMapEntry(def, pre->indexMods, pre->indexVMods);
        if (!match) {
            WSGO("Couldn't find matching entry for preserve\n");
            ACTION("Aborting\n");
            return false;
        }
        pre->matchingMapIndex = match - &darray_item(def->entries, 0);
    }
    type->mods.real_mods = def->mask;
    type->mods.vmods = def->vmask;
    type->num_levels = def->numLevels;
    memcpy(&type->map, &def->entries, sizeof(def->entries));
    if (def->preserve) {
        type->preserve = uTypedCalloc(darray_size(type->map), struct xkb_mods);
        if (!type->preserve) {
            WARN("Couldn't allocate preserve array in CopyDefToKeyType\n");
            ACTION("Preserve setting for type %s lost\n",
                   xkb_atom_text(keymap->ctx, def->name));
        }
        else {
            pre = def->preserve;
            for (; pre != NULL; pre = (PreserveInfo *) pre->defs.next) {
                int ndx = pre->matchingMapIndex;
                type->preserve[ndx].mask = pre->preMods;
                type->preserve[ndx].real_mods = pre->preMods;
                type->preserve[ndx].vmods = pre->preVMods;
            }
        }
    }
    else
        type->preserve = NULL;
    type->name = xkb_atom_strdup(keymap->ctx, def->name);

    if (!darray_empty(def->lvlNames)) {
        type->level_names = calloc(darray_size(def->lvlNames),
                                   sizeof(*type->level_names));

        /* assert def->szNames<=def->numLevels */
        for (i = 0; i < darray_size(def->lvlNames); i++)
            type->level_names[i] =
                xkb_atom_strdup(keymap->ctx, darray_item(def->lvlNames, i));
    }
    else {
        type->level_names = NULL;
    }

    darray_init(def->entries);
    return ComputeEffectiveMap(keymap, type);
}

static struct xkb_kt_map_entry map2Level[] = {
    {
        .level = ShiftMask,
        .mods = { .mask = 1, .vmods = ShiftMask, .real_mods = 0 }
    }
};

static struct xkb_kt_map_entry mapAlpha[] = {
    {
        .level = ShiftMask,
        .mods = { .mask = 1, .vmods = ShiftMask, .real_mods = 0 }
    },
    {
        .level = LockMask,
        .mods = { .mask = 0, .vmods = LockMask, .real_mods = 0 }
    }
};

static struct xkb_mods preAlpha[] = {
    { .mask = 0, .vmods = 0, .real_mods = 0 },
    { .mask = LockMask, .vmods = LockMask, .real_mods = 0 }
};

static struct xkb_kt_map_entry mapKeypad[] = {
    {
        .level = ShiftMask,
        .mods = { .mask = 1, .vmods = ShiftMask, .real_mods = 0 }
    },
    {
        .level = 0,
        .mods = { .mask = 1, .vmods = 0, .real_mods = 0 }
    }
};

static const struct xkb_key_type canonicalTypes[XkbNumRequiredTypes] = {
    {
        .mods = { .mask = 0, .vmods = 0, .real_mods = 0 },
        .num_levels = 1,
        .preserve = NULL,
        .name = NULL,
        .level_names = NULL
    },
    {
        .mods = { .mask = ShiftMask, .vmods = ShiftMask, .real_mods = 0 },
        .num_levels = 2,
        .map = darray_lit(map2Level),
        .preserve = NULL,
        .name = NULL,
        .level_names = NULL
    },
    {
        .mods = {
            .mask = ShiftMask | LockMask,
            .vmods = ShiftMask | LockMask,
            .real_mods = 0
        },
        .num_levels = 2,
        .map = darray_lit(mapAlpha),
        .preserve = preAlpha,
        .name = NULL,
        .level_names = NULL
    },
    {
        .mods = { .mask = ShiftMask, .vmods = ShiftMask, .real_mods = 0 },
        .num_levels = 2,
        .map = darray_lit(mapKeypad),
        .preserve = NULL,
        .name = NULL,
        .level_names = NULL
    }
};

static int
InitCanonicalKeyTypes(struct xkb_keymap *keymap, unsigned which,
                      int keypadVMod)
{
    const struct xkb_key_type *from;
    int rtrn;

    darray_growalloc(keymap->types, XkbNumRequiredTypes);

    if ((which & XkbAllRequiredTypes) == 0)
        return Success;

    rtrn = Success;
    from = canonicalTypes;

    if (which & XkbOneLevelMask)
        rtrn = XkbcCopyKeyType(&from[XkbOneLevelIndex],
                               &darray_item(keymap->types, XkbOneLevelIndex));

    if ((which & XkbTwoLevelMask) && rtrn == Success)
        rtrn = XkbcCopyKeyType(&from[XkbTwoLevelIndex],
                               &darray_item(keymap->types, XkbTwoLevelIndex));

    if ((which & XkbAlphabeticMask) && rtrn == Success)
        rtrn = XkbcCopyKeyType(&from[XkbAlphabeticIndex],
                               &darray_item(keymap->types, XkbAlphabeticIndex));

    if ((which & XkbKeypadMask) && rtrn == Success) {
        struct xkb_key_type *type;

        rtrn = XkbcCopyKeyType(&from[XkbKeypadIndex],
                               &darray_item(keymap->types, XkbKeypadIndex));
        type = &darray_item(keymap->types, XkbKeypadIndex);

        if (keypadVMod >= 0 && keypadVMod < XkbNumVirtualMods &&
            rtrn == Success) {
            struct xkb_kt_map_entry *entry;
            type->mods.vmods = (1 << keypadVMod);

            entry = &darray_item(type->map, 0);
            entry->mods.mask = ShiftMask;
            entry->mods.real_mods = ShiftMask;
            entry->mods.vmods = 0;
            entry->level = 1;

            entry = &darray_item(type->map, 1);
            entry->mods.mask = 0;
            entry->mods.real_mods = 0;
            entry->mods.vmods = (1 << keypadVMod);
            entry->level = 1;
        }
    }

    return Success;
}

bool
CompileKeyTypes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge)
{
    unsigned int i;
    struct xkb_key_type *type, *next;
    KeyTypesInfo info;
    KeyTypeInfo *def;

    InitKeyTypesInfo(&info, keymap, NULL, file->id);

    HandleKeyTypesFile(file, keymap, merge, &info);

    if (info.errorCount != 0)
        goto err_info;

    if (info.name)
        keymap->types_section_name = strdup(info.name);

    i = info.nTypes;
    if ((info.stdPresent & XkbOneLevelMask) == 0)
        i++;
    if ((info.stdPresent & XkbTwoLevelMask) == 0)
        i++;
    if ((info.stdPresent & XkbKeypadMask) == 0)
        i++;
    if ((info.stdPresent & XkbAlphabeticMask) == 0)
        i++;

    darray_resize0(keymap->types, i);

    if (XkbAllRequiredTypes & (~info.stdPresent)) {
        unsigned missing, keypadVMod;

        missing = XkbAllRequiredTypes & (~info.stdPresent);
        keypadVMod = FindKeypadVMod(keymap);

        if (InitCanonicalKeyTypes(keymap, missing, keypadVMod) != Success) {
            WSGO("Couldn't initialize canonical key types\n");
            goto err_info;
        }

        if (missing & XkbOneLevelMask)
            darray_item(keymap->types, XkbOneLevelIndex).name =
                xkb_atom_strdup(keymap->ctx, tok_ONE_LEVEL);
        if (missing & XkbTwoLevelMask)
            darray_item(keymap->types, XkbTwoLevelIndex).name =
                xkb_atom_strdup(keymap->ctx, tok_TWO_LEVEL);
        if (missing & XkbAlphabeticMask)
            darray_item(keymap->types, XkbAlphabeticIndex).name =
                xkb_atom_strdup(keymap->ctx, tok_ALPHABETIC);
        if (missing & XkbKeypadMask)
            darray_item(keymap->types, XkbKeypadIndex).name =
                xkb_atom_strdup(keymap->ctx, tok_KEYPAD);
    }

    next = &darray_item(keymap->types, XkbLastRequiredType + 1);
    for (i = 0, def = info.types; i < info.nTypes; i++) {
        if (def->name == tok_ONE_LEVEL)
            type = &darray_item(keymap->types, XkbOneLevelIndex);
        else if (def->name == tok_TWO_LEVEL)
            type = &darray_item(keymap->types, XkbTwoLevelIndex);
        else if (def->name == tok_ALPHABETIC)
            type = &darray_item(keymap->types, XkbAlphabeticIndex);
        else if (def->name == tok_KEYPAD)
            type = &darray_item(keymap->types, XkbKeypadIndex);
        else
            type = next++;

        DeleteLevel1MapEntries(def);

        if (!CopyDefToKeyType(keymap, type, def))
            goto err_info;

        def = (KeyTypeInfo *) def->defs.next;
    }

    FreeKeyTypesInfo(&info);
    return true;

err_info:
    FreeKeyTypesInfo(&info);
    return false;
}
