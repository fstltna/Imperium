/*
 * Imperium
 *
 * $Id: ServerMain.c,v 1.5 2000/05/23 20:25:42 marisa Exp $
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
 * $Log: ServerMain.c,v $
 * Revision 1.5  2000/05/23 20:25:42  marisa
 * Added pl_weapon[] element to Planet_t struct
 * Added inclusion ofcrypt.hfor Linux
 *
 * Revision 1.4  2000/05/18 08:11:13  marisa
 * Autoconf working
 *
 * Revision 1.3  2000/05/18 06:41:52  marisa
 * More autoconf
 *
 * Revision 1.2  2000/05/18 06:32:08  marisa
 * Crypt, etc.
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:31:08  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:59:34  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:13:47  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:06:07  marisag
 * patch20: Fix empty rev.
 *
 */

#include "../config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
BOGUS - Imperium not supported on this machine due to missing stdlib.h
#endif
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#define _XOPEN_SOURCE
#include <unistd.h>
#include <crypt.h>
#else
BOGUS - Imperium not supported on this machine due to missing unistd.h
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <setjmp.h>
#include <signum.h>
#include <signal.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "Server.h"
#include "../impsec.h"

static const char rcsid[] = "$Id: ServerMain.c,v 1.5 2000/05/23 20:25:42 marisa Exp $";

#ifdef DEBUG_SERV
BOOL DO_DEBUG;
UBYTE DebugMode;
#endif

#define rk_world            0
#define rk_player           1
#define rk_sector           2
#define rk_ship             3
#define rk_fleet            4
#define rk_loan             5
#define rk_offer            6
#define rk_planet           7
#define rk_bigItem          8
#define rk_sectorPair       9
#define rk_shipPair         10
#define rk_sectorShipPair   11
#define rk_planetPair       12
#define rk_planetShipPair   13
#define rk_planetItemPair   14
typedef UBYTE ResourceKind_t;

#define DEF_SECTOR_CACHE_SIZE   30
#define DEF_SHIP_CACHE_SIZE     180
#define DEF_PLANET_CACHE_SIZE   60
#define DEF_BIGITEM_CACHE_SIZE  450
ULONG
    SectorCacheSize,
    ShipCacheSize,
    PlanetCacheSize,
    BigItemCacheSize;

struct messageListStruct
    {
        struct messageListStruct
            *ml_next;
        USHORT
            ml_length;
        char
            *ml_data;
    };
typedef struct messageListStruct MessageList_t;

struct clientStruct
    {
        struct clientStruct
            *cl_next,               /* next in any list of clients  */
            *cl_prev,               /* prev in any list of clients  */
            *cl_waitQueue,          /* queue waiting for what I have */
            *cl_masterNext,         /* next in master list          */
            *cl_masterPrev;         /* prev in master list          */
	int
	    cl_handle;		    /* the file handle to use for this client */
        MessageList_t
            *cl_messages;           /* messages waiting to go to him */
        ULONG
            cl_identifier;          /* client counter               */
        ResourceKind_t
            cl_lockKind;            /* kind of thing I have locked  */
        ULONG
            cl_resource,            /* identifier of what I have locked */
            cl_resource2,           /* identifier of other          */
            cl_playerNumber;        /* which player this client is  */
        FILE
            *cl_fd;                 /* file hndl for various things */
        BOOL
            cl_newWorld,            /* needs to see new world       */
            cl_newPlayer,           /* needs to see new player      */
            cl_chatting;            /* chat is enabled for this client */
    };
typedef struct clientStruct Client_t;

#define ft_normal   0
#define ft_help     1
#define ft_doc      2
typedef UBYTE FileType_t;

static World_t
    World;                  /* Global world struct      */

static Player_t
    Player[PLAYER_MAX];     /* Array of all players     */

int
    ImperiumPort;          /* Our message port         */
static char
    ImperiumPortName[255];

static Client_t
    *Clients,               /* master list of clients   */
    *IdleClients,           /* nothing locked, not waiting */
    *BusyClients;           /* holding something locked */
static ULONG
    ClientCounter;          /* count the clients active */
static char
    Path[257],              /* path to files            */
    HelpDir[257],           /* help file directory      */
    DocDir[257];            /* doc file directory       */

static BOOL
    ClearPlayers,           /* should we clear the p_loggedOn flag? */
    BackupBegun,            /* has the backup actually begun? */
    UseTest,                /* Use test port            */
    DoNews;                 /* Do we want to write news files? */

static ULONG
    SpecialFlags;           /* Contains special flags set to be set for */
                            /* all clients                              */

static Request_t
    LocalReq;

char
    BackReq[256];           /* holds the pipe name for a delayed reply */

static char
    PlayPort[256];	    /* the number of the port they want to use */

#define GOT_INT		1

static jmp_buf mainLoop;    /* holds the current system state for longjmp() */

int
	oldUmask;

void clearLogFlag(void)
{
    register int plNum;

    for (plNum=0; plNum < PLAYER_MAX; plNum++)
    {
        Player[plNum].p_loggedOn = 0;
    }
}


/*
 * copyData - supply appropriate data to the request.
 */

void copyData(Request_t *rq, ResourceKind_t rk)
{
    switch(rk)
    {
        case rk_world:
            rq->rq_u.ru_world = World;
            break;
        case rk_player:
            rq->rq_u.ru_player = Player[rq->rq_whichUnit];
            break;
        case rk_sector:
            readSector(rq->rq_whichUnit, &rq->rq_u.ru_sector);
            break;
        case rk_ship:
            readShip(rq->rq_whichUnit, &rq->rq_u.ru_ship);
            break;
        case rk_fleet:
            readFleet(rq->rq_whichUnit, &rq->rq_u.ru_fleet);
            break;
        case rk_loan:
            readLoan(rq->rq_whichUnit, &rq->rq_u.ru_loan);
            break;
        case rk_offer:
            readOffer(rq->rq_whichUnit, &rq->rq_u.ru_offer);
            break;
        case rk_planet:
            readPlanet(rq->rq_whichUnit, &rq->rq_u.ru_planet);
            break;
        case rk_bigItem:
            readBigItem(rq->rq_whichUnit, &rq->rq_u.ru_bigItem);
            break;
        case rk_sectorPair:
            readSector(rq->rq_whichUnit, &rq->rq_u.ru_sectorPair[0]);
            readSector(rq->rq_otherUnit, &rq->rq_u.ru_sectorPair[1]);
            break;
        case rk_shipPair:
            readShip(rq->rq_whichUnit, &rq->rq_u.ru_shipPair[0]);
            readShip(rq->rq_otherUnit, &rq->rq_u.ru_shipPair[1]);
            break;
        case rk_sectorShipPair:
            readSector(rq->rq_whichUnit, &rq->rq_u.ru_sectorShipPair.p_s);
            readShip(rq->rq_otherUnit, &rq->rq_u.ru_sectorShipPair.p_sh);
            break;
        case rk_planetPair:
            readPlanet(rq->rq_whichUnit, &rq->rq_u.ru_planetPair[0]);
            readPlanet(rq->rq_otherUnit, &rq->rq_u.ru_planetPair[1]);
            break;
        case rk_planetShipPair:
            readPlanet(rq->rq_whichUnit, &rq->rq_u.ru_planetShipPair.p_p);
            readShip(rq->rq_otherUnit, &rq->rq_u.ru_planetShipPair.p_sh);
            break;
        case rk_planetItemPair:
            readPlanet(rq->rq_whichUnit, &rq->rq_u.ru_planetItemPair.p_p);
            readBigItem(rq->rq_otherUnit, &rq->rq_u.ru_planetItemPair.p_bi);
            break;
    }
}

void DeletePort(void)
{
    close(ImperiumPort);
    (void)unlink(ImperiumPortName);
}

/*
 * validPswd - validate the given password and return TRUE if it is
 *             good, FALSE otherwise
 */

BOOL validPswd(register Request_t *rq, BOOL isShut)
{
    register char *pPtr;
#ifdef HAVE_CRYPT
    char locBuf[16], locSalt[3];
#endif

    pPtr = &rq->rq_u.ru_text[0];
    if (*pPtr == '\0')
    {
	return(FALSE);
    }
#ifdef HAVE_CRYPT
    locSalt[0] = pPtr[0];
    locSalt[1] = pPtr[1];
    if (isShut)
    {
	(void)strncpy(&locBuf[0], crypt(IMP_SHUT_CODE, &locSalt[0]), 15 *
	    sizeof(char));
	if (strcmp(pPtr, &locBuf[0]) == 0)
	{
	    return(TRUE);
	}
	return(FALSE);
    }
    (void)strncpy(&locBuf[0], crypt(CON_IMP_CODE, &locSalt[0]), 15 *
	sizeof(char));
    if (strcmp(pPtr, &locBuf[0]) == 0)
    {
	return(TRUE);
    }
    (void)strncpy(&locBuf[0], crypt(IMP_CTRL_CODE, &locSalt[0]), 15 *
	sizeof(char));
    if (strcmp(pPtr, &locBuf[0]) == 0)
#else /* !HAVE_CRYPT */
    if (isShut)
    {
	if (strcmp(pPtr, IMP_SHUT_CODE) == 0)
	{
	    return(TRUE);
	}
	return(FALSE);
    }
    if ((strcmp(pPtr, CON_IMP_CODE) == 0) ||
	(strcmp(pPtr, IMP_CTRL_CODE) == 0))
#endif
    {
	return(TRUE);
    }
    return(FALSE);
}

/*
 * startClient - a new client is starting up.
 */

BOOL startClient(register Request_t *rq)
{
    register Client_t *cl;

    cl = (Client_t *)calloc(1, sizeof(Client_t));
    if (cl == NULL)
    {
        rq->rq_type = rt_stopClient;
        log("*** can't allocate record for new client");
        return(FALSE);
    }
#ifdef zzz
    if (!validPswd(rq, FALSE))
    {
	free(cl);
        rq->rq_type = rt_stopClient;
        log("*** invalid registration password for client");
        return(FALSE);
    }
#endif
    /* general initialization of client structure */
    cl->cl_messages = NULL;
    cl->cl_identifier = ClientCounter;
    cl->cl_playerNumber = NO_OWNER;
    cl->cl_fd = NULL;
    cl->cl_newWorld = FALSE;
    cl->cl_newPlayer = FALSE;
    cl->cl_chatting = FALSE;
    /* put new client on chain of idle clients */
    cl->cl_next = IdleClients;
    cl->cl_prev = NULL;
    cl->cl_handle = open(&rq->rq_text[0], O_WRONLY | O_NDELAY, 0);
    if (cl->cl_handle < 0)
    {
	free(cl);
        rq->rq_type = rt_stopClient;
	log("Unable to open client pipe in startClient()");
	return(FALSE);
    }
    rq->rq_clientId = ClientCounter;
    if (IdleClients != NULL)
    {
        IdleClients->cl_prev = cl;
    }
    IdleClients = cl;
    /* put new client on master client chain */
    cl->cl_masterNext = Clients;
    cl->cl_masterPrev = NULL;
    if (Clients != NULL)
    {
        Clients->cl_masterPrev = cl;
    }
    Clients = cl;
    ClientCounter++;
    return(TRUE);
}

/*
 * stopClient - a client is shutting down.
 */

