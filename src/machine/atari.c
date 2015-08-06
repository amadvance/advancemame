/******************************************************************************
    Atari 400/800

    Machine driver

    Juergen Buchmueller, June 1998
******************************************************************************/

#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "includes/atari.h"
#include "sound/pokey.h"
#ifdef MESS
#include "image.h"
#endif

#define VERBOSE_POKEY	0
#define VERBOSE_SERIAL	0
#define VERBOSE_TIMERS	0
#define VERBOSE_CHKSUM	0

//#define LOG(x) if (errorlog) fprintf x

ATARI_PIA atari_pia;
ATARI_FDC atari_fdc;

typedef struct {
	UINT8 *image;		/* malloc'd image */
	int type;			/* type of image (XFD, ATR, DSK) */
	int mode;			/* 0 read only, != 0 read/write */
	int density;		/* 0 SD, 1 MD, 2 DD */
	int header_skip;	/* number of bytes in format header */
	int tracks; 		/* number of tracks (35,40,77,80) */
	int heads;			/* number of heads (1,2) */
	int spt;			/* sectors per track (18,26) */
	int seclen; 		/* sector length (128,256) */
	int bseclen;		/* boot sector length (sectors 1..3) */
	int sectors;		/* total sectors, ie. tracks x heads x spt */
}	DRIVE;

static DRIVE drv[4] = {{0}, };

static int atari = 0;
#define ATARI_5200	0
#define ATARI_400	1
#define ATARI_800	2
#define ATARI_600XL 3
#define ATARI_800XL 4

#ifdef MESS
static int a800_cart_loaded = 0;
static int a800_cart_is_16k = 0;
#endif

static void a800xl_mmu(UINT8 old_mmu, UINT8 new_mmu);
static void a600xl_mmu(UINT8 old_mmu, UINT8 new_mmu);

static void pokey_reset(void);
static void atari_pia_reset(void);

static void make_chksum(UINT8 * chksum, UINT8 data);
static void clr_serout(int expect_data);
static void clr_serin(int ser_delay);
static void add_serin(UINT8 data, int with_checksum);

#ifdef MESS
static void a800_setbank(int n)
{
	void *read_addr;
	void *write_addr;
	UINT8 *mem = memory_region(REGION_CPU1);

	switch (n)
	{
		case 1:
			read_addr = &mem[0x10000];
			write_addr = NULL;
			break;
		default:
			if( atari <= ATARI_400 )
			{
				/* Atari 400 has no RAM here, so install the NOP handler */
				read_addr = NULL;
				write_addr = NULL;
			}
			else
			{
				read_addr = &mess_ram[0x08000];
				write_addr = &mess_ram[0x08000];
			}
			break;
	}

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0,
		read_addr ? MRA8_BANK1 : MRA8_NOP);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xbfff, 0, 0,
		write_addr ? MWA8_BANK1 : MWA8_NOP);
	if (read_addr)
		memory_set_bankptr(1, read_addr);
	if (write_addr)
		memory_set_bankptr(1, write_addr);
}
#endif

static void machine_reset_atari_generic(int machine_type, int has_cart, int has_pia)
{
	atari = machine_type;

	gtia_reset();
	pokey_reset();
	if (has_pia)
		atari_pia_reset();
	antic_reset();

#ifdef MESS
	if (has_cart && a800_cart_loaded)
		a800_setbank(1);
#endif
}

MACHINE_RESET( a400 )
{
	machine_reset_atari_generic(ATARI_400, TRUE, TRUE);
}

MACHINE_RESET( a800 )
{
	machine_reset_atari_generic(ATARI_800, TRUE, TRUE);
}

#ifdef MESS
DEVICE_LOAD( a800_cart )
{
	UINT8 *mem = memory_region(REGION_CPU1);
	int size;

	/* load an optional (dual) cartridge (e.g. basic.rom) */
	if( image_index_in_device(image) > 0 )
	{
		size = mame_fread(file, &mem[0x12000], 0x2000);
		a800_cart_is_16k = (size == 0x2000);
		logerror("%s loaded right cartridge '%s' size 16K\n", Machine->gamedrv->name, image_filename(image) );
	}
	else
	{
		size = mame_fread(file, &mem[0x10000], 0x2000);
		a800_cart_loaded = size > 0x0000;
		size = mame_fread(file, &mem[0x12000], 0x2000);
		a800_cart_is_16k = size > 0x2000;
		logerror("%s loaded left cartridge '%s' size %s\n", Machine->gamedrv->name, image_filename(image) , (a800_cart_is_16k) ? "16K":"8K");
	}
	return INIT_PASS;
}

DEVICE_UNLOAD( a800_cart )
{
	if( image_index_in_device(image) > 0 )
	{
		a800_cart_is_16k = 0;
		a800_setbank(1);
    }
	else
	{
		a800_cart_loaded = 0;
		a800_setbank(0);
    }
}
#endif

/**************************************************************
 *
 * Atari 600XL (for MAME only)
 *
 **************************************************************/
MACHINE_RESET(a600xl)
{
	machine_reset_atari_generic(ATARI_600XL, FALSE, TRUE);
	a600xl_mmu(atari_pia.w.pbout, atari_pia.w.pbout);
}

/**************************************************************
 *
 * Atari 800-XL
 *
 **************************************************************/

MACHINE_RESET( a800xl)
{
	machine_reset_atari_generic(ATARI_800XL, FALSE, TRUE);
	a800xl_mmu(atari_pia.w.pbout, atari_pia.w.pbout);
}

#ifdef MESS
DEVICE_LOAD( a800xl_cart )
{
	UINT8 *mem = memory_region(REGION_CPU1);
	const char *filename;
	mame_file *basic_fp;
	unsigned size;

	filename = "basic.rom";
	basic_fp = mame_fopen(Machine->gamedrv->name, filename, FILETYPE_ROM, 0);
	if (basic_fp)
	{
		size = mame_fread(basic_fp, &mem[0x14000], 0x2000);
		if( size < 0x2000 )
		{
			logerror("%s image '%s' load failed (less than 8K)\n", Machine->gamedrv->name, filename);
			return 2;
		}
	}

	/* load an optional (dual) cartidge (e.g. basic.rom) */
	if (file)
	{
		{
			size = mame_fread(file, &mem[0x14000], 0x2000);
			a800_cart_loaded = size / 0x2000;
			size = mame_fread(file, &mem[0x16000], 0x2000);
			a800_cart_is_16k = size / 0x2000;
			logerror("%s loaded cartridge '%s' size %s\n",
					Machine->gamedrv->name, image_filename(image), (a800_cart_is_16k) ? "16K":"8K");
		}
	}

	return INIT_PASS;
}
#endif
/**************************************************************
 *
 * Atari 5200 console
 *
 **************************************************************/

MACHINE_RESET( a5200 )
{
	machine_reset_atari_generic(ATARI_5200, FALSE, FALSE);
}

#ifdef MESS
DEVICE_LOAD( a5200_cart )
{
	UINT8 *mem = memory_region(REGION_CPU1);
	int size;

	/* load an optional (dual) cartidge */
	size = mame_fread(file, &mem[0x4000], 0x8000);
	if (size<0x8000) memmove(mem+0x4000+0x8000-size, mem+0x4000, size);
	// mirroring of smaller cartridges
	if (size <= 0x1000) memcpy(mem+0xa000, mem+0xb000, 0x1000);
	if (size <= 0x2000) memcpy(mem+0x8000, mem+0xa000, 0x2000);
	if (size <= 0x4000)
	{
		const char *info;
		memcpy(&mem[0x4000], &mem[0x8000], 0x4000);
		info = image_extrainfo(image);
		if (info!=NULL && strcmp(info, "A13MIRRORING")==0)
		{
			memcpy(&mem[0x8000], &mem[0xa000], 0x2000);
			memcpy(&mem[0x6000], &mem[0x4000], 0x2000);
		}
	}
	logerror("%s loaded cartridge '%s' size %dK\n",
		Machine->gamedrv->name, image_filename(image) , size/1024);
	return INIT_PASS;
}

DEVICE_UNLOAD( a5200_cart )
{
	UINT8 *mem = memory_region(REGION_CPU1);
    /* zap the cartridge memory (again) */
	memset(&mem[0x4000], 0x00, 0x8000);
}
#endif

