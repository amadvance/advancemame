// license:BSD-3-Clause
// copyright-holders:David Haywood, Luca Elia, MetalliC
/*
license:BSD-3-Clause
copyright-holders:David Haywood, Luca Elia, MetalliC

U2 flash rom note

Cave often programmed the u2 roms onto defective flash chips, programming around the bad blocks.
As a result these are highly susceptible to failure, blocks around the known bad blocks appear to
decay at an alarming rate in some cases, and in others data has clearly been programmed over
blocks that were already going bad. - this is why the same game often has different u2 roms
in the different sets at the moment. - these will be reconstructed at some point.

The flash roms do contain a 'bad block' table, so it should be possible to rebuild a 'clean'
flash ROM for each game by comparing multiple dumps of each game and ensuring no other data has
decayed.  Naturally this is not an ideal situation for the less common games!

----

Cave CV1000 hardware

Games by Cave ID number:

CA011  Mushihime-Sama
CA012  Ibara
CA012B Ibara Kuro Black Label
CA013  Espgaluda II
CA014  Pink Sweets: Ibara Sorekara
CA015  Mushihime-Sama Futari
CA015B Mushihime-Sama Futari Black Label
CA016  Muchi Muchi Pork!
CA017  Deathsmiles
CA017B Deathsmiles Black Label
CA019  Do-Don-Pachi Dai-Fukkatsu
CA019B Do-Don-Pachi Dai-Fukkatsu Black Label
CA021  Akai Katana

CMDL01 Medal Mahjong Moukari Bancho
       Pirates of Gappori http://web.archive.org/web/20090907145501/http://www.cave.co.jp/gameonline/gappori/
       Oooku http://web.archive.org/web/20141104001322/http://www.cave.co.jp/gameonline/oooku/

Note: CA018 - Deathsmiles II: Makai no Merry Christmas on unknown custom platform
      CA020 - Do-Don-Pachi Dai-ou-jou Tamashii on PGM2 platform

PCB CV1000-B / CV1000-D
+--------------------------------------------+
|                                            |
|                                            |
|                                            |
|                 VOL                        |
|                                            |
+-+        +-----+ +-----+       X3          |
  |        | U24 | | U23 |                   |
+-+        +-----+ +-----+    +------+       |
|                             |Yamaha|       |
|            U25*    U26*     |YMZ770|       |
|                             |      |       |
|J  D5                        +------+       |
|A  D2                                       |
|M  D3                    +-----+ +-----+    |
|M  D4         P2*        | U7  | | U6  |    |
|A  D1                    +-----+ +-----+    |
|          +-------+                         |
|C         |P4 JTAG|                         |
|o         +-------+       +-------+         |
|n                         |Altera |  +--+   |
|n          D6             |Cyclone|  |  |   |
|e     X1      S1          |       |  |U1|   |
|c    S3 +---------+       +-------+  |  |   |
|t       |         |                  +--+   |
|e       | Hitachi |                         |
|r       |         |     S2                  |
|        |   SH3   |   +-----------------+   |
|        |         |   |       P3        |   |
|        +---------+   +-----------------+   |
+-+    X2                                    |
  |       +--+  +--+    +---+  U27           |
+-+       |  |  |  |    |U13|                |
|+-+      |U2|  |U4|    +---+                |
||P|      |  |  |  |                 __      |
||8| U12* +--+  +--+     U10        /  \     |
|+-+                               |C126|    |
|     P5* P7*                       \__/     |
+--------------------------------------------+

* Denotes not populated

  CPU: Hitachi 6417709S SH3 clocked at 102.4MHz (12.800MHz * 8)
Sound: Yamaha YMZ770C-F clocked at 16.384MHz
Other: Altera Cyclone EPIC12 FPGA
       Altera EPM7032 (MAX 7000 Series) at U13

OSC:
 X1 12.800MHz (SH3 clock derived from this)
 X2 32.768kHz (Used by the RTC)
 X3 16.384MHz (Yamaha YMZ770C-F clock)

Memory:
 U6 (SDRAM)  MT46V16M16 ? 4 MBit x 16 x 4 banks, RAM (256 MBit)
 U7 (SDRAM)  MT46V16M16 ? 4 MBit x 16 x 4 banks, RAM (256 MBit)
 U1 (SDRAM)  MT48LC2M32 ? 512K x 32 x 4 banks, (64 MBit) for CV1000-B
 U1 (SDRAM)  IS42S32400 - 1024K x 32 x 4 banks, (128 MBit) for CV1000-D

Roms:
      U4 (FLASH)  29LV160BB 16M-Bit CMOS 3.0V, Boot device, FPGA bit file, main program code for CV1000-B
      U4 (FLASH)  S29JL032H 32M-Bit CMOS 3.0V, Boot device, FPGA bit file, main program code for CV1000-D
      U2 (FLASH)  K9F1G08U0M 128M x 8 Bit / 64M x 16 Bit NAND. Graphics data.
 U23-U24 (FLASH)  MBM 29DL321, 32M-Bit CMOS 3.0V. Sound data.
 U25-U26 (FLASH)  MBM 29DL321, not populated

Battery:
 C126 CR2450, Powers the RTC (Real Time Clock) U10. Look at the garden clock in Ibara. NOT present on CV1000-D

Dipswitches & Push Buttons:
 S1 (DIL SWITCH) Half Pitch DIL Switch x 1, function unknown
 S2 (DIL SWITCH) Half Pitch DIL Switch x 4, SW1=Setup, other switches unknown
 S3 (MICRO PUSH BUTTON) Test switch, same as on the JAMMA connector

Connectors:
 P2 (IDC CONNECTOR 20 PIN) function unknown, P2 is not always mounted
 P4 (IDC CONNECTOR 14 PIN) JTAG connector
 P8 (IDC CONNECTOR 10 PIN) Advanced User Debugger
 P3 (CONNECTOR) Most likely an expansion port, P3 is not always mounted
 P5 (CONNECTOR) D9 serial connector. Used for the mahjong Touchscreen titles.  Also mounted on early Mushihime-Sama PCB's
 P7 (CONNECTOR) Network port pinout. Never seen mounted on any PCB.

Misc:
   U27 (SUPERVISOR) MAX 690S 3.0V Microprocessor Supervisory Circuit.
   U10 (RTC & EEPROM) RTC 9701, Serial RTC Module with EEPROM 4 kbit (256x16 bit), controlled by Altera EPM7032 U13.
   U12 (RS-232 TRANSCEIVER) MAX 3244E RS-232 Transceiver, only mounted when P5 is mounted.
 D1-D6 (LED) Status LED's. D6 lights up at power on then shuts off, D2 indicates coinage.

Note: * The Altera EPM7032 usually stamped / labeled with the Cave game ID number as listed above.
      * Actual flash ROMs will vary by manufacturer but will be compatible with flash ROM listed.
      * The CV1000-D revision PCB has double the RAM at U1, double the ROM at U4 and no battery.
        The CV1000-D is used for Dodonpachi Daifukkatsu and later games. Commonly referred to as SH3B PCB.

Information by The Sheep, rtw, Ex-Cyber, BrianT & Guru

------------------------------------------------------

 To enter service mode in most cases hold down 0 (Service 2) for a few seconds
  (I believe it's the test button on the PCB)
 Some games also use the test dipswitch as an alternative method.

ToDo:

Improve Blending precision?
 - I'm not sure what precision the original HW mixes with, source data is 555 RGB with 1 bit transparency (16-bits)
   and the real VRAM is also clearly in this format.  The Alpha values supplied however are 8bpp, and the 'Tint'
   values use 0x20 for 'normal' (not 0x1f)

Overall screen brightness / contrast (see test mode)
 - Could convert ram back to 16-bit and use a palette lookup at the final blit.. probably easiest / quickest.

Touchscreen
 - Used for mmmbanc, needs SH3 serial support.

Remaining Video issues
 - measure h/v video timing
 - mmpork startup screen flicker - the FOR USE IN JAPAN screen doesn't appear on the real PCB until after the graphics are fully loaded, it still displays 'please wait' until that point.
 - is the use of the 'scroll' registers 100% correct? (related to above?)
 - Sometimes the 'sprites' in mushisam lag by a frame vs the 'backgrounds' is this a timing problem, does the real game do it?

Speedups
 - Blitter is already tightly optimized
 - Need SH3 recompiler?

Blitter Timing
 - Correct slowdown emulation and flags (depends on blit mode, and speed of RAM) - could do with the recompiler or alt idle skips on the busy flag wait loops
 - End of Blit IRQ? (one game has a valid irq routine that looks like it was used for profiling, but nothing depends on it)

*/

