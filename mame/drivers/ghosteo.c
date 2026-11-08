// license:BSD-3-Clause
// copyright-holders:Tim Schuerewegen, smf
/* Eolith Ghost hardware: Samsung S3C2410, NAND flash and QS1000 audio. */
#include "driver.h"
#include "cpu/arm920t/arm920t.h"
#include "cpu/i8051/i8051.h"
#include "machine/s3c2410.h"
#include "includes/eolith.h"
#include "sound/custom.h"
static UINT32 *steppingstone, *system_memory;
static int security_count = 0;
static const UINT8 security_data[] = { 0x01, 0xc4, 0xff, 0x22, 0xff, 0xff, 0xff, 0xff };

static UINT32 bballoon_port[20];

enum nand_mode_t
{
	NAND_M_INIT,
	NAND_M_READ,
};

struct nand_t
{
	enum nand_mode_t mode;
	int page_addr;
	int byte_addr;
	int addr_load_ptr;
};

static struct nand_t nand;

static UINT32 s3c2410_gpio_port_r( int device, int port)
{
	UINT32 data = bballoon_port[port];
	switch (port)
	{
		case S3C2410_GPIO_PORT_F :
		{
			data = (data & ~0xFF) | security_data[security_count];
		}
		break;
		case S3C2410_GPIO_PORT_G :
		{
			data = data ^ 0x20;
			bballoon_port[port] = data;
		}
		break;
	}
	return data;
}

static void s3c2410_gpio_port_w( int device, int port, UINT32 data)
{
	UINT32 old_value = bballoon_port[port];
	bballoon_port[port] = data;
	switch (port)
	{
		case S3C2410_GPIO_PORT_F :
		{
			switch (data)
			{
				case 0x04 : security_count = 0; break;
				case 0x44 : security_count = 2; break;
			}
		}
		break;
		case S3C2410_GPIO_PORT_G :
		{

			if (((data & 0x10) != 0) && ((old_value & 0x10) == 0))
			{
				security_count++;
				if (security_count > 7) security_count = 0;
			}
		}
		break;
	}
}

static void s3c2410_nand_command_w(int device, offs_t offset, UINT8 data)
{
	switch (data)
	{
		case 0xFF :
		{
			nand.mode = NAND_M_INIT;
			s3c2410_frnb_w( 1);
		}
		break;
		case 0x00 :
		{
			nand.mode = NAND_M_READ;
			nand.page_addr = 0;
			nand.addr_load_ptr = 0;
		}
		break;
	}
}

static void s3c2410_nand_address_w(int device, offs_t offset, UINT8 data)
{
	switch (nand.mode)
	{
		case NAND_M_INIT :
		{
		}
		break;
		case NAND_M_READ :
		{
			if (nand.addr_load_ptr == 0)
			{
				nand.byte_addr = data;
			}
			else
			{
				nand.page_addr = (nand.page_addr & ~(0xFF << ((nand.addr_load_ptr - 1) * 8))) | (data << ((nand.addr_load_ptr - 1) * 8));
			}
			nand.addr_load_ptr++;
			if ((nand.mode == NAND_M_READ) && (nand.addr_load_ptr == 4))
			{
				s3c2410_frnb_w( 0);
				s3c2410_frnb_w( 1);
			}
		}
		break;
	}
}

static UINT8 s3c2410_nand_data_r(int device, offs_t offset)
{

	UINT8 data = 0;
	switch (nand.mode)
	{
		case NAND_M_INIT :
		{
		}
		break;
		case NAND_M_READ :
		{
			UINT8 *flash = (UINT8 *)memory_region(REGION_USER1);
			if (nand.byte_addr < 0x200)
			{
				data = *(flash + nand.page_addr * 0x200 + nand.byte_addr);
			}
			else
			{
				if ((nand.byte_addr >= 0x200) && (nand.byte_addr < 0x204))
				{
					UINT8 mecc[4] = {0,0,0,0xff};
					s3c2410_nand_calculate_mecc( flash + nand.page_addr * 0x200, 0x200, mecc);
					data = mecc[nand.byte_addr-0x200];
				}
				else
				{
					data = 0xFF;
				}
			}
			nand.byte_addr++;
			if (nand.byte_addr == 0x210)
			{
				nand.byte_addr = 0;
				nand.page_addr++;
				if (nand.page_addr == 0x10000) nand.page_addr = 0;
			}
		}
		break;
	}
	return data;
}

static void s3c2410_nand_data_w(int device, offs_t offset, UINT8 data)
{
}

static UINT8 eeprom[0x800];
static int m_scl, m_sdaw, m_sdar, m_state, m_bits, m_shift, m_devsel, m_byteaddr, m_page_offset;
enum { STATE_IDLE, STATE_DEVSEL, STATE_BYTEADDR, STATE_DATAIN, STATE_DATAOUT };
#define DEVSEL_RW 1
static int select_device(void) { return (m_devsel & 0xf0) == 0xa0; }
static int data_offset(void) { return (((m_devsel & 0x0e) << 7) | (m_byteaddr & 0xff)) & 0x7ff; }
static int s3c2410_i2c_sda_r(int device) { return m_sdar; }
void s3c2410_i2c_sda_w(int device, int state)
{
	state &= 1;
	if( m_sdaw != state )
	{
		m_sdaw = state;

		if( m_scl )
		{
			if( m_sdaw )
			{
				m_state = STATE_IDLE;
			}
			else
			{
				m_state = STATE_DEVSEL;
				m_bits = 0;
			}

			m_sdar = 1;
		}
	}
}

