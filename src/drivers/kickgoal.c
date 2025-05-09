/* Driver Info


Kick Goal (c)1995 TCH
Action Hollywood (c)1995 TCH
Top Driving (c)1995 Proyesel

 prelim driver by David Haywood


todo:

Sound - Not possible without PIC dump?
  the PIC is protected, sound will have to be simulated
  the kickgoal sound rom is also bad.

Should the screen size really be doubled in kickgoal or should the fg tiles be 8bpp instead
because otherwise these don't seem much like the same hardware..

Both games have problems with the Eeprom (settings are not saved)


*/

/* Notes

68k interrupts
lev 1 : 0x64 : 0000 0000 - x
lev 2 : 0x68 : 0000 0000 - x
lev 3 : 0x6c : 0000 0000 - x
lev 4 : 0x70 : 0000 0000 - x
lev 5 : 0x74 : 0000 0000 - x
lev 6 : 0x78 : 0000 0510 - vblank?
lev 7 : 0x7c : 0000 0000 - x

*/

#include "driver.h"
#include "cpu/pic16c5x/pic16c5x.h"
#include "machine/eeprom.h"
#include "sound/okim6295.h"

/**************************************************************************
   This table converts commands sent from the main CPU, into sample numbers
   played back by the sound processor.
   All commentry and most sound effects are correct, however the music
   tracks may be playing at the wrong times.
   Accordingly, the commands for playing the below samples is just a guess:
   1A, 1B, 1C, 1D, 1E, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 6A, 6B, 6C
   Note: that samples 60, 61 and 62 combine to form a music track.
   Ditto for samples 65, 66, 67 and 68.
*/

static const UINT8 kickgoal_cmd_snd[128] =
{
/*00*/	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
/*08*/	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x70, 0x71,
/*10*/	0x72, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
/*18*/	0x15, 0x16, 0x17, 0x18, 0x19, 0x73, 0x74, 0x75,
/*20*/	0x76, 0x1a, 0x1b, 0x1c, 0x1d, 0x00, 0x1f, 0x6c,
/*28*/  0x1e, 0x65, 0x00, 0x00, 0x60, 0x20, 0x69, 0x65,
/*30*/	0x00, 0x00, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
/*38*/	0x29, 0x2a, 0x2b, 0x00, 0x6b, 0x00, 0x00, 0x00
};

/* Sound numbers in the sample ROM
01 Melody A     Bank 0
02 Melody B     Bank 0
03 Melody C     Bank 1
04 Melody D     Bank 1
05 Melody E     Bank 2
06 Melody F     Bank 2

20 Kick
21 Kick (loud)
22 Bounce
23 Bounce (loud)
24 Hit post
25 Close door
26 Gunshot
27 "You've scored!"
28 "Goal"
29 "Goal" (loud)
2a Kick (loud)
2b Throw ball
2c Coin
2d Crowd
2e Crowd (loud)
2f 27 - 29
30 Goal (in room?)
31 2B - 2D
32 2D - 2E
33 Crowd (short)
34 Crowd (shortest)

****************************************************************
Hollywood Action

01-19 Samples
21-26 Melodies Bank 0
41-48 Melodies Bank 1
61-63 Melodies Bank 2

*/


#define oki_time_base 0x08

static int kickgoal_sound;
static int kickgoal_melody;
static int kickgoal_melody_loop;
static int kickgoal_snd_bank;
static int snd_new, snd_sam[4];


UINT16 *kickgoal_fgram, *kickgoal_bgram, *kickgoal_bg2ram, *kickgoal_scrram;
UINT16 *topdrive_fgram, *topdrive_bgram, *topdrive_bg2ram, *topdrive_scrram;

WRITE16_HANDLER( kickgoal_fgram_w  );
WRITE16_HANDLER( kickgoal_bgram_w  );
WRITE16_HANDLER( kickgoal_bg2ram_w );

WRITE16_HANDLER( topdrive_fgram_w  );
WRITE16_HANDLER( topdrive_bgram_w  );
WRITE16_HANDLER( topdrive_bg2ram_w );

VIDEO_START( kickgoal );
VIDEO_UPDATE( kickgoal );

VIDEO_START( actionhw );
VIDEO_UPDATE( actionhw );

VIDEO_START( topdrive );
VIDEO_UPDATE( topdrive );


