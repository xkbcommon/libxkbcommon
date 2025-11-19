/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/input.h>

#include "xkbcommon/xkbcommon.h"

#include "src/utils.h"
#include "src/keymap-formats.h"
#include "tools-common.h"

struct keyboard {
    char *path;
    int fd;
    struct xkb_state_machine *state_machine;
    struct xkb_event_iterator *state_events;
    struct xkb_state *state;
    struct xkb_compose_state *compose_state;
    struct keyboard *next;
};

static bool verbose = false;
static bool terminate = false;
static int evdev_offset = 8;
static bool use_events_api = true;
static bool report_state_changes = true;
static bool with_compose = false;
static enum xkb_consumed_mode consumed_mode = XKB_CONSUMED_MODE_XKB;

enum print_state_options print_options = DEFAULT_PRINT_OPTIONS;

#define DEFAULT_INCLUDE_PATH_PLACEHOLDER "__defaults__"
#define NLONGS(n) (((n) + LONG_BIT - 1) / LONG_BIT)

static bool
evdev_bit_is_set(const unsigned long *array, int bit)
{
    return array[bit / LONG_BIT] & (1ULL << (bit % LONG_BIT));
}

/* Some heuristics to see if the device is a keyboard. */
static bool
is_keyboard(int fd)
{
    int i;
    unsigned long evbits[NLONGS(EV_CNT)] = { 0 };
    unsigned long keybits[NLONGS(KEY_CNT)] = { 0 };

    errno = 0;
    ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits);
    if (errno)
        return false;

    if (!evdev_bit_is_set(evbits, EV_KEY))
        return false;

    errno = 0;
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);
    if (errno)
        return false;

    for (i = KEY_RESERVED; i <= KEY_MIN_INTERESTING; i++)
        if (evdev_bit_is_set(keybits, i))
            return true;

    return false;
}

static int
keyboard_new(struct dirent *ent, struct xkb_keymap *keymap,
             const struct xkb_state_machine_options *options,
             enum xkb_keyboard_controls kbd_controls_affect,
             enum xkb_keyboard_controls kbd_controls_values,
             struct xkb_compose_table *compose_table, struct keyboard **out)
{
    int ret;
    char *path;
    int fd;
    struct xkb_state_machine *state_machine = NULL;
    struct xkb_event_iterator *state_events = NULL;
    struct xkb_state *state = NULL;
    struct xkb_compose_state *compose_state = NULL;
    struct keyboard *kbd = NULL;

    ret = asprintf(&path, "/dev/input/%s", ent->d_name);
    if (ret < 0)
        return -ENOMEM;

    fd = open(path, O_NONBLOCK | O_CLOEXEC | O_RDONLY);
    if (fd < 0) {
        ret = -errno;
        goto err_path;
    }

    if (!is_keyboard(fd)) {
        /* Dummy "skip this device" value. */
        ret = -ENOTSUP;
        goto err_fd;
    }

    if (use_events_api) {
        state_machine = xkb_state_machine_new(keymap, options);
        if (!state_machine) {
            fprintf(stderr, "Couldn't create xkb state machine for %s\n", path);
            ret = -EFAULT;
            goto err_state_machine;
        }

        state_events = xkb_event_iterator_new(state_machine);
        if (!state_events) {
            fprintf(stderr, "Couldn't create xkb events iterator for %s\n", path);
            ret = -EFAULT;
            goto err_state_events;
        }
    }

    state = xkb_state_new(keymap);
    if (!state) {
        fprintf(stderr, "Couldn't create xkb state for %s\n", path);
        ret = -EFAULT;
        goto err_state;
    }

    if (use_events_api) {
        xkb_state_machine_update_controls(state_machine, state_events,
                                          kbd_controls_affect, kbd_controls_values);
        const struct xkb_event *event;
        while ((event = xkb_event_iterator_next(state_events))) {
            xkb_state_update_from_event(state, event);
        }
    } else {
        xkb_state_update_controls(state, kbd_controls_affect, kbd_controls_values);
    }

    if (with_compose) {
        compose_state = xkb_compose_state_new(compose_table,
                                              XKB_COMPOSE_STATE_NO_FLAGS);
        if (!compose_state) {
            fprintf(stderr, "Couldn't create compose state for %s\n", path);
            ret = -EFAULT;
            goto err_compose_state;
        }
    }