#include "driver.h"
#include "cpu/sh3/sh3.h"
#include "includes/cv1k.h"
#include "sound/custom.h"

static UINT32 *cv1k_ram;
static size_t cv1k_ram_size;
static int cv1k_boot_patch_frame;

#define LANE_ACCESSED(lane)		((mem_mask & (0xff000000 >> ((lane) * 8))) == 0)
#define LANE_DATA(lane)			((data >> (24 - (lane) * 8)) & 0xff)

static READ32_HANDLER(flash_r)
{
	UINT32 result = 0;
	int lane;

	for (lane = 0; lane < 4; lane++)
	{
		if (LANE_ACCESSED(lane))
		{
			UINT32 byte_offset = offset * 4 + lane;
			UINT8 d = 0xff;
			sh3_update_access_cycles(0xb0000000 + byte_offset, 0);
			if (byte_offset == 0)
				d = cv1k_flash_io_r();
			result |= (UINT32)d << (24 - lane * 8);
		}
	}
	return result;
}

static WRITE32_HANDLER(flash_w)
{
	int lane;

	for (lane = 0; lane < 4; lane++)
	{
		if (LANE_ACCESSED(lane))
		{
			UINT32 byte_offset = offset * 4 + lane;
			sh3_update_access_cycles(0xb0000000 + byte_offset, 1);
			switch (byte_offset)
			{
				case 0: cv1k_flash_data_w(LANE_DATA(lane)); break;
				case 1: cv1k_flash_cmd_w(LANE_DATA(lane)); break;
				case 2: cv1k_flash_addr_w(LANE_DATA(lane)); break;
				default: break;
			}
		}
	}
}

static WRITE32_HANDLER(ymz_w)
{
	int lane;

	for (lane = 0; lane < 4; lane++)
	{
		if (LANE_ACCESSED(lane))
		{
			UINT32 byte_offset = offset * 4 + lane;
			sh3_update_access_cycles(0xb0400000 + byte_offset, 1);
			cv1k_ymz_write(byte_offset, LANE_DATA(lane));
		}
	}
}

static READ32_HANDLER(rtc_r)
{
	UINT32 result = 0;
	int lane;

	for (lane = 0; lane < 4; lane++)
	{
		if (LANE_ACCESSED(lane))
		{
			UINT32 byte_offset = offset * 4 + lane;
			UINT8 d = 0;
			sh3_update_access_cycles(0xb0c00000 + byte_offset, 0);
			if (byte_offset == 1)
				d = 0xfe | cv1k_rtc_read_bit();
			result |= (UINT32)d << (24 - lane * 8);
		}
	}
	return result;
}

static WRITE32_HANDLER(rtc_w)
{
	int lane;

	for (lane = 0; lane < 4; lane++)
	{
		if (LANE_ACCESSED(lane))
		{
			UINT32 byte_offset = offset * 4 + lane;
			sh3_update_access_cycles(0xb0c00000 + byte_offset, 1);
			if (byte_offset == 1)
			{
				UINT8 value = LANE_DATA(lane);
				cv1k_rtc_write_bit(value & 1);
				cv1k_rtc_set_clock_line((value >> 1) & 1);
				cv1k_rtc_set_cs_line((value & 4) ? CLEAR_LINE : ASSERT_LINE);
			}
			else if (byte_offset == 3)
			{
				cv1k_flash_enab_w(LANE_DATA(lane));
			}
		}
	}
}

static UINT8 cv1k_port_r(int port)
{
	switch (port)
	{
		case SH3_PORT_C: return readinputportbytag("PORT_C");
		case SH3_PORT_D: return readinputportbytag("PORT_D");
		case SH3_PORT_E: return 0xdf | (cv1k_flash_ready_r() ? 0x20 : 0);
		case SH3_PORT_F: return readinputportbytag("PORT_F");
		case SH3_PORT_L: return readinputportbytag("PORT_L");
		case SH3_PORT_J: return 0xff;
	}
	return 0;
}

static void cv1k_port_w(int port, UINT8 data)
{
}

static struct sh3_config cv1k_sh3_config =
{
	1,					/* cache and bus timing */
	102400000 / 4,		/* peripheral clock */
	cv1k_port_r,
	cv1k_port_w
};

static ADDRESS_MAP_START(cv1k_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x003fffff) AM_ROM AM_REGION(REGION_CPU1, 0)
	AM_RANGE(0x0c000000, 0x0c7fffff) AM_RAM AM_BASE(&cv1k_ram) AM_SIZE(&cv1k_ram_size)
	AM_RANGE(0x10000000, 0x10000007) AM_READWRITE(flash_r, flash_w)
	AM_RANGE(0x10400000, 0x10400007) AM_WRITE(ymz_w)
	AM_RANGE(0x10c00000, 0x10c00007) AM_READWRITE(rtc_r, rtc_w)
	AM_RANGE(0x18000000, 0x18000057) AM_READWRITE(cv1k_blitter_r, cv1k_blitter_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(cv1k_d_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x003fffff) AM_ROM AM_REGION(REGION_CPU1, 0)
	AM_RANGE(0x0c000000, 0x0cffffff) AM_RAM AM_BASE(&cv1k_ram) AM_SIZE(&cv1k_ram_size)
	AM_RANGE(0x10000000, 0x10000007) AM_READWRITE(flash_r, flash_w)
	AM_RANGE(0x10400000, 0x10400007) AM_WRITE(ymz_w)
	AM_RANGE(0x10c00000, 0x10c00007) AM_READWRITE(rtc_r, rtc_w)
	AM_RANGE(0x18000000, 0x18000057) AM_READWRITE(cv1k_blitter_r, cv1k_blitter_w)
ADDRESS_MAP_END
static INPUT_PORTS_START( cv1k_base )
	PORT_START_TAG("DSW")

	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT(0xfffffff0, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START_TAG("PORT_C")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("PORT_D")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1        ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2        ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3        ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4        ) PORT_PLAYER(1)

	PORT_START_TAG("PORT_F")
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_SERVICE3 )
	PORT_BIT( 0xfd, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("PORT_L")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1        ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2        ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3        ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4        ) PORT_PLAYER(2)
INPUT_PORTS_END

static INPUT_PORTS_START( cv1k )
	PORT_INCLUDE( cv1k_base )

	PORT_MODIFY("PORT_F")
	PORT_SERVICE_NO_TOGGLE( 0x02, IP_ACTIVE_LOW )
