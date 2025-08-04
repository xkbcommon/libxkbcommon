/*
 * Copyright © 2009 Dan Nicholson <dbn.lists@gmail.com>
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT-open-group
 *
 * Author: Dan Nicholson <dbn.lists@gmail.com>
 *         Daniel Stone <daniel@fooishbar.org>
 *         Ran Benita <ran234@gmail.com>
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <termios.h>
#endif

#include "xkbcommon/xkbcommon.h"
#include "tools-common.h"
#include "src/utils.h"
#include "src/keysym.h"
#include "src/compose/parser.h"
#include "src/keymap.h"

#ifdef _WIN32
#define S_ISFIFO(mode) 0
#endif

static void
print_keycode(struct xkb_keymap *keymap, const char* prefix,
              xkb_keycode_t keycode, const char *suffix) {
    const char *keyname = xkb_keymap_key_get_name(keymap, keycode);
    if (keyname) {
        printf("%s%-4s%s", prefix, keyname, suffix);
    } else {
        printf("%s%-4d%s", prefix, keycode, suffix);
    }
}

/* Variant of ModMaskText of main lib */
static void
print_mod_mask(struct xkb_keymap *keymap,
               enum mod_type type, xkb_mod_mask_t mask)
{
    /* We want to avoid boolean blindness, but we expected only 2 values */
    assert(type == MOD_REAL || type == MOD_BOTH);

    if (!mask) {
        printf("0");
        return;
    }

    const xkb_mod_index_t num_mods = xkb_keymap_num_mods(keymap);
    const xkb_mod_mask_t keymap_named_mods = (type == MOD_REAL)
        ? MOD_REAL_MASK_ALL
        : (xkb_mod_mask_t) ((UINT64_C(1) << num_mods) - 1);

    /* Print known mods */
    bool first = true;
    xkb_mod_mask_t named = mask & keymap_named_mods;
    for (xkb_mod_index_t mod = 0; named && mod < num_mods; mod++, named >>= 1) {
        if (named & UINT32_C(0x1)) {
            if (first) {
                first = false;
                printf("%s", xkb_keymap_mod_get_name(keymap, mod));
            } else {
                printf(" + %s", xkb_keymap_mod_get_name(keymap, mod));
            }
        }
    }
    if (mask & ~keymap_named_mods) {
        /* If some bits of the mask cannot be expressed with the known modifiers
         * of the given type, print it as hexadecimal */
        printf("%s%#"PRIx32, (first ? "" : " + "), mask & ~keymap_named_mods);
    }
}

/* Modifiers encodings, formatted as YAML */
void
print_modifiers_encodings(struct xkb_keymap *keymap) {
    printf("Modifiers encodings:");

    /* Find the padding required for modifier names */
    int padding = 0;
    for (xkb_mod_index_t mod = 0; mod < xkb_keymap_num_mods(keymap); mod++) {
        const char* name = xkb_keymap_mod_get_name(keymap, mod);
        padding = MAX(padding, (int) strlen(name));
    }

    /* Print encodings */
    static const char indent[] = "\n  ";
    for (xkb_mod_index_t mod = 0; mod < xkb_keymap_num_mods(keymap); mod++) {
        if (mod == 0)
            printf("%s# Real modifiers (predefined)", indent);
        else if (mod == _XKB_MOD_INDEX_NUM_ENTRIES)
            printf("\n%s# Virtual modifiers (keymap-dependent)", indent);
        const char* name = xkb_keymap_mod_get_name(keymap, mod);
        const xkb_mod_mask_t encoding = xkb_keymap_mod_get_mask(keymap, name);
        const int count = printf("%s%s", indent, name);
        printf(":%*s 0x%08"PRIx32,
               MAX(0, padding - count + (int)sizeof(indent) - 1), "", encoding);
        if (mod >= _XKB_MOD_INDEX_NUM_ENTRIES) {
            printf(" # ");
            if (encoding) {
                if (!(encoding & MOD_REAL_MASK_ALL)) {
                    /* Prevent printing the numeric form again */
                    if (encoding == (UINT32_C(1) << mod))
                        printf("Canonical virtual modifier");
                    else
                        printf("Non-canonical virtual modifier");
                } else {
                    print_mod_mask(keymap, MOD_REAL, encoding);
                }
                if (encoding & ~MOD_REAL_MASK_ALL)
                    printf(" (incompatible with X11)");
            } else {
                printf("(unmapped)");
            }
        }
    }
    printf("\n");
}

