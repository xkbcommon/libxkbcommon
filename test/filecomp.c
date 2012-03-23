/*
Copyright 2009 Dan Nicholson

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the authors or their
institutions shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the authors.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <X11/X.h>
#include <X11/Xdefs.h>
#include "xkbcommon/xkbcommon.h"
#include "utils.h"

static char buffer[8192];

int main(int argc, char *argv[])
{
    char *path;
    int fd;
    struct xkb_desc * xkb;
    int i, len, from_string = 0;

    /* Require xkb file */
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\n");
        fprintf(stderr, "Usage: %s [-s] XKBFILE\n",
                argv[0]);
        exit(1);
    }

    i = 1;
    if (strcmp(argv[1], "-s") == 0) {
	from_string = 2;
	i++;
    }

    path = argv[i];

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open file \"%s\": %s\n", path,
                strerror(errno));
        exit(1);
    }

    if (from_string) {
	len = read(fd, buffer, sizeof(buffer));
	buffer[len] = '\0';
	xkb = xkb_map_new_from_string(buffer, XKB_KEYMAP_FORMAT_TEXT_V1);
    } else {
	xkb = xkb_map_new_from_fd(fd, XKB_KEYMAP_FORMAT_TEXT_V1);
    }
    close(fd);

    if (!xkb) {
        fprintf(stderr, "Failed to compile keymap\n");
        exit(1);
    }

    xkb_map_unref(xkb);

    return 0;
}
