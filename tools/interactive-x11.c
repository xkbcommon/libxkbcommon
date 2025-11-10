/*
 * Copyright © 2013 Ran Benita <ran234@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <getopt.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <xcb/xkb.h>
#include <xcb/xproto.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-x11.h"
#include "xkbcommon/xkbcommon-compose.h"
#include "src/keymap-formats.h"
#include "src/utils.h"
#include "tools-common.h"

/*
 * Note: This program only handles the core keyboard device for now.
 * It should be straigtforward to change struct keyboard to a list of
 * keyboards with device IDs, as in tools/interactive-evdev.c. This would
 * require:
 *
 * - Initially listing the keyboard devices.
 * - Listening to device changes.
 * - Matching events to their devices.
 *
 * XKB itself knows about xinput1 devices, and most requests and events are
 * device-specific.
 *
 * In order to list the devices and react to changes, you need xinput1/2.
 * You also need xinput for the key press/release event, since the core
 * protocol key press event does not carry a device ID to match on.
 */

struct keyboard {
    xcb_connection_t *conn;
    uint8_t first_xkb_event;
    struct xkb_context *ctx;

    struct xkb_keymap *keymap;
    struct xkb_state *state;
    struct xkb_state_machine *state_machine;
    struct xkb_event_iterator *state_events;
    struct xkb_compose_state *compose_state;
    int32_t device_id;
};

static bool terminate;
#ifdef KEYMAP_DUMP
static enum xkb_keymap_format keymap_format = DEFAULT_OUTPUT_KEYMAP_FORMAT;
static enum xkb_keymap_serialize_flags serialize_flags =
    (enum xkb_keymap_serialize_flags) DEFAULT_KEYMAP_SERIALIZE_FLAGS;
#else
static bool use_events_api = true;
static enum print_state_options print_options = DEFAULT_PRINT_OPTIONS;
static bool report_state_changes = true;
static bool use_local_state = false;
static struct xkb_any_state_options any_state_options = { 0 };
static enum xkb_keyboard_controls kbd_controls_affect = XKB_KEYBOARD_CONTROL_NONE;
static enum xkb_keyboard_controls kbd_controls_values = XKB_KEYBOARD_CONTROL_NONE;
static struct xkb_keymap *custom_keymap = NULL;
#endif

static int
select_xkb_events_for_device(xcb_connection_t *conn, int32_t device_id)
{
    enum {
        required_events =
            (XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
             XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
             XCB_XKB_EVENT_TYPE_STATE_NOTIFY),

        required_nkn_details =
            (XCB_XKB_NKN_DETAIL_KEYCODES),

        required_map_parts =
            (XCB_XKB_MAP_PART_KEY_TYPES |
             XCB_XKB_MAP_PART_KEY_SYMS |
             XCB_XKB_MAP_PART_MODIFIER_MAP |
             XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
             XCB_XKB_MAP_PART_KEY_ACTIONS |
             XCB_XKB_MAP_PART_VIRTUAL_MODS |
             XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP),

        required_state_details =
            (XCB_XKB_STATE_PART_MODIFIER_BASE |
             XCB_XKB_STATE_PART_MODIFIER_LATCH |
             XCB_XKB_STATE_PART_MODIFIER_LOCK |
             XCB_XKB_STATE_PART_GROUP_BASE |
             XCB_XKB_STATE_PART_GROUP_LATCH |
             XCB_XKB_STATE_PART_GROUP_LOCK),
    };

    static const xcb_xkb_select_events_details_t details = {
        .affectNewKeyboard = required_nkn_details,
        .newKeyboardDetails = required_nkn_details,
        .affectState = required_state_details,
        .stateDetails = required_state_details,
    };

    xcb_void_cookie_t cookie =
        xcb_xkb_select_events_aux_checked(conn,
                                          device_id,
                                          required_events,    /* affectWhich */
                                          0,                  /* clear */
                                          0,                  /* selectAll */
                                          required_map_parts, /* affectMap */
                                          required_map_parts, /* map */
                                          &details);          /* details */

    xcb_generic_error_t *error = xcb_request_check(conn, cookie);
    if (error) {
        free(error);
        return -1;
    }

    return 0;
}

