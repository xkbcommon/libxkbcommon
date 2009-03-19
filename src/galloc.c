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
#include "X11/XkbCommon.h"
#include "XkbCommonInt.h"
#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/extensions/XKB.h>

static void
_XkbFreeGeomLeafElems(	Bool			freeAll,
			int			first,
			int 			count,
			unsigned short *	num_inout,
			unsigned short *	sz_inout,
			char **			elems,
			unsigned int		elem_sz)
{
    if ((freeAll)||(*elems==NULL)) {
	*num_inout= *sz_inout= 0;
	if (*elems!=NULL) {
	    _XkbFree(*elems);
	    *elems= NULL;
	}
	return;
    }

    if ((first>=(*num_inout))||(first<0)||(count<1))
	return;

    if (first+count>=(*num_inout)) {
	/* truncating the array is easy */
	(*num_inout)= first;
    }
    else {
	char *	ptr;
	int 	extra;
	ptr= *elems;
	extra= ((*num_inout)-(first+count))*elem_sz;
	if (extra>0)
	    memmove(&ptr[first*elem_sz],&ptr[(first+count)*elem_sz],extra);
	(*num_inout)-= count;
    }
    return;
}

typedef void (*ContentsClearFunc)(
		char *		/* priv */
);

static void
_XkbFreeGeomNonLeafElems(	Bool			freeAll,
				int			first,
				int 			count,
				unsigned short *	num_inout,
				unsigned short *	sz_inout,
				char **			elems,
				unsigned int		elem_sz,
				ContentsClearFunc	freeFunc)
{
register int i;
register char *ptr;

    if (freeAll) {
	first= 0;
	count= (*num_inout);
    }
    else if ((first>=(*num_inout))||(first<0)||(count<1))
	return;
    else if (first+count>(*num_inout))
	count= (*num_inout)-first;
    if (*elems==NULL)
	return;

    if (freeFunc) {
	ptr= *elems;
	ptr+= first*elem_sz;
	for (i=0;i<count;i++) {
	    (*freeFunc)(ptr);
	    ptr+= elem_sz;
	}
    }
    if (freeAll) {
	(*num_inout)= (*sz_inout)= 0;
	if (*elems) {
	    _XkbFree(*elems);
	    *elems= NULL;
	}
    }
    else if (first+count>=(*num_inout))
	*num_inout= first;
    else {
	i= ((*num_inout)-(first+count))*elem_sz;
	ptr= *elems;
	memmove(&ptr[first*elem_sz],&ptr[(first+count)*elem_sz],i);
	(*num_inout)-= count;
    }
    return;
}

static void
_XkbClearProperty(char *prop_in)
{
XkbPropertyPtr	prop= (XkbPropertyPtr)prop_in;

    if (prop->name) {
	_XkbFree(prop->name);
	prop->name= NULL;
    }
    if (prop->value) {
	_XkbFree(prop->value);
	prop->value= NULL;
    }
    return;
}

void
XkbcFreeGeomProperties(	XkbGeometryPtr	geom,
			int		first,
			int		count,
			Bool		freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll,first,count,
				&geom->num_properties,&geom->sz_properties,
				(char **)&geom->properties,
				sizeof(XkbPropertyRec),_XkbClearProperty);
    return;
}

void
XkbcFreeGeomKeyAliases(	XkbGeometryPtr	geom,
			int		first,
			int		count,
			Bool		freeAll)
{
    _XkbFreeGeomLeafElems(freeAll,first,count,
				&geom->num_key_aliases,&geom->sz_key_aliases,
				(char **)&geom->key_aliases,
				sizeof(XkbKeyAliasRec));
    return;
}

static void
_XkbClearColor(char *color_in)
{
XkbColorPtr	color= (XkbColorPtr)color_in;

    if (color->spec)
	_XkbFree(color->spec);
    return;
}

void
XkbcFreeGeomColors(XkbGeometryPtr geom,int first,int count,Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll,first,count,
				&geom->num_colors,&geom->sz_colors,
				(char **)&geom->colors,
				sizeof(XkbColorRec),_XkbClearColor);
    return;
}

