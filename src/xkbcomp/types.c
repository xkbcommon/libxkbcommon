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

#include "config.h"

#include "xkbcomp-priv.h"
#include "text.h"
#include "vmod.h"
#include "expr.h"
#include "include.h"
#include "util-mem.h"

enum type_field {
    TYPE_FIELD_MASK = (1 << 0),
    TYPE_FIELD_MAP = (1 << 1),
    TYPE_FIELD_PRESERVE = (1 << 2),
    TYPE_FIELD_LEVEL_NAME = (1 << 3),
};

typedef struct {
    enum type_field defined;
    enum merge_mode merge;

    xkb_atom_t name;
    xkb_mod_mask_t mods;
    xkb_level_index_t num_levels;
    darray(struct xkb_key_type_entry) entries;
    darray(xkb_atom_t) level_names;
} KeyTypeInfo;

typedef struct {
    char *name;
    int errorCount;
    unsigned int include_depth;

    darray(KeyTypeInfo) types;
    struct xkb_mod_set mods;

    struct xkb_context *ctx;
} KeyTypesInfo;

/***====================================================================***/

static inline const char *
MapEntryTxt(KeyTypesInfo *info, struct xkb_key_type_entry *entry)
{
    return ModMaskText(info->ctx, &info->mods, entry->mods.mods);
}

static inline const char *
TypeTxt(KeyTypesInfo *info, KeyTypeInfo *type)
{
    return xkb_atom_text(info->ctx, type->name);
}

static inline const char *
TypeMaskTxt(KeyTypesInfo *info, KeyTypeInfo *type)
{
    return ModMaskText(info->ctx, &info->mods, type->mods);
}

static inline bool
ReportTypeShouldBeArray(KeyTypesInfo *info, KeyTypeInfo *type,
                        const char *field)
{
    return ReportShouldBeArray(info->ctx, "key type", field,
                               TypeTxt(info, type));
}

static inline bool
ReportTypeBadType(KeyTypesInfo *info, xkb_message_code_t code,
                  KeyTypeInfo *type, const char *field, const char *wanted)
{
    return ReportBadType(info->ctx, code, "key type", field,
                         TypeTxt(info, type), wanted);
}

/***====================================================================***/

static void
InitKeyTypesInfo(KeyTypesInfo *info, struct xkb_context *ctx,
                 unsigned int include_depth,
                 const struct xkb_mod_set *mods)
{
    memset(info, 0, sizeof(*info));
    info->ctx = ctx;
    info->include_depth = include_depth;
    info->mods = *mods;
}

static void
ClearKeyTypeInfo(KeyTypeInfo *type)
{
    darray_free(type->entries);
    darray_free(type->level_names);
}

static void
ClearKeyTypesInfo(KeyTypesInfo *info)
{
    free(info->name);
    KeyTypeInfo *type;
    darray_foreach(type, info->types)
        ClearKeyTypeInfo(type);
    darray_free(info->types);
}

static KeyTypeInfo *
FindMatchingKeyType(KeyTypesInfo *info, xkb_atom_t name)
{
    KeyTypeInfo *old;

    darray_foreach(old, info->types)
        if (old->name == name)
            return old;

    return NULL;
}

static bool
AddKeyType(KeyTypesInfo *info, KeyTypeInfo *new, bool same_file)
{
    KeyTypeInfo *old;
    const int verbosity = xkb_context_get_log_verbosity(info->ctx);

    old = FindMatchingKeyType(info, new->name);
    if (old) {
        if (new->merge == MERGE_REPLACE || new->merge == MERGE_OVERRIDE) {
            if ((same_file && verbosity > 0) || verbosity > 9) {
                log_warn(info->ctx,
                         XKB_WARNING_CONFLICTING_KEY_TYPE_DEFINITIONS,
                         "Multiple definitions of the %s key type; "
                         "Earlier definition ignored\n",
                         xkb_atom_text(info->ctx, new->name));
            }

            ClearKeyTypeInfo(old);
            *old = *new;
            darray_init(new->entries);
            darray_init(new->level_names);
            return true;
        }

        if (same_file)
            log_vrb(info->ctx, 4,
                    XKB_WARNING_CONFLICTING_KEY_TYPE_DEFINITIONS,
                    "Multiple definitions of the %s key type; "
                    "Later definition ignored\n",
                    xkb_atom_text(info->ctx, new->name));

        ClearKeyTypeInfo(new);
        return true;
    }

    darray_append(info->types, *new);
    return true;
}

