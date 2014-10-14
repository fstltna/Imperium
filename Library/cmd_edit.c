/*
 * Imperium
 *
 * $Id: cmd_edit.c,v 1.3 2000/05/25 18:21:23 marisa Exp $
 *
 * deity related code
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
 * $Log: cmd_edit.c,v $
 * Revision 1.3  2000/05/25 18:21:23  marisa
 * Fix more T/F issues
 *
 * Revision 1.2  2000/05/18 06:50:01  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:47  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:59:04  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:24:10  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:02  marisag
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

#define CmdEditC 1

#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"

static char const rcsid[] = "$Id: cmd_edit.c,v 1.3 2000/05/25 18:21:23 marisa Exp $";

static const char fleetChar[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
/*
 * examinePlayer - part of cmd_examine
 *          Note: clears the buffer
 */

void examinePlayer(IMP, USHORT who)
{
    register Player_t *p;
    register USHORT i, col;
    char *ptr;

    /* read in the player */
    server(IS, rt_readPlayer, who);
    /* set the pointer */
    p = &IS->is_request.rq_u.ru_player;
    user3(IS, "Player: '", &p->p_name[0], "'; password: '");
    /* display the status of the player */
    switch(p->p_status)
    {
        case ps_deity:
            ptr = "DEITY\n";
            break;
        case ps_active:
            ptr = "Active\n";
            break;
        case ps_quit:
            ptr = "Resigned\n";
            break;
        case ps_idle:
            ptr = "Idle\n";
            break;
        case ps_visitor:
            ptr = "Visitor\n";
            break;
        default:
            ptr = "<UNKNOWN>\n";
            break;
    }
    user3(IS, &p->p_password[0], "'; status: ", ptr);
    /* Print the players race number and race name, if any */
    if (p->p_race != NO_RACE)
    {
        userN3(IS, "Race: ", p->p_race, "(");
        user2(IS, &IS->is_world.w_race[p->p_race].r_name[0], ")\n");
    }
    user(IS,"Last on at: ");
    uTime(IS, p->p_lastOn);
    userN3(IS, "; Planets: ", p->p_planetCount, "\n");
    userN2(IS, "BTU's left: ", p->p_btu);
    userN2(IS, "; minutes left: ", p->p_timeLeft);
    userN3(IS, "; credits: ", p->p_money, "\n");
    userN2(IS, "telegramsNew = ", p->p_telegramsNew);
    userN3(IS, ",   telegramsTail = ", p->p_telegramsTail, "\n");
    userN3(IS, "Warn at ", p->p_btuWarn, " BTUs left\n");
    userN3(IS, "Warn at ", p->p_timeWarn, " minutes left\n");
    user(IS, "Player-Player Relations: \n");
    /* loop through the players p-p relations */
    for (i = 0; i < IS->is_world.w_currPlayers; i++)
    {
        switch(p->p_playrel[i])
        {
            case r_neutral:
                userC(IS, 'n');
                break;
            case r_allied:
                userC(IS, 'a');
                break;
            case r_war:
                userC(IS, 'w');
                break;
            case r_default:
                /* A default here means use the players race relations */
                userC(IS, 'd');
                break;
            default:
                /* this should never occur normally */
                userC(IS, '?');
                break;
        }
    }
    userNL(IS);
    /* loop through the players race relations */
    user(IS, "Player-Race Relations: \n");
    for (i = 0; i < RACE_MAX; i++)
    {
        switch(p->p_racerel[i])
        {
            case r_neutral:
                userC(IS, 'n');
                break;
            case r_allied:
                userC(IS, 'a');
                break;
            case r_war:
                userC(IS, 'w');
                break;
            case r_default:
                /* A default should never occur here, but we want to print */
                /* it so that we can use it to diagnose problems */
                userC(IS, 'd');
                break;
            default:
                /* this should never occur normally */
                userC(IS, '?');
                break;
        }
    }
    userNL(IS);
    /* loop through the players fleets */
    user(IS, "Fleets: ");
    col = 11;
    for (i = 0; i < (26 + 26); i++)
    {
        if (p->p_fleets[i] != NO_FLEET)
        {
            if (col > 73)
            {
                userNL(IS);
                user(IS, "          ");
                col = 11;
            }
            userC(IS, fleetChar[i]);
            userN3(IS, ":", p->p_fleets[i], " ");
        }
    }
    userNL(IS);
    /* print the online status */
    if (p->p_loggedOn)
    {
        ptr = "Currently marked as logged on\n";
    }
    else
    {
        ptr = "Currently not marked as logged on\n";
    }
    user(IS, ptr);
    /* print the notify method */
    switch(p->p_notify)
    {
        case nt_telegram:
            ptr =  "telegram\n";
            break;
        case nt_message:
            ptr = "message\n";
            break;
        case nt_both:
            ptr = "both\n";
            break;
    }
    user2(IS, "Notify via: ", ptr);
    if (IS->is_player.p_sendEmail)
    {
        ptr = "ON\n";
    }
    else
    {
        ptr = "OFF\n";
    }
    /* print out the email sending option */
    user2(IS, "Sending email is ", ptr);

    /* print the chat status */
    if (p->p_inChat)
    {
        ptr = "Marked as being in chat mode\n";
    }
    else
    {
        ptr = "Marked as out of chat mode\n";
    }
    user(IS, ptr);
    /* print the compressed status */
    if (p->p_compressed)
    {
        ptr = "Has compressed map mode turned on\n";
    }
    else
    {
        ptr = "Does not have compressed map mode turned on\n";
    }
    user(IS, ptr);
    /* print whether the player is building a power report */
    if (p->p_doingPower)
    {
        ptr = "Marked as updating power report\n";
    }
    else
    {
        ptr = "Not marked as updating power report\n";
    }
    user(IS, ptr);
    /* Print whether the user is using a FE, should probobly be false */
    /* unless the user is online */
    if (p->p_feMode != 0)
    {
        userN3(IS, "Marked as using front-end mode ", p->p_feMode, "\n");
    }
    else
    {
        user(IS, "Not marked as using a front-end\n");
    }
}

/*
 * examinePlanet - part of cmd_examine. Prints out info on a given planet
 */

void examinePlanet(IMP, ULONG pNum)
{
    Planet_t p;
    ItemType_t it;
    unsigned int curWeap;

    /* read the planet in */
    server(IS, rt_readPlanet, pNum);
    /* copy the planet out so we can reuse the buffer */
    p = IS->is_request.rq_u.ru_planet;

    userN2(IS, "Planet ", pNum);
    if (p.pl_name[0] != '\0')
    {
        user3(IS, " (", &p.pl_name[0], ")");
    }
    user(IS, " - ");
    userC(IS, PLANET_CHAR[p.pl_class]);
    userS(IS, " at ", p.pl_row, p.pl_col, "\nlastUpdate = ");
    uTime(IS, p.pl_lastUpdate);
    userN3(IS, ", owner = ", p.pl_owner, " (");
    /* If the planet is owned then read in the owner and get the name */
    if (p.pl_owner != NO_OWNER)
    {
        server(IS, rt_readPlayer, p.pl_owner);
        user2(IS, &IS->is_request.rq_u.ru_player.p_name[0], ")\n");
    }
    else
    {
        /* otherwise just print a default string */
        user(IS, "*UNOWNED*)\n");
    }
    user(IS, "Planet ");
    switch(p.pl_transfer)
    {
        case pt_trade:
            user(IS, "was traded peacefully\n");
            break;
        case pt_peacefull:
            user(IS, "was colonized peacefully\n");
            break;
        case pt_home:
            userN3(IS, "is a home planet of race ", p.pl_ownRace, "\n");
            break;
        case pt_hostile:
            userN3(IS, "was taken by force from player ", p.pl_lastOwner,
                "\n");
            break;
        default:
            user(IS, "* ERROR TRANSFER STATUS *\n");
            break;
    }
    userN2(IS, "size = ", p.pl_size);
    userN2(IS, ", minerals = ", p.pl_minerals);
    userN2(IS, ", gold = ", p.pl_gold);
    userN2(IS, ", techLevel = ", p.pl_techLevel);
    userN2(IS, ", resLevel = ", p.pl_resLevel);
    userN2(IS, ", gas = ", p.pl_gas);
    userN2(IS, ", water = ", p.pl_water);
    user3(IS, "\ncheckPoint = ", &p.pl_checkpoint[0], "\n");
    userN2(IS, "shipCount = ", p.pl_shipCount);
    userN2(IS, ", bigItems = ", p.pl_bigItems);
    userN2(IS, ", BTUs = ", p.pl_btu);
    userN2(IS, ", mobility = ", p.pl_mobility);
    userN3(IS, ", efficiency = ", p.pl_efficiency, "\n");
    userN2(IS, "polution = ", p.pl_polution);
    userN2(IS, ", plagueStage = ", p.pl_plagueStage);
    userN3(IS, ", plagueTime = ", p.pl_plagueTime, "\n");
    /* loop through the planets item list */
    user(IS, "Planet has the following items:\n");
    for (it = IT_FIRST; it <= IT_LAST; it++)
    {
        user(IS, "   ");
        userC(IS, IS->is_itemChar[it]);
        userN3(IS, ": ", p.pl_quantity[it], "\n");
    }
    /* loop through the planets production points and percents */
    user(IS, "Planet has the following production points:\n");
    for (it = PPROD_FIRST; it <= PPROD_LAST; it++)
    {
        user2(IS, "   ", PPROD_NAME[it]);
        userN2(IS, ": ", p.pl_prod[it]);
        userN3(IS, " (", p.pl_workPer[it], "%)\n");
    }
    /* loop through the planets weapons */
    user(IS, "Planet has the following weapons sites:\n");
    for (curWeap = 0; curWeap < MAX_WEAP_PL; curWeap++)
    {
	if (p.pl_weapon[curWeap] != NO_ITEM)
	{
            userN2(IS, "    ", p.pl_weapon[curWeap]);
            userNL(IS);
	}
    }
}