static void pokey_reset(void)
{
	pokey1_w(15,0);
}

#ifdef MESS

#define FORMAT_XFD	0
#define FORMAT_ATR	1
#define FORMAT_DSK	2

/*****************************************************************************
 * This is the structure I used in my own Atari 800 emulator some years ago
 * Though it's a bit overloaded, I used it as it is the maximum of all
 * supported formats:
 * XFD no header at all
 * ATR 16 bytes header
 * DSK this struct
 * It is used to determine the format of a XFD image by it's size only
 *****************************************************************************/

typedef struct {
	UINT8 density;
	UINT8 tracks;
	UINT8 door;
	UINT8 sta1;
	UINT8 spt;
	UINT8 doublesided;
	UINT8 highdensity;
	UINT8 seclen_hi;
	UINT8 seclen_lo;
	UINT8 status;
	UINT8 sta2;
	UINT8 sta3;
	UINT8 sta4;
	UINT8 cr;
	UINT8 info[65+1];
}	dsk_format;

/* combined with the size the image should have */
typedef struct	{
	int size;
	dsk_format dsk;
}	xfd_format;

/* here's a table of known xfd formats */
static	xfd_format xfd_formats[] = {
	{35 * 18 * 1 * 128, 				{0,35,1,0,18,0,0,0,128,255,0,0,0,13,"35 SS/SD"}},
	{35 * 26 * 1 * 128, 				{1,35,1,0,26,0,4,0,128,255,0,0,0,13,"35 SS/MD"}},
	{(35 * 18 * 1 - 3) * 256 + 3 * 128, {2,35,1,0,18,0,4,1,  0,255,0,0,0,13,"35 SS/DD"}},
	{40 * 18 * 1 * 128, 				{0,40,1,0,18,0,0,0,128,255,0,0,0,13,"40 SS/SD"}},
	{40 * 26 * 1 * 128, 				{1,40,1,0,26,0,4,0,128,255,0,0,0,13,"40 SS/MD"}},
	{(40 * 18 * 1 - 3) * 256 + 3 * 128, {2,40,1,0,18,0,4,1,  0,255,0,0,0,13,"40 SS/DD"}},
	{40 * 18 * 2 * 128, 				{0,40,1,0,18,1,0,0,128,255,0,0,0,13,"40 DS/SD"}},
	{40 * 26 * 2 * 128, 				{1,40,1,0,26,1,4,0,128,255,0,0,0,13,"40 DS/MD"}},
	{(40 * 18 * 2 - 3) * 256 + 3 * 128, {2,40,1,0,18,1,4,1,  0,255,0,0,0,13,"40 DS/DD"}},
	{77 * 18 * 1 * 128, 				{0,77,1,0,18,0,0,0,128,255,0,0,0,13,"77 SS/SD"}},
	{77 * 26 * 1 * 128, 				{1,77,1,0,26,0,4,0,128,255,0,0,0,13,"77 SS/MD"}},
	{(77 * 18 * 1 - 3) * 256 + 3 * 128, {2,77,1,0,18,0,4,1,  0,255,0,0,0,13,"77 SS/DD"}},
	{77 * 18 * 2 * 128, 				{0,77,1,0,18,1,0,0,128,255,0,0,0,13,"77 DS/SD"}},
	{77 * 26 * 2 * 128, 				{1,77,1,0,26,1,4,0,128,255,0,0,0,13,"77 DS/MD"}},
	{(77 * 18 * 2 - 3) * 256 + 3 * 128, {2,77,1,0,18,1,4,1,  0,255,0,0,0,13,"77 DS/DD"}},
	{80 * 18 * 2 * 128, 				{0,80,1,0,18,1,0,0,128,255,0,0,0,13,"80 DS/SD"}},
	{80 * 26 * 2 * 128, 				{1,80,1,0,26,1,4,0,128,255,0,0,0,13,"80 DS/MD"}},
	{(80 * 18 * 2 - 3) * 256 + 3 * 128, {2,80,1,0,18,1,4,1,  0,255,0,0,0,13,"80 DS/DD"}},
	{0, {0,}}
};
#endif

/*****************************************************************************
 *
 * Open a floppy image for drive 'drive' if it is not yet openend
 * and a name was given. Determine the image geometry depending on the
 * type of image and store the results into the global drv[] structure
 *
 *****************************************************************************/
#define MAXSIZE 5760 * 256 + 80
#ifdef MESS
DEVICE_LOAD( a800_floppy )
{
	int size, i;
	const char *ext;
	int id = image_index_in_device(image);

	drv[id].image = image_malloc(image, MAXSIZE);
	if (!drv[id].image)
		return INIT_FAIL;

	/* tell whether the image is writable */
	drv[id].mode = image_is_writable(image);
	/* set up image if it has been created */
	if (image_has_been_created(image))
	{
		int sector;
		char buff[256];
		memset(buff, 0, 256);
		/* default to 720 sectors */
		for( sector = 0; sector < 720; sector++ )
			mame_fwrite(file, buff, 256);
		mame_fseek(file, 0, SEEK_SET);
	}

	size = mame_fread(file, drv[id].image, MAXSIZE);
	if( size <= 0 )
	{
		drv[id].image = NULL;
		return INIT_FAIL;
	}
	/* re allocate the buffer; we don't want to be too lazy ;) */
    drv[id].image = image_realloc(image, drv[id].image, size);

	ext = image_filetype(image);
    /* no extension: assume XFD format (no header) */
    if (!ext)
    {
        drv[id].type = FORMAT_XFD;
        drv[id].header_skip = 0;
    }
    else
    /* XFD extension */
    if( toupper(ext[0])=='X' && toupper(ext[1])=='F' && toupper(ext[2])=='D' )
    {
        drv[id].type = FORMAT_XFD;
        drv[id].header_skip = 0;
    }
    else
    /* ATR extension */
    if( toupper(ext[0])=='A' && toupper(ext[1])=='T' && toupper(ext[2])=='R' )
    {
        drv[id].type = FORMAT_ATR;
        drv[id].header_skip = 16;
    }
    else
    /* DSK extension */
    if( toupper(ext[0])=='D' && toupper(ext[1])=='S' && toupper(ext[2])=='K' )
    {
        drv[id].type = FORMAT_DSK;
        drv[id].header_skip = sizeof(dsk_format);
    }
    else
    {
		drv[id].type = FORMAT_XFD;
        drv[id].header_skip = 0;
    }

	if( drv[id].type == FORMAT_ATR &&
		(drv[id].image[0] != 0x96 || drv[id].image[1] != 0x02) )
	{
		drv[id].type = FORMAT_XFD;
		drv[id].header_skip = 0;
	}

	switch (drv[id].type)
	{
	/* XFD or unknown format: find a matching size from the table */
	case FORMAT_XFD:
		for( i = 0; xfd_formats[i].size; i++ )
		{
			if( size == xfd_formats[i].size )
			{
				drv[id].density = xfd_formats[i].dsk.density;
				drv[id].tracks = xfd_formats[i].dsk.tracks;
				drv[id].spt = xfd_formats[i].dsk.spt;
				drv[id].heads = (xfd_formats[i].dsk.doublesided) ? 2 : 1;
				drv[id].bseclen = 128;
				drv[id].seclen = 256 * xfd_formats[i].dsk.seclen_hi + xfd_formats[i].dsk.seclen_lo;
				drv[id].sectors = drv[id].tracks * drv[id].heads * drv[id].spt;
				break;
			}
		}
		break;
	/* ATR format: find a size including the 16 bytes header */
	case FORMAT_ATR:
		{
			int s;

			drv[id].bseclen = 128;
			/* get sectors from ATR header */
			s = (size - 16) / 128;
			/* 3 + odd number of sectors ? */
			if ( drv[id].image[4] == 128 || (s % 18) == 0 || (s % 26) == 0 || ((s - 3) % 1) != 0 )
			{
				drv[id].sectors = s;
				drv[id].seclen = 128;
				/* sector size 128 or count not evenly dividable by 26 ? */
				if( drv[id].seclen == 128 || (s % 26) != 0 )
				{
					/* yup! single density */
					drv[id].density = 0;
					drv[id].spt = 18;
					drv[id].heads = 1;
					drv[id].tracks = s / 18;
					if( s % 18 != 0 )
						drv[id].tracks += 1;
					if( drv[id].tracks % 2 == 0 && drv[id].tracks > 80 )
					{
						drv[id].heads = 2;
						drv[id].tracks /= 2;
					}
				}
				else
				{
					/* yes: medium density */
					drv[id].density = 0;
					drv[id].spt = 26;
					drv[id].heads = 1;
					drv[id].tracks = s / 26;
					if( s % 26 != 0 )
						drv[id].tracks += 1;
					if( drv[id].tracks % 2 == 0 && drv[id].tracks > 80 )
					{
						drv[id].heads = 2;
						drv[id].tracks /= 2;
					}
				}
			}
			else
			{
				/* it's double density */
				s = (s - 3) / 2 + 3;
				drv[id].sectors = s;
				drv[id].density = 2;
				drv[id].seclen = 256;
				drv[id].spt = 18;
				drv[id].heads = 1;
				drv[id].tracks = s / 18;
				if( s % 18 != 0 )
					drv[id].tracks += 1;
				if( drv[id].tracks % 2 == 0 && drv[id].tracks > 80 )
				{
					drv[id].heads = 2;
					drv[id].tracks /= 2;
				}
			}
		}
		break;
	/* DSK format: it's all in the header */
	case FORMAT_DSK:
		{
			dsk_format *dsk = (dsk_format *) drv[id].image;

			drv[id].tracks = dsk->tracks;
			drv[id].spt = dsk->spt;
			drv[id].heads = (dsk->doublesided) ? 2 : 1;
			drv[id].seclen = 256 * dsk->seclen_hi + dsk->seclen_lo;
			drv[id].bseclen = drv[id].seclen;
			drv[id].sectors = drv[id].tracks * drv[id].heads * drv[id].spt;
		}
		break;
	}
	logerror("atari opened floppy '%s', %d sectors (%d %s%s) %d bytes/sector\n",
			image_filename(image),
			drv[id].sectors,
			drv[id].tracks,
			(drv[id].heads == 1) ? "SS" : "DS",
			(drv[id].density == 0) ? "SD" : (drv[id].density == 1) ? "MD" : "DD",
			drv[id].seclen);
	return INIT_PASS;
}
#endif

