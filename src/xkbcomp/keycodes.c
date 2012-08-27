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
#include "text.h"
#include "expr.h"
#include "keycodes.h"
#include "include.h"

/*
 * The xkb_keycodes section
 * ========================
 *
 * This is the simplest section type, and is the first one to be
 * compiled. The purpose of this is mostly to map between the
 * hardware/evdev scancodes and xkb keycodes. Each key is given a name
 * of up to 4 letters, by which it can be referred to later, e.g. in the
 * symbols section.
 *
 * Minimum/Maximum keycode
 * -----------------------
 * Statements of the form:
 *      minimum = 8;
 *      maximum = 255;
 *
 * The file may explicitly declare the minimum and/or maximum keycode
 * contained therein (traditionally 8-255, inherited from old xfree86
 * keycodes). If these are stated explicitly, they are enforced. If
 * they are not stated, they are computed automatically.
 *
 * Keycode statements
 * ------------------
 * Statements of the form:
 *      <TLDE> = 49;
 *      <AE01> = 10;
 *
 * The above would let 49 and 10 be valid keycodes in the keymap, and
 * assign them the names TLDE and AE01 respectively. The format <WXYZ> is
 * always used to refer to a key by name.
 *
 * [ The naming convention <AE01> just denoted the position of the key
 * in the main alphanumric section of the keyboard, with the top left key
 * (usually "~") acting as origin. <AE01> is the key in the first row
 * second column (which is usually "1"). ]
 *
 * In the common case this just maps to the evdev scancodes from
 * /usr/include/linux/input.h, e.g. the following definitions:
 *      #define KEY_GRAVE            41
 *      #define KEY_1                2
 * Similar definitions appear in the xf86-input-keyboard driver. Note
 * that in all current keymaps there's a constant offset of 8 (for
 * historical reasons).
 *
 * If there's a conflict, like the same name given to different keycodes,
 * or same keycode given different names, it is resolved according to the
 * merge mode which applies to the definitions.
 *
 * The reason for the 4 characters limit is that the name is sometimes
 * converted to an unsigned long (in a direct mapping), instead of a char
 * array (see KeyNameToLong, LongToKeyName).
 *
 * Alias statements
 * ----------------
 * Statements of the form:
 *      alias <MENU> = <COMP>;
 *
 * Allows to refer to a previously defined key (here <COMP>) by another
 * name (here <MENU>). Conflicts are handled similarly.
 *
 * Indicator name statements
 * -------------------------
 * Statements of the form:
 *      indicator 1 = "Caps Lock";
 *      indicator 2 = "Num Lock";
 *      indicator 3 = "Scroll Lock";
 *
 * Assigns a name the indicator (i.e. keyboard LED) with the given index.
 * The amount of possible indicators is predetermined (XkbNumIndicators).
 * The indicator may be referred by this name later in the compat section
 * and by the user.
 *
 * Effect on the keymap
 * --------------------
 * After all of the xkb_keycodes sections have been compiled, the
 * following members of struct xkb_keymap are finalized:
 *      xkb_keycode_t min_key_code;
 *      xkb_keycode_t max_key_code;
 *      darray(struct xkb_key_alias) key_aliases;
 *      const char *indicator_names[XkbNumIndicators];
 *      char *keycodes_section_name;
 * Further, the array of keys:
 *      darray(struct xkb_key) keys;
 * had been resized to its final size (i.e. all of the xkb_key objects are
 * referable by their keycode). However the objects themselves do not
 * contain any useful information besides the key name at this point.
 */

typedef struct _AliasInfo {
    enum merge_mode merge;
    unsigned file_id;

    unsigned long alias;
    unsigned long real;
} AliasInfo;

typedef struct _IndicatorNameInfo {
    enum merge_mode merge;
    unsigned file_id;

    xkb_atom_t name;
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
    IndicatorNameInfo indicator_names[XkbNumIndicators];
    darray(AliasInfo) aliases;

    struct xkb_context *ctx;
} KeyNamesInfo;

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
              char alias[XkbKeyNameLength], char real[XkbKeyNameLength])
{
    memset(info, 0, sizeof(*info));
    info->merge = merge;
    info->file_id = file_id;
    info->alias = KeyNameToLong(alias);
    info->real = KeyNameToLong(real);
}