void
XkbcFreeGeomPoints(XkbOutlinePtr outline,int first,int count,Bool freeAll)
{
    _XkbFreeGeomLeafElems(freeAll,first,count,
				&outline->num_points,&outline->sz_points,
				(char **)&outline->points,
				sizeof(XkbPointRec));
    return;
}

static void
_XkbClearOutline(char *outline_in)
{
XkbOutlinePtr	outline= (XkbOutlinePtr)outline_in;

    if (outline->points!=NULL)
	XkbcFreeGeomPoints(outline,0,outline->num_points,True);
    return;
}

void
XkbcFreeGeomOutlines(XkbShapePtr	shape,int first,int count,Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll,first,count,
				&shape->num_outlines,&shape->sz_outlines,
				(char **)&shape->outlines,
				sizeof(XkbOutlineRec),_XkbClearOutline);

    return;
}

static void
_XkbClearShape(char *shape_in)
{
XkbShapePtr	shape= (XkbShapePtr)shape_in;

    if (shape->outlines)
	XkbcFreeGeomOutlines(shape,0,shape->num_outlines,True);
    return;
}

void
XkbcFreeGeomShapes(XkbGeometryPtr geom,int first,int count,Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll,first,count,
				&geom->num_shapes,&geom->sz_shapes,
				(char **)&geom->shapes,
				sizeof(XkbShapeRec),_XkbClearShape);
    return;
}

void
XkbcFreeGeomOverlayKeys(XkbOverlayRowPtr row,int first,int count,Bool freeAll)
{
    _XkbFreeGeomLeafElems(freeAll,first,count,
				&row->num_keys,&row->sz_keys,
				(char **)&row->keys,
				sizeof(XkbOverlayKeyRec));
    return;
}


static void
_XkbClearOverlayRow(char *row_in)
{
XkbOverlayRowPtr	row= (XkbOverlayRowPtr)row_in;

    if (row->keys!=NULL)
	XkbcFreeGeomOverlayKeys(row,0,row->num_keys,True);
    return;
}

void
XkbcFreeGeomOverlayRows(XkbOverlayPtr overlay,int first,int count,Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll,first,count,
				&overlay->num_rows,&overlay->sz_rows,
				(char **)&overlay->rows,
				sizeof(XkbOverlayRowRec),_XkbClearOverlayRow);
    return;
}


static void
_XkbClearOverlay(char *overlay_in)
{
XkbOverlayPtr	overlay= (XkbOverlayPtr)overlay_in;

    if (overlay->rows!=NULL)
	XkbcFreeGeomOverlayRows(overlay,0,overlay->num_rows,True);
    return;
}

void
XkbcFreeGeomOverlays(XkbSectionPtr section,int first,int	count,Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll,first,count,
				&section->num_overlays,&section->sz_overlays,
				(char **)&section->overlays,
				sizeof(XkbOverlayRec),_XkbClearOverlay);
    return;
}


void
XkbcFreeGeomKeys(XkbRowPtr row,int first,int count,Bool freeAll)
{
    _XkbFreeGeomLeafElems(freeAll,first,count,
				&row->num_keys,&row->sz_keys,
				(char **)&row->keys,
				sizeof(XkbKeyRec));
    return;
}


static void
_XkbClearRow(char *row_in)
{
XkbRowPtr	row= (XkbRowPtr)row_in;

    if (row->keys!=NULL)
	XkbcFreeGeomKeys(row,0,row->num_keys,True);
    return;
}

void
XkbcFreeGeomRows(XkbSectionPtr section,int first,int count,Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll,first,count,
				&section->num_rows,&section->sz_rows,
				(char **)&section->rows,
				sizeof(XkbRowRec),_XkbClearRow);
}


static void
_XkbClearSection(char *section_in)
{
XkbSectionPtr	section= (XkbSectionPtr)section_in;

    if (section->rows!=NULL)
	XkbcFreeGeomRows(section,0,section->num_rows,True);
    if (section->doodads!=NULL) {
	XkbcFreeGeomDoodads(section->doodads,section->num_doodads,True);
	section->doodads= NULL;
    }
    return;
}

void
XkbcFreeGeomSections(XkbGeometryPtr geom,int first,int count,Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll,first,count,
				&geom->num_sections,&geom->sz_sections,
				(char **)&geom->sections,
				sizeof(XkbSectionRec),_XkbClearSection);
    return;
}