static int
update_keymap(struct keyboard *kbd)
{
#ifndef KEYMAP_DUMP
    if (custom_keymap) {
        if (!kbd->keymap) {
            /* Keymap is only parsed the one time, then we just reference it. */
            kbd->keymap = xkb_keymap_ref(custom_keymap);
        }
    } else {
#endif
        const bool update = !!kbd->keymap && !!kbd->state;
        xkb_keymap_unref(kbd->keymap);
        kbd->keymap = xkb_x11_keymap_new_from_device(kbd->ctx, kbd->conn,
                                                     kbd->device_id,
                                                     XKB_KEYMAP_COMPILE_NO_FLAGS);
        if (!kbd->keymap)
            return -1;

        if (update)
            printf("Keymap updated!\n");
#ifndef KEYMAP_DUMP
    }

    if (!use_local_state) {
#endif
        /* Reset state on keymap reset */
        xkb_state_unref(kbd->state);
        kbd->state = xkb_x11_state_new_from_device(kbd->keymap, kbd->conn,
                                                   kbd->device_id);
        if (!kbd->state)
            return -1;
#ifndef KEYMAP_DUMP
    } else {
        /* Ignore state from server and reset only if state if undefined. */
        if (use_events_api) {
            if (!kbd->state_machine) {
                kbd->state_machine = xkb_state_machine_new(
                    kbd->keymap, any_state_options.machine
                );
                if (!kbd->state_machine)
                    return -1;
            }
            if (!kbd->state_events) {
                kbd->state_events = xkb_event_iterator_new(kbd->state_machine);
                if (!kbd->state_events)
                    return -1;
            }
            if (!kbd->state) {
                kbd->state = xkb_state_new(kbd->keymap);
                if (!kbd->state)
                    return -1;
            }
            const int ret = xkb_state_machine_update_controls(
                kbd->state_machine, kbd->state_events,
                kbd_controls_affect, kbd_controls_values
            );
            if (ret)
                return ret;
            const struct xkb_event *event;
            while ((event = xkb_event_iterator_next(kbd->state_events))) {
                xkb_state_update_from_event(kbd->state, event);
            }
        } else {
            kbd->state = xkb_state_new2(kbd->keymap, any_state_options.state);
            if (!kbd->state)
                return -1;
            xkb_state_update_controls(kbd->state,
                                      kbd_controls_affect, kbd_controls_values);
        }
    }
#endif

    return 0;
}

static int
init_kbd(struct keyboard *kbd, xcb_connection_t *conn, uint8_t first_xkb_event,
         int32_t device_id, struct xkb_context *ctx,
         struct xkb_compose_table *compose_table)
{
    int ret;

    kbd->conn = conn;
    kbd->first_xkb_event = first_xkb_event;
    kbd->ctx = ctx;
    kbd->keymap = NULL;
    kbd->state = NULL;
    kbd->compose_state = NULL;
    kbd->device_id = device_id;

    ret = update_keymap(kbd);
    if (ret)
        goto err_out;

#ifdef KEYMAP_DUMP
    /* Dump the keymap */
    char *dump = xkb_keymap_get_as_string2(kbd->keymap, keymap_format,
                                           serialize_flags);
    fprintf(stdout, "%s", dump);
    free(dump);
    terminate = true;
    return 0;
#endif

    if (compose_table)
        kbd->compose_state = xkb_compose_state_new(compose_table,
                                                   XKB_COMPOSE_STATE_NO_FLAGS);

    ret = select_xkb_events_for_device(conn, device_id);
    if (ret)
        goto err_state;

    return 0;

err_state:
    xkb_state_unref(kbd->state);
    xkb_state_machine_unref(kbd->state_machine);
    xkb_event_iterator_destroy(kbd->state_events);
    xkb_compose_state_unref(kbd->compose_state);
    xkb_keymap_unref(kbd->keymap);
err_out:
    return -1;
}

static void
deinit_kbd(struct keyboard *kbd)
{
    xkb_state_unref(kbd->state);
    xkb_state_machine_unref(kbd->state_machine);
    xkb_event_iterator_destroy(kbd->state_events);
    xkb_compose_state_unref(kbd->compose_state);
    xkb_keymap_unref(kbd->keymap);
}

