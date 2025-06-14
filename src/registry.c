/*
 * Copyright © 2020 Red Hat, Inc.
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <libxml/parser.h>

#include "xkbcommon/xkbregistry.h"
#include "messages-codes.h"
#include "utils.h"
#include "util-list.h"
#include "util-mem.h"

struct rxkb_object;

/**
 * All our objects are refcounted and are linked to iterate through them.
 * Abstract those bits away into a shared parent class so we can generate
 * most of the functions through macros.
 */
struct rxkb_object {
    struct rxkb_object *parent;
    uint32_t refcount;
    struct list link;
};

struct rxkb_iso639_code {
    struct rxkb_object base;
    char *code;
};

struct rxkb_iso3166_code {
    struct rxkb_object base;
    char *code;
};

enum context_state {
    CONTEXT_NEW,
    CONTEXT_PARSED,
    CONTEXT_FAILED,
};

struct rxkb_context {
    struct rxkb_object base;
    enum context_state context_state;

    bool load_extra_rules_files;
    bool use_secure_getenv;

    struct list models;         /* list of struct rxkb_models */
    struct list layouts;        /* list of struct rxkb_layouts */
    struct list option_groups;  /* list of struct rxkb_option_group */

    darray(char *) includes;


    ATTR_PRINTF(3, 0) void (*log_fn)(struct rxkb_context *ctx,
                                     enum rxkb_log_level level,
                                     const char *fmt, va_list args);
    enum rxkb_log_level log_level;

    void *userdata;
};

struct rxkb_model {
    struct rxkb_object base;

    char *name;
    char *vendor;
    char *description;
    enum rxkb_popularity popularity;
};

struct rxkb_layout {
    struct rxkb_object base;

    char *name;
    char *brief;
    char *description;
    char *variant;
    enum rxkb_popularity popularity;

    struct list iso639s;  /* list of struct rxkb_iso639_code */
    struct list iso3166s; /* list of struct rxkb_iso3166_code */
};

struct rxkb_option_group {
    struct rxkb_object base;

    bool allow_multiple;
    struct list options; /* list of struct rxkb_options */
    char *name;
    char *description;
    enum rxkb_popularity popularity;
};

struct rxkb_option {
    struct rxkb_object base;

    char *name;
    char *brief;
    char *description;
    enum rxkb_popularity popularity;
    bool layout_specific;
};

static bool
parse(struct rxkb_context *ctx, const char *path,
      enum rxkb_popularity popularity);

ATTR_PRINTF(3, 4)
static void
rxkb_log(struct rxkb_context *ctx, enum rxkb_log_level level,
         const char *fmt, ...)
{
    va_list args;

    if (ctx->log_level < level)
        return;

    va_start(args, fmt);
    ctx->log_fn(ctx, level, fmt, args);
    va_end(args);
}

/*
 * The format is not part of the argument list in order to avoid the
 * "ISO C99 requires rest arguments to be used" warning when only the
 * format is supplied without arguments. Not supplying it would still
 * result in an error, though.
 */
#define rxkb_log_with_code(ctx, level, msg_id, fmt, ...) \
    rxkb_log(ctx, level, PREPEND_MESSAGE_ID(msg_id, fmt), ##__VA_ARGS__)
#define log_dbg(ctx, ...) \
    rxkb_log((ctx), RXKB_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define log_info(ctx, ...) \
    rxkb_log((ctx), RXKB_LOG_LEVEL_INFO, __VA_ARGS__)
#define log_warn(ctx, id, ...) \
    rxkb_log_with_code((ctx), RXKB_LOG_LEVEL_WARNING, id, __VA_ARGS__)
#define log_err(ctx, id, ...) \
    rxkb_log_with_code((ctx), RXKB_LOG_LEVEL_ERROR, id, __VA_ARGS__)
#define log_wsgo(ctx, id, ...) \
    rxkb_log_with_code((ctx), RXKB_LOG_LEVEL_CRITICAL, id, __VA_ARGS__)


#define DECLARE_REF_UNREF_FOR_TYPE(type_) \
struct type_ * type_##_ref(struct type_ *object) { \
    rxkb_object_ref(&object->base); \
    return object; \
} \
struct type_ * type_##_unref(struct type_ *object) { \
    if (!object) return NULL; \
    assert(object->base.refcount >= 1); \
    if (--object->base.refcount == 0) {\
        type_##_destroy(object); \
        list_remove(&object->base.link);\
        free(object); \
    } \
    return NULL;\
}

#define DECLARE_CREATE_FOR_TYPE(type_) \
static inline struct type_ * type_##_create(struct rxkb_object *parent) { \
    struct type_ *t = calloc(1, sizeof *t); \
    if (t) \
        rxkb_object_init(&t->base, parent); \
    return t; \
}

#define DECLARE_TYPED_GETTER_FOR_TYPE(type_, field_, rtype_) \
rtype_ type_##_get_##field_(struct type_ *object) { \
    return object->field_; \
}

#define DECLARE_GETTER_FOR_TYPE(type_, field_) \
   DECLARE_TYPED_GETTER_FOR_TYPE(type_, field_, const char*)

