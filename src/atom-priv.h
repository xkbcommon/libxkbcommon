#ifndef _ATOM_H_
#define _ATOM_H_

#include "atom.h"
#include "darray.h"

/*
 * The atom table is an insert-only linear probing hash table
 * mapping strings to atoms. Another array maps the atoms to
 * strings. The atom value is the position in the strings array.
 */
struct atom_table {
    xkb_atom_t *index;
    size_t index_size;
    darray(char *) strings;
};

#endif
