# Recipes for the introspection tool

## Installation

> [!IMPORTANT]
> The tools are not yet public, so it is required to build libxkbcommon
> locally and run them with e.g. `meson devenv -C …`.

The Python dependencies ares:
- `PyYAML`
- `rdflib`
- `tqdm` (optional)

## File paths

### XKB directories

> [!TIP]
> It is advised to always use *explicit* XKB directories, denoted `XKB_ROOT` in the
> following sections. This avoids any conflict with e.g. personal settings.

### Keymap files

File paths are resolved following these rules:
- `-` corresponds to the standard input `stdin`.
- If using `--section`: target section must be present in the file.
- Absolute paths:
  - If using `--resolve` with `--type`: file types must match.
- Relative paths:
  - They are resolved relative to the provided XKB directories, unless
    *not* using `--resolve` nor `--type`. In the former case:
    - `--type`: file types must match.

## Resolve paths

Answers to:
- What file does this include resolves to?
- Where is the file corresponding to a registry entry?

```bash
# TYPE is a KcCGST component
# FILE is a file name
# SECTION is a section name in FILE

# Default section
introspection --include XKB_ROOT --resolve --type TYPE FILE
# Specific section
introspection --include XKB_ROOT --resolve --type TYPE --section SECTION FILE
```

## List sections

The default output format is YAML, which is handy for analyzing a single file.

```bash
# Specific file (absolute path)
introspection --include XKB_ROOT --yaml FILE
# Specific file (relative path)
introspection --include XKB_ROOT --yaml --type TYPE FILE

# Specific file (absolute path) and its transitive dependencies
introspection --include XKB_ROOT --yaml --recursive FILE

# Keymap from RMLVO
xkbcli compile-keymap --kccgst --layout LAYOUT | introspection --include XKB_ROOT --yaml

# XKB tree
introspection-query tree XKB_ROOT -- --yaml --recursive
# XKB tree for a specific type
introspection-query tree XKB_ROOT --type TYPE -- --yaml --recursive
```

## Draw graphs

This section uses the [DOT] output format.

[DOT]: https://en.wikipedia.org/wiki/DOT_(graph_description_language)

> [!NOTE]
> DOT is a basic format. Usual conversions:
> - SVG: `… | dot -Tsvg:cairo > OUTPUT.svg`
> - PDF: `… | dot -Tpdf:cairo > OUTPUT.pdf`

```bash
# Specific file (absolute path)
introspection --include XKB_ROOT --dot FILE
# Specific file (relative path)
introspection --include XKB_ROOT --dot --type TYPE FILE

# Specific file (absolute path) and its transitive dependencies
introspection --include XKB_ROOT --dot --recursive FILE

# Keymap from RMLVO
xkbcli compile-keymap --kccgst --layout LAYOUT | introspection --include XKB_ROOT --dot

# XKB tree
introspection-query tree XKB_ROOT -- --dot --recursive
# XKB tree for a specific type
introspection-query tree XKB_ROOT --type TYPE -- --dot --recursive
```

## Query

This section uses the [RDF Turtle] output format and the [SPARQL] query
language.

[RDF Turtle]: https://www.w3.org/TR/turtle/
[SPARQL]: https://www.w3.org/TR/sparql11-query/

### List dependencies

```bash
# Get data about the XKB directories
introspection-query tree XKB_ROOT -- --rdf --recursive > /tmp/tree.ttl
# Query direct dependencies
introspection-query deps --include XKB_ROOT --type TYPE --file FILE --section SECTION /tmp/tree.ttl
# Query all dependencies
introspection-query deps --include XKB_ROOT --type TYPE --file FILE --section SECTION --transitive /tmp/tree.ttl
```

### List unused files/sections

```bash
# Get data about the XKB directories
introspection-query tree XKB_ROOT -- --rdf --recursive > /tmp/tree.ttl
# Analyze a ruleset
RULES=evdev; introspection-query rules --rules $RULES --rdf --output /tmp/$RULES.ttl XKB_ROOT
# Get the unused items for a specific ruleset
RULES=evdev; cat /tmp/tree.ttl /tmp/$RULES.ttl | introspection-query use --include XKB_ROOT --unused -
# Get the unused items for multiple rulesets
cat /tmp/tree.ttl /tmp/base.ttl /tmp/evdev.ttl | introspection-query use --include XKB_ROOT --unused -
```
