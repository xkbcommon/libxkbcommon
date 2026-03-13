- Added keysyms from latest [xorgproto]
      \(commit: `04482cdee458445eab7c6a0b6d4ea64b74387401`):

  - `XKB_KEY_dead_apostrophe` ([xorproto-110])
  - `XKB_KEY_SSHARP` ([xorproto-110])
  - `XKB_KEY_leftsingleanglequotemark` ([xorproto-110])
  - `XKB_KEY_rightsingleanglequotemark` ([xorproto-110])
  - `XF86ElectronicPrivacyScreenOn` ([xorproto-109])
  - `XF86ElectronicPrivacyScreenOff` ([xorproto-109])

- Changed:
  - `ISO_Group_Shift` is now the canonical name of the corresponding keysym.
    Previously it was `Mode_switch`, which refers to a core X group
    mechanism obsoleted by XKB.

  [xorproto-109]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/109
  [xorproto-110]: https://gitlab.freedesktop.org/xorg/proto/xorgproto/-/merge_requests/110