static	void make_chksum(UINT8 * chksum, UINT8 data)
{
	UINT8 new;
	new = *chksum + data;
	if (new < *chksum)
		new++;
#if VERBOSE_CHKSUM
	logerror("atari chksum old $%02x + data $%02x -> new $%02x\n", *chksum, data, new);
#endif
	*chksum = new;
}

static	void clr_serout(int expect_data)
{
	atari_fdc.serout_chksum = 0;
	atari_fdc.serout_offs = 0;
	atari_fdc.serout_count = expect_data + 1;
}

static	void add_serout(int expect_data)
{
	atari_fdc.serout_chksum = 0;
	atari_fdc.serout_count = expect_data + 1;
}

static	void clr_serin(int ser_delay)
{
	atari_fdc.serin_chksum = 0;
	atari_fdc.serin_offs = 0;
	atari_fdc.serin_count = 0;
	pokey1_serin_ready(ser_delay * 40);
}

static	void add_serin(UINT8 data, int with_checksum)
{
	atari_fdc.serin_buff[atari_fdc.serin_count++] = data;
	if (with_checksum)
		make_chksum(&atari_fdc.serin_chksum, data);
}

/*****************************************************************************
 *
 * This is a description of the data flow between Atari (A) and the
 * Floppy (F) for the supported commands.
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  'S'  00   00                 get status
 * F->A     ACK  CPL  04   FF   E0   00   CKS
 *                     ^    ^
 *                     |    |
 *                     |    bit 7 : door closed
 *                     |
 *                     bit7  : MD 128 bytes/sector, 26 sectors/track
 *                     bit5  : DD 256 bytes/sector, 18 sectors/track
 *                     else  : SD 128 bytes/sector, 18 sectors/track
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  'R'  SECL SECH               read sector
 * F->A     ACK                               command acknowledge
 *               ***                          now read the sector
 * F->A              CPL                      complete: sector read
 * F->A                  128/256 byte CKS
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  'W'  SECL SECH               write with verify
 * F->A     ACK                               command acknowledge
 * A->F          128/256 data CKS
 * F->A                            CPL        complete: CKS okay
 *          execute writing the sector
 * F->A                                 CPL   complete: sector written
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  'P'  SECL SECH               put sector
 * F->A     ACK                               command acknowledge
 * A->F          128/256 data CKS
 * F->A                            CPL        complete: CKS okay
 *          execute writing the sector
 * F->A                                 CPL   complete: sector written
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *           '1' '!'  xx   xx                 single density format
 * F->A     ACK                               command acknowledge
 *          execute formatting
 * F->A               CPL                     complete: format
 * F->A                    128/256 byte CKS   bad sector table
 *
 *
 * A->F     DEV  CMD  AUX1 AUX2 CKS
 *          '1'  '"'  xx   xx                 double density format
 * F->A     ACK                               command acknowledge
 *          execute formatting
 * F->A               CPL                     complete: format
 * F->A                    128/256 byte CKS   bad sector table
 *
 *****************************************************************************/

