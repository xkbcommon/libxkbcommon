/************************************************************
 Copyright (c) 1996 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/

#include <stdio.h>
#include <ctype.h>

#include "rules.h"
#include "path.h"

static bool
input_line_get(FILE *file, darray_char *line)
{
    int ch;
    bool end_of_file = false;
    bool space_pending;
    bool slash_pending;
    bool in_comment;

    while (!end_of_file && darray_empty(*line)) {
        space_pending = slash_pending = in_comment = false;

        while ((ch = getc(file)) != '\n' && ch != EOF) {
            if (ch == '\\') {
                ch = getc(file);

                if (ch == EOF)
                    break;

                if (ch == '\n') {
                    in_comment = false;
                    ch = ' ';
                }
            }

            if (in_comment)
                continue;

            if (ch == '/') {
                if (slash_pending) {
                    in_comment = true;
                    slash_pending = false;
                }
                else {
                    slash_pending = true;
                }

                continue;
            }

            if (slash_pending) {
                if (space_pending) {
                    darray_append(*line, ' ');
                    space_pending = false;
                }

                darray_append(*line, '/');
                slash_pending = false;
            }

            if (isspace(ch)) {
                while (isspace(ch) && ch != '\n' && ch != EOF)
                    ch = getc(file);

                if (ch == EOF)
                    break;

                if (ch != '\n' && !darray_empty(*line))
                    space_pending = true;

                ungetc(ch, file);
            }
            else {
                if (space_pending) {
                    darray_append(*line, ' ');
                    space_pending = false;
                }

                if (ch == '!') {
                    if (!darray_empty(*line)) {
                        WARN("The '!' is legal only at start of line\n");
                        ACTION("Line containing '!' ignored\n");
                        darray_resize(*line, 0);
                        break;
                    }
                }

                darray_append(*line, ch);
            }
        }

        if (ch == EOF)
            end_of_file = true;
    }

    if (darray_empty(*line) && end_of_file)
        return false;

    darray_append(*line, '\0');
    return true;
}

/***====================================================================***/

enum {
    /* "Parts" - the MLVO which rules file maps to components. */
    MODEL = 0,
    LAYOUT,
    VARIANT,
    OPTION,

#define PART_MASK \
    ((1 << MODEL) | (1 << LAYOUT) | (1 << VARIANT) | (1 << OPTION))

    /* Components */
    KEYCODES,
    SYMBOLS,
    TYPES,
    COMPAT,
    GEOMETRY,

#define COMPONENT_MASK \
    ((1 << KEYCODES) | (1 << SYMBOLS) | (1 << TYPES) | (1 << COMPAT) | \
     (1 << GEOMETRY))

    MAX_WORDS
};

static const char *cname[] = {
    [MODEL] = "model",
    [LAYOUT] = "layout",
    [VARIANT] = "variant",
    [OPTION] = "option",

    [KEYCODES] = "keycodes",
    [SYMBOLS] = "symbols",
    [TYPES] = "types",
    [COMPAT] = "compat",
    [GEOMETRY] = "geometry",
};

struct multi_defs {
    const char *model;
    const char *layout[XkbNumKbdGroups + 1];
    const char *variant[XkbNumKbdGroups + 1];
    char *options;
};

struct mapping {
    /* Sequential id for the mappings. */
    int number;
    size_t num_maps;

    struct {
        int word;
        int index;
    } map[MAX_WORDS];
};

struct var_desc {
    char *name;
    char *desc;
};

struct group {
    int number;
    char *name;
    char *words;
};

enum rule_flag {
    RULE_FLAG_PENDING_MATCH = (1L << 1),
    RULE_FLAG_OPTION        = (1L << 2),
    RULE_FLAG_APPEND        = (1L << 3),
    RULE_FLAG_NORMAL        = (1L << 4),
};

struct rule {
    int number;

    char *model;
    char *layout;
    int layout_num;
    char *variant;
    int variant_num;
    char *option;

    /* yields */

    char *keycodes;
    char *symbols;
    char *types;
    char *compat;
    unsigned flags;
};

struct rules {
    darray(struct rule) rules;
    darray(struct group) groups;
};

/***====================================================================***/

/*
 * Resolve numeric index, such as "[4]" in layout[4]. Missing index
 * means zero.
 */
static char *
get_index(char *str, int *ndx)
{
    int empty = 0, consumed = 0, num;

    sscanf(str, "[%n%d]%n", &empty, &num, &consumed);
    if (consumed > 0) {
        *ndx = num;
        str += consumed;
    } else if (empty > 0) {
        *ndx = -1;
    } else {
        *ndx = 0;
    }

    return str;
}

