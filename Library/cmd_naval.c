/*
 * Imperium
 *
 * $Id: cmd_naval.c,v 1.2 2000/05/18 06:50:02 marisa Exp $
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
 * $Log: cmd_naval.c,v $
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:51  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:59:13  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:24:20  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:13  marisag
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

#define CmdNavC 1
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"

static const char rcsid[] = "$Id: cmd_naval.c,v 1.2 2000/05/18 06:50:02 marisa Exp $";

/*
 * main function for ship reports
 *          Assumes that the ship IS in the buffer
 */

void doShReport(IMP, register Ship_t *sh, BigPart_t part)
{
    register BigItem_t *rbi;
    register ULONG count, iMax;
    Ship_t saveSh;
    ULONG *pArr;
    ItemType_t it;
    int InstalledGroup = 0;

    /* set these up for later */
    rbi = &IS->is_request.rq_u.ru_bigItem;
    saveSh = *sh;

    /* if not doing all items, see if we can do it in a simple manner */
    if (part != 0xff)
    {
        it = cvtBpIt(part);
        /* look for no items */
        if (sh->sh_items[it] == 0)
        {
            return;
        }
        /* see if all the items are installed, need to add up all of each */
	/* of the multi type big items ZZZ */
        /*if (saveSh.sh_items[it] == numInst(IS, &saveSh, part))*/
        switch(part)
        {
                case bp_computer:
        	    InstalledGroup = numInst(IS, &saveSh, bp_computer);
                    break;
                case bp_engines:
        	    InstalledGroup = numInst(IS, &saveSh, bp_engines);
                    break;
                case bp_lifeSupp:
        	    InstalledGroup = numInst(IS, &saveSh, bp_lifeSupp);
                    break;
                case bp_sensors:
                case bp_teleport:
                case bp_shield:
                case bp_tractor:
        	    InstalledGroup = numInst(IS, &saveSh, bp_sensors);
        	    InstalledGroup += numInst(IS, &saveSh, bp_teleport);
        	    InstalledGroup += numInst(IS, &saveSh, bp_shield);
        	    InstalledGroup += numInst(IS, &saveSh, bp_tractor);
                    break;
                case bp_photon:
                case bp_blaser:
        	    InstalledGroup = numInst(IS, &saveSh, bp_photon);
        	    InstalledGroup += numInst(IS, &saveSh, bp_blaser);
                    break;
	}
        /*if (saveSh.sh_items[it] == numInst(IS, &saveSh, part))*/
        if (saveSh.sh_items[it] == InstalledGroup)
        {
            switch(part)
            {
                case bp_computer:
                    pArr = &saveSh.sh_computer[0];
                    break;
                case bp_engines:
                    pArr = &saveSh.sh_engine[0];
                    break;
                case bp_lifeSupp:
                    pArr = &saveSh.sh_lifeSupp[0];
                    break;
                case bp_sensors:
                case bp_teleport:
                case bp_shield:
                case bp_tractor:
                    pArr = &saveSh.sh_elect[0];
                    break;
                case bp_photon:
                case bp_blaser:
                    pArr = &saveSh.sh_weapon[0];
                    break;
                default:
                    return;
            }
            /* loop through all installed items */
            iMax = saveSh.sh_items[it];
            for (count = 0; count < iMax; count++)
            {
                /* make sure array isn't screwed up */
                if (pArr[count] == NO_ITEM)
                {
                    return;
                }
                server(IS, rt_readBigItem, pArr[count]);
		if (part == rbi->bi_part) /* ZZZ */
		{
                /* read the item in */
                feShBig(IS);
                userF(IS, rbi->bi_number, 8);
                userC(IS, '|');
                userF(IS, rbi->bi_itemLoc, 8);
                userC(IS, '|');
                userF(IS, getTechFactor(rbi->bi_techLevel), 3);
                user(IS, " | ");
                userC(IS, BIG_PART_CHAR[rbi->bi_part]);
                user(IS, " |");
                userF(IS, rbi->bi_weight, 5);
                user(IS, "| ");
                userF(IS, rbi->bi_effic, 3);
                /* do some optimization here, since always true */
                user(IS, " | Yes\n");
		}
            }
            /* all done now */
            return;
        }
    }
    /* well, we got here, so either we want everything or the item count */
    /* was not the same as the number installed. Therefore we have to read */
    /* the entire big item file to find everything */
    iMax = IS->is_world.w_bigItemNext;
    if (part == 0xff)
    {
        it = 0;
    }
    else
    {
        /* Save this for later use */
        it = cvtBpIt(part);
    }
    for (count = 0; count < iMax; count++)
    {
        /* read the item in */
        server(IS, rt_readBigItem, count);
        /* never display a hull */
        if (rbi->bi_part != bp_hull)
        {
            /* see if it is the type we want to print */
            if ((part == 0xff) || (cvtBpIt(rbi->bi_part) == it))
            {
                /* make sure it is on this ship */
                if ((rbi->bi_itemLoc == saveSh.sh_number) && rbi->bi_onShip &&
                    (rbi->bi_status == bi_inuse))
                {
                    feShBig(IS);
                    userF(IS, rbi->bi_number, 8);
                    userC(IS, '|');
                    userF(IS, rbi->bi_itemLoc, 8);
                    userC(IS, '|');
                    userF(IS, getTechFactor(rbi->bi_techLevel), 3);
                    user(IS, " | ");
                    userC(IS, BIG_PART_CHAR[rbi->bi_part]);
                    user(IS, " |");
                    userF(IS, rbi->bi_weight, 5);
                    user(IS, "| ");
                    userF(IS, rbi->bi_effic, 3);
                    /* see if the item is installed or just being carried */
                    if (isInst(&saveSh, rbi->bi_part, count))
                    {
                        user(IS, " | Yes\n");
                    }
                    else
                    {
                        user(IS, " | No\n");
                    }
                }
            }
        }
    }
}

/*
 * These next reports are just dummy calls to the above function
 */

void shCompReport(IMP, ULONG shipNumber, register Ship_t *sh)
{
    doShReport(IS, sh, bp_computer);
}

void shEngReport(IMP, ULONG shipNumber, register Ship_t *sh)
{
    doShReport(IS, sh, bp_engines);
}

void shLSReport(IMP, ULONG shipNumber, register Ship_t *sh)
{
    doShReport(IS, sh, bp_lifeSupp);
}

void shSensReport(IMP, ULONG shipNumber, register Ship_t *sh)
{
    doShReport(IS, sh, bp_sensors);
}

void shTeleReport(IMP, ULONG shipNumber, register Ship_t *sh)
{
    doShReport(IS, sh, bp_teleport);
}

void shWeapReport(IMP, ULONG shipNumber, register Ship_t *sh)
{
    doShReport(IS, sh, bp_photon);
}

void shBIReport(IMP, ULONG shipNumber, register Ship_t *sh)
{
    /* for this report we pass 0xff indicating ALL big items */
    doShReport(IS, sh, 0xff);
}

/*
 * shipStatusReport - report on the status of the passed ship structure.
 */

void shipStatusReport(IMP, ULONG shipNumber, register Ship_t *sh)
{
    Ship_t saveShip;
    register USHORT col;

    saveShip = *sh;
    feShStat(IS);
    if (sh->sh_owner == IS->is_player.p_number)
    {
        server(IS, rt_lockShip, shipNumber);
        updateShip(IS);
        server(IS, rt_unlockShip, shipNumber);
        saveShip = IS->is_request.rq_u.ru_ship;
    }
    server(IS, rt_readPlayer, saveShip.sh_owner);
    userF(IS, shipNumber, 8);
    if (IS->is_request.rq_u.ru_player.p_status == ps_deity)
    {
        userC(IS, '*');
    }
    else
    {
        userC(IS, ' ');
    }
    if (saveShip.sh_course[0] != '\0')
    {
        userC(IS, 'C');
    }
    else
    {
        userC(IS, ' ');
    }
    if (saveShip.sh_plagueStage > 1)
    {
        userC(IS, '\x50');
    }
    else
    {
        userC(IS, ' ');
    }
    if (saveShip.sh_type <= ST_LAST)
    {
        userC(IS, IS->is_shipChar[saveShip.sh_type]);
    }
    else
    {
        userC(IS, '?');
    }
    user(IS, "|");

    col = saveShip.sh_col;
    userF(IS, saveShip.sh_row, 4);
    userN2(IS, ",", col);
    if (col < 10)
    {
        user(IS, "   ");
    }
    else if (col < 100)
    {
        user(IS, "  ");
    }
    else if (col < 1000)
    {
        userSp(IS);
    }
    user(IS, "|");
    userC(IS, saveShip.sh_fleet);
    user(IS, "|");
    userF(IS, saveShip.sh_efficiency, 3);
    user(IS, "%|");
    userF(IS, getShipTechFactor(IS, &saveShip, bp_engines), 3);
    user(IS, "|");
    userF(IS, saveShip.sh_energy, 5);
    user(IS, "|");
    userF(IS, saveShip.sh_fuelLeft, 5);
    user(IS, "|");
    userF(IS, saveShip.sh_armourLeft, 5);
    user(IS, "|");
    userF(IS, saveShip.sh_price, 5);
    user(IS, "|");
    if (saveShip.sh_planet != NO_ITEM)
    {
        userF(IS, saveShip.sh_planet, 8);
    }
    else
    {
        user(IS, "        ");
    }
    user(IS, "|");
    /* If the ship has a name, display it */
    if (saveShip.sh_name[0] != '\0')
    {
        user(IS, &saveShip.sh_name[0]);
    }
    userNL(IS);
}

/*
 * shipConfReport - report on the configuration of the passed ship structure.
 */

void shipConfReport(IMP, ULONG shipNumber, register Ship_t *sh)
{
    Ship_t saveShip;
    register USHORT col;

    saveShip = *sh;
    feShConf(IS);
    if (sh->sh_owner == IS->is_player.p_number)
    {
        server(IS, rt_lockShip, shipNumber);
        updateShip(IS);
        server(IS, rt_unlockShip, shipNumber);
        saveShip = IS->is_request.rq_u.ru_ship;
    }
    server(IS, rt_readPlayer, saveShip.sh_owner);
    userF(IS, shipNumber, 8);
    if (IS->is_request.rq_u.ru_player.p_status == ps_deity)
    {
        userC(IS, '*');
    }
    else
    {
        userC(IS, ' ');
    }
    if (saveShip.sh_course[0] != '\0')
    {
        userC(IS, 'C');
    }
    else
    {
        userC(IS, ' ');
    }
    if (saveShip.sh_plagueStage > 1)
    {
        userC(IS, '\x50');
    }
    else
    {
        userC(IS, ' ');
    }
    if (saveShip.sh_type <= ST_LAST)
    {
        userC(IS, IS->is_shipChar[saveShip.sh_type]);
    }
    else
    {
        userC(IS, '?');
    }
    user(IS, "|");

    col = saveShip.sh_col;
    userF(IS, saveShip.sh_row, 4);
    userN2(IS, ",", col);
    if (col < 10)
    {
        user(IS, "   ");
    }
    else if (col < 100)
    {
        user(IS, "  ");
    }
    else if (col < 1000)
    {
        userSp(IS);
    }
    user(IS, "|");
    userC(IS, saveShip.sh_fleet);
    user(IS, "|");
    userF(IS, saveShip.sh_energy, 5);
    user(IS, "|");
    userF(IS, saveShip.sh_shields, 5);
    user(IS, "|");
    userF(IS, saveShip.sh_shieldKeep, 5);
    user(IS, "| ");
    if (saveShip.sh_flags & SHF_DEFEND)
    {
        user(IS, "Y ");
    }
    else
    {
        user(IS, "N ");
    }
    if (saveShip.sh_flags & SHF_SHIELDS)
    {
        user(IS, "Y ");
    }
    else
    {
        user(IS, "N ");
    }
    if (saveShip.sh_flags & SHF_SHKEEP)
    {
        user(IS, "Y\n");
    }
    else
    {
        user(IS, "N\n");
    }
}

