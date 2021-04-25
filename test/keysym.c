/*
 * Copyright © 2009 Dan Nicholson
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

#include <locale.h>

#include "test.h"
#include "keysym.h" /* For unexported is_lower/upper/keypad() */

static int
test_string(const char *string, xkb_keysym_t expected)
{
    xkb_keysym_t keysym;

    keysym = xkb_keysym_from_name(string, 0);

    fprintf(stderr, "Expected string %s -> %x\n", string, expected);
    fprintf(stderr, "Received string %s -> %x\n\n", string, keysym);

    return keysym == expected;
}

static int
test_casestring(const char *string, xkb_keysym_t expected)
{
    xkb_keysym_t keysym;

    keysym = xkb_keysym_from_name(string, XKB_KEYSYM_CASE_INSENSITIVE);

    fprintf(stderr, "Expected casestring %s -> %x\n", string, expected);
    fprintf(stderr, "Received casestring %s -> %x\n\n", string, keysym);

    return keysym == expected;
}

static int
test_keysym(xkb_keysym_t keysym, const char *expected)
{
    char s[16];

    xkb_keysym_get_name(keysym, s, sizeof(s));

    fprintf(stderr, "Expected keysym %#x -> %s\n", keysym, expected);
    fprintf(stderr, "Received keysym %#x -> %s\n\n", keysym, s);

    return streq(s, expected);
}

static int
test_utf8(xkb_keysym_t keysym, const char *expected)
{
    char s[8];
    int ret;

    ret = xkb_keysym_to_utf8(keysym, s, sizeof(s));
    if (ret <= 0)
        return ret;

    assert(expected != NULL);

    fprintf(stderr, "Expected keysym %#x -> %s (%u bytes)\n", keysym, expected,
            (unsigned) strlen(expected));
    fprintf(stderr, "Received keysym %#x -> %s (%u bytes)\n\n", keysym, s,
            (unsigned) strlen(s));

    return streq(s, expected);
}

static void
test_github_issue_42(void)
{
    // Verify we are not dependent on locale, Turkish-i problem in particular.
    if (setlocale(LC_CTYPE, "tr_TR.UTF-8") == NULL) {
        // The locale is not available, probably; skip.
        return;
    }

    assert(test_string("i", XKB_KEY_i));
    assert(test_string("I", XKB_KEY_I));
    assert(test_casestring("i", XKB_KEY_i));
    assert(test_casestring("I", XKB_KEY_i));
    assert(xkb_keysym_to_upper(XKB_KEY_i) == XKB_KEY_I);
    assert(xkb_keysym_to_lower(XKB_KEY_I) == XKB_KEY_i);

    setlocale(LC_CTYPE, "C");
}

static void
get_keysym_name(xkb_keysym_t keysym, char *buffer, size_t size)
{
    int name_length = xkb_keysym_get_name(keysym, buffer, size);
    if (name_length < 0) {
        snprintf(buffer, size, "(unknown: 0x%lx)", (unsigned long)keysym);
    }
}

static int
test_utf32_to_keysym(uint32_t ucs, xkb_keysym_t expected)
{
    char expected_name[64];
    char actual_name[64];
    xkb_keysym_t actual = xkb_utf32_to_keysym(ucs);
    get_keysym_name(expected, expected_name, 64);
    get_keysym_name(actual, actual_name, 64);

    fprintf(stderr, "Code point 0x%lx: expected keysym: %s, actual: %s\n\n",
            (unsigned long)ucs, expected_name, actual_name);
    return expected == actual;
}