void a800_serial_command(void)
{
	int  i, drive, sector, offset;

	if( !atari_fdc.serout_offs )
	{
#if VERBOSE_SERIAL
	logerror("atari serout command offset = 0\n");
#endif
		return;
	}
	clr_serin(10);
#if VERBOSE_SERIAL
	logerror("atari serout command %d: %02X %02X %02X %02X %02X : %02X ",
		atari_fdc.serout_offs,
		atari_fdc.serout_buff[0], atari_fdc.serout_buff[1], atari_fdc.serout_buff[2],
		atari_fdc.serout_buff[3], atari_fdc.serout_buff[4], atari_fdc.serout_chksum);
#endif
	if (atari_fdc.serout_chksum == 0)
	{
#if VERBOSE_SERIAL
		logerror("OK\n");
#endif
		drive = atari_fdc.serout_buff[0] - '1';   /* drive # */
		/* sector # */
		if (drive < 0 || drive > 3) 			/* ignore unknown drives */
		{
			logerror("atari unsupported drive #%d\n", drive+1);
			sprintf(atari_frame_message, "DRIVE #%d not supported", drive+1);
			atari_frame_counter = Machine->drv->frames_per_second/2;
			return;
		}

		/* extract sector number from the command buffer */
		sector = atari_fdc.serout_buff[2] + 256 * atari_fdc.serout_buff[3];

		switch (atari_fdc.serout_buff[1]) /* command ? */
		{
			case 'S':   /* status */
				sprintf(atari_frame_message, "DRIVE #%d STATUS", drive+1);
				atari_frame_counter = Machine->drv->frames_per_second/2;
#if VERBOSE_SERIAL
				logerror("atari status\n");
#endif
				add_serin('A',0);
				add_serin('C',0);
				if (!drv[drive].mode) /* read only mode ? */
				{
					if (drv[drive].spt == 26)
						add_serin(0x80,1);	/* MD: 0x80 */
					else
					if (drv[drive].seclen == 128)
						add_serin(0x00,1);	/* SD: 0x00 */
					else
						add_serin(0x20,1);	/* DD: 0x20 */
				}
				else
				{
					if (drv[drive].spt == 26)
						add_serin(0x84,1);	/* MD: 0x84 */
					else
					if (drv[drive].seclen == 128)
						add_serin(0x04,1);	/* SD: 0x04 */
					else
						add_serin(0x24,1);	/* DD: 0x24 */
				}
				if (drv[drive].image)
					add_serin(0xff,1);	/* door closed: 0xff */
				else
					add_serin(0x7f,1);	/* door open: 0x7f */
				add_serin(0xe0,1);	/* dunno */
				add_serin(0x00,1);	/* dunno */
				add_serin(atari_fdc.serin_chksum,0);
				break;

			case 'R':   /* read sector */
#if VERBOSE_SERIAL
				logerror("atari read sector #%d\n", sector);
#endif
				if( sector < 1 || sector > drv[drive].sectors )
				{
					sprintf(atari_frame_message, "DRIVE #%d READ SECTOR #%3d - ERR", drive+1, sector);
					atari_frame_counter = Machine->drv->frames_per_second/2;
#if VERBOSE_SERIAL
					logerror("atari bad sector #\n");
#endif
					add_serin('E',0);
					break;
				}
				add_serin('A',0);   /* acknowledge */
				add_serin('C',0);   /* completed */
				if (sector < 4) 	/* sector 1 .. 3 might be different length */
				{
					sprintf(atari_frame_message, "DRIVE #%d READ SECTOR #%3d - SD", drive+1, sector);
                    atari_frame_counter = Machine->drv->frames_per_second/2;
                    offset = (sector - 1) * drv[drive].bseclen + drv[drive].header_skip;
					for (i = 0; i < 128; i++)
						add_serin(drv[drive].image[offset++],1);
				}
				else
				{
					sprintf(atari_frame_message, "DRIVE #%d READ SECTOR #%3d - %cD", drive+1, sector, (drv[drive].seclen == 128) ? 'S' : 'D');
                    atari_frame_counter = Machine->drv->frames_per_second/2;
                    offset = (sector - 1) * drv[drive].seclen + drv[drive].header_skip;
					for (i = 0; i < drv[drive].seclen; i++)
						add_serin(drv[drive].image[offset++],1);
				}
				add_serin(atari_fdc.serin_chksum,0);
				break;

			case 'W':   /* write sector with verify */
#if VERBOSE_SERIAL
				logerror("atari write sector #%d\n", sector);
#endif
                add_serin('A',0);
				if (sector < 4) 	/* sector 1 .. 3 might be different length */
				{
					add_serout(drv[drive].bseclen);
					sprintf(atari_frame_message, "DRIVE #%d WRITE SECTOR #%3d - SD", drive+1, sector);
					atari_frame_counter = Machine->drv->frames_per_second/2;
                }
				else
				{
					add_serout(drv[drive].seclen);
					sprintf(atari_frame_message, "DRIVE #%d WRITE SECTOR #%3d - %cD", drive+1, sector, (drv[drive].seclen == 128) ? 'S' : 'D');
                    atari_frame_counter = Machine->drv->frames_per_second/2;
                }
				break;

			case 'P':   /* put sector (no verify) */
#if VERBOSE_SERIAL
				logerror("atari put sector #%d\n", sector);
#endif
				add_serin('A',0);
				if (sector < 4) 	/* sector 1 .. 3 might be different length */
				{
					add_serout(drv[drive].bseclen);
					sprintf(atari_frame_message, "DRIVE #%d PUT SECTOR #%3d - SD", drive+1, sector);
                    atari_frame_counter = Machine->drv->frames_per_second/2;
                }
				else
				{
					add_serout(drv[drive].seclen);
					sprintf(atari_frame_message, "DRIVE #%d PUT SECTOR #%3d - %cD", drive+1, sector, (drv[drive].seclen == 128) ? 'S' : 'D');
                    atari_frame_counter = Machine->drv->frames_per_second/2;
                }
				break;

			case '!':   /* SD format */
#if VERBOSE_SERIAL
				logerror("atari format SD drive #%d\n", drive+1);
#endif
				sprintf(atari_frame_message, "DRIVE #%d FORMAT SD", drive+1);
				atari_frame_counter = Machine->drv->frames_per_second/2;
                add_serin('A',0);   /* acknowledge */
				add_serin('C',0);   /* completed */
				for (i = 0; i < 128; i++)
					add_serin(0,1);
				add_serin(atari_fdc.serin_chksum,0);
				break;

			case '"':   /* DD format */
#if VERBOSE_SERIAL
				logerror("atari format DD drive #%d\n", drive+1);
#endif
				sprintf(atari_frame_message, "DRIVE #%d FORMAT DD", drive+1);
                atari_frame_counter = Machine->drv->frames_per_second/2;
                add_serin('A',0);   /* acknowledge */
				add_serin('C',0);   /* completed */
				for (i = 0; i < 256; i++)
					add_serin(0,1);
				add_serin(atari_fdc.serin_chksum,0);
				break;

			default:
#if VERBOSE_SERIAL
				logerror("atari unknown command #%c\n", atari_fdc.serout_buff[1]);
#endif
				sprintf(atari_frame_message, "DRIVE #%d UNKNOWN CMD '%c'", drive+1, atari_fdc.serout_buff[1]);
                atari_frame_counter = Machine->drv->frames_per_second/2;
                add_serin('N',0);   /* negative acknowledge */
		}
	}
	else
	{
		sprintf(atari_frame_message, "serial cmd chksum error");
		atari_frame_counter = Machine->drv->frames_per_second/2;
#if VERBOSE_SERIAL
		logerror("BAD\n");
#endif
		add_serin('E',0);
	}
#if VERBOSE_SERIAL
	logerror("atari %d bytes to read\n", atari_fdc.serin_count);
#endif
}

void a800_serial_write(void)
{
int i, drive, sector, offset;
#if VERBOSE_SERIAL
	logerror("atari serout %d bytes written : %02X ",
		atari_fdc.serout_offs, atari_fdc.serout_chksum);
#endif
	clr_serin(80);
	if (atari_fdc.serout_chksum == 0)
	{
#if VERBOSE_SERIAL
		logerror("OK\n");
#endif
		add_serin('C',0);
		/* write the sector */
		drive = atari_fdc.serout_buff[0] - '1';   /* drive # */
		/* not write protected and image available ? */
		if (drv[drive].mode && drv[drive].image)
		{
			/* extract sector number from the command buffer */
			sector = atari_fdc.serout_buff[2] + 256 * atari_fdc.serout_buff[3];
			if (sector < 4) 	/* sector 1 .. 3 might be different length */
			{
				offset = (sector - 1) * drv[drive].bseclen + drv[drive].header_skip;
#if VERBOSE_SERIAL
				logerror("atari storing 128 byte sector %d at offset 0x%08X", sector, offset );
#endif
				for (i = 0; i < 128; i++)
					drv[drive].image[offset++] = atari_fdc.serout_buff[5+i];
				sprintf(atari_frame_message, "DRIVE #%d WROTE SECTOR #%3d - SD", drive+1, sector);
				atari_frame_counter = Machine->drv->frames_per_second/2;
            }
			else
			{
				offset = (sector - 1) * drv[drive].seclen + drv[drive].header_skip;
#if VERBOSE_SERIAL
				logerror("atari storing %d byte sector %d at offset 0x%08X", drv[drive].seclen, sector, offset );
#endif
				for (i = 0; i < drv[drive].seclen; i++)
					drv[drive].image[offset++] = atari_fdc.serout_buff[5+i];
				sprintf(atari_frame_message, "DRIVE #%d WROTE SECTOR #%3d - %cD", drive+1, sector, (drv[drive].seclen == 128) ? 'S' : 'D');
                atari_frame_counter = Machine->drv->frames_per_second/2;
            }
			add_serin('C',0);
		}
		else
		{
			add_serin('E',0);
		}
	}
	else
	{
#if VERBOSE_SERIAL
		logerror("BAD\n");
#endif
		add_serin('E',0);
	}
}

READ8_HANDLER ( atari_serin_r )
{
	int data = 0x00;
	int ser_delay = 0;

	if (atari_fdc.serin_count)
	{
		data = atari_fdc.serin_buff[atari_fdc.serin_offs];
		ser_delay = 2 * 40;
		if (atari_fdc.serin_offs < 3)
		{
			ser_delay = 4 * 40;
			if (atari_fdc.serin_offs < 2)
				ser_delay = 200 * 40;
		}
		atari_fdc.serin_offs++;
		if (--atari_fdc.serin_count == 0)
			atari_fdc.serin_offs = 0;
		else
			pokey1_serin_ready(ser_delay);
	}
#if VERBOSE_SERIAL
	logerror("atari serin[$%04x] -> $%02x; delay %d\n", atari_fdc.serin_offs, data, ser_delay);
#endif
	return data;
}

