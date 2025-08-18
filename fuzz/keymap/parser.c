/*
 * A target program for fuzzing the XKB keymap text format.
 *
 * Currently, just parses an input file, and hopefully doesn't crash or hang.
 */
#include "config.h"

#include <assert.h>

#include "xkbcommon/xkbcommon.h"
#include "xkbcomp/ast-build.h"
#include "atom-priv.h"
#include "keymap.h"
#include "context.h"
#include "xkbcomp/xkbcomp-priv.h"

static const char dummy[] =
    "xkb_keymap {\n"
    "    xkb_keycodes { };\n"
    "    xkb_types { };\n"
    "    xkb_compat { };\n"
    "    xkb_symbols { };\n"
    "};";

/* Forward-declare the libFuzzer's mutator callback. */
extern size_t
LLVMFuzzerMutate(uint8_t *Data, size_t Size, size_t MaxSize);

static void
mutate_ast(ParseCommon *stmt)
{
    switch (stmt->type) {
    case STMT_INCLUDE:
        // TODO
        break;
    case STMT_KEYCODE: {
        KeycodeDef *def = (KeycodeDef *) stmt;
        LLVMFuzzerMutate((uint8_t *)&def->merge,
                          sizeof(def->merge), sizeof(def->merge));
        LLVMFuzzerMutate((uint8_t *)&def->value,
                          sizeof(def->value), sizeof(def->value));
        break;
    }
    case STMT_ALIAS:
        // TODO
        break;
    case STMT_EXPR: {
        ExprDef *def = (ExprDef *) stmt;
        switch (def->expr.op) {
            case EXPR_VALUE: {
                switch (def->expr.value_type) {
                case EXPR_TYPE_BOOLEAN: {
                    LLVMFuzzerMutate((uint8_t *)&def->boolean.set,
                                     sizeof(def->boolean.set),
                                     sizeof(def->boolean.set));
                    break;
                }
                case EXPR_TYPE_INT: {
                    LLVMFuzzerMutate((uint8_t *)&def->integer.ival,
                                     sizeof(def->integer.ival),
                                     sizeof(def->integer.ival));
                    break;
                }
                case EXPR_TYPE_FLOAT:
                case EXPR_TYPE_STRING:
                case EXPR_TYPE_ACTION:
                case EXPR_TYPE_ACTIONS:
                case EXPR_TYPE_KEYNAME:
                case EXPR_TYPE_SYMBOLS:
                default:
                    ;
                }
                break;
            }
            case EXPR_IDENT:
            case EXPR_ACTION_DECL:
            case EXPR_FIELD_REF:
            case EXPR_ARRAY_REF:
            case EXPR_EMPTY_LIST:
                break;
            case EXPR_KEYSYM_LIST: {
                xkb_keysym_t keysyms[10] = {0};
                LLVMFuzzerMutate((uint8_t *)keysyms,
                                 sizeof(keysyms), sizeof(keysyms));
                const size_t len = darray_size(def->keysym_list.syms);
                for (size_t j = 0, k = 0; j < len && k < ARRAY_SIZE(keysyms); k++) {
                    /* Ensure there is no null entry */
                    if (keysyms[k] == XKB_KEY_NoSymbol)
                        continue;
                    darray_item(def->keysym_list.syms, j) = keysyms[k];
                    j++;
                }
                break;
            }
            case EXPR_ACTION_LIST:
                break;
            case EXPR_ADD:
            case EXPR_SUBTRACT:
            case EXPR_MULTIPLY:
            case EXPR_DIVIDE: {
                mutate_ast((ParseCommon*)def->binary.left);
                mutate_ast((ParseCommon*)def->binary.right);
                break;
            }
            case EXPR_ASSIGN:
            case EXPR_NOT:
            case EXPR_NEGATE:
            case EXPR_INVERT:
            case EXPR_UNARY_PLUS:
            default:
                ;
        }
        break;
    }
    case STMT_VAR: {
        VarDef *def = (VarDef *) stmt;
        LLVMFuzzerMutate((uint8_t *)&def->merge,
                         sizeof(def->merge), sizeof(def->merge));
        // mutate_ast((ParseCommon*)def->name);
        mutate_ast((ParseCommon*)def->value);
        break;
    }
    case STMT_TYPE:
    case STMT_INTERP:
    case STMT_VMOD:
        break;
    case STMT_SYMBOLS: {
        SymbolsDef *def = (SymbolsDef *) stmt;
        LLVMFuzzerMutate((uint8_t *)&def->merge,
                         sizeof(def->merge), sizeof(def->merge));
        for (VarDef *varDef = def->symbols; varDef;
             varDef = (VarDef *) varDef->common.next) {
            mutate_ast((ParseCommon*)varDef);
        }
        break;
    }
    case STMT_MODMAP:
    case STMT_GROUP_COMPAT:
    case STMT_LED_MAP:
    case STMT_LED_NAME:
    default:
        ;
    }
}

