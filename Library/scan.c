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
 * $Id: scan.c,v 1.2 2000/05/18 06:50:02 marisa Exp $
 *
 * $Log: scan.c,v $
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:58  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:24:34  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:25  marisag
 * patch20: Fix empty revision.
 *
 */

#include "../config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#define ScanC 1
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Scan.h"
#include "ImpPrivate.h"

static const char rcsid[] = "$Id: scan.c,v 1.2 2000/05/18 06:50:02 marisa Exp $";

/* negative codes for a unit to be compared: */

    /* these are shared by ships and planets */
#define U_EFFICIENCY        -1
#define U_OFFICERS          -2
#define U_MILITARY          -3
#define U_PLANES            -4
#define U_MISSILES          -5
#define U_ORE               -6
#define U_GOLD              -7
#define U_CIVILIANS         -8
#define U_WEAPONS           -9
#define U_BARS              -10
#define U_CLASS             -11
#define U_OWNER             -12
#define U_SCIENTISTS        -13
#define U_PLAGUED           -14
#define U_TF                -15     /* tech factor */
    /* these are only for ships */
#define U_FUEL              -16
#define U_DRAGGED           -17
#define U_PRICE             -18
#define U_CARGO             -19
#define U_ARMOUR            -20
#define U_SHIELDS           -21
#define U_AIRLEFT           -22
#define U_ENERGY            -23
#define U_ENGEFF            -24
#define U_COURSE            -25
#define U_BTF               -26     /* blaser tech fact */
#define U_CTF               -27     /* computer tech fact */
#define U_ETF               -28     /* engine tech fact */
#define U_LTF               -29     /* life supp TF     */
#define U_STF               -30     /* sensor TF        */
#define U_TTF               -31     /* telep TF         */
#define U_PTF               -32     /* photon TF        */
    /* these are only for planets */
#define U_MOBILITY          -33
#define U_BTU               -34
#define U_MINERALS          -35
#define U_PRODUCTION        -36     /* this the TOTAL production */
#define U_CHECKPOINT        -37
#define U_CIVPROD           -38
#define U_MILPROD           -39
#define U_TECHPROD          -40
#define U_RESPROD           -41
#define U_EDUPROD           -42
#define U_OCSPROD           -43
#define U_OREPROD           -44
#define U_GOLDPROD          -45
#define U_AIRPROD           -46
#define U_FUELPROD          -47
#define U_WEAPPROD          -48
#define U_ENGPROD           -49
#define U_HULLPROD          -50
#define U_MISSPROD          -51
#define U_PLANPROD          -52
#define U_ELECPROD          -53
#define U_CASHPROD          -54
#define U_CIVPER            -55
#define U_MILPER            -56
#define U_TECHPER           -57
#define U_RESPER            -58
#define U_EDUPER            -59
#define U_OCSPER            -60
#define U_OREPER            -61
#define U_GOLDPER           -62
#define U_AIRPER            -63
#define U_FUELPER           -64
#define U_WEAPPER           -65
#define U_ENGPER            -66
#define U_HULLPER           -67
#define U_MISSPER           -68
#define U_PLANPER           -69
#define U_ELECPER           -70
#define U_CASHPER           -71
#define U_POLUTION          -72
#define U_GAS               -73
#define U_WATER             -74
#define U_NUMBIG            -75
#define U_OWNRACE           -76

    /* these are used by planets and sectors */
#define U_SHIP_COUNT        -77
    /* these are useable only for sectors */
#define U_PLANET_COUNT      -78
    /* these are useable only for miners */
#define U_CARRIED           -79
#define U_MESSAGE           -80
#define U_ACTIVITY          -81
    /* these are used only for miners and ships */
#define U_FREE              -82

static const char *UNITS =
    "efficiency\0"
    "officers\0"
    "military\0"
    "planes\0"
    "missiles\0"
    "ore\0"
    "gold\0"
    "civilians\0"
    "weapons\0"
    "bars\0"
    "class\0"
    "owner\0"
    "scientists\0"
    "plagued\0"
    "tf\0"

    "fuel\0"
    "dragged\0"
    "price\0"
    "cargo\0"
    "armor\0"
    "shields\0"
    "airleft\0"
    "energy\0"
    "engeff\0"
    "course\0"
    "btf\0"
    "ctf\0"
    "etf\0"
    "ltf\0"
    "stf\0"
    "ttf\0"
    "ptf\0"

    "mobility\0"
    "btu\0"
    "minerals\0"
    "production\0"
    "checkpoint\0"
    "prodciv\0"
    "prodmil\0"
    "prodtech\0"
    "prodres\0"
    "prodedu\0"
    "prodocs\0"
    "prodore\0"
    "prodgold\0"
    "prodair\0"
    "prodfuel\0"
    "prodweap\0"
    "prodeng\0"
    "prodhull\0"
    "prodmiss\0"
    "prodplan\0"
    "prodelec\0"
    "prodcash\0"
    "perciv\0"
    "permil\0"
    "pertech\0"
    "perres\0"
    "peredu\0"
    "perocs\0"
    "perore\0"
    "pergold\0"
    "perair\0"
    "perfuel\0"
    "perweap\0"
    "pereng\0"
    "perhull\0"
    "permiss\0"
    "perplan\0"
    "perelec\0"
    "percash\0"
    "polution\0"
    "gas\0"
    "water\0"
    "numbig\0"
    "ownrace\0"

    "scount\0"

    "pcount\0"

    "carried\0"
    "message\0"
    "activity\0"

    "free\0";

#define ct_ship     0
#define ct_planet   1
#define ct_sector   2
#define ct_miner    3
typedef UBYTE ConditionType_t;

/*
 * member -
 *      return true if character is in string
 */

BOOL member(register char *set, register char element)
{
    while ((*set != '\0') && (*set != element))
    {
        set += sizeof(char);
    }
    return (*set == element);
}

/*
 * getValue -
 *      get a value for a condition.
 */

BOOL getValue(IMP, register ConditionSet_t *cs, BOOL isRight,
    ConditionType_t ct)
{
    register char *inputPtr;
    char *p;
    USHORT res;
    register long valu = 0;
    char ch;
    register BOOL ok = TRUE;

    /* initialize variables */
    inputPtr = IS->is_textInPos;

    /* check for a number in the input stream */
    if ((*inputPtr >= '0') && (*inputPtr <= '9'))
    {
        /* a simple numeric value */
        while ((*inputPtr >= '0') && (*inputPtr <= '9'))
        {
            valu = (valu * 10) + (*inputPtr - '0');
            inputPtr += sizeof(char);
        }
    }
    else
    {
	p = inputPtr;
        /* Not a number, so look for designation character */
        if ((*(inputPtr + 1) < 'a') || (*(inputPtr + 1) > 'z'))
        {
            /* assume its a one-character designation character */
            ch = *inputPtr;
            inputPtr += sizeof(char);
            switch(ct)
            {
                case ct_ship:
                    if ((ch == 'm') || (!member(&IS->is_shipChar[0], ch)))
                    {
                        err(IS, "invalid ship designation in condition");
                        ok = FALSE;
                    }
                    else
                    {
                        valu = getIndex(IS, &IS->is_shipChar[0], ch);
                    }
                    break;

                case ct_miner:
                    if (ch != 'm')
                    {
                        err(IS, "invalid ship designation in condition");
                        ok = FALSE;
                    }
                    else
                    {
                        valu = getIndex(IS, &IS->is_shipChar[0], ch);
                    }
                    break;

                case ct_planet:
                    err(IS, "invalid character in planet list");
                    ok = FALSE;
                    break;

                default:
                    if (!member(&IS->is_sectorChar[0], ch))
                    {
                        err(IS, "invalid sector designation in condition");
                        ok = FALSE;
                    }
                    else
                    {
                        valu = getIndex(IS, &IS->is_sectorChar[0], ch);
                    }
                    break;
            }
        }
        else
        {
            /* look for first non-letter character */
            while ((*inputPtr >= 'a') && (*inputPtr <= 'z'))
            {
                inputPtr += sizeof(char);
            }
        }
        ch = *inputPtr;
        *inputPtr = '\0';
        /* try to find item in allowed list */
        res = lookupCommand(UNITS, p);
        /* did we find it? */
        if (res == 0)
        {
            err(IS, "invalid unit in condition");
            ok = FALSE;
        }
        else
        {
            if (res == 1)
            {
                err(IS, "ambiguous unit in condition");
                ok = FALSE;
            }
            else
            {
                *inputPtr = ch;
                valu = 1 - (long)res;
            }
        }
    }
    if (ok)
    {
        if (isRight)
        {
            cs->cs_condition[cs->cs_conditionCount].c_right = valu;
        }
        else
        {
            cs->cs_condition[cs->cs_conditionCount].c_left = valu;
        }
    }
    IS->is_textInPos = inputPtr;
    return ok;
}

/*
 * getOperator -
 *      get a valid condition operator.
 */

