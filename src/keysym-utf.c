/* The table and comments below along with the function xkb_keysym_to_ucs4
 * are under the public domain and are derived as described below.
 */
/* This module converts keysym values into the corresponding ISO 10646
 * (UCS, Unicode) values.
 *
 * The array keysymtab[] contains pairs of X11 keysym values for graphical
 * characters and the corresponding Unicode value. The function
 * keysym2ucs() maps a keysym onto a Unicode value using a binary search,
 * therefore keysymtab[] must remain SORTED by keysym value.
 *
 * The keysym -> UTF-8 conversion will hopefully one day be provided
 * by Xlib via XmbLookupString() and should ideally not have to be
 * done in X applications. But we are not there yet.
 *
 * We allow to represent any UCS character in the range U-00000000 to
 * U-00FFFFFF by a keysym value in the range 0x01000000 to 0x01ffffff.
 * This admittedly does not cover the entire 31-bit space of UCS, but
 * it does cover all of the characters up to U-10FFFF, which can be
 * represented by UTF-16, and more, and it is very unlikely that higher
 * UCS codes will ever be assigned by ISO. So to get Unicode character
 * U+ABCD you can directly use keysym 0x0100abcd.
 *
 * NOTE: The comments in the table below contain the actual character
 * encoded in UTF-8, so for viewing and editing best use an editor in
 * UTF-8 mode.
 *
 * Author: Markus G. Kuhn <http://www.cl.cam.ac.uk/~mgk25/>,
 *         University of Cambridge, April 2001
 *
 * Special thanks to Richard Verhoeven <river@win.tue.nl> for preparing
 * an initial draft of the mapping table.
 *
 * This software is in the public domain. Share and enjoy!
 *
 */

#include "config.h"

#include <stdbool.h>

#include "xkbcommon/xkbcommon.h"
#include "utils.h"
#include "utf8.h"
#include "keysym.h"

#define NO_KEYSYM_UNICODE_CONVERSION 0

/* We don't use the uint32_t types here, to save some space. */
struct codepair {
    uint16_t keysym:15;
    bool deprecated:1;
    uint16_t ucs;
};