WRITE8_HANDLER ( atari_serout_w )
{
	/* ignore serial commands if no floppy image is specified */
	if( !drv[0].image )
		return;
	if (atari_fdc.serout_count)
	{
		/* store data */
		atari_fdc.serout_buff[atari_fdc.serout_offs] = data;
#if VERBOSE_SERIAL
		logerror("atari serout[$%04x] <- $%02x; count %d\n", atari_fdc.serout_offs, data, atari_fdc.serout_count));
#endif
		atari_fdc.serout_offs++;
		if (--atari_fdc.serout_count == 0)
		{
			/* exclusive or written checksum with calculated */
			atari_fdc.serout_chksum ^= data;
			/* if the attention line is high, this should be data */
			if (atari_pia.rw.pbctl & 0x08)
				a800_serial_write();
		}
		else
		{
			make_chksum(&atari_fdc.serout_chksum, data);
		}
	}
}

void atari_interrupt_cb(int mask)
{

#if VERBOSE_POKEY
		if (mask & 0x80)
			logerror("atari interrupt_cb BREAK\n");
		if (mask & 0x40)
			logerror("atari interrupt_cb KBCOD\n");
#endif
#if VERBOSE_SERIAL
		if (mask & 0x20)
			logerror("atari interrupt_cb SERIN\n");
		if (mask & 0x10)
			logerror("atari interrupt_cb SEROR\n");
		if (mask & 0x08)
			logerror("atari interrupt_cb SEROC\n");
#endif
#if VERBOSE_TIMERS
		if (mask & 0x04)
			logerror("atari interrupt_cb TIMR4\n");
		if (mask & 0x02)
			logerror("atari interrupt_cb TIMR2\n");
		if (mask & 0x01)
			logerror("atari interrupt_cb TIMR1\n");
#endif

	cpunum_set_input_line(0, 0, HOLD_LINE);
}

/**************************************************************
 *
 * Read PIA hardware registers
 *
 **************************************************************/

READ8_HANDLER ( atari_pia_r )
{
	UINT8 result;
	switch (offset & 3)
	{
		case 0: /* PIA port A */
			atari_pia.r.painp = atari_readinputport(PORT_JOY_1_2);
			result = (atari_pia.w.paout & atari_pia.h.pamsk) | (atari_pia.r.painp & ~atari_pia.h.pamsk);
			break;

		case 1: /* PIA port B */
			atari_pia.r.pbinp = atari_readinputport(PORT_JOY_2_3);
			result =  (atari_pia.w.pbout & atari_pia.h.pbmsk) | (atari_pia.r.pbinp & ~atari_pia.h.pbmsk);
			break;

		case 2: /* PIA port A control */
			result = atari_pia.rw.pactl;
			break;

		case 3: /* PIA port B control */
			result = atari_pia.rw.pbctl;
			break;

		default:
			result = 0xFF;
			break;
	}
	return result;
}

/**************************************************************
 *
 * Write PIA hardware registers
 *
 **************************************************************/

WRITE8_HANDLER ( atari_pia_w )
{
	switch (offset & 3)
	{
		case 0: /* PIA port A */
			if (atari_pia.rw.pactl & 0x04)
				atari_pia.w.paout = data;	/* output */
			else
				atari_pia.h.pamsk = data;	/* mask register */
			break;
		case 1: /* PIA port B */
			if( atari_pia.rw.pbctl & 0x04 )
			{
				if( atari == ATARI_800XL )
				{
					a800xl_mmu(atari_pia.w.pbout, data);
				}
				else if ( atari == ATARI_600XL )
				{
					a600xl_mmu(atari_pia.w.pbout, data);
				}
				atari_pia.w.pbout = data;	/* output */
			}
			else
			{
				atari_pia.h.pbmsk = data;	/* 400/800 mode mask register */
			}
			break;
		case 2: /* PIA port A control */
			atari_pia.rw.pactl = data;
			break;
		case 3: /* PIA port B control */
			if( (atari_pia.rw.pbctl ^ data) & 0x08 )  /* serial attention change ? */
			{
				if( atari_pia.rw.pbctl & 0x08 ) 	  /* serial attention before ? */
				{
					clr_serout(4);				/* expect 4 command bytes + checksum */
				}
				else
				{
					atari_fdc.serin_delay = 0;
					a800_serial_command();
				}
			}
			atari_pia.rw.pbctl = data;
			break;
	}
}

/**************************************************************
 *
 * Reset hardware
 *
 **************************************************************/
static void	atari_pia_reset(void)
{
	/* reset the PIA */
	atari_pia_w(3,0);
	atari_pia_w(2,0);
	atari_pia_w(1,0);
	atari_pia_w(0,0);
}

void a600xl_mmu(UINT8 old_mmu, UINT8 new_mmu)
{
	UINT8 changes = new_mmu ^ old_mmu;
	read8_handler rbank2;
	write8_handler wbank2;

	if( changes == 0 )
		return;

	logerror("%s MMU old:%02x new:%02x\n", Machine->gamedrv->name, old_mmu, new_mmu);

	/* check if self-test ROM changed */
	if( changes & 0x80 )
	{
		if ( new_mmu & 0x80 )
		{
			logerror("%s MMU SELFTEST RAM\n", Machine->gamedrv->name);
			rbank2 = MRA8_NOP;
			wbank2 = MWA8_NOP;
		}
		else
		{
			logerror("%s MMU SELFTEST ROM\n", Machine->gamedrv->name);
			rbank2 = MRA8_BANK2;
			wbank2 = MWA8_ROM;
		}
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x57ff, 0, 0, rbank2);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x57ff, 0, 0, wbank2);
		if (rbank2 == MRA8_BANK2)
			memory_set_bankptr(2, memory_region(REGION_CPU1)+0x5000);
	}
}

void a800xl_mmu(UINT8 old_mmu, UINT8 new_mmu)
{
	UINT8 changes = new_mmu ^ old_mmu;
	read8_handler rbank1, rbank2, rbank3, rbank4;
	write8_handler wbank1, wbank2, wbank3, wbank4;
	UINT8 *base1, *base2, *base3, *base4;

	if( changes == 0 )
		return;

	logerror("%s MMU old:%02x new:%02x\n", Machine->gamedrv->name, old_mmu, new_mmu);

	/* check if memory C000-FFFF changed */
	if( changes & 0x01 )
	{
		if( new_mmu & 0x01 )
		{
			logerror("%s MMU BIOS ROM\n", Machine->gamedrv->name);
			rbank3 = MRA8_BANK3;
			wbank3 = MWA8_ROM;
			base3 = memory_region(REGION_CPU1)+0x14000;  /* 8K lo BIOS */
			rbank4 = MRA8_BANK4;
			wbank4 = MWA8_ROM;
			base4 = memory_region(REGION_CPU1)+0x15800;  /* 4K FP ROM + 8K hi BIOS */
		}
		else
		{
			logerror("%s MMU BIOS RAM\n", Machine->gamedrv->name);
			rbank3 = MRA8_BANK3;
			wbank3 = MWA8_BANK3;
			base3 = memory_region(REGION_CPU1)+0x0c000;  /* 8K RAM */
			rbank4 = MRA8_BANK4;
			wbank4 = MWA8_BANK4;
			base4 = memory_region(REGION_CPU1)+0x0d800;  /* 4K RAM + 8K RAM */
		}
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xcfff, 0, 0, rbank3);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xcfff, 0, 0, wbank3);
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd800, 0xffff, 0, 0, rbank4);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xd800, 0xffff, 0, 0, wbank4);
		memory_set_bankptr(3, base3);
		memory_set_bankptr(4, base4);
	}
	/* check if BASIC changed */
	if( changes & 0x02 )
	{
		if( new_mmu & 0x02 )
		{
			logerror("%s MMU BASIC RAM\n", Machine->gamedrv->name);
			rbank1 = MRA8_BANK1;
			wbank1 = MWA8_BANK1;
			base1 = memory_region(REGION_CPU1)+0x0a000;  /* 8K RAM */
		}
		else
		{
			logerror("%s MMU BASIC ROM\n", Machine->gamedrv->name);
			rbank1 = MRA8_BANK1;
			wbank1 = MWA8_ROM;
			base1 = memory_region(REGION_CPU1)+0x10000;  /* 8K BASIC */
		}
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xbfff, 0, 0, rbank1);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xbfff, 0, 0, wbank1);
		memory_set_bankptr(1, base1);
	}
	/* check if self-test ROM changed */
	if( changes & 0x80 )
	{
		if( new_mmu & 0x80 )
		{
			logerror("%s MMU SELFTEST RAM\n", Machine->gamedrv->name);
			rbank2 = MRA8_BANK2;
			wbank2 = MWA8_BANK2;
			base2 = memory_region(REGION_CPU1)+0x05000;  /* 0x0800 bytes */
		}
		else
		{
			logerror("%s MMU SELFTEST ROM\n", Machine->gamedrv->name);
			rbank2 = MRA8_BANK2;
			wbank2 = MWA8_ROM;
			base2 = memory_region(REGION_CPU1)+0x15000;  /* 0x0800 bytes */
		}
		memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x57ff, 0, 0, rbank2);
		memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x57ff, 0, 0, wbank2);
		memory_set_bankptr(2, base2);
	}
}