BOOL getOperator(IMP, ConditionSet_t *cs)
{
    register char ch;

    ch = *IS->is_textInPos;
    if ((ch == '<') || (ch == '>') || (ch == '=') || (ch == '#'))
    {
        cs->cs_condition[cs->cs_conditionCount].c_operator = ch;
        IS->is_textInPos += sizeof(char);
        return TRUE;
    }
    err(IS, "invalid operator in condition");
    return FALSE;
}

/*
 * parseConditions -
 *      parse a set of conditions from the command line, and store them into
 *      the condition array.
 *      We return 'TRUE' if all went well.
 */

BOOL parseConditions(IMP, register ConditionSet_t *cs, ConditionType_t type)
{
    register char ch;
    BOOL hadError;

    ch = *IS->is_textInPos;
    if (ch == '/')
    {
        /* special case, allow '/x' to mean '?des=x' */
        IS->is_textInPos += sizeof(char);
        cs->cs_conditionCount = 0;
        cs->cs_condition[0].c_left = U_CLASS;
        cs->cs_condition[0].c_operator = '=';
        if (getValue(IS, cs, TRUE, type))
        {
            if (cs->cs_condition[0].c_right < 0)
            {
                err(IS, "must use designation letter with '/'");
                return FALSE;
            }
            cs->cs_conditionCount = 1;
            return TRUE;
        }
        return FALSE;
    }
    cs->cs_conditionCount = 0;
    if (ch == '?')
    {
        IS->is_textInPos += sizeof(char);
        hadError = FALSE;
        while (!hadError)
        {
            ch = *IS->is_textInPos;
            if ((ch == '\0') || (ch == ' ') || (ch == '\t'))
            {
                /* no more conditions */
                return (!hadError);
            }
            if (cs->cs_conditionCount == MAX_CONDITIONS)
            {
                err(IS, "too many conditions");
                hadError = TRUE;
            }
            else
            {
                if (getValue(IS, cs, FALSE, type) &&
                    getOperator(IS, cs) &&
                    getValue(IS, cs, TRUE, type))
                {
                    if (((cs->cs_condition[cs->cs_conditionCount].c_left
                        == U_CLASS) &&
                        (cs->cs_condition[cs->cs_conditionCount].c_right
                        < 0)) ||
                        ((cs->cs_condition[cs->cs_conditionCount].c_right
                        == U_CLASS) &&
                        (cs->cs_condition[cs->cs_conditionCount].c_left
                        < 0)))
                    {
                        err(IS, "invalid use of designation character");
                        hadError = TRUE;
                    }
                    else
                    {
                        if ((cs->cs_condition[cs->cs_conditionCount].c_left>= 0)
                            && (cs->cs_condition[cs->cs_conditionCount].c_right
                             >= 0))
                        {
                            err(IS, "invalid condition - no field");
                            hadError = TRUE;
                        }
                        else
                        {
                            if ((type == ct_sector) &&
                                (((cs->cs_condition[cs->cs_conditionCount].c_left
                                != U_CLASS) &&
                                (cs->cs_condition[cs->cs_conditionCount].c_left
                                != U_PLANET_COUNT) &&
                                (cs->cs_condition[cs->cs_conditionCount].c_left
                                != U_SHIP_COUNT)) || (IS->is_player.p_status !=
                                ps_deity)))
                            {
                                err(IS, "invalid condition for a sector");
                                hadError = TRUE;
                            }
                            else
                            {
                                cs->cs_conditionCount++;
                                ch = *IS->is_textInPos;
                                if (ch == '&')
                                {
                                    IS->is_textInPos += sizeof(char);
                                }
                                else
                                {
                                    if ((ch != '\0') && (ch != ' ') &&
                                        (ch != '\t'))
                                    {
                                        err(IS, "syntax error in conditions");
                                        hadError = TRUE;
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    hadError = TRUE;
                }
            }
        }
        return (!hadError);
    }
    return TRUE;
}

/*
 * handleShipRealm - does the actual parsing of a realm based on what is
 *          correct for ships. At the function call time we know
 *          IS->is_textInPos is pointing to a "#"
 */

BOOL handleShipRealm(IMP, register ShipScan_t *shs)
{
    register char *p;
    register ULONG planetNumber;
    register USHORT realm;
    register char ch;
    register ULONG wkNum;
    BOOL hadError, done, doRange;

    /* set up the default realm */
    realm = 0;
    /* move past the "#" */
    IS->is_textInPos += sizeof(char);
    /* store the next character */
    ch = *IS->is_textInPos;
    if (((ch >= '0') && (ch < ('0' + REALM_MAX))) || (ch == '\0') ||
        (ch == ' ') || (ch == '\t') || (ch == '/') || (ch == '?'))
    {
        /* if the next character is a number, store the number in realm */
        if ((ch >= '0') && (ch < ('0' + REALM_MAX)))
        {
            realm = ch - '0';
            /* move to the next character */
            IS->is_textInPos += sizeof(char);
        }
    }
    /* determine if this is a box entry or a list item */
    if (strpbrk(&IS->is_player.p_realm[realm][0], ",:") != NULL)
    {
        /* only box type entries should have a "," or ":" character */
        shs->shs_shipPatternType = shp_box;
        /* getBox() handles the "#" on it's own, so we have to back up */
        IS->is_textInPos -= sizeof(char);
        /* if it still isn't a "#", back up one last time */
        if (*IS->is_textInPos != '#')
        {
            IS->is_textInPos -= sizeof(char);
        }
        /* now let getBox() do it's thing, and return whatever it gives us */
        return (getBox(IS, &shs->shs_cs.cs_boxTop, &shs->shs_cs.cs_boxBottom,
           &shs->shs_cs.cs_boxLeft, &shs->shs_cs.cs_boxRight));
    }
    /* we have a list of planet numbers separated by '/'s */
    p = &IS->is_player.p_realm[realm][0];
    shs->shs_shipPatternType = shp_plList;
    /* note that shipCount is planets in this case */
    shs->shs_shipCount = 0;
    hadError = FALSE;
    done = FALSE;
    doRange = FALSE;
    while (!done && !hadError)
    {
        if (shs->shs_shipCount == MAX_SHIPS)
        {
            err(IS, "too many planets listed");
            hadError = TRUE;
        }
        else
        {
            if ((*p < '0') || (*p > '9'))
            {
                err(IS, "invalid planet number");
                hadError = TRUE;
            }
            else
            {
                planetNumber = 0;
                while ((*p >= '0') && (*p <= '9'))
                {
                    planetNumber = planetNumber * 10 + (*p - '0');
                    p += sizeof(char);
                }
                if (planetNumber >= IS->is_world.w_planetNext)
                {
                    err(IS, "planet number too big");
                    hadError = TRUE;
                }
                else
                {
                    if (doRange && (shs->shs_shipCount > 0))
                    {
                        wkNum = shs->shs_shipList[shs->shs_shipCount - 1] + 1;
                        while ((wkNum <= planetNumber) &&
                            (shs->shs_shipCount < MAX_SHIPS - 1))
                        {
                            shs->shs_shipList[shs->shs_shipCount] = wkNum;
                            shs->shs_shipCount++;
                            wkNum++;
                        }
                        if (shs->shs_shipCount == MAX_SHIPS)
                        {
                            err(IS, "too many planets listed");
                            hadError = TRUE;
                        }
                    }
                    else
                    {
                        shs->shs_shipList[shs->shs_shipCount] = planetNumber;
                        shs->shs_shipCount++;
                    }
                    doRange = FALSE;
                }
                if (*p == '/')
                {
                    p += sizeof(char);
                }
                else
                {
                    if (*p == '-')
                    {
                        doRange = TRUE;
                        p += sizeof(char);
                    }
                    else if ((*p == '\0') || (*p == '?') || (*p == ' ') ||
                        (*p == '\t'))
                    {
                        done = TRUE;
                    }
                    else
                    {
                        err(IS, "invalid character in planet list");
                        hadError = TRUE;
                    }
                }
            }
        }
    }
    return !hadError;
}

/*
 * getShips -
 *      get a ships specifier
 */

BOOL getShips(IMP, register ShipScan_t *shs)
{
    register char *p;
    register ULONG shipNumber;
    register char ch;
    register ULONG wkNum;
    BOOL hadError, done, doRange;

    ch = *IS->is_textInPos;
    if ((ch == '?') || (ch == '/'))
    {
        shs->shs_shipPatternType = shp_none;
        return parseConditions(IS, &shs->shs_cs, ct_ship);
    }
    if ((ch == '*') || ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') &&
        (ch <= 'Z')))
    {
        shs->shs_shipPatternType = shp_fleet;
        shs->shs_shipFleet = ch;
        IS->is_textInPos += sizeof(char);
        return parseConditions(IS, &shs->shs_cs, ct_ship);
    }
    if (ch == '#')
    {
        if (handleShipRealm(IS, shs))
        {
            return parseConditions(IS, &shs->shs_cs, ct_ship);
        }
        return FALSE;
    }
    if (ch == '-')
    {
        shs->shs_shipPatternType = shp_box;
        if (getBox(IS, &shs->shs_cs.cs_boxTop, &shs->shs_cs.cs_boxBottom,
               &shs->shs_cs.cs_boxLeft, &shs->shs_cs.cs_boxRight))
        {
            return parseConditions(IS, &shs->shs_cs, ct_ship);
        }
        return FALSE;
    }
    if ((ch < '0') || (ch > '9'))
    {
        err(IS, "invalid ships specification");
        return FALSE;
    }
    p = IS->is_textInPos;
    while ((*p >= '0') && (*p <= '9'))
    {
        p += sizeof(char);
    }
    if ((*p == ',') || (*p == ':'))
    {
        shs->shs_shipPatternType = shp_box;
        return (getBox(IS, &shs->shs_cs.cs_boxTop, &shs->shs_cs.cs_boxBottom,
               &shs->shs_cs.cs_boxLeft, &shs->shs_cs.cs_boxRight) &&
            parseConditions(IS, &shs->shs_cs, ct_ship));
    }
    /* we have a list of ship numbers separated by '/'s */
    p = IS->is_textInPos;
    shs->shs_shipPatternType = shp_list;
    shs->shs_shipCount = 0;
    hadError = FALSE;
    done = FALSE;
    doRange = FALSE;
    while (!done && !hadError)
    {
        if (shs->shs_shipCount == MAX_SHIPS)
        {
            err(IS, "too many ships listed");
            hadError = TRUE;
        }
        else
        {
            if ((*p < '0') || (*p > '9'))
            {
                err(IS, "invalid ship number");
                hadError = TRUE;
            }
            else
            {
                shipNumber = 0;
                while ((*p >= '0') && (*p <= '9'))
                {
                    shipNumber = shipNumber * 10 + (*p - '0');
                    p += sizeof(char);
                }
                if (shipNumber >= IS->is_world.w_shipNext)
                {
                    err(IS, "ship number too big");
                    hadError = TRUE;
                }
                else
                {
                    if (doRange && (shs->shs_shipCount > 0))
                    {
                        wkNum = shs->shs_shipList[shs->shs_shipCount - 1] + 1;
                        while ((wkNum <= shipNumber) &&
                            (shs->shs_shipCount < MAX_SHIPS - 1))
                        {
                            shs->shs_shipList[shs->shs_shipCount] = wkNum;
                            shs->shs_shipCount++;
                            wkNum++;
                        }
                        if (shs->shs_shipCount == MAX_SHIPS)
                        {
                            err(IS, "too many ships listed");
                            hadError = TRUE;
                        }
                    }
                    else
                    {
                        shs->shs_shipList[shs->shs_shipCount] = shipNumber;
                        shs->shs_shipCount++;
                    }
                    doRange = FALSE;
                }
                if (*p == '/')
                {
                    p += sizeof(char);
                }
                else
                {
                    if (*p == '-')
                    {
                        doRange = TRUE;
                        p += sizeof(char);
                    }
                    else if ((*p == '\0') || (*p == '?') || (*p == ' ') ||
                        (*p == '\t'))
                    {
                        done = TRUE;
                    }
                    else
                    {
                        err(IS, "invalid character in ship list");
                        hadError = TRUE;
                    }
                }
            }
        }
    }
    IS->is_textInPos = p;
    if (!hadError)
    {
        return parseConditions(IS, &shs->shs_cs, ct_ship);
    }
    return FALSE;
}

/*
 * reqShips - request/get a ships list
 */

BOOL reqShips(IMP, ShipScan_t *shs, char *prompt)
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
        while(!getShips(IS, shs));
        return TRUE;
    }
    return getShips(IS, shs);
}

/*
 * getShipCondVal - return the long giving the appropriate value for a ship cond
 */

long getShipCondVal(IMP, register ShipScan_t *shs, long valu)
{
    if (valu >= 0)
    {
        return valu;
    }
    switch (valu)
    {
        case U_CIVILIANS:
            return (long)shs->shs_currentShip.sh_items[it_civilians];
        case U_SCIENTISTS:
            return (long)shs->shs_currentShip.sh_items[it_scientists];
        case U_MILITARY:
            return (long)shs->shs_currentShip.sh_items[it_military];
        case U_OFFICERS:
            return (long)shs->shs_currentShip.sh_items[it_officers];
        case U_MISSILES:
            return (long)shs->shs_currentShip.sh_items[it_missiles];
        case U_PLANES:
            return (long)shs->shs_currentShip.sh_items[it_planes];
        case U_ORE:
            return (long)shs->shs_currentShip.sh_items[it_ore];
        case U_BARS:
            return (long)shs->shs_currentShip.sh_items[it_bars];
        case U_EFFICIENCY:
            return (long)shs->shs_currentShip.sh_efficiency;
        case U_FUEL:
            return (long)shs->shs_currentShip.sh_fuelLeft;
        case U_OWNER:
            return (long)shs->shs_currentShip.sh_owner;
        case U_TF:
            return (long)shs->shs_currentShip.sh_hullTF;
        case U_DRAGGED:
            if (shs->shs_currentShip.sh_dragger != NO_ITEM)
            {
                return 1;
            }
            return 0;
        case U_PRICE:
            return (long)shs->shs_currentShip.sh_price;
        case U_CARGO:
            return (long)shs->shs_currentShip.sh_cargo;
        case U_ARMOUR:
            return (long)shs->shs_currentShip.sh_armourLeft;
        case U_SHIELDS:
            return (long)shs->shs_currentShip.sh_shields;
        case U_AIRLEFT:
            return (long)shs->shs_currentShip.sh_airLeft;
        case U_ENERGY:
            return (long)shs->shs_currentShip.sh_energy;
        case U_ENGEFF:
            return (long)shs->shs_currentShip.sh_engEff;
        case U_COURSE:
            if (shs->shs_currentShip.sh_course[0] != '\0')
            {
                return 1;
            }
            return 0;
        case U_BTF:
            return (long)getShipTechFactor(IS, &shs->shs_currentShip,
                bp_blaser);
        case U_CTF:
            return (long)getShipTechFactor(IS, &shs->shs_currentShip,
                bp_computer);
        case U_ETF:
            return (long)getShipTechFactor(IS, &shs->shs_currentShip,
                bp_engines);
        case U_LTF:
            return (long)getShipTechFactor(IS, &shs->shs_currentShip,
                bp_lifeSupp);
        case U_STF:
            return (long)getShipTechFactor(IS, &shs->shs_currentShip,
                bp_sensors);
        case U_TTF:
            return (long)getShipTechFactor(IS, &shs->shs_currentShip,
                bp_teleport);
        case U_PTF:
            return (long)getShipTechFactor(IS, &shs->shs_currentShip,
                bp_photon);
        case U_FREE:
            return (long)
                (IS->is_world.w_shipCargoLim[shs->shs_currentShip.sh_type] -
                shs->shs_currentShip.sh_cargo);
        default:
            err(IS, "unknown ship unit");
            return(0);
    }
}

/*
 * checkShipCond -
 *      see if the setup conditions match the current ship
 */

BOOL checkShipCond(IMP, ShipScan_t *shs)
{
    register ConditionSet_t *cs;
    register ULONG condition;
    register int left, right;
    BOOL matching;

    cs = &shs->shs_cs;
    matching = TRUE;
    condition = 0;
    while ((condition != cs->cs_conditionCount) && matching)
    {
        left = getShipCondVal(IS, shs, cs->cs_condition[condition].c_left);
        right = getShipCondVal(IS, shs, cs->cs_condition[condition].c_right);
        switch(cs->cs_condition[condition].c_operator)
        {
            case '<':
                matching = left < right;
                break;
            case '>':
                matching = left > right;
                break;
            case '=':
                matching = left == right;
                break;
            case '#':
                matching = left != right;
                break;
        }
        condition++;
    }
    return matching;
}

/*
 * scanShips -
 *      The actual ship scanning routine. It calls its argument proc
 *      for each ship that meets the set up specs and conditions.
 */

ULONG scanShips(IMP, register ShipScan_t *shs,
        void (*scanner)(IMP, ULONG shipNumber, Ship_t *sh))
{
    Fleet_t fleet;
    register ULONG shipNumber;
    register ULONG i, count;
    USHORT r, c;
    BOOL doIt = FALSE;

    count = 0;
    if (IS->is_world.w_shipNext)
    {
        if (shs->shs_shipPatternType == shp_list)
        {
            i = 0;
            while ((i != shs->shs_shipCount) && !clGotCtrlC(IS))
            {
                shipNumber = shs->shs_shipList[i];
                server(IS, rt_readShip, shipNumber);
                shs->shs_currentShip = IS->is_request.rq_u.ru_ship;
                if (((shs->shs_currentShip.sh_owner == IS->is_player.p_number) ||
                    (IS->is_player.p_status == ps_deity))  &&
                    (IS->is_request.rq_u.ru_ship.sh_type != st_m))
                {
                    if (checkShipCond(IS, shs))
                    {
                        count++;
                        scanner(IS, shipNumber, &shs->shs_currentShip);
                    }
                }
                i++;
            }
        }
        else if ((shs->shs_shipPatternType == shp_fleet) && (shs->shs_shipFleet
            != '*'))
        {
            i = IS->is_player.p_fleets[fleetPos(shs->shs_shipFleet)];
            if (i == NO_FLEET)
            {
                err(IS, "you have no such fleet");
            }
            else
            {
                server(IS, rt_readFleet, i);
                fleet = IS->is_request.rq_u.ru_fleet;
                if (fleet.f_count == 0)
                {
                    err(IS, "fleet has no ships");
                }
                else
                {
                    i = 0;
                    while ((i != fleet.f_count) && !clGotCtrlC(IS))
                    {
                        server(IS, rt_readShip, fleet.f_ship[i]);
                        shs->shs_currentShip = IS->is_request.rq_u.ru_ship;
                        if (checkShipCond(IS, shs))
                        {
                            count++;
                            scanner(IS, fleet.f_ship[i],&shs->shs_currentShip);
                        }
                        i++;
                    }
                }
            }
        }
        else
        {
            shipNumber = 0;
            while ((shipNumber != IS->is_world.w_shipNext) &&
                !clGotCtrlC(IS))
            {
                server(IS, rt_readShip, shipNumber);
                if (((IS->is_request.rq_u.ru_ship.sh_owner ==
                    IS->is_player.p_number) || (IS->is_player.p_status ==
                    ps_deity)) && (IS->is_request.rq_u.ru_ship.sh_type !=
                    st_m))
                {
                    shs->shs_currentShip = IS->is_request.rq_u.ru_ship;
                    switch(shs->shs_shipPatternType)
                    {
                        case shp_none:
                            doIt = TRUE;
                            break;
                        case shp_box:
                            r = shs->shs_currentShip.sh_row;
                            c = shs->shs_currentShip.sh_col;
                            doIt = ((r >= shs->shs_cs.cs_boxTop) &&
                                (r <= shs->shs_cs.cs_boxBottom) &&
                                (c >= shs->shs_cs.cs_boxLeft) &&
                                (c <= shs->shs_cs.cs_boxRight));
                            break;
                        case shp_fleet:       /* '*' fleet */
                            doIt = (shs->shs_currentShip.sh_fleet ==
                                shs->shs_shipFleet);
                            break;
                        case shp_plList:
                            i = 0;
                            doIt = FALSE;
                            while ((i != shs->shs_shipCount) && !doIt)
                            {
                                if (shs->shs_currentShip.sh_planet ==
                                    shs->shs_shipList[i])
                                {
                                    doIt = TRUE;
                                }
                                i++;
                            }
                            break;
                    }
                    if (doIt)
                    {
                        if (checkShipCond(IS, shs))
                        {
                            count++;
                            scanner(IS, shipNumber, &shs->shs_currentShip);
                        }
                    }
                }
                shipNumber++;
            }
        }
    }
    return count;
}

/*
 * handlePlanetRealm - does the actual parsing of a realm based on what is
 *          correct for planets. At the function call time we know
 *          IS->is_textInPos is pointing to a "#"
 */

BOOL handlePlanetRealm(IMP, register PlanetScan_t *ps)
{
    register char *p;
    register ULONG planetNumber;
    register USHORT realm;
    register char ch;
    register ULONG wkNum;
    BOOL hadError, done, doRange;

    /* set up the default realm */
    realm = 0;
    /* move past the "#" */
    IS->is_textInPos += sizeof(char);
    /* store the next character */
    ch = *IS->is_textInPos;
    if (((ch >= '0') && (ch < ('0' + REALM_MAX))) || (ch == '\0') ||
        (ch == ' ') || (ch == '\t') || (ch == '/') || (ch == '?'))
    {
        /* if the next character is a number, store the number in realm */
        if ((ch >= '0') && (ch < ('0' + REALM_MAX)))
        {
            realm = ch - '0';
            /* move to the next character */
            IS->is_textInPos += sizeof(char);
        }
    }
    /* determine if this is a box entry or a list item */
    if (strpbrk(&IS->is_player.p_realm[realm][0], ",:") != NULL)
    {
        /* only box type entries should have a "," or ":" character */
        ps->ps_planetPatternType = pp_box;
        /* getBox() handles the "#" on it's own, so we have to back up */
        IS->is_textInPos -= sizeof(char);
        /* if it still isn't a "#", back up one last time */
        if (*IS->is_textInPos != '#')
        {
            IS->is_textInPos -= sizeof(char);
        }
        /* now let getBox() do it's thing, and return whatever it gives us */
        return (getBox(IS, &ps->ps_cs.cs_boxTop, &ps->ps_cs.cs_boxBottom,
           &ps->ps_cs.cs_boxLeft, &ps->ps_cs.cs_boxRight));
    }
    /* we have a list of planet numbers separated by '/'s */
    p = &IS->is_player.p_realm[realm][0];
    ps->ps_planetPatternType = pp_list;
    ps->ps_planetCount = 0;
    hadError = FALSE;
    done = FALSE;
    doRange = FALSE;
    while (!done && !hadError)
    {
        if (ps->ps_planetCount == MAX_PLANETS)
        {
            err(IS, "too many planets listed");
            hadError = TRUE;
        }
        else
        {
            if ((*p < '0') || (*p > '9'))
            {
                err(IS, "invalid planet number");
                hadError = TRUE;
            }
            else
            {
                planetNumber = 0;
                while ((*p >= '0') && (*p <= '9'))
                {
                    planetNumber = planetNumber * 10 + (*p - '0');
                    p += sizeof(char);
                }
                if (planetNumber >= IS->is_world.w_planetNext)
                {
                    err(IS, "planet number too big");
                    hadError = TRUE;
                }
                else
                {
                    if (doRange && (ps->ps_planetCount > 0))
                    {
                        wkNum = ps->ps_planetList[ps->ps_planetCount - 1] + 1;
                        while ((wkNum <= planetNumber) &&
                            (ps->ps_planetCount < MAX_PLANETS - 1))
                        {
                            ps->ps_planetList[ps->ps_planetCount] = wkNum;
                            ps->ps_planetCount++;
                            wkNum++;
                        }
                        if (ps->ps_planetCount == MAX_PLANETS)
                        {
                            err(IS, "too many planets listed");
                            hadError = TRUE;
                        }
                    }
                    else
                    {
                        ps->ps_planetList[ps->ps_planetCount] = planetNumber;
                        ps->ps_planetCount++;
                    }
                    doRange = FALSE;
                }
                if (*p == '/')
                {
                    p += sizeof(char);
                }
                else
                {
                    if (*p == '-')
                    {
                        doRange = TRUE;
                        p += sizeof(char);
                    }
                    else if ((*p == '\0') || (*p == '?') || (*p == ' ') ||
                        (*p == '\t'))
                    {
                        done = TRUE;
                    }
                    else
                    {
                        err(IS, "invalid character in planet list");
                        hadError = TRUE;
                    }
                }
            }
        }
    }
    return !hadError;
}

/*
 * getPlanets -
 *      get a planets specifier
 */

BOOL getPlanets(IMP, register PlanetScan_t *ps)
{
    register char *p;
    register ULONG planetNumber;
    register char ch;
    register ULONG wkNum;
    BOOL hadError, done, doRange;

    ch = *IS->is_textInPos;
    if ((ch == '?') || (ch == '/'))
    {
        ps->ps_planetPatternType = pp_none;
        return parseConditions(IS, &ps->ps_cs, ct_planet);
    }
    if (ch == '#')
    {
        if (handlePlanetRealm(IS, ps))
        {
            return parseConditions(IS, &ps->ps_cs, ct_planet);
        }
        return FALSE;
    }
    if (ch == '-')
    {
        ps->ps_planetPatternType = pp_box;
        if (getBox(IS, &ps->ps_cs.cs_boxTop, &ps->ps_cs.cs_boxBottom,
               &ps->ps_cs.cs_boxLeft, &ps->ps_cs.cs_boxRight))
        {
            return parseConditions(IS, &ps->ps_cs, ct_planet);
        }
        return FALSE;
    }
    if ((ch < '0') || (ch > '9'))
    {
        err(IS, "invalid planets specification");
        return FALSE;
    }
    p = IS->is_textInPos;
    while ((*p >= '0') && (*p <= '9'))
    {
        p += sizeof(char);
    }
    if ((*p == ',') || (*p == ':'))
    {
        ps->ps_planetPatternType = pp_box;
        return (getBox(IS, &ps->ps_cs.cs_boxTop, &ps->ps_cs.cs_boxBottom,
               &ps->ps_cs.cs_boxLeft, &ps->ps_cs.cs_boxRight) &&
            parseConditions(IS, &ps->ps_cs, ct_planet));
    }
    /* we have a list of planet numbers separated by '/'s */
    p = IS->is_textInPos;
    ps->ps_planetPatternType = pp_list;
    ps->ps_planetCount = 0;
    hadError = FALSE;
    done = FALSE;
    doRange = FALSE;
    while (!done && !hadError)
    {
        if (ps->ps_planetCount == MAX_PLANETS)
        {
            err(IS, "too many planets listed");
            hadError = TRUE;
        }
        else
        {
            if ((*p < '0') || (*p > '9'))
            {
                err(IS, "invalid planet number");
                hadError = TRUE;
            }
            else
            {
                planetNumber = 0;
                while ((*p >= '0') && (*p <= '9'))
                {
                    planetNumber = planetNumber * 10 + (*p - '0');
                    p += sizeof(char);
                }
                if (planetNumber >= IS->is_world.w_planetNext)
                {
                    err(IS, "planet number too big");
                    hadError = TRUE;
                }
                else
                {
                    if (doRange && (ps->ps_planetCount > 0))
                    {
                        wkNum = ps->ps_planetList[ps->ps_planetCount - 1] + 1;
                        while ((wkNum <= planetNumber) &&
                            (ps->ps_planetCount < MAX_PLANETS - 1))
                        {
                            ps->ps_planetList[ps->ps_planetCount] = wkNum;
                            ps->ps_planetCount++;
                            wkNum++;
                        }
                        if (ps->ps_planetCount == MAX_PLANETS)
                        {
                            err(IS, "too many planets listed");
                            hadError = TRUE;
                        }
                    }
                    else
                    {
                        ps->ps_planetList[ps->ps_planetCount] = planetNumber;
                        ps->ps_planetCount++;
                    }
                    doRange = FALSE;
                }
                if (*p == '/')
                {
                    p += sizeof(char);
                }
                else
                {
                    if (*p == '-')
                    {
                        doRange = TRUE;
                        p += sizeof(char);
                    }
                    else if ((*p == '\0') || (*p == '?') || (*p == ' ') ||
                        (*p == '\t'))
                    {
                        done = TRUE;
                    }
                    else
                    {
                        err(IS, "invalid character in planet list");
                        hadError = TRUE;
                    }
                }
            }
        }
    }
    IS->is_textInPos = p;
    return (!hadError && parseConditions(IS, &ps->ps_cs, ct_planet));
}

/*
 * reqPlanets - request/get a planets specification
 */

BOOL reqPlanets(IMP, PlanetScan_t *ps, char *prompt)
{
    BOOL cont;

    if (*IS->is_textInPos == '\0')
    {
        cont = TRUE;
        while(cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = !getPlanets(IS, ps);
        }
        return TRUE;
    }
    return getPlanets(IS, ps);
}

/*
 * getPlanetCondVal -
 *      return the int giving the appropriate value for a condition.
 */

long getPlanetCondVal(IMP, register PlanetScan_t *ps, long valu)
{
    register ULONG loop;
    register long amount;

    if (valu >= 0)
    {
        return valu;
    }
    switch(valu)
    {
        case U_MILITARY:
            return (long)readPlQuan(IS, &ps->ps_currentPlanet, it_military);
        case U_OFFICERS:
            return (long)readPlQuan(IS, &ps->ps_currentPlanet, it_officers);
        case U_PLANES:
            return (long)readPlQuan(IS, &ps->ps_currentPlanet, it_planes);
        case U_MISSILES:
            return (long)readPlQuan(IS, &ps->ps_currentPlanet, it_missiles);
        case U_ORE:
            return (long)readPlQuan(IS, &ps->ps_currentPlanet, it_ore);
        case U_GOLD:
            return (long)ps->ps_currentPlanet.pl_gold;
        case U_TF:
            return (long)getTechFactor(ps->ps_currentPlanet.pl_techLevel);
        case U_CIVILIANS:
            return (long)readPlQuan(IS, &ps->ps_currentPlanet, it_civilians);
        case U_SCIENTISTS:
            return (long)readPlQuan(IS, &ps->ps_currentPlanet, it_scientists);
        case U_WEAPONS:
            return (long)readPlQuan(IS, &ps->ps_currentPlanet, it_weapons);
        case U_BARS:
            return (long)readPlQuan(IS, &ps->ps_currentPlanet, it_bars);
        case U_CLASS:
            return (long)ps->ps_currentPlanet.pl_class;
        case U_OWNER:
            return (long)ps->ps_currentPlanet.pl_owner;
        case U_EFFICIENCY:
            return (long)ps->ps_currentPlanet.pl_efficiency;
        case U_MOBILITY:
            return (long)ps->ps_currentPlanet.pl_mobility;
        case U_BTU:
            return (long)ps->ps_currentPlanet.pl_btu;
        case U_MINERALS:
            return (long)ps->ps_currentPlanet.pl_minerals;
        case U_PRODUCTION:
            amount = 0;
            for (loop = 0; loop < PPROD_LAST; loop++)   /* last is cash */
            {
                if (ps->ps_currentPlanet.pl_prod[loop] > 0)
                {
                    amount += ps->ps_currentPlanet.pl_prod[loop];
                }
            }
            return amount;
        case U_CHECKPOINT:
            if (ps->ps_currentPlanet.pl_checkpoint[0] != '\0')
            {
                return 1;
            }
            else
            {
                return 0;
            }
        case U_CIVPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_civ];
        case U_MILPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_mil];
        case U_TECHPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_technology];
        case U_RESPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_research];
        case U_EDUPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_education];
        case U_OCSPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_ocs];
        case U_OREPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_oMine];
        case U_GOLDPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_gMine];
        case U_AIRPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_airTank];
        case U_FUELPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_fuelTank];
        case U_WEAPPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_weapon];
        case U_ENGPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_engine];
        case U_HULLPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_hull];
        case U_MISSPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_missile];
        case U_PLANPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_planes];
        case U_ELECPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_electronics];
        case U_CASHPROD:
            return (long)ps->ps_currentPlanet.pl_prod[pp_cash];
        case U_CIVPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_civ];
        case U_MILPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_mil];
        case U_TECHPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_technology];
        case U_RESPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_research];
        case U_EDUPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_education];
        case U_OCSPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_ocs];
        case U_OREPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_oMine];
        case U_GOLDPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_gMine];
        case U_AIRPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_airTank];
        case U_FUELPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_fuelTank];
        case U_WEAPPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_weapon];
        case U_ENGPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_engine];
        case U_HULLPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_hull];
        case U_MISSPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_missile];
        case U_PLANPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_planes];
        case U_ELECPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_electronics];
        case U_CASHPER:
            return (long)ps->ps_currentPlanet.pl_workPer[pp_cash];
        case U_POLUTION:
            return (long)ps->ps_currentPlanet.pl_polution;
        case U_GAS:
            return (long)ps->ps_currentPlanet.pl_gas;
        case U_WATER:
            return (long)ps->ps_currentPlanet.pl_water;
        case U_NUMBIG:
            return (long)ps->ps_currentPlanet.pl_bigItems;
        case U_OWNRACE:
            return (long)ps->ps_currentPlanet.pl_ownRace;
        default:
            err(IS, "unknown planet unit");
            return(0);
    }
}

