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
#include "X11/extensions/XKBcommon.h"

extern Bool
XkbcRF_GetComponents(XkbRF_RulesPtr rules, XkbRF_VarDefsPtr defs,
                     struct xkb_component_names * names);

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

#endif /* _XKBRULES_H_ */
