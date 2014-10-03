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
 * $Id: messages.c,v 1.4 2000/05/26 22:55:19 marisa Exp $
 *
 * $Log: messages.c,v $
 * Revision 1.4  2000/05/26 22:55:19  marisa
 * Combat changes
 *
 * Revision 1.3  2000/05/25 18:21:23  marisa
 * Fix more T/F issues
 *
 * Revision 1.2  2000/05/18 06:50:02  marisa
 * More autoconf
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:30:57  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.3  1997/09/03 18:59:23  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.2  1997/03/14 07:24:32  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:03:22  marisag
 * patch20: Fix empty revision.
 *
 */

#include "../config.h"

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
#include <math.h>
#include "../Include/Imperium.h"
#include "../Include/Request.h"
#include "../Include/ImpFeMess.h"
#include "Scan.h"
#include "ImpPrivate.h"

static const char rcsid[] = "$Id: messages.c,v 1.4 2000/05/26 22:55:19 marisa Exp $";

#define tt_telegram     0
#define tt_message      1
#define tt_propaganda   2
typedef UBYTE TextType_t;

const UBYTE NEWS_PAGE[n_torp_ship + 1] = {
    2,  /* n_nothing    */
    1,  /* n_destroyed  */
    1,  /* n_won_planet */
    1,  /* n_lost_planet */
    2,  /* n_sent_telegram */
    3,  /* n_make_loan  */
    3,  /* n_repay_loan */
    3,  /* n_make_sale  */
    3,  /* n_grant_planet */
    1,  /* n_blase_ship */
    2,  /* n_took_unoccupied */
    1,  /* n_fire_back  */
    1,  /* n_bomb_planet */
    1,  /* n_bomb_dest */
    1,  /* n_board_ship */
    1,  /* n_failed_board */
    1,  /* n_flak       */
    1,  /* n_sieze_planet */
    1,  /* n_decl_ally  */
    1,  /* n_decl_neut  */
    1,  /* n_decl_war   */
    1,  /* n_disavow_ally */
    1,  /* n_disavow_war */
    2,  /* n_plague_outbreak */
    2,  /* n_plague_die */
    2,  /* n_plague_dest */
    1,  /* n_ship_dest  */
    1,  /* n_torp_dest  */
    2,  /* n_torp_dest  */
    2,  /* n_torp_dest  */
    2,  /* n_torp_dest  */
    1}; /* n_torp_ship  */

const BYTE NEWS_GOOD_WILL[n_torp_ship + 1] = {
    0,      /* n_nothing    */
    -5,     /* n_destroyed  */
    -3,     /* n_won_planet */
    -1,     /* n_lost_planet */
    1,      /* n_sent_telegram */
    5,      /* n_make_loan  */
    5,      /* n_repay_loan */
    4,      /* n_make_sale  */
    5,      /* n_grant_planet */
    -2,     /* n_blase_ship */
    0,      /* n_took_unoccupied */
    -2,     /* n_fire_back  */
    -4,     /* n_bomb_planet */
    -5,     /* n_bomb_dest */
    -2,     /* n_board_ship */
    -1,     /* n_failed_board */
    -1,     /* n_flak       */
    -3,     /* n_sieze_planet */
    5,      /* n_decl_ally  */
    0,      /* n_decl_neut  */
    -5,     /* n_decl_war   */
    -4,     /* n_disavow_ally */
    4,      /* n_disavow_war */
    0,      /* n_plague_outbreak */
    0,      /* n_plague_die */
    0,      /* n_plague_dest */
    -3,     /* n_ship_dest  */
    0,      /* n_torp_dest  */
    0,      /* n_torp_dest  */
    0,      /* n_torp_dest  */
    0,      /* n_torp_dest  */
    0};     /* n_torp_ship  */

#define LIST_SIZE   100 /* How many players to allow in a list */


/*
 * findString - part of 'getText' - given an input pointer and a string
 *      pointer, return the pointer to the string in the input, else NULL.
 */

char *findString(char *pos, char *sub)
{
    /* REPLACE ME */
    return strstr(pos, sub);
}


BOOL getMoreText(IMP, register USHORT len, register USHORT line)
{
    if (len == TELEGRAM_MAX - 1)
    {
        return FALSE;
    }
    userN(IS, line + 1);
    uPrompt(IS, "");
    if (clReadUser(IS))
    {
        if ((*IS->is_textInPos != '.') || (*(IS->is_textInPos + sizeof(char))
            != '\0'))
        {
            return TRUE;
        }
    }
    return FALSE;
}

char *getMoreText2(IMP, USHORT lineCount, char *what, BOOL *cancel)
{
    register char *inputPos;

    if (lineCount == 0)
    {
        user3(IS, "** empty ", what, " - cancelled\n");
        *cancel = TRUE;
        return NULL;
    }
    uPrompt(IS, "Send, List, Replace, Delete, Insert, Cancel");
    if (clReadUser(IS))
    {
        if (*IS->is_textInPos != '\0')
        {
            inputPos = skipBlanks(IS);
            if ((*inputPos == 'c') || (*inputPos == 'C'))
            {
                user3(IS, "** ", what, " cancelled\n");
                *cancel = TRUE;
                return NULL;
            }
            if ((*inputPos == 's') || (*inputPos == 'S'))
            {
                return NULL;
            }
            return inputPos;
        }
    }
    return NULL;
}

/*
 * getText - get a text item into the request structure.
 * Return 'FALSE' if aborting.
 */

