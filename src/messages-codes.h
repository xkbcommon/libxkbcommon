// NOTE: This file has been generated automatically by “update-message-registry.py”.
//       Do not edit manually!
#pragma once

#include <stdint.h>

/*
 * Macro sorcery: PREPEND_MESSAGE_ID enables the log functions to format messages
 * with the message ID only if the ID is not 0 (XKB_LOG_MESSAGE_NO_ID).
 * This avoid checking the ID value at run time.
 *
 * The trick resides in CHECK_ID:
 * • CHECK_ID(0) expands to:
 *   ‣ SECOND(MATCH0, WITH_ID, unused)
 *   ‣ SECOND(unused,WITHOUT_ID, WITH_ID, unused)
 *   ‣ WITHOUT_ID
 * • CHECK_ID(123) expands to:
 *   ‣ SECOND(MATCH123, WITH_ID, unused)
 *   ‣ WITH_ID
*/
#define EXPAND(...)              __VA_ARGS__ /* needed for MSVC compatibility */

#define JOIN_EXPAND(a, b)        a##b
#define JOIN(a, b)               JOIN_EXPAND(a, b)

#define SECOND_EXPAND(a, b, ...) b
#define SECOND(...)              EXPAND(SECOND_EXPAND(__VA_ARGS__))

#define MATCH0                   unused,WITHOUT_ID
#define CHECK_ID(value)          SECOND(JOIN(MATCH, value), WITH_ID, unused)

#define FORMAT_MESSAGE_WITHOUT_ID(id, fmt) fmt
#define FORMAT_MESSAGE_WITH_ID(id, fmt)    "[XKB-%03d] " fmt, id
#define PREPEND_MESSAGE_ID(id, fmt) JOIN(FORMAT_MESSAGE_, CHECK_ID(id))(id, fmt)

/**
 * Special case when no message identifier is defined.
 */
#define XKB_LOG_MESSAGE_NO_ID 0

/**
 * @name Codes of the log messages
 */
