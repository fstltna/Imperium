/*
 * Imperium
 *
 * $Id: cmd_action.c,v 1.2 2000/05/18 06:50:01 marisa Exp $
 *
 * code related to setting/handling "actions"
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
 * $Log: cmd_action.c,v $
 * Revision 1.2  2000/05/18 06:50:01  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:46  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:59:01  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:24:09  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:00  marisag
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
#include "../Include/ImpFeMess.h"
#include "Scan.h"
#include "ImpPrivate.h"

static char const rcsid[] = "$Id: cmd_action.c,v 1.2 2000/05/18 06:50:01 marisa Exp $";

#ifdef FOR_REFERENCE

typedef struct
    {
        ActionType_t
            ac_action;
                                /* The ac_items field holds the percentage  */
                                /* or the actual number of items that the   */
                                /* action is to refer to                    */
        USHORT
            ac_items[IT_LAST + 1];      /* item counts              */
    } Action_t;


#endif

/*
 * displayActionTypes - displays the name and summary for the actions
 */

void displayActionTypes(IMP)
{
    user(IS, "Valid actions are:\n");
    user(IS, "   none - does nothing\n");
    user(IS, "   load - loads items onto ship\n");
    user(IS, " unload - unloads items from ship\n");
    user(IS, "%unload - unloads a percentage of the ships cargo\n");
    user(IS, " LRscan - does a long-range scan\n");
    user(IS, " SRscan - does a short-range scan\n");
    user(IS, " VRscan - does a visual range scan\n");
    user(IS, "   land - attempt to land on planet\n");
    user(IS, "   lift - attempt to lift off from planet\n");
    user(IS, " teleDn - attempt to teleport down to planet\n");
    user(IS, " teleUp - attempt to teleport up from planet\n\n");
}

/*
 * reqActionType - selects a valid action type
 */

BOOL reqActionType(IMP, ActionType_t *act, char *prompt)
{
    USHORT temp;

    temp = *act;
    if (reqChoice(IS, &temp, "none\0load\0unload\0%unload\0lrscan\0"
        "srscan\0vrscan\0land\0lift\0teledn\0teleup\0", prompt))
    {
        *act = temp;
        return TRUE;
    }
    return FALSE;
}

static char const *itName[] =
    {
        "civilians",
        "scientists",
        "military",
        "officers",
        "missiles",
        "planes",
        "ore",
        "gold bars",
        "air tanks",
        "fuel tanks"
    };
/*
 * cmd_setup - allows a player to set up "actions"
 */

void cmd_setup(IMP)
{
    char actionChar;
    ItemType_t it;
    ActionType_t act;
    USHORT action;
    char buff[255];

    /* find out which action they want to work with */
    if (reqChar(IS, &actionChar, "abcdefghij", "Set up which action",
        "illegal action character"))
    {
        /* skip over any blank areas */
        (void) skipBlanks(IS);
        if (*IS->is_textInPos == '\0')
        {
            /* show them the defined action types */
            displayActionTypes(IS);
        }
        action = actionChar - 'a';
        if (reqActionType(IS, &act, "Which action"))
        {
            IS->is_player.p_action[action].ac_action = act;
            switch(act)
            {
                case a_load:
                case a_unload:
                case a_teleDn:
                case a_teleUp:
                    /* loop through items */
                    for (it = IT_FIRST; it <= IT_LAST_SMALL; it++)
                    {
                        sprintf(&buff[0], "Number of %s to ", itName[it]);
                        switch(act)
                        {
                            case a_load:
                                strcat(&buff[0], "load");
                                break;
                            case a_unload:
                                strcat(&buff[0], "unload");
                                break;
                            case a_teleDn:
                                strcat(&buff[0], "beam down");
                                break;
                            case a_teleUp:
                                strcat(&buff[0], "beam up");
                                break;
                        }
                        /* prompt them to change it */
                        IS->is_player.p_action[action].ac_items[it] =
                            repNum(IS, 0, 0, MAX_WORK, &buff[0]);
                    }
                    break;
                case a_unloadpct:
                    /* loop through items */
                    for (it = IT_FIRST; it <= IT_LAST_SMALL; it++)
                    {
                        sprintf(&buff[0], "Percent of %s to unload",
                            itName[it]);
                        /* prompt them to change it */
                        IS->is_player.p_action[action].ac_items[it] =
                            repNum(IS, 0, 0, 100, &buff[0]);
                    }
                    break;
                default:
                    /* the other types do not need their values set     */
                    /* clear them out to avoid possible problems later  */
                    for (it = IT_FIRST; it <= IT_LAST_SMALL; it++)
                    {
                        IS->is_player.p_action[action].ac_items[it] = 0;
                    }
                    break;
            }
            server(IS, rt_lockPlayer, IS->is_player.p_number);
            IS->is_request.rq_u.ru_player.p_action[action] =
                IS->is_player.p_action[action];
            server(IS, rt_unlockPlayer, IS->is_player.p_number);
        }
    }
}

