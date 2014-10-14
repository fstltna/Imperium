/*
 * Imperium
 *
 * $Id: cmd_general1.c,v 1.3 2000/05/25 18:21:23 marisa Exp $
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
 * $Log: cmd_general1.c,v $
 * Revision 1.3  2000/05/25 18:21:23  marisa
 * Fix more T/F issues
 *
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:48  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:59:06  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:24:14  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:05  marisag
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

#define CmdGen1C 1
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"

static const char rcsid[] = "$Id: cmd_general1.c,v 1.3 2000/05/25 18:21:23 marisa Exp $";

/*
 * cmd_change - allows players to change various things
 */

void cmd_change(IMP)
{
    register char *n;
    register USHORT i;
    ULONG now, lastOn, deltaTime;
    USHORT what, newAmt;
    BOOL duplicate;
    Player_t *rp;
    ULONG FeMode;
    char buff[91];

    /* find out what they want to change */
    if (reqChoice(IS, &what, "name\0password\0player\0notify\0compressed\0"
        "width\0length\0fe\0time\0btu\0sendemail\0emailaddr\0", "Change what (btu warn/compressed/email address/"
        "fe mode/length/name/notify/password/player/send email/time warn/width)"))
    {
        rp = &IS->is_request.rq_u.ru_player;
        /* note that for most of the options you must be an active player */
        switch(what)
        {
            /* change email send mode */
            case 10:
                if (IS->is_player.p_status != ps_visitor)
                {
                    /* lock the current player */
                    server(IS, rt_lockPlayer, IS->is_player.p_number);
                    /* change the flag in the buffer */
                    IS->is_request.rq_u.ru_player.p_sendEmail =
                        !IS->is_request.rq_u.ru_player.p_sendEmail;
                    /* unlock the current player */
                    server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    /* copy the new flag into the current player */
                    IS->is_player.p_sendEmail =
                        IS->is_request.rq_u.ru_player.p_sendEmail;
                    /* tell them what mode they just selected */
                    user(IS, "Send email mode is now ");
                        if (IS->is_player.p_sendEmail)
                        {
                            user(IS, "ON\n");
                        }
                        else
                        {
                            user(IS, "OFF\n");
                        }
                }
                else
                {
                    user(IS, "You must be an active player to use this "
                        "option\n");
                }
                break;
            /* change btu warning level */
            case 9:
                if (IS->is_player.p_status != ps_visitor)
                {
                    /* pre-fill prompt */
                    strcpy(&buff[0], "Warn when BTU's are less than");
                    /* get the new amount */
                    newAmt = repNum(IS, (USHORT)IS->is_player.p_btuWarn, 0,
                        254, &buff[0]);
                    /* lock the current player */
                    server(IS, rt_lockPlayer, IS->is_player.p_number);
                    /* store the new length */
                    IS->is_request.rq_u.ru_player.p_btuWarn = (UBYTE)newAmt;
                    /* unlock the current player */
                    server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    /* copy the new amount out to the various places */
                    IS->is_player.p_btuWarn = (UBYTE)newAmt;
                }
                else
                {
                    user(IS, "You must be an active player to use this "
                        "option\n");
                }
                break;
            /* change time warning level */
            case 8:
                if (IS->is_player.p_status != ps_visitor)
                {
                    /* pre-fill prompt */
                    strcpy(&buff[0], "Warn when time is less than");
                    /* get the new amount */
                    newAmt = repNum(IS, (USHORT)IS->is_player.p_timeWarn, 0,
                        120, &buff[0]);
                    /* lock the current player */
                    server(IS, rt_lockPlayer, IS->is_player.p_number);
                    /* store the new length */
                    IS->is_request.rq_u.ru_player.p_timeWarn = (ULONG)newAmt;
                    /* unlock the current player */
                    server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    /* copy the new amount out to the various places */
                    IS->is_player.p_timeWarn = (ULONG)newAmt;
                }
                else
                {
                    user(IS, "You must be an active player to use this "
                        "option\n");
                }
                break;
            /* change FE mode */
            case 7:
                if (IS->is_player.p_status != ps_visitor)
                {
                    if (reqPosRange(IS, &FeMode, 0xFFFF,
                        "Enter FE mode to activate"))
                    {
                        /* lock the current player */
                        server(IS, rt_lockPlayer, IS->is_player.p_number);
                        /* change the mode in the buffer */
                        IS->is_request.rq_u.ru_player.p_feMode = FeMode;
                        /* unlock the current player */
                        server(IS, rt_unlockPlayer, IS->is_player.p_number);
                        /* copy the new flag into the current player */
                        IS->is_player.p_feMode =
                            IS->is_request.rq_u.ru_player.p_feMode;
                        /* tell them what mode they just selected */
                        user(IS, "FE mode is now ");
                            if (IS->is_player.p_feMode != 0)
                            {
                                user(IS, "ON\n");
                            }
                            else
                            {
                                user(IS, "OFF\n");
                            }
                    }
                }
                else
                {
                    user(IS, "You must be an active player to use this "
                        "option\n");
                }
                break;
            /* change console length */
            case 6:
                if (IS->is_player.p_status != ps_visitor)
                {
                    /* pre-fill prompt */
                    strcpy(&buff[0], "Set console length to");
                    /* get the new amount */
                    newAmt = repNum(IS, IS->is_player.p_conLength, 0,
                        32767, &buff[0]);
                    /* lock the current player */
                    server(IS, rt_lockPlayer, IS->is_player.p_number);
                    /* store the new length */
                    IS->is_request.rq_u.ru_player.p_conLength = newAmt;
                    /* unlock the current player */
                    server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    /* copy the new amount out to the various places */
                    IS->is_player.p_conLength = newAmt;
                    IS->is_conLength = newAmt;
                }
                else
                {
                    user(IS, "You must be an active player to use this "
                        "option\n");
                }
                break;
            /* change console width */
            case 5:
                if (IS->is_player.p_status != ps_visitor)
                {
                    /* pre-fill prompt */
                    strcpy(&buff[0], "Set console width to");
                    /* get the new amount */
                    newAmt = repNum(IS, IS->is_player.p_conWidth, 0,
                        32767, &buff[0]);
                    /* lock the current player */
                    server(IS, rt_lockPlayer, IS->is_player.p_number);
                    /* store the new width */
                    IS->is_request.rq_u.ru_player.p_conWidth = newAmt;
                    /* unlock the current player */
                    server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    /* copy the new amount out to the various places */
                    IS->is_player.p_conWidth = newAmt;
                    IS->is_conWidth = newAmt;
                }
                else
                {
                    user(IS, "You must be an active player to use this "
                        "option\n");
                }
                break;
            /* change compressed state */
            case 4:
                if (IS->is_player.p_status != ps_visitor)
                {
                    /* lock the current player */
                    server(IS, rt_lockPlayer, IS->is_player.p_number);
                    /* change the flag in the buffer */
                    IS->is_request.rq_u.ru_player.p_compressed =
                        !IS->is_request.rq_u.ru_player.p_compressed;
                    /* unlock the current player */
                    server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    /* copy the new flag into the current player */
                    IS->is_player.p_compressed =
                        IS->is_request.rq_u.ru_player.p_compressed;
                    /* tell them what mode they just selected */
                    user(IS, "Compressed mode is now ");
                        if (IS->is_player.p_compressed)
                        {
                            user(IS, "ON\n");
                        }
                        else
                        {
                            user(IS, "OFF\n");
                        }
                }
                else
                {
                    user(IS, "You must be an active player to use this "
                        "option\n");
                }
                break;

            /* change notify mode */
            case 3:
                if (IS->is_player.p_status != ps_visitor)
                {
                    /* see what type of notification they want */
                    if (reqChoice(IS, &what, "telegram\0message\0both\0",
                        "Change to which (telegram/message/both)"))
                    {
                        /* lock the current player */
                        server(IS, rt_lockPlayer, IS->is_player.p_number);
                        /* set the notification method in the buffer */
                        IS->is_request.rq_u.ru_player.p_notify = what;
                        /* unlock the current player */
                        server(IS, rt_unlockPlayer, IS->is_player.p_number);
                        /* copy the mode to the current player */
                        IS->is_player.p_notify =
                            IS->is_request.rq_u.ru_player.p_notify;
                    }
                }
                else
                {
                    user(IS, "You must be an active player to use this "
                        "option\n");
                }
                break;

            /* change player */
            case 2:
                if (reqPlayer(IS, &what, "Enter player to change to"))
                {
                    i = what;
                    /* make sure that changing players is allowed */
                    if ((i != 0) && !IS->is_world.w_chaPlay &&
                       (IS->is_player.p_status != ps_deity))
                    {
                        /* the deity doesn't want it */
                        user(IS, "Changing players is not allowed\n");
                    }
                    else
                    {
                        /* make sure that they can become that player */
                        server(IS, rt_readPlayer, i);
                        if (IS->is_request.rq_u.ru_player.p_loggedOn)
                        {
                            n = &IS->is_request.rq_u.ru_player.p_name[0];
                            user3(IS, "Player ", n, " is already logged "
                                "on.\n");
                            log3(IS, "Player ", n, " is already logged on.");
                        }
                        else if (getPassword(IS, "Enter password",
                            &IS->is_request.rq_u.ru_player.p_password[0]))
                        {
                            what = IS->is_player.p_number;
                            IS->is_player = IS->is_request.rq_u.ru_player;
                            lastOn = IS->is_player.p_lastOn;
                            n = &IS->is_player.p_name[0];
                            /* make sure the player has some time left */
                            if (resetTimer(IS))
                            {
                                user3(IS, "*** ", n,
                                    " is out of time for today\n");
                                server(IS, rt_readPlayer, what);
                                IS->is_player = IS->is_request.rq_u.ru_player;
                            }
                            else
                            {
                                /* let the player become the new player */
                                server(IS, rt_lockPlayer, what);
                                now = IS->is_request.rq_time;
                                deltaTime = (now - lastOn) /
                                    IS->is_world.w_secondsPerITU;
                                /* Check for an overflow of work */
                                if (deltaTime > MAX_WORK)
                                {
                                    /* And cap it at MAX_WORK */
                                    deltaTime = MAX_WORK;
                                }
                                rp->p_lastOn = now;
                                rp->p_loggedOn = FALSE;
                                rp->p_feMode = 0;
                                server(IS, rt_unlockPlayer, what);
                                n = &IS->is_request.rq_u.ru_player.p_name[0];
                                user2(IS, n, " off at ");
                                uTime(IS, now);
                                userNL(IS);
                                log3(IS, "Player ", n, " logged off.");
                                server(IS, rt_lockPlayer,
                                    IS->is_player.p_number);
                                /* update the players BTU's */
                                rp->p_btu = umin(IS->is_world.w_maxBTUs,
                                    (USHORT) rp->p_btu + deltaTime * 12700 /
                                    IS->is_world.w_BTUDivisor);
                                rp->p_loggedOn = TRUE;
                                rp->p_feMode = IS->is_player.p_feMode;
                                rp->p_lastOn = now;
                                server(IS, rt_unlockPlayer,
                                    IS->is_player.p_number);
                                IS->is_player = *rp;
                                IS->is_conWidth = rp->p_conWidth;
                                IS->is_conLength = rp->p_conLength;
                                user2(IS, n, " on at ");
                                uTime(IS, now);
                                user(IS, " last on at ");
                                uTime(IS, lastOn);
                                userNL(IS);
                                log3(IS, "Player ", n, " logged on.");
                                server(IS, rt_setPlayer,
                                    IS->is_player.p_number);
                                telegramCheck(IS);
                            }
                        }
                        else
                        {
                            /* didn't get the password correct, no go */
                            user(IS, "player not changed\n");
                        }
                    }
                }
                break;

            case 11:
            case 1:
            case 0:
                /* they want to change their name, email address, or password */
                if (IS->is_player.p_status != ps_visitor)
                {
                    /* make sure they are the real player */
                    if (getPassword(IS, "Enter your current password",
                           &IS->is_player.p_password[0]))
                    {
                        /* see what they want to change */
                        if (what == 0)
                        {
                            if (IS->is_world.w_noNameChange)
                            {
                                err(IS, "Changing names is not allowed");
                            }
                            else
                            {
                                /* they want to change their name */
                                uPrompt(IS, "Enter new player name");
                                if (clReadUser(IS) && (*IS->is_textInPos !=
                                    '\0'))
                                {
                                    duplicate = FALSE;
                                    i = 0;
                                    /* see if that name is in use */
                                    while ((i < IS->is_world.w_currPlayers) &&
                                        !duplicate)
                                    {
                                        server(IS, rt_readPlayer, i);
                                        if ((i != IS->is_player.p_number) &&
                                            (strcmp(&IS->is_request.rq_u.ru_player.p_name[0],
                                            &IS->is_textIn[0]) == 0))
                                        {
                                            duplicate = TRUE;
                                        }
                                        i++;
                                    }
                                    if (duplicate)
                                    {
                                       err(IS, "That name is already in use");
                                    }
                                    else
                                    {
                                        /* name is OK, so change it */
                                        server(IS, rt_lockPlayer,
                                            IS->is_player.p_number);
                                        strcpy(
                                            &IS->is_request.rq_u.ru_player.p_name[0],
                                            &IS->is_textIn[0]);
                                        server(IS, rt_unlockPlayer,
                                            IS->is_player.p_number);
                                        IS->is_player =
                                            IS->is_request.rq_u.ru_player;
                                    }
                                }
                            }
                        }
                        else if (what == 1)
                        {
                            /* they want to change their password */
                            if (newPlayerPassword(IS))
			    {
                                server(IS, rt_lockPlayer, IS->is_player.p_number);
                                strcpy(&IS->is_request.rq_u.ru_player.p_password[0],
                                    &IS->is_player.p_password[0]);
                                server(IS, rt_unlockPlayer, IS->is_player.p_number);
			    }
                        }
                        else
                        {
                            /* they want to change their email address */
                            if (newPlayerEmail(IS))
			    {
                                server(IS, rt_lockPlayer, IS->is_player.p_number);
                                strcpy(&IS->is_request.rq_u.ru_player.p_email[0],
                                    &IS->is_player.p_email[0]);
                                server(IS, rt_unlockPlayer, IS->is_player.p_number);
			    }
                        }
                    }
                    else
                    {
                        /* they didn't know the password.... */
                        user(IS, "Sorry, that's not the password\n");
                    }
                }
                else
                {
                    user(IS, "You must be an active player to use this "
                        "option\n");
                }
                break;

            default:
                break;
        }
    }
}

