/*
 * Copyright 1985, 1987, 1990, 1998  The Open Group
 * Copyright 2008  Dan Nicholson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors or their
 * institutions shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization from the authors.
 */

/************************************************************
 * Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ********************************************************/

/*
 * Copyright © 2009-2012 Daniel Stone
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#ifndef _XKBCOMMON_H_
#define _XKBCOMMON_H_

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <xkbcommon/xkbcommon-names.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Main libxkbcommon API.
 */

/**
 * @struct xkb_context
 * @brief Opaque top level library context object.
 *
 * The context contains various general library data and state, like
 * logging level and include paths.
 * Objects are created in a specific context, and multiple contexts may
 * coexist simultaneously. Objects from different contexts are completely
 * separated and do not share any memory or state.
 * A context is created, accessed, manipulated and destroyed through the
 * xkb_context_*() API.
 */
struct xkb_context;

/**
 * @struct xkb_keymap
 * @brief Opaque compiled XKB keymap object.
 *
 * The keymap object holds all of the static keyboard information obtained
 * from compiling XKB files.
 * A keymap is immutable after it is created (besides reference counts, etc.);
 * if you need to change it, you must create a new one.
 * A keymap object is created, accessed and destroyed through the
 * xkb_keymap_*() API.
 */
struct xkb_keymap;

/**
 * @struct xkb_state
 * @brief Opaque XKB keyboard state object.
 *
 * State objects contain the active state of a keyboard (or keyboards), such
 * as the currently effective layout and the active modifiers. It acts as a
 * simple state machine, wherein key presses and releases are the input, and
 * key symbols (keysyms) are the output.
 * A state object is created, accessed, manipulated and destroyed through the
 * xkb_state_*() API.
 */
struct xkb_state;

typedef uint32_t xkb_keycode_t;
typedef uint32_t xkb_keysym_t;
typedef uint32_t xkb_mod_index_t;
typedef uint32_t xkb_mod_mask_t;
typedef uint32_t xkb_layout_index_t;
typedef uint32_t xkb_layout_mask_t;
typedef uint32_t xkb_level_index_t;
typedef uint32_t xkb_led_index_t;
typedef uint32_t xkb_led_mask_t;

#define XKB_MOD_INVALID     (0xffffffff)
#define XKB_LAYOUT_INVALID  (0xffffffff)
#define XKB_KEYCODE_INVALID (0xffffffff)
#define XKB_LEVEL_INVALID   (0xffffffff)
#define XKB_LED_INVALID     (0xffffffff)

#define XKB_KEYCODE_MAX     (0xffffffff - 1)
#define xkb_keycode_is_legal_ext(kc) (kc <= XKB_KEYCODE_MAX)
#define xkb_keycode_is_legal_x11(kc) (kc >= 8 && kc <= 255)

/**
 * @brief Names to compile a keymap with, also known as RMLVO.
 *
 * These names together are the primary identifier for a keymap.
 * If any of the members is NULL or an empty string (""), a default value is
 * used.
 */
struct xkb_rule_names {
    /** The rules file to use. The rules file describes how to interpret
     *  the values of the model, layout, variant and options fields. */
    const char *rules;
    /** The keyboard model by which to interpret keycodes and LEDs. */
    const char *model;
    /** A comma seperated list of layouts (languages) to include in the
     *  keymap. */
    const char *layout;
    /** A comma seperated list of variants, one per layout, which may
     *  modify or augment the respective layout in various ways. */
    const char *variant;
    /** A comma seprated list of options, through which the user specifies
     *  non-layout related preferences, like which key combinations are used
     *  for switching layouts, or which key is the Compose key. */
    const char *options;
};

/**
 * @defgroup keysyms Keysyms
 * @brief Utility functions related to keysyms.
 *
 * @{
 */

