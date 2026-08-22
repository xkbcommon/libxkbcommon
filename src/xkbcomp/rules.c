/*
 * For HPND:
 * Copyright (c) 1996 by Silicon Graphics Computer Systems, Inc.
 *
 * For MIT:
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * SPDX-License-Identifier: HPND AND MIT
 */

#include "config.h"

#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcomp-priv.h"
#include "context.h"
#include "keymap.h"
#include "messages-codes.h"
#include "rules.h"
#include "rmlvo.h"
#include "include.h"
#include "scanner-utils.h"
#include "darray.h"
#include "utils.h"
#include "utils-numbers.h"
#include "utils-paths.h"

#define MAX_INCLUDE_DEPTH 5

/* Scanner / Lexer */

/* Values returned with some tokens, like yylval. */
union lvalue {
    struct sval string;
};

enum rules_token {
    TOK_END_OF_FILE = 0,
    TOK_END_OF_LINE,
    TOK_IDENTIFIER,
    TOK_GROUP_NAME,
    TOK_BANG,
    TOK_EQUALS,
    TOK_WILD_CARD_STAR,
    TOK_WILD_CARD_NONE,
    TOK_WILD_CARD_SOME,
    TOK_WILD_CARD_ANY,
    TOK_INCLUDE,
    TOK_ERROR
};

static inline bool
is_ident(char ch)
{
    return is_graph(ch) && ch != '\\';
}

static enum rules_token
lex(struct scanner *s, union lvalue *val)
{
skip_more_whitespace_and_comments:
    /* Skip spaces. */
    while (scanner_chr(s, ' ') || scanner_chr(s, '\t') || scanner_chr(s, '\r'));

    /* Skip comments. */
    if (scanner_lit(s, "//")) {
        scanner_skip_to_eol(s);
    }

    /* New line. */
    if (scanner_eol(s)) {
        while (scanner_eol(s)) scanner_next(s);
        return TOK_END_OF_LINE;
    }

    /* Escaped line continuation. */
    if (scanner_chr(s, '\\')) {
        /* Optional \r. */
        scanner_chr(s, '\r');
        if (!scanner_eol(s)) {
            scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                        "illegal new line escape; must appear at end of line");
            return TOK_ERROR;
        }
        scanner_next(s);
        goto skip_more_whitespace_and_comments;
    }

    /* See if we're done. */
    if (scanner_eof(s)) return TOK_END_OF_FILE;

    /* New token. */
    s->token_pos = s->pos;

    /* Operators and punctuation. */
    if (scanner_chr(s, '!')) return TOK_BANG;
    if (scanner_chr(s, '=')) return TOK_EQUALS;

    /* Wild cards */
    if (scanner_chr(s, '*')) return TOK_WILD_CARD_STAR;
    if (scanner_lit(s, "<none>")) return TOK_WILD_CARD_NONE;
    if (scanner_lit(s, "<some>")) return TOK_WILD_CARD_SOME;
    if (scanner_lit(s, "<any>")) return TOK_WILD_CARD_ANY;

    /* Group name. */
    if (scanner_chr(s, '$')) {
        val->string.start = s->s + s->pos;
        val->string.len = 0;
        while (is_ident(scanner_peek(s))) {
            scanner_next(s);
            val->string.len++;
        }
        if (val->string.len == 0) {
            scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                        "unexpected character after \'$\'; expected name");
            return TOK_ERROR;
        }
        return TOK_GROUP_NAME;
    }

    /* Include statement. */
    if (scanner_lit(s, "include"))
        return TOK_INCLUDE;

    /* Identifier. */
    /* Ensure that we can parse KcCGST values with merge modes */
    assert(is_ident(MERGE_OVERRIDE_PREFIX));
    assert(is_ident(MERGE_AUGMENT_PREFIX));
    assert(is_ident(MERGE_REPLACE_PREFIX));
    if (is_ident(scanner_peek(s))) {
        val->string.start = s->s + s->pos;
        val->string.len = 0;
        while (is_ident(scanner_peek(s))) {
            scanner_next(s);
            val->string.len++;
        }
        return TOK_IDENTIFIER;
    }

    scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                "unrecognized token");
    return TOK_ERROR;
}

/***====================================================================***/

enum rules_mlvo {
    MLVO_MODEL,
    MLVO_LAYOUT,
    MLVO_VARIANT,
    MLVO_OPTION,
    _MLVO_NUM_ENTRIES
};

enum {
    MLVO_LAYOUT_AND_VARIANT = ((1 << MLVO_LAYOUT) | (1 << MLVO_VARIANT))
};

typedef uint8_t mlvo_index_t;
typedef uint8_t mlvo_mask_t;


static const struct sval rules_mlvo_svals[_MLVO_NUM_ENTRIES] = {
    [MLVO_MODEL] = SVAL_INIT("model"),
    [MLVO_LAYOUT] = SVAL_INIT("layout"),
    [MLVO_VARIANT] = SVAL_INIT("variant"),
    [MLVO_OPTION] = SVAL_INIT("option"),
};

enum rules_kccgst {
    KCCGST_KEYCODES,
    KCCGST_TYPES,
    KCCGST_COMPAT,
    KCCGST_SYMBOLS,
    KCCGST_GEOMETRY,
    _KCCGST_NUM_ENTRIES
};

typedef uint8_t kccgst_index_t;
typedef uint8_t kccgst_mask_t;

static const struct sval rules_kccgst_svals[_KCCGST_NUM_ENTRIES] = {
    [KCCGST_KEYCODES] = SVAL_INIT("keycodes"),
    [KCCGST_TYPES] = SVAL_INIT("types"),
    [KCCGST_COMPAT] = SVAL_INIT("compat"),
    [KCCGST_SYMBOLS] = SVAL_INIT("symbols"),
    [KCCGST_GEOMETRY] = SVAL_INIT("geometry"),
};

enum {
    /** Value of the matched layout mask for layout-independent MLVO fields */
    GLOBAL_MATCHED_LAYOUTS = XKB_ALL_GROUPS,
};

static_assert(~(xkb_layout_mask_t)XKB_OPTION_LAYOUT_MASK_GLOBAL ==
              (xkb_layout_mask_t)GLOBAL_MATCHED_LAYOUTS,
              "Global layout mask consistency");

/* We use this to keep score whether an mlvo was matched or not; if not,
 * we warn the user that their preference was ignored. */
struct matched_sval {
    struct sval sval;
    /** Used for layout-specific options */
    xkb_layout_mask_t layouts;
    xkb_layout_mask_t matched;
};
typedef darray(struct matched_sval) darray_matched_sval;

/*
 * A broken-down version of xkb_rule_names (without the rules,
 * obviously).
 */
struct rule_names {
    struct matched_sval model;
    darray_matched_sval layouts;
    darray_matched_sval variants;
    darray_matched_sval options;
};

struct group {
    struct sval name;
    darray_sval elements;
};

struct mapping {
    enum rules_mlvo mlvo_at_pos[_MLVO_NUM_ENTRIES];
    mlvo_index_t num_mlvo;
    mlvo_mask_t defined_mlvo_mask;
    bool has_layout_idx_range;
    bool has_multiple_layouts;
    /* This member has 2 uses:
     * • Keep track of layout and variant indices while parsing MLVO headers.
     * • Store layout/variant range afterwards.
     * Thus provide 2 structs to reflect these semantics in the code. */
    union {
        struct { xkb_layout_index_t layout_idx, variant_idx; };
        struct { xkb_layout_index_t layout_idx_min, layout_idx_max; };
    };
    /* This member has 2 uses:
     * • Check if the mapping is active by interpreting the value as a boolean.
     * • Keep track of the remaining layout indices to match.
     * Thus provide 2 names to reflect these semantics in the code. */
    union {
        xkb_layout_mask_t active;
        xkb_layout_mask_t layouts_candidates_mask;
    };
    enum rules_kccgst kccgst_at_pos[_KCCGST_NUM_ENTRIES];
    kccgst_index_t num_kccgst;
    kccgst_mask_t defined_kccgst_mask;
};

enum mlvo_match_type {
    /** Match the given plain value */
    MLVO_MATCH_NORMAL = 0,
    /** Match depending on the value of `wildcard_match_type` */
    MLVO_MATCH_WILDCARD_LEGACY,
    /** Match empty value */
    MLVO_MATCH_WILDCARD_NONE,
    /** Match non-empty value */
    MLVO_MATCH_WILDCARD_SOME,
    /** Match any value, optionally empty */
    MLVO_MATCH_WILDCARD_ANY,
    /** Match any entry in a group */
    MLVO_MATCH_GROUP,
};

enum wildcard_match_type {
    /** ‘*’ matches only non-empty strings */
    WILDCARD_MATCH_NONEMPTY = 0,
    /** ‘*’ matches all strings */
    WILDCARD_MATCH_ALL,
};

struct rule {
    struct sval mlvo_value_at_pos[_MLVO_NUM_ENTRIES];
    enum mlvo_match_type match_type_at_pos[_MLVO_NUM_ENTRIES];
    struct sval kccgst_value_at_pos[_KCCGST_NUM_ENTRIES];
    mlvo_index_t num_mlvo_values;
    kccgst_index_t num_kccgst_values;
    bool skip;
};

#define SLICE_KCCGST_BIT_FIELD_SIZE 4
static_assert(!((1u << (SLICE_KCCGST_BIT_FIELD_SIZE - 1)) &
                (_KCCGST_NUM_ENTRIES - 1)),
              "Cannot encode KcCGST enum safely (need space for the sign)");
struct kccgst_buffer_slice {
    uint32_t length:(32 - SLICE_KCCGST_BIT_FIELD_SIZE);
    enum rules_kccgst kccgst:SLICE_KCCGST_BIT_FIELD_SIZE;
    xkb_layout_index_t layout;
};

/* Buffer for pending KcCGST values */
struct kccgst_buffer {
    darray_char buffer;
    /* Slice corresponding to each value in the buffer */
    darray(struct kccgst_buffer_slice) slices;
};

/*
 * This is the main object used to match a given RMLVO against a rules
 * file and aggregate the results in a KcCGST. It goes through a simple
 * matching state machine, with tokens as transitions (see
 * matcher_match()).
 */
struct matcher {
    struct xkb_context *ctx;
    /* Input.*/
    struct rule_names rmlvo;
    union lvalue val;
    darray(struct group) groups;
    /* Current mapping. */
    struct mapping mapping;
    /* Current rule. */
    struct rule rule;
    /*
     * Buffers for pending KcCGST values. Required in case of using layout
     * index ranges, to ensure that the values are merged in the expected order.
     * See the note: “Layout index ranges and merging KcCGST values”.
     */
    struct kccgst_buffer pending_kccgst;
    /* Output. */
    darray_char kccgst[_KCCGST_NUM_ENTRIES];
};

