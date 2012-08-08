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

/*
 * The xkb_types section
 * =====================
 * This section is the second to be processesed, after xkb_keycodes.
 * However, it is completely independent and could have been the first
 * to be processed (it does not refer to specific keys as specified in
 * the xkb_keycodes section).
 *
 * This section defines key types, which, given a key and a keyboard
 * state (i.e. modifier state and group), determine the shift level to
 * be used in translating the key to keysyms. These types are assigned
 * to each group in each key, in the xkb_symbols section.
 *
 * Key types are called this way because, in a way, they really describe
 * the "type" of the key (or more correctly, a specific group of the
 * key). For example, an ordinary keymap will provide a type called
 * "KEYPAD", which consists of two levels, with the second level being
 * chosen according to the state of the Num Lock (or Shift) modifiers.
 * Another example is a type called "ONE_LEVEL", which is usually
 * assigned to keys such as Escape; these have just one level and are
 * not affected by the modifier state. Yet more common examples are
 * "TWO_LEVEL" (with Shift choosing the second level), "ALPHABETIC"
 * (where Caps Lock may also choose the second level), etc.
 *
 * Type definitions
 * ----------------
 *  Statements of the form:
 *      type "FOUR_LEVEL" { ... }
 *
 * The above would create a new type named "FOUR_LEVEL".
 * The body of the definition may include statements of the following
 * forms:
 *
 * - level_name statements (mandatory for each level in the type):
 *      level_name[Level1] = "Base";
 *
 *   Gives each level in this type a descriptive name. It isn't used
 *   for any thing.
 *   Note: A level may be specified as Level[1-8] or just a number (can
 *   be more than 8).
 *
 * - modifiers statement (mandatory, should be specified only once):
 *      modifiers = Shift+Lock+LevelThree;
 *
 *   A mask of real and virtual modifiers. These are the only modifiers
 *   being considered when matching the modifier state against the type.
 *   The other modifiers, whether active or not, are masked out in the
 *   calculation.
 *
 * - map entry statements (should have at least as many mappings as there
 *   are levels in the type):
 *      map[Shift+LevelThree] = Level4;
 *
 *   If the active modifiers, masked with the type's modifiers (as stated
 *   above), match (i.e. equal) the modifiers inside the map[] statement,
 *   then the level in the right hand side is chosen. For example, in the
 *   above, if in the current keyboard state the Shift and LevelThree
 *   modifiers are active, while the Lock modifier is not, then the
 *   keysym(s) in the 4th level of the group will be returned to the
 *   user.
 *
 * - preserve statements:
 *      map[Shift+Lock+LevelThree] = Level5;
 *      preserve[Shift+Lock+LevelThree] = Lock;
 *
 *   When a map entry matches the active modifiers and the level it
 *   specified is chosen, then these modifiers are said to be "consumed";
 *   for example, in a simple US keymap where the "g" key is assigned an
 *   ordinary ALPHABETIC key type, if the Lock (Caps Lock) modifier is
 *   active and the key is pressed, then a "G" keysym is produced (as
 *   opposed to lower-case "g"). This is because the type definition has
 *   a map entry like the following:
 *      map[Lock] = Level2;
 *   And as such the Lock modifier is consumed. This information is
 *   relevant for applications which further process the modifiers,
 *   since by then the consumed modifiers have already "done their part"
 *   and should be masked out.
 *
 *   However, sometimes even if a modifier is actually used to choose
 *   the shift level (as Lock above), it should *not* be reported as
 *   consumed, for various reasons. In this case, a preserve[] statement
 *   can be used to augment the map entry. The modifiers inside the square
 *   brackets should match one of the map[] statements in the type. The
 *   right hand side should consists of modifiers from the left hand
 *   side; these modifiers are then "preserved" and not reported as
 *   consumed.
 *
 * Virtual modifier statements
 * ---------------------------
 * Statements of the form:
 *     virtual_modifiers LControl;
 *
 * Can appear in the xkb_types, xkb_compat, xkb_symbols sections.
 * TODO
 */

typedef struct _PreserveInfo {
    struct list entry;
    xkb_mod_mask_t indexMods;
    xkb_mod_mask_t preMods;
    xkb_mod_mask_t indexVMods;
    xkb_mod_mask_t preVMods;
} PreserveInfo;

enum type_field {
    TYPE_FIELD_MASK       = (1 << 0),
    TYPE_FIELD_MAP        = (1 << 1),
    TYPE_FIELD_PRESERVE   = (1 << 2),
    TYPE_FIELD_LEVEL_NAME = (1 << 3),
};

