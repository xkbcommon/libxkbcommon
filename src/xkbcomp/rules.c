/************************************************************
 * Copyright (c) 1996 by Silicon Graphics Computer Systems, Inc.
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

/*
 * Copyright © 2012 Ran Benita <ran234@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <limits.h>

#include "xkbcomp-priv.h"
#include "rules.h"
#include "include.h"
#include "scanner-utils.h"
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
    TOK_STAR,
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
            scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                        "illegal new line escape; must appear at end of line");
            return TOK_ERROR;
        }
        scanner_next(s);
        goto skip_more_whitespace_and_comments;
    }

    /* See if we're done. */
    if (scanner_eof(s)) return TOK_END_OF_FILE;

    /* New token. */
    s->token_line = s->line;
    s->token_column = s->column;

    /* Operators and punctuation. */
    if (scanner_chr(s, '!')) return TOK_BANG;
    if (scanner_chr(s, '=')) return TOK_EQUALS;
    if (scanner_chr(s, '*')) return TOK_STAR;

    /* Group name. */
    if (scanner_chr(s, '$')) {
        val->string.start = s->s + s->pos;
        val->string.len = 0;
        while (is_ident(scanner_peek(s))) {
            scanner_next(s);
            val->string.len++;
        }
        if (val->string.len == 0) {
            scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                        "unexpected character after \'$\'; expected name");
            return TOK_ERROR;
        }
        return TOK_GROUP_NAME;
    }

    /* Include statement. */
    if (scanner_lit(s, "include"))
        return TOK_INCLUDE;

    /* Identifier. */
    if (is_ident(scanner_peek(s))) {
        val->string.start = s->s + s->pos;
        val->string.len = 0;
        while (is_ident(scanner_peek(s))) {
            scanner_next(s);
            val->string.len++;
        }
        return TOK_IDENTIFIER;
    }

    scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
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

#define SVAL_LIT(literal) { literal, sizeof(literal) - 1 }

static const struct sval rules_mlvo_svals[_MLVO_NUM_ENTRIES] = {
    [MLVO_MODEL] = SVAL_LIT("model"),
    [MLVO_LAYOUT] = SVAL_LIT("layout"),
    [MLVO_VARIANT] = SVAL_LIT("variant"),
    [MLVO_OPTION] = SVAL_LIT("option"),
};

enum rules_kccgst {
    KCCGST_KEYCODES,
    KCCGST_TYPES,
    KCCGST_COMPAT,
    KCCGST_SYMBOLS,
    KCCGST_GEOMETRY,
    _KCCGST_NUM_ENTRIES
};

static const struct sval rules_kccgst_svals[_KCCGST_NUM_ENTRIES] = {
    [KCCGST_KEYCODES] = SVAL_LIT("keycodes"),
    [KCCGST_TYPES] = SVAL_LIT("types"),
    [KCCGST_COMPAT] = SVAL_LIT("compat"),
    [KCCGST_SYMBOLS] = SVAL_LIT("symbols"),
    [KCCGST_GEOMETRY] = SVAL_LIT("geometry"),
};

/* We use this to keep score whether an mlvo was matched or not; if not,
 * we warn the user that his preference was ignored. */
struct matched_sval {
    struct sval sval;
    bool matched;
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
    int mlvo_at_pos[_MLVO_NUM_ENTRIES];
    unsigned int num_mlvo;
    unsigned int defined_mlvo_mask;
    bool has_layout_idx_range;
    /* This member has 2 uses:
     * • Keep track of layout and variant indexes while parsing MLVO headers.
     * • Store layout/variant range afterwards.
     * Thus provide 2 structs to reflect these semantics in the code. */
    union {
        struct { xkb_layout_index_t layout_idx, variant_idx; };
        struct { xkb_layout_index_t layout_idx_min, layout_idx_max; };
    };
    /* This member has 2 uses:
     * • Check if the mapping is active by interpreting the value as a boolean.
     * • Keep track of the remaining layout indexes to match.
     * Thus provide 2 names to reflect these semantics in the code. */
    union {
        xkb_layout_mask_t active;
        xkb_layout_mask_t layouts_candidates_mask;
    };
    int kccgst_at_pos[_KCCGST_NUM_ENTRIES];
    unsigned int num_kccgst;
    unsigned int defined_kccgst_mask;
};

enum mlvo_match_type {
    MLVO_MATCH_NORMAL = 0,
    MLVO_MATCH_WILDCARD,
    MLVO_MATCH_GROUP,
};