static struct sval
strip_spaces(struct sval v)
{
    while (v.len > 0 && is_space(v.start[0])) { v.len--; v.start++; }
    while (v.len > 0 && is_space(v.start[v.len - 1])) v.len--;
    return v;
}

static darray_matched_sval
split_comma_separated_mlvo(struct xkb_context *ctx,
                           enum rules_mlvo mlvo, const char *s)
{
    darray_matched_sval arr = darray_new();

    /*
     * Make sure the array returned by this function always includes at
     * least one value, e.g. "" -> { "" } and "," -> { "", "" }.
     */

    if (!s) {
        ensure_at_least_one_value:
        /* Label followed by a declaration is a C23 extension */ ;

        struct matched_sval val = {
            .sval = SVAL(NULL, 0),
            .layouts = (xkb_layout_mask_t)XKB_OPTION_LAYOUT_MASK_GLOBAL,
            .matched = 0
        };
        darray_append(arr, val);
        return arr;
    }

    while (true) {
        struct matched_sval val = {
            .sval = SVAL(s, 0),
            .layouts = (xkb_layout_mask_t)XKB_OPTION_LAYOUT_MASK_GLOBAL,
            .matched = 0,
        };
        while (*s != '\0' && *s != ',' && *s != OPTIONS_GROUP_SPECIFIER_PREFIX) {
            s++;
            val.sval.len++;
        }
        val.sval = strip_spaces(val.sval);

        if (*s == OPTIONS_GROUP_SPECIFIER_PREFIX) {
            /* Handle numeric layout index */
            s++;
            const char* const layout_start = s;
            xkb_layout_index_t layout = XKB_LAYOUT_INVALID;

            int count = parse_dec_to_uint32_t(s, SIZE_MAX, &layout);
            if (count > 0) {
                /* Note: 1-indexed layout */
                s += count;
                static_assert(XKB_MAX_GROUPS == 32, "Invalid shift");
                if (layout == 0 || layout > XKB_MAX_GROUPS) {
                    log_err(ctx, XKB_ERROR_UNSUPPORTED_LAYOUT_INDEX_,
                            "Invalid layout index %"PRIu32" "
                            "for the RMVLO component: \"%.*s\"\n", layout,
                            (unsigned int) val.sval.len, val.sval.start);
                } else if (mlvo != MLVO_OPTION) {
                    log_warn(ctx, XKB_LOG_MESSAGE_NO_ID,
                             "Layout index %"PRIu32" is not supported for "
                             "the RMLVO component: \"%.*s\"\n", layout,
                             (unsigned int) val.sval.len, val.sval.start);
                } else {
                    val.layouts = (UINT32_C(1) << (layout - 1));
                }
            }

            /* Consume until reaching next item or end of string */
            const char* const layout_index_end = s;
            while (*s != '\0' && *s != ',') { s++; }
            if (count <= 0 || layout_index_end != s) {
                log_err(ctx, XKB_ERROR_UNSUPPORTED_LAYOUT_INDEX_,
                        "Invalid layout index \"%.*s\" for the RMLVO "
                        "component \"%.*s\"; discarding specifier.\n",
                        (unsigned int) (s - layout_start), layout_start,
                        (unsigned int) val.sval.len, val.sval.start);
            }
        }

        if (val.sval.len || mlvo != MLVO_OPTION) {
            if (mlvo == MLVO_OPTION) {
                /* Check for previous entry */
                struct matched_sval *prev;
                darray_foreach(prev, arr) {
                    if (svaleq(prev->sval, val.sval)) {
                        /* Merge options */
                        if (val.layouts ==
                            (xkb_layout_mask_t)XKB_OPTION_LAYOUT_MASK_GLOBAL) {
                            /* Global option overrides layout-specific ones */
                            prev->layouts = val.layouts;
                        } else if (prev->layouts != (xkb_layout_mask_t)
                                   XKB_OPTION_LAYOUT_MASK_GLOBAL) {
                            /* Layout-specific option cannot override global */
                            prev->layouts |= val.layouts;
                        }
                        goto next_value;
                    }
                }
            }
            darray_append(arr, val);
        }

next_value:
        if (*s == '\0') break;
        if (*s == ',') s++;
    }

    if (darray_empty(arr))
        goto ensure_at_least_one_value;

    return arr;
}

static struct matcher *
matcher_new_from_rmlvo(const struct xkb_rmlvo_builder *rmlvo, const char **rules)
{
    struct matcher *m = calloc(1, sizeof(*m));
    if (!m)
        return NULL;

    m->ctx = rmlvo->ctx;

    /* Sanitize */
    struct xkb_rule_names names = {
        .rules = rmlvo->rules,
        .model = rmlvo->model,
        /* Initial values only used to detect missing entries */
        .layout = (darray_empty(rmlvo->layouts)) ? NULL : "x",
        .variant = (darray_empty(rmlvo->layouts)) ? NULL : "x",
        .options = (darray_empty(rmlvo->options)) ? NULL : "x",
    };
    const enum RMLVO changed = xkb_context_sanitize_rule_names(rmlvo->ctx,
                                                               &names);

    /* Initialize matcher with sanitized names, if relevant */
    if (changed & RMLVO_RULES) {
        *rules = names.rules;
    } else {
        *rules = rmlvo->rules;
    }

    if (changed & RMLVO_MODEL) {
        m->rmlvo.model.sval.start = names.model;
    } else {
        m->rmlvo.model.sval.start = rmlvo->model;
    }
    m->rmlvo.model.sval.len = strlen_safe(rmlvo->model);
    m->rmlvo.model.layouts = (xkb_layout_mask_t)XKB_OPTION_LAYOUT_MASK_GLOBAL;

    assert((changed & RMLVO_LAYOUT) || !(changed & RMLVO_VARIANT));
    if (changed & RMLVO_LAYOUT) {
        /* Layout and variant are tied together */
        m->rmlvo.layouts = split_comma_separated_mlvo(rmlvo->ctx, MLVO_LAYOUT,
                                                      names.layout);
        m->rmlvo.variants = split_comma_separated_mlvo(rmlvo->ctx, MLVO_VARIANT,
                                                       names.variant);
        if (darray_size(m->rmlvo.layouts) > darray_size(m->rmlvo.variants)) {
            /* Do not warn if no variants was provided */
            if (!isempty(names.variant))
                log_warn(m->ctx, XKB_LOG_MESSAGE_NO_ID,
                         "More layouts than variants: \"%s\" vs. \"%s\".\n",
                         names.layout ? names.layout : "(none)",
                         names.variant ? names.variant : "(none)");
            darray_resize0(m->rmlvo.variants, darray_size(m->rmlvo.layouts));
        } else if (darray_size(m->rmlvo.layouts) < darray_size(m->rmlvo.variants)) {
            log_err(m->ctx, XKB_LOG_MESSAGE_NO_ID,
                    "Less layouts than variants: \"%s\" vs. \"%s\".\n",
                    names.layout ? names.layout : "(none)",
                    names.variant ? names.variant : "(none)");
            darray_resize(m->rmlvo.variants, darray_size(m->rmlvo.layouts));
            darray_shrink(m->rmlvo.variants);
        }
    } else {
        struct xkb_rmlvo_builder_layout *layout;
        darray_foreach(layout, rmlvo->layouts) {
            struct matched_sval val = {
                .sval = SVAL(layout->layout, strlen_safe(layout->layout)),
                .layouts = (xkb_layout_mask_t)XKB_OPTION_LAYOUT_MASK_GLOBAL,
                .matched = 0
            };
            darray_append(m->rmlvo.layouts, val);
            val.sval.start = layout->variant;
            val.sval.len = strlen_safe(layout->variant);
            darray_append(m->rmlvo.variants, val);
        }
    }

    if (changed & RMLVO_OPTIONS) {
        m->rmlvo.options = split_comma_separated_mlvo(rmlvo->ctx, MLVO_OPTION,
                                                      names.options);
    } else {
        struct xkb_rmlvo_builder_option *option;
        darray_foreach(option, rmlvo->options) {
            struct matched_sval val = {
                .sval = SVAL(option->option, strlen_safe(option->option)),
                .layouts = option->layouts,
                .matched = 0
            };
            darray_append(m->rmlvo.options, val);
        }
    }

    return m;
}

static struct matcher *
matcher_new_from_names(struct xkb_context *ctx,
            const struct xkb_rule_names *rmlvo)
{
    struct matcher *m = calloc(1, sizeof(*m));
    if (!m)
        return NULL;

    m->ctx = ctx;
    m->rmlvo.model.sval.start = rmlvo->model;
    m->rmlvo.model.sval.len = strlen_safe(rmlvo->model);
    m->rmlvo.model.layouts = (xkb_layout_mask_t)XKB_OPTION_LAYOUT_MASK_GLOBAL;
    m->rmlvo.layouts = split_comma_separated_mlvo(ctx, MLVO_LAYOUT, rmlvo->layout);
    m->rmlvo.variants = split_comma_separated_mlvo(ctx, MLVO_VARIANT, rmlvo->variant);
    m->rmlvo.options = split_comma_separated_mlvo(ctx, MLVO_OPTION, rmlvo->options);

    if (darray_size(m->rmlvo.layouts) > darray_size(m->rmlvo.variants)) {
        /* Do not warn if no variants was provided */
        if (!isempty(rmlvo->variant))
            log_warn(ctx, XKB_LOG_MESSAGE_NO_ID,
                     "More layouts than variants: \"%s\" vs. \"%s\".\n",
                     rmlvo->layout ? rmlvo->layout : "(none)",
                     rmlvo->variant ? rmlvo->variant : "(none)");
        darray_resize0(m->rmlvo.variants, darray_size(m->rmlvo.layouts));
    } else if (darray_size(m->rmlvo.layouts) < darray_size(m->rmlvo.variants)) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Less layouts than variants: \"%s\" vs. \"%s\".\n",
                rmlvo->layout ? rmlvo->layout : "(none)",
                rmlvo->variant ? rmlvo->variant : "(none)");
        darray_resize(m->rmlvo.variants, darray_size(m->rmlvo.layouts));
        darray_shrink(m->rmlvo.variants);
    }

    return m;
}

static void
matcher_free(struct matcher *m)
{
    if (!m)
        return;
    darray_free(m->rmlvo.layouts);
    darray_free(m->rmlvo.variants);
    darray_free(m->rmlvo.options);
    struct group *group;
    darray_foreach(group, m->groups)
        darray_free(group->elements);
    darray_free(m->pending_kccgst.buffer);
    darray_free(m->pending_kccgst.slices);
    for (kccgst_index_t i = 0; i < (kccgst_index_t) _KCCGST_NUM_ENTRIES; i++)
        darray_free(m->kccgst[i]);
    darray_free(m->groups);
    free(m);
}