/*
 * checkPlanetCond -
 *      see if the setup conditions match the current planet
 */

BOOL checkPlanetCond(IMP, register PlanetScan_t *ps)
{
    register ConditionSet_t *cs;
    register USHORT condition;
    register long left, right;
    BOOL matching;

    cs = &ps->ps_cs;
    matching = TRUE;
    condition = 0;
    while ((condition != cs->cs_conditionCount) && matching)
    {
        left = getPlanetCondVal(IS, ps, cs->cs_condition[condition].c_left);
        right = getPlanetCondVal(IS, ps, cs->cs_condition[condition].c_right);
        switch(cs->cs_condition[condition].c_operator)
        {
            case '<':
                matching = (left < right);
                break;
            case '>':
                matching = (left > right);
                break;
            case '=':
                matching = (left == right);
                break;
            case '#':
                matching = (left != right);
                break;
        }
        condition++;
    }
    return matching;
}

/*
 * scanPlanets -
 *      The actual planet scanning routine. It calls its argument function
 *      for each planet that meets the set up specs and conditions.
 */

ULONG scanPlanets(IMP, register PlanetScan_t *ps,
        void (*scanner)(IMP, register Planet_t *planetPtr))
{
    register ULONG planetNumber;
    register ULONG i, count;
    USHORT r, c;
    BOOL doIt = FALSE;
    UBYTE race;

    count = 0;
    if (IS->is_world.w_planetNext)
    {
        race = IS->is_player.p_race;
        if (race >= NO_RACE)
        {
            /* just to get around possible problems */
            race = 0;
            if ((IS->is_player.p_status != ps_deity) &&
                (IS->is_player.p_status != ps_visitor))
            {
                log3(IS, "*** invalid race for player ", &IS->is_player.p_name[0],
                    " ***");
            }
        }
        if (ps->ps_planetPatternType == pp_list)
        {
            i = 0;
            while ((i != ps->ps_planetCount) && !clGotCtrlC(IS))
            {
                planetNumber = ps->ps_planetList[i];
                server(IS, rt_readPlanet, planetNumber);
                ps->ps_currentPlanet = IS->is_request.rq_u.ru_planet;
                if ((ps->ps_currentPlanet.pl_owner == IS->is_player.p_number)
                    || ((IS->is_world.w_race[race].r_homePlanet ==
                    planetNumber) && (race == IS->is_player.p_race)) ||
                    (IS->is_player.p_status == ps_deity))
                {
                    /* see if the planet matches the requirements */
                    if (checkPlanetCond(IS, ps))
                    {
                        count++;
                        scanner(IS, &ps->ps_currentPlanet);
                    }
                }
                i++;
            }
        }
        else
        {
            planetNumber = 0;
            while ((planetNumber != IS->is_world.w_planetNext) &&
                !clGotCtrlC(IS))
            {
                server(IS, rt_readPlanet, planetNumber);
                if ((IS->is_request.rq_u.ru_planet.pl_owner ==
                    IS->is_player.p_number) ||
                    ((IS->is_world.w_race[race].r_homePlanet ==
                    planetNumber) && (race == IS->is_player.p_race)) ||
                    (IS->is_player.p_status == ps_deity))
                {
                    ps->ps_currentPlanet = IS->is_request.rq_u.ru_planet;
                    switch(ps->ps_planetPatternType)
                    {
                        case pp_none:
                            doIt = TRUE;
                            break;
                        case pp_box:
                            r = ps->ps_currentPlanet.pl_row;
                            c = ps->ps_currentPlanet.pl_col;
                            doIt = ((r >= ps->ps_cs.cs_boxTop) &&
                                (r <= ps->ps_cs.cs_boxBottom) &&
                                (c >= ps->ps_cs.cs_boxLeft) &&
                                (c <= ps->ps_cs.cs_boxRight));
                            break;
                    }
                    if (doIt)
                    {
                        if (checkPlanetCond(IS, ps))
                        {
                            count++;
                            scanner(IS, &ps->ps_currentPlanet);
                        }
                    }
                }
                planetNumber++;
            }
        }
    }
    return count;
}

