Rules: Added support for special layouts indexes:
- *single*: matches a single layout; `layout[single]` is the same as without
  explicit index: `layout`.
- *first*: matches the first layout/variant, no matter how many layouts are in
  the RMLVO configuration. Acts as both `layout` and `layout[1]`.
- *later*: matches all but the first layout. This is an index range. Acts as
  `layout[2]` .. `layout[4]`.
- *any*: matches layout at any position. This is an index range.

Also added:
- the special index `%i` which correspond to the index of the matched layout.
- the `:all` qualifier: it applies the qualified item to all layouts.

See the [documentation](https://xkbcommon.org/doc/current/rule-file-format.html)
for further information.
