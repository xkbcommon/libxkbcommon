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

#include <stdlib.h>
#include <limits.h>
#include <locale.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#endif

#include "xkbcommon/xkbcommon.h"

#include "darray.h"
#include "keymap.h"
#include "test.h"
#include "utils.h"
#include "utils-paths.h"
#include "src/keysym.h"
#include "src/xkbcomp/rules.h"
#include "src/utils-numbers.h"

#include "tools/tools-common.h"


/* Setup test */
void
test_init(void)
{
    /* Make stdout always unbuffered, to ensure we always get it entirely */
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    /* Enable to use another locale than C or en_US, so we can catch
     * locale-specific bugs */
    setlocale(LC_ALL, "");
}

void
print_detailed_state(struct xkb_state *state)
{
    fprintf(stderr,
            "  Layout: base: %"PRId32", latched: %"PRId32", locked: %"PRId32", "
            "effective: %"PRIu32"\n",
            xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_DEPRESSED),
            xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LATCHED),
            xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_LOCKED),
            xkb_state_serialize_layout(state, XKB_STATE_LAYOUT_EFFECTIVE));
    fprintf(stderr,
            "  Modifiers: base: %#"PRIx32", latched: %#"PRIx32", "
            "locked: %#"PRIx32", effective: %#"PRIx32"\n",
            xkb_state_serialize_mods(state, XKB_STATE_MODS_DEPRESSED),
            xkb_state_serialize_mods(state, XKB_STATE_MODS_LATCHED),
            xkb_state_serialize_mods(state, XKB_STATE_MODS_LOCKED),
            xkb_state_serialize_mods(state, XKB_STATE_MODS_EFFECTIVE));

    /* There is currently no xkb_state_serialize_leds */
    struct xkb_keymap *keymap = xkb_state_get_keymap(state);
    xkb_led_mask_t leds = 0;
    for (xkb_led_index_t led = 0; led < xkb_keymap_num_leds(keymap); led++) {
        if (xkb_state_led_index_is_active(state, led) > 0)
            leds |= UINT32_C(1) << led;
    }
    fprintf(stderr, "  LEDs: 0x%x\n", leds);
}

/*
 * Test a sequence of keysyms, resulting from a sequence of key presses,
 * against the keysyms they're supposed to generate.
 *
 * - Each test runs with a clean state.
 * - Each line in the test is made up of:
 *   + A keycode, given as a KEY_* from linux/input.h.
 *   + A direction - DOWN for press, UP for release, BOTH for
 *     immediate press + release, REPEAT to just get the syms.
 *   + A sequence of keysyms that should result from this keypress.
 *
 * The vararg format is:
 * <KEY_*>  <DOWN | UP | BOTH>  <XKB_KEY_* (zero or more)>  <NEXT | FINISH>
 *
 * See below for examples.
 */
