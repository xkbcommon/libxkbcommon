/*
 * For HPND:
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * For MIT:
 * Copyright © 2012 Intel Corporation
 *
 * SPDX-License-Identifier: HPND AND MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon-keysyms.h"
#include "xkbcommon/xkbcommon.h"
#include "atom.h"
#include "action.h"
#include "darray.h"
#include "keymap.h"
#include "messages-codes.h"
#include "text.h"
#include "xkbcomp-priv.h"

#define BUF_CHUNK_SIZE 4096

struct buf {
    char *buf;
    size_t size;
    size_t alloc;
};

#define xkb_abs(n) _Generic((n),                \
                            int: abs,           \
                            long int: labs,     \
                            default: llabs )((n))

static bool
do_realloc(struct buf *buf, size_t at_least)
{
    buf->alloc += BUF_CHUNK_SIZE;
    if (at_least >= BUF_CHUNK_SIZE)
        buf->alloc += at_least;

    char *const new = realloc(buf->buf, buf->alloc);
    if (!new)
        return false;

    buf->buf = new;
    return true;
}

ATTR_PRINTF(2, 3) static bool
check_write_buf(struct buf *buf, const char *fmt, ...)
{
    va_list args;
    int printed;
    size_t available;

    available = buf->alloc - buf->size;
    /* Try to write to the buffer and get the required min size (without the
     * final null byte).
     * C11 states that if `available` is 0, then the pointer may be null. */
    assert(buf->buf || available == 0);
    va_start(args, fmt);
    printed = vsnprintf(buf->buf ? buf->buf + buf->size : buf->buf, available,
                        fmt, args);
    va_end(args);

    if (printed < 0) {
        /* Some error */
        goto err;
    } else if ((size_t) printed >= available) {
        /* Not enough space: need to realloc */
        if (!do_realloc(buf, printed + 1))
            goto err;

        /* The buffer has enough space now. */
        available = buf->alloc - buf->size;
        assert(buf->buf && (size_t) printed < available);
        va_start(args, fmt);
        printed = vsnprintf(buf->buf + buf->size, available, fmt, args);
        va_end(args);

        if (printed < 0 || (size_t) printed >= available)
            goto err;
    }

    buf->size += printed;
    return true;

err:
    free(buf->buf);
    buf->buf = NULL;
    return false;
}

#define write_buf(buf, ...) do { \
    if (!check_write_buf(buf, __VA_ARGS__)) \
        return false; \
} while (0)

static bool
check_copy_to_buf(struct buf *buf, const char* source, size_t len)
{
    if (len == 0)
        return true;

    const size_t available = buf->alloc - buf->size;
    if (len >= available) {
        /* len + 1 (terminating NULL) */
        if (!do_realloc(buf, len + 1)) {
            free(buf->buf);
            buf->buf = NULL;
            return false;
        }
    }

    memcpy(buf->buf + buf->size, source, len);
    buf->size += len;
    /* Append NULL byte */
    buf->buf[buf->size] = '\0';
    return true;
}

#define copy_to_buf_len(buf, source, len) do { \
    if (!check_copy_to_buf(buf, source, len)) \
        return false; \
} while (0)

#define copy_to_buf(buf, source) \
    copy_to_buf_len(buf, source, sizeof(source) - 1)

static bool
check_write_string_literal(struct buf *buf, const char* string)
{
    copy_to_buf(buf, "\"");

    /* Write chunks, separated by characters requiring escape sequence */
    size_t pending_start = 0;
    size_t current = 0;
    char escape_buffer[] = {'\\', '0', '0', '0', '0'};
    for (; string[current] != '\0'; current++) {
        uint8_t escape_len;
        switch (string[current]) {
        case '\n':
            /* `\n` would break strings */
            escape_buffer[1] = 'n';
            escape_len = 2;
            break;
        case '\\':
            /* `\` would create escape sequences */
            escape_buffer[1] = '\\';
            escape_len = 2;
            break;
        default:
            /*
             * Handle `"` (would break strings) and ASCII control characters
             * with an octal escape sequence. Xorg xkbcomp does not parse the
             * escape sequence `\"` nor xkbcommon < 1.9.0, so in order to be
             * backward compatible we must use the octal escape sequence in
             * xkbcomp style `\0nnn` with *4* digits:
             *
             * 1. To be compatible with Xorg xkbcomp.
             * 2. To avoid issues with the next char: e.g. "\00427" should not
             *    be emitted as "\427" nor "\0427".
             *
             * Note that xkbcommon < 1.9.0 will still not parse this correctly,
             * although it will not raise an error: e.g. the escape for `"`,
             * `\0042`, would be parsed as `\004` + `2`.
             */
            if (unlikely((unsigned char)string[current] < 0x20U ||
                         string[current] == '"')) {
                escape_buffer[1] = '0';
                escape_buffer[2] = '0';
                escape_buffer[3] = (char)('0' + (string[current] >> 3));
                escape_buffer[4] = (char)('0' + (string[current] & 0x7));
                escape_len = 5;
            } else {
                /* Expand the chunk */
                continue;
            }
        }
        /* Write pending chunk */
        assert(current >= pending_start);
        copy_to_buf_len(buf, string + pending_start, current - pending_start);
        /* Write escape sequence */
        copy_to_buf_len(buf, escape_buffer, escape_len);
        pending_start = current + 1;
    }
    /* Write remaining chunk */
    assert(current >= pending_start);
    copy_to_buf_len(buf, string + pending_start, current - pending_start);

    copy_to_buf(buf, "\"");
    return true;
}

#define write_buf_string_literal(buf, string) do { \
    if (!check_write_string_literal(buf, string)) \
        return false; \
} while (0)

static void
delete_last_char(struct buf *buf, char c)
{
    if (buf->size && buf->buf[buf->size - 1] == c)
        buf->buf[--buf->size] = '\0';
}

/** Entry for a name substituion LUT */
struct key_name_substitution {
    xkb_atom_t source;
    xkb_atom_t target;
    /** Used for intuitive renaming */
    xkb_keycode_t keycode;
};

/** Compare 2 key name substitution entries by they original name (atom) */
static int
cmp_key_name_substitution(const void *a, const void *b)
{
    const xkb_atom_t s1 = ((struct key_name_substitution *) a)->source;
    const xkb_atom_t s2 = ((struct key_name_substitution *) b)->source;
    return (s1 < s2) ? -1 : ((s1 > s2) ? 1 : 0);
}

typedef darray(struct key_name_substitution) key_name_substitutions;

/**
 * Rename keys that are not compatible with the original XKB protocol,
 * i.e. more that 4 bytes. The new name will be formed as prefix + 3-digit
 * hexadecimal number.
 */
