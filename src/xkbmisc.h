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

#ifndef _XKBMISC_H_
#define _XKBMISC_H_

#include <X11/X.h>
#include <X11/Xdefs.h>
#include "X11/extensions/XKBcommon.h"

/***====================================================================***/

extern Bool
XkbcComputeEffectiveMap(struct xkb_desc * xkb, struct xkb_key_type * type,
                        unsigned char *map_rtrn);

/***====================================================================***/

extern int
XkbcInitCanonicalKeyTypes(struct xkb_desc * xkb, unsigned which, int keypadVMod);

extern Bool
XkbcVirtualModsToReal(struct xkb_desc * xkb, unsigned virtual_mask,
                      unsigned *mask_rtrn);

extern void
XkbcEnsureSafeMapName(char *name);

extern unsigned
_XkbcKSCheckCase(uint32_t sym);

#define _XkbKSLower (1 << 0)
#define _XkbKSUpper (1 << 1)

#define XkbcKSIsLower(k) (_XkbcKSCheckCase(k) & _XkbKSLower)
#define XkbcKSIsUpper(k) (_XkbcKSCheckCase(k) & _XkbKSUpper)

#define XkbKSIsKeypad(k) (((k) >= XK_KP_Space) && ((k) <= XK_KP_Equal))
#define XkbKSIsDeadKey(k) \
    (((k) >= XK_dead_grave) && ((k) <= XK_dead_semivoiced_sound))

extern Bool
XkbcNameMatchesPattern(char *name, char *ptrn);

/***====================================================================***/

extern char *
XkbcAtomGetString(uint32_t atom);

extern uint32_t
XkbcInternAtom(const char *name, Bool onlyIfExists);

/***====================================================================***/

extern const char *
XkbcAtomText(uint32_t atm);

extern char *
XkbcVModMaskText(struct xkb_desc * xkb, unsigned modMask, unsigned mask);

extern char *
XkbcModIndexText(unsigned ndx);

extern char *
XkbcModMaskText(unsigned mask, Bool cFormat);

extern char *
XkbcConfigText(unsigned config);

extern char *
XkbcGeomFPText(int val);

extern char *
XkbcActionTypeText(unsigned type);

extern char *
XkbcKeysymText(uint32_t sym);

extern char *
XkbcKeyNameText(char *name);

extern char *
XkbcSIMatchText(unsigned type);

#endif /* _XKBMISC_H_ */
