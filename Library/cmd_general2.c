/*
 * Imperium
 *
 * $Id: cmd_general2.c,v 1.2 2000/05/18 06:50:02 marisa Exp $
 *
 * Feel free to modify and use these sources however you wish, so long
 * as you preserve this copyright notice.
 *
 * $Log: cmd_general2.c,v $
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:49  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:59:08  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:24:15  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:06  marisag
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

static const char rcsid[] = "$Id: cmd_general2.c,v 1.2 2000/05/18 06:50:02 marisa Exp $";

/*
 * doDischarge1 - adds up the number of military in the planets for
 *          cmd_discharge
 */

void doDischarge1(IMP, Planet_t *pl)
{
    if ((readPlQuan(IS, pl, it_civilians) > 25) &&
        (readPlQuan(IS, pl, it_military) > 5))
    {
        IS->is_ULONG1 += readPlQuan(IS, pl, it_military);
    }
}

/*
 * doDischarge2 - does the actual discharges for cmd_discharge
 */

void doDischarge2(IMP, register Planet_t *pl)
{
    register ULONG plNum;
    register USHORT discharged;

    if ((readPlQuan(IS, pl, it_civilians) > 25) &&
        (readPlQuan(IS, pl, it_military) > 5))
    {
        plNum = pl->pl_number;
        server(IS, rt_lockPlanet, plNum);
        pl = &IS->is_request.rq_u.ru_planet;
	if (pl->pl_btu < 3)
	{
	    server(IS, rt_unlockPlanet, plNum);
	    userP(IS, "Insufficient BTUs on ", pl, " to discharge anyone\n");
	    return;
	}
	pl->pl_btu -= 3;
        discharged =
            umin(127 - readPlQuan(IS, pl, it_civilians),
                readPlQuan(IS, pl, it_military) * IS->is_USHORT1 / 100);
        writePlQuan(IS, pl, it_military, readPlQuan(IS, pl, it_military) -
            discharged);
        writePlQuan(IS, pl, it_civilians, readPlQuan(IS, pl, it_civilians) +
            discharged);
        server(IS, rt_unlockPlanet, plNum);
        if (discharged)
        {
            IS->is_ULONG1 += discharged;
            userN(IS, discharged);
            userP(IS, " military released from active service on ", pl, "\n");
            fePlDirty(IS, plNum);
        }
    }
}

/*
 * cmd_discharge - Allows a player to discharge military from a planet.
 *          Note that only grunts can be discharged, not officers
 */

