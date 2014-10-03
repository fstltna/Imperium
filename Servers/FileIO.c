/*
 * Imperium
 *
 * $Id: FileIO.c,v 1.2 2000/05/18 06:32:08 marisa Exp $
 *
 * This file handles file I/O for files which have a cache associated with
 * them.
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
 * $Log: FileIO.c,v $
 * Revision 1.2  2000/05/18 06:32:08  marisa
 * Crypt, etc.
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:31:04  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:13:54  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:06:02  marisag
 * patch20: Fix empty rev.
 *
 */

#include "../config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
BOGUS - Imperium not supported on this machine due to missing stdlib.h
#endif
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
BOGUS - Imperium not supported on this machine due to missing unistd.h
#endif
#include <sys/stat.h>
#include "../Include/Imperium.h"
#include "Server.h"

static const char rcsid[] = "$Id: FileIO.c,v 1.2 2000/05/18 06:32:08 marisa Exp $";

#define OS_BLOCK_SIZE       512

extern ULONG SectorCacheSize;
extern ULONG ShipCacheSize;
extern ULONG PlanetCacheSize;
extern ULONG BigItemCacheSize;

#ifdef DEBUG_SERV
extern UBYTE DebugMode;
#endif

extern int oldUmask;

struct sectorCacheStruct
    {
        struct sectorCacheStruct
            *sc_next,                           /* next, prev in LRU chain */
            *sc_prev;
        USHORT
            sc_rowCol;                          /* row & column, encoded */
        BOOL
            sc_dirty;                           /* needs writing to disk */
        Sector_t
            sc_sector;                          /* the sector data      */
    };
typedef struct sectorCacheStruct SectorCache_t;

struct shipCacheStruct
    {
        struct shipCacheStruct
            *shc_next,
            *shc_prev;
        ULONG
            shc_shipNumber;
        BOOL
            shc_dirty;
        Ship_t
            shc_ship;
    };
typedef struct shipCacheStruct ShipCache_t;

struct planetCacheStruct
    {
        struct planetCacheStruct
            *plc_next,
            *plc_prev;
        ULONG
            plc_planetNumber;
        BOOL
            plc_dirty;
        Planet_t
            plc_planet;
    };
typedef struct planetCacheStruct PlanetCache_t;

struct bigItemCacheStruct
    {
        struct bigItemCacheStruct
            *bic_next,
            *bic_prev;
        ULONG
            bic_bigItemNumber;
        BOOL
            bic_dirty;
        BigItem_t
            bic_bigItem;
    };
typedef struct bigItemCacheStruct BigItemCache_t;

SectorCache_t
    *SectorCache;                       /* the sector cache */
USHORT
    SectorFree;                         /* next unused slot */
SectorCache_t
    *SectorHead,                        /* head of LRU chain */
    *SectorTail;                        /* tail of LRU chain */

ShipCache_t
    *ShipCache;
ULONG
    ShipFree;
ShipCache_t
    *ShipHead,
    *ShipTail;

PlanetCache_t
    *PlanetCache;
ULONG
    PlanetFree;
PlanetCache_t
    *PlanetHead,
    *PlanetTail;

BigItemCache_t
    *BigItemCache;
ULONG
    BigItemFree;
BigItemCache_t
    *BigItemHead,
    *BigItemTail;

FILE
    *FileFd,
    *LogFd,
    *WorldFd,
    *PlayerFd,
    *SectorFd,
    *ShipFd,
    *FleetFd,
    *LoanFd,
    *PlanetFd,
    *BigItemFd,
    *OfferFd;

char
    FileName[150],                            /* buffer when opening */
    LogFileName[150],
    *Path;

/*
 * closeImperiumFiles - do the actual file closing.
 */

void closeImperiumFiles(void)
{
    if (OfferFd != NULL)
    {
        fclose(OfferFd);
        OfferFd = NULL;
    }
    if (LoanFd != NULL)
    {
        fclose(LoanFd);
        LoanFd = NULL;
    }
    if (FleetFd != NULL)
    {
        fclose(FleetFd);
        FleetFd = NULL;
    }
    if (ShipFd != NULL)
    {
        fclose(ShipFd);
        ShipFd = NULL;
    }
    if (SectorFd != NULL)
    {
        fclose(SectorFd);
        SectorFd = NULL;
    }
    if (PlayerFd != NULL)
    {
        fclose(PlayerFd);
        PlayerFd = NULL;
    }
    if (PlanetFd != NULL)
    {
        fclose(PlanetFd);
        PlanetFd = NULL;
    }
    if (BigItemFd != NULL)
    {
        fclose(BigItemFd);
        BigItemFd = NULL;
    }
    if (WorldFd != NULL)
    {
        fclose(WorldFd);
        WorldFd = NULL;
    }
    if (SectorCache != NULL)
    {
        free(SectorCache);
        SectorCache = NULL;
    }
    if (ShipCache != NULL)
    {
        free(ShipCache);
        ShipCache = NULL;
    }
    if (PlanetCache != NULL)
    {
        free(PlanetCache);
        PlanetCache = NULL;
    }
    if (BigItemCache != NULL)
    {
        free(BigItemCache);
        BigItemCache = NULL;
    }
}