static IndicatorNameInfo *
FindIndicatorByName(KeyNamesInfo *info, xkb_atom_t name,
                    xkb_led_index_t *idx_out)
{
    xkb_led_index_t idx;

    for (idx = 0; idx < XkbNumIndicators; idx++) {
        if (info->indicator_names[idx].name == name) {
            *idx_out = idx;
            return &info->indicator_names[idx];
        }
    }

    return NULL;
}

static bool
AddIndicatorName(KeyNamesInfo *info, enum merge_mode merge,
                 IndicatorNameInfo *new, xkb_led_index_t new_idx)
{
    xkb_led_index_t old_idx;
    IndicatorNameInfo *old;
    bool replace, report;
    int verbosity = xkb_get_log_verbosity(info->ctx);

    replace = (merge == MERGE_REPLACE) || (merge == MERGE_OVERRIDE);

    old = FindIndicatorByName(info, new->name, &old_idx);
    if (old) {
        report = ((old->file_id == new->file_id && verbosity > 0) ||
                  verbosity > 9);

        if (old_idx == new_idx) {
            if (report)
                log_warn(info->ctx, "Multiple indicators named %s; "
                         "Identical definitions ignored\n",
                         xkb_atom_text(info->ctx, new->name));
            return true;
        }

        if (report)
            log_warn(info->ctx, "Multiple indicators named %s; "
                     "Using %d, ignoring %d\n",
                     xkb_atom_text(info->ctx, new->name),
                     (replace ? old_idx + 1 : new_idx + 1),
                     (replace ? new_idx + 1 : old_idx + 1));

        /*
         * XXX: If in the next check we ignore new, than we will have
         * deleted this old for nothing!
         */
        if (replace)
            memset(old, 0, sizeof(*old));
    }

    old = &info->indicator_names[new_idx];
    if (old->name != XKB_ATOM_NONE) {
        report = ((old->file_id == new->file_id && verbosity > 0) ||
                  verbosity > 9);

        if (old->name == new->name) {
            if (report)
                log_warn(info->ctx, "Multiple names for indicator %d; "
                         "Identical definitions ignored\n", new_idx + 1);
        }
        else if (replace) {
            if (report)
                log_warn(info->ctx, "Multiple names for indicator %d; "
                         "Using %s, ignoring %s\n", new_idx + 1,
                         xkb_atom_text(info->ctx, new->name),
                         xkb_atom_text(info->ctx, old->name));
            old->name = new->name;
        }
        else {
            if (report)
                log_warn(info->ctx, "Multiple names for indicator %d; "
                         "Using %s, ignoring %s\n", new_idx + 1,
                         xkb_atom_text(info->ctx, old->name),
                         xkb_atom_text(info->ctx, new->name));
        }

        return true;
    }

    info->indicator_names[new_idx] = *new;
    return true;
}

static void
ClearKeyNamesInfo(KeyNamesInfo * info)
{
    free(info->name);
    info->name = NULL;
    info->merge = MERGE_DEFAULT;
    info->computedMax = info->explicitMax = info->explicitMin = 0;
    info->computedMin = XKB_KEYCODE_MAX;
    darray_free(info->names);
    darray_free(info->files);
    memset(info->indicator_names, 0, sizeof(info->indicator_names));
    darray_free(info->aliases);
}

static void
InitKeyNamesInfo(KeyNamesInfo *info, struct xkb_context *ctx,
                 unsigned file_id)
{
    info->name = NULL;
    info->merge = MERGE_DEFAULT;
    darray_init(info->aliases);
    info->file_id = file_id;
    darray_init(info->names);
    darray_init(info->files);
    ClearKeyNamesInfo(info);
    info->errorCount = 0;
    info->ctx = ctx;
}

