# How to make a libxkbcommon release

### Prerequisites

- Have write access to xkbcommon Git repositories.
- Be subscribed to the [wayland-devel](https://lists.freedesktop.org/postorius/lists/wayland-devel.lists.freedesktop.org/) mailing list.
  <!-- Old link: https://lists.freedesktop.org/mailman/listinfo/wayland-devel -->

### Steps

#### Prepare the release

- [ ] Ensure there is no issue in the tracker blocking the release. Make sure
  they have their milestone set to the relevant release and the relevant tag
  “critical”.

- [ ] Ensure all items in the current milestone are processed. Remaining items
  must be *explicitly* postponed by changing their milestone.

- [ ] Ensure the keysyms header is up-to-date (see [xorgproto])

- [ ] Create a release branch: `git checkout -b release/vMAJOR.MINOR.PATCH master`

- [ ] Update the `NEWS.md` file for the release, following [the corresponding instructions](changes/README.md).

- [ ] Bump the `version` in `meson.build`.

- [ ] Run `meson dist -C build` to make sure the release is good to go.

- [ ] Commit `git commit -m 'Bump version to MAJOR.MINOR.PATCH'`.

- [ ] Create a pull request using this template and ensure *all* CI is green.

- [ ] Merge the pull request.

- [ ] Tag `git switch master && git pull && git tag --annotate -m xkbcommon-<MAJOR.MINOR.PATCH> xkbcommon-<MAJOR.MINOR.PATCH>`.

- [ ] Push the tag `git push origin xkbcommon-<MAJOR.MINOR.PATCH>`.

[xorgproto]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/tree/master/include/X11

#### Send announcement email to wayland-devel

- [ ] Send an email to the wayland-devel@lists.freedesktop.org mailing list, using this template:

```
Subject: [ANNOUNCE] libxkbcommon MAJOR.MINOR.PATCH

<NEWS & comments for this release>

Git tag:
--------

git tag: xkbcommon-<MAJOR.MINOR.PATCH>
git commit: <git commit sha>

<YOUR NAME>
```

#### Update website

- [ ] Pull the latest [website repository](https://github.com/xkbcommon/website).

- [ ] Add the doc for the release: `cp -r <xkbommon>/build/html doc/<MAJOR.MINOR.PATCH>`.
  Check carefully that there is no warning during generation with Doxygen.
  It may be necessary to use another version of Doxygen to get a clean build.
  Building from source using the main branch is also a good option.

- [ ] Apply manual Doxygen fixes:
  - [ ] Fix labels of the TOC in the “Release notes” page.

- [ ] Update the `current` symlink: `ln -nsrf doc/<MAJOR.MINOR.PATCH> doc/current`.

- [ ] Grab a link to the announcement mail from the [wayland-devel archives](https://lore.freedesktop.org/wayland-devel/).
  <!-- Old archives: https://lists.freedesktop.org/archives/wayland-devel/ -->

- [ ] Update the `index.html`:
    - "Our latest API- and ABI-stable release ..."
    - Add entry to the `releases` HTML list.

- [ ] Commit `git commit -m MAJOR.MINOR.PATCH`.

- [ ] Push `git push`. This automatically publishes the website after a few seconds.