/*
 * Match a mapping line which opens a rule, e.g:
 * ! model      layout[4]       variant[4]      =       symbols       geometry
 * Which will be followed by lines such as:
 *   *          ben             basic           =       +in(ben):4    nec(pc98)
 * So if the MLVO matches the LHS of some line, we'll get the components
 * on the RHS.
 * In this example, we will get for the second and fourth columns:
 * mapping->map[1] = {.word = LAYOUT, .index = 4}
 * mapping->map[3] = {.word = SYMBOLS, .index = 0}
 */
static void
match_mapping_line(darray_char *line, struct mapping *mapping)
{
    char *tok;
    char *str = darray_mem(*line, 1);
    unsigned present = 0, layout_ndx_present = 0, variant_ndx_present = 0;
    int i, tmp;
    size_t len;
    int ndx;
    char *strtok_buf;
    bool found;

    /*
     * Remember the last sequential mapping id (incremented if the match
     * is successful).
     */
    tmp = mapping->number;
    memset(mapping, 0, sizeof(*mapping));
    mapping->number = tmp;

    while ((tok = strtok_r(str, " ", &strtok_buf)) != NULL) {
        found = false;
        str = NULL;

        if (strcmp(tok, "=") == 0)
            continue;

        for (i = 0; i < MAX_WORDS; i++) {
            len = strlen(cname[i]);

            if (strncmp(cname[i], tok, len) == 0) {
                if (strlen(tok) > len) {
                    char *end = get_index(tok + len, &ndx);

                    if ((i != LAYOUT && i != VARIANT) ||
                        *end != '\0' || ndx == -1) {
                        WARN("Illegal %s index: %d\n", cname[i], ndx);
                        WARN("Can only index layout and variant\n");
                        break;
                    }

                    if (ndx < 1 || ndx > XkbNumKbdGroups) {
                        WARN("Illegal %s index: %d\n", cname[i], ndx);
                        WARN("Index must be in range 1..%d\n", XkbNumKbdGroups);
                        break;
                    }
                } else {
                    ndx = 0;
                }

                found = true;

                if (present & (1 << i)) {
                    if ((i == LAYOUT && layout_ndx_present & (1 << ndx)) ||
                        (i == VARIANT && variant_ndx_present & (1 << ndx))) {
                        WARN("Component \"%s\" listed twice\n", tok);
                        ACTION("Second definition ignored\n");
                        break;
                    }
                }

                present |= (1 << i);
                if (i == LAYOUT)
                    layout_ndx_present |= 1 << ndx;
                if (i == VARIANT)
                    variant_ndx_present |= 1 << ndx;

                mapping->map[mapping->num_maps].word = i;
                mapping->map[mapping->num_maps].index = ndx;
                mapping->num_maps++;
                break;
            }
        }

        if (!found) {
            WARN("Unknown component \"%s\"\n", tok);
            ACTION("ignored\n");
        }
    }

    if ((present & PART_MASK) == 0) {
        WARN("Mapping needs at least one MLVO part\n");
        ACTION("Illegal mapping ignored\n");
        mapping->num_maps = 0;
        return;
    }

    if ((present & COMPONENT_MASK) == 0) {
        WARN("Mapping needs at least one component\n");
        ACTION("Illegal mapping ignored\n");
        mapping->num_maps = 0;
        return;
    }

    mapping->number++;
}

/*
 * Match a line such as:
 * ! $pcmodels = pc101 pc102 pc104 pc105
 */
static bool
match_group_line(darray_char *line, struct group *group)
{
    int i;
    char *name = strchr(darray_mem(*line, 0), '$');
    char *words = strchr(name, ' ');

    if (!words)
        return false;

    *words++ = '\0';

    for (; *words; words++) {
        if (*words != '=' && *words != ' ')
            break;
    }

    if (*words == '\0')
        return false;

    group->name = strdup(name);
    group->words = strdup(words);

    words = group->words;
    for (i = 1; *words; words++) {
        if (*words == ' ') {
            *words++ = '\0';
            i++;
        }
    }
    group->number = i;

    return true;

}

/* Match lines following a mapping (see match_mapping_line comment). */
static bool
match_rule_line(darray_char *line, struct mapping *mapping,
                struct rule *rule)
{
    char *str, *tok;
    int nread, i;
    char *strtok_buf;
    bool append = false;
    const char *names[MAX_WORDS] = { NULL };

