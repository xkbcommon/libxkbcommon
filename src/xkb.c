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

Bool
XkbcComputeEffectiveMap(	XkbcDescPtr 	xkb,
			XkbKeyTypePtr 	type,
			unsigned char *	map_rtrn)
{
register int 		i;
unsigned     		tmp;
XkbKTMapEntryPtr	entry = NULL;

    if ((!xkb)||(!type)||(!xkb->server))
	return False;

    if (type->mods.vmods!=0) {
	if (!XkbcVirtualModsToReal(xkb,type->mods.vmods,&tmp))
	    return False;

	type->mods.mask= tmp|type->mods.real_mods;
	entry= type->map;
	for (i=0;i<type->map_count;i++,entry++) {
	    tmp= 0;
	    if (entry->mods.vmods!=0) {
		if (!XkbcVirtualModsToReal(xkb,entry->mods.vmods,&tmp))
		    return False;
		if (tmp==0) {
		    entry->active= False;
		    continue;
		}
	    }
	    entry->active= True;
	    entry->mods.mask= (entry->mods.real_mods|tmp)&type->mods.mask;
	}
    }
    else {
	type->mods.mask= type->mods.real_mods;
    }
    if (map_rtrn!=NULL) {
	bzero(map_rtrn,type->mods.mask+1);
	for (i=0;i<type->map_count;i++) {
	    if (entry->active) {
		map_rtrn[type->map[i].mods.mask]= type->map[i].level;
	    }
	}
    }
    return True;
}