BOOL getText(IMP, TextType_t tt)
{
    register char *pos, *inputPos;
    register USHORT len, line, n;
    char *BAD_REP = "Use is: replace line-# /oldtext/newtext/\n";
    USHORT MAXLEN = TELEGRAM_MAX - 1;
    char *what, *buf;
    char *fromString, *toString;
    USHORT lineCount, fromLen, toLen;
    char delim;
    BOOL cancel, newLine, needEdit;

    cancel = FALSE;
    switch(tt)
    {
        case tt_telegram:
            what = "telegram";
            break;
        case tt_message:
            what = "message";
            break;
        case tt_propaganda:
            what = "propaganda";
            break;
        default:
            what = "????";
            break;
    }
    user3(IS, "Please enter text of your ", what, ".\n");
    needEdit = TRUE;
    len = 0;
    line = 0;
    buf = &IS->is_request.rq_u.ru_telegram.te_data[0];
    pos = buf;
    if (tt == tt_message)
    {
        len = strlen(&IS->is_player.p_name[0]);
        memcpy(pos, &IS->is_player.p_name[0], len);
        pos += len * sizeof(char);
        *pos = ':';
        pos += sizeof(char);
        *pos = '\n';
        pos += sizeof(char);
        buf = pos;
        len += 2;
    }
    /* Attempt to use the external editor, if available */
    switch(clEdit(IS, buf, MAXLEN - (len + 1)))
    {
        case 1:
            /* the external edit worked */
            needEdit = FALSE;
            /* set the correct length */
            len += IS->is_argLong;
            break;
        case 3:
            break;
        case 0:
        case 2:
        default:
            needEdit = FALSE;
            cancel = TRUE;
    }
    /* See if the external editor call worked */
    if (needEdit)
    {
	/* Fall back to using the built-in editor */
        userN3(IS, "   (max ", MAXLEN, " chars). End with . :\n");
        while(getMoreText(IS, len, line))
        {
            inputPos = IS->is_textInPos;
            while ((*inputPos != '\0') && (len != MAXLEN))
            {
                *pos = *inputPos;
                pos += sizeof(char);
                inputPos += sizeof(char);
                len++;
            }
            if (len == MAXLEN)
            {
                *(pos - sizeof(char)) = '\n';
            }
            else
            {
                *pos = '\n';
                pos += sizeof(char);
                len++;
            }
            line++;
        }
        *pos = '\0';
        lineCount = line;
        while ((inputPos = getMoreText2(IS, lineCount, what, &cancel))
            != NULL)
        {
            switch(*inputPos)
            {
                case 'l':
                case 'L':
                    pos = buf;
                    line = 1;
                    newLine = TRUE;
                    while (*pos != '\0')
                    {
                        if (newLine)
                        {
                            newLine = FALSE;
                            userN(IS, line);
                            user(IS, ": ");
                        }
                        userC(IS, *pos);
                        if (*pos == '\n')
                        {
                            newLine = TRUE;
                            line++;
                        }
                        pos += sizeof(char);
                    }
                    break;
                case 'r':
                case 'R':
                    inputPos += sizeof(char);
                    while ((*inputPos != '\0') && !((*inputPos >= '0') &&
                        (*inputPos <= '9')))
                    {
                        inputPos += sizeof(char);
                    }
                    if (*inputPos == '\0')
                    {
                        user(IS, BAD_REP);
                    }
                    else
                    {
                        n = 0;
                        while ((*inputPos >= '0') && (*inputPos <= '9'))
                        {
                            n = n * 10 + (*inputPos - '0');
                            inputPos += sizeof(char);
                        }
                        if ((n == 0) || (n > lineCount))
                        {
                            err(IS, "invalid line number");
                        }
                        else
                        {
                            while (*inputPos == ' ')
                            {
                                inputPos += sizeof(char);
                            }
                            if (*inputPos == '\0')
                            {
                                user(IS, BAD_REP);
                            }
                            else
                            {
                                delim = *inputPos;
                                inputPos += sizeof(char);
                                fromString = inputPos;
                                fromLen = 0;
                                while ((*inputPos != delim) && (*inputPos != '\0'))
                                {
                                    inputPos += sizeof(char);
                                    fromLen++;
                                }
                                if (*inputPos == delim)
                                {
                                    *inputPos = '\0';
                                    inputPos += sizeof(char);
                                    toString = inputPos;
                                    toLen = 0;
                                    while ((*inputPos != delim) &&
                                        (*inputPos != '\0'))
                                    {
                                        inputPos += sizeof(char);
                                        toLen++;
                                    }
                                    if (*inputPos == delim)
                                    {
                                        *inputPos = '\0';
                                        line = 1;
                                        pos = buf;
                                        while (line < n)
                                        {
                                            if (*pos == '\n')
                                            {
                                                line++;
                                            }
                                            pos += sizeof(char);
                                        }
                                        pos = findString(pos, fromString);
                                        if (pos == NULL)
                                        {
                                            user3(IS, "** '", fromString,
                                                "' not found\n");
                                        }
                                        else
                                        {
                                            if (len - fromLen + toLen > MAXLEN)
                                            {
                                                err(IS, "not replaced - text too "
                                                    "big");
                                            }
                                            else
                                            {
                                                n = (len - fromLen + 1) *
                                                    sizeof(char) - (pos - buf);
                                                if (fromLen > toLen)
                                                {
                                                    memcpy(pos + toLen *
                                                        sizeof(char), pos +
                                                        fromLen * sizeof(char),
                                                      n);
                                                }
                                                else
                                                {
                                                    line = len + toLen - fromLen
                                                        - 1;
                                                    memmove(buf+line*sizeof(char),
                                                        buf+(len-1) *
                                                        sizeof(char), n);
                                                }
                                            }
                                            memcpy(pos, toString, toLen);
                                            len = len - fromLen + toLen;
                                        }
                                    }
                                    else
                                    {
                                        user(IS, BAD_REP);
                                    }
                                }
                                else
                                {
                                    user(IS, BAD_REP);
                                }
                            }
                        }
                    }
                    break;
                case 'd':
                case 'D':
                    inputPos += sizeof(char);
                    while ((*inputPos != '\0') && !((*inputPos >= '0') &&
                        (*inputPos <= '9')))
                    {
                        inputPos += sizeof(char);
                    }
                    if (*inputPos == '\0')
                    {
                        user(IS, "Use is: delete line-#\n");
                    }
                    else
                    {
                        n = 0;
                        while ((*inputPos >= '0') && (*inputPos <= '9'))
                        {
                            n = n * 10 + (*inputPos - '0');
                            inputPos += sizeof(char);
                        }
                        if ((n == 0) || (n > lineCount))
                        {
                            err(IS, "invalid line number");
                        }
                        else
                        {
                            line = 1;
                            pos = buf;
                            while (line < n)
                            {
                                if (*pos == '\n')
                                {
                                    line++;
                                }
                                pos += sizeof(char);
                            }
                            inputPos = pos;
                            while (*inputPos != '\n')
                            {
                                inputPos += sizeof(char);
                            }
                            inputPos += sizeof(char);
                            n = (inputPos - pos) / sizeof(char);
                            memcpy(pos, inputPos,
                                (len + 1) * sizeof(char) - (inputPos - buf));
                            lineCount--;
                            len -= n;
                        }
                    }
                    break;
            case 'i':
            case 'I':
                inputPos += sizeof(char);
                while ((*inputPos != '\0') && !((*inputPos >= '0') &&
                    (*inputPos <= '9')))
                {
                    inputPos += sizeof(char);
                }
                if (*inputPos == '\0')
                {
                    user(IS, "Useage is: insert line-# <text>\n");
                }
                else
                {
                    n = 0;
                    while ((*inputPos >= '0') && (*inputPos <= '9'))
                    {
                        n = n * 10 + (*inputPos - '0');
                        inputPos += sizeof(char);
                    }
                    if (n > lineCount)
                    {
                        err(IS, "invalid line number");
                    }
                    else
                    {
                        line = 0;
                        pos = buf;
                        while (line < n)
                        {
                            if (*pos == '\n')
                            {
                                line++;
                            }
                            pos += sizeof(char);
                        }
                        while (*inputPos == ' ')
                        {
                            inputPos += sizeof(char);
                        }
                        toString = inputPos;
                        toLen = 0;
                        while (*inputPos != '\0')
                        {
                            inputPos += sizeof(char);
                            toLen++;
                        }
                        *inputPos = '\n';
                        toLen++;
                        if (len + toLen > MAXLEN)
                        {
                            err(IS, "not inserted - text too big");
                        }
                        else
                        {
                            memmove(buf + (len + toLen) * sizeof(char), buf + len *
                                sizeof(char), (len + 1) * sizeof(char) -
                                (pos - buf));
                            memcpy(pos, toString, toLen);
                            len += toLen;
                            lineCount++;
                        }
                    }
                }
            }
        }
    }
    IS->is_request.rq_u.ru_telegram.te_data[len] = '\0';
    len++;
    IS->is_request.rq_u.ru_telegram.te_length = len;
    if (cancel)
    {
        return FALSE;
    }
    return TRUE;
}

