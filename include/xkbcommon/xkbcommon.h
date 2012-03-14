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


#ifndef _XKBCOMMON_H_
#define _XKBCOMMON_H_

#include <stdint.h>
#include <stdio.h>
#include <X11/Xfuncproto.h>
#include <X11/extensions/XKB.h>

typedef uint32_t xkb_keycode_t;
typedef uint32_t xkb_keysym_t;

#define XKB_KEYCODE_MAX                 (0xffffffff - 1)
#define xkb_keycode_is_legal_ext(kc)    (kc <= XKB_KEYCODE_MAX)
#define xkb_keycode_is_legal_x11(kc)    (kc <= XKB_KEYCODE_MAX)
#define xkb_keymap_keycode_range_is_legal(xkb) \
    (xkb->max_key_code > 0 && \
     xkb->max_key_code > xkb->min_key_code && \
     xkb_keycode_is_legal_ext(xkb->min_key_code) && \
     xkb_keycode_is_legal_ext(xkb->max_key_code))

/* Duplicate the modifier mask defines so libxkcommon can be used
 * without X.h */
#define XKB_COMMON_SHIFT_MASK		(1 << 0)
#define XKB_COMMON_LOCK_MASK		(1 << 1)
#define XKB_COMMON_CONTROL_MASK		(1 << 2)
#define XKB_COMMON_MOD1_MASK		(1 << 3)
#define XKB_COMMON_MOD2_MASK		(1 << 4)
#define XKB_COMMON_MOD3_MASK		(1 << 5)
#define XKB_COMMON_MOD4_MASK		(1 << 6)
#define XKB_COMMON_MOD5_MASK		(1 << 7)


struct xkb_rule_names {
    const char *  rules;
    const char *  model;
    const char *  layout;
    const char *  variant;
    const char *  options;
};

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
    uint8_t         mods; /* XXX real or virt? */
    uint32_t        virtual_mod;
    struct xkb_any_action   act;
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
	unsigned short	 offset;
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

    uint32_t                 size_syms;
    uint32_t                 num_syms;
    xkb_keysym_t             *syms;
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
struct xkb_desc {
    unsigned int        defined;
    unsigned short      flags;
    unsigned short      device_spec;
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
#define	XkbKeyNumSyms(d,k)      (XkbKeyGroupsWidth(d,k)*XkbKeyNumGroups(d,k))
#define	XkbKeySymsOffset(d,k)	((d)->map->key_sym_map[k].offset)
#define	XkbKeySymsPtr(d,k)	(&(d)->map->syms[XkbKeySymsOffset(d,k)])
#define	XkbKeySym(d,k,n)	(XkbKeySymsPtr(d,k)[n])
#define	XkbKeySymEntry(d,k,sl,g) \
	(XkbKeySym(d,k,((XkbKeyGroupsWidth(d,k)*(g))+(sl))))
#define	XkbKeyHasActions(d,k)	((d)->server->key_acts[k]!=0)
#define	XkbKeyNumActions(d,k)	(XkbKeyHasActions(d,k)?XkbKeyNumSyms(d,k):1)
#define	XkbKeyActionsPtr(d,k)   (&(d)->server->acts[(d)->server->key_acts[k]])
#define	XkbKeyAction(d,k,n) \
	(XkbKeyHasActions(d,k)?&XkbKeyActionsPtr(d,k)[n]:NULL)
#define	XkbKeyActionEntry(d,k,sl,g) \
	(XkbKeyHasActions(d,k)?\
		XkbKeyAction(d,k,((XkbKeyGroupsWidth(d,k)*(g))+(sl))):NULL)

#define	XkbKeycodeInRange(d,k)	(((k)>=(d)->min_key_code)&&\
				 ((k)<=(d)->max_key_code))
#define	XkbNumKeys(d)		((d)->max_key_code-(d)->min_key_code+1)

struct xkb_map_changes {
	unsigned short		 changed;
	xkb_keycode_t		 min_key_code;
	xkb_keycode_t		 max_key_code;
	unsigned char		 first_type;
	unsigned char		 num_types;
	xkb_keycode_t		 first_key_sym;
	xkb_keycode_t		 num_key_syms;
	xkb_keycode_t		 first_key_act;
	xkb_keycode_t		 num_key_acts;
	xkb_keycode_t		 first_key_behavior;
	xkb_keycode_t		 num_key_behaviors;
	xkb_keycode_t		 first_key_explicit;
	xkb_keycode_t		 num_key_explicit;
	xkb_keycode_t		 first_modmap_key;
	xkb_keycode_t		 num_modmap_keys;
	xkb_keycode_t		 first_vmodmap_key;
	xkb_keycode_t		 num_vmodmap_keys;
	unsigned short		 vmods;
};

struct xkb_controls_changes {
	unsigned int 		 changed_ctrls;
	unsigned int		 enabled_ctrls_changes;
	int			 num_groups_changed;
};