static int
FindKeyByLong(KeyNamesInfo * info, unsigned long name)
{
    xkb_keycode_t i;

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
AddKeyName(KeyNamesInfo *info, xkb_keycode_t kc, unsigned long name,
           enum merge_mode merge, unsigned file_id, bool reportCollisions)
{
    xkb_keycode_t old;
    int verbosity = xkb_get_log_verbosity(info->ctx);

    ResizeKeyNameArrays(info, kc);

    if (kc < info->computedMin)
        info->computedMin = kc;
    if (kc > info->computedMax)
        info->computedMax = kc;

    if (reportCollisions)
        reportCollisions = (verbosity > 7 ||
                            (verbosity > 0 &&
                             file_id == darray_item(info->files, kc)));

    if (darray_item(info->names, kc) != 0) {
        const char *lname = LongKeyNameText(darray_item(info->names, kc));
        const char *kname = LongKeyNameText(name);

        if (darray_item(info->names, kc) == name && reportCollisions) {
            log_warn(info->ctx, "Multiple identical key name definitions; "
                     "Later occurences of \"%s = %d\" ignored\n", lname, kc);
            return true;
        }

        if (merge == MERGE_AUGMENT) {
            if (reportCollisions)
                log_warn(info->ctx, "Multiple names for keycode %d; "
                         "Using %s, ignoring %s\n", kc, lname, kname);
            return true;
        }
        else {
            if (reportCollisions)
                log_warn(info->ctx, "Multiple names for keycode %d; "
                         "Using %s, ignoring %s\n", kc, kname, lname);
            darray_item(info->names, kc) = 0;
            darray_item(info->files, kc) = 0;
        }
    }

    old = FindKeyByLong(info, name);
    if (old != 0 && old != kc) {
        const char *kname = LongKeyNameText(name);

        if (merge == MERGE_OVERRIDE) {
            darray_item(info->names, old) = 0;
            darray_item(info->files, old) = 0;
            if (reportCollisions)
                log_warn(info->ctx, "Key name %s assigned to multiple keys; "
                         "Using %d, ignoring %d\n", kname, kc, old);
        }
        else {
            if (reportCollisions && verbosity > 3)
                log_warn(info->ctx, "Key name %s assigned to multiple keys; "
                         "Using %d, ignoring %d\n", kname, old, kc);
            return true;
        }
    }

    darray_item(info->names, kc) = name;
    darray_item(info->files, kc) = file_id;
    return true;
}

/***====================================================================***/

static int
HandleAliasDef(KeyNamesInfo *info, KeyAliasDef *def, enum merge_mode merge,
               unsigned file_id);

static bool
MergeAliases(KeyNamesInfo *into, KeyNamesInfo *from, enum merge_mode merge)
{
    AliasInfo *alias;
    KeyAliasDef def;

    if (darray_empty(from->aliases))
        return true;

    if (darray_empty(into->aliases)) {
        into->aliases = from->aliases;
        darray_init(from->aliases);
        return true;
    }

    memset(&def, 0, sizeof(def));

    darray_foreach(alias, from->aliases) {
        def.merge = (merge == MERGE_DEFAULT) ? alias->merge : merge;
        LongToKeyName(alias->alias, def.alias);
        LongToKeyName(alias->real, def.real);

        if (!HandleAliasDef(into, &def, def.merge, alias->file_id))
            return false;
    }

    return true;
}

static void
MergeIncludedKeycodes(KeyNamesInfo *into, KeyNamesInfo *from,
                      enum merge_mode merge)
{
    xkb_keycode_t i;
    xkb_led_index_t idx;

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
        unsigned long name = darray_item(from->names, i);
        if (name == 0)
            continue;

        if (!AddKeyName(into, i, name, merge, from->file_id, false))
            into->errorCount++;
    }

    for (idx = 0; idx < XkbNumIndicators; idx++) {
        IndicatorNameInfo *led = &from->indicator_names[idx];
        if (led->name == XKB_ATOM_NONE)
            continue;

        led->merge = (merge == MERGE_DEFAULT ? led->merge : merge);
        if (!AddIndicatorName(into, led->merge, led, idx))
            into->errorCount++;
    }

    if (!MergeAliases(into, from, merge))
        into->errorCount++;

    if (from->explicitMin != 0)
        if (into->explicitMin == 0 || into->explicitMin > from->explicitMin)
            into->explicitMin = from->explicitMin;

    if (from->explicitMax > 0)
        if (into->explicitMax == 0 || into->explicitMax < from->explicitMax)
            into->explicitMax = from->explicitMax;
}

static void
HandleKeycodesFile(KeyNamesInfo *info, XkbFile *file, enum merge_mode merge);