/*
 * cmd_show - displays an already created action
 */

void cmd_show(IMP)
{
    register USHORT which;
    BOOL doNum;
    char actionChar;
    char *sPtr;
    ItemType_t it;
    char buff[255];

    doNum = TRUE;
    /* find out which action they want to work with */
    if (reqChar(IS, &actionChar, "abcdefghij", "Show which action",
        "illegal action character"))
    {
        which = actionChar - 'a';
        switch(IS->is_player.p_action[which].ac_action)
        {
            case a_none:
                sPtr = "*no action*";
                doNum = FALSE;
                break;
            case a_load:
                sPtr = "load items";
                break;
            case a_unload:
                sPtr = "unload items";
                break;
            case a_unloadpct:
                sPtr = "unload percentage of items";
                break;
            case a_LRscan:
                sPtr = "long range scan";
                doNum = FALSE;
                break;
            case a_SRscan:
                sPtr = "short range scan";
                doNum = FALSE;
                break;
            case a_VRscan:
                sPtr = "visual range scan";
                doNum = FALSE;
                break;
            case a_land:
                sPtr = "land on planet";
                doNum = FALSE;
                break;
            case a_lift:
                sPtr = "lift off";
                doNum = FALSE;
                break;
            case a_teleDn:
                sPtr = "teleport items down";
                break;
            case a_teleUp:
                sPtr = "teleport items up";
                break;
            default:
                sPtr = "";
                doNum = FALSE;
                break;
        }
        if (*sPtr != '\0')
        {
            sprintf(&buff[0], "Action type: %s\n", sPtr);
            user(IS, &buff[0]);
            if (doNum)
            {
                user(IS, "|civil|scien|mili|offic||missl|plane| ore |bars |"
                    "airtn|ftank|\n|");
                /* loop through the items */
                for (it = IT_FIRST; it <= IT_LAST_SMALL; it++)
                {
                    /* get the current quantity of items */
                    userF(IS, IS->is_player.p_action[which].ac_items[it], 5);
                    userC(IS, '|');
                }
                userNL(IS);
            }
        }
    }
}

/*
 * handleScan - the "wrapper" for the SR & LR scan functions
 */

void handleScan(IMP, void (command)(IMP, USHORT, USHORT, USHORT))
{
    register USHORT i;
    register Nav_t *nav;
    Ship_t sShip;

    for (i = 0; i < IS->is_movingShipCount; i++)
    {
        nav = &IS->is_movingShips[i];
        if (nav->n_active)
        {
            server(IS, rt_readShip, nav->n_ship);
            sShip = IS->is_request.rq_u.ru_ship;
            command(IS, sShip.sh_row, sShip.sh_col,
                getShipVRange(IS, &sShip) *
                sShip.sh_efficiency * (100 - (100 -
                getShipTechFactor(IS, &sShip, bp_sensors))
                / 2) / 100);
        }
    }
}

/*
 * handleAction - entry point for the various actions. Assumes that the
 *          ships you wish this action to take place for are in the
 *          global nav list.
 *          Returns FALSE if the action can not be completed
 */

BOOL handleAction(IMP, char actCh)
{
    USHORT which;
    ActionType_t act;

    which = actCh - 'a';
    act = IS->is_player.p_action[which].ac_action;
    switch(act)
    {
        /* see if it is a "dead" action */
        case  a_none:
            /* it is, so let them know about it */
            user(IS, "Ship tried to execute an action that no longer "
                "exists\n");
            return FALSE;

        case a_load:
            user(IS, "load items\n");
            return TRUE;
        case a_unload:
            user(IS, "unload items\n");
            return TRUE;
        case a_unloadpct:
            user(IS, "unload percentage of items\n");
            return TRUE;
        case a_LRscan:
            handleScan(IS, doLRScan);
            return TRUE;
        case a_SRscan:
            handleScan(IS, doSRScan);
            return TRUE;
        case a_VRscan:
            /* note that right now only ships are seen! */
            handleScan(IS, visShips);
            return TRUE;
        case a_land:
            return doLand(IS);
        case a_lift:
            return doLiftoff(IS);
        case a_teleDn:
            user(IS, "teleport items down\n");
            return TRUE;
        case a_teleUp:
            user(IS, "teleport items up\n");
            return TRUE;
        default:
            err(IS, "unknown action");
            return FALSE;
    }
}