/**************************************************************
 *
 * Keyboard
 *
 **************************************************************/

#define AKEY_L			0x00
#define AKEY_J			0x01
#define AKEY_SEMICOLON	0x02
#define AKEY_BREAK		0x03	/* this not really a scancode */
#define AKEY_K			0x05
#define AKEY_PLUS		0x06
#define AKEY_ASTERISK	0x07
#define AKEY_O			0x08
#define AKEY_NONE		0x09
#define AKEY_P			0x0a
#define AKEY_U			0x0b
#define AKEY_ENTER		0x0c
#define AKEY_I			0x0d
#define AKEY_MINUS		0x0e
#define AKEY_EQUALS 	0x0f
#define AKEY_V			0x10
#define AKEY_C			0x12
#define AKEY_B			0x15
#define AKEY_X			0x16
#define AKEY_Z			0x17
#define AKEY_4			0x18
#define AKEY_3			0x1a
#define AKEY_6			0x1b
#define AKEY_ESC		0x1c
#define AKEY_5			0x1d
#define AKEY_2			0x1e
#define AKEY_1			0x1f
#define AKEY_COMMA		0x20
#define AKEY_SPACE		0x21
#define AKEY_STOP		0x22
#define AKEY_N			0x23
#define AKEY_M			0x25
#define AKEY_SLASH		0x26
#define AKEY_ATARI		0x27
#define AKEY_R			0x28
#define AKEY_E			0x2a
#define AKEY_Y			0x2b
#define AKEY_TAB		0x2c
#define AKEY_T			0x2d
#define AKEY_W			0x2e
#define AKEY_Q			0x2f
#define AKEY_9			0x30
#define AKEY_0			0x32
#define AKEY_7			0x33
#define AKEY_BSP		0x34
#define AKEY_8			0x35
#define AKEY_LESSER 	0x36
#define AKEY_GREATER	0x37
#define AKEY_F			0x38
#define AKEY_H			0x39
#define AKEY_D			0x3a
#define AKEY_CAPS		0x3c
#define AKEY_G			0x3d
#define AKEY_S			0x3e
#define AKEY_A			0x3f
#define ASHF_L			0x40
#define ASHF_J			0x41
#define ASHF_COLON		0x42
#define ASHF_BREAK		0x43	/* this not really a scancode */
#define ASHF_K			0x45
#define ASHF_BACKSLASH	0x46
#define ASHF_TILDE		0x47
#define ASHF_O			0x48
#define ASHF_SHIFT		0x49
#define ASHF_P			0x4a
#define ASHF_U			0x4b
#define ASHF_ENTER		0x4c
#define ASHF_I			0x4d
#define ASHF_UNDERSCORE 0x4e
#define ASHF_BAR		0x4f
#define ASHF_V			0x50
#define ASHF_C			0x52
#define ASHF_B			0x55
#define ASHF_X			0x56
#define ASHF_Z			0x57
#define ASHF_DOLLAR 	0x58
#define ASHF_HASH		0x5a
#define ASHF_AMPERSAND	0x5b
#define ASHF_ESC		0x5c
#define ASHF_PERCENT	0x5d
#define ASHF_DQUOTE 	0x5e
#define ASHF_EXCLAM 	0x5f
#define ASHF_LBRACE 	0x60
#define ASHF_SPACE		0x61
#define ASHF_RBRACE 	0x62
#define ASHF_N			0x63
#define ASHF_M			0x65
#define ASHF_QUESTION	0x66
#define ASHF_ATARI		0x67
#define ASHF_R			0x68
#define ASHF_E			0x6a
#define ASHF_Y			0x6b
#define ASHF_TAB		0x6c
#define ASHF_T			0x6d
#define ASHF_W			0x6e
#define ASHF_Q			0x6f
#define ASHF_LPAREN 	0x70
#define ASHF_RPAREN 	0x72
#define ASHF_QUOTE		0x73
#define ASHF_BSP		0x74
#define ASHF_AT 		0x75
#define ASHF_CLEAR		0x76
#define ASHF_INSERT 	0x77
#define ASHF_F			0x78
#define ASHF_H			0x79
#define ASHF_D			0x7a
#define ASHF_CAPS		0x7c
#define ASHF_G			0x7d
#define ASHF_S			0x7e
#define ASHF_A			0x7f
#define ACTL_L			0x80
#define ACTL_J			0x81
#define ACTL_SEMICOLON	0x82
#define ACTL_BREAK		0x83	/* this not really a scancode */
#define ACTL_K			0x85
#define ACTL_PLUS		0x86
#define ACTL_ASTERISK	0x87
#define ACTL_O			0x88
#define ACTL_CONTROL	0x89
#define ACTL_P			0x8a
#define ACTL_U			0x8b
#define ACTL_ENTER		0x8c
#define ACTL_I			0x8d
#define ACTL_MINUS		0x8e
#define ACTL_EQUALS 	0x8f
#define ACTL_V			0x90
#define ACTL_C			0x92
#define ACTL_B			0x95
#define ACTL_X			0x96
#define ACTL_Z			0x97
#define ACTL_4			0x98
#define ACTL_3			0x9a
#define ACTL_6			0x9b
#define ACTL_ESC		0x9c
#define ACTL_5			0x9d
#define ACTL_2			0x9e
#define ACTL_1			0x9f
#define ACTL_COMMA		0xa0
#define ACTL_SPACE		0xa1
#define ACTL_STOP		0xa2
#define ACTL_N			0xa3
#define ACTL_M			0xa5
#define ACTL_SLASH		0xa6
#define ACTL_ATARI		0xa7
#define ACTL_R			0xa8
#define ACTL_E			0xaa
#define ACTL_Y			0xab
#define ACTL_TAB		0xac
#define ACTL_T			0xad
#define ACTL_W			0xae
#define ACTL_Q			0xaf
#define ACTL_9			0xb0
#define ACTL_0			0xb2
#define ACTL_7			0xb3
#define ACTL_BSP		0xb4
#define ACTL_8			0xb5
#define ACTL_LESSER 	0xb6
#define ACTL_GREATER	0xb7
#define ACTL_F			0xb8
#define ACTL_H			0xb9
#define ACTL_D			0xba
#define ACTL_CAPS		0xbc
#define ACTL_G			0xbd
#define ACTL_S			0xbe
#define ACTL_A			0xbf
#define ACSH_L			0xc0
#define ACSH_J			0xc1
#define ACSH_COLON		0xc2
#define ACSH_BREAK		0xc3	/* this not really a scancode */
#define ACSH_K			0xc5
#define ACSH_BACKSLASH	0xc6
#define ACSH_TILDE		0xc7
#define ACSH_O			0xc8
#define ACSH_CTRLSHIFT	0xc9
#define ACSH_P			0xca
#define ACSH_U			0xcb
#define ACSH_ENTER		0xcc
#define ACSH_I			0xcd
#define ACSH_UNDERSCORE 0xce
#define ACSH_BAR		0xcf
#define ACSH_V			0xd0
#define ACSH_C			0xd2
#define ACSH_B			0xd5
#define ACSH_X			0xd6
#define ACSH_Z			0xd7
#define ACSH_DOLLAR 	0xd8
#define ACSH_HASH		0xda
#define ACSH_AMPERSAND	0xdb
#define ACSH_ESC		0xdc
#define ACSH_PERCENT	0xdd
#define ACSH_DQUOTE 	0xde
#define ACSH_EXCLAM 	0xdf
#define ACSH_LBRACE 	0xe0
#define ACSH_SPACE		0xe1
#define ACSH_RBRACE 	0xe2
#define ACSH_N			0xe3
#define ACSH_M			0xe5
#define ACSH_QUESTION	0xe6
#define ACSH_ATARI		0xe7
#define ACSH_R			0xe8
#define ACSH_E			0xea
#define ACSH_Y			0xeb
#define ACSH_TAB		0xec
#define ACSH_T			0xed
#define ACSH_W			0xee
#define ACSH_Q			0xef
#define ACSH_LPAREN 	0xf0
#define ACSH_RPAREN 	0xf2
#define ACSH_QUOTE		0xf3
#define ACSH_BSP		0xf4
#define ACSH_AT 		0xf5
#define ACSH_CLEAR		0xf6
#define ACSH_INSERT 	0xf7
#define ACSH_F			0xf8
#define ACSH_H			0xf9
#define ACSH_D			0xfa
#define ACSH_CAPS		0xfc
#define ACSH_G			0xfd
#define ACSH_S			0xfe
#define ACSH_A			0xff

