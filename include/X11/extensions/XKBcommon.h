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

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/Xfuncproto.h>
#include <X11/extensions/XKBstrcommon.h>
#include <X11/extensions/XKBgeomcommon.h>
#include <X11/extensions/XKBrulescommon.h>

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

#define _XkbcKSLower (1 << 0)
#define _XkbcKSUpper (1 << 1)

#define XkbcKSIsLower(k) (_XkbcKSCheckCase(k) & _XkbcKSLower)
#define XkbcKSIsUpper(k) (_XkbcKSCheckCase(k) & _XkbcKSUpper)
#define XkbcKSIsKeypad(k) \
    (((k) >= XK_KP_Space) && ((k) <= XK_KP_Equal))
#define XkbcKSIsDeadKey(k) \
    (((k) >= XK_dead_grave) && ((k) <= XK_dead_semivoiced_sound))

_XFUNCPROTOBEGIN

extern XkbcDescPtr
XkbcCompileKeymapFromRules(const char *rules, XkbRF_VarDefsPtr defs);

extern XkbcDescPtr
XkbcCompileKeymapFromComponents(XkbComponentNamesPtr ktcsg);

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

extern void
XkbcFreeKeyboard(XkbcDescPtr xkb, unsigned which, Bool freeAll);

extern int
XkbcAllocClientMap(XkbcDescPtr xkb, unsigned which, unsigned nTotalTypes);

extern int
XkbcAllocServerMap(XkbcDescPtr xkb, unsigned which, unsigned nNewActions);

extern int
XkbcCopyKeyType(XkbKeyTypePtr from, XkbKeyTypePtr into);

extern int
XkbcCopyKeyTypes(XkbKeyTypePtr from, XkbKeyTypePtr into, int num_types);

extern int
XkbcResizeKeyType(XkbcDescPtr xkb, int type_ndx, int map_count,
                  Bool want_preserve, int new_num_lvls);

extern KeySym *
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
XkbcAllocGeomProps(XkbGeometryPtr geom, int nProps);

extern int
XkbcAllocGeomColors(XkbGeometryPtr geom, int nColors);

extern int
XkbcAllocGeomKeyAliases(XkbGeometryPtr geom, int nKeyAliases);

extern int
XkbcAllocGeomShapes(XkbGeometryPtr geom, int nShapes);

extern int
XkbcAllocGeomSections(XkbGeometryPtr geom, int nSections);

extern int
XkbcAllocGeomOverlays(XkbSectionPtr section, int nOverlays);

extern int
XkbcAllocGeomOverlayRows(XkbOverlayPtr overlay, int nRows);

extern int
XkbcAllocGeomOverlayKeys(XkbOverlayRowPtr row, int nKeys);

extern int
XkbcAllocGeomDoodads(XkbGeometryPtr geom, int nDoodads);

extern int
XkbcAllocGeomSectionDoodads(XkbSectionPtr section, int nDoodads);

extern int
XkbcAllocGeomOutlines(XkbShapePtr shape, int nOL);

extern int
XkbcAllocGeomRows(XkbSectionPtr section, int nRows);

extern int
XkbcAllocGeomPoints(XkbOutlinePtr ol, int nPts);

extern int
XkbcAllocGeomKeys(XkbRowPtr row, int nKeys);

extern int
XkbcAllocGeometry(XkbcDescPtr xkb, XkbGeometrySizesPtr sizes);

extern XkbPropertyPtr
XkbcAddGeomProperty(XkbGeometryPtr geom, char *name, char *value);

extern XkbKeyAliasPtr
XkbcAddGeomKeyAlias(XkbGeometryPtr geom, char *aliasStr, char *realStr);

extern XkbColorPtr
XkbcAddGeomColor(XkbGeometryPtr geom, char *spec, unsigned int pixel);

extern XkbOutlinePtr
XkbcAddGeomOutline(XkbShapePtr shape, int sz_points);

extern XkbShapePtr
XkbcAddGeomShape(XkbGeometryPtr geom, Atom name, int sz_outlines);

extern XkbKeyPtr
XkbcAddGeomKey(XkbRowPtr row);

