/*
 * Copyright © 2012 Collabora, Ltd.
 * Copyright © 2013 Ran Benita <ran234@gmail.com>
 * Copyright © 2016 Daniel Stone <daniel@fooishbar.org>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcommon/xkbcommon-compose.h"
#include "src/utils.h"
#include "src/keymap-formats.h"
#include "tools-common.h"

#include <wayland-client.h>
#include "wayland-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-v1-client-protocol.h"
#include <wayland-util.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* Offset between evdev keycodes (where KEY_ESCAPE is 1), and the evdev XKB
 * keycode set (where ESC is 9). */
#define EVDEV_OFFSET 8

struct interactive_dpy {
    struct wl_display *dpy;
    struct wl_compositor *compositor;
    struct xdg_wm_base *shell;
    struct wl_shm *shm;
    uint32_t shm_format;
    struct wl_buffer *buf;
    struct zxdg_decoration_manager_v1 *decoration_manager;
    struct zxdg_toplevel_decoration_v1 *decoration;

    struct xkb_context *ctx;
    struct xkb_compose_table *compose_table;

    struct wl_surface *wl_surf;
    struct xdg_surface *xdg_surf;
    struct xdg_toplevel *xdg_top;

    struct wl_list seats;
};

struct interactive_seat {
    struct interactive_dpy *inter;

    struct wl_seat *wl_seat;
    struct wl_keyboard *wl_kbd;
    struct wl_pointer *wl_pointer;
    uint32_t version; /* ... of wl_seat */
    uint32_t global_name; /* an ID of sorts */
    char *name_str; /* a descriptor */

    struct xkb_keymap *keymap;
    struct xkb_state *state;
    struct xkb_state_machine *state_machine;
    struct xkb_event_iterator *events;
    struct xkb_compose_state *compose_state;

    struct wl_list link;
};

static bool terminate = false;
static enum xkb_keymap_format keymap_input_format = DEFAULT_INPUT_KEYMAP_FORMAT;
#ifdef KEYMAP_DUMP
static_assert(DEFAULT_OUTPUT_KEYMAP_FORMAT == XKB_KEYMAP_USE_ORIGINAL_FORMAT,
              "Out of sync usage()");
static enum xkb_keymap_format keymap_output_format = DEFAULT_OUTPUT_KEYMAP_FORMAT;
static enum xkb_keymap_serialize_flags serialize_flags =
    (enum xkb_keymap_serialize_flags) DEFAULT_KEYMAP_SERIALIZE_FLAGS;
static bool dump_raw_keymap;
#else
static bool use_events_api = true;
static enum xkb_consumed_mode consumed_mode = XKB_CONSUMED_MODE_XKB;
static enum print_state_options print_options = DEFAULT_PRINT_OPTIONS;
static bool report_state_changes = true;
static bool use_local_state = false;
static struct xkb_state_machine_options * state_machine_options = NULL;
static enum xkb_keyboard_controls kbd_controls_affect = XKB_KEYBOARD_CONTROL_NONE;
static enum xkb_keyboard_controls kbd_controls_values = XKB_KEYBOARD_CONTROL_NONE;
static const char *raw_modifiers_mapping = NULL;
static const char *raw_shortcuts_mask = NULL;
static struct xkb_keymap *custom_keymap = NULL;
#endif

#ifndef KEYMAP_DUMP
static void
surface_configure(void *data, struct xdg_surface *surface,
                  uint32_t serial)
{
    struct interactive_dpy *inter = data;

    xdg_surface_ack_configure(inter->xdg_surf, serial);
    wl_surface_commit(inter->wl_surf);
}

static const struct xdg_surface_listener surface_listener = {
    surface_configure,
};

#ifdef HAVE_MKOSTEMP
static int
create_tmpfile_cloexec(char *tmpname)
{
    int fd = mkostemp(tmpname, O_CLOEXEC);
    if (fd >= 0)
        unlink(tmpname);
    return fd;
}
#else
/* The following utility functions are taken from Weston's
 * shared/os-compatibility.c. */
static int
os_fd_set_cloexec(int fd)
{
    long flags;

    if (fd == -1)
        return -1;

    flags = fcntl(fd, F_GETFD);
    if (flags == -1)
        return -1;

    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
        return -1;

    return 0;
}

