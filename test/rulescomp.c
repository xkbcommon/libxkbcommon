/*
Copyright 2009 Dan Nicholson

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the authors or their
institutions shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the authors.
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "xkbcommon/xkbcommon.h"

#define BENCHMARK_ITERATIONS 1000

static int
do_test_rmlvo(const char *rules, const char *model, const char *layout,
              const char *variant, const char *options, int silent)
{
    struct xkb_context *context;
    struct xkb_keymap *keymap;
    struct xkb_rule_names rmlvo = {
        .rules = rules,
        .model = model,
        .layout = layout,
        .variant = variant,
        .options = options
    };

    context = xkb_context_new(0);
    assert(context);

    if (!silent)
        fprintf(stderr, "Compiling %s %s %s %s %s\n", rules, model, layout,
                variant, options);

    keymap = xkb_map_new_from_names(context, &rmlvo, 0);
    if (!keymap) {
        xkb_context_unref(context);
        return 0;
    }

    xkb_map_unref(keymap);
    xkb_context_unref(context);
    return 1;
}

static int
test_rmlvo(const char *rules, const char *model, const char *layout,
           const char *variant, const char *options)
{
    return do_test_rmlvo(rules, model, layout, variant, options, 0);
}

static int
test_rmlvo_silent(const char *rules, const char *model, const char *layout,
                  const char *variant, const char *options)
{
    return do_test_rmlvo(rules, model, layout, variant, options, 1);
}

static void
benchmark(void)
{
    struct timespec start, stop, elapsed;
    int i;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (i = 0; i < BENCHMARK_ITERATIONS; i++)
        assert(test_rmlvo_silent("base",       "",       "us",  "",      ""));
    clock_gettime(CLOCK_MONOTONIC, &stop);

    elapsed.tv_sec = stop.tv_sec - start.tv_sec;
    elapsed.tv_nsec = stop.tv_nsec - start.tv_nsec;
    if (elapsed.tv_nsec < 0) {
        elapsed.tv_nsec += 1000000000;
        elapsed.tv_sec--;
    }

    fprintf(stderr, "compiled %d keymaps in %ld.%09lds\n",
            BENCHMARK_ITERATIONS, elapsed.tv_sec, elapsed.tv_nsec);
}

int main(int argc, char *argv[])
{
    assert(test_rmlvo("base",       "pc105",  "us,il,ru,ca",  ",,,multix",      "grp:alts_toggle,ctrl:nocaps,compose:rwin"));
    assert(test_rmlvo("base",       "",       "us",  "",      ""));
    assert(test_rmlvo("evdev",      "pc105",  "us",  "intl",  ""));
    assert(test_rmlvo("evdev",      "pc105",  "us",  "intl",  "grp:alts_toggle"));

    assert(!test_rmlvo("",          "",       "",    "",      ""));
    assert(!test_rmlvo("base",      "",       "",    "",      ""));
    assert(!test_rmlvo("base",      "pc105",  "",    "",      ""));
    assert(!test_rmlvo("badrules",  "",       "us",  "",      ""));

    if (argc > 1 && strcmp(argv[1], "bench") == 0)
        benchmark();
}