int
test_key_seq_va(struct xkb_keymap *keymap, va_list ap)
{
    struct xkb_state *state;

    xkb_keycode_t kc;
    int op;
    xkb_keysym_t keysym;

    const xkb_keysym_t *syms;
    xkb_keysym_t sym;
    char ksbuf[XKB_KEYSYM_NAME_MAX_SIZE];
    const char *opstr = NULL;

    fprintf(stderr, "----\n");

    state = xkb_state_new(keymap);
    assert(state);

    for (;;) {
        kc = va_arg(ap, int) + EVDEV_OFFSET;
        op = va_arg(ap, int);

        switch (op) {
            case DOWN: opstr = "DOWN"; break;
            case REPEAT: opstr = "REPEAT"; break;
            case UP: opstr = "UP"; break;
            case BOTH: opstr = "BOTH"; break;
            case NEXT: opstr = "NEXT"; break;
            case FINISH: opstr = "FINISH"; break;
            default:
                fprintf(stderr, "ERROR: Unsupported operation: %d\n", op);
                goto fail;
        }

        const int nsyms = xkb_state_key_get_syms(state, kc, &syms);
        if (nsyms == 1) {
            sym = xkb_state_key_get_one_sym(state, kc);
            syms = &sym;
        }

        if (op == DOWN || op == BOTH)
            xkb_state_update_key(state, kc, XKB_KEY_DOWN);
        if (op == UP || op == BOTH)
            xkb_state_update_key(state, kc, XKB_KEY_UP);

#if HAVE_TOOLS
        tools_print_keycode_state("", state, NULL, kc,
                                  (op == DOWN ? XKB_KEY_DOWN : XKB_KEY_UP),
                                  XKB_CONSUMED_MODE_XKB,
                                  PRINT_ALL_FIELDS | PRINT_UNILINE);
#endif
        fprintf(stderr, "op %-6s got %d syms for keycode %3"PRIu32": [", opstr, nsyms, kc);

        for (int i = 0; i < nsyms; i++) {
            keysym = va_arg(ap, int);
            xkb_keysym_get_name(syms[i], ksbuf, sizeof(ksbuf));
            fprintf(stderr, "%s%s", (i != 0) ? ", " : "", ksbuf);

            if (keysym == FINISH || keysym == NEXT) {
                xkb_keysym_get_name(syms[i], ksbuf, sizeof(ksbuf));
                fprintf(stderr, "\nERROR: Did not expect keysym: %s.\n", ksbuf);
                goto fail;
            }

            if (keysym != syms[i]) {
                xkb_keysym_get_name(keysym, ksbuf, sizeof(ksbuf));
                fprintf(stderr, "\nERROR: Expected keysym: %s. ", ksbuf);;
                xkb_keysym_get_name(syms[i], ksbuf, sizeof(ksbuf));
                fprintf(stderr, " Got keysym: %s.\n", ksbuf);;
                goto fail;
            }
        }

        if (nsyms == 0) {
            keysym = va_arg(ap, int);
            if (keysym != XKB_KEY_NoSymbol) {
                xkb_keysym_get_name(keysym, ksbuf, sizeof(ksbuf));
                fprintf(stderr, "\nERROR: Expected %s, but got no keysyms.\n", ksbuf);
                goto fail;
            }
        }

        fprintf(stderr, "]\n");

        keysym = va_arg(ap, int);
        if (keysym == NEXT)
            continue;
        if (keysym == FINISH)
            break;

        xkb_keysym_get_name(keysym, ksbuf, sizeof(ksbuf));
        fprintf(stderr, "\nERROR: Expected keysym: %s. Didn't get it.\n", ksbuf);
        goto fail;
    }

    xkb_state_unref(state);
    return 1;

fail:
    fprintf(stderr, "Current state:\n");
    print_detailed_state(state);
    xkb_state_unref(state);
    return 0;
}

int
test_key_seq(struct xkb_keymap *keymap, ...)
{
    va_list ap;
    int ret;

    va_start(ap, keymap);
    ret = test_key_seq_va(keymap, ap);
    va_end(ap);

    return ret;
}

char *
test_makedir(const char *parent, const char *path)
{
    char *dirname;
    int err;

    dirname = asprintf_safe("%s/%s", parent, path);
    assert(dirname);
#ifdef _WIN32
    err = _mkdir(dirname);
#else
    err = mkdir(dirname, 0777);
#endif
    assert(err == 0);

    return dirname;
}

char *
test_maketempdir(const char *template)
{
#ifdef _WIN32
    const char *basetmp = getenv("TMP");
    if (basetmp == NULL) {
        basetmp = getenv("TEMP");
    }
    if (basetmp == NULL) {
        basetmp = getenv("top_builddir");
    }
    assert(basetmp != NULL);
    char *tmpdir = asprintf_safe("%s/%s", basetmp, template);
    assert(tmpdir != NULL);
    char *tmp = _mktemp(tmpdir);
    assert(tmp == tmpdir);
    int ret = _mkdir(tmp);
    assert(ret == 0);
    return tmpdir;
#else
    char *tmpdir = asprintf_safe("/tmp/%s", template);
    assert(tmpdir != NULL);
    char *tmp = mkdtemp(tmpdir);
    assert(tmp == tmpdir);
    return tmpdir;
#endif
}

char *
test_get_path(const char *path_rel)
{
    char *path;
    const char *srcdir;

    srcdir = getenv("top_srcdir");
    if (!srcdir)
        srcdir = ".";

    if (is_absolute_path(path_rel))
        return strdup(path_rel);

    path = asprintf_safe("%s/test/data%s%s", srcdir,
                         path_rel[0] ? "/" : "", path_rel);
    if (!path) {
        fprintf(stderr, "Failed to allocate path for %s\n", path_rel);
        return NULL;
    }
    return path;
}

