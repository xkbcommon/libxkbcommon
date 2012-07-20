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
    struct list entry;
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
    unsigned short defined;
    unsigned file_id;
    enum merge_mode merge;
    struct list entry;

    xkb_atom_t name;
    unsigned mask;
    unsigned vmask;
    unsigned numLevels;
    darray(struct xkb_kt_map_entry) entries;
    struct list preserves;
    darray(xkb_atom_t) lvlNames;
} KeyTypeInfo;

typedef struct _KeyTypesInfo {
    char *name;
    int errorCount;
    unsigned file_id;
    unsigned stdPresent;
    unsigned nTypes;
    struct list types;
    KeyTypeInfo dflt;
    VModInfo vmods;

    xkb_atom_t tok_ONE_LEVEL;
    xkb_atom_t tok_TWO_LEVEL;
    xkb_atom_t tok_ALPHABETIC;
    xkb_atom_t tok_KEYPAD;
} KeyTypesInfo;

/***====================================================================***/

static inline const char *
MapEntryTxt(struct xkb_keymap *keymap, struct xkb_kt_map_entry *entry)
{
    return XkbcVModMaskText(keymap, entry->mods.real_mods, entry->mods.vmods);
}

static inline const char *
PreserveIndexTxt(struct xkb_keymap *keymap, PreserveInfo *pi)
{
    return XkbcVModMaskText(keymap, pi->indexMods, pi->indexVMods);
}

static inline const char *
PreserveTxt(struct xkb_keymap *keymap, PreserveInfo *pi)
{
    return XkbcVModMaskText(keymap, pi->preMods, pi->preVMods);
}

static inline const char *
TypeTxt(struct xkb_keymap *keymap, KeyTypeInfo *type)
{
    return xkb_atom_text(keymap->ctx, type->name);
}

static inline const char *
TypeMaskTxt(struct xkb_keymap *keymap, KeyTypeInfo *type)
{
    return XkbcVModMaskText(keymap, type->mask, type->vmask);
}

static inline bool
ReportTypeShouldBeArray(struct xkb_keymap *keymap, KeyTypeInfo *type,
                        const char *field)
{
    return ReportShouldBeArray("key type", field, TypeTxt(keymap, type));
}

static inline bool
ReportTypeBadType(struct xkb_keymap *keymap, KeyTypeInfo *type,
                  const char *field, const char *wanted)
{
    return ReportBadType("key type", field, TypeTxt(keymap, type), wanted);
}

/***====================================================================***/

static void
InitKeyTypesInfo(KeyTypesInfo *info, struct xkb_keymap *keymap,
                 KeyTypesInfo *from, unsigned file_id)
{
    PreserveInfo *old, *new;

    info->tok_ONE_LEVEL = xkb_atom_intern(keymap->ctx, "ONE_LEVEL");
    info->tok_TWO_LEVEL = xkb_atom_intern(keymap->ctx, "TWO_LEVEL");
    info->tok_ALPHABETIC = xkb_atom_intern(keymap->ctx, "ALPHABETIC");
    info->tok_KEYPAD = xkb_atom_intern(keymap->ctx, "KEYPAD");
    info->name = strdup("default");
    info->errorCount = 0;
    info->stdPresent = 0;
    info->nTypes = 0;
    list_init(&info->types);
    info->file_id = file_id;
    info->dflt.defined = 0;
    info->dflt.file_id = file_id;
    info->dflt.merge = MERGE_OVERRIDE;
    info->dflt.name = XKB_ATOM_NONE;
    info->dflt.mask = 0;
    info->dflt.vmask = 0;
    info->dflt.numLevels = 1;
    darray_init(info->dflt.entries);
    darray_init(info->dflt.lvlNames);
    list_init(&info->dflt.preserves);
    InitVModInfo(&info->vmods, keymap);

    if (!from)
        return;

    info->dflt = from->dflt;

    darray_copy(info->dflt.entries, from->dflt.entries);
    darray_copy(info->dflt.lvlNames, from->dflt.lvlNames);

    list_init(&info->dflt.preserves);
    list_foreach(old, &from->dflt.preserves, entry) {
        new = malloc(sizeof(*new));
        if (!new)
            return;

        *new = *old;
        list_append(&new->entry, &info->dflt.preserves);
    }
}

static void
FreeKeyTypeInfo(KeyTypeInfo * type)
{
    PreserveInfo *pi, *next_pi;
    darray_free(type->entries);
    darray_free(type->lvlNames);
    list_foreach_safe(pi, next_pi, &type->preserves, entry)
        free(pi);
    list_init(&type->preserves);
}

static void
FreeKeyTypesInfo(KeyTypesInfo * info)
{
    KeyTypeInfo *type, *next_type;
    free(info->name);
    info->name = NULL;
    list_foreach_safe(type, next_type, &info->types, entry) {
        FreeKeyTypeInfo(type);
        free(type);
    }
    FreeKeyTypeInfo(&info->dflt);
}