void cmd_telegram(IMP)
{
    USHORT targets[LIST_SIZE];
    register USHORT targetCount = 0, i = 0, maxLoop = 0;
    Player_t *target;
    USHORT player;
    BOOL cancel, broadCast, sendFlag;
    Relation_t relation[LIST_SIZE];
    PlayerStatus_t status[LIST_SIZE];
    char whoTo = ' ';

    cancel = FALSE;
    broadCast = FALSE;
    target = &IS->is_request.rq_u.ru_player;
    if (((*IS->is_textInPos == '*') || (*IS->is_textInPos == '&') ||
       (*IS->is_textInPos == '!') || (*IS->is_textInPos == '@')) &&
       ((IS->is_player.p_status == ps_deity) || IS->is_world.w_sendAll))
    {
        whoTo = *IS->is_textInPos;
        IS->is_textInPos += sizeof(char);
        broadCast = TRUE;
    }
    else
    {
        if (!reqPlayer(IS, &player, "Player to send telegram to"))
        {
            cancel = TRUE;
        }
    }
    if (!cancel)
    {
        if (!broadCast)
        {
            targets[0] = player;
            targetCount = 1;
            while ((*skipBlanks(IS) != '\0') && (targetCount != LIST_SIZE))
            {
                if (getPlayer(IS, &targets[targetCount]))
                {
                    targetCount++;
                }
            }
            for (i = 0; i < targetCount; i++)
            {
                server(IS, rt_readPlayer, targets[i]);
                relation[i] = getMyRelation(IS, target);
                status[i] = target->p_status;
            }
        }
        if (broadCast)
        {
            if (IS->is_player.p_status == ps_visitor)
            {
                maxLoop = 1;
            }
            else
            {
                maxLoop = IS->is_world.w_currPlayers;
            }
            for (player = 0; player < maxLoop; player++)
            {
                server(IS, rt_readPlayer, player);
                relation[player] = getMyRelation(IS, target);
                status[i] = target->p_status;
            }
        }
        if (getText(IS, tt_telegram))
        {
            server(IS, rt_nop, 0);
            IS->is_request.rq_u.ru_telegram.te_time = IS->is_request.rq_time;
            IS->is_request.rq_u.ru_telegram.te_from = IS->is_player.p_number;
            if (broadCast)
            {
                for (player = 0; player < maxLoop; player++)
                {
                    sendFlag = FALSE;
                    if (player != IS->is_player.p_number)
                    {
                        switch(whoTo)
                        {
                           case '*':
                                sendFlag = TRUE;
                                break;
                           case '&':
                                if (relation[player] == r_allied)
                                {
                                    sendFlag = TRUE;
                                }
                                break;
                           case '!':
                                if (relation[player] == r_war)
                                {
                                    sendFlag = TRUE;
                                }
                                break;
                           case '@':
                                if (relation[player] == r_neutral)
                                {
                                    sendFlag = TRUE;
                                }
                                break;
                        }
                    }
                    if (sendFlag)
                    {
                       IS->is_request.rq_u.ru_telegram.te_to = player;
                       server(IS, rt_sendTelegram, player);
                    }
                }
            }
            else
            {
                for (i = 0; i < targetCount; i++)
                {
                    player = targets[i];
                    if ((IS->is_player.p_status != ps_visitor) ||
                        (player == 0))
                    {
                       IS->is_request.rq_u.ru_telegram.te_to = player;
                       server(IS, rt_sendTelegram, player);
                    }
                }
                /* have to split up, since both use the message structure */
                if ((IS->is_player.p_status != ps_visitor) &&
                   (IS->is_player.p_status != ps_deity))
                {
                    for (i = 0; i < targetCount; i++)
                    {
                        if ((targets[i] != 0) && (status[i] != ps_deity))
                        {
                           news(IS, n_sent_telegram, IS->is_player.p_number,
                               targets[i]);
                        }
                    }
                }
            }
        }
    }
}