char *
read_file(const char *path, FILE *file)
{
    if (!file)
        return NULL;

    const int fd = fileno(file);
    if (fd < 0)
        return NULL;

    struct stat info;
    if (fstat(fd, &info) != 0) {
        fclose(file);
        return NULL;
    }

    char* ret = malloc(info.st_size + 1);
    if (!ret) {
        fclose(file);
        return NULL;
    }

    const size_t size = info.st_size;
    const size_t count = fread(ret, sizeof(*ret), size, file);
    if (count != size) {
        if (!feof(file))
            printf("Error reading file %s: unexpected end of file\n", path);
        else if (ferror(file))
            perror("Error reading file");
        fclose(file);
        free(ret);
        return NULL;
    }
    ret[count] = '\0';

    return ret;
}

char *
test_read_file(const char *path_rel)
{
    char *path = test_get_path(path_rel);
    if (!path)
        return NULL;

    FILE* file = fopen(path, "rb");
    char *ret = NULL;

    if (!file)
        goto out;

    ret = read_file(path, file);

out:
    if (file)
        fclose(file);
    free(path);
    return ret;
}

struct xkb_context *
test_get_context(enum test_context_flags test_flags)
{
    enum xkb_context_flags ctx_flags = XKB_CONTEXT_NO_DEFAULT_INCLUDES;
    if (test_flags & CONTEXT_ALLOW_ENVIRONMENT_NAMES) {
        unsetenv("XKB_DEFAULT_RULES");
        unsetenv("XKB_DEFAULT_MODEL");
        unsetenv("XKB_DEFAULT_LAYOUT");
        unsetenv("XKB_DEFAULT_VARIANT");
        unsetenv("XKB_DEFAULT_OPTIONS");
    }
    else {
        ctx_flags |= XKB_CONTEXT_NO_ENVIRONMENT_NAMES;
    }

    struct xkb_context * const ctx = xkb_context_new(ctx_flags);
    if (!ctx)
        return NULL;

    char * const path = test_get_path("");
    if (!path) {
        xkb_context_unref(ctx);
        return NULL;
    }

    xkb_context_include_path_append(ctx, path);
    free(path);

    return ctx;
}

struct xkb_keymap *
test_compile_file(struct xkb_context *context, enum xkb_keymap_format format,
                  const char *path_rel)
{
    struct xkb_keymap *keymap;
    FILE *file;
    char *path;

    path = test_get_path(path_rel);
    if (!path)
        return NULL;

    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open path: %s\n", path);
        free(path);
        return NULL;
    }
    assert(file != NULL);

    keymap = xkb_keymap_new_from_file(context, file, format,
                                      XKB_KEYMAP_COMPILE_NO_FLAGS);
    fclose(file);

    if (!keymap) {
        fprintf(stderr, "Failed to compile path: %s\n", path);
        free(path);
        return NULL;
    }

    fprintf(stderr, "Successfully compiled path: %s\n", path);
    free(path);

    return keymap;
}

struct xkb_keymap *
test_compile_string(struct xkb_context *context, enum xkb_keymap_format format,
                    const char *string)
{
    struct xkb_keymap *keymap;

    keymap = xkb_keymap_new_from_string(context, string, format,
                                        XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        fprintf(stderr, "Failed to compile string\n");
        return NULL;
    }

    return keymap;
}

struct xkb_keymap *
test_compile_buffer(struct xkb_context *context, enum xkb_keymap_format format,
                    const char *buf, size_t len)
{
    struct xkb_keymap *keymap;

    keymap = xkb_keymap_new_from_buffer(context, buf, len, format,
                                        XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        fprintf(stderr, "Failed to compile keymap from memory buffer\n");
        return NULL;
    }

    return keymap;
}

