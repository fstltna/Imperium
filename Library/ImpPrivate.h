/*
 * Imperium
 *
 * $Id: ImpPrivate.h,v 1.3 2000/05/26 22:55:19 marisa Exp $
 *
 * definitions private to the Imperium library.
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
 * $Log: ImpPrivate.h,v $
 * Revision 1.3  2000/05/26 22:55:19  marisa
 * Combat changes
 *
 * Revision 1.2  2000/05/23 20:25:14  marisa
 * Added pl_weapon[] element to Planet_t struct
 * Began working on ship<->planet combat code
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:43  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:58:56  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:15:50  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:33  marisag
 * patch20: Fix empty revision.
 *
 */

#define INPUT_BUFFER_SIZE   500
#define OUTPUT_BUFFER_SIZE  500

typedef struct
    {
        USHORT
            sbp_row,
            sbp_col;
        ULONG
            sbp_number;
        PlanetClass_t
            sbp_class;
    } SBPlanet_t;

typedef struct
    {
        ULONG
            sb_worldCount;  /* number of planets in the world */
                            /* (used for authentication)    */

        USHORT
            sb_sector,      /* The "mapped" sector number   */
            sb_plCount;     /* number of planets here       */
        SBPlanet_t
            sb_planet[PLANET_MAX];
    } SectBuf_t;


typedef struct
    {
        /* fields used to interface to the library caller */

        void (*is_serverRequest)(void);
        void (*is_writeUser)(void);
        void (*is_readUser)(void);
        void (*is_timedReadUser)(void);
        void (*is_echoOff)(void);
        void (*is_echoOn)(void);
        void (*is_gotControlC)(void);
        void (*is_sleep)(void);
        void (*is_log)(void);
        void (*is_extEdit)(void);
        Request_t   is_request;
        char        is_textIn[INPUT_BUFFER_SIZE];
        char        is_textOut[OUTPUT_BUFFER_SIZE];
        char        *is_textInPos;
        USHORT      is_textOutPos;

        USHORT      is_conWidth,    /* desired console width (num columns)  */
                    is_conLength;   /* desired console height (num lines)   */

        /* These next fields hold values that are coming into the library */
        /* via call-back functions */
        BOOL        is_argBool;     /* used to hold returned BOOL values    */
        USHORT      is_argShort;    /* used to hold returned short values   */
        ULONG       is_argLong;     /* used to hold returned long values    */
        ULONG       is_specialFlags;/* special flags - see Request.h        */
        char       *is_argPoint;    /* holds character pointers             */

        /* fields which cache values used often or required in general */

        World_t     is_world;
        Player_t    is_player;
        char        is_sectorChar[S_LAST + 2];
        char        is_shipChar[ST_LAST + 2];
        char        is_itemChar[IT_LAST + 2];
        USHORT      is_seed[3];     /* 48 bit random number seed            */
        ULONG       is_contractEarnings,
                    is_interestEarnings,
                    is_improvementCost,
                    is_militaryCost,
                    is_utilitiesCost;
        BOOL        is_noWrite,
                    is_quietUpdate,
                    is_verboseUpdate;

        /* used during the 'navigate' command */

        Nav_t       is_movingShips[MAX_NAV_SHIPS];
        USHORT      is_movingShipCount;

        /* general utility fields used by various commands */

        ULONG       is_ULONG1,
                    is_ULONG2,
                    is_ULONG3,
                    is_ULONG4;
        long        is_long1,
                    is_long2,
                    is_long3,
                    is_long4;
        USHORT      is_USHORT1,
                    is_USHORT2,
                    is_USHORT3,
                    is_USHORT4;
        short       is_short1,
                    is_short2,
                    is_short3,
                    is_short4;
        BOOL        is_BOOL1,
                    is_BOOL2,
                    is_BOOL3,
                    is_BOOL4;
        SectorType_t
                    is_sectorType1,
                    is_sectorType2;
        ShipType_t  is_shipType1,
                    is_shipType2;
        ItemType_t  is_itemType1,
                    is_itemType2;
        SectBuf_t
                    is_sectBuf;
        BOOL
                    is_DEBUG;               /* True if we are debugging */
    } ImpState_t;
#define IMP register ImpState_t *IS


#define ft_normal   0
#define ft_help     1
#define ft_doc      2
typedef UBYTE FileType_t;

/* result type of the various 'xxxOnce' routines: */
/* using enum's since these never get stored on disk, and so the 3 byte */
/* waste doesn't matter */

typedef enum
    {
        m_readMore = 0,
        m_continue,
        m_done
    } Move_t;

/* what type of checkpoint code we are looking for: */
/* cpt_access = complete access to planet */
/* cpt_land   = only allowed to land and use teleporters to buy/sell goods */
/* using enum's since these never get stored on disk, and so the 3 byte */
/* waste doesn't matter */

