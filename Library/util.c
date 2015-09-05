/*
 * Imperium
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
 * some utility routines.
 *
 * $Id: util.c,v 1.5 2000/05/26 22:55:19 marisa Exp $
 *
 * $Log: util.c,v $
 * Revision 1.5  2000/05/26 22:55:19  marisa
 * Combat changes
 *
 * Revision 1.4  2000/05/25 18:21:23  marisa
 * Fix more T/F issues
 *
 * Revision 1.3  2000/05/23 20:25:14  marisa
 * Added pl_weapon[] element to Planet_t struct
 * Began working on ship<->planet combat code
 *
 * Revision 1.2  2000/05/18 06:32:04  marisa
 * Crypt, etc.
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:59  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:24:39  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:29  marisag
 * patch20: Fix empty revision.
 *
 */

#include "../config.h"

#ifdef HAVE_STRINGS_H
#include <strings.h>
#else
#include <string.h>
#endif
#include <stdio.h>
#include <math.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
BOGUS - Imperium can not run on this machine due to missing stdlib.h
#endif

#define UtilC 1
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"

static const char rcsid[] = "$Id: util.c,v 1.5 2000/05/26 22:55:19 marisa Exp $";

/*
 * numcnvt - converts a number to a string
 */

char *numcnvt(register ULONG number, register char *buffer)
{
    register char *p;

    p = buffer + 10 * sizeof(char);
    *p = '\0';
    do
    {
        p -= sizeof(char);
        *p = number % 10 + '0';
        number /= 10;
    }
    while (number);
    return p;
}

/*
 * timeRound - round a seconds time value down to a number of ITUs.
 */

ULONG timeRound(IMP, ULONG t)
{
    return t / IS->is_world.w_secondsPerITU * IS->is_world.w_secondsPerITU;
}

/*
 * timeNow - return a rounded down time from the current request.
 */

ULONG timeNow(IMP)
{
    return IS->is_request.rq_time / IS->is_world.w_secondsPerITU *
        IS->is_world.w_secondsPerITU;
}

/*
 * impRandom - return a random number 0 - passed range.
 */

USHORT impRandom(IMP, USHORT rang)
{
    if (rang == 0)
    {
        return 0;
    }
    return nrand48(&IS->is_seed[0]) % rang;
}

/*
 * lookupCommand - look up a command in a table of command names.
 *      If the name is unambiguous, return it's code number (2 - n + 2).
 *      Return 0 if the command is not found, 1 if it's ambiguous.
 */

USHORT lookupCommand(register const char *commandList, char *command)
{
    register char *p;
    register USHORT i, which = 0, found;

    i = 2;
    found = 0;
    while (*commandList != '\0')
    {
        p = command;
        while ((*p == *commandList) && (*p != '\0'))
        {
            p++;
            commandList++;
        }
        if (*p == '\0')
        {
            which = i;
            found++;
        }
        while (*commandList != '\0')
        {
            commandList++;
        }
        commandList++;
        i++;
    }
    if (found == 0)
    {
        return 0;
    }
    if (found == 1)
    {
        return which;
    }
    return 1;
}

/*
 * dash - utility to print a line of dashes.
 */

void dash(IMP, register USHORT count)
{
    while (count)
    {
        count--;
        userC(IS, '-');
    }
    userNL(IS);
}

/*
 * err - print an error message.
 */

void err(IMP, char *mess)
{

    user3(IS, "*** ", mess, " ***\n");
}

/*
 * getDesigName - return the full string for a sector type name.
 */

char *getDesigName(SectorType_t desig)
{

    switch (desig)
    {
        case s_unknown:
            return "unknown";
        case s_blackhole:
            return "blackhole";
        case s_supernova:
            return "supernova";
        case s_vacant:
            return "vacant";
        case s_normal:
            return "normal";
        default:
            return "?bad desig?";
    }
}

/*
 * getItemName - return the full string name for a commodity.
 */

char *getItemName(ItemType_t item)
{
    switch (item)
    {
        case it_civilians:
            return "civilians";
        case it_scientists:
            return "scientists";
        case it_military:
            return "military";
        case it_officers:
            return "officers";
        case it_missiles:
            return "missiles";
        case it_planes:
            return "planes";
        case it_ore:
            return "ore";
        case it_bars:
            return "bars";
        case it_airTanks:
            return "air tanks";
        case it_fuelTanks:
            return "fuel tanks";
        case it_computers:
            return "computers";
        case it_engines:
            return "engines";
        case it_lifeSupp:
            return "life support";
        case it_elect:
            return "electronics";
        case it_weapons:
            return "weapons";
        default:
            return "?bad item?";
    }
}

/*
 * getShipName - return the full string name for a ship.
 */

char *getShipName(ShipType_t typ)
{
    switch (typ)
    {
        case st_a:
            return "class A";
        case st_b:
            return "class B";
        case st_c:
            return "class C";
        case st_d:
            return "class D";
        case st_e:
            return "class E";
        case st_m:
            return "miner";
        default:
            return "?bad ship?";
    }
}

char *getPlanetName(PlanetClass_t typ)
{
    switch (typ)
    {
        case pc_S:
            return "star";
        case pc_HOME:
            return "home planet";
        case pc_A:
            return "class A";
        case pc_B:
            return "class B";
        case pc_C:
            return "class C";
        case pc_D:
            return "class D";
        case pc_M:
            return "class M";
        case pc_N:
            return "class N";
        case pc_O:
            return "class O";
        case pc_Q:
            return "class Q";
        default:
            return "?bad planet?";
    }
}

/*
 * getIndex - return the position of a character in a character array.
 * Note that the function will return 65535 if the character is not found
 * in the string, and that you MUST end the string with '\0' for this to
 * work.
 */

USHORT getIndex(IMP, register char *types, register char typ)
{
    register USHORT myIndex;
    char tempbuf[4];

    myIndex = 0;
    while((*types != '\0') && (*types != typ))
    {
        types += sizeof(char);
        myIndex++;
    }
    if (*types == '\0')
    {
        myIndex = 65535;
        tempbuf[0] = '\'';
        tempbuf[1] = ' ';
        tempbuf[2] = typ;
        tempbuf[3] = '\0';
        log3(IS, "*** getIndex passed a value not found in the given string '",
            types,&tempbuf[0]);
    }
    return myIndex;
}

/*
 * getShipIndex - return the Index of the given ship type code.
 */

USHORT getShipIndex(IMP, char shipType)
{
    register USHORT tempval;

    tempval = getIndex(IS, &IS->is_shipChar[0], shipType);
    if (tempval == 65535)
    {
        tempval = getIndex(IS, &IS->is_shipChar[0], 'S');
    }
    return tempval;
}

/*
 * getItemIndex - return the Index of the given item type code.
 */

USHORT getItemIndex(IMP, char itemType)
{
    register USHORT tempval;

    tempval = getIndex(IS, &IS->is_itemChar[0], itemType);
    if (tempval == 65535)
    {
        tempval = getIndex(IS, &IS->is_itemChar[0], 'c');
    }
    return tempval;
}

/*
 * umin - return the minimum of two USHORTs.
 */

USHORT umin(USHORT a, USHORT b)
{

    if (a < b)
    {
        return a;
    }
    return b;
}

/*
 * ulmin - return the minimum of two ULONGs.
 */

ULONG ulmin(ULONG a, ULONG b)
{

    if (a < b)
    {
        return a;
    }
    return b;
}

/*
 * mapSector - map a user's coordinate idea of a sector to a server one.
 */

USHORT mapSector(IMP, register USHORT r, register USHORT c)
{
    /* need to strip out any subrow and column specified */
    r /= 10;
    c /= 10;
    return (r % IS->is_world.w_rows * IS->is_world.w_columns +
        c % IS->is_world.w_columns);
}

/*
 * accessShip - someone is looking at a ship, but in a way which will
 *      force it to be updated.
 */

void accessShip(IMP, ULONG shipNumber)
{
    IS->is_noWrite = TRUE;
    server(IS, rt_lockShip, shipNumber);
    updateShip(IS);
    server(IS, rt_unlockShip, shipNumber);
    IS->is_noWrite = FALSE;
    uFlush(IS);
}

/*
 * accessPlanet - similar thing for a planet.
 */

void accessPlanet(IMP, ULONG planetNumber)
{
    IS->is_noWrite = TRUE;
    server(IS, rt_lockPlanet, planetNumber);
    updatePlanet(IS);
    server(IS, rt_unlockPlanet, planetNumber);
    IS->is_noWrite = FALSE;
    uFlush(IS);
}

/*
 * readPlQuan - return the current quantity of the indicated commodity at the
 *        passed planet. This function is provided for compatibility with
 *        Empire, and may be used for future Imperium uses if bundling ever
 *        has to occur. You should use these functions even though they
 *        don't really do anything.
 */

USHORT readPlQuan(IMP, Planet_t *p, ItemType_t what)
{
    return p->pl_quantity[what];
}

/*
 * writeQuan - write the given quantity of the indicated commodity to the
 *        passed planet. Any excess is discarded silently.
 */

void writePlQuan(IMP, Planet_t *p, ItemType_t what, USHORT quan)
{
    p->pl_quantity[what] = quan;
}

/*
 * getTransportCost - get the transportion cost of moving the given quantity
 *      of the given thing on/off of the given planet.
 */

