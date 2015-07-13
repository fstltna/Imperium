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
#include "Imperium.h"
#include "Request.h"
#include "../Library/Scan.h"
#include "../Library/ImpPrivate.h"

main()
{
	puts("----------------- Important structs");
	printf("sizeof(ImpState_t) = %d bytes\n", sizeof(ImpState_t));
	printf("sizeof(Request_t) = %d bytes\n", sizeof(Request_t));
	printf("REQUEST_PRIVATE_SIZE = %d bytes\n", REQUEST_PRIVATE_SIZE);
	puts("\n----------------- Constituant structs");
	printf("sizeof(World_t) = %d bytes\n", sizeof(World_t));
	printf("sizeof(Player_t) = %d bytes\n", sizeof(Player_t));
	printf("sizeof(Planet_t) = %d bytes\n", sizeof(Planet_t));
	printf("sizeof(Ship_t) = %d bytes\n", sizeof(Ship_t));
}

