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
 * Imperium.h - main include file for Imperium
 *
 * $Id: Imperium.h,v 1.3 2000/05/26 22:55:13 marisa Exp $
 * $Log: Imperium.h,v $
 * Revision 1.3  2000/05/26 22:55:13  marisa
 * Combat changes
 *
 * Revision 1.2  2000/05/23 20:24:49  marisa
 * Added pl_weapon[] element to Planet_t struct
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:38  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:58:47  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:34:31  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:00:27  marisag
 * patch20: Fix empty revision.
 *
 */

/*
 * adjustable parameters - if you want to change these, be careful, and then
 *      recompile everything, including ImpCre.
 */

#define NAME_LEN                32      /* max length of player name    */
#define PASSWORD_LEN            10      /* max length of password       */
#define EMAIL_LEN               128     /* max length of password       */
#define PLAN_NAME_LEN           16      /* max length of planet names   */
#define PLAN_PSWD_LEN           8       /* max length of planet pswd    */
#define SHIP_NAME_LEN           20      /* max length of ship names     */
#define INPUT_LENGTH            500     /* max length of input line     */
#define REALM_MAX               6       /* max number of realms         */
#define REALM_LEN               80      /* length of each realm string  */

#define PLAYER_MAX              253      /* max number of players        */
/* never set PLANET_MAX below 12, or various things might break         */
#define PLANET_MAX              12      /* max # of planets per sect    */
/* Do not set FLEET_MAX to over 254!                                    */
#define FLEET_MAX               100     /* max ships in a fleet         */
#define MAX_NAV_SHIPS           32      /* max ships in a nav list      */
#define MAX_PROG_STEPS          41      /* max # of programbl nav steps */
#define MAX_NUM_ACTIONS         10      /* The max # of actions for ships */

#define MIN_AIR         45      /* the minimum air that supports life   */

#define NOBODY                  PLAYER_MAX /* author of automatic message */
#define SINGLE_SECTOR           0xffff  /* single sector in scanSectors */

#define MAX_WORK        32767   /* maximum amount of work on a single planet */

#define EFFIC_WARN      60      /* warning level       */
#define EFFIC_CRIT      40      /* critical level      */
#define EFFIC_DEAD      20      /* system destroyed    */

#define MAX_BIG_SHI     16      /* Maximum of the following set */
#define MAX_ENG         16      /* max number of engines in a ship */
#define MAX_COMP        16      /* max number of computers      */
#define MAX_WEAP        16      /* max number of weapons        */
#define MAX_WEAP_PL     8       /* max number of weapons        */
#define MAX_LIFESUP     4       /* max number of life supp syst */
#define MAX_ELECT       16      /* max number of electronic modules */

#define IMPERIUM_PORT "imperium.port"     /* The IPC port for Imperium    */
#define IMP_TEST_PORT "imp.test.port"     /* The test IPC port for Imperium    */

#define CONNECT_MESSAGE_FILE   "imp.conMess"   /* displayed at connect */
#define LOGIN_MESSAGE_FILE     "imp.logMess"   /* displayed after log on */
#define LOGOUT_MESSAGE_FILE    "imp.hangMess"  /* displayed at log out */
#define BULLETIN_FILE          "imp.bulletin"  /* displayed in news    */
#define ACCESS_FILE            "imp.access"    /* how to get on the system */
#define POWER_FILE             "imp.power"     /* power report file    */

/*
 * do not change anything from here on down
 */

/* This is where server puclishing emails go -
 * Don't change this if you want to be in the official list
 */
static const char SENDPUBTO[]="imppub@empiredirectory.net";

/*
 * Type definitions
 */

#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

#define BOOL unsigned char
#define BYTE signed char
#define UBYTE unsigned char
#define ULONG unsigned long
#define USHORT unsigned short

/*
 * Definitions for Imperium items
 */

#define NO_ITEM     0xffffffff  /* item not in use in struct    */
#define NO_FLEET    0xffff      /* player has no such fleet     */
#define NO_OWNER    0xff        /* item has no owner            */


    /* item (commodity) types */
