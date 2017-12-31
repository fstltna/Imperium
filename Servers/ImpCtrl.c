/*
 * Imperium
 *
 * $Id: ImpCtrl.c,v 1.4 2000/05/23 20:25:42 marisa Exp $
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
 * $Log: ImpCtrl.c,v $
 * Revision 1.4  2000/05/23 20:25:42  marisa
 * Added pl_weapon[] element to Planet_t struct
 * Added inclusion ofcrypt.hfor GNU/Linux
 *
 * Revision 1.3  2000/05/18 08:11:13  marisa
 * Autoconf working
 *
 * Revision 1.2  2000/05/18 06:32:08  marisa
 * Crypt, etc.
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0  2000/05/16 06:31:05  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:13:52  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:06:03  marisag
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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <ctype.h>
#include <pwd.h>

#define TRUE (1)
#define FALSE (0)
#define BOOL unsigned char
#define BYTE signed char
#define UBYTE unsigned char
#define ULONG unsigned long
#define USHORT unsigned short

#include "../Include/ImpLib.h"
#include "ImpCtrl.h"
#include "../impsec.h"

static char const rcsid[] = "$Id: ImpCtrl.c,v 1.4 2000/05/23 20:25:42 marisa Exp $";

#define LOG_SIZE        (512 * 10)

#define MAX_CMDS    8

static char
    UsePort[256],
    PlayPort[256];

static int
    ImperiumPort,
    myPort;

ImpState_t
    *ISt;

FILE
    *LogFd;

ImpState_t
    *IS;
static BOOL
    ServLog,
    DoDebug;

static char
    nameBuff[257],
    LogBuffer[LOG_SIZE],
    cmds[MAX_CMDS + 1];

static USHORT
    NumCmds,
    passedTime;

static ULONG
    LogPos;


/*
 * logFlush - flush the user log data.
 */

void logFlush(void)
{
    if (LogPos)
    {
        (void) fwrite(&LogBuffer[0], sizeof(char), LogPos, LogFd);
        LogPos = 0;
    }
}

/*
 * serverAbort - something has gone wrong in our communications with the
 *      Imperium server. We issue a message and then shut down.
 */

void serverAbort(void)
{
    register Request_t *rq;
    fd_set readPipe;
    struct timeval tv;

    if (LogFd != NULL)
    {
        logFlush();
        fclose(LogFd);
    }
    rq = &IS->is_request;
    rq->rq_type = rt_stopClient;
    if (write(ImperiumPort, (char *)rq, sizeof(Request_t)) >= 0)
    {
	tv.tv_sec = 60;
	tv.tv_usec = 0;
	FD_ZERO(&readPipe);
	FD_SET(myPort, &readPipe);
	if (select(myPort+1, &readPipe, 0, 0, &tv) > 0)
	{
	    if (FD_ISSET(myPort, &readPipe))
	    {
		(void)read(myPort, (char *)rq, sizeof(Request_t));
	    }
	}
    }
    close(myPort);
    (void)unlink(&UsePort[0]);
    if (ISt != NULL)
    {
        ImpFree(ISt);
    }
    exit(10);
}

/*
 * serverRequest - routine to allow library to send a request to the Imperium
 *      server, and wait for a reply.
 */

void serverRequest(void)
{
    register Request_t *rq;
    RequestType_t rt;
    fd_set readPipe;

    rq = &IS->is_request;
    rt = rq->rq_type;
    if (write(ImperiumPort, (char *)rq, sizeof(Request_t)) < 0)
    {
        puts("*** serverRequest: write() failed! ***\n");
        serverAbort();
    }
    FD_ZERO(&readPipe);
    FD_SET(myPort, &readPipe);
    if (select(myPort+1, &readPipe, 0, 0, 0) <= 0)
    {
	perror("serverRequest: select");
        serverAbort();
    }
    if (FD_ISSET(myPort, &readPipe))
    {
	if (read(myPort, (char *)rq, sizeof(Request_t)) != sizeof(Request_t))
	{
	    puts("*** serverRequest: read() failed! ***\n");
	    serverAbort();
	}
        if (rq->rq_type != rt)
        {
            printf("*** serverRequest: got type %d back instead of %d ***\n",
		rq->rq_type - rt_nop, rt - rt_nop);
            serverAbort();
	}
    }
    else
    {
	puts("*** serverRequest: had FD_ISSET == FALSE");
        serverAbort();
    }
}

