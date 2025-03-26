Added the following *wild cards* to the **rules** file syntax, in addition to
the current `*` legacy wild card:
- `<none>`: Match *empty* value.
- `<some>`: Match *non-empty* value.
- `<any>`: Match *any* (optionally empty) value. Its behavior does not depend on
  the context, contrary to the legacy wild card `*`.
