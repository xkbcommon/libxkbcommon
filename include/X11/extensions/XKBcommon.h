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

#include <stdint.h>
#include <stdio.h>
#include <X11/Xfuncproto.h>
#include <X11/extensions/XKBstrcommon.h>
#include <X11/extensions/XKBrulescommon.h>

#define KeySym CARD32
#define Atom CARD32

/* Action structures used in the server */

#define XkbcAnyActionDataSize 18
typedef struct _XkbcAnyAction {
    unsigned char   type;
    unsigned char   pad[XkbcAnyActionDataSize];
} XkbcAnyAction;

typedef struct _XkbcModAction {
    unsigned char   type;
    uint8_t         flags;
    uint32_t        mask;
    uint32_t        real_mods;
    uint32_t        vmods;
} XkbcModAction;

typedef struct _XkbcGroupAction {
    unsigned char   type;
    unsigned char   flags;
    int16_t         group;
} XkbcGroupAction;

typedef struct _XkbcISOAction {
    unsigned char   type;
    uint8_t         flags;
    int16_t         group;
    uint32_t        mask;
    uint32_t        vmods;
    uint32_t        real_mods;
    uint8_t         affect;
} XkbcISOAction;

typedef struct _XkbcCtrlsAction {
    unsigned char   type;
    unsigned char   flags;
    uint32_t        ctrls;
} XkbcCtrlsAction;

typedef struct _XkbcDeviceBtnAction {
    unsigned char   type;
    unsigned char   flags;
    uint16_t        device;
    uint16_t        button;
    uint8_t         count;
} XkbcDeviceBtnAction;

typedef struct _XkbcDeviceValuatorAction {
    unsigned char   type;
    uint8_t         v1_what;
    uint16_t        device;
    uint16_t        v1_index;
    int16_t         v1_value;
    uint16_t        v2_index;
    int16_t         v2_value;
    uint8_t         v2_what;
} XkbcDeviceValuatorAction;

typedef struct _XkbcPtrDfltAction {
    unsigned char   type;
    uint8_t         flags;
    uint8_t         affect;
    uint8_t         value;
} XkbcPtrDfltAction;

typedef struct _XkbcSwitchScreenAction {
    unsigned char   type;
    uint8_t         flags;
    uint8_t         screen;
} XkbcSwitchScreenAction;

typedef union _XkbcAction {
    XkbcAnyAction            any;
    XkbcModAction            mods;
    XkbcGroupAction          group;
    XkbcISOAction            iso;
    XkbcCtrlsAction          ctrls;
    XkbcDeviceBtnAction      devbtn;
    XkbcDeviceValuatorAction devval;
    XkbcPtrDfltAction        dflt;
    XkbcSwitchScreenAction   screen;
    XkbRedirectKeyAction     redirect; /* XXX wholly unnecessary? */
    XkbPtrAction             ptr; /* XXX delete for DeviceValuator */
    XkbPtrBtnAction          btn; /* XXX delete for DeviceBtn */
    XkbMessageAction         msg; /* XXX just delete */
    unsigned char           type;
} XkbcAction;

typedef struct _XkbcSymInterpretRec {
    CARD32          sym;
    unsigned char   flags;
    unsigned char   match;
    unsigned char   mods;
    unsigned char   virtual_mod;
    XkbcAnyAction    act;
} XkbcSymInterpretRec,*XkbcSymInterpretPtr;

typedef struct _XkbcCompatMapRec {
    XkbcSymInterpretPtr      sym_interpret;
    XkbModsRec               groups[XkbNumKbdGroups];
    unsigned short           num_si;
    unsigned short           size_si;
} XkbcCompatMapRec, *XkbcCompatMapPtr;

typedef struct _XkbcClientMapRec {
    unsigned char            size_types;
    unsigned char            num_types;
    XkbKeyTypePtr            types;

    unsigned short           size_syms;
    unsigned short           num_syms;
    uint32_t                *syms;
    XkbSymMapPtr             key_sym_map;

    unsigned char           *modmap;
} XkbcClientMapRec, *XkbcClientMapPtr;

typedef struct _XkbcServerMapRec {
    unsigned short      num_acts;
    unsigned short      size_acts;

#if defined(__cplusplus) || defined(c_plusplus)
    /* explicit is a C++ reserved word */
    unsigned char *     c_explicit;
#else
    unsigned char *     explicit;
#endif

    XkbcAction          *acts;
    XkbBehavior         *behaviors;
    unsigned short      *key_acts;
    unsigned char       *explicits;
    uint32_t            vmods[XkbNumVirtualMods];
    uint32_t            *vmodmap;
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
    XkbcClientMapPtr    map;
    XkbIndicatorPtr     indicators;
    XkbNamesPtr         names;
    XkbcCompatMapPtr    compat;
    XkbGeometryPtr      geom;
} XkbcDescRec, *XkbcDescPtr;

_XFUNCPROTOBEGIN

typedef uint32_t (*InternAtomFuncPtr)(const char *val);
typedef const char *(*GetAtomValueFuncPtr)(uint32_t atom);

extern void
XkbcInitAtoms(InternAtomFuncPtr intern, GetAtomValueFuncPtr get_atom_value);

extern XkbcDescPtr
XkbcCompileKeymapFromRules(const XkbRMLVOSet *rmlvo);

extern XkbcDescPtr
XkbcCompileKeymapFromComponents(const XkbComponentNamesPtr ktcsg);

extern XkbcDescPtr
XkbcCompileKeymapFromFile(FILE *inputFile, const char *mapName);

extern XkbComponentListPtr
XkbcListComponents(XkbComponentNamesPtr ptrns, int *maxMatch);

/*
 * Canonicalises component names by prepending the relevant component from
 * 'old' to the one in 'names' when the latter has a leading '+' or '|', and
 * by replacing a '%' with the relevant component, e.g.:
 *
 * names        old           output
 * ------------------------------------------
 * +bar         foo           foo+bar
 * |quux        baz           baz|quux
 * foo+%|baz    bar           foo+bar|baz
 *
 * If a component in names needs to be modified, the existing value will be
 * free()d, and a new one allocated with malloc().
 */
extern void
XkbcCanonicaliseComponents(XkbComponentNamesPtr names,
                           const XkbComponentNamesPtr old);

/*
 * Converts a keysym to a string; will return unknown Unicode codepoints
 * as "Ua1b2", and other unknown keysyms as "0xabcd1234".
 *
 * The string returned may become invalidated after the next call to
 * XkbcKeysymToString: if you need to preserve it, then you must
 * duplicate it.
 *
 * This is CARD32 rather than KeySym, as KeySym changes size between
 * client and server (no, really).
 */
extern char *
XkbcKeysymToString(CARD32 ks);

/*
 * See XkbcKeysymToString comments: this function will accept any string
 * from that function.
 */
extern CARD32
XkbcStringToKeysym(const char *s);

_XFUNCPROTOEND

#undef KeySym
#undef Atom

#endif /* _XKBCOMMON_H_ */
