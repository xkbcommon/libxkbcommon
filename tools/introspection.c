/*
 * Copyright © 2025 Pierre Le Marre <dev@wismill.eu>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "xkbcommon/xkbcommon.h"

#include "ast.h"
#include "tools-common.h"
#include "src/darray.h"
#include "src/utils.h"
#include "src/utils-paths.h"
#include "src/keymap-formats.h"
#include "src/xkbcomp/keymap-file-iterator.h"

/*
 * The output is meant to be valid YAML; however we do not enforce it
 * because we expect the file and section names to be valid text values.
 */

enum input_source {
    INPUT_SOURCE_AUTO = 0,
    INPUT_SOURCE_STDIN,
    INPUT_SOURCE_PATH
};

enum output_format {
    OUTPUT_FORMAT_YAML = 0,
    OUTPUT_FORMAT_DOT,
    OUTPUT_FORMAT_RDF_TURTLE,
    OUTPUT_FORMAT_RESOLVED_PATH,
};

enum output_options {
    OUTPUT_NO_OPTIONS = 0,
    OUTPUT_YAML_SHORT_LABELS = (1u << 0),
};

static const unsigned int indent_size = 2;

static enum xkb_file_type
xkb_parse_file_type(const char *raw)
{
    static struct {
        enum xkb_file_type type;
        const char *name;
    } xkb_file_type_strings[_FILE_TYPE_NUM_ENTRIES] = {
        {FILE_TYPE_KEYCODES, "keycodes"},
        {FILE_TYPE_TYPES, "types"},
        {FILE_TYPE_COMPAT, "compat"},
        {FILE_TYPE_SYMBOLS, "symbols"},
        {FILE_TYPE_GEOMETRY, "geometry"},
        {FILE_TYPE_RULES, "rules"},
    };

    for (size_t t = 0; t < ARRAY_SIZE(xkb_file_type_strings); t++) {
        if (streq_not_null(raw, xkb_file_type_strings[t].name))
            return xkb_file_type_strings[t].type;
    }

    return FILE_TYPE_INVALID;
}

/**
 * Try to get the relative path of a file in a XKB hierarchy
 *
 * This is a fragile! We could improve it by using the context include paths,
 * but the analyzed path may be in a XKB hierarchy but not in the include paths.
 */
static const char*
xkb_relative_path(const char *path)
{
    static const char * xkb_file_type_include_dirs[] = {
        "keycodes/",
        "types/",
        "compat/",
        "symbols/",
        "geometry/",
        // TODO: "keymap/",
        // TODO: "rules/",
    };
    for (size_t t = 0; t < ARRAY_SIZE(xkb_file_type_include_dirs); t++) {
        const char * const rel = strstr(path, xkb_file_type_include_dirs[t]);
        if (rel != NULL) {
            return rel + strlen(xkb_file_type_include_dirs[t]);
        }
    }
    return NULL;
}

static bool
is_stdin_path(const char *path)
{
    return isempty(path) || strcmp(path, "-") == 0;
}

static bool
print_included_section(struct xkb_context *ctx,
                       enum xkb_file_iterator_flags iterator_flags,
                       enum output_format output_format,
                       enum output_options output_options,
                       enum xkb_keymap_format keymap_input_format,
                       const char *path, const char *map,
                       unsigned int include_depth, unsigned int ident_depth,
                       const char *parent);

/*******************************************************************************
 * YAML output
 ******************************************************************************/

static void
print_yaml_flags(unsigned int indent, enum xkb_map_flags flags)
{
    printf("%*sflags: [", indent, "");
    bool first = true;
    const char *flag_name = NULL;
    unsigned int index = 0;
    while ((flag_name = xkb_map_flags_string_iter(&index, flags))) {
        if (first)
            first = false;
        else
            printf(", ");
        printf("%s", flag_name);
    }
    printf("]\n");
}

