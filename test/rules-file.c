/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "xkbcommon/xkbcommon.h"

#include "context.h"
#include "keymap.h"
#include "utils.h"
#include "xkbcomp/rules.h"
#include "test.h"

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
    xkb_layout_index_t explicit_layouts;

    /* Or set this if xkb_components_from_rules() should fail. */
    bool should_fail;
};

static bool
test_rules(struct xkb_context *ctx, const struct test_data *data)
{
    fprintf(stderr, "\n\nChecking : %s\t%s\t%s\t%s\t%s\n", data->rules,
            data->model, data->layout, data->variant, data->options);

    if (data->should_fail)
        fprintf(stderr, "Expecting: FAILURE\n");
    else
        fprintf(stderr, "Expecting: %s\t%s\t%s\t%s\t%s\t%u\n",
                data->keycodes, data->types, data->compat, data->symbols,
                data->geometry, data->explicit_layouts);

    bool passed = true;
    xkb_layout_index_t explicit_layouts = 0;
    for (int k = 0; k < 2; k++) {
        bool ok;
        const struct xkb_rule_names rmlvo = {
            data->rules, data->model, data->layout, data->variant, data->options
        };
        struct xkb_component_names kccgst;

        if (k == 0) {
            /* Private API */
            ok = xkb_components_from_rules_names(ctx, &rmlvo, &kccgst,
                                                 &explicit_layouts);
        } else {
            /* Public API */
            ok = xkb_components_names_from_rules(ctx, &rmlvo, NULL, &kccgst);
        }
        if (ok) {
            fprintf(stderr, "Received : %s\t%s\t%s\t%s\t%s\t%u\n",
                    kccgst.keycodes, kccgst.types, kccgst.compatibility,
                    kccgst.symbols, kccgst.geometry, explicit_layouts);
        } else {
            fprintf(stderr, "Received : FAILURE\n");
            return data->should_fail;
        }

        passed &= streq_not_null(kccgst.keycodes, data->keycodes) &&
                  streq_not_null(kccgst.types, data->types) &&
                  streq_not_null(kccgst.compatibility, data->compat) &&
                  streq_not_null(kccgst.symbols, data->symbols) &&
                  streq_null(kccgst.geometry, data->geometry) &&
                  explicit_layouts == data->explicit_layouts;

        free(kccgst.keycodes);
        free(kccgst.types);
        free(kccgst.compatibility);
        free(kccgst.symbols);
        free(kccgst.geometry);
    }

    return passed;
}

static void
test_encodings(struct xkb_context *ctx)
{
    static const struct test_data tests[] = {
        {
            .rules = "utf-8_with_bom",

            .model = "my_model", .layout = "my_layout", .variant = "my_variant",
            .options = "my_option",

            .keycodes = "my_keycodes", .types = "my_types",
            .compat = "my_compat|some:compat",
            .symbols = "my_symbols+extra_variant",
            .explicit_layouts = 1,
        },
        {
            .rules = "utf-16le_with_bom",

            .model = "my_model", .layout = "my_layout", .variant = "my_variant",
            .options = "my_option",

            .keycodes = "my_keycodes", .types = "my_types",
            .compat = "my_compat|some:compat",
            .symbols = "my_symbols+extra_variant",
            .explicit_layouts = 1,
            .should_fail = true,
        },
        {
            .rules = "utf-16be_with_bom",

            .model = "my_model", .layout = "my_layout", .variant = "my_variant",
            .options = "my_option",

            .keycodes = "my_keycodes", .types = "my_types",
            .compat = "my_compat|some:compat",
            .symbols = "my_symbols+extra_variant",
            .explicit_layouts = 1,
            .should_fail = true,
        },
        {
            .rules = "utf-32be",

            .model = "my_model", .layout = "my_layout", .variant = "my_variant",
            .options = "my_option",

            .keycodes = "my_keycodes", .types = "my_types",
            .compat = "my_compat|some:compat",
            .symbols = "my_symbols+extra_variant",
            .explicit_layouts = 1,
            .should_fail = true,
        }
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_rules(ctx, &tests[k]));
    }
}