#ifndef KEYMAP_DUMP
static void
process_xkb_event(xcb_generic_event_t *gevent, struct keyboard *kbd)
{
    union xkb_event {
        struct {
            uint8_t response_type;
            uint8_t xkbType;
            uint16_t sequence;
            xcb_timestamp_t time;
            uint8_t deviceID;
        } any;
        xcb_xkb_new_keyboard_notify_event_t new_keyboard_notify;
        xcb_xkb_map_notify_event_t map_notify;
        xcb_xkb_state_notify_event_t state_notify;
    } *event = (union xkb_event *) gevent;

    if (event->any.deviceID != kbd->device_id)
        return;

    /*
     * XkbNewKkdNotify and XkbMapNotify together capture all sorts of keymap
     * updates (e.g. xmodmap, xkbcomp, setxkbmap), with minimal redundent
     * recompilations.
     */
    switch (event->any.xkbType) {
    case XCB_XKB_NEW_KEYBOARD_NOTIFY:
        if (event->new_keyboard_notify.changed & XCB_XKB_NKN_DETAIL_KEYCODES)
            update_keymap(kbd);
        break;

    case XCB_XKB_MAP_NOTIFY:
        update_keymap(kbd);
        break;

    case XCB_XKB_STATE_NOTIFY: {
        if (use_local_state) {
            /* Ignore state update if using a local state machine */
            break;
        }

        const enum xkb_state_component changed =
            xkb_state_update_mask(kbd->state,
                                  event->state_notify.baseMods,
                                  event->state_notify.latchedMods,
                                  event->state_notify.lockedMods,
                                  event->state_notify.baseGroup,
                                  event->state_notify.latchedGroup,
                                  event->state_notify.lockedGroup);
        if (report_state_changes)
            tools_print_state_changes(NULL, kbd->state, changed, print_options);
        break;
    }

    default:
        /* Ignore */
        break;
    }
}

static void
process_event(xcb_generic_event_t *gevent, struct keyboard *kbd)
{
    switch (gevent->response_type) {
    case XCB_KEY_PRESS: {
        xcb_key_press_event_t *event = (xcb_key_press_event_t *) gevent;
        const xkb_keycode_t keycode = event->detail;

        if (use_local_state && use_events_api) {
            /* Run our local state machine with the event API */
            const int ret = xkb_state_machine_update_key(kbd->state_machine,
                                                         kbd->state_events,
                                                         keycode, XKB_KEY_DOWN);
            if (ret) {
                fprintf(stderr, "ERROR: could not update the state machine\n");
                // TODO: better error handling
            } else {
                tools_print_events(NULL, kbd->state, kbd->state_events,
                                   kbd->compose_state, print_options,
                                   report_state_changes);
            }
        } else {
            if (kbd->compose_state) {
                xkb_keysym_t keysym = xkb_state_key_get_one_sym(kbd->state,
                                                                keycode);
                xkb_compose_state_feed(kbd->compose_state, keysym);
            }

            tools_print_keycode_state(NULL, kbd->state, kbd->compose_state,
                                      keycode, XKB_KEY_DOWN,
                                      XKB_CONSUMED_MODE_XKB, print_options);

            if (kbd->compose_state) {
                enum xkb_compose_status status =
                    xkb_compose_state_get_status(kbd->compose_state);
                if (status == XKB_COMPOSE_CANCELLED ||
                    status == XKB_COMPOSE_COMPOSED)
                    xkb_compose_state_reset(kbd->compose_state);
            }
            if (use_local_state) {
                /* Run our local state machine with the legacy API */
                const enum xkb_state_component changed =
                    xkb_state_update_key(kbd->state, keycode, XKB_KEY_DOWN);
                if (changed && report_state_changes)
                    tools_print_state_changes(NULL, kbd->state,
                                              changed, print_options);
            }
        }

        /* Exit on ESC. */
        if (xkb_state_key_get_one_sym(kbd->state, keycode) == XKB_KEY_Escape)
            terminate = true;
        break;
    }
    case XCB_KEY_RELEASE: {
        xcb_key_press_event_t *event = (xcb_key_press_event_t *) gevent;
        const xkb_keycode_t keycode = event->detail;
        if (use_local_state && use_events_api) {
            /* Run our local state machine */
            const int ret = xkb_state_machine_update_key(kbd->state_machine,
                                                         kbd->state_events,
                                                         keycode, XKB_KEY_UP);
            if (ret) {
                fprintf(stderr, "ERROR: could not update the state machine\n");
                // TODO: better error handling
            } else {
                tools_print_events(NULL, kbd->state, kbd->state_events,
                                   kbd->compose_state, print_options,
                                   report_state_changes);
            }
        } else {
            tools_print_keycode_state(NULL, kbd->state, kbd->compose_state,
                                      keycode, XKB_KEY_UP,
                                      XKB_CONSUMED_MODE_XKB, print_options);
            if (use_local_state) {
                /* Run our local state machine with the legacy API */
                const enum xkb_state_component changed =
                    xkb_state_update_key(kbd->state, keycode, XKB_KEY_UP);
                if (changed && report_state_changes)
                    tools_print_state_changes(NULL, kbd->state,
                                              changed, print_options);
            }
        }
        break;
    }
    default:
        if (gevent->response_type == kbd->first_xkb_event)
            process_xkb_event(gevent, kbd);
        break;
    }
}