static void
_XkbClearDoodad(char *doodad_in)
{
XkbDoodadPtr	doodad= (XkbDoodadPtr)doodad_in;

    switch (doodad->any.type) {
   	case XkbTextDoodad:
	    {
		if (doodad->text.text!=NULL) {
		    _XkbFree(doodad->text.text);
		    doodad->text.text= NULL;
		}
		if (doodad->text.font!=NULL) {
		    _XkbFree(doodad->text.font);
		    doodad->text.font= NULL;
		}
	    }
	    break;
   	case XkbLogoDoodad:
	    {
		if (doodad->logo.logo_name!=NULL) {
		    _XkbFree(doodad->logo.logo_name);
		    doodad->logo.logo_name= NULL;
		}
	    }
	    break;
    }
    return;
}

void
XkbcFreeGeomDoodads(XkbDoodadPtr doodads,int nDoodads,Bool freeAll)
{
register int 		i;
register XkbDoodadPtr	doodad;

    if (doodads) {
	for (i=0,doodad= doodads;i<nDoodads;i++,doodad++) {
	    _XkbClearDoodad((char *)doodad);
	}
	if (freeAll)
	    _XkbFree(doodads);
    }
    return;
}

void
XkbcFreeGeometry(XkbGeometryPtr geom,unsigned which,Bool freeMap)
{
    if (geom==NULL)
	return;
    if (freeMap)
	which= XkbGeomAllMask;
    if ((which&XkbGeomPropertiesMask)&&(geom->properties!=NULL))
	XkbcFreeGeomProperties(geom,0,geom->num_properties,True);
    if ((which&XkbGeomColorsMask)&&(geom->colors!=NULL))
	XkbcFreeGeomColors(geom,0,geom->num_colors,True);
    if ((which&XkbGeomShapesMask)&&(geom->shapes!=NULL))
	XkbcFreeGeomShapes(geom,0,geom->num_shapes,True);
    if ((which&XkbGeomSectionsMask)&&(geom->sections!=NULL))
	XkbcFreeGeomSections(geom,0,geom->num_sections,True);
    if ((which&XkbGeomDoodadsMask)&&(geom->doodads!= NULL)) {
	XkbcFreeGeomDoodads(geom->doodads,geom->num_doodads,True);
	geom->doodads= NULL;
	geom->num_doodads= geom->sz_doodads= 0;
    }
    if ((which&XkbGeomKeyAliasesMask)&&(geom->key_aliases!=NULL))
	XkbcFreeGeomKeyAliases(geom,0,geom->num_key_aliases,True);
    if (freeMap) {
	if (geom->label_font!=NULL) {
	    _XkbFree(geom->label_font);
	    geom->label_font= NULL;
	}
	_XkbFree(geom);
    }
    return;
}

static int
_XkbGeomAlloc(	char **		old,
		unsigned short *	num,
		unsigned short *	total,
		int			num_new,
		size_t			sz_elem)
{
    if (num_new<1)
	return Success;
    if ((*old)==NULL)
	*num= *total= 0;

    if ((*num)+num_new<=(*total))
	return Success;

    *total= (*num)+num_new;
    if ((*old)!=NULL)
	 (*old)= (char *)_XkbRealloc((*old),(*total)*sz_elem);
    else (*old)= (char *)_XkbCalloc((*total),sz_elem);
    if ((*old)==NULL) {
	*total= *num= 0;
	return BadAlloc;
    }

    if (*num>0) {
	char *tmp= (char *)(*old);
	bzero(&tmp[sz_elem*(*num)],(num_new*sz_elem));
    }
    return Success;
}

#define	_XkbAllocProps(g,n) _XkbGeomAlloc((char **)&(g)->properties,\
				&(g)->num_properties,&(g)->sz_properties,\
				(n),sizeof(XkbPropertyRec))
#define	_XkbAllocColors(g,n) _XkbGeomAlloc((char **)&(g)->colors,\
				&(g)->num_colors,&(g)->sz_colors,\
				(n),sizeof(XkbColorRec))
