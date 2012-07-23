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

#include "parseutils.h"
#include "path.h"

ATTR_MALLOC static void *
malloc_or_die(size_t size)
{
    void *p = malloc(size);
    if (!p) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return p;
}

ParseCommon *
AppendStmt(ParseCommon * to, ParseCommon * append)
{
    ParseCommon *start = to;

    if (append == NULL)
        return to;
    while ((to != NULL) && (to->next != NULL))
    {
        to = to->next;
    }
    if (to) {
        to->next = append;
        return start;
    }
    return append;
}

ExprDef *
ExprCreate(unsigned op, unsigned type)
{
    ExprDef *expr;

    expr = malloc_or_die(sizeof(*expr));

    expr->common.stmtType = StmtExpr;
    expr->common.next = NULL;
    expr->op = op;
    expr->type = type;
    return expr;
}

ExprDef *
ExprCreateUnary(unsigned op, unsigned type, ExprDef * child)
{
    ExprDef *expr;
    expr = malloc_or_die(sizeof(*expr));

    expr->common.stmtType = StmtExpr;
    expr->common.next = NULL;
    expr->op = op;
    expr->type = type;
    expr->value.child = child;
    return expr;
}

ExprDef *
ExprCreateBinary(unsigned op, ExprDef * left, ExprDef * right)
{
    ExprDef *expr;

    expr = malloc_or_die(sizeof(*expr));

    expr->common.stmtType = StmtExpr;
    expr->common.next = NULL;
    expr->op = op;
    if ((op == OpAssign) || (left->type == TypeUnknown))
        expr->type = right->type;
    else if ((left->type == right->type) || (right->type == TypeUnknown))
        expr->type = left->type;
    else
        expr->type = TypeUnknown;
    expr->value.binary.left = left;
    expr->value.binary.right = right;
    return expr;
}

KeycodeDef *
KeycodeCreate(char keyName[XkbKeyNameLength], unsigned long value)
{
    KeycodeDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.stmtType = StmtKeycodeDef;
    def->common.next = NULL;
    strncpy(def->name, keyName, XkbKeyNameLength);
    def->name[XkbKeyNameLength] = '\0';
    def->value = value;
    return def;
}

KeyAliasDef *
KeyAliasCreate(char alias[XkbKeyNameLength], char real[XkbKeyNameLength])
{
    KeyAliasDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.stmtType = StmtKeyAliasDef;
    def->common.next = NULL;
    strncpy(def->alias, alias, XkbKeyNameLength);
    def->alias[XkbKeyNameLength] = '\0';
    strncpy(def->real, real, XkbKeyNameLength);
    def->real[XkbKeyNameLength] = '\0';
    return def;
}

VModDef *
VModCreate(xkb_atom_t name, ExprDef * value)
{
    VModDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.stmtType = StmtVModDef;
    def->common.next = NULL;
    def->name = name;
    def->value = value;
    return def;
}

VarDef *
VarCreate(ExprDef * name, ExprDef * value)
{
    VarDef *def;
    def = malloc_or_die(sizeof(*def));

    def->common.stmtType = StmtVarDef;
    def->common.next = NULL;
    def->name = name;
    def->value = value;
    return def;
}

VarDef *
BoolVarCreate(xkb_atom_t nameToken, unsigned set)
{
    ExprDef *name, *value;

    name = ExprCreate(ExprIdent, TypeUnknown);
    name->value.str = nameToken;
    value = ExprCreate(ExprValue, TypeBoolean);
    value->value.uval = set;
    return VarCreate(name, value);
}

InterpDef *
InterpCreate(char *sym, ExprDef * match)
{
    InterpDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.stmtType = StmtInterpDef;
    def->common.next = NULL;
    def->sym = sym;
    def->match = match;
    return def;
}

KeyTypeDef *
KeyTypeCreate(xkb_atom_t name, VarDef * body)
{
    KeyTypeDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.stmtType = StmtKeyTypeDef;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->name = name;
    def->body = body;
    return def;
}

SymbolsDef *
SymbolsCreate(char keyName[XkbKeyNameLength], ExprDef *symbols)
{
    SymbolsDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.stmtType = StmtSymbolsDef;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    strncpy(def->keyName, keyName, XkbKeyNameLength);
    def->symbols = symbols;
    return def;
}

GroupCompatDef *
GroupCompatCreate(int group, ExprDef * val)
{
    GroupCompatDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.stmtType = StmtGroupCompatDef;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->group = group;
    def->def = val;
    return def;
}

ModMapDef *
ModMapCreate(uint32_t modifier, ExprDef * keys)
{
    ModMapDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.stmtType = StmtModMapDef;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->modifier = modifier;
    def->keys = keys;
    return def;
}

IndicatorMapDef *
IndicatorMapCreate(xkb_atom_t name, VarDef * body)
{
    IndicatorMapDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.stmtType = StmtIndicatorMapDef;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->name = name;
    def->body = body;
    return def;
}

