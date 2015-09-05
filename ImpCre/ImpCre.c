/*
 * Imperium
 *
 * $Id: ImpCre.c,v 1.6 2000/05/25 23:39:28 marisa Exp $
 *
 * Builds up a region of "space" for Imperium to use
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
 * $Log: ImpCre.c,v $
 * Revision 1.6  2000/05/25 23:39:28  marisa
 * Removed useage of "register" tag
 * Fixed bug in Makefile.in lacking -c
 *
 * Revision 1.5  2000/05/24 19:35:47  marisa
 * Make compresses mode the default
 *
 * Revision 1.4  2000/05/23 20:22:33  marisa
 * Now initializes the new pl_weapon structure in planets.
 * Fix incorrect path setting in the .sh files
 *
 * Revision 1.3  2000/05/18 08:11:09  marisa
 * Autoconf working
 *
 * Revision 1.2  2000/05/18 06:41:45  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:31  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:29:29  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 19:48:19  marisag
 * patch20: Fix empty rev.
 *
 */

#include "../config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
BOGUS - You need to replace unistd.h
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
BOGUS - You need to replace stdlib.h
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
BOGUS - You need to replace stdarg.h
#endif
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <time.h>
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
BOGUS - You need to replace unistd.h
#endif
#include <sys/stat.h>
#include <signal.h>
#include "../Include/Imperium.h"

static const char rcsid[] =
    "$Id: ImpCre.c,v 1.6 2000/05/25 23:39:28 marisa Exp $";

/*
 * These are global values that get set via the config file created by
 *          ImpCre
 */

static char
    gbLogFile[255],
    gbWorldFile[255],
    gbPlayerFile[255],
    gbShipFile[255],
    gbFleetFile[255],
    gbLoanFile[255],
    gbOfferFile[255],
    gbBigItemFile[255],
    gbCreationPassword[PASSWORD_LEN],
    gbGodPassword[PASSWORD_LEN],
    gbGameName[48],
    gbGameHost[48],
    gbGamePort[10],
    gbLastWinner[NAME_LEN],
    gbPlanetsFile[255],
    gbSectorsFile[255],
    gbEmailAddress[128];
static ULONG
    gbVertSize,
    gbHorizSize,
    gbNumPlay,
    gbConTime,
    gbSecItu,
    MaxTries;
static BOOL
    gbPublicMessages,
    gbChangePlayers;


/*
 --------------------------------------------------------------------
                          Combat Items
*/

/* arrays giving default values for several things which the Deity can
   adjust in Imperium. See the actual code for many others */

const USHORT
    ATTACK_MOBILITY_COST[4][4] = {
      {125,   208,   291, 21300},
      {167,   250,   333, 21300},
      {208,   291,   375, 21400},
    {10700, 10800, 10900, 31900}
};

/*
 --------------------------------------------------------------------
                          Ship Items
*/

const USHORT
    /* crew reqd to nav. per engine */
    SHIP_CREWREQ[ST_LAST + 1] = {2, 5, 8, 20, 40, 0},

    /* crew reqd to fire per weapon */
    SHIP_FIREREQ[ST_LAST + 1] = {2, 2, 2, 8, 16, 0},

    /* cargo limits */
    SHIP_CARGO[ST_LAST + 1]  = {3010, 6010, 12020, 24320, 51310, 3000},

    /* base nav costs */
    SHIP_NAVCOST[ST_LAST + 1]  = {423, 644, 871, 1016, 1405, 200},

    /* hull cost    */
    SHIP_COST[ST_LAST + 1]  = {120, 275, 425, 800, 1625, 275};

/*
 --------------------------------------------------------------------
                  Mobility Costs for Items
*/

const USHORT
    MOB_COST[IT_LAST + 1]  = {1, 1, 1, 1, 2, 4, 1, 10, 2, 3, 1, 1, 1, 1, 1};

/*
 --------------------------------------------------------------------
                 Default Production costs for big items
*/


        /* note that many items do not use this, and are set to 0 */
const USHORT
    PROD_COST[PPROD_LAST + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 85, 190, 0, 0,
        0, 10, 0};

/*
 --------------------------------------------------------------------
                      Default weights for items
*/

const USHORT
    IT_WEIGHT[IT_LAST + 1] = {1, 1, 1, 1, 15, 75, 3, 10, 25, 75, 35, 1050,
        450, 400, 800};


/*
 --------------------------------------------------------------------
                        Space Building Items
*/

/* attempt counters, array sizes, etc. */

#define MAX_GROW_TRIES          100
#define MAX_HOME_TRIES_OUTER    100
#define MAX_HOME_TRIES_INNER    100
#define MIN_SIZE                10          /* minimum universe size        */
#define MAX_SIZE                256         /* maximum universe size        */

/*
 ------------------------------------------------------------------------
                            Type Definitions
*/

#define l_eof       0
#define l_empty     1
#define l_ok        2
typedef UBYTE LineStatus_t;

#define rn_eof      0
#define rn_empty    1
#define rn_error    2
#define rn_ok       3
typedef UBYTE NumberStatus_t;

/*
 --------------------------------------------------------------------
         Arrays Needed During Terrain and Mineral Construction
*/

typedef struct
    {
        UBYTE
            ba_bit[MAX_SIZE];
    } BitArray_t;

USHORT
    Range[MAX_SIZE],
    TerrainRange[MAX_SIZE],
    OreRange[MAX_SIZE];

BitArray_t
    *Bits[MAX_SIZE];       /* array of pointers to array of bytes  */

/* values used as bits to describe sectors being built */
/*

   Bits 76543210
        --------
         fedcbaa    aa - Sector type
                         00 - Vacant
                         01 - Normal sector
                         10 - home planet sector
                         11 - mask used to detect is sector is vacant
                         11 - Other type of sector
                    b  - Sector contains at least 1 planet
                    c  - Sector contains "high level" of stars
                    d  - Sector is reachable by a home planet
                    e  - Sector is taken by a home planet
                    f  - Sector contains "high level" of planets
*/

#define CELL_VACANT (0x0)
#define CELL_NORM   (0x1)
#define CELL_HOME   (0x2)
#define CELL_TERR   (0x3)
#define CELL_OTHER  (0x3)
#define CELL_PLAN   (0x4)
#define CELL_HISTAR (0x8)
#define CELL_REACH  (0x10)
#define CELL_TAKEN  (0x20)
#define CELL_HIPLAN (0x40)

/*
 -------------------------------------------------------------------------
                      General Purpose Globals
*/

char *DEFAULT_PATH = "";                /* default to current directory */

char
    RaceName[RACE_MAX][NAME_LEN];

char
    PlanName[RACE_MAX][PLAN_NAME_LEN];

char finalname[RACE_MAX][PLAN_NAME_LEN];

static const char
	*DEF_RACE[] =
		{
			"Voltar",
			"Human",
			"Nakasumi",
			"Krellian",
			"Kindit",
			"Ego",
			"Dorn",
			"Zyxylck"
		},
	*DEF_PLAN[] =
		{
			"Voltar",
			"Terra",
			"Tokiga",
			"Krell",
			"Targe",
			"Arcadia",
			"Huraza",
			"Krykcoq"
		};

char
    Path[258];                          /* path to access files */

#define BUFF_SIZE 258
char
    InputBuff[BUFF_SIZE],
    InputLine[BUFF_SIZE],
    OutputLine[BUFF_SIZE];

USHORT
    InputLinePos,
    OutputLinePos;

char
    FileName[258];

ULONG
    StarAmt,                            /* number of sect w/stars           */
    MultStarAmt,                        /* # of sect w/more than 3 stars    */
    PlanAmt,                            /* # of sect w/planets              */
    MultPlanAmt,                        /* # of sect w/more than 5 planets  */
    OthAmt,                             /* number of "Other" special sectors*/
    SqSize,                             /* total number of galactic sectors */
    BlkHolePct;                         /* odds of an "other" sector being  */
                                        /* a black hole                     */

static BOOL
    HadBreak,
    DoVerb;

/*
 --------------------------------------------------------------------
                Configurable Variables and Their Defaults
*/

#define DEF_MIN_DIST        10
#define DEF_MIN_NEAR        1
#define DEF_MIN_REACHABLE   15
#define DEF_MAX_REACH       15
#define DEF_STARPER         65
#define DEF_SMALLSTARSIZE   6
#define DEF_LARGESTARSIZE   9
#define DEF_SMALLPLANSIZE   1
#define DEF_LARGEPLANSIZE   5
#define DEF_PLANPER         75

ULONG
    OtherSect,                          /* desired percent of "other" sect. */
    StarPer,                            /* percent of stars to sectors      */
    MultStarPer,                        /* percent with higher # of stars   */
    PlanPer,                            /* percent of planets to sectors    */
    MultPlanPer,                        /* percent with higher # of planets */
    MinDist,                            /* minimum home planet separation   */
    MinNear,                            /* minimum normal sect beside home  */
    MinReachable,                       /* minimum normal sect per home plnt */
    MaxReach,                           /* they must be within this distance */
    MaxSpread,                          /* max diff between races           */
    InitialMoney,                       /* initial money for each player    */
    SmallStarSize,                      /* minum star size                  */
    LargeStarSize,                      /* max star size                    */
    HomePlanSize,                       /* size of home planets             */
    SmallPlanetSize,                    /* minum planet size                */
    LargePlanetSize;                    /* max planet size                  */