static const struct codepair keysymtab[] = {
    { 0x01a1, false, 0x0104 }, /*                     Aogonek Ą LATIN CAPITAL LETTER A WITH OGONEK */
    { 0x01a2, false, 0x02d8 }, /*                       breve ˘ BREVE */
    { 0x01a3, false, 0x0141 }, /*                     Lstroke Ł LATIN CAPITAL LETTER L WITH STROKE */
    { 0x01a5, false, 0x013d }, /*                      Lcaron Ľ LATIN CAPITAL LETTER L WITH CARON */
    { 0x01a6, false, 0x015a }, /*                      Sacute Ś LATIN CAPITAL LETTER S WITH ACUTE */
    { 0x01a9, false, 0x0160 }, /*                      Scaron Š LATIN CAPITAL LETTER S WITH CARON */
    { 0x01aa, false, 0x015e }, /*                    Scedilla Ş LATIN CAPITAL LETTER S WITH CEDILLA */
    { 0x01ab, false, 0x0164 }, /*                      Tcaron Ť LATIN CAPITAL LETTER T WITH CARON */
    { 0x01ac, false, 0x0179 }, /*                      Zacute Ź LATIN CAPITAL LETTER Z WITH ACUTE */
    { 0x01ae, false, 0x017d }, /*                      Zcaron Ž LATIN CAPITAL LETTER Z WITH CARON */
    { 0x01af, false, 0x017b }, /*                   Zabovedot Ż LATIN CAPITAL LETTER Z WITH DOT ABOVE */
    { 0x01b1, false, 0x0105 }, /*                     aogonek ą LATIN SMALL LETTER A WITH OGONEK */
    { 0x01b2, false, 0x02db }, /*                      ogonek ˛ OGONEK */
    { 0x01b3, false, 0x0142 }, /*                     lstroke ł LATIN SMALL LETTER L WITH STROKE */
    { 0x01b5, false, 0x013e }, /*                      lcaron ľ LATIN SMALL LETTER L WITH CARON */
    { 0x01b6, false, 0x015b }, /*                      sacute ś LATIN SMALL LETTER S WITH ACUTE */
    { 0x01b7, false, 0x02c7 }, /*                       caron ˇ CARON */
    { 0x01b9, false, 0x0161 }, /*                      scaron š LATIN SMALL LETTER S WITH CARON */
    { 0x01ba, false, 0x015f }, /*                    scedilla ş LATIN SMALL LETTER S WITH CEDILLA */
    { 0x01bb, false, 0x0165 }, /*                      tcaron ť LATIN SMALL LETTER T WITH CARON */
    { 0x01bc, false, 0x017a }, /*                      zacute ź LATIN SMALL LETTER Z WITH ACUTE */
    { 0x01bd, false, 0x02dd }, /*                 doubleacute ˝ DOUBLE ACUTE ACCENT */
    { 0x01be, false, 0x017e }, /*                      zcaron ž LATIN SMALL LETTER Z WITH CARON */
    { 0x01bf, false, 0x017c }, /*                   zabovedot ż LATIN SMALL LETTER Z WITH DOT ABOVE */
    { 0x01c0, false, 0x0154 }, /*                      Racute Ŕ LATIN CAPITAL LETTER R WITH ACUTE */
    { 0x01c3, false, 0x0102 }, /*                      Abreve Ă LATIN CAPITAL LETTER A WITH BREVE */
    { 0x01c5, false, 0x0139 }, /*                      Lacute Ĺ LATIN CAPITAL LETTER L WITH ACUTE */
    { 0x01c6, false, 0x0106 }, /*                      Cacute Ć LATIN CAPITAL LETTER C WITH ACUTE */
    { 0x01c8, false, 0x010c }, /*                      Ccaron Č LATIN CAPITAL LETTER C WITH CARON */
    { 0x01ca, false, 0x0118 }, /*                     Eogonek Ę LATIN CAPITAL LETTER E WITH OGONEK */
    { 0x01cc, false, 0x011a }, /*                      Ecaron Ě LATIN CAPITAL LETTER E WITH CARON */
    { 0x01cf, false, 0x010e }, /*                      Dcaron Ď LATIN CAPITAL LETTER D WITH CARON */
    { 0x01d0, false, 0x0110 }, /*                     Dstroke Đ LATIN CAPITAL LETTER D WITH STROKE */
    { 0x01d1, false, 0x0143 }, /*                      Nacute Ń LATIN CAPITAL LETTER N WITH ACUTE */
    { 0x01d2, false, 0x0147 }, /*                      Ncaron Ň LATIN CAPITAL LETTER N WITH CARON */
    { 0x01d5, false, 0x0150 }, /*                Odoubleacute Ő LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
    { 0x01d8, false, 0x0158 }, /*                      Rcaron Ř LATIN CAPITAL LETTER R WITH CARON */
    { 0x01d9, false, 0x016e }, /*                       Uring Ů LATIN CAPITAL LETTER U WITH RING ABOVE */
    { 0x01db, false, 0x0170 }, /*                Udoubleacute Ű LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
    { 0x01de, false, 0x0162 }, /*                    Tcedilla Ţ LATIN CAPITAL LETTER T WITH CEDILLA */
    { 0x01e0, false, 0x0155 }, /*                      racute ŕ LATIN SMALL LETTER R WITH ACUTE */
    { 0x01e3, false, 0x0103 }, /*                      abreve ă LATIN SMALL LETTER A WITH BREVE */
    { 0x01e5, false, 0x013a }, /*                      lacute ĺ LATIN SMALL LETTER L WITH ACUTE */
    { 0x01e6, false, 0x0107 }, /*                      cacute ć LATIN SMALL LETTER C WITH ACUTE */
    { 0x01e8, false, 0x010d }, /*                      ccaron č LATIN SMALL LETTER C WITH CARON */
    { 0x01ea, false, 0x0119 }, /*                     eogonek ę LATIN SMALL LETTER E WITH OGONEK */
    { 0x01ec, false, 0x011b }, /*                      ecaron ě LATIN SMALL LETTER E WITH CARON */
    { 0x01ef, false, 0x010f }, /*                      dcaron ď LATIN SMALL LETTER D WITH CARON */
    { 0x01f0, false, 0x0111 }, /*                     dstroke đ LATIN SMALL LETTER D WITH STROKE */
    { 0x01f1, false, 0x0144 }, /*                      nacute ń LATIN SMALL LETTER N WITH ACUTE */
    { 0x01f2, false, 0x0148 }, /*                      ncaron ň LATIN SMALL LETTER N WITH CARON */
    { 0x01f5, false, 0x0151 }, /*                odoubleacute ő LATIN SMALL LETTER O WITH DOUBLE ACUTE */
    { 0x01f8, false, 0x0159 }, /*                      rcaron ř LATIN SMALL LETTER R WITH CARON */
    { 0x01f9, false, 0x016f }, /*                       uring ů LATIN SMALL LETTER U WITH RING ABOVE */
    { 0x01fb, false, 0x0171 }, /*                udoubleacute ű LATIN SMALL LETTER U WITH DOUBLE ACUTE */
    { 0x01fe, false, 0x0163 }, /*                    tcedilla ţ LATIN SMALL LETTER T WITH CEDILLA */
    { 0x01ff, false, 0x02d9 }, /*                    abovedot ˙ DOT ABOVE */
    { 0x02a1, false, 0x0126 }, /*                     Hstroke Ħ LATIN CAPITAL LETTER H WITH STROKE */
    { 0x02a6, false, 0x0124 }, /*                 Hcircumflex Ĥ LATIN CAPITAL LETTER H WITH CIRCUMFLEX */
    { 0x02a9, false, 0x0130 }, /*                   Iabovedot İ LATIN CAPITAL LETTER I WITH DOT ABOVE */
    { 0x02ab, false, 0x011e }, /*                      Gbreve Ğ LATIN CAPITAL LETTER G WITH BREVE */
    { 0x02ac, false, 0x0134 }, /*                 Jcircumflex Ĵ LATIN CAPITAL LETTER J WITH CIRCUMFLEX */
    { 0x02b1, false, 0x0127 }, /*                     hstroke ħ LATIN SMALL LETTER H WITH STROKE */
    { 0x02b6, false, 0x0125 }, /*                 hcircumflex ĥ LATIN SMALL LETTER H WITH CIRCUMFLEX */
    { 0x02b9, false, 0x0131 }, /*                    idotless ı LATIN SMALL LETTER DOTLESS I */
    { 0x02bb, false, 0x011f }, /*                      gbreve ğ LATIN SMALL LETTER G WITH BREVE */
    { 0x02bc, false, 0x0135 }, /*                 jcircumflex ĵ LATIN SMALL LETTER J WITH CIRCUMFLEX */
    { 0x02c5, false, 0x010a }, /*                   Cabovedot Ċ LATIN CAPITAL LETTER C WITH DOT ABOVE */
    { 0x02c6, false, 0x0108 }, /*                 Ccircumflex Ĉ LATIN CAPITAL LETTER C WITH CIRCUMFLEX */
    { 0x02d5, false, 0x0120 }, /*                   Gabovedot Ġ LATIN CAPITAL LETTER G WITH DOT ABOVE */
    { 0x02d8, false, 0x011c }, /*                 Gcircumflex Ĝ LATIN CAPITAL LETTER G WITH CIRCUMFLEX */
    { 0x02dd, false, 0x016c }, /*                      Ubreve Ŭ LATIN CAPITAL LETTER U WITH BREVE */
    { 0x02de, false, 0x015c }, /*                 Scircumflex Ŝ LATIN CAPITAL LETTER S WITH CIRCUMFLEX */
    { 0x02e5, false, 0x010b }, /*                   cabovedot ċ LATIN SMALL LETTER C WITH DOT ABOVE */
    { 0x02e6, false, 0x0109 }, /*                 ccircumflex ĉ LATIN SMALL LETTER C WITH CIRCUMFLEX */
    { 0x02f5, false, 0x0121 }, /*                   gabovedot ġ LATIN SMALL LETTER G WITH DOT ABOVE */
    { 0x02f8, false, 0x011d }, /*                 gcircumflex ĝ LATIN SMALL LETTER G WITH CIRCUMFLEX */
    { 0x02fd, false, 0x016d }, /*                      ubreve ŭ LATIN SMALL LETTER U WITH BREVE */
    { 0x02fe, false, 0x015d }, /*                 scircumflex ŝ LATIN SMALL LETTER S WITH CIRCUMFLEX */
    { 0x03a2, false, 0x0138 }, /*                         kra ĸ LATIN SMALL LETTER KRA */
    { 0x03a3, false, 0x0156 }, /*                    Rcedilla Ŗ LATIN CAPITAL LETTER R WITH CEDILLA */
    { 0x03a5, false, 0x0128 }, /*                      Itilde Ĩ LATIN CAPITAL LETTER I WITH TILDE */
    { 0x03a6, false, 0x013b }, /*                    Lcedilla Ļ LATIN CAPITAL LETTER L WITH CEDILLA */
    { 0x03aa, false, 0x0112 }, /*                     Emacron Ē LATIN CAPITAL LETTER E WITH MACRON */
    { 0x03ab, false, 0x0122 }, /*                    Gcedilla Ģ LATIN CAPITAL LETTER G WITH CEDILLA */
    { 0x03ac, false, 0x0166 }, /*                      Tslash Ŧ LATIN CAPITAL LETTER T WITH STROKE */
    { 0x03b3, false, 0x0157 }, /*                    rcedilla ŗ LATIN SMALL LETTER R WITH CEDILLA */
    { 0x03b5, false, 0x0129 }, /*                      itilde ĩ LATIN SMALL LETTER I WITH TILDE */
    { 0x03b6, false, 0x013c }, /*                    lcedilla ļ LATIN SMALL LETTER L WITH CEDILLA */
    { 0x03ba, false, 0x0113 }, /*                     emacron ē LATIN SMALL LETTER E WITH MACRON */
    { 0x03bb, false, 0x0123 }, /*                    gcedilla ģ LATIN SMALL LETTER G WITH CEDILLA */
    { 0x03bc, false, 0x0167 }, /*                      tslash ŧ LATIN SMALL LETTER T WITH STROKE */
    { 0x03bd, false, 0x014a }, /*                         ENG Ŋ LATIN CAPITAL LETTER ENG */
    { 0x03bf, false, 0x014b }, /*                         eng ŋ LATIN SMALL LETTER ENG */
    { 0x03c0, false, 0x0100 }, /*                     Amacron Ā LATIN CAPITAL LETTER A WITH MACRON */
    { 0x03c7, false, 0x012e }, /*                     Iogonek Į LATIN CAPITAL LETTER I WITH OGONEK */
    { 0x03cc, false, 0x0116 }, /*                   Eabovedot Ė LATIN CAPITAL LETTER E WITH DOT ABOVE */
    { 0x03cf, false, 0x012a }, /*                     Imacron Ī LATIN CAPITAL LETTER I WITH MACRON */
    { 0x03d1, false, 0x0145 }, /*                    Ncedilla Ņ LATIN CAPITAL LETTER N WITH CEDILLA */
    { 0x03d2, false, 0x014c }, /*                     Omacron Ō LATIN CAPITAL LETTER O WITH MACRON */
    { 0x03d3, false, 0x0136 }, /*                    Kcedilla Ķ LATIN CAPITAL LETTER K WITH CEDILLA */
    { 0x03d9, false, 0x0172 }, /*                     Uogonek Ų LATIN CAPITAL LETTER U WITH OGONEK */
    { 0x03dd, false, 0x0168 }, /*                      Utilde Ũ LATIN CAPITAL LETTER U WITH TILDE */
    { 0x03de, false, 0x016a }, /*                     Umacron Ū LATIN CAPITAL LETTER U WITH MACRON */
    { 0x03e0, false, 0x0101 }, /*                     amacron ā LATIN SMALL LETTER A WITH MACRON */
    { 0x03e7, false, 0x012f }, /*                     iogonek į LATIN SMALL LETTER I WITH OGONEK */
    { 0x03ec, false, 0x0117 }, /*                   eabovedot ė LATIN SMALL LETTER E WITH DOT ABOVE */
    { 0x03ef, false, 0x012b }, /*                     imacron ī LATIN SMALL LETTER I WITH MACRON */
    { 0x03f1, false, 0x0146 }, /*                    ncedilla ņ LATIN SMALL LETTER N WITH CEDILLA */
    { 0x03f2, false, 0x014d }, /*                     omacron ō LATIN SMALL LETTER O WITH MACRON */
    { 0x03f3, false, 0x0137 }, /*                    kcedilla ķ LATIN SMALL LETTER K WITH CEDILLA */
    { 0x03f9, false, 0x0173 }, /*                     uogonek ų LATIN SMALL LETTER U WITH OGONEK */
    { 0x03fd, false, 0x0169 }, /*                      utilde ũ LATIN SMALL LETTER U WITH TILDE */
    { 0x03fe, false, 0x016b }, /*                     umacron ū LATIN SMALL LETTER U WITH MACRON */
    { 0x047e, false, 0x203e }, /*                    overline ‾ OVERLINE */
    { 0x04a1, false, 0x3002 }, /*               kana_fullstop 。 IDEOGRAPHIC FULL STOP */
    { 0x04a2, false, 0x300c }, /*         kana_openingbracket 「 LEFT CORNER BRACKET */
    { 0x04a3, false, 0x300d }, /*         kana_closingbracket 」 RIGHT CORNER BRACKET */
    { 0x04a4, false, 0x3001 }, /*                  kana_comma 、 IDEOGRAPHIC COMMA */
    { 0x04a5, false, 0x30fb }, /*            kana_conjunctive ・ KATAKANA MIDDLE DOT */
    { 0x04a6, false, 0x30f2 }, /*                     kana_WO ヲ KATAKANA LETTER WO */
    { 0x04a7, false, 0x30a1 }, /*                      kana_a ァ KATAKANA LETTER SMALL A */
    { 0x04a8, false, 0x30a3 }, /*                      kana_i ィ KATAKANA LETTER SMALL I */
    { 0x04a9, false, 0x30a5 }, /*                      kana_u ゥ KATAKANA LETTER SMALL U */
    { 0x04aa, false, 0x30a7 }, /*                      kana_e ェ KATAKANA LETTER SMALL E */
    { 0x04ab, false, 0x30a9 }, /*                      kana_o ォ KATAKANA LETTER SMALL O */
    { 0x04ac, false, 0x30e3 }, /*                     kana_ya ャ KATAKANA LETTER SMALL YA */
    { 0x04ad, false, 0x30e5 }, /*                     kana_yu ュ KATAKANA LETTER SMALL YU */
    { 0x04ae, false, 0x30e7 }, /*                     kana_yo ョ KATAKANA LETTER SMALL YO */
    { 0x04af, false, 0x30c3 }, /*                    kana_tsu ッ KATAKANA LETTER SMALL TU */
    { 0x04b0, false, 0x30fc }, /*              prolongedsound ー KATAKANA-HIRAGANA PROLONGED SOUND MARK */
    { 0x04b1, false, 0x30a2 }, /*                      kana_A ア KATAKANA LETTER A */
    { 0x04b2, false, 0x30a4 }, /*                      kana_I イ KATAKANA LETTER I */
    { 0x04b3, false, 0x30a6 }, /*                      kana_U ウ KATAKANA LETTER U */
    { 0x04b4, false, 0x30a8 }, /*                      kana_E エ KATAKANA LETTER E */
    { 0x04b5, false, 0x30aa }, /*                      kana_O オ KATAKANA LETTER O */
    { 0x04b6, false, 0x30ab }, /*                     kana_KA カ KATAKANA LETTER KA */
    { 0x04b7, false, 0x30ad }, /*                     kana_KI キ KATAKANA LETTER KI */
    { 0x04b8, false, 0x30af }, /*                     kana_KU ク KATAKANA LETTER KU */
    { 0x04b9, false, 0x30b1 }, /*                     kana_KE ケ KATAKANA LETTER KE */
    { 0x04ba, false, 0x30b3 }, /*                     kana_KO コ KATAKANA LETTER KO */
    { 0x04bb, false, 0x30b5 }, /*                     kana_SA サ KATAKANA LETTER SA */
    { 0x04bc, false, 0x30b7 }, /*                    kana_SHI シ KATAKANA LETTER SI */
    { 0x04bd, false, 0x30b9 }, /*                     kana_SU ス KATAKANA LETTER SU */
    { 0x04be, false, 0x30bb }, /*                     kana_SE セ KATAKANA LETTER SE */
    { 0x04bf, false, 0x30bd }, /*                     kana_SO ソ KATAKANA LETTER SO */
    { 0x04c0, false, 0x30bf }, /*                     kana_TA タ KATAKANA LETTER TA */
    { 0x04c1, false, 0x30c1 }, /*                    kana_CHI チ KATAKANA LETTER TI */
    { 0x04c2, false, 0x30c4 }, /*                    kana_TSU ツ KATAKANA LETTER TU */
    { 0x04c3, false, 0x30c6 }, /*                     kana_TE テ KATAKANA LETTER TE */
    { 0x04c4, false, 0x30c8 }, /*                     kana_TO ト KATAKANA LETTER TO */
    { 0x04c5, false, 0x30ca }, /*                     kana_NA ナ KATAKANA LETTER NA */
    { 0x04c6, false, 0x30cb }, /*                     kana_NI ニ KATAKANA LETTER NI */
    { 0x04c7, false, 0x30cc }, /*                     kana_NU ヌ KATAKANA LETTER NU */
    { 0x04c8, false, 0x30cd }, /*                     kana_NE ネ KATAKANA LETTER NE */
    { 0x04c9, false, 0x30ce }, /*                     kana_NO ノ KATAKANA LETTER NO */
    { 0x04ca, false, 0x30cf }, /*                     kana_HA ハ KATAKANA LETTER HA */
    { 0x04cb, false, 0x30d2 }, /*                     kana_HI ヒ KATAKANA LETTER HI */
    { 0x04cc, false, 0x30d5 }, /*                     kana_FU フ KATAKANA LETTER HU */
    { 0x04cd, false, 0x30d8 }, /*                     kana_HE ヘ KATAKANA LETTER HE */
    { 0x04ce, false, 0x30db }, /*                     kana_HO ホ KATAKANA LETTER HO */
    { 0x04cf, false, 0x30de }, /*                     kana_MA マ KATAKANA LETTER MA */
    { 0x04d0, false, 0x30df }, /*                     kana_MI ミ KATAKANA LETTER MI */
    { 0x04d1, false, 0x30e0 }, /*                     kana_MU ム KATAKANA LETTER MU */
    { 0x04d2, false, 0x30e1 }, /*                     kana_ME メ KATAKANA LETTER ME */
    { 0x04d3, false, 0x30e2 }, /*                     kana_MO モ KATAKANA LETTER MO */
    { 0x04d4, false, 0x30e4 }, /*                     kana_YA ヤ KATAKANA LETTER YA */
    { 0x04d5, false, 0x30e6 }, /*                     kana_YU ユ KATAKANA LETTER YU */
    { 0x04d6, false, 0x30e8 }, /*                     kana_YO ヨ KATAKANA LETTER YO */
    { 0x04d7, false, 0x30e9 }, /*                     kana_RA ラ KATAKANA LETTER RA */
    { 0x04d8, false, 0x30ea }, /*                     kana_RI リ KATAKANA LETTER RI */
    { 0x04d9, false, 0x30eb }, /*                     kana_RU ル KATAKANA LETTER RU */
    { 0x04da, false, 0x30ec }, /*                     kana_RE レ KATAKANA LETTER RE */
    { 0x04db, false, 0x30ed }, /*                     kana_RO ロ KATAKANA LETTER RO */
    { 0x04dc, false, 0x30ef }, /*                     kana_WA ワ KATAKANA LETTER WA */
    { 0x04dd, false, 0x30f3 }, /*                      kana_N ン KATAKANA LETTER N */
    { 0x04de, false, 0x309b }, /*                 voicedsound ゛ KATAKANA-HIRAGANA VOICED SOUND MARK */
    { 0x04df, false, 0x309c }, /*             semivoicedsound ゜ KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
    { 0x05ac, false, 0x060c }, /*                Arabic_comma ، ARABIC COMMA */
    { 0x05bb, false, 0x061b }, /*            Arabic_semicolon ؛ ARABIC SEMICOLON */
    { 0x05bf, false, 0x061f }, /*        Arabic_question_mark ؟ ARABIC QUESTION MARK */
    { 0x05c1, false, 0x0621 }, /*                Arabic_hamza ء ARABIC LETTER HAMZA */
    { 0x05c2, false, 0x0622 }, /*          Arabic_maddaonalef آ ARABIC LETTER ALEF WITH MADDA ABOVE */
    { 0x05c3, false, 0x0623 }, /*          Arabic_hamzaonalef أ ARABIC LETTER ALEF WITH HAMZA ABOVE */
    { 0x05c4, false, 0x0624 }, /*           Arabic_hamzaonwaw ؤ ARABIC LETTER WAW WITH HAMZA ABOVE */
    { 0x05c5, false, 0x0625 }, /*       Arabic_hamzaunderalef إ ARABIC LETTER ALEF WITH HAMZA BELOW */
    { 0x05c6, false, 0x0626 }, /*           Arabic_hamzaonyeh ئ ARABIC LETTER YEH WITH HAMZA ABOVE */
    { 0x05c7, false, 0x0627 }, /*                 Arabic_alef ا ARABIC LETTER ALEF */
    { 0x05c8, false, 0x0628 }, /*                  Arabic_beh ب ARABIC LETTER BEH */
    { 0x05c9, false, 0x0629 }, /*           Arabic_tehmarbuta ة ARABIC LETTER TEH MARBUTA */
    { 0x05ca, false, 0x062a }, /*                  Arabic_teh ت ARABIC LETTER TEH */
    { 0x05cb, false, 0x062b }, /*                 Arabic_theh ث ARABIC LETTER THEH */
    { 0x05cc, false, 0x062c }, /*                 Arabic_jeem ج ARABIC LETTER JEEM */
    { 0x05cd, false, 0x062d }, /*                  Arabic_hah ح ARABIC LETTER HAH */
    { 0x05ce, false, 0x062e }, /*                 Arabic_khah خ ARABIC LETTER KHAH */
    { 0x05cf, false, 0x062f }, /*                  Arabic_dal د ARABIC LETTER DAL */
    { 0x05d0, false, 0x0630 }, /*                 Arabic_thal ذ ARABIC LETTER THAL */
    { 0x05d1, false, 0x0631 }, /*                   Arabic_ra ر ARABIC LETTER REH */
    { 0x05d2, false, 0x0632 }, /*                 Arabic_zain ز ARABIC LETTER ZAIN */
    { 0x05d3, false, 0x0633 }, /*                 Arabic_seen س ARABIC LETTER SEEN */
    { 0x05d4, false, 0x0634 }, /*                Arabic_sheen ش ARABIC LETTER SHEEN */
    { 0x05d5, false, 0x0635 }, /*                  Arabic_sad ص ARABIC LETTER SAD */
    { 0x05d6, false, 0x0636 }, /*                  Arabic_dad ض ARABIC LETTER DAD */
    { 0x05d7, false, 0x0637 }, /*                  Arabic_tah ط ARABIC LETTER TAH */
    { 0x05d8, false, 0x0638 }, /*                  Arabic_zah ظ ARABIC LETTER ZAH */
    { 0x05d9, false, 0x0639 }, /*                  Arabic_ain ع ARABIC LETTER AIN */
    { 0x05da, false, 0x063a }, /*                Arabic_ghain غ ARABIC LETTER GHAIN */
    { 0x05e0, false, 0x0640 }, /*              Arabic_tatweel ـ ARABIC TATWEEL */
    { 0x05e1, false, 0x0641 }, /*                  Arabic_feh ف ARABIC LETTER FEH */
    { 0x05e2, false, 0x0642 }, /*                  Arabic_qaf ق ARABIC LETTER QAF */
    { 0x05e3, false, 0x0643 }, /*                  Arabic_kaf ك ARABIC LETTER KAF */
    { 0x05e4, false, 0x0644 }, /*                  Arabic_lam ل ARABIC LETTER LAM */
    { 0x05e5, false, 0x0645 }, /*                 Arabic_meem م ARABIC LETTER MEEM */
    { 0x05e6, false, 0x0646 }, /*                 Arabic_noon ن ARABIC LETTER NOON */
    { 0x05e7, false, 0x0647 }, /*                   Arabic_ha ه ARABIC LETTER HEH */
    { 0x05e8, false, 0x0648 }, /*                  Arabic_waw و ARABIC LETTER WAW */
    { 0x05e9, false, 0x0649 }, /*          Arabic_alefmaksura ى ARABIC LETTER ALEF MAKSURA */
    { 0x05ea, false, 0x064a }, /*                  Arabic_yeh ي ARABIC LETTER YEH */
    { 0x05eb, false, 0x064b }, /*             Arabic_fathatan ً ARABIC FATHATAN */
    { 0x05ec, false, 0x064c }, /*             Arabic_dammatan ٌ ARABIC DAMMATAN */
    { 0x05ed, false, 0x064d }, /*             Arabic_kasratan ٍ ARABIC KASRATAN */
    { 0x05ee, false, 0x064e }, /*                Arabic_fatha َ ARABIC FATHA */
    { 0x05ef, false, 0x064f }, /*                Arabic_damma ُ ARABIC DAMMA */
    { 0x05f0, false, 0x0650 }, /*                Arabic_kasra ِ ARABIC KASRA */
    { 0x05f1, false, 0x0651 }, /*               Arabic_shadda ّ ARABIC SHADDA */
    { 0x05f2, false, 0x0652 }, /*                Arabic_sukun ْ ARABIC SUKUN */
    { 0x06a1, false, 0x0452 }, /*                 Serbian_dje ђ CYRILLIC SMALL LETTER DJE */
    { 0x06a2, false, 0x0453 }, /*               Macedonia_gje ѓ CYRILLIC SMALL LETTER GJE */
    { 0x06a3, false, 0x0451 }, /*                 Cyrillic_io ё CYRILLIC SMALL LETTER IO */
    { 0x06a4, false, 0x0454 }, /*                Ukrainian_ie є CYRILLIC SMALL LETTER UKRAINIAN IE */
    { 0x06a5, false, 0x0455 }, /*               Macedonia_dse ѕ CYRILLIC SMALL LETTER DZE */
    { 0x06a6, false, 0x0456 }, /*                 Ukrainian_i і CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I */
    { 0x06a7, false, 0x0457 }, /*                Ukrainian_yi ї CYRILLIC SMALL LETTER YI */
    { 0x06a8, false, 0x0458 }, /*                 Cyrillic_je ј CYRILLIC SMALL LETTER JE */
    { 0x06a9, false, 0x0459 }, /*                Cyrillic_lje љ CYRILLIC SMALL LETTER LJE */
    { 0x06aa, false, 0x045a }, /*                Cyrillic_nje њ CYRILLIC SMALL LETTER NJE */
    { 0x06ab, false, 0x045b }, /*                Serbian_tshe ћ CYRILLIC SMALL LETTER TSHE */
    { 0x06ac, false, 0x045c }, /*               Macedonia_kje ќ CYRILLIC SMALL LETTER KJE */
    { 0x06ad, false, 0x0491 }, /*   Ukrainian_ghe_with_upturn ґ CYRILLIC SMALL LETTER GHE WITH UPTURN */
    { 0x06ae, false, 0x045e }, /*         Byelorussian_shortu ў CYRILLIC SMALL LETTER SHORT U */
    { 0x06af, false, 0x045f }, /*               Cyrillic_dzhe џ CYRILLIC SMALL LETTER DZHE */
    { 0x06b0, false, 0x2116 }, /*                  numerosign № NUMERO SIGN */
    { 0x06b1, false, 0x0402 }, /*                 Serbian_DJE Ђ CYRILLIC CAPITAL LETTER DJE */
    { 0x06b2, false, 0x0403 }, /*               Macedonia_GJE Ѓ CYRILLIC CAPITAL LETTER GJE */
    { 0x06b3, false, 0x0401 }, /*                 Cyrillic_IO Ё CYRILLIC CAPITAL LETTER IO */
    { 0x06b4, false, 0x0404 }, /*                Ukrainian_IE Є CYRILLIC CAPITAL LETTER UKRAINIAN IE */
    { 0x06b5, false, 0x0405 }, /*               Macedonia_DSE Ѕ CYRILLIC CAPITAL LETTER DZE */
    { 0x06b6, false, 0x0406 }, /*                 Ukrainian_I І CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
    { 0x06b7, false, 0x0407 }, /*                Ukrainian_YI Ї CYRILLIC CAPITAL LETTER YI */
    { 0x06b8, false, 0x0408 }, /*                 Cyrillic_JE Ј CYRILLIC CAPITAL LETTER JE */
    { 0x06b9, false, 0x0409 }, /*                Cyrillic_LJE Љ CYRILLIC CAPITAL LETTER LJE */
    { 0x06ba, false, 0x040a }, /*                Cyrillic_NJE Њ CYRILLIC CAPITAL LETTER NJE */
    { 0x06bb, false, 0x040b }, /*                Serbian_TSHE Ћ CYRILLIC CAPITAL LETTER TSHE */
    { 0x06bc, false, 0x040c }, /*               Macedonia_KJE Ќ CYRILLIC CAPITAL LETTER KJE */
    { 0x06bd, false, 0x0490 }, /*   Ukrainian_GHE_WITH_UPTURN Ґ CYRILLIC CAPITAL LETTER GHE WITH UPTURN */
    { 0x06be, false, 0x040e }, /*         Byelorussian_SHORTU Ў CYRILLIC CAPITAL LETTER SHORT U */
    { 0x06bf, false, 0x040f }, /*               Cyrillic_DZHE Џ CYRILLIC CAPITAL LETTER DZHE */
    { 0x06c0, false, 0x044e }, /*                 Cyrillic_yu ю CYRILLIC SMALL LETTER YU */
    { 0x06c1, false, 0x0430 }, /*                  Cyrillic_a а CYRILLIC SMALL LETTER A */
    { 0x06c2, false, 0x0431 }, /*                 Cyrillic_be б CYRILLIC SMALL LETTER BE */
    { 0x06c3, false, 0x0446 }, /*                Cyrillic_tse ц CYRILLIC SMALL LETTER TSE */
    { 0x06c4, false, 0x0434 }, /*                 Cyrillic_de д CYRILLIC SMALL LETTER DE */
    { 0x06c5, false, 0x0435 }, /*                 Cyrillic_ie е CYRILLIC SMALL LETTER IE */
    { 0x06c6, false, 0x0444 }, /*                 Cyrillic_ef ф CYRILLIC SMALL LETTER EF */
    { 0x06c7, false, 0x0433 }, /*                Cyrillic_ghe г CYRILLIC SMALL LETTER GHE */
    { 0x06c8, false, 0x0445 }, /*                 Cyrillic_ha х CYRILLIC SMALL LETTER HA */
    { 0x06c9, false, 0x0438 }, /*                  Cyrillic_i и CYRILLIC SMALL LETTER I */
    { 0x06ca, false, 0x0439 }, /*             Cyrillic_shorti й CYRILLIC SMALL LETTER SHORT I */
    { 0x06cb, false, 0x043a }, /*                 Cyrillic_ka к CYRILLIC SMALL LETTER KA */
    { 0x06cc, false, 0x043b }, /*                 Cyrillic_el л CYRILLIC SMALL LETTER EL */
    { 0x06cd, false, 0x043c }, /*                 Cyrillic_em м CYRILLIC SMALL LETTER EM */
    { 0x06ce, false, 0x043d }, /*                 Cyrillic_en н CYRILLIC SMALL LETTER EN */
    { 0x06cf, false, 0x043e }, /*                  Cyrillic_o о CYRILLIC SMALL LETTER O */
    { 0x06d0, false, 0x043f }, /*                 Cyrillic_pe п CYRILLIC SMALL LETTER PE */
    { 0x06d1, false, 0x044f }, /*                 Cyrillic_ya я CYRILLIC SMALL LETTER YA */
    { 0x06d2, false, 0x0440 }, /*                 Cyrillic_er р CYRILLIC SMALL LETTER ER */
    { 0x06d3, false, 0x0441 }, /*                 Cyrillic_es с CYRILLIC SMALL LETTER ES */
    { 0x06d4, false, 0x0442 }, /*                 Cyrillic_te т CYRILLIC SMALL LETTER TE */
    { 0x06d5, false, 0x0443 }, /*                  Cyrillic_u у CYRILLIC SMALL LETTER U */
    { 0x06d6, false, 0x0436 }, /*                Cyrillic_zhe ж CYRILLIC SMALL LETTER ZHE */
    { 0x06d7, false, 0x0432 }, /*                 Cyrillic_ve в CYRILLIC SMALL LETTER VE */
    { 0x06d8, false, 0x044c }, /*           Cyrillic_softsign ь CYRILLIC SMALL LETTER SOFT SIGN */
    { 0x06d9, false, 0x044b }, /*               Cyrillic_yeru ы CYRILLIC SMALL LETTER YERU */
    { 0x06da, false, 0x0437 }, /*                 Cyrillic_ze з CYRILLIC SMALL LETTER ZE */
    { 0x06db, false, 0x0448 }, /*                Cyrillic_sha ш CYRILLIC SMALL LETTER SHA */
    { 0x06dc, false, 0x044d }, /*                  Cyrillic_e э CYRILLIC SMALL LETTER E */
    { 0x06dd, false, 0x0449 }, /*              Cyrillic_shcha щ CYRILLIC SMALL LETTER SHCHA */
    { 0x06de, false, 0x0447 }, /*                Cyrillic_che ч CYRILLIC SMALL LETTER CHE */
    { 0x06df, false, 0x044a }, /*           Cyrillic_hardsign ъ CYRILLIC SMALL LETTER HARD SIGN */
    { 0x06e0, false, 0x042e }, /*                 Cyrillic_YU Ю CYRILLIC CAPITAL LETTER YU */
    { 0x06e1, false, 0x0410 }, /*                  Cyrillic_A А CYRILLIC CAPITAL LETTER A */
    { 0x06e2, false, 0x0411 }, /*                 Cyrillic_BE Б CYRILLIC CAPITAL LETTER BE */
    { 0x06e3, false, 0x0426 }, /*                Cyrillic_TSE Ц CYRILLIC CAPITAL LETTER TSE */
    { 0x06e4, false, 0x0414 }, /*                 Cyrillic_DE Д CYRILLIC CAPITAL LETTER DE */
    { 0x06e5, false, 0x0415 }, /*                 Cyrillic_IE Е CYRILLIC CAPITAL LETTER IE */
    { 0x06e6, false, 0x0424 }, /*                 Cyrillic_EF Ф CYRILLIC CAPITAL LETTER EF */
    { 0x06e7, false, 0x0413 }, /*                Cyrillic_GHE Г CYRILLIC CAPITAL LETTER GHE */
    { 0x06e8, false, 0x0425 }, /*                 Cyrillic_HA Х CYRILLIC CAPITAL LETTER HA */
    { 0x06e9, false, 0x0418 }, /*                  Cyrillic_I И CYRILLIC CAPITAL LETTER I */
    { 0x06ea, false, 0x0419 }, /*             Cyrillic_SHORTI Й CYRILLIC CAPITAL LETTER SHORT I */
    { 0x06eb, false, 0x041a }, /*                 Cyrillic_KA К CYRILLIC CAPITAL LETTER KA */
    { 0x06ec, false, 0x041b }, /*                 Cyrillic_EL Л CYRILLIC CAPITAL LETTER EL */
    { 0x06ed, false, 0x041c }, /*                 Cyrillic_EM М CYRILLIC CAPITAL LETTER EM */
    { 0x06ee, false, 0x041d }, /*                 Cyrillic_EN Н CYRILLIC CAPITAL LETTER EN */
    { 0x06ef, false, 0x041e }, /*                  Cyrillic_O О CYRILLIC CAPITAL LETTER O */
    { 0x06f0, false, 0x041f }, /*                 Cyrillic_PE П CYRILLIC CAPITAL LETTER PE */
    { 0x06f1, false, 0x042f }, /*                 Cyrillic_YA Я CYRILLIC CAPITAL LETTER YA */
    { 0x06f2, false, 0x0420 }, /*                 Cyrillic_ER Р CYRILLIC CAPITAL LETTER ER */
    { 0x06f3, false, 0x0421 }, /*                 Cyrillic_ES С CYRILLIC CAPITAL LETTER ES */
    { 0x06f4, false, 0x0422 }, /*                 Cyrillic_TE Т CYRILLIC CAPITAL LETTER TE */
    { 0x06f5, false, 0x0423 }, /*                  Cyrillic_U У CYRILLIC CAPITAL LETTER U */
    { 0x06f6, false, 0x0416 }, /*                Cyrillic_ZHE Ж CYRILLIC CAPITAL LETTER ZHE */
    { 0x06f7, false, 0x0412 }, /*                 Cyrillic_VE В CYRILLIC CAPITAL LETTER VE */
    { 0x06f8, false, 0x042c }, /*           Cyrillic_SOFTSIGN Ь CYRILLIC CAPITAL LETTER SOFT SIGN */
    { 0x06f9, false, 0x042b }, /*               Cyrillic_YERU Ы CYRILLIC CAPITAL LETTER YERU */
    { 0x06fa, false, 0x0417 }, /*                 Cyrillic_ZE З CYRILLIC CAPITAL LETTER ZE */
    { 0x06fb, false, 0x0428 }, /*                Cyrillic_SHA Ш CYRILLIC CAPITAL LETTER SHA */
    { 0x06fc, false, 0x042d }, /*                  Cyrillic_E Э CYRILLIC CAPITAL LETTER E */
    { 0x06fd, false, 0x0429 }, /*              Cyrillic_SHCHA Щ CYRILLIC CAPITAL LETTER SHCHA */
    { 0x06fe, false, 0x0427 }, /*                Cyrillic_CHE Ч CYRILLIC CAPITAL LETTER CHE */
    { 0x06ff, false, 0x042a }, /*           Cyrillic_HARDSIGN Ъ CYRILLIC CAPITAL LETTER HARD SIGN */
    { 0x07a1, false, 0x0386 }, /*           Greek_ALPHAaccent Ά GREEK CAPITAL LETTER ALPHA WITH TONOS */
    { 0x07a2, false, 0x0388 }, /*         Greek_EPSILONaccent Έ GREEK CAPITAL LETTER EPSILON WITH TONOS */
    { 0x07a3, false, 0x0389 }, /*             Greek_ETAaccent Ή GREEK CAPITAL LETTER ETA WITH TONOS */
    { 0x07a4, false, 0x038a }, /*            Greek_IOTAaccent Ί GREEK CAPITAL LETTER IOTA WITH TONOS */
    { 0x07a5, false, 0x03aa }, /*         Greek_IOTAdiaeresis Ϊ GREEK CAPITAL LETTER IOTA WITH DIALYTIKA */
    { 0x07a7, false, 0x038c }, /*         Greek_OMICRONaccent Ό GREEK CAPITAL LETTER OMICRON WITH TONOS */
    { 0x07a8, false, 0x038e }, /*         Greek_UPSILONaccent Ύ GREEK CAPITAL LETTER UPSILON WITH TONOS */
    { 0x07a9, false, 0x03ab }, /*       Greek_UPSILONdieresis Ϋ GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA */
    { 0x07ab, false, 0x038f }, /*           Greek_OMEGAaccent Ώ GREEK CAPITAL LETTER OMEGA WITH TONOS */
    { 0x07ae, false, 0x0385 }, /*        Greek_accentdieresis ΅ GREEK DIALYTIKA TONOS */
    { 0x07af, false, 0x2015 }, /*              Greek_horizbar ― HORIZONTAL BAR */
    { 0x07b1, false, 0x03ac }, /*           Greek_alphaaccent ά GREEK SMALL LETTER ALPHA WITH TONOS */
    { 0x07b2, false, 0x03ad }, /*         Greek_epsilonaccent έ GREEK SMALL LETTER EPSILON WITH TONOS */
    { 0x07b3, false, 0x03ae }, /*             Greek_etaaccent ή GREEK SMALL LETTER ETA WITH TONOS */
    { 0x07b4, false, 0x03af }, /*            Greek_iotaaccent ί GREEK SMALL LETTER IOTA WITH TONOS */
    { 0x07b5, false, 0x03ca }, /*          Greek_iotadieresis ϊ GREEK SMALL LETTER IOTA WITH DIALYTIKA */
    { 0x07b6, false, 0x0390 }, /*    Greek_iotaaccentdieresis ΐ GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
    { 0x07b7, false, 0x03cc }, /*         Greek_omicronaccent ό GREEK SMALL LETTER OMICRON WITH TONOS */
    { 0x07b8, false, 0x03cd }, /*         Greek_upsilonaccent ύ GREEK SMALL LETTER UPSILON WITH TONOS */
    { 0x07b9, false, 0x03cb }, /*       Greek_upsilondieresis ϋ GREEK SMALL LETTER UPSILON WITH DIALYTIKA */
    { 0x07ba, false, 0x03b0 }, /* Greek_upsilonaccentdieresis ΰ GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS */
    { 0x07bb, false, 0x03ce }, /*           Greek_omegaaccent ώ GREEK SMALL LETTER OMEGA WITH TONOS */
    { 0x07c1, false, 0x0391 }, /*                 Greek_ALPHA Α GREEK CAPITAL LETTER ALPHA */
    { 0x07c2, false, 0x0392 }, /*                  Greek_BETA Β GREEK CAPITAL LETTER BETA */
    { 0x07c3, false, 0x0393 }, /*                 Greek_GAMMA Γ GREEK CAPITAL LETTER GAMMA */
    { 0x07c4, false, 0x0394 }, /*                 Greek_DELTA Δ GREEK CAPITAL LETTER DELTA */
    { 0x07c5, false, 0x0395 }, /*               Greek_EPSILON Ε GREEK CAPITAL LETTER EPSILON */
    { 0x07c6, false, 0x0396 }, /*                  Greek_ZETA Ζ GREEK CAPITAL LETTER ZETA */
    { 0x07c7, false, 0x0397 }, /*                   Greek_ETA Η GREEK CAPITAL LETTER ETA */
    { 0x07c8, false, 0x0398 }, /*                 Greek_THETA Θ GREEK CAPITAL LETTER THETA */
    { 0x07c9, false, 0x0399 }, /*                  Greek_IOTA Ι GREEK CAPITAL LETTER IOTA */
    { 0x07ca, false, 0x039a }, /*                 Greek_KAPPA Κ GREEK CAPITAL LETTER KAPPA */
    { 0x07cb, false, 0x039b }, /*                Greek_LAMBDA Λ GREEK CAPITAL LETTER LAMDA */
    { 0x07cc, false, 0x039c }, /*                    Greek_MU Μ GREEK CAPITAL LETTER MU */
    { 0x07cd, false, 0x039d }, /*                    Greek_NU Ν GREEK CAPITAL LETTER NU */
    { 0x07ce, false, 0x039e }, /*                    Greek_XI Ξ GREEK CAPITAL LETTER XI */
    { 0x07cf, false, 0x039f }, /*               Greek_OMICRON Ο GREEK CAPITAL LETTER OMICRON */
    { 0x07d0, false, 0x03a0 }, /*                    Greek_PI Π GREEK CAPITAL LETTER PI */
    { 0x07d1, false, 0x03a1 }, /*                   Greek_RHO Ρ GREEK CAPITAL LETTER RHO */
    { 0x07d2, false, 0x03a3 }, /*                 Greek_SIGMA Σ GREEK CAPITAL LETTER SIGMA */
    { 0x07d4, false, 0x03a4 }, /*                   Greek_TAU Τ GREEK CAPITAL LETTER TAU */
    { 0x07d5, false, 0x03a5 }, /*               Greek_UPSILON Υ GREEK CAPITAL LETTER UPSILON */
    { 0x07d6, false, 0x03a6 }, /*                   Greek_PHI Φ GREEK CAPITAL LETTER PHI */
    { 0x07d7, false, 0x03a7 }, /*                   Greek_CHI Χ GREEK CAPITAL LETTER CHI */
    { 0x07d8, false, 0x03a8 }, /*                   Greek_PSI Ψ GREEK CAPITAL LETTER PSI */
    { 0x07d9, false, 0x03a9 }, /*                 Greek_OMEGA Ω GREEK CAPITAL LETTER OMEGA */
    { 0x07e1, false, 0x03b1 }, /*                 Greek_alpha α GREEK SMALL LETTER ALPHA */
    { 0x07e2, false, 0x03b2 }, /*                  Greek_beta β GREEK SMALL LETTER BETA */
    { 0x07e3, false, 0x03b3 }, /*                 Greek_gamma γ GREEK SMALL LETTER GAMMA */
    { 0x07e4, false, 0x03b4 }, /*                 Greek_delta δ GREEK SMALL LETTER DELTA */
    { 0x07e5, false, 0x03b5 }, /*               Greek_epsilon ε GREEK SMALL LETTER EPSILON */
    { 0x07e6, false, 0x03b6 }, /*                  Greek_zeta ζ GREEK SMALL LETTER ZETA */
    { 0x07e7, false, 0x03b7 }, /*                   Greek_eta η GREEK SMALL LETTER ETA */
    { 0x07e8, false, 0x03b8 }, /*                 Greek_theta θ GREEK SMALL LETTER THETA */
    { 0x07e9, false, 0x03b9 }, /*                  Greek_iota ι GREEK SMALL LETTER IOTA */
    { 0x07ea, false, 0x03ba }, /*                 Greek_kappa κ GREEK SMALL LETTER KAPPA */
    { 0x07eb, false, 0x03bb }, /*                Greek_lambda λ GREEK SMALL LETTER LAMDA */
    { 0x07ec, false, 0x03bc }, /*                    Greek_mu μ GREEK SMALL LETTER MU */
    { 0x07ed, false, 0x03bd }, /*                    Greek_nu ν GREEK SMALL LETTER NU */
    { 0x07ee, false, 0x03be }, /*                    Greek_xi ξ GREEK SMALL LETTER XI */
    { 0x07ef, false, 0x03bf }, /*               Greek_omicron ο GREEK SMALL LETTER OMICRON */
    { 0x07f0, false, 0x03c0 }, /*                    Greek_pi π GREEK SMALL LETTER PI */
    { 0x07f1, false, 0x03c1 }, /*                   Greek_rho ρ GREEK SMALL LETTER RHO */
    { 0x07f2, false, 0x03c3 }, /*                 Greek_sigma σ GREEK SMALL LETTER SIGMA */
    { 0x07f3, false, 0x03c2 }, /*       Greek_finalsmallsigma ς GREEK SMALL LETTER FINAL SIGMA */
    { 0x07f4, false, 0x03c4 }, /*                   Greek_tau τ GREEK SMALL LETTER TAU */
    { 0x07f5, false, 0x03c5 }, /*               Greek_upsilon υ GREEK SMALL LETTER UPSILON */
    { 0x07f6, false, 0x03c6 }, /*                   Greek_phi φ GREEK SMALL LETTER PHI */
    { 0x07f7, false, 0x03c7 }, /*                   Greek_chi χ GREEK SMALL LETTER CHI */
    { 0x07f8, false, 0x03c8 }, /*                   Greek_psi ψ GREEK SMALL LETTER PSI */
    { 0x07f9, false, 0x03c9 }, /*                 Greek_omega ω GREEK SMALL LETTER OMEGA */
    { 0x08a1, false, 0x23b7 }, /*                 leftradical ⎷ ??? */
    { 0x08a2, true,  0x250c }, /*              topleftradical ┌ BOX DRAWINGS LIGHT DOWN AND RIGHT */
    { 0x08a3, true,  0x2500 }, /*              horizconnector ─ BOX DRAWINGS LIGHT HORIZONTAL */
    { 0x08a4, false, 0x2320 }, /*                 topintegral ⌠ TOP HALF INTEGRAL */
    { 0x08a5, false, 0x2321 }, /*                 botintegral ⌡ BOTTOM HALF INTEGRAL */
    { 0x08a6, true,  0x2502 }, /*               vertconnector │ BOX DRAWINGS LIGHT VERTICAL */
    { 0x08a7, false, 0x23a1 }, /*            topleftsqbracket ⎡ ??? */
    { 0x08a8, false, 0x23a3 }, /*            botleftsqbracket ⎣ ??? */
    { 0x08a9, false, 0x23a4 }, /*           toprightsqbracket ⎤ ??? */
    { 0x08aa, false, 0x23a6 }, /*           botrightsqbracket ⎦ ??? */
    { 0x08ab, false, 0x239b }, /*               topleftparens ⎛ ??? */
    { 0x08ac, false, 0x239d }, /*               botleftparens ⎝ ??? */
    { 0x08ad, false, 0x239e }, /*              toprightparens ⎞ ??? */
    { 0x08ae, false, 0x23a0 }, /*              botrightparens ⎠ ??? */
    { 0x08af, false, 0x23a8 }, /*        leftmiddlecurlybrace ⎨ ??? */
    { 0x08b0, false, 0x23ac }, /*       rightmiddlecurlybrace ⎬ ??? */
    /*  0x08b1                               topleftsummation ? ??? */
    /*  0x08b2                               botleftsummation ? ??? */
    /*  0x08b3                      topvertsummationconnector ? ??? */
    /*  0x08b4                      botvertsummationconnector ? ??? */
    /*  0x08b5                              toprightsummation ? ??? */
    /*  0x08b6                              botrightsummation ? ??? */
    /*  0x08b7                           rightmiddlesummation ? ??? */
    { 0x08bc, false, 0x2264 }, /*               lessthanequal ≤ LESS-THAN OR EQUAL TO */
    { 0x08bd, false, 0x2260 }, /*                    notequal ≠ NOT EQUAL TO */
    { 0x08be, false, 0x2265 }, /*            greaterthanequal ≥ GREATER-THAN OR EQUAL TO */
    { 0x08bf, false, 0x222b }, /*                    integral ∫ INTEGRAL */
    { 0x08c0, false, 0x2234 }, /*                   therefore ∴ THEREFORE */
    { 0x08c1, false, 0x221d }, /*                   variation ∝ PROPORTIONAL TO */
    { 0x08c2, false, 0x221e }, /*                    infinity ∞ INFINITY */
    { 0x08c5, false, 0x2207 }, /*                       nabla ∇ NABLA */
    { 0x08c8, false, 0x223c }, /*                 approximate ∼ TILDE OPERATOR */
    { 0x08c9, false, 0x2243 }, /*                similarequal ≃ ASYMPTOTICALLY EQUAL TO */
    { 0x08cd, false, 0x21d4 }, /*                    ifonlyif ⇔ LEFT RIGHT DOUBLE ARROW */
    { 0x08ce, false, 0x21d2 }, /*                     implies ⇒ RIGHTWARDS DOUBLE ARROW */
    { 0x08cf, false, 0x2261 }, /*                   identical ≡ IDENTICAL TO */
    { 0x08d6, false, 0x221a }, /*                     radical √ SQUARE ROOT */
    { 0x08da, false, 0x2282 }, /*                  includedin ⊂ SUBSET OF */
    { 0x08db, false, 0x2283 }, /*                    includes ⊃ SUPERSET OF */
    { 0x08dc, false, 0x2229 }, /*                intersection ∩ INTERSECTION */
    { 0x08dd, false, 0x222a }, /*                       union ∪ UNION */
    { 0x08de, false, 0x2227 }, /*                  logicaland ∧ LOGICAL AND */
    { 0x08df, false, 0x2228 }, /*                   logicalor ∨ LOGICAL OR */
    { 0x08ef, false, 0x2202 }, /*           partialderivative ∂ PARTIAL DIFFERENTIAL */
    { 0x08f6, false, 0x0192 }, /*                    function ƒ LATIN SMALL LETTER F WITH HOOK */
    { 0x08fb, false, 0x2190 }, /*                   leftarrow ← LEFTWARDS ARROW */
    { 0x08fc, false, 0x2191 }, /*                     uparrow ↑ UPWARDS ARROW */
    { 0x08fd, false, 0x2192 }, /*                  rightarrow → RIGHTWARDS ARROW */
    { 0x08fe, false, 0x2193 }, /*                   downarrow ↓ DOWNWARDS ARROW */
/*  0x09df                                              blank ? ??? */
    { 0x09e0, false, 0x25c6 }, /*                soliddiamond ◆ BLACK DIAMOND */
    { 0x09e1, false, 0x2592 }, /*                checkerboard ▒ MEDIUM SHADE */
    { 0x09e2, false, 0x2409 }, /*                          ht ␉ SYMBOL FOR HORIZONTAL TABULATION */
    { 0x09e3, false, 0x240c }, /*                          ff ␌ SYMBOL FOR FORM FEED */
    { 0x09e4, false, 0x240d }, /*                          cr ␍ SYMBOL FOR CARRIAGE RETURN */
    { 0x09e5, false, 0x240a }, /*                          lf ␊ SYMBOL FOR LINE FEED */
    { 0x09e8, false, 0x2424 }, /*                          nl ␤ SYMBOL FOR NEWLINE */
    { 0x09e9, false, 0x240b }, /*                          vt ␋ SYMBOL FOR VERTICAL TABULATION */
    { 0x09ea, false, 0x2518 }, /*              lowrightcorner ┘ BOX DRAWINGS LIGHT UP AND LEFT */
    { 0x09eb, false, 0x2510 }, /*               uprightcorner ┐ BOX DRAWINGS LIGHT DOWN AND LEFT */
    { 0x09ec, false, 0x250c }, /*                upleftcorner ┌ BOX DRAWINGS LIGHT DOWN AND RIGHT */
    { 0x09ed, false, 0x2514 }, /*               lowleftcorner └ BOX DRAWINGS LIGHT UP AND RIGHT */
    { 0x09ee, false, 0x253c }, /*               crossinglines ┼ BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL */
    { 0x09ef, false, 0x23ba }, /*              horizlinescan1 ⎺ HORIZONTAL SCAN LINE-1 (Unicode 3.2 draft) */
    { 0x09f0, false, 0x23bb }, /*              horizlinescan3 ⎻ HORIZONTAL SCAN LINE-3 (Unicode 3.2 draft) */
    { 0x09f1, false, 0x2500 }, /*              horizlinescan5 ─ BOX DRAWINGS LIGHT HORIZONTAL */
    { 0x09f2, false, 0x23bc }, /*              horizlinescan7 ⎼ HORIZONTAL SCAN LINE-7 (Unicode 3.2 draft) */
    { 0x09f3, false, 0x23bd }, /*              horizlinescan9 ⎽ HORIZONTAL SCAN LINE-9 (Unicode 3.2 draft) */
    { 0x09f4, false, 0x251c }, /*                       leftt ├ BOX DRAWINGS LIGHT VERTICAL AND RIGHT */
    { 0x09f5, false, 0x2524 }, /*                      rightt ┤ BOX DRAWINGS LIGHT VERTICAL AND LEFT */
    { 0x09f6, false, 0x2534 }, /*                        bott ┴ BOX DRAWINGS LIGHT UP AND HORIZONTAL */
    { 0x09f7, false, 0x252c }, /*                        topt ┬ BOX DRAWINGS LIGHT DOWN AND HORIZONTAL */
    { 0x09f8, false, 0x2502 }, /*                     vertbar │ BOX DRAWINGS LIGHT VERTICAL */
    { 0x0aa1, false, 0x2003 }, /*                     emspace   EM SPACE */
    { 0x0aa2, false, 0x2002 }, /*                     enspace   EN SPACE */
    { 0x0aa3, false, 0x2004 }, /*                    em3space   THREE-PER-EM SPACE */
    { 0x0aa4, false, 0x2005 }, /*                    em4space   FOUR-PER-EM SPACE */
    { 0x0aa5, false, 0x2007 }, /*                  digitspace   FIGURE SPACE */
    { 0x0aa6, false, 0x2008 }, /*                  punctspace   PUNCTUATION SPACE */
    { 0x0aa7, false, 0x2009 }, /*                   thinspace   THIN SPACE */
    { 0x0aa8, false, 0x200a }, /*                   hairspace   HAIR SPACE */
    { 0x0aa9, false, 0x2014 }, /*                      emdash — EM DASH */
    { 0x0aaa, false, 0x2013 }, /*                      endash – EN DASH */
    { 0x0aac, true,  0x2423 }, /*                 signifblank ␣ OPEN BOX */
    { 0x0aae, false, 0x2026 }, /*                    ellipsis … HORIZONTAL ELLIPSIS */
    { 0x0aaf, false, 0x2025 }, /*             doubbaselinedot ‥ TWO DOT LEADER */
    { 0x0ab0, false, 0x2153 }, /*                    onethird ⅓ VULGAR FRACTION ONE THIRD */
    { 0x0ab1, false, 0x2154 }, /*                   twothirds ⅔ VULGAR FRACTION TWO THIRDS */
    { 0x0ab2, false, 0x2155 }, /*                    onefifth ⅕ VULGAR FRACTION ONE FIFTH */
    { 0x0ab3, false, 0x2156 }, /*                   twofifths ⅖ VULGAR FRACTION TWO FIFTHS */
    { 0x0ab4, false, 0x2157 }, /*                 threefifths ⅗ VULGAR FRACTION THREE FIFTHS */
    { 0x0ab5, false, 0x2158 }, /*                  fourfifths ⅘ VULGAR FRACTION FOUR FIFTHS */
    { 0x0ab6, false, 0x2159 }, /*                    onesixth ⅙ VULGAR FRACTION ONE SIXTH */
    { 0x0ab7, false, 0x215a }, /*                  fivesixths ⅚ VULGAR FRACTION FIVE SIXTHS */
    { 0x0ab8, false, 0x2105 }, /*                      careof ℅ CARE OF */
    { 0x0abb, false, 0x2012 }, /*                     figdash ‒ FIGURE DASH */
    { 0x0abc, true,  0x27e8 }, /*            leftanglebracket ⟨ MATHEMATICAL LEFT ANGLE BRACKET */
    { 0x0abd, true,  0x002e }, /*                decimalpoint . FULL STOP */
    { 0x0abe, true,  0x27e9 }, /*           rightanglebracket ⟩ MATHEMATICAL RIGHT ANGLE BRACKET */
    /*  0x0abf                                         marker ? ??? */
    { 0x0ac3, false, 0x215b }, /*                   oneeighth ⅛ VULGAR FRACTION ONE EIGHTH */
    { 0x0ac4, false, 0x215c }, /*                threeeighths ⅜ VULGAR FRACTION THREE EIGHTHS */
    { 0x0ac5, false, 0x215d }, /*                 fiveeighths ⅝ VULGAR FRACTION FIVE EIGHTHS */
    { 0x0ac6, false, 0x215e }, /*                seveneighths ⅞ VULGAR FRACTION SEVEN EIGHTHS */
    { 0x0ac9, false, 0x2122 }, /*                   trademark ™ TRADE MARK SIGN */
    { 0x0aca, true,  0x2613 }, /*               signaturemark ☓ SALTIRE */
    /*  0x0acb                              trademarkincircle ? ??? */
    { 0x0acc, true,  0x25c1 }, /*            leftopentriangle ◁ WHITE LEFT-POINTING TRIANGLE */
    { 0x0acd, true,  0x25b7 }, /*           rightopentriangle ▷ WHITE RIGHT-POINTING TRIANGLE */
    { 0x0ace, true,  0x25cb }, /*                emopencircle ○ WHITE CIRCLE */
    { 0x0acf, true,  0x25af }, /*             emopenrectangle ▯ WHITE VERTICAL RECTANGLE */
    { 0x0ad0, false, 0x2018 }, /*         leftsinglequotemark ‘ LEFT SINGLE QUOTATION MARK */
    { 0x0ad1, false, 0x2019 }, /*        rightsinglequotemark ’ RIGHT SINGLE QUOTATION MARK */
    { 0x0ad2, false, 0x201c }, /*         leftdoublequotemark “ LEFT DOUBLE QUOTATION MARK */
    { 0x0ad3, false, 0x201d }, /*        rightdoublequotemark ” RIGHT DOUBLE QUOTATION MARK */
    { 0x0ad4, false, 0x211e }, /*                prescription ℞ PRESCRIPTION TAKE */
    { 0x0ad5, false, 0x2030 }, /*                    permille ‰ PER MILLE SIGN */
    { 0x0ad6, false, 0x2032 }, /*                     minutes ′ PRIME */
    { 0x0ad7, false, 0x2033 }, /*                     seconds ″ DOUBLE PRIME */
    { 0x0ad9, false, 0x271d }, /*                  latincross ✝ LATIN CROSS */
    /*  0x0ada                                       hexagram ? ??? */
    { 0x0adb, true,  0x25ac }, /*            filledrectbullet ▬ BLACK RECTANGLE */
    { 0x0adc, true,  0x25c0 }, /*         filledlefttribullet ◀ BLACK LEFT-POINTING TRIANGLE */
    { 0x0add, true,  0x25b6 }, /*        filledrighttribullet ▶ BLACK RIGHT-POINTING TRIANGLE */
    { 0x0ade, true,  0x25cf }, /*              emfilledcircle ● BLACK CIRCLE */
    { 0x0adf, true,  0x25ae }, /*                emfilledrect ▮ BLACK VERTICAL RECTANGLE */
    { 0x0ae0, true,  0x25e6 }, /*            enopencircbullet ◦ WHITE BULLET */
    { 0x0ae1, true,  0x25ab }, /*          enopensquarebullet ▫ WHITE SMALL SQUARE */
    { 0x0ae2, true,  0x25ad }, /*              openrectbullet ▭ WHITE RECTANGLE */
    { 0x0ae3, true,  0x25b3 }, /*             opentribulletup △ WHITE UP-POINTING TRIANGLE */
    { 0x0ae4, true,  0x25bd }, /*           opentribulletdown ▽ WHITE DOWN-POINTING TRIANGLE */
    { 0x0ae5, true,  0x2606 }, /*                    openstar ☆ WHITE STAR */
    { 0x0ae6, true,  0x2022 }, /*          enfilledcircbullet • BULLET */
    { 0x0ae7, true,  0x25aa }, /*            enfilledsqbullet ▪ BLACK SMALL SQUARE */
    { 0x0ae8, true,  0x25b2 }, /*           filledtribulletup ▲ BLACK UP-POINTING TRIANGLE */
    { 0x0ae9, true,  0x25bc }, /*         filledtribulletdown ▼ BLACK DOWN-POINTING TRIANGLE */
    { 0x0aea, true,  0x261c }, /*                 leftpointer ☜ WHITE LEFT POINTING INDEX */
    { 0x0aeb, true,  0x261e }, /*                rightpointer ☞ WHITE RIGHT POINTING INDEX */
    { 0x0aec, false, 0x2663 }, /*                        club ♣ BLACK CLUB SUIT */
    { 0x0aed, false, 0x2666 }, /*                     diamond ♦ BLACK DIAMOND SUIT */
    { 0x0aee, false, 0x2665 }, /*                       heart ♥ BLACK HEART SUIT */
    { 0x0af0, false, 0x2720 }, /*                maltesecross ✠ MALTESE CROSS */
    { 0x0af1, false, 0x2020 }, /*                      dagger † DAGGER */
    { 0x0af2, false, 0x2021 }, /*                doubledagger ‡ DOUBLE DAGGER */
    { 0x0af3, false, 0x2713 }, /*                   checkmark ✓ CHECK MARK */
    { 0x0af4, false, 0x2717 }, /*                 ballotcross ✗ BALLOT X */
    { 0x0af5, false, 0x266f }, /*                musicalsharp ♯ MUSIC SHARP SIGN */
    { 0x0af6, false, 0x266d }, /*                 musicalflat ♭ MUSIC FLAT SIGN */
    { 0x0af7, false, 0x2642 }, /*                  malesymbol ♂ MALE SIGN */
    { 0x0af8, false, 0x2640 }, /*                femalesymbol ♀ FEMALE SIGN */
    { 0x0af9, false, 0x260e }, /*                   telephone ☎ BLACK TELEPHONE */
    { 0x0afa, false, 0x2315 }, /*           telephonerecorder ⌕ TELEPHONE RECORDER */
    { 0x0afb, false, 0x2117 }, /*         phonographcopyright ℗ SOUND RECORDING COPYRIGHT */
    { 0x0afc, false, 0x2038 }, /*                       caret ‸ CARET */
    { 0x0afd, false, 0x201a }, /*          singlelowquotemark ‚ SINGLE LOW-9 QUOTATION MARK */
    { 0x0afe, false, 0x201e }, /*          doublelowquotemark „ DOUBLE LOW-9 QUOTATION MARK */
    /*  0x0aff                                         cursor ? ??? */
    { 0x0ba3, true,  0x003c }, /*                   leftcaret < LESS-THAN SIGN */
    { 0x0ba6, true,  0x003e }, /*                  rightcaret > GREATER-THAN SIGN */
    { 0x0ba8, true,  0x2228 }, /*                   downcaret ∨ LOGICAL OR */
    { 0x0ba9, true,  0x2227 }, /*                     upcaret ∧ LOGICAL AND */
    { 0x0bc0, true,  0x00af }, /*                     overbar ¯ MACRON */
    { 0x0bc2, false, 0x22a4 }, /*                    downtack ⊤ DOWN TACK */
    { 0x0bc3, true,  0x2229 }, /*                      upshoe ∩ INTERSECTION */
    { 0x0bc4, false, 0x230a }, /*                   downstile ⌊ LEFT FLOOR */
    { 0x0bc6, true,  0x005f }, /*                    underbar _ LOW LINE */
    { 0x0bca, false, 0x2218 }, /*                         jot ∘ RING OPERATOR */
    { 0x0bcc, false, 0x2395 }, /*                        quad ⎕ APL FUNCTIONAL SYMBOL QUAD (Unicode 3.0) */
    { 0x0bce, false, 0x22a5 }, /*                      uptack ⊥ UP TACK */
    { 0x0bcf, false, 0x25cb }, /*                      circle ○ WHITE CIRCLE */
    { 0x0bd3, false, 0x2308 }, /*                     upstile ⌈ LEFT CEILING */
    { 0x0bd6, true,  0x222a }, /*                    downshoe ∪ UNION */
    { 0x0bd8, true,  0x2283 }, /*                   rightshoe ⊃ SUPERSET OF */
    { 0x0bda, true,  0x2282 }, /*                    leftshoe ⊂ SUBSET OF */
    { 0x0bdc, false, 0x22a3 }, /*                    lefttack ⊣ LEFT TACK */
    { 0x0bfc, false, 0x22a2 }, /*                   righttack ⊢ RIGHT TACK */
    { 0x0cdf, false, 0x2017 }, /*        hebrew_doublelowline ‗ DOUBLE LOW LINE */
    { 0x0ce0, false, 0x05d0 }, /*                hebrew_aleph א HEBREW LETTER ALEF */
    { 0x0ce1, false, 0x05d1 }, /*                  hebrew_bet ב HEBREW LETTER BET */
    { 0x0ce2, false, 0x05d2 }, /*                hebrew_gimel ג HEBREW LETTER GIMEL */
    { 0x0ce3, false, 0x05d3 }, /*                hebrew_dalet ד HEBREW LETTER DALET */
    { 0x0ce4, false, 0x05d4 }, /*                   hebrew_he ה HEBREW LETTER HE */
    { 0x0ce5, false, 0x05d5 }, /*                  hebrew_waw ו HEBREW LETTER VAV */
    { 0x0ce6, false, 0x05d6 }, /*                 hebrew_zain ז HEBREW LETTER ZAYIN */
    { 0x0ce7, false, 0x05d7 }, /*                 hebrew_chet ח HEBREW LETTER HET */
    { 0x0ce8, false, 0x05d8 }, /*                  hebrew_tet ט HEBREW LETTER TET */
    { 0x0ce9, false, 0x05d9 }, /*                  hebrew_yod י HEBREW LETTER YOD */
    { 0x0cea, false, 0x05da }, /*            hebrew_finalkaph ך HEBREW LETTER FINAL KAF */
    { 0x0ceb, false, 0x05db }, /*                 hebrew_kaph כ HEBREW LETTER KAF */
    { 0x0cec, false, 0x05dc }, /*                hebrew_lamed ל HEBREW LETTER LAMED */
    { 0x0ced, false, 0x05dd }, /*             hebrew_finalmem ם HEBREW LETTER FINAL MEM */
    { 0x0cee, false, 0x05de }, /*                  hebrew_mem מ HEBREW LETTER MEM */
    { 0x0cef, false, 0x05df }, /*             hebrew_finalnun ן HEBREW LETTER FINAL NUN */
    { 0x0cf0, false, 0x05e0 }, /*                  hebrew_nun נ HEBREW LETTER NUN */
    { 0x0cf1, false, 0x05e1 }, /*               hebrew_samech ס HEBREW LETTER SAMEKH */
    { 0x0cf2, false, 0x05e2 }, /*                 hebrew_ayin ע HEBREW LETTER AYIN */
    { 0x0cf3, false, 0x05e3 }, /*              hebrew_finalpe ף HEBREW LETTER FINAL PE */
    { 0x0cf4, false, 0x05e4 }, /*                   hebrew_pe פ HEBREW LETTER PE */
    { 0x0cf5, false, 0x05e5 }, /*            hebrew_finalzade ץ HEBREW LETTER FINAL TSADI */
    { 0x0cf6, false, 0x05e6 }, /*                 hebrew_zade צ HEBREW LETTER TSADI */
    { 0x0cf7, false, 0x05e7 }, /*                 hebrew_qoph ק HEBREW LETTER QOF */
    { 0x0cf8, false, 0x05e8 }, /*                 hebrew_resh ר HEBREW LETTER RESH */
    { 0x0cf9, false, 0x05e9 }, /*                 hebrew_shin ש HEBREW LETTER SHIN */
    { 0x0cfa, false, 0x05ea }, /*                  hebrew_taw ת HEBREW LETTER TAV */
    { 0x0da1, false, 0x0e01 }, /*                  Thai_kokai ก THAI CHARACTER KO KAI */
    { 0x0da2, false, 0x0e02 }, /*                Thai_khokhai ข THAI CHARACTER KHO KHAI */
    { 0x0da3, false, 0x0e03 }, /*               Thai_khokhuat ฃ THAI CHARACTER KHO KHUAT */
    { 0x0da4, false, 0x0e04 }, /*               Thai_khokhwai ค THAI CHARACTER KHO KHWAI */
    { 0x0da5, false, 0x0e05 }, /*                Thai_khokhon ฅ THAI CHARACTER KHO KHON */
    { 0x0da6, false, 0x0e06 }, /*             Thai_khorakhang ฆ THAI CHARACTER KHO RAKHANG */
    { 0x0da7, false, 0x0e07 }, /*                 Thai_ngongu ง THAI CHARACTER NGO NGU */
    { 0x0da8, false, 0x0e08 }, /*                Thai_chochan จ THAI CHARACTER CHO CHAN */
    { 0x0da9, false, 0x0e09 }, /*               Thai_choching ฉ THAI CHARACTER CHO CHING */
    { 0x0daa, false, 0x0e0a }, /*               Thai_chochang ช THAI CHARACTER CHO CHANG */
    { 0x0dab, false, 0x0e0b }, /*                   Thai_soso ซ THAI CHARACTER SO SO */
    { 0x0dac, false, 0x0e0c }, /*                Thai_chochoe ฌ THAI CHARACTER CHO CHOE */
    { 0x0dad, false, 0x0e0d }, /*                 Thai_yoying ญ THAI CHARACTER YO YING */
    { 0x0dae, false, 0x0e0e }, /*                Thai_dochada ฎ THAI CHARACTER DO CHADA */
    { 0x0daf, false, 0x0e0f }, /*                Thai_topatak ฏ THAI CHARACTER TO PATAK */
    { 0x0db0, false, 0x0e10 }, /*                Thai_thothan ฐ THAI CHARACTER THO THAN */
    { 0x0db1, false, 0x0e11 }, /*          Thai_thonangmontho ฑ THAI CHARACTER THO NANGMONTHO */
    { 0x0db2, false, 0x0e12 }, /*             Thai_thophuthao ฒ THAI CHARACTER THO PHUTHAO */
    { 0x0db3, false, 0x0e13 }, /*                  Thai_nonen ณ THAI CHARACTER NO NEN */
    { 0x0db4, false, 0x0e14 }, /*                  Thai_dodek ด THAI CHARACTER DO DEK */
    { 0x0db5, false, 0x0e15 }, /*                  Thai_totao ต THAI CHARACTER TO TAO */
    { 0x0db6, false, 0x0e16 }, /*               Thai_thothung ถ THAI CHARACTER THO THUNG */
    { 0x0db7, false, 0x0e17 }, /*              Thai_thothahan ท THAI CHARACTER THO THAHAN */
    { 0x0db8, false, 0x0e18 }, /*               Thai_thothong ธ THAI CHARACTER THO THONG */
    { 0x0db9, false, 0x0e19 }, /*                   Thai_nonu น THAI CHARACTER NO NU */
    { 0x0dba, false, 0x0e1a }, /*               Thai_bobaimai บ THAI CHARACTER BO BAIMAI */
    { 0x0dbb, false, 0x0e1b }, /*                  Thai_popla ป THAI CHARACTER PO PLA */
    { 0x0dbc, false, 0x0e1c }, /*               Thai_phophung ผ THAI CHARACTER PHO PHUNG */
    { 0x0dbd, false, 0x0e1d }, /*                   Thai_fofa ฝ THAI CHARACTER FO FA */
    { 0x0dbe, false, 0x0e1e }, /*                Thai_phophan พ THAI CHARACTER PHO PHAN */
    { 0x0dbf, false, 0x0e1f }, /*                  Thai_fofan ฟ THAI CHARACTER FO FAN */
    { 0x0dc0, false, 0x0e20 }, /*             Thai_phosamphao ภ THAI CHARACTER PHO SAMPHAO */
    { 0x0dc1, false, 0x0e21 }, /*                   Thai_moma ม THAI CHARACTER MO MA */
    { 0x0dc2, false, 0x0e22 }, /*                  Thai_yoyak ย THAI CHARACTER YO YAK */
    { 0x0dc3, false, 0x0e23 }, /*                  Thai_rorua ร THAI CHARACTER RO RUA */
    { 0x0dc4, false, 0x0e24 }, /*                     Thai_ru ฤ THAI CHARACTER RU */
    { 0x0dc5, false, 0x0e25 }, /*                 Thai_loling ล THAI CHARACTER LO LING */
    { 0x0dc6, false, 0x0e26 }, /*                     Thai_lu ฦ THAI CHARACTER LU */
    { 0x0dc7, false, 0x0e27 }, /*                 Thai_wowaen ว THAI CHARACTER WO WAEN */
    { 0x0dc8, false, 0x0e28 }, /*                 Thai_sosala ศ THAI CHARACTER SO SALA */
    { 0x0dc9, false, 0x0e29 }, /*                 Thai_sorusi ษ THAI CHARACTER SO RUSI */
    { 0x0dca, false, 0x0e2a }, /*                  Thai_sosua ส THAI CHARACTER SO SUA */
    { 0x0dcb, false, 0x0e2b }, /*                  Thai_hohip ห THAI CHARACTER HO HIP */
    { 0x0dcc, false, 0x0e2c }, /*                Thai_lochula ฬ THAI CHARACTER LO CHULA */
    { 0x0dcd, false, 0x0e2d }, /*                   Thai_oang อ THAI CHARACTER O ANG */
    { 0x0dce, false, 0x0e2e }, /*               Thai_honokhuk ฮ THAI CHARACTER HO NOKHUK */
    { 0x0dcf, false, 0x0e2f }, /*              Thai_paiyannoi ฯ THAI CHARACTER PAIYANNOI */
    { 0x0dd0, false, 0x0e30 }, /*                  Thai_saraa ะ THAI CHARACTER SARA A */
    { 0x0dd1, false, 0x0e31 }, /*             Thai_maihanakat ั THAI CHARACTER MAI HAN-AKAT */
    { 0x0dd2, false, 0x0e32 }, /*                 Thai_saraaa า THAI CHARACTER SARA AA */
    { 0x0dd3, false, 0x0e33 }, /*                 Thai_saraam ำ THAI CHARACTER SARA AM */
    { 0x0dd4, false, 0x0e34 }, /*                  Thai_sarai ิ THAI CHARACTER SARA I */
    { 0x0dd5, false, 0x0e35 }, /*                 Thai_saraii ี THAI CHARACTER SARA II */
    { 0x0dd6, false, 0x0e36 }, /*                 Thai_saraue ึ THAI CHARACTER SARA UE */
    { 0x0dd7, false, 0x0e37 }, /*                Thai_sarauee ื THAI CHARACTER SARA UEE */
    { 0x0dd8, false, 0x0e38 }, /*                  Thai_sarau ุ THAI CHARACTER SARA U */
    { 0x0dd9, false, 0x0e39 }, /*                 Thai_sarauu ู THAI CHARACTER SARA UU */
    { 0x0dda, false, 0x0e3a }, /*                Thai_phinthu ฺ THAI CHARACTER PHINTHU */
    { 0x0dde, true,  0x0e3e }, /*      Thai_maihanakat_maitho ฾ ??? */
    { 0x0ddf, false, 0x0e3f }, /*                   Thai_baht ฿ THAI CURRENCY SYMBOL BAHT */
    { 0x0de0, false, 0x0e40 }, /*                  Thai_sarae เ THAI CHARACTER SARA E */
    { 0x0de1, false, 0x0e41 }, /*                 Thai_saraae แ THAI CHARACTER SARA AE */
    { 0x0de2, false, 0x0e42 }, /*                  Thai_sarao โ THAI CHARACTER SARA O */
    { 0x0de3, false, 0x0e43 }, /*          Thai_saraaimaimuan ใ THAI CHARACTER SARA AI MAIMUAN */
    { 0x0de4, false, 0x0e44 }, /*         Thai_saraaimaimalai ไ THAI CHARACTER SARA AI MAIMALAI */
    { 0x0de5, false, 0x0e45 }, /*            Thai_lakkhangyao ๅ THAI CHARACTER LAKKHANGYAO */
    { 0x0de6, false, 0x0e46 }, /*               Thai_maiyamok ๆ THAI CHARACTER MAIYAMOK */
    { 0x0de7, false, 0x0e47 }, /*              Thai_maitaikhu ็ THAI CHARACTER MAITAIKHU */
    { 0x0de8, false, 0x0e48 }, /*                  Thai_maiek ่ THAI CHARACTER MAI EK */
    { 0x0de9, false, 0x0e49 }, /*                 Thai_maitho ้ THAI CHARACTER MAI THO */
    { 0x0dea, false, 0x0e4a }, /*                 Thai_maitri ๊ THAI CHARACTER MAI TRI */
    { 0x0deb, false, 0x0e4b }, /*            Thai_maichattawa ๋ THAI CHARACTER MAI CHATTAWA */
    { 0x0dec, false, 0x0e4c }, /*            Thai_thanthakhat ์ THAI CHARACTER THANTHAKHAT */
    { 0x0ded, false, 0x0e4d }, /*               Thai_nikhahit ํ THAI CHARACTER NIKHAHIT */
    { 0x0df0, false, 0x0e50 }, /*                 Thai_leksun ๐ THAI DIGIT ZERO */
    { 0x0df1, false, 0x0e51 }, /*                Thai_leknung ๑ THAI DIGIT ONE */
    { 0x0df2, false, 0x0e52 }, /*                Thai_leksong ๒ THAI DIGIT TWO */
    { 0x0df3, false, 0x0e53 }, /*                 Thai_leksam ๓ THAI DIGIT THREE */
    { 0x0df4, false, 0x0e54 }, /*                  Thai_leksi ๔ THAI DIGIT FOUR */
    { 0x0df5, false, 0x0e55 }, /*                  Thai_lekha ๕ THAI DIGIT FIVE */
    { 0x0df6, false, 0x0e56 }, /*                 Thai_lekhok ๖ THAI DIGIT SIX */
    { 0x0df7, false, 0x0e57 }, /*                Thai_lekchet ๗ THAI DIGIT SEVEN */
    { 0x0df8, false, 0x0e58 }, /*                Thai_lekpaet ๘ THAI DIGIT EIGHT */
    { 0x0df9, false, 0x0e59 }, /*                 Thai_lekkao ๙ THAI DIGIT NINE */
    { 0x0ea1, false, 0x3131 }, /*               Hangul_Kiyeog ㄱ HANGUL LETTER KIYEOK */
    { 0x0ea2, false, 0x3132 }, /*          Hangul_SsangKiyeog ㄲ HANGUL LETTER SSANGKIYEOK */
    { 0x0ea3, false, 0x3133 }, /*           Hangul_KiyeogSios ㄳ HANGUL LETTER KIYEOK-SIOS */
    { 0x0ea4, false, 0x3134 }, /*                Hangul_Nieun ㄴ HANGUL LETTER NIEUN */
    { 0x0ea5, false, 0x3135 }, /*           Hangul_NieunJieuj ㄵ HANGUL LETTER NIEUN-CIEUC */
    { 0x0ea6, false, 0x3136 }, /*           Hangul_NieunHieuh ㄶ HANGUL LETTER NIEUN-HIEUH */
    { 0x0ea7, false, 0x3137 }, /*               Hangul_Dikeud ㄷ HANGUL LETTER TIKEUT */
    { 0x0ea8, false, 0x3138 }, /*          Hangul_SsangDikeud ㄸ HANGUL LETTER SSANGTIKEUT */
    { 0x0ea9, false, 0x3139 }, /*                Hangul_Rieul ㄹ HANGUL LETTER RIEUL */
    { 0x0eaa, false, 0x313a }, /*          Hangul_RieulKiyeog ㄺ HANGUL LETTER RIEUL-KIYEOK */
    { 0x0eab, false, 0x313b }, /*           Hangul_RieulMieum ㄻ HANGUL LETTER RIEUL-MIEUM */
    { 0x0eac, false, 0x313c }, /*           Hangul_RieulPieub ㄼ HANGUL LETTER RIEUL-PIEUP */
    { 0x0ead, false, 0x313d }, /*            Hangul_RieulSios ㄽ HANGUL LETTER RIEUL-SIOS */
    { 0x0eae, false, 0x313e }, /*           Hangul_RieulTieut ㄾ HANGUL LETTER RIEUL-THIEUTH */
    { 0x0eaf, false, 0x313f }, /*          Hangul_RieulPhieuf ㄿ HANGUL LETTER RIEUL-PHIEUPH */
    { 0x0eb0, false, 0x3140 }, /*           Hangul_RieulHieuh ㅀ HANGUL LETTER RIEUL-HIEUH */
    { 0x0eb1, false, 0x3141 }, /*                Hangul_Mieum ㅁ HANGUL LETTER MIEUM */
    { 0x0eb2, false, 0x3142 }, /*                Hangul_Pieub ㅂ HANGUL LETTER PIEUP */
    { 0x0eb3, false, 0x3143 }, /*           Hangul_SsangPieub ㅃ HANGUL LETTER SSANGPIEUP */
    { 0x0eb4, false, 0x3144 }, /*            Hangul_PieubSios ㅄ HANGUL LETTER PIEUP-SIOS */
    { 0x0eb5, false, 0x3145 }, /*                 Hangul_Sios ㅅ HANGUL LETTER SIOS */
    { 0x0eb6, false, 0x3146 }, /*            Hangul_SsangSios ㅆ HANGUL LETTER SSANGSIOS */
    { 0x0eb7, false, 0x3147 }, /*                Hangul_Ieung ㅇ HANGUL LETTER IEUNG */
    { 0x0eb8, false, 0x3148 }, /*                Hangul_Jieuj ㅈ HANGUL LETTER CIEUC */
    { 0x0eb9, false, 0x3149 }, /*           Hangul_SsangJieuj ㅉ HANGUL LETTER SSANGCIEUC */
    { 0x0eba, false, 0x314a }, /*                Hangul_Cieuc ㅊ HANGUL LETTER CHIEUCH */
    { 0x0ebb, false, 0x314b }, /*               Hangul_Khieuq ㅋ HANGUL LETTER KHIEUKH */
    { 0x0ebc, false, 0x314c }, /*                Hangul_Tieut ㅌ HANGUL LETTER THIEUTH */
    { 0x0ebd, false, 0x314d }, /*               Hangul_Phieuf ㅍ HANGUL LETTER PHIEUPH */
    { 0x0ebe, false, 0x314e }, /*                Hangul_Hieuh ㅎ HANGUL LETTER HIEUH */
    { 0x0ebf, false, 0x314f }, /*                    Hangul_A ㅏ HANGUL LETTER A */
    { 0x0ec0, false, 0x3150 }, /*                   Hangul_AE ㅐ HANGUL LETTER AE */
    { 0x0ec1, false, 0x3151 }, /*                   Hangul_YA ㅑ HANGUL LETTER YA */
    { 0x0ec2, false, 0x3152 }, /*                  Hangul_YAE ㅒ HANGUL LETTER YAE */
    { 0x0ec3, false, 0x3153 }, /*                   Hangul_EO ㅓ HANGUL LETTER EO */
    { 0x0ec4, false, 0x3154 }, /*                    Hangul_E ㅔ HANGUL LETTER E */
    { 0x0ec5, false, 0x3155 }, /*                  Hangul_YEO ㅕ HANGUL LETTER YEO */
    { 0x0ec6, false, 0x3156 }, /*                   Hangul_YE ㅖ HANGUL LETTER YE */
    { 0x0ec7, false, 0x3157 }, /*                    Hangul_O ㅗ HANGUL LETTER O */
    { 0x0ec8, false, 0x3158 }, /*                   Hangul_WA ㅘ HANGUL LETTER WA */
    { 0x0ec9, false, 0x3159 }, /*                  Hangul_WAE ㅙ HANGUL LETTER WAE */
    { 0x0eca, false, 0x315a }, /*                   Hangul_OE ㅚ HANGUL LETTER OE */
    { 0x0ecb, false, 0x315b }, /*                   Hangul_YO ㅛ HANGUL LETTER YO */
    { 0x0ecc, false, 0x315c }, /*                    Hangul_U ㅜ HANGUL LETTER U */
    { 0x0ecd, false, 0x315d }, /*                  Hangul_WEO ㅝ HANGUL LETTER WEO */
    { 0x0ece, false, 0x315e }, /*                   Hangul_WE ㅞ HANGUL LETTER WE */
    { 0x0ecf, false, 0x315f }, /*                   Hangul_WI ㅟ HANGUL LETTER WI */
    { 0x0ed0, false, 0x3160 }, /*                   Hangul_YU ㅠ HANGUL LETTER YU */
    { 0x0ed1, false, 0x3161 }, /*                   Hangul_EU ㅡ HANGUL LETTER EU */
    { 0x0ed2, false, 0x3162 }, /*                   Hangul_YI ㅢ HANGUL LETTER YI */
    { 0x0ed3, false, 0x3163 }, /*                    Hangul_I ㅣ HANGUL LETTER I */
    { 0x0ed4, false, 0x11a8 }, /*             Hangul_J_Kiyeog ᆨ HANGUL JONGSEONG KIYEOK */
    { 0x0ed5, false, 0x11a9 }, /*        Hangul_J_SsangKiyeog ᆩ HANGUL JONGSEONG SSANGKIYEOK */
    { 0x0ed6, false, 0x11aa }, /*         Hangul_J_KiyeogSios ᆪ HANGUL JONGSEONG KIYEOK-SIOS */
    { 0x0ed7, false, 0x11ab }, /*              Hangul_J_Nieun ᆫ HANGUL JONGSEONG NIEUN */
    { 0x0ed8, false, 0x11ac }, /*         Hangul_J_NieunJieuj ᆬ HANGUL JONGSEONG NIEUN-CIEUC */
    { 0x0ed9, false, 0x11ad }, /*         Hangul_J_NieunHieuh ᆭ HANGUL JONGSEONG NIEUN-HIEUH */
    { 0x0eda, false, 0x11ae }, /*             Hangul_J_Dikeud ᆮ HANGUL JONGSEONG TIKEUT */
    { 0x0edb, false, 0x11af }, /*              Hangul_J_Rieul ᆯ HANGUL JONGSEONG RIEUL */
    { 0x0edc, false, 0x11b0 }, /*        Hangul_J_RieulKiyeog ᆰ HANGUL JONGSEONG RIEUL-KIYEOK */
    { 0x0edd, false, 0x11b1 }, /*         Hangul_J_RieulMieum ᆱ HANGUL JONGSEONG RIEUL-MIEUM */
    { 0x0ede, false, 0x11b2 }, /*         Hangul_J_RieulPieub ᆲ HANGUL JONGSEONG RIEUL-PIEUP */
    { 0x0edf, false, 0x11b3 }, /*          Hangul_J_RieulSios ᆳ HANGUL JONGSEONG RIEUL-SIOS */
    { 0x0ee0, false, 0x11b4 }, /*         Hangul_J_RieulTieut ᆴ HANGUL JONGSEONG RIEUL-THIEUTH */
    { 0x0ee1, false, 0x11b5 }, /*        Hangul_J_RieulPhieuf ᆵ HANGUL JONGSEONG RIEUL-PHIEUPH */
    { 0x0ee2, false, 0x11b6 }, /*         Hangul_J_RieulHieuh ᆶ HANGUL JONGSEONG RIEUL-HIEUH */
    { 0x0ee3, false, 0x11b7 }, /*              Hangul_J_Mieum ᆷ HANGUL JONGSEONG MIEUM */
    { 0x0ee4, false, 0x11b8 }, /*              Hangul_J_Pieub ᆸ HANGUL JONGSEONG PIEUP */
    { 0x0ee5, false, 0x11b9 }, /*          Hangul_J_PieubSios ᆹ HANGUL JONGSEONG PIEUP-SIOS */
    { 0x0ee6, false, 0x11ba }, /*               Hangul_J_Sios ᆺ HANGUL JONGSEONG SIOS */
    { 0x0ee7, false, 0x11bb }, /*          Hangul_J_SsangSios ᆻ HANGUL JONGSEONG SSANGSIOS */
    { 0x0ee8, false, 0x11bc }, /*              Hangul_J_Ieung ᆼ HANGUL JONGSEONG IEUNG */
    { 0x0ee9, false, 0x11bd }, /*              Hangul_J_Jieuj ᆽ HANGUL JONGSEONG CIEUC */
    { 0x0eea, false, 0x11be }, /*              Hangul_J_Cieuc ᆾ HANGUL JONGSEONG CHIEUCH */
    { 0x0eeb, false, 0x11bf }, /*             Hangul_J_Khieuq ᆿ HANGUL JONGSEONG KHIEUKH */
    { 0x0eec, false, 0x11c0 }, /*              Hangul_J_Tieut ᇀ HANGUL JONGSEONG THIEUTH */
    { 0x0eed, false, 0x11c1 }, /*             Hangul_J_Phieuf ᇁ HANGUL JONGSEONG PHIEUPH */
    { 0x0eee, false, 0x11c2 }, /*              Hangul_J_Hieuh ᇂ HANGUL JONGSEONG HIEUH */
    { 0x0eef, false, 0x316d }, /*     Hangul_RieulYeorinHieuh ㅭ HANGUL LETTER RIEUL-YEORINHIEUH */
    { 0x0ef0, false, 0x3171 }, /*    Hangul_SunkyeongeumMieum ㅱ HANGUL LETTER KAPYEOUNMIEUM */
    { 0x0ef1, false, 0x3178 }, /*    Hangul_SunkyeongeumPieub ㅸ HANGUL LETTER KAPYEOUNPIEUP */
    { 0x0ef2, false, 0x317f }, /*              Hangul_PanSios ㅿ HANGUL LETTER PANSIOS */
    { 0x0ef3, false, 0x3181 }, /*    Hangul_KkogjiDalrinIeung ㆁ HANGUL LETTER YESIEUNG */
    { 0x0ef4, false, 0x3184 }, /*   Hangul_SunkyeongeumPhieuf ㆄ HANGUL LETTER KAPYEOUNPHIEUPH */
    { 0x0ef5, false, 0x3186 }, /*          Hangul_YeorinHieuh ㆆ HANGUL LETTER YEORINHIEUH */
    { 0x0ef6, false, 0x318d }, /*                Hangul_AraeA ㆍ HANGUL LETTER ARAEA */
    { 0x0ef7, false, 0x318e }, /*               Hangul_AraeAE ㆎ HANGUL LETTER ARAEAE */
    { 0x0ef8, false, 0x11eb }, /*            Hangul_J_PanSios ᇫ HANGUL JONGSEONG PANSIOS */
    { 0x0ef9, false, 0x11f0 }, /*  Hangul_J_KkogjiDalrinIeung ᇰ HANGUL JONGSEONG YESIEUNG */
    { 0x0efa, false, 0x11f9 }, /*        Hangul_J_YeorinHieuh ᇹ HANGUL JONGSEONG YEORINHIEUH */
    { 0x0eff, true,  0x20a9 }, /*                  Korean_Won ₩ WON SIGN */
    { 0x13bc, false, 0x0152 }, /*                          OE Œ LATIN CAPITAL LIGATURE OE */
    { 0x13bd, false, 0x0153 }, /*                          oe œ LATIN SMALL LIGATURE OE */
    { 0x13be, false, 0x0178 }, /*                  Ydiaeresis Ÿ LATIN CAPITAL LETTER Y WITH DIAERESIS */
    { 0x20ac, false, 0x20ac }, /*                    EuroSign € EURO SIGN */
};

