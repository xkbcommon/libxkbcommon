/*
 * Copyright © 2014 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include <time.h>
#include <errno.h>
#include <stdio.h>

#include "xkbcommon/xkbcommon-compose.h"

#include "test.h"
#include "src/utf8.h"
#include "src/keysym.h"
#include "src/compose/parser.h"
#include "src/compose/escape.h"
#include "src/compose/dump.h"
#include "test/compose-iter.h"
#include "test/utils-text.h"

static const char *
compose_status_string(enum xkb_compose_status status)
{
    switch (status) {
    case XKB_COMPOSE_NOTHING:
        return "nothing";
    case XKB_COMPOSE_COMPOSING:
        return "composing";
    case XKB_COMPOSE_COMPOSED:
        return "composed";
    case XKB_COMPOSE_CANCELLED:
        return "cancelled";
    }

    return "<invalid-status>";
}

static const char *
feed_result_string(enum xkb_compose_feed_result result)
{
    switch (result) {
    case XKB_COMPOSE_FEED_IGNORED:
        return "ignored";
    case XKB_COMPOSE_FEED_ACCEPTED:
        return "accepted";
    }

    return "<invalid-result>";
}

/*
 * Feed a sequence of keysyms to a fresh compose state and test the outcome.
 *
 * The varargs consists of lines in the following format:
 *      <input keysym> <expected feed result> <expected status> <expected string> <expected keysym>
 * Terminated by a line consisting only of XKB_KEY_NoSymbol.
 */
static bool
test_compose_seq_va(struct xkb_compose_table *table, va_list ap)
{
    int ret;
    struct xkb_compose_state *state;
    char buffer[MAX(XKB_COMPOSE_MAX_STRING_SIZE, XKB_KEYSYM_NAME_MAX_SIZE)];

    state = xkb_compose_state_new(table, XKB_COMPOSE_STATE_NO_FLAGS);
    assert(state);

    for (int i = 1; ; i++) {
        xkb_keysym_t input_keysym;
        enum xkb_compose_feed_result result, expected_result;
        enum xkb_compose_status status, expected_status;
        const char *expected_string;
        xkb_keysym_t keysym, expected_keysym;

        input_keysym = va_arg(ap, xkb_keysym_t);
        if (input_keysym == XKB_KEY_NoSymbol)
            break;

        expected_result = va_arg(ap, enum xkb_compose_feed_result);
        expected_status = va_arg(ap, enum xkb_compose_status);
        expected_string = va_arg(ap, const char *);
        expected_keysym = va_arg(ap, xkb_keysym_t);

        result = xkb_compose_state_feed(state, input_keysym);

        if (result != expected_result) {
            fprintf(stderr, "after feeding %d keysyms:\n", i);
            fprintf(stderr, "expected feed result: %s\n",
                    feed_result_string(expected_result));
            fprintf(stderr, "got feed result: %s\n",
                    feed_result_string(result));
            goto fail;
        }

        status = xkb_compose_state_get_status(state);
        if (status != expected_status) {
            fprintf(stderr, "after feeding %d keysyms:\n", i);
            fprintf(stderr, "expected status: %s\n",
                    compose_status_string(expected_status));
            fprintf(stderr, "got status: %s\n",
                    compose_status_string(status));
            goto fail;
        }

        ret = xkb_compose_state_get_utf8(state, buffer, sizeof(buffer));
        if (ret < 0 || (size_t) ret >= sizeof(buffer)) {
            fprintf(stderr, "after feeding %d keysyms:\n", i);
            fprintf(stderr, "expected string: %s\n", expected_string);
            fprintf(stderr, "got error: %d\n", ret);
            goto fail;
        }
        if (!streq(buffer, expected_string)) {
            fprintf(stderr, "after feeding %d keysyms:\n", i);
            fprintf(stderr, "expected string: %s\n", strempty(expected_string));
            fprintf(stderr, "got string: %s\n", buffer);
            goto fail;
        }

        keysym = xkb_compose_state_get_one_sym(state);
        if (keysym != expected_keysym) {
            fprintf(stderr, "after feeding %d keysyms:\n", i);
            xkb_keysym_get_name(expected_keysym, buffer, sizeof(buffer));
            fprintf(stderr, "expected keysym: %s\n", buffer);
            xkb_keysym_get_name(keysym, buffer, sizeof(buffer));
            fprintf(stderr, "got keysym (%#x): %s\n", keysym, buffer);
            goto fail;
        }
    }

    xkb_compose_state_unref(state);
    return true;

fail:
    xkb_compose_state_unref(state);
    return false;
}

static bool
test_compose_seq(struct xkb_compose_table *table, ...)
{
    va_list ap;
    bool ok;
    va_start(ap, table);
    ok = test_compose_seq_va(table, ap);
    va_end(ap);
    return ok;
}