World_t
    World;

World_t *w;

Player_t
    Player[PLAYER_MAX];

int
	oldUmask;

/*
 --------------------------------------------------------------------
                        Start of Functions
*/

void breakHandler(int whichSig)
{
	HadBreak = TRUE;
	signal(SIGINT, breakHandler); /* Needed for SYSV */
}

/*
 * impRandom - return a random number 0 - passed range.
 */

USHORT impRandom(USHORT rang)
{
    if (rang)
    {
        return ((lrand48() >> 8) % rang);
    }
    return(0);
}

BOOL doGrow(void)
{
    ULONG i;
    USHORT row=0, col=0; /* prevent gcc warning */
    ULONG numStar, numPlan, numHiStar, numHiPlan, numOth;
    BOOL doRoll;
    int hDir=0, vDir=0; /* prevent gcc warning */

    i = 0;
    numStar = i;
    numHiStar = i;
    numPlan = i;
    numHiPlan = i;
    numOth = i;
    doRoll = TRUE;
    /* loop until we run out of retries or we have nothing else to place */
    while(((numStar < StarAmt) || (numPlan < PlanAmt) || (numOth < OthAmt))
        && (i < MaxTries))
    {
        if (doRoll)
        {
            /* pick a random row and column */
            row = impRandom(w->w_rows);
            col = impRandom(w->w_columns);
        }
        /* make sure selected sector is vacant */
        if ((Bits[row]->ba_bit[col] & CELL_TERR) == 0)
        {
            /* Set this back since we had success this time */
            doRoll = TRUE;
            /* If there are any "other" sectors to place, do them first */
            if (numOth < OthAmt)
            {
                Bits[row]->ba_bit[col] = CELL_OTHER;
                numOth++;
            }
            else
            {
                /* check for stars to place */
                if (numStar < StarAmt)
                {
                    if (numStar < StarAmt / 2)
                    {
                        /* Have more than half left, so be sure */
                        /* and make a multistar sector, if avail*/
                        if (numHiStar < MultStarAmt)
                        {
                            Bits[row]->ba_bit[col] = (CELL_NORM | CELL_HISTAR);
                            numStar++;
                            numHiStar++;
                        }
                        else
                        {
                            Bits[row]->ba_bit[col] = CELL_NORM;
                            numStar++;
                        }
                    }
                    else
                    {
                        /* have less than half left, so try and break up */
                        /* remaining stars */
                        if (impRandom(100) < 50)
                        {
                            if (numHiStar < MultStarAmt)
                            {
                                Bits[row]->ba_bit[col] = (CELL_NORM |
                                    CELL_HISTAR);
                                numStar++;
                                numHiStar++;
                            }
                            else
                            {
                                Bits[row]->ba_bit[col] = CELL_NORM;
                                numStar++;
                            }
                        }
                        else
                        {
                            Bits[row]->ba_bit[col] = CELL_NORM;
                            numStar++;
                        }
                    }
                }
                /* Now see about adding planets */
                if (numPlan < PlanAmt)
                {
                    if (numPlan < PlanAmt / 2)
                    {
                        /* Have more than half left, so be sure */
                        /* and make a multiplan sector, if avail*/
                        if (numHiPlan < MultPlanAmt)
                        {
                            Bits[row]->ba_bit[col] = (Bits[row]->ba_bit[col] |
                                CELL_HIPLAN);
                            numPlan++;
                            numHiPlan++;
                        }
                        else
                        {
                            Bits[row]->ba_bit[col] = (Bits[row]->ba_bit[col] |
                                CELL_PLAN);
                            numPlan++;
                        }
                    }
                    else
                    {
                        if (impRandom(100) < 75)
                        {
                            if (numHiPlan < MultPlanAmt)
                            {
                                Bits[row]->ba_bit[col] = (Bits[row]->ba_bit[col]
                                    | CELL_HIPLAN);
                                numPlan++;
                                numHiPlan++;
                            }
                            else
                            {
                                Bits[row]->ba_bit[col] = (Bits[row]->ba_bit[col]
                                    | CELL_PLAN);
                                numPlan++;
                            }
                        }
                        else
                        {
                            Bits[row]->ba_bit[col] = (Bits[row]->ba_bit[col] |
                                CELL_PLAN);
                            numPlan++;
                        }
                    }
                }
            }
        }
        else
        {
            /* Since we did not find a vacant sector, try to find a */
            /* nearby one to use instead */
            if (doRoll)
            {
                doRoll = FALSE;
                /* Since doRoll is TRUE, we know that position we are */
                /* currently at was randomly derived. So lets treat it */
                /* as an "explosion" and try to create debris */
                vDir = impRandom(1000);
                if (vDir > 444)
                {
                    if (vDir > 888)
                    {
                        if (vDir > 990)
                        {
                            vDir = -3;
                        }
                        else
                        {
                            vDir = -2;
                        }
                    }
                    else
                    {
                        vDir = -1;
                    }
                }
                else if (vDir > 333)
                {
                    vDir = 0;
                }
                else
                {
                    if (vDir > 111)
                    {
                        vDir = 1;
                    }
                    else
                    {
                        if (vDir > 9)
                        {
                            vDir = 2;
                        }
                        else
                        {
                            vDir = 3;
                        }
                    }
                }
                hDir = impRandom(1000);
                if (hDir > 666)
                {
                    if (hDir > 888)
                    {
                        if (hDir > 990)
                        {
                            hDir = -3;
                        }
                        else
                        {
                            hDir = -2;
                        }
                    }
                    else
                    {
                        hDir = -1;
                    }
                }
                else if (hDir > 333)
                {
                    hDir = 0;
                }
                else
                {
                    if (hDir > 111)
                    {
                        hDir = 1;
                    }
                    else
                    {
                        if (hDir > 9)
                        {
                            hDir = 2;
                        }
                        else
                        {
                            hDir = 3;
                        }
                    }
                }
                /* sanity checking */
                if ((vDir == 0) && (hDir == 0))
                {
                    /* note this means that 1/3 * 1/3 of the time we will */
                    /* just reroll another number anyway */
                    doRoll = TRUE;
                }
            }
            if (vDir >= 0)
            {
                row += vDir;
                if (row >= w->w_rows)
                {
                    doRoll = TRUE;
                }
            }
            else
            {
                if (abs(vDir) > row)
                {
                    doRoll = TRUE;
                }
                else
                {
                    row += vDir;
                }
            }
            if (hDir >= 0)
            {
                col += hDir;
                if (col >= w->w_columns)
                {
                    doRoll = TRUE;
                }
            }
            else
            {
                if (abs(hDir) > col)
                {
                    doRoll = TRUE;
                }
                else
                {
                    col += hDir;
                }
            }
            i++;
        }
    }
    if (i < MaxTries)
    {
        return TRUE;
    }
    if (DoVerb)
    {
	printf("-> Using the value of %lu for MaxTries I was:\n", MaxTries);
	printf("-> Able to allocate %lu out of %lu stars\n", numStar, StarAmt);
	printf("-> Able to allocate %lu out of %lu planets\n", numPlan, PlanAmt);
	printf("-> Able to allocate %lu out of %lu \"others\"\n", numOth, OthAmt);
    }
    return FALSE;
}

/*
 * growSpace - attempt to grow a decent area of space. Return 'false' if fail.
 */

BOOL growSpace(void)
{
    USHORT row;
    USHORT attempt;

    attempt = 0;
    while(attempt < MAX_GROW_TRIES)
    {
        if (attempt > 0)
        {
            puts("Unable to grow a satisfactory area of space - "
                "Trying again");
        }
	if (HadBreak)
        {
            (void) puts("*** Build interupted!");
            return FALSE;
        }
        /* Make sure the area of memory is cleared before using it */
        for (row = 0; row < w->w_rows; row++)
        {
            memset(Bits[row], '\0', sizeof(BitArray_t));
        }
        if (doGrow())
        {
            return TRUE;
        }
        attempt++;
    }
    return FALSE;
}

/*
 * initializePlayer - initialize the stats for a new user.
 */

