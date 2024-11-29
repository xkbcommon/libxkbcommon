Updated keysyms case mappings to cover full **[Unicode 16.0]**. This change
provides a *consistent behavior* with respect to case mappings, and affects
the following:

- `xkb_keysym_to_lower()` and `xkb_keysym_to_upper()` give different output
  for keysyms not covered previously and handle *title*-cased keysyms.

  Example of title-cased keysym: `0x10001f2` (`U+01F2` “ǲ”):
  - `xkb_keysym_to_lower(0x10001f2) == 0x10001f3` (`U+01F3` “ǳ”)
  - `xkb_keysym_to_upper(0x10001f2) == 0x10001f1` (`U+01F1` “Ǳ”)
- *Implicit* alphabetic key types are better detected, because they use the
  latest Unicode case mappings and now handle the *title*-cased keysyms the
  same way as upper-case ones.

Note: As before, only *simple* case mappings (i.e. one-to-one) are supported.
For example, the full upper case of `U+01F0` “ǰ” is “J̌” (2 characters: `U+004A`
and `U+030C`), which would require 2 keysyms, which is not supported by the
current API.

[Unicode 16.0]: https://www.unicode.org/versions/Unicode16.0.0/