static void kickgoal_play(int melody, int data)
{
	int status = OKIM6295_status_0_r(0);

	logerror("Playing sample %01x:%02x from command %02x\n",kickgoal_snd_bank,kickgoal_sound,data);
	if (kickgoal_sound == 0) ui_popup("Unknown sound command %02x",kickgoal_sound);

	if (melody) {
		if (kickgoal_melody != kickgoal_sound) {
			kickgoal_melody      = kickgoal_sound;
			kickgoal_melody_loop = kickgoal_sound;
			if (status & 0x08)
				OKIM6295_data_0_w(0,0x40);
			OKIM6295_data_0_w(0,(0x80 | kickgoal_melody));
			OKIM6295_data_0_w(0,0x81);
		}
	}
	else {
		if ((status & 0x01) == 0) {
		OKIM6295_data_0_w(0,(0x80 | kickgoal_sound));
			OKIM6295_data_0_w(0,0x11);
		}
		else if ((status & 0x02) == 0) {
		OKIM6295_data_0_w(0,(0x80 | kickgoal_sound));
			OKIM6295_data_0_w(0,0x21);
		}
		else if ((status & 0x04) == 0) {
		OKIM6295_data_0_w(0,(0x80 | kickgoal_sound));
			OKIM6295_data_0_w(0,0x41);
		}
	}
}

WRITE16_HANDLER( kickgoal_snd_w )
{
	if (ACCESSING_LSB)
	{
		logerror("PC:%06x Writing %04x to Sound CPU\n",activecpu_get_previouspc(),data);
		if (data >= 0x40) {
			if (data == 0xfe) {
				OKIM6295_data_0_w(0,0x40);	/* Stop playing the melody */
				kickgoal_melody      = 0x00;
				kickgoal_melody_loop = 0x00;
			}
			else {
				logerror("Unknown command (%02x) sent to the Sound controller\n",data);
			}
		}
		else if (data == 0) {
			OKIM6295_data_0_w(0,0x38);		/* Stop playing effects */
		}
		else {
			kickgoal_sound = kickgoal_cmd_snd[data];

			if (kickgoal_sound >= 0x70) {
				if (kickgoal_snd_bank != 1)
					OKIM6295_set_bank_base(0, (1 * 0x40000));
				kickgoal_snd_bank = 1;
				kickgoal_play(0, data);
			}
			else if (kickgoal_sound >= 0x69) {
				if (kickgoal_snd_bank != 2)
					OKIM6295_set_bank_base(0, (2 * 0x40000));
				kickgoal_snd_bank = 2;
				kickgoal_play(4, data);
			}
			else if (kickgoal_sound >= 0x65) {
				if (kickgoal_snd_bank != 1)
					OKIM6295_set_bank_base(0, (1 * 0x40000));
				kickgoal_snd_bank = 1;
				kickgoal_play(4, data);
			}
			else if (kickgoal_sound >= 0x60) {
				kickgoal_snd_bank = 0;
					OKIM6295_set_bank_base(0, (0 * 0x40000));
				kickgoal_snd_bank = 0;
				kickgoal_play(4, data);
			}
			else {
				kickgoal_play(0, data);
			}
		}
	}
}

WRITE16_HANDLER( actionhw_snd_w )
{
	logerror("PC:%06x Writing %04x to Sound CPU - mask %04x\n",activecpu_get_previouspc(),data,mem_mask);

	if (!ACCESSING_LSB) data >>= 8;

	switch (data)
	{
		case 0xfc:	OKIM6295_set_bank_base(0, (0 * 0x40000)); break;
		case 0xfd:	OKIM6295_set_bank_base(0, (2 * 0x40000)); break;
		case 0xfe:	OKIM6295_set_bank_base(0, (1 * 0x40000)); break;
		case 0xff:	OKIM6295_set_bank_base(0, (3 * 0x40000)); break;
		case 0x78:	OKIM6295_data_0_w(0,data);
					snd_sam[0]=00; snd_sam[1]=00; snd_sam[2]=00; snd_sam[3]=00;
					break;
		default:	if (snd_new) /* Play new sample */
					{
						if ((data & 0x80) && (snd_sam[3] != snd_new))
						{
							logerror("About to play sample %02x at vol %02x\n",snd_new,data);
							if ((OKIM6295_status_0_r(0) & 0x08) != 0x08)
							{
							logerror("Playing sample %02x at vol %02x\n",snd_new,data);
								OKIM6295_data_0_w(0,snd_new);
								OKIM6295_data_0_w(0,data);
							}
							snd_new = 00;
						}
						if ((data & 0x40) && (snd_sam[2] != snd_new))
						{
							logerror("About to play sample %02x at vol %02x\n",snd_new,data);
							if ((OKIM6295_status_0_r(0) & 0x04) != 0x04)
							{
							logerror("Playing sample %02x at vol %02x\n",snd_new,data);
								OKIM6295_data_0_w(0,snd_new);
								OKIM6295_data_0_w(0,data);
							}
							snd_new = 00;
						}
						if ((data & 0x20) && (snd_sam[1] != snd_new))
						{
							logerror("About to play sample %02x at vol %02x\n",snd_new,data);
							if ((OKIM6295_status_0_r(0) & 0x02) != 0x02)
							{
							logerror("Playing sample %02x at vol %02x\n",snd_new,data);
								OKIM6295_data_0_w(0,snd_new);
								OKIM6295_data_0_w(0,data);
							}
							snd_new = 00;
						}
						if ((data & 0x10) && (snd_sam[0] != snd_new))
						{
							logerror("About to play sample %02x at vol %02x\n",snd_new,data);
							if ((OKIM6295_status_0_r(0) & 0x01) != 0x01)
							{
							logerror("Playing sample %02x at vol %02x\n",snd_new,data);
								OKIM6295_data_0_w(0,snd_new);
								OKIM6295_data_0_w(0,data);
							}
							snd_new = 00;
						}
						break;
					}
					else if (data > 0x80) /* New sample command */
					{
						logerror("Next sample %02x\n",data);
						snd_new = data;
						break;
					}
					else /* Turn a channel off */
					{
						logerror("Turning channel %02x off\n",data);
						OKIM6295_data_0_w(0,data);
						if (data & 0x40) snd_sam[3] = 00;
						if (data & 0x20) snd_sam[2] = 00;
						if (data & 0x10) snd_sam[1] = 00;
						if (data & 0x08) snd_sam[0] = 00;
						snd_new = 00;
						break;
					}
	}
}