int
main(void)
{
    assert(test_string("Undo", 0xFF65));
    assert(test_string("ThisKeyShouldNotExist", XKB_KEY_NoSymbol));
    assert(test_string("XF86_Switch_VT_5", 0x1008FE05));
    assert(test_string("VoidSymbol", 0xFFFFFF));
    assert(test_string("U4567", 0x1004567));
    assert(test_string("U+4567", XKB_KEY_NoSymbol));
    assert(test_string("U+4567ffff", XKB_KEY_NoSymbol));
    assert(test_string("U+4567ffffff", XKB_KEY_NoSymbol));
    assert(test_string("U   4567", XKB_KEY_NoSymbol));
    assert(test_string("U  +4567", XKB_KEY_NoSymbol));
    assert(test_string("0x10203040", 0x10203040));
    assert(test_string("0x102030400", XKB_KEY_NoSymbol));
    assert(test_string("0x010203040", XKB_KEY_NoSymbol));
    assert(test_string("0x+10203040", XKB_KEY_NoSymbol));
    assert(test_string("0x  10203040", XKB_KEY_NoSymbol));
    assert(test_string("0x  +10203040", XKB_KEY_NoSymbol));
    assert(test_string("0x-10203040", XKB_KEY_NoSymbol));
    assert(test_string("a", 0x61));
    assert(test_string("A", 0x41));
    assert(test_string("ch", 0xfea0));
    assert(test_string("Ch", 0xfea1));
    assert(test_string("CH", 0xfea2));
    assert(test_string("THORN", 0x00de));
    assert(test_string("Thorn", 0x00de));
    assert(test_string("thorn", 0x00fe));
    /* Max keysym. */
    assert(test_string("0xffffffff", 0xffffffff));
    /* Outside range. */
    assert(test_string("0x100000000", XKB_KEY_NoSymbol));

    assert(test_keysym(0x1008FF56, "XF86Close"));
    assert(test_keysym(0x0, "NoSymbol"));
    assert(test_keysym(0x1008FE20, "XF86Ungrab"));
    assert(test_keysym(0x01001234, "U1234"));
    /* 16-bit unicode padded to width 4. */
    assert(test_keysym(0x010002DE, "U02DE"));
    /* 32-bit unicode padded to width 8. */
    assert(test_keysym(0x0101F4A9, "U0001F4A9"));

    assert(test_casestring("Undo", 0xFF65));
    assert(test_casestring("UNDO", 0xFF65));
    assert(test_casestring("A", 0x61));
    assert(test_casestring("a", 0x61));
    assert(test_casestring("ThisKeyShouldNotExist", XKB_KEY_NoSymbol));
    assert(test_casestring("XF86_Switch_vT_5", 0x1008FE05));
    assert(test_casestring("xF86_SwitcH_VT_5", 0x1008FE05));
    assert(test_casestring("xF86SwiTch_VT_5", 0x1008FE05));
    assert(test_casestring("xF86Switch_vt_5", 0x1008FE05));
    assert(test_casestring("VoidSymbol", 0xFFFFFF));
    assert(test_casestring("vOIDsymBol", 0xFFFFFF));
    assert(test_casestring("U4567", 0x1004567));
    assert(test_casestring("u4567", 0x1004567));
    assert(test_casestring("0x10203040", 0x10203040));
    assert(test_casestring("0X10203040", 0x10203040));
    assert(test_casestring("THORN", 0x00fe));
    assert(test_casestring("Thorn", 0x00fe));
    assert(test_casestring("thorn", 0x00fe));

    assert(test_string("", XKB_KEY_NoSymbol));
    assert(test_casestring("", XKB_KEY_NoSymbol));

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
    assert(test_utf8(XKB_KEY_BackSpace, "\b"));
    assert(test_utf8(XKB_KEY_Escape, "\033"));
    assert(test_utf8(XKB_KEY_KP_Separator, ","));
    assert(test_utf8(XKB_KEY_KP_Decimal, "."));
    assert(test_utf8(XKB_KEY_Tab, "\t"));
    assert(test_utf8(XKB_KEY_KP_Tab, "\t"));
    assert(test_utf8(XKB_KEY_hyphen, "­"));
    assert(test_utf8(XKB_KEY_Linefeed, "\n"));
    assert(test_utf8(XKB_KEY_Return, "\r"));
    assert(test_utf8(XKB_KEY_KP_Enter, "\r"));
    assert(test_utf8(XKB_KEY_KP_Equal, "="));
    assert(test_utf8(XKB_KEY_9, "9"));
    assert(test_utf8(XKB_KEY_KP_9, "9"));
    assert(test_utf8(XKB_KEY_KP_Multiply, "*"));
    assert(test_utf8(XKB_KEY_KP_Subtract, "-"));

    assert(test_utf8(0x10005d0, "א"));
    assert(test_utf8(0x110ffff, "\xf4\x8f\xbf\xbf"));
    assert(test_utf8(0x1110000, NULL) == 0);

    assert(test_utf32_to_keysym('y', XKB_KEY_y));
    assert(test_utf32_to_keysym('u', XKB_KEY_u));
    assert(test_utf32_to_keysym('m', XKB_KEY_m));
    assert(test_utf32_to_keysym(0x43c, XKB_KEY_Cyrillic_em));
    assert(test_utf32_to_keysym(0x443, XKB_KEY_Cyrillic_u));
    assert(test_utf32_to_keysym('!', XKB_KEY_exclam));
    assert(test_utf32_to_keysym(0xF8, XKB_KEY_oslash));
    assert(test_utf32_to_keysym(0x5D0, XKB_KEY_hebrew_aleph));
    assert(test_utf32_to_keysym(0x634, XKB_KEY_Arabic_sheen));
    assert(test_utf32_to_keysym(0x1F609, 0x0101F609)); // ;) emoji

    assert(test_utf32_to_keysym('\b', XKB_KEY_BackSpace));
    assert(test_utf32_to_keysym('\t', XKB_KEY_Tab));
    assert(test_utf32_to_keysym('\n', XKB_KEY_Linefeed));
    assert(test_utf32_to_keysym(0x0b, XKB_KEY_Clear));
    assert(test_utf32_to_keysym('\r', XKB_KEY_Return));
    assert(test_utf32_to_keysym(0x1b, XKB_KEY_Escape));
    assert(test_utf32_to_keysym(0x7f, XKB_KEY_Delete));

    assert(test_utf32_to_keysym(' ', XKB_KEY_space));
    assert(test_utf32_to_keysym(',', XKB_KEY_comma));
    assert(test_utf32_to_keysym('.', XKB_KEY_period));
    assert(test_utf32_to_keysym('=', XKB_KEY_equal));
    assert(test_utf32_to_keysym('9', XKB_KEY_9));
    assert(test_utf32_to_keysym('*', XKB_KEY_asterisk));
    assert(test_utf32_to_keysym(0xd7, XKB_KEY_multiply));
    assert(test_utf32_to_keysym('-', XKB_KEY_minus));
    assert(test_utf32_to_keysym(0x10fffd, 0x110fffd));

    // Unicode non-characters
    assert(test_utf32_to_keysym(0xfdd0, XKB_KEY_NoSymbol));
    assert(test_utf32_to_keysym(0xfdef, XKB_KEY_NoSymbol));
    assert(test_utf32_to_keysym(0xfffe, XKB_KEY_NoSymbol));
    assert(test_utf32_to_keysym(0xffff, XKB_KEY_NoSymbol));
    assert(test_utf32_to_keysym(0x7fffe, XKB_KEY_NoSymbol));
    assert(test_utf32_to_keysym(0x7ffff, XKB_KEY_NoSymbol));
    assert(test_utf32_to_keysym(0xafffe, XKB_KEY_NoSymbol));
    assert(test_utf32_to_keysym(0xaffff, XKB_KEY_NoSymbol));

    // Codepoints outside the Unicode planes
    assert(test_utf32_to_keysym(0x110000, XKB_KEY_NoSymbol));
    assert(test_utf32_to_keysym(0xdeadbeef, XKB_KEY_NoSymbol));

    assert(xkb_keysym_is_lower(XKB_KEY_a));
    assert(xkb_keysym_is_lower(XKB_KEY_Greek_lambda));
    assert(xkb_keysym_is_lower(xkb_keysym_from_name("U03b1", 0))); /* GREEK SMALL LETTER ALPHA */
    assert(xkb_keysym_is_lower(xkb_keysym_from_name("U03af", 0))); /* GREEK SMALL LETTER IOTA WITH TONOS */

    assert(xkb_keysym_is_upper(XKB_KEY_A));
    assert(xkb_keysym_is_upper(XKB_KEY_Greek_LAMBDA));
    assert(xkb_keysym_is_upper(xkb_keysym_from_name("U0391", 0))); /* GREEK CAPITAL LETTER ALPHA */
    assert(xkb_keysym_is_upper(xkb_keysym_from_name("U0388", 0))); /* GREEK CAPITAL LETTER EPSILON WITH TONOS */

    assert(!xkb_keysym_is_upper(XKB_KEY_a));
    assert(!xkb_keysym_is_lower(XKB_KEY_A));
    assert(!xkb_keysym_is_lower(XKB_KEY_Return));
    assert(!xkb_keysym_is_upper(XKB_KEY_Return));
    assert(!xkb_keysym_is_lower(XKB_KEY_hebrew_aleph));
    assert(!xkb_keysym_is_upper(XKB_KEY_hebrew_aleph));
    assert(!xkb_keysym_is_upper(xkb_keysym_from_name("U05D0", 0))); /* HEBREW LETTER ALEF */
    assert(!xkb_keysym_is_lower(xkb_keysym_from_name("U05D0", 0))); /* HEBREW LETTER ALEF */
    assert(!xkb_keysym_is_lower(XKB_KEY_8));
    assert(!xkb_keysym_is_upper(XKB_KEY_8));

    assert(xkb_keysym_is_keypad(XKB_KEY_KP_Enter));
    assert(xkb_keysym_is_keypad(XKB_KEY_KP_6));
    assert(xkb_keysym_is_keypad(XKB_KEY_KP_Add));
    assert(!xkb_keysym_is_keypad(XKB_KEY_Num_Lock));
    assert(!xkb_keysym_is_keypad(XKB_KEY_1));
    assert(!xkb_keysym_is_keypad(XKB_KEY_Return));

    assert(xkb_keysym_to_upper(XKB_KEY_a) == XKB_KEY_A);
    assert(xkb_keysym_to_upper(XKB_KEY_A) == XKB_KEY_A);
    assert(xkb_keysym_to_lower(XKB_KEY_a) == XKB_KEY_a);
    assert(xkb_keysym_to_lower(XKB_KEY_A) == XKB_KEY_a);
    assert(xkb_keysym_to_upper(XKB_KEY_Return) == XKB_KEY_Return);
    assert(xkb_keysym_to_lower(XKB_KEY_Return) == XKB_KEY_Return);
    assert(xkb_keysym_to_upper(XKB_KEY_Greek_lambda) == XKB_KEY_Greek_LAMBDA);
    assert(xkb_keysym_to_upper(XKB_KEY_Greek_LAMBDA) == XKB_KEY_Greek_LAMBDA);
    assert(xkb_keysym_to_lower(XKB_KEY_Greek_lambda) == XKB_KEY_Greek_lambda);
    assert(xkb_keysym_to_lower(XKB_KEY_Greek_LAMBDA) == XKB_KEY_Greek_lambda);
    assert(xkb_keysym_to_upper(XKB_KEY_eacute) == XKB_KEY_Eacute);
    assert(xkb_keysym_to_lower(XKB_KEY_Eacute) == XKB_KEY_eacute);

    test_github_issue_42();

    return 0;
}