/*
 * shipCargoReport - report on the cargo in the passed ship structure.
 */

void shipCargoReport(IMP, ULONG shipNumber, register Ship_t *sh)
{
    Ship_t saveShip;
    register USHORT dVal;
    register ItemType_t it;

    feShCargo(IS);
    if (sh->sh_owner == IS->is_player.p_number)
    {
        server(IS, rt_lockShip, shipNumber);
        updateShip(IS);
        server(IS, rt_unlockShip, shipNumber);
        saveShip = IS->is_request.rq_u.ru_ship;
        sh = &saveShip;
    }
    server(IS, rt_readPlayer, sh->sh_owner);
    userF(IS, shipNumber, 8);
    if (IS->is_request.rq_u.ru_player.p_status == ps_deity)
    {
        userC(IS, '*');
    }
    else
    {
        userC(IS, ' ');
    }
    if (sh->sh_type <= ST_LAST)
    {
        userC(IS, IS->is_shipChar[sh->sh_type]);
    }
    else
    {
        userC(IS, '?');
    }
    userC(IS, '|');

    dVal = IS->is_world.w_shipCargoLim[sh->sh_type] - sh->sh_cargo;

    /* if there is more than will fit, scale them */
    if (dVal > 999)
    {
        if (dVal > 9999)
        {
            userF(IS, (dVal / 1000), 2);
            userC(IS, 'K');
        }
        else
        {
            userF(IS, (dVal / 100), 2);
            userC(IS, 'x');
        }
    }
    else
    {
        userF(IS, dVal, 3);
    }
    /* loop through the items */
    for (it = IT_FIRST; it <= IT_LAST; it++)
    {
        userC(IS, '|');
        /* get the current quantity of items */
        dVal = sh->sh_items[it];
        /* if there is more than will fit, scale them */
        if (dVal > 999)
        {
            if (dVal > 9999)
            {
                userF(IS, (dVal / 1000), 2);
                userC(IS, 'K');
            }
            else
            {
                userF(IS, (dVal / 100), 2);
                userC(IS, 'x');
            }
        }
        else
        {
            userF(IS, dVal, 3);
        }
    }
    userNL(IS);
}

/*
 * cmd_ships - updates and reports on a group of ships, similar to cmd_census
 */

