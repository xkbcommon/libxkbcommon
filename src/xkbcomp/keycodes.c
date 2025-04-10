/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */

#include "config.h"

#include <assert.h>
#include <stdint.h>

#include "darray.h"
#include "xkbcommon/xkbcommon.h"
#include "xkbcomp-priv.h"
#include "text.h"
#include "expr.h"
#include "include.h"
#include "util-mem.h"

typedef struct {
    enum merge_mode merge;

    xkb_atom_t alias;
    xkb_atom_t real;
} AliasInfo;

typedef struct {
    enum merge_mode merge;

    xkb_atom_t name;
} LedNameInfo;

typedef struct {
    char *name;
    int errorCount;
    unsigned int include_depth;

    xkb_keycode_t min_key_code;
    xkb_keycode_t max_key_code;
    darray(xkb_atom_t) key_names;
    LedNameInfo led_names[XKB_MAX_LEDS];
    unsigned int num_led_names;
    darray(AliasInfo) aliases;

    struct xkb_context *ctx;
} KeyNamesInfo;

/***====================================================================***/

static void
InitAliasInfo(AliasInfo *info, enum merge_mode merge,
              xkb_atom_t alias, xkb_atom_t real)
{
    memset(info, 0, sizeof(*info));
    info->merge = merge;
    info->alias = alias;
    info->real = real;
}

static LedNameInfo *
FindLedByName(KeyNamesInfo *info, xkb_atom_t name,
              xkb_led_index_t *idx_out)
{
    for (xkb_led_index_t idx = 0; idx < info->num_led_names; idx++) {
        LedNameInfo *ledi = &info->led_names[idx];
        if (ledi->name == name) {
            *idx_out = idx;
            return ledi;
        }
    }

    return NULL;
}

static bool
AddLedName(KeyNamesInfo *info, enum merge_mode merge, bool same_file,
           LedNameInfo *new, xkb_led_index_t new_idx)
{
    xkb_led_index_t old_idx;
    LedNameInfo *old;
    const int verbosity = xkb_context_get_log_verbosity(info->ctx);
    const bool report = (same_file && verbosity > 0) || verbosity > 9;
    const bool replace = (merge == MERGE_REPLACE || merge == MERGE_OVERRIDE);

    /* LED with the same name already exists. */
    old = FindLedByName(info, new->name, &old_idx);
    if (old) {
        if (old_idx == new_idx) {
            log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                     "Multiple indicators named \"%s\"; "
                     "Identical definitions ignored\n",
                     xkb_atom_text(info->ctx, new->name));
            return true;
        }

        if (report) {
            xkb_led_index_t use = (replace ? new_idx + 1 : old_idx + 1);
            xkb_led_index_t ignore = (replace ? old_idx + 1 : new_idx + 1);
            log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                     "Multiple indicators named %s; Using %d, ignoring %d\n",
                     xkb_atom_text(info->ctx, new->name), use, ignore);
        }

        if (replace)
            *old = *new;

        return true;
    }

    if (new_idx >= info->num_led_names)
        info->num_led_names = new_idx + 1;

    /* LED with the same index already exists. */
    old = &info->led_names[new_idx];
    if (old->name != XKB_ATOM_NONE) {
        if (report) {
            const xkb_atom_t use = (replace ? new->name : old->name);
            const xkb_atom_t ignore = (replace ? old->name : new->name);
            log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                     "Multiple names for indicator %d; "
                     "Using %s, ignoring %s\n", new_idx + 1,
                     xkb_atom_text(info->ctx, use),
                     xkb_atom_text(info->ctx, ignore));
        }

        if (replace)
            *old = *new;

        return true;
    }

    *old = *new;
    return true;
}

static void
ClearKeyNamesInfo(KeyNamesInfo *info)
{
    free(info->name);
    darray_free(info->key_names);
    darray_free(info->aliases);
}

static void
InitKeyNamesInfo(KeyNamesInfo *info, struct xkb_context *ctx,
                 unsigned int include_depth)
{
    memset(info, 0, sizeof(*info));
    info->ctx = ctx;
    info->include_depth = include_depth;
    info->min_key_code = XKB_KEYCODE_INVALID;
#if XKB_KEYCODE_INVALID < XKB_KEYCODE_MAX
#error "Hey, you can't be changing stuff like that."
#endif
}

static xkb_keycode_t
FindKeyByName(KeyNamesInfo *info, xkb_atom_t name)
{
    xkb_keycode_t i;

    for (i = info->min_key_code; i <= info->max_key_code; i++)
        if (darray_item(info->key_names, i) == name)
            return i;

    return XKB_KEYCODE_INVALID;
}