/*
 * logString - write a string to the log file.
 */

void logString(char *message)
{
    register char *p;

    p = message;
    while (*p != '\0')
    {
        p++;
    }
    (void) fwrite(message, sizeof(char), p - message, LogFd);
}

/*
 * logNumber - write a number to the log file.
 */

void logNumber(ULONG num)
{
    char buffer[25];
    register int len;

    len = sprintf(&buffer[0], "%-u", num);
    (void) fwrite(&buffer[0], sizeof(char), len, LogFd);
}

/*
 * logOpen - write the time and a separator to the log file.
 */

BOOL logOpen(void)
{
    char timeBuffer[30];
    ULONG oTime;

    LogFd = fopen(&LogFileName[0], "a");
    if (LogFd != NULL)
    {
        (void)time(&oTime);
        strcpy(&timeBuffer[0], ctime(&oTime));
        /* eliminate the '\n' that ctime() adds */
        timeBuffer[strlen(timeBuffer) - 1] = '\0';
        logString(&timeBuffer[0]);
        logString(" - ");
        return TRUE;
    }
    return FALSE;
}

/*
 * log - write a message, with current time, to the log file.
 */

void log(char *message)
{
    if (logOpen())
    {
        logString(message);
        logString("\n");
        fclose(LogFd);
	LogFd = NULL;
    }
}

/*
 * logN - write a message, built from 2 strings and an integer.
 */

void logN(register char *m1, register ULONG n, char *m2)
{
    char buffer[11];

    if (logOpen())
    {
        logString(m1);
        m1 = &buffer[10];
        *m1 = '\0';
        do
        {
            m1--;
            *m1 = n % 10 + '0';
            n /= 10;
        } while (n != 0);
        logString(m1);
        logString(m2);
        logString("\n");
        fclose(LogFd);
	LogFd = NULL;
    }
}

/*
 * myAbort - abort with a message.
 */

void myAbort(char *message)
{
    if (logOpen())
    {
        logString("*** ");
        logString(message);
        logString(" - aborting\n");
        fclose(LogFd);
	LogFd = NULL;
    }
    closeImperiumFiles();
    (void) umask(oldUmask);
    exit(5);
}

/*
 * abortN - abort with a message and a number.
 */

void abortN(char *message, ULONG num)
{
    if (logOpen())
    {
        logString("*** ");
        logString(message);
        logNumber(num);
        logString(" - aborting\n");
        fclose(LogFd);
	LogFd = NULL;
    }
    closeImperiumFiles();
    (void) umask(oldUmask);
    exit(5);
}

/*
 * sectorFlush - flush the sector cache. We flush the sectors in increasing
 *      absolute sector order, so as to minimize disk seeking.
 */

void sectorFlush(void)
{
    register USHORT i, minRowCol;
    BOOL foundDirty;
    register SectorCache_t *sc, *lowest = NULL;

    if (SectorFree != 0)
    {
        foundDirty = FALSE;
        minRowCol = 0xffff;
        sc = &SectorCache[0];
        for (i = 0; i < SectorFree; i++)
        {
            if (sc->sc_dirty && (sc->sc_rowCol < minRowCol))
            {
                foundDirty = TRUE;
                lowest = sc;
                minRowCol = sc->sc_rowCol;
            }
            sc++;
        }
        while (foundDirty)
        {
            if (fseek(SectorFd, (ULONG)(minRowCol) * sizeof(Sector_t),
                        SEEK_SET))
            {
                abortN("Can't seek to sector ", minRowCol);
            }
            if (fwrite(&lowest->sc_sector, sizeof(char), sizeof(Sector_t),
		SectorFd) != sizeof(Sector_t))
            {
                abortN("Can't write sector ", minRowCol);
            }
            lowest->sc_dirty = FALSE;
            foundDirty = FALSE;
            minRowCol = 0xffff;
            sc = &SectorCache[0];
            for (i = 0; i < SectorFree; i++)
            {
                if (sc->sc_dirty && (sc->sc_rowCol < minRowCol))
                {
                    foundDirty = TRUE;
                    lowest = sc;
                    minRowCol = sc->sc_rowCol;
                }
                sc++;
            }
        }
    }
}

/*
 * sectorLookup - lookup/enter the requested sector in the sector cache.
 *      Return 'FALSE' if it was already there. Referencing it will always
 *      put it at the head of the chain.
 */

