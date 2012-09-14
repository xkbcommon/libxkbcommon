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

/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "xkb-priv.h"
#include "text.h"

#define VMOD_HIDE_VALUE    0
#define VMOD_SHOW_VALUE    1
#define VMOD_COMMENT_VALUE 2

#define BUF_CHUNK_SIZE     4096

struct buf {
    char *buf;
    size_t size;
    size_t alloc;
};

static bool
do_realloc(struct buf *buf, size_t at_least)
{
    char *new;

    buf->alloc += BUF_CHUNK_SIZE;
    if (at_least >= BUF_CHUNK_SIZE)
        buf->alloc += at_least;

    new = realloc(buf->buf, buf->alloc);
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
    va_start(args, fmt);
    printed = vsnprintf(buf->buf + buf->size, available, fmt, args);
    va_end(args);

    if (printed < 0)
        goto err;

    if (printed >= available)
        if (!do_realloc(buf, printed))
            goto err;

    /* The buffer has enough space now. */

    available = buf->alloc - buf->size;
    va_start(args, fmt);
    printed = vsnprintf(buf->buf + buf->size, available, fmt, args);
    va_end(args);

    if (printed >= available || printed < 0)
        goto err;

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
write_vmods(struct xkb_keymap *keymap, struct buf *buf)
{
    int num_vmods = 0;
    int i;

    for (i = 0; i < XKB_NUM_VIRTUAL_MODS; i++) {
        if (!keymap->vmod_names[i])
            continue;
        if (num_vmods == 0)
            write_buf(buf, "\t\tvirtual_modifiers ");
        else
            write_buf(buf, ",");
        write_buf(buf, "%s",
                  xkb_atom_text(keymap->ctx, keymap->vmod_names[i]));
        num_vmods++;
    }

    if (num_vmods > 0)
        write_buf(buf, ";\n\n");

    return true;
}

#define GET_TEXT_BUF_SIZE 512

#define append_get_text(...) do { \
        int _size = snprintf(ret, GET_TEXT_BUF_SIZE, __VA_ARGS__); \
        if (_size >= GET_TEXT_BUF_SIZE) \
            return NULL; \
} while (0)

static char *
get_indicator_state_text(uint8_t which)
{
    int i;
    static char ret[GET_TEXT_BUF_SIZE];

    memset(ret, 0, GET_TEXT_BUF_SIZE);

    if (which == 0) {
        strcpy(ret, "0");
        return NULL;
    }

    for (i = 0; which != 0; i++) {
        const char *name;

        if (!(which & (1 << i)))
            continue;

        which &= ~(1 << i);
        name = LookupValue(modComponentMaskNames, (1 << i));

        if (ret[0] != '\0')
            append_get_text("%s+%s", ret, name);
        else
            append_get_text("%s", name);
    }

    return ret;
}

static char *
get_control_mask_text(enum xkb_action_controls control_mask)
{
    int i;
    static char ret[GET_TEXT_BUF_SIZE];
    const char *control_name;

    memset(ret, 0, GET_TEXT_BUF_SIZE);

    if (control_mask == 0) {
        strcpy(ret, "none");
        return ret;
    }
    else if (control_mask == CONTROL_ALL) {
        strcpy(ret, "all");
        return ret;
    }

    for (i = 0; control_mask; i++) {
        if (!(control_mask & (1 << i)))
            continue;

        control_mask &= ~(1 << i);
        control_name = LookupValue(ctrlMaskNames, (1 << i));

        if (ret[0] != '\0')
            append_get_text("%s+%s", ret, control_name);
        else
            append_get_text("%s", control_name);
    }

    return ret;
}

