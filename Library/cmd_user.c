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
 * code related to displaying player-related stats
 *
 * $Id: cmd_user.c,v 1.2 2000/05/18 06:50:02 marisa Exp $
 *
 * $Log: cmd_user.c,v $
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:52  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:59:16  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:24:27  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:16  marisag
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

static char const rcsid[] = "$Id: cmd_user.c,v 1.2 2000/05/18 06:50:02 marisa Exp $";

/*
 * printPlayer - part of cmd_player
 */

void printPlayer(IMP, USHORT i, BOOL onlyOn)
{
    register Player_t *p;
    char *actPtr;
    char *racePtr;
    char actChar;
    char tempBuf[81];
    char timeBuf[30];
    ULONG t;

    server(IS, rt_readPlayer, i);
    p = &IS->is_request.rq_u.ru_player;
    if (p->p_loggedOn || !onlyOn)
    {
        fePlayList(IS);
        if (p->p_loggedOn)
        {
            if (p->p_inChat)
            {
                actChar = '<';  /* User is in chat mode */
            }
            else
            {
                actChar = '*';  /* User is not in chat mode */
            }
        }
        else
        {
            actChar = ' ';      /* User is not online   */
        }
        /* build up the time the user was last on into a string */
        t = p->p_lastOn;
        strcpy(&timeBuf[0], ctime(&t));
        /* eliminate the '\n' that ctime() adds */
        timeBuf[strlen(&timeBuf[0]) - 1] = '\0';
        switch (p->p_status)
        {
            case ps_deity:
                actPtr = "DEITY";
                break;
            case ps_active:
                actPtr = "Active";
                break;
            case ps_quit:
                actPtr = "Resigned";
                break;
            case ps_idle:
                actPtr = "Idle";
                break;
            case ps_visitor:
                actPtr = "Visitor";
                break;
            default:
                actPtr = "*UNKNOWN*";
                break;
        }
        if (p->p_race != NO_RACE)
        {
            racePtr = &IS->is_world.w_race[p->p_race].r_name[0];
        }
        else
        {
            racePtr = "NONE";
        }
        (void) sprintf(&tempBuf[0],
"%2.2u  %c%-20.20s  [%4.4u] [%3.3u] %-9.9s %-6.6s %-20.20s\n",
            i, actChar, &timeBuf[0], p->p_timeLeft, p->p_btu, actPtr,
            racePtr, &p->p_name[0]);
        user(IS, &tempBuf[0]);
    }
}

/*
 * strLess - compare strings, ignoring case. Return 'true' if the first is
 *      less than the second.
 */

BOOL strLess(register char *a, register char *b)
{
    register char a1, b1;

    a1 = *a;
    if ((a1 >= 'a') && (a1 <= 'z'))
    {
        a1 = a1 - 'a' + 'A';
    }
    b1 = *b;
    if ((b1 >= 'a') && (b1 <= 'z'))
    {
        b1 = b1 - 'a' + 'A';
    }
    while((*a != '\0') && (a1 == b1))
    {
        a += sizeof(char);
        b += sizeof(char);
        a1 = *a;
        if ((a1 >= 'a') && (a1 <= 'z'))
        {
            a1 = a1 - 'a' + 'A';
        }
        b1 = *b;
        if ((b1 >= 'a') && (b1 <= 'z'))
        {
            b1 = b1 - 'a' + 'A';
        }
    }
    return (a1 < b1);
}

