/*
 * For MIT-open-group:
 * Copyright 1985, 1987, 1990, 1998  The Open Group
 * Copyright 2008  Dan Nicholson
 *
 * For HPND:
 * Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 *
 * For MIT:
 * Copyright © 2009-2012 Daniel Stone
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita
 * Copyright © 2023-2026 Pierre Le Marre
 *
 * SPDX-License-Identifier: MIT-open-group AND HPND AND MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifndef _XKBCOMMON_H_
#define _XKBCOMMON_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <xkbcommon/xkbcommon-errors.h>
#include <xkbcommon/xkbcommon-names.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && !defined(__CYGWIN__)
# define XKB_EXPORT      __attribute__((visibility("default")))
#elif defined(_WIN32)
# define XKB_EXPORT      __declspec(dllexport)
#else
# define XKB_EXPORT
#endif

/**
 * @file
 * Main libxkbcommon API.
 *
 * @brief Core API for keyboard keymap compilation and state processing.
 *
 * This header provides the primary public API for libxkbcommon. It exposes
 * facilities for:
 */

/**
 * @struct xkb_context
 * @ingroup context
 * Opaque top level library context object.
 *
 * The context contains various general library data and state, like
 * logging level and include paths.
 *
 * Objects are created in a specific context, and multiple contexts may
 * coexist simultaneously.  Objects from different contexts are completely
 * separated and do not share any memory or state.
 */
struct xkb_context;

/**
 * @struct xkb_keymap
 * @ingroup keymap
 * Opaque compiled keymap object.
 *
 * The keymap object holds all of the static keyboard information obtained
 * from compiling XKB files.
 *
 * A keymap is immutable after it is created (besides reference counts, etc.);
 * if you need to change it, you must create a new one.
 */
struct xkb_keymap;

/**
 * @struct xkb_machine
 * @ingroup state
 * Opaque XKB state machine object.
 *
 * `xkb_machine` is a [Mealy machine]<!-- -->: it is a finite-state machine that
 * takes a stream of raw key events – a pair ([keycode], [direction]) – as input,
 * and produces a stream of atomic [XKB events](@ref xkb_event) as output. Output
 * depends on *both* the input and the current internal state (active modifiers,
 * current layout, etc.).
 *
 * This is the authoritative object for *server-side* XKB processing.
 *
 * @note To query the resulting keyboard state (active modifiers, current
 * layout, LED states, etc.), pair this object with an `xkb_state` updated via
 * `xkb_state::xkb_state_update_event()`. The `xkb_state` object is the
 * *observable state* of the machine and provides the full query API.
 *
 * See @ref server-client-state for details.
 *
 * See the [example for a Wayland server](@ref quick-guide-wayland-server)
 * in the quick guide.
 *
 * @since 1.14.0
 *
 * [Mealy machine]: https://en.wikipedia.org/wiki/Mealy_machine
 * [keycode]: @ref xkb_keycode_t
 * [direction]: @ref xkb_key_direction
 * [keyboard events]: @ref xkb_event
 */
struct xkb_machine;

/**
 * @struct xkb_state
 * @ingroup state
 * Opaque keyboard state object.
 *
 * State objects contain the active state of a keyboard (or keyboards), such
 * as the currently effective layout and the active modifiers. Depending on
 * the use case, the state can be driven by raw key events or updated from
 * server serializations, and always exposes a query API for keysyms,
 * modifiers, layout and LEDs.
 *
 * This object serves 3 roles:
 * <dl>
 * <dt>*Client* API</dt>
 * <dd>
 * Update the state from server serializations via `xkb_state_update_mask()`,
 * then query it (keysyms, modifiers, layout, LEDs).
 *
 * Use the constructor `xkb_state_new_with_mode()` with
 * `::XKB_STATE_MODE_CLIENT`.
 *
 * See the [examples](@ref quick-guide-clients) in the quick guide.
 * </dd>
 * <dt>Server query companion</dt>
 * <dd>
 * Update via `xkb_state_update_event()` to expose the full query API
 * alongside an [`xkb_machine`](@ref xkb_machine): `xkb_machine` is the
 * [Mealy machine] that processes keyboard input; `xkb_state` is its
 * *observable state*, exposing the query API.
 *
 * Use the constructor `xkb_state_new_with_mode()` with
 * `::XKB_STATE_MODE_SERVER_QUERY`.
 *
 * See [examples](@ref quick-guide-wayland-server) in the quick guide.
 * </dd>
 * <dt>Legacy *server* API</dt>
 * <dd>
 * `xkb_state` is a [Mealy machine]<!-- -->: it is a finite-state machine that
 * takes a stream of raw key events – a pair ([keycode], [direction]) – as input,
 * and produces `xkb_state_component` delta with the previous state. Output
 * depends on *both* the input and the current internal state (active modifiers,
 * current layout, etc.).
 *
 * - Create it using the constructor `xkb_state_new_with_mode()` with
 *   `::XKB_STATE_MODE_SERVER` or the legacy `xkb_state_new()`.
 * - Update it via `xkb_state_update_key()` and `xkb_state_update_synthetic()`.
 * - Query it directly via the API common to the client and companion use cases.
 *
 * @deprecated Since 1.14.0, prefer `xkb_machine` for new server
 * applications.
 * </dd>
 * </dl>
 *
 * See @ref server-client-state and @ref xkb_state_mode for further details.
 *
 * [Mealy machine]: https://en.wikipedia.org/wiki/Mealy_machine
 */
struct xkb_state;

/**
 * A number used to represent a physical key on a keyboard.
 *
 * A standard PC-compatible keyboard might have 102 keys.  An appropriate
 * keymap would assign each of them a keycode, by which the user should
 * refer to the key throughout the library.
 *
 * Historically, the X11 protocol, and consequentially the XKB protocol,
 * assign only 8 bits for keycodes.  This limits the number of different
 * keys that can be used simultaneously in a single keymap to 256
 * (disregarding other limitations).  This library does not share this limit;
 * keycodes beyond 255 (*extended* keycodes) are not treated specially.
 * Keymaps and applications which are compatible with X11 should not use
 * these keycodes.
 *
 * The values of specific keycodes are determined by the keymap and the
 * underlying input system.  For example, with an X11-compatible keymap
 * and Linux evdev scan codes (see `linux/input.h`), a fixed offset is used:
 *
 * The keymap defines a canonical name for each key, plus possible aliases.
 * Historically, the XKB protocol restricts these names to at most 4 (ASCII)
 * characters, but this library does not share this limit.
 *
 * @code
 * xkb_keycode_t keycode_A = KEY_A + 8;
 * @endcode
 *
 * @sa xkb_keycode_is_legal_ext() xkb_keycode_is_legal_x11()
 */
typedef uint32_t xkb_keycode_t;

/**
 * A number used to represent the symbols generated from a key on a keyboard.
 *
 * A key, represented by a keycode, may generate different symbols according
 * to keyboard state.  For example, on a QWERTY keyboard, pressing the key
 * labled \<A\> generates the symbol ‘a’.  If the Shift key is held, it
 * generates the symbol ‘A’.  If a different layout is used, say Greek,
 * it generates the symbol ‘α’.  And so on.
 *
 * Each such symbol is represented by a *keysym* (short for “key symbol”).
 * Note that keysyms are somewhat more general, in that they can also represent
 * some “function”, such as “Left” or “Right” for the arrow keys.  For more
 * information, see: Appendix A [“KEYSYM Encoding”][encoding] of the X Window
 * System Protocol.
 *
 * Specifically named keysyms can be found in the
 * xkbcommon/xkbcommon-keysyms.h header file.  Their name does not include
 * the `XKB_KEY_` prefix.
 *
 * Besides those, any Unicode/ISO&nbsp;10646 character in the range `U+0100` to
 * `U+10FFFF` can be represented by a keysym value in the range `0x01000100` to
 * `0x0110FFFF`.  The name of Unicode keysyms is `U<codepoint>`, e.g. `UA1B2`.
 *
 * The name of other unnamed keysyms is the hexadecimal representation of
 * their value, e.g. `0xabcd1234`.
 *
 * Keysym names are case-sensitive.
 *
 * @note **Encoding:** Keysyms are 32-bit integers with the 3 most significant
 * bits always set to zero.  Thus valid keysyms are in the range
 * `0 .. 0x1fffffff` = @ref XKB_KEYSYM_MAX.
 * See: Appendix A [“KEYSYM Encoding”][encoding] of the X Window System Protocol.
 *
 * [encoding]: https://www.x.org/releases/current/doc/xproto/x11protocol.html#keysym_encoding
 *
 * @ingroup keysyms
 * @sa `::XKB_KEYSYM_MAX`
 */
typedef uint32_t xkb_keysym_t;

/**
 * Index of a keyboard layout.
 *
 * The layout index is a state component which determines which <em>keyboard
 * layout</em> is active.  These may be different alphabets, different key
 * arrangements, etc.
 *
 * Layout indices are consecutive.  The first layout has index 0.
 *
 * Each layout is not required to have a name, and the names are not
 * guaranteed to be unique (though they are usually provided and unique).
 * Therefore, it is not safe to use the name as a unique identifier for a
 * layout.  Layout names are case-sensitive.
 *
 * Layout names are specified in the layout’s definition, for example
 * “English (US)”.  These are different from the (conventionally) short names
 * which are used to locate the layout, for example `us` or `us(intl)`.  These
 * names are not present in a compiled keymap.
 *
 * If the user selects layouts from a list generated from the XKB registry
 * (using libxkbregistry or directly), and this metadata is needed later on, it
 * is recommended to store it along with the keymap.
 *
 * Layouts are also called *groups* by XKB.
 *
 * @sa xkb_keymap::xkb_keymap_num_layouts()
 * @sa xkb_keymap::xkb_keymap_num_layouts_for_key()
 */
typedef uint32_t xkb_layout_index_t;
/** A mask of layout indices. */
typedef uint32_t xkb_layout_mask_t;

/**
 * Index of a shift level.
 *
 * Any key, in any layout, can have several <em>shift levels</em>.  Each
 * shift level can assign different keysyms to the key.  The shift level
 * to use is chosen according to the current keyboard state; for example,
 * if no keys are pressed, the first level may be used; if the Left Shift
 * key is pressed, the second; if Num Lock is pressed, the third; and
 * many such combinations are possible (see `xkb_mod_index_t`).
 *
 * Level indices are consecutive.  The first level has index 0.
 */
typedef uint32_t xkb_level_index_t;

/**
 * Index of a modifier.
 *
 * A @e modifier is a state component which changes the way keys are
 * interpreted.  A keymap defines a set of modifiers, such as Alt, Shift,
 * Num Lock or Meta, and specifies which keys may @e activate which
 * modifiers (in a many-to-many relationship, i.e. a key can activate
 * several modifiers, and a modifier may be activated by several keys.
 * Different keymaps do this differently).
 *
 * When retrieving the keysyms for a key, the active modifier set is
 * consulted; this determines the correct shift level to use within the
 * currently active layout (see `xkb_level_index_t`).
 *
 * Modifier indices are consecutive.  The first modifier has index 0.
 *
 * Each modifier must have a name, and the names are unique.  Therefore, it
 * is safe to use the name as a unique identifier for a modifier.  The names
 * of some common modifiers are provided in the `xkbcommon/xkbcommon-names.h`
 * header file.  Modifier names are case-sensitive.
 *
 * @sa `xkb_keymap::xkb_keymap_num_mods()`
 * @sa `xkb_mod_mask_t`
 */
typedef uint32_t xkb_mod_index_t;
/**
 * @parblock
 * A mask of [modifier encodings](@ref modifiers-encoding), i.e. a mask
 * of [real modifiers] indices.
 * @endparblock
 *
 * @warning A [modifier encoding](@ref modifiers-encoding) is **opaque**.
 *
 * @warning Computing a modifier mask from its index works for [real modifiers]
 * but does *not* work in general for [virtual modifiers].
 * Therefore the encoding of a modifier should be retrieved *only* using
 * `xkb_keymap::xkb_keymap_mod_get_mask()` or
 * `xkb_keymap::xkb_keymap_mod_get_mask2()`.
 *
 * @sa `xkb_keymap::xkb_keymap_mod_get_mask()`
 * @sa `xkb_keymap::xkb_keymap_mod_get_mask2()`
 *
 * [real modifiers]: @ref real-modifier-def
 * [virtual modifiers]: @ref virtual-modifier-def
 */
typedef uint32_t xkb_mod_mask_t;

/**
 * Index of a keyboard LED.
 *
 * LEDs are logical objects which may be @e active or @e inactive.  They
 * typically correspond to the lights on the keyboard. Their state is
 * determined by the current keyboard state.
 *
 * LED indices are non-consecutive.  The first LED has index 0.
 *
 * Each LED must have a name, and the names are unique. Therefore,
 * it is safe to use the name as a unique identifier for a LED.  The names
 * of some common LEDs are provided in the `xkbcommon/xkbcommon-names.h`
 * header file.  LED names are case-sensitive.
 *
 * @warning A given keymap may specify an exact index for a given LED.
 * Therefore, LED indexing is not necessarily sequential, as opposed to
 * modifiers and layouts.  This means that when iterating over the LEDs
 * in a keymap using e.g. `xkb_keymap::xkb_keymap_num_leds()`, some indices might
 * be invalid.
 * Given such an index, functions like `xkb_keymap::xkb_keymap_led_get_name()`
 * will return `NULL`, and `xkb_state::xkb_state_led_index_is_active()` will
 * return -1.
 *
 * LEDs are also called *indicators* by XKB.
 *
 * @sa `xkb_keymap::xkb_keymap_num_leds()`
 */
typedef uint32_t xkb_led_index_t;
/** A mask of LED indices. */
typedef uint32_t xkb_led_mask_t;

/** Invalid keycode */
#define XKB_KEYCODE_INVALID (0xffffffff)
/** Invalid layout index */
#define XKB_LAYOUT_INVALID  (0xffffffff)
/** Invalid level index */
#define XKB_LEVEL_INVALID   (0xffffffff)
/** Invalid modifier index */
#define XKB_MOD_INVALID     (0xffffffff)
/** Invalid LED index */
#define XKB_LED_INVALID     (0xffffffff)

/** Maximum legal keycode */
#define XKB_KEYCODE_MAX     (0xffffffff - 1)

/**
 * Maximum keysym value
 *
 * @since 1.6.0
 * @sa xkb_keysym_t
 * @ingroup keysyms
 */
#define XKB_KEYSYM_MAX      0x1fffffff

/**
 * Test whether a value is a valid extended keycode.
 * @sa xkb_keycode_t
 **/
#define xkb_keycode_is_legal_ext(key) ((key) <= XKB_KEYCODE_MAX)

/**
 * Test whether a value is a valid X11 keycode.
 * @sa xkb_keycode_t
 */
#define xkb_keycode_is_legal_x11(key) ((key) >= 8 && (key) <= 255)

/**
 * @defgroup rules-api Rules
 * Utility functions related to *rules*, whose purpose is introduced in:
 * @ref xkb-the-config "".
 *
 * @{
 */

/**
 * @struct xkb_rmlvo_builder
 * Opaque [RMLVO] configuration object.
 *
 * It denotes the configuration values by which a user picks a keymap.
 *
 * @see [Introduction to RMLVO][RMLVO]
 * @see @ref rules-api ""
 * @since 1.11.0
 *
 * [RMLVO]: @ref RMLVO-intro
 */
struct xkb_rmlvo_builder;

/**
 * @enum xkb_rmlvo_builder_flags
 * Flags for `xkb_rmlvo_builder_new()`.
 *
 * @since 1.11.0
 */
enum xkb_rmlvo_builder_flags {
    /**
     * Do not apply any flags.
     *
     * @since 1.11.0
     */
    XKB_RMLVO_BUILDER_NO_FLAGS = 0
};

/**
 * Create a new [RMLVO] builder.
 *
 * @param[in] context The context in which to create the builder.
 * @param[in] rules   The ruleset.
 * If `NULL` or the empty string `""`, a default value is used.
 * If the `XKB_DEFAULT_RULES` environment variable is set, it is used
 * as the default.  Otherwise the system default is used.
 * @param[in] model   The keyboard model.
 * If `NULL` or the empty string `""`, a default value is used.
 * If the `XKB_DEFAULT_MODEL` environment variable is set, it is used
 * as the default.  Otherwise the system default is used.
 * @param[in] flags   Optional flags for the builder, or 0.
 *
 * @returns A `xkb_rmlvo_builder`, or `NULL` if the compilation failed.
 *
 * @see `xkb_rule_names` for a detailed description of @p rules and @p model.
 * @since 1.11.0
 * @memberof xkb_rmlvo_builder
 *
 * [RMLVO]: @ref RMLVO-intro
 */
XKB_EXPORT struct xkb_rmlvo_builder*
xkb_rmlvo_builder_new(struct xkb_context *context,
                      const char *rules, const char *model,
                      enum xkb_rmlvo_builder_flags flags);

/**
 * Append a layout to the given [RMLVO] builder.
 *
 * @param[in,out] rmlvo         The builder to modify.
 * @param[in]     layout        The name of the layout.
 * @param[in]     variant       The name of the layout variant, or `NULL` to
 *                              select the default variant.
 * @param[in]     options       An array of options to apply only to this
 *                              layout, or `NULL` if there is no such options.
 * @param[in]     options_len   The length of @p options.
 *
 * @note The options are only effectual if the corresponding ruleset has the
 * proper rules to handle them as *layout-specific* options.
 * @note See `rxkb_option_is_layout_specific()` to query whether an option
 * supports the layout-specific feature.
 *
 * @returns `true` if the call succeeded, otherwise `false`.
 *
 * @since 1.11.0
 * @memberof xkb_rmlvo_builder
 *
 * [RMLVO]: @ref RMLVO-intro
 */
XKB_EXPORT bool
xkb_rmlvo_builder_append_layout(struct xkb_rmlvo_builder *rmlvo,
                                const char *layout, const char *variant,
                                const char* const* options, size_t options_len);

/**
 * Append an option to the given [RMLVO] builder.
 *
 * @param[in,out] rmlvo   The builder to modify.
 * @param[in]     option  The name of the option.
 *
 * @returns `true` if the call succeeded, otherwise `false`.
 *
 * @since 1.11.0
 * @memberof xkb_rmlvo_builder
 *
 * [RMLVO]: @ref RMLVO-intro
 */
XKB_EXPORT bool
xkb_rmlvo_builder_append_option(struct xkb_rmlvo_builder *rmlvo,
                                const char *option);

/**
 * Take a new reference on a [RMLVO] builder.
 *
 * @param[in] rmlvo The builder to reference.
 *
 * @returns The passed in builder.
 *
 * @since 1.11.0
 * @memberof xkb_rmlvo_builder
 *
 * [RMLVO]: @ref RMLVO-intro
 */
XKB_EXPORT struct xkb_rmlvo_builder *
xkb_rmlvo_builder_ref(struct xkb_rmlvo_builder *rmlvo);

/**
 * Release a reference on a [RMLVO] builder, and possibly free it.
 *
 * @param[in] rmlvo The builder.  If it is `NULL`, this function does nothing.
 *
 * @since 1.11.0
 * @memberof xkb_rmlvo_builder
 *
 * [RMLVO]: @ref RMLVO-intro
 */
XKB_EXPORT void
xkb_rmlvo_builder_unref(struct xkb_rmlvo_builder *rmlvo);

/**
 * @struct xkb_rule_names
 * Names to compile a keymap with, also known as [RMLVO].
 *
 * The names are the common configuration values by which a user picks
 * a keymap.
 *
 * If the entire struct is `NULL`, then each field is taken to be `NULL`.
 * You should prefer passing `NULL` instead of choosing your own defaults.
 *
 * @see [Introduction to RMLVO][RMLVO]
 * @see @ref rules-api ""
 *
 * [RMLVO]: @ref RMLVO-intro
 */
