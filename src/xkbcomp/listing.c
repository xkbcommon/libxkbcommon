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

typedef struct _Listing {
    char *file;
    char *map;
} Listing;

typedef struct _ListHead {
    int szListing;
    int nListed;

    Listing *list;
} ListHead;

typedef struct _CompPair {
    int num;
    int sz;

    XkbComponentNamePtr comp;
} CompPair;

/***====================================================================***/

static void
ClearList(ListHead *lh)
{
    int i;

    if (!lh)
        return;

    for (i = 0; i < lh->nListed; i++) {
        _XkbFree(lh->list[i].file);
        lh->list[i].file = NULL;
        _XkbFree(lh->list[i].map);
        lh->list[i].map = NULL;
    }

    lh->nListed = 0;
}

static void
FreeList(ListHead *lh)
{
    int i;

    if (!lh)
        return;

    for (i = 0; i < lh->nListed; i++) {
        _XkbFree(lh->list[i].file);
        _XkbFree(lh->list[i].map);
    }

    _XkbFree(lh->list);

    memset(lh, 0, sizeof(ListHead));
}

static Bool
AddListing(ListHead *lh, char *file, char *map)
{
    if (lh->nListed >= lh->szListing) {
        int orig_sz = lh->szListing;
        Listing *orig_list = lh->list;

        if (lh->szListing < 1)
            lh->szListing = 10;
        else
            lh->szListing *= 2;

        lh->list = _XkbRealloc(lh->list, lh->szListing * sizeof(Listing));
        if (!lh->list) {
            ERROR("Couldn't allocate list of files and maps\n");
            lh->szListing = orig_sz;
            lh->list = orig_list;
            return False;
        }
    }

    lh->list[lh->nListed].file = file;
    lh->list[lh->nListed].map = map;
    lh->nListed++;

    return True;
}

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
            cp->sz = 10;
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

/***====================================================================***/