extern XkbRowPtr
XkbcAddGeomRow(XkbSectionPtr section, int sz_keys);

extern XkbSectionPtr
XkbcAddGeomSection(XkbGeometryPtr geom, Atom name,
                   int sz_rows, int sz_doodads, int sz_over);

extern XkbDoodadPtr
XkbcAddGeomDoodad(XkbGeometryPtr geom, XkbSectionPtr section, Atom name);

extern XkbOverlayKeyPtr
XkbcAddGeomOverlayKey(XkbOverlayPtr overlay, XkbOverlayRowPtr row,
                      char *over, char *under);

extern XkbOverlayRowPtr
XkbcAddGeomOverlayRow(XkbOverlayPtr overlay, int row_under, int sz_keys);

extern XkbOverlayPtr
XkbcAddGeomOverlay(XkbSectionPtr section, Atom name, int sz_rows);

extern void
XkbcInitAtoms(void);

extern char *
XkbcAtomGetString(Atom atom);

extern Atom
XkbcInternAtom(char *name, Bool onlyIfExists);

extern char *
XkbcAtomText(Atom atm);

extern char *
XkbcVModIndexText(XkbcDescPtr xkb, unsigned ndx);

extern char *
XkbcVModMaskText(XkbcDescPtr xkb, unsigned modMask, unsigned mask);

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
XkbcKeysymText(KeySym sym);

extern char *
XkbcKeyNameText(char *name);

extern char *
XkbcSIMatchText(unsigned type);

extern Bool
XkbcComputeShapeBounds(XkbShapePtr shape);

extern Bool
XkbcComputeShapeTop(XkbShapePtr shape, XkbBoundsPtr bounds);

extern Bool
XkbcComputeRowBounds(XkbGeometryPtr geom, XkbSectionPtr section, XkbRowPtr row);

extern Bool
XkbcComputeSectionBounds(XkbGeometryPtr geom, XkbSectionPtr section);

extern Bool
XkbcRF_GetComponents(XkbRF_RulesPtr rules, XkbRF_VarDefsPtr defs,
                     XkbComponentNamesPtr names);

extern XkbRF_RulePtr
XkbcRF_AddRule(XkbRF_RulesPtr rules);

extern XkbRF_GroupPtr
XkbcRF_AddGroup(XkbRF_RulesPtr rules);

extern Bool
XkbcRF_LoadRules(FILE *file, XkbRF_RulesPtr rules);

extern Bool
XkbcRF_LoadRulesByName(char *base, char *locale, XkbRF_RulesPtr rules);

extern XkbRF_VarDescPtr
XkbcRF_AddVarDesc(XkbRF_DescribeVarsPtr vars);

extern XkbRF_VarDescPtr
XkbcRF_AddVarDescCopy(XkbRF_DescribeVarsPtr vars, XkbRF_VarDescPtr from);

extern XkbRF_DescribeVarsPtr
XkbcRF_AddVarToDescribe(XkbRF_RulesPtr rules, char *name);

extern Bool
XkbcRF_LoadDescriptions(FILE *file, XkbRF_RulesPtr rules);

extern Bool
XkbcRF_LoadDescriptionsByName(char *base, char *locale, XkbRF_RulesPtr rules);

extern XkbRF_RulesPtr
XkbcRF_Load(char *base, char *locale, Bool wantDesc, Bool wantRules);

extern XkbRF_RulesPtr
XkbcRF_Create(int szRules, int szExtra);

extern void
XkbcRF_Free(XkbRF_RulesPtr rules, Bool freeRules);

extern int
XkbcInitCanonicalKeyTypes(XkbcDescPtr xkb, unsigned which, int keypadVMod);

extern Bool
XkbcVirtualModsToReal(XkbcDescPtr xkb, unsigned virtual_mask,
                      unsigned *mask_rtrn);

extern Bool
XkbcComputeEffectiveMap(XkbcDescPtr xkb, XkbKeyTypePtr type,
                        unsigned char *map_rtrn);

extern void
XkbcEnsureSafeMapName(char *name);

extern unsigned
_XkbcKSCheckCase(KeySym sym);

_XFUNCPROTOEND

#endif /* _XKBCOMMON_H_ */
