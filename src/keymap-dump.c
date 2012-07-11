/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

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

#include <config.h>

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "xkb-priv.h"
#include "text.h"

#define	VMOD_HIDE_VALUE	0
#define	VMOD_SHOW_VALUE	1
#define	VMOD_COMMENT_VALUE 2

#define BUF_CHUNK_SIZE 4096

static bool
do_realloc(char **buf, size_t *size, size_t offset, size_t at_least)
{
    char *new;

    *size += BUF_CHUNK_SIZE;
    if (at_least >= BUF_CHUNK_SIZE)
        *size += at_least;

    new = realloc(*buf, *size);
    if (!new)
        return false;
    *buf = new;

    memset(*buf + offset, 0, *size - offset);

    return true;
}

/* This whole thing should be a function, but you can't call vsnprintf
 * multiple times. */
#define check_write_buf(keymap, buf, size, offset, ...) \
do { \
    size_t _printed; \
    bool _ret = true; \
    \
    /* Concatenate the strings, and check whether or not the output was \
     * truncated. */ \
    while ((_printed = snprintf(*buf + *offset, *size - *offset, \
                                __VA_ARGS__)) >= \
           (*size - *offset)) {\
        /* If it was truncated, embiggen the string and roll from the top. */ \
        if (!do_realloc(buf, size, *offset, _printed)) { \
            fprintf(stderr, \
                    "xkbcommon: failed to allocate %zu bytes for keymap\n", \
                    *size); \
            free(*buf); \
            *buf = NULL; \
            _ret = false; \
            break; \
        } \
    } \
    if (_ret == true) \
        *offset += _printed; \
} while (0)

#define write_buf(keymap, buf, size, offset, ...) \
do { \
    check_write_buf(keymap, buf, size, offset, __VA_ARGS__); \
    if (*buf == NULL) \
        return false; \
} while (0)

static bool
write_vmods(struct xkb_keymap *keymap, char **buf, size_t *size, size_t *offset)
{
    int num_vmods = 0;
    int i;

    for (i = 0; i < XkbNumVirtualMods; i++) {
	if (!keymap->names->vmods[i])
            continue;
        if (num_vmods == 0)
            write_buf(keymap, buf, size, offset, "\t\tvirtual_modifiers ");
        else
            write_buf(keymap, buf, size, offset, ",");
        write_buf(keymap, buf, size, offset, "%s", keymap->names->vmods[i]);
        num_vmods++;
    }

    if (num_vmods > 0)
	write_buf(keymap, buf, size, offset, ";\n\n");

    return true;
}

#define GET_TEXT_BUF_SIZE 512

#define append_get_text(...) do { \
    int _size = snprintf(ret, GET_TEXT_BUF_SIZE, __VA_ARGS__); \
    if (_size >= GET_TEXT_BUF_SIZE) \
        return NULL; \
} while(0)

/* FIXME: Merge with src/xkbcomp/expr.c::modIndexNames. */
static const char *core_mod_names[] = {
    "Shift",
    "Lock",
    "Control",
    "Mod1",
    "Mod2",
    "Mod3",
    "Mod4",
    "Mod5",
};

static const char *
get_mod_index_text(uint8_t real_mod)
{
    return core_mod_names[real_mod];
}

