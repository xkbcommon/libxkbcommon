/*

Copyright 1990, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

/* Constructs hash tables for XStringToKeysym and XKeysymToString. */

#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/keysymdef.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint32_t Signature;

#define KTNUM 4000

static struct info {
    char        *name;
    KeySym      val;
} info[KTNUM];

#define MIN_REHASH 15
#define MATCHES 10

static char tab[KTNUM];
static unsigned short offsets[KTNUM];
static unsigned short indexes[KTNUM];
static KeySym values[KTNUM];

/*
 * XFree86 special action keys - for some reason, these have an
 * underscore after the XF86 prefix.
 */
static const char *xf86_special_keys[] = {
    "Switch_VT_1",
    "Switch_VT_2",
    "Switch_VT_3",
    "Switch_VT_4",
    "Switch_VT_5",
    "Switch_VT_6",
    "Switch_VT_7",
    "Switch_VT_8",
    "Switch_VT_9",
    "Switch_VT_10",
    "Switch_VT_11",
    "Switch_VT_12",
    "Ungrab",
    "ClearGrab",
    "Next_VMode",
    "Prev_VMode",
    NULL
};

static int
is_xf86_special(const char *key)
{
    const char **s = xf86_special_keys;
    while (*s) {
        if (strcmp(key, *s) == 0)
            return 1;
        s++;
    }
    return 0;
}

static int
get_keysym(const char *buf, char *key, int ndx)
{
    if (sscanf(buf, "#define XK_%127s 0x%lx", key, &info[ndx].val) != 2)
        return 0;
    return 1;
}

static int
get_keysym_alias(const char *buf, char *key, int ndx)
{
    int i;
    char alias[128];

    if (sscanf(buf, "#define XK_%127s XK_%127s", key, alias) != 2)
        return 0;

    for (i = ndx - 1; i >= 0; i--) {
        if (strcmp(info[i].name, alias) == 0) {
            info[ndx].val = info[i].val;
            return 1;
        }
    }

    /* Didn't find a match */
    fprintf(stderr, "can't find matching definition %s for keysym %s\n",
            alias, key);
    return -1;
}

static int
get_xf86_keysym(const char *buf, char *key, size_t len, int ndx)
{
    char tmp[128];

    if (sscanf(buf, "#define XF86XK_%127s 0x%lx", tmp, &info[ndx].val) != 2)
        return 0;

    /* Prepend XF86 or XF86_ to the key */
    snprintf(key, len, "XF86%s%s", is_xf86_special(tmp) ? "_" : "", tmp);

    return 1;
}

static int
get_xf86_keysym_alias(const char *buf, char *key, size_t len, int ndx)
{
    int i;
    char alias[128], ktmp[128], atmp[128];

    /* Try to handle both XF86XK and XK aliases */
    if (sscanf(buf, "#define XF86XK_%127s XF86XK_%127s", ktmp, atmp) == 2) {
        /* Prepend XF86 to the key and alias */
        snprintf(key, len, "XF86%s%s", is_xf86_special(ktmp) ? "_" : "",
                 ktmp);
        snprintf(alias, sizeof(alias), "XF86%s%s",
                 is_xf86_special(atmp) ? "_" : "", atmp);
    } else {
        if (sscanf(buf, "#define XF86XK_%127s XK_%127s", ktmp, alias) != 2)
            return 0;
        /* Prepend XF86 to the key */
        snprintf(key, len, "XF86%s%s", is_xf86_special(ktmp) ? "_" : "",
                 ktmp);
    }

    for (i = ndx - 1; i >= 0; i--) {
        if (strcmp(info[i].name, alias) == 0) {
            info[ndx].val = info[i].val;
            return 1;
        }
    }

    /* Didn't find a match */
    fprintf(stderr, "can't find matching definition %s for keysym %s\n",
            alias, key);
    return -1;
}

