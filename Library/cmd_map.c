/*
 * Imperium
 *
 * $Id: cmd_map.c,v 1.3 2000/05/25 23:46:17 marisa Exp $
 *
 * Code related to displaying sector maps and sensor scans
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: cmd_map.c,v $
 * Revision 1.3  2000/05/25 23:46:17  marisa
 * Now always displays 4 digit row numbers
 *
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:50  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:59:10  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:24:17  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:09  marisag
 * patch20: Fix empty revision.
 *
 */

#include "../config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdio.h>
#include <time.h>

#define CmdMapC 1
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "../Include/ImpFeMess.h"
#include "Scan.h"
#include "ImpPrivate.h"

static const char rcsid[] = "$Id: cmd_map.c,v 1.3 2000/05/25 23:46:17 marisa Exp $";

#define MAX_RANGE   20
#define SCAN_COST   2   /* cost in energy per scan */

/*
 * digit printing routines for mapping.
 */

/* digit0 - prints the 4th digit of a 4 digit row or column number
 *          Expects scaled numbers
 */

void digit0(IMP, register USHORT x)
{
    userC(IS, (char) (x / 1000) + '0');
}

/* colDigit1 - print the 3rd digit of a column number
 *          Expects scaled numbers
 */

void colDigit1(IMP, register USHORT x)
{
    if (IS->is_BOOL4)
    {
        userC(IS, (char) ((x / 100) % 10) + '0');
    }
    else
    {
        userC(IS, (char) (x / 100) + '0');
    }
}

/* digit2 - print the 2nd digit of a row or column number
 *          Expects scaled numbers
 */

void digit2(IMP, register USHORT x)
{
    userC(IS, (char) ((x / 10) % 10) + '0');
}

/* digit3 - print the low digit of a row or column number
 *          Expects scaled numbers
 */

void digit3(IMP, register USHORT x)
{
    userC(IS, (char) (x % 10) + '0');
}

/* doDigit - print a line of one digit of the column numbers
 *          Expects scaled numbers
 */

void doDigit(IMP, USHORT which, USHORT left, USHORT right,
    USHORT incVal)
{
    register USHORT c;

    user(IS, "    ");
    if (IS->is_USHORT4)
    {
        userSp(IS);
    }
    if (IS->is_BOOL2)
    {
        userSp(IS);
    }
    for (c = left; c <= right; c += incVal)
    {
        if (!IS->is_BOOL1)
        {
            userSp(IS);
        }
        switch(which)
        {
            case 0:
                digit0(IS, c);
                break;
            case 1:
                colDigit1(IS, c);
                break;
            case 2:
                digit2(IS, c);
                break;
            case 3:
                digit3(IS, c);
                break;
            default:
                break;
        }
    }
    userNL(IS);
}

/*
 * coords - print 2 or 3 lines of column numbers. Expects scaled
 *          row & col numbers
 */

void coords(IMP, USHORT left, USHORT right,
    USHORT incVal)
{
    if (IS->is_BOOL4)
    {
        doDigit(IS, 0, left, right, incVal);
    }
    if (IS->is_BOOL3)
    {
        doDigit(IS, 1, left, right, incVal);
    }
    doDigit(IS, 2, left, right, incVal);
    doDigit(IS, 3, left, right, incVal);
}

/*
 * doMap - part of cmd_map - called by scanning code
 */

void doMap(IMP, USHORT r, USHORT c, register Sector_t *s)
{
    register SectorType_t type;
    register char ch;

    type = s->s_type;
    ch = IS->is_sectorChar[type];
    if (!IS->is_BOOL1)
    {
        userSp(IS);
    }
    userC(IS, ch);
}

/*
 * special routines called if we set the map hook.
 */

/* setSizes - determine row and column number sizes based on the range
 *          given to us by the scanning code. Expects scaled numbers
 */

void setSizes(IMP, USHORT top, USHORT bottom, USHORT left, USHORT right)
{
    IS->is_BOOL2 = ((top >= 100) || (bottom >= 100));
    IS->is_USHORT4 = ((top >= 1000) || (bottom >= 1000));
    IS->is_BOOL3 = ((left >= 100) || (right >= 100));
    IS->is_BOOL4 = ((left >= 1000) || (right >= 1000));
}

/* mapCoords - print header or trailer column numbers for a map
 *          Expects scaled numbers
 */

void mapCoords(IMP, USHORT left, USHORT right)
{
    coords(IS, left, right, 1);
}

/* digits - print 2 to 4 digits of a row number
 *          Expects scaled numbers
 */

void digits(IMP, USHORT r)
{
    char numBuf[5];

    sprintf(numBuf, "%04u", r);
    user(IS, numBuf);
}

/* mapRowStart - scanning code calls this to print beginning of map line
 *          Expects scaled numbers
 */

void mapRowStart(IMP, USHORT r)
{
    digits(IS, r);
}

/* mapRowEnd - scanning code calls this to print end of map line
 *          Expects scaled numbers
 */

void mapRowEnd(IMP, USHORT r)
{
    if (!IS->is_BOOL1)
    {
        userSp(IS);
    }
    digits(IS, r);
    userNL(IS);
}

/* mapEmpty - scanning code calls this to print an unknown map sector
 */

void mapEmpty(IMP)
{
    if (IS->is_BOOL1)
    {
        userSp(IS);
    }
    else
    {
        user(IS, "  ");
    }
}

/*
 * readRow - part of 'doSimpleMap' - Expects scaled row and col numbers
 */

void readRow(IMP, register USHORT r, register USHORT c1, USHORT c2,
    char pChar[])
{
    register Sector_t *rs;
    register USHORT c;
    register USHORT lMap, cMap;

    /* initialize the flag */
    lMap = 0xffff;
    rs = &IS->is_request.rq_u.ru_sector;
    for (c = c1; c <= c2; c++)
    {
        cMap = mapSector(IS, r, c);
        /* see if we need to read in the sector */
        if (lMap != cMap)
        {
            /* read the sector */
            server(IS, rt_readSector, cMap);
            /* set the last map flag the same as the current flag */
            lMap = cMap;
            pChar[(c / 10) - (c1 / 10)] = IS->is_sectorChar[rs->s_type];
        }
    }
}

/*
 * doSimpleMap - do mapping assuming there were no conditions on the region
 *      given. This means that all sectors we can see will be shown.
 */

