/*************************************************************************

    Sega Z80-3D system

*************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "turbo.h"
#include "artwork.h"
#include "sound/samples.h"

/* globals */
UINT8 turbo_opa, turbo_opb, turbo_opc;
UINT8 turbo_ipa, turbo_ipb, turbo_ipc;
UINT8 turbo_fbpla, turbo_fbcol;
UINT8 turbo_speed;

UINT8 subroc3d_col, subroc3d_ply, subroc3d_chofs;

UINT8 buckrog_fchg, buckrog_mov, buckrog_obch;

/* local data */
static UINT8 segment_address, segment_increment;
static UINT8 port_8279;

static UINT8 buckrog_command;

static UINT8 old_segment_data[32];
static UINT8 new_segment_data[32];

static int segment_init;


/*******************************************

    Turbo 8255 PPI handling

*******************************************/
/*
    chip index:
    0 = IC75 - CPU Board, Sheet 6, D7
    1 = IC32 - CPU Board, Sheet 6, D6
    2 = IC123 - CPU Board, Sheet 6, D4
    3 = IC6 - CPU Board, Sheet 5, D7
*/

static WRITE8_HANDLER( turbo_opa_w )
{
	turbo_opa = data;	/* signals 0PA0 to 0PA7 */
}


static WRITE8_HANDLER( turbo_opb_w )
{
	turbo_opb = data;	/* signals 0PB0 to 0PB7 */
}


static WRITE8_HANDLER( turbo_opc_w )
{
	turbo_opc = data;	/* signals 0PC0 to 0PC7 */
}


static WRITE8_HANDLER( turbo_ipa_w )
{
	turbo_ipa = data;	/* signals 1PA0 to 1PA7 */
}


static WRITE8_HANDLER( turbo_ipb_w )
{
	turbo_ipb = data;	/* signals 1PB0 to 1PB7 */
}


static WRITE8_HANDLER( turbo_ipc_w )
{
	turbo_ipc = data;	/* signals 1PC0 to 1PC7 */
}


static WRITE8_HANDLER( turbo_pla_col_w )
{
	/* bit 0-3 = signals PLA0 to PLA3 */
	/* bit 4-6 = signals COL0 to COL2 */
	/* bit 7 = unused */
	turbo_fbpla = data & 0x0f;
	turbo_fbcol = (data & 0x70) >> 4;
}


static ppi8255_interface turbo_8255_intf =
{
	4,
	{ NULL,        NULL,        NULL,            input_port_4_r },
	{ NULL,        NULL,        NULL,            input_port_2_r },
	{ NULL,        NULL,        NULL,            NULL },
	{ turbo_opa_w, turbo_ipa_w, turbo_sound_A_w, NULL },
	{ turbo_opb_w, turbo_ipb_w, turbo_sound_B_w, NULL },
	{ turbo_opc_w, turbo_ipc_w, turbo_sound_C_w, turbo_pla_col_w }
};



/*******************************************

    Subroc3D 8255 PPI handling

*******************************************/

static WRITE8_HANDLER( subroc3d_sprite_pri_w )
{
	subroc3d_ply = data & 0x0f;
}


static WRITE8_HANDLER( subroc3d_coin_led_w )
{
	coin_counter_w(0, data & 0x01);
	coin_counter_w(1, data & 0x02);
	set_led_status(0, data & 0x04);
	subroc3d_chofs = ((data >> 4) & 1) * 3;
}


static WRITE8_HANDLER( subroc3d_palette_w )
{
	subroc3d_col = data & 0x0f;
}


static ppi8255_interface subroc3d_8255_intf =
{
	2,
	{ NULL,                  NULL },
	{ NULL,                  NULL },
	{ NULL,                  NULL },
	{ subroc3d_sprite_pri_w, subroc3d_sound_A_w },
	{ subroc3d_coin_led_w,   subroc3d_sound_B_w },
	{ subroc3d_palette_w,    subroc3d_sound_C_w }
};