static void
matcher_group_start_new(struct matcher *m, struct sval name)
{
    struct group group = { .name = name, .elements = darray_new() };
    darray_append(m->groups, group);
}

static void
matcher_group_add_element(struct matcher *m, struct scanner *s,
                          struct sval element)
{
    darray_append(darray_item(m->groups, darray_size(m->groups) - 1).elements,
                  element);
}

static bool
read_rules_file(struct xkb_context *ctx,
                struct matcher *matcher,
                unsigned int include_depth,
                FILE *file,
                const char *path);

static void
matcher_include(struct matcher *m, struct scanner *parent_scanner,
                unsigned int include_depth,
                struct sval inc)
{
    if (include_depth >= MAX_INCLUDE_DEPTH) {
        scanner_err(parent_scanner, XKB_LOG_MESSAGE_NO_ID,
                    "maximum include depth (%u) exceeded; "
                    "maybe there is an include loop?",
                    MAX_INCLUDE_DEPTH);
        return;
    }

    const char *stmt_file = inc.start;
    size_t stmt_file_len = inc.len;

    /* Process %-expansion, if any */
    char buf[PATH_MAX] = {0};
    const ssize_t expanded = expand_path(m->ctx, parent_scanner->file_name,
                                         stmt_file, stmt_file_len,
                                         FILE_TYPE_RULES,
                                         buf, sizeof(buf));
    if (expanded < 0) {
        /* Error */
        return;
    } else if (expanded > 0) {
        /* %-expanded */
        stmt_file = buf;
        stmt_file_len = (size_t) expanded;
        assert(stmt_file[stmt_file_len] == '\0');
    }

    /* Lookup the first candidate */
    FILE* file;
    unsigned int offset = 0;
    const bool absolute_path = is_absolute_path(stmt_file);
    if (absolute_path) {
        /* Absolute path: no need for lookup in XKB paths */
        if (!expanded) {
            /* No %expansion: ensure it’s NULL-terminated */
            if (stmt_file_len < sizeof(buf)) {
                memcpy(buf, stmt_file, stmt_file_len);
                buf[stmt_file_len] = '\0';
                stmt_file = buf;
            } else {
                log_err(m->ctx, XKB_ERROR_INVALID_PATH,
                        "Path is too long: %zu > %zu, got raw path: %.*s\n",
                        stmt_file_len, sizeof(buf),
                        (unsigned int) stmt_file_len, stmt_file);
                return;
            }
        } else {
            /* %-expansion is always NULL-terminated */
            assert(stmt_file[stmt_file_len] == '\0');
        }
        file = fopen(stmt_file, "rb");
    } else {
        /* Relative path: lookup the first XKB path */
        if (unlikely(expanded)) {
            /*
             * Relative path after expansion
             *
             * Unlikely to happen, because %-expansion is meant to use absolute
             * paths. Considering that:
             * - we do not resolve paths before expansion, leading to unexpected
             *   result here;
             * - we need the buffer afterwards, but it currently contains the
             *   expanded path;
             * - this is an edge case;
             * we simply make the lookup fail.
             */
            file = NULL;
        } else {
            file = FindFileInXkbPath(m->ctx, parent_scanner->file_name,
                                     stmt_file, stmt_file_len, FILE_TYPE_RULES,
                                     buf, sizeof(buf), &offset, true);
        }
    }

    while (file) {
        assert(strlen_safe(buf) < sizeof(buf));
        bool ret = read_rules_file(m->ctx, m, include_depth + 1, file, buf);
        fclose(file);
        if (ret)
            return;
        /* Failed to parse rules or get all the components */
        log_err(m->ctx, XKB_LOG_MESSAGE_NO_ID,
                "No components returned from included XKB rules \"%s\"\n",
                buf);

        if (absolute_path) {
            /* There is no point to search further if the path is absolute */
            break;
        }

        /* Try next XKB path */
        offset++;
        file = FindFileInXkbPath(m->ctx, parent_scanner->file_name,
                                 stmt_file, stmt_file_len, FILE_TYPE_RULES,
                                 buf, sizeof(buf), &offset, true);
    }

    log_err(m->ctx, XKB_LOG_MESSAGE_NO_ID,
            "Failed to open included XKB rules \"%.*s\"\n",
            (unsigned int) stmt_file_len, stmt_file);
}

static void
matcher_mapping_start_new(struct matcher *m)
{
    for (mlvo_index_t i = 0; i < (mlvo_index_t) _MLVO_NUM_ENTRIES; i++)
        m->mapping.mlvo_at_pos[i] = _MLVO_NUM_ENTRIES;
    for (kccgst_index_t i = 0; i < (kccgst_index_t) _KCCGST_NUM_ENTRIES; i++)
        m->mapping.kccgst_at_pos[i] = _KCCGST_NUM_ENTRIES;
    m->mapping.has_multiple_layouts = false;
    m->mapping.layout_idx = m->mapping.variant_idx = XKB_LAYOUT_INVALID;
    m->mapping.num_mlvo = m->mapping.num_kccgst = 0;
    m->mapping.defined_mlvo_mask = 0;
    m->mapping.defined_kccgst_mask = 0;
    m->mapping.active = true;
}

/** Caller must check prefix `[` and initialize `out` to XKB_LAYOUT_INVALID */
static int
parse_layout_int_index_tail(const char *s, size_t max_len,
                            xkb_layout_index_t *out)
{
    if (max_len < 3)
        return -1;
    uint32_t val = 0;
    const int count = parse_dec_to_uint32_t(&s[1], max_len - 2, &val);
    if (count <= 0 || s[1 + count] != ']' || val == 0 || val > XKB_MAX_GROUPS)
        return -1;
    /* To zero-based index. */
    *out = val - 1;
    return count + 2; /* == length "[index]" */
}

/* Parse Kccgst layout index:
 * "[%i]" or "[n]", where "n" is a decimal number */
static int
extract_layout_index(const char *s, size_t max_len, xkb_layout_index_t *out)
{
    /* This function is pretty stupid, but works for now. */
    *out = XKB_LAYOUT_INVALID;
    if (max_len < 3 || s[0] != '[')
        return -1;
    if (s[2] == ']' && s[1] >= '1' && s[1] <= '9') {
        /* Small numeric index */
        *out = (xkb_layout_index_t)(s[1] - '1');
        return 3;
    }
    if (max_len > 3 && s[1] == '%' && s[2] == 'i' && s[3] == ']') {
        /* Special index: %i */
        return 4; /* == length "[%i]" */
    }
    /* Fallback: any numeric index (unlikely) */
    return parse_layout_int_index_tail(s, max_len, out);
}

/* Special layout indices */
enum layout_index_ranges {
    LAYOUT_INDEX_SINGLE = XKB_LAYOUT_INVALID - 5,
    LAYOUT_INDEX_FIRST,
    LAYOUT_INDEX_LATER,
    LAYOUT_INDEX_MULTIPLE,
    LAYOUT_INDEX_ANY,
};

static_assert((xkb_layout_index_t) XKB_MAX_GROUPS <
              (xkb_layout_index_t) LAYOUT_INDEX_SINGLE,
              "Cannot define special indices");
static_assert((xkb_layout_index_t) LAYOUT_INDEX_SINGLE <
              (xkb_layout_index_t) LAYOUT_INDEX_FIRST &&
              (xkb_layout_index_t) LAYOUT_INDEX_FIRST <
              (xkb_layout_index_t) LAYOUT_INDEX_LATER &&
              (xkb_layout_index_t) LAYOUT_INDEX_LATER <
              (xkb_layout_index_t) LAYOUT_INDEX_MULTIPLE &&
              (xkb_layout_index_t) LAYOUT_INDEX_MULTIPLE <
              (xkb_layout_index_t) LAYOUT_INDEX_ANY &&
              (xkb_layout_index_t) LAYOUT_INDEX_ANY <
              (xkb_layout_index_t) XKB_LAYOUT_INVALID,
              "Special indices must respect certain order");

/* Parse index of layout/variant in MLVO mapping */
static bool
extract_mapping_layout_index(const char *s, size_t max_len,
                             xkb_layout_index_t *out)
{
    /* Check for minimal `[` + index + `]` */
    if (max_len < 3 || s[0] != '[') {
        *out = XKB_LAYOUT_INVALID;
        return false;
    }

    /* Small numeric index */
    if (max_len == 3 && s[2] == ']' && s[1] >= '1' && s[1] <= '9') {
        *out = (xkb_layout_index_t)(s[1] - '1');
        return true;
    }

    /* Named indices ranges */
    static const struct {
        const char* name;
        size_t size;
        xkb_layout_index_t value;
    } names[] = {
        #define INDEX_RANGE(tail, value) \
            { (tail), sizeof(tail), (xkb_layout_index_t)(value) }
        INDEX_RANGE("single]",   LAYOUT_INDEX_SINGLE),
        INDEX_RANGE("first]",    LAYOUT_INDEX_FIRST),
        INDEX_RANGE("later]",    LAYOUT_INDEX_LATER),
        INDEX_RANGE("multiple]", LAYOUT_INDEX_MULTIPLE),
        INDEX_RANGE("any]",      LAYOUT_INDEX_ANY),
        #undef INDEX_RANGE
    };

    for (size_t k = 0; k < ARRAY_SIZE(names); k++) {
        if (max_len == names[k].size &&
            memcmp(&s[1], names[k].name, names[k].size - 1) == 0) {
            *out = names[k].value;
            return true;
        }
    }

    /* Fallback: any numeric index (unlikely) */
    *out = XKB_LAYOUT_INVALID;
    const int count = parse_layout_int_index_tail(s, max_len, out);
    return count > 0 && (size_t)count == max_len;
}

static inline bool
is_mlvo_mask_defined(struct matcher *m, enum rules_mlvo mlvo)
{
    return m->mapping.defined_mlvo_mask & (1u << mlvo);
}

static void
matcher_mapping_set_mlvo(struct matcher *m, struct scanner *s,
                         struct sval ident)
{
    enum rules_mlvo mlvo;
    struct sval mlvo_sval;

    for (mlvo = 0; mlvo < _MLVO_NUM_ENTRIES; mlvo++) {
        mlvo_sval = rules_mlvo_svals[mlvo];

        if (svaleq_prefix(mlvo_sval, ident))
            break;
    }

