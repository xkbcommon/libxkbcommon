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

#include "keymap.h"
#include "xkbcomp-priv.h"
#include "text.h"

#define BUF_CHUNK_SIZE 4096

struct buf {
    char *buf;
    size_t size;
    size_t alloc;
};

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

static bool
write_vmods(struct xkb_keymap *keymap, struct buf *buf)
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
        if ((keymap->mods.explicit_vmods & (UINT32_C(1) << vmod)) &&
            mod->mapping != 0) {
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
write_keycodes(struct xkb_keymap *keymap, struct buf *buf)
{
    const struct xkb_key *key;
    xkb_led_index_t idx;
    const struct xkb_led *led;

    if (keymap->keycodes_section_name)
        write_buf(buf, "xkb_keycodes \"%s\" {\n",
                  keymap->keycodes_section_name);
    else
        copy_to_buf(buf, "xkb_keycodes {\n");

    /* xkbcomp and X11 really want to see keymaps with a minimum of 8, and
     * a maximum of at least 255, else XWayland really starts hating life.
     * If this is a problem and people really need strictly bounded keymaps,
     * we should probably control this with a flag. */
    write_buf(buf, "\tminimum = %u;\n", MIN(keymap->min_key_code, 8));
    write_buf(buf, "\tmaximum = %u;\n", MAX(keymap->max_key_code, 255));

    xkb_keys_foreach(key, keymap) {
        if (key->name == XKB_ATOM_NONE)
            continue;

        write_buf(buf, "\t%-20s = %u;\n",
                  KeyNameText(keymap->ctx, key->name), key->keycode);
    }

    xkb_leds_enumerate(idx, led, keymap)
        if (led->name != XKB_ATOM_NONE) {
            write_buf(buf, "\tindicator %u = ", idx + 1);
            write_buf_string_literal(buf, xkb_atom_text(keymap->ctx, led->name));
            copy_to_buf(buf, ";\n");
        }


    for (unsigned i = 0; i < keymap->num_key_aliases; i++)
        write_buf(buf, "\talias %-14s = %s;\n",
                  KeyNameText(keymap->ctx, keymap->key_aliases[i].alias),
                  KeyNameText(keymap->ctx, keymap->key_aliases[i].real));

    copy_to_buf(buf, "};\n\n");
    return true;
}

static bool
write_types(struct xkb_keymap *keymap, struct buf *buf)
{
    if (keymap->types_section_name)
        write_buf(buf, "xkb_types \"%s\" {\n",
                  keymap->types_section_name);
    else
        copy_to_buf(buf, "xkb_types {\n");

    if (!write_vmods(keymap, buf))
        return false;

    for (unsigned i = 0; i < keymap->num_types; i++) {
        const struct xkb_key_type *type = &keymap->types[i];

        copy_to_buf(buf, "\ttype ");
        write_buf_string_literal(buf, xkb_atom_text(keymap->ctx, type->name));
        copy_to_buf(buf, " {\n");

        write_buf(buf, "\t\tmodifiers= %s;\n",
                  ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods,
                              type->mods.mods));

        for (unsigned j = 0; j < type->num_entries; j++) {
            const char *str;
            const struct xkb_key_type_entry *entry = &type->entries[j];

            /*
             * Printing level 1 entries is redundant, it's the default,
             * unless there's preserve info.
             */
            if (entry->level == 0 && entry->preserve.mods == 0)
                continue;

            str = ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods,
                              entry->mods.mods);
            write_buf(buf, "\t\tmap[%s]= %u;\n",
                      str, entry->level + 1);

            if (entry->preserve.mods)
                write_buf(buf, "\t\tpreserve[%s]= %s;\n",
                          str, ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods,
                                           entry->preserve.mods));
        }

        for (xkb_level_index_t n = 0; n < type->num_level_names; n++)
            if (type->level_names[n]) {
                write_buf(buf, "\t\tlevel_name[%u]= ", n + 1);
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
write_led_map(struct xkb_keymap *keymap, struct buf *buf,
              const struct xkb_led *led)
{
    copy_to_buf(buf, "\tindicator ");
    write_buf_string_literal(buf, xkb_atom_text(keymap->ctx, led->name));
    copy_to_buf(buf, " {\n");

    if (led->which_groups) {
        if (led->which_groups != XKB_STATE_LAYOUT_EFFECTIVE) {
            write_buf(buf, "\t\twhichGroupState= %s;\n",
                      LedStateMaskText(keymap->ctx, groupComponentMaskNames,
                                       led->which_groups));
        }
        write_buf(buf, "\t\tgroups= 0x%02x;\n",
                  led->groups);
    }

    if (led->which_mods) {
        if (led->which_mods != XKB_STATE_MODS_EFFECTIVE) {
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
write_action(struct xkb_keymap *keymap, struct buf *buf,
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
        write_buf(buf, "%s%s(modifiers=%s%s%s%s)%s", prefix, type, args,
                  (action->type != ACTION_TYPE_MOD_LOCK && (action->mods.flags & ACTION_LOCK_CLEAR)) ? ",clearLocks" : "",
                  (action->type != ACTION_TYPE_MOD_LOCK && (action->mods.flags & ACTION_LATCH_TO_LOCK)) ? ",latchToLock" : "",
                  (action->type == ACTION_TYPE_MOD_LOCK) ? affect_lock_text(action->mods.flags, false) : "",
                  suffix);
        break;

    case ACTION_TYPE_GROUP_SET:
    case ACTION_TYPE_GROUP_LATCH:
    case ACTION_TYPE_GROUP_LOCK:
        write_buf(buf, "%s%s(group=%s%d%s%s)%s", prefix, type,
                  (!(action->group.flags & ACTION_ABSOLUTE_SWITCH) && action->group.group > 0) ? "+" : "",
                  (action->group.flags & ACTION_ABSOLUTE_SWITCH) ? action->group.group + 1 : action->group.group,
                  (action->type != ACTION_TYPE_GROUP_LOCK && (action->group.flags & ACTION_LOCK_CLEAR)) ? ",clearLocks" : "",
                  (action->type != ACTION_TYPE_GROUP_LOCK && (action->group.flags & ACTION_LATCH_TO_LOCK)) ? ",latchToLock" : "",
                  suffix);
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
            write_buf(buf, "%d", action->btn.button);
        else
            copy_to_buf(buf, "default");
        if (action->btn.count)
            write_buf(buf, ",count=%d", action->btn.count);
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

    case ACTION_TYPE_NONE:
        write_buf(buf, "%sNoAction()%s", prefix, suffix);
        break;

    case ACTION_TYPE_VOID:
        /*
         * VoidAction() is a libxkbcommon extension.
         * Use LockControls as a backward-compatible fallback.
         * We cannot serialize it to `NoAction()`, as it would be dropped in
         * e.g. the context of multiple actions.
         * We better not use `Private` either, because it could still be
         * interpreted by X11.
         */
        write_buf(buf, "%sLockControls(controls=none,affect=neither)%s", prefix, suffix);
        break;

    default:
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

static bool
write_actions(struct xkb_keymap *keymap, struct buf *buf, struct buf *buf2,
              const struct xkb_key *key, xkb_layout_index_t group)
{
    static const union xkb_action noAction = { .type = ACTION_TYPE_NONE };

    for (xkb_level_index_t level = 0; level < XkbKeyNumLevels(key, group);
         level++) {
        const union xkb_action *actions;
        unsigned int count;

        if (level != 0)
            copy_to_buf(buf, ", ");

        count = xkb_keymap_key_get_actions_by_level(keymap, key,
                                                    group, level, &actions);
        buf2->size = 0;
        if (count == 0) {
            if (!write_action(keymap, buf2, &noAction, NULL, NULL))
                return false;
            write_buf(buf, "%*s", ACTION_PADDING, buf2->buf);
        }
        else if (count == 1) {
            if (!write_action(keymap, buf2, &(actions[0]), NULL, NULL))
                return false;
            write_buf(buf, "%*s", ACTION_PADDING, buf2->buf);
        }
        else {
            copy_to_buf(buf2, "{ ");
            for (unsigned int k = 0; k < count; k++) {
                if (k != 0)
                    copy_to_buf(buf2, ", ");
                size_t old_size = buf2->size;
                if (!write_action(keymap, buf2, &(actions[k]), NULL, NULL))
                    return false;
                /* Check if padding is necessary */
                if (buf2->size >= old_size + ACTION_PADDING)
                    continue;
                /* Compute and write padding, then write the action again */
                const int padding = (int)(old_size + ACTION_PADDING - buf2->size);
                buf2->size = old_size;
                write_buf(buf2, "%*s", padding, "");
                if (!write_action(keymap, buf2, &(actions[k]), NULL, NULL))
                    return false;
            }
            copy_to_buf(buf2, " }");
            write_buf(buf, "%*s", ACTION_PADDING, buf2->buf);
        }
    }

    return true;
}

static bool
write_compat(struct xkb_keymap *keymap, struct buf *buf)
{
    const struct xkb_led *led;

    if (keymap->compat_section_name)
        write_buf(buf, "xkb_compatibility \"%s\" {\n",
                  keymap->compat_section_name);
    else
        copy_to_buf(buf, "xkb_compatibility {\n");

    if (!write_vmods(keymap, buf))
        return false;

    copy_to_buf(buf, "\tinterpret.useModMapMods= AnyLevel;\n");
    copy_to_buf(buf, "\tinterpret.repeat= False;\n");

    /* xkbcomp requires at least one interpret entry. */
    const unsigned int num_sym_interprets = keymap->num_sym_interprets
                                          ? keymap->num_sym_interprets
                                          : 1;
    const struct xkb_sym_interpret* const sym_interprets
        = keymap->num_sym_interprets
        ? keymap->sym_interprets
        : &default_interpret;

    for (unsigned i = 0; i < num_sym_interprets; i++) {
        const struct xkb_sym_interpret *si = &sym_interprets[i];

        write_buf(buf, "\tinterpret %s+%s(%s) {",
                  si->sym ? KeysymText(keymap->ctx, si->sym) : "Any",
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
        }

        if (si->repeat) {
            copy_to_buf(buf, "\n\t\trepeat= True;");
            has_explicit_properties = true;
        }

        if (si->num_actions > 1) {
            copy_to_buf(buf, "\n\t\taction= {");
            const char suffix[] = ", ";
            for (xkb_action_count_t k = 0; k < si->num_actions; k++) {
                if (!write_action(keymap, buf, &si->a.actions[k], "", suffix))
                    return false;
            }
            buf->size -= sizeof(suffix) - 1; /* trailing comma */
            copy_to_buf(buf, "};");
            has_explicit_properties = true;
        } else if (si->num_actions == 1) {
            if (!write_action(keymap, buf, &si->a.action,
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

    xkb_leds_foreach(led, keymap)
        if (led->which_groups || led->groups || led->which_mods ||
            led->mods.mods || led->ctrls)
            if (!write_led_map(keymap, buf, led))
                return false;

    copy_to_buf(buf, "};\n\n");

    return true;
}

static bool
write_keysyms(struct xkb_keymap *keymap, struct buf *buf, struct buf *buf2,
              const struct xkb_key *key, xkb_layout_index_t group,
              bool show_actions)
{
    unsigned int padding = show_actions ? ACTION_PADDING : SYMBOL_PADDING;
    for (xkb_level_index_t level = 0; level < XkbKeyNumLevels(key, group);
         level++) {
        const xkb_keysym_t *syms;
        int num_syms;

        if (level != 0)
            copy_to_buf(buf, ", ");

        num_syms = xkb_keymap_key_get_syms_by_level(keymap, key->keycode,
                                                    group, level, &syms);
        if (num_syms == 0) {
            write_buf(buf, "%*s", padding, "NoSymbol");
        }
        else if (num_syms == 1) {
            write_buf(buf, "%*s", padding, KeysymText(keymap->ctx, syms[0]));
        }
        else {
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
        }
    }

    return true;
}

static bool
write_key(struct xkb_keymap *keymap, struct buf *buf, struct buf *buf2,
          const struct xkb_key *key)
{
    xkb_layout_index_t group;
    bool simple = true;

    write_buf(buf, "\tkey %-20s {", KeyNameText(keymap->ctx, key->name));

    if (key->explicit & EXPLICIT_TYPES) {
        simple = false;

        bool multi_type = false;
        for (group = 1; group < key->num_groups; group++) {
            if (key->groups[group].type != key->groups[0].type) {
                multi_type = true;
                break;
            }
        }

        if (multi_type) {
            for (group = 0; group < key->num_groups; group++) {
                if (!key->groups[group].explicit_type)
                    continue;

                const struct xkb_key_type * const type = key->groups[group].type;
                /* TODO: This will require using integer indexes when > 4 */
                write_buf(buf, "\n\t\ttype[Group%u]= ", group + 1);
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
    const bool show_actions = (key->explicit & EXPLICIT_INTERP);

    /* If we show actions, interprets are not going to be used to set this
     * field, so make it explicit. */
    if ((key->explicit & EXPLICIT_REPEAT) || show_actions) {
        if (key->repeats)
            copy_to_buf(buf, "\n\t\trepeat= Yes,");
        else
            copy_to_buf(buf, "\n\t\trepeat= No,");
    }

    /* If we show actions, interprets are not going to be used to set this
     * field, so make it explicit. */
    if ((key->explicit & EXPLICIT_VMODMAP) || (show_actions && key->vmodmap))
        write_buf(buf, "\n\t\tvirtualMods= %s,",
                  ModMaskText(keymap->ctx, MOD_BOTH, &keymap->mods,
                              key->vmodmap));

    switch (key->out_of_range_group_action) {
    case RANGE_SATURATE:
        copy_to_buf(buf, "\n\t\tgroupsClamp,");
        break;

    case RANGE_REDIRECT:
        write_buf(buf, "\n\t\tgroupsRedirect= Group%u,",
                    key->out_of_range_group_number + 1);
        break;

    default:
        break;
    }

    if (key->num_groups > 1 || show_actions)
        simple = false;

    if (simple) {
        const bool only_symbols = key->explicit == EXPLICIT_SYMBOLS;
        if (key->num_groups == 0) {
            /* Remove trailing comma */
            if (buf->buf[buf->size - 1] == ',')
                buf->size--;
        } else {
	        if (!only_symbols)
	            copy_to_buf(buf, "\n\t");
            copy_to_buf(buf, "\t[ ");
            if (!write_keysyms(keymap, buf, buf2, key, 0, false))
                return false;
            copy_to_buf(buf, " ]");
        }
        write_buf(buf, "%s", (only_symbols ? " };\n" : "\n\t};\n"));
    }
    else {
        assert(key->num_groups > 0);
        for (group = 0; group < key->num_groups; group++) {
            if (group != 0)
                copy_to_buf(buf, ",");
            write_buf(buf, "\n\t\tsymbols[Group%u]= [ ", group + 1);
            if (!write_keysyms(keymap, buf, buf2, key, group, show_actions))
                return false;
            copy_to_buf(buf, " ]");
            if (show_actions) {
                write_buf(buf, ",\n\t\tactions[Group%u]= [ ", group + 1);
                if (!write_actions(keymap, buf, buf2, key, group))
                    return false;
                copy_to_buf(buf, " ]");
            }
        }
        copy_to_buf(buf, "\n\t};\n");
    }

    return true;
}

static bool
write_symbols(struct xkb_keymap *keymap, struct buf *buf)
{
    const struct xkb_key *key;
    xkb_layout_index_t group;
    xkb_mod_index_t i;
    const struct xkb_mod *mod;

    if (keymap->symbols_section_name)
        write_buf(buf, "xkb_symbols \"%s\" {\n",
                  keymap->symbols_section_name);
    else
        copy_to_buf(buf, "xkb_symbols {\n");

    for (group = 0; group < keymap->num_group_names; group++)
        if (keymap->group_names[group]) {
            write_buf(buf, "\tname[Group%u]=", group + 1);
            write_buf_string_literal(
                buf, xkb_atom_text(keymap->ctx, keymap->group_names[group]));
            copy_to_buf(buf, ";\n");
        }
    if (group > 0)
        copy_to_buf(buf, "\n");

    struct buf buf2 = { NULL, 0, 0 };
    xkb_keys_foreach(key, keymap) {
        /* Skip keys with no explicit values */
        if (key->explicit) {
            if (!write_key(keymap, buf, &buf2, key)) {
                free(buf2.buf);
                return false;
            }
        }
    }
    free(buf2.buf);

    xkb_rmods_enumerate(i, mod, &keymap->mods) {
        bool had_any = false;
        xkb_keys_foreach(key, keymap) {
            if (key->modmap & (UINT32_C(1) << i)) {
                if (!had_any)
                    write_buf(buf, "\tmodifier_map %s { ",
                              xkb_atom_text(keymap->ctx, mod->name));
                write_buf(buf, "%s%s",
                          had_any ? ", " : "",
                          KeyNameText(keymap->ctx, key->name));
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
write_keymap(struct xkb_keymap *keymap, struct buf *buf)
{
    return (check_write_buf(buf, "xkb_keymap {\n") &&
            write_keycodes(keymap, buf) &&
            write_types(keymap, buf) &&
            write_compat(keymap, buf) &&
            write_symbols(keymap, buf) &&
            check_write_buf(buf, "};\n"));
}

char *
text_v1_keymap_get_as_string(struct xkb_keymap *keymap)
{
    struct buf buf = { NULL, 0, 0 };

    if (!write_keymap(keymap, &buf)) {
        free(buf.buf);
        return NULL;
    }

    return buf.buf;
}
