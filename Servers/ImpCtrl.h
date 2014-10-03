/*
 * Imperium
 *
 * $Id: ImpCtrl.h,v 1.1.1.1 2000/05/17 19:15:04 marisa Exp $
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
 * Definitions used by the ImpControl() interface
 *
 * $Log: ImpCtrl.h,v $
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:31:06  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:13:51  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:06:09  marisag
 * patch20: Fix empty rev.
 *
 */

/*
 * Actions that can be performed through the ImpControl() entry point in
 * the library. These get placed in is_argShort.
 */

#define IC_RESET    0   /* reset to defaults            */
#define IC_FPOWER   1   /* force power update           */
#define IC_POWER    2   /* display-only power update    */
#define IC_UPDATE   3   /* do global planet update      */
#define IC_MINER    4   /* do global miner update       */
#define IC_BACKS    5   /* start a backup               */
#define IC_BACKE    6   /* complete a backup            */
#define IC_INCUSR   7   /* incremensts the user count   */
#define IC_DECUSR   8   /* decrements the user count    */
#define IC_DOFLUSH  9   /* flush the server's buffers   */

/*
 * Additional notes:
 *
 *  1) If is_argBool is TRUE then actions will be logged. If FALSE, no
 *     actions will be logged.
 *
 */