    /* Not found. */
    if (mlvo >= _MLVO_NUM_ENTRIES) {
        scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                    "invalid mapping: \"%.*s\" is not a valid value here; "
                    "ignoring rule set",
                    (unsigned int) ident.len, ident.start);
        m->mapping.active = false;
        return;
    }

    if (is_mlvo_mask_defined(m, mlvo)) {
        scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                    "invalid mapping: \"%.*s\" appears twice on the same line; "
                    "ignoring rule set",
                    (unsigned int) mlvo_sval.len, mlvo_sval.start);
        m->mapping.active = false;
        return;
    }

    /* If there are leftovers still, it must be an index. */
    if (mlvo_sval.len < ident.len) {
        xkb_layout_index_t idx;
        if (!extract_mapping_layout_index(ident.start + mlvo_sval.len,
                                          ident.len - mlvo_sval.len, &idx)) {
            scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                        "invalid mapping: \"%.*s\" may only be followed by a "
                        "valid group index; ignoring rule set",
                        (unsigned int) mlvo_sval.len, mlvo_sval.start);
            m->mapping.active = false;
            return;
        }

        if (mlvo == MLVO_LAYOUT) {
            m->mapping.layout_idx = idx;
        }
        else if (mlvo == MLVO_VARIANT) {
            m->mapping.variant_idx = idx;
        }
        else {
            scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                        "invalid mapping: \"%.*s\" cannot be followed by a group "
                        "index; ignoring rule set",
                        (unsigned int) mlvo_sval.len, mlvo_sval.start);
            m->mapping.active = false;
            return;
        }
    } else if (mlvo == MLVO_LAYOUT) {
        m->mapping.layout_idx = (xkb_layout_index_t) LAYOUT_INDEX_SINGLE;
    } else if (mlvo == MLVO_VARIANT) {
        m->mapping.variant_idx = (xkb_layout_index_t) LAYOUT_INDEX_SINGLE;
    }

    /* Check that if both layout and variant are defined, then they must have
     * the same index */
    if (((mlvo == MLVO_LAYOUT && is_mlvo_mask_defined(m, MLVO_VARIANT)) ||
         (mlvo == MLVO_VARIANT && is_mlvo_mask_defined(m, MLVO_LAYOUT))) &&
        m->mapping.layout_idx != m->mapping.variant_idx) {
        scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                    "invalid mapping: \"layout\" index must be the same as the "
                    "\"variant\" index");
        m->mapping.active = false;
        return;
    }

    m->mapping.mlvo_at_pos[m->mapping.num_mlvo] = mlvo;
    m->mapping.defined_mlvo_mask |= (mlvo_mask_t) 1u << mlvo;
    m->mapping.num_mlvo++;
}

static void
matcher_mapping_set_layout_bounds(struct matcher *m)
{
    /* Handle case where one of the index is XKB_LAYOUT_INVALID */
    xkb_layout_index_t idx = MIN(m->mapping.layout_idx, m->mapping.variant_idx);
    switch (idx) {
        case XKB_LAYOUT_INVALID:
            /* No layout nor variant */
            assert(!is_mlvo_mask_defined(m, MLVO_LAYOUT) &&
                   !is_mlvo_mask_defined(m, MLVO_VARIANT));
            m->mapping.has_layout_idx_range = false;
            m->mapping.layout_idx_min = XKB_LAYOUT_INVALID;
            m->mapping.layout_idx_max = m->mapping.layout_idx_min;
            m->mapping.layouts_candidates_mask = 0x1; /* active = true */
            break;
        case LAYOUT_INDEX_LATER:
            m->mapping.has_layout_idx_range = true;
            m->mapping.layout_idx_min = 1;
            m->mapping.layout_idx_max = MIN(XKB_MAX_GROUPS,
                                            darray_size(m->rmlvo.layouts));
            m->mapping.layouts_candidates_mask = (xkb_layout_mask_t)
                /* All but the first layout */
                ((UINT64_C(1) << m->mapping.layout_idx_max) - UINT64_C(1)) &
                ~UINT64_C(1);
            break;
        case LAYOUT_INDEX_MULTIPLE:
        case LAYOUT_INDEX_ANY:
            m->mapping.has_layout_idx_range = true;
            m->mapping.layout_idx_min = 0;
            m->mapping.layout_idx_max = MIN(XKB_MAX_GROUPS,
                                            darray_size(m->rmlvo.layouts));
            m->mapping.layouts_candidates_mask = (xkb_layout_mask_t)
                /* All layouts */
                (UINT64_C(1) << m->mapping.layout_idx_max) - UINT64_C(1);
            break;
        case LAYOUT_INDEX_SINGLE:
        case LAYOUT_INDEX_FIRST:
            /* No index or first index */
            idx = 0;
            /* fallthrough */
        default:
            /* Mere layout index */
            m->mapping.has_layout_idx_range = false;
            m->mapping.layout_idx_min = idx;
            m->mapping.layout_idx_max = idx + 1;
            m->mapping.layouts_candidates_mask = UINT32_C(1) << idx;
    }
    m->mapping.has_multiple_layouts = (
        m->mapping.layout_idx_max > m->mapping.layout_idx_min &&
        m->mapping.layout_idx_max - m->mapping.layout_idx_min > 1
    );
}

static void
matcher_mapping_set_kccgst(struct matcher *m, struct scanner *s, struct sval ident)
{
    enum rules_kccgst kccgst;
    struct sval kccgst_sval;

    for (kccgst = 0; kccgst < _KCCGST_NUM_ENTRIES; kccgst++) {
        kccgst_sval = rules_kccgst_svals[kccgst];

        if (svaleq(rules_kccgst_svals[kccgst], ident))
            break;
    }

    /* Not found. */
    if (kccgst >= _KCCGST_NUM_ENTRIES) {
        scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                    "invalid mapping: \"%.*s\" is not a valid value here; "
                    "ignoring rule set",
                    (unsigned int) ident.len, ident.start);
        m->mapping.active = false;
        return;
    }

    if (m->mapping.defined_kccgst_mask & (1u << kccgst)) {
        scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                    "invalid mapping: \"%.*s\" appears twice on the same line; "
                    "ignoring rule set",
                    (unsigned int) kccgst_sval.len, kccgst_sval.start);
        m->mapping.active = false;
        return;
    }

    m->mapping.kccgst_at_pos[m->mapping.num_kccgst] = kccgst;
    m->mapping.defined_kccgst_mask |= (kccgst_mask_t) 1u << kccgst;
    m->mapping.num_kccgst++;
}

static bool
matcher_mapping_verify(struct matcher *m, struct scanner *s)
{
    if (m->mapping.num_mlvo == 0) {
        scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                    "invalid mapping: must have at least one value on the left "
                    "hand side; ignoring rule set");
        goto skip;
    }

    if (m->mapping.num_kccgst == 0) {
        scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                    "invalid mapping: must have at least one value on the right "
                    "hand side; ignoring rule set");
        goto skip;
    }

    /*
     * This following is very stupid, but this is how it works.
     * See the "Notes" section in the overview above.
     */

    if (is_mlvo_mask_defined(m, MLVO_LAYOUT)) {
        assert(m->mapping.layout_idx != XKB_LAYOUT_INVALID);
        switch (m->mapping.layout_idx) {
            case LAYOUT_INDEX_SINGLE:
                /* Layout rule without index matches when
                 * exactly 1 layout is specified */
                if (darray_size(m->rmlvo.layouts) > 1)
                    goto skip;
                break;
            case LAYOUT_INDEX_MULTIPLE:
            case LAYOUT_INDEX_LATER:
                /* Layout rule matches when at least 2 layouts are specified */
                if (darray_size(m->rmlvo.layouts) < 2)
                    goto skip;
                break;
            case LAYOUT_INDEX_ANY:
            case LAYOUT_INDEX_FIRST:
                /* No restrictions */
                break;
            default:
                /* Layout rule with index matches when at least 2 layouts are
                 * specified. Index must be in valid range. */
                if (darray_size(m->rmlvo.layouts) < 2 ||
                    m->mapping.layout_idx >= darray_size(m->rmlvo.layouts))
                    goto skip;
        }
    }

    if (is_mlvo_mask_defined(m, MLVO_VARIANT)) {
        assert(m->mapping.variant_idx != XKB_LAYOUT_INVALID);
        switch (m->mapping.variant_idx) {
            case LAYOUT_INDEX_SINGLE:
                /* Variant rule without index matches
                 * when exactly 1 variant is specified */
                if (darray_size(m->rmlvo.variants) > 1)
                    goto skip;
                break;
            case LAYOUT_INDEX_ANY:
            case LAYOUT_INDEX_LATER:
            case LAYOUT_INDEX_FIRST:
                /* No restriction */
                break;
            default:
                /* Variant rule with index matches when at least 2 variants are
                 * specified. Index must be in valid range. */
                if (darray_size(m->rmlvo.variants) < 2 ||
                    m->mapping.variant_idx >= darray_size(m->rmlvo.variants))
                    goto skip;
        }
    }

    return true;

skip:
    m->mapping.active = false;
    return false;
}

static void
matcher_rule_start_new(struct matcher *m)
{
    memset(&m->rule, 0, sizeof(m->rule));
    m->rule.skip = !m->mapping.active;
}

static void
matcher_rule_set_mlvo_common(struct matcher *m, struct scanner *s,
                             struct sval ident,
                             enum mlvo_match_type match_type)
{
    if (m->rule.num_mlvo_values >= m->mapping.num_mlvo) {
        scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                    "invalid rule: has more values than the mapping line; "
                    "ignoring rule");
        m->rule.skip = true;
        return;
    }
    m->rule.match_type_at_pos[m->rule.num_mlvo_values] = match_type;
    m->rule.mlvo_value_at_pos[m->rule.num_mlvo_values] = ident;
    m->rule.num_mlvo_values++;
}

static void
matcher_rule_set_mlvo_wildcard(struct matcher *m, struct scanner *s,
                               enum mlvo_match_type match_type)
{
    struct sval dummy = SVAL(NULL, 0);
    matcher_rule_set_mlvo_common(m, s, dummy, match_type);
}

static void
matcher_rule_set_mlvo_group(struct matcher *m, struct scanner *s,
                            struct sval ident)
{
    matcher_rule_set_mlvo_common(m, s, ident, MLVO_MATCH_GROUP);
}

static void
matcher_rule_set_mlvo(struct matcher *m, struct scanner *s,
                      struct sval ident)
{
    matcher_rule_set_mlvo_common(m, s, ident, MLVO_MATCH_NORMAL);
}

static void
matcher_rule_set_kccgst(struct matcher *m, struct scanner *s,
                        struct sval ident)
{
    if (m->rule.num_kccgst_values >= m->mapping.num_kccgst) {
        scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                    "invalid rule: has more values than the mapping line; "
                    "ignoring rule");
        m->rule.skip = true;
        return;
    }
    m->rule.kccgst_value_at_pos[m->rule.num_kccgst_values] = ident;
    m->rule.num_kccgst_values++;
}