#define it_civilians    0
#define it_scientists   1
#define it_military     2
#define it_officers     3
#define it_missiles     4
#define it_planes       5
#define it_ore          6
#define it_bars         7
#define it_airTanks     8
#define it_fuelTanks    9
#define it_computers    10
#define it_engines      11
#define it_lifeSupp     12
#define it_elect        13
#define it_weapons      14
typedef UBYTE ItemType_t;
#define IT_FIRST        0
#define IT_LAST_SMALL   9
#define IT_LAST         14
#ifdef ImpStartC
static const char ITEM_CHAR[] = "csmOMpobafCelEw\0";
#endif

    /* sector designations */
#define s_unknown       0
#define s_blackhole     1
#define s_supernova     2
#define s_vacant        3
#define s_normal        4
typedef UBYTE SectorType_t;
#define S_FIRST     0
#define S_LAST      4
#ifdef ImpStartC
static const char SECTOR_CHAR[] = "?@* :\0";
#endif
typedef struct
    {
        SectorType_t
            s_type;                 /* designation of this sector   */
        USHORT
            s_shipCount,            /* number of ships here         */
            s_planetCount;          /* number of planets here       */
        ULONG
            s_planet[PLANET_MAX];   /* the numbers of the planets   */
    } Sector_t;

#define pc_S    0   /* star (not really a planet :-)    */
#define pc_HOME 1   /* A home planet                    */
#define pc_A    2   /* rocky, lifeless, no minerals     */
#define pc_B    3   /* entirely water, no land masses   */
#define pc_C    4   /* Gaseous                          */
#define pc_D    5   /* generic planet class, but no intelligent life */
#define pc_M    6   /* earth-type, likely to support int. life  */
#define pc_N    7   /* rocky, no native life  good min  */
#define pc_O    8   /* about the same as N, but good gold */
#define pc_Q    9   /* Q should be fairly rare, maybe 1 per game */
typedef UBYTE PlanetClass_t;
#define PC_FIRST    0
#define PC_LAST     9
#if defined(CmdEditC) || defined(CmdGen1C) || defined(CmdMapC)
static const char PLANET_CHAR[] = "*HABCDMNOQ\0";
#endif

    /* ship types */
/*
    Note that "ship types" only describes the general "size" or purpose of
    the craft. Since each ship can be named and customized seperately, the
    "type" only covers the basic "shell" or "skeleton" of the ship
*/
#define st_a    0
#define st_b    1
#define st_c    2
#define st_d    3
#define st_e    4
#define st_m    5
typedef UBYTE ShipType_t;
#define ST_FIRST    0
#define ST_LAST     5
#define ST_BIGGEST  4   /* this is the ship that carries the most cargo */
#if defined(ImpStartC) || defined(CmdGen3C) || defined(CmdMapC)
static const char SHIP_CHAR[] = "abcdem\0";
#endif

#define RACE_MAX    7
#define NO_RACE     7
#define rs_notyet   0
#define rs_active   1
#define rs_dead     2
typedef UBYTE RaceStat_t;

/* planet production fields */
#define pp_civ          0
#define pp_mil          1
#define pp_technology   2
#define pp_research     3
#define pp_education    4
#define pp_ocs          5
#define pp_oMine        6
#define pp_gMine        7
#define pp_airTank      8
#define pp_fuelTank     9
#define pp_weapon       10
#define pp_engine       11
#define pp_hull         12
#define pp_missile      13
#define pp_planes       14
#define pp_electronics  15
#define pp_cash         16
typedef UBYTE PProd_t;
#define PPROD_FIRST    0
#define PPROD_LAST     16
#if defined(CmdEditC) || defined(CmdGen1C) || defined(UpdateC)
static const char *PPROD_NAME[] =
    {
        "civilians",
        "military",
        "technology",
        "research",
        "education",
        "ocs",
        "ore mining",
        "gold mining",
        "air tanks",
        "fuel tanks",
        "weapons",
        "engines",
        "hull",
        "missile",
        "planes",
        "electronics",
        "cash"
    };