BOOL sectorLookup(register USHORT rowCol)
{
    register USHORT dirtyCount;
    register SectorCache_t *sc;
    BOOL doCont;

    sc = SectorHead;
    if (sc != NULL)
    {
        /* this is needed to prevent derefrencing a NULL pointer */
        doCont = (sc->sc_rowCol != rowCol);
        while (doCont)
        {
            sc = sc->sc_next;
            /* this is needed to prevent derefrencing a NULL pointer */
            if (sc != NULL)
            {
                doCont = (sc->sc_rowCol != rowCol);
            }
            else
            {
                doCont = FALSE;
            }
        }
    }
    if (sc == NULL)
    {
        /* didn't find the needed sector - add it to the cache. */
        if (SectorFree != SectorCacheSize)
        {
            /* free slot left - just use it */
            sc = &SectorCache[SectorFree];
            SectorFree++;
        }
        else
        {
            /* no free slot - look for a non-dirty one */
            dirtyCount = 0;
            sc = SectorTail;
            if (sc != NULL)
            {
                /* this is needed to prevent derefrencing a NULL pointer */
                doCont = sc->sc_dirty;
                while (doCont)
                {
                    sc = sc->sc_prev;
                    dirtyCount++;
                    /* this is needed to prevent derefrencing a NULL pointer */
                    if (sc != NULL)
                    {
                        doCont = sc->sc_dirty;
                    }
                    else
                    {
                        doCont = FALSE;
                    }
                }
            }
            if ((dirtyCount > SectorCacheSize * 4 / 5) || (sc == NULL))
            {
                /* no non-dirty slot left. Flush all and use tail one. */
                sectorFlush();
                sc = SectorTail;
            }
            /* delete the sector from it's current position in the chain */
            if (sc->sc_prev == NULL)
            {
                SectorHead = sc->sc_next;
            }
            else
            {
                sc->sc_prev->sc_next = sc->sc_next;
            }
            if (sc->sc_next == NULL)
            {
                SectorTail = sc->sc_prev;
            }
            else
            {
                sc->sc_next->sc_prev = sc->sc_prev;
            }
        }
        /* insert it at the head of the chain */
        sc->sc_prev = NULL;
        sc->sc_next = SectorHead;
        if (SectorHead != NULL)
        {
            SectorHead->sc_prev = sc;
        }
        else
        {
            SectorTail = sc;
        }
        SectorHead = sc;
        /* set it to be the requested sector */
        sc->sc_rowCol = rowCol;
        sc->sc_dirty = FALSE;
        return TRUE;
    }
    /* move it to the front of the LRU chain if its not there already */
    if (sc->sc_prev != NULL)
    {
        sc->sc_prev->sc_next = sc->sc_next;
        if (sc->sc_next != NULL)
        {
            sc->sc_next->sc_prev = sc->sc_prev;
        }
        else
        {
            SectorTail = sc->sc_prev;
        }
        sc->sc_prev = NULL;
        sc->sc_next = SectorHead;
        if (SectorHead != NULL)
        {
            SectorHead->sc_prev = sc;
        }
        else
        {
            SectorTail = sc;
        }
        SectorHead = sc;
    }
    return FALSE;
}

/*
 * shipFlush - flush the ship cache. We flush the ships in increasing
 *      ship number order, so as to minimize disk seeking.
 */

void shipFlush(void)
{
    register ULONG minNum, i;
    BOOL foundDirty;
    register ShipCache_t *lowest = NULL, *shc;

    if (ShipFree != 0l)
    {
        do
        {
            foundDirty = FALSE;
            minNum = 0xffffffff;
            shc = &ShipCache[0];
            for (i = 0; i < ShipFree; i++)
            {
                if (shc->shc_dirty && (shc->shc_shipNumber < minNum))
                {
#ifdef DEBUG_SERV
                    if (DebugMode & DEBUG_SHIP)
                    {
                        logString("shipFlush() found dirty ship ");
                        logNumber(shc->shc_shipNumber);
                        logString("\n");
                    }
#endif
                    foundDirty = TRUE;
                    lowest = shc;
                    minNum = shc->shc_shipNumber;
                }
                shc++;
            }
            if (foundDirty)
            {
                if (fseek(ShipFd, minNum * sizeof(Ship_t), SEEK_SET))
                {
                    abortN("Can't seek to ship ", minNum);
                }
                if (fwrite(&lowest->shc_ship, sizeof(char), sizeof(Ship_t),
		    ShipFd) != sizeof(Ship_t))
                {
                    abortN("Can't write ship ", minNum);
                }
                lowest->shc_dirty = FALSE;
            }
        } while (foundDirty);
    }
}

/*
 * shipLookup - lookup/enter the requested ship in the ship cache.
 *      Return 'FALSE' if it was already there. Referencing it will always
 *      put it at the head of the chain.
 */