/*******************************************

    Buck Rogers 8255 PPI handling

*******************************************/

static WRITE8_HANDLER( buckrog_cpu2_command_w )
{
	buckrog_command = data;
}


static WRITE8_HANDLER( buckrog_back_palette_w )
{
	buckrog_mov = data & 0x1f;
}


static WRITE8_HANDLER( buckrog_fore_palette_w )
{
	if ((data & 0x80) == 0x00)
	{
		/* this actually just touches bit 6 */
		ppi8255_set_portC(0, 0x40);

		cpunum_set_input_line(1, 0, HOLD_LINE);
	}
	buckrog_fchg = data & 0x07;
}


static WRITE8_HANDLER( buckrog_extra_w )
{
	buckrog_obch = data & 0x07;
	coin_counter_w(0, data & 0x10);
	coin_counter_w(1, data & 0x20);
	set_led_status(0, data & 0x40);
	// NOUSE = data & 0x80 -> body sonic???
}


static ppi8255_interface buckrog_8255_intf =
{
	2,
	{ NULL,                   NULL },
	{ NULL,                   NULL },
	{ NULL,                   NULL },
	{ buckrog_cpu2_command_w, buckrog_sound_A_w },
	{ buckrog_back_palette_w, buckrog_sound_B_w },
	{ buckrog_fore_palette_w, buckrog_extra_w }
};



/*******************************************

    Machine Init

*******************************************/

MACHINE_RESET( turbo )
{
	ppi8255_init(&turbo_8255_intf);
	segment_address = segment_increment = 0;
	segment_init = 1;
	port_8279 = 1;
}


MACHINE_RESET( subroc3d )
{
	ppi8255_init(&subroc3d_8255_intf);
	segment_address = segment_increment = 0;
	segment_init = 1;
	sample_start(0, 0, 1);
	sample_start(1, 1, 1);
	sample_start(2, 2, 1);
	sample_start(7, 8, 1);
}


MACHINE_RESET( buckrog )
{
	ppi8255_init(&buckrog_8255_intf);
	segment_address = segment_increment = 0;
	segment_init = 1;
	buckrog_command = 0x00;
	port_8279 = 1;
}



/*******************************************

    8279 handling
    IC84 - CPU Board, Sheet 5, C7

*******************************************/

READ8_HANDLER( turbo_8279_r )
{
	if ((offset & 1) == 0)
		return readinputport(1);  /* DSW 1 */
	else
	{
		logerror("read 0xfc%02x\n", offset);
		return 0x10;
	}
}

WRITE8_HANDLER( turbo_8279_w )
{
	switch (offset & 1)
	{
		case 0x00:
			new_segment_data[segment_address * 2] = data & 15;
			new_segment_data[segment_address * 2 + 1] = (data >> 4) & 15;
			segment_address = (segment_address + segment_increment) & 15;
			break;

		case 0x01:
			switch (data & 0xe0)
			{
				case 0x80:
					segment_address = data & 15;
					segment_increment = 0;
					break;
				case 0x90:
					segment_address = data & 15;
					segment_increment = 1;
					break;
				case 0xc0:
					memset(new_segment_data, 0, 32);
					break;
			}
			break;
	}
}


void turbo_update_segments(void)
{
	int i;

	for (i = 0; i < 32; i++)
	{
		char buf_old[8];
		char buf_new[8];

		int v_old = old_segment_data[i];
		int v_new = new_segment_data[i];

		if (segment_init || v_old != v_new)
		{
			sprintf(buf_old, "LED%02d-%c", i, v_old >= 10 ? 'X' : '0' + v_old);
			sprintf(buf_new, "LED%02d-%c", i, v_new >= 10 ? 'X' : '0' + v_new);

			artwork_show(buf_old, 0);
			artwork_show(buf_new, 1);
		}
	}

	memcpy(old_segment_data, new_segment_data, sizeof old_segment_data);

	segment_init = 0;
}


