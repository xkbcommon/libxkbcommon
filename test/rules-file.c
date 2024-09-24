/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
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
 */

#include "config.h"

#include "test.h"
#include "xkbcomp/xkbcomp-priv.h"
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

    /* Or set this if xkb_components_from_rules() should fail. */
    bool should_fail;
};

static bool
test_rules(struct xkb_context *ctx, struct test_data *data)
{
    bool passed;
    const struct xkb_rule_names rmlvo = {
        data->rules, data->model, data->layout, data->variant, data->options
    };
    struct xkb_component_names kccgst;

    fprintf(stderr, "\n\nChecking : %s\t%s\t%s\t%s\t%s\n", data->rules,
            data->model, data->layout, data->variant, data->options);

    if (data->should_fail)
        fprintf(stderr, "Expecting: FAILURE\n");
    else
        fprintf(stderr, "Expecting: %s\t%s\t%s\t%s\n",
                data->keycodes, data->types, data->compat, data->symbols);

    if (!xkb_components_from_rules(ctx, &rmlvo, &kccgst)) {
        fprintf(stderr, "Received : FAILURE\n");
        return data->should_fail;
    }

    fprintf(stderr, "Received : %s\t%s\t%s\t%s\n",
            kccgst.keycodes, kccgst.types, kccgst.compat, kccgst.symbols);

    passed = streq_not_null(kccgst.keycodes, data->keycodes) &&
             streq_not_null(kccgst.types, data->types) &&
             streq_not_null(kccgst.compat, data->compat) &&
             streq_not_null(kccgst.symbols, data->symbols);

    free(kccgst.keycodes);
    free(kccgst.types);
    free(kccgst.compat);
    free(kccgst.symbols);

    return passed;
}

