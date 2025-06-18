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
#include "tools-common.h"
#include "src/utils.h"
#include "src/keymap-formats.h"

#include <wayland-client.h>
#include "wayland-client-protocol.h"
#include "xdg-shell-client-protocol.h"
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
    struct xkb_compose_state *compose_state;

    struct wl_list link;
};

static bool terminate;
static enum xkb_keymap_format keymap_input_format = DEFAULT_INPUT_KEYMAP_FORMAT;
#ifdef KEYMAP_DUMP
static enum xkb_keymap_format keymap_output_format = DEFAULT_OUTPUT_KEYMAP_FORMAT;
static bool dump_raw_keymap;
#endif

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
    wl_buffer_destroy(buffer);
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

static void
toplevel_configure(void *data, struct xdg_toplevel *toplevel,
                   int32_t width, int32_t height, struct wl_array *states)
{
    struct interactive_dpy *inter = data;

    if (width == 0)
        width = 200;
    if (height == 0)
        height = 200;

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
    inter->xdg_top = xdg_surface_get_toplevel(inter->xdg_surf);
    xdg_toplevel_add_listener(inter->xdg_top, &toplevel_listener, inter);
    xdg_toplevel_set_title(inter->xdg_top, "xkbcommon event tester");
    xdg_toplevel_set_app_id(inter->xdg_top,
                            "org.xkbcommon.test.interactive-wayland");
    wl_surface_commit(inter->wl_surf);
}

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
    void *buf;

    buf = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
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

    seat->keymap = xkb_keymap_new_from_buffer(seat->inter->ctx, buf, size - 1,
                                              keymap_input_format,
                                              XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(buf, size);
    close(fd);
    if (!seat->keymap) {
        fprintf(stderr, "ERROR: Failed to compile keymap!\n");
        return;
    }
#ifdef KEYMAP_DUMP
    /* Dump the reformatted keymap */
    char *dump = xkb_keymap_get_as_string(seat->keymap, keymap_output_format);
    fprintf(stdout, "%s", dump);
    free(dump);
    return;
#endif

    seat->state = xkb_state_new(seat->keymap);
    if (!seat->state) {
        fprintf(stderr, "ERROR: Failed to create XKB state!\n");
        return;
    }
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
    struct interactive_seat *seat = data;
    xkb_keycode_t keycode = key + EVDEV_OFFSET;

    if (seat->compose_state && state != WL_KEYBOARD_KEY_STATE_RELEASED) {
        xkb_keysym_t keysym = xkb_state_key_get_one_sym(seat->state, keycode);
        xkb_compose_state_feed(seat->compose_state, keysym);
    }

    if (state != WL_KEYBOARD_KEY_STATE_RELEASED) {
        char *prefix = asprintf_safe("%s: ", seat->name_str);
        tools_print_keycode_state(prefix, seat->state, seat->compose_state, keycode,
                                XKB_CONSUMED_MODE_XKB,
                                PRINT_ALL_FIELDS);
        free(prefix);
    }

    if (seat->compose_state) {
        enum xkb_compose_status status = xkb_compose_state_get_status(seat->compose_state);
        if (status == XKB_COMPOSE_CANCELLED || status == XKB_COMPOSE_COMPOSED)
            xkb_compose_state_reset(seat->compose_state);
    }

    /* Exit on ESC. */
    if (xkb_state_key_get_one_sym(seat->state, keycode) == XKB_KEY_Escape &&
        state != WL_KEYBOARD_KEY_STATE_PRESSED)
        terminate = true;
}