/*******************************************

    Turbo misc handling

*******************************************/

READ8_HANDLER( turbo_collision_r )
{
	return readinputport(3) | (turbo_collision & 15);
}


WRITE8_HANDLER( turbo_collision_clear_w )
{
	turbo_collision = 0;
}


WRITE8_HANDLER( turbo_coin_and_lamp_w )
{
	switch (offset & 7)
	{
		case 0:
			coin_counter_w(0, data & 1);
			break;
		case 1:
			coin_counter_w(1, data & 1);
			break;
		case 3:
			set_led_status(0, data & 1);
			break;
	}
}


void turbo_update_tachometer(void)
{
	int i;

	char buf[8] = "Speed00";

	for (i = 0; i < 16; i++)
	{
		buf[5] = '0' + i / 10;
		buf[6] = '0' + i % 10;

		artwork_show(buf, i == turbo_speed);
	}
}


/*******************************************

    Turbo ROM decoding

*******************************************/

void turbo_rom_decode(void)
{
/*
 * The table is arranged this way (second half is mirror image of first)
 *
 *      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
 *
 * 0   00 00 00 00 01 01 01 01 02 02 02 02 03 03 03 03
 * 1   04 04 04 04 05 05 05 05 06 06 06 06 07 07 07 07
 * 2   08 08 08 08 09 09 09 09 0A 0A 0A 0A 0B 0B 0B 0B
 * 3   0C 0C 0C 0C 0D 0D 0D 0D 0E 0E 0E 0E 0F 0F 0F 0F
 * 4   10 10 10 10 11 11 11 11 12 12 12 12 13 13 13 13
 * 5   14 14 14 14 15 15 15 15 16 16 16 16 17 17 17 17
 * 6   18 18 18 18 19 19 19 19 1A 1A 1A 1A 1B 1B 1B 1B
 * 7   1C 1C 1C 1C 1D 1D 1D 1D 1E 1E 1E 1E 1F 1F 1F 1F
 * 8   1F 1F 1F 1F 1E 1E 1E 1E 1D 1D 1D 1D 1C 1C 1C 1C
 * 9   1B 1B 1B 1B 1A 1A 1A 1A 19 19 19 19 18 18 18 18
 * A   17 17 17 17 16 16 16 16 15 15 15 15 14 14 14 14
 * B   13 13 13 13 12 12 12 12 11 11 11 11 10 10 10 10
 * C   0F 0F 0F 0F 0E 0E 0E 0E 0D 0D 0D 0D 0C 0C 0C 0C
 * D   0B 0B 0B 0B 0A 0A 0A 0A 09 09 09 09 08 08 08 08
 * E   07 07 07 07 06 06 06 06 05 05 05 05 04 04 04 04
 * F   03 03 03 03 02 02 02 02 01 01 01 01 00 00 00 00
 *
 */

	static const UINT8 xortable[][32]=
	{
		/* Table 0 */
		/* 0x0000-0x3ff */
		/* 0x0800-0xbff */
		/* 0x4000-0x43ff */
		/* 0x4800-0x4bff */
		{ 0x00,0x44,0x0c,0x48,0x00,0x44,0x0c,0x48,
		  0xa0,0xe4,0xac,0xe8,0xa0,0xe4,0xac,0xe8,
		  0x60,0x24,0x6c,0x28,0x60,0x24,0x6c,0x28,
		  0xc0,0x84,0xcc,0x88,0xc0,0x84,0xcc,0x88 },

		/* Table 1 */
		/* 0x0400-0x07ff */
		/* 0x0c00-0x0fff */
		/* 0x1400-0x17ff */
		/* 0x1c00-0x1fff */
		/* 0x2400-0x27ff */
		/* 0x2c00-0x2fff */
		/* 0x3400-0x37ff */
		/* 0x3c00-0x3fff */
		/* 0x4400-0x47ff */
		/* 0x4c00-0x4fff */
		/* 0x5400-0x57ff */
		/* 0x5c00-0x5fff */
		{ 0x00,0x44,0x18,0x5c,0x14,0x50,0x0c,0x48,
		  0x28,0x6c,0x30,0x74,0x3c,0x78,0x24,0x60,
		  0x60,0x24,0x78,0x3c,0x74,0x30,0x6c,0x28,
		  0x48,0x0c,0x50,0x14,0x5c,0x18,0x44,0x00 }, //0x00 --> 0x10 ?

		/* Table 2 */
		/* 0x1000-0x13ff */
		/* 0x1800-0x1bff */
		/* 0x5000-0x53ff */
		/* 0x5800-0x5bff */
		{ 0x00,0x00,0x28,0x28,0x90,0x90,0xb8,0xb8,
		  0x28,0x28,0x00,0x00,0xb8,0xb8,0x90,0x90,
		  0x00,0x00,0x28,0x28,0x90,0x90,0xb8,0xb8,
		  0x28,0x28,0x00,0x00,0xb8,0xb8,0x90,0x90 },

		/* Table 3 */
		/* 0x2000-0x23ff */
		/* 0x2800-0x2bff */
		/* 0x3000-0x33ff */
		/* 0x3800-0x3bff */
		{ 0x00,0x14,0x88,0x9c,0x30,0x24,0xb8,0xac,
		  0x24,0x30,0xac,0xb8,0x14,0x00,0x9c,0x88,
		  0x48,0x5c,0xc0,0xd4,0x78,0x6c,0xf0,0xe4,
		  0x6c,0x78,0xe4,0xf0,0x5c,0x48,0xd4,0xc0 }
	};

	static const int findtable[]=
	{
		0,1,0,1, /* 0x0000-0x0fff */
		2,1,2,1, /* 0x1000-0x1fff */
		3,1,3,1, /* 0x2000-0x2fff */
		3,1,3,1, /* 0x3000-0x3fff */
		0,1,0,1, /* 0x4000-0x4fff */
		2,1,2,1	 /* 0x5000-0x5fff */
	};

	UINT8 *RAM = memory_region(REGION_CPU1);
	int offs, i, j;
	UINT8 src;

	for (offs = 0x0000; offs < 0x6000; offs++)
	{
		src = RAM[offs];
		i = findtable[offs >> 10];
		j = src >> 2;
		if (src & 0x80) j ^= 0x3f;
		RAM[offs] = src ^ xortable[i][j];
	}
}