void stopClient(register Client_t *cl, register Request_t *rq)
{
    register MessageList_t *ml, *mlt;

    /* Send them the message that indicates they can go bye-bye */
    (void)write(cl->cl_handle, (char *)rq, sizeof(Request_t));
    /* Close their file pointer */
    close(cl->cl_handle);
    /* delete client from chain of idle clients */
    if (cl->cl_next != NULL)
    {
        cl->cl_next->cl_prev = cl->cl_prev;
    }
    if (cl->cl_prev != NULL)
    {
        cl->cl_prev->cl_next = cl->cl_next;
    }
    else
    {
        IdleClients = cl->cl_next;
    }
    /* delete client from master client chain */
    if (cl->cl_masterNext != NULL)
    {
        cl->cl_masterNext->cl_masterPrev = cl->cl_masterPrev;
    }
    if (cl->cl_masterPrev != NULL)
    {
        cl->cl_masterPrev->cl_masterNext = cl->cl_masterNext;
    }
    else
    {
        Clients = cl->cl_masterNext;
    }
    /* delete any messages that slipped in */
    ml = cl->cl_messages;
    while (ml)
    {
        free(ml->ml_data);
        mlt = ml;
        ml = ml->ml_next;
        free(mlt);
    }
    if ((cl->cl_playerNumber != 0) && (cl->cl_playerNumber != NO_OWNER))
    {
        Player[cl->cl_playerNumber].p_loggedOn = FALSE;
    }
    free(cl);
    /* we don't want to write any files when doing a backup */
    if (!BackupBegun)
    {
        writeWorld(&World, &Player[0]);
        if (World.w_doFlush)
        {
           closeFiles();
           openFiles(&Path[0], &HelpDir[0], &DocDir[0]);
        }
    }
}

/*
 * readFile - starting a normal, help or doc file.
 */

void readFile(register Client_t *cl, register Request_t *rq, FileType_t ft)
{
    char fileName[257];
    register long len;

    switch(ft)
    {
        case ft_normal:
            strcpy(&fileName[0], &Path[0]);
            break;
        case ft_help:
            strcpy(&fileName[0], &HelpDir[0]);
            break;
        case ft_doc:
            strcpy(&fileName[0], &DocDir[0]);
            break;
    }
    strcat(&fileName[0], &rq->rq_u.ru_text[0]);

    /* tack on any needed extension */
    switch(ft)
    {
        case ft_help:
            strcat(&fileName[0], ".hlp");
            break;
        case ft_doc:
            strcat(&fileName[0], ".doc");
            break;
        default:
            break;
    }
    /* attempt to open the file */
    cl->cl_fd = fopen(&fileName[0], "r");
    if (cl->cl_fd == NULL)
    {
        rq->rq_whichUnit = 0;
        return;
    }
    len = fread(&rq->rq_u.ru_text[0], sizeof(char),
	REQ_TEXT_LENGTH * sizeof(char), cl->cl_fd);
    if (len != REQ_TEXT_LENGTH)
    {
        fclose(cl->cl_fd);
        cl->cl_fd = NULL;
        if (len < 0)
        {
            len = 0;
        }
    }
    rq->rq_whichUnit = len;
}

/*
 * moreFile - continue reading a file.
 */

void moreFile(register Client_t *cl, register Request_t *rq)
{
    register long len;

    if (cl->cl_fd == NULL)
    {
        rq->rq_whichUnit = 0;
        return;
    }
    len = fread(&rq->rq_u.ru_text[0], sizeof(char), REQ_TEXT_LENGTH, cl->cl_fd);
    if (len != REQ_TEXT_LENGTH)
    {
        fclose(cl->cl_fd);
        cl->cl_fd = NULL;
        if (len < 0)
        {
            len = 0;
        }
    }
    rq->rq_whichUnit = len;
}

/*
 * appendMessage - add a message from the request to the end of the
 *      client's message queue.
 */

void appendMessage(register Client_t *cl, register Request_t *rq)
{
    char *NOBODY_HEADER = "******:\n";
    USHORT NOBODY_LENGTH = 8;
    register MessageList_t **mlp;
    register MessageList_t *ml;

    mlp = &cl->cl_messages;
    while (*mlp)
    {
        mlp = &(*mlp)->ml_next;
    }
    ml = (MessageList_t *)calloc(1, sizeof(MessageList_t));
    if (ml)
    {
        ml->ml_next = NULL;
        if (rq->rq_u.ru_telegram.te_from == NOBODY)
        {
            ml->ml_length = rq->rq_u.ru_telegram.te_length + NOBODY_LENGTH;
        }
        else
        {
            ml->ml_length = rq->rq_u.ru_telegram.te_length;
        }
        ml->ml_data = (char *)calloc(1, ml->ml_length * sizeof(char));
        if (ml->ml_data)
        {
            if (rq->rq_u.ru_telegram.te_from == NOBODY)
            {
                memcpy(ml->ml_data, NOBODY_HEADER, NOBODY_LENGTH);
                memcpy(ml->ml_data + NOBODY_LENGTH * sizeof(char),
                          &rq->rq_u.ru_telegram.te_data[0],
                          rq->rq_u.ru_telegram.te_length);
            }
            else
            {
                memcpy(ml->ml_data, &rq->rq_u.ru_telegram.te_data[0],
                          ml->ml_length);
            }
            *mlp = ml;
            rq->rq_whichUnit = MESSAGE_SENT;
#ifdef DEBUG_SERV
            if (DO_DEBUG)
            {
                log("message appended to queue");
            }
#endif
        }
        else
        {
            free(ml);
            rq->rq_whichUnit = MESSAGE_FAIL;
#ifdef DEBUG_SERV
            if (DO_DEBUG)
            {
                log("*** message append failed - no mem");
            }
#endif
        }
        return;
    }
    rq->rq_whichUnit = MESSAGE_FAIL;
#ifdef DEBUG_SERV
    if (DO_DEBUG)
    {
        log("*** message append failed - no mem for messageList");
    }
#endif
}

/*
 * message - send a message (in telegram structure) to other user.
 */

void message(register Request_t *rq)
{
    register Client_t *cl;
    BOOL doCont;

    cl = Clients;
    if (cl != NULL)
    {
        doCont = (cl->cl_playerNumber != rq->rq_whichUnit);
        while (doCont)
        {
            cl = cl->cl_masterNext;
            if (cl != NULL)
            {
                doCont = (cl->cl_playerNumber != rq->rq_whichUnit);
            }
            else
            {
                doCont = FALSE;
            }
        }
    }
    if (cl != NULL)
    {
        appendMessage(cl, rq);
        return;
    }
    rq->rq_whichUnit = MESSAGE_NO_PLAYER;
}

/*
 * getMessage - retrieve the next message destined for this client.
 */

void getMessage(register Client_t *cl, register Request_t *rq)
{
    register MessageList_t *ml;

    ml = cl->cl_messages;
    if (ml)
    {
        rq->rq_u.ru_telegram.te_length = ml->ml_length;
        memcpy(&rq->rq_u.ru_telegram.te_data[0], ml->ml_data,ml->ml_length);
        free(ml->ml_data);
        cl->cl_messages = ml->ml_next;
        free(ml);
        return;
    }
    rq->rq_u.ru_telegram.te_length = 0;
}

/*
 * startChat - send the new chatter a list of the current chatters.
 */

void startChat(register Client_t *clNew)
{
    register Client_t *cl;
    register MessageList_t *ml;
    register USHORT len;

    cl = Clients;
    while (cl != NULL)
    {
        if ((cl != clNew) && cl->cl_chatting)
        {
            ml = (MessageList_t *)calloc(1, sizeof(MessageList_t));
            if (ml)
            {
                len = strlen(&Player[cl->cl_playerNumber].p_name[0]);
                ml->ml_data = (char *)calloc(1, (len + 12) * sizeof(char));
                if (ml->ml_data)
                {
                    memcpy(ml->ml_data, "Chatting: ", 10);
                    memcpy(ml->ml_data + 10 * sizeof(char),
                              &Player[cl->cl_playerNumber].p_name[0], len);
                    *(ml->ml_data + (len + 10) * sizeof(char)) = '\n';
                    *(ml->ml_data + (len + 11) * sizeof(char)) = '\0';
                    ml->ml_length = len + 12;
                    ml->ml_next = clNew->cl_messages;
                    clNew->cl_messages = ml;
                }
                else
                {
                    free(ml);
                }
            }
        }
        cl = cl->cl_masterNext;
    }
}

/*
 * sendChat - this client has sent a new chat line.
 *      Copy it to the end of all other chatting client's message queues.
 */

void sendChat(register Client_t *sender, register Request_t *rq)
{
    register Client_t *cl;

    cl = Clients;
    while (cl != NULL)
    {
        if ((cl != sender) && cl->cl_chatting)
        {
            appendMessage(cl, rq);
        }
        cl = cl->cl_masterNext;
    }
}

/*
 * setFileName - fill in a file name for something for the given time's day.
 */

void setFileName(register char *buffer, register ULONG fTime, char *what)
{
    fTime = fTime / (World.w_secondsPerITU * (2 * 24));
    sprintf(buffer, "%s%s.%4.4lu", &Path[0], what, fTime);
}

/*
 * news - add a news item to today's news file.
 */

void news(register Request_t *rq)
{
    ULONG now;
    FILE *fd;
    char fileName[257];

    (void) time(&now);
    rq->rq_u.ru_news.n_time = now;
    setFileName(&fileName[0], now, "news");
    fd = fopen(&fileName[0], "ab");
    if (fd != NULL)
    {
        (void) fwrite(&rq->rq_u.ru_news, sizeof(char), sizeof(News_t), fd);
        fclose(fd);
    }
}

/*
 * readNews - get the indicated news item. Lots of file open/closes!
 */

void readNews(register Request_t *rq)
{
    FILE *fd;
    char fileName[257];
    long len;

    setFileName(&fileName[0], rq->rq_u.ru_news.n_time, "news");
    fd = fopen(&fileName[0], "rb");
    if (fd == NULL)
    {
        rq->rq_whichUnit = 0;
        return;
    }
    if (fseek(fd, (ULONG)(rq->rq_whichUnit) * sizeof(News_t), SEEK_SET))
    {
        rq->rq_whichUnit = 0;
    }
    else
    {
        len = fread(&rq->rq_u.ru_news, sizeof(char), sizeof(News_t), fd);
        if (len != sizeof(News_t))
        {
            rq->rq_whichUnit = 0;
        }
        else
        {
            rq->rq_whichUnit = rq->rq_whichUnit + 1;
        }
    }
    fclose(fd);
}

/*
 * propaganda - enter a piece of propaganda.
 */

void propaganda(register Request_t *rq)
{
    FILE *fd;
    char fileName[257];
    long len;

    setFileName(&fileName[0], rq->rq_u.ru_telegram.te_time, "prop");
    fd = fopen(&fileName[0], "ab");
    if (fd != NULL)
    {
        len = sizeof(Telegram_t) - (TELEGRAM_MAX * sizeof(char));
        if (fwrite(&rq->rq_u.ru_telegram, sizeof(char), len, fd) != len)
	{
            log("*** write of propaganda header failed");
	}
	else
	{
            len = rq->rq_u.ru_telegram.te_length;
	    if (fwrite(&rq->rq_u.ru_telegram.te_data[0], sizeof(char), len, fd) != len)
	    {
		log("*** write of propaganda body failed");
	    }
	}
        fclose(fd);
        return;
    }
    log("*** write open of propaganda file failed");
}

/*
 * readPropaganda - reand an item of propaganda.
 */