typedef struct _KeyTypeInfo {
    enum type_field defined;
    unsigned file_id;
    enum merge_mode merge;
    struct list entry;

    xkb_atom_t name;
    xkb_mod_mask_t mask;
    xkb_mod_mask_t vmask;
    xkb_level_index_t num_levels;
    darray(struct xkb_kt_map_entry) entries;
    struct list preserves;
    darray(xkb_atom_t) level_names;
} KeyTypeInfo;

typedef struct _KeyTypesInfo {
    char *name;
    int errorCount;
    unsigned file_id;
    unsigned num_types;
    struct list types;
    VModInfo vmods;
    struct xkb_keymap *keymap;
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
                 unsigned file_id)
{
    info->name = strdup("default");
    info->errorCount = 0;
    info->num_types = 0;
    list_init(&info->types);
    info->file_id = file_id;
    InitVModInfo(&info->vmods, keymap);
    info->keymap = keymap;
}

static void
FreeKeyTypeInfo(KeyTypeInfo * type)
{
    PreserveInfo *pi, *next_pi;
    darray_free(type->entries);
    darray_free(type->level_names);
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
    info->num_types++;
    return type;
}

static KeyTypeInfo *
FindMatchingKeyType(KeyTypesInfo *info, xkb_atom_t name)
{
    KeyTypeInfo *old;

    list_foreach(old, &info->types, entry)
        if (old->name == name)
            return old;

    return NULL;
}

static bool
AddKeyType(KeyTypesInfo *info, KeyTypeInfo *new)
{
    KeyTypeInfo *old;
    struct list type_entry, preserves_entry;
    int verbosity = xkb_get_log_verbosity(info->keymap->ctx);

    old = FindMatchingKeyType(info, new->name);
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
            darray_init(new->level_names);
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
    if (!old)
        return false;

    list_replace(&new->preserves, &old->preserves);
    type_entry = old->entry;
    preserves_entry = old->preserves;
    *old = *new;
    old->preserves = preserves_entry;
    old->entry = type_entry;
    darray_init(new->entries);
    darray_init(new->level_names);
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
}

static void
HandleKeyTypesFile(KeyTypesInfo *info, XkbFile *file, enum merge_mode merge);