/* Only parse strict decimal groups */
static void
test_strict_decimal_groups(struct xkb_context *ctx)
{
    static const struct test_data tests[] = {
        {
            .rules = "invalid-group-index",

            .model = "my_model", .layout = "1,2", .variant = NULL,
            .options = NULL,

            .keycodes = "default_keycodes", .types = "default_types",
            .compat = "default_compat",
            .symbols = "default_symbols+default_symbols:2",
            .explicit_layouts = 2,
            .should_fail = true,
        },
        {
            .rules = "invalid-group-qualifier",

            .model = "my_model", .layout = "1,2", .variant = NULL,
            .options = NULL,

            .keycodes = "default_keycodes", .types = "default_types",
            .compat = "default_compat",
            .symbols = "default_symbols+default_symbols:+2",
            .explicit_layouts = 1,
        }
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_rules(ctx, &tests[k]));
    }
}

static void
test_simple(struct xkb_context *ctx)
{
    static const struct test_data tests[] = {
        {
            .rules = "simple",

            .model = "my_model", .layout = "my_layout", .variant = "my_variant",
            .options = "my_option",

            .keycodes = "my_keycodes", .types = "my_types",
            .compat = "my_compat|some:compat",
            .symbols = "my_symbols+extra_variant",
            .explicit_layouts = 1,
        },
        {
            .rules = "simple",

            .model = "", .layout = "foo", .variant = "", .options = "",

            .keycodes = "default_keycodes", .types = "default_types",
            .compat = "default_compat", .symbols = "default_symbols",
            .explicit_layouts = 1,
        },
        {
            .rules = "groups",

            .model = "pc104", .layout = "foo", .variant = "", .options = "",

            .keycodes = "something(pc104)", .types = "default_types",
            .compat = "default_compat", .symbols = "default_symbols",
            .explicit_layouts = 1,
        },
        {
            .rules = "groups",

            .model = "foo", .layout = "ar", .variant = "bar", .options = "",

            .keycodes = "default_keycodes", .types = "default_types",
            .compat = "default_compat", .symbols = "my_symbols+(bar)",
            .explicit_layouts = 1,
        },
        {
            .rules = "simple",

            .model = NULL, .layout = "my_layout,second_layout", .variant = "my_variant",
            .options = "my_option",

            .should_fail = true
        },
        {
            .rules = "index",

            .model = "", .layout = "br,al,cn,az", .variant = "",
            .options = "some:opt",

            .keycodes = "default_keycodes", .types = "default_types",
            .compat = "default_compat",
            .symbols = "default_symbols+extra:1+extra:2+extra:3+extra:4",
            .explicit_layouts = 4,
        },
        {
            .rules = "multiple-options",

            .model = "my_model", .layout = "my_layout", .variant = "my_variant",
            .options = "option3,option1,colon:opt,option11",

            .keycodes = "my_keycodes", .types = "my_types",
            .compat = "my_compat+some:compat+group(bla)",
            .symbols = "my_symbols+extra_variant+compose(foo)+keypad(bar)+altwin(menu)",
            .explicit_layouts = 1,
        },
        /* “replace” merge mode */
        {
            .rules = "merge-mode-replace",

            .model = "my_model", .layout = "us,de", .variant = "",
            .options = "replace:first",

            .keycodes = "evdev", .types = "complete",
            .compat = "complete",
            .symbols = "pc+us+de:2^level3(ralt_alt)|empty",
            .explicit_layouts = 2,
        }
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_rules(ctx, &tests[k]));
    }
}