void doSimpleMap(IMP, register SectorScan_t *ss)
{
    register USHORT r, c;
    USHORT count;
    register USHORT bott;
    char charNext[256 * 2 + 4], charThis[256 * 2 + 4];
    BOOL abort;
    USHORT top, left, right, lRow;

    top = ss->ss_cs.cs_boxTop;
    bott = ss->ss_cs.cs_boxBottom;
    left = ss->ss_cs.cs_boxLeft;
    if ((ss->ss_cs.cs_boxRight / 10) - (left / 10) >= 256 * 2 + 4)
    {
        /* if too large, trim it */
        ss->ss_cs.cs_boxRight = ss->ss_cs.cs_boxLeft + ((256 * 2 + 4 - 1)
            * 10);
    }
    right = ss->ss_cs.cs_boxRight;
    readRow(IS, top, left, right, charNext);
    setSizes(IS, top, bott, left, right);
    coords(IS, left, right, 1);
    if (!IS->is_BOOL1)
    {
       userNL(IS);
    }
    count = right - left + 1;
    r = top;
    abort = FALSE;
    lRow = 0xffff;
    while (!abort && (r <= bott))
    {
        if ((r / 10) != lRow)
        {
            lRow = r / 10;
            memcpy(charThis, charNext, (256 * 2 + 4) * sizeof(char));
            readRow(IS, r, left, right, charNext);
        }
        mapRowStart(IS, r);
        for (c = 0; c < count; c++)
        {
            if (!IS->is_BOOL1)
            {
                userSp(IS);
            }
            userC(IS, charThis[c / 10]);
        }
        mapRowEnd(IS, r);
        r++;
        abort = clGotCtrlC(IS);
    }
    if (!abort)
    {
        if (!IS->is_BOOL1)
        {
           userNL(IS);
        }
        coords(IS, left, right, 1);
    }
}

/* cmd_map - top level entry for mapping code */

BOOL cmd_map(IMP)
{
    SectorScan_t ss;
    char *p;

    if (reqSectors(IS, &ss, "Enter sectors specification for map"))
    {
        ss.ss_mapHook = FALSE;
        p = skipBlanks(IS);
        IS->is_BOOL1 = IS->is_player.p_compressed;
        if (*p != '\0')
        {
            *skipWord(IS) = '\0';
            if ((strcmp(p, "compressed") == 0) || (strcmp(p, "!") == 0))
            {
                IS->is_BOOL1 = !IS->is_BOOL1;
            }
            else
            {
                err(IS, "invalid flag on 'map' - only 'compressed' and "
                   "'!' allowed");
                /* Prevent problems */
                IS->is_BOOL1 = FALSE;
                return FALSE;
            }
        }
        if (ss.ss_cs.cs_conditionCount == 0)
        {
            doSimpleMap(IS, &ss);
        }
        else
        {
            ss.ss_mapHook = TRUE;
            setSizes(IS, ss.ss_cs.cs_boxTop, ss.ss_cs.cs_boxBottom,
                     ss.ss_cs.cs_boxLeft, ss.ss_cs.cs_boxRight);
            (void) scanSectors(IS, &ss, doMap);
        }
        /* Prevent problems */
        IS->is_BOOL1 = FALSE;
        return TRUE;
    }
    return FALSE;
}

/*
 * doLRScan - given a position and a range of subsectors (x 100), do a
 *          long-range sensor scan from there.
 */

void doLRScan(IMP, USHORT rRow, USHORT rCol, USHORT rang)
{
    register Sector_t *s;
    register USHORT r, c;
    char map[((MAX_RANGE * 2) * 2) + 1][((MAX_RANGE * 2) * 2) + 1];
    USHORT mapRow[((MAX_RANGE * 2) * 2) + 1];
    SectorType_t desigRow[((MAX_RANGE * 2) * 2) + 1];
    ULONG range1, rangeSq, distance;
    USHORT top, bottom, left, right, mapped, lastMapped;
    SectorType_t desig;
    char numBuf[25];

    /* initialize array */
    for (r = 0; r < ((MAX_RANGE * 2) * 2) + 1; r++)
    {
        for (c = 0; c < ((MAX_RANGE * 2) * 2) + 1; c++)
        {
            map[r][c] = '?';
        }
        mapRow[r] = 0xffff;
    }
    /* get initial range */
    range1 = umin(MAX_RANGE * 2, (USHORT) rang / 100);

    /* make sure rows and cols don't wrap around */
    if (rRow >= range1)
    {
        top = rRow - range1;
    }
    else
    {
        top = 0;
    }
    bottom = rRow + range1;
    if (bottom > (((IS->is_world.w_rows - 1) * 10) + 9))
    {
        bottom = ((IS->is_world.w_rows - 1) * 10) + 9;
    }
    if (rCol >= range1)
    {
        left = rCol - range1;
    }
    else
    {
        left = 0;
    }
    right = rCol + range1;
    if (right > (((IS->is_world.w_columns - 1) * 10) + 9))
    {
        right = ((IS->is_world.w_columns - 1) * 10) + 9;
    }

    rangeSq = ((ULONG)rang) * rang / (100 * 100);

    /* first, fill in the map with terrain values */
    s = &IS->is_request.rq_u.ru_sector;
    for (r = top; r <= bottom; r++)
    {
        for (c = left; c <= right; c++)
        {
            distance = findDistance(IS, r, c, rRow, rCol);
            if (distance <= rangeSq)
            {
                if (mapRow[(c - left) / 10] != (r / 10))
                {
                    mapRow[(c - left) / 10] = (r / 10);
                    server(IS, rt_readSector, mapSector(IS, r, c));
                    desigRow[(c - left) / 10] = s->s_type;
                }
                desig = desigRow[(c - left) / 10];
                if ((desig == s_supernova) || (distance <= rangeSq / 2))
                {
                    map[(r - top) / 10][(c - left) / 10] =
                        IS->is_sectorChar[desig];
                }
            }
        }
    }

    /* now display the resulting map */

    IS->is_BOOL1 = IS->is_player.p_compressed;
    setSizes(IS, (USHORT) ((top / 10) * 10), (USHORT) ((bottom / 10) * 10),
	(USHORT) ((left / 10) * 10), (USHORT) ((right / 10) * 10));
    coords(IS, (USHORT) ((left / 10) * 10), (USHORT) ((right / 10) * 10), (USHORT) 10);
    if (!IS->is_BOOL1)
    {
        userNL(IS);
    }
    mapped = 0xffff;
    sprintf(numBuf, "%04u:%04u", left, right);
    for (r = top; r <= bottom; r++)
    {
        if ((r / 10) != mapped)
        {
            mapped = r / 10;
            lastMapped = 0xffff;
            if (IS->is_player.p_feMode & FE_WANT_SCAN)
            {
                user2(IS, FE_LRS, numBuf);
            }
            digits(IS, (USHORT) mapped * 10);
            for (c = left; c <= right; c++)
            {
                if ((c / 10) != lastMapped)
                {
                    lastMapped = c / 10;
                    if (!IS->is_BOOL1)
                    {
                        userSp(IS);
                    }
                    userC(IS, map[(r - top) / 10][(c - left) / 10]);
                }
            }
            if (!IS->is_BOOL1)
            {
                userSp(IS);
            }
            digits(IS, (USHORT) mapped * 10);
            userNL(IS);
        }
    }
    if (!IS->is_BOOL1)
    {
        userNL(IS);
    }
    coords(IS, (USHORT) ((left / 10) * 10), (USHORT) ((right / 10) * 10), (USHORT) 10);
    /* Prevent problems */
    IS->is_BOOL1 = FALSE;
}