/* binary search with range check */
static uint32_t
bin_search(const struct codepair *table, size_t length, xkb_keysym_t keysym)
{
    size_t first = 0;
    size_t last = length;

    if (keysym < table[0].keysym  || keysym > table[length].keysym)
        return 0;

    /* binary search in table */
    while (last >= first) {
        size_t mid = (first + last) / 2;
        if (table[mid].keysym < keysym)
            first = mid + 1;
        else if (table[mid].keysym > keysym)
            last = mid - 1;
        else /* found it */
            return table[mid].ucs;
    }

    /* no matching Unicode value found in table */
    return NO_KEYSYM_UNICODE_CONVERSION;
}

uint32_t
xkb_keysym_to_utf32(xkb_keysym_t keysym)
{
    /* first check for Latin-1 characters (1:1 mapping) */
    if ((keysym >= 0x0020 && keysym <= 0x007e) ||
        (keysym >= 0x00a0 && keysym <= 0x00ff))
        return keysym;

    /* patch encoding botch */
    if (keysym == XKB_KEY_KP_Space)
        return XKB_KEY_space & 0x7f;

    /* special keysyms */
    if ((keysym >= XKB_KEY_BackSpace && keysym <= XKB_KEY_Clear) ||
        (keysym >= XKB_KEY_KP_Multiply && keysym <= XKB_KEY_KP_9) ||
        keysym == XKB_KEY_Return || keysym == XKB_KEY_Escape ||
        keysym == XKB_KEY_Delete || keysym == XKB_KEY_KP_Tab ||
        keysym == XKB_KEY_KP_Enter || keysym == XKB_KEY_KP_Equal)
        return keysym & 0x7f;

    /* also check for directly encoded Unicode codepoints */

    /* Exclude surrogates: they are invalid in UTF-32.
     * See https://www.unicode.org/versions/Unicode15.0.0/ch03.pdf#G28875
     * for further details.
     */
    if (keysym >= XKB_KEYSYM_UNICODE_SURROGATE_MIN &&
        keysym <= XKB_KEYSYM_UNICODE_SURROGATE_MAX)
        return NO_KEYSYM_UNICODE_CONVERSION;
    /*
     * In theory, this is supposed to start from 0x100100, such that the ASCII
     * range, which is already covered by 0x00-0xff, can't be encoded in two
     * ways. However, changing this after a couple of decades probably won't
     * go well, so it stays as it is.
     */
    if (XKB_KEYSYM_UNICODE_OFFSET <= keysym && keysym <= XKB_KEYSYM_UNICODE_MAX)
        return keysym - XKB_KEYSYM_UNICODE_OFFSET;

    /* search main table */
    return bin_search(keysymtab, ARRAY_SIZE(keysymtab) - 1, keysym);
}

