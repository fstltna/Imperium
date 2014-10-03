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
 * $Id: parse.c,v 1.4 2000/05/26 22:55:19 marisa Exp $
 *
 * $Log: parse.c,v $
 * Revision 1.4  2000/05/26 22:55:19  marisa
 * Combat changes
 *
 * Revision 1.3  2000/05/26 18:17:30  marisa
 * Fix bug in getPlanetOrShip
 *
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:57  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:24:33  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:24  marisag
 * patch20: Fix empty revision.
 *
 */

#include "../config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
BOGUS - Imperium can not run on this machine due to missing stdlib.h
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"

static const char rcsid[] = "$Id: parse.c,v 1.4 2000/05/26 22:55:19 marisa Exp $";

/*
 * skipBlanks - skip past blanks in the input line.
 */

char *skipBlanks(IMP)
{
    char *p;

    p = IS->is_textInPos;
    while ((*p == ' ') || (*p == '\t'))
    {
        p += sizeof(char);
    }
    IS->is_textInPos = p;
    return p;
}

/*
 * skipWord - skip up to blanks or end of string in input line.
 */

char *skipWord(IMP)
{
    char *p;

    p = IS->is_textInPos;
    while ((*p != ' ') && (*p != '\0') && (*p != '\t'))
    {
        p += sizeof(char);
    }
    IS->is_textInPos = p;
    return p;
}

/*
 * doSkipBlanks - skip blanks, always returns 'TRUE'
 */

BOOL doSkipBlanks(IMP)
{
    (void) skipBlanks(IS);
    return TRUE;
}

/*
 * getNumber - read in a number.
 */

BOOL getNumber(IMP, long *pNum)
{
    char *inputPtr;
    long n;
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
        if (isNeg)
        {
            *pNum = -n;
        }
        else
        {
            *pNum = n;
        }
        IS->is_textInPos = inputPtr;
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
 * reqNumber - get (and prompt for if needed) a number.
 */

BOOL reqNumber(IMP, long *pN, char *prompt)
{
    if (*IS->is_textInPos == '\0')
    {
        uPrompt(IS, prompt);
        if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
        {
            return FALSE;
        }
        (void) skipBlanks(IS);
        while (!getNumber(IS, pN))
        {
            err(IS, "invalid number, try again");
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
        }
        return TRUE;
    }
    return getNumber(IS, pN);
}

/*
 * getPosRange - get a positive value less than or equal to a given amount
 */

BOOL getPosRange(IMP, ULONG *pQuan, ULONG maximum)
{
    ULONG n;

    if (getNumber(IS, (long *)&n))
    {
        if (n > maximum)
        {
            userN3(IS, "*** value must be 0 - ", maximum, " ***\n");
            return FALSE;
        }
        *pQuan = n;
        return TRUE;
    }
    return FALSE;
}

/*
 * reqPosRange - get/request a positive value
 */

BOOL reqPosRange(IMP, ULONG *pQuan, ULONG maximum, char *prompt)
{
    if (*IS->is_textInPos == '\0')
    {
        do
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
        }
        while (!getPosRange(IS, pQuan, maximum));
        return TRUE;
    }
    return getPosRange(IS, pQuan, maximum);
}

/*
 * getPosRange1 - get a number within a given range, with 1 decimal digit.
 */

BOOL getPosRange1(IMP, ULONG *pNum, ULONG maximum)
{
    char *inputPtr;
    ULONG n = 0;
    BOOL bad = FALSE;

    inputPtr = IS->is_textInPos;

    if (((*inputPtr >= '0') && (*inputPtr <= '9')) || (*inputPtr == '.'))
    {
        n = 0;
        while ((*inputPtr >= '0') && (*inputPtr <= '9'))
        {
            n = (n * 10) + (*inputPtr - '0');
            inputPtr++;
        }
        if (*inputPtr == '.')
        {
            inputPtr++;
            if ((*inputPtr >= '0') && (*inputPtr <= '9'))
            {
                n = (n * 10) + (*inputPtr - '0');
                inputPtr++;
                while ((*inputPtr >= '0') && (*inputPtr <= '9'))
                {
                    inputPtr++;
                }
            }
            else
            {
                n *= 10;
            }
        }
        else
        {
            n *= 10;
        }
        if ((*inputPtr != '\0') && (*inputPtr != ' ') && (*inputPtr != '\t'))
        {
            err(IS, "invalid digit");
            bad = TRUE;
        }
    }
    else
    {
        err(IS, "invalid digit");
        bad = TRUE;
    }
    if (!bad)
    {
        if (n > maximum)
        {
            err(IS, "value too large");
            bad = TRUE;
        }
        else
        {
            *pNum = n;
        }
    }
    if (bad)
    {
        *inputPtr = '\0';
    }
    IS->is_textInPos = inputPtr;
    return !bad;
}