#define DECLARE_FIRST_NEXT_FOR_TYPE(type_, parent_type_, parent_field_) \
struct type_ * type_##_first(struct parent_type_ *parent) { \
    struct type_ *o = NULL; \
    if (!list_empty(&parent->parent_field_)) \
        o = list_first_entry(&parent->parent_field_, o, base.link); \
    return o; \
} \
struct type_ * \
type_##_next(struct type_ *o) \
{ \
    struct parent_type_ *parent; \
    struct type_ *next; \
    parent = container_of(o->base.parent, struct parent_type_, base); \
    next = list_first_entry(&o->base.link, o, base.link); \
    if (list_is_last(&parent->parent_field_, &o->base.link)) \
        return NULL; \
    return next; \
}

static void
rxkb_object_init(struct rxkb_object *object, struct rxkb_object *parent)
{
    object->refcount = 1;
    object->parent = parent;
    list_init(&object->link);
}

static void *
rxkb_object_ref(struct rxkb_object *object)
{
    assert(object->refcount >= 1);
    ++object->refcount;
    return object;
}

static void
rxkb_iso639_code_destroy(struct rxkb_iso639_code *code)
{
    free(code->code);
}

struct rxkb_iso639_code *
rxkb_layout_get_iso639_first(struct rxkb_layout *layout)
{
    struct rxkb_iso639_code *code = NULL;

    if (!list_empty(&layout->iso639s))
        code = list_first_entry(&layout->iso639s, code, base.link);

    return code;
}

struct rxkb_iso639_code *
rxkb_iso639_code_next(struct rxkb_iso639_code *code)
{
    struct rxkb_iso639_code *next = NULL;
    struct rxkb_layout *layout;

    layout = container_of(code->base.parent, struct rxkb_layout, base);

    if (list_is_last(&layout->iso639s, &code->base.link))
        return NULL;

    next = list_first_entry(&code->base.link, code, base.link);

    return next;
}

DECLARE_REF_UNREF_FOR_TYPE(rxkb_iso639_code);
DECLARE_CREATE_FOR_TYPE(rxkb_iso639_code);
DECLARE_GETTER_FOR_TYPE(rxkb_iso639_code, code);

static void
rxkb_iso3166_code_destroy(struct rxkb_iso3166_code *code)
{
    free(code->code);
}

struct rxkb_iso3166_code *
rxkb_layout_get_iso3166_first(struct rxkb_layout *layout)
{
    struct rxkb_iso3166_code *code = NULL;

    if (!list_empty(&layout->iso3166s))
        code = list_first_entry(&layout->iso3166s, code, base.link);

    return code;
}

struct rxkb_iso3166_code *
rxkb_iso3166_code_next(struct rxkb_iso3166_code *code)
{
    struct rxkb_iso3166_code *next = NULL;
    struct rxkb_layout *layout;

    layout = container_of(code->base.parent, struct rxkb_layout, base);

    if (list_is_last(&layout->iso3166s, &code->base.link))
        return NULL;

    next = list_first_entry(&code->base.link, code, base.link);

    return next;
}

DECLARE_REF_UNREF_FOR_TYPE(rxkb_iso3166_code);
DECLARE_CREATE_FOR_TYPE(rxkb_iso3166_code);
DECLARE_GETTER_FOR_TYPE(rxkb_iso3166_code, code);

static void
rxkb_option_destroy(struct rxkb_option *o)
{
    free(o->name);
    free(o->brief);
    free(o->description);
}

DECLARE_REF_UNREF_FOR_TYPE(rxkb_option);
DECLARE_CREATE_FOR_TYPE(rxkb_option);
DECLARE_GETTER_FOR_TYPE(rxkb_option, name);
DECLARE_GETTER_FOR_TYPE(rxkb_option, brief);
DECLARE_GETTER_FOR_TYPE(rxkb_option, description);
DECLARE_TYPED_GETTER_FOR_TYPE(rxkb_option, popularity, enum rxkb_popularity);
bool rxkb_option_is_layout_specific(struct rxkb_option *object) {
    return object->layout_specific;
}
DECLARE_FIRST_NEXT_FOR_TYPE(rxkb_option, rxkb_option_group, options);

static void
rxkb_layout_destroy(struct rxkb_layout *l)
{
    struct rxkb_iso639_code *iso639, *tmp_639;
    struct rxkb_iso3166_code *iso3166, *tmp_3166;

    free(l->name);
    free(l->brief);
    free(l->description);
    free(l->variant);

    list_for_each_safe(iso639, tmp_639, &l->iso639s, base.link) {
        rxkb_iso639_code_unref(iso639);
    }
    list_for_each_safe(iso3166, tmp_3166, &l->iso3166s, base.link) {
        rxkb_iso3166_code_unref(iso3166);
    }
}

DECLARE_REF_UNREF_FOR_TYPE(rxkb_layout);
DECLARE_CREATE_FOR_TYPE(rxkb_layout);
DECLARE_GETTER_FOR_TYPE(rxkb_layout, name);
DECLARE_GETTER_FOR_TYPE(rxkb_layout, brief);
DECLARE_GETTER_FOR_TYPE(rxkb_layout, description);
DECLARE_GETTER_FOR_TYPE(rxkb_layout, variant);
DECLARE_TYPED_GETTER_FOR_TYPE(rxkb_layout, popularity, enum rxkb_popularity);
DECLARE_FIRST_NEXT_FOR_TYPE(rxkb_layout, rxkb_context, layouts);