static int
set_cloexec_or_close(int fd)
{
    if (os_fd_set_cloexec(fd) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int
create_tmpfile_cloexec(char *tmpname)
{
    int fd = mkstemp(tmpname);
    if (fd >= 0) {
        fd = set_cloexec_or_close(fd);
        unlink(tmpname);
    }
    return fd;
}
#endif

static int
os_resize_anonymous_file(int fd, off_t size)
{
    int ret;
#ifdef HAVE_POSIX_FALLOCATE
    ret = posix_fallocate(fd, 0, size);
    if (ret == 0)
        return 0;
    /*
     * Filesystems that do support fallocate will return EINVAL
     * or EOPNOTSUPP, fallback to ftruncate() then.
     */
    if (ret != EINVAL && ret != EOPNOTSUPP)
        return ret;
#endif
    ret = ftruncate(fd, size);
    if (ret != 0)
        return errno;
    return 0;
}

/*
 * Create a new, unique, anonymous file of the given size, and
 * return the file descriptor for it. The file descriptor is set
 * CLOEXEC. The file is immediately suitable for mmap()'ing
 * the given size at offset zero.
 *
 * The file should not have a permanent backing store like a disk,
 * but may have if XDG_RUNTIME_DIR is not properly implemented in OS.
 *
 * The file name is deleted from the file system.
 *
 * The file is suitable for buffer sharing between processes by
 * transmitting the file descriptor over Unix sockets using the
 * SCM_RIGHTS methods.
 *
 * If the C library implements posix_fallocate(), it is used to
 * guarantee that disk space is available for the file at the
 * given size. If disk space is insufficent, errno is set to ENOSPC.
 * If posix_fallocate() is not supported, program will fallback
 * to ftruncate() instead.
 */
static int
os_create_anonymous_file(off_t size)
{
    static const char template[] = "/weston-shared-XXXXXX";
    const char *path;
    char *name;
    int fd;
    int ret;

    path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        errno = ENOENT;
        return -1;
    }

    const size_t len = strlen(path);
    name = malloc(len + sizeof(template));
    if (!name)
        return -1;

    memcpy(name, path, len);
    memcpy(name + len, template, sizeof(template));

    fd = create_tmpfile_cloexec(name);

    free(name);

    if (fd < 0)
        return -1;

    ret = os_resize_anonymous_file(fd, size);
    if (ret != 0) {
        close(fd);
        errno = ret;
        return -1;
    }

    return fd;
}

static void
buffer_release(void *data, struct wl_buffer *buffer)
{
    struct interactive_dpy *inter = data;

    wl_buffer_destroy(buffer);
    inter->buf = NULL;
}

static const struct wl_buffer_listener buffer_listener = {
    buffer_release
};

static void
buffer_create(struct interactive_dpy *inter, uint32_t width, uint32_t height)
{
    struct wl_shm_pool *pool;
    struct wl_region *opaque;
    uint32_t stride;
    size_t size;
    void *map;
    int fd;

    switch (inter->shm_format) {
    case WL_SHM_FORMAT_ARGB8888:
    case WL_SHM_FORMAT_XRGB8888:
    case WL_SHM_FORMAT_ABGR8888:
    case WL_SHM_FORMAT_XBGR8888:
        stride = width * 4;
        break;
    case WL_SHM_FORMAT_RGB565:
    case WL_SHM_FORMAT_BGR565:
        stride = width * 2;
        break;
    default:
        fprintf(stderr, "Unsupported SHM format %"PRIu32"\n", inter->shm_format);
        exit(EXIT_FAILURE);
    }

    size = (size_t)(stride) * height;

    const off_t offset = (off_t) size;
    if ((size_t) offset != size) {
        fprintf(stderr, "Couldn't create surface buffer (buffer size error)\n");
        exit(EXIT_FAILURE);
    }

    fd = os_create_anonymous_file(offset);
    if (fd < 0) {
        fprintf(stderr, "Couldn't create surface buffer (buffer file error)\n");
        exit(EXIT_FAILURE);
    }

    map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        fprintf(stderr, "Couldn't mmap surface buffer\n");
        exit(EXIT_FAILURE);
    }
    memset(map, 0xff, size);
    munmap(map, size);

    if (size > INT32_MAX) {
        fprintf(stderr, "Couldn't create surface pool\n");
        exit(EXIT_FAILURE);
    }
    pool = wl_shm_create_pool(inter->shm, fd, (int32_t) size);

    if (width > INT32_MAX || height > INT32_MAX || stride > INT32_MAX) {
        fprintf(stderr, "Couldn't create surface pool buffer\n");
        exit(EXIT_FAILURE);
    }
    const int32_t iwidth = (int32_t) width;
    const int32_t iheight = (int32_t) height;
    const int32_t istride = (int32_t) stride;

    if (inter->buf)
        wl_buffer_destroy(inter->buf);

    inter->buf = wl_shm_pool_create_buffer(pool, 0, iwidth, iheight, istride,
                                           inter->shm_format);
    wl_buffer_add_listener(inter->buf, &buffer_listener, inter);

    wl_surface_attach(inter->wl_surf, inter->buf, 0, 0);
    wl_surface_damage(inter->wl_surf, 0, 0, iwidth, iheight);

    opaque = wl_compositor_create_region(inter->compositor);
    wl_region_add(opaque, 0, 0, iwidth, iheight);
    wl_surface_set_opaque_region(inter->wl_surf, opaque);
    wl_region_destroy(opaque);

    wl_shm_pool_destroy(pool);
    close(fd);
}

static void
toplevel_configure(void *data, struct xdg_toplevel *toplevel,
                   int32_t width, int32_t height, struct wl_array *states)
{
    struct interactive_dpy *inter = data;

    if (width == 0)
        width = 400;
    if (height == 0)
        height = 400;

    buffer_create(inter, width, height);
}

static void
toplevel_close(void *data, struct xdg_toplevel *toplevel)
{
    terminate = true;
}

static const struct xdg_toplevel_listener toplevel_listener = {
    toplevel_configure,
    toplevel_close
};