static void
test_wild_card(struct xkb_context *ctx)
{
    /* Wild card does not match empty entries for layouts and variants */
#define ENTRY(_model, _layout, _variant, _options, _symbols, _layouts, _fail) \
    { .rules = "wildcard", .model = (_model),                                 \
      .layout = (_layout), .variant = (_variant), .options = (_options),      \
      .keycodes = "evdev", .types = "complete", .compat = "complete",         \
      .symbols = (_symbols) , .geometry = "pc(pc104)",                        \
      .explicit_layouts = (_layouts), .should_fail = (_fail) }
    static const struct test_data tests[] = {
        /* OK: empty model and options and at least one layout+variant combo */
        ENTRY(NULL, "a"  , "1"  , NULL, "pc+a(1)", 1, false),
        ENTRY(""  , "a"  , "1"  , ""  , "pc+a(1)", 1, false),
        ENTRY(""  , "a," , "1," , ""  , "pc+a(1)", 1, false),
        ENTRY(""  , ",b" , ",2" , ""  , "+b(2):2", 2, false),
        ENTRY(""  , "a,b", "1," , ""  , "pc+a(1)", 1, false),
        ENTRY(""  , "a,b", ",2" , ""  , "+b(2):2", 2, false),
        /* Fails: empty layout or variant */
        ENTRY(NULL, NULL , NULL , NULL, "", 1, true),
        ENTRY(NULL, ""   , ""   , NULL, "", 1, true),
        ENTRY(NULL, NULL , "1"  , NULL, "", 1, true),
        ENTRY(NULL, ""   , "1"  , NULL, "", 1, true),
        ENTRY(NULL, ","  , "1,2", NULL, "", 2, true),
        ENTRY(NULL, "a"  , NULL , NULL, "", 1, true),
        ENTRY(NULL, "a"  , ""   , NULL, "", 1, true),
        ENTRY(NULL, "a,b", NULL , NULL, "", 2, true),
        ENTRY(NULL, "a,b", ""   , NULL, "", 2, true),
        ENTRY(NULL, "a,b", ","  , NULL, "", 2, true)
    };
#undef ENTRY

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_rules(ctx, &tests[k]));
    }

}

/* Test extended wild cards: <none>, <some> and <any> */
static void
test_extended_wilcards(struct xkb_context *ctx)
{
#define ENTRY(_rules, _layout, _variant, _symbols, _layouts, _fail)   \
    { .rules = (_rules), .model = NULL,                               \
      .layout = (_layout), .variant = (_variant), .options = NULL,    \
      .keycodes = "evdev", .types = "complete", .compat = "complete", \
      .symbols = (_symbols) , .explicit_layouts = (_layouts),         \
      .should_fail = (_fail) }

    static const struct test_data tests[] = {
        ENTRY("extended-wild-cards", "l1", NULL, "pc+l10:1", 1, false),
        ENTRY("extended-wild-cards", "l1", "v1", "pc+l20:1", 1, false),
        ENTRY("extended-wild-cards", "l1", "v2", "pc+l30(v2):1", 1, false),
        /* legacy wild card * does not catch empty variant */
        ENTRY("extended-wild-cards", "l2", NULL, "pc+l2:1", 1, false),
        ENTRY("extended-wild-cards", "l2", "v1", "pc+l40(v1):1", 1, false),
        ENTRY("extended-wild-cards", "l2", "v2", "pc+l40(v2):1", 1, false),
        ENTRY("extended-wild-cards", "l3", NULL, "pc+l50:1", 1, false),
        ENTRY("extended-wild-cards", "l3", "v1", "pc+l50(v1):1", 1, false),
        ENTRY("extended-wild-cards", "l3", "v2", "pc+l50(v2):1", 1, false),
        /* ? wild card does catch empty variant */
        ENTRY("extended-wild-cards", "l4", NULL, "pc+l4:1", 1, false),
        ENTRY("extended-wild-cards", "l4", "v1", "pc+l4(v1):1", 1, false),
        ENTRY("extended-wild-cards", "l4", "v2", "pc+l4(v20):1", 1, false),
        ENTRY("extended-wild-cards", "l1,l1,l1,l2", ",v1,v2,",
              "pc+l10:1+l20:2+l30(v2):3+l2:4", 4, false),
        ENTRY("extended-wild-cards", "l2,l2,l3,l3", "v1,v2,,v1",
              "pc+l40(v1):1+l40(v2):2+l50:3+l50(v1):4", 4, false),
        ENTRY("extended-wild-cards", "l3,l4,l4,l4", "v2,,v1,v2",
              "pc+l50(v2):1+l4:2+l4(v1):3+l4(v20):4", 4, false),
    };
#undef ENTRY

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_rules(ctx, &tests[k]));
    }
}