WRITE16_HANDLER( topdrive_snd_w )
{
		// In theory this would be sample banking (it writes a value of 01 on startup)
		// however all samples addresses in header are sequential, and data after
		// the last used sample doesn't appear to be sound data anyway.
		// Furthermore no other values are ever written here
}


static int m6295_comm;
static int m6295_bank;
static UINT16 m6295_key_delay;
static INTERRUPT_GEN( kickgoal_interrupt )
{
	if ((OKIM6295_status_0_r(0) & 0x08) == 0)
	{
		switch(kickgoal_melody_loop)
		{
			case 0x060:	kickgoal_melody_loop = 0x061; break;
			case 0x061:	kickgoal_melody_loop = 0x062; break;
			case 0x062:	kickgoal_melody_loop = 0x060; break;

			case 0x065:	kickgoal_melody_loop = 0x165; break;
			case 0x165:	kickgoal_melody_loop = 0x265; break;
			case 0x265:	kickgoal_melody_loop = 0x365; break;
			case 0x365:	kickgoal_melody_loop = 0x066; break;
			case 0x066:	kickgoal_melody_loop = 0x067; break;
			case 0x067:	kickgoal_melody_loop = 0x068; break;
			case 0x068:	kickgoal_melody_loop = 0x065; break;

			case 0x063:	kickgoal_melody_loop = 0x063; break;
			case 0x064:	kickgoal_melody_loop = 0x064; break;
			case 0x069:	kickgoal_melody_loop = 0x069; break;
			case 0x06a:	kickgoal_melody_loop = 0x06a; break;
			case 0x06b:	kickgoal_melody_loop = 0x06b; break;
			case 0x06c:	kickgoal_melody_loop = 0x06c; break;

			default:	kickgoal_melody_loop = 0x00; break;
		}

		if (kickgoal_melody_loop)
		{
//          logerror("Changing to sample %02x\n",kickgoal_melody_loop);
			OKIM6295_data_0_w(0,((0x80 | kickgoal_melody_loop) & 0xff));
			OKIM6295_data_0_w(0,0x81);
		}
	}
	if ( code_pressed_memory(KEYCODE_PGUP) )
	{
		if (m6295_key_delay >= (0x60 * oki_time_base))
		{
			m6295_bank += 0x01;
			m6295_bank &= 0x03;
			if (m6295_bank == 0x03) m6295_bank = 0x00;
			ui_popup("Changing Bank to %02x",m6295_bank);
			OKIM6295_set_bank_base(0, ((m6295_bank) * 0x40000));

			if (m6295_key_delay == 0xffff) m6295_key_delay = 0x00;
			else m6295_key_delay = (0x30 * oki_time_base);
		}
		else
			m6295_key_delay += (0x01 * oki_time_base);
	}
	else if ( code_pressed_memory(KEYCODE_PGDN) )
	{
		if (m6295_key_delay >= (0x60 * oki_time_base))
		{
			m6295_bank -= 0x01;
			m6295_bank &= 0x03;
			if (m6295_bank == 0x03) m6295_bank = 0x02;
			ui_popup("Changing Bank to %02x",m6295_bank);
			OKIM6295_set_bank_base(0, ((m6295_bank) * 0x40000));

			if (m6295_key_delay == 0xffff) m6295_key_delay = 0x00;
			else m6295_key_delay = (0x30 * oki_time_base);
		}
		else
			m6295_key_delay += (0x01 * oki_time_base);
	}
	else if ( code_pressed_memory(KEYCODE_INSERT) )
	{
		if (m6295_key_delay >= (0x60 * oki_time_base))
		{
			m6295_comm += 1;
			m6295_comm &= 0x7f;
			if (m6295_comm == 0x00) { OKIM6295_set_bank_base(0, (0 * 0x40000)); m6295_bank = 0; }
			if (m6295_comm == 0x60) { OKIM6295_set_bank_base(0, (0 * 0x40000)); m6295_bank = 0; }
			if (m6295_comm == 0x65) { OKIM6295_set_bank_base(0, (1 * 0x40000)); m6295_bank = 1; }
			if (m6295_comm == 0x69) { OKIM6295_set_bank_base(0, (2 * 0x40000)); m6295_bank = 2; }
			if (m6295_comm == 0x70) { OKIM6295_set_bank_base(0, (1 * 0x40000)); m6295_bank = 1; }
			ui_popup("Sound test command %02x on Bank %02x",m6295_comm,m6295_bank);

			if (m6295_key_delay == 0xffff) m6295_key_delay = 0x00;
			else m6295_key_delay = (0x5d * oki_time_base);
		}
		else
			m6295_key_delay += (0x01 * oki_time_base);
	}
	else if ( code_pressed_memory(KEYCODE_DEL) )
	{
		if (m6295_key_delay >= (0x60 * oki_time_base))
		{
			m6295_comm -= 1;
			m6295_comm &= 0x7f;
			if (m6295_comm == 0x2b) { OKIM6295_set_bank_base(0, (0 * 0x40000)); m6295_bank = 0; }
			if (m6295_comm == 0x64) { OKIM6295_set_bank_base(0, (0 * 0x40000)); m6295_bank = 0; }
			if (m6295_comm == 0x68) { OKIM6295_set_bank_base(0, (1 * 0x40000)); m6295_bank = 1; }
			if (m6295_comm == 0x6c) { OKIM6295_set_bank_base(0, (2 * 0x40000)); m6295_bank = 2; }
			if (m6295_comm == 0x76) { OKIM6295_set_bank_base(0, (1 * 0x40000)); m6295_bank = 1; }
			ui_popup("Sound test command %02x on Bank %02x",m6295_comm,m6295_bank);

			if (m6295_key_delay == 0xffff) m6295_key_delay = 0x00;
			else m6295_key_delay = (0x5d * oki_time_base);
		}
		else
			m6295_key_delay += (0x01 * oki_time_base);
	}
	else if ( code_pressed_memory(KEYCODE_Z) )
	{
		if (m6295_key_delay >= (0x80 * oki_time_base))
		{
			OKIM6295_data_0_w(0,0x78);
			OKIM6295_data_0_w(0,(0x80 | m6295_comm));
			OKIM6295_data_0_w(0,0x11);

			ui_popup("Playing sound %02x on Bank %02x",m6295_comm,m6295_bank);

			if (m6295_key_delay == 0xffff) m6295_key_delay = 0x00;
			else m6295_key_delay = (0x60 * oki_time_base);
		}
		else
			m6295_key_delay += (0x01 * oki_time_base);
//      logerror("Sending %02x to the sound CPU\n",m6295_comm);
	}
	else m6295_key_delay = 0xffff;
}


