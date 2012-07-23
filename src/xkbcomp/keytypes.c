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
    xkb_mod_mask_t mask;
    xkb_mod_mask_t vmask;
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
    struct xkb_keymap *keymap;

    xkb_atom_t tok_ONE_LEVEL;
    xkb_atom_t tok_TWO_LEVEL;
    xkb_atom_t tok_ALPHABETIC;
    xkb_atom_t tok_KEYPAD;
} KeyTypesInfo;

/***====================================================================***/

static inline const char *
MapEntryTxt(KeyTypesInfo *info, struct xkb_kt_map_entry *entry)
{
    return VModMaskText(info->keymap, entry->mods.real_mods,
                        entry->mods.vmods);
}

static inline const char *
PreserveIndexTxt(KeyTypesInfo *info, PreserveInfo *pi)
{
    return VModMaskText(info->keymap, pi->indexMods, pi->indexVMods);
}

static inline const char *
PreserveTxt(KeyTypesInfo *info, PreserveInfo *pi)
{
    return VModMaskText(info->keymap, pi->preMods, pi->preVMods);
}

static inline const char *
TypeTxt(KeyTypesInfo *info, KeyTypeInfo *type)
{
    return xkb_atom_text(info->keymap->ctx, type->name);
}

static inline const char *
TypeMaskTxt(KeyTypesInfo *info, KeyTypeInfo *type)
{
    return VModMaskText(info->keymap, type->mask, type->vmask);
}

static inline bool
ReportTypeShouldBeArray(KeyTypesInfo *info, KeyTypeInfo *type,
                        const char *field)
{
    return ReportShouldBeArray(info->keymap, "key type", field,
                               TypeTxt(info, type));
}

static inline bool
ReportTypeBadType(KeyTypesInfo *info, KeyTypeInfo *type,
                  const char *field, const char *wanted)
{
    return ReportBadType(info->keymap, "key type", field, TypeTxt(info, type),
                         wanted);
}

