/*
 * Imperium
 *
 * $Id: cmd_move.c,v 1.2 2000/05/18 06:50:02 marisa Exp $
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
 * $Log: cmd_move.c,v $
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:51  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:24:19  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:11  marisag
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
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"

static const char rcsid[] = "$Id: cmd_move.c,v 1.2 2000/05/18 06:50:02 marisa Exp $";

#define LIFTOFF_COST    10
#define LAND_COST       5

/*
 * moveDir - get cost and move code from one character of moving.
 *          Will not allow you to move outside the worlds dimensions
 *          (expects scaled row and column numbers)
 */

Move_t moveDir(IMP, char dir, USHORT *pRow, USHORT *pCol, ULONG *pCost)
{
    ULONG cost;
    register USHORT r, c;
    Move_t result;

    result = m_continue;
    cost = *pCost;
    r = *pRow;
    c = *pCol;
    /* which way are they trying to move */
    switch(dir)
    {
        case '4':
        case '6':
        case '8':
        case '2':
            switch(dir)
            {
                case '8':
                    if (r == 0)
                    {
                        err(IS, "Can't move out of allocated space");
                        result = m_readMore;
                    }
                    else
                    {
                        r--;
                    }
                    break;
                case '2':
                    if (r == (((IS->is_world.w_rows - 1) * 10) + 9))
                    {
                        err(IS, "Can't move out of allocated space");
                        result = m_readMore;
                    }
                    else
                    {
                        r++;
                    }
                    break;
                case '4':
                    if (c == 0)
                    {
                        err(IS, "Can't move out of allocated space");
                        result = m_readMore;
                    }
                    else
                    {
                        c--;
                    }
                    break;
                case '6':
                    if (c == (((IS->is_world.w_columns - 1) * 10) + 9))
                    {
                        err(IS, "Can't move out of allocated space");
                        result = m_readMore;
                    }
                    else
                    {
                        c++;
                    }
                    break;
            }
            break;
        case '7':
        case '9':
        case '1':
        case '3':
            cost = cost * 141 / 100;
            switch(dir)
            {
                case '7':
                    if ((c == 0) || (r == 0))
                    {
                        err(IS, "Can't move out of allocated space");
                        result = m_readMore;
                    }
                    else
                    {
                        r--;
                        c--;
                    }
                    break;
                case '3':
                    if ((c == (((IS->is_world.w_columns - 1) * 10) + 9)) ||
                        (r == (((IS->is_world.w_rows - 1) * 10) + 9)))
                    {
                        err(IS, "Can't move out of allocated space");
                        result = m_readMore;
                    }
                    else
                    {
                        r++;
                        c++;
                    }
                    break;
                case '9':
                    if ((c == (((IS->is_world.w_columns - 1) * 10) + 9)) ||
                        (r == 0))
                    {
                        err(IS, "Can't move out of allocated space");
                        result = m_readMore;
                    }
                    else
                    {
                        r--;
                        c++;
                    }
                    break;
                case '1':
                    if ((c == 0) || (r == (((IS->is_world.w_rows - 1) * 10)
                        + 9)))
                    {
                        err(IS, "Can't move out of allocated space");
                        result = m_readMore;
                    }
                    else
                    {
                        r++;
                        c--;
                    }
                    break;
            }
            break;
        case '.':
            result = m_done;
            break;
        case '\0':
            result = m_readMore;
            break;
        default:
            err(IS, "illegal direction character");
            result = m_readMore;
            break;
    }
    *pRow = r;
    *pCol = c;
    *pCost = cost;
    return result;
}

#ifdef BUBBA
/*
 * checkFlak - check for and handle flak aimed at our planes.
 */

void checkFlak(IMP, USHORT r, USHORT c, USHORT *pPlanes, BOOL alwaysFire)
{
    register Sector_t *rs;
    register Ship_t *rsh;
    register USHORT i, planes, killed;
    Sector_t saveSector;
    Ship_t saveShip;
    USHORT guns, shells, aRow, aCol, mapped;

    planes = *pPlanes;
    accessSector(IS, r, c);
    rs = &IS->is_request.rq_u.ru_sector;
    if ((rs->s_type == s_water) || (rs->s_type == s_harbour) ||
        (rs->s_type == s_bridgeSpan))
    {
        if (rs->s_shipCount != 0)
        {
            aRow = r;
            aCol = c;
            i = 0;
            while ((i != IS->is_world.w_shipNext) && (planes != 0))
            {
                server(IS, rt_readShip, i);
                saveShip = *rsh;
                server(IS, rt_readPlayer, saveShip.sh_owner);
                if ((IS->is_request.rq_u.ru_player.p_playrel[
                    IS->is_player.p_number] == r_war) && (saveShip.sh_row ==
                    aRow) && (saveShip.sh_col == aCol))
                {
                    accessShip(IS, i);
                    if ((rsh->sh_owner != NO_OWNER) &&
                        (rsh->sh_efficiency >= EFFIC_WARN)
                        && (rsh->sh_guns != 0) && (rsh->sh_shells != 0))
                    {
                        user(IS, "Whump! Fzzt!!! Zap!! Flak encountered!\n");
                        server(IS, rt_lockShip, i);
                        if (rsh->sh_type == st_tender)
                        {
                            guns = 1;
                        }
                        else
                        {
                            guns = readShipQuan(rsh, it_guns);
                        }
                        guns = umin(guns, rsh->sh_items[it_missiles]);
                        killed = guns * rsh->sh_efficiency * planes / 100;
                        killed = impRandom(killed) / IS->is_world.w_flakFactor;
                        if ((killed == 0) &&
                            (impRandom(IS->is_world.w_flakFactor + 1) < guns))
                        {
                            killed = 1;
                        }
                        killed = umin(umin(killed, planes), guns);
                        rsh->sh_items[it_missiles] -= guns;
                        server(rt_unlockShip, i);
                        if (killed != 0)
                        {
                            userN3("You lost ", killed, " plane(s)\n");
                            planes = planes - killed;
                        }
                        IS->is_noWrite = TRUE;
                        user2("Your ", getShipName(saveShip.sh_type));
                        userN3(" #", i, " shot at overflying plane(s) "
                            "from ");
                        user2(&IS->is_player.c_name[0], ".");
                        if (killed != 0)
                        {
                            userN2("\n", killed);
                            user(" plane(s) were shot down!");
                        }
                        notify(IS, saveShip.sh_owner);
                        IS->is_noWrite = FALSE;
                        news(n_flak, saveShip.sh_owner, IS->is_playerNumber);
                    }
                }
                i++;
            }
        }
    }
    else if (rs->s_type = s_fortress or rs->s_type = s_airport or
          rs->s_type = s_capital or rs->s_type = s_bank) and
        rs->s_quantity[it_missiles] != 0 and rs->s_quantity[it_guns] != 0 and
        rs->s_efficiency >= 60 and rs->s_quantity[it_military] >= 5
    {
        server(rt_readPlayer, rs->s_owner);
        if ((IS->is_request.rq_u.ru_player.c_relations[IS->is_playerNumber] ==
            r_war) || alwaysFire)
        {
            user("Bang!! BOOM!!! Bang!! Flak encountered!\n");
            mapped = mapSector(IS, r, c);
            server(rt_lockSector, mapped);
            guns = readQuan(rs, it_guns);
            guns = umin(guns, IS->is_world.w_gunMax);
            shells = readQuan(rs, it_missiles);
            guns = umin(guns, shells);
            guns = umin(guns, rs->s_quantity[it_military] / 5);
            guns = umin(guns, planes * 3);
            killed = make(guns, ULONG) * rs->s_efficiency * planes / 100;
            killed = impRandom(killed) / IS->is_world.w_flakFactor;
            if killed = 0 and impRandom(IS->is_world.w_flakFactor + 1) < guns
            {
                killed = 1;
            }
            killed = umin(umin(killed, planes), guns);
            writeQuan(rs, it_missiles, readQuan(rs, it_missiles) - guns);
            server(rt_unlockSector, mapped);
            if (killed != 0)
            {
                userN3("You lost ", killed, " plane(s)\n");
                planes = planes - killed;
            }
            saveSector = rs*;
            IS->is_noWrite = TRUE;
            user2("Your ", getDesigName(saveSector.s_type));
            userTarget(saveSector.s_owner, " at ", r, c,
                  " shot at overflying plane(s) from ");
            user2(&IS->is_player.c_name[0], ".");
            if (killed != 0)
            {
                userN3("\n", killed, " plane(s) were shot down!");
            }
            notify(IS, saveSector.s_owner);
            IS->is_noWrite = FALSE;
            news(n_flak, saveSector.s_owner, IS->is_playerNumber);
        }
    }
    pPlanes* = planes;
}
#endif