/*
 * examineShip - part of cmd_examine
 */

void examineShip(IMP, ULONG shipNumber)
{
    Ship_t ship;
    ItemType_t it;
    BigPart_t bp;
    USHORT maxNum;
    char *ptr = "";
    ULONG *numPtr = NULL;

    server(IS, rt_readShip, shipNumber);
    ship = IS->is_request.rq_u.ru_ship;
    userN2(IS, "Ship #", shipNumber);
    if (ship.sh_name[0] != '\0')
    {
        user3(IS, " (", &ship.sh_name[0], ")");
    }
    user3(IS, " ", getShipName(ship.sh_type), "\n");
    user(IS, "lastUpdate ");
    uTime(IS, ship.sh_lastUpdate);
    userN3(IS, ", owner ", ship.sh_owner, "(");
    if (ship.sh_owner != NO_OWNER)
    {
        server(IS, rt_readPlayer, ship.sh_owner);
        user(IS, &IS->is_request.rq_u.ru_player.p_name[0]);
    }
    else
    {
        user(IS, "*UNOWNED*");
    }
    user(IS, ")\n");
    if (ship.sh_price > 0)
    {
        userN3(IS, "price = ", ship.sh_price, " per ton");
    }
    else
    {
        user(IS, "NOT FOR SALE");
    }
    user(IS, ", fleet = ");
    userC(IS, ship.sh_fleet);
    userS(IS, ", at ", ship.sh_row, ship.sh_col, ", effic = ");
    userN(IS, ship.sh_efficiency);
    userN2(IS, ", energy = ", ship.sh_energy);
    userN3(IS, ", fuel left = ", ship.sh_fuelLeft, "\n");
    userN2(IS, "number = ", ship.sh_number);
    userN2(IS, ", planet = ", ship.sh_planet);
    userN2(IS, ", cargo = ", ship.sh_cargo);
    userN3(IS, ", armour = ", ship.sh_armourLeft, "\n");
    userN2(IS, "hull TF = ", ship.sh_hullTF);
    userN3(IS, ", engine TF = ", ship.sh_engTF, "\n");
    userN2(IS, "plague stage = ", ship.sh_plagueStage);
    userN3(IS, ", plague time = ", ship.sh_plagueTime, "\nShip carries:\n");
    for (it = IT_FIRST; it <= IT_LAST; it++)
    {
        user(IS, "   ");
        userC(IS, IS->is_itemChar[it]);
        userN3(IS, ": ", ship.sh_items[it], "\n");
    }
    user(IS, "Big items:\n");
    for (bp = BP_FIRST; bp <= BP_LAST; bp++)
    {
        switch(bp)
        {
            case bp_computer:
                maxNum = MAX_COMP;
                ptr = "computers: ";
                numPtr = &ship.sh_computer[0];
                break;
            case bp_engines:
                maxNum = MAX_ENG;
                ptr = "engines: ";
                numPtr = &ship.sh_engine[0];
                break;
            case bp_lifeSupp:
                maxNum = MAX_LIFESUP;
                ptr = "life supp: ";
                numPtr = &ship.sh_lifeSupp[0];
                break;
            case bp_sensors:
                /* note that we only specify one of the modules */
                maxNum = MAX_ELECT;
                ptr = "electronics: ";
                numPtr = &ship.sh_elect[0];
                break;
            case bp_photon:
                /* note that we only specify one of the weapons */
                maxNum = MAX_WEAP;
                ptr = "weapons: ";
                numPtr = &ship.sh_weapon[0];
                break;
            default:
                maxNum = 0;
                break;
        }
        if (maxNum)
        {
            user(IS, ptr);
            for (it = 0; it < maxNum; it++)
            {
                if (it)
                {
                    user(IS, ", ");
                }
                userN(IS, numPtr[it]);
            }
            userNL(IS);
        }
    }
}

/*
 * examineFleet - part of cmd_examine.
 */

void examineFleet(IMP, USHORT fleetNumber)
{
    register Fleet_t *fleet;
    register USHORT i;

    server(IS, rt_readFleet, fleetNumber);
    fleet = &IS->is_request.rq_u.ru_fleet;
    if (fleet->f_count == 0)
    {
        userN3(IS, "No ships in fleet #", fleetNumber, "\n");
    }
    else
    {
        userN3(IS, "Ships in fleet #", fleetNumber, ": ");
        for (i = 0; i < fleet->f_count; i++)
        {
            userN(IS, fleet->f_ship[i]);
            if (i < fleet->f_count)
            {
                userC(IS, '/');
            }
        }
        userNL(IS);
    }
}

/*
 * examineLoan - part of cmd_examine.
 */

void examineLoan(IMP, USHORT loanNumber)
{
    Loan_t loan;
    char *ptr = "";

    server(IS, rt_readLoan, loanNumber);
    loan = IS->is_request.rq_u.ru_loan;
    userN3(IS, "Loan #", loanNumber, " offered by ");
    server(IS, rt_readPlayer, loan.l_loaner);
    user2(IS, &IS->is_request.rq_u.ru_player.p_name[0], " to ");
    server(IS, rt_readPlayer, loan.l_loanee);
    user2(IS, &IS->is_request.rq_u.ru_player.p_name[0], "\n");
    user(IS, "lastPay = ");
    uTime(IS, loan.l_lastPay);
    user(IS, ", dueDate = ");
    uTime(IS, loan.l_dueDate);
    userNL(IS);
    userN2(IS, "amount = ", loan.l_amount);
    userN2(IS, ", paid = ", loan.l_paid);
    userN2(IS, ", duration = ", loan.l_duration);
    userN2(IS, ", rate = ", loan.l_rate);
    switch(loan.l_state)
    {
        case l_offered:
            ptr = "offered\n";
            break;
        case l_declined:
            ptr = "declined\n";
            break;
        case l_outstanding:
            ptr = "outstanding\n";
            break;
        case l_paidUp:
            ptr = "paid up\n";
            break;
    }
    user2(IS, ", state = ", ptr);
}

/*
 * examineWorld - part of cmd_examine.
 */

void examineWorld(IMP)
{
    char *ptr;

    user(IS, "World created on ");
    uTime(IS, IS->is_world.w_buildDate);
    userNL(IS);
    userN2(IS, "World is ", IS->is_world.w_rows);
    userN2(IS, " galactic rows by ", IS->is_world.w_columns);
    userN2(IS, " galactic columns with ", IS->is_world.w_currPlayers);
    userN2(IS, " out of ", IS->is_world.w_maxPlayers);
    user(IS, " players.\n");
    userNL(IS);
    userN2(IS, "Maximum daily connect time is ", IS->is_world.w_maxConnect);
    userN2(IS, " minutes, and maximum BTU's allowed are ",
       IS->is_world.w_maxBTUs);
    user3(IS, "\nPlayer creation password: ", &IS->is_world.w_password[0], "\n");
    if (!IS->is_world.w_sendAll)
    {
        ptr = "NOT send";
    }
    else
    {
        ptr = "send";
    }
    user3(IS, "Users may ", ptr, " public messages\n");
    if (!IS->is_world.w_chaPlay)
    {
        ptr = "NOT change";
    }
    else
    {
        ptr = "change";
    }
    user3(IS, "Users may ", ptr, " to another player\n");
    userN2(IS, "Next loan: ", IS->is_world.w_loanNext);
    userN2(IS, "\nNext offer: ", IS->is_world.w_offerNext);
    userN2(IS, "\nNext ship: ", IS->is_world.w_shipNext);
    userN2(IS, "\nNext fleet: ", IS->is_world.w_fleetNext);
    userN2(IS, "\nNext planet: ", IS->is_world.w_planetNext);
    userN2(IS, "\nNext big item: ", IS->is_world.w_bigItemNext);
    userNL(IS);
}