static char *
get_mod_mask_text(struct xkb_keymap *keymap, uint8_t real_mods, uint32_t vmods)
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
                append_get_text("%s+%s", ret2, core_mod_names[i]);
            }
            else {
                append_get_text("%s", core_mod_names[i]);
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
            append_get_text("%s+%s", ret2, keymap->names->vmods[i]);
        }
        else {
            append_get_text("%s", keymap->names->vmods[i]);
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
write_keycodes(struct xkb_keymap *keymap, char **buf, size_t *size,
               size_t *offset)
{
    xkb_keycode_t key;
    struct xkb_key_alias *alias;
    int i;

    write_buf(keymap, buf, size, offset, "\txkb_keycodes {\n");
    write_buf(keymap, buf, size, offset, "\t\tminimum = %d;\n",
              keymap->min_key_code);
    write_buf(keymap, buf, size, offset, "\t\tmaximum = %d;\n",
              keymap->max_key_code);

    for (key = keymap->min_key_code; key <= keymap->max_key_code; key++) {
        const char *alternate = "";

	if (darray_item(keymap->names->keys, key).name[0] == '\0')
            continue;
	if (XkbcFindKeycodeByName(keymap,
                                  darray_item(keymap->names->keys, key).name,
                                  true) != key)
	    alternate = "alternate ";
        write_buf(keymap, buf, size, offset, "\t\t%s%6s = %d;\n", alternate,
		  XkbcKeyNameText(darray_item(keymap->names->keys, key).name),
                                  key);
    }

    for (i = 0; i < XkbNumIndicators; i++) {
        if (!keymap->names->indicators[i])
            continue;
	write_buf(keymap, buf, size, offset, "\t\tindicator %d = \"%s\";\n",
                  i + 1, keymap->names->indicators[i]);
    }


    darray_foreach(alias, keymap->names->key_aliases)
        write_buf(keymap, buf, size, offset, "\t\talias %6s = %6s;\n",
                  XkbcKeyNameText(alias->alias),
                  XkbcKeyNameText(alias->real));

    write_buf(keymap, buf, size, offset, "\t};\n\n");
    return true;
}

static bool
write_types(struct xkb_keymap *keymap, char **buf, size_t *size,
            size_t *offset)
{
    int n;
    struct xkb_key_type *type;

    write_buf(keymap, buf, size, offset, "\txkb_types {\n\n");
    write_vmods(keymap, buf, size, offset);

    darray_foreach(type, keymap->map->types) {
	write_buf(keymap, buf, size, offset, "\t\ttype \"%s\" {\n",
	          type->name);
	write_buf(keymap, buf, size, offset, "\t\t\tmodifiers= %s;\n",
	          get_mod_mask_text(keymap, type->mods.real_mods,
                                    type->mods.vmods));

	for (n = 0; n < darray_size(type->map); n++) {
            struct xkb_kt_map_entry *entry = &darray_item(type->map, n);
            char *str;

            str = get_mod_mask_text(keymap, entry->mods.real_mods,
                                    entry->mods.vmods);
	    write_buf(keymap, buf, size, offset, "\t\t\tmap[%s]= Level%d;\n",
                      str, entry->level + 1);

            if (!type->preserve || (!type->preserve[n].real_mods &&
                                    !type->preserve[n].vmods))
                continue;
            write_buf(keymap, buf, size, offset, "\t\t\tpreserve[%s]= ", str);
            write_buf(keymap, buf, size, offset, "%s;\n",
                      get_mod_mask_text(keymap, type->preserve[n].real_mods,
                                        type->preserve[n].vmods));
	}

	if (type->level_names) {
	    for (n = 0; n < type->num_levels; n++) {
		if (!type->level_names[n])
		    continue;
		write_buf(keymap, buf, size, offset,
                          "\t\t\tlevel_name[Level%d]= \"%s\";\n", n + 1,
                          type->level_names[n]);
	    }
	}
	write_buf(keymap, buf, size, offset, "\t\t};\n");
    }

    write_buf(keymap, buf, size, offset, "\t};\n\n");
    return true;
}

static bool
write_indicator_map(struct xkb_keymap *keymap, char **buf, size_t *size,
                    size_t *offset, int num)
{
    struct xkb_indicator_map *led = &keymap->indicators->maps[num];

    write_buf(keymap, buf, size, offset, "\t\tindicator \"%s\" {\n",
              keymap->names->indicators[num]);

    if (led->which_groups) {
	if (led->which_groups != XkbIM_UseEffective) {
	    write_buf(keymap, buf, size, offset, "\t\t\twhichGroupState= %s;\n",
		      get_indicator_state_text(led->which_groups));
	}
	write_buf(keymap, buf, size, offset, "\t\t\tgroups= 0x%02x;\n",
                  led->groups);
    }

    if (led->which_mods) {
	if (led->which_mods != XkbIM_UseEffective) {
	    write_buf(keymap, buf, size, offset, "\t\t\twhichModState= %s;\n",
                      get_indicator_state_text(led->which_mods));
	}
	write_buf(keymap, buf, size, offset, "\t\t\tmodifiers= %s;\n",
                  get_mod_mask_text(keymap, led->mods.real_mods,
                  led->mods.vmods));
    }

    if (led->ctrls) {
	write_buf(keymap, buf, size, offset, "\t\t\tcontrols= %s;\n",
		  get_control_mask_text(led->ctrls));
    }

    write_buf(keymap, buf, size, offset, "\t\t};\n");
    return true;
}

static char *
get_interp_match_text(uint8_t type)
{
    static char ret[16];

    switch (type & XkbSI_OpMask) {
        case XkbSI_NoneOf:
            sprintf(ret, "NoneOf");
            break;
        case XkbSI_AnyOfOrNone:
            sprintf(ret, "AnyOfOrNone");
            break;
        case XkbSI_AnyOf:
            sprintf(ret, "AnyOf");
            break;
        case XkbSI_AllOf:
            sprintf(ret, "AllOf");
            break;
        case XkbSI_Exactly:
            sprintf(ret, "Exactly");
            break;
        default:
            sprintf(ret, "0x%x", type & XkbSI_OpMask);
            break;
    }

    return ret;
}

static bool
write_action(struct xkb_keymap *keymap, char **buf, size_t *size,
             size_t *offset, union xkb_action *action, const char *prefix,
             const char *suffix)
{
    const char *type = NULL;
    const char *args = NULL;

    if (!prefix)
        prefix = "";
    if (!suffix)
        suffix = "";

    switch (action->any.type) {
    case XkbSA_SetMods:
        if (!type)
            type = "SetMods";
    case XkbSA_LatchMods:
        if (!type)
            type = "LatchMods";
    case XkbSA_LockMods:
        if (!type)
            type = "LockMods";
        if (action->mods.flags & XkbSA_UseModMapMods)
            args = "modMapMods";
        else
            args = get_mod_mask_text(keymap, action->mods.real_mods,
                                     action->mods.vmods);
        write_buf(keymap, buf, size, offset, "%s%s(modifiers=%s%s%s)%s",
                  prefix, type, args,
                  (action->any.type != XkbSA_LockGroup &&
                   action->mods.flags & XkbSA_ClearLocks) ? ",clearLocks" : "",
                  (action->any.type != XkbSA_LockGroup &&
                   action->mods.flags & XkbSA_LatchToLock) ? ",latchToLock" : "",
                  suffix);
        break;
    case XkbSA_SetGroup:
        if (!type)
            type = "SetGroup";
    case XkbSA_LatchGroup:
        if (!type)
            type = "LatchGroup";
    case XkbSA_LockGroup:
        if (!type)
            type = "LockGroup";
        write_buf(keymap, buf, size, offset, "%s%s(group=%s%d%s%s)%s",
                  prefix, type,
                  (!(action->group.flags & XkbSA_GroupAbsolute) &&
                   action->group.group > 0) ? "+" : "",
                  (action->group.flags & XkbSA_GroupAbsolute) ?
                   action->group.group + 1 : action->group.group,
                  (action->any.type != XkbSA_LockGroup &&
                   action->group.flags & XkbSA_ClearLocks) ? ",clearLocks" : "",
                  (action->any.type != XkbSA_LockGroup &&
                   action->group.flags & XkbSA_LatchToLock) ? ",latchToLock" : "",
                  suffix);
        break;
    case XkbSA_Terminate:
        write_buf(keymap, buf, size, offset, "%sTerminate()%s", prefix, suffix);
        break;
    case XkbSA_MovePtr:
        write_buf(keymap, buf, size, offset, "%sMovePtr(x=%s%d,y=%s%d%s)%s",
                  prefix,
                  (!(action->ptr.flags & XkbSA_MoveAbsoluteX) &&
                   action->ptr.x >= 0) ? "+" : "",
                  action->ptr.x,
                  (!(action->ptr.flags & XkbSA_MoveAbsoluteY) &&
                   action->ptr.y >= 0) ? "+" : "",
                  action->ptr.y,
                  (action->ptr.flags & XkbSA_NoAcceleration) ? ",!accel" : "",
                  suffix);
        break;
    case XkbSA_PtrBtn:
        if (!type)
            type = "PtrBtn";
    case XkbSA_LockPtrBtn:
        if (!type) {
            type = "LockPtrBtn";
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
        }
        else {
            args = NULL;
        }
        write_buf(keymap, buf, size, offset, "%s%s(button=", prefix, type);
        if (action->btn.button > 0 && action->btn.button <= 5)
            write_buf(keymap, buf, size, offset, "%d", action->btn.button);
        else
            write_buf(keymap, buf, size, offset, "default");
        if (action->btn.count)
            write_buf(keymap, buf, size, offset, ",count=%d",
                      action->btn.count);
        if (args)
            write_buf(keymap, buf, size, offset, "%s", args);
        write_buf(keymap, buf, size, offset, ")%s", suffix);
        break;
    case XkbSA_SetPtrDflt:
        write_buf(keymap, buf, size, offset, "%sSetPtrDflt(", prefix);
        if (action->dflt.affect == XkbSA_AffectDfltBtn)
            write_buf(keymap, buf, size, offset, "affect=button,button=%s%d",
                      (!(action->dflt.flags & XkbSA_DfltBtnAbsolute) &&
                       action->dflt.value >= 0) ? "+" : "",
                      action->dflt.value);
        write_buf(keymap, buf, size, offset, ")%s", suffix);
        break;
    case XkbSA_SwitchScreen:
        write_buf(keymap, buf, size, offset,
                  "%sSwitchScreen(screen=%s%d,%ssame)%s", prefix,
                  (!(action->screen.flags & XkbSA_SwitchAbsolute) &&
                   action->screen.screen >= 0) ? "+" : "",
                  action->screen.screen,
                  (action->screen.flags & XkbSA_SwitchApplication) ? "!" : "",
                  suffix);
        break;
    /* Deprecated actions below here */
    case XkbSA_SetControls:
        if (!type)
            type = "SetControls";
    case XkbSA_LockControls:
        if (!type)
            type = "LockControls";
        write_buf(keymap, buf, size, offset, "%s%s(controls=%s)%s",
                  prefix, type, get_control_mask_text(action->ctrls.ctrls),
                  suffix);
        break;
    case XkbSA_ISOLock:
    case XkbSA_ActionMessage:
    case XkbSA_RedirectKey:
    case XkbSA_DeviceBtn:
    case XkbSA_LockDeviceBtn:
    case XkbSA_NoAction:
        /* XXX TODO */
        write_buf(keymap, buf, size, offset, "%sNoAction()%s", prefix, suffix);
        break;
    case XkbSA_XFree86Private:
    default:
        write_buf(keymap, buf, size, offset,
                  "%sPrivate(type=0x%02x,data[0]=0x%02x,data[1]=0x%02x,data[2]=0x%02x,data[3]=0x%02x,data[4]=0x%02x,data[5]=0x%02x,data[6]=0x%02x)%s",
                  prefix, action->any.type, action->any.data[0],
                  action->any.data[1], action->any.data[2],
                  action->any.data[3], action->any.data[4],
                  action->any.data[5], action->any.data[6], suffix);
        break;
    }

    return true;
}

static bool
write_compat(struct xkb_keymap *keymap, char **buf, size_t *size,
             size_t *offset)
{
    int i;
    struct xkb_sym_interpret *interp;

    write_buf(keymap, buf, size, offset, "\txkb_compatibility {\n\n");

    write_vmods(keymap, buf, size, offset);

    write_buf(keymap, buf, size, offset, "\t\tinterpret.useModMapMods= AnyLevel;\n");
    write_buf(keymap, buf, size, offset, "\t\tinterpret.repeat= False;\n");
    write_buf(keymap, buf, size, offset, "\t\tinterpret.locking= False;\n");

    darray_foreach(interp, keymap->compat->sym_interpret) {
        char keysym_name[64];

        if (interp->sym == XKB_KEY_NoSymbol)
            sprintf(keysym_name, "Any");
        else
            xkb_keysym_get_name(interp->sym, keysym_name, sizeof(keysym_name));

        write_buf(keymap, buf, size, offset, "\t\tinterpret %s+%s(%s) {\n",
                  keysym_name,
		  get_interp_match_text(interp->match),
		  get_mod_mask_text(keymap, interp->mods, 0));

	if (interp->virtual_mod != XkbNoModifier) {
	    write_buf(keymap, buf, size, offset, "\t\t\tvirtualModifier= %s;\n",
                      keymap->names->vmods[interp->virtual_mod]);
	}

	if (interp->match & XkbSI_LevelOneOnly)
	    write_buf(keymap, buf, size, offset, "\t\t\tuseModMapMods=level1;\n");
	if (interp->flags & XkbSI_LockingKey)
	    write_buf(keymap, buf, size, offset, "\t\t\tlocking= True;\n");
	if (interp->flags & XkbSI_AutoRepeat)
	    write_buf(keymap, buf, size, offset, "\t\t\trepeat= True;\n");

	write_action(keymap, buf, size, offset, &interp->act,
                     "\t\t\taction= ", ";\n");
	write_buf(keymap, buf, size, offset, "\t\t};\n");
    }

    for (i = 0; i < XkbNumKbdGroups; i++) {
	struct xkb_mods	*gc;

	gc = &keymap->compat->groups[i];
	if (gc->real_mods == 0 && gc->vmods ==0)
	    continue;
	write_buf(keymap, buf, size, offset,
                  "\t\tgroup %d = %s;\n", i + 1,
                  get_mod_mask_text(keymap, gc->real_mods, gc->vmods));
    }

    for (i = 0; i < XkbNumIndicators; i++) {
	struct xkb_indicator_map *map = &keymap->indicators->maps[i];
        if (map->flags == 0 && map->which_groups == 0 &&
            map->groups == 0 && map->which_mods == 0 &&
            map->mods.real_mods == 0 && map->mods.vmods == 0 &&
            map->ctrls == 0)
            continue;
        write_indicator_map(keymap, buf, size, offset, i);
    }

    write_buf(keymap, buf, size, offset, "\t};\n\n");

    return true;
}

static bool
write_keysyms(struct xkb_keymap *keymap, char **buf, size_t *size,
              size_t *offset, xkb_keycode_t key, unsigned int group)
{
    const xkb_keysym_t *syms;
    int num_syms, level;
#define OUT_BUF_LEN 128
    char out_buf[OUT_BUF_LEN];

    for (level = 0; level < XkbKeyGroupWidth(keymap, key, group); level++) {
        if (level != 0)
            write_buf(keymap, buf, size, offset, ", ");
        num_syms = xkb_key_get_syms_by_level(keymap, key, group, level,
                                             &syms);
        if (num_syms == 0) {
            write_buf(keymap, buf, size, offset, "%15s", "NoSymbol");
        }
        else if (num_syms == 1) {
            xkb_keysym_get_name(syms[0], out_buf, OUT_BUF_LEN);
            write_buf(keymap, buf, size, offset, "%15s", out_buf);
        }
        else {
            int s;
            write_buf(keymap, buf, size, offset, "{ ");
            for (s = 0; s < num_syms; s++) {
                if (s != 0)
                    write_buf(keymap, buf, size, offset, ", ");
                xkb_keysym_get_name(syms[s], out_buf, OUT_BUF_LEN);
                write_buf(keymap, buf, size, offset, "%15s", out_buf);
            }
            write_buf(keymap, buf, size, offset, " }");
        }
    }
#undef OUT_BUF_LEN

    return true;
}

static bool
write_symbols(struct xkb_keymap *keymap, char **buf, size_t *size,
              size_t *offset)
{
    struct xkb_client_map *map = keymap->map;
    struct xkb_server_map *srv = keymap->server;
    xkb_keycode_t key;
    int group, tmp;
    bool showActions;

    write_buf(keymap, buf, size, offset, "\txkb_symbols {\n\n");

    for (tmp = group = 0; group < XkbNumKbdGroups; group++) {
	if (!keymap->names->groups[group])
            continue;
        write_buf(keymap, buf, size, offset,
                  "\t\tname[group%d]=\"%s\";\n", group + 1,
                  keymap->names->groups[group]);
	tmp++;
    }
    if (tmp > 0)
	write_buf(keymap, buf, size, offset, "\n");

    for (key = keymap->min_key_code; key <= keymap->max_key_code; key++) {
	bool simple = true;

	if (xkb_key_num_groups(keymap, key) == 0)
	    continue;
	if (XkbcFindKeycodeByName(keymap,
                                  darray_item(keymap->names->keys, key).name,
                                  true) != key)
	    continue;

	write_buf(keymap, buf, size, offset, "\t\tkey %6s {",
		  XkbcKeyNameText(darray_item(keymap->names->keys, key).name));
	if (srv->explicit) {
            if ((srv->explicit[key] & XkbExplicitKeyTypesMask)) {
                bool multi_type = false;
                int type = XkbKeyTypeIndex(keymap, key, 0);

                simple = false;

                for (group = 0; group < xkb_key_num_groups(keymap, key); group++) {
                    if (XkbKeyTypeIndex(keymap, key, group) != type) {
                        multi_type = true;
                        break;
                    }
                }
                if (multi_type) {
                    for (group = 0;
                         group < xkb_key_num_groups(keymap, key);
                         group++) {
                        if (!(srv->explicit[key] & (1 << group)))
                            continue;
                        type = XkbKeyTypeIndex(keymap, key, group);
                        write_buf(keymap, buf, size, offset,
                                  "\n\t\t\ttype[group%d]= \"%s\",",
                                  group + 1,
                                  darray_item(map->types, type).name);
                    }
                }
                else {
                    write_buf(keymap, buf, size, offset,
                              "\n\t\t\ttype= \"%s\",",
                              darray_item(map->types, type).name);
                }
            }
	    if (keymap->ctrls && (srv->explicit[key] & XkbExplicitAutoRepeatMask)) {
		if (keymap->ctrls->per_key_repeat[key / 8] & (1 << (key % 8)))
		     write_buf(keymap, buf, size, offset,
                               "\n\t\t\trepeat= Yes,");
		else
                    write_buf(keymap, buf, size, offset,
                              "\n\t\t\trepeat= No,");
		simple = false;
	    }
	    if (keymap->server->vmodmap[key] &&
		(srv->explicit[key] & XkbExplicitVModMapMask)) {
		write_buf(keymap, buf, size, offset, "\n\t\t\tvirtualMods= %s,",
                          get_mod_mask_text(keymap, 0, keymap->server->vmodmap[key]));
	    }
	}

	switch (XkbOutOfRangeGroupAction(XkbKeyGroupInfo(keymap, key))) {
	    case XkbClampIntoRange:
		write_buf(keymap, buf, size, offset, "\n\t\t\tgroupsClamp,");
		break;
	    case XkbRedirectIntoRange:
		write_buf(keymap, buf, size, offset,
                          "\n\t\t\tgroupsRedirect= Group%d,",
			  XkbOutOfRangeGroupNumber(XkbKeyGroupInfo(keymap, key)) + 1);
		break;
	}

	if (srv->explicit == NULL ||
            (srv->explicit[key] & XkbExplicitInterpretMask))
	    showActions = XkbKeyHasActions(keymap, key);
	else
            showActions = false;

	if (xkb_key_num_groups(keymap, key) > 1 || showActions)
	    simple = false;

	if (simple) {
	    write_buf(keymap, buf, size, offset, "\t[ ");
            if (!write_keysyms(keymap, buf, size, offset, key, 0))
                return false;
            write_buf(keymap, buf, size, offset, " ] };\n");
	}
	else {
	    union xkb_action *acts;
	    int level;

	    acts = XkbKeyActionsPtr(keymap, key);
	    for (group = 0; group < xkb_key_num_groups(keymap, key); group++) {
		if (group != 0)
		    write_buf(keymap, buf, size, offset, ",");
		write_buf(keymap, buf, size, offset,
                          "\n\t\t\tsymbols[Group%d]= [ ", group + 1);
                if (!write_keysyms(keymap, buf, size, offset, key, group))
                    return false;
		write_buf(keymap, buf, size, offset, " ]");
		if (showActions) {
		    write_buf(keymap, buf, size, offset,
                              ",\n\t\t\tactions[Group%d]= [ ", group + 1);
		    for (level = 0;
                         level < XkbKeyGroupWidth(keymap, key, group);
                         level++) {
			if (level != 0)
			    write_buf(keymap, buf, size, offset, ", ");
			write_action(keymap, buf, size, offset, &acts[level],
                                     NULL, NULL);
		    }
		    write_buf(keymap, buf, size, offset, " ]");
		    acts += XkbKeyGroupsWidth(keymap, key);
		}
	    }
	    write_buf(keymap, buf, size, offset, "\n\t\t};\n");
	}
    }
    if (map && map->modmap) {
	for (key = keymap->min_key_code; key <= keymap->max_key_code; key++) {
            int mod;

            if (map->modmap[key] == 0)
                continue;

            for (mod = 0; mod < XkbNumModifiers; mod++) {
                if (!(map->modmap[key] & (1 << mod)))
                    continue;

                write_buf(keymap, buf, size, offset,
                          "\t\tmodifier_map %s { %s };\n",
                          get_mod_index_text(mod),
                          XkbcKeyNameText(darray_item(keymap->names->keys,
                                                      key).name));
            }
	}
    }

    write_buf(keymap, buf, size, offset, "\t};\n\n");
    return true;
}

_X_EXPORT char *
xkb_map_get_as_string(struct xkb_keymap *keymap)
{
    char *ret = NULL;
    size_t size = 0;
    size_t offset = 0;

    check_write_buf(keymap, &ret, &size, &offset, "xkb_keymap {\n");
    if (ret == NULL)
        return NULL;
    if (!write_keycodes(keymap, &ret, &size, &offset))
        return NULL;
    if (!write_types(keymap, &ret, &size, &offset))
        return NULL;
    if (!write_compat(keymap, &ret, &size, &offset))
        return NULL;
    if (!write_symbols(keymap, &ret, &size, &offset))
        return NULL;
    check_write_buf(keymap, &ret, &size, &offset, "};\n");
    if (ret == NULL)
        return NULL;

    return ret;
}
