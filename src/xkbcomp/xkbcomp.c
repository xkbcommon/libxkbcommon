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
#include "parseutils.h"
#include "utils.h"

/* Global debugging flags */
unsigned int debugFlags = 0;
unsigned int warningLevel = 0;

#define ISEMPTY(str) (!(str) || (strlen(str) == 0))

static XkbFile *
XkbKeymapFileFromComponents(const struct xkb_component_names * ktcsg)
{
    XkbFile *keycodes, *types, *compat, *symbols, *geometry;
    IncludeStmt *inc;

    if (!ktcsg) {
        ERROR("no components to generate keymap file from\n");
        return NULL;
    }

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

    inc = IncludeCreate(ktcsg->geometry, MergeDefault);
    geometry = CreateXKBFile(XkmGeometryIndex, NULL, (ParseCommon *)inc, 0);
    AppendStmt(&keycodes->common, &geometry->common);

    return CreateXKBFile(XkmKeymapFile, ktcsg->keymap ? ktcsg->keymap : "",
                         &keycodes->common, 0);
}

static struct xkb_component_names *
XkbComponentsFromRules(const char *rules, const XkbRF_VarDefsPtr defs)
{
    FILE *rulesFile = NULL;
    char *rulesPath = NULL;
    static XkbRF_RulesPtr loaded = NULL;
    static char *cached_name = NULL;
    struct xkb_component_names * names = NULL;

    if (!cached_name || strcmp(rules, cached_name) != 0) {
        if (loaded)
            XkbcRF_Free(loaded, True);
        loaded = NULL;
        free(cached_name);
        cached_name = NULL;
    }

    if (!loaded) {
        rulesFile = XkbFindFileInPath((char *)rules, XkmRulesFile, &rulesPath);
        if (!rulesFile) {
            ERROR("could not find \"%s\" rules in XKB path\n", rules);
            goto out;
        }

        if (!(loaded = _XkbTypedCalloc(1, XkbRF_RulesRec))) {
            ERROR("failed to allocate XKB rules\n");
            goto unwind_file;
        }

        if (!XkbcRF_LoadRules(rulesFile, loaded)) {
            ERROR("failed to load XKB rules \"%s\"\n", rulesPath);
            goto unwind_file;
        }

        cached_name = strdup(rules);
    }

    if (!(names = _XkbTypedCalloc(1, struct xkb_component_names))) {
        ERROR("failed to allocate XKB components\n");
        goto unwind_file;
    }

    if (!XkbcRF_GetComponents(loaded, defs, names)) {
        free(names->keymap);
        free(names->keycodes);
        free(names->types);
        free(names->compat);
        free(names->symbols);
        free(names->geometry);
        free(names);
        names = NULL;
        ERROR("no components returned from XKB rules \"%s\"\n", rulesPath);
    }

unwind_file:
    if (rulesFile)
        fclose(rulesFile);
    free(rulesPath);
out:
    return names;
}

struct xkb_desc *
xkb_compile_keymap_from_rules(const struct xkb_rule_names *rmlvo)
{
    XkbRF_VarDefsRec defs;
    struct xkb_component_names * names;
    struct xkb_desc * xkb;

    if (!rmlvo || ISEMPTY(rmlvo->rules) || ISEMPTY(rmlvo->layout)) {
        ERROR("rules and layout required to generate XKB keymap\n");
        return NULL;
    }

    defs.model = (char *) rmlvo->model;
    defs.layout = (char *) rmlvo->layout;
    defs.variant = (char *) rmlvo->variant;
    defs.options = (char *) rmlvo->options;

    names = XkbComponentsFromRules(rmlvo->rules, &defs);
    if (!names) {
        ERROR("failed to generate XKB components from rules \"%s\"\n",
              rmlvo->rules);
        return NULL;
    }

    xkb = xkb_compile_keymap_from_components(names);

    free(names->keymap);
    free(names->keycodes);
    free(names->types);
    free(names->compat);
    free(names->symbols);
    free(names->geometry);
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

struct xkb_desc *
xkb_compile_keymap_from_components(const struct xkb_component_names * ktcsg)
{
    XkbFile *file, *mapToUse;
    struct xkb_desc * xkb;

    uSetErrorFile(NULL);

    if (!ktcsg || ISEMPTY(ktcsg->keycodes)) {
        ERROR("keycodes required to generate XKB keymap\n");
        goto fail;
    }

    if (!(file = XkbKeymapFileFromComponents(ktcsg))) {
        ERROR("failed to generate parsed XKB file from components\n");
        goto fail;
    }

    /* Find map to use */
    if (!(mapToUse = XkbChooseMap(file, NULL)))
        goto unwind_file;

    /* Compile the keyboard */
    if (!(xkb = XkbcAllocKeyboard())) {
        ERROR("could not allocate keyboard description\n");
        goto unwind_file;
    }

    if (!CompileKeymap(mapToUse, xkb, MergeReplace)) {
        ERROR("failed to compile keymap\n");
        goto unwind_xkb;
    }

    return xkb;
unwind_xkb:
    XkbcFreeKeyboard(xkb, XkbAllComponentsMask, True);
unwind_file:
    /* XXX: here's where we would free the XkbFile */
fail:
    return NULL;
}

static struct xkb_desc *
compile_keymap(XkbFile *file, const char *mapName)
{
    XkbFile *mapToUse;
    struct xkb_desc * xkb;

    /* Find map to use */
    if (!(mapToUse = XkbChooseMap(file, mapName)))
        goto unwind_file;

    switch (mapToUse->type) {
    case XkmSemanticsFile:
    case XkmLayoutFile:
    case XkmKeymapFile:
        break;
    default:
        ERROR("file type %d not handled\n", mapToUse->type);
        goto unwind_file;
    }

    /* Compile the keyboard */
    if (!(xkb = XkbcAllocKeyboard())) {
        ERROR("could not allocate keyboard description\n");
        goto unwind_file;
    }

    if (!CompileKeymap(mapToUse, xkb, MergeReplace)) {
        ERROR("failed to compile keymap\n");
        goto unwind_xkb;
    }

    return xkb;
unwind_xkb:
    XkbcFreeKeyboard(xkb, XkbAllComponentsMask, True);
unwind_file:
    /* XXX: here's where we would free the XkbFile */

    return NULL;
}

struct xkb_desc *
xkb_compile_keymap_from_string(const char *string, const char *mapName)
{
    XkbFile *file;

    if (!string) {
        ERROR("no string specified to generate XKB keymap\n");
        return NULL;
    }

    setScanState("input", 1);
    if (!XKBParseString(string, &file) || !file) {
        ERROR("failed to parse input xkb file\n");
        return NULL;
    }

    return compile_keymap(file, mapName);
}

struct xkb_desc *
xkb_compile_keymap_from_file(FILE *inputFile, const char *mapName)
{
    XkbFile *file;

    if (!inputFile) {
        ERROR("no file specified to generate XKB keymap\n");
	return NULL;
    }

    setScanState("input", 1);
    if (!XKBParseFile(inputFile, &file) || !file) {
        ERROR("failed to parse input xkb file\n");
	return NULL;
    }

    return compile_keymap(file, mapName);
}