static bool
match_group(struct matcher *m, struct sval group_name, struct sval to)
{
    struct group *group = NULL;
    struct sval *element;
    bool found = false;

    darray_foreach(group, m->groups) {
        if (svaleq(group->name, group_name)) {
            found = true;
            break;
        }
    }

    if (!found) {
        /*
         * rules/evdev intentionally uses some undeclared group names
         * in rules (e.g. commented group definitions which may be
         * uncommented if needed). So we continue silently.
         */
        return false;
    }

    darray_foreach(element, group->elements)
        if (svaleq(to, *element))
            return true;

    return false;
}

static bool
match_value(struct matcher *m, const struct sval val, const struct sval to,
            enum mlvo_match_type match_type,
            enum wildcard_match_type wildcard_type)
{
    switch (match_type) {
        case MLVO_MATCH_WILDCARD_LEGACY:
            /* Match empty values only if explicitly required */
            return wildcard_type == WILDCARD_MATCH_ALL || !!to.len;
        case MLVO_MATCH_WILDCARD_NONE:
            return !to.len;
        case MLVO_MATCH_WILDCARD_SOME:
            return !!to.len;
        case MLVO_MATCH_WILDCARD_ANY:
            /* Contrary to the legacy ‘*’, this wild card *always* matches */
            return true;
        case MLVO_MATCH_GROUP:
            return match_group(m, val, to);
        default:
            assert(match_type == MLVO_MATCH_NORMAL);
            return svaleq(val, to);
    }
}

static bool
match_value_and_mark(struct matcher *m, const struct sval val,
                     struct matched_sval *to, enum mlvo_match_type match_type,
                     enum wildcard_match_type wildcard_type,
                     xkb_layout_mask_t layouts)
{
    bool matched = match_value(m, val, to->sval, match_type, wildcard_type);
    if (matched)
        to->matched |= layouts;
    return matched;
}

/*
 * This function performs %-expansion on @value (see overview above),
 * and appends the result to @expanded.
 */
static bool
expand_rmlvo_in_kccgst_value(struct matcher *m, struct scanner *s,
                             struct sval value, xkb_layout_index_t layout_idx,
                             darray_char *expanded, size_t *i)
{
    const char *str = value.start;

    /*
     * Some ugly hand-lexing here, but going through the scanner is more
     * trouble than it's worth, and the format is ugly on its own merit.
     */
    enum rules_mlvo mlv;
    xkb_layout_index_t idx;
    char pfx, sfx;
    struct matched_sval *expanded_value;

    /* %i not as layout/variant index "%l[%i]" but as qualifier ":%i" */
    if (str[*i] == 'i' &&
        (*i + 1 == value.len || is_merge_mode_prefix(str[*i + 1])))
    {
        if (layout_idx == XKB_LAYOUT_INVALID) {
            scanner_err(
                s, XKB_ERROR_RULES_INVALID_LAYOUT_INDEX_PERCENT_EXPANSION,
                "Invalid %%i in %%-expansion: there is no corresponding "
                "layout nor variant in the MLVO fields of the rules header."
            );
            goto error;
        }
        (*i)++;
        char index_str[MAX_LAYOUT_INDEX_STR_LENGTH + 1];
        const int count = snprintf(index_str, sizeof(index_str), "%"PRIu32,
                                   layout_idx + 1);
        assert(count > 0); /* See MAX_LAYOUT_INDEX_STR_LENGTH */
        darray_appends_nullterminate(*expanded, index_str, (darray_size_t)count);
        return true;
    }

    pfx = sfx = 0;

    /* Check for prefix. */
    if (str[*i] == '(' ||
        is_merge_mode_prefix(str[*i]) ||
        str[*i] == '_' || str[*i] == '-') {
        pfx = str[*i];
        if (str[*i] == '(') sfx = ')';
        if (++(*i) >= value.len) goto error;
    }

    /* Mandatory model/layout/variant specifier. */
    switch (str[(*i)++]) {
    case 'm': mlv = MLVO_MODEL; break;
    case 'l': mlv = MLVO_LAYOUT; break;
    case 'v': mlv = MLVO_VARIANT; break;
    default: goto error;
    }

    /* Check for index. */
    idx = XKB_LAYOUT_INVALID;
    bool variable_index = false;
    if (*i < value.len && str[*i] == '[') {
        if (mlv != MLVO_LAYOUT && mlv != MLVO_VARIANT) {
            scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                        "invalid index in %%-expansion; "
                        "may only index layout or variant");
            goto error;
        }

        int consumed = extract_layout_index(str + (*i), value.len - (*i), &idx);
        if (consumed <= 0) goto error;
        if (idx == XKB_LAYOUT_INVALID) {
            /* %i encountered */
            assert(layout_idx != XKB_LAYOUT_INVALID);
            idx = layout_idx;
            variable_index = true;
        }
        *i += (size_t)consumed;
    }

    /* Check for suffix, if there supposed to be one. */
    if (sfx != 0) {
        if (*i >= value.len) goto error;
        if (str[(*i)++] != sfx) goto error;
    }

    /* Get the expanded value. */
    expanded_value = NULL;

    if (mlv == MLVO_LAYOUT) {
        if (idx == XKB_LAYOUT_INVALID) {
            /* No index provided: match only if single layout */
            if (darray_size(m->rmlvo.layouts) == 1)
                expanded_value = &darray_item(m->rmlvo.layouts, 0);
        /* Some index provided: expand only if it is %i or
         * if there are multiple layouts */
        } else if (idx < darray_size(m->rmlvo.layouts) &&
                   (variable_index || darray_size(m->rmlvo.layouts) > 1)) {
                expanded_value = &darray_item(m->rmlvo.layouts, idx);
        }
    }
    else if (mlv == MLVO_VARIANT) {
        if (idx == XKB_LAYOUT_INVALID) {
            /* No index provided: match only if single variant */
            if (darray_size(m->rmlvo.variants) == 1)
                expanded_value = &darray_item(m->rmlvo.variants, 0);
        /* Some index provided: expand only if it is %i or
         * if there are multiple variants */
        } else if (idx < darray_size(m->rmlvo.variants) &&
                   (variable_index || darray_size(m->rmlvo.variants) > 1)) {
                expanded_value = &darray_item(m->rmlvo.variants, idx);
        }
    }
    else if (mlv == MLVO_MODEL) {
        expanded_value = &m->rmlvo.model;
    }

    /* If we didn't get one, skip silently. */
    if (!expanded_value || expanded_value->sval.len == 0) {
        return true;
    }

    if (pfx != 0)
        darray_appends_nullterminate(*expanded, &pfx, 1);
    darray_appends_nullterminate(*expanded,
                                 expanded_value->sval.start,
                                 (darray_size_t) expanded_value->sval.len);
    if (sfx != 0)
        darray_appends_nullterminate(*expanded, &sfx, 1);

    if (mlv != MLVO_LAYOUT && mlv != MLVO_VARIANT) {
        static_assert(XKB_MAX_GROUPS == 32 &&
                      XKB_MAX_GROUPS < XKB_LAYOUT_INVALID,
                      "Invalid shift");
        const xkb_layout_mask_t layouts = (idx == XKB_LAYOUT_INVALID)
            ? 0x1 /* default to first layout */
            : (UINT32_C(1) << idx);
        expanded_value->matched |= layouts;
    } else {
        expanded_value->matched |= (xkb_layout_mask_t)GLOBAL_MATCHED_LAYOUTS;
    }

    return true;

error:
    scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                "invalid %%-expansion in value; not used");
    return false;
}

/*
 * This function performs :all replacement on @value (see overview above),
 * and appends the result to @expanded.
 */
static void
expand_qualifier_in_kccgst_value(
    struct matcher *m, struct scanner *s,
    struct sval value, darray_char *expanded,
    bool has_layout_idx_range, bool has_separator,
    darray_size_t prefix_idx, size_t *i)
{
    const char *str = value.start;

    /* “all” followed by nothing or by a layout separator */
    if (*i + 2 < value.len &&
        str[*i] == 'a' && str[*i + 1] == 'l' && str[*i + 2] == 'l' &&
        (*i + 3 == value.len || is_merge_mode_prefix(str[*i + 3]))) {
        if (has_layout_idx_range)
            scanner_vrb(s, XKB_LOG_VERBOSITY_DETAILED, XKB_LOG_MESSAGE_NO_ID,
                        "Using :all qualifier with indices range "
                        "is not recommended.");
        /* Add at least one layout */
        darray_appends_nullterminate(*expanded, "1", 1);
        /* Check for more layouts (slow path) */
        if (darray_size(m->rmlvo.layouts) > 1) {
            char layout_index[MAX_LAYOUT_INDEX_STR_LENGTH + 1];
            const darray_size_t prefix_length =
                darray_size(*expanded) - prefix_idx - 1;
            for (xkb_layout_index_t l = 1;
                 l < MIN(XKB_MAX_GROUPS, darray_size(m->rmlvo.layouts));
                 l++)
            {
                if (!has_separator)
                    darray_append(*expanded, MERGE_DEFAULT_PREFIX);
                /* Append prefix */
                darray_appends_nullterminate(*expanded,
                                             &darray_item(*expanded, prefix_idx),
                                             prefix_length);
                /* Append index */
                const int count = snprintf(layout_index, sizeof(layout_index),
                                           "%"PRIu32, l + 1);
                assert(count > 0); /* See MAX_LAYOUT_INDEX_STR_LENGTH */
                darray_appends_nullterminate(*expanded, layout_index,
                                             (darray_size_t)count);
            }
        }
        *i += 3;
    }
}

static inline void
#ifdef _MSC_VER
concat_kccgst(darray_char *into, darray_size_t size, _In_reads_(size) const char* from)
#else
concat_kccgst(darray_char *into, darray_size_t size, const char from[static size])
#endif
{
    /*
     * Appending  bar to  foo ->  foo (not an error if this happens)
     * Appending +bar to  foo ->  foo+bar
     * Appending  bar to +foo ->  bar+foo
     * Appending +bar to +foo -> +foo+bar
     */
    const bool from_plus = is_merge_mode_prefix(from[0]);
    if (from_plus || darray_empty(*into)) {
        darray_appends_nullterminate(*into, from, size);
    } else {
        const char ch =
            (char) (darray_empty(*into) ? '\0' : darray_item(*into, 0));
        const bool into_plus = is_merge_mode_prefix(ch);
        if (into_plus)
            darray_prepends_nullterminate(*into, from, size);
    }
}