void cmd_player(IMP)
{
    typedef struct
        {
            char ul_name[NAME_LEN];
            USHORT ul_which;
            PlayerStatus_t ul_status;
        } playerList_t;
    playerList_t names[PLAYER_MAX];
    playerList_t ulTemp;
    USHORT playerCount;
    register short i, j;
    BOOL onlyOn;
    char *ptr;

    onlyOn = FALSE;
    ptr = IS->is_textInPos;
    if (*ptr != '\0')
    {
        if (strcmp(ptr, "on") == 0)
        {
           onlyOn = TRUE;
        }
    }
    user(IS, "Current time ");
    uTime(IS, IS->is_request.rq_time);
    user(IS, ", world created ");
    uTime(IS, IS->is_world.w_buildDate);
    userNL(IS);
    user(IS, " #   last access            time  BTU's status  "
        "  race   player name\n");
    dash(IS, 68);
    if (IS->is_player.p_status == ps_deity)
    {
        for (i = 0; i < IS->is_world.w_maxPlayers; i++)
        {
            printPlayer(IS, i, onlyOn);
        }
    }
    else
    {
        playerCount = IS->is_world.w_currPlayers;
        for (i = playerCount - 1; i >= 0; i--)
        {
            server(IS, rt_readPlayer, i);
            strcpy(names[i].ul_name, IS->is_request.rq_u.ru_player.p_name);
            names[i].ul_which = i;
            names[i].ul_status = IS->is_request.rq_u.ru_player.p_status;
        }
        if (playerCount > 1)
        {
            for (i = 0; i < playerCount - 1; i++)
            {
                for (j = i; j >= 0; j--)
                {
                    if (strLess(&names[j].ul_name[0],
                               &names[j + 1].ul_name[0]))
                    {
                        ulTemp = names[j];
                        names[j] = names[j + 1];
                        names[j + 1] = ulTemp;
                    }
                }
            }
        }
        i = playerCount;
        while (!clGotCtrlC(IS) && (i != 0))
        {
            i--;
            if (names[i].ul_status != ps_idle)
            {
               printPlayer(IS, names[i].ul_which, onlyOn);
            }
        }
    }
    userNL(IS);
}

/*
 * getMyRelation - returns the relation of the current player to the player
 *          in the structure
 */

Relation_t getMyRelation(IMP, register Player_t *p)
{
    /* switch based on the players relation to the other player */
    switch(IS->is_player.p_playrel[p->p_number])
    {
        /* these three are direct relations, so return them */
        case r_allied:
        case r_war:
        case r_neutral:
            return IS->is_player.p_playrel[p->p_number];
        case r_default:
            /* The player has not set a relation to this player, */
            /* so we have to look for a race relation */
            /* Make sure the player belongs to a race (Deity?) */
            if (p->p_race != NO_RACE)
            {
                switch(IS->is_player.p_racerel[p->p_race])
                {
                    case r_allied:
                    case r_neutral:
                    case r_war:
                        return IS->is_player.p_racerel[p->p_race];
                    default:
                        return r_neutral;
                }
            }
            return r_neutral;
        default:
            return r_neutral;
    }
}

Relation_t relationTo(USHORT playNum, UBYTE playRace,
	register Player_t *p)
{
    switch(p->p_playrel[playNum])
    {
        case r_allied:
        case r_war:
        case r_neutral:
            return(p->p_playrel[playNum]);
	    break;
        case r_default:
            if (playRace != NO_RACE)
            {
                switch(p->p_racerel[playRace])
                {
                    case r_allied:
                    case r_neutral:
                    case r_war:
                        return(p->p_racerel[playRace]);
			break;
                    default:
                        return(r_neutral);
			break;
                }
            }
            return(r_neutral);
	    break;
        default:
            return(r_neutral);
	    break;
    }
}

/*
 * getYourRelation - returns the relation of the player in the structure
 *          to the current player
 */

Relation_t getYourRelation(IMP, register Player_t *p)
{
    return(relationTo(IS->is_player.p_number, IS->is_player.p_race,
	p));
}

/*
 * cmd_status - Displays the status of the current player
 */