static void surface_create(struct interactive_dpy *inter)
{
    inter->wl_surf = wl_compositor_create_surface(inter->compositor);
    inter->xdg_surf = xdg_wm_base_get_xdg_surface(inter->shell, inter->wl_surf);
    xdg_surface_add_listener(inter->xdg_surf, &surface_listener, inter);

    /* Create a window only for the interactive tool */
    inter->xdg_top = xdg_surface_get_toplevel(inter->xdg_surf);
    xdg_toplevel_add_listener(inter->xdg_top, &toplevel_listener, inter);
    xdg_toplevel_set_title(inter->xdg_top, "xkbcommon event tester");
    xdg_toplevel_set_app_id(inter->xdg_top,
                            "org.xkbcommon.test.interactive-wayland");
    if (inter->decoration_manager) {
        inter->decoration =
            zxdg_decoration_manager_v1_get_toplevel_decoration(
                inter->decoration_manager, inter->xdg_top
            );
            zxdg_toplevel_decoration_v1_set_mode(
                inter->decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
            );
    }

    wl_surface_commit(inter->wl_surf);
}
#endif

static void
shell_ping(void *data, struct xdg_wm_base *shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener shell_listener = {
    shell_ping
};

static void
kbd_keymap(void *data, struct wl_keyboard *wl_kbd, uint32_t format,
           int fd, uint32_t size)
{
    struct interactive_seat *seat = data;
#ifndef KEYMAP_DUMP
    if (custom_keymap) {
        /* Custom keymap: ignore keymap from the server */
        close(fd);
        if (!seat->keymap)
            seat->keymap = xkb_keymap_ref(custom_keymap);
    } else {
#endif
        void *buf = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
        if (buf == MAP_FAILED) {
            fprintf(stderr, "ERROR: Failed to mmap keymap: %d\n", errno);
            close(fd);
            return;
        }
#ifdef KEYMAP_DUMP
        /* We do not want to be interactive, so stop at next loop */
        terminate = true;
        if (dump_raw_keymap) {
            /* Dump the raw keymap */
            fprintf(stdout, "%s", (char *)buf);
            munmap(buf, size);
            close(fd);
            /* Do not go further */
            return;
        }
#endif
        xkb_keymap_unref(seat->keymap);
        seat->keymap = xkb_keymap_new_from_buffer(seat->inter->ctx,
                                                  buf, size - 1,
                                                  keymap_input_format,
                                                  XKB_KEYMAP_COMPILE_NO_FLAGS);
        munmap(buf, size);
        close(fd);
#ifndef KEYMAP_DUMP
    }
#endif
    if (!seat->keymap) {
        fprintf(stderr, "ERROR: Failed to compile keymap!\n");
        return;
    }

#ifdef KEYMAP_DUMP
    /* Dump the reformatted keymap */
    char *dump = xkb_keymap_get_as_string2(seat->keymap, keymap_output_format,
                                           serialize_flags);
    fprintf(stdout, "%s", dump);
    free(dump);
#else
    /* Reset the state, except if already set and not using a local state */
    if (!seat->state || !use_local_state) {
        xkb_state_unref(seat->state);
        seat->state = xkb_state_new(seat->keymap);
        if (!seat->state) {
            fprintf(stderr, "ERROR: Failed to create XKB state!\n");
        } else if (use_local_state && !use_events_api) {
            xkb_state_update_controls(seat->state,
                                      kbd_controls_affect, kbd_controls_values);
        }
    }
    if (use_local_state && use_events_api) {
        if (!seat->state_machine) {
            if (raw_modifiers_mapping) {
                /* No race condition when using wl_display_dispatch() */
                xkb_state_machine_options_mods_set_mapping(
                    state_machine_options, 0, 0
                );
                if (!tools_parse_modifiers_mappings(raw_modifiers_mapping, seat->keymap,
                                                    state_machine_options)) {
                    fprintf(stderr,
                            "ERROR: Failed to parse modifiers mapping: \"%s\"\n",
                            raw_modifiers_mapping);
                }
            }
            if (raw_shortcuts_mask) {
                /* No race condition when using wl_display_dispatch() */
                xkb_state_machine_options_shortcuts_update_mods(
                    state_machine_options, XKB_MOD_ALL, 0
                );
                if (!tools_parse_shortcuts_mask(raw_shortcuts_mask, seat->keymap,
                                                state_machine_options)) {
                    fprintf(stderr,
                            "ERROR: Failed to parse shortcuts mask: \"%s\"\n",
                            raw_shortcuts_mask);
                }
            }

            seat->state_machine =
                xkb_state_machine_new(seat->keymap, state_machine_options);
            if (!seat->state_machine)
                fprintf(stderr, "ERROR: Failed to create local XKB state!\n");
        }
        if (!seat->events) {
            /* Initialize the events queue */
            seat->events = xkb_event_iterator_new(seat->inter->ctx,
                                                  XKB_EVENT_ITERATOR_NO_FLAGS);
            if (seat->events) {
                xkb_state_machine_update_controls(seat->state_machine,
                                                  seat->events,
                                                  kbd_controls_affect,
                                                  kbd_controls_values);
                const struct xkb_event *event;
                while ((event = xkb_event_iterator_next(seat->events))) {
                    xkb_state_update_from_event(seat->state, event);
                }
            } else {
                fprintf(stderr, "ERROR: Failed to create XKB event queue!\n");
            }
        }
    }
#endif
}

static void
kbd_enter(void *data, struct wl_keyboard *wl_kbd, uint32_t serial,
          struct wl_surface *surf, struct wl_array *keys)
{
}

static void
kbd_leave(void *data, struct wl_keyboard *wl_kbd, uint32_t serial,
          struct wl_surface *surf)
{
}

static void
kbd_key(void *data, struct wl_keyboard *wl_kbd, uint32_t serial, uint32_t time,
        uint32_t key, uint32_t state)
{
#ifndef KEYMAP_DUMP
    struct interactive_seat *seat = data;
    xkb_keycode_t keycode = key + EVDEV_OFFSET;

    char * const prefix = asprintf_safe("%s: ", seat->name_str);
    const enum xkb_key_direction direction =
        (state == WL_KEYBOARD_KEY_STATE_RELEASED) ? XKB_KEY_UP : XKB_KEY_DOWN;

    if (use_local_state && use_events_api) {
        /* Run our local state machine with the state event API */
        const int ret = xkb_state_machine_update_key(seat->state_machine,
                                                     seat->events,
                                                     keycode, direction);
        if (ret) {
            fprintf(stderr, "ERROR: could not update the state machine\n");
            // TODO: better error handling
        } else {
            tools_print_events(prefix, seat->state, seat->events,
                               seat->compose_state, consumed_mode,
                               print_options, report_state_changes);
        }
    } else {
        if (seat->compose_state && direction == XKB_KEY_DOWN) {
            const xkb_keysym_t keysym =
                xkb_state_key_get_one_sym(seat->state, keycode);
            xkb_compose_state_feed(seat->compose_state, keysym);
        }
        tools_print_keycode_state(prefix, seat->state, seat->compose_state,
                                  keycode, direction, consumed_mode,
                                  print_options);
        if (seat->compose_state) {
            const enum xkb_compose_status status =
                xkb_compose_state_get_status(seat->compose_state);
            if (status == XKB_COMPOSE_CANCELLED || status == XKB_COMPOSE_COMPOSED)
                xkb_compose_state_reset(seat->compose_state);
        }
        if (use_local_state) {
            /* Run our local state machine with the legacy state API */
            const enum xkb_state_component changed =
                xkb_state_update_key(seat->state, keycode,
                                     (state == WL_KEYBOARD_KEY_STATE_RELEASED
                                             ? XKB_KEY_UP
                                             : XKB_KEY_DOWN));
            if (changed && report_state_changes)
                 tools_print_state_changes(prefix, seat->state,
                                           changed, print_options);
        }
    }

    free(prefix);

    /* Exit on ESC. */
    if (xkb_state_key_get_one_sym(seat->state, keycode) == XKB_KEY_Escape &&
        state != WL_KEYBOARD_KEY_STATE_PRESSED)
        terminate = true;
#endif
}

static void
kbd_modifiers(void *data, struct wl_keyboard *wl_kbd, uint32_t serial,
              uint32_t mods_depressed, uint32_t mods_latched,
              uint32_t mods_locked, uint32_t group)
{
#ifndef KEYMAP_DUMP
    if (use_local_state) {
        /* Ignore state update if using a local state machine */
        return;
    }

    struct interactive_seat *seat = data;

    const enum xkb_state_component changed = xkb_state_update_mask(
        seat->state, mods_depressed, mods_latched, mods_locked, 0, 0, group
    );
    char * const prefix = asprintf_safe("%s: ", seat->name_str);
    if (report_state_changes)
        tools_print_state_changes(prefix, seat->state, changed, print_options);
    free(prefix);
#endif
}

static void
kbd_repeat_info(void *data, struct wl_keyboard *wl_kbd, int32_t rate,
                int32_t delay)
{
}

static const struct wl_keyboard_listener kbd_listener = {
    kbd_keymap,
    kbd_enter,
    kbd_leave,
    kbd_key,
    kbd_modifiers,
    kbd_repeat_info
};

static void
pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
              struct wl_surface *surf, wl_fixed_t fsx, wl_fixed_t fsy)
{
}

