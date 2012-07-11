/*
Copyright 1985, 1987, 1990, 1998  The Open Group
Copyright 2008  Dan Nicholson

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the authors or their
institutions shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the authors.
*/

/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/*
 * Copyright © 2009 Daniel Stone
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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <xkbcommon/xkbcommon-names.h>
#include <xkbcommon/xkbcommon-keysyms.h>

typedef uint32_t xkb_keycode_t;
typedef uint32_t xkb_keysym_t;
typedef uint32_t xkb_mod_index_t;
typedef uint32_t xkb_mod_mask_t;
typedef uint32_t xkb_group_index_t;
typedef uint32_t xkb_led_index_t;

#define XKB_MOD_INVALID                 (0xffffffff)
#define XKB_GROUP_INVALID               (0xffffffff)
#define XKB_KEYCODE_INVALID             (0xffffffff)
#define XKB_LED_INVALID                 (0xffffffff)

#define XKB_KEYCODE_MAX                 (0xffffffff - 1)
#define xkb_keycode_is_legal_ext(kc)    (kc <= XKB_KEYCODE_MAX)
#define xkb_keycode_is_legal_x11(kc)    (kc >= 8 && kc <= 255)

/**
 * Names to compile a keymap with, also known as RMLVO.  These names together
 * should be the primary identifier for a keymap.
 */
struct xkb_rule_names {
    const char *rules;
    const char *model;
    const char *layout;
    const char *variant;
    const char *options;
};

/**
 * Opaque context object; may only be created, accessed, manipulated and
 * destroyed through the xkb_context_*() API.
 */
struct xkb_context;

/**
 * Opaque keymap object; may only be created, accessed, manipulated and
 * destroyed through the xkb_state_*() API.
 */
struct xkb_keymap;

/**
 * Opaque state object; may only be created, accessed, manipulated and
 * destroyed through the xkb_state_*() API.
 */
struct xkb_state;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Returns the name for a keysym as a string; will return unknown Unicode
 * codepoints as "Ua1b2", and other unknown keysyms as "0xabcd1234".
 * If the buffer passed is too small, the string is truncated; a size of
 * at least 32 bytes is recommended.
 */
void
xkb_keysym_get_name(xkb_keysym_t ks, char *buffer, size_t size);

/*
 * See xkb_keysym_get_name comments: this function will accept any string
 * from that function.
 */
xkb_keysym_t
xkb_keysym_from_name(const char *s);

/**
 * Return the printable representation of the keystring in Unicode/UTF-8.
 * The buffer passed must be at least 7 bytes long.  The return value
 * is the number of bytes written to the buffer.  A return value of zero
 * means that the keysym does not have a known printable Unicode
 * representation, and a return value of -1 means that the buffer was too
 * small to contain the return.
 */
int
xkb_keysym_to_utf8(xkb_keysym_t keysym, char *buffer, size_t size);

/**
 * Returns the Unicode/UTF-32 representation of the provided keysym, which is
 * also compatible with UCS-4.  A return value of zero means the keysym does
 * not have a known printable Unicode representation.
*/
uint32_t
xkb_keysym_to_utf32(xkb_keysym_t keysym);

/**
 * @defgroup context XKB contexts
 * Every keymap compilation request must have an XKB context associated with
 * it.  The context keeps around state such as the include path.
 *
 * @{
 */

enum xkb_context_flags {
    /** Create this context with an empty include path. */
    XKB_CONTEXT_NO_DEFAULT_INCLUDES = 1,
};

/**
 * Returns a new XKB context, or NULL on failure.  If successful, the caller
 * holds a reference on the context, and must free it when finished with
 * xkb_context_unref().
 */
struct xkb_context *
xkb_context_new(enum xkb_context_flags flags);

/**
 * Appends a new entry to the include path used for keymap compilation.
 * Returns 1 on success, or 0 if the include path could not be added or is
 * inaccessible.
 */
int
xkb_context_include_path_append(struct xkb_context *context, const char *path);

/**
 * Appends the default include paths to the context's current include path.
 * Returns 1 on success, or 0 if the primary include path could not be
 * added.
 */
int
xkb_context_include_path_append_default(struct xkb_context *context);

/**
 * Removes all entries from the context's include path, and inserts the
 * default paths.  Returns 1 on success, or 0 if the primary include path
 * could not be added.
 */
int
xkb_context_include_path_reset_defaults(struct xkb_context *context);

/**
 * Removes all entries from the context's include path.
 */
void
xkb_context_include_path_clear(struct xkb_context *context);

/**
 * Returns the number of include paths currently active in the context.
 */
unsigned int
xkb_context_num_include_paths(struct xkb_context *context);

/**
 * Returns the include path at the specified index within the context.
 */
const char *
xkb_context_include_path_get(struct xkb_context *context, unsigned int index);

/**
 * Takes a new reference on an XKB context.
 */
struct xkb_context *
xkb_context_ref(struct xkb_context *context);

/**
 * Releases a reference on an XKB context, and possibly frees it.
 */
void
xkb_context_unref(struct xkb_context *context);

/** @} */

/**
 * @defgroup map Keymap management
 * These utility functions allow you to create and deallocate XKB keymaps.
 *
 * @{
 */

enum xkb_map_compile_flags {
    /** Apparently you can't have empty enums.  What a drag. */
    XKB_MAP_COMPILE_PLACEHOLDER = 0,
};

/**
 * The primary keymap entry point: creates a new XKB keymap from a set of
 * RMLVO (Rules + Model + Layout + Variant + Option) names.
 *
 * You should almost certainly be using this and nothing else to create
 * keymaps.
 */