static void
rxkb_model_destroy(struct rxkb_model *m)
{
    free(m->name);
    free(m->vendor);
    free(m->description);
}

DECLARE_REF_UNREF_FOR_TYPE(rxkb_model);
DECLARE_CREATE_FOR_TYPE(rxkb_model);
DECLARE_GETTER_FOR_TYPE(rxkb_model, name);
DECLARE_GETTER_FOR_TYPE(rxkb_model, vendor);
DECLARE_GETTER_FOR_TYPE(rxkb_model, description);
DECLARE_TYPED_GETTER_FOR_TYPE(rxkb_model, popularity, enum rxkb_popularity);
DECLARE_FIRST_NEXT_FOR_TYPE(rxkb_model, rxkb_context, models);

static void
rxkb_option_group_destroy(struct rxkb_option_group *og)
{
    struct rxkb_option *o, *otmp;

    free(og->name);
    free(og->description);

    list_for_each_safe(o, otmp, &og->options, base.link) {
        rxkb_option_unref(o);
    }
}

bool
rxkb_option_group_allows_multiple(struct rxkb_option_group *g)
{
    return g->allow_multiple;
}

DECLARE_REF_UNREF_FOR_TYPE(rxkb_option_group);
DECLARE_CREATE_FOR_TYPE(rxkb_option_group);
DECLARE_GETTER_FOR_TYPE(rxkb_option_group, name);
DECLARE_GETTER_FOR_TYPE(rxkb_option_group, description);
DECLARE_TYPED_GETTER_FOR_TYPE(rxkb_option_group, popularity, enum rxkb_popularity);
DECLARE_FIRST_NEXT_FOR_TYPE(rxkb_option_group, rxkb_context, option_groups);

static void
rxkb_context_destroy(struct rxkb_context *ctx)
{
    struct rxkb_model *m, *mtmp;
    struct rxkb_layout *l, *ltmp;
    struct rxkb_option_group *og, *ogtmp;
    char **path;

    list_for_each_safe(m, mtmp, &ctx->models, base.link)
        rxkb_model_unref(m);
    assert(list_empty(&ctx->models));

    list_for_each_safe(l, ltmp, &ctx->layouts, base.link)
        rxkb_layout_unref(l);
    assert(list_empty(&ctx->layouts));

    list_for_each_safe(og, ogtmp, &ctx->option_groups, base.link)
        rxkb_option_group_unref(og);
    assert(list_empty(&ctx->option_groups));

    darray_foreach(path, ctx->includes)
        free(*path);
    darray_free(ctx->includes);

    assert(darray_empty(ctx->includes));
}

DECLARE_REF_UNREF_FOR_TYPE(rxkb_context);
DECLARE_CREATE_FOR_TYPE(rxkb_context);
DECLARE_TYPED_GETTER_FOR_TYPE(rxkb_context, log_level, enum rxkb_log_level);

static char *
rxkb_context_getenv(struct rxkb_context *ctx, const char *name)
{
    if (ctx->use_secure_getenv) {
        return secure_getenv(name);
    } else {
        return getenv(name);
    }
}


void
rxkb_context_set_log_level(struct rxkb_context *ctx,
                           enum rxkb_log_level level)
{
    ctx->log_level = level;
}

static const char *
log_level_to_prefix(enum rxkb_log_level level)
{
    switch (level) {
    case RXKB_LOG_LEVEL_DEBUG:
        return "xkbregistry: DEBUG: ";
    case RXKB_LOG_LEVEL_INFO:
        return "xkbregistry: INFO: ";
    case RXKB_LOG_LEVEL_WARNING:
        return "xkbregistry: WARNING: ";
    case RXKB_LOG_LEVEL_ERROR:
        return "xkbregistry: ERROR: ";
    case RXKB_LOG_LEVEL_CRITICAL:
        return "xkbregistry: CRITICAL: ";
    default:
        return NULL;
    }
}

ATTR_PRINTF(3, 0) static void
default_log_fn(struct rxkb_context *ctx, enum rxkb_log_level level,
               const char *fmt, va_list args)
{
    const char *prefix = log_level_to_prefix(level);

    if (prefix)
        fprintf(stderr, "%s", prefix);
    vfprintf(stderr, fmt, args);
}

static enum rxkb_log_level
log_level(const char *level) {
    char *endptr;
    enum rxkb_log_level lvl;

    errno = 0;
    lvl = strtol(level, &endptr, 10);
    if (errno == 0 && (endptr[0] == '\0' || is_space(endptr[0])))
        return lvl;
    if (istreq_prefix("crit", level))
        return RXKB_LOG_LEVEL_CRITICAL;
    if (istreq_prefix("err", level))
        return RXKB_LOG_LEVEL_ERROR;
    if (istreq_prefix("warn", level))
        return RXKB_LOG_LEVEL_WARNING;
    if (istreq_prefix("info", level))
        return RXKB_LOG_LEVEL_INFO;
    if (istreq_prefix("debug", level) || istreq_prefix("dbg", level))
        return RXKB_LOG_LEVEL_DEBUG;

    return RXKB_LOG_LEVEL_ERROR;
}