/*
 * This function performs %-expansion and :all-expansion on @value
 * (see overview above), and appends the result to @to.
 */
static bool
append_expanded_kccgst_value(struct matcher *m, struct scanner *s,
                             bool merge, darray_char *to, struct sval value,
                             xkb_layout_index_t layout_idx)
{
    const char *str = value.start;
    darray_char expanded = darray_new();
    darray_size_t last_item_idx = 0;
    bool has_separator = false;

    for (size_t i = 0; i < value.len; ) {
        /* Check if that's a start of an expansion or qualifier */
        switch (str[i]) {
            /* Qualifier */
            case ':':
                darray_appends_nullterminate(expanded, &str[i++], 1);
                expand_qualifier_in_kccgst_value(m, s, value, &expanded,
                                                 m->mapping.has_layout_idx_range,
                                                 has_separator,
                                                 last_item_idx, &i);
                break;
            /* Expansion */
            case '%':
                i++;
                if (i >= value.len ||
                    !expand_rmlvo_in_kccgst_value(m, s, value, layout_idx,
                                                  &expanded, &i))
                        goto error;
                break;
            /* New item */
            case MERGE_OVERRIDE_PREFIX:
            case MERGE_AUGMENT_PREFIX:
            case MERGE_REPLACE_PREFIX:
                darray_appends_nullterminate(expanded, &str[i++], 1);
                last_item_idx = darray_size(expanded) - 1;
                has_separator = true;
                break;
            /* Just a normal character. */
            default:
                darray_appends_nullterminate(expanded, &str[i++], 1);
        }
    }

    /* See note: “Layout index ranges and merging KcCGST values” */
    if (merge) {
        if (!darray_empty(expanded))
            concat_kccgst(to, darray_size(expanded), darray_items(expanded));
    } else {
        darray_concat(*to, expanded);
    }
    darray_free(expanded);
    return true;

error:
    darray_free(expanded);
    return false;
}

static bool
matcher_append_pending_kccgst(struct matcher *m)
{
    if (!m->mapping.has_multiple_layouts)
        return true;
    /*
     * Handle pending KcCGST values
     * See note: “Layout index ranges and merging KcCGST values”
     */
    for (kccgst_index_t i = 0; i < m->mapping.num_kccgst; i++) {
        const enum rules_kccgst kccgst = m->mapping.kccgst_at_pos[i];
        /* For each relevant layout, append the relevant KcCGST values to
         * the output. */
        for (xkb_layout_index_t layout = m->mapping.layout_idx_min;
             layout < m->mapping.layout_idx_max;
             layout++) {
            /* There may be multiple values to add if the rule set involved
             * options. Process them sequentially. */
            register const struct kccgst_buffer* const buf = &m->pending_kccgst;
            size_t offset = 0;
            for (darray_size_t k = 0; k < darray_size(buf->slices); k++) {
                register const struct kccgst_buffer_slice * const slice =
                    &darray_item(buf->slices, k);
                if (slice->kccgst == kccgst && slice->layout == layout &&
                    slice->length)
                    concat_kccgst(&m->kccgst[kccgst], slice->length,
                                  darray_items(buf->buffer) + offset);
                offset += slice->length;
            }
        }
    }
    /* Ensure we won’t come here before the next relevant rule set */
    m->mapping.has_multiple_layouts = false;
    return true;
}

static void
matcher_rule_verify(struct matcher *m, struct scanner *s)
{
    if (m->rule.num_mlvo_values != m->mapping.num_mlvo ||
        m->rule.num_kccgst_values != m->mapping.num_kccgst) {
        scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                    "invalid rule: must have same number of values "
                    "as mapping line; ignoring rule");
        m->rule.skip = true;
    }
}

/*
 * NOTE: The wildcard `*` behaves differently depending on the MLVO field:
 * - model and options: matches any value, including empty;
 * - layout and variant: matches any *non-empty* value.
 *
 * This aligns with the implementation in libxkbfile and xserver, with
 * the exception of options, where `*` is entirely ignored.
 *
 * The underlying rationale for this discrepancy across MLVO fields is
 * undocumented.
 *
 * In xkbcommon, `*` behaves identically for both model and options to
 * maintain consistency and simplicity. This divergence is unlikely
 * to have a practical impact: to the best of our knowledge, `*` has
 * no real-world use for the options field.
 *
 * | MLVO field | Wildcard `*` matches        |
 * | ---------- | --------------------------- |
 * | Model      | Any value (including empty) |
 * | Option     | Any value (including empty) |
 * | Layout     | Non-empty values only       |
 * | Variant    | Non-empty values only       |
 *
 */

/** Match the `model` MLVO field
 *
 * `model` input is a single value.
 */
static bool
matcher_rule_match_model(struct matcher *m, const struct sval value,
                         enum mlvo_match_type match_type)
{
    return match_value_and_mark(
        m, value, &m->rmlvo.model, match_type, WILDCARD_MATCH_ALL,
        /* layout-independent */
        (xkb_layout_mask_t)GLOBAL_MATCHED_LAYOUTS
    );
}

/**
 * Match the `option` MLVO field.
 *
 * `option` input is a list of values (*not* layout-indexed).
 *
 * Matching an option field requires considering the entire list of input options
 * (e.g. `grp:menu_toggle,caps:swapescape`), because any number of them may
 * legitimately satisfy a single rule at once.
 *
 * - If the rule has no `layout` nor `variant` field, evaluation stops early on
 *   the first match.
 * - Otherwise each option potentially covers a different subset of the
 *   candidate layout indices. Candidates are narrowed by accumulating which
 *   candidates get covered:
 *
 *   - A layout-specific option (e.g. `grp:menu_toggle!1`, encoded as a non-zero
 *     `option->layouts` mask) can only cover candidates it applies to, so it
 *     is tried against `unmatched & option->layouts`.
 *   - A layout-generic option (e.g. `grp:menu_toggle`) is not restricted to
 *     specific layout indices, so a match covers every remaining candidate at
 *     once.
 *
 *   Matching stops early once every candidate is covered; otherwise every
 *   input option is tried. Any candidate left uncovered at the end cannot
 *   match the next RMLVO field (if any), so it is dropped from the candidates
 *   list.
 *
 * NOTE: The matching logic here differs from `model`, `layout`, and `variant`.
 * While the rule still provides a single value to match against, how that value
 * relates to candidate layout indices varies:
 *
 * | MLVO field | Input type                          | Candidates constraint   |
 * | ---------- | ----------------------------------- | ----------------------- |
 * | `model`    | single value                        | none                    |
 * | `layout`   | list of layout-indexed values       | 1-to-1                  |
 * | `variant`  | list of layout-indexed values       | 1-to-1                  |
 * | `option`   | list of values (not layout-indexed) | many-to-many            |
 *
 */
static bool
matcher_rule_match_option(struct matcher *m, const struct sval value,
                          enum mlvo_match_type match_type,
                          xkb_layout_mask_t *candidates)
{
    /* There is always at least one value */
    assert(!darray_empty(m->rmlvo.options));

    /* Remaining layout indices to match if MLVO has a layout/variant field */
    xkb_layout_mask_t unmatched =
        (m->mapping.defined_mlvo_mask & MLVO_LAYOUT_AND_VARIANT)
            ? *candidates
            : 0;

    bool matched = false;
    struct matched_sval *option;
    darray_foreach(option, m->rmlvo.options) {
        const xkb_layout_mask_t matchable = option->layouts
            /*
             * Layout-specific option; may match only if:
             * - there is a layout or variant field, and
             * - the option layout matches the remaining candidates.
             */
            ? (unmatched & option->layouts)
            /* Layout-generic option: no restriction */
            : (xkb_layout_mask_t)GLOBAL_MATCHED_LAYOUTS;
        if (matchable && match_value_and_mark(m, value, option, match_type,
                                              WILDCARD_MATCH_ALL, matchable)) {
            matched = true;
            unmatched &= ~matchable;
            if (!unmatched)
                break;
        }
    }
    if (unmatched) {
        /* Remove unmatched layout indices */
        *candidates &= ~unmatched;
    }
    return matched;
}

/**
 * Match the `layout` or `variant` field
 *
 * Input is a list of layout-indexed values.
 *
 * The value of the rule’s field is tested against each remaining candidate
 * index using the corresponding RMLVO input list.
 *
 * - An index that fails to match can never satisfy this rule, so it is
 *   cleared from the candidates set. This narrows what later MLVO fields in
 *   the same rule still need to consider.
 * - An index that matches remains a candidate. Evaluation stops as soon as no
 *   candidates remain, since nothing left can satisfy the rule.
 */
static bool
matcher_rule_match_layout_or_variant(struct matcher *m,
                                     darray_matched_sval *input,
                                     const struct sval value,
                                     enum mlvo_match_type match_type,
                                     xkb_layout_mask_t *candidates)
{
    /* There is always at least one value */
    assert(!darray_empty(*input));

    bool matched = false;
    /* Loop over the layout index range */
    for (xkb_layout_index_t idx = m->mapping.layout_idx_min;
         idx < m->mapping.layout_idx_max && *candidates;
         idx++)
    {
        /* Process only if layout index is enabled */
        const xkb_layout_mask_t layout = (UINT32_C(1) << idx);
        if (layout & *candidates) {
            struct matched_sval *to = &darray_item(*input, idx);
            if (match_value_and_mark(m, value, to, match_type,
                                     WILDCARD_MATCH_NONEMPTY, layout)) {
                /* Matched, keep index */
                matched = true;
            } else {
                /* Not matched, remove index */
                *candidates &= ~layout;
            }
        }
    }
    return matched;
}

/**
 * Try to match the current rule against the RMLVO input; if every MLVO
 * field in the mapping matches, apply the rule’s KcCGST value(s).
 *
 * A mapping’s header line lists which MLVO fields it tests (some subset
 * of `model`, `layout`, `variant`, `option`, in any order) and which
 * KcCGST fields it produces. When the RMLVO input has several layouts
 * (e.g. `us,de,fr`), each rule needs to be checked against each layout
 * index independently. A candidate layout mask tracks the layout indices
 * valid for the current rule. Each MLVO field affects the candidates
 * layout indices differently:
 *
 * - `model` is layout-independent: it matches globally, for every
 *   candidate at once, or not at all.
 * - `layout` and `variant` narrow the mask directly: a candidate index
 *   survives only if the RMLVO value at that index matches the rule.
 * - `option` is different because the RMLVO input may contain multiple
 *   option values, possibly covering different candidates (or all of them,
 *   for a layout-generic option). A layout candidate survives only if some
 *   option matches and covers it.
 *
 * If any MLVO field fails to match, the rule cannot apply and this function
 * returns immediately, without changing the KcCGST output. For fields that
 * operate on layout indices (`layout`, `variant`, and `option`), this occurs
 * once no candidate layout indices remain.
 *
 * If every field matches, the KcCGST value(s) are processed for `%`-expansions
 * (e.g., `%l` for matched layout, `%v` for variant, `%m` for model, `%i` for
 * layout index) and applied against the remaining candidate layouts. If the
 * ruleset spans multiple layout indices and includes an `option` field, the
 * `%`-expanded values are buffered and subsequently merged to guarantee they
 * follow layout order first, rule order second.
 *
 * Within a rule set, a successful match normally terminates evaluation.
 * The only exception is rules containing an `option` field, which do not
 * terminate the set so that additional matching options may contribute.
 */