/*
 * examineRace - part of cmd_examine
 */

void examineRace(IMP, USHORT race)
{
    userN3(IS, "Race ", race, " (");
    user2(IS, &IS->is_world.w_race[race].r_name[0], ")\n");
    userN2(IS, "Home planet ", IS->is_world.w_race[race].r_homePlanet);
    user2(IS, " (", &IS->is_world.w_race[race].r_homeName[0]);
    userN2(IS, ") is at ", IS->is_world.w_race[race].r_homeRow);
    userN3(IS, ",", IS->is_world.w_race[race].r_homeCol, "\n");
    userN3(IS, "The race has ", IS->is_world.w_race[race].r_planetCount,
        " planets under it's control, and is ");
    switch(IS->is_world.w_race[race].r_status)
    {
        case rs_notyet:
            user(IS, "not yet active\n");
            break;
        case rs_active:
            user(IS, "currently active\n");
            break;
        case rs_dead:
            user(IS, "exterminated\n");
            break;
        default:
            user(IS, "in an unknown state\n");
            break;
    }
    userN2(IS, "The race has a TL of ", IS->is_world.w_race[race].r_techLevel);
    userN2(IS, ", a RL of ", IS->is_world.w_race[race].r_resLevel);
    userN3(IS, ", and is made up of ", IS->is_world.w_race[race].r_playCount,
        " players\n");
}

/*
 * cmd_examine - allows the deity to examine various items in the Imperium
 *          world.
 */

void cmd_examine(IMP)
{
    USHORT what, sn;
    ULONG n;

    if (IS->is_player.p_status != ps_deity)
    {
        err(IS, "only a deity can examine things");
        return;
    }
    if (reqChoice(IS, &what, "player\0planet\0ship\0fleet\0loan\0race\0world\0",
        "Examine what (player/planet/ship/fleet/loan/race/world)? "))
    {
        (void) skipBlanks(IS);
        switch(what)
        {
            case 0:
                if (reqPlayer(IS, &sn, "Examine which player? "))
                {
                    examinePlayer(IS, sn);
                }
                break;
            case 1:
                if (reqPlanet(IS, &n, "Examine which planet? "))
                {
                    examinePlanet(IS, n);
                }
                break;
            case 2:
                if (reqShip(IS, &n, "Examine which ship? "))
                {
                    examineShip(IS, n);
                }
                break;
            case 3:
                if (IS->is_world.w_fleetNext == 0)
                {
                    err(IS, "there are no fleets yet");
                }
                else
                {
                    if (reqPosRange(IS, &n, IS->is_world.w_fleetNext - 1,
                        "Examine which fleet? "))
                    {
                        examineFleet(IS, (USHORT) n);
                    }
                }
                break;
            case 4:
                if (IS->is_world.w_loanNext == 0)
                {
                    err(IS, "there are no loans yet");
                }
                else
                {
                    if (reqPosRange(IS, &n, IS->is_world.w_loanNext - 1,
                        "Examine which loan? "))
                    {
                        examineLoan(IS, (USHORT) n);
                    }
                }
                break;
            case 5:
                if (reqRace(IS, &sn, TRUE, "Examine which race? "))
                {
                    examineRace(IS, sn);
                }
                break;
            case 6:
                examineWorld(IS);
                break;
        }
    }
}

/*
 * getLNumber - read in a long signed number.
 */

BOOL getLNumber(IMP, long *pNum)
{
    register char *inputPtr;
    register long n;
    BOOL isNeg;

    inputPtr = IS->is_textInPos;
    if (*inputPtr == '-')
    {
        isNeg = TRUE;
        inputPtr += sizeof(char);
    }
    else
    {
        if (*inputPtr == '+')
        {
            isNeg = FALSE;
            inputPtr += sizeof(char);
        }
        else
        {
            isNeg = FALSE;
        }
    }
    if ((*inputPtr >= '0') && (*inputPtr <= '9'))
    {
        n = 0;
        while ((*inputPtr >= '0') && (*inputPtr <= '9'))
        {
            n = n * 10 + (*inputPtr - '0');
            inputPtr += sizeof(char);
        }
        IS->is_textInPos = inputPtr;
        if (isNeg)
        {
            *pNum = -n;
        }
        else
        {
            *pNum = n;
        }
        return TRUE;
    }
    if (*inputPtr == '\0')
    {
        err(IS, "missing number");
    }
    else
    {
        err(IS, "invalid number");
        *inputPtr = '\0';
    }
    IS->is_textInPos = inputPtr;
    return FALSE;
}

/*
 * getULNumber - read in a long unsigned number.
 */

BOOL getULNumber(IMP, ULONG *pNum)
{
    register char *inputPtr;
    register ULONG n;

    inputPtr = IS->is_textInPos;
    if (*inputPtr == '-')
    {
        err(IS, "negative numbers are not allowed");
        return FALSE;
    }
    if (*inputPtr == '+')
    {
        inputPtr += sizeof(char);
    }
    if ((*inputPtr >= '0') && (*inputPtr <= '9'))
    {
        n = 0;
        while ((*inputPtr >= '0') && (*inputPtr <= '9'))
        {
            n = n * 10 + (*inputPtr - '0');
            inputPtr += sizeof(char);
        }
        IS->is_textInPos = inputPtr;
        *pNum = n;
        return TRUE;
    }
    if (*inputPtr == '\0')
    {
        err(IS, "missing number");
    }
    else
    {
        err(IS, "invalid number");
        *inputPtr = '\0';
    }
    IS->is_textInPos = inputPtr;
    return FALSE;
}

/*
 * repNum - request a replacement for a long signed numeric value.
 */

long repNum(IMP, long oldValue, long minValue, long maxValue, char *prompt)
{
    long n;
    char promptBuffer[100];

    user(IS, prompt);
    userN2(IS, " (", minValue);
    userN3(IS, " - ", maxValue, ")");
    getPrompt(IS, &promptBuffer[0]);
    if ((*IS->is_textInPos == '\0') || (IS->is_BOOL1))
    {
        user(IS, &promptBuffer[0]);
        user(IS, ": ");
        userN(IS, oldValue);
        userNL(IS);
    }
    if (!IS->is_BOOL1)
    {
        while(1)
        {
            if (*IS->is_textInPos == '\0')
            {
                uPrompt(IS, &promptBuffer[0]);
                if (!clReadUser(IS))
                {
                    n = oldValue;
                }
                else if (*IS->is_textInPos == '\0')
                {
                    n = oldValue;
                }
                else
                {
                    (void) skipBlanks(IS);
                    if (!getLNumber(IS, &n))
                    {
                        n = oldValue;
                    }
                }
                if (n < minValue)
                {
                    err(IS, "value too small");
                }
                else
                {
                    if (n > maxValue)
                    {
                        err(IS, "value too large");
                    }
                    else
                    {
                        return n;
                    }
                }
            }
            else
            {
                if (!getLNumber(IS, &n))
                {
                    n = oldValue;
                }
                if (n < minValue)
                {
                    err(IS, "value too small");
                }
                else
                {
                    if (n > maxValue)
                    {
                        err(IS, "value too large");
                    }
                    else
                    {
                        return n;
                    }
                }
            }
        }
    }
    return oldValue;
}

/*
 * repUNum - request a replacement for a long unsigned numeric value.
 */

ULONG repUNum(IMP, ULONG oldValue, ULONG minValue, ULONG maxValue,
    char *prompt)
{
    ULONG n;
    char promptBuffer[100];

    user(IS, prompt);
    userN2(IS, " (", minValue);
    userN3(IS, " - ", maxValue, ")");
    getPrompt(IS, &promptBuffer[0]);
    user(IS, &promptBuffer[0]);
    user(IS, ": ");
    userN(IS, oldValue);
    userNL(IS);
    if (!IS->is_BOOL1)
    {
        while(1)
        {
            uPrompt(IS, &promptBuffer[0]);
            if (!clReadUser(IS))
            {
                n = oldValue;
            }
            else if (*IS->is_textInPos == '\0')
            {
                n = oldValue;
            }
            else
            {
                (void) skipBlanks(IS);
                if (!getULNumber(IS, &n))
                {
                    n = oldValue;
                }
            }
            if (n < minValue)
            {
                err(IS, "value too small");
            }
            else
            {
                if (n > maxValue)
                {
                    err(IS, "value too large");
                }
                else
                {
                    return n;
                }
            }
        }
    }
    return oldValue;
}

/*
 * repScale - replace a scale factor that can be 10 - 1000.
 */

void repScale(IMP, USHORT *pNum, char *prompt)
{
    *pNum = repUNum(IS, *pNum, 1, 1000, prompt);
}

/*
 * repMob - replace a mobility - value 0 - 100.
 */

