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

    for (i = 0; i < XkbNumVirtualMods; i++) {
        if (!keymap->vmod_names[i])
            continue;
        if (num_vmods == 0)
            write_buf(buf, "\t\tvirtual_modifiers ");
        else
            write_buf(buf, ",");
        write_buf(buf, "%s", keymap->vmod_names[i]);
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
get_mod_mask_text(struct xkb_keymap *keymap, uint8_t real_mods,
                  uint32_t vmods)
{
    static char ret[GET_TEXT_BUF_SIZE], ret2[GET_TEXT_BUF_SIZE];
    int i;

    memset(ret, 0, GET_TEXT_BUF_SIZE);

    if (real_mods == 0 && vmods == 0) {
        strcpy(ret, "none");
        return ret;
    }

    /* This is so broken.  If we have a real modmask of 0xff and a
     * vmodmask, we'll get, e.g., all+RightControl.  But, it's what xkbfile
     * does, so ... */
    if (real_mods == 0xff) {
        strcpy(ret, "all");
    }
    else if (real_mods) {
        for (i = 0; i < XkbNumModifiers; i++) {
            if (!(real_mods & (1 << i)))
                continue;
            if (ret[0] != '\0') {
                strcpy(ret2, ret);
                append_get_text("%s+%s", ret2, ModIndexToName(i));
            }
            else {
                append_get_text("%s", ModIndexToName(i));
            }
        }
    }

    if (vmods == 0)
        return ret;

    for (i = 0; i < XkbNumVirtualMods; i++) {
        if (!(vmods & (1 << i)))
            continue;
        if (ret[0] != '\0') {
            strcpy(ret2, ret);
            append_get_text("%s+%s", ret2, keymap->vmod_names[i]);
        }
        else {
            append_get_text("%s", keymap->vmod_names[i]);
        }
    }

    return ret;
}

static char *
get_indicator_state_text(uint8_t which)
{
    int i;
    static char ret[GET_TEXT_BUF_SIZE];
    /* FIXME: Merge with ... something ... in xkbcomp? */
    static const char *state_names[] = {
        "base",
        "latched",
        "locked",
        "effective",
        "compat"
    };

    memset(ret, 0, GET_TEXT_BUF_SIZE);

    which &= XkbIM_UseAnyMods;

    if (which == 0) {
        strcpy(ret, "0");
        return NULL;
    }

    for (i = 0; which != 0; i++) {
        if (!(which & (1 << i)))
            continue;
        which &= ~(1 << i);

        if (ret[0] != '\0')
            append_get_text("%s+%s", ret, state_names[i]);
        else
            append_get_text("%s", state_names[i]);
    }

    return ret;
}

static char *
get_control_mask_text(uint32_t control_mask)
{
    int i;
    static char ret[GET_TEXT_BUF_SIZE];
    /* FIXME: Merge with ... something ... in xkbcomp. */
    static const char *ctrl_names[] = {
        "RepeatKeys",
        "SlowKeys",
        "BounceKeys",
        "StickyKeys",
        "MouseKeys",
        "MouseKeysAccel",
        "AccessXKeys",
        "AccessXTimeout",
        "AccessXFeedback",
        "AudibleBell",
        "Overlay1",
        "Overlay2",
        "IgnoreGroupLock"
    };

    memset(ret, 0, GET_TEXT_BUF_SIZE);

    control_mask &= XkbAllBooleanCtrlsMask;

    if (control_mask == 0) {
        strcpy(ret, "none");
        return ret;
    }
    else if (control_mask == XkbAllBooleanCtrlsMask) {
        strcpy(ret, "all");
        return ret;
    }

    for (i = 0; control_mask; i++) {
        if (!(control_mask & (1 << i)))
            continue;
        control_mask &= ~(1 << i);

        if (ret[0] != '\0')
            append_get_text("%s+%s", ret, ctrl_names[i]);
        else
            append_get_text("%s", ctrl_names[i]);
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

    for (i = 0; i < XkbNumIndicators; i++) {
        if (!keymap->indicator_names[i])
            continue;
        write_buf(buf, "\t\tindicator %d = \"%s\";\n",
                  i + 1, keymap->indicator_names[i]);
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
    int n;
    struct xkb_key_type *type;
    struct xkb_kt_map_entry *entry;

    if (keymap->types_section_name)
        write_buf(buf, "\txkb_types \"%s\" {\n\n",
                  keymap->types_section_name);
    else
        write_buf(buf, "\txkb_types {\n\n");

    write_vmods(keymap, buf);

    darray_foreach(type, keymap->types) {
        write_buf(buf, "\t\ttype \"%s\" {\n",
                  type->name);
        write_buf(buf, "\t\t\tmodifiers= %s;\n",
                  get_mod_mask_text(keymap, type->mods.real_mods,
                                    type->mods.vmods));

        darray_foreach(entry, type->map) {
            char *str;

            str = get_mod_mask_text(keymap, entry->mods.real_mods,
                                    entry->mods.vmods);
            write_buf(buf, "\t\t\tmap[%s]= Level%d;\n",
                      str, entry->level + 1);

            if (!entry->preserve.real_mods && !entry->preserve.vmods)
                continue;

            write_buf(buf, "\t\t\tpreserve[%s]= ", str);
            write_buf(buf, "%s;\n",
                      get_mod_mask_text(keymap, entry->preserve.real_mods,
                                        entry->preserve.vmods));
        }

        if (type->level_names) {
            for (n = 0; n < type->num_levels; n++) {
                if (!type->level_names[n])
                    continue;
                write_buf(buf, "\t\t\tlevel_name[Level%d]= \"%s\";\n", n + 1,
                          type->level_names[n]);
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
              keymap->indicator_names[num]);

    if (led->which_groups) {
        if (led->which_groups != XkbIM_UseEffective) {
            write_buf(buf, "\t\t\twhichGroupState= %s;\n",
                      get_indicator_state_text(led->which_groups));
        }
        write_buf(buf, "\t\t\tgroups= 0x%02x;\n",
                  led->groups);
    }

    if (led->which_mods) {
        if (led->which_mods != XkbIM_UseEffective) {
            write_buf(buf, "\t\t\twhichModState= %s;\n",
                      get_indicator_state_text(led->which_mods));
        }
        write_buf(buf, "\t\t\tmodifiers= %s;\n",
                  get_mod_mask_text(keymap, led->mods.real_mods,
                                    led->mods.vmods));
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
             union xkb_action *action, const char *prefix, const char *suffix)
{
    const char *type;
    const char *args = NULL;

    if (!prefix)
        prefix = "";
    if (!suffix)
        suffix = "";

    type = ActionTypeText(action->any.type);

    switch (action->any.type) {
    case XkbSA_SetMods:
    case XkbSA_LatchMods:
    case XkbSA_LockMods:
        if (action->mods.flags & XkbSA_UseModMapMods)
            args = "modMapMods";
        else
            args = get_mod_mask_text(keymap, action->mods.real_mods,
                                     action->mods.vmods);
        write_buf(buf, "%s%s(modifiers=%s%s%s)%s", prefix, type, args,
                  (action->any.type != XkbSA_LockGroup &&
                   (action->mods.flags & XkbSA_ClearLocks)) ?
                   ",clearLocks" : "",
                  (action->any.type != XkbSA_LockGroup &&
                   (action->mods.flags & XkbSA_LatchToLock)) ?
                   ",latchToLock" : "",
                  suffix);
        break;

    case XkbSA_SetGroup:
    case XkbSA_LatchGroup:
    case XkbSA_LockGroup:
        write_buf(buf, "%s%s(group=%s%d%s%s)%s", prefix, type,
                  (!(action->group.flags & XkbSA_GroupAbsolute) &&
                   action->group.group > 0) ? "+" : "",
                  (action->group.flags & XkbSA_GroupAbsolute) ?
                  action->group.group + 1 : action->group.group,
                  (action->any.type != XkbSA_LockGroup &&
                   (action->group.flags & XkbSA_ClearLocks)) ?
                  ",clearLocks" : "",
                  (action->any.type != XkbSA_LockGroup &&
                   (action->group.flags & XkbSA_LatchToLock)) ?
                  ",latchToLock" : "",
                  suffix);
        break;

    case XkbSA_Terminate:
        write_buf(buf, "%s%s()%s", prefix, type, suffix);
        break;

    case XkbSA_MovePtr:
        write_buf(buf, "%s%s(x=%s%d,y=%s%d%s)%s", prefix, type,
                  (!(action->ptr.flags & XkbSA_MoveAbsoluteX) &&
                   action->ptr.x >= 0) ? "+" : "",
                  action->ptr.x,
                  (!(action->ptr.flags & XkbSA_MoveAbsoluteY) &&
                   action->ptr.y >= 0) ? "+" : "",
                  action->ptr.y,
                  (action->ptr.flags & XkbSA_NoAcceleration) ? ",!accel" : "",
                  suffix);
        break;

    case XkbSA_LockPtrBtn:
        switch (action->btn.flags & (XkbSA_LockNoUnlock | XkbSA_LockNoLock)) {
        case XkbSA_LockNoUnlock:
            args = ",affect=lock";
            break;

        case XkbSA_LockNoLock:
            args = ",affect=unlock";
            break;

        case XkbSA_LockNoLock | XkbSA_LockNoUnlock:
            args = ",affect=neither";
            break;

        default:
            args = ",affect=both";
            break;
        }
    case XkbSA_PtrBtn:
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

    case XkbSA_SetPtrDflt:
        write_buf(buf, "%s%s(", prefix, type);
        if (action->dflt.affect == XkbSA_AffectDfltBtn)
            write_buf(buf, "affect=button,button=%s%d",
                      (!(action->dflt.flags & XkbSA_DfltBtnAbsolute) &&
                       action->dflt.value >= 0) ? "+" : "",
                      action->dflt.value);
        write_buf(buf, ")%s", suffix);
        break;

    case XkbSA_SwitchScreen:
        write_buf(buf, "%s%s(screen=%s%d,%ssame)%s", prefix, type,
                  (!(action->screen.flags & XkbSA_SwitchAbsolute) &&
                   action->screen.screen >= 0) ? "+" : "",
                  action->screen.screen,
                  (action->screen.flags & XkbSA_SwitchApplication) ? "!" : "",
                  suffix);
        break;

    /* Deprecated actions below here */
    case XkbSA_SetControls:
    case XkbSA_LockControls:
        write_buf(buf, "%s%s(controls=%s)%s", prefix, type,
                  get_control_mask_text(action->ctrls.ctrls), suffix);
        break;

    case XkbSA_ISOLock:
    case XkbSA_ActionMessage:
    case XkbSA_RedirectKey:
    case XkbSA_DeviceBtn:
    case XkbSA_LockDeviceBtn:
    case XkbSA_NoAction:
        /* XXX TODO */
        write_buf(buf, "%sNoAction()%s", prefix, suffix);
        break;

    case XkbSA_XFree86Private:
    default:
        write_buf(buf,
                  "%s%s(type=0x%02x,data[0]=0x%02x,data[1]=0x%02x,data[2]=0x%02x,data[3]=0x%02x,data[4]=0x%02x,data[5]=0x%02x,data[6]=0x%02x)%s",
                  prefix, type, action->any.type, action->any.data[0],
                  action->any.data[1], action->any.data[2],
                  action->any.data[3], action->any.data[4],
                  action->any.data[5], action->any.data[6],
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
    write_buf(buf, "\t\tinterpret.locking= False;\n");

    darray_foreach(interp, keymap->sym_interpret) {
        char keysym_name[64];

        if (interp->sym == XKB_KEY_NoSymbol)
            sprintf(keysym_name, "Any");
        else
            xkb_keysym_get_name(interp->sym, keysym_name, sizeof(keysym_name));

        write_buf(buf, "\t\tinterpret %s+%s(%s) {\n",
                  keysym_name,
                  SIMatchText(interp->match),
                  get_mod_mask_text(keymap, interp->mods, 0));

        if (interp->virtual_mod != XkbNoModifier) {
            write_buf(buf, "\t\t\tvirtualModifier= %s;\n",
                      keymap->vmod_names[interp->virtual_mod]);
        }

        if (interp->match & XkbSI_LevelOneOnly)
            write_buf(buf,
                      "\t\t\tuseModMapMods=level1;\n");
        if (interp->flags & XkbSI_LockingKey)
            write_buf(buf, "\t\t\tlocking= True;\n");
        if (interp->flags & XkbSI_AutoRepeat)
            write_buf(buf, "\t\t\trepeat= True;\n");

        write_action(keymap, buf, &interp->act, "\t\t\taction= ", ";\n");
        write_buf(buf, "\t\t};\n");
    }

    for (i = 0; i < XkbNumKbdGroups; i++) {
        struct xkb_mods *gc;

        gc = &keymap->groups[i];
        if (gc->real_mods == 0 && gc->vmods == 0)
            continue;
        write_buf(buf, "\t\tgroup %d = %s;\n", i + 1,
                  get_mod_mask_text(keymap, gc->real_mods, gc->vmods));
    }

    for (i = 0; i < XkbNumIndicators; i++) {
        struct xkb_indicator_map *map = &keymap->indicators[i];
        if (map->flags == 0 && map->which_groups == 0 &&
            map->groups == 0 && map->which_mods == 0 &&
            map->mods.real_mods == 0 && map->mods.vmods == 0 &&
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
                write_buf(buf, "%15s", out_buf);
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

    for (tmp = group = 0; group < XkbNumKbdGroups; group++) {
        if (!keymap->group_names[group])
            continue;
        write_buf(buf,
                  "\t\tname[group%d]=\"%s\";\n", group + 1,
                  keymap->group_names[group]);
        tmp++;
    }
    if (tmp > 0)
        write_buf(buf, "\n");

    xkb_foreach_key(key, keymap) {
        bool simple = true;

        if (key->num_groups == 0)
            continue;

        write_buf(buf, "\t\tkey %6s {", KeyNameText(key->name));

        if (key->explicit & XkbExplicitKeyTypesMask) {
            bool multi_type = false;
            int type = XkbKeyTypeIndex(key, 0);

            simple = false;

            for (group = 0; group < key->num_groups; group++) {
                if (XkbKeyTypeIndex(key, group) != type) {
                    multi_type = true;
                    break;
                }
            }

            if (multi_type) {
                for (group = 0; group < key->num_groups; group++) {
                    if (!(key->explicit & (1 << group)))
                        continue;
                    type = XkbKeyTypeIndex(key, group);
                    write_buf(buf, "\n\t\t\ttype[group%u]= \"%s\",",
                              group + 1,
                              darray_item(keymap->types, type).name);
                }
            }
            else {
                write_buf(buf, "\n\t\t\ttype= \"%s\",",
                          darray_item(keymap->types, type).name);
            }
        }

        if (key->explicit & XkbExplicitAutoRepeatMask) {
            if (key->repeats)
                write_buf(buf, "\n\t\t\trepeat= Yes,");
            else
                write_buf(buf, "\n\t\t\trepeat= No,");
            simple = false;
        }

        if (key->vmodmap && (key->explicit & XkbExplicitVModMapMask)) {
            write_buf(buf, "\n\t\t\tvirtualMods= %s,",
                      get_mod_mask_text(keymap, 0, key->vmodmap));
        }

        switch (key->out_of_range_group_action) {
        case XkbClampIntoRange:
            write_buf(buf, "\n\t\t\tgroupsClamp,");
            break;

        case XkbRedirectIntoRange:
            write_buf(buf, "\n\t\t\tgroupsRedirect= Group%u,",
                      key->out_of_range_group_number + 1);
            break;
        }

        if (key->explicit & XkbExplicitInterpretMask)
            showActions = XkbKeyHasActions(key);
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
            union xkb_action *acts;
            int level;

            acts = XkbKeyActionsPtr(keymap, key);
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
                        write_action(keymap, buf, &acts[level], NULL, NULL);
                    }
                    write_buf(buf, " ]");
                    acts += key->width;
                }
            }
            write_buf(buf, "\n\t\t};\n");
        }
    }

    xkb_foreach_key(key, keymap) {
        int mod;

        if (key->modmap == 0)
            continue;

        for (mod = 0; mod < XkbNumModifiers; mod++) {
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