static void
pointer_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
              struct wl_surface *surf)
{
}

static void
pointer_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time,
               wl_fixed_t fsx, wl_fixed_t fsy)
{
}

static void
pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
               uint32_t time, uint32_t button, uint32_t state)
{
    struct interactive_seat *seat = data;

    xdg_toplevel_move(seat->inter->xdg_top, seat->wl_seat, serial);
}

static void
pointer_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time,
             uint32_t axis, wl_fixed_t value)
{
}

static void
pointer_frame(void *data, struct wl_pointer *wl_pointer)
{
}

static void
pointer_axis_source(void *data, struct wl_pointer *wl_pointer, uint32_t source)
{
}

static void
pointer_axis_stop(void *data, struct wl_pointer *wl_pointer, uint32_t time,
                  uint32_t axis)
{
}

static void
pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer, uint32_t time,
                      int32_t discrete)
{
}

static const struct wl_pointer_listener pointer_listener = {
    pointer_enter,
    pointer_leave,
    pointer_motion,
    pointer_button,
    pointer_axis,
    pointer_frame,
    pointer_axis_source,
    pointer_axis_stop,
    pointer_axis_discrete
};

static void
seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t caps)
{
    struct interactive_seat *seat = data;

    if (!seat->wl_kbd && (caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        seat->wl_kbd = wl_seat_get_keyboard(seat->wl_seat);
        wl_keyboard_add_listener(seat->wl_kbd, &kbd_listener, seat);
    }
    else if (seat->wl_kbd && !(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        if (seat->version >= WL_SEAT_RELEASE_SINCE_VERSION)
            wl_keyboard_release(seat->wl_kbd);
        else
            wl_keyboard_destroy(seat->wl_kbd);

        xkb_event_iterator_destroy(seat->events);
        xkb_state_unref(seat->state);
        xkb_state_machine_unref(seat->state_machine);
        xkb_keymap_unref(seat->keymap);
        xkb_compose_state_unref(seat->compose_state);

        seat->events = NULL;
        seat->state = NULL;
        seat->state_machine = NULL;
        seat->compose_state = NULL;
        seat->keymap = NULL;
        seat->wl_kbd = NULL;
    }

    if (!seat->wl_pointer && (caps & WL_SEAT_CAPABILITY_POINTER)) {
        seat->wl_pointer = wl_seat_get_pointer(seat->wl_seat);
        wl_pointer_add_listener(seat->wl_pointer, &pointer_listener,
                                seat);
    }
    else if (seat->wl_pointer && !(caps & WL_SEAT_CAPABILITY_POINTER)) {
        if (seat->version >= WL_SEAT_RELEASE_SINCE_VERSION)
            wl_pointer_release(seat->wl_pointer);
        else
            wl_pointer_destroy(seat->wl_pointer);
        seat->wl_pointer = NULL;
    }
}

static void
seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{
    struct interactive_seat *seat = data;

    free(seat->name_str);
    seat->name_str = strdup(name);
}

static const struct wl_seat_listener seat_listener = {
    seat_capabilities,
    seat_name
};

static void
seat_create(struct interactive_dpy *inter, struct wl_registry *registry,
            uint32_t name, uint32_t version)
{
    int ret;
    struct interactive_seat *seat = calloc(1, sizeof(*seat));

    seat->global_name = name;
    seat->inter = inter;
    seat->wl_seat = wl_registry_bind(registry, name, &wl_seat_interface,
                                     MIN(version, 5));
    wl_seat_add_listener(seat->wl_seat, &seat_listener, seat);
    if (seat->inter->compose_table) {
        seat->compose_state = xkb_compose_state_new(seat->inter->compose_table,
                                                    XKB_COMPOSE_STATE_NO_FLAGS);
    }
    ret = asprintf(&seat->name_str, "seat:%"PRIu32,
                   wl_proxy_get_id((struct wl_proxy *) seat->wl_seat));
    assert(ret >= 0);
    wl_list_insert(&inter->seats, &seat->link);
}

static void
seat_destroy(struct interactive_seat *seat)
{
    if (seat->wl_kbd) {
        if (seat->version >= WL_SEAT_RELEASE_SINCE_VERSION)
            wl_keyboard_release(seat->wl_kbd);
        else
            wl_keyboard_destroy(seat->wl_kbd);

        xkb_event_iterator_destroy(seat->events);
        xkb_state_machine_unref(seat->state_machine);
        xkb_state_unref(seat->state);
        xkb_compose_state_unref(seat->compose_state);
        xkb_keymap_unref(seat->keymap);
    }

    if (seat->wl_pointer) {
        if (seat->version >= WL_SEAT_RELEASE_SINCE_VERSION)
            wl_pointer_release(seat->wl_pointer);
        else
            wl_pointer_destroy(seat->wl_pointer);
    }

    if (seat->version >= WL_SEAT_RELEASE_SINCE_VERSION)
        wl_seat_release(seat->wl_seat);
    else
        wl_seat_destroy(seat->wl_seat);

    free(seat->name_str);
    wl_list_remove(&seat->link);
    free(seat);
}

static void
registry_global(void *data, struct wl_registry *registry, uint32_t name,
                const char *interface, uint32_t version)
{
    struct interactive_dpy *inter = data;

    if (strcmp(interface, "wl_seat") == 0) {
        seat_create(inter, registry, name, version);
    }
    else if (strcmp(interface, "xdg_wm_base") == 0) {
        inter->shell = wl_registry_bind(registry, name,
                                        &xdg_wm_base_interface,
                                        MIN(version, 2));
        xdg_wm_base_add_listener(inter->shell, &shell_listener, inter);
    }
    else if (strcmp(interface, "wl_compositor") == 0) {
        inter->compositor = wl_registry_bind(registry, name,
                                             &wl_compositor_interface,
                                             MIN(version, 1));
    }
    else if (strcmp(interface, "wl_shm") == 0) {
        inter->shm = wl_registry_bind(registry, name, &wl_shm_interface,
                                      MIN(version, 1));
    }
#ifndef KEYMAP_DUMP
    else if (strcmp(interface, "zxdg_decoration_manager_v1") == 0) {
        inter->decoration_manager = wl_registry_bind(
            registry, name, &zxdg_decoration_manager_v1_interface,
            MIN(version, 1)
        );
    }
#endif
}

static void
registry_delete(void *data, struct wl_registry *registry, uint32_t name)
{
    struct interactive_dpy *inter = data;
    struct interactive_seat *seat, *tmp;

    wl_list_for_each_safe(seat, tmp, &inter->seats, link) {
        if (seat->global_name != name)
            continue;

        seat_destroy(seat);
    }
}

static const struct wl_registry_listener registry_listener = {
    registry_global,
    registry_delete
};

static void
dpy_disconnect(struct interactive_dpy *inter)
{
    struct interactive_seat *seat, *tmp;

    wl_list_for_each_safe(seat, tmp, &inter->seats, link)
        seat_destroy(seat);

    if (inter->decoration)
        zxdg_toplevel_decoration_v1_destroy(inter->decoration);
    if (inter->decoration_manager)
        zxdg_decoration_manager_v1_destroy(inter->decoration_manager);
    if (inter->xdg_top)
        xdg_toplevel_destroy(inter->xdg_top);
    if (inter->xdg_surf)
        xdg_surface_destroy(inter->xdg_surf);
    if (inter->wl_surf)
        wl_surface_destroy(inter->wl_surf);
    if (inter->shell)
        xdg_wm_base_destroy(inter->shell);
    if (inter->compositor)
        wl_compositor_destroy(inter->compositor);
    if (inter->shm)
        wl_shm_destroy(inter->shm);
    if (inter->buf)
        wl_buffer_destroy(inter->buf);

    /* Do one last roundtrip to try to destroy our wl_buffer. */
    wl_display_roundtrip(inter->dpy);

    xkb_context_unref(inter->ctx);
    wl_display_disconnect(inter->dpy);
}

static void
usage(FILE *fp, char *progname)
{
        fprintf(fp,
                "Usage: %s [--help] [--verbose]"
#ifdef KEYMAP_DUMP
                " [--no-pretty] [--drop-unused] [--raw] [--input-format]"
                " [--output-format] [--format] [--no-pretty] [--drop-unused]"
#else
                " [--uniline] [--multiline] [--consumed-mode={xkb|gtk}]"
                " [--no-state-report] [--format] [--enable-compose]"
                " [--local-state] [--legacy-state-api true|false]"
                " [--controls CONTROLS] [--modifiers-mapping MAPPING]"
                " [--shortcuts-mask MASK] [--shortcuts-mapping]"
                " [--keymap FILE]"
#endif
                "\n",
                progname);
        fprintf(fp,
#ifdef KEYMAP_DUMP
                "    --input-format <FORMAT>     use input keymap format FORMAT (default: '%s')\n"
                "    --output-format <FORMAT>    use output keymap format FORMAT (default: same as input)\n"
                "    --format <FORMAT>           keymap format to use for both input and output\n"
                "    --no-pretty                 do not pretty-print when serializing a keymap\n"
                "    --drop-unused               disable unused bits serialization\n"
                "    --raw                       dump the raw keymap, without parsing it\n"
#else
                "    --format <FORMAT>  use keymap format FORMAT (default: '%s')\n"
                "    --enable-compose   enable Compose\n"
                "    --local-state      enable local state handling and ignore modifiers/layouts\n"
                "                       state updates from the compositor\n"
                "    --legacy-state-api [=true|false]\n"
                "                       use the legacy state API instead of the event API.\n"
                "                       It implies --local-state if explicitly disabled.\n"
                "                       Default: false.\n"
                "    --controls [<CONTROLS>]\n"
                "                       use the given keyboard controls; available values are:\n"
                "                       sticky-keys, latch-to-lock and latch-simultaneous.\n"
                "                       It implies --local-state and --legacy-state-api=false.\n"
                "    --modifiers-mapping <MAPPING>\n"
                "                       use the given modifiers mapping.\n"
                "                       <MAPPING> is a comma-separated list of modifiers masks\n"
                "                       mappings with format \"source:target\", e.g.\n"
                "                       \"Control+Alt:LevelThree,Alt:Meta\".\n"
                "                       It implies --local-state and --legacy-state-api=false.\n"
                "    --shortcuts-mask <MASK>\n"
                "                       use the given modifier mask to enable selecting a specific\n"
                "                       layout (see --shortcuts-mapping) when some of these modifiers\n"
                "                       are active. The modifier mask is a plus-separated list of\n"
                "                       modifiers names, e.g. \"Control+Alt+Super\".\n"
                "                       It implies --local-state and --legacy-state-api=false.\n"
                "    --shortcuts-mapping <MAPPING>\n"
                "                       use the given layout mapping to enable selecting a specific\n"
                "                       layout when some modifiers are active (see --shortcuts-mask).\n"
                "                       <MAPPING> is a comma-separated list of 1-indexed layout\n"
                "                       indices mappings with format \"source:target\", e.g. \"2:1,3:1\".\n"
                "                       It implies --local-state and --legacy-state-api=false.\n"
                "    --keymap [<FILE>]  use the given keymap instead of the keymap from the compositor.\n"
                "                       It implies --local-state.\n"
                "                       If <FILE> is \"-\" or missing, then load from stdin.\n"
                "    -1, --uniline      enable uniline event output\n"
                "    --multiline        enable multiline event output\n"
                "    --consumed-mode={xkb|gtk}\n"
                "                       select the consumed modifiers mode (default: xkb)\n"
                "    --no-state-report  do not report changes to the state\n"
#endif
                "    --verbose          enable verbose debugging output\n"
                "    --help             display this help and exit\n",
                xkb_keymap_get_format_label(DEFAULT_INPUT_KEYMAP_FORMAT)
        );
}

int
main(int argc, char *argv[])
{
    int ret = 0;
    bool verbose = false;
    struct interactive_dpy inter;
    struct wl_registry *registry;

#ifndef KEYMAP_DUMP
    bool with_keymap_file = false;
    const char *keymap_path = NULL;

    /* Only used for state options */
    inter.ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!inter.ctx) {
        ret = -1;
        fprintf(stderr, "Couldn't create xkb context\n");
        goto err_out;
    }
    state_machine_options = xkb_state_machine_options_new(inter.ctx);
    xkb_context_unref(inter.ctx);
    inter.ctx = NULL;
    if (!state_machine_options) {
        ret = -1;
        fprintf(stderr, "Couldn't create xkb state machine options\n");
        goto err_out;
    }

    bool with_compose = false;
    struct xkb_compose_table *compose_table = NULL;
#endif

    enum options {
        OPT_VERBOSE,
        OPT_UNILINE,
        OPT_MULTILINE,
        OPT_CONSUMED_MODE,
        OPT_NO_STATE_REPORT,
        OPT_COMPOSE,
        OPT_LOCAL_STATE,
        OPT_LEGACY_STATE_API,
        OPT_CONTROLS,
        OPT_MODIFIERS_TWEAK_MAPPING,
        OPT_SHORTCUTS_TWEAK_MASK,
        OPT_SHORTCUTS_TWEAK_MAPPING,
        OPT_INPUT_KEYMAP_FORMAT,
        OPT_OUTPUT_KEYMAP_FORMAT,
        OPT_KEYMAP_FORMAT,
        OPT_KEYMAP_NO_PRETTY,
        OPT_KEYMAP_DROP_UNUSED,
        OPT_KEYMAP,
        OPT_RAW,
    };
    static struct option opts[] = {
        {"help",                 no_argument,            0, 'h'},
        {"verbose",              no_argument,            0, OPT_VERBOSE},
#ifdef KEYMAP_DUMP
        {"input-format",         required_argument,      0, OPT_INPUT_KEYMAP_FORMAT},
        {"output-format",        required_argument,      0, OPT_OUTPUT_KEYMAP_FORMAT},
        {"format",               required_argument,      0, OPT_KEYMAP_FORMAT},
        {"no-pretty",            no_argument,            0, OPT_KEYMAP_NO_PRETTY},
        {"drop-unused",          no_argument,            0, OPT_KEYMAP_DROP_UNUSED},
        {"raw",                  no_argument,            0, OPT_RAW},
#else
        {"uniline",              no_argument,            0, OPT_UNILINE},
        {"multiline",            no_argument,            0, OPT_MULTILINE},
        {"consumed-mode",        required_argument,      0, OPT_CONSUMED_MODE},
        {"no-state-report",      no_argument,            0, OPT_NO_STATE_REPORT},
        {"format",               required_argument,      0, OPT_INPUT_KEYMAP_FORMAT},
        {"enable-compose",       no_argument,            0, OPT_COMPOSE},
        {"local-state",          no_argument,            0, OPT_LOCAL_STATE},
        {"legacy-state-api",     optional_argument,      0, OPT_LEGACY_STATE_API},
        {"controls",             required_argument,      0, OPT_CONTROLS},
        {"modifiers-mapping",    required_argument,      0, OPT_MODIFIERS_TWEAK_MAPPING},
        {"shortcuts-mask",       required_argument,      0, OPT_SHORTCUTS_TWEAK_MASK},
        {"shortcuts-mapping",    required_argument,      0, OPT_SHORTCUTS_TWEAK_MAPPING},
        {"keymap",               optional_argument,      0, OPT_KEYMAP},
#endif
        {0, 0, 0, 0},
    };

    setlocale(LC_ALL, "");

#ifndef KEYMAP_DUMP
    /* Ensure synced with usage() and man page */
    assert(use_events_api);
    assert(consumed_mode == XKB_CONSUMED_MODE_XKB);
#endif

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
        case OPT_INPUT_KEYMAP_FORMAT:
            keymap_input_format = xkb_keymap_parse_format(optarg);
            if (!keymap_input_format) {
                fprintf(stderr, "ERROR: invalid %s \"%s\"\n",
#ifdef KEYMAP_DUMP
                        "--input-format",
#else
                        "--format",
#endif
                        optarg);
                goto invalid_usage;
            }
            break;
#ifdef KEYMAP_DUMP
        case OPT_OUTPUT_KEYMAP_FORMAT:
            keymap_output_format = xkb_keymap_parse_format(optarg);
            if (!keymap_output_format) {
                fprintf(stderr, "ERROR: invalid --output-format \"%s\"\n", optarg);
                goto invalid_usage;
            }
            break;
        case OPT_KEYMAP_FORMAT:
            keymap_input_format = xkb_keymap_parse_format(optarg);
            if (!keymap_input_format) {
                fprintf(stderr, "ERROR: invalid --format: \"%s\"\n", optarg);
                goto invalid_usage;
            }
            keymap_output_format = keymap_input_format;
            break;
        case OPT_KEYMAP_NO_PRETTY:
            serialize_flags &= ~XKB_KEYMAP_SERIALIZE_PRETTY;
            break;
        case OPT_KEYMAP_DROP_UNUSED:
            serialize_flags &= ~XKB_KEYMAP_SERIALIZE_KEEP_UNUSED;
            break;
        case OPT_RAW:
            dump_raw_keymap = true;
            break;
#else
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
        case OPT_COMPOSE:
            with_compose = true;
            break;
        case OPT_LOCAL_STATE:
local_state:
            use_local_state = true;
            break;
        case OPT_LEGACY_STATE_API: {
            bool legacy_api = true;
            if (!tools_parse_bool(optarg, TOOLS_ARG_OPTIONAL, &legacy_api))
                goto invalid_usage;
            use_events_api = !legacy_api;
            if (use_events_api)
                goto local_state;
            break;
        }
        case OPT_CONTROLS:
            if (!tools_parse_controls(optarg, state_machine_options,
                                      &kbd_controls_affect,
                                      &kbd_controls_values)) {
                goto invalid_usage;
            }
            /* --local-state and --legacy-state-api=false are implied */
            use_events_api = true;
            goto local_state;
        case OPT_MODIFIERS_TWEAK_MAPPING:
            raw_modifiers_mapping = optarg;
            /* --local-state and --legacy-state-api=false are implied */
            use_events_api = true;
            goto local_state;
        case OPT_SHORTCUTS_TWEAK_MASK:
            raw_shortcuts_mask = optarg;
            /* --local-state and --legacy-state-api=false are implied */
            use_events_api = true;
            goto local_state;
        case OPT_SHORTCUTS_TWEAK_MAPPING:
            if (!tools_parse_shortcuts_mappings(optarg, state_machine_options))
                goto invalid_usage;
            /* --local-state and --legacy-state-api=false are implied */
            use_events_api = true;
            goto local_state;
        case '1':
        case OPT_UNILINE:
            print_options |= PRINT_UNILINE;
            break;
        case '*':
        case OPT_MULTILINE:
            print_options &= ~PRINT_UNILINE;
            break;
        case OPT_CONSUMED_MODE:
            if (strcmp(optarg, "gtk") == 0) {
                consumed_mode = XKB_CONSUMED_MODE_GTK;
            } else if (strcmp(optarg, "xkb") == 0) {
                consumed_mode = XKB_CONSUMED_MODE_XKB;
            } else {
                fprintf(stderr, "ERROR: invalid --consumed-mode \"%s\"\n",
                        optarg);
                usage(stderr, argv[0]);
                ret = EXIT_INVALID_USAGE;
                goto error_parse_args;
            }
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

    memset(&inter, 0, sizeof(inter));
    wl_list_init(&inter.seats);

    inter.dpy = wl_display_connect(NULL);
    if (!inter.dpy) {
        fprintf(stderr, "Couldn't connect to Wayland server\n");
        ret = -1;
        goto err_out;
    }

    inter.ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!inter.ctx) {
        ret = -1;
        fprintf(stderr, "Couldn't create xkb context\n");
        goto err_out;
    }

    if (verbose)
        tools_enable_verbose_logging(inter.ctx);

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
            xkb_context_unref(inter.ctx);
            goto err_out;
        }
        custom_keymap = xkb_keymap_new_from_file(inter.ctx, file,
                                                 keymap_input_format,
                                                 XKB_KEYMAP_COMPILE_NO_FLAGS);
        fclose(file);
    }

    if (with_compose) {
        const char *locale = setlocale(LC_CTYPE, NULL);
        compose_table =
            xkb_compose_table_new_from_locale(inter.ctx, locale,
                                              XKB_COMPOSE_COMPILE_NO_FLAGS);
        if (!compose_table) {
            fprintf(stderr, "Couldn't create compose from locale\n");
            goto err_compose;
        }
        inter.compose_table = compose_table;
    } else {
        inter.compose_table = NULL;
    }
