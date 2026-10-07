/*
 * Copyright © 2026 Pierre Le Marre
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "config.h"

#include <stdlib.h>

#if !HAVE_RANDOM

/* Simple random() fallback based on rand() */

#include <stdint.h>

static inline void
srandom(unsigned int seed)
{
    srand(seed);
}

static inline long
random(void)
{
    /* rand() is only guaranteed to cover [0, 32767] (15 bits), so
     * combine three calls to safely fill 31 bits regardless of the
     * platform's actual RAND_MAX. */
    const long r = ((long)rand() & 0x7fffU)
                 | (((long)rand() & 0x7fffU) << 15)
                 | (((long)rand() & 0x7fffU) << 30);

    return r & 0x7fffffffL;
}
#endif