BOOL shipLookup(register ULONG shipNumber)
{
    register ULONG dirtyCount;
    register ShipCache_t *shc;
    BOOL doCont;

    shc = ShipHead;
    if (shc != NULL)
    {
        /* this is needed to prevent derefrencing a NULL pointer */
        doCont = (shc->shc_shipNumber != shipNumber);
        while (doCont)
        {
            shc = shc->shc_next;
            if (shc != NULL)
            {
                doCont = (shc->shc_shipNumber != shipNumber);
            }
            else
            {
                doCont = FALSE;
            }
        }
    }
    if (shc == NULL)
    {
        /* didn't find the needed ship - add it to the cache. */
        if (ShipFree != ShipCacheSize)
        {
            /* free slot left - just use it */
            shc = &ShipCache[ShipFree];
            ShipFree++;
        }
        else
        {
            /* no free slot - look for a non-dirty one */
            dirtyCount = 0;
            shc = ShipTail;
            if (shc != NULL)
            {
                /* this is needed to prevent derefrencing a NULL pointer */
                doCont = shc->shc_dirty;
                while (doCont)
                {
                    shc = shc->shc_prev;
                    dirtyCount++;
                    if (shc != NULL)
                    {
                        doCont = shc->shc_dirty;
                    }
                    else
                    {
                        doCont = FALSE;
                    }
                }
            }
            if ((dirtyCount > ShipCacheSize * 4 / 5) || (shc == NULL))
            {
                /* no non-dirty slot left. Flush all and use tail one. */
                shipFlush();
                shc = ShipTail;
            }
            /* delete the ship from it's current position in the chain */
            if (shc->shc_prev == NULL)
            {
                ShipHead = shc->shc_next;
            }
            else
            {
                shc->shc_prev->shc_next = shc->shc_next;
            }
            if (shc->shc_next == NULL)
            {
                ShipTail = shc->shc_prev;
            }
            else
            {
                shc->shc_next->shc_prev = shc->shc_prev;
            }
        }
        /* insert it at the head of the chain */
        shc->shc_prev = NULL;
        shc->shc_next = ShipHead;
        if (ShipHead != NULL)
        {
            ShipHead->shc_prev = shc;
        }
        else
        {
            ShipTail = shc;
        }
        ShipHead = shc;
        /* set it to be the requested ship */
        shc->shc_shipNumber = shipNumber;
        shc->shc_dirty = FALSE;
        return TRUE;
    }
    /* move it to the front of the LRU chain if its not there already */
    if (shc->shc_prev != NULL)
    {
        shc->shc_prev->shc_next = shc->shc_next;
        if (shc->shc_next != NULL)
        {
            shc->shc_next->shc_prev = shc->shc_prev;
        }
        else
        {
            ShipTail = shc->shc_prev;
        }
        shc->shc_prev = NULL;
        shc->shc_next = ShipHead;
        if (ShipHead != NULL)
        {
            ShipHead->shc_prev = shc;
        }
        else
        {
            ShipTail = shc;
        }
        ShipHead = shc;
    }
    return FALSE;
}

/*
 * planetFlush - flush the planet cache. We flush the planets in increasing
 *      planet number order, so as to minimize disk seeking.
 */

void planetFlush(void)
{
    register ULONG minNum, i;
    BOOL foundDirty;
    register PlanetCache_t *lowest = NULL, *plc;

    if (PlanetFree != 0l)
    {
        do
        {
            foundDirty = FALSE;
            minNum = 0xffffffff;
            plc = &PlanetCache[0];
            for (i = 0; i < PlanetFree; i++)
            {
                if (plc->plc_dirty && (plc->plc_planetNumber < minNum))
                {
#ifdef DEBUG_SERV
                    if (DebugMode & DEBUG_PLANET)
                    {
                        logString("planetFlush() found dirty planet ");
                        logNumber(plc->plc_planetNumber);
                        logString("\n");
                    }
#endif
                    foundDirty = TRUE;
                    lowest = plc;
                    minNum = plc->plc_planetNumber;
                }
                plc++;
            }
            if (foundDirty)
            {
                if (fseek(PlanetFd, (ULONG)(minNum) * sizeof(Planet_t),
                        SEEK_SET))
                {
                    abortN("Can't seek to planet ", minNum);
                }
                if (fwrite(&lowest->plc_planet, sizeof(char), sizeof(Planet_t),
		    PlanetFd) != sizeof(Planet_t))
                {
                    abortN("Can't write planet ", minNum);
                }
                lowest->plc_dirty = FALSE;
            }
        } while (foundDirty);
    }
}

/*
 * planetLookup - lookup/enter the requested planet in the planet cache.
 *      Return 'FALSE' if it was already there. Referencing it will always
 *      put it at the head of the chain.
 */