USHORT getTransportCost(IMP, Planet_t *planet, ItemType_t thingType,
    register USHORT quantity)
{
    switch(thingType)
    {
        case it_civilians:
        case it_scientists:
        case it_military:
        case it_officers:
        case it_missiles:
        case it_planes:
        case it_ore:
        case it_bars:
        case it_airTanks:
        case it_fuelTanks:
        case it_computers:
        case it_engines:
        case it_lifeSupp:
        case it_elect:
        case it_weapons:
            quantity *= IS->is_world.w_mobCost[thingType];
            break;
        default:
            err(IS, "bad item type in 'getTransportCost'");
            break;
    }
    switch (planet->pl_class)
    {
        case pc_B:
        case pc_C:
            quantity *= 4;
            break;
        case pc_A:
            quantity *= 2;
            break;
        default:
            break;
    }
    return quantity;
}

/*
 * adjustForNewWorkers - new workers have just been moved into the planet -
 *      fix it up so as to not lose mobility or gain work.
 *      NOTE: we assume that 'rq_time' is relatively recent.
 */

void adjustForNewWorkers(IMP, register Planet_t *p,
                         ItemType_t what, USHORT quantity)
{
    ULONG now, dt, dt2, iwork;
    USHORT workForce;

    /* take care of simple cases */
    if ((p->pl_quantity[it_civilians] == 0) &&
        (p->pl_quantity[it_scientists] == 0) &&
        (p->pl_quantity[it_military] == 0) &&
        (p->pl_quantity[it_officers] == 0))
    {
        return;
    }

    /* we have to keep any work pending in the target
       planet correct. We do this by moving its last
       update time forward as required. */
    /* workforce BEFORE the new guys added: */
    switch(what)
    {
        case it_civilians:
            workForce = quantity + (p->pl_quantity[it_scientists] / 2) +
                (p->pl_quantity[it_military] / 5) +
                (p->pl_quantity[it_officers] / 10);
            break;
        case it_scientists:
            workForce = p->pl_quantity[it_civilians] + (quantity / 2) +
                (p->pl_quantity[it_military] / 5) +
                (p->pl_quantity[it_officers] / 10);
            break;
        case it_military:
            workForce = p->pl_quantity[it_civilians] +
                (p->pl_quantity[it_scientists] / 2) +
                (quantity / 5) + (p->pl_quantity[it_officers] / 10);
            break;
        case it_officers:
            workForce = p->pl_quantity[it_civilians] +
                (p->pl_quantity[it_scientists] / 2) +
                (p->pl_quantity[it_military] / 5) + (quantity / 10);
            break;
        default:
            workForce = p->pl_quantity[it_civilians] +
                (p->pl_quantity[it_scientists] / 2) +
                (p->pl_quantity[it_military] / 5) +
                (p->pl_quantity[it_officers] / 10);
            break;
    }
    now = IS->is_request.rq_time;
    if ((p->pl_lastUpdate == 0) || (p->pl_lastUpdate > now) ||
        ((p->pl_lastUpdate < now - (7 * 24 * 60 * 60)) && (workForce != 0)))
    {
        p->pl_lastUpdate = timeRound(IS, now);
    }
    dt = (now - p->pl_lastUpdate) / IS->is_world.w_secondsPerITU;
    /* work pending in the planet: */
    iwork = workForce * dt;
    /* workforce AFTER the new guys added: */
    workForce = p->pl_quantity[it_civilians] + (p->pl_quantity[it_military]
        / 5);
    /* time they would have taken to do the work: */
    /* protect against a zero workForce */
    if (workForce != 0)
    {
        dt2 = iwork / workForce;
    }
    else
    {
        dt2 = dt;
    }
    /* add mobility since it's not dependent on iwork: */
    p->pl_mobility = umin((USHORT)127, (ushort) p->pl_mobility + (dt - dt2));
    /* and crank the update time forward: */
    p->pl_lastUpdate = (now / IS->is_world.w_secondsPerITU - dt2) *
                            IS->is_world.w_secondsPerITU;
}

/*
 * getTechFactor - return the tech factor for the passed technology level
 *      (range returned is 0 - 99)
 */

UBYTE getTechFactor(register ULONG level)
{
    return (UBYTE)((150000 + (6175 * level)) / (10000 + (61 * level)));
}

/*
 * getShipTechFactor - returns the effective tech factor of the specific
 *          item on the passed ship. Note that this function WILL read in
 *          the various big item structures, so anything in the request
 *          buffer must be saved before this call.
 */

UBYTE getShipTechFactor(IMP, Ship_t *sh, BigPart_t part)
{
    register ULONG techLevel;
    register UBYTE icount, curIt;
    register BigItem_t *bip;
    ItemType_t it;
    ULONG *nump = NULL;
    USHORT maxI = 0;

#ifdef DEBUG_LIB
    if (part > BP_LAST)
    {
        log3(IS, "*** ", "big part incorrect in getShipTechFactor", " ***");
        return 0;
    }
#endif

    it = cvtBpIt(part);

#ifdef DEBUG_LIB
    if (it > IT_LAST)
    {
        log3(IS, "*** ", "item type incorrect in getShipTechFactor", " ***");
        return 0;
    }
#endif

    if (sh->sh_items[it] == 0)
    {
        /* ship does not have any of these items. */
        return 0;
    }

    switch(it)
    {
        case it_computers:
            nump = &sh->sh_computer[0];
            maxI = MAX_COMP;
            break;
        case it_engines:
            nump = &sh->sh_engine[0];
            maxI = MAX_ENG;
            break;
        case it_lifeSupp:
            nump = &sh->sh_lifeSupp[0];
            maxI = MAX_LIFESUP;
            break;
        case it_elect:
            nump = &sh->sh_elect[0];
            maxI = MAX_ELECT;
            break;
        case it_weapons:
            nump = &sh->sh_weapon[0];
            maxI = MAX_WEAP;
            break;
    }

    bip = &IS->is_request.rq_u.ru_bigItem;
    techLevel = 0;
    icount = 0;
    curIt = 0;
    while ((curIt < maxI) && (nump[curIt] != NO_ITEM))
    {
        server(IS, rt_readBigItem, nump[curIt]);
        if (bip->bi_part == part)
        {
            techLevel += bip->bi_techLevel;
            icount++;
        }
        curIt++;
    }
    if (icount == 0)
    {
        return 0;
    }
    techLevel /= icount;
    return getTechFactor(techLevel);
}

/*
 * getShipEff - returns the efficiency of the specific item on the passed
 *          ship. Note that this function WILL read in the various big item
 *          structures, so anything in the request buffer must be saved
 *          before this call.
 */

UBYTE getShipEff(IMP, Ship_t *sh, BigPart_t part)
{
    register BigItem_t *bip;
    register UBYTE icount, curIt;
    ULONG *nump = NULL;
    USHORT maxI = 0, effic;
    ItemType_t it;

#ifdef DEBUG_LIB
    if (part > BP_LAST)
    {
        log3(IS, "*** ", "part is incorrect in getShipEff", " ***");
        return 0;
    }
#endif

    it = cvtBpIt(part);

#ifdef DEBUG_LIB
    if (it > IT_LAST)
    {
        log3(IS, "*** ", "item type incorrect in getShipEff", " ***");
        return 0;
    }
#endif


    if (sh->sh_items[it] == 0)
    {
        /* ship does not have any of these items. */
        return 0;
    }

    switch(it)
    {
        case it_computers:
            nump = &sh->sh_computer[0];
            maxI = MAX_COMP;
            break;
        case it_engines:
            nump = &sh->sh_engine[0];
            maxI = MAX_ENG;
            break;
        case it_lifeSupp:
            nump = &sh->sh_lifeSupp[0];
            maxI = MAX_LIFESUP;
            break;
        case it_elect:
            nump = &sh->sh_elect[0];
            maxI = MAX_ELECT;
            break;
        case it_weapons:
            nump = &sh->sh_weapon[0];
            maxI = MAX_WEAP;
            break;
    }

    bip = &IS->is_request.rq_u.ru_bigItem;
    effic = 0;
    icount = 0;
    curIt = 0;
    while ((curIt < maxI) && (nump[curIt] != NO_ITEM))
    {
        server(IS, rt_readBigItem, nump[curIt]);
        if (bip->bi_part == part)
        {
            effic += bip->bi_effic;
            icount++;
        }
        curIt++;
    }
    if (icount == 0)
    {
        return 0;
    }
    return ((UBYTE)(effic / icount));
}

/*
 * findDistance - return the square of the distance (in sub-sectors) between
 *          two locations.
 *          Note that this function assumes the row and col numbers are
 *          scaled by 10!
 */

USHORT findDistance(IMP, USHORT r1, USHORT c1, USHORT r2, USHORT c2)
{
    register ULONG d1, d2;

    if (r1 > r2)
    {
        d1 = r1 - r2;
    }
    else
    {
        d1 = r2 - r1;
    }
    while (d1 > (((IS->is_world.w_rows - 1) * 10) + 9))
    {
        d1 -= (IS->is_world.w_rows * 10);
    }

    if (c1 > c2)
    {
        d2 = c1 - c2;
    }
    else
    {
        d2 = c2 - c1;
    }
    while (d2 > (((IS->is_world.w_columns - 1) * 10) + 9))
    {
        d2 -= (IS->is_world.w_columns * 10);
    }
    return (USHORT)((d1 * d1) + (d2 * d2));
}