enum wildcard_match_type {
    /* Match only non-empty strings */
    WILDCARD_MATCH_NONEMPTY = 0,
    /* Match all strings */
    WILDCARD_MATCH_ALL,
};

struct rule {
    struct sval mlvo_value_at_pos[_MLVO_NUM_ENTRIES];
    enum mlvo_match_type match_type_at_pos[_MLVO_NUM_ENTRIES];
    unsigned int num_mlvo_values;
    struct sval kccgst_value_at_pos[_KCCGST_NUM_ENTRIES];
    unsigned int num_kccgst_values;
    bool skip;
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
split_comma_separated_mlvo(const char *s)
{
    darray_matched_sval arr = darray_new();

    /*
     * Make sure the array returned by this function always includes at
     * least one value, e.g. "" -> { "" } and "," -> { "", "" }.
     */

    if (!s) {
        struct matched_sval val = { .sval = { NULL, 0 } };
        darray_append(arr, val);
        return arr;
    }

    while (true) {
        struct matched_sval val = { .sval = { s, 0 } };
        while (*s != '\0' && *s != ',') { s++; val.sval.len++; }
        val.sval = strip_spaces(val.sval);
        darray_append(arr, val);
        if (*s == '\0') break;
        if (*s == ',') s++;
    }

    return arr;
}

static struct matcher *
matcher_new(struct xkb_context *ctx,
            const struct xkb_rule_names *rmlvo)
{
    struct matcher *m = calloc(1, sizeof(*m));
    if (!m)
        return NULL;

    m->ctx = ctx;
    m->rmlvo.model.sval.start = rmlvo->model;
    m->rmlvo.model.sval.len = strlen_safe(rmlvo->model);
    m->rmlvo.layouts = split_comma_separated_mlvo(rmlvo->layout);
    m->rmlvo.variants = split_comma_separated_mlvo(rmlvo->variant);
    m->rmlvo.options = split_comma_separated_mlvo(rmlvo->options);

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
    struct group *group;
    if (!m)
        return;
    darray_free(m->rmlvo.layouts);
    darray_free(m->rmlvo.variants);
    darray_free(m->rmlvo.options);
    darray_foreach(group, m->groups)
        darray_free(group->elements);
    for (int i = 0; i < _KCCGST_NUM_ENTRIES; i++)
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
                unsigned include_depth,
                FILE *file,
                const char *path);

static void
matcher_include(struct matcher *m, struct scanner *parent_scanner,
                unsigned include_depth,
                struct sval inc)
{
    struct scanner s; /* parses the !include value */

    scanner_init(&s, m->ctx, inc.start, inc.len,
                 parent_scanner->file_name, NULL);
    s.token_line = parent_scanner->token_line;
    s.token_column = parent_scanner->token_column;
    s.buf_pos = 0;

    if (include_depth >= MAX_INCLUDE_DEPTH) {
        scanner_err(&s, XKB_LOG_MESSAGE_NO_ID,
                    "maximum include depth (%d) exceeded; "
                    "maybe there is an include loop?",
                    MAX_INCLUDE_DEPTH);
        return;
    }

    /* Proceed to %-expansion */
    while (!scanner_eof(&s) && !scanner_eol(&s)) {
        if (scanner_chr(&s, '%')) {
            if (scanner_chr(&s, '%')) {
                scanner_buf_append(&s, '%');
            }
            else if (scanner_chr(&s, 'H')) {
                const char *home = xkb_context_getenv(m->ctx, "HOME");
                if (!home) {
                    scanner_err(&s, XKB_LOG_MESSAGE_NO_ID,
                                "%%H was used in an include statement, but the "
                                "HOME environment variable is not set");
                    return;
                }
                if (!scanner_buf_appends(&s, home)) {
                    scanner_err(&s, XKB_LOG_MESSAGE_NO_ID,
                                "include path after expanding %%H is too long");
                    return;
                }
            }
            else if (scanner_chr(&s, 'S')) {
                const char *default_root =
                    xkb_context_include_path_get_system_path(m->ctx);
                if (!scanner_buf_appends(&s, default_root) ||
                    !scanner_buf_appends(&s, "/rules")) {
                    scanner_err(&s, XKB_LOG_MESSAGE_NO_ID,
                                "include path after expanding %%S is too long");
                    return;
                }
            }
            else if (scanner_chr(&s, 'E')) {
                const char *default_root =
                    xkb_context_include_path_get_extra_path(m->ctx);
                if (!scanner_buf_appends(&s, default_root) ||
                    !scanner_buf_appends(&s, "/rules")) {
                    scanner_err(&s, XKB_LOG_MESSAGE_NO_ID,
                                "include path after expanding %%E is too long");
                    return;
                }
            }
            else {
                scanner_err(&s, XKB_LOG_MESSAGE_NO_ID,
                            "unknown %% format (%c) in include statement",
                            scanner_peek(&s));
                return;
            }
        }
        else {
            scanner_buf_append(&s, scanner_next(&s));
        }
    }
    if (!scanner_buf_append(&s, '\0')) {
        scanner_err(&s, XKB_LOG_MESSAGE_NO_ID,
                    "include path is too long");
        return;
    }

