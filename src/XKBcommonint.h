/*
Copyright 1985, 1987, 1990, 1998  The Open Group
Copyright 2008  Dan Nicholson

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the authors or their
institutions shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the authors.
*/

/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

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

#ifndef _XKBCOMMONINT_H_
#define _XKBCOMMONINT_H_

#include <stdlib.h>
#include <string.h>
#include <X11/extensions/XKB.h>

#include "xkbcommon/xkbcommon.h"

#ifndef True
#define True  1
#define False 0
#endif

typedef int Bool;

/* From XKM.h */
#define	XkmFileVersion		15

#define	XkmIllegalFile		-1
#define	XkmSemanticsFile	20
#define	XkmLayoutFile		21
#define	XkmKeymapFile		22
#define	XkmGeometryFile		23
#define	XkmRulesFile		24

#define	XkmTypesIndex		0
#define	XkmCompatMapIndex	1
#define	XkmSymbolsIndex		2
#define	XkmIndicatorsIndex	3
#define	XkmKeyNamesIndex	4
#define	XkmGeometryIndex	5
#define	XkmVirtualModsIndex	6
#define	XkmLastIndex		XkmVirtualModsIndex

#define	XkmTypesMask		(1<<0)
#define	XkmCompatMapMask	(1<<1)
#define	XkmSymbolsMask		(1<<2)
#define	XkmIndicatorsMask	(1<<3)
#define	XkmKeyNamesMask		(1<<4)
#define XkmGeometryMask         (1<<5)
#define	XkmVirtualModsMask	(1<<6)
#define	XkmLegalIndexMask	(0x7f)
#define	XkmAllIndicesMask	(0x7f)

#define	XkmSemanticsRequired	(XkmCompatMapMask)
#define	XkmSemanticsOptional	(XkmTypesMask|XkmVirtualModsMask|XkmIndicatorsMask|XkmGeometryMask)
#define	XkmSemanticsLegal	(XkmSemanticsRequired|XkmSemanticsOptional)
#define	XkmLayoutRequired	(XkmKeyNamesMask|XkmSymbolsMask|XkmTypesMask)
#define	XkmLayoutOptional	(XkmVirtualModsMask|XkmGeometryMask)
#define	XkmLayoutLegal		(XkmLayoutRequired|XkmLayoutOptional)
#define	XkmKeymapRequired	(XkmSemanticsRequired|XkmLayoutRequired)
#define	XkmKeymapOptional	((XkmSemanticsOptional|XkmLayoutOptional)&(~XkmKeymapRequired))
#define	XkmKeymapLegal		(XkmKeymapRequired|XkmKeymapOptional)

extern unsigned int xkb_key_get_group(struct xkb_state *state,
                                      xkb_keycode_t key);
extern unsigned int xkb_key_get_level(struct xkb_state *state,
                                      xkb_keycode_t key,
                                      unsigned int group);

struct xkb_any_action {
    uint8_t   type;
    uint8_t   data[7];
};

struct xkb_mod_action {
    uint8_t         type;
    uint8_t         flags;
    uint8_t         mask;
    uint8_t         real_mods;
    uint16_t        vmods;
};

struct xkb_group_action {
    uint8_t         type;
    uint8_t         flags;
    int16_t         group;
};

struct xkb_iso_action {
    uint8_t         type;
    uint8_t         flags;
    uint8_t         mask;
    uint8_t         real_mods;
    uint8_t         group;
    uint8_t         affect;
    uint16_t        vmods;
};

struct xkb_controls_action {
    uint8_t         type;
    uint8_t         flags;
    uint32_t        ctrls;
};

struct xkb_device_button_action {
    uint8_t         type;
    uint8_t         flags;
    uint8_t         count;
    uint8_t         button;
    uint8_t         device;
};

struct xkb_device_valuator_action {
    uint8_t         type;
    uint8_t         device;
    uint8_t         v1_what;
    uint8_t         v1_index;
    uint8_t         v1_value;
    uint8_t         v2_what;
    uint8_t         v2_index;
    uint8_t         v2_value;
};

struct xkb_pointer_default_action {
    uint8_t         type;
    uint8_t         flags;
    uint8_t         affect;
    uint8_t         value;
};

struct xkb_switch_screen_action {
    uint8_t         type;
    uint8_t         flags;
    uint8_t         screen;
};

struct xkb_redirect_key_action {
    uint8_t		type;
    xkb_keycode_t	new_key;
    uint8_t		mods_mask;
    uint8_t		mods;
    uint16_t		vmods_mask;
    uint16_t		vmods;
};

struct xkb_pointer_action {
    uint8_t	type;
    uint8_t	flags;
    uint16_t	x;
    uint16_t	y;
};

struct xkb_message_action {
    uint8_t	type;
    uint8_t	flags;
    uint8_t	message[6];
};