/**
 * @brief Get the name of a keysym.
 *
 * @param[in]  keysym The keysym.
 * @param[out] buffer A string buffer to write the name into.
 * @param[in]  size   Size of the buffer.
 *
 * @remark Named keysyms are found in the xkbcommon/xkbcommon-keysyms.h
 * header file. Their name does not include the XKB_KEY_ prefix.
 * The name of Unicode keysyms is "U<codepoint>", e.g. "Ua1b2".
 * The name of other unnamed keysyms is the hexadecimal representation of
 * their value, e.g. "0xabcd1234".
 * An invalid keysym is returned as "Invalid".
 *
 * @warning If the buffer passed is too small, the string is truncated
 * (though still NUL-terminated); a size of at least 32 bytes is recommended.
 */
int
xkb_keysym_get_name(xkb_keysym_t keysym, char *buffer, size_t size);

/**
 * @brief Get a keysym from its name.
 *
 * @param name The name of a keysym. See remarks in
 * xkb_keysym_get_name(); this function will accept any name returned by that
 * function.
 *
 * @remark The lookup is case-sensitive.
 *
 * @returns The keysym, if name is valid.  Otherwise, XKB_KEY_NoSymbol is
 * returned.
 */
xkb_keysym_t
xkb_keysym_from_name(const char *name);

/**
 * @brief Get the Unicode/UTF-8 representation of a keysym.
 *
 * @param[in]  keysym The keysym.
 * @param[out] buffer A buffer to write the UTF-8 string into.
 * @param[in]  size   The size of buffer.  Must be at least 7.
 *
 * @returns The number of bytes written to the buffer.  A return value of
 * 0 means that the keysym does not have a known printable Unicode
 * representation.  A return value of -1 means that the buffer was too small.
 */
int
xkb_keysym_to_utf8(xkb_keysym_t keysym, char *buffer, size_t size);

/**
 * @brief Get the Unicode/UTF-32 representation of a keysym.
 *
 * @param keysym The keysym.
 *
 * @returns The Unicode/UTF-32 representation of keysym, which is also
 * compatible with UCS-4.  A return value of 0 means the keysym does not
 * have a known printable Unicode representation.
 */
uint32_t
xkb_keysym_to_utf32(xkb_keysym_t keysym);

/** @} */

/**
 * @defgroup context Library Context
 * @brief Creating, destroying and using library contexts.
 *
 * Every keymap compilation request must have a context associated with
 * it.  The context keeps around state such as the include path.
 *
 * @{
 */

/** @brief Flags for context creation. */
enum xkb_context_flags {
    /** Create this context with an empty include path. */
    XKB_CONTEXT_NO_DEFAULT_INCLUDES = (1 << 0),
};

/**
 * @brief Create a new context.
 *
 * @param flags Optional flags for the context, or 0.
 *
 * @returns A new context, or NULL on failure.
 *
 * @remark If successful, the caller holds a reference on the context, and
 * must call xkb_context_unref() when finished.
 *
 * @remark The user may set some environment variables to affect default
 * values in the context. See e.g. xkb_context_set_log_level() and
 * xkb_context_set_log_verbosity().
 */
struct xkb_context *
xkb_context_new(enum xkb_context_flags flags);

/**
 * @brief Take a new reference on a context.
 * @param context The context to reference.
 * @returns The passed in context.
 */
struct xkb_context *
xkb_context_ref(struct xkb_context *context);

/**
 * @brief Release a reference on a context, and possibly free it.
 * @param context The context to unreference.
 */
void
xkb_context_unref(struct xkb_context *context);

/**
 * @brief Append a new entry to the context's include path.
 * @returns 1 on success, or 0 if the include path could not be added or is
 * inaccessible.
 */
int
xkb_context_include_path_append(struct xkb_context *context, const char *path);

/**
 * @brief Append the default include paths to the context's include path.
 * @returns 1 on success, or 0 if the primary include path could not be added.
 */
int
xkb_context_include_path_append_default(struct xkb_context *context);

/**
 * @brief Reset the context's include path to the default.
 *
 * Removes all entries from the context's include path, and inserts the
 * default paths.
 *
 * @returns 1 on success, or 0 if the primary include path could not be added.
 */