static KeyTypeInfo *
NextKeyType(KeyTypesInfo * info)
{
    KeyTypeInfo *type;

    type = calloc(1, sizeof(*type));
    if (!type)
        return NULL;

    list_init(&type->preserves);
    type->file_id = info->file_id;

    list_append(&type->entry, &info->types);
    info->nTypes++;
    return type;
}

static KeyTypeInfo *
FindMatchingKeyType(KeyTypesInfo * info, KeyTypeInfo * new)
{
    KeyTypeInfo *old;

    list_foreach(old, &info->types, entry)
        if (old->name == new->name)
            return old;

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
    struct list type_entry, preserves_entry;

    if (new->name == info->tok_ONE_LEVEL) {
        if (new->numLevels > 1)
            return ReportTypeBadWidth("ONE_LEVEL", new->numLevels, 1);
        info->stdPresent |= XkbOneLevelMask;
    }
    else if (new->name == info->tok_TWO_LEVEL) {
        if (new->numLevels > 2)
            return ReportTypeBadWidth("TWO_LEVEL", new->numLevels, 2);
        else if (new->numLevels < 2)
            new->numLevels = 2;
        info->stdPresent |= XkbTwoLevelMask;
    }
    else if (new->name == info->tok_ALPHABETIC) {
        if (new->numLevels > 2)
            return ReportTypeBadWidth("ALPHABETIC", new->numLevels, 2);
        else if (new->numLevels < 2)
            new->numLevels = 2;
        info->stdPresent |= XkbAlphabeticMask;
    }
    else if (new->name == info->tok_KEYPAD) {
        if (new->numLevels > 2)
            return ReportTypeBadWidth("KEYPAD", new->numLevels, 2);
        else if (new->numLevels < 2)
            new->numLevels = 2;
        info->stdPresent |= XkbKeypadMask;
    }

    old = FindMatchingKeyType(info, new);
    if (old) {
        if (new->merge == MERGE_REPLACE || new->merge == MERGE_OVERRIDE) {
            if ((old->file_id == new->file_id && warningLevel > 0) ||
                warningLevel > 9) {
                WARN("Multiple definitions of the %s key type\n",
                     xkb_atom_text(keymap->ctx, new->name));
                ACTION("Earlier definition ignored\n");
            }

            type_entry = old->entry;
            FreeKeyTypeInfo(old);
            *old = *new;
            old->entry = type_entry;
            darray_init(new->entries);
            darray_init(new->lvlNames);
            list_init(&new->preserves);
            return true;
        }

        if (old->file_id == new->file_id && warningLevel > 0) {
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
    list_replace(&new->preserves, &old->preserves);
    type_entry = old->entry;
    preserves_entry = old->preserves;
    *old = *new;
    old->preserves = preserves_entry;
    old->entry = type_entry;
    darray_init(new->entries);
    darray_init(new->lvlNames);
    list_init(&new->preserves);
    return true;
}

/***====================================================================***/

static void
MergeIncludedKeyTypes(KeyTypesInfo *into, KeyTypesInfo *from,
                      enum merge_mode merge, struct xkb_keymap *keymap)
{
    KeyTypeInfo *type, *next_type;

    if (from->errorCount > 0) {
        into->errorCount += from->errorCount;
        return;
    }

    if (into->name == NULL) {
        into->name = from->name;
        from->name = NULL;
    }

    list_foreach_safe(type, next_type, &from->types, entry) {
        type->merge = (merge == MERGE_DEFAULT ? type->merge : merge);
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
    enum merge_mode merge = MERGE_DEFAULT;
    XkbFile *rtrn;
    KeyTypesInfo included, next_incl;

    InitKeyTypesInfo(&included, keymap, info, info->file_id);
    if (stmt->stmt) {
        free(included.name);
        included.name = stmt->stmt;
        stmt->stmt = NULL;
    }

    for (; stmt; stmt = stmt->next) {
        if (!ProcessIncludeFile(keymap->ctx, stmt, FILE_TYPE_TYPES,
                                &rtrn, &merge)) {
            info->errorCount += 10;
            FreeKeyTypesInfo(&included);
            return false;
        }

        InitKeyTypesInfo(&next_incl, keymap, &included, rtrn->id);
        next_incl.dflt.merge = merge;

        HandleKeyTypesFile(rtrn, keymap, merge, &next_incl);

        MergeIncludedKeyTypes(&included, &next_incl, merge, keymap);

        FreeKeyTypesInfo(&next_incl);
        FreeXKBFile(rtrn);
    }

    MergeIncludedKeyTypes(info, &included, merge, keymap);
    FreeKeyTypesInfo(&included);

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

    list_foreach(old, &type->preserves, entry) {
        if (old->indexMods != new->indexMods ||
            old->indexVMods != new->indexVMods)
            continue;

        if (old->preMods == new->preMods && old->preVMods == new->preVMods) {
            if (warningLevel > 9) {
                WARN("Identical definitions for preserve[%s] in %s\n",
                     PreserveIndexTxt(keymap, old), TypeTxt(keymap, type));
                ACTION("Ignored\n");
            }
            return true;
        }

        if (report && warningLevel > 0) {
            const char *str;
            WARN("Multiple definitions for preserve[%s] in %s\n",
                 PreserveIndexTxt(keymap, old), TypeTxt(keymap, type));
            str = PreserveTxt(keymap, clobber ? new : old);
            ACTION("Using %s, ", str);
            str = PreserveTxt(keymap, clobber ? old : new);
            INFO("ignoring %s\n", str);
        }

        if (clobber) {
            old->preMods = new->preMods;
            old->preVMods = new->preVMods;
        }

        return true;
    }

    old = malloc(sizeof(*old));
    if (!old) {
        WSGO("Couldn't allocate preserve in %s\n", TypeTxt(keymap, type));
        ACTION("Preserve[%s] lost\n", PreserveIndexTxt(keymap, new));
        return false;
    }

    *old = *new;
    old->matchingMapIndex = -1;
    list_append(&old->entry, &type->preserves);

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
        if (type->defined & _KT_Mask) {
            WARN("Multiple modifier mask definitions for key type %s\n",
                 xkb_atom_text(keymap->ctx, type->name));
            ACTION("Using %s, ", TypeMaskTxt(keymap, type));
            INFO("ignoring %s\n", XkbcVModMaskText(keymap, mods, vmods));
            return false;
        }
        type->mask = mods;
        type->vmask = vmods;
        type->defined |= _KT_Mask;
        return true;
    }
    else if (strcasecmp(field, "map") == 0) {
        type->defined |= _KT_Map;
        return SetMapEntry(type, keymap, arrayNdx, value);
    }
    else if (strcasecmp(field, "preserve") == 0) {
        type->defined |= _KT_Preserve;
        return SetPreserve(type, keymap, arrayNdx, value);
    }
    else if ((strcasecmp(field, "levelname") == 0) ||
             (strcasecmp(field, "level_name") == 0)) {
        type->defined |= _KT_LevelNames;
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
    PreserveInfo *pi, *pi_next;

    if (def->merge != MERGE_DEFAULT)
        merge = def->merge;

    type.defined = 0;
    type.file_id = info->file_id;
    type.merge = merge;
    type.name = def->name;
    type.mask = info->dflt.mask;
    type.vmask = info->dflt.vmask;
    type.numLevels = 1;
    darray_init(type.entries);
    darray_init(type.lvlNames);
    list_init(&type.preserves);

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

    list_foreach_safe(pi, pi_next, &info->dflt.preserves, entry) {
        if ((pi->indexMods & type.mask) == pi->indexMods &&
            (pi->indexVMods & type.vmask) == pi->indexVMods)
            AddPreserve(keymap, &type, pi, false, false);
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

    list_foreach(pre, &def->preserves, entry) {
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
    if (!list_empty(&def->preserves)) {
        type->preserve = calloc(darray_size(type->map),
                                sizeof(*type->preserve));
        if (!type->preserve) {
            WARN("Couldn't allocate preserve array in CopyDefToKeyType\n");
            ACTION("Preserve setting for type %s lost\n",
                   xkb_atom_text(keymap->ctx, def->name));
        }
        else {
            list_foreach(pre, &def->preserves, entry) {
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
                xkb_atom_strdup(keymap->ctx, info.tok_ONE_LEVEL);
        if (missing & XkbTwoLevelMask)
            darray_item(keymap->types, XkbTwoLevelIndex).name =
                xkb_atom_strdup(keymap->ctx, info.tok_TWO_LEVEL);
        if (missing & XkbAlphabeticMask)
            darray_item(keymap->types, XkbAlphabeticIndex).name =
                xkb_atom_strdup(keymap->ctx, info.tok_ALPHABETIC);
        if (missing & XkbKeypadMask)
            darray_item(keymap->types, XkbKeypadIndex).name =
                xkb_atom_strdup(keymap->ctx, info.tok_KEYPAD);
    }

    next = &darray_item(keymap->types, XkbLastRequiredType + 1);
    list_foreach(def, &info.types, entry) {
        if (def->name == info.tok_ONE_LEVEL)
            type = &darray_item(keymap->types, XkbOneLevelIndex);
        else if (def->name == info.tok_TWO_LEVEL)
            type = &darray_item(keymap->types, XkbTwoLevelIndex);
        else if (def->name == info.tok_ALPHABETIC)
            type = &darray_item(keymap->types, XkbAlphabeticIndex);
        else if (def->name == info.tok_KEYPAD)
            type = &darray_item(keymap->types, XkbKeypadIndex);
        else
            type = next++;

        DeleteLevel1MapEntries(def);

        if (!CopyDefToKeyType(keymap, type, def))
            goto err_info;
    }

    FreeKeyTypesInfo(&info);
    return true;

err_info:
    FreeKeyTypesInfo(&info);
    return false;
}
