/*
 * Imperium
 *
 * $Id: TelImp.c,v 1.8 2000/05/26 00:47:45 marisa Exp $
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
 * $Log: TelImp.c,v $
 * Revision 1.8  2000/05/26 00:47:45  marisa
 * Same..
 *
 * Revision 1.7  2000/05/26 00:46:28  marisa
 * Now doesn't echo back chars
 *
 * Revision 1.6  2000/05/25 23:45:49  marisa
 * Add check for termios.h
 *
 * Revision 1.5  2000/05/24 19:30:27  marisa
 * Add ignoring of spurious ECHILD signal codes
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
 * Revision 4.0  2000/05/16 06:31:27  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.6  1997/09/03 18:59:41  marisag
 * patch20: Check in changes before distribution
 *
 * Revision 3.5.1.5  1997/04/07 03:39:04  marisag
 * patch20: Seems to work now - but still need to find how to kick over to
 * patch20: character mode rather than line mode.
 *
 * Revision 3.5.1.4  1997/04/07 00:12:56  marisag
 * patch20: Add in locSalt
 *
 * Revision 3.5.1.3  1997/04/07 00:10:33  marisag
 * patch20: Added security code checking from impsec.h
 *
 * Revision 3.5.1.2  1997/04/06 23:55:30  marisag
 * patch20: Fix bug with select returning 0 on timeout.
 *
 * Revision 3.5.1.1  1997/04/06 23:43:18  marisag
 * patch20: Was not putting /server/ in the FIFO path.
 *
 * Revision 3.5  1997/04/06 19:27:57  marisag
 * First checkin
 *
 *
 */

#include "../config.h"

#ifndef HAVE_SELECT
BOGUS - TelImp not supported on this machine due to missing select() call!
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <signum.h>
#include <signal.h>

#include <sys/types.h>
#include <rpc/types.h>
#define _XOPEN_SOURCE
#include <unistd.h>
#include <crypt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>

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

#ifndef LINT
static char const rcsid[] = "$Id: TelImp.c,v 1.8 2000/05/26 00:47:45 marisa Exp $";
#endif

#define REVISION    "0.1"


#define LOG_SIZE        (512 * 10)

static int
    ImperiumPort,
    myPort,
    GameSocket;

ImpState_t
    *ISt;

ImpState_t
    *IS;

static BOOL
    HadBreak,		/* Did we get a BREAK signal ? */
    PswdMode,		/* Are we in "password mode" ? */
    DoDebug;		/* Is debugging mode turned on ? */

static char
    PlayPort[257],
    UsePort[257];

static USHORT
    passedTime;

#define PORTNUM 3458

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

static int _tty_ch=2; /* XXX for this to work right it needs to be the telnet sessions STDOUT, not the starting shell's! */
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
    Request_t *rq;
    fd_set readPipe;
    struct timeval tv;

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
    Request_t *rq;
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
 * write_data - writes data to the socket...
 */
int write_data(int sock, char *buf, int len)
{
	int bcount; /* Bytes written */
	int bw;     /* Bytes written this pass */

	bcount = 0;
	bw=0;
	while (bcount < len)
	{
		if ((bw=write(sock, buf, len-bcount)) > 0)
		{
			bcount += bw;
			buf += bw;
		}
		if (bw < 0)
		{
			return(-1);
		}
	}
	return(bcount);
}

/*
 * write_str - writes a C string to the game socket
 */
void write_str(char *buf)
{
    if (write_data(GameSocket, buf, strlen(buf) * sizeof(char)) == -1)
    {
	puts("*** error returned from write_data");
    }
}

/*
 * writeUser - routine to allow the Imperium library to write to the user.
 */

void writeUser(void)
{
/*    (void) fwrite(&IS->is_textOut[0], sizeof(char), IS->is_textOutPos, stdout); */
    (void) write_data(GameSocket, &IS->is_textOut[0], IS->is_textOutPos * sizeof(char));
}

/*
 * timedInput - Reads in a line of text, with two possible timeout values.
 *              The first will time out if the "cursor" is on the first
 *              character of the "line" for more than the given amount of
 *              time.
 *              The other is the amount of time before timing out at other
 *              positions on the line.
 *              All times are given in seconds.
 */