struct xkb_rule_names {
    /**
     * The rules file to use. The rules file describes how to interpret
     * the values of the model, layout, variant and options fields.
     *
     * If `NULL` or the empty string `""`, a default value is used.
     * If the `XKB_DEFAULT_RULES` environment variable is set, it is used
     * as the default.  Otherwise the system default is used.
     */
    const char *rules;
    /**
     * The keyboard model by which to interpret keycodes and LEDs.
     *
     * If `NULL` or the empty string `""`, a default value is used.
     * If the `XKB_DEFAULT_MODEL` environment variable is set, it is used
     * as the default.  Otherwise the system default is used.
     */
    const char *model;
    /**
     * A comma separated list of layouts (languages) to include in the
     * keymap.
     *
     * If `NULL` or the empty string `""`, a default value is used.
     * If the `XKB_DEFAULT_LAYOUT` environment variable is set, it is used
     * as the default.  Otherwise the system default is used.
     */
    const char *layout;
    /**
     * A comma separated list of variants, one per layout, which may
     * modify or augment the respective layout in various ways.
     *
     * Generally, should either be empty or have the same number of values
     * as the number of layouts. You may use empty values as in `intl,,neo`.
     *
     * If `NULL` or the empty string `""`, and a default value is also used
     * for the layout, a default value is used.  Otherwise no variant is
     * used.
     * If the `XKB_DEFAULT_VARIANT` environment variable is set, it is used
     * as the default.  Otherwise the system default is used.
     */
    const char *variant;
    /**
     * A comma separated list of options, through which the user specifies
     * non-layout related preferences, like which key combinations are used
     * for switching layouts, or which key is the Compose key.
     *
     * If `NULL`, a default value is used.  If the empty string `""`, no
     * options are used.
     * If the `XKB_DEFAULT_OPTIONS` environment variable is set, it is used
     * as the default.  Otherwise the system default is used.
     *
     * Each option can additionally have a *layout index specifier*, so that it
     * applies only if matching the given layout.  The index is specified by
     * appending `!` immediately after the option name, then the 1-indexed
     * target layout in decimal format: e.g. `ns:option!2`.  When no layout is
     * specified, it matches any layout.
     *
     * @note The layout index specifier is only effectual if the corresponding
     * ruleset has the proper rules to handle the option as *layout-specific*.
     * @note See `rxkb_option_is_layout_specific()` to query whether an option
     * supports the layout-specific feature.
     *
     * @since 1.11.0: Layout index specifier using `!`.
     */
    const char *options;
};

/**
 * @struct xkb_component_names
 * Keymap components, also known as [KcCGST].
 *
 * The components are the result of the [RMLVO] resolution.
 *
 * @see [Introduction to RMLVO][RMLVO]
 * @see [Introduction to KcCGST][KcCGST]
 * @see @ref rules-api ""
 *
 * [RMLVO]: @ref RMLVO-intro
 * [KcCGST]: @ref KcCGST-intro
 */
struct xkb_component_names {
    char *keycodes;
    char *compatibility;
    char *geometry;
    char *symbols;
    char *types;
};

/**
 * Resolve [RMLVO] names to [KcCGST] components.
 *
 * This function is used primarily for *debugging*. See
 * `xkb_keymap::xkb_keymap_new_from_names2()` for creating keymaps from
 * [RMLVO] names.
 *
 * @param[in]  context    The context in which to resolve the names.
 * @param[in]  rmlvo_in   The [RMLVO] names to use.
 * @param[out] rmlvo_out  The [RMLVO] names actually used after resolving
 * missing values.
 * @param[out] components_out The [KcCGST] components resulting of the [RMLVO]
 * resolution.
 *
 * @c rmlvo_out and @c components can be omitted by using `NULL`, but not both.
 *
 * If @c components is not `NULL`, it is filled with dynamically-allocated
 * strings that should be freed by the caller.
 *
 * @returns `true` if the [RMLVO] names could be resolved, `false` otherwise.
 *
 * @see [Introduction to RMLVO][RMLVO]
 * @see [Introduction to KcCGST][KcCGST]
 * @see xkb_rule_names
 * @see xkb_component_names
 * @see xkb_keymap::xkb_keymap_new_from_names2()
 *
 * @since 1.9.0
 * @memberof xkb_component_names
 *
 * [RMLVO]: @ref RMLVO-intro
 * [KcCGST]: @ref KcCGST-intro
 */
XKB_EXPORT bool
xkb_components_names_from_rules(struct xkb_context *context,
                                const struct xkb_rule_names *rmlvo_in,
                                struct xkb_rule_names *rmlvo_out,
                                struct xkb_component_names *components_out);

/** @} */

/**
 * @defgroup keysyms Keysyms
 * Utility functions related to [*keysyms*](@ref xkb_keysym_t) (short for
 * “key symbols”).
 *
 * @{
 */

/**
 * @page keysym-transformations Keysym Transformations
 *
 * Keysym translation is subject to several *keysym transformations*,
 * as described in the XKB specification.  These are:
 *
 * <dl>
 * <dt>Capitalization transformation</dt>
 * <dd>
 * If the **Caps Lock** [modifier] is
 * active and was not consumed by the translation process, keysyms
 * are transformed to their upper-case form (if applicable).
 * Similarly, the UTF-8/UTF-32 string produced is capitalized.
 *
 * This is described in:
 * https://www.x.org/releases/current/doc/kbproto/xkbproto.html#Interpreting_the_Lock_Modifier
 * </dd>
 * <dt>Control transformation</dt>
 * <dd>
 * If the **Control** [modifier] is active and was not consumed by the
 * translation process, the string produced is transformed to its matching
 * [ASCII control character]<!-- --> (if applicable). Keysyms are not affected.
 *
 * This is described in:
 * https://www.x.org/releases/current/doc/kbproto/xkbproto.html#Interpreting_the_Control_Modifier
 * </dd>
 * </dl>
 *
 * Each relevant function discusses which transformations it performs.
 *
 * These transformations are not applicable when a key produces multiple
 * keysyms.
 *
 * [modifier]: @ref modifier-def
 * [ASCII control character]: https://en.wikipedia.org/wiki/C0_and_C1_control_codes#ASCII
 */


/**
 * Get the name of a keysym.
 *
 * For a description of how keysyms are named, see @ref xkb_keysym_t.
 *
 * @param[in]  keysym The keysym.
 * @param[out] buffer A string buffer to write the name into.
 * @param[in]  size   Capacity of the buffer.
 *
 * @warning If the buffer passed is too small, the string is truncated
 * (though still `NULL`-terminated); a size of at least 64 bytes is recommended.
 *
 * @returns The number of bytes in the name, excluding the `NULL` byte. If
 * the keysym is invalid, returns -1.
 *
 * You may check if truncation has occurred by comparing the return value
 * with the length of buffer, similarly to the `snprintf(3)` function.
 *
 * @sa `xkb_keysym_t`
 */
XKB_EXPORT int
xkb_keysym_get_name(xkb_keysym_t keysym, char *buffer, size_t size);

/**
 * @enum xkb_keysym_flags
 * Flags for xkb_keysym_from_name().
 */
enum xkb_keysym_flags {
    /** Do not apply any flags. */
    XKB_KEYSYM_NO_FLAGS = 0,
    /** Find keysym by case-insensitive search. */
    XKB_KEYSYM_CASE_INSENSITIVE = (1 << 0)
};

/**
 * Get a keysym from its name.
 *
 * @param[in] name The name of a keysym. See remarks in `xkb_keysym_get_name()`;
 * this function will accept any name returned by that function.
 * @param[in] flags A set of flags controlling how the search is done. If
 * invalid flags are passed, this will fail with `XKB_KEY_NoSymbol`.
 *
 * If you use the `::XKB_KEYSYM_CASE_INSENSITIVE` flag and two keysym names
 * differ only by case, then the lower-case keysym name is returned.  For
 * instance, for `XKB_KEY_a` and `XKB_KEY_A`, this function would return
 * `XKB_KEY_a` for the case-insensitive search.  If this functionality is needed,
 * it is recommended to first call this function without this flag; and if that
 * fails, only then to try with this flag, while possibly warning the user
 * he had misspelled the name, and might get wrong results.
 *
 * Case folding is done according to the C locale; the current locale is not
 * consulted.
 *
 * @returns The keysym. If the name is invalid, returns `XKB_KEY_NoSymbol`.
 *
 * @sa xkb_keysym_t
 * @since 1.9.0: Enable support for [C0 and C1 control characters] in the Unicode
 * notation.
 *
 * [C0 and C1 control characters]: https://en.wikipedia.org/wiki/C0_and_C1_control_codes
 */
XKB_EXPORT xkb_keysym_t
xkb_keysym_from_name(const char *name, enum xkb_keysym_flags flags);

/**
 * Get the keysym corresponding to a *single* Unicode/UTF-8 encoded codepoint.
 *
 * @param[in] buffer A buffer to read the UTF-8 encoded codepoint from.
 * @param[in] size   Capacity of @p buffer.
 * @returns The keysym corresponding to the specified Unicode
 * codepoint, or `XKB_KEY_NoSymbol` if there is none.
 *
 * This function is the inverse of `xkb_keysym_to_utf8()`. In cases
 * where a single codepoint corresponds to multiple keysyms, returns
 * the keysym with the lowest value.
 *
 * Unicode codepoints which do not have a special (legacy) keysym
 * encoding use a direct encoding scheme. These keysyms don’t usually
 * have an associated keysym constant (`XKB_KEY_*`).
 *
 * @sa `xkb_keysym_to_utf8()`
 * @since 1.14.0
 */
XKB_EXPORT xkb_keysym_t
xkb_utf8_to_keysym(const char *buffer, size_t size);

/**
 * Get the Unicode/UTF-8 representation of a keysym.
 *
 * @param[in]  keysym The keysym.
 * @param[out] buffer A buffer to write the UTF-8 string into.
 * @param[in]  size   Capacity of @p buffer.  Must be at least 5.
 *
 * @returns The number of bytes written to the buffer (including the
 * terminating byte).  If the keysym does not have a Unicode
 * representation, returns 0.  If the buffer is too small, returns -1.
 *
 * This function does not perform any @ref keysym-transformations.
 * Therefore, prefer to use `xkb_state::xkb_state_key_get_utf8()` if possible.
 *
 * @sa `xkb_state::xkb_state_key_get_utf8()`
 */
XKB_EXPORT int
xkb_keysym_to_utf8(xkb_keysym_t keysym, char *buffer, size_t size);

/**
 * Get the Unicode/UTF-32 representation of a keysym.
 *
 * @returns The Unicode/UTF-32 representation of keysym, which is also
 * compatible with UCS-4.  If the keysym does not have a Unicode
 * representation, returns 0.
 *
 * This function does not perform any @ref keysym-transformations.
 * Therefore, prefer to use `xkb_state::xkb_state_key_get_utf32()` if possible.
 *
 * @sa `xkb_state::xkb_state_key_get_utf32()`
 */
XKB_EXPORT uint32_t
xkb_keysym_to_utf32(xkb_keysym_t keysym);

/**
 * Get the keysym corresponding to a Unicode/UTF-32 codepoint.
 *
 * @returns The keysym corresponding to the specified Unicode
 * codepoint, or `XKB_KEY_NoSymbol` if there is none.
 *
 * This function is the inverse of `xkb_keysym_to_utf32()`. In cases
 * where a single codepoint corresponds to multiple keysyms, returns
 * the keysym with the lowest value.
 *
 * Unicode codepoints which do not have a special (legacy) keysym
 * encoding use a direct encoding scheme. These keysyms don’t usually
 * have an associated keysym constant (`XKB_KEY_*`).
 *
 * @sa `xkb_keysym_to_utf32()`
 * @since 1.0.0
 * @since 1.9.0: Enable support for all noncharacters.
 */
XKB_EXPORT xkb_keysym_t
xkb_utf32_to_keysym(uint32_t codepoint);

/**
 * Convert a keysym to its *uppercase* form.
 *
 * If there is no such form, the keysym is returned unchanged.
 *
 * The conversion rules are the *simple* (i.e. one-to-one) Unicode case
 * mappings (with some exceptions, see hereinafter) and do not depend
 * on the locale. If you need the special case mappings (i.e. not
 * one-to-one or locale-dependent), prefer to work with the Unicode
 * representation instead, when possible.
 *
 * Exceptions to the Unicode mappings:
 *
 * | Lower keysym | Lower letter | Upper keysym | Upper letter | Comment |
 * | ------------ | ------------ | ------------ | ------------ | ------- |
 * | `ssharp`     | `U+00DF`: ß  | `SSHARP`     | `U+1E9E`: ẞ  | [Council for German Orthography] |
 *
 * [Council for German Orthography]: https://www.rechtschreibrat.com/regeln-und-woerterverzeichnis/
 *
 * @since 0.8.0: Initial implementation, based on `libX11`.
 * @since 1.8.0: Use Unicode 16.0 mappings for complete Unicode coverage.
 * @since 1.12.0: Update to Unicode 17.0.
 */
XKB_EXPORT xkb_keysym_t
xkb_keysym_to_upper(xkb_keysym_t keysym);

/**
 * Convert a keysym to its *lowercase* form.
 *
 * If there is no such form, the keysym is returned unchanged.
 *
 * The conversion rules are the *simple* (i.e. one-to-one) Unicode case
 * mappings and do not depend on the locale. If you need the special
 * case mappings (i.e. not one-to-one or locale-dependent), prefer to
 * work with the Unicode representation instead, when possible.
 *
 * @since 0.8.0: Initial implementation, based on `libX11`.
 * @since 1.8.0: Use Unicode 16.0 mappings for complete Unicode coverage.
 * @since 1.12.0: Update to Unicode 17.0.
 */
XKB_EXPORT xkb_keysym_t
xkb_keysym_to_lower(xkb_keysym_t keysym);

/** @} */

/**
 * @defgroup context Library Context
 * Creating, destroying and using library contexts.
 *
 * Every keymap compilation request must have a context associated with
 * it.  The context keeps around state such as the include path.
 *
 * @{
 */

/**
 * @page envvars Environment Variables
 *
 * The user may set some environment variables which affect the library:
 *
 * - `XKB_CONFIG_ROOT`, `XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH`,
 *   `XKB_CONFIG_VERSIONED_EXTENSIONS_PATH`, `XKB_CONFIG_EXTRA_PATH`,
 *   `XDG_CONFIG_DIR`, `HOME` - see @ref include-path.
 * - `XKB_LOG_LEVEL` - see `xkb_context::xkb_context_set_log_level()`.
 * - `XKB_LOG_VERBOSITY` - see `xkb_context::xkb_context_set_log_verbosity()`.
 * - `XKB_DEFAULT_RULES`, `XKB_DEFAULT_MODEL`, `XKB_DEFAULT_LAYOUT`,
 *   `XKB_DEFAULT_VARIANT`, `XKB_DEFAULT_OPTIONS` - see `xkb_rule_names`.
 */

/**
 * @enum xkb_context_flags
 * Flags for context creation.
 */
enum xkb_context_flags {
    /** Do not apply any context flags. */
    XKB_CONTEXT_NO_FLAGS = 0,
    /**
     * Create this context with an empty include path.
     *
     * This may be useful e.g.:
     * - to have full control over the included paths;
     * - for clients that do not need to access the XKB directories, e.g.
     *   if only retrieving keymap from the Wayland or X server. It avoids
     *   potential issues with directory access permissions.
     */
    XKB_CONTEXT_NO_DEFAULT_INCLUDES = (1 << 0),
    /**
     * Don’t take RMLVO names from the environment.
     *
     * @since 0.3.0
     */
    XKB_CONTEXT_NO_ENVIRONMENT_NAMES = (1 << 1),
    /**
     * Disable the use of secure_getenv for this context, so that privileged
     * processes can use environment variables. Client uses at their own risk.
     *
     * @since 1.5.0
     */
    XKB_CONTEXT_NO_SECURE_GETENV = (1 << 2)
};

/**
 * Create a new context.
 *
 * @param[in] flags Optional flags for the context, or 0.
 *
 * @returns A new context, or `NULL` on failure.
 *
 * @memberof xkb_context
 */
XKB_EXPORT struct xkb_context *
xkb_context_new(enum xkb_context_flags flags);

/**
 * Take a new reference on a context.
 *
 * @param[in] context The context object.
 *
 * @returns The passed in context.
 *
 * @memberof xkb_context
 */
XKB_EXPORT struct xkb_context *
xkb_context_ref(struct xkb_context *context);

/**
 * Release a reference on a context, and possibly free it.
 *
 * @param[in] context The context.  If it is `NULL`, this function does nothing.
 *
 * @memberof xkb_context
 */
XKB_EXPORT void
xkb_context_unref(struct xkb_context *context);

/**
 * Store custom user data in the context.
 *
 * This may be useful in conjunction with `xkb_context_set_log_fn()`
 * or other callbacks.
 *
 * @param[in,out] context   The context object.
 * @param[in]     user_data User data object.
 *
 * @memberof xkb_context
 */
XKB_EXPORT void
xkb_context_set_user_data(struct xkb_context *context, void *user_data);

/**
 * Retrieves stored user data from the context.
 *
 * @param[in,out] context The context object.
 *
 * @returns The stored user data.  If the user data wasn’t set, or the
 * passed in context is `NULL`, returns `NULL`.
 *
 * This may be useful to access private user data from callbacks like a
 * custom logging function.
 *
 * @memberof xkb_context
 **/
XKB_EXPORT void *
xkb_context_get_user_data(struct xkb_context *context);

/** @} */

/**
 * @defgroup include-path Include Paths
 * Manipulating the include paths in a context.
 *
 * The include paths are the file-system paths that are searched when an
 * include statement is encountered during keymap compilation.
 *
 * The default include paths are, in that lookup order:
 * - The path `$XDG_CONFIG_HOME/xkb`, where `$XDG_CONFIG_HOME` is the value of
 *   the environment variable `XDG_CONFIG_HOME`, with the usual fallback to
 *   `$HOME/.config/` if unset.
 * - The path `$HOME/.xkb`, where `$HOME` is the value of the environment
 *   variable `HOME`.
 * - The `XKB_CONFIG_EXTRA_PATH` environment variable, if defined, otherwise the
 *   system configuration directory, defined at library configuration time
 *   (usually `/etc/xkb`).
 * - Each subdirectory of each XKB extensions directory (versioned, then
 *   unversioned if no corresponding versioned subdirectory), listed in
 *   lexicographic order. The extensions directories are defined by the
 *   environment variables `XKB_CONFIG_VERSIONED_EXTENSIONS_PATH` and
 *   `XKB_CONFIG_UNVERSIONED_EXTENSIONS_PATH` and default to the system XKB
 *   root extensions directories, defined at library configuration time (usually
 *   `/usr/share/xkeyboard-config-<VERSION>.d` and
 *   `/usr/share/xkeyboard-config.d`).
 * - The `XKB_CONFIG_ROOT` environment variable, if defined, otherwise
 *   the system XKB root, defined at library configuration time
 *   (usually `/usr/share/xkeyboard-config-<VERSION>` or `/usr/share/X11/xkb`).
 *
 * @{
 */

/**
 * Append a new entry to the context’s include path.
 *
 * @returns 1 on success, or 0 if the include path could not be added or is
 * inaccessible.
 *
 * @memberof xkb_context
 */
XKB_EXPORT int
xkb_context_include_path_append(struct xkb_context *context, const char *path);

