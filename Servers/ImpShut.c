/*
 * Imperium
 *
 * $Id: ImpShut.c,v 1.4 2000/05/23 20:25:42 marisa Exp $
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
 * $Log: ImpShut.c,v $
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
 * Revision 4.0  2000/05/16 06:31:06  marisa
 * patch20: New check-in
 *
 * Revision 3.5.1.2  1997/03/14 07:13:50  marisag
 * patch20: N/A
 *
 * Revision 3.5.1.1  1997/03/11 20:06:05  marisag
 * patch20: Fix empty rev.
 *
 */

/*
 * ImpShut - simple program to request a shutdown of the Imperium server.
 */

#include "../config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>		/* System include files */
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
#include <pwd.h>

#include "../Include/Imperium.h" /* Imperium include files */
#include "../Include/Request.h"
#include "../impsec.h"

static const char rcsid[] = "$Id: ImpShut.c,v 1.4 2000/05/23 20:25:42 marisa Exp $";

static RequestType_t rqType;
static BOOL UseTest;
static char PlayPort[256];

/*
 * checkCmds - makes sure the given user is allowed to use the commands
 *             they are requesting
 */

void checkCmds(FILE *allowFD, const char *userName)
{
    BOOL foundName = FALSE;
    register USHORT nameLen;
    char lineBuf[256];

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
		rqType = rt_nop;
		return;
	    }
	}
	clearerr(allowFD);
	/* rewind the file back to the beginning */
	if (fseek(allowFD, 0L, SEEK_SET) != 0)
	{
	    puts("Error seeking to start of ic.allow");
	    rqType = rt_nop;
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
		    rqType = rt_nop;
		    return;
		}
	    }
	}
    }
    /* If we have STILL not found a name, they are out of luck */
    if (!foundName)
    {
	puts("You are not allowed to use this command");
	rqType = rt_nop;
	return;
    }
    /* Now do the simple checks */
    if ((lineBuf[nameLen] == '!') || (memcmp(&lineBuf[nameLen], "none",
	4 * sizeof(char)) == 0))
    {
	puts("Sorry, you are not allowed to use this command");
	rqType = rt_nop;
	return;
    }
    if ((lineBuf[nameLen] == '*') || (memcmp(&lineBuf[nameLen], "all",
	3 * sizeof(char)) == 0))
    {
	return;
    }
    switch(rqType)
    {
	case rt_shutDown:
	    if (strchr(&lineBuf[nameLen], 's') == NULL)
	    {
		rqType = rt_nop;
	    }
	    break;
	case rt_acOut:
	    if (strchr(&lineBuf[nameLen], 'p') == NULL)
	    {
		rqType = rt_nop;
	    }
	    break;
	case rt_acBack:
	    if (strchr(&lineBuf[nameLen], '\x50') == NULL)
	    {
		rqType = rt_nop;
	    }
	    break;
	case rt_battLow:
	    if (strchr(&lineBuf[nameLen], 'l') == NULL)
	    {
		rqType = rt_nop;
	    }
	    break;
	case rt_backStart:
	    if (strchr(&lineBuf[nameLen], 'b') == NULL)
	    {
		rqType = rt_nop;
	    }
	    break;
	case rt_backDone:
	    if (strchr(&lineBuf[nameLen], 'B') == NULL)
	    {
		rqType = rt_nop;
	    }
	    break;
	default:
	    rqType = rt_nop;
	    break;
    }
}

/*
 * doShut - all setup, try to shut the server and clients down.
 */

