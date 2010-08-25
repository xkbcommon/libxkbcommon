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

#ifndef X_PROTOCOL
typedef unsigned char KeyCode;
#endif

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

struct xkb_rule_names {
    char *  rules;
    char *  model;
    char *  layout;
    char *  variant;
    char *  options;
};

struct xkb_any_action {
    unsigned char   type;
    unsigned char   pad[18];
};

struct xkb_mod_action {
    unsigned char   type;
    uint8_t         flags;
    uint8_t         real_mods;
    uint32_t        mask;
    uint32_t        vmods;
};

struct xkb_group_action {
    unsigned char   type;
    unsigned char   flags;
    int16_t         group;
};

struct xkb_iso_action {
    unsigned char   type;
    uint8_t         flags;
    int16_t         group;
    uint32_t        mask;
    uint32_t        vmods;
    uint8_t         real_mods;
 
   uint8_t         affect;
};

struct xkb_controls_action {
    unsigned char   type;
    uint8_t         flags;
    uint32_t        ctrls;
};

struct xkb_device_button_action {
    unsigned char   type;
    uint8_t         flags;
    uint16_t        device;
    uint16_t        button;
    uint8_t         count;
};

struct xkb_device_valuator_action {
    unsigned char   type;
    uint8_t         v1_what;
    uint16_t        device;
    uint16_t        v1_index;
    int16_t         v1_value;
    uint16_t        v2_index;
    int16_t         v2_value;
    uint8_t         v2_what;
};

struct xkb_pointer_default_action {
    unsigned char   type;
    uint8_t         flags;
    uint8_t         affect;
    uint8_t         value;
};

struct xkb_switch_screen_action {
    unsigned char   type;
    uint8_t         flags;
    uint8_t         screen;
};

struct xkb_redirect_key_action {
	unsigned char	type;
	unsigned char	new_key;
	unsigned char	mods_mask;
	unsigned char	mods;
	unsigned char	vmods_mask0;
	unsigned char	vmods_mask1;
	unsigned char	vmods0;
	unsigned char	vmods1;
};
#define	XkbSARedirectVMods(a)		((((unsigned int)(a)->vmods1)<<8)|\
					((unsigned int)(a)->vmods0))
#define	XkbSARedirectSetVMods(a,m)	(((a)->vmods_mask1=(((m)>>8)&0xff)),\
					 ((a)->vmods_mask0=((m)&0xff)))
#define	XkbSARedirectVModsMask(a)	((((unsigned int)(a)->vmods_mask1)<<8)|\
					((unsigned int)(a)->vmods_mask0))
#define	XkbSARedirectSetVModsMask(a,m)	(((a)->vmods_mask1=(((m)>>8)&0xff)),\
					 ((a)->vmods_mask0=((m)&0xff)))


struct xkb_pointer_action {
	unsigned char	type;
	unsigned char	flags;
	unsigned char	high_XXX;
	unsigned char	low_XXX;
	unsigned char	high_YYY;
	unsigned char	low_YYY;
};
#define	XkbIntTo2Chars(i,h,l)	(((h)=((i>>8)&0xff)),((l)=((i)&0xff)))
#define	XkbPtrActionX(a)      (Xkb2CharsToInt((a)->high_XXX,(a)->low_XXX))
#define	XkbPtrActionY(a)      (Xkb2CharsToInt((a)->high_YYY,(a)->low_YYY))
#define	XkbSetPtrActionX(a,x) (XkbIntTo2Chars(x,(a)->high_XXX,(a)->low_XXX))
#define	XkbSetPtrActionY(a,y) (XkbIntTo2Chars(y,(a)->high_YYY,(a)->low_YYY))

struct xkb_message_action {
	unsigned char	type;
	unsigned char	flags;
	unsigned char	message[6];
};

struct xkb_pointer_button_action {
	unsigned char	type;
	unsigned char	flags;
	unsigned char	count;
	unsigned char	button;
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
    uint32_t                  name;
    uint32_t                 *level_names;
};

struct xkb_sym_interpret {
    uint32_t          sym;
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

    unsigned short           size_syms;
    unsigned short           num_syms;
    uint32_t                *syms;
    struct xkb_sym_map *             key_sym_map;