static int
loop(xcb_connection_t *conn, struct keyboard *kbd)
{
    while (!terminate) {
        xcb_generic_event_t *event;

        switch (xcb_connection_has_error(conn)) {
        case 0:
            break;
        case XCB_CONN_ERROR:
            fprintf(stderr,
                    "Closed connection to X server: connection error\n");
            return -1;
        case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
            fprintf(stderr,
                    "Closed connection to X server: extension not supported\n");
            return -1;
        default:
            fprintf(stderr,
                    "Closed connection to X server: error code %d\n",
                    xcb_connection_has_error(conn));
            return -1;
        }

        event = xcb_wait_for_event(conn);
        if (!event) {
            continue;
        }

        process_event(event, kbd);
        free(event);
    }

    return 0;
}

static int
create_capture_window(xcb_connection_t *conn)
{
    xcb_generic_error_t *error;
    xcb_void_cookie_t cookie;
    xcb_screen_t *screen =
        xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    xcb_window_t window = xcb_generate_id(conn);
    uint32_t values[2] = {
        screen->white_pixel,
        XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE,
    };

    cookie = xcb_create_window_checked(conn, XCB_COPY_FROM_PARENT,
                                       window, screen->root,
                                       10, 10, 100, 100, 1,
                                       XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                       screen->root_visual,
                                       XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
                                       values);
    if ((error = xcb_request_check(conn, cookie)) != NULL) {
        free(error);
        return -1;
    }

    cookie = xcb_map_window_checked(conn, window);
    if ((error = xcb_request_check(conn, cookie)) != NULL) {
        free(error);
        return -1;
    }

    return 0;
}
#endif

static void
usage(FILE *fp, char *progname)
{
        fprintf(fp,
                "Usage: %s [--help] [--verbose]"
#ifndef KEYMAP_DUMP
                " [--uniline] [--multiline] [--local-state] [--keymap FILE ]"
#endif
                " [--format=<format>]"
#ifndef KEYMAP_DUMP
                " [--enable-compose]"
#endif
                "\n",
                progname);
        fprintf(fp,
#ifndef KEYMAP_DUMP
                "    --enable-compose     enable Compose\n"
                "    --local-state        enable local state handling and ignore modifiers/layouts\n"
                "                         state updates from the X11 server\n"
                "    --legacy-state-api   do not use the state event API. It implies --local-state.\n"
                "    --controls [<CONTROLS>]\n"
                "                         use the given keyboard controls; available values are:\n"
                "                         sticky-keys, latch-to-lock and latch-simultaneous.\n"
                "                         It implies --local-state.\n"
                "    --keymap [<FILE>]    use the given keymap instead of the keymap from the\n"
                "                         compositor. It implies --local-state.\n"
                "                         If <FILE> is \"-\" or missing, then load from stdin.\n"
#endif
                "    --format <FORMAT>    use keymap format <FORMAT>\n"
#ifdef KEYMAP_DUMP
                "    --no-pretty          do not pretty-print when serializing a keymap\n"
                "    --drop-unused        disable unused bits serialization\n"
#else
                "    -1, --uniline        enable uniline event output\n"
                "    --multiline          enable multiline event output\n"
                "    --no-state-report    do not report changes to the state\n"
#endif
                "    --verbose            enable verbose debugging output\n"
                "    --help               display this help and exit\n"
        );
}

int
main(int argc, char *argv[])
{
    int ret;
    bool verbose = false;
    xcb_connection_t *conn;
    uint8_t first_xkb_event;
    int32_t core_kbd_device_id;
    struct xkb_context *ctx;
    struct keyboard core_kbd = {0};
    struct xkb_compose_table *compose_table = NULL;

#ifndef KEYMAP_DUMP
    bool with_keymap_file = false;
    enum xkb_keymap_format keymap_format = DEFAULT_INPUT_KEYMAP_FORMAT;
    const char *keymap_path = NULL;

    /* Only used for state options */
    core_kbd.ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!core_kbd.ctx) {
        ret = -1;
        fprintf(stderr, "Couldn't create xkb context\n");
        goto err_out;
    }
    any_state_options.state = xkb_state_options_new(core_kbd.ctx);
    any_state_options.machine = xkb_state_machine_options_new(core_kbd.ctx);
    xkb_context_unref(core_kbd.ctx);
    core_kbd.ctx = NULL;
    if (!any_state_options.state || !any_state_options.machine) {
        ret = -1;
        fprintf(stderr, "Couldn't create xkb state options\n");
        goto err_out;
    }

    bool with_compose = false;
    const char *locale;