    kbd = calloc(1, sizeof(*kbd));
    if (!kbd) {
        ret = -ENOMEM;
        goto err_kbd;
    }

    kbd->path = path;
    kbd->fd = fd;
    kbd->state_machine = state_machine;
    kbd->state_events = state_events;
    kbd->state = state;
    kbd->compose_state = compose_state;
    *out = kbd;
    return 0;

err_kbd:
    xkb_compose_state_unref(compose_state);
err_compose_state:
    xkb_state_unref(state);
err_state:
    xkb_event_iterator_destroy(state_events);
err_state_events:
    xkb_state_machine_unref(state_machine);
err_state_machine:
err_fd:
    close(fd);
err_path:
    free(path);
    return ret;
}

static void
keyboard_free(struct keyboard *kbd)
{
    if (!kbd)
        return;
    if (kbd->fd >= 0)
        close(kbd->fd);
    free(kbd->path);
    xkb_state_machine_unref(kbd->state_machine);
    xkb_event_iterator_destroy(kbd->state_events);
    xkb_state_unref(kbd->state);
    xkb_compose_state_unref(kbd->compose_state);
    free(kbd);
}

static int
filter_device_name(const struct dirent *ent)
{
    return !fnmatch("event*", ent->d_name, 0);
}

static struct keyboard *
get_keyboards(struct xkb_keymap *keymap,
              const struct xkb_state_machine_options *options,
              enum xkb_keyboard_controls kbd_controls_affect,
              enum xkb_keyboard_controls kbd_controls_values,
              struct xkb_compose_table *compose_table)
{
    int ret, i, nents;
    struct dirent **ents;
    struct keyboard *kbds = NULL, *kbd = NULL;

    nents = scandir("/dev/input", &ents, filter_device_name, alphasort);
    if (nents < 0) {
        fprintf(stderr, "Couldn't scan /dev/input: %s\n", strerror(errno));
        return NULL;
    }

    for (i = 0; i < nents; i++) {
        ret = keyboard_new(ents[i], keymap, options, kbd_controls_values,
                           kbd_controls_affect, compose_table, &kbd);
        if (ret) {
            if (ret == -EACCES) {
                fprintf(stderr, "Couldn't open /dev/input/%s: %s. "
                                "You probably need root to run this.\n",
                        ents[i]->d_name, strerror(-ret));
                break;
            }
            if (ret != -ENOTSUP) {
                fprintf(stderr, "Couldn't open /dev/input/%s: %s. Skipping.\n",
                        ents[i]->d_name, strerror(-ret));
            }
            continue;
        }

        assert(kbd != NULL);
        kbd->next = kbds;
        kbds = kbd;
    }

    if (!kbds) {
        fprintf(stderr, "Couldn't find any keyboards I can use! Quitting.\n");
        goto err;
    }

err:
    for (i = 0; i < nents; i++)
        free(ents[i]);
    free(ents);
    return kbds;
}

static void
free_keyboards(struct keyboard *kbds)
{
    struct keyboard *next;

    while (kbds) {
        next = kbds->next;
        keyboard_free(kbds);
        kbds = next;
    }
}

/* The meaning of the input_event 'value' field. */
enum {
    KEY_STATE_RELEASE = 0,
    KEY_STATE_PRESS = 1,
    KEY_STATE_REPEAT = 2,
};