static UINT8 *kickgoal_default_eeprom;
static int kickgoal_default_eeprom_length;

static unsigned char kickgoal_default_eeprom_type1[128] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


static NVRAM_HANDLER( kickgoal )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46);

		if (file) EEPROM_load(file);
		else
		{
			if (kickgoal_default_eeprom)	/* Sane Defaults */
				EEPROM_set_data(kickgoal_default_eeprom,kickgoal_default_eeprom_length);
		}
	}
}



static READ16_HANDLER( kickgoal_eeprom_r )
{
	if (ACCESSING_LSB)
	{
		return EEPROM_read_bit();
	}
	return 0;
}


static WRITE16_HANDLER( kickgoal_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		switch (offset)
		{
			case 0:
				EEPROM_set_cs_line((data & 0x0001) ? CLEAR_LINE : ASSERT_LINE);
				break;
			case 1:
				EEPROM_set_clock_line((data & 0x0001) ? ASSERT_LINE : CLEAR_LINE);
				break;
			case 2:
				EEPROM_write_bit(data & 0x0001);
				break;
		}
	}
}


/* Memory Maps *****************************************************************/

static ADDRESS_MAP_START( kickgoal_program_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
/// AM_RANGE(0x30001e, 0x30001f) AM_WRITE(kickgoal_snd_w)
	AM_RANGE(0x800000, 0x800001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x800002, 0x800003) AM_READ(input_port_1_word_r)
/// AM_RANGE(0x800004, 0x800005) AM_WRITE(soundlatch_word_w)
	AM_RANGE(0x800004, 0x800005) AM_WRITE(actionhw_snd_w)
	AM_RANGE(0x900000, 0x900005) AM_WRITE(kickgoal_eeprom_w)
	AM_RANGE(0x900006, 0x900007) AM_READ(kickgoal_eeprom_r)
	AM_RANGE(0xa00000, 0xa03fff) AM_READWRITE(MRA16_RAM, kickgoal_fgram_w) AM_BASE(&kickgoal_fgram) /* FG Layer */
	AM_RANGE(0xa04000, 0xa07fff) AM_READWRITE(MRA16_RAM, kickgoal_bgram_w) AM_BASE(&kickgoal_bgram) /* Higher BG Layer */
	AM_RANGE(0xa08000, 0xa0bfff) AM_READWRITE(MRA16_RAM, kickgoal_bg2ram_w) AM_BASE(&kickgoal_bg2ram) /* Lower BG Layer */
	AM_RANGE(0xa0c000, 0xa0ffff) AM_READWRITE(MRA16_RAM, MWA16_RAM) // more tilemap?
	AM_RANGE(0xa10000, 0xa1000f) AM_WRITE(MWA16_RAM) AM_BASE(&kickgoal_scrram) /* Scroll Registers */
	AM_RANGE(0xb00000, 0xb007ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size) /* Sprites */
	AM_RANGE(0xc00000, 0xc007ff) AM_READWRITE(MRA16_RAM, paletteram16_xxxxBBBBGGGGRRRR_word_w) AM_BASE(&paletteram16) /* Palette */ // actionhw reads this
	AM_RANGE(0xff0000, 0xffffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( topdrive_program_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x800000, 0x800001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x800002, 0x800003) AM_READ(input_port_1_word_r)
	// map(0x800006, 0x800007) // accessed in service menu, wheel maybe?
	AM_RANGE(0x900000, 0x900005) AM_WRITE(kickgoal_eeprom_w)
	AM_RANGE(0x900006, 0x900007) AM_READ(kickgoal_eeprom_r)
	AM_RANGE(0xa00000, 0xa03fff) AM_READWRITE(MRA16_RAM, topdrive_fgram_w) AM_BASE(&topdrive_fgram) /* FG Layer */
	AM_RANGE(0xa00400, 0xa01fff) AM_RAM
	AM_RANGE(0xa02000, 0xa03fff) AM_RAM // buffer for scroll regs? or layer configs?
	AM_RANGE(0xa04000, 0xa043ff) AM_READWRITE(MRA16_RAM, topdrive_bgram_w) AM_BASE(&topdrive_bgram) /* Higher BG Layer */
	AM_RANGE(0xa04400, 0xa07fff) AM_RAM
	AM_RANGE(0xa08000, 0xa083ff) AM_READWRITE(MRA16_RAM, topdrive_bg2ram_w) AM_BASE(&topdrive_bg2ram) /* Lower BG Layer */
	AM_RANGE(0xa08400, 0xa0bfff) AM_RAM
	AM_RANGE(0xa0c000, 0xa0c3ff) AM_RAM // seems to be a buffer for data that gets put at 0xa00000?
	AM_RANGE(0xa0c400, 0xa0ffff) AM_RAM
	AM_RANGE(0xa10000, 0xa1000f) AM_WRITE(MWA16_RAM) AM_BASE(&topdrive_scrram) /* Scroll Registers */
	AM_RANGE(0xb00000, 0xb007ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size) /* Sprites */
	AM_RANGE(0xc00000, 0xc007ff) AM_READWRITE(MRA16_RAM, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16) /* Palette */ // actionhw reads this
	AM_RANGE(0xe00003, 0xe00003) AM_READWRITE(OKIM6295_status_0_msb_r, OKIM6295_data_0_msb_w) // try msb 
	AM_RANGE(0xe00004, 0xe00005) AM_WRITE(topdrive_snd_w)
	AM_RANGE(0xf00000, 0xf2ffff) AM_RAM
	AM_RANGE(0xff0000, 0xffffff) AM_RAM
ADDRESS_MAP_END

/***************************** PIC16C57 Memory Map **************************/

	/* $000 - 7FF  PIC16C57 Internal Program ROM. Note: code is 12bits wide */
	/* $000 - 07F  PIC16C57 Internal Data RAM */

static ADDRESS_MAP_START( kickgoal_sound_io_map, ADDRESS_SPACE_IO, 8 )
	/* Unknown without the PIC dump */
ADDRESS_MAP_END

static ADDRESS_MAP_START( actionhw_io_map, ADDRESS_SPACE_IO, 8 )
	/* Unknown without the PIC dump */
ADDRESS_MAP_END


/* INPUT ports ***************************************************************/

INPUT_PORTS_START( kickgoal )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE(0x0800, IP_ACTIVE_LOW)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( topdrive )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE( 0x0800, IP_ACTIVE_LOW )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* GFX Decodes ***************************************************************/

static const gfx_layout fg816_charlayout =
{
	8,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8
};

static const gfx_layout bg1632_charlayout =
{
	16,32,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 0*16, 1*16, 1*16, 2*16,  2*16,  3*16,  3*16,  4*16,  4*16,  5*16,  5*16,  6*16,  6*16,  7*16, 7*16,
	  8*16, 8*16, 9*16, 9*16, 10*16, 10*16, 11*16, 11*16, 12*16, 12*16, 13*16, 13*16, 14*16, 14*16, 15*16, 15*16 },
	16*16
};

static const UINT32 bg3264_charlayout_yoffset[64] =
{
	0*32,  0*32,  1*32,  1*32,  2*32,  2*32,  3*32,  3*32,  4*32,  4*32,  5*32,  5*32,  6*32,  6*32,  7*32,  7*32,
	8*32,  8*32,  9*32,  9*32,  10*32, 10*32, 11*32, 11*32, 12*32, 12*32, 13*32, 13*32, 14*32, 14*32, 15*32, 15*32,
	16*32, 16*32, 17*32, 17*32, 18*32, 18*32, 19*32, 19*32, 20*32, 20*32, 21*32, 21*32, 22*32, 22*32, 23*32, 23*32,
	24*32, 24*32, 25*32, 25*32, 26*32, 26*32, 27*32, 27*32, 28*32, 28*32, 29*32, 29*32, 30*32, 30*32, 31*32, 31*32
};

static const gfx_layout bg3264_charlayout =
{
	32,64,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
	},
	EXTENDED_YOFFS,
	32*32,
	NULL,
	bg3264_charlayout_yoffset
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fg816_charlayout,   0x000, 0x40 },
	{ REGION_GFX1, 0, &bg1632_charlayout,  0x000, 0x40 },
	{ REGION_GFX1, 0, &bg3264_charlayout,  0x000, 0x40 },
	{ -1 } /* end of array */
};

