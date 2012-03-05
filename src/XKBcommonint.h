/*
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

#ifndef _XKBCOMMONINT_H_
#define _XKBCOMMONINT_H_

#include <stdlib.h>
#include <string.h>

#ifndef True
#define True  1
#define False 0
#endif

#define _XkbTypedAlloc(t)       ((t *)malloc(sizeof(t)))
#define _XkbTypedCalloc(n,t)    ((t *)calloc((n),sizeof(t)))
#define _XkbTypedRealloc(o,n,t) \
    ((o)?(t *)realloc((o),(n)*sizeof(t)):_XkbTypedCalloc(n,t))
#define _XkbClearElems(a,f,l,t) memset(&(a)[f], 0, ((l) - (f) + 1) * sizeof(t))

#define _XkbDupString(s)        ((s) ? strdup(s) : NULL)
#define _XkbStrCaseCmp          strcasecmp

/* From XKM.h */
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

#define	XkmTypesMask		(1<<0)
#define	XkmCompatMapMask	(1<<1)
#define	XkmSymbolsMask		(1<<2)
#define	XkmIndicatorsMask	(1<<3)
#define	XkmKeyNamesMask		(1<<4)
#define	XkmGeometryMask		(1<<5)
#define	XkmVirtualModsMask	(1<<6)
#define	XkmLegalIndexMask	(0x7f)
#define	XkmAllIndicesMask	(0x7f)

#define	XkmSemanticsRequired	(XkmCompatMapMask)
#define	XkmSemanticsOptional	(XkmTypesMask|XkmVirtualModsMask|XkmIndicatorsMask)
#define	XkmSemanticsLegal	(XkmSemanticsRequired|XkmSemanticsOptional)
#define	XkmLayoutRequired	(XkmKeyNamesMask|XkmSymbolsMask|XkmTypesMask)
#define	XkmLayoutOptional	(XkmVirtualModsMask|XkmGeometryMask)
#define	XkmLayoutLegal		(XkmLayoutRequired|XkmLayoutOptional)
#define	XkmKeymapRequired	(XkmSemanticsRequired|XkmLayoutRequired)
#define	XkmKeymapOptional	((XkmSemanticsOptional|XkmLayoutOptional)&(~XkmKeymapRequired))
#define	XkmKeymapLegal		(XkmKeymapRequired|XkmKeymapOptional)

#define	XkmLegalSection(m)	(((m)&(~XkmKeymapLegal))==0)
#define	XkmSingleSection(m)	(XkmLegalSection(m)&&(((m)&(~(m)+1))==(m)))

#endif /* _XKBCOMMONINT_H_ */
