/*
 * Imperium
 *
 * $Id: cmd_general3.c,v 1.2 2000/05/18 06:50:02 marisa Exp $
 *
 * code related to displaying player-related stats
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
 * $Log: cmd_general3.c,v $
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:49  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:24:16  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:08  marisag
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

#define CmdGen3C 1
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"

static char const rcsid[] = "$Id: cmd_general3.c,v 1.2 2000/05/18 06:50:02 marisa Exp $";

static char const *SHIP_DESC[ST_LAST + 1] =
    {
        "Scout Class Hull",
        "Light Duty Hull",
        "Medium Duty Hull",
        "Heavy Duty Hull",
        "Superliner Class Hull",
        "Robot Planet Miner"
    };
/*
 * buildItemWeight - builds up the weight of a big item at creation
 *          (and possibly refurbish) time. Note we don't just set the
 *          weight because the caller may want to do further things with
 *          the item before changing it.
 *          Note: this basically requires that the big item struct
 *          be initialized for all other values before it is called
 */

USHORT buildItemWeight(IMP, register BigItem_t *bi)
{
    register USHORT workWeight;
    USHORT weight;

    /* get base item weight depending on what the big item is */
    switch(bi->bi_part)
    {
        case bp_computer:
            weight = IS->is_world.w_weight[cvtBpIt(bi->bi_part)];
            break;
        case bp_engines:
            weight = IS->is_world.w_weight[cvtBpIt(bi->bi_part)];
            break;
        case bp_lifeSupp:
            weight = IS->is_world.w_weight[cvtBpIt(bi->bi_part)];
            break;
        case bp_sensors:
            weight = IS->is_world.w_weight[cvtBpIt(bi->bi_part)];
            break;
        case bp_teleport:
            weight = IS->is_world.w_weight[cvtBpIt(bi->bi_part)];
            break;
        case bp_shield:
            weight = IS->is_world.w_weight[cvtBpIt(bi->bi_part)];
            weight = (weight * 4) / 3;
            break;
        case bp_tractor:
            weight = IS->is_world.w_weight[cvtBpIt(bi->bi_part)];
            weight = (weight * 3) / 2;
            break;
        case bp_blaser:
            /* note that blasers weigh about 2/3rds of photon torps */
            weight = ((IS->is_world.w_weight[cvtBpIt(bi->bi_part)] * 2) / 3);
            break;
        case bp_photon:
            weight = IS->is_world.w_weight[cvtBpIt(bi->bi_part)];
            break;

        /* note that there is no "weight" for the ship's hull, so we just */
        /* drop through */
        case bp_hull:
        default:
            /* no item, so no weight */
            return(0);
    }
    /* now we need to reduce the weight due to tech level, if apropriate */
    /* note that this will limit weight reduction to about 30% */
    workWeight = (weight * (getTechFactor(bi->bi_techLevel) / 3)) / 100;
    /* return the finished weight */
    return (weight - workWeight);
}

/*
 * tellShipTypes - tells the user (if no typeahead) what kind of ships may
 *          be built on the planet
 */

void tellShipTypes(IMP, register Planet_t *pl)
{
    register USHORT type;

    if (*IS->is_textInPos != '\0')
    {
	return;
    }
    userP(IS, "Planet ", pl, " has ");
    userN(IS, pl->pl_prod[pp_hull]);
    user(IS, " production units.\nThis will allow you to build the "
	 "following types of ships:\n\n"
	 "  Cost   Class   Cargo  Desc\n"
	 "----------------------------------------\n");
    for (type = 0; type <= ST_LAST; type++)
    {
	if (IS->is_world.w_shipCost[type] <= pl->pl_prod[pp_hull])
	{
	    userC(IS, ' ');
            userF(IS, IS->is_world.w_shipCost[type], 6);
            user(IS, "  ");
            userC(IS, SHIP_CHAR[type]);
            user(IS, "      ");
            userF(IS, IS->is_world.w_shipCargoLim[type], 6);
            user3(IS, "  ", SHIP_DESC[type], "\n");
        }
    }
    user(IS, "\n");
    uPrompt(IS, "What class of ship");
}

/*
 * doBuild - part of cmd_build. This function questions the user about what
 *          kind of item they want to build on that planet, and creates the
 *          objects. It is meant to be called by scanPlanets.
 *          Note: The planet is NOT expected to be in the buffer, and the
 *          buffer WILL be trashed.
 */