struct rxkb_context *
rxkb_context_new(enum rxkb_context_flags flags)
{
    struct rxkb_context *ctx = rxkb_context_create(NULL);
    const char *env;

    if (!ctx)
        return NULL;

    ctx->context_state = CONTEXT_NEW;
    ctx->load_extra_rules_files = flags & RXKB_CONTEXT_LOAD_EXOTIC_RULES;
    ctx->use_secure_getenv = !(flags & RXKB_CONTEXT_NO_SECURE_GETENV);
    ctx->log_fn = default_log_fn;
    ctx->log_level = RXKB_LOG_LEVEL_ERROR;

    /* Environment overwrites defaults. */
    env = rxkb_context_getenv(ctx, "RXKB_LOG_LEVEL");
    if (env)
        rxkb_context_set_log_level(ctx, log_level(env));

    list_init(&ctx->models);
    list_init(&ctx->layouts);
    list_init(&ctx->option_groups);

    if (!(flags & RXKB_CONTEXT_NO_DEFAULT_INCLUDES) &&
        !rxkb_context_include_path_append_default(ctx)) {
        rxkb_context_unref(ctx);
        return NULL;
    }

    return ctx;
}

void
rxkb_context_set_log_fn(struct rxkb_context *ctx,
                        void (*log_fn)(struct rxkb_context *ctx,
                                       enum rxkb_log_level level,
                                       const char *fmt, va_list args))
{
    ctx->log_fn = (log_fn ? log_fn : default_log_fn);
}

bool
rxkb_context_include_path_append(struct rxkb_context *ctx, const char *path)
{
    struct stat stat_buf;
    int err;
    char *tmp = NULL;
    char rules[PATH_MAX];

    if (ctx->context_state != CONTEXT_NEW) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "include paths can only be appended to a new context\n");
        return false;
    }

    err = stat(path, &stat_buf);
    if (err != 0)
        return false;
    if (!S_ISDIR(stat_buf.st_mode))
        return false;

    if (!check_eaccess(path, R_OK | X_OK))
        return false;

    /* Pre-filter for the 99.9% case - if we can't assemble the default ruleset
     * path, complain here instead of during parsing later. The niche cases
     * where this is the wrong behaviour aren't worth worrying about.
     */
    if (!snprintf_safe(rules, sizeof(rules), "%s/rules/%s.xml",
                       path, DEFAULT_XKB_RULES)) {
        log_err(ctx, XKB_ERROR_INVALID_PATH,
                "Path is too long: expected max length of %zu, "
                "got: %s/rules/%s.xml\n",
                sizeof(rules), path, DEFAULT_XKB_RULES);
        return false;
    }

    tmp = strdup(path);
    if (!tmp)
        return false;

    darray_append(ctx->includes, tmp);

    return true;
}

bool
rxkb_context_include_path_append_default(struct rxkb_context *ctx)
{
    const char *home, *xdg, *root, *extra;
    char user_path[PATH_MAX];
    bool ret = false;

    if (ctx->context_state != CONTEXT_NEW) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "include paths can only be appended to a new context\n");
        return false;
    }

    home = rxkb_context_getenv(ctx, "HOME");

    xdg = rxkb_context_getenv(ctx, "XDG_CONFIG_HOME");
    if (xdg != NULL) {
        if (snprintf_safe(user_path, sizeof(user_path), "%s/xkb", xdg))
            ret |= rxkb_context_include_path_append(ctx, user_path);
    } else if (home != NULL) {
        /* XDG_CONFIG_HOME fallback is $HOME/.config/ */
        if (snprintf_safe(user_path, sizeof(user_path), "%s/.config/xkb", home))
            ret |= rxkb_context_include_path_append(ctx, user_path);
    }

    if (home != NULL) {
        if (snprintf_safe(user_path, sizeof(user_path), "%s/.xkb", home))
            ret |= rxkb_context_include_path_append(ctx, user_path);
    }

    extra = rxkb_context_getenv(ctx, "XKB_CONFIG_EXTRA_PATH");
    if (extra != NULL)
        ret |= rxkb_context_include_path_append(ctx, extra);
    else
        ret |= rxkb_context_include_path_append(ctx, DFLT_XKB_CONFIG_EXTRA_PATH);

    root = rxkb_context_getenv(ctx, "XKB_CONFIG_ROOT");
    if (root != NULL)
        ret |= rxkb_context_include_path_append(ctx, root);
    else
        ret |= rxkb_context_include_path_append(ctx, DFLT_XKB_CONFIG_ROOT);

    return ret;
}

bool
rxkb_context_parse_default_ruleset(struct rxkb_context *ctx)
{
    return rxkb_context_parse(ctx, DEFAULT_XKB_RULES);
}

bool
rxkb_context_parse(struct rxkb_context *ctx, const char *ruleset)
{
    char **path;
    bool success = false;

    if (ctx->context_state != CONTEXT_NEW) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "parse must only be called on a new context\n");
        return false;
    }

    darray_foreach_reverse(path, ctx->includes) {
        char rules[PATH_MAX];

        if (snprintf_safe(rules, sizeof(rules), "%s/rules/%s.xml",
                           *path, ruleset)) {
            log_dbg(ctx, "Parsing %s\n", rules);
            if (parse(ctx, rules, RXKB_POPULARITY_STANDARD))
                success = true;
        }

        if (ctx->load_extra_rules_files &&
            snprintf_safe(rules, sizeof(rules), "%s/rules/%s.extras.xml",
                          *path, ruleset)) {
            log_dbg(ctx, "Parsing %s\n", rules);
            if (parse(ctx, rules, RXKB_POPULARITY_EXOTIC))
                success = true;
        }
    }

    ctx->context_state = success ? CONTEXT_PARSED : CONTEXT_FAILED;

    return success;
}