INPUT_PORTS_END

static INPUT_PORTS_START( cv1ks )
	PORT_INCLUDE( cv1k_base )

	PORT_MODIFY("DSW")
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x02, 0x00, "Special Mode" ) PORT_DIPLOCATION("S2:2")
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x02, DEF_STR( On ) )
INPUT_PORTS_END

static NVRAM_HANDLER(cv1k)
{
	cv1k_rtc_nvram(file, read_or_write);
	cv1k_flash_nvram(file, read_or_write);
}
static MACHINE_RESET(cv1k)
{
	cv1k_boot_patch_frame = 0;
	cv1k_flash_enab_w(0);
	cv1k_flash_hard_reset();
	cv1k_video_reset();
}
static VIDEO_START(cv1k)
{
	cv1k_video_init(cv1k_ram, cv1k_ram_size);
	return 0;
}
static struct CustomSound_interface cv1k_sound = { cv1k_ymz_start, NULL, cv1k_ymz_reset, NULL };
static MACHINE_DRIVER_START(cv1k)
	MDRV_CPU_ADD_TAG("main", SH3BE, 102400000)
	MDRV_CPU_CONFIG(cv1k_sh3_config)
	MDRV_CPU_PROGRAM_MAP(cv1k_map, 0)
	MDRV_CPU_VBLANK_INT(irq2_line_hold, 1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_MACHINE_RESET(cv1k)
	MDRV_NVRAM_HANDLER(cv1k)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(65536)
	MDRV_VIDEO_START(cv1k)
	MDRV_VIDEO_UPDATE(cv1k)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 16384000)
	MDRV_SOUND_CONFIG(cv1k_sound)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END
static MACHINE_DRIVER_START(cv1k_d)
	MDRV_IMPORT_FROM(cv1k)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(cv1k_d_map, 0)
MACHINE_DRIVER_END
static void cv1k_init(void)
{
	cv1k_sh3_install();
	cv1k_flash_init(REGION_USER1);
	cv1k_rtc_init();
}
static UINT32 cv1k_idle_offset;
static UINT32 cv1k_idle_pc;
/* The program is copied/decompressed into work RAM at boot.  Recognize the
 * memory-check function there, rather than keeping addresses for each set.
 * Its caller treats R0 == 0 as completion.  The EEPROM task is separate and
 * must still run to load/initialize the saved settings.
 */
static void cv1k_patch_memory_check(void)
{
	static const char message[] = "   CHECKING MEMORY.....";
	UINT32 offset, literal, target = 0;
	int matches = 0;

	for (offset = 0; offset + 0x400 <= cv1k_ram_size; offset += 4)
	{
		int has_message = 0, has_result = 0;

		/* Save R8-R14, initialize R9, and keep the task pointer in R10. */
		if (cv1k_ram[offset / 4] != 0x2f862f96 ||
			cv1k_ram[offset / 4 + 1] != 0x2fa6e900 ||
			cv1k_ram[offset / 4 + 2] != 0x2fb66a43 ||
			cv1k_ram[offset / 4 + 3] != 0x2fc62fd6 ||
			(cv1k_ram[offset / 4 + 4] & 0xffffff00) != 0x2fe6d800)
			continue;

		for (literal = offset + 20; literal + 8 <= offset + 0x400; literal += 2)
		{
			UINT16 *ram16 = (UINT16 *)cv1k_ram;

			/* CMP/EQ #3,R0; MOVT R10; TST R10,R10; MOVT R0. */
			if (ram16[WORD_XOR_BE(literal) / 2] == 0x8803 &&
				ram16[WORD_XOR_BE(literal + 2) / 2] == 0x0a29 &&
				ram16[WORD_XOR_BE(literal + 4) / 2] == 0x2aa8 &&
				ram16[WORD_XOR_BE(literal + 6) / 2] == 0x0029)
				has_result = 1;

			if (!(literal & 3))
			{
				UINT32 address = cv1k_ram[literal / 4];
				if (address >= 0x0c000000 &&
					address - 0x0c000000 <= cv1k_ram_size - sizeof(message))
				{
					UINT32 text = address - 0x0c000000;
					unsigned i;
					for (i = 0; i < sizeof(message) - 1; ++i)
						if (((UINT8 *)cv1k_ram)[BYTE4_XOR_BE(text + i)] != message[i])
							break;
					if (i == sizeof(message) - 1)
						has_message = 1;
				}
			}
		}
		if (has_message && has_result)
		{
			target = offset;
			++matches;
		}
	}

	/* Unknown or ambiguous program revisions retain their original checks. */
	if (matches == 1)
	{
		/* MOV #0,R0; RTS; NOP (SH-3 delay slot). */
		cv1k_ram[target / 4] = 0xe000000b;
		cv1k_ram[target / 4 + 1] =
			(cv1k_ram[target / 4 + 1] & 0x0000ffff) | 0x00090000;
		cv1k_boot_patch_frame = -1;
		logerror("CV1K: skipped boot RAM check at %08x\n", 0x0c000000 + target);
	}
}