/***====================================================================***/

static void
MergeIncludedKeyTypes(KeyTypesInfo *into, KeyTypesInfo *from,
                      enum merge_mode merge)
{
    if (from->errorCount > 0) {
        into->errorCount += from->errorCount;
        return;
    }

    into->mods = from->mods;

    if (into->name == NULL) {
        into->name = steal(&from->name);
    }

    if (darray_empty(into->types)) {
        into->types = from->types;
        /* Types stolen via shallow copy, so reinitialize the array */
        darray_init(from->types);
    }
    else {
        KeyTypeInfo *type;
        darray_foreach(type, from->types) {
            type->merge = (merge == MERGE_DEFAULT ? type->merge : merge);
            if (!AddKeyType(into, type, false))
                into->errorCount++;
        }
        /* Types were either shallow copied or reinitialized individually
           in `AddKeyType`, so we only need to free the array */
        darray_free(from->types);
    }
}

static void
HandleKeyTypesFile(KeyTypesInfo *info, XkbFile *file, enum merge_mode merge);

static bool
HandleIncludeKeyTypes(KeyTypesInfo *info, IncludeStmt *include)
{
    KeyTypesInfo included;

    if (ExceedsIncludeMaxDepth(info->ctx, info->include_depth)) {
        info->errorCount += 10;
        return false;
    }

    InitKeyTypesInfo(&included, info->ctx, 0 /* unused */, &info->mods);
    included.name = steal(&include->stmt);

    for (IncludeStmt *stmt = include; stmt; stmt = stmt->next_incl) {
        KeyTypesInfo next_incl;
        XkbFile *file;

        file = ProcessIncludeFile(info->ctx, stmt, FILE_TYPE_TYPES);
        if (!file) {
            info->errorCount += 10;
            ClearKeyTypesInfo(&included);
            return false;
        }

        InitKeyTypesInfo(&next_incl, info->ctx, info->include_depth + 1,
                         &included.mods);

        HandleKeyTypesFile(&next_incl, file, stmt->merge);

        MergeIncludedKeyTypes(&included, &next_incl, stmt->merge);

        ClearKeyTypesInfo(&next_incl);
        FreeXkbFile(file);
    }

    MergeIncludedKeyTypes(info, &included, include->merge);
    ClearKeyTypesInfo(&included);

    return (info->errorCount == 0);
}

/***====================================================================***/

static bool
SetModifiers(KeyTypesInfo *info, KeyTypeInfo *type, ExprDef *arrayNdx,
             ExprDef *value)
{
    xkb_mod_mask_t mods;

    if (arrayNdx)
        log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                 "The modifiers field of a key type is not an array; "
                 "Illegal array subscript ignored\n");

    if (!ExprResolveModMask(info->ctx, value, MOD_BOTH, &info->mods, &mods)) {
        log_err(info->ctx,
                XKB_ERROR_UNSUPPORTED_MODIFIER_MASK,
                "Key type mask field must be a modifier mask; "
                "Key type definition ignored\n");
        return false;
    }

    if (type->defined & TYPE_FIELD_MASK) {
        log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                 "Multiple modifier mask definitions for key type %s; "
                 "Using %s, ignoring %s\n",
                 xkb_atom_text(info->ctx, type->name),
                 TypeMaskTxt(info, type),
                 ModMaskText(info->ctx, &info->mods, mods));
        return false;
    }

    type->mods = mods;
    return true;
}

/***====================================================================***/

static struct xkb_key_type_entry *
FindMatchingMapEntry(KeyTypeInfo *type, xkb_mod_mask_t mods)
{
    struct xkb_key_type_entry *entry;

    darray_foreach(entry, type->entries)
        if (entry->mods.mods == mods)
            return entry;

    return NULL;
}

static bool
AddMapEntry(KeyTypesInfo *info, KeyTypeInfo *type,
            struct xkb_key_type_entry *new, bool clobber, bool report)
{
    struct xkb_key_type_entry *old;

