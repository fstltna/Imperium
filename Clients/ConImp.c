/*
 * Imperium
 *
 * $Id: ConImp.c,v 1.6 2000/05/25 23:45:49 marisa Exp $
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
 * $Log: ConImp.c,v $
 * Revision 1.6  2000/05/25 23:45:49  marisa
 * Add check for termios.h
 *
 * Revision 1.5  2000/05/25 18:21:18  marisa
 * Fix more T/F issues
 *
 * Revision 1.4  2000/05/23 20:24:16  marisa
 * Added inclusion of crypt.h else Linux won't see the prototype for crypt()
 * Fix %lu as %u in printf()'s
 *
 * Revision 1.3  2000/05/18 06:41:40  marisa
 * More autoconf
 *
 * Revision 1.2  2000/05/18 06:09:39  marisa
 * Changes for crypt()
 *
 * Revision 1.1.1.1  2000/05/17 19:15:04  marisa
 * First CVS checkin
 *
 * Revision 4.0.1.4  2000/05/16 06:29:39  marisa
 * patch20: New check-in
 *
 * Revision 4.0.1.3  2000/05/16 06:28:45  marisa
 * patch20: New check-in
 *
 * Revision 4.0.1.2  2000/05/16 06:26:53  marisa
 * patch20: New check-in
 *
 * Revision 4.0.1.1  2000/05/16 06:21:22  marisa
 * patch20: New checkin
 *
 * Revision 3.5.1.5  1997/04/07 00:12:55  marisag
 * patch20: Add in locSalt
 *
 * Revision 3.5.1.4  1997/04/07 00:10:35  marisag
 * patch20: Added security code checking from impsec.h
 *
 * Revision 3.5.1.3  1997/04/06 23:55:27  marisag
 * patch20: Fix bug with select returning 0 on timeout.
 *
 * Revision 3.5.1.2  1997/04/06 23:43:14  marisag
 * patch20: Was not putting /server/ in the FIFO path.
 *
 * Revision 3.5.1.1  1997/03/14 07:31:48  marisag
 * patch20: N/A
 *
 * Revision 3.5  1997/03/11 19:23:26  marisag
 * First branch by me
 *
 */

#include "../config.h"

#ifndef HAVE_SELECT
BOGUS - ConImp not supported on this machine due to missing select() call!
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <bits/signum.h>
#include <signal.h>

#include <sys/types.h>
#define _XOPEN_SOURCE
#include <unistd.h>
#include <crypt.h>
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
#include <setjmp.h>

#define TRUE (1)
#define FALSE (0)
#define BOOL unsigned char
#define BYTE signed char
#define UBYTE unsigned char
#define ULONG unsigned long
#define USHORT unsigned short

#include "../Include/ImpLib.h"
#include "../impsec.h"

static char const rcsid[] = "$Id: ConImp.c,v 1.6 2000/05/25 23:45:49 marisa Exp $";

#define REVISION    "0.3w"


#define LOG_SIZE        (512 * 10)

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
    HadBreak,		/* Did we get a BREAK signal ? */
    HaveExtEdit,	/* Do we have an external editor enabled? */
    PswdMode,		/* Are we in "password mode" ? */
    LoggingAllowed,	/* Are players allowed to use cmd_log ? */
    DoDebug;		/* Is debugging mode turned on ? */

static char
    ExtEdit[257],
    nameBuff[257],
    PlayPort[257],
    UsePort[257],
    LogBuffer[LOG_SIZE];

static USHORT
    passedTime;

static ULONG
    LogPos;

/*
 * --------------- macros from TRN ---------------------------------------
 */
/* stuff wanted by terminal mode diddling routines */

#ifdef HAVE_TERMIO_H
#include <termio.h>
static struct termio _tty, _oldtty;
#else
# ifdef HAVE_TERMIOS_H
#include <termios.h>
static struct termios _tty, _oldtty;
# else
static struct sgttyb _tty;
static int _res_flg=0;
# endif
#endif

static int _tty_ch=2;
static BOOL bizarre=FALSE;  /* do we need to restore terminal? */

