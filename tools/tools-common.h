/*
 * Copyright © 2012 Intel Corporation
 * SPDX-License-Identifier: MIT
 *
 * Author: Daniel Stone <daniel@fooishbar.org>
 */

#pragma once

#include "config.h"
#include "darray.h"

#include <assert.h>
#include <stdbool.h>

/* Don't use compat names in internal code. */
#define _XKBCOMMON_COMPAT_H
#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-compose.h"
#include "src/keymap.h"

#define ARRAY_SIZE(arr) ((sizeof(arr) / sizeof(*(arr))))

enum {
    DEFAULT_KEYMAP_COMPILE_FLAGS = XKB_KEYMAP_COMPILE_NO_FLAGS,
    DEFAULT_KEYMAP_SERIALIZE_FLAGS = XKB_KEYMAP_SERIALIZE_PRETTY |
                                     XKB_KEYMAP_SERIALIZE_KEEP_UNUSED
};

/* Fields that are printed in the interactive tools. */
enum print_state_options {
    PRINT_LAYOUT = (1u << 0),
    PRINT_UNICODE = (1u << 1),
    PRINT_ALL_FIELDS = ((PRINT_UNICODE << 1) - 1),
    /*
     * Fields that can be hidden with the option --short.
     * NOTE: If this value is modified, remember to update the documentation of
     *       the --short option in the corresponding tools.
     */
    PRINT_VERBOSE_ONE_LINE_FIELDS = (PRINT_LAYOUT | PRINT_UNICODE),
    PRINT_VERBOSE = (1u << 2),
    PRINT_UNILINE = (1u << 3),
    DEFAULT_PRINT_OPTIONS = PRINT_ALL_FIELDS | PRINT_VERBOSE | PRINT_UNILINE
};

void
print_modifiers_encodings(struct xkb_keymap *keymap);
void
print_keys_modmaps(struct xkb_keymap *keymap);

void
tools_enable_verbose_logging(struct xkb_context *ctx);

void
tools_print_keycode_state(const char *prefix,
                          struct xkb_state *state,
                          struct xkb_compose_state *compose_state,
                          xkb_keycode_t keycode,
                          enum xkb_key_direction direction,
                          enum xkb_consumed_mode consumed_mode,
                          enum print_state_options options);

void
tools_print_state_changes(const char *prefix, struct xkb_state *state,
                          enum xkb_state_component changed,
                          enum print_state_options options);

void
tools_print_events(const char *prefix, struct xkb_state *state,
                   struct xkb_events *events,
                   struct xkb_compose_state *compose_state,
                   enum xkb_consumed_mode consumed_mode,
                   enum print_state_options options, bool report_state_changes);

void
tools_disable_stdin_echo(void);

void
tools_enable_stdin_echo(void);

const char *
select_backend(const char *wayland, const char *x11, const char *fallback);

int
tools_exec_command(const char *prefix, int argc, const char **argv);

bool
is_pipe_or_regular_file(int fd);

FILE*
tools_read_stdin(void);

/**
 * Specialized bool for CLI arguments optionality, in order to avoid
 * boolean blindness
 */
enum tools_arg_optionality {
    TOOLS_ARG_REQUIRED = 0,
    TOOLS_ARG_OPTIONAL = 1
};

/**
 * If `optional` is `TOOLS_ARG_OPTIONAL`, then `out` is unchanged.
 * Set `out` to the default value before calling this function.
 */
bool
tools_parse_bool(const char *s, enum tools_arg_optionality optional, bool *out);

/** Raw modifier masks: plus-separated list of modifier names */
struct xkb_raw_mod_mask {
    darray_char names;
    darray(darray_size_t) indices;
};

#define xkb_raw_mod_mask_new() { \
    .names = darray_new(),       \
    .indices = darray_new(),     \
}

/** Raw modifier mapping */
struct xkb_machine_mods_raw_mapping {
    struct xkb_raw_mod_mask source;
    struct xkb_raw_mod_mask target;
};

/** Precursor of xkb_machine_builder, to handle tools args parsing */
struct xkb_machine_options {
    struct {
        struct {
            enum xkb_keyboard_control_flags affect;
            enum xkb_keyboard_control_flags flags;
        } boolean; /**< Initial boolean controls */
        struct {
            enum xkb_a11y_flags affect;
            enum xkb_a11y_flags flags;
        } a11y; /**< Initial A11Y flags */
    } controls;

    struct {
        struct xkb_raw_mod_mask mask;
        darray(xkb_layout_index_t) mappings;
    } shortcuts; /**< Shortcut tweak */

    /** Modifiers tweak */
    darray(struct xkb_machine_mods_raw_mapping) modifiers;
};

#define xkb_machine_options_new() {     \
    .controls = {0},                    \
    .shortcuts = {                      \
        .mask = xkb_raw_mod_mask_new(), \
        .mappings = darray_new(),       \
    },                                  \
    .modifiers = darray_new(),          \
}

void
xkb_machine_options_free(struct xkb_machine_options *options);

bool
tools_parse_controls(const char *s, struct xkb_machine_options *options);

bool
tools_parse_modifiers_mappings(const char *raw,
                               struct xkb_machine_options *options);

bool
tools_parse_shortcuts_mask(const char *raw, struct xkb_machine_options *options);

bool
tools_parse_shortcuts_mappings(const char *raw,
                               struct xkb_machine_options *options);

struct xkb_machine_builder *
xkb_machine_builder_new_from_options(struct xkb_keymap *keymap,
                                     const struct xkb_machine_options *options);

#ifdef _WIN32
#define setenv(varname, value, overwrite) _putenv_s((varname), (value))
#define unsetenv(varname) _putenv_s(varname, "")
#endif
