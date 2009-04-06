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

#ifndef OPAQUE_DEFINED
typedef void *Opaque;
#endif
#ifndef NullOpaque
#define	NullOpaque	((Opaque)NULL)
#endif

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

extern Opaque
uAlloc(unsigned size);

extern Opaque
uCalloc(unsigned n, unsigned  size);

extern Opaque
uRealloc(Opaque old, unsigned newSize);

extern Opaque
uRecalloc(Opaque old, unsigned nOld, unsigned nNew, unsigned newSize);

extern void
uFree(Opaque ptr);

#define	uTypedAlloc(t)		((t *)uAlloc((unsigned)sizeof(t)))
#define	uTypedCalloc(n,t)	((t *)uCalloc((unsigned)n,(unsigned)sizeof(t)))
#define	uTypedRealloc(pO,n,t)	((t *)uRealloc((Opaque)pO,((unsigned)n)*sizeof(t)))
#define	uTypedRecalloc(pO,o,n,t) ((t *)uRecalloc((Opaque)pO,((unsigned)o),((unsigned)n),sizeof(t)))
#if (defined mdHasAlloca) && (mdHasAlloca)
#define	uTmpAlloc(n)	((Opaque)alloca((unsigned)n))
#define	uTmpFree(p)
#else
#define	uTmpAlloc(n)	uAlloc(n)
#define	uTmpFree(p)	uFree(p)
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

extern __ATTR_PRINTF(1, 2) void
uFatalError(const char *s, ...);

/* WSGO stands for "Weird Stuff Going On" (wtf???) */
#define WSGO			uInternalError

extern __ATTR_PRINTF(1, 2) void
uInternalError(const char *s, ...);

extern void
uSetPreErrorMessage(char *msg);

extern void
uSetPostErrorMessage(char *msg);

extern void
uSetErrorPrefix(char *pre);

extern void
uFinishUp(void);


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

extern __ATTR_PRINTF(1, 2) void
uDebug(char *s, ...);

extern __ATTR_PRINTF(1, 2) void
uDebugNOI(char *s, ...);

extern Boolean
uSetDebugFile(char *name);

extern FILE *uDebugFile;
extern int uDebugIndentLevel;
extern int uDebugIndentSize;

#define	uDebugIndent(l)		(uDebugIndentLevel+=(l))
#define	uDebugOutdent(l)	(uDebugIndentLevel-=(l))
#ifdef DEBUG_ON
#define	uDEBUG(f,s)		{ if (DEBUG_VAR&(f)) uDebug(s);}
#define	uDEBUG1(f,s,a)		{ if (DEBUG_VAR&(f)) uDebug(s,a);}
#define	uDEBUG2(f,s,a,b)	{ if (DEBUG_VAR&(f)) uDebug(s,a,b);}
#define	uDEBUG3(f,s,a,b,c)	{ if (DEBUG_VAR&(f)) uDebug(s,a,b,c);}
#define	uDEBUG4(f,s,a,b,c,d)	{ if (DEBUG_VAR&(f)) uDebug(s,a,b,c,d);}
#define	uDEBUG5(f,s,a,b,c,d,e)	{ if (DEBUG_VAR&(f)) uDebug(s,a,b,c,d,e);}
#define	uDEBUG_NOI(f,s)		{ if (DEBUG_VAR&(f)) uDebug(s);}
#define	uDEBUG_NOI1(f,s,a)	{ if (DEBUG_VAR&(f)) uDebugNOI(s,a);}
#define	uDEBUG_NOI2(f,s,a,b)	{ if (DEBUG_VAR&(f)) uDebugNOI(s,a,b);}
#define	uDEBUG_NOI3(f,s,a,b,c)	{ if (DEBUG_VAR&(f)) uDebugNOI(s,a,b,c);}
#define	uDEBUG_NOI4(f,s,a,b,c,d) { if (DEBUG_VAR&(f)) uDebugNOI(s,a,b,c,d);}
#define	uDEBUG_NOI5(f,s,a,b,c,d,e) { if (DEBUG_VAR&(f)) uDebugNOI(s,a,b,c,d,e);}
#else
#define	uDEBUG(f,s)
#define	uDEBUG1(f,s,a)
#define	uDEBUG2(f,s,a,b)
#define	uDEBUG3(f,s,a,b,c)
#define	uDEBUG4(f,s,a,b,c,d)
#define	uDEBUG5(f,s,a,b,c,d,e)
#define	uDEBUG_NOI(f,s)
#define	uDEBUG_NOI1(f,s,a)
#define	uDEBUG_NOI2(f,s,a,b)
#define	uDEBUG_NOI3(f,s,a,b,c)
#define	uDEBUG_NOI4(f,s,a,b,c,d)
#define	uDEBUG_NOI5(f,s,a,b,c,d,e)
#endif

