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
 * $Id: cmd_telep.c,v 1.2 2000/05/18 06:50:02 marisa Exp $
 *
 * $Log: cmd_telep.c,v $
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:51  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:24:24  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:14  marisag
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

static const char rcsid[] = "$Id: cmd_telep.c,v 1.2 2000/05/18 06:50:02 marisa Exp $";

/*
 * doBeamUp - does the actual user querry about the quantity to beam up (if
 *          needed), and puts the items onto the ship
 *          Convention: the planet and ship are in the request as a pair.
 */

void doBeamUp(IMP, ItemType_t what, USHORT percent, UBYTE telepTF,
    BOOL askQuan)
{
    register PlanetShipPair_t *rp;
    register USHORT capacity, present, loaded;
    ULONG plNum, shipNumber;
    ULONG amount, maxBeamUp, amountOrig, telepAmount;
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
            /* see if they are trying to teleport restricted items */
            if ((what == it_civilians) || (what == it_scientists) ||
                (what == it_military) || (what == it_officers))
            {
                /* set the limit on how many they may beam up */
                if (present > 500)
                {
                    present = umin((USHORT) present - 500, (USHORT) 500);
                }
                else
                {
                    /* if less than 500, they can't beam up any */
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
        user3(IS, "No ", getItemName(what), " here to beam up\n");
    }
    else if ((present != 0) && (capacity == loaded) && askQuan)
    {
        user3(IS, "No room for more ", getItemName(what), "\n");
    }
    else if ((capacity != 0) && (present != 0) && (capacity != loaded))
    {
        maxBeamUp = umin(capacity - loaded, present);
        if (askQuan)
        {
            *IS->is_textInPos = '\0';
            userN(IS, present);
            user2(IS, " ", getItemName(what));
            userN2(IS, " here, ", loaded);
            userN3(IS, " on board, beam up how many (max ", maxBeamUp, ")");
            getPrompt(IS, &buf[0]);
            if (!reqPosRange(IS, &amount, maxBeamUp, &buf[0]))
            {
                amount = 0;
            }
        }
        else
        {
            amount = umin((USHORT) (capacity * percent) / 100, maxBeamUp);
            userN3(IS, "Teleporting ", amount, " ");
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
            /* check for possible teleport problems */
            if (impRandom(IS, 100) < ((3 * telepTF) / 2))
            {
                /* everything went OK */
                telepAmount = amount;
            }
            else
            {
                telepAmount = ((amount * (75 + impRandom(IS, 24))) / 100);
            }
            writePlQuan(IS, &rp->p_p, what, present - amount);
            rp->p_sh.sh_items[what] += telepAmount;
            rp->p_sh.sh_cargo += (telepAmount * IS->is_world.w_weight[what]);
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
                userN3(IS, "Events have reduced amount to ", amount,
                    " units.\n");
            }
            if (telepAmount != amount)
            {
                userN3(IS, "Teleporter problems reduced the amount to ",
                    amount, " units\n");
            }
        }
    }
}

/*
 * beamUpItems - allows a player to beam up items onto a ship which is
 *          orbiting a planet
 */