    if (mapping->num_maps == 0) {
        WARN("Must have a mapping before first line of data\n");
        ACTION("Illegal line of data ignored\n");
        return false;
    }

    str = darray_mem(*line, 0);

    for (nread = 0; (tok = strtok_r(str, " ", &strtok_buf)) != NULL; nread++) {
        str = NULL;

        if (strcmp(tok, "=") == 0) {
            nread--;
            continue;
        }

        if (nread > mapping->num_maps) {
            WARN("Too many words on a line\n");
            ACTION("Extra word \"%s\" ignored\n", tok);
            continue;
        }

        names[mapping->map[nread].word] = tok;
        if (*tok == '+' || *tok == '|')
            append = true;
    }

    if (nread < mapping->num_maps) {
        WARN("Too few words on a line: %s\n", darray_mem(*line, 0));
        ACTION("line ignored\n");
        return false;
    }

    rule->flags = 0;
    rule->number = mapping->number;

    if (names[OPTION])
        rule->flags |= RULE_FLAG_OPTION;
    else if (append)
        rule->flags |= RULE_FLAG_APPEND;
    else
        rule->flags |= RULE_FLAG_NORMAL;

    rule->model = uDupString(names[MODEL]);
    rule->layout = uDupString(names[LAYOUT]);
    rule->variant = uDupString(names[VARIANT]);
    rule->option = uDupString(names[OPTION]);

    rule->keycodes = uDupString(names[KEYCODES]);
    rule->symbols = uDupString(names[SYMBOLS]);
    rule->types = uDupString(names[TYPES]);
    rule->compat = uDupString(names[COMPAT]);

    rule->layout_num = rule->variant_num = 0;
    for (i = 0; i < nread; i++) {
        if (mapping->map[i].index) {
            if (mapping->map[i].word == LAYOUT)
                rule->layout_num = mapping->map[i].index;
            if (mapping->map[i].word == VARIANT)
                rule->variant_num = mapping->map[i].index;
        }
    }

    return true;
}

static bool
match_line(darray_char *line, struct mapping *mapping,
           struct rule *rule, struct group *group)
{
    if (darray_item(*line, 0) != '!')
        return match_rule_line(line, mapping, rule);

    if (darray_item(*line, 1) == '$' ||
        (darray_item(*line, 1) == ' ' && darray_item(*line, 2) == '$'))
        return match_group_line(line, group);

    match_mapping_line(line, mapping);
    return false;
}

static void
squeeze_spaces(char *p1)
{
   char *p2;

   for (p2 = p1; *p2; p2++) {
       *p1 = *p2;
       if (*p1 != ' ')
           p1++;
   }

   *p1 = '\0';
}

/*
 * Expand the layout and variant of the rule_names and remove extraneous
 * spaces. If there's one layout/variant, it is kept in
 * .layout[0]/.variant[0], else is kept in [1], [2] and so on, and [0]
 * remains empty. For example, this rule_names:
 *      .model  = "pc105",
 *      .layout = "us,il,ru,ca"
 *      .variant = ",,,multix"
 *      .options = "grp:alts_toggle,   ctrl:nocaps,  compose:rwin"
 * Is expanded into this multi_defs:
 *      .model = "pc105"
 *      .layout = {NULL, "us", "il", "ru", "ca"},
 *      .variant = {NULL, "", "", "", "multix"},
 *      .options = "grp:alts_toggle,ctrl:nocaps,compose:rwin"
 */
static bool
make_multi_defs(struct multi_defs *mdefs, const struct xkb_rule_names *mlvo)
{
    char *p;
    int i;

    memset(mdefs, 0, sizeof(*mdefs));

    if (mlvo->model) {
        mdefs->model = mlvo->model;
    }

    if (mlvo->options) {
        mdefs->options = strdup(mlvo->options);
        if (mdefs->options == NULL)
            return false;

        squeeze_spaces(mdefs->options);
    }

    if (mlvo->layout) {
        if (!strchr(mlvo->layout, ',')) {
            mdefs->layout[0] = mlvo->layout;
        }
        else {
            p = strdup(mlvo->layout);
            if (p == NULL)
                return false;

            squeeze_spaces(p);
            mdefs->layout[1] = p;

            for (i = 2; i <= XkbNumKbdGroups; i++) {
                if ((p = strchr(p, ','))) {
                    *p++ = '\0';
                    mdefs->layout[i] = p;
                }
                else {
                    break;
                }
            }

            if (p && (p = strchr(p, ',')))
                *p = '\0';
        }
    }

    if (mlvo->variant) {
        if (!strchr(mlvo->variant, ',')) {
            mdefs->variant[0] = mlvo->variant;
        }
        else {
            p = strdup(mlvo->variant);
            if (p == NULL)
                return false;

            squeeze_spaces(p);
            mdefs->variant[1] = p;

            for (i = 2; i <= XkbNumKbdGroups; i++) {
                if ((p = strchr(p, ','))) {
                    *p++ = '\0';
                    mdefs->variant[i] = p;
                } else {
                    break;
                }
            }

            if (p && (p = strchr(p, ',')))
                *p = '\0';
        }
    }

    return true;
}