/**
 * Append the default include paths to the context’s include path.
 *
 * @returns 1 on success, or 0 if no default include path could be added.
 *
 * @memberof xkb_context
 */
XKB_EXPORT int
xkb_context_include_path_append_default(struct xkb_context *context);

/**
 * Reset the context’s include path to the default.
 *
 * Removes all entries from the context’s include path, and inserts the
 * default paths.
 *
 * @returns 1 on success, or 0 if the primary include path could not be added.
 *
 * @memberof xkb_context
 */
XKB_EXPORT int
xkb_context_include_path_reset_defaults(struct xkb_context *context);

/**
 * Remove all entries from the context’s include path.
 *
 * @memberof xkb_context
 */
XKB_EXPORT void
xkb_context_include_path_clear(struct xkb_context *context);

/**
 * Get the number of paths in the context’s include path.
 *
 * @memberof xkb_context
 */
XKB_EXPORT unsigned int
xkb_context_num_include_paths(struct xkb_context *context);

/**
 * Get a specific include path from the context’s include path.
 *
 * @returns The include path at the specified index.  If the index is
 * invalid, returns `NULL`.
 *
 * @memberof xkb_context
 */
XKB_EXPORT const char *
xkb_context_include_path_get(struct xkb_context *context, unsigned int index);

/** @} */

/**
 * @defgroup logging Logging Handling
 * Manipulating how logging from this library is handled.
 *
 * @{
 */

/**
 * @enum xkb_log_level
 * Specifies a logging level.
 */
enum xkb_log_level {
    XKB_LOG_LEVEL_CRITICAL = 10, /**< Log critical internal errors only. */
    XKB_LOG_LEVEL_ERROR = 20,    /**< Log all errors. */
    XKB_LOG_LEVEL_WARNING = 30,  /**< Log warnings and errors. */
    XKB_LOG_LEVEL_INFO = 40,     /**< Log information, warnings, and errors. */
    XKB_LOG_LEVEL_DEBUG = 50     /**< Log everything. */
};

/**
 * Set the current logging level.
 *
 * @param[in,out] context The context in which to set the logging level.
 * @param[in]     level   The logging level to use.  Only messages from this
 *                        level and below will be logged.
 *
 * The default level is `::XKB_LOG_LEVEL_ERROR`.  The environment variable
 * `XKB_LOG_LEVEL`, if set in the time the context was created, overrides the
 * default value.  It may be specified as a level number or name.
 *
 * @memberof xkb_context
 */
XKB_EXPORT void
xkb_context_set_log_level(struct xkb_context *context,
                          enum xkb_log_level level);

/**
 * Get the current logging level.
 *
 * @memberof xkb_context
 */
XKB_EXPORT enum xkb_log_level
xkb_context_get_log_level(struct xkb_context *context);

/**
 * Sets the current logging verbosity.
 *
 * The library can generate a number of warnings which are not helpful to
 * ordinary users of the library.  The verbosity may be increased if more
 * information is desired (e.g. when developing a new keymap).
 *
 * The default verbosity is 0.  The environment variable `XKB_LOG_VERBOSITY`,
 * if set in the time the context was created, overrides the default value.
 *
 * @param[in,out] context   The context in which to use the set verbosity.
 * @param[in]     verbosity The verbosity to use.  Currently used values are
 * 1 to 10, higher values being more verbose.  0 would result in no verbose
 * messages being logged.
 *
 * Most verbose messages are of level `::XKB_LOG_LEVEL_WARNING` or lower.
 *
 * @memberof xkb_context
 */
XKB_EXPORT void
xkb_context_set_log_verbosity(struct xkb_context *context, int verbosity);

/**
 * Get the current logging verbosity of the context.
 *
 * @memberof xkb_context
 */
XKB_EXPORT int
xkb_context_get_log_verbosity(struct xkb_context *context);

/**
 * Set a custom function to handle logging messages.
 *
 * @param[in,out] context The context in which to use the set logging function.
 * @param[in]     log_fn  The function that will be called for logging messages.
 * Passing `NULL` restores the default function, which logs to stderr.
 *
 * By default, log messages from this library are printed to stderr.  This
 * function allows you to replace the default behavior with a custom
 * handler.  The handler is only called with messages which match the
 * current logging level and verbosity settings for the context.
 * level is the logging level of the message.  @a format and @a args are
 * the same as in the `vprintf(3)` function.
 *
 * You may use `xkb_context::xkb_context_set_user_data()` on the context, and
 * then call `xkb_context::xkb_context_get_user_data()` from within the logging
 * function to provide it with additional private context.
 *
 * @memberof xkb_context
 */
XKB_EXPORT void
xkb_context_set_log_fn(struct xkb_context *context,
                       void (*log_fn)(struct xkb_context *context,
                                      enum xkb_log_level level,
                                      const char *format, va_list args));

/** @} */

/**
 * @defgroup keymap Keymap Creation
 * Creating and destroying keymaps.
 *
 * @{
 */

/**
 * @enum xkb_keymap_compile_flags
 * Flags for keymap compilation.
 */
enum xkb_keymap_compile_flags {
    /** Do not apply any flags. */
    XKB_KEYMAP_COMPILE_NO_FLAGS = 0,
    /**
     * Make the parser operate in *strict* mode.
     *
     * This useful mainly for debugging.
     *
     * When this flag is set, the following will raise an error:
     * - field type mismatch (e.g. a number instead of a string)
     * - unknown global variable
     * - unknown statement field
     * - unknown declaration
     * - unknown compound statement
     * - unknown action/action parameter
     * - invalid action parameter value
     * - TODO
     *
     * @since 1.14.0
     */
    XKB_KEYMAP_COMPILE_STRICT_MODE = (1 << 0)
};

/**
 * @enum xkb_keymap_format
 * The possible keymap formats.
 *
 * See @ref keymap-text-format-v1-v2 "" for the complete description of the
 * formats and @ref keymap-support "" for detailed differences between the
 * formats.
 *
 * @remark A keymap can be parsed in one format and serialized in another,
 * thanks to automatic fallback mechanisms.
 *
 * <table>
 * <caption>
 * Keymap format to use depending on the target protocol
 * </caption>
 * <thead>
 * <tr>
 * <th colspan="2">Protocol</th>
 * <th colspan="2">libxkbcommon keymap format</th>
 * </tr>
 * <tr>
 * <th>Name</th>
 * <th>Keymap format</th>
 * <th>Parsing</th>
 * <th>Serialization</th>
 * </tr>
 * </thead>
 * <tbody>
 * <tr>
 * <th>X11</th>
 * <td>XKB</td>
 * <td>
 * `::XKB_KEYMAP_FORMAT_TEXT_V1`
 * </td>
 * <td>
 * *Always* use `::XKB_KEYMAP_FORMAT_TEXT_V1`, since the other formats are
 * incompatible.
 * </td>
 * </tr>
 * <tr>
 * <th>Wayland</th>
 * <td><code>[xkb_v1]</code></td>
 * <td>
 * <dl>
 * <dt>Wayland compositors<dt>
 * <dd>
 * The format depends on the keyboard layout database (usually [xkeyboard-config]).
 * Note that since v2 is a superset of v1, compositors are encouraged to use
 * `::XKB_KEYMAP_FORMAT_TEXT_V2` whenever possible.
 * </dd>
 * <dt>Client apps</dt>
 * <dd>
 * Clients should use `::XKB_KEYMAP_FORMAT_TEXT_V1` to parse the keymap sent
 * by a Wayland compositor, at least until `::XKB_KEYMAP_FORMAT_TEXT_V2`
 * stabilizes.
 * </dd>
 * </td>
 * <td>
 * At the time of writing (July 2025), the Wayland <code>[xkb_v1]</code> keymap
 * format is only defined as “libxkbcommon compatible”. In theory it enables
 * flexibility, but the set of supported features varies depending on the
 * libxkbcommon version and libxkbcommon keymap format used. Unfortunately there
 * is currently no Wayland API for keymap format *negotiation*.
 *
 * Therefore the **recommended** serialization format is
 * `::XKB_KEYMAP_FORMAT_TEXT_V1`, in order to ensure maximum compatibility for
 * interchange.
 *
 * Serializing using `::XKB_KEYMAP_FORMAT_TEXT_V2` should be considered
 * **experimental**, as some clients may fail to parse the resulting string.
 * </td>
 * </tr>
 * </tbody>
 * </table>
 *
 * [xkb_v1]: https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_keyboard-enum-keymap_format
 * [xkeyboard-config]: https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config
 */
enum xkb_keymap_format {
    /**
     * The classic XKB text format, as generated by `xkbcomp -xkb`.
     *
     * @important This format should *always* be used when *serializing* a
     * keymap for **X11**.
     *
     * @important For the **Wayland** <code>[xkb_v1]</code> format, it is
     * advised to use this format as well for serializing, in order to ensure
     * maximum compatibility for interchange.
     *
     * [xkb_v1]: https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_keyboard-enum-keymap_format
     */
    XKB_KEYMAP_FORMAT_TEXT_V1 = 1,
    /**
     * Xkbcommon extensions of the classic XKB text format, **incompatible with
     * X11**.
     *
     * @important Do *not* use when *serializing* a keymap for **X11**
     * (incompatible).
     *
     * @important Considered *experimental* when *serializing* for **Wayland**:
     * at the time of writing (July 2025), there is only one XKB keymap format
     * <code>[xkb_v1]</code> in Wayland and no Wayland API for keymap format
     * *negotiation*, so the clients may not be able to parse the keymap if it
     * uses v2-specific features. Therefore a compositor may *parse* keymaps
     * using `::XKB_KEYMAP_FORMAT_TEXT_V2` but it should serialize them using
     * `::XKB_KEYMAP_FORMAT_TEXT_V1` and rely on the automatic *fallback*
     * mechanisms.
     *
     * @since 1.11.0
     *
     * [xkb_v1]: https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_keyboard-enum-keymap_format
     */
    XKB_KEYMAP_FORMAT_TEXT_V2 = 2
};

/**
 * Create a keymap from a [RMLVO] builder.
 *
 * The primary keymap entry point: creates a new XKB keymap from a set of
 * [RMLVO] \(Rules + Model + Layouts + Variants + Options) names.
 *
 * @param[in] rmlvo   The [RMLVO] builder to use.  See `xkb_rmlvo_builder`.
 * @param[in] format  The text format of the keymap file to compile.
 * @param[in] flags   Optional flags for the keymap, or 0.
 *
 * @returns A keymap compiled according to the [RMLVO] names, or `NULL` if
 * the compilation failed.
 *
 * @since 1.11.0
 * @since 1.14.0 Parser is lenient by default.
 * @sa `xkb_keymap_new_from_names2()`
 * @sa `xkb_rmlvo_builder`
 * @memberof xkb_keymap
 *
 * [RMLVO]: @ref RMLVO-intro
 */
XKB_EXPORT struct xkb_keymap *
xkb_keymap_new_from_rmlvo(const struct xkb_rmlvo_builder *rmlvo,
                          enum xkb_keymap_format format,
                          enum xkb_keymap_compile_flags flags);

/**
 * Create a keymap from [RMLVO] names.
 *
 * Same as `xkb_keymap_new_from_names2()`, but with the keymap format fixed to:
 * `::XKB_KEYMAP_FORMAT_TEXT_V2`.
 *
 * @deprecated Use `xkb_keymap_new_from_names2()` instead.
 * @since 1.11.0: Deprecated
 * @since 1.11.0: Use internally `::XKB_KEYMAP_FORMAT_TEXT_V2` instead of
 * `::XKB_KEYMAP_FORMAT_TEXT_V1`
 * @since 1.14.0 Parser is lenient by default.
 * @sa `xkb_keymap_new_from_names2()`
 * @sa `xkb_rule_names`
 * @sa `xkb_keymap_new_from_rmlvo()`
 * @memberof xkb_keymap
 *
 * [RMLVO]: @ref RMLVO-intro
 */
XKB_EXPORT struct xkb_keymap *
xkb_keymap_new_from_names(struct xkb_context *context,
                          const struct xkb_rule_names *names,
                          enum xkb_keymap_compile_flags flags);

/**
 * Create a keymap from [RMLVO] names.
 *
 * The primary keymap entry point: creates a new XKB keymap from a set of
 * [RMLVO] \(Rules + Model + Layouts + Variants + Options) names.
 *
 * @param[in] context The context in which to create the keymap.
 * @param[in] names   The [RMLVO] names to use.  See `xkb_rule_names`.
 * @param[in] format  The text format of the keymap file to compile.
 * @param[in] flags   Optional flags for the keymap, or 0.
 *
 * @returns A keymap compiled according to the [RMLVO] names, or `NULL` if
 * the compilation failed.
 *
 * @since 1.11.0
 * @since 1.14.0 Parser is lenient by default.
 * @sa `xkb_rule_names`
 * @sa `xkb_keymap_new_from_rmlvo()`
 * @memberof xkb_keymap
 *
 * [RMLVO]: @ref RMLVO-intro
 */
XKB_EXPORT struct xkb_keymap *
xkb_keymap_new_from_names2(struct xkb_context *context,
                           const struct xkb_rule_names *names,
                           enum xkb_keymap_format format,
                           enum xkb_keymap_compile_flags flags);

/**
 * Create a keymap from a keymap file.
 *
 * @param[in] context The context in which to create the keymap.
 * @param[in] file    The keymap file to compile.
 * @param[in] format  The text format of the keymap file to compile.
 * @param[in] flags   Optional flags for the keymap, or 0.
 *
 * @returns A keymap compiled from the given XKB keymap file, or `NULL` if
 * the compilation failed.
 *
 * The file must contain a complete keymap.  For example, in the
 * `::XKB_KEYMAP_FORMAT_TEXT_V1` format, this means the file must contain one
 * top level `%xkb_keymap` section, which in turn contains other required
 * sections.
 *
 * @since 1.14.0 Parser is lenient by default.
 *
 * @memberof xkb_keymap
 */
XKB_EXPORT struct xkb_keymap *
xkb_keymap_new_from_file(struct xkb_context *context, FILE *file,
                         enum xkb_keymap_format format,
                         enum xkb_keymap_compile_flags flags);

/**
 * Create a keymap from a keymap string.
 *
 * This is just like `xkb_keymap_new_from_file()`, but instead of a file, gets
 * the keymap as one enormous string.
 *
 * @since 1.14.0 Parser is lenient by default.
 * @see `xkb_keymap_new_from_file()`
 * @memberof xkb_keymap
 */
XKB_EXPORT struct xkb_keymap *
xkb_keymap_new_from_string(struct xkb_context *context, const char *string,
                           enum xkb_keymap_format format,
                           enum xkb_keymap_compile_flags flags);

/**
 * Create a keymap from a memory buffer.
 *
 * This is just like `xkb_keymap_new_from_string()`, but takes a @p length
 * argument so the input string does not have to be zero-terminated.
 *
 * @since 0.3.0
 * @since 1.14.0 Parser is lenient by default.
 * @see `xkb_keymap_new_from_string()`
 * @memberof xkb_keymap
 */
XKB_EXPORT struct xkb_keymap *
xkb_keymap_new_from_buffer(struct xkb_context *context, const char *buffer,
                           size_t length, enum xkb_keymap_format format,
                           enum xkb_keymap_compile_flags flags);

/**
 * Take a new reference on a keymap.
 *
 * @returns The passed in keymap.
 *
 * @memberof xkb_keymap
 */
XKB_EXPORT struct xkb_keymap *
xkb_keymap_ref(struct xkb_keymap *keymap);

/**
 * Release a reference on a keymap, and possibly free it.
 *
 * @param[in] keymap The keymap.  If it is `NULL`, this function does nothing.
 *
 * @memberof xkb_keymap
 */
XKB_EXPORT void
xkb_keymap_unref(struct xkb_keymap *keymap);

/**
 * Get the keymap as a string in the format from which it was created.
 * @sa `xkb_keymap::xkb_keymap_get_as_string()`
 **/
#define XKB_KEYMAP_USE_ORIGINAL_FORMAT ((enum xkb_keymap_format) -1)

/**
 * @enum xkb_keymap_serialize_flags
 * Flags to control keymap serialization.
 *
 * @since 1.12.0
 */
enum xkb_keymap_serialize_flags {
    /**
     * Do not apply any flags
     *
     * @since 1.12.0
     */
    XKB_KEYMAP_SERIALIZE_NO_FLAGS = 0,
    /**
     * Enable pretty-printing
     *
     * @since 1.12.0
     */
    XKB_KEYMAP_SERIALIZE_PRETTY = (1 << 0),
    /**
     * Do not drop unused bits (key types, compatibility entries)
     *
     * @since 1.12.0
     */
    XKB_KEYMAP_SERIALIZE_KEEP_UNUSED = (1 << 1),
    /**
     * Force all values to be explicit. This is useful mainly for debugging,
     * as it may increase considerably the size of the serialization.
     *
     * @since 1.14.0
     */
    XKB_KEYMAP_SERIALIZE_EXPLICIT = (1 << 2),
};

/**
 * Get the compiled keymap as a string.
 *
 * Same as `xkb_keymap::xkb_keymap_get_as_string2()` using
 * `::XKB_KEYMAP_SERIALIZE_NO_FLAGS`.
 *
 * @since 1.12.0: Drop unused types and compatibility entries and do not
 * pretty-print.
 *
 * @sa `xkb_keymap::xkb_keymap_get_as_string2()`
 * @memberof xkb_keymap
 */
XKB_EXPORT char *
xkb_keymap_get_as_string(struct xkb_keymap *keymap,
                         enum xkb_keymap_format format);

/**
 * Get the compiled keymap as a string.
 *
 * @param[in] keymap The keymap to get as a string.
 * @param[in] format The keymap format to use for the string.  You can pass
 * in the special value `::XKB_KEYMAP_USE_ORIGINAL_FORMAT` to use the format
 * from which the keymap was originally created. When used as an *interchange*
 * format such as Wayland <code>[xkb_v1]</code>, the format should be explicit.
 * @param[in] flags  Optional flags to control the serialization, or 0.
 *
 * @returns The keymap as a `NULL`-terminated string, or `NULL` if unsuccessful.
 *
 * The returned string may be fed back into `xkb_keymap_new_from_string()`
 * to get the exact same keymap (possibly in another process, etc.).
 *
 * The returned string is *dynamically allocated* and should be freed by the
 * caller.
 *
 * @since 1.12.0
 *
 * @sa `xkb_keymap_get_as_string()`
 * @sa `xkb_keymap_new_from_string()`
 * @memberof xkb_keymap
 *
 * [xkb_v1]: https://wayland.freedesktop.org/docs/html/apa.html#protocol-spec-wl_keyboard-enum-keymap_format
 */
XKB_EXPORT char *
xkb_keymap_get_as_string2(struct xkb_keymap *keymap,
                          enum xkb_keymap_format format,
                          enum xkb_keymap_serialize_flags flags);

/** @} */

/**
 * @defgroup components Keymap Components
 * Enumeration of state components in a keymap.
 *
 * @{
 */

/**
 * Get the minimum keycode in the keymap.
 *
 * @sa xkb_keycode_t
 * @memberof xkb_keymap
 * @since 0.3.1
 */
XKB_EXPORT xkb_keycode_t
xkb_keymap_min_keycode(struct xkb_keymap *keymap);

/**
 * Get the maximum keycode in the keymap.
 *
 * @sa xkb_keycode_t
 * @memberof xkb_keymap
 * @since 0.3.1
 */
XKB_EXPORT xkb_keycode_t
xkb_keymap_max_keycode(struct xkb_keymap *keymap);

/**
 * @struct xkb_keymap_key_iterator
 * Iterator over a keymap’s keys.
 *
 * @sa `xkb_keycode_t`
 * @sa `xkb_keymap_key_iterator_new()`
 * @sa `xkb_keymap_key_iterator_destroy()`
 * @since 1.14.0
 */
