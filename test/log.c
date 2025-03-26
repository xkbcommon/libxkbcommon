/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include "test.h"
#include "context.h"
#include "messages-codes.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wmissing-format-attribute"
#endif

static const char *
log_level_to_string(enum xkb_log_level level)
{
    switch (level) {
    case XKB_LOG_LEVEL_CRITICAL:
        return "critical";
    case XKB_LOG_LEVEL_ERROR:
        return "error";
    case XKB_LOG_LEVEL_WARNING:
        return "warning";
    case XKB_LOG_LEVEL_INFO:
        return "info";
    case XKB_LOG_LEVEL_DEBUG:
        return "debug";
    }

    return "unknown";
}

ATTR_PRINTF(3, 0) static void
log_fn(struct xkb_context *ctx, enum xkb_log_level level,
       const char *fmt, va_list args)
{
    char *s;
    int size;
    darray_char *ls = xkb_context_get_user_data(ctx);
    assert(ls);

    size = vasprintf(&s, fmt, args);
    assert(size != -1);

    darray_append_string(*ls, log_level_to_string(level));
    darray_append_lit(*ls, ": ");
    darray_append_string(*ls, s);
    free(s);
}

static void
test_basic(void)
{
    int ret;
    ret = setenv("XKB_LOG_LEVEL", "warn", 1);
    assert(ret == 0);
    ret = setenv("XKB_LOG_VERBOSITY", "5", 1);
    assert(ret == 0);

    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    darray_char log_string;
    darray_init(log_string);
    xkb_context_set_user_data(ctx, &log_string);
    xkb_context_set_log_fn(ctx, log_fn);

    log_warn(ctx, XKB_LOG_MESSAGE_NO_ID, "first warning: %d\n", 87);
    log_info(ctx, XKB_LOG_MESSAGE_NO_ID, "first info\n");
    log_dbg(ctx, XKB_LOG_MESSAGE_NO_ID, "first debug: %s\n", "hello");
    log_err(ctx, XKB_LOG_MESSAGE_NO_ID, "first error: %lu\n", 115415UL);
    log_vrb(ctx, 5, XKB_LOG_MESSAGE_NO_ID, "first verbose 5\n");

    xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_DEBUG);
    log_warn(ctx, XKB_LOG_MESSAGE_NO_ID, "second warning: %d\n", 87);
    log_dbg(ctx, XKB_LOG_MESSAGE_NO_ID, "second debug: %s %s\n", "hello", "world");
    log_info(ctx, XKB_LOG_MESSAGE_NO_ID, "second info\n");
    log_err(ctx, XKB_ERROR_MALFORMED_NUMBER_LITERAL, "second error: %lu\n", 115415UL);
    log_vrb(ctx, 6, XKB_LOG_MESSAGE_NO_ID, "second verbose 6\n");

    xkb_context_set_log_verbosity(ctx, 0);
    xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_CRITICAL);
    log_warn(ctx, XKB_LOG_MESSAGE_NO_ID, "third warning: %d\n", 87);
    log_dbg(ctx, XKB_LOG_MESSAGE_NO_ID, "third debug: %s %s\n", "hello", "world");
    log_info(ctx, XKB_LOG_MESSAGE_NO_ID, "third info\n");
    log_err(ctx, XKB_LOG_MESSAGE_NO_ID, "third error: %lu\n", 115415UL);
    log_vrb(ctx, 0, XKB_LOG_MESSAGE_NO_ID, "third verbose 0\n");

    printf("%s", darray_items(log_string));

    assert(streq(darray_items(log_string),
                 "warning: first warning: 87\n"
                 "error: first error: 115415\n"
                 "warning: first verbose 5\n"
                 "warning: second warning: 87\n"
                 "debug: second debug: hello world\n"
                 "info: second info\n"
                 "error: [XKB-034] second error: 115415\n"));

    xkb_context_unref(ctx);
    darray_free(log_string);
}

