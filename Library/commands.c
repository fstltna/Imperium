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
 * $Id: commands.c,v 1.3 2000/05/25 18:21:23 marisa Exp $
 *
 * $Log: commands.c,v $
 * Revision 1.3  2000/05/25 18:21:23  marisa
 * Fix more T/F issues
 *
 * Revision 1.2  2000/05/22 07:28:03  marisa
 * .
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:52  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:24:29  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:19  marisag
 * patch20: Fix empty revision.
 *
 */

#include "../config.h"

#include <stdio.h>
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"

#define LIST_WIDTH  25

static const char rcsid[] = "$Id: commands.c,v 1.3 2000/05/25 18:21:23 marisa Exp $";

static const BOOL INACTIVE_ALLOWED[74] = {                  /*  9 */
    TRUE, TRUE,  TRUE, TRUE, TRUE, TRUE,  TRUE, TRUE, TRUE, TRUE,
    FALSE, TRUE,  FALSE, TRUE, TRUE, TRUE,  FALSE, TRUE, TRUE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE,
    FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE,
    TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, TRUE,
    FALSE, FALSE, FALSE, TRUE};

static const char *GLOB_COMMAND_LIST =
                /*  1 */ "bye\0"
                /*  2 */ "commands\0"
                /*  3 */ "info\0"
                /*  4 */ "list\0"
                /*  5 */ "status\0"
                /*  6 */ "player\0"
                /*  7 */ "read\0"
                /*  8 */ "telegram\0"
                /*  9 */ "message\0"
                /* 10 */ "help\0"
                /* 11 */ "?\0"
                /* 12 */ "examine\0"
                /* 13 */ "dump\0"
                /* 14 */ "edit\0"
                /* 15 */ "enumerate\0"
                /* 16 */ "chat\0"
                /* 17 */ "flush\0"
                /* 18 */ "tickle\0"
                /* 19 */ "log\0"
                /* 20 */ "doc\0"
                /* 21 */ "create\0"
                /* 22 */ "build\0"
                /* 23 */ "declare\0"
                /* 24 */ "lend\0"
                /* 25 */ "accept\0"
                /* 26 */ "repay\0"
                /* 27 */ "ledger\0"
                /* 28 */ "propaganda\0"
                /* 29 */ "newspaper\0"
                /* 30 */ "headlines\0"
                /* 31 */ "collect\0"
                /* 32 */ "change\0"
                /* 33 */ "census\0"
                /* 34 */ "checkpoint\0"
                /* 35 */ "update\0"
                /* 36 */ "map\0"
                /* 37 */ "scan\0"
                /* 38 */ "power\0"
                /* 39 */ "grant\0"
                /* 40 */ "discharge\0"
                /* 41 */ "ships\0"
                /* 42 */ "load\0"
                /* 43 */ "unload\0"
                /* 44 */ "tend\0"
                /* 45 */ "fleet\0"
                /* 46 */ "visual\0"
                /* 47 */ "install\0"
                /* 48 */ "remove\0"
                /* 49 */ "navigate\0"
                /* 50 */ "liftoff\0"
                /* 51 */ "land\0"
                /* 52 */ "race\0"
                /* 53 */ "refuel\0"
                /* 54 */ "divvy\0"
                /* 55 */ "verify\0"
                /* 56 */ "name\0"
                /* 57 */ "program\0"
                /* 58 */ "run\0"
                /* 59 */ "setup\0"
                /* 60 */ "show\0"
                /* 61 */ "version\0"
                /* 62 */ "price\0"
                /* 63 */ "report\0"
                /* 64 */ "buy\0"
                /* 65 */ "sell\0"
                /* 66 */ "donate\0"
                /* 67 */ "realm\0"
                /* 68 */ "refurbish\0"
                /* 69 */ "plate\0"
                /* 70 */ "teleport\0"
                /* 71 */ "miner\0"
                /* 72 */ "configure\0"
                /* 73 */ "fire\0"
                /* 74 */ "debug\0";

