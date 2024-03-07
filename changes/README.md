# Fragments for the changelog

This directory contains fragments for the future [NEWS](../NEWS.md) file.

## Introduction

We use <code>[towncrier]</code> to produce useful, summarized news files.

There are 3 sections types:

- API: `changes/api`
- Tools: `changes/tools`
- Build System: `changes/build`

There are 3 news fragments types:

- Breaking changes: `.breaking`
- New: `.feature`
- Fixes: `.bugfix`

[towncrier]: https://pypi.org/project/towncrier/

## Adding a fragment

Add a short description of the change in a file `changes/SECTION/ID.FRAGMENT.md`,
where:

- `SECTION` and `FRAGMENT` values are described in the [previous section](#introduction).
- `ID` is the corresponding issue identifier on Github, if relevant. If there is
  no such issue, then `ID` should start with `+` and some identifier that make
  the file unique in the directory.

Examples:
- A _bug fix_ for the _issue #463_ is an _API_ change, so the corresponding file
  should be named `changes/api/463.bugfix.md`.
- A _new feature_ for _tools_ like #448 corresponds to e.g.
  `changes/tools/+add-verbose-opt.feature.md`.

Guidelines for the fragment files:

- Use the [Markdown] markup.
- Use past tense, e.g. “Fixed a segfault”.
- Look at the previous releases [NEWS](../NEWS.md) file for further examples.

[Markdown]: https://daringfireball.net/projects/markdown/

## Build the changelog

Install <code>[towncrier]</code> from Pypi:

```bash
python3 -m pip install towncrier
```

Then build the changelog:

```bash
# Only check the result. Useful after adding a new fragment.
towncrier build --draft --version 1.8.0
# Write the changelog & delete the news fragments
towncrier build --yes --version 1.8.0
```