/*
 * landOnShip - try to land some planes on a ship.
 */

void landOnShip(IMP, USHORT planes, USHORT r, USHORT c, USHORT bombs,
    USHORT fuel)
{
    register Ship_t *rsh;
    register USHORT quan, live, killed;
    USHORT absRow, absCol, capacity;
    ULONG shipNumber;
    BOOL shipHere, ditched, redo;
    ShipDamRes_t damRes;

    rsh = &IS->is_request.rq_u.ru_ship;
    absRow = r;
    absCol = c;
    shipHere = FALSE;
    shipNumber = 0;
    while (!shipHere && (shipNumber < IS->is_world.w_shipNext))
    {
        server(IS, rt_readShip, shipNumber);
        if ((rsh->sh_owner == IS->is_player.p_number) &&
            (rsh->sh_type != st_a) &&
            (absRow == rsh->sh_row) && (absCol == rsh->sh_col))
        {
            shipHere = TRUE;
        }
        shipNumber++;
    }
    ditched = FALSE;
    if (shipHere)
    {
        (void) skipBlanks(IS);
        redo = TRUE;
        while (redo)
        {
            if (reqShip(IS, &shipNumber, "Land on which ship"))
            {
                server(IS, rt_readShip, shipNumber);
                if (rsh->sh_owner != IS->is_player.p_number)
                {
                    err(IS, "you don't own that ship");
                }
                else if (rsh->sh_type == st_a)
                {
                    err(IS, "that ship can't carry planes");
                }
                else if ((rsh->sh_row != absRow) || (rsh->sh_col != absCol))
                {
                    err(IS, "that ship isn't in the same subsector");
                }
                else
                {
                    accessShip(IS, shipNumber);
                    if (rsh->sh_owner != NO_OWNER)
                    {
                        IS->is_noWrite = TRUE;
                        server(IS, rt_lockShip, shipNumber);
                        redo = FALSE;
                        live = damageUnit(IS, planes, 100 -
                            rsh->sh_efficiency);
                        /* live is now the number that landed */
                        killed = planes - live;
                        /* killed is now the number that crashed */
                        planes = umin(live, (USHORT) MAX_WORK - rsh->sh_items[it_planes]);
                        /* planes is now the number that landed */
                        userN(IS, planes);
                        userN3(IS, " plane(s) landed successfully on ship #",
                            shipNumber, "\n");
                        if (killed != 0)
                        {
                            killed = umin((USHORT) 100, killed *
                                (impRandom(IS, IS->is_world.w_planeRand) +
                                        IS->is_world.w_planeBase));
                            userN3(IS, "Crashing planes did ", killed,
                                   "% damage!\n");
                            damRes = damageShip(IS, shipNumber, killed, TRUE);
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
                        }
                        quan = rsh->sh_items[it_planes] + planes;
                        capacity = getShipCapacity(IS, it_planes, rsh);
                        if (quan > capacity)
                        {
                            userN(IS, quan - capacity);
                            user(IS, " planes abandoned in space!\n");
                            planes -= (quan - capacity);
                            quan = capacity;
                        }
                        rsh->sh_items[it_planes] = quan;
                        quan = rsh->sh_items[it_missiles] + planes *
                            bombs;
                        capacity = getShipCapacity(IS, it_missiles, rsh);
                        if (quan > capacity)
                        {
                            userN(IS, quan - capacity);
                            user(IS, " missiles jetisoned into space!\n");
                            quan = capacity;
                        }
                        rsh->sh_items[it_missiles] = quan;
                        quan = rsh->sh_items[it_officers] + planes * 2;
                        capacity = getShipCapacity(IS, it_officers, rsh);
                        if (quan > capacity)
                        {
                            userN(IS, quan - capacity);
                            user(IS, " pilots fell out the airlock!\n");
                            quan = capacity;
                        }
                        rsh->sh_items[it_officers] = quan;
                        fuel =
                            fuel * 10 / getTechFactor(IS->is_player.p_number)
                                / IS->is_world.w_fuelRichness * planes;
                        /* do a bit of inline correction */
                        if (rsh->sh_type > ST_LAST)
                        {
                            rsh->sh_type = ST_BIGGEST;
                        }
                        rsh->sh_fuelLeft =
                            umin(IS->is_world.w_shipCargoLim[rsh->sh_type],
                            rsh->sh_fuelLeft + fuel);
                        rsh->sh_cargo = umin(IS->is_world.w_shipCargoLim[
                            rsh->sh_type], rsh->sh_cargo + fuel);
                        server(IS, rt_unlockShip, shipNumber);
                        IS->is_noWrite = FALSE;
                        uFlush(IS);
                    }
                }
            }
            else
            {
                redo = FALSE;
                ditched = TRUE;
            }
        }
    }
    else
    {
        ditched = TRUE;
    }
    if (ditched && (planes != 0))
    {
        userN2(IS, "You just abandoned ", planes);
        user(IS, " plane(s) in the darkness of space!\n");
    }
}

/*
 * landOnPlanet - attempt to land planes on a planet.
 *      Convention: the planet is updated and in the request.
 */

void landOnPlanet(IMP, USHORT planes, int wea, ULONG plNum, USHORT r,
    USHORT c, USHORT bombs, USHORT fuel)
{
    register Planet_t *rpl;
    register USHORT live, killed, owner;
    USHORT chances, oldMil;

    rpl = &IS->is_request.rq_u.ru_planet;
    owner = rpl->pl_owner;
    chances = 50 - (rpl->pl_efficiency * 40 / 100);
    chances = chances * IS->is_world.w_landScale / 100;
    if (chances > 100)
    {
        chances = 100;
    }
    live = damageUnit(IS, planes, 100 - chances);
    /* live is now the number that landed */
    killed = planes - live;
    /* killed is now the number that crashed */
    planes = umin(live, (USHORT) MAX_WORK - readPlQuan(IS, rpl, it_planes));
    /* planes is now the number that lived */
    userN(IS, planes);
    userP(IS, " plane(s) landed successfully on ", rpl, "\n");
    if (killed != 0)
    {
        if (rpl->pl_owner != IS->is_player.p_number)
        {
            userN(IS, killed);
            user3(IS, " plane(s) from ", &IS->is_player.p_name[0],
                  " crashed ");
            userP(IS, "onto your planet ", rpl, "");
            notify(IS, owner);
        }
        server(IS, rt_lockPlanet, plNum);
        damagePlanet(IS, umin((USHORT) 100, killed *
            (impRandom(IS, IS->is_world.w_planeRand) +
            IS->is_world.w_planeBase)), IS->is_player.p_number);
        server(IS, rt_unlockPlanet, plNum);
    }
    if (planes != 0)
    {
        if (owner == NO_OWNER)
        {
            userP(IS, "You now own planet ", rpl, "\n");
            takeNewPlanet(IS, rpl);
        }
        else if (owner != IS->is_player.p_number)
        {
            userN(IS, planes);
            user3(IS, " plane(s) from ", &IS->is_player.p_name[0],
                  " landed ");
            userP(IS, "on your planet ", rpl, "");
            notify(IS, owner);
        }
        server(IS, rt_lockPlanet, plNum);
        writePlQuan(IS, rpl, it_planes, readPlQuan(IS, rpl, it_planes) +
            planes);
        writePlQuan(IS, rpl, it_missiles,
            umin((USHORT) MAX_WORK, readPlQuan(IS, rpl, it_missiles) + planes *
            bombs));
        oldMil = readPlQuan(IS, rpl, it_officers);
        writePlQuan(IS, rpl, it_officers, umin((USHORT) MAX_WORK, oldMil + planes *
            2));
        adjustForNewWorkers(IS, rpl, it_officers, oldMil);
        fuel = fuel * 10 / getTechFactor(IS->is_player.p_number) /
                    IS->is_world.w_fuelRichness * planes;
	if (MAX_WORK < (rpl->pl_mobility + fuel))
	{
            rpl->pl_mobility = MAX_WORK;
	}
	else
	{
            rpl->pl_mobility += fuel;
	}
        server(IS, rt_unlockPlanet, plNum);
        if (owner == NO_OWNER)
        {
            userP(IS, "You now own planet ", rpl, "\n");
            takeNewPlanet(IS, rpl);
        }
    }
}