/*
 * writeLog - write some stuff to the user log file.
 */

void writeLog(register char *p, register USHORT i)
{
    while (i)
    {
        i--;
        LogBuffer[LogPos] = *p;
        p = p + sizeof(char);
        LogPos++;
        if (LogPos == LOG_SIZE)
        {
            logFlush();
        }
    }
}

/*
 * writeUser - routine to allow the Imperium library to write to the user.
 */

void writeUser(void)
{
    (void) fwrite(&IS->is_textOut[0], sizeof(char), IS->is_textOutPos, stdout);
    if (LogFd != NULL)
    {
        writeLog(&IS->is_textOut[0], IS->is_textOutPos);
    }
}

/*
 * readUser - routine to allow the Imperium library to read from the user.
 */

void readUser(void)
{
    register ImpState_t *myISt;

    /* do this to save time */
    myISt = IS;
    /* set the text position to the start of the line */
    myISt->is_textInPos = &myISt->is_textIn[0];
    myISt->is_textIn[0] = '\0';
    /* do it again just to be sure */
    myISt->is_textIn[1] = '\0';
    myISt->is_argBool = TRUE;
    return;
}

/*
 * timedReadUser - routine to allow the Imperium library to read from the
 *          user with a timeout on the first character
 */

void timedReadUser(void)
{
    /* for now we just do this */
    readUser();
    /* This indicates that the user entered this line */
    IS->is_argShort = 0xffff;
}

/*
 * echoOff - disable echo on the user input.
 */

void echoOff(void)
{
}

/*
 * echoOn - re-enable echo on the user input.
 */

void echoOn(void)
{
}

/*
 * gotControlC - return true if user has typed a CNTL-C.
 */

void gotControlC(void)
{
#ifdef FOOBAR
    if ((SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) != 0)
    {
        (void) Write(dos_fh, "Interrupt!!!\n", 13);
        ISt->is_argBool = TRUE;
        return;
    }
#endif
    ISt->is_argBool = FALSE;
    return;
}

/*
 * locSleep - sleep for the given number of seconds.
 */

void locSleep(void)
{
    sleep(ISt->is_argShort);
}

/*
 * doExtEdit - tell the system that we do not have an external editor
 */

void doExtEdit(void)
{
    ISt->is_argShort = 3;
}

/*
 * userLog - routine to allow the user to do logging.
 */

void userLog(void)
{
    register char *fileName;

    fileName = (char *)ISt->is_argLong;
    if (LogFd != NULL)
    {
        logFlush();
        fclose(LogFd);
        LogFd = NULL;
    }
    if (!fileName)
    {
        ISt->is_argBool = TRUE;
        return;
    }
    LogFd = fopen(fileName, "a");
    if (LogFd != NULL)
    {
        ISt->is_argBool = TRUE;
        return;
    }
    ISt->is_argBool = FALSE;
    return;
}

/*
 * runImpCtrl - start up the Imperium-specific stuff.
 */