BOOL beamUpItems(IMP)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    register PlanetShipPair_t *rpp;
    Ship_t saveShip;
    Planet_t savePl;
    USHORT percent = 0, owner;
    ULONG shipNumber, plNum;
    ItemType_t it;
    BOOL askQuan, aborting;
    UBYTE telepTF = 0;

    /* get the ship number */
    if (reqShip(IS, &shipNumber, "Teleport items onto ship"))
    {
        /* set up the pointers */
        rsh = &IS->is_request.rq_u.ru_ship;
        rpl = &IS->is_request.rq_u.ru_planet;
        rpp = &IS->is_request.rq_u.ru_planetShipPair;
        (void) skipBlanks(IS);
        aborting = FALSE;
        askQuan = FALSE;

        /* if they have anything left on the command line, assume it is a */
        /* percent to be teleported */
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
                err(IS, "teleport cancelled");
                aborting = TRUE;
            }
        }
        /* abort if they entered an invalid number to beam up */
        if (!aborting)
        {
            /* read the ship in */
            server(IS, rt_readShip, shipNumber);
            /* store a copy of the ship */
            saveShip = *rsh;
            /* make sure they own it */
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else if (numInst(IS, &saveShip, bp_teleport) == 0)
            {
                err(IS, "that ship does not have any teleport units "
                    "installed");
            }
            else if (numInst(IS, &saveShip, bp_computer) == 0)
            {
                err(IS, "that ship does not have any computers installed");
            }
            else if (getShipEff(IS, &saveShip, bp_teleport) < EFFIC_WARN)
            {
                err(IS, "that ships teleport units are not efficient enough "
                    "for operation");
            }
            else if (getShipEff(IS, &saveShip, bp_computer) < EFFIC_WARN)
            {
                err(IS, "that ships computers are not efficient enough "
                    "for operation");
            }
            else
            {
                /* make sure the ship is not on a planet */
                if (saveShip.sh_planet != NO_ITEM)
                {
                    err(IS, "ship is on the surface of a planet");
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
                    /* find out which planet in the sector we are over */
                    plNum = whichPlanet(IS, saveShip.sh_row, saveShip.sh_col);
                    if (plNum != NO_ITEM)
                    {
                        /* build up the ship's teleport tech factor */
                        telepTF = getShipTechFactor(IS, &saveShip,
                            bp_teleport);
                        telepTF = (telepTF + getShipTechFactor(IS, &saveShip,
                            bp_computer)) / 2;
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
                                    userP(IS, "Planet ", rpl, " is controlled"
                                        " by someone else\n");
                                    aborting = TRUE;
                                }
                                else
                                {
                                    /* there is a code, so see if the plyr */
                                    /* knows it */
                                    if (!verifyCheckPoint(IS, rpl, cpt_land))
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
                        err(IS, "ship is not orbiting any planet");
                        aborting = TRUE;
                    }
                }
                /* is everything still hunky dorey? */
                if (!aborting)
                {
                    /* fill in the planetShipPair struct */
                    rpp->p_sh = saveShip;
                    rpp->p_p = savePl;
                    /* loop through the items, beaming them up */
                    for (it = IT_FIRST; it <= IT_LAST_SMALL; it++)
                    {
                        doBeamUp(IS, it, percent, telepTF, askQuan);
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
                }
            }
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

/*
 * doBeamDown - does the actual item transfer for beamDownItems()
 *          Assumes that the planet and ship are in the request as a pair
 */

void doBeamDown(IMP, ItemType_t what, USHORT percent, UBYTE telepTF,
    BOOL askQuan)
{
    register PlanetShipPair_t *rp;
    register USHORT capacity, present, loaded;
    ULONG plNum, shipNumber;
    ULONG amount, maxBeamDown, amountOrig, telepAmount;
    char buf[256];

    rp = &IS->is_request.rq_u.ru_planetShipPair;
    plNum = rp->p_p.pl_number;
    shipNumber = rp->p_sh.sh_number;
    capacity = MAX_WORK;
    present = readPlQuan(IS, &rp->p_p, what);
    loaded = rp->p_sh.sh_items[what];
    if ((capacity == present) && (loaded != 0))
    {
        user3(IS, "No room to beam down ", getItemName(what), "\n");
    }
    else if ((loaded == 0) && (getShipCapacity(IS, what, &rp->p_sh) != 0))
    {
        user3(IS, "No ", getItemName(what), " on board\n");
    }
    else if ((loaded != 0) && (capacity != present))
    {
        maxBeamDown = umin(capacity - present, loaded);
        if (askQuan)
        {
            *IS->is_textInPos = '\0';
            userN(IS, present);
            user2(IS, " ", getItemName(what));
            userN2(IS, " here, ", loaded);
            userN3(IS, " on board, beam down how many (max ", maxBeamDown,
                ")");
            getPrompt(IS, &buf[0]);
            if (!reqPosRange(IS, &amount, maxBeamDown, &buf[0]))
            {
                amount = 0;
            }
        }
        else
        {
            amount = umin((USHORT) (loaded * percent) / 100, maxBeamDown);
            userN3(IS, "Teleporting ", amount, " ");
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
            /* check for possible teleport problems */
            if (impRandom(IS, 100) < ((3 * telepTF) / 2))
            {
                /* everything went OK */
                telepAmount = amount;
            }
            else
            {
                telepAmount = ((amount * (75 + impRandom(IS, 24))) / 100);
            }
            writePlQuan(IS, &rp->p_p, what, present + telepAmount);
            rp->p_sh.sh_items[what] -= amount;
            rp->p_sh.sh_cargo -= (amount * IS->is_world.w_weight[what]);
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
                userN3(IS, "Actions reduced the amount to ", amount,
                    " units\n");
            }
            if (telepAmount != amount)
            {
                userN3(IS, "Teleporter problems reduced the amount to ",
                    amount, " units\n");
            }
        }
    }
}

/*
 * beamDownItems - allows a player to beam down items from a ship
 */

BOOL beamDownItems(IMP)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    register PlanetShipPair_t *rp;
    Ship_t saveShip;
    Planet_t savePl;
    register USHORT percent = 0;
    ULONG shipNumber, plNum;
    ItemType_t it;
    BOOL askQuan, aborting, wasDeserted;
    UBYTE telepTF = 0;

    if (reqShip(IS, &shipNumber, "Teleport from ship"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rpl = &IS->is_request.rq_u.ru_planet;
        rp = &IS->is_request.rq_u.ru_planetShipPair;
        (void) skipBlanks(IS);
        aborting = FALSE;
        askQuan = FALSE;

        /* if they have anything left on the command line, assume it is a */
        /* percent to be teleported */
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
                err(IS, "teleport cancelled");
                aborting = TRUE;
            }
        }
        /* abort if they entered an invalid number to beam down */
        if (!aborting)
        {
            /* read the ship in */
            server(IS, rt_readShip, shipNumber);
            /* store a copy of the ship */
            saveShip = *rsh;
            /* make sure they own it */
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else if (numInst(IS, &saveShip, bp_teleport) == 0)
            {
                err(IS, "that ship does not have any teleport units "
                    "installed");
            }
            else if (numInst(IS, &saveShip, bp_computer) == 0)
            {
                err(IS, "that ship does not have any computers installed");
            }
            else if (getShipEff(IS, &saveShip, bp_teleport) < EFFIC_WARN)
            {
                err(IS, "that ships teleport units are not efficient enough "
                    "for operation");
            }
            else if (getShipEff(IS, &saveShip, bp_computer) < EFFIC_WARN)
            {
                err(IS, "that ships computers are not efficient enough "
                    "for operation");
            }
            else
            {
                /* make sure the ship is not on a planet */
                if (saveShip.sh_planet != NO_ITEM)
                {
                    err(IS, "ship is on the surface of a planet");
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
                    /* find out which planet in the sector we are over */
                    plNum = whichPlanet(IS, saveShip.sh_row, saveShip.sh_col);
                    if (plNum != NO_ITEM)
                    {
                        /* build up the ship's teleport tech factor */
                        telepTF = getShipTechFactor(IS, &saveShip,
                            bp_teleport);
                        telepTF = (telepTF + getShipTechFactor(IS, &saveShip,
                            bp_computer)) / 2;
                        /* update the planet, if needed */
                        accessPlanet(IS, plNum);
                        /* save the planet for later use */
                        savePl = *rpl;
                        if ((rpl->pl_owner != IS->is_player.p_number) &&
                            (rpl->pl_owner != NO_OWNER))
                        {
                            if (rpl->pl_checkpoint[0] == '\0')
                            {
                                userP(IS, "Planet ", rpl, " is controlled by "
                                    "someone else\n");
                                aborting = TRUE;
                            }
                            else
                            {
                                if (!verifyCheckPoint(IS, rpl, cpt_land))
                                {
                                    aborting = TRUE;
                                }
                            }
                        }
                    }
                    else
                    {
                        err(IS, "ship is not orbiting any planet");
                        aborting = TRUE;
                    }
                }
                /* is everything still hunky dorey? */
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
                        doBeamDown(IS, it, percent, telepTF, askQuan);
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
                }
            }
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

/*
 * cmd_teleport - allows a player to teleport items up to/down from a ship
 */

BOOL cmd_teleport(IMP)
{
    USHORT what;

    /* find out what they want to do */
    if (reqChoice(IS, &what, "up\0down\0", "Teleport items (up/down)"))
    {
        (void) skipBlanks(IS);
        if (what == 0)
        {
            return beamUpItems(IS);
        }
        return beamDownItems(IS);
    }
    /* must have gotten some kind of error or blank line */
    return FALSE;
}