USHORT findPl(IMP, register ULONG plNum)
{
    register USHORT which;

    which = 0;
    while (which < IS->is_sectBuf.sb_plCount)
    {
        if (IS->is_sectBuf.sb_planet[which].sbp_number == plNum)
        {
            return which;
        }
        which++;
    }
    /* should NEVER get here */
    log3(IS, "tried to find an invalid planet number in findPl", "", "");
    return 0xffff;
}

/*
 * doSRScan - given a position and a range of subsectors (x 100), do a
 *          short-range sensor scan from there.
 */

void doSRScan(IMP, USHORT rRow, USHORT rCol, USHORT rang)
{
    register USHORT r, c, curR, curC;
    char map[((MAX_RANGE * 2) * 2) + 1][((MAX_RANGE * 2) * 2) + 1];
    Ship_t sh;
    ULONG range1, rangeSq, distance, visib;
    USHORT top, bottom, left, right, lastMapped;
    /* these hold the edges of the "window" we are looking at */
    USHORT curTop, curBottom, curLeft, curRight;
    ULONG shipNumber, plNum;
    char desig;
    SectorType_t sectorType = 0;
    char numBuf[25];

    /* initialize array */
    for (r = 0; r < ((MAX_RANGE * 2) * 2) + 1; r++)
    {
        for (c = 0; c < ((MAX_RANGE * 2) * 2) + 1; c++)
        {
            map[r][c] = '?';
        }
    }
    /* get initial range */
    range1 = umin(MAX_RANGE * 2, (USHORT) rang / 100);

    /* make sure rows and cols don't wrap around */
    if (rRow >= range1)
    {
        top = rRow - range1;
    }
    else
    {
        top = 0;
    }
    bottom = rRow + range1;
    if (bottom > (((IS->is_world.w_rows - 1) * 10) + 9))
    {
        bottom = ((IS->is_world.w_rows - 1) * 10) + 9;
    }
    if (rCol >= range1)
    {
        left = rCol - range1;
    }
    else
    {
        left = 0;
    }
    right = rCol + range1;
    if (right > (((IS->is_world.w_columns - 1) * 10) + 9))
    {
        right = ((IS->is_world.w_columns - 1) * 10) + 9;
    }

    /*
    Now we know the edges of the maximum range of the sensors, so we
    need to subdivide it up and fill in the map as we go
    */

    rangeSq = ((ULONG)rang) * rang / (100 * 100);

    /* first, fill in the map with terrain values */
    r = top;
    lastMapped = 0xffff;
    while(r <= bottom)
    {
        /* set up the rectangle */
        curTop = r;
        curBottom = r;
        /* loop until the bottom of the area or we cross a boundary */
        while ((curBottom <= bottom) && ((curBottom / 10) == (r / 10)))
        {
            curBottom++;
        }
        /*
        Ok, we now know we either are just beyond the "real" bottom, or
        we crossed a boundary. In either case we want to back up one
        */
        curBottom--;

        /* now do a similar thing to set up the columns */
        c = left;
        while(c <= right)
        {
            curLeft = c;
            curRight = c;
            /* loop until the right of the area or we cross a boundary */
            while ((curRight <= right) && ((curRight / 10) == (c / 10)))
            {
                curRight++;
            }
            /*
            Ok, we now know we either are just beyond the "real" right, or
            we crossed a boundary. In either case we want to back up one
            */
            curRight--;

            /* now loop for this square */
            for (curR = curTop; curR <= curBottom; curR++)
            {
                for (curC = curLeft; curC <= curRight; curC++)
                {
                    distance = findDistance(IS, curR, curC, rRow, rCol);
                    if (distance <= rangeSq)
                    {
                        plNum = whichPlanet(IS, curR, curC);
                        if (plNum != NO_ITEM)
                        {
                            if (distance <= rangeSq / 2)
                            {
                                desig = PLANET_CHAR[IS->is_sectBuf.sb_planet[
                                    findPl(IS, plNum)].sbp_class];
                            }
                            else
                            {
                                /* the planet is too far away to see class */
                                desig = '0';
                            }
                        }
                        else
                        {
                            if (lastMapped != IS->is_sectBuf.sb_sector)
                            {
                                server(IS, rt_readSector,
                                    IS->is_sectBuf.sb_sector);
                                sectorType =
                                    IS->is_request.rq_u.ru_sector.s_type;
                                lastMapped = IS->is_sectBuf.sb_sector;
                            }
                            if (sectorType == s_supernova)
                            {
                                desig = '$';
                            }
                            else
                            {
                                desig = ' ';
                            }
                        }
/*                        if ((desig == '$') || (distance <= rangeSq / 4))*/
                        map[curR - top][curC - left] = desig;
                    }
                }
            }
            c = curRight + 1;
        }
        r = curBottom + 1;
    }

    /* next, fill in all ships that are in range */

    if (IS->is_world.w_shipNext)
    {
        for (shipNumber = 0; shipNumber < IS->is_world.w_shipNext;
            shipNumber++)
        {
            server(IS, rt_readShip, shipNumber);
            sh = IS->is_request.rq_u.ru_ship;
            /* wrecks don't show up */
            /* Nor do miners that are being carried by other ships */
            if ((sh.sh_owner != NO_OWNER) && ((sh.sh_type != st_m) ||
                ((sh.sh_type == st_m) && (sh.sh_planet != 0))))
            {
                r = sh.sh_row;
                c = sh.sh_col;
                distance = findDistance(IS, r, c, rRow, rCol);
                visib = getShipVisib(IS, &sh);
                if (distance <= ((ULONG)rangeSq) * visib * visib / 1018)
                {
                    server(IS, rt_readPlayer, sh.sh_owner);
/*
    If we decide to hide ship owners names, then this is where it should be
    done
*/
                    user(IS, &IS->is_request.rq_u.ru_player.p_name[0]);
                    userSp(IS);
                    user(IS, getShipName(sh.sh_type));
                    userN3(IS, " #", shipNumber, " at ");
                    userN(IS, r);
                    userN3(IS, ",", c, "\n");
                    if (IS->is_player.p_feMode & FE_WANT_SCAN)
                    {
                        user(IS, FE_SHSCDET);
                        userX(IS, sh.sh_owner, 4);
                        userC(IS, SHIP_CHAR[sh.sh_type]);
                        userX(IS, shipNumber, 8);
                        userX(IS, r, 4);
                        userX(IS, c, 4);
                        userNL(IS);
                    }
                }
            }
        }
    }

    /* now display the resulting map */

    IS->is_BOOL1 = IS->is_player.p_compressed;
    setSizes(IS, top, bottom, left, right);
    coords(IS, left, right, 1);
    if (!IS->is_BOOL1)
    {
        userNL(IS);
    }
    sprintf(numBuf, "%04u:%04u", left, right);
    for (r = top; r <= bottom; r++)
    {
        if (IS->is_player.p_feMode & FE_WANT_SCAN)
        {
            user2(IS, FE_SRS, numBuf);
        }
        digits(IS, r);
        for (c = left; c <= right; c++)
        {
            if (!IS->is_BOOL1)
            {
                userSp(IS);
            }
            userC(IS, map[r - top][c - left]);
        }
        if (!IS->is_BOOL1)
        {
            userSp(IS);
        }
        digits(IS, r);
        userNL(IS);
    }
    if (!IS->is_BOOL1)
    {
        userNL(IS);
    }
    coords(IS, left, right, 1);
    /* Prevent problems */
    IS->is_BOOL1 = FALSE;
}