static bool
print_yaml_included_sections(struct xkb_context *ctx,
                             enum xkb_file_iterator_flags iterator_flags,
                             enum output_options output_options,
                             enum xkb_keymap_format format,
                             const struct xkb_file_section *section,
                             unsigned int include_depth, unsigned int ident_depth,
                             bool recursive)
{
    if (darray_size(section->includes)) {
        bool ok = true;
        const unsigned int indent1 = ident_depth * indent_size;

        printf("%*sincludes:\n", indent1, "");

        struct xkb_file_include_group *group;
        darray_foreach(group, section->include_groups) {
            assert(group->end < darray_size(section->includes));
            unsigned int indent2 = indent1;
            unsigned int ident_depth2 = ident_depth;

            if (group->start != group->end) {
                /* Multiple files included in a single statement */
                const enum merge_mode merge_mode =
                    darray_item(section->includes, group->start).merge;
                printf("%*s- merge mode: %s\n", indent1, "",
                    xkb_merge_mode_name(merge_mode));
                printf("%*s  files:\n", indent1, "");
                ident_depth2 += 1;
                indent2 = ident_depth2 * indent_size;
            }

            for (darray_size_t i = group->start; i <= group->end; i++) {
                const struct xkb_file_include * const inc =
                    &darray_item(section->includes, i);
                printf("%*s- merge mode: %s\n", indent2, "",
                       xkb_merge_mode_name(inc->merge));
                printf("%*s  file: \"%s\"\n", indent2, "",
                       xkb_file_section_get_string(section, inc->file));
                printf("%*s  section: \"%s\"\n", indent2, "",
                       xkb_file_section_get_string(section, inc->section));
                printf("%*s  explicit section: %s\n", indent2, "",
                       (inc->explicit_section ? "true" : "false"));
                printf("%*s  path: \"%s\"\n", indent2, "",
                       xkb_file_section_get_string(section, inc->path));
                const char * const modifier =
                    xkb_file_section_get_string(section, inc->modifier);
                if (!isempty(modifier))
                    printf("%*s  modifier: \"%s\"\n", indent2, "", modifier);
                if (inc->valid) {
                    print_yaml_flags(indent2 + indent_size, inc->flags);
                } else {
                    printf("%*s  valid: false\n", indent2, "");
                }

                if (recursive && inc->valid) {
                    ok = print_included_section(
                        ctx, iterator_flags, OUTPUT_FORMAT_YAML, output_options,
                        format, xkb_file_section_get_string(section, inc->path),
                        xkb_file_section_get_string(section, inc->section),
                        include_depth + 1, ident_depth2 + 1, NULL
                    );
                    if (!ok)
                        break;
                }

            }
        }
        return ok;
    } else {
        return true;
    }
}

static bool
print_yaml(struct xkb_context *ctx,
           enum xkb_file_iterator_flags iterator_flags,
           enum output_options output_options,
           enum xkb_keymap_format keymap_format,
           int path_index, const char *path, bool recursive,
           struct xkb_file_iterator *iter)
{
    bool ok = true;
    const struct xkb_file_section *section;
    if (path_index > 0) {
        /* New YAML document */
        printf("---\n");
    }
    printf("path: \"%s\"\n", (is_stdin_path(path) ? "stdin" : path));
    printf("sections:");
    bool has_sections = false;
    while ((ok = xkb_file_iterator_next(iter, &section)) && section) {
        has_sections = true;
        printf("\n- type: %s", xkb_file_type_name(section->file_type));
        printf("\n  section: \"%s\"\n",
                xkb_file_section_get_string(section, section->name));
        print_yaml_flags(indent_size, section->flags);
        print_yaml_included_sections(ctx, iterator_flags,
                                     output_options, keymap_format,
                                     section, 0, 1, recursive);
    }
    if (!has_sections)
        printf(" []\n");
    return ok;
}

/*******************************************************************************
 * Resolved path output
 ******************************************************************************/

static bool
print_resolved_path(struct xkb_context *ctx,
                    enum xkb_file_iterator_flags iterator_flags,
                    enum output_options output_options,
                    enum xkb_keymap_format keymap_format,
                    int path_index, const char *path, bool recursive,
                    struct xkb_file_iterator *iter)
{
    bool ok = true;
    const struct xkb_file_section *section;
    if (path_index > 0) {
        /* New YAML document */
        printf("---\n");
    }
    printf("path: \"%s\"\n", (is_stdin_path(path) ? "stdin" : path));
    while ((ok = xkb_file_iterator_next(iter, &section)) && section) {
        printf("type: %s\n", xkb_file_type_name(section->file_type));
        printf("section: \"%s\"\n",
                xkb_file_section_get_string(section, section->name));
        print_yaml_flags(0, section->flags);
    }
    return ok;
}

/*******************************************************************************
 * DOT output
 ******************************************************************************/

static bool
print_dot_node(enum output_options output_options, const char *parent_node,
               darray_char *node, darray_char *label,
               const struct xkb_file_section *section,
               const struct xkb_file_include *inc)
{
    /* Cannot print invalid includes */
    if (!inc->valid)
        return true;

    /* Node identifier */
    darray_size(*node) = 0;
    darray_append_string(
        *node,
        xkb_file_section_get_string(section, inc->path)
    );
    if (inc->section) {
        darray_append_lit(*node, "(");
        darray_append_string(
            *node,
            xkb_file_section_get_string(section, inc->section)
        );
        darray_append_lit(*node, ")");
    }

    /* Node label */
    darray_size(*label) = 0;
    darray_append_lit(*label, "<B>");
    darray_append_string(
        *label,
        ((output_options & OUTPUT_YAML_SHORT_LABELS)
            ? xkb_file_section_get_string(section, inc->file)
            : xkb_file_section_get_string(section, inc->path))
    );
    darray_append_lit(*label, "</B>");
    if (inc->section) {
        darray_append_lit(*label, "(");
        darray_append_string(
            *label,
            xkb_file_section_get_string(section, inc->section)
        );
        darray_append_lit(*label, ")");
    }
    printf("\t\"%s\" [label=<%s>];\n",
           darray_items(*node), darray_items(*label));
    printf("\t\"%s\" -> \"%s\";\n",
           parent_node, darray_items(*node));
    return true;
}