/*
 * list - print one string as part of the three-column help listing.
 */

void list(IMP, register char *p, register USHORT cmd, BOOL needDeity)
{
    if ((IS->is_player.p_status == ps_deity) || (IS->is_player.p_status ==
        ps_active) || INACTIVE_ALLOWED[cmd - 1])
    {
        if ((IS->is_player.p_status == ps_deity) || !needDeity)
        {
            user(IS, p);
            if (IS->is_textOutPos > LIST_WIDTH * 2)
            {
                userNL(IS);
            }
            else
            {
                while (IS->is_textOutPos % LIST_WIDTH != 0)
                {
                    userSp(IS);
                }
            }
        }
    }
}

/*
 * cmd_list - print a list of currently implemented commands.
 */

void cmd_list(IMP)
{
    user(IS, "Imperium commands you are allowed to use are:\n\n");
    list(IS, "accept - 1/0", 25, FALSE);
    list(IS, "bye", 1, FALSE);
    list(IS, "build - 2/5", 22, FALSE);
    list(IS, "buy - 2/0", 64, FALSE);
    list(IS, "census", 33, FALSE);
    list(IS, "change", 32, FALSE);
    list(IS, "chat", 16, FALSE);
    list(IS, "checkpoint - 1/2", 34, FALSE);
    list(IS, "collect - 3/0", 31, FALSE);
    list(IS, "commands", 2, FALSE);
    list(IS, "configure - 1/0", 72, FALSE);
    list(IS, "create", 21, TRUE);
    list(IS, "declare - 3/0", 23, FALSE);
    list(IS, "discharge - 2/3", 40, FALSE);
    list(IS, "divvy - 1/2", 54, FALSE);
    list(IS, "doc", 20, FALSE);
    list(IS, "donate - 2/1", 66, FALSE);
    list(IS, "dump", 13, FALSE);
    list(IS, "edit", 14, TRUE);
    list(IS, "enumerate", 15, FALSE);
    list(IS, "examine", 12, TRUE);
    list(IS, "fire - 2/0", 73, FALSE);
    list(IS, "fleet", 45, FALSE);
    list(IS, "flush", 17, FALSE);
    list(IS, "grant - 2/2", 39, FALSE);
    list(IS, "headlines", 30, FALSE);
    list(IS, "help", 10, FALSE);
    list(IS, "info", 3, FALSE);
    list(IS, "install - 2/0", 47, FALSE);
    list(IS, "ledger", 27, FALSE);
    list(IS, "land - 2/0", 51, FALSE);
    list(IS, "lend - 2/0", 24, FALSE);
    list(IS, "liftoff - 2/0", 50, FALSE);
    list(IS, "list", 4, FALSE);
    list(IS, "load - 2/0", 42, FALSE);
    list(IS, "log", 19, FALSE);
    list(IS, "map", 36, TRUE);
    list(IS, "message", 9, FALSE);
    list(IS, "miner 2/0", 71, FALSE);
    list(IS, "name", 56, FALSE);
    list(IS, "navigate - 2/0", 29, FALSE);
    list(IS, "newspaper", 29, FALSE);
    list(IS, "plate - 2/2", 69, FALSE);
    list(IS, "player", 6, FALSE);
    list(IS, "power", 38, FALSE);
    list(IS, "price - 1/0", 62, FALSE);
    list(IS, "program - 1/0", 57, FALSE);
    list(IS, "propaganda - 5/0", 28, FALSE);
    list(IS, "race", 52, FALSE);
    list(IS, "read", 7, FALSE);
    list(IS, "realm", 67, FALSE);
    list(IS, "refuel - 1/0", 53, FALSE);
    list(IS, "refurbish - 2/2", 68, FALSE);
    list(IS, "remove - 2/0", 48, FALSE);
    list(IS, "repay - 1/0", 26, FALSE);
    list(IS, "report - 1/0", 63, FALSE);
    list(IS, "run - 2/0", 58, FALSE);
    list(IS, "sell - 2/0", 65, FALSE);
    list(IS, "setup", 59, FALSE);
    list(IS, "scan - 2/1", 37, FALSE);
    list(IS, "ships", 41, FALSE);
    list(IS, "show", 60, FALSE);
    list(IS, "status", 5, FALSE);
    list(IS, "telegram", 8, FALSE);
    list(IS, "teleport - 2/0", 70, FALSE);
    list(IS, "tend - 2/0", 44, FALSE);
    list(IS, "tickle", 18, TRUE);
    list(IS, "unload - 2/2", 43, FALSE);
    list(IS, "update - 2/0", 35, FALSE);
    list(IS, "verify", 55, TRUE);
    list(IS, "version", 61, FALSE);
    list(IS, "visual - 2/0", 46, FALSE);
    list(IS, "?", 11, FALSE);
    user(IS, "\nUse 'help <command>' to get help on a particular command.\n"
         "For example 'help doc'.\nAlso try 'help topics'.\n");
}