/*
 * telegramCheck - called to check for new telegrams at login.
 */

void telegramCheck(IMP)
{
    server(IS, rt_checkMessages, IS->is_player.p_number);
    if (IS->is_request.rq_u.ru_messageCheck.mc_hasOldTelegrams)
    {
        user(IS, "You have old telegrams.\n");
    }
    if (IS->is_request.rq_u.ru_messageCheck.mc_hasNewTelegrams)
    {
        user(IS, "You have new telegrams.\n");
    }
}

/*
 * messageCheck - called to check for and print messages after each command.
 */

void messageCheck(IMP)
{
    BOOL newPlayer, messages, telegrams;

    /* By default, there are no messages */
    IS->is_argShort = 0;
    server(IS, rt_checkMessages, IS->is_player.p_number);
    newPlayer = IS->is_request.rq_u.ru_messageCheck.mc_newPlayer;
    messages = IS->is_request.rq_u.ru_messageCheck.mc_hasMessages;
    telegrams = IS->is_request.rq_u.ru_messageCheck.mc_hasNewTelegrams;
    if (IS->is_request.rq_u.ru_messageCheck.mc_newWorld)
    {
        server(IS, rt_readWorld, 0);
        memcpy(&IS->is_world, &IS->is_request.rq_u.ru_world, sizeof(World_t));
    }
    if (newPlayer)
    {
        server(IS, rt_readPlayer, IS->is_player.p_number);
        memcpy(&IS->is_player, &IS->is_request.rq_u.ru_player,
            sizeof(Player_t));
    }
    if (messages)
    {
	userNL(IS);
        server(IS, rt_getMessage, 0);
        while(IS->is_request.rq_u.ru_telegram.te_length != 0)
        {
            user(IS, &IS->is_request.rq_u.ru_telegram.te_data[0]);
            server(IS, rt_getMessage, 0);
        }
	IS->is_argShort = 1;
    }
    if (telegrams)
    {
        user(IS, "\nYou have new telegrams.\n");
	IS->is_argShort = 1;
    }
}

void cmd_read(IMP)
{
    char buff[TELEGRAM_MAX + 1];
    register USHORT i, sender;
    BOOL hadOne;

    hadOne = FALSE;
    server(IS, rt_readTelegram, IS->is_player.p_number);
    i = IS->is_request.rq_u.ru_telegram.te_length;
    while(i != 0)
    {
        memcpy(&buff[0], &IS->is_request.rq_u.ru_telegram.te_data[0], i);
        uTime(IS, IS->is_request.rq_u.ru_telegram.te_time);
        if (IS->is_request.rq_u.ru_telegram.te_from == NOBODY)
        {
            user(IS, ": anonymous telegram:\n\n");
            user(IS, &buff[0]);
            userNL(IS);
        }
        else
        {
            sender = IS->is_request.rq_u.ru_telegram.te_from;
            server(IS, rt_readPlayer, sender);
            user(IS, ": telegram from ");
            user(IS, &IS->is_request.rq_u.ru_player.p_name[0]);
            user(IS, ":\n\n");
            user(IS, &buff[0]);
            userNL(IS);
            if ((sender == 0) || (IS->is_player.p_status != ps_visitor))
            {
                if (ask(IS, "Reply to this telegram? "))
                {
                    if (getText(IS, tt_telegram))
                    {
                        IS->is_request.rq_u.ru_telegram.te_time =
                           IS->is_request.rq_time;
                        IS->is_request.rq_u.ru_telegram.te_from =
                           IS->is_player.p_number;
                        IS->is_request.rq_u.ru_telegram.te_to = sender;
                        server(IS, rt_sendTelegram, sender);
                        if ((IS->is_player.p_status != ps_deity) &&
                            (sender != 0))
                        {
                           news(IS, n_sent_telegram, IS->is_player.p_number,
                                sender);
                        }
                    }
                }
            }
        }
        hadOne = TRUE;
        server(IS, rt_readTelegram, IS->is_player.p_number);
        i = IS->is_request.rq_u.ru_telegram.te_length;
    }
    if (hadOne)
    {
        if (ask(IS, "Delete these telegrams? "))
        {
            server(IS, rt_readTelegram, TELE_DELETE);
        }
        else
        {
            server(IS, rt_readTelegram, TELE_KEEP);
        }
    }
    else
    {
        user(IS, "No telegrams\n");
    }
}

/*
 * sayHeadLine - report on a headline item.
 */

void sayHeadLine(IMP, register short recent, register short past, char *actor,
    char *victim)
{
    register char *message;
    USHORT head;

    if (past > recent)
    {
        head = 1;
    }
    else
    {
        head = 0;
    }
    if (past >= 0)
    {
        head += 2;
    }
    if (recent >= 0)
    {
        head += 4;
    }
    switch (head)
    {
        case 0:
            if (recent == past)
            {
                message =
                    "Carnage being wreaked by $ on $ continues unabated!";
            }
            else
            {
                message = "Slight let-up in the $ violence against $";
            }
            break;
        case 1:
            if (recent < -10)
            {
                message = "$ increases agression against $";
            }
            else
            {
                if (recent < -20)
                {
                    message =
                        "No sign of a letdown in the $ - $ hostilities!";
                }
                else
                {
                    message = "Minor skirmishes between $ and $";
                }
            }
            break;
        case 2:
            if (recent < -12)
            {
                if (past > 8)
                {
                    message =  "! WAR !  A complete reversal of prior $ -- $ relations";
                }
                else
                {
                    if (recent < -20)
                    {
                        message = "$ wages an all-out war on $!";
                    }
                    else
                    {
                        message = "VIOLENCE ERUPTS! -- $ wages war on $!";
                    }
                }
            }
            else
            {
                if (recent < -10)
                {
                    message = "$ violence against $ shows no sign of letting up";
                }
                else
                {
                    message = "Breakdown in communication between $ & $";
                }
            }
            break;
        case 3:
            if (past > 10)
            {
                if (recent < -8)
                {
                    message = "FLASH!   $ disavows former alliance with $ and wages war!";
                }
                else
                {
                    message = "FLASH!   $ turns on former ally, $";
                }
            }
            else
            {
                message = "$ aggravates rift with $";
            }
            break;
        case 4:
            if (recent > 10)
            {
                message = "$ enters new era of cooperation with $";
            }
            else
            {
                if (recent > 5)
                {
                    message = "$ increases their bond of friendship with $";
                }
                else
                {
                    message = "$ \"makes friends\" with $";
                }
            }
            break;
        case 5:
            if (recent > 5)
            {
                message = "$ willing to bury the hatchet with $";
            }
            else
            {
                if (past < -16)
                {
                    message = "Tensions ease as $ attacks on $ seem at an end";
                }
                else
                {
                    message = "$ seems to have forgotten earlier disagreement with $";
                }
            }
            break;
        case 6:
            if (recent > 15)
            {
                message = "The $-$ alliance seems unbreakable";
            }
            else
            {
                message = "$ good deeds further growing alliance with $";
            }
            break;
        case 7:
            if (recent - past < -20)
            {
                message = "Honeymoon appears to be over between $ & $";
            }
            else
            {
                message = "Friendly relations between $ & $ have cooled somewhat";
            }
            break;
        default:
            message = "***unknown headline***";
            break;
    }
    while (*message != '\0')
    {
        if (*message == '$')
        {
            user(IS, actor);
            actor = victim;
        }
        else
        {
            userC(IS, *message);
        }
        message += sizeof(char);
    }
    userNL(IS);
}