void initializePlayer(USHORT player)
{
    USHORT i;
    Player_t *p;
    ULONG tempval;
    USHORT j;

    tempval = 0;
    p = &Player[player];
    p->p_lastOn = World.w_lastRun;
    p->p_timeLeft = World.w_maxConnect;
    p->p_timeWarn = tempval;
    p->p_telegramsNew = tempval;
    p->p_telegramsTail = tempval;
    p->p_planetCount = tempval;
    p->p_money = InitialMoney;
    p->p_number = player;
    p->p_feMode = tempval;
    p->p_btu = World.w_maxBTUs;
    p->p_btuWarn = tempval;
    p->p_conWidth = tempval;
    p->p_conLength = tempval;

    /* clear out the fleets */
    for (i = tempval; i < 26 + 26; i++)
    {
        p->p_fleets[i] = NO_FLEET;
    }
    /* clear out the actions */
    for (i = tempval; i < MAX_NUM_ACTIONS; i++)
    {
        p->p_action[i].ac_action = a_none;
        for (j = tempval; j <= IT_LAST; j++)
        {
            p->p_action[i].ac_items[j] = tempval;
        }
    }
    p->p_status = ps_idle;

    if (player != 0)
    {
        /* clear out the names to be safe */
        memset(&p->p_name[0], '\0', NAME_LEN * sizeof(char));
        memset(&p->p_password[0], '\0', PASSWORD_LEN * sizeof(char));
        memset(&p->p_email[0], '\0', EMAIL_LEN * sizeof(char));
    }
    /* set all the player-race/player-player relations */
    for (i = tempval; i < RACE_MAX; i++)
    {
        p->p_racerel[i] = r_neutral;
    }
    for (i = tempval; i < PLAYER_MAX; i++)
    {
        /* setting to r_default means use the race relation */
        p->p_playrel[i] = r_default;
    }

    /* clear out the "realms" */
    for (i = tempval; i < REALM_MAX; i++)
    {
        memset(&p->p_realm[i][0], '\0', REALM_LEN * sizeof(char));
    }

    /* set the default race */
    p->p_race = NO_RACE;

    /* set bit field values */
    p->p_loggedOn = tempval;
    p->p_inChat = tempval;
    p->p_compressed = 1;
    p->p_doingPower = tempval;
    p->p_newPlayer = 1;
    p->p_sendEmail = 0;
    p->p_tmp = tempval;

    /* set the notify type */
    p->p_notify = nt_message;
}

/*
 * assignPlanStats - Builds up the mineral, gold, air, etc. stats based on
 *          the various stats in the sector
 */

void assignPlanStats(Planet_t *pl, UBYTE numPlanets, UBYTE numStars)
{
    ULONG minPct=0, goldPct=0, atmosPct=0, waterPct=0; /* turn of gcc warn */
    USHORT minC=0, goldC=0, atmosC=0, waterC=0; /* turn off gcc warn */

    /* If there aren't very many stars, class A planets are always barren */
    if ((numStars < 3) && (pl->pl_class == pc_A))
    {
        pl->pl_minerals = 0;
        pl->pl_gold = 0;
        pl->pl_gas = 0;
        pl->pl_water = 0;
        return;
    }

    /* otherwise pick the starting values for the different classes */
    switch (pl->pl_class)
    {
        case pc_A:		/* rocky, lifeless, no minerals */
            minPct = impRandom(6);
            minC = 0;
            goldPct = impRandom(6);
            goldC = 0;
            atmosPct = impRandom(35);
            atmosC = 0;
            waterPct = 0;
            waterC = 0;
            break;
        case pc_B:		/* entirely water, no land masses */
            minPct = impRandom(20);
            minC = 0;
            goldPct = 0;
            goldC = 0;
            atmosPct = impRandom(30);
            atmosC = 30;
            waterPct = 0;
            waterC = 100;
            break;
        case pc_C:		/* Gaseous */
            minPct = impRandom(10);
            minC = 0;
            goldPct = impRandom(5);
            goldC = 0;
            atmosPct = 0;
            atmosC = 100;
            waterPct = impRandom(11);
            waterC = 0;
            break;
        case pc_D:		/* Generic */
            minPct = impRandom(61);
            minC = 35;
            goldPct = impRandom(50);
            goldC = 20;
            atmosPct = impRandom(50);
            atmosC = 25;
            waterPct = impRandom(56);
            waterC = 21;
            break;
        case pc_M:		/* Earth-type */
            minPct = impRandom(41);
            minC = 50;
            goldPct = impRandom(61);
            goldC = 20;
            atmosPct = impRandom(36);
            atmosC = 36;
            waterPct = impRandom(41);
            waterC = 36;
            break;
        case pc_N:		/* rocky, good minerals */
            minPct = impRandom(21);
            minC = 80;
            goldPct = impRandom(51);
            goldC = 20;
            atmosPct = impRandom(45);
            atmosC = 6;
            waterPct = impRandom(17);
            waterC = 0;
            break;
        case pc_O:		/* rocky, good gold */
            minPct = impRandom(51);
            minC = 20;
            goldPct = impRandom(21);
            goldC = 80;
            atmosPct = impRandom(45);
            atmosC = 8;
            waterPct = impRandom(24);
            waterC = 0;
            break;
        case pc_Q:		/* Extremely rare, good minerals and gold */
            minPct = impRandom(26);
            minC = 99;
            goldPct = impRandom(26);
            goldC = 99;
            atmosPct = impRandom(17);
            atmosC = 36;
            waterPct = impRandom(17);
            waterC = 46;
            break;
	default:
	    puts(">>> Unexpected default taken in switch()");
            minPct = impRandom(6);
            minC = 0;
            goldPct = impRandom(6);
            goldC = 0;
            atmosPct = impRandom(35);
            atmosC = 0;
            waterPct = 0;
            waterC = 0;
	    break;
    }

    minPct += minC;
    goldPct += goldC;
    atmosPct += atmosC;
    pl->pl_water = waterPct + waterC;
    /* check for multipliers */
    if (numStars > 2)
    {
        /* A lot of stars */
        if (numStars > 4)
        {
            minPct = (minPct * 115) / 100;
            goldPct = (goldPct * 110) / 100;
            /* Stars burn off atmos */
            atmosPct = (atmosPct * 85) / 100;
            waterPct = (waterPct * 70) / 100;
        }
        else
        {
            /* a small amount of stars */
            minPct = (minPct * 105) / 100;
            goldPct = (goldPct * 102) / 100;
            /* Stars burn off atmos */
            atmosPct = (atmosPct * 95) / 100;
            waterPct = (waterPct * 90) / 100;
        }
    }
    /* now check divisors */
    if (numPlanets > 4)
    {
        /* a lot of planets */
        if (numPlanets > 6)
        {
            minPct = (minPct * 70) / 100;
            goldPct = (goldPct * 40) / 100;
        }
        else
        {
            /* a smaller amount of planets */
            minPct = (minPct * 90) / 100;
            goldPct = (goldPct * 80) / 100;
        }
    }
    pl->pl_minerals = minPct;
    pl->pl_gold = goldPct;
    pl->pl_gas = atmosPct;
    pl->pl_water = waterPct;
}

/*
 * initializePlanet - set up the stats for a planet. Will set the mineral
 *          contents for planets, ignores them for stars. Note that
 *          planet class and size must already be set, and that we pass the
 *          sub-row and sub-col for future possible use.
 */

void initializePlanet(Planet_t *pl, USHORT r, USHORT c,
    UBYTE sr, UBYTE sc, ULONG planet, UBYTE numPlanets, UBYTE numStars)
{
    ULONG tempVal;
    USHORT i;

    tempVal = 0;

    pl->pl_bigItems = tempVal;
    pl->pl_lastUpdate = World.w_lastRun;
    pl->pl_transfer = pt_peacefull;

    /* set other race by default */
    pl->pl_ownRace = NO_RACE;

    pl->pl_row = (r * 10) + sr;
    pl->pl_col = (c * 10) + sc;
    pl->pl_number = planet;
    pl->pl_techLevel = tempVal;
    pl->pl_resLevel = tempVal;

    /* clear out planet productions */
    for (i = 0; i <= PPROD_LAST; i++)
    {
        pl->pl_prod[i] = tempVal;
        pl->pl_workPer[i] = tempVal;
    }

    pl->pl_efficiency = 100;
    pl->pl_mobility = tempVal;
    pl->pl_shipCount = tempVal;

    /* clear out planet items */
    for (i = 0; i < IT_LAST + 1; i++)
    {
        pl->pl_quantity[i] = tempVal;
    }
    for (i = 0; i < MAX_WEAP_PL; i++)
    {
        pl->pl_weapon[i] = NO_ITEM;
    }

    pl->pl_plagueStage = tempVal;
    pl->pl_plagueTime = tempVal;
    pl->pl_lastOwner = NO_OWNER;
    pl->pl_owner = NO_OWNER;
    pl->pl_btu = tempVal;
    pl->pl_polution = tempVal;

    memset(&pl->pl_name[0], '\0', PLAN_NAME_LEN * sizeof(char));
    memset(&pl->pl_checkpoint[0], '\0', PASSWORD_LEN * sizeof(char));

    /* decide what to do with the planets other stats */
    switch (pl->pl_class)
    {
        /* if "planet" is a star, just set stats to 0 */
        case pc_S:
            pl->pl_minerals = tempVal;
            pl->pl_gold = tempVal;
            pl->pl_gas = tempVal;
            pl->pl_water = tempVal;
            break;
        /* if the planet is a home planet, use the default values */
        case pc_HOME:
            pl->pl_minerals = 75;
            pl->pl_gold = 75;
            if (MIN_AIR < 71)
            {
                pl->pl_gas = MIN_AIR + 30;
            }
            else
            {
                pl->pl_gas = 100;
            }
            pl->pl_water = 50;
            break;
        /* otherwise build them up based on the various factors */
        default:
            assignPlanStats(pl, numPlanets, numStars);
            break;
    }
}

/*
 * initializeSector - set up the stats for a sector.
 */