void repMob(IMP, USHORT *pCost, char *name)
{
    *pCost = repUNum(IS, *pCost, 0, 100, name);
}

/*
 * repCost - replace a cost - value is 10 - 128.
 */

void repCost(IMP, USHORT *pCost, char *prompt)
{
    *pCost = repUNum(IS, *pCost, 10, 128, prompt);
}

/*
 * repRand - replace a base/rand pair.
 */

void repRand(IMP, USHORT *pBase, USHORT *pRand, char *what)
{
    char buffer[50];

    user2(IS, what, " base");
    getPrompt(IS, &buffer[0]);
    *pBase = repUNum(IS, *pBase, 0, 127, &buffer[0]);
    user2(IS, what, " rand");
    getPrompt(IS, &buffer[0]);
    *pRand = repUNum(IS, *pRand, 1, 128, &buffer[0]);
}

/*
 * repBool - replace a BOOLean flag value.
 */

void repBool(IMP, BOOL *pFlag, char *what)
{
    register char *p, *q;
    char promptBuffer[100];

    user(IS, what);
    getPrompt(IS, &promptBuffer[0]);
    user(IS, &promptBuffer[0]);
    user(IS, ": ");
    if (*pFlag)
    {
        user(IS, "TRUE");
    }
    else
    {
        user(IS, "FALSE");
    }
    userNL(IS);
    if (!IS->is_BOOL1)
    {
        while(1)
        {
            uPrompt(IS, &promptBuffer[0]);
            if (!clReadUser(IS))
            {
                return;
            }
            if (*IS->is_textInPos == '\0')
            {
                return;
            }
            else
            {
                p = skipBlanks(IS);
                *skipWord(IS) = '\0';
                q = p;
                while (*q != '\0')
                {
                    if ((*q >= 'A') && (*q <= 'Z'))
                    {
                        *q = *q - 'A' + 'a';
                    }
                    q += sizeof(char);
                }
                if ((strcmp(p, "true") == 0) || (strcmp(p, "yes") == 0) ||
                    (strcmp(p, "ok") == 0) || (strcmp(p, "y") == 0))
                {
                    *pFlag = TRUE;
                    return;
                }
                else
                {
                    if ((strcmp(p, "false") == 0) || (strcmp(p, "no") == 0) ||
                        (strcmp(p, "n") == 0))
                    {
                        *pFlag = FALSE;
                        return;
                    }
                    else
                    {
                        err(IS, "unknown boolean value");
                    }
                }
            }
        }
    }
}

/*
 * editPlayer - part of cmd_edit
 */

void editPlayer(IMP, USHORT who)
{
    char buf[30];
    Player_t p;
    register USHORT i;

    server(IS, rt_readPlayer, who);
    p = IS->is_request.rq_u.ru_player;
    user3(IS, "Name: ", &p.p_name[0], "\n");
    uPrompt(IS, "Name");
    if (clReadUser(IS))
    {
        if (*IS->is_textInPos != '\0')
        {
            IS->is_textIn[NAME_LEN - 1] = '\0';
            strcpy(&p.p_name[0], &IS->is_textIn[0]);
        }
    }
    user3(IS, "Password: ", &p.p_password[0], "\n");
    uPrompt(IS, "Password");
    if (clReadUser(IS))
    {
        if (*IS->is_textInPos != '\0')
        {
            IS->is_textIn[PASSWORD_LEN - 1] = '\0';
            strcpy(&p.p_password[0], &IS->is_textIn[0]);
        }
    }
    user3(IS, "Email Address: ", &p.p_email[0], "\n");
    uPrompt(IS, "Email Address");
    if (clReadUser(IS))
    {
        if (*IS->is_textInPos != '\0')
        {
            IS->is_textIn[EMAIL_LEN - 1] = '\0';
            strcpy(&p.p_email[0], &IS->is_textIn[0]);
        }
    }
    if (p.p_sendEmail)
    {
        if (ask(IS, "Sendimg email. Disable this? "))
        {
            p.p_sendEmail = FALSE;
        }
    }
    else
    {
        if (ask(IS, "Not sending email. Enable this? "))
        {
            p.p_sendEmail = TRUE;
        }
    }
    p.p_status = repUNum(IS, p.p_status, ps_deity, ps_idle, "Status");
    p.p_planetCount = repUNum(IS, p.p_planetCount, 0, 0xffffffff,
        "Planet count");
    p.p_money = repNum(IS, p.p_money, -0x80000000, 0x7fffffff, "Credits");
    p.p_btu = repUNum(IS, p.p_btu, 0, IS->is_world.w_maxBTUs, "BTU's");
    p.p_btuWarn = repUNum(IS, p.p_btuWarn, 0, IS->is_world.w_maxBTUs,
        "BTU Warning Level");
    p.p_timeLeft = repUNum(IS, p.p_timeLeft, 0, IS->is_world.w_maxConnect,
        "Remaining play time");
    p.p_timeWarn = repUNum(IS, p.p_timeWarn, 0, IS->is_world.w_maxConnect,
        "Play time warning level");
    userN2(IS, "telegramsNew = ", p.p_telegramsNew);
    userN3(IS, ",   telegramsTail = ", p.p_telegramsTail, "\n");
    if (ask(IS, "Reset telegram pointers? "))
    {
        p.p_telegramsNew = 0;
        p.p_telegramsTail = 0;
    }
    p.p_race = repUNum(IS, p.p_race, 0, RACE_MAX, "Race");
    p.p_number = repUNum(IS, p.p_number, 0, PLAYER_MAX - 1, "Player number");
    if (p.p_loggedOn)
    {
        if (ask(IS, "Marked as logged on. Mark as not logged on? "))
        {
            p.p_loggedOn = FALSE;
        }
    }
    else
    {
        if (ask(IS, "Marked as not logged on. Mark as logged on? "))
        {
            p.p_loggedOn = TRUE;
        }
    }
    if (p.p_inChat)
    {
        if (ask(IS, "Marked as in chat mode. Clear status? "))
        {
            p.p_inChat = FALSE;
        }
    }
    if (p.p_doingPower)
    {
        if (ask(IS, "Marked as updating power report. Clear status? "))
        {
            p.p_doingPower = FALSE;
        }
    }
    if (ask(IS, "Edit fleets? "))
    {
        for (i = 0; i < 26 + 26; i++)
        {
            user(IS, "Fleet ");
            userC(IS, fleetChar[i]);
            getPrompt(IS, &buf[0]);
            p.p_fleets[i] = repUNum(IS, p.p_fleets[i], 0, 0xffff, &buf[0]);
        }
    }
    if (ask(IS, "Set new player status? "))
    {
        p.p_newPlayer = TRUE;
    }
    server(IS, rt_lockPlayer, who);
    IS->is_request.rq_u.ru_player = p;
    server(IS, rt_unlockPlayer, who);
}

/*
 * editPlanet - part of cmd_edit
 */