/*
 * doHeadLines - generate headlines for the given number of "days" of news.
 */

void doHeadLines(IMP, ULONG days)
{
    /* define the structures we will use */
    typedef struct
        {
            short
                h_past,
                h_recent;
        } history_t;
    typedef struct
        {
            history_t
                toPlay[PLAYER_MAX];
        } histArray_t;
    typedef struct
        {
            histArray_t
                fromPlay[PLAYER_MAX];
        } playHist_t;

    register USHORT i, j;
    ULONG now, time, day;
    playHist_t *hist;
    char name1[NAME_LEN], name2[NAME_LEN];
    News_t *n;
    history_t *h;
    short goodWill, scoop;
    USHORT scoopI = NOBODY, scoopJ = NOBODY;
    BOOL hadOne;

    server(IS, rt_nop, 0);
    now = IS->is_request.rq_time;

    /* try and allocate the memory */
    hist = (playHist_t *)calloc(1, sizeof(playHist_t));
    /* make sure we got it */
    if (hist == NULL)
    {
        err(IS, "Unable to allocate memory for headline history!");
        return;
    }

    user(IS,
        "\n"
        "            ]=-  IMPERIUM NEWS LINK  -=[\n"
        "+--------------------------------------------------+\n"
        "|       \"All the news thats fit, we print.\"        |\n"
        "+--------------------------------------------------+\n"
        "             ");
    uTime(IS, now);
    userNL(IS);
    (void) printFile(IS, BULLETIN_FILE, ft_normal);
    n = &IS->is_request.rq_u.ru_news;
    time =
        now - (ULONG)(days - 1) * IS->is_world.w_secondsPerITU * (2 * 24);
    day = 0;
    IS->is_request.rq_whichUnit = 0;
    hadOne = TRUE;
    while (hadOne || (day != days))
    {
        IS->is_request.rq_u.ru_news.n_time = time;
        server(IS, rt_readNews, IS->is_request.rq_whichUnit);
        hadOne = IS->is_request.rq_whichUnit != 0;
        if (hadOne)
        {
            if (n->n_actor != n->n_victim)
            {
                goodWill = NEWS_GOOD_WILL[n->n_verb];
                if (goodWill != 0)
                {
                    h = &hist->fromPlay[n->n_actor].toPlay[n->n_victim];
                    if (now - n->n_time >
                            days * IS->is_world.w_secondsPerITU * 24)
                    {
                        h->h_past += goodWill;
                    }
                    else
                    {
                        h->h_recent += goodWill;
                    }
                }
            }
        }
        else
        {
            day++;
            time += IS->is_world.w_secondsPerITU * (2 * 24);
        }
    }
    hadOne = FALSE;
    scoop = 9;
    for (i = 0; i < PLAYER_MAX; i++)
    {
        for (j = 0; j < PLAYER_MAX; j++)
        {
            h = &hist->fromPlay[i].toPlay[j];
            goodWill = abs(h->h_recent / 2);
            if (goodWill > scoop)
            {
                scoop = goodWill;
                scoopI = i;
                scoopJ = j;
            }
            goodWill = abs(h->h_recent - h->h_past);
            if (goodWill > scoop)
            {
                scoop = goodWill;
                scoopI = i;
                scoopJ = j;
            }
        }
    }
    userNL(IS);
    while (scoop > 9)
    {
        hadOne = TRUE;
        h = &hist->fromPlay[scoopI].toPlay[scoopJ];
        if (scoopI == NOBODY)
        {
            strcpy(name1, "*NOBODY*");
        }
        else
        {
            server(IS, rt_readPlayer, scoopI);
            strcpy(name1, IS->is_request.rq_u.ru_player.p_name);
        }
        if (scoopJ == NOBODY)
        {
            strcpy(name2, "*NOBODY*");
        }
        else
        {
            server(IS, rt_readPlayer, scoopJ);
            strcpy(name2, IS->is_request.rq_u.ru_player.p_name);
        }
        sayHeadLine(IS, h->h_recent, h->h_past, &name1[0], &name2[0]);
        scoop = h->h_recent;
        h->h_recent = 0;
        h->h_past = 0;
        h = &hist->fromPlay[scoopJ].toPlay[scoopI];
        goodWill = h->h_recent;
        if (((scoop < 0) && (goodWill >= -scoop / 2)) ||
            ((scoop > 0) && (goodWill <= -scoop / 2)))
        {
            switch (abs(goodWill % 4))
            {
                case 0:
                    user(IS, "        Meanwhile\n");
                    break;
                case 1:
                    user(IS, "        On the other hand\n");
                    break;
                case 2:
                    user(IS, "        At the same time\n");
                    break;
                case 3:
                    user(IS, "        Although\n");
                    break;
            }
            sayHeadLine(IS, h->h_recent, h->h_past, &name2[0], &name1[0]);
        }
        h->h_recent = 0;
        h->h_past = 0;
        scoop = 9;
        for (i = 0; i < PLAYER_MAX; i++)
        {
            for (j = 0; j < PLAYER_MAX; j++)
            {
                h = &hist->fromPlay[i].toPlay[j];
                goodWill = abs(h->h_recent / 2);
                if (goodWill > scoop)
                {
                    scoop = goodWill;
                    scoopI = i;
                    scoopJ = j;
                }
                goodWill = abs(h->h_recent - h->h_past);
                if (goodWill > scoop)
                {
                    scoop = goodWill;
                    scoopI = i;
                    scoopJ = j;
                }
            }
        }
        userNL(IS);
    }
    if (!hadOne)
    {
        user(IS, "Relative calm prevails.\n");
    }
    /* return the memory */
    free(hist);
}