/*
 * getItemCost - return the cost per unit of various items.
 */

USHORT getItemCost(IMP, ItemType_t what)
{
    switch (what)
    {
        case it_planes:
            return IS->is_world.w_planeCost;
            break;
        case it_bars:
            return IS->is_world.w_barCost;
            break;
        default:
            /* ore, in particular */
            return 1;
            break;
    }
}

/*
 * getShipSpeed - returns the maximum speed of the ship expressed as the number
 *          of subsectors the ship may move in one hop.
 *          Note: This will read in big items, so the ship must NOT be in
 *          the request buffer.
 */

USHORT getShipSpeed(IMP, Ship_t *sh)
{
    register USHORT speed;
    ULONG *engp;
    BigItem_t *bip;
    UBYTE tf;

    engp = &sh->sh_engine[0];
    bip = &IS->is_request.rq_u.ru_bigItem;
    speed = 0;
    /* loop through valid engines */
    while (*engp != NO_ITEM)
    {
        server(IS, rt_readBigItem, *engp);
        tf = getTechFactor(bip->bi_techLevel);
        speed += ((30 * tf) / 100);
        engp++;
    }
    /* make sure ship has a hull */
    if (sh->sh_hull != NO_ITEM)
    {
        /* take into account better hulls */
        server(IS, rt_readBigItem, sh->sh_hull);
        tf = getTechFactor(bip->bi_techLevel);
    }
    else
    {
        err(IS, "ship has no hull!");
        return 0;
    }
    speed += ((10 * tf) / 100);
    return speed;
}

/*
 * getShipVRange - returns the number of -SUB- sectors that the ship's sensors
 *          can see. Will read big items, so the ship must NOT be in the
 *          request buffer.
 */

USHORT getShipVRange(IMP, register Ship_t *sh)
{
    register USHORT units, factor, units2;

    /* make sure they have some electronics on board to save time */
    if (sh->sh_items[it_elect] == 0)
    {
        /* they have no electronics at all */
        return 0;
    }
    /* see how many are installed */
    units = numInst(IS, sh, bp_sensors);
    if (units == 0)
    {
        /* no sensors installed */
        return 0;
    }
    /* for each valid sensor array, they get at least 1 sector, plus */
    /* 5 sectors max */
    units2 = units;
    units *= 5;
    factor = getShipTechFactor(IS, sh, bp_sensors);
    factor *= units;
    /* round up the values */
    if ((factor % 100) != 0)
    {
        factor += 100;
    }
    return ((factor / 100) + units2);
}

/*
 * getShipVisib - returns a "scale" factor of how visible the ship is to
 *          other ships. This will read in big items so the ship can not
 *          be in the request buffer.
 */

USHORT getShipVisib(IMP, register Ship_t *sh)
{
    register USHORT factor, size;

    /* first get get the basic ship visability factor */
    switch(sh->sh_type)
    {
        case st_a:
            size = 15;
            break;
        case st_b:
            size = 20;
            break;
        case st_c:
            size = 30;
            break;
        case st_d:
            size = 40;
            break;
        case st_e:
            size = 50;
            break;
        case st_m:
            size = 3;
            break;
        default:
            size = 99;
            break;
    }
    /* next take into account the hull tech factor */
    if (sh->sh_hull != NO_ITEM)
    {
        server(IS, rt_readBigItem, sh->sh_hull);
        factor = getTechFactor(IS->is_request.rq_u.ru_bigItem.bi_techLevel);
    }
    else
    {
        /* the ship has no hull, and so should really not show up */
        if (IS->is_player.p_status == ps_deity)
        {
            err(IS, "ship has no hull!");
            /* make sure it shows up on diety maps */
            return 99;
        }
        else
        {
            return 0;
        }
    }
    /* note that the "10" below should never be more than the amount listed */
    /* for a class 'A' ship */
    factor = (10 * factor) / 100;
    size -= factor;
    size += ((2 * sh->sh_armourLeft) / 10);
    if (size > 99)
    {
        size = 99;
    }
    return size;
}

/*
 * getNavCost - return the fuel cost of navigating the given ship one
 *          subsector. The result is x 10.
 */

USHORT getNavCost(IMP, Ship_t *sh)
{
    return (IS->is_world.w_baseFuelCost[sh->sh_type]) *
         (10000 / sh->sh_engTF * 2) / (10000 / sh->sh_engTF + 100) / 10;
}

/*
 * damageUnit - do damage to one quantity randomly.
 */

USHORT damageUnit(IMP, register USHORT quantity, USHORT damage)
{
    USHORT units, portion;

    if (damage > 100)
    {
        damage = 100;
    }
    units = quantity * damage;
    portion = units % 100;
    quantity -= units / 100;
    if (impRandom(IS, 100) < portion)
    {
        return quantity - 1;
    }
    return quantity;
}

/*
 * damageBigItem - do damage to one quantity randomly.
 * Convention, The item will be updated, and NO_ITEM will be returned if
 * the item is destroyed. Otherwise the biNum will be returned.
 */

ULONG damageBigItem(IMP, ULONG biNum, USHORT damage)
{
    register short effic;
    USHORT units, portion;
    BigItem_t *bi;

    bi = &IS->is_request.rq_u.ru_bigItem;
    if (damage > 100)
    {
        damage = 100;
    }
    server(IS, rt_lockBigItem, biNum);
    effic = bi->bi_effic;
    units = effic * damage;
    portion = units % 100;
    effic -= units / 100;
    if ((impRandom(IS, 100) < portion) && (effic > 0))
    {
        effic--;
    }
    bi->bi_effic = effic;
    bi->bi_lastUpdate = IS->is_request.rq_time;
    if (effic < EFFIC_DEAD)
    {
        /* set the status to destroyed */
        bi->bi_status = bi_destroyed;
        /* leave the onShip flag alone, so we can restore items more    */
        /* easily if needed */
        server(IS, rt_unlockBigItem, biNum);
        return NO_ITEM;
    }
    server(IS, rt_unlockBigItem, biNum);
    return biNum;
}

/*
 * damagePlanet - do damage to a planet as a result of shelling, etc.
 *      Convention - the planet is locked.
 */

void damagePlanet(IMP, USHORT damage, USHORT responsible)
{
    register Planet_t *rp;
    register Player_t *p;
    register USHORT owner;
    ULONG plNum;
    World_t *w;
    UBYTE race;
    ItemType_t it;
    PProd_t pr;

    rp = &IS->is_request.rq_u.ru_planet;
    p = &IS->is_request.rq_u.ru_player;
    w = &IS->is_request.rq_u.ru_world;
    plNum = rp->pl_number;
    rp->pl_efficiency = damageUnit(IS, rp->pl_efficiency, damage);
    rp->pl_mobility = damageUnit(IS, rp->pl_mobility, damage);
    for (it = IT_FIRST; it <= IT_LAST; it++)
    {
        writePlQuan(IS, rp, it, damageUnit(IS, readPlQuan(IS, rp, it), damage));
    }
    for (pr = PPROD_FIRST; pr <= PPROD_LAST; pr++)
    {
        rp->pl_prod[pr] = damageUnit(IS, rp->pl_prod[pr], damage);
    }
    owner = rp->pl_owner;
    if ((owner != NO_OWNER) && (rp->pl_quantity[it_civilians] == 0) &&
        (rp->pl_quantity[it_military] == 0))
    {
        rp->pl_owner = NO_OWNER;
        server(IS, rt_unlockPlanet, plNum);
        server(IS, rt_lockPlayer, owner);
        p->p_planetCount -= 1;
        server(IS, rt_unlockPlayer, owner);
        if (owner == IS->is_player.p_number)
        {
            IS->is_player= *p;
        }
        race = p->p_race;
        server(IS, rt_lockWorld, 0);
        w->w_race[race].r_planetCount -= 1;
        if (w->w_race[race].r_planetCount == 0)
        {
            w->w_race[race].r_status = rs_dead;
            server(IS, rt_unlockWorld, 0);
            /* the target race is kaput!!!! */
            user3(IS, "Race ", &w->w_race[race].r_name[0],
                " has been destroyed!!!\n");
            news(IS, n_destroyed, responsible, (USHORT)race);
        }
        else
        {
            server(IS, rt_unlockWorld, 0);
        }
        server(IS, rt_lockPlanet, plNum);
    }
}

/*
 * fleetPos - return the Index of a fleet corresponding to the letter given.
 */

USHORT fleetPos(register char ch)
{
    if ((ch >= 'a') && (ch <= 'z'))
    {
        return ch - 'a';
    }
    if ((ch >= 'A') && (ch <= 'Z'))
    {
        return ch - ('A' - 26);
    }
    return 0;
}

/*
 * removeFromFleet - remove the given ship from whatever fleet it is in.
 *      Leave it saying it is in fleet '*'. We pass in 'owner', since the
 *      ship itself may have been updated to have some other owner.
 *      Convention: the ship is locked before and after the call.
 */