void editPlanet(IMP, ULONG pNum)
{
    Planet_t p;
    ItemType_t it;
    USHORT player;
    PProd_t prod;
    char buff[80];

    server(IS, rt_readPlanet, pNum);
    p = IS->is_request.rq_u.ru_planet;
    userN3(IS, "Planet ", pNum, " - ");
    userC(IS, PLANET_CHAR[p.pl_class]);
    user(IS, ", lastUpdate = ");
    uTime(IS, p.pl_lastUpdate);
    userNL(IS);

    /* edit the planets name */
    user3(IS, "Name: ", &p.pl_name[0], "\n");
    uPrompt(IS, "Name");
    if (clReadUser(IS))
    {
        if (*IS->is_textInPos != '\0')
        {
            /* see if we want to remove any name */
            if (strcmp(&IS->is_textIn[0], "none") == 0)
            {
                p.pl_name[0] = '\0';
            }
            else
            {
                IS->is_textIn[PLAN_NAME_LEN - 1] = '\0';
                strcpy(&p.pl_name[0], &IS->is_textIn[0]);
            }
        }
    }

    /* edit the planets number */
    p.pl_number = repUNum(IS, p.pl_number, 0, IS->is_world.w_planetNext - 1,
        "Planet number");

    /* edit the planets checkpoint code */
    user3(IS, "Checkpoint code: ", &p.pl_checkpoint[0], "\n");
    uPrompt(IS, "code");
    if (clReadUser(IS))
    {
        if (*IS->is_textInPos != '\0')
        {
            /* see if we want to remove any name */
            if (strcmp(&IS->is_textIn[0], "none") == 0)
            {
                p.pl_checkpoint[0] = '\0';
            }
            else
            {
                IS->is_textIn[PLAN_PSWD_LEN - 1] = '\0';
                strcpy(&p.pl_checkpoint[0], &IS->is_textIn[0]);
            }
        }
    }

    /* Edit the planets owner */
    if (p.pl_owner != NO_OWNER)
    {
        /* If the planet is owned, read in the owner's player struct */
        server(IS, rt_readPlayer, p.pl_owner);
        user3(IS, "Owner: ", &IS->is_request.rq_u.ru_player.p_name[0], "\n");
    }
    else
    {
        /* otherwise print a default message */
        user(IS, "The planet is currently unowned. Enter new owner.\n");
    }
    if (reqPlayer(IS, &player, "Owner"))
    {
        p.pl_owner = player;
    }
    p.pl_row = repUNum(IS, p.pl_row, 0, ((IS->is_world.w_rows - 1) * 10) + 9,
        "planets row");
    p.pl_col = repUNum(IS, p.pl_col, 0, ((IS->is_world.w_columns - 1) * 10) +
        9, "planets col");
    p.pl_btu = repUNum(IS, p.pl_btu, 0, IS->is_world.w_maxBTUs, "BTUs");
    p.pl_minerals = repUNum(IS, p.pl_minerals, 0, 255, "Mineral sample");
    p.pl_gold = repUNum(IS, p.pl_gold, 0, 255,"Gold sample");
    p.pl_size = repUNum(IS, p.pl_size, 0, 10, "Size");

    p.pl_shipCount = repUNum(IS, p.pl_shipCount, 0, 0xffff, "Ship count");
    p.pl_mobility = repUNum(IS, p.pl_mobility, 0, 0xffff, "Mobility ");
    p.pl_efficiency = repUNum(IS, p.pl_efficiency, 0, 100, "Efficiency");
    p.pl_polution = repUNum(IS, p.pl_polution, 0, 100, "Polution");
    p.pl_gas = repUNum(IS, p.pl_gas, 0, 100, "Gas");
    p.pl_water = repUNum(IS, p.pl_water, 0, 100, "Water");
    p.pl_plagueStage = repUNum(IS, p.pl_plagueStage, 0, 4, "Plague stage");
    p.pl_plagueTime = repUNum(IS, p.pl_plagueTime, 0, 127, "Plague time");
    p.pl_bigItems = repUNum(IS, p.pl_bigItems, 0, 255, "Big items");
    p.pl_techLevel = repUNum(IS, p.pl_techLevel, 0, 0xffff, "Tech level");
    p.pl_resLevel = repUNum(IS, p.pl_resLevel, 0, 0xffff, "Res level");
    p.pl_ownRace = repUNum(IS, p.pl_ownRace, 0, RACE_MAX, "Owning race");
    p.pl_transfer = repUNum(IS, p.pl_transfer, pt_trade, pt_hostile,
        "Planet transfer status");

    for (it = IT_FIRST; it <= IT_LAST; it++)
    {
        writePlQuan(IS, &p, it, (USHORT) repUNum(IS, (ULONG) readPlQuan(IS, &p, it), 0,
            MAX_WORK, getItemName(it)));
    }
    for (prod = PPROD_FIRST; prod <= PPROD_LAST; prod++)
    {
        sprintf(&buff[0], "%s production units", PPROD_NAME[prod]);
        p.pl_prod[prod] = repUNum(IS, p.pl_prod[prod], 0, MAX_WORK, &buff[0]);
        sprintf(&buff[0], "%s work percentage", PPROD_NAME[prod]);
        p.pl_workPer[prod] = repUNum(IS, p.pl_workPer[prod], 0, 100, &buff[0]);
    }
    if (!IS->is_BOOL1)
    {
        server(IS, rt_lockPlanet, pNum);
        IS->is_request.rq_u.ru_planet = p;
        server(IS, rt_unlockPlanet, pNum);
    }
}

/*
 * editShip - part of cmd_edit
 */

void editShip(IMP, ULONG shipNumber)
{
    Ship_t ship;
    USHORT player;
    ItemType_t it;
    BigPart_t bp;
    USHORT maxNum;
    char *ptr = "";
    ULONG *numPtr = NULL;
    char buff[30];
    BOOL doWeight;

    doWeight = FALSE;

    /* read in the ship */
    server(IS, rt_readShip, shipNumber);
    ship = IS->is_request.rq_u.ru_ship;
    userN3(IS, "Ship #", shipNumber, " (");
    user(IS, getShipName(ship.sh_type));
    user(IS, " in fleet ");
    userC(IS, ship.sh_fleet);
    user(IS, "): lastUpdate = ");
    uTime(IS, ship.sh_lastUpdate);
    userNL(IS);
    /* edit the ships name */
    user3(IS, "Name: ", &ship.sh_name[0], "\n");
    uPrompt(IS, "name");
    if (clReadUser(IS))
    {
        if (*IS->is_textInPos != '\0')
        {
            if (strcmp(&IS->is_textIn[0], "none") == 0)
            {
                ship.sh_name[0] = '\0';
            }
            else
            {
                IS->is_textIn[SHIP_NAME_LEN - 1] = '\0';
                strcpy(&ship.sh_name[0], &IS->is_textIn[0]);
            }
        }
    }
    /* read in the owner */
    server(IS, rt_readPlayer, ship.sh_owner);
    /* edit the ships owner */
    user3(IS, "Owner: ", &IS->is_request.rq_u.ru_player.p_name[0], "\n");
    if (reqPlayer(IS, &player, "Owner"))
    {
        if (player == 0)
        {
            ship.sh_owner = NO_OWNER;
        }
        else
        {
            ship.sh_owner = player;
        }
    }
    /* edit the ships number */
    ship.sh_number = repUNum(IS, ship.sh_number, 0, IS->is_world.w_shipNext,
        "Ship number");
    /* edit the ships planet */
    ship.sh_planet = repUNum(IS, ship.sh_planet, 0, IS->is_world.w_planetNext,
        "Planet number");
    if (ship.sh_planet == 0)
    {
        /* remove the ship from the planet */
        ship.sh_planet = NO_ITEM;
    }
    /* edit the ships price */
    ship.sh_price = repUNum(IS, ship.sh_price, 0, 127, "Price");
    /* edit the ships efficiency */
    ship.sh_efficiency = repUNum(IS, ship.sh_efficiency, 0, 100, "Efficiency");
    /* edit the ships hullTF */
    ship.sh_hullTF = repUNum(IS, ship.sh_hullTF, 0, 254, "Hull TF");
    /* edit the ships engTF */
    ship.sh_engTF = repUNum(IS, ship.sh_engTF, 0, 254, "Engine TF");
    /* edit the ships energy banks */
    ship.sh_energy = repUNum(IS, ship.sh_energy, 0, MAX_WORK, "Energy");
    /* edit the ships fuel left */
    ship.sh_fuelLeft = repUNum(IS, ship.sh_fuelLeft, 0, MAX_WORK, "Fuel left");
    /* edit the ships row and col */
    ship.sh_row = repUNum(IS, ship.sh_row, 0, ((IS->is_world.w_rows - 1) * 10)
        + 9, "Current row");
    ship.sh_col = repUNum(IS, ship.sh_col, 0, ((IS->is_world.w_columns - 1) *
        10) + 9, "Current col");
    /* get the ships plague time and state */
    ship.sh_plagueStage = repUNum(IS, ship.sh_plagueStage, 0, 4,
        "Plague stage");
    ship.sh_plagueTime = repUNum(IS, ship.sh_plagueTime, 0, 127,
        "Plague time");
    /* edit the ships cargo amount */
    ship.sh_cargo = repUNum(IS, ship.sh_cargo, 0, 0xFFFF, "Cargo aboard");
    /* edit the ships armour left */
    ship.sh_armourLeft = repUNum(IS, ship.sh_armourLeft, 0, MAX_WORK,
        "Armour left");
    /* remove from the fleet ? */
    if (ask(IS, "Remove from fleet? "))
    {
       ship.sh_fleet = '*';
    }
    /* see if they want to edit the cargo lists */
    if (ask(IS, "Edit cargo lists? "))
    {
        /* loop through items */
        for (it = IT_FIRST; it <= IT_LAST; it++)
        {
            /* build up a string with the item type in it */
            (void) sprintf(&buff[0], "Amount of item type %c aboard",
                IS->is_itemChar[it]);
            /* prompt them to change it */
            ship.sh_items[it] = repUNum(IS, ship.sh_items[it], 0, 0xFFFF,
                &buff[0]);
        }
    }
    /* see if they want to edit the big item lists */
    if (ask(IS, "Edit big item lists? "))
    {
        /* edit the ships hull number */
        ship.sh_hull = repUNum(IS, ship.sh_hull, 0, NO_ITEM, "Hull number");
        /* now we want to loop through all the other big item arrays */
        for (bp = BP_FIRST; bp <= BP_LAST; bp++)
        {
            /* set up the values based on the item type */
            switch(bp)
            {
                case bp_computer:
                    maxNum = MAX_COMP;
                    ptr = "Computer";
                    numPtr = &ship.sh_computer[0];
                    break;
                case bp_engines:
                    maxNum = MAX_ENG;
                    ptr = "Engine";
                    numPtr = &ship.sh_engine[0];
                    break;
                case bp_lifeSupp:
                    maxNum = MAX_LIFESUP;
                    ptr = "Life supp";
                    numPtr = &ship.sh_lifeSupp[0];
                    break;
                case bp_sensors:
                    /* note that we only specify one of the modules */
                    maxNum = MAX_ELECT;
                    ptr = "Electronic module";
                    numPtr = &ship.sh_elect[0];
                    break;
                case bp_photon:
                    /* note that we only specify one of the weapons */
                    maxNum = MAX_WEAP;
                    ptr = "Weapon";
                    numPtr = &ship.sh_weapon[0];
                    break;
                default:
                    maxNum = 0;
                    break;
            }
            /* if any have been set... */
            if (maxNum)
            {
                /* loop through the items */
                for (it = 0; it < maxNum; it++)
                {
                    /* build up the prompt */
                    (void) sprintf(&buff[0], "%s number %u", ptr, it);
                    numPtr[it] = repUNum(IS, numPtr[it], 0, NO_ITEM, &buff[0]);
                }
            }
        }
    }
    /* If not in "info" mode, then write changes */
    if (!IS->is_BOOL1)
    {
        /* ask if they want the weight rebuilt */
        if (ask(IS, "Rebuild ship weight? "))
        {
            doWeight = TRUE;
        }
        /* lock the ship again */
        server(IS, rt_lockShip, shipNumber);
        IS->is_request.rq_u.ru_ship = ship;
        /* if they want the weight rebuilt, then do it */
        if (doWeight)
        {
            buildShipWeight(IS, &IS->is_request.rq_u.ru_ship);
        }
        /* all done with it */
        server(IS, rt_unlockShip, shipNumber);
    }
}

