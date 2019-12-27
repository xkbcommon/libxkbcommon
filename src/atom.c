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

#include "config.h"

#include "utils.h"
#include "atom.h"

/* FNV-1a (http://www.isthe.com/chongo/tech/comp/fnv/). */
static inline uint32_t
hash_buf(const char *string, size_t len)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < (len + 1) / 2; i++) {
        hash ^= (uint8_t) string[i];
        hash *= 0x01000193;
        hash ^= (uint8_t) string[len - 1 - i];
        hash *= 0x01000193;
    }
    return hash;
}

/*
 * The atom table is a insert-only unbalanced binary search tree
 * mapping strings to atoms.
 *
 * The tree nodes are kept contiguously in the `table` array.
 *
 * The atom value is the index of the tree node in the array.
 *
 * As an optimization, strings are not compared by value directly,
 *      s1 < s2
 * instead, they are compared by fingerprint (hash) and the value
 * is only used to resolve collisions:
 *      (fingerprint(s1), s1) < (fingerprint(s2), s2)
 * Fingerprint are pre-calculated and saved in the tree nodes.
 *
 * Why is this not just a hash table? Who knows!
 */
struct atom_node {
    xkb_atom_t left, right;
    uint32_t fingerprint;
    char *string;
};

struct atom_table {
    xkb_atom_t root;
    darray(struct atom_node) table;
};

struct atom_table *
atom_table_new(void)
{
    struct atom_table *table = calloc(1, sizeof(*table));
    if (!table)
        return NULL;

    darray_init(table->table);
    /* The original throw-away root is here, at the illegal atom 0. */
    darray_resize0(table->table, 1);

    return table;
}

void
atom_table_free(struct atom_table *table)
{
    if (!table)
        return;

    struct atom_node *node;
    darray_foreach(node, table->table)
        free(node->string);
    darray_free(table->table);
    free(table);
}

const char *
atom_text(struct atom_table *table, xkb_atom_t atom)
{
    assert(atom < darray_size(table->table));
    return darray_item(table->table, atom).string;
}

xkb_atom_t
atom_intern(struct atom_table *table, const char *string, size_t len, bool add)
{
    uint32_t fingerprint = hash_buf(string, len);

    xkb_atom_t *atomp = &table->root;
    while (*atomp != XKB_ATOM_NONE) {
        struct atom_node *node = &darray_item(table->table, *atomp);

        if (fingerprint > node->fingerprint) {
            atomp = &node->right;
        }
        else if (fingerprint < node->fingerprint) {
            atomp = &node->left;
        }
        else {
            /* Now start testing the strings. */
            const int cmp = strncmp(string, node->string, len);
            if (likely(cmp == 0 && node->string[len] == '\0')) {
                return *atomp;
            }
            else if (cmp > 0) {
                atomp = &node->right;
            }
            else {
                atomp = &node->left;
            }
        }
    }

    if (!add)
        return XKB_ATOM_NONE;

    struct atom_node node;
    node.string = strndup(string, len);
    assert(node.string != NULL);
    node.left = node.right = XKB_ATOM_NONE;
    node.fingerprint = fingerprint;
    xkb_atom_t atom = darray_size(table->table);
    /* Do this before the append, as it may realloc and change the offsets. */
    *atomp = atom;
    darray_append(table->table, node);
    return atom;
}