static bool
rename_long_keys(struct xkb_keymap *keymap,
                 key_name_substitutions *substitutions)
{
    /*
     * Prefixes we want to use for the new names: ASCII range `!`..`0`.
     * They are really unlikely to be used in usual keymaps.
     * NULL denotes an unavailable prefix.
     */
    enum {
        first = '!',
        last = '0',
        invalid = '\0'
    };
    char prefixes[] = {
        first, invalid /* skip ‘"’, which would look buggy otherwise */,
        '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', last
    };
    static_assert(sizeof(prefixes) == last - first + 1, "invalid range");

    /*
     * Traverse all the keys and aliases:
     * - Collect items with incompatible names.
     * - Remove prefixes that are already used in valid 4-char names.
     *   This ensures no name conflict will occur with the new names, without
     *   requiring an extensive check.
     */

    /* Check key names */
    const struct xkb_key *key;
    xkb_keys_foreach(key, keymap) {
        if (key->name == XKB_ATOM_NONE)
            continue;

        const char * const name = xkb_atom_text(keymap->ctx, key->name);
        const size_t len = strlen_safe(name);
        if (len > 4) {
            /* Key name too long: append key, pending for renaming */
            darray_append(*substitutions, (struct key_name_substitution) {
                .source = key->name,
                .keycode = key->keycode,
                .target = key->name
            });
        } else if (len == 4 && name[0] >= first && name[0] <= last) {
            /* Potential name conflict: deactivate this prefix */
            prefixes[name[0] - first] = invalid;
        }
    }

    /* Check aliases */
    for (darray_size_t a = 0; a < keymap->num_key_aliases; a++) {
        const struct xkb_key_alias * const alias = &keymap->key_aliases[a];
        const char * const name = xkb_atom_text(keymap->ctx, alias->alias);
        const size_t len = strlen_safe(name);
        if (len > 4) {
            /* Alias name too long: append alias, pending for renaming */
            darray_append(*substitutions, (struct key_name_substitution) {
                .source = alias->alias,
                .keycode = XKB_KEYCODE_INVALID, /* Too expensive to lookup */
                .target = alias->alias
            });
        } else if (len == 4 && name[0] >= first && name[0] <= last) {
            /* Potential conflict with names substitutes: deactivate prefix */
            prefixes[name[0] - first] = invalid;
        }
    }

    if darray_empty(*substitutions) {
        /* Nothing to do */
        return true;
    }

    /*
     * Create non-conflicting X11-compatible new names
     *
     * It’s one of the following 2 forms:
     * - If key name and keycode ≤ 0xfff: prefix character + hexadecimal keycode
     * - If alias or keycode > 0xfff: prefix character + hexadecimal incrementing
     *   counter.
     * Prefixes are distinct to avoid any name conflict.
     */

    /* Get first available prefix */
    char *prefix = prefixes;
    const char *prefix_max = prefixes + sizeof(prefixes) - 1;
    while (*prefix == invalid && prefix < prefix_max)
        prefix++;

    /* Keycode prefix: prefer ‘0’ if available */
    char keycode_prefix;
    if (prefixes[last - first] == '0') {
        /* ‘0’ is available: use it then mark it unavailable for further use */
        keycode_prefix = last;
        prefixes[last - first] = invalid;
    } else {
        /* ‘0’ is unavailable: use first available prefix */
        keycode_prefix = *prefix;
        if (unlikely(keycode_prefix == invalid)) {
            /*
             * Abort: no prefix available (very unlikely).
             * Do not error though: Wayland clients can still parse the keymap.
             */
            darray_free(*substitutions);
            return true;
        }
        /* Update current prefix */
        *prefix = invalid;
        while (*prefix == invalid && prefix < prefix_max)
            prefix++;
    }

    /* Proceed renaming */
    uint16_t next_id = 1;
    struct key_name_substitution *sub;
    char buf[6] = {0}; /* 5 would suffice, but generates a compiler warning */
    darray_foreach(sub, *substitutions) {
        static_assert(XKB_KEYCODE_INVALID > 0xfff, "Invalid keys filtered out");
        if (sub->keycode <= 0xfff) {
            /*
             * Try nice formatting with the hexadecimal keycode.
             * We use lower case to reduce possible conflict because the
             * traditional key names are upper-cased.
             */
            snprintf(buf, sizeof(buf), "%c%03"PRIx32,
                     keycode_prefix, sub->keycode);
            const xkb_atom_t target = xkb_atom_intern(keymap->ctx,
                                                      buf, sizeof(buf) - 2);
            if (target == XKB_ATOM_NONE)
                goto error;
            sub->target = target;
        } else if (*prefix != invalid) {
            /* Cannot use keycode for formatting: generate next arbitrary ID */
            snprintf(buf, sizeof(buf), "%c%03"PRIx16, *prefix, next_id);
            const xkb_atom_t target = xkb_atom_intern(keymap->ctx,
                                                      buf, sizeof(buf) - 2);
            if (target == XKB_ATOM_NONE)
                goto error;
            sub->target = target;
            if (++next_id > 0xfff) {
                *prefix = invalid;
                /* Get next available prefix */
                while (*prefix == invalid && prefix < prefix_max)
                    prefix++;
                /* Reset counter */
                next_id = 1;
            }
        } else {
            /*
             * No prefix available: skip substitution (very unlikely).
             * Do not error though: Wayland clients can still parse the keymap.
             */
            sub->target = sub->source;
        }
    }

    /* Now sort by the source names so we can do a binary search */
    qsort(darray_items(*substitutions), darray_size(*substitutions),
          sizeof(*darray_items(*substitutions)), &cmp_key_name_substitution);

    return true;

error:
    darray_free(*substitutions);
    /* There was a memory error */
    log_err(keymap->ctx, XKB_ERROR_ALLOCATION_ERROR,
            "Cannot allocate key name substitution\n");
    return false;
}

/** Substitute a key name with a valid X11 key name */
static xkb_atom_t
substitute_name(const key_name_substitutions *substitutions, xkb_atom_t name)
{
    /* Binary search */
    darray_size_t first = 0;
    darray_size_t last = darray_size(*substitutions);

    while (last > first) {
        darray_size_t mid = first + (last - 1 - first) / 2;
        const xkb_atom_t source = darray_item(*substitutions, mid).source;
        if (source < name)
            first = mid + 1;
        else if (source > name)
            last = mid;
        else /* found it */
            return darray_item(*substitutions, mid).target;
    }

    /* No name substitution: return original name */
    return name;
}

static bool
write_vmods(struct xkb_keymap *keymap, enum xkb_keymap_format format,
            bool explicit, struct buf *buf)
{
    const struct xkb_mod *mod;
    xkb_mod_index_t vmod = 0;
    bool has_some = false;

    xkb_vmods_enumerate(vmod, mod, &keymap->mods) {
        if (!has_some) {
            copy_to_buf(buf, "\tvirtual_modifiers ");
            has_some = true;
        } else {
            copy_to_buf(buf, ",");
        }
        write_buf(buf, "%s", xkb_atom_text(keymap->ctx, mod->name));

        /*
         * Ensure to always honor explicit mappings when auto canonical vmods
         * is enabled, so that parsing/serializing can round-trip. This is not
         * required when the feature is disabled (keymap format v1).
         *
         * If the auto feature is enabled, `virtual_modifiers M;` will assign
         * a canonical mapping > 0xff to `M` if there is no further explicit/
         * implicit mapping for `M`.
         *
         * On the contrary, `virtual_modifiers M = none;` prevents the auto
         * mapping of `M`. So if the auto feature is enabled and there is no
         * further explicit/implicit mapping for `M`, then the mapping of `M`
         * will remain unchanged: `none`. It must be serialized to an explicit
         * mapping to prevent triggering the auto canonical mapping presented
         * above.
         */
        const bool auto_canonical_mods = (format >= XKB_KEYMAP_FORMAT_TEXT_V2);

        if ((keymap->mods.explicit_vmods & (UINT32_C(1) << vmod) ||
             explicit) &&
            (auto_canonical_mods || mod->mapping != 0)) {
            /*
             * Explicit non-default mapping
             * NOTE: we can only pretty-print *real* modifiers in this context.
             */
            write_buf(buf, "=%s",
                      ModMaskText(keymap->ctx, MOD_REAL, &keymap->mods,
                                  mod->mapping));
        }
    }

    if (has_some)
        copy_to_buf(buf, ";\n\n");

    return true;
}

