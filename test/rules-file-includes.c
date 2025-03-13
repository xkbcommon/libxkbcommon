/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * Copyright © 2019 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include "test-config.h"

#include "xkbcommon/xkbcommon.h"
#include "test.h"
#include "utils.h"
#include "xkbcomp/rules.h"

struct test_data {
    /* Rules file */
    const char *rules;

    /* Input */
    const char *model;
    const char *layout;
    const char *variant;
    const char *options;

    /* Expected output */
    const char *keycodes;
    const char *types;
    const char *compat;
    const char *symbols;
    const char *geometry;

    /* Or set this if xkb_components_from_rules() should fail. */
    bool should_fail;
};

static bool
test_rules(struct xkb_context *ctx, struct test_data *data)
{
    fprintf(stderr, "\n\nChecking : %s\t%s\t%s\t%s\t%s\n", data->rules,
            data->model, data->layout, data->variant, data->options);

    if (data->should_fail)
        fprintf(stderr, "Expecting: FAILURE\n");
    else
        fprintf(stderr, "Expecting: %s\t%s\t%s\t%s\n",
                data->keycodes, data->types, data->compat, data->symbols);

    const struct xkb_rule_names rmlvo = {
        data->rules, data->model, data->layout, data->variant, data->options
    };
    struct xkb_component_names kccgst;
    if (xkb_components_names_from_rules(ctx, &rmlvo, NULL, &kccgst)) {
        fprintf(stderr, "Received : %s\t%s\t%s\t%s\n",
                kccgst.keycodes, kccgst.types, kccgst.compatibility,
                kccgst.symbols);
    } else {
        fprintf(stderr, "Received : FAILURE\n");
        return data->should_fail;
    }

    const bool passed = streq_not_null(kccgst.keycodes, data->keycodes) &&
                        streq_not_null(kccgst.types, data->types) &&
                        streq_not_null(kccgst.compatibility, data->compat) &&
                        streq_not_null(kccgst.symbols, data->symbols) &&
                        streq_null(kccgst.geometry, data->geometry);

    free(kccgst.keycodes);
    free(kccgst.types);
    free(kccgst.compatibility);
    free(kccgst.symbols);
    free(kccgst.geometry);

    return passed;
}

int
main(int argc, char *argv[])
{
    struct xkb_context *ctx;

    test_init();

    setenv("XKB_CONFIG_ROOT", TEST_XKB_CONFIG_ROOT, 1);

    ctx = test_get_context(0);
    assert(ctx);

    struct test_data test1 = {
        .rules = "inc-src-simple",

        .model = "my_model", .layout = "my_layout", .variant = "", .options = "",

        .keycodes = "my_keycodes", .types = "default_types",
        .compat = "default_compat", .symbols = "my_symbols",
    };
    assert(test_rules(ctx, &test1));

    struct test_data test2 = {
        .rules = "inc-src-nested",

        .model = "my_model", .layout = "my_layout", .variant = "", .options = "",

        .keycodes = "my_keycodes", .types = "default_types",
        .compat = "default_compat", .symbols = "my_symbols",
    };
    assert(test_rules(ctx, &test2));

    struct test_data test3 = {
        .rules = "inc-src-looped",

        .model = "my_model", .layout = "my_layout", .variant = "", .options = "",

        .should_fail = true,
    };
    assert(test_rules(ctx, &test3));

    struct test_data test4 = {
        .rules = "inc-src-before-after",

        .model = "before_model", .layout = "my_layout", .variant = "", .options = "",

        .keycodes = "my_keycodes", .types = "default_types",
        .compat = "default_compat", .symbols = "default_symbols",
    };
    assert(test_rules(ctx, &test4));

    struct test_data test5 = {
        .rules = "inc-src-options",

        .model = "my_model", .layout = "my_layout", .variant = "my_variant",
        .options = "option11,my_option,colon:opt,option111",

        .keycodes = "my_keycodes", .types = "default_types",
        .compat = "default_compat+substring+group(bla)|some:compat",
        .symbols = "my_symbols+extra_variant+altwin(menu)",
    };
    assert(test_rules(ctx, &test5));

    struct test_data test6 = {
        .rules = "inc-src-loop-twice",

        .model = "my_model", .layout = "my_layout", .variant = "", .options = "",

        .keycodes = "my_keycodes", .types = "default_types",
        .compat = "default_compat", .symbols = "my_symbols",
    };
    assert(test_rules(ctx, &test6));

    struct test_data test7 = {
      .rules = "inc-no-newline",
      .should_fail = true,
    };
    assert(test_rules(ctx, &test7));

    struct test_data test8 = {
        .rules = "inc-src-relative-path",

        .model = "my_model", .layout = "my_layout", .variant = "", .options = "",

        .keycodes = "my_keycodes", .types = "default_types",
        .compat = "default_compat", .symbols = "my_symbols",
    };
    assert(test_rules(ctx, &test8));

    xkb_context_unref(ctx);
    return 0;
}