/*
 * cmd_headlines - allows the player to display the headlines for the given
 *          number of days
 */

void cmd_headlines(IMP)
{
    long i;

    if (*IS->is_textInPos == '\0')
    {
        doHeadLines(IS, 3);    /* Do for 3 days unless told otherwise  */
    }
    else
    {
        if (getNumber(IS, &i) && (i > 0))
        {
            doHeadLines(IS, i);
        }
        else
        {
            err(IS, "Invalid number of days");
        }
    }
}

/*
 * sayNews - report an item of news, and how many times it happened.
 */

void sayNews(IMP, NewsType_t verb, USHORT actor, USHORT victim,
             ULONG time, USHORT count)
{
    register char *v;

    if (actor == NOBODY)
    {
        v = "*NOBODY*";
    }
    else
    {
        server(IS, rt_readPlayer, actor);
        v = &IS->is_request.rq_u.ru_player.p_name[0];
    }
    uTime(IS, time);
    user3(IS, ": ", v, " ");
    if (victim == NOBODY)
    {
        v = "*NOBODY*";
    }
    else
    {
        server(IS, rt_readPlayer, victim);
        v = &IS->is_request.rq_u.ru_player.p_name[0];
    }
    switch (verb)
    {
        case n_nothing:
            user2(IS, "did absolutely nothing to ", v);
            break;
        case n_won_planet:
            user2(IS, "won a planet from ", v);
            break;
        case n_lost_planet:
            user2(IS, "was repulsed by ", v);
            break;
        case n_sent_telegram:
            user2(IS, "sent a telegram to ", v);
            break;
        case n_make_loan:
            user2(IS, "made a loan to ", v);
            break;
        case n_repay_loan:
            user2(IS, "repaid a loan from ", v);
            break;
        case n_make_sale:
            user2(IS, "made a sale to ", v);
            break;
        case n_grant_planet:
            user2(IS, "granted a planet to ", v);
            break;
        case n_took_unoccupied:
            user(IS, "took over an unoccupied planet");
            break;
        case n_fire_back:
            user3(IS, "fired on ", v, " in self defense");
            break;
        case n_bomb_planet:
            user3(IS, "bombed one of ", v, "'s planets");
            break;
        case n_bomb_dest:
            user3(IS, "wiped out one of ", v, "'s planets");
            break;
        case n_board_ship:
            user3(IS, "boarded a(n) ", v, " ship");
            break;
        case n_failed_board:
            user3(IS, "was repelled by ", v, " while attempting to board "
                "a ship");
            break;
        case n_flak:
            user3(IS, "fired on ", v, " aircraft");
            break;
        case n_sieze_planet:
            user3(IS, "siezed a planet from ", v, " to collect on a loan");
            break;
        case n_decl_ally:
            user2(IS, "announced an alliance with ", v);
            break;
        case n_decl_neut:
            user2(IS, "declared their neutrality toward ", v);
            break;
        case n_decl_war:
            user2(IS, "declared WAR on ", v);
            break;
        case n_disavow_ally:
            user2(IS, "disavowed former alliance with ", v);
            break;
        case n_disavow_war:
            user2(IS, "disavowed former war with ", v);
            break;
        case n_plague_outbreak:
            user(IS, "controlled planet reports outbreak of PLAGUE");
            break;
        case n_plague_die:
            user(IS, "controlled planet had citizens killed by PLAGUE");
            break;
        case n_plague_dest:
            user(IS, "controlled planet wiped out by PLAGUE!");
            break;
        case n_destroyed:
            user3(IS, "DESTROYED ", v, "!!!");
            break;
        case n_ship_dest:
            user3(IS, "destroyed a(n) ", v, " ship");
            break;
        default:
            user(IS, "***unknown news type***");
            break;
    }
    if (count != 1)
    {
        userN3(IS, " ", count, " times");
    }
    userNL(IS);
}

/*
 * cmd_newspaper - allows the player to read the news items for the given
 *          number of days
 */