void removeFromFleet(IMP, USHORT owner, ULONG shipNumber)
{
    register Ship_t *rsh;
    register Fleet_t *rf;
    register USHORT fleet, i;
    BOOL got_it;
    char tmpbuf[11];

    rsh = &IS->is_request.rq_u.ru_ship;
    rf = &IS->is_request.rq_u.ru_fleet;
    if (rsh->sh_fleet != '*')
    {
        i = fleetPos(rsh->sh_fleet);
        server(IS, rt_unlockShip, shipNumber);
        if (owner == IS->is_player.p_number)
        {
            fleet = IS->is_player.p_fleets[i];
        }
        else
        {
            server(IS, rt_readPlayer, owner);
            fleet = IS->is_request.rq_u.ru_player.p_fleets[i];
        }
        server(IS, rt_lockFleet, fleet);
        i = 0;
        got_it = FALSE;
        while ((!got_it) && (i != rf->f_count))
        {
            if (rf->f_ship[i] != shipNumber)
            {
                i++;
            }
            else
            {
                got_it = TRUE;
            }
        }
        if (i != rf->f_count)
        {
            while (i != rf->f_count - 1)
            {
                rf->f_ship[i] = rf->f_ship[i + 1];
                i++;
            }
            rf->f_count -= 1;
        }
        else
        {
            log3(IS, "*** Ship pointers did not match fleet pointers: ",
            numcnvt((ULONG)shipNumber, &tmpbuf[0]),"");
        }
        server(IS, rt_unlockFleet, fleet);
        server(IS, rt_lockShip, shipNumber);
        rsh->sh_fleet = '*';
    }
}

/*
 * decrShipCount - decrement the ship count in the given sector.
 *      Assumption: nothing is locked.
 */

void decrShipCount(IMP, USHORT mapped)
{
    server(IS, rt_lockSector, mapped);
    IS->is_request.rq_u.ru_sector.s_shipCount -= 1;
    server(IS, rt_unlockSector, mapped);
}

/*
 * incrShipCount - increment the ship count in the given sector.
 *      Assumption: nothing is locked.
 */

void incrShipCount(IMP, USHORT mapped)
{
    server(IS, rt_lockSector, mapped);
    IS->is_request.rq_u.ru_sector.s_shipCount += 1;
    server(IS, rt_unlockSector, mapped);
}

/*
 * decrPlShipCount - decrement the ship count on the given planet.
 *      Assumption: nothing is locked.
 */

void decrPlShipCount(IMP, register ULONG plNum)
{
    server(IS, rt_lockPlanet, plNum);
    IS->is_request.rq_u.ru_planet.pl_shipCount -= 1;
    server(IS, rt_unlockPlanet, plNum);
}

/*
 * incrPlShipCount - increment the ship count on the given planet.
 *      Assumption: nothing is locked.
 */

void incrPlShipCount(IMP, register ULONG plNum)
{
    server(IS, rt_lockPlanet, plNum);
    IS->is_request.rq_u.ru_planet.pl_shipCount += 1;
    server(IS, rt_unlockPlanet, plNum);
}

/*
 * buildShipWeight - builds up the ships current cargo weight. Should only be
 *      used when we have changed many items, and do not know the weights of
 *      the bigItems on board.
 *      Convention - the ship is locked before and after the call.
 *
 */

void buildShipWeight(IMP, Ship_t *rsh)
{
    register USHORT i;
    Ship_t tmpShip;
    USHORT cargo;
    BigItem_t *bi;

    bi = &IS->is_request.rq_u.ru_bigItem;
    tmpShip = *rsh;
    server(IS, rt_unlockShip, tmpShip.sh_number);
    cargo = 0;
    /* first build up the weight of small items on board */
    for (i = IT_FIRST; i <= IT_LAST_SMALL; i++)
    {
        cargo += IS->is_world.w_weight[i] * tmpShip.sh_items[i];
    }
    /* add on the weight of armour */
    cargo += tmpShip.sh_armourLeft * IS->is_world.w_armourWeight;
    /* loop for each of the big item types */
    for (i = 0; i < IS->is_world.w_bigItemNext; i++)
    {
        server(IS, rt_readBigItem, i);
        if ((bi->bi_itemLoc == tmpShip.sh_number) && bi->bi_onShip
            && (bi->bi_status == bi_inuse))
        {
            cargo += bi->bi_weight;
        }
    }
    tmpShip.sh_cargo = cargo;
    server(IS, rt_lockShip, tmpShip.sh_number);
    *rsh = tmpShip;
}

/*
 * autoRaiseShields - handles raising the shields for the ship in the
 *          buffer, if the user has enabled this feature.
 *
 *          Assumes:
 *                  The ship is locked and in the buffer
 */

void autoRaiseShields(IMP, BOOL needCheck)
{
    register Ship_t *rsh;
    register USHORT xferAmt;
    Ship_t saveShip;

    /* Set the main pointer */
    rsh = &IS->is_request.rq_u.ru_ship;
    /* Make sure they want to do this */
    if ((rsh->sh_flags & (SHF_SHIELDS | SHF_SHKEEP)) &&
        (rsh->sh_shieldKeep != 0))
    {
        if (rsh->sh_shields < rsh->sh_shieldKeep)
        {
            /* If they want the level maintained, we KNOW we should do it */
            /* If they only have SHF_SHIELDS, make sure that they have NO */
            /* shields currently enabled */
            if ((rsh->sh_flags & SHF_SHKEEP) || ((rsh->sh_flags & SHF_SHIELDS) &&
                (rsh->sh_shields == 0)))
            {
                /* if we do not need to check the status of the shield */
                /* generators, then just do it */
                xferAmt = rsh->sh_shieldKeep - rsh->sh_shields;
                if (rsh->sh_energy < xferAmt)
                {
                    xferAmt = rsh->sh_energy;
                }
                if (needCheck)
                {
                    saveShip = *rsh;
                    if (numInst(IS, &saveShip, bp_shield) == 0)
                    {
                        xferAmt = 0;
                    }
                    else if (getShipEff(IS, &saveShip, bp_shield) <
                        EFFIC_CRIT)
                    {
                        xferAmt = 0;
                    }
                    else if (numInst(IS, &saveShip, bp_computer) == 0)
                    {
                        xferAmt = 0;
                    }
                    else if (getShipEff(IS, &saveShip, bp_computer) <
                        EFFIC_CRIT)
                    {
                        xferAmt = 0;
                    }
                    *rsh = saveShip;
                }
                rsh->sh_shields += xferAmt;
                rsh->sh_energy -= xferAmt;
            }
        }
    }
}

/*
 * damageShip - do damage to a ship as a result of shelling, mine, etc.
 *      Convention - the ship is locked.
 *      If the ship is destroyed, the results will be cleared (they
 *      should only be used if the ship survives the damage)
 */