void s3c2410_i2c_scl_w(int device, int state)
{
	if( m_scl != state )
	{
		m_scl = state;

		switch( m_state )
		{
		case STATE_DEVSEL:
		case STATE_BYTEADDR:
		case STATE_DATAIN:
			if( m_bits < 8 )
			{
				if( m_scl )
				{
					m_shift = ( ( m_shift << 1 ) | m_sdaw ) & 0xff;
					m_bits++;
				}
			}
			else
			{
				if( m_scl )
				{
					switch( m_state )
					{
					case STATE_DEVSEL:
						m_devsel = m_shift;

						if( !select_device() )
						{
							m_state = STATE_IDLE;
						}
						else if( ( m_devsel & DEVSEL_RW ) == 0 )
						{
							m_state = STATE_BYTEADDR;
						}
						else
						{
							m_state = STATE_DATAOUT;
						}
						break;

					case STATE_BYTEADDR:
						m_byteaddr = m_shift;
						m_page_offset = 0;

						m_state = STATE_DATAIN;
						break;

					case STATE_DATAIN:
                        eeprom[data_offset()] = m_shift;
                        ++m_byteaddr;
						break;
					}

					m_bits++;
				}
				else
				{
					if( m_bits == 8 )
					{
						m_sdar = 0;
					}
					else
					{
						m_bits = 0;
						m_sdar = 1;
					}
				}
			}
			break;

		case STATE_DATAOUT:
			if( m_bits < 8 )
			{
				if( m_scl )
				{
					if( m_bits == 0 )
					{
						int offset = data_offset();

						m_shift = eeprom[offset];
						m_byteaddr++;
					}

					m_sdar = ( m_shift >> 7 ) & 1;

					m_shift = ( m_shift << 1 ) & 0xff;
					m_bits++;
				}
			}
			else
			{
				if( m_scl )
				{
					if( m_sdaw )
					{
						m_state = STATE_IDLE;
						m_sdar = 0;
					}

					m_bits++;
				}
				else
				{
					if( m_bits == 8 )
					{
						m_sdar = 1;
					}
					else
					{
						m_bits = 0;
					}
				}
			}
			break;
		}
	}
}

static NVRAM_HANDLER( ghosteo )
{
	if (read_or_write) mame_fwrite(file, eeprom, sizeof(eeprom));
	else if (file) mame_fread(file, eeprom, sizeof(eeprom));
	else memset(eeprom, 0xff, sizeof(eeprom));
}
static int qs_bank;
static WRITE32_HANDLER( sound_w )
{
	if (!(mem_mask & 0xff))
	{
		soundlatch_w(0, data);
		cpunum_set_input_line(1, I8051_INT1_LINE, ASSERT_LINE);
		cpu_boost_interleave(0, TIME_IN_USEC(250));
	}
}
static READ8_HANDLER( qs_rom_r )
{
	return memory_region(REGION_CPU2)[qs_bank * 0x10000 + offset + 0x100];
}
static READ8_HANDLER( qs_command_r ) { return soundlatch_r(0); }
static WRITE8_HANDLER( qs_p3_w )
{
	qs_bank = data & 7;
	if (!(data & 0x20)) cpunum_set_input_line(1, I8051_INT1_LINE, CLEAR_LINE);
}
static READ32_HANDLER( ghosteo_soc_r )
{
	UINT32 address = 0x48000000 + offset * 4;
	if (address == 0x4d000010)
	{
		offs_t pc = activecpu_get_pc();
		/* These waits poll the LCD phase; sleeping keeps the beam and timers moving. */
		if (pc == 0x3001c0e4 || pc == 0x3001c0d8 || pc == 0x3002b580 || pc == 0x3002b550)
			cpu_spinuntil_time(TIME_IN_USEC(20));
	}
	return s3c2410_r(offset, mem_mask);
}
static ADDRESS_MAP_START( ghosteo_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00000fff) AM_RAM AM_BASE(&steppingstone) AM_MIRROR(0x40000000)
	AM_RANGE(0x10000000, 0x10000003) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x10100000, 0x10100003) AM_READ(input_port_1_dword_r)
	AM_RANGE(0x10200000, 0x10200003) AM_READ(input_port_2_dword_r)
	AM_RANGE(0x10300000, 0x10300003) AM_WRITE(sound_w)
	AM_RANGE(0x30000000, 0x31ffffff) AM_RAM AM_BASE(&system_memory) AM_MIRROR(0x02000000)
	AM_RANGE(0x48000000, 0x5affffff) AM_READWRITE(ghosteo_soc_r, s3c2410_w)