/*
 * cmd_help - try to print a help or doc file on a given subject.
 */

void cmd_help(IMP, BOOL isHelp)
{
    register char *inputPos, *wordStart;
    char *wordEnd;
    BOOL result;

    inputPos = IS->is_textInPos;
    if (*inputPos == '\0')
    {
        cmd_list(IS);
    }
    else
    {
        while (*inputPos != '\0')
        {
            wordStart = inputPos;
            wordEnd = skipWord(IS);
            inputPos = skipBlanks(IS);
            *wordEnd = '\0';
            if (isHelp)
            {
                result = !printFile(IS, wordStart, ft_help);
            }
            else
            {
                result = !printFile(IS, wordStart, ft_doc);
            }
            if (result)
            {
                if (isHelp)
                {
                    user3(IS, "No help for topic \"", wordStart, "\"\n");
                }
                else
                {
                    user3(IS, "No doc for topic \"", wordStart, "\"\n");
                }
            }
        }
    }
}

/*
 * chargeCommand - try a command, if user can afford it. Add the BTU's back
 *      if the command returns 'FALSE'.
 */

void chargeCommand(IMP, BOOL (command)(IMP), USHORT cost)
{
    if (IS->is_player.p_status == ps_deity)
    {
        (void)command(IS);
        return;
    }
    if (IS->is_player.p_btu < cost)
    {
        user(IS, "You don't have enough BTU's remaining!\n");
        return;
    }
    IS->is_player.p_btu -= cost;
    if (!command(IS))
    {
        IS->is_player.p_btu += cost;
    }
    else
    {
        server(IS, rt_lockPlayer, IS->is_player.p_number);
        IS->is_request.rq_u.ru_player.p_btu = IS->is_player.p_btu;
        server(IS, rt_unlockPlayer, IS->is_player.p_number);
        IS->is_player = IS->is_request.rq_u.ru_player;
    }
}

/*
 * doCommand - attempt to execute the given single command.
 */