enum xkb_message_code {
    _XKB_LOG_MESSAGE_MIN_CODE = 34,
    /** Warn on malformed number literals */
    XKB_ERROR_MALFORMED_NUMBER_LITERAL = 34,
    /** Conflicting “preserve” entries in a key type */
    XKB_WARNING_CONFLICTING_KEY_TYPE_PRESERVE_ENTRIES = 43,
    /** Warn on unsupported modifier mask */
    XKB_ERROR_UNSUPPORTED_MODIFIER_MASK = 60,
    /** Expected an array entry, but the index is missing */
    XKB_ERROR_EXPECTED_ARRAY_ENTRY = 77,
    /** Invalid numeric keysym */
    XKB_ERROR_INVALID_NUMERIC_KEYSYM = 82,
    /** Illegal keycode alias with the name of a real key */
    XKB_WARNING_ILLEGAL_KEYCODE_ALIAS = 101,
    /** Warn on unrecognized keysyms */
    XKB_WARNING_UNRECOGNIZED_KEYSYM = 107,
    /** A virtual modifier is used before being declared */
    XKB_ERROR_UNDECLARED_VIRTUAL_MODIFIER = 123,
    /** A buffer has an insufficient size */
    XKB_ERROR_INSUFFICIENT_BUFFER_SIZE = 134,
    /** The type of the statement is not allowed in the context */
    XKB_ERROR_WRONG_STATEMENT_TYPE = 150,
    /** The given path is invalid */
    XKB_ERROR_INVALID_PATH = 161,
    /** Geometry sections are not supported */
    XKB_WARNING_UNSUPPORTED_GEOMETRY_SECTION = 172,
    /** Warn if no key type can be inferred */
    XKB_WARNING_CANNOT_INFER_KEY_TYPE = 183,
    /** Invalid escape sequence in a string */
    XKB_WARNING_INVALID_ESCAPE_SEQUENCE = 193,
    /** The result of a key type “preserve” entry must be a subset of its input modifiers. */
    XKB_WARNING_ILLEGAL_KEY_TYPE_PRESERVE_RESULT = 195,
    /** Syntax error in the include statement */
    XKB_ERROR_INVALID_INCLUDE_STATEMENT = 203,
    /** A modmap entry is invalid */
    XKB_ERROR_INVALID_MODMAP_ENTRY = 206,
    /** Warn when a group index is not supported */
    XKB_ERROR_UNSUPPORTED_GROUP_INDEX = 237,
    /** The name of a key type level is defined multiple times. */
    XKB_WARNING_CONFLICTING_KEY_TYPE_LEVEL_NAMES = 239,
    /** Invalid statement setting default values */
    XKB_ERROR_INVALID_SET_DEFAULT_STATEMENT = 254,
    /** Conflicting “map” entries in type definition */
    XKB_WARNING_CONFLICTING_KEY_TYPE_MAP_ENTRY = 266,
    /** Warn if using an undefined key type */
    XKB_WARNING_UNDEFINED_KEY_TYPE = 286,
    /** A keysym has been deprecated: consider using an alternative keysym */
    XKB_WARNING_DEPRECATED_KEYSYM = 301,
    /** A keysym name has been deprecated: use the corresponding canonical name instead */
    XKB_WARNING_DEPRECATED_KEYSYM_NAME = 302,
    /** Warn if a group name was defined for group other than the first one */
    XKB_WARNING_NON_BASE_GROUP_NAME = 305,
    /** Warn when a shift level is not supported */
    XKB_ERROR_UNSUPPORTED_SHIFT_LEVEL = 312,
    /** Could not find a file used in an include statement */
    XKB_ERROR_INCLUDED_FILE_NOT_FOUND = 338,
    /** Use of an operator that is unknown and thus unsupported */
    XKB_ERROR_UNKNOWN_OPERATOR = 345,
    /** Use of a legacy X11 action that is not supported */
    XKB_WARNING_UNSUPPORTED_LEGACY_ACTION = 362,
    /** An entry is duplicated and will be ignored */
    XKB_WARNING_DUPLICATE_ENTRY = 378,
    /** Included files form cycle */
    XKB_ERROR_RECURSIVE_INCLUDE = 386,
    /** Conflicting definitions of a key type */
    XKB_WARNING_CONFLICTING_KEY_TYPE_DEFINITIONS = 407,
    /** A global defaults statement is in a wrong scope and should be moved */
    XKB_ERROR_GLOBAL_DEFAULTS_WRONG_SCOPE = 428,
    /** Missing default section in included file */
    XKB_WARNING_MISSING_DEFAULT_SECTION = 433,
    /** Warn if there are conflicting keysyms while merging keys */
    XKB_WARNING_CONFLICTING_KEY_SYMBOL = 461,
    /** The operation is invalid in the context */
    XKB_ERROR_INVALID_OPERATION = 478,
    /** Warn on numeric keysym (other than 0-9) */
    XKB_WARNING_NUMERIC_KEYSYM = 489,
    /** TODO: add description */
    XKB_WARNING_EXTRA_SYMBOLS_IGNORED = 516,
    /** Conflicting definitions of a key name or alias */
    XKB_WARNING_CONFLICTING_KEY_NAME = 523,
    /** Invalid file encoding */
    XKB_ERROR_INVALID_FILE_ENCODING = 542,
    /** Cannot allocate memory */
    XKB_ERROR_ALLOCATION_ERROR = 550,
    /** Warn when a field has not the expected type */
    XKB_ERROR_WRONG_FIELD_TYPE = 578,
    /** Cannot resolve a given (Rules, Model, Layout, Variant, Options) configuration */
    XKB_ERROR_CANNOT_RESOLVE_RMLVO = 595,
    /** Invalid Unicode escape sequence */
    XKB_WARNING_INVALID_UNICODE_ESCAPE_SEQUENCE = 607,
    /** Invalid _real_ modifier */
    XKB_ERROR_INVALID_REAL_MODIFIER = 623,
    /** Warn on unknown escape sequence in string literal */
    XKB_WARNING_UNKNOWN_CHAR_ESCAPE_SEQUENCE = 645,
    /** The target file of an include statement could not be processed */
    XKB_ERROR_INVALID_INCLUDED_FILE = 661,
    /** The Compose file syntax is invalid and the entry cannot be parsed */
    XKB_ERROR_INVALID_COMPOSE_SYNTAX = 685,
    /** A level has a different number of keysyms and actions */
    XKB_ERROR_INCOMPATIBLE_ACTIONS_AND_KEYSYMS_COUNT = 693,
    /** Warn if a key defines multiple groups at once */
    XKB_WARNING_MULTIPLE_GROUPS_AT_ONCE = 700,
    /** A legacy X11 symbol field is not supported */
    XKB_WARNING_UNSUPPORTED_SYMBOLS_FIELD = 711,
    /** The XKB syntax is invalid and the file cannot be parsed */
    XKB_ERROR_INVALID_XKB_SYNTAX = 769,
    /** Reference to an undefined keycode */
    XKB_WARNING_UNDEFINED_KEYCODE = 770,
    /** An expression has not the expected type */
    XKB_ERROR_INVALID_EXPRESSION_TYPE = 784,
    /** A value is invalid and will be ignored */
    XKB_ERROR_INVALID_VALUE = 796,
    /** Warn if there are conflicting modmap definitions */
    XKB_WARNING_CONFLICTING_MODMAP = 800,
    /** A field is unknown and will be ignored */
    XKB_ERROR_UNKNOWN_FIELD = 812,
    /** Keymap compilation failed */
    XKB_ERROR_KEYMAP_COMPILATION_FAILED = 822,
    /** Unknown action type */
    XKB_ERROR_UNKNOWN_ACTION_TYPE = 844,
    /** Warn if there are conflicting actions while merging keys */
    XKB_WARNING_CONFLICTING_KEY_ACTION = 883,
    /** Warn if there are conflicting key types while merging groups */
    XKB_WARNING_CONFLICTING_KEY_TYPE_MERGING_GROUPS = 893,
    /** Conflicting symbols entry for a key */
    XKB_ERROR_CONFLICTING_KEY_SYMBOLS_ENTRY = 901,
    /** Missing group index in a group name entry */
    XKB_WARNING_MISSING_SYMBOLS_GROUP_NAME_INDEX = 903,
    /** Warn if there are conflicting fields while merging keys */
    XKB_WARNING_CONFLICTING_KEY_FIELDS = 935,
    /** An identifier is used but is not built-in */
    XKB_ERROR_INVALID_IDENTIFIER = 949,
    /** Warn if using a symbol not defined in the keymap */
    XKB_WARNING_UNRESOLVED_KEYMAP_SYMBOL = 965,
    /** The rules syntax is invalid and the file cannot be parsed */
    XKB_ERROR_INVALID_RULES_SYNTAX = 967,
    /** Some modifiers used in a key type “map” or “preserve” entry are not declared */
    XKB_WARNING_UNDECLARED_MODIFIERS_IN_KEY_TYPE = 971,
    _XKB_LOG_MESSAGE_MAX_CODE = 971
};

typedef uint32_t xkb_message_code_t;