void readPropaganda(register Client_t *cl, register Request_t *rq)
{
    char fileName[257];
    long len;

    /* See if we are already reading a propaganda file */
    if (cl->cl_fd == NULL)
    {
	/* Open the propaganda file for the day requested */
        setFileName(&fileName[0], rq->rq_u.ru_telegram.te_time, "prop");
        cl->cl_fd = fopen(&fileName[0], "rb");
        if (cl->cl_fd == NULL)
        {
            rq->rq_u.ru_telegram.te_length = 0;
        }
    }
    /* See if we were able to open the file */
    if (cl->cl_fd != NULL)
    {
	/* Get the length of the header information */
        len = sizeof(Telegram_t) - (TELEGRAM_MAX * sizeof(char));
        if (fread(&rq->rq_u.ru_telegram, sizeof(char), len, cl->cl_fd) != len)
        {
            rq->rq_u.ru_telegram.te_length = 0;
            fclose(cl->cl_fd);
            cl->cl_fd = NULL;
        }
        else
        {
            len = rq->rq_u.ru_telegram.te_length;
            if (fread(&rq->rq_u.ru_telegram.te_data[0], sizeof(char), len,
		cl->cl_fd) != len)
            {
                rq->rq_u.ru_telegram.te_length = 0;
                fclose(cl->cl_fd);
                cl->cl_fd = NULL;
                log("*** read of propaganda body failed");
            }
        }
    }
}

/*
 * sendTelegram - send the telegram from the request.
 */

void sendTelegram(register Request_t *rq)
{
    register Player_t *player;
    register FILE *fd;
    char fileName[257];
    register ULONG len;

    sprintf(&fileName[0], "%stelegrams.%2.2hd", &Path[0],
	rq->rq_u.ru_telegram.te_to);
    fd = fopen(&fileName[0], "r+b");
    if (fd != NULL)
    {
        player = &Player[rq->rq_u.ru_telegram.te_to];
        if (!fseek(fd, player->p_telegramsTail, SEEK_SET))
        {
	    /* Try and write the telegram header */
            len = sizeof(Telegram_t) - (TELEGRAM_MAX * sizeof(char));
            if (fwrite(&rq->rq_u.ru_telegram, sizeof(char), len, fd)
		!= len)
	    {
                log("*** write of telegram header failed");
	    }
	    else
	    {
		len = rq->rq_u.ru_telegram.te_length;
		if (fwrite(&rq->rq_u.ru_telegram.te_data[0], sizeof(char),
		    len, fd) == len)
		{
		    player->p_telegramsTail += (len + (sizeof(Telegram_t)
			- (TELEGRAM_MAX * sizeof(char))));
		}
		else
		{
		    log("*** write of telegram body failed");
		}
	    }
        }
        else
        {
            log("*** seek to write telegram file failed");
        }
        fclose(fd);
        return;
    }
    log("*** write open of telegram file failed");
}

/*
 * checkMessages - tell about various things waiting for the client.
 */

void checkMessages(register Client_t *cl, register Request_t *rq)
{
    register Player_t *player;

    player = &Player[rq->rq_whichUnit];
    if (cl->cl_newWorld)
    {
        rq->rq_u.ru_messageCheck.mc_newWorld = TRUE;
        cl->cl_newWorld = FALSE;
    }
    else
    {
        rq->rq_u.ru_messageCheck.mc_newWorld = FALSE;
    }
    if (cl->cl_newPlayer)
    {
        rq->rq_u.ru_messageCheck.mc_newPlayer = TRUE;
        cl->cl_newPlayer = FALSE;
    }
    else
    {
        rq->rq_u.ru_messageCheck.mc_newPlayer = FALSE;
    }
    rq->rq_u.ru_messageCheck.mc_hasMessages = (cl->cl_messages != NULL);
    rq->rq_u.ru_messageCheck.mc_hasOldTelegrams = (player->p_telegramsNew != 0);
    if (player->p_telegramsTail != player->p_telegramsNew)
    {
        rq->rq_u.ru_messageCheck.mc_hasNewTelegrams = TRUE;
        player->p_telegramsNew = player->p_telegramsTail;
        return;
    }
    rq->rq_u.ru_messageCheck.mc_hasNewTelegrams = FALSE;
    rq->rq_specialFlags = SpecialFlags;
}

/*
 * readTelegram - read a telegram from the user.
 */

void readTelegram(register Client_t *cl, register Request_t *rq)
{
    register Player_t *player;
    char fileName[257];
    register long len;
    FILE *tempPtr;

    player = &Player[cl->cl_playerNumber];
    rq->rq_u.ru_telegram.te_length = 0;
    if ((cl->cl_fd == NULL) && (player->p_telegramsTail != 0))
    {
        switch(rq->rq_whichUnit)
        {
            case TELE_DELETE:
                player->p_telegramsTail = 0;
                player->p_telegramsNew = 0;
		sprintf(&fileName[0], "%stelegrams.%2.2hd", &Path[0],
		    cl->cl_playerNumber);
                (void) unlink(&fileName[0]);
                tempPtr = fopen(&fileName[0], "w");
		if (tempPtr != NULL)
		{
                    fclose(tempPtr);
		}
                break;
            case TELE_KEEP:
                player->p_telegramsNew = player->p_telegramsTail;
                break;
            default:
                /* first read of telegrams */
		sprintf(&fileName[0], "%stelegrams.%2.2hd", &Path[0],
		    cl->cl_playerNumber);
                cl->cl_fd = fopen(&fileName[0], "r+b");
                if (cl->cl_fd == NULL)
                {
                    rq->rq_u.ru_telegram.te_length = 0;
                    log("*** read open of telegram file failed");
                }
                break;
        }
    }
    if (cl->cl_fd != NULL)
    {
        if (ftell(cl->cl_fd) >= (long)(player->p_telegramsTail))
        {
            fclose(cl->cl_fd);
            cl->cl_fd = NULL;
        }
        else
        {
	    /* Try and read in the telegram header */
            len = sizeof(Telegram_t) - (TELEGRAM_MAX * sizeof(char));
            if (fread(&rq->rq_u.ru_telegram, sizeof(char), len, cl->cl_fd)
		!= len)
            {
                rq->rq_u.ru_telegram.te_length = 0;
                fclose(cl->cl_fd);
                cl->cl_fd = NULL;
                log("*** read of telegram header failed");
            }
            else
            {
                len = rq->rq_u.ru_telegram.te_length;
                if (fread(&rq->rq_u.ru_telegram.te_data[0], sizeof(char), len,
		    cl->cl_fd) != len)
                {
                    rq->rq_u.ru_telegram.te_length = 0;
                    fclose(cl->cl_fd);
                    cl->cl_fd = NULL;
                    log("*** read of telegram body failed");
                }
            }
        }
    }
}

/*
 * writePower - write a new power file.
 */

void writePower(register Client_t *cl, register Request_t *rq)
{
    char fileName[257];

    if (cl->cl_fd == NULL)
    {
        /* try to open the power file and write the header */
        strcpy(&fileName[0], &Path[0]);
        strcat(&fileName[0], POWER_FILE);
        cl->cl_fd = fopen(&fileName[0], "wb");
        if (cl->cl_fd == NULL)
        {
            /* can't create it! */
            log("*** can't create power file");
            rq->rq_whichUnit = 0;
        }
        else
        {
            if (fwrite(&rq->rq_u.ru_powerHead, sizeof(char), sizeof(PowerHead_t),
		cl->cl_fd) != sizeof(PowerHead_t))
            {
                log("*** bad header write on power file");
                /* can't write it! */
                fclose(cl->cl_fd);
                cl->cl_fd = NULL;
                rq->rq_whichUnit = 0;
            }
            else
            {
                /* all OK */
                rq->rq_whichUnit = 1;
            }
        }
        return;
    }
    /* file already open, just write the next chunk, or close it */
    if (rq->rq_whichUnit == 0)
    {
        fclose(cl->cl_fd);
        cl->cl_fd = NULL;
        return;
    }
    if (fwrite(&rq->rq_u.ru_powerData, sizeof(char), sizeof(PowerData_t),
	cl->cl_fd) != sizeof(PowerData_t))
    {
        log("*** bad write of data on power file");
        fclose(cl->cl_fd);
        cl->cl_fd = NULL;
        rq->rq_whichUnit = 0;
    }
}

/*
 * readPower - read the current power file.
 */

void readPower(register Client_t *cl, register Request_t *rq)
{
    char fileName[257];

    if (cl->cl_fd == NULL)
    {
        strcpy(&fileName[0], &Path[0]);
        strcat(&fileName[0], POWER_FILE);
        cl->cl_fd = fopen(&fileName[0], "rb");
        if (cl->cl_fd == NULL)
        {
            rq->rq_whichUnit = 0;
        }
        else
        {
            if (fread(&rq->rq_u.ru_powerHead, sizeof(char), sizeof(PowerHead_t),
		cl->cl_fd) != sizeof(PowerHead_t))
            {
                log("*** bad header read on power file");
                fclose(cl->cl_fd);
                cl->cl_fd = NULL;
                rq->rq_whichUnit = 0;
            }
            else
            {
                rq->rq_whichUnit = 1;
            }
        }
        return;
    }
    if (fread(&rq->rq_u.ru_powerData, sizeof(char), sizeof(PowerData_t),
	cl->cl_fd) != sizeof(PowerData_t))
    {
        fclose(cl->cl_fd);
        cl->cl_fd = NULL;
        rq->rq_whichUnit = 0;
        return;
    }
    rq->rq_whichUnit = 1;
}

/*
 * noLockConflict - return TRUE if cl2 has nothing locked that prevents
 *          cl1 from going ahead.
 *          ra1 & rb1 are the first args for the two clients, and ra2 & rb2
 *          are the 2nd args (if any) for the two clients
 */