static bool
write_keycodes(struct xkb_keymap *keymap,
               const key_name_substitutions *substitutions,
               enum xkb_keymap_serialize_flags flags, struct buf *buf)
{
    const struct xkb_key *key;
    xkb_led_index_t idx;
    const struct xkb_led *led;

    const bool pretty = !!(flags & XKB_KEYMAP_SERIALIZE_PRETTY);

    if (keymap->keycodes_section_name)
        write_buf(buf, "xkb_keycodes \"%s\" {\n",
                  keymap->keycodes_section_name);
    else
        copy_to_buf(buf, "xkb_keycodes {\n");

    /* NOTE: there is no support for default values */

    /* xkbcomp and X11 really want to see keymaps with a minimum of 8, and
     * a maximum of at least 255, else XWayland really starts hating life.
     * If this is a problem and people really need strictly bounded keymaps,
     * we should probably control this with a flag. */
    write_buf(buf, "\tminimum = %"PRIu32";\n", MIN(keymap->min_key_code, 8));
    write_buf(buf, "\tmaximum = %"PRIu32";\n", MAX(keymap->max_key_code, 255));

    xkb_keys_foreach(key, keymap) {
        if (key->name == XKB_ATOM_NONE)
            continue;

        const xkb_atom_t name = (substitutions == NULL)
            ? key->name
            : substitute_name(substitutions, key->name);
        if (pretty)
            write_buf(buf, "\t%-20s", KeyNameText(keymap->ctx, name));
        else
            write_buf(buf, "\t%s", KeyNameText(keymap->ctx, name));
        write_buf(buf, " = %"PRIu32";\n", key->keycode);
    }

    xkb_leds_enumerate(idx, led, keymap)
        if (led->name != XKB_ATOM_NONE) {
            write_buf(buf, "\tindicator %"PRIu32" = ", idx + 1);
            write_buf_string_literal(buf, xkb_atom_text(keymap->ctx, led->name));
            copy_to_buf(buf, ";\n");
        }

    for (darray_size_t i = 0; i < keymap->num_key_aliases; i++) {
        const xkb_atom_t alias = (substitutions == NULL)
            ? keymap->key_aliases[i].alias
            : substitute_name(substitutions, keymap->key_aliases[i].alias);
        const xkb_atom_t real = (substitutions == NULL)
            ? keymap->key_aliases[i].real
            : substitute_name(substitutions, keymap->key_aliases[i].real);

        if (pretty)
            write_buf(buf, "\talias %-14s",
                      KeyNameText(keymap->ctx, alias));
        else
            write_buf(buf, "\talias %s",
                      KeyNameText(keymap->ctx, alias));
        write_buf(buf, " = %s;\n",
                  KeyNameText(keymap->ctx, real));
    }

    copy_to_buf(buf, "};\n\n");
    return true;
}

static bool
write_types(struct xkb_keymap *keymap, enum xkb_keymap_format format,
            enum xkb_keymap_serialize_flags flags, struct buf *buf)
{
    const bool drop_unused = !(flags & XKB_KEYMAP_SERIALIZE_KEEP_UNUSED);
    const bool explicit = !!(flags & XKB_KEYMAP_SERIALIZE_EXPLICIT);

    if (keymap->types_section_name)
        write_buf(buf, "xkb_types \"%s\" {\n",
                  keymap->types_section_name);
    else
        copy_to_buf(buf, "xkb_types {\n");

    /* NOTE: support for default values has been removed */

    if (!write_vmods(keymap, format, explicit, buf))
        return false;

    for (darray_size_t i = 0; i < keymap->num_types; i++) {
        const struct xkb_key_type * const type = &keymap->types[i];
        if (!type->required && drop_unused)
            continue;

        copy_to_buf(buf, "\ttype ");
        write_buf_string_literal(buf, xkb_atom_text(keymap->ctx, type->name));
        copy_to_buf(buf, " {\n");

        write_buf(buf, "\t\tmodifiers= %s;\n",
                  ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods,
                              type->mods.mods));

        for (darray_size_t j = 0; j < type->num_entries; j++) {
            const char *str;
            const struct xkb_key_type_entry *entry = &type->entries[j];

            /*
             * Printing level 1 entries is redundant, it's the default,
             * unless there's preserve info.
             */
            if (entry->level == 0 && entry->preserve.mods == 0 && !explicit)
                continue;

            str = ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods,
                              entry->mods.mods);
            write_buf(buf, "\t\tmap[%s]= %"PRIu32";\n",
                      str, entry->level + 1);

            if (entry->preserve.mods)
                write_buf(buf, "\t\tpreserve[%s]= %s;\n",
                          str, ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods,
                                           entry->preserve.mods));
        }

        for (xkb_level_index_t n = 0; n < type->num_level_names; n++)
            if (type->level_names[n]) {
                write_buf(buf, "\t\tlevel_name[%"PRIu32"]= ", n + 1);
                write_buf_string_literal(
                    buf, xkb_atom_text(keymap->ctx, type->level_names[n]));
                copy_to_buf(buf, ";\n");
            }


        copy_to_buf(buf, "\t};\n");
    }

    copy_to_buf(buf, "};\n\n");
    return true;
}

static bool
write_led_map(struct xkb_keymap *keymap, bool explicit, struct buf *buf,
              const struct xkb_led *led)
{
    copy_to_buf(buf, "\tindicator ");
    write_buf_string_literal(buf, xkb_atom_text(keymap->ctx, led->name));
    copy_to_buf(buf, " {\n");

    if (led->which_groups) {
        if (led->which_groups != XKB_STATE_LAYOUT_EFFECTIVE || explicit) {
            write_buf(buf, "\t\twhichGroupState= %s;\n",
                      LedStateMaskText(keymap->ctx, groupComponentMaskNames,
                                       led->which_groups));
        }
        write_buf(buf, "\t\tgroups= 0x%02"PRIx32";\n",
                  led->groups);
    }

    if (led->which_mods) {
        if (led->which_mods != XKB_STATE_MODS_EFFECTIVE || explicit) {
            write_buf(buf, "\t\twhichModState= %s;\n",
                      LedStateMaskText(keymap->ctx, modComponentMaskNames,
                                       led->which_mods));
        }
        write_buf(buf, "\t\tmodifiers= %s;\n",
                  ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods,
                              led->mods.mods));
    }

    if (led->ctrls) {
        write_buf(buf, "\t\tcontrols= %s;\n",
                  ControlMaskText(keymap->ctx, led->ctrls));
    }

    copy_to_buf(buf, "\t};\n");
    return true;
}

static const char *
affect_lock_text(enum xkb_action_flags flags, bool show_both)
{
    switch (flags & (ACTION_LOCK_NO_LOCK | ACTION_LOCK_NO_UNLOCK)) {
    case 0:
        return show_both ? ",affect=both" : "";
    case ACTION_LOCK_NO_UNLOCK:
        return ",affect=lock";
    case ACTION_LOCK_NO_LOCK:
        return ",affect=unlock";
    case ACTION_LOCK_NO_LOCK | ACTION_LOCK_NO_UNLOCK:
        return ",affect=neither";
    default:
        return "";
    }
}

#define SYMBOL_PADDING 15
#define ACTION_PADDING 30