static void
process_event(struct keyboard *kbd, uint16_t type, uint16_t code, int32_t value)
{
    if (type != EV_KEY)
        return;

    const xkb_keycode_t keycode = evdev_offset + code;
    struct xkb_keymap * const keymap = xkb_state_get_keymap(kbd->state);

    if ((value == KEY_STATE_REPEAT && !xkb_keymap_key_repeats(keymap, keycode)) ||
        (value != KEY_STATE_PRESS && value != KEY_STATE_REPEAT &&
         value != KEY_STATE_RELEASE))
        return;

    const enum xkb_key_direction direction = (value == KEY_STATE_RELEASE)
        ? XKB_KEY_UP
        : XKB_KEY_DOWN;

    if (use_events_api) {
        /* Use the state event API */
        const int ret = xkb_state_machine_update_key(kbd->state_machine,
                                                     kbd->state_events,
                                                     keycode, direction);
        if (ret) {
            fprintf(stderr, "ERROR: could not update the state machine\n");
            // TODO: better error handling
        } else {
            tools_print_events(NULL, kbd->state, kbd->state_events,
                               kbd->compose_state, print_options,
                               report_state_changes);
        }
    } else {
        /* Use the legacy state API */
        if (with_compose && direction == XKB_KEY_DOWN) {
            xkb_keysym_t keysym = xkb_state_key_get_one_sym(kbd->state, keycode);
            xkb_compose_state_feed(kbd->compose_state, keysym);
        }

        tools_print_keycode_state(
            NULL, kbd->state, kbd->compose_state, keycode, direction,
            consumed_mode, print_options
        );

        if (with_compose) {
            const enum xkb_compose_status status =
                xkb_compose_state_get_status(kbd->compose_state);
            if (status == XKB_COMPOSE_CANCELLED || status == XKB_COMPOSE_COMPOSED)
                xkb_compose_state_reset(kbd->compose_state);
        }

        const enum xkb_state_component changed =
            xkb_state_update_key(kbd->state, keycode, direction);

        if (changed && report_state_changes)
            tools_print_state_changes(NULL, kbd->state, changed, print_options);
    }
}

static int
read_keyboard(struct keyboard *kbd)
{
    ssize_t len;
    struct input_event evs[16];

    /* No fancy error checking here. */
    while ((len = read(kbd->fd, &evs, sizeof(evs))) > 0) {
        const size_t nevs = len / sizeof(struct input_event);
        for (size_t i = 0; i < nevs; i++)
            process_event(kbd, evs[i].type, evs[i].code, evs[i].value);
    }

    if (len < 0 && errno != EWOULDBLOCK) {
        fprintf(stderr, "Couldn't read %s: %s\n", kbd->path, strerror(errno));
        return 1;
    }

    return 0;
}

static int
loop(struct keyboard *kbds)
{
    int ret = -1;
    struct keyboard *kbd;
    nfds_t nfds, i;
    struct pollfd *fds = NULL;

    for (kbd = kbds, nfds = 0; kbd; kbd = kbd->next, nfds++) {}
    fds = calloc(nfds, sizeof(*fds));
    if (fds == NULL) {
        fprintf(stderr, "Out of memory");
        goto out;
    }

    for (i = 0, kbd = kbds; kbd; kbd = kbd->next, i++) {
        fds[i].fd = kbd->fd;
        fds[i].events = POLLIN;
    }

    while (!terminate) {
        ret = poll(fds, nfds, -1);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "Couldn't poll for events: %s\n",
                    strerror(errno));
            goto out;
        }

        for (i = 0, kbd = kbds; kbd; kbd = kbd->next, i++) {
            if (fds[i].revents != 0) {
                ret = read_keyboard(kbd);
                if (ret) {
                    goto out;
                }
            }
        }
    }

    ret = 0;
out:
    free(fds);
    return ret;
}

static void
sigintr_handler(int signum)
{
    terminate = true;
}

static void
usage(FILE *fp, char *progname)
{
        fprintf(fp, "Usage: %s [--include=<path>] [--include-defaults] [--format=<format>]"
                "[--rules=<rules>] [--model=<model>] [--layout=<layout>] "
                "[--variant=<variant>] [--options=<options>] "
                "[--enable-environment-names]\n",
                progname);
        fprintf(fp, "   or: %s --keymap <path to keymap file>\n",
                progname);
        fprintf(fp, "For both:\n"
                        "          --format <FORMAT> (use keymap format FORMAT)\n"
                        "          --verbose (enable verbose debugging output)\n"
                        "          -1, --uniline (enable uniline event output)\n"
                        "          --multiline (enable uniline event output)\n"
                        "          --short (shorter event output)\n"
                        "          --report-state-changes (report changes to the state)\n"
                        "          --no-state-report (do not report changes to the state)\n"
                        "          --legacy-state-api[=true|false] (use legacy state API instead of event API)\n"
                        "          --controls (sticky-keys, latch-to-lock, latch-simultaneous)\n"
                        "          --enable-compose (enable Compose)\n"
                        "          --consumed-mode={xkb|gtk} (select the consumed modifiers mode, default: xkb)\n"
                        "          --without-x11-offset (don't add X11 keycode offset)\n"
                    "Other:\n"
                        "          --help (display this help and exit)\n"
        );
}

