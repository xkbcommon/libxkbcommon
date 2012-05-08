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

#include "xkbcomp-priv.h"
#include "rules.h"
#include "parseutils.h"

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

    return CreateXKBFile(XkmKeymapFile,
                         ktcsg->keymap ? ktcsg->keymap : strdup(""),
                         &keycodes->common, 0);
}

static struct xkb_component_names *
XkbComponentsFromRules(struct xkb_context *context,
                       const char *rules,
                       const XkbRF_VarDefsPtr defs)
{
    FILE *rulesFile = NULL;
    char *rulesPath = NULL;
    XkbRF_RulesPtr loaded = NULL;
    struct xkb_component_names * names = NULL;
    int i;

    rulesFile = XkbFindFileInPath(context, rules, XkmRulesFile, &rulesPath);
    if (!rulesFile) {
        ERROR("could not find \"%s\" rules in XKB path\n", rules);
        ERROR("%d include paths searched:\n",
              xkb_context_num_include_paths(context));
        for (i = 0; i < xkb_context_num_include_paths(context); i++)
            ERROR("\t%s\n", xkb_context_include_path_get(context, i));
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

_X_EXPORT struct xkb_keymap *
xkb_map_new_from_names(struct xkb_context *context,
                       const struct xkb_rule_names *rmlvo,
                       enum xkb_map_compile_flags flags)
{
    XkbRF_VarDefsRec defs;
    struct xkb_component_names *names;
    struct xkb_keymap *xkb;

    if (!rmlvo || ISEMPTY(rmlvo->rules) || ISEMPTY(rmlvo->layout)) {
        ERROR("rules and layout required to generate XKB keymap\n");
        return NULL;
    }

    defs.model = rmlvo->model;
    defs.layout = rmlvo->layout;
    defs.variant = rmlvo->variant;
    defs.options = rmlvo->options;

    names = XkbComponentsFromRules(context, rmlvo->rules, &defs);
    if (!names) {
        ERROR("failed to generate XKB components from rules \"%s\"\n",
              rmlvo->rules);
        return NULL;
    }

    xkb = xkb_map_new_from_kccgst(context, names, 0);

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

static struct xkb_keymap *
compile_keymap(struct xkb_context *context, XkbFile *file)
{
    XkbFile *mapToUse;
    struct xkb_keymap * xkb = NULL;

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

    xkb = CompileKeymap(context, mapToUse);
    if (!xkb)
        goto err;

err:
    FreeXKBFile(file);
    XkbcFreeAllAtoms();
    return xkb;
}

_X_EXPORT struct xkb_keymap *
xkb_map_new_from_kccgst(struct xkb_context *context,
                        const struct xkb_component_names *kccgst,
                        enum xkb_map_compile_flags flags)
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

    return compile_keymap(context, file);
}

_X_EXPORT struct xkb_keymap *
xkb_map_new_from_string(struct xkb_context *context,
                        const char *string,
                        enum xkb_keymap_format format,
                        enum xkb_map_compile_flags flags)
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

    if (!XKBParseString(string, "input", &file) || !file) {
        ERROR("failed to parse input xkb file\n");
        return NULL;
    }

    return compile_keymap(context, file);
}

_X_EXPORT struct xkb_keymap *
xkb_map_new_from_fd(struct xkb_context *context,
                    int fd,
                    enum xkb_keymap_format format,
                    enum xkb_map_compile_flags flags)
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

    if (!XKBParseFile(fptr, "(unknown file)", &file) || !file) {
        ERROR("failed to parse input xkb file\n");
	return NULL;
    }

    return compile_keymap(context, file);
}

_X_EXPORT struct xkb_keymap *
xkb_map_ref(struct xkb_keymap *xkb)
{
    xkb->refcnt++;
    return xkb;
}

_X_EXPORT void
xkb_map_unref(struct xkb_keymap *xkb)
{
    if (--xkb->refcnt > 0)
        return;

    XkbcFreeKeyboard(xkb);
}