/*
 * getSectors - get a "sectors" specifier from the input buffer
 */

BOOL getSectors(IMP, register SectorScan_t *ss)
{
    return (getBox(IS, &ss->ss_cs.cs_boxTop, &ss->ss_cs.cs_boxBottom,
           &ss->ss_cs.cs_boxLeft, &ss->ss_cs.cs_boxRight) &&
        parseConditions(IS, &ss->ss_cs, ct_sector));
}

/*
 * reqSectors - request/get a sectors specification
 */

BOOL reqSectors(IMP, SectorScan_t *ss, char *prompt)
{
    BOOL cont;

    /* We aren't doing a map... */
    ss->ss_mapHook = FALSE;
    if (*IS->is_textInPos == '\0')
    {
        cont = TRUE;
        while(cont)
        {
            uPrompt(IS, prompt);
            if (!clReadUser(IS) || (*IS->is_textInPos == '\0'))
            {
                return FALSE;
            }
            (void) skipBlanks(IS);
            cont = !getSectors(IS, ss);
        }
        return TRUE;
    }
    return getSectors(IS, ss);
}

/*
 * getSectCondVal -
 *      return the int giving the appropriate value for a condition.
 */

long getSectCondVal(IMP, register SectorScan_t *ss, long valu)
{

    if (valu >= 0)
    {
        return valu;
    }
    switch(valu)
    {
        case U_CLASS:
            return (long)ss->ss_currentSector.s_type;
        case U_PLANET_COUNT:
            return (long)ss->ss_currentSector.s_planetCount;
        case U_SHIP_COUNT:
            return (long)ss->ss_currentSector.s_shipCount;
        default:
            err(IS, "unknown sector unit");
            return 0;
    }
}