struct xkb_keymap_key_iterator;

/**
 * @enum xkb_keymap_key_iterator_flags
 * Flags for `xkb_keymap_key_iterator_new()`.
 *
 * @since 1.14.0
 */
enum xkb_keymap_key_iterator_flags {
    /**
     * Do not apply any flags.
     *
     * @since 1.14.0
     */
    XKB_KEYMAP_KEY_ITERATOR_NO_FLAGS = 0,
    /**
     * Iterate keys in *descending* order.
     *
     * @since 1.14.0
     */
    XKB_KEYMAP_KEY_ITERATOR_DESCENDING_ORDER = (1 << 0),
    /**
     * @parblock
     * Skip *unbound* keys, i.e. keys with no groups.
     * @endparblock
     *
     * @since 1.14.0
     */
    XKB_KEYMAP_KEY_ITERATOR_SKIP_UNBOUND = (1 << 1),
};

/**
 * Create a new iterator over a keymap’s keys.
 *
 * Intended use:
 *
 * ```c
 * struct xkb_keymap_key_iterator *iter = xkb_keymap_key_iterator_new(keymap, 0);
 * xkb_keycode_t kc;
 * while ((kc = xkb_keymap_key_iterator_next(iter)) != XKB_KEYCODE_INVALID) {
 *     // ...
 * }
 * xkb_keymap_key_iterator_destroy(iter);
 * ```
 *
 * @param[in] keymap The keymap to iterate over.
 * @param[in] flags  Flags to control the iterator behavior, or 0.
 *
 * @returns A new keys iterator, or `NULL` on failure.
 *
 * @sa `xkb_keymap_key_iterator`
 * @sa `xkb_keymap_key_iterator_flags`
 * @sa `xkb_keymap_key_iterator_next()`
 * @sa `xkb_keymap_key_iterator_destroy()`
 * @since 1.14.0
 * @memberof xkb_keymap_key_iterator
 */
XKB_EXPORT struct xkb_keymap_key_iterator *
xkb_keymap_key_iterator_new(struct xkb_keymap *keymap,
                            enum xkb_keymap_key_iterator_flags flags);

/**
 * Free a keymap’s keys iterator.
 *
 * @param[in] iter The iterator to free. If it is `NULL`, do nothing.
 *
 * @sa `xkb_keymap_key_iterator_new()`
 * @since 1.14.0
 * @memberof xkb_keymap_key_iterator
 */
XKB_EXPORT void
xkb_keymap_key_iterator_destroy(struct xkb_keymap_key_iterator *iter);

/**
 * Get the next [keycode] from a keymap’s keys iterator.
 *
 * The keycodes are returned in *ascending* order unless
 * `::XKB_KEYMAP_KEY_ITERATOR_DESCENDING_ORDER` was used to create the iterator.
 *
 * If a keymap is sparse, this function may be called fewer than
 * `max_keycode - min_keycode + 1` times.
 *
 * @param[in,out] iter The iterator to use.
 *
 * @returns A valid [keycode], otherwise `::XKB_KEYCODE_INVALID` in case there
 * are no more entries.
 *
 * @sa `xkb_keycode_t`
 * @since 1.14.0
 * @memberof xkb_keymap_key_iterator
 *
 * [keycode]: @ref xkb_keycode_t
 */
XKB_EXPORT xkb_keycode_t
xkb_keymap_key_iterator_next(struct xkb_keymap_key_iterator *iter);

/**
 * The iterator used by `xkb_keymap_key_for_each()`.
 *
 * @sa `xkb_keymap_key_for_each()`
 * @memberof xkb_keymap
 * @since 0.3.1
 */
typedef void
(*xkb_keymap_key_iter_t)(struct xkb_keymap *keymap, xkb_keycode_t key,
                         void *data);

/**
 * Run a specified function for every valid keycode in the keymap.  If a
 * keymap is sparse, this function may be called fewer than
 * (max_keycode - min_keycode + 1) times with success.
 *
 * @sa `xkb_keymap_key_iterator`, which offers more control on the iteration.
 * @sa `xkb_keymap_min_keycode()`
 * @sa `xkb_keymap_max_keycode()`
 * @sa `xkb_keycode_t`
 * @memberof xkb_keymap
 * @since 0.3.1
 */
XKB_EXPORT void
xkb_keymap_key_for_each(struct xkb_keymap *keymap, xkb_keymap_key_iter_t iter,
                        void *data);

/**
 * Find the name of the key with the given keycode.
 *
 * This function always returns the canonical name of the key (see
 * description in `xkb_keycode_t`).
 *
 * @param[in] keymap  The keymap to query.
 * @param[in] key     The key to query.
 *
 * @returns The key name. If no key with this keycode exists,
 * returns `NULL`.
 *
 * @sa xkb_keycode_t
 * @memberof xkb_keymap
 * @since 0.6.0
 */
XKB_EXPORT const char *
xkb_keymap_key_get_name(struct xkb_keymap *keymap, xkb_keycode_t key);

/**
 * Find the keycode of the key with the given name.
 *
 * The name can be either a canonical name or an alias.
 *
 * @returns The keycode. If no key with this name exists,
 * returns `::XKB_KEYCODE_INVALID`.
 *
 * @sa xkb_keycode_t
 * @memberof xkb_keymap
 * @since 0.6.0
 */
XKB_EXPORT xkb_keycode_t
xkb_keymap_key_by_name(struct xkb_keymap *keymap, const char *name);

/**
 * Get the number of modifiers in the keymap.
 *
 * @sa xkb_mod_index_t
 * @memberof xkb_keymap
 */
XKB_EXPORT xkb_mod_index_t
xkb_keymap_num_mods(struct xkb_keymap *keymap);

/**
 * Get the name of a modifier by index.
 *
 * @returns The name.  If the index is invalid, returns `NULL`.
 *
 * @sa xkb_mod_index_t
 * @memberof xkb_keymap
 */
XKB_EXPORT const char *
xkb_keymap_mod_get_name(struct xkb_keymap *keymap, xkb_mod_index_t idx);

/**
 * Get the index of a modifier by name.
 *
 * @returns The index.  If no modifier with this name exists, returns
 * `::XKB_MOD_INVALID`.
 *
 * @sa xkb_mod_index_t
 * @memberof xkb_keymap
 */
XKB_EXPORT xkb_mod_index_t
xkb_keymap_mod_get_index(struct xkb_keymap *keymap, const char *name);

/**
 * Get the encoding of a modifier by name.
 *
 * In X11 terminology it corresponds to the mapping to the <em>[real modifiers]</em>.
 *
 * @returns The encoding of a modifier.  Note that it may be 0 if the name does
 * not exist or if the modifier is not mapped.
 *
 * @since 1.10.0
 * @sa `xkb_keymap_mod_get_mask2()`
 * @memberof xkb_keymap
 *
 * [real modifiers]: @ref real-modifier-def
 */
XKB_EXPORT xkb_mod_mask_t
xkb_keymap_mod_get_mask(struct xkb_keymap *keymap, const char *name);

/**
 * Get the encoding of a modifier by index.
 *
 * In X11 terminology it corresponds to the mapping to the <em>[real modifiers]</em>.
 *
 * @returns The encoding of a modifier.  Note that it may be 0 if the modifier is
 * not mapped.
 *
 * @since 1.11.0
 * @sa `xkb_keymap_mod_get_mask()`
 * @memberof xkb_keymap
 *
 * [real modifiers]: @ref real-modifier-def
 */
XKB_EXPORT xkb_mod_mask_t
xkb_keymap_mod_get_mask2(struct xkb_keymap *keymap, xkb_mod_index_t idx);

/**
 * Get the number of layouts in the keymap.
 *
 * @sa `xkb_layout_index_t`
 * @sa `xkb_rule_names`
 * @sa `xkb_keymap_num_layouts_for_key()`
 * @memberof xkb_keymap
 */
XKB_EXPORT xkb_layout_index_t
xkb_keymap_num_layouts(struct xkb_keymap *keymap);

/**
 * Get the name of a layout by index.
 *
 * @returns The name.  If the index is invalid, or the layout does not have
 * a name, returns `NULL`.
 *
 * @sa xkb_layout_index_t
 *     For notes on layout names.
 * @memberof xkb_keymap
 */
XKB_EXPORT const char *
xkb_keymap_layout_get_name(struct xkb_keymap *keymap, xkb_layout_index_t idx);

/**
 * Get the index of a layout by name.
 *
 * @returns The index.  If no layout exists with this name, returns
 * `::XKB_LAYOUT_INVALID`.  If more than one layout in the keymap has this name,
 * returns the lowest index among them.
 *
 * @sa `xkb_layout_index_t` for notes on layout names.
 * @memberof xkb_keymap
 */
XKB_EXPORT xkb_layout_index_t
xkb_keymap_layout_get_index(struct xkb_keymap *keymap, const char *name);

/**
 * Get the number of LEDs in the keymap.
 *
 * @warning The range [ 0...`xkb_keymap_num_leds()` ) includes all of the LEDs
 * in the keymap, but may also contain inactive LEDs.  When iterating over
 * this range, you need the handle this case when calling functions such as
 * `xkb_keymap_led_get_name()` or `xkb_state::xkb_state_led_index_is_active()`.
 *
 * @sa xkb_led_index_t
 * @memberof xkb_keymap
 */
XKB_EXPORT xkb_led_index_t
xkb_keymap_num_leds(struct xkb_keymap *keymap);

/**
 * Get the name of a LED by index.
 *
 * @returns The name.  If the index is invalid, returns `NULL`.
 *
 * @memberof xkb_keymap
 */
XKB_EXPORT const char *
xkb_keymap_led_get_name(struct xkb_keymap *keymap, xkb_led_index_t idx);

/**
 * Get the index of a LED by name.
 *
 * @returns The index.  If no LED with this name exists, returns
 * `::XKB_LED_INVALID`.
 *
 * @memberof xkb_keymap
 */
XKB_EXPORT xkb_led_index_t
xkb_keymap_led_get_index(struct xkb_keymap *keymap, const char *name);

/**
 * Get the number of layouts for a specific key.
 *
 * This number can be different from `xkb_keymap_num_layouts()`, but is always
 * smaller.  It is the appropriate value to use when iterating over the
 * layouts of a key.
 *
 * @param[in] keymap  The keymap to query.
 * @param[in] key     The key to query.
 *
 * @returns The number of layouts corresponding to the given key if it is valid
 * in the given keymap, otherwise 0 if the key is undefined or unbound.
 *
 * @sa xkb_layout_index_t
 * @memberof xkb_keymap
 */
XKB_EXPORT xkb_layout_index_t
xkb_keymap_num_layouts_for_key(struct xkb_keymap *keymap, xkb_keycode_t key);

/**
 * Get the number of shift levels for a specific key and layout.
 *
 * If @c layout is out of range for this key (that is, larger or equal to
 * the value returned by `xkb_keymap_num_layouts_for_key()`), it is brought
 * back into range in a manner consistent with
 * `xkb_state::xkb_state_key_get_layout()`.
 *
 * @sa xkb_level_index_t
 * @memberof xkb_keymap
 */
XKB_EXPORT xkb_level_index_t
xkb_keymap_num_levels_for_key(struct xkb_keymap *keymap, xkb_keycode_t key,
                              xkb_layout_index_t layout);

/**
 * Retrieves every possible modifier mask that produces the specified
 * shift level for a specific key and layout.
 *
 * This API is useful for inverse key transformation; i.e. finding out
 * which modifiers need to be active in order to be able to type the
 * keysym(s) corresponding to the specific key code, layout and level.
 *
 * @warning It returns only up to masks_size modifier masks. If the
 * buffer passed is too small, some of the possible modifier combinations
 * will not be returned.
 *
 * @param[in] keymap      The keymap.
 * @param[in] key         The keycode of the key.
 * @param[in] layout      The layout for which to get modifiers.
 * @param[in] level       The shift level in the layout for which to get the
 * modifiers. This should be smaller than:
 * @code xkb_keymap_num_levels_for_key(keymap, key) @endcode
 * @param[out] masks_out  A buffer in which the requested masks should be
 * stored.
 * @param[in] masks_size The capacity of the buffer pointed to by
 * @p masks_out.
 *
 * If @c layout is out of range for this key (that is, larger or equal to
 * the value returned by `xkb_keymap_num_layouts_for_key()`), it is brought
 * back into range in a manner consistent with
 * `xkb_state::xkb_state_key_get_layout()`.
 *
 * @returns The number of modifier masks stored in the masks_out array.
 * If the key is not in the keymap or if the specified shift level cannot
 * be reached it returns 0 and does not modify the @p masks_out buffer.
 *
 * @sa xkb_level_index_t
 * @sa xkb_mod_mask_t
 * @memberof xkb_keymap
 * @since 1.0.0
 */
XKB_EXPORT size_t
xkb_keymap_key_get_mods_for_level(struct xkb_keymap *keymap,
                                  xkb_keycode_t key,
                                  xkb_layout_index_t layout,
                                  xkb_level_index_t level,
                                  xkb_mod_mask_t *masks_out,
                                  size_t masks_size);

/**
 * Get the keysyms obtained from pressing a key in a given layout and
 * shift level.
 *
 * This function is like `xkb_state::xkb_state_key_get_syms()`, only the layout
 * and shift level are not derived from the keyboard state but are instead
 * specified explicitly.
 *
 * @param[in] keymap    The keymap.
 * @param[in] key       The keycode of the key.
 * @param[in] layout    The layout for which to get the keysyms.
 * @param[in] level     The shift level in the layout for which to get the
 * keysyms. This should be smaller than:
 * @code xkb_keymap_num_levels_for_key(keymap, key) @endcode
 * @param[out] syms_out An immutable array of keysyms corresponding to the
 * key in the given layout and shift level.
 *
 * If @c layout is out of range for this key (that is, larger or equal to
 * the value returned by `xkb_keymap_num_layouts_for_key()`), it is brought
 * back into range in a manner consistent with
 * `xkb_state::xkb_state_key_get_layout()`.
 *
 * @returns The number of keysyms in the syms_out array.  If no keysyms
 * are produced by the key in the given layout and shift level, returns 0
 * and sets @p syms_out to `NULL`.
 *
 * @sa `xkb_state::xkb_state_key_get_syms()`
 * @memberof xkb_keymap
 */
XKB_EXPORT int
xkb_keymap_key_get_syms_by_level(struct xkb_keymap *keymap,
                                 xkb_keycode_t key,
                                 xkb_layout_index_t layout,
                                 xkb_level_index_t level,
                                 const xkb_keysym_t **syms_out);

/**
 * Determine whether a key should repeat or not.
 *
 * A keymap may specify different repeat behaviors for different keys.
 * Most keys should generally exhibit repeat behavior; for example, holding
 * the `a` key down in a text editor should normally insert a single ‘a’
 * character every few milliseconds, until the key is released.  However,
 * there are keys which should not or do not need to be repeated.  For
 * example, repeating modifier keys such as Left/Right Shift or Caps Lock
 * is not generally useful or desired.
 *
 * @returns 1 if the key should repeat, 0 otherwise.
 *
 * @memberof xkb_keymap
 */
XKB_EXPORT int
xkb_keymap_key_repeats(struct xkb_keymap *keymap, xkb_keycode_t key);

/** @} */

/**
 * @defgroup state Keyboard State
 * Creating, destroying and manipulating keyboard state objects.
 *
 * @{
 */

/**
 * @page server-client-state Server State and Client State
 * @parblock
 *
 * There are two distinct actors in most window-system architectures:
 *
 * <dl>
 * <dt>Server</dt>
 * <dd>
 * For example: a Wayland compositor, an X11 server or an evdev listener.
 *
 * Servers maintain the XKB state for a device according to input events from
 * the device, such as key presses and releases, and out-of-band events from
 * the user, like UI layout switchers.
 * </dd>
 * <dt>Client</dt>
 * <dd>
 * For example: a Wayland client or an X11 client.
 *
 * Clients do not listen to input from the device; instead, whenever the
 * server state changes, the server serializes the state and notifies the
 * clients that the state has changed; the clients then update the state
 * from the serialization.
 * </dd>
 * </dl>
 *
 * There are two corresponding APIs:
 *
 * <dl>
 * <dt>`xkb_machine`: the *server* API</dt>
 * <dd>
 * This is the recommended API for **server** applications. It enables the full
 * feature set that libxkbcommon supports.
 *
 * `xkb_machine` is a [Mealy machine]<!-- -->: it is a finite-state machine that takes a
 * stream of raw key events – a pair ([keycode], [direction]) – as input, and
 * produces a stream of atomic [XKB events](@ref xkb_event) as output.
 *
 * The observable state of the machine is exposed via a companion `xkb_state`
 * object:
 * - Create it with `xkb_state::xkb_state_new_with_mode()` using
 *   `::XKB_STATE_MODE_SERVER_QUERY`.
 * - Update it with `xkb_state::xkb_state_update_event()`.
 * - Query it (keysyms, modifiers, layout, LEDs) via the `xkb_state` query API.
 *
 * Note that the `xkb_machine` API supports events other than state
 * components changes, such as key press/release events, so that it enables
 * handling most of the XKB [key actions](@ref key-action-def).
 *
 * See the [example for a Wayland server](@ref quick-guide-wayland-server)
 * in the quick guide.
 *
 * @since 1.14.0
 * </dd>
 * <dt>`xkb_state`: the *client* API (and legacy server API)</dt>
 * <dd>
 * This is the API for **client** applications and the *legacy API* for
 * **server** applications.
 *
 * <dl>
 * <dt>*Client* applications</dt>
 * <dd>
 * Create the state object with `xkb_state::xkb_state_new_with_mode()` using
 * `::XKB_STATE_MODE_CLIENT`, then update it via
 * `xkb_state::xkb_state_update_mask()` from server serializations.
 * </dd>
 * <dt>*Server* applications not using `xkb_machine`</dt>
 * <dd>
 * Create the state object with `xkb_state::xkb_state_new_with_mode()` using
 * `::XKB_STATE_MODE_SERVER`, or with the legacy
 * `xkb_state::xkb_state_new()` constructor, then update it via
 * `xkb_state::xkb_state_update_key()` for key events and
 * `xkb_state::xkb_state_update_synthetic()` for out-of-band inputs such as
 * layout switchers.
 * </dd>
 * </dl>
 *
 * @warning Some entry points in the `xkb_state` API are only meant for servers
 * and some are only meant for clients. Thus it is recommended to use
 * `xkb_state::xkb_state_new_with_mode()` because it enforces correct usage at
 * runtime and logs misuse as `::XKB_ERROR_UNEXPECTED_STATE_MODE`.
 * Using `xkb_state::xkb_state_new()` does not enforce this: mixing entry
 * points may lead to *incorrect state*.
 *
 * @note Since version 1.14.0, *server* applications should use the
 * `xkb_machine` API, which supports more features.
 *
 * See the [examples for clients](@ref quick-guide-clients) in the quick guide.
 * </dd>
 * </dl>
 *
 * @endparblock
 *
 * [Mealy machine]: https://en.wikipedia.org/wiki/Mealy_machine
 * [keycode]: @ref xkb_keycode_t
 * [direction]: @ref xkb_key_direction
 * [state machine]: @ref xkb_machine
 * [keyboard events]: @ref xkb_event
 * [event batch]: @ref xkb_events
 * [event]: @ref xkb_event
 */