#endif

typedef struct
    {
        char
            r_name[NAME_LEN],   /* The races name           */
                /* r_homeName is here so we don't have to read the planet */
                /* file just to get the name */
            r_homeName[PLAN_NAME_LEN]; /* Their home planet name */
        ULONG
            r_techLevel,        /* The races tech level     */
            r_resLevel,         /* The races research level */
            r_homePlanet;       /* Home planet of the race  */
        USHORT
            r_homeRow,          /* row of the home planet   */
            r_homeCol;          /* col of the home planet   */
        ULONG
            r_planetCount;      /* # of planets owned by race */
        UBYTE
            r_playCount;        /* How many players are of this race */
        RaceStat_t
            r_status;           /* The status of this race  */
    } Race_t;

typedef struct
    {
	USHORT
	    cr_mil,		/* # of military required */
	    cr_ofc;		/* # of officers required */
    } CrewReq_t;

#define CR_NAV	0
#define CR_PHA	1
#define CR_PHO	2
#define CR_MAX	3

typedef struct
    {

        /* miscellaneous fixed fields */

        ULONG
            w_lastRun,                  /* time of last use of Imperium     */
            w_buildDate;                /* this section of "space" was built */
        char
            w_winName[NAME_LEN];        /* Name of last winner              */
        ULONG
            w_secondsPerITU;            /* length of an Imperium time unit  */
        USHORT
            w_rows,                     /* number of rows in this "space"   */
            w_columns,                  /* number of columns in this "space" */
            w_fleetNext,                /* number of next fleet     */
            w_maxConnect,               /* max time in minutes per day      */
            w_maxBTUs;                  /* max BTUs held by a player        */
        UBYTE
            w_armourPoints,             /* # pnts of dam/armour unit        */
            w_maxPlayers,               /* maximum allowed number of users  */
            w_currPlayers;              /* number of entered users          */
        char
            w_password[PASSWORD_LEN],   /* password to create a player */
            w_emailAddress[EMAIL_LEN];  /* system email address	       */

        /* global counters */

        ULONG
            w_loanNext,                 /* number of next loan      */
            w_offerNext,                /* number of next sale offering */
            w_shipNext,                 /* number of next ship      */
            w_bigItemNext,              /* number of next big item  */
            w_planetNext;               /* number of next planet    */

        /* production costs */

        USHORT
            w_resCost,                  /* production for one research */
            w_techCost,                 /* production for one tech  */
            w_missCost,                 /* production for one missl */
            w_planeCost,                /* production for one plane */
            w_barCost,                  /* production for one bar of gold */
            w_airCost,                  /* production for one unit of air */
            w_fuelCost,                 /* production for one unit of fuel */
            w_prodCost[PPROD_LAST + 1]; /* production costs for big items */

        /* various mobility costs */

        USHORT
            w_baseFuelCost[ST_LAST + 1],/* base fuel cost for ship class */
            w_defMob;                   /* cost of move to regular  */

        /* plague adjusters */

        USHORT
            w_plagueKiller,             /* (227) divisor for die rate */
            w_plagueBooster,            /* replace the 100 in top   */
            w_plagueOneBase,            /* duration of stage one    */
            w_plagueOneRand,
            w_plagueTwoBase,            /* duration of stage two    */
            w_plagueTwoRand,
            w_plagueThreeBase,          /* duration of stage three  */
            w_plagueThreeRand;

        /* various monetary rates */

        USHORT
            w_efficCost,                /* cost of one unit of increase */
            w_milSuppliesCost,          /* (mil / 32) * dt / X => dollars */
            w_ofcSuppliesCost,          /* (ofc / 32) * dt / X => dollars */
            w_utilityRate,              /* is 1 in PSL */
            w_interestRate,             /* in percent */
            w_shipCostMult,             /* in $ per build unit */
            w_refurbCost;               /* cost of 1 level of ship refurb */

        /* scale factors for production */

        USHORT
            w_hullScale,                /* X / 100 scales production */
            w_engineScale,              /* X / 100 scales production */
            w_resScale,                 /* X / 100 scales production */
            w_techScale,                /* X / 100 scales production */
            w_defenseScale,             /* X / 100 scales production */
            w_missScale,                /* X / 100 scales production */
            w_planeScale,               /* X / 100 scales production */
            w_goldScale,                /* X / 100 scales production */
            w_ironScale,                /* X / 100 scales production */
            w_barScale,                 /* X / 100 scales production */
            w_shipWorkScale;            /* X / 100 scales ship work */

        /* factors affecting sector updates */

        USHORT
            w_efficScale,               /* X / 100 scales increase */
            w_mobilScale,               /* X / 100 scales increase */
            w_highGrowthFactor,         /* replace the 200 in PSL formula */
            w_lowGrowthFactor,          /* replace the 400 in PSL formula */
            w_BTUDivisor,               /* high gives less new BTU's */
            w_resDecreaser,             /* 10 x percent loss per day */
            w_techDecreaser;            /* 10 x percent loss per day */

        /* various attack advantages */

        USHORT
            w_assAdv,                   /* X / 100 scales assault defense   */
            w_boardAdv;                 /* X / 100 is adv of board defense  */

        /* torpedoes, phasers, and mines */

        USHORT
            w_torpCost,                 /* how many missiles per torpedo    */
            w_torpMobCost,              /* mobility cost for torpedoing     */
            w_torpAcc,                  /* percent hit at range 0           */
            w_torpBase,                 /* damage done by torpedo           */
            w_torpRand,
            w_phaserAcc,                /* DO NOT USER                      */
            w_phaserRange,              /* Range of TF100 blasers           */
            w_phaserDmg,                /* amt of dmg per unit of energy    */
                                        /* when weapon overloaded           */
            w_mineBase,                 /* damage by one mine               */
            w_mineRand;

        /* "airplane" factors */

        USHORT
            w_fuelTankSize,             /* How big the airplanes tank is */
            w_fuelRichness,             /* replace the normal 4 */
            w_flakFactor,               /* replace the 7 divisor, etc. */
            w_landScale,                /* X / 100 scales landing chances */
            w_bombBase,                 /* damage per bomb */
            w_bombRand,
            w_planeBase,                /* damage per crashing plane */
            w_planeRand;

        /* miscellaneous adjustable factors */

        USHORT
            w_contractScale,            /* X / 100 scales contract offers   */
            w_deathFactor,              /* BTU cost per death (* 100)       */
            w_gunMax,                   /* replace the normal 12            */
            w_gunScale,                 /* X / 100 scales gun damage        */
            w_lookShipFact,             /* see if d**2 < r**2 * sz**2 / X   */
            w_collectScale,             /* X / 100 scales collect price     */
            w_radarFactor,              /* replace 61 in land radar range   */
            w_spyFactor,                /* larger means spies better        */
            w_armourWeight,             /* weight of 1 unit of armour       */
            w_shipTechDecreaser;        /* 100 x percent loss per "day"     */

        Race_t
            w_race[RACE_MAX];           /* All the races in the game        */
	CrewReq_t
	    w_crewReq[CR_MAX];		/* Crew requirements		    */

        /* various flag-type things */
        unsigned int
            w_sendAll:1;                /* Allow people to send public msgs */
        unsigned int
            w_chaPlay:1;                /* Allow people to change "players" */
        unsigned int
            w_doFlush:1;                /* Flush buffers on logout?         */
        unsigned int
            w_userFlush:1;              /* anyone can flush buffers         */
        unsigned int
            w_nonDeityPower:1;          /* anyone can force a power         */
        unsigned int
            w_doingPower:1;             /* someone is building power report */
        unsigned int
            w_noNameChange:1;           /* no name changes allowed          */
        unsigned int
            w_noCreatePass:1;           /* no creation pswd needed          */
        unsigned int
            w_keepRest:24;              /* These should always total 32     */

        /* arrays of various ship factors */

        USHORT
            w_shipCargoLim[ST_LAST + 1], /* max cargo carried       */
            w_shipCost[ST_LAST + 1],    /* cost of ships            */
            w_mobCost[IT_LAST + 1],     /* mob cost for items       */
        /* scaled default "weights" of items for ships  */
            w_weight[IT_LAST + 1];      /* default weight of items  */

	char
	    w_gameName[64],        /* Name of the game              */
	    w_gameHost[64],        /* Host of the game              */
	    w_gamePort[12],        /* Port of the game              */
	    w_gameSecret[12];      /* Secret of the game              */

    } World_t;