void initializeSector(Sector_t *s, USHORT r,
    USHORT c)
{
    s->s_shipCount = 0;
    s->s_planetCount = 0;
    switch(Bits[r]->ba_bit[c] & CELL_TERR)
    {
        case CELL_VACANT:
            s->s_type = s_vacant;
            break;
        case CELL_NORM:
        case CELL_HOME:
            s->s_type = s_normal;
            break;
        case CELL_OTHER:
            if (impRandom(100) < BlkHolePct)
            {
                s->s_type = s_blackhole;
            }
            else
            {
                s->s_type = s_supernova;
            }
            break;
    }

}

/*
 * randomPlanetClass - picks a planet class in the percentages that are
 *          correct
 */
PlanetClass_t randomPlanetClass(void)
{
    USHORT value;

    /* pick a random number between 0 and 999 (0-100 scaled by 10) */
    value = impRandom(1000);
    /* now return a value based on the number */
    if (value > 994)
    {
        return pc_Q; /* .5% */
    }
    if (value > 899)
    {
        return pc_O; /* 10% */
    }
    if (value > 799)
    {
        return pc_N; /* 10% */
    }
    if (value > 599)
    {
        return pc_M; /* 20% */
    }
    if (value > 199)
    {
        return pc_D; /* 40% */
    }
    if (value > 149)
    {
        return pc_C; /* 5% */
    }
    if (value > 99)
    {
        return pc_B; /* 5% */
    }
    return pc_A;     /* 10% */
}

/*
 * addHomeItems - adds the items that should appear on all home planets
 */

void addHomeItems(Planet_t *pl)
{
    pl->pl_transfer = pt_home;
    pl->pl_quantity[it_civilians] = 500;
    pl->pl_quantity[it_military] = 250;
    pl->pl_quantity[it_officers] = 75;
    pl->pl_mobility = 127;
    pl->pl_btu = 96;
}

/*
 * buildSector - Creates all the planets in a sector (if any), and does any
 *          other assignments needed, and writes to the planet file.
 *          Returns FALSE if it encounters an error
 */

BOOL buildSector(FILE *planetFd, Sector_t *s, USHORT grow,
    USHORT gcol)
{
    ULONG totStars, totPlanets;
    USHORT i, curRace, holdRace=99;
    UBYTE subrow, subcol;
    UBYTE subsect[11][11];
    static Planet_t pl;
    BOOL needHome;

    /* check for a vacant sector */
    if ((s->s_type == s_vacant) || (s->s_type == s_blackhole) ||
        (s->s_type == s_supernova))
    {
        /* since sector can not hold any other items, return true */
        return TRUE;
    }
    /* clear out our array */
    for (subrow = 0; subrow < 10; subrow++)
    {
        for (subcol = 0; subcol < 10; subcol++)
        {
            subsect[subrow][subcol] = 0;
        }
    }
    /* Build up the star(s) in the sector */
    if ((Bits[grow]->ba_bit[gcol] & CELL_HISTAR) == 0)
    {
        /* Just a normal amount of stars in the sector */
        totStars = impRandom(2) + 1;
        i = 0;
        while(i < totStars)
        {
            /* pick a random sub row and sub col */
            subrow = impRandom(10);
            subcol = impRandom(10);
            /* we check to make sure the subsector is free */
            if (subsect[subrow][subcol] == 0)
            {
                pl.pl_class = pc_S;
                pl.pl_size = (impRandom(LargeStarSize - SmallStarSize) +
                    SmallStarSize);
                initializePlanet(&pl, grow, gcol, subrow, subcol,
                    w->w_planetNext, 0, totStars);
                if (fwrite((void *)&pl, sizeof(char), sizeof(Planet_t),
		    planetFd) != sizeof(Planet_t))
                {
		    printf("Write of planet %lu failed!\n",
			w->w_planetNext);
                    return FALSE;
                }
                /* mark the subsector as used */
                subsect[subrow][subcol] = 1;
                s->s_planet[s->s_planetCount] = w->w_planetNext;
                w->w_planetNext++;
                s->s_planetCount++;
                i++;
            }
        }
    }
    else
    {
        /* Sector has a high amount of stars */
        totStars = (impRandom(3) + 3);
        i = 0;
        while(i < totStars)
        {
            subrow = impRandom(10);
            subcol = impRandom(10);
            if (subsect[subrow][subcol] == 0)
            {
                pl.pl_class = pc_S;
                pl.pl_size = (impRandom(LargeStarSize - SmallStarSize) +
                    SmallStarSize);
                initializePlanet(&pl, grow, gcol, subrow, subcol,
                    w->w_planetNext, 0, totStars);
                if (fwrite((void *)&pl, sizeof(char), sizeof(Planet_t),
		    planetFd) != sizeof(Planet_t))
                {
		    printf("Write of planet %lu failed!\n",
			w->w_planetNext);
                    return FALSE;
                }
                subsect[subrow][subcol] = 1;
                s->s_planet[s->s_planetCount] = w->w_planetNext;
                w->w_planetNext++;
                s->s_planetCount++;
                i++;
            }
        }
    }
    /* Check if there are any planets in this sector */
    if ((Bits[grow]->ba_bit[gcol] & (CELL_PLAN | CELL_HIPLAN)) == 0)
    {
        /* No planets, so we are done */
        return TRUE;
    }
    /* check if we need a home planet in this sector */
    if ((Bits[grow]->ba_bit[gcol] & CELL_HOME) != 0)
    {
        needHome = TRUE;
    }
    else
    {
        needHome = FALSE;
    }
    /* check for a high number of planets */
    if ((Bits[grow]->ba_bit[gcol] & CELL_HIPLAN) == 0)
    {
        /* Just a normal amount of planets in the sector */
        totPlanets = impRandom(5) + 1;
        i = 0;
        while(i < totPlanets)
        {
            subrow = impRandom(10);
            subcol = impRandom(10);
            if (subsect[subrow][subcol] == 0)
            {
                if (needHome)
                {
                    pl.pl_class = pc_HOME;
                    needHome = FALSE;
                    curRace = 0;
                    /* Find the race that this home planet belongs to */
                    while (curRace < RACE_MAX)
                    {
                        if (((w->w_race[curRace].r_homeRow / 10) == grow) &&
                            ((w->w_race[curRace].r_homeCol / 10) == gcol))
                        {
                            w->w_race[curRace].r_homeRow += subrow;
                            w->w_race[curRace].r_homeCol += subcol;
                            w->w_race[curRace].r_homePlanet = w->w_planetNext;
                            w->w_race[curRace].r_planetCount = 1;
                            holdRace = curRace;
                            curRace = RACE_MAX;
                        }
                        curRace++;
                    }
                    /* set the size to the chosen size */
                    pl.pl_size = HomePlanSize;
                    /* build up the default planet stats */
                    initializePlanet(&pl, grow, gcol, subrow, subcol,
                        w->w_planetNext, totPlanets, totStars);
                    /* set the override values */
                    strcpy(&pl.pl_name[0], &finalname[holdRace][0]);
                    addHomeItems(&pl);
                    pl.pl_ownRace = holdRace;
                }
                else
                {
                    pl.pl_class = randomPlanetClass();
                    pl.pl_size = (impRandom(LargePlanetSize - SmallPlanetSize) +
                        SmallPlanetSize);
                    initializePlanet(&pl, grow, gcol, subrow, subcol,
                        w->w_planetNext, totPlanets, totStars);
                }
                if (fwrite((void *)&pl, sizeof(char), sizeof(Planet_t),
		    planetFd) != sizeof(Planet_t))
                {
		    printf("Write of planet %lu failed!\n",
			w->w_planetNext);
                    return FALSE;
                }
                subsect[subrow][subcol] = 1;
                s->s_planet[s->s_planetCount] = w->w_planetNext;
                w->w_planetNext++;
                s->s_planetCount++;
                i++;
            }
        }
    }
    else
    {
        /* High number of planets in the sector */
        totPlanets = impRandom(4) + 6;
        i = 0;
        /* we need to check that we don't allow more than PLANET_MAX */
        /* planets to the system */
        while((i < totPlanets) && ((i + totStars) < PLANET_MAX))
        {
            subrow = impRandom(10);
            subcol = impRandom(10);
            if (subsect[subrow][subcol] == 0)
            {
                if (needHome)
                {
                    pl.pl_class = pc_HOME;
                    needHome = FALSE;
                    curRace = 0;
                    /* Find the race that this home planet belongs to */
                    while (curRace < RACE_MAX)
                    {
                        if (((w->w_race[curRace].r_homeRow / 10) == grow) &&
                            ((w->w_race[curRace].r_homeCol / 10) == gcol))
                        {
                            w->w_race[curRace].r_homeRow += subrow;
                            w->w_race[curRace].r_homeCol += subcol;
                            w->w_race[curRace].r_homePlanet = w->w_planetNext;
                            w->w_race[curRace].r_planetCount = 1;
                            holdRace = curRace;
                            curRace = RACE_MAX;
                        }
                        curRace++;
                    }
                    /* set the size to the chosen size */
                    pl.pl_size = HomePlanSize;
                    /* build up the default planet stats */
                    initializePlanet(&pl, grow, gcol, subrow, subcol,
                        w->w_planetNext, totPlanets, totStars);
                    /* set the override values */
		    if (holdRace == 99)
		    {
			puts("holdRace was never set!");
			return FALSE;
		    }
                    strcpy(&pl.pl_name[0], &finalname[holdRace][0]);
                    addHomeItems(&pl);
                    pl.pl_ownRace = holdRace;
                }
                else
                {
                    pl.pl_class = randomPlanetClass();
                    pl.pl_size = (impRandom(LargePlanetSize - SmallPlanetSize) +
                        SmallPlanetSize);
                    initializePlanet(&pl, grow, gcol, subrow, subcol,
                        w->w_planetNext, totPlanets, totStars);
                }
                if (fwrite((void *)&pl, sizeof(char), sizeof(Planet_t),
		    planetFd) != sizeof(Planet_t))
                {
		    printf("Write of planet %lu failed!\n",
			w->w_planetNext);
                    return FALSE;
                }
                subsect[subrow][subcol] = 1;
                s->s_planet[s->s_planetCount] = w->w_planetNext;
                w->w_planetNext++;
                s->s_planetCount++;
                i++;
            }
        }
    }
    return TRUE;
}