static void
ShrinkKeycodeBounds(KeyNamesInfo *info)
{
    static_assert(XKB_KEYCODE_MAX < UINT32_MAX, "Overflow can occur");
    if (unlikely(info->min_key_code > XKB_KEYCODE_MAX)) {
        /* No keycode defined */
        assert(info->min_key_code == XKB_KEYCODE_INVALID);
        return;
    }

    /* Update lower bound */
    for (xkb_keycode_t low = info->min_key_code;
         low <= info->max_key_code;
         low++) {
        if (darray_item(info->key_names, low) != XKB_ATOM_NONE) {
            info->min_key_code = low;
            break;
        }
    }

    /* Update upper bound */
    for (xkb_keycode_t high = info->max_key_code + 1;
         high-- > info->min_key_code;) {
        if (darray_item(info->key_names, high) != XKB_ATOM_NONE) {
            info->max_key_code = high;
            break;
        }
    }
}

static bool
AddKeyName(KeyNamesInfo *info, xkb_keycode_t kc, xkb_atom_t name,
           enum merge_mode merge, bool same_file, bool report)
{
    const int verbosity = xkb_context_get_log_verbosity(info->ctx);

    report = report && ((same_file && verbosity > 0) || verbosity > 7);

    /* Check keycode is not too huge for our continuous array */
    if (kc > XKB_KEYCODE_MAX_IMPL) {
        log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                "Keycode too big: must be < %#"PRIx32", got %#"PRIx32"; "
                "Key ignored\n", XKB_KEYCODE_MAX_IMPL, kc);
        return false;
    }

    const xkb_atom_t old_name = (kc >= darray_size(info->key_names))
        ? XKB_ATOM_NONE
        : darray_item(info->key_names, kc);
    const xkb_keycode_t old_kc = FindKeyByName(info, name);
    bool shrink = false;

    if (old_kc != XKB_KEYCODE_INVALID && old_kc != kc) {
        /* There is already a different key with this name. */
        if (merge == MERGE_AUGMENT) {
            if (report)
                log_vrb(info->ctx, 3, XKB_WARNING_CONFLICTING_KEY_NAME,
                        "Key name %s assigned to multiple keys; "
                        "Using %"PRIu32", ignoring %"PRIu32"\n",
                        KeyNameText(info->ctx, name), old_kc, kc);
            return true;
        } else {
            /* Remove conflicting key name mapping */
            darray_item(info->key_names, old_kc) = XKB_ATOM_NONE;
            /* Detect bounds change here but correct bounds later, to ensure
             * there is at least one keycode */
            shrink = (old_kc == info->min_key_code ||
                      old_kc == info->max_key_code);

            if (report)
                log_warn(info->ctx, XKB_WARNING_CONFLICTING_KEY_NAME,
                         "Key name %s assigned to multiple keys; "
                         "Using %"PRIu32", ignoring %"PRIu32"\n",
                         KeyNameText(info->ctx, name), kc, old_kc);
        }
    }

    if (old_name != XKB_ATOM_NONE) {
        /* There is already a key with this keycode. */

        /* No need to update bounds */
        assert(kc < darray_size(info->key_names));
        assert(kc >= info->min_key_code);
        assert(kc <= info->max_key_code);

        if (old_name == name) {
            assert (old_kc == kc);
            if (report)
                log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                         "Multiple identical key name definitions; "
                         "Later occurrences of \"%s = %"PRIu32"\" ignored\n",
                         KeyNameText(info->ctx, old_name), kc);
            return true;
        }
        else if (merge == MERGE_AUGMENT) {
            if (report)
                log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                         "Multiple names for keycode %"PRIu32"; "
                         "Using %s, ignoring %s\n", kc,
                         KeyNameText(info->ctx, old_name),
                         KeyNameText(info->ctx, name));
            return true;
        }
        else {
            if (report)
                log_warn(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                         "Multiple names for keycode %"PRIu32"; "
                         "Using %s, ignoring %s\n", kc,
                         KeyNameText(info->ctx, name),
                         KeyNameText(info->ctx, old_name));
        }
    } else {
        /* No previous keycode, resize if relevant */
        if (kc >= darray_size(info->key_names))
            darray_resize0(info->key_names, kc + 1);

        /* Update the keycode bounds */
        info->min_key_code = MIN(info->min_key_code, kc);
        info->max_key_code = MAX(info->max_key_code, kc);
    }

    darray_item(info->key_names, kc) = name;

    /* Shrink the bounds if a relevant keycode has been removed. */
    if (shrink)
        ShrinkKeycodeBounds(info);

    return true;
}