IndicatorNameDef *
IndicatorNameCreate(int ndx, ExprDef * name, bool virtual)
{
    IndicatorNameDef *def;

    def = malloc_or_die(sizeof(*def));

    def->common.stmtType = StmtIndicatorNameDef;
    def->common.next = NULL;
    def->merge = MERGE_DEFAULT;
    def->ndx = ndx;
    def->name = name;
    def->virtual = virtual;
    return def;
}

ExprDef *
ActionCreate(xkb_atom_t name, ExprDef * args)
{
    ExprDef *act;

    act = malloc_or_die(sizeof(*act));

    act->common.stmtType = StmtExpr;
    act->common.next = NULL;
    act->op = ExprActionDecl;
    act->value.action.name = name;
    act->value.action.args = args;
    return act;
}

ExprDef *
CreateKeysymList(char *sym)
{
    ExprDef *def;

    def = ExprCreate(ExprKeysymList, TypeSymbols);

    darray_init(def->value.list.syms);
    darray_init(def->value.list.symsMapIndex);
    darray_init(def->value.list.symsNumEntries);

    darray_append(def->value.list.syms, sym);
    darray_append(def->value.list.symsMapIndex, 0);
    darray_append(def->value.list.symsNumEntries, 1);

    return def;
}

ExprDef *
CreateMultiKeysymList(ExprDef *list)
{
    size_t nLevels = darray_size(list->value.list.symsMapIndex);

    darray_resize(list->value.list.symsMapIndex, 1);
    darray_resize(list->value.list.symsNumEntries, 1);
    darray_item(list->value.list.symsMapIndex, 0) = 0;
    darray_item(list->value.list.symsNumEntries, 0) = nLevels;

    return list;
}

ExprDef *
AppendKeysymList(ExprDef * list, char *sym)
{
    size_t nSyms = darray_size(list->value.list.syms);

    darray_append(list->value.list.symsMapIndex, nSyms);
    darray_append(list->value.list.symsNumEntries, 1);
    darray_append(list->value.list.syms, sym);

    return list;
}

ExprDef *
AppendMultiKeysymList(ExprDef * list, ExprDef * append)
{
    size_t nSyms = darray_size(list->value.list.syms);
    size_t numEntries = darray_size(append->value.list.syms);

    darray_append(list->value.list.symsMapIndex, nSyms);
    darray_append(list->value.list.symsNumEntries, numEntries);
    darray_append_items(list->value.list.syms,
                        darray_mem(append->value.list.syms, 0),
                        numEntries);

    darray_resize(append->value.list.syms, 0);
    FreeStmt(&append->common);

    return list;
}

int
LookupKeysym(const char *str, xkb_keysym_t *sym_rtrn)
{
    xkb_keysym_t sym;

    if ((!str) || (strcasecmp(str, "any") == 0) ||
        (strcasecmp(str, "nosymbol") == 0)) {
        *sym_rtrn = XKB_KEY_NoSymbol;
        return 1;
    }
    else if ((strcasecmp(str, "none") == 0) ||
             (strcasecmp(str, "voidsymbol") == 0)) {
        *sym_rtrn = XKB_KEY_VoidSymbol;
        return 1;
    }
    sym = xkb_keysym_from_name(str);
    if (sym != XKB_KEY_NoSymbol) {
        *sym_rtrn = sym;
        return 1;
    }
    return 0;
}

static void
FreeInclude(IncludeStmt *incl);

IncludeStmt *
IncludeCreate(struct xkb_context *ctx, char *str, enum merge_mode merge)
{
    IncludeStmt *incl, *first;
    char *file, *map, *stmt, *tmp, *extra_data;
    char nextop;

    incl = first = NULL;
    file = map = NULL;
    tmp = str;
    stmt = uDupString(str);
    while (tmp && *tmp)
    {
        if (!XkbParseIncludeMap(&tmp, &file, &map, &nextop, &extra_data))
            goto err;

        if (first == NULL) {
            first = incl = malloc(sizeof(*first));
        } else {
            incl->next = malloc(sizeof(*first));
            incl = incl->next;
        }

        if (!incl) {
            log_wsgo(ctx,
                     "Allocation failure in IncludeCreate; "
                     "Using only part of the include\n");
            break;
        }

        incl->common.stmtType = StmtInclude;
        incl->common.next = NULL;
        incl->merge = merge;
        incl->stmt = NULL;
        incl->file = file;
        incl->map = map;
        incl->modifier = extra_data;
        incl->path = NULL;
        incl->next = NULL;

        if (nextop == '|')
            merge = MERGE_AUGMENT;
        else
            merge = MERGE_OVERRIDE;
    }

    if (first)
        first->stmt = stmt;
    else
        free(stmt);

    return first;

err:
    log_err(ctx, "Illegal include statement \"%s\"; Ignored\n", stmt);
    FreeInclude(first);
    free(stmt);
    return NULL;
}