typedef enum
    {
        cpt_access = 0,
        cpt_land
    } CheckPointType_t;

typedef struct
    {
        USHORT
            sdr_shields,
            sdr_armour,
            sdr_main;
    } ShipDamRes_t;

/* prototypes from the various source files */

    /* startup.c */

void user(IMP, register char *);
void userNL(IMP);
void userC(IMP, register const char);
void userSp(IMP);
void uFlush(IMP);
void uPrompt(IMP, register char *);
void userN(IMP, register long);
void userF(IMP, register long, register short);
void userX(IMP, register ULONG, register USHORT);
void user2(IMP, register char *, register char *);
void user3(IMP, register char *, register char *, register char *);
void userN2(IMP, register char *, register long);
void userN3(IMP, register char *, register long, register char *);
void userS(IMP, register char *, USHORT r, USHORT c, register char *);
void userP(IMP, register char *, register Planet_t *, register char *);
void userSh(IMP, register char *, register Ship_t *, register char *);
void userM3(IMP, register char *, register ULONG, register char *);
void getPrompt(IMP, register char *);
BOOL ask(IMP, register char *);
void server(IMP, register const RequestType_t rt, register const ULONG);
void server2(IMP, register const RequestType_t rt, register const ULONG,
    register const ULONG);
void log3(IMP, char *, char *, char *);
void log4(IMP, char *, char *, char *, char *);
void cmd_flush(IMP);
void cmd_tickle(IMP);
void cmd_log(IMP);
void impSleep(IMP, const USHORT);
void uTime(IMP, ULONG time);
BOOL printFile(IMP, char *, const FileType_t);
void news(IMP, register const NewsType_t, register const USHORT,
    register const USHORT);
BOOL getPassword(IMP, register char *, register char *);
BOOL newPlayerPassword(IMP);
BOOL newPlayerEmail(IMP);
BOOL updateTimer(IMP);
BOOL resetTimer(IMP);
void clServerReq(IMP);
void clWriteUser(IMP);
BOOL clReadUser(IMP);
BOOL clTimedReadUser(IMP);
void clEchoOff(IMP);
void clEchoOn(IMP);
BOOL clGotCtrlC(IMP);
void clSleep(IMP, USHORT);
USHORT clEdit(IMP, char *, ULONG);
BOOL clLog(IMP, char *);
void showStat(IMP, USHORT);
void cmd_version(IMP);
ImpState_t *ImpAlloc(void);
void ImpFree(IMP);
void ImpControl(IMP);
void Imperium(IMP);

    /* util.c */

ULONG timeRound(IMP, ULONG);
ULONG timeNow(IMP);
USHORT impRandom(IMP, USHORT);
USHORT lookupCommand(register const char *, char *);
void dash(IMP, register USHORT);
void err(IMP, char *);
char *getDesigName(SectorType_t);
char *getItemName(ItemType_t);
char *getShipName(ShipType_t);
char *getPlanetName(PlanetClass_t);
USHORT getIndex(IMP, register char *, register char);
USHORT getShipIndex(IMP, char);
USHORT getItemIndex(IMP, char);
USHORT umin(USHORT, USHORT);
ULONG ulmin(ULONG, ULONG);
USHORT mapSector(IMP, register USHORT, register USHORT);
void accessShip(IMP, ULONG);
void accessPlanet(IMP, ULONG);
USHORT readPlQuan(IMP, Planet_t *, ItemType_t);
void writePlQuan(IMP, Planet_t *, ItemType_t, USHORT);
USHORT getTransportCost(IMP, Planet_t *, ItemType_t, register USHORT);
void adjustForNewWorkers(IMP, register Planet_t *, ItemType_t, USHORT);
UBYTE getTechFactor(register ULONG);
USHORT getPlanetTechFactor(IMP, Planet_t *);
UBYTE getShipTechFactor(IMP, Ship_t *, BigPart_t);
UBYTE getShipEff(IMP, Ship_t *, BigPart_t);
USHORT getShipSpeed(IMP, Ship_t *);
USHORT getShipVisib(IMP, Ship_t *);
USHORT getShipVRange(IMP, Ship_t *);
USHORT findDistance(IMP, USHORT, USHORT, USHORT, USHORT);
USHORT getItemCost(IMP, ItemType_t);
USHORT getNavCost(IMP, Ship_t *);
USHORT damageUnit(IMP, register USHORT, USHORT);
ULONG damageBigItem(IMP, ULONG, USHORT);
void damagePlanet(IMP, USHORT, USHORT);
USHORT fleetPos(register char ch);
void removeFromFleet(IMP, USHORT, ULONG);
void decrShipCount(IMP, USHORT);
void incrShipCount(IMP, USHORT);
void decrPlShipCount(IMP, register ULONG);
void incrPlShipCount(IMP, register ULONG);
void buildShipWeight(IMP, Ship_t *);
void autoRaiseShields(IMP, BOOL);
ShipDamRes_t damageShip(IMP, register ULONG, USHORT, BOOL);
void attackShip(IMP, ULONG, USHORT, AttackType_t, char *);
BOOL verifyCheckPoint(IMP, Planet_t *, CheckPointType_t);
void removeSmallShipItem(IMP, Ship_t *, ItemType_t, USHORT);
void removeBigShipItem(IMP, Ship_t *, BigItem_t *);
void torpCost(IMP);
BOOL isInst(register Ship_t *, BigPart_t, register ULONG);
USHORT numInst(IMP, register Ship_t *, BigPart_t);
USHORT numInstPl(IMP, register Planet_t *, BigPart_t);
USHORT findFree(register Ship_t *, BigPart_t);
BigPart_t cvtItBp(ItemType_t);
ItemType_t cvtBpIt(BigPart_t);
void readSectBuf(IMP, register USHORT);
ULONG whichPlanet(IMP, register USHORT, register USHORT);
void abandonPlanet(IMP, register Planet_t *);
void takeNewPlanet(IMP, register Planet_t *);
USHORT getShipCapacity(IMP, ItemType_t, register Ship_t *);
void getLowHi(IMP, register ULONG *, register ULONG *);
void getPlRange(IMP, ULONG *, ULONG *, USHORT, USHORT, USHORT);
USHORT isHomePlanet(IMP, register ULONG);
void doUninstall(Ship_t *, register ULONG, ItemType_t);
void sendEmail(IMP, const char *, const char *, const char *);
void sendSystemEmail(IMP, const char *, const char *);

    /* messages.c */

