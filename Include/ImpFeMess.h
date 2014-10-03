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
 * definitions private to the Imperium library.
 *
 * $Id: ImpFeMess.h,v 1.1.1.1 2000/05/17 19:15:04 marisa Exp $
 *
 * $Log: ImpFeMess.h,v $
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:35  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:58:27  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:34:29  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:00:21  marisag
 * patch20: Fix empty revision.
 *
 */

/*
 * Things for the FE support messages
 */


/*
 *          Messages that Imperium may send
 */

#define FE_COMMENT  "!<"        /* Introduces all comments      */

#define FE_PLDUMP   "!@D"       /* Introduces a planet dump     */
#define FE_PLDIRTY  "!@U"       /* Introduces a "dirty" planet #*/
#define FE_PLGEO    "!@1"       /* A planet geo report follows  */
#define FE_PLPOP    "!@2"       /* A planet population rept foll*/
#define FE_PLITE    "!@3"
#define FE_PLBIG    "!@4"       /* A planet big item rept follw */
#define FE_PLPRO    "!@51"      /* A planet production rept. l1 */
#define FE_PLPRO2   "!@52"      /* A planet production rept. l2 */
#define FE_PLSCAN   "!@6"       /* A planet scan line           */
#define FE_PLRETSCAN "!@7"      /* Info we got about a planet who scanned us */


#define FE_SHDUMP   "!$D"       /* Introduces a ship dump line  */
#define FE_SHDIRTY  "!$U"       /* Introduces a "dirty" ship #  */
#define FE_SHSTAT   "!$1"       /* A ship status rept follows   */
#define FE_SHCARGO  "!$2"       /* A ship cargo rept follows    */
#define FE_SHBIG    "!$3"       /* A ship big item rpt follows  */
#define FE_SHCONF   "!$4"       /* A ship config rpt follows    */
#define FE_SHSCAN   "!$5"       /* A ship scan line             */
#define FE_SHSCDET  "!$6"       /* A ship detected via SRS/LRS/Vis scan */
#define FE_SHRETSCAN  "!$7"     /* Info we got about a ship who scanned us */

#define FE_SRS      "!)S"
#define FE_LRS      "!)R"
#define FE_VRS      "!)V"

#define FE_CHREQ    "!*C\n"
#define FE_PLAYLIST "!*L"       /* A player list follows            */
#define FE_POWREP   "!*\x50"    /* A power report follows           */
#define FE_RACEREP  "!*R1"      /* A race report follows            */
#define FE_RACEREP2 "!*R2"      /* A race report (line 2) follows   */
#define FE_PLAYSTAT "!*SS"      /* A player status command follows  */
#define FE_PLAYHOME "!*SH"      /* A player status (home plan) follows  */
#define FE_PRINTREALM "!*A"     /* A realm line follows             */
#define FE_WSIZE    "!*W"       /* A info world size line           */

#define FE_ATTLOSE  "!&L\n"     /* The last attack failed           */
#define FE_ATTWIN   "!&W\n"     /* The last attack succeeded        */

#define FE_DISP_PROMPT "!("     /* The prompt (if any) follows      */
                                /* the "!(" sequence (until EOL)    */
#define FE_GET_LINE "![\n"      /* The FE should get a line from the*/
                                /* user.                            */
#define FE_GET_CANC "!]\n"      /* The previous line requenst has   */
                                /* been canceled                    */

/*
 *      Bit-values for items the FE wants to see
 */

#define FE_WANT_COMM    0x0001  /* The player wants comments    */
#define FE_WANT_PLAN    0x0002  /* The player wants planet info */
#define FE_WANT_SHIP    0x0004  /* The player wants ship info   */
#define FE_WANT_MISC    0x0008  /* The player wants misc. info  */
#define FE_WANT_SCAN    0x0010  /* The player wants scanner info */
#define FE_WANT_ATTK    0x0020  /* The player wants attack info */

/*
 *      Bit-values for things the FE supports
 */

#define FE_HAS_GRAPH    0x0100
#define FE_HAS_SOUND    0x0200
#define FE_HAS_MOUSE    0x0400
#define FE_HAS_JOYST    0x0800
#define FE_HAS_2COLR    0x1000
#define FE_HAS_4COLR    0x2000
#define FE_HAS_PROMPT   0x4000  /* The FE supports "prompt" mode    */