/***====================================================================***/

static bool
HandleAliasDef(KeyNamesInfo *info, KeyAliasDef *def, enum merge_mode merge);

static void
MergeIncludedKeycodes(KeyNamesInfo *into, KeyNamesInfo *from,
                      enum merge_mode merge)
{
    if (from->errorCount > 0) {
        into->errorCount += from->errorCount;
        return;
    }

    if (into->name == NULL) {
        into->name = steal(&from->name);
    }

    /* Merge key names. */
    if (darray_empty(into->key_names)) {
        into->key_names = from->key_names;
        darray_init(from->key_names);
        into->min_key_code = from->min_key_code;
        into->max_key_code = from->max_key_code;
    }
    else {
        if (darray_size(into->key_names) < darray_size(from->key_names))
            darray_resize0(into->key_names, darray_size(from->key_names));

        for (xkb_keycode_t i = from->min_key_code; i <= from->max_key_code; i++) {
            xkb_atom_t name = darray_item(from->key_names, i);
            if (name == XKB_ATOM_NONE)
                continue;

            if (!AddKeyName(into, i, name, merge, true, false))
                into->errorCount++;
        }
    }

    /* Merge key aliases. */
    if (darray_empty(into->aliases)) {
        into->aliases = from->aliases;
        darray_init(from->aliases);
    }
    else {
        AliasInfo *alias;

        darray_foreach(alias, from->aliases) {
            KeyAliasDef def;

            def.merge = (merge == MERGE_DEFAULT ? alias->merge : merge);
            def.alias = alias->alias;
            def.real = alias->real;

            if (!HandleAliasDef(into, &def, def.merge))
                into->errorCount++;
        }
    }

    /* Merge LED names. */
    if (into->num_led_names == 0) {
        memcpy(into->led_names, from->led_names,
               sizeof(*from->led_names) * from->num_led_names);
        into->num_led_names = from->num_led_names;
        from->num_led_names = 0;
    }
    else {
        for (xkb_led_index_t idx = 0; idx < from->num_led_names; idx++) {
            LedNameInfo *ledi = &from->led_names[idx];

            if (ledi->name == XKB_ATOM_NONE)
                continue;

            ledi->merge = (merge == MERGE_DEFAULT ? ledi->merge : merge);
            if (!AddLedName(into, ledi->merge, false, ledi, idx))
                into->errorCount++;
        }
    }
}

static void
HandleKeycodesFile(KeyNamesInfo *info, XkbFile *file, enum merge_mode merge);

static bool
HandleIncludeKeycodes(KeyNamesInfo *info, IncludeStmt *include)
{
    KeyNamesInfo included;

    if (ExceedsIncludeMaxDepth(info->ctx, info->include_depth)) {
        info->errorCount += 10;
        return false;
    }

    InitKeyNamesInfo(&included, info->ctx, 0 /* unused */);
    included.name = steal(&include->stmt);

    for (IncludeStmt *stmt = include; stmt; stmt = stmt->next_incl) {
        KeyNamesInfo next_incl;
        XkbFile *file;

        file = ProcessIncludeFile(info->ctx, stmt, FILE_TYPE_KEYCODES);
        if (!file) {
            info->errorCount += 10;
            ClearKeyNamesInfo(&included);
            return false;
        }

        InitKeyNamesInfo(&next_incl, info->ctx, info->include_depth + 1);

        HandleKeycodesFile(&next_incl, file, MERGE_OVERRIDE);

        MergeIncludedKeycodes(&included, &next_incl, stmt->merge);

        ClearKeyNamesInfo(&next_incl);
        FreeXkbFile(file);
    }

    MergeIncludedKeycodes(info, &included, include->merge);
    ClearKeyNamesInfo(&included);

    return (info->errorCount == 0);
}

static bool
HandleKeycodeDef(KeyNamesInfo *info, KeycodeDef *stmt, enum merge_mode merge)
{
    if (stmt->merge != MERGE_DEFAULT) {
        if (stmt->merge == MERGE_REPLACE)
            merge = MERGE_OVERRIDE;
        else
            merge = stmt->merge;
    }

    if (stmt->value < 0 || stmt->value > XKB_KEYCODE_MAX) {
        log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                "Illegal keycode %lld: must be between 0..%u; "
                "Key ignored\n", (long long) stmt->value, XKB_KEYCODE_MAX);
        return false;
    }

    return AddKeyName(info, (xkb_keycode_t) stmt->value,
                      stmt->name, merge, false, true);
}

