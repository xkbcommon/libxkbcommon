Enable to write keysyms as UTF-8-encoded strings:
- *Single* Unicode code point `U+1F3BA` (TRUMPET) `"🎺"` is converted into a
  single keysym: `U1F3BA`.
- *Multiple* Unicode code points are converted to a keysym *list* where it is
  allowed (i.e. in key symbols). E.g. `"J́"` is converted to U+004A LATIN CAPITAL
  LETTER J plus U+0301 COMBINING ACUTE ACCENT: `{J, U0301}`.
- An empty string `""` denotes the keysym `NoSymbol`.