void
rxkb_context_set_user_data(struct rxkb_context *ctx, void *userdata)
{
    ctx->userdata = userdata;
}

void *
rxkb_context_get_user_data(struct rxkb_context *ctx)
{
    return ctx->userdata;
}

static inline bool
is_node(xmlNode *node, const char *name)
{
    return node->type == XML_ELEMENT_NODE &&
        xmlStrEqual(node->name, (const xmlChar*)name);
}

/* return a copy of the text content from the first text node of this node */
static char *
extract_text(xmlNode *node)
{
    xmlNode *n;

    for (n = node->children; n; n = n->next) {
        if (n->type == XML_TEXT_NODE)
            return (char *)xmlStrdup(n->content);
    }
    return NULL;
}

/* Data from “configItem” node */
struct config_item {
    char *name;
    char *description;
    char *brief;
    char *vendor;
    enum rxkb_popularity popularity;
    bool layout_specific;
};

#define config_item_new(popularity_) { \
    .name = NULL, \
    .description = NULL, \
    .brief = NULL, \
    .vendor = NULL, \
    .popularity = (popularity_), \
    .layout_specific = false \
}

static void
config_item_free(struct config_item *config) {
    free(config->name);
    free(config->description);
    free(config->brief);
    free(config->vendor);
}

static bool
parse_config_item(struct rxkb_context *ctx, xmlNode *parent,
                  struct config_item *config)
{
    xmlNode *node = NULL;
    xmlNode *ci = NULL;

    for (ci = parent->children; ci; ci = ci->next) {
        if (is_node(ci, "configItem")) {
            /* Process attributes */
            xmlChar *raw_popularity = xmlGetProp(ci, (const xmlChar*)"popularity");
            if (raw_popularity) {
                if (xmlStrEqual(raw_popularity, (const xmlChar*)"standard"))
                    config->popularity = RXKB_POPULARITY_STANDARD;
                else if (xmlStrEqual(raw_popularity, (const xmlChar*)"exotic"))
                    config->popularity = RXKB_POPULARITY_EXOTIC;
                else
                    log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                            "xml:%u: invalid popularity attribute: expected "
                            "'standard' or 'exotic', got: '%s'\n",
                            ci->line, raw_popularity);
            }
            xmlFree(raw_popularity);

            /* Note: this is only useful for options */
            xmlChar *raw_layout_specific =
                xmlGetProp(ci, (const xmlChar*)"layout-specific");
            if (raw_layout_specific &&
                xmlStrEqual(raw_layout_specific, (const xmlChar*)"true"))
                config->layout_specific = true;
            xmlFree(raw_layout_specific);

            /* Process children */
            for (node = ci->children; node; node = node->next) {
                if (is_node(node, "name"))
                    config->name = extract_text(node);
                else if (is_node(node, "description"))
                    config->description = extract_text(node);
                else if (is_node(node, "shortDescription"))
                    config->brief = extract_text(node);
                else if (is_node(node, "vendor"))
                    config->vendor = extract_text(node);
                /* Note: the DTD allows for vendor + brief but models only use
                 * vendor and everything else only uses shortDescription */
            }

            if (!config->name || !strlen(config->name))  {
                log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                        "xml:%u: missing required element 'name'\n",
                        ci->line);
                config_item_free(config);
                return false;
            }

            return true; /* only one configItem allowed in the dtd */
        }
    }

    return false;
}

static void
parse_model(struct rxkb_context *ctx, xmlNode *model,
            enum rxkb_popularity popularity)
{
    struct config_item config = config_item_new(popularity);

    if (parse_config_item(ctx, model, &config)) {
        struct rxkb_model *m;

        list_for_each(m, &ctx->models, base.link) {
            if (streq(m->name, config.name)) {
                config_item_free(&config);
                return;
            }
        }

        /* new model */
        m = rxkb_model_create(&ctx->base);
        m->name = steal(&config.name);
        m->description = steal(&config.description);
        m->vendor = steal(&config.vendor);
        m->popularity = config.popularity;
        list_append(&ctx->models, &m->base.link);
    }
}

static void
parse_model_list(struct rxkb_context *ctx, xmlNode *model_list,
                 enum rxkb_popularity popularity)
{
    xmlNode *node = NULL;

    for (node = model_list->children; node; node = node->next) {
        if (is_node(node, "model"))
            parse_model(ctx, node, popularity);
    }
}

static void
parse_language_list(xmlNode *language_list, struct rxkb_layout *layout)
{
    xmlNode *node = NULL;
    struct rxkb_iso639_code *code;

    for (node = language_list->children; node; node = node->next) {
        if (is_node(node, "iso639Id")) {
            char *str = extract_text(node);
            struct rxkb_object *parent;

            if (!str || strlen(str) != 3) {
                free(str);
                continue;
            }

            parent = &layout->base;
            code = rxkb_iso639_code_create(parent);
            code->code = str;
            list_append(&layout->iso639s, &code->base.link);
        }
    }
}