BOOL noLockConflict(register Client_t *cl, register Client_t *cl2)
{
    register ULONG ra1, ra2, rb1, rb2;

    ra1 = cl->cl_resource;
    rb1 = cl2->cl_resource;
    if ((cl->cl_lockKind == cl2->cl_lockKind) && (ra1 == rb1))
    {
        return FALSE;
    }
    ra2 = cl->cl_resource2;
    rb2 = cl2->cl_resource2;
    /* switch based on what we are trying to lock */
    switch(cl->cl_lockKind)
    {
        case rk_sector:
            /* check for any kind of sector lock */
            if (cl2->cl_lockKind == rk_sectorPair)
            {
                /* check to see if we are locking the same item */
                if ((ra1 == rb1) || (ra1 == rb2))
                {
                    return FALSE;
                }
            }
            else
            {
                if (cl2->cl_lockKind == rk_sectorShipPair)
                {
                    if (ra1 == rb1)
                    {
                        return FALSE;
                    }
                }
            }
            break;

        case rk_planet:
            /* check for any kind of planet lock */
            if (cl2->cl_lockKind == rk_planetPair)
            {
                /* check to see if we are locking the same item */
                if ((ra1 == rb1) || (ra1 == rb2))
                {
                    return FALSE;
                }
            }
            else
            {
                if (cl2->cl_lockKind == rk_planetShipPair)
                {
                    if (ra1 == rb1)
                    {
                        return FALSE;
                    }
                }
                else
                {
                    if (cl2->cl_lockKind == rk_planetItemPair)
                    {
                        if (ra1 == rb1)
                        {
                            return FALSE;
                        }
                    }
                }
            }
            break;

        case rk_sectorPair:
            if ((cl2->cl_lockKind == rk_sector) ||
                (cl2->cl_lockKind == rk_sectorShipPair))
            {
                if ((ra1 == rb1) || (ra2 == rb1))
                {
                    return FALSE;
                }
            }
            else
            {
                if (cl2->cl_lockKind == rk_sectorPair)
                {
                    if ((ra1 == rb1) || (ra2 == rb1) || (ra1 == rb2) ||
                        (ra2 == rb2))
                    {
                        return FALSE;
                    }
                }
            }
            break;

        case rk_planetPair:
            /* check for any kind of planet combination */
            if ((cl2->cl_lockKind == rk_planet) ||
                (cl2->cl_lockKind == rk_planetShipPair) ||
                (cl2->cl_lockKind == rk_planetItemPair))
            {
                if ((ra1 == rb1) || (ra2 == rb1))
                {
                    return FALSE;
                }
            }
            else
            {
                if (cl2->cl_lockKind == rk_planetPair)
                {
                    if ((ra1 == rb1) || (ra2 == rb1) || (ra1 == rb2) ||
                        (ra2 == rb2))
                    {
                        return FALSE;
                    }
                }
            }
            break;

        case rk_ship:
            if (cl2->cl_lockKind == rk_shipPair)
            {
                if ((ra1 == rb1) || (ra1 == rb2))
                {
                    return FALSE;
                }
            }
            else
            {
                /* put the most common check first */
                if (cl2->cl_lockKind == rk_planetShipPair)
                {
                    if (ra1 == rb2)
                    {
                        return FALSE;
                    }
                }
                else
                {
                    if (cl2->cl_lockKind == rk_sectorShipPair)
                    {
                        if (ra1 == rb2)
                        {
                            return FALSE;
                        }
                    }
                }
            }
            break;

        case rk_bigItem:
            if (cl2->cl_lockKind == rk_planetItemPair)
            {
                if (ra1 == rb2)
                {
                    return FALSE;
                }
            }
            break;

        case rk_shipPair:
            if (cl2->cl_lockKind == rk_ship)
            {
                if ((ra1 == rb1) || (ra2 == rb1))
                {
                    return FALSE;
                }
            }
            else
            {
                if (cl2->cl_lockKind == rk_shipPair)
                {
                    if ((ra1 == rb1) || (ra2 == rb1) || (ra1 == rb2) ||
                        (ra2 == rb2))
                    {
                        return FALSE;
                    }
                }
                else
                {
                    if (cl2->cl_lockKind == rk_planetShipPair)
                    {
                        if ((ra1 == rb2) || (ra2 == rb2))
                        {
                            return FALSE;
                        }
                    }
                    else
                    {
                        if (cl2->cl_lockKind == rk_sectorShipPair)
                        {
                            if ((ra1 == rb2) || (ra2 == rb2))
                            {
                                return FALSE;
                            }
                        }
                    }
                }
            }
            break;

        case rk_sectorShipPair:
            switch (cl2->cl_lockKind)
            {
                case rk_sector:
                    if (ra1 == rb1)
                    {
                        return FALSE;
                    }
                    break;
                case rk_ship:
                    if (ra2 == rb1)
                    {
                        return FALSE;
                    }
                    break;
                case rk_sectorPair:
                    if ((ra1 == rb1) || (ra1 == rb2))
                    {
                        return FALSE;
                    }
                    break;
                case rk_shipPair:
                    if ((ra2 == rb1) || (ra2 == rb2))
                    {
                        return FALSE;
                    }
                    break;
                case rk_sectorShipPair:
                    if ((ra1 == rb1) || (ra2 == rb2))
                    {
                        return FALSE;
                    }
                    break;
            }
            break;

        case rk_planetShipPair:
            switch (cl2->cl_lockKind)
            {
                case rk_planet:
                    if (ra1 == rb1)
                    {
                        return FALSE;
                    }
                    break;
                case rk_ship:
                    if (ra2 == rb1)
                    {
                        return FALSE;
                    }
                    break;
                case rk_planetPair:
                    if ((ra1 == rb1) || (ra1 == rb2))
                    {
                        return FALSE;
                    }
                    break;
                case rk_shipPair:
                    if ((ra2 == rb1) || (ra2 == rb2))
                    {
                        return FALSE;
                    }
                    break;
                case rk_planetShipPair:
                    if ((ra1 == rb1) || (ra2 == rb2))
                    {
                        return FALSE;
                    }
                    break;
                case rk_planetItemPair:
                    if (ra1 == rb1)
                    {
                        return FALSE;
                    }
                    break;
            }
            break;

        case rk_planetItemPair:
            switch (cl2->cl_lockKind)
            {
                case rk_planet:
                    if (ra1 == rb1)
                    {
                        return FALSE;
                    }
                    break;
                case rk_bigItem:
                    if (ra2 == rb1)
                    {
                        return FALSE;
                    }
                    break;
                case rk_planetPair:
                    if ((ra1 == rb1) || (ra1 == rb2))
                    {
                        return FALSE;
                    }
                    break;
                case rk_planetShipPair:
                    if (ra1 == rb1)
                    {
                        return FALSE;
                    }
                    break;
                case rk_planetItemPair:
                    if ((ra1 == rb1) || (ra2 == rb2))
                    {
                        return FALSE;
                    }
                    break;
            }
            break;
    }
    /* drop through with TRUE, since we had no match */
    return TRUE;
}

/*
 * handleLockRequest - try to handle a lock request, either arriving as a
 *      new request or as a result of an unlock.
 */

void handleLockRequest(register Client_t *cl)
{
    register Client_t *cl2;
    register Client_t **pcl;
    BOOL doCont;

    cl2 = BusyClients;
    if (cl2 != NULL)
    {
        doCont = noLockConflict(cl, cl2);
        while (doCont)
        {
            cl2 = cl2->cl_next;
            if (cl2 != NULL)
            {
                doCont = noLockConflict(cl, cl2);
            }
            else
            {
                doCont = FALSE;
            }
        }
    }
    if (cl2 == NULL)
    {
        /* nobody has it locked - go ahead and lock it */
        cl->cl_next = BusyClients;
        cl->cl_prev = NULL;
        cl->cl_waitQueue = NULL;
        if (BusyClients != NULL)
        {
            BusyClients->cl_prev = cl;
        }
        BusyClients = cl;
        copyData(&LocalReq, cl->cl_lockKind);
        LocalReq.rq_specialFlags = SpecialFlags;
        if (write(cl->cl_handle, (char *)&LocalReq, sizeof(Request_t)) < 0)
	{
	    DeletePort();
	    myAbort("can't send message for handleLockRequest");
	}
        return;
    }
    /* it is locked - we have to wait for it by putting
       ourselves on the back of the queue waiting for it */
    pcl = &cl2->cl_waitQueue;
    while (*pcl)
    {
        pcl = &(*pcl)->cl_next;
    }
    *pcl = cl;
    cl->cl_next = NULL;
}

/*
 * getRKType - gets the correct ResourceKind_t for the given request
 *          Should find a better way to do this without being as inflexible
 *          as the Empire method.
 */

ResourceKind_t getRKType(register RequestType_t rt)
{
    register ResourceKind_t rk = rk_world;

    switch(rt)
    {
        case rt_readWorld:
        case rt_lockWorld:
            rk = rk_world;
            break;
        case rt_readPlayer:
        case rt_lockPlayer:
            rk = rk_player;
            break;
        case rt_readSector:
        case rt_lockSector:
            rk = rk_sector;
            break;
        case rt_readShip:
        case rt_lockShip:
            rk = rk_ship;
            break;
        case rt_readFleet:
        case rt_lockFleet:
            rk = rk_fleet;
            break;
        case rt_readPlanet:
        case rt_lockPlanet:
            rk = rk_planet;
            break;
        case rt_readBigItem:
        case rt_lockBigItem:
            rk = rk_bigItem;
            break;
        case rt_readLoan:
        case rt_lockLoan:
            rk = rk_loan;
            break;
        case rt_readOffer:
        case rt_lockOffer:
            rk = rk_offer;
            break;
        case rt_readSectorPair:
        case rt_lockSectorPair:
            rk = rk_sectorPair;
            break;
        case rt_readPlanetPair:
        case rt_lockPlanetPair:
            rk = rk_planetPair;
            break;
        case rt_readShipPair:
        case rt_lockShipPair:
            rk = rk_shipPair;
            break;
        case rt_readSectorShipPair:
        case rt_lockSectorShipPair:
            rk = rk_sectorShipPair;
            break;
        case rt_readPlanetShipPair:
        case rt_lockPlanetShipPair:
            rk = rk_planetShipPair;
            break;
        case rt_readPlanetItemPair:
        case rt_lockPlanetItemPair:
            rk = rk_planetItemPair;
            break;
        default:
            log("*** unknown request type in getRKType");
            break;
    }
    return rk;
}

/*
 * This function gets called when we receive interupt signals
 */
void int_handler(int arg1)
{
	longjmp(mainLoop, GOT_INT);
}

/*
 * handleRequests - wait for and handle requests from clients.
 */

