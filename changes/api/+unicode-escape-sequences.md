Added [Unicode code point] escape sequences `\u{NNNN}`. They are replaced with
the [UTF-8] encoding of their corresponding code point `U+NNNN`, if legal.
Supported Unicode code points are in the range `1‥0x10ffff`. This is intended
mainly for writing keysyms as [UTF-8] encoded strings.

[Unicode code point]: https://en.wikipedia.org/wiki/Unicode#Codespace_and_code_points
[UTF-8]: https://en.wikipedia.org/wiki/UTF-8
