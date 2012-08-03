/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
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
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <linux/input.h>

#include "xkbcommon/xkbcommon.h"
#include "test.h"

struct keyboard {
    char *path;
    int fd;
    struct xkb_state *state;
    struct keyboard *next;
};

bool terminate;

#define NLONGS(n) (((n) + LONG_BIT - 1) / LONG_BIT)

static bool
evdev_bit_is_set(const unsigned long *array, int bit)
{
    return !!(array[bit / LONG_BIT] & (1LL << (bit % LONG_BIT)));
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
keyboard_new(struct dirent *ent, struct xkb_keymap *xkb, struct keyboard **out)
{
    int ret;
    char *path;
    int fd;
    struct xkb_state *state;
    struct keyboard *kbd;

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

    state = xkb_state_new(xkb);
    if (!state) {
        fprintf(stderr, "Couldn't create xkb state for %s\n", path);
        ret = -EFAULT;
        goto err_fd;
    }

    kbd = calloc(1, sizeof(*kbd));
    if (!kbd) {
        ret = -ENOMEM;
        goto err_state;
    }

    kbd->path = path;
    kbd->fd = fd;
    kbd->state = state;
    *out = kbd;
    return 0;

err_state:
    xkb_state_unref(state);
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
    xkb_state_unref(kbd->state);
    free(kbd);
}

static int
filter_device_name(const struct dirent *ent)
{
    return !fnmatch("event*", ent->d_name, 0);
}

static struct keyboard *
get_keyboards(struct xkb_keymap *keymap)
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
        ret = keyboard_new(ents[i], keymap, &kbd);
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

static void
print_keycode(struct keyboard *kbd, xkb_keycode_t keycode)
{
    unsigned int i;
    struct xkb_keymap *keymap;
    struct xkb_state *state;

    const xkb_keysym_t *syms;
    unsigned int nsyms;
    char s[16];
    uint32_t unicode;
    xkb_group_index_t group;
    xkb_mod_index_t mod;
    xkb_led_index_t led;

    state = kbd->state;
    keymap = xkb_state_get_map(state);

    nsyms = xkb_key_get_syms(state, keycode, &syms);

    if (nsyms <= 0)
        return;

    printf("keysyms [ ");
    for (i = 0; i < nsyms; i++) {
        xkb_keysym_get_name(syms[i], s, sizeof(s));
        printf("%-*s ", (int)sizeof(s), s);
    }
    printf("] ");

    /*
     * Only do this if wchar_t is UCS-4, so we can be lazy and print
     * with %lc.
     */
#ifdef __STDC_ISO_10646__
    printf("unicode [ ");
    for (i = 0; i < nsyms; i++) {
        unicode = xkb_keysym_to_utf32(syms[i]);
        printf("%lc ", (int)(unicode ? unicode : L' '));
    }
    printf("] ");
#endif

    printf("groups [ ");
    for (group = 0; group < xkb_map_num_groups(keymap); group++) {
        if (!xkb_state_group_index_is_active(state, group, XKB_STATE_EFFECTIVE))
            continue;
        printf("%s (%d) ", xkb_map_group_get_name(keymap, group), group);
    }
    printf("] ");

    printf("mods [ ");
    for (mod = 0; mod < xkb_map_num_mods(keymap); mod++) {
        if (!xkb_state_mod_index_is_active(state, mod, XKB_STATE_EFFECTIVE))
            continue;
        if (xkb_key_mod_index_is_consumed(state, keycode, mod))
            printf("-%s ", xkb_map_mod_get_name(keymap, mod));
        else
            printf("%s ", xkb_map_mod_get_name(keymap, mod));
    }
    printf("] ");

    printf("leds [ ");
    for (led = 0; led < xkb_map_num_leds(keymap); led++) {
        if (!xkb_state_led_index_is_active(state, led))
            continue;
        printf("%s ", xkb_map_led_get_name(keymap, led));
    }
    printf("] ");

    printf("\n");
}

/* The meaning of the input_event 'value' field. */
enum {
    KEY_STATE_RELEASE = 0,
    KEY_STATE_PRESS = 1,
    KEY_STATE_REPEAT = 2,
};

#define EVDEV_OFFSET 8

static void
process_event(struct keyboard *kbd, uint16_t type, uint16_t code, int32_t value)
{
    xkb_keycode_t keycode;
    struct xkb_keymap *keymap;

    if (type != EV_KEY)
        return;

    keycode = EVDEV_OFFSET + code;
    keymap = xkb_state_get_map(kbd->state);

    if (value == KEY_STATE_REPEAT && !xkb_key_repeats(keymap, keycode))
        return;

    if (value != KEY_STATE_RELEASE)
        print_keycode(kbd, keycode);

    if (value == KEY_STATE_RELEASE)
        xkb_state_update_key(kbd->state, keycode, XKB_KEY_UP);
    else
        xkb_state_update_key(kbd->state, keycode, XKB_KEY_DOWN);
}

static int
read_keyboard(struct keyboard *kbd)
{
    size_t i;
    ssize_t len;
    struct input_event evs[16];
    size_t nevs;

    /* No fancy error checking here. */
    while ((len = read(kbd->fd, &evs, sizeof(evs))) > 0) {
        nevs = len / sizeof(struct input_event);
        for (i = 0; i < nevs; i++)
            process_event(kbd, evs[i].type, evs[i].code, evs[i].value);
    }

    if (len < 0 && errno != EWOULDBLOCK) {
        fprintf(stderr, "Couldn't read %s: %s\n", kbd->path,
                strerror(errno));
        return -errno;
    }

    return 0;
}

static int
loop(struct keyboard *kbds)
{
    int i, ret;
    int epfd;
    struct keyboard *kbd;
    struct epoll_event ev;
    struct epoll_event evs[16];

    epfd = epoll_create1(0);
    if (epfd < 0) {
        fprintf(stderr, "Couldn't create epoll instance: %s\n",
                strerror(errno));
        return -errno;
    }

    for (kbd = kbds; kbd; kbd = kbd->next) {
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN;
        ev.data.ptr = kbd;
        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, kbd->fd, &ev);
        if (ret) {
            ret = -errno;
            fprintf(stderr, "Couldn't add %s to epoll: %s\n",
                    kbd->path, strerror(errno));
            goto err_epoll;
        }
    }

    while (!terminate) {
        ret = epoll_wait(epfd, evs, 16, -1);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            ret = -errno;
            fprintf(stderr, "Couldn't poll for events: %s\n",
                    strerror(errno));
            goto err_epoll;
        }

        for (i = 0; i < ret; i++) {
            kbd = evs[i].data.ptr;
            ret = read_keyboard(kbd);
            if (ret) {
                goto err_epoll;
            }
        }
    }

    close(epfd);
    return 0;