#define ps_deity    0
#define ps_active   1
#define ps_quit     2
#define ps_visitor  3
#define ps_idle     4
typedef UBYTE PlayerStatus_t;

#define r_default   0
#define r_neutral   1
#define r_allied    2
#define r_war       3
typedef UBYTE Relation_t;

#define a_none      0   /* a dummy action - does nothing */
#define a_load      1   /* load items from planet       */
#define a_unload    2   /* unload items from ship       */
#define a_unloadpct 3   /* unload a percentage of the ships items */
#define a_LRscan    4   /* do a long-range scan         */
#define a_SRscan    5   /* do a short-range scan        */
#define a_VRscan    6   /* do a visual-range scan       */
#define a_land      7   /* attempt to land on planet    */
#define a_lift      8   /* attempt to blast off         */
#define a_teleDn    9   /* teleport some items down     */
#define a_teleUp    10  /* beam some items up           */
#define a_first     0
#define a_last      10
typedef UBYTE ActionType_t;
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

typedef struct
    {
        ULONG
            p_lastOn,                   /* time of last connect     */
            p_timeLeft,                 /* amount of real time left */
            p_timeWarn,                 /* Warn when time below here*/
            p_telegramsNew,             /* offset of first new one  */
            p_telegramsTail,            /* next available offset    */
            p_planetCount;              /* number of planets owned  */
        long
            p_money;                    /* money he has             */
        USHORT
            p_feMode,                   /* FE mode activated        */
            p_btu,                      /* btu's remaining          */
            p_btuWarn,                  /* Warn when BTUs below here*/
                            /* The following 2 items are    */
                            /* NOT used by Imperium itself  */
                            /* but occur here only so they  */
                            /* may be saved and loaded with */
                            /* the rest of the players info */
            p_conWidth,                 /* players desired screen width */
            p_conLength,                /* players desired screen length */
            p_fleets[26 + 26];          /* map letters to fleet #'s */
        PlayerStatus_t
            p_status;                   /* current status           */
        UBYTE
            p_number,                   /* The players number       */
            p_race;                     /* What race is this player */
        char
            p_name[NAME_LEN],           /* player name              */
            p_password[PASSWORD_LEN],   /* password                 */
            p_email[EMAIL_LEN],         /* email address            */
            p_realm[REALM_MAX][REALM_LEN];
        Relation_t
            p_racerel[RACE_MAX],        /* player-race relations    */
            p_playrel[PLAYER_MAX];      /* player-player relations  */
        Action_t
            p_action[MAX_NUM_ACTIONS];  /* Pre-programmed actions   */
        unsigned int
            p_loggedOn:1;               /* logged on right now      */
        unsigned int
            p_inChat:1;                 /* in chat mode             */
        unsigned int
            p_compressed:1;             /* default mode for map     */
        unsigned int
            p_doingPower:1;             /* true if a plyr is updtg pwr rpt */
        unsigned int
            p_newPlayer:1;              /* true if player is new    */
        unsigned int
            p_sendEmail:1;              /* true if player wants email */
        unsigned int
            p_tmp:26;                   /* for future use  must == 32 */
#define nt_telegram 0                   /* notify => telegram       */
#define nt_message  1                   /* notify => message        */
#define nt_both     2                   /* notify => both           */
        UBYTE p_notify;
    } Player_t;