struct test_data {
    const char * const input;
    const char * const log;
    bool error;
};

static void
test_keymaps(void)
{
    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    darray_char log_string;
    darray_init(log_string);
    xkb_context_set_user_data(ctx, &log_string);
    xkb_context_set_log_fn(ctx, log_fn);

    xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_WARNING);
    xkb_context_set_log_verbosity(ctx, 10);

    const struct test_data keymaps[] = {
        {
            .input = "",
            .log = "error: [XKB-822] Failed to parse input xkb string\n",
            .error = true
        },
        {
            .input = " ",
            .log = "error: [XKB-822] Failed to parse input xkb string\n",
            .error = true
        },
        {
            .input = "\n",
            .log = "error: [XKB-822] Failed to parse input xkb string\n",
            .error = true
        },
        {
            .input = "xkb_keymap {\n",
            .log =
                "error: [XKB-769] (input string):1:12: syntax error\n"
                "error: [XKB-822] Failed to parse input xkb string\n",
            .error = true
        },
        {
            .input =
                "xkb_keymap \"\\j\"\n"
                " { symbols = { };\n"
                "};",
            .log =
                "warning: [XKB-645] (input string):1:12: unknown escape sequence (\\j) in string literal\n"
                "error: [XKB-769] (input string):2:4: syntax error\n"
                "error: [XKB-822] Failed to parse input xkb string\n",
            .error = true
        },
        {
            .input =
                "xkb_keymap {\n"
                "  xkb_keycodes {\n"
                "    <> = 1;\n"
                "\n"
                "    alias <1> = <>;\n"
                "    alias <1> =\n"
                "                <>;\n"
                "  };\n"
                "  xkb_types \"\\400x\\j\" { };\n"
                "  xkb_compat {\n"
                "    interpret invalidKeysym +\n"
                "                              Any { repeat = true; };\n"
                "  };\n"
                "  xkb_symbols { key <> {[0x30, leftshoe]}; };\n"
                "};",
            .log =
                "warning: [XKB-193] (input string):9:13: invalid octal escape sequence (\\400) in string literal\n"
                "warning: [XKB-645] (input string):9:13: unknown escape sequence (\\j) in string literal\n"
                "warning: [XKB-107] (input string):11:15: unrecognized keysym \"invalidKeysym\"\n"
                "warning: [XKB-489] (input string):14:26: numeric keysym \"0x30\" (48)\n"
                "warning: [XKB-301] (input string):14:32: deprecated keysym \"leftshoe\".\n"
                "warning: [XKB-433] No map in include statement, but \"(input string)\" contains several; Using first defined map, \"(unnamed)\"\n"
                "warning: [XKB-523] Alias of <1> for <> declared more than once; First definition ignored\n"
                "warning: [XKB-286] The type \"TWO_LEVEL\" for key '<>' group 1 was not previously defined; Using the default type\n"
                "warning: [XKB-516] Type \"default\" has 1 levels, but <> has 2 levels; Ignoring extra symbols\n",
            .error = false
        }
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(keymaps); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        struct xkb_keymap *keymap =
            test_compile_buffer(ctx, keymaps[k].input, strlen(keymaps[k].input));
        assert(keymaps[k].error ^ !!keymap);
        xkb_keymap_unref(keymap);
        assert_printf(streq_not_null(darray_items(log_string), keymaps[k].log),
                      "Expected:\n%s\nGot:\n%s\n",
                      keymaps[k].log, darray_items(log_string));
        darray_free(log_string);
    }

    xkb_context_unref(ctx);
}

