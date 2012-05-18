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

#define DFLT_LINE_SIZE 128

struct input_line {
    size_t size;
    size_t offset;
    char buf[DFLT_LINE_SIZE];
    char *line;
};

static void
input_line_init(struct input_line *line)
{
    line->size = DFLT_LINE_SIZE;
    line->offset = 0;
    line->line = line->buf;
}

static void
input_line_deinit(struct input_line *line)
{
    if (line->line != line->buf)
        free(line->line);
    line->offset = 0;
    line->size = DFLT_LINE_SIZE;
    line->line = line->buf;
}

static int
input_line_add_char(struct input_line *line, int ch)
{
    if (line->offset >= line->size) {
        if (line->line == line->buf) {
            line->line = malloc(line->size * 2);
            memcpy(line->line, line->buf, line->size);
        }
        else {
            line->line = realloc(line->line, line->size * 2);
        }

        line->size *= 2;
    }

    line->line[line->offset++] = ch;
    return ch;
}

static bool
input_line_get(FILE *file, struct input_line *line)
{
    int ch;
    bool end_of_file = false;
    bool space_pending;
    bool slash_pending;
    bool in_comment;

    while (!end_of_file && line->offset == 0) {
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
                    input_line_add_char(line, ' ');
                    space_pending = false;
                }

                input_line_add_char(line, '/');
                slash_pending = false;
            }

            if (isspace(ch)) {
                while (isspace(ch) && ch != '\n' && ch != EOF)
                    ch = getc(file);

                if (ch == EOF)
                    break;

                if (ch != '\n' && line->offset > 0)
                    space_pending = true;

                ungetc(ch, file);
            }
            else {
                if (space_pending) {
                    input_line_add_char(line, ' ');
                    space_pending = false;
                }

                if (ch == '!') {
                    if (line->offset != 0) {
                        WARN("The '!' is legal only at start of line\n");
                        ACTION("Line containing '!' ignored\n");
                        line->offset = 0;
                        break;
                    }
                }

                input_line_add_char(line, ch);
            }
        }

        if (ch == EOF)
            end_of_file = true;
    }

    if (line->offset == 0 && end_of_file)
        return false;

    input_line_add_char(line, '\0');
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
    KEYMAP,

#define COMPONENT_MASK \
    ((1 << KEYCODES) | (1 << SYMBOLS) | (1 << TYPES) | (1 << COMPAT) | \
     (1 << GEOMETRY) | (1 << KEYMAP))

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
    [KEYMAP] = "keymap",
};

struct var_defs {
    const char *model;
    const char *layout;
    const char *variant;
    const char *options;
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

struct describe_vars {
    size_t sz_desc;
    size_t num_desc;
    struct var_desc *desc;
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
    char *keymap;
    unsigned flags;
};

struct rules {
    struct describe_vars models;
    struct describe_vars layouts;
    struct describe_vars variants;
    struct describe_vars options;

    size_t sz_rules;
    size_t num_rules;
    struct rule *rules;