BOOL cmd_discharge(IMP)
{
    PlanetScan_t ps;
    ULONG wanted;

    if (reqPlanets(IS, &ps, "Enter planets to discharge on") &&
        doSkipBlanks(IS) &&
        reqNumber(IS, &wanted, "Total number to discharge"))
    {
        IS->is_ULONG1 = 0;
        (void) scanPlanets(IS, &ps, doDischarge1);
        IS->is_USHORT1 = wanted * 100 / IS->is_ULONG1;
        if (IS->is_USHORT1 > 50)
        {
            IS->is_USHORT1 = 50;
        }
        IS->is_ULONG1 = 0;
        (void) scanPlanets(IS, &ps, doDischarge2);
        if (IS->is_ULONG1 == 0)
        {
            err(IS, "nobody signed up for early retirement - how "
                "flattering!");
        }
        else
        {
            userN(IS, IS->is_ULONG1);
            user(IS, " military discharged\n");
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * cmd_power - builds up and displays a power report
 */

void cmd_power(IMP)
{
    PowerHead_t ph;
    PowerData_t pd[PLAYER_MAX];
    PowerData_t temp;
    register Planet_t *rpl;
    register Ship_t *rsh;
    register World_t *rw;
    register PowerData_t *d;
    char *ptr;
    register USHORT i, j;
    register ULONG plNum;
    USHORT play;
    ULONG shipNumber, lastItem;
    BOOL needRecalc, aborted, powAll;
    char prog[85];

    needRecalc = FALSE;
    aborted = FALSE;
    powAll = FALSE;
    rpl = &IS->is_request.rq_u.ru_planet;
    rsh = &IS->is_request.rq_u.ru_ship;
    rw = &IS->is_request.rq_u.ru_world;
    ptr = IS->is_textInPos;
    if (*ptr != '\0')
    {
        *skipWord(IS) = '\0';
        if (strcmp(ptr, "force") == 0)
        {
            if ((IS->is_player.p_status == ps_deity) ||
               IS->is_world.w_nonDeityPower)
            {
                if ((IS->is_player.p_status == ps_active) ||
                   (IS->is_player.p_status == ps_deity))
                {
                   needRecalc = TRUE;
                }
                else
                {
                   err(IS, "You must be an active player to use the 'force' "
                       "option");
                }
            }
            else
            {
                err(IS, "Only a deity can force a new power report - option "
                    "ignored");
            }
        }
        else if (strcmp(ptr, "all") == 0)
        {
            powAll = TRUE;
        }
        else
        {
            err(IS, "Unknown option to 'power'");
        }
    }

    /* check if another user is already updating the report */
    if (needRecalc)
    {
        server(IS, rt_readWorld, 0);
        if (rw->w_doingPower)
        {
            user(IS, "Another player is already updating the report, please "
                "wait for them to finish\n");
            return;
        }
    }

    if (!needRecalc)
    {
        /* read any existing power report */
        server(IS, rt_readPower, 1);
        if (IS->is_request.rq_whichUnit == 0)
        {
            err(IS, "power report not available");
            return;
        }
        else
        {
            ph = IS->is_request.rq_u.ru_powerHead;
            i = 0;
            server(IS, rt_readPower, 1);
            while(IS->is_request.rq_whichUnit)
            {
                /* just in case we have a garbage file! */
                if (i < PLAYER_MAX)
                {
                    pd[i] = IS->is_request.rq_u.ru_powerData;
                    i++;
                }
                server(IS, rt_readPower, 1);
            }
            if (i != ph.ph_playerCount)
            {
                err(IS, "power report invalid");
                return;
            }
            else
            {
                if (!IS->is_world.w_doingPower)
                {
                    if ((IS->is_player.p_status == ps_active) ||
                        (IS->is_player.p_status == ps_deity))
                    {
                        if ((ph.ph_lastTime <
                           IS->is_request.rq_time - IS->is_world.w_secondsPerITU
                           * 48) && ((IS->is_player.p_status == ps_deity) ||
                           IS->is_world.w_nonDeityPower) &&
                           ask(IS, "Power report is stale. Recompute? "))
                        {
                           needRecalc = TRUE;
                        }
                    }
                }
            }
        }
    }

    if (needRecalc)
    {
        server(IS, rt_lockWorld, 0);
        rw->w_doingPower = TRUE;
        server(IS, rt_unlockWorld, 0);
        user(IS, "Recalculating power report\n");
	if (IS->is_player.p_feMode == 0)
	{
            user(IS, "                      (CTRL-C to abort)\n");
	}

        /* initialize the slots for the various players */
        for (play = 0; play < IS->is_world.w_currPlayers; play++)
        {
            server(IS, rt_readPlayer, play);
            d = &pd[play];
            d->pd_player = play;
            d->pd_plan = 0;
            d->pd_civ = 0;
            d->pd_mil = 0;
            d->pd_shell = 0;
            d->pd_gun = 0;
            d->pd_plane = 0;
            d->pd_bar = 0;
            d->pd_effic = 0;
            d->pd_ship = 0;
            d->pd_tons = 0;
            d->pd_money = IS->is_request.rq_u.ru_player.p_money;
        }

        /* add up the contents of all of the planets in the world */

        plNum = 0;
        lastItem = 0;
        while ((plNum < IS->is_world.w_planetNext) && !aborted)
        {
	    if (IS->is_player.p_feMode == 0)
	    {
                if ((plNum - lastItem) > 25)
                {
                    userC(IS, '*');
                    uFlush(IS);
                    lastItem = plNum;
		}
	    }
	    else
	    {
                if ((plNum - lastItem) > 200)
                {
	            sprintf(prog, "!Examining planet %u", plNum);
	            uPrompt(IS, prog);
                    lastItem = plNum;
		}
            }
            /* do NOT update the planet */
            server(IS, rt_readPlanet, plNum);
            if (rpl->pl_owner != NO_OWNER)
            {
                d = &pd[rpl->pl_owner];
                d->pd_plan++;
                d->pd_civ   += (readPlQuan(IS, rpl, it_civilians) +
                    readPlQuan(IS, rpl, it_civilians));
                d->pd_mil   += (readPlQuan(IS, rpl, it_military) +
                    readPlQuan(IS, rpl, it_military));
                d->pd_shell += readPlQuan(IS, rpl, it_missiles);
                d->pd_gun   += readPlQuan(IS, rpl, it_weapons);
                d->pd_plane += readPlQuan(IS, rpl, it_planes);
                d->pd_bar   += readPlQuan(IS, rpl, it_bars);
                d->pd_effic += rpl->pl_efficiency;
            }
            plNum++;
            aborted = clGotCtrlC(IS);
            /* If aborted, then turn off the flag */
            if (aborted)
            {
                server(IS, rt_lockWorld, 0);
                rw->w_doingPower = FALSE;
                server(IS, rt_unlockWorld, 0);
            }
        }

        /* add in the contents of all of the ships */

        if ((IS->is_world.w_shipNext != 0) && !aborted)
        {
            shipNumber = 0;
            lastItem = 0;
            while ((shipNumber < IS->is_world.w_shipNext) && !aborted)
            {
                if ((shipNumber - lastItem) > 15)
                {
	    	    if (IS->is_player.p_feMode == 0)
		    {
                        userC(IS, '$');
                        uFlush(IS);
		    }
		    else
		    {
	                sprintf(prog, "!Examining ship %u", shipNumber);
	                uPrompt(IS, prog);
		    }
                    lastItem = shipNumber;
                }
                server(IS, rt_readShip, shipNumber);
                if (rsh->sh_owner != NO_OWNER)
                {
                    d = &pd[rsh->sh_owner];
                    d->pd_ship++;
                    d->pd_tons  += IS->is_world.w_shipCost[rsh->sh_type];
                    d->pd_civ += (rsh->sh_items[it_civilians] +
                        rsh->sh_items[it_civilians]);
                    d->pd_mil += (rsh->sh_items[it_military] +
                        rsh->sh_items[it_military]);
                    d->pd_shell += rsh->sh_items[it_missiles];
                    d->pd_gun   += rsh->sh_items[it_weapons];
                    d->pd_plane += rsh->sh_items[it_planes];
                    d->pd_bar   += rsh->sh_items[it_bars];
                }
                shipNumber++;
                aborted = clGotCtrlC(IS);
                /* If aborted, then turn off the flag */
                if (aborted)
                {
                    server(IS, rt_lockWorld, 0);
                    rw->w_doingPower = FALSE;
                    server(IS, rt_unlockWorld, 0);
                }
            }
        }
        userNL(IS);

        if (!aborted)
        {

            /* calculate the power for each player based on the stats */

            for (play = 1; play < IS->is_world.w_currPlayers; play++)
            {
                d = &pd[play];
                d->pd_power =
                    (d->pd_effic + d->pd_gun) / 3 +
                    (d->pd_civ + d->pd_mil + d->pd_shell + d->pd_tons) / 10 +
                    d->pd_money / 100 + d->pd_plane +
                    d->pd_ship + d->pd_bar * 5;
            }

            /* if enough players, sort them by power */

            if (IS->is_world.w_currPlayers > 2)
            {
                for (i = IS->is_world.w_currPlayers - 2; i >= 1; i--)
                {
                    for (j = 1; j <= i; j++)
                    {
                        if (pd[j].pd_power < pd[j + 1].pd_power)
                        {
                            temp = pd[j];
                            pd[j] = pd[j + 1];
                            pd[j + 1] = temp;
                        }
                    }
                }
            }
            ph.ph_lastTime = IS->is_request.rq_time;
            ph.ph_playerCount = IS->is_world.w_currPlayers;
            for (i = 1; i < ph.ph_playerCount; i++)
            {
                d = &pd[i];
                server(IS, rt_readPlayer, d->pd_player);
                if ((IS->is_player.p_status == ps_deity) &&
                    (IS->is_request.rq_u.ru_player.p_planetCount !=
                    d->pd_plan))
                {
                    user3(IS, "NOTE: Player ",
                          &IS->is_request.rq_u.ru_player.p_name[0],
                          " has ");
                    userN(IS, d->pd_plan);
                    userN3(IS, " planets, but player record shows ",
                           IS->is_request.rq_u.ru_player.p_planetCount,
                           " planets!\n");
                }
            }

            /* try to write the resulting power file */

            IS->is_request.rq_u.ru_powerHead = ph;
            server(IS, rt_writePower, 1);
            if (IS->is_request.rq_whichUnit == 0)
            {
                err(IS, "error writing power file");
                aborted = TRUE;
                /* aborted, turn off the flag */
                server(IS, rt_lockWorld, 0);
                rw->w_doingPower = FALSE;
                server(IS, rt_unlockWorld, 0);
            }
            else
            {
                i = 0;
                while (i < IS->is_world.w_currPlayers)
                {
                    IS->is_request.rq_u.ru_powerData = pd[i];
                    i++;
                    server(IS, rt_writePower, 1);
                    if (IS->is_request.rq_whichUnit == 0)
                    {
                        err(IS, "error writing power file");
                        aborted = TRUE;
                        i = IS->is_world.w_currPlayers;
                        /* aborted, turn off the flag */
                        server(IS, rt_lockWorld, 0);
                        rw->w_doingPower = FALSE;
                        server(IS, rt_unlockWorld, 0);
                    }
                }
                if (!aborted)
                {
                    server(IS, rt_writePower, 0);
                    /* done - turn off the flag */
                    server(IS, rt_lockWorld, 0);
                    rw->w_doingPower = FALSE;
                    server(IS, rt_unlockWorld, 0);
                    user(IS, "Power report updated\n\n");
                }
            }
        }
    }

    if (!aborted)
    {
        user(IS, "Imperium power report as of ");
        uTime(IS, ph.ph_lastTime);
        user(IS, ":\n");
        user(IS,
" plan    civ    mil    sh   gun  pl  bar   % ship    $     pow  player\n"
        );
        dash(IS, 75);
        for (i = 1; i < ph.ph_playerCount; i++)
        {
            d = &pd[i];
            server(IS, rt_readPlayer, d->pd_player);
            if ((IS->is_request.rq_u.ru_player.p_status == ps_active) ||
                   (powAll == TRUE))
            {
                fePowerReport(IS);
                userF(IS, d->pd_plan, 5);
                userF(IS, d->pd_civ, 7);
                userF(IS, d->pd_mil, 7);
                userF(IS, d->pd_shell, 6);
                userF(IS, d->pd_gun, 5);
                userF(IS, d->pd_plane, 5);
                userF(IS, d->pd_bar, 5);
                /* prevent a division by zero error */
                if (d->pd_plan)
                {
                    userF(IS, d->pd_effic / d->pd_plan, 4);
                }
                else
                {
                    userF(IS, 0, 4);
                }
                userF(IS, d->pd_ship, 4);
                userF(IS, d->pd_money, 8);
                userF(IS, d->pd_power, 6);
                user3(IS, "  ", &IS->is_request.rq_u.ru_player.p_name[0],
                   "\n");
            }
        }
        userNL(IS);
    }
}

/*
 * cmd_grant - allows one player to give a planet to another player
 */

BOOL cmd_grant(IMP)
{
    register Planet_t *rpl;
    register Player_t *rp;
    USHORT who;
    ULONG plNum;
    Planet_t savePl;
    UBYTE race;

    if (reqPlanet(IS, &plNum, "Planet to grant? ") && doSkipBlanks(IS) &&
        reqPlayer(IS, &who, "Player to grant it to? "))
    {
        server(IS, rt_readPlanet, plNum);
        rpl = &IS->is_request.rq_u.ru_planet;
        rp = &IS->is_request.rq_u.ru_player;
        if (rpl->pl_owner != IS->is_player.p_number)
        {
            err(IS, "you don't own that planet");
        }
	else if (rpl->pl_btu < 2)
        {
            err(IS, "not enough BTUs on that planet to grant it to someone "
		"else");
        }
        else
        {
            server(IS, rt_lockPlanet, plNum);
	    rpl->pl_btu -= 2;
            updatePlanet(IS);
            server(IS, rt_unlockPlanet, plNum);
            if (rpl->pl_owner != IS->is_player.p_number)
            {
                err(IS, "planet lost during update");
            }
            else
            {
                server(IS, rt_readPlayer, who);
                if (rp->p_status != ps_active)
                {
                    err(IS, "that player is not active");
                }
                else
                {
                    server(IS, rt_lockPlayer, who);
                    rp->p_planetCount++;
                    race = rp->p_race;
                    server(IS, rt_unlockPlayer, who);
                    server(IS, rt_lockPlanet, plNum);
                    memset(&rpl->pl_checkpoint[0], '\0', PLAN_PSWD_LEN *
                        sizeof(char));
                    /* see about relations */
                    if (rpl->pl_transfer != pt_hostile)
                    {
                        rpl->pl_lastOwner = rpl->pl_owner;
                        rpl->pl_transfer = pt_trade;
                    }
                    rpl->pl_owner = who;
                    savePl = *rpl;
                    server(IS, rt_unlockPlanet, plNum);
                    server(IS, rt_lockWorld, 0);
                    IS->is_request.rq_u.ru_world.w_race[IS->is_player.p_race].r_planetCount--;
                    IS->is_request.rq_u.ru_world.w_race[race].r_planetCount++;
                    server(IS, rt_unlockWorld, 0);
                    IS->is_world.w_race[IS->is_player.p_race].r_planetCount =
                        IS->is_request.rq_u.ru_world.w_race[IS->is_player.p_race].r_planetCount;
                    IS->is_world.w_race[race].r_planetCount =
                        IS->is_request.rq_u.ru_world.w_race[race].r_planetCount;
                    userP(IS, "Planet ", &savePl, " granted to ");
                    user2(IS, &rp->p_name[0], ".\n");
                    server(IS, rt_lockPlayer, IS->is_player.p_number);
                    rp->p_planetCount--;
                    IS->is_player.p_planetCount = rp->p_planetCount;
                    server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    if (IS->is_player.p_planetCount == 0)
                    {
                        user(IS, "You have just given away your last "
                            "planet!!\n");
                    }
                    user(IS, &IS->is_player.p_name[0]);
                    userP(IS, " has granted you planet ", &savePl, ".");
                    notify(IS, who);
                    news(IS, n_grant_planet, IS->is_player.p_number, who);
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * doDump - part of cmd_dump
 */

void doDump(IMP, register Planet_t *pl)
{
    register ItemType_t it;

    /* we put all the contsant stuff up here */
    user(IS, FE_PLDUMP);        /* indicate that this is a planet dump */
    userX(IS, pl->pl_number, 8);
    userX(IS, pl->pl_row, 4);
    userX(IS, pl->pl_col, 4);
    userX(IS, pl->pl_techLevel, 8);
    userX(IS, pl->pl_resLevel, 8);
    userX(IS, pl->pl_lastUpdate, 8);
    if (pl->pl_plagueStage > 1)
    {
        userX(IS, pl->pl_plagueStage, 1);
    }
    else
    {
        userX(IS, 0, 1);
    }
    if (pl->pl_transfer == pt_hostile)
    {
        userX(IS, 1, 1);
    }
    else
    {
        userX(IS, 0, 1);
    }
    userX(IS, pl->pl_class, 1);
    userX(IS, pl->pl_mobility, 4);
    userX(IS, pl->pl_efficiency, 2);
    userX(IS, pl->pl_minerals, 2);
    userX(IS, pl->pl_gold, 2);
    userX(IS, pl->pl_polution, 2);
    userX(IS, pl->pl_gas, 2);
    userX(IS, pl->pl_water, 2);
    userX(IS, pl->pl_size, 1);
    userX(IS, pl->pl_btu, 4);
    if (pl->pl_checkpoint[0] != '\0')
    {
        if (strchr(&pl->pl_checkpoint[0], '!') == NULL)
        {
            userX(IS, 1, 1);
        }
        else
        {
            userX(IS, 2, 1);
        }
    }
    else
    {
        userX(IS, 0, 1);
    }
    /* if the player is a diety, print extra info */
    if (IS->is_player.p_status == ps_deity)
    {
        userC(IS, '*');         /* indicates a deity block follows */
        userX(IS, pl->pl_owner, 2);
        userX(IS, pl->pl_lastOwner, 2);
        userX(IS, pl->pl_plagueStage, 2);
        userX(IS, pl->pl_plagueTime, 2);
    }
    /* now add on the variable length stuff */
    userC(IS, '@');             /* indicate start of variable length */
    for (it = IT_FIRST; it <= IT_LAST; it++)
    {
        userX(IS, readPlQuan(IS, pl, it), 4);
    }
    for (it = PPROD_FIRST; it <= PPROD_LAST; it++)
    {
        userX(IS, pl->pl_prod[it], 4);
        userX(IS, pl->pl_workPer[it], 2);
    }
    userNL(IS);
}

/*
 * cmd_dump - allows a user to dump out mucho info on planets they own in
 *          a compact ASCII hexidecimal format for use with front ends or
 *          other kinds of map builders
 */

void cmd_dump(IMP)
{
    PlanetScan_t ps;

    if (reqPlanets(IS, &ps, "Planets to dump? "))
    {
        uTime(IS, IS->is_request.rq_time);
        userNL(IS);
        (void) scanPlanets(IS, &ps, doDump);
    }
}

/*
 * cmd_name - allows a player to name ships or planets that they own
 */

void cmd_name(IMP)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    ULONG itemNum;
    BOOL isShip;

    if (reqPlanetOrShip(IS, &itemNum, &isShip, "Name which planet or ship"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rpl = &IS->is_request.rq_u.ru_planet;
        (void) skipBlanks(IS);
        if (isShip)
        {
            server(IS, rt_readShip, itemNum);
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                user(IS, "You don't own that ship!\n");
            }
            else
            {
                if (rpl->pl_name[0] != '\0')
                {
                    user3(IS, "Current planet name: ", &rpl->pl_name[0],
                        "\n");
                }
                uPrompt(IS, "Enter new ship name");
                if (clReadUser(IS))
                {
                    if (*IS->is_textInPos != '\0')
                    {
                        server(IS, rt_lockShip, itemNum);
                        if (strcmp(&IS->is_textIn[0], "none") == 0)
                        {
                            rsh->sh_name[0] = '\0';
                        }
                        else
                        {
                            IS->is_textIn[SHIP_NAME_LEN - 1] = '\0';
                            strncpy(&rsh->sh_name[0], &IS->is_textIn[0],
                                (SHIP_NAME_LEN - 1) * sizeof(char));
                        }
                        server(IS, rt_unlockShip, itemNum);
                        feShDirty(IS, itemNum);
                    }
                }
            }
        }
        else
        {
            server(IS, rt_readPlanet, itemNum);
            if (rpl->pl_owner != IS->is_player.p_number)
            {
                user(IS, "You don't control that planet!\n");
            }
            else
            {
                if (rpl->pl_name[0] != '\0')
                {
                    user3(IS, "Current planet name: ", &rpl->pl_name[0],
                        "\n");
                }
                uPrompt(IS, "Enter new planet name");
                if (clReadUser(IS))
                {
                    if (*IS->is_textInPos != '\0')
                    {
                        server(IS, rt_lockPlanet, itemNum);
                        if (strcmp(&IS->is_textIn[0], "none") == 0)
                        {
                            rpl->pl_name[0] = '\0';
                        }
                        else
                        {
                            IS->is_textIn[PLAN_NAME_LEN - 1] = '\0';
                            strncpy(&rpl->pl_name[0], &IS->is_textIn[0],
                                (PLAN_NAME_LEN - 1) * sizeof(char));
                        }
                        server(IS, rt_unlockPlanet, itemNum);
                        fePlDirty(IS, itemNum);
                    }
                }
            }
        }
    }
}

/*
 * doPlate - does the actual plating for cmd_plate
 *          Convention: the planet and ship are in the request as a pair.
 */

void doPlate(IMP)
{
    register PlanetShipPair_t *rp;
    register USHORT maxArm, present, loaded;
    ULONG plNum, shipNumber;
    ULONG amount, maxLoad, amountOrig;
    char buf[256];

    rp = &IS->is_request.rq_u.ru_planetShipPair;
    plNum = rp->p_p.pl_number;
    shipNumber = rp->p_sh.sh_number;
    maxArm = IS->is_world.w_shipCargoLim[rp->p_sh.sh_type];
    present = rp->p_p.pl_prod[pp_hull];
    loaded = rp->p_sh.sh_cargo;
    if ((present == 0) && (maxArm != loaded))
    {
        userP(IS, "No armor plating on planet #", &rp->p_p, "\n");
    }
    else if (rp->p_p.pl_btu < 2)
    {
        userP(IS, "Not enough BTUs on planet #", &rp->p_p, "\n");
    }
    else if ((present != 0) && ((maxArm == loaded) ||
        (maxArm - loaded < IS->is_world.w_armourWeight)))
    {
        user(IS, "There is no room for any additional armor plating\n");
    }
    else if ((present != 0) && (maxArm != loaded))
    {
        maxLoad = umin((maxArm - loaded) / IS->is_world.w_armourWeight,
            present);
        *IS->is_textInPos = '\0';
        userN(IS, present);
        user(IS, " units of plating");
        userN2(IS, " here, ", rp->p_sh.sh_armourLeft);
        userN3(IS, " already installed, add how many (max ", maxLoad,
            ")");
        getPrompt(IS, &buf[0]);
        if (!reqPosRange(IS, &amount, maxLoad, &buf[0]))
        {
            amount = 0;
        }
        if (amount != 0)
        {
            amountOrig = amount;
            server2(IS, rt_lockPlanetShipPair, plNum, shipNumber);
            present = rp->p_p.pl_prod[pp_hull];
            if (amount > present)
            {
                amount = present;
            }
            loaded = rp->p_sh.sh_cargo;
            if (amount > maxArm - loaded)
            {
                amount = maxArm - loaded;
            }
            rp->p_p.pl_prod[pp_hull] -= amount;
            if (rp->p_p.pl_btu >= 2)
            {
                rp->p_p.pl_btu -= 2;
            }
            else
            {
                rp->p_p.pl_btu = 0;
            }
            rp->p_sh.sh_cargo += (amount * IS->is_world.w_armourWeight);
            rp->p_sh.sh_armourLeft += amount;
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
                userN3(IS, "Events have prevented you from adding more than ",
                    amount, " units.\n");
            }
        }
    }
}

/*
 * cmd_plate - allows a player to put armor onto a ship
 */

BOOL cmd_plate(IMP)
{
    register Ship_t *rsh;
    register Planet_t *rpl;
    register PlanetShipPair_t *rpp;
    Ship_t saveShip;
    Planet_t savePl;
    ULONG shipNumber, plNum=NO_ITEM;
    BOOL aborting;

    /* get the ship number */
    if (reqShip(IS, &shipNumber, "Ship to plate"))
    {
        /* set up the pointers */
        rsh = &IS->is_request.rq_u.ru_ship;
        rpl = &IS->is_request.rq_u.ru_planet;
        rpp = &IS->is_request.rq_u.ru_planetShipPair;
        (void) skipBlanks(IS);
        aborting = FALSE;

        /* abort if they entered an invalid number to plate */
        if (!aborting)
        {
            /* read the ship in */
            server(IS, rt_readShip, shipNumber);
            /* make sure they own it */
            if (rsh->sh_owner != IS->is_player.p_number)
            {
                err(IS, "you don't own that ship");
            }
            else
            {
                /* make sure the ship is on a planet */
                if (rsh->sh_planet == NO_ITEM)
                {
                    err(IS, "ship is not on a planet");
                    aborting = TRUE;
                }
                else if (rsh->sh_type == st_m)
                {
                    err(IS, "ship is a miner");
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
                                    /* there is a code, so see if the plyr */
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
                        err(IS, "invalid planet in cmd_plate");
                        aborting = TRUE;
                    }
                }
                /* is everything still hunky dorey? */
                if (!aborting)
                {
                    /* fill in the planetShipPair struct */
                    rpp->p_sh = saveShip;
                    rpp->p_p = savePl;
                    doPlate(IS);
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