    /* Lookup rules file in XKB paths only if the include path is relative */
    unsigned int offset = 0;
    FILE *file;
    bool absolute_path = is_absolute(s.buf);
    if (absolute_path)
        file = fopen(s.buf, "rb");
    else
        file = FindFileInXkbPath(m->ctx, s.buf, FILE_TYPE_RULES, NULL, &offset);

    while (file) {
        bool ret = read_rules_file(m->ctx, m, include_depth + 1, file, s.buf);
        fclose(file);
        if (ret)
            return;
        /* Failed to parse rules or get all the components */
        log_err(m->ctx, XKB_LOG_MESSAGE_NO_ID,
                "No components returned from included XKB rules \"%s\"\n",
                s.buf);
        if (absolute_path)
            break;
        /* Try next XKB path */
        offset++;
        file = FindFileInXkbPath(m->ctx, s.buf, FILE_TYPE_RULES, NULL, &offset);
    }

    log_err(m->ctx, XKB_LOG_MESSAGE_NO_ID,
            "Failed to open included XKB rules \"%s\"\n",
            s.buf);
}

static void
matcher_mapping_start_new(struct matcher *m)
{
    for (unsigned i = 0; i < _MLVO_NUM_ENTRIES; i++)
        m->mapping.mlvo_at_pos[i] = -1;
    for (unsigned i = 0; i < _KCCGST_NUM_ENTRIES; i++)
        m->mapping.kccgst_at_pos[i] = -1;
    m->mapping.has_layout_idx_range = false;
    m->mapping.layout_idx = m->mapping.variant_idx = XKB_LAYOUT_INVALID;
    m->mapping.num_mlvo = m->mapping.num_kccgst = 0;
    m->mapping.defined_mlvo_mask = 0;
    m->mapping.defined_kccgst_mask = 0;
    m->mapping.active = true;
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
    if (max_len > 3 && s[1] == '%' && s[2] == 'i' && s[3] == ']') {
        /* Special index: %i */
        return 4; /* == length "[%i]" */
    }
    /* Numeric index */

#define parse_layout_int_index(s, endptr, index, out)              \
    char *endptr;                                                  \
    long index = strtol(&s[1], &endptr, 10);                       \
    if (index < 0 || endptr[0] != ']' || index > XKB_MAX_GROUPS)   \
        return -1;                                                 \
    /* To zero-based index. */                                     \
    *out = (xkb_layout_index_t)index - 1;                          \
    return (int)(endptr - s + 1) /* == length "[index]" */

    parse_layout_int_index(s, endptr, index, out);
}

/* Special layout indexes */
#define LAYOUT_INDEX_SINGLE XKB_LAYOUT_INVALID
#define LAYOUT_INDEX_FIRST  (XKB_LAYOUT_INVALID - 3)
#define LAYOUT_INDEX_LATER  (XKB_LAYOUT_INVALID - 2)
#define LAYOUT_INDEX_ANY    (XKB_LAYOUT_INVALID - 1)

#if XKB_MAX_GROUPS >= LAYOUT_INDEX_FIRST
    #error "Cannot define special indexes"
#endif
#if LAYOUT_INDEX_FIRST  >= LAYOUT_INDEX_LATER || \
    LAYOUT_INDEX_LATER  >= LAYOUT_INDEX_ANY || \
    LAYOUT_INDEX_ANY    >= XKB_LAYOUT_INVALID || \
    LAYOUT_INDEX_SINGLE != XKB_LAYOUT_INVALID
    #error "Special indexes must respect certain order"
#endif

#define LAYOUT_INDEX_SINGLE_STR "single"
#define LAYOUT_INDEX_FIRST_STR  "first"
#define LAYOUT_INDEX_LATER_STR  "later"
#define LAYOUT_INDEX_ANY_STR    "any"

/* Parse index of layout/variant in MLVO mapping */
static int
extract_mapping_layout_index(const char *s, size_t max_len,
                             xkb_layout_index_t *out)
{
    *out = XKB_LAYOUT_INVALID;
    if (max_len < 3 || s[0] != '[')
        return -1;
#define if_index(s, index, out)                                    \
    /* Compare s against "index]" */                               \
    if (strncmp(s, index##_STR "]", sizeof(index##_STR)) == 0) {   \
        *out = index;                                              \
        return sizeof(index##_STR) + 1; /* == length("[index]") */ \
    }
    else if_index(&s[1], LAYOUT_INDEX_SINGLE, out)
    else if_index(&s[1], LAYOUT_INDEX_FIRST, out)
    else if_index(&s[1], LAYOUT_INDEX_LATER, out)
    else if_index(&s[1], LAYOUT_INDEX_ANY, out)
    else {
        /* Numeric index */
        parse_layout_int_index(s, endptr, index, out);
    }
#undef if_index
#undef LAYOUT_INDEX_SINGLE
}

#define is_mlvo_mask_defined(m, mlvo) \
    ((m)->mapping.defined_mlvo_mask & (1u << (mlvo)))

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
        scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                    "invalid mapping: %.*s is not a valid value here; "
                    "ignoring rule set",
                    ident.len, ident.start);
        m->mapping.active = false;
        return;
    }