/*
 * checkSectCond -
 *      see if the setup conditions match the current sector
 */

BOOL checkSectCond(IMP, register SectorScan_t *ss)
{
    register ConditionSet_t *cs;
    register USHORT condition = 0;
    register USHORT left, right;
    register BOOL matching = TRUE;

    cs = &ss->ss_cs;
    while ((condition != cs->cs_conditionCount) && matching)
    {
        left = getSectCondVal(IS, ss, cs->cs_condition[condition].c_left);
        right = getSectCondVal(IS, ss, cs->cs_condition[condition].c_right);
        switch(cs->cs_condition[condition].c_operator)
        {
            case '<':
                matching = (left < right);
                break;
            case '>':
                matching = (left > right);
                break;
            case '=':
                matching = (left == right);
                break;
            case '#':
                matching = (left != right);
                break;
            default:
                matching = FALSE;
                break;
        }
        condition++;
    }
    return matching;
}

/*
 * scanSectors -
 *      The actual sector scanning routine. It calls its argument proc
 *      for each sector that meets the set up specs and conditions.
 *	Note that this function is pretty useless, as "normal" users
 *	have no commands (currently) that make use of it.
 */

ULONG scanSectors(IMP, register SectorScan_t *ss,
        void (*scanner)(IMP, USHORT row, USHORT col, register Sector_t *s))
{
    register ULONG count;
    register USHORT r, c;
    BOOL aborted;

    count = 0;
    /* See if we are displaying a map */
    if (ss->ss_mapHook)
    {
        mapCoords(IS, ss->ss_cs.cs_boxLeft, ss->ss_cs.cs_boxRight);
        if (!IS->is_BOOL1)
        {
            userNL(IS);
        }
    }
    aborted = FALSE;
    r = ss->ss_cs.cs_boxTop;
    while ((r <= ss->ss_cs.cs_boxBottom) && !aborted)
    {
	/* See if we are displaying a map */
        if (ss->ss_mapHook)
        {
            mapRowStart(IS, r);
        }
        c = ss->ss_cs.cs_boxLeft;
        while ((c <= ss->ss_cs.cs_boxRight) && !aborted)
        {
            server(IS, rt_readSector, mapSector(IS, r, c));
            ss->ss_currentSector = IS->is_request.rq_u.ru_sector;
	    /* Currently only the deity can make use of this... */
            if (IS->is_player.p_status == ps_deity)
            {
                if (checkSectCond(IS, ss))
                {
                    count++;
                    scanner(IS, r, c, &ss->ss_currentSector);
                }
                else if (ss->ss_mapHook)
                {
                    mapEmpty(IS);
                }
            }
            else if (ss->ss_mapHook)
            {
                mapEmpty(IS);
            }
            c++;
            aborted = clGotCtrlC(IS);
        }
        if (ss->ss_mapHook && !aborted)
        {
            mapRowEnd(IS, r);
        }
        r++;
        if (clGotCtrlC(IS))
        {
            aborted = TRUE;
        }
    }
    if (ss->ss_mapHook && !aborted)
    {
        if (!IS->is_BOOL1)
        {
            userNL(IS);
        }
        mapCoords(IS, ss->ss_cs.cs_boxLeft, ss->ss_cs.cs_boxRight);
    }
    if ((count == 1) && (ss->ss_cs.cs_boxTop == ss->ss_cs.cs_boxBottom) &&
        (ss->ss_cs.cs_boxRight == ss->ss_cs.cs_boxLeft) &&
        (ss->ss_cs.cs_conditionCount == 0))
    {
        count = SINGLE_SECTOR;
    }
    return count;
}