/*
 * doItemCensus - does the actual item print-outs for cmd_census
 */

void doItemCensus(IMP, register Planet_t *p)
{
    register ItemType_t it;
    register USHORT dVal;

    feIteCen(IS);
    /* print the owner number if the current player is a deity */
    if (IS->is_player.p_status == ps_deity)
    {
        userF(IS, p->pl_owner, 3);
        userSp(IS);
    }
    user(IS, "|");
    /* loop through the items */
    for (it = it_missiles; it <= IT_LAST; it++)
    {
            /* get the current quantity of items */
            dVal = readPlQuan(IS, p, it);
            /* if there is more than will fit, scale them */
            if (dVal > 9999)
            {
                userF(IS, (dVal / 100), 3);
                userC(IS, 'x');
            }
            else
            {
                userF(IS, dVal, 4);
            }
            userC(IS, '|');
    }
    userF(IS, p->pl_number, 6);
    userNL(IS);
}

/*
 * doProdCensus - does the actual production print-outs for cmd_census
 */

void doProdCensus(IMP, register Planet_t *p)
{
    register PProd_t pp;
    register USHORT dVal;

    feProCen(IS);
    if (IS->is_player.p_status == ps_deity)
    {
        userF(IS, p->pl_owner, 3);
        userSp(IS);
    }
    user(IS, "|");
    for (pp = PPROD_FIRST; pp <= PPROD_LAST; pp++)
    {
        /* get the amount of production */
        dVal = p->pl_prod[pp];
        /* if there is more than will fit, scale them */
        if (dVal > 999)
        {
            if (dVal > 9999)
            {
                userF(IS, (dVal / 1000), 2);
                userC(IS, 'k');
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
        userC(IS, '|');
    }
    userF(IS, p->pl_number, 8);
    user(IS, "|\n");

    feProCen2(IS);
    if (IS->is_player.p_status == ps_deity)
    {
        user(IS, "    ");
    }
    user(IS, "|");
    for (pp = PPROD_FIRST; pp <= PPROD_LAST; pp++)
    {
        userF(IS, p->pl_workPer[pp], 3);
        user(IS, "|");
    }
    user(IS, "        |\n");
    if (IS->is_player.p_status == ps_deity)
    {
        user(IS, "    ");
    }
    user(IS, "|___|___|___|___|___|___|___|___|___|___|___|___|"
        "___|___|___|___|___|________|\n");
}

/*
 * doGeoCensus - does the actual geology print-outs for cmd_census
 */

void doGeoCensus(IMP, register Planet_t *p)
{
    feGeoCen(IS);
    if (IS->is_player.p_status == ps_deity)
    {
        userF(IS, p->pl_owner, 3);
        userSp(IS);
    }
    if (p->pl_checkpoint[0] == '\0')
    {
        userC(IS, ' ');
    }
    else
    {
        userC(IS, '*');
    }
    userC(IS, PLANET_CHAR[p->pl_class]);
    user(IS, "  ");
    userF(IS, p->pl_size, 1);
    userSp(IS);
    userF(IS, p->pl_efficiency, 3);
    userSp(IS);
    userF(IS, p->pl_minerals, 3);
    userSp(IS);
    userF(IS, p->pl_gold, 3);
    userSp(IS);
    userF(IS, p->pl_polution, 3);
    userSp(IS);
    userF(IS, p->pl_gas, 3);
    userSp(IS);
    userF(IS, p->pl_water, 3);
    userSp(IS);
    userF(IS, p->pl_mobility, 3);
    userSp(IS);
    userF(IS, readPlQuan(IS, p, it_ore), 5);
    userSp(IS);
    userF(IS, readPlQuan(IS, p, it_bars), 5);
    user(IS, "  ");
    userF(IS, getTechFactor(p->pl_techLevel), 3);
    user(IS, "  ");
    userF(IS, p->pl_resLevel, 5);
    user(IS, "  ");
    userF(IS, p->pl_number, 6);
    user(IS, "  ");
    userF(IS, p->pl_row, 4);
    userC(IS, ',');
    userN(IS, p->pl_col);
    userNL(IS);
}

/*
 * doPopCensus - does the actual population print-outs for cmd_census
 */

void doPopCensus(IMP, register Planet_t *p)
{
    register ULONG curPop, maxPop;

    fePopCen(IS);
    if (IS->is_player.p_status == ps_deity)
    {
        userF(IS, p->pl_owner, 3);
        userSp(IS);
    }
    if (p->pl_plagueStage > 1)
    {
        userC(IS, '\x50');
    }
    else
    {
        userC(IS, ' ');
    }
    if (p->pl_checkpoint[0] == '\0')
    {
        userC(IS, ' ');
    }
    else
    {
        userC(IS, '*');
    }
    userC(IS, PLANET_CHAR[p->pl_class]);
    user(IS, "  ");
    userF(IS, readPlQuan(IS, p, it_civilians), 5);
    user(IS, "  ");
    userF(IS, readPlQuan(IS, p, it_scientists), 5);
    user(IS, "  ");
    userF(IS, readPlQuan(IS, p, it_military), 5);
    user(IS, "  ");
    userF(IS, readPlQuan(IS, p, it_officers), 5);
    user(IS, "  ");
    userF(IS, p->pl_btu, 3);
    user(IS, "  ");
    curPop = (readPlQuan(IS, p, it_civilians) +
        readPlQuan(IS, p, it_scientists) +
        readPlQuan(IS, p, it_military) +
        readPlQuan(IS, p, it_officers));
    maxPop = maxPopulation(p) / 100;
    if (maxPop != 0)
    {
        userF(IS, curPop / maxPop, 4);
    }
    else
    {
        user(IS, "    ");
    }
    user(IS, "  ");
    userF(IS, calcPlagueFactor(IS, p) / 100, 3);
    user(IS, "  ");
    userF(IS, p->pl_ownRace, 2);
    user(IS, "   ");
    userF(IS, p->pl_number, 6);
    if (p->pl_name[0] != '\0')
    {
        user2(IS, "  ", &p->pl_name[0]);
    }
    userNL(IS);
}

/*
 * doBICensus - lists the big items on the planet for cmd_census
 */

void doBICensus(IMP, register Planet_t *pl)
{
    register BigItem_t *rbi;
    register ULONG count, iMax;
    Planet_t savePl;

    /* set these up for later */
    rbi = &IS->is_request.rq_u.ru_bigItem;
    savePl = *pl;

    iMax = IS->is_world.w_bigItemNext;
    for (count = 0; count < iMax; count++)
    {
        /* read the item in */
        server(IS, rt_readBigItem, count);
        /* never display a hull */
        if (rbi->bi_part != bp_hull)
        {
            /* make sure it is on this planet */
            if ((rbi->bi_itemLoc == savePl.pl_number) && !rbi->bi_onShip &&
                (rbi->bi_status != bi_destroyed))
            {
		feBigCen(IS);
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
                userNL(IS);
            }
        }
    }
}

/*
 * cmd_census - allows the player to get a break-down of the planets they
 *          specify in several ways
 */

void cmd_census(IMP)
{
    PlanetScan_t ps;
    USHORT what, dashSize;
    register char *pPtr;
    void (*scanner)(IMP, register Planet_t *PlanetPtr);

    /* find out what they want a report on */
    if (reqChoice(IS, &what, "population\0geology\0production\0items\0big\0",
        "Report on what (population/geology/production/items/big)"))
    {
        (void) skipBlanks(IS);
        /* get a range of planets to display */
        if (reqPlanets(IS, &ps, "Enter planets specification for census"))
        {
            /* If they are a deity, display the owning players number */
            if (IS->is_player.p_status == ps_deity)
            {
                user(IS, " ## ");
            }
            /* Ok, now set things up depending on what they want */
            switch(what)
            {
                case 4:
                    pPtr = " Item   | Planet | TF | T | Wgt | Eff\n";
                    scanner = doBICensus;
                    dashSize = 38;
                    break;
                case 3:
                    pPtr = "|miss|plne| ore|bars|airt|ftnk|comp| eng|life|"
                        "elec|weap|Planet\n";
                    scanner = doItemCensus;
                    dashSize = 62;
                    break;

                case 2:
                    pPtr = "|Civ|Mil|Tec|Res|Edu|OCS| OM|"
                        " GM|ATn|FTn|Wea|Eng|Hul|Mis|Pln|Elc|Csh"
                        "|        |\n|Prd|Prd|Prd|Prd|Prd|Prd|"
                        "Prd|Prd|Prd|Prd|Prd|Prd|Prd|Prd|Prd|Prd|"
                        "Prd|Planet #|\n|Wk%|Wk%|Wk%|Wk%|Wk%|"
                        "Wk%|Wk%|Wk%|Wk%|Wk%|Wk%|Wk%|Wk%|Wk%|Wk%|"
                        "Wk%|Wk%|        |\n";
                    scanner = doProdCensus;
                    dashSize = 78;
                    break;

                /* geology report type */
                case 1:
                    pPtr = " PC S eff min gld pol gas wat mob   ore   bar  "
                        " TF  ResLv  Planet  Position\n";
                    scanner = doGeoCensus;
                    dashSize = 75;
                    break;

                /* default report type */
                default:
                    pPtr = "  PC  civl  scien    mil  offic  BTU  Pop% "
                        "  PF  Race   Pl#   Name\n";
                    scanner = doPopCensus;
                    dashSize = 71;
                    break;
            }
            /* display the header */
            user(IS, pPtr);
            /* and the correct length dashed line */
            if (IS->is_player.p_status == ps_deity)
            {
                dash(IS, (USHORT) dashSize + 4);
            }
            else
            {
                dash(IS, dashSize);
            }
            /* call scanPlanets on the list of planets they gave us */
            if (scanPlanets(IS, &ps, scanner) == 0)
            {
                err(IS, "no planets matched");
            }
            else
            {
                userNL(IS);
            }
        }
    }
}

/*
 * doEnumerate - dummy function used by cmd_enumerate
 */

void doEnumerate(IMP, register Planet_t *p)
{
    /* This function is just here to pass to scanPlanets */
}

/*
 * cmd_enumerate - returns the number of planets that meet the user supplied
 *          conditions
 */

void cmd_enumerate(IMP)
{
    PlanetScan_t ps;
    ULONG n;

    /* get a list of planets to check */
    if (reqPlanets(IS, &ps, "Enter planets specification to enumerate"))
    {
        /* real simple. Just call scanPlanets and display the number of */
        /* planets that meet the requirements */
        n = scanPlanets(IS, &ps, doEnumerate);
        userN(IS, n);
        user(IS, " planets matched.\n");
    }
}

/*
 * doCheckpoint - actually gets the new password for cmd_checkpoint
 */

void doCheckpoint(IMP, register Planet_t *p)
{
    register ULONG plNum;
    char *pswd, *q;
    char promptBuffer[40];

    /* save the number just in case */
    plNum = p->pl_number;
    /* Build up the prompt */
    userP(IS, "Enter new checkpoint code for planet ", p, "");
    /* This copies the prompt out of the output buffer into out local buffer */
    getPrompt(IS, &promptBuffer[0]);
    /* This displays it to the user properly */
    uPrompt(IS, &promptBuffer[0]);
    /* ask the user for the new code */
    if (clReadUser(IS))
    {
        /* trim leading space */
        pswd = skipBlanks(IS);
        /* if they enter a blank line, leave the code alone */
        if (*pswd == '\0')
        {
            user(IS, "Checkpoint unchanged\n");
            return;
        }
    }
    else
    {
        user(IS, "Checkpoint unchanged\n");
        return;
    }
    /* truncate the entered code to what we allow */
    q = pswd;
    while (*q != '\0')
    {
        q++;
    }
    if ((q - pswd) / sizeof(char) >= PLAN_PSWD_LEN)
    {
        pswd[PLAN_PSWD_LEN - 1] = '\0';
    }
    /* check if they want to remove a checkpoint code */
    if (strcmp(pswd, "none") == 0)
    {
        *pswd = '\0';
    }
    /* update the planet with the new code */
    server(IS, rt_lockPlanet, plNum);
    if (IS->is_request.rq_u.ru_planet.pl_btu < 2)
    {
	server(IS, rt_unlockPlanet, plNum);
        user(IS, "Insufficient BTU's - Checkpoint unchanged\n");
    }
    else
    {
	IS->is_request.rq_u.ru_planet.pl_btu -= 2;
	strcpy(&IS->is_request.rq_u.ru_planet.pl_checkpoint[0], pswd);
	server(IS, rt_unlockPlanet, plNum);
    }
}

/*
 * cmd_checkpoint - allows the player to set a checkpoint code for a range
 *          of planets
 */

BOOL cmd_checkpoint(IMP)
{
    PlanetScan_t ps;

    /* get a range of planets to set/remove codes on */
    if (reqPlanets(IS, &ps, "Checkpoint which planets"))
    {
        (void) skipBlanks(IS);
        /* make sure they are who we think they are */
        if (getPassword(IS, "Enter your password to verify",
            &IS->is_player.p_password[0]))
        {
            IS->is_textInPos = &IS->is_textIn[0];
            *IS->is_textInPos = '\0';
            /* scan the planets for matches, changing the codes if they do */
            if (scanPlanets(IS, &ps, doCheckpoint) == 0)
            {
                err(IS, "no planets matched");
            }
            return TRUE;
        }
        /* whoops, they didn't know the password */
        user(IS, "Invalid password - no planets checkpointed\n");
        return FALSE;
    }
    return FALSE;
}

/*
 * doUpdate - does the actual planet updating for cmd_update
 */

void doUpdate(IMP, register Planet_t *p)
{
    accessPlanet(IS, p->pl_number);
}

/*
 * cmd_update - allows the player to update a range of planets
 */

BOOL cmd_update(IMP)
{
    char *p, *q;
    register ULONG count;
    PlanetScan_t ps;

    /* get a list of planets to update */
    if (reqPlanets(IS, &ps, "Update which planets"))
    {
        p = skipBlanks(IS);
        /* if there is still more in the input buffer, look for flags */
        if (*p != '\0')
        {
            q = skipWord(IS);
            (void) skipBlanks(IS);
            *q = '\0';
            /* do they want hardly any output? */
            if (strcmp(p, "terse") == 0)
            {
                IS->is_quietUpdate = TRUE;
            }
            else
            {
                /* ok, how about lots of output? */
                if (strcmp(p, "verbose") == 0)
                {
                    IS->is_verboseUpdate = TRUE;
                }
            }
        }
        IS->is_textInPos = &IS->is_textIn[0];
        *IS->is_textInPos = '\0';
        /* update the planets specified */
        count = scanPlanets(IS, &ps, doUpdate);
        if (count == 0)
        {
            err(IS, "no planets matched");
        }
        else
        {
            userN(IS, count);
            user(IS, " planets updated.\n");
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * cmd_divvy - allows the player to divide up a planets "work"
 *          percentages.
 */

BOOL cmd_divvy(IMP)
{
    register Planet_t *rpl;
    register USHORT curPer;
    register USHORT oldAmt;
    ULONG plNum;
    short oldEff;
    Planet_t savePl;
    PProd_t prod;
    char buff[91];

    /* set up the pointer into the buffer */
    rpl = &IS->is_request.rq_u.ru_planet;
    /* get the planet number */
    if (reqPlanet(IS, &plNum, "Which planet do you wish to change the "
        "values for"))
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
        else if (rpl->pl_btu < 2)
        {
            user(IS, "Not enough BTUs on that planet for that command\n");
        }
        else
        {
            /* save a copy of the planet */
            savePl = *rpl;
            curPer = 0;
            /* save the efficiency */
            oldEff = (short)rpl->pl_efficiency;
            /* loop for everything but cash */
            for (prod = PPROD_FIRST; prod < PPROD_LAST; prod++)
            {
                sprintf(&buff[0], "Percent of work devoted to %s",
                    PPROD_NAME[prod]);
                /* store the current percentage */
                oldAmt = savePl.pl_workPer[prod];
                /* get the new percentage */
                savePl.pl_workPer[prod] = repNum(IS,
                    savePl.pl_workPer[prod], 0, 100 - curPer, &buff[0]);
                /* add to the current allocated work */
                curPer += savePl.pl_workPer[prod];
                /* is the amount different than it was before */
                if (oldAmt != savePl.pl_workPer[prod])
                {
                    /* yes, so decrease the efficiency by the difference */
                    oldEff -= abs(oldAmt - savePl.pl_workPer[prod]);
                }
            }
            /* can't be lower than 0% */
            if (oldEff < 0)
            {
                oldEff = 0;
            }
            /* copy the fields back */
            savePl.pl_workPer[PPROD_LAST] = 100 - curPer;
            savePl.pl_btu -= 2;
            savePl.pl_efficiency = oldEff;
            /* write the new planet */
            server(IS, rt_lockPlanet, plNum);
            *rpl = savePl;
            server(IS, rt_unlockPlanet, plNum);
            fePlDirty(IS, plNum);
        }
        return TRUE;
    }
    return FALSE;
}