BOOL runImpCtrl(void)
{
    BOOL ok;
    USHORT curCmd = 0;
    char fileDir[256], *tmpNam;
#ifdef HAVE_CRYPT
    char locSalt[3];
#endif

    ok = FALSE;
    if (DoDebug)
    {
	sprintf(fileDir, "%s/server/%s", FIFO_DIR, IMP_TEST_PORT);
    }
    else if (PlayPort[0] != '\0')
    {
	sprintf(fileDir, "%s/server/%s", FIFO_DIR, &PlayPort[0]);
    }
    else
    {
	sprintf(fileDir, "%s/server/%s", FIFO_DIR, IMPERIUM_PORT);
    }
    ImperiumPort = open(fileDir, O_WRONLY | O_NDELAY, 0);
    if (ImperiumPort >= 0)
    {
        ISt = ImpAlloc();
        if (ISt != NULL)
        {
	    tmpNam = tempnam(FIFO_DIR, "ictl");
	    if (tmpNam == NULL)
	    {
		puts("Unable to allocate memory for temp name");
	    }
	    else if (mknod(tmpNam, S_IFIFO|0622, 0) == -1)
	    {
		printf("Unable to create named pipe %s\n", tmpNam);
	    }
#ifdef BROKEN_MKNOD
	    else if (chmod(tmpNam, 0622) != 0)
	    {
		printf("Unable to change mode for named pipe %s\n", tmpNam);
	    }
#endif
#ifdef BROKEN_PIPE
	    else if ((myPort = open(tmpNam, O_RDWR | O_NDELAY)) == -1)
#else
	    else if ((myPort = open(tmpNam, O_RDONLY | O_NDELAY)) == -1)
#endif
	    {
		(void)unlink(tmpNam);
		printf("Unable to open named pipe %s\n", tmpNam);
	    }
	    else
	    {
		    strcpy(UsePort, tmpNam);
                    IS = ISt;
                    ISt->is_serverRequest = serverRequest;
                    ISt->is_writeUser = writeUser;
                    ISt->is_readUser = readUser;
                    ISt->is_timedReadUser = timedReadUser;
                    ISt->is_echoOff = echoOff;
                    ISt->is_echoOn = echoOn;
                    ISt->is_gotControlC = gotControlC;
                    ISt->is_sleep = locSleep;
                    ISt->is_log = userLog;
                    ISt->is_extEdit = doExtEdit;
                    LogFd = NULL;

                    ISt->is_request.rq_type = rt_startClient;
                    strcpy(&ISt->is_request.rq_text[0], tmpNam);
		    /* Fill in the correct security code */
#ifdef HAVE_CRYPT
		    sprintf(&locSalt[0], "%2.2x", getpid() % 64);
		    sprintf(&ISt->is_request.rq_private[2], "%-.15s",
			crypt(IMP_CTRL_CODE, &locSalt[0]));
#else /* !HAVE_CRYPT */
		    strcpy(&ISt->is_request.rq_private[2], IMP_CTRL_CODE);
#endif
                    serverRequest();
		    /* Check to see if we were recognized */
		    if (ISt->is_request.rq_private[2] == '\0')
		    {
			/* Nope! */
			puts("*** outdated/invalid identity code");
		    }
		    else
		    {
			/* Yup! */
		        if (ServLog)
		        {
                	    strcpy(&ISt->is_request.rq_private[2],
			        "Imperium control utility started");
			    ISt->is_request.rq_type = rt_log;
			    serverRequest();
		        }

                        /* clear out buffer and set up no outside time limit */
                        memset(&ISt->is_textIn[0], '\0', INPUT_BUFFER_SIZE *
                            sizeof(char));

                        /* do the init call */
                        ISt->is_argShort = 0;
                        ImpCntrl(ISt);

                        while (curCmd < NumCmds)
                        {
                            switch(cmds[curCmd])
                            {
                                case 'p':
                                    ISt->is_argShort = IC_POWER;
                                    break;
                                case 'u':
                                    ISt->is_argShort = IC_UPDATE;
                                    break;
                                case 'm':
                                    ISt->is_argShort = IC_MINER;
                                    break;
                                case 'l':
                                    ISt->is_argShort = IC_INCUSR;
                                    break;
                                case 'L':
                                    ISt->is_argShort = IC_DECUSR;
                                    break;
                                case 'f':
                                    ISt->is_argShort = IC_DOFLUSH;
                                    break;
                                case 's':
                                    ISt->is_argShort = IC_PUBLISH;
                                    break;
                                case '\x50': /* Get around metaconfig problem */
                                default:
                                    ISt->is_argShort = IC_FPOWER;
                                    break;
                            }
                            ISt->is_argBool = ServLog;
                            ImpCntrl(ISt);
                            curCmd++;
                        }

                        if (LogFd != NULL)
                        {
                            logFlush();
                            fclose(LogFd);
                        }
		        if (ServLog)
		        {
			    strcpy(&ISt->is_request.rq_private[2],
			        "Imperium control utility terminated");
			    ISt->is_request.rq_type = rt_log;
			    serverRequest();
		        }
                        ISt->is_request.rq_type = rt_stopClient;
                        serverRequest();
		    }

                    ImpFree(ISt);
                    ok = TRUE;
		    close(myPort);
		    (void)unlink(tmpNam);
	    }
	    if (tmpNam != NULL)
	    {
		free(tmpNam);
	    }
        }
        else
        {
            puts("*** can't allocate Imperium state structure.");
        }
	close(ImperiumPort);
    }
    else
    {
        puts("*** can't find Imperium port. Is server running?");
    }
    return ok;
}

