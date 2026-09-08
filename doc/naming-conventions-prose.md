# Prose naming conventions {#prose-naming-conventions}

How to refer to the project, its libraries, and its tools in *prose*.

@note *Technical names* (`pkg-config` modules, header paths, sonames) are
*fixed* and not subject to the following rules.

## Rule

Default to **xkbcommon**. Use a **lib…** artifact name only when one of these
*triggers* applies:

1. **Disambiguation:** a sibling library is actually named in the same context.
2. **Artifact specificity:** the statement is about a compiled artifact,
   `-dev` package, or link flag — something you could point to a file for.

   The `lib` prefix marks the artifact's canonical/package identity matching
   its `.pc` name, not a literal on-disk filename: MSVC builds, for example,
   produce `.lib` files without the `lib` prefix.

Sub-libraries **xkbcommon-x11**, **xkbregistry**  use their bare name in *all*
prose; add `lib` only for trigger 2.

## Quick reference

<table>
<caption>Quick project naming reference</caption>
<thead>
<tr>
<th>Term</th>
<th>When</th>
<th>Examples</th>
</tr>
</thead>
<tbody>
<!-- xkbcommon -->
<tr>
<th>xkbcommon</th>
<td>
Default: project reference, capabilities statements, API documentation,
FAQ answers, shared version numbers and `@since` tags, external comparisons;
symbol mentions with no sibling in context.
</td>
<td>
> xkbcommon supports RMLVO lookups since 1.14

> xkbcommon supports 32-bit keycodes, contrary to the X11 protocol with keycodes
> only up to 255

> `xkb_context_new()` is part of xkbcommon
<!-- blank required by Doxygen -->

</td>
</tr>
<!-- libxkbcommon -->
<tr>
<th>libxkbcommon</th>
<td>
A sibling library is named in the same context (trigger 1); the compiled
`.so` / `-dev` package (trigger 2); library-specific [release notes] entries.

[release notes]: @ref release-notes
</td>
<td>
> `xkb_context_new()` is part of libxkbcommon, `rxkb_context_new()` of
> libxkbregistry

> `libxkbcommon.so`

> `libxkbcommon-dev` package
<!-- blank required by Doxygen -->

</td>
</tr>
<!-- xkbcommon-x11, xkbregistry -->
<tr>
<th>xkbcommon-x11, xkbregistry</th>
<td>
Any prose reference: these names do not collide with anything else, so no
disambiguation is ever needed.
</td>
<td>
> xkbcommon-x11 queries the X server directly
<!-- blank required by Doxygen -->

</td>
</tr>
<!-- libxkb* -->
<tr>
<th>libxkbcommon-x11, libxkbregistry</th>
<td>Reference to a specific compiled artifact/package (trigger 2).</td>
<td>
> ships as `libxkbcommon-x11.so`
<!-- blank required by Doxygen -->

</td>
</tr>
<!-- xkbcli -->
<tr>
<th>xkbcli</th>
<td>Always (unambiguous)</td>
<td>
> Use xkbcli to inspect a keymap
<!-- blank required by Doxygen -->

</td>
</tr>
<!-- XKB -->
<tr>
<th>XKB</th>
<td>The protocol/specification/format only: *never* a library</td>
<td>
> the XKB keymap format
<!-- blank required by Doxygen -->

</td>
</tr>
<!-- xkbcomp -->
<tr>
<th>xkbcomp</th>
<td>
The external X11 tool: not part of this project, never a trigger-1 sibling.
</td>
<td>
> xkbcomp vs. xkbcommon
<!-- blank required by Doxygen -->

</td>
</tr>
</tbody>
</table>

## Specific rules

<dl>
<dt>Versions &amp; release notes.</dt>
<dd>
All libraries share one release. Name releases “xkbcommon X.Y”; keep
`@since` tags bare. In `NEWS.md`, name a specific library only when a
change is limited to it; build/CI/docs entries stay unnamed or say
“xkbcommon”.
</dd>

<dt>README &amp; top-level docs.</dt>
<dd>
Title and refer to the project as “xkbcommon”. The repository name
`libxkbcommon` doesn’t determine the prose name.
</dd>

<dt>Future language bindings (`libxkbcommon-<lang>`).</dt>
<dd>
Use the `lib` prefix for the repository name only. Let each
language ecosystem pick its own published package name. In prose: “xkbcommon
bindings for &lt;lang&gt;”.
</dd>
</dl>

<!--
TODO: Add a short paragraph to the README/wiki noting that the GitHub org is
xkbcommon, and that the libxkbcommon repo hosts the core library plus
xkbcommon-x11, xkbregistry, and xkbcli — so readers don’t infer the repo’s
scope from its name.
-->
