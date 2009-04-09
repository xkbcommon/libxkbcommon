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

#include <X11/Xfuncproto.h>

/* Action structures used in the server */
typedef struct _XkbcModAction {
    unsigned char   type;
    unsigned char   flags;
    unsigned char   mask;
    unsigned char   real_mods;
    unsigned int    vmods;
} XkbcModAction;

typedef struct _XkbcISOAction {
    unsigned char   type;
    unsigned char   flags;
    unsigned char   mask;
    unsigned char   real_mods;
    /* FIXME: Make this an int. */
    char            group_XXX;
    unsigned char   affect;
    unsigned int    vmods;
} XkbcISOAction;

typedef struct _XkbcPtrAction {
    unsigned char   type;
    unsigned char   flags;
    int             x;
    int             y;
} XkbcPtrAction;

typedef struct _XkbcCtrlsAction {
    unsigned char   type;
    unsigned char   flags;
    unsigned long   ctrls;
} XkbcCtrlsAction;

typedef struct  _XkbcRedirectKeyAction {
    unsigned char   type;
    unsigned char   new_key;
    unsigned char   mods_mask;
    unsigned char   mods;
    unsigned int    vmods_mask;
    unsigned int    vmods;
} XkbcRedirectKeyAction;

typedef union _XkbcAction {
    XkbAnyAction            any;
    XkbcModAction           mods;
    XkbGroupAction          group;
    XkbcISOAction           iso;
    XkbcPtrAction           ptr;
    XkbPtrBtnAction         btn;
    XkbPtrDfltAction        dflt;
    XkbSwitchScreenAction   screen;
    XkbcCtrlsAction         ctrls;
    XkbMessageAction        msg;
    XkbcRedirectKeyAction   redirect;
    XkbDeviceBtnAction      devbtn;
    XkbDeviceValuatorAction devval;
    unsigned char           type;
} XkbcAction;

typedef struct _XkbcServerMapRec {
    unsigned short      num_acts;
    unsigned short      size_acts;
    XkbcAction *        acts;

    XkbBehavior *       behaviors;
    unsigned short *    key_acts;
#if defined(__cplusplus) || defined(c_plusplus)
    /* explicit is a C++ reserved word */
    unsigned char *     c_explicit;
#else
    unsigned char *     explicit;
#endif
    unsigned char       vmods[XkbNumVirtualMods];
    unsigned short *    vmodmap;
} XkbcServerMapRec, *XkbcServerMapPtr;

/* Common keyboard description structure */
typedef struct _XkbcDesc {
    unsigned int        defined;
    unsigned short      flags;
    unsigned short      device_spec;
    KeyCode             min_key_code;
    KeyCode             max_key_code;

    XkbControlsPtr      ctrls;
    XkbcServerMapPtr    server;
    XkbClientMapPtr     map;
    XkbIndicatorPtr     indicators;
    XkbNamesPtr         names;
    XkbCompatMapPtr     compat;
    XkbGeometryPtr      geom;
} XkbcDescRec, *XkbcDescPtr;

_XFUNCPROTOBEGIN

extern XkbcDescPtr
XkbcCompileKeymapFromRules(const XkbRMLVOSet *rmlvo);

extern XkbcDescPtr
XkbcCompileKeymapFromComponents(const XkbComponentNamesPtr ktcsg);

_XFUNCPROTOEND

#endif /* _XKBCOMMON_H_ */
