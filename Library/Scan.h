/*
 * Imperium
 *
 * $Id: Scan.h,v 1.1.1.1 2000/05/17 19:15:04 marisa Exp $
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
 * Feel free to modify and use these sources however you wish, so long
 * as you preserve this copyright notice.
 *
 * $Log: Scan.h,v $
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:45  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:15:51  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:34  marisag
 * patch20: Fix empty revision.
 *
 */

#define MAX_CONDITIONS  8               /* max # '?' conditions     */
#define MAX_SHIPS       32              /* max # specific ships     */
#define MAX_PLANETS     32              /* max # specific planets   */
#define MAX_MINERS      32              /* max # specific miners    */


typedef struct
    {
        long
            c_left;
        long
            c_right;
        char
            c_operator;
    } Condition_t;

#define shp_none    0                   /* no conditions            */
#define shp_list    1                   /* list of ship numbers     */
#define shp_box     2                   /* sectors they are in      */
#define shp_fleet   3                   /* a fleet they are in      */
#define shp_plList  4                   /* list of planet numbers   */
typedef UBYTE ShipPattern_t;

#define pp_none     0                   /* no conditions            */
#define pp_list     1                   /* list of planet numbers   */
#define pp_box      2                   /* sectors they are in      */
typedef UBYTE PlanetPattern_t;

#define mnp_none    0                   /* no conditions            */
#define mnp_list    1                   /* list of miner numbers    */
#define mnp_box     2                   /* sectors they are in      */
#define mnp_shList  3                   /* list of ship numbers     */
#define mnp_plList  4                   /* list of planet numbers   */
typedef UBYTE MinerPattern_t;

typedef struct
    {
        USHORT
            cs_conditionCount;
        Condition_t
            cs_condition[MAX_CONDITIONS];
        USHORT
            cs_boxTop,
            cs_boxBottom,
            cs_boxLeft,
            cs_boxRight;
    } ConditionSet_t;

typedef struct
    {
        ConditionSet_t
            shs_cs;
        ShipPattern_t
            shs_shipPatternType;
        ULONG
            shs_shipCount;
        ULONG
            shs_shipList[MAX_SHIPS];
        UBYTE
            shs_shipFleet;
        Ship_t
            shs_currentShip;
    } ShipScan_t;

typedef struct
    {
        ConditionSet_t
            ps_cs;
        PlanetPattern_t
            ps_planetPatternType;
        ULONG
            ps_planetCount;
        ULONG
            ps_planetList[MAX_PLANETS];
        Planet_t
            ps_currentPlanet;
    } PlanetScan_t;

typedef struct
    {
        ConditionSet_t
            ss_cs;
        Sector_t
            ss_currentSector;
        BOOL
            ss_mapHook;
    } SectorScan_t;

typedef struct
    {
        ConditionSet_t
            mns_cs;
        MinerPattern_t
            mns_minerPatternType;
        ULONG
            mns_minerCount;
        ULONG
            mns_minerList[MAX_MINERS];
        Ship_t
            mns_currentMiner;
    } MinerScan_t;

