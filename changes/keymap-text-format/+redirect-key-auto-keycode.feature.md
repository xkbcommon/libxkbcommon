Added the *default* special value <code>[auto](@ref redirect-key-auto)</code> to
the `keycode` parameter of the `RedirectKey()` action, which resolves to the
keycode where the action is located:

```c
// Original: implict parameter
key <AB01> { [RedirectKey(…) ] };
// Original: explicit parameter
key <AB01> { [RedirectKey(keycode=auto, …) ] };
// Resolved
key <AB01> { [RedirectKey(keycode=<AB01>, …) ] };
```
