/*****************************************************************************
 *
 *	 mkimage.c
 *	 Make CP/M image file(s) for different formats
 *	 This is intended to be used with (and uses) M.E.S.S.
 *	 Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

/* The Win32 port requires this constant for variable arg routines and main */
#ifndef CLIB_DECL
#define CLIB_DECL
#endif

typedef unsigned char	byte;
typedef unsigned short	word;

typedef enum {
    DEN_FM_LO = 0,
    DEN_FM_HI,
    DEN_MFM_LO,
    DEN_MFM_HI
} DENSITY;

typedef enum {
    ORD_SIDES = 0,
    ORD_CYLINDERS,
    ORD_EAGLE
} ORDER;

typedef struct {
	word spt;				/* sectors per track		*/
	byte bsh;				/* block shift				*/
	byte blm;				/* block mask				*/
	byte exm;				/* extent mask				*/
	word dsm;				/* drive storage max sector */
	word drm;				/* directory max sector 	*/
	byte al0;				/* allocation bits low		*/
	byte al1;				/* allocation bits high 	*/
	word cks;				/* check sectors (drm+1)/4 if media is removable */
	word off;				/* offset (boot sectors)	*/
} cpm_dpb;

typedef struct {
	const char	*id;			/* short name */
	const char	*name;			/* long name */
	const char	*ref;			/* id reference */
	DENSITY density;		/* fdd density */
	word	cylinders;		/* number of cylinders */
	byte	sides;			/* number of sides */
	byte	spt;			/* sectors per track */
	word	seclen; 		/* sector length */
	byte	skew;			/* sector skew */
	byte	side1[32];		/* side number, sector numbers */
	byte	side2[32];		/* side number, sector numbers */
	ORDER	order;			/* sector ordering */
	const char	*label; 		/* disk label */
	cpm_dpb dpb;			/* associated dpb */
} dsk_fmt;

#include "machine/cpm_disk.c"

void usage(void)
{
	fprintf(stderr, "usage:\tmkimage [-option | format image.dsk]\n");
	fprintf(stderr, "\toption can be one of the following\n");
	fprintf(stderr, "-list\tlist available formats to stdout\n");
	fprintf(stderr, "\totherwise you must specify a format and an image name\n");
}

int CLIB_DECL main(int ac, char **av)
{
int i, track, side, sector;
FILE *fd;
char buff[4096];
	if (ac < 2)
	{
		usage();
		exit(1);
	}
	if (!mame_stricmp(av[1], "-list"))
	{
		for (i = 0; formats[i].id; i++)
			printf("%s\t%s\n", formats[i].id, formats[i].name);
		return 0;
    }
	if (ac < 3)
	{
		usage();
        exit(1);
    }
	for (i = 0; formats[i].id; i++)
	{
		if (!mame_stricmp(formats[i].id, av[1]))
			break;
	}
	if (!formats[i].id)
	{
		fprintf(stderr, "format '%s' not supported\n", av[1]);
		exit(1);
    }
	fd = fopen(av[2], "wb");
	if (!fd)
	{
		fprintf(stderr, "can't create image '%s'\n", av[2]);
        exit(1);
    }
	for (track = 0; track < formats[i].cylinders; track++)
	{
		for (side = 0; side < formats[i].sides; side++)
		{
			for (sector = 0; sector < formats[i].spt; sector++)
			{
				memset(buff, 0xe5, formats[i].seclen);
				sprintf(buff + 0*32 +1, "%-4.4s            TRACK     %3d", formats[i].id, track);
				sprintf(buff + 1*32 +1, "SIDE      %3d   HEAD ID   %3d", side, (side) ? formats[i].side2[0] : formats[i].side1[0]);
				sprintf(buff + 2*32 +1, "SECTOR    %3d   SECTOR ID %3d", sector, (side) ? formats[i].side2[sector+1] : formats[i].side1[sector+1]);
				if (fwrite(buff, 1, formats[i].seclen, fd) != formats[i].seclen)
				{
					fprintf(stderr, "error writing to '%s'\n", av[2]);
					exit(1);
                }
			}
		}
	}
	fclose(fd);
	return 0;
}

