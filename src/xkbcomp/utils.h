#ifndef UTILS_H
#define	UTILS_H 1

  /*\
   *
   *                          COPYRIGHT 1990
   *                    DIGITAL EQUIPMENT CORPORATION
   *                       MAYNARD, MASSACHUSETTS
   *                        ALL RIGHTS RESERVED.
   *
   * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
   * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
   * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE
   * FOR ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED
   * WARRANTY.
   *
   * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT
   * RIGHTS, APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN
   * ADDITION TO THAT SET FORTH ABOVE.
   *
   * Permission to use, copy, modify, and distribute this software and its
   * documentation for any purpose and without fee is hereby granted, provided
   * that the above copyright notice appear in all copies and that both that
   * copyright notice and this permission notice appear in supporting
   * documentation, and that the name of Digital Equipment Corporation not be
   * used in advertising or publicity pertaining to distribution of the
   * software without specific, written prior permission.
   \*/

/***====================================================================***/

#include 	<stdio.h>
#include	<X11/Xos.h>
#include	<X11/Xfuncproto.h>
#include	<X11/Xfuncs.h>

#include <stddef.h>
#include "config.h"

#ifndef NUL
#define	NUL	'\0'
#endif

/***====================================================================***/

#ifndef BOOLEAN_DEFINED
typedef char Boolean;
#endif

#ifndef True
#define	True	((Boolean)1)
#define	False	((Boolean)0)
#endif /* ndef True */
#define	booleanText(b)	((b)?"True":"False")

#ifndef COMPARISON_DEFINED
typedef int Comparison;

#define	Greater		((Comparison)1)
#define	Equal		((Comparison)0)
#define	Less		((Comparison)-1)
#define	CannotCompare	((Comparison)-37)
#define	comparisonText(c)	((c)?((c)<0?"Less":"Greater"):"Equal")
#endif

/***====================================================================***/

extern void *
recalloc(void * old, unsigned nOld, unsigned nNew, unsigned newSize);

#define	uTypedAlloc(t)		((t *)malloc((unsigned)sizeof(t)))
#define	uTypedCalloc(n,t)	((t *)calloc((unsigned)n,(unsigned)sizeof(t)))
#define	uTypedRealloc(pO,n,t)	((t *)realloc((void *)pO,((unsigned)n)*sizeof(t)))
#define	uTypedRecalloc(pO,o,n,t) ((t *)recalloc((void *)pO,((unsigned)o),((unsigned)n),sizeof(t)))
#if (defined mdHasAlloca) && (mdHasAlloca)
#define	uTmpAlloc(n)	((void *)alloca((unsigned)n))
#define	uTmpFree(p)
#else
#define	uTmpAlloc(n)	malloc(n)
#define	uTmpFree(p)	free(p)
#endif

/***====================================================================***/

extern Boolean
uSetErrorFile(char *name);

#if defined(__GNUC__) && \
    ((__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 6)))
#define __ATTR_PRINTF(i, f) __attribute__ ((format(printf, (i), (f))))
#else
#define __ATTR_PRINTF(i, f)
#endif

#define INFO 			uInformation

extern __ATTR_PRINTF(1, 2) void
uInformation(const char *s, ...);

#define ACTION			uAction

extern __ATTR_PRINTF(1, 2) void
uAction(const char *s, ...);

#define WARN			uWarning

extern __ATTR_PRINTF(1, 2) void
uWarning(const char *s, ...);

#define ERROR			uError

extern __ATTR_PRINTF(1, 2) void
uError(const char *s, ...);

#define FATAL			uFatalError

extern __ATTR_PRINTF(1, 2) _X_NORETURN void
uFatalError(const char *s, ...);

/* WSGO stands for "Weird Stuff Going On" (wtf???) */
#define WSGO			uInternalError

extern __ATTR_PRINTF(1, 2) void
uInternalError(const char *s, ...);

/***====================================================================***/

#define	NullString	((char *)NULL)

#define	uStringText(s)		((s)==NullString?"<NullString>":(s))
#define	uStringEqual(s1,s2)	(uStringCompare(s1,s2)==Equal)
#define	uStringPrefix(p,s)	(strncmp(p,s,strlen(p))==0)
#define	uStringCompare(s1,s2)	(((s1)==NullString||(s2)==NullString)?\
                                 (s1)!=(s2):strcmp(s1,s2))
#define	uStrCaseEqual(s1,s2)	(uStrCaseCmp(s1,s2)==0)
#ifdef HAVE_STRCASECMP
#define	uStrCaseCmp(s1,s2)	(strcasecmp(s1,s2))
#define	uStrCasePrefix(p,s)	(strncasecmp(p,s,strlen(p))==0)
#else
extern int
uStrCaseCmp(const char *s1, const char *s2);
extern int
uStrCasePrefix(const char *p, char *str);
#endif

/***====================================================================***/

#ifdef	ASSERTIONS_ON
#define	uASSERT(where,why) \
	{if (!(why)) uFatalError("assertion botched in %s ( why )\n",where);}
#else
#define	uASSERT(where,why)
#endif

/***====================================================================***/

#ifndef DEBUG_VAR
#define	DEBUG_VAR	debugFlags
#endif

extern unsigned int DEBUG_VAR;

#endif /* UTILS_H */