struct xkb_pointer_button_action {
    uint8_t	type;
    uint8_t	flags;
    uint8_t	count;
    uint8_t	button;
};

union xkb_action {
    struct xkb_any_action             any;
    struct xkb_mod_action             mods;
    struct xkb_group_action           group;
    struct xkb_iso_action             iso;
    struct xkb_controls_action        ctrls;
    struct xkb_device_button_action   devbtn;
    struct xkb_device_valuator_action devval;
    struct xkb_pointer_default_action dflt;
    struct xkb_switch_screen_action   screen;
    struct xkb_redirect_key_action    redirect; /* XXX wholly unnecessary? */
    struct xkb_pointer_action         ptr; /* XXX delete for DeviceValuator */
    struct xkb_pointer_button_action  btn; /* XXX delete for DeviceBtn */
    struct xkb_message_action         msg; /* XXX just delete */
    unsigned char                     type;
};

struct xkb_mods {
        uint32_t        mask;   /* effective mods */
        uint32_t        vmods;
        uint8_t         real_mods;
};

struct xkb_kt_map_entry {
        int            active;
        uint16_t        level;
        struct xkb_mods     mods;
};

struct xkb_key_type {
    struct xkb_mods             mods;
    uint16_t                num_levels;
    unsigned char           map_count;
    struct xkb_kt_map_entry *       map;
    struct xkb_mods *             preserve;
    const char              *name;
    const char              **level_names;
};

struct xkb_sym_interpret {
    xkb_keysym_t    sym;
    unsigned char   flags;
    unsigned char   match;
    uint8_t         mods;
    uint32_t        virtual_mod;
    union xkb_action act;
};

struct xkb_compat_map {
    struct xkb_sym_interpret *      sym_interpret;
    struct xkb_mods              groups[XkbNumKbdGroups];
    unsigned short           num_si;
    unsigned short           size_si;
};

struct xkb_sym_map {
	unsigned char	 kt_index[XkbNumKbdGroups];
	unsigned char	 group_info;
	unsigned char	 width;
        int              *sym_index; /* per level/group index into 'syms' */
        unsigned int     *num_syms; /* per level/group */
        xkb_keysym_t     *syms;
        unsigned int     size_syms; /* size of 'syms' */
};

#define	XkbNumGroups(g)			((g)&0x0f)
#define	XkbOutOfRangeGroupInfo(g)	((g)&0xf0)
#define	XkbOutOfRangeGroupAction(g)	((g)&0xc0)
#define	XkbOutOfRangeGroupNumber(g)	(((g)&0x30)>>4)
#define	XkbSetGroupInfo(g,w,n)	(((w)&0xc0)|(((n)&3)<<4)|((g)&0x0f))
#define	XkbSetNumGroups(g,n)	(((g)&0xf0)|((n)&0x0f))

struct xkb_client_map {
    unsigned char            size_types;
    unsigned char            num_types;
    struct xkb_key_type *           types;
    struct xkb_sym_map *             key_sym_map;
    unsigned char           *modmap;
};

struct xkb_behavior {
	unsigned char	type;
	unsigned char	data;
};

struct xkb_server_map {
    unsigned short      num_acts;
    unsigned short      size_acts;

#if defined(__cplusplus) || defined(c_plusplus)
    /* explicit is a C++ reserved word */
    unsigned char *     c_explicit;
#else
    unsigned char *     explicit;
#endif

    union xkb_action          *acts;
    struct xkb_behavior         *behaviors;
    unsigned short      *key_acts;
    unsigned char       *explicits;
    uint32_t            vmods[XkbNumVirtualMods]; /* vmod -> mod mapping */
    uint32_t            *vmodmap; /* key -> vmod mapping */
};


struct xkb_indicator_map {
	unsigned char	flags;
	unsigned char	which_groups;
	unsigned char	groups;
	unsigned char	which_mods;
	struct xkb_mods	mods;
	unsigned int	ctrls;
};

struct xkb_indicator {
	unsigned long	  	phys_indicators;
	struct xkb_indicator_map	maps[XkbNumIndicators];
};

struct xkb_key_name {
	char	name[XkbKeyNameLength];
};

struct xkb_key_alias {
	char	real[XkbKeyNameLength];
	char	alias[XkbKeyNameLength];
};

struct xkb_names {
    const char            *vmods[XkbNumVirtualMods];
    const char            *indicators[XkbNumIndicators];
    const char            *groups[XkbNumKbdGroups];
    struct xkb_key_name *     keys;
    struct xkb_key_alias *    key_aliases;

    xkb_keycode_t     num_keys;
    xkb_keycode_t     num_key_aliases;
};

