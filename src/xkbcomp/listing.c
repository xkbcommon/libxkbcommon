/************************************************************
 Copyright 1996 by Silicon Graphics Computer Systems, Inc.

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
/***********************************************************

Copyright 1988, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/keysym.h>

#if defined(sgi)
#include <malloc.h>
#endif

#define	DEBUG_VAR listingDebug
#include "xkbcomp.h"
#include <stdlib.h>

#ifdef _POSIX_SOURCE
# include <limits.h>
#else
# define _POSIX_SOURCE
# include <limits.h>
# undef _POSIX_SOURCE
#endif

#ifndef PATH_MAX
#ifdef WIN32
#define PATH_MAX 512
#else
#include <sys/param.h>
#endif
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif
#endif

#ifdef WIN32
# include <windows.h>
# define FileName(file) file.cFileName
# undef TEXT
# undef ALTERNATE
#else
# include <dirent.h>
# define FileName(file) file->d_name
#endif

#include "xkbmisc.h"
#include "xkbpath.h"
#include "parseutils.h"
#include "misc.h"
#include "tokens.h"
#include <X11/extensions/XKBgeomcommon.h>

#ifndef DFLT_XKB_CONFIG_ROOT
#define DFLT_XKB_CONFIG_ROOT "/usr/share/X11/xkb"
#endif

unsigned int listingDebug;

typedef struct _CompPair {
    int num;
    int sz;

    XkbComponentNamePtr comp;
} CompPair;

/***====================================================================***/

static Bool
AddComponent(CompPair *cp, char *fileName, XkbFile *map, unsigned dirsToStrip)
{
    if (!cp || !fileName || !map)
        return False;

    if (cp->num >= cp->sz) {
        int orig_sz = cp->sz;
        XkbComponentNamePtr orig_comp = cp->comp;

        if (cp->sz < 1)
            cp->sz = 8;
        else
            cp->sz *= 2;

        cp->comp = _XkbRealloc(cp->comp,
                               cp->sz * sizeof(XkbComponentNameRec));
        if (!cp->comp) {
            ERROR("Failed reallocating component name list\n");
            cp->sz = orig_sz;
            cp->comp = orig_comp;
            return False;
        }
    }

    /* Strip off leading directories of component */
    if (dirsToStrip > 0) {
        char *tmp, *last;
        int i;

        for (i = 0, tmp = last = fileName; (i <= dirsToStrip) && tmp; i++) {
            last = tmp;
            tmp = strchr(tmp, '/');
            if (tmp != NULL)
                tmp++;
        }

        fileName = (tmp ? tmp : last);
    }

    if (map->name) {
        size_t len = strlen(fileName) + strlen(map->name) + 3;

        cp->comp[cp->num].name = _XkbAlloc(len * sizeof(char));
        if (!cp->comp[cp->num].name) {
            ERROR("Could not allocate space for component name\n");
            return False;
        }
        sprintf(cp->comp[cp->num].name, "%s(%s)", fileName, map->name);
    }
    else {
        cp->comp[cp->num].name = strdup(fileName);
        if (!cp->comp[cp->num].name) {
            ERROR("Could not duplicate component name\n");
            return False;
        }
    }

    cp->comp[cp->num].flags = map->flags;
    cp->num++;

    return True;
}

static Bool
MapMatches(char *mapToConsider, char *ptrn)
{
    return ptrn ? XkbcNameMatchesPattern(mapToConsider, ptrn) : True;
}

