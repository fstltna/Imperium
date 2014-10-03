/*
 * Imperium
 *
 * Feel free to modify and use these sources however you wish, so long
 * as you preserve this copyright notice.
 *
 * $Id: feSupp.c,v 1.2 2000/05/18 06:50:02 marisa Exp $
 *
 * $Log: feSupp.c,v $
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:52  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:59:19  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:24:30  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:21  marisag
 * patch20: Fix empty revision.
 *
 */

#include "../config.h"

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "../Include/ImpFeMess.h"
#include "Scan.h"
#include "ImpPrivate.h"

static const char rcsid[] = "$Id: feSupp.c,v 1.2 2000/05/18 06:50:02 marisa Exp $";

void fePlDirty(IMP, ULONG plNum)
{
    if (IS->is_player.p_feMode & FE_WANT_PLAN)
    {
        user(IS, FE_PLDIRTY);
        userX(IS, plNum, 8);
        userNL(IS);
    }
}

void feShDirty(IMP, ULONG shNum)
{
    if (IS->is_player.p_feMode & FE_WANT_SHIP)
    {
        user(IS, FE_SHDIRTY);
        userX(IS, shNum, 8);
        userNL(IS);
    }
}

void fePowerReport(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_MISC)
    {
        user(IS, FE_POWREP);
    }
}

void feIteCen(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_PLAN)
    {
        user(IS, FE_PLITE);
    }
}

void feProCen(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_PLAN)
    {
        user(IS, FE_PLPRO);
    }
}

void feProCen2(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_PLAN)
    {
        user(IS, FE_PLPRO2);
    }
}

void feGeoCen(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_PLAN)
    {
        user(IS, FE_PLGEO);
    }
}

void fePopCen(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_PLAN)
    {
        user(IS, FE_PLPOP);
    }
}

void feBigCen(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_PLAN)
    {
        user(IS, FE_PLBIG);
    }
}

void feCheckReq(IMP, ULONG plNum)
{
    if (IS->is_player.p_feMode & FE_WANT_MISC)
    {
        user(IS, FE_CHREQ);
        userX(IS, plNum, 8);
        userNL(IS);
    }
}

void feShBig(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_SHIP)
    {
        user(IS, FE_SHBIG);
    }
}

void feShStat(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_SHIP)
    {
        user(IS, FE_SHSTAT);
    }
}

void feShConf(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_SHIP)
    {
        user(IS, FE_SHCONF);
    }
}

void feShCargo(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_SHIP)
    {
        user(IS, FE_SHCARGO);
    }
}

void feRaceRep(IMP, USHORT line)
{
    if (IS->is_player.p_feMode & FE_WANT_MISC)
    {
	if (line == 1)
	{
            user(IS, FE_RACEREP);
	}
	else
	{
            user(IS, FE_RACEREP2);
	}
    }
}

void fePlayStat(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_MISC)
    {
        user(IS, FE_PLAYSTAT);
    }
}

void fePlayList(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_MISC)
    {
        user(IS, FE_PLAYLIST);
    }
}

void feWorldSize(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_MISC)
    {
        user(IS, FE_WSIZE);
    }
}

void fePrintRealm(IMP)
{
    if (IS->is_player.p_feMode & FE_WANT_MISC)
    {
        user(IS, FE_PRINTREALM);
    }
}

