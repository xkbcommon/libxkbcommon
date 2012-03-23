/*
Copyright 2009  Dan Nicholson

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the authors or their
institutions shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the authors.
*/

#include <limits.h>
#include "xkbcomp.h"
#include "xkballoc.h"
#include "xkbrules.h"
#include "xkbpath.h"
#include "xkbmisc.h"
#include "parseutils.h"
#include "utils.h"

/* Global warning level */
unsigned int warningLevel = 0;

#define ISEMPTY(str) (!(str) || (strlen(str) == 0))

static XkbFile *
XkbKeymapFileFromComponents(const struct xkb_component_names * ktcsg)
{
    XkbFile *keycodes, *types, *compat, *symbols;
    IncludeStmt *inc;

    inc = IncludeCreate(ktcsg->keycodes, MergeDefault);
    keycodes = CreateXKBFile(XkmKeyNamesIndex, NULL, (ParseCommon *)inc, 0);

    inc = IncludeCreate(ktcsg->types, MergeDefault);
    types = CreateXKBFile(XkmTypesIndex, NULL, (ParseCommon *)inc, 0);
    AppendStmt(&keycodes->common, &types->common);

    inc = IncludeCreate(ktcsg->compat, MergeDefault);
    compat = CreateXKBFile(XkmCompatMapIndex, NULL, (ParseCommon *)inc, 0);
    AppendStmt(&keycodes->common, &compat->common);

    inc = IncludeCreate(ktcsg->symbols, MergeDefault);
    symbols = CreateXKBFile(XkmSymbolsIndex, NULL, (ParseCommon *)inc, 0);
    AppendStmt(&keycodes->common, &symbols->common);

    return CreateXKBFile(XkmKeymapFile, ktcsg->keymap ? ktcsg->keymap : strdup(""),
                         &keycodes->common, 0);
}

static struct xkb_component_names *
XkbComponentsFromRules(const char *rules, const XkbRF_VarDefsPtr defs)
{
    FILE *rulesFile = NULL;
    char *rulesPath = NULL;
    XkbRF_RulesPtr loaded = NULL;
    struct xkb_component_names * names = NULL;

    rulesFile = XkbFindFileInPath(rules, XkmRulesFile, &rulesPath);
    if (!rulesFile) {
        ERROR("could not find \"%s\" rules in XKB path\n", rules);
        return NULL;
    }

    if (!(loaded = uTypedCalloc(1, XkbRF_RulesRec))) {
        ERROR("failed to allocate XKB rules\n");
        goto unwind_file;
    }

    if (!XkbcRF_LoadRules(rulesFile, loaded)) {
        ERROR("failed to load XKB rules \"%s\"\n", rulesPath);
        goto unwind_file;
    }

    if (!(names = uTypedCalloc(1, struct xkb_component_names))) {
        ERROR("failed to allocate XKB components\n");
        goto unwind_file;
    }

    if (!XkbcRF_GetComponents(loaded, defs, names)) {
        free(names->keymap);
        free(names->keycodes);
        free(names->types);
        free(names->compat);
        free(names->symbols);
        free(names);
        names = NULL;
        ERROR("no components returned from XKB rules \"%s\"\n", rulesPath);
    }

unwind_file:
    XkbcRF_Free(loaded);
    if (rulesFile)
        fclose(rulesFile);
    free(rulesPath);
    return names;
}

struct xkb_desc *
xkb_map_new_from_names(const struct xkb_rule_names *rmlvo)
{
    XkbRF_VarDefsRec defs;
    struct xkb_component_names * names;
    struct xkb_desc * xkb;

    if (!rmlvo || ISEMPTY(rmlvo->rules) || ISEMPTY(rmlvo->layout)) {
        ERROR("rules and layout required to generate XKB keymap\n");
        return NULL;
    }

    defs.model = rmlvo->model;
    defs.layout = rmlvo->layout;
    defs.variant = rmlvo->variant;
    defs.options = rmlvo->options;

    names = XkbComponentsFromRules(rmlvo->rules, &defs);
    if (!names) {
        ERROR("failed to generate XKB components from rules \"%s\"\n",
              rmlvo->rules);
        return NULL;
    }

    xkb = xkb_map_new_from_kccgst(names);

    free(names->keymap);
    free(names->keycodes);
    free(names->types);
    free(names->compat);
    free(names->symbols);
    free(names);

    return xkb;
}