/*
 * useage - print a simple CLI useage message.
 */

void useage(void)
{
    printf("ImpCtrl V%s.pl%d\n\n", IMP_BASE_REV,
	0);
    puts("Useage is: ImpCtrl [-l] [-L] [-p] [-\x50] [-u] [-m] [-f] [-t] [-s]\n"
        "    Note: The order options are specified is the order they will be\n"
        "          executed\n"
        "    l = indicate start of update commands\n"
        "    L = indicate end of update commands\n"
        "  * p = power report (no force)\n"
        "    \x50 = force new power report\n"
        "    u = global update of all planets\n"
        "    m = global update of all miners\n"
        "    f = flush server buffers to disk\n"
        "    s = publish our game in the directory\n"
        "    t = use test port\n\n"
        " * - May require input and should not be used non-interactively");
}

/*
 * getBool - interpret a YES/NO or ON/OFF from a string.
 */

BOOL getBool(register char *st)
{
    char locStr[256], *strPtr;

    strPtr = &locStr[0];
    while (*st != '\0')
    {
	*strPtr = *st;
	if (isalpha(*st))
	{
	    *strPtr = toupper(*st);
	}
	strPtr++;
	st++;
    }
    st = &locStr[0];
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
 * Removes the selected command from the list of commands entered
 */

void removeOption(USHORT curCmd)
{
    while (curCmd < NumCmds)
    {
	if ((curCmd + 1) == NumCmds)
	{
	    NumCmds--;
	    return;
	}
	cmds[curCmd] = cmds[curCmd + 1];
	curCmd++;
    }
}

/*
 * checkCmds - makes sure the given user is allowed to use the commands
 *             they are requesting
 */

void checkCmds(FILE *allowFD, const char *userName)
{
    BOOL foundName = FALSE;
    register USHORT nameLen;
    char lineBuf[256];
    USHORT curCmd = 0;

    /* Save this for speed reasons */
    nameLen = strlen(userName);
    /* Scan through looking for the given user name */
    while (fgets(lineBuf, 255, allowFD) != NULL)
    {
	/* Ignore any line that starts with a # or a \n */
	if ((lineBuf[0] != '#') && (lineBuf[0] != '\n'))
	{
	    /* See if the line starts with the given user name */
	    if (memcmp(userName, lineBuf, nameLen * sizeof(char)) == 0)
	    {
		/* Yup, it did */
		foundName = TRUE;
		break;
	    }
	}
    }
    /* If we have not found the username, check for file errors */
    /* And then for an "all user" line */
    if (!foundName)
    {
	if (ferror(allowFD) != 0)
	{
	    if (feof(allowFD) == 0)
	    {
		puts("Error reading ic.allow");
		NumCmds = 0;
		return;
	    }
	}
	clearerr(allowFD);
	/* rewind the file back to the beginning */
	if (fseek(allowFD, 0L, SEEK_SET) != 0)
	{
	    puts("Error seeking to start of ic.allow");
	    NumCmds = 0;
	    return;
	}
	/* see if there is an "all" entry */
	nameLen = 4;
	while (fgets(lineBuf, 255, allowFD) != NULL)
	{
	    if (lineBuf[0] != '#')
	    {
		if (memcmp("all ", lineBuf, nameLen * sizeof(char)) == 0)
		{
		    foundName = TRUE;
		    break;
		}
	    }
	}
	if (!foundName)
	{
	    if (ferror(allowFD) != 0)
	    {
		if (feof(allowFD) == 0)
		{
		    puts("Error reading ic.allow");
		    NumCmds = 0;
		    return;
		}
	    }
	}
    }
    /* If we have STILL not found a name, they are out of luck */
    if (!foundName)
    {
	puts("You are not allowed to use this command");
	NumCmds = 0;
	return;
    }
    /* Now do the simple checks */
    if ((lineBuf[nameLen] == '!') || (memcmp(&lineBuf[nameLen], "none",
	4 * sizeof(char)) == 0))
    {
	puts("Sorry, you are not allowed to use this command");
	NumCmds = 0;
	return;
    }
    if ((lineBuf[nameLen] == '*') || (memcmp(&lineBuf[nameLen], "all",
	3 * sizeof(char)) == 0))
    {
	return;
    }
    while (curCmd < NumCmds)
    {
	switch(cmds[curCmd])
	{
	    case 'p':
	    case 'u':
	    case 'm':
	    case 'l':
	    case 'L':
	    case 'f':
	    case 's':
	    case '\x50': /* Get around metaconfig problem */
		if (strchr(&lineBuf[nameLen], cmds[curCmd]) == NULL)
		{
		    removeOption(curCmd);
		}
		else
		{
		    curCmd++;
		}
		break;
	    default:
		removeOption(curCmd);
	        break;
	}
    }
}

/*
 * main - start everything up.
 */

int main(int argc, char *argv[])
{
    char *par;
    BOOL hadError, cont;
    USHORT curArg;
    char fileDir[256], userName[256], *userNamePtr;
    FILE *allowFD;
    struct passwd *myPwEnt;

    /* initialize our globals */
    DoDebug = FALSE;
    NumCmds = 0;
    PlayPort[0] = '\0';
    UsePort[0] = '\0';
    ISt = NULL;
    ServLog = TRUE;
    memset(&nameBuff[0], '\0', 257 * sizeof(char));
    passedTime = 0xffff;

        /* running from CLI */
        hadError = FALSE;
        curArg = 1;
        do
        {
            cont = ((curArg < argc) && (NumCmds < MAX_CMDS));
            par = argv[curArg];
            curArg++;
            if (cont)
            {
                if (*par == '-')
                {
                    par += sizeof(char);
                }
                if (memcmp(par, "name=", 5) == 0)
                {
                    par += 5 * sizeof(char);
                    strncpy(&nameBuff[0], par, 256 * sizeof(char));
                    nameBuff[256] = '\0';
                }
                else if (memcmp(par, "test=", 5) == 0)
                {
                    par += 5 * sizeof(char);
                    if (*par)
                    {
                        DoDebug = getBool(par);
                    }
                }
                else if (memcmp("port=", par, 5 * sizeof(char)) == 0)
                {
                    strcpy(&PlayPort[0], (char *)par + (5 * sizeof(char)));
                }
                else if (memcmp(par, "update=", 7) == 0)
                {
                    par += 7 * sizeof(char);
                    if (*par && (NumCmds < MAX_CMDS))
                    {
                        if (getBool(par))
                        {
                            cmds[NumCmds] = 'u';
                            NumCmds++;
                        }
                    }
                }
                else if (memcmp(par, "miner=", 6) == 0)
                {
                    par += 6 * sizeof(char);
                    if (*par && (NumCmds < MAX_CMDS))
                    {
                        if (getBool(par))
                        {
                            cmds[NumCmds] = 'm';
                            NumCmds++;
                        }
                    }
                }
                else if (memcmp(par, "forcepower=", 11) == 0)
                {
                    par += 11 * sizeof(char);
                    if (*par && (NumCmds < MAX_CMDS))
                    {
                        if (getBool(par))
                        {
			    /* Do it this way to get around metaconfig problem */
                            cmds[NumCmds] = '\x50';
                            NumCmds++;
                        }
                    }
                }
                else if (memcmp(par, "power=", 6) == 0)
                {
                    par += 6 * sizeof(char);
                    if (*par && (NumCmds < MAX_CMDS))
                    {
                        if (getBool(par))
                        {
                            cmds[NumCmds] = 'p';
                            NumCmds++;
                        }
                    }
                }
                else if (memcmp(par, "publish", 7) == 0)
                {
                    cmds[NumCmds] = 's';
                    NumCmds++;
                }
                else if (memcmp(par, "flush=", 6) == 0)
                {
                    par += 6 * sizeof(char);
                    if (*par && (NumCmds < MAX_CMDS))
                    {
                        if (getBool(par))
                        {
                            cmds[NumCmds] = 'f';
                            NumCmds++;
                        }
                    }
                }
                else if (memcmp(par, "servlog=", 8) == 0)
                {
                    par += 8 * sizeof(char);
                    if (*par)
                    {
                        ServLog = getBool(par);
                    }
                }
                else
                {
                    while ((*par != '\0') && (NumCmds < MAX_CMDS))
                    {
                        switch (*par)
                        {
                            case 't':
                                DoDebug = !DoDebug;
                                break;
                            case 'p':
                            case '\x50': /* metaconfig problem */
                            case 'u':
                            case 'm':
                            case 'l':
                            case 'L':
                            case 'f':
                            case 's':
                                cmds[NumCmds] = *par;
                                NumCmds++;
                                break;
                            default:
                                hadError = TRUE;
                                break;
                        }
                        par += sizeof(char);
                    }
                }
            }
        } while (cont);
        if (NumCmds == 0)
        {
            NumCmds = 1;
            cmds[0] = '\x50'; /* metaconfig problem */
        }
        if (hadError)
        {
            useage();
        }
        if (!hadError)
        {
	    sprintf(fileDir, "%s/../ic.allow", FIFO_DIR);
	    /* If there is no ic.allow file refuse to do anything */
	    if ((allowFD = fopen(fileDir, "r")) == NULL)
	    {
		printf("Unable to open '%s' - aborting\n", fileDir);
	    }
	    else
	    {
#ifdef ZZZ
		userNamePtr = cuserid(NULL);
		if (userNamePtr != NULL)
		{
printf("-%d - Point 5.1\n", userNamePtr); /* ZZZ */
		    strncpy(userName, userNamePtr, 28 * sizeof(char));
printf("Point 5.1.1\n"); /* ZZZ */
		    strcat(userName, " ");
		}
		else
		{
#endif
		    myPwEnt = getpwuid(geteuid());
		    if (myPwEnt == NULL)
		    {
			fclose(allowFD);
			allowFD = NULL;
			printf("Unable to find information on uid %ud - "
			    "aborting\n", geteuid());
		    }
		    else
		    {
			strncpy(userName, myPwEnt->pw_name, 28 * sizeof(char));
			strcat(userName, " ");
		    }
#ifdef ZZZ
		}
#endif
		if (allowFD != NULL)
		{
		    checkCmds(allowFD, userName);
		    fclose(allowFD);
		    allowFD = NULL;
		    if (NumCmds > 0)
		    {
        		(void) runImpCtrl();
		    }
		}
	    }
        }
    exit(0);
}
