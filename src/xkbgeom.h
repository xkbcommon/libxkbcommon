/*
Copyright 2009  Dan Nicholson

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

#ifndef _XKBGEOM_H_
#define _XKBGEOM_H_

#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/extensions/XKBstrcommon.h>
#include <X11/extensions/XKBrulescommon.h>
#include "X11/extensions/XKBcommon.h"

extern void
XkbcFreeGeomProperties(XkbcGeometryPtr geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomKeyAliases(XkbcGeometryPtr geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomColors(XkbcGeometryPtr geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomPoints(XkbcOutlinePtr outline, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomOutlines(XkbcShapePtr shape, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomShapes(XkbcGeometryPtr geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomOverlayKeys(XkbcOverlayRowPtr row, int first, int count,
                        Bool freeAll);

extern void
XkbcFreeGeomOverlayRows(XkbcOverlayPtr overlay, int first, int count,
                        Bool freeAll);

extern void
XkbcFreeGeomOverlays(XkbcSectionPtr section, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomKeys(XkbcRowPtr row, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomRows(XkbcSectionPtr section, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomSections(XkbcGeometryPtr geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomDoodads(XkbcDoodadPtr doodads, int nDoodads, Bool freeAll);

extern void
XkbcFreeGeometry(XkbcGeometryPtr geom, unsigned which, Bool freeMap);

extern int
XkbcAllocGeomProps(XkbcGeometryPtr geom, int nProps);

extern int
XkbcAllocGeomColors(XkbcGeometryPtr geom, int nColors);

extern int
XkbcAllocGeomKeyAliases(XkbcGeometryPtr geom, int nKeyAliases);

extern int
XkbcAllocGeomShapes(XkbcGeometryPtr geom, int nShapes);

extern int
XkbcAllocGeomSections(XkbcGeometryPtr geom, int nSections);

extern int
XkbcAllocGeomOverlays(XkbcSectionPtr section, int nOverlays);

extern int
XkbcAllocGeomOverlayRows(XkbcOverlayPtr overlay, int nRows);

extern int
XkbcAllocGeomOverlayKeys(XkbcOverlayRowPtr row, int nKeys);

extern int
XkbcAllocGeomDoodads(XkbcGeometryPtr geom, int nDoodads);

extern int
XkbcAllocGeomSectionDoodads(XkbcSectionPtr section, int nDoodads);

extern int
XkbcAllocGeomOutlines(XkbcShapePtr shape, int nOL);

extern int
XkbcAllocGeomRows(XkbcSectionPtr section, int nRows);

extern int
XkbcAllocGeomPoints(XkbcOutlinePtr ol, int nPts);

extern int
XkbcAllocGeomKeys(XkbcRowPtr row, int nKeys);

extern int
XkbcAllocGeometry(XkbcDescPtr xkb, XkbcGeometrySizesPtr sizes);

extern XkbcPropertyPtr
XkbcAddGeomProperty(XkbcGeometryPtr geom, char *name, char *value);

extern XkbKeyAliasPtr
XkbcAddGeomKeyAlias(XkbcGeometryPtr geom, char *aliasStr, char *realStr);

extern XkbcColorPtr
XkbcAddGeomColor(XkbcGeometryPtr geom, char *spec, unsigned int pixel);

extern XkbcOutlinePtr
XkbcAddGeomOutline(XkbcShapePtr shape, int sz_points);

extern XkbcShapePtr
XkbcAddGeomShape(XkbcGeometryPtr geom, CARD32 name, int sz_outlines);

extern XkbcKeyPtr
XkbcAddGeomKey(XkbcRowPtr row);

extern XkbcRowPtr
XkbcAddGeomRow(XkbcSectionPtr section, int sz_keys);

extern XkbcSectionPtr
XkbcAddGeomSection(XkbcGeometryPtr geom, CARD32 name,
                   int sz_rows, int sz_doodads, int sz_over);

extern XkbcDoodadPtr
XkbcAddGeomDoodad(XkbcGeometryPtr geom, XkbcSectionPtr section, CARD32 name);

extern XkbcOverlayKeyPtr
XkbcAddGeomOverlayKey(XkbcOverlayPtr overlay, XkbcOverlayRowPtr row,
                      char *over, char *under);

extern XkbcOverlayRowPtr
XkbcAddGeomOverlayRow(XkbcOverlayPtr overlay, int row_under, int sz_keys);

extern XkbcOverlayPtr
XkbcAddGeomOverlay(XkbcSectionPtr section, CARD32 name, int sz_rows);

/***====================================================================***/

extern Bool
XkbcComputeShapeBounds(XkbcShapePtr shape);

extern Bool
XkbcComputeShapeTop(XkbcShapePtr shape, XkbcBoundsPtr bounds);

extern Bool
XkbcComputeRowBounds(XkbcGeometryPtr geom, XkbcSectionPtr section, XkbcRowPtr row);

extern Bool
XkbcComputeSectionBounds(XkbcGeometryPtr geom, XkbcSectionPtr section);

#endif /* _XKBGEOM_H_ */