BOOL planetLookup(register ULONG planetNumber)
{
    register ULONG dirtyCount;
    register PlanetCache_t *plc;
    BOOL doCont;

    plc = PlanetHead;
    if (plc != NULL)
    {
        doCont = (plc->plc_planetNumber != planetNumber);
        while (doCont)
        {
            plc = plc->plc_next;
            if (plc != NULL)
            {
                doCont = (plc->plc_planetNumber != planetNumber);
            }
            else
            {
                doCont = FALSE;
            }
        }
    }
    if (plc == NULL)
    {
        /* didn't find the needed sector - add it to the cache. */
        if (PlanetFree != PlanetCacheSize)
        {
            /* free slot left - just use it */
            plc = &PlanetCache[PlanetFree];
            PlanetFree++;
        }
        else
        {
            /* no free slot - look for a non-dirty one */
            dirtyCount = 0;
            plc = PlanetTail;
            if (plc != NULL)
            {
                doCont = plc->plc_dirty;
                while (doCont)
                {
                    plc = plc->plc_prev;
                    dirtyCount++;
                    if (plc != NULL)
                    {
                        doCont = plc->plc_dirty;
                    }
                    else
                    {
                        doCont = FALSE;
                    }
                }
            }
            if ((dirtyCount > PlanetCacheSize * 4 / 5) || (plc == NULL))
            {
                /* no non-dirty slot left. Flush all and use tail one. */
                planetFlush();
                plc = PlanetTail;
            }
            /* delete the ship from it's current position in the chain */
            if (plc->plc_prev == NULL)
            {
                PlanetHead = plc->plc_next;
            }
            else
            {
                plc->plc_prev->plc_next = plc->plc_next;
            }
            if (plc->plc_next == NULL)
            {
                PlanetTail = plc->plc_prev;
            }
            else
            {
                plc->plc_next->plc_prev = plc->plc_prev;
            }
        }
        /* insert it at the head of the chain */
        plc->plc_prev = NULL;
        plc->plc_next = PlanetHead;
        if (PlanetHead != NULL)
        {
            PlanetHead->plc_prev = plc;
        }
        else
        {
            PlanetTail = plc;
        }
        PlanetHead = plc;
        /* set it to be the requested ship */
        plc->plc_planetNumber = planetNumber;
        plc->plc_dirty = FALSE;
        return TRUE;
    }
    /* move it to the front of the LRU chain if its not there already */
    if (plc->plc_prev != NULL)
    {
        plc->plc_prev->plc_next = plc->plc_next;
        if (plc->plc_next != NULL)
        {
            plc->plc_next->plc_prev = plc->plc_prev;
        }
        else
        {
            PlanetTail = plc->plc_prev;
        }
        plc->plc_prev = NULL;
        plc->plc_next = PlanetHead;
        if (PlanetHead != NULL)
        {
            PlanetHead->plc_prev = plc;
        }
        else
        {
            PlanetTail = plc;
        }
        PlanetHead = plc;
    }
    return FALSE;
}

/*
 * bigItemFlush - flush the big item cache. We flush the big items in increasing
 *      item order, so as to minimize disk seeking.
 */

void bigItemFlush(void)
{
    register ULONG minNum, i;
    BOOL foundDirty;
    register BigItemCache_t *lowest = NULL, *bic;

    if (BigItemFree != 0l)
    {
        do
        {
            foundDirty = FALSE;
            minNum = 0xffffffff;
            bic = &BigItemCache[0];
            for (i = 0; i < BigItemFree; i++)
            {
                if (bic->bic_dirty && (bic->bic_bigItemNumber < minNum))
                {
#ifdef DEBUG_SERV
                    if (DebugMode & DEBUG_BIGITEM)
                    {
                        logString("bigItemFlush() found dirty big item ");
                        logNumber(bic->bic_bigItemNumber);
                        logString("\n");
                    }
#endif
                    foundDirty = TRUE;
                    lowest = bic;
                    minNum = bic->bic_bigItemNumber;
                }
                bic++;
            }
            if (foundDirty)
            {
                if (fseek(BigItemFd, (ULONG)(minNum) * sizeof(BigItem_t),
                        SEEK_SET))
                {
                    abortN("Can't seek to big item ", minNum);
                }
                if (fwrite(&lowest->bic_bigItem, sizeof(char), sizeof(BigItem_t),
		    BigItemFd) != sizeof(BigItem_t))
                {
                    abortN("Can't write big item ", minNum);
                }
                lowest->bic_dirty = FALSE;
            }
        } while (foundDirty);
    }
}

/*
 * bigItemLookup - lookup/enter the requested big item in the big item cache.
 *      Return 'FALSE' if it was already there. Referencing it will always
 *      put it at the head of the chain.
 */