BOOL cmd_ships(IMP)
{
    ShipScan_t shs;
    USHORT what, dashSize;
    register char *pPtr;
    void (*scanner)(IMP, ULONG shNum, register Ship_t *shipPtr) = NULL;

    /* find out what they want a report on */
    if (reqChoice(IS, &what, "cargo\0status\0computers\0engines\0"
        "lifesup\0sensors\0teleport\0weapons\0big\0config\0",
        "Report on what (cargo/config/status/comp/engine/lifesup/sens/telep/"
        "weap/big)"))
    {
        (void) skipBlanks(IS);
        if (*IS->is_textInPos == '\0')
        {
            shs.shs_shipPatternType = shp_none;
            shs.shs_cs.cs_conditionCount = 0;
        }
        else
        {
            if (!reqShips(IS, &shs, "Ships to report on"))
            {
                return FALSE;
            }
        }
        switch(what)
        {
            case 8:
            case 7:
            case 6:
            case 5:
            case 4:
            case 3:
            case 2:
                pPtr = " Item   | Ship   | TF | T | Wgt | Eff | Ins\n";
                dashSize = 43;
                break;
            case 1:
                pPtr = " Ship #    T| Row,Col |F| Eff| TF|Energ"
                    "| Fuel|Armor|Price|  Planet|Name\n";
                dashSize = 76;
                scanner = shipStatusReport;
                break;
            case 0:
                pPtr = " Ship #  T Avl|Civ|Sci|Mil|Off|Mis|Pln|"
                    "Ore|Bar|Air|FTn|Cmp|Eng|Lif|Ele|Wpn\n";
                dashSize = 74;
                scanner = shipCargoReport;
                break;
            case 9:
                pPtr = " Ship #    T| Row,Col |F|Energ| Shld|ShLev| D A K\n";
                dashSize = 50;
                scanner = shipConfReport;
                break;
            default:
                pPtr = ">>> unexpected default\n";
                dashSize = 43;
                scanner = shipStatusReport;
                break;
        }
        /* this next bit of ugly lets use attach the stub routines */
        /* but only enter the strings constants above once */
        switch(what)
        {
            case 8:
                scanner = shBIReport;
                break;
            case 7:
                scanner = shWeapReport;
                break;
            case 6:
                scanner = shTeleReport;
                break;
            case 5:
                scanner = shSensReport;
                break;
            case 4:
                scanner = shLSReport;
                break;
            case 3:
                scanner = shEngReport;
                break;
            case 2:
                scanner = shCompReport;
                break;
            default:
                break;
        }
        user(IS, pPtr);
        dash(IS, dashSize);
        if (scanShips(IS, &shs, scanner) == 0)
        {
            err(IS, "no ships matched");
        }
        else
        {
            userNL(IS);
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * doSmallLoad - does the actual user querry about the quantity to load (if
 *          needed), and loads the items onto the ship
 *          Convention: the planet and ship are in the request as a pair.
 */

void doSmallLoad(IMP, ItemType_t what, USHORT percent, BOOL askQuan)
{
    register PlanetShipPair_t *rp;
    register USHORT capacity, present, loaded;
    ULONG plNum, shipNumber;
    ULONG amount, maxLoad, amountOrig;
    char buf[256];

    rp = &IS->is_request.rq_u.ru_planetShipPair;
    plNum = rp->p_p.pl_number;
    shipNumber = rp->p_sh.sh_number;
    capacity = getShipCapacity(IS, what, &rp->p_sh);
    present = readPlQuan(IS, &rp->p_p, what);
    /* make sure the player's race is valid */
    if (IS->is_player.p_race < NO_RACE)
    {
        /* see if this is their home planet */
        if (rp->p_p.pl_number == IS->is_world.w_race[IS->is_player.p_race
            ].r_homePlanet)
        {
            /* see if they are trying to load restricted items */
            if ((what == it_civilians) || (what == it_scientists) ||
                (what == it_military) || (what == it_officers))
            {
                /* set the limit on how many they may load */
                if (present > 500)
                {
                    present = umin(present - 500, (USHORT) 500);
                }
                else
                {
                    /* if less than 500, they can't load any */
                    present = 0;
                }
            }
        }
    }
    loaded = rp->p_sh.sh_items[what];
    if ((capacity == 0) && (present != 0) && askQuan)
    {
        user3(IS, getShipName(rp->p_sh.sh_type), " can't carry ",
              getItemName(what));
        userNL(IS);
    }
    else if ((present == 0) && (capacity != 0) && (capacity != loaded) &&
        askQuan)
    {
        user3(IS, "No ", getItemName(what), " here to load\n");
    }
    else if ((present != 0) && (capacity == loaded) && askQuan)
    {
        user3(IS, "No room for more ", getItemName(what), "\n");
    }
    else if ((capacity != 0) && (present != 0) && (capacity != loaded))
    {
        maxLoad = umin(capacity - loaded, present);
        if (askQuan)
        {
            *IS->is_textInPos = '\0';
            userN(IS, present);
            user2(IS, " ", getItemName(what));
            userN2(IS, " here, ", loaded);
            userN3(IS, " on board, load how many (max ", maxLoad, ")");
            getPrompt(IS, &buf[0]);
            if (!reqPosRange(IS, &amount, maxLoad, &buf[0]))
            {
                amount = 0;
            }
        }
        else
        {
            amount = umin((USHORT) (capacity * percent) / 100, maxLoad);
            userN3(IS, "Loading ", amount, " ");
            user2(IS, getItemName(what), ".\n");
        }
        if (amount != 0)
        {
            amountOrig = amount;
            server2(IS, rt_lockPlanetShipPair, plNum, shipNumber);
            present = readPlQuan(IS, &rp->p_p, what);
            if (amount > present)
            {
                amount = present;
            }
            loaded = rp->p_sh.sh_items[what];
            if (amount > capacity - loaded)
            {
                amount = capacity - loaded;
            }
            writePlQuan(IS, &rp->p_p, what, present - amount);
	    if ((rp->p_p.pl_transfer == pt_hostile) &&
		((what == it_civilians) || (what == it_scientists)))
	    {
		amount -= (amount / 20);
	    }
            rp->p_sh.sh_items[what] += amount;
            rp->p_sh.sh_cargo += (amount * IS->is_world.w_weight[what]);
            /* if the planet is infected, infect the ship */
            if (((rp->p_p.pl_plagueStage == 2) || (rp->p_p.pl_plagueStage
                == 3)) && (rp->p_sh.sh_plagueStage == 0))
            {
                rp->p_sh.sh_plagueStage = 1;
                rp->p_sh.sh_plagueTime = impRandom(IS,
                    IS->is_world.w_plagueOneRand) +
                    IS->is_world.w_plagueOneBase;
            }
            server2(IS, rt_unlockPlanetShipPair, plNum, shipNumber);
            if (amount != amountOrig)
            {
		if ((rp->p_p.pl_transfer == pt_hostile) &&
		    ((what == it_civilians) || (what == it_scientists)))
		{
            	    userN(IS, amountOrig - amount);
		    user3(IS, " ", getItemName(what), " had to be shot for "
			"resisting transfer.\n");
		}
		else
		{
            	    userN3(IS, "Events have reduced amount to ", amount,
                	" units.\n");
		}
            }
        }
    }
}

/*
 * loadSmallItems - allows a player to load small items onto a ship which is
 *          docked on a planet
 */

BOOL loadSmallItems(IMP)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    register PlanetShipPair_t *rpp;
    Ship_t saveShip;
    Planet_t savePl;
    USHORT percent=0, owner; /* prevent GCC complaints */
    ULONG shipNumber, plNum;
    ItemType_t it;
    BOOL askQuan, aborting;

    /* get the ship number */
    if (reqShip(IS, &shipNumber, "Ship to load"))
    {
        /* set up the pointers */
        rsh = &IS->is_request.rq_u.ru_ship;
        rpl = &IS->is_request.rq_u.ru_planet;
        rpp = &IS->is_request.rq_u.ru_planetShipPair;
        (void) skipBlanks(IS);
        aborting = FALSE;
        askQuan = FALSE;

        /* if they have anything left on the command line, assume it is a */
        /* percent to be loaded */
        if (*IS->is_textInPos == '\0')
        {
            askQuan = TRUE;
        }
        else
        {
            if (getNumber(IS, &plNum) && (plNum > 0))
            {
                percent = plNum;
            }
            else
            {
                err(IS, "load cancelled");
                aborting = TRUE;
            }
        }
        /* abort if they entered an invalid number to load */
        if (!aborting)
        {
            /* read the ship in */
            server(IS, rt_readShip, shipNumber);
            /* make sure they own it */
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else if (rsh->sh_type == st_m)
            {
                err(IS, "you can't load anything onto a miner");
            }
            else
            {
                /* make sure the ship is on a planet */
                if (rsh->sh_planet == NO_ITEM)
                {
                    err(IS, "ship is not on a planet");
                    aborting = TRUE;
                }
                else
                {
                    /* update the ship */
                    server(IS, rt_lockShip, shipNumber);
                    updateShip(IS);
                    server(IS, rt_unlockShip, shipNumber);
                    /* store the updated ship */
                    saveShip = *rsh;
                    /* find out which planet in the sector we are on */
                    plNum = whichPlanet(IS, saveShip.sh_row, saveShip.sh_col);
                    if (plNum != NO_ITEM)
                    {
                        /* update the planet, if needed */
                        accessPlanet(IS, plNum);
                        /* save the planet for later use */
                        savePl = *rpl;
                        /* verify that the player owns the planet or the */
                        /* planet is unowned */
                        if (rpl->pl_owner != IS->is_player.p_number)
                        {
                            if (rpl->pl_owner != NO_OWNER)
                            {
                                /* someone else owns it, so look for a */
                                /* checkpoint code */
                                if (rpl->pl_checkpoint[0] == '\0')
                                {
                                    userP(IS, "Planet ", rpl, " is owned by "
                                        "someone else\n");
                                    aborting = TRUE;
                                }
                                else
                                {
                                    /* there is a code, so see if the player */
                                    /* knows it */
                                    if (!verifyCheckPoint(IS, rpl,
                                        cpt_access))
                                    {
                                        /* they didn't */
                                        aborting = TRUE;
                                    }
                                }
                            }
                            else
                            {
                                /* see if this planet is a "home" planet */
                                if (isHomePlanet(IS, rpl->pl_number) !=
                                    NO_RACE)
                                {
                                    if (IS->is_player.p_race !=
                                        rpl->pl_ownRace)
                                    {
                                        err(IS, "that planet is the home "
                                            "planet of another race");
                                        aborting = TRUE;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        /* somehow the ship thinks it is on one planet, */
                        /* when the planet is located somewhere else    */
                        err(IS, "invalid planet in loadSmallItem");
                        aborting = TRUE;
                    }
                }
                /* is everything still hunky dorey? */
                if (!aborting)
                {
                    /* fill in the planetShipPair struct */
                    rpp->p_sh = saveShip;
                    rpp->p_p = savePl;
                    /* loop through the items, loading them up */
                    for (it = IT_FIRST; it <= IT_LAST_SMALL; it++)
                    {
                        doSmallLoad(IS, it, percent, askQuan);
                    }
                    /* see if we evacuated the planet */
                    owner = rpp->p_p.pl_owner;
                    if ((owner != NO_OWNER) &&
                        (readPlQuan(IS, &rpp->p_p, it_civilians) == 0) &&
                        (readPlQuan(IS, &rpp->p_p, it_scientists) == 0) &&
                        (readPlQuan(IS, &rpp->p_p, it_military) == 0) &&
                        (readPlQuan(IS, &rpp->p_p, it_officers) == 0))
                    {
                        /* remove planet from players & players race count */
                        /* and remove planet ownership */
                        abandonPlanet(IS, &rpp->p_p);
                    }
                    fePlDirty(IS, plNum);
                    feShDirty(IS, shipNumber);
                }
            }
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

/*
 * loadBigItems - allows the player to load up big items (does not
 *          install them, they are loaded as cargo)
 */

BOOL loadBigItems(IMP)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    register BigItem_t *rbi;
    ULONG shipNumber, plNum=NO_ITEM, biNum; /* prevent GCC complaints */
    Ship_t saveShip;
    Planet_t savePl;
    USHORT weight=0; /* prevent GCC complaints */
    BOOL aborting, plagued;
    ItemType_t what=0; /* prevent GCC complaints */

    /* get the number of the ship */
    if (reqShip(IS, &shipNumber, "Ship to load"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rpl = &IS->is_request.rq_u.ru_planet;
        rbi = &IS->is_request.rq_u.ru_bigItem;
        plagued = FALSE;
        (void) skipBlanks(IS);
        /* get the number of the item to load */
        if (reqBigItem(IS, &biNum, "Big item to load"))
        {
            aborting = FALSE;
            server(IS, rt_readShip, shipNumber);
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else if (rsh->sh_type == st_m)
            {
                err(IS, "you can't load anything onto a miner");
            }
            else
            {
                /* make sure the ship is on a planet */
                if (rsh->sh_planet == NO_ITEM)
                {
                    err(IS, "ship is not on a planet");
                    aborting = TRUE;
                }
                else
                {
                    server(IS, rt_lockShip, shipNumber);
                    updateShip(IS);
                    server(IS, rt_unlockShip, shipNumber);
                    saveShip = *rsh;
                    /* find out which planet in the sector we are on */
                    plNum = whichPlanet(IS, saveShip.sh_row, saveShip.sh_col);
                    if (plNum != NO_ITEM)
                    {
                        /* update the planet, if needed */
                        accessPlanet(IS, plNum);
                        savePl = *rpl;
                        /* is the planet infected */
                        if ((rpl->pl_plagueStage == 2) || (rpl->pl_plagueStage
                            == 3))
                        {
                            /* yup */
                            plagued = TRUE;
                        }
                        /* does the player own the planet? */
                        if (rpl->pl_owner != IS->is_player.p_number)
                        {
                            /* no. So is the planet owned ? */
                            if (rpl->pl_owner != NO_OWNER)
                            {
                                /* yes it is, so look for a checkpoint */
                                if (rpl->pl_checkpoint[0] == '\0')
                                {
                                    /* no checkpoint */
                                    userP(IS, "Planet ", rpl, " is owned by "
                                        "someone else\n");
                                    aborting = TRUE;
                                }
                                else
                                {
                                    /* see if the player knows it */
                                    if (!verifyCheckPoint(IS, rpl,
                                        cpt_access))
                                    {
                                        aborting = TRUE;
                                    }
                                }
                            }
                            else
                            {
                                /* see if this planet is a "home" planet */
                                if (isHomePlanet(IS, rpl->pl_number) !=
                                    NO_RACE)
                                {
                                    if (IS->is_player.p_race !=
                                        rpl->pl_ownRace)
                                    {
                                        err(IS, "that planet is the home "
                                            "planet of another race");
                                        aborting = TRUE;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        err(IS, "invalid planet in loadBigtem");
                        aborting = TRUE;
                    }
                }
                /* verify that the big item is there too */
                if (!aborting)
                {
                    server(IS, rt_readBigItem, biNum);
                    if (rbi->bi_status == bi_forSale)
                    {
                        err(IS, "item is currently offered for sale");
                        aborting = TRUE;
                    }
                    else if (rbi->bi_status == bi_destroyed)
                    {
                        err(IS, "item no longer exists");
                        aborting = TRUE;
                    }
                    else if ((rbi->bi_itemLoc != plNum) || rbi->bi_onShip)
                    {
                        err(IS, "big item is not on the same planet as the "
                            "requested ship");
                        aborting = TRUE;
                    }
                    else if (rbi->bi_price > 0)
                    {
                        err(IS, "item is currently offered for sale");
                        aborting = TRUE;
                    }
                    else
                    {
                        server(IS, rt_lockBigItem, biNum);
                        updateBigItem(IS);
                        what = cvtBpIt(rbi->bi_part);
                        weight = rbi->bi_weight;
                        server(IS, rt_unlockBigItem, biNum);
                        if (weight > (IS->is_world.w_shipCargoLim[
                            saveShip.sh_type] - saveShip.sh_cargo))
                        {
                            err(IS, "insufficient room on the ship for item");
                            aborting = TRUE;
                        }
                    }
                }
                /* if not aborting, do the actual transfer */
                if (!aborting)
                {
                    /* do the item first, since it is most likely to */
                    /* be changed by an outside force (no ownership) */
                    server(IS, rt_lockBigItem, biNum);
                    rbi->bi_itemLoc = shipNumber;
                    rbi->bi_onShip = TRUE;
                    server(IS, rt_unlockBigItem, biNum);

                    /* next change the planet so that the counts will be */
                    /* correct as soon as possible */
                    server(IS, rt_lockPlanet, plNum);
                    rpl->pl_quantity[what]--;
                    server(IS, rt_unlockPlanet, plNum);

                    /* lastly put the item on the ship */
                    server(IS, rt_lockShip, shipNumber);
                    rsh->sh_items[what]++;
                    rsh->sh_cargo += weight;
                    /* if the planet is infected, infect the ship */
                    if (plagued && (rsh->sh_plagueStage == 0))
                    {
                        rsh->sh_plagueStage = 1;
                        rsh->sh_plagueTime = impRandom(IS,
                            IS->is_world.w_plagueOneRand) +
                            IS->is_world.w_plagueOneBase;
                    }
                    server(IS, rt_unlockShip, shipNumber);
                    user(IS, "Item loaded into the cargo hold\n");
                    fePlDirty(IS, plNum);
                    feShDirty(IS, shipNumber);
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * incMiner - increments a ship's miner count. Assumes the ship is locked
 */

void incMiner(IMP)
{
    ULONG count;

    /* get the count */
    count = (ULONG)((IS->is_request.rq_u.ru_ship.sh_flags &
        SHF_CARRY_MINE) >> 24);
    /* increment the count */
    count++;
    /* remove the current count */
    IS->is_request.rq_u.ru_ship.sh_flags =
        (IS->is_request.rq_u.ru_ship.sh_flags & ~SHF_CARRY_MINE);
    /* put the new count back */
    count = (count<<24);
    IS->is_request.rq_u.ru_ship.sh_flags =
        (IS->is_request.rq_u.ru_ship.sh_flags | count);
}

/*
 * loadMiner - allows the player to load a miner onto a ship
 */

BOOL loadMiner(IMP)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    ULONG shipNumber, plNum=NO_ITEM, mineNum; /* prevent GCC complaint */
    Ship_t saveShip;
    Planet_t savePl;
    USHORT weight=0; /* prevent GCC complaint */
    BOOL aborting, plagued, saveNoWrite;

    /* get the number of the ship */
    if (reqShip(IS, &shipNumber, "Ship to load miner onto"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rpl = &IS->is_request.rq_u.ru_planet;
        plagued = FALSE;
        (void) skipBlanks(IS);
        /* get the number of the miner to load */
        if (reqShip(IS, &mineNum, "Miner to load"))
        {
            aborting = FALSE;
            server(IS, rt_readShip, shipNumber);
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else if (rsh->sh_type == st_m)
            {
                err(IS, "you can't load anything onto a miner");
            }
            else
            {
                /* make sure the ship is on a planet */
                if (rsh->sh_planet == NO_ITEM)
                {
                    err(IS, "ship is not on a planet");
                    aborting = TRUE;
                }
                else
                {
                    server(IS, rt_lockShip, shipNumber);
                    updateShip(IS);
                    server(IS, rt_unlockShip, shipNumber);
                    saveShip = *rsh;
                    /* find out which planet in the sector we are on */
                    plNum = whichPlanet(IS, saveShip.sh_row, saveShip.sh_col);
                    if (plNum != NO_ITEM)
                    {
                        /* update the planet, if needed */
                        accessPlanet(IS, plNum);
                        savePl = *rpl;
                        /* is the planet infected */
                        if ((rpl->pl_plagueStage == 2) ||
                            (rpl->pl_plagueStage == 3))
                        {
                            /* yup */
                            plagued = TRUE;
                        }
                        /* does the player own the planet? */
                        if (rpl->pl_owner != IS->is_player.p_number)
                        {
                            /* no. So is the planet owned ? */
                            if (rpl->pl_owner != NO_OWNER)
                            {
                                /* yes it is, so look for a checkpoint */
                                if (rpl->pl_checkpoint[0] == '\0')
                                {
                                    /* no checkpoint */
                                    userP(IS, "Planet ", rpl, " is owned by "
                                        "someone else\n");
                                    aborting = TRUE;
                                }
                                else
                                {
                                    /* see if the player knows it */
                                    if (!verifyCheckPoint(IS, rpl,
                                        cpt_access))
                                    {
                                        aborting = TRUE;
                                    }
                                }
                            }
                            else
                            {
                                /* see if this planet is a "home" planet */
                                if (isHomePlanet(IS, rpl->pl_number) !=
                                    NO_RACE)
                                {
                                    if (IS->is_player.p_race !=
                                        rpl->pl_ownRace)
                                    {
                                        err(IS, "that planet is the home "
                                            "planet of another race");
                                        aborting = TRUE;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        err(IS, "invalid planet in loadMiner");
                        aborting = TRUE;
                    }
                }
                /* verify that the miner is there too */
                if (!aborting)
                {
                    server(IS, rt_readShip, mineNum);
                    if (rsh->sh_type != st_m)
                    {
                        err(IS, "the requested ship is not a miner");
                        aborting = TRUE;
                    }
                    else if (rsh->sh_price != 0)
                    {
                        err(IS, "miner is currently offered for sale");
                        aborting = TRUE;
                    }
                    else if (rsh->sh_owner == NO_OWNER)
                    {
                        err(IS, "miner no longer exists");
                        aborting = TRUE;
                    }
                    else if ((rsh->sh_planet != plNum) && (rsh->sh_planet !=
                        NO_ITEM))
                    {
                        err(IS, "miner is not on the same planet as the "
                            "requested ship");
                        aborting = TRUE;
                    }
                    else if (rsh->sh_dragger != NO_ITEM)
                    {
                        err(IS, "miner is already loaded onto a ship");
                        aborting = TRUE;
                    }
                    else
                    {
                        server(IS, rt_lockShip, mineNum);
                        updateMiner(IS);
                        weight = rsh->sh_cargo +
                            IS->is_world.w_shipCargoLim[st_m];
                        server(IS, rt_unlockShip, mineNum);
                        if (weight > (IS->is_world.w_shipCargoLim[
                            saveShip.sh_type] - saveShip.sh_cargo))
                        {
                            err(IS, "insufficient room on the ship for miner");
                            aborting = TRUE;
                        }
                    }
                }
                /* if not aborting, do the actual transfer */
                if (!aborting)
                {
                    /* do the item first, since it is most likely to */
                    /* be changed by an outside force (no ownership) */
                    server(IS, rt_lockShip, mineNum);
                    rsh->sh_dragger = shipNumber;
                    rsh->sh_planet = NO_ITEM;
                    rsh->sh_elect[0] = 0;
                    rsh->sh_elect[1] = 0;
                    if (rsh->sh_owner != IS->is_player.p_number)
                    {
                        saveNoWrite = IS->is_noWrite;
                        IS->is_noWrite = TRUE;
                        user2(IS, &IS->is_player.p_name[0], " just loaded "
                            "your miner #");
                        userN(IS, mineNum);
                        userN3(IS, ", located on planet #", plNum,
                            " onto ship #");
                        userN(IS, shipNumber);
                        user(IS, "!");
                        notify(IS, rsh->sh_owner);
                        IS->is_noWrite = saveNoWrite;
                        rsh->sh_owner = IS->is_player.p_number;
                    }
                    server(IS, rt_unlockShip, mineNum);

                    /* lastly put the miner on the ship */
                    server(IS, rt_lockShip, shipNumber);
                    rsh->sh_cargo += weight;
                    /* if the planet is infected, infect the ship */
                    if (plagued && (rsh->sh_plagueStage == 0))
                    {
                        rsh->sh_plagueStage = 1;
                        rsh->sh_plagueTime = impRandom(IS,
                            IS->is_world.w_plagueOneRand) +
                            IS->is_world.w_plagueOneBase;
                    }
                    /* increment the ship's miner count */
                    incMiner(IS);
                    server(IS, rt_unlockShip, shipNumber);
                    decrPlShipCount(IS, plNum);
                    user(IS, "Miner loaded into the cargo hold\n");
                    fePlDirty(IS, plNum);
                    feShDirty(IS, shipNumber);
                    feShDirty(IS, mineNum);
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * cmd_load - allows a player to load big or small items onto a ship
 */

BOOL cmd_load(IMP)
{
    USHORT what;

    if (reqChoice(IS, &what, "big\0small\0miner\0",
        "Load (big/small/miner) items"))
    {
        (void) skipBlanks(IS);
        switch (what)
        {
            case 0:
                return loadBigItems(IS);
                break;
            case 1:
                return loadSmallItems(IS);
                break;
            case 2:
                return loadMiner(IS);
                break;
        }
    }
    return FALSE;
}

/*
 * doUnload - does the actual item unloading for unloadSmallItems()
 *          Assumes that the planet and ship are in the request as a pair
 */

void doUnload(IMP, ItemType_t what, USHORT percent, BOOL askQuan)
{
    register PlanetShipPair_t *rp;
    register USHORT capacity, present, loaded;
    ULONG plNum, shipNumber;
    ULONG amount, maxUnload, amountOrig;
    char buf[256];

    rp = &IS->is_request.rq_u.ru_planetShipPair;
    plNum = rp->p_p.pl_number;
    shipNumber = rp->p_sh.sh_number;
    capacity = MAX_WORK;
    present = readPlQuan(IS, &rp->p_p, what);
    loaded = rp->p_sh.sh_items[what];
    if ((capacity == present) && (loaded != 0))
    {
        user3(IS, "No room to unload ", getItemName(what), "\n");
    }
    else if ((loaded == 0) &&
        (getShipCapacity(IS, what, &rp->p_sh) != 0))
    {
        user3(IS, "No ", getItemName(what), " on board\n");
    }
    else if ((loaded != 0) && (capacity != present))
    {
        maxUnload = umin(capacity - present, loaded);
        if (askQuan)
        {
            *IS->is_textInPos = '\0';
            userN(IS, present);
            user2(IS, " ", getItemName(what));
            userN2(IS, " here, ", loaded);
            userN3(IS, " on board, unload how many (max ", maxUnload, ")");
            getPrompt(IS, &buf[0]);
            if (!reqPosRange(IS, &amount, maxUnload, &buf[0]))
            {
                amount = 0;
            }
        }
        else
        {
            amount = umin((USHORT) (loaded * percent) / 100, maxUnload);
            userN3(IS, "Unloading ", amount, " ");
            user2(IS, getItemName(what), ".\n");
        }
        if (amount != 0)
        {
            amountOrig = amount;
            server2(IS, rt_lockPlanetShipPair, plNum, shipNumber);
            present = readPlQuan(IS, &rp->p_p, what);
            loaded = rp->p_sh.sh_items[what];
            if (amount > loaded)
            {
                amount = loaded;
            }
            if (amount > capacity - present)
            {
                amount = capacity - present;
            }
            rp->p_sh.sh_items[what] -= amount;
            rp->p_sh.sh_cargo -= (amount * IS->is_world.w_weight[what]);
	    if ((rp->p_p.pl_transfer == pt_hostile) &&
		((what == it_civilians) || (what == it_scientists)))
	    {
		amount -= (amount / 20);
	    }
            writePlQuan(IS, &rp->p_p, what, present + amount);
            adjustForNewWorkers(IS, &rp->p_p, what, present);
            /* if the ship is infected, infect the planet */
            if (((rp->p_sh.sh_plagueStage == 2) || (rp->p_sh.sh_plagueStage
                == 3)) && (rp->p_p.pl_plagueStage == 0))
            {
                rp->p_p.pl_plagueStage = 1;
                rp->p_p.pl_plagueTime = impRandom(IS,
                    IS->is_world.w_plagueOneRand) +
                    IS->is_world.w_plagueOneBase;
            }
            server2(IS, rt_unlockPlanetShipPair, plNum, shipNumber);
            if (amount != amountOrig)
            {
		if ((rp->p_p.pl_transfer == pt_hostile) &&
		    ((what == it_civilians) || (what == it_scientists)))
		{
            	    userN(IS, amountOrig - amount);
		    user3(IS, " ", getItemName(what), " were shot by "
			"natives.\n");
		}
		else
		{
                    userN3(IS, "Actions reduced the amount to ", amount,
                	" units\n");
		}
            }
        }
    }
}

/*
 * unloadSmallItems - allows a player to unload small items from a ship
 */

BOOL unloadSmallItems(IMP)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    register PlanetShipPair_t *rp;
    Ship_t saveShip;
    Planet_t savePl;
    register USHORT percent=0; /* prevent GCC complaint */
    ULONG shipNumber, plNum;
    ItemType_t it;
    BOOL askQuan, aborting, wasDeserted;

    if (reqShip(IS, &shipNumber, "Ship to unload"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rpl = &IS->is_request.rq_u.ru_planet;
        rp = &IS->is_request.rq_u.ru_planetShipPair;
        (void) skipBlanks(IS);
        aborting = FALSE;
        askQuan = FALSE;
        if (*IS->is_textInPos == '\0')
        {
            askQuan = TRUE;
        }
        else if (getNumber(IS, &plNum) && (plNum > 0))
        {
            percent = plNum;
        }
        else
        {
            err(IS, "unload cancelled");
            aborting = TRUE;
        }
        if (!aborting)
        {
            server(IS, rt_readShip, shipNumber);
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else if (rsh->sh_type == st_m)
            {
                err(IS, "you can't unload anything from a miner");
            }
            else
            {
                /* make sure the ship is on a planet */
                if (rsh->sh_planet == NO_ITEM)
                {
                    err(IS, "ship is not on a planet");
                    aborting = TRUE;
                }
                else
                {
                    server(IS, rt_lockShip, shipNumber);
                    updateShip(IS);
                    server(IS, rt_unlockShip, shipNumber);
                    saveShip = *rsh;
                    /* find out which planet in the sector we are on */
                    plNum = whichPlanet(IS, saveShip.sh_row, saveShip.sh_col);
                    if (plNum != NO_ITEM)
                    {
                        /* update the planet, if needed */
                        accessPlanet(IS, plNum);
                        savePl = *rpl;
                        if ((rpl->pl_owner != IS->is_player.p_number) &&
                            (rpl->pl_owner != NO_OWNER))
                        {
                            if (rpl->pl_checkpoint[0] == '\0')
                            {
                                userP(IS, "Planet ", rpl, " is owned by "
                                    "someone else\n");
                                aborting = TRUE;
                            }
                            else
                            {
                                if (!verifyCheckPoint(IS, rpl, cpt_access))
                                {
                                    aborting = TRUE;
                                }
                            }
                        }
                    }
                    else
                    {
                        err(IS, "invalid planet in cmd_unload");
                        aborting = TRUE;
                    }
                }
                if (!aborting)
                {
                    wasDeserted =
                        ((readPlQuan(IS, &savePl, it_civilians) == 0) &&
                        (readPlQuan(IS, &savePl, it_scientists) == 0) &&
                        (readPlQuan(IS, &savePl, it_military) == 0) &&
                        (readPlQuan(IS, &savePl, it_officers) == 0));
                    rp->p_p = savePl;
                    rp->p_sh = saveShip;
                    for (it = IT_FIRST; it <= IT_LAST_SMALL; it++)
                    {
                        doUnload(IS, it, percent, askQuan);
                    }
                    if (wasDeserted &&
                        ((readPlQuan(IS, &rp->p_p, it_civilians) != 0) ||
                        (readPlQuan(IS, &rp->p_p, it_scientists) != 0) ||
                        (readPlQuan(IS, &rp->p_p, it_military) != 0) ||
                        (readPlQuan(IS, &rp->p_p, it_officers) != 0)))
                    {
                        /* add planet to players & players race count */
                        /* and set planet ownership */
                        takeNewPlanet(IS, &rp->p_p);
                    }
                    fePlDirty(IS, plNum);
                    feShDirty(IS, shipNumber);
                }
            }
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

/*
 * unloadBigItems - allows the player to unload big items from the ship
 */

BOOL unloadBigItems(IMP)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    register BigItem_t *rbi;
    ULONG shipNumber, plNum=NO_ITEM, biNum; /* prevent GCC complaint */
    Ship_t saveShip;
    Planet_t savePl;
    USHORT weight=0; /* prevent GCC complaint */
    BOOL aborting, plagued;
    BigPart_t what=0; /* prevent GCC complaint */

    /* get the number of the ship */
    if (reqShip(IS, &shipNumber, "Ship to unload"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rpl = &IS->is_request.rq_u.ru_planet;
        rbi = &IS->is_request.rq_u.ru_bigItem;
        plagued = FALSE;
        (void) skipBlanks(IS);
        /* get the number of the item to unload */
        if (reqBigItem(IS, &biNum, "Big item to unload"))
        {
            aborting = FALSE;
            server(IS, rt_readShip, shipNumber);
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else if (rsh->sh_type == st_m)
            {
                err(IS, "you can't unload anything from a miner");
            }
            else
            {
                /* make sure the ship is on a planet */
                if (rsh->sh_planet == NO_ITEM)
                {
                    err(IS, "ship is not on a planet");
                    aborting = TRUE;
                }
                else
                {
                    server(IS, rt_lockShip, shipNumber);
                    updateShip(IS);
                    server(IS, rt_unlockShip, shipNumber);
                    saveShip = *rsh;
                    /* is the ship infected */
                    if ((rsh->sh_plagueStage == 2) || (rsh->sh_plagueStage
                        == 3))
                    {
                        /* yup */
                        plagued = TRUE;
                    }
                    /* find out which planet in the sector we are on */
                    plNum = whichPlanet(IS, saveShip.sh_row, saveShip.sh_col);
                    if (plNum != NO_ITEM)
                    {
                        /* update the planet, if needed */
                        accessPlanet(IS, plNum);
                        savePl = *rpl;
                        /* does the player own the planet? */
                        if (rpl->pl_owner != IS->is_player.p_number)
                        {
                            /* no. So is the planet owned ? */
                            if (rpl->pl_owner != NO_OWNER)
                            {
                                /* yes it is, so look for a checkpoint */
                                if (rpl->pl_checkpoint[0] == '\0')
                                {
                                    /* no checkpoint */
                                    userP(IS, "Planet ", rpl, " is owned by "
                                        "someone else\n");
                                    aborting = TRUE;
                                }
                                else
                                {
                                    /* see if the player knows it */
                                    if (!verifyCheckPoint(IS, rpl,
                                        cpt_access))
                                    {
                                        aborting = TRUE;
                                    }
                                }
                            }
                            else
                            {
                                /* see if this planet is a "home" planet */
                                if (isHomePlanet(IS, rpl->pl_number) !=
                                    NO_RACE)
                                {
                                    if (IS->is_player.p_race !=
                                        rpl->pl_ownRace)
                                    {
                                        err(IS, "that planet is the home "
                                            "planet of another race");
                                        aborting = TRUE;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        err(IS, "invalid planet in unloadBigtem");
                        aborting = TRUE;
                    }
                }
                /* verify that the big item is on the ship */
                if (!aborting)
                {
                    server(IS, rt_readBigItem, biNum);
                    if ((rbi->bi_itemLoc != shipNumber) || !rbi->bi_onShip ||
                        (rbi->bi_status != bi_inuse))
                    {
                        err(IS, "item is not on the requested ship");
                        aborting = TRUE;
                    }
                    else if (rbi->bi_price > 0)
                    {
                        err(IS, "item is currently for sale");
                        aborting = TRUE;
                    }
                    else
                    {
                        server(IS, rt_lockBigItem, biNum);
                        updateBigItem(IS);
                        what = rbi->bi_part;
                        weight = rbi->bi_weight;
                        server(IS, rt_unlockBigItem, biNum);
                        if (isInst(&saveShip, what, biNum))
                        {
                            err(IS, "item is currently installed");
                            aborting = TRUE;
                        }
                    }
                }
                /* if not aborting, do the actual transfer */
                if (!aborting)
                {
                    /* do the item first, since it is most likely to */
                    /* be changed by an outside force (no ownership) */
                    server(IS, rt_lockBigItem, biNum);
                    rbi->bi_itemLoc = plNum;
                    rbi->bi_onShip = FALSE;
                    server(IS, rt_unlockBigItem, biNum);

                    /* next change the planet so that the counts will be */
                    /* correct as soon as possible */
                    server(IS, rt_lockPlanet, plNum);
                    rpl->pl_quantity[cvtBpIt(what)]++;
                    /* if the ship is infected, infect the planet */
                    if (plagued && (rpl->pl_plagueStage == 0))
                    {
                        rpl->pl_plagueStage = 1;
                        rpl->pl_plagueTime = impRandom(IS,
                            IS->is_world.w_plagueOneRand) +
                            IS->is_world.w_plagueOneBase;
                    }
                    server(IS, rt_unlockPlanet, plNum);

                    /* lastly remove the item from the ship */
                    server(IS, rt_lockShip, shipNumber);
                    rsh->sh_items[cvtBpIt(what)]--;
                    rsh->sh_cargo -= weight;
                    server(IS, rt_unlockShip, shipNumber);
                    user(IS, "Item unloaded onto planet's surface\n");
                    fePlDirty(IS, plNum);
                    feShDirty(IS, shipNumber);
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * decMiner - decrements a ship's miner count. Assumes the ship is locked
 */

void decMiner(IMP)
{
    ULONG count;

    /* get the count */
    count = (ULONG)((IS->is_request.rq_u.ru_ship.sh_flags &
        SHF_CARRY_MINE) >> 24);
    /* remove the current count */
    IS->is_request.rq_u.ru_ship.sh_flags =
        (IS->is_request.rq_u.ru_ship.sh_flags & ~SHF_CARRY_MINE);
    if (count > 1)
    {
        /* decrement the count */
        count--;
        /* put the new count back */
        count = (count<<24);
        IS->is_request.rq_u.ru_ship.sh_flags =
            (IS->is_request.rq_u.ru_ship.sh_flags | count);
    }
}

/*
 * unloadMiner - allows the player to unload a miner onto a planet
 */

BOOL unloadMiner(IMP)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    ULONG shipNumber, plNum=NO_ITEM, mineNum; /* prevent GCC complaint */
    Ship_t saveShip;
    Planet_t savePl;
    USHORT weight;
    BOOL aborting, plagued;

    rsh = &IS->is_request.rq_u.ru_ship;
    rpl = &IS->is_request.rq_u.ru_planet;
    plagued = FALSE;
    /* get the number of the miner to unload */
    if (reqShip(IS, &mineNum, "Unload which miner from a ship"))
    {
        aborting = FALSE;
        server(IS, rt_readShip, mineNum);
        if (rsh->sh_owner != IS->is_player.p_number)
        {
            err(IS, "you don't own that miner");
        }
        else if (rsh->sh_type != st_m)
        {
            err(IS, "that ship isn't a miner");
        }
        if (rsh->sh_dragger == NO_ITEM)
        {
            err(IS, "that miner isn't being carried by a ship");
        }
        else
        {
            shipNumber = rsh->sh_dragger;
            server(IS, rt_readShip, shipNumber);
            /* make sure the ship is on a planet */
            if (rsh->sh_planet == NO_ITEM)
            {
                err(IS, "the ship carrying the miner is not on the "
                    "surface of a planet");
                aborting = TRUE;
            }
            else if (rsh->sh_owner != IS->is_player.p_number)
            {
                /* shouldn't get here */
                err(IS, "you don't own the ship carrying the miner");
            }
            else
            {
                server(IS, rt_lockShip, shipNumber);
                updateShip(IS);
                server(IS, rt_unlockShip, shipNumber);
                saveShip = *rsh;
                /* is the ship infected */
                if ((rsh->sh_plagueStage == 2) || (rsh->sh_plagueStage
                    == 3))
                {
                    /* yup */
                    plagued = TRUE;
                }
                /* find out which planet in the sector we are on */
                plNum = whichPlanet(IS, saveShip.sh_row, saveShip.sh_col);
                if (plNum != NO_ITEM)
                {
                    /* update the planet, if needed */
                    accessPlanet(IS, plNum);
                    savePl = *rpl;
                    /* does the player own the planet? */
                    if (rpl->pl_owner != IS->is_player.p_number)
                    {
                        /* no. So is the planet owned ? */
                        if (rpl->pl_owner != NO_OWNER)
                        {
                            /* yes it is, so look for a checkpoint */
                            if (rpl->pl_checkpoint[0] == '\0')
                            {
                                /* no checkpoint */
                                userP(IS, "Planet ", rpl, " is owned by "
                                    "someone else\n");
                                aborting = TRUE;
                            }
                            else
                            {
                                /* see if the player knows it */
                                if (!verifyCheckPoint(IS, rpl,
                                    cpt_access))
                                {
                                    aborting = TRUE;
                                }
                            }
                        }
                        else
                        {
                            /* see if this planet is a "home" planet */
                            if (isHomePlanet(IS, rpl->pl_number) !=
                                NO_RACE)
                            {
                                if (IS->is_player.p_race !=
                                    rpl->pl_ownRace)
                                {
                                    err(IS, "that planet is the home "
                                        "planet of another race");
                                    aborting = TRUE;
                                }
                            }
                        }
                    }
                }
                else
                {
                    err(IS, "invalid planet in unloadMiner");
                    aborting = TRUE;
                }
            }
            /* if not aborting, do the actual transfer */
            if (!aborting)
            {
                server(IS, rt_lockShip, mineNum);
                updateMiner(IS);
                weight = rsh->sh_cargo + IS->is_world.w_shipCargoLim[st_m];
                server(IS, rt_unlockShip, mineNum);
                /* do the miner first, since it is most likely to */
                /* be changed by an outside force (no ownership) */
                server(IS, rt_lockShip, mineNum);
                rsh->sh_planet = plNum;
                rsh->sh_dragger = NO_ITEM;
                /* this clears out any work left by accident */
                rsh->sh_elect[0] = 0;
                rsh->sh_elect[1] = 0;
                server(IS, rt_unlockShip, mineNum);

                /* next change the planet */
                server(IS, rt_lockPlanet, plNum);
                /* if the ship is infected, infect the planet */
                if (plagued && (rpl->pl_plagueStage == 0))
                {
                    rpl->pl_plagueStage = 1;
                    rpl->pl_plagueTime = impRandom(IS,
                        IS->is_world.w_plagueOneRand) +
                        IS->is_world.w_plagueOneBase;
                }
                server(IS, rt_unlockPlanet, plNum);
                incrPlShipCount(IS, plNum);

                /* lastly remove the miner from the ship */
                server(IS, rt_lockShip, shipNumber);
                rsh->sh_cargo -= weight;
                decMiner(IS);
                server(IS, rt_unlockShip, shipNumber);
                user(IS, "Miner unloaded onto planet's surface\n");
                fePlDirty(IS, plNum);
                feShDirty(IS, shipNumber);
                feShDirty(IS, mineNum);
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * cmd_unload - allows a player to unload big or small items from a ship
 */

BOOL cmd_unload(IMP)
{
    USHORT what;

    /* find out what they want to unload */
    if (reqChoice(IS, &what, "big\0small\0miner\0",
        "Unload (big/small/miner) items"))
    {
        (void) skipBlanks(IS);
        switch (what)
        {
            case 0:
                return unloadBigItems(IS);
                break;
            case 1:
                return unloadSmallItems(IS);
                break;
            case 2:
                return unloadMiner(IS);
                break;
        }
    }
    /* must have gotten some kind of error or blank line */
    return FALSE;
}

/*
 * doTend - does the actual transfer of goods for cmd_tend
 */

void doTend(IMP, ULONG tenderNumber, ULONG otherNumber, ItemType_t what)
{
    register Ship_t *rp;
    char buf[256];
    register USHORT available, loaded, capacity, maxTransfer;
    ULONG amount, amountOrig;

    rp = &IS->is_request.rq_u.ru_shipPair[0];
    available = rp[0].sh_items[what];
    loaded = rp[1].sh_items[what];
    capacity = getShipCapacity(IS, what, &rp[1]);
    if (available == 0)
    {
        user3(IS, "No ", getItemName(what), " on board tender\n");
    }
    else if ((available != 0) && (loaded == capacity))
    {
        user3(IS, "No room for more ", getItemName(what), "\n");
    }
    else
    {
        *IS->is_textInPos = '\0';
        maxTransfer = umin(capacity - loaded, available);
        user2(IS, "Transfer how many ", getItemName(what));
        userN3(IS, " (max ", maxTransfer, ")");
        getPrompt(IS, &buf[0]);
        if (reqPosRange(IS, &amount, maxTransfer, &buf[0]))
        {
            server2(IS, rt_lockShipPair, tenderNumber, otherNumber);
            available = rp[0].sh_items[what];
            loaded = rp[1].sh_items[what];
            amountOrig = amount;
            if (available < amount)
            {
                amount = available;
            }
            if (capacity - loaded < amount)
            {
                amount = capacity - loaded;
            }
            rp[0].sh_items[what] = available - amount;
            rp[1].sh_items[what] = loaded + amount;
            /* if the tender is infected, infect the other ship */
            if (((rp[0].sh_plagueStage == 2) || (rp[0].sh_plagueStage == 3))
                && (rp[1].sh_plagueStage == 0))
            {
                rp[1].sh_plagueStage = 1;
                rp[1].sh_plagueTime = impRandom(IS,
                    IS->is_world.w_plagueOneRand) +
                    IS->is_world.w_plagueOneBase;
            }
            server2(IS, rt_unlockShipPair, tenderNumber, otherNumber);
            if (amount != amountOrig)
            {
                userN3(IS, "Actions reduced the amount to ", amount,
                    " units\n");
            }
        }
    }
}

/*
 * cmd_tend - allows a player to transfer goods from one ship to another
 */

BOOL cmd_tend(IMP)
{
    register Ship_t *rp;
    ULONG tenderNum, otherNum;
    Ship_t saveShip[2];
    ItemType_t it;

    if (reqShip(IS, &otherNum, "Ship to tend") && doSkipBlanks(IS) &&
        reqShip(IS, &tenderNum, "Tender to transfer from"))
    {
        rp = &IS->is_request.rq_u.ru_shipPair[0];
        if (tenderNum == otherNum)
        {
            err(IS, "a ship can't tend itself");
        }
        else
        {
            server2(IS, rt_readShipPair, tenderNum, otherNum);
            if (rp[1].sh_owner != IS->is_player.p_number)
            {
                userN3(IS, "You don't own ship #", otherNum, "\n");
            }
            else if (rp[0].sh_planet != NO_ITEM)
            {
                userN3(IS, "Ship #", tenderNum, " is on the surface of a"
                    "planet and can't tend other ships\n");
            }
            else if (rp[1].sh_planet != NO_ITEM)
            {
                userN3(IS, "Ship #", otherNum, " is on the surface of a"
                    "planet and can't be tended by other ships\n");
            }
            else if (rp[0].sh_owner != IS->is_player.p_number)
            {
                userN3(IS, "You don't own ship #", tenderNum, "\n");
            }
            else if (rp[0].sh_type == st_m)
            {
                userN3(IS, "Ship #", tenderNum, " is a miner and can't"
                    " tend other ships\n");
            }
            else if (rp[1].sh_type == st_m)
            {
                userN3(IS, "Ship #", otherNum, " is a miner and can't"
                    " be tended by other ships\n");
            }
            else if ((rp[0].sh_row != rp[1].sh_row) ||
                (rp[0].sh_col != rp[1].sh_col))
            {
                err(IS, "ships aren't in same subsector");
            }
            else
            {
                server(IS, rt_lockShip, otherNum);
                updateShip(IS);
                server(IS, rt_unlockShip, otherNum);
                saveShip[0] = IS->is_request.rq_u.ru_ship;
                server(IS, rt_lockShip, tenderNum);
                updateShip(IS);
                server(IS, rt_unlockShip, tenderNum);
                saveShip[1] = IS->is_request.rq_u.ru_ship;
                /* set up the ship pair */
                rp[0] = saveShip[0];
                rp[1] = saveShip[1];
                for (it = IT_FIRST; it <= IT_LAST_SMALL; it++)
                {
                    doTend(IS, tenderNum, otherNum, it);
                }
                feShDirty(IS, tenderNum);
                feShDirty(IS, otherNum);
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * doFleet - part of cmd_fleet
 */

void doFleet(IMP, register ULONG shipNumber, register Ship_t *sh)
{
    register Ship_t *rsh;
    register Fleet_t *rf;

    if (sh->sh_owner == IS->is_player.p_number)
    {
        server(IS, rt_lockShip, shipNumber);
        rsh = &IS->is_request.rq_u.ru_ship;
        rf = &IS->is_request.rq_u.ru_fleet;
        updateShip(IS);
        removeFromFleet(IS, IS->is_player.p_number, shipNumber);
        if (!IS->is_BOOL1)
        {
            /* putting the ship into a fleet */
            if (rsh->sh_price != 0)
            {
                server(IS, rt_unlockShip, shipNumber);
                userN3(IS, "ship ", shipNumber,
                       " is for sale - cannot put it into a fleet\n");
            }
            else if (rsh->sh_type == st_m)
            {
                /* note that it should now be impossible to get here */
                /* as this function only gets called via scanShips() */
                /* which will not match any miner class              */
                server(IS, rt_unlockShip, shipNumber);
                userN3(IS, "ship ", shipNumber,
                       " is a miner and can not be put into a fleet\n");
            }
            else
            {
                rsh->sh_fleet = IS->is_USHORT2 + '\0';
                server(IS, rt_unlockShip, shipNumber);
                server(IS, rt_lockFleet, IS->is_USHORT1);
                if (rf->f_count != FLEET_MAX)
                {
                    rf->f_ship[rf->f_count] = shipNumber;
                    rf->f_count++;
                    server(IS, rt_unlockFleet, IS->is_USHORT1);
                }
                else
                {
                    /* oops - it wouldn't fit in the fleet */
                    server(IS, rt_unlockFleet, IS->is_USHORT1);
                    server(IS, rt_lockShip, shipNumber);
                    rsh->sh_fleet = '*';
                    server(IS, rt_unlockShip, shipNumber);
                }
            }
        }
        else
        {
            server(IS, rt_unlockShip, shipNumber);
        }
        feShDirty(IS, shipNumber);
    }
}

/*
 * cmd_fleet - allows a player to add/remove a ship from a fleet, or to
 *          display the ships currently in a fleet
 */

void cmd_fleet(IMP)
{
    register Ship_t *rsh;
    register Fleet_t *rf;
    ShipScan_t shs;
    register ULONG i, pos;
    char fleetChar;
    BOOL headerDone;

    /* find out which fleet they want to work with */
    if (reqChar(IS, &fleetChar,
               "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ*",
               "Display which fleet", "illegal fleet character"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rf = &IS->is_request.rq_u.ru_fleet;
        (void) skipBlanks(IS);
        /* if there is nothing else in the input buffer, then just */
        /* display the fleet */
        if (*IS->is_textInPos == '\0')
        {
            headerDone = FALSE;
            if (fleetChar == '*')
            {
                /* they want a list of ships that are not in any fleet */
                if (IS->is_world.w_shipNext != 0)
                {
                    /* loop through each of the ships and print out */
                    /* the number if the ship is not in a fleet */
                    for (i = 0; i < IS->is_world.w_shipNext; i++)
                    {
                        server(IS, rt_readShip, i);
                        if ((rsh->sh_owner == IS->is_player.p_number) &&
                            (rsh->sh_fleet == '*'))
                        {
                            if (!headerDone)
                            {
                                headerDone = TRUE;
                                user(IS, "Ships not part of a fleet: ");
                            }
                            else
                            {
                                userC(IS, '/');
                            }
                            userN(IS, i);
                        }
                    }
                }
                if (headerDone)
                {
                    userNL(IS);
                }
                else
                {
                    user(IS, "All of your ships are in a fleet.\n");
                }
            }
            else
            {
                pos = fleetPos(fleetChar);
                if (IS->is_player.p_fleets[pos] == NO_FLEET)
                {
                    user(IS, "You have no fleet ");
                    userC(IS, fleetChar);
                    userNL(IS);
                }
                else
                {
                    server(IS, rt_readFleet, IS->is_player.p_fleets[pos]);
                    if (rf->f_count == 0)
                    {
                        user(IS, "You have no ships in fleet ");
                        userC(IS, fleetChar);
                        userNL(IS);
                    }
                    else
                    {
                        user(IS, "Ships in fleet ");
                        userC(IS, fleetChar);
                        user(IS, ": ");
                        for (i = 0; i < rf->f_count; i++)
                        {
                            if (i != 0)
                            {
                                userC(IS, '/');
                            }
                            userN(IS, rf->f_ship[i]);
                        }
                        userNL(IS);
                    }
                }
            }
        }
        /* there is more on the line, so try and get a group of ship */
        /* numbers */
        else if (getShips(IS, &shs))
        {
            /* got a ship number, so they are changing the status of */
            /* the ships fleet */
            if (fleetChar == '*')
            {
                /* taking ships out of specific fleets */
                IS->is_BOOL1 = TRUE;
            }
            else
            {
                /* adding the ship to the fleet */
                IS->is_BOOL1 = FALSE;
                IS->is_USHORT2 = fleetChar - '\0';
                pos = fleetPos(fleetChar);
                IS->is_USHORT1 = IS->is_player.p_fleets[pos];
                /* see if the fleet currently exists */
                if (IS->is_USHORT1 == NO_FLEET)
                {
                    /* creating a new fleet */
                    server(IS, rt_lockWorld, 0);
                    IS->is_USHORT1 = IS->is_request.rq_u.ru_world.w_fleetNext;
                    IS->is_request.rq_u.ru_world.w_fleetNext =
                        IS->is_USHORT1 + 1;
                    server(IS, rt_unlockWorld, 0);
                    IS->is_world.w_fleetNext = IS->is_USHORT1 + 1;
                    /* clear out fleet array */
                    rf->f_count = 0;
                    server(IS, rt_createFleet, IS->is_USHORT1);
                    server(IS, rt_lockPlayer, IS->is_player.p_number);
                    IS->is_request.rq_u.ru_player.p_fleets[pos] =
                        IS->is_USHORT1;
                    server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    IS->is_player.p_fleets[pos] = IS->is_USHORT1;
                }
            }
            if (scanShips(IS, &shs, doFleet) == 0)
            {
                err(IS, "no ships matched");
            }
            /* reset this to prevent problems */
            IS->is_BOOL1 = FALSE;
        }
    }
}

#ifdef BUBBA
proc doTorp(USHORT subNumber, victimNumber)void:
    register *Ship_t rsh;
    register *char name;
    register USHORT distance, i, damage;
    Fleet_t fleet;
    [256] char buf;
    USHORT defender, mapped, aRow, aCol, j;
    int sRow, sCol, vRow, vCol;
    BOOL killed, underSea;
    char fleetChar;
    Ship_t sub;

    server(rt_lockShip, subNumber);
    torpCost();
    server(rt_unlockShip, subNumber);
    rsh = &IS->is_request.rq_u.ru_ship;
    sub = rsh*;
    aRow = rsh->sh_row;
    aCol = rsh->sh_col;
    sRow = rowToMe(rsh->sh_row);
    sCol = colToMe(rsh->sh_col);
    server(rt_readShip, victimNumber);
    defender = rsh->sh_owner;
    underSea = rsh->sh_type == st_submarine;
    vRow = rowToMe(rsh->sh_row);
    vCol = colToMe(rsh->sh_col);
    fleetChar = rsh->sh_fleet;
    distance = findDistance(sRow, sCol, vRow, vCol);
    if distance > IS->is_world.w_torpRange then
        distance = IS->is_world.w_torpRange + 2;
    }
    user("FWHOOSH");
    uFlush();
    for i from 0 upto impRandom(5) + distance * 2 do
        sleep(10);
        userC('.');
        uFlush();
    }
    if distance > IS->is_world.w_torpRange then
        user("out of range!\n");
    elif impRandom(100) <
        if distance = 0 then
            IS->is_world.w_torpAcc0
        elif distance = 1 then
            IS->is_world.w_torpAcc1
        elif distance = 2 then
            IS->is_world.w_torpAcc2
        else
            IS->is_world.w_torpAcc3
        fi
    then
        user("BOOM!\n");
        damage = (impRandom(IS->is_world.w_torpRand) + IS->is_world.w_torpBase) *
                        IS->is_world.w_shipDamage[rsh->sh_type];
        attackShip(victimNumber, damage, at_torp, "A submarine");
    else
        user("missed.\n");
    }
    mapped = mapAbsSector(sRow, sCol);
    server(rt_readShip, victimNumber);
    if underSea then
        /* undersea fight - the victim sub can fire back */
        if rsh->sh_efficiency >= 60 and rsh->sh_guns != 0 and
            rsh->sh_items[it_shells] >= IS->is_world.w_torpCost
        then
            server(rt_lockShip, victimNumber);
            torpCost();
            server(rt_unlockShip, victimNumber);
            user("Approaching torpedo!!");
            uFlush();
            for i from 0 upto impRandom(5) + distance * 2 do
                sleep(10);
                userC('.');
                uFlush();
            }
            if impRandom(100) <
                if distance = 0 then
                    IS->is_world.w_torpAcc0
                elif distance = 1 then
                    IS->is_world.w_torpAcc1
                elif distance = 2 then
                    IS->is_world.w_torpAcc2
                else
                    IS->is_world.w_torpAcc3
                fi
            then
                user("BOOM!\n");
                damage = (impRandom(IS->is_world.w_torpRand) +
                            IS->is_world.w_torpBase) *
                            IS->is_world.w_shipDamage[st_submarine];
                userN3("Torpedo does ", damage, "% damage!\n");
                server(rt_lockShip, subNumber);
                damageShip(subNumber, damage, TRUE);
                server(rt_unlockShip, subNumber);
                sub = rsh*; /* set current status after damage */
            else
                user("missed.\n");
            }
        }
        /* now check for other subs in the area    */
        i = 0;
        server(rt_readShip, i);
        while
           i != IS->is_world.w_shipNext and sub.sh_owner != NO_OWNER
        do
           vRow = rowToMe(rsh->sh_row);
           vCol = colToMe(rsh->sh_col);
           distance = findDistance(sRow, sCol, vRow, vCol);
           if distance > IS->is_world.w_torpRange then
               distance = IS->is_world.w_torpRange + 2;
           }
           if rsh->sh_efficiency >= 60 and rsh->sh_guns != 0 and
               rsh->sh_items[it_shells] >= IS->is_world.w_torpCost and
               distance <= IS->is_world.w_torpRange and
               rsh->sh_owner = defender and rsh->sh_type == st_submarine
           then
               server(rt_lockShip, i);
               torpCost();
               server(rt_unlockShip, i);
               user("   Approaching torpedo!!");
               uFlush();
               for j from 0 upto impRandom(5) + distance * 2 do
                   sleep(10);
                   userC('.');
                   uFlush();
               }
               if impRandom(100) <
                   if distance = 0 then
                       IS->is_world.w_torpAcc0
                   elif distance = 1 then
                       IS->is_world.w_torpAcc1
                   elif distance = 2 then
                       IS->is_world.w_torpAcc2
                   else
                       IS->is_world.w_torpAcc3
                   fi
               then
                   user("BOOM!\n");
                   damage = (impRandom(IS->is_world.w_torpRand) +
                            IS->is_world.w_torpBase) *
                            IS->is_world.w_shipDamage[st_submarine];
                   userN3("Torpedo does ", damage, "% damage!\n");
                   server(rt_lockShip, subNumber);
                   damageShip(subNumber, damage, TRUE);
                   server(rt_unlockShip, subNumber);
                   sub = rsh*; /* set current status after damage */
               else
                   user("missed.\n");
               }
           }
           i = i + 1;
           if i < IS->is_world.w_shipNext then
               server(rt_readShip, i);
           }
        }
    elif fleetChar != '*' then
        /* victim on surface - destroyers in same fleet can depth charge */
        killed = FALSE;
        server(rt_readPlayer, defender);
        server(rt_readFleet,
               IS->is_request.rq_u.ru_player.c_fleets[fleetPos(fleetChar)]);
        fleet = IS->is_request.rq_u.ru_fleet;
        i = 0;
        while i != fleet.f_count and not killed do
            victimNumber = fleet.f_ship[i];
            server(rt_readShip, victimNumber);
            if rsh->sh_row == aRow and rsh->sh_col == aCol and
                rsh->sh_type == st_destroyer and rsh->sh_efficiency >= 60 and
                rsh->sh_items[it_shells] >= IS->is_world.w_chargeCost
            then
                server(rt_lockShip, victimNumber);
                updateShip(IS, victimNumber);
                if rsh->sh_owner != NO_OWNER then
                    dropCost();
                    server(rt_unlockShip, victimNumber);
                    damage = (impRandom(IS->is_world.w_chargeRand) +
                                    IS->is_world.w_chargeBase) *
                                IS->is_world.w_shipDamage[st_submarine];
                    damage = umin((USHORT) 100, damage);
                    userN3("Kawhoomph! Depth charge does ",
                           damage, "% damage!\n");
                    server(rt_lockShip, subNumber);
                    damageShip(subNumber, damage, TRUE);
                    server(rt_unlockShip, subNumber);
                    if rsh->sh_owner = NO_OWNER then
                        killed = TRUE;
                    }
                else
                    server(rt_unlockShip, victimNumber);
                }
            }
            i = i + 1;
        }
    }
}

proc cmd_torpedo()BOOL:
    register *Ship_t rsh;
    USHORT subNumber, victimNumber;

    if reqShip(&victimNumber, "Ship to torpedo") and
        doSkipBlanks() and
        reqShip(&subNumber, "Submarine to launch torpedo")
    then
        server(rt_readShip, subNumber);
        rsh = &IS->is_request.rq_u.ru_ship;
        if rsh->sh_owner != IS->is_playerNumber then
            userN3("You don't own ship #", subNumber, "\n");
        else
            accessShip(subNumber);
            if rsh->sh_type != st_submarine then
                userN3("Ship #", subNumber, " isn't a submarine\n");
            elif rsh->sh_guns = 0 then
                user("No torpedo tubes (guns) available\n");
            elif rsh->sh_items[it_shells] < IS->is_world.w_torpCost then
                userN3("No torpedos (", IS->is_world.w_torpCost,
                       " shells each) on board\n");
            elif rsh->sh_efficiency < 60 then
                user("Submarine not efficient enough to fire torpedos\n");
            elif victimNumber = subNumber then
                user("submarines can't torpedo themselves\n");
            else
                server(rt_readShip, victimNumber);
                if rsh->sh_owner = NO_OWNER then
                    userN3("Ship #", victimNumber,
                       " lies a-rusting at the bottom of the deep blue "
                       "sea!\n");
                elif rsh->sh_owner = IS->is_playerNumber then
                   user("The captain refuses to torp one of our own "
                       "ships!\n");
                else
                    doTorp(subNumber, victimNumber);
                }
            }
        }
        TRUE
    else
        FALSE
    fi
}
#endif

BOOL cmd_refurb(IMP)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    register Player_t *rp;
    BigItem_t *rbi;
    ULONG shipNumber, planetNumber, biNum;
    USHORT shipTechLevel, planTechLevel, delta;
    long cost;

    if (reqShip(IS, &shipNumber,"Refurbish which ship"))
    {
        server(IS, rt_readShip, shipNumber);
        rsh = &IS->is_request.rq_u.ru_ship;
        rpl = &IS->is_request.rq_u.ru_planet;
        rp = &IS->is_request.rq_u.ru_player;
        rbi = &IS->is_request.rq_u.ru_bigItem;
        if (rsh->sh_owner != IS->is_player.p_number)
        {
            err(IS, "you don't own that ship");
        }
        else if (rsh->sh_type == st_m)
        {
            err(IS, "that ship is a miner and can't be refurbished");
        }
        else
        {
            accessShip(IS, shipNumber);
            planetNumber = rsh->sh_planet;
            biNum = rsh->sh_hull;
            server(IS, rt_readBigItem, biNum);
            shipTechLevel = rbi->bi_techLevel;
            if (planetNumber == NO_ITEM)
            {
                err(IS, "ship is not on the surface of a planet");
                return TRUE;
            }
            accessPlanet(IS, planetNumber);
	    if (rpl->pl_btu < 2)
            {
                userP(IS, "Planet ", rpl, " does not have enough BTUs to "
                    "refurbish your ship.\n");
                return TRUE;
            }
            planTechLevel = rpl->pl_techLevel;
            if (rpl->pl_owner != IS->is_player.p_number)
            {
                if (rpl->pl_checkpoint[0] == '\0')
                {
                    userP(IS, "Planet ", rpl, " is owned by someone "
                        "else\n");
                    return TRUE;
                }
                if (!verifyCheckPoint(IS, rpl, cpt_access))
                {
                    return TRUE;
                }
            }
            if (shipTechLevel >= planTechLevel)
            {
                err(IS, "ship is as advanced as the planet can "
                    "make it");
            }
            else if (rpl->pl_prod[pp_hull] == 0)
            {
                err(IS, "no production on planet for refurb");
            }
            else
            {
                delta = planTechLevel - shipTechLevel;
                if (delta > rpl->pl_prod[pp_hull] * 4)
                {
                    delta = rpl->pl_prod[pp_hull] * 4;
                }
                cost = delta * IS->is_world.w_refurbCost;
                if (cost > IS->is_player.p_money)
                {
                    err(IS, "you can't afford to refurbish that ship");
                }
                else
                {
                    server(IS, rt_lockPlanet, planetNumber);
                    if (rpl->pl_prod[pp_hull] * 4 < delta)
                    {
                        delta = rpl->pl_prod[pp_hull] * 4;
                        cost = delta * IS->is_world.w_refurbCost;
                    }
                    if (rpl->pl_prod[pp_hull] > 0)
                    {
                        rpl->pl_prod[pp_hull] -= delta / 4;
                    }
		    if (rpl->pl_btu >=2 )
		    {
			rpl->pl_btu -= 2;
		    }
                    server(IS, rt_unlockPlanet, planetNumber);
                    server(IS, rt_lockBigItem, biNum);
                    rbi->bi_techLevel += delta;
                    shipTechLevel = rbi->bi_techLevel;
                    server(IS, rt_unlockBigItem, biNum);
                    server(IS, rt_lockShip, shipNumber);
                    rsh->sh_hullTF = getTechFactor(shipTechLevel);
                    server(IS, rt_unlockShip, shipNumber);
                    if (shipTechLevel == planTechLevel)
                    {
                        user(IS, "Ship brought up to date.\n");
                    }
                    else
                    {
                        user(IS,
                         "Not enough production for a full refurb.\n");
                        userN3(IS, "Ship brought to tech level ",
                               shipTechLevel, ".\n");
                    }
                    server(IS, rt_lockPlayer, IS->is_player.p_number);
                    rp->p_money -= cost;
                    server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    IS->is_player.p_money = rp->p_money;
                    fePlDirty(IS, planetNumber);
                    feShDirty(IS, shipNumber);
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * cmd_install - allows the player to install a big item already loaded
 *          onto the ship into one of the ship's locations
 */

BOOL cmd_install(IMP)
{
    register Ship_t *rsh;
    register BigItem_t *rbi;
    ULONG shipNumber, biNum, *pArr=NULL;
    Ship_t saveShip;
    USHORT iMax, which=0;/* prevent GCC complaints */
    BOOL aborting;
    BigPart_t what=0;/* prevent GCC complaints */

    /* get the number of the ship */
    if (reqShip(IS, &shipNumber, "Ship that the item is on"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rbi = &IS->is_request.rq_u.ru_bigItem;
        (void) skipBlanks(IS);
        /* get the number of the item to install */
        if (reqBigItem(IS, &biNum, "Item number to install"))
        {
            aborting = FALSE;
            server(IS, rt_readShip, shipNumber);
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else if (rsh->sh_type == st_m)
            {
                err(IS, "that ship is a miner and doesn't carry big items");
            }
            else
            {
                saveShip = *rsh;
                /* make sure the item is on the ship */
                server(IS, rt_readBigItem, biNum);
                if ((rbi->bi_itemLoc != shipNumber) || !rbi->bi_onShip ||
                    (rbi->bi_status != bi_inuse))
                {
                    err(IS, "that item is not on the requested ship");
                    aborting = TRUE;
                }
                else
                {
                    server(IS, rt_lockBigItem, biNum);
                    updateBigItem(IS);
                    server(IS, rt_unlockBigItem, biNum);
                    what = rbi->bi_part;
                    if (rbi->bi_price > 0)
                    {
                        err(IS, "that item is currently for sale");
                        aborting = TRUE;
                    }
                    else if (isInst(&saveShip, what, biNum))
                    {
                        err(IS, "that item is already installed");
                        aborting = TRUE;
                    }
                    else
                    {
                        switch(what)
                        {
                            case bp_computer:
                                pArr = &rsh->sh_computer[0];
                                iMax = MAX_COMP;
                                break;
                            case bp_engines:
                                pArr = &rsh->sh_engine[0];
                                iMax = MAX_ENG;
                                break;
                            case bp_lifeSupp:
                                pArr = &rsh->sh_lifeSupp[0];
                                iMax = MAX_LIFESUP;
                                break;
                            case bp_sensors:
                            case bp_teleport:
                            case bp_shield:
                            case bp_tractor:
                                pArr = &rsh->sh_elect[0];
                                iMax = MAX_ELECT;
                                break;
                            case bp_photon:
                            case bp_blaser:
                                pArr = &rsh->sh_weapon[0];
                                iMax = MAX_WEAP;
                                break;
                            default:
                                err(IS, "bad item type");
                                return FALSE;
                        }
                        /* ship should already be in cache */
                        server(IS, rt_readShip, shipNumber);
                        which = findFree(rsh, what);
                        if (which >= iMax)
                        {
                            err(IS, "no room for more of those items");
                            aborting = TRUE;
                        }

                    }
                }
                /* if not aborting, do the actual installation */
                if (!aborting)
                {
                    server(IS, rt_lockShip, shipNumber);
                    pArr[which] = biNum;
		    /* If we installed an engine, we need to rebuild these fields */
		    if (what == bp_engines)
		    {
			saveShip = *rsh;
			saveShip.sh_engEff = getShipEff(IS, &saveShip,
						       bp_engines);
			saveShip.sh_engTF = getShipTechFactor(IS, &saveShip,
							      bp_engines);
			*rsh = saveShip;
		    }
                    server(IS, rt_unlockShip, shipNumber);
                    user(IS, "Item installed\n");
                    feShDirty(IS, shipNumber);
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * cmd_remove - allows the player to un-install a big item
 */

BOOL cmd_remove(IMP)
{
    register Ship_t *rsh;
    register BigItem_t *rbi;
    ULONG shipNumber, biNum;
    Ship_t saveShip;
    BOOL aborting;
    BigPart_t what=0; /* prevent GCC complaints */

    /* get the number of the ship */
    if (reqShip(IS, &shipNumber, "Ship that the item is on"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rbi = &IS->is_request.rq_u.ru_bigItem;
        (void) skipBlanks(IS);
        /* get the number of the item to install */
        if (reqBigItem(IS, &biNum, "Item number to remove"))
        {
            aborting = FALSE;
            server(IS, rt_readShip, shipNumber);
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else if (rsh->sh_type == st_m)
            {
                err(IS, "that ship is a miner and doesn't carry big items");
            }
            else
            {
                saveShip = *rsh;
                /* make sure the item is on the ship */
                server(IS, rt_readBigItem, biNum);
                if ((rbi->bi_itemLoc != shipNumber) || !rbi->bi_onShip ||
                    (rbi->bi_status != bi_inuse))
                {
                    err(IS, "that item is not on the requested ship");
                    aborting = TRUE;
                }
                else
                {
                    server(IS, rt_lockBigItem, biNum);
                    updateBigItem(IS);
                    server(IS, rt_unlockBigItem, biNum);
                    what = rbi->bi_part;
                    if (!isInst(&saveShip, what, biNum))
                    {
                        err(IS, "that item is not installed");
                        aborting = TRUE;
                    }
                }
                /* if not aborting, do the actual un-installation */
		if (!aborting)
		{
                    server(IS, rt_lockShip, shipNumber);
                    /* uninstall the item */
                    doUninstall(rsh, biNum, what);
                    if (what == it_engines)
                    {
			saveShip = *rsh;
			/* We need to possibly reduce the amount of energy */
			saveShip.sh_energy = umin(saveShip.sh_energy,
                            (USHORT) numInst(IS, &saveShip, bp_engines) * 127);
			/* Since we removed an engine, we need to rebuild */
			/* these fields */
			saveShip.sh_engEff = getShipEff(IS, &saveShip,
						       bp_engines);
			saveShip.sh_engTF = getShipTechFactor(IS, &saveShip,
							      bp_engines);
			*rsh = saveShip;
                    }
                    server(IS, rt_unlockShip, shipNumber);
                    user(IS, "Item removed\n");
                    feShDirty(IS, shipNumber);
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