int
xkb_context_include_path_reset_defaults(struct xkb_context *context);

/**
 * @brief Remove all entries from the context's include path.
 */
void
xkb_context_include_path_clear(struct xkb_context *context);

/**
 * @brief Get the number of paths in the context's include path.
 * @returns The number of paths in the context's include path.
 */
unsigned int
xkb_context_num_include_paths(struct xkb_context *context);

/**
 * @brief Get a specific include path from the context's include path.
 * @returns The include path at the specified index within the context, or
 * NULL if the index is invalid.
 */
const char *
xkb_context_include_path_get(struct xkb_context *context, unsigned int index);

/**
 * @brief Store custom user data in the context.
 *
 * This may be useful in conjuction with xkb_context_set_log_fn() or other
 * callbacks.
 */
void
xkb_context_set_user_data(struct xkb_context *context, void *user_data);

/**
 * @brief Retrieves stored user data from the context.
 *
 * @returns The stored user data.  If the user data wasn't set, or the
 * passed in context is NULL, returns NULL.
 *
 * This may be useful to access private user data from callbacks like a
 * custom logging function.
 **/
void *
xkb_context_get_user_data(struct xkb_context *context);

/** @} */

/**
 * @defgroup logging Logging Handling
 * @brief Manipulating how logging from this library is handled.
 *
 * @{
 */

/** @brief Specifies a logging level. */
enum xkb_log_level {
    XKB_LOG_LEVEL_CRITICAL = 10, /**< Log critical internal errors only. */
    XKB_LOG_LEVEL_ERROR = 20,    /**< Log all errors. */
    XKB_LOG_LEVEL_WARNING = 30,  /**< Log warnings and errors. */
    XKB_LOG_LEVEL_INFO = 40,     /**< Log information, warnings, and errors. */
    XKB_LOG_LEVEL_DEBUG = 50,    /**< Log everything. */
};

/**
 * @brief Set the current logging level.
 *
 * @param context The context in which to set the logging level.
 * @param level   The logging level to use.  Only messages from this level
 * and below will be logged.
 *
 * The default level is XKB_LOG_LEVEL_ERROR.  The environment variable
 * XKB_LOG, if set in the time the context was created, overrides the default
 * value.  It may be specified as a level number or name.
 */
void
xkb_context_set_log_level(struct xkb_context *context,
                          enum xkb_log_level level);

/**
 * @brief Get the current logging level.
 * @returns The current logging level.
 */
enum xkb_log_level
xkb_context_get_log_level(struct xkb_context *context);

/**
 * @brief Sets the current logging verbosity.
 *
 * The library can generate a number of warnings which are not helpful to
 * ordinary users of the library.  The verbosity may be increased if more
 * information is desired (e.g. when developing a new keymap).
 *
 * The default verbosity is 0.  The environment variable XKB_VERBOSITY, if
 * set in the time the context was created, overrdies the default value.
 *
 * @param context   The context in which to use the set verbosity.
 * @param verbosity The verbosity to use.  Currently used values are
 * 1 to 10, higher values being more verbose.  0 would result in no verbose
 * messages being logged.
 *
 * @remark Most verbose messages are of level XKB_LOG_LEVEL_WARNING or lower.
 */
void
xkb_context_set_log_verbosity(struct xkb_context *context, int verbosity);

/**
 * @brief Get the current logging verbosity of the context.
 * @returns The current logging verbosity.
 */
int
xkb_context_get_log_verbosity(struct xkb_context *context);

/**
 * @brief Set a custom function to handle logging messages.
 *
 * @param context The context in which to use the set logging function.
 * @param log_fn  The function that will be called for logging messages.
 * Passing NULL restores the default function, which logs to stderr.
 *
 * By default, log messages from this library are printed to stderr.  This
 * function allows you to replace the default behavior with a custom
 * handler.  The handler is only called with messages which match the
 * current logging level and verbosity settings for the context.
 * level is the logging level of the message.  format and args are the
 * same as in the vprintf(3) function.
 *
 * You may use xkb_context_set_user_data() on the context, and then call
 * xkb_context_get_user_data() from within the logging function to provide
 * it with additional private context.
 */
