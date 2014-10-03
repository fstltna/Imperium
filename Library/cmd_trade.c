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
 * $Id: cmd_trade.c,v 1.2 2000/05/18 06:50:02 marisa Exp $
 *
 * This file contains functions related to player - player item sales and
 * trades
 *
 * $Log: cmd_trade.c,v $
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:51  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:24:25  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:15  marisag
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

static const char rcsid[] = "$Id: cmd_trade.c,v 1.2 2000/05/18 06:50:02 marisa Exp $";

typedef struct
    {
        union
        {
            Ship_t
                un_ship;
            BigItem_t
                un_item;
            struct
            {
                Planet_t
                    unp_plan;
                short
                    unp_cost[IT_LAST + 1];
            } un_planet;
        } un;
    } Finan_t;

/*
 * getPartName - returns a pointer to the full string name for a big part
 *          type.
 */

char *getPartName(BigPart_t part)
{
    switch (part)
    {
        case bp_computer:
            return "computer";
        case bp_engines:
            return "engine";
        case bp_lifeSupp:
            return "life support unit";
        case bp_sensors:
            return "sensor array";
        case bp_teleport:
            return "teleport unit";
        case bp_photon:
            return "photon torpedo launcher";
        case bp_blaser:
            return "blaser";
        case bp_shield:
            return "shield";
        case bp_tractor:
            return "tractor beam projector";
        default:
            return "?bad part?";
    }
}

/*
 * cmd_price - allows the player to create/remove "lots"
 */