/*
 * putWorld - update the master copy of the world.
 */

void putWorld(IMP)
{
    if (!IS->is_BOOL1)
    {
        server(IS, rt_lockWorld, 0);
        IS->is_request.rq_u.ru_world = IS->is_world;
        server(IS, rt_unlockWorld, 0);
    }
}

/*
 * editWorld - part of cmd_edit.
 */

void editWorld(IMP)
{
    IS->is_world.w_maxPlayers = repUNum(IS, IS->is_world.w_maxPlayers, 2,
         PLAYER_MAX, "Maximum # players");
    IS->is_world.w_currPlayers = repUNum(IS, IS->is_world.w_currPlayers, 0,
        IS->is_world.w_maxPlayers, "Current # players");
    IS->is_world.w_maxConnect = repUNum(IS, IS->is_world.w_maxConnect, 1,
        60 * 24, "Maximum connect time in minutes");
    IS->is_world.w_maxBTUs = repUNum(IS, IS->is_world.w_maxBTUs, 10, 999,
        "Maximum BTUs held by one player");
    user3(IS, "Server Email Address: ", &IS->is_world.w_emailAddress[0], "\n");
    uPrompt(IS, "Server Email Address");
    if (clReadUser(IS))
    {
        if (*IS->is_textInPos != '\0')
        {
            IS->is_textIn[EMAIL_LEN - 1] = '\0';
            strcpy(&IS->is_world.w_emailAddress[0], &IS->is_textIn[0]);
        }
    }
    user3(IS, "Player creation password: ", &IS->is_world.w_password[0], "\n");
    uPrompt(IS, "Player creation password");
    if (clReadUser(IS))
    {
        if (*IS->is_textInPos != '\0')
        {
            IS->is_textIn[PASSWORD_LEN - 1] = '\0';
            strcpy(&IS->is_world.w_password[0], &IS->is_textIn[0]);
        }
    }
    user3(IS, "Last game won by: ", &IS->is_world.w_winName[0], "\n");
    uPrompt(IS, "Last winner");
    if (clReadUser(IS))
    {
        if (*IS->is_textInPos != '\0')
        {
            IS->is_textIn[NAME_LEN - 1] = '\0';
            strcpy(&IS->is_world.w_winName[0], &IS->is_textIn[0]);
        }
    }
    IS->is_world.w_loanNext = repUNum(IS, IS->is_world.w_loanNext, 0,
        0xffffffff, "Next loan");
    IS->is_world.w_offerNext = repUNum(IS, IS->is_world.w_offerNext, 0,
        0xffffffff, "Next offer");
    IS->is_world.w_shipNext = repUNum(IS, IS->is_world.w_shipNext, 0,
        0xffffffff, "Next ship");
    IS->is_world.w_fleetNext =  repUNum(IS, IS->is_world.w_fleetNext, 0,
        0xffff, "Next fleet");
    IS->is_world.w_planetNext =  repUNum(IS, IS->is_world.w_planetNext, 0,
        0xffffffff, "Next planet");
    IS->is_world.w_bigItemNext =  repUNum(IS, IS->is_world.w_bigItemNext, 0,
        0xffffffff, "Next big item");
    putWorld(IS);
}

/*
 * editProduction - part of cmd_edit.
 */

void editProduction(IMP)
{
    register PProd_t prod;

    IS->is_world.w_resCost = repUNum(IS, IS->is_world.w_resCost, 0, 127,
        "research cost");
    IS->is_world.w_techCost = repUNum(IS, IS->is_world.w_techCost, 0, 127,
        "technology cost");
    IS->is_world.w_missCost = repUNum(IS, IS->is_world.w_missCost, 0, 127,
        "missile cost");
    IS->is_world.w_planeCost = repUNum(IS, IS->is_world.w_planeCost, 0, 127,
        "plane cost");
    IS->is_world.w_barCost = repUNum(IS, IS->is_world.w_barCost, 0, 127,
        "bar cost");
    IS->is_world.w_airCost = repUNum(IS, IS->is_world.w_airCost, 0, 127,
        "air tank cost");
    IS->is_world.w_fuelCost = repUNum(IS, IS->is_world.w_fuelCost, 0, 127,
        "fuel tank cost");
    /* loop through the world's production points */
    for (prod = pp_weapon; prod <= PPROD_LAST; prod++)
    {
        switch(prod)
        {
            case pp_weapon:
            case pp_engine:
            case pp_electronics:
                IS->is_world.w_prodCost[prod] = repUNum(IS,
                    IS->is_world.w_prodCost[prod], 0, 255, PPROD_NAME[prod]);
                break;
            default:
                break;
        }
    }
    putWorld(IS);
}

/*
 * editMobilities - part of cmd_edit.
 */

void editMobilities(IMP)
{
    repMob(IS, &IS->is_world.w_defMob, "defMob");
    putWorld(IS);
}

/*
 * editPlague - part of cmd_edit.
 */

void editPlague(IMP)
{
    IS->is_world.w_plagueKiller = repUNum(IS, IS->is_world.w_plagueKiller,
            0, 2270, "plagueKiller");
    repScale(IS, &IS->is_world.w_plagueBooster, "plagueBooster");
    repRand(IS, &IS->is_world.w_plagueOneBase,
            &IS->is_world.w_plagueOneRand, "plague stage one");
    repRand(IS, &IS->is_world.w_plagueTwoBase,
            &IS->is_world.w_plagueTwoRand, "plague stage two");
    repRand(IS, &IS->is_world.w_plagueThreeBase, &IS->is_world.w_plagueThreeRand,
            "plague stage three");
    putWorld(IS);
}

/*
 * editCosts - edit various costs. (part of cmd_edit)
 */

void editCosts(IMP)
{
    IS->is_world.w_efficCost = repUNum(IS, IS->is_world.w_efficCost, 0, 1000,
        "efficCost");
    IS->is_world.w_milSuppliesCost = repUNum(IS, IS->is_world.w_milSuppliesCost,
        0, 800, "milSuppliesCost");
    IS->is_world.w_utilityRate = repUNum(IS, IS->is_world.w_utilityRate, 0, 100,
        "utilityRate");
    IS->is_world.w_interestRate = repUNum(IS, IS->is_world.w_interestRate, 0,
        200, "interestRate");
    IS->is_world.w_shipCostMult = repUNum(IS, IS->is_world.w_shipCostMult, 0,
        100, "shipCostMult");
    IS->is_world.w_refurbCost = repUNum(IS, IS->is_world.w_refurbCost, 0, 1000,
        "refurb cost");
    putWorld(IS);
}

/*
 * editScales - part of cmd_edit.
 */

void editScales(IMP)
{
    repScale(IS, &IS->is_world.w_hullScale, "hullScale");
    repScale(IS, &IS->is_world.w_engineScale, "engineScale");
    repScale(IS, &IS->is_world.w_resScale, "resScale");
    repScale(IS, &IS->is_world.w_techScale, "techScale");
    repScale(IS, &IS->is_world.w_defenseScale, "defenseScale");
    repScale(IS, &IS->is_world.w_missScale, "missScale");
    repScale(IS, &IS->is_world.w_planeScale, "planeScale");
    repScale(IS, &IS->is_world.w_goldScale, "goldScale");
    repScale(IS, &IS->is_world.w_ironScale, "ironScale");
    repScale(IS, &IS->is_world.w_barScale, "barScale");
    repScale(IS, &IS->is_world.w_shipWorkScale, "shipWorkScale");
    putWorld(IS);
}