void cmd_status(IMP)
{
    register Player_t *p;
    register USHORT i;
    char *ptr;
    BOOL hadOne;
    ULONG homePl;

    if (IS->is_player.p_race != NO_RACE)
    {
        ptr = &IS->is_world.w_race[IS->is_player.p_race].r_name[0];
        homePl = IS->is_world.w_race[IS->is_player.p_race].r_homePlanet;
    }
    else
    {
        ptr = "NONE";
        homePl = NO_ITEM;
    }
    fePlayStat(IS);
    user3(IS, "Status of ", &IS->is_player.p_name[0], ", of the ");
    user2(IS, ptr, " race, on ");
    uTime(IS, IS->is_request.rq_time);
    user(IS, ":\n\n");
    userN3(IS, "Number of planets: ", IS->is_player.p_planetCount, "\n");
    if (homePl != NO_ITEM)
    {
	if (IS->is_player.p_feMode & FE_WANT_MISC)
	{
	    user(IS, FE_PLAYHOME);
	}
        userN3(IS, "Home planet: ", homePl, "\n");
    }
    else if (IS->is_player.p_status != ps_deity)
    {
        user(IS, "Warning - Your race is bad. Tell the deity\n");
    }
    userN3(IS, "Cash on hand: ", IS->is_player.p_money, "\n");
    userNL(IS);
    switch(IS->is_player.p_notify)
    {
        case nt_telegram:
            ptr = "telegram\n";
            break;
        case nt_message:
            ptr = "message\n";
            break;
        case nt_both:
            ptr = "both\n";
            break;
        default:
            ptr = "*unknown*\n";
            break;
    }
    /* print out the notify method */
    user2(IS, "Notify via: ", ptr);
    if (IS->is_player.p_compressed)
    {
        ptr = "ON\n";
    }
    else
    {
        ptr = "OFF\n";
    }
    /* print out the compressed mode */
    user2(IS, "Compressed mode is ", ptr);
    if (IS->is_player.p_feMode != 0)
    {
        ptr = "are";
    }
    else
    {
        ptr = "are not";
    }
    /* print out the front-end status */
    user3(IS, "You ", ptr, " using a front-end\n\n");

    /* let them know any actions they have defined */
    user(IS, "You have the following actions defined: ");
    for (i = 0; i < MAX_NUM_ACTIONS; i++)
    {
        if (IS->is_player.p_action[i].ac_action != a_none)
        {
            userC(IS, (char) 'a' + i);
        }
    }
    user(IS, "\n\n");

    /* now do realms */
    hadOne = FALSE;
    user(IS, "You have the following realms defined: ");
    for (i = 0; i < REALM_MAX; i++)
    {
        if (IS->is_player.p_realm[i][0] != '\0')
        {
            if (hadOne)
            {
                user(IS, "/");
            }
            else
            {
                hadOne = TRUE;
            }
            userN(IS, i);
        }
    }
    user(IS, "\n\n");

    hadOne = FALSE;
    /* set up the player pointer */
    p = &IS->is_request.rq_u.ru_player;
    /* loop through the current active players, and print out this players */
    /* relation to them, taking into account race relations */
    for (i = 1; i < IS->is_world.w_currPlayers; i++)
    {
        /* make sure this is not the player himself */
        if (i != IS->is_player.p_number)
        {
            /* read the player into the buffer */
            server(IS, rt_readPlayer, i);
            /* make sure this player is active */
            if (p->p_status == ps_active)
            {
                /* switch based on the players relation to the other player */
                switch(IS->is_player.p_playrel[i])
                {
                    case r_allied:
                        hadOne = TRUE;
                        ptr = " allied to ";
                        break;
                    case r_war:
                        hadOne = TRUE;
                        ptr = " at war with ";
                        break;
                    case r_neutral:
                        /* note that a neutral here overrides any race */
                        /* relation he may have specified, and so should be */
                        /* printed */
                        hadOne = TRUE;
                        ptr = " neutral to ";
                        break;
                    case r_default:
                        /* The player has not set a relation to this player, */
                        /* so we have to look for a race relation */
                        /* Make sure the player belongs to a race (Deity?) */
                        if (p->p_race != NO_RACE)
                        {
                            switch(IS->is_player.p_racerel[p->p_race])
                            {
                                case r_allied:
                                    hadOne = TRUE;
                                    ptr = ", due to their race, allied with ";
                                    break;
                                case r_neutral:
                                    /* note that a neutral here is the */
                                    /* default, and so we print nothing */
                                    ptr = "";
                                    break;
                                case r_war:
                                    hadOne = TRUE;
                                    ptr = ", due to their race, at war with ";
                                    break;
                                default:
                                    hadOne = TRUE;
                                    ptr = " *UNKNOWN RACEREL* with ";
                                    break;
                            }
                        }
                        else
                        {
                            /* by setting ptr to a NULL string, we prevent */
                            /* any messages from appearing */
                            ptr = "";
                        }
                        break;
                    default:
                        hadOne = TRUE;
                        ptr = " *UNKNOWN PLAYREL* with ";
                        break;
                }
                /* print out the complete string, if valid */
                if (*ptr != '\0')
                {
                    user2(IS, "You are", ptr);
                    user2(IS, &p->p_name[0], ".\n");
                }
                /* now we need to print this players relation to us */
                switch(p->p_playrel[IS->is_player.p_number])
                {
                    case r_allied:
                        hadOne = TRUE;
                        ptr = " allied to";
                        break;
                    case r_war:
                        hadOne = TRUE;
                        ptr = " at war with";
                        break;
                    case r_neutral:
                        hadOne = TRUE;
                        ptr = " neutral to";
                        break;
                    case r_default:
                        if (IS->is_player.p_race != NO_RACE)
                        {
                            switch(p->p_racerel[IS->is_player.p_race])
                            {
                                case r_allied:
                                    hadOne = TRUE;
                                    ptr = ", due to your race, allied with";
                                    break;
                                case r_neutral:
                                    ptr = "";
                                    break;
                                case r_war:
                                    hadOne = TRUE;
                                    ptr = ", due to your race, at war with";
                                    break;
                                default:
                                    hadOne = TRUE;
                                    ptr = " *UNKNOWN RACEREL* with";
                                    break;
                            }
                        }
                        else
                        {
                            ptr = "";
                        }
                        break;
                    default:
                        hadOne = TRUE;
                        ptr = "*UNKNOWN PLAYREL* with";
                        break;
                }
                if (*ptr != '\0')
                {
                    user(IS, "   ");
                    user3(IS, &p->p_name[0], " is", ptr);
                    user(IS, " you\n");
                }
            }
        }
    }
    /* If we had any lines printed, issue a line feed */
    if (hadOne)
    {
        userNL(IS);
    }
    /* now we want to print out any race relations this player has set */
    for (i = 0; i < RACE_MAX; i++)
    {
        if (IS->is_player.p_racerel[i] != r_neutral)
        {
            switch(IS->is_player.p_racerel[i])
            {
                case r_allied:
                    ptr = "allied";
                    break;
                case r_war:
                    ptr = "at war";
                    break;
                case r_default:
                    ptr = "*ERROR r_default*";
                    break;
                default:
                    ptr = "*UNKNOWN*";
                    break;
            }
            user2(IS, "You are ", ptr);
            user3(IS, " with the ", &IS->is_world.w_race[i].r_name[0],
                " race.\n");
        }
    }
}