static void
test_layout_index_ranges(struct xkb_context *ctx, const char *too_much_layouts,
                         const char *too_much_symbols)
{
#define ENTRY2(_rules, _model, _layout, _variant, _options,                 \
               _keycodes, _types, _compat, _symbols, _count, _should_fail)  \
    { .rules = (_rules), .model = (_model),                                 \
      .layout = (_layout), .variant = (_variant), .options = (_options),    \
      .keycodes = (_keycodes), .types = (_types), .compat = (_compat),      \
      .symbols = (_symbols),                                                \
      .geometry = (strncmp((_rules), "evdev", 5) == 0) ? "pc(pc104)" : NULL,\
      .explicit_layouts = (_count), .should_fail = (_should_fail) }
#define ENTRY(layout, variant, options, symbols, count, should_fail)           \
        ENTRY2("special_indices", NULL, layout, variant, options,              \
               "default_keycodes", "default_types", "default_compat", symbols, \
               count, should_fail)

    const struct test_data tests[] = {
        /* Test index ranges: layout vs layout[first] */
        ENTRY("layout_a", NULL, NULL, "A", 1, false),
        ENTRY("layout_e", NULL, NULL, "E+layout_e", 1, false),
        ENTRY("a", NULL, NULL, "a", 1, false),
        ENTRY("a", "1", NULL, "a(1)", 1, false),
        /* Test index ranges: invalid layout qualifier */
        ENTRY("layout_c", NULL, NULL, "C:1+z:1", 1, false),
        /* Test index ranges: invalid layout[first] qualifier */
        ENTRY("layout_d", NULL, NULL, "D", 1, false),
        /* Test index ranges: multiple layouts */
        ENTRY("a,b", NULL, NULL, "a+b:2", 2, false),
        ENTRY("a,b", ",c", NULL, "a+b(c):2", 2, false),
        ENTRY("layout_e,layout_a", NULL, NULL, "e:1+x:2", 2, false),
        ENTRY("layout_a,layout_b,layout_c,layout_d", NULL, NULL,
              "a:1+y:2+layout_c:3+layout_d:4+z:3", 4, false),
        ENTRY("layout_a,layout_b,layout_c,layout_d",
              "extra,,,extra", NULL,
              "a:1+y:2+layout_c:3+layout_d(extra):4+z:3"
              "+foo:1|bar:1+foo:4|bar:4", 4, false),
        ENTRY("layout_a,layout_b,layout_c,layout_d,layout_e", NULL, NULL,
              "a:1+y:2+layout_c:3+layout_d:4+layout_e:5+z:3", 5, false),
        /* Check that special indices merge the KcCGST values in the expected order */
        ENTRY("layout_a,layout_b,layout_c", NULL, "option_3,option_2,option_1",
              "a:1+y:2+layout_c:3+z:3+III:2+JJJ:2+HHH:3+KKK:3+LLL+OOO:2+MMM:3+NNN:3",
              3, false),
#undef ENTRY
        /* Test index ranges: too much layouts */
        ENTRY2("special_indices-limit", NULL, too_much_layouts, NULL, NULL,
               "default_keycodes", "default_types", "default_compat", too_much_symbols,
               XKB_MAX_GROUPS, false),
#define ENTRY(model, layout, variant, options, compat, symbols, count, should_fail) \
        ENTRY2("evdev-modern", model, layout, variant, options,                     \
               "evdev+aliases(qwerty)", "complete", compat, symbols, count, should_fail)
        /* evdev-modern: 1 layout */
        ENTRY("whatever", "ar", NULL, NULL, "complete", "pc+ara+inet(evdev)", 1, false),
        ENTRY("whatever", "ben", "probhat", NULL, "complete", "pc+in(ben_probhat)+inet(evdev)", 1, false),
        ENTRY("ataritt", "es", NULL, NULL, "complete", "xfree68_vndr/ataritt(us)+es+inet(evdev)", 1, false),
        ENTRY("ataritt", "jp", NULL, NULL, "complete+japan", "xfree68_vndr/ataritt(us)+jp+inet(evdev)", 1, false),
        ENTRY2("evdev-modern", "olpc", "us", NULL, NULL,
               "evdev+olpc(olpc)+aliases(qwerty)", "complete", "olpc", "olpc+us(olpc)+inet(evdev)", 1, false),
        ENTRY2("evdev-modern", "olpc", "jp", NULL, NULL,
               "evdev+olpc(olpc)+aliases(qwerty)", "complete", "complete+japan", "olpc+jp+inet(evdev)", 1, false),
        ENTRY("pc104", "jp", NULL, NULL, "complete+japan", "pc+jp+inet(evdev)", 1, false),
        ENTRY("pc104", "jp", "xxx", NULL, "complete+japan", "pc+jp(xxx)+inet(evdev)", 1, false),
        ENTRY("pc104", "es", NULL, NULL, "complete", "pc+es+inet(evdev)", 1, false),
        ENTRY("pc104", "es", "xxx", NULL, "complete", "pc+es(xxx)+inet(evdev)", 1, false),
        ENTRY2("evdev-modern", "pc104", "de", "neo", NULL,
               "evdev+aliases(qwertz)", "complete",
               "complete+caps(caps_lock):1+misc(assign_shift_left_action):1+level5(level5_lock):1",
               "pc+de(neo)+inet(evdev)", 1, false),
        ENTRY("pc104", "br", NULL, "misc:typo,misc:apl", "complete",
              "pc+br+inet(evdev)+apl(level3):1+typo(base):1", 1, false),
        /* evdev-modern: 2 layouts */
        ENTRY("whatever", "ar,pt", NULL, NULL, "complete", "pc+ara+pt:2+inet(evdev)", 2, false),
        ENTRY("whatever", "pt,ar", NULL, NULL, "complete", "pc+pt+ara:2+inet(evdev)", 2, false),
        ENTRY("whatever", "ben,gb", "probhat,", NULL, "complete",
              "pc+in(ben_probhat)+gb:2+inet(evdev)", 2, false),
        ENTRY("whatever", "gb,ben", ",probhat", NULL, "complete",
              "pc+gb+in(ben):2+in(ben_probhat):2+inet(evdev)", 2, false),
        ENTRY("whatever", "ben,ar", "probhat,", NULL, "complete",
              "pc+in(ben_probhat)+ara:2+inet(evdev)", 2, false),
        ENTRY("ataritt", "jp,es", NULL, NULL, "complete", "pc+jp+es:2+inet(evdev)", 2, false),
        ENTRY("ataritt", "es,jp", NULL, NULL, "complete", "pc+es+jp:2+inet(evdev)", 2, false),
        ENTRY2("evdev-modern", "olpc", "jp,es", NULL, NULL,
               "evdev+olpc(olpc)+aliases(qwerty)", "complete", "complete", "pc+jp+es:2+inet(evdev)", 2, false),
        ENTRY2("evdev-modern", "olpc", "es,jp", NULL, NULL,
               "evdev+olpc(olpc)+aliases(qwerty)", "complete", "complete", "pc+es+jp:2+inet(evdev)", 2, false),
        ENTRY("pc104", "jp,es", NULL, NULL, "complete", "pc+jp+es:2+inet(evdev)", 2, false),
        ENTRY("pc104", "jp,es", "xxx,yyy", NULL, "complete", "pc+jp(xxx)+es(yyy):2+inet(evdev)", 2, false),
        ENTRY("pc104", "latin,jp", NULL, NULL, "complete", "pc+latin+jp:2+inet(evdev)", 2, false),
        ENTRY("pc104", "latin,jp", "xxx,yyy", NULL, "complete", "pc+latin(xxx)+jp(yyy):2+inet(evdev)", 2, false),
        ENTRY2("evdev-modern", "pc104", "gb,de", ",neo", NULL,
               "evdev+aliases(qwerty)", "complete",
               "complete+caps(caps_lock):2+misc(assign_shift_left_action):2+level5(level5_lock):2",
               "pc+gb+de(neo):2+inet(evdev)", 2, false),
        ENTRY("pc104", "ca,br", NULL, "misc:typo,misc:apl", "complete",
              "pc+ca+br:2+inet(evdev)+apl(level3):1+apl(level3):2+typo(base):1+typo(base):2", 2, false),
    };
