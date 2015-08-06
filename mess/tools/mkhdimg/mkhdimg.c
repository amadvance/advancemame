/*

 I don't know who originally wrote this so I cannot give the proper credit.
 I have kept the original source in the archive for referance.
 The modifications I have made are released into the Public Domain.
 This file was modified to support variable hard disk sizes in M.E.S.S. v0.37b1
 and works as of that version. The Western Digital 1004A that is emulated in
 M.E.S.S. is limited to 1024 Cylinders and 16 heads. The sector size is limited
 to 512 bytes as well as the sectors per track being limited to 17, this is
 also due to the WD1004A's limitations.

 Randy Rains, AKA Mad Hatchet April, 21, 2000

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "utils.h"

#define SECTORS     17
#define MAGIC       0xaa55
#define ECC 		11
#define CONTROL 	5
#define DIPSWITCH	0xff

#define SECLEN		512
static void acerr(void); /* command line argument error */
static void serr(void); /* size error */

int CLIB_DECL main(int ac, char **av) /* ac == Argument Count? av == Argument Variable? */
{
FILE *img;
unsigned char buffer[SECLEN];
long TOTALBYTES;
int c, h, s, CYLINDERS, HEADS;

    if (ac < 3)  /* check number of command line options */
    {
        acerr();
        return 1;
    }
      CYLINDERS = atoi(av[2]); /* make sure cylinders does not exceed 1024 */
      if (CYLINDERS > 1024)

    {
        serr();
        return 1;
    }

     HEADS = atoi(av[3]);  /* make sure heads does not exceed 16 */
      if (HEADS > 16)
    {
        serr();
        return 1;
    }


    av[1]=strcat(av[1],".img");
    img = fopen(av[1], "wb");


    TOTALBYTES = (CYLINDERS*HEADS*SECTORS*512)+512;

        fprintf(stderr, "Creating file named %s, of %ld bytes,\n",av[1],TOTALBYTES);
        fprintf(stderr, "Using %d cylinders, %d heads, %d sectors.\n",CYLINDERS,HEADS,SECTORS);

    memset(buffer, 0, 512);

    /* fill in the drive geometry information */
    buffer[0x1ad] = CYLINDERS & 0xff;           /* cylinders */
	buffer[0x1ae] = (CYLINDERS >> 8) & 3;
	buffer[0x1af] = HEADS;						/* heads */
	buffer[0x1b0] = (CYLINDERS+1) & 0xff;		/* write precompensation */
	buffer[0x1b1] = ((CYLINDERS+1) >> 8) & 3;
	buffer[0x1b2] = (CYLINDERS+1) & 0xff;		/* reduced write current */
	buffer[0x1b3] = ((CYLINDERS+1) >> 8) & 3;
	buffer[0x1b4] = ECC;						/* ECC length */
	buffer[0x1b5] = CONTROL;					/* control (step rate) */
	buffer[0x1b6] = CYLINDERS & 0xff;			/* parking cylinder */
	buffer[0x1b7] = (CYLINDERS >> 8) & 3;
	buffer[0x1b8] = 0x00;						/* no idea */
	buffer[0x1b9] = 0x00;
	buffer[0x1ba] = 0x00;
	buffer[0x1bb] = 0x00;
	buffer[0x1bc] = 0x00;
	buffer[0x1bd] = SECTORS;					/* some non zero value */
	buffer[0x1fe] = MAGIC & 0xff;
    buffer[0x1ff] = (MAGIC >> 8) & 0xff;


	if (fwrite(buffer, 1, SECLEN, img) != SECLEN) {
		fprintf(stderr, "failed to write MBR of %s\n", av[1]);
		return 1;
    }

	/* write F6 patterns throughout the image */
    memset(buffer, 0xf6, sizeof(buffer));

    for (c = 0; c < CYLINDERS; c++) {
        for (h = 0; h < HEADS; h++) {
			printf("\rwriting cylinder %4d, head %2d", c, h);
			fflush(stdout);
            for (s = 0; s < SECTORS; s++) {
				if (fwrite(buffer, 1, SECLEN, img) != SECLEN) {
					fprintf(stderr, "failed to write C%d, H%d, S%d of %s\n", c, h, s, av[1]);
					return 1;
				}
			}
		}
	}

     fclose(img);

    return 0;

}
 void acerr(void)
{

        fprintf(stderr, "usage: mhimg filename CYLINDERS HEADS\n");
        fprintf(stderr, "used to create an empty hard disk image for use with ");
        fprintf(stderr, "MESS' PC-XT emulation.\n");
        fprintf(stderr, "size will equal (CYLINDERS*HEADS*17*512)+512, bytes.\n");
        fprintf(stderr, "CYLINDERS must be in the range of 0 to 1024.\n");
        fprintf(stderr, "HEADS must be in the range of 0 to 16.\n");
        fprintf(stderr, "DO NOT include a file extension in the filename!(.img)\n");
}

void serr(void)
{
        fprintf(stderr, "CYLINDERS cannot exceed 1024 and HEADS cannot exceed 16!\n");
}