void doBuild(IMP, register Planet_t *pl)
{
    char buf[100];
    register Planet_t *rp;
    register Player_t *rPlay;
    Ship_t *rsh;
    World_t *rw;
    BigItem_t *rbi;
    USHORT cost = 0;
    ULONG shipNumber=NO_ITEM, itemNumber=NO_ITEM, *numPtr=NULL;
    UBYTE typ;
    BOOL buildAbort;
    Planet_t savePl;
    BigItem_t saveIt;
    USHORT it, curIt, maxNum, bp, plTF;
    char *prompt, *choices;

    buildAbort = FALSE;
    it = 0;
    /* set up all out pointers */
    rp = &IS->is_request.rq_u.ru_planet;
    rPlay = &IS->is_request.rq_u.ru_player;
    rsh = &IS->is_request.rq_u.ru_ship;
    rbi = &IS->is_request.rq_u.ru_bigItem;
    rw = &IS->is_request.rq_u.ru_world;

    /* read in the planet */
    server(IS, rt_lockPlanet, pl->pl_number);
    /* do an initial update, incase there may be waiting production */
    updatePlanet(IS);
    /* Make sure they have enough BTUs on the planet to build something */
    if (rp->pl_btu < 5)
    {
	server(IS, rt_unlockPlanet, pl->pl_number);
	return;
    }
    rp->pl_btu -= 5;
    /* get the planets tech factor for later use */
    plTF = getTechFactor(rp->pl_techLevel);
    server(IS, rt_unlockPlanet, pl->pl_number);

    /* set the things they can build depending on the tech levels */
    /* NOTE: You MUST place things in ascending tech factor order! */
    /* It must be possible to chop off the list and have all items below */
    /* that continue to use the same numbers */
    if (plTF > 35)
    {
        /* add in teleports */
        choices = "computer\0engine\0lifesup\0sensor\0weapon\0"
            "ship\0shield\0tractor\0teleport\0";
        prompt = "Build what (computer/engine/lifesup/sensor/teleport/weapon/"
            "shield/tractor/ship)";
    }
    else
    {
        if (plTF > 25)
        {
            if (plTF > 30)
            {
                /* add in tractor beams */
                choices = "computer\0engine\0lifesup\0sensor\0weapon\0ship\0"
                    "shield\0tractor\0";
                prompt = "Build what (computer/engine/lifesup/sensor/weapon/"
                    "shield/tractor/ship)";
            }
            else
            {
                /* add in shields */
                choices = "computer\0engine\0lifesup\0sensor\0weapon\0ship\0"
                    "shield\0";
                prompt = "Build what (computer/engine/lifesup/sensor/weapon/"
                    "shield/ship)";
            }
        }
        else
        {
            /* standard items they can build */
            choices = "computer\0engine\0lifesup\0sensor\0weapon\0ship\0";
            prompt = "Build what (computer/engine/lifesup/sensor/weapon/"
                "ship)";
        }
    }


    /* get the type of item they wish to build */
    if (reqChoice(IS, &it, choices, prompt))
    {
        /* switch depending on the item type they wanted to build */
        switch(it)
        {
            /* they want to build a teleport system */
            case 8:
                IS->is_BOOL2 = TRUE;
                /* check if there is enough production */
                cost = (IS->is_world.w_prodCost[pp_electronics] * 3);
                if (cost > rp->pl_prod[pp_electronics])
                {
                    /* The planet does not have enough production units */
                    /* for a teleport unit */
                    err(IS, "insufficient production units");
                }
                else
                {
                    /* The planet has enough production units */
                    /* so check the players wealth */
                    if (((long)cost * IS->is_world.w_shipCostMult) >
                        IS->is_player.p_money)
                    {
                        /* The player is too poor to build a teleport unit */
                        err(IS, "you don't have enough money");
                    }
                    else
                    {
                        /* They can afford it, so get to it */
                        /* decrease the planets production */
                        server(IS, rt_lockPlanet, pl->pl_number);
                        rp->pl_prod[pp_electronics] -= cost;
                        /* increase the planets big item count */
                        rp->pl_bigItems++;
                        /* increase the planets item count */
                        rp->pl_quantity[it_elect]++;
                        /* save a copy of the planet for later */
                        savePl = *rp;
                        server(IS, rt_unlockPlanet, pl->pl_number);
                        cost *= IS->is_world.w_shipCostMult;
                        /* remove the funds from the player */
                        server(IS, rt_lockPlayer, IS->is_player.p_number);
                        rPlay->p_money -= cost;
                        server(IS, rt_unlockPlayer, IS->is_player.p_number);
                        IS->is_player.p_money = rPlay->p_money;
                        IS->is_USHORT2 += cost;
                        /* incr the item count in the world */
                        server(IS, rt_lockWorld, 0);
                        itemNumber = rw->w_bigItemNext;
                        rw->w_bigItemNext++;
                        server(IS, rt_unlockWorld, 0);
                        IS->is_world.w_bigItemNext = rw->w_bigItemNext;

                        /* now we are ready to create the new items */
                        /* fill in the big item struct */
                        rbi->bi_lastUpdate = timeNow(IS);
                        rbi->bi_itemLoc = savePl.pl_number;
                        rbi->bi_techLevel = savePl.pl_techLevel;
                        rbi->bi_number = itemNumber;
                        rbi->bi_part = bp_teleport;
                        rbi->bi_status = bi_inuse;
                        rbi->bi_onShip = FALSE;
                        rbi->bi_effic = 50;
                        rbi->bi_price = 0;
                        /* get the teleport system weight */
                        rbi->bi_weight = buildItemWeight(IS, rbi);
                        server(IS, rt_createBigItem, itemNumber);
                        userN2(IS, "Built teleport unit ", itemNumber);
                        userP(IS, " on ", &savePl, ".\n");
                    }
                }
                break;

            /* they want to build a tractor beam */
            case 7:
                IS->is_BOOL2 = TRUE;
                /* check if there is enough production */
                cost = (IS->is_world.w_prodCost[pp_electronics] * 3);
                if (cost > rp->pl_prod[pp_electronics])
                {
                    /* The planet does not have enough production units */
                    /* for a tractor beam */
                    err(IS, "insufficient production units");
                }
                else
                {
                    /* The planet has enough production units */
                    /* so check the players wealth */
                    if (((long)cost * IS->is_world.w_shipCostMult) >
                        IS->is_player.p_money)
                    {
                        /* The player is too poor to build a tractor beam */
                        err(IS, "you don't have enough money");
                    }
                    else
                    {
                        /* They can afford it, so get to it */
                        /* decrease the planets production */
                        server(IS, rt_lockPlanet, pl->pl_number);
                        rp->pl_prod[pp_electronics] -= cost;
                        /* increase the planets big item count */
                        rp->pl_bigItems++;
                        /* increase the planets item count */
                        rp->pl_quantity[it_elect]++;
                        /* save a copy of the planet for later */
                        savePl = *rp;
                        server(IS, rt_unlockPlanet, pl->pl_number);
                        cost *= IS->is_world.w_shipCostMult;
                        /* remove the funds from the player */
                        server(IS, rt_lockPlayer, IS->is_player.p_number);
                        rPlay->p_money -= cost;
                        server(IS, rt_unlockPlayer, IS->is_player.p_number);
                        IS->is_player.p_money = rPlay->p_money;
                        IS->is_USHORT2 += cost;
                        /* incr the item count in the world */
                        server(IS, rt_lockWorld, 0);
                        itemNumber = rw->w_bigItemNext;
                        rw->w_bigItemNext++;
                        server(IS, rt_unlockWorld, 0);
                        IS->is_world.w_bigItemNext = rw->w_bigItemNext;

                        /* now we are ready to create the new items */
                        /* fill in the big item struct */
                        rbi->bi_lastUpdate = timeNow(IS);
                        rbi->bi_itemLoc = savePl.pl_number;
                        rbi->bi_techLevel = savePl.pl_techLevel;
                        rbi->bi_number = itemNumber;
                        rbi->bi_part = bp_tractor;
                        rbi->bi_status = bi_inuse;
                        rbi->bi_onShip = FALSE;
                        rbi->bi_effic = 50;
                        rbi->bi_price = 0;
                        /* get the tractor beam weight */
                        rbi->bi_weight = buildItemWeight(IS, rbi);
                        server(IS, rt_createBigItem, itemNumber);
                        userN2(IS, "Built tractor beam ", itemNumber);
                        userP(IS, " on ", &savePl, ".\n");
                    }
                }
                break;

            /* they want to build a shield system */
            case 6:
                IS->is_BOOL2 = TRUE;
                /* check if there is enough production */
                cost = (IS->is_world.w_prodCost[pp_electronics] * 3);
                if (cost > rp->pl_prod[pp_electronics])
                {
                    /* The planet does not have enough production units */
                    /* for a shield unit */
                    err(IS, "insufficient production units");
                }
                else
                {
                    /* The planet has enough production units */
                    /* so check the players wealth */
                    if (((long)cost * IS->is_world.w_shipCostMult) >
                        IS->is_player.p_money)
                    {
                        /* The player is too poor to build a shield unit */
                        err(IS, "you don't have enough money");
                    }
                    else
                    {
                        /* They can afford it, so get to it */
                        /* decrease the planets production */
                        server(IS, rt_lockPlanet, pl->pl_number);
                        rp->pl_prod[pp_electronics] -= cost;
                        /* increase the planets big item count */
                        rp->pl_bigItems++;
                        /* increase the planets item count */
                        rp->pl_quantity[it_elect]++;
                        /* save a copy of the planet for later */
                        savePl = *rp;
                        server(IS, rt_unlockPlanet, pl->pl_number);
                        cost *= IS->is_world.w_shipCostMult;
                        /* remove the funds from the player */
                        server(IS, rt_lockPlayer, IS->is_player.p_number);
                        rPlay->p_money -= cost;
                        server(IS, rt_unlockPlayer, IS->is_player.p_number);
                        IS->is_player.p_money = rPlay->p_money;
                        IS->is_USHORT2 += cost;
                        /* incr the item count in the world */
                        server(IS, rt_lockWorld, 0);
                        itemNumber = rw->w_bigItemNext;
                        rw->w_bigItemNext++;
                        server(IS, rt_unlockWorld, 0);
                        IS->is_world.w_bigItemNext = rw->w_bigItemNext;

                        /* now we are ready to create the new items */
                        /* fill in the big item struct */
                        rbi->bi_lastUpdate = timeNow(IS);
                        rbi->bi_itemLoc = savePl.pl_number;
                        rbi->bi_techLevel = savePl.pl_techLevel;
                        rbi->bi_number = itemNumber;
                        rbi->bi_part = bp_shield;
                        rbi->bi_status = bi_inuse;
                        rbi->bi_onShip = FALSE;
                        rbi->bi_effic = 50;
                        rbi->bi_price = 0;
                        /* get the shield weight */
                        rbi->bi_weight = buildItemWeight(IS, rbi);
                        server(IS, rt_createBigItem, itemNumber);
                        userN2(IS, "Built shield ", itemNumber);
                        userP(IS, " on ", &savePl, ".\n");
                    }
                }
                break;

            /* they want to build a ship */
            case 5:
                (void) skipBlanks(IS);
                IS->is_BOOL2 = TRUE;
                tellShipTypes(IS, rp);
                getPrompt(IS, &buf[0]);
                /* find out what kind of ship they want to build */
                if (reqShipType(IS, &typ, &buf[0]))
                {
                    /* check if they can afford that ship type */
                    cost = IS->is_world.w_shipCost[typ];
                    if (cost > rp->pl_prod[pp_hull])
                    {
                        /* The planet does not have enough production units */
                        /* for that type of ship */
                        err(IS, "insufficient production units");
                    }
                    else
                    {
                        /* The planet has enough production units for that */
                        /* item, so check the players wealth */
                        if (((long)cost * IS->is_world.w_shipCostMult) >
                            IS->is_player.p_money)
                        {
                            /* The player is too poor to make that ship type */
                            err(IS, "you don't have enough money");
                        }
                        else
                        {
                            /* They can afford it, so get to it */
/*
 BUBBA

Need to come up with a way we can create new items and ships etc.
while keeping the world locked, so that people don't try and read the
item we just created before it is actually written to disk
*/

                            server(IS, rt_lockPlanet, pl->pl_number);
                            /* decrease the planets production */
                            rp->pl_prod[pp_hull] -= cost;
                            /* save a copy of the planet for later */
                            savePl = *rp;
                            server(IS, rt_unlockPlanet, pl->pl_number);
                            cost *= IS->is_world.w_shipCostMult;
                            /* remove the funds from the player */
                            server(IS, rt_lockPlayer, IS->is_player.p_number);
                            rPlay->p_money -= cost;
                            server(IS, rt_unlockPlayer,
                                IS->is_player.p_number);
                            IS->is_player.p_money = rPlay->p_money;
                            IS->is_USHORT2 += cost;
                            /* incr the ship and item counts in the world */
                            server(IS, rt_lockWorld, 0);
                            shipNumber = rw->w_shipNext;
                            rw->w_shipNext++;
                            if (typ != st_m)
                            {
                                itemNumber = rw->w_bigItemNext;
                                rw->w_bigItemNext++;
                            }
                            server(IS, rt_unlockWorld, 0);
                            IS->is_world.w_shipNext = rw->w_shipNext;
                            IS->is_world.w_bigItemNext = rw->w_bigItemNext;

                            if (typ != st_m)
                            {
                                /* now we are ready to create the new items */
                                /* fill in the big item struct */
                                rbi->bi_lastUpdate = timeNow(IS);
                                rbi->bi_itemLoc = shipNumber;
                                rbi->bi_techLevel = savePl.pl_techLevel;
                                rbi->bi_number = itemNumber;
                                rbi->bi_part = bp_hull;
                                rbi->bi_status = bi_inuse;
                                rbi->bi_onShip = TRUE;
                                rbi->bi_effic = 50;
                                rbi->bi_price = 0;
                                /* note that weight returned here will */
                                /* probably be zero, as it is not used for */
                                /* hulls */
                                rbi->bi_weight = buildItemWeight(IS, rbi);
                                server(IS, rt_createBigItem, itemNumber);
                                saveIt = *rbi;
                            }

                            /* first blank out struct to save time */
                            memset(rsh, '\0', sizeof(Ship_t) * sizeof(char));
                            rsh->sh_number = shipNumber;
                            rsh->sh_planet = savePl.pl_number;
                            rsh->sh_lastUpdate = timeNow(IS);
                            rsh->sh_price = 0;
                            rsh->sh_fuelLeft = 0;
                            if (typ != st_m)
                            {
                                rsh->sh_cargo = saveIt.bi_weight;
                            }
                            rsh->sh_armourLeft = 0;
                            rsh->sh_row = savePl.pl_row;
                            rsh->sh_col = savePl.pl_col;
                            rsh->sh_type = typ;
                            rsh->sh_fleet = '*';
                            rsh->sh_efficiency = 50;
                            rsh->sh_flags = SHF_NONE;
                            rsh->sh_dragger = NO_ITEM;
                            rsh->sh_owner = IS->is_player.p_number;
                            rsh->sh_plagueStage = 0;
                            rsh->sh_plagueTime = 0;
                            rsh->sh_hullTF = getTechFactor(
                                savePl.pl_techLevel);
                            if (typ != st_m)
                            {
                                rsh->sh_hull = itemNumber;
                            }
                            else
                            {
                                rsh->sh_hull = NO_ITEM;
                            }
                            for (bp = BP_FIRST; bp <= BP_LAST; bp++)
                            {
                                switch(bp)
                                {
                                    case bp_computer:
                                        maxNum = MAX_COMP;
                                        numPtr = &rsh->sh_computer[0];
                                        break;
                                    case bp_engines:
                                        maxNum = MAX_ENG;
                                        numPtr = &rsh->sh_engine[0];
                                        break;
                                    case bp_lifeSupp:
                                        maxNum = MAX_LIFESUP;
                                        numPtr = &rsh->sh_lifeSupp[0];
                                        break;
                                    case bp_sensors:
                                        /* note that we only specify one */
                                        /* of the modules */
                                        maxNum = MAX_ELECT;
                                        numPtr = &rsh->sh_elect[0];
                                        break;
                                    case bp_photon:
                                        /* note that we only specify one */
                                        /* of the weapons */
                                        maxNum = MAX_WEAP;
                                        numPtr = &rsh->sh_weapon[0];
                                        break;
                                    default:
                                        maxNum = 0;
                                        break;
                                }
                                if (maxNum)
                                {
                                    for (curIt = 0; curIt < maxNum; curIt++)
                                    {
                                        numPtr[curIt] = NO_ITEM;
                                    }
                                }
                            }
                            /* if a miner, set default program */
                            if (typ == st_m)
                            {
                                rsh->sh_weapon[0] = 2;
                                rsh->sh_weapon[1] = 1;
                                rsh->sh_weapon[2] = 2;
                                rsh->sh_weapon[3] = savePl.pl_techLevel;
                                /* erase the production levels */
                                rsh->sh_elect[0] = 0;
                                rsh->sh_elect[1] = 1;
                            }
                            /* actually build the ship */
                            server(IS, rt_createShip, shipNumber);
                            /* increment the ship counts */
                            incrShipCount(IS, mapSector(IS, savePl.pl_row,
                                savePl.pl_col));
                            incrPlShipCount(IS, savePl.pl_number);
                            user3(IS, "Built \"", getShipName(typ),
                                "\" ship number ");
                            userN(IS, shipNumber);
                            userP(IS, " on ", &savePl, ".\n");
                        }
                    }
                }
                break;

            /* they want to build a weapon */
            case 4:
                (void) skipBlanks(IS);
                /* see what weapons they can build */
                /* see notes about tech factors above! */
                if (plTF > 35)
                {
                    /* for now the same weapons */
                    choices = "blaser\0photon\0";
                    prompt = "What type of weapon (blaser/photon)";
                }
                else
                {
                    /* default weapon types */
                    choices = "blaser\0photon\0";
                    prompt = "What type of weapon (blaser/photon)";
                }
                IS->is_BOOL2 = TRUE;
                userN(IS, rp->pl_prod[pp_weapon]);
                userP(IS, " production units on planet ", rp, "\n");
                if (reqChoice(IS, &it, choices, prompt))
                {
                    /* now convert the above number into a big part type */
                    switch(it)
                    {
                        case 1:
                            typ = bp_photon;
                            break;
                        case 0:
                            typ = bp_blaser;
                            break;
                    }
                    /* check if they can afford that ship type */
                    switch(typ)
                    {
                        case bp_blaser:
                            cost = ((IS->is_world.w_prodCost[pp_weapon] * 3)
                                / 2);
                            break;
                        case bp_photon:
                            cost = ((IS->is_world.w_prodCost[pp_weapon] * 4)
                                / 3);
                            break;
			default:
			    cost = 0;
			    err(IS, ">>> Unexpected default reached");
			    break;
                    }
                    if (cost > rp->pl_prod[pp_weapon])
                    {
                        /* The planet does not have enough production units */
                        /* for that type of weapon */
                        err(IS, "insufficient production units");
                    }
                    else
                    {
                        /* The planet has enough production units for that */
                        /* item, so check the players wealth */
                        if (((long)cost * IS->is_world.w_shipCostMult) >
                            IS->is_player.p_money)
                        {
                            /* The can't afford that weapon type */
                            err(IS, "you don't have enough money");
                        }
                        else
                        {
                            /* They can afford it, so get to it */
                            server(IS, rt_lockPlanet, pl->pl_number);
                            /* decrease the planets production */
                            rp->pl_prod[pp_weapon] -= cost;
                            /* increase the planets big item count */
                            rp->pl_bigItems++;
                            /* increase the planets item count */
                            rp->pl_quantity[it_weapons]++;
                            /* save a copy of the planet for later */
                            savePl = *rp;
                            server(IS, rt_unlockPlanet, pl->pl_number);
                            cost *= IS->is_world.w_shipCostMult;
                            /* remove the funds from the player */
                            server(IS, rt_lockPlayer, IS->is_player.p_number);
                            rPlay->p_money -= cost;
                            server(IS, rt_unlockPlayer,
                                IS->is_player.p_number);
                            IS->is_player.p_money = rPlay->p_money;
                            IS->is_USHORT2 += cost;
                            /* incr the item counts in the world */
                            server(IS, rt_lockWorld, 0);
                            itemNumber = rw->w_bigItemNext;
                            rw->w_bigItemNext++;
                            server(IS, rt_unlockWorld, 0);
                            IS->is_world.w_bigItemNext = rw->w_bigItemNext;

                            /* now we are ready to create the new items */
                            /* fill in the big item struct */
                            rbi->bi_lastUpdate = timeNow(IS);
                            rbi->bi_itemLoc = savePl.pl_number;
                            rbi->bi_techLevel = savePl.pl_techLevel;
                            rbi->bi_number = itemNumber;
                            rbi->bi_part = typ;
                            rbi->bi_status = bi_inuse;
                            rbi->bi_onShip = FALSE;
                            rbi->bi_effic = 50;
                            rbi->bi_weight = buildItemWeight(IS, rbi);
                            rbi->bi_price = 0;
                            server(IS, rt_createBigItem, itemNumber);
                            userN2(IS, "Built weapon ", itemNumber);
                            userP(IS, " on ", &savePl, ".\n");
                        }
                    }
                }
                break;

            /* they want to build a sensor system */
            case 3:
                IS->is_BOOL2 = TRUE;
                /* check if there is enough production */
                cost = (IS->is_world.w_prodCost[pp_electronics] * 2);
                if (cost > rp->pl_prod[pp_electronics])
                {
                    /* The planet does not have enough production units */
                    /* for a sensor unit */
                    err(IS, "insufficient production units");
                }
                else
                {
                    /* The planet has enough production units */
                    /* so check the players wealth */
                    if (((long)cost * IS->is_world.w_shipCostMult) >
                        IS->is_player.p_money)
                    {
                        /* The player is too poor to build a sensor unit */
                        err(IS, "you don't have enough money");
                    }
                    else
                    {
                        /* They can afford it, so get to it */
                        /* decrease the planets production */
                        server(IS, rt_lockPlanet, pl->pl_number);
                        rp->pl_prod[pp_electronics] -= cost;
                        /* increase the planets big item count */
                        rp->pl_bigItems++;
                        /* increase the planets item count */
                        rp->pl_quantity[it_elect]++;
                        /* save a copy of the planet for later */
                        savePl = *rp;
                        server(IS, rt_unlockPlanet, pl->pl_number);
                        cost *= IS->is_world.w_shipCostMult;
                        /* remove the funds from the player */
                        server(IS, rt_lockPlayer, IS->is_player.p_number);
                        rPlay->p_money -= cost;
                        server(IS, rt_unlockPlayer, IS->is_player.p_number);
                        IS->is_player.p_money = rPlay->p_money;
                        IS->is_USHORT2 += cost;
                        /* incr the item count in the world */
                        server(IS, rt_lockWorld, 0);
                        itemNumber = rw->w_bigItemNext;
                        rw->w_bigItemNext++;
                        server(IS, rt_unlockWorld, 0);
                        IS->is_world.w_bigItemNext = rw->w_bigItemNext;

                        /* now we are ready to create the new items */
                        /* fill in the big item struct */
                        rbi->bi_lastUpdate = timeNow(IS);
                        rbi->bi_itemLoc = savePl.pl_number;
                        rbi->bi_techLevel = savePl.pl_techLevel;
                        rbi->bi_number = itemNumber;
                        rbi->bi_part = bp_sensors;
                        rbi->bi_status = bi_inuse;
                        rbi->bi_onShip = FALSE;
                        rbi->bi_effic = 50;
                        rbi->bi_price = 0;
                        /* get the sensor system weight */
                        rbi->bi_weight = buildItemWeight(IS, rbi);
                        server(IS, rt_createBigItem, itemNumber);
                        userN2(IS, "Built sensor system ", itemNumber);
                        userP(IS, " on ", &savePl, ".\n");
                    }
                }
                break;

            /* they want to build a life supp system */
            case 2:
                IS->is_BOOL2 = TRUE;
                /* check if there is enough production */
                cost = IS->is_world.w_prodCost[pp_electronics];
                if (cost > rp->pl_prod[pp_electronics])
                {
                    /* The planet does not have enough production units */
                    /* for a lifesupp unit */
                    err(IS, "insufficient production units");
                }
                else
                {
                    /* The planet has enough production units */
                    /* so check the players wealth */
                    if (((long)cost * IS->is_world.w_shipCostMult) >
                        IS->is_player.p_money)
                    {
                        /* The player is too poor to build a life sup unit */
                        err(IS, "you don't have enough money");
                    }
                    else
                    {
                        /* They can afford it, so get to it */
                        /* decrease the planets production */
                        server(IS, rt_lockPlanet, pl->pl_number);
                        rp->pl_prod[pp_electronics] -= cost;
                        /* increase the planets big item count */
                        rp->pl_bigItems++;
                        /* increase the planets item count */
                        rp->pl_quantity[it_lifeSupp]++;
                        /* save a copy of the planet for later */
                        savePl = *rp;
                        server(IS, rt_unlockPlanet, pl->pl_number);
                        cost *= IS->is_world.w_shipCostMult;
                        /* remove the funds from the player */
                        server(IS, rt_lockPlayer, IS->is_player.p_number);
                        rPlay->p_money -= cost;
                        server(IS, rt_unlockPlayer, IS->is_player.p_number);
                        IS->is_player.p_money = rPlay->p_money;
                        IS->is_USHORT2 += cost;
                        /* incr the item count in the world */
                        server(IS, rt_lockWorld, 0);
                        itemNumber = rw->w_bigItemNext;
                        rw->w_bigItemNext++;
                        server(IS, rt_unlockWorld, 0);
                        IS->is_world.w_bigItemNext = rw->w_bigItemNext;

                        /* now we are ready to create the new items */
                        /* fill in the big item struct */
                        rbi->bi_lastUpdate = timeNow(IS);
                        rbi->bi_itemLoc = savePl.pl_number;
                        rbi->bi_techLevel = savePl.pl_techLevel;
                        rbi->bi_number = itemNumber;
                        rbi->bi_part = bp_lifeSupp;
                        rbi->bi_status = bi_inuse;
                        rbi->bi_onShip = FALSE;
                        rbi->bi_effic = 50;
                        rbi->bi_price = 0;
                        /* get the life supp weight */
                        rbi->bi_weight = buildItemWeight(IS, rbi);
                        server(IS, rt_createBigItem, itemNumber);
                        userN2(IS, "Built life support system ",
                            itemNumber);
                        userP(IS, " on ", &savePl, ".\n");
                    }
                }
                break;

            /* they want to build an engine */
            case 1:
                IS->is_BOOL2 = TRUE;
                /* check if there is enough production */
                cost = IS->is_world.w_prodCost[pp_engine];
                if (cost > rp->pl_prod[pp_engine])
                {
                    /* The planet does not have enough production units */
                    /* for an engine */
                    err(IS, "insufficient production units");
                }
                else
                {
                    /* The planet has enough production units */
                    /* so check the players wealth */
                    if (((long)cost * IS->is_world.w_shipCostMult) >
                        IS->is_player.p_money)
                    {
                        /* The player is too poor to build an engine */
                        err(IS, "you don't have enough money");
                    }
                    else
                    {
                        /* They can afford it, so get to it */
                        /* decrease the planets production */
                        server(IS, rt_lockPlanet, pl->pl_number);
                        rp->pl_prod[pp_engine] -= cost;
                        /* increase the planets big item count */
                        rp->pl_bigItems++;
                        /* increase the planets item count */
                        rp->pl_quantity[it_engines]++;
                        /* save a copy of the planet for later */
                        savePl = *rp;
                        server(IS, rt_unlockPlanet, pl->pl_number);
                        cost *= IS->is_world.w_shipCostMult;
                        /* remove the funds from the player */
                        server(IS, rt_lockPlayer, IS->is_player.p_number);
                        rPlay->p_money -= cost;
                        server(IS, rt_unlockPlayer, IS->is_player.p_number);
                        IS->is_player.p_money = rPlay->p_money;
                        IS->is_USHORT2 += cost;
                        /* incr the item count in the world */
                        server(IS, rt_lockWorld, 0);
                        itemNumber = rw->w_bigItemNext;
                        rw->w_bigItemNext++;
                        server(IS, rt_unlockWorld, 0);
                        IS->is_world.w_bigItemNext = rw->w_bigItemNext;

                        /* now we are ready to create the new items */
                        /* fill in the big item struct */
                        rbi->bi_lastUpdate = timeNow(IS);
                        rbi->bi_itemLoc = savePl.pl_number;
                        rbi->bi_techLevel = savePl.pl_techLevel;
                        rbi->bi_number = itemNumber;
                        rbi->bi_part = bp_engines;
                        rbi->bi_status = bi_inuse;
                        rbi->bi_onShip = FALSE;
                        rbi->bi_effic = 50;
                        rbi->bi_price = 0;
                        /* get the engines weight */
                        rbi->bi_weight = buildItemWeight(IS, rbi);
                        server(IS, rt_createBigItem, itemNumber);
                        userN2(IS, "Built engine number ", itemNumber);
                        userP(IS, " on ", &savePl, ".\n");
                    }
                }
                break;

            /* they want to build a computer */
            case 0:
                IS->is_BOOL2 = TRUE;
                /* check if there is enough production */
                cost = IS->is_world.w_prodCost[pp_electronics];
                if (cost > rp->pl_prod[pp_electronics])
                {
                    /* The planet does not have enough production units */
                    /* for a computer */
                    err(IS, "insufficient production units");
                }
                else
                {
                    /* The planet has enough production units */
                    /* so check the players wealth */
                    if (((long)cost * IS->is_world.w_shipCostMult) >
                        IS->is_player.p_money)
                    {
                        /* The player is too poor to build a computer */
                        err(IS, "you don't have enough money");
                    }
                    else
                    {
                        /* They can afford it, so get to it */
                        /* decrease the planets production */
                        server(IS, rt_lockPlanet, pl->pl_number);
                        rp->pl_prod[pp_electronics] -= cost;
                        /* increase the planets big item count */
                        rp->pl_bigItems++;
                        /* increase the planets item count */
                        rp->pl_quantity[it_computers]++;
                        /* save a copy of the planet for later */
                        savePl = *rp;
                        server(IS, rt_unlockPlanet, pl->pl_number);
                        cost *= IS->is_world.w_shipCostMult;
                        /* remove the funds from the player */
                        server(IS, rt_lockPlayer, IS->is_player.p_number);
                        rPlay->p_money -= cost;
                        server(IS, rt_unlockPlayer, IS->is_player.p_number);
                        IS->is_player.p_money = rPlay->p_money;
                        IS->is_USHORT2 += cost;
                        /* incr the item count in the world */
                        server(IS, rt_lockWorld, 0);
                        itemNumber = rw->w_bigItemNext;
                        rw->w_bigItemNext++;
                        server(IS, rt_unlockWorld, 0);
                        IS->is_world.w_bigItemNext = rw->w_bigItemNext;

                        /* now we are ready to create the new items */
                        /* fill in the big item struct */
                        rbi->bi_lastUpdate = timeNow(IS);
                        rbi->bi_itemLoc = savePl.pl_number;
                        rbi->bi_techLevel = savePl.pl_techLevel;
                        rbi->bi_number = itemNumber;
                        rbi->bi_part = bp_computer;
                        rbi->bi_status = bi_inuse;
                        rbi->bi_onShip = FALSE;
                        rbi->bi_effic = 50;
                        rbi->bi_price = 0;
                        /* get the computer weight */
                        rbi->bi_weight = buildItemWeight(IS, rbi);
                        server(IS, rt_createBigItem, itemNumber);
                        userN2(IS, "Built computer number ", itemNumber);
                        userP(IS, " on ", &savePl, ".\n");
                    }
                }
                break;

            default:
                break;
        }
    }
    IS->is_textInPos = &IS->is_textIn[IS->is_USHORT1];
}

