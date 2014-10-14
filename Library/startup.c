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
 * startup and library interface portion of the Imperium library.
 *
 * $Id: startup.c,v 1.3 2000/05/18 07:29:41 marisa Exp $
 *
 * $Log: startup.c,v $
 * Revision 1.3  2000/05/18 07:29:41  marisa
 * More autoconf
 *
 * Revision 1.2  2000/05/18 06:41:51  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:58  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:59:26  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:24:36  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:26  marisag
 * patch20: Fix empty revision.
 * 
 */

#include "../config.h"

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
BOGUS - Imperium can not run on this machine due to missing stdlib.h
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <time.h>

#define ImpStartC 1
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "../Include/ImpFeMess.h"
#include "Scan.h"
#include "ImpPrivate.h"
#include "../Servers/ImpCtrl.h"

static const char rcsid[] = "$Id: startup.c,v 1.3 2000/05/18 07:29:41 marisa Exp $";

/*
 * ImpAlloc - allocates a ImpState_t structure, so the user program does not
 *            need to know how this was done.
 */

ImpState_t *ImpAlloc(void)
{
    return (ImpState_t *)calloc(1, sizeof(ImpState_t));
}

/*
 * ImpFree - frees up an ImpState_t structure
 */

void ImpFree(register ImpState_t *IS)
{
    free(IS);
}


void clServerReq(IMP)
{
    IS->is_serverRequest();
}

void clWriteUser(IMP)
{
    IS->is_writeUser();
}

BOOL clReadUser(IMP)
{
/*    if (IS->is_player.p_feMode & FE_HAS_PROMPT)
    {
        user(IS, FE_GET_LINE);
    }*/
    IS->is_readUser();
    return IS->is_argBool;
}

BOOL clTimedReadUser(IMP)
{
    IS->is_timedReadUser();
    return IS->is_argBool;
}

void clEchoOff(IMP)
{
    IS->is_echoOff();
}

void clEchoOn(IMP)
{
    IS->is_echoOn();
}

BOOL clGotCtrlC(IMP)
{
    IS->is_gotControlC();
    return IS->is_argBool;
}

void clSleep(IMP, USHORT tenthsOfSeconds)
{
    IS->is_argShort = (short)tenthsOfSeconds;
    IS->is_sleep();
}

BOOL clLog(IMP, char *fileName)
{
    IS->is_argLong = (ULONG)fileName;
    IS->is_log();
    return IS->is_argBool;
}

USHORT clEdit(IMP, char *buf, ULONG length)
{
    IS->is_argPoint = buf;
    IS->is_argLong = length;
    IS->is_extEdit();
    return IS->is_argShort;
}

/*
 * user - send a string to the user.
 */

void user(IMP, register char *str)
{
    register char *p;
    register USHORT lenLeft;
    char ch;

    lenLeft = IS->is_textOutPos;
    p = &IS->is_textOut[lenLeft];
    lenLeft = OUTPUT_BUFFER_SIZE - lenLeft;
    while (*str != '\0')
    {
        ch = *str;
        str++;
        *p = ch;
        p++;
        lenLeft--;
        if (((ch == '\n') && (!IS->is_noWrite)) || (lenLeft == 0))
        {
            IS->is_textOutPos = OUTPUT_BUFFER_SIZE - lenLeft;
            clWriteUser(IS);
            p = &IS->is_textOut[0];
            lenLeft = OUTPUT_BUFFER_SIZE;
        }
    }
    IS->is_textOutPos = OUTPUT_BUFFER_SIZE - lenLeft;
}

/*
 * userNL - a newline on user output.
 */

void userNL(IMP)
{
    user(IS, "\n");
}

/*
 * userC - add a single character to the output.
 */

void userC(IMP, register const char ch)
{
    char buffer[2];

    buffer[0] = ch;
    buffer[1] = '\0';
    user(IS, &buffer[0]);
}

/*
 * userSp - a single space on user output.
 */

void userSp(IMP)
{
    user(IS, " ");
}

/*
 * uFlush - flush the output buffer.
 */

void uFlush(IMP)
{
    if (IS->is_textOutPos != 0)
    {
        clWriteUser(IS);
        IS->is_textOutPos = 0;
    }
}

/*
 * uPrompt - display a prompt to the user.
 */

void uPrompt(IMP, register char *prompt)
{
    /* See if the user supports local prompts */
    if (IS->is_player.p_feMode & FE_HAS_PROMPT)
    {
        /* They do */
        user(IS, FE_DISP_PROMPT);
        user(IS, prompt);
        user(IS, "\n");
    }
    else
    {
        /* They don't */
        user(IS, prompt);
        user(IS, ": ");
    }
    /* Note we flush in all cases, as the overhead is low if not needed */
    uFlush(IS);
}

/*
 * userN - output a 32 bit decimal value to the user stream.
 */

void userN(IMP, register long n)
{
    char buffer[12];
    register char *p;
    BOOL isNeg;

    if (n >= 0)
    {
        n = -n;
        isNeg = FALSE;
    }
    else
    {
        isNeg = TRUE;
    }
    p = &buffer[11];
    *p = '\0';
    do
    {
        p--;
        *p = -(n % 10) + '0';
        n /= 10;
    }
    while (n != 0);
    if (isNeg)
    {
        p--;
        *p = '-';
    }
    user(IS, p);
}

/*
 * userF - output a decimal value using the given number of characters.
 */

void userF(IMP, register long n, register short digits)
{
    char buffer[25];
    register char *p;
    BOOL isNeg, pad;

    if (digits < 0)
    {
        pad = TRUE;
        digits = -digits;
    }
    else
    {
        pad = FALSE;
    }
    if (n >= 0)
    {
        n = -n;
        isNeg = FALSE;
    }
    else
    {
        isNeg = TRUE;
    }
    p = &buffer[19];
    *p = '\0';
    do
    {
        p--;
        *p = -(n % 10) + '0';
        digits--;
        n /= 10;
    }
    while (n != 0);
    if (isNeg)
    {
        p--;
        *p = '-';
        digits--;
    }
    while (digits > 0)
    {
        p--;
        if (pad)
        {
            *p = '0';
        }
        else
        {
            *p = ' ';
        }
        digits--;
    }
    user(IS, p);
}

/*
 * userX - output a value with the given number of hex digits.
 */

void userX(IMP, register ULONG n, register USHORT digits)
{
    char buffer[12];
    register char *p;
    register USHORT digit;

    p = &buffer[8];
    *p = '\0';
    while (digits > 0)
    {
        digits--;
        digit = n & 0xf;
        p--;
        if (digit >= 10)
        {
            *p = digit - 10 + 'A';
        }
        else
        {
            *p = digit + '0';
        }
        n = n >> 4;
    }
    user(IS, p);
}

/*
 * user2 - write 2 strings to the user.
 */

void user2(IMP, register char *m1, register char *m2)
{
    user(IS, m1);
    user(IS, m2);
}

/*
 * user3 - write 3 strings to the user.
 */

void user3(IMP, register char *m1, register char *m2, register char *m3)
{
    user(IS, m1);
    user(IS, m2);
    user(IS, m3);
}

/*
 * userN2 - write 1 string and a number to the user.
 */

void userN2(IMP, register char *m1, register long n)
{
    user(IS, m1);
    userN(IS, n);
}

/*
 * userN3 - write 2 strings and a number to the user.
 */

void userN3(IMP, register char *m1, register long n, register char *m2)
{
    user(IS, m1);
    userN(IS, n);
    user(IS, m2);
}

/*
 * userS - output 2 messages and a sector coordinate.
 */

void userS(IMP, register char *m1, USHORT r, USHORT c, register char *m2)
{
    user(IS, m1);
    userN(IS, r);
    user(IS, ",");
    userN(IS, c);
    user(IS, m2);
}