struct xkb_keymap *
test_compile_rules(struct xkb_context *context, enum xkb_keymap_format format,
                   const char *rules, const char *model, const char *layout,
                   const char *variant, const char *options)
{
    struct xkb_keymap *keymap;
    struct xkb_rule_names rmlvo = {
        .rules = isempty(rules) ? NULL : rules,
        .model = isempty(model) ? NULL : model,
        .layout = isempty(layout) ? NULL : layout,
        .variant = isempty(variant) ? NULL : variant,
        .options = isempty(options) ? NULL : options
    };

    if (!rules && !model && !layout && !variant && !options)
        keymap = xkb_keymap_new_from_names2(context, NULL, format,
                                            XKB_KEYMAP_COMPILE_NO_FLAGS);
    else
        keymap = xkb_keymap_new_from_names2(context, &rmlvo, format,
                                            XKB_KEYMAP_COMPILE_NO_FLAGS);

    if (!keymap) {
        fprintf(stderr,
                "Failed to compile RMLVO: '%s', '%s', '%s', '%s', '%s'\n",
                rules, model, layout, variant, options);
        return NULL;
    }

    return keymap;
}

static struct xkb_rmlvo_builder *
xkb_rules_names_to_rmlvo_builder(struct xkb_context *context,
                                 const struct xkb_rule_names *names)
{
    struct xkb_rmlvo_builder *rmlvo =
        xkb_rmlvo_builder_new(context, names->rules, names->model,
                              XKB_RMLVO_BUILDER_NO_FLAGS);
    if (!rmlvo) {
        fprintf(stderr, "ERROR: xkb_rmlvo_builder_new() failed\n");
        return NULL;
    }

    char buf[1024] = { 0 };

    /* First parse options, and gather the layout-specific ones and
     * append the others to the builder */
    darray(darray_string) loptions = darray_new();
    if (!isempty(names->options)) {
        const char *o = names->options;
        while (*o != '\0') {
            /* Get option name */
            const char *option = o;
            while (*o != '\0' && *o != ',' &&
                   *o != OPTIONS_GROUP_SPECIFIER_PREFIX) {
                    o++;
            };
            const size_t len = o - option;
            if (len >= sizeof(buf))
                goto error;
            memcpy(buf, option, len);
            buf[len] = '\0';

            /* Check if layout-specific option */
            xkb_layout_index_t layout = XKB_LAYOUT_INVALID;
            if (*o == OPTIONS_GROUP_SPECIFIER_PREFIX) {
                o++;
                int count = parse_dec_to_uint32_t(o, SIZE_MAX, &layout);
                if (count > 0 && layout > 0 && layout <= XKB_MAX_GROUPS) {
                    o += count;
                    layout--;
                } else {
                    layout = XKB_LAYOUT_INVALID;
                }
                const char* const layout_index_end = o;
                while (*o != '\0' && *o != ',') { o++; }
                if (count <= 0 || layout_index_end != o)
                    layout = XKB_LAYOUT_INVALID;
            }

            if (layout != XKB_LAYOUT_INVALID) {
                /* Save as layout-specific option, to be added with layout */
                char *opt = strdup(buf);
                if (!opt)
                    goto error;
                darray_resize0(loptions, layout + 1);
                darray_append(darray_item(loptions, layout), opt);
            } else {
                /* Append layout-agnostic option */
                if (!xkb_rmlvo_builder_append_option(rmlvo, buf))
                    goto error;
            }

            if (*o == ',')
                o++;
        }
    }

    if (!isempty(names->layout)) {
        /* Process layout & variant together.
         * Ignore variants without corresponding layout. */
        const char *l = names->layout;
        const char *v = names->variant;
        if (!names->variant)
            v = "";
        xkb_layout_index_t layout_count = 0;
        while (*l != '\0') {
            const char *layout = l;
            const char *variant = v;
            char *start = buf;
            size_t buf_size = sizeof(buf);
            while (*l != '\0' && *l != ',') { l++; };
            while (*v != '\0' && *v != ',') { v++; };

            size_t len = l - layout;
            if (len >= buf_size)
                goto error;
            memcpy(start, layout, len);
            start[len] = '\0';
            start += len;
            buf_size -= len + 1;

            len = v - variant;
            if (len >= buf_size)
                goto error;
            ++start;
            memcpy(start, variant, len);
            start[len] = '\0';

            /* Get layout-specific options */
            char **opts = NULL;
            size_t opts_count = 0;
            if (layout_count < darray_size(loptions)) {
                opts = darray_items(darray_item(loptions, layout_count));
                opts_count = darray_size(darray_item(loptions, layout_count));
            }

            if (!xkb_rmlvo_builder_append_layout(rmlvo, buf, start,
                                                 (const char* *) opts,
                                                 opts_count))
                goto error;

            if (*l == ',')
                l++;
            if (*v == ',')
                v++;

            layout_count += 1;
        }
    }

exit:
    {
        darray_string *opts;
        darray_foreach(opts, loptions) {
            char **opt;
            darray_foreach(opt, *opts)
                free(*opt);
            darray_free(*opts);
        }
        darray_free(loptions);
    }

    return rmlvo;

error:
    fprintf(stderr, "ERROR: %s\n", __func__);
    xkb_rmlvo_builder_unref(rmlvo);
    rmlvo = NULL;
    goto exit;
}

