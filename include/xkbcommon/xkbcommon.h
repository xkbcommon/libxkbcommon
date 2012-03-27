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
typedef uint32_t xkb_mod_index_t;
typedef uint32_t xkb_mod_mask_t;
typedef uint32_t xkb_group_index_t;
typedef uint32_t xkb_led_index_t;

#define XKB_MOD_INVALID                 (0xffffffff)
#define XKB_GROUP_INVALID               (0xffffffff)
#define XKB_KEYCODE_INVALID             (0xffffffff)
#define XKB_LED_INVALID                 (0xffffffff)

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
    unsigned int        refcnt;
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
        struct xkb_desc *xkb;
};

#define	XkbStateFieldFromRec(s)	XkbBuildCoreState((s)->lookup_mods,(s)->group)
#define	XkbGrabStateFromRec(s)	XkbBuildCoreState((s)->grab_mods,(s)->group)

#define	XkbNumGroups(g)			((g)&0x0f)
#define	XkbOutOfRangeGroupInfo(g)	((g)&0xf0)
#define	XkbOutOfRangeGroupAction(g)	((g)&0xc0)
#define	XkbOutOfRangeGroupNumber(g)	(((g)&0x30)>>4)
#define	XkbSetNumGroups(g,n)	(((g)&0xf0)|((n)&0x0f))

_XFUNCPROTOBEGIN

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
xkb_key_get_syms(struct xkb_state *state, xkb_keycode_t key,
                 xkb_keysym_t **syms_out);

/**
 * @defgroup map Keymap management
 * These utility functions allow you to create and deallocate XKB keymaps.
 *
 * @{
 */

/**
 * The primary keymap entry point: creates a new XKB keymap from a set of
 * RMLVO (Rules + Model + Layout + Variant + Option) names.
 *
 * You should almost certainly be using this and nothing else to create
 * keymaps.
 */
_X_EXPORT extern struct xkb_desc *
xkb_map_new_from_names(const struct xkb_rule_names *names);

/**
 * Deprecated entrypoint for legacy users who need to be able to compile
 * XKB keymaps by KcCGST (Keycodes + Compat + Geometry + Symbols + Types)
 * names.
 *
 * You should not use this unless you are the X server.  This entrypoint
 * may well disappear in future releases.  Please, please, don't use it.
 *
 * Geometry will be ignored since xkbcommon does not support it in any way.
 */
_X_EXPORT extern struct xkb_desc *
xkb_map_new_from_kccgst(const struct xkb_component_names *kccgst);

enum xkb_keymap_format {
    /** The current/classic XKB text format, as generated by xkbcomp -xkb. */
    XKB_KEYMAP_FORMAT_TEXT_V1 = 1,
};

/**
 * Creates an XKB keymap from a full text XKB keymap passed into the
 * file descriptor.
 */
_X_EXPORT extern struct xkb_desc *
xkb_map_new_from_fd(int fd, enum xkb_keymap_format format);

/**
 * Creates an XKB keymap from a full text XKB keymap serialised into one
 * enormous string.
 */
_X_EXPORT extern struct xkb_desc *
xkb_map_new_from_string(const char *string, enum xkb_keymap_format format);

/**
 * Takes a new reference on a keymap.
 */
_X_EXPORT extern void
xkb_map_ref(struct xkb_desc *xkb);

/**
 * Releases a reference on a keymap.
 */
_X_EXPORT extern void
xkb_map_unref(struct xkb_desc *xkb);

/** @} */

/**
 * @defgroup components XKB state components
 * Allows enumeration of state components such as modifiers and groups within
 * the current keymap.
 *
 * @{
 */

/**
 * Returns the number of modifiers active in the keymap.
 */
_X_EXPORT xkb_mod_index_t
xkb_map_num_mods(struct xkb_desc *xkb);

/**
 * Returns the name of the modifier specified by 'idx', or NULL if invalid.
 */
_X_EXPORT const char *
xkb_map_mod_get_name(struct xkb_desc *xkb, xkb_mod_index_t idx);

/**
 * Returns the index of the modifier specified by 'name', or XKB_MOD_INVALID.
 */
_X_EXPORT xkb_mod_index_t
xkb_map_mod_get_index(struct xkb_desc *xkb, const char *name);

/**
 * Returns the number of groups active in the keymap.
 */
_X_EXPORT xkb_group_index_t
xkb_map_num_groups(struct xkb_desc *xkb);

/**
 * Returns the name of the group specified by 'idx', or NULL if invalid.
 */
_X_EXPORT const char *
xkb_map_group_get_name(struct xkb_desc *xkb, xkb_group_index_t idx);

/**
 * Returns the index of the group specified by 'name', or XKB_GROUP_INVALID.
 */
_X_EXPORT xkb_group_index_t
xkb_map_group_get_index(struct xkb_desc *xkb, const char *name);

/**
 * Returns the number of groups active for the specified key.
 */
_X_EXPORT xkb_group_index_t
xkb_key_num_groups(struct xkb_desc *xkb, xkb_keycode_t key);

/**
 * Returns the number of LEDs in the given map.
 */
_X_EXPORT xkb_led_index_t
xkb_map_num_leds(struct xkb_desc *xkb);

/**
 * Returns the name of the LED specified by 'idx', or NULL if invalid.
 */
_X_EXPORT const char *
xkb_map_led_get_name(struct xkb_desc *xkb, xkb_led_index_t idx);

/**
 * Returns the index of the LED specified by 'name', or XKB_LED_INVALID.
 */
