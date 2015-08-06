/***************************************************************************

    mamecore.c

    Simple core functions that are defined in mamecore.h and which may
    need to be accessed by other MAME-related tools.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************/

#include "mamecore.h"
#include <ctype.h>

/*-------------------------------------------------
    mame_stricmp - case-insensitive string compare
-------------------------------------------------*/

int mame_stricmp(const char *s1, const char *s2)
{
	for (;;)
 	{
		int c1 = tolower(*s1++);
		int c2 = tolower(*s2++);
		if (c1 == 0 || c1 != c2)
			return c1 - c2;
 	}
}


/*-------------------------------------------------
    mame_strnicmp - case-insensitive string compare
-------------------------------------------------*/

int mame_strnicmp(const char *s1, const char *s2, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++)
 	{
		int c1 = tolower(*s1++);
		int c2 = tolower(*s2++);
		if (c1 == 0 || c1 != c2)
			return c1 - c2;
 	}

	return 0;
}


/*-------------------------------------------------
    mame_strdup - string duplication via malloc
-------------------------------------------------*/

char *mame_strdup(const char *str)
{
	char *cpy = NULL;
	if (str != NULL)
	{
		cpy = malloc(strlen(str) + 1);
		if (cpy != NULL)
			strcpy(cpy, str);
	}
	return cpy;
}