static bool
HandleIncludeKeycodes(KeyNamesInfo *info, IncludeStmt *stmt)
{
    enum merge_mode merge = MERGE_DEFAULT;
    XkbFile *rtrn;
    KeyNamesInfo included, next_incl;

    InitKeyNamesInfo(&included, info->ctx, info->file_id);
    if (stmt->stmt) {
        free(included.name);
        included.name = stmt->stmt;
        stmt->stmt = NULL;
    }

    for (; stmt; stmt = stmt->next_incl) {
        if (!ProcessIncludeFile(info->ctx, stmt, FILE_TYPE_KEYCODES,
                                &rtrn, &merge)) {
            info->errorCount += 10;
            ClearKeyNamesInfo(&included);
            return false;
        }

        InitKeyNamesInfo(&next_incl, info->ctx, rtrn->id);

        HandleKeycodesFile(&next_incl, rtrn, MERGE_OVERRIDE);

        MergeIncludedKeycodes(&included, &next_incl, merge);

        ClearKeyNamesInfo(&next_incl);
        FreeXkbFile(rtrn);
    }

    MergeIncludedKeycodes(info, &included, merge);
    ClearKeyNamesInfo(&included);

    return (info->errorCount == 0);
}

static int
HandleKeycodeDef(KeyNamesInfo *info, KeycodeDef *stmt, enum merge_mode merge)
{
    if ((info->explicitMin != 0 && stmt->value < info->explicitMin) ||
        (info->explicitMax != 0 && stmt->value > info->explicitMax)) {
        log_err(info->ctx, "Illegal keycode %lu for name %s; "
                "Must be in the range %d-%d inclusive\n",
                stmt->value, KeyNameText(stmt->name), info->explicitMin,
                info->explicitMax ? info->explicitMax : XKB_KEYCODE_MAX);
        return 0;
    }

    if (stmt->merge != MERGE_DEFAULT) {
        if (stmt->merge == MERGE_REPLACE)
            merge = MERGE_OVERRIDE;
        else
            merge = stmt->merge;
    }

    return AddKeyName(info, stmt->value, KeyNameToLong(stmt->name), merge,
                      info->file_id, true);
}

static void
HandleAliasCollision(KeyNamesInfo *info, AliasInfo *old, AliasInfo *new)
{
    int verbosity = xkb_get_log_verbosity(info->ctx);
    bool report = ((new->file_id == old->file_id && verbosity > 0) ||
                   verbosity > 9);

    if (new->real == old->real) {
        if (report)
            log_warn(info->ctx, "Alias of %s for %s declared more than once; "
                     "First definition ignored\n",
                     LongKeyNameText(new->alias), LongKeyNameText(new->real));
    }
    else {
        unsigned long use, ignore;

        use = (new->merge == MERGE_AUGMENT ? old->real : new->real);
        ignore = (new->merge == MERGE_AUGMENT ? new->real : old->real);

        if (report)
            log_warn(info->ctx, "Multiple definitions for alias %s; "
                     "Using %s, ignoring %s\n",
                     LongKeyNameText(old->alias), LongKeyNameText(use),
                     LongKeyNameText(ignore));

        old->real = use;
    }

    old->file_id = new->file_id;
    old->merge = new->merge;
}

static int
HandleAliasDef(KeyNamesInfo *info, KeyAliasDef *def, enum merge_mode merge,
               unsigned file_id)
{
    AliasInfo *alias, new;

    darray_foreach(alias, info->aliases) {
        if (alias->alias == KeyNameToLong(def->alias)) {
            InitAliasInfo(&new, merge, file_id, def->alias, def->real);
            HandleAliasCollision(info, alias, &new);
            return true;
        }
    }

    InitAliasInfo(&new, merge, file_id, def->alias, def->real);
    darray_append(info->aliases, new);
    return true;
}

#define MIN_KEYCODE_DEF 0
#define MAX_KEYCODE_DEF 1

