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

#include "xkbcommon/xkbcommon.h"

static int
test_rmlvo(const char *rules, const char *model, const char *layout,
           const char *variant, const char *options)
{
    struct xkb_context *context;
    struct xkb_keymap *xkb;
    struct xkb_rule_names rmlvo = {
        .rules = rules,
        .model = model,
        .layout = layout,
        .variant = variant,
        .options = options
    };

    context = xkb_context_new();
    assert(context);

    fprintf(stderr, "\nCompiling %s %s %s %s %s\n", rmlvo.rules, rmlvo.model,
           rmlvo.layout, rmlvo.variant, rmlvo.options);

    xkb = xkb_map_new_from_names(context, &rmlvo, 0);
    if (!xkb) {
        xkb_context_unref(context);
        return 0;
    }

    xkb_map_unref(xkb);
    xkb_context_unref(context);
    return 1;
}

int
main(void)
{
    assert(test_rmlvo("base",       "pc105",  "us,il,ru,ca",  ",,,multix",      "grp:alts_toggle,ctrl:nocaps,compose:rwin"));
    assert(test_rmlvo("base",       "",       "us",  "",      ""));
    assert(test_rmlvo("evdev",      "pc105",  "us",  "intl",  ""));
    assert(test_rmlvo("evdev",      "pc105",  "us",  "intl",  "grp:alts_toggle"));

    assert(!test_rmlvo("",          "",       "",    "",      ""));
    assert(!test_rmlvo("base",      "",       "",    "",      ""));
    assert(!test_rmlvo("base",      "pc105",  "",    "",      ""));
    assert(!test_rmlvo("badrules",  "",       "us",  "",      ""));

    return 0;
}
