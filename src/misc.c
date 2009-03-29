/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "X11/extensions/XKBcommon.h"
#include "XKBcommonint.h"

#define	mapSize(m)	(sizeof(m)/sizeof(XkbKTMapEntryRec))
static  XkbKTMapEntryRec map2Level[]= {
  { True, ShiftMask, {1, ShiftMask, 0} }
};

static  XkbKTMapEntryRec mapAlpha[]= {
  { True, ShiftMask, { 1, ShiftMask, 0 } },
  { True, LockMask,  { 0,  LockMask, 0 } }
};

static	XkbModsRec preAlpha[]= {
	{        0,        0, 0 },
	{ LockMask, LockMask, 0 }
};

#define	NL_VMOD_MASK	0
static  XkbKTMapEntryRec mapKeypad[]= {
	{ True,	ShiftMask, { 1, ShiftMask,            0 } },
	{ False,        0, { 1,         0, NL_VMOD_MASK } }
};

static	XkbKeyTypeRec	canonicalTypes[XkbNumRequiredTypes] = {
	{ { 0, 0, 0 },
	  1,	/* num_levels */
	  0,	/* map_count */
	  NULL,		NULL,
	  None,		NULL
	},
	{ { ShiftMask, ShiftMask, 0 },
	  2,	/* num_levels */
	  mapSize(map2Level),	/* map_count */
	  map2Level,	NULL,
	  None,		NULL
	},
	{ { ShiftMask|LockMask, ShiftMask|LockMask, 0 },
	  2,				/* num_levels */
	  mapSize(mapAlpha),		/* map_count */
	  mapAlpha,	preAlpha,
	  None,		NULL
	},
	{ { ShiftMask, ShiftMask, NL_VMOD_MASK },
	  2,				/* num_levels */
	  mapSize(mapKeypad),		/* map_count */
	  mapKeypad,	NULL,
	  None,		NULL
	}
};

int
XkbcInitCanonicalKeyTypes(XkbcDescPtr xkb,unsigned which,int keypadVMod)
{
XkbClientMapPtr	map;
XkbKeyTypePtr	from,to;
int		rtrn;

    if (!xkb)
	return BadMatch;
    rtrn= XkbcAllocClientMap(xkb,XkbKeyTypesMask,XkbNumRequiredTypes);
    if (rtrn!=Success)
	return rtrn;
    map= xkb->map;
    if ((which&XkbAllRequiredTypes)==0)
	return Success;
    rtrn= Success;
    from= canonicalTypes;
    to= map->types;
    if (which&XkbOneLevelMask)
	rtrn= XkbcCopyKeyType(&from[XkbOneLevelIndex],&to[XkbOneLevelIndex]);
    if ((which&XkbTwoLevelMask)&&(rtrn==Success))
	rtrn= XkbcCopyKeyType(&from[XkbTwoLevelIndex],&to[XkbTwoLevelIndex]);
    if ((which&XkbAlphabeticMask)&&(rtrn==Success))
	rtrn= XkbcCopyKeyType(&from[XkbAlphabeticIndex],&to[XkbAlphabeticIndex]);
    if ((which&XkbKeypadMask)&&(rtrn==Success)) {
	XkbKeyTypePtr type;
	rtrn= XkbcCopyKeyType(&from[XkbKeypadIndex],&to[XkbKeypadIndex]);
	type= &to[XkbKeypadIndex];
	if ((keypadVMod>=0)&&(keypadVMod<XkbNumVirtualMods)&&(rtrn==Success)) {
	    type->mods.vmods= (1<<keypadVMod);
	    type->map[0].active= True;
	    type->map[0].mods.mask= ShiftMask;
	    type->map[0].mods.real_mods= ShiftMask;
	    type->map[0].mods.vmods= 0;
	    type->map[0].level= 1;
	    type->map[1].active= False;
	    type->map[1].mods.mask= 0;
	    type->map[1].mods.real_mods= 0;
	    type->map[1].mods.vmods= (1<<keypadVMod);
	    type->map[1].level= 1;
	}
    }
    return Success;
}

Bool
XkbcVirtualModsToReal(XkbcDescPtr xkb,unsigned virtual_mask,unsigned *mask_rtrn)
{
register int i,bit;
register unsigned mask;

    if (xkb==NULL)
	return False;
    if (virtual_mask==0) {
	*mask_rtrn= 0;
	return True;
    }
    if (xkb->server==NULL)
	return False;
    for (i=mask=0,bit=1;i<XkbNumVirtualMods;i++,bit<<=1) {
	if (virtual_mask&bit)
	    mask|= xkb->server->vmods[i];
    }
    *mask_rtrn= mask;
    return True;
}