static void
free_multi_defs(struct multi_defs *defs)
{
    free(defs->options);
    /*
     * See make_multi_defs comment for the hack; the same strdup'd
     * string is split among the indexes, but the one in [0] is const.
     */
    free(UNCONSTIFY(defs->layout[1]));
    free(UNCONSTIFY(defs->variant[1]));
}

/* See apply_rule below. */
static void
apply(const char *src, char **dst)
{
    int ret;
    char *tmp;

    if (!src)
        return;

    if (*src == '+' || *src == '!') {
        tmp = *dst;
        ret = asprintf(dst, "%s%s", *dst, src);
        if (ret < 0)
            *dst = NULL;
        free(tmp);
    }
    else if (*dst == NULL) {
        *dst = strdup(src);
    }
}

/*
 * Add the info from the matching rule to the resulting
 * xkb_component_names. If we already had a match for something
 * (e.g. keycodes), and the rule is not an appending one (e.g.
 * +whatever), than we don't override but drop the new one.
 */
static void
apply_rule(struct rule *rule, struct xkb_component_names *kccgst)
{
    /* Clear the flag because it's applied. */
    rule->flags &= ~RULE_FLAG_PENDING_MATCH;

    apply(rule->keycodes, &kccgst->keycodes);
    apply(rule->symbols, &kccgst->symbols);
    apply(rule->types, &kccgst->types);
    apply(rule->compat, &kccgst->compat);
}

/*
 * Match if name is part of the group, e.g. if the following
 * group is defined:
 *      ! $qwertz = al cz de hr hu ro si sk
 * then
 *      match_group_member(rules, "qwertz", "hr")
 * will return true.
 */
static bool
match_group_member(struct rules *rules, const char *group_name,
                   const char *name)
{
   int i;
   const char *word;
   struct group *iter, *group = NULL;

   darray_foreach(iter, rules->groups) {
       if (strcmp(iter->name, group_name) == 0) {
           group = iter;
           break;
       }
   }

   if (!group)
       return false;

   word = group->words;
   for (i = 0; i < group->number; i++, word += strlen(word) + 1)
       if (strcmp(word, name) == 0)
           return true;

   return false;
}

/* Match @needle out of @sep-seperated @haystack. */
static bool
match_one_of(const char *haystack, const char *needle, char sep)
{
    const char *s = haystack;
    const size_t len = strlen(needle);

    do {
        if (strncmp(s, needle, len) == 0 && (s[len] == '\0' || s[len] == sep))
            return true;
        s = strchr(s, sep);
    } while (s++);

    return false;
}

static int
apply_rule_if_matches(struct rules *rules, struct rule *rule,
                      struct multi_defs *mdefs,
                      struct xkb_component_names *kccgst)
{
    bool pending = false;

    if (rule->model) {
        if (mdefs->model == NULL)
            return 0;

        if (strcmp(rule->model, "*") == 0) {
            pending = true;
        }
        else if (rule->model[0] == '$') {
            if (!match_group_member(rules, rule->model, mdefs->model))
                return 0;
        }
        else if (strcmp(rule->model, mdefs->model) != 0) {
            return 0;
        }
    }

    if (rule->option) {
        if (mdefs->options == NULL)
            return 0;

        if (!match_one_of(mdefs->options, rule->option, ','))
            return 0;
    }

    if (rule->layout) {
        if (mdefs->layout[rule->layout_num] == NULL)
            return 0;

        if (strcmp(rule->layout, "*") == 0) {
            pending = true;
        }
        else if (rule->layout[0] == '$') {
            if (!match_group_member(rules, rule->layout,
                                    mdefs->layout[rule->layout_num]))
                  return 0;
        }
        else if (strcmp(rule->layout,
                        mdefs->layout[rule->layout_num]) != 0) {
            return 0;
        }
    }

