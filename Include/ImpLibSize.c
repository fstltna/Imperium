#include "../config.h"

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <math.h>

#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif
#define BOOL unsigned char
#define BYTE signed char
#define UBYTE unsigned char
#define ULONG unsigned long
#define USHORT unsigned short

#include "ImpLib.h"

main()
{
	printf("sizeof(ImpState_t) = %d bytes\n", sizeof(ImpState_t));
	printf("sizeof(Request_t) = %d bytes\n", sizeof(Request_t));
	printf("REQUEST_PRIVATE_SIZE = %d bytes\n", REQUEST_PRIVATE_SIZE);
	printf("IMPERIUM_PRIVATE_SIZE = %d bytes\n", IMPERIUM_PRIVATE_SIZE);
	exit(0);
}

