/*
 * Copyright © 2019 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"
#include "utils.h"
#include "utils-paths.h"
#include "test/utils-text.h"

static void
test_string_functions(void)
{
    char buffer[10];

    assert(!snprintf_safe(buffer, 0, "foo"));
    assert(!snprintf_safe(buffer, 1, "foo"));
    assert(!snprintf_safe(buffer, 3, "foo"));

    assert(snprintf_safe(buffer, 10, "foo"));
    assert(streq(buffer, "foo"));

    assert(!snprintf_safe(buffer, 10, "%s", "1234567890"));
    assert(snprintf_safe(buffer, 10, "%s", "123456789"));

    assert(streq_null("foo", "foo"));
    assert(!streq_null("foobar", "foo"));
    assert(!streq_null("foobar", NULL));
    assert(!streq_null(NULL, "foobar"));
    assert(streq_null(NULL, NULL));

    const char text[] =
        "123; // abc\n"
        "  // def\n"
        "456 // ghi // jkl\n"
        "// mno\n"
        "//\n"
        "ok; // pqr\n"
        "foo\n";

    char *out;
    out = strip_lines(text, ARRAY_SIZE(text), "//");
    assert_streq_not_null(
        "strip_lines",
        "123; \n"
        "456 \n"
        "ok; \n"
        "foo\n",
        out
    );
    free(out);

    out = uncomment(text, ARRAY_SIZE(text), "//");
    assert_streq_not_null(
        "uncomment",
        "123;  abc\n"
        "   def\n"
        "456  ghi // jkl\n"
        " mno\n"
        "\n"
        "ok;  pqr\n"
        "foo\n",
        out
    );
    free(out);
}

static void
test_path_functions(void)
{
    /* Absolute paths */
    assert(!is_absolute(""));
#ifdef _WIN32
    assert(!is_absolute("path\\test"));
    assert(is_absolute("c:\\test"));
    assert(!is_absolute("c:test"));
    assert(is_absolute("c:\\"));
    assert(is_absolute("c:/"));
    assert(!is_absolute("c:"));
    assert(is_absolute("\\\\foo"));
    assert(is_absolute("\\\\?\\foo"));
    assert(is_absolute("\\\\?\\UNC\\foo"));
    assert(is_absolute("/foo"));
    assert(is_absolute("\\foo"));
#else
    assert(!is_absolute("test/path"));
    assert(is_absolute("/test" ));
    assert(is_absolute("/" ));
#endif
}

int
main(void)
{
    test_init();
    test_string_functions();
    test_path_functions();

    return 0;
}