#define bp_computer     0
#define bp_engines      1
#define bp_hull         2
#define bp_lifeSupp     3
#define bp_sensors      4
#define bp_teleport     5
#define bp_photon       6
#define bp_blaser       7
#define bp_shield       8
#define bp_tractor      9
typedef UBYTE BigPart_t;
#define BP_FIRST    0
#define BP_LAST     9
#if defined(CmdGen1C) || defined(CmdNavC)
static const char BIG_PART_CHAR[] = "cehlstpbST\0";
#endif

typedef struct
    {
        ULONG
            bi_lastUpdate,  /* When the item was last updated */
            bi_itemLoc,     /* either a ship or planet #    */
            bi_techLevel,   /* tech level of the item       */
            bi_number;      /* The number of this big item  */
        BigPart_t
            bi_part;        /* what type of item is this    */
        USHORT
            bi_price,       /* only set if for sale         */
            bi_weight;      /* how much does item weigh     */
#define bi_inuse        0   /* set if item is useable       */
#define bi_destroyed    1   /* set if item is unuseable     */
#define bi_forSale      2   /* set if item is for sale      */
        UBYTE
            bi_status;
        BOOL
            bi_onShip;
        BYTE
            bi_effic;       /* efficiency of this item      */
    } BigItem_t;

/* flags for ships */
#define SHF_NONE        (0)         /* no flags set                 */
#define SHF_TRACTEE     (1)         /* ship is being pulled         */
#define SHF_BLASE       (1<<1)      /* blaser only during attack    */
#define SHF_PHOT        (1<<2)      /* photons only during attack   */
#define SHF_DEF_PB      (1<<3)      /* photons then blasers         */
#define SHF_DEF_BP      (1<<4)      /* blasers then photons         */
#define SHF_SHIELDS     (1<<5)      /* raise shld to level x during attack  */
#define SHF_SHKEEP      (1<<6)      /* refresh shield level when attacked   */
#define SHF_DISTRESS    (1<<7)      /* distress beacon              */
#define SHF_DIST_ALLIED (1<<8)      /* send distress only to allies */
#define SHF_DEFEND      (1<<9)      /* =1 if should defend other ships      */
#define SHF_CARRY_MINE  (0xFF<<24)  /* != 0 if ship is carrying any miners  */