/* terminal mode diddling routines */

#ifdef HAVE_TERMIO_H

#define crmode() ((bizarre=TRUE),_tty.c_lflag &=~ICANON,_tty.c_cc[VMIN] = 1,ioctl(_tty_ch,TCSETAF,&_tty))
#define nocrmode() ((bizarre=TRUE),_tty.c_lflag |= ICANON,stty(_tty_ch,&_tty))
#define echo()	 ((bizarre=TRUE),_tty.c_lflag |= ECHO, ioctl(_tty_ch, TCSETA, &_tty))
#define noecho() ((bizarre=TRUE),_tty.c_lflag &=~ECHO, ioctl(_tty_ch, TCSETA, &_tty))
#define	savetty() (ioctl(_tty_ch, TCGETA, &_oldtty),ioctl(_tty_ch, TCGETA, &_tty))
#define	resetty() ((bizarre=FALSE),ioctl(_tty_ch, TCSETAF, &_oldtty))

#else /* !HAVE_TERMIO_H */
# ifdef HAVE_TERMIOS_H

#define crmode() ((bizarre=TRUE), _tty.c_lflag &= ~ICANON,_tty.c_cc[VMIN]=1,tcsetattr(_tty_ch, TCSAFLUSH, &_tty))
#define nocrmode() ((bizarre=TRUE),_tty.c_lflag |= ICANON,tcsetattr(_tty_ch, TCSAFLUSH,&_tty))
#define echo()	 ((bizarre=TRUE),_tty.c_lflag |= ECHO, tcsetattr(_tty_ch, TCSAFLUSH, &_tty))
#define noecho() ((bizarre=TRUE),_tty.c_lflag &=~ECHO, tcsetattr(_tty_ch, TCSAFLUSH, &_tty))
#define	savetty() (tcgetattr(_tty_ch, &_oldtty),tcgetattr(_tty_ch, &_tty))
#define	resetty() ((bizarre=FALSE),tcsetattr(_tty_ch, TCSAFLUSH, &_oldtty))

# else /* !HAVE_TERMIOS_H */

#define raw()	 ((bizarre=TRUE),_tty.sg_flags|=RAW, stty(_tty_ch,&_tty))
#define noraw()	 ((bizarre=TRUE),_tty.sg_flags&=~RAW,stty(_tty_ch,&_tty))
#define crmode() ((bizarre=TRUE),_tty.sg_flags |= CBREAK, stty(_tty_ch,&_tty))
#define nocrmode() ((bizarre=TRUE),_tty.sg_flags &= ~CBREAK,stty(_tty_ch,&_tty))
#define echo()	 ((bizarre=TRUE),_tty.sg_flags |= ECHO, stty(_tty_ch, &_tty))
#define noecho() ((bizarre=TRUE),_tty.sg_flags &= ~ECHO, stty(_tty_ch, &_tty))
#define	savetty() (gtty(_tty_ch, &_tty), _res_flg = _tty.sg_flags)
#define	resetty() ((bizarre=FALSE),_tty.sg_flags = _res_flg, stty(_tty_ch, &_tty))
# endif /* HAVE_TERMIOS_H */

#endif /* HAVE_TERMIO_H */

/*
 Usage:
    savetty();
    noecho();
    crmode();
    .... code
    nocrmode();
    echo()
    resetty();
*/
/*
 * --------------------------------------------------------------------
 */

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