void timedInput(unsigned int blTime, unsigned int maxTime)
{
    char *curPos;
    fd_set readPipe;
    struct timeval tv;
    int bufPos = 0, nsel, bufSize, sawEsc=0;

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
	FD_SET(GameSocket, &readPipe);
	/* Wait for the timeout value or a character */
	if ((nsel = select(GameSocket+1, &readPipe, (fd_set *)0, (fd_set *)0, (tv.tv_sec != 0) ? &tv : 0)) <= 0)
	{
	    IS->is_argShort = 0; /* Timed out */
	    IS->is_textIn[0] = '\0';
	    /* See if we got an error, or just timed out */
	    if (errno != 0)
	    {
		/* Why is EFAULT returned?? */
		/* And ECHILD too? */
	        if ((errno != EINTR) && (errno != ENOENT) && (errno != EFAULT) && (errno != ECHILD) && (errno != 22) && (errno != 9))
	        {
		    /* Got a booboo */
		    printf("timedInput() got bad val from select() %d\n", errno);
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
	if ((nsel > 0) && FD_ISSET(GameSocket, &readPipe))
	{
	    if (read(GameSocket, curPos, 1) != 1)
	    {
#ifdef DEBUG
		puts("BIG error reading from keyboard");
#endif
		IS->is_textIn[0] = '\0';
		IS->is_argBool = FALSE;
		return;
	    }
	    if (sawEsc > 0)
	    {
		sawEsc--;
		continue;
	    }
	    /* Look for the two critical characters */
	    if (*curPos == '\n')
	    {
		/* ZZZZ Isn't needed via telnet? */
		/*write_data(GameSocket, "\n", sizeof(char));*/
		IS->is_textIn[bufPos+1] = '\0';
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
		    write_data(GameSocket, "\b \b", 3 * sizeof(char));
		    curPos--;
		    bufPos--;
		}
	    }
	    else
	    {
		if (*curPos == '\xff')
		{
			sawEsc=2;
		}
		else
		{
		    /* Ignore CR characters  & NULLs */
	            if ((*curPos != '\x0d') && (*curPos != '\x00'))
	            {
			curPos++;
			bufPos++;
	            }
		}
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
    /* Tell the library we don't have one... */
    ISt->is_argShort = 3;
    return;
}

/*
 * userLog - routine to allow the user to do logging.
 */

void userLog(void)
{
    ISt->is_argBool = FALSE;
}

/*
 * runImperium - start up the Imperium-specific stuff.
 */

void runImperium(void)
{
    char fileDir[256], *tmpNam;
    char *ErrStr, ErrBuff[160];
#ifdef HAVE_CRYPT
    char locSalt[3];
#endif


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
	    /* Attempt to turn off Telnet line processing */
	    /*write_str("\xff\xfb\x01\xff\xfb\x03\xff\xfd\xf3");*/
	    tmpNam = tempnam(FIFO_DIR, "telimp");
	    if (tmpNam == NULL)
	    {
		ErrStr="Unable to allocate memory for temp name\n";
		write_str(ErrStr);
		printf(ErrStr);
	    }
	    else if (mknod(tmpNam, S_IFIFO | 0622, 0) == -1)
	    {
		sprintf(ErrBuff, "Unable to create named pipe %s\n", tmpNam);
		write_str(ErrBuff);
		printf(ErrBuff);
	    }
#ifdef BROKEN_MKNOD
	    else if (chmod(tmpNam, 0622) != 0)
	    {
		sprintf(ErrBuff, "Unable to change mode for named pipe %s\n", tmpNam);
		write_str(ErrBuff);
		printf(ErrBuff);
	    }
#endif
#ifdef BROKEN_PIPE
	    else if ((myPort = open(tmpNam, O_RDWR | O_NDELAY)) == -1)
#else
	    else if ((myPort = open(tmpNam, O_RDONLY | O_NDELAY)) == -1)
#endif
	    {
		(void)unlink(tmpNam);
		sprintf(ErrBuff, "Unable to open named pipe %s\n", tmpNam);
		write_str(ErrBuff);
		printf(ErrBuff);
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

                    ISt->is_request.rq_type = rt_startClient;
                    strcpy(&ISt->is_request.rq_text[0], tmpNam);
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
                        strcpy(&ISt->is_request.rq_private[2], "Telnet client ");
                        strcat(&ISt->is_request.rq_private[2], REVISION);
                        strcat(&ISt->is_request.rq_private[2], " started.");
                        ISt->is_request.rq_type = rt_log;
                        serverRequest();

                        /* clear out buffer and set up no outside time limit */
                        memset(&ISt->is_textIn[0], '\0', INPUT_BUFFER_SIZE *
                            sizeof(char));
                        ISt->is_argShort = passedTime;

                        Imperium(ISt);

                        strcpy(&ISt->is_request.rq_private[2], "Telnet client ");
                        strcat(&ISt->is_request.rq_private[2], REVISION);
                        strcat(&ISt->is_request.rq_private[2], " terminated.");
                        ISt->is_request.rq_type = rt_log;
                        serverRequest();
                        ISt->is_request.rq_type = rt_stopClient;
                        serverRequest();
		    }

                    ImpFree(ISt);
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
            ErrStr="*** can't allocate Imperium state structure.\n";
	    write_str(ErrStr);
            printf(ErrStr);
        }
	close(ImperiumPort);
    }
    else
    {
        ErrStr="*** can't find Imperium port. Is server running?\n";
	write_str(ErrStr);
	printf(ErrStr);
    }
    return;
}

void splatZombie(void)
{
	union wait wstatus;

	while(wait3(&wstatus, WNOHANG, NULL) > 0)
	{
	}
	signal(SIGCHLD, splatZombie); /* Needed for SYSV */
}

/*
 * useage - print a simple CLI useage message.
 */

void useage(void)
{
    puts("Useage is: TelImp [-a] [-t] [m<time>]\n"
        "    -a = non-ANSI terminal\n    -t = use test port\n"
        "    -m = maximum play time");
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

int establish_sock(unsigned short portnum)
{
	char myname[MAXHOSTNAMELEN+1];
	int s;
	struct sockaddr_in sa;
	struct hostent *hp;

	bzero(&sa, sizeof(struct sockaddr_in)); /* Clear address */
	gethostname(myname, MAXHOSTNAMELEN);
	hp=gethostbyname(myname); /* get our address info */
	if (hp == NULL)
	{
		return(-1);
	}
	sa.sin_family=hp->h_addrtype; /* Our host address */
	sa.sin_port=htons(portnum);   /* Our port number */
	if ((s=socket(AF_INET, SOCK_STREAM, 0)) < 0) /* Create the socket */
	{
		return(-1);
	}
	if (bind(s, &sa, sizeof(sa)) < 0)
	{
		close(s);
		return(-1);
	}
	listen(s, 5);	/* Max number of queued requests */
	return(s);
}

int get_connection(int s)
{
	struct sockaddr_in isa; /* Sock addr */
	int i;                  /* size of addr */
	int t;                  /* socket of connection */

	i=sizeof(isa);
	getsockname(s, &isa, &i); /* For accept() */

	if ((t=accept(s, &isa, &i)) < 0) /* Accept connection one exists */
	{
		return(-1);
	}
	return(t);
}

int read_data(int sock, char *buf, int len)
{
	int bcount; /* Bytes read */
	int br;     /* Bytes read this pass */

	bcount = 0;
	br=0;
	while (bcount < len)
	{
		if ((br=read(sock, buf, len-bcount)) > 0)
		{
			bcount += br;
			buf += br;
		}
		if (br < 0)
		{
			return(-1);
		}
	}
	return(bcount);
}

/*
 * runNetImperium - This handles establishing the network connection
 */
void runNetImperium(void)
{
	int s;

	if ((s=establish_sock(PORTNUM)) < 0)
	{
		perror("establish_sock");
		return;
	}
	signal(SIGCHLD, splatZombie);

	while (-1)
	{
		if ((GameSocket=get_connection(s)) < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			perror("accept");
			return;
		}
		switch(fork())
		{
			case -1:
				perror("fork");
				close(s);
				close(GameSocket);
				return;
			case 0:	/* child */
		_tty_ch=GameSocket;
    savetty();
    noecho();
    crmode();

				(void) runImperium();
				close(GameSocket);
				return;
			default:
				close(GameSocket);
				continue;
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

    /* No output buffering at all */
    setvbuf(stdout, NULL, _IONBF, 0);
    /* initialize our globals */
    DoDebug = FALSE;
    PswdMode = FALSE;
    HadBreak = FALSE;
    ISt = NULL;
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
                else if (memcmp("port=", par, 5 * sizeof(char)) == 0)
                {
                    strncpy(&PlayPort[0], par + (5 * sizeof(char)), 250 *
                        sizeof(char));
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
            runNetImperium();
        }
    cleanup(0);
    exit(0);	/* Never gets here */
}