struct xkb_controls {
	unsigned char	num_groups;
	unsigned char	groups_wrap;
	struct xkb_mods	internal;
	struct xkb_mods	ignore_lock;
	unsigned int	enabled_ctrls;
	unsigned short	repeat_delay;
	unsigned short	repeat_interval;
	unsigned short	slow_keys_delay;
	unsigned short	debounce_delay;
	unsigned short	ax_options;
	unsigned short	ax_timeout;
	unsigned short	axt_opts_mask;
	unsigned short	axt_opts_values;
	unsigned int	axt_ctrls_mask;
	unsigned int	axt_ctrls_values;
	unsigned char	*per_key_repeat;
};

/* Common keyboard description structure */
struct xkb_keymap {
    struct xkb_context  *context;

    unsigned int        refcnt;
    unsigned int        defined;
    unsigned short      flags;
    xkb_keycode_t       min_key_code;
    xkb_keycode_t       max_key_code;

    struct xkb_controls *      ctrls;
    struct xkb_server_map *    server;
    struct xkb_client_map *    map;
    struct xkb_indicator *     indicators;
    struct xkb_names *        names;
    struct xkb_compat_map *    compat;
};

#define	XkbKeyGroupInfo(d,k)    ((d)->map->key_sym_map[k].group_info)
#define	XkbKeyNumGroups(d,k)    (XkbNumGroups((d)->map->key_sym_map[k].group_info))
#define	XkbKeyGroupWidth(d,k,g) (XkbKeyType(d,k,g)->num_levels)
#define	XkbKeyGroupsWidth(d,k)  ((d)->map->key_sym_map[k].width)
#define	XkbKeyTypeIndex(d,k,g)  ((d)->map->key_sym_map[k].kt_index[g&0x3])
#define	XkbKeyType(d,k,g)	(&(d)->map->types[XkbKeyTypeIndex(d,k,g)])
#define	XkbKeyNumSyms(d,k,g,sl) \
        ((d)->map->key_sym_map[k].num_syms[(g*XkbKeyGroupsWidth(d,k))+sl])
#define	XkbKeySym(d,k,n)	(&(d)->map->key_sym_map[k].syms[n])
#define XkbKeySymOffset(d,k,g,sl) \
        ((d)->map->key_sym_map[k].sym_index[(g*XkbKeyGroupsWidth(d,k))+sl])
#define	XkbKeySymEntry(d,k,g,sl) \
	(XkbKeySym(d,k,XkbKeySymOffset(d,k,g,sl)))
#define	XkbKeyHasActions(d,k)	((d)->server->key_acts[k]!=0)
#define	XkbKeyNumActions(d,k)	\
        (XkbKeyHasActions(d,k)?(XkbKeyGroupsWidth(d,k)*XkbKeyNumGroups(d,k)):1)
#define	XkbKeyActionsPtr(d,k)   (&(d)->server->acts[(d)->server->key_acts[k]])
#define	XkbKeyAction(d,k,n) \
	(XkbKeyHasActions(d,k)?&XkbKeyActionsPtr(d,k)[n]:NULL)
#define	XkbKeyActionEntry(d,k,sl,g) \
	(XkbKeyHasActions(d,k)?\
		XkbKeyAction(d,k,((XkbKeyGroupsWidth(d,k)*(g))+(sl))):NULL)

#define	XkbKeycodeInRange(d,k)	(((k)>=(d)->min_key_code)&&\
				 ((k)<=(d)->max_key_code))
#define	XkbNumKeys(d)		((d)->max_key_code-(d)->min_key_code+1)

struct xkb_component_name {
	unsigned short		flags;
	char *			name;
};

struct xkb_state {
	xkb_group_index_t base_group; /**< depressed */
	xkb_group_index_t latched_group;
	xkb_group_index_t locked_group;
        xkb_group_index_t group; /**< effective */

	xkb_mod_mask_t base_mods; /**< depressed */
	xkb_mod_mask_t latched_mods;
	xkb_mod_mask_t locked_mods;
	xkb_mod_mask_t mods; /**< effective */

        uint32_t        leds;

        int refcnt;
        void *filters;
        int num_filters;
        struct xkb_keymap *xkb;
};

#define	XkbStateFieldFromRec(s)	XkbBuildCoreState((s)->lookup_mods,(s)->group)
#define	XkbGrabStateFromRec(s)	XkbBuildCoreState((s)->grab_mods,(s)->group)

#define	XkbNumGroups(g)			((g)&0x0f)
#define	XkbOutOfRangeGroupInfo(g)	((g)&0xf0)
#define	XkbOutOfRangeGroupAction(g)	((g)&0xc0)
#define	XkbOutOfRangeGroupNumber(g)	(((g)&0x30)>>4)
#define	XkbSetNumGroups(g,n)	(((g)&0xf0)|((n)&0x0f))

#endif /* _XKBCOMMONINT_H_ */
