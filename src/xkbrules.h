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

#ifndef _XKBRULES_H_
#define _XKBRULES_H_

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xdefs.h>
#include "xkbcommon/xkbcommon.h"


/* From filecommon */

#define XkbXKMFile  0
#define XkbCFile    1
#define XkbXKBFile  2
#define XkbMessage  3

#define XkbMapDefined   (1 << 0)
#define XkbStateDefined (1 << 1)

/***====================================================================***/

#define _XkbSuccess                 0
#define _XkbErrMissingNames         1
#define _XkbErrMissingTypes         2
#define _XkbErrMissingReqTypes      3
#define _XkbErrMissingSymbols       4
#define _XkbErrMissingVMods         5
#define _XkbErrMissingIndicators    6
#define _XkbErrMissingCompatMap     7
#define _XkbErrMissingSymInterps    8
#define _XkbErrMissingGeometry      9
#define _XkbErrIllegalDoodad        10
#define _XkbErrIllegalTOCType       11
#define _XkbErrIllegalContents      12
#define _XkbErrEmptyFile            13
#define _XkbErrFileNotFound         14
#define _XkbErrFileCannotOpen       15
#define _XkbErrBadValue             16
#define _XkbErrBadMatch             17
#define _XkbErrBadTypeName          18
#define _XkbErrBadTypeWidth         19
#define _XkbErrBadFileType          20
#define _XkbErrBadFileVersion       21
#define _XkbErrBadFileFormat        22
#define _XkbErrBadAlloc             23
#define _XkbErrBadLength            24
#define _XkbErrXReqFailure          25
#define _XkbErrBadImplementation    26

typedef struct _XkbRF_VarDefs {
    char *          model;
    char *          layout;
    char *          variant;
    char *          options;
    unsigned short  sz_extra;
    unsigned short  num_extra;
    char *          extra_names;
    char **         extra_values;
} XkbRF_VarDefsRec,*XkbRF_VarDefsPtr;

typedef struct _XkbRF_VarDesc {
    char *  name;
    char *  desc;
} XkbRF_VarDescRec, *XkbRF_VarDescPtr;

typedef struct _XkbRF_DescribeVars {
    int                 sz_desc;
    int                 num_desc;
    XkbRF_VarDescPtr    desc;
} XkbRF_DescribeVarsRec,*XkbRF_DescribeVarsPtr;

typedef struct _XkbRF_Rule {
    int         number;
    int         layout_num;
    int         variant_num;
    char *      model;
    char *      layout;
    char *      variant;
    char *      option;
    /* yields */
    char *      keycodes;
    char *      symbols;
    char *      types;
    char *      compat;
    char *      geometry;
    char *      keymap;
    unsigned    flags;
} XkbRF_RuleRec,*XkbRF_RulePtr;

typedef struct _XkbRF_Group {
    int     number;
    char *  name;
    char *  words;
} XkbRF_GroupRec, *XkbRF_GroupPtr;

#define XkbRF_PendingMatch  (1L<<1)
#define XkbRF_Option        (1L<<2)
#define XkbRF_Append        (1L<<3)
#define XkbRF_Normal        (1L<<4)
#define XkbRF_Invalid       (1L<<5)

typedef struct _XkbRF_Rules {
    XkbRF_DescribeVarsRec   models;
    XkbRF_DescribeVarsRec   layouts;
    XkbRF_DescribeVarsRec   variants;
    XkbRF_DescribeVarsRec   options;
    unsigned short          sz_extra;
    unsigned short          num_extra;
    char **                 extra_names;
    XkbRF_DescribeVarsPtr   extra;

    unsigned short          sz_rules;
    unsigned short          num_rules;
    XkbRF_RulePtr           rules;
    unsigned short          sz_groups;
    unsigned short          num_groups;
    XkbRF_GroupPtr          groups;
} XkbRF_RulesRec, *XkbRF_RulesPtr;

#define _XKB_RF_NAMES_PROP_ATOM     "_XKB_RULES_NAMES"
#define _XKB_RF_NAMES_PROP_MAXLEN   1024

/* Action structures used in the server */

extern Bool
XkbcRF_GetComponents(XkbRF_RulesPtr rules, XkbRF_VarDefsPtr defs,
                     struct xkb_component_names * names);

extern Bool
XkbcRF_LoadRules(FILE *file, XkbRF_RulesPtr rules);

extern void
XkbcRF_Free(XkbRF_RulesPtr rules, Bool freeRules);

#endif /* _XKBRULES_H_ */
