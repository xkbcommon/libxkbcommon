/************************************************************
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of Silicon Graphics not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific prior written permission.
 * Silicon Graphics makes no representation about the suitability
 * of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 * GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 * THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ********************************************************/

#ifndef XKBCOMP_PRIV_H
#define XKBCOMP_PRIV_H

#include "xkbcomp.h"
#include "alloc.h"
#include "text.h"
#include "utils.h"

typedef struct _CommonInfo {
    unsigned short defined;
    unsigned file_id;
    enum merge_mode merge;
    struct _CommonInfo *next;
} CommonInfo;

extern bool
UseNewField(unsigned field, CommonInfo *oldDefs, CommonInfo *newDefs,
            unsigned *pCollide);

extern void *
ClearCommonInfo(CommonInfo *cmn);

extern void *
AddCommonInfo(CommonInfo * old, CommonInfo * new);

extern int
ReportNotArray(const char *type, const char *field, const char *name);

extern int
ReportShouldBeArray(const char *type, const char *field, const char *name);

extern int
ReportBadType(const char *type, const char *field, const char *name,
              const char *wanted);

extern int
ReportBadField(const char *type, const char *field, const char *name);

extern bool
ProcessIncludeFile(struct xkb_context *ctx, IncludeStmt *stmt,
                   enum xkb_file_type file_type, XkbFile **file_rtrn,
                   enum merge_mode *merge_rtrn);

extern bool
FindNamedKey(struct xkb_keymap *keymap, unsigned long name,
             xkb_keycode_t *kc_rtrn, bool use_aliases, bool create,
             xkb_keycode_t start_from);

extern bool
FindKeyNameForAlias(struct xkb_keymap *keymap, unsigned long lname,
                    unsigned long *real_name);

extern bool
UpdateModifiersFromCompat(struct xkb_keymap *keymap);

uint32_t
VModsToReal(struct xkb_keymap *keymap, uint32_t vmodmask);

#endif /* XKBCOMP_PRIV_H */