/**
 * @struct xkb_event
 * Opaque keyboard state event object.
 *
 * Events are produced by `xkb_machine::xkb_machine_process_key()` and
 * `xkb_machine::xkb_machine_process_synthetic()` and collected into an
 * `xkb_events` batch. Each event represents one atomic state change or key
 * action within a frame.
 *
 * Inspect the event type with `xkb_event::xkb_event_get_type()`, then extract
 * data with the appropriate `xkb_event::xkb_event_get_*()` or
 * `xkb_event::xkb_event_serialize_*()` functions.
 *
 * @warning Event pointers are only valid until the next call to
 * `xkb_machine::xkb_machine_process_key()` or
 * `xkb_machine::xkb_machine_process_synthetic()` on the
 * same state machine. Do not store them beyond that point.
 *
 * @since 1.14.0
 *
 * @sa `xkb_event_type`
 * @sa `xkb_events`
 */
struct xkb_event;

/**
 * @enum xkb_event_type
 * Denotes the type of a [state event](@ref xkb_event).
 *
 * @since 1.14.0
 */
enum xkb_event_type {
    /**
     * **Key _down_** event
     *
     * @since 1.14.0
     */
    XKB_EVENT_TYPE_KEY_DOWN = 1,
    /**
     * **Key _repeated_** event
     *
     * @since 1.14.0
     */
    XKB_EVENT_TYPE_KEY_REPEATED,
    /**
     * **Key _up_** event
     *
     * @since 1.14.0
     */
    XKB_EVENT_TYPE_KEY_UP,
    /**
     * **Components** change event
     *
     * @since 1.14.0
     */
    XKB_EVENT_TYPE_COMPONENTS_CHANGE,
};

/**
 * Get the [type](@ref xkb_event_type) of an event.
 *
 * @param[in] event The event to process.
 *
 * @returns The event’s type.
 *
 * @since 1.14.0
 *
 * @memberof xkb_event
 */
XKB_EXPORT enum xkb_event_type
xkb_event_get_type(const struct xkb_event *event);

/**
 * Get the keycode associated to a [state event](@ref xkb_event) of type
 * `::XKB_EVENT_TYPE_KEY_DOWN`, `::XKB_EVENT_TYPE_KEY_REPEATED` or
 * `::XKB_EVENT_TYPE_KEY_UP`.
 *
 * @param[in] event The event object to process.
 *
 * @pre The event must be of one of the following types:
 * - `::XKB_EVENT_TYPE_KEY_DOWN`
 * - `::XKB_EVENT_TYPE_KEY_REPEATED`
 * - `::XKB_EVENT_TYPE_KEY_UP`
 * Otherwise the result is *undefined*.
 *
 * @returns The keycode corresponding to the event.
 *
 * @since 1.14.0
 *
 * @memberof xkb_event
 */
XKB_EXPORT xkb_keycode_t
xkb_event_get_keycode(const struct xkb_event *event);

/**
 * @enum xkb_state_component
 * Component types for state objects, which belong to the following categories:
 *
 * - [modifier],
 * - [layout],
 * - [indicator] \(LED),
 * - [keyboard global control].
 *
 * This enum is bitmaskable, e.g.
 * `(::XKB_STATE_MODS_DEPRESSED | ::XKB_STATE_MODS_LATCHED)`
 * is valid to exclude locked modifiers.
 *
 * In XKB, the `DEPRESSED` components are also known as *base*.
 *
 * [modifier]: @ref modifier-def
 * [layout]: @ref layout-def
 * [indicator]: @ref indicator-def
 * [keyboard global control]: @ref xkb_keyboard_control_flags
 */
enum xkb_state_component {
    /**
     * @parblock
     * [Depressed modifiers], i.e. a key is physically holding them.
     * @endparblock
     *
     * [Depressed modifiers]: @ref depressed-mod-def
     */
    XKB_STATE_MODS_DEPRESSED = (1 << 0),
    /**
     * @parblock
     * [Latched modifiers], i.e. will be unset after the next non-modifier
     * key press.
     * @endparblock
     *
     * [Latched modifiers]: @ref latched-mod-def
     */
    XKB_STATE_MODS_LATCHED = (1 << 1),
    /**
     * @parblock
     * [Locked modifiers], i.e. will be unset after the key provoking the
     * lock has been pressed again.
     * @endparblock
     *
     * [Locked modifiers]: @ref locked-mod-def
     */
    XKB_STATE_MODS_LOCKED = (1 << 2),
    /**
     * @parblock
     * [Effective modifiers], i.e. currently active and affect key
     * processing (derived from the other state components).
     * @endparblock
     * Use this unless you explicitly care how the state came about.
     *
     * [Effective modifiers]: @ref effective-modifier-encoding
     */
    XKB_STATE_MODS_EFFECTIVE = (1 << 3),
    /**
     * @parblock
     * [Depressed layout], i.e. a key is physically holding it.
     * @endparblock
     *
     * [Depressed layout]: @ref depressed-group-def
     */
    XKB_STATE_LAYOUT_DEPRESSED = (1 << 4),
    /**
     * @parblock
     * [Latched layout], i.e. will be unset after the next non-modifier
     * key press.
     * @endparblock
     *
     * [Latched layout]: @ref latched-group-def
     */
    XKB_STATE_LAYOUT_LATCHED = (1 << 5),
    /**
     * @parblock
     * [Locked layout], i.e. will be unset after the key provoking the lock
     * has been pressed again.
     * @endparblock
     *
     * [Locked layout]: @ref locked-group-def
     */
    XKB_STATE_LAYOUT_LOCKED = (1 << 6),
    /**
     * @parblock
     * Effective layout, i.e. currently active and affects key processing
     * (derived from the other state components).
     * @endparblock
     * Use this unless you explicitly care how the state came about.
     */
    XKB_STATE_LAYOUT_EFFECTIVE = (1 << 7),
    /**
     * [LEDs] \(derived from the other state components).
     *
     * [LEDs]: @ref indicator-def
     */
    XKB_STATE_LEDS = (1 << 8),
    /**
     * Effective [keyboard controls]
     *
     * @since 1.14.0
     *
     * [keyboard controls]: @ref xkb_keyboard_control_flags
     */
    XKB_STATE_CONTROLS = (1 << 9)
};

/**
 * Get the [state components](@ref xkb_state_component) changes corresponding
 * to a [state event](@ref xkb_event) of type
 * `::XKB_EVENT_TYPE_COMPONENTS_CHANGE` .
 *
 * @param[in] event The event object to process.
 *
 * @pre The event must be of type `::XKB_EVENT_TYPE_COMPONENTS_CHANGE`.
 * Otherwise the result is *undefined*.
 *
 * @returns The corresponding mask of state components that have changed.
 * If nothing in the state has changed, returns 0.
 *
 * @since 1.14.0
 *
 * @memberof xkb_event
 */
XKB_EXPORT enum xkb_state_component
xkb_event_get_changed_components(const struct xkb_event *event);

/**
 * @enum xkb_keyboard_control_flags
 * _Boolean_ **global keyboard controls**, which affect the way libxkbcommon
 * handles the keyboard as a whole.
 *
 * This enumeration is bit-maskable.
 *
 * @since 1.14.0
 */
enum xkb_keyboard_control_flags {
    /**
     * Do not apply any control.
     *
     * @since 1.14.0
     */
    XKB_KEYBOARD_CONTROL_NO_FLAGS = 0,
    /**
     * **Sticky keys** is an accessibility feature primarily aimed at helping
     * people that find it difficult or impossible to press two keys at once.
     *
     * The `::XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS` control makes it easier for
     * them to type by changing the behavior of the *modifier* and *group switch*
     * keys. When *sticky keys* are enabled, <em>[set]</em> modifiers/group
     * switch are transformed into their corresponding <em>[latch]</em> version:
     * e.g. the user can first press a modifier, release it, then press another
     * key.
     *
     * @sa `::XKB_A11Y_LATCH_TO_LOCK`
     * @since 1.14.0
     *
     * [set]:   @ref depressed-mod-def
     * [latch]: @ref latched-mod-def
     */
    XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS = (1 << 0),
    /**
     * Enable the [keyboard overlay](@ref key-behavior-overlay) **1**.
     *
     * @since 1.14.0
     */
    XKB_KEYBOARD_CONTROL_OVERLAY1 = (1 << 1),
    /**
     * Enable the [keyboard overlay](@ref key-behavior-overlay) **2**.
     *
     * @since 1.14.0
     */
    XKB_KEYBOARD_CONTROL_OVERLAY2 = (1 << 2),
    /**
     * Enable the [keyboard overlay](@ref key-behavior-overlay) **3**.
     *
     * @since 1.14.0
     */
    XKB_KEYBOARD_CONTROL_OVERLAY3 = (1 << 3),
    /**
     * Enable the [keyboard overlay](@ref key-behavior-overlay) **4**.
     *
     * @since 1.14.0
     */
    XKB_KEYBOARD_CONTROL_OVERLAY4 = (1 << 4),
    /**
     * Enable the [keyboard overlay](@ref key-behavior-overlay) **5**.
     *
     * @since 1.14.0
     */
    XKB_KEYBOARD_CONTROL_OVERLAY5 = (1 << 5),
    /**
     * Enable the [keyboard overlay](@ref key-behavior-overlay) **6**.
     *
     * @since 1.14.0
     */
    XKB_KEYBOARD_CONTROL_OVERLAY6 = (1 << 6),
    /**
     * Enable the [keyboard overlay](@ref key-behavior-overlay) **7**.
     *
     * @since 1.14.0
     */
    XKB_KEYBOARD_CONTROL_OVERLAY7 = (1 << 7),
    /**
     * Enable the [keyboard overlay](@ref key-behavior-overlay) **8**.
     *
     * @since 1.14.0
     */
    XKB_KEYBOARD_CONTROL_OVERLAY8 = (1 << 8),
};

/**
 * Serialization of the *boolean* [global keyboard controls]
 * corresponding to a [state event](@ref xkb_event) of type
 * `::XKB_EVENT_TYPE_COMPONENTS_CHANGE` .
 *
 * @param[in] event      The event object to process.
 * @param[in] components A mask of the keyboard control state components to
 * serialize. State components other than `::XKB_STATE_CONTROLS` are ignored.
 *
 * @pre The event must be of type `::XKB_EVENT_TYPE_COMPONENTS_CHANGE`.
 * Otherwise the result is *undefined*.
 *
 * @returns The corresponding [control mask](@ref xkb_keyboard_control_flags)
 * representing the given components of the *boolean controls* state.
 *
 * @since 1.14.0
 *
 * @memberof xkb_event
 *
 * [global keyboard controls]: @ref xkb_keyboard_control_flags
 */
XKB_EXPORT enum xkb_keyboard_control_flags
xkb_event_serialize_enabled_controls(const struct xkb_event *event,
                                     enum xkb_state_component components);

/**
 * Serialization of the [modifiers](@ref xkb_mod_mask_t)
 * corresponding to a [state event](@ref xkb_event) of type
 * `::XKB_EVENT_TYPE_COMPONENTS_CHANGE` .
 *
 * @param[in] event      The event object to process.
 * @param[in] components A mask of the modifier state components to serialize.
 * State components other than `XKB_STATE_MODS_*` are ignored.
 * If `::XKB_STATE_MODS_EFFECTIVE` is included, all other state components are
 * ignored.
 *
 * @pre The event must be of type `::XKB_EVENT_TYPE_COMPONENTS_CHANGE`.
 * Otherwise the result is *undefined*.
 *
 * @returns The corresponding [modifier mask](@ref xkb_mod_mask_t) representing
 * the given components of the *modifier* state.
 *
 * @since 1.14.0
 *
 * @memberof xkb_event
 */
XKB_EXPORT xkb_mod_mask_t
xkb_event_serialize_mods(const struct xkb_event *event,
                         enum xkb_state_component components);

/**
 * Serialization of the [layout](@ref xkb_layout_index_t)
 * corresponding to a [state event](@ref xkb_event) of type
 * `::XKB_EVENT_TYPE_COMPONENTS_CHANGE` .
 *
 * @param[in] event      The event object to process.
 * @param[in] components A mask of the layout state components to serialize.
 * State components other than `XKB_STATE_LAYOUT_*` are ignored.
 * If `::XKB_STATE_LAYOUT_EFFECTIVE` is included, all other state components are
 * ignored.
 *
 * @pre The event must be of type `::XKB_EVENT_TYPE_COMPONENTS_CHANGE`.
 * Otherwise the result is *undefined*.
 *
 * @returns The corresponding [layout index](@ref xkb_layout_index_t)
 * representing the given components of the *layout* state.
 *
 * @since 1.14.0
 *
 * @memberof xkb_event
 */
XKB_EXPORT xkb_layout_index_t
xkb_event_serialize_layout(const struct xkb_event *event,
                           enum xkb_state_component components);

/**
 * @struct xkb_events
 * Opaque keyboard event collection object.
 *
 * An `xkb_events` batch collects [keyboard events](@ref xkb_event)
 * produced atomically by a single call to an `process_*` function such as
 * `xkb_machine::xkb_machine_process_key()`. Events are consumed
 * sequentially via `xkb_events_next()`. The collection is reset on each
 * `process_*` call.
 *
 * @since 1.14.0
 *
 * @sa `xkb_events_new_batch()`
 * @sa `xkb_events_next()`
 * @sa `xkb_events_destroy()`
 * @sa `xkb_machine::xkb_machine_process_key()`
 * @sa `xkb_machine::xkb_machine_process_synthetic()`
 */
struct xkb_events;

/**
 * @enum xkb_events_flags
 *
 * Flags for `xkb_events::xkb_events_new_batch()`.
 *
 * @since 1.14.0
 */
enum xkb_events_flags {
    /**
     * Do not apply any flags.
     *
     * @since 1.14.0
     */
    XKB_EVENTS_NO_FLAGS = 0
};

/**
 * Create a new [event](@ref xkb_event) batch.
 *
 * @param[in] context The context in which to create the batch.
 * @param[in] flags   Optional flags for the batch, or 0.
 *
 * @returns A new event batch, or `NULL` on failure.
 *
 * @since 1.14.0
 *
 * @sa `xkb_events_destroy()`
 * @sa `xkb_events_next()`
 * @sa `xkb_machine::xkb_machine_process_key()`
 *
 * @memberof xkb_events
 */
XKB_EXPORT struct xkb_events *
xkb_events_new_batch(struct xkb_context *context, enum xkb_events_flags flags);

/**
 * Free an event collection.
 *
 * @param[in] events
 *     The event collection to free.
 *     If it is `NULL`, this function does nothing.
 *
 * @since 1.14.0
 *
 * @sa `xkb_events_new_batch()`
 *
 * @memberof xkb_events
 */
XKB_EXPORT void
xkb_events_destroy(struct xkb_events *events);

/**
 * Get the next event from an event collection.
 *
 * @param[in] events The event collection.
 *
 * @returns The next event, or `NULL` if there are no more events to read.
 *
 * @since 1.14.0
 *
 * @memberof xkb_events
 */
XKB_EXPORT const struct xkb_event *
xkb_events_next(struct xkb_events *events);

/**
 * @struct xkb_machine_builder
 * Opaque builder object to configure an `xkb_machine`.
 *
 * Create with `xkb_machine_builder_new()`, configure with the
 * `xkb_machine_builder_*` functions, then build the state machine with
 * `xkb_machine::xkb_machine_new()`.
 * The builder object may be reused to create multiple `xkb_machine` objects
 * and destroyed when no longer needed. If a single `xkb_machine` object is
 * built, then the builder may be destroyed immediately after
 * `xkb_machine::xkb_machine_new()` returns.
 *
 * @since 1.14.0
 *
 * @sa `xkb_machine_builder::xkb_machine_builder_new()`
 * @sa `xkb_machine::xkb_machine_new()`
 */
struct xkb_machine_builder;

/**
 * @enum xkb_machine_builder_flags
 * Flags for `xkb_machine_builder::xkb_machine_builder_new()`.
 *
 * @since 1.14.0
 */
enum xkb_machine_builder_flags {
    /**
     * Do not apply any flags.
     *
     * @since 1.14.0
     */
    XKB_MACHINE_BUILDER_NO_FLAGS = 0,
};

/**
 * Create a new `xkb_machine` builder object.
 * `xkb_machine` objects can then be created from the builder using
 * `xkb_machine::xkb_machine_new()`.
 *
 * @param[in] keymap  The keymap which the state machine will use.
 * @param[in] flags   Flags to control the builder behavior, or 0.
 *
 * @returns A new `xkb_machine` builder object, or `NULL` on failure.
 *
 * @since 1.14.0
 *
 * @sa `xkb_machine_builder_destroy()`
 * @sa `xkb_machine::xkb_machine_new()`
 *
 * @memberof xkb_machine_builder
 */
XKB_EXPORT struct xkb_machine_builder *
xkb_machine_builder_new(struct xkb_keymap *keymap,
                        enum xkb_machine_builder_flags flags);

/**
 * Free a `xkb_machine` builder object.
 *
 * @param[in] builder The `xkb_machine` builder. If it is `NULL`, this function
 *                    does nothing.
 *
 * @since 1.14.0
 *
 * @sa `xkb_machine_builder_new()`
 *
 * @memberof xkb_machine_builder
 */
XKB_EXPORT void
xkb_machine_builder_destroy(struct xkb_machine_builder *builder);

/**
 * Get the keymap which a `xkb_machine_builder` object is using.
 *
 * @param[in] builder The state machine builder object.
 *
 * @returns The keymap which was passed to `xkb_machine_builder_new()` when
 * creating this `xkb_machine_builder` object.
 *
 * @warning This function does not take a new reference on the keymap; you must
 * explicitly reference it yourself if you plan to use it beyond the
 * lifetime of the `xkb_machine_builder` object.
 *
 * @since 1.14.0
 *
 * @memberof xkb_machine_builder
 */
XKB_EXPORT struct xkb_keymap *
xkb_machine_builder_get_keymap(const struct xkb_machine_builder *builder);

/**
 * @enum xkb_a11y_flags
 * Flags for
 * `xkb_machine_builder::xkb_machine_builder_update_a11y_flags()`.
 *
 * These flags configure the accessibility (*a11y*) features.
 *
 * @since 1.14.0
 */
enum xkb_a11y_flags {
    /**
     * Do not apply any flags.
     *
     * @since 1.14.0
     */
    XKB_A11Y_NO_FLAGS = 0,
    /**
     * If both `::XKB_A11Y_LATCH_TO_LOCK` and
     * `::XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS` are activated, they enable
     * users to [lock] modifier keys without requiring special locking keys.
     * The user can press a [latch] modifier twice in a row to lock it, and
     * then unlock it by pressing it one more time.
     *
     * @sa `::XKB_KEYBOARD_CONTROL_A11Y_STICKY_KEYS`
     * @since 1.14.0
     *
     * [latch]: @ref latched-mod-def
     * [lock]:  @ref locked-mod-def
     */
    XKB_A11Y_LATCH_TO_LOCK = (1 << 0),
    /**
     * Without this option, the [latch] keys are only triggers if keys are
     * strictly *sequentially tapped*, e.g.:
     * 1. `ISO_Level2_Latch` ↓
     * 2. `ISO_Level2_Latch` ↑
     * 3. `A` ↓
     * 4. `A` ↑
     *
     * If one wants *multiple* active latches, they must be tapped in sequence:
     * e.g.:
     * 1. `ISO_Level2_Latch` ↓
     * 2. `ISO_Level2_Latch` ↑
     * 3. `ISO_Level3_Latch` ↓
     * 4. `ISO_Level3_Latch` ↑
     * 5. `A` ↓
     * 6. `A` ↑
     *
     * This option relaxes the strict sequence requirement and enables operating
     * keys that do not break latches *simultaneously* with a [latch] key, e.g.:
     * 1. `ISO_Level2_Latch` ↓
     * 2. `ISO_Level3_Latch` ↓
     * 3. `ISO_Level2_Latch` ↑
     * 4. `ISO_Level3_Latch` ↑
     * 5. `A` ↓
     * 6. `A` ↑
     *
     * This is an extension to the X11 XKB protocol and is enabled by default
     * when using `::XKB_KEYMAP_FORMAT_TEXT_V2`.
     *
     * @since 1.14.0
     *
     * [latch]: @ref latched-mod-def
     */
    XKB_A11Y_LATCH_SIMULTANEOUS_KEYS = (1 << 1),
};