int
main(int argc, char *argv[])
{
    int ksnum = 0;
    int max_rehash;
    Signature sig;
    int i, j, k, z;
    char *name;
    char c;
    int first;
    int best_max_rehash;
    int best_z = 0;
    int num_found;
    KeySym val;
    char key[128];
    char buf[1024];


    while (fgets(buf, sizeof(buf), stdin)) {
        int ret;

        /* Manage keysyms from keysymdef.h */
        ret = get_keysym(buf, key, ksnum);
        if (!ret) {
            ret = get_keysym_alias(buf, key, ksnum);
            if (ret == -1)
                continue;
        }

        /* Manage keysyms from XF86keysym.h */
        if (!ret)
            ret = get_xf86_keysym(buf, key, sizeof(key), ksnum);
        if (!ret) {
            ret = get_xf86_keysym_alias(buf, key, sizeof(key), ksnum);
            if (ret < 1)
                continue;
        }

        if (info[ksnum].val > 0x1fffffff) {
            fprintf(stderr,
                    "ignoring illegal keysym (%s), remove it from .h file!\n",
                    key);
            continue;
        }
        name = malloc((unsigned)strlen(key) + 1);
        if (!name) {
            fprintf(stderr, "makekeys: out of memory!\n");
            exit(1);
        }
        (void)strcpy(name, key);
        info[ksnum].name = name;
        ksnum++;
        if (ksnum == KTNUM) {
            fprintf(stderr, "makekeys: too many keysyms!\n");
            exit(1);
        }
    }

    /* Special case NoSymbol. */
    info[ksnum].name = strdup("NoSymbol");
    info[ksnum].val = 0L;
    ksnum++;

    printf("/* This file is generated from keysymdef.h. */\n");
    printf("/* Do not edit. */\n");
    printf("\n");

    best_max_rehash = ksnum;
    num_found = 0;
    for (z = ksnum; z < KTNUM; z++) {
        max_rehash = 0;
        for (name = tab, i = z; --i >= 0;)
            *name++ = 0;
        for (i = 0; i < ksnum; i++) {
            name = info[i].name;
            sig = 0;
            while ((c = *name++))
                sig = (sig << 1) + c;
            first = j = sig % z;
            for (k = 0; tab[j]; k++) {
                j += first + 1;
                if (j >= z)
                    j -= z;
                if (j == first)
                    goto next1;
            }
            tab[j] = 1;
            if (k > max_rehash)
                max_rehash = k;
        }
        if (max_rehash < MIN_REHASH) {
            if (max_rehash < best_max_rehash) {
                best_max_rehash = max_rehash;
                best_z = z;
            }
            num_found++;
            if (num_found >= MATCHES)
                break;
        }
next1:  ;
    }

    z = best_z;
    printf("#ifndef KS_TABLES_H\n");
    printf("#define KS_TABLES_H\n\n");
    printf("static const unsigned char _XkeyTable[] = {\n");
    printf("0,\n");
    k = 1;
    for (i = 0; i < ksnum; i++) {
        name = info[i].name;
        sig = 0;
        while ((c = *name++))
            sig = (sig << 1) + c;
        first = j = sig % z;
        while (offsets[j]) {
            j += first + 1;
            if (j >= z)
                j -= z;
        }
        offsets[j] = k;
        indexes[i] = k;
        val = info[i].val;
        printf("0x%.2"PRIx32", 0x%.2"PRIx32", 0x%.2lx, 0x%.2lx, 0x%.2lx, 0x%.2lx, ",
               (sig >> 8) & 0xff, sig & 0xff,
               (val >> 24) & 0xff, (val >> 16) & 0xff,
               (val >> 8) & 0xff, val & 0xff);
        for (name = info[i].name, k += 7; (c = *name++); k++)
            printf("'%c',", c);
        printf((i == (ksnum-1)) ? "0\n" : "0,\n");
    }
    printf("};\n");
    printf("\n");
    printf("#define KTABLESIZE %d\n", z);
    printf("#define KMAXHASH %d\n", best_max_rehash + 1);
    printf("\n");
    printf("static const unsigned short hashString[KTABLESIZE] = {\n");
    for (i = 0; i < z;) {
        printf("0x%.4x", offsets[i]);
        i++;
        if (i == z)
            break;
        printf((i & 7) ? ", " : ",\n");
    }
    printf("\n");
    printf("};\n");

    best_max_rehash = ksnum;
    num_found = 0;
    for (z = ksnum; z < KTNUM; z++) {
        max_rehash = 0;
        for (name = tab, i = z; --i >= 0;)
            *name++ = 0;
        for (i = 0; i < ksnum; i++) {
            val = info[i].val;
            first = j = val % z;
            for (k = 0; tab[j]; k++) {
                if (values[j] == val)
                    goto skip1;
                j += first + 1;
                if (j >= z)
                    j -= z;
                if (j == first)
                    goto next2;
            }
            tab[j] = 1;
            values[j] = val;
            if (k > max_rehash)
                max_rehash = k;
skip1:  ;
        }
        if (max_rehash < MIN_REHASH) {
            if (max_rehash < best_max_rehash) {
                best_max_rehash = max_rehash;
                best_z = z;
            }
            num_found++;
            if (num_found >= MATCHES)
                break;
        }
next2:  ;
    }

    z = best_z;
    for (i = z; --i >= 0;)
        offsets[i] = 0;
    for (i = 0; i < ksnum; i++) {
        val = info[i].val;
        first = j = val % z;
        while (offsets[j]) {
            if (values[j] == val)
                goto skip2;
            j += first + 1;
            if (j >= z)
                j -= z;
        }
        offsets[j] = indexes[i] + 2;
        values[j] = val;
skip2:  ;
    }
    printf("\n");
    printf("#define VTABLESIZE %d\n", z);
    printf("#define VMAXHASH %d\n", best_max_rehash + 1);
    printf("\n");
    printf("static const unsigned short hashKeysym[VTABLESIZE] = {\n");
    for (i = 0; i < z;) {
        printf("0x%.4x", offsets[i]);
        i++;
        if (i == z)
            break;
        printf((i & 7) ? ", " : ",\n");
    }
    printf("\n");
    printf("};\n");
    printf("\n#endif /* KS_TABLES_H */\n");

    exit(0);
}