static bool
write_action(struct xkb_keymap *keymap, enum xkb_keymap_format format,
             xkb_layout_index_t max_groups, struct buf *buf,
             const union xkb_action *action,
             const char *prefix, const char *suffix)
{
    const char *type;
    const char *args = NULL;

    if (!prefix)
        prefix = "";
    if (!suffix)
        suffix = "";

    type = ActionTypeText(action->type);

    switch (action->type) {
    case ACTION_TYPE_MOD_SET:
    case ACTION_TYPE_MOD_LATCH:
    case ACTION_TYPE_MOD_LOCK:
        if (action->mods.flags & ACTION_MODS_LOOKUP_MODMAP)
            args = "modMapMods";
        else
            args = ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods,
                               action->mods.mods.mods);
        bool unlockOnPress = (action->mods.flags & ACTION_UNLOCK_ON_PRESS);
        if (unlockOnPress && !isModsUnLockOnPressSupported(format)) {
            log_err(keymap->ctx, XKB_ERROR_INCOMPATIBLE_KEYMAP_TEXT_FORMAT,
                    "Cannot use \"%s(unlockOnPress=true)\" in keymap format %d\n",
                    ActionTypeText(action->type), format);
            unlockOnPress = false;
        }
        bool latchOnPress = action->type == ACTION_TYPE_MOD_LATCH &&
                            (action->group.flags & ACTION_LATCH_ON_PRESS);
        if (latchOnPress && !isModsLatchOnPressSupported(format)) {
            log_err(keymap->ctx, XKB_ERROR_INCOMPATIBLE_KEYMAP_TEXT_FORMAT,
                    "Cannot use \"LatchMods(latchOnPress=true)\" "
                    "in keymap format %d\n", format);
            latchOnPress = false;
        }
        write_buf(buf, "%s%s(modifiers=%s%s%s%s%s%s)%s", prefix, type, args,
                  (action->type != ACTION_TYPE_MOD_LOCK && (action->mods.flags & ACTION_LOCK_CLEAR)) ? ",clearLocks" : "",
                  (action->type == ACTION_TYPE_MOD_LATCH && (action->mods.flags & ACTION_LATCH_TO_LOCK)) ? ",latchToLock" : "",
                  (action->type == ACTION_TYPE_MOD_LOCK) ? affect_lock_text(action->mods.flags, false) : "",
                  (unlockOnPress) ? ",unlockOnPress" : "",
                  (latchOnPress) ? ",latchOnPress" : "",
                  suffix);
        break;

    case ACTION_TYPE_GROUP_SET:
    case ACTION_TYPE_GROUP_LATCH:
    case ACTION_TYPE_GROUP_LOCK:
        if ((uint32_t) xkb_abs(action->group.group) <
            (max_groups + !(action->group.flags & ACTION_ABSOLUTE_SWITCH))) {
            bool lockOnRelease = action->type == ACTION_TYPE_GROUP_LOCK &&
                                 (action->group.flags & ACTION_LOCK_ON_RELEASE);
            if (lockOnRelease && !isGroupLockOnReleaseSupported(format)) {
                log_err(keymap->ctx, XKB_ERROR_INCOMPATIBLE_KEYMAP_TEXT_FORMAT,
                        "Cannot use \"GroupLock(lockOnRelease=true)\" "
                        "in keymap format %d\n", format);
                lockOnRelease = false;
            }
            write_buf(buf, "%s%s(group=%s%"PRId32"%s%s%s)%s", prefix, type,
                      (!(action->group.flags & ACTION_ABSOLUTE_SWITCH) && action->group.group >= 0) ? "+" : "",
                      (action->group.flags & ACTION_ABSOLUTE_SWITCH) ? action->group.group + 1 : action->group.group,
                      (action->type != ACTION_TYPE_GROUP_LOCK && (action->group.flags & ACTION_LOCK_CLEAR)) ? ",clearLocks" : "",
                      (action->type == ACTION_TYPE_GROUP_LATCH && (action->group.flags & ACTION_LATCH_TO_LOCK)) ? ",latchToLock" : "",
                      (lockOnRelease) ? ",lockOnRelease" : "",
                      suffix);
        } else {
            /* Unsupported group index: degrade to VoidAction() */
            goto void_action;
        }
        break;

    case ACTION_TYPE_TERMINATE:
        write_buf(buf, "%s%s()%s", prefix, type, suffix);
        break;

    case ACTION_TYPE_PTR_MOVE:
        write_buf(buf, "%s%s(x=%s%d,y=%s%d%s)%s", prefix, type,
                  (!(action->ptr.flags & ACTION_ABSOLUTE_X) && action->ptr.x >= 0) ? "+" : "",
                  action->ptr.x,
                  (!(action->ptr.flags & ACTION_ABSOLUTE_Y) && action->ptr.y >= 0) ? "+" : "",
                  action->ptr.y,
                  (action->ptr.flags & ACTION_ACCEL) ? "" : ",!accel",
                  suffix);
        break;

    case ACTION_TYPE_PTR_LOCK:
        args = affect_lock_text(action->btn.flags, true);
        /* fallthrough */
    case ACTION_TYPE_PTR_BUTTON:
        write_buf(buf, "%s%s(button=", prefix, type);
        if (action->btn.button > 0 && action->btn.button <= 5)
            write_buf(buf, "%u", action->btn.button);
        else
            copy_to_buf(buf, "default");
        if (action->btn.count)
            write_buf(buf, ",count=%u", action->btn.count);
        if (args)
            write_buf(buf, "%s", args);
        write_buf(buf, ")%s", suffix);
        break;

    case ACTION_TYPE_PTR_DEFAULT:
        write_buf(buf, "%s%s(", prefix, type);
        write_buf(buf, "affect=button,button=%s%d",
                  (!(action->dflt.flags & ACTION_ABSOLUTE_SWITCH) && action->dflt.value >= 0) ? "+" : "",
                  action->dflt.value);
        write_buf(buf, ")%s", suffix);
        break;

    case ACTION_TYPE_SWITCH_VT:
        write_buf(buf, "%s%s(screen=%s%d,%ssame)%s", prefix, type,
                  (!(action->screen.flags & ACTION_ABSOLUTE_SWITCH) && action->screen.screen >= 0) ? "+" : "",
                  action->screen.screen,
                  (action->screen.flags & ACTION_SAME_SCREEN) ? "" : "!",
                  suffix);
        break;

    case ACTION_TYPE_CTRL_SET:
    case ACTION_TYPE_CTRL_LOCK:
        write_buf(buf, "%s%s(controls=%s%s)%s", prefix, type,
                  ControlMaskText(keymap->ctx, action->ctrls.ctrls),
                  (action->type == ACTION_TYPE_CTRL_LOCK) ? affect_lock_text(action->ctrls.flags, false) : "",
                  suffix);
        break;

    case ACTION_TYPE_REDIRECT_KEY: {
        write_buf(buf, "%s%s(", prefix, type);
        const struct xkb_key * const key = XkbKey(keymap, action->redirect.keycode);
        /* Can fail if the keycode was not initialized */
        if (key)
            write_buf(buf, "keycode=%s", KeyNameText(keymap->ctx, key->name));
        if (action->redirect.affect) {
            xkb_mod_mask_t mask;
            mask = (action->redirect.affect & action->redirect.mods);
            if (mask)
                write_buf(buf, ",modifiers=%s",
                          ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods, mask));
            mask = (action->redirect.affect & ~action->redirect.mods);
            if (mask)
                write_buf(buf, ",clearMods=%s",
                          ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods, mask));
        }
        write_buf(buf, ")%s", suffix);
        break;
    }

    case ACTION_TYPE_UNKNOWN:
    case ACTION_TYPE_NONE:
        write_buf(buf, "%sNoAction()%s", prefix, suffix);
        break;

    case ACTION_TYPE_VOID:
void_action:
        /*
         * VoidAction() is a libxkbcommon extension.
         * Use LockControls as a backward-compatible fallback.
         * We cannot serialize it to `NoAction()`, as it would be dropped in
         * e.g. the context of multiple actions.
         * We better not use `Private` either, because it could still be
         * interpreted by X11.
         */
        if (format == XKB_KEYMAP_FORMAT_TEXT_V1)
            write_buf(buf, "%sLockControls(controls=none,affect=neither)%s",
                      prefix, suffix);
        else
            write_buf(buf, "%sVoidAction()%s", prefix, suffix);
        break;

    default:
        {} /* Label followed by declaration requires C23 */
        /* Ensure to not miss `xkb_action_type` updates */
        static_assert(ACTION_TYPE_INTERNAL == 20 &&
                      ACTION_TYPE_INTERNAL + 1 == _ACTION_TYPE_NUM_ENTRIES,
                      "Missing action type");
        /* Unsupported legacy actions should have degraded to NoAction */
        assert(action->type != ACTION_TYPE_UNSUPPORTED_LEGACY);
        write_buf(buf,
                  "%s%s(type=0x%02x,data[0]=0x%02x,data[1]=0x%02x,data[2]=0x%02x,data[3]=0x%02x,data[4]=0x%02x,data[5]=0x%02x,data[6]=0x%02x)%s",
                  prefix, type, action->type, action->priv.data[0],
                  action->priv.data[1], action->priv.data[2],
                  action->priv.data[3], action->priv.data[4],
                  action->priv.data[5], action->priv.data[6],
                  suffix);
        break;
    }

    return true;
}

static const union xkb_action void_actions[] = {
    { .type = ACTION_TYPE_VOID }
};

static bool
write_actions(struct xkb_keymap *keymap, enum xkb_keymap_format format,
              xkb_layout_index_t max_groups, struct buf *buf, struct buf *buf2,
              const struct xkb_key *key, xkb_layout_index_t group)
{
    static const union xkb_action noAction = { .type = ACTION_TYPE_NONE };

    for (xkb_level_index_t level = 0; level < XkbKeyNumLevels(key, group);
         level++) {

        if (level != 0)
            copy_to_buf(buf, ", ");

        const union xkb_action *actions = NULL;
        xkb_action_count_t count = xkb_keymap_key_get_actions_by_level(
            keymap, key, group, level, &actions
        );

        if (count > 1 && format < XKB_KEYMAP_FORMAT_TEXT_V2) {
            /* V1: Degrade multiple actions to VoidAction() */
            actions = void_actions;
            count = 1;
            log_err(keymap->ctx, XKB_ERROR_INCOMPATIBLE_KEYMAP_TEXT_FORMAT,
                    "Cannot serialize multiple actions per level "
                    "in keymap format %d; degrade to VoidAction()\n", format);
        }

        buf2->size = 0;
        if (count == 0) {
            if (!write_action(keymap, format, max_groups,
                              buf2, &noAction, NULL, NULL))
                return false;
            write_buf(buf, "%*s", ACTION_PADDING, buf2->buf);
        }
        else if (count == 1) {
            if (!write_action(keymap, format, max_groups,
                              buf2, &(actions[0]), NULL, NULL))
                return false;
            write_buf(buf, "%*s", ACTION_PADDING, buf2->buf);
        }
        else {
            copy_to_buf(buf2, "{ ");
            for (xkb_action_count_t k = 0; k < count; k++) {
                if (k != 0)
                    copy_to_buf(buf2, ", ");
                size_t old_size = buf2->size;
                if (!write_action(keymap, format, max_groups,
                                  buf2, &(actions[k]), NULL, NULL))
                    return false;
                /* Check if padding is necessary */
                if (buf2->size >= old_size + ACTION_PADDING)
                    continue;
                /* Compute and write padding, then write the action again */
                const int padding = (int)(old_size + ACTION_PADDING - buf2->size);
                buf2->size = old_size;
                write_buf(buf2, "%*s", padding, "");
                if (!write_action(keymap, format, max_groups,
                                  buf2, &(actions[k]), NULL, NULL))
                    return false;
            }
            copy_to_buf(buf2, " }");
            write_buf(buf, "%*s", ACTION_PADDING, buf2->buf);
        }
    }

    return true;
}

static bool
write_action_defaults(const struct xkb_keymap *keymap,
                      enum xkb_keymap_format format,
                      const union xkb_action *action,
                      struct buf *buf)
{
    const char *type = ActionTypeText(action->type);

#define PREFIX "\t"

    switch (action->type) {
    case ACTION_TYPE_MOD_SET:
    case ACTION_TYPE_MOD_LATCH:
    case ACTION_TYPE_MOD_LOCK:
        assert(action->mods.flags == 0);
        assert(action->mods.mods.mods == 0);
        write_buf(buf, PREFIX"%s.modifiers = 0;\n", type);
        if (action->type != ACTION_TYPE_MOD_LOCK)
            write_buf(buf, PREFIX"%s.clearLocks = false;\n", type);
        if (action->type == ACTION_TYPE_MOD_LATCH &&
            isModsLatchOnPressSupported(format))
            write_buf(buf, PREFIX"%s.latchOnPress = false;\n", type);
        if (action->type == ACTION_TYPE_MOD_LATCH)
            write_buf(buf, PREFIX"%s.latchToLock = false;\n", type);
        if (action->type == ACTION_TYPE_MOD_LOCK)
            write_buf(buf, PREFIX"%s.affect = both;\n", type);
        if (isModsUnLockOnPressSupported(format))
            write_buf(buf, PREFIX"%s.unlockOnPress = false;\n", type);
        break;

    case ACTION_TYPE_GROUP_SET:
    case ACTION_TYPE_GROUP_LATCH:
    case ACTION_TYPE_GROUP_LOCK:
        assert(action->group.flags == 0);
        assert(action->group.group == 0);
        /* Explicit sign to avoid setting ACTION_ABSOLUTE_SWITCH */
        write_buf(buf, PREFIX"%s.group = +0;\n", type);
        if (action->type != ACTION_TYPE_GROUP_LOCK)
            write_buf(buf, PREFIX"%s.clearLocks = false;\n", type);
        if (action->type == ACTION_TYPE_GROUP_LATCH)
            write_buf(buf, PREFIX"%s.latchToLock = false;\n", type);
        if (action->type == ACTION_TYPE_GROUP_LOCK &&
            isGroupLockOnReleaseSupported(format))
            write_buf(buf, PREFIX"%s.lockOnRelease = false;\n", type);
        break;


    case ACTION_TYPE_PTR_MOVE:
        assert(action->ptr.x == 0);
        assert(action->ptr.y == 0);
        assert(action->ptr.flags == ACTION_ACCEL);
        /* Explicit sign to avoid setting ACTION_ABSOLUTE_SWITCH */
        write_buf(buf, PREFIX"%s.x = +0;\n", type);
        write_buf(buf, PREFIX"%s.y = +0;\n", type);
        write_buf(buf, PREFIX"%s.accel = true;\n", type);
        break;

    case ACTION_TYPE_PTR_LOCK:
    case ACTION_TYPE_PTR_BUTTON:
        assert(action->btn.flags == 0);
        assert(action->btn.button == 0);
        assert(action->btn.count == 0);
        write_buf(buf, PREFIX"%s.button = 0;\n", type);
        write_buf(buf, PREFIX"%s.count = 0;\n", type);
        if (action->type == ACTION_TYPE_PTR_LOCK)
            write_buf(buf, PREFIX"%s.affect = both;\n", type);
        break;

    case ACTION_TYPE_PTR_DEFAULT:
        assert(action->dflt.flags == 0);
        assert(action->dflt.value == 1);
        /* Explicit sign to avoid setting ACTION_ABSOLUTE_SWITCH */
        write_buf(buf, PREFIX"%s.button = +1;\n", type);
        break;

    case ACTION_TYPE_SWITCH_VT:
        assert(action->screen.flags == ACTION_SAME_SCREEN);
        assert(action->screen.screen == 0);
        /* Explicit sign to avoid setting ACTION_ABSOLUTE_SWITCH */
        write_buf(buf, PREFIX"%s.screen = +0;\n", type);
        write_buf(buf, PREFIX"%s.same = true;\n", type);
        break;

    case ACTION_TYPE_CTRL_SET:
    case ACTION_TYPE_CTRL_LOCK:
        assert(action->ctrls.flags == 0);
        assert(action->ctrls.ctrls == 0);
        write_buf(buf, PREFIX"%s.controls = 0;\n", type);
        if (action->type == ACTION_TYPE_CTRL_LOCK)
            write_buf(buf, PREFIX"%s.affect = both;\n", type);
        break;

    case ACTION_TYPE_REDIRECT_KEY:
        assert(action->redirect.keycode == keymap->redirect_key_auto);
        assert(action->redirect.affect == 0);
        assert(action->redirect.mods == 0);
        write_buf(buf, PREFIX"%s.keycode = auto;\n", type);
        write_buf(buf, PREFIX"%s.modifiers = 0;\n", type);
        write_buf(buf, PREFIX"%s.clearMods = 0;\n", type);
        break;

    default:
        {} /* Label followed by declaration requires C23 */
        /* Ensure to not miss `xkb_action_type` updates */
        static_assert(ACTION_TYPE_INTERNAL == 20 &&
                      ACTION_TYPE_INTERNAL + 1 == _ACTION_TYPE_NUM_ENTRIES,
                      "Missing action type");
    }

#undef PREFIX