static bool
print_dot_included_sections(struct xkb_context *ctx,
                            enum xkb_file_iterator_flags iterator_flags,
                            enum output_options output_options,
                            enum xkb_keymap_format format,
                            const struct xkb_file_section *section,
                            unsigned int include_depth,
                            bool recursive, const char *parent)
{
    if (darray_size(section->includes)) {
        bool ok = true;

        darray_char parent2_node = darray_new();
        darray_char parent2_label = darray_new();
        struct xkb_file_include *inc;
        darray_foreach(inc, section->includes) {
                assert(inc->valid);
                ok = print_dot_node(output_options, parent,
                                    &parent2_node, &parent2_label,
                                    section, inc);
                if (!ok)
                    break;

                if (recursive && inc->valid) {
                    ok = print_included_section(
                        ctx, iterator_flags, OUTPUT_FORMAT_DOT, output_options,
                        format, xkb_file_section_get_string(section, inc->path),
                        xkb_file_section_get_string(section, inc->section),
                        include_depth + 1, 0, darray_items(parent2_node)
                    );
                    if (!ok)
                        break;
                }
        }
        darray_free(parent2_node);
        darray_free(parent2_label);
        return ok;
    } else {
        return true;
    }
}

static bool
print_dot(struct xkb_context *ctx,
          enum xkb_file_iterator_flags iterator_flags,
          enum output_options output_options,
          enum xkb_keymap_format format,
          int path_index, const char *path, bool recursive,
          struct xkb_file_iterator *iter)
{
    char root[PATH_MAX] = "";
    if (is_stdin_path(path)) {
        if (unlikely(!strcpy_safe(root, sizeof(root), "stdin"))) {
            return false;
        }
    } else {
#ifdef HAVE_REAL_PATH
        realpath(path, root);
#else
        if (unlikely(!strcpy_safe(root, sizeof(root), path)))
            return false;
#endif
    }
    const char * root_file = xkb_relative_path(root);

    bool ok = true;
    const struct xkb_file_section *section;
    darray_char root_node = darray_new();
    darray_char parent_node = darray_new();
    darray_char parent_label = darray_new();
    unsigned int idx = 0;
    bool is_composite_file = false;

    while ((ok = xkb_file_iterator_next(iter, &section)) && section) {
        if (idx == 0) {
            /* Check if this is a composite file */
            if (section->file_type == FILE_TYPE_KEYMAP) {
                is_composite_file = true;
            } else {
                /* Root node set globally */
                darray_append_string(root_node, root);
                printf("\t\"%s\" [label=<<B>%s</B>>, style=\"rounded,filled\"];\n",
                       darray_items(root_node), root);
                if (path_index == 0) {
                    // FIXME: handle multiple roots using subgraphs?
                    printf("root=\"%s\";\n", darray_items(root_node));
                }
            }
        }

        /* Node identifier */
        darray_size(parent_node) = 0;
        /* Prefix with section type to avoid ID clashes */
        if (is_composite_file) {
            darray_append_string(parent_node,
                                 xkb_file_type_name(section->file_type));
            darray_append_lit(parent_node, ":");
        }
        /* Append full path & section */
        darray_append_string(parent_node, root);
        if (section->name) {
            darray_append_lit(parent_node, "(");
            darray_append_string(
                parent_node,
                xkb_file_section_get_string(section, section->name)
            );
            darray_append_lit(parent_node, ")");
        }

        if (section->file_type == FILE_TYPE_KEYMAP) {
            /* Root node set for each keymap */
            assert(is_composite_file);
            darray_copy(root_node, parent_node);
            darray_append(root_node, '\0');
        }

        /* Node label */
        darray_size(parent_label) = 0;
        if (!is_composite_file || section->file_type == FILE_TYPE_KEYMAP) {
            /* Display file only for top-level components */
            darray_append_lit(parent_label, "<B>");
            if (root_file && (output_options & OUTPUT_YAML_SHORT_LABELS)) {
                darray_append_string(parent_label, root_file);
            } else {
                darray_append_string(parent_label, root);
            }
            darray_append_lit(parent_label, "</B>");
        }
        if (section->name) {
            darray_append_lit(parent_label, "(");
            darray_append_string(
                parent_label,
                xkb_file_section_get_string(section, section->name)
            );
            darray_append_lit(parent_label, ")");
        } else {
            darray_append_string(
                parent_label,
                (is_composite_file ? "(unnamed)" : "(-)")
            );
        }

        if (is_composite_file) {
            if (section->file_type == FILE_TYPE_KEYMAP && idx != 0) {
                /* Close the previous keymap subgraph */
                printf("}\n");
            }
            /* Draw each component in a subgraph */
            printf("\nsubgraph \"cluster::%s\" {\n", darray_items(parent_node));
            printf("\tlabel=<<B>%s</B>>;\n",
                   xkb_file_type_name(section->file_type));
        }

        /* Create edge */
        if (section->file_type == FILE_TYPE_KEYMAP) {
            /*
             * Avoid keymap node being included in component clusters by
             * creating its own cluster.
             */
            printf("\nsubgraph \"cluster::root::%s\" {\n",
                   darray_items(parent_node));
            printf("\tstyle=invis;\n");
            printf("\t\"%s\" [label=<%s>, style=\"rounded,filled\"];\n",
                   darray_items(parent_node), darray_items(parent_label));
            printf("}\n");
            printf("root=\"%s\";\n\n", darray_items(parent_node));
        } else {
            printf("\t\"%s\" [label=<%s>];\n",
                   darray_items(parent_node), darray_items(parent_label));
        }

        /* Link to root */
        if (section->file_type != FILE_TYPE_KEYMAP) {
            printf("\t\"%s\" -> \"%s\" [arrowhead=empty];\n",
                   darray_items(root_node), darray_items(parent_node));
        }
        print_dot_included_sections(ctx, iterator_flags,
                                    output_options, format,
                                    section, 0, recursive, darray_items(parent_node));
        if (is_composite_file && section->file_type != FILE_TYPE_KEYMAP) {
            printf("}\n");
        }
        idx++;
    }