/* Key modifier maps, formatted as YAML */
void
print_keys_modmaps(struct xkb_keymap *keymap) {
    printf("Keys modifier maps:");
    uint32_t count = 0;
    const struct xkb_key *key;
    xkb_keys_foreach(key, keymap) {
        if (!key->modmap && !key->vmodmap)
            continue;
        print_keycode(keymap, "\n  ", key->keycode, ":");
        printf("\n    real:    ");
        print_mod_mask(keymap, MOD_REAL, key->modmap);
        printf("\n    virtual: ");
        print_mod_mask(keymap, MOD_BOTH, key->vmodmap);
        count++;
    }
    if (count == 0)
        printf(" {} # No modifier map");
    printf("\n");
}

static void
print_modifiers_names(struct xkb_state *state,
                      enum xkb_state_component components,
                      xkb_keycode_t keycode,
                      enum xkb_consumed_mode consumed_mode)
{
    struct xkb_keymap * const keymap = xkb_state_get_keymap(state);
    for (xkb_mod_index_t mod = 0; mod < xkb_keymap_num_mods(keymap); mod++) {
        if (xkb_state_mod_index_is_active(state, mod, components) <= 0)
            continue;
        const bool consumed =
            keycode != XKB_KEYCODE_INVALID &&
            xkb_state_mod_index_is_consumed2(state, keycode, mod, consumed_mode);
        printf(" %s%s",
               (consumed ? "-" : ""), xkb_keymap_mod_get_name(keymap, mod));
    }
}

static void
print_modifiers(struct xkb_state *state, enum xkb_state_component changed,
                xkb_keycode_t keycode, bool show_consumed,
                enum xkb_consumed_mode consumed_mode, bool verbose)
{
    static const struct {
        enum xkb_state_component component;
        unsigned int padding;
        const char *label;
    } types[] = {
        { XKB_STATE_MODS_DEPRESSED, 0, "depressed" },
        { XKB_STATE_MODS_LATCHED,   2, "latched"   },
        { XKB_STATE_MODS_LOCKED,    3, "locked"    },
        { XKB_STATE_MODS_EFFECTIVE, 0, "effective" },
    };
    if (changed) {
        for (unsigned int k = 0; k < ARRAY_SIZE(types); k++) {
            if (!(changed & types[k].component))
                continue;
            const xkb_mod_mask_t mods =
                xkb_state_serialize_mods(state, types[k].component);
            printf("%s-mods: 0x%08"PRIx32"; ", types[k].label, mods);
        }
    } else {
        const xkb_mod_mask_t mods =
            xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE);
        printf("modifiers: 0x%08"PRIx32, mods);
        print_modifiers_names(state, XKB_STATE_MODS_EFFECTIVE, keycode,
                                consumed_mode);
        printf("\n");
    }
}

static void
print_layouts(struct xkb_state *state, enum xkb_state_component changed,
              xkb_keycode_t keycode, bool verbose)
{
    static const struct {
        enum xkb_state_component component;
        unsigned int padding;
        const char *label;
    } types[] = {
        { XKB_STATE_LAYOUT_DEPRESSED, 0, "depressed" },
        { XKB_STATE_LAYOUT_LATCHED,   2, "latched"   },
        { XKB_STATE_LAYOUT_LOCKED,    3, "locked"    },
        { XKB_STATE_LAYOUT_EFFECTIVE, 0, "effective" },
    };
    for (unsigned int k = 0; k < ARRAY_SIZE(types); k++) {
        if (!(changed & types[k].component))
            continue;
        const xkb_layout_index_t layout =
            xkb_state_serialize_layout(state, types[k].component);
        printf("%s-layout: %"PRId32"; ", types[k].label, layout);
    }
}