extern Boolean
uSetEntryFile(char *name);

extern __ATTR_PRINTF(2, 3) void
uEntry(int l, char *s, ...);

extern void
uExit(int l, char *rtVal);

#ifdef ENTRY_TRACKING_ON
#define	ENTRY_BIT	0x10
#define	LOW_ENTRY_BIT	0x1000
#define	ENTER	(DEBUG_VAR&ENTRY_BIT)
#define	FLAG(fLag)	(DEBUG_VAR&(fLag))

extern int uEntryLevel;

#define	uENTRY(s)			{ if (ENTER) uEntry(1,s);}
#define	uENTRY1(s,a)			{ if (ENTER) uEntry(1,s,a);}
#define	uENTRY2(s,a,b)			{ if (ENTER) uEntry(1,s,a,b);}
#define	uENTRY3(s,a,b,c)		{ if (ENTER) uEntry(1,s,a,b,c);}
#define	uENTRY4(s,a,b,c,d)		{ if (ENTER) uEntry(1,s,a,b,c,d);}
#define	uENTRY5(s,a,b,c,d,e)		{ if (ENTER) uEntry(1,s,a,b,c,d,e);}
#define	uENTRY6(s,a,b,c,d,e,f)		{ if (ENTER) uEntry(1,s,a,b,c,d,e,f);}
#define	uENTRY7(s,a,b,c,d,e,f,g)	{ if (ENTER) uEntry(1,s,a,b,c,d,e,f,g);}
#define	uRETURN(v)			{ if (ENTER) uEntryLevel--; return(v); }
#define	uVOIDRETURN			{ if (ENTER) uEntryLevel--; return; }

#define	uFLAG_ENTRY(w,s)		{ if (FLAG(w)) uEntry(0,s);}
#define	uFLAG_ENTRY1(w,s,a)		{ if (FLAG(w)) uEntry(0,s,a);}
#define	uFLAG_ENTRY2(w,s,a,b)		{ if (FLAG(w)) uEntry(0,s,a,b);}
#define	uFLAG_ENTRY3(w,s,a,b,c)		{ if (FLAG(w)) uEntry(0,s,a,b,c);}
#define	uFLAG_ENTRY4(w,s,a,b,c,d)	{ if (FLAG(w)) uEntry(0,s,a,b,c,d);}
#define	uFLAG_ENTRY5(w,s,a,b,c,d,e)	{ if (FLAG(w)) uEntry(0,s,a,b,c,d,e);}
#define	uFLAG_ENTRY6(w,s,a,b,c,d,e,f)	{ if (FLAG(w)) uEntry(0,s,a,b,c,d,e,f);}
#define	uFLAG_ENTRY7(w,s,a,b,c,d,e,f,g)	{ if(FLAG(w))uEntry(0,s,a,b,c,d,e,f,g);}
#define	uFLAG_RETURN(v)			{ return(v);}
#define	uFLAG_VOIDRETURN		{ return; }
#else
#define	uENTRY(s)
#define	uENTRY1(s,a)
#define	uENTRY2(s,a1,a2)
#define	uENTRY3(s,a1,a2,a3)
#define	uENTRY4(s,a1,a2,a3,a4)
#define	uENTRY5(s,a1,a2,a3,a4,a5)
#define	uENTRY6(s,a1,a2,a3,a4,a5,a6)
#define	uENTRY7(s,a1,a2,a3,a4,a5,a6,a7)
#define	uRETURN(v)	{ return(v); }
#define	uVOIDRETURN	{ return; }

#define	uFLAG_ENTRY(f,s)
#define	uFLAG_ENTRY1(f,s,a)
#define	uFLAG_ENTRY2(f,s,a,b)
#define	uFLAG_ENTRY3(f,s,a,b,c)
#define	uFLAG_ENTRY4(f,s,a,b,c,d)
#define	uFLAG_ENTRY5(f,s,a,b,c,d,e)
#define	uFLAG_ENTRY6(f,s,a,b,c,d,e,g)
#define	uFLAG_ENTRY7(f,s,a,b,c,d,e,g,h)
#define	uFLAG_RETURN(v)			{ return(v);}
#define	uFLAG_VOIDRETURN		{ return; }
#endif

#endif /* UTILS_H */