#endif

    enum options {
        OPT_VERBOSE,
        OPT_UNILINE,
        OPT_MULTILINE,
        OPT_NO_STATE_REPORT,
        OPT_COMPOSE,
        OPT_LOCAL_STATE,
        OPT_LEGACY_STATE_API,
        OPT_CONTROLS,
        OPT_KEYMAP_FORMAT,
        OPT_KEYMAP_NO_PRETTY,
        OPT_KEYMAP_DROP_UNUSED,
        OPT_KEYMAP,
    };
    static struct option opts[] = {
        {"help",                 no_argument,            0, 'h'},
        {"verbose",              no_argument,            0, OPT_VERBOSE},
        {"format",               required_argument,      0, OPT_KEYMAP_FORMAT},
#ifdef KEYMAP_DUMP
        {"no-pretty",            no_argument,            0, OPT_KEYMAP_NO_PRETTY},
        {"drop-unused",          no_argument,            0, OPT_KEYMAP_DROP_UNUSED},
#else
        {"uniline",              no_argument,            0, OPT_UNILINE},
        {"multiline",            no_argument,            0, OPT_MULTILINE},
        {"no-state-report",      no_argument,            0, OPT_NO_STATE_REPORT},
        {"enable-compose",       no_argument,            0, OPT_COMPOSE},
        {"local-state",          no_argument,            0, OPT_LOCAL_STATE},
        {"legacy-state-api",     no_argument,            0, OPT_LEGACY_STATE_API},
        {"controls",             required_argument,      0, OPT_CONTROLS},
        {"keymap",               optional_argument,      0, OPT_KEYMAP},
#endif
        {0, 0, 0, 0},
    };

    setlocale(LC_ALL, "");

    while (1) {
        int opt;
        int option_index = 0;

        opt = getopt_long(argc, argv, "*1h", opts, &option_index);
        if (opt == -1)
            break;

        switch (opt) {
        case OPT_VERBOSE:
            verbose = true;
            break;
        case OPT_KEYMAP_FORMAT:
            /*
             * This is either:
             * - the input format for the interactive tool
             * - the output format for the dump tool
             */
            keymap_format = xkb_keymap_parse_format(optarg);
            if (!keymap_format) {
                fprintf(stderr, "ERROR: invalid --format \"%s\"\n", optarg);
                goto invalid_usage;
            }
            break;
#ifdef KEYMAP_DUMP
        case OPT_KEYMAP_NO_PRETTY:
            serialize_flags &= ~XKB_KEYMAP_SERIALIZE_PRETTY;
            break;
        case OPT_KEYMAP_DROP_UNUSED:
            serialize_flags &= ~XKB_KEYMAP_SERIALIZE_KEEP_UNUSED;
            break;
#else
        case OPT_COMPOSE:
            with_compose = true;
            break;
        case OPT_LOCAL_STATE:
local_state:
            use_local_state = true;
            break;
        case OPT_LEGACY_STATE_API:
            use_events_api = false;
            goto local_state;
        case OPT_CONTROLS:
            if (!tools_parse_controls(optarg, &any_state_options,
                                      &kbd_controls_affect,
                                      &kbd_controls_values)) {
                goto invalid_usage;
            }
            /* --local-state is implied */
            goto local_state;
        case OPT_KEYMAP:
            with_keymap_file = true;
            /* Optional arguments require `=`, but we want to make this
             * requirement optional too, so that both `--keymap=xxx` and
             * `--keymap xxx` work. */
            if (!optarg && argv[optind] &&
                (argv[optind][0] != '-' || strcmp(argv[optind], "-") == 0 )) {
                keymap_path = argv[optind++];
            } else {
                keymap_path = optarg;
            }
            /* --local-state is implied */
            goto local_state;
        case '1':
        case OPT_UNILINE:
            print_options |= PRINT_UNILINE;
            break;
        case '*':
        case OPT_MULTILINE:
            print_options &= ~PRINT_UNILINE;
            break;
        case OPT_NO_STATE_REPORT:
            report_state_changes = false;
            break;
#endif
        case 'h':
            usage(stdout, argv[0]);
            ret = EXIT_SUCCESS;
            goto error_parse_args;
        default:
invalid_usage:
            usage(stderr, argv[0]);
            ret = EXIT_INVALID_USAGE;
            goto error_parse_args;
        }
    }