/*
 * reqPosRange1 - req a number within a given range, with 1 decimal digit.
 */

BOOL reqPosRange1(IMP, ULONG *pNum, ULONG maximum, char *prompt)
{
    if (*IS->is_textInPos == '\0')
    {
        do
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
        }
        while (!getPosRange1(IS, pNum, maximum));
        return TRUE;
    }
    return getPosRange1(IS, pNum, maximum);
}

/*
 * getPair - get a coordinate pair.
 */

BOOL getPair(IMP, USHORT *pA, USHORT *pB)
{
    char ch;
    ULONG tempV;

    tempV = (ULONG)*pA;
    if (getNumber(IS, (long *)&tempV))
    {
        *pA = (USHORT)tempV;
        ch = *IS->is_textInPos;
        if (ch == ':')
        {
            IS->is_textInPos += sizeof(char);
            tempV = (ULONG)*pB;
            if (getNumber(IS, (long *)&tempV))
            {
                *pB = (USHORT)tempV;
                return TRUE;
            }
            return FALSE;
        }
        if ((ch == ',') || (ch == '?') || (ch == ' ') ||
        (ch == '\t') || (ch == '/') || (ch == '\0'))
        {
            *pB = *pA;
            return TRUE;
        }
        err(IS, "missing ':' for coordinate pair");
        *IS->is_textInPos = '\0';
        return FALSE;
    }
    return FALSE;
}

/*
 * getBox - get a pair of sector designations (a rectangle).
 */

BOOL getBox(IMP, USHORT *pA, USHORT *pB, USHORT *pC, USHORT *pD)
{
    USHORT i;
    char ch;
    char *bufStart;

    if (*IS->is_textInPos == '#')
    {
        IS->is_textInPos += sizeof(char);
        ch = *IS->is_textInPos;
        if (((ch >= '0') && (ch < ('0' + REALM_MAX))) || (ch == '\0') ||
            (ch == ' ') || (ch == '\t') || (ch == '/') || (ch == '?'))
        {
            if ((ch >= '0') && (ch < ('0' + REALM_MAX)))
            {
                i = ch - '0';
                IS->is_textInPos += sizeof(char);
            }
            else
            {
                i = 0;
            }
            bufStart = (char *)calloc(1, (INPUT_LENGTH + 1) * sizeof(char));
            if (bufStart == NULL)
            {
                err(IS, "unable to allocate memory for realm copy");
                *IS->is_textInPos = '\0';
                return FALSE;
            }
            strcpy(bufStart, &IS->is_player.p_realm[i][0]);
            strncat(bufStart, IS->is_textInPos, (INPUT_LENGTH -
                strlen(bufStart)) * sizeof(char));
            strncpy(IS->is_textInPos, bufStart, INPUT_LENGTH * sizeof(char));
            free(bufStart);
        }
    }
    if (getPair(IS, pA, pB))
    {
        if ((*pA / 10) >= IS->is_world.w_rows)
        {
            *pA = ((IS->is_world.w_rows - 1) * 10) + 9;
        }
        if ((*pB / 10) >= IS->is_world.w_rows)
        {
            *pB = ((IS->is_world.w_rows - 1) * 10) + 9;
        }
        if (*IS->is_textInPos == ',')
        {
            IS->is_textInPos = IS->is_textInPos + sizeof(char);
            if (getPair(IS, pC, pD))
            {
                if ((*pC / 10) >= IS->is_world.w_columns)
                {
                    *pC = ((IS->is_world.w_columns - 1) * 10) + 9;
                }
                if ((*pD / 10) >= IS->is_world.w_columns)
                {
                    *pD = ((IS->is_world.w_columns - 1) * 10) + 9;
                }
                if (*pA > *pB)
                {
                    err(IS, "top > bottom");
                    return FALSE;
                }
                if (*pC > *pD)
                {
                    err(IS, "right > left");
                    return FALSE;
                }
                return TRUE;
            }
            return FALSE;
        }
        err(IS, "missing ',' for sectors specification");
        *IS->is_textInPos = '\0';
        return FALSE;
    }
    return FALSE;
}

