The rules file {#rule-file-format}
==============

The purpose of the rules file is to map between configuration values
that are easy for a user to specify and understand, and the
configuration values that the keymap compiler, `xkbcomp`, uses and
understands. The following diagram presents an overview of this
process. See the [XKB introduction] for further details on the
components.

@dotfile xkb-configuration "XKB keymap configurations"

@tableofcontents{html:2}

`libxkbcommon`’s keymap compiler `xkbcomp` uses the `xkb_component_names`
struct internally, which maps directly to [include statements] of the
appropriate [sections] \(called [KcCGST] for short):

- [key codes],
- [compatibility],
- geometry ([not supported](@ref geometry-support) by xkbcommon),
- [symbols],
- [types].

These are not really intuitive nor straightforward for the uninitiated.
Instead, the user passes in a `xkb_rule_names` struct, which consists
of the following fields (called [RMLVO] for short):

- the name of a [rules] file (in Linux this is usually “evdev”),
- a keyboard [model] \(e.g. “pc105”),
- a set of [layouts][layout] (which will end up in different
  groups, e.g. “us,fr”),
- a set of [variants][variant] (used to alter/augment the respective
  layout, e.g. “intl,dvorak”),
- a set of [options] \(used to tweak some general
  behavior of the keyboard, e.g. “ctrl:nocaps,compose:menu” to make
  the Caps Lock key act like Ctrl and the Menu key like Compose).

[KcCGST]: @ref KcCGST-intro
[RMLVO]: @ref RMLVO-intro
[MLVO]: @ref RMLVO-intro
[XKB introduction]: @ref xkb-intro
[include statements]: @ref xkb-include
[sections]: @ref keymap-section-def
[key codes]: @ref the-xkb_keycodes-section
[compatibility]: @ref the-xkb_compat-section
[symbols]: @ref the-xkb_symbols-section
[types]: @ref the-xkb_types-section
[rules]: @ref config-rules-def
[model]: @ref config-model-def
[layout]: @ref config-layout-def
[variant]: @ref config-variant-def
[option]: @ref config-options-def
[options]: @ref config-options-def

# Format of the file

## Rules and rule sets {#rule-def}

@anchor rule-set-def
The file consists of **rule sets**, each consisting of **rules** (one
per line), which match the [MLVO] values on the left hand side, and,
if the values match to the values the user passed in, results in the
values on the right hand side being [added][value update] to the
resulting [KcCGST]. See @ref rmlvo-resolution for further details.

[rule set]: @ref rule-set-def
[rule sets]: @ref rule-set-def
[rule]: @ref rule-def

```c
// This is a comment

// The following line is a rule header.
// It starts with ‘!’ and introduces a rules set.
// It indicates that the rules map MLVO options to KcCGST symbols.
! option       = symbols
  // The following lines are rules that add symbols of the RHS when the
  // LHS matches an option.
  ctrl:nocaps  = +ctrl(nocaps)
  compose:menu = +compose(menu)

// One may use multiple MLVO components on the LHS
! layout    option          = symbols
  be        caps:digits_row = +capslock(digits_row)
  fr        caps:digits_row = +capslock(digits_row)
```

## Groups {#rules-group-def}

Since some values are related and repeated often, it is possible
to *group* them together and refer to them by a **group name** in the
rules.

[group]: @ref rules-group-def

```c
// Let’s rewrite the previous rules set using groups.
// Groups starts with ‘$’.

// Define a group for countries with AZERTY layouts
! $azerty = be fr

// The following rule will match option `caps:digits_row` only for
// layouts in the $azerty group, i.e. `fr` and `be`.
! layout    option          = symbols
 $azerty    caps:digits_row = +capslock(digits_row)
```

## Wild cards {#rules-wildcard-def}

Along with matching values by simple string equality and for
membership in a [group] defined previously, rules may also contain
**wild card** values with the following behavior:

<dl>
<dt>* @anchor rules-wildcard-legacy-def</dt>
<dd>

Legacy wild card:
- For `model` and `options`: *always* match.
- For `layout` and `variant`: match any *non-empty* value.

This wild card usually appears near the end of a rule set to set *default* values.

@note Prefer using the wild cards @ref rules-wildcard-some-def "\<some\>" or
@ref rules-wildcard-any-def "\<any\>" for their simpler semantics, as it does not
depend on the context.
</dd>
<dt>\<none\> @anchor rules-wildcard-none-def</dt>
<dd>

Match *empty* values

@since 1.9.0
</dd>
<dt>\<some\> @anchor rules-wildcard-some-def</dt>
<dd>

Match *non-empty* value

@since 1.9.0
</dd>
<dt>\<any\> @anchor rules-wildcard-any-def</dt>
<dd>

Match *any* (optionally empty) value. Its behavior does not depend on the
context, contrary to the legacy wild card @ref rules-wildcard-legacy-def "*".

This wild card usually appears near the end of a rule set to set *default* values.

@since 1.9.0
</dd>
</dl>

```c
! layout = keycodes
  // The following two lines only match exactly their respective groups.
 $azerty = +aliases(azerty)
 $qwertz = +aliases(qwertz)
  // This line will match layouts that are neither in $azerty nor in
  // $qwertz groups.
  *      = +aliases(qwerty)
```

# Grammar

It is advised to look at a file like `rules/evdev` along with
this grammar.

@note Comments, whitespace, etc. are not shown.

```bnf
File         ::= { "!" (Include | Group | RuleSet) }

Include      ::= "include" <ident>

Group        ::= GroupName "=" { GroupElement } "\n"
GroupName    ::= "$"<ident>
GroupElement ::= <ident>

RuleSet      ::= Mapping { Rule }

Mapping      ::= { Mlvo } "=" { Kccgst } "\n"
Mlvo         ::= "model" | "option" | ("layout" | "variant") [ Index ]
Index        ::= "[" ({ NumericIndex } | { SpecialIndex }) "]"
NumericIndex ::= 1..XKB_MAX_GROUPS
SpecialIndex ::= "single" | "first" | "later" | "any"
Kccgst       ::= "keycodes" | "symbols" | "types" | "compat" | "geometry"

Rule         ::= { MlvoValue } "=" { KccgstValue } "\n"
MlvoValue    ::= "*" | "<none>" | "<some>" | "<any>" | GroupName | <ident>
KccgstValue  ::= <ident> [ { Qualifier } ]
Qualifier    ::= ":" ({ NumericIndex } | "all")
```

<!--
[WARNING]: Doxygen parsing is a mess. \% does not work as expected
in Markdown code quotes, e.g. `\%H` gives `\H`. But using <code> tags
or %%H seems to do the job though.
-->
@note
- Include processes the rules in the file path specified in the `ident`,
  in order. **%-expansion** is performed, as follows: @rules-include-expansion
  <dl>
    <dt>`%%`</dt>
    <dd>A literal %.</dd>
    <dt><code>\%H</code></dt>
    <dd>The value of the `$HOME` environment variable.</dd>
    <dt><code>\%E</code></dt>
    <dd>
        The extra lookup path for system-wide XKB data (usually
        `/etc/xkb/rules`).
    </dd>
    <dt><code>\%S</code></dt>
    <dd>
        The system-installed rules directory (usually
        `/usr/share/X11/xkb/rules`).
    </dd>
  </dl>
  **Note:** This feature is supported by libxkbcommon but not by the legacy X11
  tools.

- @anchor rules-extended-layout-indexes
  (Since version `1.8.0`)
  The following *extended layout indexes* can be used to avoid repetition and
  clarify the semantics:

  <dl>
    <dt>`single`</dt>
    <dd>
        Matches a single layout; `layout[single]` is the same as without
        explicit index: `layout`.
    </dd>
    <dt>`first`</dt>
    <dd>
        Matches the first layout/variant, no matter how many layouts are in
        the RMLVO configuration. Acts as both `layout` and `layout[1]`.
    </dd>
    <dt>`later`</dt>
    <dd>
        Matches all but the first layout. This is an index *range*.
        Acts as `layout[2]` .. `layout[4]`.
    </dd>
    <dt>any</dt>
    <dd>
        Matches layout at any position. This is an index *range*.
        Acts as `layout`, `layout[1]` .. `layout[4]`.
    </dd>
  </dl>

  When using a layout index *range* (`later`, `any`), the @ref rules-i-expansion "%i expansion"
  can be used in the `KccgstValue` to refer to the index of the matched layout.

- The order of values in a `Rule` must be the same as the `Mapping` it
  follows. The mapping line determines the meaning of the values in
  the rules which follow in the `RuleSet`.

- If a `Rule` is matched, **%-expansion** is performed on the
  `KccgstValue`, as follows:

  <dl>
    <dt><code>\%m</code>, <code>\%l</code>, <code>\%v</code></dt>
    <dd>
        The [model], [layout] or [variant], if *only one* was given
        (e.g. <code>\%l</code> for “us,il” is invalid).
    </dd>
    <dt>
        <code>\%l[1]</code>, <code>\%l[2]</code>, …,
        <code>\%v[1]</code>, <code>\%v[2]</code>, …
    </dt>
    <dd>
        [Layout][layout] or [variant] for the specified layout `Index`,
        if *more than one* was given, e.g.: <code>\%l[1]</code> is
        invalid for “us” but expands to “us” for “us,de”.
    </dd>
    <dt>
        `%+m`,
        `%+l`, `%+l[1]`, `%+l[2]`, …,
        `%+v`, `%+v[1]`, `%+v[2]`, …
    </dt>
    <dd>
        As above, but prefixed with ‘+’. Similarly, ‘|’, ‘^’, ‘-’, ‘_’ may be
        used instead of ‘+’. See the [merge mode] documentation for the
        special meaning of ‘+’, ‘|’ and ‘^’.
    </dd>
    <dt>
        `%(m)`,
        `%(l)`, `%(l[1])`, `%(l[2])`, …,
        `%(v)`, `%(v[1])`, `%(v[2])`, …
    </dt>
    <dd>
        As above, but prefixed by ‘(’ and suffixed by ‘)’.
    </dd>
    <dt>
        @anchor rules-i-expansion
        `:%%i`,
        `%%l[%%i]`,
        `%(l[%%i])`,
        etc.
    </dt>
    <dd>
        (Since version `1.8.0`)
        In case the mapping uses an @ref rules-extended-layout-indexes "extended layout index",
        `%%i` corresponds to the index of the matched layout.
    </dd>
  </dl>

  In case the expansion is *invalid*, as described above, it is *skipped*
  (the rest of the string is still processed); this includes the prefix
  and suffix. This is why one should use e.g. <code>%(v[1])</code>
  instead of <code>(\%v[1])</code>. See @ref rules-symbols-example for
  an illustration.

- @anchor rules-all-qualifier
  (Since version `1.8.0`) If a `Rule` is matched, the `:all` *qualifier* in the
  `KccgstValue` applies the qualified value (and its optional merge mode) to all
  layouts. If there is no merge mode, it defaults to *override* `+`.

  <table>
    <caption>Examples of `:all` qualified use</caption>
    <tr>
        <th>`KccgstValue`</th>
        <th>Layouts count</th>
        <th>Final `KccgstValue`</th>
    </tr>
    <tr>
        <td rowspan="2">`x:all`</td>
        <td>1</td>
        <td>`x:1`</td>
    </tr>
    <tr>
        <td>2</td>
        <td>`x:1+x:2`</td>
    </tr>
    <tr>
        <td rowspan="2">`+x:all`</td>
        <td>1</td>
        <td>`+x:1`</td>
    </tr>
    <tr>
        <td>3</td>
        <td>`+x:1+x:2+x:3`</td>
    </tr>
    <tr>
        <td rowspan="2">`|x:all`</td>
        <td>1</td>
        <td>`|x:1`</td>
    </tr>
    <tr>
        <td>4</td>
        <td>`|x:1|x:2|x:3|x:4`</td>
    </tr>
    <tr>
        <td rowspan="2">`x|y:all`</td>
        <td>1</td>
        <td>`x|y:1`</td>
    </tr>
    <tr>
        <td>3</td>
        <td>`x|y:1|y:2|y:3`</td>
    </tr>
    <tr>
        <td>`x:all+y|z:all`</td>
        <td>2</td>
        <td>`x:1+x:2+y|z:1|z:2`</td>
    </tr>
  </table>

# RMLVO resolution process {#rmlvo-resolution}

## Process

First of all, the rules *file* is extracted from the provided
[<em>R</em>MLVO][RMLVO] configuration (usually `evdev`). Then its path
is resolved and the file is parsed to get the [rule sets].

Then *each rule set* is checked against the provided [MLVO] configuration,
following their *order* in the rules file.

@important @anchor irrelevant-options-order Contrary to layouts and variants,
the *options order* in a [MLVO] configuration (e.g. via `xkbcli`) is irrelevant
for its resolution: only the order of the rules matters. See
“@ref rules-options-example ""” for an illustration.

If a [rule] matches in a @ref rule-set-def "rule set", then:

<!--
Using HTML list tags due to Doxygen Markdown limitation with tables
inside lists.
-->
<ol>
  <li>
    @anchor rules-kccgst-value-update
    The *KcCGST* value of the rule is used to update the [KcCGST]
    configuration, using the following instructions. Note that `foo`
    and `bar` are placeholders; ‘+’ specifies the *override* [merge mode]
    and can be replaced by ‘|’ or ‘^’ to specify respectively the *augment*
    or *replace* merge mode instead.

    | Rule value        | Old KcCGST value | New KcCGST value      |
    | ----------------- | ---------------- | --------------------- |
    | `bar`             |                  | `bar`                 |
    | `bar`             | `foo`            | `foo` (*skip* `bar`)  |
    | `bar`             | `+foo`           | `bar+foo` (*prepend*) |
    | `+bar`            |                  | `+bar`                |
    | `+bar`            | `foo`            | `foo+bar`             |
    | `+bar`            | `+foo`           | `+foo+bar`            |
  </li>
  <li>
    The rest of the set will be *skipped*, except if the set matches
    against [options]. Indeed, those may contain *multiple* legitimate
    rules, so they are processed entirely. See @ref rules-options-example
    for an illustration.
  </li>
</ol>

[value update]: @ref rules-kccgst-value-update
[merge mode]: @ref merge-mode-def

## Examples

### Example: key codes

Using the following example:

```c
! $jollamodels = jollasbj
! $azerty = be fr
! $qwertz = al ch cz de hr hu ro si sk

! model       = keycodes
 $jollamodels = evdev+jolla(jolla)
  olpc        = evdev+olpc(olpc)
  *           = evdev

! layout      = keycodes
 $azerty      = +aliases(azerty)
 $qwertz      = +aliases(qwertz)
  *           = +aliases(qwerty)
```

we would have the following resolutions of <em>[key codes]</em>:

| Model      | Layout   | Keycodes                             |
| ---------- | :------: | :----------------------------------- |
| `jollasbj` | `us`     | `evdev+jolla(jolla)+aliases(qwerty)` |
| `olpc`     | `be`     | `evdev+olpc(olpc)+aliases(azerty)`   |
| `pc`       | `al`     | `evdev+aliases(qwertz)`              |

### Example: layouts, variants and symbols {#rules-symbols-example}

Using the following example:

```c
! layout    = symbols
  *         = pc+%l%(v)
// The following would not work: syntax for *multiple* layouts
// in a rule set for *single* layout.
//*         = pc+%l[1]%(v[1])

! layout[1] = symbols
  *         = pc+%l[1]%(v[1])
// The following would not work: syntax for *single* layout
// in a rule set for *multiple* layouts.
//*         = pc+%l%(v)

! layout[2] = symbols
  *         = +%l[2]%(v[2]):2

! layout[3] = symbols
  *         = +%l[3]%(v[3]):3
```

we would have the following resolutions of <em>[symbols]</em>:

| Layout     | Variant      | Symbols                       | Rules sets used |
| ---------- | ------------ | ----------------------------- | --------------- |
| `us`       |              | `pc+us`                       | #1              |
| `us`       | `intl`       | `pc+us(intl)`                 | #1              |
| `us,es`    |              | `pc+us+es:2`                  | #2, #3          |
| `us,es,fr` | `intl,,bepo` | `pc+us(intl)+es:2+fr(bepo):3` | #2, #3, #4      |

Since version `1.8.0`, the previous code can be replaced with simply:

```c
! layout[first] = symbols
  *             = pc+%l[%i]%(v[%i])

! layout[later] = symbols
  *             = +%l[%i]%(v[%i]):%i
```

### Example: layout, option and symbols {#rules-options-example}

Using the following example:

```c
! $azerty = be fr

! layout = symbols
  *      = pc+%l%(v)

! layout[1] = symbols
  *         = pc+%l[1]%(v[1])

! layout[2] = symbols
  *         = +%l[2]%(v[2])
// Repeat the previous rules set with indexes 3 and 4

! layout     option          = symbols
 $azerty     caps:digits_row = +capslock(digits_row)
  *          misc:typo       = +typo(base)
  *          lv3:ralt_alt    = +level3(ralt_alt)

! layout[1]  option          = symbols
 $azerty     caps:digits_row = +capslock(digits_row):1
  *          misc:typo       = +typo(base):1
  *          lv3:ralt_alt    = +level3(ralt_alt):1
// Repeat the previous rules set for indexes 2 to 4
```

we would have the following resolutions of <em>[symbols]</em>:

| Layout  | Option                                   | Symbols                                                     |
| ------- | ---------------------------------------- | ----------------------------------------------------------- |
| `be`    | `caps:digits_row`                        | `pc+be+capslock(digits_row)`                                |
| `gb`    | `caps:digits_row`                        | `pc+gb`                                                     |
| `fr`    | `misc:typo`                              | `pc+fr+typo(base)`                                          |
| `fr`    | `misc:typo,caps:digits_row`              | `pc+fr+capslock(digits_row)+typo(base)`                     |
| `fr`    | `lv3:ralt_alt,caps:digits_row,misc:typo` | `pc+fr+capslock(digits_row)+typo(base)+level3(ralt_alt)`    |
| `fr,gb` | `caps:digits_row,misc:typo`              | `pc+fr+gb:2+capslock(digits_row)+typo(base):1+typo(base):2` |

Note that the configuration with `gb` [layout] has no match for the
[option] `caps:digits_row` and that the order of the [options] in the
[RMLVO] configuration has no influence on the resulting [symbols], as it
depends solely on their order in the rules.

Since version `1.8.0`, the previous code can be replaced with simply:

```c
! $azerty = be fr

! layout[first] = symbols
  *             = pc+%l[%i]%(v[%i])

! layout[later] = symbols
  *             = +%l[%i]%(v[%i])

! layout[any]  option          = symbols
 $azerty       caps:digits_row = +capslock(digits_row):%i

! option       = symbols
  misc:typo    = +typo(base):all
  lv3:ralt_alt = +level3(ralt_alt):all

// The previous is equivalent to:
! layout[any]  option       = symbols
  *            misc:typo    = +typo(base):%i
  *            lv3:ralt_alt = +level3(ralt_alt):%i
```