ADDRESS_MAP_END
static ADDRESS_MAP_START( qs_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START( qs_data, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE(0x0000, 0x00ff) AM_RAM
	AM_RANGE(0x0100, 0xffff) AM_READ(qs_rom_r)
	AM_RANGE(0x0200, 0x0211) AM_WRITE(eolith_qs_wave_w)
ADDRESS_MAP_END
static ADDRESS_MAP_START( qs_ports, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(1, 1) AM_READ(qs_command_r)
	AM_RANGE(2, 2) AM_WRITE(eolith_qs_p2_w)
	AM_RANGE(3, 3) AM_WRITE(qs_p3_w)
ADDRESS_MAP_END
static const s3c2410_interface soc_interface =
{
	{ s3c2410_gpio_port_r, s3c2410_gpio_port_w },
	{ s3c2410_i2c_scl_w, s3c2410_i2c_sda_r, s3c2410_i2c_sda_w },
	{ 0 }, { 0 },
	{ s3c2410_nand_command_w, s3c2410_nand_address_w, s3c2410_nand_data_r, s3c2410_nand_data_w }
};
static MACHINE_START( ghosteo ) { s3c2410_init(&soc_interface); return 0; }
static MACHINE_RESET( ghosteo )
{
	s3c2410_reset();
	eolith_qs_reset();
	timer_set(TIME_NOW, 1, eolith_qs_configure);
	security_count = qs_bank = 0;
	memset(bballoon_port, 0, sizeof(bballoon_port));
	memset(&nand, 0, sizeof(nand));
	m_scl = m_sdaw = m_state = m_bits = m_shift = m_devsel = m_byteaddr = 0;
	m_sdar = 1;
	/* NAND boot hardware fills its 4 KiB SRAM before releasing the CPU. */
	memcpy(steppingstone, memory_region(REGION_USER1), 0x1000);
}
INPUT_PORTS_START( bballoon )
	PORT_START
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0xffffffc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0xffffffc0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_SERVICE_NO_TOGGLE( 0x00000020, IP_ACTIVE_LOW )
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xffffff50, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END
static MACHINE_DRIVER_START( ghosteo )
	MDRV_CPU_ADD(ARM920T, 200000000)
	MDRV_CPU_PROGRAM_MAP(ghosteo_map, 0)
	MDRV_CPU_ADD(I8052, 24000000)
	MDRV_CPU_PROGRAM_MAP(qs_map, 0)
	MDRV_CPU_DATA_MAP(qs_data, 0)
	MDRV_CPU_IO_MAP(qs_ports, 0)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)
	MDRV_MACHINE_START(ghosteo)
	MDRV_MACHINE_RESET(ghosteo)
	MDRV_NVRAM_HANDLER(ghosteo)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(1024, 1024)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(256)
	MDRV_VIDEO_START(s3c2410)
	MDRV_VIDEO_UPDATE(s3c2410)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 24000000)
	MDRV_SOUND_CONFIG(eolith_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END
ROM_START( bballoon )
	ROM_REGION( 0x2000000, REGION_USER1, 0 )
	ROM_LOAD( "flash.u1",     0x000000, 0x2000000, BAD_DUMP CRC(73285634) SHA1(4d0210c1bebdf3113a99978ffbcd77d6ee854168) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "b2.u20", 0x000000, 0x080000, CRC(0a12334c) SHA1(535b5b34f28435517218100d70147d87809f485a) )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )
	ROM_LOAD( "b1.u16",      0x000000, 0x100000, CRC(c42c1c85) SHA1(e1f49d556ffd6bc27142a7784c3bb8e37999857d) )
	ROM_LOAD( "qs1001a.u17", 0x200000, 0x080000, CRC(d13c6407) SHA1(57b14f97c7d4f9b5d9745d3571a0b7115fbe3176) )
ROM_END

ROM_START( hapytour )
	ROM_REGION( 0x2000000, REGION_USER1, 0 )
	ROM_LOAD( "flash.u1", 0x000000, 0x2000000, BAD_DUMP CRC(49deb7f9) SHA1(708a27d7177cf6261a49ded975c2bbb6c2427742) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "ht.u20", 0x000000, 0x080000, CRC(c0581fce) SHA1(dafce679002534ffabed249a92e6b83301b8312b) )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )
	ROM_LOAD( "ht.u16",      0x000000, 0x100000, CRC(6a590a3a) SHA1(c1140f70c919661162334db66c6aa0ad656bfc47) )
	ROM_LOAD( "qs1001a.u17", 0x200000, 0x080000, CRC(d13c6407) SHA1(57b14f97c7d4f9b5d9745d3571a0b7115fbe3176) )
ROM_END

GAME( 2003, bballoon, 0, ghosteo, bballoon, 0, ROT0, "Eolith", "BnB Arcade (V1.0005 World)", GAME_IMPERFECT_SOUND )
GAME( 2005, hapytour, 0, ghosteo, bballoon, 0, ROT0, "GAV Company", "Happy Tour (V1.12)", GAME_IMPERFECT_SOUND )
