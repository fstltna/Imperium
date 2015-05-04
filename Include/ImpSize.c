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