static int
HandleKeyNameVar(KeyNamesInfo *info, VarDef *stmt)
{
    const char *elem, *field;
    xkb_keycode_t kc;
    ExprDef *arrayNdx;
    int which;

    if (!ExprResolveLhs(info->ctx, stmt->name, &elem, &field, &arrayNdx))
        return false;

    if (elem) {
        log_err(info->ctx, "Unknown element %s encountered; "
                "Default for field %s ignored\n", elem, field);
        return false;
    }

    if (istreq(field, "minimum")) {
        which = MIN_KEYCODE_DEF;
    }
    else if (istreq(field, "maximum")) {
        which = MAX_KEYCODE_DEF;
    }
    else {
        log_err(info->ctx, "Unknown field encountered; "
                "Assigment to field %s ignored\n", field);
        return false;
    }

    if (arrayNdx != NULL) {
        log_err(info->ctx, "The %s setting is not an array; "
                "Illegal array reference ignored\n", field);
        return false;
    }

    if (!ExprResolveKeyCode(info->ctx, stmt->value, &kc)) {
        log_err(info->ctx, "Illegal keycode encountered; "
                "Assignment to field %s ignored\n", field);
        return false;
    }

    if (kc > XKB_KEYCODE_MAX) {
        log_err(info->ctx,
                "Illegal keycode %d (must be in the range %d-%d inclusive); "
                "Value of \"%s\" not changed\n",
                kc, 0, XKB_KEYCODE_MAX, field);
        return false;
    }

    if (which == MIN_KEYCODE_DEF) {
        if (info->explicitMax > 0 && info->explicitMax < kc) {
            log_err(info->ctx,
                    "Minimum key code (%d) must be <= maximum key code (%d); "
                    "Minimum key code value not changed\n",
                    kc, info->explicitMax);
            return false;
        }

        if (info->computedMax > 0 && info->computedMin < kc) {
            log_err(info->ctx,
                    "Minimum key code (%d) must be <= lowest defined key (%d); "
                    "Minimum key code value not changed\n",
                    kc, info->computedMin);
            return false;
        }

        info->explicitMin = kc;
    }
    else if (which == MAX_KEYCODE_DEF) {
        if (info->explicitMin > 0 && info->explicitMin > kc) {
            log_err(info->ctx,
                    "Maximum code (%d) must be >= minimum key code (%d); "
                    "Maximum code value not changed\n",
                    kc, info->explicitMin);
            return false;
        }

        if (info->computedMax > 0 && info->computedMax > kc) {
            log_err(info->ctx,
                    "Maximum code (%d) must be >= highest defined key (%d); "
                    "Maximum code value not changed\n",
                    kc, info->computedMax);
            return false;
        }

        info->explicitMax = kc;
    }

    return true;
}

static int
HandleIndicatorNameDef(KeyNamesInfo *info, IndicatorNameDef *def,
                       enum merge_mode merge)
{
    IndicatorNameInfo ii;
    const char *str;

    if (def->ndx < 1 || def->ndx > XkbNumIndicators) {
        info->errorCount++;
        log_err(info->ctx,
                "Name specified for illegal indicator index %d\n; Ignored\n",
                def->ndx);
        return false;
    }

    if (!ExprResolveString(info->ctx, def->name, &str)) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", def->ndx);
        info->errorCount++;
        return ReportBadType(info->ctx, "indicator", "name", buf,
                             "string");
    }

    ii.merge = info->merge;
    ii.file_id = info->file_id;
    ii.name = xkb_atom_intern(info->ctx, str);
    return AddIndicatorName(info, merge, &ii, def->ndx - 1);
}

static void
HandleKeycodesFile(KeyNamesInfo *info, XkbFile *file, enum merge_mode merge)
{
    ParseCommon *stmt;
    bool ok;

    free(info->name);
    info->name = strdup_safe(file->name);

    for (stmt = file->defs; stmt; stmt = stmt->next) {
        switch (stmt->type) {
        case STMT_INCLUDE:
            ok = HandleIncludeKeycodes(info, (IncludeStmt *) stmt);
            break;
        case STMT_KEYCODE:
            ok = HandleKeycodeDef(info, (KeycodeDef *) stmt, merge);
            break;
        case STMT_ALIAS:
            ok = HandleAliasDef(info, (KeyAliasDef *) stmt, merge,
                                info->file_id);
            break;
        case STMT_VAR:
            ok = HandleKeyNameVar(info, (VarDef *) stmt);
            break;
        case STMT_INDICATOR_NAME:
            ok = HandleIndicatorNameDef(info, (IndicatorNameDef *) stmt,
                                        merge);
            break;
        default:
            log_err(info->ctx,
                    "Keycode files may define key and indicator names only; "
                    "Ignoring %s\n", stmt_type_to_string(stmt->type));
            ok = false;
            break;
        }

        if (!ok)
            info->errorCount++;

        if (info->errorCount > 10) {
            log_err(info->ctx, "Abandoning keycodes file \"%s\"\n",
                    file->topName);
            break;
        }
    }
}