/*
 * handleMinerRealm - does the actual parsing of a realm based on what is
 *          correct for miners. At the function call time we know
 *          IS->is_textInPos is pointing to a "#"
 */

BOOL handleMinerRealm(IMP, register MinerScan_t *mns)
{
    register char *p;
    register ULONG planetNumber;
    register USHORT realm;
    register char ch;
    register ULONG wkNum;
    BOOL hadError, done, doRange;

    /* set up the default realm */
    realm = 0;
    /* move past the "#" */
    IS->is_textInPos += sizeof(char);
    /* store the next character */
    ch = *IS->is_textInPos;
    if (((ch >= '0') && (ch < ('0' + REALM_MAX))) || (ch == '\0') ||
        (ch == ' ') || (ch == '\t') || (ch == '/') || (ch == '?'))
    {
        /* if the next character is a number, store the number in realm */
        if ((ch >= '0') && (ch < ('0' + REALM_MAX)))
        {
            realm = ch - '0';
            /* move to the next character */
            IS->is_textInPos += sizeof(char);
        }
    }
    /* determine if this is a box entry or a list item */
    if (strpbrk(&IS->is_player.p_realm[realm][0], ",:") != NULL)
    {
        /* only box type entries should have a "," or ":" character */
        mns->mns_minerPatternType = mnp_box;
        /* getBox() handles the "#" on it's own, so we have to back up */
        IS->is_textInPos -= sizeof(char);
        /* if it still isn't a "#", back up one last time */
        if (*IS->is_textInPos != '#')
        {
            IS->is_textInPos -= sizeof(char);
        }
        /* now let getBox() do it's thing, and return whatever it gives us */
        return (getBox(IS, &mns->mns_cs.cs_boxTop, &mns->mns_cs.cs_boxBottom,
           &mns->mns_cs.cs_boxLeft, &mns->mns_cs.cs_boxRight));
    }
    /* we have a list of planet numbers separated by '/'s */
    p = &IS->is_player.p_realm[realm][0];
    mns->mns_minerPatternType = mnp_plList;
    /* note that minerCount is planets in this case */
    mns->mns_minerCount = 0;
    hadError = FALSE;
    done = FALSE;
    doRange = FALSE;
    while (!done && !hadError)
    {
        if (mns->mns_minerCount == MAX_MINERS)
        {
            err(IS, "too many planets listed");
            hadError = TRUE;
        }
        else
        {
            if ((*p < '0') || (*p > '9'))
            {
                err(IS, "invalid planet number");
                hadError = TRUE;
            }
            else
            {
                planetNumber = 0;
                while ((*p >= '0') && (*p <= '9'))
                {
                    planetNumber = planetNumber * 10 + (*p - '0');
                    p += sizeof(char);
                }
                if (planetNumber >= IS->is_world.w_planetNext)
                {
                    err(IS, "planet number too big");
                    hadError = TRUE;
                }
                else
                {
                    if (doRange && (mns->mns_minerCount > 0))
                    {
                        wkNum = mns->mns_minerList[mns->mns_minerCount - 1] + 1;
                        while ((wkNum <= planetNumber) &&
                            (mns->mns_minerCount < MAX_MINERS - 1))
                        {
                            mns->mns_minerList[mns->mns_minerCount] = wkNum;
                            mns->mns_minerCount++;
                            wkNum++;
                        }
                        if (mns->mns_minerCount == MAX_MINERS)
                        {
                            err(IS, "too many miners listed");
                            hadError = TRUE;
                        }
                    }
                    else
                    {
                        mns->mns_minerList[mns->mns_minerCount] = planetNumber;
                        mns->mns_minerCount++;
                    }
                    doRange = FALSE;
                }
                if (*p == '/')
                {
                    p += sizeof(char);
                }
                else
                {
                    if (*p == '-')
                    {
                        doRange = TRUE;
                        p += sizeof(char);
                    }
                    else if ((*p == '\0') || (*p == '?') || (*p == ' ') ||
                        (*p == '\t'))
                    {
                        done = TRUE;
                    }
                    else
                    {
                        err(IS, "invalid character in planet list");
                        hadError = TRUE;
                    }
                }
            }
        }
    }
    return !hadError;
}

