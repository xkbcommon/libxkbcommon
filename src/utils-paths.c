/*
 * Copyright © 2024 Pierre Le Marre
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#include "utils.h"
#include "utils-paths.h"


/* Caller must ensure that the input is not NULL or empty */
bool
is_absolute(const char *path)
{
#ifdef _WIN32
    /*
     * A file name is relative to the current directory if it does not begin with
     * one of the following:
     * - A UNC name of any format, which always start with two backslash characters ("\\").
     * - A disk designator with a backslash, for example "C:\" or "d:\".
     * - A single backslash, for example, "\directory" or "\file.txt".
     * See: https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file
     */
    return is_path_separator(path[0]) ||
           (strlen_safe(path) >= 3 && path[1] == ':' && is_path_separator(path[2]));
#else
    return is_path_separator(path[0]);
#endif
}