/*
 * userP - output 2 messages, a planet number, and possibly a planet name
 */

void userP(IMP, register char *m1, register Planet_t *p,
    register char *m2)
{
    user(IS, m1);
    userN(IS, p->pl_number);
    if (p->pl_name[0] != '\0')
    {
        user3(IS, " (", &p->pl_name[0], ")");
    }
    user(IS, m2);
}

/*
 * userSh - output 2 messages, a ship number, and possibly a ship name
 */

void userSh(IMP, register char *m1, register Ship_t *sh,
    register char *m2)
{
    user(IS, m1);
    userN(IS, sh->sh_number);
    if (sh->sh_name[0] != '\0')
    {
        user3(IS, " (", &sh->sh_name[0], ")");
    }
    user(IS, m2);
}

/*
 * userM3 - output a monetary value with a decimal point, and 2 messages.
 */

void userM3(IMP, register char *m1, register ULONG price, register char *m2)
{
    user(IS, m1);
    userN(IS, price / 10);
    userC(IS, '.');
    userN(IS, price % 10);
    userC(IS, '0');
    user(IS, m2);
}

/*
 * getPrompt - get a prompt from the output buffer.
 */

void getPrompt(IMP, register char *buffer)
{
    IS->is_textOut[IS->is_textOutPos] = '\0';
    strcpy(buffer, &IS->is_textOut[0]);
    IS->is_textOutPos = 0;
}

/*
 * ask - get a yes/no answer from the user.
 */