void
CheckDefaultMap(struct xkb_context *ctx, XkbFile *maps, const char *fileName)
{
    XkbFile *dflt = NULL, *tmp;

    for (tmp = maps; tmp; tmp = (XkbFile *) tmp->common.next) {
        if (!(tmp->flags & XkbLC_Default))
            continue;
        if (!dflt) {
            dflt = tmp;
            continue;
        }

        log_lvl(ctx, 3,
                "Multiple default components in %s; "
                "Using %s, ignoring %s\n",
                (fileName ? fileName : "(unknown)"),
                (dflt->name ? dflt->name : "(first)"),
                (tmp->name ? tmp->name : "(subsequent)"));

        tmp->flags &= (~XkbLC_Default);
    }
}

/*
 * All latin-1 alphanumerics, plus parens, slash, minus, underscore and
 * wildcards.
 */
static const unsigned char componentSpecLegal[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xa7, 0xff, 0x83,
    0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0xff
};

static void
EnsureSafeMapName(char *name)
{
    if (!name)
        return;

    while (*name != '\0') {
        if ((componentSpecLegal[(*name) / 8] & (1 << ((*name) % 8))) == 0)
            *name = '_';
        name++;
    }
}

XkbFile *
CreateXKBFile(struct xkb_context *ctx, enum xkb_file_type type, char *name,
              ParseCommon *defs, unsigned flags)
{
    XkbFile *file;

    file = calloc(1, sizeof(*file));
    if (!file)
        return NULL;

    EnsureSafeMapName(name);
    file->type = type;
    file->topName = name ? strdup(name) : NULL;
    file->name = name;
    file->defs = defs;
    file->id = xkb_context_take_file_id(ctx);
    file->flags = flags;
    return file;
}

static void
FreeExpr(ExprDef *expr)
{
    char **sym;

    if (!expr)
        return;

    switch (expr->op) {
    case ExprActionList:
    case OpNegate:
    case OpUnaryPlus:
    case OpNot:
    case OpInvert:
        FreeStmt(&expr->value.child->common);
        break;

    case OpDivide:
    case OpAdd:
    case OpSubtract:
    case OpMultiply:
    case OpAssign:
        FreeStmt(&expr->value.binary.left->common);
        FreeStmt(&expr->value.binary.right->common);
        break;

    case ExprActionDecl:
        FreeStmt(&expr->value.action.args->common);
        break;

    case ExprArrayRef:
        FreeStmt(&expr->value.array.entry->common);
        break;

    case ExprKeysymList:
        darray_foreach(sym, expr->value.list.syms)
            free(*sym);
        darray_free(expr->value.list.syms);
        darray_free(expr->value.list.symsMapIndex);
        darray_free(expr->value.list.symsNumEntries);
        break;

    default:
        break;
    }
}

static void
FreeInclude(IncludeStmt *incl)
{
    IncludeStmt *next;

    while (incl)
    {
        next = incl->next;

        free(incl->file);
        free(incl->map);
        free(incl->modifier);
        free(incl->path);
        free(incl->stmt);

        free(incl);
        incl = next;
    }
}

void
FreeStmt(ParseCommon *stmt)
{
    ParseCommon *next;
    YYSTYPE u;

    while (stmt)
    {
        next = stmt->next;
        u.any = stmt;

        switch (stmt->stmtType) {
        case StmtInclude:
            FreeInclude((IncludeStmt *) stmt);
            /* stmt is already free'd here. */
            stmt = NULL;
            break;
        case StmtExpr:
            FreeExpr(u.expr);
            break;
        case StmtVarDef:
            FreeStmt(&u.var->name->common);
            FreeStmt(&u.var->value->common);
            break;
        case StmtKeyTypeDef:
            FreeStmt(&u.keyType->body->common);
            break;
        case StmtInterpDef:
            free(u.interp->sym);
            FreeStmt(&u.interp->match->common);
            FreeStmt(&u.interp->def->common);
            break;
        case StmtVModDef:
            FreeStmt(&u.vmod->value->common);
            break;
        case StmtSymbolsDef:
            FreeStmt(&u.syms->symbols->common);
            break;
        case StmtModMapDef:
            FreeStmt(&u.modMask->keys->common);
            break;
        case StmtGroupCompatDef:
            FreeStmt(&u.groupCompat->def->common);
            break;
        case StmtIndicatorMapDef:
            FreeStmt(&u.ledMap->body->common);
            break;
        case StmtIndicatorNameDef:
            FreeStmt(&u.ledName->name->common);
            break;
        default:
            break;
        }

        free(stmt);
        stmt = next;
    }
}

void
FreeXKBFile(XkbFile *file)
{
    XkbFile *next;

    while (file)
    {
        next = (XkbFile *) file->common.next;

        switch (file->type) {
        case FILE_TYPE_KEYMAP:
            FreeXKBFile((XkbFile *) file->defs);
            break;

        case FILE_TYPE_TYPES:
        case FILE_TYPE_COMPAT:
        case FILE_TYPE_SYMBOLS:
        case FILE_TYPE_KEYCODES:
        case FILE_TYPE_GEOMETRY:
            FreeStmt(file->defs);
            break;

        default:
            break;
        }

        free(file->name);
        free(file->topName);
        free(file);
        file = next;
    }
}
