Trailing `NoSymbol` and `NoAction()` are now dropped. This may affect
keys that rely on an *implicit* key type.

Example:
- Input:
  ```c
  key <> { [a, A, NoSymbol] };
  ```
- Compilation with xkbcommon \< 1.9.0:
  ```c
  key <> {
    type= "FOUR_LEVEL_SEMIALPHABETIC",
    [a, A, NoSymbol, NoSymbol]
  };
  ```

- Compilation with xkbcommon ≥ 1.9.0:
  ```c
  key <> {
    type= "ALPHABETIC",
    [a, A]
  };
  ```