    if (is_composite_file) {
        /* Close the keymap subgraph */
        printf("}\n");
    }

    darray_free(root_node);
    darray_free(parent_node);
    darray_free(parent_label);

    return ok;
}

/*******************************************************************************
 * Output format: RDF Turtle
 ******************************************************************************/

static void
mk_rdf_path_id(darray_char *node, const char *path)
{
    darray_append_lit(*node, "file:");
    darray_append_string(*node, (is_stdin_path(path) ? "stdin" : path));
}

static void
mk_rdf_section_id(darray_char *node, const char *path, const char *section)
{
    mk_rdf_path_id(node, path);
    darray_append_lit(*node, "#section=");
    darray_append_string(*node, section);
}

static void
print_rdf_flags(enum xkb_map_flags flags)
{
    bool first = true;
    const char *flag_name = NULL;
    unsigned int index = 0;
    while ((flag_name = xkb_map_flags_string_iter(&index, flags))) {
        if (first)
            first = false;
        else
            printf(", ");
        printf("flags:%s", flag_name);
    }
}

static bool
print_rdf_sections(struct xkb_context *ctx,
                   enum xkb_file_iterator_flags iterator_flags,
                   enum output_options output_options,
                   enum xkb_keymap_format keymap_format,
                   const struct xkb_file_section *section,
                   unsigned int include_depth, bool recursive,
                   const char *path, const char *map,
                   unsigned int index, const char *node)
{
    printf("<%s>\n", node);
    printf("\txkb:path\t\"%s\" ;\n", (is_stdin_path(path) ? "stdin" : path));
    printf("\txkb:section\t\"%s\" ;\n", map);
    printf("\trdf:type\txkb:%s ;\n", xkb_file_type_name(section->file_type));
    if (section->flags) {
        printf("\txkb:flag\t");
        print_rdf_flags(section->flags);
        printf(" ;\n");
    }
    printf("\txkb:section-index\t%u", index);
    if (darray_size(section->includes)) {
        bool ok = true;

        darray_char include_target = darray_new();

        struct xkb_file_include_group *group;
        darray_size_t g = 0;
        printf(" ;\n\txkb:includes\t(");
        darray_enumerate(g, group, section->include_groups) {
            printf("\n\t\t(");
            for (darray_size_t f = group->start; f <= group->end; f++) {
                const struct xkb_file_include * const inc =
                    &darray_item(section->includes, f);
                assert(inc->valid);

                darray_size(include_target) = 0;
                mk_rdf_section_id(
                    &include_target,
                    xkb_file_section_get_string(section, inc->path),
                    xkb_file_section_get_string(section, inc->section)
                );

                printf("\n\t\t\t[\n");

                printf("\t\t\t\txkb:merge-mode\txkb:%s ;\n",
                       xkb_merge_mode_name(inc->merge));
                printf("\t\t\t\txkb:file\t\"%s\" ;\n",
                       xkb_file_section_get_string(section, inc->file));
                printf("\t\t\t\txkb:section\t\"%s\" ;\n",
                       xkb_file_section_get_string(section, inc->section));
                printf("\t\t\t\txkb:path\t\"%s\"",
                       xkb_file_section_get_string(section, inc->path));
                if (inc->flags) {
                    printf(" ;\n\t\t\t\txkb:flag\t");
                    print_rdf_flags(inc->flags);
                }
                printf(" ;\n\t\t\t\txkb:includes\t<%s>\n",
                       darray_items(include_target));

                printf("\t\t\t]");
            }
            printf("\n\t\t)");
        }
        printf("\n\t) .\n\n");

        if (recursive) {
            struct xkb_file_include * inc;
            darray_foreach(inc, section->includes) {
                if (!inc->valid)
                    continue;

                darray_size(include_target) = 0;
                mk_rdf_section_id(
                    &include_target,
                    xkb_file_section_get_string(section, inc->path),
                    xkb_file_section_get_string(section, inc->section)
                );
                ok = print_included_section(
                    ctx, iterator_flags, OUTPUT_FORMAT_RDF_TURTLE,
                    output_options, keymap_format,
                    xkb_file_section_get_string(section, inc->path),
                    xkb_file_section_get_string(section, inc->section),
                    include_depth + 1, 0, darray_items(include_target)
                );
                if (!ok)
                    break;
            }
        }

        darray_free(include_target);
        return ok;
    } else {
        printf(" .\n\n");
        return true;
    }
}