/*
 * reqBox - get (prompting for if necessary) a rectangle specification.
 */

BOOL reqBox(IMP, USHORT *pA, USHORT *pB, USHORT *pC, USHORT *pD, char *prompt)
{
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        cont = TRUE;
        while (cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = getBox(IS, pA, pB, pC, pD);
        }
        return TRUE;
    }
    return getBox(IS, pA, pB, pC, pD);
}

/*
 * getSector - get a coordinate pair.
 */

BOOL getSector(IMP, USHORT *pA, USHORT *pB)
{
    ULONG tempV;

    tempV = (ULONG)*pA;
    if (getNumber(IS, (long *)&tempV))
    {
        *pA = (USHORT)tempV;
        if (*IS->is_textInPos == ',')
        {
            IS->is_textInPos += sizeof(char);
            tempV = (ULONG)*pB;
            if (getNumber(IS, (long *)&tempV))
            {
                *pB = (USHORT)tempV;
                return TRUE;
            }
            return FALSE;
        }
        err(IS, "missing ',' for sector number");
        *IS->is_textInPos = '\0';
        return FALSE;
    }
    return FALSE;
}

/*
 * reqSector - get (prompting if needed) a single sector spec.
 */

BOOL reqSector(IMP, USHORT *pA, USHORT *pB, char *prompt)
{
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        cont = TRUE;
        while (cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = getSector(IS, pA, pB);
        }
        return TRUE;
    }
    return getSector(IS, pA, pB);
}

/*
 * reqChar - get (prompting if needed) a character in the given set.
 */

BOOL reqChar(IMP, char *pChar, char *validSet,
             char *prompt, char *errMess)
{
    char *p;
    char ch;
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        uPrompt(IS, prompt);
        if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
        {
            return FALSE;
        }
        ch = *IS->is_textInPos;
        IS->is_textInPos++;
        p = validSet;
        while ((*p != ch) && (*p != '\0'))
        {
            p += sizeof(char);
        }
        if (*p == '\0')
        {
            err(IS, errMess);
            cont = TRUE;
        }
        else
        {
            cont = FALSE;
        }
        while (cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            ch = *IS->is_textInPos;
            IS->is_textInPos++;
            p = validSet;
            while ((*p != ch) && (*p != '\0'))
            {
                p += sizeof(char);
            }
            if (*p == '\0')
            {
                err(IS, errMess);
                cont = TRUE;
            }
            else
            {
                cont = FALSE;
            }
        }
        *pChar = ch;
        return ch != ' ';
    }
    ch = *IS->is_textInPos;
    IS->is_textInPos += sizeof(char);
    while ((*validSet != ch) && (*validSet != '\0'))
    {
        validSet += sizeof(char);
    }
    if (*validSet == '\0')
    {
        err(IS, errMess);
        return FALSE;
    }
    *pChar = ch;
    if ((*IS->is_textInPos != '\0') && (*IS->is_textInPos != ' ') &&
        (*IS->is_textInPos != '\t'))
    {
        err(IS, "excess characters after type on command line");
        return FALSE;
    }
    return TRUE;
}

/*
 * reqCmsgpob - get (prompting if needed) a cmsgpob type.
 */

BOOL reqCmsgpob(IMP, ItemType_t *pWhich, char *prompt)
{
    char ch;

    if (reqChar(IS, &ch, &IS->is_itemChar[0], prompt, "invalid item type"))
    {
        *pWhich = getIndex(IS, &IS->is_itemChar[0], ch) + IT_FIRST;
        return TRUE;
    }
    return FALSE;
}

/*
 * reqDesig - get (prompting if needed) a sector designation character.
 */

BOOL reqDesig(IMP, SectorType_t *pDesig, char *prompt)
{
    char ch;

    if (reqChar(IS, &ch, &IS->is_sectorChar[0], prompt,
               "invalid sector designation"))
    {
        *pDesig = getIndex(IS, &IS->is_sectorChar[0], ch) + S_FIRST;
        return TRUE;
    }
    return FALSE;
}

/*
 * reqShipType - get a ship type character.
 */

BOOL reqShipType(IMP, ShipType_t *pType, char *prompt)
{
    char ch;

    if (reqChar(IS, &ch, &IS->is_shipChar[0], prompt, "invalid ship type"))
    {
        *pType = getIndex(IS, &IS->is_shipChar[0], ch) + ST_FIRST;
        return TRUE;
    }
    return FALSE;
}