static int
ParseComponents(CompPair *cp, char *path, char *map, int *max, unsigned strip)
{
    int extra = 0;
    FILE *inputFile;
    XkbFile *rtrn, *mapToUse;
    unsigned oldWarningLevel;

    if (!cp || !path || !max)
        return 0;

#ifdef DEBUG
    if (warningLevel > 9)
        fprintf(stderr, "should list:\n");
#endif

    oldWarningLevel = warningLevel;
    warningLevel = 0;

#ifdef DEBUG
    if (oldWarningLevel > 9)
        fprintf(stderr, "%s(%s)\n", path ? path : "*", map ? map : "*");
#endif

    inputFile = fopen(path, "r");
    if (!inputFile) {
        if (oldWarningLevel > 5)
            WARN("Couldn't open \"%s\"\n", path);
        return 0;
    }

    setScanState(path, 1);
    if (!XKBParseFile(inputFile, &rtrn) || !rtrn) {
        if (oldWarningLevel > 5)
            WARN("Couldn't parse file \"%s\"\n", path);
        fclose(inputFile);
        return 0;
    }

    mapToUse = rtrn;
    for (; mapToUse; mapToUse = (XkbFile *)mapToUse->common.next) {
        if (!MapMatches(mapToUse->name, map))
            continue;
        if (*max <= 0) {
            extra++;
            continue;
        }
        if (AddComponent(cp, path, mapToUse, strip))
            (*max)--;
    }

    warningLevel = oldWarningLevel;

    if (extra)
        *max = 0;

    return extra;
}

/***====================================================================***/

static int
AddDirectory(CompPair *cp, char *head, char *ptrn, char *rest, char *map,
             int *max, unsigned strip)
{
#ifdef WIN32
    HANDLE dirh;
    WIN32_FIND_DATA file;
#else
    DIR *dirp;
    struct dirent *file;
#endif
    int nMatch;
    size_t baselen;
    char path[PATH_MAX];

    if (!head) {
        ERROR("Must specify directory name\n");
        return 0;
    }

    strncpy(path, head, sizeof(path));
    path[PATH_MAX - 1] = '\0';
    baselen = strlen(path);

    if (!map) {
        char *tmp = ptrn;

        if (!rest && ptrn && !strchr(ptrn, '/')) {
            tmp = ptrn;
            map = strchr(ptrn, '(');
        }
        else if (!rest && !ptrn && !strchr(head, '/')) {
            tmp = head;
            map = strchr(head, '(');
        }

        if (map) {
            tmp = strchr(tmp, ')');
            if (!tmp || tmp[1] != '\0') {
                ERROR("File and map must have the format file(map)\n");
                return 0;
            }
            *map = '\0';
            map++;
            *tmp = '\0';
        }
    }

#ifdef WIN32
    if ((dirh = FindFirstFile("*.*", &file)) == INVALID_HANDLE_VALUE)
        return 0;
#else
    if (!(dirp = opendir(head))) {
        ERROR("Could not open directory \"%s\"\n", head ? head : ".");
        return 0;
    }
#endif
    nMatch = 0;
#ifdef WIN32
    do
#else
    while ((file = readdir(dirp)) != NULL)
#endif
    {
        char *filename;
        struct stat sbuf;

        filename = FileName(file);
        if (!filename || filename[0] == '.')
            continue;
        if (ptrn && (!XkbcNameMatchesPattern(filename, ptrn)))
            continue;

        snprintf(&path[baselen], sizeof(path) - baselen, "/%s", filename);
        if (stat(path, &sbuf) < 0) {
            ERROR("Could not read file \"%s\"\n", path);
            continue;
        }

        if ((rest && !S_ISDIR(sbuf.st_mode)) ||
            (map && S_ISDIR(sbuf.st_mode)))
            continue;

        if (S_ISDIR(sbuf.st_mode))
            nMatch += AddDirectory(cp, path, rest, NULL, map, max, strip);
        else
            nMatch += ParseComponents(cp, path, map, max, strip);
    }
#ifdef WIN32
    while (FindNextFile(dirh, &file));
#endif
    return nMatch;
}

/***====================================================================***/