BOOL bigItemLookup(register ULONG bigItemNumber)
{
    register ULONG dirtyCount;
    register BigItemCache_t *bic;
    BOOL doCont;

    bic = BigItemHead;
    if (bic != NULL)
    {
        doCont = (bic->bic_bigItemNumber != bigItemNumber);
        while (doCont)
        {
            bic = bic->bic_next;
            if (bic != NULL)
            {
                doCont = (bic->bic_bigItemNumber != bigItemNumber);
            }
            else
            {
                doCont = FALSE;
            }
        }
    }
    if (bic == NULL)
    {
        /* didn't find the needed sector - add it to the cache. */
        if (BigItemFree != BigItemCacheSize)
        {
            /* free slot left - just use it */
            bic = &BigItemCache[BigItemFree];
            BigItemFree++;
        }
        else
        {
            /* no free slot - look for a non-dirty one */
            dirtyCount = 0;
            bic = BigItemTail;
            if (bic != NULL)
            {
                doCont = bic->bic_dirty;
                while (doCont)
                {
                    bic = bic->bic_prev;
                    dirtyCount++;
                    if (bic != NULL)
                    {
                        doCont = bic->bic_dirty;
                    }
                    else
                    {
                        doCont = FALSE;
                    }
                }
            }
            if ((dirtyCount > BigItemCacheSize * 4 / 5) || (bic == NULL))
            {
                /* no non-dirty slot left. Flush all and use tail one. */
                bigItemFlush();
                bic = BigItemTail;
            }
            /* delete the big item from it's current position in the chain */
            if (bic->bic_prev == NULL)
            {
                BigItemHead = bic->bic_next;
            }
            else
            {
                bic->bic_prev->bic_next = bic->bic_next;
            }
            if (bic->bic_next == NULL)
            {
                BigItemTail = bic->bic_prev;
            }
            else
            {
                bic->bic_next->bic_prev = bic->bic_prev;
            }
        }
        /* insert it at the head of the chain */
        bic->bic_prev = NULL;
        bic->bic_next = BigItemHead;
        if (BigItemHead != NULL)
        {
            BigItemHead->bic_prev = bic;
        }
        else
        {
            BigItemTail = bic;
        }
        BigItemHead = bic;
        /* set it to be the requested big item */
        bic->bic_bigItemNumber = bigItemNumber;
        bic->bic_dirty = FALSE;
        return TRUE;
    }
    /* move it to the front of the LRU chain if its not there already */
    if (bic->bic_prev != NULL)
    {
        bic->bic_prev->bic_next = bic->bic_next;
        if (bic->bic_next != NULL)
        {
            bic->bic_next->bic_prev = bic->bic_prev;
        }
        else
        {
            BigItemTail = bic->bic_prev;
        }
        bic->bic_prev = NULL;
        bic->bic_next = BigItemHead;
        if (BigItemHead != NULL)
        {
            BigItemHead->bic_prev = bic;
        }
        else
        {
            BigItemTail = bic;
        }
        BigItemHead = bic;
    }
    return FALSE;
}

/*
 * closeFiles - flush and close the Imperium data files.
 */

void closeFiles(void)
{
    sectorFlush();
    shipFlush();
    planetFlush();
    bigItemFlush();
    closeImperiumFiles();
}

/*
 * getFileName - read, into FileName, a data file name from the name file.
 */

void getFileName(char *defName)
{
    register USHORT i;

    if (FileFd == NULL)
    {
        strcpy(&FileName[0], Path);
        strcat(&FileName[0], defName);
    }
    else
    {
        i = 0;
        while (FileName[i] = ' ', (fread(&FileName[i], sizeof(char), 1,
	    FileFd) == 1) && (FileName[i] != '\n'))
        {
            i++;
        }
        if (FileName[i] != '\n')
        {
            fclose(FileFd);
            myAbort("Bad file name in name file");
        }
        FileName[i] = '\0';
    }
}

/*
 * tryToOpen - attempt to open one Imperium data file.
 */

FILE *tryToOpen(char *defName)
{
    FILE *fd;

    getFileName(defName);
    fd = fopen(&FileName[0], "r+b");
    if (fd == NULL)
    {
        if (logOpen())
        {
            logString("*** ");
            logString("can't open file ");
            logString(&FileName[0]);
            logString(" - aborting\n");
            fclose(LogFd);
	    LogFd = NULL;
        }
        closeImperiumFiles();
        if (FileFd != NULL)
        {
            fclose(FileFd);
        }
	(void) umask(oldUmask);
        exit(5);
    }
    return fd;
}

/*
 * openFiles - open the Imperium data files.
 */

