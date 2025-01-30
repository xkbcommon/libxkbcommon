/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */

#ifndef XKBCOMP_PRIV_H
#define XKBCOMP_PRIV_H

#include "keymap.h"
#include "ast.h"

struct xkb_component_names {
    char *keycodes;
    char *types;
    char *compat;
    char *symbols;
};

char *
text_v1_keymap_get_as_string(struct xkb_keymap *keymap);

XkbFile *
XkbParseFile(struct xkb_context *ctx, FILE *file,
             const char *file_name, const char *map);

XkbFile *
XkbParseString(struct xkb_context *ctx,
               const char *string, size_t len,
               const char *file_name, const char *map);

void
FreeXkbFile(XkbFile *file);

XkbFile *
XkbFileFromComponents(struct xkb_context *ctx,
                      const struct xkb_component_names *kkctgs);

bool
CompileKeycodes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge);

bool
CompileKeyTypes(XkbFile *file, struct xkb_keymap *keymap,
                enum merge_mode merge);

bool
CompileCompatMap(XkbFile *file, struct xkb_keymap *keymap,
                 enum merge_mode merge);

bool
CompileSymbols(XkbFile *file, struct xkb_keymap *keymap,
               enum merge_mode merge);

bool
CompileKeymap(XkbFile *file, struct xkb_keymap *keymap,
              enum merge_mode merge);

/***====================================================================***/

static inline bool
ReportNotArray(struct xkb_context *ctx, const char *type, const char *field,
               const char *name)
{
    log_err(ctx, XKB_ERROR_WRONG_FIELD_TYPE,
            "The %s %s field is not an array; "
            "Ignoring illegal assignment in %s\n",
            type, field, name);
    return false;
}

static inline bool
ReportShouldBeArray(struct xkb_context *ctx, const char *type,
                    const char *field, const char *name)
{
    log_err(ctx, XKB_ERROR_EXPECTED_ARRAY_ENTRY,
            "Missing subscript for %s %s; "
            "Ignoring illegal assignment in %s\n",
            type, field, name);
    return false;
}

static inline bool
ReportBadType(struct xkb_context *ctx, xkb_message_code_t code, const char *type,
              const char *field, const char *name, const char *wanted)
{
    log_err(ctx, code,
            "The %s %s field must be a %s; "
            "Ignoring illegal assignment in %s\n",
            type, field, wanted, name);
    return false;
}

static inline bool
ReportBadField(struct xkb_context *ctx, const char *type, const char *field,
               const char *name)
{
    log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
            "Unknown %s field %s in %s; "
            "Ignoring assignment to unknown field in %s\n",
            type, field, name, name);
    return false;
}

#endif