static void
parse_country_list(xmlNode *country_list, struct rxkb_layout *layout)
{
    xmlNode *node = NULL;
    struct rxkb_iso3166_code *code;

    for (node = country_list->children; node; node = node->next) {
        if (is_node(node, "iso3166Id")) {
            char *str = extract_text(node);
            struct rxkb_object *parent;

            if (!str || strlen(str) != 2) {
                free(str);
                continue;
            }

            parent = &layout->base;
            code = rxkb_iso3166_code_create(parent);
            code->code = str;
            list_append(&layout->iso3166s, &code->base.link);
        }
    }
}

static void
parse_variant(struct rxkb_context *ctx, struct rxkb_layout *l,
              xmlNode *variant, enum rxkb_popularity popularity)
{
    xmlNode *ci;
    struct config_item config = config_item_new(popularity);

    if (parse_config_item(ctx, variant, &config)) {
        struct rxkb_layout *v;
        bool exists = false;

        list_for_each(v, &ctx->layouts, base.link) {
            if (streq_null(v->variant, config.name) &&
                streq(v->name, l->name)) {
                exists = true;
                break;
            }
        }

        if (!exists) {
            v = rxkb_layout_create(&ctx->base);
            list_init(&v->iso639s);
            list_init(&v->iso3166s);
            v->name = strdup(l->name);
            v->variant = steal(&config.name);
            v->description = steal(&config.description);
            // if variant omits brief, inherit from parent layout.
            v->brief = config.brief == NULL ? strdup_safe(l->brief) : steal(&config.brief);
            v->popularity = config.popularity;
            list_append(&ctx->layouts, &v->base.link);

            for (ci = variant->children; ci; ci = ci->next) {
                xmlNode *node;

                if (!is_node(ci, "configItem"))
                    continue;

                bool found_language_list = false;
                bool found_country_list = false;
                for (node = ci->children; node; node = node->next) {
                    if (is_node(node, "languageList")) {
                        parse_language_list(node, v);
                        found_language_list = true;
                    }
                    if (is_node(node, "countryList")) {
                        parse_country_list(node, v);
                        found_country_list = true;
                    }
                }
                if (!found_language_list) {
                    // inherit from parent layout
                    struct rxkb_iso639_code* x;
                    list_for_each(x, &l->iso639s, base.link) {
                        struct rxkb_iso639_code* code = rxkb_iso639_code_create(&v->base);
                        code->code = strdup(x->code);
                        list_append(&v->iso639s, &code->base.link);
                    }
                }
                if (!found_country_list) {
                    // inherit from parent layout
                    struct rxkb_iso3166_code* x;
                    list_for_each(x, &l->iso3166s, base.link) {
                        struct rxkb_iso3166_code* code = rxkb_iso3166_code_create(&v->base);
                        code->code = strdup(x->code);
                        list_append(&v->iso3166s, &code->base.link);
                    }
                }
            }
        } else {
            config_item_free(&config);
        }
    }
}

static void
parse_variant_list(struct rxkb_context *ctx, struct rxkb_layout *l,
                   xmlNode *variant_list, enum rxkb_popularity popularity)
{
    xmlNode *node = NULL;

    for (node = variant_list->children; node; node = node->next) {
        if (is_node(node, "variant"))
            parse_variant(ctx, l, node, popularity);
    }
}

static void
parse_layout(struct rxkb_context *ctx, xmlNode *layout,
             enum rxkb_popularity popularity)
{
    struct config_item config = config_item_new(popularity);
    struct rxkb_layout *l;
    xmlNode *node = NULL;
    bool exists = false;

    if (!parse_config_item(ctx, layout, &config))
        return;

    list_for_each(l, &ctx->layouts, base.link) {
        if (streq(l->name, config.name) && l->variant == NULL) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        l = rxkb_layout_create(&ctx->base);
        list_init(&l->iso639s);
        list_init(&l->iso3166s);
        l->name = steal(&config.name);
        l->variant = NULL;
        l->description = steal(&config.description);
        l->brief = steal(&config.brief);
        l->popularity = config.popularity;
        list_append(&ctx->layouts, &l->base.link);
    } else {
        config_item_free(&config);
    }

    for (node = layout->children; node; node = node->next) {
        if (is_node(node, "variantList")) {
            parse_variant_list(ctx, l, node, popularity);
        }
        if (!exists && is_node(node, "configItem")) {
            xmlNode *ll;
            for (ll = node->children; ll; ll = ll->next) {
                if (is_node(ll, "languageList"))
                    parse_language_list(ll, l);
                if (is_node(ll, "countryList"))
                    parse_country_list(ll, l);
            }
        }
    }
}

static void
parse_layout_list(struct rxkb_context *ctx, xmlNode *layout_list,
                  enum rxkb_popularity popularity)
{
    xmlNode *node = NULL;

    for (node = layout_list->children; node; node = node->next) {
        if (is_node(node, "layout"))
            parse_layout(ctx, node, popularity);
    }
}

static void
parse_option(struct rxkb_context *ctx, struct rxkb_option_group *group,
             xmlNode *option, enum rxkb_popularity popularity)
{
    struct config_item config = config_item_new(popularity);

    if (parse_config_item(ctx, option, &config)) {
        struct rxkb_option *o;

        list_for_each(o, &group->options, base.link) {
            if (streq(o->name, config.name)) {
                config_item_free(&config);
                return;
            }
        }

        o = rxkb_option_create(&group->base);
        o->name = steal(&config.name);
        o->description = steal(&config.description);
        o->popularity = config.popularity;
        o->layout_specific = config.layout_specific;
        list_append(&group->options, &o->base.link);
    }
}