void
tools_print_keycode_state(const char *prefix,
                          struct xkb_state *state,
                          struct xkb_compose_state *compose_state,
                          xkb_keycode_t keycode,
                          enum xkb_key_direction direction,
                          enum xkb_consumed_mode consumed_mode,
                          print_state_fields_mask_t fields)
{
    xkb_keysym_t sym;
#define BUFFER_SIZE MAX(XKB_COMPOSE_MAX_STRING_SIZE, XKB_KEYSYM_NAME_MAX_SIZE)
    static_assert(XKB_KEYSYM_UTF8_MAX_SIZE <= BUFFER_SIZE,
                  "buffer too small");
    char s[BUFFER_SIZE];
#undef BUFFER_SIZE
    xkb_layout_index_t layout;
    enum xkb_compose_status status;

    struct xkb_keymap * const keymap = xkb_state_get_keymap(state);

    if (prefix)
        printf("%s", prefix);
    printf("key %s", (direction == XKB_KEY_UP ? "up  " : "down"));
    print_keycode(keymap, " [ ", keycode, " ] ");

    if (direction == XKB_KEY_UP)
        goto out;

    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(state, keycode, &syms);

    if (nsyms <= 0)
        goto out;

    status = XKB_COMPOSE_NOTHING;
    if (compose_state)
        status = xkb_compose_state_get_status(compose_state);

    if (status == XKB_COMPOSE_COMPOSING || status == XKB_COMPOSE_CANCELLED)
        goto out;

    if (status == XKB_COMPOSE_COMPOSED) {
        sym = xkb_compose_state_get_one_sym(compose_state);
        syms = &sym;
        nsyms = 1;
    }
    else if (nsyms == 1) {
        sym = xkb_state_key_get_one_sym(state, keycode);
        syms = &sym;
    }

    if (prefix)
        printf("%s", prefix);

    print_keycode(keymap, "keycode [ ", keycode, " ] ");

    printf("keysyms [ ");
    for (int i = 0; i < nsyms; i++) {
        xkb_keysym_get_name(syms[i], s, sizeof(s));
        printf("%-*s ", XKB_KEYSYM_NAME_MAX_SIZE, s);
    }
    printf("] ");

    if (fields & PRINT_UNICODE) {
        if (status == XKB_COMPOSE_COMPOSED)
            xkb_compose_state_get_utf8(compose_state, s, sizeof(s));
        else
            xkb_state_key_get_utf8(state, keycode, s, sizeof(s));
        /* HACK: escape single control characters from C0 set using the
        * Unicode codepoint convention. Ideally we would like to escape
        * any non-printable character in the string.
        */
        if (!*s) {
            printf("unicode [   ] ");
        } else if (strlen(s) == 1 && (*s <= 0x1F || *s == 0x7F)) {
            printf("unicode [ U+%04hX ] ", *s);
        } else {
            printf("unicode [ %s ] ", s);
        }
    }

    layout = xkb_state_key_get_layout(state, keycode);
    if (fields & PRINT_LAYOUT) {
        printf("layout [ %s (%"PRIu32") ] ",
               xkb_keymap_layout_get_name(keymap, layout), layout);
    }

    printf("level [ %"PRIu32" ] ",
           xkb_state_key_get_level(state, keycode, layout));

    printf("mods [ ");
    for (xkb_mod_index_t mod = 0; mod < xkb_keymap_num_mods(keymap); mod++) {
        if (xkb_state_mod_index_is_active(state, mod,
                                          XKB_STATE_MODS_EFFECTIVE) <= 0)
            continue;
        if (xkb_state_mod_index_is_consumed2(state, keycode, mod,
                                             consumed_mode))
            printf("-%s ", xkb_keymap_mod_get_name(keymap, mod));
        else
            printf("%s ", xkb_keymap_mod_get_name(keymap, mod));
    }
    printf("] ");

    printf("leds [ ");
    for (xkb_led_index_t led = 0; led < xkb_keymap_num_leds(keymap); led++) {
        if (xkb_state_led_index_is_active(state, led) <= 0)
            continue;
        printf("%s ", xkb_keymap_led_get_name(keymap, led));
    }
    printf("] ");

out:
    printf("\n");
}

void
tools_print_state_changes(const char *prefix, struct xkb_state *state,
                          enum xkb_state_component changed)
{
    if (changed == 0)
        return;