#ifdef BUBBA
/*
 * bombLand - drop bombs on a land sector.
 *      Convention: the sector is updated and is in the request.
 */

proc bombLand(USHORT planes; int r, c)void:
    USHORT owner, mapped;

    user("Bombs away...\n");
    user3(&IS->is_player.c_name[0], " bombed your ",
          getDesigName(IS->is_request.rq_u.ru_sector.s_type));
    owner = IS->is_request.rq_u.ru_sector.s_owner;
    userTarget(owner, " at ", r, c, "!");
    notify(IS, owner);
    news(n_bomb_sector, IS->is_playerNumber, owner);
    mapped = mapSector(IS, r, c);
    server(rt_lockSector, mapped);
    damageSector(mapped, umin((USHORT) 100, planes *
            (impRandom(IS, IS->is_world.w_bombRand) + IS->is_world.w_bombBase)),
            IS->is_playerNumber);
    server(rt_unlockSector, mapped);
}

/*
 * bombShip - drop bombs on a ship.
 *      Convention: the sector is updated and is in the request.
 */

proc bombShip(USHORT planes; int r, c)void:
    register *Sector_t rs;
    register *Ship_t rsh @ rs;
    USHORT shipNumber, row, col;

    rs = &IS->is_request.rq_u.ru_sector;
    if rs->s_shipCount = 0 then
        user("There are no ships here - nothing bombed.\n");
    else
        row = rowToAbs(r);
        col = colToAbs(c);
        while
            (void) skipBlanks();
            if reqShip(&shipNumber, "Bomb which ship? ") then
                server(rt_readShip, shipNumber);
                if rsh->sh_owner = NO_OWNER then
                    err("that ship is a rusting hulk");
                    TRUE
                elif rsh->sh_row != row or rsh->sh_col != col then
                    err("that ship isn't below you");
                    TRUE
                elif rsh->sh_owner = IS->is_playerNumber then
                    err("Bomb one of our own ships? You must be kidding!");
                    TRUE
                else
                    user("Bombs away...\n");
                    attackShip(shipNumber,
                               make(planes, ULONG) *
                                    (impRandom(IS, IS->is_world.w_bombRand) +
                                     IS->is_world.w_bombBase),
                               at_bomb, &IS->is_player.c_name[0]);
                    FALSE
                fi
            else
                user("OK, nothing bombed.\n");
                FALSE
            fi
        do
        }
    }
}

/*
 * flyOnce - fly a group of planes for one unit.
 */

proc flyOnce(**char ptr; *int pRow, pCol; *USHORT pPlanes, pBombs, pFuel;
             BOOL gotEnemy)Move_t:
    Sector_t saveSector;
    register *Sector_t rs;
    int r, c, wea;
    ULONG cost;
    USHORT planes, fuel, bombs, live, killed, mapped, owner;
    char ch;
    Move_t result;
    BOOL noCheck;

    noCheck = FALSE;
    planes = pPlanes*;
    bombs = pBombs*;
    fuel = pFuel*;
    ch = ptr**;
    r = pRow*;
    c = pCol*;
    rs = &IS->is_request.rq_u.ru_sector;
    mapped = mapSector(IS, r, c);
    wea = weather(rowToAbs(r), colToAbs(c));
    /* scale cost by 100 to avoid rounding problems */
    cost = (bombs + 2) / 3 * 100 + bombs * 20 + 100;
    if ch = 'v' or ch = '5' then
        /* snoop down at the sector below */
        result = m_continue;
        noCheck = TRUE;
        ptr* = ptr* + 1;
        if gotEnemy then
            checkFlak(r, c, &planes, FALSE);
        }
        server(rt_readSector, mapped);
        userS("Current sector: ", r, c, ": ");
        userN(rs->s_efficiency / 10 * 10);
        user3("% ", getDesigName(rs->s_type), "\n");
    elif ch = '0' then
        /* drop some bombs */
        result = m_continue;
        noCheck = TRUE;
        ptr* = ptr* + 1;
        if bombs = 0 then
            err("you have no bombs");
        else
            accessSector(r, c);
            if rs->s_type = s_water then
                bombShip(planes, r, c);
            elif rs->s_type = s_harbour or rs->s_type = s_bridgeSpan then
                ptr** = '\0';
                if ask("Attempt to bomb a ship? ") then
                    bombShip(planes, r, c);
                else
                    bombLand(planes, r, c);
                }
            else
                bombLand(planes, r, c);
            }
            bombs = bombs - 1;
            pBombs* = bombs;
            if rs->s_type = s_bridgeHead and rs->s_efficiency < 20 then
                collapseSpans(r, c);
            }
            checkFlak(r, c, &planes, TRUE);
        }
    elif ch = '.' then
        /* try to land here */
        result = m_done;
        ptr* = ptr* + 1;
        accessSector(r, c);
        if rs->s_type = s_water then
            landOnShip(planes, wea, r, c, bombs, fuel);
        elif rs->s_type = s_harbour or rs->s_type = s_bridgeSpan then
            ptr** = '\0';
            if ask("Attempt to land on a carrier? ") then
                landOnShip(planes, wea, r, c, bombs, fuel);
            else
                landOnLand(planes, wea, r, c, bombs, fuel);
            }
        else
            landOnLand(planes, wea, r, c, bombs, fuel);
        }
        planes = 0;
    else
        result = moveDir(IS, ptr, &r, &c, &cost);
    }
    if result = m_continue and not noCheck then
        /* add to cost for bad weather */
        if wea <= HURRICANE_FORCE then
            cost = cost * 3;
            user("Your planes face an almost irresistable headwind...\n");
        elif wea <= MONSOON_FORCE then
            cost = cost * 2;
            user("The planes are buffeted, but sustain no damage...\n");
        elif wea <= STORM_FORCE then
            cost = cost * 3 / 2;
            user("The planes fly above the storm with no problem\n");
        }
        /* unscale cost back to the way fuel is scaled: */
        cost = (cost + 9) / 10;
        if cost > fuel then
            err("insufficient fuel");
            result = m_readMore;
        else
            server(rt_readSector, mapped);
            if wea <= HURRICANE_FORCE then
                live = damageUnit(IS, planes, impRandom(IS, 51));
                killed = planes - live;
                if killed != 0 then
                    accessSector(r, c);
                    owner = rs->s_owner;
                    if owner != IS->is_playerNumber then
                        userN(killed);
                        user3(" plane(s) from ", &IS->is_player.c_name[0],
                              " crashed into your ");
                        user(getDesigName(rs->s_type));
                        userTarget(owner, " at ", r, c, "!");
                        notify(IS, owner);
                    }
                    owner = umin((USHORT) 100,
                        killed * (impRandom(IS, IS->is_world.w_planeRand) +
                        IS->is_world.w_planeBase));
                    if owner != NO_OWNER then
                        server(rt_lockSector, mapped);
                        damageSector(mapped, owner, IS->is_playerNumber);
                        server(rt_unlockSector, mapped);
                    }
                    userN(killed);
                    user(" planes crashed due to hurricane winds!\n");
                    planes = live;
                }
            }
            if planes != 0 then
                accessSector(r, c);
                saveSector = rs*;
                server(rt_readPlayer, saveSector.s_owner);
                if saveSector.s_owner != IS->is_playerNumber and
                    saveSector.s_owner != NO_OWNER and
                    IS->is_player.c_status != cs_deity and
                    (saveSector.s_type = s_fortress or
                     saveSector.s_type = s_airport or
                     saveSector.s_type = s_capital or
                     saveSector.s_type = s_bank) and
                    saveSector.s_quantity[it_missiles] != 0 and
                    saveSector.s_quantity[it_guns] != 0 and
                    saveSector.s_efficiency >= 60 and
                    saveSector.s_quantity[it_military] >= 5 and
                    IS->is_request.rq_u.ru_player.c_relations[
                        IS->is_playerNumber] = r_war
                then
                    if saveSector.s_checkPoint != 0 then
                        ptr** = '\0';
                        if not verifyCheckPoint(r, c, &saveSector) then
                            checkFlak(r, c, &planes, FALSE);
                        }
                    else
                        if gotEnemy then
                            checkFlak(r, c, &planes, FALSE);
                        }
                    }
                }
            }
        }
        if result = m_continue then
            pFuel* = fuel - cost;
            pRow* = r;
            pCol* = c;
        }
    }
    pPlanes* = planes;
    if planes = 0 then
        m_done
    else
        result
    fi
}

