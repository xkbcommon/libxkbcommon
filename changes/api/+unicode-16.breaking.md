Updated keysyms case mappings to cover full **[Unicode 16.0]**. This change
provides a *consistent behavior* with respect to case mappings, and affects
the following:

- `xkb_keysym_to_lower()` and `xkb_keysym_to_upper()` give different output
  for keysyms not covered previously and handle *title*-cased keysyms.

  Example of title-cased keysym: `U01F2` “ǲ”:
  - `xkb_keysym_to_lower(U01F2) == U01F3` “ǲ” → “ǳ”
  - `xkb_keysym_to_upper(U01F2) == U01F1` “ǲ” → “Ǳ”
- *Implicit* alphabetic key types are better detected, because they use the
  latest Unicode case mappings and now handle the *title*-cased keysyms the
  same way as upper-case ones.

Note: There is a single *exception* that do not follow the Unicode mappings:
- `xkb_keysym_to_upper(ssharp) == U1E9E` “ß” → “ẞ”

Note: As before, only *simple* case mappings (i.e. one-to-one) are supported.
For example, the full upper case of `U+01F0` “ǰ” is “J̌” (2 characters: `U+004A`
and `U+030C`), which would require 2 keysyms, which is not supported by the
current API.

[Unicode 16.0]: https://www.unicode.org/versions/Unicode16.0.0/