/*
 * cmd_build - The actual front-end for the build command. Asks the user
 *          what planet to build on, and prints the return value from the
 *          doBuild() function
 */

BOOL cmd_build(IMP)
{
    PlanetScan_t ps;

    if (reqPlanets(IS, &ps, "Planets to build on"))
    {
        (void) skipBlanks(IS);
        IS->is_USHORT1 = (IS->is_textInPos - &IS->is_textIn[0]) / sizeof(char);
        IS->is_BOOL1 = FALSE;
        IS->is_BOOL2 = FALSE;
        IS->is_BOOL3 = FALSE;
        IS->is_USHORT2 = 0;
        (void) scanPlanets(IS, &ps, doBuild);
        if (!IS->is_BOOL1 && !IS->is_BOOL2)
        {
            err(IS, "can't build that there");
        }
        else if (IS->is_BOOL1 && !IS->is_BOOL3)
        {
            err(IS, "insufficient money/production");
        }
        else if (IS->is_USHORT2 != 0)
        {
            userN3(IS, "That just cost you $", IS->is_USHORT2, "!\n");
        }
        /* Reset these to be safe... */
        IS->is_BOOL1 = FALSE;
        IS->is_BOOL2 = FALSE;
        IS->is_BOOL3 = FALSE;
        IS->is_USHORT2 = 0;
        return TRUE;
    }
    return FALSE;
}

