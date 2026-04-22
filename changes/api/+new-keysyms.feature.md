- Added keysyms from latest [xorgproto]
      \(commit: `fcb7e9a1a0b593a44740d83b0babddd331fea830`):

  - `XKB_KEY_dead_apostrophe` ([xorgproto-110])
  - `XKB_KEY_SSHARP` ([xorgproto-110])
  - `XKB_KEY_leftsingleanglequotemark` ([xorgproto-110])
  - `XKB_KEY_rightsingleanglequotemark` ([xorgproto-110])
  - `XKB_KEY_XF86ElectronicPrivacyScreenOn` ([xorgproto-109])
  - `XKB_KEY_XF86ElectronicPrivacyScreenOff` ([xorgproto-109])
  - `XKB_KEY_XF86ActionOnSelection` ([xorgproto-112])
  - `XKB_KEY_XF86ContextualInsert` ([xorgproto-112])
  - `XKB_KEY_XF86ContextualQuery` ([xorgproto-112])

- Changed:
  - `ISO_Group_Shift` is now the canonical name of the corresponding keysym.
    Previously it was `Mode_switch`, which refers to a core X group
    mechanism obsoleted by XKB.

  [xorgproto-109]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/109
  [xorgproto-110]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/110
  [xorgproto-112]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/112
