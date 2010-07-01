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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xkbgeom.h"
#include "X11/extensions/XKBcommon.h"
#include "XKBcommonint.h"

#ifndef MINSHORT
#define MINSHORT -32768
#endif
#ifndef MAXSHORT
#define MAXSHORT 32767
#endif

static void
_XkbCheckBounds(struct xkb_bounds * bounds, int x, int y)
{
    if (x < bounds->x1)
        bounds->x1 = x;
    if (x > bounds->x2)
        bounds->x2 = x;
    if (y < bounds->y1)
        bounds->y1 = y;
    if (y > bounds->y2)
        bounds->y2 = y;
}

Bool
XkbcComputeShapeBounds(struct xkb_shape * shape)
{
    int o, p;
    struct xkb_outline * outline;
    struct xkb_point * pt;

    if ((!shape) || (shape->num_outlines < 1))
        return False;

    shape->bounds.x1 = shape->bounds.y1 = MAXSHORT;
    shape->bounds.x2 = shape->bounds.y2 = MINSHORT;

    for (outline = shape->outlines, o = 0; o < shape->num_outlines;
         o++, outline++)
    {
        for (pt = outline->points, p = 0; p < outline->num_points; p++, pt++)
            _XkbCheckBounds(&shape->bounds, pt->x, pt->y);
        if (outline->num_points < 2)
            _XkbCheckBounds(&shape->bounds, 0, 0);
    }
    return True;
}

Bool
XkbcComputeShapeTop(struct xkb_shape * shape, struct xkb_bounds * bounds)
{
    int p;
    struct xkb_outline * outline;
    struct xkb_point * pt;

    if ((!shape) || (shape->num_outlines < 1))
        return False;

    if (shape->approx)
        outline = shape->approx;
    else
        outline = &shape->outlines[shape->num_outlines - 1];

    if (outline->num_points < 2) {
         bounds->x1 = bounds->y1 = 0;
         bounds->x2 = bounds->y2 = 0;
    }
    else {
        bounds->x1 = bounds->y1 = MAXSHORT;
        bounds->x2 = bounds->y2 = MINSHORT;
    }

    for (pt = outline->points, p = 0; p < outline->num_points; p++, pt++)
        _XkbCheckBounds(bounds, pt->x, pt->y);

    return True;
}

Bool
XkbcComputeRowBounds(struct xkb_geometry * geom, struct xkb_section * section, struct xkb_row * row)
{
    int k, pos;
    struct xkb_key * key;
    struct xkb_bounds *bounds, *sbounds;

    if (!geom || !section || !row)
        return False;

    pos = 0;
    bounds = &row->bounds;
    bzero(bounds, sizeof(struct xkb_bounds));

    for (key = row->keys, pos = k = 0; k < row->num_keys; k++, key++) {
        sbounds = &XkbKeyShape(geom, key)->bounds;
        _XkbCheckBounds(bounds, pos, 0);

        if (!row->vertical) {
            if (key->gap != 0) {
                pos += key->gap;
                _XkbCheckBounds(bounds, pos, 0);
            }
            _XkbCheckBounds(bounds, pos + sbounds->x1, sbounds->y1);
            _XkbCheckBounds(bounds, pos + sbounds->x2, sbounds->y2);
            pos += sbounds->x2;
        }
        else {
            if (key->gap != 0) {
                pos += key->gap;
                _XkbCheckBounds(bounds, 0, pos);
            }
            _XkbCheckBounds(bounds,pos + sbounds->x1, sbounds->y1);
            _XkbCheckBounds(bounds,pos + sbounds->x2, sbounds->y2);
            pos += sbounds->y2;
        }
    }

    return True;
}

Bool
XkbcComputeSectionBounds(struct xkb_geometry * geom, struct xkb_section * section)
{
    int i;
    struct xkb_shape * shape;
    struct xkb_row * row;
    union xkb_doodad * doodad;
    struct xkb_bounds * bounds, *rbounds = NULL;

    if (!geom || !section)
        return False;

    bounds = &section->bounds;
    bzero(bounds, sizeof(struct xkb_bounds));

    for (i = 0, row = section->rows; i < section->num_rows; i++, row++) {
        if (!XkbcComputeRowBounds(geom, section, row))
            return False;
        rbounds = &row->bounds;
        _XkbCheckBounds(bounds, row->left + rbounds->x1,
                        row->top + rbounds->y1);
        _XkbCheckBounds(bounds, row->left + rbounds->x2,
                        row->top + rbounds->y2);
    }

    for (i = 0, doodad = section->doodads; i < section->num_doodads;
         i++, doodad++)
    {
        static struct xkb_bounds tbounds;

        switch (doodad->any.type) {
        case XkbOutlineDoodad:
        case XkbSolidDoodad:
            shape = XkbShapeDoodadShape(geom, &doodad->shape);
            rbounds = &shape->bounds;
            break;
        case XkbTextDoodad:
            tbounds.x1 = doodad->text.left;
            tbounds.y1 = doodad->text.top;
            tbounds.x2 = tbounds.x1 + doodad->text.width;
            tbounds.y2 = tbounds.y1 + doodad->text.height;
            rbounds = &tbounds;
            break;
        case XkbIndicatorDoodad:
            shape = XkbIndicatorDoodadShape(geom, &doodad->indicator);
            rbounds = &shape->bounds;
            break;
        case XkbLogoDoodad:
            shape = XkbLogoDoodadShape(geom, &doodad->logo);
            rbounds = &shape->bounds;
            break;
        default:
            tbounds.x1 = tbounds.x2 = doodad->any.left;
            tbounds.y1 = tbounds.y2 = doodad->any.top;
            break;
        }

        _XkbCheckBounds(bounds, rbounds->x1, rbounds->y1);
        _XkbCheckBounds(bounds, rbounds->x2, rbounds->y2);
    }

    return True;
}