void handleRequests(void)
{
    register Request_t *rq;
    register Client_t *cl = NULL, *cl2;
    register Player_t *player;
    register BOOL BatGood, badBack;
    BOOL quitRequested, doCont, ACOut;
    USHORT NumPlay;
    fd_set readPipe;
    int jmpCode, nsel, backFile;

    ClientCounter = 1;
    NumPlay = 0;

    /* quitRequested is true if a USER requests the server shut down */
    quitRequested = FALSE;

    /* ACOut is true if line AC line voltage is lost */
    ACOut = FALSE;

    /* BatGood is false if we should shut down right away */
    BatGood = TRUE;

    /* Try to enable signal trapping */
    if (signal(SIGINT, int_handler) == SIG_ERR)
    {
	BatGood = FALSE;
        log("Unable to set SIGINT handler");
    } else if (signal(SIGHUP, int_handler) == SIG_ERR)
    {
	BatGood = FALSE;
        log("Unable to set SIGHUP handler");
    } else if (signal(SIGTERM, int_handler) == SIG_ERR)
    {
	BatGood = FALSE;
        log("Unable to set SIGTERM handler");
    } else if (signal(SIGQUIT, int_handler) == SIG_ERR)
    {
	BatGood = FALSE;
        log("Unable to set SIGQUIT handler");
    }
    /* Save our position */
    if ((jmpCode = setjmp(mainLoop)) != 0)
    {	/* If we get in here, we just came back via longjmp() */
	switch(jmpCode)
	{
	    case GOT_INT:
		BatGood = FALSE; /* This forces immediate shutdown */
		break;
	    default:
		fprintf(stderr, "Got an unknown return value %d from "
			"setjmp\n", jmpCode);
		BatGood = FALSE; /* This forces immediate shutdown */
		break;
	}
    }
    while (((IdleClients != NULL) || (BusyClients != NULL) || !(quitRequested || ACOut)) &&
        BatGood)
    {
	FD_ZERO(&readPipe);
	FD_SET(ImperiumPort, &readPipe);
	if ((nsel = select(ImperiumPort+1, &readPipe, 0, 0, 0)) < 0)
	{
	    if (errno != EINTR)
	    {
		perror("select");
	    }
	    nsel = 0;
	}
	if ((nsel > 0) && FD_ISSET(ImperiumPort, &readPipe))
	{
	    if ((nsel = read(ImperiumPort, &LocalReq, sizeof(Request_t))) !=
		sizeof(Request_t))
	    {
		printf("Only got %d bytes when reading Request_t\n", nsel);
		nsel = 0;
	    }
	}
	else
	{
	    nsel = 0;
	}
	if (nsel > 0)
        {
	    rq = &LocalReq;
            badBack = FALSE;
            (void) time(&rq->rq_time);
#ifdef DEBUG_SERV
            switch(rq->rq_type)
            {
                case rt_unlockShip:
                        if (DebugMode & DEBUG_SHIP)
                        {
                            if (rq->rq_u.ru_ship.sh_number !=
				rq->rq_whichUnit)
                            {
				char numBuff[120];
				sprintf(&numBuff[0],
					"*** sh_shipNumber %lu does not "
					"equal rq_whichUnit %lu !",
					rq->rq_u.ru_ship.sh_number,
					rq->rq_whichUnit);
                                log(&numBuff[0]);
                            }
                            if (rq->rq_u.ru_ship.sh_number >=
                                World.w_shipNext)
                            {
				char numBuff[80];
				sprintf(&numBuff[0],
					"*** sh_shipNumber %lu too big!",
					rq->rq_u.ru_ship.sh_number);
                                log(&numBuff[0]);
                            }
                        }
                    break;
                default:
                    break;
            }
#endif
            switch(rq->rq_type)
            {
                case rt_stopClient:
                case rt_setPlayer:
                case rt_readFile:
                case rt_moreFile:
                case rt_readHelp:
                case rt_readDoc:
                case rt_getMessage:
                case rt_setChat:
                case rt_sendChat:
                case rt_readPropaganda:
                case rt_checkMessages:
                case rt_readTelegram:
                case rt_writePower:
                case rt_readPower:
                case rt_lockWorld:
                case rt_lockPlayer:
                case rt_lockSector:
                case rt_lockShip:
                case rt_lockFleet:
                case rt_lockPlanet:
                case rt_lockBigItem:
                case rt_lockLoan:
                case rt_lockOffer:
                case rt_lockSectorPair:
                case rt_lockPlanetPair:
                case rt_lockShipPair:
                case rt_lockSectorShipPair:
                case rt_lockPlanetShipPair:
                case rt_lockPlanetItemPair:
                case rt_createShip:
                case rt_createFleet:
                case rt_createLoan:
                case rt_createOffer:
                case rt_createPlanet:
                case rt_createBigItem:
                    cl = IdleClients;
                    if (cl != NULL)
                    {
                        doCont = (cl->cl_identifier != rq->rq_clientId);
                        while (doCont)
                        {
                            cl = cl->cl_next;
                            if (cl != NULL)
                            {
                                doCont = (cl->cl_identifier !=
                                    rq->rq_clientId);
                            }
                            else
                            {
                                doCont = FALSE;
                            }
                        }
                    }
                    if (cl == NULL)
                    {
                        logN("no idle client ", rq->rq_clientId, " found");
                        rq->rq_type = rt_nop;
                    }
                    break;
                case rt_unlockWorld:
                case rt_unlockPlayer:
                case rt_unlockSector:
                case rt_unlockShip:
                case rt_unlockFleet:
                case rt_unlockPlanet:
                case rt_unlockBigItem:
                case rt_unlockLoan:
                case rt_unlockOffer:
                case rt_unlockSectorPair:
                case rt_unlockPlanetPair:
                case rt_unlockShipPair:
                case rt_unlockSectorShipPair:
                case rt_unlockPlanetShipPair:
                case rt_unlockPlanetItemPair:
                    cl = BusyClients;
                    if (cl != NULL)
                    {
                        doCont = (cl->cl_identifier != rq->rq_clientId);
                        while (doCont)
                        {
                            cl = cl->cl_next;
                            if (cl != NULL)
                            {
                                doCont = (cl->cl_identifier !=
                                    rq->rq_clientId);
                            }
                            else
                            {
                                doCont = FALSE;
                            }
                        }
                    }
                    if (cl == NULL)
                    {
                        logN("no busy client ", rq->rq_clientId, " found");
                        rq->rq_type = rt_nop;
                    }
                    break;
                default:
                    break;
            }
	    /* Attempt to verify the password for certain commands */
	    switch(rq->rq_type)
	    {
                case rt_shutDown:
                case rt_acOut:
                case rt_acBack:
                case rt_battLow:
                case rt_backStart:
                case rt_backDone:
		    if (!validPswd(rq, TRUE))
		    {
			rq = NULL;
		    }
		    break;
		default:
		    break;
	    }
            if (rq != NULL)
            {
            switch(rq->rq_type)
            {
                case rt_nop:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("nop");
                    }
#endif
                    break;
                case rt_log:
                    log(&rq->rq_u.ru_text[0]);
                    break;
                case rt_startClient:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("start client");
                    }
#endif
                    (void) startClient(rq);
                    break;
                case rt_stopClient:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("stop client");
                    }
#endif
                    stopClient(cl, rq);
		    rq = NULL; /* already gave response */
                    break;
                case rt_shutDown:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("shutDown");
                    }
#endif
                    logN("shut-down requested by client ", rq->rq_clientId,
                        ".");
                    SpecialFlags |= ISF_SERV_DOWN;
                    quitRequested = TRUE;
		    rq = NULL; /* no repsonse needed */
                    break;
                case rt_acOut:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("acOut");
                    }
#endif
                    logN("Client ", rq->rq_clientId, " reports AC power "
                        "lost.");
                    ACOut = TRUE;
                    SpecialFlags |= ISF_POW_WARN;
		    rq = NULL; /* no repsonse needed */
                    break;
                case rt_acBack:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("acBack");
                    }
#endif
                    logN("Client ", rq->rq_clientId, " reports AC power "
                        "has been restored.");
                    ACOut = FALSE;
                    SpecialFlags &= ~ISF_POW_WARN;
		    rq = NULL; /* no repsonse needed */
                    break;
                case rt_battLow:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("battLow");
                    }
#endif
                    logN("Client ", rq->rq_clientId, " reports UPS battery is "
                        "critically low - shutting down immediately!");
                    BatGood = FALSE;
                    writeWorld(&World, &Player[0]);
                    closeFiles();
                    openFiles(&Path[0], &HelpDir[0], &DocDir[0]);
                    SpecialFlags |= ISF_POW_CRIT;
		    rq = NULL; /* no repsonse needed */
                    break;
                case rt_backStart:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("backStart");
                    }
#endif
                    /* make sure someone else didn't already request this */
                    if (SpecialFlags & ISF_BACKUP)
                    {
                        logN("Client ", rq->rq_clientId, " requested system "
                            "backup when already in backup mode");
                        badBack = TRUE;
                    }
                    else
                    {
                        logN("Client ", rq->rq_clientId, " requests system "
                            "backup");
                        SpecialFlags |= ISF_BACKUP;
                        if (NumPlay != 0)
                        {
			    /* Store the port name of the client requesting BU */
                            strcpy(&BackReq[0], &rq->rq_text[0]);
                            rq = NULL;
                        }
                        else
                        {
                            log("Backup started");
                            writeWorld(&World, &Player[0]);
                            closeFiles();
                            openFiles(&Path[0], &HelpDir[0], &DocDir[0]);
                            BackupBegun = TRUE;
                        }
                    }
                    break;
                case rt_backDone:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("backDone");
                    }
#endif
                    logN("Client ", rq->rq_clientId, " reports backup "
                        "complete");
                    SpecialFlags &= ~ISF_BACKUP;
                    BackupBegun = FALSE;
		    rq = NULL; /* no repsonse needed */
                    break;
                case rt_poll:
                    if (quitRequested || ACOut)
                    {
                        rq->rq_whichUnit = 1;
                        logN("shut-down request sent to client ",
                             rq->rq_clientId, ".");
                    }
                    else
                    {
                        rq->rq_whichUnit = 0;
                    }
                    break;
                case rt_flush:
                    writeWorld(&World, &Player[0]);
                    closeFiles();
                    openFiles(&Path[0], &HelpDir[0], &DocDir[0]);
                    break;
                case rt_writeWorld:
                    writeWorld(&World, &Player[0]);
                    break;
                case rt_setPlayer:
                    /* see if we need to reduce NumPlay */
                    if (cl->cl_playerNumber != NO_OWNER)
                    {
                        if (rq->rq_whichUnit == NO_OWNER)
                        {
                            NumPlay--;
                            if ((BackReq[0] != '\0') && (NumPlay == 0))
                            {
                                log("Backup started");
                                writeWorld(&World, &Player[0]);
                                closeFiles();
                                openFiles(&Path[0], &HelpDir[0], &DocDir[0]);
                                BackupBegun = TRUE;
                                LocalReq.rq_specialFlags = SpecialFlags;
				backFile = open(&BackReq[0], O_WRONLY |
					O_NDELAY, 0);
				if (backFile < 0)
				{
					DeletePort();
					myAbort("Unable to open BackReq pipe - 1");
				}
				else
				{
				    if (write(backFile, (char *)&LocalReq,
					sizeof(Request_t)) < 0)
				    {
					close(backFile);
					DeletePort();
					myAbort("can't send message for BackReq - 1");
				    }
				}
				close(backFile);
                                BackReq[0] = '\0';
				rq = NULL;
                            }
                        }
                    }
                    else
                    {
                        if ((rq->rq_whichUnit == NO_OWNER + 1) ||
                            (rq->rq_whichUnit == NO_OWNER + 2))
                        {
                            if (rq->rq_whichUnit == NO_OWNER + 1)
                            {
                                NumPlay++;
                            }
                            else
                            {
                                NumPlay--;
                                if ((BackReq[0] != '\0') && (NumPlay == 0))
                                {
                                    log("Backup started");
                                    writeWorld(&World, &Player[0]);
                                    closeFiles();
                                    openFiles(&Path[0], &HelpDir[0], &DocDir[0]);
                                    BackupBegun = TRUE;
                                    LocalReq.rq_specialFlags = SpecialFlags;
				    backFile = open(&BackReq[0], O_WRONLY |
					O_NDELAY, 0);
				    if (backFile < 0)
				    {
					DeletePort();
					myAbort("Unable to open BackReq pipe - 2");
				    }
				    else
				    {
				        if (write(backFile, (char *)&LocalReq,
					    sizeof(Request_t)) < 0)
				        {
					    close(backFile);
					    DeletePort();
					    myAbort("can't send message for BackReq - 2");
				        }
				    }
				    close(backFile);
                                    BackReq[0] = '\0';
				    rq = NULL;
                                }
                            }
                            rq->rq_whichUnit = NO_OWNER;
                        }
                        else if (rq->rq_whichUnit != NO_OWNER)
                        {
                            NumPlay++;
                        }
                    }
                    cl->cl_playerNumber = rq->rq_whichUnit;
                    break;
                case rt_readFile:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("readFile");
                    }
#endif
                    readFile(cl, rq, ft_normal);
                    break;
                case rt_moreFile:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("moreFile");
                    }
#endif
                    moreFile(cl, rq);
                    break;
                case rt_readHelp:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("readHelp");
                    }
#endif
                    readFile(cl, rq, ft_help);
                    break;
                case rt_readDoc:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("readDoc");
                    }
#endif
                    readFile(cl, rq, ft_doc);
                    break;
                case rt_message:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("message");
                    }
#endif
                    message(rq);
                    break;
                case rt_getMessage:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("getMessage");
                    }
#endif
                    getMessage(cl, rq);
                    break;
                case rt_setChat:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        if (rq->rq_whichUnit == 0)
                        {
                            log("set chat off");
                        }
                        else
                        {
                            log("set chat on");
                        }
                    }
#endif
                    if (rq->rq_whichUnit != 0)
                    {
                        cl->cl_chatting = TRUE;
                        startChat(cl);
                    }
                    else
                    {
                        cl->cl_chatting = FALSE;
                    }
                    break;
                case rt_sendChat:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("sendChat");
                    }
