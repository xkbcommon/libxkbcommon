@defgroup keysym-encoding Keysym encoding
@brief Description of the [keysyms](@ref xkb_keysym_t) encoding and the categories of keysyms

@sa Appendix A [“Keysym Encoding”][encoding] of the X Window System Protocol.
@sa Appendix C [“New keysyms”][new-keysyms] of the XKB Protocol Specification.

@tableofcontents{html:2}

# Encoding

@important Keysyms are **32-bit** integers whoose 3 most significant
bits are always set to zero.  Thus valid keysyms are in the range
`0 – 0x1fffffff` = @ref XKB_KEYSYM_MAX.

@note Throughout this documentation, keysyms values are viewed as four bytes,
numbered from most significant to least significant:
<dl>
<dt>Byte *1*</dt>
<dd>
The most significant 8 bits: 3 zero bits and the most-significant 5 bits of
the 29-bit effective value.
</dd>
<dt>Byte *2*</dt>
<dd>The next most-significant 8 bits</dd>
<dt>Byte *3*</dt>
<dd>The next most-significant 8 bits</dd>
<dt>Byte *4*</dt>
<dd>The least-significant 8 bits</dd>
</dl>

# Categories

There are 6 categories:

- @ref special-keysyms
- @ref latin-1-keysyms
- @ref legacy-keysyms
- @ref function-keysyms
- @ref unicode-keysyms
- @ref vendor-keysyms

## Overview