    return true;
}

static bool
write_actions_defaults(const struct xkb_keymap *keymap,
                       enum xkb_keymap_format format, struct buf *buf)
{
    ActionsInfo defaults = {0};
    InitActionsInfo(keymap, &defaults);

    for (uint8_t a = 0; a < (uint8_t) _ACTION_TYPE_NUM_ENTRIES; a++) {
        const union xkb_action * restrict const action = &defaults.actions[a];
        if (!write_action_defaults(keymap, format, action, buf))
            return false;
    }
    copy_to_buf(buf, "\n");

    return true;
}

static const struct xkb_sym_interpret fallback_interpret = {
    .sym = XKB_KEY_VoidSymbol,
    .required = true,
    .repeat = FALLBACK_INTERPRET_KEY_REPEAT,
    .match = MATCH_ANY_OR_NONE,
    .mods = 0,
    .virtual_mod = (xkb_mod_index_t) DEFAULT_INTERPRET_VMOD,
    .num_actions = 0,
    .a = { .action = { .type = ACTION_TYPE_NONE } },
};

static bool
write_compat(struct xkb_keymap * restrict keymap,
             enum xkb_keymap_format format, xkb_layout_index_t max_groups,
             enum xkb_keymap_serialize_flags flags, bool * restrict some_interp,
             struct buf *buf)
{
    const bool pretty = !!(flags & XKB_KEYMAP_SERIALIZE_PRETTY);
    const bool explicit = !!(flags & XKB_KEYMAP_SERIALIZE_EXPLICIT);
    const bool drop_unused = !(flags & XKB_KEYMAP_SERIALIZE_KEEP_UNUSED);
    const bool drop_interprets = (explicit && drop_unused);
    /* TODO: print all default values if explicit flag is set */

    if (keymap->compat_section_name)
        write_buf(buf, "xkb_compatibility \"%s\" {\n",
                  keymap->compat_section_name);
    else
        copy_to_buf(buf, "xkb_compatibility {\n");

    if (!write_vmods(keymap, format, explicit, buf))
        return false;

    if (explicit && !write_actions_defaults(keymap, format, buf))
        return false;

    copy_to_buf(buf, "\tinterpret.useModMapMods= AnyLevel;\n");
    copy_to_buf(buf, "\tinterpret.repeat= False;\n");

    /* xkbcomp requires at least one interpret entry. */
    const bool use_fallback_interpret =
        (!keymap->num_sym_interprets || drop_interprets);
    const darray_size_t num_sym_interprets =
        (use_fallback_interpret ? 1 : keymap->num_sym_interprets);
    const struct xkb_sym_interpret* const sym_interprets =
        (use_fallback_interpret ? &fallback_interpret : keymap->sym_interprets);

    for (darray_size_t i = 0; i < num_sym_interprets; i++) {
        const struct xkb_sym_interpret *si = &sym_interprets[i];
        if (!si->required && drop_unused)
            continue;

        *some_interp = true;

        copy_to_buf(buf, "\tinterpret ");
        if (!si->sym) {
            copy_to_buf(buf, "Any");
        } else if (pretty) {
            write_buf(buf, "%s", KeysymText(keymap->ctx, si->sym));
        } else {
            write_buf(buf, "0x%"PRIx32, si->sym);
        }
        write_buf(buf, "+%s(%s) {",
                  SIMatchText(si->match),
                  ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods, si->mods));

        bool has_explicit_properties = false;

        if (si->virtual_mod != XKB_MOD_INVALID) {
            write_buf(buf, "\n\t\tvirtualModifier= %s;",
                      ModIndexText(keymap->ctx, &keymap->mods,
                                   si->virtual_mod));
            has_explicit_properties = true;
        }

        if (si->level_one_only) {
            copy_to_buf(buf, "\n\t\tuseModMapMods=level1;");
            has_explicit_properties = true;
        } else if (explicit) {
            copy_to_buf(buf, "\n\t\tuseModMapMods=AnyLevel;");
            has_explicit_properties = true;
        }

        if (si->repeat) {
            copy_to_buf(buf, "\n\t\trepeat= True;");
            has_explicit_properties = true;
        } else if (explicit) {
            copy_to_buf(buf, "\n\t\trepeat= False;");
            has_explicit_properties = true;
        }

        const union xkb_action *action = NULL;
        xkb_action_count_t action_count = si->num_actions;
        if (action_count <= 1) {
            action = &si->a.action;
        } else if (format < XKB_KEYMAP_FORMAT_TEXT_V2) {
            /* V1: Degrade multiple actions to VoidAction() */
            action = &void_actions[0];
            action_count = 1;
            log_err(keymap->ctx, XKB_ERROR_INCOMPATIBLE_KEYMAP_TEXT_FORMAT,
                    "Cannot serialize multiple actions per level "
                    "in keymap format %d; degrade to VoidAction()\n", format);
        }