static void
matcher_rule_apply_if_matches(struct matcher *m, struct scanner *s)
{
    /* Initial candidates (used if MLVO has a layout or variant field) */
    xkb_layout_mask_t candidate_layouts = m->mapping.layouts_candidates_mask;

    /* Loop over MLVO pattern components */
    for (mlvo_index_t i = 0; i < m->mapping.num_mlvo; i++) {
        const enum rules_mlvo mlvo = m->mapping.mlvo_at_pos[i];
        const struct sval value = m->rule.mlvo_value_at_pos[i];
        const enum mlvo_match_type match_type = m->rule.match_type_at_pos[i];
        bool matched = false;

        switch (mlvo) {
        case MLVO_MODEL:
            matched = matcher_rule_match_model(m, value, match_type);
            break;
        case MLVO_LAYOUT:
            matched = matcher_rule_match_layout_or_variant(
                m, &m->rmlvo.layouts, value, match_type, &candidate_layouts
            );
            break;
        case MLVO_VARIANT:
            matched = matcher_rule_match_layout_or_variant(
                m, &m->rmlvo.variants, value, match_type, &candidate_layouts
            );
            break;
        case MLVO_OPTION:
            matched = matcher_rule_match_option(
                m, value, match_type, &candidate_layouts
            );
            break;
        default: {
            static_assert(MLVO_OPTION == 3 &&
                          MLVO_OPTION == _MLVO_NUM_ENTRIES - 1,
                          "Unexpected MLVO field");
            assert(!"Unreachable");
        }
        }

        if (!matched)
            return;
    }

    if (m->mapping.has_multiple_layouts) {
        /* Special index: loop over the index range */
        for (xkb_layout_index_t idx = m->mapping.layout_idx_min;
             idx < m->mapping.layout_idx_max;
             idx++)
        {
            if (candidate_layouts & (UINT32_C(1) << idx)) {
                for (kccgst_index_t i = 0; i < m->mapping.num_kccgst; i++) {
                    const enum rules_kccgst kccgst = m->mapping.kccgst_at_pos[i];
                    const struct sval value = m->rule.kccgst_value_at_pos[i];
                    /*
                     * [NOTE] Layout index ranges and merging KcCGST values
                     *
                     * Layout indices match following first the order of the
                     * rules in the file, then their natural order. So do not
                     * merge with the output for now but buffer the resulting
                     * KcCGST value and wait reaching the end of the rule set.
                     *
                     * Because the rule set may also involve options, it may
                     * match multiple times for the *same* layout index. So
                     * buffer the result of *each* match.
                     *
                     * When the end of the rule set is reached, merge buffered
                     * KcCGST sequentially, following first the layouts order,
                     * then the order of the rules in the file.
                     *
                     * Example:
                     *
                     * ! model = symbols
                     *   *     = pc
                     * ! layout[any] option = symbols
                     *   C           1      = +c1:%i
                     *   C           2      = +c2:%i
                     *   B           3      = skip
                     *   B           4      = +b:%i
                     *
                     * The result of {layout: "A,B,C", options: "4,3,2,1"} is:
                     * symbols = pc+b:2+c1:3+c2:3.
                     *
                     * - `skip` was dropped because it has no explicit merge
                     *   mode;
                     * - although every rule was matched in order, the resulting
                     *   order of the symbols follows the order of the layouts,
                     *   so `+b` appears before `+c1` and `+c2`.
                     * - the relative order of the options for layout C follows
                     *   the order within the rule set, not the order of RMLVO.
                     */
                    struct kccgst_buffer * const buf = &m->pending_kccgst;
                    const darray_size_t prev_buffer_length =
                        darray_size(buf->buffer);
                    append_expanded_kccgst_value(m, s, false, &buf->buffer,
                                                 value, idx);
                    const uint32_t length = (uint32_t) (darray_size(buf->buffer)
                                          - prev_buffer_length);
                    const struct kccgst_buffer_slice slice = {
                        .length = length,
                        .kccgst = kccgst,
                        .layout = idx
                    };
                    darray_append(buf->slices, slice);
                }
            }
        }
    } else {
        /* Numeric index or no index */
        for (kccgst_index_t i = 0; i < m->mapping.num_kccgst; i++) {
            enum rules_kccgst kccgst = m->mapping.kccgst_at_pos[i];
            struct sval value = m->rule.kccgst_value_at_pos[i];
            append_expanded_kccgst_value(m, s, true, &m->kccgst[kccgst], value,
                                         m->mapping.layout_idx_min);
        }
    }

    /*
     * If a rule matches in a rule set, the rest of the set should be
     * skipped. However, rule sets matching against options may contain
     * several legitimate rules, so they are processed entirely.
     */
    if (!(is_mlvo_mask_defined(m, MLVO_OPTION))) {
        m->mapping.layouts_candidates_mask &= ~candidate_layouts;
    }
}

static enum rules_token
gettok(struct matcher *m, struct scanner *s)
{
    return lex(s, &m->val);
}

static bool
matcher_match(struct matcher *m, struct scanner *s,
              unsigned int include_depth,
              const char *string, size_t len,
              const char *file_name)
{
    enum rules_token tok;

    if (!m)
        return false;

initial:
    switch (tok = gettok(m, s)) {
    case TOK_BANG:
        goto bang;
    case TOK_END_OF_LINE:
        goto initial;
    case TOK_END_OF_FILE:
        goto finish;
    default:
        goto unexpected;
    }

bang:
    switch (tok = gettok(m, s)) {
    case TOK_GROUP_NAME:
        matcher_group_start_new(m, m->val.string);
        goto group_name;
    case TOK_INCLUDE:
        goto include_statement;
    case TOK_IDENTIFIER:
        matcher_mapping_start_new(m);
        matcher_mapping_set_mlvo(m, s, m->val.string);
        goto mapping_mlvo;
    default:
        goto unexpected;
    }

group_name:
    switch (tok = gettok(m, s)) {
    case TOK_EQUALS:
        goto group_element;
    default:
        goto unexpected;
    }

group_element:
    switch (tok = gettok(m, s)) {
    case TOK_IDENTIFIER:
        matcher_group_add_element(m, s, m->val.string);
        goto group_element;
    case TOK_END_OF_LINE:
        goto initial;
    default:
        goto unexpected;
    }

include_statement:
    switch (tok = gettok(m, s)) {
    case TOK_IDENTIFIER:
        matcher_include(m, s, include_depth, m->val.string);
        goto include_statement_end;
    default:
        goto unexpected;
    }

include_statement_end:
    switch (tok = gettok(m, s)) {
    case TOK_END_OF_LINE:
        goto initial;
    default:
        goto unexpected;
    }

mapping_mlvo:
    switch (tok = gettok(m, s)) {
    case TOK_IDENTIFIER:
        if (m->mapping.active)
            matcher_mapping_set_mlvo(m, s, m->val.string);
        goto mapping_mlvo;
    case TOK_EQUALS:
        goto mapping_kccgst;
    default:
        goto unexpected;
    }

mapping_kccgst:
    switch (tok = gettok(m, s)) {
    case TOK_IDENTIFIER:
        if (m->mapping.active)
            matcher_mapping_set_kccgst(m, s, m->val.string);
        goto mapping_kccgst;
    case TOK_END_OF_LINE:
        if (m->mapping.active && matcher_mapping_verify(m, s)) {
            matcher_mapping_set_layout_bounds(m);
            if (m->mapping.has_multiple_layouts) {
                /* Lazily reset buffers for layout index ranges.
                 * We’ll reuse the allocations. */
                darray_size(m->pending_kccgst.buffer) = 0;
                darray_size(m->pending_kccgst.slices) = 0;
            }
        }
        goto rule_mlvo_first;
    default:
        goto unexpected;
    }

rule_mlvo_first:
    switch (tok = gettok(m, s)) {
    case TOK_BANG:
        matcher_append_pending_kccgst(m);
        goto bang;
    case TOK_END_OF_LINE:
        goto rule_mlvo_first;
    case TOK_END_OF_FILE:
        matcher_append_pending_kccgst(m);
        goto finish;
    default:
        matcher_rule_start_new(m);
        goto rule_mlvo_no_tok;
    }

rule_mlvo:
    tok = gettok(m, s);
rule_mlvo_no_tok:
    switch (tok) {
    case TOK_IDENTIFIER:
        if (!m->rule.skip)
            matcher_rule_set_mlvo(m, s, m->val.string);
        goto rule_mlvo;
    case TOK_WILD_CARD_STAR:
        if (!m->rule.skip)
            matcher_rule_set_mlvo_wildcard(m, s, MLVO_MATCH_WILDCARD_LEGACY);
        goto rule_mlvo;
    case TOK_WILD_CARD_NONE:
        if (!m->rule.skip)
            matcher_rule_set_mlvo_wildcard(m, s, MLVO_MATCH_WILDCARD_NONE);
        goto rule_mlvo;
    case TOK_WILD_CARD_SOME:
        if (!m->rule.skip)
            matcher_rule_set_mlvo_wildcard(m, s, MLVO_MATCH_WILDCARD_SOME);
        goto rule_mlvo;
    case TOK_WILD_CARD_ANY:
        if (!m->rule.skip)
            matcher_rule_set_mlvo_wildcard(m, s, MLVO_MATCH_WILDCARD_ANY);
        goto rule_mlvo;
    case TOK_GROUP_NAME:
        if (!m->rule.skip)
            matcher_rule_set_mlvo_group(m, s, m->val.string);
        goto rule_mlvo;
    case TOK_EQUALS:
        goto rule_kccgst;
    default:
        goto unexpected;
    }

rule_kccgst:
    switch (tok = gettok(m, s)) {
    case TOK_IDENTIFIER:
        if (!m->rule.skip)
            matcher_rule_set_kccgst(m, s, m->val.string);
        goto rule_kccgst;
    case TOK_END_OF_LINE:
        if (!m->rule.skip)
            matcher_rule_verify(m, s);
        if (!m->rule.skip)
            matcher_rule_apply_if_matches(m, s);
        goto rule_mlvo_first;
    default:
        goto unexpected;
    }

unexpected:
    switch (tok) {
    case TOK_ERROR:
        goto error;
    default:
        goto state_error;
    }

finish:
    return true;

state_error:
    scanner_err(s, XKB_ERROR_INVALID_RULES_SYNTAX,
                "unexpected token");
error:
    return false;
}