    if (rule->variant) {
        if (mdefs->variant[rule->variant_num] == NULL)
            return 0;

        if (strcmp(rule->variant, "*") == 0) {
            pending = true;
        } else if (rule->variant[0] == '$') {
            if (!match_group_member(rules, rule->variant,
                                    mdefs->variant[rule->variant_num]))
                return 0;
        }
        else if (strcmp(rule->variant,
                        mdefs->variant[rule->variant_num]) != 0) {
            return 0;
        }
    }

    if (pending) {
        rule->flags |= RULE_FLAG_PENDING_MATCH;
    } else {
        /* Exact match, apply it now. */
        apply_rule(rule, kccgst);
    }

    return rule->number;
}

static void
clear_partial_matches(struct rules *rules)
{
    struct rule *rule;

    darray_foreach(rule, rules->rules)
        rule->flags &= ~RULE_FLAG_PENDING_MATCH;
}

static void
apply_partial_matches(struct rules *rules, struct xkb_component_names *kccgst)
{
    struct rule *rule;

    darray_foreach(rule, rules->rules)
        if (rule->flags & RULE_FLAG_PENDING_MATCH)
            apply_rule(rule, kccgst);
}

static void
apply_matching_rules(struct rules *rules, struct multi_defs *mdefs,
                     struct xkb_component_names *kccgst, unsigned int flags)
{
    int skip = -1;
    struct rule *rule;

    darray_foreach(rule, rules->rules) {
        if ((rule->flags & flags) != flags)
            continue;

        if ((flags & RULE_FLAG_OPTION) == 0 && rule->number == skip)
            continue;

        skip = apply_rule_if_matches(rules, rule, mdefs, kccgst);
    }
}

/***====================================================================***/

static char *
substitute_vars(char *name, struct multi_defs *mdefs)
{
    char *str, *outstr, *var;
    char *orig = name;
    size_t len, extra_len;
    char pfx, sfx;
    int ndx;

    if (!name)
        return NULL;

    str = strchr(name, '%');
    if (str == NULL)
        return name;

    len = strlen(name);

    while (str != NULL) {
        pfx = str[1];
        extra_len = 0;

        if (pfx == '+' || pfx == '|' || pfx == '_' || pfx == '-') {
            extra_len = 1;
            str++;
        }
        else if (pfx == '(') {
            extra_len = 2;
            str++;
        }

        var = str + 1;
        str = get_index(var + 1, &ndx);
        if (ndx == -1) {
            str = strchr(str, '%');
            continue;
        }

        if (*var == 'l' && mdefs->layout[ndx] && *mdefs->layout[ndx])
            len += strlen(mdefs->layout[ndx]) + extra_len;
        else if (*var == 'm' && mdefs->model)
            len += strlen(mdefs->model) + extra_len;
        else if (*var == 'v' && mdefs->variant[ndx] && *mdefs->variant[ndx])
            len += strlen(mdefs->variant[ndx]) + extra_len;

        if (pfx == '(' && *str == ')')
            str++;

        str = strchr(&str[0], '%');
    }

    name = malloc(len + 1);
    str = orig;
    outstr = name;

    while (*str != '\0') {
        if (str[0] == '%') {
            str++;
            pfx = str[0];
            sfx = '\0';

            if (pfx == '+' || pfx == '|' || pfx == '_' || pfx == '-') {
                str++;
            }
            else if (pfx == '(') {
                sfx = ')';
                str++;
            }
            else {
                pfx = '\0';
            }

            var = str;
            str = get_index(var + 1, &ndx);
            if (ndx == -1)
                continue;

            if (*var == 'l' && mdefs->layout[ndx] && *mdefs->layout[ndx]) {
                if (pfx)
                    *outstr++ = pfx;

                strcpy(outstr, mdefs->layout[ndx]);
                outstr += strlen(mdefs->layout[ndx]);

                if (sfx)
                    *outstr++ = sfx;
            }
            else if (*var == 'm' && mdefs->model) {
                if (pfx)
                    *outstr++ = pfx;

                strcpy(outstr, mdefs->model);
                outstr += strlen(mdefs->model);

                if (sfx)
                    *outstr++ = sfx;
            }
            else if (*var == 'v' && mdefs->variant[ndx] && *mdefs->variant[ndx]) {
                if (pfx)
                    *outstr++ = pfx;

                strcpy(outstr, mdefs->variant[ndx]);
                outstr += strlen(mdefs->variant[ndx]);

                if (sfx)
                    *outstr++ = sfx;
            }

            if (pfx == '(' && *str == ')')
                str++;
        }
        else {
            *outstr++= *str++;
        }
    }

    *outstr++= '\0';

    if (orig != name)
        free(orig);

    return name;
}