static void
parse_group(struct rxkb_context *ctx, xmlNode *group,
            enum rxkb_popularity popularity)
{
    struct config_item config = config_item_new(popularity);
    struct rxkb_option_group *g;
    xmlNode *node = NULL;
    xmlChar *multiple;
    bool exists = false;

    if (!parse_config_item(ctx, group, &config))
        return;

    list_for_each(g, &ctx->option_groups, base.link) {
        if (streq(g->name, config.name)) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        g = rxkb_option_group_create(&ctx->base);
        g->name = steal(&config.name);
        g->description = steal(&config.description);
        g->popularity = config.popularity;

        multiple = xmlGetProp(group, (const xmlChar*)"allowMultipleSelection");
        if (multiple && xmlStrEqual(multiple, (const xmlChar*)"true"))
            g->allow_multiple = true;
        xmlFree(multiple);

        list_init(&g->options);
        list_append(&ctx->option_groups, &g->base.link);
    } else {
        config_item_free(&config);
    }

    for (node = group->children; node; node = node->next) {
        if (is_node(node, "option"))
            parse_option(ctx, g, node, popularity);
    }
}

static void
parse_option_list(struct rxkb_context *ctx, xmlNode *option_list,
                  enum rxkb_popularity popularity)
{
    xmlNode *node = NULL;

    for (node = option_list->children; node; node = node->next) {
        if (is_node(node, "group"))
            parse_group(ctx, node, popularity);
    }
}

static void
parse_rules_xml(struct rxkb_context *ctx, xmlNode *root,
                enum rxkb_popularity popularity)
{
    xmlNode *node = NULL;

    for (node = root->children; node; node = node->next) {
        if (is_node(node, "modelList"))
            parse_model_list(ctx, node, popularity);
        else if (is_node(node, "layoutList"))
            parse_layout_list(ctx, node, popularity);
        else if (is_node(node, "optionList"))
            parse_option_list(ctx, node, popularity);
    }
}

static void
ATTR_PRINTF(2, 0)
xml_error_func(void *ctx, const char *msg, ...)
{
    static char buf[PATH_MAX];
    static int slen = 0;
    va_list args;
    int rc;

    /* libxml2 prints IO errors from bad includes paths by
     * calling the error function once per word. So we get to
     * re-assemble the message here and print it when we get
     * the line break. My enthusiasm about this is indescribable.
     */
    va_start(args, msg);
    rc = vsnprintf(&buf[slen], sizeof(buf) - slen, msg, args);
    va_end(args);

    /* This shouldn't really happen */
    if (rc < 0) {
        log_err(ctx, XKB_ERROR_INSUFFICIENT_BUFFER_SIZE,
                "+++ out of cheese error. redo from start +++\n");
        slen = 0;
        memset(buf, 0, sizeof(buf));
        return;
    }

    slen += rc;
    if (slen >= (int)sizeof(buf)) {
        /* truncated, let's flush this */
        buf[sizeof(buf) - 1] = '\n';
        slen = sizeof(buf);
    }

    /* We're assuming here that the last character is \n. */
    if (buf[slen - 1] == '\n') {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID, "%s", buf);
        memset(buf, 0, sizeof(buf));
        slen = 0;
    }
}

#ifdef HAVE_XML_CTXT_SET_ERRORHANDLER
static void
xml_structured_error_func(void *userData, const xmlError * error)
{
    xmlFormatError(error, xml_error_func, userData);
}
#endif

#ifdef XML_PARSE_NO_XXE
#define _XML_OPTIONS (XML_PARSE_NONET | XML_PARSE_NOENT | XML_PARSE_NO_XXE)
#else
#define _XML_OPTIONS (XML_PARSE_NONET)
#endif