static READ32_HANDLER(cv1k_speedup_r)
{
	offs_t pc = activecpu_get_pc();
	if (pc == cv1k_idle_pc || pc == cv1k_idle_pc + 2 || pc == cv1k_idle_pc + 8)
	{
		/* Patch only at the idle loop, outside the function being replaced.
		 * Retry during decompression, then stop scanning after boot.
		 */
		if (cv1k_boot_patch_frame >= 0)
		{
			int frame = cpu_getcurrentframe();
			if (frame >= cv1k_boot_patch_frame)
			{
				cv1k_boot_patch_frame = frame < 600 ? frame + 30 : -1;
				cv1k_patch_memory_check();
			}
		}
		activecpu_eat_cycles(1024);
	}
	return cv1k_ram[cv1k_idle_offset / 4 + offset];
}
static void cv1k_install_speedup(UINT32 offset, UINT32 pc)
{
	cv1k_init();
	cv1k_idle_offset = offset;
	cv1k_idle_pc = pc;
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x0c000000 + offset, 0x0c000007 + offset, 0, 0, cv1k_speedup_r);

	/* fast memory (sh3_set_fastram): ROM and work RAM (excluding the idle loop) */
	sh3_set_fastram(0x00000000, 0x003fffff, 1, (UINT32 *)memory_region(REGION_CPU1));
	if (offset)
		sh3_set_fastram(0x0c000000, 0x0c000000 + offset - 1, 0, cv1k_ram);
	sh3_set_fastram(0x0c000000 + offset + 8, 0x0c000000 + cv1k_ram_size - 1, 0, cv1k_ram + (offset + 8) / 4);
}
static DRIVER_INIT(mushisam)
{
	cv1k_install_speedup(0x024d8, 0xc04a2aa);
}
static DRIVER_INIT(ibara)
{
	cv1k_install_speedup(0x022f0, 0xc04a0aa);
}
static DRIVER_INIT(espgal2)
{
	cv1k_install_speedup(0x02310, 0xc05177a);
}
static DRIVER_INIT(mushitam)
{
	cv1k_install_speedup(0x0022f0, 0xc04a0da);
}
static DRIVER_INIT(pinkswts)
{
	cv1k_install_speedup(0x02310, 0xc05176a);
}
static DRIVER_INIT(deathsml)
{
	cv1k_install_speedup(0x02310, 0xc0519a2);
}
static DRIVER_INIT(ddpdfk)
{
	cv1k_install_speedup(0x02310, 0xc1d1346);
}
ROM_START( mushisam )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("mushisam_u4", 0x000000, 0x200000, CRC(15321b30) SHA1(d2cd714ff2299eeab6f9a7c219dfb559c8f98b45) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD("mushisam_u2", 0x000000, 0x8400000, CRC(4f0a842a) SHA1(33f3550ec676a7088b6348cd72c16cc6594afb84) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u23", 0x000000, 0x400000, CRC(138e2050) SHA1(9e86489a4e65af5efb5495adf6d4b3e01d5b2816) )
	ROM_LOAD16_WORD_SWAP("u24", 0x400000, 0x400000, CRC(e3d05c9f) SHA1(130c3d62317da1729c85bd178bd51500edd73ada) )
ROM_END

ROM_START( mushisama )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("mushisama_u4", 0x000000, 0x200000, CRC(0b5b30b2) SHA1(35fd1bb1561c30b311b4325bc8f4628f2fccd20b) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD("mushisam_u2", 0x000000, 0x8400000, CRC(4f0a842a) SHA1(33f3550ec676a7088b6348cd72c16cc6594afb84) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u23", 0x000000, 0x400000, CRC(138e2050) SHA1(9e86489a4e65af5efb5495adf6d4b3e01d5b2816) )
	ROM_LOAD16_WORD_SWAP("u24", 0x400000, 0x400000, CRC(e3d05c9f) SHA1(130c3d62317da1729c85bd178bd51500edd73ada) )
ROM_END

ROM_START( mushisamb )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("mushisamb_u4", 0x000000, 0x200000, CRC(9f1c7f51) SHA1(f82ae72ec03687904ca7516887080be92365a5f3) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD("mushisam_u2", 0x000000, 0x8400000, CRC(4f0a842a) SHA1(33f3550ec676a7088b6348cd72c16cc6594afb84) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u23", 0x000000, 0x400000, CRC(138e2050) SHA1(9e86489a4e65af5efb5495adf6d4b3e01d5b2816) )
	ROM_LOAD16_WORD_SWAP("u24", 0x400000, 0x400000, CRC(e3d05c9f) SHA1(130c3d62317da1729c85bd178bd51500edd73ada) )
ROM_END

ROM_START( espgal2 )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "espgal2_u4", 0x000000, 0x200000, CRC(2cb37c03) SHA1(da438efc497f72aa345d2cdb0143d269e51576d3) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "u2", 0x000000, 0x8400000, CRC(222f58c7) SHA1(d47a5085a1debd9cb8c61d88cd39e4f5036d1797) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(b9a10c22) SHA1(4561f95c6018c9716077224bfe9660e61fb84681) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(c76b1ec4) SHA1(b98a53d41a995d968e0432ed824b0b06d93dcea8) )
ROM_END

ROM_START( espgal2a )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "espgal2a_u4", 0x000000, 0x200000, CRC(843608b8) SHA1(2f5fcd38e76df531a923cd9956104cef5185aaa9) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "u2", 0x000000, 0x8400000, CRC(222f58c7) SHA1(d47a5085a1debd9cb8c61d88cd39e4f5036d1797) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(b9a10c22) SHA1(4561f95c6018c9716077224bfe9660e61fb84681) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(c76b1ec4) SHA1(b98a53d41a995d968e0432ed824b0b06d93dcea8) )
ROM_END

ROM_START( espgal2b )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "espgal2b_u4", 0x000000, 0x200000, CRC(09c908bb) SHA1(7d6031fd3542b3e1d296ff218feb40502fd78694) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "u2", 0x000000, 0x8400000, CRC(222f58c7) SHA1(d47a5085a1debd9cb8c61d88cd39e4f5036d1797) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(b9a10c22) SHA1(4561f95c6018c9716077224bfe9660e61fb84681) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(c76b1ec4) SHA1(b98a53d41a995d968e0432ed824b0b06d93dcea8) )
ROM_END

ROM_START( mushitam )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("mushitam_u4", 0x000000, 0x200000, CRC(c49eb6b1) SHA1(c40ee5de89e3f1cb49ac19687657dd2b42a88d81) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD("mushitam_u2", 0x000000, 0x8400000, CRC(8ba498ab) SHA1(459c0b4ab831bbe019bdd5b0ac56955948b9e3a6) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u23", 0x000000, 0x400000, CRC(701a912a) SHA1(85c198946fb693d99928ea2595c84ba4d9dc8157) )
	ROM_LOAD16_WORD_SWAP("u24", 0x400000, 0x400000, CRC(6feeb9a1) SHA1(992711c80e660c32f97b343c2ce8184fddd7364e) )
ROM_END

ROM_START( mushitama )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("mushitama_u4", 0x000000, 0x200000, CRC(4a23e6c8) SHA1(d44c287bb88e6d413a8d35d75bc1b4928ad52cdf) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)

	ROM_LOAD("mushitam_u2", 0x000000, 0x8400000, CRC(8ba498ab) SHA1(459c0b4ab831bbe019bdd5b0ac56955948b9e3a6) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u23", 0x000000, 0x400000, CRC(701a912a) SHA1(85c198946fb693d99928ea2595c84ba4d9dc8157) )
	ROM_LOAD16_WORD_SWAP("u24", 0x400000, 0x400000, CRC(6feeb9a1) SHA1(992711c80e660c32f97b343c2ce8184fddd7364e) )
ROM_END

ROM_START( futari15 )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("futari15_u4", 0x000000, 0x200000, CRC(e8c5f128) SHA1(45fb8066fdbecb83fdc2e14555c460d0c652cd5f) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD("futari15_u2", 0x000000, 0x8400000, CRC(b9eae1fc) SHA1(410f8e7cfcbfd271b41fb4f8d049a13a3191a1f9) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u23", 0x000000, 0x400000, CRC(39f1e1f4) SHA1(53d12f59a56df35c705408c76e6e02118da656f1) )
	ROM_LOAD16_WORD_SWAP("u24", 0x400000, 0x400000, CRC(c631a766) SHA1(8bb6934a2f5b8a9841c3dcf85192b1743773dd8b) )
ROM_END

ROM_START( futari15a )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("futari15a_u4", 0x000000, 0x200000, CRC(a609cf89) SHA1(56752fae9f42fa852af8ee2eae79e25ec7f17953) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)

	ROM_LOAD("futari15_u2", 0x000000, 0x8400000, CRC(b9eae1fc) SHA1(410f8e7cfcbfd271b41fb4f8d049a13a3191a1f9) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u23", 0x000000, 0x400000, CRC(39f1e1f4) SHA1(53d12f59a56df35c705408c76e6e02118da656f1) )
	ROM_LOAD16_WORD_SWAP("u24", 0x400000, 0x400000, CRC(c631a766) SHA1(8bb6934a2f5b8a9841c3dcf85192b1743773dd8b) )
ROM_END

ROM_START( futari10 )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "futari10_u4", 0x000000, 0x200000, CRC(b127dca7) SHA1(e1f518bc72fc1cdf69aefa89eafa4edaf4e84778) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "futari10_u2", 0x000000, 0x8400000, CRC(78ffcd0c) SHA1(0e2937edec15ce3f5741b72ebd3bbaaefffb556e) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(39f1e1f4) SHA1(53d12f59a56df35c705408c76e6e02118da656f1) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(c631a766) SHA1(8bb6934a2f5b8a9841c3dcf85192b1743773dd8b) )
ROM_END

ROM_START( futaribl )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "futaribli_u4", 0x000000, 0x200000, CRC(1971dd16) SHA1(e75993f2978cbaaf925b4b8bb33d094a5a7cebf0) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "futariblk_u2", 0x000000, 0x8400000, CRC(08c6fd62) SHA1(e1fc386b2b0e41906c724287cbf82304297e0150) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(39f1e1f4) SHA1(53d12f59a56df35c705408c76e6e02118da656f1) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(c631a766) SHA1(8bb6934a2f5b8a9841c3dcf85192b1743773dd8b) )
ROM_END

ROM_START( futariblj )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "futariblk_u4", 0x000000, 0x200000, CRC(b9467b6d) SHA1(64782807fe69acb4ae028e36c5c689d8bd9b7857) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "futariblk_u2", 0x000000, 0x8400000, CRC(08c6fd62) SHA1(e1fc386b2b0e41906c724287cbf82304297e0150) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(39f1e1f4) SHA1(53d12f59a56df35c705408c76e6e02118da656f1) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(c631a766) SHA1(8bb6934a2f5b8a9841c3dcf85192b1743773dd8b) )
ROM_END

ROM_START( ibara )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "ibara_u4", 0x000000, 0x200000, CRC(d5fb6657) SHA1(4b2c9057cc765054232f520ee1c93495bc6b301e) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "u2", 0x000000, 0x8400000, CRC(55840976) SHA1(4982bdce84f9603adfed7a618f18bc80359ab81e) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(ee5e585d) SHA1(7eeba4ee693060e927f8c46b16e39227c6a62392) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(f0aa3cb6) SHA1(f9d137cd879e718811b2d21a0af2a9c6b7dca2f9) )
ROM_END

ROM_START( ibarao )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "ibarao_u4", 0x000000, 0x200000, CRC(8e6c155d) SHA1(38ac2107dc7824836e2b4e04c7180d5ae43c9b79) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "u2", 0x000000, 0x8400000, CRC(55840976) SHA1(4982bdce84f9603adfed7a618f18bc80359ab81e) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(ee5e585d) SHA1(7eeba4ee693060e927f8c46b16e39227c6a62392) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(f0aa3cb6) SHA1(f9d137cd879e718811b2d21a0af2a9c6b7dca2f9) )
ROM_END

ROM_START( ibarablk )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "ibarablk_u4", 0x000000, 0x200000, CRC(ee1f1f77) SHA1(ac276f3955aa4dde2544af4912819a7ae6bcf8dd) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "ibarablk_u2", 0x000000, 0x8400000, CRC(5e46be44) SHA1(bed5f1bf452f2cac58747ecabec3c4392566a3a7) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(a436bb22) SHA1(0556e771cc02638bf8814315ba671c2d442594f1) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(d11ab6b6) SHA1(2132191cbe847e2560423e4545c969f21f8ff825) )
ROM_END

ROM_START( ibarablka )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "ibarablka_u4", 0x000000, 0x200000, CRC(a9d43839) SHA1(507696e616608c05893c7ac2814b3365e9cb0720) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "ibarablka_u2", 0x000000, 0x8400000, CRC(33400d96) SHA1(09c22b5431ac3726bf88c56efd970f56793f825a) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(a436bb22) SHA1(0556e771cc02638bf8814315ba671c2d442594f1) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(d11ab6b6) SHA1(2132191cbe847e2560423e4545c969f21f8ff825) )
ROM_END

ROM_START( deathsml )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u4", 0x000000, 0x200000, CRC(1a7b98bf) SHA1(07798a4a846e5802756396b34df47d106895c1f1) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "u2", 0x000000, 0x8400000, CRC(59ef5d78) SHA1(426e506b6d88948aa55aec71c0db6e91da3d490d) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(aab718c8) SHA1(0e636c46d06151abd6f73232bc479dafcafe5327) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(83881d84) SHA1(6e2294b247dfcbf0ced155dc45c706f29052775d) )
ROM_END

ROM_START( mmpork )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u4", 0x000000, 0x200000, CRC(d06cfa42) SHA1(5707feb4b3e5265daf5926f38c38612b24106f1f) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "u2", 0x000000, 0x8400000, CRC(1ee961b8) SHA1(81a2eba704ac1cf7fc44fa7c6a3f50e3570c104f) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(4a4b36df) SHA1(5db5ce6fa47e5ca3263d4bd19315890c6d29df66) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(ce83d07b) SHA1(a5947467c8f5b7c4b0ad8e32df2ee29b787e355f) )
ROM_END

ROM_START( mmmbanc )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u4", 0x0000, 0x200000, CRC(5589d8c6) SHA1(43fbdb0effe2bc0e7135698757b6ee50200aecde) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "u2", 0x000000, 0x8400000, CRC(2e38965a) SHA1(2b58d1cd1a3dbc261d4a46805d2ea015fe22c444) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(4caaa1bf) SHA1(9b92c13eac05601da4d9bb3eb727c156974e9f0c) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(8e3a51ba) SHA1(e34cf9acb13c3d8ca6cd1306b060b1d429872abd) )
ROM_END

ROM_START( pinkswts )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "pinkswts_u4", 0x0000, 0x200000, CRC(5d812c9e) SHA1(db821ec3892fd150513749d64a8b60bf147f3275) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "pinkswts_u2", 0x000000, 0x8400000, CRC(a2fa5363) SHA1(5be327534840871592df523ac82ee1927bd79d67) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(4b82d250) SHA1(ee98dbc3f791efb6d58f3945bcb2044667ae7978) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(e93f0627) SHA1(6f5ec0ade87f7fc42a58a8f125557a4d1f3f187d) )
ROM_END

ROM_START( pinkswtsa )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "pnkswtsa_u4", 0x0000, 0x200000, CRC(ee3339b2) SHA1(995988d370731a7074b49ce8752525dadf06a954) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "pnkswtsa_u2", 0x000000, 0x8400000, CRC(829a862e) SHA1(8c0ee2a0eb33b68869252fd68aed74820a904287) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(4b82d250) SHA1(ee98dbc3f791efb6d58f3945bcb2044667ae7978) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(e93f0627) SHA1(6f5ec0ade87f7fc42a58a8f125557a4d1f3f187d) )
ROM_END

