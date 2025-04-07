/*
 * Copyright © 2009 Dan Nicholson
 * SPDX-License-Identifier: MIT
 */
#include "config.h"
#include "xkbcommon/xkbcommon-keysyms.h"

#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#if HAVE_ICU
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <unicode/utf16.h>
#include "test/keysym-case-mapping.h"
#endif

#include "test.h"
#include "src/keysym.h" /* For unexported is_lower/upper/keypad() */
#include "test/keysym.h"
#include "src/utf8.h"

/* Explicit ordered list of modifier keysyms */
static const xkb_keysym_t modifier_keysyms[] = {
    XKB_KEY_ISO_Lock,
    XKB_KEY_ISO_Level2_Latch,
    XKB_KEY_ISO_Level3_Shift,
    XKB_KEY_ISO_Level3_Latch,
    XKB_KEY_ISO_Level3_Lock,
    /* XKB_KEY_ISO_Group_Shift == XKB_KEY_Mode_switch */
    XKB_KEY_ISO_Group_Latch,
    XKB_KEY_ISO_Group_Lock,
    XKB_KEY_ISO_Next_Group,
    XKB_KEY_ISO_Next_Group_Lock,
    XKB_KEY_ISO_Prev_Group,
    XKB_KEY_ISO_Prev_Group_Lock,
    XKB_KEY_ISO_First_Group,
    XKB_KEY_ISO_First_Group_Lock,
    XKB_KEY_ISO_Last_Group,
    XKB_KEY_ISO_Last_Group_Lock,
    0xfe10, /* Currently unassigned, but xkb_keysym_is_modifier returns true */
    XKB_KEY_ISO_Level5_Shift,
    XKB_KEY_ISO_Level5_Latch,
    XKB_KEY_ISO_Level5_Lock,

    XKB_KEY_Mode_switch,
    XKB_KEY_Num_Lock,

    XKB_KEY_Shift_L,
    XKB_KEY_Shift_R,
    XKB_KEY_Control_L,
    XKB_KEY_Control_R,
    XKB_KEY_Caps_Lock,
    XKB_KEY_Shift_Lock,

    XKB_KEY_Meta_L,
    XKB_KEY_Meta_R,
    XKB_KEY_Alt_L,
    XKB_KEY_Alt_R,
    XKB_KEY_Super_L,
    XKB_KEY_Super_R,
    XKB_KEY_Hyper_L,
    XKB_KEY_Hyper_R
};

#define MIN_MODIFIER_KEYSYM modifier_keysyms[0]
#define MAX_MODIFIER_KEYSYM modifier_keysyms[ARRAY_SIZE(modifier_keysyms) - 1]

static void
test_modifiers_table(void)
{
    xkb_keysym_t ks = XKB_KEY_NoSymbol;

    /* Ensure ordered array */
    for (size_t k = 0; k < ARRAY_SIZE(modifier_keysyms); k++) {
        assert_printf(ks < modifier_keysyms[k],
                      "modifier_keysyms[] is not ordered: 0x%04"PRIx32">=0x%04"PRIx32"\n",
                      ks, modifier_keysyms[k]);
        ks = modifier_keysyms[k];
    }

    /* Unassigned keysym */
    assert(!xkb_keysym_is_assigned(0xfe10));
}

static bool
test_modifier(xkb_keysym_t ks)
{
    if (ks < MIN_MODIFIER_KEYSYM || ks > MAX_MODIFIER_KEYSYM)
        return false;
    for (size_t k = 0; k < ARRAY_SIZE(modifier_keysyms); k++) {
        if (ks == modifier_keysyms[k])
            return true;
    }
    return false;
}

static bool
test_keypad(xkb_keysym_t ks, char *name)
{
    const char prefix[] = "KP_";
    return strncmp(prefix, name, sizeof(prefix) - 1) == 0;
}

