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

#include "X11/extensions/XKBcommon.h"
#include <X11/extensions/XKM.h>
#include "xkbcomp.h"
#include "parseutils.h"
#include "utils.h"

static int
XkbFileFromComponents(const XkbComponentNamesPtr ktcsg, XkbFile **file)
{
    XkbFile *keycodes, *types, *compat, *symbols, *geometry;
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

    inc = IncludeCreate(ktcsg->geometry, MergeDefault);
    geometry = CreateXKBFile(XkmGeometryIndex, NULL, (ParseCommon *)inc, 0);
    AppendStmt(&keycodes->common, &geometry->common);

    *file = CreateXKBFile(XkmKeymapFile, ktcsg->keymap, &keycodes->common, 0);

    return 1;
}

XkbcDescPtr
XkbcCompileKeymap(XkbComponentNamesPtr ktcsg)
{
    XkbFile *file, *mapToUse;
    XkbcDescPtr xkb;

    if (!XkbFileFromComponents(ktcsg, &file)) {
        ERROR("failed to generate parsed XKB file from components");
        goto fail;
    }

    /* Find map to use */
    mapToUse = file;
    if (file->common.next) {
        for (; mapToUse; mapToUse = (XkbFile *)mapToUse->common.next) {
            if (mapToUse->flags & XkbLC_Default)
                break;
        }
        if (!mapToUse) {
            mapToUse = file;
            WARN("no map specified, but components have several");
        }
    }

    /* Compile the keyboard */
    if (!(xkb = XkbcAllocKeyboard())) {
        ERROR("Could not allocate keyboard description");
        goto unwind_file;
    }

    if (!CompileKeymap(mapToUse, xkb, MergeReplace)) {
        ERROR("Failed to compile keymap");
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