static int
GenerateComponent(XkbComponentListPtr complist, unsigned type, char *head_in,
                 char *base, int *max)
{
    char *str, *head, *ptrn = NULL, *rest = NULL;
    char buf[PATH_MAX];
    size_t len;
    CompPair cp;
    unsigned dirsToStrip;
    int extra;

    if (!complist || !head_in || !max)
        return 0;

    ptrn = NULL;
    for (str = head_in; (*str != '\0') && (*str != '?') && (*str != '*');
         str++)
    {
        if ((str != head_in) && (*str == '/'))
            ptrn = str;
    }

    if (*str == '\0') {
        /* no wildcards */
        head = head_in;
    }
    else if (!ptrn) {
        /* no slash before the first wildcard */
        head = NULL;
        ptrn = head_in;
    }
    else {
        /* slash followed by wildcard */
        head = head_in;
        *ptrn = '\0';
        ptrn++;
    }

    if (ptrn) {
        rest = strchr(ptrn, '/');
        if (rest) {
            *rest = '\0';
            rest++;
        }
    }

    if ((rest && ptrn && (strchr(ptrn, '(') || strchr(ptrn, ')'))) ||
        (head && (strchr(head, '(') || strchr(head, ')')))) {
        ERROR("Files/maps to list must have the form file(map)\n");
        return 0;
    }

    /* Prepend XKB directory */
    snprintf(buf, sizeof(buf), "%s%s%s", base ? base : "", base ? "/" : "",
             XkbDirectoryForInclude(type));
    len = strlen(buf);

    /* Figure out directory stripping */
    str = buf;
    dirsToStrip = 0;
    while ((str = strchr(str, '/')) != NULL) {
        str++;
        dirsToStrip++;
    }

    if (head)
        snprintf(&buf[len], PATH_MAX - len, "/%s", head);
    head = buf;

    memset(&cp, 0, sizeof(CompPair));
    extra = AddDirectory(&cp, head, ptrn, rest, NULL, max, dirsToStrip);

    /* Trim excess component slots */
    if (cp.sz > 0 && cp.sz > cp.num) {
        if (_XkbRealloc(cp.comp, cp.num * sizeof(XkbComponentNameRec)))
            cp.sz = cp.num;
        else
            WARN("Could not reallocate component name list\n");
    }

    switch (type) {
    case XkmKeymapFile:
        complist->num_keymaps = cp.num;
        complist->keymaps = cp.comp;
        break;
    case XkmKeyNamesIndex:
        complist->num_keycodes = cp.num;
        complist->keycodes = cp.comp;
        break;
    case XkmTypesIndex:
        complist->num_types = cp.num;
        complist->types = cp.comp;
        break;
    case XkmCompatMapIndex:
        complist->num_compat = cp.num;
        complist->compat = cp.comp;
        break;
    case XkmSymbolsIndex:
        complist->num_symbols = cp.num;
        complist->symbols = cp.comp;
        break;
    case XkmGeometryIndex:
        complist->num_geometry = cp.num;
        complist->geometry = cp.comp;
        break;
    }

    return extra;
}

/***====================================================================***/

XkbComponentListPtr
XkbcListComponents(unsigned int deviceSpec, XkbComponentNamesPtr ptrns,
                   int *maxMatch)
{
    XkbComponentListPtr complist = NULL;
    int extra = 0;

    complist = _XkbTypedCalloc(1, XkbComponentListRec);
    if (!complist) {
        ERROR("could not allocate space for listing\n");
        goto out;
    }

    if (!maxMatch || *maxMatch <= 0)
        goto out;

    if (ptrns->keymap && *ptrns->keymap != '\0')
        extra += GenerateComponent(complist, XkmKeymapFile, ptrns->keycodes,
                                   DFLT_XKB_CONFIG_ROOT, maxMatch);

    if (ptrns->keycodes && *ptrns->keycodes != '\0')
        extra += GenerateComponent(complist, XkmKeyNamesIndex, ptrns->keycodes,
                                   DFLT_XKB_CONFIG_ROOT, maxMatch);

    if (ptrns->types && *ptrns->types != '\0')
        extra += GenerateComponent(complist, XkmTypesIndex, ptrns->types,
                                   DFLT_XKB_CONFIG_ROOT, maxMatch);

    if (ptrns->compat && *ptrns->compat != '\0')
        extra += GenerateComponent(complist, XkmCompatMapIndex, ptrns->compat,
                                   DFLT_XKB_CONFIG_ROOT, maxMatch);

    if (ptrns->symbols && *ptrns->symbols != '\0')
        extra += GenerateComponent(complist, XkmSymbolsIndex, ptrns->symbols,
                                   DFLT_XKB_CONFIG_ROOT, maxMatch);

    if (ptrns->geometry && *ptrns->geometry != '\0')
        extra += GenerateComponent(complist, XkmGeometryIndex, ptrns->geometry,
                                   DFLT_XKB_CONFIG_ROOT, maxMatch);

out:
    if (maxMatch)
        *maxMatch = extra;
    return complist;
}