static bool
write_keycodes(struct xkb_keymap *keymap, struct buf *buf)
{
    struct xkb_key *key;
    struct xkb_key_alias *alias;
    int i;

    if (keymap->keycodes_section_name)
        write_buf(buf, "\txkb_keycodes \"%s\" {\n",
                  keymap->keycodes_section_name);
    else
        write_buf(buf, "\txkb_keycodes {\n");

    write_buf(buf, "\t\tminimum = %d;\n",
              keymap->min_key_code);
    write_buf(buf, "\t\tmaximum = %d;\n",
              keymap->max_key_code);

    xkb_foreach_key(key, keymap) {
        if (key->name[0] == '\0')
            continue;

        write_buf(buf, "\t\t%6s = %d;\n",
                  KeyNameText(key->name), XkbKeyGetKeycode(keymap, key));
    }

    for (i = 0; i < XKB_NUM_INDICATORS; i++) {
        if (keymap->indicators[i].name == XKB_ATOM_NONE)
            continue;
        write_buf(buf, "\t\tindicator %d = \"%s\";\n", i + 1,
                  xkb_atom_text(keymap->ctx, keymap->indicators[i].name));
    }


    darray_foreach(alias, keymap->key_aliases)
        write_buf(buf, "\t\talias %6s = %6s;\n",
                  KeyNameText(alias->alias),
                  KeyNameText(alias->real));

    write_buf(buf, "\t};\n\n");
    return true;
}

static bool
write_types(struct xkb_keymap *keymap, struct buf *buf)
{
    unsigned int i, j;
    xkb_level_index_t n;
    struct xkb_key_type *type;
    struct xkb_kt_map_entry *entry;

    if (keymap->types_section_name)
        write_buf(buf, "\txkb_types \"%s\" {\n\n",
                  keymap->types_section_name);
    else
        write_buf(buf, "\txkb_types {\n\n");

    write_vmods(keymap, buf);

    for (i = 0; i < keymap->num_types; i++) {
        type = &keymap->types[i];

        write_buf(buf, "\t\ttype \"%s\" {\n",
                  xkb_atom_text(keymap->ctx, type->name));
        write_buf(buf, "\t\t\tmodifiers= %s;\n",
                  VModMaskText(keymap, type->mods.mods));

        for (j = 0; j < type->num_entries; j++) {
            const char *str;
            entry = &type->map[j];

            /*
             * Printing level 1 entries is redundant, it's the default,
             * unless there's preserve info.
             */
            if (entry->level == 0 && entry->preserve.mods == 0)
                continue;

            str = VModMaskText(keymap, entry->mods.mods);
            write_buf(buf, "\t\t\tmap[%s]= Level%d;\n",
                      str, entry->level + 1);

            if (entry->preserve.mods == 0)
                continue;

            write_buf(buf, "\t\t\tpreserve[%s]= ", str);
            write_buf(buf, "%s;\n", VModMaskText(keymap, entry->preserve.mods));
        }

        if (type->level_names) {
            for (n = 0; n < type->num_levels; n++) {
                if (!type->level_names[n])
                    continue;
                write_buf(buf, "\t\t\tlevel_name[Level%d]= \"%s\";\n", n + 1,
                          xkb_atom_text(keymap->ctx, type->level_names[n]));
            }
        }
        write_buf(buf, "\t\t};\n");
    }

    write_buf(buf, "\t};\n\n");
    return true;
}