static bool
print_rdf(struct xkb_context *ctx,
          enum xkb_file_iterator_flags iterator_flags,
          enum output_options output_options,
          enum xkb_keymap_format keymap_format,
          int path_index, const char *path, const char *map, bool recursive,
          struct xkb_file_iterator *iter)
{
    bool ok = true;
    const struct xkb_file_section *section;
    bool is_composite_file = false;
    darray_char keymap = darray_new();
    darray_char node = darray_new();

    /* Save some CLI arguments, so that the graph is easier to query */
    mk_rdf_path_id(&node, path);
    printf("<%s>\n", darray_items(node));
    printf("\trdf:type\txkb:Introspection ;\n");
    printf("\txkb:path\t\"%s\" ;\n",
           (is_stdin_path(path) ? "stdin" : path));
    printf("\txkb:section\t\"%s\" .\n\n",
           (isempty(map) ? "" : map));

    unsigned int index = 0;
    while ((ok = xkb_file_iterator_next(iter, &section)) && section) {
        if (section->file_type == FILE_TYPE_KEYMAP)
            is_composite_file = true;

        darray_size(node) = 0;
        mk_rdf_section_id(&node, path,
                          xkb_file_section_get_string(section, section->name));

        if (is_composite_file) {
            /* Disambiguate components */
            darray_append_lit(node, ":type=");
            darray_append_string(node,
                                 xkb_file_type_name(section->file_type));
            if (section->file_type == FILE_TYPE_KEYMAP) {
                /* Backup keymap node */
                darray_size(keymap) = 0;
                darray_copy(keymap, node);
                darray_append(keymap, '\0');
            } else {
                /* Link component to parent keymap */
                printf("<%s>\txkb:includes\t<%s> .\n\n",
                       darray_items(keymap), darray_items(node));
            }
        }

        print_rdf_sections(ctx, iterator_flags,
                           output_options, keymap_format,
                           section, 0, recursive, path,
                           xkb_file_section_get_string(section, section->name),
                           index, darray_items(node));
        index++;
    }
    darray_free(keymap);
    darray_free(node);
    return ok;
}

/*******************************************************************************
 * Common output
 ******************************************************************************/

static bool
print_included_section(struct xkb_context *ctx,
                       enum xkb_file_iterator_flags iterator_flags,
                       enum output_format output_format,
                       enum output_options output_options,
                       enum xkb_keymap_format format,
                       const char *path, const char *map,
                       unsigned int include_depth, unsigned int ident_depth,
                       const char *parent)
{
    if (isempty(map))
        map = NULL;

    struct xkb_file_section section = {0};
    xkb_file_section_init(&section);
    bool ok = xkb_file_section_parse(ctx, iterator_flags, format,
                                     XKB_KEYMAP_COMPILE_NO_FLAGS,
                                     include_depth, path, map, &section);

    if (!ok)
        goto out;

    switch (output_format) {
    case OUTPUT_FORMAT_YAML:
        ok = print_yaml_included_sections(ctx, iterator_flags,
                                          output_options, format, &section,
                                          include_depth, ident_depth, true);
        break;
    case OUTPUT_FORMAT_DOT:
        ok = print_dot_included_sections(ctx, iterator_flags,
                                         output_options, format, &section,
                                         include_depth, true, parent);
        break;
    case OUTPUT_FORMAT_RDF_TURTLE:
        ok = print_rdf_sections(ctx, iterator_flags,
                                output_options, format, &section,
                                include_depth, true, path, map, 0, parent);
        break;
    default:
        /* unreachable */
        assert(!"unreachable");
    }

out:
    xkb_file_section_free(&section);
    return ok;
}

static void
print_sections_header(enum output_format output_format)
{
    switch (output_format) {
        case OUTPUT_FORMAT_YAML:
        case OUTPUT_FORMAT_RESOLVED_PATH:
            break;
        case OUTPUT_FORMAT_DOT:
            printf("digraph {\n");
            printf("node [shape=box, style=rounded];\n");
            printf("overlap=false;\n");
            printf("concentrate=true;\n");
            printf("rankdir=\"LR\";\n");
            printf("fontsize=\"20pt\";\n");
            break;
        case OUTPUT_FORMAT_RDF_TURTLE:
            printf("@prefix\trdf:\t<http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n");
            printf("@prefix\txkb:\t<xkb:> .\n");
            printf("@prefix\tflags:\t<xkb:flags/> .\n\n");
            break;
        default:
            assert(!"unreachable");
    }
}

static void
print_sections_footer(enum output_format output_format)
{
    switch (output_format) {
        case OUTPUT_FORMAT_YAML:
        case OUTPUT_FORMAT_RESOLVED_PATH:
        case OUTPUT_FORMAT_RDF_TURTLE:
            break;
        case OUTPUT_FORMAT_DOT:
            /* Close graph */
            printf("}\n");
            break;
        default:
            assert(!"unreachable");
    }
}