void
xkb_context_set_log_fn(struct xkb_context *context,
                       void (*log_fn)(struct xkb_context *context,
                                      enum xkb_log_level level,
                                      const char *format, va_list args));

/** @} */

/**
 * @defgroup keymap Keymap Creation
 * @brief Creating and destroying XKB keymaps.
 *
 * @{
 */

/** @brief Flags for keymap compilation. */
enum xkb_keymap_compile_flags {
    /** Apparently you can't have empty enums.  What a drag. */
    XKB_MAP_COMPILE_PLACEHOLDER = 0,
};

/**
 * @brief Create a keymap from RMLVO names.
 *
 * The primary keymap entry point: creates a new XKB keymap from a set of
 * RMLVO (Rules + Model + Layouts + Variants + Options) names.
 *
 * You should almost certainly be using this and nothing else to create
 * keymaps.
 *
 * @param context The context in which to create the keymap.
 * @param names   The RMLVO names to use.
 * @param flags   Optional flags for the keymap, or 0.
 *
 * @returns A keymap compiled according to the RMLVO names, or NULL if
 * the compilation failed.
 *
 * @sa xkb_rule_names
 */
struct xkb_keymap *
xkb_keymap_new_from_names(struct xkb_context *context,
                          const struct xkb_rule_names *names,
                          enum xkb_keymap_compile_flags flags);

/** @brief The possible keymap text formats. */
enum xkb_keymap_format {
    /** The current/classic XKB text format, as generated by xkbcomp -xkb. */
    XKB_KEYMAP_FORMAT_TEXT_V1 = 1,
};

/**
 * @brief Create a keymap from an XKB keymap file.
 *
 * @param context The context in which to create the keymap.
 * @param file    The XKB keymap file to compile.
 * @param format  The text format of the XKB keymap file to compile.
 * @param flags   Optional flags for the keymap, or 0.
 *
 * @returns A keymap compiled from the given XKB keymap file, or NULL of
 * the compilation failed.
 *
 * @remark The file must contain an entire XKB keymap.  For example, in the
 * XKB_KEYMAP_FORMAT_TEXT_V1 format, this means the file must contain one
 * top level '%xkb_keymap' section, which in turn contains other required
 * sections.
 */
struct xkb_keymap *
xkb_keymap_new_from_file(struct xkb_context *context, FILE *file,
                         enum xkb_keymap_format format,
                         enum xkb_keymap_compile_flags flags);

/**
 * @brief Create a keymap from an XKB keymap given as a string.
 *
 * This is just like xkb_keymap_new_from_file(), but instead of a file, gets
 * the XKB keymap as one enormous string.
 *
 * @see xkb_keymap_new_from_string()
 */
struct xkb_keymap *
xkb_keymap_new_from_string(struct xkb_context *context, const char *string,
                           enum xkb_keymap_format format,
                           enum xkb_keymap_compile_flags flags);

/**
 * @brief Take a new reference on a keymap.
 * @param keymap The keymap to reference.
 * @returns The passed in keymap.
 */
struct xkb_keymap *
xkb_keymap_ref(struct xkb_keymap *keymap);

/**
 * @brief Release a reference on a keymap, and possibly free it.
 * @param keymap The keymap to unreference. If the reference count reaches
 * zero, the keymap is freed.
 */
void
xkb_keymap_unref(struct xkb_keymap *keymap);

/**
 * @brief Get the compiled keymap as a string.
 *
 * @returns The compiled keymap as a NUL-terminated string, or NULL if
 * unsuccessful.
 *
 * The returned string may be fed back into xkb_map_new_from_string() to get
 * the exact same keymap (possibly in another process, etc.).
 *
 * @remark The returned string is dynamically allocated and should be freed
 * by the caller.
 */
char *
xkb_keymap_get_as_string(struct xkb_keymap *keymap);

/** @} */