int
main(int argc, char *argv[])
{
    struct xkb_context *ctx;

    test_init();

    ctx = test_get_context(0);
    assert(ctx);

    struct test_data test_utf_8_with_bom = {
        .rules = "utf-8_with_bom",

        .model = "my_model", .layout = "my_layout", .variant = "my_variant",
        .options = "my_option",

        .keycodes = "my_keycodes", .types = "my_types",
        .compat = "my_compat|some:compat",
        .symbols = "my_symbols+extra_variant",
    };
    assert(test_rules(ctx, &test_utf_8_with_bom));

    struct test_data test_utf_16le_with_bom = {
        .rules = "utf-16le_with_bom",

        .model = "my_model", .layout = "my_layout", .variant = "my_variant",
        .options = "my_option",

        .keycodes = "my_keycodes", .types = "my_types",
        .compat = "my_compat|some:compat",
        .symbols = "my_symbols+extra_variant",
    };
    assert(!test_rules(ctx, &test_utf_16le_with_bom));

    struct test_data test_utf_16be_with_bom = {
        .rules = "utf-16be_with_bom",

        .model = "my_model", .layout = "my_layout", .variant = "my_variant",
        .options = "my_option",

        .keycodes = "my_keycodes", .types = "my_types",
        .compat = "my_compat|some:compat",
        .symbols = "my_symbols+extra_variant",
    };
    assert(!test_rules(ctx, &test_utf_16be_with_bom));

    struct test_data test_utf_32be = {
        .rules = "utf-32be",

        .model = "my_model", .layout = "my_layout", .variant = "my_variant",
        .options = "my_option",

        .keycodes = "my_keycodes", .types = "my_types",
        .compat = "my_compat|some:compat",
        .symbols = "my_symbols+extra_variant",
    };
    assert(!test_rules(ctx, &test_utf_32be));

    struct test_data test1 = {
        .rules = "simple",

        .model = "my_model", .layout = "my_layout", .variant = "my_variant",
        .options = "my_option",

        .keycodes = "my_keycodes", .types = "my_types",
        .compat = "my_compat|some:compat",
        .symbols = "my_symbols+extra_variant",
    };
    assert(test_rules(ctx, &test1));

    struct test_data test2 = {
        .rules = "simple",

        .model = "", .layout = "foo", .variant = "", .options = "",

        .keycodes = "default_keycodes", .types = "default_types",
        .compat = "default_compat", .symbols = "default_symbols",
    };
    assert(test_rules(ctx, &test2));

    struct test_data test3 = {
        .rules = "groups",

        .model = "pc104", .layout = "foo", .variant = "", .options = "",

        .keycodes = "something(pc104)", .types = "default_types",
        .compat = "default_compat", .symbols = "default_symbols",
    };
    assert(test_rules(ctx, &test3));

    struct test_data test4 = {
        .rules = "groups",

        .model = "foo", .layout = "ar", .variant = "bar", .options = "",

        .keycodes = "default_keycodes", .types = "default_types",
        .compat = "default_compat", .symbols = "my_symbols+(bar)",
    };
    assert(test_rules(ctx, &test4));

    struct test_data test5 = {
        .rules = "simple",

        .model = NULL, .layout = "my_layout,second_layout", .variant = "my_variant",
        .options = "my_option",

        .should_fail = true
    };
    assert(test_rules(ctx, &test5));

    struct test_data test6 = {
        .rules = "index",

        .model = "", .layout = "br,al,cn,az", .variant = "",
        .options = "some:opt",

        .keycodes = "default_keycodes", .types = "default_types",
        .compat = "default_compat",
        .symbols = "default_symbols+extra:1+extra:2+extra:3+extra:4",
    };
    assert(test_rules(ctx, &test6));

    struct test_data test7 = {
        .rules = "multiple-options",

        .model = "my_model", .layout = "my_layout", .variant = "my_variant",
        .options = "option3,option1,colon:opt,option11",

        .keycodes = "my_keycodes", .types = "my_types",
        .compat = "my_compat+some:compat+group(bla)",
        .symbols = "my_symbols+extra_variant+compose(foo)+keypad(bar)+altwin(menu)",
    };
    assert(test_rules(ctx, &test7));

    /* Wild card does not match empty entries for layouts and variants */
#define ENTRY(_model, _layout, _variant, _options, _symbols, _should_fail) \
    { .rules = "wildcard", .model = _model,                                \
      .layout = _layout, .variant = _variant, .options = _options,         \
      .keycodes = "evdev", .types = "complete", .compat = "complete",      \
      .symbols = _symbols , .should_fail = _should_fail }
    struct test_data wildcard_data[] = {
        /* OK: empty model and options and at least one layout+variant combo */
        ENTRY(NULL, "a"  , "1"  , NULL, "pc+a(1)", false),
        ENTRY(""  , "a"  , "1"  , ""  , "pc+a(1)", false),
        ENTRY(""  , "a," , "1," , ""  , "pc+a(1)", false),
        ENTRY(""  , ",b" , ",2" , ""  , "+b(2):2", false),
        ENTRY(""  , "a,b", "1," , ""  , "pc+a(1)", false),
        ENTRY(""  , "a,b", ",2" , ""  , "+b(2):2", false),
        /* Fails: empty layout or variant */
        ENTRY(NULL, NULL , NULL , NULL, "", true),
        ENTRY(NULL, ""   , ""   , NULL, "", true),
        ENTRY(NULL, NULL , "1"  , NULL, "", true),
        ENTRY(NULL, ""   , "1"  , NULL, "", true),
        ENTRY(NULL, ","  , "1,2", NULL, "", true),
        ENTRY(NULL, "a"  , NULL , NULL, "", true),
        ENTRY(NULL, "a"  , ""   , NULL, "", true),
        ENTRY(NULL, "a,b", NULL , NULL, "", true),
        ENTRY(NULL, "a,b", ""   , NULL, "", true),
        ENTRY(NULL, "a,b", ","  , NULL, "", true)
    };
#undef ENTRY
    for (size_t k = 0; k < ARRAY_SIZE(wildcard_data); k++) {
        assert(test_rules(ctx, &wildcard_data[k]));
    }

    /* Prepare data with too much layouts */
    char too_much_layouts[(2 + MAX_LAYOUT_INDEX_STR_LENGTH) * (XKB_MAX_GROUPS + 1)] = { 0 };
    char too_much_symbols[(3 + MAX_LAYOUT_INDEX_STR_LENGTH) * XKB_MAX_GROUPS] = { 0 };
    size_t i = 0;
    for (xkb_layout_index_t l = 0; l <= XKB_MAX_GROUPS; l++) {
        i += snprintf(&too_much_layouts[i], sizeof(too_much_layouts) - i,
                      "x%"PRIu32",", l + 1);
    }
    too_much_layouts[--i] = '\0';
    i = 0;
    for (xkb_layout_index_t l = 0; l < XKB_MAX_GROUPS; l++) {
        i += snprintf(&too_much_symbols[i], sizeof(too_much_symbols) - i,
                      "x:%"PRIu32"+", l + 1);
    }
    too_much_symbols[--i] = '\0';

#define ENTRY2(_rules, _model, _layout, _variant, _options,                 \
               _keycodes, _types, _compat, _symbols, _should_fail)          \
    { .rules = _rules, .model = _model,                                     \
      .layout = _layout, .variant = _variant, .options = _options,          \
      .keycodes = _keycodes,                                                \
      .types = _types,                                                      \
      .compat = _compat,                                                    \
      .symbols = _symbols , .should_fail = _should_fail }