static bool
HandleAliasDef(KeyNamesInfo *info, KeyAliasDef *def, enum merge_mode merge)
{
    AliasInfo *old, new;

    darray_foreach(old, info->aliases) {
        if (old->alias == def->alias) {
            if (def->real == old->real) {
                log_vrb(info->ctx, 1,
                        XKB_WARNING_CONFLICTING_KEY_NAME,
                        "Alias of %s for %s declared more than once; "
                        "First definition ignored\n",
                        KeyNameText(info->ctx, def->alias),
                        KeyNameText(info->ctx, def->real));
            }
            else {
                xkb_atom_t use, ignore;

                use = (merge == MERGE_AUGMENT ? old->real : def->real);
                ignore = (merge == MERGE_AUGMENT ? def->real : old->real);

                log_warn(info->ctx, XKB_WARNING_CONFLICTING_KEY_NAME,
                         "Multiple definitions for alias %s; "
                         "Using %s, ignoring %s\n",
                         KeyNameText(info->ctx, old->alias),
                         KeyNameText(info->ctx, use),
                         KeyNameText(info->ctx, ignore));

                old->real = use;
            }

            old->merge = merge;
            return true;
        }
    }

    InitAliasInfo(&new, merge, def->alias, def->real);
    darray_append(info->aliases, new);
    return true;
}

static bool
HandleKeyNameVar(KeyNamesInfo *info, VarDef *stmt)
{
    const char *elem, *field;
    ExprDef *arrayNdx;

    if (!ExprResolveLhs(info->ctx, stmt->name, &elem, &field, &arrayNdx))
        return false;

    if (elem) {
        log_err(info->ctx, XKB_ERROR_GLOBAL_DEFAULTS_WRONG_SCOPE,
                "Cannot set global defaults for \"%s\" element; "
                "Assignment to \"%s.%s\" ignored\n", elem, elem, field);
        return false;
    }

    if (!istreq(field, "minimum") && !istreq(field, "maximum")) {
        log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                "Unknown field encountered; "
                "Assignment to field \"%s\" ignored\n", field);
        return false;
    }

    /* We ignore explicit min/max statements, we always use computed. */
    return true;
}

static bool
HandleLedNameDef(KeyNamesInfo *info, LedNameDef *def,
                 enum merge_mode merge)
{
    if (def->ndx < 1 || def->ndx > XKB_MAX_LEDS) {
        info->errorCount++;
        log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                "Illegal indicator index (%"PRId64") specified; "
                "must be between 1 .. %u; Ignored\n", def->ndx, XKB_MAX_LEDS);
        return false;
    }

    xkb_atom_t name = XKB_ATOM_NONE;
    if (!ExprResolveString(info->ctx, def->name, &name)) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%"PRId64, def->ndx);
        info->errorCount++;
        return ReportBadType(info->ctx, XKB_ERROR_WRONG_FIELD_TYPE,
                             "indicator", "name", buf, "string");
    }

    LedNameInfo ledi = {.merge = merge, .name = name};
    return AddLedName(info, merge, true, &ledi, (xkb_led_index_t) def->ndx - 1);
}

static void
HandleKeycodesFile(KeyNamesInfo *info, XkbFile *file, enum merge_mode merge)
{
    bool ok;

    free(info->name);
    info->name = strdup_safe(file->name);

    for (ParseCommon *stmt = file->defs; stmt; stmt = stmt->next) {
        switch (stmt->type) {
        case STMT_INCLUDE:
            ok = HandleIncludeKeycodes(info, (IncludeStmt *) stmt);
            break;
        case STMT_KEYCODE:
            ok = HandleKeycodeDef(info, (KeycodeDef *) stmt, merge);
            break;
        case STMT_ALIAS:
            ok = HandleAliasDef(info, (KeyAliasDef *) stmt, merge);
            break;
        case STMT_VAR:
            ok = HandleKeyNameVar(info, (VarDef *) stmt);
            break;
        case STMT_LED_NAME:
            ok = HandleLedNameDef(info, (LedNameDef *) stmt, merge);
            break;
        default:
            log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Keycode files may define key and indicator names only; "
                    "Ignoring %s\n", stmt_type_to_string(stmt->type));
            ok = false;
            break;
        }

        if (!ok)
            info->errorCount++;

        if (info->errorCount > 10) {
            log_err(info->ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Abandoning keycodes file \"%s\"\n",
                    safe_map_name(file));
            break;
        }
    }
}

/***====================================================================***/