/*******************************************

    Subroc-3D misc handling

*******************************************/

/*******************************************

    Buck Rogers misc handling

*******************************************/

READ8_HANDLER( buckrog_cpu2_command_r )
{
	/* this actually just touches bit 6 */
	ppi8255_set_portC(0, 0x00);

	return buckrog_command;
}


READ8_HANDLER( buckrog_port_2_r )
{
	int inp1 = readinputport(2);
	int inp2 = readinputport(3);

	return  (((inp2 >> 6) & 1) << 7) |
			(((inp2 >> 4) & 1) << 6) |
			(((inp2 >> 3) & 1) << 5) |
			(((inp2 >> 0) & 1) << 4) |
			(((inp1 >> 6) & 1) << 3) |
			(((inp1 >> 4) & 1) << 2) |
			(((inp1 >> 3) & 1) << 1) |
			(((inp1 >> 0) & 1) << 0);
}


READ8_HANDLER( buckrog_port_3_r )
{
	int inp1 = readinputport(2);
	int inp2 = readinputport(3);

	return  (((inp2 >> 7) & 1) << 7) |
			(((inp2 >> 5) & 1) << 6) |
			(((inp2 >> 2) & 1) << 5) |
			(((inp2 >> 1) & 1) << 4) |
			(((inp1 >> 7) & 1) << 3) |
			(((inp1 >> 5) & 1) << 2) |
			(((inp1 >> 2) & 1) << 1) |
			(((inp1 >> 1) & 1) << 0);
}
