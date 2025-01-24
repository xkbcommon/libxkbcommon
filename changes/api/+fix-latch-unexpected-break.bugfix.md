Make the key events that break *latches* also the only events that
prevent the latches to trigger. Previously any down event would prevent
it.

The major effect of this change is that depressing and releasing
two latching keys simultaneously will now activate both latches, as
expected.