/*
 * doFly - common code for flying.
 */

proc doFly(int r, c; USHORT planes, bombs, fuel)void:
    register USHORT i;
    Move_t result;
    BOOL gotEnemy;

    gotEnemy = FALSE;
    i = 1;
    while (!gotEnemy && (i < IS->is_world.w_currPlayers))
    {
        server(rt_readPlayer, i);
        if IS->is_request.rq_u.ru_player.c_relations[IS->is_playerNumber] =
            r_war and IS->is_request.rq_u.ru_player.c_status = cs_active
        then
            gotEnemy = TRUE;
        }
        i = i - 1;
    }

    (void) skipBlanks();
    /* fuel is scaled by a factor of 10 */
    fuel = getTechFactor(IS->is_playerNumber) * fuel / 10;
    while
        while
            result =
                flyOnce(&IS->is_textInPos, &r, &c, &planes, &bombs, &fuel,
                        gotEnemy);
            result = m_continue
        do
        }
        result = m_readMore
    do
        userN3("<", fuel / 10, ".");
        userN(fuel % 10);
        userN3(":", planes, ":");
        userN(bombs);
        userS(":", r, c, "> ");
        uFlush();
        if not IS->is_readUser() or IS->is_textInPos* = '\0' then
            IS->is_textInPos = "e";    /* force an end */
        else
            (void) skipBlanks();
        }
    }
}

BOOL cmd_fly(IMP)
{
    [100] char buf1, buf2;
    register *Sector_t rs;
    register *Ship_t rsh @ rs;
    USHORT shipNumber, planes, shells, crew, bombs, fuel, quan, newPlanes;
    USHORT mapped;
    int r, c;
    BOOL isShip;

    if reqSectorOrShip(&r, &c, &shipNumber, &isShip,
                       "Sector/ship to fly from? ")
    then
        (void) skipBlanks();
        if isShip then
            /* we have a ship number - flying from a carrier */
            server(rt_readShip, shipNumber);
            rsh = &IS->is_request.rq_u.ru_ship;
            planes = rsh->sh_items[it_planes];
            crew = rsh->sh_items[it_military];
            if rsh->sh_owner != IS->is_playerNumber then
                err("you don't own that ship");
            elif rsh->sh_type != st_carrier and rsh->sh_type != st_destroyer
                   and rsh->sh_type != st_battleship then
                err("that ship can't launch planes");
            elif planes = 0 then
                err("no planes on board");
            elif crew < 2 then
                err("you need at least one pilot and gunner per plane");
            elif rsh->sh_efficiency < 60 then
                err("ship not efficient enough to take off from");
            else
                /* At this point, and later, we may not even own the ship.
                   I figure that's OK. If things are happening that close
                   together in time, then we may get what we can describe
                   as more interesting behavior. E.g. the crew didn't like
                   us selling the carrier out from under them, so they sort-
                   of rebelled by flying off a bunch of the planes. They
                   will not, of course, be allowed to land again! */
                accessShip(shipNumber);
                planes = rsh->sh_items[it_planes];
                crew = rsh->sh_items[it_military];
                quan = umin(planes, crew / 2);
                shells = rsh->sh_items[it_missiles];
                userN(planes);
                userN3(" planes on board, fly how many (max ", quan, ")? ");
                getPrompt(&buf1[0]);
                userN(shells);
                user(" bombs on board, carry how many per plane? ");
                getPrompt(&buf2[0]);
                if reqPosRange(&planes, quan, &buf1[0]) and
                    planes != 0 and doSkipBlanks() and
                    reqPosRange(&bombs, shells / planes, &buf2[0])
                then
                    /* if the ship is now a sunken wreck, we don't much care,
                       as these numbers are all likely to be 0 or very small */
                    IS->is_noWrite = TRUE;
                    server(rt_lockShip, shipNumber);
                    newPlanes = planes;
                    crew = rsh->sh_items[it_military];
                    if crew < newPlanes * 2 then
                        newPlanes = crew / 2;
                    }
                    quan = rsh->sh_items[it_planes];
                    if quan < newPlanes then
                        newPlanes = quan;
                    }
                    rsh->sh_items[it_military] =  crew - newPlanes * 2;
                    rsh->sh_items[it_planes] = quan - newPlanes;
                    if newPlanes != planes then
                        planes = newPlanes;
                        userN3("Sudden events have reduced your force to ",
                               planes, " plane(s)!!\n");
                    }
                    shells = rsh->sh_items[it_missiles];
                    if shells < bombs * planes then
                        bombs = shells / planes;
                        userN3("Sudden events have reduced your force to ",
                               bombs, " bomb(s) per plane!!\n");
                    }
                    rsh->sh_items[it_missiles] = shells - bombs * planes;
                    fuel = rsh->sh_fuelLeft / planes;
                    fuel = umin(fuel * IS->is_world.w_fuelRichness,
                                IS->is_world.w_fuelTankSize);
                    rsh->sh_fuelLeft -= fuel / IS->is_world.w_fuelRichness *
                        planes;
                    server(rt_unlockShip, shipNumber);
                    IS->is_noWrite = FALSE;
                    uFlush();
                    doFly(rowToMe(rsh->sh_row), colToMe(rsh->sh_col),
                          planes, bombs, fuel)
                }
            }
        else
            /* we have a sector specification - flying from an airport */
            mapped = mapSector(IS, r, c);
            server(rt_readSector, mapped);
            rs = &IS->is_request.rq_u.ru_sector;
            if rs->s_owner = IS->is_playerNumber then
                accessSector(r, c);
            }
            planes = readQuan(rs, it_planes);
            crew = readQuan(rs, it_military);
            if rs->s_owner != IS->is_playerNumber then
                err("you don't own that sector");
            elif rs->s_type != s_airport then
                err("can only take off from an airport");
            elif planes = 0 then
                err("there are no planes there");
            elif crew < 2 then
                err("you need at least 1 pilot and gunner per plane");
            elif rs->s_efficiency < 60 then
                err("airport not efficient enough to take off from");
            else
                quan = umin(planes, crew / 2);
                shells = readQuan(rs, it_missiles);
                userN(planes);
                userN3(" planes available, fly how many (max ", quan, ")? ");
                getPrompt(&buf1[0]);
                userN(shells);
                user(" bombs available, carry how many per plane? ");
                getPrompt(&buf2[0]);
                if reqPosRange(&planes, quan, &buf1[0]) and
                        planes != 0 and doSkipBlanks() and
                        reqPosRange(&bombs, shells / planes, &buf2[0])
                then
                    IS->is_noWrite = TRUE;
                    server(rt_lockSector, mapped);
                    newPlanes = planes;
                    quan = readQuan(rs, it_planes);
                    if newPlanes > quan then
                        newPlanes = quan;
                    }
                    crew = readQuan(rs, it_military);
                    if newPlanes > crew / 2 then
                        newPlanes = crew / 2;
                    }
                    if newPlanes != planes then
                        planes = newPlanes;
                        userN3("Sudden events have reduced your force to ",
                               planes, " plane(s)!!\n");
                    }
                    writeQuan(rs, it_planes, quan - planes);
                    writeQuan(rs, it_military, crew - planes * 2);
                    shells = readQuan(rs, it_missiles);
                    if shells < bombs * planes then
                        bombs = shells / planes;
                        userN3("Sudden events have reduced your force to ",
                               bombs, " bomb(s) per plane!!\n");
                    }
                    writeQuan(rs, it_missiles, shells - bombs * planes);
                    fuel = rs->s_mobility / planes;
                    fuel = umin(fuel * IS->is_world.w_fuelRichness,
                                 IS->is_world.w_fuelTankSize);
                    rs->s_mobility = rs->s_mobility - fuel /
                                IS->is_world.w_fuelRichness * planes;
                    server(rt_unlockSector, mapped);
                    IS->is_noWrite = FALSE;
                    uFlush();
                    doFly(r, c, planes, bombs, fuel);
                }
            }
        }
        TRUE
    else
        FALSE
    fi
}
#endif

