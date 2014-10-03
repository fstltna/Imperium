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
 * $Id: cmd_naval2.c,v 1.2 2000/05/18 06:50:02 marisa Exp $
 *
 * $Log: cmd_naval2.c,v $
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:51  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:24:22  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 19:28:19  marisag
 * First branch by me
 *
 * Revision 3.5  1997/03/11 18:46:01  marisag
 * New branch
 *
 */

#include "../config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
BOGUS - Imperium can not run on this machine due to missing stdlib.h!
#endif
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"

static const char rcsid[] = "$Id: cmd_naval2.c,v 1.2 2000/05/18 06:50:02 marisa Exp $";

/*
 * cmd_refuel - allows a player to transfer fuel from fueltanks to the
 *          fuelLeft area to be converted to energy
 */

BOOL cmd_refuel(IMP)
{
    register Ship_t *rsh;
    ULONG shNum;

    /* get the number of the ship */
    if (reqShip(IS, &shNum, "Ship to refuel"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        server(IS, rt_readShip, shNum);
        if (rsh->sh_owner != IS->is_player.p_number)
        {
            user(IS, "You don't own that ship\n");
        }
        else if (rsh->sh_type == st_m)
        {
            user(IS, "Use the 'miner' command to load fuel onto a miner\n");
            /* return FALSE here, since this could be a common mistake */
            return FALSE;
        }
        else
        {
            server(IS, rt_lockShip, shNum);
            updateShip(IS);
            if (rsh->sh_items[it_fuelTanks] == 0)
            {
                server(IS, rt_unlockShip, shNum);
                user(IS, "There are no fuel tanks on that ship\n");
            }
            else
            {
                rsh->sh_items[it_fuelTanks]--;
                rsh->sh_fuelLeft += IS->is_world.w_weight[it_fuelTanks];
                server(IS, rt_unlockShip, shNum);
                user(IS, "Fuel transfered to active fuel array\n");
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * showError - displays an error message to the user and points out at what
 *          part of the line the error occured
 */

void showError(IMP, char *course, char *where, char *desc)
{
    register char *p;

    p = course;
    userNL(IS);
    user2(IS, course, "\n");
    while (p != where)
    {
        userSp(IS);
        p += sizeof(char);
    }
    user(IS, "^\n");
    user3(IS, "Error: ", desc, "\n");
}

/*
 * checkCourse - does some basic syntax checking on a course that the
 *          user has just entered.
 */

void checkCourse(IMP, register char *course)
{
    register char *p, *q;

    /* first check for invalid characters */
    p = course;
    while (*p != '\0')
    {
        if ((isalnum(*p) == 0) && (*p != '[') && (*p != ',') && (*p != ']') &&
            (*p != '@'))
        {
            /* was not a valid character for programs */
            showError(IS, course, p, "Invalid character");
        }
        p += sizeof(char);
    }

    /* now verify that "counted" movements are valid */
    p = course;
    while (*p != '\0')
    {
        /* look for start of range */
        if (*p == '[')
        {
            q = p + sizeof(char);
            if (*q != '\0')
            {
                p = q;
                while (isdigit(*q) != 0)
                {
                    q += sizeof(char);
                }
                if (*q == '\0')
                {
                    showError(IS, course, q, "Unexpected program end");
                    return;
                }
                if (p == q)
                {
                    showError(IS, course, q, "Numbers expected in block");
                    return;
                }
                if (*q != ',')
                {
                    /* seperator was not a comma */
                    showError(IS, course, q, "Comma expected in count");
                    return;
                }
                q += sizeof(char);
                p = q;
                while (isdigit(*q) != 0)
                {
                    q += sizeof(char);
                }
                if (*q == '\0')
                {
                    showError(IS, course, q, "Unexpected program end");
                    return;
                }
                if (p == q)
                {
                    showError(IS, course, q, "Numbers expected in block");
                    return;
                }
                if (*q != ']')
                {
                    /* not a proper count end character */
                    showError(IS, course, q, "Block end expected in "
                        "count");
                    return;
                }
                q += sizeof(char);
                if (*q == '\0')
                {
                    showError(IS, course, q, "Unexpected program "
                        "end");
                    return;
                }
                if (isdigit(*q) == 0)
                {
                    /* was not a valid movement direction */
                    showError(IS, course, q, "Illegal movement "
                        "direction");
                    return;
                }
                p = q;
            }
            else
            {
                showError(IS, course, q, "Unexpected program end");
                return;
            }
        }
        else if (*p == ']')
        {
            /* should never see these without a '[' in front of them */
            showError(IS, course, p, "Unexpected block end");
            return;
        }
        else if (*p == ',')
        {
            /* should never see these outside a block */
            showError(IS, course, p, "Unexpected count seperator");
            return;
        }
        else if (*p == '@')
        {
            /* make sure there is something after the loop character */
            q = p + sizeof(char);
            if (*q == '\0')
            {
                showError(IS, course, q, "Nothing after loop character");
                return;
            }
        }
        p += sizeof(char);
    }
}

/*
 * cmd_program - allows a user to "program" a ship's course
 */

BOOL cmd_program(IMP)
{
    register Ship_t *rsh;
    ULONG shNum;

    /* get the number of the ship */
    if (reqShip(IS, &shNum, "Ship to program"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        server(IS, rt_readShip, shNum);
        if (rsh->sh_owner != IS->is_player.p_number)
        {
            user(IS, "You don't own that ship\n");
        }
        else if (rsh->sh_type == st_m)
        {
            user(IS, "Use the 'miner' command to program a miner\n");
            /* return FALSE since this could be a common mistake */
            return FALSE;
        }
        else
        {
            if (rsh->sh_course[0] != '\0')
            {
                user3(IS, "Current program: ", &rsh->sh_course[0], "\n");
            }
            uPrompt(IS, "Enter new program");
            if (clReadUser(IS) && (*IS->is_textInPos != '\0'))
            {
                server(IS, rt_lockShip, shNum);
                if (strcmp(IS->is_textInPos, "none") == 0)
                {
                    rsh->sh_course[0] = '\0';
                }
                else
                {
                    IS->is_textIn[MAX_PROG_STEPS - 1] = '\0';
                    strncpy(&rsh->sh_course[0], &IS->is_textIn[0],
                        (MAX_PROG_STEPS - 1) * sizeof(char));
                }
                server(IS, rt_unlockShip, shNum);
            }
            /* if there is a course, do some basic syntax checking */
            if (rsh->sh_course[0] != '\0')
            {
                checkCourse(IS, &rsh->sh_course[0]);
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * rollString - rolls a string down with wrap-around
 *          "knows" about the loop character, and if it is the first
 *          character on the line, correctly wraps from the next
 *          character.
 */

void rollString(USHORT rollBy, char *str)
{
    char buff[257];
    USHORT startAt;

    startAt = 0;
    if (*str == '\0')
    {
        /* don't do anything for null strings */
        return;
    }
    /* look for a loop character */
    if (*str == '@')
    {
        buff[startAt] = '@';
        startAt = 1;
    }
    /* copy the part of the string that always get's moved */
    /* note this assumes that if you ask it to roll by x chars */
    /* that there are at least x chars left to roll */
    strcpy(&buff[startAt], &str[rollBy + startAt]);

    /* if we are looping, copy the other part of the string to the */
    /* tail end */
    if (startAt == 1)
    {
        str[rollBy + startAt] = '\0';
        strcat(&buff[0], &str[startAt]);
    }

    /* copy the completed string back */
    /* note this assumes the destination is AT LEAST the full size */
    /* of the programmed course area */
    strcpy(str, &buff[0]);
}

/*
 * doCountNavOnce - executes navOnce a given amount of times for
 *          a certain movement direction. Returns NULL if
 *          navOnce returns anything but m_continue, or returns a pointer to
 *          the position in the string where it left off.
 *          If it is unable to execute all items in the loop
 *          it will rebuild the string with the new loop values,
 *          and return NULL
 */

char *doCountNavOnce(IMP, char *oPtr, char *program, USHORT *flagRow,
    USHORT *flagCol, USHORT *flagMobil, USHORT *minMobil)
{
    char *bPtr, *mPtr, *ePtr, *sPtr;
    USHORT current, end;
    BOOL cont;
    char num1[16], num2[10], course[MAX_PROG_STEPS];
    memset(num1, '\0', 16 * sizeof(char));
    memset(num2, '\0', 10 * sizeof(char));
    memset(course, '\0', MAX_PROG_STEPS * sizeof(char));

    sPtr = program;
    if (program[0] == '@')
    {
        sPtr += sizeof(char);
    }
    bPtr = sPtr + sizeof(char);
    /* make sure that there is at least one digit */
    if (*bPtr == '\0')
    {
        user(IS, "unexpected end of program\n");
        rollString(bPtr - sPtr, program);
        return NULL;
    }
    if ((*bPtr < '0') || (*bPtr > '9'))
    {
        user(IS, "invalid character in first number of loop: '");
        userC(IS, *bPtr);
        user(IS, "'\n");
        rollString((bPtr - sPtr) + 1, program);
        return oPtr + (bPtr - sPtr);
    }
    mPtr = bPtr;
    while ((*mPtr >= '0') && (*mPtr <= '9'))
    {
        mPtr++;
    }
    if (*mPtr == '\0')
    {
        user(IS, "unexpected end of program\n");
        rollString(mPtr - sPtr, program);
        return NULL;
    }
    /* verify that the seperator is a "," */
    if (*mPtr != ',')
    {
        user(IS, "invalid seperation character: '");
        userC(IS, *mPtr);
        user(IS, "'\n");
        rollString((mPtr - sPtr) + 1, program);
        return oPtr + (mPtr - sPtr);
    }
    memcpy(&num1[0], bPtr, umin((mPtr - bPtr), (USHORT) 5) * sizeof(char));
    mPtr++;
    if (*mPtr == '\0')
    {
        user(IS, "unexpected end of program\n");
        rollString(mPtr - sPtr, program);
        return NULL;
    }
    if ((*mPtr < '0') || (*mPtr > '9'))
    {
        user(IS, "invalid character in 2nd number in loop: '");
        userC(IS, *mPtr);
        user(IS, "'\n");
        rollString((mPtr - sPtr) + 1, program);
        return oPtr + (mPtr - sPtr);
    }
    ePtr = mPtr;
    while ((*ePtr >= '0') && (*ePtr <= '9'))
    {
        ePtr++;
    }
    if (*ePtr == '\0')
    {
        user(IS, "unexpected end of program\n");
        rollString(ePtr - sPtr, program);
        return NULL;
    }
    /* verify that we got the proper closure character */
    if (*ePtr != ']')
    {
        user(IS, "invalid closure character: '");
        userC(IS, *ePtr);
        user(IS, "'\n");
        rollString((ePtr - sPtr) + 1, program);
        return oPtr + (ePtr - sPtr);
    }
    memcpy(&num2[0], mPtr, umin((ePtr - mPtr), (USHORT) 5) * sizeof(char));

    ePtr++;
    /* verify that there is a valid direction following the count */
    if (*ePtr == '\0')
    {
        user(IS, "unexpected end of program\n");
        rollString(ePtr - sPtr, program);
        return NULL;
    }
    if ((*ePtr < '0') || (*ePtr > '9'))
    {
        user(IS, "invalid navigation character: '");
        userC(IS, *ePtr);
        user(IS, "'\n");
        rollString((ePtr - sPtr) + 1, program);
        return oPtr + (ePtr - sPtr);
    }
    current = atoi(num1);
    end = atoi(num2);
    if (current >= end)
    {
        userN2(IS, "first number (", current);
        userN3(IS, ") is >= other number (", end, ")\n");
        rollString((ePtr - sPtr) + 1, program);
        return oPtr + (ePtr - sPtr);
    }
    cont = TRUE;
    while(cont && (current < end))
    {
        cont = ((navOnce(IS, *ePtr, flagRow, flagCol,
            flagMobil, minMobil)) == m_continue);
        if (cont)
        {
            current++;
        }
    }
    /* if cont is TRUE, then must have done all desired movements */
    if (cont)
    {
        current = 0;
    }
    sprintf(num1, "[%d,%d]%c", current, end, *ePtr);
    /* make sure that the loop is not the first thing */
    if (sPtr != program)
    {
        course[0] = '@';
        bPtr = &course[1];
    }
    else
    {
        bPtr = course;
    }
    strcat(course, num1);
    strcat(course, ePtr + sizeof(char));
    if (!cont)
    {
        strcpy(program, course);
        return NULL;
    }
    mPtr = &course[3];
    while(*mPtr != ']')
    {
        mPtr++;
    }
    mPtr++;
    rollString((mPtr - bPtr) + 1, course);
    strcpy(program, course);
    return oPtr + (ePtr - sPtr);
}

/*
 * runNavProg - runs a navigation program
 *            Note: Assumes that "program" is a pointer to the ACTUAL
 *            program field in the ship, and that any changes it makes to
 *            the program will get saved with the ship. If you are pointing
 *            to somewhere else, you MUST put it back into the ship for things
 *            to work properly.
 */

void runNavProg(IMP, char *program, USHORT *flagRow, USHORT *flagCol,
    USHORT *flagMobil, USHORT *minMobil)
{
    BOOL cont, needRoll;
    char progBuf[MAX_PROG_STEPS];
    char *sPtr;

    sPtr = &progBuf[0];
    /* copy initial program, since we will be munging it up */
    /* and we only want to loop through the program once */
    strcpy(sPtr, program);
    /* skip past any loop characters */
    if (*sPtr == '@')
    {
        sPtr++;
    }
    /* loop until end of program or no more fuel */
    cont = TRUE;
    while (cont)
    {
        /* need to roll string unless told otherwise */
        needRoll = TRUE;
        /* look for a "plain" navigation direction */
        if ((*sPtr >= '0') && (*sPtr <= '9'))
        {
            /* execute movement command */
            cont = ((navOnce(IS, *sPtr, flagRow, flagCol,
                flagMobil, minMobil)) == m_continue);
            /* don't roll if it returns bad */
            needRoll = cont;
        }
        else if ((*sPtr >= 'a') && (*sPtr <= 'z'))
        {
            /* execute an action */
            cont = handleAction(IS, *sPtr);
            /* don't roll if it returns bad */
            needRoll = cont;
        }
        else if (*sPtr == '[')
        {
            /* try and execute a counted movement command */
            needRoll = FALSE;
            sPtr = doCountNavOnce(IS, sPtr, program, flagRow, flagCol,
                flagMobil, minMobil);
            if (sPtr == NULL)
            {
                cont = FALSE;
            }
        }
        else
        {
            /* some other character */
            if (*sPtr == '\0')
            {
                /* end of line */
                cont = FALSE;
            }
            else
            {
                /* is it an embedded loop character? */
                if (*sPtr != '@')
                {
                    /* nope */
                    user(IS, "invalid character in program: '");
                    userC(IS, *sPtr);
                    user(IS, "'\n");
                }
                else
                {
                    needRoll = FALSE;
                }
            }
        }
        /* if not at end of line, roll by one */
        if (needRoll && (*sPtr  != '\0'))
        {
            rollString(1, program);
        }
        /* look at next character */
        sPtr++;
    }
}


/*
 * cmd_run - allows the player to "run" a ship's program
 */

BOOL cmd_run(IMP)
{
    register Ship_t *rsh;
    register Nav_t *nav;
    Fleet_t fl;
    ULONG shNum, curNum;
    USHORT msc, flagRow, flagCol, flagMobil, minMobil, i;
    char progBuf[MAX_PROG_STEPS];
    char fleet;
    BOOL gotOne, tooMany;

    /* get the number of the ship */
    if (reqShip(IS, &shNum, "Run program on which ship"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        server(IS, rt_readShip, shNum);
        gotOne = FALSE;
        tooMany = FALSE;
        if (rsh->sh_owner != IS->is_player.p_number)
        {
            user(IS, "You don't own that ship\n");
        }
        else if (rsh->sh_type == st_m)
        {
            user(IS, "You do not need to use the 'run' command with a "
                "miner\n");
            return FALSE;
        }
        else if (rsh->sh_course[0] == '\0')
        {
            user(IS, "Ship has no program!\n");
        }
        else
        {
            msc = 0;
            /* copy the current course */
            strcpy(&progBuf[0], &rsh->sh_course[0]);
            /* set the default for the fleet */
            fleet = '*';
            /* see if the ship is in a fleet */
            if (rsh->sh_fleet != '*')
            {
                /* it is, so set fleet apropriately */
                fleet = rsh->sh_fleet;
            }
            /* was there a fleet? */
            if (fleet == '*')
            {
                /* no, so do single-ship things */
                accessShip(IS, shNum);
                if (rsh->sh_efficiency < EFFIC_WARN)
                {
                    err(IS, "that ship isn't efficient enough to navigate");
                }
                else
                {
                    IS->is_movingShipCount = 1;
                    msc = 1;
                    nav = &IS->is_movingShips[0];
                    nav->n_ship = shNum;
                    nav->n_mobil = ((short)rsh->sh_energy) * 10;
                    nav->n_active = TRUE;
                    flagMobil = rsh->sh_energy;
                    minMobil = rsh->sh_energy;
                    flagRow = rsh->sh_row;
                    flagCol = rsh->sh_col;
                    decrShipCount(IS, mapSector(IS, rsh->sh_row,
                        rsh->sh_col));
                }
            }
            else
            {
                /* yes, so set it up */
                i = fleetPos(fleet);
                if (IS->is_player.p_fleets[i] == NO_FLEET)
                {
                    log3(IS, "attempted to run a program for fleet not used "
                        "by player ", &IS->is_player.p_name[0], "");
                    err(IS, "you are not using the fleet listed for the ship!"
                        " Notify the deity!");
                }
                else
                {
                    server(IS, rt_readFleet, IS->is_player.p_fleets[i]);
                    fl = IS->is_request.rq_u.ru_fleet;
                    if (fl.f_count == 0)
                    {
                        log3(IS, "attempted to run a program for fleet "
                            "which believed itself to be empty "
                            "by player ", &IS->is_player.p_name[0], "");
                        err(IS, "fleet believes it has no ships! "
                            "Notify the deity!");
                    }
                    else
                    {
                        gotOne = TRUE;
                        for (i = 0; i < fl.f_count; i++)
                        {
                            curNum = fl.f_ship[i];
                            accessShip(IS, curNum);
                            if (rsh->sh_efficiency < EFFIC_WARN)
                            {
                                user(IS, getShipName(rsh->sh_type));
                                userN3(IS, " #", curNum,
                                    " isn't efficient enough to "
                                    "navigate.\n");
                            }
                            else if (rsh->sh_planet != NO_ITEM)
                            {
                                user(IS, getShipName(rsh->sh_type));
                                userN3(IS, " #", curNum,
                                    " is on the surface of a planet\n");
                            }
                            else
                            {
                                if (msc == 0)
                                {
                                    flagMobil = rsh->sh_energy;
                                    minMobil = rsh->sh_energy;
                                    flagRow = rsh->sh_row;
                                    flagCol = rsh->sh_col;
                                }
                                if (msc == MAX_NAV_SHIPS)
                                {
                                    if (!tooMany)
                                    {
                                        tooMany = TRUE;
                                        err(IS, "too many ships to navigate "
                                            "at once");
                                    }
                                }
                                else
                                {
                                    nav = &IS->is_movingShips[msc];
                                    nav->n_ship = curNum;
                                    nav->n_mobil = ((short)rsh->sh_energy) *
                                        10;
                                    nav->n_active = TRUE;
                                    msc++;
                                    IS->is_movingShipCount = msc;
                                    if (rsh->sh_energy < minMobil)
                                    {
                                        minMobil = rsh->sh_energy;
                                    }
                                    decrShipCount(IS, mapSector(IS, rsh->sh_row,
                                        rsh->sh_col));
                                }
                            }
                        }
                    }
                }
            }
            if (msc == 0)
            {
                if (gotOne)
                {
                    err(IS, "no ships in that fleet can navigate");
                }
            }
            else if (flagMobil == 0)
            {
                err(IS, "no energy");
            }
            else if (!tooMany)
            {
                /* scale mobilities by 10 */
                flagMobil = flagMobil * 10;
                minMobil = minMobil * 10;

                /* now actually run the program */
                runNavProg(IS, &progBuf[0], &flagRow, &flagCol, &flagMobil,
                    &minMobil);

                /* now remove the ships from the navlist */
                for (i = 0; i < IS->is_movingShipCount; i++)
                {
                    nav = &IS->is_movingShips[i];
                    if (nav->n_active)
                    {
                        curNum = nav->n_ship;
                        server(IS, rt_lockShip, curNum);
                        rsh->sh_energy = nav->n_mobil / 10;
                        server(IS, rt_unlockShip, curNum);
                        incrShipCount(IS, mapSector(IS, rsh->sh_row,
                            rsh->sh_col));
                    }
                }

                /* copy the modifief course back into the ship */
                server(IS, rt_lockShip, shNum);
                strcpy(&rsh->sh_course[0], &progBuf[0]);
                server(IS, rt_unlockShip, shNum);
            }
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * updateAllMiners - updates all the miners in the world. This helps to
 *          add to the illusion of the miners "doing their thing" without
 *          user interaction
 */

void updateAllMiners(IMP)
{
    register ULONG ship;
    register Ship_t *rsh;

    /* set up the pointer */
    rsh = &IS->is_request.rq_u.ru_ship;

#ifdef DEBUG_LIB
    if (IS->is_DEBUG)
    {
        log3(IS, ">>> In updateAllMiners", "", "");
    }
#endif
    /* loop through all the ships in the world looking for miners */
    for (ship = 0; ship < IS->is_world.w_shipNext; ship++)
    {
        /* read the current ship in */
        server(IS, rt_readShip, ship);
        /* if it is a miner, update it */
        if ((rsh->sh_type == st_m) && (rsh->sh_owner != NO_OWNER))
        {
#ifdef DEBUG_LIB
            if (IS->is_DEBUG)
            {
		char numBuff[80];
		sprintf(&numBuff[0], "%lu", ship);
		log3(IS, "!!! About to lock miner ", &numBuff[0], "");
            }
#endif
            server(IS, rt_lockShip, ship);
            updateMiner(IS);
            server(IS, rt_unlockShip, ship);
        }
    }
#ifdef DEBUG_LIB
    if (IS->is_DEBUG)
    {
        log3(IS, "<<< Out of updateAllMiners", "", "");
    }
#endif
}

/*
 * minerEmpty - empty the ore from a miner onto a ship
 */

void minerEmpty(IMP)
{
    register Ship_t *rsh, *rp;
    ULONG shipNumber, mineNum;
    USHORT ore, bars;

    /* get the number of the miner */
    if (reqShip(IS, &mineNum, "Miner to empty"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rp = &IS->is_request.rq_u.ru_shipPair[0];
        (void) skipBlanks(IS);
        server(IS, rt_readShip, mineNum);
        if (rsh->sh_owner != IS->is_player.p_number)
        {
            err(IS, "you don't own that miner");
        }
        else if (rsh->sh_type != st_m)
        {
            err(IS, "that ship is not a miner");
        }
        /* make sure the ship is on a planet */
        else if (rsh->sh_planet != NO_ITEM)
        {
            err(IS, "miner is on the surface of a planet");
            return;
        }
        else
        {
            /* make sure the miner is in a ship */
            if (rsh->sh_dragger == NO_ITEM)
            {
                err(IS, "miner is not inside a ship - tell system owner!");
                return;
            }

            /* get the number of the ship the miner is on */
            shipNumber = rsh->sh_dragger;
            /* update the carrying ship */
            server(IS, rt_lockShip, shipNumber);
            updateShip(IS);
            server(IS, rt_unlockShip, shipNumber);

            /* now do the actual transfer */
            server2(IS, rt_lockShipPair, mineNum, shipNumber);
            /* get the maximum amount of ore that may be transfered */
            ore = umin(rp[0].sh_items[it_ore], (USHORT) MAX_WORK -
                rp[1].sh_items[it_ore]);
            /* remove that amount from the miner */
            rp[0].sh_items[it_ore] -= ore;
            /* get the maximum amount of bars that may be transfered */
            bars = umin(rp[0].sh_items[it_bars], (USHORT) MAX_WORK -
                rp[1].sh_items[it_bars]);
            /* remove that amount from the miner */
            rp[0].sh_items[it_bars] -= bars;
            /* now handle the weight */
            rp[0].sh_cargo -= (ore * IS->is_world.w_weight[it_ore]);
            rp[0].sh_cargo -= (bars * IS->is_world.w_weight[it_bars]);

            /* now put the items onto the carrying ship */
            rp[1].sh_items[it_ore] += ore;
            rp[1].sh_items[it_bars] += bars;
            server2(IS, rt_unlockShipPair, mineNum, shipNumber);

            /* see if we transfered anything */
            if ((ore == 0) && (bars == 0))
            {
                user(IS, "Unable to transfer anything from that miner\n");
            }
            else
            {
                userN3(IS, "Transfered ", ore, " units of ore ");
                userN3(IS, "and ", bars, " gold bars ");
                userN3(IS, "to ship ", shipNumber, "\n");
            }
            /* put out the dirty ship numbers */
            feShDirty(IS, mineNum);
            feShDirty(IS, shipNumber);
        }
    }
}

/*
 * minerRefuel - allows a player to refuel a miner from the hosting ship's
 *          energy supply
 */

void minerRefuel(IMP)
{
    register Ship_t *rsh, *rp;
    ULONG shipNumber, mineNum;
    Ship_t saveShip;
    ULONG shEnergy, newEnergy;
    BOOL reduced;

    /* get the number of the miner */
    if (reqShip(IS, &mineNum, "Miner to refuel"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        rp = &IS->is_request.rq_u.ru_shipPair[0];
        (void) skipBlanks(IS);
        reduced = FALSE;
        server(IS, rt_readShip, mineNum);
        if (rsh->sh_owner != IS->is_player.p_number)
        {
            err(IS, "you don't own that miner");
            return;
        }
        /* make sure the ship is a miner */
        if (rsh->sh_type != st_m)
        {
            err(IS, "that ship is not a miner");
            return;
        }
        /* make sure the ship is not on a planet */
        if (rsh->sh_planet != NO_ITEM)
        {
            err(IS, "miner is on the surface of a planet");
            return;
        }
        if (rsh->sh_dragger == NO_ITEM)
        {
            err(IS, "miner is not on a ship - alert system owner!");
            return;
        }
        shipNumber = rsh->sh_dragger;
        saveShip = *rsh;
        server(IS, rt_lockShip, shipNumber);
        updateShip(IS);
        server(IS, rt_unlockShip, shipNumber);
        if (rsh->sh_energy == 0)
        {
            err(IS, "the ship the miner is on has no energy");
            return;
        }
        shEnergy = umin(rsh->sh_energy, (USHORT) MAX_WORK - saveShip.sh_energy);
        if (shEnergy == 0)
        {
            err(IS, "the miner can not contain any more energy");
            return;
        }
        if (reqPosRange(IS, &newEnergy, shEnergy, "Transfer how many units"
            " of energy to the miner"))
        {
            server2(IS, rt_lockShipPair, mineNum, shipNumber);
            shEnergy = rp[1].sh_energy;
            if (shEnergy < newEnergy)
            {
                newEnergy = shEnergy;
                reduced = TRUE;
            }
            rp[0].sh_energy += newEnergy;
            rp[1].sh_energy -= newEnergy;
            server2(IS, rt_unlockShipPair, mineNum, shipNumber);
            if (reduced)
            {
                userN3(IS, "Actions have reduced the energy transfered to ",
                    newEnergy, " units\n");
            }
            else
            {
                userN3(IS, "Transfered ", newEnergy, " units\n");
            }
        }
        /* put out the dirty ship numbers */
        feShDirty(IS, mineNum);
        feShDirty(IS, shipNumber);
    }
}

/*
 * repLevel - replace a 4-position level
 *          Returns TRUE if they entered a valid value
 */

BOOL repLevel(IMP, USHORT *oldLevel)
{
    USHORT what;

    if (*IS->is_textInPos == '\0')
    {
        user(IS, "The current level is: ");
        switch(*oldLevel)
        {
            case 0:
                user(IS, "none\n");
                break;
            case 1:
                user(IS, "low\n");
                break;
            case 2:
                user(IS, "medium\n");
                break;
            case 3:
                user(IS, "high\n");
                break;
        }
    }
    if (reqChoice(IS, &what, "none\0low\0medium\0high\0",
        "Set to what level (none/low/medium/high)"))
    {
        *oldLevel = what;
        return TRUE;
    }
    return FALSE;
}

/*
 * minerProgram - allows a player to reprogram a miner
 */

void minerProgram(IMP)
{
    register Ship_t *rsh;
    ULONG mineNum;
    USHORT what, curLevel;

    /* get the number of the miner */
    if (reqShip(IS, &mineNum, "Miner to program"))
    {
        rsh = &IS->is_request.rq_u.ru_ship;
        (void) skipBlanks(IS);
        server(IS, rt_readShip, mineNum);
        if (rsh->sh_owner != IS->is_player.p_number)
        {
            err(IS, "you don't own that miner");
            return;
        }
        /* make sure the ship is a miner */
        if (rsh->sh_type != st_m)
        {
            err(IS, "that ship is not a miner");
            return;
        }
        /* make sure the ship is not on a planet */
        if (rsh->sh_planet != NO_ITEM)
        {
            err(IS, "miner is on the surface of a planet");
            return;
        }
        if (rsh->sh_dragger == NO_ITEM)
        {
            err(IS, "miner is not on a ship - alert system owner!");
            return;
        }
        if (reqChoice(IS, &what, "ore\0messages\0gold\0",
            "Request miner (gold/messages/ore)"))
        {
            curLevel = rsh->sh_weapon[what];
            if (repLevel(IS, &curLevel))
            {
                server(IS, rt_lockShip, mineNum);
                rsh->sh_weapon[what] = curLevel;
                server(IS, rt_unlockShip, mineNum);
                /* put out the dirty ship number */
                feShDirty(IS, mineNum);
            }
        }
    }
}

/*
 * doMinerRep - prints out the actual lines for minerList
 */

void doMinerRep(IMP, ULONG minerNumber, register Ship_t *sh)
{
#ifdef DEBUG_LIB
    Ship_t saveSh;

#endif
#ifdef DEBUG_LIB
    if (IS->is_DEBUG)
    {
	saveSh = *sh;
        log3(IS, ">>> In doMinerRep", "", "");
	*sh = saveSh;
    }
#endif
    /* display the miner's number */
    userF(IS, minerNumber, 8);
    user(IS, " | ");
    if (sh->sh_dragger == NO_ITEM)
    {
        /* the miner is on a planet */
        if (sh->sh_planet == NO_ITEM)
        {
            /* should never happen */
            user(IS, "******** | ?");
        }
        else
        {
            userF(IS, sh->sh_planet, 8);
            user(IS, " | \x50");
        }
    }
    else
    {
        userF(IS, sh->sh_dragger, 8);
        user(IS, " | S");
    }
    user(IS, " | ");
    userF(IS, sh->sh_efficiency, 3);
    user(IS, " | ");
    userF(IS, sh->sh_items[it_ore], 5);
    user(IS, " | ");
    userF(IS, sh->sh_items[it_bars], 5);
    user(IS, " | ");
    userF(IS, IS->is_world.w_shipCargoLim[st_m] - sh->sh_cargo, 5);
    user(IS, " | ");
    userF(IS, sh->sh_energy, 5);
    user(IS, " | ");
    switch(sh->sh_weapon[0])
    {
        case 0:
            userC(IS, 'N');
            break;
        case 1:
            userC(IS, 'L');
            break;
        case 2:
            userC(IS, 'M');
            break;
        case 3:
            userC(IS, 'H');
            break;
        default:
            userC(IS, '?');
            break;
            
    }
    switch(sh->sh_weapon[1])
    {
        case 0:
            userC(IS, 'N');
            break;
        case 1:
            userC(IS, 'L');
            break;
        case 2:
            userC(IS, 'M');
            break;
        case 3:
            userC(IS, 'H');
            break;
        default:
            userC(IS, '?');
            break;
            
    }
    switch(sh->sh_weapon[2])
    {
        case 0:
            userC(IS, 'N');
            break;
        case 1:
            userC(IS, 'L');
            break;
        case 2:
            userC(IS, 'M');
            break;
        case 3:
            userC(IS, 'H');
            break;
        default:
            userC(IS, '?');
            break;
            
    }
    userNL(IS);
#ifdef DEBUG_LIB
    if (IS->is_DEBUG)
    {
	saveSh = *sh;
        log3(IS, "<<< Out of doMinerRep", "", "");
	*sh = saveSh;
    }
#endif
}

/*
 * minerList - displays a list of all miners owned by the player
 */

void minerList(IMP)
{
    MinerScan_t mns;

    (void) skipBlanks(IS);
#ifdef DEBUG_LIB
    if (IS->is_DEBUG)
    {
        log3(IS, ">>> In minerList()", "", "");
    }
#endif
    if (*IS->is_textInPos == '\0')
    {
        mns.mns_minerPatternType = mnp_none;
        mns.mns_cs.cs_conditionCount = 0;
    }
    else
    {
        if (!reqMiners(IS, &mns, "Miners to report on"))
        {
            return;
        }
    }
    /* display the report header */
    user(IS, "  Number | Plan/Shp | T | Eff |  Ore  |  Bars | Space | "
        "Energ | OMG\n=========+==========+===+=====+=======+=======+="
        "======+=======+====\n");
    if (scanMiners(IS, &mns, doMinerRep) == 0)
    {
        err(IS, "no miners matched");
    }
    else
    {
        userNL(IS);
    }
#ifdef DEBUG_LIB
    if (IS->is_DEBUG)
    {
        log3(IS, "<<< Out of minerList()", "", "");
    }
#endif
}

/*
 * cmd_miner - allows a player to control miners
 */

void cmd_miner(IMP)
{
    USHORT what;

    /* find out what they want to do */
    if (reqChoice(IS, &what, "list\0program\0update\0empty\0refuel\0",
        "Request miner (empty/list/program/refuel/update)"))
    {
        (void) skipBlanks(IS);
        switch (what)
        {
            case 0:
                minerList(IS);
                break;
            case 1:
                minerProgram(IS);
                break;
            case 2:
                updateAllMiners(IS);
                break;
            case 3:
                minerEmpty(IS);
                break;
            case 4:
                minerRefuel(IS);
                break;
        }
    }
}