struct xkb_keymap *
test_compile_rmlvo(struct xkb_context *context, enum xkb_keymap_format format,
                   const char *rules, const char *model, const char *layout,
                   const char *variant, const char *options)
{
    struct xkb_keymap *keymap = NULL;
    const struct xkb_rule_names names = {
        .rules = rules,
        .model = model,
        .layout = layout,
        .variant = variant,
        .options = options
    };

    struct xkb_rmlvo_builder *rmlvo = xkb_rules_names_to_rmlvo_builder(context,
                                                                       &names);

    if (!rmlvo) {
        fprintf(stderr,
                "Failed to create RMLVO builder: "
                "'%s', '%s', '%s', '%s', '%s'\n",
                rules, model, layout, variant, options);
        return NULL;
    }

    keymap = xkb_keymap_new_from_rmlvo(rmlvo, format,
                                       XKB_KEYMAP_COMPILE_NO_FLAGS);

    xkb_rmlvo_builder_unref(rmlvo);

    if (!keymap) {
        fprintf(stderr,
                "Failed to compile RMLVO from builder: "
                "'%s', '%s', '%s', '%s', '%s'\n",
                rules, model, layout, variant, options);
    }

    return keymap;
}

bool
test_compile_output(struct xkb_context *ctx, enum xkb_keymap_format input_format,
                    enum xkb_keymap_format output_format,
                    test_compile_buffer_t compile_buffer,
                    void *compile_buffer_private, const char *test_title,
                    const char *keymap_str, size_t keymap_len,
                    const char *rel_path, bool update_output_files)
{
    return test_compile_output2(ctx, input_format, output_format,
                                TEST_KEYMAP_SERIALIZE_FLAGS,
                                compile_buffer, compile_buffer_private,
                                test_title, keymap_str, keymap_len, rel_path,
                                update_output_files);
}

