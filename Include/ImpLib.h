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
 * $Id: ImpLib.h,v 1.1.1.1 2000/05/17 19:15:04 marisa Exp $
 *
 * $Log: ImpLib.h,v $
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:36  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:34:30  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:00:23  marisag
 * patch20: Fix empty revision.
 *
 */

/*
 * ImpLib.h - definitions for using the Imperium library.
 */

#define IMPERIUM_PORT	"imperium.port"
#define IMP_TEST_PORT	"imp.test.port"

#define INPUT_BUFFER_SIZE       500
#define OUTPUT_BUFFER_SIZE      500
#define REQUEST_PRIVATE_SIZE    2048
#define IMPERIUM_PRIVATE_SIZE   2642

/* Alas, I would like to use an enum type, but that wastes 3 bytes */
#define rt_nop          0       /* no operation                     */
#define rt_log          1       /* log a string                     */
#define rt_startClient  2       /* start a new client               */
#define rt_stopClient   3       /* stop this client                 */
#define rt_shutDown     4       /* shut down the server             */
#define rt_flush        5       /* request flush of all data        */
#define rt_poll         6       /* check to see if we should exit   */
#define rt_writeWorld   7       /* write out the world buffer       */

    /* Please note the following request types are NOT for */
    /* "normal" client program uses!                       */
#define rt_acOut        8       /* AC power is out - for utilities  */
#define rt_acBack       9       /* AC power came back online        */
#define rt_battLow      10      /* UPS battery is low, exit immed.  */
#define rt_backStart    80      /* request a backup lock on the     */
                                /* system. Will NOT return until OK */
                                /* to start the backup              */
#define rt_backDone     81      /* indicate to the server you are   */
                                /* done with a backup               */
#define rt_broadcastMsg 82      /* send a message to all clients    */
#define rt_publish      83      /* publish our server               */

  /* note that there are many others that are private! */
typedef UBYTE RequestType_t;

typedef struct
    {
        ULONG
                    rq_clientId,        /* id of this client            */
                    rq_time,            /* time from server             */
                    rq_whichUnit,       /* unit identifier              */
                    rq_otherUnit,       /* identifier of other in pair  */
                    rq_specialFlags;    /* special flags - CAUTION      */
        RequestType_t
                    rq_type;            /* type of request              */
        char        rq_text[133];       /* holds C-style string to be   */
                                        /* broadcast w/rt_broadcastMsg  */
        char rq_private[REQUEST_PRIVATE_SIZE];
        /* rq_private is 'char' so we can use it for rt_log requests    */
    } Request_t;

/*
 * Special flag definitions for use in the "is_specialFlags" field in
 *          ImpState_t.
 *          Note that some of these flags may be set by clients or
 *          the library itself. Please check for and honor them
 *          even if you didn't set them yourself!
 */

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


/*
 * ImpState_t - The main "Imperium State" structure
 */

typedef struct
    {
        /* fields used to interface to the library caller */

        void (*is_serverRequest)(void);
        void (*is_writeUser)(void);
        void (*is_readUser)(void);
        void (*is_timedReadUser)(void); /* This is basically the same as */
                                        /* readUser, but should time out */
                                        /* after 1 sec or so if the user */
                                        /* does not start typing or the  */
                                        /* input line is empty           */
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

        /* the next two fields are READ ONLY for clients and reflect    */
        /* the users desired screen size. These will be valid as soon   */
        /* as the "last on" message is displayed when the user logs in  */
        USHORT      is_conWidth,
                    is_conLength;   /* for both these fields '0' values */
                                    /* indicate the user does not want  */
                                    /* that feature activated           */


        /* the next fields hold values used for communication with the  */
        /* call-back functions */
        BOOL        is_argBool;     /* used to hold BOOL values         */
        USHORT      is_argShort;    /* used to hold short values        */
        ULONG       is_argLong;     /* used to hold long values         */
        ULONG       is_specialFlags;/* special flags, see above         */
        char       *is_argPoint;    /* used to hold char pointer values */

        /* fields private to the Imperium library itself */

        UBYTE       is_impPrivate[IMPERIUM_PRIVATE_SIZE];
    } ImpState_t;
#define IMP register ImpState_t *IS

extern void ImpCntrl(register ImpState_t *);
extern ImpState_t *ImpAlloc(void);
extern void ImpFree(register ImpState_t *);
extern void Imperium(register ImpState_t *);