/*
 * getPlayer - get a player name or number from the input line.
 */

BOOL getPlayer(IMP, USHORT *pPlayer)
{
    USHORT n1;
    char *name, *p;
    long n;
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        return FALSE;
    }
    if ((*IS->is_textInPos >= '0') && (*IS->is_textInPos <= '9'))
    {
        if (getNumber(IS, &n))
        {
            n1 = n;
            if (n1 < IS->is_world.w_currPlayers)
            {
                server(IS, rt_readPlayer, n1);
                if ((IS->is_request.rq_u.ru_player.p_status != ps_deity) &&
                    (IS->is_request.rq_u.ru_player.p_status != ps_active) &&
                    (IS->is_player.p_status != ps_deity))
                {
                    err(IS, "that player is not active");
                    return FALSE;
                }
                *pPlayer = n1;
                return TRUE;
            }
            if ((IS->is_player.p_status == ps_deity) && (n >= 0) &&
                (n1 < IS->is_world.w_maxPlayers))
            {
                *pPlayer = n1;
                user(IS, "\n\n*** Warning - this player is still "
                    "inactive ***\n\n\n");
                return TRUE;
            }
            err(IS, "player number out of range");
            return FALSE;
        }
        return FALSE;
    }
    name = IS->is_textInPos;
    p = skipWord(IS);
    (void) skipBlanks(IS);
    *p = '\0';
    n1 = 0;
    if (n1 >= IS->is_world.w_currPlayers)
    {
        cont = FALSE;
    }
    else
    {
        server(IS, rt_readPlayer, n1);
        cont = (strcmp(&IS->is_request.rq_u.ru_player.p_name[0], name) != 0);
    }
    while (cont)
    {
        n1++;
        if (n1 >= IS->is_world.w_currPlayers)
        {
            cont = FALSE;
        }
        else
        {
            server(IS, rt_readPlayer, n1);
            cont = (strcmp(&IS->is_request.rq_u.ru_player.p_name[0], name)
                != 0);
        }
    }
    if (n1 == IS->is_world.w_currPlayers)
    {
        err(IS, "no player by that name");
        return FALSE;
    }
    if ((IS->is_request.rq_u.ru_player.p_status != ps_deity) &&
        (IS->is_request.rq_u.ru_player.p_status != ps_active) &&
        (IS->is_player.p_status != ps_deity))
    {
        err(IS, "that player is not active");
        return FALSE;
    }
    *pPlayer = n1;
    return TRUE;
}

/*
 * reqPlayer - get (promting if needed) a player name or number.
 */

BOOL reqPlayer(IMP, USHORT *pPlayer, char *prompt)
{
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        uPrompt(IS, prompt);
        if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
        {
            return FALSE;
        }
        (void) skipBlanks(IS);
        cont = !getPlayer(IS, pPlayer);
        while (cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = !getPlayer(IS, pPlayer);
        }
        return TRUE;
    }
    return getPlayer(IS, pPlayer);
}

/*
 * getRace - get a race name or number from the input line.
 *          If deadOk is TRUE, then dead races may be selected
 */

BOOL getRace(IMP, USHORT *pRace, BOOL deadOk)
{
    short n1;
    char *name, *p;
    ULONG n;
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        return FALSE;
    }
    if ((*IS->is_textInPos >= '0') && (*IS->is_textInPos <= '9'))
    {
        if (getNumber(IS, (long *)&n))
        {
            n1 = n;
            if (n1 < RACE_MAX)
            {
                if ((IS->is_world.w_race[n1].r_status == rs_dead) && !deadOk)
                {
                    err(IS, "that race is extinct");
                    return FALSE;
                }
                *pRace = n1;
                return TRUE;
            }
            err(IS, "race number out of range");
            return FALSE;
        }
        return FALSE;
    }
    name = IS->is_textInPos;
    p = skipWord(IS);
    (void) skipBlanks(IS);
    *p = '\0';
    n1 = -1;
    cont = TRUE;
    while (cont)
    {
        n1++;
        if (n1 >= RACE_MAX)
        {
            cont = FALSE;
        }
        else
        {
            cont = (strcmp(&IS->is_world.w_race[n1].r_name[0], name) != 0);
        }
    }
    if (n1 >= RACE_MAX)
    {
        err(IS, "no race by that name");
        return FALSE;
    }
    if ((IS->is_world.w_race[n1].r_status == rs_dead) && !deadOk)
    {
        err(IS, "that race is extinct");
        return FALSE;
    }
    *pRace = n1;
    return TRUE;
}

