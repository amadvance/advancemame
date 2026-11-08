// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
/*
	Nintendo 64
	
	Driver by Ville Linde
	Reformatted to share hardware by R. Belmont

	Note (RB): Some games boot and do useful things with the MIPS3_DRC enabled 
	that don't otherwise.  Example: the entire "Madden" series starting with 
	"Madden Football 64".
*/

#include "driver.h"
#include "cpu/mips/mips3.h"
#include "sound/dmadac.h"
#include "devices/cartslot.h"
#include "streams.h"
#include "includes/n64.h"

static ADDRESS_MAP_START( n64_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM	AM_BASE(&rdram)				// RDRAM
	AM_RANGE(0x04000000, 0x04000fff) AM_RAM AM_SHARE(1)				// RSP DMEM
	AM_RANGE(0x04001000, 0x04001fff) AM_RAM AM_SHARE(2)				// RSP IMEM
	AM_RANGE(0x04040000, 0x040fffff) AM_READWRITE(n64_sp_reg_r, n64_sp_reg_w)	// RSP
	AM_RANGE(0x04100000, 0x041fffff) AM_READWRITE(n64_dp_reg_r, n64_dp_reg_w)	// RDP
	AM_RANGE(0x04300000, 0x043fffff) AM_READWRITE(n64_mi_reg_r, n64_mi_reg_w)	// MIPS Interface
	AM_RANGE(0x04400000, 0x044fffff) AM_READWRITE(n64_vi_reg_r, n64_vi_reg_w)	// Video Interface
	AM_RANGE(0x04500000, 0x045fffff) AM_READWRITE(n64_ai_reg_r, n64_ai_reg_w)	// Audio Interface
	AM_RANGE(0x04600000, 0x046fffff) AM_READWRITE(n64_pi_reg_r, n64_pi_reg_w)	// Peripheral Interface
	AM_RANGE(0x04700000, 0x047fffff) AM_READWRITE(n64_ri_reg_r, n64_ri_reg_w)	// RDRAM Interface
	AM_RANGE(0x04800000, 0x048fffff) AM_READWRITE(n64_si_reg_r, n64_si_reg_w)	// Serial Interface
	AM_RANGE(0x10000000, 0x13ffffff) AM_ROM AM_REGION(REGION_USER2, 0)	// Cartridge
	AM_RANGE(0x1fc00000, 0x1fc007bf) AM_ROM AM_REGION(REGION_USER1, 0)	// PIF ROM
	AM_RANGE(0x1fc007c0, 0x1fc007ff) AM_READWRITE(n64_pif_ram_r, n64_pif_ram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( rsp_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x04000000, 0x04000fff) AM_RAM AM_BASE(&rsp_dmem) AM_SHARE(1)
	AM_RANGE(0x04001000, 0x04001fff) AM_RAM AM_BASE(&rsp_imem) AM_SHARE(2)
ADDRESS_MAP_END

INPUT_PORTS_START( n64 )
	PORT_START_TAG("P1")
		PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)			// Button A
		PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)			// Button B
		PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)			// Button Z
		PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON4 )							// Start
		PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)		// Joypad Up
		PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)	// Joypad Down
		PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)	// Joypad Left
		PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)	// Joypad Right
		PORT_BIT( 0x00c0, IP_ACTIVE_HIGH, IPT_UNUSED )
		PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(1)			// Pan Left
		PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(1)			// Pan Right
		PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(1)			// C Button Up
		PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_PLAYER(1)			// C Button Down
		PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON8 ) PORT_PLAYER(1)			// C Button Left
		PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_PLAYER(1)			// C Button Right
	
	PORT_START_TAG("P1_ANALOG_X")
		PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(1)

	PORT_START_TAG("P1_ANALOG_Y")
		PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0xff,0x00) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(1)	
INPUT_PORTS_END

/* ?? */
static struct mips3_config config =
{
	16384,				/* code cache size */
	8192,				/* data cache size */
	62500000			/* system clock */
};

static INTERRUPT_GEN( n64_vblank )
{
	signal_rcp_interrupt(VI_INTERRUPT);
}