/*
 * initializeRace - set up the stats for a race.
 */

void initializeRace(Race_t *r)
{

    memset(&r->r_name[0], '\0', NAME_LEN * sizeof(char));
    memset(&r->r_homeName[0], '\0', PLAN_NAME_LEN * sizeof(char));
    r->r_homePlanet = NO_ITEM;
    r->r_status = rs_notyet;
    r->r_planetCount = 0;
    r->r_techLevel = 0;
    r->r_resLevel = 0;
    r->r_playCount = 0;

}

/*
 * cleanup - free our resources, then exit with the given status.
 */

void cleanup(int status)
{
    USHORT i;

    /* Free up the bit arrays */
    for (i = 0; i < w->w_rows; i++)
    {
        if (Bits[i])
        {
            free(Bits[i]);
            Bits[i] = NULL;
        }
    }
    (void) umask(oldUmask);
    exit(status);
}

/*
 * zapFile - delete and recreate the file whose name is in FileName.
 */

void zapFile(void)
{
    FILE *fd;

    (void) unlink(&FileName[0]);
    fd = fopen(&FileName[0], "wb");
    if (fd == NULL)
    {
        printf("Can't create %s.\n", &FileName[0]);
        cleanup(20);
    }
    fclose(fd);
}

/*
 * newFile - delete and recreate the next file name in the name file.
 */

void newFile(char *defName)
{
    strcpy(&FileName[0], defName);
    zapFile();
}

/*
 * newEmptyFile - set up a new file that starts off empty.
 */

void newEmptyFile(char *defName)
{
    newFile(defName);
    printf("Cleared %s.\n", &FileName[0]);
}

/*
 * clearFile - delete a file and say we are doing so.
 */

void clearFile(char *fileName)
{
    strcpy(&FileName[0], &Path[0]);
    strcat(&FileName[0], fileName);
    printf("Deleting %s:\n", &FileName[0]);
    (void) unlink(&FileName[0]);
}

/*
 * writeFiles - create and write the Imperium data files.
 */

void writeFiles(void)
{
    FILE *fd, *planetFd;
    Sector_t s;
    USHORT i, j;
    char sectorFile[258];

    /* log file */
    (void) unlink(gbLogFile);

    /* Player file */
    newFile(gbPlayerFile);
    fd = fopen(&FileName[0], "r+b");
    if (fd == NULL)
    {
        cleanup(20);
    }
    printf("Writing %s:\n", &FileName[0]);
    for (i = 0; i < PLAYER_MAX; i++)
    {
        initializePlayer(i);
    }
    Player[0].p_status = ps_deity;
    Player[0].p_timeLeft = 999;
    strcpy(&Player[0].p_name[0], "god");
    strcpy(&Player[0].p_password[0], gbGodPassword);
    if (fwrite((void *)&Player, sizeof(char), sizeof(Player_t) * PLAYER_MAX,
	fd) != (sizeof(Player_t) * PLAYER_MAX))
    {
        fclose(fd);
        cleanup(20);
    }
    fclose(fd);

    for (i = 0; i < RACE_MAX; i++)
    {
        strcpy(&World.w_race[i].r_name[0], RaceName[i]);
        strcpy(&finalname[i][0], PlanName[i]);
        strcpy(&World.w_race[i].r_homeName[0], &finalname[i][0]);
    }
    memset(&w->w_winName[0], '\0', NAME_LEN * sizeof(char));
    strcpy(&World.w_winName[0], gbLastWinner);
    /* sector and planet file */
    newFile(gbSectorsFile);
    fd = fopen(&FileName[0], "r+b");
    if (fd == 0)
    {
        cleanup(20);
    }
    strcpy(&sectorFile[0], &FileName[0]);
    newFile(gbPlanetsFile);
    planetFd = fopen(&FileName[0], "r+b");
    if (planetFd == NULL)
    {
        fclose(fd);
        cleanup(20);
    }
    printf("Writing %s and %s:\n", &sectorFile[0], &FileName[0]);
    for (i = 0; i < w->w_rows; i++)
    {
        for (j = 0; j < w->w_columns; j++)
        {
            initializeSector(&s, i, j);
            if (buildSector(planetFd, &s, i, j))
            {
                if (fwrite((void *)&s, sizeof(char), sizeof(Sector_t),
		    fd) != sizeof(Sector_t))
                {
                    fclose(fd);
                    fclose(planetFd);
		    printf("Write of sector %u, %u failed\n", i, j);
                    cleanup(20);
                }
            }
            else
            {
                fclose(fd);
                fclose(planetFd);
		printf("Buildsector failed for %u, %u\n", i, j);
                cleanup(20);
            }
        }
    }
    fclose(planetFd);
    fclose(fd);
    printf("Created %lu planets and stars\n", World.w_planetNext - 1);

    /* world file */
    newFile(gbWorldFile);
    printf("Writing %s:\n", &FileName[0]);
    fd = fopen(&FileName[0], "r+b");
    if (fd == NULL)
    {
        cleanup(20);
    }
    strcpy(&World.w_password[0], gbCreationPassword);
    strcpy(&World.w_gameName[0], gbGameName);
    strcpy(&World.w_gameHost[0], gbGameHost);
    strcpy(&World.w_gamePort[0], gbGamePort);
    strcpy(&World.w_emailAddress[0], gbEmailAddress);
    if (fwrite((void *)&World, sizeof(char), sizeof(World_t),
	fd) != sizeof(World_t))
    {
        fclose(fd);
        cleanup(20);
    }
    fclose(fd);

    /* ship file */
    newEmptyFile(gbShipFile);

    /* fleet file */
    newEmptyFile(gbFleetFile);

    /* loan file */
    newEmptyFile(gbLoanFile);

    /* offer file */
    newEmptyFile(gbOfferFile);

    /* offer file */
    newEmptyFile(gbBigItemFile);

    clearFile(POWER_FILE);

    puts("Writing telegram files:");
    for (i = 0; i < World.w_maxPlayers; i++)
    {
    	sprintf(&FileName[0], "%stelegrams.%03.3u", &Path[0], i);
        zapFile();
    }
    puts("\nImperium universe created!\n");
}

/*
 * prevRow - return previous row-index, or 0xFFFF if at first row.
 */

USHORT prevRow(USHORT row)
{
    if ((row == 0) || (row == 0xFFFF))
    {
        return(0xFFFF);
    }
    return(row - 1);
}

/*
 * prevCol - return previous col-index, or 0xFFFF if at first col.
 */

USHORT prevCol(USHORT col)
{
    if ((col == 0) || (col == 0xFFFF))
    {
        return(0xFFFF);
    }
    return(col - 1);
}

/*
 * nextRow - return next row-index, or 0xFFFF if last row.
 */

USHORT nextRow(USHORT row)
{
    if ((row == World.w_rows - 1) || (row == 0xFFFF))
    {
        return(0xFFFF);
    }
    return(row + 1);
}

/*
 * nextCol - return next col-index, or 0xFFFF if last col.
 */

USHORT nextCol(USHORT col)
{
    if ((col == World.w_columns - 1) || (col == 0xFFFF))
    {
        return(0xFFFF);
    }
    return(col + 1);
}

/*
 * isNorm - return 'true' if the indicated sector is "normal" and has at least
 *          one planet.
 */

BOOL isNorm(USHORT r, USHORT c)
{
    if ((r == 0xFFFF) || (c == 0xFFFF))
    {
        return FALSE;
    }
    return(((Bits[r]->ba_bit[c] & CELL_TERR) == CELL_NORM) &&
        ((Bits[r]->ba_bit[c] & (CELL_PLAN | CELL_HIPLAN)) != 0));
}

/*
 * getBits - safely gets the bits for row & col arguments that may be out of
 *          bounds (0xFFFF). If either row or col is invalid, returns "0".
 */
UBYTE getBits(USHORT r, USHORT c)
{
    if ((r == 0xFFFF) || (c == 0xFFFF))
    {
        return(0);
    }
    return(Bits[r]->ba_bit[c]);
}

/*
 * findHomePlanets - find free spaces to put home planets, and tries to be
 *          as fair about planet placement as the user requested.
 */