<table>
<thead>
<tr>
<th>Category</th>
<th>Subcategory</th>
<th>Byte 1</th>
<th>Byte 2</th>
<th>Byte 3</th>
<th>Byte 4</th>
<th>First keysym</th>
<th>Last keysym</th>
</tr>
</thead>
<tbody>
<tr>
<td>@ref special-keysyms</td>
<td></td>
<td>`0`</td>
<td>`0`</td>
<td>`0`</td>
<td>`0`</td>
<td colspan="2">[`NoSymbol`][NoSymbol]</td>
</tr>
<tr>
<td rowspan="2">@ref latin-1-keysyms</td>
<td>ASCII non-control characters</td>
<td rowspan="2">`0`</td>
<td rowspan="2">`0`</td>
<td rowspan="2">`0`</td>
<td>`0x20` – `0x7e`</td>
<td>[`space`](@ref XKB_KEY_space)</td>
<td>[`asciitilde`](@ref XKB_KEY_asciitilde)</td>
</tr>
<tr>
<td>Latin-1 Supplement</td>
<td>`0xa0` – `0xff`</td>
<td>[`nobreakspace`](@ref XKB_KEY_nobreakspace)</td>
<td>[`ydiaeresis`](@ref XKB_KEY_ydiaeresis)</td>
</tr>
<tr>
<td rowspan="20">@ref legacy-keysyms</td>
<td>[Latin-2]</td>
<td rowspan="20">`0`</td>
<td rowspan="20">`0`</td>
<td>`0x01`</td>
<td rowspan="20">`0x00` – `0xff`</td>
<td>[`Aogonek`](@ref XKB_KEY_Aogonek)</td>
<td>[`abovedot`](@ref XKB_KEY_abovedot)</td>
</tr>
<tr>
<td>[Latin-3]</td>
<td>`0x02`</td>
<td>[`Hstroke`](@ref XKB_KEY_Hstroke)</td>
<td>[`scircumflex`](@ref XKB_KEY_scircumflex)</td>
</tr>
<tr>
<td>[Latin-4]</td>
<td>`0x03`</td>
<td>[`kra`](@ref XKB_KEY_kra)</td>
<td>[`umacron`](@ref XKB_KEY_umacron)</td>
</tr>
<tr>
<td>Kana</td>
<td>`0x04`</td>
<td>[`overline`](@ref XKB_KEY_overline)</td>
<td>[`semivoicedsound`](@ref XKB_KEY_semivoicedsound)</td>
</tr>
<tr>
<td>Arabic</td>
<td>`0x05`</td>
<td>[`Arabic_comma`](@ref XKB_KEY_Arabic_comma)</td>
<td>[`Arabic_sukun`](@ref XKB_KEY_Arabic_sukun)</td>
</tr>
<tr>
<td>Cyrillic</td>
<td>`0x06`</td>
<td>[`Serbian_dje`](@ref XKB_KEY_Serbian_dje)</td>
<td>[`Cyrillic_HARDSIGN`](@ref XKB_KEY_Cyrillic_HARDSIGN)</td>
</tr>
<tr>
<td>Greek</td>
<td>`0x07`</td>
<td>[`Greek_ALPHAaccent`](@ref XKB_KEY_Greek_ALPHAaccent)</td>
<td>[`Greek_omega`](@ref XKB_KEY_Greek_omega)</td>
</tr>
<tr>
<td>Technical</td>
<td>`0x08`</td>
<td>[`leftradical`](@ref XKB_KEY_leftradical)</td>
<td>[`downarrow`](@ref XKB_KEY_downarrow)</td>
</tr>
<tr>
<td>Special</td>
<td>`0x09`</td>
<td>[`blank`](@ref XKB_KEY_blank)</td>
<td>[`vertbar`](@ref XKB_KEY_vertbar)</td>
</tr>
<tr>
<td>Publishing</td>
<td>`0x0a`</td>
<td>[`emspace`](@ref XKB_KEY_emspace)</td>
<td>[`cursor`](@ref XKB_KEY_cursor)</td>
</tr>
<tr>
<td>APL</td>
<td>`0x0b`</td>
<td>[`leftcaret`](@ref XKB_KEY_leftcaret)</td>
<td>[`righttack`](@ref XKB_KEY_righttack)</td>
</tr>
<tr>
<td>Hebrew</td>
<td>`0x0c`</td>
<td>[`hebrew_doublelowline`](@ref XKB_KEY_hebrew_doublelowline)</td>
<td>[`hebrew_taf`](@ref XKB_KEY_hebrew_taf)</td>
</tr>
<tr>
<td>Thai</td>
<td>`0x0d`</td>
<td>[`Thai_kokai`](@ref XKB_KEY_Thai_kokai)</td>
<td>[`Thai_lekkao`](@ref XKB_KEY_Thai_lekkao)</td>
</tr>
<tr>
<td>Korean</td>
<td>`0x0e`</td>
<td>[`Hangul_Kiyeog`](@ref XKB_KEY_Hangul_Kiyeog)</td>
<td>[`Korean_Won`](@ref XKB_KEY_Korean_Won)</td>
</tr>
<tr>
<td>[Latin-5]</td>
<td>`0x0f`</td>
<td></td>
<td></td>
</tr>
<tr>
<td>[Latin-6]</td>
<td>`0x10`</td>
<td></td>
<td></td>
</tr>
<tr>
<td>[Latin-7]</td>
<td>`0x11`</td>
<td></td>
<td></td>
</tr>
<tr>
<td>[Latin-8]</td>
<td>`0x12`</td>
<!--
<td>[`Wcircumflex`](@ref XKB_KEY_Wcircumflex)</td>
<td>[`ygrave`](@ref XKB_KEY_ygrave)</td>
-->
<td></td>
<td></td>
</tr>
<tr>
<td>[Latin-9]</td>
<td>`0x13`</td>
<td>[`OE`](@ref XKB_KEY_OE)</td>
<td>[`Ydiaeresis`](@ref XKB_KEY_Ydiaeresis)</td>
</tr>
<tr>
<td>Currency</td>
<td>`0x20`</td>
<td colspan="2">[`EuroSign`](@ref XKB_KEY_EuroSign)</td>
</tr>
<tr>
<td rowspan="3">@ref function-keysyms</td>
<td>IBM 3270 terminals</td>
<td rowspan="3">`0`</td>
<td rowspan="3">`0`</td>
<td>`0xfd`</td>
<td rowspan="3">`0x00` – `0xff`</td>
<td>[`3270_Duplicate`](@ref XKB_KEY_3270_Duplicate)</td>
<td>[`3270_Enter`](@ref XKB_KEY_3270_Enter)</td>
</tr>
<tr>
<td>Keyboard (XKB) Extension</td>
<td>`0xfe`</td>
<td>[`ISO_Lock`](@ref XKB_KEY_ISO_Lock)</td>
<td>[`Pointer_Drag5`](@ref XKB_KEY_Pointer_Drag5)</td>
</tr>
<tr>
<td>Keyboard</td>
<td>`0xff`</td>
<td>[`BackSpace`](@ref XKB_KEY_BackSpace)</td>
<td>[`Delete`](@ref XKB_KEY_Delete)</td>
</tr>
<tr>
<td>@ref special-keysyms</td>
<td></td>
<td>`0`</td>
<td>`0xff`</td>
<td>`0xff`</td>
<td>`0xff`</td>
<td colspan="2">[`VoidSymbol`][VoidSymbol]</td>
</tr>
<tr>
<td>@ref unicode-keysyms</td>
<td></td>
<td>`0x01`</td>
<td>`0x00` – `0x10`</td>
<td>`0x00` – `0xff`</td>
<td>`0x00` – `0xff`</td>
<td>`0x01000100`</td>
<td>`0x0110ffff`</td>
</tr>
<tr>
<td rowspan="7">@ref vendor-keysyms</td>
<td>DEC, HP, misc.</td>
<td>`0x10`</td>
<td>`0x00`</td>
<td rowspan="7">`0x00` – `0xff`</td>
<td rowspan="7">`0x00` – `0xff`</td>
<td>`0x10000000`</td>
<td>`0x1000ffff`</td>
</tr>
<tr>
<td></td>
<td>`0x10`</td>
<td>`0x01` – `0x03`</td>
<td>`0x10010000`</td>
<td>`0x1003ffff`</td>
</tr>
<tr>
<td>OSF</td>
<td>`0x10`</td>
<td>`0x04`</td>
<td>`0x10040000`</td>
<td>`0x1004ffff`</td>
</tr>
<tr>
<td>Sun</td>
<td>`0x10`</td>
<td>`0x05`</td>
<td>`0x10050000`</td>
<td>`0x1005ffff`</td>
</tr>
<tr>
<td></td>
<td>`0x10`</td>
<td>`0x06` – `0x07`</td>
<td>`0x10060000`</td>
<td>`0x1007ffff`</td>
</tr>
<tr>
<td>XFree86</td>
<td>`0x10`</td>
<td>`0x08`</td>
<td>`0x10080000`</td>
<td>`0x1008ffff`</td>
</tr>
<tr>
<td></td>
<td>`0x10` – `0x1f`</td>
<td>`0x09` – `0xff`</td>
<td>`0x10090000`</td>
<td>`0x1fffffff`</td>
</tr>
</tbody>
</table>

