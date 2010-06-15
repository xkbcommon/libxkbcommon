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

#ifdef MALLOC_0_RETURNS_NULL
# define Xmalloc(size) malloc(((size) == 0 ? 1 : (size)))
# define Xrealloc(ptr, size) realloc((ptr), ((size) == 0 ? 1 : (size)))
# define Xcalloc(nelem, elsize) calloc(((nelem) == 0 ? 1 : (nelem)), (elsize))
#else
# define Xmalloc(size) malloc((size))
# define Xrealloc(ptr, size) realloc((ptr), (size))
# define Xcalloc(nelem, elsize) calloc((nelem), (elsize))
#endif

#define _XkbAlloc(s)            Xmalloc((s))
#define _XkbCalloc(n,s)         Xcalloc((n),(s))
#define _XkbRealloc(o,s)        Xrealloc((o),(s))
#define _XkbTypedAlloc(t)       ((t *)Xmalloc(sizeof(t)))
#define _XkbTypedCalloc(n,t)    ((t *)Xcalloc((n),sizeof(t)))
#define _XkbTypedRealloc(o,n,t) \
    ((o)?(t *)Xrealloc((o),(n)*sizeof(t)):_XkbTypedCalloc(n,t))
#define _XkbClearElems(a,f,l,t) bzero(&(a)[f],((l)-(f)+1)*sizeof(t))
#define _XkbFree(p)             free((p))

#define _XkbDupString(s)        ((s) ? strdup(s) : NULL)
#define _XkbStrCaseCmp          strcasecmp

extern char *tbGetBuffer(unsigned int len);

#endif /* _XKBCOMMONINT_H_ */