/**
 * Update the accessibility flags of an `xkb_machine_builder` object.
 *
 * @param[in,out] builder The `xkb_machine` builder object to modify.
 * @param[in]     affect  Accessibility flags to modify.
 * @param[in]     flags   Accessibility flags to set or unset.
 *                        Flags in @p affect but not in @p flags are cleared.
 *                        Flags outside @p affect are not changed.
 *
 * @returns `::XKB_SUCCESS` on success, otherwise an error code.
 *
 * @since 1.14.0
 *
 * @memberof xkb_machine_builder
 */
XKB_EXPORT enum xkb_error_code
xkb_machine_builder_update_a11y_flags(
    struct xkb_machine_builder *builder,
    enum xkb_a11y_flags affect,
    enum xkb_a11y_flags flags
);

/**
 * Remap a modifier combination, e.g. to make `Control+Alt` act as
 * `LevelThree` (`AltGr`). This helps improve *compatibility* across platforms.
 *
 * The remapping takes effect only using
 * `xkb_machine::xkb_machine_process_key()` and under certain
 * conditions:
 *
 * - The corresponding *effective* modifiers are active.
 * - The key being processed has a type that does *not* use any of the *source*
 *   modifiers.
 * - There is no other remapping entry with the source modifiers being a
 *   superset of this entry. E.g. `Control+Alt` has priority over `Control`.
 *
 * @param[in,out] builder The `xkb_machine` builder object to modify.
 * @param[in]     source  Modifier combination to remap, using their [encoding].
 *                        Must be non-zero, unless both @p source and @p target
 *                        are 0 to clear all entries.
 * @param[in]     target  Modifier combination to remap to, using their
 *                        [encoding], or 0 to remove the entry for @p source.
 *                        If both @p source and @p target are 0, all entries are
 *                        cleared.
 *
 * @returns `::XKB_SUCCESS` on success, otherwise an error code.
 *
 * Example:
 *
 * ```c
 * struct xkb_keymap *keymap = xkb_machine_builder_get_keymap(builder);
 * // Remap Control+Alt to LevelThree (AltGr)
 * const xkb_mod_mask_t ctrl = xkb_keymap_mod_get_mask(keymap, XKB_MOD_NAME_CTRL);
 * const xkb_mod_mask_t alt = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_ALT);
 * const xkb_mod_mask_t level3 = xkb_keymap_mod_get_mask(keymap, XKB_VMOD_NAME_LEVEL3);
 * if (xkb_machine_builder_remap_mods(builder, ctrl | alt, level3)) {
 *     // handle error
 *     …
 * }
 * ```
 *
 * @since 1.14.0
 *
 * @memberof xkb_machine_builder
 *
 * [encoding]: @ref modifiers-encoding
 */
XKB_EXPORT enum xkb_error_code
xkb_machine_builder_remap_mods(
    struct xkb_machine_builder *builder,
    xkb_mod_mask_t source,
    xkb_mod_mask_t target
);

/**
 * Set the modifiers that trigger the keyboard shortcut overrides.
 *
 * When any of the specified modifiers is active, the effective layout
 * is substituted according to the mapping set by
 * `xkb_machine_builder_remap_shortcut_layout()`.
 * This ensures a consistent user experience with keyboard shortcuts
 * across the layouts.
 *
 * @param[in,out] builder The `xkb_machine` builder object to modify.
 * @param[in]     affect  Modifiers to consider, using their [encoding].
 * @param[in]     mask    Modifiers to set or unset, using their [encoding].
 *                        Modifiers in @p affect but not in @p mask are cleared.
 *                        Modifiers outside @p affect are not changed.
 *
 * @returns `::XKB_SUCCESS` on success, otherwise an error code.
 *
 * @sa `xkb_machine_builder_remap_shortcut_layout()`
 * @sa `xkb_keymap::xkb_keymap_mod_get_mask2()`
 * @since 1.14.0
 * @memberof xkb_machine_builder
 *
 * [encoding]: @ref modifiers-encoding
 */
XKB_EXPORT enum xkb_error_code
xkb_machine_builder_update_shortcut_mods(struct xkb_machine_builder *builder,
                                         xkb_mod_mask_t affect,
                                         xkb_mod_mask_t mask);

/**
 * Set a layout substitution for the shortcut layout override.
 *
 * When any modifier set via `xkb_machine_builder_update_shortcut_mods()` is
 * active, the effective layout @p source is substituted with layout @p target
 * in key processing. This allows shortcuts defined in layout @p target
 * (typically a Latin layout) to remain reachable when layout @p source is
 * active.
 *
 * @param[in,out] builder The `xkb_machine` builder object to modify.
 * @param[in]     source  Source layout to substitute.
 * @param[in]     target  Target layout to use instead of @p source.
 *
 * @returns `::XKB_SUCCESS` on success, otherwise an error code.
 *
 * @since 1.14.0
 * @sa `xkb_machine_builder_update_shortcut_mods()`
 * @memberof xkb_machine_builder
 */
XKB_EXPORT enum xkb_error_code
xkb_machine_builder_remap_shortcut_layout(struct xkb_machine_builder *builder,
                                          xkb_layout_index_t source,
                                          xkb_layout_index_t target);

/**
 * Create a new keyboard state machine object.
 *
 * This entry point is intended for *server* applications; *client* applications
 * should not run a state machine locally: instead they should use the
 * `xkb_state` API and process server state update using
 * `xkb_state::xkb_state_update_mask()`.
 * See @ref server-client-state for further information.
 *
 * @param[in] builder The [builder](@ref xkb_machine_builder) object from which
 *                    to create the state machine.
 *
 * @returns A new keyboard state machine object, or `NULL` on failure.
 *
 * @since 1.14.0
 *
 * @sa `xkb_machine_builder::xkb_machine_builder_new()`
 *
 * @memberof xkb_machine
 */
XKB_EXPORT struct xkb_machine *
xkb_machine_new(const struct xkb_machine_builder *builder);

/**
 * Take a new reference on a `xkb_machine` object.
 *
 * @param[in] machine The state machine.
 *
 * @returns The passed in object.
 *
 * @since 1.14.0
 *
 * @memberof xkb_machine
 */
XKB_EXPORT struct xkb_machine *
xkb_machine_ref(struct xkb_machine *machine);

/**
 * Release a reference on a `xkb_machine` object, and possibly free it.
 *
 * @param[in] machine The state machine.  If it is `NULL`, this function does nothing.
 *
 * @since 1.14.0
 *
 * @memberof xkb_machine
 */
XKB_EXPORT void
xkb_machine_unref(struct xkb_machine *machine);

/**
 * Get the keymap which a `xkb_machine` object is using.
 *
 * @param[in] machine The state machine.
 *
 * @returns The keymap which was used to create the `xkb_machine` object, i.e.
 * the keymap passed to `xkb_machine_builder::xkb_machine_builder_new()` when
 * creating the corresponding [builder](@ref xkb_machine_builder) used in
 * `xkb_machine_new()`.
 *
 * @warning This function does not take a new reference on the keymap; you must
 * explicitly reference it yourself if you plan to use it beyond the
 * lifetime of the `xkb_machine` object.
 *
 * @since 1.14.0
 *
 * @memberof xkb_machine
 */
XKB_EXPORT struct xkb_keymap *
xkb_machine_get_keymap(const struct xkb_machine *machine);

/**
 * @enum xkb_key_direction
 * Specifies the direction of the key (press / release) or a repetition.
 */
enum xkb_key_direction {
    /** The key was *released*. */
    XKB_KEY_UP,
    /** The key was *pressed*. */
    XKB_KEY_DOWN,
    /**
     * The key was *repeated*.
     *
     * This should be used by the compositor only if it handles key repetition
     * itself.
     *
     * @since 1.14.0
     */
    XKB_KEY_REPEATED
};

/**
 * Process a key event – a pair ([keycode], [direction]) – through the XKB
 * [state machine], and collect the resulting [keyboard events] into an
 * [event batch].
 *
 * The produced events form a single *frame*.
 *
 * Use this function for in-band (device) inputs.
 * Use `xkb_machine_process_synthetic()` instead to update the state machine in
 * response to out-of-band (non-device) inputs, such as UI layout switchers or
 * accessibility settings changes.
 *
 * A series of calls to this function should be consistent; that is, a call
 * with `::XKB_KEY_DOWN` for a key should be matched by an `::XKB_KEY_UP`; if a
 * key is pressed twice, it should be released twice; etc. Otherwise (e.g. due
 * to missed input events), situations like “stuck modifiers” may occur.
 *
 * @param[in,out] machine    The XKB [state machine] object.
 * @param[in]     key        The keycode of the key being operated.
 * @param[in]     direction  The direction of the key operation.
 * @param[out]    events     The event batch to collect events into. It will be
 *                           reset before collecting.
 *
 * @returns `::XKB_SUCCESS` on success, otherwise an error code.
 *
 * @since 1.14.0
 *
 * @sa `xkb_machine_process_synthetic()`
 *
 * @memberof xkb_machine
 *
 * [keycode]: @ref xkb_keycode_t
 * [direction]: @ref xkb_key_direction
 * [state machine]: @ref xkb_machine
 * [keyboard events]: @ref xkb_event
 * [event batch]: @ref xkb_events
 */
XKB_EXPORT enum xkb_error_code
xkb_machine_process_key(struct xkb_machine *machine,
                        xkb_keycode_t key, enum xkb_key_direction direction,
                        struct xkb_events *events);

/**
 * @struct xkb_state_components_update
 * Latched and locked state components for an out-of-band state update.
 *
 * Carries the modifier, layout and boolean controls assignments for
 * `xkb_state_update`.
 * Used to update latched and locked modifiers and layouts atomically via
 * `xkb_machine::xkb_machine_process_synthetic()`.
 *
 * Which fields are considered is determined by `components`:
 * - `::XKB_STATE_MODS_LATCHED`   → `affect_latched_mods`, `latched_mods`
 * - `::XKB_STATE_MODS_LOCKED`    → `affect_locked_mods`, `locked_mods`
 * - `::XKB_STATE_LAYOUT_LATCHED` → `latched_layout`
 * - `::XKB_STATE_LAYOUT_LOCKED`  → `locked_layout`
 * - `::XKB_STATE_CONTROLS`       → `affect_controls`, `controls`
 *
 * @since 1.14.0
 *
 * @sa `xkb_state_update`
 */
struct xkb_state_components_update {
    /**
     * Size of this structure, for forward-compatibility.
     *
     * Must be set to `sizeof(struct xkb_state_components_update)`.
     * If the caller’s size exceeds the library’s (e.g. version mismatch),
     * an error is returned unless all extra bytes are zero.
     */
    size_t size;
    /**
     * Mask of [state components](@ref xkb_state_component) to update.
     *
     * The following components are meaningful:
     * - `::XKB_STATE_MODS_LATCHED`
     * - `::XKB_STATE_MODS_LOCKED`
     * - `::XKB_STATE_LAYOUT_LATCHED`
     * - `::XKB_STATE_LAYOUT_LOCKED`
     * - `::XKB_STATE_CONTROLS`
     *
     * Other components are ignored.
     */
    enum xkb_state_component components;
    /**
     * Mask of latched modifiers to affect.
     *
     * Only modifiers present in this mask are considered when updating
     * `latched_mods`. Only considered if `::XKB_STATE_MODS_LATCHED` is
     * set in `components`.
     */
    xkb_mod_mask_t affect_latched_mods;
    /**
     * Modifiers to set as latched or unlatched.
     *
     * Only modifiers in `affect_latched_mods` are considered. Only
     * considered if `::XKB_STATE_MODS_LATCHED` is set in `components`.
     */
    xkb_mod_mask_t latched_mods;
    /**
     * Mask of locked modifiers to affect.
     *
     * Only modifiers present in this mask are considered when updating
     * `locked_mods`. Only considered if `::XKB_STATE_MODS_LOCKED` is
     * set in `components`.
     */
    xkb_mod_mask_t affect_locked_mods;
    /**
     * Modifiers to set as locked or unlocked.
     *
     * Only modifiers in `affect_locked_mods` are considered. Only
     * considered if `::XKB_STATE_MODS_LOCKED` is set in `components`.
     */
    xkb_mod_mask_t locked_mods;
    /**
     * Layout to latch.
     *
     * May be out of range (including negative); the layout is brought into
     * range according to the current out-of-range layout policy. Only
     * considered if `::XKB_STATE_LAYOUT_LATCHED` is set in `components`.
     *
     * @sa xkb_layout_index_t
     */
    int32_t latched_layout;
    /**
     * Layout to lock.
     *
     * May be out of range (including negative); the layout is brought into
     * range according to the current out-of-range layout policy. Only
     * considered if `::XKB_STATE_LAYOUT_LOCKED` is set in `components`.
     *
     * @sa xkb_layout_index_t
     */
    int32_t locked_layout;
    /**
     * Boolean keyboard controls to affect.
     *
     * Only controls present in this mask are considered when updating
     * `controls`. Only considered if `::XKB_STATE_CONTROLS` is set in
     * `components`.
     *
     * @sa `xkb_keyboard_control_flags`
     */
    enum xkb_keyboard_control_flags affect_controls;
    /**
     * Boolean keyboard controls to enable or disable.
     *
     * Only controls in `affect_controls` are considered. Only considered
     * if `::XKB_STATE_CONTROLS` is set in `components`.
     *
     * @sa `xkb_keyboard_control_flags`
     */
    enum xkb_keyboard_control_flags controls;
};

/**
 * @enum xkb_layout_out_of_range_policy
 * Policies defining how to bring out-of-range layout indices into range.
 *
 * @since 1.14.0
 */
enum xkb_layout_out_of_range_policy {
    /**
     * Wrap into range using integer modulus (default).
     *
     * @since 1.14.0
     */
    XKB_LAYOUT_OUT_OF_RANGE_WRAP = 0,
    /**
     * Clamp into range, i.e. invalid indices are corrected to the closest
     * valid bound (0 or highest layout index).
     *
     * @since 1.14.0
     */
    XKB_LAYOUT_OUT_OF_RANGE_CLAMP,
    /**
     * Redirect to a specific [layout index](@ref xkb_layout_index_t).
     *
     * @since 1.14.0
     */
    XKB_LAYOUT_OUT_OF_RANGE_REDIRECT
};

/**
 * @struct xkb_layout_policy_update
 * Configures the policy used to bring out-of-range layout indices into range.
 *
 * If `policy` is `::XKB_LAYOUT_OUT_OF_RANGE_REDIRECT`, `redirect` specifies
 * the target layout index; otherwise `redirect` is ignored.
 *
 * @since 1.14.0
 *
 * @sa `xkb_layout_out_of_range_policy`
 * @sa `xkb_state_update::layout_policy`
 */
struct xkb_layout_policy_update {
    /**
     * Size of this structure, for forward-compatibility.
     *
     * Must be set to `sizeof(struct xkb_layout_policy_update)`.
     * If the caller’s size exceeds the library’s (e.g. version mismatch),
     * an error is returned unless all extra bytes are zero.
     */
    size_t size;
    /**
     * Policy to use to handle out-of-range layout indices.
     *
     * @sa `xkb_layout_out_of_range_policy`
     */
    enum xkb_layout_out_of_range_policy policy;
    /**
     * Layout index to redirect to when `policy` is
     * `::XKB_LAYOUT_OUT_OF_RANGE_REDIRECT`. Ignored otherwise.
     */
    xkb_layout_index_t redirect;
};

/**
 * @struct xkb_state_update
 * Request to process an out-of-band atomic update through an `xkb_machine` or
 * `xkb_state`.
 *
 * Used with `xkb_state::xkb_state_update_synthetic()` and
 * `xkb_machine::xkb_machine_process_synthetic()` to atomically
 * apply any combination of:
 * - Latched and locked modifier and layout changes (via `components`)
 * - Boolean keyboard control changes (via `components`)
 * - Parameterized keyboard control changes (via `layout_policy`)
 *
 * A `NULL` pointer means “not set / no change”.
 *
 * @since 1.14.0
 *
 * @sa `xkb_state::xkb_state_update_synthetic()`
 * @sa `xkb_machine::xkb_machine_process_synthetic()`
 * @sa `xkb_state_components_update`
 * @sa `xkb_layout_policy_update`
 */
struct xkb_state_update {
    /**
     * Size of this structure, for forward-compatibility.
     *
     * Must be set to `sizeof(struct xkb_state_update)`.
     * If the caller’s size exceeds the library’s (e.g. version mismatch),
     * an error is returned unless all extra bytes are zero.
     */
    size_t size;
    /**
     * Components updates, or `NULL` for no change.
     *
     * @sa `xkb_state_component`
     */
    const struct xkb_state_components_update *components;
    /**
     * Out-of-range layout policy update, or `NULL` for no change.
     *
     * @sa `xkb_layout_out_of_range_policy`
     */
    const struct xkb_layout_policy_update *layout_policy;
};

/**
 * Process a *synthetic* (out-of-band) atomic update through the XKB
 * [state machine], and collect the resulting [keyboard events] into an
 * [event batch].
 *
 * Use this function to update the state machine in response to
 * out-of-band (non-device) inputs, such as UI layout switchers or
 * accessibility settings changes.
 * Use `xkb_machine_process_key()` instead for in-band (device) inputs.
 *
 * All changes specified in @p update are applied atomically as a single
 * *frame*: the resulting events reflect the **net** state change at the
 * end of the frame, not intermediate steps. In particular, a
 * `::XKB_EVENT_TYPE_COMPONENTS_CHANGE` event in the batch represents the
 * cumulative state change for the entire frame — individual intermediate
 * state transitions are not observable.
 *
 * Only latched, locked and control components can be updated out-of-band;
 * depressed components can only change through key presses via
 * `xkb_machine_process_key()`.
 *
 * @par Layout out of range
 * @parblock
 * If the effective layout, after taking into account the depressed, latched
 * and locked layout, is out of range (negative or greater than the maximum
 * layout index), it is brought into range according to the current
 * out-of-range layout policy (see `xkb_layout_out_of_range_policy`).
 * @endparblock
 *
 * @param[in,out] machine The XKB [state machine] object.
 * @param[in]     update  The update to apply.
 *                        Must have `xkb_state_update::size` set.
 * @param[out]    events  The event batch to collect events into. It will be
 *                        reset before collecting.
 *
 * @returns `::XKB_SUCCESS` on success, otherwise an error code.
 *
 * @since 1.14.0
 *
 * @sa `xkb_state_update`
 * @sa `xkb_machine_process_key()`
 * @memberof xkb_machine
 *
 * [state machine]: @ref xkb_machine
 * [keyboard events]: @ref xkb_event
 * [event batch]: @ref xkb_events
 */
XKB_EXPORT enum xkb_error_code
xkb_machine_process_synthetic(struct xkb_machine *machine,
                              const struct xkb_state_update *update,
                              struct xkb_events *events);

/**
 * @enum xkb_state_mode
 * Mode for creating a [keyboard state object](@ref xkb_state).
 *
 * @since 1.14.0
 * @sa `xkb_state::xkb_state_new_with_mode()`
 */
