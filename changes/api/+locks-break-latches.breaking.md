Changed modifier and group latches so that setting or releasing a lock now breaks them.
This enables implementing a key that, for example, latches `Shift`
when pressed once, but locks `Caps` when pressed twice in succession.