BOOL findHomePlanets(void)
{
    USHORT r, c, rt, ct;
    ULONG planCount, count;
    USHORT tryOuter, tryInner, i, j, distance, d1, d2;
    USHORT rp, rn;
    ULONG planMin, planMax;
    short r2, c2;
    BOOL failed, tooClose, addedOne, doIt;

    for (i = 0; i < RACE_MAX; i++)
    {
        initializeRace(&w->w_race[i]);
    }
    tryOuter = 0;
    doIt = TRUE;
    while (doIt)
    {
        failed = FALSE;
        planMin = 0xFFFF;
        planMax = 0;
        i = 0;
        while ((i < RACE_MAX) && !failed)
        {
            tryInner = 0;
            tooClose = TRUE;
            while (tooClose && (tryInner < MAX_HOME_TRIES_INNER))
            {
                tooClose = FALSE;
                r = impRandom(w->w_rows);
                c = impRandom(w->w_columns);
/* #1 : home planets must be CELL_NORM */
                if (isNorm(r, c))
                {
                    for (j = 0; j < i; j++)
                    {
/* #2 : home planets must be at least MinDist galactic sectors apart */
                        d1 = (r + w->w_rows - (w->w_race[j].r_homeRow / 10))
                            % w->w_rows;
                        d2 = ((w->w_race[j].r_homeRow / 10) + w->w_rows - r)
                            % w->w_rows;
                        if (d1 < d2)
                        {
                            rt = d1;
                        }
                        else
                        {
                            rt = d2;
                        }
                        d1 = (c + w->w_columns - (w->w_race[j].r_homeCol / 10))
                            % w->w_columns;
                        d2 = ((w->w_race[j].r_homeCol / 10) + w->w_columns - c)
                            % w->w_columns;
                        if (d1 < d2)
                        {
                            ct = d1;
                        }
                        else
                        {
                            ct = d2;
                        }
                        if (((rt * rt) + (ct * ct)) < (MinDist * MinDist))
                        {
                            tooClose = TRUE;
                        }
                        else
                        {
/* #3 : home planets must have at least MinNear neighboring CELL_NORM */
                            rp = prevRow(r);
                            rn = nextRow(r);
                            count = 0;
                            if (isNorm(rp, prevCol(c)))
                            {
                                count++;
                            }
                            if (isNorm(r, prevCol(c)))
                            {
                                count++;
                            }
                            if (isNorm(rn, prevCol(c)))
                            {
                                count++;
                            }
                            if (isNorm(rp, c))
                            {
                                count++;
                            }
                            if (isNorm(rn, c))
                            {
                                count++;
                            }
                            if (isNorm(rn, nextCol(c)))
                            {
                                count++;
                            }
                            if (isNorm(r, nextCol(c)))
                            {
                                count++;
                            }
                            if (isNorm(rp, nextCol(c)))
                            {
                                count++;
                            }
                            if (count < MinNear)
                            {
                                tooClose = TRUE;
                            }
                        }
                    }
                }
                else
                {
                    tooClose = TRUE;
                }
                tryInner++;
#ifdef FOO
                if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C != 0)
                {
                    tryInner = MAX_HOME_TRIES_INNER;
                }
#endif
            }
            if (tooClose)
            {
                puts("Home planets too close together, or too few "
                    "neighboring 'normal' sectors.");
                failed = TRUE;
            }
            else
            {
/* #4 : home planets must have private access to MinReachable "normal" sects
    within MaxReach distance */
                Bits[r]->ba_bit[c] = (Bits[r]->ba_bit[c] | CELL_REACH);
                count = 2;
                addedOne = TRUE;
                distance = 1;
                planCount = 0;
                while ((count < MinReachable) && addedOne &&
                    (distance <= MaxReach))
                {
                    addedOne = FALSE;
                    for (r2 = (r - distance); r2 <= (r + distance); r2++)
                    {
                        if (r2 < 0)
                        {
                            rt = (r2 + w->w_rows);
                        }
                        else
                        {
                            if (r2 >= (short)w->w_rows)
                            {
                                rt = (r2 - w->w_rows);
                            }
                            else
                            {
                                rt = r2;
                            }
                        }
                        for (c2 = (c - distance); c2 <= (c + distance); c2++)
                        {
                            if (c2 < 0)
                            {
                                ct = (c2 + w->w_columns);
                            }
                            else
                            {
                                if (c2 >= (short)w->w_columns)
                                {
                                    ct = (c2 - w->w_columns);
                                }
                                else
                                {
                                    ct = c2;
                                }
                            }
                            if (isNorm(rt, ct) &&
                                ((getBits(rt, ct) & (CELL_REACH | CELL_TAKEN))
                                == 0) && (((getBits(prevRow(rt), prevCol(ct)) &
                                CELL_REACH) != 0) ||
                                ((getBits(prevRow(rt), ct) & CELL_REACH) != 0)
                                || ((getBits(prevRow(rt), nextCol(ct)) &
                                CELL_REACH) != 0) || ((getBits(rt, prevCol(ct))
                                & CELL_REACH) != 0) ||
                                ((getBits(rt, nextCol(ct)) & CELL_REACH) != 0)
                                || ((getBits(nextRow(rt), prevCol(ct)) &
                                CELL_REACH) != 0) || ((getBits(nextRow(rt), ct)
                                & CELL_REACH) != 0) ||
                                ((getBits(nextRow(rt), nextCol(ct)) & CELL_REACH)
                                != 0)))
                            {
                                Bits[rt]->ba_bit[ct] = (Bits[rt]->ba_bit[ct] |
                                    CELL_REACH);
                                count++;
                                addedOne = TRUE;
                                if (Bits[rt]->ba_bit[ct] & CELL_PLAN)
                                {
                                    planCount++;
                                }
                                /* If the sector is a high plan, count as 2 */
                                if (Bits[rt]->ba_bit[ct] & CELL_HIPLAN)
                                {
                                    planCount++;
                                }
                            }
                        }
                    }
                    distance++;
                }
                if (count < MinReachable)
                {
                    puts("Too few unclaimed reachable vacant planets.");
                    failed = TRUE;
                }
                else
                {
                    if (planCount < planMin)
                    {
                        planMin = planCount;
                    }
                    if (planCount > planMax)
                    {
                        planMax = planCount;
                    }
                }
                distance--;
                for (r2 = (r - distance); r2 <= (r + distance); r2++)
                {
                    if (r2 < 0)
                    {
                        rt = (r2 + w->w_rows);
                    }
                    else
                    {
                        if (r2 >= (short)w->w_rows)
                        {
                        rt = (r2 - w->w_rows);
                        }
                        else
                        {
                        rt = r2;
                        }
                    }
                    for (c2 = (c - distance); c2 <= (c + distance); c2++)
                    {
                        if (c2 < 0)
                        {
                            ct = (c2 + w->w_columns);
                        }
                        else
                        {
                            if (c2 >= (short)w->w_columns)
                            {
                            ct = (c2 - w->w_columns);
                            }
                            else
                            {
                            ct = c2;
                            }
                        }
                        if (Bits[rt]->ba_bit[ct] & CELL_REACH)
                        {
                            if (failed)
                            {
                                Bits[rt]->ba_bit[ct] = (Bits[rt]->ba_bit[ct] &
                                    ~CELL_REACH);
                            }
                            else
                            {
                                Bits[rt]->ba_bit[ct] = ((Bits[rt]->ba_bit[ct] &
                                    ~CELL_REACH) | CELL_TAKEN);
                            }
                        }
                    }
                }
                if (!failed)
                {
                    Bits[r]->ba_bit[c] = ((Bits[r]->ba_bit[c] & ~CELL_TERR) |
                        CELL_HOME);
                    w->w_race[i].r_homeRow = (r * 10);
                    w->w_race[i].r_homeCol = (c * 10);
                    i++;
                }
            }
        }
        if (!failed && (tryOuter < MAX_HOME_TRIES_OUTER))
        {
            if ((planMax - planMin) > MaxSpread)
            {
                printf("Planet spread too large - %lu vs %lu - "
                    "retrying\n", planMin, planMax);
                failed = TRUE;
            }
        }
#ifdef FOO
        if ((SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) != 0)
        {
            tryOuter = MAX_HOME_TRIES_OUTER;
        }
#endif
        doIt = (failed && (tryOuter != MAX_HOME_TRIES_OUTER));
        if (doIt)
        {
            /* need to undo any initial home planets placed before failure */
            for (r = 0; r < w->w_rows; r++)
            {
                for (c = 0; c < w->w_columns; c++)
                {
                    if (isNorm(r, c) || ((Bits[r]->ba_bit[c] & CELL_TERR) ==
                        CELL_HOME))
                    {
                        Bits[r]->ba_bit[c] = ((Bits[r]->ba_bit[c] & ~(CELL_TERR
                            | CELL_TAKEN)) | CELL_NORM);
                    }
                }
            }
            tryOuter++;
            if (tryOuter < MAX_HOME_TRIES_OUTER)
            {
                puts("Home planet placement failed, trying again.");
            }
        }
    }
    return (tryOuter != MAX_HOME_TRIES_OUTER);
}



/*
 * build - main routine of world building.
 */

