/***********************************************************
Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/************************************************************
 Copyright 1994 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "xkbmisc.h"
#include "xkbcommon/xkbcommon.h"
#include "XKBcommonint.h"

#define InitialTableSize 100

typedef struct _Node {
    struct _Node    *left, *right;
    uint32_t          a;
    unsigned int    fingerPrint;
    char            *string;
} NodeRec, *NodePtr;

#define BAD_RESOURCE 0xe0000000

static xkb_atom_t lastAtom = XKB_ATOM_NONE;
static NodePtr atomRoot;
static unsigned long tableLength;
static NodePtr *nodeTable;

const char *
XkbcAtomText(xkb_atom_t atom)
{
    NodePtr node;

    if ((atom == XKB_ATOM_NONE) || (atom > lastAtom))
        return NULL;
    if (!(node = nodeTable[atom]))
        return NULL;
    return node->string;
}

char *
XkbcAtomGetString(xkb_atom_t atom)
{
    const char *ret = XkbcAtomText(atom);
    return ret ? strdup(ret) : NULL;
}

xkb_atom_t
xkb_intern_atom(const char *string)
{
    NodePtr *np;
    NodePtr nd;
    unsigned i;
    int comp;
    unsigned int fp = 0;
    size_t len;

    if (!string)
	return XKB_ATOM_NONE;

    len = strlen(string);
    np = &atomRoot;
    for (i = 0; i < (len + 1) / 2; i++) {
        fp = fp * 27 + string[i];
        fp = fp * 27 + string[len - 1 - i];
    }

    while (*np) {
        if (fp < (*np)->fingerPrint)
            np = &((*np)->left);
        else if (fp > (*np)->fingerPrint)
            np = &((*np)->right);
        else {
            /* now start testing the strings */
            comp = strncmp(string, (*np)->string, len);
            if ((comp < 0) || ((comp == 0) && (len < strlen((*np)->string))))
                np = &((*np)->left);
            else if (comp > 0)
                np = &((*np)->right);
            else
                return(*np)->a;
            }
    }

    nd = malloc(sizeof(NodeRec));
    if (!nd)
        return BAD_RESOURCE;

    nd->string = malloc(len + 1);
    if (!nd->string) {
        free(nd);
        return BAD_RESOURCE;
    }
    strncpy(nd->string, string, len);
    nd->string[len] = 0;

    if ((lastAtom + 1) >= tableLength) {
        NodePtr *table;
        int newLength;

        if (tableLength == 0)
            newLength = InitialTableSize;
        else
            newLength = tableLength * 2;

        table = realloc(nodeTable, newLength * sizeof(NodePtr));
        if (!table) {
            if (nd->string != string)
                free(nd->string);
            free(nd);
            return BAD_RESOURCE;
        }
        tableLength = newLength;
        table[XKB_ATOM_NONE] = NULL;

        nodeTable = table;
    }

    *np = nd;
    nd->left = nd->right = NULL;
    nd->fingerPrint = fp;
    nd->a = (++lastAtom);
    *(nodeTable + lastAtom) = nd;

    return nd->a;
}

static void
FreeAtom(NodePtr patom)
{
    if (patom->left)
        FreeAtom(patom->left);
    if (patom->right)
        FreeAtom(patom->right);
    free(patom->string);
    free(patom);
}

void
XkbcFreeAllAtoms(void)
{
    if (atomRoot == NULL)
        return;
    FreeAtom(atomRoot);
    atomRoot = NULL;
    free(nodeTable);
    nodeTable = NULL;
    lastAtom = XKB_ATOM_NONE;
}