#undef ENTRY

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_rules(ctx, &tests[k]));
    }
}

static void
test_extended_layout_indices(struct xkb_context *ctx)
{
    char layouts[(2 + MAX_LAYOUT_INDEX_STR_LENGTH) * XKB_MAX_GROUPS] = { 0 };
    size_t count = 0;
    for (xkb_layout_index_t l = 0; l < XKB_MAX_GROUPS; l++) {
        count += snprintf(&layouts[count], sizeof(layouts) - count,
                          "x%"PRIu32",", l + 1);
    }
    layouts[--count] = '\0';

    static const char symbols_prefix[] = "pc+x1+";
    static const char symbols_suffix[] = "+inet(evdev)";
    char symbols[
        (3 + 2 * MAX_LAYOUT_INDEX_STR_LENGTH) * XKB_MAX_GROUPS +
        sizeof(symbols_prefix) + sizeof(symbols_suffix)
    ] = { 0 };
    memcpy(symbols, symbols_prefix, sizeof(symbols_prefix));
    count = sizeof(symbols_prefix) - 1;
    for (xkb_layout_index_t l = 1; l < XKB_MAX_GROUPS; l++) {
        count += snprintf(&symbols[count], sizeof(symbols) - count,
                          "x%"PRIu32":%"PRIu32"+", l + 1, l + 1);
    }
    memcpy(symbols + count - 1, symbols_suffix, sizeof(symbols_suffix));

    const struct test_data test = {
        .rules = "evdev-modern",

        .model = "pc104",
        .layout = layouts, .variant = "", .options = "",

        .keycodes = "evdev+aliases(qwerty)",
        .compat = "complete",
        .types = "complete",
        .symbols = symbols,
        .geometry = "pc(pc104)",

        .explicit_layouts = XKB_MAX_GROUPS
    };
    assert(test_rules(ctx, &test));
}

