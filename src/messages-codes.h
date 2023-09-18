// NOTE: This file has been generated automatically by “update-message-registry.py”.
//       Do not edit manually!

#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>

/**
 * @name Codes of the log messages
 *
 * @added 1.6.0
 *
 */
enum xkb_message_code {
    _XKB_LOG_MESSAGE_MIN_CODE = 34,
    /** Warn on malformed number literals */
    XKB_ERROR_MALFORMED_NUMBER_LITERAL = 34,
    /** Warn on unsupported modifier mask */
    XKB_ERROR_UNSUPPORTED_MODIFIER_MASK = 60,
    /** Warn on unrecognized keysyms */
    XKB_WARNING_UNRECOGNIZED_KEYSYM = 107,
    /** Warn if no key type can be inferred */
    XKB_WARNING_CANNOT_INFER_KEY_TYPE = 183,
    /** Warn when a group index is not supported */
    XKB_ERROR_UNSUPPORTED_GROUP_INDEX = 237,
    /** Warn if using an undefined key type */
    XKB_WARNING_UNDEFINED_KEY_TYPE = 286,
    /** Warn if a group name was defined for group other than the first one */
    XKB_WARNING_NON_BASE_GROUP_NAME = 305,
    /** Warn when a shift level is not supported */
    XKB_ERROR_UNSUPPORTED_SHIFT_LEVEL = 312,
    /** Warn if there are conflicting keysyms while merging keys */
    XKB_WARNING_CONFLICTING_KEY_SYMBOL = 461,
    /** TODO: add description */
    XKB_WARNING_EXTRA_SYMBOLS_IGNORED = 516,
    /** Warn when a field has not the expected type */
    XKB_ERROR_WRONG_FIELD_TYPE = 578,
    /** Warn on unknown escape sequence in string literal */
    XKB_WARNING_UNKNOWN_CHAR_ESCAPE_SEQUENCE = 645,
    /** Warn if a key defines multiple groups at once */
    XKB_WARNING_MULTIPLE_GROUPS_AT_ONCE = 700,
    /** The syntax is invalid and the file cannot be parsed */
    XKB_ERROR_INVALID_SYNTAX = 769,
    /** TODO: add description */
    XKB_WARNING_UNDEFINED_KEYCODE = 770,
    /** Warn if there are conflicting modmap definitions */
    XKB_WARNING_CONFLICTING_MODMAP = 800,
    /** Warn if there are conflicting actions while merging keys */
    XKB_WARNING_CONFLICTING_KEY_ACTION = 883,
    /** Warn if there are conflicting key types while merging groups */
    XKB_WARNING_CONFLICTING_KEY_TYPE = 893,
    /** Warn if there are conflicting fields while merging keys */
    XKB_WARNING_CONFLICTING_KEY_FIELDS = 935,
    /** Warn if using a symbol not defined in the keymap */
    XKB_WARNING_UNRESOLVED_KEYMAP_SYMBOL = 965,
    _XKB_LOG_MESSAGE_MAX_CODE = 965
};

typedef uint32_t xkb_message_code_t;

#endif