        if (action_count > 1) {
            copy_to_buf(buf, "\n\t\taction= {");
            const char suffix[] = ", ";
            for (xkb_action_count_t k = 0; k < si->num_actions; k++) {
                if (!write_action(keymap, format, max_groups,
                                  buf, &si->a.actions[k], "", suffix))
                    return false;
            }
            buf->size -= sizeof(suffix) - 1; /* trailing comma */
            copy_to_buf(buf, "};");
            has_explicit_properties = true;
        } else if (action_count == 1) {
            if (!write_action(keymap, format, max_groups, buf, action,
                              "\n\t\taction= ", ";"))
                return false;
            has_explicit_properties = true;
        }
        write_buf(buf, (has_explicit_properties
                        ? "\n\t};\n"
                        /* Empty interpret is a syntax error in xkbcomp, so
                         * use a dummy entry */
                        : "\n\t\taction= NoAction();\n\t};\n"));
    }

    if (use_fallback_interpret || (
        /* Detect previous interpret fallback */
        num_sym_interprets == 1 && *some_interp &&
        sym_interprets[0].sym == fallback_interpret.sym &&
        sym_interprets[0].match == fallback_interpret.match &&
        sym_interprets[0].mods == fallback_interpret.mods &&
        sym_interprets[0].repeat == fallback_interpret.repeat &&
        sym_interprets[0].virtual_mod == fallback_interpret.virtual_mod &&
        sym_interprets[0].level_one_only == fallback_interpret.level_one_only &&
        sym_interprets[0].num_actions == fallback_interpret.num_actions
        /* `required` field is not compared */
    ))
        *some_interp = false;

    const struct xkb_led *led;
    xkb_leds_foreach(led, keymap)
        if (led->which_groups || led->groups || led->which_mods ||
            led->mods.mods || led->ctrls)
            if (!write_led_map(keymap, explicit, buf, led))
                return false;

    copy_to_buf(buf, "};\n\n");

    return true;
}

static bool
write_keysyms(struct xkb_keymap *keymap, struct buf *buf, struct buf *buf2,
              const struct xkb_key *key, xkb_layout_index_t group,
              bool pretty, bool show_actions)
{
    const unsigned int padding = (pretty)
        ? ((show_actions) ? ACTION_PADDING : SYMBOL_PADDING)
        : 0;
    for (xkb_level_index_t level = 0; level < XkbKeyNumLevels(key, group);
         level++) {
        const xkb_keysym_t *syms;
        int num_syms;

        if (level != 0)
            copy_to_buf(buf, ", ");

        num_syms = xkb_keymap_key_get_syms_by_level(keymap, key->keycode,
                                                    group, level, &syms);

        const xkb_keysym_t no_symbol = XKB_KEY_NoSymbol;
        if (num_syms == 0) {
            num_syms = 1;
            syms = &no_symbol;
        }

        /*
         * NOTE: Use `NoSymbol` even without pretty output, for compatibility
         * with xkbcomp and libxkbcommon < 1.12
         */

        if (num_syms == 1) {
            if (pretty || syms[0] == XKB_KEY_NoSymbol)
                write_buf(buf, "%*s", padding, KeysymText(keymap->ctx, syms[0]));
            else
                write_buf(buf, "0x%"PRIx32, syms[0]);
        } else {
            if (pretty) {
                buf2->size = 0;
                copy_to_buf(buf2, "{ ");
                for (int s = 0; s < num_syms; s++) {
                    if (s != 0)
                        copy_to_buf(buf2, ", ");
                    write_buf(buf2, "%*s", (show_actions ? padding : 0),
                              KeysymText(keymap->ctx, syms[s]));
                }
                copy_to_buf(buf2, " }");
                write_buf(buf, "%*s", padding, buf2->buf);
            } else {
                copy_to_buf(buf, "{");
                for (int s = 0; s < num_syms; s++) {
                    if (s != 0)
                        copy_to_buf(buf, ",");
                    if (syms[s] == XKB_KEY_NoSymbol)
                        copy_to_buf(buf, "NoSymbol");
                    else
                        write_buf(buf, "0x%"PRIx32, syms[s]);
                }
                copy_to_buf(buf, "}");
            }
        }
    }

    return true;
}

static bool
write_key(struct xkb_keymap *keymap, enum xkb_keymap_format format,
          const key_name_substitutions *substitutions,
          xkb_layout_index_t max_groups, bool some_interprets,
          bool drop_interprets, bool explicit, bool pretty,
          struct buf *buf, struct buf *buf2, const struct xkb_key *key)
{
    bool simple = true;

    const xkb_layout_index_t num_groups = MIN(key->num_groups, max_groups);

    const xkb_atom_t name = (substitutions == NULL)
        ? key->name
        : substitute_name(substitutions, key->name);

    if (pretty)
        write_buf(buf, "\tkey %-20s {", KeyNameText(keymap->ctx, name));
    else
        write_buf(buf, "\tkey %s {", KeyNameText(keymap->ctx, name));

    if (key->explicit & EXPLICIT_TYPES || explicit) {
        simple = false;

        bool multi_type = false;
        for (xkb_layout_index_t group = 1; group < num_groups; group++) {
            if (key->groups[group].type != key->groups[0].type) {
                multi_type = true;
                break;
            }
        }

        if (multi_type) {
            for (xkb_layout_index_t group = 0; group < num_groups; group++) {
                if (!key->groups[group].explicit_type && !explicit)
                    continue;

                const struct xkb_key_type * const type = key->groups[group].type;
                write_buf(buf, "\n\t\ttype[%"PRIu32"]= ", group + 1);
                write_buf_string_literal(
                  buf, xkb_atom_text(keymap->ctx, type->name));
                copy_to_buf(buf, ",");
            }
        }
        else {
            const struct xkb_key_type * const type = key->groups[0].type;
            write_buf(buf, "\n\t\ttype= ");
            write_buf_string_literal(
                buf, xkb_atom_text(keymap->ctx, type->name));
            copy_to_buf(buf, ",");
        }
    }

    /*
     * NOTE: we use key->explicit and not key->group[i].explicit_actions, in
     * order to have X11 and the previous versions of libxkbcommon (without this
     * group property) parse the keymap as intended, by setting explicitly for
     * this key all actions in all groups.
     *
     * One side effect is that no interpretation will be run on this key anymore,
     * so we may have to set some extra fields explicitly: repeat, virtualMods.
     */
    const bool explicit_actions = (key->explicit & EXPLICIT_INTERP);

