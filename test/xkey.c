#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xkbcommon/xkbcommon.h"

static int
test_string(const char *string, xkb_keysym_t expected)
{
    xkb_keysym_t keysym;

    keysym = xkb_keysym_from_name(string);

    fprintf(stderr, "Expected string %s -> %x\n", string, expected);
    fprintf(stderr, "Received string %s -> %x\n\n", string, keysym);

    return keysym == expected;
}

static int
test_keysym(xkb_keysym_t keysym, const char *expected)
{
    char s[16];

    xkb_keysym_get_name(keysym, s, sizeof(s));

    fprintf(stderr, "Expected keysym %#x -> %s\n", keysym, expected);
    fprintf(stderr, "Received keysym %#x -> %s\n\n", keysym, s);

    return strcmp(s, expected) == 0;
}

static int
test_utf8(xkb_keysym_t keysym, const char *expected)
{
    char s[8];
    int ret;

    ret = xkb_keysym_to_utf8(keysym, s, sizeof(s));
    if (ret <= 0)
        return ret;

    fprintf(stderr, "Expected keysym %#x -> %s\n", keysym, expected);
    fprintf(stderr, "Received keysym %#x -> %s\n\n", keysym, s);

    return strcmp(s, expected) == 0;
}

int
main(void)
{
    assert(test_string("Undo", 0xFF65));
    assert(test_string("ThisKeyShouldNotExist", XKB_KEY_NoSymbol));
    assert(test_string("XF86_Switch_VT_5", 0x1008FE05));
    assert(test_string("VoidSymbol", 0xFFFFFF));
    assert(test_string("U4567", 0x1004567));
    assert(test_string("0x10203040", 0x10203040));

    assert(test_keysym(0x1008FF56, "XF86Close"));
    assert(test_keysym(0x0, "NoSymbol"));
    assert(test_keysym(0x1008FE20, "XF86Ungrab"));
    assert(test_keysym(0x01001234, "U1234"));

    assert(test_utf8(XKB_KEY_y, "y"));
    assert(test_utf8(XKB_KEY_u, "u"));
    assert(test_utf8(XKB_KEY_m, "m"));
    assert(test_utf8(XKB_KEY_Cyrillic_em, "м"));
    assert(test_utf8(XKB_KEY_Cyrillic_u, "у"));
    assert(test_utf8(XKB_KEY_exclam, "!"));
    assert(test_utf8(XKB_KEY_oslash, "ø"));
    assert(test_utf8(XKB_KEY_hebrew_aleph, "א"));
    assert(test_utf8(XKB_KEY_Arabic_sheen, "ش"));

    assert(test_utf8(XKB_KEY_space, " "));
    assert(test_utf8(XKB_KEY_KP_Space, " "));
    assert(test_utf8(XKB_KEY_9, "9"));
    assert(test_utf8(XKB_KEY_KP_9, "9"));
    assert(test_utf8(XKB_KEY_KP_Multiply, "*"));
    assert(test_utf8(XKB_KEY_KP_Subtract, "-"));

    return 0;
}
