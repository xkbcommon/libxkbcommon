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

#include "atom.h"

#define INITIAL_TABLE_SIZE 100

struct atom_node {
    struct atom_node *left, *right;
    uint32_t a;
    unsigned int fingerprint;
    char *string;
};

struct atom_table {
    xkb_atom_t last_atom;
    struct atom_node *atom_root;
    size_t table_length;
    struct atom_node **node_table;
};

struct atom_table *
atom_table_new(void)
{
    struct atom_table *table;

    table = calloc(1, sizeof(*table));
    if (!table)
        return NULL;

    table->last_atom = XKB_ATOM_NONE;

    return table;
}

static void
free_atom(struct atom_node *patom)
{
    if (!patom)
        return;

    free_atom(patom->left);
    free_atom(patom->right);
    free(patom->string);
    free(patom);
}

void
atom_table_free(struct atom_table *table)
{
    if (!table)
        return;

    free_atom(table->atom_root);
    free(table->node_table);
    free(table);
}

const char *
atom_text(struct atom_table *table, xkb_atom_t atom)
{
    if (atom == XKB_ATOM_NONE || atom > table->last_atom ||
        !table->node_table[atom])
        return NULL;

    return table->node_table[atom]->string;
}

char *
atom_strdup(struct atom_table *table, xkb_atom_t atom)
{
    const char *ret = atom_text(table, atom);
    return ret ? strdup(ret) : NULL;
}

xkb_atom_t
atom_intern(struct atom_table *table, const char *string)
{
    struct atom_node **np;
    struct atom_node *nd;
    unsigned i;
    int comp;
    unsigned int fp = 0;
    size_t len;

    if (!string)
        return XKB_ATOM_NONE;

    len = strlen(string);
    np = &table->atom_root;
    for (i = 0; i < (len + 1) / 2; i++) {
        fp = fp * 27 + string[i];
        fp = fp * 27 + string[len - 1 - i];
    }

    while (*np) {
        if (fp < (*np)->fingerprint)
            np = &((*np)->left);
        else if (fp > (*np)->fingerprint)
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

    nd = malloc(sizeof(*nd));
    if (!nd)
        return XKB_ATOM_NONE;

    nd->string = malloc(len + 1);
    if (!nd->string) {
        free(nd);
        return XKB_ATOM_NONE;
    }
    strncpy(nd->string, string, len);
    nd->string[len] = 0;

    if ((table->last_atom + 1) >= table->table_length) {
        struct atom_node **new_node_table;
        int new_length;

        if (table->table_length == 0)
            new_length = INITIAL_TABLE_SIZE;
        else
            new_length = table->table_length * 2;

        new_node_table = realloc(table->node_table,
                                 new_length * sizeof(*new_node_table));
        if (!new_node_table) {
            if (nd->string != string)
                free(nd->string);
            free(nd);
            return XKB_ATOM_NONE;
        }
        new_node_table[XKB_ATOM_NONE] = NULL;

        table->table_length = new_length;
        table->node_table = new_node_table;
    }

    *np = nd;
    nd->left = nd->right = NULL;
    nd->fingerprint = fp;
    nd->a = (++table->last_atom);
    *(table->node_table + table->last_atom) = nd;

    return nd->a;
}