/*
 * isStar - returns TRUE if the given row and col (assumed to be in the
 *          current sector buffer) is a star.
 */

BOOL isStar(IMP, register USHORT row, register USHORT col)
{
    register USHORT which;

    /* loop through the entries */
    for (which = 0; which < PLANET_MAX; which++)
    {
        /* look for one at the given row and col */
        if ((IS->is_sectBuf.sb_planet[which].sbp_row == row) &&
            (IS->is_sectBuf.sb_planet[which].sbp_col == col))
        {
            /* found one, is it a star? */
            if (IS->is_sectBuf.sb_planet[which].sbp_class == pc_S)
            {
                /* yes, return TRUE */
                return TRUE;
            }
            /* no, return FALSE */
            return FALSE;
        }
    }
    /* didn't find it, no items at this location */
    return FALSE;
}

/*
 * navOnce - inner part of cmd_navigate.
 */

Move_t navOnce(IMP, char dir, USHORT *pRow, USHORT *pCol,
    USHORT *pFlagEnerg, USHORT *pMinEnerg)
{
    register Sector_t *rs;
    register Ship_t *rsh;
    register Nav_t *nav;
    register USHORT i;
    USHORT r, c, oldR = 0, oldC = 0, flagEnerg, minEnerg;
    ULONG cost;
    Move_t result;
    ShipDamRes_t damRes;
    BOOL noCheck, continuing, inSpace;

    noCheck = FALSE;
    r = *pRow;
    c = *pCol;
    flagEnerg = *pFlagEnerg;
    rs = &IS->is_request.rq_u.ru_sector;
    rsh = &IS->is_request.rq_u.ru_ship;
    if (dir == '5')
    {
        /* snoop at the sector we're in - not much use here */
        result = m_continue;
        noCheck = TRUE;
        server(IS, rt_readSector, mapSector(IS, r, c));
        userS(IS, "Current sector: ", r, c, ": ");
        user2(IS, getDesigName(rs->s_type), "\n");
    }
    else if (dir == '.')
    {
        /* stop here */
        result = m_done;
    }
    else
    {
        server(IS, rt_readShip, IS->is_movingShips[0].n_ship);
        if (rsh->sh_planet != NO_ITEM)
        {
            err(IS, "Flagship is on the surface of a planet");
            result = m_readMore;
        }
        else
        {
            cost = getNavCost(IS, rsh);
            result = moveDir(IS, dir, &r, &c, &cost);
        }
    }
    if ((result == m_continue) && !noCheck)
    {
        if (((long)cost) > flagEnerg)
        {
            err(IS, "insufficient energy");
            result = m_readMore;
        }
        if (result == m_continue)
        {
            *pRow = r;
            *pCol = c;
            for (i = 0; i < IS->is_movingShipCount; i++)
            {
                nav = &IS->is_movingShips[i];
                if (nav->n_active)
                {
                    continuing = TRUE;
                    inSpace = TRUE;
                    server(IS, rt_readShip, nav->n_ship);
                    /* make sure the ship is not on a planet */
                    /* need to do this mainly for executing programs */
                    if (rsh->sh_planet != NO_ITEM)
                    {
                        continuing = FALSE;
                        inSpace = FALSE;
                    }
                    else
                    {
                        cost = getNavCost(IS, rsh);
                        r = rsh->sh_row;
                        oldR = r;
                        c = rsh->sh_col;
                        oldC = c;
                        (void) moveDir(IS, dir, &r, &c, &cost);
                        if (((long)cost) > nav->n_mobil)
                        {
                            continuing = FALSE;
                        }
                        else
                        {
                            /* read in the sector buffer, if needed */
                            if ((mapSector(IS, r, c) !=
                                IS->is_sectBuf.sb_sector) ||
                                (IS->is_world.w_planetNext !=
                                IS->is_sectBuf.sb_worldCount))
                            {
                                /* we have to read in the info again */
                                readSectBuf(IS, mapSector(IS, r, c));
                            }
                            /* read in the sector */
                            server(IS, rt_readSector, mapSector(IS, r, c));
                            /* is it one of the "special" types? */
                            if ((rs->s_type == s_blackhole) || (rs->s_type ==
                                s_supernova) || isStar(IS, r, c))
                            {
                                user(IS, "Bzzzzwwwaaapppp!\n");
                                server(IS, rt_lockShip, nav->n_ship);
                                IS->is_noWrite = TRUE;
                                user(IS, getShipName(rsh->sh_type));
                                userN3(IS, " #", nav->n_ship,
                                    " takes 100% damage from moving"
                                    " into a ");
                                switch(rs->s_type)
                                {
                                    case s_blackhole:
                                        user(IS, "black hole!\n");
                                        break;
                                    case s_supernova:
                                        user(IS, "super nova!\n");
                                        break;
                                    default:
                                        user(IS, "star!\n");
                                        break;
                                }
                                damRes = damageShip(IS, nav->n_ship, 100,
				    TRUE);
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
                                    userN3(IS, "Various other parts of the "
                                        "ship took ", damRes.sdr_main,
                                        " units of damage.\n");
                                }
                                server(IS, rt_unlockShip, nav->n_ship);
                                IS->is_noWrite = FALSE;
                                uFlush(IS);
                                nav->n_mobil = 0;
                                if (rsh->sh_owner == NO_OWNER)
                                {
                                    nav->n_active = FALSE;
                                    /* undo the decrement that 'damageShip' did */
                                    incrShipCount(IS, mapSector(IS, r, c));
                                }
                                else if (rsh->sh_efficiency < EFFIC_WARN)
                                {
                                    user(IS, getShipName(rsh->sh_type));
                                    userN3(IS, " #", nav->n_ship,
                                           " can no longer navigate.\n");
                                    nav->n_active = FALSE;
                                    incrShipCount(IS, mapSector(IS, r, c));
                                }
                            }
                        }
                    }
                    if (continuing)
                    {
                        server(IS, rt_lockShip, nav->n_ship);
                        rsh->sh_row = r;
                        rsh->sh_col = c;
                        nav->n_mobil -= cost;
                        server(IS, rt_unlockShip, nav->n_ship);
                        if (rsh->sh_owner == NO_OWNER)
                        {
                            nav->n_active = FALSE;
                        }
                        else
                        {
                            if (rsh->sh_efficiency < EFFIC_WARN)
                            {
                                user(IS, getShipName(rsh->sh_type));
                                userN3(IS, " #", nav->n_ship,
                                       " can no longer navigate.\n");
                                nav->n_active = FALSE;
                                incrShipCount(IS, mapSector(IS, oldR, oldC));
                            }
                        }
                    }
                    else
                    {
                        if (inSpace)
                        {
                            user(IS, getShipName(rsh->sh_type));
                            userN2(IS, " #", nav->n_ship);
                            userS(IS, " stops at ", oldR, oldC, "\n");
                            nav->n_active = FALSE;
                            incrShipCount(IS, mapSector(IS, oldR, oldC));
                        }
                        else
                        {
                            user(IS, getShipName(rsh->sh_type));
                            userN2(IS, " #", nav->n_ship);
                            user(IS, " is on the surface of a planet and can"
                                " not be navigated\n");
                            nav->n_active = FALSE;
                        }
                    }
                    if (!nav->n_active)
                    {
                        server(IS, rt_lockShip, nav->n_ship);
                        rsh->sh_energy = nav->n_mobil / 10;
                        server(IS, rt_unlockShip, nav->n_ship);
                    }
                }
            }
            minEnerg = IS->is_movingShips[0].n_mobil;
            for (i = 0; i < IS->is_movingShipCount; i++)
            {
                if (IS->is_movingShips[i].n_active &&
                    (IS->is_movingShips[i].n_mobil < minEnerg))
                {
                    minEnerg = IS->is_movingShips[i].n_mobil;
                }
            }
            *pFlagEnerg = IS->is_movingShips[0].n_mobil;
            *pMinEnerg = minEnerg;
            if (!IS->is_movingShips[0].n_active)
            {
                result = m_done;
            }
        }
    }
    return result;
}