static int
print_sections(struct xkb_context *ctx,
               enum xkb_file_iterator_flags iterator_flags,
               enum output_format output_format,
               enum output_options output_options,
               enum xkb_keymap_format keymap_format, enum input_source source,
               enum xkb_file_type file_type,
               int path_index, const char *path, const char *map,
               bool recursive)
{
    if (source == INPUT_SOURCE_PATH && is_stdin_path(path)) {
        source = INPUT_SOURCE_STDIN;
        path = NULL;
    }

    int ret = EXIT_FAILURE;
    FILE *file = NULL;
    char resolved_path[PATH_MAX] = {0};
    char resolved_section[1024] = {0};
    if (source == INPUT_SOURCE_PATH && !is_stdin_path(path)) {
        /* Read from regular file */
        const bool absolute = is_absolute_path(path);
        if (output_format != OUTPUT_FORMAT_RESOLVED_PATH &&
            (absolute || file_type > FILE_TYPE_KEYMAP)) {
            /*
             * Absolute path or undefined file type: open directly.
             * Relative paths are resolved using the working directory as usual.
             */
            file = fopen(path, "rb");
            if (!file) {
                fprintf(stderr, "ERROR: Failed to open keymap file \"%s\": %s\n",
                        path, strerror(errno));
                return ret;
            }
            if (unlikely(!strcpy_safe(resolved_path, sizeof(resolved_path), path))) {
                goto map_error;
            }
        } else {
            /*
             * Relative path: interpret as a file in a XKB tree of the given
             * file type
             */
            // TODO: this is currently a bit silly, since we are parsing the
            //       file here and then again in the file iterator.
            file = xkb_resolve_file(ctx, file_type,
                                    path, (isempty(map) ? NULL : map),
                                    resolved_path, sizeof(resolved_path),
                                    resolved_section, sizeof(resolved_section));
            if (!file) {
                fprintf(stderr,
                        "ERROR: File not found in XKB paths: %s (section: %s)\n",
                        path, (isempty(map)) ? "(none)" : map);
                return ret;
            }

            if (output_format == OUTPUT_FORMAT_RESOLVED_PATH &&
                !isempty(resolved_section)) {
                assert(isempty(map) || streq(map, resolved_section));
                map = resolved_section;
            }
        }
    } else {
        /* Read from stdin */
        file = tools_read_stdin();
    }

    char *string;
    size_t string_len;
    if (!map_file(file, &string, &string_len)) {
        fprintf(stderr, "ERROR: cannot map file\n");
        goto map_error;
    }

    // FIXME: check that file_type is respected in the iterator

    struct xkb_file_iterator * iter = xkb_file_iterator_new_from_buffer(
        ctx, iterator_flags, keymap_format, XKB_KEYMAP_COMPILE_NO_FLAGS,
        (is_stdin_path(path) ? "(stdin)" : path), map, file_type,
        string, string_len
    );

    if (!iter) {
        fprintf(stderr, "ERROR: cannot create iterator\n");
        goto iter_error;
    }

    bool ok = true;
    switch (output_format) {
    case OUTPUT_FORMAT_YAML:
        ok = print_yaml(ctx, iterator_flags, output_options,
                        keymap_format, path_index, path, recursive, iter);
        break;
    case OUTPUT_FORMAT_RESOLVED_PATH:
        ok = print_resolved_path(ctx, iterator_flags, output_options,
                                 keymap_format, path_index, resolved_path,
                                 recursive, iter);
        break;
    case OUTPUT_FORMAT_DOT:
        ok = print_dot(ctx, iterator_flags, output_options,
                       keymap_format, path_index, path, recursive, iter);
        break;
    case OUTPUT_FORMAT_RDF_TURTLE:
        ok = print_rdf(ctx, iterator_flags, output_options,
                       keymap_format, path_index,
                       resolved_path, map, recursive, iter);
        break;
    default:
        /* unreachable */
        assert(!"unreachable");
    }

    xkb_file_iterator_free(iter);
    ret = (ok) ? EXIT_SUCCESS : EXIT_FAILURE;

iter_error:
    unmap_file(string, string_len);

map_error:
    if (file)
        fclose(file);

    return ret;
}

/*******************************************************************************
 * CLI handling
 ******************************************************************************/