short build(void)
{
    USHORT i, j;
    char buff[512];

    OutputLinePos = 0;
    w = &World;

    puts("Building world");

    /* initialize adjustable parameters */
    w->w_rows = gbVertSize;
    w->w_columns = gbHorizSize;
    SqSize = w->w_rows * w->w_columns;
    StarAmt = (SqSize * StarPer) / 100;
    MultStarAmt = (SqSize * MultStarPer) / 100;
    PlanAmt = (SqSize * PlanPer) / 100;
    MultPlanAmt = (SqSize * MultPlanPer) / 100;
    OthAmt = (SqSize * OtherSect) / 100;
    if (SqSize < 101)
    {
        i = 8;
    }
    else
    {
        if (SqSize < 901)
        {
            i = 14;
        }
        else
        {
            if (SqSize < 4097)
            {
                i = 30;
            }
            else
            {
                if (SqSize < 16385)
                {
                    i = 62;
                }
                else
                {
                    i = PLAYER_MAX;
                }
            }
        }
    }
    w->w_maxPlayers = gbNumPlay;
    w->w_maxConnect = gbConTime;
    w->w_secondsPerITU = gbSecItu;
    if (gbPublicMessages)
    {
       w->w_sendAll = 1;
    }
    else
    {
       w->w_sendAll = 0;
    }
    if (gbChangePlayers)
    {
       w->w_chaPlay = 1;
    }
    else
    {
       w->w_chaPlay = 0;
    }

    /* By default, do NOT flush buffers after logging off  */
    w->w_doFlush = 0;
    /* By default, allow normal users to use the flush cmd */
    w->w_userFlush = 1;
    /* By default, allow users to force power update */
    w->w_nonDeityPower = 1;
    /* By default, allow people to change their names */
    w->w_noNameChange = 0;
    /* no one is doing a power report now... */
    w->w_doingPower = 0;
    /* you must know the creation password to make a new player */
    w->w_noCreatePass = 0;

    /* clear out the other unused flags */
    w->w_keepRest = 0;

    /* initialize and default various fields in the the World_t structure */

    (void) time(&w->w_lastRun);             /* last time Imperium run   */
    w->w_buildDate = w->w_lastRun;
    w->w_currPlayers = 1;                   /* only deity so far        */
    w->w_maxBTUs = 96;

    w->w_loanNext = 0;                      /* none of these so far     */
    w->w_offerNext = 0;
    w->w_shipNext = 0;
    w->w_fleetNext = 0;
    w->w_planetNext = 0;
    w->w_bigItemNext = 0;

    w->w_resCost = 25;
    w->w_techCost = 25;
    w->w_missCost = 2;
    w->w_planeCost = 25;
    w->w_barCost = 5;
    w->w_airCost = 15;
    w->w_fuelCost = 20;

    w->w_defMob = 1;

    w->w_plagueKiller = 227;
    w->w_plagueBooster = 100;
    w->w_plagueOneBase = 32;
    w->w_plagueOneRand = 33;
    w->w_plagueTwoBase = 32;
    w->w_plagueTwoRand = 33;
    w->w_plagueThreeBase = 32;
    w->w_plagueThreeRand = 33;

    w->w_efficCost = 1;
    w->w_milSuppliesCost = 8;
    w->w_ofcSuppliesCost = 12;
    w->w_utilityRate = 1;
    w->w_interestRate = 12;
    w->w_shipCostMult = 9;
    w->w_refurbCost = 1;

    w->w_hullScale = 100;
    w->w_engineScale = 100;
    w->w_resScale = 100;
    w->w_techScale = 100;
    w->w_defenseScale = 100;
    w->w_missScale = 100;
    w->w_planeScale = 100;
    w->w_goldScale = 100;
    w->w_ironScale = 100;
    w->w_barScale = 100;
    w->w_shipWorkScale = 100;

    w->w_efficScale = 100;
    w->w_mobilScale = 100;
    w->w_highGrowthFactor = 200;
    w->w_lowGrowthFactor = 400;
    w->w_BTUDivisor = 5000;
    w->w_resDecreaser = 10;
    w->w_techDecreaser = 10;

    w->w_assAdv = 125;
    w->w_boardAdv = 125;

    w->w_torpCost = 3;
    w->w_torpMobCost = 5;
    w->w_torpAcc = 90;
    w->w_torpBase = 21;
    w->w_torpRand = 38;
    w->w_phaserAcc = 100;
    w->w_phaserRange = 40;
    w->w_phaserDmg = 5;
    w->w_mineBase = 20;
    w->w_mineRand = 10;

    w->w_fuelTankSize = 32;
    w->w_fuelRichness = 4;
    w->w_flakFactor = 7;
    w->w_landScale = 100;
    w->w_bombBase = 13;
    w->w_bombRand = 10;
    w->w_planeBase = 7;
    w->w_planeRand = 10;

    w->w_contractScale = 100;
    w->w_deathFactor = 15;
    w->w_gunMax = 12;
    w->w_gunScale = 100;
    w->w_lookShipFact = 2036;
    w->w_collectScale = 100;
    w->w_radarFactor = 61;
    w->w_spyFactor = 150;
    w->w_armourWeight = 5;
    w->w_armourPoints = 1;
    w->w_shipTechDecreaser = 10;

    w->w_nonDeityPower = 1;

    for (i = 0; i <= ST_LAST; i++)
    {
        w->w_shipCargoLim[i] = SHIP_CARGO[i];
        w->w_shipCost[i] = SHIP_COST[i];
        w->w_baseFuelCost[i] = SHIP_NAVCOST[i];
    }
    for (i = 0; i <= IT_LAST; i++)
    {
        w->w_mobCost[i] = MOB_COST[i];
        w->w_weight[i] = IT_WEIGHT[i];
    }

    for (i = 0; i <= PPROD_LAST; i++)
    {
        w->w_prodCost[i] = PROD_COST[i];
    }

    /* now go create the world */

    puts("Growing terrain:");
    for (i = 0; i < w->w_rows; i++)
    {
        Bits[i] = NULL;
    }
    for (i = 0; i < w->w_rows; i++)
    {
        Bits[i] = calloc(1, sizeof(BitArray_t));
        if (Bits[i] == NULL)
        {
            puts("Can't allocate memory for 'Bits' - aborting.");
            cleanup(20);
        }
        for (j = 0; j < w->w_columns; j++)
        {
            Bits[i]->ba_bit[j] = 0;
        }
    }
    if (growSpace())
    {
        puts("Placing home planets:");
        if (findHomePlanets())
        {
            for (i = 0; i < w->w_rows; i++)
            {
                for (j = 0; j < w->w_columns; j++)
                {
                    switch (Bits[i]->ba_bit[j] & CELL_TERR)
                    {
                        case CELL_OTHER:
                            buff[j] = '@';
                            break;
                        case CELL_VACANT:
                            buff[j] = ' ';
                            break;
                        case CELL_HOME:
                            if (Bits[i]->ba_bit[j] & CELL_HIPLAN)
                            {
                                if (Bits[i]->ba_bit[j] & CELL_HISTAR)
                                {
                                    buff[j] = 'X';
                                }
                                else
                                {
				    /* Doing it this way to get around */
				    /* problem with metaconfig */
                                    buff[j] = '\x50';
                                }
                            }
                            else
                            {
                                if (Bits[i]->ba_bit[j] & CELL_HISTAR)
                                {
                                    buff[j] = 'S';
                                }
                                else
                                {
                                    buff[j] = 'H';
                                }
                            }
                            break;
                        case CELL_NORM:
                            if (Bits[i]->ba_bit[j] & CELL_HIPLAN)
                            {
                                if (Bits[i]->ba_bit[j] & CELL_HISTAR)
                                {
                                    buff[j] = 'x';
                                }
                                else
                                {
                                    buff[j] = 'p';
                                }
                            }
                            else
                            {
                                if (Bits[i]->ba_bit[j] & CELL_HISTAR)
                                {
                                    buff[j] = 's';
                                }
                                else
                                {
                                    buff[j] = ':';
                                }
                            }
                            break;
                    }
                }
                buff[j] = '\0';
                puts(&buff[0]);
            }
            return TRUE;
        }
        puts("Can't place home planets - giving up.");
        return FALSE;
    }
    puts("Can't grow terrain - giving up.");
    return FALSE;
}

void fixupStr(char *str)
{
    USHORT len;

    len = strlen(str);
    if (str[len - 1] == '\n')
    {
        str[len - 1] = '\0';
    }
}

void getEnvDef(char *dest, const char *envVar, const char *defVal)
{
    if (getenv(envVar) == NULL)
    {
	strcpy(dest, defVal);
    }
    else
    {
	strcpy(dest, getenv(envVar));
    }
}

/*
 * The call-back point to start building a world. Will first try and read
 * in a config file that was generated by ImpCre. This file will be removed
 * when we are done with it.
 */