static bool
test_compose_seq_buffer(struct xkb_context *ctx, const char *buffer, ...)
{
    va_list ap;
    bool ok;
    struct xkb_compose_table *table;
    table = xkb_compose_table_new_from_buffer(ctx, buffer, strlen(buffer), "",
                                              XKB_COMPOSE_FORMAT_TEXT_V1,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(table);
    va_start(ap, buffer);
    ok = test_compose_seq_va(table, ap);
    va_end(ap);
    xkb_compose_table_unref(table);
    return ok;
}

static void
test_compose_utf8_bom(struct xkb_context *ctx)
{
    const char buffer[] = "\xef\xbb\xbf<A> : X";
    assert(test_compose_seq_buffer(ctx, buffer,
        XKB_KEY_A, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSED, "X", XKB_KEY_X,
        XKB_KEY_NoSymbol));
}

static void
test_invalid_encodings(struct xkb_context *ctx)
{
    struct xkb_compose_table *table;

    /* ISO 8859-1 (latin1) */
    const char iso_8859_1[] = "<A> : \"\xe1\" acute";
    assert(!test_compose_seq_buffer(ctx, iso_8859_1,
        XKB_KEY_A, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSED, "\xc3\xa1", XKB_KEY_acute,
        XKB_KEY_NoSymbol));

    /* UTF-16LE */
    const char utf_16_le[] =
        "<\0A\0>\0 \0:\0 \0X\0\n\0"
        "<\0B\0>\0 \0:\0 \0Y\0";
    table = xkb_compose_table_new_from_buffer(ctx,
                                              utf_16_le, sizeof(utf_16_le), "",
                                              XKB_COMPOSE_FORMAT_TEXT_V1,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(!table);

    /* UTF-16BE */
    const char utf_16_be[] =
        "\0<\0A\0>\0 \0:\0 \0X\0\n"
        "\0<\0B\0>\0 \0:\0 \0Y";
    table = xkb_compose_table_new_from_buffer(ctx,
                                              utf_16_be, sizeof(utf_16_be), "",
                                              XKB_COMPOSE_FORMAT_TEXT_V1,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(!table);

    /* UTF-16BE with BOM */
    const char utf_16_be_bom[] =
        "\xfe\xff"
        "\0<\0A\0>\0 \0:\0 \0X\0\n"
        "\0<\0B\0>\0 \0:\0 \0Y";
    table = xkb_compose_table_new_from_buffer(ctx,
                                              utf_16_be_bom, sizeof(utf_16_be_bom), "",
                                              XKB_COMPOSE_FORMAT_TEXT_V1,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(!table);

    /* UTF-32LE */
    const char utf_32_le[] =
        "<\0\0\0A\0\0\0>\0\0\0 \0\0\0:\0\0\0 \0\0\0X\0\0\0\n\0\0\0"
        "<\0\0\0B\0\0\0>\0\0\0 \0\0\0:\0\0\0 \0\0\0Y\0\0\0";
    table = xkb_compose_table_new_from_buffer(ctx,
                                              utf_32_le, sizeof(utf_32_le), "",
                                              XKB_COMPOSE_FORMAT_TEXT_V1,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(!table);

    /* UTF-32LE with BOM */
    const char utf_32_le_bom[] =
        "\xff\xfe\0\0"
        "<\0\0\0A\0\0\0>\0\0\0 \0\0\0:\0\0\0 \0\0\0X\0\0\0\n\0\0\0"
        "<\0\0\0B\0\0\0>\0\0\0 \0\0\0:\0\0\0 \0\0\0Y\0\0\0";
    table = xkb_compose_table_new_from_buffer(ctx,
                                              utf_32_le_bom, sizeof(utf_32_le_bom), "",
                                              XKB_COMPOSE_FORMAT_TEXT_V1,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(!table);

    /* UTF-32BE */
    const char utf_32_be[] =
        "\0\0\0<\0\0\0A\0\0\0>\0\0\0 \0\0\0:\0\0\0 \0\0\0X\0\0\0\n\0\0\0"
        "<\0\0\0B\0\0\0>\0\0\0 \0\0\0:\0\0\0 \0\0\0Y";
    table = xkb_compose_table_new_from_buffer(ctx,
                                              utf_32_be, sizeof(utf_32_be), "",
                                              XKB_COMPOSE_FORMAT_TEXT_V1,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(!table);
}


static void
test_seqs(struct xkb_context *ctx)
{
    struct xkb_compose_table *table;
    char *path;
    FILE *file;

    path = test_get_path("locale/en_US.UTF-8/Compose");
    file = fopen(path, "rb");
    assert(file);
    free(path);

    table = xkb_compose_table_new_from_file(ctx, file, "",
                                            XKB_COMPOSE_FORMAT_TEXT_V1,
                                            XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(table);
    fclose(file);

    assert(test_compose_seq(table,
        XKB_KEY_dead_tilde,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_space,          XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "~",    XKB_KEY_asciitilde,
        XKB_KEY_NoSymbol));

    assert(test_compose_seq(table,
        XKB_KEY_dead_tilde,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_space,          XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "~",    XKB_KEY_asciitilde,
        XKB_KEY_dead_tilde,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_space,          XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "~",    XKB_KEY_asciitilde,
        XKB_KEY_NoSymbol));

    assert(test_compose_seq(table,
        XKB_KEY_dead_tilde,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_dead_tilde,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "~",    XKB_KEY_asciitilde,
        XKB_KEY_NoSymbol));

    assert(test_compose_seq(table,
        XKB_KEY_dead_acute,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_space,          XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "'",    XKB_KEY_apostrophe,
        XKB_KEY_Caps_Lock,      XKB_COMPOSE_FEED_IGNORED,   XKB_COMPOSE_COMPOSED,   "'",    XKB_KEY_apostrophe,
        XKB_KEY_NoSymbol));

    assert(test_compose_seq(table,
        XKB_KEY_dead_acute,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_dead_acute,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "´",    XKB_KEY_acute,
        XKB_KEY_NoSymbol));

    assert(test_compose_seq(table,
        XKB_KEY_Multi_key,      XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_Shift_L,        XKB_COMPOSE_FEED_IGNORED,   XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_Caps_Lock,      XKB_COMPOSE_FEED_IGNORED,   XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_Control_L,      XKB_COMPOSE_FEED_IGNORED,   XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_T,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "@",    XKB_KEY_at,
        XKB_KEY_NoSymbol));

    assert(test_compose_seq(table,
        XKB_KEY_7,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_NOTHING,    "",     XKB_KEY_NoSymbol,
        XKB_KEY_a,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_NOTHING,    "",     XKB_KEY_NoSymbol,
        XKB_KEY_b,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_NOTHING,    "",     XKB_KEY_NoSymbol,
        XKB_KEY_NoSymbol));

    assert(test_compose_seq(table,
        XKB_KEY_Multi_key,      XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_apostrophe,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_7,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_CANCELLED,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_7,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_NOTHING,    "",     XKB_KEY_NoSymbol,
        XKB_KEY_Caps_Lock,      XKB_COMPOSE_FEED_IGNORED,   XKB_COMPOSE_NOTHING,    "",     XKB_KEY_NoSymbol,
        XKB_KEY_NoSymbol));

    xkb_compose_table_unref(table);

    /* Make sure one-keysym sequences work. */
    assert(test_compose_seq_buffer(ctx,
        "<A>          :  \"foo\"  X \n"
        "<B> <A>      :  \"baz\"  Y \n",
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,  "foo",   XKB_KEY_X,
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,  "foo",   XKB_KEY_X,
        XKB_KEY_C,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_NOTHING,   "",      XKB_KEY_NoSymbol,
        XKB_KEY_B,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING, "",      XKB_KEY_NoSymbol,
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,  "baz",   XKB_KEY_Y,
        XKB_KEY_NoSymbol));

    /* No sequences at all. */
    assert(test_compose_seq_buffer(ctx,
        "",
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_NOTHING,   "",      XKB_KEY_NoSymbol,
        XKB_KEY_B,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_NOTHING,   "",      XKB_KEY_NoSymbol,
        XKB_KEY_C,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_NOTHING,   "",      XKB_KEY_NoSymbol,
        XKB_KEY_Multi_key,      XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_NOTHING,   "",      XKB_KEY_NoSymbol,
        XKB_KEY_dead_acute,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_NOTHING,   "",      XKB_KEY_NoSymbol,
        XKB_KEY_NoSymbol));

    /* Only keysym - string derived from keysym. */
    assert(test_compose_seq_buffer(ctx,
        "<A> <B>     :  X \n"
        "<B> <A>     :  dollar \n"
        "<C>         :  dead_acute \n",
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING, "",      XKB_KEY_NoSymbol,
        XKB_KEY_B,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,  "X",     XKB_KEY_X,
        XKB_KEY_B,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING, "",      XKB_KEY_NoSymbol,
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,  "$",     XKB_KEY_dollar,
        XKB_KEY_C,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,  "",      XKB_KEY_dead_acute,
        XKB_KEY_NoSymbol));

    /* Make sure a cancelling keysym doesn't start a new sequence. */
    assert(test_compose_seq_buffer(ctx,
        "<A> <B>     :  X \n"
        "<C> <D>     :  Y \n",
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING, "",      XKB_KEY_NoSymbol,
        XKB_KEY_C,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_CANCELLED, "",      XKB_KEY_NoSymbol,
        XKB_KEY_D,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_NOTHING,   "",      XKB_KEY_NoSymbol,
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING, "",      XKB_KEY_NoSymbol,
        XKB_KEY_C,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_CANCELLED, "",      XKB_KEY_NoSymbol,
        XKB_KEY_C,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING, "",      XKB_KEY_NoSymbol,
        XKB_KEY_D,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,  "Y",     XKB_KEY_Y,
        XKB_KEY_NoSymbol));
}

static void
test_conflicting(struct xkb_context *ctx)
{
    // new is prefix of old
    assert(test_compose_seq_buffer(ctx,
        "<A> <B> <C>  :  \"foo\"  A \n"
        "<A> <B>      :  \"bar\"  B \n",
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_B,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_C,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "foo",  XKB_KEY_A,
        XKB_KEY_NoSymbol));

    // old is a prefix of new
    assert(test_compose_seq_buffer(ctx,
        "<A> <B>      :  \"bar\"  B \n"
        "<A> <B> <C>  :  \"foo\"  A \n",
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_B,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_C,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "foo",  XKB_KEY_A,
        XKB_KEY_NoSymbol));

    // new duplicate of old
    assert(test_compose_seq_buffer(ctx,
        "<A> <B>      :  \"bar\"  B \n"
        "<A> <B>      :  \"bar\"  B \n",
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_B,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "bar",  XKB_KEY_B,
        XKB_KEY_C,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_NOTHING,    "",     XKB_KEY_NoSymbol,
        XKB_KEY_NoSymbol));

    // new same length as old #1
    assert(test_compose_seq_buffer(ctx,
        "<A> <B>      :  \"foo\"  A \n"
        "<A> <B>      :  \"bar\"  B \n",
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_B,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "bar",  XKB_KEY_B,
        XKB_KEY_NoSymbol));

    // new same length as old #2
    assert(test_compose_seq_buffer(ctx,
        "<A> <B>      :  \"foo\"  A \n"
        "<A> <B>      :  \"foo\"  B \n",
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_B,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "foo",  XKB_KEY_B,
        XKB_KEY_NoSymbol));

    // new same length as old #3
    assert(test_compose_seq_buffer(ctx,
        "<A> <B>      :  \"foo\"  A \n"
        "<A> <B>      :  \"bar\"  A \n",
        XKB_KEY_A,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_B,              XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "bar",  XKB_KEY_A,
        XKB_KEY_NoSymbol));
}

static void
test_state(struct xkb_context *ctx)
{
    struct xkb_compose_table *table;
    struct xkb_compose_state *state;
    char *path;
    FILE *file;

    path = test_get_path("locale/en_US.UTF-8/Compose");
    file = fopen(path, "rb");
    assert(file);
    free(path);

    table = xkb_compose_table_new_from_file(ctx, file, "",
                                            XKB_COMPOSE_FORMAT_TEXT_V1,
                                            XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(table);
    fclose(file);

    state = xkb_compose_state_new(table, XKB_COMPOSE_STATE_NO_FLAGS);
    assert(state);

    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_NOTHING);
    xkb_compose_state_reset(state);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_NOTHING);
    xkb_compose_state_feed(state, XKB_KEY_NoSymbol);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_NOTHING);
    xkb_compose_state_feed(state, XKB_KEY_Multi_key);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_COMPOSING);
    xkb_compose_state_reset(state);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_NOTHING);
    xkb_compose_state_feed(state, XKB_KEY_Multi_key);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_COMPOSING);
    xkb_compose_state_feed(state, XKB_KEY_Multi_key);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_CANCELLED);
    xkb_compose_state_feed(state, XKB_KEY_Multi_key);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_COMPOSING);
    xkb_compose_state_feed(state, XKB_KEY_Multi_key);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_CANCELLED);
    xkb_compose_state_reset(state);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_NOTHING);
    xkb_compose_state_feed(state, XKB_KEY_dead_acute);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_COMPOSING);
    xkb_compose_state_feed(state, XKB_KEY_A);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_COMPOSED);
    xkb_compose_state_reset(state);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_NOTHING);
    xkb_compose_state_feed(state, XKB_KEY_dead_acute);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_COMPOSING);
    xkb_compose_state_feed(state, XKB_KEY_A);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_COMPOSED);
    xkb_compose_state_reset(state);
    xkb_compose_state_feed(state, XKB_KEY_NoSymbol);
    assert(xkb_compose_state_get_status(state) == XKB_COMPOSE_NOTHING);

    xkb_compose_state_unref(state);
    xkb_compose_table_unref(table);
}