static void
usage(FILE *file, const char *progname)
{
    fprintf(file,
           "Usage: %s [OPTIONS] [FILES]\n"
           "\n"
           "Introspect a XKB file\n"
           "\n"
           "General options:\n"
           " --help\n"
           "    Print this help and exit\n"
           " --verbose\n"
           "    Enable verbose debugging output\n"
           "\n"
           "Input options:\n"
           " --include\n"
           "    Add the given path to the include path list. This option is\n"
           "    order-dependent, include paths given first are searched first.\n"
           "    If an include path is given, the default include path list is\n"
           "    not used. Use --include-defaults to add the default include\n"
           "    paths\n"
           " --include-defaults\n"
           "    Add the default set of include directories.\n"
           "    This option is order-dependent, include paths given first\n"
           "    are searched first.\n"
           " --format <format>\n"
           "    The keymap format to use for parsing (default: '%d')\n"
           " --section <name>\n"
           "    The name of a specific section to parse\n"
           " --type <type>\n"
           "    The type of XKB file (KcCGST): keycodes, compatibility, geometry, symbols, types.\n"
           " --recursive\n"
           "    Recursive analysis of the included sections\n"
           " --include-failures\n"
           "    Do not stop on include failures but collect them (YAML only)\n"
           " --resolve\n"
           "    Output resolved paths (YAML only)\n"
           " --yaml\n"
           "    Output YAML\n"
           " --dot\n"
           "    Output a DOT graph\n"
           " --rdf\n"
           "    Output a RDF graph in the Turtle format\n"
           " --long-labels\n"
           "    Output long nodes labels\n"
           "\n"
           "This program can process multiple files. Use e.g.:\n"
           "  %s \\\n"
           "    $(find \"" DFLT_XKB_CONFIG_ROOT "/symbols\" -type f -not -name README | xargs)\n"
           "to process all symbols files."
           "\n",
           progname, DEFAULT_INPUT_KEYMAP_FORMAT, progname);
}

#define DEFAULT_INCLUDE_PATH_PLACEHOLDER "__defaults__"
static const char *includes[64] = { 0 };
static size_t num_includes = 0;

static bool
parse_options(int argc, char **argv, bool *verbose,
              enum input_source *input_source,
              enum xkb_keymap_format *keymap_input_format,
              enum xkb_file_iterator_flags *iterator_flags,
              int *paths_start, char **section,
              enum xkb_file_type *section_type,
              bool *recursive,
              enum output_format *output_format,
              enum output_options *output_options)
{
    enum input_source input_format = INPUT_SOURCE_AUTO;
    enum options {
        /* General */
        OPT_VERBOSE,
        /* Input */
        OPT_INCLUDE,
        OPT_INCLUDE_DEFAULTS,
        OPT_INCLUDE_FAILURES,
        OPT_KEYMAP_FORMAT,
        OPT_SECTION_NAME,
        OPT_SECTION_TYPE,
        OPT_RECURSIVE,
        OPT_OUTPUT_RESOLVED_PATH,
        OPT_OUTPUT_YAML,
        OPT_OUTPUT_DOT,
        OPT_OUTPUT_RDF,
        OPT_LONG_LABELS,
    };
    static struct option opts[] = {
        /*
         * General
         */
        {"help",             no_argument,            0, 'h'},
        {"verbose",          no_argument,            0, OPT_VERBOSE},
        /*
         * Input
         */
        {"include",          required_argument,      0, OPT_INCLUDE},
        {"include-defaults", no_argument,            0, OPT_INCLUDE_DEFAULTS},
        {"include-failures", no_argument,            0, OPT_INCLUDE_FAILURES},
        {"format",           required_argument,      0, OPT_KEYMAP_FORMAT},
        {"section",          required_argument,      0, OPT_SECTION_NAME},
        {"type",             required_argument,      0, OPT_SECTION_TYPE},
        {"recursive",        no_argument,            0, OPT_RECURSIVE},
        {"resolve",          no_argument,            0, OPT_OUTPUT_RESOLVED_PATH},
        {"yaml",             no_argument,            0, OPT_OUTPUT_YAML},
        {"dot",              no_argument,            0, OPT_OUTPUT_DOT},
        {"rdf",              no_argument,            0, OPT_OUTPUT_RDF},
        {"long-labels",      no_argument,            0, OPT_LONG_LABELS},
        {0, 0, 0, 0},
    };

    int option_index = 0;
    while (1) {
        option_index = 0;
        int c = getopt_long(argc, argv, "h", opts, &option_index);
        if (c == -1)
            break;

        switch (c) {
        /* General */
        case 'h':
            usage(stdout, argv[0]);
            exit(0);
        case OPT_VERBOSE:
            *verbose = true;
            break;
        /* Input */
        case OPT_INCLUDE:
            if (num_includes >= ARRAY_SIZE(includes))
                goto too_many_includes;
            includes[num_includes++] = optarg;
            break;
        case OPT_INCLUDE_DEFAULTS:
            if (num_includes >= ARRAY_SIZE(includes))
                goto too_many_includes;
            includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;
            break;
        case OPT_INCLUDE_FAILURES:
            *iterator_flags &= ~XKB_FILE_ITERATOR_FAIL_ON_INCLUDE_ERROR;
            break;
        case OPT_KEYMAP_FORMAT:
            *keymap_input_format = xkb_keymap_parse_format(optarg);
            if (!(*keymap_input_format)) {
                fprintf(stderr, "ERROR: invalid --format: \"%s\"\n", optarg);
                usage(stderr, argv[0]);
                exit(EXIT_INVALID_USAGE);
            }
            break;
        case OPT_SECTION_NAME:
            *section = optarg;
            break;
        case OPT_SECTION_TYPE:
            *section_type = xkb_parse_file_type(optarg);
            if (*section_type == FILE_TYPE_INVALID) {
                fprintf(stderr, "ERROR: invalid --type: \"%s\"\n", optarg);
                usage(stderr, argv[0]);
                exit(EXIT_INVALID_USAGE);
            }
            break;
        case OPT_RECURSIVE:
            *recursive = true;
            break;
        case OPT_OUTPUT_RESOLVED_PATH:
            *output_format = OUTPUT_FORMAT_RESOLVED_PATH;
            *iterator_flags |= XKB_FILE_ITERATOR_NO_INCLUDES;
            break;
        case OPT_OUTPUT_YAML:
            *output_format = OUTPUT_FORMAT_YAML;
            break;
        case OPT_OUTPUT_DOT:
            *output_format = OUTPUT_FORMAT_DOT;
            break;
        case OPT_OUTPUT_RDF:
            *output_format = OUTPUT_FORMAT_RDF_TURTLE;
            break;
        case OPT_LONG_LABELS:
            *output_options &= ~OUTPUT_YAML_SHORT_LABELS;
            break;
        default:
            goto invalid_usage;
        }
    }

