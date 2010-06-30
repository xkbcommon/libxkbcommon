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
#include <X11/Xmd.h>
#include <X11/extensions/XKBstrcommon.h>
#include <X11/extensions/XKBrulescommon.h>

/* Action structures used in the server */

#define XkbcAnyActionDataSize 18
struct xkb_any_action {
    unsigned char   type;
    unsigned char   pad[XkbcAnyActionDataSize];
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

struct xkb_pointer_action {
	unsigned char	type;
	unsigned char	flags;
	unsigned char	high_XXX;
	unsigned char	low_XXX;
	unsigned char	high_YYY;
	unsigned char	low_YYY;
};

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

typedef struct _XkbcMods {
        uint32_t        mask;   /* effective mods */
        uint32_t        vmods;
        uint8_t         real_mods;
} XkbcModsRec, *XkbcModsPtr;

typedef struct _XkbcKTMapEntry {
        Bool            active;
        uint16_t        level;
        XkbcModsRec     mods;
} XkbcKTMapEntryRec, *XkbcKTMapEntryPtr;

typedef struct _XkbcKeyType {
    XkbcModsRec             mods;
    uint16_t                num_levels;
    unsigned char           map_count;
    XkbcKTMapEntryPtr       map;
    XkbcModsPtr             preserve;
    uint32_t                  name;
    uint32_t                 *level_names;
} XkbcKeyTypeRec, *XkbcKeyTypePtr;

typedef struct _XkbcSymInterpretRec {
    uint32_t          sym;
    unsigned char   flags;
    unsigned char   match;
    uint8_t         mods; /* XXX real or virt? */
    uint32_t        virtual_mod;
    struct xkb_any_action   act;
} XkbcSymInterpretRec, *XkbcSymInterpretPtr;

typedef struct _XkbcCompatMapRec {
    XkbcSymInterpretPtr      sym_interpret;
    XkbcModsRec              groups[XkbNumKbdGroups];
    unsigned short           num_si;
    unsigned short           size_si;
} XkbcCompatMapRec, *XkbcCompatMapPtr;

typedef struct _XkbcClientMapRec {
    unsigned char            size_types;
    unsigned char            num_types;
    XkbcKeyTypePtr           types;

    unsigned short           size_syms;
    unsigned short           num_syms;
    uint32_t                *syms;
    XkbSymMapPtr             key_sym_map;

    unsigned char           *modmap;
} XkbcClientMapRec, *XkbcClientMapPtr;

typedef struct _XkbcServerMapRec {
    unsigned short      num_acts;
    unsigned short      size_acts;

#if defined(__cplusplus) || defined(c_plusplus)
    /* explicit is a C++ reserved word */
    unsigned char *     c_explicit;
#else
    unsigned char *     explicit;
#endif

    union xkb_action          *acts;
    XkbBehavior         *behaviors;
    unsigned short      *key_acts;
    unsigned char       *explicits;
    uint32_t            vmods[XkbNumVirtualMods];
    uint32_t            *vmodmap;
} XkbcServerMapRec, *XkbcServerMapPtr;

typedef struct _XkbcNamesRec {
    uint32_t            keycodes;
    uint32_t            geometry;
    uint32_t            symbols;
    uint32_t            types;
    uint32_t            compat;
    uint32_t            vmods[XkbNumVirtualMods];
    uint32_t            indicators[XkbNumIndicators];
    uint32_t            groups[XkbNumKbdGroups];
    XkbKeyNamePtr     keys;
    XkbKeyAliasPtr    key_aliases;
    uint32_t           *radio_groups;
    uint32_t            phys_symbols;

    unsigned char     num_keys;
    unsigned char     num_key_aliases;
    unsigned short    num_rg;
} XkbcNamesRec, *XkbcNamesPtr;

typedef	struct _XkbcProperty {
	char	*name;
	char	*value;
} XkbcPropertyRec, *XkbcPropertyPtr;

typedef struct _XkbcColor {
	unsigned int 	pixel;
	char *		spec;
} XkbcColorRec, *XkbcColorPtr;

typedef	struct _XkbcPoint {
	short	x;
	short	y;
} XkbcPointRec, *XkbcPointPtr;

typedef struct	_XkbcBounds {
	short	x1,y1;
	short	x2,y2;
} XkbcBoundsRec, *XkbcBoundsPtr;
#define	XkbBoundsWidth(b)	(((b)->x2)-((b)->x1))
#define	XkbBoundsHeight(b)	(((b)->y2)-((b)->y1))

typedef struct _XkbcOutline {
	unsigned short	num_points;
	unsigned short	sz_points;
	unsigned short	corner_radius;
	XkbcPointPtr	points;
} XkbcOutlineRec, *XkbcOutlinePtr;

typedef struct _XkbcShape {
	uint32_t		 name;
	unsigned short	 num_outlines;
	unsigned short	 sz_outlines;
	XkbcOutlinePtr	 outlines;
	XkbcOutlinePtr	 approx;
	XkbcOutlinePtr	 primary;
	XkbcBoundsRec	 bounds;
} XkbcShapeRec, *XkbcShapePtr;
#define	XkbOutlineIndex(s,o)	((int)((o)-&(s)->outlines[0]))

typedef struct _XkbcShapeDoodad {
	uint32_t		 name;
	unsigned char	 type;
	unsigned char	 priority;
	short		 top;
	short		 left;
	short		 angle;
	unsigned short	 color_ndx;
	unsigned short	 shape_ndx;
} XkbcShapeDoodadRec, *XkbcShapeDoodadPtr;
#define	XkbShapeDoodadColor(g,d)	(&(g)->colors[(d)->color_ndx])
#define	XkbShapeDoodadShape(g,d)	(&(g)->shapes[(d)->shape_ndx])
#define	XkbSetShapeDoodadColor(g,d,c)	((d)->color_ndx= (c)-&(g)->colors[0])
#define	XkbSetShapeDoodadShape(g,d,s)	((d)->shape_ndx= (s)-&(g)->shapes[0])

typedef struct _XkbcTextDoodad {
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
} XkbcTextDoodadRec, *XkbcTextDoodadPtr;
#define	XkbTextDoodadColor(g,d)	(&(g)->colors[(d)->color_ndx])
#define	XkbSetTextDoodadColor(g,d,c)	((d)->color_ndx= (c)-&(g)->colors[0])

typedef struct _XkbcIndicatorDoodad {
	uint32_t		 name;
	unsigned char	 type;
	unsigned char	 priority;
	short		 top;
	short		 left;
	short		 angle;
	unsigned short	 shape_ndx;
	unsigned short	 on_color_ndx;
	unsigned short	 off_color_ndx;
} XkbcIndicatorDoodadRec, *XkbcIndicatorDoodadPtr;
#define	XkbIndicatorDoodadShape(g,d)	(&(g)->shapes[(d)->shape_ndx])
#define	XkbIndicatorDoodadOnColor(g,d)	(&(g)->colors[(d)->on_color_ndx])
#define	XkbIndicatorDoodadOffColor(g,d)	(&(g)->colors[(d)->off_color_ndx])
#define	XkbSetIndicatorDoodadOnColor(g,d,c) \
				((d)->on_color_ndx= (c)-&(g)->colors[0])
#define	XkbSetIndicatorDoodadOffColor(g,d,c) \
				((d)->off_color_ndx= (c)-&(g)->colors[0])
#define	XkbSetIndicatorDoodadShape(g,d,s) \
				((d)->shape_ndx= (s)-&(g)->shapes[0])

typedef struct _XkbcLogoDoodad {
	uint32_t		 name;
	unsigned char	 type;
	unsigned char	 priority;
	short		 top;
	short		 left;
	short		 angle;
	unsigned short	 color_ndx;
	unsigned short	 shape_ndx;
	char *		 logo_name;
} XkbcLogoDoodadRec, *XkbcLogoDoodadPtr;
#define	XkbLogoDoodadColor(g,d)		(&(g)->colors[(d)->color_ndx])
#define	XkbLogoDoodadShape(g,d)		(&(g)->shapes[(d)->shape_ndx])
#define	XkbSetLogoDoodadColor(g,d,c)	((d)->color_ndx= (c)-&(g)->colors[0])
#define	XkbSetLogoDoodadShape(g,d,s)	((d)->shape_ndx= (s)-&(g)->shapes[0])

typedef struct _XkbcAnyDoodad {
	uint32_t		 name;
	unsigned char	 type;
	unsigned char	 priority;
	short		 top;
	short		 left;
	short		 angle;
} XkbcAnyDoodadRec, *XkbcAnyDoodadPtr;

typedef union _XkbcDoodad {
	XkbcAnyDoodadRec	any;
	XkbcShapeDoodadRec	shape;
	XkbcTextDoodadRec	text;
	XkbcIndicatorDoodadRec	indicator;
	XkbcLogoDoodadRec	logo;
} XkbcDoodadRec, *XkbcDoodadPtr;

#define	XkbUnknownDoodad	0
#define	XkbOutlineDoodad	1
#define	XkbSolidDoodad		2
#define	XkbTextDoodad		3
#define	XkbIndicatorDoodad	4
#define	XkbLogoDoodad		5

typedef struct _XkbcKey {
	XkbKeyNameRec	 name;
	short		 gap;
	unsigned char	 shape_ndx;
	unsigned char	 color_ndx;
} XkbcKeyRec, *XkbcKeyPtr;
#define	XkbKeyShape(g,k)	(&(g)->shapes[(k)->shape_ndx])
#define	XkbKeyColor(g,k)	(&(g)->colors[(k)->color_ndx])
#define	XkbSetKeyShape(g,k,s)	((k)->shape_ndx= (s)-&(g)->shapes[0])
#define	XkbSetKeyColor(g,k,c)	((k)->color_ndx= (c)-&(g)->colors[0])

typedef struct _XkbRow {
	short	 	top;
	short	 	left;
	unsigned short	num_keys;
	unsigned short	sz_keys;
	int		vertical;
	XkbcKeyPtr	keys;
	XkbcBoundsRec	bounds;
} XkbcRowRec, *XkbcRowPtr;

typedef struct _XkbcSection {
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
	XkbcRowPtr	 rows;
	XkbcDoodadPtr	 doodads;
	XkbcBoundsRec	 bounds;
	struct _XkbOverlay *overlays;
} XkbcSectionRec, *XkbcSectionPtr;

typedef	struct _XkbcOverlayKey {
	XkbKeyNameRec	over;
	XkbKeyNameRec	under;
} XkbcOverlayKeyRec, *XkbcOverlayKeyPtr;

typedef struct _XkbOverlayRow {
	unsigned short		row_under;
	unsigned short		num_keys;
	unsigned short		sz_keys;
	XkbcOverlayKeyPtr	keys;
} XkbcOverlayRowRec, *XkbcOverlayRowPtr;

typedef struct _XkbOverlay {
	uint32_t			name;
	XkbcSectionPtr		section_under;
	unsigned short		num_rows;
	unsigned short		sz_rows;
	XkbcOverlayRowPtr	rows;
	XkbcBoundsPtr		bounds;
} XkbcOverlayRec, *XkbcOverlayPtr;

typedef struct _XkbcGeometry {
	uint32_t		 name;
	unsigned short	 width_mm;
	unsigned short	 height_mm;
	char *		 label_font;
	XkbcColorPtr	 label_color;
	XkbcColorPtr	 base_color;
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
	XkbcPropertyPtr	 properties;
	XkbcColorPtr	 colors;
	XkbcShapePtr	 shapes;
	XkbcSectionPtr	 sections;
	XkbcDoodadPtr	 doodads;
	XkbKeyAliasPtr	 key_aliases;
} XkbcGeometryRec, *XkbcGeometryPtr;
#define	XkbGeomColorIndex(g,c)	((int)((c)-&(g)->colors[0]))

#define	XkbGeomPropertiesMask	(1<<0)
#define	XkbGeomColorsMask	(1<<1)
#define	XkbGeomShapesMask	(1<<2)
#define	XkbGeomSectionsMask	(1<<3)
#define	XkbGeomDoodadsMask	(1<<4)
#define	XkbGeomKeyAliasesMask	(1<<5)
#define	XkbGeomAllMask		(0x3f)

typedef struct _XkbcGeometrySizes {
	unsigned int	which;
	unsigned short	num_properties;
	unsigned short	num_colors;
	unsigned short	num_shapes;
	unsigned short	num_sections;
	unsigned short	num_doodads;
	unsigned short	num_key_aliases;
} XkbcGeometrySizesRec, *XkbcGeometrySizesPtr;

/* Common keyboard description structure */
typedef struct _XkbcDesc {
    unsigned int        defined;
    unsigned short      flags;
    unsigned short      device_spec;
    KeyCode             min_key_code;
    KeyCode             max_key_code;

    XkbControlsPtr      ctrls;
    XkbcServerMapPtr    server;
    XkbcClientMapPtr    map;
    XkbIndicatorPtr     indicators;
    XkbcNamesPtr        names;
    XkbcCompatMapPtr    compat;
    XkbcGeometryPtr     geom;
} XkbcDescRec, *XkbcDescPtr;

_XFUNCPROTOBEGIN

typedef uint32_t (*InternAtomFuncPtr)(const char *val);
typedef const char *(*GetAtomValueFuncPtr)(uint32_t atom);

extern void
XkbcInitAtoms(InternAtomFuncPtr intern, GetAtomValueFuncPtr get_atom_value);

extern XkbcDescPtr
XkbcCompileKeymapFromRules(const XkbRMLVOSet *rmlvo);

extern XkbcDescPtr
XkbcCompileKeymapFromComponents(const XkbComponentNamesPtr ktcsg);

extern XkbcDescPtr
XkbcCompileKeymapFromFile(FILE *inputFile, const char *mapName);

extern XkbComponentListPtr
XkbcListComponents(XkbComponentNamesPtr ptrns, int *maxMatch);

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
extern void
XkbcCanonicaliseComponents(XkbComponentNamesPtr names,
                           const XkbComponentNamesPtr old);

/*
 * Converts a keysym to a string; will return unknown Unicode codepoints
 * as "Ua1b2", and other unknown keysyms as "0xabcd1234".
 *
 * The string returned may become invalidated after the next call to
 * XkbcKeysymToString: if you need to preserve it, then you must
 * duplicate it.
 *
 * This is uint32_t rather than KeySym, as KeySym changes size between
 * client and server (no, really).
 */
extern char *
XkbcKeysymToString(uint32_t ks);

/*
 * See XkbcKeysymToString comments: this function will accept any string
 * from that function.
 */
extern uint32_t
XkbcStringToKeysym(const char *s);

_XFUNCPROTOEND

#endif /* _XKBCOMMON_H_ */