/*
 * getTechLevDiff - returns the difference (in tech factors) of the first
 *          tech level from the 2nd. Returns 0 if equal, a negative number
 *          if the 2nd is higher. Returns it as a tech factor.
 */

short getTechDiff(IMP, register ULONG lev1, register ULONG lev2)
{
    register short fact1, fact2;

    fact1 = (short) getTechFactor(lev1);
    fact2 = (short) getTechFactor(lev2);
    return (fact1 - fact2);
}

/*
 * giveShDetails - prints out varying details of a ship depending on the
 *          difference of tech factors
 *          Ship must NOT be in the buffer
 */

void giveShDetails(IMP, register Ship_t *sh, short difTf)
{
    register USHORT none;
    USHORT numEng, cargo, numWeap, eff, ETF, WTF, civ,
        mil, shield, energ, armour, temp;
	char shNameBuf[25], work[15];

    /* pre-initialize all the fields */
    none = NO_ITEM;
    numEng = none;
    cargo = none;
    numWeap = none;
    eff = none;
    ETF = none;
    WTF = none;
    civ = none;
    mil = none;
    shield = none;
    energ = none;
    armour = none;

    /* find out what to print */

    if (difTf > -10)
    {
        /* engines are pretty obvious */
        numEng = numInst(IS, sh, bp_engines);
        /* as is energy in a shield coil */
        shield = sh->sh_shields;
        /* weapons are not so obvious */
        numWeap = numInst(IS, sh, bp_photon);
        numWeap += numInst(IS, sh, bp_blaser);
        if (numWeap != 0)
        {
            if (difTf > 0)
            {
                if (difTf > 25)
                {
                    if (difTf < 51)
                    {
                        numWeap = ((numWeap / 2) * 2);
                    }
                }
                else
                {
                    numWeap = ((numWeap / 3) * 3);
                }
            }
            else
            {
                numWeap = ((numWeap / 5) * 5);
            }
            if (numWeap == 0)
            {
                numWeap = 1;
            }
        }
        cargo = sh->sh_cargo;
        eff = sh->sh_efficiency;
        civ = sh->sh_items[it_civilians] + sh->sh_items[it_scientists];
        mil = sh->sh_items[it_military] + sh->sh_items[it_officers];
        energ = sh->sh_energy;
        armour = sh->sh_armourLeft;
        ETF = sh->sh_engTF;
        WTF = getShipTechFactor(IS, sh, bp_photon);
        temp = WTF;
        WTF += getShipTechFactor(IS, sh, bp_blaser);
        if (WTF != temp)
        {
            WTF /= 2;
        }
        if (difTf > 25)
        {
            if (difTf < 50)
            {
                cargo = ((cargo / 10) * 10);
                civ = ((civ / 4) * 4);
                mil = ((mil / 4) * 4);
                energ = ((energ / 3) * 3);
                armour = ((armour / 3) * 3);
                ETF = ((ETF / 5) * 5);
                WTF = ((ETF / 6) * 6);
            }
        }
        else
        {
            eff = ((eff / 10) * 10);
            cargo = ((cargo / 15) * 15);
            civ = ((civ / 8) * 8);
            mil = ((mil / 8) * 8);
            energ = ((energ / 10) * 10);
            armour = ((armour / 10) * 10);
            ETF = ((ETF / 10) * 10);
            WTF = ((ETF / 15) * 15);
        }
    }

    /* print out the header */
    user(IS, 
"#Eng|#Weap| ETF | WTF | EFF | Carg | Civ | Mil | Shld|Energ| Armr\n");
    /* if each item has been set, then print it */
    if (numEng == none)
    {
        user(IS, "  ? ");
		numEng=0;
    }
    else
    {
        userF(IS, numEng, 4);
    }
    userC(IS, '|');
    if (numWeap == none)
    {
        user(IS, "  ?  ");
		numWeap=0;
    }
    else
    {
        userF(IS, numWeap, 5);
    }
    userC(IS, '|');
    if (ETF == none)
    {
        user(IS, "  ?  ");
		ETF=0;
    }
    else
    {
        userF(IS, ETF, 5);
    }
    userC(IS, '|');
    if (WTF == none)
    {
        user(IS, "  ?  ");
		WTF=0;
    }
    else
    {
        userF(IS, WTF, 5);
    }
    userC(IS, '|');
    if (eff == none)
    {
        user(IS, "  ?  ");
		eff=0;
    }
    else
    {
        userF(IS, eff, 5);
    }
    userC(IS, '|');
    if (cargo == none)
    {
        user(IS, "   ?  ");
		cargo=0;
    }
    else
    {
        userF(IS, cargo, 6);
    }
    userC(IS, '|');
    if (civ == none)
    {
        user(IS, "  ?  ");
		civ=0;
    }
    else
    {
        userF(IS, civ, 5);
    }
    userC(IS, '|');
    if (mil == none)
    {
        user(IS, "  ?  ");
		mil=0;
    }
    else
    {
        userF(IS, mil, 5);
    }
    userC(IS, '|');
    if (shield == none)
    {
        user(IS, "  ?  ");
		shield=0;
    }
    else
    {
        userF(IS, shield, 5);
    }
    userC(IS, '|');
    if (energ == none)
    {
        user(IS, "  ?  ");
		energ=0;
    }
    else
    {
        userF(IS, energ, 5);
    }
    userC(IS, '|');
    if (armour == none)
    {
        user(IS, "  ?  ");
		armour=0;
    }
    else
    {
        userF(IS, armour, 5);
    }
    userNL(IS);
    /* See about doing the FE stuff */
    if (IS->is_player.p_feMode & FE_WANT_SHIP)
    {
		/* we put all the contstant stuff up here */
		user(IS, FE_SHSCAN);        /* indicate that this is a ship scan */
		userX(IS, sh->sh_number, 8);
		userX(IS, sh->sh_row, 4);
		userX(IS, sh->sh_col, 4);
		userX(IS, sh->sh_type, 1);
		userX(IS, sh->sh_owner, 2);
		strncpy(shNameBuf, sh->sh_name, 12 * sizeof(char));
		shNameBuf[12]='\0';
		sprintf(work, "%-12s", shNameBuf);
		user(IS, work);
		userX(IS, numEng, 3);
		userX(IS, numWeap, 3);
		userX(IS, ETF, 3);
		userX(IS, WTF, 3);
		userX(IS, eff, 2);
		userX(IS, cargo, 4);
		userX(IS, civ, 4);
		userX(IS, mil, 4);
		userX(IS, shield, 4);
		userX(IS, energ, 4);
		userX(IS, armour, 4);
		userNL(IS);
    }
}