void doCommand(IMP, USHORT cmd, char *command)
{
    if ((IS->is_player.p_status != ps_deity) && (IS->is_player.p_status !=
        ps_active) && !INACTIVE_ALLOWED[cmd - 2])
    {
        user(IS, "You must be an active player to use that command.\n");
        return;
    }
    (void) clGotCtrlC(IS);
    switch(cmd)
    {
        case 3:
            cmd_list(IS);
            break;
        case 4:
            cmd_info(IS);
            break;
        case 5:
            cmd_list(IS);
            break;
        case 6:
            cmd_status(IS);
            break;
        case 7:
            cmd_player(IS);
            break;
        case 8:
            cmd_read(IS);
            break;
        case 9:
            cmd_telegram(IS);
            break;
        case 10:
            cmd_message(IS);
            break;
        case 11:
            cmd_help(IS, TRUE);
            break;
        case 12:
            cmd_list(IS);
            break;
        case 13:
            cmd_examine(IS);
            break;
        case 14:
            cmd_dump(IS);
            break;
        case 15:
            cmd_edit(IS);
            break;
        case 16:
            cmd_enumerate(IS);
            break;
        case 17:
            cmd_chat(IS);
            break;
        case 18:
            cmd_flush(IS);
            break;
        case 19:
            cmd_tickle(IS);
            break;
        case 20:
            cmd_log(IS);
            break;
        case 21:
            cmd_help(IS, FALSE);
            break;
        case 22:
            cmd_create(IS);
            break;
        case 23:
            chargeCommand(IS, cmd_build, 2);
            break;
        case 24:
            chargeCommand(IS, cmd_declare, 3);
            break;
        case 25:
            chargeCommand(IS, cmd_lend, 2);
            break;
        case 26:
            chargeCommand(IS, cmd_accept, 1);
            break;
        case 27:
            chargeCommand(IS, cmd_repay, 1);
            break;
        case 28:
            cmd_ledger(IS);
            break;
        case 29:
            chargeCommand(IS, cmd_propaganda, 5);
            break;
        case 30:
            cmd_newspaper(IS);
            break;
        case 31:
            cmd_headlines(IS);
            break;
        case 32:
            chargeCommand(IS, cmd_collect, 3);
            break;
        case 33:
            cmd_change(IS);
            break;
        case 34:
            cmd_census(IS);
            break;
        case 35:
            chargeCommand(IS, cmd_checkpoint, 1);
            break;
        case 36:
            chargeCommand(IS, cmd_update, 2);
            break;
        case 37:
            (void) cmd_map(IS);
            break;
        case 38:
            chargeCommand(IS, cmd_scan, 2);
            break;
        case 39:
            (void) cmd_power(IS);
            break;
        case 40:
            chargeCommand(IS, cmd_grant, 2);
            break;
        case 41:
            chargeCommand(IS, cmd_discharge, 2);
            break;
        case 42:
            cmd_ships(IS);
            break;
        case 43:
            chargeCommand(IS, cmd_load, 2);
            break;
        case 44:
            chargeCommand(IS, cmd_unload, 2);
            break;
        case 45:
            chargeCommand(IS, cmd_tend, 2);
            break;
        case 46:
            cmd_fleet(IS);
            break;
        case 47:
            chargeCommand(IS, cmd_visual, 2);
            break;
        case 48:
            chargeCommand(IS, cmd_install, 2);
            break;
        case 49:
            chargeCommand(IS, cmd_remove, 2);
            break;
        case 50:
            chargeCommand(IS, cmd_navigate, 2);
            break;
        case 51:
            chargeCommand(IS, cmd_liftoff, 2);
            break;
        case 52:
            chargeCommand(IS, cmd_land, 2);
            break;
        case 53:
            cmd_race(IS);
            break;
        case 54:
            chargeCommand(IS, cmd_refuel, 1);
            break;
        case 55:
            chargeCommand(IS, cmd_divvy, 1);
            break;
        case 56:
            cmd_verify(IS);
            break;
        case 57:
            cmd_name(IS);
            break;
        case 58:
            chargeCommand(IS, cmd_program, 1);
            break;
        case 59:
            chargeCommand(IS, cmd_run, 1);
            break;
        case 60:
            cmd_setup(IS);
            break;
        case 61:
            cmd_show(IS);
            break;
        case 62:
            cmd_version(IS);
            break;
        case 63:
            chargeCommand(IS, cmd_price, 1);
            break;
        case 64:
            chargeCommand(IS, cmd_report, 1);
            break;
        case 65:
            chargeCommand(IS, cmd_buy, 2);
            break;
        case 66:
            chargeCommand(IS, cmd_sell, 2);
            break;
        case 67:
            chargeCommand(IS, cmd_donate, 2);
            break;
        case 68:
            cmd_realm(IS);
            break;
        case 69:
            chargeCommand(IS, cmd_refurb, 2);
            break;
        case 70:
            chargeCommand(IS, cmd_plate, 2);
            break;
        case 71:
            chargeCommand(IS, cmd_teleport, 2);
            break;
        case 72:
            /* note that the command charges BTUs internally */
            cmd_miner(IS);
            break;
        case 73:
            chargeCommand(IS, cmd_configure, 2);
            break;
        case 74:
            chargeCommand(IS, cmd_fire, 2);
            break;
        case 75:
#ifdef DEBUG_LIB
            if (IS->is_DEBUG)
            {
                user(IS, "Debug mode is now OFF\n");
                IS->is_DEBUG = FALSE;
            }
            else
            {
                user(IS, "Debug mode is now ON\n");
                IS->is_DEBUG = TRUE;
            }
#else
            user(IS, "Debugging is not enabled.\n");
#endif
            break;
        default:
            user3(IS, "Unimplemented Imperium command: ", command, "\n");
            break;
    }
}