/*
 * reqRace - get (promting if needed) a race name or number.
 *          If deadOk is TRUE, then dead races may be selected
 */

BOOL reqRace(IMP, USHORT *pRace, BOOL deadOk, char *prompt)
{
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        uPrompt(IS, prompt);
        if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
        {
            return FALSE;
        }
        (void) skipBlanks(IS);
        cont = !getRace(IS, pRace, deadOk);
        while (cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = !getRace(IS, pRace, deadOk);
        }
        return TRUE;
    }
    return getRace(IS, pRace, deadOk);
}

/*
 * getChoice - see below.
 */

BOOL getChoice(IMP, USHORT *pWhat, char *choices)
{
    char *p, *q;
    USHORT result;

    p = IS->is_textInPos;
    q = skipWord(IS);
    (void) skipBlanks(IS);
    *q = '\0';
    result = lookupCommand(choices, p);
    if (result == 0)
    {
        err(IS, "unknown 'what'");
        return FALSE;
    }
    if (result == 1)
    {
        err(IS, "ambiguous 'what'");
        return FALSE;
    }
    *pWhat = result - 2;
    return TRUE;
}

/*
 * reqChoice - get and check a word from a given set of choices. Just complain
 *      if it's from the command line, else prompt for a correct one. Return
 *      'TRUE' if we get a correct choice and set the 'what' parameter to the
 *      position of the choice in choices (0 origin).
 */

BOOL reqChoice(IMP, USHORT *pWhat, char *choices, char *prompt)
{
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        uPrompt(IS, prompt);
        if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
        {
            return FALSE;
        }
        (void) skipBlanks(IS);
        cont = !getChoice(IS, pWhat, choices);
        while (cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = !getChoice(IS, pWhat, choices);
        }
        return TRUE;
    }
    return getChoice(IS, pWhat, choices);
}

/*
 * getRangedItem - get an item number from the input line, >= 0
 *      and < a passed limit.
 */

BOOL getRangedItem(IMP, ULONG *pNumber, ULONG pLimit, char *pError)
{
    ULONG n;

    if (*IS->is_textInPos == '\0')
    {
        return FALSE;
    }
    if (getNumber(IS, (long *)&n))
    {
        if (n < pLimit)
        {
            *pNumber = n;
            return TRUE;
        }
        err(IS, pError);
        return FALSE;
    }
    return FALSE;
}

/*
 * getBigItem - get a big item number from the input line.
 */

BOOL getBigItem(IMP, ULONG *pBI)
{
    return getRangedItem(IS, pBI, IS->is_world.w_bigItemNext,
        "number out of range");
}

/*
 * getPlanet - get a planet number from the input line.
 */

BOOL getPlanet(IMP, ULONG *pPlanet)
{
    return getRangedItem(IS, pPlanet, IS->is_world.w_planetNext,
        "planet number out of range");
}

/*
 * getShip - get a ship number from the input line.
 */

BOOL getShip(IMP, ULONG *pShip)
{
    return getRangedItem(IS, pShip, IS->is_world.w_shipNext,
        "ship number out of range");
}

/*
 * reqShip - get (promting if needed) a ship number.
 */

BOOL reqShip(IMP, ULONG *pShip, char *prompt)
{
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        uPrompt(IS, prompt);
        if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
        {
            return FALSE;
        }
        (void) skipBlanks(IS);
        cont = !getShip(IS, pShip);
        while (cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = !getShip(IS, pShip);
        }
        return TRUE;
    }
    return getShip(IS, pShip);
}

/*
 * reqPlanet - get (promting if needed) a planet number.
 */

BOOL reqPlanet(IMP, ULONG *pNum, char *prompt)
{
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        uPrompt(IS, prompt);
        if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
        {
            return FALSE;
        }
        (void) skipBlanks(IS);
        cont = !getPlanet(IS, pNum);
        while (cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = !getPlanet(IS, pNum);
        }
        return TRUE;
    }
    return getPlanet(IS, pNum);
}

/*
 * reqBigItem - get (promting if needed) a big item number.
 */