static bool
HandleIncludeKeyTypes(KeyTypesInfo *info, IncludeStmt *stmt)
{
    enum merge_mode merge = MERGE_DEFAULT;
    XkbFile *rtrn;
    KeyTypesInfo included, next_incl;

    InitKeyTypesInfo(&included, info->keymap, info->file_id);
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

        InitKeyTypesInfo(&next_incl, info->keymap, rtrn->id);

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

static bool
SetModifiers(KeyTypesInfo *info, KeyTypeInfo *type, ExprDef *arrayNdx,
             ExprDef *value)
{
    xkb_mod_mask_t mask, mods, vmods;

    if (arrayNdx)
        log_warn(info->keymap->ctx,
                 "The modifiers field of a key type is not an array; "
                 "Illegal array subscript ignored\n");

    /* get modifier mask for current type */
    if (!ExprResolveVModMask(info->keymap, value, &mask)) {
        log_err(info->keymap->ctx,
                "Key type mask field must be a modifier mask; "
                "Key type definition ignored\n");
        return false;
    }

    mods = mask & 0xff; /* core mods */
    vmods = (mask >> XkbNumModifiers) & 0xffff; /* xkb virtual mods */

    if (type->defined & TYPE_FIELD_MASK) {
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
    return true;
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

    old = FindMatchingMapEntry(type, new->mods.real_mods, new->mods.vmods);
    if (old) {
        if (report && old->level != new->level) {
            xkb_level_index_t use, ignore;

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

    old = NextMapEntry(info, type);
    if (!old)
        return false;

    if (new->level >= type->num_levels)
        type->num_levels = new->level + 1;
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
    struct xkb_kt_map_entry entry;
    xkb_mod_mask_t mask;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(info, type, "map entry");

    if (!ExprResolveVModMask(info->keymap, arrayNdx, &mask))
        return ReportTypeBadType(info, type, "map entry", "modifier mask");

    entry.mods.real_mods = mask & 0xff;
    entry.mods.vmods = (mask >> XkbNumModifiers) & 0xffff;

    if ((entry.mods.real_mods & (~type->mask)) ||
        (entry.mods.vmods & (~type->vmask))) {
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

    if (!ExprResolveLevel(info->keymap->ctx, value, &entry.level)) {
        log_err(info->keymap->ctx,
                "Level specifications in a key type must be integer; "
                "Ignoring malformed level specification\n");
        return false;
    }

    return AddMapEntry(info, type, &entry, true, true);
}

static bool
SetPreserve(KeyTypesInfo *info, KeyTypeInfo *type, ExprDef *arrayNdx,
            ExprDef *value)
{
    xkb_mod_mask_t mask;
    PreserveInfo new;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(info, type, "preserve entry");

    if (!ExprResolveVModMask(info->keymap, arrayNdx, &mask))
        return ReportTypeBadType(info, type, "preserve entry",
                                 "modifier mask");

    new.indexMods = mask & 0xff;
    new.indexVMods = (mask >> XkbNumModifiers) & 0xffff;

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

    if (!ExprResolveVModMask(info->keymap, value, &mask)) {
        log_err(info->keymap->ctx,
                "Preserve value in a key type is not a modifier mask; "
                "Ignoring preserve[%s] in type %s\n",
                PreserveIndexTxt(info, &new), TypeTxt(info, type));
        return false;
    }

    new.preMods = mask & 0xff;
    new.preVMods = (mask >> XkbNumModifiers) & 0xffff;

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
             xkb_level_index_t level, xkb_atom_t name, bool clobber)
{
    /* New name. */
    if (level >= darray_size(type->level_names)) {
        darray_resize0(type->level_names, level + 1);
        goto finish;
    }

    /* Same level, same name. */
    if (darray_item(type->level_names, level) == name) {
        log_lvl(info->keymap->ctx, 10,
                "Duplicate names for level %d of key type %s; Ignored\n",
                level + 1, TypeTxt(info, type));
        return true;
    }

    /* Same level, different name. */
    if (darray_item(type->level_names, level) != XKB_ATOM_NONE) {
        const char *old, *new;
        old = xkb_atom_text(info->keymap->ctx,
                            darray_item(type->level_names, level));
        new = xkb_atom_text(info->keymap->ctx, name);
        log_lvl(info->keymap->ctx, 1,
                "Multiple names for level %d of key type %s; "
                "Using %s, ignoring %s\n",
                level + 1, TypeTxt(info, type),
                (clobber ? new : old), (clobber ? old : new));

        if (!clobber)
            return true;
    }

    /* XXX: What about different level, same name? */

finish:
    darray_item(type->level_names, level) = name;
    return true;
}

static bool
SetLevelName(KeyTypesInfo *info, KeyTypeInfo *type, ExprDef *arrayNdx,
             ExprDef *value)
{
    xkb_level_index_t level;
    xkb_atom_t level_name;
    struct xkb_context *ctx = info->keymap->ctx;
    const char *str;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(info, type, "level name");

    if (!ExprResolveLevel(ctx, arrayNdx, &level))
        return ReportTypeBadType(info, type, "level name", "integer");

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
    bool ok = false;
    enum type_field type_field = 0;

    if (istreq(field, "modifiers")) {
        type_field = TYPE_FIELD_MASK;
        ok = SetModifiers(info, type, arrayNdx, value);
    }
    else if (istreq(field, "map")) {
        type_field = TYPE_FIELD_MAP;
        ok = SetMapEntry(info, type, arrayNdx, value);
    }
    else if (istreq(field, "preserve")) {
        type_field = TYPE_FIELD_PRESERVE;
        ok = SetPreserve(info, type, arrayNdx, value);
    }
    else if (istreq(field, "levelname") || istreq(field, "level_name")) {
        type_field = TYPE_FIELD_LEVEL_NAME;
        ok = SetLevelName(info, type, arrayNdx, value);
    } else {
        log_err(info->keymap->ctx,
                "Unknown field %s in key type %s; Definition ignored\n",
                field, TypeTxt(info, type));
    }

    type->defined |= type_field;
    return ok;
}

static bool
HandleKeyTypeBody(KeyTypesInfo *info, VarDef *def, KeyTypeInfo *type)
{
    bool ok = true;
    const char *elem, *field;
    ExprDef *arrayNdx;

    for (; def; def = (VarDef *) def->common.next) {
        ok = ExprResolveLhs(info->keymap->ctx, def->name, &elem, &field,
                            &arrayNdx);
        if (!ok)
            continue;

        if (elem && istreq(elem, "type")) {
            log_err(info->keymap->ctx,
                    "Support for changing the default type has been removed; "
                    "Statement ignored\n");
            continue;
        }

        ok = SetKeyTypeField(info, type, field, arrayNdx, def->value);
    }

    return ok;
}

/**
 * Process a type "XYZ" { } specification in the xkb_types section.
 *
 */
static bool
HandleKeyTypeDef(KeyTypesInfo *info, KeyTypeDef *def, enum merge_mode merge)
{
    KeyTypeInfo type = {
        .defined = 0,
        .file_id = info->file_id,
        .merge = (def->merge == MERGE_DEFAULT ? merge : def->merge),
        .name = def->name,
        .mask = 0, .vmask = 0,
        .num_levels = 1,
        .entries = darray_new(),
        .level_names = darray_new(),
    };
    list_init(&type.preserves);

    /* Parse the actual content. */
    if (!HandleKeyTypeBody(info, def->body, &type)) {
        info->errorCount++;
        return false;
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
    bool ok;
    ParseCommon *stmt;

    free(info->name);
    info->name = strdup_safe(file->name);

    for (stmt = file->defs; stmt; stmt = stmt->next) {
        switch (stmt->type) {
        case STMT_INCLUDE:
            ok = HandleIncludeKeyTypes(info, (IncludeStmt *) stmt);
            break;
        case STMT_TYPE: /* e.g. type "ONE_LEVEL" */
            ok = HandleKeyTypeDef(info, (KeyTypeDef *) stmt, merge);
            break;
        case STMT_VAR:
            log_err(info->keymap->ctx,
                    "Support for changing the default type has been removed; "
                    "Statement ignored\n");
            ok = true;
            break;
        case STMT_VMOD: /* virtual_modifiers NumLock, ... */
            ok = HandleVModDef((VModDef *) stmt, info->keymap, merge,
                               &info->vmods);
            break;
        default:
            log_err(info->keymap->ctx,
                    "Key type files may not include other declarations; "
                    "Ignoring %s\n", StmtTypeToString(stmt->type));
            ok = false;
            break;
        }

        if (!ok)
            info->errorCount++;

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
    unsigned int i;
    xkb_mod_mask_t tmp;
    struct xkb_kt_map_entry *entry = NULL;

    if (type->mods.vmods != 0) {
        tmp = VModsToReal(keymap, type->mods.vmods);
        type->mods.mask = tmp | type->mods.real_mods;

        for (i = 0; i < type->num_entries; i++) {
            entry = &type->map[i];
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
    else {
        type->mods.mask = type->mods.real_mods;
    }

    return true;
}

static bool
CopyDefToKeyType(KeyTypesInfo *info, KeyTypeInfo *def,
                 struct xkb_key_type *type)
{
    PreserveInfo *pre;
    struct xkb_keymap *keymap = info->keymap;

    list_foreach(pre, &def->preserves, entry) {
        struct xkb_kt_map_entry *match;
        struct xkb_kt_map_entry tmp;

        tmp.mods.real_mods = pre->indexMods;
        tmp.mods.vmods = pre->indexVMods;
        tmp.level = 0;
        (void) AddMapEntry(info, def, &tmp, false, false);

        match = FindMatchingMapEntry(def, pre->indexMods, pre->indexVMods);
        if (!match) {
            log_wsgo(info->keymap->ctx,
                     "Couldn't find matching entry for preserve; Aborting\n");
            return false;
        }

        match->preserve.mask = pre->preMods;
        match->preserve.real_mods = pre->preMods;
        match->preserve.vmods = pre->preVMods;
    }

    type->mods.real_mods = def->mask;
    type->mods.vmods = def->vmask;
    type->num_levels = def->num_levels;
    type->map = darray_mem(def->entries, 0);
    type->num_entries = darray_size(def->entries);
    darray_init(def->entries);
    type->name = def->name;
    type->level_names = darray_mem(def->level_names, 0);
    darray_init(def->level_names);

    return ComputeEffectiveMap(keymap, type);
}

bool
CompileKeyTypes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge)
{
    unsigned int i;
    unsigned int num_types;
    KeyTypesInfo info;
    KeyTypeInfo *def;

    InitKeyTypesInfo(&info, keymap, file->id);

    HandleKeyTypesFile(&info, file, merge);

    if (info.errorCount != 0)
        goto err_info;

    if (info.name)
        keymap->types_section_name = strdup(info.name);

    num_types = info.num_types ? info.num_types : 1;
    keymap->types = calloc(num_types, sizeof(*keymap->types));
    if (!keymap->types)
        goto err_info;
    keymap->num_types = num_types;

    /*
     * If no types were specified, a default unnamed one-level type is
     * used for all keys.
     */
    if (info.num_types == 0) {
        KeyTypeInfo dflt = {
            .name = xkb_atom_intern(keymap->ctx, "default"),
            .mask = 0, .vmask = 0,
            .num_levels = 1,
            .entries = darray_new(),
            .level_names = darray_new(),
        };
        list_init(&dflt.preserves);

        if (!CopyDefToKeyType(&info, &dflt, &keymap->types[0]))
            goto err_info;
    } else {
        i = 0;
        list_foreach(def, &info.types, entry)
            if (!CopyDefToKeyType(&info, def, &keymap->types[i++]))
                goto err_info;
    }

    FreeKeyTypesInfo(&info);
    return true;

err_info:
    FreeKeyTypesInfo(&info);
    return false;
}