static bool
read_rules_file(struct xkb_context *ctx,
                struct matcher *matcher,
                unsigned int include_depth,
                FILE *file,
                const char *path)
{
    bool ret;
    char *string;
    size_t size;
    struct scanner scanner;

    if (!map_file(file, &string, &size)) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "Couldn't read rules file \"%s\": %s\n",
                path, strerror(errno));
        return false;
    }

    scanner_init(&scanner, matcher->ctx, string, size, path, NULL);

    /* Basic detection of wrong character encoding.
       The first character relevant to the grammar must be ASCII:
       whitespace, !, / (for comment) */
    if (!scanner_check_supported_char_encoding(&scanner)) {
        scanner_err(&scanner, XKB_ERROR_INVALID_FILE_ENCODING,
                    "This could be a file encoding issue. "
                    "Supported encodings must be backward compatible with ASCII.");
        scanner_err(&scanner, XKB_ERROR_INVALID_FILE_ENCODING,
                    "E.g. ISO/CEI 8859 and UTF-8 are supported "
                    "but UTF-16, UTF-32 and CP1026 are not.");
        unmap_file(string, size);
        return false;
    }

    ret = matcher_match(matcher, &scanner, include_depth, string, size, path);
    unmap_file(string, size);
    return ret;
}

/**
 * XKB extension: append all *.pre or *.post optional partial rules files.
 * This is a lightweight extension mechanism to enable *simple* rules files
 * composition: rules files are just processed sequentially using the matcher
 * resulting from the previous file.
 */
static bool
xkb_resolve_partial_rules(struct xkb_context *ctx, char *path, size_t path_size,
                          const char* rules, const char* suffix,
                          struct matcher *matcher)
{
    /* Set partial rules filename: canonical rules filename + suffix */
    char partial_rules[60]; /* Arbitrary, but we do not expect long names */
    if (unlikely(!snprintf_safe(partial_rules, sizeof(partial_rules),
                                "%s%s", rules, suffix))) {
        log_err(ctx, XKB_ERROR_CANNOT_RESOLVE_RMLVO,
                "Cannot load XKB rules \"%s%s\"\n", rules, suffix);
        return false;
    }

    /*
     * Traverse all include paths and resolve all corresponding partial rules.
     * These files are optional, but required to be valid if present.
     */
    unsigned int offset = 0;
    FILE *file = NULL;
    const size_t len = strlen(partial_rules);
    while ((file = FindFileInXkbPath(ctx, "(unknown)",
                                     partial_rules, len, FILE_TYPE_RULES,
                                     path, path_size, &offset, false)) != NULL) {
        const bool ok = read_rules_file(ctx, matcher, 0, file, path);
        fclose(file);
        if (!ok) {
            log_err(ctx, XKB_ERROR_CANNOT_RESOLVE_RMLVO,
                    "Error while parsing XKB rules \"%s\"\n", path);
            return false;
        }
        offset++;
    }

    return true;
}

static bool
xkb_resolve_rules(struct xkb_context *ctx,
                  const char* rules, struct matcher *matcher,
                  struct xkb_component_names *out,
                  xkb_layout_index_t *explicit_layouts)
{
    bool ret = false;

    /* First check if the main rules file exists or error early */
    unsigned int offset = 0;
    char path[PATH_MAX];
    FILE * const file = FindFileInXkbPath(ctx, "(unknown)",
                                          rules, strlen(rules), FILE_TYPE_RULES,
                                          path, sizeof(path), &offset, true);
    if (!file) {
        log_err(ctx, XKB_ERROR_CANNOT_RESOLVE_RMLVO,
                "Cannot load XKB rules \"%s\"\n", rules);
        goto err_out;
    }

    /*
     * Final rule set is equivalent to:
     *
     * ! include <include path 1>/rules/<rules>.pre  // only if defined
     * …
     * ! include <include path n>/rules/<rules>.pre  // only if defined
     * ! include <rules>                             // main rules file
     * ! include <include path 1>/rules/<rules>.post // only if defined
     * …
     * ! include <include path n>/rules/<rules>.post // only if defined
     */

    /* XKB extension: resolve optional <rules>.pre files */
    ret = xkb_resolve_partial_rules(ctx, path, sizeof(path),
                                    rules, ".pre", matcher);
    if (!ret)
        goto err_out;

    /* Resolve main <rules> file */
    ret = read_rules_file(ctx, matcher, 0, file, path);
    if (!ret) {
        log_err(ctx, XKB_ERROR_CANNOT_RESOLVE_RMLVO,
                "Error while parsing XKB rules \"%s\"\n", path);
        goto err_out;
    }

    /* XKB extension: resolve optional <rules>.post files */
    ret = xkb_resolve_partial_rules(ctx, path, sizeof(path),
                                    rules, ".post", matcher);
    if (!ret)
        goto err_out;

    /* Check we got required components */
    if (darray_empty(matcher->kccgst[KCCGST_KEYCODES]) ||
        darray_empty(matcher->kccgst[KCCGST_TYPES]) ||
        darray_empty(matcher->kccgst[KCCGST_COMPAT]) ||
        /* darray_empty(matcher->kccgst[KCCGST_GEOMETRY]) || */
        darray_empty(matcher->kccgst[KCCGST_SYMBOLS])) {
        log_err(ctx, XKB_ERROR_CANNOT_RESOLVE_RMLVO,
                "No components returned from XKB rules \"%s\"\n", rules);
        ret = false;
        goto err_out;
    }

    darray_steal(matcher->kccgst[KCCGST_KEYCODES], &out->keycodes, NULL);
    darray_steal(matcher->kccgst[KCCGST_TYPES], &out->types, NULL);
    darray_steal(matcher->kccgst[KCCGST_COMPAT], &out->compatibility, NULL);
    darray_steal(matcher->kccgst[KCCGST_SYMBOLS], &out->symbols, NULL);
    darray_steal(matcher->kccgst[KCCGST_GEOMETRY], &out->geometry, NULL);

    struct matched_sval *mval = &matcher->rmlvo.model;
    if (!mval->matched && mval->sval.len > 0)
        log_err(matcher->ctx, XKB_ERROR_UNRECOGNIZED_RMLVO_VALUE,
                "RMLVO mismatch: unrecognized model \"%.*s\" "
                "was ignored in rules \"%s\"\n",
                (unsigned int) mval->sval.len, mval->sval.start, rules);
    darray_foreach(mval, matcher->rmlvo.layouts)
        if (!mval->matched && mval->sval.len > 0)
            log_err(matcher->ctx, XKB_ERROR_UNRECOGNIZED_RMLVO_VALUE,
                    "RMLVO mismatch: unrecognized layout \"%.*s\" "
                    "was ignored in rules \"%s\"\n",
                    (unsigned int) mval->sval.len, mval->sval.start, rules);
    darray_foreach(mval, matcher->rmlvo.variants)
        if (!mval->matched && mval->sval.len > 0)
            log_err(matcher->ctx, XKB_ERROR_UNRECOGNIZED_RMLVO_VALUE,
                    "RMLVO mismatch: unrecognized variant \"%.*s\" "
                    "was ignored in rules \"%s\"\n",
                    (unsigned int) mval->sval.len, mval->sval.start, rules);
    darray_foreach(mval, matcher->rmlvo.options) {
        if (!mval->sval.len)
            continue;

        if (!mval->matched) {
            /* No match (either layout-specific or layout-independent) */
            log_err(matcher->ctx, XKB_ERROR_UNRECOGNIZED_RMLVO_VALUE,
                    "RMLVO mismatch: unrecognized option \"%.*s\" "
                    "was ignored in rules \"%s\"\n",
                    (unsigned int) mval->sval.len, mval->sval.start, rules);
        } else if (mval->layouts && mval->layouts != mval->matched) {
            /* Partial match (layout-specific) */
            const xkb_layout_mask_t unmatched = (mval->layouts & ~mval->matched);
            assert(unmatched);
            log_err(matcher->ctx, XKB_ERROR_UNRECOGNIZED_RMLVO_VALUE,
                    "RMLVO mismatch: option \"%.*s\" with layout mask "
                    "0x%"PRIx32" was ignored in rules \"%s\"\n",
                    (unsigned int)mval->sval.len, mval->sval.start,
                    unmatched, rules);
        }
    }

    /* Set the number of explicit layouts */
    if (out->symbols != NULL && explicit_layouts != NULL) {
        *explicit_layouts = 1; /* at least one group */
        const char *symbols = out->symbols;
        /* Take the highest modifier */
        while ((symbols = strchr(symbols, ':')) != NULL && symbols[1] != '\0') {
            xkb_layout_index_t group = 0;
            const int count = parse_dec_to_uint32_t(++symbols, SIZE_MAX, &group);
            /* Update only when valid group index, but continue parsing
             * even on invalid ones, as we do not handle them here. */
            if (count > 0 && (symbols[count] == '\0' ||
                is_merge_mode_prefix(symbols[count])) &&
                group > 0 && group <= XKB_MAX_GROUPS) {
                *explicit_layouts = MAX(*explicit_layouts, group);
                symbols += count;
            }
        }
    }

err_out:
    if (file)
        fclose(file);

    return ret;
}

bool
xkb_components_from_rmlvo_builder(const struct xkb_rmlvo_builder *rmlvo,
                                  struct xkb_component_names *out,
                                  xkb_layout_index_t *explicit_layouts)
{
    const char *rules = rmlvo->rules;
    struct matcher *matcher = matcher_new_from_rmlvo(rmlvo, &rules);
    if (!matcher)
        return false;

    const bool ret = xkb_resolve_rules(rmlvo->ctx, rules, matcher,
                                       out, explicit_layouts);

    matcher_free(matcher);
    return ret;
}


bool
xkb_components_from_rules_names(struct xkb_context *ctx,
                                const struct xkb_rule_names *rmlvo,
                                struct xkb_component_names *out,
                                xkb_layout_index_t *explicit_layouts)
{
    struct matcher *matcher = matcher_new_from_names(ctx, rmlvo);
    if (!matcher)
        return false;

    const bool ret = xkb_resolve_rules(ctx, rmlvo->rules, matcher,
                                       out, explicit_layouts);

    matcher_free(matcher);
    return ret;
}