void cleanup(int retVal)
{
    nocrmode();
    echo();
    resetty();
    exit(retVal);
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
    cleanup(10);
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
 * timedInput - Reads in a line of text, with two possible timeout values.
 *              The first will time out of the "cursor" is on the first
 *              character of the "line" for more than the given amount of
 *              time.
 *              The other is the amount of time before timing out at other
 *              positions on the line.
 *              All times are given in seconds.
 */
void timedInput(unsigned int blTime, unsigned int maxTime)
{
    register char *curPos;
    fd_set readPipe;
    struct timeval tv;
    int bufPos = 0, nsel, bufSize;

    IS->is_textInPos = &IS->is_textIn[0];

    /* First make sure the buffer will be blanked out */
    curPos = &IS->is_textIn[0];
    *curPos = '\0';

    bufSize = INPUT_BUFFER_SIZE - 1;
    curPos[bufSize] = '\0';
    /* Now it will be safe to read in bufSize characters and still have it */
    /* be NULL terminated */

    IS->is_argShort = 0xFFFF; /* The user entered the line by default */
    while(bufPos < bufSize)
    {
	/* fill in the timeout value */
	tv.tv_sec = ((bufPos == 0) ? blTime : maxTime);
	tv.tv_usec = 0;
	FD_ZERO(&readPipe);
	FD_SET(0, &readPipe);
	/* Wait for the timeout value or a character */
	if ((nsel = select(1, &readPipe, 0, 0, (tv.tv_sec != 0) ? &tv : 0)) <= 0)
	{
	    IS->is_argShort = 0; /* Timed out */
	    IS->is_textIn[0] = '\0';
	    /* See if we got an error, or just timed out */
	    if (errno != 0)
	    {
	        if ((errno != EINTR) && (errno != ENOENT))
	        {
		    /* Got a booboo */
		    printf("timedInput() got bad val from select(): %d\n", errno);
		    IS->is_argBool = FALSE;
		    return;
	        }
	    }
	    /* Return the apropriate code for where we timed out */
	    IS->is_argBool = ((bufPos > 0) || (blTime == maxTime)) ? FALSE
		: TRUE;
	    return;
	}
	/* See if we have something to read in */
	if ((nsel > 0) && FD_ISSET(0, &readPipe))
	{
	    if (read(0, curPos, 1) != 1)
	    {
		puts("BIG error reading from keyboard");
		IS->is_textIn[0] = '\0';
		IS->is_argBool = FALSE;
		return;
	    }
	    /* Look for the two critical characters */
	    if (*curPos == '\n')
	    {
		putchar('\n');
		IS->is_textIn[bufPos+1] = '\0';
		/* Log the string, if doing logging */
		if (LogFd != NULL)
		{
			writeLog(&IS->is_textIn[0], bufPos + 1);
		}
		/* strip out trailing "newline" character */
		*curPos = '\0';
		IS->is_argBool = TRUE;
		return;
	    }
	    /* Linux seems to send char 127 for BS */
	    if ((*curPos == '\b') || (*curPos == 127))
	    {
		*curPos = '\0';
		if (bufPos > 0)
		{
		    printf("\b \b");
		    curPos--;
		    bufPos--;
		}
	    }
	    else
	    {
		if (PswdMode)
		{
		    putchar('*');
		}
		else
		{
		    putchar(*curPos);
		}
		curPos++;
		bufPos++;
	    }
	}
    }
    IS->is_argBool = TRUE;
    return;
}

/*
 * readUser - routine to allow the Imperium library to read from the user.
 */

void readUser(void)
{
    /* Allow 15 minutes of idle time before disconnecting */
    timedInput(900, 900);
}

/*
 * timedReadUser - routine to allow the Imperium library to read from the
 *          user with a timeout on the first character
 */

void timedReadUser(void)
{
    /* Allow 3 seconds on the first char, 15 minutes on the others */
    timedInput(3, 900);
}

/*
 * echoOff - disable echo on the user input.
 */

void echoOff(void)
{
	PswdMode = TRUE;
}

/*
 * echoOn - re-enable echo on the user input.
 */

void echoOn(void)
{
	PswdMode = FALSE;
}

void breakHandler(int whichSig)
{
    HadBreak = TRUE;
    signal(SIGINT, breakHandler); /* Needed for SYSV */
}

/*
 * gotControlC - return true if user has typed a CNTL-C.
 */

void gotControlC(void)
{
    if (HadBreak)
    {
	puts("Interupt!!!");
	ISt->is_argBool = TRUE;
	HadBreak = FALSE;
    }
    else
    {
	ISt->is_argBool = FALSE;
    }
}

/*
 * locSleep - sleep for the given number of seconds.
 */

void locSleep(void)
{
    sleep(ISt->is_argShort);
}

/*
 * doExtEdit - try and use an external editor, if allowed
 */

void doExtEdit(void)
{
    ULONG maxLen;
    char *bufLoc, *tmpName;
    FILE *tmpFile;
    int retVal;
    char cmdName[512];

    /* See if we have enabled this feature */
    if (!HaveExtEdit)
    {
	/* Tell the library we don't have one... */
	ISt->is_argShort = 3;
	return;
    }
    /* Store the passed arguments */
    maxLen = ISt->is_argLong;
    bufLoc = ISt->is_argPoint;
    /* Try to get a temporary file name */
    tmpName = tmpnam(NULL);
    if (tmpName == NULL)
    {
	/* Tell the library we don't have an external editor */
	ISt->is_argShort = 3;
	return;
    }
    /* Build up the command line to execute */
    sprintf(&cmdName[0], "%s %s\n", &ExtEdit[0], tmpName);
    /* Execute the command */
    retVal = system(&cmdName[0]);
    /* If you know of some way to tell if a command failed by the return
     * code, feel free to implement it here
     */
    /* Attempt to open the file the editor should have created */
    tmpFile = fopen(tmpName, "rb");
    /* Did the open succeed? */
    if (tmpFile == NULL)
    {
	/* Nope */
	/* Do this just to be safe */
	(void)unlink(tmpName);
	/* Tell the library we failed */
	ISt->is_argShort = 0;
	return;
    }
    /* Now attempt to read in the first maxLen characters, if there are
     * that many in the file
     */
    retVal = fread(bufLoc, sizeof(char), maxLen, tmpFile);
    /* Close the file */
    fclose(tmpFile);
    /* Delete it */
    (void)unlink(tmpName);
    /* If we read NO characters, then tell them we failed (or the user
     * aborted the edit)
     */
    if (retVal == 0)
    {
	/* Tell the library we failed */
	ISt->is_argShort = 0;
	return;
    }
    /* Code 1 means success */
    ISt->is_argShort = 1;
    /* Return the number of bytes we were able to read */
    ISt->is_argLong = retVal;
}

/*
 * userLog - routine to allow the user to do logging.
 */

void userLog(void)
{
    register char *fileName;

    if (!LoggingAllowed)
    {
	ISt->is_argBool = FALSE;
	return;
    }
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
 * runImperium - start up the Imperium-specific stuff.
 */

BOOL runImperium(void)
{
    BOOL ok;
    char fileDir[256], *tmpNam;
#ifdef HAVE_CRYPT
    char locSalt[3];
#endif

    ok = FALSE;
    /* First we attempt to come up with the port name to use */
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
    /* Try to open the port */
    ImperiumPort = open(fileDir, O_WRONLY | O_NDELAY, 0);
    if (ImperiumPort >= 0)
    {
        ISt = ImpAlloc();
        if (ISt != NULL)
        {
	    tmpNam = tempnam(FIFO_DIR, "icon");
	    if (tmpNam == NULL)
	    {
		puts("Unable to allocate memory for temp name");
	    }
	    else if (mknod(tmpNam, S_IFIFO | 0622, 0) == -1)
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
                        crypt(CON_IMP_CODE, &locSalt[0]));
#else /* !HAVE_CRYPT */
                    strcpy(&ISt->is_request.rq_private[2], CON_IMP_CODE);
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
                        strcpy(&ISt->is_request.rq_private[2], "Local client ");
                        strcat(&ISt->is_request.rq_private[2], REVISION);
                        strcat(&ISt->is_request.rq_private[2], " started.");
                        ISt->is_request.rq_type = rt_log;
                        serverRequest();

                        /* clear out buffer and set up no outside time limit */
                        memset(&ISt->is_textIn[0], '\0', INPUT_BUFFER_SIZE *
                            sizeof(char));
                        strcpy(&ISt->is_textIn[0], &nameBuff[0]);
                        ISt->is_argShort = passedTime;

                        Imperium(ISt);

                        if (LogFd)
                        {
                            logFlush();
                            fclose(LogFd);
                        }
                        strcpy(&ISt->is_request.rq_private[2], "Local client ");
                        strcat(&ISt->is_request.rq_private[2], REVISION);
                        strcat(&ISt->is_request.rq_private[2], " terminated.");
                        ISt->is_request.rq_type = rt_log;
                        serverRequest();
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
            puts("*** can't allocate Imperium state structure.\n");
        }
	close(ImperiumPort);
    }
    else
    {
        printf("*** can't find Imperium port %s. Is server running?", fileDir);
    }
    return ok;
}

/*
 * useage - print a simple CLI useage message.
 */

void useage(void)
{
    puts("Useage is: ConImp [-a] [-t] [n<name>] [m<time>]\n"
        "    -a = non-ANSI terminal\n    -t = use test port\n"
        "    -n = force user name        -m = maximum play time\n"
	"    -e<editor> = allow external editor");
}

/*
 * getBool - interpret a YES/NO or ON/OFF from a string.
 */

BOOL getBool(char *st)
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
 * main - start everything up.
 */

int main(int argc, char *argv[])
{
    char *par;
    BOOL hadError, cont;
    USHORT curArg;

    /* No output buffering at all */
    setvbuf(stdout, NULL, _IONBF, 0);
    /* initialize our globals */
    DoDebug = FALSE;
    LoggingAllowed = FALSE;
    PswdMode = FALSE;
    HaveExtEdit = FALSE;
    HadBreak = FALSE;
    ISt = NULL;
    memset(&nameBuff[0], '\0', 257 * sizeof(char));
    PlayPort[0]='\0';
    UsePort[0]='\0';
    passedTime = 0xffff;

    savetty();
    noecho();
    crmode();

        /* running from CLI */
        hadError = FALSE;
        curArg = 1;
        do
        {
            cont = (curArg < argc);
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
                else if (memcmp(par, "time=", 5) == 0)
                {
                    par += 5 * sizeof(char);
		    if (*par != '\0')
		    {
                	passedTime = (USHORT)atoi(par);
		    }
                }
                else if (memcmp(par, "test=", 5) == 0)
                {
                    par += 5 * sizeof(char);
                    if (*par != '\0')
                    {
                        DoDebug = getBool(par);
                    }
                }
                else if (memcmp(par, "logcmd=", 7) == 0)
                {
                    par += 7 * sizeof(char);
                    if (*par != '\0')
                    {
                        LoggingAllowed = getBool(par);
                    }
                }
                else if (memcmp("port=", par, 5 * sizeof(char)) == 0)
                {
                    strncpy(&PlayPort[0], par + (5 * sizeof(char)), 250 *
                        sizeof(char));
                }
                else if (memcmp("editor=", par, 7 * sizeof(char)) == 0)
                {
                    strncpy(&ExtEdit[0], par + (7 * sizeof(char)), 250 *
                        sizeof(char));
		    if (ExtEdit[0] != '\0')
		    {
			HaveExtEdit = TRUE;
		    }
                }
                else
                {
                    while (*par != '\0')
                    {
                        switch (*par)
                        {
                            case 't':
                            case 'T':
                                DoDebug = TRUE;
                                break;
                            case 'n':
                            case 'N':
                                par += sizeof(char);
                                strncpy(&nameBuff[0], par, 250 *
                                    sizeof(char));
                                nameBuff[256] = '\0';
                                par = "\0\0";
                                break;
                            case 'e':
                            case 'E':
                                par += sizeof(char);
                                strncpy(&ExtEdit[0], par, 250 *
                                    sizeof(char));
				if (ExtEdit[0] != '\0')
				{
                                    ExtEdit[251] = '\0';
				    HaveExtEdit = TRUE;
				}
                                par = "\0\0";
                                break;
                            case 'm':
                            case 'M':
                                par += sizeof(char);
				if (*par != '\0')
				{
				    passedTime = (USHORT)atoi(par);
				}
                                par = "\0\0";
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
        if (hadError)
        {
            useage();
        }
        if (!hadError)
        {
	    signal(SIGINT, breakHandler);
            (void) runImperium();
        }
    cleanup(0);
    exit(0);	/* Never gets here */
}