short startGenerate(void)
{
    char locBuf[120];
    UBYTE race;
    long Seed;          /* seed for random number generator */

    puts("Reading in creation parameters");

    getEnvDef(gbWorldFile, "WorldFile", "imp.world");

    /* read in the PlanetsFile now */
    getEnvDef(gbPlanetsFile, "PlanetsFile", "imp.planet");

    /* read in the SectorsFile now */
    getEnvDef(gbSectorsFile, "SectorsFile", "imp.sector");

    /* read in the BigItemFile now */
    getEnvDef(gbBigItemFile, "BigItemFile", "imp.bigitem");

    /* read in the PlayerFile now */
    getEnvDef(gbPlayerFile, "PlayerFile", "imp.player");

    /* read in the ShipFile now */
    getEnvDef(gbShipFile, "ShipFile", "imp.ship");

    /* read in the FleetFile now */
    getEnvDef(gbFleetFile, "FleetFile", "imp.fleet");

    /* read in the LoanFile now */
    getEnvDef(gbLoanFile, "LoanFile", "imp.loan");

    /* read in the OfferFile now */
    getEnvDef(gbOfferFile, "OfferFile", "imp.offer");

    /* read in the LogFile now */
    getEnvDef(gbLogFile, "LogFile", "imp.log");

    /* read in the LastWinner now */
    getEnvDef(gbLastWinner, "LastWinner", "");

    /* read in the server email address */
    getEnvDef(gbEmailAddress, "EmailAddr", "changeme@foobar.com");

    /* read in the GodPassword now */
    getEnvDef(gbGodPassword, "GodPassword", "imperium");

    /* read in the CreationPassword now */
    getEnvDef(gbCreationPassword, "CreationPassword", "add.me");

    /* read in game name */
    getEnvDef(gbGameName, "Name", "Default Server");

    /* read in game host */
    getEnvDef(gbGameHost, "Host", "change.me");

    /* read in game port */
    getEnvDef(gbGamePort, "Port", "3458");

    /* now handle the race names and planets */
    for (race = 1; race < 8; race++)
    {
        /* read in the race's name now */
	(void) sprintf(&locBuf[0], "Race%dName", race);
	getEnvDef(&RaceName[race - 1][0], &locBuf[0], DEF_RACE[race - 1]);

        /* read in the race's planet now */
	(void) sprintf(&locBuf[0], "Race%dPlanet", race);
	getEnvDef(&PlanName[race - 1][0], &locBuf[0], DEF_PLAN[race - 1]);
    }
    /* read in the random number seed now */
    getEnvDef(&locBuf[0], "RandomSeed", "0");
    Seed = atol(&locBuf[0]);
    if (Seed == 0)
    {
	Seed = (getpid() + time(NULL)) / 3;
    }
    srand48(Seed);

    /* read in the horiz size now */
    getEnvDef(&locBuf[0], "HorizSize", "10");
    gbHorizSize = atol(&locBuf[0]);
    /* read in the vert size now */
    getEnvDef(&locBuf[0], "VertSize", "10");
    gbVertSize = atol(&locBuf[0]);

    /* read in the PctOth now */
    getEnvDef(&locBuf[0], "PctOth", "5");
    OtherSect = atoi(&locBuf[0]);
    /* read in the PctBH now */
    getEnvDef(&locBuf[0], "PctBH", "50");
    BlkHolePct = atoi(&locBuf[0]);
    /* read in NumPlay now */
    getEnvDef(&locBuf[0], "NumPlay", "6");
    gbNumPlay = atol(&locBuf[0]);
    /* read in ConTime now */
    getEnvDef(&locBuf[0], "ConTime", "60");
    gbConTime = atol(&locBuf[0]);
    /* read in the SecItu now */
    getEnvDef(&locBuf[0], "SecItu", "1800");
    gbSecItu = atol(&locBuf[0]);
    /* read in StartingCash now */
    getEnvDef(&locBuf[0], "StartingCash", "5000");
    InitialMoney = atol(&locBuf[0]);
    /* read in the MinPlSpc now */
    getEnvDef(&locBuf[0], "MinPlSpc", "3");
    MinDist = atol(&locBuf[0]);
    /* read in the MinPlAdj now */
    getEnvDef(&locBuf[0], "MinPlAdj", "3");
    MinNear = atol(&locBuf[0]);
    /* read in the MinUnclaimed now */
    getEnvDef(&locBuf[0], "MinUnclaimed", "5");
    MinReachable = atol(&locBuf[0]);
    /* read in the MaxDist now */
    getEnvDef(&locBuf[0], "MaxReach", "15");
    MaxReach = atol(&locBuf[0]);
    /* read in the MaxSpread now */
    getEnvDef(&locBuf[0], "MaxSpread", "15");
    MaxSpread = atol(&locBuf[0]);
    /* read in the LowStarPct now */
    getEnvDef(&locBuf[0], "LowStarPct", "95");
    StarPer = atol(&locBuf[0]);
    /* read in the HiStarPct now */
    getEnvDef(&locBuf[0], "HiStarPct", "25");
    MultStarPer = atol(&locBuf[0]);
    /* read in MinStar now */
    getEnvDef(&locBuf[0], "MinStar", "6");
    SmallStarSize = atol(&locBuf[0]);
    /* read in MaxStar now */
    getEnvDef(&locBuf[0], "MaxStar", "9");
    LargeStarSize = atol(&locBuf[0]);
    /* read in the LowPlPct now */
    getEnvDef(&locBuf[0], "LowPlPct", "75");
    PlanPer = atol(&locBuf[0]);
    /* read in the HiPlPct now */
    getEnvDef(&locBuf[0], "HiPlPct", "10");
    MultPlanPer = atol(&locBuf[0]);
    /* read in MinPlan now */
    getEnvDef(&locBuf[0], "MinPlan", "1");
    SmallPlanetSize = atol(&locBuf[0]);
    /* read in MaxPlan now */
    getEnvDef(&locBuf[0], "MaxPlan", "5");
    LargePlanetSize = atol(&locBuf[0]);
    /* read in HomeSize now */
    getEnvDef(&locBuf[0], "HomeSize", "2");
    HomePlanSize = atol(&locBuf[0]);
    /* read in MaxTries now */
    /* First calculate the default value based on world size */
    MaxTries = (gbHorizSize * gbVertSize);
    if (MaxTries > 60000)	/* ~ 256x256 */
    {
        getEnvDef(&locBuf[0], "MaxTries", "850575");
    }
    else if (MaxTries > 16000)	/* ~ 128x128 */
    {
        getEnvDef(&locBuf[0], "MaxTries", "75000");
    }
    else
    {
        getEnvDef(&locBuf[0], "MaxTries", "35000");
    }
    MaxTries = atol(&locBuf[0]);

    /* read in ChangePlayers now */
    getEnvDef(&locBuf[0], "ChangePlayers", "1");
    if (locBuf[0] == '1')
    {
        gbChangePlayers = TRUE;
    }
    else
    {
        gbChangePlayers = FALSE;
    }
    /* read in PublicMessages now */
    getEnvDef(&locBuf[0], "PublicMessages", "1");
    if (locBuf[0] == '1')
    {
        gbPublicMessages = TRUE;
    }
    else
    {
        gbPublicMessages = FALSE;
    }
    return(build());
}

void doWriteFiles(void)
{
    writeFiles();
}

/*
 * useage - print a CLI useage message.
 */

void useage(void)
{
    puts("ImpCre - Creates the files needed to play Imperium\n");
    puts("     -v = Display extra status messages.\n");
}

/*
 * main - open libraries and windows, etc.
 */

int main(int argc, char *argv[])
{
    char locBuf[80];
    BOOL doLoop;

    HadBreak = FALSE;
    DoVerb = FALSE;
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("\n\t\tImpCre v%s.pl%d\n\n",
	IMP_BASE_REV, 0);
    strcpy(&Path[0], DEFAULT_PATH);
    oldUmask = umask(0077);
    if (argc == 2)
    {
	if (strcmp(argv[1], "-v") == 0)
	{
	    DoVerb = TRUE;
	    argc = 1;
	}
    }
    if (argc == 1)
    {
	signal(SIGINT, breakHandler);
        if (startGenerate())
        {
	    if (getenv("DoNotAsk") != NULL)
	    {
		if (strcmp(getenv("DoNotAsk"), "1") == 0)
		{
		    writeFiles();
	        }
		else
		{
		    puts("No files created...");
		}
	    }
	    else
	    {
		doLoop = TRUE;
		while (doLoop)
		{
		    printf("Do you wish to create the data files for "
			"this world? [Y/N] <N>:");
		    (void) fgets(&locBuf[0], 79, stdin);
		    if ((locBuf[0] == 'y') || (locBuf[0] == 'Y'))
		    {
			doLoop = FALSE;
			writeFiles();
		    }
		    else if ((locBuf[0] == '\n') || (locBuf[0] == 'n')
			|| (locBuf[0] == 'N'))
		    {
			puts("No files created...");
			doLoop = FALSE;
		    }
		    else
		    {
			puts("\n--- I don't know what you mean - try again\n");
		    }
		}
	    }
            cleanup(0);
        }
        cleanup(10);
    }
    useage();
    cleanup(5);
    /* it will never get to here */
    return(0);
}

static const char spew[] =
"Seek and ye shall find\0Learn to will\0Learn to know\0Learn to dare\0"
"Learn to KEEP SILENT\0Who was Adam the son of (Luke 3:38)\0"
"How many sons of god are there (Romans 8:14-17)\0"
"Are the sons of god also gods (John 10:34)\0"
"Oh Lord, is There No Hope for the Widows Son?\0";
