Changed latching behavior so that latching a modifier or group now breaks existing modifier latches,
but only if the type of the key responsible for the latter latch
has the modifier of the pre-existing latch in its modifiers list,
and did not `preserve` the modifier.

For example, if a new latch is triggered by pressing a key of type `ALPHABETIC`,
existing `Shift` and `Lock` latches will now be broken, but other latches
will be preserved as before.
