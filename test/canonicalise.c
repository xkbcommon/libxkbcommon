/*
 * Copyright © 2009 Daniel Stone
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "xkbcommon/xkbcommon.h"

struct test_data {
    struct xkb_component_names new;
    struct xkb_component_names old;
    int pass_old;
    const char *exp_keycodes;
    const char *exp_compat;
    const char *exp_symbols;
    const char *exp_types;
};

static struct test_data *
new_data(void)
{
    return calloc(1, sizeof(struct test_data));
}

static void
free_data(struct test_data *data)
{
    free(data->new.keycodes);
    free(data->new.compat);
    free(data->new.symbols);
    free(data->new.types);
    free(data->old.keycodes);
    free(data->old.compat);
    free(data->old.symbols);
    free(data->old.types);
    free(data);
}

static void
set_new(struct test_data *data, const char *keycodes, const char *compat,
        const char *symbols, const char *types)
{
    data->new.keycodes = strdup(keycodes);
    data->new.compat = strdup(compat);
    data->new.symbols = strdup(symbols);
    data->new.types = strdup(types);
}

static void
set_old(struct test_data *data, const char *keycodes, const char *compat,
        const char *symbols, const char *types)
{
    data->old.keycodes = strdup(keycodes);
    data->old.compat = strdup(compat);
    data->old.symbols = strdup(symbols);
    data->old.types = strdup(types);
    data->pass_old = 1;
}

static void
set_exp(struct test_data *data, const char *keycodes, const char *compat,
        const char *symbols, const char *types)
{
    data->exp_keycodes = keycodes;
    data->exp_compat = compat;
    data->exp_symbols = symbols;
    data->exp_types = types;
}

static int
test_canonicalise(struct test_data *data)
{
    fprintf(stderr, "New: %s %s %s %s\n", data->new.keycodes,
            data->new.compat, data->new.symbols, data->new.types);
    if (data->pass_old)
        fprintf(stderr, "Old: %s %s %s %s\n", data->old.keycodes,
                data->old.compat, data->old.symbols, data->old.types);
    fprintf(stderr, "Expected: %s %s %s %s\n", data->exp_keycodes,
            data->exp_compat, data->exp_symbols, data->exp_types);

    if (data->pass_old)
        xkb_canonicalise_components(&data->new, &data->old);
    else
        xkb_canonicalise_components(&data->new, NULL);

    fprintf(stderr, "Received: %s %s %s %s\n\n", data->new.keycodes,
            data->new.compat, data->new.symbols, data->new.types);

    return (strcmp(data->new.keycodes, data->exp_keycodes) == 0) &&
           (strcmp(data->new.compat, data->exp_compat) == 0) &&
           (strcmp(data->new.symbols, data->exp_symbols) == 0) &&
           (strcmp(data->new.types, data->exp_types) == 0);
}

int
main(void)
{
    struct test_data *twopart, *onepart;

    twopart = new_data();
    set_new(twopart, "+inet(pc104)",        "%+complete",     "pc(pc104)+%+ctrl(nocaps)",          "|complete");
    set_old(twopart, "xfree86",             "basic",          "us(dvorak)",                        "xfree86");
    set_exp(twopart, "xfree86+inet(pc104)", "basic+complete", "pc(pc104)+us(dvorak)+ctrl(nocaps)", "xfree86|complete");
    assert(test_canonicalise(twopart));
    free_data(twopart);

    onepart = new_data();
    set_new(onepart, "evdev", "complete", "pc(pc104)+us+compose(ralt)", "complete");
    set_exp(onepart, "evdev", "complete", "pc(pc104)+us+compose(ralt)", "complete");
    assert(test_canonicalise(onepart));
    free_data(onepart);

    return 0;
}