/*
 * cmd_navigate - allows the player to move ships and fleets around
 */

BOOL cmd_navigate(IMP)
{
    register Ship_t *rsh;
    register Nav_t *nav;
    Fleet_t fl;
    register USHORT i;
    ULONG shipNumber;
    USHORT msc;
    USHORT flagEnerg, minEnerg;
    USHORT flagRow, flagCol;
    Move_t result;
    char fleet;
    BOOL gotOne, tooMany;
    char promptBuff[25];

    if (reqShipOrFleet(IS, &shipNumber, &fleet,
        "Ship or fleet to navigate"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        tooMany = FALSE;
        gotOne = FALSE;
        IS->is_movingShipCount = 0;
        msc = 0;
        (void) skipBlanks(IS);
        if (fleet == ' ')
        {
            server(IS, rt_readShip, shipNumber);
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else
            {
                accessShip(IS, shipNumber);
                if (rsh->sh_efficiency < EFFIC_WARN)
                {
                    if (rsh->sh_owner != NO_OWNER)
                    {
                        err(IS, "that ship isn't efficient enough to "
                            "navigate");
                    }
                }
                else if (rsh->sh_planet != NO_ITEM)
                {
                    err(IS, "that ship is still on the surface of a planet");
                }
                else
                {
                    IS->is_movingShipCount = 1;
                    msc = 1;
                    nav = &IS->is_movingShips[0];
                    nav->n_ship = shipNumber;
                    nav->n_mobil = ((short)rsh->sh_energy) * 10;
                    nav->n_active = TRUE;
                    flagEnerg = rsh->sh_energy;
                    minEnerg = rsh->sh_energy;
                    flagRow = rsh->sh_row;
                    flagCol = rsh->sh_col;
                    decrShipCount(IS, mapSector(IS, rsh->sh_row,
                        rsh->sh_col));
                }
            }
        }
        else
        {
            i = fleetPos(fleet);
            if (fleet == '*')
            {
                err(IS, "can't navigate fleet '*'");
            }
            else if (IS->is_player.p_fleets[i] == NO_FLEET)
            {
                err(IS, "you have no such fleet");
            }
            else
            {
                server(IS, rt_readFleet, IS->is_player.p_fleets[i]);
                fl = IS->is_request.rq_u.ru_fleet;
                if (fl.f_count == 0)
                {
                    err(IS, "fleet has no ships");
                }
                else
                {
                    gotOne = TRUE;
                    for (i = 0; i < fl.f_count; i++)
                    {
                        shipNumber = fl.f_ship[i];
                        accessShip(IS, shipNumber);
                        if (rsh->sh_efficiency < EFFIC_WARN)
                        {
                            if (rsh->sh_owner != NO_OWNER)
                            {
                                user(IS, getShipName(rsh->sh_type));
                                userN3(IS, " #", shipNumber,
                                    " isn't efficient enough to "
                                    "navigate.\n");
                            }
                        }
                        else if (rsh->sh_planet != NO_ITEM)
                        {
                            user(IS, getShipName(rsh->sh_type));
                            userN3(IS, " #", shipNumber,
                                " is on the surface of a planet\n");
                        }
                        else
                        {
                            gotOne = TRUE;
                            if (msc == 0)
                            {
                                flagEnerg = rsh->sh_energy;
                                minEnerg = rsh->sh_energy;
                                flagRow = rsh->sh_row;
                                flagCol = rsh->sh_col;
                            }
                            if (msc == MAX_NAV_SHIPS)
                            {
                                if (!tooMany)
                                {
                                    tooMany = TRUE;
                                    err(IS, "too many ships to navigate at "
                                        "once");
                                }
                            }
                            else
                            {
                                nav = &IS->is_movingShips[msc];
                                nav->n_ship = shipNumber;
                                nav->n_mobil = ((short)rsh->sh_energy) *
                                    10;
                                nav->n_active = TRUE;
                                msc++;
                                IS->is_movingShipCount = msc;
                                if (rsh->sh_energy < minEnerg)
                                {
                                    minEnerg = rsh->sh_energy;
                                }
                                decrShipCount(IS, mapSector(IS, rsh->sh_row,
                                    rsh->sh_col));
                            }
                        }
                    }
                }
            }
        }
        if (msc == 0)
        {
            if (gotOne)
            {
                err(IS, "no ships in that fleet can navigate");
            }
        }
        else if (flagEnerg == 0)
        {
            err(IS, "no energy");
        }
        else if (!tooMany)
        {
            /* scale energy by 10 */
            flagEnerg *= 10;
            minEnerg *= 10;
            while((result = navOnce(IS, *IS->is_textInPos, &flagRow, &flagCol,
                &flagEnerg, &minEnerg)) == m_continue)
            {
                IS->is_textInPos += sizeof(char);
            }
            while(result == m_readMore)
            {
                userN2(IS, "<", flagEnerg / 10);
                userN2(IS, ".", flagEnerg % 10);
                userN2(IS, ":", minEnerg / 10);
                userN2(IS, ".", minEnerg % 10);
                userS(IS, ": ", flagRow, flagCol, "> ");
                getPrompt(IS, &promptBuff[0]);
                uPrompt(IS, &promptBuff[0]);
                if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
                {
                    IS->is_textInPos[0] = '.';    /* force an end */
                    IS->is_textInPos[1] = '\0';
                }
                else
                {
                    (void) skipBlanks(IS);
                }
                while((result = navOnce(IS, *IS->is_textInPos, &flagRow,
                    &flagCol, &flagEnerg, &minEnerg)) == m_continue)
                {
                    IS->is_textInPos += sizeof(char);
                }
            }
            for (i = 0; i < IS->is_movingShipCount; i++)
            {
                nav = &IS->is_movingShips[i];
                if (nav->n_active)
                {
                    shipNumber = nav->n_ship;
                    server(IS, rt_lockShip, shipNumber);
                    rsh->sh_energy = nav->n_mobil / 10;
                    server(IS, rt_unlockShip, shipNumber);
                    incrShipCount(IS, mapSector(IS, rsh->sh_row,
                        rsh->sh_col));
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * doLiftoff - actually puts the ships into space
 */

BOOL doLiftoff(IMP)
{
    register Ship_t *rsh;
    register Nav_t *nav;
    register USHORT i;
    ULONG plNum;
    BOOL continuing;
    Ship_t sh;

    rsh = &IS->is_request.rq_u.ru_ship;
    server(IS, rt_readShip, IS->is_movingShips[0].n_ship);
    if (LIFTOFF_COST > rsh->sh_energy)
    {
        err(IS, "insufficient energy");
    }
    else
    {
        for (i = 0; i < IS->is_movingShipCount; i++)
        {
            nav = &IS->is_movingShips[i];
            if (nav->n_active)
            {
                continuing = TRUE;
                server(IS, rt_readShip, nav->n_ship);
                if ((LIFTOFF_COST * 10) > nav->n_mobil)
                {
                    continuing = FALSE;
                }
                if (continuing)
                {
                    server(IS, rt_lockShip, nav->n_ship);
                    plNum = rsh->sh_planet;
                    rsh->sh_planet = NO_ITEM;
                    rsh->sh_energy -= LIFTOFF_COST;
                    nav->n_mobil -= (LIFTOFF_COST * 10);
                    server(IS, rt_unlockShip, nav->n_ship);
                    sh = *rsh;
                    decrPlShipCount(IS, plNum);
                    if (sh.sh_owner == NO_OWNER)
                    {
                        nav->n_active = FALSE;
                    }
                    else
                    {
                        user(IS, getShipName(sh.sh_type));
                        userN3(IS, " #", nav->n_ship,
                            " is safely in orbit\n");
                        if (sh.sh_efficiency < EFFIC_WARN)
                        {
                            user(IS, getShipName(sh.sh_type));
                            userN3(IS, " #", nav->n_ship,
                                " is not able to navigate at the moment.\n");
                            nav->n_active = FALSE;
                        }
                    }
                }
                else
                {
                    user(IS, getShipName(rsh->sh_type));
                    userN2(IS, " #", nav->n_ship);
                    user(IS, " unable to lift off\n");
                    nav->n_active = FALSE;
                }
                if (!nav->n_active)
                {
                    server(IS, rt_lockShip, nav->n_ship);
                    rsh->sh_energy = nav->n_mobil / 10;
                    server(IS, rt_unlockShip, nav->n_ship);
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * cmd_liftoff - allows players to put ships into space
 */

BOOL cmd_liftoff(IMP)
{
    register Ship_t *rsh;
    register Nav_t *nav;
    Fleet_t fl;
    register USHORT i;
    ULONG shipNumber;
    USHORT msc;
    USHORT flagEnerg = 0;
    char fleet;
    BOOL gotOne, tooMany;

    if (reqShipOrFleet(IS, &shipNumber, &fleet,
        "Ship or fleet to launch"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        tooMany = FALSE;
        gotOne = FALSE;
        IS->is_movingShipCount = 0;
        msc = 0;
        (void) skipBlanks(IS);
        if (fleet == ' ')
        {
            server(IS, rt_readShip, shipNumber);
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else
            {
                accessShip(IS, shipNumber);
                if (rsh->sh_efficiency < EFFIC_WARN)
                {
                    if (rsh->sh_owner != NO_OWNER)
                    {
                        err(IS, "that ship isn't efficient enough to "
                            "lift off");
                    }
                }
                else if (rsh->sh_planet == NO_ITEM)
                {
                    err(IS, "that ship is already in space");
                }
                else
                {
                    IS->is_movingShipCount = 1;
                    msc = 1;
                    nav = &IS->is_movingShips[0];
                    nav->n_ship = shipNumber;
                    nav->n_mobil = ((short)rsh->sh_energy) * 10;
                    nav->n_active = TRUE;
                    flagEnerg = rsh->sh_energy;
                }
            }
        }
        else
        {
            i = fleetPos(fleet);
            if (fleet == '*')
            {
                err(IS, "can't launch fleet '*'");
            }
            else if (IS->is_player.p_fleets[i] == NO_FLEET)
            {
                err(IS, "you have no such fleet");
            }
            else
            {
                server(IS, rt_readFleet, IS->is_player.p_fleets[i]);
                fl = IS->is_request.rq_u.ru_fleet;
                if (fl.f_count == 0)
                {
                    err(IS, "fleet has no ships");
                }
                else
                {
                    gotOne = TRUE;
                    for (i = 0; i < fl.f_count; i++)
                    {
                        shipNumber = fl.f_ship[i];
                        accessShip(IS, shipNumber);
                        if (rsh->sh_efficiency < EFFIC_WARN)
                        {
                            if (rsh->sh_owner != NO_OWNER)
                            {
                                user(IS, getShipName(rsh->sh_type));
                                userN3(IS, " #", shipNumber,
                                    " isn't efficient enough to "
                                    "lift off.\n");
                            }
                        }
                        else if (rsh->sh_planet == NO_ITEM)
                        {
                            user(IS, getShipName(rsh->sh_type));
                            userN3(IS, " #", shipNumber,
                                " is already in space\n");
                        }
                        else
                        {
                            gotOne = TRUE;
                            if (msc == 0)
                            {
                                flagEnerg = rsh->sh_energy;
                            }
                            if (msc == MAX_NAV_SHIPS)
                            {
                                if (!tooMany)
                                {
                                    tooMany = TRUE;
                                    err(IS, "too many ships to launch at "
                                        "once");
                                }
                            }
                            else
                            {
                                nav = &IS->is_movingShips[msc];
                                nav->n_ship = shipNumber;
                                nav->n_mobil = ((short)rsh->sh_energy) *
                                    10;
                                nav->n_active = TRUE;
                                msc = msc + 1;
                                IS->is_movingShipCount = msc;
                            }
                        }
                    }
                }
            }
        }
        if (msc == 0)
        {
            if (gotOne)
            {
                err(IS, "no ships in that fleet can lift off");
            }
        }
        else
        {
            if (flagEnerg == 0)
            {
                err(IS, "no energy");
            }
            else
            {
                if (!tooMany)
                {
                    doLiftoff(IS);
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * doLand - actually puts the ships on a planet
 */

BOOL doLand(IMP)
{
    register Ship_t *rsh;
    register Nav_t *nav;
    register USHORT i;
    register Planet_t *rpl;
    ULONG plNum;
    BOOL continuing, saveWrite;
    Ship_t sh;
    ShipDamRes_t damRes;
    UBYTE watPct;
    USHORT shTf;

    rsh = &IS->is_request.rq_u.ru_ship;
    rpl = &IS->is_request.rq_u.ru_planet;
    server(IS, rt_readShip, IS->is_movingShips[0].n_ship);
    /* see if the flagship has enough energy */
    if (LAND_COST > rsh->sh_energy)
    {
        err(IS, "insufficient energy");
    }
    else
    {
        /* loop through all the ships in the list */
        for (i = 0; i < IS->is_movingShipCount; i++)
        {
            nav = &IS->is_movingShips[i];
            /* is this entry active? */
            if (nav->n_active)
            {
                continuing = TRUE;
                server(IS, rt_readShip, nav->n_ship);
                /* make sure this ship has enough energy to land */
                if ((LAND_COST * 10) > nav->n_mobil)
                {
                    continuing = FALSE;
                }
                if (continuing)
                {
                    sh = *rsh;
                    shTf = getShipTechFactor(IS, &sh, bp_engines);
                    shTf = (shTf + getShipTechFactor(IS, &sh, bp_computer))
                        / 2;
                    /* find the planet the ship is over, if any */
                    plNum = whichPlanet(IS, sh.sh_row, sh.sh_col);
                    if (plNum != NO_ITEM)
                    {
                        /* read in the planet */
                        server(IS, rt_readPlanet, plNum);
                        watPct = rpl->pl_water;
                        /* verify the player should be able to land here */
                        if ((rpl->pl_owner == IS->is_player.p_number) ||
                            ((rpl->pl_owner == NO_OWNER) && (isHomePlanet(IS,
                            plNum) == NO_RACE)) ||
                            (isHomePlanet(IS, plNum) == IS->is_player.p_race))
                        {
                            incrPlShipCount(IS, plNum);
                            server(IS, rt_lockShip, nav->n_ship);
                            rsh->sh_planet = plNum;
                            rsh->sh_energy -= LAND_COST;
                            nav->n_mobil -= (LAND_COST * 10);
                            /* is it possible to land here? */
                            if (watPct == 100)
                            {
                                damRes = damageShip(IS, nav->n_ship, 65535,
				    TRUE);
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
                            }
                            else if (watPct > 0)
                            {
                                if (impRandom(IS, 100) > (((100 - watPct) / 4) +
                                    70))
                                {
                                        if (impRandom(IS, 100) > ((shTf * 3) / 2))
                                        {
                                            saveWrite = IS->is_noWrite;
                                            IS->is_noWrite = TRUE;
                                            user(IS, getShipName(sh.sh_type));
                                            userN3(IS, " #", nav->n_ship,
                                                " takes damage due to a "
                                                "partial landing in water\n");
                                            damRes = damageShip(IS,
                                                nav->n_ship, 40, TRUE);
                                            if (damRes.sdr_shields != 0)
                                            {
                                                userN3(IS, "Your shields "
                                                    "absorbed ",
                                                    damRes.sdr_shields,
                                                    " units.\n");
                                            }
                                            if (damRes.sdr_armour != 0)
                                            {
                                                userN3(IS, "Your armour "
                                                    "absorbed ",
                                                    damRes.sdr_armour,
                                                    " units.\n");
                                            }
                                            if (damRes.sdr_main != 0)
                                            {
                                                userN3(IS, "Various other "
                                                    "parts of the ship took ",
                                                    damRes.sdr_main,
                                                    " units of damage.\n");
                                            }
                                            IS->is_noWrite = saveWrite;
                                        }
                                }
                            }
                            if (shTf < 30)
                            {
                                /* those gosh-darned accidents */
                                if (impRandom(IS, 100) < 2)
                                {
                                    saveWrite = IS->is_noWrite;
                                    IS->is_noWrite = TRUE;
                                    user(IS, getShipName(sh.sh_type));
                                    userN3(IS, " #", nav->n_ship,
                                        " takes damage due to an "
                                        "accident!\n");
                                    damRes = damageShip(IS, nav->n_ship, 25,
					TRUE);
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
                                        userN3(IS, "Various other parts of "
                                            "the ship took ", damRes.sdr_main,
                                            " units of damage.\n");
                                    }
                                    IS->is_noWrite = saveWrite;
                                }
                            }
                            sh = *rsh;
                            server(IS, rt_unlockShip, nav->n_ship);
                            uFlush(IS);
                            if (sh.sh_owner == NO_OWNER)
                            {
                                nav->n_active = FALSE;
                            }
                            else
                            {
                                user(IS, getShipName(sh.sh_type));
                                userN3(IS, " #", nav->n_ship,
                                    " is safely on the surface of ");
                                userN3(IS, "planet #", plNum, ".\n");
                                if (sh.sh_efficiency < EFFIC_WARN)
                                {
                                    user(IS, getShipName(sh.sh_type));
                                    userN3(IS, " #", nav->n_ship,
                                        " is not able to navigate at the "
                                        "moment.\n");
                                    nav->n_active = FALSE;
                                }
                            }
                        }
                        else
                        {
                            /* someone else owns it, so look for a */
                            /* checkpoint code */
                            if (rpl->pl_checkpoint[0] == '\0')
                            {
                                userP(IS, "Planet ", rpl, " is owned by "
                                    "someone else\n");
                                nav->n_active = FALSE;
                            }
                            else
                            {
                                /* there is a code, so see if the player */
                                /* knows it */
                                if (verifyCheckPoint(IS, rpl, cpt_land))
                                {
                                    server(IS, rt_lockShip, nav->n_ship);
                                    rsh->sh_planet = plNum;
                                    rsh->sh_energy -= LAND_COST;
                                    sh = *rsh;
                                    nav->n_mobil -= (LAND_COST * 10);
                                    server(IS, rt_unlockShip, nav->n_ship);
                                    incrPlShipCount(IS, plNum);
                                    if (sh.sh_owner == NO_OWNER)
                                    {
                                        nav->n_active = FALSE;
                                    }
                                    else
                                    {
                                        user(IS, getShipName(sh.sh_type));
                                        userN3(IS, " #", nav->n_ship,
                                            " is safely on the surface of ");
                                        userN3(IS, "planet #", plNum, ".\n");
                                        if (sh.sh_efficiency < EFFIC_WARN)
                                        {
                                            user(IS, getShipName(sh.sh_type)
                                                );
                                            userN3(IS, " #", nav->n_ship,
                                                " is not able to navigate at "
                                                "the moment.\n");
                                            nav->n_active = FALSE;
                                        }
                                    }
                                }
                                else
                                {
                                    user(IS, "incorrect access code\n");
                                    nav->n_active = FALSE;
                                }
                            }
                        }
                    }
                    else
                    {
                        /* ship is not over a planet */
                        userN3(IS, "Ship  #", nav->n_ship,
                            " is not orbiting a planet\n");
                        nav->n_active = FALSE;
                    }
                }
                else
                {
                    user(IS, getShipName(rsh->sh_type));
                    userN2(IS, " #", nav->n_ship);
                    user(IS, " unable to land\n");
                    nav->n_active = FALSE;
                }
                if (!nav->n_active)
                {
                    server(IS, rt_lockShip, nav->n_ship);
                    rsh->sh_energy = nav->n_mobil / 10;
                    server(IS, rt_unlockShip, nav->n_ship);
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * cmd_land - allows players to put land ships on a planet
 */

BOOL cmd_land(IMP)
{
    register Ship_t *rsh;
    register Nav_t *nav;
    Fleet_t fl;
    register USHORT i;
    ULONG shipNumber;
    USHORT msc;
    USHORT flagEnerg = 0;
    char fleet;
    BOOL gotOne, tooMany;

    if (reqShipOrFleet(IS, &shipNumber, &fleet,
        "Ship or fleet to land"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        tooMany = FALSE;
        gotOne = FALSE;
        IS->is_movingShipCount = 0;
        msc = 0;
        (void) skipBlanks(IS);
        if (fleet == ' ')
        {
            server(IS, rt_readShip, shipNumber);
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else
            {
                accessShip(IS, shipNumber);
                if (rsh->sh_efficiency < EFFIC_WARN)
                {
                    if (rsh->sh_owner != NO_OWNER)
                    {
                        err(IS, "that ship isn't efficient enough to "
                            "land");
                    }
                }
                else if (rsh->sh_planet != NO_ITEM)
                {
                    err(IS, "that ship is already on a planet");
                }
                else
                {
                    IS->is_movingShipCount = 1;
                    msc = 1;
                    nav = &IS->is_movingShips[0];
                    nav->n_ship = shipNumber;
                    nav->n_mobil = ((short)rsh->sh_energy) * 10;
                    nav->n_active = TRUE;
                    flagEnerg = rsh->sh_energy;
                }
            }
        }
        else
        {
            i = fleetPos(fleet);
            if (fleet == '*')
            {
                err(IS, "can't land fleet '*'");
            }
            else if (IS->is_player.p_fleets[i] == NO_FLEET)
            {
                err(IS, "you have no such fleet");
            }
            else
            {
                server(IS, rt_readFleet, IS->is_player.p_fleets[i]);
                fl = IS->is_request.rq_u.ru_fleet;
                if (fl.f_count == 0)
                {
                    err(IS, "fleet has no ships");
                }
                else
                {
                    gotOne = TRUE;
                    for (i = 0; i < fl.f_count; i++)
                    {
                        shipNumber = fl.f_ship[i];
                        accessShip(IS, shipNumber);
                        if (rsh->sh_efficiency < EFFIC_WARN)
                        {
                            if (rsh->sh_owner != NO_OWNER)
                            {
                                user(IS, getShipName(rsh->sh_type));
                                userN3(IS, " #", shipNumber,
                                    " isn't efficient enough to "
                                    "land.\n");
                            }
                        }
                        else if (rsh->sh_planet != NO_ITEM)
                        {
                            user(IS, getShipName(rsh->sh_type));
                            userN3(IS, " #", shipNumber,
                                " is already on a planet\n");
                        }
                        else
                        {
                            gotOne = TRUE;
                            if (msc == 0)
                            {
                                flagEnerg = rsh->sh_energy;
                            }
                            if (msc == MAX_NAV_SHIPS)
                            {
                                if (!tooMany)
                                {
                                    tooMany = TRUE;
                                    err(IS, "too many ships to land at "
                                        "once");
                                }
                            }
                            else
                            {
                                nav = &IS->is_movingShips[msc];
                                nav->n_ship = shipNumber;
                                nav->n_mobil = ((short)rsh->sh_energy) *
                                    10;
                                nav->n_active = TRUE;
                                msc = msc + 1;
                                IS->is_movingShipCount = msc;
                            }
                        }
                    }
                }
            }
        }
        if (msc == 0)
        {
            if (gotOne)
            {
                err(IS, "no ships in that fleet can land");
            }
        }
        else
        {
            if (flagEnerg == 0)
            {
                err(IS, "no energy");
            }
            else
            {
                if (!tooMany)
                {
                    doLand(IS);
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}