    /* Check if an explicit value is required */
    #define require_explicit(key, prop,                                       \
                             value, initial_value, default_interpret_value) ( \
        /* Explicit value in the source */                                    \
        ((key)->explicit & (prop)) ||                                         \
        /* Explicit serialization flag */                                     \
        explicit ||                                                           \
        (                                                                     \
            (                                                                 \
                /* Value is not set by interprets due to explicit actions */  \
                explicit_actions ||                                           \
                /* Value is set by an interpret, but no interpret remains:    \
                 * print the value if it is not the default interpret value,  \
                 * unless it cannot trigger due to implicit actions forcibly  \
                 * made explicit */                                           \
                ((key)->implicit_actions && !some_interprets)                 \
            ) && (                                                            \
                /* Non-initial value */                                       \
                (value) != (initial_value) ||                                 \
                /* Force initial value for clarity */                         \
                (initial_value) != (default_interpret_value)                  \
            )                                                                 \
        )                                                                     \
    )

    if (require_explicit(key, EXPLICIT_REPEAT, key->repeats,
                         DEFAULT_KEY_REPEAT, DEFAULT_INTERPRET_KEY_REPEAT)) {
        if (key->repeats)
            copy_to_buf(buf, "\n\t\trepeat= Yes,");
        else
            copy_to_buf(buf, "\n\t\trepeat= No,");
        simple = false;
    }

    if (require_explicit(key, EXPLICIT_VMODMAP, key->vmodmap,
                         (xkb_mod_mask_t) DEFAULT_KEY_VMODMAP,
                         (xkb_mod_mask_t) DEFAULT_INTERPRET_VMODMAP)) {
        write_buf(buf, "\n\t\tvirtualMods= %s,",
                  ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods,
                              key->vmodmap));
        simple = false;
    }

    #undef require_explicit

    switch (key->out_of_range_group_action) {
    case RANGE_SATURATE:
        copy_to_buf(buf, "\n\t\tgroupsClamp,");
        simple = false;
        break;

    case RANGE_REDIRECT:
        if (key->out_of_range_group_number < num_groups) {
            /* TODO: Fallback or warning if condition fails? */
            write_buf(buf, "\n\t\tgroupsRedirect= %"PRIu32",",
                      key->out_of_range_group_number + 1);
            simple = false;
        }
        break;

    default:
        break;
    }

    if (num_groups > 1 || explicit_actions || explicit)
        simple = false;

    if (simple) {
        const bool only_symbols = key->explicit == EXPLICIT_SYMBOLS;
        if (num_groups == 0) {
            /* Remove trailing comma */
            if (buf->buf[buf->size - 1] == ',')
                buf->size--;
        } else {
	        if (!only_symbols)
	            copy_to_buf(buf, "\n\t");
            copy_to_buf(buf, "\t[ ");
            if (!write_keysyms(keymap, buf, buf2, key, 0, pretty, false))
                return false;
            copy_to_buf(buf, " ]");
        }
        write_buf(buf, "%s", (only_symbols ? " };\n" : "\n\t};\n"));
    }
    else {
        for (xkb_layout_index_t group = 0; group < num_groups; group++) {
            const bool print_actions =
                /* Group has explicit actions */
                key->groups[group].explicit_actions ||
                /* Explicit values are required */
                explicit ||
                /* Group has symbols but no explicit actions and key has explicit
                 * actions in another group: ensure compatibility with xkbcomp
                 * and all libxkbcommon versions */
                (key->groups[group].explicit_symbols &&
                 explicit_actions && some_interprets) ||
                /* Group has implicit actions but no interpret can set them, so
                 * the actions must be made explicit */
                (key->groups[group].implicit_actions && !some_interprets);

            if (group != 0)
                copy_to_buf(buf, ",");
            write_buf(buf, "\n\t\tsymbols[%"PRIu32"]= [ ", group + 1);

            if (!write_keysyms(keymap, buf, buf2, key, group,
                               pretty, print_actions))
                return false;
            copy_to_buf(buf, " ]");
            if (print_actions) {
                write_buf(buf, ",\n\t\tactions[%"PRIu32"]= [ ", group + 1);
                if (!write_actions(keymap, format, max_groups,
                                   buf, buf2, key, group))
                    return false;
                copy_to_buf(buf, " ]");
            }
        }
        if (!num_groups)
            delete_last_char(buf, ',');
        copy_to_buf(buf, "\n\t};\n");
    }

    return true;
}

static bool
write_symbols(struct xkb_keymap *keymap, enum xkb_keymap_format format,
              const key_name_substitutions *substitutions,
              xkb_layout_index_t max_groups,
              enum xkb_keymap_serialize_flags flags, bool some_interp,
              struct buf *buf)
{
    const bool pretty = !!(flags & XKB_KEYMAP_SERIALIZE_PRETTY);
    const bool drop_unused = !(flags & XKB_KEYMAP_SERIALIZE_KEEP_UNUSED);
    const bool drop_interprets =
        ((flags & XKB_KEYMAP_SERIALIZE_EXPLICIT) && drop_unused);
    const bool explicit = !!(flags & XKB_KEYMAP_SERIALIZE_EXPLICIT);
    /* TODO: print all default values if explicit flag is set */

    if (keymap->symbols_section_name)
        write_buf(buf, "xkb_symbols \"%s\" {\n", keymap->symbols_section_name);
    else
        copy_to_buf(buf, "xkb_symbols {\n");

    const xkb_layout_index_t num_group_names = MIN(keymap->num_group_names,
                                                   max_groups);
    bool has_group_names = false;
    for (xkb_layout_index_t group = 0; group < num_group_names; group++)
        if (keymap->group_names[group]) {
            write_buf(buf, "\tname[%"PRIu32"]=", group + 1);
            write_buf_string_literal(
                buf, xkb_atom_text(keymap->ctx, keymap->group_names[group]));
            copy_to_buf(buf, ";\n");
            has_group_names = true;
        }
    if (has_group_names)
        copy_to_buf(buf, "\n");

    if (explicit && !write_actions_defaults(keymap, format, buf))
        return false;

    struct buf buf2 = { NULL, 0, 0 };
    const struct xkb_key *key;
    xkb_keys_foreach(key, keymap) {
        /* Skip keys with no explicit values */
        if (key->explicit) {
            if (!write_key(keymap, format, substitutions, max_groups,
                           some_interp, drop_interprets, explicit,
                           pretty, buf, &buf2, key)) {
                free(buf2.buf);
                return false;
            }
        }
    }
    free(buf2.buf);

    xkb_mod_index_t i;
    const struct xkb_mod *mod;
    xkb_rmods_enumerate(i, mod, &keymap->mods) {
        bool had_any = false;
        xkb_keys_foreach(key, keymap) {
            if (key->modmap & (UINT32_C(1) << i)) {
                if (!had_any)
                    write_buf(buf, "\tmodifier_map %s { ",
                              xkb_atom_text(keymap->ctx, mod->name));
                const xkb_atom_t name = (substitutions == NULL)
                    ? key->name
                    : substitute_name(substitutions, key->name);
                write_buf(buf, "%s%s",
                          had_any ? ", " : "",
                          KeyNameText(keymap->ctx, name));
                had_any = true;
            }
        }
        if (had_any)
            copy_to_buf(buf, " };\n");
    }

    copy_to_buf(buf, "};\n\n");
    return true;
}

static bool
write_keymap(struct xkb_keymap *keymap, enum xkb_keymap_format format,
             enum xkb_keymap_serialize_flags flags, struct buf *buf)
{
    const xkb_layout_index_t max_groups = format_max_groups(format);
    if (keymap->num_groups > max_groups) {
        log_err(keymap->ctx, XKB_ERROR_UNSUPPORTED_GROUP_INDEX,
                "Cannot serialize %"PRIu32" groups in keymap format %d: "
                "maximum is %"PRIu32"; discarding unsupported groups\n",
                keymap->num_groups, format, max_groups);
    }

    key_name_substitutions substitutions = darray_new();
    if (format == XKB_KEYMAP_FORMAT_TEXT_V1) {
        if (!rename_long_keys(keymap, &substitutions))
            return false;
    }
    const key_name_substitutions * const substitutions_ptr =
        (darray_empty(substitutions)) ? NULL : &substitutions;

    bool some_interp = false;
    const bool ok = (
        check_write_buf(buf, "xkb_keymap {\n") &&
        write_keycodes(keymap, substitutions_ptr, flags, buf) &&
        write_types(keymap, format, flags, buf) &&
        write_compat(keymap, format, max_groups, flags, &some_interp, buf) &&
        write_symbols(keymap, format, substitutions_ptr, max_groups,
                      flags, some_interp, buf) &&
        check_write_buf(buf, "};\n")
    );

    darray_free(substitutions);
    return ok;
}

char *
text_v1_keymap_get_as_string(struct xkb_keymap *keymap,
                             enum xkb_keymap_format format,
                             enum xkb_keymap_serialize_flags flags)
{
    struct buf buf = { NULL, 0, 0 };

    if (!write_keymap(keymap, format, flags, &buf)) {
        free(buf.buf);
        return NULL;
    }

    return buf.buf;
}
