/*
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
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "xkbgeom.h"
#include "xkbcommon/xkbcommon.h"
#include "XKBcommonint.h"
#include <X11/extensions/XKB.h>

typedef void (*ContentsClearFunc)(void *priv);

static void
_XkbFreeGeomNonLeafElems(unsigned short *num_inout, unsigned short *sz_inout,
                         void *elems, size_t elem_sz,
                         ContentsClearFunc freeFunc)
{
    int i;
    char *start, *ptr;

    if (!elems)
        return;

    start = *(char **)elems;
    if (!start)
        return;

    ptr = start;
    for (i = 0; i < *num_inout; i++) {
        freeFunc(ptr);
        ptr += elem_sz;
    }

    *num_inout = *sz_inout = 0;
    free(start);
}

static void
_XkbClearProperty(void *prop_in)
{
    struct xkb_property * prop = prop_in;

    free(prop->name);
    prop->name = NULL;
    free(prop->value);
    prop->value = NULL;
}

static void
XkbcFreeGeomProperties(struct xkb_geometry * geom)
{
    _XkbFreeGeomNonLeafElems(&geom->num_properties, &geom->sz_properties,
                             &geom->properties, sizeof(struct xkb_property),
                             _XkbClearProperty);
}

static void
_XkbClearColor(void *color_in)
{
    struct xkb_color * color = color_in;

    free(color->spec);
}

static void
XkbcFreeGeomColors(struct xkb_geometry * geom)
{
    _XkbFreeGeomNonLeafElems(&geom->num_colors, &geom->sz_colors,
                             &geom->colors, sizeof(struct xkb_color),
                             _XkbClearColor);
}

static void
_XkbClearOutline(void *outline_in)
{
    struct xkb_outline * outline = outline_in;

    free(outline->points);
}

static void
XkbcFreeGeomOutlines(struct xkb_shape * shape)
{
    _XkbFreeGeomNonLeafElems(&shape->num_outlines, &shape->sz_outlines,
                             &shape->outlines, sizeof(struct xkb_outline),
                             _XkbClearOutline);
}

static void
_XkbClearShape(void *shape_in)
{
    struct xkb_shape * shape = shape_in;

    XkbcFreeGeomOutlines(shape);
}

static void
XkbcFreeGeomShapes(struct xkb_geometry * geom)
{
    _XkbFreeGeomNonLeafElems(&geom->num_shapes, &geom->sz_shapes,
                             &geom->shapes, sizeof(struct xkb_shape),
                             _XkbClearShape);
}

static void
_XkbClearRow(void *row_in)
{
    struct xkb_row * row = row_in;

    free(row->keys);
}

static void
XkbcFreeGeomRows(struct xkb_section * section)
{
    _XkbFreeGeomNonLeafElems(&section->num_rows, &section->sz_rows,
                             &section->rows, sizeof(struct xkb_row),
                             _XkbClearRow);
}

static void
_XkbClearDoodad(union xkb_doodad *doodad)
{
    switch (doodad->any.type) {
    case XkbTextDoodad:
        free(doodad->text.text);
        doodad->text.text = NULL;
        free(doodad->text.font);
        doodad->text.font = NULL;
        break;

    case XkbLogoDoodad:
        free(doodad->logo.logo_name);
        doodad->logo.logo_name = NULL;
        break;
    }
}

static void
XkbcFreeGeomDoodads(union xkb_doodad * doodads, int nDoodads)
{
    int i;
    union xkb_doodad * doodad;

    for (i = 0, doodad = doodads; i < nDoodads && doodad; i++, doodad++)
        _XkbClearDoodad(doodad);
    free(doodads);
}

static void
_XkbClearSection(void *section_in)
{
    struct xkb_section * section = section_in;

    XkbcFreeGeomRows(section);
    XkbcFreeGeomDoodads(section->doodads, section->num_doodads);
    section->doodads = NULL;
}

static void
XkbcFreeGeomSections(struct xkb_geometry * geom)
{
    _XkbFreeGeomNonLeafElems(&geom->num_sections, &geom->sz_sections,
                             &geom->sections, sizeof(struct xkb_section),
                             _XkbClearSection);
}

void
XkbcFreeGeometry(struct xkb_desc * xkb)
{
    struct xkb_geometry *geom;

    if (!xkb || !xkb->geom)
        return;

    geom = xkb->geom;

    XkbcFreeGeomProperties(geom);
    XkbcFreeGeomColors(geom);
    XkbcFreeGeomShapes(geom);
    XkbcFreeGeomSections(geom);
    XkbcFreeGeomDoodads(geom->doodads, geom->num_doodads);
    free(geom->key_aliases);
    free(geom->label_font);
    free(geom);
}

static int
_XkbGeomAlloc(char **old, unsigned short *num, unsigned short *total,
              int num_new, size_t sz_elem)
{
    if (num_new < 1)
        return Success;

    if (!(*old))
        *num = *total = 0;

    if ((*num) + num_new <= (*total))
        return Success;

    *total = (*num) + num_new;

    if (*old)
        *old = realloc(*old, (*total) * sz_elem);
    else
        *old = calloc(*total, sz_elem);
    if (!(*old)) {
        *total = *num = 0;
        return BadAlloc;
    }

    if (*num > 0) {
        char *tmp = *old;
        memset(&tmp[sz_elem * (*num)], 0, num_new * sz_elem);
    }

    return Success;
}

#define _XkbAllocProps(g, n)    _XkbGeomAlloc((char **)&(g)->properties, \
                                              &(g)->num_properties, \
                                              &(g)->sz_properties, \
                                              (n), sizeof(struct xkb_property))
#define _XkbAllocColors(g, n)   _XkbGeomAlloc((char **)&(g)->colors, \
                                              &(g)->num_colors, \
                                              &(g)->sz_colors, \
                                              (n), sizeof(struct xkb_color))
#define _XkbAllocShapes(g, n)   _XkbGeomAlloc((char **)&(g)->shapes, \
                                              &(g)->num_shapes, \
                                              &(g)->sz_shapes, \
                                              (n), sizeof(struct xkb_shape))
#define _XkbAllocSections(g, n) _XkbGeomAlloc((char **)&(g)->sections, \
                                              &(g)->num_sections, \
                                              &(g)->sz_sections, \
                                              (n), sizeof(struct xkb_section))
#define _XkbAllocDoodads(g, n)  _XkbGeomAlloc((char **)&(g)->doodads, \
                                              &(g)->num_doodads, \
                                              &(g)->sz_doodads, \
                                              (n), sizeof(union xkb_doodad))
#define _XkbAllocKeyAliases(g, n)   _XkbGeomAlloc((char **)&(g)->key_aliases, \
                                                  &(g)->num_key_aliases, \
                                                  &(g)->sz_key_aliases, \
                                                  (n), sizeof(struct xkb_key_alias))

#define _XkbAllocOutlines(s, n) _XkbGeomAlloc((char **)&(s)->outlines, \
                                              &(s)->num_outlines, \
                                              &(s)->sz_outlines, \
                                              (n), sizeof(struct xkb_outline))
#define _XkbAllocRows(s, n)     _XkbGeomAlloc((char **)&(s)->rows, \
                                              &(s)->num_rows, \
                                              &(s)->sz_rows, \
                                              (n), sizeof(struct xkb_row))
#define _XkbAllocPoints(o, n)   _XkbGeomAlloc((char **)&(o)->points, \
                                              &(o)->num_points, \
                                              &(o)->sz_points, \
                                              (n), sizeof(struct xkb_point))
#define _XkbAllocKeys(r, n)     _XkbGeomAlloc((char **)&(r)->keys, \
                                              &(r)->num_keys, \
                                              &(r)->sz_keys, \
                                              (n), sizeof(struct xkb_key))
#define _XkbAllocOverlays(s, n) _XkbGeomAlloc((char **)&(s)->overlays, \
                                              &(s)->num_overlays, \
                                              &(s)->sz_overlays, \
                                              (n), sizeof(struct xkb_overlay))
#define _XkbAllocOverlayRows(o, n)  _XkbGeomAlloc((char **)&(o)->rows, \
                                                  &(o)->num_rows, \
                                                  &(o)->sz_rows, \
                                                  (n), sizeof(struct xkb_overlay_row))
#define _XkbAllocOverlayKeys(r, n)  _XkbGeomAlloc((char **)&(r)->keys, \
                                                  &(r)->num_keys, \
                                                  &(r)->sz_keys, \
                                                  (n), sizeof(struct xkb_overlay_key))

int
XkbcAllocGeomKeyAliases(struct xkb_geometry * geom, int nKeyAliases)
{
    return _XkbAllocKeyAliases(geom, nKeyAliases);
}

int
XkbcAllocGeometry(struct xkb_desc * xkb, struct xkb_geometry_sizes * sizes)
{
    struct xkb_geometry * geom;
    int rtrn;

    if (!xkb->geom) {
        xkb->geom = _XkbTypedCalloc(1, struct xkb_geometry);
        if (!xkb->geom)
            return BadAlloc;
    }
    geom = xkb->geom;

    if ((sizes->which & XkbGeomPropertiesMask) &&
        ((rtrn = _XkbAllocProps(geom, sizes->num_properties)) != Success))
        goto bail;

    if ((sizes->which & XkbGeomColorsMask) &&
        ((rtrn = _XkbAllocColors(geom, sizes->num_colors)) != Success))
        goto bail;

    if ((sizes->which & XkbGeomShapesMask) &&
        ((rtrn = _XkbAllocShapes(geom, sizes->num_shapes)) != Success))
        goto bail;

    if ((sizes->which & XkbGeomSectionsMask) &&
        ((rtrn = _XkbAllocSections(geom, sizes->num_sections)) != Success))
        goto bail;

    if ((sizes->which & XkbGeomDoodadsMask) &&
        ((rtrn = _XkbAllocDoodads(geom, sizes->num_doodads)) != Success))
        goto bail;

    if ((sizes->which & XkbGeomKeyAliasesMask) &&
        ((rtrn = _XkbAllocKeyAliases(geom, sizes->num_key_aliases)) != Success))
        goto bail;

    return Success;
bail:
    XkbcFreeGeometry(xkb);
    xkb->geom = NULL;
    return rtrn;
}

struct xkb_property *
XkbcAddGeomProperty(struct xkb_geometry * geom,const char *name,const char *value)
{
    int i;
    struct xkb_property * prop;

    if ((!geom)||(!name)||(!value))
	return NULL;
    for (i=0,prop=geom->properties;i<geom->num_properties;i++,prop++) {
	if ((prop->name)&&(strcmp(name,prop->name)==0)) {
	    free(prop->value);
            prop->value = strdup(value);
	    return prop;
	}
    }
    if ((geom->num_properties>=geom->sz_properties)&&
					(_XkbAllocProps(geom,1)!=Success)) {
	return NULL;
    }
    prop= &geom->properties[geom->num_properties];
    prop->name = strdup(name);
    if (!prop->name)
	return NULL;
    prop->value = strdup(value);
    if (!prop->value) {
	free(prop->name);
	prop->name= NULL;
	return NULL;
    }
    geom->num_properties++;
    return prop;
}

struct xkb_color *
XkbcAddGeomColor(struct xkb_geometry * geom,const char *spec,unsigned int pixel)
{
    int i;
    struct xkb_color * color;

    if ((!geom)||(!spec))
	return NULL;
    for (i=0,color=geom->colors;i<geom->num_colors;i++,color++) {
	if ((color->spec)&&(strcmp(color->spec,spec)==0)) {
	    color->pixel= pixel;
	    return color;
	}
    }
    if ((geom->num_colors>=geom->sz_colors)&&
					(_XkbAllocColors(geom,1)!=Success)) {
	return NULL;
    }
    color= &geom->colors[geom->num_colors];
    color->pixel= pixel;
    color->spec = strdup(spec);
    if (!color->spec)
	return NULL;
    geom->num_colors++;
    return color;
}

struct xkb_outline *
XkbcAddGeomOutline(struct xkb_shape * shape,int sz_points)
{
struct xkb_outline *	outline;

    if ((!shape)||(sz_points<0))
	return NULL;
    if ((shape->num_outlines>=shape->sz_outlines)&&
					(_XkbAllocOutlines(shape,1)!=Success)) {
	return NULL;
    }
    outline= &shape->outlines[shape->num_outlines];
    memset(outline, 0, sizeof(struct xkb_outline));
    if ((sz_points>0)&&(_XkbAllocPoints(outline,sz_points)!=Success))
	return NULL;
    shape->num_outlines++;
    return outline;
}

struct xkb_shape *
XkbcAddGeomShape(struct xkb_geometry * geom,uint32_t name,int sz_outlines)
{
    struct xkb_shape *shape;
    int i;

    if ((!geom)||(!name)||(sz_outlines<0))
	return NULL;
    if (geom->num_shapes>0) {
	for (shape=geom->shapes,i=0;i<geom->num_shapes;i++,shape++) {
	    if (name==shape->name)
		return shape;
	}
    }
    if ((geom->num_shapes>=geom->sz_shapes)&&
					(_XkbAllocShapes(geom,1)!=Success))
	return NULL;
    shape= &geom->shapes[geom->num_shapes];
    memset(shape, 0, sizeof(struct xkb_shape));
    if ((sz_outlines>0)&&(_XkbAllocOutlines(shape,sz_outlines)!=Success))
	return NULL;
    shape->name= name;
    shape->primary= shape->approx= NULL;
    geom->num_shapes++;
    return shape;
}

struct xkb_key *
XkbcAddGeomKey(struct xkb_row * row)
{
struct xkb_key *	key;
    if (!row)
	return NULL;
    if ((row->num_keys>=row->sz_keys)&&(_XkbAllocKeys(row,1)!=Success))
	return NULL;
    key= &row->keys[row->num_keys++];
    memset(key, 0, sizeof(struct xkb_key));
    return key;
}

struct xkb_row *
XkbcAddGeomRow(struct xkb_section * section,int sz_keys)
{
struct xkb_row *	row;

    if ((!section)||(sz_keys<0))
	return NULL;
    if ((section->num_rows>=section->sz_rows)&&
    					(_XkbAllocRows(section,1)!=Success))
	return NULL;
    row= &section->rows[section->num_rows];
    memset(row, 0, sizeof(struct xkb_row));
    if ((sz_keys>0)&&(_XkbAllocKeys(row,sz_keys)!=Success))
	return NULL;
    section->num_rows++;
    return row;
}

struct xkb_section *
XkbcAddGeomSection(	struct xkb_geometry *	geom,
			uint32_t		name,
			int		sz_rows,
			int		sz_doodads,
			int		sz_over)
{
    int	i;
    struct xkb_section *	section;

    if ((!geom)||(name==None)||(sz_rows<0))
	return NULL;
    for (i=0,section=geom->sections;i<geom->num_sections;i++,section++) {
	if (section->name!=name)
	    continue;
	if (((sz_rows>0)&&(_XkbAllocRows(section,sz_rows)!=Success))||
	    ((sz_doodads>0)&&(_XkbAllocDoodads(section,sz_doodads)!=Success))||
	    ((sz_over>0)&&(_XkbAllocOverlays(section,sz_over)!=Success)))
	    return NULL;
	return section;
    }
    if ((geom->num_sections>=geom->sz_sections)&&
					(_XkbAllocSections(geom,1)!=Success))
	return NULL;
    section= &geom->sections[geom->num_sections];
    if ((sz_rows>0)&&(_XkbAllocRows(section,sz_rows)!=Success))
	return NULL;
    if ((sz_doodads>0)&&(_XkbAllocDoodads(section,sz_doodads)!=Success)) {
        free(section->rows);
        section->rows= NULL;
        section->sz_rows= section->num_rows= 0;
	return NULL;
    }
    section->name= name;
    geom->num_sections++;
    return section;
}

union xkb_doodad *
XkbcAddGeomDoodad(struct xkb_geometry * geom,struct xkb_section * section,uint32_t name)
{
    union xkb_doodad *old, *doodad;
    int i, nDoodads;

    if ((!geom)||(name==None))
	return NULL;
    if ((section!=NULL)&&(section->num_doodads>0)) {
	old= section->doodads;
	nDoodads= section->num_doodads;
    }
    else {
	old= geom->doodads;
	nDoodads= geom->num_doodads;
    }
    for (i=0,doodad=old;i<nDoodads;i++,doodad++) {
	if (doodad->any.name==name)
	    return doodad;
    }
    if (section) {
	if ((section->num_doodads>=geom->sz_doodads)&&
	    (_XkbAllocDoodads(section,1)!=Success)) {
	    return NULL;
	}
	doodad= &section->doodads[section->num_doodads++];
    }
    else {
	if ((geom->num_doodads>=geom->sz_doodads)&&
					(_XkbAllocDoodads(geom,1)!=Success))
	    return NULL;
	doodad= &geom->doodads[geom->num_doodads++];
    }
    memset(doodad, 0, sizeof(union xkb_doodad));
    doodad->any.name= name;
    return doodad;
}

struct xkb_overlay_row *
XkbcAddGeomOverlayRow(struct xkb_overlay * overlay,int row_under,int sz_keys)
{
    int i;
    struct xkb_overlay_row *row;

    if ((!overlay)||(sz_keys<0))
	return NULL;
    if (row_under>=overlay->section_under->num_rows)
	return NULL;
    for (i=0;i<overlay->num_rows;i++) {
	if (overlay->rows[i].row_under==row_under) {
	    row= &overlay->rows[i];
	    if ((row->sz_keys<sz_keys)&&
				(_XkbAllocOverlayKeys(row,sz_keys)!=Success)) {
		return NULL;
	    }
	    return &overlay->rows[i];
	}
    }
    if ((overlay->num_rows>=overlay->sz_rows)&&
				(_XkbAllocOverlayRows(overlay,1)!=Success))
	return NULL;
    row= &overlay->rows[overlay->num_rows];
    memset(row, 0, sizeof(struct xkb_overlay_row));
    if ((sz_keys>0)&&(_XkbAllocOverlayKeys(row,sz_keys)!=Success))
	return NULL;
    row->row_under= row_under;
    overlay->num_rows++;
    return row;
}

struct xkb_overlay *
XkbcAddGeomOverlay(struct xkb_section * section,uint32_t name,int sz_rows)
{
    int i;
    struct xkb_overlay *overlay;

    if ((!section)||(name==None)||(sz_rows==0))
	return NULL;

    for (i=0,overlay=section->overlays;i<section->num_overlays;i++,overlay++) {
	if (overlay->name==name) {
	    if ((sz_rows>0)&&(_XkbAllocOverlayRows(overlay,sz_rows)!=Success))
		return NULL;
	    return overlay;
	}
    }
    if ((section->num_overlays>=section->sz_overlays)&&
				(_XkbAllocOverlays(section,1)!=Success))
	return NULL;
    overlay= &section->overlays[section->num_overlays];
    if ((sz_rows>0)&&(_XkbAllocOverlayRows(overlay,sz_rows)!=Success))
	return NULL;
    overlay->name= name;
    overlay->section_under= section;
    section->num_overlays++;
    return overlay;
}