    old = FindMatchingMapEntry(type, new->mods.mods);
    if (old) {
        if (report && old->level != new->level) {
            log_warn(info->ctx,
                     XKB_WARNING_CONFLICTING_KEY_TYPE_MAP_ENTRY,
                     "Multiple map entries for %s in %s; "
                     "Using %d, ignoring %d\n",
                     MapEntryTxt(info, new), TypeTxt(info, type),
                     (clobber ? new->level : old->level) + 1,
                     (clobber ? old->level : new->level) + 1);
        }
        else {
            log_vrb(info->ctx, 10,
                    XKB_WARNING_CONFLICTING_KEY_TYPE_MAP_ENTRY,
                    "Multiple occurrences of map[%s]= %d in %s; Ignored\n",
                    MapEntryTxt(info, new), new->level + 1,
                    TypeTxt(info, type));
            return true;
        }

        if (clobber) {
            if (new->level >= type->num_levels)
                type->num_levels = new->level + 1;
            old->level = new->level;
        }

        return true;
    }

    if (new->level >= type->num_levels)
        type->num_levels = new->level + 1;

    darray_append(type->entries, *new);
    return true;
}

static bool
SetMapEntry(KeyTypesInfo *info, KeyTypeInfo *type, ExprDef *arrayNdx,
            ExprDef *value)
{
    struct xkb_key_type_entry entry;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(info, type, "map entry");

    if (!ExprResolveModMask(info->ctx, arrayNdx, MOD_BOTH, &info->mods,
                            &entry.mods.mods))
        return ReportTypeBadType(info, XKB_ERROR_UNSUPPORTED_MODIFIER_MASK,
                                 type, "map entry", "modifier mask");

    if (entry.mods.mods & (~type->mods)) {
        log_vrb(info->ctx, 1,
                XKB_WARNING_UNDECLARED_MODIFIERS_IN_KEY_TYPE,
                "Map entry for modifiers not used by type %s; "
                "Using %s instead of %s\n",
                TypeTxt(info, type),
                ModMaskText(info->ctx, &info->mods,
                            entry.mods.mods & type->mods),
                MapEntryTxt(info, &entry));
        entry.mods.mods &= type->mods;
    }

    if (!ExprResolveLevel(info->ctx, value, &entry.level)) {
        log_err(info->ctx, XKB_ERROR_UNSUPPORTED_SHIFT_LEVEL,
                          "Level specifications in a key type must be integer; "
                          "Ignoring malformed level specification\n");
        return false;
    }

    entry.preserve.mods = 0;

    return AddMapEntry(info, type, &entry, true, true);
}

/***====================================================================***/

static bool
AddPreserve(KeyTypesInfo *info, KeyTypeInfo *type,
            xkb_mod_mask_t mods, xkb_mod_mask_t preserve_mods)
{
    struct xkb_key_type_entry *entry;
    struct xkb_key_type_entry new;

    darray_foreach(entry, type->entries) {
        if (entry->mods.mods != mods)
            continue;

        /* Map exists without previous preserve (or "None"); override. */
        if (entry->preserve.mods == 0) {
            entry->preserve.mods = preserve_mods;
            return true;
        }

        /* Map exists with same preserve; do nothing. */
        if (entry->preserve.mods == preserve_mods) {
            log_vrb(info->ctx, 10,
                    XKB_WARNING_DUPLICATE_ENTRY,
                    "Identical definitions for preserve[%s] in %s; "
                    "Ignored\n",
                    ModMaskText(info->ctx, &info->mods, mods),
                    TypeTxt(info, type));
            return true;
        }

        /* Map exists with different preserve; latter wins. */
        log_vrb(info->ctx, 1,
                XKB_WARNING_CONFLICTING_KEY_TYPE_PRESERVE_ENTRIES,
                "Multiple definitions for preserve[%s] in %s; "
                "Using %s, ignoring %s\n",
                ModMaskText(info->ctx, &info->mods, mods),
                TypeTxt(info, type),
                ModMaskText(info->ctx, &info->mods, preserve_mods),
                ModMaskText(info->ctx, &info->mods, entry->preserve.mods));

        entry->preserve.mods = preserve_mods;
        return true;
    }

    /*
     * Map does not exist, i.e. preserve[] came before map[].
     * Create a map with the specified mask mapping to Level1. The level
     * may be overridden later with an explicit map[] statement.
     */
    new.level = 0;
    new.mods.mods = mods;
    new.preserve.mods = preserve_mods;
    darray_append(type->entries, new);
    return true;
}