#define	_XkbAllocShapes(g,n) _XkbGeomAlloc((char **)&(g)->shapes,\
				&(g)->num_shapes,&(g)->sz_shapes,\
				(n),sizeof(XkbShapeRec))
#define	_XkbAllocSections(g,n) _XkbGeomAlloc((char **)&(g)->sections,\
				&(g)->num_sections,&(g)->sz_sections,\
				(n),sizeof(XkbSectionRec))
#define	_XkbAllocDoodads(g,n) _XkbGeomAlloc((char **)&(g)->doodads,\
				&(g)->num_doodads,&(g)->sz_doodads,\
				(n),sizeof(XkbDoodadRec))
#define	_XkbAllocKeyAliases(g,n) _XkbGeomAlloc((char **)&(g)->key_aliases,\
				&(g)->num_key_aliases,&(g)->sz_key_aliases,\
				(n),sizeof(XkbKeyAliasRec))

#define	_XkbAllocOutlines(s,n) _XkbGeomAlloc((char **)&(s)->outlines,\
				&(s)->num_outlines,&(s)->sz_outlines,\
				(n),sizeof(XkbOutlineRec))
#define	_XkbAllocRows(s,n) _XkbGeomAlloc((char **)&(s)->rows,\
				&(s)->num_rows,&(s)->sz_rows,\
				(n),sizeof(XkbRowRec))
#define	_XkbAllocPoints(o,n) _XkbGeomAlloc((char **)&(o)->points,\
				&(o)->num_points,&(o)->sz_points,\
				(n),sizeof(XkbPointRec))
#define	_XkbAllocKeys(r,n) _XkbGeomAlloc((char **)&(r)->keys,\
				&(r)->num_keys,&(r)->sz_keys,\
				(n),sizeof(XkbKeyRec))
#define	_XkbAllocOverlays(s,n) _XkbGeomAlloc((char **)&(s)->overlays,\
				&(s)->num_overlays,&(s)->sz_overlays,\
				(n),sizeof(XkbOverlayRec))
#define	_XkbAllocOverlayRows(o,n) _XkbGeomAlloc((char **)&(o)->rows,\
				&(o)->num_rows,&(o)->sz_rows,\
				(n),sizeof(XkbOverlayRowRec))
#define	_XkbAllocOverlayKeys(r,n) _XkbGeomAlloc((char **)&(r)->keys,\
				&(r)->num_keys,&(r)->sz_keys,\
				(n),sizeof(XkbOverlayKeyRec))

int
XkbcAllocGeometry(XkbcDescPtr xkb,XkbGeometrySizesPtr sizes)
{
XkbGeometryPtr	geom;
int		rtrn;

    if (xkb->geom==NULL) {
	xkb->geom= _XkbTypedCalloc(1,XkbGeometryRec);
	if (!xkb->geom)
	    return BadAlloc;
    }
    geom= xkb->geom;
    if ((sizes->which&XkbGeomPropertiesMask)&&
	((rtrn=_XkbAllocProps(geom,sizes->num_properties))!=Success)) {
	goto BAIL;
    }
    if ((sizes->which&XkbGeomColorsMask)&&
	((rtrn=_XkbAllocColors(geom,sizes->num_colors))!=Success)) {
	goto BAIL;
    }
    if ((sizes->which&XkbGeomShapesMask)&&
	((rtrn=_XkbAllocShapes(geom,sizes->num_shapes))!=Success)) {
	goto BAIL;
    }
    if ((sizes->which&XkbGeomSectionsMask)&&
	((rtrn=_XkbAllocSections(geom,sizes->num_sections))!=Success)) {
	goto BAIL;
    }
    if ((sizes->which&XkbGeomDoodadsMask)&&
	((rtrn=_XkbAllocDoodads(geom,sizes->num_doodads))!=Success)) {
	goto BAIL;
    }
    if ((sizes->which&XkbGeomKeyAliasesMask)&&
	((rtrn=_XkbAllocKeyAliases(geom,sizes->num_key_aliases))!=Success)) {
	goto BAIL;
    }
    return Success;
BAIL:
    XkbcFreeGeometry(geom,XkbGeomAllMask,True);
    xkb->geom= NULL;
    return rtrn;
}
