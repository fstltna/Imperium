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
 * $Id: cmd_verify.c,v 1.3 2000/05/25 18:21:23 marisa Exp $
 *
 * Contains functions to try and verify the Imperium database
 *
 * $Log: cmd_verify.c,v $
 * Revision 1.3  2000/05/25 18:21:23  marisa
 * Fix more T/F issues
 *
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:52  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:24:28  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:18  marisag
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
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"

static const char rcsid[] = "$Id: cmd_verify.c,v 1.3 2000/05/25 18:21:23 marisa Exp $";


int min(int arg1, int arg2)
{
	if (arg1 < arg2)
	{
		return(arg1);
	}
	return(arg2);
}

/*
 * askAll - get a yes/no answer from the user. Set IS->is_BOOL1 TRUE if
 * they answer "a". Set IS->is_BOOL2 TRUE if they enter "q".
 */

BOOL askAll(IMP, char *question)
{
    register char ch;

    uPrompt(IS, question);
    if (clReadUser(IS))
    {
        ch = *skipBlanks(IS);
        IS->is_textInPos = &IS->is_textIn[0];
        IS->is_textIn[0] = '\0';
        switch(ch)
        {
            case 'y':
            case 'Y':
                return TRUE;
                break;
            case 'a':
            case 'A':
                log3(IS, "    Auto-TRUE selected", "", "");
                IS->is_BOOL1 = TRUE;
                return TRUE;
                break;
            case 'q':
            case 'Q':
                log3(IS, "    Quick mode selected", "", "");
                IS->is_BOOL2 = TRUE;
                return FALSE;
                break;
            default:
                return FALSE;
                break;
        }
    }
    return FALSE;
}


/*
 * user4 - write 3 strings to the user followed by a NL.
 */

void user4(IMP, char *m1, char *m2, char *m3, char *m4)
{
    user(IS, m1);
    user(IS, m2);
    user(IS, m3);
    user(IS, m4);
    userNL(IS);
}

/*
 * gotError() - We got an error. Report it to the local screen and
 * log it in the log. Then ask the user if they want to correct it if
 * IS->is_BOOL1 is FALSE, always correct it if it is TRUE
 */

BOOL gotError(IMP, register char *string1, register char *string2,
    register char *string3, register char *string4)
{
    log4(IS, string1, string2, string3, string4);
    user4(IS, string1, string2, string3, string4);
    /* if is_BOOL2 is TRUE, then the user wants to ignore all */
    /* errors, with no prompting */
    if (!IS->is_BOOL2)
    {
        /* if is_BOOL1 is TRUE, then the user wants to correct all */
        /* errors, with no prompting	*/
        if (!IS->is_BOOL1)
        {
            if (askAll(IS, "Fix it? "))
            {
                return TRUE;
            }
            return FALSE;
        }
        log3(IS, "    Selection made by Auto-TRUE", "", "");
        return TRUE;
    }
    log3(IS, "    Selection made by Quick-mode", "", "");
    return FALSE;
}

/*
 * removeShip() - removes a ship from the given fleet. Note that ship
 * number passed is NOT the actual ship number, but the field in the
 * fleet array to be removed.
 * This assumes that numInFleet <= FLEET_MAX and that ships are in a
 * contiguous block at the start of the array, and the calling
 * function will handle decrementing the ship count
 */

void removeShip(IMP, register ULONG shipNum, USHORT fleetNum,
    USHORT numInFleet)
{
    register Fleet_t *rf;

    rf = &IS->is_request.rq_u.ru_fleet;
    /* make sure ship is less than the highest in fleet */
    if (shipNum < numInFleet)
    {
        server(IS, rt_lockFleet, fleetNum);
        /* if shipNum is = numInFleet - 1 then we don't need to */
        /* do anything, as it is the last ship */
        if (shipNum != (numInFleet - 1))
        {
            while(shipNum < (numInFleet - 1))
            {
                rf->f_ship[shipNum] = rf->f_ship[shipNum + 1];
                shipNum++;
            }
        }
        server(IS, rt_unlockFleet, fleetNum);
    }
}

/*
 * verifyFleets() - verifies the correctness of the fleet database
 */