ShipDamRes_t damageShip(IMP, register ULONG shipNumber, USHORT damage,
	BOOL useShld)
{
    register Ship_t *rsh;
    register USHORT i;
    register BigItem_t *rbi;
    USHORT owner, oldAmt;
    Ship_t tempShip;
    BigPart_t part;
    ShipDamRes_t damRes;
    ULONG miner;

    rsh = &IS->is_request.rq_u.ru_ship;
    rbi = &IS->is_request.rq_u.ru_bigItem;
    damRes.sdr_shields = 0;
    damRes.sdr_armour = 0;
    damRes.sdr_main = 0;
    /* does the ship have any shields? */
    if (useShld && (rsh->sh_shields > 0))
    {
        /* first see if the shields absorb all the damage */
        if (rsh->sh_shields >= damage)
        {
            /* it did */
            rsh->sh_shields -= damage;
            damRes.sdr_shields = damage;
            /* Raise the shields, if desired */
            autoRaiseShields(IS, FALSE);
            return damRes;
        }
        /* it didn't, so eliminate the damage that was absorbed */
        damage -= rsh->sh_shields;
        damRes.sdr_shields = rsh->sh_shields;
        rsh->sh_shields = 0;
    }
    /* does the ship have any armour? */
    if (useShld && (rsh->sh_armourLeft > 0))
    {
        /* first see if the armour absorbs all the damage */
        if (rsh->sh_armourLeft * IS->is_world.w_armourPoints >= damage)
        {
            /* it did */
            rsh->sh_armourLeft -= damage / IS->is_world.w_armourPoints;
            rsh->sh_cargo -= damage * IS->is_world.w_armourWeight;
            damRes.sdr_armour = damage / IS->is_world.w_armourPoints;
            autoRaiseShields(IS, FALSE);
            return damRes;
        }
        /* it didn't, so eliminate the damage that was absorbed */
        rsh->sh_cargo -= damage * IS->is_world.w_armourWeight;
        damage -= (rsh->sh_armourLeft * IS->is_world.w_armourPoints);
        damRes.sdr_armour = (rsh->sh_armourLeft * IS->is_world.w_armourPoints);
        rsh->sh_armourLeft = 0;
    }
    damRes.sdr_main = damage;
    /* damage the small items */
    for (i = IT_FIRST; i <= IT_LAST_SMALL; i++)
    {
        oldAmt = rsh->sh_items[i];
        rsh->sh_items[i] = damageUnit(IS, rsh->sh_items[i], damage);
        /* see if we need to decrease the ship's cargo */
        if (oldAmt != rsh->sh_items[i])
        {
            rsh->sh_cargo -= ((oldAmt - rsh->sh_items[i]) *
                IS->is_world.w_weight[i]);
        }
    }
    if (rsh->sh_fuelLeft > 0)
    {
        oldAmt = rsh->sh_fuelLeft;
        rsh->sh_fuelLeft = damageUnit(IS, rsh->sh_fuelLeft, damage);
        if (oldAmt != rsh->sh_fuelLeft)
        {
            rsh->sh_cargo -= (oldAmt - rsh->sh_fuelLeft);
        }
    }
    /* copy the ship, since we will be unlocking and locking things */
    tempShip = *rsh;
    server(IS, rt_unlockShip, shipNumber);
    /* damage the big items on board */
    for (i = 0; i < IS->is_world.w_bigItemNext; i++)
    {
        /* note that this is slow, but I can't think of any other way */
        /* to assure that both installed and carried items get damaged */
        server(IS, rt_readBigItem, i);
        if ((rbi->bi_itemLoc == shipNumber) && rbi->bi_onShip &&
            (rbi->bi_status == bi_inuse) && (rbi->bi_part != bp_hull))
        {
            /* check to see if the item was destroyed */
            if (damageBigItem(IS, i, damage) == NO_ITEM)
            {
                part = rbi->bi_part;
                /* if the item is installed, remove it first */
                if (isInst(&tempShip, part, i))
                {
                    doUninstall(&tempShip, i, part);
                }
                /* decrement the ships item count */
                tempShip.sh_items[cvtBpIt(part)]--;
                /* and remove the weight */
                tempShip.sh_cargo -= rbi->bi_weight;
            }
        }
    }
    /* engines are already damaged */
    tempShip.sh_engEff = getShipEff(IS, &tempShip, bp_engines);
    if (damageBigItem(IS, tempShip.sh_hull, damage) == NO_ITEM)
    {
        server(IS, rt_lockShip, shipNumber);
        *rsh = tempShip;
        owner = rsh->sh_owner;
        removeFromFleet(IS, owner, shipNumber);
        damRes.sdr_shields = 0;
        damRes.sdr_armour = 0;
        damRes.sdr_main = 0;
        rsh->sh_owner = NO_OWNER;
        rsh->sh_efficiency = 0;
        rsh->sh_hullTF = 0;
        rsh->sh_engTF = 0;
        rsh->sh_engEff = 0;
        rsh->sh_planet = NO_ITEM;
        server(IS, rt_unlockShip, shipNumber);
        if (owner == IS->is_player.p_number)
        {
            user(IS, getShipName(tempShip.sh_type));
            userN3(IS, " #", shipNumber, " destroyed!\n");
        }
	/* See if we need to destroy any miner's on board: */
	if (rsh->sh_flags & SHF_CARRY_MINE)
	{
	    /* The ship WAS carrying some miner(s), so we need to scan */
	    for (miner = 0; miner < IS->is_world.w_shipNext; miner++)
	    {
		/* Read the ship */
		server(IS, rt_readShip, miner);
		/* Make sure it is a miner, and owned by the same player */
		if ((rsh->sh_type == st_m) && (rsh->sh_owner == owner))
		{
		    /* Make sure that it was on the ship that was destroyed */
		    if (rsh->sh_dragger == shipNumber)
		    {
			server(IS, rt_lockShip, miner);
			(void) damageShip(IS, miner, (USHORT) 0xFFFF, TRUE);
			server(IS, rt_unlockShip, miner);
			if (rsh->sh_owner != NO_OWNER)
			{
			    user(IS, "*** Alert the system owner that a miner "
				 "was not destroyed by damage 0xFFFF!\n");
			}
		    }
		}
	    }
	}
        decrShipCount(IS, mapSector(IS, tempShip.sh_row, tempShip.sh_col));
        if (tempShip.sh_planet != NO_ITEM)
        {
            decrPlShipCount(IS, tempShip.sh_planet);
        }
        server(IS, rt_lockShip, shipNumber);
	/* Free up the flags we KNOW we don't need */
	rsh->sh_flags &= ~(SHF_CARRY_MINE|SHF_TRACTEE);
    }
    else
    {
        server(IS, rt_readBigItem, tempShip.sh_hull);
        tempShip.sh_efficiency = rbi->bi_effic;
        server(IS, rt_lockShip, shipNumber);
        *rsh = tempShip;
        buildShipWeight(IS, rsh);
    }
    /* Prevent needless checking */
    if (rsh->sh_owner != NO_OWNER)
    {
        autoRaiseShields(IS, TRUE);
    }
    return damRes;
}

/*
 * attackShip - the current player is attacking a ship.
 */

void attackShip(IMP, ULONG shipNumber, USHORT damage, AttackType_t at,
    char *prefix)
{
    register Ship_t *rsh;
    ShipDamRes_t damRes;
    USHORT owner;

    server(IS, rt_lockShip, shipNumber);
    updateShip(IS);
    rsh = &IS->is_request.rq_u.ru_ship;
    owner = rsh->sh_owner;

    damRes = damageShip(IS, shipNumber, damage, TRUE);
    server(IS, rt_unlockShip, shipNumber);
    switch (at)
    {
#ifdef FOOBAR
        case at_shell:
            news(IS, n_shell_ship, IS->is_player.p_number, owner);
            break;
#endif
        case at_blaser:
            news(IS, n_blase_ship, IS->is_player.p_number, owner);
            break;
    }
    if (rsh->sh_owner == NO_OWNER)
    {
        switch (at)
        {
            case at_torp:
                news(IS, n_torp_dest, IS->is_player.p_number, owner);
                break;
            default:
                news(IS, n_ship_dest, IS->is_player.p_number, owner);
                break;
        }
    }
    switch (at)
    {
        case at_bomb:
            user(IS, "Bomb");
            break;
        case at_shell:
            user(IS, "Shell");
            break;
        case at_torp:
            user(IS, "Torpedo");
            break;
        case at_blaser:
            user(IS, "Blast");
            break;
    }
    userN3(IS, " does ", damage, "% damage to ");
    user(IS, getShipName(rsh->sh_type));
    userN2(IS, " #", shipNumber);
    if (rsh->sh_owner == NO_OWNER)
    {
        user(IS, " - destroyed");
    }
    user(IS, "!\n");
    IS->is_noWrite = TRUE;
    user(IS, prefix);
    userSp(IS);
    switch (at)
    {
        case at_bomb:
            user(IS, "bombed");
            break;
        case at_shell:
            user(IS, "fired at");
            break;
        case at_torp:
            user(IS, "torpedoed");
            break;
        case at_blaser:
            user(IS, "fired a blaser at");
            break;
    }
    if (rsh->sh_owner == NO_OWNER)
    {
        user(IS, " and sank");
    }
    user2(IS, " your ", getShipName(rsh->sh_type));
    userN3(IS, " #", shipNumber, "!\n");
    if (damRes.sdr_shields != 0)
    {
        userN3(IS, "Your shields absorbed ",
            damRes.sdr_shields, " units.\n");
    }
    if (damRes.sdr_armour != 0)
    {
        userN3(IS, "Your armour absorbed ",
            damRes.sdr_armour, " units.\n");
    }
    if (damRes.sdr_main != 0)
    {
        userN3(IS, "Various other parts of the ship "
            "took ", damRes.sdr_main, " units of"
            "damage.\n");
    }
    notify(IS, owner);
    IS->is_noWrite = FALSE;
}

/*
 * verifyCheckPoint - ask for and check a checkpoint code for access to
 *      another player's planet.
 */

BOOL verifyCheckPoint(IMP, Planet_t *p, CheckPointType_t cpt)
{
    BOOL ok;
    char *spPos;
    char pswd[PLAN_PSWD_LEN];

    feCheckReq(IS, p->pl_number);
    userP(IS, "Checkpoint code for planet #", p, ": ");
    uFlush(IS);
    ok = clReadUser(IS);
    if (ok)
    {
        /* look for a string break */
        spPos = strchr(&p->pl_checkpoint[0], '!');
        if ((spPos == NULL) || (spPos == &p->pl_checkpoint[0]))
        {
            /* none or it is the first character, so make sure they typed */
            /* the exact string */
            if (strcmp(&IS->is_textIn[0], &p->pl_checkpoint[0]) == 0)
            {
                return TRUE;
            }
            err(IS, "access denied!!");
            return FALSE;
        }
        /* do they just need landing access? */
        if (cpt == cpt_land)
        {
            /* see if they used the access password */
            /* make sure there is a valid access password for them to enter */
            spPos += sizeof(char);
            if (*spPos != '\0')
            {
                /* there is, so check it */
                if (strcmp(&IS->is_textIn[0], spPos) == 0)
                {
                    /* got it */
                    return TRUE;
                }
            }
            /* reposition the pointer */
            spPos -= sizeof(char);
            /* copy the string */
            strcpy(&pswd[0], &p->pl_checkpoint[0]);
            /* bisect it at the correct place */
            pswd[spPos - &p->pl_checkpoint[0]] = '\0';
            /* and now verify that part of it */
            if (strcmp(&IS->is_textIn[0], &pswd[0]) == 0)
            {
                return TRUE;
            }
            err(IS, "access denied!!");
            return FALSE;
        }
        else
        {
            /* make sure there is a valid password for them to enter */
            spPos += sizeof(char);
            if (*spPos == '\0')
            {
                /* nope - they are only allowed to land */
                err(IS, "access denied!!");
                return FALSE;
            }
            if (strcmp(&IS->is_textIn[0], spPos) == 0)
            {
                return TRUE;
            }
            err(IS, "access denied!!");
            return FALSE;
        }
    }
    err(IS, "access denied!!");
    return FALSE;
}