static void
test_XCOMPOSEFILE(struct xkb_context *ctx)
{
    struct xkb_compose_table *table;
    char *path;

    /* Error: directory */
    path = test_get_path("locale/en_US.UTF-8");
    setenv("XCOMPOSEFILE", path, 1);
    free(path);

    table = xkb_compose_table_new_from_locale(ctx, "blabla",
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert_printf(errno != ENODEV && errno != EISDIR,
                  "Should not be an error from `map_file`\n");
    assert(!table);

    /* OK: regular file */
    path = test_get_path("locale/en_US.UTF-8/Compose");
    setenv("XCOMPOSEFILE", path, 1);
    free(path);

    table = xkb_compose_table_new_from_locale(ctx, "blabla",
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(table);

    unsetenv("XCOMPOSEFILE");

    assert(test_compose_seq(table,
        XKB_KEY_dead_tilde,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_space,          XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "~",    XKB_KEY_asciitilde,
        XKB_KEY_NoSymbol));

    xkb_compose_table_unref(table);
}

static void
test_from_locale(struct xkb_context *ctx)
{
    struct xkb_compose_table *table;
    char *path;

    path = test_get_path("locale");
    setenv("XLOCALEDIR", path, 1);
    free(path);

    /* Direct directory name match. */
    table = xkb_compose_table_new_from_locale(ctx, "en_US.UTF-8",
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(table);
    xkb_compose_table_unref(table);

    /* Direct locale name match. */
    table = xkb_compose_table_new_from_locale(ctx, "C.UTF-8",
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(table);
    xkb_compose_table_unref(table);

    /* Alias. */
    table = xkb_compose_table_new_from_locale(ctx, "univ.utf8",
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(table);
    xkb_compose_table_unref(table);

    /* Special case - C. */
    table = xkb_compose_table_new_from_locale(ctx, "C",
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(table);
    xkb_compose_table_unref(table);

    /* Bogus - not found. */
    table = xkb_compose_table_new_from_locale(ctx, "blabla",
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(!table);

    unsetenv("XLOCALEDIR");
}


static void
test_modifier_syntax(struct xkb_context *ctx)
{
    const char *table_string;

    /* We don't do anything with the modifiers, but make sure we can parse
     * them. */

    assert(test_compose_seq_buffer(ctx,
        "None <A>          : X \n"
        "Shift <B>         : Y \n"
        "Ctrl <C>          : Y \n"
        "Alt <D>           : Y \n"
        "Caps <E>          : Y \n"
        "Lock <F>          : Y \n"
        "Shift Ctrl <G>    : Y \n"
        "~Shift <H>        : Y \n"
        "~Shift Ctrl <I>   : Y \n"
        "Shift ~Ctrl <J>   : Y \n"
        "Shift ~Ctrl ~Alt <K> : Y \n"
        "! Shift <B>       : Y \n"
        "! Ctrl <C>        : Y \n"
        "! Alt <D>         : Y \n"
        "! Caps <E>        : Y \n"
        "! Lock <F>        : Y \n"
        "! Shift Ctrl <G>  : Y \n"
        "! ~Shift <H>      : Y \n"
        "! ~Shift Ctrl <I> : Y \n"
        "! Shift ~Ctrl <J> : Y \n"
        "! Shift ~Ctrl ~Alt <K> : Y \n"
        "<L> ! Shift <M>   : Y \n"
        "None <N> ! Shift <O> : Y \n"
        "None <P> ! Shift <Q> : Y \n",
        XKB_KEY_NoSymbol));

    fprintf(stderr, "<START bad input string>\n");
    table_string =
        "! None <A>        : X \n"
        "! Foo <B>         : X \n"
        "None ! Shift <C>  : X \n"
        "! ! <D>           : X \n"
        "! ~ <E>           : X \n"
        "! ! <F>           : X \n"
        "! Ctrl ! Ctrl <G> : X \n"
        "<H> !             : X \n"
        "<I> None          : X \n"
        "None None <J>     : X \n"
        "<K>               : !Shift X \n";
    assert(!xkb_compose_table_new_from_buffer(ctx, table_string,
                                              strlen(table_string), "C",
                                              XKB_COMPOSE_FORMAT_TEXT_V1,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS));
    fprintf(stderr, "<END bad input string>\n");
}

static void
test_include(struct xkb_context *ctx)
{
    char *path, *table_string;

    path = test_get_path("locale/en_US.UTF-8/Compose");
    assert(path);

    /* We don't have a mechanism to change the include paths like we
     * have for keymaps. So we must include the full path. */
    table_string = asprintf_safe("<dead_tilde> <space>   : \"foo\" X\n"
                                 "include \"%s\"\n"
                                 "<dead_tilde> <dead_tilde> : \"bar\" Y\n", path);
    assert(table_string);

    assert(test_compose_seq_buffer(ctx, table_string,
        /* No conflict. */
        XKB_KEY_dead_acute,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_dead_acute,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "´",    XKB_KEY_acute,

        /* Comes before - doesn't override. */
        XKB_KEY_dead_tilde,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_space,          XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "~",    XKB_KEY_asciitilde,

        /* Comes after - does override. */
        XKB_KEY_dead_tilde,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_dead_tilde,     XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "bar",  XKB_KEY_Y,

        XKB_KEY_NoSymbol));

    free(path);
    free(table_string);
}

static void
test_override(struct xkb_context *ctx)
{
    const char *table_string = "<dead_circumflex> <dead_circumflex> : \"foo\" X\n"
                               "<dead_circumflex> <e> : \"bar\" Y\n"
                               "<dead_circumflex> <dead_circumflex> <e> : \"baz\" Z\n";

    assert(test_compose_seq_buffer(ctx, table_string,
        /* Comes after - does override. */
        XKB_KEY_dead_circumflex, XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_dead_circumflex, XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_e,               XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "baz",  XKB_KEY_Z,

        /* Override does not affect sibling nodes */
        XKB_KEY_dead_circumflex, XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_e,               XKB_COMPOSE_FEED_ACCEPTED,  XKB_COMPOSE_COMPOSED,   "bar",  XKB_KEY_Y,

        XKB_KEY_NoSymbol));
}

static bool
test_eq_entry_va(struct xkb_compose_table_entry *entry, xkb_keysym_t keysym_ref, const char *utf8_ref, va_list ap)
{
    assert (entry != NULL);

    assert (xkb_compose_table_entry_keysym(entry) == keysym_ref);

    const char *utf8 = xkb_compose_table_entry_utf8(entry);
    assert (utf8 && utf8_ref && strcmp(utf8, utf8_ref) == 0);

    size_t nsyms;
    const xkb_keysym_t *sequence = xkb_compose_table_entry_sequence(entry, &nsyms);

    xkb_keysym_t keysym;
    for (unsigned k = 0; ; k++) {
        keysym = va_arg(ap, xkb_keysym_t);
        if (keysym == XKB_KEY_NoSymbol) {
            return (k == nsyms - 1);
        }
        assert (k < nsyms);
        assert (keysym == sequence[k]);
    }
}

static bool
test_eq_entry(struct xkb_compose_table_entry *entry, xkb_keysym_t keysym, const char *utf8, ...)
{
    va_list ap;
    bool ok;
    va_start(ap, utf8);
    ok = test_eq_entry_va(entry, keysym, utf8, ap);
    va_end(ap);
    return ok;
}

static bool
test_eq_entries(struct xkb_compose_table_entry *entry1, struct xkb_compose_table_entry *entry2)
{
    if (!entry1 || !entry2)
        goto error;
    bool ok = true;
    if (entry1->keysym != entry2->keysym ||
        !streq_null(entry1->utf8, entry2->utf8) ||
        entry1->sequence_length != entry2->sequence_length)
        ok = false;
    for (size_t k = 0; k < entry1->sequence_length; k++) {
        if (entry1->sequence[k] != entry2->sequence[k])
            ok = false;
    }
    if (ok)
        return true;
error:
#define print_entry(msg, entry)                   \
    fprintf(stderr, msg);                         \
    if (entry)                                    \
        print_compose_table_entry(stderr, entry); \
    else                                          \
        fprintf(stderr, "\n");
    print_entry("Expected: ", entry1);
    print_entry("Got:      ", entry2);
#undef print_entry
    return false;
}

static void
compose_traverse_fn(struct xkb_compose_table_entry *entry_ref, void *data)
{
    struct xkb_compose_table_iterator *iter = (struct xkb_compose_table_iterator *)data;
    struct xkb_compose_table_entry *entry = xkb_compose_table_iterator_next(iter);
    assert(test_eq_entries(entry_ref, entry));
}

static void
test_traverse(struct xkb_context *ctx, size_t quickcheck_loops)
{
    struct xkb_compose_table *table;
    struct xkb_compose_table_iterator *iter;

    /* Empty table */
    table = xkb_compose_table_new_from_buffer(ctx, "", 0, "",
                                              XKB_COMPOSE_FORMAT_TEXT_V1,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(table);
    iter = xkb_compose_table_iterator_new(table);
    assert (xkb_compose_table_iterator_next(iter) == NULL);
    xkb_compose_table_iterator_free(iter);
    xkb_compose_table_unref(table);

    /* Non-empty table */
    const char *buffer = "<dead_circumflex> <dead_circumflex> : \"foo\" X\n"
                         "<Ahook> <x> : \"foobar\"\n"
                         "<Multi_key> <o> <e> : oe\n"
                         "<dead_circumflex> <e> : \"bar\" Y\n"
                         "<Multi_key> <a> <e> : \"æ\" ae\n"
                         "<dead_circumflex> <a> : \"baz\" Z\n"
                         "<dead_acute> <e> : \"é\" eacute\n"
                         "<Multi_key> <a> <a> <c>: \"aac\"\n"
                         "<Multi_key> <a> <a> <b>: \"aab\"\n"
                         "<Multi_key> <a> <a> <a>: \"aaa\"\n";

    table = xkb_compose_table_new_from_buffer(ctx, buffer, strlen(buffer), "",
                                              XKB_COMPOSE_FORMAT_TEXT_V1,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
    assert(table);

    iter = xkb_compose_table_iterator_new(table);

    test_eq_entry(xkb_compose_table_iterator_next(iter),
                  XKB_KEY_eacute, "é",
                  XKB_KEY_dead_acute, XKB_KEY_e, XKB_KEY_NoSymbol);

    test_eq_entry(xkb_compose_table_iterator_next(iter),
                  XKB_KEY_Z, "baz",
                  XKB_KEY_dead_circumflex, XKB_KEY_a, XKB_KEY_NoSymbol);

    test_eq_entry(xkb_compose_table_iterator_next(iter),
                  XKB_KEY_Y, "bar",
                  XKB_KEY_dead_circumflex, XKB_KEY_e, XKB_KEY_NoSymbol);

    test_eq_entry(xkb_compose_table_iterator_next(iter),
                  XKB_KEY_X, "foo",
                  XKB_KEY_dead_circumflex, XKB_KEY_dead_circumflex, XKB_KEY_NoSymbol);

    test_eq_entry(xkb_compose_table_iterator_next(iter),
                  XKB_KEY_NoSymbol, "aaa",
                  XKB_KEY_Multi_key, XKB_KEY_a, XKB_KEY_a, XKB_KEY_a, XKB_KEY_NoSymbol);

    test_eq_entry(xkb_compose_table_iterator_next(iter),
                  XKB_KEY_NoSymbol, "aab",
                  XKB_KEY_Multi_key, XKB_KEY_a, XKB_KEY_a, XKB_KEY_b, XKB_KEY_NoSymbol);

    test_eq_entry(xkb_compose_table_iterator_next(iter),
                  XKB_KEY_NoSymbol, "aac",
                  XKB_KEY_Multi_key, XKB_KEY_a, XKB_KEY_a, XKB_KEY_c, XKB_KEY_NoSymbol);

    test_eq_entry(xkb_compose_table_iterator_next(iter),
                  XKB_KEY_ae, "æ",
                  XKB_KEY_Multi_key, XKB_KEY_a, XKB_KEY_e, XKB_KEY_NoSymbol);

    test_eq_entry(xkb_compose_table_iterator_next(iter),
                  XKB_KEY_oe, "",
                  XKB_KEY_Multi_key, XKB_KEY_o, XKB_KEY_e, XKB_KEY_NoSymbol);

    test_eq_entry(xkb_compose_table_iterator_next(iter),
                  XKB_KEY_NoSymbol, "foobar",
                  XKB_KEY_Ahook, XKB_KEY_x, XKB_KEY_NoSymbol);

    assert (xkb_compose_table_iterator_next(iter) == NULL);

    xkb_compose_table_iterator_free(iter);
    xkb_compose_table_unref(table);

    /* QuickCheck: shuffle compose file lines and compare against
     * reference implementation */
    char *input = test_read_file("locale/en_US.UTF-8/Compose");
    assert(input);
    struct text_line lines[6000];
    size_t input_length = strlen(input);
    size_t lines_count = split_lines(input, input_length, lines, ARRAY_SIZE(lines));
    /* Note: we may add additional new line char */
    char *shuffled = calloc(input_length + 1, sizeof(char));
    assert(shuffled);
    for (size_t k = 0; k < quickcheck_loops; k++) {
        size_t shuffled_length = shuffle_lines(lines, lines_count, shuffled);
        table = xkb_compose_table_new_from_buffer(ctx, shuffled, shuffled_length, "",
                                                  XKB_COMPOSE_FORMAT_TEXT_V1,
                                                  XKB_COMPOSE_COMPILE_NO_FLAGS);
        assert(table);

        iter = xkb_compose_table_iterator_new(table);
        assert(iter);
        xkb_compose_table_for_each(table, compose_traverse_fn, iter);
        assert(xkb_compose_table_iterator_next(iter) == NULL);
        xkb_compose_table_iterator_free(iter);

        xkb_compose_table_unref(table);
    }
    free(shuffled);
    free(input);
}

static void
test_string_length(struct xkb_context *ctx)
{
    // Invalid: empty string
    const char table_string_1[] = "<a> <b> : \"\" X\n";
    assert(test_compose_seq_buffer(ctx, table_string_1,
        XKB_KEY_a, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSING, "", XKB_KEY_NoSymbol,
        XKB_KEY_b, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSED,  "", XKB_KEY_X,
        XKB_KEY_NoSymbol));

    char long_string[XKB_COMPOSE_MAX_STRING_SIZE] = { 0 };
    memset(long_string, 0x61, XKB_COMPOSE_MAX_STRING_SIZE - 1);
    char table_string_2[XKB_COMPOSE_MAX_STRING_SIZE + sizeof(table_string_1) - 1];
    assert(snprintf_safe(table_string_2, sizeof(table_string_2),
                         "<a> <b> : \"%s\" X\n", long_string));
    assert(test_compose_seq_buffer(ctx, table_string_2,
        XKB_KEY_a, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSING, "",          XKB_KEY_NoSymbol,
        XKB_KEY_b, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSED,  long_string, XKB_KEY_X,
        XKB_KEY_NoSymbol));
}

static void
test_decode_escape_sequences(struct xkb_context *ctx)
{
    /* The following escape sequences should be ignored:
     * • \401 overflows
     * • \0 and \x0 produce NULL
     */
    const char table_string_1[] = "<o> <e> : \"\\401f\\x0o\\0o\" X\n";

    assert(test_compose_seq_buffer(ctx, table_string_1,
        XKB_KEY_o, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSING,  "",     XKB_KEY_NoSymbol,
        XKB_KEY_e, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSED,   "foo",  XKB_KEY_X,
        XKB_KEY_NoSymbol));

    /* Test various cases */
    const char table_string_2[] =
        "<a> : \"\\x0abcg\\\"x\" A\n" /* hexadecimal sequence has max 2 chars */
        "<b> : \"éxyz\" B\n"          /* non-ASCII (2 bytes) */
        "<c> : \"€xyz\" C\n"          /* non-ASCII (3 bytes) */
        "<d> : \"✨xyz\" D\n"         /* non-ASCII (4 bytes) */
        "<e> : \"✨\\x0aé\\x0a€x\\\"\" E\n"
        "<f> : \"\" F\n";

    assert(test_compose_seq_buffer(ctx, table_string_2,
        XKB_KEY_a, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSED, "\x0a""bcg\"x",   XKB_KEY_A,
        XKB_KEY_b, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSED, "éxyz",           XKB_KEY_B,
        XKB_KEY_c, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSED, "€xyz",           XKB_KEY_C,
        XKB_KEY_d, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSED, "✨xyz",           XKB_KEY_D,
        XKB_KEY_e, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSED, "✨\x0aé\x0a€x\"", XKB_KEY_E,
        XKB_KEY_f, XKB_COMPOSE_FEED_ACCEPTED, XKB_COMPOSE_COMPOSED, "",               XKB_KEY_F,
        XKB_KEY_NoSymbol));
}

static uint32_t
random_non_null_unicode_char(bool ascii)
{
    if (ascii)
        return 0x01 + (rand() % 0x80);
    switch (rand() % 5) {
        case 0:
            /* U+0080..U+07FF: 2 bytes in UTF-8 */
            return 0x80 + (rand() % 0x800);
        case 1:
            /* U+0800..U+FFFF: 3 bytes in UTF-8 */
            return 0x800 + (rand() % 0x10000);
        case 2:
            /* U+10000..U+10FFFF: 4 bytes in UTF-8 */
            return 0x10000 + (rand() % 0x110000);
        default:
            /* NOTE: Higher probability for ASCII */
            /* U+0001..U+007F: 1 byte in UTF-8 */
            return 0x01 + (rand() % 0x80);
    }
}

static void
test_encode_escape_sequences(struct xkb_context *ctx)
{
    char *escaped;

    /* Test empty string */
    escaped = escape_utf8_string_literal("");
    assert_streq_not_null("Empty string", "", escaped);
    free(escaped);

    /* Test specific ASCII characters: ", \ */
    escaped = escape_utf8_string_literal("\"\\");
    assert_streq_not_null("Quote and backslash", "\\\"\\\\", escaped);
    free(escaped);

    /* Test round-trip of random strings */
#   define SAMPLE_SIZE 1000
#   define MIN_CODE_POINT 0x0001
#   define MAX_CODE_POINTS_COUNT 15
    char buf[1 + MAX_CODE_POINTS_COUNT * 4];
    for (int ascii = 1; ascii >= 0; ascii--) {
        for (size_t s = 0; s < SAMPLE_SIZE; s++) {
            memset(buf, 0xab, sizeof(buf));
            /* Create the string */
            size_t length = 1 + (rand() % MAX_CODE_POINTS_COUNT);
            size_t c = 0;
            for (size_t idx = 0; idx < length; idx++) {
                int nbytes;
                /* Get a random Unicode code point and encode it in UTF-8 */
                do {
                    const uint32_t cp = random_non_null_unicode_char(ascii);
                    nbytes = utf32_to_utf8(cp, &buf[c]);
                } while (!nbytes); /* Handle invalid code point in UTF-8 */
                c += nbytes - 1;
                assert(c <= sizeof(buf) - 1);
            }
            assert_printf(buf[c] == '\0', "NULL-terminated string\n");
            assert_printf(strlen(buf) == c, "Contains no NULL char\n");
            assert_printf(is_valid_utf8(buf, c),
                          "Invalid input UTF-8 string: \"%s\"\n", buf);
            /* Escape the string */
            escaped = escape_utf8_string_literal(buf);
            if (!escaped)
                break;
            assert_printf(is_valid_utf8(escaped, strlen(escaped)),
                          "Invalid input UTF-8 string: %s\n", escaped);
            char *string_literal = asprintf_safe("\"%s\"", escaped);
            if (!string_literal) {
                free(escaped);
                break;
            }
            /* Unescape the string */
            char *unescaped = parse_string_literal(ctx, string_literal);
            assert_streq_not_null("Escaped string", buf, unescaped);
            free(unescaped);
            free(string_literal);
            free(escaped);
        }
    }
#   undef SAMPLE_SIZE
#   undef MIN_CODE_POINT
#   undef MAX_CODE_POINTS_COUNT
}

/* Roundtrip check: check that a table parsed from a file and the table parsed
 * from the dump of the previous table are identical */
static void
test_roundtrip(struct xkb_context *ctx)
{
/* TODO: add support for systems without open_memstream */
#if HAVE_OPEN_MEMSTREAM
    bool ok = false;

    /* Parse reference file */
    char *input = test_read_file("locale/en_US.UTF-8/Compose");
    assert(input);
    size_t input_length = strlen(input);
    struct xkb_compose_table *ref_table = xkb_compose_table_new_from_buffer(
        ctx, input, input_length, "",
        XKB_COMPOSE_FORMAT_TEXT_V1,
        XKB_COMPOSE_COMPILE_NO_FLAGS
    );
    free(input);
    assert(ref_table);

    /* Dump reference Compose table */
    char *output;
    size_t output_length = 0;
    FILE *output_file = open_memstream(&output, &output_length);
    assert(output_file);

    ok = xkb_compose_table_dump(output_file, ref_table);
    fclose(output_file);
    assert(input);

    if (!ok) {
        free(output);
        xkb_compose_table_unref(ref_table);
        exit(TEST_SETUP_FAILURE);
    }

    /* Parse dumped table */
    struct xkb_compose_table *table = xkb_compose_table_new_from_buffer(
        ctx, output, output_length, "",
        XKB_COMPOSE_FORMAT_TEXT_V1,
        XKB_COMPOSE_COMPILE_NO_FLAGS
    );
    free(output);
    assert(table);

    /* Check roundtrip by comparing table entries */
    struct xkb_compose_table_iterator *iter = xkb_compose_table_iterator_new(table);
    xkb_compose_table_for_each(ref_table, compose_traverse_fn, iter);
    assert(xkb_compose_table_iterator_next(iter) == NULL);

    xkb_compose_table_iterator_free(iter);
    xkb_compose_table_unref(table);
    xkb_compose_table_unref(ref_table);
#endif
}

/* CLI positional arguments:
 * 1. Seed for the pseudo-random generator:
 *    - Leave it unset or set it to “-” to use current time.
 *    - Use an integer to set it explicitly.
 * 2. Number of quickcheck loops:
 *    - Leave it unset to use the default. It depends if the `RUNNING_VALGRIND`
 *      environment variable is set.
 *    - Use an integer to set it explicitly.
 */
int
main(int argc, char *argv[])
{
    struct xkb_context *ctx;

    test_init();

    ctx = test_get_context(CONTEXT_NO_FLAG);
    assert(ctx);

    /* Initialize pseudo-random generator with program arg or current time */
    int seed;
    if (argc >= 2 && !streq(argv[1], "-")) {
        seed = atoi(argv[1]);
    } else {
        seed = (int)time(NULL);
    }
    fprintf(stderr, "Seed for the pseudo-random generator: %d\n", seed);
    srand(seed);

    /* Determine number of loops for quickchecks */
    size_t quickcheck_loops = 50; /* Default */
    if (argc > 2) {
        /* From command-line */
        quickcheck_loops = (size_t)atoi(argv[2]);
    } else if (getenv("RUNNING_VALGRIND") != NULL) {
        /* Reduce if running Valgrind */
        quickcheck_loops = quickcheck_loops / 20;
    }

    /*
     * Ensure no environment variables but “top_srcdir” is set. This ensures
     * that user Compose file paths are unset before the tests and set
     * explicitly when necessary.
     */
#ifdef __linux__
    const char *srcdir = getenv("top_srcdir");
    clearenv();
    if (srcdir)
        setenv("top_srcdir", srcdir, 1);
#else
    unsetenv("XCOMPOSEFILE");
    unsetenv("XDG_CONFIG_HOME");
    unsetenv("HOME");
    unsetenv("XLOCALEDIR");
#endif

    test_compose_utf8_bom(ctx);
    test_invalid_encodings(ctx);
    test_seqs(ctx);
    test_conflicting(ctx);
    test_XCOMPOSEFILE(ctx);
    test_from_locale(ctx);
    test_state(ctx);
    test_modifier_syntax(ctx);
    test_include(ctx);
    test_override(ctx);
    test_traverse(ctx, quickcheck_loops);
    test_string_length(ctx);
    test_decode_escape_sequences(ctx);
    test_encode_escape_sequences(ctx);
    test_roundtrip(ctx);

    xkb_context_unref(ctx);
    return 0;
}