static bool
CopyKeyNamesToKeymap(struct xkb_keymap *keymap, KeyNamesInfo *info)
{
    struct xkb_key *keys;
    xkb_keycode_t min_key_code, max_key_code, kc;

    min_key_code = info->min_key_code;
    max_key_code = info->max_key_code;
    /* If the keymap has no keys, let's just use the safest pair we know. */
    if (min_key_code == XKB_KEYCODE_INVALID) {
        min_key_code = 8;
        max_key_code = 255;
    }

    keys = calloc(max_key_code + 1, sizeof(*keys));
    if (!keys)
        return false;

    for (kc = min_key_code; kc <= max_key_code; kc++)
        keys[kc].keycode = kc;

    for (kc = info->min_key_code; kc <= info->max_key_code; kc++)
        keys[kc].name = darray_item(info->key_names, kc);

    keymap->min_key_code = min_key_code;
    keymap->max_key_code = max_key_code;
    keymap->keys = keys;
    return true;
}

static bool
CopyKeyAliasesToKeymap(struct xkb_keymap *keymap, KeyNamesInfo *info)
{
    AliasInfo *alias;
    unsigned i, num_key_aliases;
    struct xkb_key_alias *key_aliases;

    /*
     * Do some sanity checking on the aliases. We can't do it before
     * because keys and their aliases may be added out-of-order.
     */
    num_key_aliases = 0;
    darray_foreach(alias, info->aliases) {
        /* Check that ->real is a key. */
        if (!XkbKeyByName(keymap, alias->real, false)) {
            log_vrb(info->ctx, 5,
                    XKB_WARNING_UNDEFINED_KEYCODE,
                    "Attempt to alias %s to non-existent key %s; Ignored\n",
                    KeyNameText(info->ctx, alias->alias),
                    KeyNameText(info->ctx, alias->real));
            alias->real = XKB_ATOM_NONE;
            continue;
        }

        /* Check that ->alias is not a key. */
        if (XkbKeyByName(keymap, alias->alias, false)) {
            log_vrb(info->ctx, 5,
                    XKB_WARNING_ILLEGAL_KEYCODE_ALIAS,
                    "Attempt to create alias with the name of a real key; "
                    "Alias \"%s = %s\" ignored\n",
                    KeyNameText(info->ctx, alias->alias),
                    KeyNameText(info->ctx, alias->real));
            alias->real = XKB_ATOM_NONE;
            continue;
        }

        num_key_aliases++;
    }

    /* Copy key aliases. */
    key_aliases = NULL;
    if (num_key_aliases > 0) {
        key_aliases = calloc(num_key_aliases, sizeof(*key_aliases));
        if (!key_aliases)
            return false;

        i = 0;
        darray_foreach(alias, info->aliases) {
            if (alias->real != XKB_ATOM_NONE) {
                key_aliases[i].alias = alias->alias;
                key_aliases[i].real = alias->real;
                i++;
            }
        }
    }

    keymap->num_key_aliases = num_key_aliases;
    keymap->key_aliases = key_aliases;
    return true;
}

static bool
CopyLedNamesToKeymap(struct xkb_keymap *keymap, KeyNamesInfo *info)
{
    keymap->num_leds = info->num_led_names;
    for (xkb_led_index_t idx = 0; idx < info->num_led_names; idx++) {
        LedNameInfo *ledi = &info->led_names[idx];

        if (ledi->name == XKB_ATOM_NONE)
            continue;

        keymap->leds[idx].name = ledi->name;
    }

    return true;
}

static bool
CopyKeyNamesInfoToKeymap(struct xkb_keymap *keymap, KeyNamesInfo *info)
{
    /* This function trashes keymap on error, but that's OK. */
    if (!CopyKeyNamesToKeymap(keymap, info) ||
        !CopyKeyAliasesToKeymap(keymap, info) ||
        !CopyLedNamesToKeymap(keymap, info))
        return false;

    keymap->keycodes_section_name = strdup_safe(info->name);
    XkbEscapeMapName(keymap->keycodes_section_name);
    return true;
}

/***====================================================================***/

bool
CompileKeycodes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge)
{
    KeyNamesInfo info;

    InitKeyNamesInfo(&info, keymap->ctx, 0);

    if (file != NULL)
        HandleKeycodesFile(&info, file, merge);

    if (info.errorCount != 0)
        goto err_info;

    if (!CopyKeyNamesInfoToKeymap(keymap, &info))
        goto err_info;

    ClearKeyNamesInfo(&info);
    return true;

err_info:
    ClearKeyNamesInfo(&info);
    return false;
}
