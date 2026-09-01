# Quick Guide

@tableofcontents{html:2}

## Introduction

This document contains a quick walk-through of the often-used parts of
the library. We will employ a few use-cases to lead the examples:

1. An **evdev** client. *evdev* is the Linux kernel’s input subsystem; it
   only reports to the client which keys are pressed and released.

2. An **X11** client, using the XCB library to communicate with the X
   server and the xcb-xkb library for using the XKB protocol.

3. A **Wayland** *client*, using the standard protocol.

4. A **Wayland** *server*, using the standard protocol.

The snippets are not complete, and some support code is omitted. You
can find complete and more complex examples in the [source directory]:

1. `tools/interactive-evdev.c` contains an interactive evdev client.

2. `tools/interactive-x11.c` contains an interactive X11 client.

3. `tools/interactive-wayland.c` contains an interactive Wayland client.

Also, the library contains many more functions for examining and using
the library context, the keymap and the keyboard state. See the
hyper-linked reference documentation or go through the header files in
xkbcommon/ for more details.

[source directory]: https://github.com/xkbcommon/libxkbcommon

## Code for clients {#quick-guide-clients}

Before we can do anything interesting, we need a library context:

@snippet{trimleft} "test/quick-guide.c" quick-guide-context-example

The `xkb_context` contains the keymap include paths, the log level and
functions, and other general customizable administrativia.

Next we need to create a keymap, `xkb_keymap`. This is an immutable object
which contains all of the information about the keys, layouts, etc. There
are different ways to do this.

If we are an **evdev** client, we have nothing to go by, so we need to ask
the user for his/her keymap preferences (for example, an Icelandic
keyboard with a Dvorak layout). The configuration format is commonly
called [RMLVO] \(Rules+Model+Layout+Variant+Options), the same format used
by the X server. With it, we can fill a struct called `xkb_rule_names`;
passing `NULL` chooses the system’s default.

[RMLVO]: @ref RMLVO-intro

@snippet{trimleft} "test/quick-guide.c" quick-guide-evdev-keymap-example

If we are a **Wayland** client, the compositor gives us a string complete
with a keymap. In this case, we can create the keymap object like this:

@snippet{trimleft} "test/quick-guide.c" quick-guide-wayland-client-compilation-example

If we are an **X11** client, we are better off getting the keymap from the
X server directly. For this we need to choose the XInput device; here
we will use the core keyboard device:

@snippet{trimleft} "test/x11.c" quick-guide-x11-client-init-example

Now that we have the keymap, we are ready to handle the keyboard devices.
For each device, we create an `xkb_state`, which remembers things like which
keyboard modifiers and LEDs are active:

<dl>
<dt>Wayland</dt>
<dd>
@snippet{trimleft} "test/quick-guide.c" quick-guide-wayland-client-state-example
</dd>
<dt>X11/XCB</dt>
<dd>
@snippet{trimleft} "test/x11.c" quick-guide-x11-client-state-example
</dd>
<dt>evdev</dt>
<dd>
@snippet{trimleft} "test/quick-guide.c" quick-guide-evdev-client-state-example
</dd>
</dl>

When we have an `xkb_state` for a device, we can start handling key events
from it.  Given a keycode for a key, we can get its keysym:

@snippet{trimleft} "test/quick-guide.c" quick-guide-client-keysym-example

We can see which keysym we got, and get its name:

@snippet{trimleft} "test/quick-guide.c" quick-guide-client-keysym-name-example

libxkbcommon also supports an extension to the classic XKB, whereby a
single event can result in multiple keysyms. Here’s how to use it:

@snippet{trimleft} "test/quick-guide.c" quick-guide-client-keysyms-example

We can also get a UTF-8 string representation for this key:

@snippet{trimleft} "test/quick-guide.c" quick-guide-client-keysym-utf8-example

Of course, we also need to keep the `xkb_state` up-to-date with the
keyboard device, if we want to get the correct keysyms in the future.

If we are an **evdev** client, we must let the library know whether a key
is pressed or released at any given time:

@snippet{trimleft} "test/quick-guide.c" quick-guide-evdev-client-key-update-example

The `changed` return value tells us exactly which parts of the state
have changed.

If it is a key-repeat event, we can ask the keymap what to do with it:

@snippet{trimleft} "test/quick-guide.c" quick-guide-evdev-client-key-repeated-example

On the other hand, if we are an **X** or **Wayland** client, the server already
does the hard work for us. It notifies us when the device’s state
changes, and we can simply use what it tells us (the necessary
information usually comes in a form of some “state changed” event):

@snippet{trimleft} "test/quick-guide.c" quick-guide-client-update-mask-example

Now that we have an always-up-to-date `xkb_state`, we can examine it.
For example, we can check whether the Control modifier is active, or
whether the Num Lock LED is active:

@snippet{trimleft} "test/quick-guide.c" quick-guide-client-mods-example

And that’s it! Eventually, we should free the objects we’ve created:

@snippet{trimleft} "test/quick-guide.c" quick-guide-client-unref-example

## Code for a Wayland server {#quick-guide-wayland-server}

The code is very similar to the evdev client presented hereinabove. The main
difference is the use of the `xkb_machine` API instead of the `xkb_state`
API.

@snippet{trimleft} "test/quick-guide.c" wayland-server-example

Synthetic updates are handled using `xkb_machine::xkb_machine_process_synthetic()`.