/*
 * editUpdates - part of cmd_edit.
 */

void editUpdates(IMP)
{
    IS->is_world.w_secondsPerITU = repUNum(IS, IS->is_world.w_secondsPerITU,
        1, 60 * 60 * 24, "secondsPerITU");
    repScale(IS, &IS->is_world.w_efficScale, "efficScale");
    repScale(IS, &IS->is_world.w_mobilScale, "mobilScale");
    IS->is_world.w_highGrowthFactor = repUNum(IS, IS->is_world.w_highGrowthFactor,
            10, 2000, "highGrowthFactor");
    IS->is_world.w_lowGrowthFactor = repUNum(IS, IS->is_world.w_lowGrowthFactor,
            10, 4000, "lowGrowthFactor");
    IS->is_world.w_BTUDivisor = repUNum(IS, IS->is_world.w_BTUDivisor,
            500, 0xffff, "BTUDivisor");
    IS->is_world.w_resDecreaser = repUNum(IS, IS->is_world.w_resDecreaser,
            480, 0xffff, "resDecreaser");
    IS->is_world.w_techDecreaser = repUNum(IS, IS->is_world.w_techDecreaser,
            480, 0xffff, "techDecreaser");
    putWorld(IS);
}

/*
 * editFighting - part of cmd_edit.
 */

void editFighting(IMP)
{
    IS->is_world.w_assAdv = repUNum(IS, IS->is_world.w_assAdv, 0, 1000,
        "assAdv");
    IS->is_world.w_boardAdv = repUNum(IS, IS->is_world.w_boardAdv, 0, 10000,
        "boardAdv");
    putWorld(IS);
}

/*
 * editSea - part of cmd_edit.
 */

void editSea(IMP)
{
    IS->is_world.w_torpCost = repUNum(IS, IS->is_world.w_torpCost, 0, 10,
        "torpCost");
    IS->is_world.w_torpMobCost = repUNum(IS, IS->is_world.w_torpMobCost, 0, 127,
        "torpMobCost");
    IS->is_world.w_torpAcc = repUNum(IS, IS->is_world.w_torpAcc, 0, 100,
        "torpAcc");
    repRand(IS, &IS->is_world.w_torpBase, &IS->is_world.w_torpRand,
        "torpedo damage");
    IS->is_world.w_phaserAcc = repUNum(IS, IS->is_world.w_phaserAcc, 0, 100,
        "phaserAcc");
    IS->is_world.w_phaserRange = repUNum(IS, IS->is_world.w_phaserRange, 0,
        1000, "phaserRange");
    IS->is_world.w_phaserDmg = repUNum(IS, IS->is_world.w_phaserDmg, 0,
        100, "phaserDmg");
    repRand(IS, &IS->is_world.w_mineBase, &IS->is_world.w_mineRand,
        "mine damage");
    putWorld(IS);
}

/*
 * editAir - part of cmd_edit.
 */

void editAir(IMP)
{
    IS->is_world.w_fuelTankSize = repUNum(IS, IS->is_world.w_fuelTankSize,
        0, 500, "fuelTankSize");
    IS->is_world.w_fuelRichness = repUNum(IS, IS->is_world.w_fuelRichness,
        0, 400, "fuelRichness");
    IS->is_world.w_flakFactor = repUNum(IS, IS->is_world.w_flakFactor, 1, 100,
        "flakFactor");
    repScale(IS, &IS->is_world.w_landScale, "landScale");
    repRand(IS, &IS->is_world.w_bombBase, &IS->is_world.w_bombRand,
        "bomb damage");
    repRand(IS, &IS->is_world.w_planeBase, &IS->is_world.w_planeRand,
        "crashing plane damage");
    putWorld(IS);
}

/*
 * editMisc - part of cmd_edit.
 */

void editMisc(IMP)
{
    repScale(IS, &IS->is_world.w_contractScale, "contractScale");
    IS->is_world.w_deathFactor = repUNum(IS, IS->is_world.w_deathFactor, 0, 500,
        "deathFactor");
    IS->is_world.w_gunMax = repUNum(IS, IS->is_world.w_gunMax, 1, 20, "gunMax");
    repScale(IS, &IS->is_world.w_gunScale, "gunScale");
    IS->is_world.w_lookShipFact = repUNum(IS, IS->is_world.w_lookShipFact,
            10, 0xffff, "lookShipFact");
    repScale(IS, &IS->is_world.w_collectScale, "collectScale");
    repScale(IS, &IS->is_world.w_radarFactor, "radarFactor");
    IS->is_world.w_spyFactor = repUNum(IS, IS->is_world.w_spyFactor, 1, 1000,
        "spyFactor");
    IS->is_world.w_armourWeight = repUNum(IS, IS->is_world.w_armourWeight, 0,
        500, "armourWeight");
    IS->is_world.w_shipTechDecreaser = repUNum(IS,
        IS->is_world.w_shipTechDecreaser, 1, 1000, "shipTechDecreaser");
    putWorld(IS);
}

/*
 * editFlags - replace flag values.
 */

void editFlags(IMP)
{
    BOOL temp;

    temp = IS->is_world.w_nonDeityPower;
    repBool(IS, &temp, "Allow non Deity Power FORCE option");
    IS->is_world.w_nonDeityPower = temp;
    temp = IS->is_world.w_sendAll;
    repBool(IS, &temp, "Allow public messages");
    IS->is_world.w_sendAll = temp;
    temp = IS->is_world.w_chaPlay;
    repBool(IS, &temp, "Allow changing players");
    IS->is_world.w_chaPlay = temp;
    temp = IS->is_world.w_doFlush;
    repBool(IS, &temp, "Flush buffers on client termination");
    IS->is_world.w_doFlush = temp;
    temp = IS->is_world.w_userFlush;
    repBool(IS, &temp, "Allow normal users to use flush command");
    IS->is_world.w_userFlush = temp;
    temp = IS->is_world.w_noNameChange;
    repBool(IS, &temp, "Prevent users from changing their names");
    IS->is_world.w_noNameChange = temp;
    temp = IS->is_world.w_noCreatePass;
    repBool(IS, &temp, "Allow new players without knowing the creation"
        " password");
    IS->is_world.w_noCreatePass = temp;
    putWorld(IS);
}

/*
 * repNaval - replace values in a naval array.
 */

void repNaval(IMP, USHORT *pArray, char *what, USHORT minVal,
    USHORT maxVal)
{
    char promptBuffer[100];
    register ShipType_t st;

    for (st = ST_FIRST; st <= ST_LAST; st++)
    {
        user3(IS, what, " of ", getShipName(st));
        getPrompt(IS, &promptBuffer[0]);
        pArray[st] = repUNum(IS, pArray[st], minVal, maxVal,
            &promptBuffer[0]);
    }
    putWorld(IS);
}

/*
 * repItems - replace values in a items array.
 */

void repItems(IMP, USHORT *pArray, char *what, USHORT minVal,
    USHORT maxVal)
{
    char promptBuffer[100];
    register ItemType_t it;

    for (it = IT_FIRST; it <= IT_LAST; it++)
    {
        user3(IS, what, " of ", getItemName(it));
        getPrompt(IS, &promptBuffer[0]);
        pArray[it] = repUNum(IS, pArray[it], minVal, maxVal,
            &promptBuffer[0]);
    }
    putWorld(IS);
}

/*
 * editNaval - edit ship parameters.
 */

void editNaval(IMP)
{
    char buff[80];
    USHORT what;

    if (IS->is_BOOL1)
    {
        strcpy(&buff[0], 
            "Info on what (cargolim/cost/energcost/itemmobcost/weight)? ");
    }
    else
    {
        strcpy(&buff[0],
            "Edit what (cargolim/cost/energcost/itemmobcost/weight)? ");
    }
    if (reqChoice(IS, &what, "cargolim\0cost\0energcost\0itemmobcost\0"
        "weight\0", &buff[0]))
    {
        switch(what)
        {
            case 0:
                repNaval(IS, IS->is_world.w_shipCargoLim, "cargo limit", 0,
                    (USHORT)0xFFFF);
                break;
            case 1:
                repNaval(IS, IS->is_world.w_shipCost, "cost", 0, (USHORT) 0xFFFF);
                break;
            case 2:
                repNaval(IS, IS->is_world.w_baseFuelCost, "energy cost", 0,
                    (USHORT) 0xFFFF);
                break;
            case 3:
                repItems(IS, IS->is_world.w_mobCost, "mobility cost", 0,
                    (USHORT) 0xFFFF);
                break;
            case 4:
                repItems(IS, IS->is_world.w_weight, "default weight", 0,
                    (USHORT) 0xFFFF);
                break;
        }
    }
}

