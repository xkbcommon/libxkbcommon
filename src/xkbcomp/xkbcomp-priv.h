/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */
#pragma once

#include "config.h"

#include "keymap.h"
#include "ast.h"
#include "scanner-utils.h"

char *
text_v1_keymap_get_as_string(struct xkb_keymap *keymap,
                             enum xkb_keymap_format format);

XkbFile *
XkbParseFile(struct xkb_context *ctx, FILE *file,
             const char *file_name, const char *map);
bool
XkbParseStringInit(struct xkb_context *ctx, struct scanner *scanner,
                   const char *string, size_t len,
                   const char *file_name, const char *map);

XkbFile *
XkbParseString(struct xkb_context *ctx,
               const char *string, size_t len,
               const char *file_name, const char *map);

bool
XkbParseStringNext(struct xkb_context *ctx, struct scanner *scanner,
                   const char *map, XkbFile **out);

void
FreeXkbFile(XkbFile *file);

XkbFile *
XkbFileFromComponents(struct xkb_context *ctx,
                      const struct xkb_component_names *kkctgs);

bool
CompileKeycodes(XkbFile *file, struct xkb_keymap *keymap);

bool
CompileKeyTypes(XkbFile *file, struct xkb_keymap *keymap);

bool
CompileCompatMap(XkbFile *file, struct xkb_keymap *keymap);

bool
CompileSymbols(XkbFile *file, struct xkb_keymap *keymap);

bool
CompileKeymap(XkbFile *file, struct xkb_keymap *keymap);

extern const struct xkb_sym_interpret default_interpret;

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
            "Unknown %s field \"%s\" in %s; "
            "Ignoring assignment to unknown field in %s\n",
            type, field, name, name);
    return false;
}

static inline const char*
safe_map_name(XkbFile *file)
{
    return file->name ? file->name : "(unnamed map)";
}