typedef struct
    {
        ULONG
            sh_number,                  /* The number of this ship  */
            sh_planet,                  /* Planet it's on, if any   */
            sh_dragger,                 /* Which ship is pulling this one */
                                        /* Also used to hold the ship */
                                        /* that is carying this ship */
                                        /* if this ship is a miner  */
            sh_flags,                   /* misc flags, see above    */
            sh_lastUpdate;              /* time of last update      */
        USHORT
            sh_price,                   /* price if for sale        */
            sh_fuelLeft,                /* amount of fuel left      */
            sh_cargo,                   /* current cargo amount     */
            sh_armourLeft,              /* Amount of armour on ship */
            sh_row,                     /* row it's at              */
            sh_col,                     /* column it's at           */
            sh_shields,                 /* energy in shields        */
            sh_shieldKeep,              /* energy to maintain in shields */
                                        /* during an attack */
            sh_airLeft,                 /* amount of air left       */
            sh_energy;                  /* state of the energy banks*/
        ShipType_t
            sh_type;                    /* basic type of this ship  */
        UBYTE
            sh_fleet,                   /* letter of fleet it's in  */
            sh_efficiency,              /* eff at last update       */
            sh_owner,                   /* who owns it              */
                    /* next two fields save reading in every */
                    /* navigate movement */
            sh_hullTF,                  /* hull tech factor         */
            sh_engTF,                   /* engine tech factor       */
            sh_engEff,                  /* effic of engines         */
            sh_plagueStage,             /* Plague stage             */
            sh_plagueTime;              /* Time left in this stage  */
            /* if the ship is a miner the following two fields will */
            /* hold the information in the table that follows:      */
        char
            sh_name[SHIP_NAME_LEN],     /* The ships name, if any   */
            sh_course[MAX_PROG_STEPS];  /* The ships pre-programmed course */
            /* These name field will be headed by a NULL char and   */
            /* will contain data in this format:                    */
            /*
                  OFFSET
                         11111111112
                12345678901234567890
                aaaab???????????????

                aaaa = long int:    ship that dropped the miner
                b    = 8 bits:      various flags
                ?    = undefined
            */
        USHORT
            sh_items[IT_LAST + 1];      /* items in the hold        */
                /* the next fields contain BigItem_t numbers */
        ULONG
            sh_computer[MAX_COMP],      /* ships computers #'s      */
            sh_engine[MAX_ENG],         /* ships engines #'s        */
            sh_hull,                    /* ships hull #             */
            sh_lifeSupp[MAX_LIFESUP],   /* ships life supp syst #'s */
            sh_elect[MAX_ELECT],        /* ships electronics #'s    */
            sh_weapon[MAX_WEAP];        /* ships weapons #'s        */
    } Ship_t;