static void
test_all_qualifier(struct xkb_context *ctx,
                   const char *too_much_layouts, const char *too_much_symbols)
{
    const struct test_data tests[] = {
        /* Test :all qualifier without special indices, with option */
        {
            .rules = "all_qualifier",

            .model = "my_model",
            .layout = "layout_a,layout_b,layout_a,layout_b,layout_c",
            .variant = "",
            .options = "my_option",

            .keycodes = "my_keycodes", .types = "my_types",
            .compat = "my_compat",
            .symbols = "symbols_a:1+symbols_b:2+symbols_a:3+symbols_b:4+symbols_c:5"
                       "+extra_option:1+extra_option:2+extra_option:3+extra_option:4"
                       "+extra_option:5",
            .explicit_layouts = 5,
        },
        /* Test :all qualifier without special indices, base for all layout */
        {
            .rules = "all_qualifier",

            .model = "my_model",
            .layout = "layout_x,layout_a,layout_b,layout_c,layout_d",
            .variant = "",
            .options = "",

            .keycodes = "my_keycodes", .types = "my_types",
            .compat = "my_compat",
            .symbols = "base:1+base:2+base:3+base:4+base:5"
                       "+symbols_a:2+symbols_b:3+default_symbols:4+default_symbols:5",
            .explicit_layouts = 5,
        },
        /* Test :all qualifier without special indices, with option, too much layouts */
        {
            .rules = "all_qualifier-limit",

            .model = "my_model",
            .layout = too_much_layouts,
            .variant = "",
            .options = "",

            .keycodes = "default_keycodes", .types = "default_types",
            .compat = "default_compat",
            .symbols = too_much_symbols,
            .explicit_layouts = XKB_MAX_GROUPS,
        },
        /* Test :all qualifier with special indices */
        {
            .rules = "all_qualifier",

            .model = "my_model",
            .layout = "layout_a,layout_b,layout_a,layout_b,layout_c",
            .variant = "extra1,,,,",
            .options = "my_option",

            .keycodes = "my_keycodes", .types = "my_types",
            .compat = "my_compat",
            .symbols = "symbols_a:1+symbols_b:2+symbols_a:3+symbols_b:4+symbols_c:5"
                       "+extra_symbols:1+extra_symbols:2+extra_symbols:3+extra_symbols:4+extra_symbols:5"
                       "+extra_option:1+extra_option:2+extra_option:3+extra_option:4+extra_option:5",
            .explicit_layouts = 5,
        },
        /* Test :all qualifier with special indices
         * It uses :all combined with layout[any], which is valid but
         * :%i was probably the intended qualified, so raises warning */
        {
            .rules = "all_qualifier",

            .model = "my_model",
            .layout = "layout_a,layout_b,layout_a,layout_b,layout_c",
            .variant = "extra2,,extra3,,",
            .options = "my_option",

            .keycodes = "my_keycodes", .types = "my_types",
            .compat = "my_compat",
            .symbols = "symbols_a:1+symbols_b:2+symbols_a:3+symbols_b:4+symbols_c:5"
                       "+extra_symbols1:1+extra_symbols2:1+extra_symbols2:2+extra_symbols2:3+extra_symbols2:4+extra_symbols2:5"
                       "+extra_symbols2:1+extra_symbols2:2+extra_symbols2:3+extra_symbols2:4+extra_symbols2:5"
                       "+extra_symbols1:3"
                       "+extra_option:1"
                       "+extra_option:2+extra_option:3+extra_option:4+extra_option:5",
            .explicit_layouts = 5,
        }
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_rules(ctx, &tests[k]));
    }
}