ROM_START( pinkswtsb )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "pnkswtsb_u4", 0x0000, 0x200000, CRC(68bcc009) SHA1(2fef544b93c61161a37365f868b431d8262e4b21) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)

	ROM_LOAD( "pnkswtsx_u2", 0x000000, 0x8400000, CRC(91e4deb2) SHA1(893cb10d6f805df7cb4a1bb709a3ea6de147b7e9) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(4b82d250) SHA1(ee98dbc3f791efb6d58f3945bcb2044667ae7978) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(e93f0627) SHA1(6f5ec0ade87f7fc42a58a8f125557a4d1f3f187d) )
ROM_END

ROM_START( pinkswtsx )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "pnkswtsx_u4", 0x0000, 0x200000, CRC(8fe05bf0) SHA1(5cd45ac0e74690787c83d3fb383a65ed7cd47104) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "pnkswtsx_u2", 0x000000, 0x8400000, CRC(91e4deb2) SHA1(893cb10d6f805df7cb4a1bb709a3ea6de147b7e9) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(4b82d250) SHA1(ee98dbc3f791efb6d58f3945bcb2044667ae7978) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(e93f0627) SHA1(6f5ec0ade87f7fc42a58a8f125557a4d1f3f187d) )
ROM_END

ROM_START( pinkswtssc )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "suicideclub.u4", 0x0000, 0x200000, CRC(5e03662f) SHA1(b974204b8dcd55fc1b7775f7c1806150919caff3) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "suicideclub.u2", 0x000000, 0x8400000, CRC(32324608) SHA1(cec1416c943520cb3f91eb295e2ba864a0db7d45) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(4b82d250) SHA1(ee98dbc3f791efb6d58f3945bcb2044667ae7978) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(e93f0627) SHA1(6f5ec0ade87f7fc42a58a8f125557a4d1f3f187d) )
ROM_END