void verifyFleets(IMP)
{
    register Ship_t *rsh;
    register Fleet_t *rf;
    register Player_t *rp;
    register USHORT fleetNum;
    ULONG shipNum, countFleet;
    USHORT playerNum, lastPlayer;
    char numbuff1[25];
    char numbuff2[25];
    USHORT numShips, shipList, shipsLeft;
    ULONG foundships[FLEET_MAX];
    Fleet_t tempFleet;
    char fleetChar;
    BOOL aError, didAbort;

    /* Are there any fleets? */
    if (IS->is_world.w_fleetNext > 0)
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rf = &IS->is_request.rq_u.ru_fleet;
        rp = &IS->is_request.rq_u.ru_player;

        aError = FALSE;
        /* Check the fleet's number */
        for(fleetNum = 0; fleetNum < IS->is_world.w_fleetNext; fleetNum++)
        {
            userN2(IS, "\rChecking the internal number of fleet ", fleetNum);
            uFlush(IS);
            server(IS, rt_readFleet, fleetNum);
            if (rf->f_number != fleetNum)
            {
                /* oops, they aren't the same */
                userNL(IS);
                sprintf(&numbuff1[0], "%hd", fleetNum);
                if (gotError(IS, "### Fleet ", &numbuff1[0],
                    " has a bad ", "internal number - expect other problems"))
                {
                    server(IS, rt_lockFleet, fleetNum);
                    rf->f_number = fleetNum;
                    server(IS, rt_unlockFleet, fleetNum);
                    aError = TRUE;
                }
            }
            didAbort = clGotCtrlC(IS);
            if (didAbort)
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        didAbort = FALSE;
        /* Check the fleet ship counts	*/
        for(fleetNum = 0; fleetNum < IS->is_world.w_fleetNext; fleetNum++)
        {
            userN3(IS, "\rChecking fleet ", fleetNum, "'s shipcount");
            uFlush(IS);
            server(IS, rt_readFleet, fleetNum);
            if (rf->f_count > FLEET_MAX)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%hd", FLEET_MAX);
                sprintf(&numbuff2[0], "%hd", fleetNum);
                if (gotError(IS, "### Ship count was greater than ",
                    &numbuff1[0]," in fleet ", &numbuff2[0]))
                {
                    server(IS, rt_lockFleet, fleetNum);
                    rf->f_count = FLEET_MAX;
                    server(IS, rt_unlockFleet, fleetNum);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Check the ship numbers for validity */
        for(fleetNum = 0; fleetNum < IS->is_world.w_fleetNext; fleetNum++)
        {
            userN2(IS, "\rChecking ship numbers in fleet ", fleetNum);
            uFlush(IS);
            server(IS, rt_readFleet, fleetNum);
            if (rf->f_count > 0)
            {
                tempFleet = *rf;
                numShips = 0;
		if ((tempFleet.f_count - 1) < (FLEET_MAX - 1))
		{
                    shipNum = tempFleet.f_count - 1;
		}
		else
		{
                    shipNum = FLEET_MAX - 1;
		}
		if (tempFleet.f_count < FLEET_MAX)
		{
                    shipsLeft = tempFleet.f_count;
		}
		else
		{
                    shipsLeft = FLEET_MAX;
		}
                while((shipsLeft > 0) && !didAbort)
                {
                    /* make sure each ship listed is valid */
                    if (tempFleet.f_ship[shipNum] < IS->is_world.w_shipNext)
                    {
                        numShips++;
                    }
                    else
                    {
                        userNL(IS);
                        sprintf(&numbuff1[0], "%ld",
                            tempFleet.f_ship[shipNum]);
                        sprintf(&numbuff2[0], "%ld",
                            IS->is_world.w_shipNext - 1);
                        if (gotError(IS, "### Ship ", &numbuff1[0],
                            " is greater than ", &numbuff2[0]))
                        {
                            removeShip(IS, shipNum, fleetNum,
                                min(tempFleet.f_count, FLEET_MAX));
                            aError = TRUE;
                        }
                        else
                        {
                            numShips++;
                        }
                    }
                    shipNum--;
                    shipsLeft--;
                    didAbort = clGotCtrlC(IS);
                }
                if (didAbort)
                {
                    if (aError)
                    {
                        server(IS, rt_flush, 0);
                    }
                    return;
                }
                if (numShips != tempFleet.f_count)
                {
                    userNL(IS);
                    sprintf(&numbuff1[0], "%hd", fleetNum);
                    sprintf(&numbuff2[0], "%hd", numShips);
                    if (gotError(IS, "### Ship count in fleet ",
                        &numbuff1[0], " does not agree "
                        "with count found: ", &numbuff2[0]))
                    {
                        server(IS, rt_lockFleet, fleetNum);
                        rf->f_count = numShips;
                        server(IS, rt_unlockFleet, fleetNum);
                        aError = TRUE;
                    }
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Check for multiple entries for the same ship */
        for(fleetNum = 0; fleetNum < IS->is_world.w_fleetNext; fleetNum++)
        {
            userN2(IS, "\rChecking for duplicate ships in fleet ", fleetNum);
            uFlush(IS);
            server(IS, rt_readFleet, fleetNum);
            if (rf->f_count > 0)
            {
                tempFleet = *rf;
                numShips = 0;
                shipNum = min(tempFleet.f_count - 1, FLEET_MAX - 1);
                shipsLeft = min(tempFleet.f_count, FLEET_MAX);
                /* Go through the fleet */
                while((shipsLeft > 0) && !didAbort)
                {
                    /* If this is the first ship, simply add it */
                    if (numShips == 0)
                    {
                        foundships[numShips] = tempFleet.f_ship[shipNum];
                        numShips = 1;
                    }
                    else
                    {
                        /* since it isn't the first ship, see if it is */
                        /* already listed in the fleet */
                        shipList = 0;
                        while((shipList < numShips) &&
                            (tempFleet.f_ship[shipNum] !=
                            foundships[shipList]))
                        {
                            /* Didn't match, so go to next ship */
                            shipList++;
                        }
                        if (shipList == numShips)
                        {
                            /* at max, so must be new */
                            foundships[shipList] = tempFleet.f_ship[shipNum];
                            numShips++;
                        }
                        else
                        {
                            /* not at max, so we found the ship */
                            userNL(IS);
                            sprintf(&numbuff1[0], "%ld",
                                tempFleet.f_ship[shipNum]);
                            sprintf(&numbuff2[0], "%hd", fleetNum);
                            if (gotError(IS, "### Ship ",
                                &numbuff1[0],
                                " is listed more than once in fleet ",
                                &numbuff2[0]))
                            {
                                removeShip(IS, shipNum, fleetNum,
                                    min(tempFleet.f_count, FLEET_MAX));
                                aError = TRUE;
                            }
                            else
                            {
                                numShips++;
                            }
                        }
                    }
                    shipNum--;
                    shipsLeft--;
                    didAbort = clGotCtrlC(IS);
                }
                if (didAbort)
                {
                    if (aError)
                    {
                        server(IS, rt_flush, 0);
                    }
                    return;
                }
                if (numShips != tempFleet.f_count)
                {
                    userNL(IS);
                    sprintf(&numbuff1[0], "%hd", fleetNum);
                    sprintf(&numbuff2[0], "%hd", numShips);
                    if (gotError(IS, "### Ship count in fleet ",
                        &numbuff1[0], " does not agree "
                        "with count found: ", &numbuff2[0]))
                    {
                        server(IS, rt_lockFleet, fleetNum);
                        rf->f_count = numShips;
                        server(IS, rt_unlockFleet, fleetNum);
                        aError = TRUE;
                    }
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Check the owner of the fleet for validity */
        for(fleetNum = 0; fleetNum < IS->is_world.w_fleetNext; fleetNum++)
        {
            userN2(IS, "\rChecking the owner of fleet ", fleetNum);
            uFlush(IS);
            server(IS, rt_readFleet, fleetNum);
            if ((rf->f_owner != NO_OWNER) &&
                (rf->f_owner >= IS->is_world.w_currPlayers))
            {
                /* oops, they are bad */
                userNL(IS);
                sprintf(&numbuff1[0], "%hd", fleetNum);
                if (gotError(IS, "### Fleet ", &numbuff1[0],
                    " has a bad ", "owner"))
                {
                    server(IS, rt_lockFleet, fleetNum);
                    rf->f_owner = NO_OWNER;
                    server(IS, rt_unlockFleet, fleetNum);
                    aError = TRUE;
                }
            }
            didAbort = clGotCtrlC(IS);
            if (didAbort)
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Check to see if more than one country is using this fleet */
        /* number */
        for (fleetNum = 0; fleetNum < IS->is_world.w_fleetNext; fleetNum++)
        {
            userN2(IS, "\rChecking users of fleet ", fleetNum);
            uFlush(IS);
            /* lastPlayer holds the number of the first player that owns */
            /* a ship in this fleet */
            lastPlayer = NO_OWNER;
            /* Loop through all players */
            for (playerNum = 0; playerNum < IS->is_world.w_currPlayers;
                playerNum++)
            {
                server(IS, rt_readPlayer, playerNum);
                /* loop through this countries fleets */
                for (countFleet = 0; countFleet < 52; countFleet++)
                {
                    /* Check if this fleet number matches the fleet we */
                    /* are looking for */
                    if (rp->p_fleets[countFleet] == fleetNum)
                    {
                        /* it matched, so check if we have used this fleet */
                        /* before */
                        if (lastPlayer == NO_OWNER)
                        {
                            /* We haven't seen this fleet before, it's OK */
                            lastPlayer = playerNum;
                        }
                        else
                        {
                            /* We have seen this fleet, so check if we */
                            /* saw it in this player or not */
                            if (lastPlayer != playerNum)
                            {
                                /* 2 players are using the same fleet */
                                userNL(IS);
                                sprintf(&numbuff1[0], "%hd", playerNum);
                                sprintf(&numbuff2[0], "%hd", lastPlayer);
                                userN3(IS, "###   (", fleetNum, ")\n");
                                if (gotError(IS, "### Player ",
                                    &numbuff1[0],
                                    " is using the same fleet as player ",
                                    &numbuff2[0]))
                                {
                                    server(IS, rt_lockPlayer, playerNum);
                                    rp->p_fleets[countFleet] = 65535;
                                    server(IS, rt_unlockPlayer, playerNum);
                                    aError = TRUE;
                                }
                            }
                        }
                    }
                }
                if (clGotCtrlC(IS))
                {
                    if (aError)
                    {
                        server(IS, rt_flush, 0);
                    }
                    return;
                }
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Check for ships that are actually in another fleet or */
        /* in no fleet */
        for (fleetNum = 0; fleetNum < IS->is_world.w_fleetNext; fleetNum++)
        {
            userN3(IS, "\rChecking fleet ", fleetNum, " for ships that are "
                "actually in another fleet");
            uFlush(IS);
            server(IS, rt_readFleet, fleetNum);
            if (rf->f_count > 0)
            {
                numShips = 0;
                tempFleet = *rf;
                /* loop through all the ships in the fleet */
                for (shipNum = 0; shipNum < min(tempFleet.f_count,
                    FLEET_MAX); shipNum++)
                {
                    server(IS, rt_readShip, tempFleet.f_ship[shipNum]);
                    fleetChar = rsh->sh_fleet;
                    playerNum = rsh->sh_owner;
                    server(IS, rt_readPlayer, playerNum);
                    /* Check the players fleet and see if it matches */
                    if (rp->p_fleets[fleetPos(fleetChar)] != fleetNum)
                    {
                    /* Ships is in another fleet or none */
                        userNL(IS);
                        sprintf(&numbuff1[0], "%ld",
                            tempFleet.f_ship[shipNum]);
                        sprintf(&numbuff2[0], "%hd", fleetNum);
                        if (gotError(IS, "### Ship ",
                            &numbuff1[0], " is not really in fleet ",
                            &numbuff2[0]))
                        {
                            removeShip(IS, shipNum, fleetNum,
                                min(tempFleet.f_count, FLEET_MAX));
                            aError = TRUE;
                        }
                        else
                        {
                            numShips++;
                        }
                    }
                    else
                    {
                        numShips++;
                    }
                }
                if (numShips != tempFleet.f_count)
                {
                    userNL(IS);
                    sprintf(&numbuff1[0], "%hd", fleetNum);
                    sprintf(&numbuff2[0], "%hd", numShips);
                    if (gotError(IS, "### Ship count in fleet ",
                        &numbuff1[0], " does not agree "
                        "with count found: ", &numbuff2[0]))
                    {
                        server(IS, rt_lockFleet, fleetNum);
                        rf->f_count = numShips;
                        server(IS, rt_unlockFleet, fleetNum);
                        aError = TRUE;
                    }
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Make sure all the ships in the fleet belong to the same */
        /* owner.
            NOTE:
            This only removes the ships from the FLEET. If the ships
            themselves point to this fleet than you MUST verify the
            ship file or Imperium will almost certainly crash!
        */
        for (fleetNum = 0; fleetNum < IS->is_world.w_fleetNext; fleetNum++)
        {
            userN2(IS, "\rChecking the owner of ships in fleet ", fleetNum);
            uFlush(IS);
            server(IS, rt_readFleet, fleetNum);
            if (rf->f_count > 0)
            {
                lastPlayer = NO_OWNER;
                numShips = 0;
                tempFleet = *rf;
                for (shipNum = 0; shipNum < min(tempFleet.f_count,
                        FLEET_MAX); shipNum++)
                {
                    server(IS, rt_readShip, tempFleet.f_ship[shipNum]);
                    playerNum = rsh->sh_owner;
                    if (lastPlayer == NO_OWNER)
                    {
                        lastPlayer = playerNum;
                        numShips++;
                    }
                    else
                    {
                        if (lastPlayer != playerNum)
                        {
                            userNL(IS);
                            sprintf(&numbuff1[0], "%hd", playerNum);
                            sprintf(&numbuff2[0], "%hd", lastPlayer);
                            userN3(IS, "###   (", fleetNum, ")\n");
                            if (gotError(IS, "### Player ",
                                &numbuff1[0], " has a ship in the same fleet "
                                "as player ", &numbuff2[0]))
                            {
                                removeShip(IS, shipNum, fleetNum,
                                    min(tempFleet.f_count, FLEET_MAX));
                                aError = TRUE;
                            }
                            else
                            {
                                numShips++;
                            }
                        }
                        else
                        {
                            numShips++;
                        }
                    }
                }
                if (numShips != tempFleet.f_count)
                {
                    userNL(IS);
                    sprintf(&numbuff1[0], "%hd", fleetNum);
                    sprintf(&numbuff2[0], "%hd", numShips);
                    if (gotError(IS, "### Ship count in fleet ",
                        &numbuff1[0], " does not agree "
                        "with count found: ", &numbuff2[0]))
                    {
                        server(IS, rt_lockFleet, fleetNum);
                        rf->f_count = numShips;
                        server(IS, rt_unlockFleet, fleetNum);
                        aError = TRUE;
                    }
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }
    }
    else
    {
        user(IS, "No fleets yet\n");
    }
}

void verifyShips(IMP)
{
    register ULONG shipNum;
    register Ship_t *rsh;
    register Fleet_t *rf;
    register Player_t *rp;
    register USHORT fleetNum, playerNum;
    USHORT numFleet, checkShip;
    char fleetChar;
    BOOL gotIt, aError, didAbort;
    char numbuff1[25];
    char numbuff2[25];

    /* Are there any ships? */
    if (IS->is_world.w_shipNext > 0)
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rf = &IS->is_request.rq_u.ru_fleet;
        rp = &IS->is_request.rq_u.ru_player;

        aError = FALSE;
        /* Check the ships internal number field */
        for (shipNum = 0; shipNum < IS->is_world.w_shipNext; shipNum++)
        {
            userN3(IS, "\rChecking ship ", shipNum, "'s internal number "
                "field");
            uFlush(IS);
            server(IS, rt_readShip, shipNum);
            if (rsh->sh_number != shipNum)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", shipNum);
                if (gotError(IS, "### Ship ", &numbuff1[0],
                    " had a bad internal number - expect other problems",""))
                {
                    server(IS, rt_lockShip, shipNum);
                    rsh->sh_number = shipNum;
                    server(IS, rt_unlockShip, shipNum);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        didAbort = FALSE;
        /* Check the ship types */
        for (shipNum = 0; shipNum < IS->is_world.w_shipNext; shipNum++)
        {
            userN3(IS, "\rChecking ship ", shipNum, "'s type");
            uFlush(IS);
            server(IS, rt_readShip, shipNum);
            if (rsh->sh_type > ST_LAST)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", shipNum);
                if (gotError(IS, "### Ship ", &numbuff1[0],
                    " type was invalid",""))
                {
                    server(IS, rt_lockShip, shipNum);
                    /* want to make sure it can contain any cargo amount */
                    rsh->sh_type = ST_BIGGEST;
                    server(IS, rt_unlockShip, shipNum);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Check the ship owner field */
        for (shipNum = 0; shipNum < IS->is_world.w_shipNext; shipNum++)
        {
            userN3(IS, "\rChecking ship ", shipNum, "'s owner");
            uFlush(IS);
            server(IS, rt_readShip, shipNum);
            if ((rsh->sh_owner >= IS->is_world.w_currPlayers) &&
                (rsh->sh_owner != NO_OWNER))
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", shipNum);
                if (gotError(IS, "### Ship ", &numbuff1[0],
                    " owner was invalid",""))
                {
                    server(IS, rt_lockShip, shipNum);
                    rsh->sh_owner = NO_OWNER;
                    server(IS, rt_unlockShip, shipNum);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Check the ship fleet */
        for (shipNum = 0; shipNum < IS->is_world.w_shipNext; shipNum++)
        {
            userN3(IS, "\rChecking ship ", shipNum, "'s fleet");
            uFlush(IS);
            server(IS, rt_readShip, shipNum);
            /* is ship in a fleet? */
            if (rsh->sh_fleet != '*')
            {
                fleetChar = rsh->sh_fleet;
                playerNum = rsh->sh_owner;
                server(IS, rt_readPlayer, playerNum);
                fleetNum = rp->p_fleets[fleetPos(fleetChar)];
                /* check if player is even using that fleet */
                if (fleetNum >= IS->is_world.w_fleetNext)
                {
                    userNL(IS);
                    sprintf(&numbuff1[0], "%ld", shipNum);
                    sprintf(&numbuff2[0], "%hd", playerNum);
                    if (gotError(IS, "### Fleet listed in ship ",
                        &numbuff1[0], " is not in use by owner ",
                        &numbuff2[0]))
                    {
                        server(IS, rt_lockShip, shipNum);
                        rsh->sh_fleet = '*';
                        server(IS, rt_unlockShip, shipNum);
                        aError = TRUE;
                    }
                }
                else
                {
                    server(IS, rt_readFleet, fleetNum);
                    /* check if there are any ships in the fleet */
                    if (rf->f_count > 0)
                    {
                        numFleet = min(rf->f_count, FLEET_MAX);
                        checkShip = 0;
                        gotIt = FALSE;
                        while((checkShip < numFleet) && !gotIt && !didAbort)
                        {
                            if (rf->f_ship[checkShip] == shipNum)
                            {
                                gotIt = TRUE;
                            }
                            else
                            {
                                checkShip++;
                            }
                            didAbort = clGotCtrlC(IS);
                        }
                        if (didAbort)
                        {
                            server(IS, rt_flush, 0);
                            return;
                        }
                        if (!gotIt)
                        {
                            /* ship not listed in that fleet */
                            userNL(IS);
                            sprintf(&numbuff1[0], "%ld", shipNum);
                            sprintf(&numbuff2[0], "%hd", fleetNum);
                            if (gotError(IS, "### Ship ", &numbuff1[0],
                                " was not found in fleet ",
                                &numbuff2[0]))
                            {
                                server(IS, rt_lockShip, shipNum);
                                rsh->sh_fleet = '*';
                                server(IS, rt_unlockShip, shipNum);
                                aError = TRUE;
                            }
                        }
                    }
                    else
                    {
                        /* no ships listed in that fleet */
                        userNL(IS);
                        sprintf(&numbuff1[0], "%hd", fleetNum);
                        sprintf(&numbuff2[0], "%ld", shipNum);
                        if (gotError(IS, "### Fleet ", &numbuff1[0],
                            " is empty but is listed in ship ",
                            &numbuff2[0]))
                        {
                            server(IS, rt_lockShip, shipNum);
                            rsh->sh_fleet = '*';
                            server(IS, rt_unlockShip, shipNum);
                            aError = TRUE;
                        }
                    }
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }
    }
    else
    {
        user(IS, "There are no ships yet\n");
    }
}

void verifyPlanets(IMP)
{
    register ULONG planetNum;
    register Planet_t *rp;
    char numbuff1[25];
    BOOL aError;

    rp = &IS->is_request.rq_u.ru_planet;

    /*
        Because scanning all the planets is so slow, I am doing more
        than one test at a time. Hopefully this won't prevent the
        test from being as safe as the others...
    */

    aError = FALSE;
    for (planetNum = 0; planetNum < IS->is_world.w_planetNext; planetNum++)
    {
        userN2(IS, "\rChecking planet ", planetNum);
        uFlush(IS);
        server(IS, rt_readPlanet, planetNum);
        /* Check planet types */
        if (rp->pl_class > PC_LAST)
        {
            userNL(IS);
            sprintf(&numbuff1[0], "%ld", planetNum);
            if (gotError(IS, "### Type is invalid for planet ",
                &numbuff1[0], "", ""))
            {
                server(IS, rt_lockPlanet, planetNum);
                /* set default of a liveable planet */
                rp->pl_class = pc_M;
                server(IS, rt_unlockPlanet, planetNum);
                aError = TRUE;
            }
        }
        /* Check planet owner */
        if ((rp->pl_owner >= IS->is_world.w_maxPlayers) && (rp->pl_owner !=
            NO_OWNER))
        {
            userNL(IS);
            sprintf(&numbuff1[0], "%ld", planetNum);
            if (gotError(IS, "### Owner is invalid for planet ",
                &numbuff1[0], "", ""))
            {
                server(IS, rt_lockPlanet, planetNum);
                rp->pl_owner = NO_OWNER;
                server(IS, rt_unlockPlanet, planetNum);
                aError = TRUE;
            }
        }
        /* Check sector ship count */
        if (rp->pl_shipCount > 65000)
        {
            userNL(IS);
            sprintf(&numbuff1[0], "%ld", planetNum);
            if (gotError(IS, "### Ship count is less than zero for "
                "planet ", &numbuff1[0], "", ""))
            {
                server(IS, rt_lockPlanet, planetNum);
                rp->pl_shipCount = 0;
                server(IS, rt_unlockPlanet, planetNum);
                aError = TRUE;
            }
        }
        /* Check sector update time */
        if (rp->pl_lastUpdate > IS->is_request.rq_time)
        {
            userNL(IS);
            sprintf(&numbuff1[0], "%ld", planetNum);
            if (gotError(IS, "### Update time is in the future for "
                "planet ", &numbuff1[0], "", ""))
            {
                server(IS, rt_lockPlanet, planetNum);
                rp->pl_lastUpdate = IS->is_request.rq_time;
                server(IS, rt_unlockPlanet, planetNum);
                aError = TRUE;
            }
        }
        if (clGotCtrlC(IS))
        {
            if (aError)
            {
                server(IS, rt_flush, 0);
            }
            return;
        }
    }
    userNL(IS);
    if (aError)
    {
        server(IS, rt_flush, 0);
    }

}

void verifySectors(IMP)
{
    register USHORT row, col;
    register Sector_t *rs;
    register USHORT planet;
    USHORT mapped;
    char numbuff1[25];
    char numbuff2[25];
    BOOL aError;

    rs = &IS->is_request.rq_u.ru_sector;

    aError = FALSE;
    for (row = 0; row < IS->is_world.w_rows; row++)
    {
        for (col = 0; col < IS->is_world.w_columns; col++)
        {
            userS(IS, "\rChecking sector ", row, col, "     ");
            uFlush(IS);
            mapped = mapSector(IS, row, col);
            server(IS, rt_readSector, mapped);
            /* Check sector types */
            if (rs->s_type > S_LAST)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%hd", row);
                sprintf(&numbuff2[0], "%hd", col);
                if (gotError(IS, "### Type is invalid for sector ",
                    &numbuff1[0], ",", &numbuff2[0]))
                {
                    server(IS, rt_lockSector, mapped);
                    rs->s_type = s_unknown;
                    server(IS, rt_unlockSector, mapped);
                    aError = TRUE;
                }
            }
            /* Check sector ship count */
            if (rs->s_shipCount > 65000)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%hd", row);
                sprintf(&numbuff2[0], "%hd", col);
                if (gotError(IS, "### Ship count is less than zero for "
                    "sector ", &numbuff1[0], ",", &numbuff2[0]))
                {
                    server(IS, rt_lockSector, mapped);
                    rs->s_shipCount = 0;
                    server(IS, rt_unlockSector, mapped);
                    aError = TRUE;
                }
            }
            /* Check sector planet count */
            if (rs->s_planetCount >= PLANET_MAX)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%hd", row);
                sprintf(&numbuff2[0], "%hd", col);
                if (gotError(IS, "### Planet count is too large in "
                    "sector ", &numbuff1[0], ",", &numbuff2[0]))
                {
                    server(IS, rt_lockSector, mapped);
                    rs->s_planetCount = 0;
                    server(IS, rt_unlockSector, mapped);
                    aError = TRUE;
                }
            }
            /* make sure all the planet numbers are valid */
            for (planet = 0; planet < PLANET_MAX; planet++)
            {
                if ((rs->s_planet[planet] >= IS->is_world.w_planetNext) &&
                    (rs->s_planet[planet] != NO_ITEM))
                {
                    userNL(IS);
                    sprintf(&numbuff1[0], "%hd", row);
                    sprintf(&numbuff2[0], "%hd", col);
                    if (gotError(IS, "### Invalid planet number in sector ",
                        &numbuff1[0], ",", &numbuff2[0]))
                    {
                        server(IS, rt_lockSector, mapped);
                        rs->s_planet[planet] = NO_ITEM;
                        server(IS, rt_unlockSector, mapped);
                        aError = TRUE;
                    }
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
    }
    userNL(IS);
    if (aError)
    {
        server(IS, rt_flush, 0);
    }

}

void verifyOffers(IMP)
{
    register USHORT i;
    register Offer_t *ro;
    register Planet_t *rpl;
    register Ship_t *rsh;
    Offer_t tempOff;
    ULONG topEnd;
    char numbuff1[25];
    char numbuff2[25];
    BOOL aError;

    ro = &IS->is_request.rq_u.ru_offer;
    rpl = &IS->is_request.rq_u.ru_planet;
    rsh = &IS->is_request.rq_u.ru_ship;

    aError = FALSE;
    if (IS->is_world.w_offerNext != 0)
    {
        /* Check what is being offered */
        for (i = 0; i <  IS->is_world.w_offerNext; i++)
        {
            userN2(IS, "\rChecking type of sale in offer ", i);
            uFlush(IS);
            server(IS, rt_readOffer, i);
            if (ro->of_state > of_none)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%hd", i);
                if (gotError(IS, "### Invalid type of sale for offer ",
                    &numbuff1[0], "", ""))
                {
                    server(IS, rt_lockOffer, i);
                    ro->of_state = of_none;
                    server(IS, rt_unlockOffer, i);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Check who is offering it */
        topEnd = IS->is_world.w_maxPlayers - 1;
        for (i = 0; i < IS->is_world.w_offerNext; i++)
        {
            userN2(IS, "\rChecking seller in offer ", i);
            uFlush(IS);
            server(IS, rt_readOffer, i);
            if (ro->of_who > topEnd)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%u", i);
                if (gotError(IS, "### Invalid seller for offer ",
                    &numbuff1[0], "", ""))
                {
                    server(IS, rt_lockOffer, i);
                    ro->of_who = 0;
                    ro->of_state = of_none;
                    server(IS, rt_unlockOffer, i);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* If offer is ship, verify the number */
        topEnd = IS->is_world.w_shipNext - 1;
        for (i = 0; i < IS->is_world.w_offerNext; i++)
        {
            userN2(IS, "\rVerifying ship number (if ship) in offer ", i);
            uFlush(IS);
            server(IS, rt_readOffer, i);
            /* Check if offer is a ship */
            if (ro->of_state == of_ship)
            {
                if (ro->of_.of_shipNumber > topEnd)
                {
                    userNL(IS);
                    sprintf(&numbuff1[0], "%u", i);
                    if (gotError(IS, "### Invalid ship number for offer ",
                        &numbuff1[0], "", ""))
                    {
                        server(IS, rt_lockOffer, i);
                        ro->of_.of_shipNumber = 0;
                        ro->of_state = of_none;
                        server(IS, rt_unlockOffer, i);
                        aError = TRUE;
                    }
                }
                else
                {
                    tempOff = *ro;
                    server(IS, rt_readShip, tempOff.of_.of_shipNumber);
                    if (rsh->sh_fleet != '*')
                    {
                        userNL(IS);
                        sprintf(&numbuff2[0], "%ld",
                            ro->of_.of_shipNumber);
                        sprintf(&numbuff1[0], "%u", i);
                        if (gotError(IS, "### Ship ",
                            &numbuff2[0], "is in a fleet, but is also in "
                            "offer ", &numbuff1[0]))
                        {
                            server(IS, rt_lockOffer, i);
                            ro->of_.of_shipNumber = 0;
                            ro->of_state = of_none;
                            server(IS, rt_unlockOffer, i);
                            aError = TRUE;
                        }
                    }
                    else if (rsh->sh_owner != tempOff.of_who)
                    {
                        userNL(IS);
                        sprintf(&numbuff2[0], "%lu",
                            ro->of_.of_shipNumber);
                        sprintf(&numbuff1[0], "%u", i);
                        if (gotError(IS, "### Ship ", &numbuff2[0],
                            "is not owned by player making offer ",
                            &numbuff1[0]))
                        {
                            server(IS, rt_lockOffer, i);
                            ro->of_.of_shipNumber = 0;
                            ro->of_state = of_none;
                            server(IS, rt_unlockOffer, i);
                            aError = TRUE;
                        }
                    }
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* If offer is a planet, verify other stats */
        for (i = 0; i < IS->is_world.w_offerNext; i++)
        {
            userN2(IS, "\rVerifying stats (if planet) in offer ", i);
            uFlush(IS);
            server(IS, rt_readOffer, i);
            /* Check if offer is a planet */
            if (ro->of_state == of_planet)
            {
                if (ro->of_.of_plan.of_planetNumber >=
                    IS->is_world.w_planetNext)
                {
                    userNL(IS);
                    sprintf(&numbuff1[0], "%d", i);
                    if (gotError(IS, "### Planet number is greater than "
                        "maximum in offer ", &numbuff1[0], "", ""))
                    {
                        server(IS, rt_lockOffer, i);
                        ro->of_.of_plan.of_planetNumber = 0;
                        ro->of_state = of_none;
                        server(IS, rt_unlockOffer, i);
                        aError = TRUE;
                    }
                }
                else
                {
                    if ((ro->of_.of_plan.of_payor != of_planOwn) &&
                        (ro->of_.of_plan.of_payor != of_shipOwn))
                    {
                        userNL(IS);
                        sprintf(&numbuff1[0], "%d", i);
                        if (gotError(IS, "### Damaged goods payor is "
                            "invalid in offer ", &numbuff1[0], "", ""))
                        {
                            server(IS, rt_lockOffer, i);
                            ro->of_.of_plan.of_payor = of_shipOwn;
                            server(IS, rt_unlockOffer, i);
                            aError = TRUE;
                        }
                    }
                    tempOff = *ro;
                    server(IS, rt_readPlanet,
                        tempOff.of_.of_plan.of_planetNumber);
                    if (rpl->pl_owner != tempOff.of_who)
                    {
                        userNL(IS);
                        sprintf(&numbuff1[0], "%d", rpl->pl_owner);
                        sprintf(&numbuff2[0], "%d", i);
                        if (gotError(IS, "### Owner of planet is ",
                            &numbuff1[0],
                            ", which is not the person making offer ",
                            &numbuff2[0]))
                        {
                            server(IS, rt_lockOffer, i);
                            ro->of_state = of_none;
                            server(IS, rt_unlockOffer, i);
                            aError = TRUE;
                        }
                    }
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* If offer is an item, verify other stats */
        for (i = 0; i < IS->is_world.w_offerNext; i++)
        {
            userN2(IS, "\rVerifying stats (if item) in offer ", i);
            uFlush(IS);
            server(IS, rt_readOffer, i);
            /* Check if offer is an item */
            if (ro->of_state == of_item)
            {
                if (ro->of_.of_itemNumber >=
                    IS->is_world.w_bigItemNext)
                {
                    userNL(IS);
                    sprintf(&numbuff1[0], "%d", i);
                    if (gotError(IS, "### Item number is greater than "
                        "maximum in offer ", &numbuff1[0], "", ""))
                    {
                        server(IS, rt_lockOffer, i);
                        ro->of_.of_itemNumber = 0;
                        ro->of_state = of_none;
                        server(IS, rt_unlockOffer, i);
                        aError = TRUE;
                    }
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }
    }
    else
    {
        user(IS, "No offers yet\n");
    }
}

void verifyLoans(IMP)
{
    register ULONG i;
    register Loan_t *rl;
    char numbuff1[25];
    BOOL aError;

    rl = &IS->is_request.rq_u.ru_loan;
    if (IS->is_world.w_loanNext != 0)
    {
        aError = FALSE;
        /* Verify the loan's internal number */
        for (i = 0; i < IS->is_world.w_loanNext; i++)
        {
            userN2(IS, "\rChecking the internal number of loan ", i);
            uFlush(IS);
            server(IS, rt_readLoan, i);
            if (rl->l_number != i)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", i);
                if (gotError(IS, "### The internal number for loan ",
                    &numbuff1[0], " is bad", " - expect more problems"))
                {
                    server(IS, rt_lockLoan, i);
                    rl->l_number = i;
                    server(IS, rt_unlockLoan, i);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Verify the loan states */
        for (i = 0; i < IS->is_world.w_loanNext; i++)
        {
            userN2(IS, "\rChecking the state of loan ", i);
            uFlush(IS);
            server(IS, rt_readLoan, i);
            if (rl->l_state > l_paidUp)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", i);
                if (gotError(IS, "### Invalid state for loan ",
                    &numbuff1[0], "", ""))
                {
                    server(IS, rt_lockLoan, i);
                    rl->l_state = l_paidUp;
                    server(IS, rt_unlockLoan, i);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Verify the loaner */
        for (i = 0; i < IS->is_world.w_loanNext; i++)
        {
            userN2(IS, "\rChecking the loaner of loan ", i);
            uFlush(IS);
            server(IS, rt_readLoan, i);
            if (rl->l_loaner > IS->is_world.w_maxPlayers - 1)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", i);
                if (gotError(IS, "### Loaner is greater than the highest "
                    "player in loan ", &numbuff1[0], "", ""))
                {
                    server(IS, rt_lockLoan, i);
                    rl->l_loaner = 0;
                    rl->l_state = l_paidUp;
                    server(IS, rt_unlockLoan, i);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                server(IS, rt_flush, 0);
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Verify the loanee */
        for (i = 0; i < IS->is_world.w_loanNext; i++)
        {
            userN2(IS, "\rChecking the loanee of loan ", i);
            uFlush(IS);
            server(IS, rt_readLoan, i);
            if (rl->l_loanee > IS->is_world.w_maxPlayers - 1)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", i);
                if (gotError(IS, "### Loanee is greater than the highest "
                    "player in loan ", &numbuff1[0], "", ""))
                {
                    server(IS, rt_lockLoan, i);
                    rl->l_loanee = 0;
                    rl->l_state = l_paidUp;
                    server(IS, rt_unlockLoan, i);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Verify the loan amount */
        for (i = 0; i < IS->is_world.w_loanNext; i++)
        {
            userN2(IS, "\rChecking the amount of loan ", i);
            uFlush(IS);
            server(IS, rt_readLoan, i);
            if ((rl->l_amount == 0) && (rl->l_state != l_paidUp))
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", i);
                if (gotError(IS, "### Amount left in loan ", &numbuff1[0],
                    " is zero, but not marked as paid", ""))
                {
                    server(IS, rt_lockLoan, i);
                    rl->l_state = l_paidUp;
                    server(IS, rt_unlockLoan, i);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }
    }
    else
    {
        user(IS, "No loans yet\n");
    }
}

void verifyBigItems(IMP)
{
    register BigItem_t *rbi;
    register ULONG i;
    char numbuff1[25];
    BOOL aError;

    rbi = &IS->is_request.rq_u.ru_bigItem;
    if (IS->is_world.w_bigItemNext != 0)
    {
        aError = FALSE;
        /* Verify the big item internal number */
        for (i = 0; i < IS->is_world.w_bigItemNext; i++)
        {
            userN2(IS, "\rChecking the internal number of big item ", i);
            uFlush(IS);
            server(IS, rt_readBigItem, i);
            if (rbi->bi_number != i)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", i);
                if (gotError(IS, "### Incorrect internal number for big "
                    "item ", &numbuff1[0], " - expect more problems", ""))
                {
                    server(IS, rt_lockBigItem, i);
                    rbi->bi_number = i;
                    server(IS, rt_unlockBigItem, i);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Verify the big item state */
        for (i = 0; i < IS->is_world.w_bigItemNext; i++)
        {
            userN2(IS, "\rChecking the status of big item ", i);
            uFlush(IS);
            server(IS, rt_readBigItem, i);
            if (rbi->bi_status > bi_forSale)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", i);
                if (gotError(IS, "### Invalid status for big item ",
                    &numbuff1[0], "", ""))
                {
                    server(IS, rt_lockBigItem, i);
                    rbi->bi_status = bi_destroyed;
                    server(IS, rt_unlockBigItem, i);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Verify the part type */
        for (i = 0; i < IS->is_world.w_bigItemNext; i++)
        {
            userN2(IS, "\rChecking the part type of big item ", i);
            uFlush(IS);
            server(IS, rt_readBigItem, i);
            if (rbi->bi_part > BP_LAST)
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", i);
                if (gotError(IS, "### Invalid part type for big item ",
                    &numbuff1[0], "", ""))
                {
                    server(IS, rt_lockBigItem, i);
                    rbi->bi_part = bp_computer;
                    rbi->bi_status = bi_destroyed;
                    server(IS, rt_unlockBigItem, i);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Verify the efficiency */
        for (i = 0; i < IS->is_world.w_bigItemNext; i++)
        {
            userN2(IS, "\rChecking the efficiency of big item ", i);
            uFlush(IS);
            server(IS, rt_readBigItem, i);
            if ((((short)rbi->bi_effic) > 100) ||
                (((short)rbi->bi_effic) < -100))
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", i);
                if (gotError(IS, "### Invalid efficiency for big item ",
                    &numbuff1[0], "", ""))
                {
                    server(IS, rt_lockBigItem, i);
                    rbi->bi_effic = 0;
                    server(IS, rt_unlockBigItem, i);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Verify the location */
        for (i = 0; i < IS->is_world.w_bigItemNext; i++)
        {
            userN2(IS, "\rChecking the location of big item ", i);
            uFlush(IS);
            server(IS, rt_readBigItem, i);
            if (rbi->bi_onShip &&
                (rbi->bi_itemLoc > (IS->is_world.w_shipNext - 1)) &&
                (rbi->bi_status != bi_destroyed))
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", i);
                if (gotError(IS, "### Big item ", &numbuff1[0], " has an",
                    " invalid ship number"))
                {
                    server(IS, rt_lockBigItem, i);
                    rbi->bi_itemLoc = 0;
                    rbi->bi_onShip = FALSE;
                    rbi->bi_status = bi_destroyed;
                    server(IS, rt_unlockBigItem, i);
                    aError = TRUE;
                }
            }
            else
            {
                if ((rbi->bi_itemLoc > (IS->is_world.w_planetNext - 1)) &&
                    (rbi->bi_status != bi_destroyed))
                {
                    userNL(IS);
                    sprintf(&numbuff1[0], "%ld", i);
                    if (gotError(IS, "### Big item ", &numbuff1[0],
                        " has an", " invalid planet number"))
                    {
                        server(IS, rt_lockBigItem, i);
                        rbi->bi_itemLoc = 0;
                        rbi->bi_status = bi_destroyed;
                        server(IS, rt_unlockBigItem, i);
                        aError = TRUE;
                    }
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }

        aError = FALSE;
        /* Verify the price */
        for (i = 0; i < IS->is_world.w_bigItemNext; i++)
        {
            userN2(IS, "\rChecking the price of big item ", i);
            uFlush(IS);
            server(IS, rt_readBigItem, i);
            if ((rbi->bi_price != 0) && (rbi->bi_status != bi_forSale))
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%ld", i);
                if (gotError(IS, "### Big item ", &numbuff1[0],
                    " has a price listed but is ",
                    " not marked as being for sale"))
                {
                    server(IS, rt_lockBigItem, i);
                    rbi->bi_price = 0;
                    server(IS, rt_unlockBigItem, i);
                    aError = TRUE;
                }
            }
            if (clGotCtrlC(IS))
            {
                if (aError)
                {
                    server(IS, rt_flush, 0);
                }
                return;
            }
        }
        userNL(IS);
        if (aError)
        {
            server(IS, rt_flush, 0);
        }
    }
    else
    {
        user(IS, "No big items yet\n");
    }
}

void verifyPlayers(IMP)
{
    register Player_t *rp;
    register USHORT i;
    char numbuff1[25];
    char numbuff2[25];
    USHORT fleetNum, checkFleetNum;
    BOOL aError;

    rp = &IS->is_request.rq_u.ru_player;

    aError = FALSE;
    /* Check the players internal number */
    for (i = 0; i < IS->is_world.w_maxPlayers; i++)
    {
        userN3(IS, "\rChecking player ", i, "'s internal number");
        uFlush(IS);
        server(IS, rt_readPlayer, i);
        /* Verify the status is reasonable */
        if (rp->p_number != i)
        {
            userNL(IS);
            sprintf(&numbuff1[0], "%hd", i);
            if (gotError(IS, "### The the internal number is bad for "
                "player ", &numbuff1[0], " - expect more problems", ""))
            {
                server(IS, rt_lockPlayer, i);
                rp->p_number = i;
                server(IS, rt_unlockPlayer, i);
                aError = TRUE;
            }
        }
        if (clGotCtrlC(IS))
        {
            if (aError)
            {
                server(IS, rt_flush, 0);
            }
            return;
        }
    }
    userNL(IS);
    if (aError)
    {
        server(IS, rt_flush, 0);
    }

    aError = FALSE;
    /* Check the player status */
    for (i = 0; i < IS->is_world.w_maxPlayers; i++)
    {
        userN3(IS, "\rChecking player ", i, "'s status");
        uFlush(IS);
        server(IS, rt_readPlayer, i);
        /* Verify the status is reasonable */
        if (rp->p_status > ps_idle)
        {
            userNL(IS);
            sprintf(&numbuff1[0], "%hd", i);
            if (gotError(IS, "### The the status is bad for player ",
                &numbuff1[0], "", ""))
            {
                server(IS, rt_lockPlayer, i);
                rp->p_status = ps_idle;
                server(IS, rt_unlockPlayer, i);
                aError = TRUE;
            }
        }
        if (clGotCtrlC(IS))
        {
            if (aError)
            {
                server(IS, rt_flush, 0);
            }
            return;
        }
    }
    userNL(IS);
    if (aError)
    {
        server(IS, rt_flush, 0);
    }

    aError = FALSE;
    /* Check the player race */
    for (i = 0; i < IS->is_world.w_maxPlayers; i++)
    {
        userN3(IS, "\rChecking player ", i, "'s race");
        uFlush(IS);
        server(IS, rt_readPlayer, i);
        /* Verify the status is reasonable */
        if ((rp->p_race >= RACE_MAX) && (rp->p_race != NO_RACE))
        {
            userNL(IS);
            sprintf(&numbuff1[0], "%hd", i);
            if (gotError(IS, "### Bad race for player ",
                &numbuff1[0], "", ""))
            {
                server(IS, rt_lockPlayer, i);
                if (rp->p_status == ps_deity)
                {
                    /* set no race for deities */
                    rp->p_race = NO_RACE;
                }
                else
                {
                    /* set race 0 for "normal" people */
                    rp->p_race = 0;
                }
                server(IS, rt_unlockPlayer, i);
                aError = TRUE;
            }
        }
        if (clGotCtrlC(IS))
        {
            if (aError)
            {
                server(IS, rt_flush, 0);
            }
            return;
        }
    }
    userNL(IS);
    if (aError)
    {
        server(IS, rt_flush, 0);
    }

    aError = FALSE;
    /* Check the players fleets for duplicates */
    for (i = 0; i < IS->is_world.w_maxPlayers; i++)
    {
        userN3(IS, "\rChecking player ", i, "'s fleets for duplicates");
        uFlush(IS);
        server(IS, rt_readPlayer, i);
        /* scan the countries fleets for duplicates */
        for (fleetNum = 0; fleetNum < 51; fleetNum++)
        {
            /* only need to scan forward, since there should be no dups */
            /* behind us */
            for (checkFleetNum = fleetNum + 1; checkFleetNum < 52;
                checkFleetNum++)
            {
                if ((rp->p_fleets[fleetNum] == rp->p_fleets[checkFleetNum]) &&
                    (rp->p_fleets[fleetNum] < IS->is_world.w_fleetNext))
                {
                    userNL(IS);
                    sprintf(&numbuff1[0], "%hd", i);
                    if (gotError(IS, "### 2 fleets match in player ",
                        &numbuff1[0], "", ""))
                    {
                        server(IS, rt_lockPlayer, i);
                        rp->p_fleets[checkFleetNum] = 65535;
                        server(IS, rt_unlockPlayer, i);
                        aError = TRUE;
                    }
                }
            }
        }
        if (clGotCtrlC(IS))
        {
            server(IS, rt_flush, 0);
            return;
        }
    }
    userNL(IS);
    if (aError)
    {
        server(IS, rt_flush, 0);
    }

    aError = FALSE;
    /* Check the countries fleets */
    for (i = 0; i < IS->is_world.w_maxPlayers; i++)
    {
        userN3(IS, "\rChecking player ", i, "'s fleets ");
        uFlush(IS);
        server(IS, rt_readPlayer, i);
        /* scan the players fleets for correctness */
        for (fleetNum = 0; fleetNum < 52; fleetNum++)
        {
            if ((rp->p_fleets[fleetNum] >= IS->is_world.w_fleetNext) &&
                (rp->p_fleets[fleetNum] != 65535))
            {
                userNL(IS);
                sprintf(&numbuff1[0], "%hd", fleetNum);
                sprintf(&numbuff2[0], "%hd", i);
                if (gotError(IS, "### Fleet ", &numbuff1[0],
                    " is invalid in player ", &numbuff2[0]))
                {
                    server(IS, rt_lockPlayer, i);
                    rp->p_fleets[fleetNum] = 65535;
                    server(IS, rt_unlockPlayer, i);
                    aError = TRUE;
                }
            }
        }
        if (clGotCtrlC(IS))
        {
            server(IS, rt_flush, 0);
            return;
        }
    }
    userNL(IS);
    if (aError)
    {
        server(IS, rt_flush, 0);
    }

    aError = FALSE;
    /* Check the countries flags */
    for (i = 0; i < IS->is_world.w_maxPlayers; i++)
    {
        userN3(IS, "\rChecking player ", i, "'s flags");
        uFlush(IS);
        server(IS, rt_readPlayer, i);
        /* Verify the flags are reasonable */
        if (rp->p_loggedOn && (i != IS->is_player.p_number))
        {
            userNL(IS);
            sprintf(&numbuff1[0], "%hd", i);
            if (gotError(IS, "### The loggedOn flag is bad for player ",
                &numbuff1[0], "", ""))
            {
                server(IS, rt_lockPlayer, i);
                rp->p_loggedOn = FALSE;
                server(IS, rt_unlockPlayer, i);
                aError = TRUE;
            }
        }
        if  /* Chat should be FALSE, as we should be the only ones */
            /* online, and we aren't chatting */
            (rp->p_inChat)
        {
            userNL(IS);
            sprintf(&numbuff1[0], "%hd", i);
            if (gotError(IS, "### The inChat flag is bad for player ",
                &numbuff1[0], "", ""))
            {
                server(IS, rt_lockPlayer, i);
                rp->p_inChat = FALSE;
                server(IS, rt_unlockPlayer, i);
                aError = TRUE;
            }
        }
        if (clGotCtrlC(IS))
        {
            server(IS, rt_flush, 0);
            return;
        }
    }
    userNL(IS);
    if (aError)
    {
        server(IS, rt_flush, 0);
    }

    aError = FALSE;
    /* Check the notify type */
    for (i = 0; i < IS->is_world.w_maxPlayers; i++)
    {
        userN3(IS, "\rChecking player ", i, "'s notify type");
        uFlush(IS);
        server(IS, rt_readPlayer, i);
        /* Verify the notify type is reasonable */
        if (rp->p_notify > nt_both)
        {
            userNL(IS);
            sprintf(&numbuff1[0], "%hd", i);
            if (gotError(IS, "### The the notify type is bad for player ",
                &numbuff1[0], "", ""))
            {
                server(IS, rt_lockPlayer, i);
                rp->p_notify = nt_message;
                server(IS, rt_unlockPlayer, i);
                aError = TRUE;
            }
        }
        if (clGotCtrlC(IS))
        {
            server(IS, rt_flush, 0);
            return;
        }
    }
    userNL(IS);
    if (aError)
    {
        server(IS, rt_flush, 0);
    }

    /* Check the planet counts */
    for (i = 0; i < IS->is_world.w_maxPlayers; i++)
    {
        userN3(IS, "\rChecking player ", i, "'s planet count");
        uFlush(IS);
        server(IS, rt_readPlayer, i);
        /* Verify the sector count is reasonable */
        if ((rp->p_planetCount > 8000) && (rp->p_status != ps_deity))
        {
            userNL(IS);
            sprintf(&numbuff1[0], "%hd", i);
            if (gotError(IS, "### The the planet count for player ",
                &numbuff1[0], " seems quite unlikely. Please "
                "check it by hand", ""))
            {
                user(IS, "I can't fix this one. Human decision is needed.\n");
            }
        }
            if (clGotCtrlC(IS))
            {
                return;
            }
    }
    userNL(IS);
}

void verifyWorld(IMP)
{
    register World_t *rw;
    char numbuff1[25];
    char numbuff2[25];
    BOOL aError;

    rw = &IS->is_request.rq_u.ru_world;
    server(IS, rt_readWorld, 0);
    aError = FALSE;
    /* Verify that the players in use are reasonable */
    user(IS, "Checking the current player count\n");
    if (rw->w_currPlayers > rw->w_maxPlayers)
    {
        sprintf(&numbuff1[0], "%hd", rw->w_currPlayers);
        sprintf(&numbuff2[0], "%hd", rw->w_maxPlayers);
        if (gotError(IS, "### Current players (", &numbuff1[0],
            ") is greater than maximum: ", &numbuff2[0]))
        {
            server(IS, rt_lockWorld, 0);
            rw->w_currPlayers = rw->w_maxPlayers;
            server(IS, rt_unlockWorld, 0);
            server(IS, rt_readWorld, 0);
            aError = TRUE;
        }
    }

    /* w_doingPower SHOULD be FALSE, since you aren't supposed to verify */
    /* the databases while another country is logged in */
    if (rw->w_doingPower)
    {
        if (gotError(IS, "### The doingPower flag is not FALSE", "", "",
            ""))
        {
            server(IS, rt_lockWorld, 0);
            rw->w_doingPower = FALSE;
            server(IS, rt_unlockWorld, 0);
            aError = TRUE;
        }
    }
    if (aError)
    {
        server(IS, rt_flush, 0);
    }
}

/*
 * verifyLimits() - makes sure the maximum values are correct
 * before any other tests are run
 */

void verifyLimits(IMP)
{
    register USHORT i;

    /* Try and read in all the fleets */
    if (IS->is_world.w_fleetNext != 0)
    {
        for(i = 0; i < IS->is_world.w_fleetNext; i++)
        {
            userN2(IS, "\rAttempting to read fleet ", i);
            uFlush(IS);
            server(IS, rt_readFleet, i);
            if (clGotCtrlC(IS))
            {
                server(IS, rt_flush, 0);
                return;
            }
        }
        userNL(IS);
    }
    else
    {
        user(IS, "No fleets yet\n");
    }

    /* Try and read in all the ships */
    if (IS->is_world.w_shipNext != 0)
    {
        for(i = 0; i < IS->is_world.w_shipNext; i++)
        {
            userN2(IS, "\rAttempting to read ship ", i);
            uFlush(IS);
            server(IS, rt_readShip, i);
            if (clGotCtrlC(IS))
            {
                server(IS, rt_flush, 0);
                return;
            }
        }
        userNL(IS);
    }
    else
    {
        user(IS, "No ships yet\n");
    }

    /* Try and read in all the planets */
    if (IS->is_world.w_planetNext != 0)
    {
        for(i = 0; i < IS->is_world.w_planetNext; i++)
        {
            userN2(IS, "\rAttempting to read planet ", i);
            uFlush(IS);
            server(IS, rt_readPlanet, i);
            if (clGotCtrlC(IS))
            {
                server(IS, rt_flush, 0);
                return;
            }
        }
        userNL(IS);
    }
    else
    {
        /* I would like to see it ever get here! */
        user(IS, "No planets yet\n");
    }

    /* Try and read in all the loans */
    if (IS->is_world.w_loanNext != 0)
    {
        for(i = 0; i < IS->is_world.w_loanNext; i++)
        {
            userN2(IS, "\rAttempting to read loan ", i);
            uFlush(IS);
            server(IS, rt_readLoan, i);
            if (clGotCtrlC(IS))
            {
                server(IS, rt_flush, 0);
                return;
            }
        }
        userNL(IS);
    }
    else
    {
        user(IS, "No loans yet\n");
    }

    /* Try and read in all the offers */
    if (IS->is_world.w_offerNext != 0)
    {
        for(i = 0; i < IS->is_world.w_offerNext; i++)
        {
            userN2(IS, "\rAttempting to read offer ", i);
            uFlush(IS);
            server(IS, rt_readOffer, i);
            if (clGotCtrlC(IS))
            {
                server(IS, rt_flush, 0);
                return;
            }
        }
        userNL(IS);
    }
    else
    {
        user(IS, "No offers yet\n");
    }

    /* Try and read in all the big items */
    if (IS->is_world.w_bigItemNext != 0)
    {
        for(i = 0; i < IS->is_world.w_bigItemNext; i++)
        {
            userN2(IS, "\rAttempting to read big item ", i);
            uFlush(IS);
            server(IS, rt_readBigItem, i);
            if (clGotCtrlC(IS))
            {
                server(IS, rt_flush, 0);
                return;
            }
        }
        userNL(IS);
    }
    else
    {
        user(IS, "No big items yet\n");
    }

    /* Try and read in all the players */
    if (IS->is_world.w_maxPlayers != 0)
    {
        for(i = 0; i < IS->is_world.w_maxPlayers; i++)
        {
            userN2(IS, "\rAttempting to read player ", i);
            uFlush(IS);
            server(IS, rt_readPlayer, i);
            if (clGotCtrlC(IS))
            {
                server(IS, rt_flush, 0);
                return;
            }
        }
        userNL(IS);
    }
    else
    {
        /* This line SHOULD be impossible to get! */
        user(IS, "No players yet\n");
    }

}

/*
 * doVerify() - Does the verification of the various items
 */

void doVerify(IMP)
{
    char *prompt;
    USHORT what;

    prompt = "Verify what (fleet/ship/plan/sect/loan/offer/play/items"
        "/world/limit/all)";
    IS->is_BOOL1 = FALSE;
    IS->is_BOOL2 = FALSE;
    if (reqChoice(IS, &what,
        "fleets\0"
        "ships\0"
        "planets\0"
        "sectors\0"
        "loans\0"
        "offers\0"
        "players\0"
        "world\0"
        "limit\0"
        "all\0"
        "items\0",
        prompt))
    {
        log3(IS, "--> Starting verify run", "", "");
        (void) skipBlanks(IS);
        switch(what)
        {
            case 0:
                verifyFleets(IS);
                break;
            case 1:
                verifyShips(IS);
                break;
            case 2:
                verifyPlanets(IS);
                break;
            case 3:
                verifySectors(IS);
                break;
            case 4:
                verifyLoans(IS);
                break;
            case 5:
                verifyOffers(IS);
                break;
            case 6:
                verifyPlayers(IS);
                break;
            case 7:
                verifyWorld(IS);
                break;
            case 8:
                verifyLimits(IS);
                break;
            case 10:
                verifyBigItems(IS);
                break;
            case 9:
                verifyLimits(IS);
                verifyWorld(IS);
                verifySectors(IS);
                verifyPlayers(IS);
                verifyBigItems(IS);
                verifyFleets(IS);
                verifyShips(IS);
                verifyPlanets(IS);
                verifyLoans(IS);
                verifyOffers(IS);
                break;
            default:
                user(IS, "Feature not yet supported\n");
                break;
        }
        log3(IS, "<-- Verify run complete", "", "");
    }
}

/*
 * cmd_verify() - Allows a deity to verify the Imperium database for
 *          correctness.
 *          *** RTFM!!! AND USE AT YOUR OWN RISK! ***
 */

void cmd_verify(IMP)
{
    if (IS->is_player.p_status == ps_deity)
    {
        doVerify(IS);
        /* Reset these back */
        IS->is_BOOL1 = FALSE;
        IS->is_BOOL2 = FALSE;
    }
    else
    {
        err(IS, "Only a deity can use this command");
    }
}