#endif
                    sendChat(cl, rq);
                    break;
                case rt_news:
                    if (DoNews)
                    {
#ifdef DEBUG_SERV
                        if (DO_DEBUG)
                        {
                            log("news");
                        }
#endif
                        news(rq);
                    }
                    break;
                case rt_readNews:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("readNews");
                    }
#endif
                    readNews(rq);
                    break;
                case rt_propaganda:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("propaganda");
                    }
#endif
                    propaganda(rq);
                    break;
                case rt_readPropaganda:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("readPropaganda");
                    }
#endif
                    readPropaganda(cl, rq);
                    break;
                case rt_sendTelegram:
                    player = &Player[rq->rq_whichUnit];
                    if (rq->rq_u.ru_telegram.te_from == NOBODY)
                    {
                        if ((player->p_notify != nt_message) ||
                            !player->p_loggedOn)
                        {
#ifdef DEBUG_SERV
                            if (DO_DEBUG)
                            {
                                log("notify sendTelegram");
                            }
#endif
                            sendTelegram(rq);
                        }
                        if ((player->p_notify != nt_telegram) &&
                            player->p_loggedOn)
                        {
#ifdef DEBUG_SERV
                            if (DO_DEBUG)
                            {
                                log("notify message");
                            }
#endif
                            message(rq);
                        }
                    }
                    else
                    {
#ifdef DEBUG_SERV
                        if (DO_DEBUG)
                        {
                            log("sendTelegram");
                        }
#endif
                        sendTelegram(rq);
                    }
                    break;
                case rt_checkMessages:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("checkMessages");
                    }
#endif
                    checkMessages(cl, rq);
                    break;
                case rt_readTelegram:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("readTelegram");
                    }
#endif
                    readTelegram(cl, rq);
                    break;
                case rt_writePower:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("writePower");
                    }
#endif
                    writePower(cl, rq);
                    break;
                case rt_readPower:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("readPower");
                    }
#endif
                    readPower(cl, rq);
                    break;
                case rt_readWorld:
                case rt_readPlayer:
                case rt_readSector:
                case rt_readShip:
                case rt_readFleet:
                case rt_readPlanet:
                case rt_readBigItem:
                case rt_readLoan:
                case rt_readOffer:
                case rt_readSectorPair:
                case rt_readPlanetPair:
                case rt_readShipPair:
                case rt_readSectorShipPair:
                case rt_readPlanetShipPair:
                case rt_readPlanetItemPair:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        switch(rq->rq_type)
                        {
                            case rt_readWorld:
                                log("readWorld");
                                break;
                            case rt_readPlayer:
                                logN("readPlayer ", rq->rq_whichUnit, "");
                                break;
                            case rt_readSector:
                                logN("readSector ", rq->rq_whichUnit, "" );
                                break;
                            case rt_readShip:
                                logN("readShip ", rq->rq_whichUnit, "");
                                break;
                            case rt_readFleet:
                                logN("readFleet ", rq->rq_whichUnit, "");
                                break;
                            case rt_readPlanet:
                                logN("readPlanet ", rq->rq_whichUnit, "");
                                break;
                            case rt_readBigItem:
                                logN("readBigItem ", rq->rq_whichUnit, "");
                                break;
                            case rt_readLoan:
                                logN("readLoan ", rq->rq_whichUnit, "");
                                break;
                            case rt_readOffer:
                                logN("readOffer ", rq->rq_whichUnit, "");
                                break;
                            case rt_readSectorPair:
                                logN("readSectorPair ", rq->rq_whichUnit, "");
                                break;
                            case rt_readPlanetPair:
                                logN("readPlanetPair ", rq->rq_whichUnit, "");
                                break;
                            case rt_readShipPair:
                                logN("readShipPair ", rq->rq_whichUnit, "");
                                break;
                            case rt_readSectorShipPair:
                                logN("readSectorShipPair ", rq->rq_whichUnit,
                                    "");
                                break;
                            case rt_readPlanetShipPair:
                                logN("readPlanetShipPair ", rq->rq_whichUnit,
                                    "");
                                break;
                            case rt_readPlanetItemPair:
                                logN("readPlanetItemPair ", rq->rq_whichUnit,
                                    "");
                                break;
                        }
                    }
#endif
                    copyData(rq, getRKType(rq->rq_type));
                    break;
                case rt_lockWorld:
                case rt_lockPlayer:
                case rt_lockSector:
                case rt_lockShip:
                case rt_lockFleet:
                case rt_lockPlanet:
                case rt_lockBigItem:
                case rt_lockLoan:
                case rt_lockOffer:
                case rt_lockSectorPair:
                case rt_lockPlanetPair:
                case rt_lockShipPair:
                case rt_lockSectorShipPair:
                case rt_lockPlanetShipPair:
                case rt_lockPlanetItemPair:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        switch(rq->rq_type)
                        {
                            case rt_lockWorld:
                                log("lockWorld");
                                break;
                            case rt_lockPlayer:
                                logN("lockPlayer ", rq->rq_whichUnit, "");
                                break;
                            case rt_lockSector:
                                logN("lockSector ", rq->rq_whichUnit, "" );
                                break;
                            case rt_lockShip:
                                logN("lockShip ", rq->rq_whichUnit, "");
                                break;
                            case rt_lockFleet:
                                logN("lockFleet ", rq->rq_whichUnit, "");
                                break;
                            case rt_lockPlanet:
                                logN("lockPlanet ", rq->rq_whichUnit, "");
                                break;
                            case rt_lockBigItem:
                                logN("lockBigItem ", rq->rq_whichUnit, "");
                                break;
                            case rt_lockLoan:
                                logN("lockLoan ", rq->rq_whichUnit, "");
                                break;
                            case rt_lockOffer:
                                logN("lockOffer ", rq->rq_whichUnit, "");
                                break;
                            case rt_lockSectorPair:
                                logN("lockSectorPair ", rq->rq_whichUnit, "");
                                break;
                            case rt_lockPlanetPair:
                                logN("lockPlanetPair ", rq->rq_whichUnit, "");
                                break;
                            case rt_lockShipPair:
                                logN("lockShipPair ", rq->rq_whichUnit, "");
                                break;
                            case rt_lockSectorShipPair:
                                logN("lockSectorShipPair ", rq->rq_whichUnit,
                                    "");
                                break;
                            case rt_lockPlanetShipPair:
                                logN("lockPlanetShipPair ", rq->rq_whichUnit,
                                    "");
                                break;
                            case rt_lockPlanetItemPair:
                                logN("lockPlanetItemPair ", rq->rq_whichUnit,
                                    "");
                                break;
                        }
                    }
                    else
                    {
                        switch(rq->rq_type)
                        {
                            case rt_lockShip:
                                if (DebugMode & DEBUG_SHIP)
                                {
                                    logN("lockShip ", rq->rq_whichUnit, "");
                                }
                                break;
                            case rt_lockPlanet:
                                if (DebugMode & DEBUG_PLANET)
                                {
                                    logN("lockPlanet ", rq->rq_whichUnit, "");
                                }
                                break;
                            case rt_lockBigItem:
                                if (DebugMode & DEBUG_BIGITEM)
                                {
                                    logN("lockBigItem ", rq->rq_whichUnit,
                                        "");
                                }
                                break;
                            case rt_lockPlanetPair:
                                if (DebugMode & DEBUG_PLANET)
                                {
                                    logN("lockPlanetPair ", rq->rq_whichUnit,
                                        "");
                                }
                                break;
                            case rt_lockShipPair:
                                if (DebugMode & DEBUG_SHIP)
                                {
                                    logN("lockShipPair ", rq->rq_whichUnit,
                                        "");
                                }
                                break;
                            case rt_lockSectorShipPair:
                                if (DebugMode & DEBUG_SHIP)
                                {
                                    logN("lockSectorPair ", rq->rq_whichUnit,
                                        "");
                                }
                                break;
                            case rt_lockPlanetShipPair:
                                if (DebugMode & (DEBUG_SHIP | DEBUG_PLANET))
                                {
                                    logN("lockPlanetShipPair ",
                                        rq->rq_whichUnit, "");
                                }
                                break;
                            case rt_lockPlanetItemPair:
                                if (DebugMode & (DEBUG_PLANET | DEBUG_BIGITEM))
                                {
                                    logN("lockPlanetItemPair ",
                                        rq->rq_whichUnit, "");
                                }
                                break;
                            default:
                                break;
                        }
                    }
#endif
                    cl->cl_lockKind = getRKType(rq->rq_type);
                    cl->cl_resource = rq->rq_whichUnit;
                    cl->cl_resource2 = rq->rq_otherUnit;
                    /* remove client from list of idle clients */
                    if (cl->cl_next != NULL)
                    {
                        cl->cl_next->cl_prev = cl->cl_prev;
                    }
                    if (cl->cl_prev != NULL)
                    {
                        cl->cl_prev->cl_next = cl->cl_next;
                    }
                    else
                    {
                        IdleClients = cl->cl_next;
                    }
                    /* see if anyone has that resource locked */
                    handleLockRequest(cl);
                    rq = NULL;
                    break;
                case rt_unlockWorld:
                case rt_unlockPlayer:
                case rt_unlockSector:
                case rt_unlockShip:
                case rt_unlockFleet:
                case rt_unlockPlanet:
                case rt_unlockBigItem:
                case rt_unlockLoan:
                case rt_unlockOffer:
                case rt_unlockSectorPair:
                case rt_unlockPlanetPair:
                case rt_unlockShipPair:
                case rt_unlockSectorShipPair:
                case rt_unlockPlanetShipPair:
                case rt_unlockPlanetItemPair:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        switch(rq->rq_type)
                        {
                            case rt_unlockWorld:
                                log("unlockWorld");
                                break;
                            case rt_unlockPlayer:
                                logN("unlockPlayer ", rq->rq_whichUnit, "");
                                break;
                            case rt_unlockSector:
                                logN("unlockSector ", rq->rq_whichUnit, "" );
                                break;
                            case rt_unlockShip:
                                logN("unlockShip ", rq->rq_whichUnit, "");
                                break;
                            case rt_unlockFleet:
                                logN("unlockFleet ", rq->rq_whichUnit, "");
                                break;
                            case rt_unlockPlanet:
                                logN("unlockPlanet ", rq->rq_whichUnit, "");
                                break;
                            case rt_unlockBigItem:
                                logN("unlockBigItem ", rq->rq_whichUnit, "");
                                break;
                            case rt_unlockLoan:
                                logN("unlockLoan ", rq->rq_whichUnit, "");
                                break;
                            case rt_unlockOffer:
                                logN("unlockOffer ", rq->rq_whichUnit, "");
                                break;
                            case rt_unlockSectorPair:
                                logN("unlockSectorPair ", rq->rq_whichUnit, "");
                                break;
                            case rt_unlockPlanetPair:
                                logN("unlockPlanetPair ", rq->rq_whichUnit, "");
                                break;
                            case rt_unlockShipPair:
                                logN("unlockShipPair ", rq->rq_whichUnit, "");
                                break;
                            case rt_unlockSectorShipPair:
                                logN("unlockSectorShipPair ", rq->rq_whichUnit,
                                    "");
                                break;
                            case rt_unlockPlanetShipPair:
                                logN("unlockPlanetShipPair ", rq->rq_whichUnit,
                                    "");
                                break;
                            case rt_unlockPlanetItemPair:
                                logN("unlockPlanetItemPair ", rq->rq_whichUnit,
                                    "");
                                break;
                        }
                    }
                    else
                    {
                        switch(rq->rq_type)
                        {
                            case rt_unlockShip:
                                if (DebugMode & DEBUG_SHIP)
                                {
                                    logN("unlockShip ", rq->rq_whichUnit, "");
                                }
                                break;
                            case rt_unlockPlanet:
                                if (DebugMode & DEBUG_PLANET)
                                {
                                    logN("unlockPlanet ", rq->rq_whichUnit, "");
                                }
                                break;
                            case rt_unlockBigItem:
                                if (DebugMode & DEBUG_BIGITEM)
                                {
                                    logN("unlockBigItem ", rq->rq_whichUnit,
                                        "");
                                }
                                break;
                            case rt_unlockPlanetPair:
                                if (DebugMode & DEBUG_PLANET)
                                {
                                    logN("unlockPlanetPair ", rq->rq_whichUnit,
                                        "");
                                }
                                break;
                            case rt_unlockShipPair:
                                if (DebugMode & DEBUG_SHIP)
                                {
                                    logN("unlockShipPair ", rq->rq_whichUnit,
                                        "");
                                }
                                break;
                            case rt_unlockSectorShipPair:
                                if (DebugMode & DEBUG_SHIP)
                                {
                                    logN("unlockSectorPair ", rq->rq_whichUnit,
                                        "");
                                }
                                break;
                            case rt_unlockPlanetShipPair:
                                if (DebugMode & (DEBUG_SHIP | DEBUG_PLANET))
                                {
                                    logN("unlockPlanetShipPair ",
                                        rq->rq_whichUnit, "");
                                }
                                break;
                            case rt_unlockPlanetItemPair:
                                if (DebugMode & (DEBUG_PLANET | DEBUG_BIGITEM))
                                {
                                    logN("unlockPlanetItemPair ",
                                        rq->rq_whichUnit, "");
                                }
                                break;
                            default:
                                break;
                        }
                    }