/**
 * @defgroup components XKB State Components
 * @brief Enumeration of state components in a keymap.
 *
 * @{
 */

/**
 * Returns the number of modifiers active in the keymap.
 */
xkb_mod_index_t
xkb_keymap_num_mods(struct xkb_keymap *keymap);

/**
 * Returns the name of the modifier specified by 'idx', or NULL if invalid.
 */
const char *
xkb_keymap_mod_get_name(struct xkb_keymap *keymap, xkb_mod_index_t idx);

/**
 * Returns the index of the modifier specified by 'name', or XKB_MOD_INVALID.
 */
xkb_mod_index_t
xkb_keymap_mod_get_index(struct xkb_keymap *keymap, const char *name);

/**
 * Returns the number of groups active in the keymap.
 */
xkb_layout_index_t
xkb_keymap_num_layouts(struct xkb_keymap *keymap);

/**
 * Returns the name of the group specified by 'idx', or NULL if invalid.
 */
const char *
xkb_keymap_layout_get_name(struct xkb_keymap *keymap, xkb_layout_index_t idx);

/**
 * Returns the index of the layout specified by 'name', or XKB_LAYOUT_INVALID.
 */
xkb_layout_index_t
xkb_keymap_layout_get_index(struct xkb_keymap *keymap, const char *name);

/**
 * Returns the number of layouts active for the specified key.
 */
xkb_layout_index_t
xkb_keymap_num_layouts_for_key(struct xkb_keymap *keymap, xkb_keycode_t key);

/**
 * Returns the number of levels active for the specified key and layout.
 */
xkb_level_index_t
xkb_keymap_num_levels_for_key(struct xkb_keymap *keymap, xkb_keycode_t key,
                              xkb_layout_index_t layout);

/**
 * Returns the number of LEDs in the given map.
 */
xkb_led_index_t
xkb_keymap_num_leds(struct xkb_keymap *keymap);

/**
 * Returns the name of the LED specified by 'idx', or NULL if invalid.
 */
const char *
xkb_keymap_led_get_name(struct xkb_keymap *keymap, xkb_led_index_t idx);

/**
 * Returns the index of the LED specified by 'name', or XKB_LED_INVALID.
 */
xkb_led_index_t
xkb_keymap_led_get_index(struct xkb_keymap *keymap, const char *name);

/**
 * Returns 1 if the key should repeat, or 0 otherwise.
 */
int
xkb_keymap_key_repeats(struct xkb_keymap *keymap, xkb_keycode_t key);

/** @} */

/**
 * @defgroup state XKB State Objects
 * @brief Creating, destroying and manipulating keyboard state objects.
 *
 * @{
 */

/**
 * Returns a new XKB state object for use with the given keymap, or NULL on
 * failure.
 */
struct xkb_state *
xkb_state_new(struct xkb_keymap *keymap);

/**
 * Takes a new reference on a state object.
 */
struct xkb_state *
xkb_state_ref(struct xkb_state *state);

/**
 * Unrefs and potentially deallocates a state object; the caller must not
 * use the state object after calling this.
 */
void
xkb_state_unref(struct xkb_state *state);

/**
 * Get the keymap from which the state object was created.  Does not take
 * a new reference on the map; you must explicitly reference it yourself
 * if you plan to use it beyond the lifetime of the state.
 */
struct xkb_keymap *
xkb_state_get_keymap(struct xkb_state *state);

/** @brief Specifies the direction of the key (press / release). */
enum xkb_key_direction {
    XKB_KEY_UP,   /**< The key was released. */
    XKB_KEY_DOWN, /**< The key was pressed. */
};

/**
 * Updates a state object to reflect the given key being pressed or released.
 */
void
xkb_state_update_key(struct xkb_state *state, xkb_keycode_t key,
                     enum xkb_key_direction direction);

/**
 * Gives the symbols obtained from pressing a particular key with the given
 * state.  *syms_out will be set to point to an array of keysyms, with the
 * return value being the number of symbols in *syms_out.  If the return
 * value is 0, *syms_out will be set to NULL, as there are no symbols produced
 * by this event.
 */