enum xkb_state_mode {
    /**
     * State driven by **serialized state updates** via
     * `xkb_state::xkb_state_update_mask()`.
     *
     * Use this mode for *client* applications.
     *
     * This is the *recommended* mode for new client applications, as it creates
     * `xkb_state` objects with much *smaller* memory footprint than with
     * `xkb_state::xkb_state_new()`.
     *
     * @important `xkb_state` objects created with this mode cannot be used
     * with the following API:
     * - `xkb_state::xkb_state_update_event()`
     * - `xkb_state::xkb_state_update_key()`
     * - `xkb_state::xkb_state_update_synthetic()`
     * - `xkb_state::xkb_state_update_latched_locked()` *(deprecated)*
     *
     * @since 1.14.0
     */
    XKB_STATE_MODE_CLIENT = 0,
    /**
     * State driven by <strong>[XKB events]</strong> via
     * `xkb_state::xkb_state_update_event()`.
     *
     * Use this mode for an observable state companion to an `xkb_machine` in
     * *server* applications using the `xkb_machine` API.
     *
     * This is the *recommended* mode for new server applications, as it creates
     * `xkb_state` objects with much *smaller* memory footprint than with
     * `xkb_state::xkb_state_new()`.
     *
     * @important `xkb_state` objects created with this mode cannot be used
     * with the following API:
     * - `xkb_state::xkb_state_update_mask()`
     * - `xkb_state::xkb_state_update_key()`
     * - `xkb_state::xkb_state_update_synthetic()`
     * - `xkb_state::xkb_state_update_latched_locked()` *(deprecated)*
     *
     * @since 1.14.0
     *
     * [XKB events]: @ref xkb_event
     */
    XKB_STATE_MODE_SERVER_QUERY = 1,
    /**
     * State driven directly by **key events**, via
     * `xkb_state::xkb_state_update_key()` or by *synthetic input events*, via
     * `xkb_state::xkb_state_update_synthetic()`.
     *
     * Use this mode for *server* applications that do not use the preferred
     * full-featured `xkb_machine` API.
     *
     * @important Contrary to `xkb_state::xkb_state_new()`, `xkb_state` objects
     * created with this mode cannot be used with the following API:
     * - `xkb_state::xkb_state_update_mask()`
     * - `xkb_state::xkb_state_update_event()`
     *
     * @warning Prefer `xkb_machine` for new server applications.
     * This mode exists for *compatibility* with code predating the
     * `xkb_machine` API and may be deprecated in a future release.
     *
     * @since 1.14.0
     */
    XKB_STATE_MODE_SERVER = 2,
};

/**
 * Create a new keyboard state object with an explicit mode.
 *
 * This entry point is intended for both server and client applications.
 * It enables using the optimal implementation for the intended use.
 *
 * @param[in] keymap The keymap which the state will use.
 * @param[in] mode   The [state mode](@ref xkb_state_mode) to use.
 *
 * @returns A new keyboard state object, or `NULL` on failure.
 *
 * @since 1.14.0
 * @sa `xkb_state_mode`
 * @memberof xkb_state
 */
XKB_EXPORT struct xkb_state *
xkb_state_new_with_mode(struct xkb_keymap *keymap, enum xkb_state_mode mode);

/**
 * Create a new keyboard state object.
 *
 * @note This is the legacy constructor, predating the `xkb_machine` API.
 * It imposes no restrictions on which update functions may be called,
 * making it easy to accidentally mix incompatible update paths. Prefer
 * `xkb_state::xkb_state_new_with_mode()` for new code, which enforces
 * correct API usage at runtime and optimal performance.
 *
 * @param[in] keymap The keymap which the state will use.
 *
 * @returns A new keyboard state object, or `NULL` on failure.
 *
 * @memberof xkb_state
 */
XKB_EXPORT struct xkb_state *
xkb_state_new(struct xkb_keymap *keymap);

/**
 * Take a new reference on a keyboard state object.
 *
 * @param[in] state The [state](@ref xkb_state) to reference.
 *
 * @returns The passed-in object.
 *
 * @memberof xkb_state
 */
XKB_EXPORT struct xkb_state *
xkb_state_ref(struct xkb_state *state);

/**
 * Release a reference on a keyboard state object, and possibly free it.
 *
 * @param[in] state The state.  If it is `NULL`, this function does nothing.
 *
 * @memberof xkb_state
 */
XKB_EXPORT void
xkb_state_unref(struct xkb_state *state);

/**
 * Get the keymap which a keyboard state object is using.
 *
 * @param[in] state  The keyboard state object.
 *
 * @returns The keymap which was passed to `xkb_state_new()` or
 * `xkb_state_new_with_mode()` when creating this state object.
 *
 * @warning This function does not take a new reference on the keymap; you must
 * explicitly reference it yourself if you plan to use it beyond the
 * lifetime of the state.
 *
 * @memberof xkb_state
 */
XKB_EXPORT struct xkb_keymap *
xkb_state_get_keymap(struct xkb_state *state);

/**
 * Update a keyboard state from a set of explicit masks.
 *
 * This entry point is intended for *client* applications; see @ref
 * server-client-state for details. *Server* applications should use
 * either the recommended modern `xkb_machine` API with the corresponding
 * `xkb_state_update_event()` or the legacy `xkb_state_update_synthetic()`
 * API instead.
 *
 * @param[in,out] state            The keyboard state object.
 * @param[in]     depressed_mods   Modifiers to set as depressed.
 * @param[in]     latched_mods     Modifiers to set as latched.
 * @param[in]     locked_mods      Modifiers to set as locked.
 * @param[in]     depressed_layout Layout to set as depressed.
 * @param[in]     latched_layout   Layout to set as latched.
 * @param[in]     locked_layout    Layout to set as locked.
 *
 * @pre *All* parameters must represent a *consistent* snapshot of the server
 * keyboard state at a single point in time.  In case the server provides only a
 * single layout state (e.g. the effective layout in the Wayland protocol), it
 * should be used for @p locked_layout, while @p depressed_layout and
 * @p latched_layout are both set to `0`.
 *
 * @important If @p state was not created with `::XKB_STATE_MODE_CLIENT` or
 * `xkb_state_new()`, the call is *rejected* without updating the state,
 * and the misuse is logged as `::XKB_ERROR_UNEXPECTED_STATE_MODE`.
 * The return value is `0` in this case, which is indistinguishable from
 * a no-op update.
 *
 * @returns A mask of state components that have changed as a result of
 * the update.  If nothing in the state has changed, returns 0.
 *
 * @sa `xkb_state_component`
 * @sa `xkb_state_update_synthetic()`
 * @sa `xkb_state_update_event()`
 *
 * @memberof xkb_state
 */
XKB_EXPORT enum xkb_state_component
xkb_state_update_mask(struct xkb_state *state,
                      xkb_mod_mask_t depressed_mods,
                      xkb_mod_mask_t latched_mods,
                      xkb_mod_mask_t locked_mods,
                      xkb_layout_index_t depressed_layout,
                      xkb_layout_index_t latched_layout,
                      xkb_layout_index_t locked_layout);

/**
 * Update the keyboard state [components](@ref xkb_state_component) from an
 * [event](@ref xkb_event).
 *
 * This entry point is intended for *server* applications and should not be used
 * by *client* applications; see @ref server-client-state for details.
 *
 * It enables server applications to use `xkb_state` as the observable state
 * companion to an `xkb_machine`: feed each event produced by
 * `xkb_machine::xkb_machine_process_key()` or
 * `xkb_machine::xkb_machine_process_synthetic()` into this
 * function to keep the observable state in sync.
 *
 * @param[in,out] state The keyboard state object.
 * @param[in]     event The state event to update from.
 *
 * @important If @p state was not created with `::XKB_STATE_MODE_SERVER_QUERY`
 * or `xkb_state_new()`, the call is *rejected* without updating the state,
 * and the misuse is logged as `::XKB_ERROR_UNEXPECTED_STATE_MODE`.
 * The return value is `0` in this case, which is indistinguishable from
 * a no-op update.
 *
 * @returns A mask of state components that have changed as a result of
 * the update.  If nothing in the state has changed, returns 0.
 *
 * @since 1.14.0
 *
 * @memberof xkb_state
 */
XKB_EXPORT enum xkb_state_component
xkb_state_update_event(struct xkb_state *state,
                       const struct xkb_event *event);

/**
 * Update the keyboard state to reflect a given key being pressed or
 * released.
 *
 * This entry point is intended for *server* applications and should not be used
 * by *client* applications; see @ref server-client-state for details.
 *
 * A series of calls to this function should be consistent; that is, a call
 * with `::XKB_KEY_DOWN` for a key should be matched by an `::XKB_KEY_UP`; if a
 * key is pressed twice, it should be released twice; etc. Otherwise (e.g. due
 * to missed input events), situations like “stuck modifiers” may occur.
 *
 * This function is often used in conjunction with the function
 * `xkb_state_key_get_syms()` (or `xkb_state_key_get_one_sym()`), for example,
 * when handling a key event.  In this case, you should prefer to get the
 * keysyms *before* updating the key, such that the keysyms reported for
 * the key event are not affected by the event itself.  This is the
 * conventional behavior.
 *
 * @note This is the legacy server entry point and only supports a restricted
 * set of libxkbcommon features.  Since 1.14.0, prefer `xkb_machine` for new
 * server applications to enable the full feature set.
 *
 * @param[in,out] state     The keyboard state object.
 * @param[in]     key       The key being operated.
 * @param[in]     direction The direction of the key operation.
 *
 * @important If @p state was not created with `::XKB_STATE_MODE_SERVER` or
 * `xkb_state_new()`, the call is *rejected* without updating the state,
 * and the misuse is logged as `::XKB_ERROR_UNEXPECTED_STATE_MODE`.
 * The return value is `0` in this case, which is indistinguishable from
 * a no-op update.
 *
 * @returns A mask of state components that have changed as a result of
 * the update.  If nothing in the state has changed, returns 0.
 *
 * @memberof xkb_state
 *
 * @sa `xkb_state_update_mask()`
 */
XKB_EXPORT enum xkb_state_component
xkb_state_update_key(struct xkb_state *state, xkb_keycode_t key,
                     enum xkb_key_direction direction);

/**
 * Apply a *synthetic* (out-of-band) atomic update to the keyboard state.
 *
 * This entry point is intended for *server* applications and should not be used
 * by *client* applications; see @ref server-client-state for details.
 *
 * - Use this function to update the keyboard state in response to
 *   out-of-band (non-device) inputs, such as UI layout switchers or
 *   accessibility settings changes.
 * - Use `xkb_state_update_key()` instead for in-band (device) inputs.
 * - Use `xkb_state_update_event()` instead when updating from an
 *   [event](@ref xkb_event) produced by `xkb_machine`.
 *
 * Only latched, locked and control components can be updated out-of-band;
 * depressed components can only change through key presses via
 * `xkb_state_update_key()`.
 *
 * @par Layout out of range
 * @parblock
 * If the effective layout, after taking into account the depressed, latched
 * and locked layout, is out of range (negative or greater than the maximum
 * layout index), it is brought into range according to the current
 * out-of-range layout policy (see `xkb_layout_out_of_range_policy`).
 * @endparblock
 *
 * @note This entry point serves the legacy server use case and only supports a
 * restricted set of libxkbcommon features.  Since 1.14.0, prefer `xkb_machine`
 * for new server applications to enable the full feature set.
 *
 * @param[in,out] state   The keyboard state object.
 * @param[in]     update  The update to apply.
 *                        Must have `xkb_state_update::size` set.
 * @param[out]    changed A pointer to store the mask of state components that
 *                        have changed as a result of the update, or `NULL` to
 *                        ignore. Set to 0 if nothing in the state has changed.
 *
 * @returns
 * - `::XKB_SUCCESS` on success;
 * - `::XKB_ERROR_UNEXPECTED_STATE_MODE` without updating the state if @p state
 *   was not created with `::XKB_STATE_MODE_SERVER` or `xkb_state_new()`.
 * - Otherwise another [error code](@ref xkb_error_code).
 *
 * @note This function returns an error code rather than a state component
 * delta (unlike the other `xkb_state_update_*` functions), in order to align
 * with the `xkb_machine::xkb_machine_process_synthetic()` API. The delta
 * is optionally available via the @p changed parameter.
 *
 * @since 1.14.0
 *
 * @sa `xkb_state_update`
 * @sa `xkb_state_update_key()`
 * @sa `xkb_machine::xkb_machine_process_synthetic()`
 * @memberof xkb_state
 */
XKB_EXPORT enum xkb_error_code
xkb_state_update_synthetic(struct xkb_state *state,
                           const struct xkb_state_update *update,
                           enum xkb_state_component *changed);

/**
 * Update the keyboard state to change the latched and locked state of
 * the modifiers and layout.
 *
 * @deprecated Use `xkb_state_update_synthetic()` instead.
 *
 * This entry point is intended for *server* applications and should not be used
 * by *client* applications; see @ref server-client-state for details.
 *
 * Use this function to update the latched and locked state according to
 * out-of-band (non-device) inputs, such as UI layout switchers.
 *
 * @par Layout out of range
 * @parblock
 * If the effective layout, after taking into account the depressed, latched and
 * locked layout, is out of range (negative or greater than the maximum layout),
 * it is brought into range. Currently, the layout is wrapped using integer
 * modulus (with negative values wrapping from the end). The wrapping behavior
 * can be configured using `xkb_state_update_synthetic()`.
 * @endparblock
 *
 * @param[in,out] state The keyboard state object.
 * @param[in]     affect_latched_mods See @p latched_mods.
 * @param[in]     latched_mods
 *     Modifiers to set as latched or unlatched. Only modifiers in
 *     @p affect_latched_mods are considered.
 * @param[in]     affect_latched_layout See @p latched_layout.
 * @param[in]     latched_layout
 *     Layout to latch. Only considered if @p affect_latched_layout is `true`.
 *     May be out of range (including negative) -- see note above.
 * @param[in]     affect_locked_mods See @p locked_mods.
 * @param[in]     locked_mods
 *     Modifiers to set as locked or unlocked. Only modifiers in
 *     @p affect_locked_mods are considered.
 * @param[in]     affect_locked_layout See @p locked_layout.
 * @param[in]     locked_layout
 *     Layout to lock. Only considered if @p affect_locked_layout is `true`.
 *     May be out of range (including negative) -- see note above.
 *
 * @important If @p state was not created with `::XKB_STATE_MODE_SERVER` or
 * `xkb_state_new()`, the call is *rejected* without updating the state,
 * and the misuse is logged as `::XKB_ERROR_UNEXPECTED_STATE_MODE`.
 * The return value is `0` in this case, which is indistinguishable from
 * a no-op update.
 *
 * @returns A mask of state components that have changed as a result of
 * the update.  If nothing in the state has changed, returns 0.
 *
 * @memberof xkb_state
 *
 * @sa `xkb_state_update_synthetic()`
 */
XKB_EXPORT enum xkb_state_component
xkb_state_update_latched_locked(struct xkb_state *state,
                                xkb_mod_mask_t affect_latched_mods,
                                xkb_mod_mask_t latched_mods,
                                bool affect_latched_layout,
                                int32_t latched_layout,
                                xkb_mod_mask_t affect_locked_mods,
                                xkb_mod_mask_t locked_mods,
                                bool affect_locked_layout,
                                int32_t locked_layout);

/**
 * Get the keysyms obtained from pressing a particular key in a given
 * keyboard state.
 *
 * Get the keysyms for a key according to the current active layout,
 * modifiers and shift level for the key, as determined by a keyboard
 * state.
 *
 * @param[in]  state    The keyboard state object.
 * @param[in]  key      The keycode of the key.
 * @param[out] syms_out An immutable array of keysyms corresponding the
 * key in the given keyboard state.
 *
 * As an extension to XKB, this function can return more than one keysym.
 * If you do not want to handle this case, you can use
 * `xkb_state_key_get_one_sym()` for a simpler interface.
 *
 * @returns The number of keysyms in the syms_out array.  If no keysyms
 * are produced by the key in the given keyboard state, returns 0 and sets
 * syms_out to `NULL`.
 *
 * This function performs Capitalization @ref keysym-transformations.
 *
 * @memberof xkb_state
 *
 * @since 1.9.0 This function now performs @ref keysym-transformations.
 */
XKB_EXPORT int
xkb_state_key_get_syms(struct xkb_state *state, xkb_keycode_t key,
                       const xkb_keysym_t **syms_out);

/**
 * Get the Unicode/UTF-8 string obtained from pressing a particular key
 * in a given keyboard state.
 *
 * @param[in]  state  The keyboard state object.
 * @param[in]  key    The keycode of the key.
 * @param[out] buffer A buffer to write the string into.
 * @param[in]  size   Capacity of the buffer.
 *
 * @warning If the buffer passed is too small, the string is truncated
 * (though still `NULL`-terminated).
 *
 * @returns The number of bytes required for the string, excluding the
 * `NULL` byte.  If there is nothing to write, returns 0.
 *
 * You may check if truncation has occurred by comparing the return value
 * with the size of @p buffer, similarly to the `snprintf(3)` function.
 * You may safely pass `NULL` and 0 to @p buffer and @p size to find the
 * required size (without the `NULL`-byte).
 *
 * This function performs Capitalization and Control @ref
 * keysym-transformations.
 *
 * @memberof xkb_state
 * @since 0.4.1
 */
XKB_EXPORT int
xkb_state_key_get_utf8(struct xkb_state *state, xkb_keycode_t key,
                       char *buffer, size_t size);

/**
 * Get the Unicode/UTF-32 codepoint obtained from pressing a particular
 * key in a a given keyboard state.
 *
 * @param[in]  state  The keyboard state object.
 * @param[in]  key    The keycode of the key.
 *
 * @returns The UTF-32 representation for the key, if it consists of only
 * a single codepoint.  Otherwise, returns 0.
 *
 * This function performs Capitalization and Control @ref
 * keysym-transformations.
 *
 * @memberof xkb_state
 * @since 0.4.1
 */
XKB_EXPORT uint32_t
xkb_state_key_get_utf32(struct xkb_state *state, xkb_keycode_t key);

/**
 * Get the single keysym obtained from pressing a particular key in a
 * given keyboard state.
 *
 * This function is similar to `xkb_state_key_get_syms()`, but intended
 * for users which cannot or do not want to handle the case where
 * multiple keysyms are returned (in which case this function is
 * preferred).
 *
 * @param[in]  state  The keyboard state object.
 * @param[in]  key    The keycode of the key.
 *
 * @returns The keysym.  If the key does not have exactly one keysym,
 * returns `XKB_KEY_NoSymbol`.
 *
 * This function performs Capitalization @ref keysym-transformations.
 *
 * @sa xkb_state_key_get_syms()
 * @memberof xkb_state
 */
XKB_EXPORT xkb_keysym_t
xkb_state_key_get_one_sym(struct xkb_state *state, xkb_keycode_t key);

/**
 * Get the effective layout index for a key in a given keyboard state.
 *
 * @param[in]  state  The keyboard state object.
 * @param[in]  key    The keycode of the key.
 *
 * @returns The layout index for the key in the given keyboard state.  If
 * the given keycode is invalid, or if the key is not included in any
 * layout at all, returns `::XKB_LAYOUT_INVALID`.
 *
 * @invariant If the returned layout is valid, the following always holds:
 * @code
 * xkb_state_key_get_layout(state, key) < xkb_keymap_num_layouts_for_key(keymap, key)
 * @endcode
 *
 * @memberof xkb_state
 */
XKB_EXPORT xkb_layout_index_t
xkb_state_key_get_layout(struct xkb_state *state, xkb_keycode_t key);