/*
 * cmd_race - displays the race stats
 */

void cmd_race(IMP)
{
    register USHORT i;

    user(IS, "Stats of each race as of: ");
    uTime(IS, IS->is_request.rq_time);
    userNL(IS);
    user(IS,
" ______________________________________________________________________\n"
"| ## | Race Name                       | Tech Lev | # Planets | Status |\n"
"|    | Home Planet Name                | Res Lev  | # Players |        |\n"
"|======================================================================|\n");
    /* loop through the races, printing them out in the above format */
    for (i = 0; i < RACE_MAX; i++)
    {
        showStat(IS, i);
    }
}

void printRealm(IMP, USHORT realm)
{
    userN3(IS, "    Realm ", realm, ": ");
    if (IS->is_player.p_realm[realm][0] != '\0')
    {
        user(IS, &IS->is_player.p_realm[realm][0]);
    }
    else
    {
        user(IS, "<unused>");
    }
    userNL(IS);
}

/*
 * cmd_realm - allows the player to define/remove realms
 */

void cmd_realm(IMP)
{
    long realm;

    if (*IS->is_textInPos == '\0')
    {
        for (realm = 0; realm < REALM_MAX; realm++)
        {
            fePrintRealm(IS);
            printRealm(IS, (USHORT) realm);
        }
        return;
    }
    if (getNumber(IS, &realm))
    {
        if (realm >= REALM_MAX)
        {
            err(IS, "invalid realm number");
        }
        else
        {
            (void) skipBlanks(IS);
            if (*IS->is_textInPos != '\0')
            {
                server(IS, rt_lockPlayer, IS->is_player.p_number);
                if (strcmp(IS->is_textInPos, "none") == 0)
                {
                    memset(&IS->is_request.rq_u.ru_player.p_realm[realm][0],
                        '\0', REALM_LEN * sizeof(char));
                }
                else
                {
                    strncpy(&IS->is_request.rq_u.ru_player.p_realm[realm][0],
                        IS->is_textInPos, (REALM_LEN - 1) * sizeof(char));
                }
                server(IS, rt_unlockPlayer, IS->is_player.p_number);
                strcpy(&IS->is_player.p_realm[realm][0],
                    &IS->is_request.rq_u.ru_player.p_realm[realm][0]);
            }
            else
            {
                fePrintRealm(IS);
                printRealm(IS, (USHORT) realm);
            }
        }
    }
}