/***====================================================================***/

static bool
get_components(struct rules *rules, const struct xkb_rule_names *mlvo,
               struct xkb_component_names *kccgst)
{
    struct multi_defs mdefs;

    make_multi_defs(&mdefs, mlvo);

    clear_partial_matches(rules);

    apply_matching_rules(rules, &mdefs, kccgst, RULE_FLAG_NORMAL);
    apply_partial_matches(rules, kccgst);

    apply_matching_rules(rules, &mdefs, kccgst, RULE_FLAG_APPEND);
    apply_partial_matches(rules, kccgst);

    apply_matching_rules(rules, &mdefs, kccgst, RULE_FLAG_OPTION);
    apply_partial_matches(rules, kccgst);

    kccgst->keycodes = substitute_vars(kccgst->keycodes, &mdefs);
    kccgst->symbols = substitute_vars(kccgst->symbols, &mdefs);
    kccgst->types = substitute_vars(kccgst->types, &mdefs);
    kccgst->compat = substitute_vars(kccgst->compat, &mdefs);

    free_multi_defs(&mdefs);

    return
        kccgst->keycodes && kccgst->symbols && kccgst->types && kccgst->compat;
}

static struct rules *
load_rules(FILE *file)
{
    darray_char line;
    struct mapping mapping;
    struct rule trule;
    struct group tgroup;
    struct rules *rules;

    rules = calloc(1, sizeof(*rules));
    if (!rules)
        return NULL;
    darray_init(rules->rules);
    darray_growalloc(rules->rules, 16);

    memset(&mapping, 0, sizeof(mapping));
    memset(&tgroup, 0, sizeof(tgroup));
    darray_init(line);
    darray_growalloc(line, 128);

    while (input_line_get(file, &line)) {
        if (match_line(&line, &mapping, &trule, &tgroup)) {
            if (tgroup.number) {
                darray_append(rules->groups, tgroup);
                memset(&tgroup, 0, sizeof(tgroup));
            } else {
                darray_append(rules->rules, trule);
                memset(&trule, 0, sizeof(trule));
            }
        }

        darray_resize(line, 0);
    }

    darray_free(line);
    return rules;
}

static void
free_rules(struct rules *rules)
{
    struct rule *rule;
    struct group *group;

    if (!rules)
        return;

    darray_foreach(rule, rules->rules) {
        free(rule->model);
        free(rule->layout);
        free(rule->variant);
        free(rule->option);
        free(rule->keycodes);
        free(rule->symbols);
        free(rule->types);
        free(rule->compat);
    }
    darray_free(rules->rules);

    darray_foreach(group, rules->groups) {
        free(group->name);
        free(group->words);
    }
    darray_free(rules->groups);

    free(rules);
}

struct xkb_component_names *
xkb_components_from_rules(struct xkb_context *ctx,
                          const struct xkb_rule_names *rmlvo)
{
    int i;
    FILE *file;
    char *path;
    struct rules *rules;
    struct xkb_component_names *kccgst = NULL;

    file = XkbFindFileInPath(ctx, rmlvo->rules, FILE_TYPE_RULES, &path);
    if (!file) {
        ERROR("could not find \"%s\" rules in XKB path\n", rmlvo->rules);
        ERROR("%d include paths searched:\n",
              xkb_context_num_include_paths(ctx));
        for (i = 0; i < xkb_context_num_include_paths(ctx); i++)
            ERROR("\t%s\n", xkb_context_include_path_get(ctx, i));
        return NULL;
    }

    rules = load_rules(file);
    if (!rules) {
        ERROR("failed to load XKB rules \"%s\"\n", path);
        goto err;
    }

    kccgst = calloc(1, sizeof(*kccgst));
    if (!kccgst) {
        ERROR("failed to allocate XKB components\n");
        goto err;
    }

    if (!get_components(rules, rmlvo, kccgst)) {
        free(kccgst->keycodes);
        free(kccgst->types);
        free(kccgst->compat);
        free(kccgst->symbols);
        free(kccgst);
        kccgst = NULL;
        ERROR("no components returned from XKB rules \"%s\"\n", path);
        goto err;
    }

err:
    free_rules(rules);
    if (file)
        fclose(file);
    free(path);
    return kccgst;
}