    if (*output_format != OUTPUT_FORMAT_YAML &&
        !(*iterator_flags & XKB_FILE_ITERATOR_FAIL_ON_INCLUDE_ERROR)) {
        fprintf(stderr,
                "ERROR: --include-failures is only compatible with YAML output\n");
        goto invalid_usage;
    }

    if (optind < argc && !isempty(argv[optind])) {
        /* Some positional arguments left: use as a file input */
        if (input_format != INPUT_SOURCE_AUTO) {
            fprintf(stderr, "ERROR: Too many positional arguments\n");
            goto invalid_usage;
        }
        input_format = INPUT_SOURCE_PATH;
        *paths_start = optind;
    } else if (is_pipe_or_regular_file(STDIN_FILENO) &&
               input_format == INPUT_SOURCE_AUTO &&
               *output_format != OUTPUT_FORMAT_RESOLVED_PATH) {
        /* No positional argument: detect piping */
        input_format = INPUT_SOURCE_STDIN;
    }

    *input_source = input_format;
    return true;

too_many_includes:
    fprintf(stderr, "ERROR: too many includes (max: %zu)\n",
            ARRAY_SIZE(includes));

invalid_usage:
    usage(stderr, argv[0]);
    exit(EXIT_INVALID_USAGE);
}

int
main(int argc, char **argv)
{
    struct xkb_context *ctx;
    bool verbose = false;
    int paths_start = argc;
    char *map = NULL;
    enum xkb_file_type section_type = FILE_TYPE_INVALID;
    enum xkb_keymap_format keymap_input_format = DEFAULT_INPUT_KEYMAP_FORMAT;
    enum xkb_file_iterator_flags iterator_flags =
        XKB_FILE_ITERATOR_FAIL_ON_INCLUDE_ERROR;
    bool recursive = false;
    enum output_format output_format = OUTPUT_FORMAT_YAML;
    enum output_options output_options = OUTPUT_YAML_SHORT_LABELS;
    int rc = 1;

    setlocale(LC_ALL, "");

    if (argc < 1) {
        usage(stderr, argv[0]);
        return EXIT_INVALID_USAGE;
    }

    enum input_source input_source = INPUT_SOURCE_AUTO;
    if (!parse_options(argc, argv, &verbose, &input_source,
                       &keymap_input_format, &iterator_flags,
                       &paths_start, &map, &section_type, &recursive,
                       &output_format, &output_options))
        return EXIT_INVALID_USAGE;

    enum xkb_context_flags ctx_flags = XKB_CONTEXT_NO_DEFAULT_INCLUDES;

    ctx = xkb_context_new(ctx_flags);
    assert(ctx);

    if (verbose)
        tools_enable_verbose_logging(ctx);

    if (num_includes == 0)
        includes[num_includes++] = DEFAULT_INCLUDE_PATH_PLACEHOLDER;

    for (size_t i = 0; i < num_includes; i++) {
        const char *include = includes[i];
        if (strcmp(include, DEFAULT_INCLUDE_PATH_PLACEHOLDER) == 0)
            xkb_context_include_path_append_default(ctx);
        else
            xkb_context_include_path_append(ctx, include);
    }

    static char * no_paths[] = { NULL };
    if (input_source == INPUT_SOURCE_STDIN) {
        /* Dummy list of paths */
        argv = no_paths;
        argc = (int) ARRAY_SIZE(no_paths);
        paths_start = 0;
    }

    print_sections_header(output_format);

    for (int p = paths_start; p < argc; p++) {
        // fprintf(stderr,
        //         "------\nProcessing: %s\n",
        //         (is_stdin_path(argv[p]) ? "stdin" : argv[p]));
        rc = print_sections(ctx, iterator_flags, output_format, output_options,
                            keymap_input_format, input_source, section_type,
                            p - paths_start, argv[p], map, recursive);
        if (rc != EXIT_SUCCESS)
            break;
    }

    print_sections_footer(output_format);

    xkb_context_unref(ctx);

    return rc;
}
