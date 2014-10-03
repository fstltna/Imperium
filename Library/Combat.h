/*
 * Imperium
 *
 * $Id: Combat.h,v 1.2 2000/05/23 20:25:14 marisa Exp $
 *
 * definitions private to combat-related functions
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
 * $Log: Combat.h,v $
 * Revision 1.2  2000/05/23 20:25:14  marisa
 * Added pl_weapon[] element to Planet_t struct
 * Began working on ship<->planet combat code
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:41  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:15:49  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:32  marisag
 * patch20: Fix empty revision.
 *
 */

/*
 * Note that this file has some dependancies on things declared in
 * Imperium.h (mainly the weapon types). This had to be done to avoid
 * either putting all the combat-related definitions in Imperium.h, which
 * is already to big and contains things not needed by all modules, or
 * avoid putting the things that this file depended on in here, thus forcing
 * more modules than need to to include this file.
 *
 * Basically, the things in Imperium.h are of general interest, while the
 * things in this file are of interest only to routines that directly
 * handle combat.
 */

/*
 * weapons.c
 */

/* Picks the best "beam" weapon to use */
BigPart_t pickBestBeam(IMP, register Ship_t *);
/* Picks the best "projectile" weapon to use */
BigPart_t pickBestProj(IMP, register Ship_t *);
/* Picks the "best" weapon to use of either type */
BigPart_t pickBestWeap(IMP, register Ship_t *, ULONG, BOOL, USHORT, USHORT);

BOOL hasBeam(IMP, register Ship_t *);   /* Ship has a beam weapon */
BOOL hasProj(IMP, register Ship_t *);   /* Ship has a projectile weapon */
BOOL hasWeap(IMP, register Ship_t *);
BOOL hasBeamPl(IMP, register Planet_t *);   /* Ship has a beam weapon */
BOOL hasProjPl(IMP, register Planet_t *);   /* Ship has a projectile weapon */
BOOL hasWeapPl(IMP, register Planet_t *);

/* Picks the amount to try and fire - for use in defending a ship */
USHORT pickAmtToFire(IMP, register Ship_t *, BigPart_t, BOOL);