    unsigned char           *modmap;
};

#define	XkbCMKeyGroupInfo(m,k)  ((m)->key_sym_map[k].group_info)
#define	XkbCMKeyNumGroups(m,k)	 (XkbNumGroups((m)->key_sym_map[k].group_info))
#define	XkbCMKeyGroupWidth(m,k,g) (XkbCMKeyType(m,k,g)->num_levels)
#define	XkbCMKeyGroupsWidth(m,k) ((m)->key_sym_map[k].width)
#define	XkbCMKeyTypeIndex(m,k,g) ((m)->key_sym_map[k].kt_index[g&0x3])
#define	XkbCMKeyType(m,k,g)	 (&(m)->types[XkbCMKeyTypeIndex(m,k,g)])
#define	XkbCMKeyNumSyms(m,k) (XkbCMKeyGroupsWidth(m,k)*XkbCMKeyNumGroups(m,k))
#define	XkbCMKeySymsOffset(m,k)	((m)->key_sym_map[k].offset)
#define	XkbCMKeySymsPtr(m,k)	(&(m)->syms[XkbCMKeySymsOffset(m,k)])

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
    uint32_t            vmods[XkbNumVirtualMods];
    uint32_t            *vmodmap;
};

#define	XkbSMKeyActionsPtr(m,k) (&(m)->acts[(m)->key_acts[k]])

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
    uint32_t            keycodes;
    uint32_t            geometry;
    uint32_t            symbols;
    uint32_t            types;
    uint32_t            compat;
    uint32_t            vmods[XkbNumVirtualMods];
    uint32_t            indicators[XkbNumIndicators];
    uint32_t            groups[XkbNumKbdGroups];
    struct xkb_key_name *     keys;
    struct xkb_key_alias *    key_aliases;
    uint32_t           *radio_groups;
    uint32_t            phys_symbols;

    unsigned char     num_keys;
    unsigned char     num_key_aliases;
    unsigned short    num_rg;
};

struct xkb_property {
	char	*name;
	char	*value;
};

struct xkb_color {
	unsigned int 	pixel;
	char *		spec;
};

struct xkb_point {
	short	x;
	short	y;
};

struct xkb_bounds {
	short	x1,y1;
	short	x2,y2;
};

struct xkb_outline {
	unsigned short	num_points;
	unsigned short	sz_points;
	unsigned short	corner_radius;
	struct xkb_point *	points;
};

struct xkb_shape {
	uint32_t		 name;
	unsigned short	 num_outlines;
	unsigned short	 sz_outlines;
	struct xkb_outline *	 outlines;
	struct xkb_outline *	 approx;
	struct xkb_outline *	 primary;
	struct xkb_bounds	 bounds;
};

struct xkb_shape_doodad {
	uint32_t		 name;
	unsigned char	 type;
	unsigned char	 priority;
	short		 top;
	short		 left;
	short		 angle;
	unsigned short	 color_ndx;
	unsigned short	 shape_ndx;
};
#define	XkbShapeDoodadColor(g,d)	(&(g)->colors[(d)->color_ndx])
#define	XkbShapeDoodadShape(g,d)	(&(g)->shapes[(d)->shape_ndx])
#define	XkbSetShapeDoodadColor(g,d,c)	((d)->color_ndx= (c)-&(g)->colors[0])
#define	XkbSetShapeDoodadShape(g,d,s)	((d)->shape_ndx= (s)-&(g)->shapes[0])

struct xkb_text_doodad {
	uint32_t		 name;
	unsigned char	 type;
	unsigned char	 priority;
	short		 top;
	short		 left;
	short		 angle;
	short		 width;
	short		 height;
	unsigned short	 color_ndx;
	char *		 text;
	char *		 font;
};
#define	XkbTextDoodadColor(g,d)	(&(g)->colors[(d)->color_ndx])
#define	XkbSetTextDoodadColor(g,d,c)	((d)->color_ndx= (c)-&(g)->colors[0])