err_epoll:
    close(epfd);
    return ret;
}

static void
sigintr_handler(int signum)
{
    terminate = true;
}

int
main(int argc, char *argv[])
{
    int ret;
    int opt;
    struct keyboard *kbds;
    struct xkb_context *ctx;
    struct xkb_keymap *keymap;
    const char *rules = NULL;
    const char *model = NULL;
    const char *layout = NULL;
    const char *variant = NULL;
    const char *options = NULL;
    const char *keymap_path = NULL;
    struct sigaction act;

    setlocale(LC_ALL, "");

    while ((opt = getopt(argc, argv, "r:m:l:v:o:k:")) != -1) {
        switch (opt) {
        case 'r':
            rules = optarg;
            break;
        case 'm':
            model = optarg;
            break;
        case 'l':
            layout = optarg;
            break;
        case 'v':
            variant = optarg;
            break;
        case 'o':
            options = optarg;
            break;
        case 'k':
            keymap_path = optarg;
            break;
        case '?':
            fprintf(stderr, "Usage: %s [-r <rules>] [-m <model>] "
                    "[-l <layout>] [-v <variant>] [-o <options>]\n",
                    argv[0]);
            fprintf(stderr, "   or: %s -k <path to keymap file>\n",
                    argv[0]);
            exit(EX_USAGE);
        }
    }

    ctx = test_get_context();
    if (!ctx) {
        ret = -1;
        fprintf(stderr, "Couldn't create xkb context\n");
        goto err_out;
    }

    if (keymap_path)
        keymap = test_compile_file(ctx, keymap_path);
    else
        keymap = test_compile_rules(ctx, rules, model, layout, variant,
                                    options);
    if (!keymap) {
        ret = -1;
        fprintf(stderr, "Couldn't create xkb keymap\n");
        goto err_ctx;
    }

    kbds = get_keyboards(keymap);
    if (!kbds) {
        ret = -1;
        goto err_xkb;
    }

    act.sa_handler = sigintr_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    /* Instead of fiddling with termios.. */
    system("stty -echo");

    ret = loop(kbds);
    if (ret)
        goto err_stty;

err_stty:
    system("stty echo");
    free_keyboards(kbds);
err_xkb:
    xkb_map_unref(keymap);
err_ctx:
    xkb_context_unref(ctx);
err_out:
    exit(ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