/*
 * getMiners -
 *      get a miners specifier
 */

BOOL getMiners(IMP, register MinerScan_t *mns)
{
    register char *p;
    register ULONG minerNumber;
    register char ch;
    register ULONG wkNum;
    BOOL hadError, done, doRange;

    ch = *IS->is_textInPos;
    if ((ch == '?') || (ch == '/'))
    {
        mns->mns_minerPatternType = mnp_none;
        return parseConditions(IS, &mns->mns_cs, ct_miner);
    }
    if (ch == '#')
    {
        if (handleMinerRealm(IS, mns))
        {
            return parseConditions(IS, &mns->mns_cs, ct_miner);
        }
        return FALSE;
    }
    if (ch == '-')
    {
        mns->mns_minerPatternType = mnp_box;
        if (getBox(IS, &mns->mns_cs.cs_boxTop, &mns->mns_cs.cs_boxBottom,
               &mns->mns_cs.cs_boxLeft, &mns->mns_cs.cs_boxRight))
        {
            return parseConditions(IS, &mns->mns_cs, ct_miner);
        }
        return FALSE;
    }
    if ((ch < '0') || (ch > '9'))
    {
        err(IS, "invalid miner specification");
        return FALSE;
    }
    p = IS->is_textInPos;
    while ((*p >= '0') && (*p <= '9'))
    {
        p += sizeof(char);
    }
    if ((*p == ',') || (*p == ':'))
    {
        mns->mns_minerPatternType = mnp_box;
        return (getBox(IS, &mns->mns_cs.cs_boxTop, &mns->mns_cs.cs_boxBottom,
               &mns->mns_cs.cs_boxLeft, &mns->mns_cs.cs_boxRight) &&
            parseConditions(IS, &mns->mns_cs, ct_miner));
    }
    if (*IS->is_textInPos == 's')
    {
        /* we have a list of ship numbers separated by '/'s */
        p = IS->is_textInPos;
        mns->mns_minerPatternType = mnp_shList;
        mns->mns_minerCount = 0;
        hadError = FALSE;
        done = FALSE;
        doRange = FALSE;
        while (!done && !hadError)
        {
            if (mns->mns_minerCount == MAX_MINERS)
            {
                err(IS, "too many ships listed");
                hadError = TRUE;
            }
            else
            {
                if ((*p < '0') || (*p > '9'))
                {
                    err(IS, "invalid ship number");
                    hadError = TRUE;
                }
                else
                {
                    minerNumber = 0;
                    while ((*p >= '0') && (*p <= '9'))
                    {
                        minerNumber = minerNumber * 10 + (*p - '0');
                        p += sizeof(char);
                    }
                    if (minerNumber >= IS->is_world.w_shipNext)
                    {
                        err(IS, "ship number too big");
                        hadError = TRUE;
                    }
                    else
                    {
                        if (doRange && (mns->mns_minerCount > 0))
                        {
                            wkNum = mns->mns_minerList[
                                mns->mns_minerCount - 1] + 1;
                            while ((wkNum <= minerNumber) &&
                                (mns->mns_minerCount < MAX_MINERS - 1))
                            {
                                mns->mns_minerList[mns->mns_minerCount]
                                    = wkNum;
                                mns->mns_minerCount++;
                                wkNum++;
                            }
                            if (mns->mns_minerCount == MAX_MINERS)
                            {
                                err(IS, "too many ships listed");
                                hadError = TRUE;
                            }
                        }
                        else
                        {
                            mns->mns_minerList[mns->mns_minerCount]
                                = minerNumber;
                            mns->mns_minerCount++;
                        }
                        doRange = FALSE;
                    }
                    if (*p == '/')
                    {
                        p += sizeof(char);
                    }
                    else
                    {
                        if (*p == '-')
                        {
                            doRange = TRUE;
                            p += sizeof(char);
                        }
                        else if ((*p == '\0') || (*p == '?') || (*p == ' ') ||
                            (*p == '\t'))
                        {
                            done = TRUE;
                        }
                        else
                        {
                            err(IS, "invalid character in ship list");
                            hadError = TRUE;
                        }
                    }
                }
            }
        }
    }
    else if (*IS->is_textInPos == 'p')
    {
        /* we have a list of planet numbers separated by '/'s */
        p = IS->is_textInPos;
        mns->mns_minerPatternType = mnp_plList;
        mns->mns_minerCount = 0;
        hadError = FALSE;
        done = FALSE;
        doRange = FALSE;
        while (!done && !hadError)
        {
            if (mns->mns_minerCount == MAX_MINERS)
            {
                err(IS, "too many planets listed");
                hadError = TRUE;
            }
            else
            {
                if ((*p < '0') || (*p > '9'))
                {
                    err(IS, "invalid planet number");
                    hadError = TRUE;
                }
                else
                {
                    minerNumber = 0;
                    while ((*p >= '0') && (*p <= '9'))
                    {
                        minerNumber = minerNumber * 10 + (*p - '0');
                        p += sizeof(char);
                    }
                    if (minerNumber >= IS->is_world.w_planetNext)
                    {
                        err(IS, "planet number too big");
                        hadError = TRUE;
                    }
                    else
                    {
                        if (doRange && (mns->mns_minerCount > 0))
                        {
                            wkNum = mns->mns_minerList[
                                mns->mns_minerCount - 1] + 1;
                            while ((wkNum <= minerNumber) &&
                                (mns->mns_minerCount < MAX_MINERS - 1))
                            {
                                mns->mns_minerList[mns->mns_minerCount]
                                    = wkNum;
                                mns->mns_minerCount++;
                                wkNum++;
                            }
                            if (mns->mns_minerCount == MAX_MINERS)
                            {
                                err(IS, "too many planets listed");
                                hadError = TRUE;
                            }
                        }
                        else
                        {
                            mns->mns_minerList[mns->mns_minerCount]
                                = minerNumber;
                            mns->mns_minerCount++;
                        }
                        doRange = FALSE;
                    }
                    if (*p == '/')
                    {
                        p += sizeof(char);
                    }
                    else
                    {
                        if (*p == '-')
                        {
                            doRange = TRUE;
                            p += sizeof(char);
                        }
                        else if ((*p == '\0') || (*p == '?') || (*p == ' ') ||
                            (*p == '\t'))
                        {
                            done = TRUE;
                        }
                        else
                        {
                            err(IS, "invalid character in planet list");
                            hadError = TRUE;
                        }
                    }
                }
            }
        }
    }
    else
    {
        /* we have a list of miner numbers separated by '/'s */
        p = IS->is_textInPos;
        mns->mns_minerPatternType = mnp_list;
        mns->mns_minerCount = 0;
        hadError = FALSE;
        done = FALSE;
        doRange = FALSE;
        while (!done && !hadError)
        {
            if (mns->mns_minerCount == MAX_MINERS)
            {
                err(IS, "too many miners listed");
                hadError = TRUE;
            }
            else
            {
                if ((*p < '0') || (*p > '9'))
                {
                    err(IS, "invalid miner number");
                    hadError = TRUE;
                }
                else
                {
                    minerNumber = 0;
                    while ((*p >= '0') && (*p <= '9'))
                    {
                        minerNumber = minerNumber * 10 + (*p - '0');
                        p += sizeof(char);
                    }
                    if (minerNumber >= IS->is_world.w_shipNext)
                    {
                        err(IS, "miner number too big");
                        hadError = TRUE;
                    }
                    else
                    {
                        if (doRange && (mns->mns_minerCount > 0))
                        {
                            wkNum = mns->mns_minerList[
                                mns->mns_minerCount - 1] + 1;
                            while ((wkNum <= minerNumber) &&
                                (mns->mns_minerCount < MAX_MINERS - 1))
                            {
                                mns->mns_minerList[mns->mns_minerCount]
                                    = wkNum;
                                mns->mns_minerCount++;
                                wkNum++;
                            }
                            if (mns->mns_minerCount == MAX_MINERS)
                            {
                                err(IS, "too many miners listed");
                                hadError = TRUE;
                            }
                        }
                        else
                        {
                            mns->mns_minerList[mns->mns_minerCount]
                                = minerNumber;
                            mns->mns_minerCount++;
                        }
                        doRange = FALSE;
                    }
                    if (*p == '/')
                    {
                        p += sizeof(char);
                    }
                    else
                    {
                        if (*p == '-')
                        {
                            doRange = TRUE;
                            p += sizeof(char);
                        }
                        else if ((*p == '\0') || (*p == '?') || (*p == ' ') ||
                            (*p == '\t'))
                        {
                            done = TRUE;
                        }
                        else
                        {
                            err(IS, "invalid character in miner list");
                            hadError = TRUE;
                        }
                    }
                }
            }
        }
    }
    IS->is_textInPos = p;
    if (!hadError)
    {
        return parseConditions(IS, &mns->mns_cs, ct_miner);
    }
    return FALSE;
}