struct xkb_indicator_doodad {
	uint32_t		 name;
	unsigned char	 type;
	unsigned char	 priority;
	short		 top;
	short		 left;
	short		 angle;
	unsigned short	 shape_ndx;
	unsigned short	 on_color_ndx;
	unsigned short	 off_color_ndx;
};
#define	XkbIndicatorDoodadShape(g,d)	(&(g)->shapes[(d)->shape_ndx])
#define	XkbIndicatorDoodadOnColor(g,d)	(&(g)->colors[(d)->on_color_ndx])
#define	XkbIndicatorDoodadOffColor(g,d)	(&(g)->colors[(d)->off_color_ndx])
#define	XkbSetIndicatorDoodadOnColor(g,d,c) \
				((d)->on_color_ndx= (c)-&(g)->colors[0])
#define	XkbSetIndicatorDoodadOffColor(g,d,c) \
				((d)->off_color_ndx= (c)-&(g)->colors[0])
#define	XkbSetIndicatorDoodadShape(g,d,s) \
				((d)->shape_ndx= (s)-&(g)->shapes[0])

struct xkb_logo_doodad {
	uint32_t		 name;
	unsigned char	 type;
	unsigned char	 priority;
	short		 top;
	short		 left;
	short		 angle;
	unsigned short	 color_ndx;
	unsigned short	 shape_ndx;
	char *		 logo_name;
};
#define	XkbLogoDoodadColor(g,d)		(&(g)->colors[(d)->color_ndx])
#define	XkbLogoDoodadShape(g,d)		(&(g)->shapes[(d)->shape_ndx])
#define	XkbSetLogoDoodadColor(g,d,c)	((d)->color_ndx= (c)-&(g)->colors[0])
#define	XkbSetLogoDoodadShape(g,d,s)	((d)->shape_ndx= (s)-&(g)->shapes[0])

struct xkb_any_doodad {
	uint32_t		 name;
	unsigned char	 type;
	unsigned char	 priority;
	short		 top;
	short		 left;
	short		 angle;
};

union xkb_doodad {
	struct xkb_any_doodad	any;
	struct xkb_shape_doodad	shape;
	struct xkb_text_doodad	text;
	struct xkb_indicator_doodad	indicator;
	struct xkb_logo_doodad	logo;
};

#define	XkbUnknownDoodad	0
#define	XkbOutlineDoodad	1
#define	XkbSolidDoodad		2
#define	XkbTextDoodad		3
#define	XkbIndicatorDoodad	4
#define	XkbLogoDoodad		5

struct xkb_key {
	struct xkb_key_name	 name;
	short		 gap;
	unsigned char	 shape_ndx;
	unsigned char	 color_ndx;
};
#define	XkbKeyShape(g,k)	(&(g)->shapes[(k)->shape_ndx])
#define	XkbKeyColor(g,k)	(&(g)->colors[(k)->color_ndx])
#define	XkbSetKeyShape(g,k,s)	((k)->shape_ndx= (s)-&(g)->shapes[0])
#define	XkbSetKeyColor(g,k,c)	((k)->color_ndx= (c)-&(g)->colors[0])

struct xkb_row {
	short	 	top;
	short	 	left;
	unsigned short	num_keys;
	unsigned short	sz_keys;
	int		vertical;
	struct xkb_key *	keys;
	struct xkb_bounds	bounds;
};

struct xkb_section {
	uint32_t		 name;
	unsigned char	 priority;
	short	 	 top;
	short	 	 left;
	unsigned short	 width;
	unsigned short	 height;
	short	 	 angle;
	unsigned short	 num_rows;
	unsigned short	 num_doodads;
	unsigned short	 num_overlays;
	unsigned short	 sz_rows;
	unsigned short	 sz_doodads;
	unsigned short	 sz_overlays;
	struct xkb_row *	 rows;
	union xkb_doodad *	 doodads;
	struct xkb_bounds	 bounds;
	struct xkb_overlay *overlays;
};

struct xkb_overlay_key {
	struct xkb_key_name	over;
	struct xkb_key_name	under;
};

struct xkb_overlay_row {
	unsigned short		row_under;
	unsigned short		num_keys;
	unsigned short		sz_keys;
	struct xkb_overlay_key *	keys;
};

struct xkb_overlay {
	uint32_t			name;
	struct xkb_section *		section_under;
	unsigned short		num_rows;
	unsigned short		sz_rows;
	struct xkb_overlay_row *	rows;
	struct xkb_bounds *		bounds;
};