int
main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    struct keyboard *kbds;
    struct xkb_context *ctx = NULL;
    struct xkb_keymap *keymap = NULL;
    struct xkb_compose_table *compose_table = NULL;
    const char *includes[64];
    size_t num_includes = 0;
    bool use_env_names = false;
    enum xkb_keymap_format keymap_format = DEFAULT_INPUT_KEYMAP_FORMAT;
    const char *rules = NULL;
    const char *model = NULL;
    const char *layout = NULL;
    const char *variant = NULL;
    const char *options = NULL;
    const char *keymap_path = NULL;
    const char *locale;
    struct sigaction act;
    enum options {
        OPT_VERBOSE,
        OPT_UNILINE,
        OPT_MULTILINE,
        OPT_INCLUDE,
        OPT_INCLUDE_DEFAULTS,
        OPT_ENABLE_ENV_NAMES,
        OPT_KEYMAP_FORMAT,
        OPT_RULES,
        OPT_MODEL,
        OPT_LAYOUT,
        OPT_VARIANT,
        OPT_OPTION,
        OPT_KEYMAP,
        OPT_WITHOUT_X11_OFFSET,
        OPT_LEGACY_STATE_API,
        OPT_CONTROLS,
        OPT_CONSUMED_MODE,
        OPT_COMPOSE,
        OPT_SHORT,
        OPT_REPORT_STATE,
        OPT_NO_STATE_REPORT,
    };
    static struct option opts[] = {
        {"help",                 no_argument,            0, 'h'},
        {"verbose",              no_argument,            0, OPT_VERBOSE},
        {"uniline",              no_argument,            0, OPT_UNILINE},
        {"multiline",            no_argument,            0, OPT_MULTILINE},
        {"include",              required_argument,      0, OPT_INCLUDE},
        {"include-defaults",     no_argument,            0, OPT_INCLUDE_DEFAULTS},
        {"enable-environment-names", no_argument,        0, OPT_ENABLE_ENV_NAMES},
        {"format",               required_argument,      0, OPT_KEYMAP_FORMAT},
        {"rules",                required_argument,      0, OPT_RULES},
        {"model",                required_argument,      0, OPT_MODEL},
        {"layout",               required_argument,      0, OPT_LAYOUT},
        {"variant",              required_argument,      0, OPT_VARIANT},
        {"options",              required_argument,      0, OPT_OPTION},
        {"keymap",               required_argument,      0, OPT_KEYMAP},
        {"legacy-state-api",     optional_argument,      0, OPT_LEGACY_STATE_API},
        {"controls",             required_argument,      0, OPT_CONTROLS},
        {"consumed-mode",        required_argument,      0, OPT_CONSUMED_MODE},
        {"enable-compose",       no_argument,            0, OPT_COMPOSE},
        {"short",                no_argument,            0, OPT_SHORT},
        {"report-state-changes", no_argument,            0, OPT_REPORT_STATE},
        {"no-state-report",      no_argument,            0, OPT_NO_STATE_REPORT},
        {"without-x11-offset",   no_argument,            0, OPT_WITHOUT_X11_OFFSET},
        {0, 0, 0, 0},
    };

    setlocale(LC_ALL, "");

    bool has_rmlvo_options = false;

    /* Initialize state options */
    ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS); /* Only used for state options */
    struct xkb_state_machine_options * const state_machine_options =
        xkb_state_machine_options_new(ctx);
    if (!state_machine_options)
        goto error_state_options;
    xkb_context_unref(ctx);
    ctx = NULL;
    enum xkb_keyboard_controls kbd_controls_affect = XKB_KEYBOARD_CONTROL_NONE;
    enum xkb_keyboard_controls kbd_controls_values = XKB_KEYBOARD_CONTROL_NONE;

    while (1) {
        int option_index = 0;
        int opt = getopt_long(argc, argv, "*1h", opts, &option_index);
        if (opt == -1)
            break;

        switch (opt) {
        case OPT_VERBOSE:
            verbose = true;
            break;
        case '1':
        case OPT_UNILINE:
            print_options |= PRINT_UNILINE;
            break;
        case '*':
        case OPT_MULTILINE:
            print_options &= ~PRINT_UNILINE;
            break;
        case OPT_INCLUDE:
            if (num_includes >= ARRAY_SIZE(includes))
                goto too_many_includes;
            includes[num_includes++] = optarg;
            break;
        case OPT_INCLUDE_DEFAULTS:
            if (num_includes >= ARRAY_SIZE(includes))
                goto too_many_includes;
            includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;
            break;
        case OPT_ENABLE_ENV_NAMES:
            use_env_names = true;
            break;
        case OPT_KEYMAP_FORMAT:
            keymap_format = xkb_keymap_parse_format(optarg);
            if (!keymap_format) {
                fprintf(stderr, "ERROR: invalid --format \"%s\"\n", optarg);
                usage(stderr, argv[0]);
                ret = EXIT_INVALID_USAGE;
                goto error_parse_args;
            }
            break;
        case OPT_RULES:
            if (keymap_path)
                goto input_format_error;
            rules = optarg;
            has_rmlvo_options = true;
            break;
        case OPT_MODEL:
            if (keymap_path)
                goto input_format_error;
            model = optarg;
            has_rmlvo_options = true;
            break;
        case OPT_LAYOUT:
            if (keymap_path)
                goto input_format_error;
            layout = optarg;
            has_rmlvo_options = true;
            break;
        case OPT_VARIANT:
            if (keymap_path)
                goto input_format_error;
            variant = optarg;
            has_rmlvo_options = true;
            break;
        case OPT_OPTION:
            if (keymap_path)
                goto input_format_error;
            options = optarg;
            has_rmlvo_options = true;
            break;
        case OPT_KEYMAP:
            if (has_rmlvo_options)
                goto input_format_error;
            keymap_path = optarg;
            break;
        case OPT_WITHOUT_X11_OFFSET:
            evdev_offset = 0;
            break;
        case OPT_REPORT_STATE:
            report_state_changes = true;
            break;
        case OPT_NO_STATE_REPORT:
            report_state_changes = false;
            break;
        case OPT_COMPOSE:
            with_compose = true;
            break;
        case OPT_SHORT:
            print_options &= ~PRINT_VERBOSE;
            break;
        case OPT_CONSUMED_MODE:
            if (strcmp(optarg, "gtk") == 0) {
                consumed_mode = XKB_CONSUMED_MODE_GTK;
            } else if (strcmp(optarg, "xkb") == 0) {
                consumed_mode = XKB_CONSUMED_MODE_XKB;
            } else {
                fprintf(stderr, "ERROR: invalid --consumed-mode \"%s\"\n", optarg);
                usage(stderr, argv[0]);
                ret = EXIT_INVALID_USAGE;
                goto error_parse_args;
            }
            break;
#ifdef ENABLE_PRIVATE_APIS
        case OPT_PRINT_MODMAPS:
            print_modmaps = true;
            break;
#endif
        case OPT_LEGACY_STATE_API: {
            bool legacy_api = true;
            if (!tools_parse_bool(optarg, TOOLS_ARG_OPTIONAL, &legacy_api)) {
                usage(stderr, argv[0]);
                ret = EXIT_INVALID_USAGE;
                goto error_parse_args;
            }
            use_events_api = !legacy_api;
            break;
        }
        case OPT_CONTROLS:
            if (!tools_parse_controls(optarg, state_machine_options,
                                      &kbd_controls_affect,
                                      &kbd_controls_values)) {
                usage(stderr, argv[0]);
                ret = EXIT_INVALID_USAGE;
                goto error_parse_args;
            }
            /* --legacy-state-api=false is implied */
            use_events_api = true;
            break;
        case 'h':
            usage(stdout, argv[0]);
            ret = EXIT_SUCCESS;
            goto error_parse_args;
        default:
            usage(stderr, argv[0]);
            ret = EXIT_INVALID_USAGE;
            goto error_parse_args;
        }
    }

    if (optind < argc && !isempty(argv[optind])) {
        /* Some positional arguments left: use as a keymap input */
        if (keymap_path || has_rmlvo_options)
            goto too_much_arguments;
        keymap_path = argv[optind++];
        if (optind < argc) {
too_much_arguments:
            fprintf(stderr, "ERROR: Too much positional arguments\n");
            usage(stderr, argv[0]);
            ret = EXIT_INVALID_USAGE;
            goto error_parse_args;
        }
    }

    if (!(print_options & PRINT_VERBOSE) && (print_options & PRINT_UNILINE)) {
        print_options &= ~PRINT_VERBOSE_ONE_LINE_FIELDS;
    }

    enum xkb_context_flags ctx_flags = XKB_CONTEXT_NO_DEFAULT_INCLUDES;
    if (!use_env_names)
        ctx_flags |= XKB_CONTEXT_NO_ENVIRONMENT_NAMES;

    ctx = xkb_context_new(ctx_flags);
    if (!ctx) {
        fprintf(stderr, "ERROR: Couldn't create xkb context\n");
        goto out;
    }

    if (verbose)
        tools_enable_verbose_logging(ctx);

    if (num_includes == 0)
        includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;

    for (size_t i = 0; i < num_includes; i++) {
        const char *include = includes[i];
        if (strcmp(include, DEFAULT_INCLUDE_PATH_PLACEHOLDER) == 0)
            xkb_context_include_path_append_default(ctx);
        else
            xkb_context_include_path_append(ctx, include);
    }

    if (keymap_path) {
        FILE *file = fopen(keymap_path, "rb");
        if (!file) {
            fprintf(stderr, "ERROR: Couldn't open '%s': %s\n",
                    keymap_path, strerror(errno));
            goto out;
        }
        keymap = xkb_keymap_new_from_file(ctx, file, keymap_format,
                                          XKB_KEYMAP_COMPILE_NO_FLAGS);
        fclose(file);
    }
    else {
        struct xkb_rule_names rmlvo = {
            .rules = (isempty(rules)) ? NULL : rules,
            .model = (isempty(model)) ? NULL : model,
            .layout = (isempty(layout)) ? NULL : layout,
            .variant = (isempty(variant)) ? NULL : variant,
            .options = (isempty(options)) ? NULL : options
        };

        if (!rules && !model && !layout && !variant && !options)
            keymap = xkb_keymap_new_from_names2(ctx, NULL, keymap_format,
                                                XKB_KEYMAP_COMPILE_NO_FLAGS);
        else
            keymap = xkb_keymap_new_from_names2(ctx, &rmlvo, keymap_format,
                                                XKB_KEYMAP_COMPILE_NO_FLAGS);

        if (!keymap) {
            fprintf(stderr,
                    "ERROR: Failed to compile RMLVO: "
                    "'%s', '%s', '%s', '%s', '%s'\n",
                    rules, model, layout, variant, options);
            goto out;
        }
    }

    if (!keymap) {
        fprintf(stderr, "ERROR: Couldn't create xkb keymap\n");
        goto out;
    }

    if (with_compose) {
        locale = setlocale(LC_CTYPE, NULL);
        compose_table =
            xkb_compose_table_new_from_locale(ctx, locale,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
        if (!compose_table) {
            fprintf(stderr, "ERROR: Couldn't create compose from locale\n");
            goto out;
        }
    }

    kbds = get_keyboards(keymap, state_machine_options, kbd_controls_affect,
                         kbd_controls_values, compose_table);
    if (!kbds) {
        goto out;
    }

#ifdef ENABLE_PRIVATE_APIS
    if (print_modmaps) {
        print_keys_modmaps(keymap);
        putchar('\n');
        print_modifiers_encodings(keymap);
        putchar('\n');
    }
#endif

    act.sa_handler = sigintr_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    tools_disable_stdin_echo();
    ret = loop(kbds);
    tools_enable_stdin_echo();

    free_keyboards(kbds);
out:
    xkb_compose_table_unref(compose_table);
    xkb_keymap_unref(keymap);
error_parse_args:
error_state_options:
    xkb_state_machine_options_destroy(state_machine_options);
    xkb_context_unref(ctx);

    return ret;

too_many_includes:
    fprintf(stderr, "ERROR: too many includes (max: %zu)\n",
            ARRAY_SIZE(includes));
    xkb_state_machine_options_destroy(state_machine_options);
    exit(EXIT_INVALID_USAGE);

input_format_error:
    fprintf(stderr, "ERROR: Cannot use RMLVO options with keymap input\n");
    usage(stderr, argv[0]);
    xkb_state_machine_options_destroy(state_machine_options);
    exit(EXIT_INVALID_USAGE);
}