/*
 * doShScan - does a detailed scan of a ship. Assumes the FROM ship/planet is
 *          in the request buffer
 */

void doShScan(IMP, ULONG targetNum, BOOL isShip)
{
    register Planet_t *rpl;
    register Ship_t *rsh;
    Ship_t saveSh, targSh;
    Planet_t savePl;
    USHORT distance, range, rangeSq;
    short tf, difTf;
    BOOL didIt;

    rpl = &IS->is_request.rq_u.ru_planet;
    rsh = &IS->is_request.rq_u.ru_ship;

    didIt = FALSE;
    /* are we scanning from a ship? */
    if (isShip)
    {
        if (targetNum == rsh->sh_number)
        {
            user(IS, "Can't use scanners on the ship they are mounted in!\n");
            return;
        }
        server(IS, rt_lockShip, rsh->sh_number);
        if (rsh->sh_energy < SCAN_COST)
        {
            server(IS, rt_unlockShip, rsh->sh_number);
            user(IS, "Not enough energy to do a scan\n");
            return;
        }
        rsh->sh_energy -= SCAN_COST;
        saveSh = *rsh;
        server(IS, rt_unlockShip, rsh->sh_number);
        tf = (short) getShipTechFactor(IS, &saveSh, bp_sensors);
        range = getShipVRange(IS, &saveSh) * saveSh.sh_efficiency * (100 -
            (100 - tf) / 2) / 100;
    }
    else
    {
        savePl = *rpl;
        tf = (short) getTechFactor(rpl->pl_techLevel);
        range = ((ULONG)rpl->pl_efficiency) * IS->is_world.w_radarFactor *
            (100 - (100 - tf) / 2) / 100;
    }
    /* read in the target ship */
    server(IS, rt_readShip, targetNum);
    distance = findDistance(IS, saveSh.sh_row, saveSh.sh_col, rsh->sh_row,
        rsh->sh_col);
    range /= 3;
    rangeSq = ((ULONG)range) * range / (100 * 100);
    if (distance > (rangeSq / 3))
    {
        user(IS, "Target ship is too far away\n");
        return;
    }
    targSh = *rsh;
    difTf = tf - ((getShipTechFactor(IS, &targSh, bp_engines) +
        targSh.sh_hullTF) / 2);
    /* check target difference */
    if (difTf < -60)
    {
        user(IS, "Other ship is just too advanced to read any information"
            " from it\n");
    }
    else
    {
        userN3(IS, "Results of sensor scan of ship ", targSh.sh_number,
            ":\n");
        user3(IS, "Appears to be a \"", getShipName(targSh.sh_type),
            "\" hull, registered to ");
        if (targSh.sh_owner != NO_OWNER)
        {
            server(IS, rt_readPlayer, targSh.sh_owner);
            user(IS, &IS->is_request.rq_u.ru_player.p_name[0]);

        }
        else
        {
            user(IS, "*UNKNOWN*");
        }
        userNL(IS);
        if (targSh.sh_name[0] != '\0')
        {
            user3(IS, "Ship bears the name \"", &targSh.sh_name[0],
                "\"\n");
        }
        giveShDetails(IS, &targSh, difTf);
        didIt = TRUE;
    }
    if (targSh.sh_owner != NO_OWNER)
    {
        /* now see about sending a telegram to the ship owner */
        if (difTf > 60)
        {
            /* they can't detect the scan */
            return;
        }
        user(IS, "Sensors indicate we are being probed!\n");
        if (difTf < 25)
        {
            user(IS, "They detected our identity!\n");
            user2(IS, &IS->is_player.p_name[0], " ");
        }
        if (isShip)
        {
            user(IS, getShipName(saveSh.sh_type));
            userN2(IS, " #", saveSh.sh_number);
            if (saveSh.sh_name[0] != '\0')
            {
                user3(IS, " (", &saveSh.sh_name[0], ")");
            }
            userN2(IS, " at ", saveSh.sh_row);
            userN2(IS, ",", saveSh.sh_col);
            if (didIt)
            {
                user(IS, " was able to scan your ");
            }
            else
            {
                user(IS, " attempted to scan your ");
            }
            user(IS, getShipName(targSh.sh_type));
            userN2(IS, " #", targetNum);
        }
        else
        {
            userN2(IS, "planet #", savePl.pl_number);
            if (savePl.pl_name[0] != '\0')
            {
                user3(IS, " (", &savePl.pl_name[0], ")");
            }
            userN2(IS, " at ", savePl.pl_row);
            userN2(IS, ",", savePl.pl_col);
            if (didIt)
            {
                user(IS, " was able to scan your ");
            }
            else
            {
                user(IS, " attempted to scan your ");
            }
            user(IS, getShipName(targSh.sh_type));
            userN2(IS, " #", targetNum);
        }
        notify(IS, targSh.sh_owner);
    }
}

/*
 * givePlDetails - prints out varying details of a planet depending on the
 *          difference of tech factors
 */

