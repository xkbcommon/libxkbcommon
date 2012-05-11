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

#ifndef RULES_H
#define RULES_H

#include <stdio.h>

#include "xkb-priv.h"

/* From filecommon */

typedef struct _XkbRF_VarDefs {
    const char *    model;
    const char *    layout;
    const char *    variant;
    const char *    options;
} XkbRF_VarDefsRec,*XkbRF_VarDefsPtr;

typedef struct _XkbRF_VarDesc {
    char *  name;
    char *  desc;
} XkbRF_VarDescRec, *XkbRF_VarDescPtr;

typedef struct _XkbRF_DescribeVars {
    size_t              sz_desc;
    size_t              num_desc;
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

    size_t                  sz_rules;
    size_t                  num_rules;
    XkbRF_RulePtr           rules;
    size_t                  sz_groups;
    size_t                  num_groups;
    XkbRF_GroupPtr          groups;
} XkbRF_RulesRec, *XkbRF_RulesPtr;

extern bool
XkbcRF_GetComponents(XkbRF_RulesPtr rules, XkbRF_VarDefsPtr defs,
                     struct xkb_component_names * names);

extern bool
XkbcRF_LoadRules(FILE *file, XkbRF_RulesPtr rules);

extern void
XkbcRF_Free(XkbRF_RulesPtr rules);

#endif /* RULES_H */