/*
 * removeSmallShipItem - removes a small item from a ship and updates the
 *          ships cargo field.
 */

void removeSmallShipItem(IMP, Ship_t *sh, ItemType_t item, USHORT qty)
{
    sh->sh_items[item] -= qty;
    sh->sh_cargo -= (qty * IS->is_world.w_weight[item]);
}

/*
 * removeBigShipItem - removes a small item from a ship and updates the
 *          ships cargo field.
 *          This assumes that you are processing the big item later,
 *          and you will set the correct flags in the big item to
 *          indicate it's new location.
 */

void removeBigShipItem(IMP, Ship_t *sh, BigItem_t *bi)
{
    register USHORT i, j;
    USHORT cutoff;
    ULONG *pnArray = NULL;
    ItemType_t item = 0;

    switch(bi->bi_part)
    {
        case bp_computer:
            cutoff = MAX_COMP;
            pnArray = &sh->sh_computer[0];
            item = it_computers;
            break;
        case bp_engines:
            cutoff = MAX_ENG;
            pnArray = &sh->sh_engine[0];
            item = it_engines;
            break;
        case bp_lifeSupp:
            cutoff = MAX_LIFESUP;
            pnArray = &sh->sh_lifeSupp[0];
            item = it_lifeSupp;
            break;
        case bp_sensors:
        case bp_teleport:
        case bp_shield:
        case bp_tractor:
            cutoff = MAX_ELECT;
            pnArray = &sh->sh_elect[0];
            item = it_elect;
            break;
        case bp_photon:
        case bp_blaser:
            cutoff = MAX_WEAP;
            pnArray = &sh->sh_weapon[0];
            item = it_weapons;
            break;
        default:
            cutoff = 0;
            break;
    }
    for (i = 0; i < cutoff; i++)
    {
        if (pnArray[i] == bi->bi_number)
        {
            sh->sh_items[item]--;
            sh->sh_cargo -= bi->bi_weight;
            j = i;
            while (j < cutoff - 1)
            {
                pnArray[j] = pnArray[j + 1];
                j++;
            }
            pnArray[j] = NO_ITEM;
            i = cutoff;
        }
    }
}

/*
 * torpCost - deduct mobility and shell cost for using photon torpedos.
 *      Assumption - the ship is locked.
 */

void torpCost(IMP)
{
    register Ship_t *rsh;
    register USHORT shells;

    rsh = &IS->is_request.rq_u.ru_ship;
    shells = rsh->sh_items[it_missiles];
    if (shells >= IS->is_world.w_torpCost)
    {
        removeSmallShipItem(IS, rsh, it_missiles, IS->is_world.w_torpCost);
        if (IS->is_world.w_torpMobCost)
        {
            if (rsh->sh_fuelLeft >= IS->is_world.w_torpMobCost)
            {
                rsh->sh_fuelLeft -= IS->is_world.w_torpMobCost;
            }
            else
            {
                rsh->sh_fuelLeft = 0;
            }
        }
    }
}

/*
 * isInst - returns TRUE if the given item number is installed in the ship
 */

BOOL isInst(register Ship_t *sh, BigPart_t part, register ULONG iNum)
{
    register USHORT count, iMax;
    register ULONG *pArr;

    /* switch based on item type */
    switch(part)
    {
        case bp_computer:
            pArr = &sh->sh_computer[0];
            iMax = MAX_COMP;
            break;
        case bp_engines:
            pArr = &sh->sh_engine[0];
            iMax = MAX_ENG;
            break;
        case bp_lifeSupp:
            pArr = &sh->sh_lifeSupp[0];
            iMax = MAX_LIFESUP;
            break;
        case bp_sensors:
        case bp_teleport:
        case bp_shield:
        case bp_tractor:
            pArr = &sh->sh_elect[0];
            iMax = MAX_ELECT;
            break;
        case bp_photon:
        case bp_blaser:
            pArr = &sh->sh_weapon[0];
            iMax = MAX_WEAP;
            break;
        default:
            return FALSE;
    }
    /* scan through list looking for the item */
    for (count = 0; count < iMax; count++)
    {
        if (pArr[count] == NO_ITEM)
        {
            /* hit the end of the items */
            return FALSE;
        }
        if (pArr[count] == iNum)
        {
            /* found it!, so return TRUE */
            return TRUE;
        }
    }
    /* all slots are installed */
    return FALSE;
}

/*
 * numInst - returns the number of the given items installed in the ship
 *          May read in big items, so ship must NOT be in the buffer!
 */

USHORT numInst(IMP, register Ship_t *sh, BigPart_t part)
{
    register USHORT count, iMax, curIt;
    register ULONG *pArr;
    register BigItem_t *bip;

    bip = &IS->is_request.rq_u.ru_bigItem;
    /* switch based on item type */
    switch(part)
    {
        case bp_computer:
            pArr = &sh->sh_computer[0];
            iMax = MAX_COMP;
            break;
        case bp_engines:
            pArr = &sh->sh_engine[0];
            iMax = MAX_ENG;
            break;
        case bp_lifeSupp:
            pArr = &sh->sh_lifeSupp[0];
            iMax = MAX_LIFESUP;
            break;
        case bp_sensors:
        case bp_teleport:
        case bp_shield:
        case bp_tractor:
            pArr = &sh->sh_elect[0];
            iMax = MAX_ELECT;
            break;
        case bp_photon:
        case bp_blaser:
            pArr = &sh->sh_weapon[0];
            iMax = MAX_WEAP;
            break;
        default:
            return 0;
    }
    /* scan through list looking for a vacant slot */
    count = 0;
    for (curIt = 0; curIt < iMax; curIt++)
    {
        if (pArr[curIt] == NO_ITEM)
        {
            /* Hit end of items, so return the number of units so far */
            return count;
        }
        switch(part)
        {
            /* put here anything that shares an array with other items */
            case bp_sensors:
            case bp_teleport:
            case bp_shield:
            case bp_tractor:
            case bp_photon:
            case bp_blaser:
                server(IS, rt_readBigItem, pArr[curIt]);
                if (bip->bi_part == part)
                {
                    count++;
                }
                break;
            default:
                /* the default handles other items which have their own */
                /* array, and thus we don't need to find the item type */
                count++;
                break;
        }
    }
    /* all slots are installed */
    return count;
}

/*
 * numInstPl - returns the number of the given items installed in the planet
 *          May read in big items, so planet must NOT be in the buffer!
 */

USHORT numInstPl(IMP, register Planet_t *pl, BigPart_t part)
{
    register USHORT count, iMax, curIt;
    register ULONG *pArr;
    register BigItem_t *bip;

    bip = &IS->is_request.rq_u.ru_bigItem;
    /* switch based on item type */
    switch(part)
    {
        case bp_photon:
        case bp_blaser:
            pArr = &pl->pl_weapon[0];
            iMax = MAX_WEAP_PL;
            break;
        default:
            return 0;
    }
    /* scan through list looking for a vacant slot */
    count = 0;
    for (curIt = 0; curIt < iMax; curIt++)
    {
        if (pArr[curIt] == NO_ITEM)
        {
            /* Hit end of items, so return the number of units so far */
            return count;
        }
        switch(part)
        {
            /* put here anything that shares an array with other items */
            case bp_photon:
            case bp_blaser:
                server(IS, rt_readBigItem, pArr[curIt]);
                if (bip->bi_part == part)
                {
                    count++;
                }
                break;
            default:
                /* the default handles other items which have their own */
                /* array, and thus we don't need to find the item type */
                count++;
                break;
        }
    }
    /* all slots are installed */
    return count;
}

/*
 * findFree - returns the number of the first free area in the given
 *          ship for the given item, or the max for that type of item
 */

USHORT findFree(register Ship_t *sh, BigPart_t part)
{
    register USHORT iMax, curIt;
    register ULONG *pArr;

    /* switch based on item type */
    switch(part)
    {
        case bp_computer:
            pArr = &sh->sh_computer[0];
            iMax = MAX_COMP;
            break;
        case bp_engines:
            pArr = &sh->sh_engine[0];
            iMax = MAX_ENG;
            break;
        case bp_lifeSupp:
            pArr = &sh->sh_lifeSupp[0];
            iMax = MAX_LIFESUP;
            break;
        case bp_sensors:
        case bp_teleport:
        case bp_shield:
        case bp_tractor:
            pArr = &sh->sh_elect[0];
            iMax = MAX_ELECT;
            break;
        case bp_photon:
        case bp_blaser:
            pArr = &sh->sh_weapon[0];
            iMax = MAX_WEAP;
            break;
        default:
            return 0;
    }
    /* scan through list looking for a vacant slot */
    for (curIt = 0; curIt < iMax; curIt++)
    {
        if (pArr[curIt] == NO_ITEM)
        {
            /* found one, so return the number of units so far */
            return curIt;
        }
    }
    /* all slots are installed */
    return iMax;
}

/*
 * cvtItBp - convertes an ItemType_t to a BigPart_t
 *          Note that not all types can be supported!
 */