/* Keep the console's generated PIF boot ROM; the arcade driver uses a dumped ROM. */
static void n64_pif_bootstrap(void)
{
	int i;
	UINT16 crc_seed = n64_get_cic_seed();
	UINT32 *pif_rom = (UINT32*)memory_region(REGION_USER1);

	// The PIF Boot ROM is not dumped, the following code simulates it

	// clear all registers
	for (i=1; i < 32; i++)
	{
		*pif_rom++ = 0x00000000 | 0 << 21 | 0 << 16 | i << 11 | 0x20;		// ADD ri, r0, r0
	}

	// R20 <- 0x00000001
	*pif_rom++ = 0x34000000 | 20 << 16 | 0x0001;					// ORI r20, r0, 0x0001

	// R22 <- 0x0000003F
	*pif_rom++ = 0x34000000 | 22 << 16 | crc_seed;					// ORI r22, r0, 0x003f

	// R29 <- 0xA4001FF0
	*pif_rom++ = 0x3c000000 | 29 << 16 | 0xa400;					// LUI r29, 0xa400
	*pif_rom++ = 0x34000000 | 29 << 21 | 29 << 16 | 0x1ff0;			// ORI r29, r29, 0x1ff0

	// clear CP0 registers
	for (i=0; i < 32; i++)
	{
		*pif_rom++ = 0x40000000 | 4 << 21 | 0 << 16 | i << 11;		// MTC2 cp0ri, r0
	}

	// Random <- 0x0000001F
	*pif_rom++ = 0x34000000 | 1 << 16 | 0x001f;
	*pif_rom++ = 0x40000000 | 4 << 21 | 1 << 16 | 1 << 11;			// MTC2 Random, r1

	// Status <- 0x70400004
	*pif_rom++ = 0x3c000000 | 1 << 16 | 0x7040;						// LUI r1, 0x7040
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0x0004;			// ORI r1, r1, 0x0004
	*pif_rom++ = 0x40000000 | 4 << 21 | 1 << 16 | 12 << 11;			// MTC2 Status, r1

	// PRId <- 0x00000B00
	*pif_rom++ = 0x34000000 | 1 << 16 | 0x0b00;						// ORI r1, r0, 0x0b00
	*pif_rom++ = 0x40000000 | 4 << 21 | 1 << 16 | 15 << 11;			// MTC2 PRId, r1

	// Config <- 0x0006E463
	*pif_rom++ = 0x3c000000 | 1 << 16 | 0x0006;						// LUI r1, 0x0006
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0xe463;			// ORI r1, r1, 0xe463
	*pif_rom++ = 0x40000000 | 4 << 21 | 1 << 16 | 16 << 11;			// MTC2 Config, r1

	// (0xa4300004) <- 0x01010101
	*pif_rom++ = 0x3c000000 | 1 << 16 | 0x0101;						// LUI r1, 0x0101
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0x0101;			// ORI r1, r1, 0x0101
	*pif_rom++ = 0x3c000000 | 3 << 16 | 0xa430;						// LUI r3, 0xa430
	*pif_rom++ = 0xac000000 | 3 << 21 | 1 << 16 | 0x0004;			// SW r1, 0x0004(r3)

	// Copy 0xb0000000...1fff -> 0xa4000000...1fff
	*pif_rom++ = 0x34000000 | 3 << 16 | 0x0400;						// ORI r3, r0, 0x0400
	*pif_rom++ = 0x3c000000 | 4 << 16 | 0xb000;						// LUI r4, 0xb000
	*pif_rom++ = 0x3c000000 | 5 << 16 | 0xa400;						// LUI r5, 0xa400
	*pif_rom++ = 0x8c000000 | 4 << 21 | 1 << 16;					// LW r1, 0x0000(r4)
	*pif_rom++ = 0xac000000 | 5 << 21 | 1 << 16;					// SW r1, 0x0000(r5)
	*pif_rom++ = 0x20000000 | 4 << 21 | 4 << 16 | 0x0004;			// ADDI r4, r4, 0x0004
	*pif_rom++ = 0x20000000 | 5 << 21 | 5 << 16 | 0x0004;			// ADDI r5, r5, 0x0004
	*pif_rom++ = 0x20000000 | 3 << 21 | 3 << 16 | 0xffff;			// ADDI r3, r3, -1
	*pif_rom++ = 0x14000000 | 3 << 21 | 0 << 16 | 0xfffa;			// BNE r3, r0, -6
	*pif_rom++ = 0x00000000;

	*pif_rom++ = 0x34000000 | 3 << 16 | 0x0000;						// ORI r3, r0, 0x0000
	*pif_rom++ = 0x34000000 | 4 << 16 | 0x0000;						// ORI r4, r0, 0x0000
	*pif_rom++ = 0x34000000 | 5 << 16 | 0x0000;						// ORI r5, r0, 0x0000

	// Zelda and DK64 need these
	*pif_rom++ = 0x3c000000 | 9 << 16 | 0xa400;
	*pif_rom++ = 0x34000000 | 9 << 21 | 9 << 16 | 0x1ff0;
	*pif_rom++ = 0x3c000000 | 11 << 16 | 0xa400;
	*pif_rom++ = 0x3c000000 | 31 << 16 | 0xffff;
	*pif_rom++ = 0x34000000 | 31 << 21 | 31 << 16 | 0xffff;

	*pif_rom++ = 0x3c000000 | 1 << 16 | 0xa400;						// LUI r1, 0xa400
	*pif_rom++ = 0x34000000 | 1 << 21 | 1 << 16 | 0x0040;			// ORI r1, r1, 0x0040
	*pif_rom++ = 0x00000000 | 1 << 21 | 0x8;						// JR r1
}

