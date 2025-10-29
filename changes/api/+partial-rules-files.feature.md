rules: Added lightweight composition of rules files: for a given ruleset `<ruleset>`,
the *optional* files `<ruleset>.pre` and `<ruleset>.post` of each include
path are respectively *prepended* and *appended* to the main rules file.

The resulting rule set is equivalent to:

```
! include <include path 1>/rules/<ruleset>.pre // only if defined
…
! include <include path n>/rules/<ruleset>.pre // only if defined

! include <ruleset> // main rules file

! include <include path 1>/rules/<ruleset>.post // only if defined
…
! include <include path n>/rules/<ruleset>.post // only if defined
```

See @ref rmlvo-resolution "" for further details.