BigPart_t cvtItBp(ItemType_t it)
{
    switch(it)
    {
        case it_computers:
            return bp_computer;
        case it_engines:
            return bp_engines;
        case it_lifeSupp:
            return bp_lifeSupp;
        default:
            return 0xff;
    }
}

/*
 * cvtBpIt - converts a BigPart_t item to a ItemType_t item. Note that
 *          some types are not supported, and will return 0xff, while other
 *          types may map several big parts to the same item type
 */

ItemType_t cvtBpIt(BigPart_t bp)
{
    switch(bp)
    {
        case bp_computer:
            return it_computers;
        case bp_engines:
            return it_engines;
        case bp_lifeSupp:
            return it_lifeSupp;
        case bp_sensors:
            return it_elect;
        case bp_teleport:
            return it_elect;
        case bp_photon:
            return it_weapons;
        case bp_blaser:
            return it_weapons;
        case bp_tractor:
            return it_elect;
        case bp_shield:
            return it_elect;
        default:
            return 0xFF;
    }
}

/*
 * readSectBuf - reads in the mapped sector (and it's planets, if any)
 *          into the global "sector array" for later use
 */

void readSectBuf(IMP, register USHORT mapped)
{
    register Planet_t *rpl;
    register USHORT which;
    Sector_t saveSect;

    server(IS, rt_readSector, mapped);
    IS->is_sectBuf.sb_sector = mapped;
    IS->is_sectBuf.sb_worldCount = IS->is_world.w_planetNext;
    IS->is_sectBuf.sb_plCount = IS->is_request.rq_u.ru_sector.s_planetCount;
    which = 0;
    rpl = &IS->is_request.rq_u.ru_planet;
    saveSect = IS->is_request.rq_u.ru_sector;
    while (which < IS->is_sectBuf.sb_plCount)
    {
        /* make sure the planet number is not invalid */
        if (saveSect.s_planet[which] == NO_ITEM)
        {
            /* it is, so decrease the number of planets and return */
            IS->is_sectBuf.sb_plCount = which;
            return;
        }
        server(IS, rt_readPlanet, saveSect.s_planet[which]);
        IS->is_sectBuf.sb_planet[which].sbp_row = rpl->pl_row;
        IS->is_sectBuf.sb_planet[which].sbp_col = rpl->pl_col;
        IS->is_sectBuf.sb_planet[which].sbp_number = rpl->pl_number;
        IS->is_sectBuf.sb_planet[which].sbp_class = rpl->pl_class;
        which++;
    }
}

/*
 * whichPlanet - finds the planet at the passed row and column, if any.
 *          Returns NO_ITEM if there is no planet there.
 *          Reads in the sector and possibly all of the planets in the
 *          sector, if needed, into the global "sector array" in ImpState
 *          for future use.
 */

ULONG whichPlanet(IMP, register USHORT row, register USHORT col)
{
    register USHORT mapped, which;

    mapped = mapSector(IS, row, col);
    /* check if this was the last sector buffered, and that if it was,  */
    /* that there have been no new planets that may have invalidated    */
    /* this info. Since we are only storing the invariable stats of the */
    /* sector, this should normally be good enough to insure valid data */
    if ((mapped != IS->is_sectBuf.sb_sector) || (IS->is_world.w_planetNext !=
        IS->is_sectBuf.sb_worldCount))
    {
        /* we have to read in the info again */
        readSectBuf(IS, mapped);
    }
    which = 0;
    /* scan through each planet in the array */
    while (which < IS->is_sectBuf.sb_plCount)
    {
        if ((IS->is_sectBuf.sb_planet[which].sbp_row == row) &&
            (IS->is_sectBuf.sb_planet[which].sbp_col == col))
        {
            /* found the planet at that location */
            return IS->is_sectBuf.sb_planet[which].sbp_number;
        }
        which++;
    }
    /* we never found the planet */
    return NO_ITEM;
}

/*
 * abandonPlanet - remove planet from players & players race count
 *          and remove planet ownership. Messages will be sent if the
 *          current player is not the planet owner
 */

void abandonPlanet(IMP, register Planet_t *pl)
{
    register USHORT owner, ownRace;
    register Player_t *rp;
    register World_t *rw;
    Planet_t savePl;

    owner = pl->pl_owner;
    ownRace = pl->pl_ownRace;
    savePl = *pl;
    server(IS, rt_lockPlanet, pl->pl_number);
    pl = &IS->is_request.rq_u.ru_planet;
    rp = &IS->is_request.rq_u.ru_player;
    rw = &IS->is_request.rq_u.ru_world;
    pl->pl_lastOwner = owner;
    pl->pl_transfer = pt_peacefull;
    pl->pl_owner = NO_OWNER;
    pl->pl_ownRace = NO_RACE;
    server(IS, rt_unlockPlanet, pl->pl_number);
    /* make sure the owner is valid */
    if (owner < PLAYER_MAX)
    {
        server(IS, rt_lockPlayer, owner);
        rp->p_planetCount--;
        server(IS, rt_unlockPlayer, owner);
        /* if the player is the owner then decrement his planet counter */
        if (owner == IS->is_player.p_number)
        {
            IS->is_player.p_planetCount = rp->p_planetCount;
        }
        else
        {
            user2(IS, "Player ", &IS->is_player.p_name[0]);
            userP(IS, "has evacuated your planet ", &savePl, "!");
            notify(IS, owner);
        }
    }
    userP(IS, "You just evacuated planet ", &savePl, "!\n");
    if (ownRace != NO_RACE)
    {
        server(IS, rt_lockWorld, 0);
        rw->w_race[ownRace].r_planetCount--;
        /* see if this was the last planet (the home planet) */
        /* This should probobly NEVER happen */
        if (rw->w_race[ownRace].r_planetCount == 0)
        {
            /* set race status to dead */
            rw->w_race[ownRace].r_status = rs_dead;
        }
        server(IS, rt_unlockWorld, 0);
        IS->is_world.w_race[ownRace] = rw->w_race[ownRace];
        /* see if this killed the race */
        if (IS->is_world.w_race[ownRace].r_status == rs_dead)
        {
            if (ownRace == IS->is_player.p_race)
            {
                user(IS, "Which destroyed your own race!\n");
            }
            else
            {
                user3(IS, "Which destroyed race ",
                    &IS->is_world.w_race[ownRace].r_name[0], "!\n");
            }
            news(IS, n_destroyed, IS->is_player.p_number, ownRace);
        }
    }
}

/*
 * takeNewPlanet - adds planet to players & players race count
 *          and set planet ownership
 */

void takeNewPlanet(IMP, register Planet_t *pl)
{
    register ULONG plNum;
    register Planet_t *rp;
    BOOL  didRestore;

    /* save the planet number */
    plNum = pl->pl_number;

    didRestore = FALSE;

    /* now increment the players planet count */
    server(IS, rt_lockPlayer, IS->is_player.p_number);
    IS->is_request.rq_u.ru_player.p_planetCount++;
    server(IS, rt_unlockPlayer, IS->is_player.p_number);
    /* and copy the new count into the current player */
    IS->is_player.p_planetCount = IS->is_request.rq_u.ru_player.p_planetCount;

    /* now increase the race planet count */
    if (IS->is_player.p_race != NO_RACE)
    {
        server(IS, rt_lockWorld, 0);
        IS->is_request.rq_u.ru_world.w_race[IS->is_player.p_race
            ].r_planetCount++;
        if (IS->is_request.rq_u.ru_world.w_race[IS->is_player.p_race
            ].r_status == rs_dead)
        {
            if (IS->is_request.rq_u.ru_world.w_race[IS->is_player.p_race
                ].r_homePlanet == plNum)
            {
                IS->is_request.rq_u.ru_world.w_race[IS->is_player.p_race
                    ].r_status = rs_active;
                didRestore = TRUE;
            }
        }
        IS->is_world.w_race[IS->is_player.p_race] =
            IS->is_request.rq_u.ru_world.w_race[IS->is_player.p_race];
        server(IS, rt_unlockWorld, 0);
    }

    /* lock the planet */
    server(IS, rt_lockPlanet, plNum);
    /* point to the buffer */
    rp = &IS->is_request.rq_u.ru_planet;
    /* set the new owner */
    if (didRestore)
    {
        rp->pl_owner = NO_OWNER;
    }
    else
    {
        rp->pl_owner = IS->is_player.p_number;
    }
    /* set the new owning race */
    rp->pl_ownRace = IS->is_player.p_race;
    rp->pl_transfer = pt_peacefull;
    /* unlock the planet */
    server(IS, rt_unlockPlanet, plNum);

    if (didRestore)
    {
        userP(IS, "You recovered your home planet ", rp, "!\n");
    }
    else
    {
        userP(IS, "You now own planet ", rp, "!\n");
        /* make the news item */
        news(IS, n_took_unoccupied, IS->is_player.p_number, 0);
    }
}

/*
 * getShipCapacity - returns the number of passed items the ship can carry
 *          Used for small-items, which have no independant weight
 *          associated with them that can be checked.
 */

