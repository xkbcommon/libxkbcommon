Fixed segfault due to invalid arithmetic to bring *negative* layout indexes
into range. It triggers with the following settings:

- layouts count (per key or total) N: `N > 0`, and
- layout index n: `n = - k * N` (`k > 0`)

Note that these settings are unlikely in usual use cases.