static void
kbd_modifiers(void *data, struct wl_keyboard *wl_kbd, uint32_t serial,
              uint32_t mods_depressed, uint32_t mods_latched,
              uint32_t mods_locked, uint32_t group)
{
    struct interactive_seat *seat = data;

    xkb_state_update_mask(seat->state, mods_depressed, mods_latched,
                          mods_locked, 0, 0, group);
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

        xkb_state_unref(seat->state);
        xkb_keymap_unref(seat->keymap);
        xkb_compose_state_unref(seat->compose_state);

        seat->state = NULL;
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

    if (inter->xdg_surf)
        xdg_surface_destroy(inter->xdg_surf);
    if (inter->xdg_top)
        xdg_toplevel_destroy(inter->xdg_top);
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
                "Usage: %s [--help]"
#ifdef KEYMAP_DUMP
                " [--raw] [--input-format] [--output-format] [--format]"
#else
                " [--format] [--enable-compose]"
#endif
                "\n",
                progname);
        fprintf(fp,
#ifdef KEYMAP_DUMP
                "    --input-format <FORMAT>     use input keymap format FORMAT\n"
                "    --output-format <FORMAT>    use output keymap format FORMAT\n"
                "    --format <FORMAT>           keymap format to use for both input and output\n"
                "    --raw                       dump the raw keymap, without parsing it\n"
#else
                "    --format <FORMAT>  use keymap format FORMAT\n"
                "    --enable-compose   enable Compose\n"
#endif
                "    --help             display this help and exit\n"
        );
}

int
main(int argc, char *argv[])
{
    int ret = 0;
    struct interactive_dpy inter;
    struct wl_registry *registry;
    const char *locale;
    struct xkb_compose_table *compose_table = NULL;

    bool with_compose = false;
    enum options {
        OPT_COMPOSE,
        OPT_INPUT_KEYMAP_FORMAT,
        OPT_OUTPUT_KEYMAP_FORMAT,
        OPT_KEYMAP_FORMAT,
        OPT_RAW,
    };
    static struct option opts[] = {
        {"help",                 no_argument,            0, 'h'},
#ifdef KEYMAP_DUMP
        {"input-format",         required_argument,      0, OPT_INPUT_KEYMAP_FORMAT},
        {"output-format",        required_argument,      0, OPT_OUTPUT_KEYMAP_FORMAT},
        {"format",               required_argument,      0, OPT_KEYMAP_FORMAT},
        {"raw",                  no_argument,            0, OPT_RAW},
#else
        {"format",               required_argument,      0, OPT_INPUT_KEYMAP_FORMAT},
        {"enable-compose",       no_argument,            0, OPT_COMPOSE},
#endif
        {0, 0, 0, 0},
    };

    setlocale(LC_ALL, "");

    while (1) {
        int opt;
        int option_index = 0;

        opt = getopt_long(argc, argv, "h", opts, &option_index);
        if (opt == -1)
            break;

        switch (opt) {
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
                usage(stderr, argv[0]);
                return EXIT_INVALID_USAGE;
            }
            break;
#ifdef KEYMAP_DUMP
        case OPT_OUTPUT_KEYMAP_FORMAT:
            keymap_output_format = xkb_keymap_parse_format(optarg);
            if (!keymap_output_format) {
                fprintf(stderr, "ERROR: invalid --output-format \"%s\"\n", optarg);
                usage(stderr, argv[0]);
                return EXIT_INVALID_USAGE;
            }
            break;
        case OPT_KEYMAP_FORMAT:
            keymap_input_format = xkb_keymap_parse_format(optarg);
            if (!keymap_input_format) {
                fprintf(stderr, "ERROR: invalid --format: \"%s\"\n", optarg);
                usage(stderr, argv[0]);
                exit(EXIT_INVALID_USAGE);
            }
            keymap_output_format = keymap_input_format;
            break;
        case OPT_RAW:
            dump_raw_keymap = true;
            break;
#else
        case OPT_COMPOSE:
            with_compose = true;
            break;
#endif
        case 'h':
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;
        default:
            usage(stderr, argv[0]);
            return EXIT_INVALID_USAGE;
        }
    }

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

    if (with_compose) {
        locale = setlocale(LC_CTYPE, NULL);
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

    surface_create(&inter);

    tools_disable_stdin_echo();
    do {
        ret = wl_display_dispatch(inter.dpy);
    } while (ret >= 0 && !terminate);
    tools_enable_stdin_echo();

    wl_registry_destroy(registry);
err_conn:
    dpy_disconnect(&inter);
err_compose:
    xkb_compose_table_unref(compose_table);
err_out:
    exit(ret >= 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