void cmd_newspaper(IMP)
{
    char buff[TELEGRAM_MAX + 1];
    register News_t *n;
    News_t saveItem;
    ULONG now, timeStep, start, time, prevTime = 0;
    long i;
    register USHORT day, count;
    USHORT days = 3, page, item;
    NewsType_t prevVerb;
    USHORT prevActor, prevVictim;
    BOOL quit, hadOne, hadItem, deflt = FALSE;

    quit = FALSE;
    if (*IS->is_textInPos == '\0')
    {
        deflt = TRUE;  /* use 3 days for headlines ONLY, if defaulting */
    }
    else
    {
        if (getNumber(IS, &i) && (i > 0))
        {
            days = i;
        }
        else
        {
            err(IS, "Invalid number of days");
            quit = TRUE;
        }
    }
    n = &IS->is_request.rq_u.ru_news;
    if (!quit)
    {
        doHeadLines(IS, days);
        if (deflt)
        {
           days = 1;
        }
        server(IS, rt_nop, 0);
        timeStep = IS->is_world.w_secondsPerITU * (2 * 24);
        now = IS->is_request.rq_time / timeStep * timeStep;
        start = now - (ULONG)(days - 1) * timeStep;
        userNL(IS);
        user(IS, "Summary of the Imperium news link since ");
        uTime(IS, start);
        user(IS, "\n\n");
        page = 1;
        while ((page != 5) && !quit)
        {
            userN3(IS, "        === page ", page, " ===\n\n");
            hadItem = FALSE;
            prevVerb = n_nothing;
            prevActor = 0;
            prevVictim = 0;
            count = 0;
            time = start;
            day = 0;
            hadOne = FALSE;
            item = 0;
            while ((hadOne || (day != days)) && !quit)
            {
                if (page != 4)
                {
                    IS->is_request.rq_u.ru_news.n_time = time;
                    server(IS, rt_readNews, item);
                    hadOne = IS->is_request.rq_whichUnit != 0;
                    if (hadOne)
                    {
                        item++;
                        if (NEWS_PAGE[n->n_verb] == page)
                        {
                            if ((n->n_verb != prevVerb) ||
                                (n->n_actor != prevActor) ||
                                (n->n_victim != prevVictim))
                            {
                                prevTime = n->n_time;
                                if (count != 0)
                                {
                                    saveItem = *n;
                                    sayNews(IS, prevVerb, prevActor,
                                            prevVictim, prevTime, count);
                                    hadItem = TRUE;
                                    if (clGotCtrlC(IS))
                                    {
                                        quit = TRUE;
                                    }
                                    *n = saveItem;
                                }
                                count = 1;
                                prevVerb = n->n_verb;
                                prevActor = n->n_actor;
                                prevVictim = n->n_victim;
                                prevTime = n->n_time;
                            }
                            else
                            {
                                count++;
                            }
                        }
                    }
                }
                else
                {
                    IS->is_request.rq_u.ru_telegram.te_time = time;
                    server(IS, rt_readPropaganda, 0);
                    count = IS->is_request.rq_u.ru_telegram.te_length;
                    if (count != 0)
                    {
                        memcpy(&buff[0],
                                  &IS->is_request.rq_u.ru_telegram.te_data[0],
                                  count);
                        buff[count] = '\0';
                        prevTime = IS->is_request.rq_u.ru_telegram.te_time;
                        if (IS->is_request.rq_u.ru_telegram.te_from == NOBODY)
                        {
                            user(IS, "*NOBODY*");
                        }
                        else
                        {
                            server(IS, rt_readPlayer,
                               IS->is_request.rq_u.ru_telegram.te_from);
                            user(IS, &IS->is_request.rq_u.ru_player.p_name[0]);
                        }
                        user(IS, " at ");
                        uTime(IS, prevTime);
                        user(IS, ":\n");
                        user(IS, &buff[0]);
                        userNL(IS);
                        hadOne = TRUE;
                    }
                    else
                    {
                        hadOne = FALSE;
                    }
                }
                if (!hadOne)
                {
                    day++;
                    time += timeStep;
                    item = 0;
                }
            }
            if ((day != 4) && (count != 0) && !quit)
            {
                sayNews(IS, prevVerb, prevActor, prevVictim, prevTime, count);
                hadItem = TRUE;
            }
            page = page + 1;
            if (hadItem)
            {
                userNL(IS);
            }
        }
    }
}

/*
 * cmd_propaganda - allows the player to enter section 4 items
 */

BOOL cmd_propaganda(IMP)
{
    if (getText(IS, tt_propaganda))
    {
        server(IS, rt_nop, 0);
        IS->is_request.rq_u.ru_telegram.te_time = IS->is_request.rq_time;
        IS->is_request.rq_u.ru_telegram.te_from = IS->is_player.p_number;
        server(IS, rt_propaganda, 0);
        return TRUE;
    }
    return FALSE;
}

/*
 * cmd_message - allows the player to send a real-time message to another
 *          player
 */

void cmd_message(IMP)
{
    register char *p, *q;
    register long len;
    USHORT player;
    BOOL ok;

    ok = FALSE;
    if (*IS->is_textInPos != '\0')
    {
        if (getPlayer(IS, &player))
        {
            p = skipBlanks(IS);
            if (*p == '\0')
            {
                ok = getText(IS, tt_message);
            }
            else
            {
                q = &IS->is_request.rq_u.ru_telegram.te_data[0];
                len = strlen(&IS->is_player.p_name[0]);
                memcpy(q, &IS->is_player.p_name[0], len);
                q += len * sizeof(char);
                *q = ':';
                q += sizeof(char);
                *q = ' ';
                q += sizeof(char);
                len += 2;
                while (*p != '\0')
                {
                    *q = *p;
                    q += sizeof(char);
                    p += sizeof(char);
                    len++;
                }
                *q = '\n';
                q += sizeof(char);
                *q = '\0';
                len += 2;
                IS->is_request.rq_u.ru_telegram.te_length = len;
                ok = TRUE;
            }
        }
    }
    else
    {
        if (reqPlayer(IS, &player, "Player to send message to"))
        {
            ok = getText(IS, tt_message);
        }
    }
    if (ok)
    {
        IS->is_request.rq_u.ru_telegram.te_to = player;
        server(IS, rt_message, player);
        if (IS->is_request.rq_whichUnit == MESSAGE_NO_PLAYER)
        {
            if (IS->is_player.p_status != ps_visitor)
            {
                if (ask(IS, "Not logged in - send as telegram? "))
                {
                    IS->is_request.rq_u.ru_telegram.te_time =
                       IS->is_request.rq_time;
                    IS->is_request.rq_u.ru_telegram.te_from =
                       IS->is_player.p_number;
                    server(IS, rt_sendTelegram, player);
                    if ((IS->is_player.p_status != ps_deity) && (player != 0))
                    {
                        news(IS, n_sent_telegram, IS->is_player.p_number,
                            player);
                    }
               }
           }
        }
        else
        {
            if (IS->is_request.rq_whichUnit == MESSAGE_FAIL)
            {
                err(IS, "Sorry, message send failed due to lack of space in "
                    "server");
            }
        }
    }
}

/*
 * startChat - ready a header to go into a chat message.
 */