bool
test_compile_output2(struct xkb_context *ctx,
                     enum xkb_keymap_format input_format,
                     enum xkb_keymap_format output_format,
                     enum xkb_keymap_serialize_flags serialize_flags,
                     test_compile_buffer_t compile_buffer,
                     void *compile_buffer_private, const char *test_title,
                     const char *keymap_str, size_t keymap_len,
                     const char *rel_path, bool update_output_files)
{
    int success = true;
    fprintf(stderr, "*** %s ***\n", test_title);

    struct xkb_keymap *keymap = compile_buffer(
        ctx, input_format, keymap_str, keymap_len, compile_buffer_private
    );

    if (!rel_path) {
        /* No path given: expect compilation failure */
        if (keymap) {
            char *got = xkb_keymap_get_as_string2(keymap, output_format,
                                                  serialize_flags);
            xkb_keymap_unref(keymap);
            assert(got);
            fprintf(stderr,
                    "Unexpected keymap compilation success:\n%s\n", got);
            free(got);
        }
        return !keymap;
    }

    if (!keymap) {
        fprintf(stderr, "Unexpected keymap compilation failure\n");
        return false;
    }

    char *got = xkb_keymap_get_as_string2(keymap, output_format,
                                          serialize_flags);
    if (!got) {
        fprintf(stderr, "Unexpected keymap serialization failure\n");
        return false;
    }

    xkb_keymap_unref(keymap);

    char* const path = test_get_path(rel_path);
    assert(path);

    if (update_output_files) {
        fprintf(stderr, "Writing golden test output to: %s\n", path);
        FILE *file = fopen(path, "wb");
        assert(file);
        fwrite(got, 1, strlen(got), file);
        fclose(file);
    } else {
        fprintf(stderr, "Reading golden test output: %s\n", path);
        char* const expected = test_read_file(rel_path);
        assert(expected);
        static const char *label[2] = {"Golden test", "Roundtrip"};
        bool test_round_trip =
            (output_format == input_format ||
             output_format == XKB_KEYMAP_USE_ORIGINAL_FORMAT);
        for (unsigned int k = 0; k < ARRAY_SIZE(label) && success; k++) {
            if (streq(expected, got)) {
                fprintf(stderr, "%s succeeded.\n", label[k]);
                if (!test_round_trip)
                    break;
                /* Test round trip */
                keymap = compile_buffer(ctx, input_format,
                                        expected, strlen(expected),
                                        compile_buffer_private);
                if (!keymap) {
                    fprintf(stderr,
                            "Unexpected keymap roundtrip compilation failure\n");
                    success = false;
                    break;
                }
                free(got);
                got = xkb_keymap_get_as_string2(keymap, output_format,
                                                serialize_flags);
                if (!got) {
                    fprintf(stderr,
                            "Unexpected keymap roundtrip serialization failure\n");
                    success = false;
                }
                xkb_keymap_unref(keymap);
                test_round_trip = false;
            } else {
                fprintf(stderr,
                        "%s failed: dumped map differs from expected.\n",
                        label[k]);
                fprintf(stderr, "Path to expected file: %s\n", path);
                fprintf(stderr, "Length: expected %zu, got: %zu\n",
                        strlen(expected), strlen(got));
                fprintf(stderr, "Dumped map:\n");
                fprintf(stderr, "%s\n", got);
                success = false;
            }
        }
        free(expected);
    }
    free(got);
    free(path);
    return success;
}

bool
test_third_pary_compile_output(test_third_party_compile_buffer_t compile_buffer,
                               void *compile_buffer_private,
                               const char *test_title,
                               const char *keymap_in, size_t keymap_in_size,
                               const char *rel_path, bool update_output_files)
{
    int success = true;
    fprintf(stderr, "*** %s ***\n", test_title);
    char* got = NULL;
    size_t got_size = 0;

    int ret = compile_buffer(keymap_in, keymap_in_size, compile_buffer_private,
                             &got, &got_size);

    if (!rel_path) {
        /* No path given: expect compilation failure */
        if (ret == EXIT_SUCCESS) {
            fprintf(stderr,
                    "Unexpected keymap compilation success:\nstdout:\n%s\n",
                    got);
        }
        free(got);
        return (ret != EXIT_SUCCESS);
    }

    if (ret != EXIT_SUCCESS || isempty(got)) {
        fprintf(stderr, "Unexpected keymap compilation failure.\nstdout:\n%s\n",
                got);
        free(got);
        return false;
    }

    char *path = test_get_path(rel_path);
    assert(path);

    if (update_output_files) {
        fprintf(stderr, "Writing golden test output to: %s\n", path);
        FILE *file = fopen(path, "wb");
        assert(file);
        fwrite(got, 1, got_size, file);
        fclose(file);
    } else {
        fprintf(stderr, "Reading golden test output: %s\n", path);
        char *expected = test_read_file(rel_path);
        assert(expected);
        const char *label[2] = {"Golden test", "Roundtrip"};
        for (unsigned int k = 0; k < ARRAY_SIZE(label) && success; k++) {
            if (streq(expected, got)) {
                fprintf(stderr, "%s succeeded.\n", label[k]);
                if (k > 0)
                    continue;
                /* Test round trip */
                // TODO
                break;
            }else {
                fprintf(stderr,
                        "%s failed: dumped map differs from expected.\n",
                        label[k]);
                fprintf(stderr, "Path to expected file: %s\n", path);
                fprintf(stderr, "Length: expected %zu, got: %zu\n",
                        strlen(expected), strlen(got));
                fprintf(stderr, "Dumped map:\n");
                fprintf(stderr, "%s\n", got);
                success = false;
            }
        }
        free(expected);
    }
    free(got);
    free(path);
    return success;
}