struct xkb_geometry {
	uint32_t		 name;
	unsigned short	 width_mm;
	unsigned short	 height_mm;
	char *		 label_font;
	struct xkb_color *	 label_color;
	struct xkb_color *	 base_color;
	unsigned short	 sz_properties;
	unsigned short	 sz_colors;
	unsigned short	 sz_shapes;
	unsigned short   sz_sections;
	unsigned short	 sz_doodads;
	unsigned short	 sz_key_aliases;
	unsigned short	 num_properties;
	unsigned short	 num_colors;
	unsigned short	 num_shapes;
	unsigned short	 num_sections;
	unsigned short	 num_doodads;
	unsigned short	 num_key_aliases;
	struct xkb_property *	 properties;
	struct xkb_color *	 colors;
	struct xkb_shape *	 shapes;
	struct xkb_section *	 sections;
	union xkb_doodad *	 doodads;
	struct xkb_key_alias *	 key_aliases;
};
#define	XkbGeomColorIndex(g,c)	((int)((c)-&(g)->colors[0]))

#define	XkbGeomPropertiesMask	(1<<0)
#define	XkbGeomColorsMask	(1<<1)
#define	XkbGeomShapesMask	(1<<2)
#define	XkbGeomSectionsMask	(1<<3)
#define	XkbGeomDoodadsMask	(1<<4)
#define	XkbGeomKeyAliasesMask	(1<<5)
#define	XkbGeomAllMask		(0x3f)

struct xkb_geometry_sizes {
	unsigned int	which;
	unsigned short	num_properties;
	unsigned short	num_colors;
	unsigned short	num_shapes;
	unsigned short	num_sections;
	unsigned short	num_doodads;
	unsigned short	num_key_aliases;
};

struct xkb_controls {
	unsigned char	mk_dflt_btn;
	unsigned char	num_groups;
	unsigned char	groups_wrap;
	struct xkb_mods	internal;
	struct xkb_mods	ignore_lock;
	unsigned int	enabled_ctrls;
	unsigned short	repeat_delay;
	unsigned short	repeat_interval;
	unsigned short	slow_keys_delay;
	unsigned short	debounce_delay;
	unsigned short	mk_delay;
	unsigned short	mk_interval;
	unsigned short	mk_time_to_max;
	unsigned short	mk_max_speed;
		 short	mk_curve;
	unsigned short	ax_options;
	unsigned short	ax_timeout;
	unsigned short	axt_opts_mask;
	unsigned short	axt_opts_values;
	unsigned int	axt_ctrls_mask;
	unsigned int	axt_ctrls_values;
	unsigned char	per_key_repeat[XkbPerKeyBitArraySize];
};

/* Common keyboard description structure */
struct xkb_desc {
    unsigned int        defined;
    unsigned short      flags;
    unsigned short      device_spec;
    KeyCode             min_key_code;
    KeyCode             max_key_code;

    struct xkb_controls *      ctrls;
    struct xkb_server_map *    server;
    struct xkb_client_map *    map;
    struct xkb_indicator *     indicators;
    struct xkb_names *        names;
    struct xkb_compat_map *    compat;
    struct xkb_geometry *     geom;
};

#define	XkbKeyKeyTypeIndex(d,k,g)	(XkbCMKeyTypeIndex((d)->map,k,g))
#define	XkbKeyKeyType(d,k,g)		(XkbCMKeyType((d)->map,k,g))
#define	XkbKeyGroupWidth(d,k,g)		(XkbCMKeyGroupWidth((d)->map,k,g))
#define	XkbKeyGroupsWidth(d,k)		(XkbCMKeyGroupsWidth((d)->map,k))
#define	XkbKeyGroupInfo(d,k)		(XkbCMKeyGroupInfo((d)->map,(k)))
#define	XkbKeyNumGroups(d,k)		(XkbCMKeyNumGroups((d)->map,(k)))
#define	XkbKeyNumSyms(d,k)		(XkbCMKeyNumSyms((d)->map,(k)))
#define	XkbKeySymsPtr(d,k)		(XkbCMKeySymsPtr((d)->map,(k)))
#define	XkbKeySym(d,k,n)		(XkbKeySymsPtr(d,k)[n])
#define	XkbKeySymEntry(d,k,sl,g) \
	(XkbKeySym(d,k,((XkbKeyGroupsWidth(d,k)*(g))+(sl))))