USHORT startChat(IMP, char *m)
{
    register char *p, *q;
    register USHORT len;

    q = &IS->is_request.rq_u.ru_telegram.te_data[0];
    len = 0;
    if (m != NULL)
    {
        *q = '*';
        q += sizeof(char);
        *q = '*';
        q += sizeof(char);
        *q = '*';
        q += sizeof(char);
        len = 3;
    }
    p = &IS->is_player.p_name[0];
    while (*p != '\0')
    {
        *q = *p;
        q += sizeof(char);
        p += sizeof(char);
        len++;
    }
    *q = ':';
    q += sizeof(char);
    *q = ' ';
    q += sizeof(char);
    len += 2;
    if (m != NULL)
    {
        p = m;
        while (*p != '\0')
        {
            *q = *p;
            q += sizeof(char);
            p += sizeof(char);
            len++;
        }
        *q = '\n';
        q += sizeof(char);
        *q = '\0';
        len += 2;
    }
    return len;
}

/*
 * cmd_chat - go into chat mode.
 */

void cmd_chat(IMP)
{
    register char *p, *q;
    register USHORT len;
    register USHORT lastTime;
    register BOOL needPrompt;

    user(IS, "Entering chat mode. Type . to exit.\n");
    server(IS, rt_lockPlayer, IS->is_player.p_number);
    IS->is_request.rq_u.ru_player.p_inChat = TRUE;
    server(IS, rt_unlockPlayer, IS->is_player.p_number);
    IS->is_player.p_inChat = TRUE;
    server(IS, rt_setChat, 1);
    IS->is_request.rq_u.ru_telegram.te_length = startChat(IS, "<ENTERED>");
    server(IS, rt_sendChat, 0);
    server(IS, rt_getMessage, 0);
    while (IS->is_request.rq_u.ru_telegram.te_length != 0)
    {
        user(IS, &IS->is_request.rq_u.ru_telegram.te_data[0]);
        server(IS, rt_getMessage, 0);
    }
    if (IS->is_player.p_feMode & FE_HAS_PROMPT)
    {
        user(IS, FE_DISP_PROMPT);
	user3(IS, "(Chat Mode) ", &IS->is_player.p_name[0], ">\n");
    }
    else
    {
	user2(IS, &IS->is_player.p_name[0], "> ");
    }
    uFlush(IS);
    p = &IS->is_textIn[0];
    lastTime = 0xffff;
    /* loop until they either hang up, enter a '.' at the start of a line */
    /* or they run out of time */
    while (clTimedReadUser(IS) &&
            ((*p != '.') || (*(p + sizeof(char)) != '\0')) &&
            !updateTimer(IS))
    {
        if (*p != '\0')
        {
            len = startChat(IS, NULL);
            q = &IS->is_request.rq_u.ru_telegram.te_data[len];
            while (*p != '\0')
            {
                *q = *p;
                q += sizeof(char);
                p += sizeof(char);
                len++;
            }
            *q = '\n';
            q += sizeof(char);
            *q = '\0';
            len += 2;
            IS->is_request.rq_u.ru_telegram.te_length = len;
            server(IS, rt_sendChat, 0);
        }
        needPrompt = TRUE;
        if (IS->is_argShort == 0)
        {
            /* the line timed out, and we should not prompt them again */
            needPrompt = FALSE;
        }
        /* see if they are low on time */
        if (IS->is_player.p_timeLeft < 4)
        {
            /* have we already warned them this minute */
            if (lastTime != IS->is_player.p_timeLeft)
            {
                /* then do so */
                userN3(IS, ">>> WARNING: ", IS->is_player.p_timeLeft,
                    " minute(s) left <<<\n");
                lastTime = IS->is_player.p_timeLeft;
                /* force it to display a new prompt */
                needPrompt = TRUE;
            }
        }
        server(IS, rt_getMessage, 0);
        if (IS->is_request.rq_u.ru_telegram.te_length != 0)
        {
            if (!needPrompt)
            {
                userNL(IS);
            }
            while (IS->is_request.rq_u.ru_telegram.te_length != 0)
            {
                user(IS, &IS->is_request.rq_u.ru_telegram.te_data[0]);
                server(IS, rt_getMessage, 0);
            }
            needPrompt = TRUE;
        }
        if (needPrompt)
        {
	    if (IS->is_player.p_feMode & FE_HAS_PROMPT)
	    {
	        user(IS, FE_DISP_PROMPT);
		user3(IS, "(Chat Mode) ", &IS->is_player.p_name[0], ">\n");
	    }
	    else
	    {
                user2(IS, &IS->is_player.p_name[0], "> ");
                uFlush(IS);
	    }
        }
        p = &IS->is_textIn[0];
    }
    server(IS, rt_setChat, 0);
    IS->is_request.rq_u.ru_telegram.te_length = startChat(IS, "<EXITED>");
    server(IS, rt_sendChat, 0);
    server(IS, rt_lockPlayer, IS->is_player.p_number);
    IS->is_request.rq_u.ru_player.p_inChat = FALSE;
    server(IS, rt_unlockPlayer, IS->is_player.p_number);
    IS->is_player.p_inChat = FALSE;
    user(IS, "Leaving chat mode.\n");
}

/*
 * notify - send the buffered up message, after adding a newline, to the
 *      given player as a telegram or realtime message
 */

void notify(IMP, USHORT player)
{
    if (player == IS->is_player.p_number)
    {
        userNL(IS);
    }
    else
    {
        IS->is_textOut[IS->is_textOutPos] = '\n';
        IS->is_textOut[IS->is_textOutPos + 1] = '\0';
        memcpy(&IS->is_request.rq_u.ru_telegram.te_data[0],
                  &IS->is_textOut[0], IS->is_textOutPos + 2);
        IS->is_request.rq_u.ru_telegram.te_time = IS->is_request.rq_time;
        IS->is_request.rq_u.ru_telegram.te_from = NOBODY;
        IS->is_request.rq_u.ru_telegram.te_to = player;
        IS->is_request.rq_u.ru_telegram.te_length = IS->is_textOutPos + 2;
        server(IS, rt_sendTelegram, player);
        IS->is_textOutPos = 0;
    }
}
