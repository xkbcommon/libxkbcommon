/*
 * Copyright © 2019 Red Hat, Inc.
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"
#include "utils.h"

int
main(void)
{
    char buffer[10];

    test_init();

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

    return 0;
}