/*
 * cmd_declare - allows players to specify player-player and player-race
 *          relations
 */

BOOL cmd_declare(IMP)
{
    USHORT player, what, race;
    char *ptr;

    if (reqChoice(IS, &what, "default\0neutrality\0alliance\0war\0",
                "Enter relationship (default/neutrality/alliance/war)"))
    {
        skipBlanks(IS);
        if (reqChoice(IS, &race, "player\0race\0",
            "Declare on/with (player/race)"))
        {
            skipBlanks(IS);
            switch(race)
            {
                case 0:
                    if (reqPlayer(IS, &player, "Declare with which player"))
                    {
                        if (what == r_default)
                        {
                            server(IS, rt_readPlayer, player);
                            if (IS->is_request.rq_u.ru_player.p_race !=
                                NO_RACE)
                            {
                                what = IS->is_player.p_racerel[
                                    IS->is_request.rq_u.ru_player.p_race];
                            }
                            else
                            {
                                what = r_neutral;
                            }
                        }
                        if (player == IS->is_player.p_number)
                        {
                            err(IS, "your relation to yourself is none of "
                                "Imperium's concern");
                            return FALSE;
                        }
                        if (player == 0)
                        {
                            err(IS, "Imperium does not care about your "
                                "spiritual feelings");
                            return FALSE;
                        }
                        if (IS->is_player.p_status != ps_deity)
                        {
                            if (IS->is_player.p_playrel[player] == r_war)
                            {
                                news(IS, n_disavow_war, IS->is_player.p_number,
                                    player);
                            }
                            else
                            {
                                if (IS->is_player.p_playrel[player] ==
                                    r_allied)
                                {
                                    news(IS, n_disavow_ally,
                                        IS->is_player.p_number, player);
                                }
                            }
                        }
                        IS->is_player.p_playrel[player] = what;
                        server(IS, rt_lockPlayer, IS->is_player.p_number);
                        IS->is_request.rq_u.ru_player.p_playrel[player] = what;
                        server(IS, rt_unlockPlayer, IS->is_player.p_number);
                        switch(what)
                        {
                            case r_neutral:
                                ptr = "their neutrality to you!";
                                break;
                            case r_war:
                                ptr = "WAR on you!";
                                break;
                            case r_allied:
                                ptr = "an alliance with you!";
                                break;
			    default:
				ptr = ">>> unexpected default!";
				break;
                        }
                        user3(IS, &IS->is_player.p_name[0], " declared ", ptr);
                        notify(IS, player);
                        if (IS->is_player.p_status != ps_deity)
                        {
                            switch(what)
                            {
                                case r_neutral:
                                    news(IS, n_decl_neut,
                                        IS->is_player.p_number, player);
                                    break;
                                case r_war:
                                    news(IS, n_decl_war,
                                        IS->is_player.p_number, player);
                                    break;
                                case r_allied:
                                    news(IS, n_decl_ally,
                                        IS->is_player.p_number, player);
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    break;

                /* they want to declare a race relation */
                case 1:
                    /* note this is much simpler than player relations */
                    if (reqRace(IS, &player, TRUE,
                        "Declare with which race"))
                    {
                        /* if they tried selecting default */
                        /* set it to neutral instead */
                        if (what == r_default)
                        {
                            what = r_neutral;
                        }
                        IS->is_player.p_racerel[player] = what;
                        server(IS, rt_lockPlayer, IS->is_player.p_number);
                        IS->is_request.rq_u.ru_player.p_racerel[player] = what;
                        server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    }
                    break;

                default:
                    break;
            }
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

/*
 * cmd_lend - lends money from one player to another
 */

BOOL cmd_lend(IMP)
{
    register Loan_t *rl;
    long loanNumber;
    ULONG amount, rate, duration;
    USHORT loanee;

    loanNumber = IS->is_player.p_money;
    if (loanNumber <= 0)
    {
        err(IS, "you have no money to lend");
        return TRUE;
    }
    if (reqPlayer(IS, &loanee, "Player to loan to") && doSkipBlanks(IS) &&
        reqPosRange(IS, &amount, loanNumber, "Amount to loan") &&
        doSkipBlanks(IS) && reqPosRange(IS, &rate, 127,
        "Interest over the duration (max 127%)") && doSkipBlanks(IS) &&
        reqPosRange(IS, &duration, 127, "Duration in \"days\" (max 127)"))
    {
        if (loanee == IS->is_player.p_number)
        {
            err(IS, "can't lend yourself money");
            return FALSE;
        }
        if ((amount == 0) || (duration == 0))
        {
            err(IS, "loan aborted");
            return FALSE;
        }
        server(IS, rt_lockWorld, 0);
        loanNumber = IS->is_request.rq_u.ru_world.w_loanNext;
        IS->is_request.rq_u.ru_world.w_loanNext++;
        server(IS, rt_unlockWorld, 0);
        IS->is_world.w_loanNext = IS->is_request.rq_u.ru_world.w_loanNext;
        rl = &IS->is_request.rq_u.ru_loan;
        rl->l_lastPay = timeNow(IS);
        rl->l_dueDate = rl->l_lastPay + IS->is_world.w_secondsPerITU *
            (2 * 24) * ((ULONG)duration);
        rl->l_amount = amount;
        rl->l_paid = 0;
        rl->l_duration = duration;
        rl->l_rate = rate;
        rl->l_loaner = IS->is_player.p_number;
        rl->l_loanee = loanee;
        rl->l_state = l_offered;
        server(IS, rt_createLoan, loanNumber);
        user(IS, &IS->is_player.p_name[0]);
        userN3(IS, " has offered you loan #", loanNumber, ".");
        notify(IS, loanee);
        userN3(IS, "You have offered loan #", loanNumber, ".\n");
        return TRUE;
    }
    return FALSE;
}

/*
 * cmd_accept - accepts or declines a loan offered by another player
 */

BOOL cmd_accept(IMP)
{
    Loan_t save;
    register Loan_t *rl;
    register Player_t *rp;
    USHORT choice, loaner;
    ULONG loanNumber;

    if (IS->is_world.w_loanNext == 0)
    {
        err(IS, "there are no loans yet");
        return FALSE;
    }
    if (reqPosRange(IS, &loanNumber, IS->is_world.w_loanNext - 1,
        "Loan to accept"))
    {
        server(IS, rt_readLoan, loanNumber);
        rl = &IS->is_request.rq_u.ru_loan;
        rp = &IS->is_request.rq_u.ru_player;
        if (rl->l_loanee != IS->is_player.p_number)
        {
            err(IS, "that loan is not offered to you");
            return FALSE;
        }
        if (rl->l_state != l_offered)
        {
            err(IS, "that loan is not an outstanding loan offer");
            return FALSE;
        }
        save = *rl;
        server(IS, rt_readPlayer, save.l_loaner);
        userN3(IS, "Loan #", loanNumber, " offered by ");
        user2(IS, &rp->p_name[0], " on ");
        uTime(IS, save.l_lastPay);
        userNL(IS);
        userN2(IS, "Principal $", save.l_amount);
        userN2(IS, " at ", save.l_rate);
        userN3(IS, "% interest lasting ", save.l_duration, " \"days\"\n");
        user(IS, "This loan offer will be retracted if not accepted by ");
        uTime(IS, save.l_dueDate);
        userNL(IS);
        *IS->is_textInPos = '\0';
        while (!reqChoice(IS, &choice, "accept\0decline\0postpone\0",
            "Accept/decline/postpone this loan"))
        {
        }
        switch(choice)
        {
            case 0:
                server(IS, rt_lockPlayer, save.l_loaner);
                rp->p_money -= save.l_amount;
                server(IS, rt_unlockPlayer, save.l_loaner);
                server(IS, rt_lockPlayer, IS->is_player.p_number);
                rp->p_money += save.l_amount;
                server(IS, rt_unlockPlayer, IS->is_player.p_number);
                IS->is_player.p_money = rp->p_money;
                server(IS, rt_lockLoan, loanNumber);
                rl->l_paid = 0;
                rl->l_lastPay = timeNow(IS);
                rl->l_dueDate = rl->l_lastPay +
                    IS->is_world.w_secondsPerITU * (2 * 24) *
                        ((ULONG)rl->l_duration);
                rl->l_state = l_outstanding;
                server(IS, rt_unlockLoan, loanNumber);
                loaner = rl->l_loaner;
                userN3(IS, "Loan #", loanNumber, " accepted by ");
                user(IS, &IS->is_player.p_name[0]);
                notify(IS, save.l_loaner);
                news(IS, n_make_loan, loaner, IS->is_player.p_number);
                userN3(IS, "You are now $", save.l_amount,
                    " richer (sort of)\n");
                break;
            case 1:
                server(IS, rt_lockLoan, loanNumber);
                rl->l_state = l_declined;
                server(IS, rt_unlockLoan, loanNumber);
                userN3(IS, "Loan #", loanNumber, " declined by ");
                user(IS, &IS->is_player.p_name[0]);
                notify(IS, save.l_loaner);
                userN3(IS, "You declined loan #", loanNumber, "\n");
                break;
            case 2:
                userN3(IS, "Loan #", loanNumber, " considered by ");
                user(IS, &IS->is_player.p_name[0]);
                notify(IS, save.l_loaner);
                user(IS, "Okay...\n");
                break;
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * loanValue - calculate the current value of a loan.
 */

ULONG loanValue(IMP, register Loan_t *rl)
{
    register ULONG owed, due, now;
    ULONG lastPay, rate, regularTime, extraTime;

    now = IS->is_request.rq_time / IS->is_world.w_secondsPerITU;
    due = rl->l_dueDate / IS->is_world.w_secondsPerITU;
    lastPay = rl->l_lastPay / IS->is_world.w_secondsPerITU;
    /* now, due and lastPay are now ITU values */
    rate = ((ULONG)rl->l_rate) * 1000;
    rate = rate / (((ULONG)rl->l_duration) * (24 * 2));
    /* rate is now 1000 * interest per ITU */
    if (now <= due)
    {
        regularTime = now - lastPay;
        extraTime = 0;
    }
    else
    {
        if (lastPay <= due)
        {
            regularTime = due - lastPay;
            extraTime = now - due;
        }
        else
        {
            regularTime = 0;
            extraTime = now - lastPay;
        }
    }
    owed = ((regularTime + extraTime * 2) * rate / 100 + 1000) *
            rl->l_amount / 1000;
    if (owed > 65535)
    {
        owed = 65535;
    }
    return owed;
}

/*
 * cmd_repay - allows a player to repay all or part of a loan
 */

BOOL cmd_repay(IMP)
{
    Loan_t save;
    register Loan_t *rl;
    register Player_t *rp;
    ULONG loanNumber, amount;
    register ULONG owed;

    if (IS->is_world.w_loanNext == 0)
    {
        err(IS, "there are no loans yet");
        return FALSE;
    }
    if (reqPosRange(IS, &loanNumber, IS->is_world.w_loanNext - 1,
        "Repay which loan"))
    {
        server(IS, rt_readLoan, loanNumber);
        rl = &IS->is_request.rq_u.ru_loan;
        rp = &IS->is_request.rq_u.ru_player;
        if ((rl->l_state != l_outstanding) || (rl->l_loanee !=
            IS->is_player.p_number))
        {
            err(IS, "you don't owe any money on that loan");
            return FALSE;
        }
        owed = loanValue(IS, rl);
        userN3(IS, "You owe $", owed, " on that loan.\n");
        (void) skipBlanks(IS);
        if (reqPosRange(IS, &amount, owed, "Pay how much"))
        {
            if (((long)amount) > IS->is_player.p_money)
            {
                err(IS, "you don't have that much money");
            }
            else
            {
                save = *rl;
                server(IS, rt_lockPlayer, IS->is_player.p_number);
                rp->p_money -= amount;
                server(IS, rt_unlockPlayer, IS->is_player.p_number);
                IS->is_player.p_money = rp->p_money;
                server(IS, rt_lockPlayer, save.l_loaner);
                rp->p_money += amount;
                server(IS, rt_unlockPlayer, save.l_loaner);
                server(IS, rt_lockLoan, loanNumber);
                rl->l_lastPay = timeNow(IS);
                if (amount == owed)
                {
                    rl->l_state = l_paidUp;
                    server(IS, rt_unlockLoan, loanNumber);
                    user(IS, &IS->is_player.p_name[0]);
                    userN2(IS, " paid off loan #", loanNumber);
                    userN2(IS, " with $", amount);
                    notify(IS, save.l_loaner);
                    news(IS, n_repay_loan, IS->is_player.p_number,
                        save.l_loaner);
                    user(IS, "Congratulations, you've paid off the loan!\n");
                }
                else
                {
                    owed -= amount;
                    if (owed > 65535)
                    {
                        owed = 65535;
                    }
                    rl->l_amount = owed;
                    if (amount > (65535 - rl->l_paid))
                    {
                        amount = 65535 - rl->l_paid;
                    }
                    rl->l_paid += amount;
                    server(IS, rt_unlockLoan, loanNumber);
                    user(IS, &IS->is_player.p_name[0]);
                    userN2(IS, " paid $", amount);
                    userN2(IS, " on loan #", loanNumber);
                    notify(IS, save.l_loaner);
                }
            }
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    return FALSE;
}

/*
 * dumpLoan - part of cmd_ledger. Return 'TRUE' if loan involves this user.
 */

BOOL dumpLoan(IMP, ULONG loanNumber)
{
    Loan_t save;
    register Loan_t *rl;
    register ULONG due, rate, lastPay, now;
    ULONG regularTime, extraTime, owed;

    server(IS, rt_readLoan, loanNumber);
    rl = &IS->is_request.rq_u.ru_loan;
    if (((rl->l_loaner == IS->is_player.p_number) || (rl->l_loanee ==
        IS->is_player.p_number)) && (rl->l_state != l_declined) &&
        (rl->l_state != l_paidUp))
    {
        userNL(IS);
        save = *rl;
        server(IS, rt_readPlayer, save.l_loaner);
        userN3(IS, "Loan #", loanNumber, " from ");
        user2(IS, &IS->is_request.rq_u.ru_player.p_name[0], " to ");
        server(IS, rt_readPlayer, save.l_loanee);
        user2(IS, &IS->is_request.rq_u.ru_player.p_name[0], "\n");
        if (save.l_state == l_offered)
        {
            userN2(IS, "(proposed) principal = $", save.l_amount);
            userN2(IS, " interest rate = ", save.l_rate);
            userN3(IS, "% duration (\"days\") = ", save.l_duration, "\n");
            if (save.l_dueDate < timeNow(IS))
            {
                user(IS, "This offer has expired\n");
                server(IS, rt_lockLoan, loanNumber);
                rl->l_state = l_paidUp;
                server(IS, rt_unlockLoan, loanNumber);
            }
            else
            {
                user(IS, "Loan must be accepted by ");
                uTime(IS, save.l_dueDate);
                userNL(IS);
            }
        }
        else
        {
            /* see loanValue for comments on these. We don't use loanValue
               here since we need rate and lastPay below */
            now = IS->is_request.rq_time / IS->is_world.w_secondsPerITU;
            due = save.l_dueDate / IS->is_world.w_secondsPerITU;
            lastPay = save.l_lastPay / IS->is_world.w_secondsPerITU;
            rate = ((ULONG)save.l_rate) * 1000;
            rate = rate / (((ULONG)save.l_duration) * (24 * 2));
            if (now <= due)
            {
                regularTime = now - lastPay;
                extraTime = 0;
            }
            else
            {
                if (lastPay <= due)
                {
                    regularTime = due - lastPay;
                    extraTime = now - due;
                }
                else
                {
                    regularTime = 0;
                    extraTime = now - lastPay;
                }
            }
            owed = ((regularTime + extraTime * 2) * rate / 100 + 1000) *
                    save.l_amount / 1000;
            userN3(IS, "Amount paid to date: $", save.l_paid, "\n");
            userN3(IS, "Amount due if paid now: $", owed, "\n");
            if (extraTime == 0)
            {
                userN3(IS, "(if paid on due date): ",
                        ((due - lastPay) * rate / 100 + 1000) *
                            save.l_amount / 1000, "\n");
                user(IS, "Due date is: ");
                uTime(IS, save.l_dueDate);
                userNL(IS);
            }
            else
            {
                user(IS, " ** In Arrears **\n");
            }
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*
 * cmd_ledger - displays the players ledger
 */

void cmd_ledger(IMP)
{
    ULONG loanNumber;
    BOOL gotOne;

    if (IS->is_world.w_loanNext == 0)
    {
        err(IS, "there are no loans yet");
        return;
    }
    if (*IS->is_textInPos == '\0')
    {
        user3(IS, "        ... ", &IS->is_player.p_name[0], " Ledger ...\n");
        gotOne = FALSE;
        for (loanNumber = 0; loanNumber < IS->is_world.w_loanNext;
            loanNumber++)
        {
            if (dumpLoan(IS, loanNumber))
            {
                gotOne = TRUE;
            }
        }
        if (!gotOne)
        {
            user(IS, "No entries in ledger\n");
        }
        return;
    }
    if (getPosRange(IS, &loanNumber, IS->is_world.w_loanNext - 1))
    {
        if (!dumpLoan(IS, loanNumber))
        {
            userN3(IS, "There is no entry in the ledger for loan #",
                loanNumber, "\n");
        }
    }
}

/*
 * cmd_collect - Allows one player to repossess a planet owned by another
 *          player, if the debt is large enough, or to declare the debt void
 *          and collect 80% of the original loan
 */

BOOL cmd_collect(IMP)
{
    register Loan_t *rl;
    register Planet_t *rp;
    register Player_t *rPl;
    register USHORT loanee;
    ULONG loanNumber, plNum;
    register ULONG val, owed;
    register ItemType_t it;
    BOOL killed;
    Player_t player;
    Planet_t planet;

    if (IS->is_world.w_loanNext == 0)
    {
        err(IS, "there are no loans yet");
        return FALSE;
    }
    if (reqPosRange(IS, &loanNumber, IS->is_world.w_loanNext - 1,
        "Loan to collect on"))
    {
        server(IS, rt_readLoan, loanNumber);
        rl = &IS->is_request.rq_u.ru_loan;
        rp = &IS->is_request.rq_u.ru_planet;
        rPl = &IS->is_request.rq_u.ru_player;
        if ((rl->l_state != l_outstanding) || (rl->l_loaner !=
            IS->is_player.p_number))
        {
            user(IS, "You are not owed anything on that loan\n");
            return FALSE;
        }
        if (rl->l_dueDate >= timeNow(IS))
        {
            userN2(IS, "There has been no default on loan #", loanNumber);
            userNL(IS);
            return FALSE;
        }
        loanee = rl->l_loanee;
        server(IS, rt_readPlayer, loanee);
        player = IS->is_request.rq_u.ru_player;
        server(IS, rt_readLoan, loanNumber);
        owed = loanValue(IS, rl);
        userN3(IS, "You are owed $", owed, " on that loan\n");
        (void) skipBlanks(IS);
        if (player.p_planetCount == 0)
        {
            user(IS, "That player has no planets to confiscate\n");
            if (ask(IS, "Do you wish to collect 80% of the initial "
                   "loan as bad debt"))
            {
                owed = (rl->l_amount * 8) / 10;
                server(IS, rt_lockPlayer, IS->is_player.p_number);
                rPl->p_money += owed;
                IS->is_player.p_money = rPl->p_money;
                server(IS, rt_unlockPlayer, IS->is_player.p_number);
                server(IS, rt_lockLoan, loanNumber);
                rl->l_state = l_paidUp;
                server(IS, rt_unlockLoan, loanNumber);
                userN3(IS, "You collected $", owed, " as bad debt\n");
                user(IS, "This loan is considered PAID IN FULL\n");
            }
        }
        else
        {
            if (reqPlanet(IS, &plNum, "Confiscate which planet"))
            {
                server(IS, rt_readPlanet, plNum);
                planet = *rp;
                if (rp->pl_owner != loanee)
                {
                    server(IS, rt_readPlayer, loanee);
                    user(IS, &rPl->p_name[0]);
                    userP(IS, " doesn't own planet ", &planet, "\n");
                }
                else
                {
                    server(IS, rt_lockPlanet, plNum);
                    /* get the latest version of the planet */
                    planet = *rp;
                    updatePlanet(IS);
                    val = (rp->pl_efficiency + 100);
                    switch(rp->pl_class)
                    {
                        case pc_S:
                        case pc_HOME:
                            val *= 65590;
                            break;
                        case pc_D:
                            val *= 20;
                            break;
                        case pc_M:
                            val *= 40;
                            break;
                        case pc_N:
                            val *= 30;
                            break;
                        case pc_O:
                            val *= 30;
                            break;
                        case pc_Q:
                            val *= 500;
                            break;
                        default:
                            val *= 10;
                            break;
                    }
                    val += rp->pl_minerals * 10;
                    val += rp->pl_gold * 10;
                    for (it = PPROD_FIRST; it <= PPROD_LAST; it++)
                    {
                        val += rp->pl_prod[it] * 10;
                    }
                    val += rp->pl_mobility * 10;
                    for (it = IT_FIRST; it <= IT_LAST; it++)
                    {
                        val += rp->pl_quantity[it] * 10 * getItemCost(IS, it);
                    }
                    /* add in a small percentage of the total based on size */
                    val += (val * rp->pl_size) / 100;
                    val *= IS->is_world.w_collectScale / 100;
                    userN3(IS, "That planet (and its contents) is "
                        "valued at $", val, "\n");
                    if (val <= owed)
                    {
                        /* set the last owner field */
                        rp->pl_lastOwner = rp->pl_owner;
                        /* set the new owner field */
                        rp->pl_owner = IS->is_player.p_number;
                        /* clear out any checkpoint code */
                        memset(&rp->pl_checkpoint[0], '\0', PLAN_PSWD_LEN *
                            sizeof(char));
                        /* set the transfer status to hostile */
                        rp->pl_transfer = pt_hostile;
                        /* reduce the planets efficiency slightly */
                        rp->pl_efficiency -= umin(rp->pl_efficiency, 10);
                        server(IS, rt_unlockPlanet, plNum);
                        server(IS, rt_lockPlayer, IS->is_player.p_number);
                        rPl->p_planetCount++;
                        server(IS, rt_unlockPlayer, IS->is_player.p_number);
                        IS->is_player.p_planetCount = rPl->p_planetCount;
                        server(IS, rt_lockPlayer, loanee);
                        rPl->p_planetCount--;
                        killed = FALSE;
                        if (rPl->p_planetCount == 0)
                        {
                            killed = TRUE;
                        }
                        server(IS, rt_unlockPlayer, loanee);
                        news(IS, n_sieze_planet, IS->is_player.p_number,
                            loanee);
                        if (killed)
                        {
                            user(IS, "You have just confiscated "
                                 "that player's last sector!!\n");
                        }
                        user(IS, &IS->is_player.p_name[0]);
                        userP(IS, " siezed ", &planet, "");
                        if ((val * 105 / 100 >= owed) || (val + 100 >= owed))
                        {
                            news(IS, n_repay_loan, IS->is_player.p_number,
                                loanee);
                            userN2(IS, " to satisfy loan #", loanNumber);
                            notify(IS, loanee);
                            user(IS, "That loan is now considered repaid\n");
                            server(IS, rt_lockLoan, loanNumber);
                            rl->l_state = l_paidUp;
                        }
                        else
                        {
                            userN2(IS, " in partial payment of loan #",
                                   loanNumber);
                            notify(IS, loanee);
                            owed -= val;
                            userN2(IS, "You are still owed $", owed);
                            userN3(IS, " on loan #", loanNumber, "\n");
                            server(IS, rt_lockLoan, loanNumber);
                            if (owed > 65535)
                            {
                                rl->l_amount = 65535;
                            }
                            else
                            {
                                rl->l_amount = owed;
                            }
                            val += rl->l_paid;
                            if (val > 65535)
                            {
                                rl->l_paid = 65535;
                            }
                            else
                            {
                                rl->l_paid = val;
                            }
                            rl->l_lastPay = timeNow(IS);
                        }
                        server(IS, rt_unlockLoan, loanNumber);
                    }
                    else
                    {
                        server(IS, rt_unlockPlanet, plNum);
                    }
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * cmd_donate - allows the player to donate some research or technology
 *          to their home planet
 */

BOOL cmd_donate(IMP)
{
    register Planet_t *rpl;
    USHORT what, howMuch;
    ULONG plNum;

    /* set up the pointer into the buffer */
    rpl = &IS->is_request.rq_u.ru_planet;
    /* get the planet number */
    if (reqPlanet(IS, &plNum, "Which planet do you wish to donate from"))
    {
        (void) skipBlanks(IS);
        /* read the planet in */
        server(IS, rt_readPlanet, plNum);
        /* make sure the player owns the planet */
        if (rpl->pl_owner != IS->is_player.p_number)
        {
            user(IS, "You don't own that planet\n");
        }
        /* make sure there are enough BTUs for the command */
        else if (rpl->pl_btu < 1)
        {
            user(IS, "Not enough BTUs on that planet for that command\n");
        }
        else
        {
            if (reqChoice(IS, &what, "research\0technology\0",
                "Donate what type of production (research/technology)"))
            {
                server(IS, rt_lockPlanet, plNum);
                switch (what)
                {
                    case 0:
                        howMuch = umin(rpl->pl_prod[pp_research], 127);
                        rpl->pl_prod[pp_research] -= howMuch;
                        break;
                    default:
                        howMuch = umin(rpl->pl_prod[pp_technology], 127);
                        rpl->pl_prod[pp_technology] -= howMuch;
                        break;
                }
                server(IS, rt_unlockPlanet, plNum);
                if (IS->is_player.p_race < NO_RACE)
                {
                    plNum = IS->is_world.w_race[IS->is_player.p_race].r_homePlanet;
                    server(IS, rt_lockPlanet, plNum);
                    switch (what)
                    {
                        case 0:
                            rpl->pl_prod[pp_research] += howMuch;
                            break;
                        default:
                            rpl->pl_prod[pp_technology] += howMuch;
                            break;
                    }
                    server(IS, rt_unlockPlanet, plNum);
                }
                userN3(IS, "You just donated ", howMuch, " production units "
                    "to your home planet\n");
            }
        }
        return TRUE;
    }
    return FALSE;
}