BOOL cmd_price(IMP)
{
    register Planet_t *rpl;
    register Ship_t *rsh;
    register Offer_t *rof;
    register World_t *rw;
    register BigItem_t *rbi;
    USHORT offerNumber, cost, oldPrice;
    Finan_t saveIt;
    ULONG itemNumber;
    USHORT what, isSell, shipOwn;
    ItemType_t which;
    ULONG price;
    BOOL gotOffer, cont, noOff, doAbort;

    gotOffer = FALSE;
    /* find out what they want to sell */
    if (reqChoice(IS, &what, "ship\0item\0planet\0",
        "Set price on what (ship/item/planet)"))
    {
        /* set up pointers */
        rpl = &IS->is_request.rq_u.ru_planet;
        rsh = &IS->is_request.rq_u.ru_ship;
        rof = &IS->is_request.rq_u.ru_offer;
        rw = &IS->is_request.rq_u.ru_world;
        rbi = &IS->is_request.rq_u.ru_bigItem;
        switch(what)
        {
            /* ship for sale */
            case 0:
                (void) skipBlanks(IS);
                if (reqShip(IS, &itemNumber, "Ship to sell"))
                {
                    /* make sure the ship is valid */
                    server(IS, rt_readShip, itemNumber);
                    if (rsh->sh_owner != IS->is_player.p_number)
                    {
                       err(IS, "you don't own that ship");
                       return FALSE;
                    }
                    else if (rsh->sh_fleet != '*')
                    {
                       err(IS, "ship must be removed from fleet first");
                       return FALSE;
                    }
                    else
                    {
                        saveIt.un.un_ship = *rsh;
                        offerNumber = 0;
                        do
                        {
                            if (offerNumber == IS->is_world.w_offerNext)
                            {
                                cont = FALSE;
                            }
                            else
                            {
                                server(IS, rt_readOffer, offerNumber);
                                cont = ((rof->of_state != of_ship) ||
                                    (rof->of_.of_shipNumber != itemNumber));
                            }
                            if (cont)
                            {
                                offerNumber++;
                            }
                        } while (cont);
                        if (offerNumber != IS->is_world.w_offerNext)
                        {
                            if ((rof->of_who != IS->is_player.p_number) &&
                                (saveIt.un.un_ship.sh_price != 0))
                            {
                                server(IS, rt_lockShip, itemNumber);
                                rsh->sh_price = 0;
                                server(IS, rt_unlockShip, itemNumber);
                                saveIt.un.un_ship.sh_price = 0;
                            }
                            gotOffer = TRUE;
                        }
                        cost = IS->is_world.w_shipCost[
                            saveIt.un.un_ship.sh_type];
                        user(IS, getShipName(saveIt.un.un_ship.sh_type));
                        userN2(IS, " #", itemNumber);
                        if (rsh->sh_price == 0)
                        {
                            user(IS, " is currently not for sale\n");
                        }
                        else
                        {
                            userN2(IS, " is currently offered at $",
                                saveIt.un.un_ship.sh_price);
                            userN3(IS, " per ton (total $",
                                saveIt.un.un_ship.sh_price * cost, ")\n");
                        }
                        (void) skipBlanks(IS);
                        if (reqPosRange(IS, &price, 256, "New price per ton? "))
                        {
                            if (price == 0)
                            {
                                if (saveIt.un.un_ship.sh_price != 0)
                                {
                                    server(IS, rt_lockShip, itemNumber);
                                    rsh->sh_price = 0;
                                    server(IS, rt_unlockShip, itemNumber);
                                    server(IS, rt_lockOffer, offerNumber);
                                    rof->of_state = of_none;
                                    server(IS, rt_unlockOffer, offerNumber);
                                    userN3(IS, "Lot #", offerNumber, " withdrawn\n");
                                }
                                else
                                {
                                    user(IS, "OK, ship not offered for sale\n");
                                }
                            }
                            else
                            {
                                userN3(IS, "Total value is $", price * cost, "\n");
                                server(IS, rt_lockShip, itemNumber);
                                rsh->sh_price = price;
                                server(IS, rt_unlockShip, itemNumber);
                                if (gotOffer)
                                {
                                    server(IS, rt_lockOffer, offerNumber);
                                }
                                else
                                {
                                    server(IS, rt_lockWorld, 0);
                                    offerNumber = rw->w_offerNext;
                                    rw->w_offerNext = offerNumber + 1;
                                    server(IS, rt_unlockWorld, 0);
                                    IS->is_world.w_offerNext = rw->w_offerNext;
                                }
                                rof->of_state = of_ship;
                                rof->of_who = IS->is_player.p_number;
                                rof->of_.of_shipNumber = itemNumber;
                                if (gotOffer)
                                {
                                    server(IS, rt_unlockOffer, offerNumber);
                                }
                                else
                                {
                                   server(IS, rt_createOffer, offerNumber);
                                }
                                user(IS, getShipName(
                                    saveIt.un.un_ship.sh_type));
                                userN2(IS, " #", itemNumber);
                                userN3(IS, " is now lot #", offerNumber,
                                    "\n");
                           }
                        }
                        return TRUE;
                    }
                }
                break;

            /* big item for sale */
            case 1:
                (void) skipBlanks(IS);
                if (reqBigItem(IS, &itemNumber, "Item to sell"))
                {
                    server(IS, rt_readBigItem, itemNumber);
                    if (rbi->bi_onShip)
                    {
                       err(IS, "item is not on a planet");
                       return FALSE;
                    }
                    saveIt.un.un_item = *rbi;
                    server(IS, rt_readPlanet, saveIt.un.un_item.bi_itemLoc);
                    if (rpl->pl_owner != IS->is_player.p_number)
                    {
                       err(IS, "you don't own the planet that item is on");
                       return FALSE;
                    }
                    /* we want to loop until we find the item */
                    offerNumber = 0;
                    do
                    {
                        if (offerNumber == IS->is_world.w_offerNext)
                        {
                            cont = FALSE;
                        }
                        else
                        {
                            server(IS, rt_readOffer, offerNumber);
                            cont = ((rof->of_state != of_item) ||
                                (rof->of_.of_itemNumber != itemNumber));
                        }
                        if (cont)
                        {
                            offerNumber++;
                        }
                    } while (cont);
                    /* did we find it? */
                    if (offerNumber != IS->is_world.w_offerNext)
                    {
                        /* yes we did */
                        if ((rof->of_who != IS->is_player.p_number) &&
                            (saveIt.un.un_item.bi_status != bi_inuse))
                        {
                            server(IS, rt_lockBigItem, itemNumber);
                            rbi->bi_price = 0;
                            server(IS, rt_unlockBigItem, itemNumber);
                            saveIt.un.un_item = *rbi;
                        }
                        gotOffer = TRUE;
                    }
                    userN2(IS, " Big item #", itemNumber);
                    if (rbi->bi_price == 0)
                    {
                        user(IS, " is currently not for sale\n");
                    }
                    else
                    {
                        userN3(IS, " is currently offered at $",
                            saveIt.un.un_item.bi_price, "\n");
                    }
                    (void) skipBlanks(IS);
                    if (reqPosRange(IS, &price, MAX_WORK, "New price"))
                    {
                        if (price == 0)
                        {
                            if (saveIt.un.un_item.bi_price != 0)
                            {
                                server(IS, rt_lockBigItem, itemNumber);
                                rbi->bi_price = 0;
                                server(IS, rt_unlockBigItem, itemNumber);
                                server(IS, rt_lockOffer, offerNumber);
                                rof->of_state = of_none;
                                server(IS, rt_unlockOffer, offerNumber);
                                userN3(IS, "Lot #", offerNumber,
                                    " withdrawn\n");
                            }
                            else
                            {
                                user(IS, "OK, item not offered for "
                                    "sale\n");
                            }
                        }
                        else
                        {
                            userN3(IS, "Item offered for sale at $", price,
                                "\n");
                            server(IS, rt_lockBigItem, itemNumber);
                            rbi->bi_price = price;
                            server(IS, rt_unlockBigItem, itemNumber);
                            if (gotOffer)
                            {
                                server(IS, rt_lockOffer, offerNumber);
                            }
                            else
                            {
                                server(IS, rt_lockWorld, 0);
                                offerNumber = rw->w_offerNext;
                                rw->w_offerNext++;
                                server(IS, rt_unlockWorld, 0);
                                IS->is_world.w_offerNext = rw->w_offerNext;
                            }
                            rof->of_state = of_item;
                            rof->of_who = IS->is_player.p_number;
                            rof->of_.of_itemNumber = itemNumber;
                            if (gotOffer)
                            {
                                server(IS, rt_unlockOffer, offerNumber);
                            }
                            else
                            {
                               server(IS, rt_createOffer, offerNumber);
                            }
                            userN2(IS, "Item #", itemNumber);
                            userN3(IS, " is now lot #", offerNumber, "\n");
                       }
                    }
                    return TRUE;
                }
                break;

            case 2:
                doAbort = FALSE;
                (void) skipBlanks(IS);
                /* setting selling/buying prices for items on a planet */
                if (!reqPlanet(IS, &itemNumber, "Planet to set prices on"))
                {
                    doAbort = TRUE;
                }
                /* this is pretty ugly, but saves excessive indentation */
                if (!doAbort)
                {
                    skipBlanks(IS);
                    if (!reqChoice(IS, &shipOwn, "planet\0ship\0",
                        "Who pays teleport damages (planet/ship) owner"))
                    {
                        doAbort = TRUE;
                    }
                }
                if (!doAbort)
                {
                    skipBlanks(IS);
                    if (!reqChoice(IS, &isSell, "buying\0selling\0",
                        "Set which price (buying/selling)"))
                    {
                        doAbort = TRUE;
                    }
                }
                if (!doAbort)
                {
                    skipBlanks(IS);
                    if (!reqCmsgpob(IS, &which, "Set price on what? "))
                    {
                        doAbort = TRUE;
                    }
                }
                if (!doAbort)
                {
                    server(IS, rt_readPlanet, itemNumber);
                    if ((which == it_civilians) || (which == it_military) ||
                        (which == it_scientists) || (which == it_officers))
                    {
                        err(IS, "can't buy or sell people");
                    }
                    else if (rpl->pl_owner != IS->is_player.p_number)
                    {
                        err(IS, "you don't own that planet");
                    }
                    else if (rpl->pl_efficiency < EFFIC_WARN)
                    {
                        err(IS, "planet is not efficient enough for trading");
                    }
                    else
                    {
                        offerNumber = 0;
                        noOff = TRUE;
                        /* look for an old offer number */
                        while (noOff && (offerNumber <
                            IS->is_world.w_offerNext))
                        {
                            server(IS, rt_readOffer, offerNumber);
                            if ((rof->of_.of_plan.of_planetNumber ==
                                itemNumber) && (rof->of_state == of_planet))
                            {
                                noOff = FALSE;
                            }
                            else
                            {
                                offerNumber++;
                            }
                        }
                        cost = getItemCost(IS, which);
                        if (noOff)
                        {
                            oldPrice = 0;
                        }
                        else
                        {
                            if (rof->of_who != IS->is_player.p_number)
                            {
                                oldPrice = 0;
                            }
                            else
                            {
                                oldPrice =
                                    rof->of_.of_plan.prices[isSell][which] *
                                    cost;
                            }
                            gotOffer = TRUE;
                        }
                        user(IS, getItemName(which));
                        if (oldPrice == 0)
                        {
                            if (isSell)
                            {
                                user(IS, " currently not for sale");
                            }
                            else
                            {
                                user(IS, " currently not being "
                                    "bought");
                            }
                        }
                        else
                        {
                            if (isSell)
                            {
                                userM3(IS, " currently offered at $",
                                    oldPrice, "");
                            }
                            else
                            {
                                userM3(IS, " currently being bought "
                                    "at $", oldPrice, "");
                            }
                        }
                        server(IS, rt_readPlanet, itemNumber);
                        userP(IS, " on ", rpl, "\n");
                        (void) skipBlanks(IS);
                        if (reqPosRange1(IS, &price, (cost * 10) *
                            127, "New price? "))
                        {
                            if (((price / cost) == 0) && (price != 0))
                            {
                                userM3(IS, "Price too low, must be "
                                    "at least $", cost, "\n");
                            }
                            else
                            {
                                price /= cost;
                                if (price == 0)
                                {
                                    if (oldPrice == 0)
                                    {
                                        if (isSell)
                                        {
                                            user3(IS, "OK, ",
                                                getItemName(which),
                                                " is still not "
                                                "offered for sale\n");
                                        }
                                        else
                                        {
                                            user3(IS, "OK, ",
                                                getItemName(which),
                                                " is still not being "
                                                "bought\n");
                                        }
                                    }
                                    else
                                    {
                                        if (isSell)
                                        {
                                            user3(IS, "OK, ",
                                                getItemName(which),
                                                " is no longer "
                                                "offered for sale\n");
                                        }
                                        else
                                        {
                                            user3(IS, "OK, ",
                                                getItemName(which),
                                                " are no longer "
                                                "being bought\n");
                                        }
                                        server(IS, rt_lockOffer,
                                            offerNumber);
                                        rof->of_.of_plan.prices[isSell][which] = 0;

                                        if ((rof->of_.of_plan.prices[of_sellPrice][it_missiles] == 0)
                                            && (rof->of_.of_plan.prices[of_sellPrice][it_planes] == 0)
                                            && (rof->of_.of_plan.prices[of_sellPrice][it_ore] == 0)
                                            && (rof->of_.of_plan.prices[of_sellPrice][it_bars] == 0)
                                            && (rof->of_.of_plan.prices[of_sellPrice][it_airTanks] == 0)
                                            && (rof->of_.of_plan.prices[of_sellPrice][it_fuelTanks] == 0)
                                            && (rof->of_.of_plan.prices[of_buyPrice][it_missiles] == 0)
                                            && (rof->of_.of_plan.prices[of_buyPrice][it_planes] == 0)
                                            && (rof->of_.of_plan.prices[of_buyPrice][it_ore] == 0)
                                            && (rof->of_.of_plan.prices[of_buyPrice][it_bars] == 0)
                                            && (rof->of_.of_plan.prices[of_buyPrice][it_airTanks] == 0)
                                            && (rof->of_.of_plan.prices[of_buyPrice][it_fuelTanks] == 0))
                                        {
                                            rof->of_state = of_none;
                                            server(IS, rt_unlockOffer,
                                                offerNumber);
                                            userN3(IS, "Lot #",
                                                offerNumber,
                                                " withdrawn\n");
                                        }
                                        else
                                        {
                                            server(IS, rt_unlockOffer,
                                                offerNumber);
                                            userN3(IS, "Lot #",
                                                offerNumber,
                                                " updated\n");
                                        }
                                    }
                                }
                                else
                                {
                                    userM3(IS, "Price set to $",
                                        price * cost, "\n");
                                    if (gotOffer)
                                    {
                                        userN3(IS, "Lot #",
                                            offerNumber,
                                            " updated\n");
                                        server(IS, rt_lockOffer,
                                            offerNumber);
                                    }
                                    else
                                    {
                                        server(IS, rt_lockWorld, 0);
                                        offerNumber = rw->w_offerNext;
                                        rw->w_offerNext++;
                                        server(IS, rt_unlockWorld, 0);
                                        IS->is_world.w_offerNext =
                                            rw->w_offerNext;
                                        userN3(IS, "This item is lot"
                                            " #", offerNumber, "\n");
                                        for (what = 0; what <= IT_LAST_SMALL;
                                            what++)
                                        {
                                            rof->of_.of_plan.prices[of_buyPrice][what] = 0;
                                            rof->of_.of_plan.prices[of_sellPrice][what] = 0;
                                        }
                                    }
                                    rof->of_.of_plan.of_payor = shipOwn;
                                    rof->of_state = of_planet;
                                    rof->of_who = IS->is_player.p_number;
                                    rof->of_.of_plan.of_planetNumber =
                                        itemNumber;
                                    rof->of_.of_plan.prices[isSell][which] =
                                        price;
                                    if (gotOffer)
                                    {
                                        server(IS, rt_unlockOffer,
                                            offerNumber);
                                    }
                                    else
                                    {
                                        server(IS, rt_createOffer,
                                            offerNumber);
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (oldPrice == 0)
                            {
                                if (isSell)
                                {
                                    user3(IS, "OK, ", getItemName(which),
                                        " are no longer offered for sale\n");
                                }
                                else
                                {
                                    user3(IS, "OK, ", getItemName(which),
                                        " are no longer being bought\n");
                                }
                            }
                            else
                            {
                                user(IS, "OK, price not changed\n");
                            }
                        }
                        return TRUE;
                    }
                }
                break;

            default:
                err(IS, "unknown or unimplemented sale type");
                break;
        }
    }
    return FALSE;
}

/*
 * printOffer - print out an offer. Return 'FALSE' if it is gone.
 *          Convention - the offer is in the request buffer
 */

BOOL printOffer(IMP, USHORT offerNumber, Player_t *player)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    register BigItem_t *rbi;
    register USHORT cost;
    ULONG plNum;
    Offer_t *rof;
    Offer_t saveOf;
    Finan_t saveIt;
    USHORT who;
    ULONG theNumber;
    register ItemType_t it;
    BOOL needMess;

    /* set up all pointers */
    rof = &IS->is_request.rq_u.ru_offer;
    rsh = &IS->is_request.rq_u.ru_ship;
    rpl = &IS->is_request.rq_u.ru_planet;
    rbi = &IS->is_request.rq_u.ru_bigItem;
    who = rof->of_who;
    needMess = FALSE;

    /* are they selling a ship */
    if (rof->of_state == of_ship)
    {
        theNumber = rof->of_.of_shipNumber;
        server(IS, rt_readShip, theNumber);
        if (rsh->sh_owner != who)
        {
            server(IS, rt_lockShip, theNumber);
            rsh->sh_price = 0;
            server(IS, rt_unlockShip, theNumber);
            server(IS, rt_lockOffer, offerNumber);
            rof->of_state = of_none;
            server(IS, rt_unlockOffer, offerNumber);
            needMess = TRUE;
#ifdef DEBUG_LIB
            user(IS, "Owner of ship is not the player offering it for "
                "sale\n");
#endif
        }
        else
        {
            saveIt.un.un_ship = *rsh;
            userN2(IS, "Lot #", offerNumber);
            user2(IS, " for sale by ", &player->p_name[0]);
            user2(IS, ":\n ", getShipName(rsh->sh_type));
            userN2(IS, " #", theNumber);
            userS(IS, " at ", rsh->sh_row, rsh->sh_col, ": ");
            userN(IS, rsh->sh_efficiency);
            userN2(IS, "%, Eng Tech Fact: ", getShipTechFactor(IS,
                &saveIt.un.un_ship, bp_engines));
            userNL(IS);
            userN2(IS, "  crew ", saveIt.un.un_ship.sh_items[it_civilians] +
                saveIt.un.un_ship.sh_items[it_military] +
                saveIt.un.un_ship.sh_items[it_scientists] +
                saveIt.un.un_ship.sh_items[it_officers]);
            userN2(IS, "  weapons: ", saveIt.un.un_ship.sh_items[it_weapons]);
            userN2(IS, "  shells: ", saveIt.un.un_ship.sh_items[it_missiles]);
            userN2(IS, "  @ $", saveIt.un.un_ship.sh_price);
            userN3(IS, " (tot $", (saveIt.un.un_ship.sh_price *
               IS->is_world.w_shipCost[saveIt.un.un_ship.sh_type]), ")\n");
        }
    }
    /* how about a big item */
    else if (rof->of_state == of_item)
    {
        theNumber = rof->of_.of_shipNumber;
        server(IS, rt_readShip, theNumber);
        saveIt.un.un_item = *rbi;
        if ((rbi->bi_itemLoc == NO_ITEM) || rbi->bi_onShip ||
            (rbi->bi_price == 0) || (rbi->bi_status != bi_inuse))
        {
            server(IS, rt_lockBigItem, theNumber);
            rbi->bi_price = 0;
            server(IS, rt_unlockBigItem, theNumber);
            server(IS, rt_lockOffer, offerNumber);
            rof->of_state = of_none;
            server(IS, rt_unlockOffer, offerNumber);
            needMess = TRUE;
#ifdef DEBUG_LIB
            user(IS, "Initial error with big item in lot\n");
#endif
        }
        else
        {
            server(IS, rt_readPlanet, saveIt.un.un_item.bi_itemLoc);
            if (rpl->pl_owner != who)
            {
                server(IS, rt_lockBigItem, theNumber);
                rbi->bi_price = 0;
                server(IS, rt_unlockBigItem, theNumber);
                server(IS, rt_lockOffer, offerNumber);
                rof->of_state = of_none;
                server(IS, rt_unlockOffer, offerNumber);
                needMess = TRUE;
#ifdef DEBUG_LIB
                user(IS, "Owner of the planet that the item is on is "
                    "not the person offering the lot\n");
#endif
            }
            else
            {
                userN2(IS, "Lot #", offerNumber);
                user2(IS, " for sale by ", &player->p_name[0]);
                user2(IS, ":\n ", getPartName(saveIt.un.un_item.bi_part));
                userN2(IS, " #", theNumber);
                userP(IS, " on ", rpl, ": ");
                userN(IS, saveIt.un.un_item.bi_effic);
                userN2(IS, "%, Tech Fact: ", getTechFactor(
                    saveIt.un.un_item.bi_techLevel));
                userN3(IS, ", Weight: ", saveIt.un.un_item.bi_weight, "\n");
                userN3(IS, "  $", saveIt.un.un_item.bi_price, "\n");
            }
        }
    }
    /* Ok, how about items on a planet */
    else if (rof->of_state == of_planet)
    {
        plNum = rof->of_.of_plan.of_planetNumber;
        saveOf = *rof;
        server(IS, rt_readPlanet, plNum);
        if ((rpl->pl_owner != who) || (rpl->pl_efficiency < EFFIC_WARN))
        {
            server(IS, rt_lockOffer, offerNumber);
            rof->of_state = of_none;
            server(IS, rt_unlockOffer, offerNumber);
            needMess = TRUE;
        }
        else
        {
            userN2(IS, "Lot #", offerNumber);
            user2(IS, " offered by ", &player->p_name[0]);
            userP(IS, " on planet ", rpl, "");
            userS(IS, " at ", rpl->pl_row, rpl->pl_col, "\n");
            user(IS, "For Sale:\n");
            /* loop for all the valid items */
            for (it = it_missiles; it <= IT_LAST_SMALL; it++)
            {
                if (saveOf.of_.of_plan.prices[of_sellPrice][it] != 0)
                {
                    cost = getItemCost(IS, it) *
                        saveOf.of_.of_plan.prices[of_sellPrice][it];
                    userN(IS, readPlQuan(IS, rpl, it));
                    user2(IS, " ", getItemName(it));
                    userM3(IS, " @ $", cost, "  ");
                }
            }
            user(IS, "\nBuying:\n");
            for (it = it_missiles; it <= IT_LAST_SMALL; it++)
            {
                if (saveOf.of_.of_plan.prices[of_buyPrice][it] != 0)
                {
                    cost = getItemCost(IS, it) *
                        saveOf.of_.of_plan.prices[of_buyPrice][it];
                    user(IS, getItemName(it));
                    userM3(IS, " @ $", cost, "  ");
                }
            }
            userNL(IS);
            if (saveOf.of_.of_plan.of_payor == of_shipOwn)
            {
                user(IS, "--- Ship owner pays all damages\n");
            }
            else
            {
                user(IS, "--- Planet owner pays all damages\n");
            }
        }
    }
    if (needMess)
    {
       userN3(IS, "Your lot #", offerNumber, " cancelled");
       notify(IS, rof->of_who);
    }
    return (rof->of_state != of_none);
}

/*
 * cmd_report - allows the player to see offers available from other players
 *          who are at least neutral to him
 */

BOOL cmd_report(IMP)
{
    register Offer_t *rof;
    USHORT choice, bywho;
    ULONG offerNumber;
    BOOL doNaval, doPlanet, doItem;
    Player_t player;

    /* check to see if they have numbers as the next item in the input */
    /* buffer */
    if ((*IS->is_textInPos >= '0') && (*IS->is_textInPos <= '9') &&
        (IS->is_world.w_offerNext != 0))
    {
        /* they did, so try and parse them as a lot number */
        if (getPosRange(IS, &offerNumber, IS->is_world.w_offerNext - 1))
        {
            server(IS, rt_readOffer, offerNumber);
            bywho = IS->is_request.rq_u.ru_offer.of_who;
            server(IS, rt_readPlayer, bywho);
            if ((getYourRelation(IS, &IS->is_request.rq_u.ru_player)
                != r_war) || (IS->is_player.p_number == bywho))
            {
                player = IS->is_request.rq_u.ru_player;
                server(IS, rt_readOffer, offerNumber);
                (void) printOffer(IS, offerNumber, &player);
            }
            return TRUE;
        }
        return FALSE;
    }

    /* set up some default items to print */
    doNaval = TRUE;
    doPlanet = TRUE;
    doItem = TRUE;

    /* now just check for any other arguments they may have given us */
    if (*IS->is_textInPos != '\0')
    {
        /* since they gave us SOMETHING, clear out the previous defaults */
        /* assuming what they gave us was a valid report type */
        doNaval = FALSE;
        doPlanet = FALSE;
        doItem = FALSE;
        /* find out which report type they want */
        if (getChoice(IS, &choice, "ships\0planets\0items\0"))
        {
            switch (choice)
            {
                case 0:
                    doNaval = TRUE;
                    break;
                case 1:
                    doPlanet = TRUE;
                    break;
                case 2:
                    doItem = TRUE;
                    break;
            }
        }
    }

    /* make sure they want SOME kind of report */
    if (doPlanet || doNaval || doItem)
    {
        user(IS, "        Imperium Trade Report as of ");
        uTime(IS, IS->is_request.rq_time);
        userNL(IS);
        userNL(IS);
        if (IS->is_world.w_offerNext != 0)
        {
            rof = &IS->is_request.rq_u.ru_offer;
            for (offerNumber = 0; offerNumber < IS->is_world.w_offerNext;
                offerNumber++)
            {
                server(IS, rt_readOffer, offerNumber);
                bywho = IS->is_request.rq_u.ru_offer.of_who;
                server(IS, rt_readPlayer, bywho);
                if ((IS->is_player.p_number == bywho) || (getYourRelation(IS,
                    &IS->is_request.rq_u.ru_player) != r_war))
                {
                    player = IS->is_request.rq_u.ru_player;
                    server(IS, rt_readOffer, offerNumber);
                    if ((((rof->of_state == of_ship) && doNaval) ||
                        ((rof->of_state == of_planet) && doPlanet) ||
                        ((rof->of_state == of_item) && doItem)) &&
                        printOffer(IS, offerNumber, &player))
                    {
                        userNL(IS);
                    }
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * doBuy - try to buy a given item from a planet.
 *          Convention: the planet and ship pair is in the request on both
 *          entry and exit
 */

void doBuy(IMP, ULONG plNum, ULONG shNum, register ItemType_t what,
    ULONG *pFunds, USHORT oPrice, UBYTE telepTF, UBYTE shOwnPay)
{
    register PlanetShipPair_t *rps;
    ULONG price, cost, count, countOrig, telepAmount;
    USHORT max, q1, q2, capacity, present, loaded;
    char buff[100];

    rps = &IS->is_request.rq_u.ru_planetShipPair;
    price = getItemCost(IS, what) * oPrice;
    if (price != 0)
    {
        present = readPlQuan(IS, &rps->p_p, what);
        capacity = getShipCapacity(IS, what, &rps->p_sh);
        loaded = rps->p_sh.sh_items[what];
        max = umin(present, capacity - loaded);
        user2(IS, "Buy how many ", getItemName(what));
        userM3(IS, " at $", price, " (max ");
        userN(IS, max);
        user(IS, ")");
        getPrompt(IS, &buff[0]);
        if (reqPosRange(IS, &count, max, &buff[0]))
        {
            cost = price * count / 10;
            if (*pFunds > cost)
            {
                countOrig = count;
                server2(IS, rt_lockPlanetShipPair, plNum, shNum);
                q1 = readPlQuan(IS, &rps->p_p, what);
                if (q1 < count)
                {
                    count = q1;
                }
                q2 = rps->p_sh.sh_items[what];
                if (count > capacity - q2)
                {
                    count = capacity - q2;
                }
                telepAmount = count;
                /* see if they were using teleports */
                if (telepTF != 0)
                {
                    /* check for possible teleport problems */
                    if (impRandom(IS, 100) < ((3 * telepTF) / 2))
                    {
                        /* everything went OK */
                        telepAmount = count;
                    }
                    else
                    {
                        telepAmount = ((count * (75 + impRandom(IS, 24))) / 100);
                    }
                }
                writePlQuan(IS, &rps->p_p, what, q1 - count);
                rps->p_sh.sh_items[what] += telepAmount;
                rps->p_sh.sh_cargo += (telepAmount *
                    IS->is_world.w_weight[what]);
                if (telepTF != 0)
                {
                    rps->p_sh.sh_energy -= 5;
                }
                /* if the planet is infected, infect the ship */
                if (((rps->p_p.pl_plagueStage == 2) ||
                    (rps->p_p.pl_plagueStage == 3)) &&
                    (rps->p_sh.sh_plagueStage == 0))
                {
                    rps->p_sh.sh_plagueStage = 1;
                    rps->p_sh.sh_plagueTime = impRandom(IS,
                        IS->is_world.w_plagueOneRand) +
                        IS->is_world.w_plagueOneBase;
                }
                server2(IS, rt_unlockPlanetShipPair, plNum, shNum);
                if (count != countOrig)
                {
                    userN3(IS, "Events reduced the sale to ", count,
                        " units\n");
                    cost = price * telepAmount / 10;
                }
                if (count != telepAmount)
                {
                    userN3(IS, "Only ", telepAmount, " units arrived "
                        "safely\n");
                    if (shOwnPay)
                    {
                        cost = price * count / 10;
                    }
                    else
                    {
                        cost = price * telepAmount / 10;
                    }
                }
                *pFunds = *pFunds - cost;
            }
            else
            {
                err(IS, "insufficient funds");
            }
        }
        else
        {
            user(IS, "OK, none bought\n");
        }
    }
}

/*
 * makeSale - a sale for the given amount has been made.
 *          Takes into account that the current player may be either
 *          the seller or the buyer of the lot, and handles messages
 *          apropriately
 */

void makeSale(IMP, USHORT seller, USHORT buyer, USHORT offerNumber,
    ULONG amount)
{
    register Player_t *rp;
    BOOL saveNoWrite = FALSE;

    server(IS, rt_lockPlayer, buyer);
    rp = &IS->is_request.rq_u.ru_player;
    rp->p_money -= amount;
    server(IS, rt_unlockPlayer, buyer);
    if (IS->is_player.p_number == buyer)
    {
        IS->is_player.p_money = rp->p_money;
    }
    server(IS, rt_lockPlayer, seller);
    rp->p_money += amount;
    server(IS, rt_unlockPlayer, seller);
    if (IS->is_player.p_number == seller)
    {
        IS->is_player.p_money = rp->p_money;
    }
    server(IS, rt_readPlayer, buyer);
    if (IS->is_player.p_number != seller)
    {
        saveNoWrite = IS->is_noWrite;
        IS->is_noWrite = TRUE;
    }
    userN3(IS, "You made a sale via lot #", offerNumber, " to ");
    user(IS, &rp->p_name[0]);
        userN3(IS, ".\nYou are now $", amount, " richer!");
    if (IS->is_player.p_number == seller)
    {
        userNL(IS);
    }
    else
    {
        notify(IS, seller);
        IS->is_noWrite = saveNoWrite;
    }
    news(IS, n_make_sale, seller, buyer);
    if (IS->is_player.p_number != buyer)
    {
        saveNoWrite = IS->is_noWrite;
        IS->is_noWrite = TRUE;
        user(IS, &IS->is_player.p_name[0]);
        userN3(IS, " sold you goods via lot #", offerNumber, ".\n");
    }
    userN3(IS, "You are now $", amount, " poorer!");
    if (IS->is_player.p_number == buyer)
    {
        userNL(IS);
    }
    else
    {
        notify(IS, buyer);
        IS->is_noWrite = saveNoWrite;
    }
}

/*
 * cmd_buy - allows the player to buy goods
 */

BOOL cmd_buy(IMP)
{
    register Offer_t *rof;
    register Ship_t *rsh;
    register BigItem_t *rbi;
    register ItemType_t it;
    PlanetShipPair_t *rps;
    long cost;
    ULONG shipNumber, planetNumber, itemNumber, funds;
    USHORT seller;
    ULONG which;
    ShipType_t shType;
    Player_t player;
    Offer_t saveOff;
    BigItem_t saveBi;
    Ship_t saveShip;
    BOOL doAbort;
    UBYTE telepTF;

    if (IS->is_world.w_offerNext == 0)
    {
        err(IS, "nothing is for sale yet");
        return FALSE;
    }
    if (reqPosRange(IS, &which, IS->is_world.w_offerNext - 1,
        "Buy which lot"))
    {
        server(IS, rt_readOffer, which);
        rof = &IS->is_request.rq_u.ru_offer;
        rsh = &IS->is_request.rq_u.ru_ship;
        rps = &IS->is_request.rq_u.ru_planetShipPair;
        rbi = &IS->is_request.rq_u.ru_bigItem;
        seller = rof->of_who;
        server(IS, rt_readPlayer, seller);
        if ((IS->is_player.p_number == seller) || (getYourRelation(IS,
            &IS->is_request.rq_u.ru_player) != r_war))
        {
            player = IS->is_request.rq_u.ru_player;
            server(IS, rt_readOffer, which);
            rof = &IS->is_request.rq_u.ru_offer;
            if (printOffer(IS, which, &player))
            {
                /* we need to re-read it, since printOffer() might have */
                /* removed it */
                server(IS, rt_readOffer, which);
                if (rof->of_who == IS->is_player.p_number)
                {
                    err(IS, "can't buy from yourself");
                    return FALSE;
                }

                /* selling a ship */
                if (rof->of_state == of_ship)
                {
                    shipNumber = rof->of_.of_shipNumber;
                    server(IS, rt_readShip, shipNumber);
                    cost = rsh->sh_price *
                        IS->is_world.w_shipCost[rsh->sh_type];
                    if (cost < IS->is_player.p_money)
                    {
                        if (ask(IS, "Buy the ship"))
                        {
                            server(IS, rt_lockShip, shipNumber);
                            rsh->sh_price = 0;
                            rsh->sh_owner = IS->is_player.p_number;
                            /* erase any program */
                            memset(&rsh->sh_course[0], '\0',
                                MAX_PROG_STEPS * sizeof(char));
                            server(IS, rt_unlockShip, shipNumber);
                            shType = rsh->sh_type;
                            server(IS, rt_lockOffer, which);
                            rof->of_state = of_none;
                            server(IS, rt_unlockOffer, which);
                            user(IS, "You now own ");
                            user(IS, getShipName(shType));
                            userN3(IS, " #", shipNumber, "\n");
                            makeSale(IS, rof->of_who, IS->is_player.p_number,
                                which, cost);
                        }
                    }
                    else
                    {
                        err(IS, "you can't afford that ship");
                    }
                    return TRUE;
                }

                /* selling a big item on a planet */
                if (rof->of_state == of_item)
                {
                    saveOff = *rof;
                    itemNumber = rof->of_.of_itemNumber;
                    (void) skipBlanks(IS);
                    if (reqShip(IS, &shipNumber, "Ship to receive the "
                        "item"))
                    {
                        seller = rof->of_who;
                        server(IS, rt_readBigItem, itemNumber);
                        planetNumber = rbi->bi_itemLoc;
                        saveBi = *rbi;
                        if (rbi->bi_onShip || (rbi->bi_price == 0) ||
                            (rbi->bi_itemLoc == NO_ITEM) ||
                            (rbi->bi_status != bi_inuse))
                        {
                            err(IS, "unexpected condition encountered");
                            server(IS, rt_lockBigItem, itemNumber);
                            rbi->bi_price = 0;
                            server(IS, rt_unlockBigItem, itemNumber);
                            server(IS, rt_lockOffer, which);
                            rof->of_state = of_none;
                            server(IS, rt_unlockOffer, which);
                        }
                        else
                        {
                            cost = rbi->bi_price;
                            if (cost < IS->is_player.p_money)
                            {
                                server2(IS, rt_readPlanetShipPair,
                                    planetNumber, shipNumber);
                                if (rps->p_sh.sh_owner !=
                                    IS->is_player.p_number)
                                {
                                    err(IS, "you don't own that ship");
                                }
                                else if (rps->p_sh.sh_efficiency < EFFIC_WARN)
                                {
                                    err(IS, "ship is not efficient enough "
                                        "yet");
                                }
                                else if (rps->p_sh.sh_planet != planetNumber)
                                {
                                    err(IS, "ship is not on the same planet "
                                        "as the item");
                                }
                                else if (saveBi.bi_weight >
                                    (IS->is_world.w_shipCargoLim[
                                    rps->p_sh.sh_type] - rps->p_sh.sh_cargo))
                                {
                                    err(IS, "not enough room on that ship "
                                        "for the item");
                                }
                                else if (ask(IS, "Buy the item"))
                                {
                                    server(IS, rt_lockBigItem, itemNumber);
                                    rbi->bi_price = 0;
                                    rbi->bi_onShip = TRUE;
                                    rbi->bi_itemLoc = shipNumber;
                                    server(IS, rt_unlockBigItem, itemNumber);
                                    server2(IS, rt_lockPlanetShipPair,
                                        planetNumber, shipNumber);
                                    rps->p_sh.sh_cargo += saveBi.bi_weight;
                                    rps->p_sh.sh_items[
                                        cvtBpIt(saveBi.bi_part)]++;
                                    rps->p_p.pl_quantity[
                                        cvtBpIt(saveBi.bi_part)]--;
                                    /* see if the planet is infected */
                                    if (((rps->p_p.pl_plagueStage == 2) ||
                                        (rps->p_p.pl_plagueStage == 3)) &&
                                        (rps->p_sh.sh_plagueStage == 0))
                                    {
                                        rps->p_sh.sh_plagueStage = 1;
                                        rps->p_sh.sh_plagueTime = impRandom(IS,
                                            IS->is_world.w_plagueOneRand) +
                                            IS->is_world.w_plagueOneBase;
                                    }
                                    server2(IS, rt_unlockPlanetShipPair,
                                        planetNumber, shipNumber);
                                    server(IS, rt_lockOffer, which);
                                    rof->of_state = of_none;
                                    server(IS, rt_unlockOffer, which);
                                    user2(IS, getItemName(saveBi.bi_part),
                                        " is now loaded on ship ");
                                    userN3(IS, " #", shipNumber, "\n");
                                    makeSale(IS, rof->of_who,
                                        IS->is_player.p_number, which, cost);
                                }
                            }
                            else
                            {
                                err(IS, "you can't afford that item");
                            }
                        }
                        return TRUE;
                    }
                    return FALSE;
                }

                /* selling items on a planet */
                saveOff = *rof;
                planetNumber = saveOff.of_.of_plan.of_planetNumber;
                telepTF = 0;
                doAbort = FALSE;
                (void) skipBlanks(IS);
                if (reqShip(IS, &shipNumber, "Ship to receive items"))
                {
                    seller = rof->of_who;
                    server2(IS, rt_readPlanetShipPair, planetNumber,
                        shipNumber);
                    if (rps->p_sh.sh_owner != IS->is_player.p_number)
                    {
                        err(IS, "you don't own that ship");
                        doAbort = TRUE;
                    }
                    else if (rps->p_sh.sh_efficiency < EFFIC_WARN)
                    {
                        err(IS, "ship is not efficient enough yet");
                        doAbort = TRUE;
                    }
                    else if (rps->p_sh.sh_planet != planetNumber)
                    {
                        if (rps->p_sh.sh_planet == NO_ITEM)
                        {
                            saveShip = rps->p_sh;
                            if (whichPlanet(IS, saveShip.sh_row,
                                saveShip.sh_col) == planetNumber)
                            {
                                if (numInst(IS, &saveShip, bp_teleport) == 0)
                                {
                                    err(IS, "that ship does not have any "
                                        "teleport units installed");
                                    doAbort = TRUE;
                                }
                                else if (numInst(IS, &saveShip, bp_computer)
                                    == 0)
                                {
                                    err(IS, "that ship does not have any "
                                        "computers installed");
                                    doAbort = TRUE;
                                }
                                else if (getShipEff(IS, &saveShip, bp_teleport)
                                    < EFFIC_WARN)
                                {
                                    err(IS, "that ships teleport units are not"
                                        " efficient enough for operation");
                                    doAbort = TRUE;
                                }
                                else if (getShipEff(IS, &saveShip, bp_computer)
                                    < EFFIC_WARN)
                                {
                                    err(IS, "that ships computers are not "
                                        "efficient enough for operation");
                                    doAbort = TRUE;
                                }
                                else if (saveShip.sh_energy < 5)
                                {
                                    err(IS, "that ship does not have enough "
                                        "energy to operate the teleporters");
                                    doAbort = TRUE;
                                }
                                else
                                {
                                    /* build up the ship's teleport TF */
                                    telepTF = getShipTechFactor(IS, &saveShip,
                                        bp_teleport);
                                    telepTF = (telepTF + getShipTechFactor(IS,
                                        &saveShip, bp_computer)) / 2;
                                }
                                server2(IS, rt_readPlanetShipPair,
                                    planetNumber, shipNumber);
                            }
                            else
                            {
                                err(IS, "ship is not orbiting the planet "
                                    "the lot is offered on");
                                doAbort = TRUE;
                            }
                        }
                        else
                        {
                            err(IS, "ship is not on or orbiting the planet "
                                "the lot is offered on to");
                            doAbort = TRUE;
                        }
                    }
                    if (!doAbort)
                    {
                        funds = IS->is_player.p_money;
                        for (it = it_missiles; it <= IT_LAST_SMALL; it++)
                        {
                            doBuy(IS, planetNumber, shipNumber, it, &funds,
                                saveOff.of_.of_plan.prices[of_sellPrice][it],
                                telepTF, saveOff.of_.of_plan.of_payor);
                        }
                        if (funds != IS->is_player.p_money)
                        {
                        /* the offer sticks around */
                            makeSale(IS, seller, IS->is_player.p_number,
                                which, IS->is_player.p_money - funds);
                        }
                    }
                    return TRUE;
                }
                return FALSE;
            }
            userN3(IS, "Lot #", which, " is not available\n");
            return FALSE;
        }
        userN3(IS, "Lot #", which, " is not available\n");
        return FALSE;
    }
    return FALSE;
}

/*
 * doSell - try to sell a given item to a planet.
 *          Convention: the planet and ship pair is in the request on both
 *          entry and exit
 */

void doSell(IMP, ULONG plNum, ULONG shNum, register ItemType_t what,
    ULONG *pFunds, USHORT oPrice, UBYTE telepTF, UBYTE shOwnPay)
{
    register PlanetShipPair_t *rps;
    ULONG price, cost, count, countOrig, telepAmount;
    USHORT q1, q2, present;
    char buff[100];

    rps = &IS->is_request.rq_u.ru_planetShipPair;
    price = getItemCost(IS, what) * oPrice;
    present = rps->p_sh.sh_items[what];
    if (price != 0)
    {
        user2(IS, "Sell how many ", getItemName(what));
        userM3(IS, " at $", price, " (max ");
        userN(IS, present);
        user(IS, ")");
        getPrompt(IS, &buff[0]);
        if (reqPosRange(IS, &count, present, &buff[0]))
        {
            cost = price * count / 10;
            if (*pFunds > cost)
            {
                countOrig = count;
                server2(IS, rt_lockPlanetShipPair, plNum, shNum);
                q1 = rps->p_sh.sh_items[what];
                if (q1 < count)
                {
                    count = q1;
                }
                q2 = readPlQuan(IS, &rps->p_p, what);
                telepAmount = count;
                /* see if they were using teleports */
                if (telepTF != 0)
                {
                    /* check for possible teleport problems */
                    if (impRandom(IS, 100) < ((3 * telepTF) / 2))
                    {
                        /* everything went OK */
                        telepAmount = count;
                    }
                    else
                    {
                        telepAmount = ((count * (75 + impRandom(IS, 24))) / 100);
                    }
                }
                writePlQuan(IS, &rps->p_p, what, q2 + telepAmount);
                rps->p_sh.sh_items[what] -= count;
                rps->p_sh.sh_cargo -= (count * IS->is_world.w_weight[what]);
                if (telepTF != 0)
                {
                    rps->p_sh.sh_energy -= 5;
                }
                /* if the ship is infected, infect the planet */
                if (((rps->p_sh.sh_plagueStage == 2) ||
                    (rps->p_sh.sh_plagueStage == 3)) &&
                    (rps->p_p.pl_plagueStage == 0))
                {
                    rps->p_p.pl_plagueStage = 1;
                    rps->p_p.pl_plagueTime = impRandom(IS,
                        IS->is_world.w_plagueOneRand) +
                        IS->is_world.w_plagueOneBase;
                }
                server2(IS, rt_unlockPlanetShipPair, plNum, shNum);
                if (count != countOrig)
                {
                    userN3(IS, "Events reduced the sale to ", count,
                        " units\n");
                    cost = price * count / 10;
                }
                if (count != telepAmount)
                {
                    userN3(IS, "Only ", telepAmount, " units arrived "
                        "safely\n");
                    if (shOwnPay)
                    {
                        cost = price * telepAmount / 10;
                    }
                    else
                    {
                        cost = price * count / 10;
                    }
                }
                *pFunds = *pFunds - cost;
            }
            else
            {
                err(IS, "the planet owner has insufficient funds to buy that "
                    "many");
            }
        }
        else
        {
            user(IS, "OK, none sold\n");
        }
    }
}

/*
 * cmd_sell - allows the player to sell goods to a planet which has a
 *          "buying" price set for them.
 */

BOOL cmd_sell(IMP)
{
    register Offer_t *rof;
    register Ship_t *rsh;
    register BigItem_t *rbi;
    register ItemType_t it;
    PlanetShipPair_t *rps;
    ULONG shipNumber, planetNumber, funds;
    USHORT buyer;
    ULONG which;
    Player_t player;
    Offer_t saveOff;
    Ship_t saveShip;
    BOOL doAbort;
    UBYTE telepTF;

    if (IS->is_world.w_offerNext == 0)
    {
        err(IS, "nobody is buying anything yet");
        return FALSE;
    }
    if (reqPosRange(IS, &which, IS->is_world.w_offerNext - 1,
        "Sell to which lot"))
    {
        server(IS, rt_readOffer, which);
        rof = &IS->is_request.rq_u.ru_offer;
        rsh = &IS->is_request.rq_u.ru_ship;
        rps = &IS->is_request.rq_u.ru_planetShipPair;
        rbi = &IS->is_request.rq_u.ru_bigItem;
        saveOff = *rof;
        buyer = rof->of_who;
        server(IS, rt_readPlayer, buyer);
        if ((IS->is_player.p_number == buyer) || (getYourRelation(IS,
            &IS->is_request.rq_u.ru_player) != r_war))
        {
            player = IS->is_request.rq_u.ru_player;
            *rof = saveOff;
            if (printOffer(IS, which, &player))
            {
                /* we need to re-read it, since printOffer() might have */
                /* removed it */
                server(IS, rt_readOffer, which);
                if (rof->of_who == IS->is_player.p_number)
                {
                    err(IS, "can't sell to yourself");
                    return FALSE;
                }

                /* are they selling a ship or a big item? */
                if ((rof->of_state == of_ship) || (rof->of_state == of_item))
                {
                    err(IS, "that lot does not cover the purchase of small "
                        "items");
                    return FALSE;
                }
                saveOff = *rof;
                planetNumber = saveOff.of_.of_plan.of_planetNumber;
                telepTF = 0;
                doAbort = FALSE;
                (void) skipBlanks(IS);
                if (reqShip(IS, &shipNumber, "Sell items on ship"))
                {
                    server2(IS, rt_readPlanetShipPair, planetNumber,
                        shipNumber);
                    if (rps->p_sh.sh_owner != IS->is_player.p_number)
                    {
                        err(IS, "you don't own that ship");
                        doAbort = TRUE;
                    }
                    else if (rps->p_sh.sh_efficiency < EFFIC_WARN)
                    {
                        err(IS, "ship is not efficient enough yet");
                        doAbort = TRUE;
                    }
                    else if (rps->p_sh.sh_planet != planetNumber)
                    {
                        if (rps->p_sh.sh_planet == NO_ITEM)
                        {
                            saveShip = rps->p_sh;
                            if (whichPlanet(IS, saveShip.sh_row,
                                saveShip.sh_col) == planetNumber)
                            {
                                if (numInst(IS, &saveShip, bp_teleport) == 0)
                                {
                                    err(IS, "that ship does not have any "
                                        "teleport units installed");
                                    doAbort = TRUE;
                                }
                                else if (numInst(IS, &saveShip, bp_computer)
                                    == 0)
                                {
                                    err(IS, "that ship does not have any "
                                        "computers installed");
                                    doAbort = TRUE;
                                }
                                else if (getShipEff(IS, &saveShip, bp_teleport)
                                    < EFFIC_WARN)
                                {
                                    err(IS, "that ships teleport units are not"
                                        " efficient enough for operation");
                                    doAbort = TRUE;
                                }
                                else if (getShipEff(IS, &saveShip, bp_computer)
                                    < EFFIC_WARN)
                                {
                                    err(IS, "that ships computers are not "
                                        "efficient enough for operation");
                                    doAbort = TRUE;
                                }
                                else if (saveShip.sh_energy < 5)
                                {
                                    err(IS, "that ship does not have enough "
                                        "energy to operate the teleporters");
                                    doAbort = TRUE;
                                }
                                else
                                {
                                    /* build up the ship's teleport TF */
                                    telepTF = getShipTechFactor(IS, &saveShip,
                                        bp_teleport);
                                    telepTF = (telepTF + getShipTechFactor(IS,
                                        &saveShip, bp_computer)) / 2;
                                }
                                server2(IS, rt_readPlanetShipPair,
                                    planetNumber, shipNumber);
                            }
                            else
                            {
                                err(IS, "ship is not orbiting the planet "
                                    "the lot is offered on");
                                doAbort = TRUE;
                            }
                        }
                        else
                        {
                            err(IS, "ship is not on or orbiting the planet "
                                "the lot is offered on to");
                            doAbort = TRUE;
                        }
                    }
                    if (!doAbort)
                    {
                        funds = player.p_money;
                        for (it = it_missiles; it <= IT_LAST_SMALL; it++)
                        {
                            doSell(IS, planetNumber, shipNumber, it, &funds,
                                saveOff.of_.of_plan.prices[of_buyPrice][it],
                                telepTF, saveOff.of_.of_plan.of_payor);
                        }
                        if (funds != player.p_money)
                        {
                        /* the offer sticks around */
                            makeSale(IS, IS->is_player.p_number, buyer,
                                which, player.p_money - funds);
                        }
                    }
                    return TRUE;
                }
                return FALSE;
            }
            userN3(IS, "Lot #", which, " is not available\n");
            return FALSE;
        }
        userN3(IS, "Lot #", which, " is not available\n");
        return FALSE;
    }
    return FALSE;
}