_X_EXPORT xkb_led_index_t
xkb_map_led_get_index(struct xkb_desc *xkb, const char *name);

/** @} */

/**
 * @defgroup state XKB state objects
 * Creation, destruction and manipulation of keyboard state objects,
 * representing modifier and group state.
 *
 * @{
 */

/**
 * Allocates a new XKB state object for use with the given keymap.
 */
_X_EXPORT struct xkb_state *
xkb_state_new(struct xkb_desc *xkb);

/**
 * Adds a reference to a state object, so it will not be freed until unref.
 */
_X_EXPORT void
xkb_state_ref(struct xkb_state *state);

/**
 * Unrefs and potentially deallocates a state object; the caller must not
 * use the state object after calling this.
 */
_X_EXPORT void
xkb_state_unref(struct xkb_state *state);

enum xkb_key_direction {
    XKB_KEY_UP,
    XKB_KEY_DOWN,
};

/**
 * Updates a state object to reflect the given key being pressed or released.
 */
_X_EXPORT void
xkb_state_update_key(struct xkb_state *state, xkb_keycode_t key,
                     enum xkb_key_direction direction);

/**
 * Modifier and group types for state objects.  This enum is bitmaskable,
 * e.g. (XKB_STATE_DEPRESSED | XKB_STATE_LATCHED) is valid to exclude
 * locked modifiers.
 */
enum xkb_state_component {
    XKB_STATE_DEPRESSED = (1 << 0),
        /**< A key holding this modifier or group is currently physically
         *   depressed; also known as 'base'. */
    XKB_STATE_LATCHED = (1 << 1),
        /**< Modifier or group is latched, i.e. will be unset after the next
         *   non-modifier key press. */
    XKB_STATE_LOCKED = (1 << 2),
        /**< Modifier or group is locked, i.e. will be unset after the key
         *   provoking the lock has been pressed again. */
    XKB_STATE_EFFECTIVE =
        (XKB_STATE_DEPRESSED | XKB_STATE_LATCHED | XKB_STATE_LOCKED),
        /**< Combinatination of depressed, latched, and locked. */
};

/**
 * Updates a state object from a set of explicit masks.  This entrypoint is
 * really only for window systems and the like, where a master process
 * holds an xkb_state, then serialises it over a wire protocol, and clients
 * then use the serialisation to feed in to their own xkb_state.
 *
 * All parameters must always be passed, or the resulting state may be
 * incoherent.
 *
 * The serialisation is lossy and will not survive round trips; it must only
 * be used to feed slave state objects, and must not be used to update the
 * master state.
 *
 * Please do not use this unless you fit the description above.
 */
_X_EXPORT void
xkb_state_update_mask(struct xkb_state *state,
                      xkb_mod_mask_t base_mods,
                      xkb_mod_mask_t latched_mods,
                      xkb_mod_mask_t locked_mods,
                      xkb_group_index_t base_group,
                      xkb_group_index_t latched_group,
                      xkb_group_index_t locked_group);

/**
 * The counterpart to xkb_state_update_mask, to be used on the server side
 * of serialisation.  Returns a xkb_mod_mask_t representing the given
 * component(s) of the state.
 *
 * This function should not be used in regular clients; please use the
 * xkb_state_mod_*_is_active or xkb_state_foreach_active_mod API instead.
 *
 * Can return NULL on failure.
 */
_X_EXPORT xkb_mod_mask_t
xkb_state_serialise_mods(struct xkb_state *state,
                         enum xkb_state_component component);

/**
 * The group equivalent of xkb_state_serialise_mods: please see its
 * documentation.
 */
_X_EXPORT xkb_group_index_t
xkb_state_serialise_group(struct xkb_state *state,
                          enum xkb_state_component component);

/**
 * Returns 1 if the modifier specified by 'name' is active in the manner
 * specified by 'type', 0 if it is unset, or -1 if the modifier does not
 * exist in the current map.
 */
_X_EXPORT int
xkb_state_mod_name_is_active(struct xkb_state *state, const char *name,
                             enum xkb_state_component type);

/**
 * Returns 1 if the modifier specified by 'idx' is active in the manner
 * specified by 'type', 0 if it is unset, or -1 if the modifier does not
 * exist in the current map.
 */
_X_EXPORT int
xkb_state_mod_index_is_active(struct xkb_state *state, xkb_mod_index_t idx,
                              enum xkb_state_component type);

/**
 * Returns 1 if the group specified by 'name' is active in the manner
 * specified by 'type', 0 if it is unset, or -1 if the group does not
 * exist in the current map.
 */
_X_EXPORT int
xkb_state_group_name_is_active(struct xkb_state *state, const char *name,
                               enum xkb_state_component type);

/**
 * Returns 1 if the group specified by 'idx' is active in the manner
 * specified by 'type', 0 if it is unset, or -1 if the group does not
 * exist in the current map.
 */
_X_EXPORT int
xkb_state_group_index_is_active(struct xkb_state *state, xkb_group_index_t idx,
                                enum xkb_state_component type);

/**
 * Returns 1 if the LED specified by 'name' is active, 0 if it is unset, or
 * -1 if the LED does not exist in the current map.
 */
_X_EXPORT int
xkb_state_led_name_is_active(struct xkb_state *state, const char *name);

/**
 * Returns 1 if the LED specified by 'idx' is active, 0 if it is unset, or
 * -1 if the LED does not exist in the current map.
 */
_X_EXPORT int
xkb_state_led_index_is_active(struct xkb_state *state, xkb_led_index_t idx);

/** @} */

_XFUNCPROTOEND

#endif /* _XKBCOMMON_H_ */