static MACHINE_START( n64 )
{
	n64_machine_start();
	return 0;
}

MACHINE_RESET( n64 )
{
	n64_machine_reset();
	n64_pif_bootstrap();
}

MACHINE_DRIVER_START( n64 )
	/* basic machine hardware */
	MDRV_CPU_ADD(R4600BE, 93750000)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(n64_map, 0)
	MDRV_CPU_VBLANK_INT( n64_vblank, 1 )
	
	MDRV_CPU_ADD(RSP, 62500000)
	MDRV_CPU_PROGRAM_MAP(rsp_map, 0)
	
	MDRV_MACHINE_START( n64 )
	MDRV_MACHINE_RESET( n64 )
	MDRV_INTERLEAVE( 10)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(640, 525)
	MDRV_VISIBLE_AREA(0, 639, 0, 479)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(n64)
	MDRV_VIDEO_UPDATE(n64)
	
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.00)
	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.00)
MACHINE_DRIVER_END

DRIVER_INIT( n64 )
{

}

ROM_START( n64)
	ROM_REGION( 0x400000, REGION_CPU1, 0 )		/* dummy region for R4300 */
	ROM_REGION( 0x800, REGION_USER1, 0 )
	ROM_REGION32_BE( 0x4000000, REGION_USER2, 0)
ROM_END

static DEVICE_LOAD(n64_cart)
{
	int i, length;
	UINT8 *cart = memory_region(REGION_USER2);
	
	length = mame_fread(file, cart, 0x4000000);
	
	if (cart[0] == 0x37 && cart[1] == 0x80)
	{
		for (i=0; i < length; i+=4)
		{
			UINT8 b1 = cart[i+0];
			UINT8 b2 = cart[i+1];
			UINT8 b3 = cart[i+2];
			UINT8 b4 = cart[i+3];
			cart[i+0] = b3;
			cart[i+1] = b4;
			cart[i+2] = b1;
			cart[i+3] = b2;
		}
	}
	else
	{
		for (i=0; i < length; i+=4)
		{
			UINT8 b1 = cart[i+0];
			UINT8 b2 = cart[i+1];
			UINT8 b3 = cart[i+2];
			UINT8 b4 = cart[i+3];
			cart[i+0] = b4;
			cart[i+1] = b3;
			cart[i+2] = b2;
			cart[i+3] = b1;
		}		
	}
	
	printf("cart length = %d\n", length);
	return INIT_PASS;
}

static void n64_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_n64_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "v64,z64,rom,n64"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(n64)
	CONFIG_DEVICE(n64_cartslot_getinfo)
SYSTEM_CONFIG_END


CONS(1996, n64, 	0,		0,		n64, 	n64, 	0,	n64,	"Nintendo", "Nintendo 64", 0 )