## Special keysyms

There are 2 special values: [`NoSymbol`][NoSymbol] and [`VoidSymbol`][VoidSymbol].
They are used to indicate the *absence of symbols*.

Byte 1 | Byte 2 | Byte 3	| Byte 4 | Hex. value   | Name
------ | ------ | ------ | -- --- | ------------ | -----
0      | 0      | 0      | 0      | `0x00000000` | [`NoSymbol`][NoSymbol]
0      | 255    | 255    | 255    | `0x00ffffff` | [`VoidSymbol`][VoidSymbol]

@sa Appendix A “[Special keysyms]” of the X Window System Protocol.

[NoSymbol]: @ref XKB_KEY_NoSymbol
[VoidSymbol]: @ref XKB_KEY_VoidSymbol
[Special keysyms]: https://xorg.freedesktop.org/archive/current/doc/xproto/x11protocol.html#Special_KEYSYMs

## Latin-1 keysyms

The [Latin-1] keysyms occupy the range `0x0020` – `0x007E` and `0x00a0` – `0x00ff` and represent
the [ISO 10646 / Unicode][Unicode] characters `U+0020` – `U+007E` and `U+00A0` – `U+00FF`, respectively.

@sa [ISO/IEC 8859-1][Latin-1] character encoding
@sa Appendix A “[Latin-1 keysyms]” of the X Window System Protocol.