void givePlDetails(IMP, register Planet_t *pl, short difTf)
{
    register USHORT none;
    USHORT size, polut, eff, minr, gold, gas, water, TF, civ,
        mil, ore, bar;
    UBYTE race;
    char plNameBuf[25], work[15];

    /* pre-initialize all the fields */
    none = NO_ITEM;
    size = none;
    polut = none;
    eff = none;
    minr = none;
    gold = none;
    gas = none;
    water = none;
    TF = none;
    civ = none;
    mil = none;
    ore = none;
    bar = none;

    /* find out what to print */

    if (difTf > -65)
    {
        /* geological stats are fairly easy to get */
        size = pl->pl_size;
        polut = pl->pl_polution;
        minr = pl->pl_minerals;
        gold = pl->pl_gold;
        gas = pl->pl_gas;
        water = pl->pl_water;
        /* these are not as easy to get */
        if (difTf > -30)
        {
            ore = readPlQuan(IS, pl, it_ore);
            bar = readPlQuan(IS, pl, it_bars);
            if (difTf < 20)
            {
                if (difTf > 0)
                {
                    ore = ((ore / 6) * 6);
                    bar = ((bar / 5) * 5);
                }
                else
                {
                    ore = ((ore / 18) * 18);
                    bar = ((bar / 10) * 10);
                }
            }
            eff = pl->pl_efficiency;
            TF = getTechFactor(pl->pl_techLevel);
            civ = readPlQuan(IS, pl, it_civilians) + readPlQuan(IS, pl,
																it_scientists);
            mil = readPlQuan(IS, pl, it_military) + readPlQuan(IS, pl,
															   it_officers);
            if (difTf < 11)
            {
                eff = ((eff / 20) * 20);
                civ = ((civ / 15) * 15);
                mil = ((mil / 20) * 20);
            }
            else
            {
                if (difTf < 35)
                {
                    eff = ((eff / 5) * 5);
                    civ = ((civ / 8) * 8);
                    mil = ((mil / 12) * 12);
                }
                else
                {
                    if (difTf < 50)
                    {
                        mil = ((mil / 6) * 6);
                    }
                }
            }
            
        }
    }

    /* print out the header */
    user(IS, "Size|Polut|Eff|Minr|Gold|Gas|Wat|  Ore  |  Bars | Civil | Milit | TF\n");
    /* if each item has been set, then print it */
    if (size == none)
    {
        user(IS, "  ? ");
		size=0;
    }
    else
    {
        userF(IS, size, 4);
    }
    userC(IS, '|');
    if (polut == none)
    {
        user(IS, "  ?  ");
		polut=0;
    }
    else
    {
        userF(IS, polut, 5);
    }
    userC(IS, '|');
    if (eff == none)
    {
        user(IS, " ? ");
		eff=0;
    }
    else
    {
        userF(IS, eff, 3);
    }
    userC(IS, '|');
    if (minr == none)
    {
        user(IS, "  ? ");
		minr=0;
    }
    else
    {
        userF(IS, minr, 4);
    }
    userC(IS, '|');
    if (gold == none)
    {
        user(IS, "  ? ");
		gold=0;
    }
    else
    {
        userF(IS, gold, 4);
    }
    userC(IS, '|');
    if (gas == none)
    {
        user(IS, " ? ");
		gas=0;
    }
    else
    {
        userF(IS, gas, 3);
    }
    userC(IS, '|');
    if (water == none)
    {
        user(IS, " ? ");
		water=0;
    }
    else
    {
        userF(IS, water, 3);
    }
    userC(IS, '|');
    if (ore == none)
    {
        user(IS, "   ?   ");
		ore=0;
    }
    else
    {
        userF(IS, ore, 7);
    }
    userC(IS, '|');
    if (bar == none)
    {
        user(IS, "   ?   ");
		bar=0;
    }
    else
    {
        userF(IS, bar, 7);
    }
    userC(IS, '|');
    if (civ == none)
    {
        user(IS, "   ?   ");
		civ=0;
    }
    else
    {
        userF(IS, civ, 7);
    }
    userC(IS, '|');
    if (mil == none)
    {
        user(IS, "   ?   ");
		mil=0;
    }
    else
    {
        userF(IS, mil, 7);
    }
    userC(IS, '|');
    if (TF == none)
    {
        user(IS, " ?");
		TF=0;
    }
    else
    {
        userF(IS, TF, 3);
    }
    userNL(IS);
    /* See about doing the FE stuff */
    if (IS->is_player.p_feMode & FE_WANT_PLAN)
    {
		/* we put all the contstant stuff up here */
		user(IS, FE_PLSCAN);        /* indicate that this is a planet scan */
		userX(IS, pl->pl_number, 8);
		userX(IS, pl->pl_row, 4);
		userX(IS, pl->pl_col, 4);
		userX(IS, pl->pl_class, 1);
        race = isHomePlanet(IS, pl->pl_number);
        if (race != NO_RACE)
		{
			userX(IS, race, 1);
		}
		else
		{
			userX(IS, 0, 1);
		}
		userX(IS, pl->pl_owner, 2);
		strncpy(plNameBuf, pl->pl_name, 12 * sizeof(char));
		plNameBuf[12]='\0';
		sprintf(work, "%-12s", plNameBuf);
		user(IS, work);
		userX(IS, size, 2);
		userX(IS, polut, 2);
		userX(IS, minr, 2);
		userX(IS, gold, 2);
		userX(IS, gas, 2);
		userX(IS, water, 2);
		userX(IS, ore, 4);
		userX(IS, bar, 4);
		userX(IS, eff, 2);
		userX(IS, TF, 2);
		userX(IS, civ, 4);
		userX(IS, mil, 4);
		userNL(IS);
    }
}

/*
 * doPlScan - does a detailed scan of a planet. Assumes the FROM ship is
 *          in the request buffer
 */