struct xkb_indicator_changes {
	unsigned int		 state_changes;
	unsigned int		 map_changes;
};

struct xkb_name_changes {
	unsigned int 		changed;
	unsigned char		first_type;
	unsigned char		num_types;
	unsigned char		first_lvl;
	unsigned char		num_lvls;
	xkb_keycode_t		num_aliases;
	xkb_keycode_t		first_key;
	xkb_keycode_t		num_keys;
	unsigned short		changed_vmods;
	unsigned long		changed_indicators;
	unsigned char		changed_groups;
};

struct xkb_compat_changes {
	unsigned char		changed_groups;
	unsigned short		first_si;
	unsigned short		num_si;
};

struct xkb_changes {
	unsigned short		 device_spec;
	unsigned short		 state_changes;
	struct xkb_map_changes	 map;
	struct xkb_controls_changes	 ctrls;
	struct xkb_indicator_changes	 indicators;
	struct xkb_name_changes	 names;
	struct xkb_compat_changes	 compat;
};

struct xkb_component_names {
	char *			 keymap;
	char *			 keycodes;
	char *			 types;
	char *			 compat;
	char *			 symbols;
};

struct xkb_component_name {
	unsigned short		flags;
	char *			name;
};

struct xkb_state {
	unsigned char   group; /* base + latched + locked */
	/* FIXME: Why are base + latched short and not char?? */
	unsigned short  base_group; /* physically ... down? */
	unsigned short  latched_group;
	unsigned char   locked_group;

	unsigned char   mods; /* base + latched + locked */
	unsigned char   base_mods; /* physically down */
	unsigned char   latched_mods;
	unsigned char   locked_mods;

	unsigned char   compat_state; /* mods + group for core state */

	/* grab mods = all depressed and latched mods, _not_ locked mods */
	unsigned char   grab_mods; /* grab mods minus internal mods */
	unsigned char   compat_grab_mods; /* grab mods + group for core state,
	                                     but not locked groups if
                                             IgnoreGroupLocks set */

	/* effective mods = all mods (depressed, latched, locked) */
	unsigned char   lookup_mods; /* effective mods minus internal mods */
	unsigned char   compat_lookup_mods; /* effective mods + group */

	unsigned short  ptr_buttons; /* core pointer buttons */
};

#define	XkbStateFieldFromRec(s)	XkbBuildCoreState((s)->lookup_mods,(s)->group)
#define	XkbGrabStateFromRec(s)	XkbBuildCoreState((s)->grab_mods,(s)->group)

#define	XkbNumGroups(g)			((g)&0x0f)
#define	XkbOutOfRangeGroupInfo(g)	((g)&0xf0)
#define	XkbOutOfRangeGroupAction(g)	((g)&0xc0)
#define	XkbOutOfRangeGroupNumber(g)	(((g)&0x30)>>4)
#define	XkbSetNumGroups(g,n)	(((g)&0xf0)|((n)&0x0f))

_XFUNCPROTOBEGIN

_X_EXPORT extern struct xkb_desc *
xkb_compile_keymap_from_rules(const struct xkb_rule_names *rules);

_X_EXPORT extern struct xkb_desc *
xkb_compile_keymap_from_components(const struct xkb_component_names * ktcsg);

_X_EXPORT extern struct xkb_desc *
xkb_compile_keymap_from_file(FILE *inputFile, const char *mapName);

_X_EXPORT extern struct xkb_desc *
xkb_compile_keymap_from_string(const char *string, const char *mapName);

_X_EXPORT extern void
xkb_free_keymap(struct xkb_desc *xkb);

/*
 * Canonicalises component names by prepending the relevant component from
 * 'old' to the one in 'names' when the latter has a leading '+' or '|', and
 * by replacing a '%' with the relevant component, e.g.:
 *
 * names        old           output
 * ------------------------------------------
 * +bar         foo           foo+bar
 * |quux        baz           baz|quux
 * foo+%|baz    bar           foo+bar|baz
 *
 * If a component in names needs to be modified, the existing value will be
 * free()d, and a new one allocated with malloc().
 */
_X_EXPORT extern void
xkb_canonicalise_components(struct xkb_component_names * names,
                           const struct xkb_component_names * old);

/*
 * Converts a keysym to a string; will return unknown Unicode codepoints
 * as "Ua1b2", and other unknown keysyms as "0xabcd1234".
 */
_X_EXPORT extern void
xkb_keysym_to_string(xkb_keysym_t ks, char *buffer, size_t size);

/*
 * See xkb_keysym_to_string comments: this function will accept any string
 * from that function.
 */
_X_EXPORT extern xkb_keysym_t
xkb_string_to_keysym(const char *s);

_X_EXPORT unsigned int
xkb_key_get_syms(struct xkb_desc *xkb, struct xkb_state *state,
                 xkb_keycode_t key, xkb_keysym_t **syms_out);

_XFUNCPROTOEND

#endif /* _XKBCOMMON_H_ */
