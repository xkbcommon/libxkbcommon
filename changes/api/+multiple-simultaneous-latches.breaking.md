Changed latching behavior so that the same rules used to detect
whether an action breaks a latch after the latching key is released,
are now also used to detect whether the action prevents the latch
from forming before the latching key is released.

The major effect of this change is that depressing and releasing
two latching keys simultaneously will now activate both latches.
