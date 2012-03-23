
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

#include "utils.h"
#include "XKBcommonint.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

void *
recalloc(void *ptr, size_t old_size, size_t new_size)
{
    char *rtrn;

    if (ptr == NULL)
        return calloc(1, new_size);

    rtrn = realloc(ptr, new_size);
    if (rtrn && new_size > old_size)
        memset(&rtrn[old_size], 0, new_size - old_size);

    return rtrn;
}

static FILE *errorFile = NULL;
static int outCount = 0;
static char *preMsg = NULL;
static char *prefix = NULL;

Bool
uSetErrorFile(char *name)
{
    if ((errorFile != NULL) && (errorFile != stderr))
    {
        fprintf(errorFile, "switching to %s\n", name ? name : "stderr");
        fclose(errorFile);
    }
    if (name != NULL)
        errorFile = fopen(name, "w");
    else
        errorFile = stderr;
    if (errorFile == NULL)
    {
        errorFile = stderr;
        return (False);
    }
    return (True);
}

void
uInformation(const char *s, ...)
{
    va_list args;

    if (!errorFile)
        errorFile = stderr;

    va_start(args, s);
    vfprintf(errorFile, s, args);
    va_end(args);
    fflush(errorFile);
}

/***====================================================================***/

void
uAction(const char *s, ...)
{
    va_list args;

    if (!errorFile)
        errorFile = stderr;

    if (prefix != NULL)
        fprintf(errorFile, "%s", prefix);
    fprintf(errorFile, "                  ");
    va_start(args, s);
    vfprintf(errorFile, s, args);
    va_end(args);
    fflush(errorFile);
}

/***====================================================================***/

void
uWarning(const char *s, ...)
{
    va_list args;

    if (!errorFile)
        errorFile = stderr;

    if ((outCount == 0) && (preMsg != NULL))
        fprintf(errorFile, "%s\n", preMsg);
    if (prefix != NULL)
        fprintf(errorFile, "%s", prefix);
    fprintf(errorFile, "Warning:          ");
    va_start(args, s);
    vfprintf(errorFile, s, args);
    va_end(args);
    fflush(errorFile);
    outCount++;
}

/***====================================================================***/

void
uError(const char *s, ...)
{
    va_list args;

    if (!errorFile)
        errorFile = stderr;

    if ((outCount == 0) && (preMsg != NULL))
        fprintf(errorFile, "%s\n", preMsg);
    if (prefix != NULL)
        fprintf(errorFile, "%s", prefix);
    fprintf(errorFile, "Error:            ");
    va_start(args, s);
    vfprintf(errorFile, s, args);
    va_end(args);
    fflush(errorFile);
    outCount++;
}

/***====================================================================***/

void
uFatalError(const char *s, ...)
{
    va_list args;

    if (!errorFile)
        errorFile = stderr;

    if ((outCount == 0) && (preMsg != NULL))
        fprintf(errorFile, "%s\n", preMsg);
    if (prefix != NULL)
        fprintf(errorFile, "%s", prefix);
    fprintf(errorFile, "Fatal Error:      ");
    va_start(args, s);
    vfprintf(errorFile, s, args);
    va_end(args);
    fprintf(errorFile, "                  Exiting\n");
    fflush(errorFile);
    outCount++;
    exit(1);
    /* NOTREACHED */
}

/***====================================================================***/

void
uInternalError(const char *s, ...)
{
    va_list args;

    if (!errorFile)
        errorFile = stderr;

    if ((outCount == 0) && (preMsg != NULL))
        fprintf(errorFile, "%s\n", preMsg);
    if (prefix != NULL)
        fprintf(errorFile, "%s", prefix);
    fprintf(errorFile, "Internal error:   ");
    va_start(args, s);
    vfprintf(errorFile, s, args);
    va_end(args);
    fflush(errorFile);
    outCount++;
}