static bool
write_indicator_map(struct xkb_keymap *keymap, struct buf *buf, int num)
{
    struct xkb_indicator_map *led = &keymap->indicators[num];

    write_buf(buf, "\t\tindicator \"%s\" {\n",
              xkb_atom_text(keymap->ctx, keymap->indicators[num].name));

    if (led->which_groups) {
        if (led->which_groups != XKB_STATE_EFFECTIVE) {
            write_buf(buf, "\t\t\twhichGroupState= %s;\n",
                      get_indicator_state_text(led->which_groups));
        }
        write_buf(buf, "\t\t\tgroups= 0x%02x;\n",
                  led->groups);
    }

    if (led->which_mods) {
        if (led->which_mods != XKB_STATE_EFFECTIVE) {
            write_buf(buf, "\t\t\twhichModState= %s;\n",
                      get_indicator_state_text(led->which_mods));
        }
        write_buf(buf, "\t\t\tmodifiers= %s;\n",
                  VModMaskText(keymap, led->mods.mods));
    }

    if (led->ctrls) {
        write_buf(buf, "\t\t\tcontrols= %s;\n",
                  get_control_mask_text(led->ctrls));
    }

    write_buf(buf, "\t\t};\n");
    return true;
}

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
            args = VModMaskText(keymap, action->mods.mods.mods);
        write_buf(buf, "%s%s(modifiers=%s%s%s)%s", prefix, type, args,
                  (action->type != ACTION_TYPE_MOD_LOCK &&
                   (action->mods.flags & ACTION_LOCK_CLEAR)) ?
                   ",clearLocks" : "",
                  (action->type != ACTION_TYPE_MOD_LOCK &&
                   (action->mods.flags & ACTION_LATCH_TO_LOCK)) ?
                   ",latchToLock" : "",
                  suffix);
        break;

    case ACTION_TYPE_GROUP_SET:
    case ACTION_TYPE_GROUP_LATCH:
    case ACTION_TYPE_GROUP_LOCK:
        write_buf(buf, "%s%s(group=%s%d%s%s)%s", prefix, type,
                  (!(action->group.flags & ACTION_ABSOLUTE_SWITCH) &&
                   action->group.group > 0) ? "+" : "",
                  (action->group.flags & ACTION_ABSOLUTE_SWITCH) ?
                  action->group.group + 1 : action->group.group,
                  (action->type != ACTION_TYPE_GROUP_LOCK &&
                   (action->group.flags & ACTION_LOCK_CLEAR)) ?
                  ",clearLocks" : "",
                  (action->type != ACTION_TYPE_GROUP_LOCK &&
                   (action->group.flags & ACTION_LATCH_TO_LOCK)) ?
                  ",latchToLock" : "",
                  suffix);
        break;

    case ACTION_TYPE_TERMINATE:
        write_buf(buf, "%s%s()%s", prefix, type, suffix);
        break;

    case ACTION_TYPE_PTR_MOVE:
        write_buf(buf, "%s%s(x=%s%d,y=%s%d%s)%s", prefix, type,
                  (!(action->ptr.flags & ACTION_ABSOLUTE_X) &&
                   action->ptr.x >= 0) ? "+" : "",
                  action->ptr.x,
                  (!(action->ptr.flags & ACTION_ABSOLUTE_Y) &&
                   action->ptr.y >= 0) ? "+" : "",
                  action->ptr.y,
                  (action->ptr.flags & ACTION_NO_ACCEL) ? ",!accel" : "",
                  suffix);
        break;

    case ACTION_TYPE_PTR_LOCK:
        switch (action->btn.flags &
                 (ACTION_LOCK_NO_LOCK | ACTION_LOCK_NO_UNLOCK)) {
        case ACTION_LOCK_NO_UNLOCK:
            args = ",affect=lock";
            break;

        case ACTION_LOCK_NO_LOCK:
            args = ",affect=unlock";
            break;

        case ACTION_LOCK_NO_LOCK | ACTION_LOCK_NO_UNLOCK:
            args = ",affect=neither";
            break;

        default:
            args = ",affect=both";
            break;
        }
    case ACTION_TYPE_PTR_BUTTON:
        write_buf(buf, "%s%s(button=", prefix, type);
        if (action->btn.button > 0 && action->btn.button <= 5)
            write_buf(buf, "%d", action->btn.button);
        else
            write_buf(buf, "default");
        if (action->btn.count)
            write_buf(buf, ",count=%d", action->btn.count);
        if (args)
            write_buf(buf, "%s", args);
        write_buf(buf, ")%s", suffix);
        break;

    case ACTION_TYPE_PTR_DEFAULT:
        write_buf(buf, "%s%s(", prefix, type);
        write_buf(buf, "affect=button,button=%s%d",
                  (!(action->dflt.flags & ACTION_ABSOLUTE_SWITCH) &&
                   action->dflt.value >= 0) ? "+" : "",
                  action->dflt.value);
        write_buf(buf, ")%s", suffix);
        break;

    case ACTION_TYPE_SWITCH_VT:
        write_buf(buf, "%s%s(screen=%s%d,%ssame)%s", prefix, type,
                  (!(action->screen.flags & ACTION_ABSOLUTE_SWITCH) &&
                   action->screen.screen >= 0) ? "+" : "",
                  action->screen.screen,
                  (action->screen.flags & ACTION_SAME_SCREEN) ? "!" : "",
                  suffix);
        break;

    case ACTION_TYPE_CTRL_SET:
    case ACTION_TYPE_CTRL_LOCK:
        write_buf(buf, "%s%s(controls=%s)%s", prefix, type,
                  get_control_mask_text(action->ctrls.ctrls), suffix);
        break;

    case ACTION_TYPE_NONE:
        write_buf(buf, "%sNoAction()%s", prefix, suffix);
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
write_compat(struct xkb_keymap *keymap, struct buf *buf)
{
    int i;
    struct xkb_sym_interpret *interp;

    if (keymap->compat_section_name)
        write_buf(buf, "\txkb_compatibility \"%s\" {\n\n",
                  keymap->compat_section_name);
    else
        write_buf(buf, "\txkb_compatibility {\n\n");

    write_vmods(keymap, buf);

    write_buf(buf, "\t\tinterpret.useModMapMods= AnyLevel;\n");
    write_buf(buf, "\t\tinterpret.repeat= False;\n");

    darray_foreach(interp, keymap->sym_interpret) {
        char keysym_name[64];

        if (interp->sym == XKB_KEY_NoSymbol)
            sprintf(keysym_name, "Any");
        else
            xkb_keysym_get_name(interp->sym, keysym_name, sizeof(keysym_name));

        write_buf(buf, "\t\tinterpret %s+%s(%s) {\n",
                  keysym_name,
                  SIMatchText(interp->match),
                  VModMaskText(keymap, interp->mods));

        if (interp->virtual_mod != XKB_MOD_INVALID) {
            write_buf(buf, "\t\t\tvirtualModifier= %s;\n",
                      xkb_atom_text(keymap->ctx,
                                    keymap->vmod_names[interp->virtual_mod]));
        }

        if (interp->match & MATCH_LEVEL_ONE_ONLY)
            write_buf(buf,
                      "\t\t\tuseModMapMods=level1;\n");
        if (interp->repeat)
            write_buf(buf, "\t\t\trepeat= True;\n");

        write_action(keymap, buf, &interp->act, "\t\t\taction= ", ";\n");
        write_buf(buf, "\t\t};\n");
    }

    for (i = 0; i < XKB_NUM_INDICATORS; i++) {
        struct xkb_indicator_map *map = &keymap->indicators[i];
        if (map->which_groups == 0 && map->groups == 0 &&
            map->which_mods == 0 && map->mods.mods == 0 &&
            map->ctrls == 0)
            continue;
        write_indicator_map(keymap, buf, i);
    }

    write_buf(buf, "\t};\n\n");

    return true;
}