int doShut(void)
{
    int imperiumPort, myPort, nsel;
    register Request_t *rq;
    char *tmpNam;
    fd_set readPipe;
    struct timeval tv;
    int retCode = 0;
    char fileDir[256];
#ifdef HAVE_CRYPT
    char locSalt[3];
#endif

    /* Set up the path & file name of the server port */
    if (UseTest)
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
    imperiumPort = open(fileDir, O_WRONLY | O_NDELAY, 0);
    if (imperiumPort >= 0)
    {
            rq = calloc(1, sizeof(Request_t));
            if (rq != NULL)
            {
		tmpNam = tempnam(FIFO_DIR, "isht");
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
                    rq->rq_type = rqType;
                    rq->rq_clientId = 0; /* says new client */
		    /* Tell them our pipe name */
		    strcpy(&rq->rq_text[0], tmpNam);
		    /* Fill in the correct security code */
#ifdef HAVE_CRYPT
		    sprintf(&locSalt[0], "%2.2x", getpid() % 16);
		    sprintf(&rq->rq_u.ru_text[0], "%-.15s", crypt(IMP_SHUT_CODE,
			&locSalt[0]));
#else /* !HAVE_CRYPT */
		    strcpy(&rq->rq_u.ru_text[0], IMP_SHUT_CODE);
#endif
                    if (write(imperiumPort, (char *)rq, sizeof(Request_t)) < 0)
		    {
			puts("msgsnd to imperiumPort failed!");
		    }
		    else if (rqType == rt_backStart)
		    {
			tv.tv_sec = 60; /* time out after 60 secs */
			tv.tv_usec = 0;
			FD_ZERO(&readPipe);
			FD_SET(myPort, &readPipe);
			if ((nsel = select(myPort+1, &readPipe, 0, 0,
				(rqType == rt_backStart) ? 0 : &tv)) < 0)
			{
			    if (errno != EINTR)
			    {
				perror("select");
			    }
			    nsel = 0;
			}
			if (nsel == 0)
			{
				puts("Never received a response to my request");
			}
			else if ((nsel > 0) && FD_ISSET(myPort, &readPipe))
			{
			    if ((nsel = read(myPort, (char *)rq,
				sizeof(Request_t))) != sizeof(Request_t))
			    {
				printf("Only read %u bytes from pipe\n",
					nsel);
			    }
                            if ((rq->rq_specialFlags & ISF_BACKUP) == 0)
                            {
				retCode = 5;
                            }
			}
		    }
		    close(myPort);
		    (void)unlink(tmpNam);
		}
		if (tmpNam != NULL)
		{
		    free(tmpNam);
		}
                free(rq);
            }
	    else
	    {
		puts("Unable to allocate memory for Request_t struct!");
	    }
	    close(imperiumPort);
    }
    else
    {
	puts("Unable to find port for Imperium server!");
    }
    return retCode;
}

/*
 * useage - issue a useage error line.
 */

void useage(void)
{
    printf("ImpShut V%s.pl%d\n\n", IMP_BASE_REV,
	0);
    puts("Useage: ImpShut [-t] [-s | -p | -\x50 | -l | -b | -B] [port=<port name>]");
    puts("    -t = Use test port");
    puts("    -s = normal shutdown (default)");
    puts("    -p = AC power out");
    puts("    -\x50 = AC power back");
    puts("    -l = Low battery - exit immediately");
    puts("    -b = Request back up locking");
    puts("    -B = Release back up locking");
}

/*
 * main - primary entry point
 */

int main(int argc, char *argv[])
{
    register char *par;
    BOOL hadError;
    int retCode = 0;
    USHORT curArg;
    char fileDir[256], userName[30], *userNamePtr;
    FILE *allowFD;
    struct passwd *myPwEnt;

    hadError = FALSE;
    UseTest = FALSE;
    rqType = rt_shutDown;
    PlayPort[0] = '\0';

        if (argc > 1)
        {
            curArg = 1;
            while (curArg < argc)
            {
                par = argv[curArg];
                if (*par == '-')
                {
                    par += sizeof(char);
                }
		if (*par == 't')
		{
		    UseTest = TRUE;
		}
                else if (memcmp("port=", par, 5 * sizeof(char)) == 0)
                {
                    strcpy(&PlayPort[0], (char *)(par + (5 * sizeof(char))));
                }
                else
                {
                    switch(*par)
                    {
                        case 'p':
                            if (rqType != rt_shutDown)
                            {
                                hadError = TRUE;
                            }
                            rqType = rt_acOut;
                            break;
                        case '\x50':
			    /* Do it this way to get around metaconfig problem */
                            if (rqType != rt_shutDown)
                            {
                                hadError = TRUE;
                            }
                            rqType = rt_acBack;
                            break;
                        case 'l':
                            if (rqType != rt_shutDown)
                            {
                                hadError = TRUE;
                            }
                            rqType = rt_battLow;
                            break;
                        case 'b':
                            if (rqType != rt_shutDown)
                            {
                                hadError = TRUE;
                            }
                            rqType = rt_backStart;
                            break;
                        case 'B':
                            if (rqType != rt_shutDown)
                            {
                                hadError = TRUE;
                            }
                            rqType = rt_backDone;
                            break;
			case 's':
			    break;
                        default:
                            hadError = TRUE;
                            break;
                    }
                }
                curArg++;
            }
        }
        if (hadError)
        {
            useage();
        }
        else
        {
	    sprintf(fileDir, "%s/../is.allow", FIFO_DIR);
	    /* if there is no is.allow refuse to do anything */
	    if ((allowFD = fopen(fileDir, "r")) == NULL)
	    {
		printf("Unable to open '%s' - aborting\n", fileDir);
	    }
	    else
	    {
		userNamePtr = cuserid(NULL);
		if (userNamePtr != NULL)
		{
		    strncpy(userName, userNamePtr, 28 * sizeof(char));
		    strcat(userName, " ");
		}
		else
		{
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
		}
		if (allowFD != NULL)
		{
		    checkCmds(allowFD, userName);
		    fclose(allowFD);
		    allowFD = NULL;
		    if (rqType != rt_nop)
		    {
			retCode = doShut();
		    }
		}
	    }
	}
    exit(retCode);
}