static bool
SetPreserve(KeyTypesInfo *info, KeyTypeInfo *type, ExprDef *arrayNdx,
            ExprDef *value)
{
    xkb_mod_mask_t mods, preserve_mods;

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(info, type, "preserve entry");

    if (!ExprResolveModMask(info->ctx, arrayNdx, MOD_BOTH, &info->mods, &mods))
        return ReportTypeBadType(info, XKB_ERROR_UNSUPPORTED_MODIFIER_MASK,
                                 type, "preserve entry", "modifier mask");

    if (mods & ~type->mods) {
        const char *before, *after;

        before = ModMaskText(info->ctx, &info->mods, mods);
        mods &= type->mods;
        after = ModMaskText(info->ctx, &info->mods, mods);

        log_vrb(info->ctx, 1,
                XKB_WARNING_UNDECLARED_MODIFIERS_IN_KEY_TYPE,
                "Preserve entry for modifiers not used by the %s type; "
                "Index %s converted to %s\n",
                TypeTxt(info, type), before, after);
    }

    if (!ExprResolveModMask(info->ctx, value, MOD_BOTH, &info->mods,
                            &preserve_mods)) {
        log_err(info->ctx,
                XKB_ERROR_UNSUPPORTED_MODIFIER_MASK,
                "Preserve value in a key type is not a modifier mask; "
                "Ignoring preserve[%s] in type %s\n",
                ModMaskText(info->ctx, &info->mods, mods),
                TypeTxt(info, type));
        return false;
    }

    if (preserve_mods & ~mods) {
        const char *before, *after;

        before = ModMaskText(info->ctx, &info->mods, preserve_mods);
        preserve_mods &= mods;
        after = ModMaskText(info->ctx, &info->mods, preserve_mods);

        log_vrb(info->ctx, 1,
                XKB_WARNING_ILLEGAL_KEY_TYPE_PRESERVE_RESULT,
                "Illegal value for preserve[%s] in type %s; "
                "Converted %s to %s\n",
                ModMaskText(info->ctx, &info->mods, mods),
                TypeTxt(info, type), before, after);
    }

    return AddPreserve(info, type, mods, preserve_mods);
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
        log_vrb(info->ctx, 10,
                XKB_WARNING_DUPLICATE_ENTRY,
                "Duplicate names for level %d of key type %s; Ignored\n",
                level + 1, TypeTxt(info, type));
        return true;
    }

    /* Same level, different name. */
    if (darray_item(type->level_names, level) != XKB_ATOM_NONE) {
        const char *old, *new;
        old = xkb_atom_text(info->ctx,
                            darray_item(type->level_names, level));
        new = xkb_atom_text(info->ctx, name);
        log_vrb(info->ctx, 1,
                XKB_WARNING_CONFLICTING_KEY_TYPE_LEVEL_NAMES,
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

    if (arrayNdx == NULL)
        return ReportTypeShouldBeArray(info, type, "level name");

    if (!ExprResolveLevel(info->ctx, arrayNdx, &level))
        return ReportTypeBadType(info, XKB_ERROR_UNSUPPORTED_SHIFT_LEVEL,
                                 type, "level name", "integer");

    if (!ExprResolveString(info->ctx, value, &level_name)) {
        log_err(info->ctx,
                XKB_ERROR_WRONG_FIELD_TYPE,
                "Non-string name for level %d in key type %s; "
                "Ignoring illegal level name definition\n",
                level + 1, xkb_atom_text(info->ctx, type->name));
        return false;
    }

    return AddLevelName(info, type, level, level_name, true);
}

/***====================================================================***/

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
        log_err(info->ctx,
                XKB_ERROR_UNKNOWN_FIELD,
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
        ok = ExprResolveLhs(info->ctx, def->name, &elem, &field,
                            &arrayNdx);
        if (!ok)
            continue;

        if (elem) {
            if (istreq(elem, "type")) {
                log_err(info->ctx,
                        XKB_ERROR_INVALID_SET_DEFAULT_STATEMENT,
                        "Support for changing the default type has been removed; "
                        "Statement \"%s.%s\" ignored.\n", elem, field);
            }
            else {
                log_err(info->ctx, XKB_ERROR_GLOBAL_DEFAULTS_WRONG_SCOPE,
                        "Cannot set global defaults for \"%s\" element within "
                        "a key type statement: move statements to the global "
                        "file scope. Assignment to \"%s.%s\" ignored.\n",
                        elem, elem, field);
                ok = false;
            }
            continue;
        }

        ok = SetKeyTypeField(info, type, field, arrayNdx, def->value);
    }

    return ok;
}