int
xkb_state_key_get_syms(struct xkb_state *state, xkb_keycode_t key,
                       const xkb_keysym_t **syms_out);

/**
 * Returns the layout number that would be active for a particular key with
 * the given state.
 */
xkb_layout_index_t
xkb_state_key_get_layout(struct xkb_state *state, xkb_keycode_t key);

/**
 * Returns the level number that would be active for a particular key with
 * the given state and layout number, usually obtained from
 * xkb_state_key_get_layout.
 */
xkb_level_index_t
xkb_state_key_get_level(struct xkb_state *state, xkb_keycode_t key,
                        xkb_layout_index_t layout);

/**
 * Gives the symbols obtained from pressing a particular key with the given
 * layout and level.  *syms_out will be set to point to an array of keysyms,
 * with the return value being the number of symbols in *syms_out.  If the
 * return value is 0, *syms_out will be set to NULL, as there are no symbols
 * produced by this event.
 */
int
xkb_keymap_key_get_syms_by_level(struct xkb_keymap *keymap,
                                 xkb_keycode_t key,
                                 xkb_layout_index_t layout,
                                 xkb_level_index_t level,
                                 const xkb_keysym_t **syms_out);

/**
 * Modifier and group types for state objects.  This enum is bitmaskable,
 * e.g. (XKB_STATE_DEPRESSED | XKB_STATE_LATCHED) is valid to exclude
 * locked modifiers.
 */
enum xkb_state_component {
    /** A key holding this modifier or group is currently physically
     *  depressed; also known as 'base'. */
    XKB_STATE_DEPRESSED = (1 << 0),
    /** Modifier or group is latched, i.e. will be unset after the next
     *  non-modifier key press. */
    XKB_STATE_LATCHED = (1 << 1),
    /** Modifier or group is locked, i.e. will be unset after the key
     *  provoking the lock has been pressed again. */
    XKB_STATE_LOCKED = (1 << 2),
    /** Combinatination of depressed, latched, and locked. */
    XKB_STATE_EFFECTIVE =
        (XKB_STATE_DEPRESSED | XKB_STATE_LATCHED | XKB_STATE_LOCKED),
};

/**
 * Match flags for xkb_state_mod_indices_are_active and
 * xkb_state_mod_names_are_active, specifying how the conditions for a
 * successful match.  XKB_STATE_MATCH_NON_EXCLUSIVE is bitmaskable with
 * the other modes.
 */
enum xkb_state_match {
    /** Returns true if any of the modifiers are active. */
    XKB_STATE_MATCH_ANY = (1 << 0),
    /** Returns true if all of the modifiers are active. */
    XKB_STATE_MATCH_ALL = (1 << 1),
    /** Makes matching non-exclusive, i.e. will not return false if a
     *  modifier not specified in the arguments is active. */
    XKB_STATE_MATCH_NON_EXCLUSIVE = (1 << 16),
};

/**
 * Updates a state object from a set of explicit masks.  This entrypoint is
 * really only for window systems and the like, where a master process
 * holds an xkb_state, then serializes it over a wire protocol, and clients
 * then use the serialization to feed in to their own xkb_state.
 *
 * All parameters must always be passed, or the resulting state may be
 * incoherent.
 *
 * The serialization is lossy and will not survive round trips; it must only
 * be used to feed slave state objects, and must not be used to update the
 * master state.
 *
 * Please do not use this unless you fit the description above.
 */
void
xkb_state_update_mask(struct xkb_state *state, xkb_mod_mask_t base_mods,
                      xkb_mod_mask_t latched_mods, xkb_mod_mask_t locked_mods,
                      xkb_layout_index_t base_group,
                      xkb_layout_index_t latched_group,
                      xkb_layout_index_t locked_group);

/**
 * The counterpart to xkb_state_update_mask, to be used on the server side
 * of serialization.  Returns a xkb_mod_mask_t representing the given
 * component(s) of the state.
 *
 * This function should not be used in regular clients; please use the
 * xkb_state_mod_*_is_active or xkb_state_foreach_active_mod API instead.
 *
 * Can return NULL on failure.
 */