void openFiles(char *path, char *pHelpDir, char *pDocDir)
{
    Path = path;
    /* if we can't open "imperium.files", we use some default names */
    strcpy(&FileName[0], path);
    strcat(&FileName[0], "imperium.files");
    FileFd = fopen(&FileName[0], "r");

    WorldFd = NULL;
    PlayerFd = NULL;
    SectorFd = NULL;
    ShipFd = NULL;
    FleetFd = NULL;
    LoanFd = NULL;
    OfferFd = NULL;
    SectorCache = NULL;
    ShipCache = NULL;
    PlanetCache = NULL;
    BigItemCache = NULL;
    getFileName("imp.log");
    strcpy(&LogFileName[0], &FileName[0]);
    WorldFd = tryToOpen("imp.world");
    PlayerFd = tryToOpen("imp.player");
    SectorFd = tryToOpen("imp.sector");
    PlanetFd = tryToOpen("imp.planet");
    ShipFd = tryToOpen("imp.ship");
    BigItemFd = tryToOpen("imp.bigitem");
    FleetFd = tryToOpen("imp.fleet");
    LoanFd = tryToOpen("imp.loan");
    OfferFd = tryToOpen("imp.offer");
    getFileName("../help/");
    strcpy(pHelpDir, &FileName[0]);
    getFileName("../doc/");
    strcpy(pDocDir, &FileName[0]);
    if (FileFd != NULL)
    {
        fclose(FileFd);
        FileFd = NULL;
    }
    SectorFree = 0;
    SectorHead = NULL;
    SectorTail = NULL;
    ShipFree = 0l;
    ShipHead = NULL;
    ShipTail = NULL;
    PlanetFree = 0l;
    PlanetHead = NULL;
    PlanetTail = NULL;
    BigItemFree = 0l;
    BigItemHead = NULL;
    BigItemTail = NULL;
    SectorCache = calloc(1, sizeof(SectorCache_t) * SectorCacheSize);
    if (SectorCache == NULL)
    {
        myAbort("Unable to allocate sector cache");
    }
    ShipCache = calloc(1, sizeof(ShipCache_t) * ShipCacheSize);
    if (ShipCache == NULL)
    {
        myAbort("Unable to allocate ship cache");
    }
    PlanetCache = calloc(1, sizeof(PlanetCache_t) * PlanetCacheSize);
    if (PlanetCache == NULL)
    {
        myAbort("Unable to allocate planet cache");
    }
    BigItemCache = calloc(1, sizeof(BigItemCache_t) * BigItemCacheSize);
    if (BigItemCache == NULL)
    {
        myAbort("Unable to allocate big item cache");
    }
}

/*
 * readWorld - read the world header and player information
 */

BOOL readWorld(World_t *pWorld, Player_t *pPlayer)
{
    if (fread(pWorld, sizeof(char), sizeof(World_t), WorldFd) != sizeof(World_t))
    {
        return FALSE;
    }
    if (fread(pPlayer, sizeof(char), PLAYER_MAX * sizeof(Player_t),
	PlayerFd) != PLAYER_MAX * sizeof(Player_t))
    {
        return FALSE;
    }
    return TRUE;
}

/*
 * writeWorld - write the world header and player information
 */

void writeWorld(World_t *pWorld, Player_t *pPlayer)
{
    if (fseek(PlayerFd, 0, SEEK_SET))
    {
        myAbort("Can't rewrite players");
    }
    if (fwrite(pPlayer, sizeof(char), PLAYER_MAX * sizeof(Player_t),
	PlayerFd) != PLAYER_MAX * sizeof(Player_t))
    {
        myAbort("Can't write players");
    }
    if (fseek(WorldFd, 0, SEEK_SET))
    {
        myAbort("Can't rewrite world file");
    }
    if (fwrite(pWorld, sizeof(char), sizeof(World_t),
	WorldFd) != sizeof(World_t))
    {
        myAbort("Can't write world");
    }
}

/*
 * readSector - read the given sector into a given buffer.
 */

void readSector(USHORT rowCol, Sector_t *s)
{
    if (sectorLookup(rowCol))
    {
        if (fseek(SectorFd, (ULONG)(rowCol) * sizeof(Sector_t), SEEK_SET))
        {
            abortN("Can't seek to sector ", rowCol);
        }
        if (fread(&SectorHead->sc_sector, sizeof(char), sizeof(Sector_t),
	    SectorFd) != sizeof(Sector_t))
        {
            abortN("Can't read sector ", rowCol);
        }
    }
    *s = SectorHead->sc_sector;
}

/*
 * writeSector - write the given sector from a given buffer.
 */

void writeSector(USHORT rowCol, Sector_t *s)
{
    (void) sectorLookup(rowCol);
    SectorHead->sc_sector = *s;
    SectorHead->sc_dirty = TRUE;
}

/*
 * readShip - read the given ship into a given buffer.
 */

void readShip(ULONG shipNumber, Ship_t *ship)
{
    if (shipLookup(shipNumber))
    {
        if (fseek(ShipFd, shipNumber * sizeof(Ship_t), SEEK_SET))
        {
            abortN("Can't seek to ship ", shipNumber);
        }
        if (fread(&ShipHead->shc_ship, sizeof(char), sizeof(Ship_t),
	    ShipFd) != sizeof(Ship_t))
        {
            abortN("Can't read ship ", shipNumber);
        }
    }
    *ship = ShipHead->shc_ship;
}

/*
 * writeShip - write the given ship from a given buffer.
 */

void writeShip(Ship_t *ship)
{
    (void) shipLookup(ship->sh_number);
    ShipHead->shc_ship = *ship;
    ShipHead->shc_dirty = TRUE;
}

/*
 * readFleet - read the given fleet into a given buffer.
 */