static void
test_compose(void)
{
    struct xkb_context *ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    darray_char log_string;
    darray_init(log_string);
    xkb_context_set_user_data(ctx, &log_string);
    xkb_context_set_log_fn(ctx, log_fn);

    xkb_context_set_log_level(ctx, XKB_LOG_LEVEL_WARNING);
    xkb_context_set_log_verbosity(ctx, 10);

    const struct test_data composes[] = {
        {
            .input = "",
            .log = NULL,
            .error = false
        },
        {
            .input = "\n",
            .log = NULL,
            .error = false
        },
        {
            .input = "\xff\n",
            .log =
                "error: [XKB-542] (input string):1:1: unexpected non-ASCII character.\n"
                "error: [XKB-542] (input string):1:1: This could be a file encoding issue. Supported file encodings are ASCII and UTF-8.\n"
                "error: (input string):1:1: failed to parse file\n",
            .error = true
        },
        {
            .input =
                "<leftshoe> : x\n"
                "include \"x\"\n",
            .log =
                "warning: [XKB-301] (input string):1:1: deprecated keysym \"leftshoe\".\n"
                "error: (input string):2:9: failed to open included Compose file \"x\": No such file or directory\n"
                "error: (input string):2:9: failed to parse file\n",
            .error = true
        },
        {
            .input =
                "<a> : \"a\"\n"
                "\n"
                "<b> : \"i\\j\\xk\n"
                "<0x30> : \"\\400\" invalidKeysym\n"
                "<0> <1> <2> <3> <4> <5> <6> <7> <8> <9> <leftshoe> : \"\"\n",
            .log =
                "warning: [XKB-645] (input string):3:7: unknown escape sequence (\\j) in string literal\n"
                "warning: [XKB-193] (input string):3:7: illegal hexadecimal escape sequence (\\x) in string literal\n"
                "error: [XKB-685] (input string):3:7: unterminated string literal\n"
                "warning: [XKB-193] (input string):4:10: illegal octal escape sequence (\\400) in string literal\n"
                "error: (input string):4:17: unrecognized keysym \"invalidKeysym\" on right-hand side\n"
                "warning: [XKB-301] (input string):5:41: deprecated keysym \"leftshoe\".\n"
                "warning: [XKB-685] (input string):5:41: too many keysyms (11) on left-hand side; skipping line\n",
            .error = false
        },
        {
            .input =
                ":\n"
                "<a> :\n"
                "#\n"
                "<c> : \"a\" \"b\"\n"
                "<d> : a b\n",
            .log =
                "warning: (input string):1:1: expected at least one keysym on left-hand side; skipping line\n"
                "warning: [XKB-685] (input string):2:5: right-hand side must have at least one of string or keysym; skipping line\n"
                "warning: (input string):4:11: right-hand side can have at most one string; skipping line\n"
                "error: [XKB-685] (input string):5:9: unrecognized modifier \"b\"\n",
            .error = false
        },
        {
            .input =
                "<a> : a\n"
                "<a> : a\n"
                "<b>     : b\n"
                "<b> <c> : x\n"
                "<c> <d> : y\n"
                "<c>     : c\n",
            .log =
                "warning: (input string):2:7: this compose sequence is a duplicate of another; skipping line\n"
                "warning: (input string):4:11: a sequence already exists which is a prefix of this sequence; overriding\n"
                "warning: (input string):6:11: this compose sequence is a prefix of another; skipping line\n",
            .error = false
        }
    };

    for (unsigned int k = 0; k < ARRAY_SIZE(composes); k++) {
        fprintf(stderr, "------\n*** %s: #%u ***\n", __func__, k);
        struct xkb_compose_table *table = xkb_compose_table_new_from_buffer(
            ctx, composes[k].input, strlen(composes[k].input), "",
            XKB_COMPOSE_FORMAT_TEXT_V1,
            XKB_COMPOSE_COMPILE_NO_FLAGS
        );
        assert(composes[k].error ^ !!table);
        xkb_compose_table_unref(table);
        assert_printf(streq_null(darray_items(log_string), composes[k].log),
                      "Expected:\n%s\nGot:\n%s\n",
                      darray_items(log_string), composes[k].log);
        darray_free(log_string);
    }

    xkb_context_unref(ctx);
}

int
main(void)
{
    test_init();
    test_basic();
    test_keymaps();
    test_compose();
    return EXIT_SUCCESS;
}