static void
test_layout_specific_options(struct xkb_context *ctx)
{
    const struct test_data tests[] = {
        /*
         * 1 layout, no layout index
         */
        {
            .rules = "layout-specific-options",
            .model = "pc104", .layout = "l1", .variant = NULL,
            .options = "opt1,opt2,opt3,opt4,opt5,opt6,opt7",
            .keycodes = "evdev", .compat = "complete", .types = "complete",
            .symbols = "pc+l1:1+s1:1+s3:1+s7",
            .explicit_layouts = 1
        },
        {
            .rules = "layout-specific-options",
            .model = "pc104", .layout = "l2", .variant = NULL,
            .options = "opt1,opt2,opt3,opt4,opt5,opt6,opt7",
            .keycodes = "evdev", .compat = "complete", .types = "complete",
            .symbols = "pc+l2:1+s1:1+s2:1+s3:1+s4:1+s7",
            .explicit_layouts = 1
        },
        /*
         * 1 layout, invalid layout index
         */
        {
            .rules = "layout-specific-options",
            .model = "pc104", .layout = "l1", .variant = NULL,
            .options = "opt1!,opt2!1x,opt3!x,opt4!x1,opt5!!,opt6!+,opt7!|",
            .keycodes = "evdev", .compat = "complete", .types = "complete",
            .symbols = "pc+l1:1+s1:1+s3:1+s7",
            .explicit_layouts = 1
        },
        /*
         * 1 layout, matching layout index
         */
        {
            .rules = "layout-specific-options",
            .model = "pc104", .layout = "l1", .variant = NULL,
            .options = "opt1!1,opt2!1,opt3!1,opt4!1,opt5!1,opt6!1,opt7!1",
            .keycodes = "evdev", .compat = "complete", .types = "complete",
            .symbols = "pc+l1:1+s1:1+s3:1",
            .explicit_layouts = 1
        },
        {
            .rules = "layout-specific-options",
            .model = "pc104", .layout = "l2", .variant = NULL,
            .options = "opt1!1,opt2!1,opt3!1,opt4!1,opt5!1,opt6!1,opt7!1",
            .keycodes = "evdev", .compat = "complete", .types = "complete",
            .symbols = "pc+l2:1+s1:1+s2:1+s3:1+s4:1",
            .explicit_layouts = 1
        },
        /*
         * 1 layout, non-matching layout index
         */
        {
            .rules = "layout-specific-options",
            .model = "pc104", .layout = "l1", .variant = NULL,
            .options = "opt1!2,opt2!2,opt3!2,opt4!2,opt5!2,opt6!2,opt7!2",
            .keycodes = "evdev", .compat = "complete", .types = "complete",
            .symbols = "pc+l1:1",
            .explicit_layouts = 1
        },
        {
            .rules = "layout-specific-options",
            .model = "pc104", .layout = "l2", .variant = NULL,
            .options = "opt1!2,opt2!2,opt3!2,opt4!2,opt5!2,opt6!2,opt7!2",
            .keycodes = "evdev", .compat = "complete", .types = "complete",
            .symbols = "pc+l2:1",
            .explicit_layouts = 1
        },
        /*
         * 2 layouts: no specifier
         */
        {
            .rules = "layout-specific-options",
            .model = "pc104", .layout = "l1,l2", .variant = NULL,
            .options = "opt1,opt2,opt3,opt4,opt5,opt6,opt7",
            .keycodes = "evdev", .compat = "complete", .types = "complete",
            .symbols = "pc+l1:1+l2:2+s1:1+s3:1+s3:2+s4:2+s5:1+s6:2+s7",
            .explicit_layouts = 2
        },
        /* 2 layout: non-matching index */
        {
            .rules = "layout-specific-options",
            .model = "pc104", .layout = "l1,l2", .variant = NULL,
            .options = "opt1!3,opt2!3,opt3!3,opt4!3,opt5!3,opt6!3,opt7!3",
            .keycodes = "evdev", .compat = "complete", .types = "complete",
            .symbols = "pc+l1:1+l2:2",
            .explicit_layouts = 2
        },
        /*
         * 2 layouts: specify only 1 layout for each option
         */
        {
            .rules = "layout-specific-options",
            .model = "pc104", .layout = "l1,l2", .variant = NULL,
            .options = "opt1!1,opt2!1,opt3!1,opt4!1,opt5!1,opt6!1,opt7!1",
            .keycodes = "evdev", .compat = "complete", .types = "complete",
            .symbols = "pc+l1:1+l2:2+s1:1+s3:1+s5:1",
            .explicit_layouts = 2
        },
        {
            .rules = "layout-specific-options",
            .model = "pc104", .layout = "l1,l2", .variant = NULL,
            .options = "opt1!2,opt2!2,opt3!2,opt4!2,opt5!2,opt6!2,opt7!2",
            .keycodes = "evdev", .compat = "complete", .types = "complete",
            .symbols = "pc+l1:1+l2:2+s3:2+s4:2+s6:2",
            .explicit_layouts = 2
        },
        /*
         * 2 layouts: specify 2 layouts for each option
         */
        {
            .rules = "layout-specific-options",
            .model = "pc104", .layout = "l1,l2", .variant = NULL,
            .options = "opt1!1,opt1!2,opt2!1,opt2!2,opt3!1,opt3!2,"
                       "opt4!1,opt4!2,opt5!1,opt5!2,opt6!1,opt6!2,opt7!1,opt7!2",
            .keycodes = "evdev", .compat = "complete", .types = "complete",
            .symbols = "pc+l1:1+l2:2+s1:1+s3:1+s3:2+s4:2+s5:1+s6:2",
            .explicit_layouts = 2
        },
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(tests); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        assert(test_rules(ctx, &tests[k]));
    }
}

int
main(int argc, char *argv[])
{
    struct xkb_context *ctx;

    test_init();

    ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    test_encodings(ctx);
    test_strict_decimal_groups(ctx);
    test_simple(ctx);
    test_wild_card(ctx);
    test_extended_wilcards(ctx);

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

    test_layout_index_ranges(ctx, too_much_layouts, too_much_symbols);
    test_extended_layout_indices(ctx);
    test_all_qualifier(ctx, too_much_layouts, too_much_symbols);
    test_layout_specific_options(ctx);

    xkb_context_unref(ctx);
    return EXIT_SUCCESS;
}