ROM_START( ddpdfk )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "ddpdfk_u4", 0x0000, 0x400000, CRC(9976d699) SHA1(9dfe9d1daf6f638cafce8cdc5230209e2bcb7522) )

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "ddpdfk_u2", 0x000000, 0x8400000, CRC(84a51a4f) SHA1(291a6279c0746d2eb8630e7d6d886043f0cfdd94) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(27032cde) SHA1(5b58d0140d72b91db4e763ca4af93060d36ac74d) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(a6178c2c) SHA1(7742ad1de2e4b0d0206ad11d467ea35da36352df) )
ROM_END

ROM_START( ddpdfk10 )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "ddpdfk10_u4", 0x0000, 0x400000, CRC(a3d650b2) SHA1(46a7551760e18c2cecd372c3f4be16f6600efc2c) )

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "ddpdfk10_u2", 0x000000, 0x8400000, CRC(d349cb2a) SHA1(c364c36b69b93f8f62390f185d044f51056669ff) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(27032cde) SHA1(5b58d0140d72b91db4e763ca4af93060d36ac74d) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(a6178c2c) SHA1(7742ad1de2e4b0d0206ad11d467ea35da36352df) )
ROM_END

ROM_START( dsmbl )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u4", 0x000000, 0x400000, CRC(77fc5ad1) SHA1(afe044fc16e9494143c876879b033caccd08cf22) )

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "u2", 0x000000, 0x8400000, CRC(d6b85b7a) SHA1(4674f6ad07f6a03904ca3d05060816b8fe061add) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(a9536a6a) SHA1(4b9dcaf6803b1fcfdf73ae9daabc4508fec71631) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(3b673326) SHA1(1ae847eb4e752fef1d72081d82344f0ad0537c31) )
ROM_END

ROM_START( dfkbl )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u4", 0x000000, 0x400000, CRC(8092ca9d) SHA1(75e16cd7c8d0f9c715115ce12da5c245fbcd2416) )

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "u2", 0x000000, 0x8400000, CRC(29f9d73a) SHA1(ed978ab5e3ad8c05e7778a91bfb5aaa17b0f72d9) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(36d4093b) SHA1(4aed7e2f7c0d2c9bceeb110a9907d8d99d55f4c3) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(31f9eb0a) SHA1(322158779e969bb321241065dd49c1167b91ff6c) )
ROM_END

ROM_START( akatana )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u4", 0x000000, 0x400000, CRC(613fd380) SHA1(6e28480eef3b483d00b42d811a9d2c7fa1097924) ) // (2010/ 8/13 MASTER VER.)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "u2", 0x000000, 0x8400000, CRC(89a2e1a5) SHA1(e6f4ec974406283665697fdd52bd606d0337dd11) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(34a67e24) SHA1(78a7e82123b86311f1116a80c39f147b8b695549) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(10760fed) SHA1(b70f4506c00f3901ff38f5efd4b897af1afc7a0c) )
ROM_END

ROM_START( akatanaultd )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u4", 0x000000, 0x400000, CRC(c229869a) SHA1(6e42074746d409ca8edcdc7b4ef3de3741a6594c) )

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "u2", 0x000000, 0x8400000, CRC(89a2e1a5) SHA1(e6f4ec974406283665697fdd52bd606d0337dd11) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(34a67e24) SHA1(78a7e82123b86311f1116a80c39f147b8b695549) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(10760fed) SHA1(b70f4506c00f3901ff38f5efd4b897af1afc7a0c) )
ROM_END

ROM_START( ddpsdoj )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u4", 0x000000, 0x400000, CRC(e2a4411c) SHA1(f8b2b6326dd8eeae99b8b1ab2bd5a3f0b9c7f027) )
	ROM_IGNORE( 0x100 )

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD("u2", 0x000000, 0x8400000, CRC(668e4cd6) SHA1(da0b10865df3a3e46cf8a109ca88a551faba4483) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u23", 0x000000, 0x400000, CRC(ac94801c) SHA1(cbcc6d5d89860bc961967e1d3b7c329adaf200c5) )
	ROM_IGNORE( 0x100 )
	ROM_LOAD16_WORD_SWAP("u24", 0x400000, 0x400000, CRC(f593045b) SHA1(91b92870d0dd2a7817cb0059cc750e2393686f4c) )
	ROM_IGNORE( 0x100 )
ROM_END

ROM_START( deathsml01 )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u4", 0x000000, 0x200000, CRC(1a7b98bf) SHA1(07798a4a846e5802756396b34df47d106895c1f1) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "s01.u2", 0x000000, 0x8400000, CRC(d7dd7bad) SHA1(579f14a7f3ec399f6694b20eaae2990fa699aa72) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(aab718c8) SHA1(0e636c46d06151abd6f73232bc479dafcafe5327) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(83881d84) SHA1(6e2294b247dfcbf0ced155dc45c706f29052775d) )
ROM_END

ROM_START( ddpsdojo )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u4", 0x000000, 0x400000, CRC(e2a4411c) SHA1(f8b2b6326dd8eeae99b8b1ab2bd5a3f0b9c7f027) )
	ROM_IGNORE( 0x000100 )

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD("old.u2", 0x000000, 0x8400000, CRC(7420302f) SHA1(7db4bd7a72f84fbd6e9c7966769d7b6fccc17cea) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u23", 0x000000, 0x400000, CRC(ac94801c) SHA1(cbcc6d5d89860bc961967e1d3b7c329adaf200c5) )
	ROM_IGNORE( 0x000100 )
	ROM_LOAD16_WORD_SWAP("u24", 0x400000, 0x400000, CRC(f593045b) SHA1(91b92870d0dd2a7817cb0059cc750e2393686f4c) )
	ROM_IGNORE( 0x000100 )
ROM_END

ROM_START( sdojak )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "sdojak.u4", 0x000000, 0x400000, CRC(a878ff4c) SHA1(cf5f2f987a775099992009c376466c44170e0e3f) )

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "sdojak.u2", 0x000000, 0x8400000, CRC(54353425) SHA1(5c749cd2e93a1b51bfdf1a0d4fd436b4d17acf9f) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "sdojak.u23", 0x000000, 0x400000, CRC(32b91544) SHA1(49e914bfdf33a328fbfa7efb42c5b89915e37643) )
	ROM_LOAD16_WORD_SWAP( "sdojak.u24", 0x400000, 0x400000, CRC(7b9e749f) SHA1(374085a60098cc62501abae4d148bfd88c88149f) )
ROM_END

ROM_START( sdoja )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "sdoja.u4", 0x000000, 0x400000, CRC(5a3b94e8) SHA1(a546f2baa366ca02dbd462b8ee7808c2643927cb) )

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "sdoja.u2", 0x000000, 0x8400000, CRC(7e19adc7) SHA1(6a6e2220096c80471d391d93979e0c883f733c1b) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "sdojak.u23", 0x000000, 0x400000, CRC(32b91544) SHA1(49e914bfdf33a328fbfa7efb42c5b89915e37643) )
	ROM_LOAD16_WORD_SWAP( "sdojak.u24", 0x400000, 0x400000, CRC(7b9e749f) SHA1(374085a60098cc62501abae4d148bfd88c88149f) )