static bool
validate(struct rxkb_context *ctx, xmlDoc *doc)
{
    bool success = false;
    /* This is a modified version of the xkeyboard-config xkb.dtd:
     * • xkeyboard-config requires modelList, layoutList and optionList,
     *   but we allow for any of those to be missing.
     * • xkeyboard-config sets default value of “popularity” to “standard”,
     *   but we set this value depending if we are currently parsing an
     *   “extras” rule file.
     */
    const char dtdstr[] =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!ELEMENT xkbConfigRegistry (modelList?, layoutList?, optionList?)>\n"
        "<!ATTLIST xkbConfigRegistry version CDATA \"1.1\">\n"
        "<!ELEMENT modelList (model*)>\n"
        "<!ELEMENT model (configItem)>\n"
        "<!ELEMENT layoutList (layout*)>\n"
        "<!ELEMENT layout (configItem,  variantList?)>\n"
        "<!ELEMENT optionList (group*)>\n"
        "<!ELEMENT variantList (variant*)>\n"
        "<!ELEMENT variant (configItem)>\n"
        "<!ELEMENT group (configItem, option*)>\n"
        "<!ATTLIST group allowMultipleSelection (true|false) \"false\">\n"
        "<!ELEMENT option (configItem)>\n"
        "<!ELEMENT configItem (name, shortDescription?, description?, vendor?, countryList?, languageList?, hwList?)>\n"
        "<!ATTLIST configItem layout-specific (true|false) \"false\">\n"
        "<!ATTLIST configItem popularity (standard|exotic) #IMPLIED>\n"
        "<!ELEMENT name (#PCDATA)>\n"
        "<!ELEMENT shortDescription (#PCDATA)>\n"
        "<!ELEMENT description (#PCDATA)>\n"
        "<!ELEMENT vendor (#PCDATA)>\n"
        "<!ELEMENT countryList (iso3166Id+)>\n"
        "<!ELEMENT iso3166Id (#PCDATA)>\n"
        "<!ELEMENT languageList (iso639Id+)>\n"
        "<!ELEMENT iso639Id (#PCDATA)>\n"
        "<!ELEMENT hwList (hwId+)>\n"
        "<!ELEMENT hwId (#PCDATA)>\n";

#ifdef HAVE_XML_CTXT_PARSE_DTD
    /* Use safer function with context if available, and set
     * the contextual error handler. */
    xmlParserCtxtPtr xmlCtxt = xmlNewParserCtxt();
    if (!xmlCtxt)
        return false;
    xmlCtxtSetErrorHandler(xmlCtxt, xml_structured_error_func, ctx);
    xmlCtxtSetOptions(xmlCtxt, _XML_OPTIONS | XML_PARSE_DTDLOAD);

    xmlParserInputPtr pinput =
        xmlNewInputFromMemory(NULL, dtdstr, ARRAY_SIZE(dtdstr) - 1,
                              XML_INPUT_BUF_STATIC);
    if (!pinput)
        goto dtd_error;

    xmlDtd *dtd = xmlCtxtParseDtd(xmlCtxt, pinput, NULL, NULL);
#else
    /* Note: do not use xmlParserInputBufferCreateStatic, it generates random
     * DTD validity errors for unknown reasons */
    xmlParserInputBufferPtr buf =
        xmlParserInputBufferCreateMem(dtdstr, ARRAY_SIZE(dtdstr) - 1,
                                      XML_CHAR_ENCODING_NONE);
    if (!buf)
        goto dtd_error;
    xmlDtd *dtd = xmlIOParseDTD(NULL, buf, XML_CHAR_ENCODING_UTF8);
#endif

    if (!dtd) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID, "Failed to load DTD\n");
        goto dtd_error;
    }

#ifdef HAVE_XML_CTXT_PARSE_DTD
    success = xmlCtxtValidateDtd(xmlCtxt, doc, dtd);
#else
    xmlValidCtxt *dtdvalid = xmlNewValidCtxt();
    success = xmlValidateDtd(dtdvalid, doc, dtd);
    if (dtdvalid)
        xmlFreeValidCtxt(dtdvalid);
#endif

    xmlFreeDtd(dtd);

dtd_error:
#ifdef HAVE_XML_CTXT_PARSE_DTD
    xmlFreeParserCtxt(xmlCtxt);
#endif
    return success;
}

static bool
parse(struct rxkb_context *ctx, const char *path,
      enum rxkb_popularity popularity)
{
    bool success = false;
    xmlDoc *doc = NULL;
    xmlNode *root = NULL;

    if (!check_eaccess(path, R_OK))
        return false;

    LIBXML_TEST_VERSION

    xmlParserCtxtPtr xmlCtxt = xmlNewParserCtxt();
    if (!xmlCtxt)
        return false;

#ifdef XML_PARSE_NO_XXE
#define _XML_OPTIONS (XML_PARSE_NONET | XML_PARSE_NOENT | XML_PARSE_NO_XXE)
#else
#define _XML_OPTIONS (XML_PARSE_NONET)
#endif
    xmlCtxtUseOptions(xmlCtxt, _XML_OPTIONS);

#ifdef HAVE_XML_CTXT_SET_ERRORHANDLER
    /* Prefer contextual handler whenever possible. It takes precedence over
     * the global generic handler. */
    xmlCtxtSetErrorHandler(xmlCtxt, xml_structured_error_func, ctx);
#endif
#ifndef HAVE_XML_CTXT_PARSE_DTD
    /* This is needed for the DTD validation */
    xmlSetGenericErrorFunc(ctx, xml_error_func);
#endif

    doc = xmlCtxtReadFile(xmlCtxt, path, NULL, 0);
    if (!doc)
        goto parse_error;

    if (!validate(ctx, doc)) {
        log_err(ctx, XKB_LOG_MESSAGE_NO_ID,
                "XML error: failed to validate document at %s\n", path);
        goto validate_error;
    }

    root = xmlDocGetRootElement(doc);
    parse_rules_xml(ctx, root, popularity);

    success = true;
validate_error:
    xmlFreeDoc(doc);
parse_error:

#ifndef HAVE_XML_CTXT_PARSE_DTD
    /*
     * Reset the default libxml2 error handler to default, because this handler
     * is global and may be used on an invalid rxkb_context, e.g. *after* the
     * following sequence:
     *
     *     rxkb_context_new();
     *     rxkb_context_parse();
     *     rxkb_context_unref();
     */
    xmlSetGenericErrorFunc(NULL, NULL);
#endif
#ifdef HAVE_XML_CTXT_SET_ERRORHANDLER
    xmlCtxtSetErrorHandler(xmlCtxt, NULL, NULL);
#endif
    xmlFreeParserCtxt(xmlCtxt);

    return success;
}