USHORT getShipCapacity(IMP, ItemType_t what, register Ship_t *sh)
{
    USHORT capacity;

    /* make sure ship type is valid */
    if ((sh->sh_type > ST_LAST) || (what > IT_LAST_SMALL))
    {
        /* it isn't, so don't let anything get loaded */
        return 0;
    }
    /* special case for miner ship classes */
    if ((sh->sh_type == st_m) && ((what != it_ore) && (what != it_bars)))
    {
        return 0;
    }
    /* get the amount of cargo space left */
    capacity = IS->is_world.w_shipCargoLim[sh->sh_type] - sh->sh_cargo;
    return (capacity / IS->is_world.w_weight[what]);
}

/*
 * getLowHi - fills in the lowest and highest planet numbers in the sector
 *          in the buffer, if they are lesser or greater than the values
 *          already present
 */

void getLowHi(IMP, register ULONG *minPl, register ULONG *maxPl)
{
    register Sector_t *rs;
    register USHORT whichPl, lastPlan;

    rs = &IS->is_request.rq_u.ru_sector;
    lastPlan = rs->s_planetCount;
    for (whichPl = 0; whichPl < lastPlan; whichPl++)
    {
        if (rs->s_planet[whichPl] != NO_ITEM)
        {
            if (rs->s_planet[whichPl] < *minPl)
            {
                *minPl = rs->s_planet[whichPl];
            }
            if (rs->s_planet[whichPl] > *maxPl)
            {
                *maxPl = rs->s_planet[whichPl];
            }
        }
    }
}

/*
 * getPlRange - returns the highest and lowest planet numbers that must
 *          be read for a given sector position and range.
 *          Destroys the buffer
 */

void getPlRange(IMP, ULONG *minPl, ULONG *maxPl, USHORT r, USHORT c,
    USHORT rang)
{
    long row, col;

    row = (long) r;
    col = (long) c;

    if (row - 10 >= 0)
    {
        if (col - 10 >= 0)
        {
            server(IS, rt_readSector, (ULONG) mapSector(IS, (USHORT) r - 10, (USHORT)c - 10));
            getLowHi(IS, minPl, maxPl);
        }
        server(IS, rt_readSector, (ULONG) mapSector(IS, r - 10, c));
        getLowHi(IS, minPl, maxPl);
        if (c + 10 <= (((IS->is_world.w_columns - 1) * 10) + 9))
        {
            server(IS, rt_readSector, (ULONG) mapSector(IS, r - 10, c + 10));
            getLowHi(IS, minPl, maxPl);
        }
    }
    if (col - 10 >= 0)
    {
        server(IS, rt_readSector, (ULONG) mapSector(IS, r, c - 10));
        getLowHi(IS, minPl, maxPl);
    }
    server(IS, rt_readSector, mapSector(IS, r, c));
    getLowHi(IS, minPl, maxPl);
    if (c + 10 <= (((IS->is_world.w_columns - 1) * 10) + 9))
    {
        server(IS, rt_readSector, (ULONG) mapSector(IS, r, c + 10));
        getLowHi(IS, minPl, maxPl);
    }
    if (r + 10 <= (((IS->is_world.w_rows - 1) * 10) + 9))
    {
        if (col - 10 >= 0)
        {
            server(IS, rt_readSector, (ULONG) mapSector(IS, r + 10, c - 10));
            getLowHi(IS, minPl, maxPl);
        }
        server(IS, rt_readSector, (ULONG) mapSector(IS, r + 10, c));
        getLowHi(IS, minPl, maxPl);
        if (c + 10 <= (((IS->is_world.w_columns - 1) * 10) + 9))
        {
            server(IS, rt_readSector, (ULONG) mapSector(IS, (USHORT) r + 10, (USHORT) c + 10));
            getLowHi(IS, minPl, maxPl);
        }
    }
}

/*
 * isHomePlanet - returns the number of the race that owns the planet,
 *          or NO_RACE if none do
 */

USHORT isHomePlanet(IMP, register ULONG plNum)
{
    register USHORT race;

    for (race = 0; race < RACE_MAX; race++)
    {
        if (IS->is_world.w_race[race].r_homePlanet == plNum)
        {
            return race;
        }
    }
    return NO_RACE;
}

/*
 * doUnInstall - removes a big item of the given type from the ships
 *          list and leaves it as transportable cargo
 */

void doUninstall(Ship_t *sh, register ULONG biNum, BigPart_t what)
{
    register ULONG *pArr;
    register USHORT iMax, which;

    /* set up the pointer to the arrays, and the limit */
    switch(what)
    {
        case bp_computer:
            pArr = &sh->sh_computer[0];
            iMax = MAX_COMP;
            break;
        case bp_engines:
            pArr = &sh->sh_engine[0];
            iMax = MAX_ENG;
            break;
        case bp_lifeSupp:
            pArr = &sh->sh_lifeSupp[0];
            iMax = MAX_LIFESUP;
            break;
        case bp_sensors:
        case bp_teleport:
        case bp_shield:
        case bp_tractor:
            pArr = &sh->sh_elect[0];
            iMax = MAX_ELECT;
            break;
        case bp_photon:
        case bp_blaser:
            pArr = &sh->sh_weapon[0];
            iMax = MAX_WEAP;
            break;
        default:
            return;
    }

    /* now loop for each of the items in the ship of that type */
    which = 0;
    while (which < iMax)
    {
        /* is the current item the one we want? */
        if (pArr[which] == biNum)
        {
            /* yes, so loop until the last item */
            while (which < iMax - 1)
            {
                /* copy the item numbers down */
                pArr[which] = pArr[which + 1];
                which++;
            }
            /* set the last item to NO_ITEM */
            pArr[which] = NO_ITEM;
            return;
        }
        which++;
    }
}

/* Sends an email */
void sendEmail(IMP, const char *sendto, const char *subject, const char *emailbody)
{
	char cmd[1024]; /* holds the cli command */
	char tempFile[100];

	strcpy(tempFile, tempnam("/tmp", "sendmail")); /* generate temp file name. */
	FILE *fp = fopen(tempFile, "w"); /* open it for writing. */
	fprintf(fp, "From: %s\r\n", &IS->is_world.w_emailAddress[0]); /* add sender */
	fprintf(fp, "Subject: %s\r\n", subject); /* Add subject */
	fprintf(fp, "\r\n"); /* seperate headers from body */
	fprintf(fp, "%s\r\n", emailbody); /* write body to it. */
	fclose(fp); /* close it. */
	sprintf(cmd, "/usr/sbin/sendmail %s < %s", sendto, tempFile); /* prepare command. */
	(void) system(cmd); /* execute it. */
	/* remove temp file */
	unlink(tempFile);
}

/* Sends a system email */
void sendSystemEmail(IMP, const char *subject, const char *emailbody)
{
	char cmd[255]; /* holds the cli command */
	char tempFile[100];
	strcpy(tempFile, tempnam("/tmp", "sendmail")); /* generate temp file name. */
	FILE *fp = fopen(tempFile, "w"); /* open it for writing. */
	fprintf(fp, "Subject: %s\r\n", subject); /* write body to it. */
	fprintf(fp, "\r\n"); /* seperate headers from body */
	fprintf(fp, "%s\r\n", emailbody); /* write body to it. */
	fclose(fp); /* close it. */
	sprintf(cmd, "/usr/sbin/sendmail %s < %s", &IS->is_world.w_emailAddress[0], tempFile); /* prepare command. */
	(void) system(cmd); /* execute it. */
	/* remove temp file */
	unlink(tempFile);
}

/*
 *  displayTime - returns the human readable time string
 */

char * displayTime(ULONG t)
{
    static char buffer[30];

    strcpy(&buffer[0], ctime(&t));
    /* eliminate the '\n' that ctime() adds */
    buffer[strlen(buffer) - 1] = '\0';
    return(&buffer[0]);
}

/*
 * publishStats - send our stats to the global server
 */

void publishStats(IMP)
{
    char cmd[255]; /* holds the cli command */
    char tempFile[100];
    char emailbody[2048];

    log3(IS, "*** ", "Starting to publish stats", " ***");
    sprintf(emailbody, "sname=%s\ncurpl=%u\nmaxpl=%u\nw_row=%u\nw_col=%u\nmaxcn=%u\nmaxbt=%u\nsrvvr=%s\nstdat=%s\n",
		    "server name will be here after next server update", /* required DB change */
		    IS->is_world.w_currPlayers,
		    IS->is_world.w_maxPlayers,
		    IS->is_world.w_rows,
		    IS->is_world.w_columns,
		    IS->is_world.w_maxConnect,
		    IS->is_world.w_maxBTUs,
		    IMP_BASE_REV,
		    displayTime(IS->is_world.w_buildDate));
    /* generate temp file name. */
    strcpy(tempFile, tempnam("/tmp", "sendmail")); /* generate temp file name. */
    FILE *fp = fopen(tempFile, "w"); /* open it for writing. */
    fprintf(fp, "Subject: Server Publish Data\r\n"); /* write body to it. */
    fprintf(fp, "\r\n"); /* seperate headers from body */
    fprintf(fp, "%s\r\n", emailbody); /* write body to it. */
    fclose(fp); /* close it. */
    sprintf(cmd, "/usr/sbin/sendmail %s < %s", SENDPUBTO, tempFile); /* prepare command. */
    (void) system(cmd); /* execute it. */
    /* remove temp file */
    unlink(tempFile);
    log3(IS, "*** ", "Publishing complete", " ***");
}