void cmd_telegram(IMP);
void telegramCheck(IMP);
void messageCheck(IMP);
void cmd_read(IMP);
void cmd_headlines(IMP);
void cmd_newspaper(IMP);
BOOL cmd_propaganda(IMP);
void cmd_message(IMP);
void cmd_chat(IMP);
void notify(IMP, USHORT);

    /* parse.c */

char *skipBlanks(IMP);
char *skipWord(IMP);
BOOL doSkipBlanks(IMP);
BOOL getNumber(IMP, long *);
BOOL reqNumber(IMP, long *, char *);
BOOL getPosRange(IMP, ULONG *, ULONG);
BOOL reqPosRange(IMP, ULONG *, ULONG, char *);
BOOL reqPosRange1(IMP, ULONG *, ULONG, char *);
BOOL getBox(IMP, USHORT *, USHORT *, USHORT *, USHORT *);
BOOL reqBox(IMP, USHORT *, USHORT *, USHORT *, USHORT *, char *);
BOOL reqSector(IMP, USHORT *, USHORT *, char *);
BOOL reqChar(IMP, char *, char *, char *, char *);
BOOL reqCmsgpob(IMP, ItemType_t *, char *);
BOOL reqDesig(IMP, SectorType_t *, char *);
BOOL reqShipType(IMP, ShipType_t *, char *);
BOOL getPlayer(IMP, USHORT *);
BOOL reqPlayer(IMP, USHORT *, char *);
BOOL getRace(IMP, USHORT *, BOOL);
BOOL reqRace(IMP, USHORT *, BOOL, char *);
BOOL getChoice(IMP, USHORT *, char *);
BOOL reqChoice(IMP, USHORT *, char *, char *);
BOOL reqShip(IMP, ULONG *, char *);
BOOL getPlanet(IMP, register ULONG *);
BOOL reqPlanet(IMP, ULONG *, char *);
BOOL reqSectorOrShip(IMP, USHORT *, USHORT *, ULONG *, BOOL *, char *);
BOOL reqShipOrFleet(IMP, ULONG *, char *, char *);
BOOL reqPlanetOrShip(IMP, ULONG *, BOOL *, char *);
BOOL getBigItem(IMP, register ULONG *);
BOOL reqBigItem(IMP, register ULONG *, char *);

    /* commands.c */

void processCommands(IMP);

    /* scan.c */

BOOL getShips(IMP, ShipScan_t *);
BOOL reqShips(IMP, ShipScan_t *, char *);
ULONG scanShips(IMP, register ShipScan_t *,
    void (*)(IMP, ULONG, Ship_t *));
BOOL reqPlanets(IMP, PlanetScan_t *, char *);
ULONG scanPlanets(IMP, register PlanetScan_t *,  void (*)(IMP,
    register Planet_t *));
BOOL reqSectors(IMP, SectorScan_t *, char *);
ULONG scanSectors(IMP, register SectorScan_t *,  void (*)(IMP,
    USHORT, USHORT, register Sector_t *));
