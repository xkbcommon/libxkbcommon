/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <time.h>

#include "test.h"
#include "atom.h"

#define INTERN_LITERAL(table, literal) \
    atom_intern(table, literal, sizeof(literal) - 1, true)

#define LOOKUP_LITERAL(table, literal) \
    atom_intern(table, literal, sizeof(literal) - 1, false)

static void
random_string(char **str_out, size_t *len_out)
{
    /* Keep this small, so collisions might happen. */
    static const char random_chars[] = {
        'a', 'b', 'c', 'd', 'e', 'f', 'g'
    };

    size_t len;
    char *str;

    len = rand() % 15;
    str = malloc(len + 1);
    assert(str);

    for (size_t i = 0; i < len; i++)
        str[i] = random_chars[rand() % ARRAY_SIZE(random_chars)];
    /* Don't always terminate it; should work without. */
    if (rand() % 2 == 0)
        str[len] = '\0';

    *str_out = str;
    *len_out = len;
}

static void
test_random_strings(void)
{
    struct atom_string {
        xkb_atom_t atom;
        char *string;
        size_t len;
    };

    struct atom_table *table;
    struct atom_string *arr;
    int N;
    xkb_atom_t atom;
    const char *string;

    table = atom_table_new();
    assert(table);

    N = 1 + rand() % 100000;
    arr = calloc(N, sizeof(*arr));
    assert(arr);

    for (int i = 0; i < N; i++) {
        random_string(&arr[i].string, &arr[i].len);

        atom = atom_intern(table, arr[i].string, arr[i].len, false);
        if (atom != XKB_ATOM_NONE) {
            string = atom_text(table, atom);
            assert(string);

            if (arr[i].len != strlen(string) ||
                strncmp(string, arr[i].string, arr[i].len) != 0) {
                fprintf(stderr, "got a collision, but strings don't match!\n");
                fprintf(stderr, "existing length %zu, string %s\n",
                        strlen(string), string);
                fprintf(stderr, "new length %zu, string %.*s\n",
                        arr[i].len, (int) arr[i].len, arr[i].string);
                assert(false);
            }

            /* OK, got a real collision. */
            free(arr[i].string);
            i--;
            continue;
        }

        arr[i].atom = atom_intern(table, arr[i].string, arr[i].len, true);
        if (arr[i].atom == XKB_ATOM_NONE) {
            fprintf(stderr, "failed to intern! len: %zu, string: %.*s\n",
                    arr[i].len, (int) arr[i].len, arr[i].string);
            assert(false);
        }
    }

    for (int i = 0; i < N; i++) {
        string = atom_text(table, arr[i].atom);
        assert(string);

        if (arr[i].len != strlen(string) ||
            strncmp(string, arr[i].string, arr[i].len) != 0) {
            fprintf(stderr, "looked-up string doesn't match!\n");
            fprintf(stderr, "found length %zu, string %s\n",
                    strlen(string), string);
            fprintf(stderr, "expected length %zu, string %.*s\n",
                    arr[i].len, (int) arr[i].len, arr[i].string);

            /* Since this is random, we need to dump the failing data,
             * so we might have some chance to reproduce. */
            fprintf(stderr, "START dump of arr, N=%d\n", N);
            for (int j = 0; j < N; j++) {
                fprintf(stderr, "%u\t\t%zu\t\t%.*s\n", arr[i].atom,
                        arr[i].len, (int) arr[i].len, arr[i].string);
            }
            fprintf(stderr, "END\n");

            assert(false);
        }
    }

    for (int i = 0; i < N; i++)
        free(arr[i].string);
    free(arr);
    atom_table_free(table);
}

/* CLI positional arguments:
 * 1. Seed for the pseudo-random generator:
 *    - Leave it unset or set it to “-” to use current time.
 *    - Use an integer to set it explicitly.
 */
int
main(int argc, char *argv[])
{
    struct atom_table *table;
    xkb_atom_t atom1, atom2, atom3;

    test_init();

    /* Initialize pseudo-random generator with program arg or current time */
    unsigned int seed;
    if (argc >= 2 && !streq(argv[1], "-")) {
        seed = (unsigned int) atoi(argv[1]);
    } else {
        seed = (unsigned int) time(NULL);
    }
    fprintf(stderr, "Seed for the pseudo-random generator: %u\n", seed);
    srand(seed);

    table = atom_table_new();
    assert(table);

    assert(atom_text(table, XKB_ATOM_NONE) == NULL);
    assert(atom_intern(table, NULL, 0, false) == XKB_ATOM_NONE);

    atom1 = INTERN_LITERAL(table, "hello");
    assert(atom1 != XKB_ATOM_NONE);
    assert(atom1 == LOOKUP_LITERAL(table, "hello"));
    assert(streq(atom_text(table, atom1), "hello"));

    atom2 = atom_intern(table, "hello", 3, true);
    assert(atom2 != XKB_ATOM_NONE);
    assert(atom1 != atom2);
    assert(streq(atom_text(table, atom2), "hel"));
    assert(LOOKUP_LITERAL(table, "hel") == atom2);
    assert(LOOKUP_LITERAL(table, "hell") == XKB_ATOM_NONE);
    assert(LOOKUP_LITERAL(table, "hello") == atom1);

    atom3 = atom_intern(table, "", 0, true);
    assert(atom3 != XKB_ATOM_NONE);
    assert(LOOKUP_LITERAL(table, "") == atom3);

    atom_table_free(table);

    test_random_strings();

    return 0;
}