/* Custom mutator */
extern size_t LLVMFuzzerCustomMutator(uint8_t *Data, size_t Size,
                                      size_t MaxSize, unsigned int Seed);

extern size_t LLVMFuzzerCustomMutator(uint8_t *Data, size_t Size,
                                      size_t MaxSize, unsigned int Seed)
{
    struct xkb_context *ctx;
    ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES | XKB_CONTEXT_NO_ENVIRONMENT_NAMES);
    assert(ctx);

    /* Parse */
    size_t len = Size;
    /* Allow a zero-terminated string as a buffer */
    if (len > 0 && Data[len - 1] == '\0')
        len--;
    XkbFile *xkb_file =
        XkbParseString(ctx, (const char*)Data, len, "(input string)", NULL);

    if (!xkb_file) {
err:
        xkb_context_unref(ctx);
        if (Size > sizeof(dummy))
            return 0;
        memcpy(Data, dummy, sizeof(dummy));
        return sizeof(dummy);
    }

    if (xkb_file->file_type != FILE_TYPE_KEYMAP) {
        FreeXkbFile(xkb_file);
        goto err;
    }

    /* Mutate the atom table */
    // TODO
    // size_t k = 0;
    // char **string;
    // darray_enumerate(k, string, ctx->atom_table->strings) {
    //     if (! (*string))
    //         continue;
    //     len = strlen(*string);
    //     len = LLVMFuzzerMutate((uint8_t *)(*string), len, len);
    //     (*string)[len] = '\0';
    // }

    /* Mutate AST */
    for (XkbFile *file = (XkbFile *)xkb_file->defs; file;
         file = (XkbFile *) file->common.next) {
        for (ParseCommon *stmt = file->defs; stmt; stmt = stmt->next)
            mutate_ast(stmt);
    }

    /* Compile */
    // TODO: serialize directly from AST?
    struct xkb_keymap *keymap = xkb_keymap_new(ctx,
                                               XKB_KEYMAP_FORMAT_TEXT_V1,
                                               XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        FreeXkbFile(xkb_file);
        goto err;
    }

    if (!CompileKeymap(xkb_file, keymap, MERGE_OVERRIDE)) {
        FreeXkbFile(xkb_file);
        xkb_keymap_unref(keymap);
        goto err;
    }

    FreeXkbFile(xkb_file);

    /* Serialize */
    char *dump =
        xkb_keymap_get_as_string(keymap, XKB_KEYMAP_USE_ORIGINAL_FORMAT);
    assert(dump);

    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);
    ctx = NULL;

    len = strlen(dump);
    if (len > Size) {
        free(dump);
        goto err;
    }

    memcpy(Data, dump, len);
    free(dump);
    return len;
}

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size);
extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_DEFAULT_INCLUDES |
                                              XKB_CONTEXT_NO_ENVIRONMENT_NAMES);
    assert(ctx);
    struct xkb_keymap *keymap =
        xkb_keymap_new_from_buffer(ctx, (const char*)Data, Size,
                                   XKB_KEYMAP_FORMAT_TEXT_V1,
                                   XKB_KEYMAP_COMPILE_NO_FLAGS);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);

    /* Values other than 0 and -1 are reserved for future use. */
    return EXIT_SUCCESS;
}