#define ENTRY(layout, variant, options, symbols, should_fail)                  \
        ENTRY2("special_indexes", NULL, layout, variant, options,              \
               "default_keycodes", "default_types", "default_compat", symbols, \
               should_fail)
    struct test_data special_indexes_first_data[] = {
        /* Test index ranges: layout vs layout[first] */
        ENTRY("layout_a", NULL, NULL, "symbols_A", false),
        ENTRY("layout_e", NULL, NULL, "symbols_E+layout_e", false),
        ENTRY("a", NULL, NULL, "a", false),
        ENTRY("a", "1", NULL, "a(1)", false),
        /* Test index ranges: invalid layout qualifier */
        ENTRY("layout_c", NULL, NULL, "symbols_C:1+symbols_z:1", false),
        /* Test index ranges: invalid layout[first] qualifier */
        ENTRY("layout_d", NULL, NULL, "symbols_D", false),
        /* Test index ranges: multiple layouts */
        ENTRY("a,b", NULL, NULL, "a+b:2", false),
        ENTRY("a,b", ",c", NULL, "a+b(c):2", false),
        ENTRY("layout_e,layout_a", NULL, NULL, "symbols_e:1+symbols_x:2", false),
        ENTRY("layout_a,layout_b,layout_c,layout_d", NULL, NULL,
              "symbols_a:1+symbols_y:2+layout_c:3+layout_d:4+symbols_z:3", false),
        ENTRY("layout_a,layout_b,layout_c,layout_d",
              "extra,,,extra", NULL,
              "symbols_a:1+symbols_y:2+layout_c:3+layout_d(extra):4+symbols_z:3"
              "+foo:1|bar:1+foo:4|bar:4", false),
        /* NOTE: 5 layouts is intentional;
         * will require update when raising XKB_MAX_LAYOUTS */
        ENTRY("layout_a,layout_b,layout_c,layout_d,layout_e", NULL, NULL,
              "symbols_a:1+symbols_y:2+layout_c:3+layout_d:4+symbols_z:3", false),
#undef ENTRY
        /* Test index ranges: too much layouts */
        ENTRY2("special_indexes-limit", NULL, too_much_layouts, NULL, NULL,
               "default_keycodes", "default_types", "default_compat", too_much_symbols, false),
#define ENTRY(model, layout, variant, options, compat, symbols, should_fail) \
        ENTRY2("evdev-modern", model, layout, variant, options,      \
               "evdev+aliases(qwerty)", "complete", compat, symbols, should_fail)
        /* evdev-modern: 1 layout */
        ENTRY("whatever", "ar", NULL, NULL, "complete", "pc+ara+inet(evdev)", false),
        ENTRY("whatever", "ben", "probhat", NULL, "complete", "pc+in(ben_probhat)+inet(evdev)", false),
        ENTRY("ataritt", "es", NULL, NULL, "complete", "xfree68_vndr/ataritt(us)+es+inet(evdev)", false),
        ENTRY("ataritt", "jp", NULL, NULL, "complete+japan", "xfree68_vndr/ataritt(us)+jp+inet(evdev)", false),
        ENTRY2("evdev-modern", "olpc", "us", NULL, NULL,
               "evdev+olpc(olpc)+aliases(qwerty)", "complete", "olpc", "olpc+us(olpc)+inet(evdev)", false),
        ENTRY2("evdev-modern", "olpc", "jp", NULL, NULL,
               "evdev+olpc(olpc)+aliases(qwerty)", "complete", "complete+japan", "olpc+jp+inet(evdev)", false),
        ENTRY("pc104", "jp", NULL, NULL, "complete+japan", "pc+jp+inet(evdev)", false),
        ENTRY("pc104", "jp", "xxx", NULL, "complete+japan", "pc+jp(xxx)+inet(evdev)", false),
        ENTRY("pc104", "es", NULL, NULL, "complete", "pc+es+inet(evdev)", false),
        ENTRY("pc104", "es", "xxx", NULL, "complete", "pc+es(xxx)+inet(evdev)", false),
        ENTRY2("evdev-modern", "pc104", "de", "neo", NULL,
               "evdev+aliases(qwertz)", "complete",
               "complete+caps(caps_lock):1+misc(assign_shift_left_action):1+level5(level5_lock):1",
               "pc+de(neo)+inet(evdev)", false),
        ENTRY("pc104", "br", NULL, "misc:typo,misc:apl", "complete",
              "pc+br+inet(evdev)+typo(base):1+apl(level3):1", false),
        /* evdev-modern: 2 layouts */
        ENTRY("whatever", "ar,pt", NULL, NULL, "complete", "pc+ara+pt:2+inet(evdev)", false),
        ENTRY("whatever", "pt,ar", NULL, NULL, "complete", "pc+pt+ara:2+inet(evdev)", false),
        ENTRY("whatever", "ben,gb", "probhat,", NULL, "complete",
              "pc+in(ben_probhat)+gb:2+inet(evdev)", false),
        ENTRY("whatever", "gb,ben", ",probhat", NULL, "complete",
              "pc+gb+in(ben):2+in(ben_probhat):2+inet(evdev)", false),
        ENTRY("whatever", "ben,ar", "probhat,", NULL, "complete",
              "pc+in(ben_probhat)+ara:2+inet(evdev)", false),
        ENTRY("ataritt", "jp,es", NULL, NULL, "complete", "pc+jp+es:2+inet(evdev)", false),
        ENTRY("ataritt", "es,jp", NULL, NULL, "complete", "pc+es+jp:2+inet(evdev)", false),
        ENTRY2("evdev-modern", "olpc", "jp,es", NULL, NULL,
               "evdev+olpc(olpc)+aliases(qwerty)", "complete", "complete", "pc+jp+es:2+inet(evdev)", false),
        ENTRY2("evdev-modern", "olpc", "es,jp", NULL, NULL,
               "evdev+olpc(olpc)+aliases(qwerty)", "complete", "complete", "pc+es+jp:2+inet(evdev)", false),
        ENTRY("pc104", "jp,es", NULL, NULL, "complete", "pc+jp+es:2+inet(evdev)", false),
        ENTRY("pc104", "jp,es", "xxx,yyy", NULL, "complete", "pc+jp(xxx)+es(yyy):2+inet(evdev)", false),
        ENTRY("pc104", "latin,jp", NULL, NULL, "complete", "pc+latin+jp:2+inet(evdev)", false),
        ENTRY("pc104", "latin,jp", "xxx,yyy", NULL, "complete", "pc+latin(xxx)+jp(yyy):2+inet(evdev)", false),
        ENTRY2("evdev-modern", "pc104", "gb,de", ",neo", NULL,
               "evdev+aliases(qwerty)", "complete",
               "complete+caps(caps_lock):2+misc(assign_shift_left_action):2+level5(level5_lock):2",
               "pc+gb+de(neo):2+inet(evdev)", false),
        ENTRY("pc104", "ca,br", NULL, "misc:typo,misc:apl", "complete",
              "pc+ca+br:2+inet(evdev)+typo(base):1+typo(base):2+apl(level3):1+apl(level3):2", false),
#undef ENTRY
    };
    for (size_t k = 0; k < ARRAY_SIZE(special_indexes_first_data); k++) {
        assert(test_rules(ctx, &special_indexes_first_data[k]));
    }

    xkb_context_unref(ctx);
    return 0;
}
