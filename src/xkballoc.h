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

#ifndef _XKBALLOC_H_
#define _XKBALLOC_H_

#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/extensions/XKBstrcommon.h>
#include <X11/extensions/XKBrulescommon.h>
#include "X11/extensions/XKBcommon.h"

extern int
XkbcAllocCompatMap(XkbcDescPtr xkb, unsigned which, unsigned nSI);

extern void
XkbcFreeCompatMap(XkbcDescPtr xkb, unsigned which, Bool freeMap);

extern int
XkbcAllocNames(XkbcDescPtr xkb, unsigned which, int nTotalRG,
               int nTotalAliases);

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

extern void
XkbcFreeKeyboard(XkbcDescPtr xkb, unsigned which, Bool freeAll);

/***====================================================================***/

extern int
XkbcAllocClientMap(XkbcDescPtr xkb, unsigned which, unsigned nTotalTypes);

extern int
XkbcAllocServerMap(XkbcDescPtr xkb, unsigned which, unsigned nNewActions);

extern int
XkbcCopyKeyType(XkbcKeyTypePtr from, XkbcKeyTypePtr into);

extern int
XkbcCopyKeyTypes(XkbcKeyTypePtr from, XkbcKeyTypePtr into, int num_types);

extern int
XkbcResizeKeyType(XkbcDescPtr xkb, int type_ndx, int map_count,
                  Bool want_preserve, int new_num_lvls);

extern uint32_t *
XkbcResizeKeySyms(XkbcDescPtr xkb, int key, int needed);

extern int
XkbcChangeKeycodeRange(XkbcDescPtr xkb, int minKC, int maxKC,
                       XkbChangesPtr changes);

extern XkbcAction *
XkbcResizeKeyActions(XkbcDescPtr xkb, int key, int needed);

extern void
XkbcFreeClientMap(XkbcDescPtr xkb, unsigned what, Bool freeMap);

extern void
XkbcFreeServerMap(XkbcDescPtr xkb, unsigned what, Bool freeMap);

#endif /* _XKBALLOC_H_ */