void doPlScan(IMP, ULONG targetNum)
{
    register Planet_t *rpl;
    register Ship_t *rsh;
    Ship_t saveSh;
    Planet_t savePl;
    USHORT distance, range, rangeSq, player;
    short tf, difTf;
    ULONG shNum;
    BOOL didIt;
    UBYTE race;

    rpl = &IS->is_request.rq_u.ru_planet;
    rsh = &IS->is_request.rq_u.ru_ship;

    didIt = FALSE;
    shNum = rsh->sh_number;
    /* check for the special case planet of 0, meaning planet we are */
    /* currently orbiting */
    if (targetNum == 0)
    {
        targetNum = whichPlanet(IS, rsh->sh_row, rsh->sh_col);
        if (targetNum == NO_ITEM)
        {
            user(IS, "That ship is not currently orbiting or on the "
                "surface of a planet\n");
            return;
        }
    }
    server(IS, rt_lockShip, shNum);
    if (rsh->sh_energy < SCAN_COST)
    {
        server(IS, rt_unlockShip, rsh->sh_number);
        user(IS, "Not enough energy to do a scan\n");
        return;
    }
    rsh->sh_energy -= SCAN_COST;
    saveSh = *rsh;
    server(IS, rt_unlockShip, rsh->sh_number);
    tf = (short) getShipTechFactor(IS, &saveSh, bp_sensors);
    range = getShipVRange(IS, &saveSh) * saveSh.sh_efficiency * (100 -
        (100 - tf) / 2) / 100;
    /* read in the target planet */
    server(IS, rt_readPlanet, targetNum);
    distance = findDistance(IS, saveSh.sh_row, saveSh.sh_col, rpl->pl_row,
        rpl->pl_col);
    range /= 3;
    rangeSq = ((ULONG)range) * range / (100 * 100);
    if (distance > (rangeSq / 3))
    {
        user(IS, "Target planet is too far away\n");
        return;
    }
    savePl = *rpl;
    difTf = tf - getTechFactor(savePl.pl_techLevel);
    /* check target difference */
    if (difTf < -60)
    {
        user(IS, "Other planet is just too advanced to read any information"
            " from it\n");
    }
    else
    {
        userN3(IS, "Results of sensor scan of planet ", savePl.pl_number,
            ":\n");
        if (savePl.pl_class == pc_HOME)
        {
            race = isHomePlanet(IS, targetNum);
            if (race != NO_RACE)
            {
                user3(IS, "Planet is the home planet of the ",
                    &IS->is_world.w_race[race].r_name[0], " race\n");
                if (IS->is_player.p_race != race)
                {
                    /* now see about sending a telegram to the planets */
                    /* owning race */
                    if (difTf < 61)
                    {
                        user(IS, "Sensors indicate we are being probed!\n");
                        for (player = 0; player < IS->is_world.w_currPlayers;
                            player++)
                        {
                            server(IS, rt_readPlayer, player);
                            if (IS->is_request.rq_u.ru_player.p_race == race)
                            {
                                if (difTf < 25)
                                {
                                    user2(IS, &IS->is_player.p_name[0], " ");
                                }
                                user(IS, getShipName(saveSh.sh_type));
                                userN2(IS, " #", saveSh.sh_number);
                                if (saveSh.sh_name[0] != '\0')
                                {
                                    user3(IS, " (", &saveSh.sh_name[0], ")");
                                }
                                userN2(IS, " at ", saveSh.sh_row);
                                userN2(IS, ",", saveSh.sh_col);
                                if (didIt)
                                {
                                    user(IS, " was able to scan your ");
                                }
                                else
                                {
                                    user(IS, " attempted to scan your ");
                                }
                                userN2(IS, "home planet #", targetNum);
                                notify(IS, player);
                            }
                        }
                        if (difTf < 25)
                        {
                            user(IS, "They detected our identity!\n");
                        }
                    }
                }
            }
            else
            {
                user(IS, "Planet is highly dangerous - notify god right "
                    "away\n");
            }
            return;
        }
        user(IS, "Is a class ");
        userC(IS, PLANET_CHAR[savePl.pl_class]);
        user(IS, " planet");
        if (savePl.pl_owner != NO_OWNER)
        {
            server(IS, rt_readPlayer, savePl.pl_owner);
            user2(IS, ", and is controlled by ",
                &IS->is_request.rq_u.ru_player.p_name[0]);

        }
        else
        {
            user(IS, ", and is not under the control of any player");
        }
        userNL(IS);
        if (savePl.pl_name[0] != '\0')
        {
            user3(IS, "Planet was at one time called \"",
                &savePl.pl_name[0], "\"\n");
        }
        givePlDetails(IS, &savePl, difTf);
        didIt = TRUE;
    }
    if (savePl.pl_owner != NO_OWNER)
    {
        /* now see about sending a telegram to the planets owner */
        if (difTf > 60)
        {
            /* they can't detect the scan */
            return;
        }
        user(IS, "Sensors indicate we are being probed!\n");
        if (difTf < 25)
        {
            user(IS, "They detected our identity!\n");
            user2(IS, &IS->is_player.p_name[0], " ");
        }
        user(IS, getShipName(saveSh.sh_type));
        userN2(IS, " #", saveSh.sh_number);
        if (saveSh.sh_name[0] != '\0')
        {
            user3(IS, " (", &saveSh.sh_name[0], ")");
        }
        userN2(IS, " at ", saveSh.sh_row);
        userN2(IS, ",", saveSh.sh_col);
        if (didIt)
        {
            user(IS, " was able to scan your ");
        }
        else
        {
            user(IS, " attempted to scan your ");
        }
        userN2(IS, "planet #", targetNum);
        notify(IS, savePl.pl_owner);
    }
}

/*
 * cmd_scan - allows the player to do various types of scans
 *
 */