/*
 * processCommands - loop, getting commands and parsing them.
 */

void processCommands(IMP)
{
    char *command, *p;
    long income;
    USHORT cmd = 0, lastWarn, lastBack, lastTime;
    BOOL cont, badPow, doWarn, doBack, needPrompt;
    char promptBuff[25];

    /* initialize our crud */
    cont = TRUE;
    badPow = FALSE;
    doWarn = FALSE;
    doBack = FALSE;
    lastBack = 0;
    lastWarn = 0;
    lastTime = 0xffff;
    needPrompt = TRUE;

    while (cont)
    {
        if (IS->is_request.rq_specialFlags & ISF_OFF_IMM)
        {
            user(IS, "*** Imperium server forcing you off to prevent your "
                "files from being corrupted ***\n");
            return;
        }
        if (badPow)
        {
            user(IS, "*** Please log out IMMEDIATELY - The system is "
                "currently operating on ***\n*** it's reserve battery power "
                "and may shut down at any time          ***\n");
	    needPrompt = TRUE;
        }
        if (doBack)
        {
            if (lastBack == 0)
            {
                user(IS, "*** Please log out - The system is about to be "
                    "backed up ***\n");
                lastBack++;
		needPrompt = TRUE;
            }
        }
        if (doWarn)
        {
            if (lastWarn == 0)
            {
                user(IS, "*** Please log out - The system is preparing to "
                    "shut down ***\n");
                lastWarn++;
		needPrompt = TRUE;
            }
        }
        if (IS->is_player.p_btu < IS->is_player.p_btuWarn)
        {
            user(IS, "Your BTU's are getting low\n");
	    needPrompt = TRUE;
        }
        if (IS->is_player.p_timeLeft < IS->is_player.p_timeWarn)
        {
            user(IS, "Your time left is getting low\n");
	    needPrompt = TRUE;
        }
	if (needPrompt)
	{
            (void) sprintf(&promptBuff[0], "[%hu:%lu] Command",
        	IS->is_player.p_btu, IS->is_player.p_timeLeft);
            uPrompt(IS, &promptBuff[0]);
	}
        if (!clTimedReadUser(IS))
        {
            userNL(IS);
            return;
        }
	needPrompt = TRUE;
	if (IS->is_argShort == 0)
	{
	    needPrompt = FALSE;
	    cmd = 0;
	}
	else
	{
            command = skipBlanks(IS);
            if (*command != '\0')
            {
                p = skipWord(IS);
                (void) skipBlanks(IS);
                *p = '\0';
                IS->is_quietUpdate = TRUE;
                IS->is_verboseUpdate = FALSE;
                IS->is_contractEarnings = 0;
                IS->is_interestEarnings = 0;
                IS->is_improvementCost = 0;
                IS->is_militaryCost = 0;
                IS->is_utilitiesCost = 0;
                /* Prevent any problems of sticky global BOOL settings */
                IS->is_BOOL1 = FALSE;
                IS->is_BOOL2 = FALSE;
                cmd = lookupCommand(GLOB_COMMAND_LIST, command);
                if (cmd == 0)
                {
                    user3(IS, "Unknown Imperium command: ", command, "\n");
                }
                else
                {
                    if (cmd == 1)
                    {
                        user3(IS, "Ambiguous Imperium command: ", command, "\n");
                    }
                    else
                    {
                        if (cmd == 2)
                        {
                            /* bye */
                            cont = FALSE;
                        }
                        else
                        {
                            doCommand(IS, cmd, command);
                        }
                    }
                }
                if (IS->is_contractEarnings != 0)
                {
                    userN3(IS, "You earned $", IS->is_contractEarnings,
                           " from contracted production.\n");
                }
                if (IS->is_interestEarnings != 0)
                {
                    userN3(IS, "You earned $", IS->is_interestEarnings,
                           " interest from bank deposits.\n");
                }
                if (IS->is_improvementCost != 0)
                {
                    userN3(IS, "You paid $", IS->is_improvementCost,
                           " for planetary improvements.\n");
                }
                if (IS->is_militaryCost != 0)
                {
                    userN3(IS, "You paid $", IS->is_militaryCost,
                           " for military payroll.\n");
                }
                if (IS->is_utilitiesCost != 0)
                {
                    userN3(IS, "You paid $", IS->is_utilitiesCost,
                           " for utilities.\n");
                }
                income = IS->is_contractEarnings + IS->is_interestEarnings -
                        IS->is_improvementCost - IS->is_militaryCost -
                        IS->is_utilitiesCost;
                if (income != 0)
                {
                    server(IS, rt_lockPlayer, IS->is_player.p_number);
                    IS->is_request.rq_u.ru_player.p_money += income;
                    server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    /* The money was already put here by 'updatePlanet', but
                       doing it again will reflect changes from any other
                       simultaneous players */
                    IS->is_player.p_money =
                        IS->is_request.rq_u.ru_player.p_money;
                }
            }
        }
        if (cmd != 9)
        {
            messageCheck(IS);
	    if (IS->is_argShort == 1)
	    {
		needPrompt = TRUE;
	    }
            if (IS->is_request.rq_specialFlags != ISF_NONE)
            {
                if (IS->is_request.rq_specialFlags & ISF_BACKUP)
                {
                    if (!doBack)
                    {
                        doBack = TRUE;
                        lastBack = 0;
                    }
                    else
                    {
                        lastBack++;
                        if (lastBack > 4)
                        {
                            lastBack = 0;
                        }
                    }
                }
                if (IS->is_request.rq_specialFlags & ISF_SERV_DOWN)
                {
                    if (!doWarn)
                    {
                        doWarn = TRUE;
                        lastWarn = 0;
                    }
                    else
                    {
                        lastWarn++;
                        if (lastWarn > 5)
                        {
                            lastWarn = 0;
                        }
                    }
                }
                if (IS->is_request.rq_specialFlags & (ISF_POW_WARN |
                    ISF_POW_CRIT))
                {
                    badPow = TRUE;
                }
            }
            if (badPow && ((IS->is_request.rq_specialFlags & (ISF_POW_WARN |
                ISF_POW_CRIT)) == 0))
            {
                badPow = FALSE;
                user(IS, "*** Previous power outage has been corrected. "
                    "You do not need to log out ***\n");
		needPrompt = TRUE;
            }
            if (doWarn && ((IS->is_request.rq_specialFlags & ISF_SERV_DOWN) ==
                0))
            {
                doWarn = FALSE;
                user(IS, "*** System shutdown cancled - "
                    "You do not need to log out ***\n");
		needPrompt = TRUE;
            }
	}
        if (updateTimer(IS))
        {
            /* user's timer has expired! Log him out! */
            user(IS, "You've run out of time! Reconnect later.\n");
            return;
        }
    }
}