    size_t sz_groups;
    size_t num_groups;
    struct group *groups;
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
match_mapping_line(struct input_line *line, struct mapping *mapping)
{
    char *tok;
    char *str = &line->line[1];
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

    if (((present & COMPONENT_MASK) & (1 << KEYMAP)) &&
        ((present & COMPONENT_MASK) != (1 << KEYMAP))) {
        WARN("Keymap cannot appear with other components\n");
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
match_group_line(struct input_line *line, struct group *group)
{
    int i;
    char *name = strchr(line->line, '$');
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
match_rule_line(struct input_line *line, struct mapping *mapping,
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

    str = line->line;

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
        WARN("Too few words on a line: %s\n", line->line);
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
    rule->keymap = uDupString(names[KEYMAP]);

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
match_line(struct input_line *line, struct mapping *mapping,
           struct rule *rule, struct group *group)
{
    if (line->line[0] != '!')
        return match_rule_line(line, mapping, rule);

    if (line->line[1] == '$' || (line->line[1] == ' ' && line->line[2] == '$'))
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
 * Expand the layout and variant of a var_defs and remove extraneous spaces.
 * If there's one layout/variant, it is kept in .layout[0]/.variant[0], else
 * is kept in [1], [2] and so on, and [0] remains empty.
 * For example, this var_defs:
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
make_multi_defs(struct multi_defs *mdefs, struct var_defs *defs)
{
    char *p;
    int i;

    memset(mdefs, 0, sizeof(*mdefs));

    if (defs->model) {
        mdefs->model = defs->model;
    }

    if (defs->options) {
        mdefs->options = strdup(defs->options);
        if (mdefs->options == NULL)
            return false;

        squeeze_spaces(mdefs->options);
    }

    if (defs->layout) {
        if (!strchr(defs->layout, ',')) {
            mdefs->layout[0] = defs->layout;
        }
        else {
            p = strdup(defs->layout);
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

    if (defs->variant) {
        if (!strchr(defs->variant, ',')) {
            mdefs->variant[0] = defs->variant;
        }
        else {
            p = strdup(defs->variant);
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
    apply(rule->keymap, &kccgst->keymap);
}

static bool
CheckGroup(struct rules *rules, const char *group_name, const char *name)
{
   int i;
   const char *p;
   struct group *group;

   for (i = 0, group = rules->groups; i < rules->num_groups; i++, group++) {
       if (! strcmp(group->name, group_name)) {
           break;
       }
   }
   if (i == rules->num_groups)
       return false;
   for (i = 0, p = group->words; i < group->number; i++, p += strlen(p)+1) {
       if (! strcmp(p, name)) {
           return true;
       }
   }
   return false;
}

/* Match @needle out of @sep-seperated @haystack. */
static bool
match_one_of(const char *haystack, const char *needle, char sep)
{
    const char *s = strstr(haystack, needle);

    if (s == NULL)
        return false;

    if (s != haystack && *s != sep)
        return false;

    s += strlen(needle);
    if (*s != '\0' && *s != sep)
        return false;

    return true;
}

static int
XkbRF_CheckApplyRule(struct rule *rule, struct multi_defs *mdefs,
                     struct xkb_component_names *names, struct rules *rules)
{
    bool pending = false;

    if (rule->model != NULL) {
        if(mdefs->model == NULL)
            return 0;
        if (strcmp(rule->model, "*") == 0) {
            pending = true;
        } else {
            if (rule->model[0] == '$') {
               if (!CheckGroup(rules, rule->model, mdefs->model))
                  return 0;
            } else {
	       if (strcmp(rule->model, mdefs->model) != 0)
	          return 0;
	    }
	}
    }
    if (rule->option != NULL) {
	if (mdefs->options == NULL)
	    return 0;
        if ((!match_one_of(mdefs->options, rule->option, ',')))
	    return 0;
    }

    if (rule->layout != NULL) {
	if(mdefs->layout[rule->layout_num] == NULL ||
	   *mdefs->layout[rule->layout_num] == '\0')
	    return 0;
        if (strcmp(rule->layout, "*") == 0) {
            pending = true;
        } else {
            if (rule->layout[0] == '$') {
               if (!CheckGroup(rules, rule->layout,
                               mdefs->layout[rule->layout_num]))
                  return 0;
	    } else {
	       if (strcmp(rule->layout, mdefs->layout[rule->layout_num]) != 0)
	           return 0;
	    }
	}
    }
    if (rule->variant != NULL) {
	if (mdefs->variant[rule->variant_num] == NULL ||
	    *mdefs->variant[rule->variant_num] == '\0')
	    return 0;
        if (strcmp(rule->variant, "*") == 0) {
            pending = true;
        } else {
            if (rule->variant[0] == '$') {
               if (!CheckGroup(rules, rule->variant,
                               mdefs->variant[rule->variant_num]))
                  return 0;
            } else {
	       if (strcmp(rule->variant,
                          mdefs->variant[rule->variant_num]) != 0)
	           return 0;
	    }
	}
    }
    if (pending) {
        rule->flags |= RULE_FLAG_PENDING_MATCH;
	return rule->number;
    }
    /* exact match, apply it now */
    apply_rule(rule, names);
    return rule->number;
}

static void
XkbRF_ClearPartialMatches(struct rules *rules)
{
    int i;
    struct rule *rule;

    for (i=0,rule=rules->rules;i<rules->num_rules;i++,rule++) {
	rule->flags &= ~RULE_FLAG_PENDING_MATCH;
    }
}

static void
XkbRF_ApplyPartialMatches(struct rules *rules,
                          struct xkb_component_names *names)
{
    int i;
    struct rule *rule;

    for (rule = rules->rules, i = 0; i < rules->num_rules; i++, rule++) {
	if ((rule->flags & RULE_FLAG_PENDING_MATCH)==0)
	    continue;
        apply_rule(rule, names);
    }
}

static void
XkbRF_CheckApplyRules(struct rules *rules, struct multi_defs *mdefs,
                      struct xkb_component_names *names, unsigned int flags)
{
    int i;
    struct rule *rule;
    int skip;

    for (rule = rules->rules, i=0; i < rules->num_rules; rule++, i++) {
	if ((rule->flags & flags) != flags)
	    continue;
	skip = XkbRF_CheckApplyRule(rule, mdefs, names, rules);
	if (skip && !(flags & RULE_FLAG_OPTION)) {
	    for ( ;(i < rules->num_rules) && (rule->number == skip);
		  rule++, i++);
	    rule--; i--;
	}
    }
}

/***====================================================================***/

static char *
XkbRF_SubstituteVars(char *name, struct multi_defs *mdefs)
{
    char *str, *outstr, *orig, *var;
    size_t len;
    int ndx;

    if (!name)
        return NULL;

    orig= name;
    str= strchr(name,'%');
    if (str==NULL)
	return name;
    len= strlen(name);
    while (str!=NULL) {
	char pfx= str[1];
	int   extra_len= 0;
	if ((pfx=='+')||(pfx=='|')||(pfx=='_')||(pfx=='-')) {
	    extra_len= 1;
	    str++;
	}
	else if (pfx=='(') {
	    extra_len= 2;
	    str++;
	}
	var = str + 1;
	str = get_index(var + 1, &ndx);
	if (ndx == -1) {
	    str = strchr(str,'%');
	    continue;
        }
	if ((*var=='l') && mdefs->layout[ndx] && *mdefs->layout[ndx])
	    len+= strlen(mdefs->layout[ndx])+extra_len;
	else if ((*var=='m')&&mdefs->model)
	    len+= strlen(mdefs->model)+extra_len;
	else if ((*var=='v') && mdefs->variant[ndx] && *mdefs->variant[ndx])
	    len+= strlen(mdefs->variant[ndx])+extra_len;
	if ((pfx=='(')&&(*str==')')) {
	    str++;
	}
	str= strchr(&str[0],'%');
    }
    name = malloc(len + 1);
    str= orig;
    outstr= name;
    while (*str!='\0') {
	if (str[0]=='%') {
	    char pfx,sfx;
	    str++;
	    pfx= str[0];
	    sfx= '\0';
	    if ((pfx=='+')||(pfx=='|')||(pfx=='_')||(pfx=='-')) {
		str++;
	    }
	    else if (pfx=='(') {
		sfx= ')';
		str++;
	    }
	    else pfx= '\0';

	    var = str;
	    str = get_index(var + 1, &ndx);
	    if (ndx == -1) {
	        continue;
            }
	    if ((*var=='l') && mdefs->layout[ndx] && *mdefs->layout[ndx]) {
		if (pfx) *outstr++= pfx;
		strcpy(outstr,mdefs->layout[ndx]);
		outstr+= strlen(mdefs->layout[ndx]);
		if (sfx) *outstr++= sfx;
	    }
	    else if ((*var=='m')&&(mdefs->model)) {
		if (pfx) *outstr++= pfx;
		strcpy(outstr,mdefs->model);
		outstr+= strlen(mdefs->model);
		if (sfx) *outstr++= sfx;
	    }
	    else if ((*var=='v') && mdefs->variant[ndx] && *mdefs->variant[ndx]) {
		if (pfx) *outstr++= pfx;
		strcpy(outstr,mdefs->variant[ndx]);
		outstr+= strlen(mdefs->variant[ndx]);
		if (sfx) *outstr++= sfx;
	    }
	    if ((pfx=='(')&&(*str==')'))
		str++;
	}
	else {
	    *outstr++= *str++;
	}
    }
    *outstr++= '\0';
    if (orig!=name)
	free(orig);
    return name;
}

/***====================================================================***/

static bool
XkbcRF_GetComponents(struct rules *rules, struct var_defs *defs,
                     struct xkb_component_names *names)
{
    struct multi_defs mdefs;

    make_multi_defs(&mdefs, defs);

    memset(names, 0, sizeof(struct xkb_component_names));
    XkbRF_ClearPartialMatches(rules);
    XkbRF_CheckApplyRules(rules, &mdefs, names, RULE_FLAG_NORMAL);
    XkbRF_ApplyPartialMatches(rules, names);
    XkbRF_CheckApplyRules(rules, &mdefs, names, RULE_FLAG_APPEND);
    XkbRF_ApplyPartialMatches(rules, names);
    XkbRF_CheckApplyRules(rules, &mdefs, names, RULE_FLAG_OPTION);
    XkbRF_ApplyPartialMatches(rules, names);

    names->keycodes = XkbRF_SubstituteVars(names->keycodes, &mdefs);
    names->symbols = XkbRF_SubstituteVars(names->symbols, &mdefs);
    names->types = XkbRF_SubstituteVars(names->types, &mdefs);
    names->compat = XkbRF_SubstituteVars(names->compat, &mdefs);
    names->keymap = XkbRF_SubstituteVars(names->keymap, &mdefs);

    free_multi_defs(&mdefs);

    return (names->keycodes && names->symbols && names->types &&
            names->compat) || names->keymap;
}

static struct rule *
XkbcRF_AddRule(struct rules *rules)
{
    if (rules->sz_rules<1) {
	rules->sz_rules= 16;
	rules->num_rules= 0;
	rules->rules = calloc(rules->sz_rules, sizeof(*rules->rules));
    }
    else if (rules->num_rules>=rules->sz_rules) {
	rules->sz_rules*= 2;
        rules->rules = realloc(rules->rules,
                               rules->sz_rules * sizeof(*rules->rules));
    }
    if (!rules->rules) {
	rules->sz_rules= rules->num_rules= 0;
	return NULL;
    }
    memset(&rules->rules[rules->num_rules], 0, sizeof(*rules->rules));
    return &rules->rules[rules->num_rules++];
}

static struct group *
XkbcRF_AddGroup(struct rules *rules)
{
    if (rules->sz_groups<1) {
	rules->sz_groups= 16;
	rules->num_groups= 0;
        rules->groups = calloc(rules->sz_groups, sizeof(*rules->groups));
    }
    else if (rules->num_groups >= rules->sz_groups) {
	rules->sz_groups *= 2;
        rules->groups = realloc(rules->groups,
                                rules->sz_groups * sizeof(*rules->groups));
    }
    if (!rules->groups) {
	rules->sz_groups= rules->num_groups= 0;
	return NULL;
    }

    memset(&rules->groups[rules->num_groups], 0, sizeof(*rules->groups));
    return &rules->groups[rules->num_groups++];
}

static struct rules *
XkbcRF_LoadRules(FILE *file)
{
    struct input_line line;
    struct mapping mapping;
    struct rule trule, *rule;
    struct group tgroup, *group;
    struct rules *rules;

    rules = calloc(1, sizeof(*rules));
    if (!rules)
        return NULL;

    memset(&mapping, 0, sizeof(mapping));
    memset(&tgroup, 0, sizeof(tgroup));
    input_line_init(&line);

    while (input_line_get(file, &line)) {
        if (match_line(&line, &mapping, &trule, &tgroup)) {
            if (tgroup.number) {
	        if ((group= XkbcRF_AddGroup(rules))!=NULL) {
		    *group= tgroup;
		    memset(&tgroup, 0, sizeof(tgroup));
	        }
	    } else {
	        if ((rule= XkbcRF_AddRule(rules))!=NULL) {
		    *rule= trule;
		    memset(&trule, 0, sizeof(trule));
	        }
	    }
	}
	line.offset = 0;
    }
    input_line_deinit(&line);
    return rules;
}

static void
XkbRF_ClearVarDescriptions(struct describe_vars *var)
{
    int i;

    for (i=0;i<var->num_desc;i++) {
	free(var->desc[i].name);
	free(var->desc[i].desc);
	var->desc[i].name= var->desc[i].desc= NULL;
    }
    free(var->desc);
    var->desc= NULL;
}

static void
XkbcRF_Free(struct rules *rules)
{
    int i;
    struct rule *rule;
    struct group *group;

    if (!rules)
	return;
    XkbRF_ClearVarDescriptions(&rules->models);
    XkbRF_ClearVarDescriptions(&rules->layouts);
    XkbRF_ClearVarDescriptions(&rules->variants);
    XkbRF_ClearVarDescriptions(&rules->options);

    for (i=0, rule = rules->rules; i < rules->num_rules && rules; i++, rule++) {
        free(rule->model);
        free(rule->layout);
        free(rule->variant);
        free(rule->option);
        free(rule->keycodes);
        free(rule->symbols);
        free(rule->types);
        free(rule->compat);
        free(rule->keymap);
    }
    free(rules->rules);

    for (i=0, group = rules->groups; i < rules->num_groups && group; i++, group++) {
        free(group->name);
        free(group->words);
    }
    free(rules->groups);

    free(rules);
}

struct xkb_component_names *
xkb_components_from_rules(struct xkb_context *ctx,
                          const struct xkb_rule_names *rmlvo)
{
    int i;
    FILE *rulesFile;
    char *rulesPath;
    struct rules *loaded;
    struct xkb_component_names *names = NULL;
    struct var_defs defs = {
        .model = rmlvo->model,
        .layout = rmlvo->layout,
        .variant = rmlvo->variant,
        .options = rmlvo->options,
    };

    rulesFile = XkbFindFileInPath(ctx, rmlvo->rules, XkmRulesFile,
                                  &rulesPath);
    if (!rulesFile) {
        ERROR("could not find \"%s\" rules in XKB path\n", rmlvo->rules);
        ERROR("%d include paths searched:\n",
              xkb_context_num_include_paths(ctx));
        for (i = 0; i < xkb_context_num_include_paths(ctx); i++)
            ERROR("\t%s\n", xkb_context_include_path_get(ctx, i));
        return NULL;
    }

    loaded = XkbcRF_LoadRules(rulesFile);
    if (!loaded) {
        ERROR("failed to load XKB rules \"%s\"\n", rulesPath);
        goto unwind_file;
    }

    names = calloc(1, sizeof(*names));
    if (!names) {
        ERROR("failed to allocate XKB components\n");
        goto unwind_file;
    }

    if (!XkbcRF_GetComponents(loaded, &defs, names)) {
        free(names->keymap);
        free(names->keycodes);
        free(names->types);
        free(names->compat);
        free(names->symbols);
        free(names);
        names = NULL;
        ERROR("no components returned from XKB rules \"%s\"\n", rulesPath);
    }

unwind_file:
    XkbcRF_Free(loaded);
    if (rulesFile)
        fclose(rulesFile);
    free(rulesPath);
    return names;
}