BOOL cmd_scan(IMP)
{
    register Planet_t *rp;
    ULONG itemNumber, targetNum;
    USHORT what;
    BOOL isShip;
    Ship_t sShip;

    /* find out what they want to scan */
    if (reqChoice(IS, &what, "sr\0lr\0ship\0planet\0",
        "Scan what (SR/LR/ship/planet)"))
    {
        (void) skipBlanks(IS);
        /* planets are special, since we know the exact args */
        if (what == 3)
        {
            /* get the planet to scan */
            if (reqPlanet(IS, &targetNum, "Scan which planet"))
            {
                (void) skipBlanks(IS);
                /* and the ship to scan from */
                if (reqShip(IS, &itemNumber, "Scan from which ship"))
                {
                    server(IS, rt_readShip, itemNumber);
                    if (IS->is_request.rq_u.ru_ship.sh_owner !=
                        IS->is_player.p_number)
                    {
                        err(IS, "that ship doesn't belong to you");
                    }
                    else if (IS->is_request.rq_u.ru_ship.sh_type == st_m)
                    {
                        err(IS, "that ship is a miner");
                    }
                    else
                    {
                        /* note this is assuming the ship is in the */
                        /* request buffer */
                        doPlScan(IS, targetNum);
                        return TRUE;
                    }
                }
            }
            return FALSE;
        }
        /* if they want to scan a ship, get the target ship number first */
        if (what == 2)
        {
            if (reqShip(IS, &targetNum, "Enter the ship number to scan"))
            {
                /* position to the next field */
                (void) skipBlanks(IS);
            }
            else
            {
                /* otherwise return */
                return FALSE;
            }
        }
        /* get the planet or ship to scan FROM */
        if (reqPlanetOrShip(IS, &itemNumber, &isShip,
            "Enter planet or ship to scan from"))
        {
            /* check if it was a ship */
            if (isShip)
            {
                server(IS, rt_readShip, itemNumber);
                if (IS->is_request.rq_u.ru_ship.sh_owner !=
                    IS->is_player.p_number)
                {
                    err(IS, "that ship doesn't belong to you");
                }
                else if (IS->is_request.rq_u.ru_ship.sh_type == st_m)
                {
                    err(IS, "that ship is a miner");
                }
                else
                {
                    accessShip(IS, itemNumber);
                    sShip = IS->is_request.rq_u.ru_ship;
                    switch(what)
                    {
                        case 0:
                            doSRScan(IS, sShip.sh_row, sShip.sh_col,
                                (USHORT) getShipVRange(IS, &sShip) *
                                sShip.sh_efficiency * (100 - (100 -
                                getShipTechFactor(IS, &sShip, bp_sensors))
                                / 2) / 100);
                            break;
                        case 1:
                            doLRScan(IS, sShip.sh_row, sShip.sh_col,
                                (USHORT) getShipVRange(IS, &sShip) *
                                sShip.sh_efficiency * (100 - (100 -
                                getShipTechFactor(IS, &sShip, bp_sensors))
                                / 2) / 100);
                            break;
                        case 2:
                            /* note this assumes the FROM ship is in the */
                            /* buffer */
                            IS->is_request.rq_u.ru_ship = sShip;
                            doShScan(IS, targetNum, TRUE);
                            break;
                        default:
                            break;
                    }
                }
            }
            else
            {
                accessPlanet(IS, itemNumber);
                rp = &IS->is_request.rq_u.ru_planet;
                if ((rp->pl_owner != IS->is_player.p_number) &&
                    (isHomePlanet(IS, itemNumber) != IS->is_player.p_race))
                {
                    err(IS, "that planet doesn't belong to you");
                }
                else
                {
                    switch(what)
                    {
                        case 0:
                            doSRScan(IS, rp->pl_row, rp->pl_col,
                                (USHORT) ((ULONG)rp->pl_efficiency) *
                                IS->is_world.w_radarFactor * (100 - (100 -
                                getTechFactor(rp->pl_techLevel)) / 2) /
                                100);
                            break;
                        case 1:
                            doLRScan(IS, rp->pl_row, rp->pl_col,
                                (USHORT) ((ULONG)rp->pl_efficiency) *
                                IS->is_world.w_radarFactor * (100 - (100 -
                                getTechFactor(rp->pl_techLevel)) / 2) /
                                100);
                            break;
                        case 2:
                            /* note this assumes the FROM planet is in the */
                            /* buffer */
                            doShScan(IS, targetNum, FALSE);
                            break;
                        default:
                            break;
                    }
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * visPlan - get visual appearence of a planets or stars in range.
 */

void visPlan(IMP, register ULONG minPl, register ULONG maxPl, USHORT vRow,
    USHORT vCol, USHORT rang)
{
    register Planet_t *rpl;
    register USHORT r, c;
    USHORT distance, size, rangeSq;
    ULONG planetNumber;

    if (IS->is_world.w_shipNext != 0)
    {
        rangeSq = ((ULONG)rang) * rang / (1000 * 1000);
        rpl = &IS->is_request.rq_u.ru_planet;
        for (planetNumber = minPl; planetNumber <= maxPl;
            planetNumber++)
        {
            server(IS, rt_readPlanet, planetNumber);
            if (rpl->pl_class == pc_S)
            {
                r = rpl->pl_row;
                c = rpl->pl_col;
                distance = findDistance(IS, r, c, vRow, vCol);
                size = rpl->pl_size * 3;
                /* stars can be seen 4 times as far away as planets */
                if (distance < (((ULONG)rangeSq) * size * size /
                    IS->is_world.w_lookShipFact) * 4)
                {
                    userN2(IS, "Star #", rpl->pl_number);
                    userS(IS, " at ", rpl->pl_row, rpl->pl_col, " appears");
                    userN3(IS, " to be size ", rpl->pl_size, "\n");
                }
            }
            else
            {
                r = rpl->pl_row;
                c = rpl->pl_col;
                distance = findDistance(IS, r, c, vRow, vCol);
                size = rpl->pl_size * 3;
                if (distance < ((ULONG)rangeSq) * size * size /
                    IS->is_world.w_lookShipFact)
                {
                    userP(IS, "Planet ", rpl, "");
                    userS(IS, " at ", r, c, " appears to be ");
                    userN3(IS, "size ", rpl->pl_size, "\n");
                }
            }
        }
    }
}

/*
 * visShips - get visual appearence of ships nearby.
 */

void visShips(IMP, USHORT vRow, USHORT vCol, USHORT rang)
{
    register Ship_t *rsh;
    register USHORT r, c;
    Ship_t sh;
    USHORT distance, size, rangeSq;
    ULONG shipNumber;

    if (IS->is_world.w_shipNext != 0)
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rangeSq = ((ULONG)rang) * rang / (1000 * 1000);
        for (shipNumber = 0; shipNumber < IS->is_world.w_shipNext;
            shipNumber++)
        {
            server(IS, rt_readShip, shipNumber);
            if (rsh->sh_owner != NO_OWNER)      /* wrecks don't show up */
            {
                r = rsh->sh_row;
                c = rsh->sh_col;
                distance = findDistance(IS, r, c, vRow, vCol);
                size = getShipVisib(IS, rsh);
                if (distance < ((ULONG)rangeSq) * size * size /
                    IS->is_world.w_lookShipFact)
                {
                    if (IS->is_player.p_feMode & FE_WANT_SCAN)
                    {
                        user(IS, FE_VRS);
                    }
                    sh = *rsh;
                    server(IS, rt_readPlayer, sh.sh_owner);
                    user(IS, &IS->is_request.rq_u.ru_player.p_name[0]);
                    userSp(IS);
                    user(IS, getShipName(sh.sh_type));
                    userN2(IS, " #", shipNumber);
                    userN3(IS, " (", sh.sh_efficiency / 10 * 10,
                               "% efficient)");
                    userS(IS, " at ", r, c, "\n");
                }
            }
        }
    }
}

/*
 * cmd_visual - allows players to do visual observations of planets and ships
 *          nearby
 */

BOOL cmd_visual(IMP)
{
    register Planet_t *rpl;
    register Ship_t *rsh;
    ULONG shipNumber;
    USHORT rang;
    register USHORT r, c;
    BOOL isShip;
    ULONG minPl, maxPl;

    if (reqPlanetOrShip(IS, &shipNumber, &isShip,
        "Planet or ship to do visual observation from"))
    {
        minPl = 0xffffffff;
        maxPl = 0;
        rpl = &IS->is_request.rq_u.ru_planet;
        rsh = &IS->is_request.rq_u.ru_ship;
        /* looking from a ship? */
        if (isShip)
        {
            server(IS, rt_readShip, shipNumber);
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else if (rsh->sh_type == st_m)
            {
                err(IS, "that ship is a miner");
            }
            else
            {
                accessShip(IS, shipNumber);
                r = rsh->sh_row;
                c = rsh->sh_col;
                rang = getShipVRange(IS, rsh);
/* rang = IS->is_world.w_shipRange[rsh->sh_type] * rsh->sh_efficiency; */
                getPlRange(IS, &minPl, &maxPl, r, c, rang);
                visPlan(IS, minPl, maxPl, r, c, rang);
                visShips(IS, r, c, rang);
                return TRUE;
            }
        }
        else
        {
            /* looking from a planet */
            server(IS, rt_readPlanet, shipNumber);
            if (rpl->pl_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that planet");
            }
            else
            {
                /* update the planet for more accurate observations */
                accessPlanet(IS, shipNumber);
                r = rpl->pl_row;
                c = rpl->pl_col;
                rang = ((USHORT)rpl->pl_efficiency) * 40;
                getPlRange(IS, &minPl, &maxPl, r, c, rang);
                visPlan(IS, minPl, maxPl, r, c, rang);
                visShips(IS, r, c, rang);
                return TRUE;
            }
        }
    }
    return FALSE;
}