static UINT8 keys[64][4] = {
{AKEY_NONE		 ,AKEY_NONE 	  ,AKEY_NONE	   ,AKEY_NONE		}, /* ""       CODE_NONE             */
{AKEY_ESC		 ,ASHF_ESC		  ,ACTL_ESC 	   ,ACSH_ESC		}, /*"Escape" KEYCODE_ESC              */
{AKEY_1 		 ,ASHF_EXCLAM	  ,ACTL_1		   ,ACSH_EXCLAM 	}, /* "1 !"    KEYCODE_1               */
{AKEY_2 		 ,ASHF_DQUOTE	  ,ACTL_2		   ,ACSH_DQUOTE 	}, /* "2 \""   KEYCODE_2               */
{AKEY_3 		 ,ASHF_HASH 	  ,ACTL_3		   ,ACSH_HASH		}, /* "3 #"    KEYCODE_3               */
{AKEY_4 		 ,ASHF_DOLLAR	  ,ACTL_4		   ,ACSH_DOLLAR 	}, /* "4 $"    KEYCODE_4               */
{AKEY_5 		 ,ASHF_PERCENT	  ,ACTL_5		   ,ACSH_PERCENT	}, /* "5 %"    KEYCODE_5               */
{AKEY_6 		 ,ASHF_TILDE	  ,ACTL_6		   ,ACSH_AMPERSAND	}, /* "6 ^"    KEYCODE_6               */
{AKEY_7 		 ,ASHF_AMPERSAND  ,ACTL_7		   ,ACSH_N			}, /* "7 &"    KEYCODE_7               */
{AKEY_8 		 ,AKEY_ASTERISK   ,ACTL_8		   ,ACSH_M			}, /* "8 *"    KEYCODE_8               */
{AKEY_9 		 ,ASHF_LPAREN	  ,ACTL_9		   ,ACSH_LBRACE 	}, /* "9 ("    KEYCODE_9               */
{AKEY_0 		 ,ASHF_RPAREN	  ,ACTL_0		   ,ACSH_RBRACE 	}, /* "0 )"    KEYCODE_0               */
{AKEY_MINUS 	 ,ASHF_UNDERSCORE ,ACTL_MINUS	   ,ACSH_UNDERSCORE }, /* "- _"    KEYCODE_MINUS           */
{AKEY_EQUALS	 ,AKEY_PLUS 	  ,ACTL_EQUALS	   ,ACTL_PLUS		}, /* "= +"    KEYCODE_EQUALS          */
{AKEY_BSP		 ,ASHF_BSP		  ,ACTL_BSP 	   ,ACSH_BSP		}, /* "Backsp" KEYCODE_BACKSPACE       */
{AKEY_TAB		 ,ASHF_TAB		  ,ACTL_TAB 	   ,ACSH_TAB		}, /* "Tab"    KEYCODE_TAB             */
{AKEY_Q 		 ,ASHF_Q		  ,ACTL_Q		   ,ACSH_Q			}, /* "q Q"    KEYCODE_Q               */
{AKEY_W 		 ,ASHF_W		  ,ACTL_W		   ,ACSH_W			}, /* "w W"    KEYCODE_W               */
{AKEY_E 		 ,ASHF_E		  ,ACTL_E		   ,ACSH_E			}, /* "e E"    KEYCODE_E               */
{AKEY_R 		 ,ASHF_R		  ,ACTL_R		   ,ACSH_R			}, /* "r R"    KEYCODE_R               */
{AKEY_T 		 ,ASHF_T		  ,ACTL_T		   ,ACSH_T			}, /* "t T"    KEYCODE_T               */
{AKEY_Y 		 ,ASHF_Y		  ,ACTL_Y		   ,ACSH_Y			}, /* "y Y"    KEYCODE_Y               */
{AKEY_U 		 ,ASHF_U		  ,ACTL_U		   ,ACTL_U			}, /* "u U"    KEYCODE_U               */
{AKEY_I 		 ,ASHF_I		  ,ACTL_I		   ,ACTL_I			}, /* "i I"    KEYCODE_I               */
{AKEY_O 		 ,ASHF_O		  ,ACTL_O		   ,ACTL_O			}, /* "o O"    KEYCODE_O               */
{AKEY_P 		 ,ASHF_P		  ,ACTL_P		   ,ACTL_P			}, /* "p P"    KEYCODE_P               */
{ASHF_LBRACE	 ,ACTL_COMMA	  ,ACTL_COMMA	   ,ACSH_LBRACE 	}, /* "[ {"    KEYCODE_LBRACE          */
{ASHF_RBRACE	 ,ACTL_STOP 	  ,ACTL_STOP	   ,ACSH_RBRACE 	}, /* "] }"    KEYCODE_RBRACE          */
{AKEY_ENTER 	 ,ASHF_ENTER	  ,ACTL_ENTER	   ,ACSH_ENTER		}, /* "Enter"  KEYCODE_ENTER           */
{AKEY_A 		 ,ASHF_A		  ,ACTL_A		   ,ACSH_A			}, /* "a A"    KEYCODE_A               */
{AKEY_S 		 ,ASHF_S		  ,ACTL_S		   ,ACSH_S			}, /* "s S"    KEYCODE_S               */
{AKEY_D 		 ,ASHF_D		  ,ACTL_D		   ,ACSH_D			}, /* "d D"    KEYCODE_D               */
{AKEY_F 		 ,ASHF_F		  ,ACTL_F		   ,ACSH_F			}, /* "f F"    KEYCODE_F               */
{AKEY_G 		 ,ASHF_G		  ,ACTL_G		   ,ACSH_G			}, /* "g G"    KEYCODE_G               */
{AKEY_H 		 ,ASHF_H		  ,ACTL_H		   ,ACSH_H			}, /* "h H"    KEYCODE_H               */
{AKEY_J 		 ,ASHF_J		  ,ACTL_J		   ,ACSH_J			}, /* "j J"    KEYCODE_J               */
{AKEY_K 		 ,ASHF_K		  ,ACTL_K		   ,ACSH_K			}, /* "k K"    KEYCODE_K               */
{AKEY_L 		 ,ASHF_L		  ,ACTL_L		   ,ACSH_L			}, /* "l L"    KEYCODE_L               */
{AKEY_SEMICOLON  ,ASHF_COLON	  ,ACTL_SEMICOLON  ,ACSH_COLON		}, /* "; :"    KEYCODE_COLON           */
{ASHF_QUOTE 	 ,ACSH_QUOTE	  ,ASHF_DQUOTE	   ,ACSH_DQUOTE 	}, /* "+ \\"   KEYCODE_QUOTE           */
{ASHF_QUOTE 	 ,ACSH_QUOTE	  ,ACTL_ASTERISK   ,ACSH_TILDE		}, /* "* ^"    KEYCODE_TILDE           */
{ASHF_BACKSLASH  ,ASHF_BAR		  ,ACSH_BACKSLASH  ,ACSH_BAR		}, /* "\ |"    KEYCODE_BACKSLASH       */
{AKEY_Z 		 ,ASHF_Z		  ,ACTL_Z		   ,ACSH_Z			}, /* "z Z"    KEYCODE_Z               */
{AKEY_X 		 ,ASHF_X		  ,ACTL_X		   ,ACTL_X			}, /* "x X"    KEYCODE_X               */
{AKEY_C 		 ,ASHF_C		  ,ACTL_C		   ,ACTL_C			}, /* "c C"    KEYCODE_C               */
{AKEY_V 		 ,ASHF_V		  ,ACTL_V		   ,ACTL_V			}, /* "v V"    KEYCODE_V               */
{AKEY_B 		 ,ASHF_B		  ,ACTL_B		   ,ACTL_B			}, /* "b B"    KEYCODE_B               */
{AKEY_N 		 ,ASHF_N		  ,ACTL_N		   ,ACTL_N			}, /* "n N"    KEYCODE_N               */
{AKEY_M 		 ,ASHF_M		  ,ACTL_M		   ,ACTL_M			}, /* "m M"    KEYCODE_M               */
{AKEY_COMMA 	 ,AKEY_LESSER	  ,ACTL_COMMA	   ,ACTL_LESSER 	}, /* ", ["    KEYCODE_COMMA           */
{AKEY_STOP		 ,AKEY_GREATER	  ,ACTL_STOP	   ,ACTL_GREATER	}, /* ". ]"    KEYCODE_STOP            */
{AKEY_SLASH 	 ,ASHF_QUESTION   ,ACTL_SLASH	   ,ACSH_QUESTION	}, /* "/ ?"    KEYCODE_SLASH           */
{ASHF_BACKSLASH  ,ASHF_BAR		  ,ACSH_BACKSLASH  ,ACSH_BAR		}, /* "\ |"    KEYCODE_BACKSLASH2      */
{AKEY_ATARI 	 ,ASHF_ATARI	  ,ACTL_ATARI	   ,ACSH_ATARI		}, /* "Atari"  KEYCODE_LALT            */
{AKEY_SPACE 	 ,ASHF_SPACE	  ,ACTL_SPACE	   ,ACSH_SPACE		}, /* "Space"  KEYCODE_SPACE           */
{AKEY_CAPS		 ,ASHF_CAPS 	  ,ACTL_CAPS	   ,ACSH_CAPS		}, /* "Caps"   KEYCODE_CAPSLOCK        */
{ASHF_CLEAR 	 ,ASHF_CLEAR	  ,ACSH_CLEAR	   ,ACSH_CLEAR		}, /* "Clear"  KEYCODE_HOME            */
{ASHF_INSERT	 ,ASHF_INSERT	  ,ASHF_INSERT	   ,ASHF_INSERT 	}, /* "Insert" KEYCODE_INSERT          */
{AKEY_BSP		 ,AKEY_BSP		  ,AKEY_BSP 	   ,AKEY_BSP		}, /* "Delete" KEYCODE_DEL             */
{AKEY_BREAK 	 ,ASHF_BREAK	  ,ACTL_BREAK	   ,ACSH_BREAK		}, /* "Break"  KEYCODE_PGUP            */
{ACTL_PLUS		 ,ACTL_PLUS 	  ,ACTL_PLUS	   ,ACTL_PLUS		}, /* "(Left)" KEYCODE_LEFT            */
{ACTL_ASTERISK	 ,ACTL_ASTERISK   ,ACTL_ASTERISK   ,ACTL_ASTERISK	}, /* "(Right)"KEYCODE_RIGHT           */
{ACTL_MINUS 	 ,ACTL_MINUS	  ,ACTL_MINUS	   ,ACTL_MINUS		}, /* "(Up)"   KEYCODE_UP              */
{ACTL_EQUALS	 ,ACTL_EQUALS	  ,ACTL_EQUALS	   ,ACTL_EQUALS 	}  /* "(Down)" KEYCODE_DOWN            */
};