static XkbFile *
XkbChooseMap(XkbFile *file, const char *name)
{
    XkbFile *map = file;

    /* map specified? */
    if (name) {
        while (map) {
            if (map->name && strcmp(map->name, name) == 0)
                break;
            map = (XkbFile *) map->common.next;
        }

        if (!map)
            ERROR("no map named \"%s\" in input file\n", name);
    }
    else if (file->common.next) {
        /* look for map with XkbLC_Default flag. */
        for (; map; map = (XkbFile *) map->common.next) {
            if (map->flags & XkbLC_Default)
                break;
        }

        if (!map) {
            map = file;
            WARN("no map specified, but components have several\n");
            WARN("using the first defined map, \"%s\"\n",
                 map->name ? map->name : "");
        }
    }

    return map;
}

static struct xkb_desc *
compile_keymap(XkbFile *file)
{
    XkbFile *mapToUse;
    struct xkb_desc * xkb = NULL;

    /* Find map to use */
    mapToUse = XkbChooseMap(file, NULL);
    if (!mapToUse)
        goto err;

    switch (mapToUse->type) {
    case XkmSemanticsFile:
    case XkmLayoutFile:
    case XkmKeymapFile:
        break;
    default:
        ERROR("file type %d not handled\n", mapToUse->type);
        goto err;
    }

    xkb = CompileKeymap(mapToUse, MergeReplace);
    if (!xkb)
        goto err;

err:
    FreeXKBFile(file);
    free(scanFile);
    XkbFreeIncludePath();
    XkbcFreeAllAtoms();
    return xkb;
}

struct xkb_desc *
xkb_map_new_from_kccgst(const struct xkb_component_names *kccgst)
{
    XkbFile *file;

    if (!kccgst) {
        ERROR("no components specified\n");
        return NULL;
    }

    if (ISEMPTY(kccgst->keycodes)) {
        ERROR("keycodes required to generate XKB keymap\n");
        return NULL;
    }

    if (ISEMPTY(kccgst->compat)) {
        ERROR("compat map required to generate XKB keymap\n");
        return NULL;
    }

    if (ISEMPTY(kccgst->types)) {
        ERROR("types required to generate XKB keymap\n");
        return NULL;
    }

    if (ISEMPTY(kccgst->symbols)) {
        ERROR("symbols required to generate XKB keymap\n");
        return NULL;
    }

    if (!(file = XkbKeymapFileFromComponents(kccgst))) {
        ERROR("failed to generate parsed XKB file from components\n");
        return NULL;
    }

    return compile_keymap(file);
}

struct xkb_desc *
xkb_map_new_from_string(const char *string, enum xkb_keymap_format format)
{
    XkbFile *file;

    if (format != XKB_KEYMAP_FORMAT_TEXT_V1) {
        ERROR("unsupported keymap format %d\n", format);
        return NULL;
    }

    if (!string) {
        ERROR("no string specified to generate XKB keymap\n");
        return NULL;
    }

    setScanState("input", 1);
    if (!XKBParseString(string, &file) || !file) {
        ERROR("failed to parse input xkb file\n");
        return NULL;
    }

    return compile_keymap(file);
}

struct xkb_desc *
xkb_map_new_from_fd(int fd, enum xkb_keymap_format format)
{
    XkbFile *file;
    FILE *fptr;

    if (format != XKB_KEYMAP_FORMAT_TEXT_V1) {
        ERROR("unsupported keymap format %d\n", format);
        return NULL;
    }

    if (fd < 0) {
        ERROR("no file specified to generate XKB keymap\n");
	return NULL;
    }

    fptr = fdopen(fd, "r");
    if (!fptr) {
        ERROR("couldn't associate fd with file pointer\n");
        return NULL;
    }

    setScanState("input", 1);
    if (!XKBParseFile(fptr, &file) || !file) {
        ERROR("failed to parse input xkb file\n");
	return NULL;
    }

    return compile_keymap(file);
}

void
xkb_map_ref(struct xkb_desc *xkb)
{
    xkb->refcnt++;
}

void
xkb_map_unref(struct xkb_desc *xkb)
{
    if (--xkb->refcnt > 0)
        return;

    XkbcFreeKeyboard(xkb);
}