#endif
                    /* copy out the updated data */
                    switch(rq->rq_type)
                    {
                        case rt_unlockWorld:
                            World = rq->rq_u.ru_world;
                            cl2 = Clients;
                            while (cl2 != NULL)
                            {
                                if (cl2 != cl)
                                {
                                    cl2->cl_newWorld = TRUE;
                                }
                                cl2 = cl2->cl_masterNext;
                            }
                            break;
                        case rt_unlockPlayer:
                            Player[rq->rq_whichUnit] = rq->rq_u.ru_player;
                            cl2 = Clients;
                            while (cl2 != NULL)
                            {
                                if (cl2->cl_playerNumber == rq->rq_whichUnit)
                                {
                                    cl2->cl_newPlayer = TRUE;
                                }
                                cl2 = cl2->cl_masterNext;
                            }
                            break;
                        case rt_unlockSector:
                            writeSector(rq->rq_whichUnit, &rq->rq_u.ru_sector);
                            break;
                        case rt_unlockShip:
#ifdef DEBUG_SERV
                            if (DebugMode & DEBUG_SHIP)
                            {
                                if (rq->rq_u.ru_ship.sh_number !=
				    rq->rq_whichUnit)
                                {
				    char numBuff[120];
				    sprintf(&numBuff[0],
					    "*** sh_shipNumber %lu does not "
					    "equal rq_whichUnit %lu !",
					    rq->rq_u.ru_ship.sh_number,
					    rq->rq_whichUnit);
                                    log(&numBuff[0]);
                                }
                                if (rq->rq_u.ru_ship.sh_number >=
                                    World.w_shipNext)
                                {
				    char numBuff[80];
				    sprintf(&numBuff[0],
					    "*** sh_shipNumber %lu too big!",
					    rq->rq_u.ru_ship.sh_number);
                                    log(&numBuff[0]);
                                }
                            }
#endif
                            writeShip(&rq->rq_u.ru_ship);
                            break;
                        case rt_unlockFleet:
                            writeFleet(rq->rq_whichUnit, &rq->rq_u.ru_fleet);
                            break;
                        case rt_unlockPlanet:
#ifdef DEBUG_SERV
                            if (DebugMode & DEBUG_PLANET)
                            {
                                if (rq->rq_u.ru_planet.pl_number >=
                                    World.w_planetNext)
                                {
				    char numBuff[80];
				    sprintf(&numBuff[0],
					    "*** pl_planetNumber %lu too big!",
					    rq->rq_u.ru_planet.pl_number);
                                    log(&numBuff[0]);
                                }
                            }
#endif
                            writePlanet(&rq->rq_u.ru_planet);
                            break;
                        case rt_unlockBigItem:
#ifdef DEBUG_SERV
                            if (DebugMode & DEBUG_BIGITEM)
                            {
                                if (rq->rq_u.ru_bigItem.bi_number >=
                                    World.w_bigItemNext)
                                {
				    char numBuff[80];
				    sprintf(&numBuff[0],
					    "*** bi_bigItemNumber %lu too big!",
					    rq->rq_u.ru_bigItem.bi_number);
                                    log(&numBuff[0]);
                                }
                            }
#endif
                            writeBigItem(&rq->rq_u.ru_bigItem);
                            break;
                        case rt_unlockLoan:
                            writeLoan(rq->rq_whichUnit, &rq->rq_u.ru_loan);
                            break;
                        case rt_unlockOffer:
                            writeOffer(rq->rq_whichUnit, &rq->rq_u.ru_offer);
                            break;
                        case rt_unlockSectorPair:
                            writeSector(rq->rq_whichUnit,
                                &rq->rq_u.ru_sectorPair[0]);
                            writeSector(rq->rq_otherUnit,
                                &rq->rq_u.ru_sectorPair[1]);
                            break;
                        case rt_unlockPlanetPair:
#ifdef DEBUG_SERV
                            if (DebugMode & DEBUG_PLANET)
                            {
                                if (rq->rq_u.ru_planetPair[0].pl_number >=
                                    World.w_planetNext)
                                {
                                    log("*** [0]pl_planetNumber too big!");
                                }
                            }
#endif
                            writePlanet(&rq->rq_u.ru_planetPair[0]);
#ifdef DEBUG_SERV
                            if (DebugMode & DEBUG_PLANET)
                            {
                                if (rq->rq_u.ru_planetPair[1].pl_number >=
                                    World.w_planetNext)
                                {
                                    log("*** [1]pl_planetNumber too big!");
                                }
                            }
#endif
                            writePlanet(&rq->rq_u.ru_planetPair[1]);
                            break;
                        case rt_unlockShipPair:
#ifdef DEBUG_SERV
                            if (DebugMode & DEBUG_SHIP)
                            {
                                if (rq->rq_u.ru_shipPair[0].sh_number >=
                                    World.w_shipNext)
                                {
                                    log("*** [0]sh_shipNumber too big!");
                                }
                            }
#endif
                            writeShip(&rq->rq_u.ru_shipPair[0]);
#ifdef DEBUG_SERV
                            if (DebugMode & DEBUG_SHIP)
                            {
                                if (rq->rq_u.ru_shipPair[1].sh_number >=
                                    World.w_shipNext)
                                {
                                    log("*** [1]sh_shipNumber too big!");
                                }
                            }
#endif
                            writeShip(&rq->rq_u.ru_shipPair[1]);
                            break;
                        case rt_unlockSectorShipPair:
                            writeSector(rq->rq_whichUnit,
                                        &rq->rq_u.ru_sectorShipPair.p_s);
#ifdef DEBUG_SERV
                            if (DebugMode & DEBUG_SHIP)
                            {
                                if (rq->rq_u.ru_sectorShipPair.p_sh.sh_number >=
                                    World.w_shipNext)
                                {
                                    log("*** sh_shipNumber too big!");
                                }
                            }
#endif
                            writeShip(&rq->rq_u.ru_sectorShipPair.p_sh);
                            break;
                        case rt_unlockPlanetShipPair:
#ifdef DEBUG_SERV
                            if (DebugMode & DEBUG_PLANET)
                            {
                                if (rq->rq_u.ru_planetShipPair.p_p.pl_number >=
                                    World.w_planetNext)
                                {
                                    log("*** pl_planetNumber too big!");
                                }
                            }
#endif
                            writePlanet(&rq->rq_u.ru_planetShipPair.p_p);
#ifdef DEBUG_SERV
                            if (DebugMode & DEBUG_SHIP)
                            {
                                if (rq->rq_u.ru_planetShipPair.p_sh.sh_number >=
                                    World.w_shipNext)
                                {
                                    log("*** sh_shipNumber too big!");
                                }
                            }
#endif
                            writeShip(&rq->rq_u.ru_planetShipPair.p_sh);
                            break;
                        case rt_unlockPlanetItemPair:
#ifdef DEBUG_SERV
                            if (DebugMode & DEBUG_PLANET)
                            {
                                if (rq->rq_u.ru_planetItemPair.p_p.pl_number >=
                                    World.w_planetNext)
                                {
                                    log("*** pl_planetNumber too big!");
                                }
                            }
#endif
                            writePlanet(&rq->rq_u.ru_planetItemPair.p_p);
#ifdef DEBUG_SERV
                            if (DebugMode & DEBUG_BIGITEM)
                            {
                                if (rq->rq_u.ru_planetItemPair.p_bi.bi_number >=
                                    World.w_bigItemNext)
                                {
                                    log("*** bi_bigItemNumber too big!");
                                }
                            }
#endif
                            writeBigItem(&rq->rq_u.ru_planetItemPair.p_bi);
                            break;
                    }
                    /* remove this client from the busy list */
                    if (cl->cl_next != NULL)
                    {
                        cl->cl_next->cl_prev = cl->cl_prev;
                    }
                    if (cl->cl_prev != NULL)
                    {
                        cl->cl_prev->cl_next = cl->cl_next;
                    }
                    else
                    {
                        BusyClients = cl->cl_next;
                    }
                    /* put it onto the list of idle clients */
                    cl->cl_next = IdleClients;
                    cl->cl_prev = NULL;
                    if (IdleClients != NULL)
                    {
                        IdleClients->cl_prev = cl;
                    }
                    IdleClients = cl;
                    /* reply to this request */
                    rq->rq_specialFlags = SpecialFlags;
                    if (write(cl->cl_handle, (char *)rq,
			sizeof(Request_t)) < 0)
		    {
			DeletePort();
			myAbort("can't send message for standard reply");
		    }
                    rq = NULL;
                    /* go through the wait queue of the now finished lock. A
                       request that is still waiting is put on another wait
                       queue. Any that can run are released. */
                    cl = cl->cl_waitQueue;
                    while (cl != NULL)
                    {
                        cl2 = cl;
                        cl = cl->cl_next;
                        handleLockRequest(cl2);
                    }
                    break;
                case rt_createShip:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        logN("createShip ", rq->rq_whichUnit, "");
                    }
#endif
                    writeShip(&rq->rq_u.ru_ship);
                    break;
                case rt_createFleet:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        logN("createFleet ", rq->rq_whichUnit, "");
                    }
#endif
                    writeFleet(rq->rq_whichUnit, &rq->rq_u.ru_fleet);
                    break;
                case rt_createPlanet:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        logN("createPlanet ", rq->rq_whichUnit, "");
                    }
#endif
                    writePlanet(&rq->rq_u.ru_planet);
                    break;
                case rt_createBigItem:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        logN("createBigItem ", rq->rq_whichUnit, "");
                    }