xkb_keysym_t
xkb_utf32_to_keysym(uint32_t ucs)
{
    /* first check for Latin-1 characters (1:1 mapping) */
    if ((ucs >= 0x0020 && ucs <= 0x007e) ||
        (ucs >= 0x00a0 && ucs <= 0x00ff))
        return ucs;

    /* special keysyms */
    if ((ucs >= (XKB_KEY_BackSpace & 0x7f) && ucs <= (XKB_KEY_Clear & 0x7f)) ||
        ucs == (XKB_KEY_Return & 0x7f) || ucs == (XKB_KEY_Escape & 0x7f))
        return ucs | 0xff00;
    if (ucs == (XKB_KEY_Delete & 0x7f))
        return XKB_KEY_Delete;

    /*
     * U+0000 NULL and invalid Unicode code points.
     * Surrogates are invalid in UTF-32.
     * See https://www.unicode.org/versions/Unicode15.0.0/ch03.pdf#G28875
     * for further details.
     */
    if (unlikely(ucs == 0 || is_surrogate(ucs) || ucs > 0x10ffff))
        return XKB_KEY_NoSymbol;

    /* search main table */
    for (size_t i = 0; i < ARRAY_SIZE(keysymtab); i++) {
        /* Do not use deprecated keysyms */
        if (keysymtab[i].ucs == ucs && !keysymtab[i].deprecated)
            return keysymtab[i].keysym;
    }

    /* Use direct encoding if everything else failed */
    return ucs | XKB_KEYSYM_UNICODE_OFFSET;
}

/*
 * SPDX-SnippetBegin
 * Copyright © 2012 Intel Corporation
 * SPDX-License-Identifier: MIT
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 */

int
xkb_keysym_to_utf8(xkb_keysym_t keysym, char *buffer, size_t size)
{
    uint32_t codepoint;

    /* Unicode code point takes up to 4 bytes in UTF-8, plus the
     * NULL-terminating byte. */
    if (size < XKB_KEYSYM_UTF8_MAX_SIZE)
        return -1;

    codepoint = xkb_keysym_to_utf32(keysym);

    if (codepoint == NO_KEYSYM_UNICODE_CONVERSION)
        return 0;

    return utf32_to_utf8(codepoint, buffer);
}

/* SPDX-SnippetEnd */