BOOL reqMiners(IMP, MinerScan_t *, char *);
ULONG scanMiners(IMP, register MinerScan_t *,
    void (*)(IMP, ULONG, Ship_t *));

    /* cmd_edit.c */

long repNum(IMP, long, long, long, char *);
void cmd_examine(IMP);
void cmd_edit(IMP);
void cmd_info(IMP);
void cmd_create(IMP);

    /* cmd_user.c */
Relation_t relationTo(USHORT, UBYTE, register Player_t *);
Relation_t getMyRelation(IMP, register Player_t *);
Relation_t getYourRelation(IMP, register Player_t *);
void cmd_player(IMP);
void cmd_status(IMP);
void cmd_race(IMP);
void cmd_realm(IMP);

    /* cmd_verify.c */

void cmd_verify(IMP);

    /* update.c */

ULONG maxPopulation(register Planet_t *);
USHORT calcPlagueFactor(IMP, register Planet_t *);
void updateShip(IMP);
void updateBigItem(IMP);
void updateMiner(IMP);
void updatePlanet(IMP);

    /* cmd_general1.c */

void cmd_change(IMP);
void cmd_census(IMP);
void cmd_enumerate(IMP);
BOOL cmd_designate(IMP);
BOOL cmd_checkpoint(IMP);
BOOL cmd_update(IMP);
void cmd_realm(IMP);
BOOL cmd_divvy(IMP);

    /* cmd_map.c */

void mapCoords(IMP, USHORT, USHORT);
void mapRowStart(IMP, USHORT);
void mapRowEnd(IMP, USHORT);
void mapEmpty(IMP);
void doLRScan(IMP, USHORT, USHORT, USHORT);
void doSRScan(IMP, USHORT, USHORT, USHORT);
void visShips(IMP, USHORT, USHORT, USHORT);
BOOL cmd_map(IMP);
BOOL cmd_scan(IMP);

    /* cmd_move.c */

BOOL cmd_fly(IMP);
Move_t navOnce(IMP, char, USHORT *, USHORT *, USHORT *, USHORT *);
BOOL cmd_navigate(IMP);
BOOL doLiftoff(IMP);
BOOL cmd_liftoff(IMP);
BOOL doLand(IMP);
BOOL cmd_land(IMP);

    /* cmd_general2.c */

BOOL cmd_discharge(IMP);
void cmd_power(IMP);
BOOL cmd_grant(IMP);
void cmd_dump(IMP);
void cmd_name(IMP);
BOOL cmd_plate(IMP);

    /* cmd_general3.c */

USHORT buildItemWeight(IMP, BigItem_t *);
BOOL cmd_build(IMP);
BOOL cmd_declare(IMP);
BOOL cmd_lend(IMP);
BOOL cmd_accept(IMP);
BOOL cmd_repay(IMP);
void cmd_ledger(IMP);
BOOL cmd_collect(IMP);
BOOL cmd_donate(IMP);

    /* cmd_trade.c */

char *getPartName(BigPart_t);
BOOL cmd_price(IMP);
BOOL cmd_report(IMP);
BOOL cmd_buy(IMP);
BOOL cmd_sell(IMP);

    /* cmd_naval.c */

BOOL cmd_ships(IMP);
BOOL cmd_load(IMP);
void cmd_fleet(IMP);
BOOL cmd_unload(IMP);
BOOL cmd_tend(IMP);
BOOL cmd_torpedo(IMP);
BOOL cmd_visual(IMP);
BOOL cmd_refurb(IMP);
BOOL cmd_install(IMP);
BOOL cmd_remove(IMP);

    /* cmd_fight.c */

BOOL cmd_attack(IMP);
BOOL cmd_fire(IMP);
BOOL cmd_board(IMP);
BOOL cmd_assault(IMP);
BOOL cmd_configure(IMP);


    /* cmd_naval2.c */

BOOL cmd_refuel(IMP);
BOOL cmd_program(IMP);
BOOL cmd_run(IMP);
void cmd_miner(IMP);


    /* cmd_action.c */

void cmd_setup(IMP);
void cmd_show(IMP);
BOOL handleAction(IMP, char);

    /* cmd_telep.c */

BOOL cmd_teleport(IMP);

    /* feSupp.c */

void fePlDirty(IMP, ULONG);
void feShDirty(IMP, ULONG);
void fePowerReport(IMP);
void feIteCen(IMP);
void feProCen(IMP);
void feProCen2(IMP);
void feGeoCen(IMP);
void fePopCen(IMP);
void feBigCen(IMP);
void feCheckReq(IMP, ULONG);
void feShBig(IMP);
void feShStat(IMP);
void feShConf(IMP);
void feShCargo(IMP);
void feRaceRep(IMP, USHORT);
void fePlayStat(IMP);
void fePlayList(IMP);
void fePrintRealm(IMP);
void feWorldSize(IMP);