#define	XkbKeyAction(d,k,n) \
	(XkbKeyHasActions(d,k)?&XkbKeyActionsPtr(d,k)[n]:NULL)
#define	XkbKeyActionEntry(d,k,sl,g) \
	(XkbKeyHasActions(d,k)?\
		XkbKeyAction(d,k,((XkbKeyGroupsWidth(d,k)*(g))+(sl))):NULL)

#define	XkbKeyHasActions(d,k)	((d)->server->key_acts[k]!=0)
#define	XkbKeyNumActions(d,k)	(XkbKeyHasActions(d,k)?XkbKeyNumSyms(d,k):1)
#define	XkbKeyActionsPtr(d,k)	(XkbSMKeyActionsPtr((d)->server,k))
#define	XkbKeycodeInRange(d,k)	(((k)>=(d)->min_key_code)&&\
				 ((k)<=(d)->max_key_code))
#define	XkbNumKeys(d)		((d)->max_key_code-(d)->min_key_code+1)

struct xkb_map_changes {
	unsigned short		 changed;
	KeyCode			 min_key_code;
	KeyCode			 max_key_code;
	unsigned char		 first_type;
	unsigned char		 num_types;
	KeyCode			 first_key_sym;
	unsigned char		 num_key_syms;
	KeyCode			 first_key_act;
	unsigned char		 num_key_acts;
	KeyCode			 first_key_behavior;
	unsigned char		 num_key_behaviors;
	KeyCode 		 first_key_explicit;
	unsigned char		 num_key_explicit;
	KeyCode			 first_modmap_key;
	unsigned char		 num_modmap_keys;
	KeyCode			 first_vmodmap_key;
	unsigned char		 num_vmodmap_keys;
	unsigned char		 pad;
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
	unsigned char		num_aliases;
	unsigned char		num_rg;
	unsigned char		first_key;
	unsigned char		num_keys;
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
	char *			 geometry;
};

struct xkb_component_name {
	unsigned short		flags;
	char *			name;
};

struct xkb_component_list {
	int			num_keymaps;
	int			num_keycodes;
	int			num_types;
	int			num_compat;
	int			num_symbols;
	int			num_geometry;
	struct xkb_component_name *	keymaps;
	struct xkb_component_name * 	keycodes;
	struct xkb_component_name *	types;
	struct xkb_component_name *	compat;
	struct xkb_component_name *	symbols;
	struct xkb_component_name *	geometry;
};

_XFUNCPROTOBEGIN

typedef uint32_t (*InternAtomFuncPtr)(const char *val);
typedef const char *(*GetAtomValueFuncPtr)(uint32_t atom);

_X_EXPORT extern void
xkb_init_atoms(InternAtomFuncPtr intern, GetAtomValueFuncPtr get_atom_value);

_X_EXPORT extern struct xkb_desc *
xkb_compile_keymap_from_rules(const struct xkb_rule_names *rules);

_X_EXPORT extern struct xkb_desc *
xkb_compile_keymap_from_components(const struct xkb_component_names * ktcsg);

_X_EXPORT extern struct xkb_desc *
xkb_compile_keymap_from_file(FILE *inputFile, const char *mapName);

_X_EXPORT extern struct xkb_component_list *
xkb_list_components(struct xkb_component_names * ptrns, int *maxMatch);

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
 *
 * The string returned may become invalidated after the next call to
 * xkb_keysym_to_string: if you need to preserve it, then you must
 * duplicate it.
 *
 * This is uint32_t rather than KeySym, as KeySym changes size between
 * client and server (no, really).
 */
_X_EXPORT extern char *
xkb_keysym_to_string(uint32_t ks);

/*
 * See xkb_keysym_to_string comments: this function will accept any string
 * from that function.
 */
_X_EXPORT extern uint32_t
xkb_string_to_keysym(const char *s);

_XFUNCPROTOEND

#endif /* _XKBCOMMON_H_ */