static bool
HandleKeyTypeDef(KeyTypesInfo *info, KeyTypeDef *def, enum merge_mode merge)
{
    KeyTypeInfo type = {
        .defined = 0,
        .merge = (def->merge == MERGE_DEFAULT ? merge : def->merge),
        .name = def->name,
        .mods = 0,
        .num_levels = 1,
        .entries = darray_new(),
        .level_names = darray_new(),
    };

    if (!HandleKeyTypeBody(info, def->body, &type) ||
        !AddKeyType(info, &type, true))
    {
        info->errorCount++;
        ClearKeyTypeInfo(&type);
        return false;
    }

    /* Type has been either stolen via shallow copy or reinitialized in
       `AddKeyType`: no need to free the arrays */
    return true;
}

static void
HandleKeyTypesFile(KeyTypesInfo *info, XkbFile *file, enum merge_mode merge)
{
    bool ok;

    free(info->name);
    info->name = strdup_safe(file->name);

    for (ParseCommon *stmt = file->defs; stmt; stmt = stmt->next) {
        switch (stmt->type) {
        case STMT_INCLUDE:
            ok = HandleIncludeKeyTypes(info, (IncludeStmt *) stmt);
            break;
        case STMT_TYPE:
            ok = HandleKeyTypeDef(info, (KeyTypeDef *) stmt, merge);
            break;
        case STMT_VAR:
            log_err(info->ctx,
                    XKB_ERROR_WRONG_STATEMENT_TYPE,
                    "Support for changing the default type has been removed; "
                    "Statement ignored\n");
            ok = true;
            break;
        case STMT_VMOD:
            ok = HandleVModDef(info->ctx, &info->mods, (VModDef *) stmt, merge);
            break;
        default:
            log_err(info->ctx,
                    XKB_ERROR_WRONG_STATEMENT_TYPE,
                    "Key type files may not include other declarations; "
                    "Ignoring %s\n", stmt_type_to_string(stmt->type));
            ok = false;
            break;
        }

        if (!ok)
            info->errorCount++;

        if (info->errorCount > 10) {
            log_err(info->ctx,
                    XKB_ERROR_INVALID_SYNTAX,
                    "Abandoning keytypes file \"%s\"\n", file->name);
            break;
        }
    }
}

/***====================================================================***/

static bool
CopyKeyTypesToKeymap(struct xkb_keymap *keymap, KeyTypesInfo *info)
{
    unsigned num_types;
    struct xkb_key_type *types;

    num_types = darray_empty(info->types) ? 1 : darray_size(info->types);
    types = calloc(num_types, sizeof(*types));
    if (!types)
        return false;

    /*
     * If no types were specified, a default unnamed one-level type is
     * used for all keys.
     */
    if (darray_empty(info->types)) {
        struct xkb_key_type *type = &types[0];

        type->mods.mods = 0;
        type->num_levels = 1;
        type->entries = NULL;
        type->num_entries = 0;
        type->name = xkb_atom_intern_literal(keymap->ctx, "default");
        type->level_names = NULL;
        type->num_level_names = 0;
    }
    else {
        for (unsigned i = 0; i < num_types; i++) {
            KeyTypeInfo *def = &darray_item(info->types, i);
            struct xkb_key_type *type = &types[i];

            type->name = def->name;
            type->mods.mods = def->mods;
            type->num_levels = def->num_levels;
            darray_steal(def->level_names, &type->level_names, &type->num_level_names);
            darray_steal(def->entries, &type->entries, &type->num_entries);
        }
    }

    keymap->types_section_name = strdup_safe(info->name);
    XkbEscapeMapName(keymap->types_section_name);
    keymap->num_types = num_types;
    keymap->types = types;
    keymap->mods = info->mods;
    return true;
}

/***====================================================================***/

bool
CompileKeyTypes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge)
{
    KeyTypesInfo info;

    InitKeyTypesInfo(&info, keymap->ctx, 0, &keymap->mods);

    HandleKeyTypesFile(&info, file, merge);
    if (info.errorCount != 0)
        goto err_info;

    if (!CopyKeyTypesToKeymap(keymap, &info))
        goto err_info;

    ClearKeyTypesInfo(&info);
    return true;

err_info:
    ClearKeyTypesInfo(&info);
    return false;
}
