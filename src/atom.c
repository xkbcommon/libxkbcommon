/***********************************************************
 * Copyright 1987, 1998  The Open Group
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of The Open Group shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from The Open Group.
 *
 *
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 *                      All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Digital not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 ******************************************************************/

/************************************************************
 * Copyright 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ********************************************************/

#include "utils.h"
#include "atom.h"

struct atom_node {
    struct atom_node *left, *right;
    xkb_atom_t atom;
    unsigned int fingerprint;
    char *string;
};

struct atom_table {
    struct atom_node *root;
    darray(struct atom_node *) table;
};

struct atom_table *
atom_table_new(void)
{
    struct atom_table *table;

    table = calloc(1, sizeof(*table));
    if (!table)
        return NULL;

    darray_init(table->table);
    darray_growalloc(table->table, 100);
    darray_append(table->table, NULL);

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

    free_atom(table->root);
    darray_free(table->table);
    free(table);
}

const char *
atom_text(struct atom_table *table, xkb_atom_t atom)
{
    if (atom >= darray_size(table->table) ||
        darray_item(table->table, atom) == NULL)
        return NULL;

    return darray_item(table->table, atom)->string;
}

static bool
find_node_pointer(struct atom_table *table, const char *string, size_t len,
                  struct atom_node ***nodep_out, unsigned int *fingerprint_out)
{
    struct atom_node **nodep;
    unsigned int fingerprint = 0;
    bool found = false;

    nodep = &table->root;
    for (size_t i = 0; i < (len + 1) / 2; i++) {
        fingerprint = fingerprint * 27 + string[i];
        fingerprint = fingerprint * 27 + string[len - 1 - i];
    }

    while (*nodep) {
        if (fingerprint < (*nodep)->fingerprint) {
            nodep = &((*nodep)->left);
        }
        else if (fingerprint > (*nodep)->fingerprint) {
            nodep = &((*nodep)->right);
        }
        else {
            /* Now start testing the strings. */
            const int cmp = strncmp(string, (*nodep)->string, len);
            if (cmp < 0 || (cmp == 0 && len < strlen((*nodep)->string))) {
                nodep = &((*nodep)->left);
            }
            else if (cmp > 0) {
                nodep = &((*nodep)->right);
            }
            else {
                found = true;
                break;
            }
        }
    }

    *fingerprint_out = fingerprint;
    *nodep_out = nodep;
    return found;
}

xkb_atom_t
atom_lookup(struct atom_table *table, const char *string, size_t len)
{
    struct atom_node **nodep;
    unsigned int fingerprint;

    if (!string)
        return XKB_ATOM_NONE;

    if (!find_node_pointer(table, string, len, &nodep, &fingerprint))
        return XKB_ATOM_NONE;

    return (*nodep)->atom;
}

/*
 * If steal is true, we do not strdup @string; therefore it must be
 * dynamically allocated, NUL-terminated, not be free'd by the caller
 * and not be used afterwards. Use to avoid some redundant allocations.
 */
xkb_atom_t
atom_intern(struct atom_table *table, const char *string, size_t len,
            bool steal)
{
    struct atom_node **nodep;
    struct atom_node *node;
    unsigned int fingerprint;

    if (!string || len == 0)
        return XKB_ATOM_NONE;

    if (find_node_pointer(table, string, len, &nodep, &fingerprint)) {
        if (steal)
            free(UNCONSTIFY(string));
        return (*nodep)->atom;
    }

    node = malloc(sizeof(*node));
    if (!node)
        return XKB_ATOM_NONE;

    if (steal) {
        node->string = UNCONSTIFY(string);
    }
    else {
        node->string = strndup(string, len);
        if (!node->string) {
            free(node);
            return XKB_ATOM_NONE;
        }
    }

    *nodep = node;
    node->left = node->right = NULL;
    node->fingerprint = fingerprint;
    node->atom = darray_size(table->table);
    darray_append(table->table, node);

    return node->atom;
}