static bool
write_keysyms(struct xkb_keymap *keymap, struct buf *buf,
              struct xkb_key *key, xkb_group_index_t group)
{
    const xkb_keysym_t *syms;
    int num_syms;
    xkb_level_index_t level;
#define OUT_BUF_LEN 128
    char out_buf[OUT_BUF_LEN];

    for (level = 0; level < XkbKeyGroupWidth(keymap, key, group); level++) {
        if (level != 0)
            write_buf(buf, ", ");
        num_syms = xkb_key_get_syms_by_level(keymap, key, group, level,
                                             &syms);
        if (num_syms == 0) {
            write_buf(buf, "%15s", "NoSymbol");
        }
        else if (num_syms == 1) {
            xkb_keysym_get_name(syms[0], out_buf, OUT_BUF_LEN);
            write_buf(buf, "%15s", out_buf);
        }
        else {
            int s;
            write_buf(buf, "{ ");
            for (s = 0; s < num_syms; s++) {
                if (s != 0)
                    write_buf(buf, ", ");
                xkb_keysym_get_name(syms[s], out_buf, OUT_BUF_LEN);
                write_buf(buf, "%s", out_buf);
            }
            write_buf(buf, " }");
        }
    }
#undef OUT_BUF_LEN

    return true;
}

static bool
write_symbols(struct xkb_keymap *keymap, struct buf *buf)
{
    struct xkb_key *key;
    xkb_group_index_t group, tmp;
    bool showActions;

    if (keymap->symbols_section_name)
        write_buf(buf, "\txkb_symbols \"%s\" {\n\n",
                  keymap->symbols_section_name);
    else
        write_buf(buf, "\txkb_symbols {\n\n");

    for (tmp = group = 0; group < XKB_NUM_GROUPS; group++) {
        if (!keymap->group_names[group])
            continue;
        write_buf(buf,
                  "\t\tname[group%d]=\"%s\";\n", group + 1,
                  xkb_atom_text(keymap->ctx, keymap->group_names[group]));
        tmp++;
    }
    if (tmp > 0)
        write_buf(buf, "\n");

    xkb_foreach_key(key, keymap) {
        bool simple = true;

        if (key->num_groups == 0)
            continue;

        write_buf(buf, "\t\tkey %6s {", KeyNameText(key->name));

        if (key->explicit_groups) {
            bool multi_type = false;
            struct xkb_key_type *type = XkbKeyType(keymap, key, 0);

            simple = false;

            for (group = 1; group < key->num_groups; group++) {
                if (XkbKeyType(keymap, key, group) != type) {
                    multi_type = true;
                    break;
                }
            }

            if (multi_type) {
                for (group = 0; group < key->num_groups; group++) {
                    if (!(key->explicit_groups & (1 << group)))
                        continue;
                    type = XkbKeyType(keymap, key, group);
                    write_buf(buf, "\n\t\t\ttype[group%u]= \"%s\",",
                              group + 1,
                              xkb_atom_text(keymap->ctx, type->name));
                }
            }
            else {
                write_buf(buf, "\n\t\t\ttype= \"%s\",",
                          xkb_atom_text(keymap->ctx, type->name));
            }
        }

        if (key->explicit & EXPLICIT_REPEAT) {
            if (key->repeats)
                write_buf(buf, "\n\t\t\trepeat= Yes,");
            else
                write_buf(buf, "\n\t\t\trepeat= No,");
            simple = false;
        }

        if (key->vmodmap && (key->explicit & EXPLICIT_VMODMAP)) {
            /* XXX: vmodmap cmask? */
            write_buf(buf, "\n\t\t\tvirtualMods= %s,",
                      VModMaskText(keymap, key->vmodmap << XKB_NUM_CORE_MODS));
        }

        switch (key->out_of_range_group_action) {
        case RANGE_SATURATE:
            write_buf(buf, "\n\t\t\tgroupsClamp,");
            break;

        case RANGE_REDIRECT:
            write_buf(buf, "\n\t\t\tgroupsRedirect= Group%u,",
                      key->out_of_range_group_number + 1);
            break;

        default:
            break;
        }

        if (key->explicit & EXPLICIT_INTERP)
            showActions = (key->actions != NULL);
        else
            showActions = false;

        if (key->num_groups > 1 || showActions)
            simple = false;

        if (simple) {
            write_buf(buf, "\t[ ");
            if (!write_keysyms(keymap, buf, key, 0))
                return false;
            write_buf(buf, " ] };\n");
        }
        else {
            xkb_level_index_t level;

            for (group = 0; group < key->num_groups; group++) {
                if (group != 0)
                    write_buf(buf, ",");
                write_buf(buf, "\n\t\t\tsymbols[Group%u]= [ ", group + 1);
                if (!write_keysyms(keymap, buf, key, group))
                    return false;
                write_buf(buf, " ]");
                if (showActions) {
                    write_buf(buf, ",\n\t\t\tactions[Group%u]= [ ",
                              group + 1);
                    for (level = 0;
                         level < XkbKeyGroupWidth(keymap, key, group);
                         level++) {
                        if (level != 0)
                            write_buf(buf, ", ");
                        write_action(keymap, buf,
                                     XkbKeyActionEntry(key, group, level),
                                     NULL, NULL);
                    }
                    write_buf(buf, " ]");
                }
            }
            write_buf(buf, "\n\t\t};\n");
        }
    }

    xkb_foreach_key(key, keymap) {
        int mod;

        if (key->modmap == 0)
            continue;

        for (mod = 0; mod < XKB_NUM_CORE_MODS; mod++) {
            if (!(key->modmap & (1 << mod)))
                continue;

            write_buf(buf, "\t\tmodifier_map %s { %s };\n",
                      ModIndexToName(mod), KeyNameText(key->name));
        }
    }

    write_buf(buf, "\t};\n\n");
    return true;
}

XKB_EXPORT char *
xkb_map_get_as_string(struct xkb_keymap *keymap)
{
    bool ok;
    struct buf buf = { NULL, 0, 0 };

    ok = (check_write_buf(&buf, "xkb_keymap {\n") &&
          write_keycodes(keymap, &buf) &&
          write_types(keymap, &buf) &&
          write_compat(keymap, &buf) &&
          write_symbols(keymap, &buf) &&
          check_write_buf(&buf, "};\n"));

    return (ok ? buf.buf : NULL);
}