ROM_END

ROM_START( sdojb )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "sdojb.u4", 0x000000, 0x400000, CRC(be54ca1a) SHA1(d7ad13cc6676ab3e938c65b7e6f5c870033f42f1) )

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "sdojb.u2", 0x000000, 0x8400000, CRC(2a6353a6) SHA1(29b180263d5f08bce7527cf1d4cae33a98d93149) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u23", 0x000000, 0x400000, CRC(ac94801c) SHA1(cbcc6d5d89860bc961967e1d3b7c329adaf200c5) )
	ROM_IGNORE( 0x000100 )
	ROM_LOAD16_WORD_SWAP("u24", 0x400000, 0x400000, CRC(f593045b) SHA1(91b92870d0dd2a7817cb0059cc750e2393686f4c) )
	ROM_IGNORE( 0x000100 )
ROM_END

ROM_START( ibaracs )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u4", 0x000000, 0x200000, CRC(8e6c155d) SHA1(38ac2107dc7824836e2b4e04c7180d5ae43c9b79) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "ibaracs_u2", 0x000000, 0x8400000, CRC(BD68D0AF) SHA1(8FAF49D9B4B472F711DD04A772B28999EDEB21E3) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(ee5e585d) SHA1(7eeba4ee693060e927f8c46b16e39227c6a62392) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(f0aa3cb6) SHA1(f9d137cd879e718811b2d21a0af2a9c6b7dca2f9) )
ROM_END

ROM_START( ibarablkcs )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "ibarablk_u4", 0x000000, 0x200000, CRC(ee1f1f77) SHA1(ac276f3955aa4dde2544af4912819a7ae6bcf8dd) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "ibarablkcs_u2", 0x000000, 0x8400000, CRC(60723C96) SHA1(EB06AD85F0EBDAD1F66102772DC9181FBD727950) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(a436bb22) SHA1(0556e771cc02638bf8814315ba671c2d442594f1) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(d11ab6b6) SHA1(2132191cbe847e2560423e4545c969f21f8ff825) )
ROM_END