#ifndef KEYMAP_DUMP
    if (optind < argc && !isempty(argv[optind])) {
        /* Some positional arguments left: use as a keymap input */
        if (keymap_path)
            goto too_much_arguments;
        keymap_path = argv[optind++];
        if (optind < argc) {
            /* Further positional arguments is an error */
too_much_arguments:
            fprintf(stderr, "ERROR: Too many positional arguments\n");
            goto invalid_usage;
        }
        with_keymap_file = true;
    } else if (is_pipe_or_regular_file(STDIN_FILENO) && !with_keymap_file) {
        /* No positional argument: piping detected */
        with_keymap_file = true;
    }

    if (with_keymap_file) {
        /* --local-state is implied with custom keymap */
        use_local_state = true;
    }

    if (isempty(keymap_path) || strcmp(keymap_path, "-") == 0)
        keymap_path = NULL;
#endif

    conn = xcb_connect(NULL, NULL);
    if (!conn || xcb_connection_has_error(conn)) {
        fprintf(stderr, "Couldn't connect to X server: error code %d\n",
                conn ? xcb_connection_has_error(conn) : -1);
        ret = -1;
        goto err_out;
    }

    ret = xkb_x11_setup_xkb_extension(conn,
                                      XKB_X11_MIN_MAJOR_XKB_VERSION,
                                      XKB_X11_MIN_MINOR_XKB_VERSION,
                                      XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
                                      NULL, NULL, &first_xkb_event, NULL);
    if (!ret) {
        fprintf(stderr, "Couldn't setup XKB extension\n");
        goto err_conn;
    }

    ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx) {
        ret = -1;
        fprintf(stderr, "Couldn't create xkb context\n");
        goto err_conn;
    }

    if (verbose)
        tools_enable_verbose_logging(ctx);

#ifndef KEYMAP_DUMP
    if (with_keymap_file) {
        FILE *file = NULL;
        if (keymap_path) {
            /* Read from regular file */
            file = fopen(keymap_path, "rb");
        } else {
            /* Read from stdin */
            file = tools_read_stdin();
        }
        if (!file) {
            fprintf(stderr, "ERROR: Failed to open keymap file \"%s\": %s\n",
                    keymap_path ? keymap_path : "stdin", strerror(errno));
            xkb_context_unref(ctx);
            goto err_out;
        }
        custom_keymap = xkb_keymap_new_from_file(ctx, file,
                                                 keymap_format,
                                                 XKB_KEYMAP_COMPILE_NO_FLAGS);
    }

    if (with_compose) {
        locale = setlocale(LC_CTYPE, NULL);
        compose_table =
            xkb_compose_table_new_from_locale(ctx, locale,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
        if (!compose_table) {
            fprintf(stderr, "Couldn't create compose from locale\n");
            goto err_compose;
        }
    } else {
        compose_table = NULL;
    }
#endif

    core_kbd_device_id = xkb_x11_get_core_keyboard_device_id(conn);
    if (core_kbd_device_id == -1) {
        ret = -1;
        fprintf(stderr, "Couldn't find core keyboard device\n");
        goto err_ctx;
    }

    ret = init_kbd(&core_kbd, conn, first_xkb_event, core_kbd_device_id,
                   ctx, compose_table);
    if (ret) {
        fprintf(stderr, "Couldn't initialize core keyboard device\n");
        goto err_ctx;
    }

#ifndef KEYMAP_DUMP
    ret = create_capture_window(conn);
    if (ret) {
        fprintf(stderr, "Couldn't create a capture window\n");
        goto err_window;
    }

    tools_disable_stdin_echo();
    ret = loop(conn, &core_kbd);
    tools_enable_stdin_echo();

err_window:
    xkb_compose_table_unref(compose_table);
err_compose:
#endif
    deinit_kbd(&core_kbd);
err_ctx:
    xkb_context_unref(ctx);
err_conn:
    xcb_disconnect(conn);
err_out:
#ifndef KEYMAP_DUMP
    xkb_keymap_unref(custom_keymap);
#endif
    ret = (ret >= 0) ? EXIT_SUCCESS : EXIT_FAILURE;
error_parse_args:
#ifndef KEYMAP_DUMP
    xkb_any_state_options_destroy(&any_state_options);
#endif
    exit(ret);
}