/**
 * Get the effective shift level for a key in a given keyboard state and
 * layout.
 *
 * @param[in] state The keyboard state.
 * @param[in] key The keycode of the key.
 * @param[in] layout The layout for which to get the shift level.  This must be
 * smaller than:
 * @code xkb_keymap_num_layouts_for_key(keymap, key) @endcode
 * usually it would be:
 * @code xkb_state_key_get_layout(state, key) @endcode
 *
 * @return The shift level index.  If the key or layout are invalid,
 * returns `::XKB_LEVEL_INVALID`.
 *
 * @invariant If the returned level is valid, the following always holds:
 * @code
 * xkb_state_key_get_level(state, key, layout) < xkb_keymap_num_levels_for_key(keymap, key, layout)
 * @endcode
 *
 * @memberof xkb_state
 */
XKB_EXPORT xkb_level_index_t
xkb_state_key_get_level(struct xkb_state *state, xkb_keycode_t key,
                        xkb_layout_index_t layout);

/**
 * @enum xkb_state_match
 * Match flags for `xkb_state::xkb_state_mod_indices_are_active()` and
 * `xkb_state::xkb_state_mod_names_are_active()`, specifying the conditions for a
 * successful match.  `::XKB_STATE_MATCH_NON_EXCLUSIVE` is bitmaskable with
 * the other modes.
 */
enum xkb_state_match {
    /** Returns `true` if any of the modifiers are active. */
    XKB_STATE_MATCH_ANY = (1 << 0),
    /** Returns `true` if all of the modifiers are active. */
    XKB_STATE_MATCH_ALL = (1 << 1),
    /**
     * @parblock
     * Makes matching non-exclusive, i.e. will not return `false` if a
     * modifier not specified in the arguments is active.
     * @endparblock
     */
    XKB_STATE_MATCH_NON_EXCLUSIVE = (1 << 16)
};

/**
 * Serialization of the *boolean* [global keyboard controls], to be used on the
 * server side of serialization.
 *
 * This entry point is intended for *server* applications; see @ref
 * server-client-state for details.
 *
 * @param[in] state      The keyboard state.
 * @param[in] components A mask of the keyboard control state components to
 * serialize. State components other than `::XKB_STATE_CONTROLS` are ignored.
 *
 * @returns A `xkb_keyboard_control_flags` mask representing the enabled
 * keyboard controls for the given @p components.
 *
 * @since 1.14.0
 *
 * @memberof xkb_state
 *
 * [global keyboard controls]: @ref xkb_keyboard_control_flags
 */
XKB_EXPORT enum xkb_keyboard_control_flags
xkb_state_serialize_enabled_controls(const struct xkb_state *state,
                                     enum xkb_state_component components);

/**
 * The counterpart to `xkb_state::xkb_state_update_mask()` for modifiers, to be
 * used on the server side of serialization.
 *
 * This entry point is intended for *server* applications; see @ref
 * server-client-state for details. *Client* applications should use the
 * `xkb_state_mod_*_is_active` API.
 *
 * @warning The serialization is lossy and will not survive round trips.
 * It must only be used to feed *client* state objects created with either
 * `::XKB_STATE_MODE_CLIENT` or `xkb_state_new()`, and must not be used to
 * update the *server* state.
 *
 * @param[in] state      The keyboard state.
 * @param[in] components A mask of the modifier state components to serialize.
 * State components other than `XKB_STATE_MODS_*` are ignored.
 * If `::XKB_STATE_MODS_EFFECTIVE` is included, all other state components are
 * ignored.
 *
 * @returns A `xkb_mod_mask_t` representing the given components of the
 * modifier state.
 *
 * @memberof xkb_state
 */
XKB_EXPORT xkb_mod_mask_t
xkb_state_serialize_mods(struct xkb_state *state,
                         enum xkb_state_component components);

/**
 * The counterpart to `xkb_state::xkb_state_update_mask()` for layouts, to be
 * used on the server side of serialization.
 *
 * This entry point is intended for *server* applications; see @ref
 * server-client-state for details. *Client* applications should use the
 * xkb_state_layout_*_is_active API.
 *
 * @warning The serialization is lossy and will not survive round trips.
 * It must only be used to feed *client* state objects created with either
 * `::XKB_STATE_MODE_CLIENT` or `xkb_state_new()`, and must not be used to
 * update the *server* state.
 *
 * @param[in] state      The keyboard state.
 * @param[in] components A mask of the layout state components to serialize.
 * State components other than `XKB_STATE_LAYOUT_*` are ignored.
 * If `::XKB_STATE_LAYOUT_EFFECTIVE` is included, all other state components are
 * ignored.
 *
 * @returns A layout index representing the given components of the
 * layout state.
 *
 * @memberof xkb_state
 */
XKB_EXPORT xkb_layout_index_t
xkb_state_serialize_layout(struct xkb_state *state,
                           enum xkb_state_component components);

/**
 * Test whether a modifier is active in a given keyboard state by name.
 *
 * @warning For [virtual modifiers], this function may *overmatch* in case
 * there are virtual modifiers with overlapping mappings to [real modifiers].
 *
 * @param[in]  state  The keyboard state object.
 * @param[in]  name   The modifier name, as a `NULL`-terminated string.
 * @param[in]  type   The component of the state against which to match the
 * given modifiers.
 *
 * @returns 1 if the modifier is active, 0 if it is not.  If the modifier
 * name does not exist in the keymap, returns -1.
 *
 * @memberof xkb_state
 *
 * @since 0.1.0: Works only with *real* modifiers
 * @since 1.8.0: Works also with *virtual* modifiers
 *
 * [virtual modifiers]: @ref virtual-modifier-def
 * [real modifiers]: @ref real-modifier-def
 */
XKB_EXPORT int
xkb_state_mod_name_is_active(struct xkb_state *state, const char *name,
                             enum xkb_state_component type);

/**
 * Test whether a set of modifiers are active in a given keyboard state by
 * name.
 *
 * @warning For [virtual modifiers], this function may *overmatch* in case
 * there are virtual modifiers with overlapping mappings to [real modifiers].
 *
 * @param[in] state The keyboard state.
 * @param[in] type  The component of the state against which to match the
 * given modifiers.
 * @param[in] match The manner by which to match the state against the
 * given modifiers.
 * @param[in] ...   The set of of modifier names to test, terminated by a `NULL`
 * argument (sentinel).
 *
 * @returns 1 if the modifiers are active, 0 if they are not.  If any of
 * the modifier names do not exist in the keymap, returns -1. If @p match
 * contains invalid flags, returns -2.
 *
 * @memberof xkb_state
 *
 * @since 0.1.0: Works only with *real* modifiers
 * @since 1.8.0: Works also with *virtual* modifiers
 * @since 1.14.0: Reject invalid @p match flags
 *
 * [virtual modifiers]: @ref virtual-modifier-def
 * [real modifiers]: @ref real-modifier-def
 */
XKB_EXPORT int
xkb_state_mod_names_are_active(struct xkb_state *state,
                               enum xkb_state_component type,
                               enum xkb_state_match match,
                               ...);

/**
 * Test whether a modifier is active in a given keyboard state by index.
 *
 * @warning For [virtual modifiers], this function may *overmatch* in case
 * there are virtual modifiers with overlapping mappings to [real modifiers].
 *
 * @param[in] state The keyboard state.
 * @param[in] idx   The index of the modifier to test.
 * @param[in] type  The component of the state against which to match the
 * given modifiers.
 *
 * @returns 1 if the modifier is active, 0 if it is not.  If the modifier
 * index is invalid in the keymap, returns -1.
 *
 * @memberof xkb_state
 *
 * @since 0.1.0: Works only with *real* modifiers
 * @since 1.8.0: Works also with *virtual* modifiers
 *
 * [virtual modifiers]: @ref virtual-modifier-def
 * [real modifiers]: @ref real-modifier-def
 */
XKB_EXPORT int
xkb_state_mod_index_is_active(struct xkb_state *state, xkb_mod_index_t idx,
                              enum xkb_state_component type);

/**
 * Test whether a set of modifiers are active in a given keyboard state by
 * index.
 *
 * @warning For [virtual modifiers], this function may *overmatch* in case
 * there are virtual modifiers with overlapping mappings to [real modifiers].
 *
 * @param[in] state The keyboard state.
 * @param[in] type  The component of the state against which to match the
 * given modifiers.
 * @param[in] match The manner by which to match the state against the
 * given modifiers.
 * @param[in] ...   The set of of modifier indices to test, terminated by a
 * `::XKB_MOD_INVALID` argument (sentinel).
 *
 * @returns 1 if the modifiers are active, 0 if they are not.  If any of
 * the modifier indices are invalid in the keymap, returns -1. If @p match
 * contains invalid flags, returns -2.
 *
 * @memberof xkb_state
 *
 * @since 0.1.0: Works only with *real* modifiers
 * @since 1.8.0: Works also with *virtual* modifiers
 * @since 1.14.0: Reject invalid @p match flags
 *
 * [virtual modifiers]: @ref virtual-modifier-def
 * [real modifiers]: @ref real-modifier-def
 */
XKB_EXPORT int
xkb_state_mod_indices_are_active(struct xkb_state *state,
                                 enum xkb_state_component type,
                                 enum xkb_state_match match,
                                 ...);

/**
 * @page consumed-modifiers Consumed Modifiers
 * @parblock
 *
 * Some functions, like `xkb_state::xkb_state_key_get_syms()`, look at the state
 * of the modifiers in the keymap and derive from it the correct shift level
 * to use for the key.  For example, in a US layout, pressing the key
 * labeled `<A>` while the Shift modifier is active, generates the keysym
 * `A`.  In this case, the Shift modifier is said to be *consumed*.
 * However, the Num Lock modifier does not affect this translation at all,
 * even if it is active, so it is not consumed by this translation.
 *
 * It may be desirable for some application to not reuse consumed modifiers
 * for further processing, e.g. for hotkeys or keyboard shortcuts.  To
 * understand why, consider some requirements from a standard shortcut
 * mechanism, and how they are implemented:
 *
 * 1. The shortcut’s modifiers must match exactly to the state.  For
 *    example, it is possible to bind separate actions to `<Alt><Tab>`
 *    and to `<Alt><Shift><Tab>`.  Further, if only `<Alt><Tab>` is
 *    bound to an action, pressing `<Alt><Shift><Tab>` should not
 *    trigger the shortcut.
 *    Effectively, this means that the modifiers are compared using the
 *    equality operator (`==`).
 *
 * 2. Only relevant modifiers are considered for the matching.  For example,
 *    Caps Lock and Num Lock should not generally affect the matching, e.g.
 *    when matching `<Alt><Tab>` against the state, it does not matter
 *    whether Num Lock is active or not.  These relevant, or *significant*,
 *    modifiers usually include Alt, Control, Shift, Super and similar.
 *    Effectively, this means that non-significant modifiers are masked out,
 *    before doing the comparison as described above.
 *
 * 3. The matching must be independent of the layout/keymap.  For example,
 *    the `<Plus>` (+) symbol is found on the first level on some layouts,
 *    but requires holding Shift on others.  If you simply bind the action
 *    to the `<Plus>` keysym, it would work for the unshifted kind, but
 *    not for the others, because the match against Shift would fail.  If
 *    you bind the action to `<Shift><Plus>`, only the shifted kind would
 *    work.  So what is needed is to recognize that Shift is used up in the
 *    translation of the keysym itself, and therefore should not be included
 *    in the matching.
 *    Effectively, this means that consumed modifiers (Shift in this example)
 *    are masked out as well, before doing the comparison.
 *
 * In summary, this is approximately how the matching would be performed:
 *
 * ```c
 *   (keysym == shortcut_keysym) &&
 *   ((state_mods & ~consumed_mods & significant_mods) == shortcut_mods)
 * ```
 *
 * @c state_mods are the modifiers reported by
 * `xkb_state::xkb_state_mod_index_is_active()` and similar functions.
 * @c consumed_mods are the modifiers reported by
 * `xkb_state::xkb_state_mod_index_is_consumed()` and similar functions.
 * @c significant_mods are decided upon by the application/toolkit/user;
 * it is up to them to decide whether these are configurable or hard-coded.
 *
 * @endparblock
 */

/**
 * @enum xkb_consumed_mode
 * Consumed modifiers mode.
 *
 * There are several possible methods for deciding which modifiers are
 * consumed and which are not, each applicable for different systems or
 * situations. The mode selects the method to use.
 *
 * Keep in mind that in all methods, the keymap may decide to *preserve*
 * a modifier, meaning it is not reported as consumed even if it would
 * have otherwise.
 */
enum xkb_consumed_mode {
    /**
     * This is the mode defined in the XKB specification and used by libX11.
     *
     * A modifier is consumed if and only if it *may affect* key translation.
     *
     * For example, if `Control+Alt+<Backspace>` produces some assigned keysym,
     * then when pressing just `<Backspace>`, `Control` and `Alt` are consumed,
     * even though they are not active, since if they *were* active they would
     * have affected key translation.
     */
    XKB_CONSUMED_MODE_XKB,
    /**
     * This is the mode used by the GTK+ toolkit.
     *
     * The mode consists of the following two independent heuristics:
     *
     * - The currently active set of modifiers, excluding modifiers which do
     *   not affect the key (as described for @ref XKB_CONSUMED_MODE_XKB), are
     *   considered consumed, if the keysyms produced when all of them are
     *   active are different from the keysyms produced when no modifiers are
     *   active.
     *
     * - A single modifier is considered consumed if the keysyms produced for
     *   the key when it is the only active modifier are different from the
     *   keysyms produced when no modifiers are active.
     */
    XKB_CONSUMED_MODE_GTK
};

/**
 * Get the mask of modifiers consumed by translating a given key.
 *
 * @param[in] state The keyboard state.
 * @param[in] key   The keycode of the key.
 * @param[in] mode  The consumed modifiers mode to use;
 *                  see [enum description](@ref xkb_consumed_mode).
 *
 * @returns a mask of the consumed [real modifiers] modifiers.
 *
 * @memberof xkb_state
 * @since 0.7.0
 *
 * [real modifiers]: @ref real-modifier-def
 */
XKB_EXPORT xkb_mod_mask_t
xkb_state_key_get_consumed_mods2(struct xkb_state *state, xkb_keycode_t key,
                                 enum xkb_consumed_mode mode);

/**
 * Same as `xkb_state_key_get_consumed_mods2()` with mode `::XKB_CONSUMED_MODE_XKB`.
 *
 * @memberof xkb_state
 * @since 0.4.1
 */
XKB_EXPORT xkb_mod_mask_t
xkb_state_key_get_consumed_mods(struct xkb_state *state, xkb_keycode_t key);

/**
 * Test whether a modifier is consumed by keyboard state translation for
 * a key.
 *
 * @warning For [virtual modifiers], this function may *overmatch* in case
 * there are virtual modifiers with overlapping mappings to [real modifiers].
 *
 * @param[in] state The keyboard state.
 * @param[in] key   The keycode of the key.
 * @param[in] idx   The index of the modifier to check.
 * @param[in] mode  The consumed modifiers mode to use; see enum description.
 *
 * @returns 1 if the modifier is consumed, 0 if it is not.  If the modifier
 * index is not valid in the keymap, returns -1.
 *
 * @sa xkb_state_mod_mask_remove_consumed()
 * @sa xkb_state_key_get_consumed_mods()
 * @memberof xkb_state
 * @since 0.7.0: Works only with *real* modifiers
 * @since 1.8.0: Works also with *virtual* modifiers
 *
 * [virtual modifiers]: @ref virtual-modifier-def
 * [real modifiers]: @ref real-modifier-def
 */
XKB_EXPORT int
xkb_state_mod_index_is_consumed2(struct xkb_state *state,
                                 xkb_keycode_t key,
                                 xkb_mod_index_t idx,
                                 enum xkb_consumed_mode mode);

/**
 * Same as `xkb_state_mod_index_is_consumed2()` with mode `::XKB_CONSUMED_MODE_XKB`.
 *
 * @warning For [virtual modifiers], this function may *overmatch* in case
 * there are virtual modifiers with overlapping mappings to [real modifiers].
 *
 * @memberof xkb_state
 * @since 0.4.1: Works only with *real* modifiers
 * @since 1.8.0: Works also with *virtual* modifiers
 *
 * [virtual modifiers]: @ref virtual-modifier-def
 * [real modifiers]: @ref real-modifier-def
 */
XKB_EXPORT int
xkb_state_mod_index_is_consumed(struct xkb_state *state, xkb_keycode_t key,
                                xkb_mod_index_t idx);

/**
 * Remove consumed modifiers from a modifier mask for a key.
 *
 * @deprecated Use `xkb_state_key_get_consumed_mods2()` instead.
 *
 * Takes the given modifier mask, and removes all modifiers which are
 * consumed for that particular key (as in `xkb_state_mod_index_is_consumed()`).
 *
 * @returns a mask of [real modifiers] modifiers.
 *
 * @sa xkb_state_mod_index_is_consumed()
 * @memberof xkb_state
 * @since 0.5.0: Works only with *real* modifiers
 * @since 1.8.0: Works also with *virtual* modifiers
 *
 * [real modifiers]: @ref real-modifier-def
 */
XKB_EXPORT xkb_mod_mask_t
xkb_state_mod_mask_remove_consumed(struct xkb_state *state, xkb_keycode_t key,
                                   xkb_mod_mask_t mask);

/**
 * Test whether a layout is active in a given keyboard state by name.
 *
 * @param[in] state The keyboard state.
 * @param[in] name  The layout name to test (`NULL`-terminated string).
 * @param[in] type  The component of the state against which to match the
 * given layout.
 *
 * @returns 1 if the layout is active, 0 if it is not.  If no layout with
 * this name exists in the keymap, return -1.
 *
 * If multiple layouts in the keymap have this name, the one with the lowest
 * index is tested.
 *
 * @sa xkb_layout_index_t
 * @memberof xkb_state
 */
XKB_EXPORT int
xkb_state_layout_name_is_active(struct xkb_state *state, const char *name,
                                enum xkb_state_component type);

/**
 * Test whether a layout is active in a given keyboard state by index.
 *
 * @param[in] state The keyboard state.
 * @param[in] idx   The layout index to test.
 * @param[in] type  The component of the state against which to match the
 * given layout.
 *
 * @returns 1 if the layout is active, 0 if it is not.  If the layout index
 * is not valid in the keymap, returns -1.
 *
 * @sa xkb_layout_index_t
 * @memberof xkb_state
 */
XKB_EXPORT int
xkb_state_layout_index_is_active(struct xkb_state *state,
                                 xkb_layout_index_t idx,
                                 enum xkb_state_component type);

/**
 * Test whether a LED is active in a given keyboard state by name.
 *
 * @param[in] state The keyboard state.
 * @param[in] name  The LED name to test (`NULL`-terminated string).
 *
 * @returns 1 if the LED is active, 0 if it not.  If no LED with this name
 * exists in the keymap, returns -1.
 *
 * @sa xkb_led_index_t
 * @memberof xkb_state
 */
XKB_EXPORT int
xkb_state_led_name_is_active(struct xkb_state *state, const char *name);

/**
 * Test whether a LED is active in a given keyboard state by index.
 *
 * @param[in] state The keyboard state.
 * @param[in] idx   The LED index to test.
 *
 * @returns 1 if the LED is active, 0 if it not.  If the LED index is not
 * valid in the keymap, returns -1.
 *
 * @sa xkb_led_index_t
 * @memberof xkb_state
 */
XKB_EXPORT int
xkb_state_led_index_is_active(struct xkb_state *state, xkb_led_index_t idx);

/** @} */

/* Leave this include last, so it can pick up our types, etc. */
#include <xkbcommon/xkbcommon-compat.h>

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _XKBCOMMON_H_ */
