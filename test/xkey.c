#include "xkbmisc.h"
#include "xkbcommon/xkbcommon.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void print_keysym(const char *s)
{
    KeySym ks = xkb_string_to_keysym(s);
    if (ks == NoSymbol)
        printf("NoSymbol\n");
    else
        printf("0x%lX\n", ks);
}

static void print_string(KeySym ks)
{
    char s[16];

    xkb_keysym_to_string(ks, s, sizeof s);
    printf("%s\n", s);
}

int main(int argc, char *argv[])
{
    int mode;
    KeySym sym;

    if (argc < 3) {
        fprintf(stderr, "error: not enough arguments\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "-k") == 0) {
        mode = 0;
        sym = strtoul(argv[2], NULL, 16);
    }
    else if (strcmp(argv[1], "-s") == 0)
        mode = 1;
    else {
        fprintf(stderr, "error: unrecognized argument \"%s\"\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if (mode == 0)
        print_string(sym);
    else
        print_keysym(argv[2]);

    return 0;
}