xkb_mod_mask_t
xkb_state_serialize_mods(struct xkb_state *state,
                         enum xkb_state_component component);

/**
 * The group equivalent of xkb_state_serialize_mods: please see its
 * documentation.
 */
xkb_layout_index_t
xkb_state_serialize_layout(struct xkb_state *state,
                           enum xkb_state_component component);

/**
 * Returns 1 if the modifier specified by 'name' is active in the manner
 * specified by 'type', 0 if it is unset, or -1 if the modifier does not
 * exist in the map.
 */
int
xkb_state_mod_name_is_active(struct xkb_state *state, const char *name,
                             enum xkb_state_component type);

/**
 * Returns 1 if the modifiers specified by the varargs (NULL-terminated
 * strings, with a NULL sentinel) are active in the manner specified by
 * 'match', 0 otherwise, or -1 if any of the modifiers do not exist in
 * the map.
 */
int
xkb_state_mod_names_are_active(struct xkb_state *state,
                               enum xkb_state_component type,
                               enum xkb_state_match match,
                               ...);

/**
 * Returns 1 if the modifier specified by 'idx' is active in the manner
 * specified by 'type', 0 if it is unset, or -1 if the modifier does not
 * exist in the current map.
 */
int
xkb_state_mod_index_is_active(struct xkb_state *state, xkb_mod_index_t idx,
                              enum xkb_state_component type);

/**
 * Returns 1 if the modifiers specified by the varargs (of type
 * xkb_mod_index_t, with a XKB_MOD_INVALID sentinel) are active in the
 * manner specified by 'match' and 'type', 0 otherwise, or -1 if any of
 * the modifiers do not exist in the map.
 */
int
xkb_state_mod_indices_are_active(struct xkb_state *state,
                                 enum xkb_state_component type,
                                 enum xkb_state_match match,
                                 ...);

/**
 * Returns 1 if the modifier specified by 'idx' is used in the
 * translation of the keycode 'key' to the key symbols obtained by
 * pressing it (as in xkb_key_get_syms), given the current state.
 * Returns 0 otherwise.
 */
int
xkb_state_mod_index_is_consumed(struct xkb_state *state, xkb_keycode_t key,
                                xkb_mod_index_t idx);

/**
 * Takes the given modifier mask, and removes all modifiers which are
 * marked as 'consumed' (see xkb_key_mod_index_is_consumed definition)
 * for that particular key.
 */
xkb_mod_mask_t
xkb_state_mod_mask_remove_consumed(struct xkb_state *state, xkb_keycode_t key,
                                   xkb_mod_mask_t mask);

/**
 * Returns 1 if the group specified by 'name' is active in the manner
 * specified by 'type', 0 if it is unset, or -1 if the group does not
 * exist in the current map.
 */
int
xkb_state_layout_name_is_active(struct xkb_state *state, const char *name,
                                enum xkb_state_component type);

/**
 * Returns 1 if the group specified by 'idx' is active in the manner
 * specified by 'type', 0 if it is unset, or -1 if the group does not
 * exist in the current map.
 */
int
xkb_state_layout_index_is_active(struct xkb_state *state,
                                 xkb_layout_index_t idx,
                                 enum xkb_state_component type);

/**
 * Returns 1 if the LED specified by 'name' is active, 0 if it is unset, or
 * -1 if the LED does not exist in the current map.
 */
int
xkb_state_led_name_is_active(struct xkb_state *state, const char *name);

/**
 * Returns 1 if the LED specified by 'idx' is active, 0 if it is unset, or
 * -1 if the LED does not exist in the current map.
 */
int
xkb_state_led_index_is_active(struct xkb_state *state, xkb_led_index_t idx);

/** @} */

/* Leave this include last, so it can pick up our types, etc. */
#include <xkbcommon/xkbcommon-compat.h>

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _XKBCOMMON_H_ */
