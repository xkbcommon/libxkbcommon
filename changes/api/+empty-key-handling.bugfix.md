Fixed the handling of empty keys. Previously keys with no symbols nor actions
would simply be skipped entirely. E.g. in the following:

```c
key <A> { vmods = M };
modifier_map Shift { <A> };
```

the key `<A>` would be skipped and the virtual modifier `M` would not be
mapped to `Shift`. This is now handled properly.