void readFleet(USHORT fleetNumber, Fleet_t *fleet)
{
    if (fseek(FleetFd, (ULONG)(fleetNumber) * sizeof(Fleet_t), SEEK_SET))
    {
        abortN("Can't seek to fleet ", fleetNumber);
    }
    if (fread(fleet, sizeof(char), sizeof(Fleet_t), FleetFd) != sizeof(Fleet_t))
    {
        abortN("Can't read fleet ", fleetNumber);
    }
}

/*
 * writeFleet - write the given fleet from a given buffer.
 */

void writeFleet(USHORT fleetNumber, Fleet_t *fleet)
{
    if (fseek(FleetFd, (ULONG)(fleetNumber) * sizeof(Fleet_t), SEEK_SET))
    {
        abortN("Can't seek to fleet ", fleetNumber);
    }
    if (fwrite(fleet, sizeof(char), sizeof(Fleet_t),
	FleetFd) != sizeof(Fleet_t))
    {
        abortN("Can't write fleet ", fleetNumber);
    }
}

/*
 * readLoan - read the given loan into a given buffer.
 */

void readLoan(USHORT loanNumber, Loan_t *loan)
{
    if (fseek(LoanFd, (ULONG)(loanNumber) * sizeof(Loan_t), SEEK_SET))
    {
        abortN("Can't seek to loan ", loanNumber);
    }
    if (fread(loan, sizeof(char), sizeof(Loan_t), LoanFd) != sizeof(Loan_t))
    {
        abortN("Can't read loan ", loanNumber);
    }
}

/*
 * writeLoan - write the given loan from a given buffer.
 */

void writeLoan(USHORT loanNumber, Loan_t *loan)
{
    if (fseek(LoanFd, (ULONG)(loanNumber) * sizeof(Loan_t), SEEK_SET))
    {
        abortN("Can't seek to loan ", loanNumber);
    }
    if (fwrite(loan, sizeof(char), sizeof(Loan_t), LoanFd) != sizeof(Loan_t))
    {
        abortN("Can't write loan ", loanNumber);
    }
}

/*
 * readOffer - read the given offer into a given buffer.
 */

void readOffer(USHORT offerNumber, Offer_t *offer)
{
    if (fseek(OfferFd, (ULONG)(offerNumber) * sizeof(Offer_t), SEEK_SET))
    {
        abortN("Can't seek to offer ", offerNumber);
    }
    if (fread(offer, sizeof(char), sizeof(Offer_t), OfferFd) != sizeof(Offer_t))
    {
        abortN("Can't read offer ", offerNumber);
    }
}

/*
 * writeOffer - write the given offer from a given buffer.
 */

void writeOffer(USHORT offerNumber, Offer_t *offer)
{
    if (fseek(OfferFd, (ULONG)offerNumber * sizeof(Offer_t), SEEK_SET))
    {
        abortN("Can't seek to offer ", offerNumber);
    }
    if (fwrite(offer, sizeof(char), sizeof(Offer_t), OfferFd) != sizeof(Offer_t))
    {
        abortN("Can't write offer ", offerNumber);
    }
}

/*
 * readPlanet - read the given planet into a given buffer.
 */

void readPlanet(ULONG planetNumber, Planet_t *planet)
{
    if (planetLookup(planetNumber))
    {
        if (fseek(PlanetFd, planetNumber * sizeof(Planet_t), SEEK_SET))
        {
            abortN("Can't seek to planet ", planetNumber);
        }
        if (fread(&PlanetHead->plc_planet, sizeof(char), sizeof(Planet_t),
	    PlanetFd) != sizeof(Planet_t))
        {
            abortN("Can't read planet ", planetNumber);
        }
    }
    *planet = PlanetHead->plc_planet;
}

/*
 * writePlanet - write the given planet from a given buffer.
 */

void writePlanet(Planet_t *planet)
{
    (void) planetLookup(planet->pl_number);
    PlanetHead->plc_planet = *planet;
    PlanetHead->plc_dirty = TRUE;
}

/*
 * readBigItem - read the given big item into a given buffer.
 */

void readBigItem(ULONG bigItemNumber, BigItem_t *bigItem)
{
    if (bigItemLookup(bigItemNumber))
    {
        if (fseek(BigItemFd, (ULONG)(bigItemNumber) * sizeof(BigItem_t),
            SEEK_SET))
        {
            abortN("Can't seek to big item ", bigItemNumber);
        }
        if (fread(&BigItemHead->bic_bigItem, sizeof(char), sizeof(BigItem_t),
	    BigItemFd) != sizeof(BigItem_t))
        {
            abortN("Can't read big item ", bigItemNumber);
        }
    }
    *bigItem = BigItemHead->bic_bigItem;
}

/*
 * writeBigItem - write the given big item from a given buffer.
 */

void writeBigItem(BigItem_t *bigItem)
{
    (void) bigItemLookup(bigItem->bi_number);
    BigItemHead->bic_bigItem = *bigItem;
    BigItemHead->bic_dirty = TRUE;
}

