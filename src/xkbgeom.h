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
#include "X11/extensions/XKBcommon.h"

extern void
XkbcFreeGeomProperties(struct xkb_geometry * geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomKeyAliases(struct xkb_geometry * geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomColors(struct xkb_geometry * geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomPoints(struct xkb_outline * outline, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomOutlines(struct xkb_shape * shape, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomShapes(struct xkb_geometry * geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomOverlayKeys(struct xkb_overlay_row * row, int first, int count,
                        Bool freeAll);

extern void
XkbcFreeGeomOverlayRows(struct xkb_overlay * overlay, int first, int count,
                        Bool freeAll);

extern void
XkbcFreeGeomOverlays(struct xkb_section * section, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomKeys(struct xkb_row * row, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomRows(struct xkb_section * section, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomSections(struct xkb_geometry * geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomDoodads(union xkb_doodad * doodads, int nDoodads, Bool freeAll);

extern void
XkbcFreeGeometry(struct xkb_geometry * geom, unsigned which, Bool freeMap);

extern int
XkbcAllocGeomProps(struct xkb_geometry * geom, int nProps);

extern int
XkbcAllocGeomColors(struct xkb_geometry * geom, int nColors);

extern int
XkbcAllocGeomKeyAliases(struct xkb_geometry * geom, int nKeyAliases);

extern int
XkbcAllocGeomShapes(struct xkb_geometry * geom, int nShapes);

extern int
XkbcAllocGeomSections(struct xkb_geometry * geom, int nSections);

extern int
XkbcAllocGeomOverlays(struct xkb_section * section, int nOverlays);

extern int
XkbcAllocGeomOverlayRows(struct xkb_overlay * overlay, int nRows);

extern int
XkbcAllocGeomOverlayKeys(struct xkb_overlay_row * row, int nKeys);

extern int
XkbcAllocGeomDoodads(struct xkb_geometry * geom, int nDoodads);

extern int
XkbcAllocGeomSectionDoodads(struct xkb_section * section, int nDoodads);

extern int
XkbcAllocGeomOutlines(struct xkb_shape * shape, int nOL);

extern int
XkbcAllocGeomRows(struct xkb_section * section, int nRows);

extern int
XkbcAllocGeomPoints(struct xkb_outline * ol, int nPts);

extern int
XkbcAllocGeomKeys(struct xkb_row * row, int nKeys);

extern int
XkbcAllocGeometry(struct xkb_desc * xkb, struct xkb_geometry_sizes * sizes);

extern struct xkb_property *
XkbcAddGeomProperty(struct xkb_geometry * geom, const char *name, const char *value);

extern struct xkb_key_alias *
XkbcAddGeomKeyAlias(struct xkb_geometry * geom, const char *aliasStr, const char *realStr);

extern struct xkb_color *
XkbcAddGeomColor(struct xkb_geometry * geom, const char *spec, unsigned int pixel);

extern struct xkb_outline *
XkbcAddGeomOutline(struct xkb_shape * shape, int sz_points);

extern struct xkb_shape *
XkbcAddGeomShape(struct xkb_geometry * geom, uint32_t name, int sz_outlines);

extern struct xkb_key *
XkbcAddGeomKey(struct xkb_row * row);

extern struct xkb_row *
XkbcAddGeomRow(struct xkb_section * section, int sz_keys);

extern struct xkb_section *
XkbcAddGeomSection(struct xkb_geometry * geom, uint32_t name,
                   int sz_rows, int sz_doodads, int sz_over);

extern union xkb_doodad *
XkbcAddGeomDoodad(struct xkb_geometry * geom, struct xkb_section * section, uint32_t name);

extern struct xkb_overlay_key *
XkbcAddGeomOverlayKey(struct xkb_overlay * overlay, struct xkb_overlay_row * row,
                      const char *over, const char *under);

extern struct xkb_overlay_row *
XkbcAddGeomOverlayRow(struct xkb_overlay * overlay, int row_under, int sz_keys);

extern struct xkb_overlay *
XkbcAddGeomOverlay(struct xkb_section * section, uint32_t name, int sz_rows);

/***====================================================================***/

extern Bool
XkbcComputeShapeBounds(struct xkb_shape * shape);

extern Bool
XkbcComputeShapeTop(struct xkb_shape * shape, struct xkb_bounds * bounds);

extern Bool
XkbcComputeRowBounds(struct xkb_geometry * geom, struct xkb_section * section, struct xkb_row * row);

extern Bool
XkbcComputeSectionBounds(struct xkb_geometry * geom, struct xkb_section * section);

#endif /* _XKBGEOM_H_ */