#endif
                    writeBigItem(&rq->rq_u.ru_bigItem);
                    break;
                case rt_createLoan:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        logN("createLoan ", rq->rq_whichUnit, "");
                    }
#endif
                    writeLoan(rq->rq_whichUnit, &rq->rq_u.ru_loan);
                    break;
                case rt_createOffer:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        logN("createOffer ", rq->rq_whichUnit, "");
                    }
#endif
                    writeOffer(rq->rq_whichUnit, &rq->rq_u.ru_offer);
                    break;
                case rt_broadcastMsg:
#ifdef DEBUG_SERV
                    if (DO_DEBUG)
                    {
                        log("broadcastMsg");
                    }
#endif
                    break;
                default:
                    logN("bad request type ", rq->rq_type - rt_nop,
                        " received");
                    break;
            }
	    }
            /* only reply if we have not already done so, or made it wait */
            if (rq != NULL)
            {
                rq->rq_specialFlags = SpecialFlags;
		if (rq->rq_type == rt_backStart)
		{
            	    if (badBack)
            	    {
   			rq->rq_specialFlags &= ~ISF_BACKUP;
            	    }
		    backFile = open(&rq->rq_text[0], O_WRONLY | O_NDELAY, 0);
		    if (backFile < 0)
		    {
			DeletePort();
			myAbort("Unable to open BackReq pipe - 5");
		    }
		    else
		    {
			if (write(backFile, (char *)rq, sizeof(Request_t)) < 0)
			{
			    close(backFile);
			    DeletePort();
			    myAbort("can't send message for BackReq - 5");
			}
		    }
		    close(backFile);
		}
		else
		{
		    /* First check the idle clients */
                    cl = IdleClients;
                    if (cl != NULL)
                    {
                        doCont = (cl->cl_identifier != rq->rq_clientId);
                        while (doCont)
                        {
                            cl = cl->cl_next;
                            if (cl != NULL)
                            {
                                doCont = (cl->cl_identifier !=
                                    rq->rq_clientId);
                            }
                            else
                            {
                                doCont = FALSE;
                            }
                        }
                    }
		    if (cl == NULL)
		    {
         		cl = BusyClients;
                        doCont = (cl->cl_identifier != rq->rq_clientId);
                        while (doCont)
                        {
                            cl = cl->cl_next;
                            if (cl != NULL)
                            {
                                doCont = (cl->cl_identifier !=
                                    rq->rq_clientId);
                            }
                            else
                            {
                                doCont = FALSE;
                            }
                        }
		    }
                    if (cl == NULL)
                    {
                        logN("no client ", rq->rq_clientId, " found for reply");
                    }
           	    else if (write(cl->cl_handle, (char *)rq,
			sizeof(Request_t)) < 0)
		    {
			DeletePort();
			myAbort("can't send message for final reply");
		    }
		}
            }
        }
    }
#ifdef DEBUG_SERV
    if (DO_DEBUG)
    {
        log("shutting down");
    }
#endif
    logN("shutting down, ", ClientCounter - 1, " clients serviced.");
}

/*
 * openPort - attempts to open the FIFO
 */

int openPort(char *buff)
{
    int retCode = -1;
    struct stat locStat;

    if (stat(buff, &locStat))
    {
	(void) umask(oldUmask);
	if (mknod(buff, S_IFIFO|0622, 0) == 0)
	{
#ifdef BROKEN_MKNOD
	    chmod(buff, 0622);
#endif
#ifdef BROKEN_PIPE
	    retCode = open(buff, O_RDWR | O_NDELAY, 0);
#else
	    retCode = open(buff, O_RDONLY | O_NDELAY, 0);
#endif
	    if (retCode < 0)
	    {
		(void)unlink(buff);
	    }
	    else
	    {
		strcpy(ImperiumPortName, buff);
	    }
	}
	(void) umask(0077);
    }
    return(retCode);
}

/*
 * startServing - start the Imperium-specific stuff.
 */

void startServing(void)
{
    char lineBuf[512];

    openFiles(&Path[0], &HelpDir[0], &DocDir[0]);
    if (UseTest)
    {
	sprintf(lineBuf, "%s/server/%s", FIFO_DIR, IMP_TEST_PORT);
    }
    else if (PlayPort[0] != '\0')
    {
	sprintf(lineBuf, "%s/server/%s", FIFO_DIR, &PlayPort[0]);
    }
    else
    {
	sprintf(lineBuf, "%s/server/%s", FIFO_DIR, IMPERIUM_PORT);
    }
    ImperiumPort = openPort(lineBuf);
    if (ImperiumPort < 0)
    {
            if (UseTest)
            {
                myAbort("can't create test port, or it already exists");
            }
            else
            {
                if (PlayPort[0] != '\0')
                {
                    sprintf(&lineBuf[0], "can't create the port \"%s\","
			" or it already exists",
                        &PlayPort[0]);
                    myAbort(&lineBuf[0]);
                }
                else
                {
                    myAbort("can't create Imperium port, or it already exists");
                }
            }
    }
    if (!readWorld(&World, &Player[0]))
    {
            DeletePort();
            myAbort("can't read world/player");
    }
    IdleClients = NULL;
    BusyClients = NULL;
    if (UseTest)
    {
            log("Test server 1.0w started up.");
    }
    else
    {
            if (PlayPort[0] != '\0')
            {
                sprintf(&lineBuf[0], "Imperium server 1.0w starting up on port"
                    " \"%s\"", &PlayPort[0]);
                log(&lineBuf[0]);
            }
            else
            {
                log("Imperium server 1.0w started up.");
            }
    }
    if (ClearPlayers)
    {
            log("Clearing the p_loggedOn flags");
            clearLogFlag();
    }
    handleRequests();
    DeletePort();
    writeWorld(&World, &Player[0]);
    if (UseTest)
    {
            log("Test server 1.0w shut down.");
    }
    else
    {
            if (PlayPort[0] != '\0')
            {
                sprintf(&lineBuf[0], "Imperium server 1.0w shutting down on "
                    "port \"%s\"", &PlayPort[0]);
                log(&lineBuf[0]);
            }
            else
            {
                log("Imperium server 1.0w shut down.");
            }
    }
    closeFiles();
}

/*
 * useage - print a simple CLI useage message to the console
 */

void useage(void)
{
    printf("ImpServ V%s.pl%d\n\n", IMP_BASE_REV,
	0);
    puts("Useage is: ImpServ [-t] [-n] [-p<port name>]");
    puts("                             [sectorcache=<num>] [shipcache=<num>]");
    puts("                             [planetcache=<num>] [item=<num>]");
    puts("                             [clearplay=<on|off>]");
#ifdef DEBUG_SERV
    puts("                             [trace=<on|off>]");
    puts("                             [watch=1|2|4]");
#endif
    puts("    -t = use test port (overides \"-p\")");
    puts("    -n = don't generate news files");
}

/*
 * getBool - interpret a YES/NO or ON/OFF from a string.
 */

BOOL getBool(char *st)
{
    if (strcmp(st, "YES") == 0)
    {
        return TRUE;
    }
    if (strcmp(st, "ON") == 0)
    {
        return TRUE;
    }
    if (strcmp(st, "TRUE") == 0)
    {
        return TRUE;
    }
    if (strcmp(st, "SI") == 0)
    {
        return TRUE;
    }
    if (strcmp(st, "DA") == 0)
    {
        return TRUE;
    }
    return FALSE;
}

/*
 * getNumber - parse a number from the passed string. Little error checking.
 */

ULONG getNumber(register char *str)
{
    register ULONG n;
    register length;

    n = 0;
    length = 0;
    while ((*str >= '0') && (*str <= '9') && (length < 10))
    {
        n = n * 10 + (*str - '0');
        str += sizeof(char);
        length++;
    }
    return n;
}

/*
 * main - entry point for Imperium server.
 */

int main(int argc, char *argv[])
{
    USHORT curArg;
    BOOL hadError;
    char *par;

    UseTest = FALSE;
    DoNews = TRUE;
    ClearPlayers = FALSE;
    BackupBegun = FALSE;
    BackReq[0] = '\0';
    SpecialFlags = ISF_NONE;
    SectorCacheSize = DEF_SECTOR_CACHE_SIZE;
    ShipCacheSize = DEF_SHIP_CACHE_SIZE;
    PlanetCacheSize = DEF_PLANET_CACHE_SIZE;
    BigItemCacheSize = DEF_BIGITEM_CACHE_SIZE;
    PlayPort[0] = '\0';
#ifdef DEBUG_SERV
    /* default it to NO extra logging */
    DO_DEBUG = FALSE;
    DebugMode = 0;
#endif

    Path[0]='\0';
    oldUmask = umask(0077);
    {
        /* running from CLI */
        hadError = FALSE;
        if (argc > 1)
        {
            curArg = 1;
            while (curArg < argc)
            {
                par = argv[curArg];
                if (*par == '-')
                {
                    par += sizeof(char);
                }
                if (memcmp("sectorcache=", par, 12 * sizeof(char)) == 0)
                {
                    SectorCacheSize = getNumber(par + (13 * sizeof(char)));
                    if (SectorCacheSize == 0)
                    {
                        SectorCacheSize = DEF_SECTOR_CACHE_SIZE;
                    }
                }
                else if (memcmp("shipcache=", par, 10 * sizeof(char)) == 0)
                {
                    ShipCacheSize = getNumber(par + (10 * sizeof(char)));
                    if (ShipCacheSize == 0)
                    {
                        ShipCacheSize = DEF_SHIP_CACHE_SIZE;
                    }
                }
                else if (memcmp("planetcache=", par, 12 * sizeof(char)) == 0)
                {
                    PlanetCacheSize = getNumber(par + (12 * sizeof(char)));
                    if (PlanetCacheSize == 0)
                    {
                        PlanetCacheSize = DEF_PLANET_CACHE_SIZE;
                    }
                }
                else if (memcmp("itemcache=", par, 10 * sizeof(char)) == 0)
                {
                    BigItemCacheSize = getNumber(par + (10 * sizeof(char)));
                    if (BigItemCacheSize == 0)
                    {
                        BigItemCacheSize = DEF_BIGITEM_CACHE_SIZE;
                    }
                }
#ifdef DEBUG_SERV
                else if (memcmp("trace=", par, 6 * sizeof(char)) == 0)
                {
                    DO_DEBUG = getBool(par + (6 * sizeof(char)));
		    printf("DO_DEBUG is %d\n", DO_DEBUG);
                }
                else if (memcmp("watch=", par, 6 * sizeof(char)) == 0)
                {
                    DebugMode = (UBYTE)getNumber(par + (6 * sizeof(char)));
                }
#endif
                else if (memcmp("clearplay=", par, 10 * sizeof(char)) == 0)
                {
                    ClearPlayers = getBool(par + (10 * sizeof(char)));
                }
                else if (memcmp("port=", par, 5 * sizeof(char)) == 0)
                {
                    strcpy(PlayPort, (char *)(par + (5 * sizeof(char))));
                }
                else
                {
                        while (*par != '\0')
                        {
                            switch(*par)
                            {
                                case 't':
                                    UseTest = TRUE;
                                    break;
                                case 'n':
                                    DoNews = FALSE;
                                    break;
                                default:
                                    hadError = TRUE;
                                    break;
                            }
                            par += sizeof(char);
                        }
                }
                curArg++;
            }
        }
        if (hadError)
        {
            useage();
        }
        else
        {
            startServing();
        }
    }
    (void) umask(oldUmask);
    exit(0);
}