#endif

    registry = wl_display_get_registry(inter.dpy);
    wl_registry_add_listener(registry, &registry_listener, &inter);

    /* The first roundtrip gets the list of advertised globals. */
    wl_display_roundtrip(inter.dpy);

    /* The second roundtrip dispatches the events sent after binding, e.g.
     * after binding to wl_seat globals in the first roundtrip, we will get
     * the wl_seat::capabilities event in this roundtrip. */
    wl_display_roundtrip(inter.dpy);

    if (!inter.shell || !inter.shm || !inter.compositor) {
        fprintf(stderr, "Required Wayland interfaces %s%s%s unsupported\n",
                (inter.shell) ? "" : "xdg_shell ",
                (inter.shm) ? "" : "wl_shm",
                (inter.compositor) ? "" : "wl_compositor");
        ret = -1;
        goto err_conn;
    }

#ifndef KEYMAP_DUMP
    surface_create(&inter);
#endif

    tools_disable_stdin_echo();
    do {
        ret = wl_display_dispatch(inter.dpy);
    } while (ret >= 0 && !terminate);
    tools_enable_stdin_echo();

    wl_registry_destroy(registry);
err_conn:
    dpy_disconnect(&inter);
#ifndef KEYMAP_DUMP
err_compose:
    xkb_compose_table_unref(compose_table);
#endif
err_out:
#ifndef KEYMAP_DUMP
    xkb_keymap_unref(custom_keymap);
#endif
    ret = (ret >= 0) ? EXIT_SUCCESS : EXIT_FAILURE;
error_parse_args:
#ifndef KEYMAP_DUMP
    xkb_state_machine_options_destroy(state_machine_options);
#endif
    exit(ret);
}