static int
test_string(const char *string, xkb_keysym_t expected)
{
    xkb_keysym_t keysym;

    keysym = xkb_keysym_from_name(string, XKB_KEYSYM_NO_FLAGS);

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

static void
test_ambiguous_icase_names(const struct ambiguous_icase_ks_names_entry *entry)
{
    for (int k = 0; k < entry->count; k++) {
        /* Check expected result */
        assert(test_casestring(entry->names[k], entry->keysym));
        /* If the keysym is cased, then check the resulting keysym is lower-cased */
        xkb_keysym_t keysym = xkb_keysym_from_name(entry->names[k],
                                                   XKB_KEYSYM_NO_FLAGS);
        if (xkb_keysym_is_lower(keysym) || xkb_keysym_is_upper_or_title(keysym)) {
            assert(xkb_keysym_is_lower(entry->keysym));
        }
    }
}

static int
test_keysym(xkb_keysym_t keysym, const char *expected)
{
    char s[XKB_KEYSYM_NAME_MAX_SIZE];

    xkb_keysym_get_name(keysym, s, sizeof(s));

    fprintf(stderr, "Expected keysym %#x -> %s\n", keysym, expected);
    fprintf(stderr, "Received keysym %#x -> %s\n\n", keysym, s);

    return streq(s, expected);
}

static bool
test_deprecated(xkb_keysym_t keysym, const char *name,
                bool expected_deprecated, const char *expected_reference)
{
    const char *reference;
    bool deprecated = xkb_keysym_is_deprecated(keysym, name, &reference);

    fprintf(stderr, "Expected keysym %#x -> deprecated: %d, reference: %s\n",
            keysym, expected_deprecated, expected_reference);
    fprintf(stderr, "Received keysym %#x -> deprecated: %d, reference: %s\n",
            keysym, deprecated, reference);

    return deprecated == expected_deprecated &&
           (
            (reference == NULL && expected_reference == NULL) ||
            (reference != NULL && expected_reference != NULL &&
             strcmp(reference, expected_reference) == 0)
           );
}

static int
test_utf8(xkb_keysym_t keysym, const char *expected)
{
    char s[XKB_KEYSYM_UTF8_MAX_SIZE];
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

#if HAVE_ICU

static UVersionInfo xkb_unicode_version = XKB_KEYSYM_UNICODE_VERSION;
static UVersionInfo icu_unicode_version;

static inline int
compare_unicode_version(const UVersionInfo v1, const UVersionInfo v2)
{
    return memcmp(v1,v2,sizeof(UVersionInfo));
}

/* Unicode assertion
 * In case the test fails, do not raise an exception if there is
 * an ICU version mismatch with our Unicode version. */
#define uassert_printf(cp, cond, ...)                                                     \
   if (!(cond)) {                                                                         \
      fprintf(stderr, "Assertion failure: " __VA_ARGS__);                                 \
      UVersionInfo char_age;                                                              \
      u_charAge((UChar32)cp, char_age);                                                   \
      if (compare_unicode_version(char_age, xkb_unicode_version) > 0) {                   \
        fprintf(stderr,                                                                   \
                "[WARNING] ICU version mismatch: "                                        \
                "too recent for code point: "CODE_POINT"\n", cp);                         \
      } else if (compare_unicode_version(icu_unicode_version, xkb_unicode_version) < 0) { \
        fprintf(stderr,                                                                   \
                "[WARNING] ICU version mismatch: "                                        \
                "too old for code point: "CODE_POINT"\n", cp);                            \
      } else {                                                                            \
        assert(cond);                                                                     \
      }                                                                                   \
   }

#define KEYSYM     "0x%04"PRIx32
#define CODE_POINT "U+%04"PRIX32

static void
test_icu_case_mappings(xkb_keysym_t ks)
{
    uint32_t cp = xkb_keysym_to_utf32(ks);

    /* Check predicates */
    bool is_lower = xkb_keysym_is_lower(ks);
    uint32_t expected = !!u_isULowercase((UChar32) cp);
    uassert_printf(cp, is_lower == expected,
                   "Invalid xkb_keysym_is_lower("KEYSYM") ("CODE_POINT"): "
                   "expected %d, got: %d\n",
                   ks, cp, expected, is_lower);
    bool is_upper_or_title = xkb_keysym_is_upper_or_title(ks);
    expected = !!(u_isUUppercase((UChar32) cp) || u_istitle((UChar32) cp));
    uassert_printf(cp, is_upper_or_title == expected,
                   "Invalid xkb_keysym_is_upper_or_title("KEYSYM") ("CODE_POINT"): "
                   "expected %d, got: %d\n",
                   ks, cp, expected, is_upper_or_title);
    assert(is_lower != is_upper_or_title || !is_lower);

    /* Check lower case mapping */
    xkb_keysym_t ks_mapped = xkb_keysym_to_lower(ks);
    expected = to_simple_lower(cp);
    if (u_istitle((UChar32) cp)) {
        /* Check that title case letter have simple lower case mappings */
        uassert_printf(cp, ks_mapped != ks && expected != cp,
                       "Invalid title case lower transformation. "
                       "Expected keysym: "KEYSYM" != "KEYSYM" "
                       "and code point "CODE_POINT" != "CODE_POINT"\n",
                       ks_mapped, ks, expected, cp);
    }
    if (ks_mapped && ks_mapped != ks) {
        /* Given keysym has been transformed to lower-case */
        uint32_t cp_mapped = xkb_keysym_to_utf32(ks_mapped);
        uint32_t got = cp_mapped;
        uassert_printf(cp, got == expected,
                       "Invalid xkb_keysym_to_lower("KEYSYM") == "KEYSYM": "
                       "expected "CODE_POINT", got: "CODE_POINT"\n",
                       ks, ks_mapped, expected, got);
        uassert_printf(cp, is_upper_or_title,
                       "Expected upper case for keysym "KEYSYM" ("CODE_POINT")\n",
                       ks, cp);
        got = !!xkb_keysym_is_lower(ks_mapped);
        expected = !!u_isULowercase((UChar32) cp_mapped);
        uassert_printf(cp_mapped, got == expected,
                       "Invalid xkb_keysym_is_lower("KEYSYM") ("CODE_POINT"): "
                       "expected %d, got: %d (tested keysym: "KEYSYM")\n",
                       ks_mapped, cp_mapped, expected, got, ks);
    } else if (expected != cp) {
        /* Missing case mapping; the corresponding predicate must be consistent. */
        fprintf(stderr,
                "[WARNING] Missing lower case mapping for "KEYSYM": "
                "expected "CODE_POINT", got: "CODE_POINT"\n",
                ks, expected, cp);
        uassert_printf(cp, !xkb_keysym_is_upper_or_title(ks),
                       "Invalid xkb_keysym_is_upper_or_title("KEYSYM") ("CODE_POINT"): "
                       "expected false, got: true\n",
                       ks, cp);
    }

    /* Check upper case mapping */
    ks_mapped = xkb_keysym_to_upper(ks);
    expected = to_simple_upper(cp);
    if (u_istitle((UChar32) cp)) {
        /* Check title case upper mapping; may be:
         * • simple: 1 code point, or
         * • special: muliple code points */
        UChar cp_string[2] = {0};
        UChar cp_expected_string[8] = {0};
        UBool isError = false;
        int32_t offset = 0;
        /* Convert code point to UTF-16 string */
        U16_APPEND(cp_string, offset, (int32_t) ARRAY_SIZE(cp_string), cp, isError);
        UErrorCode pErrorCode = U_ZERO_ERROR;
        /* Unicode full upper mapping */
        int32_t length = u_strToUpper(cp_expected_string,
                                      ARRAY_SIZE(cp_expected_string),
                                      cp_string, offset, "C", &pErrorCode);
        length = u_countChar32(cp_expected_string, length);
        if (length == 1) {
            /* Simple upper case mapping: one-to-one. */
            uint32_t cp_mapped = xkb_keysym_to_utf32(ks_mapped);
            uassert_printf(cp,
                           !isError && pErrorCode == U_ZERO_ERROR &&
                           ks_mapped != ks && expected != cp &&
                           u_isUUppercase(cp_mapped),
                           "Invalid title case simple upper transformation. "
                           "Expected keysym: "KEYSYM" != "KEYSYM" "
                           "and code point "CODE_POINT" != "CODE_POINT"\n",
                           ks_mapped, ks, expected, cp);
        } else {
            /* Special upper case mapping: maps to multiple code points.
             * We do not handle those, so our mapping is the original. */
            uassert_printf(cp,
                           !isError && pErrorCode == U_ZERO_ERROR &&
                           ks_mapped == ks && expected == cp && length > 1,
                           "Invalid title case special upper transformation. "
                           "Expected keysym: "KEYSYM" == "KEYSYM" "
                           "and code point "CODE_POINT" == "CODE_POINT"\n",
                           ks_mapped, ks, expected, cp);
        }
    }
    if (ks_mapped && ks_mapped != ks) {
        /* Given keysym has been transformed to upper-case */
        uint32_t cp_mapped = xkb_keysym_to_utf32(ks_mapped);
        uint32_t got = cp_mapped;
        uassert_printf(cp, got == expected,
                       "Invalid xkb_keysym_to_upper("KEYSYM") == "KEYSYM": "
                       "expected "CODE_POINT", got: "CODE_POINT"\n",
                       ks, ks_mapped, expected, got);
        uassert_printf(cp, is_lower || u_istitle(cp),
                       "Expected lower or title case for keysym "KEYSYM" ("CODE_POINT")\n",
                       ks, cp);
        got = !!xkb_keysym_is_upper_or_title(ks_mapped);
        expected = !!(u_isUUppercase((UChar32)cp_mapped) || u_istitle((UChar32)cp_mapped));
        uassert_printf(cp_mapped, got == expected,
                       "Invalid xkb_keysym_is_upper_or_title("KEYSYM") ("CODE_POINT"): "
                       "expected %d, got: %d (tested keysym: "KEYSYM")\n",
                       ks_mapped, cp_mapped, expected, got, ks);
    } else if (expected != cp) {
        /* Missing case mapping; the corresponding predicate must be consistent. */
        fprintf(stderr,
                "[WARNING] Missing upper case mapping for "KEYSYM": "
                "expected "CODE_POINT", got: "CODE_POINT"\n",
                ks, expected, cp);
        uassert_printf(cp, !xkb_keysym_is_lower(ks),
                       "Invalid xkb_keysym_is_lower("KEYSYM") ("CODE_POINT"): "
                       "expected false, got: true\n",
                       ks, cp);
    }
}
#endif

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
    char expected_name[XKB_KEYSYM_NAME_MAX_SIZE];
    char actual_name[XKB_KEYSYM_NAME_MAX_SIZE];
    xkb_keysym_t actual = xkb_utf32_to_keysym(ucs);
    get_keysym_name(expected, expected_name, XKB_KEYSYM_NAME_MAX_SIZE);
    get_keysym_name(actual, actual_name, XKB_KEYSYM_NAME_MAX_SIZE);

    fprintf(stderr, "Code point 0x%lx: expected keysym: %s, actual: %s\n\n",
            (unsigned long)ucs, expected_name, actual_name);
    return expected == actual;
}

static inline bool
is_surrogate(uint32_t cp)
{
    return (cp >= 0xd800 && cp <= 0xdfff);
}

/*
 * Unicode handling consistency
 * We should be able to roundtrip with the Unicode keysyms most of the time.
 * Keysyms that are illegal with one function should be illegal in all functions,
 * unless we *explictly* tolerate it.
 */
static void
test_unicode_consistency(void)
{
    /* Latin-1 code points should not be converted to
     * cp + XKB_KEYSYM_UNICODE_OFFSET, as per specification in the keysym
     * header. */
    xkb_keysym_t ks;
    static_assert(XKB_KEYSYM_UTF8_MAX_SIZE <= XKB_KEYSYM_NAME_MAX_SIZE);
    char buffer[XKB_KEYSYM_NAME_MAX_SIZE] = {0};
    for (uint32_t cp = 0; cp < 0x10ffff; cp++) {
        char utf8[5] = {0};
        utf32_to_utf8(cp, utf8);

        ks = xkb_utf32_to_keysym(cp);
        if (ks == 0) {
            /* NULL or surrogates */
            /* Or non-character!? */
            assert_printf(cp == 0 || is_surrogate(cp) ||
                          /* FIXME */
                          (cp >= 0xfdd0 && cp <= 0xfdef) ||
                          cp > 0x10ffff || (cp & 0xfffe) == 0xfffe,
                          "Unexpected NoSymbol for code point U+%#"PRIX32"\n",
                          cp);
        } else if (ks < 0x100) {
            /* 1:1 mapping */
            assert_printf(ks == cp && xkb_keysym_to_utf32(ks) == cp,
                          "Expected keysym %#"PRIx32" for "
                          "Unicode code point U+%04"PRIX32", "
                          "but got: %#"PRIx32"\n",
                          cp, cp, ks);
        } else if (cp < 0x100 && ks <= 0xffff) {
            /* Special keysyms */
            xkb_keysym_t expected = (cp == 0x7f) ? XKB_KEY_Delete : (cp | 0xff00);
            assert_printf(ks == expected && xkb_keysym_to_utf32(ks) == cp,
                          "Expected keysym %#"PRIx32" for "
                          "Unicode code point U+%04"PRIX32", "
                          "but got: %#"PRIx32"\n",
                          expected, cp, ks);
        } else {
            /* Named keysym or Unicode keysym */
            xkb_keysym_t expected = cp + XKB_KEYSYM_UNICODE_OFFSET;
            assert_printf((ks < 0xffff || ks == expected) &&
                          xkb_keysym_to_utf32(ks) == cp,
                          "Expected keysym %#"PRIx32" for "
                          "Unicode code point U+%04"PRIX32", "
                          "but got: %#"PRIx32"\n",
                          expected, cp, ks);
        }

        ks = XKB_KEYSYM_UNICODE_OFFSET + cp;

        /* Convert all Unicode keysyms to their code point, except for surrogates */
        const uint32_t cp2 = xkb_keysym_to_utf32(ks);
        assert_printf((cp2 == cp) ^ (cp2 == 0 && is_surrogate(cp)),
                      "Expected keysym %#"PRIx32" to convert to code point "
                      "U+%04"PRIX32", but got: U+%04"PRIX32"\n",
                      ks, cp, cp2);

        /* Convert all Unicode keysyms to the UTF-8 encoding of their code point,
         * except for U0000 and surrogates */
        int count = xkb_keysym_to_utf8(ks, buffer, sizeof(buffer));
        assert_printf((count == 0 && (cp == 0 || is_surrogate(cp))) ^
                      (count > 0 && strcmp(buffer, utf8) == 0),
                      "Unexpected failure of UTF-8 encoding for keysym "
                      "%#"PRIx32"\n",
                      ks);

        /* Unicode keysyms always have a name */
        count = xkb_keysym_get_name(ks, buffer, sizeof(buffer));
        if (cp < 0x100) {
            /* 0xNNNN notation */
            assert_printf(count > 2 && buffer[0] == '0' && buffer[1] == 'x' &&
                          strtoll(buffer, NULL, 16) == ks,
                          "Unexpected name for Unicode keysym %#"PRIx32": %s\n",
                          ks, buffer);
        } else {
            /* Predefined name or Unnnn notation */
            char *end = NULL;
            assert_printf(count > 1 && (buffer[0] != 'U' ||
                          (strtoll(buffer + 1, &end, 16) == cp) || *end != '\0'),
                          "Unexpected name for Unicode keysym %#"PRIx32": %s\n",
                          ks, buffer);
        }

        /* Numeric hexadecimal format always work */
        snprintf(buffer, sizeof(buffer), "%#"PRIx32, ks);
        ks = xkb_keysym_from_name(buffer, 0);
        xkb_keysym_t expected = cp + XKB_KEYSYM_UNICODE_OFFSET;
        assert_printf((cp == 0 && ks == XKB_KEY_NoSymbol) ^ (ks == expected),
                      "Unexpected failure of numeric notation %s; "
                      "got keysym: %#"PRIx32"\n",
                      buffer, ks);

        /* Unicode notation does not work for control code points */
        snprintf(buffer, sizeof(buffer), "U%04"PRIX32, cp);
        ks = xkb_keysym_from_name(buffer, 0);
        if (cp < 0x20 || (cp > 0x7e && cp < 0xa0)) {
            /* Control code points */
            assert_printf(ks == XKB_KEY_NoSymbol,
                          "Unexpected success of Unicode notation %s; "
                          "got keysym: %#"PRIx32"\n",
                          buffer, ks);
        } else if (cp < 0x100) {
            /* Direct mapping */
            assert_printf(ks == cp,
                          "Expected 1:1 mapping for Unicode notation %s, "
                          "but got keysym: %#"PRIx32"\n",
                          buffer, ks);
        } else {
            assert_printf(ks == cp + XKB_KEYSYM_UNICODE_OFFSET,
                          "Unexpected keysym from Unicode notation %s: "
                          "%#"PRIx32"\n",
                          buffer, ks);
        }
    }
}

int
main(void)
{
    test_init();
#if HAVE_ICU
    u_getUnicodeVersion(icu_unicode_version);
#endif

    /* Bounds */
    assert(XKB_KEYSYM_MIN == 0);
    assert(XKB_KEYSYM_MIN < XKB_KEYSYM_MAX);
    assert(XKB_KEYSYM_MAX <= UINT32_MAX); /* Ensure it fits in xkb_keysym_t */
    assert(XKB_KEYSYM_MAX <= INT32_MAX); /* Ensure correct cast to int32_t */
    assert(XKB_KEYSYM_MIN_ASSIGNED == XKB_KEYSYM_MIN);
    assert(XKB_KEYSYM_MIN_ASSIGNED < XKB_KEYSYM_MAX_ASSIGNED);
    assert(XKB_KEYSYM_MAX_ASSIGNED <= XKB_KEYSYM_MAX);
    assert(XKB_KEYSYM_MIN_EXPLICIT == XKB_KEYSYM_MIN_ASSIGNED);
    assert(XKB_KEYSYM_MIN_EXPLICIT < XKB_KEYSYM_MAX_EXPLICIT);
    assert(XKB_KEYSYM_MAX_EXPLICIT <= XKB_KEYSYM_MAX_ASSIGNED);
    assert(XKB_KEYSYM_COUNT_EXPLICIT <= XKB_KEYSYM_MAX_EXPLICIT - XKB_KEYSYM_MIN_EXPLICIT + 1);
    assert(XKB_KEYSYM_UNICODE_MIN >= XKB_KEYSYM_MIN_EXPLICIT);
    assert(XKB_KEYSYM_UNICODE_MIN < XKB_KEYSYM_UNICODE_MAX);
    assert(XKB_KEYSYM_UNICODE_MAX <= XKB_KEYSYM_MAX_EXPLICIT);

    /* Assigned keysyms */
    assert(xkb_keysym_is_assigned(XKB_KEYSYM_MIN));
    assert(xkb_keysym_is_assigned(XKB_KEYSYM_MIN_ASSIGNED));
    assert(xkb_keysym_is_assigned(XKB_KEY_space));
    assert(xkb_keysym_is_assigned(XKB_KEY_nobreakspace));
    assert(xkb_keysym_is_assigned(XKB_KEY_Aogonek));
    assert(xkb_keysym_is_assigned(XKB_KEY_Hstroke));
    assert(xkb_keysym_is_assigned(XKB_KEY_kra));
    assert(xkb_keysym_is_assigned(XKB_KEY_braille_dot_1));
    assert(xkb_keysym_is_assigned(XKB_KEY_XF86KbdLcdMenu5));
    assert(xkb_keysym_is_assigned(XKB_KEY_Shift_L));
    assert(xkb_keysym_is_assigned(XKB_KEY_XF86MonBrightnessUp));
    assert(xkb_keysym_is_assigned(XKB_KEY_VoidSymbol));
    assert(xkb_keysym_is_assigned(XKB_KEYSYM_UNICODE_MIN));
    assert(xkb_keysym_is_assigned((XKB_KEYSYM_UNICODE_MIN + XKB_KEYSYM_UNICODE_MAX) / 2));
    assert(xkb_keysym_is_assigned(XKB_KEYSYM_UNICODE_MAX));
    assert(xkb_keysym_is_assigned(XKB_KEYSYM_MAX_ASSIGNED));
    assert(!xkb_keysym_is_assigned(XKB_KEYSYM_MAX));

    test_modifiers_table();
    test_unicode_consistency();

    struct xkb_keysym_iterator *iter = xkb_keysym_iterator_new(false);
    xkb_keysym_t ks_prev = XKB_KEYSYM_MIN;
    uint32_t count = 0;
    uint32_t count_non_unicode = 0;
    while (xkb_keysym_iterator_next(iter)) {
        count++;
        xkb_keysym_t ks = xkb_keysym_iterator_get_keysym(iter);
        if (ks < XKB_KEYSYM_UNICODE_MIN || ks > XKB_KEYSYM_UNICODE_MAX)
            count_non_unicode++;
        assert(ks > ks_prev || count == 1);
        ks_prev = ks;
        /* Check assigned keysyms bounds */
        assert((int32_t)XKB_KEYSYM_MIN_ASSIGNED <= (int32_t)ks && ks <= XKB_KEYSYM_MAX_ASSIGNED);
        /* Check utf8 */
        /* Older implementation required 7 bytes for old UTF-8 (see RFC 2279) */
        char utf8[7];
        int needed = xkb_keysym_to_utf8(ks, utf8, sizeof(utf8));
        assert(0 <= needed && needed <= XKB_KEYSYM_UTF8_MAX_SIZE);
        /* Check maximum name length (`needed` does not include the ending NULL) */
        char name[XKB_KEYSYM_NAME_MAX_SIZE];
        needed = xkb_keysym_iterator_get_name(iter, name, sizeof(name));
        assert(0 < needed && (size_t)needed <= sizeof(name) - 1);
        /* Test modifier keysyms */
        bool expected = test_modifier(ks);
        bool got = xkb_keysym_is_modifier(ks);
        assert_printf(got == expected,
                      "xkb_keysym_is_modifier(0x%04"PRIx32"): expected %d, got: %d\n",
                      ks, expected, got);
        /* Test keypad keysyms */
        expected = test_keypad(ks, name);
        got = xkb_keysym_is_keypad(ks);
        assert_printf(got == expected,
                      "xkb_keysym_is_keypad(0x%04"PRIx32") \"%s\": "
                      "expected %d, got: %d\n",
                      ks, name, expected, got);
#if HAVE_ICU
        /* Check case mappings */
        test_icu_case_mappings(ks);
#endif
    }
    xkb_keysym_iterator_unref(iter);
    assert(ks_prev == XKB_KEYSYM_MAX_ASSIGNED);
    assert(count == XKB_KEYSYM_UNICODE_MAX - XKB_KEYSYM_UNICODE_MIN + 1 + count_non_unicode);

    /* Named keysyms */
    assert(test_string("NoSymbol", XKB_KEY_NoSymbol));
    assert(test_string("Undo", 0xFF65));
    assert(test_string("UNDO", XKB_KEY_NoSymbol)); /* Require XKB_KEYSYM_CASE_INSENSITIVE */
    assert(test_string("ThisKeyShouldNotExist", XKB_KEY_NoSymbol));
    assert(test_string("XF86_Switch_VT_5", 0x1008FE05));
    assert(test_string("VoidSymbol", 0xFFFFFF));
    assert(test_string("0", 0x30));
    assert(test_string("9", 0x39));
    assert(test_string("a", 0x61));
    assert(test_string("A", 0x41));
    assert(test_string("ch", 0xfea0));
    assert(test_string("Ch", 0xfea1));
    assert(test_string("CH", 0xfea2));
    assert(test_string("THORN", 0x00de));
    assert(test_string("Thorn", 0x00de));
    assert(test_string("thorn", 0x00fe));
    assert(test_string(" thorn", XKB_KEY_NoSymbol));
    assert(test_string("thorn ", XKB_KEY_NoSymbol));
#define LONGEST_NAME STRINGIFY2(XKB_KEYSYM_LONGEST_NAME)
#define XKB_KEY_LONGEST_NAME CONCAT2(XKB_KEY_, XKB_KEYSYM_LONGEST_NAME)
    assert(test_string(LONGEST_NAME, XKB_KEY_LONGEST_NAME));
#define LONGEST_CANONICAL_NAME STRINGIFY2(XKB_KEYSYM_LONGEST_CANONICAL_NAME)
#define XKB_KEY_LONGEST_CANONICAL_NAME CONCAT2(XKB_KEY_, XKB_KEYSYM_LONGEST_CANONICAL_NAME)
    assert(test_string(LONGEST_CANONICAL_NAME, XKB_KEY_LONGEST_CANONICAL_NAME));

    /* Decimal keysyms are not supported (digits are special cases) */
    assert(test_string("-1", XKB_KEY_NoSymbol));
    assert(test_string("10", XKB_KEY_NoSymbol));
    assert(test_string("010", XKB_KEY_NoSymbol));
    assert(test_string("4567", XKB_KEY_NoSymbol));

    /* Unicode: test various ranges */
    assert(test_string("U0000", XKB_KEY_NoSymbol)); /* Min Unicode */
    assert(test_string("U001f", XKB_KEY_NoSymbol));
    assert(test_string("U0020", 0x0000020));
    assert(test_string("U007E", 0x000007e));
    assert(test_string("U007f", XKB_KEY_NoSymbol));
    assert(test_string("U009f", XKB_KEY_NoSymbol));
    assert(test_string("U00a0", 0x00000a0));
    assert(test_string("U00ff", 0x00000ff));
    assert(test_string("U0100", XKB_KEYSYM_UNICODE_MIN));
    assert(test_string("U4567", 0x1004567));
    assert(test_string("U1F4A9", 0x0101F4A9));
    assert(test_string("U10FFFF", XKB_KEYSYM_UNICODE_MAX)); /* Max Unicode */
    assert(test_string("U110000", XKB_KEY_NoSymbol));
    /* Unicode: test syntax */
    assert(test_string("U00004567", 0x1004567));         /* OK:  8 digits */
    assert(test_string("U000004567", XKB_KEY_NoSymbol)); /* ERR: 9 digits */
    assert(test_string("U+4567", XKB_KEY_NoSymbol));     /* ERR: Standard Unicode notation */
    assert(test_string("U+4567ffff", XKB_KEY_NoSymbol));
    assert(test_string("U+4567ffffff", XKB_KEY_NoSymbol));
    assert(test_string("U-456", XKB_KEY_NoSymbol)); /* No negative number */
    assert(test_string("U456w", XKB_KEY_NoSymbol)); /* Not hexadecimal digit */
    assert(test_string("U4567   ", XKB_KEY_NoSymbol));
    assert(test_string("   U4567", XKB_KEY_NoSymbol));
    assert(test_string("U   4567", XKB_KEY_NoSymbol));
    assert(test_string("U  +4567", XKB_KEY_NoSymbol));
    assert(test_string("u4567", XKB_KEY_NoSymbol)); /* Require XKB_KEYSYM_CASE_INSENSITIVE */

    /* Hexadecimal: test ranges */
    assert(test_string(STRINGIFY2(XKB_KEYSYM_MIN), XKB_KEY_NoSymbol)); /* Min keysym. */
    assert(test_string("0x1", 0x00000001));
    assert(test_string("0x01234567", 0x01234567));
    assert(test_string("0x09abcdef", 0x09abcdef));
    assert(test_string("0x01000100", XKB_KEYSYM_UNICODE_MIN)); /* Min Unicode. */
    assert(test_string("0x0110ffff", XKB_KEYSYM_UNICODE_MAX)); /* Max Unicode. */
    assert(test_string(STRINGIFY2(XKB_KEYSYM_MAX), XKB_KEYSYM_MAX)); /* Max keysym. */
    assert(test_string("0x20000000", XKB_KEY_NoSymbol));
    assert(test_string("0xffffffff", XKB_KEY_NoSymbol));
    assert(test_string("0x100000000", XKB_KEY_NoSymbol));
    /* Hexadecimal: test syntax */
    assert(test_string("0x10203040", 0x10203040));        /* OK:  8 digits */
    assert(test_string("0x102030400", XKB_KEY_NoSymbol)); /* ERR: 9 digits */
    assert(test_string("0x01020304", 0x1020304));         /* OK:  8 digits, starts with 0 */
    assert(test_string("0x010203040", XKB_KEY_NoSymbol)); /* ERR: 9 digits, starts with 0 */
    assert(test_string("0x+10203040", XKB_KEY_NoSymbol));
    assert(test_string("0x01020304w", XKB_KEY_NoSymbol)); /* Not hexadecimal digit */
    assert(test_string("0x102030  ", XKB_KEY_NoSymbol));
    assert(test_string("0x  102030", XKB_KEY_NoSymbol));
    assert(test_string("  0x102030", XKB_KEY_NoSymbol));
    assert(test_string("0x  +10203040", XKB_KEY_NoSymbol));
    assert(test_string("0x-10203040", XKB_KEY_NoSymbol));
    assert(test_string("0X10203040", XKB_KEY_NoSymbol)); /* Require XKB_KEYSYM_CASE_INSENSITIVE */
    assert(test_string("10203040", XKB_KEY_NoSymbol)); /* Missing prefix/decimal not implemented */
    assert(test_string("0b0101", XKB_KEY_NoSymbol)); /* Wrong prefix: binary not implemented */
    assert(test_string("0o0701", XKB_KEY_NoSymbol)); /* Wrong prefix: octal not implemented */

    assert(test_keysym(0x1008FF56, "XF86Close"));
    assert(test_keysym(0x0, "NoSymbol"));
    assert(test_keysym(0x1008FE20, "XF86Ungrab"));
    assert(test_keysym(XKB_KEYSYM_UNICODE_OFFSET, "0x01000000"));
    /* Longest names */
    assert(test_keysym(XKB_KEY_LONGEST_NAME, LONGEST_NAME));
    assert(test_keysym(XKB_KEY_LONGEST_CANONICAL_NAME, LONGEST_CANONICAL_NAME));
    /* Canonical names */
    assert(test_keysym(XKB_KEY_Henkan, "Henkan_Mode"));
    assert(test_keysym(XKB_KEY_ISO_Group_Shift, "Mode_switch"));
    assert(test_keysym(XKB_KEY_dead_perispomeni, "dead_tilde"));
    assert(test_keysym(XKB_KEY_guillemetleft, "guillemotleft"));
    assert(test_keysym(XKB_KEY_ordmasculine, "masculine"));
    assert(test_keysym(XKB_KEY_Greek_lambda, "Greek_lamda"));
    /* Min Unicode */
    assert(test_keysym(XKB_KEYSYM_UNICODE_MIN, "U0100"));
    assert(test_keysym(0x01001234, "U1234"));
    /* 16-bit unicode padded to width 4. */
    assert(test_keysym(0x010002DE, "U02DE"));
    /* 32-bit unicode padded to width 8. */
    assert(test_keysym(0x0101F4A9, "U0001F4A9"));
    /* Max Unicode */
    assert(test_keysym(XKB_KEYSYM_UNICODE_MAX, "U0010FFFF"));
    /* Max Unicode + 1 */
    assert(test_keysym(0x01110000, "0x01110000"));
    /* Min keysym. */
    assert(test_keysym(XKB_KEYSYM_MIN, "NoSymbol"));
    /* Max keysym. */
    assert(test_keysym(XKB_KEYSYM_MAX, STRINGIFY2(XKB_KEYSYM_MAX)));
    /* Outside range. */
    assert(test_keysym(XKB_KEYSYM_MAX + 1, "Invalid"));
    assert(test_keysym(0xffffffff, "Invalid"));

    /* Name is assumed to be correct but we provide garbage */
    const char garbage_name[] = "bla bla bla";
    assert(test_deprecated(XKB_KEY_NoSymbol, NULL, false, NULL));
    assert(test_deprecated(XKB_KEY_NoSymbol, "NoSymbol", false, NULL));
    assert(test_deprecated(XKB_KEY_A, "A", false, NULL));
    assert(test_deprecated(XKB_KEY_A, NULL, false, NULL));
    assert(test_deprecated(XKB_KEY_A, garbage_name, false, NULL));
    assert(test_deprecated(XKB_KEY_ETH, "ETH", false, "ETH"));
    assert(test_deprecated(XKB_KEY_ETH, "Eth", true, "ETH"));
    assert(test_deprecated(XKB_KEY_ETH, garbage_name, true, "ETH"));
    assert(test_deprecated(XKB_KEY_topleftradical, NULL, true, NULL));
    assert(test_deprecated(XKB_KEY_topleftradical, "topleftradical", true, NULL));
    assert(test_deprecated(XKB_KEY_topleftradical, garbage_name, true, NULL));
    /* Mixed deprecated and not deprecated aliases */
    assert(test_deprecated(XKB_KEY_Mode_switch, NULL, false, "Mode_switch"));
    assert(test_deprecated(XKB_KEY_Mode_switch, "Mode_switch", false, "Mode_switch"));
    assert(test_deprecated(XKB_KEY_Mode_switch, garbage_name, false, "Mode_switch"));
    assert(test_deprecated(XKB_KEY_ISO_Group_Shift, NULL, false, "Mode_switch"));
    assert(test_deprecated(XKB_KEY_ISO_Group_Shift, "ISO_Group_Shift", false, "Mode_switch"));
    assert(test_deprecated(XKB_KEY_ISO_Group_Shift, garbage_name, false, "Mode_switch"));
    assert(test_deprecated(XKB_KEY_SunAltGraph, NULL, false, "Mode_switch"));
    assert(test_deprecated(XKB_KEY_SunAltGraph, "SunAltGraph", true, "Mode_switch"));
    assert(test_deprecated(XKB_KEY_SunAltGraph, garbage_name, false, "Mode_switch"));
    /* Unicode is never deprecated */
    assert(test_deprecated(0x0100250C, "U250C", false, NULL));
    assert(test_deprecated(0x0100250C, "0x0100250C", false, NULL));
    assert(test_deprecated(XKB_KEYSYM_MAX, NULL, false, NULL));
    assert(test_deprecated(XKB_KEYSYM_MAX, NULL, false, NULL));
    /* Invalid keysym */
    assert(test_deprecated(0xffffffff, NULL, false, NULL));
    assert(test_deprecated(0xffffffff, NULL, false, NULL));

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

    for (size_t k = 0; k < ARRAY_SIZE(ambiguous_icase_ks_names); k++) {
        test_ambiguous_icase_names(&ambiguous_icase_ks_names[k]);
    }

    assert(test_string("", XKB_KEY_NoSymbol));
    assert(test_casestring("", XKB_KEY_NoSymbol));

    /* Latin-1 keysyms (1:1 mapping in UTF-32) */
    assert(test_utf8(0x0020, "\x20"));
    assert(test_utf8(0x007e, "\x7e"));
    assert(test_utf8(0x00a0, "\xc2\xa0"));
    assert(test_utf8(0x00ff, "\xc3\xbf"));

    assert(test_utf8(XKB_KEY_y, "y"));
    assert(test_utf8(XKB_KEY_u, "u"));
    assert(test_utf8(XKB_KEY_m, "m"));
    assert(test_utf8(XKB_KEY_Cyrillic_em, "м"));
    assert(test_utf8(XKB_KEY_Cyrillic_u, "у"));
    assert(test_utf8(XKB_KEY_exclam, "!"));
    assert(test_utf8(XKB_KEY_oslash, "ø"));
    assert(test_utf8(XKB_KEY_hebrew_aleph, "א"));
    assert(test_utf8(XKB_KEY_Arabic_sheen, "ش"));

    /* Keysyms with special handling */
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

    /* Unicode keysyms */
    assert(test_utf8(XKB_KEYSYM_UNICODE_OFFSET, NULL) == 0); /* Min Unicode codepoint */
    assert(test_utf8(0x1000001, "\x01"));     /* Currently accepted, but not intended (< 0x100100) */
    assert(test_utf8(0x1000020, " "));        /* Currently accepted, but not intended (< 0x100100) */
    assert(test_utf8(0x100007f, "\x7f"));     /* Currently accepted, but not intended (< 0x100100) */
    assert(test_utf8(0x10000a0, "\xc2\xa0")); /* Currently accepted, but not intended (< 0x100100) */
    assert(test_utf8(XKB_KEYSYM_UNICODE_MIN, "Ā")); /* Min Unicode keysym */
    assert(test_utf8(0x10005d0, "א"));
    assert(test_utf8(XKB_KEYSYM_UNICODE_MAX, "\xf4\x8f\xbf\xbf")); /* Max Unicode */
    assert(test_utf8(0x0100d800, NULL) == 0); // Unicode surrogates
    assert(test_utf8(0x0100dfff, NULL) == 0); // Unicode surrogates
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

    // FIXME assert(test_utf32_to_keysym('\0', XKB_KEY_NoSymbol));
    // FIXME assert(test_utf32_to_keysym('\1', XKB_KEY_NoSymbol));
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
    assert(test_utf32_to_keysym(0x20ac, XKB_KEY_EuroSign));

    // Unicode non-characters
    assert(test_utf32_to_keysym(0xd800, XKB_KEY_NoSymbol)); // Unicode surrogates
    assert(test_utf32_to_keysym(0xdfff, XKB_KEY_NoSymbol)); // Unicode surrogates
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

    assert(xkb_keysym_is_upper_or_title(XKB_KEY_A));
    assert(xkb_keysym_is_upper_or_title(XKB_KEY_Greek_LAMBDA));
    assert(xkb_keysym_is_upper_or_title(xkb_keysym_from_name("U0391", 0))); /* GREEK CAPITAL LETTER ALPHA */
    assert(xkb_keysym_is_upper_or_title(xkb_keysym_from_name("U0388", 0))); /* GREEK CAPITAL LETTER EPSILON WITH TONOS */

    assert(!xkb_keysym_is_upper_or_title(XKB_KEY_a));
    assert(!xkb_keysym_is_lower(XKB_KEY_A));
    assert(!xkb_keysym_is_lower(XKB_KEY_Return));
    assert(!xkb_keysym_is_upper_or_title(XKB_KEY_Return));
    assert(!xkb_keysym_is_lower(XKB_KEY_hebrew_aleph));
    assert(!xkb_keysym_is_upper_or_title(XKB_KEY_hebrew_aleph));
    assert(!xkb_keysym_is_upper_or_title(xkb_keysym_from_name("U05D0", 0))); /* HEBREW LETTER ALEF */
    assert(!xkb_keysym_is_lower(xkb_keysym_from_name("U05D0", 0))); /* HEBREW LETTER ALEF */
    assert(!xkb_keysym_is_lower(XKB_KEY_8));
    assert(!xkb_keysym_is_upper_or_title(XKB_KEY_8));

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

    /* S sharp
     * • U+00DF ß: lower case
     * •       SS: upper case (special mapping, not handled by us)
     * • U+1E9E ẞ: upper case, only for capitals
     */
#ifndef XKB_KEY_Ssharp
#define XKB_KEY_Ssharp (XKB_KEYSYM_UNICODE_OFFSET + 0x1E9E)
#endif
    assert(!xkb_keysym_is_upper_or_title(XKB_KEY_ssharp));
    assert(xkb_keysym_is_upper_or_title(XKB_KEY_Ssharp));
    assert(xkb_keysym_is_lower(XKB_KEY_ssharp));
    assert(!xkb_keysym_is_lower(XKB_KEY_Ssharp));
    assert(xkb_keysym_to_upper(XKB_KEY_ssharp) == XKB_KEY_Ssharp);
    assert(xkb_keysym_to_lower(XKB_KEY_ssharp) == XKB_KEY_ssharp);
    assert(xkb_keysym_to_upper(XKB_KEY_Ssharp) == XKB_KEY_Ssharp);
    assert(xkb_keysym_to_lower(XKB_KEY_Ssharp) == XKB_KEY_ssharp);

    /* Title case: simple mappings
     * • U+01F1 Ǳ: upper case
     * • U+01F2 ǲ: title case
     * • U+01F3 ǳ: lower case
     */
#ifndef XKB_KEY_DZ
#define XKB_KEY_DZ (XKB_KEYSYM_UNICODE_OFFSET + 0x01F1)
#endif
#ifndef XKB_KEY_Dz
#define XKB_KEY_Dz (XKB_KEYSYM_UNICODE_OFFSET + 0x01F2)
#endif
#ifndef XKB_KEY_dz
#define XKB_KEY_dz (XKB_KEYSYM_UNICODE_OFFSET + 0x01F3)
#endif
    assert(xkb_keysym_is_upper_or_title(XKB_KEY_DZ));
    assert(xkb_keysym_is_upper_or_title(XKB_KEY_Dz));
    assert(!xkb_keysym_is_upper_or_title(XKB_KEY_dz));
    assert(!xkb_keysym_is_lower(XKB_KEY_DZ));
    assert(!xkb_keysym_is_lower(XKB_KEY_Dz));
    assert(xkb_keysym_is_lower(XKB_KEY_dz));
    assert(xkb_keysym_to_upper(XKB_KEY_DZ) == XKB_KEY_DZ);
    assert(xkb_keysym_to_lower(XKB_KEY_DZ) == XKB_KEY_dz);
    assert(xkb_keysym_to_upper(XKB_KEY_Dz) == XKB_KEY_DZ);
    assert(xkb_keysym_to_lower(XKB_KEY_Dz) == XKB_KEY_dz);
    assert(xkb_keysym_to_upper(XKB_KEY_dz) == XKB_KEY_DZ);
    assert(xkb_keysym_to_lower(XKB_KEY_dz) == XKB_KEY_dz);

    /* Title case: special mappings
     * • U+1F80         ᾀ: lower case
     * • U+1F88         ᾈ: title case
     * • U+1F88         ᾈ: upper case (simple)
     * • U+1F08 U+0399 ἈΙ: upper case (full)
     *
     * We do not handle special upper mapping
     */
    assert(!xkb_keysym_is_upper_or_title(XKB_KEYSYM_UNICODE_OFFSET + 0x1F80));
    assert(xkb_keysym_is_upper_or_title(XKB_KEYSYM_UNICODE_OFFSET + 0x1F88));
    assert(xkb_keysym_is_lower(XKB_KEYSYM_UNICODE_OFFSET + 0x1F80));
    assert(!xkb_keysym_is_lower(XKB_KEYSYM_UNICODE_OFFSET + 0x1F88));
    assert(xkb_keysym_to_upper(XKB_KEYSYM_UNICODE_OFFSET + 0x1F80) == XKB_KEYSYM_UNICODE_OFFSET + 0x1F88);
    assert(xkb_keysym_to_lower(XKB_KEYSYM_UNICODE_OFFSET + 0x1F80) == XKB_KEYSYM_UNICODE_OFFSET + 0x1F80);
    assert(xkb_keysym_to_upper(XKB_KEYSYM_UNICODE_OFFSET + 0x1F88) == XKB_KEYSYM_UNICODE_OFFSET + 0x1F88);
    assert(xkb_keysym_to_lower(XKB_KEYSYM_UNICODE_OFFSET + 0x1F88) == XKB_KEYSYM_UNICODE_OFFSET + 0x1F80);

    test_github_issue_42();

    return 0;
}
