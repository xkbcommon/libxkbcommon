/*
 * Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.
 * SPDX-License-Identifier: HPND
 */
#pragma once

#include "config.h"

#include "xkbcommon/xkbcommon.h"
#include "ast.h"
#include "darray.h"
#include "keymap.h"
#include "scanner-utils.h"
#include "text.h"

enum xkb_parser_strict_flags {
    PARSER_NO_STRICT_FLAGS = 0,

    PARSER_V1_STRICT_FLAGS = ((PARSER_NO_STRICT_FLAGS << 1) - 1),
    /* Limited flexibility */
    PARSER_V1_LAX_FLAGS = PARSER_NO_STRICT_FLAGS,

    PARSER_V2_STRICT_FLAGS = PARSER_V1_STRICT_FLAGS,
    PARSER_V2_LAX_FLAGS = PARSER_NO_STRICT_FLAGS,
};

typedef union ExprDef ExprDef;
struct pending_computation {
    ExprDef * expr;
    bool computed;
    uint32_t value;
};
typedef darray(struct pending_computation) pending_computation_array;

/** Keymap augmented with miscellanenous data used during compilation */
struct xkb_keymap_info {
    /** The keymap being compiled */
    struct xkb_keymap keymap;

    /** Flags of the strict mode */
    enum xkb_parser_strict_flags strict;

    /** Features */
    struct {
        /** Maximum groups for the keymap format */
        xkb_layout_index_t max_groups;
        /* Actions */
        bool group_lock_on_release;
        bool mods_unlock_on_press;
        bool mods_latch_on_press;
    } features;

    /* Non-static LUTs */
    struct {
        LookupEntry groupIndexNames[3];
        LookupEntry groupMaskNames[5];
    } lookup;

    /** Pending computations */
    pending_computation_array *pending_computations;
};

char *
text_v1_keymap_get_as_string(struct xkb_keymap *keymap,
                             enum xkb_keymap_format format,
                             enum xkb_keymap_serialize_flags flags);

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
CompileKeycodes(XkbFile *file, struct xkb_keymap_info *keymap_info);

bool
CompileKeyTypes(XkbFile *file, struct xkb_keymap_info *keymap_info);

bool
CompileCompatMap(XkbFile *file, struct xkb_keymap_info *keymap_info);

bool
CompileSymbols(XkbFile *file, struct xkb_keymap_info *keymap_info);

bool
CompileKeymap(XkbFile *file, struct xkb_keymap *keymap);

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
ReportBadType(struct xkb_context *ctx, enum xkb_message_code code,
              const char *type, const char *field, const char *name,
              const char *wanted)
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
