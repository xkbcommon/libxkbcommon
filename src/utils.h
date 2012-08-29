#ifndef UTILS_H
#define UTILS_H 1

/*\
 *
 *                          COPYRIGHT 1990
 *                    DIGITAL EQUIPMENT CORPORATION
 *                       MAYNARD, MASSACHUSETTS
 *                        ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED
 * WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT
 * RIGHTS, APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN
 * ADDITION TO THAT SET FORTH ABOVE.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Digital Equipment Corporation not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 \*/

#include <stdbool.h>
#include <string.h>

/*
 * We sometimes malloc strings and then expose them as const char*'s. This
 * macro is used when we free these strings in order to avoid -Wcast-qual
 * errors.
 */
#define UNCONSTIFY(const_ptr)  ((void *) (uintptr_t) (const_ptr))

static inline bool
streq(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0;
}

static inline bool
streq_not_null(const char *s1, const char *s2)
{
    if (!s1 || !s2)
        return false;
    return streq(s1, s2);
}

static inline bool
istreq(const char *s1, const char *s2)
{
    return strcasecmp(s1, s2) == 0;
}

static inline bool
istreq_prefix(const char *s1, const char *s2)
{
    return strncasecmp(s1, s2, strlen(s1)) == 0;
}

static inline char *
strdup_safe(const char *s)
{
    return s ? strdup(s) : NULL;
}

static inline bool
isempty(const char *s)
{
    return s == NULL || s[0] == '\0';
}

static inline const char *
strnull(const char *s)
{
    return s ? s : "(null)";
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Compiler Attributes */

#if defined(__GNUC__) && (__GNUC__ >= 4) && !defined(__CYGWIN__)
# define XKB_EXPORT      __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)
# define XKB_EXPORT      __global
#else /* not gcc >= 4 and not Sun Studio >= 8 */
# define XKB_EXPORT
#endif

#if defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 203)
# define ATTR_PRINTF(x,y) __attribute__((__format__(__printf__, x, y)))
#else /* not gcc >= 2.3 */
# define ATTR_PRINTF(x,y)
#endif

#if (defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 205)) \
    || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
# define ATTR_NORETURN __attribute__((__noreturn__))
#else
# define ATTR_NORETURN
#endif /* GNUC  */

#if (defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 296)
#define ATTR_MALLOC  __attribute__((__malloc__))
#else
#define ATTR_MALLOC
#endif

#if defined(__GNUC__) && (__GNUC__ >= 4)
# define ATTR_NULL_SENTINEL __attribute__((__sentinel__))
#else
# define ATTR_NULL_SENTINEL
#endif /* GNUC >= 4 */

#endif /* UTILS_H */
