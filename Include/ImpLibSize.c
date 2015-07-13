/*
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
 */

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
}