BOOL ask(IMP, register char *question)
{
    char ch;

    uPrompt(IS, question);
    if (clReadUser(IS))
    {
        ch = *skipBlanks(IS);
        IS->is_textInPos = &IS->is_textIn[0];
        IS->is_textIn[0] = '\0';
        if ((ch == 'y') || (ch == 'Y') || (ch == 'o') || (ch == 'O'))
        {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

/*
 * server - simple interface stub to do a server request.
 */

void server(IMP, register const RequestType_t rt,
    register const ULONG whichUnit)
{
    IS->is_request.rq_type = rt;
    IS->is_request.rq_whichUnit = whichUnit;
    clServerReq(IS);
}

/*
 * server2 - similar interface, but for a pair.
 */

void server2(IMP, register const RequestType_t rt,
    register const ULONG whichUnit, register const ULONG otherUnit)
{
    IS->is_request.rq_type = rt;
    IS->is_request.rq_whichUnit = whichUnit;
    IS->is_request.rq_otherUnit = otherUnit;
    clServerReq(IS);
}

/*
 * log3 - write a 3 - part log message to the log file.
 *      NOTE: there is NO checking for overflow on this. BE CAREFUL!
 */

void log3(IMP, char *m1, char *m2, char *m3)
{
    register char *p, *q;

    p = &IS->is_request.rq_u.ru_text[0];
    q = m1;
    while (*q != '\0')
    {
        *p = *q;
        p++;
        q++;
    }
    q = m2;
    while (*q != '\0')
    {
        *p = *q;
        p++;
        q++;
    }
    q = m3;
    while (*q != '\0')
    {
        *p = *q;
        p++;
        q++;
    }
    *p = '\0';
    server(IS, rt_log, 0);
}

/*
 * log4 - write a 4 - part log message to the log file.
 *      NOTE: there is NO checking for overflow on this. BE CAREFUL!
 */

void log4(IMP, char *m1, char *m2, char *m3, char *m4)
{
    register char *p, *q;

    p = &IS->is_request.rq_u.ru_text[0];
    q = m1;
    while (*q != '\0')
    {
        *p = *q;
        p++;
        q++;
    }
    q = m2;
    while (*q != '\0')
    {
        *p = *q;
        p++;
        q++;
    }
    q = m3;
    while (*q != '\0')
    {
        *p = *q;
        p++;
        q++;
    }
    q = m4;
    while (*q != '\0')
    {
        *p = *q;
        p++;
        q++;
    }
    *p = '\0';
    server(IS, rt_log, 0);
}

/*
 * cmd_flush - deity command to tell the server to flush the files.
 */

void cmd_flush(IMP)
{
    if ((IS->is_player.p_status != ps_deity) && (!IS->is_world.w_userFlush))
    {
        err(IS, "only a deity can use this command");
    }
    else
    {
        server(IS, rt_flush, 0);
    }
}

/*
 * cmd_tickle - deity command to bring the time on all sectors, ships, etc.
 *      forward by the time the game has been idle.
 */

void cmd_tickle(IMP)
{
    register ULONG delta;
    ULONG i;

    if (IS->is_player.p_status != ps_deity)
    {
        err(IS, "only a deity can use this command");
    }
    else
    {
        delta = timeNow(IS) - timeRound(IS, IS->is_world.w_lastRun);
        if (IS->is_world.w_loanNext != 0)
        {
            for (i = 0; i < IS->is_world.w_loanNext; i++)
            {
                server(IS, rt_lockLoan, i);
                IS->is_request.rq_u.ru_loan.l_lastPay += delta;
                IS->is_request.rq_u.ru_loan.l_dueDate += delta;
                server(IS, rt_unlockLoan, i);
            }
        }
        if (IS->is_world.w_shipNext != 0)
        {
            for (i = 0; i < IS->is_world.w_shipNext; i++)
            {
                server(IS, rt_lockShip, i);
                IS->is_request.rq_u.ru_ship.sh_lastUpdate += delta;
                server(IS, rt_unlockShip, i);
            }
        }
        if (IS->is_world.w_planetNext != 0)
        {
            for (i = 0; i < IS->is_world.w_planetNext; i++)
            {
                server(IS, rt_lockPlanet, i);
                IS->is_request.rq_u.ru_planet.pl_lastUpdate += delta;
                server(IS, rt_unlockPlanet, i);
            }
        }
        if (IS->is_world.w_bigItemNext != 0)
        {
            for (i = 0; i < IS->is_world.w_bigItemNext; i++)
            {
                server(IS, rt_lockBigItem, i);
                IS->is_request.rq_u.ru_bigItem.bi_lastUpdate += delta;
                server(IS, rt_unlockBigItem, i);
            }
        }
        server(IS, rt_lockWorld, 0);
        IS->is_world.w_lastRun += delta;
        server(IS, rt_unlockWorld, 0);
        server(IS, rt_flush, 0);
    }
}

/*
 * cmd_log - allow local player to enable logging.
 */

void cmd_log(IMP)
{
    char *p;

    if (*IS->is_textInPos == '\0')
    {
        if (clLog(IS, NULL))
        {
            err(IS, "logging turned off");
        }
    }
    else
    {
        p = IS->is_textInPos;
        (void) skipWord(IS);
        if (clLog(IS, p))
        {
            err(IS, "logging turned on");
        }
        else
        {
            err(IS, "cannot turn logging on (only allowed for a local player)");
        }
    }
}

/*
 * impSleep - call the sleep routine to sleep a bit.
 */

void impSleep(IMP, const USHORT seconds)
{
    if (seconds != 0)
    {
        clSleep(IS, seconds);
    }
}

/*
 * uTime - add a given time value to the output.
 */

void uTime(IMP, ULONG t)
{
    char buffer[30];

    strcpy(&buffer[0], ctime(&t));
    /* eliminate the '\n' that ctime() adds */
    buffer[strlen(buffer) - 1] = '\0';
    user(IS, &buffer[0]);
}

/*
 * printFile - interface to the client's printFile routine.
 */

BOOL printFile(IMP, char *fileName, const FileType_t ft)
{
    register char *p, *q;
    register ULONG len;
    register char ch;
    register BOOL abort;
    int curLine, maxLine;
    BOOL doMore=TRUE;	/* TRUE if we should do "more" prompt */

    strcpy(&IS->is_request.rq_u.ru_text[0], fileName);
    switch(ft)
    {
        case ft_normal:
            server(IS, rt_readFile, 0);
            break;
        case ft_help:
            server(IS, rt_readHelp, 0);
            break;
        case ft_doc:
            server(IS, rt_readDoc, 0);
            break;
    }
    if (IS->is_request.rq_whichUnit == 0)
    {
        return FALSE;
    }
    abort = FALSE;
    q = &IS->is_textOut[0];
    maxLine = IS->is_player.p_conLength - 2;
    curLine = maxLine;
    if (maxLine < 1)
    {
	doMore = FALSE;
    }
    while ((len = IS->is_request.rq_whichUnit) != 0)
    {
        if (!abort)
        {
            p = &IS->is_request.rq_u.ru_text[0];
            while ((!abort) && (len != 0))
            {
                do
                {
                    len--;
                    ch = *p;
                    p++;
                    *q = ch;
                    q++;
                }
                while ((ch != '\n') && (len != 0));
                if (ch == '\n')
                {
                    IS->is_textOutPos = q - &IS->is_textOut[0];
                    clWriteUser(IS);
                    q = &IS->is_textOut[0];
                    abort = clGotCtrlC(IS);
		    curLine--;
		    if (!abort && doMore && (curLine == 0))
		    {
IS->is_textOutPos = 0;
			uPrompt(IS, " -- more -- ");	
			uFlush(IS);
			IS->is_textInPos = &IS->is_textIn[0];
			*IS->is_textInPos = '\0';
			if (clReadUser(IS))
			{
			    (void) skipBlanks(IS);
			    if ((*IS->is_textInPos == 'q') ||
				(*IS->is_textInPos == 'Q') ||
				(*IS->is_textInPos == 'n') ||
				(*IS->is_textInPos == 'N'))
			    {
				abort = TRUE;
			    }
			    else if ((*IS->is_textInPos == 'c') ||
				(*IS->is_textInPos == 'C'))
			    {
				doMore = FALSE;
			    }
			}
			curLine = maxLine;
		    }
                }
            }
        }
        server(IS, rt_moreFile, 0);
    }
    IS->is_textOutPos = 0;
    return TRUE;
}

/*
 * news - generate an item of news.
 */

void news(IMP, register const NewsType_t verb, register const USHORT actor,
    register const USHORT victim)
{
    IS->is_request.rq_u.ru_news.n_verb = verb;
    IS->is_request.rq_u.ru_news.n_actor = actor;
    IS->is_request.rq_u.ru_news.n_victim = victim;
    server(IS, rt_news, 0);
}

/*
 * getPassword - prompt for and check a password.
 */

BOOL getPassword(IMP, register char *prompt, register char *password)
{
    BOOL ok;

    if (*password == '\0')
    {
        return TRUE;
    }
    uPrompt(IS, prompt);
    clEchoOff(IS);
    ok = clReadUser(IS);
    clEchoOn(IS);
    if (ok)
    {
        if (strcmp(&IS->is_textIn[0], password) == 0)
        {
            return TRUE;
        }
        user(IS, "Password incorrect. - "); /* note NO newline */
        return FALSE;
    }
    return FALSE;
}

/*
 * getPlayerPassword - get and check the player's password.
 */

BOOL getPlayerPassword(IMP)
{
    return getPassword(IS, ":Enter player password",
        &IS->is_player.p_password[0]);
}

/*
 * newPlayerPassword - get a new password for the player. Verify it.
 *      return 'TRUE' if it is verified, and install into the player.
 */

BOOL newPlayerPassword(IMP)
{
    char password[PASSWORD_LEN];
    BOOL ok;

    uPrompt(IS, "Enter new player password");
    clEchoOff(IS);
    ok = clReadUser(IS);
    clEchoOn(IS);
    if (ok)
    {
        memcpy(&password[0], &IS->is_textIn[0], PASSWORD_LEN);
        uPrompt(IS, "Re-enter password to verify");
        clEchoOff(IS);
        ok = clReadUser(IS);
        clEchoOn(IS);
        if (ok)
        {
            if (strcmp(&password[0], &IS->is_textIn[0]) == 0)
            {
                memcpy(&IS->is_player.p_password[0], &password[0],
                    PASSWORD_LEN);
                return TRUE;
            }
            user(IS, "Password not verified.\n");
            return FALSE;
        }
        return FALSE;
    }
    return FALSE;
}

/*
 * newPlayerEmail - get a new email address for the player. Verify it.
 *      return 'TRUE' if it is verified, and install into the player.
 */

BOOL newPlayerEmail(IMP)
{
    BOOL ok;

    uPrompt(IS, "Enter new email address");
    ok = clReadUser(IS);
    if (ok)
    {
            memcpy(&IS->is_player.p_email[0], &IS->is_textIn[0], EMAIL_LEN - 1);
            return TRUE;
     }
     return FALSE;
}

/*
 * updateTimer - updates connect time limit, returns 'FALSE' if we should quit
 */

BOOL updateTimer(IMP)
{
    ULONG now, dt;

    now = IS->is_request.rq_time;
    if (IS->is_player.p_status == ps_deity)
    {
        return FALSE;
    }
    if (IS->is_player.p_lastOn / (24 * 60 * 60) != now / (24 * 60 * 60))
    {
        /* it's now the next day - reset timer */
        server(IS, rt_lockPlayer, IS->is_player.p_number);
        IS->is_request.rq_u.ru_player.p_lastOn = now;
        if (IS->is_player.p_status == ps_visitor)
        {
            IS->is_request.rq_u.ru_player.p_timeLeft = 10;
        }
        else
        {
            IS->is_request.rq_u.ru_player.p_timeLeft =
                IS->is_world.w_maxConnect;
        }
        IS->is_player = IS->is_request.rq_u.ru_player;
        server(IS, rt_unlockPlayer, IS->is_player.p_number);
        return FALSE;
    }
    if (IS->is_player.p_timeLeft < (now - IS->is_player.p_lastOn) / 60)
    {
        /* he's been here too long! - he should go away */
        server(IS, rt_lockPlayer, IS->is_player.p_number);
        IS->is_request.rq_u.ru_player.p_timeLeft = 0;
        IS->is_player = IS->is_request.rq_u.ru_player;
        server(IS, rt_unlockPlayer, IS->is_player.p_number);
        return TRUE;
    }
    dt = (now - IS->is_player.p_lastOn) / 60;
    if (dt >= 1)
    {
        server(IS, rt_lockPlayer, IS->is_player.p_number);
        IS->is_request.rq_u.ru_player.p_timeLeft =
        IS->is_request.rq_u.ru_player.p_timeLeft - dt;
        IS->is_request.rq_u.ru_player.p_lastOn =
        IS->is_request.rq_u.ru_player.p_lastOn + dt * 60;
        IS->is_player = IS->is_request.rq_u.ru_player;
        server(IS, rt_unlockPlayer, IS->is_player.p_number);
    }
    return FALSE;
}

/*
 * resetTimer - Resets the timer when you enter the program if you haven't
 *      been in the program since 12 midnight. Also recalculate BTUS.
 *      Returns 'TRUE' if you are out of time for the day.
 */

BOOL resetTimer(IMP)
{
    ULONG now;
    BOOL timedOut;

    /* special handling for utility programs */
    if (IS->is_player.p_number == NO_OWNER)
    {
        server(IS, rt_lockPlayer, 0);
        server(IS, rt_unlockPlayer, 0);
        now = IS->is_request.rq_time;
        IS->is_player.p_lastOn = now;
        IS->is_player.p_timeLeft = 9999;
        return FALSE;
    }

    /* is the current player a deity */
    if (IS->is_player.p_status == ps_deity)
    {
        server(IS, rt_lockPlayer, IS->is_player.p_number);
        now = IS->is_request.rq_time;
        IS->is_request.rq_u.ru_player.p_timeLeft = 9999;
        timedOut = FALSE;
    }
    else
    {
        /* Check to see if we changed days since the last access */
        server(IS, rt_lockPlayer, IS->is_player.p_number);
        now = IS->is_request.rq_time;
        if (IS->is_request.rq_u.ru_player.p_lastOn / (24 * 60 * 60) !=
            now / (24 * 60 * 60))
        {
            if (IS->is_player.p_status == ps_visitor)
            {
                IS->is_request.rq_u.ru_player.p_timeLeft = 10;
            }
            else
            {
                IS->is_request.rq_u.ru_player.p_timeLeft =
                   IS->is_world.w_maxConnect;
            }
        }
        if (IS->is_request.rq_u.ru_player.p_timeLeft == 0)
        {
            timedOut = TRUE;
        }
        else
        {
            timedOut = FALSE;
        }
    }
    IS->is_request.rq_u.ru_player.p_lastOn = now;
    server(IS, rt_unlockPlayer, IS->is_player.p_number);
    IS->is_player = IS->is_request.rq_u.ru_player;
    return timedOut;
}

/*
 * cmd_version - displays the current version of the library and other
 *          blurb info
 */

void cmd_version(IMP)
{
    user(IS, "This program is free software; you can redistribute it and/or "
        "modify\n");
    user(IS, "it under the terms of the GNU General Public License as "
        "published by\n");
    user(IS, "the Free Software Foundation; either version 1, or (at your "
        "option)\n");
    user(IS, "any later version.\n\n");
    user(IS, "This program is distributed in the hope that it will be "
        "useful,\n");
    user(IS, "but WITHOUT ANY WARRANTY; without even the implied warranty "
        "of\n");
    user(IS, "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See "
        "the\n");
    user(IS, "GNU General Public License for more details.\n\n");
    user(IS, "By running this software you are agreeing to furnish the "
        "complete\n");
    user(IS, "package as you received it to anyone who asks, for a "
        "reasonable handling\n");
    user(IS, "fee. See the GPL for more information.\n\n");
    user2(IS, "Imperium ", IMP_BASE_REV);
    userN3(IS, "pl", 0, "\n");
    user(IS, "Compiled options:\n");
#ifdef DEBUG_LIB
    user(IS, "    Debug Enabled\n");
#endif
#ifdef HAVE_CRYPT
    user(IS, "    Client authentication\n");
#endif
}

/*
 * giveWhy - tells why the Imperium server is not accepting connections
 */

void giveWhy(IMP)
{
    register ULONG flags;

    /* save some time */
    flags = IS->is_request.rq_specialFlags;

    if (flags & (ISF_POW_CRIT | ISF_POW_WARN))
    {
        user(IS, "The Imperium server is presently experiencing a ");
        if (flags & ISF_POW_CRIT)
        {
            user(IS, "SERIOUS ");
        }
        user(IS, "AC power problem.\n");
    }
    else if (flags & ISF_BACKUP)
    {
        user(IS, "The Imperium server is presently in the process of"
            " doing a file backup.\n");
    }
    else
    {
        user(IS, "The Imperium server is not currently accepting new "
            "connections.\n");
    }
    user(IS, "Please try again later!\n");
}

/*
 * ImpCntrl - Entry point for a non-game playing related (utility) program
 *          NOTE: is_argShort is assumed to contain the desired operation!
 *                is_argBool will determine logging of actions
 */

void ImpCntrl(IMP)
{
    register Player_t *rp;
    register ULONG curPlan;
    ULONG lastOn;
    BOOL doLog;

    rp = &IS->is_request.rq_u.ru_player;
    doLog = IS->is_argBool;

    /* check the server status */
    if ((IS->is_request.rq_specialFlags != ISF_NONE) &&
        ((IS->is_argShort != IC_RESET) && (IS->is_argShort != IC_BACKE) &&
        (IS->is_argShort != IC_DECUSR)))
    {
        giveWhy(IS);
        return;
    }

    /* see what they want us to do */
    switch(IS->is_argShort)
    {
        case IC_INCUSR:
            if (doLog)
            {
                log3(IS, "Control utility incrementing NumPlay", "", "");
            }
            server(IS, rt_setPlayer, NO_OWNER + 1);
            break;
        case IC_DECUSR:
            if (doLog)
            {
                log3(IS, "Control utility decrementing NumPlay", "", "");
            }
            server(IS, rt_setPlayer, NO_OWNER + 2);
            break;
        case IC_BACKS:
            if (doLog)
            {
                log3(IS, "Control utility requesting a backup lock", "", "");
            }
            /* note this will not return until all clients are logged out */
            server(IS, rt_backStart, 0);
            if (doLog)
            {
                log3(IS, "Backup lock obtained", "", "");
            }
            break;
        case IC_BACKE:
            if (doLog)
            {
                log3(IS, "Control utility releasing backup lock", "", "");
            }
            server(IS, rt_backDone, 0);
            break;
        case IC_FPOWER:
            if (doLog)
            {
                log3(IS, "Control utility updating power report", "", "");
            }
            IS->is_textInPos = &IS->is_textIn[0];
            strcpy(IS->is_textInPos, "force");
            cmd_power(IS);
            if (doLog)
            {
                log3(IS, "Power report update complete", "", "");
            }
            break;
        case IC_POWER:
            if (doLog)
            {
                log3(IS, "Control utility displaying/updating power report",
                    "", "");
            }
            IS->is_textInPos = &IS->is_textIn[0];
            IS->is_textIn[0] = '\0';
            IS->is_textIn[1] = '\0';
            cmd_power(IS);
            if (doLog)
            {
                log3(IS, "Power report display/update complete", "", "");
            }
            break;
        case IC_MINER:
            if (doLog)
            {
                log3(IS, "Control utility updating all miners", "", "");
            }
            IS->is_textInPos = &IS->is_textIn[0];
            strcpy(IS->is_textInPos, "update");
            cmd_miner(IS);
            if (doLog)
            {
                log3(IS, "Miner update complete", "", "");
            }
            break;
        case IC_UPDATE:
            if (doLog)
            {
                log3(IS, "Control utility updating all planets", "", "");
            }
            for (curPlan = 0; curPlan < IS->is_world.w_planetNext; curPlan++)
            {
                accessPlanet(IS, curPlan);
            }
            if (doLog)
            {
                log3(IS, "Planet update complete", "", "");
            }
            break;
        case IC_DOFLUSH:
            if (doLog)
            {
                log3(IS, "Control utility flushing server buffers", "", "");
            }
            server(IS, rt_flush, 0);
            if (doLog)
            {
                log3(IS, "Server buffers flushed to disk", "", "");
            }
            break;
        default:
            server(IS, rt_readWorld, 0);
            IS->is_world = IS->is_request.rq_u.ru_world;
            /* reset all options to sane values */
            IS->is_textOutPos = 0;
            IS->is_noWrite = FALSE;
            strcpy(IS->is_sectorChar, &SECTOR_CHAR[0]);
            strcpy(IS->is_shipChar, &SHIP_CHAR[0]);
            strcpy(IS->is_itemChar, &ITEM_CHAR[0]);
            /* initialize the upper 32 bits of the random number "seed" */
            (void) time(&lastOn);
            memcpy(&IS->is_seed[0], (USHORT *)&lastOn, 2 * sizeof(USHORT));
            /* now set the lower 16 bits to the same value as SAS uses */
            IS->is_seed[2] = 0x330E;
            IS->is_contractEarnings = 0;
            IS->is_interestEarnings = 0;
            IS->is_improvementCost = 0;
            IS->is_militaryCost = 0;
            IS->is_utilitiesCost = 0;
            IS->is_noWrite = FALSE;
            IS->is_quietUpdate = TRUE;
            IS->is_verboseUpdate = FALSE;
            /* read in the deity player */
            server(IS, rt_readPlayer, 0);
            IS->is_player = *rp;
            /* erase any fields we don't need */
            IS->is_player.p_telegramsNew = 0;
            IS->is_player.p_telegramsTail = 0;
            IS->is_player.p_planetCount = 0;
            IS->is_player.p_money = 0;
            IS->is_player.p_btu = 96;
            IS->is_player.p_number = NO_OWNER;
            IS->is_player.p_race = NO_RACE;
            strcpy(&IS->is_player.p_name[0], "System Utility");
            strcpy(&IS->is_player.p_password[0], "foobar");
            strcpy(&IS->is_player.p_email[0], "");
            IS->is_player.p_loggedOn = TRUE;
            IS->is_player.p_inChat = FALSE;
            IS->is_player.p_compressed = FALSE;
            IS->is_player.p_doingPower = FALSE;
            IS->is_player.p_feMode = 0;
            IS->is_player.p_sendEmail = 0;
            IS->is_player.p_newPlayer = FALSE;
            IS->is_player.p_notify = nt_message;
            server(IS, rt_setPlayer, IS->is_player.p_number);
            (void) resetTimer(IS);
            break;
    }
    return;
}

/*
 * playNameEqu - checks to see if a name is equal to the name of a
 * particular player
 */

BOOL playNameEqu(IMP, USHORT playNum, char *name)
{
    register Player_t *rp;

    rp = &IS->is_request.rq_u.ru_player;
    server(IS, rt_readPlayer, playNum);
    if (strcmp(&rp->p_name[0], name) == 0)
    {
        return TRUE;
    }
    return FALSE;
}

/*
 * playStatusEqu - checks to see if a players status is equal to the passed
 *          value.
 *          Note: The buffer will be cleared!
 */

BOOL playStatusEqu(IMP, USHORT playNum, PlayerStatus_t status)
{
    register Player_t *rp;

    rp = &IS->is_request.rq_u.ru_player;
    server(IS, rt_readPlayer, playNum);
    if (rp->p_status == status)
    {
        return TRUE;
    }
    return FALSE;
}

/*
 * showStat - displays the stats for the given race
 */

void showStat(IMP, USHORT race)
{
    char tempBuf[81];
    char *ptr = "*ERROR";

    /* Set the status text */
    switch(IS->is_world.w_race[race].r_status)
    {
        case rs_notyet:
            ptr = "INACT ";
            break;
        case rs_active:
            ptr = "ACTIVE";
            break;
        case rs_dead:
            ptr = "DEAD  ";
            break;
    }
    /* fill in the buffer with the finished line */
    (void) sprintf(&tempBuf[0],
        "| %2.2u | %-31.31s | %8.8u | %9.9u | %-6.6s |\n", race,
        &IS->is_world.w_race[race].r_name[0],
        IS->is_world.w_race[race].r_techLevel,
        IS->is_world.w_race[race].r_planetCount, ptr);
    /* send the line to the user */
    feRaceRep(IS, 1);
    user(IS, &tempBuf[0]);
    /* fill in the buffer with the finished line */
    (void) sprintf(&tempBuf[0],
        "|    | %-15.15s                 | %8.8u | %9.9u |        |\n",
        &IS->is_world.w_race[race].r_homeName[0],
        IS->is_world.w_race[race].r_resLevel,
        IS->is_world.w_race[race].r_playCount);
    /* send the line to the user */
    feRaceRep(IS, 2);
    user(IS, &tempBuf[0]);
    /* send the closing line */
    user(IS,
"|----------------------------------------------------------------------|\n");
}

/*
 * selectRace - displays some stats about each race, and allow the user to
 *          pick one of the races. Returns true if the user selected one,
 *          or false if they did not or there was an error
 *          Note: The buffer will be cleared
 */

BOOL selectRace(IMP, USHORT *pRace)
{
    register USHORT i;

    user(IS, "Please choose a race. Following are the stats of each "
        "race as\n");
    user(IS, "of this time:\n\n");
    user(IS,
" ______________________________________________________________________\n"
"| ## | Race Name                       | Tech Lev | # Planets | Status |\n"
"|    | Home Planet Name                | Res Lev  | # Players |        |\n"
"|======================================================================|\n");
    /* loop through the races, printing them out in the above format */
    for (i = 0; i < RACE_MAX; i++)
    {
        showStat(IS, i);
    }
    /* ask them to pick a race */
    uPrompt(IS, "Join which race");
    if (clReadUser(IS))
    {
        return reqRace(IS, pRace, FALSE, "Join which race");
    }
    else
    {
        return FALSE;
    }
}

/*
 * makeUserShip - create a complete ship for the player on the players
 *          home planet.
 *          Note: Since this needs to create many items, it locks/unlocks
 *          the world before each item is created, to allow for the most
 *          carefull creation of object numbers. Since the world struct is
 *          kept in core by the server anyway, this should not cause
 *          significant disk overhead
 */

void makeUserShip(IMP)
{
    register ULONG itNum;
    register BigItem_t *rbi;
    register Ship_t *rsh;
    register USHORT bp, curIt, maxNum;
    ULONG *numPtr = NULL;
    Ship_t ship;

    user(IS, "Assembling your initial spacecraft on your home planet...\n");

    rbi = &IS->is_request.rq_u.ru_bigItem;
    rsh = &IS->is_request.rq_u.ru_ship;
    server(IS, rt_lockWorld, 0);
    itNum = IS->is_request.rq_u.ru_world.w_bigItemNext;
    IS->is_request.rq_u.ru_world.w_bigItemNext++;
    /* we store ship number here just temporarily */
    ship.sh_number = IS->is_request.rq_u.ru_world.w_shipNext;
    IS->is_world = IS->is_request.rq_u.ru_world;
    server(IS, rt_unlockWorld, 0);
    ship.sh_hull = itNum;
    userN3(IS, "Creating ship #", ship.sh_number, ":\n");

    /* create the ship's hull */
    rbi->bi_lastUpdate = IS->is_player.p_lastOn;
    rbi->bi_itemLoc = ship.sh_number;
    rbi->bi_techLevel = IS->is_world.w_race[IS->is_player.p_race].r_techLevel;
    rbi->bi_number = itNum;
    rbi->bi_part = bp_hull;
    rbi->bi_weight = 0;
    rbi->bi_price = 0;
    rbi->bi_status = bi_inuse;
    rbi->bi_onShip = TRUE;
    rbi->bi_effic = 50;
    server(IS, rt_createBigItem, itNum);

    /* now create the ship itself */
    server(IS, rt_lockWorld, 0);
    if (ship.sh_number != IS->is_request.rq_u.ru_world.w_shipNext)
    {
        ship.sh_number = IS->is_request.rq_u.ru_world.w_shipNext;
        IS->is_request.rq_u.ru_world.w_shipNext++;
        server(IS, rt_unlockWorld, 0);
        userN3(IS, "Someone else just created that ship, you can have #",
            ship.sh_number, ".\n");

        /* they differ, so we need to update the hull as well */
        server(IS, rt_lockBigItem, itNum);
        rbi->bi_itemLoc = ship.sh_number;
        server(IS, rt_unlockBigItem, itNum);
    }
    else
    {
        IS->is_request.rq_u.ru_world.w_shipNext++;
        IS->is_world = IS->is_request.rq_u.ru_world;
        server(IS, rt_unlockWorld, 0);
    }
    IS->is_world.w_shipNext = IS->is_request.rq_u.ru_world.w_shipNext;
    incrShipCount(IS, mapSector(IS, IS->is_world.w_race[
        IS->is_player.p_race].r_homeRow, IS->is_world.w_race[
        IS->is_player.p_race].r_homeCol));
    incrPlShipCount(IS, IS->is_world.w_race[IS->is_player.p_race
        ].r_homePlanet);
    memset(rsh, '\0', sizeof(Ship_t) * sizeof(char));
    rsh->sh_number = ship.sh_number;
    rsh->sh_planet = IS->is_world.w_race[IS->is_player.p_race].r_homePlanet;
    rsh->sh_lastUpdate = timeNow(IS);
    rsh->sh_price = 0;
    rsh->sh_fuelLeft = 0;
    rsh->sh_cargo = 0;
    rsh->sh_armourLeft = 0;
    rsh->sh_airLeft = 127;
    rsh->sh_row = IS->is_world.w_race[IS->is_player.p_race].r_homeRow;
    rsh->sh_col = IS->is_world.w_race[IS->is_player.p_race].r_homeCol;
    rsh->sh_type = st_a;
    rsh->sh_fleet = '*';
    rsh->sh_hullTF = getTechFactor(IS->is_world.w_race[
        IS->is_player.p_race].r_techLevel);
    rsh->sh_engTF = rsh->sh_hullTF;
    rsh->sh_engEff = 50;
    rsh->sh_efficiency = 50;
    rsh->sh_owner = IS->is_player.p_number;
    rsh->sh_plagueStage = 0;
    rsh->sh_plagueTime = 0;
    rsh->sh_hull = itNum;
    rsh->sh_flags = SHF_NONE;
    rsh->sh_dragger = NO_ITEM;

    /* fill in the default starting values for small items */
    rsh->sh_items[it_civilians] = 5;
    rsh->sh_items[it_scientists] = 2;
    rsh->sh_items[it_military] = 5;
    rsh->sh_items[it_officers] = 5;
    rsh->sh_items[it_fuelTanks] = 2;

    /* now build up the current cargo weight */
    for (curIt = IT_FIRST; curIt <= IT_LAST_SMALL; curIt++)
    {
        rsh->sh_cargo += IS->is_world.w_weight[curIt] *
            rsh->sh_items[curIt];
    }

    /* set the default amount of fuel on board */
    rsh->sh_fuelLeft = 75;
    rsh->sh_cargo += 150;

    /* and the amount of already-converted energy */
    rsh->sh_energy = 75;

    /* loop through the items setting them to the NO_ITEM state */
    for (bp = BP_FIRST; bp <= BP_LAST; bp++)
    {
        switch(bp)
        {
            case bp_computer:
                maxNum = MAX_COMP;
                numPtr = &rsh->sh_computer[0];
                break;
            case bp_engines:
                maxNum = MAX_ENG;
                numPtr = &rsh->sh_engine[0];
                break;
            case bp_lifeSupp:
                maxNum = MAX_LIFESUP;
                numPtr = &rsh->sh_lifeSupp[0];
                break;
            case bp_sensors:
                /* note that we only specify one of the modules */
                maxNum = MAX_ELECT;
                numPtr = &rsh->sh_elect[0];
                break;
            case bp_photon:
                /* note that we only specify one of the weapons */
                maxNum = MAX_WEAP;
                numPtr = &rsh->sh_weapon[0];
                break;
            default:
                maxNum = 0;
                break;
        }
        if (maxNum)
        {
            for (curIt = 0; curIt < maxNum; curIt++)
            {
                numPtr[curIt] = NO_ITEM;
            }
        }
    }
    ship = *rsh;
    server(IS, rt_createShip, ship.sh_number);

    /* create the ship's engine */
    server(IS, rt_lockWorld, 0);
    itNum = IS->is_request.rq_u.ru_world.w_bigItemNext;
    IS->is_request.rq_u.ru_world.w_bigItemNext++;
    IS->is_world = IS->is_request.rq_u.ru_world;
    server(IS, rt_unlockWorld, 0);
    rbi->bi_lastUpdate = IS->is_player.p_lastOn;
    rbi->bi_itemLoc = ship.sh_number;
    rbi->bi_techLevel = IS->is_world.w_race[IS->is_player.p_race].r_techLevel;
    rbi->bi_number = itNum;
    rbi->bi_part = bp_engines;
    rbi->bi_status = bi_inuse;
    rbi->bi_onShip = TRUE;
    rbi->bi_effic = 50;
    rbi->bi_price = 0;
    /* get the items weight */
    rbi->bi_weight = buildItemWeight(IS, rbi);
    /* store the weight for use in the ship */
    bp = rbi->bi_weight;
    server(IS, rt_createBigItem, itNum);
    server(IS, rt_lockShip, ship.sh_number);
    rsh->sh_items[it_engines]++;
    rsh->sh_engine[0] = itNum;
    rsh->sh_cargo += bp;
    server(IS, rt_unlockShip, ship.sh_number);
    userN2(IS, "You will receive engine #", itNum);

    /* create the ship's life support system */
    server(IS, rt_lockWorld, 0);
    itNum = IS->is_request.rq_u.ru_world.w_bigItemNext;
    IS->is_request.rq_u.ru_world.w_bigItemNext++;
    IS->is_world = IS->is_request.rq_u.ru_world;
    server(IS, rt_unlockWorld, 0);
    rbi->bi_lastUpdate = IS->is_player.p_lastOn;
    rbi->bi_itemLoc = ship.sh_number;
    rbi->bi_techLevel = IS->is_world.w_race[IS->is_player.p_race].r_techLevel;
    rbi->bi_number = itNum;
    rbi->bi_part = bp_lifeSupp;
    rbi->bi_status = bi_inuse;
    rbi->bi_onShip = TRUE;
    rbi->bi_effic = 50;
    rbi->bi_price = 0;
    rbi->bi_weight = buildItemWeight(IS, rbi);
    bp = rbi->bi_weight;
    server(IS, rt_createBigItem, itNum);
    server(IS, rt_lockShip, ship.sh_number);
    rsh->sh_items[it_lifeSupp]++;
    rsh->sh_lifeSupp[0] = itNum;
    rsh->sh_cargo += bp;
    server(IS, rt_unlockShip, ship.sh_number);
    userN3(IS, ", life-support system #", itNum, ",\n");

    /* create the ship's computer */
    server(IS, rt_lockWorld, 0);
    itNum = IS->is_request.rq_u.ru_world.w_bigItemNext;
    IS->is_request.rq_u.ru_world.w_bigItemNext++;
    IS->is_world = IS->is_request.rq_u.ru_world;
    server(IS, rt_unlockWorld, 0);
    rbi->bi_lastUpdate = IS->is_player.p_lastOn;
    rbi->bi_itemLoc = ship.sh_number;
    rbi->bi_techLevel = IS->is_world.w_race[IS->is_player.p_race].r_techLevel;
    rbi->bi_number = itNum;
    rbi->bi_part = bp_computer;
    rbi->bi_status = bi_inuse;
    rbi->bi_onShip = TRUE;
    rbi->bi_effic = 50;
    rbi->bi_price = 0;
    rbi->bi_weight = buildItemWeight(IS, rbi);
    bp = rbi->bi_weight;
    server(IS, rt_createBigItem, itNum);
    server(IS, rt_lockShip, ship.sh_number);
    rsh->sh_items[it_computers]++;
    rsh->sh_computer[0] = itNum;
    rsh->sh_cargo += bp;
    server(IS, rt_unlockShip, ship.sh_number);
    userN2(IS, "computer #", itNum);

    /* create the ship's sensors */
    server(IS, rt_lockWorld, 0);
    itNum = IS->is_request.rq_u.ru_world.w_bigItemNext;
    /* set our local world's item count from this last big item */
    IS->is_request.rq_u.ru_world.w_bigItemNext++;
    IS->is_world = IS->is_request.rq_u.ru_world;
    server(IS, rt_unlockWorld, 0);
    IS->is_world.w_bigItemNext = IS->is_request.rq_u.ru_world.w_bigItemNext;
    rbi->bi_lastUpdate = IS->is_player.p_lastOn;
    rbi->bi_itemLoc = ship.sh_number;
    rbi->bi_techLevel = IS->is_world.w_race[IS->is_player.p_race].r_techLevel;
    rbi->bi_number = itNum;
    rbi->bi_part = bp_sensors;
    rbi->bi_status = bi_inuse;
    rbi->bi_onShip = TRUE;
    rbi->bi_effic = 50;
    rbi->bi_price = 0;
    rbi->bi_weight = buildItemWeight(IS, rbi);
    bp = rbi->bi_weight;
    server(IS, rt_createBigItem, itNum);
    server(IS, rt_lockShip, ship.sh_number);
    rsh->sh_items[it_elect]++;
    rsh->sh_elect[0] = itNum;
    rsh->sh_cargo += bp;
    server(IS, rt_unlockShip, ship.sh_number);
    userN3(IS, ", and sensor array #", itNum, ".\n");
    user(IS, "You can display your initial ship with the"
        " \"ship\" command.\n");
}

/*
 * Imperium - the entry point to the Imperium library.
 */

void Imperium(IMP)
{
    register World_t *w, *rw;
    Player_t *p, *rp;
    char *name = "", *q;
    register USHORT i;
    ULONG now, lastOn, deltaTime;
    USHORT wantRace, passedTime;
    BOOL aborting, makeNew, usingFE;

    IS->is_textOutPos = 0;
    IS->is_noWrite = FALSE;
    strcpy(IS->is_sectorChar, &SECTOR_CHAR[0]);
    strcpy(IS->is_shipChar, &SHIP_CHAR[0]);
    strcpy(IS->is_itemChar, &ITEM_CHAR[0]);
    p = &IS->is_player;
    rp = &IS->is_request.rq_u.ru_player;
    w = &IS->is_world;
    rw = &IS->is_request.rq_u.ru_world;
    (void) printFile(IS, CONNECT_MESSAGE_FILE, ft_normal);
    user2(IS, "Running Imperium ", IMP_BASE_REV);
    userN3(IS, "pl", 0, "\n\n");

    /* check the server status */
    if (IS->is_request.rq_specialFlags != ISF_NONE)
    {
        giveWhy(IS);
        return;
    }
    aborting = FALSE;
    usingFE = FALSE;
    passedTime = IS->is_argShort;

    /* initialize the upper 32 bits of the random number "seed" */
    (void) time(&lastOn);
    memcpy(&IS->is_seed[0], (USHORT *)&lastOn, 2 * sizeof(USHORT));
    /* now set the lower 16 bits to the same value as SAS uses */
    IS->is_seed[2] = 0x330E;

    /* check if they passed us a player name at startup */
    if (IS->is_textIn[0] == '\0')
    {
        uPrompt(IS, ":Enter player name");
        if (clReadUser(IS))
        {
            /* trim leading space */
            name = skipBlanks(IS);
            if (*name == '!')
            {
                usingFE = TRUE;
                name += sizeof(char);
            }
            if (*name == '\0')
            {
                aborting = TRUE;
                log3(IS, "*** Empty player name entered.", "", "");
            }
        }
        else
        {
            aborting = TRUE;
            log3(IS, "*** Error when reading player name.", "", "");
        }
    }
    else
    {
        /* They aparently did pass us a player name */
        /* trim leading space */
        IS->is_textInPos = &IS->is_textIn[0];
        name = skipBlanks(IS);
        if (*name == '!')
        {
            usingFE = TRUE;
            name += sizeof(char);
        }
        if (*name == '\0')
        {
            aborting = TRUE;
            log3(IS, "*** Empty player name passed at startup.", "", "");
        }
    }
    if (!aborting)
    {
        /* truncate the entered name to what we allow */
        q = name;
        while (*q != '\0')
        {
            q++;
        }
        if ((q - name) / sizeof(char) >= NAME_LEN)
        {
            name[NAME_LEN - 1] = '\0';
        }
        server(IS, rt_readWorld, 0);
        *w = *rw;
        i = 0;
        while ((i != w->w_currPlayers) && !playNameEqu(IS, i, name))
        {
            i++;
        }
        if (i == w->w_currPlayers)
        {
            /* q points to the player name in the input buffer. That is
               very volatile, so copy the name to the player record, and
               make q point at the name there. */
            memcpy(&IS->is_player.p_name[0], name, NAME_LEN);
            name = &IS->is_player.p_name[0];
            user3(IS, "Player ", name, " does not exist.\n");
            if (i >= w->w_maxPlayers)
            {
                user(IS, "There is no space for a new player.\n");
                i = 0;
                while ((i < w->w_currPlayers) && !playStatusEqu(IS, i,
                    ps_idle))
                {
                   i++;
                }
                if (i == w->w_currPlayers)
                {
                   aborting = TRUE;
                   user(IS, "And there are no idle players.\n");
                   log3(IS, "*** Tried to create player ", name,
                       " when no space left.");
                   (void) printFile(IS, ACCESS_FILE, ft_normal);
                }
                else
                {
                    user(IS, "There is an idle player you may use.\n");
                    if (ask(IS, "Use it? "))
                    {
                       if (getPassword(IS,
                           "Enter creation password for this game",
                           &w->w_password[0]))
                       {
                            if (newPlayerPassword(IS))
                            {
                              /* be careful, someone else could have gone in
                                 and taken the next player while we are
                                 fiddling around */
                                server(IS, rt_lockPlayer, i);
                                if (rp->p_status != ps_idle)
                                {
                                   server(IS, rt_unlockPlayer, i);
                                   aborting = TRUE;
                                   user(IS, "Ooops, someone stole it out from "
                                     "under you!\nTry again!\n");
                                   log3(IS, "*** Player create clash, player ",
                                       name, "");
                                }
                                else
                                {
                                    memcpy(&rp->p_password[0], &p->p_password[0],
                                        PASSWORD_LEN);
                                    memcpy(&rp->p_name[0], &p->p_name[0],
                                        NAME_LEN);
                                    rp->p_status = ps_active;
                                    rp->p_lastOn = IS->is_request.rq_time;
                                    *p = *rp;
                                    server(IS, rt_unlockPlayer, i);
                                    (void) resetTimer(IS);
                                    log3(IS, "*** Player ", name, " created "
                                        "from an idle player.");
                                }
                            }
                            else
                            {
                               user(IS, "passwords did not match.\n");
                               aborting = TRUE;
                               log4(IS, "*** Unverified password, player ",
                                   name, ": ", &IS->is_textIn[0]);
                               (void) printFile(IS, ACCESS_FILE, ft_normal);
                            }
                        }
                        else
                        {
                           user(IS, "reconnect to try again.\n");
                           aborting = TRUE;
                           log4(IS, "*** Bad creation password for player ",
                               name, ": ", &IS->is_textIn[0]);
                           (void) printFile(IS, ACCESS_FILE, ft_normal);
                        }
                    }
                    else
                    {
                       aborting = TRUE;
                       user(IS, "Ok.\n");
                       log3(IS, "*** Player ", name, " refused to use an idle "
                           "player location.");
                       (void) printFile(IS, ACCESS_FILE, ft_normal);
                    }
                }
            }
            else
            {
                /* see if they want to create it */
                if (ask(IS, "Do you wish to create it? "))
                {
                    makeNew = FALSE;
                    if (IS->is_world.w_noCreatePass)
                    {
                        /* no creation password required */
                        makeNew = TRUE;
                    }
                    else
                    {
                        /* see if they know the creation password */
                        if (getPassword(IS,
                            "Enter creation password for this game",
                            &w->w_password[0]))
                        {
                            /* yup, they do */
                            makeNew = TRUE;
                        }
                    }
                    if (makeNew)
                    {
                        if (newPlayerPassword(IS))
                        {
                            if (selectRace(IS, &wantRace))
                            {
                        /* be careful, someone else could have gone in and
                           taken the next player while we are fiddling
                           around */
                                server(IS, rt_lockWorld, 0);
                                *w = *rw;
                                i = w->w_currPlayers;
                                if (i == w->w_maxPlayers)
                                {
                                    server(IS, rt_unlockWorld, 0);
                                    aborting = TRUE;
                                    user(IS, "Ooops, someone stole it out "
                                        "from under you!\n");
                                    log3(IS, "*** Player create clash, player ",
                                        name, "");
                                }
                                else
                                {
                                    w->w_currPlayers = i + 1;
                                    rw->w_currPlayers = i + 1;
                                    rw->w_race[wantRace].r_playCount++;
                                    if (rw->w_race[wantRace].r_status ==
                                        rs_notyet)
                                    {
                                        rw->w_race[wantRace].r_status =
                                            rs_active;
                                    }
                                    w->w_race[wantRace] = rw->w_race[wantRace];
                                    server(IS, rt_unlockWorld, 0);
                                    server(IS, rt_lockPlayer, i);
                                    /* we want the player name and password
                                    that we have set up here, but the rest of
                                    the record from what was set up by ImpCre
                                    */
                                    memcpy(&rp->p_password[0],
                                        &p->p_password[0], PASSWORD_LEN);
                                    memcpy(&rp->p_name[0], &p->p_name[0],
                                        NAME_LEN);
                                    rp->p_status = ps_active;
                                    rp->p_race = wantRace;
                                    rp->p_lastOn = IS->is_request.rq_time;
                                    *p = *rp;
                                    server(IS, rt_unlockPlayer, i);
                                    (void) resetTimer(IS);
                                    log3(IS, "*** Player ", name,
                                        " created.");
                                    makeUserShip(IS);
                                }
                            }
                            else
                            {
                                aborting = TRUE;
                                user(IS, "You can't play until you select "
                                    "a race\n");
                                log3(IS, "*** Player ", name,
                                    " refused a race.");
                            }
                        }
                        else
                        {
                            aborting = TRUE;
                            user(IS, "passwords did not match.\n");
                            log4(IS, "*** Unverified password, player ", name,
                                ": ", &IS->is_textIn[0]);
                            (void) printFile(IS, ACCESS_FILE, ft_normal);
                        }
                    }
                    else
                    {
                        user(IS, "reconnect to try again.\n");
                        aborting = TRUE;
                        log4(IS, "*** Bad creation password for player ",
                            name, ": ", &IS->is_textIn[0]);
                        (void) printFile(IS, ACCESS_FILE, ft_normal);
                    }
                }
                else
                {
                    user(IS, "OK.\n");
                    aborting = TRUE;
                    log3(IS, "*** Player ", name, " creation declined.");
                    (void) printFile(IS, ACCESS_FILE, ft_normal);
                }
            }
        }
        else
        {
            IS->is_player = *rp;
            name = &IS->is_player.p_name[0];
            if (!getPlayerPassword(IS))
            {
                log4(IS, "*** Bad password for ", name, ": ",
                    &IS->is_textIn[0]);
                user(IS, "try again.\n");
                if (!getPlayerPassword(IS))
                {
                    log4(IS, "*** Bad password for ", name, ": ",
                       &IS->is_textIn[0]);
                    user(IS, "try again.\n");
                    if (!getPlayerPassword(IS))
                    {
                        user(IS, "aborting\n");
                        log4(IS, "*** Player ", name, " denied - bad password: ",
                           &IS->is_textIn[0]);
                        aborting = TRUE;
                    }
                }
            }
            if (!aborting)
            {
                if (IS->is_player.p_loggedOn &&
                    (IS->is_player.p_status != ps_deity))
                {
                    user(IS,
                       "\n*** Warning!!! Your player was already marked"
                       " as logged on!!!\n\n");
                    aborting = TRUE;
                    log3(IS, "*** Player ", name, " is already logged on.");
                }
            }
        }
        if (!aborting)
        {
            lastOn = IS->is_player.p_lastOn;
            if (resetTimer(IS))
            {
                user(IS, "Sorry, you are out of time for today."
                     " Come back tomorrow.\n");
                log3(IS, "*** Player ", name, " denied - no time left.");
            }
            else
            {
                /* if they aren't a deity and the passed time is 0, log */
                /* out */
                if ((passedTime == 0) && (IS->is_player.p_status != ps_deity))
                {
                    user(IS, "Sorry, you are out of time for today.\n");
                    log3(IS, "*** Player ", name, " denied - no time left "
                        "due to passed argument.");
                }
                else
                {
                    server(IS, rt_lockPlayer, IS->is_player.p_number);
                    now = IS->is_request.rq_time;
                    deltaTime = (now - lastOn) / IS->is_world.w_secondsPerITU;
                    /* Check for an overflow of work */
                    if (deltaTime > MAX_WORK)
                    {
                        /* And cap it at MAX_WORK */
                        deltaTime = MAX_WORK;
                    }
                    /* update the players BTU's */
                    rp->p_btu = umin((USHORT) IS->is_world.w_maxBTUs, (USHORT) rp->p_btu +
                        deltaTime * 12700 / IS->is_world.w_BTUDivisor);
                    rp->p_loggedOn = TRUE;
                    if (usingFE)
                    {
                        rp->p_feMode = 0xFFFF;
                    }
                    else
                    {
                        rp->p_feMode = 0;
                    }
                    rp->p_lastOn = now;
                    /* if they aren't a deity, use the passed time */
                    if (IS->is_player.p_status != ps_deity)
                    {
                        rp->p_timeLeft = umin((USHORT) rp->p_timeLeft, (USHORT) passedTime);
                    }
                    server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    *p = *rp;
                    user2(IS, name, " on at ");
                    uTime(IS, now);
                    user(IS, " last on at ");
                    uTime(IS, lastOn);
                    userNL(IS);
                    log3(IS, "Player ", name, " logged on.");

                    /* get the players desired screen size */
                    IS->is_conWidth = p->p_conWidth;
                    IS->is_conLength = p->p_conLength;

                    (void) printFile(IS, LOGIN_MESSAGE_FILE, ft_normal);
                    server(IS, rt_setPlayer, IS->is_player.p_number);
                    telegramCheck(IS);
                    processCommands(IS);
                    server(IS, rt_lockPlayer, IS->is_player.p_number);
                    now = IS->is_request.rq_time;
                    rp->p_loggedOn = FALSE;
                    rp->p_feMode = 0;
                    rp->p_lastOn = now;
                    server(IS, rt_unlockPlayer, IS->is_player.p_number);
                    user2(IS, name, " off at ");
                    uTime(IS, now);
                    userNL(IS);
                    (void) printFile(IS, LOGOUT_MESSAGE_FILE, ft_normal);
                    server(IS, rt_lockWorld, 0);
                    IS->is_request.rq_u.ru_world.w_lastRun = now;
                    server(IS, rt_unlockWorld, 0);
                    log3(IS, "Player ", name, " logged off.");
                    server(IS, rt_setPlayer, NO_OWNER);
                }
            }
        }
    }
}