struct xkb_keymap *
xkb_map_new_from_names(struct xkb_context *context,
                       const struct xkb_rule_names *names,
                       enum xkb_map_compile_flags flags);

enum xkb_keymap_format {
    /** The current/classic XKB text format, as generated by xkbcomp -xkb. */
    XKB_KEYMAP_FORMAT_TEXT_V1 = 1,
};

/**
 * Creates an XKB keymap from a full text XKB keymap passed into the
 * file.
 */
struct xkb_keymap *
xkb_map_new_from_file(struct xkb_context *context,
                    FILE *file, enum xkb_keymap_format format,
                    enum xkb_map_compile_flags flags);

/**
 * Creates an XKB keymap from a full text XKB keymap serialized into one
 * enormous string.
 */
struct xkb_keymap *
xkb_map_new_from_string(struct xkb_context *context,
                        const char *string,
                        enum xkb_keymap_format format,
                        enum xkb_map_compile_flags flags);

/**
 * Returns the compiled XKB map as a string which can later be fed back into
 * xkb_map_new_from_string to return the exact same keymap.
 */
char *
xkb_map_get_as_string(struct xkb_keymap *keymap);

/**
 * Takes a new reference on a keymap.
 */
struct xkb_keymap *
xkb_map_ref(struct xkb_keymap *keymap);

/**
 * Releases a reference on a keymap.
 */
void
xkb_map_unref(struct xkb_keymap *keymap);

/** @} */

/**
 * @defgroup components XKB state components
 * Allows enumeration of state components such as modifiers and groups within
 * the current keymap.
 *
 * @{
 */

/**
 * Returns the number of modifiers active in the keymap.
 */
xkb_mod_index_t
xkb_map_num_mods(struct xkb_keymap *keymap);

/**
 * Returns the name of the modifier specified by 'idx', or NULL if invalid.
 */
const char *
xkb_map_mod_get_name(struct xkb_keymap *keymap, xkb_mod_index_t idx);

/**
 * Returns the index of the modifier specified by 'name', or XKB_MOD_INVALID.
 */
xkb_mod_index_t
xkb_map_mod_get_index(struct xkb_keymap *keymap, const char *name);

/**
 * Returns the number of groups active in the keymap.
 */
xkb_group_index_t
xkb_map_num_groups(struct xkb_keymap *keymap);

/**
 * Returns the name of the group specified by 'idx', or NULL if invalid.
 */
const char *
xkb_map_group_get_name(struct xkb_keymap *keymap, xkb_group_index_t idx);

/**
 * Returns the index of the group specified by 'name', or XKB_GROUP_INVALID.
 */
xkb_group_index_t
xkb_map_group_get_index(struct xkb_keymap *keymap, const char *name);

/**
 * Returns the number of groups active for the specified key.
 */
xkb_group_index_t
xkb_key_num_groups(struct xkb_keymap *keymap, xkb_keycode_t key);

/**
 * Returns 1 if the key should repeat, or 0 otherwise.
 */
int
xkb_key_repeats(struct xkb_keymap *keymap, xkb_keycode_t key);

/**
 * Returns the number of LEDs in the given map.
 */
xkb_led_index_t
xkb_map_num_leds(struct xkb_keymap *keymap);

/**
 * Returns the name of the LED specified by 'idx', or NULL if invalid.
 */
const char *
xkb_map_led_get_name(struct xkb_keymap *keymap, xkb_led_index_t idx);

/**
 * Returns the index of the LED specified by 'name', or XKB_LED_INVALID.
 */
xkb_led_index_t
xkb_map_led_get_index(struct xkb_keymap *keymap, const char *name);

/** @} */

/**
 * @defgroup state XKB state objects
 * Creation, destruction and manipulation of keyboard state objects,
 * representing modifier and group state.
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
xkb_state_get_map(struct xkb_state *state);

enum xkb_key_direction {
    XKB_KEY_UP,
    XKB_KEY_DOWN,
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
 *
 * This should be called before xkb_state_update_key.
 */
int
xkb_key_get_syms(struct xkb_state *state, xkb_keycode_t key,
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
xkb_state_update_mask(struct xkb_state *state,
                      xkb_mod_mask_t base_mods,
                      xkb_mod_mask_t latched_mods,
                      xkb_mod_mask_t locked_mods,
                      xkb_group_index_t base_group,
                      xkb_group_index_t latched_group,
                      xkb_group_index_t locked_group);

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
xkb_group_index_t
xkb_state_serialize_group(struct xkb_state *state,
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
 * Returns 1 if the modifiers specified by the varargs (treated as
 * NULL-terminated pointers to strings) are active in the manner
 * specified by 'match', 0 otherwise, or -1 if any of the modifiers
 * do not exist in the map.
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
 * Returns 1 if the modifiers specified by the varargs (treated as
 * xkb_mod_index_t, terminated with XKB_MOD_INVALID) are active in the manner
 * specified by 'match' and 'type', 0 otherwise, or -1 if the modifier does not
 * exist in the current map.
 */
int
xkb_state_mod_indices_are_active(struct xkb_state *state,
                                 enum xkb_state_component type,
                                 enum xkb_state_match match,
                                 ...);

/**
 * Returns 1 if the group specified by 'name' is active in the manner
 * specified by 'type', 0 if it is unset, or -1 if the group does not
 * exist in the current map.
 */
int
xkb_state_group_name_is_active(struct xkb_state *state, const char *name,
                               enum xkb_state_component type);

/**
 * Returns 1 if the group specified by 'idx' is active in the manner
 * specified by 'type', 0 if it is unset, or -1 if the group does not
 * exist in the current map.
 */
int
xkb_state_group_index_is_active(struct xkb_state *state, xkb_group_index_t idx,
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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _XKBCOMMON_H_ */