[Latin-1 keysyms]: https://xorg.freedesktop.org/archive/current/doc/xproto/x11protocol.html#Latin_1_KEYSYMs

## Legacy keysyms

**Legacy keysyms** date from the time *before* [ISO 10646 / Unicode][Unicode] was available.
They represent characters from a number of different older 8-bit coded character sets and have zero
values for bytes 1 and 2. Byte 3 indicates a coded character set and byte 4 is the 8-bit value of the
particular character within that set.

<table>
<thead>
<tr>
<th colspan="3">Byte 3</th>
</tr>
<tr>
<th>Value</th>
<th>Character set</th>
<th>Comment</th>
</tr>
</thead>
<tbody>
<tr>
<td>1</td>
<td>[Latin-2]</td>
<td></td>
</tr>
<tr>
<td>2</td>
<td>[Latin-3]</td>
<td></td>
</tr>
<tr>
<td>3</td>
<td>[Latin-4]</td>
<td></td>
</tr>
<tr>
<td>4</td>
<td>Kana</td>
<td></td>
</tr>
<tr>
<td>5</td>
<td>Arabic	</td>
<td></td>
</tr>
<tr>
<td>6</td>
<td>Cyrillic</td>
<td></td>
</tr>
<tr>
<td>7</td>
<td>Greek</td>
<td></td>
</tr>
<tr>
<td>8</td>
<td>Technical</td>
<td>Based on [DEC Technical]&zwnj;: add `0x880` to the original code point</td>
</tr>
<tr>
<td>9</td>
<td>Special</td>
<td>Based on [DEC Special Graphics]&zwnj;: add `0x980` to the original code point</td>
</tr>
<tr>
<td>10</td>
<td>Publishing</td>
<td></td>
</tr>
<tr>
<td>11</td>
<td>APL</td>
<td></td>
</tr>
<tr>
<td>12</td>
<td>Hebrew</td>
<td></td>
</tr>
<tr>
<td>13</td>
<td>Thai</td>
<td></td>
</tr>
<tr>
<td>14</td>
<td>Korean</td>
<td></td>
</tr>
<tr>
<td>15</td>
<td>[Latin-5]</td>
<td></td>
</tr>
<tr>
<td>16</td>
<td>[Latin-6]</td>
<td></td>
</tr>
<tr>
<td>17</td>
<td>[Latin-7]</td>
<td></td>
</tr>
<tr>
<td>18</td>
<td>[Latin-8]</td>
<td></td>
</tr>
<tr>
<td>19</td>
<td>[Latin-9]</td>
<td></td>
</tr>
<tr>
<td>32</td>
<td>Currency</td>
<td></td>
</tr>
</tbody>
</table>

Each character set contains gaps where codes have been removed that were duplicates with codes in
previous character sets (that is, character sets with lesser byte 3 value).

@todo Legacy keysyms

@sa @ref latin-1-keysyms
@sa Appendix A “[Legacy keysyms]” of the X Window System Protocol.

[Legacy keysyms]: https://xorg.freedesktop.org/archive/current/doc/xproto/x11protocol.html#Legacy_KEYSYMs
[DEC Technical]: https://en.wikipedia.org/wiki/DEC_Technical_Character_Set
[DEC Special Graphics]: https://en.wikipedia.org/wiki/DEC_Special_Graphics


## Function keysyms

**Function keysyms** represent keycap symbols that do not directly represent elements of a coded
character set. Instead, they typically identify a software function, mode, or operation (e.g., cursor up,
caps lock, insert) that can be activated using a dedicated key. Function keysyms have zero values for
bytes 1 and 2. Byte 3 distinguishes between several 8-bit sets within which byte 4 identifies the
individual function key.

