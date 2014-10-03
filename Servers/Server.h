/*
 * Imperium
 *
 * $Id: Server.h,v 1.1.1.1 2000/05/17 19:15:04 marisa Exp $
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
 * $Log: Server.h,v $
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:31:08  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:13:48  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:06:10  marisag
 * patch20: Fix empty rev.
 *
 */

/*
 * Server.h - include file for Imperium server - define function prototypes
 */

/* FileIO.c */

void log(char *);
void logN(char *, ULONG, char *);
void myAbort(char *);
void closeFiles(void);
void openFiles(char *, char *, char *);
BOOL readWorld(World_t *, Player_t *);
void writeWorld(World_t *, Player_t *);
void readSector(USHORT, Sector_t *);
void writeSector(USHORT, Sector_t *);
void readShip(ULONG, Ship_t *);
void writeShip(Ship_t *);
void readFleet(USHORT, Fleet_t *);
void writeFleet(USHORT, Fleet_t *);
void readLoan(USHORT, Loan_t *);
void writeLoan(USHORT, Loan_t *);
void readOffer(USHORT, Offer_t *);
void writeOffer(USHORT, Offer_t *);
void readPlanet(ULONG, Planet_t *);
void writePlanet(Planet_t *);
void readBigItem(ULONG, BigItem_t *);
void writeBigItem(BigItem_t *);

#ifdef DEBUG_SERV
#define DEBUG_SHIP      0x1
#define DEBUG_PLANET    0x2
#define DEBUG_BIGITEM   0x4
#endif