/*
 * reqMiners - request/get a ships list
 */

BOOL reqMiners(IMP, MinerScan_t *mns, char *prompt)
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
        while(!getMiners(IS, mns));
        return TRUE;
    }
    return getMiners(IS, mns);
}

/*
 * getMinerCondVal - return the long giving the appropriate value for a
 *          miner cond
 */

long getMinerCondVal(IMP, register MinerScan_t *mns, long valu)
{
    if (valu >= 0)
    {
        return valu;
    }
    switch (valu)
    {
        case U_ORE:
            return (long)mns->mns_currentMiner.sh_items[it_ore];
        case U_BARS:
            return (long)mns->mns_currentMiner.sh_items[it_bars];
        case U_EFFICIENCY:
            return (long)mns->mns_currentMiner.sh_efficiency;
        case U_OWNER:
            return (long)mns->mns_currentMiner.sh_owner;
        case U_TF:
            return (long)mns->mns_currentMiner.sh_hullTF;
        case U_CARRIED:
            if (mns->mns_currentMiner.sh_dragger != NO_ITEM)
            {
                return 1;
            }
            return 0;
        case U_PRICE:
            return (long)mns->mns_currentMiner.sh_price;
        case U_CARGO:
            return (long)mns->mns_currentMiner.sh_cargo;
        case U_FUEL:
        case U_ENERGY:
            return (long)mns->mns_currentMiner.sh_energy;
        case U_MESSAGE:
            return (long)mns->mns_currentMiner.sh_weapon[1];
        case U_ACTIVITY:
            return (long)mns->mns_currentMiner.sh_weapon[0];
        case U_FREE:
            return (long)
                (IS->is_world.w_shipCargoLim[mns->mns_currentMiner.sh_type] -
                mns->mns_currentMiner.sh_cargo);
        default:
            err(IS, "unknown miner unit");
            return(0);
    }
}

/*
 * checkMinerCond -
 *      see if the setup conditions match the current miner
 */

BOOL checkMinerCond(IMP, MinerScan_t *mns)
{
    register ConditionSet_t *cs;
    register ULONG condition;
    register int left, right;
    BOOL matching;

    cs = &mns->mns_cs;
    matching = TRUE;
    condition = 0;
    while ((condition != cs->cs_conditionCount) && matching)
    {
        left  = getMinerCondVal(IS, mns, cs->cs_condition[condition].c_left);
        right = getMinerCondVal(IS, mns, cs->cs_condition[condition].c_right);
        switch (cs->cs_condition[condition].c_operator)
        {
            case '<':
                matching = left < right;
                break;
            case '>':
                matching = left > right;
                break;
            case '=':
                matching = left == right;
                break;
            case '#':
                matching = left != right;
                break;
        }
        condition++;
    }
    return matching;
}

/*
 * scanMiners -
 *      The actual miner scanning routine. It calls its argument proc
 *      for each miner that meets the set up specs and conditions.
 */

ULONG scanMiners(IMP, register MinerScan_t *mns,
    void (*scanner)(IMP, ULONG minerNumber, Ship_t *sh))
{
    register ULONG minerNumber;
    register ULONG i, count;
    USHORT r, c;
    BOOL doIt = FALSE;

    count = 0;
    if (IS->is_world.w_shipNext)
    {
        if (mns->mns_minerPatternType == mnp_list)
        {
            i = 0;
            while ((i != mns->mns_minerCount) && !clGotCtrlC(IS))
            {
                minerNumber = mns->mns_minerList[i];
                server(IS, rt_readShip, minerNumber);
                mns->mns_currentMiner = IS->is_request.rq_u.ru_ship;
                if (((mns->mns_currentMiner.sh_owner == IS->is_player.p_number)
                    || (IS->is_player.p_status == ps_deity)) &&
                    (IS->is_request.rq_u.ru_ship.sh_type == st_m))
                {
                    if (checkMinerCond(IS, mns))
                    {
                        count++;
                        scanner(IS, minerNumber, &mns->mns_currentMiner);
                    }
                }
                i++;
            }
        }
        else
        {
            minerNumber = 0;
            while ((minerNumber != IS->is_world.w_shipNext) &&
                !clGotCtrlC(IS))
            {
                server(IS, rt_readShip, minerNumber);
                if (((IS->is_request.rq_u.ru_ship.sh_owner ==
                    IS->is_player.p_number) || (IS->is_player.p_status ==
                    ps_deity)) && (IS->is_request.rq_u.ru_ship.sh_type ==
                    st_m))
                {
                    mns->mns_currentMiner = IS->is_request.rq_u.ru_ship;
                    switch(mns->mns_minerPatternType)
                    {
                        case mnp_none:
                            doIt = TRUE;
                            break;
                        case mnp_box:
                            r = mns->mns_currentMiner.sh_row;
                            c = mns->mns_currentMiner.sh_col;
                            doIt = ((r >= mns->mns_cs.cs_boxTop) &&
                                (r <= mns->mns_cs.cs_boxBottom) &&
                                (c >= mns->mns_cs.cs_boxLeft) &&
                                (c <= mns->mns_cs.cs_boxRight));
                            break;
                        case mnp_shList:
                            i = 0;
                            doIt = FALSE;
                            while ((i != mns->mns_minerCount) && !doIt)
                            {
                                if (mns->mns_currentMiner.sh_dragger ==
                                    mns->mns_minerList[i])
                                {
                                    doIt = TRUE;
                                }
                                i++;
                            }
                            break;
                        case mnp_plList:
                            i = 0;
                            doIt = FALSE;
                            while ((i != mns->mns_minerCount) && !doIt)
                            {
                                if (mns->mns_currentMiner.sh_planet ==
                                    mns->mns_minerList[i])
                                {
                                    doIt = TRUE;
                                }
                                i++;
                            }
                            break;
                    }
                    if (doIt)
                    {
                        if (checkMinerCond(IS, mns))
                        {
                            count++;
                            scanner(IS, minerNumber, &mns->mns_currentMiner);
                        }
                    }
                }
                minerNumber++;
            }
        }
    }
    return count;
}