ROM_START( futari15bc )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("futari15_u4", 0x000000, 0x200000, CRC(e8c5f128) SHA1(45fb8066fdbecb83fdc2e14555c460d0c652cd5f) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD("futari15bc_u2", 0x000000, 0x8400000, CRC(D5459F63) SHA1(4F01E4D71561C83B5F69CCCA4B3F1317F0EE8FF5) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u23", 0x000000, 0x400000, CRC(39f1e1f4) SHA1(53d12f59a56df35c705408c76e6e02118da656f1) )
	ROM_LOAD16_WORD_SWAP("u24", 0x400000, 0x400000, CRC(c631a766) SHA1(8bb6934a2f5b8a9841c3dcf85192b1743773dd8b) )
ROM_END

ROM_START( futariblc )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "futaribli_u4", 0x000000, 0x200000, CRC(1971dd16) SHA1(e75993f2978cbaaf925b4b8bb33d094a5a7cebf0) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "futariblkc_u2", 0x000000, 0x8400000, CRC(299B9BAC) SHA1(7CD160B17ED2931978143341C997BF402DEBC48B) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(39f1e1f4) SHA1(53d12f59a56df35c705408c76e6e02118da656f1) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(c631a766) SHA1(8bb6934a2f5b8a9841c3dcf85192b1743773dd8b) )
ROM_END

ROM_START( futaribljc )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "futariblk_u4", 0x000000, 0x200000, CRC(b9467b6d) SHA1(64782807fe69acb4ae028e36c5c689d8bd9b7857) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD( "futariblkc_u2", 0x000000, 0x8400000, CRC(299B9BAC) SHA1(7CD160B17ED2931978143341C997BF402DEBC48B) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP( "u23", 0x000000, 0x400000, CRC(39f1e1f4) SHA1(53d12f59a56df35c705408c76e6e02118da656f1) )
	ROM_LOAD16_WORD_SWAP( "u24", 0x400000, 0x400000, CRC(c631a766) SHA1(8bb6934a2f5b8a9841c3dcf85192b1743773dd8b) )
ROM_END

ROM_START( msmatsuri )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u4", 0x000000, 0x200000, CRC(6af99f0e) SHA1(e91b3a7c329a9ce66687b48a9f2e1a8ea7282cd5) )
	ROM_RELOAD(0x200000,0x200000)

	ROM_REGION( 0x8400000, REGION_USER1, ROMREGION_ERASEFF)
	ROM_LOAD("u2", 0x000000, 0x8400000, CRC(fbc6f3c1) SHA1(9967c56121a2886261026665d9f64978f2cb4dda) )

	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("u23", 0x000000, 0x400000, CRC(6dff634f) SHA1(781042316fae904657326189b3b46e49a46ba675) )
	ROM_LOAD16_WORD_SWAP("u24", 0x400000, 0x400000, CRC(f228b801) SHA1(b9dd2a7462ccad4b20b8cd7eaefcdfd881dd413f) )
ROM_END

GAME( 2004, mushisam,   0,        cv1k,   cv1k, mushisam, ROT270, "Cave (AMI license)",   "Mushihime-Sama (2004/10/12.MASTER VER.)",                                       0 )
GAME( 2004, mushisama,  mushisam, cv1k,   cv1k, ibara,    ROT270, "Cave (AMI license)",   "Mushihime-Sama (2004/10/12 MASTER VER.)",                                       0 )
GAME( 2004, mushisamb,  mushisam, cv1k,   cv1k, mushisam, ROT270, "Cave (AMI license)",   "Mushihime-Sama (2004/10/12 MASTER VER)",                                        0 )
GAME( 2005, ibara,      0,        cv1k,   cv1ks,ibara,    ROT270, "Cave (AMI license)",   "Ibara (2005/03/22 MASTER VER.., '06. 3. 7 ver.)",                               0 )
GAME( 2005, ibarao,     ibara,    cv1k,   cv1ks,ibara,    ROT270, "Cave (AMI license)",   "Ibara (2005/03/22 MASTER VER..)",                                               0 )
GAME( 2006, ibarablk,   0,        cv1k,   cv1ks,pinkswts, ROT270, "Cave (AMI license)",   "Ibara Kuro Black Label (2006/02/06. MASTER VER.)",                              0 )
GAME( 2006, ibarablka,  ibarablk, cv1k,   cv1ks,pinkswts, ROT270, "Cave (AMI license)",   "Ibara Kuro Black Label (2006/02/06 MASTER VER.)",                               0 )
GAME( 2005, espgal2,    0,        cv1k,   cv1k, espgal2,  ROT270, "Cave (AMI license)",   "Espgaluda II (2005/11/14.MASTER VER.)",                                         0 )
GAME( 2005, espgal2a,   espgal2,  cv1k,   cv1k, espgal2,  ROT270, "Cave (AMI license)",   "Espgaluda II (2005/11/14 MASTER VER, newer CV1000-B PCB)",                      0 )
GAME( 2005, espgal2b,   espgal2,  cv1k,   cv1k, espgal2,  ROT270, "Cave (AMI license)",   "Espgaluda II (2005/11/14 MASTER VER, original CV1000-B PCB)",                   0 )
GAME( 2005, mushitam,   0,        cv1k,   cv1k, mushitam, ROT0,   "Cave (AMI license)",   "Puzzle! Mushihime-Tama (2005/09/09.MASTER VER)",                                0 )
GAME( 2005, mushitama,  mushitam, cv1k,   cv1k, mushitam, ROT0,   "Cave (AMI license)",   "Puzzle! Mushihime-Tama (2005/09/09 MASTER VER)",                                0 )
GAME( 2006, pinkswts,   0,        cv1k,   cv1ks,pinkswts, ROT270, "Cave (AMI license)",   "Pink Sweets: Ibara Sorekara (2006/04/06 MASTER VER....)",                       0 )
GAME( 2006, pinkswtsa,  pinkswts, cv1k,   cv1ks,pinkswts, ROT270, "Cave (AMI license)",   "Pink Sweets: Ibara Sorekara (2006/04/06 MASTER VER...)",                        0 )
GAME( 2006, pinkswtsb,  pinkswts, cv1k,   cv1ks,pinkswts, ROT270, "Cave (AMI license)",   "Pink Sweets: Ibara Sorekara (2006/04/06 MASTER VER.)",                          0 )
GAME( 2006, pinkswtsx,  pinkswts, cv1k,   cv1ks,pinkswts, ROT270, "Cave (AMI license)",   "Pink Sweets: Ibara Sorekara (2006/xx/xx MASTER VER.)",                          0 )
GAME( 2017, pinkswtssc, pinkswts, cv1k,   cv1ks,pinkswts, ROT270, "bootleg (Four Horsemen)", "Pink Sweets: Suicide Club (2017/10/31 SUICIDECLUB VER., bootleg)",           0 )
GAME( 2006, futari15,   0,        cv1k,   cv1k, pinkswts, ROT270, "Cave (AMI license)",   "Mushihime-Sama Futari Ver 1.5 (2006/12/8.MASTER VER. 1.54.)",                   0 )
GAME( 2006, futari15a,  futari15, cv1k,   cv1k, pinkswts, ROT270, "Cave (AMI license)",   "Mushihime-Sama Futari Ver 1.5 (2006/12/8 MASTER VER 1.54)",                     0 )
GAME( 2006, futari10,   futari15, cv1k,   cv1k, pinkswts, ROT270, "Cave (AMI license)",   "Mushihime-Sama Futari Ver 1.0 (2006/10/23 MASTER VER.)",                        0 )
GAME( 2007, futaribl,   0,        cv1k,   cv1k, pinkswts, ROT270, "Cave (AMI license)",   "Mushihime-Sama Futari Black Label - Another Ver (2009/11/27 INTERNATIONAL BL)", 0 )
GAME( 2007, futariblj,  futaribl, cv1k,   cv1k, pinkswts, ROT270, "Cave (AMI license)",   "Mushihime-Sama Futari Black Label (2007/12/11 BLACK LABEL VER)",                0 )
GAME( 2007, mmpork,     0,        cv1k,   cv1ks,pinkswts, ROT270, "Cave (AMI license)",   "Muchi Muchi Pork! (2007/ 4/17 MASTER VER.)",                                    0 )
GAME( 2007, deathsml,   0,        cv1k,   cv1k, deathsml, ROT0,   "Cave (AMI license)",   "Deathsmiles (2007/10/09 MASTER VER)",                                           0 )
GAME( 2008, dsmbl,      0,        cv1k_d, cv1k, ddpdfk,   ROT0,   "Cave (AMI license)",   "Deathsmiles MegaBlack Label (2008/10/06 MEGABLACK LABEL VER)",                  0 )
GAME( 2008, ddpdfk,     0,        cv1k_d, cv1k, ddpdfk,   ROT270, "Cave (AMI license)",   "DoDonPachi Dai-Fukkatsu Ver 1.5 (2008/06/23 MASTER VER 1.5)",                   0 )
GAME( 2008, ddpdfk10,   ddpdfk,   cv1k_d, cv1k, ddpdfk,   ROT270, "Cave (AMI license)",   "DoDonPachi Dai-Fukkatsu Ver 1.0 (2008/05/16 MASTER VER)",                       0 )
GAME( 2010, dfkbl,      0,        cv1k_d, cv1k, ddpdfk,   ROT270, "Cave",                 "DoDonPachi Dai-Fukkatsu Black Label (2010/1/18 BLACK LABEL)",                   0 )
GAME( 2007, mmmbanc,    0,        cv1k,   cv1k, pinkswts, ROT0,   "Cave (AMI license)",   "Medal Mahjong Moukari Bancho (2007/06/05 MASTER VER.)",                         GAME_NOT_WORKING )
GAME( 2010, akatana,    0,        cv1k_d, cv1k, ddpdfk,   ROT0,   "Cave",                 "Akai Katana (2010/ 8/13 MASTER VER.)",                                          0 )
GAME( 2010, akatanaultd, akatana, cv1k_d, cv1k, ddpdfk,   ROT0,   "Cave",                 "Akai Katana Unlimited",                                                         0 ) /* https://github.com/IAmTheChuckster/Akai_Katana_Unlimited */
GAME( 2007, deathsml01,  deathsml, cv1k,   cv1k, deathsml, ROT0, "Cave (AMI license)", "Death Smiles (Hidden elements full open + character enhancement)", 0 )
GAME( 2012, ddpsdoj,    0,        cv1k_d, cv1k, ddpdfk,   ROT270, "Cave",                 "DoDonPachi SaiDaiOuJou (2012/ 4/20)",                                           0 )
GAME( 2012, ddpsdojo,    ddpsdoj,  cv1k_d, cv1k, ddpdfk,   ROT270, "Cave", "DoDonPachi SaiDaiOuJou (dirty dump)", 0 )
GAME( 2021, sdojak,      0,        cv1k_d, cv1k, ddpdfk,   ROT270, "CaveDwellers", "DoDonPachi SaiDaiOuJou & Knuckles (2021-12-01)", 0 )
GAME( 2021, sdoja,       sdojak,   cv1k_d, cv1k, ddpdfk,   ROT270, "hack", "DoDonPachi SaiDaiOuJou hack", 0 )
GAME( 2022, sdojb,       sdojak,   cv1k_d, cv1k, ddpdfk,   ROT270, "hack", "DoDonPachi SaiDaiOuJou Saya Type B (2022-01-06)", 0 )
GAME( 2005, ibaracs,     ibara,    cv1k,   cv1ks, ibara,    ROT270, "ArcCabView", "Ibara (2005-03-22 MASTER VER.. COLOR SAT)", 0 )
GAME( 2006, ibarablkcs,  ibarablk, cv1k,   cv1ks, pinkswts, ROT270, "ArcCabView", "Ibara Kuro Black Label (2006-02-06. MASTER VER. COLOR SAT)", 0 )
GAME( 2022, futari15bc,  futari15, cv1k,   cv1k, pinkswts, ROT270, "ArcCabView", "Mushihime-Sama Futari Ver 1.5 (2006-12-08.MASTER VER. 1.54. BLK COLOR)", 0 )
GAME( 2022, futariblc,   futaribl, cv1k,   cv1k, pinkswts, ROT270, "ArcCabView", "Mushihime-Sama Futari Black Label - Another Ver (2009-11-27 INTERNATIONAL BL - ORIG COLOR)", 0 )
GAME( 2022, futaribljc,  futaribl, cv1k,   cv1k, pinkswts, ROT270, "ArcCabView", "Mushihime-Sama Futari Black Label (2007-12-11 BLACK LABEL VER - ORIG COLOR)", 0 )
GAME( 2011, msmatsuri,   0,        cv1k,   cv1k, mushisam, ROT270, "Cave (AMI license)",   "Mushihime-Sama Matsuri (Japan, 2011-05-23 CAVEMATSURI VER1.5)", 0 )