typedef struct
    {
        UBYTE
            f_count,                    /* number of ships now in fleet */
            f_owner;                    /* who owns this fleet  */
        USHORT
            f_number;                   /* this fleet number    */
        ULONG
            f_ship[FLEET_MAX];          /* the ships in the fleet */
    } Fleet_t;

typedef struct
    {
        ULONG
            l_lastPay,                  /* time of last payment */
            l_amount,                   /* amount left to pay   */
            l_paid,                     /* amount paid so far   */
            l_dueDate;                  /* when payment is due  */
        USHORT
            l_number,                   /* the number of this loan */
            l_duration,                 /* duration in days     */
            l_rate;                     /* interest rate (daily) */
        UBYTE
            l_loaner,                   /* who made the loan    */
            l_loanee,                   /* who the loan is to   */
            l_state;
#define l_offered       0
#define l_declined      1
#define l_outstanding   2
#define l_paidUp        3
    } Loan_t;

typedef struct
    {
        USHORT
            of_number;          /* the number of this offer     */

#define of_ship     0           /* ship is for sale             */
#define of_planet   1           /* items on a planet are for sale */
#define of_item     2           /* big item is for sale         */
#define of_none     3           /* this offer is closed         */
        UBYTE
            of_state,           /* one of the above conditions  */
            of_who;             /* who is offering it           */
        union
        {
            ULONG
                of_shipNumber,  /* which ship is offered        */
                of_itemNumber;  /* which item is offered        */
            struct
            {
                ULONG
                    of_planetNumber;

#define of_buyPrice     0
#define of_sellPrice    1
                USHORT
                    prices[2][IT_LAST_SMALL + 1];

#define of_planOwn  0
#define of_shipOwn  1
                UBYTE
                    of_payor;
            }   of_plan;
        }
            of_;
    } Offer_t;

#define pt_trade        0   /* Planet was traded peacefully */
#define pt_peacefull    1   /* Planet was taken peacefully, previously vacant */
#define pt_home         2   /* This is a home planet        */
#define pt_hostile      3   /* Planet was taken by force, people might rebel */
typedef UBYTE PlTransfer_t;