static void
ApplyAliases(KeyNamesInfo *info, struct xkb_keymap *keymap)
{
    struct xkb_key *key;
    struct xkb_key_alias *a, new;
    AliasInfo *alias;

    darray_foreach(alias, info->aliases) {
        /* Check that ->real is a key. */
        key = FindNamedKey(keymap, alias->real, false, 0);
        if (!key) {
            log_lvl(info->ctx, 5,
                    "Attempt to alias %s to non-existent key %s; Ignored\n",
                    LongKeyNameText(alias->alias),
                    LongKeyNameText(alias->real));
            continue;
        }

        /* Check that ->alias is not a key. */
        key = FindNamedKey(keymap, alias->alias, false, 0);
        if (key) {
            log_lvl(info->ctx, 5,
                    "Attempt to create alias with the name of a real key; "
                    "Alias \"%s = %s\" ignored\n",
                    LongKeyNameText(alias->alias),
                    LongKeyNameText(alias->real));
            continue;
        }

        /* Check that ->alias in not already an alias, and if so handle it. */
        darray_foreach(a, keymap->key_aliases) {
            AliasInfo old_alias;

            if (KeyNameToLong(a->alias) != alias->alias)
                continue;

            InitAliasInfo(&old_alias, MERGE_AUGMENT, 0, a->alias, a->real);
            HandleAliasCollision(info, &old_alias, alias);
            LongToKeyName(old_alias.alias, a->alias);
            LongToKeyName(old_alias.real, a->real);
            alias->alias = 0;
        }
        if (alias->alias == 0)
            continue;

        /* Add the alias. */
        LongToKeyName(alias->alias, new.alias);
        LongToKeyName(alias->real, new.real);
        darray_append(keymap->key_aliases, new);
    }

    darray_free(info->aliases);
}

static bool
CopyKeyNamesToKeymap(struct xkb_keymap *keymap, KeyNamesInfo *info)
{
    xkb_keycode_t kc;
    xkb_led_index_t idx;

    if (info->explicitMin > 0)
        keymap->min_key_code = info->explicitMin;
    else
        keymap->min_key_code = info->computedMin;

    if (info->explicitMax > 0)
        keymap->max_key_code = info->explicitMax;
    else
        keymap->max_key_code = info->computedMax;

    darray_resize0(keymap->keys, keymap->max_key_code + 1);
    for (kc = info->computedMin; kc <= info->computedMax; kc++)
        LongToKeyName(darray_item(info->names, kc),
                      XkbKey(keymap, kc)->name);

    keymap->keycodes_section_name = strdup_safe(info->name);

    for (idx = 0; idx < XkbNumIndicators; idx++) {
        IndicatorNameInfo *led = &info->indicator_names[idx];
        if (led->name == XKB_ATOM_NONE)
            continue;

        keymap->indicator_names[idx] =
            xkb_atom_text(keymap->ctx, led->name);
    }

    ApplyAliases(info, keymap);

    return true;
}

bool
CompileKeycodes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge)
{
    KeyNamesInfo info;

    InitKeyNamesInfo(&info, keymap->ctx, file->id);

    HandleKeycodesFile(&info, file, merge);
    if (info.errorCount != 0)
        goto err_info;

    if (!CopyKeyNamesToKeymap(keymap, &info))
        goto err_info;

    ClearKeyNamesInfo(&info);
    return true;

err_info:
    ClearKeyNamesInfo(&info);
    return false;
}

struct xkb_key *
FindNamedKey(struct xkb_keymap *keymap, unsigned long name,
             bool use_aliases, xkb_keycode_t start_from)
{
    struct xkb_key *key;

    if (start_from < keymap->min_key_code)
        start_from = keymap->min_key_code;
    else if (start_from > keymap->max_key_code)
        return NULL;

    xkb_foreach_key_from(key, keymap, start_from)
        if (KeyNameToLong(key->name) == name)
            return key;

    if (use_aliases) {
        unsigned long new_name;
        if (FindKeyNameForAlias(keymap, name, &new_name))
            return FindNamedKey(keymap, new_name, false, 0);
    }

    return NULL;
}

bool
FindKeyNameForAlias(struct xkb_keymap *keymap, unsigned long lname,
                    unsigned long *real_name)
{
    char name[XkbKeyNameLength];
    struct xkb_key_alias *a;

    LongToKeyName(lname, name);
    darray_foreach(a, keymap->key_aliases) {
        if (strncmp(name, a->alias, XkbKeyNameLength) == 0) {
            *real_name = KeyNameToLong(a->real);
            return true;
        }
    }

    return false;
}