void a800_handle_keyboard(void)
{
	static int atari_last = 0xff, joystick = 0xaa;
	int i, modifiers, atari_code;

	if( joystick != (readinputport(PORT_CONFIGURATION) & 0x80) )
	{
		joystick = readinputport(PORT_CONFIGURATION) & 0x80;
		if( joystick )
			sprintf(atari_frame_message, "Cursor Keys JOYSTICK");
		else
			sprintf(atari_frame_message, "Cursor Keys KEYBOARD");
	}

    modifiers = 0;

    /* with shift ? */
	if( code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT) )
		modifiers |= 1;

    /* with control ? */
	if( code_pressed(KEYCODE_LCONTROL) || code_pressed(KEYCODE_RCONTROL) )
		modifiers |= 2;

	for( i = 0; i < 64; i++ )
	{
		if( readinputport(PORT_KEYBOARD_BASE + i/16) & (1 << (i&15)) )
		{
			/* joystick key and joystick mode enabled ? */
			if( i >= 60 && joystick )
				continue;
			atari_code = keys[i][modifiers];
			if( atari_code != AKEY_NONE )
			{
				if( atari_code == atari_last )
					return;
				atari_last = atari_code;
				if( (atari_code & 0x3f) == AKEY_BREAK )
				{
					pokey1_break_w(atari_code & 0x40);
					return;
				}
				pokey1_kbcode_w(atari_code, 1);
				return;
			}
		}
	}
	/* remove key pressed status bit from skstat */
	pokey1_kbcode_w(AKEY_NONE, 0);
	atari_last = AKEY_NONE;
}

#define VKEY_BREAK		0x10

/* absolutely no clue what to do here :((( */
void a5200_handle_keypads(void)
{
	int i, modifiers;
	static int atari_last = 0xff;

    modifiers = 0;

    /* with shift ? */
	if (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT))
		modifiers |= 1;

    /* with control ? */
	if (code_pressed(KEYCODE_LCONTROL) || code_pressed(KEYCODE_RCONTROL))
		modifiers |= 2;

	/* check keypad */
	for (i = 0; i < 16; i++)
	{
		if( readinputport(PORT_KEYBOARD_BASE) & (1 << i) )
		{
			if( i == atari_last )
				return;
			atari_last = i;
			if( i == 0 )
			{
				pokey1_break_w(i & 0x40);
				return;
			}
			pokey1_kbcode_w((i << 1) | 0x21, 1);
			return;
		}
	}

	/* check top button */
	if ((readinputport(PORT_JOY_BUTTONS) & 0x10) == 0)
	{
		if (atari_last == 0xFE)
			return;
		pokey1_kbcode_w(0x61, 1);
		//pokey1_break_w(0x40);
		atari_last = 0xFE;
		return;
	}
	else if (atari_last == 0xFE)
		pokey1_kbcode_w(0x21, 1);

	/* remove key pressed status bit from skstat */
	pokey1_kbcode_w(0xFF, 0);
	atari_last = 0xff;
}

#ifdef MESS
DRIVER_INIT( atari )
{
	offs_t ram_top;
	offs_t ram_size;

	if (!strcmp(Machine->gamedrv->name, "a400")
		|| !strcmp(Machine->gamedrv->name, "a400pal")
		|| !strcmp(Machine->gamedrv->name, "a800")
		|| !strcmp(Machine->gamedrv->name, "a800pal")
		|| !strcmp(Machine->gamedrv->name, "a800xl"))
	{
		ram_size = 0xA000;
	}
	else
	{
		ram_size = 0x8000;
	}

	/* install RAM */
	ram_top = MIN(mess_ram_size, ram_size) - 1;
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM,
		0x0000, ram_top, 0, 0, MRA8_BANK2);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM,
		0x0000, ram_top, 0, 0, MWA8_BANK2);
	memory_set_bankptr(2, mess_ram);

	/* save states */
	state_save_register_global_pointer(((UINT8 *) &antic.r), sizeof(antic.r));
	state_save_register_global_pointer(((UINT8 *) &antic.w), sizeof(antic.w));
	state_save_register_global_pointer(((UINT8 *) &gtia.r), sizeof(gtia.r));
	state_save_register_global_pointer(((UINT8 *) &gtia.w), sizeof(gtia.w));
}
#endif