    if (is_mlvo_mask_defined(m, mlvo)) {
        scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                    "invalid mapping: %.*s appears twice on the same line; "
                    "ignoring rule set",
                    mlvo_sval.len, mlvo_sval.start);
        m->mapping.active = false;
        return;
    }

    /* If there are leftovers still, it must be an index. */
    if (mlvo_sval.len < ident.len) {
        xkb_layout_index_t idx;
        int consumed = extract_mapping_layout_index(ident.start + mlvo_sval.len,
                                                    ident.len - mlvo_sval.len,
                                                    &idx);
        if ((int) (ident.len - mlvo_sval.len) != consumed) {
            scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                        "invalid mapping: \"%.*s\" may only be followed by a "
                        "valid group index; ignoring rule set",
                        mlvo_sval.len, mlvo_sval.start);
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
            scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                        "invalid mapping: \"%.*s\" cannot be followed by a group "
                        "index; ignoring rule set",
                        mlvo_sval.len, mlvo_sval.start);
            m->mapping.active = false;
            return;
        }
    }

    /* Check that if both layout and variant are defined, then they must have
       the same index */
    if (((mlvo == MLVO_LAYOUT && is_mlvo_mask_defined(m, MLVO_VARIANT)) ||
         (mlvo == MLVO_VARIANT && is_mlvo_mask_defined(m, MLVO_LAYOUT))) &&
        m->mapping.layout_idx != m->mapping.variant_idx) {
        scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                    "invalid mapping: \"layout\" index must be the same as the "
                    "\"variant\" index");
        m->mapping.active = false;
        return;
    }

    m->mapping.mlvo_at_pos[m->mapping.num_mlvo] = mlvo;
    m->mapping.defined_mlvo_mask |= 1u << mlvo;
    m->mapping.num_mlvo++;
}

static void
matcher_mapping_set_layout_bounds(struct matcher *m)
{
    /* Handle case where one of the index is XKB_LAYOUT_INVALID */
    xkb_layout_index_t idx = MIN(m->mapping.layout_idx, m->mapping.variant_idx);
    switch (idx) {
        case LAYOUT_INDEX_LATER:
            m->mapping.has_layout_idx_range = true;
            m->mapping.layout_idx_min = 1;
            m->mapping.layout_idx_max = MIN(XKB_MAX_GROUPS,
                                            darray_size(m->rmlvo.layouts));
            m->mapping.layouts_candidates_mask =
                /* All but the first layout */
                ((1u << m->mapping.layout_idx_max) - 1) & ~1;
            break;
        case LAYOUT_INDEX_ANY:
            m->mapping.has_layout_idx_range = true;
            m->mapping.layout_idx_min = 0;
            m->mapping.layout_idx_max = MIN(XKB_MAX_GROUPS,
                                            darray_size(m->rmlvo.layouts));
            m->mapping.layouts_candidates_mask =
                /* All layouts */
                (1u << m->mapping.layout_idx_max) - 1;
            break;
        case LAYOUT_INDEX_FIRST:
        case XKB_LAYOUT_INVALID:
            /* No index or first index */
            idx = 0;
            /* fallthrough */
        default:
            /* Mere layout index */
            m->mapping.has_layout_idx_range = false;
            m->mapping.layout_idx_min = idx;
            m->mapping.layout_idx_max = idx + 1;
            m->mapping.layouts_candidates_mask = 1u << idx;
    }
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
        scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                    "invalid mapping: %.*s is not a valid value here; "
                    "ignoring rule set",
                    ident.len, ident.start);
        m->mapping.active = false;
        return;
    }

    if (m->mapping.defined_kccgst_mask & (1u << kccgst)) {
        scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                    "invalid mapping: %.*s appears twice on the same line; "
                    "ignoring rule set",
                    kccgst_sval.len, kccgst_sval.start);
        m->mapping.active = false;
        return;
    }

    m->mapping.kccgst_at_pos[m->mapping.num_kccgst] = kccgst;
    m->mapping.defined_kccgst_mask |= 1u << kccgst;
    m->mapping.num_kccgst++;
}

