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

#ifndef _XKBCOMMON_H_
#define _XKBCOMMON_H_

#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/keysymdef.h>
#include <X11/extensions/XKBstrcommon.h>
#include <X11/extensions/XKBgeomcommon.h>

/* Common keyboard description structure */
typedef struct _XkbcDesc {
    unsigned int        defined;
    unsigned short      flags;
    unsigned short      device_spec;
    KeyCode             min_key_code;
    KeyCode             max_key_code;

    XkbControlsPtr      ctrls;
    XkbServerMapPtr     server;
    XkbClientMapPtr     map;
    XkbIndicatorPtr     indicators;
    XkbNamesPtr         names;
    XkbCompatMapPtr     compat;
    XkbGeometryPtr      geom;
} XkbcDescRec, *XkbcDescPtr;

extern char *
XkbcKeysymToString(KeySym ks);

extern KeySym
XkbcStringToKeysym(const char *s);

extern int
XkbcAllocCompatMap(XkbcDescPtr xkb, unsigned which, unsigned nSI);

extern void
XkbcFreeCompatMap(XkbcDescPtr xkb, unsigned which, Bool freeMap);

extern int
XkbcAllocNames(XkbcDescPtr xkb, unsigned which, int nTotalRG, int nTotalAliases);

extern void
XkbcFreeNames(XkbcDescPtr xkb, unsigned which, Bool freeMap);

extern int
XkbcAllocControls(XkbcDescPtr xkb, unsigned which);

extern void
XkbcFreeControls(XkbcDescPtr xkb, unsigned which, Bool freeMap);

extern int
XkbcAllocIndicatorMaps(XkbcDescPtr xkb);

extern void
XkbcFreeIndicatorMaps(XkbcDescPtr xkb);

extern XkbcDescRec *
XkbcAllocKeyboard(void);

extern int
XkbcAllocClientMap(XkbcDescPtr xkb, unsigned which, unsigned nTotalTypes);

extern int
XkbcAllocServerMap(XkbcDescPtr xkb, unsigned which, unsigned nNewActions);

extern void
XkbcFreeClientMap(XkbcDescPtr xkb, unsigned what, Bool freeMap);

extern void
XkbcFreeServerMap(XkbcDescPtr xkb, unsigned what, Bool freeMap);

extern void
XkbcFreeKeyboard(XkbcDescPtr xkb, unsigned which, Bool freeAll);

extern void
XkbcFreeGeomProperties(XkbGeometryPtr geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomKeyAliases(XkbGeometryPtr geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomColors(XkbGeometryPtr geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomPoints(XkbOutlinePtr outline, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomOutlines(XkbShapePtr shape, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomShapes(XkbGeometryPtr geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomOverlayKeys(XkbOverlayRowPtr row, int first, int count,
                        Bool freeAll);

extern void
XkbcFreeGeomOverlayRows(XkbOverlayPtr overlay, int first, int count,
                        Bool freeAll);

extern void
XkbcFreeGeomOverlays(XkbSectionPtr section, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomKeys(XkbRowPtr row, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomRows(XkbSectionPtr section, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomSections(XkbGeometryPtr geom, int first, int count, Bool freeAll);

extern void
XkbcFreeGeomDoodads(XkbDoodadPtr doodads, int nDoodads, Bool freeAll);

extern void
XkbcFreeGeometry(XkbGeometryPtr geom, unsigned which, Bool freeMap);

extern int
XkbcAllocGeometry(XkbcDescPtr xkb, XkbGeometrySizesPtr sizes);

extern void
XkbcInitAtoms(void);

extern char *
XkbcAtomGetString(Atom atom);

extern Atom
XkbcInternAtom(char *name, Bool onlyIfExists);

#endif /* _XKBCOMMON_H_ */