<table>
<thead>
<tr>
<th>Byte 1</th>
<th>Byte 2</th>
<th colspan="2">Byte 3</th>
<th>Byte 4</th>
</tr>
<tr>
<th>Value</th>
<th>Value</th>
<th>Value</th>
<th>Function set</th>
<th>Value</th>
</tr>
</thead>
<tbody>
<tr>
<td rowspan="3">0</td>
<td rowspan="3">0</td>
<td>253</td><td>IBM 3270 terminals</td>
<td rowspan="3">(individual function key)</td>
</tr>
<tr>
<td>254</td><td>Keyboard (XKB) Extension</td>
</tr>
<tr>
<td>255</td><td>Keyboard</td>
</tr>
</tbody>
</table>

@todo Function keysyms

@sa Appendix A “[Function keysyms]” of the X Window System Protocol.
@sa Appendix C “[New keysyms]” of the XKB Protocol Specification.

[Function keysyms]: https://xorg.freedesktop.org/archive/current/doc/xproto/x11protocol.html#Function_KEYSYMs
[New keysyms]: https://xorg.freedesktop.org/releases/X11R7.7/doc/kbproto/xkbproto.html#New_KeySyms

## Unicode keysyms

The **Unicode keysyms** occupy the range `0x01000100` – `0x0110FFFF` and represent the
[ISO 10646 / Unicode][Unicode] characters `U+0100` – `U+10FFFF`, respectively.
The numeric value of a Unicode keysym is the Unicode code point of the corresponding character plus
`0x01000000`. In the interest of backwards compatibility, clients should be able to process both the
Unicode keysym and the [legacy keysym](@ref legacy-keysyms) for those characters where both exist.

@note *Dead keys*, which place an accent on the next character entered, shall be encoded as
[function keysyms](@ref function-keysyms), and not as the Unicode keysym corresponding to an
equivalent [*combining* character](https://en.wikipedia.org/wiki/Combining_Diacritical_Marks).

@note Where a keycap indicates a specific *function* with a graphical symbol that is also available in Unicode
(e.g., an upwards arrow for the cursor up function), the appropriate [function keysym](@ref function-keysyms)
should be used, and not the Unicode keysym corresponding to the depicted symbol.

@sa Appendix A “[Unicode keysyms]” of the X Window System Protocol.
@sa [Unicode technical website](https://unicode.org/main.html).

[Unicode keysyms]: https://xorg.freedesktop.org/archive/current/doc/xproto/x11protocol.html#Unicode_KEYSYMs

## Vendor keysyms

**Vendor keysyms** are vendor-specific extentions in the range `0x10000000` – `0x1fffffff`.
Among these, the range `0x11000000` – `0x1100ffff` is designated for *keypad keysyms*.

@sa Appendix A “[Vendor keysyms]” of the X Window System Protocol.

[Vendor keysyms]: https://xorg.freedesktop.org/archive/current/doc/xproto/x11protocol.html#Vendor_KEYSYMs

[encoding]: https://www.x.org/releases/current/doc/xproto/x11protocol.html#keysym_encoding
[new-keysyms]: https://xorg.freedesktop.org/releases/X11R7.7/doc/kbproto/xkbproto.html#new_keysyms
[Unicode]: https://en.wikipedia.org/wiki/Universal_Coded_Character_Set
[Latin-1]: https://en.wikipedia.org/wiki/ISO/IEC_8859-1
[Latin-2]: https://en.wikipedia.org/wiki/ISO/IEC_8859-2
[Latin-3]: https://en.wikipedia.org/wiki/ISO/IEC_8859-3
[Latin-4]: https://en.wikipedia.org/wiki/ISO/IEC_8859-4
[Latin-5]: https://en.wikipedia.org/wiki/ISO/IEC_8859-5
[Latin-6]: https://en.wikipedia.org/wiki/ISO/IEC_8859-6
[Latin-7]: https://en.wikipedia.org/wiki/ISO/IEC_8859-7
[Latin-8]: https://en.wikipedia.org/wiki/ISO/IEC_8859-8
[Latin-9]: https://en.wikipedia.org/wiki/ISO/IEC_8859-9

@ingroup keysyms