BOOL reqBigItem(IMP, ULONG *itNum, char *prompt)
{
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        uPrompt(IS, prompt);
        if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
        {
            return FALSE;
        }
        (void) skipBlanks(IS);
        cont = !getBigItem(IS, itNum);
        while (cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = !getBigItem(IS, itNum);
        }
        return TRUE;
    }
    return getBigItem(IS, itNum);
}

/*
 * getSectorOrShip - get a sector or ship spec.
 */

BOOL getSectorOrShip(IMP, USHORT *pA, USHORT *pB, ULONG *pS, BOOL *pIsShip)
{
    char *p;

    p = IS->is_textInPos;
    if (*p == '-')
    {
        /* has to be a sector spec */
        *pIsShip = FALSE;
        return getSector(IS, pA, pB);
    }
    while ((*p >= '0') && (*p <= '9'))
    {
        p += sizeof(char);
    }
    if (*p == ',')
    {
        *pIsShip = FALSE;
        return getSector(IS, pA, pB);
    }
    *pIsShip = TRUE;
    return getShip(IS, pS);
}

/*
 * reqSectorOrShip - get (prompting if needed) a single sector or ship spec.
 */

BOOL reqSectorOrShip(IMP, USHORT *pA, USHORT *pB, ULONG *pS,
                     BOOL *pIsShip, char *prompt)
{
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        uPrompt(IS, prompt);
        if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
        {
            return FALSE;
        }
        (void) skipBlanks(IS);
        cont = !getSectorOrShip(IS, pA, pB, pS, pIsShip);
        while (cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = !getSectorOrShip(IS, pA, pB, pS, pIsShip);
        }
        return TRUE;
    }
    return getSectorOrShip(IS, pA, pB, pS, pIsShip);
}

/*
 * getPlanetOrShip - get a planet or ship spec.
 */

BOOL getPlanetOrShip(IMP, ULONG *iPtr, BOOL *pIsShip)
{
    char *p;

    p = IS->is_textInPos;
    while ((*p >= '0') && (*p <= '9'))
    {
        p += sizeof(char);
    }
    if (*p == 'p')
    {
        *pIsShip = FALSE;
        *p = ' ';
        return getPlanet(IS, iPtr);
    }
    p += sizeof(char);
    *pIsShip = TRUE;
    return getShip(IS, iPtr);
}

/*
 * reqPlanetOrShip - get (prompting if needed) a single planet or ship spec.
 */

BOOL reqPlanetOrShip(IMP, ULONG *iPtr, BOOL *pIsShip, char *prompt)
{
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        uPrompt(IS, prompt);
        if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
        {
            return FALSE;
        }
        (void) skipBlanks(IS);
        cont = !getPlanetOrShip(IS, iPtr, pIsShip);
        while (cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = !getPlanetOrShip(IS, iPtr, pIsShip);
        }
        return TRUE;
    }
    return getPlanetOrShip(IS, iPtr, pIsShip);
}

/*
 * getShipOrFleet - get a ship number or fleet letter.
 */

BOOL getShipOrFleet(IMP, ULONG *pShipNumber, char *pFleet)
{
    char ch;

    ch = *IS->is_textInPos;
    if ((ch >= '0') && (ch <= '9'))
    {
        *pFleet = ' ';
        if (getShip(IS, pShipNumber))
        {
            ch = *IS->is_textInPos;
            if ((ch == ' ') || (ch == '\t') || (ch == '\0'))
            {
                return TRUE;
            }
            err(IS, "extraneous characters after ship number");
            return FALSE;
        }
        return FALSE;
    }
    if (((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) ||
        (ch == '*'))
    {
        *pFleet = ch;
        IS->is_textInPos += sizeof(char);
        return TRUE;
    }
    err(IS, "invalid fleet character");
    *IS->is_textInPos = '\0';
    return FALSE;
}

/*
 * reqShipOrFleet - request a ship number or fleet letter. Return a fleet
 *      letter of ' ' if a ship number is given.
 */

BOOL reqShipOrFleet(IMP, ULONG *pShipNumber, char *pFleet, char *prompt)
{
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        uPrompt(IS, prompt);
        if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
        {
            return FALSE;
        }
        (void) skipBlanks(IS);
        cont = !getShipOrFleet(IS, pShipNumber, pFleet);
        while (cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = !getShipOrFleet(IS, pShipNumber, pFleet);
        }
        return TRUE;
    }
    return getShipOrFleet(IS, pShipNumber, pFleet);
}
