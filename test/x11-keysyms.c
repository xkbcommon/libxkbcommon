/*
 * Copyright © 2013 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <locale.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
// #include <xcb/xcb.h>
#include <X11/XKBlib.h>

#include "test.h"
#include "src/keysym.h" /* For unexported is_lower/upper/keypad() */
#include "src/utf8.h"
#include "xkbcommon/xkbcommon-keysyms.h"
#include "xvfb-wrapper.h"
#include "test/keysym.h"

static inline bool
is_surrogate(uint32_t cp)
{
    return (cp >= 0xd800 && cp <= 0xdfff);
}

static bool
test_invalid_keysym(Display *conn, uint32_t cp)
{
    fprintf(stderr, "*** Code point: U+%04"PRIX32" ***\n", cp);
    KeySym ks;

    ks = XKB_KEYSYM_UNICODE_OFFSET | cp;

    static_assert(XKB_KEYSYM_UTF8_MAX_SIZE <= XKB_KEYSYM_NAME_MAX_SIZE);
    char buffer[XKB_KEYSYM_NAME_MAX_SIZE] = {0};
    char utf8[5] = {0};
    utf32_to_utf8(cp, utf8);

    /* All Unicode keysyms convert to UTF-8, except U0000.
     * X11 wrongly encodes surrogates. */
    int count = XkbTranslateKeySym(conn, &ks, 0, buffer, sizeof(buffer), NULL);
    assert_printf((count > 0 && (strcmp(buffer, utf8) == 0 ||
                                 (is_surrogate(cp) &&
                                  !is_valid_utf8(buffer, strlen(buffer))))) ^
                  (count == 0 && cp == 0),
                  "Keysym %#lx cannot convert to UTF-8: %#x %#x %#x %#x\n",
                  ks, buffer[0], buffer[1], buffer[2], buffer[3]);

    /* All Unicode keysyms but the Latin-1 code points have a name */
    char *name = XKeysymToString(ks);
    assert_printf((name == NULL) ^ (cp >= 0x100 &&
                                    (!is_surrogate(cp) || name[0] == 'U')),
                  "Unicode keysym %#lx has an unexpected name: %s\n",
                  ks, name);
    if (name && name[0] == 'U') {
        /* Unicode keysyms need to be freed, but the others must not! */
        size_t len = strlen(name);
        unsigned k = 1;
        for (; k < len; k++)
            if (!is_xdigit(name[k]))
                break;
        if (k == len) {
            assert(strtoll(name + 1, NULL, 16) == cp);
            free(name);
        }
    }

    /* Numeric hexadecimal format always works */
    snprintf(buffer, sizeof(buffer), "%#lx", ks);
    KeySym ks2 = XStringToKeysym(buffer);
    assert_printf(ks2 == ks,
                  "Unicode keysym name %s cannot convert "
                  "to keysym: %#lx\n",
                  buffer, ks);

    /*
     * Unicode format
     * • Does not work for control code points
     * • Converts to canonical keysym for other Latin-1 code points
     */
    snprintf(buffer, sizeof(buffer), "U%04"PRIX32, cp);
    ks2 = XStringToKeysym(buffer);
    assert_printf((ks2 == XKB_KEY_NoSymbol && (cp < 0x20 || (cp > 0x7e && cp < 0xa0))) ||
                  (ks2 == cp && cp < 0x100) || (ks2 == ks && cp >= 0x100),
                  "Unicode keysym name %s is illegal, but it converts "
                  "to keysym: %#lx\n",
                  buffer, ks);
}

X11_TEST(test_basic)
{
    int exit_code = EXIT_SUCCESS;

    /*
    * The next two steps depend on a running X server with XKB support.
    * If it fails, it's not necessarily an actual problem with the code.
    * So we don't want a FAIL here.
    */
    int mjr = XkbMajorVersion;
    int mnr = XkbMinorVersion;
    int error = 0;
    Display *conn = XkbOpenDisplay(display, NULL, NULL, &mjr, &mnr, &error);
    if (!conn) {
        exit_code = TEST_SETUP_FAILURE;
        goto err_conn;
    }

    KeySym ks;
    char *name;
    static_assert(XKB_KEYSYM_UTF8_MAX_SIZE <= XKB_KEYSYM_NAME_MAX_SIZE);
    char buffer[XKB_KEYSYM_NAME_MAX_SIZE] = {0};
    for (uint32_t cp = 0; cp <= 0x10ffff; cp++) {
        test_invalid_keysym(conn, cp);
    }

    // ks = 0x1000100;
    // name = XKeysymToString(ks);
    // assert_printf(name == NULL,
    //               "Unicode keysym %#lx has an "
    //               "unexpected name: %s\n",
    //               ks, name);
    // free(name);

    // ks = 0x1000104;
    // name = XKeysymToString(ks);
    // assert_printf(name == NULL,
    //               "Unicode keysym %#lx has an "
    //               "unexpected name: %s\n",
    //               ks, name);
    // free(name);

    // Will segfault
    // name = XKeysymToString(XKB_KEY_space);
    // free(name);

err_conn:
    XCloseDisplay(conn);

    return exit_code;
}

int main(void) {
    test_init();

    setlocale(LC_CTYPE, "C.UTF-8");

    return x11_tests_run();
}