static bool
matcher_mapping_verify(struct matcher *m, struct scanner *s)
{
    if (m->mapping.num_mlvo == 0) {
        scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                    "invalid mapping: must have at least one value on the left "
                    "hand side; ignoring rule set");
        goto skip;
    }

    if (m->mapping.num_kccgst == 0) {
        scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                    "invalid mapping: must have at least one value on the right "
                    "hand side; ignoring rule set");
        goto skip;
    }

    /*
     * This following is very stupid, but this is how it works.
     * See the "Notes" section in the overview above.
     */

    if (is_mlvo_mask_defined(m, MLVO_LAYOUT)) {
        switch (m->mapping.layout_idx) {
            case XKB_LAYOUT_INVALID:
                /* Layout rule without index matches when
                 * exactly 1 layout is specified */
                if (darray_size(m->rmlvo.layouts) > 1)
                    goto skip;
                break;
            case LAYOUT_INDEX_ANY:
            case LAYOUT_INDEX_LATER:
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
        switch (m->mapping.variant_idx) {
            case XKB_LAYOUT_INVALID:
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
    if (m->rule.num_mlvo_values + 1 > m->mapping.num_mlvo) {
        scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
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
matcher_rule_set_mlvo_wildcard(struct matcher *m, struct scanner *s)
{
    struct sval dummy = { NULL, 0 };
    matcher_rule_set_mlvo_common(m, s, dummy, MLVO_MATCH_WILDCARD);
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
    if (m->rule.num_kccgst_values + 1 > m->mapping.num_kccgst) {
        scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
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
    struct group *group;
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
match_value(struct matcher *m, struct sval val, struct sval to,
            enum mlvo_match_type match_type,
            enum wildcard_match_type wildcard_type)
{
    if (match_type == MLVO_MATCH_WILDCARD)
        /* Wild card match empty values only if explicitly required */
        return wildcard_type == WILDCARD_MATCH_ALL || !!to.len;
    if (match_type == MLVO_MATCH_GROUP)
        return match_group(m, val, to);
    return svaleq(val, to);
}

static bool
match_value_and_mark(struct matcher *m, struct sval val,
                     struct matched_sval *to, enum mlvo_match_type match_type,
                     enum wildcard_match_type wildcard_type)
{
    bool matched = match_value(m, val, to->sval, match_type, wildcard_type);
    if (matched)
        to->matched = true;
    return matched;
}

/*
 * This function performs %-expansion on @value (see overview above),
 * and appends the result to @expanded.
 */
static bool
expand_rmlvo_in_kccgst_value(struct matcher *m, struct scanner *s,
                             struct sval value, xkb_layout_index_t layout_idx,
                             darray_char *expanded, unsigned *i)
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
        (*i)++;
        char index_str[MAX_LAYOUT_INDEX_STR_LENGTH + 1];
        int count = snprintf(index_str, sizeof(index_str), "%"PRIu32,
                             layout_idx + 1);
        darray_appends_nullterminate(*expanded, index_str, count);
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
    bool expanded_index = false;
    if (*i < value.len && str[*i] == '[') {
        if (mlv != MLVO_LAYOUT && mlv != MLVO_VARIANT) {
            scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                        "invalid index in %%-expansion; "
                        "may only index layout or variant");
            goto error;
        }

        int consumed = extract_layout_index(str + (*i), value.len - (*i), &idx);
        if (consumed == -1) goto error;
        if (idx == XKB_LAYOUT_INVALID) {
            /* %i encountered */
            idx = layout_idx;
            expanded_index = true;
        }
        *i += consumed;
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
                   (expanded_index || darray_size(m->rmlvo.layouts) > 1)) {
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
                   (expanded_index || darray_size(m->rmlvo.variants) > 1)) {
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
                                 expanded_value->sval.len);
    if (sfx != 0)
        darray_appends_nullterminate(*expanded, &sfx, 1);
    expanded_value->matched = true;

    return true;

error:
    scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
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
    unsigned int prefix_idx, unsigned int *i)
{
    const char *str = value.start;

    /* “all” followed by nothing or by a layout separator */
    if ((*i + 3 <= value.len || is_merge_mode_prefix(str[*i + 3])) &&
        str[*i] == 'a' && str[*i+1] == 'l' && str[*i+2] == 'l') {
        if (has_layout_idx_range)
            scanner_vrb(s, 2, XKB_LOG_MESSAGE_NO_ID,
                        "Using :all qualifier with indexes range "
                        "is not recommended.");
        /* Add at least one layout */
        darray_appends_nullterminate(*expanded, "1", 1);
        /* Check for more layouts (slow path) */
        if (darray_size(m->rmlvo.layouts) > 1) {
            char layout_index[MAX_LAYOUT_INDEX_STR_LENGTH + 1];
            const size_t prefix_length = darray_size(*expanded) - prefix_idx - 1;
            xkb_layout_index_t l;
            for (l = 1;
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
                int count = snprintf(layout_index, sizeof(layout_index),
                                     "%"PRIu32, l + 1);
                darray_appends_nullterminate(*expanded, layout_index, count);
            }
        }
        *i += 3;
    }
}

/*
 * This function performs %-expansion and :all-expansion on @value
 * (see overview above), and appends the result to @to.
 */
static bool
append_expanded_kccgst_value(struct matcher *m, struct scanner *s,
                             darray_char *to, struct sval value,
                             xkb_layout_index_t layout_idx)
{
    const char *str = value.start;
    darray_char expanded = darray_new();
    char ch;
    bool expanded_plus, to_plus;
    unsigned int last_item_idx = 0;
    bool has_separator = false;

    for (unsigned i = 0; i < value.len; ) {
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
                if (++i >= value.len ||
                    !expand_rmlvo_in_kccgst_value(m, s, value, layout_idx,
                                                  &expanded, &i))
                        goto error;
                break;
            /* New item */
            case MERGE_OVERRIDE_PREFIX:
            case MERGE_AUGMENT_PREFIX:
                darray_appends_nullterminate(expanded, &str[i++], 1);
                last_item_idx = darray_size(expanded) - 1;
                has_separator = true;
                break;
            /* Just a normal character. */
            default:
                darray_appends_nullterminate(expanded, &str[i++], 1);
        }
    }

    /*
     * Appending  bar to  foo ->  foo (not an error if this happens)
     * Appending +bar to  foo ->  foo+bar
     * Appending  bar to +foo ->  bar+foo
     * Appending +bar to +foo -> +foo+bar
     */

    ch = (darray_empty(expanded) ? '\0' : darray_item(expanded, 0));
    expanded_plus = is_merge_mode_prefix(ch);
    ch = (darray_empty(*to) ? '\0' : darray_item(*to, 0));
    to_plus = is_merge_mode_prefix(ch);

    if (expanded_plus || darray_empty(*to))
        darray_appends_nullterminate(*to,
                                     darray_items(expanded),
                                     darray_size(expanded));
    else if (to_plus)
        darray_prepends_nullterminate(*to,
                                      darray_items(expanded),
                                      darray_size(expanded));

    darray_free(expanded);
    return true;

error:
    darray_free(expanded);
    return false;
}

static void
matcher_rule_verify(struct matcher *m, struct scanner *s)
{
    if (m->rule.num_mlvo_values != m->mapping.num_mlvo ||
        m->rule.num_kccgst_values != m->mapping.num_kccgst) {
        scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                    "invalid rule: must have same number of values "
                    "as mapping line; ignoring rule");
        m->rule.skip = true;
    }
}

static void
matcher_rule_apply_if_matches(struct matcher *m, struct scanner *s)
{
    /* Initial candidates (used if m->mapping.has_layout_idx_range == true) */
    xkb_layout_mask_t candidate_layouts = m->mapping.layouts_candidates_mask;
    xkb_layout_index_t idx;
    /* Loop over MLVO pattern components */
    for (unsigned i = 0; i < m->mapping.num_mlvo; i++) {
        enum rules_mlvo mlvo = m->mapping.mlvo_at_pos[i];
        struct sval value = m->rule.mlvo_value_at_pos[i];
        enum mlvo_match_type match_type = m->rule.match_type_at_pos[i];
        struct matched_sval *to;
        bool matched = false;

        /* NOTE: Wild card matches empty values only for model and options, as
         * implemented in libxkbfile and xserver. The reason for such different
         * treatment is not documented. */
        if (mlvo == MLVO_MODEL) {
            to = &m->rmlvo.model;
            matched = match_value_and_mark(m, value, to, match_type,
                                           WILDCARD_MATCH_ALL);
        }
#define process_component(_component, m, value, idx, candidate_layouts, to,  \
                          match_type, matched)                               \
            if (m->mapping.has_layout_idx_range) {                           \
                /* Special index: loop over the index range */               \
                for (idx = m->mapping.layout_idx_min;                        \
                     idx < m->mapping.layout_idx_max;                        \
                     idx++)                                                  \
                {                                                            \
                    /* Process only if index not skipped */                  \
                    xkb_layout_mask_t mask = 1u << idx;                      \
                    if (candidate_layouts & mask) {                          \
                        to = &darray_item(m->rmlvo._component, idx);         \
                        if (match_value_and_mark(m, value, to, match_type,   \
                                                 WILDCARD_MATCH_NONEMPTY)) { \
                            /* Mark matched, keep index */                   \
                            matched = true;                                  \
                        } else {                                             \
                            /* Not matched, remove index */                  \
                            candidate_layouts &= ~mask;                      \
                        }                                                    \
                    }                                                        \
                }                                                            \
            } else {                                                         \
                /* Numeric index or no index */                              \
                to = &darray_item(m->rmlvo._component,                       \
                                  m->mapping.layout_idx_min);                \
                matched = match_value_and_mark(m, value, to, match_type,     \
                                               WILDCARD_MATCH_NONEMPTY);     \
            }
        else if (mlvo == MLVO_LAYOUT) {
            process_component(layouts, m, value, idx, candidate_layouts, to,
                              match_type, matched)
        }
        else if (mlvo == MLVO_VARIANT) {
            process_component(variants, m, value, idx, candidate_layouts, to,
                              match_type, matched)
        }
#undef process_component
        else if (mlvo == MLVO_OPTION) {
            darray_foreach(to, m->rmlvo.options) {
                matched = match_value_and_mark(m, value, to, match_type,
                                               WILDCARD_MATCH_ALL);
                if (matched)
                    break;
            }
        }

        if (!matched)
            return;
    }

    if (m->mapping.has_layout_idx_range) {
        /* Special index: loop over the index range */
        for (idx = m->mapping.layout_idx_min;
             idx < m->mapping.layout_idx_max;
             idx++)
        {
            if (candidate_layouts & (1u << idx)) {
                for (unsigned i = 0; i < m->mapping.num_kccgst; i++) {
                    enum rules_kccgst kccgst = m->mapping.kccgst_at_pos[i];
                    struct sval value = m->rule.kccgst_value_at_pos[i];
                    append_expanded_kccgst_value(m, s, &m->kccgst[kccgst],
                                                 value, idx);
                }
            }
        }
    } else {
        /* Numeric index or no index */
        for (unsigned i = 0; i < m->mapping.num_kccgst; i++) {
            enum rules_kccgst kccgst = m->mapping.kccgst_at_pos[i];
            struct sval value = m->rule.kccgst_value_at_pos[i];
            append_expanded_kccgst_value(m, s, &m->kccgst[kccgst], value,
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
              unsigned include_depth,
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
        if (m->mapping.active && matcher_mapping_verify(m, s))
            matcher_mapping_set_layout_bounds(m);
        goto rule_mlvo_first;
    default:
        goto unexpected;
    }

rule_mlvo_first:
    switch (tok = gettok(m, s)) {
    case TOK_BANG:
        goto bang;
    case TOK_END_OF_LINE:
        goto rule_mlvo_first;
    case TOK_END_OF_FILE:
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
    case TOK_STAR:
        if (!m->rule.skip)
            matcher_rule_set_mlvo_wildcard(m, s);
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
    scanner_err(s, XKB_LOG_MESSAGE_NO_ID,
                "unexpected token");
error:
    return false;
}

static bool
read_rules_file(struct xkb_context *ctx,
                struct matcher *matcher,
                unsigned include_depth,
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
        scanner_err(&scanner, XKB_LOG_MESSAGE_NO_ID,
            "This could be a file encoding issue. "
            "Supported encodings must be backward compatible with ASCII.");
        scanner_err(&scanner, XKB_LOG_MESSAGE_NO_ID,
            "E.g. ISO/CEI 8859 and UTF-8 are supported "
            "but UTF-16, UTF-32 and CP1026 are not.");
        unmap_file(string, size);
        return false;
    }

    ret = matcher_match(matcher, &scanner, include_depth, string, size, path);
    unmap_file(string, size);
    return ret;
}

bool
xkb_components_from_rules(struct xkb_context *ctx,
                          const struct xkb_rule_names *rmlvo,
                          struct xkb_component_names *out,
                          xkb_layout_index_t *explicit_layouts)
{
    bool ret = false;
    FILE *file;
    char *path = NULL;
    struct matcher *matcher = NULL;
    struct matched_sval *mval;
    unsigned int offset = 0;

    file = FindFileInXkbPath(ctx, rmlvo->rules, FILE_TYPE_RULES, &path, &offset);
    if (!file) {
        log_err(ctx, XKB_ERROR_CANNOT_RESOLVE_RMLVO,
                "Cannot load XKB rules \"%s\"\n", rmlvo->rules);
        goto err_out;
    }

    matcher = matcher_new(ctx, rmlvo);

    ret = read_rules_file(ctx, matcher, 0, file, path);
    if (!ret ||
        darray_empty(matcher->kccgst[KCCGST_KEYCODES]) ||
        darray_empty(matcher->kccgst[KCCGST_TYPES]) ||
        darray_empty(matcher->kccgst[KCCGST_COMPAT]) ||
        /* darray_empty(matcher->kccgst[KCCGST_GEOMETRY]) || */
        darray_empty(matcher->kccgst[KCCGST_SYMBOLS])) {
        log_err(ctx, XKB_ERROR_CANNOT_RESOLVE_RMLVO,
                "No components returned from XKB rules \"%s\"\n", path);
        ret = false;
        goto err_out;
    }

    darray_steal(matcher->kccgst[KCCGST_KEYCODES], &out->keycodes, NULL);
    darray_steal(matcher->kccgst[KCCGST_TYPES], &out->types, NULL);
    darray_steal(matcher->kccgst[KCCGST_COMPAT], &out->compat, NULL);
    darray_steal(matcher->kccgst[KCCGST_SYMBOLS], &out->symbols, NULL);
    darray_free(matcher->kccgst[KCCGST_GEOMETRY]);

    mval = &matcher->rmlvo.model;
    if (!mval->matched && mval->sval.len > 0)
        log_err(matcher->ctx, XKB_ERROR_CANNOT_RESOLVE_RMLVO,
                "Unrecognized RMLVO model \"%.*s\" was ignored\n",
                mval->sval.len, mval->sval.start);
    darray_foreach(mval, matcher->rmlvo.layouts)
        if (!mval->matched && mval->sval.len > 0)
            log_err(matcher->ctx, XKB_ERROR_CANNOT_RESOLVE_RMLVO,
                    "Unrecognized RMLVO layout \"%.*s\" was ignored\n",
                    mval->sval.len, mval->sval.start);
    darray_foreach(mval, matcher->rmlvo.variants)
        if (!mval->matched && mval->sval.len > 0)
            log_err(matcher->ctx, XKB_ERROR_CANNOT_RESOLVE_RMLVO,
                    "Unrecognized RMLVO variant \"%.*s\" was ignored\n",
                    mval->sval.len, mval->sval.start);
    darray_foreach(mval, matcher->rmlvo.options)
        if (!mval->matched && mval->sval.len > 0)
            log_err(matcher->ctx, XKB_ERROR_CANNOT_RESOLVE_RMLVO,
                    "Unrecognized RMLVO option \"%.*s\" was ignored\n",
                    mval->sval.len, mval->sval.start);

    /* Set the number of explicit layouts */
    if (out->symbols != NULL && explicit_layouts != NULL) {
        *explicit_layouts = 1; /* at least one group */
        const char *symbols = out->symbols;
        /* Take the highest modifier */
        while ((symbols = strchr(symbols, ':')) != NULL && symbols[1] != '\0') {
            char *endptr;
            unsigned long group = strtoul(++symbols, &endptr, 10);
            /* Update only when valid group index, but continue parsing
             * even on invalid ones, as we do not handle them here. */
            if (group > 0 && group <= XKB_MAX_GROUPS)
                *explicit_layouts = MAX(*explicit_layouts,
                                        (xkb_layout_index_t)group);
            symbols = endptr;
        }
    }

err_out:
    if (file)
        fclose(file);
    matcher_free(matcher);
    free(path);
    return ret;
}