static const gfx_layout actionhw_fg88_alt_charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8,  2*8,  3*8,  4*8, 5*8, 6*8,  7*8 },
	8*8
};


static const gfx_layout actionhw_bg1616_charlayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16,  1*16,  2*16,  3*16,   4*16,   5*16,   6*16,   7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};



static const gfx_decode actionhw_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &actionhw_fg88_alt_charlayout,   0x000, 0x40 },
	{ REGION_GFX1, 0, &actionhw_bg1616_charlayout,  0x000, 0x40 },
	{ -1 } /* end of array */
};

static const gfx_layout topdrive_bg1616_charlayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16,  1*16,  2*16,  3*16,   4*16,   5*16,   6*16,   7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static const gfx_decode topdrive_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &topdrive_bg1616_charlayout,  0x000, 0x40 },
	{ -1 } /* end of array */
};

/* MACHINE drivers ***********************************************************/



static MACHINE_DRIVER_START( kickgoal )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_PROGRAM_MAP(kickgoal_program_map, 0)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)
	MDRV_CPU_PERIODIC_INT(kickgoal_interrupt, TIME_IN_HZ(240))

	MDRV_CPU_ADD(PIC16C57, ((12000000/4)/PIC16C5x_CLOCK_DIVIDER))	/* 3MHz ? */
	MDRV_CPU_FLAGS(CPU_DISABLE)	/* Disables since the internal rom isn't dumped */
	/* Program and Data Maps are internal to the MCU */
	MDRV_CPU_IO_MAP(kickgoal_sound_io_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(kickgoal)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(9*8, 55*8-1, 4*8, 60*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(kickgoal)
	MDRV_VIDEO_UPDATE(kickgoal)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(OKIM6295, 12000000/8/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( actionhw )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_PROGRAM_MAP(kickgoal_program_map, 0)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(PIC16C57, ((12000000/4)/PIC16C5x_CLOCK_DIVIDER))	/* 3MHz ? */
	MDRV_CPU_FLAGS(CPU_DISABLE) /* Disables since the internal rom isn't dumped */
	/* Program and Data Maps are internal to the MCU */
	MDRV_CPU_IO_MAP(actionhw_io_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(kickgoal) // 93C46 really

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(10*8+2, 54*8-1+2, 0*8, 30*8-1)
	MDRV_GFXDECODE(actionhw_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(actionhw)
	MDRV_VIDEO_UPDATE(actionhw)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(OKIM6295, 12000000/8/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( topdrive )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_PROGRAM_MAP(topdrive_program_map, 0)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(kickgoal) // 93C46 really

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(topdrive_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x400)

	MDRV_VIDEO_START(topdrive)
	MDRV_VIDEO_UPDATE(topdrive)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(OKIM6295, 1000000/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.95)
MACHINE_DRIVER_END



/* Rom Loading ***************************************************************/

ROM_START( kickgoal )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "ic6",   0x000000, 0x40000, CRC(498ca792) SHA1(c638c3a1755870010c5961b58bcb02458ff4e238) )
	ROM_LOAD16_BYTE( "ic5",   0x000001, 0x40000, CRC(d528740a) SHA1(d56a71004aabc839b0833a6bf383e5ef9d4948fa) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )	/* sound? (missing) */
	/* Remove the CPU_DISABLED flag in MACHINE_DRIVER when the rom is dumped */
	ROM_LOAD( "pic16c57",     0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic33",   0x000000, 0x80000, CRC(5038f52a) SHA1(22ed0e2c8a99056e73cff912731626689996a276) )
	ROM_LOAD( "ic34",   0x080000, 0x80000, CRC(06e7094f) SHA1(e41b893ef91d541d2623d76ce6c69ecf4218c16d) )
	ROM_LOAD( "ic35",   0x100000, 0x80000, CRC(ea010563) SHA1(5e474db372550e9d33f933ab00881a9b29a712d1) )
	ROM_LOAD( "ic36",   0x180000, 0x80000, CRC(b6a86860) SHA1(73ab43830d5e62154bc8953615cdb397c7a742aa) )

	/* $00000-$20000 stays the same in all sound banks, */
	/* the second half of the bank is the area that gets switched */
	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic13",        0x00000, 0x40000, BAD_DUMP CRC(c6cb56e9) SHA1(835773b3f0647d3c553180bcf10e57ad44d68353) ) // BAD ADDRESS LINES (mask=010000)
	ROM_CONTINUE(            0x60000, 0x20000 )
	ROM_CONTINUE(            0xa0000, 0x20000 )
	ROM_COPY( REGION_SOUND1, 0x00000, 0x40000, 0x20000)
	ROM_COPY( REGION_SOUND1, 0x00000, 0x80000, 0x20000)
	ROM_COPY( REGION_SOUND1, 0x00000, 0xc0000, 0x20000)
ROM_END

ROM_START( actionhw )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "2.ic6",  0x000000, 0x80000, CRC(2b71d58c) SHA1(3e58531fa56d41a3c7944e3beab4850907564a89) )
	ROM_LOAD16_BYTE( "1.ic5",  0x000001, 0x80000, CRC(136b9711) SHA1(553f9fdd99bb9ce2e1492d0755633075e59ba587) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )	/* sound? (missing) */
	/* Remove the CPU_DISABLED flag in MACHINE_DRIVER when the rom is dumped */
	ROM_LOAD( "pic16c57",     0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "4.ic29",  0x000000, 0x80000, CRC(df076744) SHA1(4b2c8e21a201e1491e4ba3cda8d71b51e0943431) )
	ROM_LOAD( "5.ic33",  0x080000, 0x80000, CRC(8551fdd4) SHA1(f29bdfb75af7607534de171d7b3927419c00377c) )
	ROM_LOAD( "6.ic30",  0x100000, 0x80000, CRC(5cb005a5) SHA1(d3a5ab8f9a520bfaa53fdf6145142ccba416fbb8) )
	ROM_LOAD( "7.ic34",  0x180000, 0x80000, CRC(c2f7d284) SHA1(b3c3d6aa932c813affd667344ea5ddefa55f219b) )
	ROM_LOAD( "8.ic31",  0x200000, 0x80000, CRC(50dffa47) SHA1(33da3b2cabb7b0e480158d343e876563bd0f0930) )
	ROM_LOAD( "9.ic35",  0x280000, 0x80000, CRC(c1ea0370) SHA1(c836611e478d2bf9ae2a5d7e7665982c2b731189) )
	ROM_LOAD( "10.ic32", 0x300000, 0x80000, CRC(5ee5db3e) SHA1(c79f84548ce5311acac478c5180330bf56485863) )
	ROM_LOAD( "11.ic36", 0x380000, 0x80000, CRC(8d376b1e) SHA1(37f16b3237d9813a8d153ab5640252e7643f3b99) )

	/* $00000-$20000 stays the same in all sound banks, */
	/* the second half of the bank is the area that gets switched */
	ROM_REGION( 0x100000, REGION_SOUND1, 0 )    /* OKIM6295 samples */
	ROM_LOAD( "3.ic13",      0x00000, 0x40000, CRC(b8f6705d) SHA1(55116e14aba6dac7334e26f704b3e6b0b9f856c2) )
	ROM_CONTINUE(            0x60000, 0x20000 )
	ROM_CONTINUE(            0xa0000, 0x20000 )
	ROM_COPY( REGION_SOUND1, 0x00000, 0x40000, 0x20000)
	ROM_COPY( REGION_SOUND1, 0x00000, 0x80000, 0x20000)
	ROM_COPY( REGION_SOUND1, 0x00000, 0xc0000, 0x20000) /* Last bank used in Test Mode */
ROM_END

ROM_START( topdrive )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2-27c040.bin", 0x00000, 0x80000, CRC(37798c4e) SHA1(708a64b416bd2104fbc4b72a37bfeae33bbab454) )
	ROM_LOAD16_BYTE( "1-27c040.bin", 0x00001, 0x80000, CRC(e2dc5096) SHA1(82b22e03be225ab7f20eff6314383a9f28d52294) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "4-27c040.bin",  0x000000, 0x80000, CRC(a81ca7f7) SHA1(cc2030a9bea90b694adbf222389766945ce9552b) )
	ROM_LOAD( "5-27c040.bin",  0x080000, 0x80000, CRC(a756d2b2) SHA1(59ddef858850b0f6c5865d555d6402c41cc3cb6c) )
	ROM_LOAD( "6-27c040.bin",  0x100000, 0x80000, CRC(90c778a2) SHA1(8122ee085e388bb1f7952edb6a99dffc466f2e2c) )
	ROM_LOAD( "7-27c040.bin",  0x180000, 0x80000, CRC(db219087) SHA1(c79145555678971db29e91a24d69738da7d8f07f) )
	ROM_LOAD( "8-27c040.bin",  0x200000, 0x80000, CRC(0e5f4419) SHA1(4fc8173001e2b412f4a7b0b5160c853436bbb139) )
	ROM_LOAD( "9-27c040.bin",  0x280000, 0x80000, CRC(159a7426) SHA1(6851fbc1fe11ae72a86d35011730d2df641e8fc5) )
	ROM_LOAD( "10-27c040.bin", 0x300000, 0x80000, CRC(54c1617a) SHA1(7bb4faaa54581f080f19f98e78fa9cae899f4c2a) )
	ROM_LOAD( "11-27c040.bin", 0x380000, 0x80000, CRC(6b3c3c73) SHA1(8ac76abdc4676cfcd9dc66a4c7b55010de099133) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "3-27c040.bin",      0x00000, 0x80000, CRC(2894b89b) SHA1(cf884042edd2fc05e04d21ccd36f5183f9a7ec5c) )
ROM_END

/* GAME drivers **************************************************************/

DRIVER_INIT( kickgoal )
{
#if 0 /* we should find a real fix instead  */
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);

	/* fix "bug" that prevents game from writing to EEPROM */
	rom[0x12b0/2] = 0x0001;
#endif

	kickgoal_default_eeprom = kickgoal_default_eeprom_type1;
	kickgoal_default_eeprom_length = sizeof(kickgoal_default_eeprom_type1);
}


GAME( 1995, kickgoal,0, kickgoal, kickgoal, kickgoal, ROT0, "TCH", "Kick Goal", GAME_NO_SOUND )
GAME( 1995, actionhw,0, actionhw, kickgoal, kickgoal, ROT0, "TCH", "Action Hollywood", GAME_IMPERFECT_SOUND )
GAME( 1995, topdrive,0, topdrive, topdrive, kickgoal, ROT0, "Proyesel", "Top Driving (version 1.1)", GAME_IMPERFECT_SOUND )