static int
AddDirectory(ListHead *lh, char *head, char *ptrn, char *rest, char *map)
{
#ifdef WIN32
    HANDLE dirh;
    WIN32_FIND_DATA file;
#else
    DIR *dirp;
    struct dirent *file;
#endif
    int nMatch;

    if (map == NULL)
    {
        char *tmp = ptrn;
        if ((rest == NULL) && (ptrn != NULL) && (strchr(ptrn, '/') == NULL))
        {
            tmp = ptrn;
            map = strchr(ptrn, '(');
        }
        else if ((rest == NULL) && (ptrn == NULL) &&
                 (head != NULL) && (strchr(head, '/') == NULL))
        {
            tmp = head;
            map = strchr(head, '(');
        }
        if (map != NULL)
        {
            tmp = strchr(tmp, ')');
            if ((tmp == NULL) || (tmp[1] != '\0'))
            {
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
    if ((dirp = opendir((head ? head : "."))) == NULL)
        return 0;
    nMatch = 0;
#endif
#ifdef WIN32
    do
#else
    while ((file = readdir(dirp)) != NULL)
#endif
    {
        char *tmp, *filename;
        struct stat sbuf;

        filename = FileName(file);
        if (!filename || filename[0] == '.')
            continue;
        if (ptrn && (!XkbcNameMatchesPattern(filename, ptrn)))
            continue;
        tmp = _XkbAlloc((head ? strlen(head) : 0) + strlen(filename) + 2);
        if (!tmp) {
            ERROR("Could not allocate space for file listing\n");
            continue;
        }
        sprintf(tmp, "%s%s%s", (head ? head : ""), (head ? "/" : ""),
                filename);
        if (stat(tmp, &sbuf) < 0)
        {
            uFree(tmp);
            continue;
        }
        if (((rest != NULL) && (!S_ISDIR(sbuf.st_mode))) ||
            ((map != NULL) && (S_ISDIR(sbuf.st_mode))))
        {
            uFree(tmp);
            continue;
        }
        if (S_ISDIR(sbuf.st_mode))
            nMatch += AddDirectory(lh, tmp, rest, NULL, map);
        else
            nMatch += AddListing(lh, tmp, map);
    }
#ifdef WIN32
    while (FindNextFile(dirh, &file));
#endif
    return nMatch;
}

/***====================================================================***/

static int
AddMatchingFiles(ListHead *lh, char *head_in, char *base, unsigned type)
{
    char *str, *head, *ptrn, *rest = NULL;
    char buf[PATH_MAX];
    size_t len;

    if (head_in == NULL)
        return 0;

    ptrn = NULL;
    for (str = head_in; (*str != '\0') && (*str != '?') && (*str != '*');
         str++)
    {
        if ((str != head_in) && (*str == '/'))
            ptrn = str;
    }
    if (*str == '\0')
    {                           /* no wildcards */
        head = head_in;
        ptrn = NULL;
        rest = NULL;
    }
    else if (ptrn == NULL)
    {                           /* no slash before the first wildcard */
        head = NULL;
        ptrn = head_in;
    }
    else
    {                           /* slash followed by wildcard */
        head = head_in;
        *ptrn = '\0';
        ptrn++;
    }

    if (ptrn)
    {
        rest = strchr(ptrn, '/');
        if (rest != NULL)
        {
            *rest = '\0';
            rest++;
        }
    }

    if (((rest && ptrn)
         && ((strchr(ptrn, '(') != NULL) || (strchr(ptrn, ')') != NULL)))
        || (head
            && ((strchr(head, '(') != NULL) || (strchr(head, ')') != NULL))))
    {
        ERROR("Files/maps to list must have the form file(map)\n");
        ACTION("Illegal specifier ignored\n");
        return 0;
    }

    /* Prepend XKB directory */
    snprintf(buf, PATH_MAX, "%s%s%s", base ? base : "", base ? "/" : "",
             XkbDirectoryForInclude(type));
    len = strlen(buf);
    if (head)
        snprintf(&buf[len], PATH_MAX - len, "/%s", head);
    head = buf;

    return AddDirectory(lh, head, ptrn, rest, NULL);
}

/***====================================================================***/

static Bool
MapMatches(char *mapToConsider, char *ptrn)
{
    return ptrn ? XkbcNameMatchesPattern(mapToConsider, ptrn) : True;
}

static int
GenerateComponents(ListHead *lh, XkbComponentListPtr complist, unsigned type,
                   int *max, unsigned strip)
{
    int i, extra = 0;
    FILE *inputFile;
    CompPair cp = { 0 };
    XkbFile *rtrn, *mapToUse;
    unsigned oldWarningLevel;
    char *mapName;

    if (!lh || !complist || !max) {
        ERROR("Missing arguments to GenerateComponents\n");
        return 0;
    }

#ifdef DEBUG
    if (warningLevel > 9)
        fprintf(stderr, "should list:\n");
#endif

    oldWarningLevel = warningLevel;
    warningLevel = 0;

    for (i = 0; i < lh->nListed; i++) {
        struct stat sbuf;

#ifdef DEBUG
        if (oldWarningLevel > 9) {
            fprintf(stderr, "%s(%s)\n",
                    (lh->list[i].file ? lh->list[i].file : "*"),
                    (lh->list[i].map ? lh->list[i].map : "*"));
        }
#endif

        if (!lh->list[i].file)
            continue;

        if (stat(lh->list[i].file, &sbuf) < 0) {
            if (oldWarningLevel > 5)
                WARN("Couldn't open \"%s\"\n", lh->list[i].file);
            continue;
        }

        if (S_ISDIR(sbuf.st_mode))
            continue;

        inputFile = fopen(lh->list[i].file, "r");
        if (!inputFile) {
            if (oldWarningLevel > 5)
                WARN("Couldn't open \"%s\"\n", lh->list[i].file);
            continue;
        }

        setScanState(lh->list[i].file, 1);
        if (!XKBParseFile(inputFile, &rtrn) || !rtrn) {
            if (oldWarningLevel > 5)
                WARN("Couldn't parse file \"%s\"\n", lh->list[i].file);
            continue;
        }

        mapName = lh->list[i].map;
        mapToUse = rtrn;
        for (; mapToUse; mapToUse = (XkbFile *)mapToUse->common.next) {
            if (!MapMatches(mapToUse->name, mapName))
                continue;
            if (cp.num >= *max) {
                extra++;
                continue;
            }
            AddComponent(&cp, lh->list[i].file, mapToUse, strip);
        }
    }
    warningLevel = oldWarningLevel;

    /* Trim excess component slots */
    if (cp.sz > 0 && cp.sz > cp.num) {
        if (_XkbRealloc(cp.comp, cp.num * sizeof(XkbComponentNameRec)))
            cp.sz = cp.num;
        else
            WARN("Could not reallocate component name list\n");
    }

    if (extra)
        *max = 0;
    else
        *max -= cp.num;

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

XkbComponentListPtr
XkbcListComponents(unsigned int deviceSpec, XkbComponentNamesPtr ptrns,
                   int *maxMatch)
{
    XkbComponentListPtr complist = NULL;
    char *cur;
    ListHead lh = { 0 };
    int extra = 0;
    unsigned dirsToStrip;

    complist = _XkbTypedCalloc(1, XkbComponentListRec);
    if (!complist) {
        ERROR("could not allocate space for listing\n");
        goto out;
    }

    if (!maxMatch || *maxMatch <= 0)
        goto out;

    /* Figure out directory stripping (including 1 for type directory) */
    cur = DFLT_XKB_CONFIG_ROOT;
    dirsToStrip = 1;
    while ((cur = strchr(cur, '/')) != NULL) {
        cur++;
        dirsToStrip++;
    }

    if (ptrns->keymap) {
        AddMatchingFiles(&lh, ptrns->keymap, DFLT_XKB_CONFIG_ROOT,
                         XkmKeymapFile);
        extra += GenerateComponents(&lh, complist, XkmKeymapFile,
                                    maxMatch, dirsToStrip);
        ClearList(&lh);
    }

    if (ptrns->keycodes) {
        AddMatchingFiles(&lh, ptrns->keycodes, DFLT_XKB_CONFIG_ROOT,
                         XkmKeyNamesIndex);
        extra += GenerateComponents(&lh, complist, XkmKeyNamesIndex,
                                    maxMatch, dirsToStrip);
        ClearList(&lh);
    }

    if (ptrns->types) {
        AddMatchingFiles(&lh, ptrns->types, DFLT_XKB_CONFIG_ROOT,
                         XkmTypesIndex);
        extra += GenerateComponents(&lh, complist, XkmTypesIndex,
                                    maxMatch, dirsToStrip);
        ClearList(&lh);
    }

    if (ptrns->compat) {
        AddMatchingFiles(&lh, ptrns->compat, DFLT_XKB_CONFIG_ROOT,
                         XkmCompatMapIndex);
        extra += GenerateComponents(&lh, complist, XkmCompatMapIndex,
                                    maxMatch, dirsToStrip);
        ClearList(&lh);
    }

    if (ptrns->symbols) {
        AddMatchingFiles(&lh, ptrns->symbols, DFLT_XKB_CONFIG_ROOT,
                         XkmSymbolsIndex);
        extra += GenerateComponents(&lh, complist, XkmSymbolsIndex,
                                    maxMatch, dirsToStrip);
        ClearList(&lh);
    }

    if (ptrns->geometry) {
        AddMatchingFiles(&lh, ptrns->geometry, DFLT_XKB_CONFIG_ROOT,
                         XkmGeometryIndex);
        extra += GenerateComponents(&lh, complist, XkmGeometryIndex,
                                    maxMatch, dirsToStrip);
        ClearList(&lh);
    }

    FreeList(&lh);
out:
    if (maxMatch)
        *maxMatch = extra;
    return complist;
}