/*
 * editRace - part of cmd_edit
 */

void editRace(IMP, USHORT rNum)
{
    Race_t r;

    server(IS, rt_readWorld, 0);
    r = IS->is_request.rq_u.ru_world.w_race[rNum];
    user3(IS, "Name: ", &r.r_name[0], "\n");
    uPrompt(IS, "Name");
    if (clReadUser(IS))
    {
        if (*IS->is_textInPos != '\0')
        {
            IS->is_textIn[NAME_LEN - 1] = '\0';
            strcpy(&r.r_name[0], &IS->is_textIn[0]);
        }
    }
    user3(IS, "Home name: ", &r.r_homeName[0], "\n");
    uPrompt(IS, "Home name");
    if (clReadUser(IS))
    {
        if (*IS->is_textInPos != '\0')
        {
            IS->is_textIn[PLAN_NAME_LEN - 1] = '\0';
            strcpy(&r.r_homeName[0], &IS->is_textIn[0]);
        }
    }
    r.r_homeRow = repUNum(IS, r.r_homeRow, 0, 0xffff, "Home row");
    r.r_homeCol = repUNum(IS, r.r_homeCol, 0, 0xffff, "Home col");
    r.r_status = repUNum(IS, r.r_status, rs_notyet, rs_dead, "Status");
    r.r_homePlanet = repUNum(IS, r.r_homePlanet, 0, 0xFFFFFFFF,
        "Home planet");
    r.r_planetCount = repUNum(IS, r.r_planetCount, 0, 0xffffffff,
        "Planet count");
    r.r_techLevel = repUNum(IS, r.r_techLevel, 0, 0xffffffff, "Tech level");
    r.r_resLevel = repUNum(IS, r.r_resLevel, 0, 0xffffffff, "Res level");
    r.r_playCount = repUNum(IS, r.r_playCount, 0, PLAYER_MAX - 1,
        "Player count");
    IS->is_world.w_race[rNum] = r;
    putWorld(IS);
}

/*
 * doEdit - function common to both cmd_edit and cmd_info which asks the
 *          user what they want to look at
 */

void doEdit(IMP)
{
    char *prompt;
    USHORT what, n;
    ULONG pNum, sNum;

    if (IS->is_BOOL1)
    {
        /* info */
        prompt = "Info on what (we/pr/mo/pl/cos/sc/up/fi/sea/ai/mi/na/fl)? ";
    }
    else
    {
        /* edit */
        prompt =
        "Edit what (play/rac/plan/sh/wor/prod/mob/plag/cost/sc/upd/fi/sea/"
        "air/misc/nav/fl)? ";
    }
    if (reqChoice(IS, &what,
        "player\0"
        "planet\0"
        "ship\0"
        "world\0"
        "production\0"
        "mobility\0"
        "plague\0"
        "costs\0"
        "scales\0"
        "updates\0"
        "fighting\0"
        "sea\0"
        "air\0"
        "miscellaneous\0"
        "naval\0"
        "flags\0"
        "race\0", prompt))
    {
        (void) skipBlanks(IS);
        switch(what)
        {
            case 0:
                if (IS->is_BOOL1)
                {
                    err(IS, "no such info");
                }
                else
                {
                    if (reqPlayer(IS, &n, "Edit which player? "))
                    {
                        editPlayer(IS, n);
                    }
                }
                break;
            case 1:
                if (IS->is_BOOL1)
                {
                    err(IS, "no such info");
                }
                else
                {
                    if (reqPlanet(IS, &pNum, "Edit which planet? "))
                    {
                        editPlanet(IS, pNum);
                    }
                }
                break;
            case 2:
                if (IS->is_BOOL1)
                {
                    err(IS, "no such info");
                }
                else
                {
                    if (reqShip(IS, &sNum, "Edit which ship? "))
                    {
                        editShip(IS, sNum);
                    }
                }
                break;
            case 3:
                if (IS->is_BOOL1)
                {
                    err(IS, "no such info");
                }
                else
                {
                    editWorld(IS);
                }
                break;
            case 4:
                editProduction(IS);
                break;
            case 5:
                editMobilities(IS);
                break;
            case 6:
                editPlague(IS);
                break;
            case 7:
                editCosts(IS);
                break;
            case 8:
                editScales(IS);
                break;
            case 9:
                editUpdates(IS);
                break;
            case 10:
                editFighting(IS);
                break;
            case 11:
                editSea(IS);
                break;
            case 12:
                editAir(IS);
                break;
            case 13:
                editMisc(IS);
                break;
            case 14:
                editNaval(IS);
                break;
            case 15:
                editFlags(IS);
                break;
            case 16:
                if (IS->is_BOOL1)
                {
                    err(IS, "no such info");
                }
                else
                {
                    if (reqRace(IS, &n, TRUE, "Edit which player? "))
                    {
                        editRace(IS, n);
                    }
                }
                break;
            default:
                break;
        }
    }
}

/*
 * cmd_edit - used by deities to make changes to items in the world.
 */

void cmd_edit(IMP)
{
    if (IS->is_player.p_status == ps_deity)
    {
        IS->is_BOOL1 = FALSE;
        doEdit(IS);
        return;
    }
    err(IS, "Only a deity can edit things");
}

/*
 * cmd_info - Entry point for the command that displays the various stats
 *          to the normal people playing the game
 */

void cmd_info(IMP)
{
    char firstChar;

    firstChar = *skipBlanks(IS);
    if (firstChar == '\0')
    {
        user3(IS, "Last game won by: ", &IS->is_world.w_winName[0], "\n");
        user3(IS, "Server Email Address: ", &IS->is_world.w_emailAddress[0], "\n");
        user(IS, "World created ");
        uTime(IS, IS->is_world.w_buildDate);
        user(IS, ".\n");
	feWorldSize(IS);
        userN2(IS, "World size: ", IS->is_world.w_rows);
        userN3(IS, " rows by ", IS->is_world.w_columns,
            " columns, and contains ");
        userN(IS, IS->is_world.w_planetNext);
        user(IS, " stars and planets\n");
        userN2(IS, "There are currently ", IS->is_world.w_currPlayers);
        userN3(IS, " players out of a maximum of ", IS->is_world.w_maxPlayers,
               ".\n");
        userN3(IS, "Maximum daily connect time is ", IS->is_world.w_maxConnect,
               " minutes.\n");
        userN3(IS, "An ITU (Imperium Time Unit) is ", IS->is_world.w_secondsPerITU,
               " seconds.\n");
        userN2(IS, "The world has seen ", IS->is_world.w_loanNext);
        userN2(IS, " loans, ", IS->is_world.w_offerNext);
        userN2(IS, " offers,\n", IS->is_world.w_bigItemNext);
        userN2(IS, " big items, ", IS->is_world.w_shipNext);
        userN3(IS, " ships and ", IS->is_world.w_fleetNext, " fleets.\n");
        return;
    }
    if (firstChar == '?')
    {
        user(IS, "Info is available on:\n"
            "  production - production unit costs\n"
            "  mobility   - mobility costs of various actions/movements\n"
            "  plague     - the various factors that influence plague\n"
            "  costs      - various monetary costs\n"
            "  scales     - various work scale factors\n"
            "  updates    - factors affecting planet updates\n"
            "  fighting   - various factors that affect combat\n"
            "  sea        - various factors relevant to sea conflict\n"
            "  air        - some factors affecting airplanes\n"
            "  misc       - a few miscellaneous factors\n"
            "  naval      - various items relating to ships\n"
            "  flags      - some BOOLean flags affecting permissions\n");
        return;
    }
    IS->is_BOOL1 = TRUE;
    doEdit(IS);
    IS->is_BOOL1 = FALSE;
}

void createPlanet(IMP)
{
    user(IS, "Called createPlanet()\n");
}

void createItem(IMP)
{
    user(IS, "Called createItem()\n");
}

void createShip(IMP)
{
    user(IS, "Called createShip()\n");
}

void createLoan(IMP)
{
    user(IS, "Called createLoan()\n");
}

/*
 * cmd_create - Allows deities to create various items in the world directly
 */

void cmd_create(IMP)
{
    USHORT what;

    if (IS->is_player.p_status != ps_deity)
    {
        err(IS, "only a deity can directly create things");
        return;
    }
    if (reqChoice(IS, &what, "planet\0item\0ship\0loan\0",
        "Create what (planet/item/ship/loan)? "))
    {
        (void) skipBlanks(IS);
        switch(what)
        {
            case 0:
                createPlanet(IS);
                break;
            case 1:
                createItem(IS);
                break;
            case 2:
                createShip(IS);
                break;
            case 3:
                createLoan(IS);
                break;
        }
    }
}