static inline bool
ReportTypeBadWidth(KeyTypesInfo *info, const char *type, int has, int needs)
{
    log_err(info->keymap->ctx,
            "Key type \"%s\" has %d levels, must have %d; "
            "Illegal type definition ignored\n",
            type, has, needs);
    return false;
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
    info->keymap = keymap;

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
AddKeyType(KeyTypesInfo *info, KeyTypeInfo *new)
{
    KeyTypeInfo *old;
    struct list type_entry, preserves_entry;
    int verbosity = xkb_get_log_verbosity(info->keymap->ctx);

    if (new->name == info->tok_ONE_LEVEL) {
        if (new->numLevels > 1)
            return ReportTypeBadWidth(info, "ONE_LEVEL", new->numLevels, 1);
        info->stdPresent |= XkbOneLevelMask;
    }
    else if (new->name == info->tok_TWO_LEVEL) {
        if (new->numLevels > 2)
            return ReportTypeBadWidth(info, "TWO_LEVEL", new->numLevels, 2);
        else if (new->numLevels < 2)
            new->numLevels = 2;
        info->stdPresent |= XkbTwoLevelMask;
    }
    else if (new->name == info->tok_ALPHABETIC) {
        if (new->numLevels > 2)
            return ReportTypeBadWidth(info, "ALPHABETIC", new->numLevels, 2);
        else if (new->numLevels < 2)
            new->numLevels = 2;
        info->stdPresent |= XkbAlphabeticMask;
    }
    else if (new->name == info->tok_KEYPAD) {
        if (new->numLevels > 2)
            return ReportTypeBadWidth(info, "KEYPAD", new->numLevels, 2);
        else if (new->numLevels < 2)
            new->numLevels = 2;
        info->stdPresent |= XkbKeypadMask;
    }

    old = FindMatchingKeyType(info, new);
    if (old) {
        if (new->merge == MERGE_REPLACE || new->merge == MERGE_OVERRIDE) {
            if ((old->file_id == new->file_id && verbosity > 0) ||
                verbosity > 9) {
                log_warn(info->keymap->ctx,
                         "Multiple definitions of the %s key type; "
                         "Earlier definition ignored\n",
                         xkb_atom_text(info->keymap->ctx, new->name));
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

        if (old->file_id == new->file_id)
            log_lvl(info->keymap->ctx, 4,
                    "Multiple definitions of the %s key type; "
                    "Later definition ignored\n",
                    xkb_atom_text(info->keymap->ctx, new->name));

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
                      enum merge_mode merge)
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
        if (!AddKeyType(into, type))
            into->errorCount++;
    }

    into->stdPresent |= from->stdPresent;
}

static void
HandleKeyTypesFile(KeyTypesInfo *info, XkbFile *file, enum merge_mode merge);

static bool
HandleIncludeKeyTypes(KeyTypesInfo *info, IncludeStmt *stmt)
{
    enum merge_mode merge = MERGE_DEFAULT;
    XkbFile *rtrn;
    KeyTypesInfo included, next_incl;

    InitKeyTypesInfo(&included, info->keymap, info, info->file_id);
    if (stmt->stmt) {
        free(included.name);
        included.name = stmt->stmt;
        stmt->stmt = NULL;
    }

    for (; stmt; stmt = stmt->next_incl) {
        if (!ProcessIncludeFile(info->keymap->ctx, stmt, FILE_TYPE_TYPES,
                                &rtrn, &merge)) {
            info->errorCount += 10;
            FreeKeyTypesInfo(&included);
            return false;
        }

        InitKeyTypesInfo(&next_incl, info->keymap, &included, rtrn->id);
        next_incl.dflt.merge = merge;

        HandleKeyTypesFile(&next_incl, rtrn, merge);

        MergeIncludedKeyTypes(&included, &next_incl, merge);

        FreeKeyTypesInfo(&next_incl);
        FreeXKBFile(rtrn);
    }

    MergeIncludedKeyTypes(info, &included, merge);
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
NextMapEntry(KeyTypesInfo *info, KeyTypeInfo * type)
{
    darray_resize0(type->entries, darray_size(type->entries) + 1);
    return &darray_item(type->entries, darray_size(type->entries) - 1);
}

static bool
AddPreserve(KeyTypesInfo *info, KeyTypeInfo *type,
            PreserveInfo *new, bool clobber, bool report)
{
    PreserveInfo *old;

    list_foreach(old, &type->preserves, entry) {
        if (old->indexMods != new->indexMods ||
            old->indexVMods != new->indexVMods)
            continue;

        if (old->preMods == new->preMods && old->preVMods == new->preVMods) {
            log_lvl(info->keymap->ctx, 10,
                    "Identical definitions for preserve[%s] in %s; "
                    "Ignored\n",
                    PreserveIndexTxt(info, old), TypeTxt(info, type));
            return true;
        }

        if (report)
            log_lvl(info->keymap->ctx, 1,
                    "Multiple definitions for preserve[%s] in %s; "
                    "Using %s, ignoring %s\n",
                    PreserveIndexTxt(info, old), TypeTxt(info, type),
                    PreserveTxt(info, clobber ? new : old),
                    PreserveTxt(info, clobber ? old : new));

        if (clobber) {
            old->preMods = new->preMods;
            old->preVMods = new->preVMods;
        }

        return true;
    }

    old = malloc(sizeof(*old));
    if (!old) {
        log_wsgo(info->keymap->ctx,
                 "Couldn't allocate preserve in %s; Preserve[%s] lost\n",
                 TypeTxt(info, type), PreserveIndexTxt(info, new));
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
AddMapEntry(KeyTypesInfo *info, KeyTypeInfo *type,
            struct xkb_kt_map_entry *new, bool clobber, bool report)
{
    struct xkb_kt_map_entry * old;

    if ((old = FindMatchingMapEntry(type, new->mods.real_mods,
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
            log_warn(info->keymap->ctx,
                     "Multiple map entries for %s in %s; "
                     "Using %d, ignoring %d\n",
                     MapEntryTxt(info, new), TypeTxt(info, type), use,
                     ignore);
        }
        else {
            log_lvl(info->keymap->ctx, 10,
                    "Multiple occurences of map[%s]= %d in %s; Ignored\n",
                    MapEntryTxt(info, new), new->level + 1,
                    TypeTxt(info, type));
            return true;
        }
        if (clobber)
            old->level = new->level;
        return true;
    }
    if ((old = NextMapEntry(info, type)) == NULL)
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
SetMapEntry(KeyTypesInfo *info, KeyTypeInfo *type, ExprDef *arrayNdx,
            ExprDef *value)
{
    ExprResult rtrn;
    struct xkb_kt_map_entry entry;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(info, type, "map entry");

    if (!ExprResolveVModMask(info->keymap, arrayNdx, &rtrn))
        return ReportTypeBadType(info, type, "map entry", "modifier mask");

    entry.mods.real_mods = rtrn.uval & 0xff;      /* modifiers < 512 */
    entry.mods.vmods = (rtrn.uval >> 8) & 0xffff; /* modifiers > 512 */

    if ((entry.mods.real_mods & (~type->mask)) ||
        ((entry.mods.vmods & (~type->vmask)) != 0)) {
        log_lvl(info->keymap->ctx, 1,
                "Map entry for unused modifiers in %s; "
                "Using %s instead of %s\n",
                TypeTxt(info, type),
                VModMaskText(info->keymap,
                             entry.mods.real_mods & type->mask,
                             entry.mods.vmods & type->vmask),
                MapEntryTxt(info, &entry));
        entry.mods.real_mods &= type->mask;
        entry.mods.vmods &= type->vmask;
    }

    if (!ExprResolveLevel(info->keymap->ctx, value, &rtrn)) {
        log_err(info->keymap->ctx,
                "Level specifications in a key type must be integer; "
                "Ignoring malformed level specification\n");
        return false;
    }

    entry.level = rtrn.ival - 1;
    return AddMapEntry(info, type, &entry, true, true);
}

static bool
SetPreserve(KeyTypesInfo *info, KeyTypeInfo *type, ExprDef *arrayNdx,
            ExprDef *value)
{
    ExprResult rtrn;
    PreserveInfo new;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(info, type, "preserve entry");

    if (!ExprResolveVModMask(info->keymap, arrayNdx, &rtrn))
        return ReportTypeBadType(info, type, "preserve entry",
                                 "modifier mask");

    new.indexMods = rtrn.uval & 0xff;
    new.indexVMods = (rtrn.uval >> 8) & 0xffff;

    if ((new.indexMods & (~type->mask)) ||
        (new.indexVMods & (~type->vmask))) {
        const char *before = PreserveIndexTxt(info, &new);

        new.indexMods &= type->mask;
        new.indexVMods &= type->vmask;

        log_lvl(info->keymap->ctx, 1,
                "Preserve for modifiers not used by the %s type; "
                "Index %s converted to %s\n",
                TypeTxt(info, type), before,
                PreserveIndexTxt(info, &new));
    }

    if (!ExprResolveVModMask(info->keymap, value, &rtrn)) {
        log_err(info->keymap->ctx,
                "Preserve value in a key type is not a modifier mask; "
                "Ignoring preserve[%s] in type %s\n",
                PreserveIndexTxt(info, &new), TypeTxt(info, type));
        return false;
    }

    new.preMods = rtrn.uval & 0xff;
    new.preVMods = (rtrn.uval >> 16) & 0xffff;

    if ((new.preMods & (~new.indexMods)) ||
        (new.preVMods & (~new.indexVMods))) {
        const char *before = PreserveIndexTxt(info, &new);

        new.preMods &= new.indexMods;
        new.preVMods &= new.indexVMods;

        log_lvl(info->keymap->ctx, 1,
                "Illegal value for preserve[%s] in type %s; "
                "Converted %s to %s\n",
                PreserveTxt(info, &new), TypeTxt(info, type),
                before, PreserveIndexTxt(info, &new));
    }

    return AddPreserve(info, type, &new, true, true);
}

/***====================================================================***/

static bool
AddLevelName(KeyTypesInfo *info, KeyTypeInfo *type,
             unsigned level, xkb_atom_t name, bool clobber)
{
    if (level >= darray_size(type->lvlNames))
        darray_resize0(type->lvlNames, level + 1);

    if (darray_item(type->lvlNames, level) == name) {
        log_lvl(info->keymap->ctx, 10,
                "Duplicate names for level %d of key type %s; Ignored\n",
                level + 1, TypeTxt(info, type));
        return true;
    }
    else if (darray_item(type->lvlNames, level) != XKB_ATOM_NONE) {
        if (xkb_get_log_verbosity(info->keymap->ctx) > 0) {
            const char *old, *new;
            old = xkb_atom_text(info->keymap->ctx,
                                darray_item(type->lvlNames, level));
            new = xkb_atom_text(info->keymap->ctx, name);
            log_lvl(info->keymap->ctx, 1,
                    "Multiple names for level %d of key type %s; "
                    "Using %s, ignoring %s\n",
                    level + 1, TypeTxt(info, type),
                    (clobber ? new : old), (clobber ? old : new));
        }

        if (!clobber)
            return true;
    }

    darray_item(type->lvlNames, level) = name;
    return true;
}

static bool
SetLevelName(KeyTypesInfo *info, KeyTypeInfo *type, ExprDef *arrayNdx,
             ExprDef *value)
{
    ExprResult rtrn;
    unsigned level;
    xkb_atom_t level_name;
    struct xkb_context *ctx = info->keymap->ctx;
    const char *str;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(info, type, "level name");

    if (!ExprResolveLevel(ctx, arrayNdx, &rtrn))
        return ReportTypeBadType(info, type, "level name", "integer");
    level = rtrn.ival - 1;

    if (!ExprResolveString(ctx, value, &str)) {
        log_err(info->keymap->ctx,
                "Non-string name for level %d in key type %s; "
                "Ignoring illegal level name definition\n",
                level + 1, xkb_atom_text(ctx, type->name));
        return false;
    }

    level_name = xkb_atom_intern(ctx, str);

    return AddLevelName(info, type, level, level_name, true);
}

/***====================================================================***/

/**
 * Parses the fields in a type "..." { } description.
 *
 * @param field The field to parse (e.g. modifiers, map, level_name)
 */
static bool
SetKeyTypeField(KeyTypesInfo *info, KeyTypeInfo *type,
                const char *field, ExprDef *arrayNdx, ExprDef *value)
{
    ExprResult tmp;

    if (istreq(field, "modifiers")) {
        unsigned mods, vmods;
        if (arrayNdx != NULL)
            log_warn(info->keymap->ctx,
                     "The modifiers field of a key type is not an array; "
                     "Illegal array subscript ignored\n");
        /* get modifier mask for current type */
        if (!ExprResolveVModMask(info->keymap, value, &tmp)) {
            log_err(info->keymap->ctx,
                    "Key type mask field must be a modifier mask; "
                    "Key type definition ignored\n");
            return false;
        }
        mods = tmp.uval & 0xff; /* core mods */
        vmods = (tmp.uval >> 8) & 0xffff; /* xkb virtual mods */
        if (type->defined & _KT_Mask) {
            log_warn(info->keymap->ctx,
                     "Multiple modifier mask definitions for key type %s; "
                     "Using %s, ignoring %s\n",
                     xkb_atom_text(info->keymap->ctx, type->name),
                     TypeMaskTxt(info, type),
                     VModMaskText(info->keymap, mods, vmods));
            return false;
        }
        type->mask = mods;
        type->vmask = vmods;
        type->defined |= _KT_Mask;
        return true;
    }
    else if (istreq(field, "map")) {
        type->defined |= _KT_Map;
        return SetMapEntry(info, type, arrayNdx, value);
    }
    else if (istreq(field, "preserve")) {
        type->defined |= _KT_Preserve;
        return SetPreserve(info, type, arrayNdx, value);
    }
    else if (istreq(field, "levelname") || istreq(field, "level_name")) {
        type->defined |= _KT_LevelNames;
        return SetLevelName(info, type, arrayNdx, value);
    }

    log_err(info->keymap->ctx,
            "Unknown field %s in key type %s; Definition ignored\n",
            field, TypeTxt(info, type));

    return false;
}

static bool
HandleKeyTypeVar(KeyTypesInfo *info, VarDef *stmt)
{
    const char *elem, *field;
    ExprDef *arrayNdx;

    if (!ExprResolveLhs(info->keymap->ctx, stmt->name, &elem, &field,
                        &arrayNdx))
        return false;           /* internal error, already reported */

    if (elem && istreq(elem, "type"))
        return SetKeyTypeField(info, &info->dflt, field, arrayNdx,
                               stmt->value);

    if (elem) {
        log_err(info->keymap->ctx,
                "Default for unknown element %s; "
                "Value for field %s ignored\n", field, elem);
    }
    else if (field) {
        log_err(info->keymap->ctx,
                "Default defined for unknown field %s; Ignored\n", field);
    }

    return false;
}

static int
HandleKeyTypeBody(KeyTypesInfo *info, VarDef *def, KeyTypeInfo *type)
{
    int ok = 1;
    const char *elem, *field;
    ExprDef *arrayNdx;

    for (; def; def = (VarDef *) def->common.next) {
        if (def->name && def->name->op == EXPR_FIELD_REF) {
            ok = HandleKeyTypeVar(info, def);
            continue;
        }
        ok = ExprResolveLhs(info->keymap->ctx, def->name, &elem, &field,
                            &arrayNdx);
        if (ok) {
            ok = SetKeyTypeField(info, type, field, arrayNdx,
                                 def->value);
        }
    }

    return ok;
}

/**
 * Process a type "XYZ" { } specification in the xkb_types section.
 *
 */
static int
HandleKeyTypeDef(KeyTypesInfo *info, KeyTypeDef *def, enum merge_mode merge)
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
    if (!HandleKeyTypeBody(info, def->body, &type)) {
        info->errorCount++;
        return false;
    }

    /* now copy any appropriate map, preserve or level names from the */
    /* default type */
    darray_foreach(entry, info->dflt.entries) {
        if ((entry->mods.real_mods & type.mask) == entry->mods.real_mods &&
            (entry->mods.vmods & type.vmask) == entry->mods.vmods)
            AddMapEntry(info, &type, entry, false, false);
    }

    list_foreach_safe(pi, pi_next, &info->dflt.preserves, entry) {
        if ((pi->indexMods & type.mask) == pi->indexMods &&
            (pi->indexVMods & type.vmask) == pi->indexVMods)
            AddPreserve(info, &type, pi, false, false);
    }

    for (i = 0; i < darray_size(info->dflt.lvlNames); i++) {
        if (i < type.numLevels &&
            darray_item(info->dflt.lvlNames, i) != XKB_ATOM_NONE) {
            AddLevelName(info, &type, i,
                         darray_item(info->dflt.lvlNames, i), false);
        }
    }

    /* Now add the new keytype to the info struct */
    if (!AddKeyType(info, &type)) {
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
HandleKeyTypesFile(KeyTypesInfo *info, XkbFile *file, enum merge_mode merge)
{
    ParseCommon *stmt;

    free(info->name);
    info->name = strdup_safe(file->name);
    stmt = file->defs;
    while (stmt)
    {
        switch (stmt->type) {
        case STMT_INCLUDE:
            if (!HandleIncludeKeyTypes(info, (IncludeStmt *) stmt))
                info->errorCount++;
            break;
        case STMT_TYPE: /* e.g. type "ONE_LEVEL" */
            if (!HandleKeyTypeDef(info, (KeyTypeDef *) stmt, merge))
                info->errorCount++;
            break;
        case STMT_VAR:
            if (!HandleKeyTypeVar(info, (VarDef *) stmt))
                info->errorCount++;
            break;
        case STMT_VMOD: /* virtual_modifiers NumLock, ... */
            if (!HandleVModDef((VModDef *) stmt, info->keymap, merge,
                               &info->vmods))
                info->errorCount++;
            break;
        case STMT_ALIAS:
            log_err(info->keymap->ctx,
                    "Key type files may not include other declarations; "
                    "Ignoring definition of key alias\n");
            info->errorCount++;
            break;
        case STMT_KEYCODE:
            log_err(info->keymap->ctx,
                    "Key type files may not include other declarations; "
                    "Ignoring definition of key name\n");
            info->errorCount++;
            break;
        case STMT_INTERP:
            log_err(info->keymap->ctx,
                    "Key type files may not include other declarations; "
                    "Ignoring definition of symbol interpretation\n");
            info->errorCount++;
            break;
        default:
            log_wsgo(info->keymap->ctx,
                     "Unexpected statement type %d in HandleKeyTypesFile\n",
                     stmt->type);
            break;
        }
        stmt = stmt->next;
        if (info->errorCount > 10) {
            log_err(info->keymap->ctx,
                    "Abandoning keytypes file \"%s\"\n", file->topName);
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
CopyDefToKeyType(KeyTypesInfo *info, KeyTypeInfo *def,
                 struct xkb_key_type *type)
{
    unsigned int i;
    PreserveInfo *pre;
    struct xkb_keymap *keymap = info->keymap;

    list_foreach(pre, &def->preserves, entry) {
        struct xkb_kt_map_entry * match;
        struct xkb_kt_map_entry tmp;
        tmp.mods.real_mods = pre->indexMods;
        tmp.mods.vmods = pre->indexVMods;
        tmp.level = 0;
        AddMapEntry(info, def, &tmp, false, false);
        match = FindMatchingMapEntry(def, pre->indexMods, pre->indexVMods);
        if (!match) {
            log_wsgo(info->keymap->ctx,
                     "Couldn't find matching entry for preserve; Aborting\n");
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
            log_warn(info->keymap->ctx,
                     "Couldn't allocate preserve array in CopyDefToKeyType; "
                     "Preserve setting for type %s lost\n",
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
    type->name = xkb_atom_text(keymap->ctx, def->name);

    if (!darray_empty(def->lvlNames)) {
        type->level_names = calloc(darray_size(def->lvlNames),
                                   sizeof(*type->level_names));

        /* assert def->szNames<=def->numLevels */
        for (i = 0; i < darray_size(def->lvlNames); i++)
            type->level_names[i] =
                xkb_atom_text(keymap->ctx, darray_item(def->lvlNames, i));
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
    [XkbOneLevelIndex] = {
        .mods = { .mask = 0, .vmods = 0, .real_mods = 0 },
        .num_levels = 1,
        .preserve = NULL,
        .name = NULL,
        .level_names = NULL
    },
    [XkbTwoLevelIndex] = {
        .mods = { .mask = ShiftMask, .vmods = ShiftMask, .real_mods = 0 },
        .num_levels = 2,
        .map = darray_lit(map2Level),
        .preserve = NULL,
        .name = NULL,
        .level_names = NULL
    },
    [XkbAlphabeticIndex] = {
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
    [XkbKeypadIndex] = {
        .mods = { .mask = ShiftMask, .vmods = ShiftMask, .real_mods = 0 },
        .num_levels = 2,
        .map = darray_lit(mapKeypad),
        .preserve = NULL,
        .name = NULL,
        .level_names = NULL
    }
};

static int
CopyKeyType(const struct xkb_key_type *from, struct xkb_key_type *into)
{
    int i;

    darray_free(into->map);
    free(into->preserve);
    free(into->level_names);

    *into = *from;
    darray_init(into->map);

    darray_copy(into->map, from->map);

    if (from->preserve && !darray_empty(into->map)) {
        into->preserve = calloc(darray_size(into->map),
                                sizeof(*into->preserve));
        if (!into->preserve)
            return BadAlloc;
        memcpy(into->preserve, from->preserve,
               darray_size(into->map) * sizeof(*into->preserve));
    }

    if (from->level_names && into->num_levels > 0) {
        into->level_names = calloc(into->num_levels,
                                   sizeof(*into->level_names));
        if (!into->level_names)
            return BadAlloc;

        for (i = 0; i < into->num_levels; i++)
            into->level_names[i] = strdup(from->level_names[i]);
    }

    return Success;
}

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
        rtrn = CopyKeyType(&from[XkbOneLevelIndex],
                           &darray_item(keymap->types, XkbOneLevelIndex));

    if ((which & XkbTwoLevelMask) && rtrn == Success)
        rtrn = CopyKeyType(&from[XkbTwoLevelIndex],
                           &darray_item(keymap->types, XkbTwoLevelIndex));

    if ((which & XkbAlphabeticMask) && rtrn == Success)
        rtrn = CopyKeyType(&from[XkbAlphabeticIndex],
                           &darray_item(keymap->types, XkbAlphabeticIndex));

    if ((which & XkbKeypadMask) && rtrn == Success) {
        struct xkb_key_type *type;

        rtrn = CopyKeyType(&from[XkbKeypadIndex],
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

    HandleKeyTypesFile(&info, file, merge);

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
            log_wsgo(info.keymap->ctx,
                     "Couldn't initialize canonical key types\n");
            goto err_info;
        }

        if (missing & XkbOneLevelMask)
            darray_item(keymap->types, XkbOneLevelIndex).name =
                xkb_atom_text(keymap->ctx, info.tok_ONE_LEVEL);
        if (missing & XkbTwoLevelMask)
            darray_item(keymap->types, XkbTwoLevelIndex).name =
                xkb_atom_text(keymap->ctx, info.tok_TWO_LEVEL);
        if (missing & XkbAlphabeticMask)
            darray_item(keymap->types, XkbAlphabeticIndex).name =
                xkb_atom_text(keymap->ctx, info.tok_ALPHABETIC);
        if (missing & XkbKeypadMask)
            darray_item(keymap->types, XkbKeypadIndex).name =
                xkb_atom_text(keymap->ctx, info.tok_KEYPAD);
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

        if (!CopyDefToKeyType(&info, def, type))
            goto err_info;
    }

    FreeKeyTypesInfo(&info);
    return true;

err_info:
    FreeKeyTypesInfo(&info);
    return false;
}
