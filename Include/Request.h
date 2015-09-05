/*
 * Imperium
 *
 * Feel free to modify and use these sources however you wish, so long
 * as you preserve this copyright notice.
 *
 * $Id: Request.h,v 1.1.1.1 2000/05/17 19:15:04 marisa Exp $
 *
 * $Log: Request.h,v $
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:40  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:34:32  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:00:30  marisag
 * patch20: Fix empty revision.
 *
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

/* request structures passed between Imperium server(s) and clients */

#define REQUEST_PRIVATE_SIZE    2048
#define REQ_TEXT_LENGTH         REQUEST_PRIVATE_SIZE
#define TELEGRAM_MAX            (REQUEST_PRIVATE_SIZE - 80)

#define TELE_DELETE     (PLAYER_MAX + 1)
#define TELE_KEEP       (PLAYER_MAX + 2)

#define MESSAGE_SENT    0
#define MESSAGE_FAIL    1
#define MESSAGE_NO_PLAYER 2

/*
 * Types of requests:
 *
 * The 'read' requests just return the current value of the resource.
 * They will wait if someone has it locked. The 'lock' requests wait
 * to lock the resource, then return the current value. The 'unlock'
 * requests supply a new value for the resource and unlock it.
 */

/* note that I would like to use an enum type, but this wastes 3 bytes */
#define rt_nop                  0
#define rt_log                  1
#define rt_startClient          2
#define rt_stopClient           3
#define rt_shutDown             4
#define rt_flush                5
#define rt_poll                 6
#define rt_writeWorld           7
#define rt_acOut                8
#define rt_acBack               9
#define rt_battLow              10
#define rt_setPlayer            11
#define rt_readFile             12
#define rt_moreFile             13
#define rt_readHelp             14
#define rt_readDoc              15
#define rt_message              16
#define rt_getMessage           17
#define rt_setChat              18
#define rt_sendChat             19
#define rt_news                 20
#define rt_readNews             21
#define rt_propaganda           22
#define rt_readPropaganda       23
#define rt_sendTelegram         24
#define rt_checkMessages        25
#define rt_readTelegram         26
#define rt_writePower           27
#define rt_readPower            28
#define rt_readWorld            29
#define rt_lockWorld            30
#define rt_unlockWorld          31
#define rt_readPlayer           32
#define rt_lockPlayer           33
#define rt_unlockPlayer         34
#define rt_readSector           35
#define rt_lockSector           36
#define rt_unlockSector         37
#define rt_readShip             38
#define rt_lockShip             39
#define rt_unlockShip           40
#define rt_readFleet            41
#define rt_lockFleet            42
#define rt_unlockFleet          43
#define rt_readLoan             44
#define rt_lockLoan             45
#define rt_unlockLoan           46
#define rt_readOffer            47
#define rt_lockOffer            48
#define rt_unlockOffer          49
#define rt_readSectorPair       50
#define rt_lockSectorPair       51
#define rt_unlockSectorPair     52
#define rt_readShipPair         53
#define rt_lockShipPair         54
#define rt_unlockShipPair       55
#define rt_readSectorShipPair   56
#define rt_lockSectorShipPair   57
#define rt_unlockSectorShipPair 58
#define rt_createShip           59
#define rt_createFleet          60
#define rt_createLoan           61
#define rt_createOffer          62
#define rt_readPlanet           63
#define rt_lockPlanet           64
#define rt_unlockPlanet         65
#define rt_createPlanet         66
#define rt_readBigItem          67
#define rt_lockBigItem          68
#define rt_unlockBigItem        69
#define rt_createBigItem        70
#define rt_readPlanetShipPair   71
#define rt_lockPlanetShipPair   72
#define rt_unlockPlanetShipPair 73
#define rt_readPlanetItemPair   74
#define rt_lockPlanetItemPair   75
#define rt_unlockPlanetItemPair 76
#define rt_readPlanetPair       77
#define rt_lockPlanetPair       78
#define rt_unlockPlanetPair     79
#define rt_backStart            80
#define rt_backDone             81
#define rt_broadcastMsg         82
#define rt_publish              83

typedef UBYTE RequestType_t;

typedef struct
    {
        BOOL
            mc_newWorld,
            mc_newPlayer,
            mc_hasMessages,
            mc_hasNewTelegrams,
            mc_hasOldTelegrams;
    } MessageCheck_t;

typedef struct
    {
        USHORT
            te_to,
            te_from,
            te_length;
        ULONG
            te_time;
        char
            te_data[TELEGRAM_MAX];
    } Telegram_t;

typedef struct
    {
        USHORT
            pd_player,
            pd_plan;
        ULONG
            pd_civ,
            pd_mil,
            pd_shell,
            pd_gun,
            pd_plane,
            pd_bar,
            pd_effic;
        USHORT
            pd_ship;
        ULONG
            pd_tons,
            pd_power;
        long
            pd_money;
    } PowerData_t;

typedef struct
    {
        ULONG
            ph_lastTime;
        USHORT
            ph_playerCount;
    } PowerHead_t;

typedef struct
    {
        Sector_t    p_s;
        Ship_t      p_sh;
    } SectorShipPair_t;

typedef struct
    {
        Planet_t    p_p;
        Ship_t      p_sh;
    } PlanetShipPair_t;

typedef struct
    {
        Planet_t    p_p;
        BigItem_t   p_bi;
    } PlanetItemPair_t;

#define ISF_NONE      0x00L /* no flags set                             */
#define ISF_POW_WARN  0x01L /* Please log out, AC power out. This       */
                            /* should be broadcasted to the user every  */
                            /* 3 or 4 commands.                         */
#define ISF_POW_CRIT  0x02L /* Log out right away, battery power low.   */
                            /* This should be broadcasted to the user   */
                            /* EVERY command, and possibly FORCE a      */
                            /* log out after the 3rd or 4th time.       */
#define ISF_SERV_DOWN 0x04L /* Please log out, the server wants to go   */
                            /* down. This is just a general request     */
                            /* which you should broadcast every 5 or 10 */
                            /* commands                                 */
#define ISF_OFF_IMM   0x08L /* Log out immediately                      */
#define ISF_BACKUP    0x10L /* System wants to do a backup              */

typedef struct
    {
        ULONG
                    rq_clientId,
                    rq_time,
                    rq_whichUnit,
                    rq_otherUnit,
                    rq_specialFlags;
        RequestType_t
                    rq_type;
        char        rq_text[133];
        union
        {
            MessageCheck_t  ru_messageCheck;
            Telegram_t      ru_telegram;
            News_t          ru_news;
            PowerHead_t     ru_powerHead;
            PowerData_t     ru_powerData;
            World_t         ru_world;
            Player_t        ru_player;
            Sector_t        ru_sector;
            Ship_t          ru_ship;
            Fleet_t         ru_fleet;
            Loan_t          ru_loan;
            Offer_t         ru_offer;
            Ship_t          ru_shipPair[2];
            Planet_t        ru_planet;
            Planet_t        ru_planetPair[2];
            Sector_t        ru_sectorPair[2];
            PlanetShipPair_t
                            ru_planetShipPair;
            PlanetItemPair_t
                            ru_planetItemPair;
            SectorShipPair_t
                            ru_sectorShipPair;
            char            ru_text[REQ_TEXT_LENGTH];
            BigItem_t       ru_bigItem;
            BYTE            ru_private[REQUEST_PRIVATE_SIZE];
        } rq_u;
    } Request_t;