typedef struct
    {
        ULONG
            pl_number,          /* The planets number               */
            pl_techLevel,       /* The planets tech level           */
            pl_resLevel,        /* The planets research level       */
            pl_lastUpdate;      /* time of last update              */
        PlanetClass_t
            pl_class;           /* What type of planet is it        */
        PlTransfer_t
            pl_transfer;        /* how was planet transfered        */
        USHORT
            pl_row,             /* The row the planet is in         */
            pl_col,             /* The column the planet is in      */
            pl_mobility,        /* The planets mobility             */
            pl_shipCount,       /* num ships orbiting the planet    */
            pl_btu,             /* planets BTUs                     */
            pl_prod[PPROD_LAST + 1],    /* How many prod. points/type */
            pl_quantity[IT_LAST + 1];   /* items on the planet      */
        UBYTE
            pl_plagueStage,     /* Plague stage the planet is in    */
            pl_plagueTime,      /* Time left in this stage          */
            pl_lastOwner,       /* the last owner of the planet     */
            pl_size,            /* size of the planet, 0-10         */
            pl_owner,           /* owner of the planet              */
            pl_polution,        /* 0 - 100 factors                  */
            pl_efficiency,      /* How efficient is the planet?     */
            pl_minerals,
            pl_gold,
            pl_gas,
            pl_water,
            pl_bigItems,        /* how many big items are here      */
            pl_ownRace;         /* This saves us from having to     */
                                /* read in owner just to get race   */
        UBYTE
            pl_workPer[PPROD_LAST + 1]; /* 0-100% of production     */
	ULONG
            pl_weapon[MAX_WEAP_PL];        /* ships weapons #'s        */
        char
            pl_name[PLAN_NAME_LEN],  /* The planets name, if any    */
            pl_checkpoint[PLAN_PSWD_LEN]; /* checkpoint for planet  */
            
    } Planet_t;

#define n_nothing           0           /* dummy - no action            */
#define n_destroyed         1           /* actor destr. victim race     */
#define n_won_planet        2           /* actor won a planet           */
#define n_lost_planet       3           /* actor failed to win a planet */
#define n_sent_telegram     4           /* actor sent a telegram        */
#define n_make_loan         5           /* actor made a loan            */
#define n_repay_loan        6           /* actor repaid a loan          */
#define n_make_sale         7           /* actor made a sale            */
#define n_grant_planet      8           /* actor granted a planet       */
#define n_blase_ship        11          /* actor blased a ship          */
#define n_took_unoccupied   12          /* actor took an empty planet   */
#define n_fire_back         14          /* actor fired in self-defense  */
#define n_bomb_planet       15          /* actor bombed a planet        */
#define n_bomb_dest         16          /* actor bombed a planet        */
#define n_board_ship        17          /* actor boarded a ship         */
#define n_failed_board      18          /* actor failed to board a ship */
#define n_flak              19          /* actor fired on aircraft      */
#define n_sieze_planet      20          /* actor siezed a planet (loan) */
#define n_decl_ally         21          /* actor declared alliance      */
#define n_decl_neut         22          /* actor declared neutrality    */
#define n_decl_war          23          /* actor declared war           */
#define n_disavow_ally      24          /* actor disavowed former alliance */
#define n_disavow_war       25          /* actor disavowed former war   */
#define n_plague_outbreak   26          /* actor had outbreak of plague */
#define n_plague_die        27          /* actor had plague deaths      */
#define n_plague_dest       28          /* actor had pl wiped out by plague */
#define n_ship_dest         29          /* actor destroyed a ship       */
#define n_torp_dest         30          /* actor had a ship dest by torp */
#define n_ship_sick         31          /* actor ship caught the plague */
#define n_ship_sie          32          /* actor ship had plague deaths */
#define n_ship_wipe         33          /* actor ship wiped out by plague */
#define n_torp_ship	    34		/* actor torpedoed a ship	*/
typedef UBYTE NewsType_t;

typedef struct
    {
        ULONG
            n_time;                     /* time when it happened */
        NewsType_t
            n_verb;                     /* what he did */
        UBYTE
	    n_aRace,
            n_actor,                    /* who did it */
	    n_vRace,
            n_victim;                   /* who he did it to */
    } News_t;

typedef struct
    {
        ULONG
            n_ship;
        long
            n_mobil;
        BOOL
            n_active;
    } Nav_t;

#define at_bomb     0
#define at_shell    1
#define at_torp     2
#define at_blaser   3
typedef UBYTE AttackType_t;