    if (prefix)
        printf("%s", prefix);
    printf("state    [ ");
    print_layouts(state, changed, XKB_KEYCODE_INVALID, false);
    print_modifiers(state, changed, XKB_KEYCODE_INVALID, false,
                    XKB_CONSUMED_MODE_XKB /* unused*/, false);
    if (changed & XKB_STATE_LEDS)
        printf("leds ");
    printf("]\n");
}

#ifdef _WIN32
void
tools_disable_stdin_echo(void)
{
    HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(stdin_handle, &mode);
    SetConsoleMode(stdin_handle, mode & ~ENABLE_ECHO_INPUT);
}

void
tools_enable_stdin_echo(void)
{
    HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(stdin_handle, &mode);
    SetConsoleMode(stdin_handle, mode | ENABLE_ECHO_INPUT);
}
#else
void
tools_disable_stdin_echo(void)
{
    /* Same as `stty -echo`. */
    struct termios termios;
    if (tcgetattr(STDIN_FILENO, &termios) == 0) {
        termios.c_lflag &= ~ECHO;
        (void) tcsetattr(STDIN_FILENO, TCSADRAIN, &termios);
    }
}

void
tools_enable_stdin_echo(void)
{
    /* Same as `stty echo`. */
    struct termios termios;
    if (tcgetattr(STDIN_FILENO, &termios) == 0) {
        termios.c_lflag |= ECHO;
        (void) tcsetattr(STDIN_FILENO, TCSADRAIN, &termios);
    }
}

#endif

static inline bool
is_wayland_session(void)
{
    /* This simple check should be enough for our use case. */
    return !isempty(getenv("WAYLAND_DISPLAY"));
}

static inline bool
is_x11_session(void)
{
    /* This simple check should be enough for our use case. */
    return !isempty(getenv("DISPLAY"));
}

const char *
select_backend(const char *wayland, const char *x11, const char *fallback)
{
    if (wayland && is_wayland_session())
        return wayland;
    else if (x11 && is_x11_session())
        return x11;
    else
        return fallback;
}

int
tools_exec_command(const char *prefix, int real_argc, const char **real_argv)
{
    const char *argv[64] = {NULL};
    char executable[PATH_MAX];
    const char *command;
    int rc;

    if (((size_t)real_argc >= ARRAY_SIZE(argv))) {
        fprintf(stderr, "Too many arguments\n");
        return EXIT_INVALID_USAGE;
    }

    command = real_argv[0];

    rc = snprintf(executable, sizeof(executable),
                  "%s/%s-%s", LIBXKBCOMMON_TOOL_PATH, prefix, command);
    if (rc < 0 || (size_t) rc >= sizeof(executable)) {
        fprintf(stderr, "Failed to assemble command\n");
        return EXIT_FAILURE;
    }

    argv[0] = executable;
    for (int i = 1; i < MIN(real_argc, (int) ARRAY_SIZE(argv)); i++)
        argv[i] = real_argv[i];

    execv(executable, (char **) argv);
    if (errno == ENOENT) {
        fprintf(stderr, "Command '%s' is not available\n", command);
        return EXIT_INVALID_USAGE;
    } else {
        fprintf(stderr, "Failed to execute '%s' (%s)\n",
                command, strerror(errno));
    }

    return EXIT_FAILURE;
}

bool
is_pipe_or_regular_file(int fd)
{
    struct stat info;
    if (fstat(fd, &info) == 0) {
        return S_ISFIFO(info.st_mode) || S_ISREG(info.st_mode);
    } else {
        return false;
    }
}

FILE*
tools_read_stdin(void)
{
    FILE *file = tmpfile();
    if (!file) {
        fprintf(stderr, "Failed to create tmpfile\n");
        return NULL;
    }

    while (true) {
        char buf[4096] = {0};
        const size_t len = fread(buf, 1, sizeof(buf), stdin);
        if (ferror(stdin)) {
            fprintf(stderr, "Failed to read from stdin\n");
            goto err;
        }
        if (len > 0) {
            size_t wlen = fwrite(buf, 1, len, file);
            if (wlen != len) {
                fprintf(stderr, "Failed to write to tmpfile\n");
                goto err;
            }
        }
        if (feof(stdin))
            break;
    }
    fseek(file, 0, SEEK_SET);
    return file;
err:
    fclose(file);
    return NULL;
}
