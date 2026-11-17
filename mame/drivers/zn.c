// license:BSD-3-Clause
// copyright-holders:smf, R. Belmont
/***************************************************************************

  Sony ZN1/ZN2 - Arcade PSX Hardware
  ==================================
  Driver by smf & R Belmont
  Board notes by The Guru
  Thanks to Zinc Team, Amuse & Miguel Angel Horna

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/mips/psx.h"
#include "cpu/z80/z80.h"
#include "includes/psx.h"
#include "machine/at28c16.h"
#include "machine/mb3773.h"
#include "machine/znsec.h"
#include "machine/idectrl.h"
#include "sndhrdw/taitosnd.h"
#include "sound/2610intf.h"
#include "sound/ymz280b.h"
#include "sound/qsound.h"
#include "sound/psx.h"
#include "sound/ymf271.h"

#define VERBOSE_LEVEL ( 0 )

INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		if( cpu_getactivecpu() != -1 )
		{
			logerror( "%08x: %s", activecpu_get_pc(), buf );
		}
		else
		{
			logerror( "(timer) : %s", buf );
		}
	}
}

INLINE UINT8 psxreadbyte( UINT32 n_address )
{
	return *( (UINT8 *)g_p_n_psxram + BYTE4_XOR_LE( n_address ) );
}

INLINE void psxwritebyte( UINT32 n_address, UINT8 n_data )
{
	psxcpu_ram_written( (UINT8 *)g_p_n_psxram + BYTE4_XOR_LE( n_address ), 1 );
	*( (UINT8 *)g_p_n_psxram + BYTE4_XOR_LE( n_address ) ) = n_data;
}


static UINT32 m_n_znsecsel;
static UINT32 m_b_znsecport;
static int m_n_dip_bit;
static int m_b_lastclock;

static READ32_HANDLER( znsecsel_r )
{
	verboselog( 2, "znsecsel_r( %08x, %08x )\n", offset, mem_mask );
	return m_n_znsecsel;
}

static void sio_znsec0_handler( int n_data )
{
	if( ( n_data & PSX_SIO_OUT_CLOCK ) == 0 )
	{
		if( m_b_lastclock )
		{
			psx_sio_input( 0, PSX_SIO_IN_DATA, ( znsec_step( 0, ( n_data & PSX_SIO_OUT_DATA ) / PSX_SIO_OUT_DATA ) & 1 ) * PSX_SIO_IN_DATA );
		}
		m_b_lastclock = 0;
	}
	else
	{
		m_b_lastclock = 1;
	}
}

static void sio_znsec1_handler( int n_data )
{
	if( ( n_data & PSX_SIO_OUT_CLOCK ) == 0 )
	{
		if( m_b_lastclock )
		{
			psx_sio_input( 0, PSX_SIO_IN_DATA, ( znsec_step( 1, ( n_data & PSX_SIO_OUT_DATA ) / PSX_SIO_OUT_DATA ) & 1 ) * PSX_SIO_IN_DATA );
		}
		m_b_lastclock = 0;
	}
	else
	{
		m_b_lastclock = 1;
	}
}

static void sio_pad_handler( int n_data )
{
	if( ( n_data & PSX_SIO_OUT_DTR ) != 0 )
	{
		m_b_znsecport = 1;
	}
	else
	{
		m_b_znsecport = 0;
	}

	verboselog( 2, "read pad %04x %04x %02x\n", m_n_znsecsel, m_b_znsecport, n_data );
	psx_sio_input( 0, PSX_SIO_IN_DATA | PSX_SIO_IN_DSR, PSX_SIO_IN_DATA | PSX_SIO_IN_DSR );
}

static void *dip_timer;
static int m_n_dip_byte;

static void dip_timer_fired( int param )
{
	psx_sio_input( 0, PSX_SIO_IN_DSR, param * PSX_SIO_IN_DSR );

	if( param )
	{
		timer_adjust( dip_timer, TIME_IN_USEC( 5 ), 0, 0 );
	}
}

static void sio_dip_handler( int n_data )
{
	if( ( n_data & PSX_SIO_OUT_CLOCK ) == 0 )
	{
		if( m_b_lastclock )
		{
			int n_value = 0;
			if( m_n_dip_byte == 0 )
			{
				/* the mcu sends the number of data bytes (1) in the high nibble and the dips in the low nibble */
				n_value = 0x10 | ( readinputportbytag( "DSW" ) & 0x0f );
			}
			verboselog( 2, "read dip %02x -> %02x\n", n_data, ( ( n_value >> m_n_dip_bit ) & 1 ) * PSX_SIO_IN_DATA );
			psx_sio_input( 0, PSX_SIO_IN_DATA, ( ( n_value >> m_n_dip_bit ) & 1 ) * PSX_SIO_IN_DATA );
			++m_n_dip_bit;
			if( m_n_dip_bit == 8 )
			{
				/* the games wait for the ack before reading each following data byte */
				/* Ray Storm boots in the test mode when the data byte is acknowledged,
				   the mcu is not documented well enough to know why */
				if( m_n_dip_byte < 1 && strncmp( Machine->gamedrv->name, "raystorm", 8 ) != 0 )
				{
					timer_adjust( dip_timer, TIME_IN_USEC( 50 ), 1, 0 );
				}
				m_n_dip_bit = 0;
				++m_n_dip_byte;
			}
		}
		m_b_lastclock = 0;
	}
	else
	{
		m_b_lastclock = 1;
	}
}

static WRITE32_HANDLER( znsecsel_w )
{
	COMBINE_DATA( &m_n_znsecsel );

	/* deselecting the dips mcu cancels the pending ack pulses */
	timer_adjust( dip_timer, TIME_NEVER, 0, 0 );

	if( ( m_n_znsecsel & 0x80 ) == 0 )
	{
		psx_sio_install_handler( 0, sio_pad_handler );
	}
	else if( ( m_n_znsecsel & 0x08 ) == 0 )
	{
		znsec_start( 1 );
		psx_sio_install_handler( 0, sio_znsec1_handler );
		psx_sio_input( 0, PSX_SIO_IN_DSR, 0 );
	}
	else if( ( m_n_znsecsel & 0x04 ) == 0 )
	{
		znsec_start( 0 );
		psx_sio_install_handler( 0, sio_znsec0_handler );
		psx_sio_input( 0, PSX_SIO_IN_DSR, 0 );
	}
	else
	{
		/* the dips are read from the first bit and the games wait for the ack line
		   to be pulsed, without it they are stuck in the test mode */
		m_n_dip_bit = 0;
		m_n_dip_byte = 0;
		m_b_lastclock = 1;

		psx_sio_install_handler( 0, sio_dip_handler );
		psx_sio_input( 0, PSX_SIO_IN_DSR, 0 );

		timer_adjust( dip_timer, TIME_IN_USEC( 50 ), 1, 0 );
	}

	verboselog( 2, "znsecsel_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
}

static READ32_HANDLER( jamma_0_r )
{
	return readinputport( 0 );
}

static READ32_HANDLER( jamma_1_r )
{
	return readinputport( 1 );
}

static READ32_HANDLER( jamma_2_r )
{
	return readinputport( 2 );
}

static READ32_HANDLER( jamma_3_r )
{
	return readinputport( 3 );
}

static READ32_HANDLER( jamma_4_r )
{
	return readinputport( 4 );
}

static READ32_HANDLER( jamma_5_r )
{
	return readinputport( 5 );
}

static READ32_HANDLER( boardconfig_r )
{
	/*
    ------00 mem=4M
    ------01 mem=4M
    ------10 mem=8M
    ------11 mem=16M
    -----0-- smem=hM
    -----1-- smem=2M
    ----0--- vmem=1M
    ----1--- vmem=2M
    000----- rev=-2
    001----- rev=-1
    010----- rev=0
    011----- rev=1
    100----- rev=2
    101----- rev=3
    110----- rev=4
    111----- rev=5
    */

	if( Machine->drv->screen_height == 1024 )
	{
		return 64|32|8 | ( g_n_psxramsize == 0x800000 ? 2 : 0 );
	}
	else
	{
		return 64|32;
	}
}

static READ32_HANDLER( unknown_r )
{
	verboselog( 0, "unknown_r( %08x, %08x )\n", offset, mem_mask );
	return 0xffffffff;
}

static WRITE32_HANDLER( coin_w )
{
	/* 0x01=counter
       0x02=coin lock 1
       0x08=??
       0x20=coin lock 2
       0x80=??
    */
	if( ( data & ~0x23 ) != 0 )
	{
		verboselog( 0, "coin_w %08x\n", data );
	}
}

static ADDRESS_MAP_START( zn_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM	AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize) /* ram */
	AM_RANGE(0x00400000, 0x007fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f80100f) AM_RAM /* ?? */
	AM_RANGE(0x1f801010, 0x1f801013) AM_NOP
	AM_RANGE(0x1f801014, 0x1f801017) AM_READWRITE(psx_spu_delay_r, psx_spu_delay_w)
	AM_RANGE(0x1f801018, 0x1f80101f) AM_NOP
	AM_RANGE(0x1f801020, 0x1f801023) AM_READWRITE(psx_com_delay_r, psx_com_delay_w)
	AM_RANGE(0x1f801024, 0x1f80102f) AM_NOP
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READWRITE(psx_sio_r, psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_RAM
	AM_RANGE(0x1f801070, 0x1f801077) AM_READWRITE(psx_irq_r, psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READWRITE(psx_dma_r, psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80112f) AM_READWRITE(psx_counter_r, psx_counter_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READWRITE(psx_gpu_r, psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READWRITE(psx_mdec_r, psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_READWRITE(psx_spu_r, psx_spu_w)
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM /* ?? */
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fa00000, 0x1fa00003) AM_READ(jamma_0_r)
	AM_RANGE(0x1fa00100, 0x1fa00103) AM_READ(jamma_1_r)
	AM_RANGE(0x1fa00200, 0x1fa00203) AM_READ(jamma_2_r)
	AM_RANGE(0x1fa00300, 0x1fa00303) AM_READ(jamma_3_r)
	AM_RANGE(0x1fa10000, 0x1fa10003) AM_READ(jamma_4_r)
	AM_RANGE(0x1fa10100, 0x1fa10103) AM_READ(jamma_5_r)
	AM_RANGE(0x1fa10200, 0x1fa10203) AM_READ(boardconfig_r)
	AM_RANGE(0x1fa10300, 0x1fa10303) AM_READWRITE(znsecsel_r, znsecsel_w)
	AM_RANGE(0x1fa20000, 0x1fa20003) AM_WRITE(coin_w)
	AM_RANGE(0x1fa30000, 0x1fa30003) AM_NOP /* ?? */
	AM_RANGE(0x1fa40000, 0x1fa40003) AM_READNOP /* ?? */
	AM_RANGE(0x1fa60000, 0x1fa60003) AM_READNOP /* ?? */
	AM_RANGE(0x1faf0000, 0x1faf07ff) AM_READWRITE( at28c16_32le_0_r, at28c16_32le_0_w ) /* eeprom */
	AM_RANGE(0x1fb20000, 0x1fb20007) AM_READ(unknown_r)
	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0) /* bios */
	AM_RANGE(0x80000000, 0x803fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x80400000, 0x807fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xa0000000, 0xa03fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_WRITENOP AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
ADDRESS_MAP_END

static NVRAM_HANDLER( zn_eeprom )
{
	const UINT8 *defaults = memory_region( REGION_USER6 );
	int i;
	nvram_handler_at28c16_0( file, read_or_write );
	if( !read_or_write && !file && defaults )
	{
		/* Keep the native EEPROM's state registration and ID bytes while
		   supplying the game's factory settings on the first boot. */
		for( i = 0; i < 0x800; i += 4 )
			at28c16_32le_0_w( i / 4, defaults[i] | ( defaults[i+1] << 8 ) |
				( defaults[i+2] << 16 ) | ( (UINT32)defaults[i+3] << 24 ), 0 );
	}
}

static void zn_driver_init( void )
{
	psx_driver_init();

	dip_timer = timer_alloc( dip_timer_fired );

	znsec_init( 0, memory_region( REGION_USER4 ) );
	if( memory_region( REGION_USER5 ) )
	{
		znsec_init( 1, memory_region( REGION_USER5 ) );
	}
	psx_sio_install_handler( 0, sio_pad_handler );
}

static struct PSXSPUinterface psxspu_interface =
{
	&g_p_n_psxram,
	psx_irq_set,
	psx_dma_install_read_handler,
	psx_dma_install_write_handler
};

static void zn_machine_init( void )
{
	m_n_dip_bit = 0;
	m_b_lastclock = 1;
	psx_machine_init();
}

/*
Capcom ZN1 generic PCB Layout
----------------------------

Main board (Standard ZN1 Main Board with Capcom ZN1 BIOS)
ZN-1 1-659-709-12  COH-1000C
|--------------------------------------------------------|
|  LA4705             |---------------------------|      |
|                     |---------------------------|      |
|    AKM_AK4310VM      AT28C16                           |
|  VOL                                                   |
|       SW1            BIOS                              |
|  TD62064                                               |
|  TD62064        PALCE16V8                              |
|J                                                       |
|                                                        |
|A                EPM7032    814260    CXD2922CQ         |
|                                                        |
|M                                                       |
|                                                        |
|M                                                       |
|                *                                       |
|A           DSW                                         |
|                                CXD8561Q    CXD8530CQ   |
|                KM4132G271Q-12                          |
|CN505  CN506                   53.693MHz    67.737MHz   |
|            CAT702                                      |
|                                                        |
|CN504  CN503                                            |
|                                                        |
|                                                        |
|  NEC_78081G503         KM48V514BJ-12   KM48V514BJ-12   |
|            MC44200FT   KM48V514BJ-12   KM48V514BJ-12   |
|CN651  CN652            KM48V514BJ-12   KM48V514BJ-12   |
|                CN654   KM48V514BJ-12   KM48V514BJ-12   |
|--------------------------------------------------------|
Notes:
      CN506 - Connector for optional 3rd player controls
      CN505 - Connector for optional 4th player controls
      CN503 - Connector for optional 15kHz external video output (R,G,B,Sync, GND)
      CN504 - Connector for optional 2nd speaker (for stereo output)
      CN652 - Connector for optional trackball
      CN651 - Connector for optional analog controls
      CN654 - Connector for optional memory card
      SW1   - Slide switch for stereo or mono sound output
      DSW   - Dip switch (4 position, defaults all OFF)

      BIOS           - COH1000C.353, Capcom ZN1 BIOS, 4MBit MaskROM type M534002 (SOP40)
      AT28C16        - Atmel AT28C16 2K x8 EEPROM (SOP24)
      814260-70      - 256K x16 (4MBit) DRAM (SOJ40)
      KM4132G271Q-12 - 128K x32 x2 banks (32MBit) SGRAM (QFP100)
      *              - Unpopulated position for KM4132G271Q-12 SGRAM
      KM48V514BJ-6   - 512k x8 (4MBit) DRAM (SOJ28)
      EPM7032        - Altera EPM7032QC44-15 CPLD labelled 'ZN1A' (QFP44)
      CAT702         - Protection chip labelled 'CP01' (DIP20)
      PALCE16V8      - PAL, labelled 'ZN1A' (PLCC20)


Game board (Gallop Racer)

95681-2
|--------------------------------------|
|    |---------------------------|     |
|    |---------------------------|     |
|LM833    60MHz              1.3B  8MHz|
|                                      |
|                                  Z80 |
|NE5532        CAPCOM-Q1               |
|                                      |
|                                      |
|TDA1543   93C46   C.P.S.2-B           |
|                            3.3E  2.2E|
|                                      |
|CN2                                   |
|  CAT702                         8464 |
|                                      |
|                                      |
|  PAL1                                |
|                                      |
|               7.5H  6.4H   5.3H  4.2H|
|  PAL2                                |
|PAL3                                  |
|                                      |
|M54532                                |
|                                      |
|                                      |
|13.7K  12.6K  11.5K  10.4K  9.3K  8.2K|
|                                      |
|                                      |
|--------------------------------------|
Notes:
      CN2    - Standard 34 pin CAPCOM connector for extra player controls.
      CAT702 - protection chip labelled 'CP05' (DIP20)
      PAL1   - PAL16L8 stamped "CS1CNT"
      PAL2   - PALCE16V8 stamped "BANK01"
      PAL3   - PALCE16V8 stamped "CS0CNT"
      4.2H   - 27C4002 DIP40 EPROM labelled 'GPAJ_04'
      5.3H   - uPD23C32020CZ 32MBit DIP42 MaskROM labelled 'GRA-05M'
      6.4H   - uPD23C32020CZ 32MBit DIP42 MaskROM labelled 'GRA-06M'
      7.5H   - uPD23C32020CZ 32MBit DIP42 MaskROM labelled 'GRA-07M'
      8464   - 8K x8 SRAM
      LM833  - National Semiconductor Dual Audio Operational Amplifier
      NE5532 - Fairchild Semiconductor Dual Operational Amplifier
      M54532 - Mitsubishi M54532 IC, connected to CN2
      93C46  - 128Bytes EEPROM
      TDA1543- PHILIPS Dual 16-bit DAC
      CAPCOM-Q1 - Q-Sound chip also stamped DL-1425 45570 9420S 40 (C)92 AT&T (PLCC84)
      C.P.S.2-B - RF5C320 CAPCOM C.P.S.2-B DL-3129 (QFP208)
      Z80 clock - 4.000MHz

      Unpopulated sockets - 1.3B, 2.2E, 3.3E, 8.2K, 9.3K, 10.4K, 11.5K, 12.6K & 13.7K
*/

static READ32_HANDLER( capcom_kickharness1_r )
{
	/* required for buttons 4,5&6 */
	verboselog( 2, "capcom_kickharness1_r( %08x, %08x )\n", offset, mem_mask );
	return readinputportbytag_safe( "KICK1", 0xffffffff );
}

static READ32_HANDLER( capcom_kickharness2_r )
{
	verboselog( 2, "capcom_kickharness2_r( %08x, %08x )\n", offset, mem_mask );
	return readinputportbytag_safe( "KICK2", 0xffffffff );
}

static WRITE32_HANDLER( bank_coh1000c_w )
{
	memory_set_bankptr( 2, memory_region( REGION_USER2 ) + 0x400000 + ( data * 0x400000 ) );
}

static WRITE8_HANDLER( qsound_bankswitch_w )
{
	memory_set_bankptr( 10, memory_region( REGION_CPU2 ) + 0x10000 + ( ( data & 0x0f ) * 0x4000 ) );
}

static INTERRUPT_GEN( qsound_interrupt )
{
	cpunum_set_input_line(1, 0, HOLD_LINE);
}

static WRITE32_HANDLER( zn_qsound_w )
{
	soundlatch_w(0, data);
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

DRIVER_INIT( coh1000c )
{
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f000000, 0x1f3fffff, 0, 0, MRA32_BANK1 );     /* fixed game rom */
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f400000, 0x1f7fffff, 0, 0, MRA32_BANK2 );     /* banked game rom */
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb00000, 0x1fb00003, 0, 0, bank_coh1000c_w ); /* bankswitch */
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fb40010, 0x1fb40013, 0, 0, capcom_kickharness1_r );
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fb40020, 0x1fb40023, 0, 0, capcom_kickharness2_r );
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fb80000, 0x1fbfffff, 0, 0, MRA32_BANK3 );     /* country rom */
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb60000, 0x1fb60003, 0, 0, zn_qsound_w );

	zn_driver_init();

	if( strcmp( Machine->gamedrv->name, "glpracr" ) == 0 ||
		strcmp( Machine->gamedrv->name, "glpracrj" ) == 0 ||
		strcmp( Machine->gamedrv->name, "glpracr2l" ) == 0 )
	{
		/* disable:
            the QSound CPU for glpracr as it doesn't have any roms &
            the link cpu for glprac2l as the h/w is not emulated yet. */
		cpunum_suspend( 1, SUSPEND_REASON_DISABLE, 1 );
	}
}

MACHINE_RESET( coh1000c )
{
	memory_set_bankptr( 1, memory_region( REGION_USER2 ) ); /* fixed game rom */
	memory_set_bankptr( 2, memory_region( REGION_USER2 ) + 0x400000 ); /* banked game rom */
	memory_set_bankptr( 3, memory_region( REGION_USER3 ) ); /* country rom */
	zn_machine_init();
}

static ADDRESS_MAP_START( qsound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK10)	/* banked (contains music data) */
	AM_RANGE(0xd007, 0xd007) AM_READ(qsound_status_r)
	AM_RANGE(0xf000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( qsound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xd000, 0xd000) AM_WRITE(qsound_data_h_w)
	AM_RANGE(0xd001, 0xd001) AM_WRITE(qsound_data_l_w)
	AM_RANGE(0xd002, 0xd002) AM_WRITE(qsound_cmd_w)
	AM_RANGE(0xd003, 0xd003) AM_WRITE(qsound_bankswitch_w)
	AM_RANGE(0xf000, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( qsound_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static struct QSound_interface qsound_interface =
{
	REGION_SOUND1
};

static MACHINE_DRIVER_START( coh1000c )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( Z80, 8000000 )
	/* audio CPU */  /* 8MHz ?? */
	MDRV_CPU_PROGRAM_MAP( qsound_readmem, qsound_writemem )
	MDRV_CPU_IO_MAP( qsound_readport, 0 )
	MDRV_CPU_VBLANK_INT( qsound_interrupt, 4 ) /* 4 interrupts per frame ?? */

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( coh1000c )
	MDRV_NVRAM_HANDLER( zn_eeprom )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 512 )
	MDRV_VISIBLE_AREA( 0, 512-1, 0, 240-1 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)

	MDRV_SOUND_ADD( QSOUND, QSOUND_CLOCK )
	MDRV_SOUND_CONFIG( qsound_interface )
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coh1002c )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( Z80, 8000000 )
	/* audio CPU */  /* 8MHz ?? */
	MDRV_CPU_PROGRAM_MAP( qsound_readmem, qsound_writemem )
	MDRV_CPU_IO_MAP( qsound_readport, 0 )
	MDRV_CPU_VBLANK_INT( qsound_interrupt, 4 ) /* 4 interrupts per frame ?? */

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( coh1000c )
	MDRV_NVRAM_HANDLER( zn_eeprom )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 512-1, 0, 240-1 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)

	MDRV_SOUND_ADD( QSOUND, QSOUND_CLOCK )
	MDRV_SOUND_CONFIG( qsound_interface )
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

/*

Capcom ZN2 generic PCB Layout
-----------------------------

PCB Layouts
-----------

1-665-825-11
ZN-2 COH-3000 (sticker says COH-3002C denoting Capcom ZN2 BIOS version)
|--------------------------------------------------------|
|  LA4705             |---------------------------|      |
|                     |---------------------------|      |
|    AKM_AK4310VM      AT28C16                           |
|  VOL                                                   |
|       S301           BIOS                              |
|                                                        |
|                                                        |
|J                                                       |
|                                                        |
|A              814260    CXD2925Q     EPM7064           |
|                                                        |
|M                                     67.73MHz          |
|                                                        |
|M                                                       |
|            S551    KM4132G271BQ-8                      |
|A                                                       |
|                                CXD8654Q    CXD8661R    |
|                    KM4132G271BQ-8                      |
|CN505  CN506                   53.693MHz    100MHz      |
|            CAT702                                      |
|                                                        |
|CN504  CN503                                            |
|                                                        |
|            MC44200FT                                   |
|  NEC_78081G503        KM416V1204BT-L5  KM416V1204BT-L5 |
|                                                        |
|CN651  CN652                 *                 *        |
|                CN654                                   |
|--------------------------------------------------------|
Notes:
      CN506 - Connector for optional 3rd player controls
      CN505 - Connector for optional 4th player controls
      CN503 - Connector for optional 15kHz external video output (R,G,B,Sync, GND)
      CN504 - Connector for optional 2nd speaker (for stereo output)
      CN652 - Connector for optional trackball
      CN651 - Connector for optional analog controls
      CN654 - Connector for optional memory card
      S301  - Slide switch for stereo or mono sound output
      S551  - Dip switch (4 position, defaults all OFF)

      BIOS           - COH-3002C.353, Capcom ZN2 BIOS 4MBit MaskROM type M534002 (SOP40)
      AT28C16        - Atmel AT28C16 2K x8 EEPROM
      814260-70      - 256K x16 (4MBit) DRAM
      KM4132G271BQ-8 - 128K x 32Bit x 2 Banks SGRAM
      KM416V1204BT-L5- 1M x16 EDO DRAM
      EPM7064        - Altera EPM7064QC100 CPLD (QFP100)
      CAT702         - Protection chip labelled 'CP10' (DIP20)
      *              - Unpopulated position for additional KM416V1204BT-L5 RAMs


Game board
(This covers at least Rival Schools and Street Fighter EX2, but likely all Capcom ZN2 games)

97695-1
|--------------------------------------|
|    |---------------------------|     |
|    |---------------------------|     |
|LM833    60MHz              1.3A  8MHz|
|                                      |
|                                  Z80 |
|NE5532        CAPCOM-Q1               |
|                                      |
|                                      |
|TDA1543   93C46   C.P.S.2-B           |
|                            3.3E  2.2E|
|                                      |
|CN2                                   |
|  CAT702                      N341256 |
|                                      |
|                                      |
|  PAL1                                |
|                                      |
|               7.5H  6.4H   5.3H  4.2H|
|  PAL2                                |
|PAL3                                  |
|                                      |
|M54532                                |
|                                      |
|                                      |
|13.7K  12.6K  11.5K  10.4K  9.3K  8.2K|
|                                      |
|                                      |
|--------------------------------------|
Notes:
      CN2    - Standard 34 pin CAPCOM connector for extra player controls.
      CAT702 - protection chip
                               SFEX2 labelled 'CP08' (DIP20)
                               Rival Schools labelled 'CP06' (DIP20)
      PAL1   - PAL16L8 stamped "CS1CNT"
      PAL2   - PALCE16V8 stamped "BANK02"
      PAL3   - PALCE16V8 stamped "MEMDEC"
      N341256- 32K x8 SRAM
      LM833  - National Semiconductor Dual Audio Operational Amplifier
      NE5532 - Fairchild Semiconductor Dual Operational Amplifier
      M54532 - Mitsubishi M54532 IC, connected to CN2
      93C46  - 128Bytes EEPROM
      TDA1543- PHILIPS Dual 16-bit DAC
      CAPCOM-Q1 - Q-Sound chip also stamped DL-1425 11008 9741T 74 (C)92 LUCENT (PLCC84)
      C.P.S.2-B - RF5C320 CAPCOM C.P.S.2-B DL-3129 (QFP208)
      Z80 clock - 4.000MHz
      ROMs      -
                  SFEX2
                       1.3A   - uPD23C32020CZ 32MBit DIP42 MaskROM labelled 'EX2-01M'
                       2.2E   - 27C1001 DIP32 EPROM labelled 'EX2_02'
                       4.2H   - 27C240 DIP40 EPROM labelled 'EX2J_04'
                       5.3H   - 64MBit DIP42 MaskROM labelled 'EX2-05M'  \
                       6.4H   - 64MBit DIP42 MaskROM labelled 'EX2-06M'  |
                       7.5H   - 64MBit DIP42 MaskROM labelled 'EX2-07M'  |  Unknown type manufactured by Sharp
                       8.2K   - 64MBit DIP42 MaskROM labelled 'EX2-08M'  /
                       9.3K   - uPD23C32020CZ 32MBit DIP42 MaskROM labelled 'EX2-09M'
                       Unpopulated sockets on SFEX2 - 3.3E, 10.4K, 11.5K, 12.6K & 13.7K

                  Rival Schools
                       1.3A   - M533203E 32MBit DIP42 MaskROM labelled 'JST-01M'
                       2.2E   - 27C1001 DIP32 EPROM labelled 'JST_02'
                       3.3E   - 27C1001 DIP32 EPROM labelled 'JST_03'
                       4.2H   - 27C240 DIP40 EPROM labelled 'JSTJ_04A'
                       5.3H   - M533203E 32MBit DIP42 MaskROM labelled 'JST-05M'
                       6.4H   - M533203E 32MBit DIP42 MaskROM labelled 'JST-06M'
                       7.5H   - M533203E 32MBit DIP42 MaskROM labelled 'JST-07M'
                       8.2K   - TC5332202 32MBit DIP42 MaskROM labelled 'JST-08M'
                       9.3K   - TC5332202 32MBit DIP42 MaskROM labelled 'JST-09M'
                       10.4K  - TC5332202 32MBit DIP42 MaskROM labelled 'JST-10M'
                       11.5K  - GM23C32100A 32MBit DIP42 MaskROM labelled 'JST-11M'
                       12.6K  - GM23C32100A 32MBit DIP42 MaskROM labelled 'JST-12M'
                       13.7K  - GM23C32100A 32MBit DIP42 MaskROM labelled 'JST-13M'
                       Unpopulated sockets on Rival Schools - None
*/

static WRITE32_HANDLER( bank_coh3002c_w )
{
	memory_set_bankptr( 2, memory_region( REGION_USER2 ) + 0x400000 + ( data * 0x400000 ) );
}

DRIVER_INIT( coh3002c )
{
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f000000, 0x1f3fffff, 0, 0, MRA32_BANK1 );     /* fixed game rom */
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f400000, 0x1f7fffff, 0, 0, MRA32_BANK2 );     /* banked game rom */
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fb40010, 0x1fb40013, 0, 0, capcom_kickharness1_r );
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fb40020, 0x1fb40023, 0, 0, capcom_kickharness2_r );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb00000, 0x1fb00003, 0, 0, bank_coh3002c_w ); /* bankswitch */
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fb80000, 0x1fbfffff, 0, 0, MRA32_BANK3 );     /* country rom */
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb60000, 0x1fb60003, 0, 0, zn_qsound_w );

	zn_driver_init();
}

MACHINE_RESET( coh3002c )
{
	memory_set_bankptr( 1, memory_region( REGION_USER2 ) ); /* fixed game rom */
	memory_set_bankptr( 2, memory_region( REGION_USER2 ) + 0x400000 ); /* banked game rom */
	memory_set_bankptr( 3, memory_region( REGION_USER3 ) ); /* country rom */
	zn_machine_init();
}

static MACHINE_DRIVER_START( coh3002c )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( Z80, 8000000 )
	/* audio CPU */  /* 8MHz ?? */
	MDRV_CPU_PROGRAM_MAP( qsound_readmem, qsound_writemem )
	MDRV_CPU_IO_MAP( qsound_readport, 0 )
	MDRV_CPU_VBLANK_INT( qsound_interrupt, 4 ) /* 4 interrupts per frame ?? */

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( coh3002c )
	MDRV_NVRAM_HANDLER( zn_eeprom )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 512-1, 0, 240-1 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)

	MDRV_SOUND_ADD( QSOUND, QSOUND_CLOCK )
	MDRV_SOUND_CONFIG( qsound_interface )
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

/*

Main board (Standard ZN1 Main Board with Taito ZN1 BIOS)
ZN-1 1-659-709-12  COH-1000T
|--------------------------------------------------------|
|  LA4705             |---------------------------|      |
|                     |---------------------------|      |
|    AKM_AK4310VM      AT28C16                           |
|  VOL                                                   |
|       SW1            BIOS                              |
|  TD62064                                               |
|  TD62064        PALCE16V8                              |
|J                                                       |
|                                                        |
|A                EPM7032    814260    CXD2922CQ         |
|                                                        |
|M                                                       |
|                                                        |
|M                                                       |
|                *                                       |
|A           DSW                                         |
|                                CXD8561Q    CXD8530CQ   |
|                KM4132G271Q-12                          |
|CN505  CN506                   53.693MHz    67.737MHz   |
|            CAT702                                      |
|                                                        |
|CN504  CN503                                            |
|                                                        |
|                                                        |
|  NEC_78081G503         KM48V514BJ-12   KM48V514BJ-12   |
|            MC44200FT   KM48V514BJ-12   KM48V514BJ-12   |
|CN651  CN652            KM48V514BJ-12   KM48V514BJ-12   |
|                CN654   KM48V514BJ-12   KM48V514BJ-12   |
|--------------------------------------------------------|
Notes:
      CN506 - Connector for optional 3rd player controls
      CN505 - Connector for optional 4th player controls
      CN503 - Connector for optional 15kHz external video output (R,G,B,Sync, GND)
      CN504 - Connector for optional 2nd speaker (for stereo output)
      CN652 - Connector for optional trackball
      CN651 - Connector for optional analog controls
      CN654 - Connector for optional memory card
      SW1   - Slide switch for stereo or mono sound output
      DSW   - Dip switch (4 position, defaults all OFF)

      BIOS           - COH1000T.353, Taito ZN1 BIOS, 4MBit MaskROM type M534002 (SOP40)
      AT28C16        - Atmel AT28C16 2K x8 EEPROM (SOP24)
      814260-70      - 256K x16 (4MBit) DRAM (SOJ40)
      KM4132G271Q-12 - 128K x32 x2 banks (32MBit) SGRAM (QFP100)
      *              - Unpopulated position for KM4132G271Q-12 SGRAM
      KM48V514BJ-6   - 512k x8 (4MBit) DRAM (SOJ28)
      EPM7032        - Altera EPM7032QC44-15 CPLD labelled 'ZN1A' (QFP44)
      CAT702         - Protection chip labelled 'TT01' (DIP20)
      PALCE16V8      - PAL, labelled 'ZN1A' (PLCC20)


Game board
----------

SROM PCB-A
K11X0643A PSYCHIC FORCE
M43J0308A 238102270
|--------------------------------------------|
|   |---------------------------|            |
|   |---------------------------|            |
|                                    IC1     |
|              CAT702    E22-05.2            |
|    E18-06                                  |
|                        E22-10.7            |
|                                    IC6     |
| MB3773                                     |
|                                            |
|                                            |
|              |---------|           IC12    |
| E22-01.15    |TAITO    |                   |
|              |TC0140SYT|                   |
|              |         |                   |
|              |         |          E22-02.16|
|              |---------|                   |
|  IC20                     16MHz            |
|                                   E22-03.19|
|                    84C000AM-6              |
|                                            |
|                                   E22-04.21|
|TL074                                       |
|                    LH5268AN-10LL           |
|    MB87078                         IC27    |
|                         E22-07.22          |
|TL074           YM2610                      |
|      Y3016                                 |
|--------------------------------------------|
Notes:
      IC1, IC6     \
      IC12, IC20   |   - Unpopulated positions for 16MBit SOP44 MaskROM
      IC27         /
      E22-01, E22-02 \
      E22-03, E22-04 / - 23C16000 16MBit SOP44 MaskROMs
      E22-05, E22-10   - 27C040 4MBit DIP32 EPROM
      E22-07           - 27C010 1MBit DIP32 EPROM
      E18-06           - AMD MACH111 CPLD stamped 'E18-06' (PLCC44)
      LH5268AN-10LL    - Sharp 8K x8 SRAM (SOP28)
      CAT702           - Protection chip labelled 'TT03' (DIP20)
      MB3773           - Power Supply Monitor with Watch Dog Timer (i.e. Reset IC)
      MB87078          - Electronic Volume Control IC
      84C000AM-6       - Z80-A; clock 4.000MHz
      YM2610           - clock 8.000MHz

Main board (Standard ZN1 Main Board with Taito ZN1 BIOS)
ZN-1 1-659-709-12  COH-1000T
K11X0831A RAY STORM
|--------------------------------------------------------|
|  LA4705             |---------------------------|      |
|                     |---------------------------|      |
|    AKM_AK4310VM      AT28C16                           |
|  VOL                                                   |
|       SW1            BIOS                              |
|  TD62064                                               |
|  TD62064        PALCE16V8                              |
|J                                                       |
|                                                        |
|A                EPM7032    814260    CXD2922CQ         |
|                                                        |
|M                                                       |
|                                                        |
|M                                                       |
|                *                                       |
|A           DSW                                         |
|                                CXD8561Q    CXD8530CQ   |
|                KM4132G271Q-12                          |
|CN505  CN506                   53.693MHz    67.737MHz   |
|            CAT702                                      |
|                                                        |
|CN504  CN503                                            |
|                                                        |
|                                                        |
|  NEC_78081G503         KM48V514BJ-12   KM48V514BJ-12   |
|            MC44200FT   KM48V514BJ-12   KM48V514BJ-12   |
|CN651  CN652            KM48V514BJ-12   KM48V514BJ-12   |
|                CN654   KM48V514BJ-12   KM48V514BJ-12   |
|--------------------------------------------------------|
Notes:
      CN506 - Connector for optional 3rd player controls
      CN505 - Connector for optional 4th player controls
      CN503 - Connector for optional 15kHz external video output (R,G,B,Sync, GND)
      CN504 - Connector for optional 2nd speaker (for stereo output)
      CN652 - Connector for optional trackball
      CN651 - Connector for optional analog controls
      CN654 - Connector for optional memory card
      SW1   - Slide switch for stereo or mono sound output
      DSW   - Dip switch (4 position, defaults all OFF)

      BIOS           - COH1000T.353, Taito ZN1 BIOS, 4MBit MaskROM type M534002 (SOP40)
      AT28C16        - Atmel AT28C16 2K x8 EEPROM (SOP24)
      814260-70      - 256K x16 (4MBit) DRAM (SOJ40)
      KM4132G271Q-12 - 128K x32 x2 banks (32MBit) SGRAM (QFP100)
      *              - Unpopulated position for KM4132G271Q-12 SGRAM
      KM48V514BJ-6   - 512k x8 (4MBit) DRAM (SOJ28)
      EPM7032        - Altera EPM7032QC44-15 CPLD labelled 'ZN1A' (QFP44)
      CAT702         - Protection chip labelled 'TT01' (DIP20)
      PALCE16V8      - PAL, labelled 'ZN1A' (PLCC20)


Game board

ZROM PCB
K91J0636A RAY STORM
M43J0311A 241103582
|--------------------------------------------|
|   |---------------------------|            |
|   |---------------------------|            |
|              CAT702                        |
|                         E24-06.3           |
|              E24-01                        |
|                         E24-05.4   E24-02.1|
|                                            |
|  25MHz                                     |
|                         LH52B256   E24-03.2|
|      MB3773                                |
|                    MN1020012A              |
|                                    IC12    |
|        MB87078                             |
|                                            |
| NJM2100                            IC13    |
|                                            |
| NJM2100                                    |
|                                    IC20    |
|              ZSG-2        E24-09.14        |
|                                            |
|                                    IC21    |
| TMS57002DPHA                               |
|                           M66220FP         |
|                                    IC25    |
|              IC28   E24-04.27              |
| LC321664                                   |
|                            FM1208S         |
|--------------------------------------------|
Notes:
      IC12, IC13 \
      IC20, IC21  |  - Unpopulated positions for 16MBit SOP44 MaskROM
      IC25       /
      IC28           - Unpopulated position for 32MBit SOP44 MaskROM
      E24-04         - TC5332201 32MBit SOP44 MaskROM
      E24-02, E24-03 - 23C16000 16MBit SOP44 MaskROM
      E24-06, E24-05 - M27C4001 4MBit DIP32 EPROM
      E24-09         - M27C4001 4MBit DIP32 EPROM
      E24-01         - AMD MACH111 CPLD stamped 'E24-01' (PLCC44)
      LH52B256       - Sharp 32K x8 SRAM (SOP28)
      LC321664       - Sanyo 64K x16 EDO DRAM (SOP40)
      MN1020012A     - Panasonic MN1020012A Sound CPU (QFP128)
      ZSG-2          - Zoom Corp ZSG-2 sound DSP (QFP100)
      TMS57002DPHA   - Texas Instruments TMS57002DPHA sound DSP (QFP80)
      M66220FP       - 256 x8bit Mail-Box Inter-MPU data transfer
      CAT702         - Protection chip labelled 'TT04' (DIP20)
      MB3773         - Power Supply Monitor with Watch Dog Timer (i.e. Reset IC)
      MB87078        - Electronic Volume Control IC
      FM1208S        - RAMTRON 4096bit Nonvolatile Ferroelectric RAM (512w x 8b)
*/

static size_t taitofx1_eeprom_size1 = 0;
static UINT8 *taitofx1_eeprom1 = NULL;
static size_t taitofx1_eeprom_size2 = 0;
static UINT8 *taitofx1_eeprom2 = NULL;

static WRITE32_HANDLER( bank_coh1000t_w )
{
	mb3773_set_ck( ( data & 0x20 ) >> 5 );
	verboselog( 1, "bank_coh1000t_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
	memory_set_bankptr( 1, memory_region( REGION_USER2 ) + ( ( data & 3 ) * 0x800000 ) );
}

INTERRUPT_GEN( coh1000t_vblank )
{
	/* kludge: stop dropping into test mode on bootup */
	if( strncmp( Machine->gamedrv->name, "raystorm", 8 ) == 0 )
	{
		if( g_p_n_psxram[ 0x1b358 / 4 ] == 0x34020001 )
		{
			psxcpu_ram_written( &g_p_n_psxram[ 0x1b358 / 4 ], 4 );
			g_p_n_psxram[ 0x1b358 / 4 ] = 0x34020000;
		}
	}
	if(strcmp( Machine->gamedrv->name, "gdariusj" ) == 0 )
	{
		if (psxreadbyte(0x165d53) == 0)
		{
			psxwritebyte(0x165d53, 1);
		}
	}
	if(strcmp( Machine->gamedrv->name, "gdariusu" ) == 0 )
	{
		if (psxreadbyte(0x165dfb) == 0)
		{
			psxwritebyte(0x165dfb, 1);
		}
	}
	if(strcmp( Machine->gamedrv->name, "gdarius2" ) == 0 )
	{
		if (psxreadbyte(0x16be3b) == 0)
		{
			psxwritebyte(0x16be3b, 1);
		}
	}
	psx_vblank();
}

static unsigned fx1a_rom_banks;

static WRITE32_HANDLER( fx1a_rom_bankswitch_w )
{
	if( ACCESSING_LSB32 )
	{
		unsigned bank = data & 3;
		mb3773_set_ck( ( data & 0x20 ) >> 5 );
		if( bank < fx1a_rom_banks )
			memory_set_bank( 1, bank );
	}
}

static WRITE8_HANDLER( fx1a_sound_bankswitch_w )
{
	memory_set_bank( 10, data & 7 );
}

static READ32_HANDLER( taitofx1a_ymsound_r )
{
	if( ( mem_mask & 0x00ff0000 ) == 0 )
		return taitosound_comm_r( 0 ) << 16;
	return 0;
}

static WRITE32_HANDLER( taitofx1a_ymsound_w )
{
	if( ACCESSING_LSB32 )
		taitosound_port_w( 0, data & 0xff );
	if( ( mem_mask & 0x00ff0000 ) == 0 )
		taitosound_comm_w( 0, ( data >> 16 ) & 0xff );
}

DRIVER_INIT( coh1000ta )
{
	taitofx1_eeprom_size1 = 0x200; taitofx1_eeprom1 = auto_malloc( taitofx1_eeprom_size1 );

	fx1a_rom_banks = memory_region_length( REGION_USER2 ) / 0x800000;
	if( fx1a_rom_banks )
	{
		memory_install_read32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1f000000, 0x1f7fffff, 0, 0, MRA32_BANK1 );
		memory_configure_bank( 1, 0, fx1a_rom_banks, memory_region( REGION_USER2 ), 0x800000 );
		memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb40000, 0x1fb40003, 0, 0, fx1a_rom_bankswitch_w );

		memory_configure_bank( 10, 0, 1, memory_region( REGION_CPU2 ), 0x4000 );
		memory_configure_bank( 10, 1, 7, memory_region( REGION_CPU2 ) + 0x10000, 0x4000 );
	}
	taitosound_state_register();
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fb80000, 0x1fb80003, 0, 0, taitofx1a_ymsound_r );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb80000, 0x1fb80003, 0, 0, taitofx1a_ymsound_w );
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fbe0000, 0x1fbe0000 + ( taitofx1_eeprom_size1 - 1 ), 0, 0, MRA32_BANK2 );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fbe0000, 0x1fbe0000 + ( taitofx1_eeprom_size1 - 1 ), 0, 0, MWA32_BANK2 );

	zn_driver_init();
	mb3773_init();
}

MACHINE_RESET( coh1000ta )
{
	if( fx1a_rom_banks )
	{
		memory_set_bank( 1, 0 );
		memory_set_bank( 10, 1 );
	}
	memory_set_bankptr( 2, taitofx1_eeprom1 );
	taitosound_reset();
	zn_machine_init();
}

static NVRAM_HANDLER( coh1000ta )
{
	nvram_handler_at28c16_0( file, read_or_write );
	if (read_or_write)
	{
		mame_fwrite(file, taitofx1_eeprom1, taitofx1_eeprom_size1);
	}
	else if (file)
	{
		mame_fread(file, taitofx1_eeprom1, taitofx1_eeprom_size1);
	}
	else
	{
		memset(taitofx1_eeprom1, 0, taitofx1_eeprom_size1);
	}
}

static ADDRESS_MAP_START( fx1a_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK10)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_READ(YM2610_status_port_0_A_r)
	AM_RANGE(0xe001, 0xe001) AM_READ(YM2610_read_port_0_r)
	AM_RANGE(0xe002, 0xe002) AM_READ(YM2610_status_port_0_B_r)
	AM_RANGE(0xe200, 0xe200) AM_READ(MRA8_NOP)
	AM_RANGE(0xe201, 0xe201) AM_READ(taitosound_slave_comm_r)
	AM_RANGE(0xea00, 0xea00) AM_READ(MRA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( fx1a_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(YM2610_control_port_0_A_w)
	AM_RANGE(0xe001, 0xe001) AM_WRITE(YM2610_data_port_0_A_w)
	AM_RANGE(0xe002, 0xe002) AM_WRITE(YM2610_control_port_0_B_w)
	AM_RANGE(0xe003, 0xe003) AM_WRITE(YM2610_data_port_0_B_w)
	AM_RANGE(0xe200, 0xe200) AM_WRITE(taitosound_slave_port_w)
	AM_RANGE(0xe201, 0xe201) AM_WRITE(taitosound_slave_comm_w)
	AM_RANGE(0xe400, 0xe403) AM_WRITE(MWA8_NOP) /* pan */
	AM_RANGE(0xee00, 0xee00) AM_WRITE(MWA8_NOP) /* ? */
	AM_RANGE(0xf000, 0xf000) AM_WRITE(MWA8_NOP) /* ? */
	AM_RANGE(0xf200, 0xf200) AM_WRITE(fx1a_sound_bankswitch_w)
ADDRESS_MAP_END

/* handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irq_handler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	irq_handler,
	REGION_SOUND1,	/* Delta-T */
	REGION_SOUND1	/* ADPCM */
};

static MACHINE_DRIVER_START( coh1000ta )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( Z80, 16000000 / 4 )
	/* audio CPU */	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP( fx1a_sound_readmem, fx1a_sound_writemem )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( coh1000ta )
	MDRV_NVRAM_HANDLER( coh1000ta )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 512 )
	MDRV_VISIBLE_AREA( 0, 320-1, 0, 240-1 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)

	MDRV_SOUND_ADD(YM2610B, 16000000/2)
	MDRV_SOUND_CONFIG(ym2610_interface)
	MDRV_SOUND_ROUTE(0, "left",  0.25)
	MDRV_SOUND_ROUTE(0, "right", 0.25)
	MDRV_SOUND_ROUTE(1, "left",  1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
MACHINE_DRIVER_END

static WRITE32_HANDLER( taitofx1b_volume_w )
{
	verboselog( 1, "taitofx1_volume_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
}

static WRITE32_HANDLER( taitofx1b_sound_w )
{
	verboselog( 1, "taitofx1_sound_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
}

static READ32_HANDLER( taitofx1b_sound_r )
{
	UINT32 data = 0; // bit 0 = busy?
	verboselog( 1, "taitofx1_sound_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

DRIVER_INIT( coh1000tb )
{
	taitofx1_eeprom_size1 = 0x400; taitofx1_eeprom1 = auto_malloc( taitofx1_eeprom_size1 );
	taitofx1_eeprom_size2 = 0x200; taitofx1_eeprom2 = auto_malloc( taitofx1_eeprom_size2 );

	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f000000, 0x1f7fffff, 0, 0, MRA32_BANK1 ); /* banked game rom */
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fb00000, 0x1fb00000 + ( taitofx1_eeprom_size1 - 1 ), 0, 0, MRA32_BANK2 );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb00000, 0x1fb00000 + ( taitofx1_eeprom_size1 - 1 ), 0, 0, MWA32_BANK2 );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb40000, 0x1fb40003, 0, 0, bank_coh1000t_w ); /* bankswitch */
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb80000, 0x1fb80003, 0, 0, taitofx1b_volume_w );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fba0000, 0x1fba0003, 0, 0, taitofx1b_sound_w );
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fbc0000, 0x1fbc0003, 0, 0, taitofx1b_sound_r );
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fbe0000, 0x1fbe0000 + ( taitofx1_eeprom_size2 - 1 ), 0, 0, MRA32_BANK3 );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fbe0000, 0x1fbe0000 + ( taitofx1_eeprom_size2 - 1 ), 0, 0, MWA32_BANK3 );

	zn_driver_init();
	mb3773_init();
}

MACHINE_RESET( coh1000tb )
{
	memory_set_bankptr( 1, memory_region( REGION_USER2 ) ); /* banked game rom */
	memory_set_bankptr( 2, taitofx1_eeprom1 );
	memory_set_bankptr( 3, taitofx1_eeprom2 );
	zn_machine_init();
}

static NVRAM_HANDLER( coh1000tb )
{
	nvram_handler_at28c16_0( file, read_or_write );
	if (read_or_write)
	{
		mame_fwrite(file, taitofx1_eeprom1, taitofx1_eeprom_size1);
		mame_fwrite(file, taitofx1_eeprom2, taitofx1_eeprom_size2);
	}
	else if (file)
	{
		mame_fread(file, taitofx1_eeprom1, taitofx1_eeprom_size1);
		mame_fread(file, taitofx1_eeprom2, taitofx1_eeprom_size2);
	}
	else
	{
		memset(taitofx1_eeprom1, 0xff, taitofx1_eeprom_size1);
		memset(taitofx1_eeprom2, 0xff, taitofx1_eeprom_size2);
	}
}

static MACHINE_DRIVER_START( coh1000tb )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( coh1000t_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( coh1000tb )
	MDRV_NVRAM_HANDLER( coh1000tb )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 320-1, 0, 240-1 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)
MACHINE_DRIVER_END

/*

Primal Rage 2
Atari, 1996

This game runs on Sony ZN1 hardware with a custom Atari top board.


PCB Layout
----------

Main board (Standard ZN1 Main Board with Atari BIOS)

ZN-1 1-659-709-12  COH-1000W
|--------------------------------------------------------|
|  LA4705             |---------------------------|      |
|                     |---------------------------|      |
|    AKM_AK4310VM      AT28C16                           |
|  VOL                                                   |
|       SW1            BIOS                              |
|  TD62064                                               |
|  TD62064        PALCE16V8                              |
|J                                                       |
|                                                        |
|A                EPM7032    814260    CXD2922CQ         |
|                                                        |
|M                                                       |
|                                                        |
|M                                                       |
|                KM4132G271Q-12                          |
|A           DSW                                         |
|                                CXD8561Q    CXD8530CQ   |
|                KM4132G271Q-12                          |
|CN505  CN506                   53.693MHz    67.737MHz   |
|            CAT702                                      |
|                                                        |
|CN504  CN503                                            |
|                                                        |
|                                                        |
|  NEC_78081G503         KM48V2104AT-6   KM48V2104AT-6   |
|            MC44200FT   KM48V2104AT-6   KM48V2104AT-6   |
|CN651  CN652                 *                *         |
|                CN654        *                *         |
|--------------------------------------------------------|
Notes:
      CN506 - Connector for optional 3rd player controls
      CN505 - Connector for optional 4th player controls
      CN503 - Connector for optional 15kHz external video output (R,G,B,Sync, GND)
      CN504 - Connector for optional 2nd speaker (for stereo output)
      CN652 - Connector for optional trackball
      CN651 - Connector for optional analog controls
      CN654 - Connector for optional memory card
      SW1   - Slide switch for stereo or mono sound output
      DSW   - Dip switch (4 position, defaults all OFF)

      BIOS           - COH1000W.353, Atari ZN1 BIOS, 4MBit MaskROM type M534002 (SOP40)
      AT28C16        - Atmel AT28C16 2K x8 EEPROM (SOP24)
      814260-70      - 256K x16 (4MBit) DRAM (SOJ40)
      KM4132G271Q-12 - 128K x32 x2 banks (32MBit) SGRAM (QFP100)
      KM48V2104AT-6  - Bank0: 2M x8 (16MBit) DRAM (SOP28).
                       * - Note Bank1 is empty.
      EPM7032        - Altera EPM7032QC44-15 CPLD labelled 'ZN1A' (QFP44)
      CAT702         - Protection chip labelled 'TW01' (DIP20)
      PALCE16V8      - PAL, labelled 'ZN1A' (PLCC20)


Game board

PSXTRA A055056-   055053-01 REV 1
Also printed on the board near the ROMs is....
"IM FEELING A LITTLE ANXIOUS, IF YOU KNOW WHAT I MEAN..."
|--------------------------------------|
|    |---------------------------|     |
|    |---------------------------|     |
|                                      |
|CAT702                           JGUN1|
|                                      |
|                                      |
|                           *1    JGUN2|
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|   DS1232S                       LED  |
|LED                                   |
|                                 IDE1 |
|                VT83C461              |
|                                      |
|   EPM7160ELC84                       |
|                                      |
|                                 IDE2 |
|                                      |
|                                      |
|*2                                    |
|PR2_036.U14   PR2_036.U16          NW1|
|      PR2_036.U15    PR2_036.U17   NW2|
|                            2631      |
|                            2631   NW3|
|--------------------------------------|
Notes:
      CAT702              - protection chip labelled 'TW02' (DIP20)
      JGUN1, JGUN2        - Connector for optional gun controllers
      ROMs U14 thru U17   - 27C040 EPROM
      DS1232S             - Dallas DS1232 (reset IC, SOIC16)
      VT83C461            - VIA VT83C461 (IDE Hard Drive controller, QFP100)
      EPM7160ELC84        - Altera MAX EPM7160ELC84-10 (PLCC84 CPLD, labelled 'PSX PiD 9-19-96 2FDA')
      IDE1, IDE2          - 40 pin IDE Hard Drive connector, using Quantum Fireball 1080AT 1GB IDE hard drive.
      NW1                 - 8 pin RJ45 network connector labelled "SERIN"
      NW2                 - 8 pin RJ45 network connector labelled "SEROUT
      NW3                 - 8 pin RJ45 network connector labelled "SLONET"
      2631                - ICPL2631 IC (DIP8 x2)
      *1                  - Unpopulated position for PLCC44 IC
      *2                  - Unpopulated DIP28 socket
*/

/* Atari COH-1000W: VT83C461 IDE, PSX IRQ 10 and DMA channel 5. */
static UINT16 atari_irqctrl;
static UINT16 atari_latch;
static UINT8 atari_ide_irq;

static void atari_ide_interrupt( int state )
{
	atari_ide_irq = state != CLEAR_LINE;
	if( atari_ide_irq && ( atari_irqctrl & 1 ) )
		psx_irq_set( 0x400 );
}

static struct ide_interface atari_ide_intf =
{
	atari_ide_interrupt
};

static READ32_HANDLER( atari_irqctrl_r )
{
	return atari_irqctrl;
}

static WRITE32_HANDLER( atari_irqctrl_w )
{
	UINT32 control = atari_irqctrl;
	COMBINE_DATA( &control );
	atari_irqctrl = control & 0xffff;
}

/* Reading the low half consumes a complete IDE dword and latches its high
   half; reading only the high half must not advance to another disk word. */
static READ32_HANDLER( atari_ide32_r )
{
	UINT32 result = 0xffffffff;
	if( offset == 0x1f0 / 4 )
	{
		if( ( mem_mask & 0x0000ffff ) == 0 )
		{
			UINT32 data = ide_controller32_0_r( 0x1f0 / 4, 0 );
			atari_latch = data >> 16;
			result = ( result & 0xffff0000 ) | ( data & 0xffff );
		}
		if( ( mem_mask & 0xffff0000 ) == 0 )
			result = ( result & 0x0000ffff ) | ( (UINT32)atari_latch << 16 );
	}
	return result;
}

static void atari_dma_read( UINT32 address, INT32 size )
{
	UINT8 *ram = (UINT8 *)g_p_n_psxram;
	UINT32 mask = g_n_psxramsize - 1;
	/* PSX DMA counts dwords; the IDE data bus supplies 16-bit words. */
	size *= 2;
	while( size > 0 )
	{
		UINT16 data = ide_controller32_0_r( 0x1f0 / 4, 0xffff0000 );
		*(UINT16 *)( ram + WORD_XOR_LE( address & mask ) ) = data;
		address += 2;
		--size;
	}
}

static void atari_dma_write( UINT32 address, INT32 size )
{
}

static READ32_HANDLER( atari_inputs_r )
{
	return readinputport( ( offset >> 6 ) & 3 ) | 0xffffff00;
}

static READ32_HANDLER( atari_inputs2_r )
{
	return readinputport( 4 + ( ( offset >> 6 ) & 1 ) ) | 0xffffff00;
}

static ADDRESS_MAP_START( atari_zn_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM	AM_SHARE(1) AM_BASE(&g_p_n_psxram) AM_SIZE(&g_n_psxramsize) /* ram */
	AM_RANGE(0x1f800000, 0x1f8003ff) AM_RAM /* scratchpad */
	AM_RANGE(0x1f801000, 0x1f80100f) AM_RAM /* ?? */
	AM_RANGE(0x1f801010, 0x1f801013) AM_NOP
	AM_RANGE(0x1f801014, 0x1f801017) AM_READWRITE(psx_spu_delay_r, psx_spu_delay_w)
	AM_RANGE(0x1f801018, 0x1f80101f) AM_NOP
	AM_RANGE(0x1f801020, 0x1f801023) AM_READWRITE(psx_com_delay_r, psx_com_delay_w)
	AM_RANGE(0x1f801024, 0x1f80102f) AM_NOP
	AM_RANGE(0x1f801040, 0x1f80105f) AM_READWRITE(psx_sio_r, psx_sio_w)
	AM_RANGE(0x1f801060, 0x1f80106f) AM_RAM
	AM_RANGE(0x1f801070, 0x1f801077) AM_READWRITE(psx_irq_r, psx_irq_w)
	AM_RANGE(0x1f801080, 0x1f8010ff) AM_READWRITE(psx_dma_r, psx_dma_w)
	AM_RANGE(0x1f801100, 0x1f80112f) AM_READWRITE(psx_counter_r, psx_counter_w)
	AM_RANGE(0x1f801810, 0x1f801817) AM_READWRITE(psx_gpu_r, psx_gpu_w)
	AM_RANGE(0x1f801820, 0x1f801827) AM_READWRITE(psx_mdec_r, psx_mdec_w)
	AM_RANGE(0x1f801c00, 0x1f801dff) AM_READWRITE(psx_spu_r, psx_spu_w)
	AM_RANGE(0x1f802020, 0x1f802033) AM_RAM /* ?? */
	AM_RANGE(0x1f802040, 0x1f802043) AM_WRITENOP
	AM_RANGE(0x1fa00000, 0x1fa003ff) AM_READ(atari_inputs_r)
	AM_RANGE(0x1fa10000, 0x1fa101ff) AM_READ(atari_inputs2_r)
	AM_RANGE(0x1fa10200, 0x1fa10203) AM_READ(boardconfig_r)
	AM_RANGE(0x1fa10300, 0x1fa10303) AM_READWRITE(znsecsel_r, znsecsel_w)
	AM_RANGE(0x1fa20000, 0x1fa20003) AM_WRITE(coin_w)
	AM_RANGE(0x1fa30000, 0x1fa30003) AM_NOP /* ?? */
	AM_RANGE(0x1fa40000, 0x1fa40003) AM_READNOP /* ?? */
	AM_RANGE(0x1fa60000, 0x1fa60003) AM_READNOP /* ?? */
	AM_RANGE(0x1faf0000, 0x1faf07ff) AM_READWRITE( at28c16_32le_0_r, at28c16_32le_0_w ) /* eeprom */
	AM_RANGE(0x1fb20000, 0x1fb20007) AM_READ(unknown_r)
	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0) /* bios */
	AM_RANGE(0x80000000, 0x807fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0x9fc00000, 0x9fc7ffff) AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xa0000000, 0xa07fffff) AM_RAM AM_SHARE(1) /* ram mirror */
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_WRITENOP AM_ROM AM_SHARE(2) /* bios mirror */
	AM_RANGE(0xfffe0130, 0xfffe0133) AM_WRITENOP
	AM_RANGE(0x1f000000, 0x1f1fffff) AM_READ(MRA32_BANK1)
	AM_RANGE(0x1f000000, 0x1f000003) AM_WRITENOP /* DS1232 watchdog */
	AM_RANGE(0x1f000004, 0x1f1fffff) AM_WRITE(MWA32_ROM)
	AM_RANGE(0x1f7e4000, 0x1f7e4fff) AM_READWRITE(ide_controller32_0_r, ide_controller32_0_w)
	AM_RANGE(0x1f7e8000, 0x1f7e8003) AM_READWRITE(atari_irqctrl_r, atari_irqctrl_w)
	AM_RANGE(0x1f7f4000, 0x1f7f4fff) AM_READ(atari_ide32_r) AM_WRITENOP
ADDRESS_MAP_END

DRIVER_INIT( coh1000w )
{
	zn_driver_init();
	ide_controller_init( 0, &atari_ide_intf );
	state_save_register_global( atari_irqctrl );
	state_save_register_global( atari_latch );
	state_save_register_global( atari_ide_irq );
}

MACHINE_RESET( coh1000w )
{
	memory_set_bankptr( 1, memory_region( REGION_USER2 ) );
	zn_machine_init();
	atari_irqctrl = 0;
	atari_latch = 0;
	atari_ide_irq = 0;
	ide_controller_reset( 0 );
	psx_dma_install_read_handler( 5, atari_dma_read );
	psx_dma_install_write_handler( 5, atari_dma_write );
}

static MACHINE_DRIVER_START( coh1000w )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( atari_zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( coh1000w )
	MDRV_NVRAM_HANDLER( zn_eeprom )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 512-1, 0, 240-1 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)
MACHINE_DRIVER_END

/*

Main board (Standard ZN1 Main Board with Raizing/8ing BIOS)

ZN-1 1-659-709-12  COH-1002E
|--------------------------------------------------------|
|  LA4705             |---------------------------|      |
|                     |---------------------------|      |
|    AKM_AK4310VM      AT28C16                           |
|  VOL                                                   |
|       SW1            BIOS                              |
|  TD62064                                               |
|  TD62064        PALCE16V8                              |
|J                                                       |
|                                                        |
|A                EPM7032    814260    CXD2925Q          |
|                                                        |
|M                                                       |
|                                                        |
|M                                                       |
|                KM4132G271Q-12                          |
|A           DSW                                         |
|                                CXD8561BQ   CXD8530CQ   |
|                KM4132G271Q-12                          |
|CN505  CN506                   53.693MHz    67.737MHz   |
|            CAT702                                      |
|                                                        |
|CN504  CN503                                            |
|                                                        |
|                                                        |
|  NEC_78081G503         KM48V514BJ-12   KM48V514BJ-12   |
|            MC44200FT   KM48V514BJ-12   KM48V514BJ-12   |
|CN651  CN652            KM48V514BJ-12   KM48V514BJ-12   |
|                CN654   KM48V514BJ-12   KM48V514BJ-12   |
|--------------------------------------------------------|
Notes:
      CN506 - Connector for optional 3rd player controls
      CN505 - Connector for optional 4th player controls
      CN503 - Connector for optional 15kHz external video output (R,G,B,Sync, GND)
      CN504 - Connector for optional 2nd speaker (for stereo output)
      CN652 - Connector for optional trackball
      CN651 - Connector for optional analog controls
      CN654 - Connector for optional memory card
      SW1   - Slide switch for stereo or mono sound output
      DSW   - Dip switch (4 position, defaults all OFF)

      BIOS           - COH1002E.353, Raizing/8ing ZN1 BIOS, 4MBit MaskROM type M53402CZ (SOP40)
      AT28C16        - Atmel AT28C16 2K x8 EEPROM (SOP24)
      814260-70      - 256K x16 (4MBit) DRAM (SOJ40)
      KM4132G271Q-12 - 128K x32 x2 banks (32MBit) SGRAM (QFP100)
      KM48V514BJ-6   - 512k x8 (4MBit) DRAM (SOJ28)
      EPM7032        - Altera EPM7032QC44-15 CPLD labelled 'ZN1A' (QFP44)
      CAT702         - Protection chip labelled 'ET01' (DIP20)
      PALCE16V8      - PAL, labelled 'ZN1A' (PLCC20)


Beastorizer Game board

RA9701 SUB
|--------------------------------------|
|    |---------------------------|     |
|    |---------------------------|     |
|NJM2060   RA-B.ROAR-1.217             |
|                    CAT702  GAL16V8B  |
|                                      |
|NJM2060                     B.ROAR.212|
|YAC513    RA-B.ROAR-2.216             |
|                                      |
|                                      |
|                            B.ROAR.214|
|          MAIN_IF2                    |
|                                      |
|                                      |
|                            B.ROAR.213|
|                                      |
|          SUB_IF2                     |
|                                      |
|16.93MHz                    B.ROAR.215|
|  YMF271-F                            |
|  (QFP128)                            |
|                                      |
|                            B.ROAR.042|
| RA-B.ROAR-3.326                      |
|                                      |
|                                      |
|  TMP68HC000N-16            B.ROAR.046|
|12MHz                 M628032  M628032|
|--------------------------------------|
Notes:
      CAT702              - protection chip labelled 'ET02' (DIP20)
      ROMs 217, 216 & 326 - surface mounted 32MBit MASK ROM (SOP44)
      ROMs 042 & 046      - 27C2001 EPROM
      ROMs 212 thru 215   - 27C4001 EPROM
      MAIN_IF2 & SUB_IF2  - AMD Mach211 CPLD (PLCC44)
      M628032             - 32K x8 SRAM, equivalent to 62256 SRAM (SOJ28)
      68000 clock         - 12MHz
      YMF271-F clock      - 16.93MHz


Brave Blade / Bloody Roar 2 Game board

PS9805
|--------------------------------------|
|    |---------------------------|     |
|    |---------------------------|     |
|4560                    CAT702        |
|          FLASH0.021    GAL16V8B      |
|                                      |
| YAC516   FLASH1.024                  |
|                       ROM-1A.028     |
|         *FLASH2.025                  |
|                                      |
|                                      |
|             MAIN_IF2  ROM-1B.029     |
|16.93MHz                              |
|       YMF271-F                       |
|12MHz  (QFP128)                       |
|                       ROM-2A.026     |
|             SUB_IF2                  |
|                                      |
|                                      |
|    ROM-3.336          ROM-2B.210     |
|                                      |
|TMP68HC000N-16                        |
|                                      |
|                       *MASK4A.027    |
|                                      |
|    BR2_U0412.412    N341256          |
|                     N341256          |
|    BR2_U049.049       *MASK4B.016    |
|--------------------------------------|
Notes:
      *                   - Unpopulated ROM positions.
      CAT702              - protection chip labelled 'MG11' (DIP20)
      ROM-x               - surface mounted 32MBit MASK ROM (SOP44)
      ROMs 412 & 049      - 27C040 EPROM
      MASK4A              - smt solder pads (unpopulated)
      MASK4B              - DIP42 socket (unpopulated)
      FLASHx              - surface mounted TSOP56 16MBit FlashROM type Sharp LH28F160S5T-L10
      MAIN_IF2 & SUB_IF2  - AMD Mach211 CPLD (PLCC44)
      N341256             - 32K x8 SRAM, equivalent to 62256 SRAM (SOJ28)
      68000 clock         - 12MHz
      YMF271-F clock      - 16.93MHz


*/

static WRITE32_HANDLER( coh1002e_bank_w )
{
	znsecsel_w( offset, data, mem_mask );

	memory_set_bankptr( 1, memory_region( REGION_USER2 ) + ( ( data & 3 ) * 0x800000 ) );
}

static WRITE32_HANDLER( coh1002e_latch_w )
{
	if (offset)
	{
		cpunum_set_input_line(1, 2, HOLD_LINE);	// irq 2 on the 68k
	}
	else
	{
		soundlatch_w(0, data);
	}
}

DRIVER_INIT( coh1002e )
{
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f000000, 0x1f7fffff, 0, 0, MRA32_BANK1 );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fa10300, 0x1fa10303, 0, 0, coh1002e_bank_w );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb00000, 0x1fb00007, 0, 0, coh1002e_latch_w );

	zn_driver_init();
}

MACHINE_RESET( coh1002e )
{
	memory_set_bankptr( 1, memory_region( REGION_USER2 ) ); /* banked game rom */
	zn_machine_init();
}

static READ16_HANDLER( psarc_ymf_r )
{
	return YMF271_0_r(0);
}

static WRITE16_HANDLER( psarc_ymf_w )
{
	YMF271_0_w(offset, data);
}

static READ16_HANDLER( psarc_latch_r )
{
	return soundlatch_r(0);
}

static ADDRESS_MAP_START( psarc_snd_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x080000, 0x0fffff) AM_RAM
	AM_RANGE(0x100000, 0x10001f) AM_READWRITE( psarc_ymf_r, psarc_ymf_w )
	AM_RANGE(0x180008, 0x180009) AM_READ( psarc_latch_r )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITENOP
	AM_RANGE(0x100020, 0xffffff) AM_WRITENOP
ADDRESS_MAP_END

static struct YMF271interface ymf271_interface =
{
	REGION_SOUND1
};

static MACHINE_DRIVER_START( coh1002e )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( M68000, 12000000 )
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP( psarc_snd_map, 0 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( coh1002e )
	MDRV_NVRAM_HANDLER( zn_eeprom )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 320-1, 0, 240-1 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)

	MDRV_SOUND_ADD( YMF271, 0 )
	MDRV_SOUND_CONFIG( ymf271_interface )
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

/*

Judge Dread

light-gun type shooting game

Uses the Capcom/Sony  ZN-1 hardware with Rom board and
Hard disk Drive

U35 and U36 eproms are 27c1001 are believed to be the bios
data.

Disk Drive is a Quantum ????2.1 GB??

connectors CN506 and CN505 are the gun inputs pins 13, 14, 15
white/blue/ black respectively.   The + 5 (red) is seperate source (not
from the CN506 or Cn505).

You'll have to get the +5 for the guns from the jamma harness.


NBA Jam Extreme
Acclaim, 1996


Main board (Standard ZN1 Main Board with Acclaim ZN1 BIOS)
ZN-1 1-659-709-11  COH-1000A
|--------------------------------------------------------|
|  LA4705             |---------------------------|      |
|                     |---------------------------|      |
|    AKM_AK4310VM      AT28C16                           |
|  VOL                                                   |
|       SW1            BIOS                              |
|  TD62064                                               |
|  TD62064        PALCE16V8                              |
|J                                                       |
|                                                        |
|A                EPM7032   TC51V4260BJ-80   CXD2922CQ   |
|                                                        |
|M                                                       |
|                                                        |
|M                                                       |
|                KM4132G271Q-12                          |
|A           DSW                                         |
|                                CXD8561Q    CXD8530CQ   |
|                KM4132G271Q-12                          |
|CN505  CN506                   53.693MHz    67.737MHz   |
|            CAT702                                      |
|                                                        |
|CN504  CN503                                            |
|                                                        |
|                                                        |
|  NEC_78081G503         424805AL-A60    424805AL-A60    |
|            MC44200FT   424805AL-A60    424805AL-A60    |
|CN651  CN652            424805AL-A60    424805AL-A60    |
|                CN654   424805AL-A60    424805AL-A60    |
|--------------------------------------------------------|
Notes:
      CN506 - Connector for optional 3rd player controls
      CN505 - Connector for optional 4th player controls
      CN503 - Connector for optional 15kHz external video output (R,G,B,Sync, GND)
      CN504 - Connector for optional 2nd speaker (for stereo output)
      CN652 - Connector for optional trackball
      CN651 - Connector for optional analog controls
      CN654 - Connector for optional memory card
      SW1   - Slide switch for stereo or mono sound output
      DSW   - Dip switch (4 position, defaults all OFF)

      BIOS           - COH1000A.353, Acclaim ZN1 BIOS, 4MBit MaskROM type M534002 (SOP40)
      AT28C16        - Atmel AT28C16 2K x8 EEPROM (SOP24)
      TC51V4260BJ-80 - 256K x16 (4MBit) DRAM (SOJ40)
      KM4132G271Q-12 - 128K x32 x2 banks (32MBit) SGRAM (QFP100)
      424805AL-A60   - 512k x8 (4MBit) DRAM (SOJ28)
      EPM7032        - Altera EPM7032QC44-15 CPLD labelled 'ZN1A' (QFP44)
      CAT702         - Protection chip labelled 'AC01' (DIP20)
      PALCE16V8      - PAL, labelled 'ZN1A' (PLCC20)


Game board

PCB-100102
|--------------------------------------|
|LED |---------------------------|     |
|    |---------------------------|     |
|                                      |
|71256                                 |
|       ADM619AR                   CN1 |
|                                      |
|  BATT_3V     A1425       CAT702      |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|NBA0E.U41 NBA0O.U28 NBA4E.U17 NBA4O.U3|
|                                      |
|                                      |
|NBA1E.U42 NBA1O.U29 NBA5E.U18 NBA5O.U4|
|                                      |
|                                      |
|NBA2E.U43 NBA2O.U30 NBA6E.U19 NBA6O.U5|
|                                      |
|                                      |
|NBA3E.U44 NBA3O.U31       U20       U6|
|                                      |
| 360-MP-A1_EVEN.U35       U21         |
|                                      |
|  360-MP-A1_ODD.U36       U22         |
|                                      |
|--------------------------------------|
Notes:
      CN1      - 40 pin IDC connector (for 40 pin flat cable joining game PCB to sound PCB)
      CAT702   - protection chip labelled 'AC02' (DIP20)
      71256    - 32K x8 SRAM
      BATT_3V  - 3 Volt coin battery (CR2032)
      ADM619AR - 900MHz RF Transceiver (SOIC16, compatible to AD6190)
      A1425    - Actel A1425A-2 PQ100C 9536 (FPGA, QFP100, labelled 'PD-11010A REV A 05/21 C/S EEC1')
      U35, U36 - 27C080 DIP32 EPROM
      U21, U22 - Unpopulated positions for DIP32 EPROM
      U6, U20  - Unpopulated position for SOP44 MaskROM
      U3 thru U6    \
      U17 thru U19  |  surface mounted 32MBit SOP44 MaskROM
      U28 thru U31  |
      U41 thru U44  /


Sound Board

PCB-100095
|---------------------------|
|  TDA7240A  TDA7240A       |
|                    LMC6484|
|                           |
|                    LMC6484|
|                           |
|CN1         AD1866  LMC6484|
|                           |
|                           |
|                  LED      |
|                           |
|                  LED      |
|              *        *   |
|                           |
|              *        *   |
|                           |
|           SND1.U49    *   |
| 16.67MHz                  |
|                           |
|           SND0.U48    *   |
| ADSP-2181                 |
|                           |
|             360-SND-A1.U52|
| 52258   MACH111           |
| 52258                     |
|---------------------------|
Notes:
      CN1       - 40 pin IDC connector (for 40 pin flat cable joining sound PCB to game PCB)
      AD1866    - Dual 16bit Audio DAC
      52258     - Sharp LH52258 32K x8 SRAM (SOJ28)
      TDA7240A  - 20W Bridge Amplifier
      LMC6484   - CMOS Quad Rail-to-Rail Input and Output Operational Amplifier
      MACH111   - AMD MACH111 CPLD (PLCC44, labelled '360 PLD-A1 CS=0794')
      ADSP-2181 - Analog Devices DSP (QFP128, 16Bit, 40 MIPs, 5V, 2 serial ports, 16Bit internal DMA
                  port, a byte DMA port, programmable timer, 80K on-chip memory configured as
                  16K words (24 Bit) RAM and 16K data (16Bit) RAM
      U48, U49  - 32MBit DIP42 MaskROM
      U52       - 27C040 DIP32 EPROM labelled '360-SND-A1 IC110345 CS = 7D5A'
      *         - Unpopulated DIP42 socket
*/

static void jdredd_ide_interrupt(int state)
{
	if (state)
	{
	 	psx_irq_set(0x400);
	}
}

static struct ide_interface jdredd_ide_intf =
{
	jdredd_ide_interrupt
};

static READ32_HANDLER( jdredd_idestat_r )
{
	return ide_controller_0_r(0x1f7);
}

static READ32_HANDLER( jdredd_ide_r)
{
	int reg = offset*2;
	int shift = 0;
	int ret;

	if (mem_mask == 0xff00ffff)
	{
		shift = 16;
		reg++;
	}
	else if (mem_mask == 0xffff0000)	// code sometimes reads shorts from the DATA register
	{
		if (reg != 0)
		{
			logerror("JDREDD IDE: read 16-bit from non-DATA register %d!\n", reg);
		}

		return (ide_controller_0_r(0x1f0) | (ide_controller_0_r(0x1f0)<<8));
	}

	ret = ide_controller_0_r(0x1f0 + reg) << shift;

	return ret;
}

static WRITE32_HANDLER( jdredd_ide_w )
{
	int reg = offset*2;

	if (mem_mask == 0xff00ffff)
	{
		data >>= 16;
		reg++;
	}

	ide_controller_0_w(0x1f0 + reg, data);
}

static size_t nbajamex_eeprom_size;
static UINT8 *nbajamex_eeprom;
static UINT32 nbajamex_curr_rombank[ 2 ];
static int nbajamex_bankmode;
static int nbajamex_entry[ 2 ];

/* the rom is banked in 16 pieces of 2MB, the first window is always visible, the second one
   is visible only in the bank mode 1 and replaces the backup ram */
static READ32_HANDLER( nbajamex_rom_r )
{
	const UINT8 *rom = memory_region( REGION_USER2 );
	UINT32 n_address = offset * 4;

	if( n_address < 0x200000 )
	{
		return *(const UINT32 *)( rom + nbajamex_entry[ 0 ] * 0x200000 + n_address );
	}
	if( nbajamex_bankmode == 0 )
	{
		if( n_address < 0x200000 + nbajamex_eeprom_size )
		{
			return *(UINT32 *)( nbajamex_eeprom + n_address - 0x200000 );
		}
	}
	else
	{
		return *(const UINT32 *)( rom + nbajamex_entry[ 1 ] * 0x200000 + n_address - 0x200000 );
	}
	return 0;
}

static WRITE32_HANDLER( nbajamex_rom_w )
{
	UINT32 n_address = offset * 4;

	if( n_address >= 0x200000 && n_address < 0x200000 + nbajamex_eeprom_size )
	{
		COMBINE_DATA( (UINT32 *)( nbajamex_eeprom + n_address - 0x200000 ) );
	}
}

static void nbajamex_bank_update( int n_offset )
{
	int n_base0 = ( ( nbajamex_curr_rombank[ 0 ] >> 4 ) & 1 ) | ( ( nbajamex_curr_rombank[ 0 ] & 7 ) << 1 );
	int n_base1 = ( ( ~nbajamex_curr_rombank[ 1 ] >> 4 ) & 1 ) | ( ( nbajamex_curr_rombank[ 1 ] & 7 ) << 1 );

	if( n_offset == 0 )
	{
		if( nbajamex_curr_rombank[ 0 ] == 0 )
		{
			nbajamex_bankmode = 0;
		}
		else
		{
			nbajamex_bankmode = 1;
			nbajamex_entry[ 1 ] = n_base1;
		}
		nbajamex_entry[ 0 ] = n_base0;
	}
	else if( nbajamex_curr_rombank[ 0 ] != 0 )
	{
		nbajamex_bankmode = 1;
		nbajamex_entry[ 1 ] = n_base1;
	}
}

static WRITE32_HANDLER( nbajamex_bank_w )
{
	verboselog( 1, "nbajamex_bank_w( %08x, %08x, %08x )\n", offset, data, mem_mask );

	if( ( mem_mask & 0x0000ffff ) != 0x0000ffff )
	{
		nbajamex_curr_rombank[ 0 ] = data & 0xffff;
		nbajamex_bank_update( 0 );
	}
	if( ( mem_mask & 0xffff0000 ) != 0xffff0000 )
	{
		nbajamex_curr_rombank[ 1 ] = ( data >> 16 ) & 0xffff;
		nbajamex_bank_update( 1 );
	}
}

static NVRAM_HANDLER( coh1000a )
{
	nvram_handler_zn_eeprom( file, read_or_write );
	if( nbajamex_eeprom != 0 )
	{
		if( read_or_write )
		{
			mame_fwrite( file, nbajamex_eeprom, nbajamex_eeprom_size );
		}
		else if( file )
		{
			mame_fread( file, nbajamex_eeprom, nbajamex_eeprom_size );
		}
		else
		{
			memset( nbajamex_eeprom, 0xff, nbajamex_eeprom_size );
		}
	}
}

static WRITE32_HANDLER( acpsx_00_w )
{
	verboselog( 0, "acpsx_00_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
}

static WRITE32_HANDLER( acpsx_10_w )
{
	verboselog( 0, "acpsx_10_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
}

static WRITE32_HANDLER( nbajamex_80_w )
{
	verboselog( 0, "nbajamex_80_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
	psx_irq_set(0x400);
}

static READ32_HANDLER( nbajamex_08_r )
{
	UINT32 data = 0x400; /* sound related */
	verboselog( 0, "nbajamex_08_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

static READ32_HANDLER( nbajamex_80_r )
{
	UINT32 data = 0xffffffff;
	verboselog( 0, "nbajamex_80_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

DRIVER_INIT( coh1000a )
{
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f000000, 0x1f1fffff, 0, 0, MRA32_BANK1 );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fbfff00, 0x1fbfff03, 0, 0, acpsx_00_w );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fbfff10, 0x1fbfff13, 0, 0, acpsx_10_w );

	if( strcmp( Machine->gamedrv->name, "nbajamex" ) == 0 ||
		strcmp( Machine->gamedrv->name, "nbajamexa" ) == 0 )
	{
		nbajamex_eeprom_size = 0x8000; nbajamex_eeprom = auto_malloc( nbajamex_eeprom_size );

		memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f000000, 0x1f7fffff, 0, 0, nbajamex_rom_r );
		memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1f000000, 0x1f7fffff, 0, 0, nbajamex_rom_w );
		memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fbfff00, 0x1fbfff03, 0, 0, nbajamex_bank_w );
		memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fbfff80, 0x1fbfff83, 0, 0, nbajamex_80_w );
		memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fbfff08, 0x1fbfff0b, 0, 0, nbajamex_08_r );
		memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fbfff80, 0x1fbfff83, 0, 0, nbajamex_80_r );
	}

	if( ( !strcmp( Machine->gamedrv->name, "jdredd" ) ) ||
		( !strcmp( Machine->gamedrv->name, "jdreddb" ) ) )
	{
		memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fbfff8c, 0x1fbfff8f, 0, 0, jdredd_idestat_r );
		memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fbfff8c, 0x1fbfff8f, 0, 0, MWA32_NOP );
		memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fbfff90, 0x1fbfff9f, 0, 0, jdredd_ide_r );
		memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fbfff90, 0x1fbfff9f, 0, 0, jdredd_ide_w );

		ide_controller_init( 0, &jdredd_ide_intf );
	}

	zn_driver_init();
}

MACHINE_RESET( coh1000a )
{
	memory_set_bankptr( 1, memory_region( REGION_USER2 ) ); /* fixed game rom */
	nbajamex_curr_rombank[ 0 ] = 0;
	nbajamex_curr_rombank[ 1 ] = 0;
	nbajamex_bankmode = 0;
	nbajamex_entry[ 0 ] = 0;
	nbajamex_entry[ 1 ] = 0;
	zn_machine_init();
	if( ( !strcmp( Machine->gamedrv->name, "jdredd" ) ) ||
		( !strcmp( Machine->gamedrv->name, "jdreddb" ) ) )
	{
		ide_controller_reset( 0 );
	}
}

static MACHINE_DRIVER_START( coh1000a )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( coh1000a )
	MDRV_NVRAM_HANDLER( coh1000a )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 512-1, 0, 240-1 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)
MACHINE_DRIVER_END

/*

Main board (Standard ZN1 Main Board with Atlus BIOS)

ZN-1 1-659-709-12  COH-1001L
|--------------------------------------------------------|
|  LA4705             |---------------------------|      |
|                     |---------------------------|      |
|    AKM_AK4310VM      AT28C16                           |
|  VOL                                                   |
|       SW1            BIOS                              |
|  TD62064                                               |
|  TD62064        PALCE16V8                              |
|J                                                       |
|                                                        |
|A                EPM7032     814260     CXD2925Q        |
|                                                        |
|M                                                       |
|                                                        |
|M                                                       |
|                                                        |
|A           DSW                                         |
|                                CXD8561Q    CXD8530CQ   |
|                MB81G83222-012                          |
|CN505  CN506                   53.693MHz    67.737MHz   |
|            CAT702                                      |
|                                                        |
|CN504  CN503                                            |
|                                                        |
|                                                        |
|  NEC_78081G503         KM48V514BJ-12   KM48V514BJ-12   |
|            MC44200FT   KM48V514BJ-12   KM48V514BJ-12   |
|CN651  CN652            KM48V514BJ-12   KM48V514BJ-12   |
|                CN654   KM48V514BJ-12   KM48V514BJ-12   |
|--------------------------------------------------------|
Notes:
      CN506 - Connector for optional 3rd player controls
      CN505 - Connector for optional 4th player controls
      CN503 - Connector for optional 15kHz external video output (R,G,B,Sync, GND)
      CN504 - Connector for optional 2nd speaker (for stereo output)
      CN652 - Connector for optional trackball
      CN651 - Connector for optional analog controls
      CN654 - Connector for optional memory card
      SW1   - Slide switch for stereo or mono sound output
      DSW   - Dip switch (4 position, defaults all OFF)

      BIOS           - coh1001l.353, Atlus ZN1 BIOS, 4MBit MaskROM type M534002 (SOP40)
      AT28C16        - Atmel AT28C16 2K x8 EEPROM
      814260         - 256K x16 (4MBit) DRAM
      MB81G83222-012 - 128K x32 x2 banks (32MBit) SGRAM
      KM48V514BJ-6   - 512k x8 (4MBit) DRAM
      EPM7032        - Altera EPM7032QC44-15 CPLD labelled 'ZN1A' (QFP44)
      CAT702         - Protection chip labelled 'AT01' (DIP20)
      PALCE16V8      - PAL, labelled 'ZN1A' (PLCC20)


Game board
----------

ATHG-01
|--------------------------------------|
|    |---------------------------|     |
|    |---------------------------|     |
|LM358  LM358  PAL(1) PAL(2)           |
|                                      |
|YAC513 LM358                          |
|                            62256     |
|              CAT702        62256     |
|                                      |
|                                      |
| 16.9344MHz                           |
| YMZ280B      ATHG-01B.18             |
|                                      |
|                       ATHG-02B.17    |
|                                      |
|                                      |
|              ATHG-03.22     10MHz    |
|                                      |
|ATHG-06.4134           ATHG-04.21     |
|                                      |
|     ATHG-05.4136            68000    |
|                                      |
|                                PAL(3)|
|                                      |
|ATHG-11.215  ATHG-09.210   ATHG-07.027|
|                                      |
|                                      |
|    ATHG-10.029    ATHG-08.028        |
|--------------------------------------|
Notes:
      PAL(1)  - labelled 'ROM1'
      PAL(2)  - labelled 'ROM2'
      PAL(3)  - labelled 'ROM3'
      CAT702  - Protection chip labelled 'AT02' (DIP20)
      62256   - 32K x8 SRAM

      ATHG-01B.18   - Main program (27C040 EPROM)
      ATHG-02B.17   /

      ATHG-03.22    - Sound program (27C010 EPROM)
      ATHG-04.21    /

      ATHG-05.4136  - Sound data (16MBit DIP42 MASKROM)
      ATHG-06.4134  /

      ATHG-07.027   - Graphics data (32MBit DIP42 MASKROM)
      ATHG-08.028   /
      ATHG-09.210   /
      ATHG-10.029   /
      ATHG-11.215   /

      68000 clock  - 10.000MHz
      YMZ280 clock - 16.9344MHz
      VSync        - 60Hz
*/

static WRITE32_HANDLER( coh1001l_bnk_w )
{
	memory_set_bankptr( 1, memory_region( REGION_USER2 ) + ( ( ( data >> 16 ) & 3 ) * 0x800000 ) );
}

DRIVER_INIT( coh1001l )
{
	memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f000000, 0x1f7fffff, 0, 0, MRA32_BANK1 ); /* banked rom */
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb00000, 0x1fb00003, 0, 0, coh1001l_bnk_w );

	zn_driver_init();
}

MACHINE_RESET( coh1001l )
{
	memory_set_bankptr( 1, memory_region( REGION_USER2 ) ); /* banked rom */
	zn_machine_init();
}

static MACHINE_DRIVER_START( coh1001l )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

//  MDRV_CPU_ADD( M68000, 10000000 )
//  /* audio CPU */
//  MDRV_CPU_PROGRAM_MAP( atlus_snd_map, 0 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( coh1001l )
	MDRV_NVRAM_HANDLER( zn_eeprom )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 320-1, 0, 240-1 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)

//  MDRV_SOUND_ADD( YMZ280B, ymz280b_intf )
MACHINE_DRIVER_END

/*

Sonic Wings Limited (JPN Ver.)
(c)1996 Video System

Board:  PS based (ZN-1,COH-1002V)
    VS34 (ROM board)

Key:    Mother    KN01
    ROM board KN02

*/

static WRITE32_HANDLER( coh1002v_bnk_w )
{
	UINT8 *region = memory_region( REGION_USER3 );
	if( region )
	{
		memory_set_bankptr( 2, region + ( ( data & 0xff ) * 0x100000 ) );
	}
}

DRIVER_INIT( coh1002v )
{
	if( memory_region( REGION_USER2 ) )
	{
		memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1f000000, 0x1f27ffff, 0, 0, MRA32_BANK1 );
	}
	if( memory_region( REGION_USER3 ) )
	{
		memory_install_read32_handler ( 0, ADDRESS_SPACE_PROGRAM, 0x1fb00000, 0x1fbfffff, 0, 0, MRA32_BANK2 );
		memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb00000, 0x1fb00003, 0, 0, coh1002v_bnk_w );
	}

	zn_driver_init();
}

MACHINE_RESET( coh1002v )
{
	if( memory_region( REGION_USER2 ) )
	{
		memory_set_bankptr( 1, memory_region( REGION_USER2 ) ); /* fixed game rom */
	}
	if( memory_region( REGION_USER3 ) )
	{
		memory_set_bankptr( 2, memory_region( REGION_USER3 ) ); /* banked rom */
	}
	zn_machine_init();
}

INTERRUPT_GEN( coh1002v_vblank )
{
	/* kludge: to stop dropping into test mode on bootup */
	if(strcmp( Machine->gamedrv->name, "aerofgts" ) == 0 )
	{
		if (psxreadbyte(0x0db4d6) == 0)
		{
			psxwritebyte(0x0db4d6, 1);
		}
		if (psxreadbyte(0x0db4d7) == 0)
		{
			psxwritebyte(0x0db4d7, 1);
		}
	}
	if(strcmp( Machine->gamedrv->name, "aerofgtst" ) == 0 )
	{
		if (psxreadbyte(0x0db49a) == 0)
		{
			psxwritebyte(0x0db49a, 1);
		}
		if (psxreadbyte(0x0db49b) == 0)
		{
			psxwritebyte(0x0db49b, 1);
		}
	}
	if(strcmp( Machine->gamedrv->name, "sncwgltd" ) == 0 )
	{
		if (psxreadbyte(0x0db422) == 0)
		{
			psxwritebyte(0x0db422, 1);
		}
		if (psxreadbyte(0x0db423) == 0)
		{
			psxwritebyte(0x0db423, 1);
		}
	}
	psx_vblank();
}

static MACHINE_DRIVER_START( coh1002v )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( coh1002v_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( coh1002v )
	MDRV_NVRAM_HANDLER( zn_eeprom )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 320-1, 0, 240-1 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)
MACHINE_DRIVER_END

/*

Main board (Standard ZN1 Main Board with Tecmo BIOS)

ZN-1 1-659-709-12  COH-1002M
|--------------------------------------------------------|
|  LA4705             |---------------------------|      |
|                     |---------------------------|      |
|    AKM_AK4310VM      AT28C16                           |
|  VOL                                                   |
|       SW1            BIOS                              |
|  TD62064                                               |
|  TD62064        PALCE16V8                              |
|J                                                       |
|                                                        |
|A                EPM7032   M5M44260     CXD2925Q        |
|                                                        |
|M                                                       |
|                                                        |
|M                                                       |
|                KM4132G271Q-12                          |
|A           DSW                                         |
|                                CXD8561Q    CXD8530CQ   |
|                KM4132G271Q-12                          |
|CN505  CN506                   53.693MHz    67.737MHz   |
|            CAT702                                      |
|                                                        |
|CN504  CN503                                            |
|                                                        |
|                                                        |
|  NEC_78081G503         KM48V514BJ-12   KM48V514BJ-12   |
|            MC44200FT   KM48V514BJ-12   KM48V514BJ-12   |
|CN651  CN652            KM48V514BJ-12   KM48V514BJ-12   |
|                CN654   KM48V514BJ-12   KM48V514BJ-12   |
|--------------------------------------------------------|
Notes:
      CN506 - Connector for optional 3rd player controls
      CN505 - Connector for optional 4th player controls
      CN503 - Connector for optional 15kHz external video output (R,G,B,Sync, GND)
      CN504 - Connector for optional 2nd speaker (for stereo output)
      CN652 - Connector for optional trackball
      CN651 - Connector for optional analog controls
      CN654 - Connector for optional memory card
      SW1   - Slide switch for stereo or mono sound output
      DSW   - Dip switch (4 position, defaults all OFF)

      BIOS           - COH1002M.353, Tecmo ZN1 BIOS, 4MBit MaskROM type M534002 (SOP40)
      AT28C16        - Atmel AT28C16 2K x8 EEPROM
      M5M44260       - 256K x16 (4MBit) DRAM
      KM4132G271Q-12 - 128K x32 x2 banks (32MBit) SGRAM
      KM48V514BJ-6   - 512k x8 (4MBit) DRAM
      EPM7032        - Altera EPM7032QC44-15 CPLD labelled 'ZN1A' (QFP44)
      CAT702         - Protection chip labelled 'MG01' (DIP20)
      PALCE16V8      - PAL, labelled 'ZN1A' (PLCC20)


Game board with sound

Tecmo TPS1-7
|--------------------------------------|
|    |---------------------------|     |
|    |---------------------------|     |
|LM358  GAL16V8B(1)  CAT702            |
|YAC513  LM358  CBAJ1.119  CBAJ2.120   |
|        LM358                         |
|                                      |
|                                      |
| CB-VO  CB-SE            CB-08  CB-07 |
|                                      |
|                                      |
|      CB-05     CB-03     CB-01       |
| CB-06     CB-04     CB-02     CB-00  |
|                                      |
|                                      |
|                                      |
|                                      |
|YMZ280B  16.9MHz  GAL16V8(2)  LH54020 |
|                  GAL16V8(3)          |
|                  GAL16V8(4)  LH54020 |
|                                      |
|                                      |
|                                      |
| 32MHz  GAL16V8(5)         GAL16V8(6) |
|                   D43001             |
|                                      |
|                                      |
|             CBAJZ80.3118     Z80     |
|--------------------------------------|
Notes:
      There are a few unpopulated positions on this game board, including
      4 unpopulated positions for 4x 32MBit smt SOP44 MASKROMs
      1 unpopulated position for uPD72103AG near the D43001 RAM
      2 unpopulated positions for 2 connectors near the Z80 ROM possibly for a network link?
      1 unpopulated position for a PAL16V8 near ROM 'CBAJ2'

      This board contains....
      PAL16V8B(1) labelled 'SOPROM1'
      PAL16V8B(2) labelled 'SOPROM3'
      PAL16V8B(3) labelled 'SOPROM4C'
      PAL16V8B(4) labelled 'SOPROM5B'
      PAL16V8B(5) labelled 'SOPROM6A'
      PAL16V8B(6) labelled 'SOPROM2B'
      CAT702 Protection chip labelled 'MG03' (DIP20)
      3 logic chips near main program ROMs.
      2x 4MBit EPROMs labelled 'CBAJ1' and 'CBAJ2'
      1x 2MBit EPROM labelled 'CBAJZ80'
      9x 32MBit smt SOP44 MASKROMs labelled 'CB-00' thru 'CB-08' (Graphics)
      2x 32MBit smt SOP44 MASKROMs labelled 'CB-SE' and 'CB-V0' (connected to the YMZ280B)
      LH540202 - CMOS 1024 x 9 Asyncronous FIFO (PLCC32)
      D43001   - 32K x8 SRAM, equivalent to 62256 SRAM

      Z80 clock: 4.000MHz
      VSync    : 60Hz

Game board without sound

Tecmo TPS1-7
|--------------------------------------|
|    |---------------------------|     |
|    |---------------------------|     |
|       GAL16V8B    CAT702             |
|              SHMJ-B.119  SHMJ-A.120  |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|  SH-03.220       SH-01.218           |
|         SH-02.219       SH-00.217    |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|                                      |
|--------------------------------------|
Notes:
      There are many unpopulated positions on this game board. This game only contains
      the following parts...
      PAL16V8B labelled 'SOPROM1'
      CAT702 Protection chip labelled 'MG04' (DIP20)
      3 logic chips
      2x 4MBit EPROMs labelled 'SHMJ-B' and 'SHMJ-A'
      4x 32MBit smt SOP44 MASKROMs labelled 'SH03, 'SH02', 'SH01' & 'SH00'. There is space
      for 11 more 32MBit smt SOP44 MASKROMs.
*/

#define TPS_ROM_BANK_SIZE 0x800000

static int coh1002m_rom_banks;

static WRITE32_HANDLER( coh1002m_bank_w )
{
	if( ( mem_mask & 0x00ff0000 ) == 0 )
	{
		unsigned bank = ( data >> 16 ) & 0xff;
		if( bank < coh1002m_rom_banks )
			memory_set_bank( 1, bank );
	}
}

static WRITE32_HANDLER( coh1002m_coin_w )
{
	if( ACCESSING_LSB32 )
	{
		coin_counter_w( 0, data & 0x01 );
		coin_counter_w( 2, data & 0x04 );
		coin_counter_w( 1, data & 0x10 );
		coin_counter_w( 3, data & 0x40 );
	}
}

#define CBAJ_FIFO_SIZE 0x400

static struct cbaj_fifo
{
	UINT8 data[CBAJ_FIFO_SIZE];
	UINT16 head;
	UINT16 count;
} cbaj_to_z80, cbaj_to_main;

static UINT8 cbaj_fifo_read( struct cbaj_fifo *fifo )
{
	UINT8 data;

	if( !fifo->count )
		return 0xff;

	data = fifo->data[fifo->head];
	fifo->head = (fifo->head + 1) & (CBAJ_FIFO_SIZE - 1);
	fifo->count--;
	return data;
}

static void cbaj_fifo_write( struct cbaj_fifo *fifo, UINT8 data )
{
	if( fifo->count < CBAJ_FIFO_SIZE )
	{
		fifo->data[(fifo->head + fifo->count) & (CBAJ_FIFO_SIZE - 1)] = data;
		fifo->count++;
	}
}

static READ32_HANDLER( cbaj_z80_r )
{
	UINT32 result = 0;

	if( ACCESSING_LSB32 )
		result = cbaj_fifo_read( &cbaj_to_main );
	if( ACCESSING_MSB32 && cbaj_to_main.count )
		result |= 0x02000000;
	return result;
}

static WRITE32_HANDLER( cbaj_z80_w )
{
	if( ACCESSING_LSB32 )
		cbaj_fifo_write( &cbaj_to_z80, data );
}

DRIVER_INIT( coh1002m )
{
	coh1002m_rom_banks = memory_region_length( REGION_USER2 ) / TPS_ROM_BANK_SIZE;
	if( coh1002m_rom_banks )
	{
		memory_install_read32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1f000000, 0x1f7fffff, 0, 0, MRA32_BANK1 );
		memory_configure_bank( 1, 0, coh1002m_rom_banks, memory_region( REGION_USER2 ), TPS_ROM_BANK_SIZE );
		memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb00004, 0x1fb00007, 0, 0, coh1002m_bank_w );
	}
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fa20000, 0x1fa20003, 0, 0, coh1002m_coin_w );
	zn_driver_init();
}

static DRIVER_INIT( cbaj )
{
	init_coh1002m();
	memory_install_read32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb00000, 0x1fb00003, 0, 0, cbaj_z80_r );
	memory_install_write32_handler( 0, ADDRESS_SPACE_PROGRAM, 0x1fb00000, 0x1fb00003, 0, 0, cbaj_z80_w );

	state_save_register_item_array( "cbaj", 0, cbaj_to_z80.data );
	state_save_register_item( "cbaj", 0, cbaj_to_z80.head );
	state_save_register_item( "cbaj", 0, cbaj_to_z80.count );
	state_save_register_item_array( "cbaj", 0, cbaj_to_main.data );
	state_save_register_item( "cbaj", 0, cbaj_to_main.head );
	state_save_register_item( "cbaj", 0, cbaj_to_main.count );
}

MACHINE_RESET( coh1002m )
{
	if( coh1002m_rom_banks )
		memory_set_bank( 1, 0 );
	zn_machine_init();
}

static MACHINE_RESET( coh1002msnd )
{
	cbaj_to_z80.head = cbaj_to_z80.count = 0;
	cbaj_to_main.head = cbaj_to_main.count = 0;
	machine_reset_coh1002m();
}

static READ8_HANDLER( cbaj_z80_latch_r )
{
	return cbaj_fifo_read( &cbaj_to_z80 );
}

static WRITE8_HANDLER( cbaj_z80_latch_w )
{
	cbaj_fifo_write( &cbaj_to_main, data );
}

static READ8_HANDLER( cbaj_z80_ready_r )
{
	return cbaj_to_z80.count ? 0x02 : 0x00;
}

ADDRESS_MAP_START( cbaj_z80_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x7fff ) AM_ROM
	AM_RANGE( 0x8000, 0xffff ) AM_RAM
ADDRESS_MAP_END

ADDRESS_MAP_START( cbaj_z80_port_map, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE( 0x84, 0x84 ) AM_READWRITE( YMZ280B_status_0_r, YMZ280B_register_0_w )
	AM_RANGE( 0x85, 0x85 ) AM_READWRITE( YMZ280B_status_0_r, YMZ280B_data_0_w )
	AM_RANGE( 0x90, 0x90 ) AM_READWRITE( cbaj_z80_latch_r, cbaj_z80_latch_w )
	AM_RANGE( 0x91, 0x91 ) AM_READ( cbaj_z80_ready_r )
ADDRESS_MAP_END

static struct YMZ280Binterface ymz280b_intf =
{
	REGION_SOUND1,
	0
};

static MACHINE_DRIVER_START( coh1002m )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_RESET( coh1002m )
	MDRV_NVRAM_HANDLER( zn_eeprom )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 320-1, 0, 240-1 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coh1002msnd )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_PROGRAM_MAP( zn_map, 0 )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD( Z80, 32000000/8 )
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP( cbaj_z80_map, 0 )
	MDRV_CPU_IO_MAP( cbaj_z80_port_map, 0 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_INTERLEAVE( 100 ) /* 6000 Hz, as on the MAME sound board configuration */
	MDRV_MACHINE_RESET( coh1002msnd )
	MDRV_NVRAM_HANDLER( zn_eeprom )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 320-1, 0, 240-1 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type2 )
	MDRV_VIDEO_UPDATE( psx )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD( PSXSPU, 0 )
	MDRV_SOUND_CONFIG( psxspu_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)
MACHINE_DRIVER_END

INPUT_PORTS_START( zn )
	PORT_START_TAG("P1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_8WAY
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_8WAY
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON2)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON3)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON4)

	PORT_START_TAG("P2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(2)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(2)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(2)

	PORT_START_TAG("SERVICE")
	PORT_SERVICE_NO_TOGGLE(0x01, IP_ACTIVE_LOW)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_SERVICE1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_TILT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED) /* gun mux */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON5)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON5) PORT_PLAYER(2)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON5) PORT_PLAYER(3)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON5) PORT_PLAYER(4)

	PORT_START_TAG("SYSTEM")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_START2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_START3)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_START4)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_COIN3)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_COIN4)

	PORT_START_TAG("P3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(3)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(3)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(3)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(3)

	PORT_START_TAG("P4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(4)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(4)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(4)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(4)

	PORT_START_TAG("DSW")
	PORT_DIPNAME(0x01, 0x01, DEF_STR(Unknown)) PORT_DIPLOCATION("S551:1")
	PORT_DIPSETTING(0x01, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
	PORT_DIPNAME(0x02, 0x02, DEF_STR(Service_Mode)) PORT_DIPLOCATION("S551:2") /* bios testmode */
	PORT_DIPSETTING(0x02, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
	PORT_DIPNAME(0x04, 0x04, DEF_STR(Unknown)) PORT_DIPLOCATION("S551:3")
	PORT_DIPSETTING(0x04, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
	PORT_DIPNAME(0x08, 0x08, DEF_STR(Unknown)) PORT_DIPLOCATION("S551:4")
	PORT_DIPSETTING(0x08, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
INPUT_PORTS_END

INPUT_PORTS_START( zn2p )
	PORT_INCLUDE(zn)

	PORT_MODIFY("P1")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED) /* P1 BUTTON4 */

	PORT_MODIFY("P2")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED) /* P2 BUTTON4 */

	PORT_MODIFY("SYSTEM")
	PORT_BIT(0xcc, IP_ACTIVE_LOW, IPT_UNUSED) /* START3/START4/COIN3/COIN4 */

	PORT_MODIFY("SERVICE")
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED) /* P1 BUTTON5/P2 BUTTON5/P3 BUTTON5/P4 BUTTON5 */

	PORT_MODIFY("P3")
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED) /* P3 UP/P3 DOWN/P3 LEFT/P3 RIGHT/P3 BUTTON1/P3 BUTTON2/P3 BUTTON3/P3 BUTTON4 */

	PORT_MODIFY("P4")
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED) /* P4 UP/P4 DOWN/P4 LEFT/P4 RIGHT/P4 BUTTON1/P4 BUTTON2/P4 BUTTON3/P4 BUTTON4 */
INPUT_PORTS_END

INPUT_PORTS_START( znt2p )
	PORT_INCLUDE(zn2p)

	PORT_MODIFY("DSW")
	PORT_DIPNAME(0x08, 0x08, "Test Mode") PORT_DIPLOCATION("S551:4")
	PORT_DIPSETTING(0x08, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
INPUT_PORTS_END

INPUT_PORTS_START( zn1p )
	PORT_INCLUDE(zn2p)

	PORT_MODIFY("SYSTEM")
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED) /* START2 */

	PORT_MODIFY("P2")
	PORT_BIT(0x7f, IP_ACTIVE_LOW, IPT_UNUSED) /* P2 UP/P2 DOWN/P2 LEFT/P2 RIGHT/P2 BUTTON1/P2 BUTTON2/P2 BUTTON3 */
INPUT_PORTS_END

INPUT_PORTS_START( aerofgts )
	PORT_INCLUDE(zn2p)

	PORT_MODIFY("P1")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED) /* P1 BUTTON3 */

	PORT_MODIFY("P2")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED) /* P2 BUTTON3 */

	PORT_MODIFY("DSW")
	PORT_DIPNAME(0x04, 0x04, "Test Mode") PORT_DIPLOCATION("S551:3")
	PORT_DIPSETTING(0x04, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
	PORT_DIPNAME(0x08, 0x08, "Save") PORT_DIPLOCATION("S551:4")
	PORT_DIPSETTING(0x08, DEF_STR(Yes)) /* OK */
	PORT_DIPSETTING(0x00, DEF_STR(No)) /* NG */
INPUT_PORTS_END

INPUT_PORTS_START( bldyror2 )
	PORT_INCLUDE(znt2p)

	PORT_MODIFY("P3")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON4) /* P3 BUTTON1 */
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(2) /* P3 BUTTON2 */
INPUT_PORTS_END

INPUT_PORTS_START( capcom6b )
	PORT_INCLUDE(zn2p)

	PORT_MODIFY("P3")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON4) /* P3 BUTTON1 */
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON5) /* P3 BUTTON2 */
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON6) /* P3 BUTTON3 */

	PORT_MODIFY("P4")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(2) /* P4 BUTTON1 */
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON5) PORT_PLAYER(2) /* P4 BUTTON2 */
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON6) PORT_PLAYER(2) /* P4 BUTTON3 */

	PORT_START_TAG("KICK1")
	PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_NAME("%p Kick Harness Button 4")
	PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_BUTTON5) PORT_NAME("%p Kick Harness Button 5")
	PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_BUTTON6) PORT_NAME("%p Kick Harness Button 6")
	PORT_BIT(0xfffffff8, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("KICK2")
	PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_NAME("%p Kick Harness Button 4") PORT_PLAYER(2)
	PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_BUTTON5) PORT_NAME("%p Kick Harness Button 5") PORT_PLAYER(2)
	PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_BUTTON6) PORT_NAME("%p Kick Harness Button 6") PORT_PLAYER(2)
	PORT_BIT(0xfffffff8, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

INPUT_PORTS_START( capcom4b )
	PORT_INCLUDE(capcom6b)

	PORT_MODIFY("P1")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED) /* P1 BUTTON3 */

	PORT_MODIFY("P2")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED) /* P2 BUTTON3 */

	PORT_MODIFY("P3")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON3) /* P3 BUTTON1 */
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON4) /* P3 BUTTON2 */
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED) /* P3 BUTTON3 */

	PORT_MODIFY("P4")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(2) /* P4 BUTTON1 */
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(2) /* P4 BUTTON2 */
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED) /* P4 BUTTON3 */

	PORT_MODIFY("KICK1")
	PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_NAME("%p Kick Harness Button 3") /* KICK1 BUTTON4 */
	PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_NAME("%p Kick Harness Button 4") /* KICK1 BUTTON5 */
	PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_UNUSED) /* KICK1 BUTTON6 */

	PORT_MODIFY("KICK2")
	PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_NAME("%p Kick Harness Button 3") PORT_PLAYER(2) /* KICK2 BUTTON4 */
	PORT_BIT(0x0002, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_NAME("%p Kick Harness Button 4") PORT_PLAYER(2) /* KICK2 BUTTON5 */
	PORT_BIT(0x0004, IP_ACTIVE_LOW, IPT_UNUSED) /* KICK2 BUTTON6 */
INPUT_PORTS_END

INPUT_PORTS_START( glpracr )
	PORT_INCLUDE(capcom4b)

	PORT_MODIFY("SYSTEM")
	PORT_BIT(0x22, IP_ACTIVE_LOW, IPT_UNUSED) /* START2/COIN2 */

	PORT_MODIFY("P2")
	PORT_BIT(0x3f, IP_ACTIVE_LOW, IPT_UNUSED) /* P2 UP/P2 DOWN/P2 LEFT/P2 RIGHT/P2 BUTTON1/P2 BUTTON2 */

	PORT_MODIFY("P3")
	PORT_BIT(0x30, IP_ACTIVE_LOW, IPT_UNUSED) /* P3 BUTTON1/P3 BUTTON2 */

	PORT_MODIFY("P4")
	PORT_BIT(0x30, IP_ACTIVE_LOW, IPT_UNUSED) /* P4 BUTTON1/P3 BUTTON2 */

	PORT_MODIFY("KICK1")
	PORT_BIT(0x0003, IP_ACTIVE_LOW, IPT_UNUSED) /* KICK1 BUTTON4/KICK1 BUTTON5 */

	PORT_MODIFY("KICK2")
	PORT_BIT(0x0003, IP_ACTIVE_LOW, IPT_UNUSED) /* KICK2 BUTTON4/KICK2 BUTTON5 */
INPUT_PORTS_END

INPUT_PORTS_START( glpracr2 )
	PORT_INCLUDE(zn2p)

	PORT_MODIFY("SYSTEM")
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED) /* COIN2 */

	PORT_MODIFY("P1")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED) /* P1 BUTTON3 */

	PORT_MODIFY("P2")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED) /* P2 BUTTON3 */
INPUT_PORTS_END

INPUT_PORTS_START( jdredd )
	PORT_INCLUDE(zn)

	PORT_MODIFY("P1")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON2) /* P1 BUTTON1 */
	PORT_BIT(0xef, IP_ACTIVE_LOW, IPT_UNUSED) /* P1 UP/P1 DOWN/P1 LEFT/P1 RIGHT/P1 BUTTON2/P1 BUTTON3/P1 BUTTON4 */

	PORT_MODIFY("P2")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(2) /* P2 BUTTON1 */
	PORT_BIT(0xef, IP_ACTIVE_LOW, IPT_UNUSED) /* P2 UP/P2 DOWN/P2 LEFT/P2 RIGHT/P2 BUTTON2/P2 BUTTON3/P2 BUTTON4 */

	PORT_MODIFY("SERVICE")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED) /* gun mux */
	PORT_BIT(0x30, IP_ACTIVE_LOW, IPT_UNUSED) /* P1 BUTTON5/P2 BUTTON5 */
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON1) /* P3 BUTTON5 */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2) /* P4 BUTTON5 */

	PORT_MODIFY("SYSTEM")
	PORT_BIT(0x0c, IP_ACTIVE_LOW, IPT_UNUSED) /* START3/START4 */

	PORT_MODIFY("P3")
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED) /* P3 UP/P3 DOWN/P3 LEFT/P3 RIGHT/P3 BUTTON1/P3 BUTTON2/P3 BUTTON3/P3 BUTTON4 */

	PORT_MODIFY("P4")
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED) /* P4 UP/P4 DOWN/P4 LEFT/P4 RIGHT/P4 BUTTON1/P4 BUTTON2/P4 BUTTON3/P4 BUTTON4 */
INPUT_PORTS_END

INPUT_PORTS_START( nbajamex )
	PORT_INCLUDE(zn)

	PORT_MODIFY("SERVICE")
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED) /* P1 BUTTON5/P2 BUTTON5/P3 BUTTON5/P4 BUTTON5 */
INPUT_PORTS_END

INPUT_PORTS_START( plsmaswd )
	PORT_INCLUDE(capcom6b)

	PORT_MODIFY("P3")
	PORT_BIT(0x60, IP_ACTIVE_LOW, IPT_UNUSED) /* P3 BUTTON2/P3 BUTTON3 */

	PORT_MODIFY("P4")
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED) /* P4 BUTTON2/P4 BUTTON3 */

	PORT_MODIFY("KICK1")
	PORT_BIT(0x0006, IP_ACTIVE_LOW, IPT_UNUSED) /* KICK1 BUTTON5/KICK1 BUTTON6 */

	PORT_MODIFY("KICK2")
	PORT_BIT(0x0006, IP_ACTIVE_LOW, IPT_UNUSED) /* KICK2 BUTTON5/KICK2 BUTTON6 */
INPUT_PORTS_END

INPUT_PORTS_START( primrag2 )
	PORT_INCLUDE(zn2p)

	PORT_MODIFY("P3")
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNUSED) /* P3 UP/P3 DOWN/P3 LEFT/P3 RIGHT/P3 BUTTON1/P3 BUTTON2/P3 BUTTON3/P3 BUTTON4 */

	PORT_MODIFY("P4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON4) /* P4 UP */
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_BUTTON5) /* P4 DOWN */
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_BUTTON6) /* P4 LEFT */
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(2) /* P4 BUTTON1 */
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON5) PORT_PLAYER(2) /* P4 BUTTON2 */
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON6) PORT_PLAYER(2) /* P4 BUTTON3 */
	PORT_BIT(0x88, IP_ACTIVE_LOW, IPT_UNUSED) /* P4 RIGHT/P4 BUTTON4 */
INPUT_PORTS_END

INPUT_PORTS_START( raystorm )
	PORT_INCLUDE(znt2p)

	PORT_MODIFY("P1")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED) /* P1 BUTTON3 */

	PORT_MODIFY("P2")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED) /* P2 BUTTON3 */
INPUT_PORTS_END

INPUT_PORTS_START( strider2 )
	PORT_INCLUDE(capcom6b)

	PORT_MODIFY("SYSTEM")
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED) /* START2 */

	PORT_MODIFY("P2")
	PORT_BIT(0x7f, IP_ACTIVE_LOW, IPT_UNUSED) /* P2 UP/P2 DOWN/P2 LEFT/P2 RIGHT/P2 BUTTON1/P2 BUTTON2/P2 BUTTON3 */

	PORT_MODIFY("P3")
	PORT_BIT(0x70, IP_ACTIVE_LOW, IPT_UNUSED) /* P3 BUTTON1/P3 BUTTON2/P3 BUTTON3 */

	PORT_MODIFY("P4")
	PORT_BIT(0x70, IP_ACTIVE_LOW, IPT_UNUSED) /* P4 BUTTON1/P4 BUTTON2/P4 BUTTON3 */

	PORT_MODIFY("KICK1")
	PORT_BIT(0x0007, IP_ACTIVE_LOW, IPT_UNUSED) /* KICK1 BUTTON4/KICK1 BUTTON5/KICK1 BUTTON6 */

	PORT_MODIFY("KICK2")
	PORT_BIT(0x0007, IP_ACTIVE_LOW, IPT_UNUSED) /* KICK2 BUTTON4/KICK2 BUTTON5/KICK1 BUTTON6 */
INPUT_PORTS_END

INPUT_PORTS_START( tgmj )
	PORT_INCLUDE(capcom6b)

	PORT_MODIFY("P1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_4WAY
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_4WAY
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_4WAY
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_4WAY

	PORT_MODIFY("P2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_4WAY PORT_PLAYER(2)

	PORT_MODIFY("P3")
	PORT_BIT(0x70, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_MODIFY("P4")
	PORT_BIT(0x70, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_MODIFY("KICK1")
	PORT_BIT(0x0007, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_MODIFY("KICK2")
	PORT_BIT(0x0007, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


/* Capcom ZN1 */

#define CPZN1_BIOS \
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 ) \
	ROMX_LOAD( "m534002c-13.ic353", 0x0000000, 0x080000, CRC(50033af6) SHA1(486d92ff6c7f1e54f8e0ef41cd9116eca0e10e1a), ROM_BIOS(1) ) /* japanese */ \
	ROMX_LOAD( "msm27c402zb.ic353", 0x0000000, 0x080000, CRC(f20f7fe5) SHA1(9aac7d3b3d0cc0bbbe4056164b73078dce41d91c), ROM_BIOS(2) ) /* english (debug) */ \
	ROM_REGION( 0x000008, REGION_USER4, 0 ) /* CAT702, main board */ \
	ROM_LOAD( "cp01.ic652", 0, 8, CRC(bb7ba033) SHA1(966f3aee5d00e1312c455f855915d621e6cb0672) )

ROM_START( coh1000c )
	CPZN1_BIOS
ROM_END

ROM_START( ts2 )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ts2e_04.2h", 0x0000000, 0x080000, CRC(7a3b535f) SHA1(f17f0dad81e50a7f4ccd042a26af1d472df4785f) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "ts2-05m.3h", 0x0000000, 0x400000, CRC(7f4228e2) SHA1(3690a76d19d97e55bc7b05a8456328697cfd7a77) )
	ROM_LOAD( "ts2-06m.4h", 0x0400000, 0x200000, CRC(67be6797) SHA1(521f69439ba7666f54d1008a291f3391f3a16499) )
	ROM_LOAD( "ts2-07m.5h", 0x0600000, 0x200000, CRC(db73e2b8) SHA1(239243f44c41df765789c14cc4036bb02e2ab373) )
	ROM_LOAD( "ts2-08m.2k", 0x0800000, 0x200000, CRC(01a48431) SHA1(6e395af726da91909e07dac25bb9b70b3ccebd4e) )
	ROM_LOAD( "ts2-09m.3k", 0x0a00000, 0x200000, CRC(83f408de) SHA1(415787c4dca604dd5611e16936a0ffa981dedf78) )
	ROM_LOAD( "ts2-10m.4k", 0x0c00000, 0x200000, CRC(ad90679a) SHA1(19dd30764f892ee7f89c78ccbccdaf4d6b0e6e09) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ts2_02.2e", 0x00000, 0x08000, CRC(2f45c461) SHA1(513b6b9b5a2f7c567c30c958e0e13834cd9bd266) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ts2-01m.3b", 0x0000000, 0x400000, CRC(d7a505e0) SHA1(f1b0cdea712101f695bd326eccd753eb79a07490) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp02", 0, 8, CRC(40b0b697) SHA1(a8ee10fc9c0257de0a9ed0db29ea08afe900405f) )
ROM_END

ROM_START( ts2u )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ts2u_04.2h", 0x0000000, 0x080000, CRC(ddb52e7c) SHA1(e77891abae7681d911ef6eba2e0920d81433ebe6) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "ts2-05m.3h", 0x0000000, 0x400000, CRC(7f4228e2) SHA1(3690a76d19d97e55bc7b05a8456328697cfd7a77) )
	ROM_LOAD( "ts2-06m.4h", 0x0400000, 0x200000, CRC(67be6797) SHA1(521f69439ba7666f54d1008a291f3391f3a16499) )
	ROM_LOAD( "ts2-07m.5h", 0x0600000, 0x200000, CRC(db73e2b8) SHA1(239243f44c41df765789c14cc4036bb02e2ab373) )
	ROM_LOAD( "ts2-08m.2k", 0x0800000, 0x200000, CRC(01a48431) SHA1(6e395af726da91909e07dac25bb9b70b3ccebd4e) )
	ROM_LOAD( "ts2-09m.3k", 0x0a00000, 0x200000, CRC(83f408de) SHA1(415787c4dca604dd5611e16936a0ffa981dedf78) )
	ROM_LOAD( "ts2-10m.4k", 0x0c00000, 0x200000, CRC(ad90679a) SHA1(19dd30764f892ee7f89c78ccbccdaf4d6b0e6e09) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ts2_02.2e", 0x00000, 0x08000, CRC(2f45c461) SHA1(513b6b9b5a2f7c567c30c958e0e13834cd9bd266) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ts2-01m.3b", 0x0000000, 0x400000, CRC(d7a505e0) SHA1(f1b0cdea712101f695bd326eccd753eb79a07490) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp02", 0, 8, CRC(40b0b697) SHA1(a8ee10fc9c0257de0a9ed0db29ea08afe900405f) )
ROM_END

ROM_START( ts2ua )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ts2u_04.2h", 0x0000000, 0x080000, CRC(ddb52e7c) SHA1(e77891abae7681d911ef6eba2e0920d81433ebe6) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "ts2-05m.3h", 0x0000000, 0x400000, CRC(7f4228e2) SHA1(3690a76d19d97e55bc7b05a8456328697cfd7a77) )
	ROM_LOAD( "ts2-06m", 0x0400000, 0x400000, CRC(cd7e0a27) SHA1(325b5f2e653cdea07cddc9d20d12b5ab50dca949) )
	ROM_LOAD( "ts2-08m", 0x0800000, 0x400000, CRC(b1f7f115) SHA1(3f416d2aac07aa73a99593b5a21b047da60cea6a) )
	ROM_LOAD( "ts2-10m.4k", 0x0c00000, 0x200000, CRC(ad90679a) SHA1(19dd30764f892ee7f89c78ccbccdaf4d6b0e6e09) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ts2_02.2e", 0x00000, 0x08000, CRC(2f45c461) SHA1(513b6b9b5a2f7c567c30c958e0e13834cd9bd266) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ts2-01m.3b", 0x0000000, 0x400000, CRC(d7a505e0) SHA1(f1b0cdea712101f695bd326eccd753eb79a07490) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp02", 0, 8, CRC(40b0b697) SHA1(a8ee10fc9c0257de0a9ed0db29ea08afe900405f) )
ROM_END

ROM_START( ts2j )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ts2j_04.2h", 0x0000000, 0x080000, CRC(4aba8c5e) SHA1(a56001bf50bfc1b03036e88ae1febd1aac8c63c0) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "ts2-05m.3h", 0x0000000, 0x400000, CRC(7f4228e2) SHA1(3690a76d19d97e55bc7b05a8456328697cfd7a77) )
	ROM_LOAD( "ts2-06m.4h", 0x0400000, 0x200000, CRC(67be6797) SHA1(521f69439ba7666f54d1008a291f3391f3a16499) )
	ROM_LOAD( "ts2-07m.5h", 0x0600000, 0x200000, CRC(db73e2b8) SHA1(239243f44c41df765789c14cc4036bb02e2ab373) )
	ROM_LOAD( "ts2-08m.2k", 0x0800000, 0x200000, CRC(01a48431) SHA1(6e395af726da91909e07dac25bb9b70b3ccebd4e) )
	ROM_LOAD( "ts2-09m.3k", 0x0a00000, 0x200000, CRC(83f408de) SHA1(415787c4dca604dd5611e16936a0ffa981dedf78) )
	ROM_LOAD( "ts2-10m.4k", 0x0c00000, 0x200000, CRC(ad90679a) SHA1(19dd30764f892ee7f89c78ccbccdaf4d6b0e6e09) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ts2_02.2e", 0x00000, 0x08000, CRC(2f45c461) SHA1(513b6b9b5a2f7c567c30c958e0e13834cd9bd266) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ts2-01m.3b", 0x0000000, 0x400000, CRC(d7a505e0) SHA1(f1b0cdea712101f695bd326eccd753eb79a07490) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp02", 0, 8, CRC(40b0b697) SHA1(a8ee10fc9c0257de0a9ed0db29ea08afe900405f) )
ROM_END

ROM_START( ts2ja )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ts2j_04.2h", 0x0000000, 0x080000, CRC(4aba8c5e) SHA1(a56001bf50bfc1b03036e88ae1febd1aac8c63c0) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "ts2-05m.3h", 0x0000000, 0x400000, CRC(7f4228e2) SHA1(3690a76d19d97e55bc7b05a8456328697cfd7a77) )
	ROM_LOAD( "ts2-06m", 0x0400000, 0x400000, CRC(cd7e0a27) SHA1(325b5f2e653cdea07cddc9d20d12b5ab50dca949) )
	ROM_LOAD( "ts2-08m", 0x0800000, 0x400000, CRC(b1f7f115) SHA1(3f416d2aac07aa73a99593b5a21b047da60cea6a) )
	ROM_LOAD( "ts2-10m.4k", 0x0c00000, 0x200000, CRC(ad90679a) SHA1(19dd30764f892ee7f89c78ccbccdaf4d6b0e6e09) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ts2_02.2e", 0x00000, 0x08000, CRC(2f45c461) SHA1(513b6b9b5a2f7c567c30c958e0e13834cd9bd266) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ts2-01m.3b", 0x0000000, 0x400000, CRC(d7a505e0) SHA1(f1b0cdea712101f695bd326eccd753eb79a07490) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp02", 0, 8, CRC(40b0b697) SHA1(a8ee10fc9c0257de0a9ed0db29ea08afe900405f) )
ROM_END

ROM_START( starglad )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ps1u_04.2h", 0x0000000, 0x080000, CRC(121fb234) SHA1(697d18d37afd95f302b40a5a6a78d8c92a41ea73) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "ps1-05m.3h", 0x0000000, 0x400000, CRC(8ad72c4f) SHA1(c848c37eb5365000b4d4720b5c08d89ddd8e2c33) )
	ROM_LOAD( "ps1-06m.4h", 0x0400000, 0x400000, CRC(95d8ed61) SHA1(e9f259d589dc38a8321a6fea1f5dac741cadc0ff) )
	ROM_LOAD( "ps1-07m.5h", 0x0800000, 0x400000, CRC(c06752db) SHA1(0884b308e9cd9dde8660b422bc8fec9a362bcb52) )
	ROM_LOAD( "ps1-08m.2k", 0x0c00000, 0x400000, CRC(381f9ded) SHA1(b7878a90740f5b3c5881ac7d46e2b84b18727337) )
	ROM_LOAD( "ps1-09m.3k", 0x1000000, 0x400000, CRC(bd894812) SHA1(9f0c3365e685a53ae793f4a256a6c177a843a424) )
	ROM_LOAD( "ps1-10m.4k", 0x1400000, 0x400000, CRC(ff80c18a) SHA1(8d01717eed6ec1f508fe7c445da941fb84ef7d22) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ps1_02a.2e", 0x00000, 0x08000, CRC(b854df92) SHA1(ea71a613b5b19ec7e9c6e342e7743d320582a6bb) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "ps1_03a.3e", 0x28000, 0x20000, CRC(a2562fbb) SHA1(3de02a4aa7ea620961ca2a5c331f38134033db79) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ps1-01m.3b", 0x0000000, 0x400000, CRC(0bfb17aa) SHA1(cf4482785a2a33ad814c8b1461c5bc8e8e027895) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp03", 0, 8, CRC(27970400) SHA1(b240e3587d39315840c7471a0e97d7cd8275886c) )
ROM_END

ROM_START( stargladj )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ps1j_04.2h", 0x0000000, 0x080000, CRC(f865006d) SHA1(6abb476777a4309b1a60f12e6313c051db195808) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "ps1-05m.3h", 0x0000000, 0x400000, CRC(8ad72c4f) SHA1(c848c37eb5365000b4d4720b5c08d89ddd8e2c33) )
	ROM_LOAD( "ps1-06m.4h", 0x0400000, 0x400000, CRC(95d8ed61) SHA1(e9f259d589dc38a8321a6fea1f5dac741cadc0ff) )
	ROM_LOAD( "ps1-07m.5h", 0x0800000, 0x400000, CRC(c06752db) SHA1(0884b308e9cd9dde8660b422bc8fec9a362bcb52) )
	ROM_LOAD( "ps1-08m.2k", 0x0c00000, 0x400000, CRC(381f9ded) SHA1(b7878a90740f5b3c5881ac7d46e2b84b18727337) )
	ROM_LOAD( "ps1-09m.3k", 0x1000000, 0x400000, CRC(bd894812) SHA1(9f0c3365e685a53ae793f4a256a6c177a843a424) )
	ROM_LOAD( "ps1-10m.4k", 0x1400000, 0x400000, CRC(ff80c18a) SHA1(8d01717eed6ec1f508fe7c445da941fb84ef7d22) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ps1_02a.2e", 0x00000, 0x08000, CRC(b854df92) SHA1(ea71a613b5b19ec7e9c6e342e7743d320582a6bb) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "ps1_03a.3e", 0x28000, 0x20000, CRC(a2562fbb) SHA1(3de02a4aa7ea620961ca2a5c331f38134033db79) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ps1-01m.3b", 0x0000000, 0x400000, CRC(0bfb17aa) SHA1(cf4482785a2a33ad814c8b1461c5bc8e8e027895) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp03", 0, 8, CRC(27970400) SHA1(b240e3587d39315840c7471a0e97d7cd8275886c) )
ROM_END

ROM_START( glpracr )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "gra_ep-eng.2h", 0x0000000, 0x080000, CRC(f3ab9c85) SHA1(ce9d5d0406a6854975d5c71935fe917706334429) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "gra-05m.3h", 0x0000000, 0x400000, CRC(78053700) SHA1(38727c8cc34bb57b7b7e73041e382fb0361f184e) )
	ROM_LOAD( "gra-06m.4h", 0x0400000, 0x400000, CRC(d73b392b) SHA1(241ddf474cea035e81a2abc580d3c0395ee925bb) )
	ROM_LOAD( "gra-07m.5h", 0x0800000, 0x400000, CRC(acaefe3a) SHA1(32d596b0f975e1558fa7929c3166d8dad40a1c80) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_FILL( 0, 0x50000, 0x76 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 ) /* Q Sound Samples */

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp05", 0, 8, CRC(ae930900) SHA1(01fd03487943e21e3634285ef50107c6c2c4f6ca) )
ROM_END

ROM_START( glpracrj )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "gpaj_04.2h", 0x0000000, 0x080000, CRC(53bf551c) SHA1(320632b5010630cee4c5ccb1578d5ee6d2754632) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "gra-05m.3h", 0x0000000, 0x400000, CRC(78053700) SHA1(38727c8cc34bb57b7b7e73041e382fb0361f184e) )
	ROM_LOAD( "gra-06m.4h", 0x0400000, 0x400000, CRC(d73b392b) SHA1(241ddf474cea035e81a2abc580d3c0395ee925bb) )
	ROM_LOAD( "gra-07m.5h", 0x0800000, 0x400000, CRC(acaefe3a) SHA1(32d596b0f975e1558fa7929c3166d8dad40a1c80) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_FILL( 0, 0x50000, 0x76 )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 ) /* Q Sound Samples */

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp05", 0, 8, CRC(ae930900) SHA1(01fd03487943e21e3634285ef50107c6c2c4f6ca) )
ROM_END

ROM_START( sfex )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfee_04a.2h", 0x0000000, 0x080000, CRC(092cfa2e) SHA1(8af38a3f4f89f661233995a672faf486e71b79bc) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "sfe-05m.3h", 0x0000000, 0x400000, CRC(eab781fe) SHA1(205476cb72c8dac915e140fb32243dfc5d209ba4) )
	ROM_LOAD( "sfe-06m.4h", 0x0400000, 0x400000, CRC(999de60c) SHA1(092882698c411fc5c3bcb43105bf1886f94b8e40) )
	ROM_LOAD( "sfe-07m.5h", 0x0800000, 0x400000, CRC(76117b0a) SHA1(027233199170fa6e5b32f28da2031638c6d3d14a) )
	ROM_LOAD( "sfe-08m.2k", 0x0c00000, 0x400000, CRC(a36bbec5) SHA1(fa22ea50d4d8bed2ded97a346f61b2f5f68769b9) )
	ROM_LOAD( "sfe-09m.3k", 0x1000000, 0x400000, CRC(62c424cc) SHA1(ea19c49b486473b150dbf8541286e225655496db) )
	ROM_LOAD( "sfe-10m.4k", 0x1400000, 0x400000, CRC(83791a8b) SHA1(534969797640834ca692c11d0ce7c3a060fc7e4b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe_02.2e", 0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe_03.3e", 0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "sfe-01m.3b", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp04", 0, 8, CRC(e0dc24ae) SHA1(17d6e3dc11308195f4c46a6cd8093db9eaf584e5) )
ROM_END

ROM_START( sfexu )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfeu_04b.2h", 0x0000000, 0x080000, CRC(de02bd29) SHA1(62a88a30f73db661f5b98fc7e2d34d51acb965cc) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "sfe-05m.3h", 0x0000000, 0x400000, CRC(eab781fe) SHA1(205476cb72c8dac915e140fb32243dfc5d209ba4) )
	ROM_LOAD( "sfe-06m.4h", 0x0400000, 0x400000, CRC(999de60c) SHA1(092882698c411fc5c3bcb43105bf1886f94b8e40) )
	ROM_LOAD( "sfe-07m.5h", 0x0800000, 0x400000, CRC(76117b0a) SHA1(027233199170fa6e5b32f28da2031638c6d3d14a) )
	ROM_LOAD( "sfe-08m.2k", 0x0c00000, 0x400000, CRC(a36bbec5) SHA1(fa22ea50d4d8bed2ded97a346f61b2f5f68769b9) )
	ROM_LOAD( "sfe-09m.3k", 0x1000000, 0x400000, CRC(62c424cc) SHA1(ea19c49b486473b150dbf8541286e225655496db) )
	ROM_LOAD( "sfe-10m.4k", 0x1400000, 0x400000, CRC(83791a8b) SHA1(534969797640834ca692c11d0ce7c3a060fc7e4b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe_02.2e", 0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe_03.3e", 0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "sfe-01m.3b", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp04", 0, 8, CRC(e0dc24ae) SHA1(17d6e3dc11308195f4c46a6cd8093db9eaf584e5) )
ROM_END

ROM_START( sfexa )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfea_04a.2h", 0x0000000, 0x080000, CRC(08247bd4) SHA1(07f356ef2827b3fbd0bfaf2010915315d9d60ef1) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "sfe-05m.3h", 0x0000000, 0x400000, CRC(eab781fe) SHA1(205476cb72c8dac915e140fb32243dfc5d209ba4) )
	ROM_LOAD( "sfe-06m.4h", 0x0400000, 0x400000, CRC(999de60c) SHA1(092882698c411fc5c3bcb43105bf1886f94b8e40) )
	ROM_LOAD( "sfe-07m.5h", 0x0800000, 0x400000, CRC(76117b0a) SHA1(027233199170fa6e5b32f28da2031638c6d3d14a) )
	ROM_LOAD( "sfe-08m.2k", 0x0c00000, 0x400000, CRC(a36bbec5) SHA1(fa22ea50d4d8bed2ded97a346f61b2f5f68769b9) )
	ROM_LOAD( "sfe-09m.3k", 0x1000000, 0x400000, CRC(62c424cc) SHA1(ea19c49b486473b150dbf8541286e225655496db) )
	ROM_LOAD( "sfe-10m.4k", 0x1400000, 0x400000, CRC(83791a8b) SHA1(534969797640834ca692c11d0ce7c3a060fc7e4b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe_02.2e", 0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe_03.3e", 0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "sfe-01m.3b", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp04", 0, 8, CRC(e0dc24ae) SHA1(17d6e3dc11308195f4c46a6cd8093db9eaf584e5) )
ROM_END

ROM_START( sfexj )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfej_04.2h", 0x0000000, 0x080000, CRC(ea100607) SHA1(27ef8c619804999d32d14fcc5ec783c057b4dc73) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "sfe-05m.3h", 0x0000000, 0x400000, CRC(eab781fe) SHA1(205476cb72c8dac915e140fb32243dfc5d209ba4) )
	ROM_LOAD( "sfe-06m.4h", 0x0400000, 0x400000, CRC(999de60c) SHA1(092882698c411fc5c3bcb43105bf1886f94b8e40) )
	ROM_LOAD( "sfe-07m.5h", 0x0800000, 0x400000, CRC(76117b0a) SHA1(027233199170fa6e5b32f28da2031638c6d3d14a) )
	ROM_LOAD( "sfe-08m.2k", 0x0c00000, 0x400000, CRC(a36bbec5) SHA1(fa22ea50d4d8bed2ded97a346f61b2f5f68769b9) )
	ROM_LOAD( "sfe-09m.3k", 0x1000000, 0x400000, CRC(62c424cc) SHA1(ea19c49b486473b150dbf8541286e225655496db) )
	ROM_LOAD( "sfe-10m.4k", 0x1400000, 0x400000, CRC(83791a8b) SHA1(534969797640834ca692c11d0ce7c3a060fc7e4b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe_02.2e", 0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe_03.3e", 0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "sfe-01m.3b", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp04", 0, 8, CRC(e0dc24ae) SHA1(17d6e3dc11308195f4c46a6cd8093db9eaf584e5) )
ROM_END

ROM_START( sfexp )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfpu_04a.2h", 0x0000000, 0x080000, CRC(4617adc2) SHA1(200307904349ad7e5d7d76d8c904b6b10424c7ef) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "sfp-05m.3h", 0x0000000, 0x400000, CRC(ac7dcc5e) SHA1(216de2de691a9bd7982d5d6b5b1e3e35ff381a2f) )
	ROM_LOAD( "sfp-06m.4h", 0x0400000, 0x400000, CRC(1d504758) SHA1(bd56141aba35dbb5b318445ba5db12eff7442221) )
	ROM_LOAD( "sfp-07m.5h", 0x0800000, 0x400000, CRC(0f585f30) SHA1(24ffdbc360f8eddb702905c99d315614327861a7) )
	ROM_LOAD( "sfp-08m.2k", 0x0c00000, 0x400000, CRC(65eabc61) SHA1(bbeb3bcd8dd8f7f88ed82412a81134a3d6f6ffd9) )
	ROM_LOAD( "sfp-09m.3k", 0x1000000, 0x400000, CRC(15f8b71e) SHA1(efb28fbe750f443550ee9718385355aae7e858c9) )
	ROM_LOAD( "sfp-10m.4k", 0x1400000, 0x400000, CRC(c1ecf652) SHA1(616e14ff63d38272730c810b933a6b3412e2da17) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe_02.2e", 0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe_03.3e", 0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "sfe-01m.3b", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp04", 0, 8, CRC(e0dc24ae) SHA1(17d6e3dc11308195f4c46a6cd8093db9eaf584e5) )
ROM_END

ROM_START( sfexpu1 )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfpu_04.2h", 0x0000000, 0x080000, CRC(305e4ec0) SHA1(0df9572d7fc1bbc7131483960771d016fa5487a5) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "sfp-05m.3h", 0x0000000, 0x400000, CRC(ac7dcc5e) SHA1(216de2de691a9bd7982d5d6b5b1e3e35ff381a2f) )
	ROM_LOAD( "sfp-06m.4h", 0x0400000, 0x400000, CRC(1d504758) SHA1(bd56141aba35dbb5b318445ba5db12eff7442221) )
	ROM_LOAD( "sfp-07m.5h", 0x0800000, 0x400000, CRC(0f585f30) SHA1(24ffdbc360f8eddb702905c99d315614327861a7) )
	ROM_LOAD( "sfp-08m.2k", 0x0c00000, 0x400000, CRC(65eabc61) SHA1(bbeb3bcd8dd8f7f88ed82412a81134a3d6f6ffd9) )
	ROM_LOAD( "sfp-09m.3k", 0x1000000, 0x400000, CRC(15f8b71e) SHA1(efb28fbe750f443550ee9718385355aae7e858c9) )
	ROM_LOAD( "sfp-10m.4k", 0x1400000, 0x400000, CRC(c1ecf652) SHA1(616e14ff63d38272730c810b933a6b3412e2da17) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe_02.2e", 0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe_03.3e", 0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "sfe-01m.3b", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp04", 0, 8, CRC(e0dc24ae) SHA1(17d6e3dc11308195f4c46a6cd8093db9eaf584e5) )
ROM_END

ROM_START( sfexpj )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfpj_04a.2h", 0x0000000, 0x080000, CRC(6e99a0f7) SHA1(8f22bc545dd0e3eff24ab62ce5af1998d48d3770) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "sfp-05m.3h", 0x0000000, 0x400000, CRC(ac7dcc5e) SHA1(216de2de691a9bd7982d5d6b5b1e3e35ff381a2f) )
	ROM_LOAD( "sfp-06m.4h", 0x0400000, 0x400000, CRC(1d504758) SHA1(bd56141aba35dbb5b318445ba5db12eff7442221) )
	ROM_LOAD( "sfp-07m.5h", 0x0800000, 0x400000, CRC(0f585f30) SHA1(24ffdbc360f8eddb702905c99d315614327861a7) )
	ROM_LOAD( "sfp-08m.2k", 0x0c00000, 0x400000, CRC(65eabc61) SHA1(bbeb3bcd8dd8f7f88ed82412a81134a3d6f6ffd9) )
	ROM_LOAD( "sfp-09m.3k", 0x1000000, 0x400000, CRC(15f8b71e) SHA1(efb28fbe750f443550ee9718385355aae7e858c9) )
	ROM_LOAD( "sfp-10m.4k", 0x1400000, 0x400000, CRC(c1ecf652) SHA1(616e14ff63d38272730c810b933a6b3412e2da17) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe_02.2e", 0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe_03.3e", 0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "sfe-01m.3b", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp04", 0, 8, CRC(e0dc24ae) SHA1(17d6e3dc11308195f4c46a6cd8093db9eaf584e5) )
ROM_END

ROM_START( sfexpj1 )
	CPZN1_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sfpj_04.2h", 0x0000000, 0x080000, CRC(18d043f5) SHA1(9e6e24a722d13888fbfd391ddb1a5045b162488c) )

	ROM_REGION32_LE( 0x2400000, REGION_USER2, 0 )
	ROM_LOAD( "sfp-05m.3h", 0x0000000, 0x400000, CRC(ac7dcc5e) SHA1(216de2de691a9bd7982d5d6b5b1e3e35ff381a2f) )
	ROM_LOAD( "sfp-06m.4h", 0x0400000, 0x400000, CRC(1d504758) SHA1(bd56141aba35dbb5b318445ba5db12eff7442221) )
	ROM_LOAD( "sfp-07m.5h", 0x0800000, 0x400000, CRC(0f585f30) SHA1(24ffdbc360f8eddb702905c99d315614327861a7) )
	ROM_LOAD( "sfp-08m.2k", 0x0c00000, 0x400000, CRC(65eabc61) SHA1(bbeb3bcd8dd8f7f88ed82412a81134a3d6f6ffd9) )
	ROM_LOAD( "sfp-09m.3k", 0x1000000, 0x400000, CRC(15f8b71e) SHA1(efb28fbe750f443550ee9718385355aae7e858c9) )
	ROM_LOAD( "sfp-10m.4k", 0x1400000, 0x400000, CRC(c1ecf652) SHA1(616e14ff63d38272730c810b933a6b3412e2da17) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfe_02.2e", 0x00000, 0x08000, CRC(1908475c) SHA1(99f68cff2d92f5697eec0846201f6fb317d5dc08) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sfe_03.3e", 0x28000, 0x20000, CRC(95c1e2e0) SHA1(383bbe9613798a3ac6944d18768280a840994e40) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "sfe-01m.3b", 0x0000000, 0x400000, CRC(f5afff0d) SHA1(7f9ac32ba0a3d9c6fef367e36a92d47c9ac1feb3) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp04", 0, 8, CRC(e0dc24ae) SHA1(17d6e3dc11308195f4c46a6cd8093db9eaf584e5) )
ROM_END

/* Capcom ZN2 */

#define CPZN2_BIOS \
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "m534002c-59.ic353", 0x0000000, 0x080000, CRC(e860ea8b) SHA1(66e7e1d4e426466b8f48a2ba055a91b475569504) ) \
	ROM_REGION( 0x000008, REGION_USER4, 0 ) /* CAT702, main board */ \
	ROM_LOAD( "cp10.ic652", 0, 8, CRC(7808fcef) SHA1(3c3fb8227b05dace042dc1429b9f2355b1a15638) )

ROM_START( coh3002c )
	CPZN2_BIOS
ROM_END

ROM_START( rvschool )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "jste_04.2h", 0x0000000, 0x080000, CRC(1567555a) SHA1(3b93235076ab3c06914c83becf0da8e810b8917a) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "jst-05m.3h", 0x0000000, 0x400000, CRC(723372b8) SHA1(2a7c95d1f9a3f58c469dfc28ead1fd192eaaebd1) )
	ROM_LOAD( "jst-06m.4h", 0x0400000, 0x400000, CRC(4248988e) SHA1(4bdf7cac17d70ea85aa2002fc6b21a64d05e6e5a) )
	ROM_LOAD( "jst-07m.5h", 0x0800000, 0x400000, CRC(c84c5a16) SHA1(5c0ca7454189c766f1ca7305504ff1867007c8e6) )
	ROM_LOAD( "jst-08m.2k", 0x0c00000, 0x400000, CRC(791b57f3) SHA1(4ea12a0f7a7110d7dcbc55b3f02aa9a92dea4b12) )
	ROM_LOAD( "jst-09m.3k", 0x1000000, 0x400000, CRC(6df42048) SHA1(9e2b4a424de3918e5e54bc87fd9dcceff8d162be) )
	ROM_LOAD( "jst-10m.4k", 0x1400000, 0x400000, CRC(d7e22769) SHA1(733f96dce2586fc0a8af3cec18153085750c9a4d) )
	ROM_LOAD( "jst-11m.5k", 0x1800000, 0x400000, CRC(0a033ac5) SHA1(218b33cb51db99d3e9ee180da6a74460f4444fc6) )
	ROM_LOAD( "jst-12m.6k", 0x1c00000, 0x400000, CRC(43bd2ddd) SHA1(7f2976e394362cb648f620e430b3bf11b71485a6) )
	ROM_LOAD( "jst-13m.7k", 0x2000000, 0x400000, CRC(6b443235) SHA1(c764d8b742aa1c46bc8d37f36e864ef50a1ff4e4) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst_02.2e", 0x00000, 0x08000, CRC(7809e2c3) SHA1(0216a665f7978bc8db3f7fdab038e1c7aa120844) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "jst_03.3e", 0x28000, 0x20000, CRC(860ff24d) SHA1(eea72fa5eaf407a112a5b3daf60f7ac8ad191cc7) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "jst-01m.3b", 0x0000000, 0x400000, CRC(9a7c98f9) SHA1(764c6c4f41047e1f36d2dceac4aa9b943a9d529a) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp06", 0, 8, CRC(99b22128) SHA1(9a773927ead72ed4ded44d53d89ecb123e1d3f17) )
ROM_END

ROM_START( rvschoolu )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "jstu_04.2h", 0x0000000, 0x080000, CRC(d83724ae) SHA1(0890c0164116606acc600f646e82972d0d4f79b4) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "jst-05m.3h", 0x0000000, 0x400000, CRC(723372b8) SHA1(2a7c95d1f9a3f58c469dfc28ead1fd192eaaebd1) )
	ROM_LOAD( "jst-06m.4h", 0x0400000, 0x400000, CRC(4248988e) SHA1(4bdf7cac17d70ea85aa2002fc6b21a64d05e6e5a) )
	ROM_LOAD( "jst-07m.5h", 0x0800000, 0x400000, CRC(c84c5a16) SHA1(5c0ca7454189c766f1ca7305504ff1867007c8e6) )
	ROM_LOAD( "jst-08m.2k", 0x0c00000, 0x400000, CRC(791b57f3) SHA1(4ea12a0f7a7110d7dcbc55b3f02aa9a92dea4b12) )
	ROM_LOAD( "jst-09m.3k", 0x1000000, 0x400000, CRC(6df42048) SHA1(9e2b4a424de3918e5e54bc87fd9dcceff8d162be) )
	ROM_LOAD( "jst-10m.4k", 0x1400000, 0x400000, CRC(d7e22769) SHA1(733f96dce2586fc0a8af3cec18153085750c9a4d) )
	ROM_LOAD( "jst-11m.5k", 0x1800000, 0x400000, CRC(0a033ac5) SHA1(218b33cb51db99d3e9ee180da6a74460f4444fc6) )
	ROM_LOAD( "jst-12m.6k", 0x1c00000, 0x400000, CRC(43bd2ddd) SHA1(7f2976e394362cb648f620e430b3bf11b71485a6) )
	ROM_LOAD( "jst-13m.7k", 0x2000000, 0x400000, CRC(6b443235) SHA1(c764d8b742aa1c46bc8d37f36e864ef50a1ff4e4) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst_02.2e", 0x00000, 0x08000, CRC(7809e2c3) SHA1(0216a665f7978bc8db3f7fdab038e1c7aa120844) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "jst_03.3e", 0x28000, 0x20000, CRC(860ff24d) SHA1(eea72fa5eaf407a112a5b3daf60f7ac8ad191cc7) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "jst-01m.3b", 0x0000000, 0x400000, CRC(9a7c98f9) SHA1(764c6c4f41047e1f36d2dceac4aa9b943a9d529a) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp06", 0, 8, CRC(99b22128) SHA1(9a773927ead72ed4ded44d53d89ecb123e1d3f17) )
ROM_END

ROM_START( rvschoola )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "jsta_04.2h", 0x0000000, 0x080000, CRC(034b1011) SHA1(6773246be242ee336503d21d7d44a3884832eb1e) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "jst-05m.3h", 0x0000000, 0x400000, CRC(723372b8) SHA1(2a7c95d1f9a3f58c469dfc28ead1fd192eaaebd1) )
	ROM_LOAD( "jst-06m.4h", 0x0400000, 0x400000, CRC(4248988e) SHA1(4bdf7cac17d70ea85aa2002fc6b21a64d05e6e5a) )
	ROM_LOAD( "jst-07m.5h", 0x0800000, 0x400000, CRC(c84c5a16) SHA1(5c0ca7454189c766f1ca7305504ff1867007c8e6) )
	ROM_LOAD( "jst-08m.2k", 0x0c00000, 0x400000, CRC(791b57f3) SHA1(4ea12a0f7a7110d7dcbc55b3f02aa9a92dea4b12) )
	ROM_LOAD( "jst-09m.3k", 0x1000000, 0x400000, CRC(6df42048) SHA1(9e2b4a424de3918e5e54bc87fd9dcceff8d162be) )
	ROM_LOAD( "jst-10m.4k", 0x1400000, 0x400000, CRC(d7e22769) SHA1(733f96dce2586fc0a8af3cec18153085750c9a4d) )
	ROM_LOAD( "jst-11m.5k", 0x1800000, 0x400000, CRC(0a033ac5) SHA1(218b33cb51db99d3e9ee180da6a74460f4444fc6) )
	ROM_LOAD( "jst-12m.6k", 0x1c00000, 0x400000, CRC(43bd2ddd) SHA1(7f2976e394362cb648f620e430b3bf11b71485a6) )
	ROM_LOAD( "jst-13m.7k", 0x2000000, 0x400000, CRC(6b443235) SHA1(c764d8b742aa1c46bc8d37f36e864ef50a1ff4e4) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst_02.2e", 0x00000, 0x08000, CRC(7809e2c3) SHA1(0216a665f7978bc8db3f7fdab038e1c7aa120844) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "jst_03.3e", 0x28000, 0x20000, CRC(860ff24d) SHA1(eea72fa5eaf407a112a5b3daf60f7ac8ad191cc7) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "jst-01m.3b", 0x0000000, 0x400000, CRC(9a7c98f9) SHA1(764c6c4f41047e1f36d2dceac4aa9b943a9d529a) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp06", 0, 8, CRC(99b22128) SHA1(9a773927ead72ed4ded44d53d89ecb123e1d3f17) )
ROM_END

ROM_START( jgakuen )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "jstj_04a.2h", 0x0000000, 0x080000, CRC(91b36f38) SHA1(423dbf0f9f08458a866e9acb2a5b657c736b179f) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "jst-05m.3h", 0x0000000, 0x400000, CRC(723372b8) SHA1(2a7c95d1f9a3f58c469dfc28ead1fd192eaaebd1) )
	ROM_LOAD( "jst-06m.4h", 0x0400000, 0x400000, CRC(4248988e) SHA1(4bdf7cac17d70ea85aa2002fc6b21a64d05e6e5a) )
	ROM_LOAD( "jst-07m.5h", 0x0800000, 0x400000, CRC(c84c5a16) SHA1(5c0ca7454189c766f1ca7305504ff1867007c8e6) )
	ROM_LOAD( "jst-08m.2k", 0x0c00000, 0x400000, CRC(791b57f3) SHA1(4ea12a0f7a7110d7dcbc55b3f02aa9a92dea4b12) )
	ROM_LOAD( "jst-09m.3k", 0x1000000, 0x400000, CRC(6df42048) SHA1(9e2b4a424de3918e5e54bc87fd9dcceff8d162be) )
	ROM_LOAD( "jst-10m.4k", 0x1400000, 0x400000, CRC(d7e22769) SHA1(733f96dce2586fc0a8af3cec18153085750c9a4d) )
	ROM_LOAD( "jst-11m.5k", 0x1800000, 0x400000, CRC(0a033ac5) SHA1(218b33cb51db99d3e9ee180da6a74460f4444fc6) )
	ROM_LOAD( "jst-12m.6k", 0x1c00000, 0x400000, CRC(43bd2ddd) SHA1(7f2976e394362cb648f620e430b3bf11b71485a6) )
	ROM_LOAD( "jst-13m.7k", 0x2000000, 0x400000, CRC(6b443235) SHA1(c764d8b742aa1c46bc8d37f36e864ef50a1ff4e4) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst_02.2e", 0x00000, 0x08000, CRC(7809e2c3) SHA1(0216a665f7978bc8db3f7fdab038e1c7aa120844) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "jst_03.3e", 0x28000, 0x20000, CRC(860ff24d) SHA1(eea72fa5eaf407a112a5b3daf60f7ac8ad191cc7) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "jst-01m.3b", 0x0000000, 0x400000, CRC(9a7c98f9) SHA1(764c6c4f41047e1f36d2dceac4aa9b943a9d529a) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp06", 0, 8, CRC(99b22128) SHA1(9a773927ead72ed4ded44d53d89ecb123e1d3f17) )
ROM_END

ROM_START( jgakuen1 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "jstj_04.2h", 0x0000000, 0x080000, CRC(28b8000a) SHA1(9ebf74b453d775cadca9c2d7d8e2c7eb57bb9a38) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "jst-05m.3h", 0x0000000, 0x400000, CRC(723372b8) SHA1(2a7c95d1f9a3f58c469dfc28ead1fd192eaaebd1) )
	ROM_LOAD( "jst-06m.4h", 0x0400000, 0x400000, CRC(4248988e) SHA1(4bdf7cac17d70ea85aa2002fc6b21a64d05e6e5a) )
	ROM_LOAD( "jst-07m.5h", 0x0800000, 0x400000, CRC(c84c5a16) SHA1(5c0ca7454189c766f1ca7305504ff1867007c8e6) )
	ROM_LOAD( "jst-08m.2k", 0x0c00000, 0x400000, CRC(791b57f3) SHA1(4ea12a0f7a7110d7dcbc55b3f02aa9a92dea4b12) )
	ROM_LOAD( "jst-09m.3k", 0x1000000, 0x400000, CRC(6df42048) SHA1(9e2b4a424de3918e5e54bc87fd9dcceff8d162be) )
	ROM_LOAD( "jst-10m.4k", 0x1400000, 0x400000, CRC(d7e22769) SHA1(733f96dce2586fc0a8af3cec18153085750c9a4d) )
	ROM_LOAD( "jst-11m.5k", 0x1800000, 0x400000, CRC(0a033ac5) SHA1(218b33cb51db99d3e9ee180da6a74460f4444fc6) )
	ROM_LOAD( "jst-12m.6k", 0x1c00000, 0x400000, CRC(43bd2ddd) SHA1(7f2976e394362cb648f620e430b3bf11b71485a6) )
	ROM_LOAD( "jst-13m.7k", 0x2000000, 0x400000, CRC(6b443235) SHA1(c764d8b742aa1c46bc8d37f36e864ef50a1ff4e4) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "jst_02.2e", 0x00000, 0x08000, CRC(7809e2c3) SHA1(0216a665f7978bc8db3f7fdab038e1c7aa120844) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "jst_03.3e", 0x28000, 0x20000, CRC(860ff24d) SHA1(eea72fa5eaf407a112a5b3daf60f7ac8ad191cc7) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "jst-01m.3b", 0x0000000, 0x400000, CRC(9a7c98f9) SHA1(764c6c4f41047e1f36d2dceac4aa9b943a9d529a) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp06", 0, 8, CRC(99b22128) SHA1(9a773927ead72ed4ded44d53d89ecb123e1d3f17) )
ROM_END

ROM_START( sfex2 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ex2e_04.2h", 0x0000000, 0x080000, CRC(77e1622b) SHA1(626a6718e7e843b09075b652d4fcbd9c86eea02b) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "ex2-05m.3h", 0x0000000, 0x800000, CRC(78726b17) SHA1(2da449df335ef133ebc3997bbad73ef4137f4771) )
	ROM_LOAD( "ex2-06m.4h", 0x0800000, 0x800000, CRC(be1075ed) SHA1(36dc673372f30f8b3ff5689ae568c5cd01fe2c07) )
	ROM_LOAD( "ex2-07m.5h", 0x1000000, 0x800000, CRC(6496c6ed) SHA1(054bcecbb04033abea14d9ffe6634b2bd11ca88b) )
	ROM_LOAD( "ex2-08m.2k", 0x1800000, 0x800000, CRC(3194132e) SHA1(d1324fcf0a8528fc683791d6342697a7e08674f4) )
	ROM_LOAD( "ex2-09m.3k", 0x2000000, 0x400000, CRC(075ae585) SHA1(6b88851db618fc3e96f1d740c46c1bc5be0ee21b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ex2_02.2e", 0x00000, 0x08000, CRC(9489875e) SHA1(1fc9985ff98232c63ea8d05a69f7d77cdf72919f) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ex2-01m.3a", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp08", 0, 8, CRC(a63d6fa6) SHA1(68995438a1e90ff9aa59090e7e031d51c68c4d73) )
ROM_END

ROM_START( sfex2u )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ex2u_04a.2h", 0x0000000, 0x080000, CRC(8dc5317f) SHA1(c35224caf70662a0e45a74cbead294a51f9b9e16) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "ex2-05m.3h", 0x0000000, 0x800000, CRC(78726b17) SHA1(2da449df335ef133ebc3997bbad73ef4137f4771) )
	ROM_LOAD( "ex2-06m.4h", 0x0800000, 0x800000, CRC(be1075ed) SHA1(36dc673372f30f8b3ff5689ae568c5cd01fe2c07) )
	ROM_LOAD( "ex2-07m.5h", 0x1000000, 0x800000, CRC(6496c6ed) SHA1(054bcecbb04033abea14d9ffe6634b2bd11ca88b) )
	ROM_LOAD( "ex2-08m.2k", 0x1800000, 0x800000, CRC(3194132e) SHA1(d1324fcf0a8528fc683791d6342697a7e08674f4) )
	ROM_LOAD( "ex2-09m.3k", 0x2000000, 0x400000, CRC(075ae585) SHA1(6b88851db618fc3e96f1d740c46c1bc5be0ee21b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ex2_02.2e", 0x00000, 0x08000, CRC(9489875e) SHA1(1fc9985ff98232c63ea8d05a69f7d77cdf72919f) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ex2-01m.3a", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp08", 0, 8, CRC(a63d6fa6) SHA1(68995438a1e90ff9aa59090e7e031d51c68c4d73) )
ROM_END

ROM_START( sfex2u1 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ex2u_04.2h", 0x0000000, 0x080000, CRC(eb417c2c) SHA1(bd47d0a1d911af4457c795ec348c847c2cf9e721) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "ex2-05m.3h", 0x0000000, 0x800000, CRC(78726b17) SHA1(2da449df335ef133ebc3997bbad73ef4137f4771) )
	ROM_LOAD( "ex2-06m.4h", 0x0800000, 0x800000, CRC(be1075ed) SHA1(36dc673372f30f8b3ff5689ae568c5cd01fe2c07) )
	ROM_LOAD( "ex2-07m.5h", 0x1000000, 0x800000, CRC(6496c6ed) SHA1(054bcecbb04033abea14d9ffe6634b2bd11ca88b) )
	ROM_LOAD( "ex2-08m.2k", 0x1800000, 0x800000, CRC(3194132e) SHA1(d1324fcf0a8528fc683791d6342697a7e08674f4) )
	ROM_LOAD( "ex2-09m.3k", 0x2000000, 0x400000, CRC(075ae585) SHA1(6b88851db618fc3e96f1d740c46c1bc5be0ee21b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ex2_02.2e", 0x00000, 0x08000, CRC(9489875e) SHA1(1fc9985ff98232c63ea8d05a69f7d77cdf72919f) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ex2-01m.3a", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp08", 0, 8, CRC(a63d6fa6) SHA1(68995438a1e90ff9aa59090e7e031d51c68c4d73) )
ROM_END

ROM_START( sfex2a )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ex2a_04.2h", 0x0000000, 0x080000, CRC(ac9a872d) SHA1(4e237f5e2e5de58e587e9abc5767509b8d750004) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "ex2-05m.3h", 0x0000000, 0x800000, CRC(78726b17) SHA1(2da449df335ef133ebc3997bbad73ef4137f4771) )
	ROM_LOAD( "ex2-06m.4h", 0x0800000, 0x800000, CRC(be1075ed) SHA1(36dc673372f30f8b3ff5689ae568c5cd01fe2c07) )
	ROM_LOAD( "ex2-07m.5h", 0x1000000, 0x800000, CRC(6496c6ed) SHA1(054bcecbb04033abea14d9ffe6634b2bd11ca88b) )
	ROM_LOAD( "ex2-08m.2k", 0x1800000, 0x800000, CRC(3194132e) SHA1(d1324fcf0a8528fc683791d6342697a7e08674f4) )
	ROM_LOAD( "ex2-09m.3k", 0x2000000, 0x400000, CRC(075ae585) SHA1(6b88851db618fc3e96f1d740c46c1bc5be0ee21b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ex2_02.2e", 0x00000, 0x08000, CRC(9489875e) SHA1(1fc9985ff98232c63ea8d05a69f7d77cdf72919f) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ex2-01m.3a", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp08", 0, 8, CRC(a63d6fa6) SHA1(68995438a1e90ff9aa59090e7e031d51c68c4d73) )
ROM_END

ROM_START( sfex2h )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ex2h_04.2h", 0x0000000, 0x080000, CRC(68f2ef80) SHA1(212bb3c0d935c64f5e3b20e427e06d97404709d8) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "ex2-05m.3h", 0x0000000, 0x800000, CRC(78726b17) SHA1(2da449df335ef133ebc3997bbad73ef4137f4771) )
	ROM_LOAD( "ex2-06m.4h", 0x0800000, 0x800000, CRC(be1075ed) SHA1(36dc673372f30f8b3ff5689ae568c5cd01fe2c07) )
	ROM_LOAD( "ex2-07m.5h", 0x1000000, 0x800000, CRC(6496c6ed) SHA1(054bcecbb04033abea14d9ffe6634b2bd11ca88b) )
	ROM_LOAD( "ex2-08m.2k", 0x1800000, 0x800000, CRC(3194132e) SHA1(d1324fcf0a8528fc683791d6342697a7e08674f4) )
	ROM_LOAD( "ex2-09m.3k", 0x2000000, 0x400000, CRC(075ae585) SHA1(6b88851db618fc3e96f1d740c46c1bc5be0ee21b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ex2_02.2e", 0x00000, 0x08000, CRC(9489875e) SHA1(1fc9985ff98232c63ea8d05a69f7d77cdf72919f) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ex2-01m.3a", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp08", 0, 8, CRC(a63d6fa6) SHA1(68995438a1e90ff9aa59090e7e031d51c68c4d73) )
ROM_END

ROM_START( sfex2j )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "ex2j_04.2h", 0x0000000, 0x080000, CRC(5d603586) SHA1(ff546d3bd011d6441e9672b88bab763d3cd89be2) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "ex2-05m.3h", 0x0000000, 0x800000, CRC(78726b17) SHA1(2da449df335ef133ebc3997bbad73ef4137f4771) )
	ROM_LOAD( "ex2-06m.4h", 0x0800000, 0x800000, CRC(be1075ed) SHA1(36dc673372f30f8b3ff5689ae568c5cd01fe2c07) )
	ROM_LOAD( "ex2-07m.5h", 0x1000000, 0x800000, CRC(6496c6ed) SHA1(054bcecbb04033abea14d9ffe6634b2bd11ca88b) )
	ROM_LOAD( "ex2-08m.2k", 0x1800000, 0x800000, CRC(3194132e) SHA1(d1324fcf0a8528fc683791d6342697a7e08674f4) )
	ROM_LOAD( "ex2-09m.3k", 0x2000000, 0x400000, CRC(075ae585) SHA1(6b88851db618fc3e96f1d740c46c1bc5be0ee21b) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ex2_02.2e", 0x00000, 0x08000, CRC(9489875e) SHA1(1fc9985ff98232c63ea8d05a69f7d77cdf72919f) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ex2-01m.3a", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp08", 0, 8, CRC(a63d6fa6) SHA1(68995438a1e90ff9aa59090e7e031d51c68c4d73) )
ROM_END

ROM_START( plsmaswd )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sg2u_04.2h", 0x0000000, 0x080000, CRC(154187c0) SHA1(58cc0e9d32786b1c1d64ecee4667190456b36ef6) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "sg2-05m.3h", 0x0000000, 0x800000, CRC(f1759236) SHA1(fbe3a820a8c571dfb186eae68346e6461168ed48) )
	ROM_LOAD( "sg2-06m.4h", 0x0800000, 0x800000, CRC(33de4f72) SHA1(ab32af76b5682e3d9f67dadbaed35abc043912b4) )
	ROM_LOAD( "sg2-07m.5h", 0x1000000, 0x800000, CRC(72f724ba) SHA1(e6658b495d308d1de6710f87b5b9d346008b0c5a) )
	ROM_LOAD( "sg2-08m.2k", 0x1800000, 0x800000, CRC(9e169eee) SHA1(6141b1a7863fdfb200ca35d2893979a34dcc3f6c) )
	ROM_LOAD( "sg2-09m.3k", 0x2000000, 0x400000, CRC(33f73d4c) SHA1(954695a43e77b58585409678bd87c76adac1d855) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sg2_02.2e", 0x00000, 0x08000, CRC(415ee138) SHA1(626083c8705f012552691c450f95401ddc88065b) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sg2_03.3e", 0x28000, 0x20000, CRC(43806735) SHA1(88d389bcc79cbd4fa1f4b62008e171a897e77652) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "sg2-01m.3a", 0x0000000, 0x400000, CRC(643ea27b) SHA1(40747432d5cfebac54d3824b6a6f26b5e7742fc1) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp07", 0, 8, CRC(4c1acdc2) SHA1(8c0b4ae95d6ba89e4bdf9fce5e2605c2728ae67f) )
ROM_END

ROM_START( plsmaswda )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sg2a_04.2h", 0x0000000, 0x080000, CRC(66e5dada) SHA1(f2e50ee963b8a6aadf25a17b3ff6dcb428b8bdb2) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "sg2-05m.3h", 0x0000000, 0x800000, CRC(f1759236) SHA1(fbe3a820a8c571dfb186eae68346e6461168ed48) )
	ROM_LOAD( "sg2-06m.4h", 0x0800000, 0x800000, CRC(33de4f72) SHA1(ab32af76b5682e3d9f67dadbaed35abc043912b4) )
	ROM_LOAD( "sg2-07m.5h", 0x1000000, 0x800000, CRC(72f724ba) SHA1(e6658b495d308d1de6710f87b5b9d346008b0c5a) )
	ROM_LOAD( "sg2-08m.2k", 0x1800000, 0x800000, CRC(9e169eee) SHA1(6141b1a7863fdfb200ca35d2893979a34dcc3f6c) )
	ROM_LOAD( "sg2-09m.3k", 0x2000000, 0x400000, CRC(33f73d4c) SHA1(954695a43e77b58585409678bd87c76adac1d855) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sg2_02.2e", 0x00000, 0x08000, CRC(415ee138) SHA1(626083c8705f012552691c450f95401ddc88065b) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sg2_03.3e", 0x28000, 0x20000, CRC(43806735) SHA1(88d389bcc79cbd4fa1f4b62008e171a897e77652) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "sg2-01m.3a", 0x0000000, 0x400000, CRC(643ea27b) SHA1(40747432d5cfebac54d3824b6a6f26b5e7742fc1) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp07", 0, 8, CRC(4c1acdc2) SHA1(8c0b4ae95d6ba89e4bdf9fce5e2605c2728ae67f) )
ROM_END

ROM_START( stargld2 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "sg2j_04.2h", 0x0000000, 0x080000, CRC(cf4ce6ac) SHA1(52b6f61d79671c9c108b3dfbd3c2ac333285412c) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "sg2-05m.3h", 0x0000000, 0x800000, CRC(f1759236) SHA1(fbe3a820a8c571dfb186eae68346e6461168ed48) )
	ROM_LOAD( "sg2-06m.4h", 0x0800000, 0x800000, CRC(33de4f72) SHA1(ab32af76b5682e3d9f67dadbaed35abc043912b4) )
	ROM_LOAD( "sg2-07m.5h", 0x1000000, 0x800000, CRC(72f724ba) SHA1(e6658b495d308d1de6710f87b5b9d346008b0c5a) )
	ROM_LOAD( "sg2-08m.2k", 0x1800000, 0x800000, CRC(9e169eee) SHA1(6141b1a7863fdfb200ca35d2893979a34dcc3f6c) )
	ROM_LOAD( "sg2-09m.3k", 0x2000000, 0x400000, CRC(33f73d4c) SHA1(954695a43e77b58585409678bd87c76adac1d855) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sg2_02.2e", 0x00000, 0x08000, CRC(415ee138) SHA1(626083c8705f012552691c450f95401ddc88065b) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "sg2_03.3e", 0x28000, 0x20000, CRC(43806735) SHA1(88d389bcc79cbd4fa1f4b62008e171a897e77652) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "sg2-01m.3a", 0x0000000, 0x400000, CRC(643ea27b) SHA1(40747432d5cfebac54d3824b6a6f26b5e7742fc1) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp07", 0, 8, CRC(4c1acdc2) SHA1(8c0b4ae95d6ba89e4bdf9fce5e2605c2728ae67f) )
ROM_END

ROM_START( tgmj )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "atej_04.2h", 0x0000000, 0x080000, CRC(bb4bbb96) SHA1(808f4b29493e74efd661d561d11cbec2f4afd1c8) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "ate-05m.3h", 0x0000000, 0x400000, CRC(50977f5a) SHA1(78c2b1965957ff1756c25b76e549f11fc0001153) )
	ROM_LOAD( "ate-06m.4h", 0x0400000, 0x400000, CRC(05973f16) SHA1(c9262e8de14c4a9489f7050316012913c1caf0ff) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ate_02.2e", 0x00000, 0x08000, CRC(f4f6e82f) SHA1(ad6c49197a60f456367c9f78353741fb847819a1) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "ate-01m.3a", 0x0000000, 0x400000, CRC(a21c6521) SHA1(560e4855f6e00def5277bdd12064b49e55c3b46b) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp11", 0, 8, CRC(22d77c59) SHA1(8e837a2f1b0c146ff835f20f4b9f6f9eb7e5ff04) )
ROM_END

ROM_START( techromn )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "kioe_04.2h", 0x0000000, 0x080000, CRC(ebd33b09) SHA1(3f0226d275efc7b97c8d3431211f948aa1271d34) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "kio-05m.3h", 0x0000000, 0x800000, CRC(98e9eb24) SHA1(144773296c213ab09d626c915f90bb74e24487f0) )
	ROM_LOAD( "kio-06m.4h", 0x0800000, 0x800000, CRC(be8d7d73) SHA1(bcbbbd0b83503f2ed32527444e0da3afd774d3f7) )
	ROM_LOAD( "kio-07m.5h", 0x1000000, 0x800000, CRC(ffd81f18) SHA1(f8387a9d45e79f97ccdffabe755638a60f80ccf5) )
	ROM_LOAD( "kio-08m.2k", 0x1800000, 0x800000, CRC(17302226) SHA1(976ba7f48c9a52d24388cd63d02be08627cf2e30) )
	ROM_LOAD( "kio-09m.3k", 0x2000000, 0x800000, CRC(a34f2119) SHA1(50fa992eba5324a173fcc0923227c13cad4f97e5) )
	ROM_LOAD( "kio-10m.4k", 0x2800000, 0x800000, CRC(7400037a) SHA1(d58641e1d6bf1c6ca04f6c98d6809edaa7df75d3) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kio_02.2e", 0x00000, 0x08000, CRC(174309b3) SHA1(b35b9c3905d2fabaa8410f70f7b382e916c89733) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "kio_03.3e", 0x28000, 0x20000, CRC(0b313ae5) SHA1(0ea39305ca30f376930e39b134fd1a52200624fa) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "kio-01m.3a", 0x0000000, 0x400000, CRC(6dc5bd07) SHA1(e1755a48465f741691ea0fa1166cb2dc09210ed9) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp09", 0, 8, CRC(02939f83) SHA1(a2a4d5218609c28da8175647cfb5114064d3265e) )
ROM_END

ROM_START( techromnu )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "kiou_04.2h", 0x0000000, 0x080000, CRC(08aca34a) SHA1(768a37f719af5d96993db5592b6505b013e0d6f4) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "kio-05m.3h", 0x0000000, 0x800000, CRC(98e9eb24) SHA1(144773296c213ab09d626c915f90bb74e24487f0) )
	ROM_LOAD( "kio-06m.4h", 0x0800000, 0x800000, CRC(be8d7d73) SHA1(bcbbbd0b83503f2ed32527444e0da3afd774d3f7) )
	ROM_LOAD( "kio-07m.5h", 0x1000000, 0x800000, CRC(ffd81f18) SHA1(f8387a9d45e79f97ccdffabe755638a60f80ccf5) )
	ROM_LOAD( "kio-08m.2k", 0x1800000, 0x800000, CRC(17302226) SHA1(976ba7f48c9a52d24388cd63d02be08627cf2e30) )
	ROM_LOAD( "kio-09m.3k", 0x2000000, 0x800000, CRC(a34f2119) SHA1(50fa992eba5324a173fcc0923227c13cad4f97e5) )
	ROM_LOAD( "kio-10m.4k", 0x2800000, 0x800000, CRC(7400037a) SHA1(d58641e1d6bf1c6ca04f6c98d6809edaa7df75d3) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kio_02.2e", 0x00000, 0x08000, CRC(174309b3) SHA1(b35b9c3905d2fabaa8410f70f7b382e916c89733) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "kio_03.3e", 0x28000, 0x20000, CRC(0b313ae5) SHA1(0ea39305ca30f376930e39b134fd1a52200624fa) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "kio-01m.3a", 0x0000000, 0x400000, CRC(6dc5bd07) SHA1(e1755a48465f741691ea0fa1166cb2dc09210ed9) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp09", 0, 8, CRC(02939f83) SHA1(a2a4d5218609c28da8175647cfb5114064d3265e) )
ROM_END

ROM_START( techromna )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "kioa_04.2h", 0x0000000, 0x080000, CRC(8ff43381) SHA1(7ce81e70fdc593b01a56e5e8b7d1de8000fe9453) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "kio-05m.3h", 0x0000000, 0x800000, CRC(98e9eb24) SHA1(144773296c213ab09d626c915f90bb74e24487f0) )
	ROM_LOAD( "kio-06m.4h", 0x0800000, 0x800000, CRC(be8d7d73) SHA1(bcbbbd0b83503f2ed32527444e0da3afd774d3f7) )
	ROM_LOAD( "kio-07m.5h", 0x1000000, 0x800000, CRC(ffd81f18) SHA1(f8387a9d45e79f97ccdffabe755638a60f80ccf5) )
	ROM_LOAD( "kio-08m.2k", 0x1800000, 0x800000, CRC(17302226) SHA1(976ba7f48c9a52d24388cd63d02be08627cf2e30) )
	ROM_LOAD( "kio-09m.3k", 0x2000000, 0x800000, CRC(a34f2119) SHA1(50fa992eba5324a173fcc0923227c13cad4f97e5) )
	ROM_LOAD( "kio-10m.4k", 0x2800000, 0x800000, CRC(7400037a) SHA1(d58641e1d6bf1c6ca04f6c98d6809edaa7df75d3) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kio_02.2e", 0x00000, 0x08000, CRC(174309b3) SHA1(b35b9c3905d2fabaa8410f70f7b382e916c89733) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "kio_03.3e", 0x28000, 0x20000, CRC(0b313ae5) SHA1(0ea39305ca30f376930e39b134fd1a52200624fa) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "kio-01m.3a", 0x0000000, 0x400000, CRC(6dc5bd07) SHA1(e1755a48465f741691ea0fa1166cb2dc09210ed9) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp09", 0, 8, CRC(02939f83) SHA1(a2a4d5218609c28da8175647cfb5114064d3265e) )
ROM_END

ROM_START( kikaioh )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "kioj_04.2h", 0x0000000, 0x080000, CRC(3a2a3bc8) SHA1(3c4ae3cfe00a7f60ab2196ae042dab4a8eb6f597) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "kio-05m.3h", 0x0000000, 0x800000, CRC(98e9eb24) SHA1(144773296c213ab09d626c915f90bb74e24487f0) )
	ROM_LOAD( "kio-06m.4h", 0x0800000, 0x800000, CRC(be8d7d73) SHA1(bcbbbd0b83503f2ed32527444e0da3afd774d3f7) )
	ROM_LOAD( "kio-07m.5h", 0x1000000, 0x800000, CRC(ffd81f18) SHA1(f8387a9d45e79f97ccdffabe755638a60f80ccf5) )
	ROM_LOAD( "kio-08m.2k", 0x1800000, 0x800000, CRC(17302226) SHA1(976ba7f48c9a52d24388cd63d02be08627cf2e30) )
	ROM_LOAD( "kio-09m.3k", 0x2000000, 0x800000, CRC(a34f2119) SHA1(50fa992eba5324a173fcc0923227c13cad4f97e5) )
	ROM_LOAD( "kio-10m.4k", 0x2800000, 0x800000, CRC(7400037a) SHA1(d58641e1d6bf1c6ca04f6c98d6809edaa7df75d3) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "kio_02.2e", 0x00000, 0x08000, CRC(174309b3) SHA1(b35b9c3905d2fabaa8410f70f7b382e916c89733) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "kio_03.3e", 0x28000, 0x20000, CRC(0b313ae5) SHA1(0ea39305ca30f376930e39b134fd1a52200624fa) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "kio-01m.3a", 0x0000000, 0x400000, CRC(6dc5bd07) SHA1(e1755a48465f741691ea0fa1166cb2dc09210ed9) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp09", 0, 8, CRC(02939f83) SHA1(a2a4d5218609c28da8175647cfb5114064d3265e) )
ROM_END

ROM_START( sfex2p )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "x2pe_04.2h", 0x0000000, 0x080000, CRC(2cbd44ff) SHA1(9f32e447b811df8a8fa8d28d11204fc8ca7f93f8) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "x2p-05m.3h", 0x0000000, 0x800000, CRC(4ee3110f) SHA1(704f8dca7d0b698659af9e3271ea5072dfd42b8b) )
	ROM_LOAD( "x2p-06m.4h", 0x0800000, 0x800000, CRC(4cd53a45) SHA1(39499ea6c9aa51c71f4fe44cc02f93d5a39e14ec) )
	ROM_LOAD( "x2p-07m.5h", 0x1000000, 0x800000, CRC(11207c2a) SHA1(0182652819f1c3a36e7b42e34ef86d2455a2dd90) )
	ROM_LOAD( "x2p-08m.2k", 0x1800000, 0x800000, CRC(3560c2cc) SHA1(8b0ce22d954387f7bb032b5220d1014ef68741e8) )
	ROM_LOAD( "x2p-09m.3k", 0x2000000, 0x800000, CRC(344aa227) SHA1(69dc6f511939bf7fa25c2531ecf307a7565fe7a8) )
	ROM_LOAD( "x2p-10m.4k", 0x2800000, 0x800000, CRC(2eef5931) SHA1(e5227529fb68eeb1b2f25813694173a75d906b52) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "x2p_02.2e", 0x00000, 0x08000, CRC(3705de5e) SHA1(847007ca271da64bf13ffbf496d4291429eee27a) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "x2p_03.3e", 0x28000, 0x20000, CRC(6ae828f6) SHA1(41c54165e87b846a845da581f408b96979288158) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "x2p-01m.3a", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp12", 0, 8, CRC(7cc2ed68) SHA1(a409ae837665700bdc4e3aa7c41a418d5b792940) )
ROM_END

ROM_START( sfex2pu )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "x2pu_04.2h", 0x0000000, 0x080000, CRC(2938118c) SHA1(4bdeeb9aa3dd54ef44aa3fc73d78d65297b1ed25) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "x2p-05m.3h", 0x0000000, 0x800000, CRC(4ee3110f) SHA1(704f8dca7d0b698659af9e3271ea5072dfd42b8b) )
	ROM_LOAD( "x2p-06m.4h", 0x0800000, 0x800000, CRC(4cd53a45) SHA1(39499ea6c9aa51c71f4fe44cc02f93d5a39e14ec) )
	ROM_LOAD( "x2p-07m.5h", 0x1000000, 0x800000, CRC(11207c2a) SHA1(0182652819f1c3a36e7b42e34ef86d2455a2dd90) )
	ROM_LOAD( "x2p-08m.2k", 0x1800000, 0x800000, CRC(3560c2cc) SHA1(8b0ce22d954387f7bb032b5220d1014ef68741e8) )
	ROM_LOAD( "x2p-09m.3k", 0x2000000, 0x800000, CRC(344aa227) SHA1(69dc6f511939bf7fa25c2531ecf307a7565fe7a8) )
	ROM_LOAD( "x2p-10m.4k", 0x2800000, 0x800000, CRC(2eef5931) SHA1(e5227529fb68eeb1b2f25813694173a75d906b52) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "x2p_02.2e", 0x00000, 0x08000, CRC(3705de5e) SHA1(847007ca271da64bf13ffbf496d4291429eee27a) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "x2p_03.3e", 0x28000, 0x20000, CRC(6ae828f6) SHA1(41c54165e87b846a845da581f408b96979288158) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "x2p-01m.3a", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp12", 0, 8, CRC(7cc2ed68) SHA1(a409ae837665700bdc4e3aa7c41a418d5b792940) )
ROM_END

ROM_START( sfex2pa )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "x2pa_04.2h", 0x0000000, 0x080000, CRC(c437d602) SHA1(150f0dfd9f2e4f9adc11f8960da1e6be250456b1) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "x2p-05m.3h", 0x0000000, 0x800000, CRC(4ee3110f) SHA1(704f8dca7d0b698659af9e3271ea5072dfd42b8b) )
	ROM_LOAD( "x2p-06m.4h", 0x0800000, 0x800000, CRC(4cd53a45) SHA1(39499ea6c9aa51c71f4fe44cc02f93d5a39e14ec) )
	ROM_LOAD( "x2p-07m.5h", 0x1000000, 0x800000, CRC(11207c2a) SHA1(0182652819f1c3a36e7b42e34ef86d2455a2dd90) )
	ROM_LOAD( "x2p-08m.2k", 0x1800000, 0x800000, CRC(3560c2cc) SHA1(8b0ce22d954387f7bb032b5220d1014ef68741e8) )
	ROM_LOAD( "x2p-09m.3k", 0x2000000, 0x800000, CRC(344aa227) SHA1(69dc6f511939bf7fa25c2531ecf307a7565fe7a8) )
	ROM_LOAD( "x2p-10m.4k", 0x2800000, 0x800000, CRC(2eef5931) SHA1(e5227529fb68eeb1b2f25813694173a75d906b52) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "x2p_02.2e", 0x00000, 0x08000, CRC(3705de5e) SHA1(847007ca271da64bf13ffbf496d4291429eee27a) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "x2p_03.3e", 0x28000, 0x20000, CRC(6ae828f6) SHA1(41c54165e87b846a845da581f408b96979288158) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "x2p-01m.3a", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp12", 0, 8, CRC(7cc2ed68) SHA1(a409ae837665700bdc4e3aa7c41a418d5b792940) )
ROM_END

ROM_START( sfex2ph )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "x2ph_04.2h", 0x0000000, 0x080000, CRC(a4f07439) SHA1(630be537062134da2d01866b9587e5c119c198bb) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "x2p-05m.3h", 0x0000000, 0x800000, CRC(4ee3110f) SHA1(704f8dca7d0b698659af9e3271ea5072dfd42b8b) )
	ROM_LOAD( "x2p-06m.4h", 0x0800000, 0x800000, CRC(4cd53a45) SHA1(39499ea6c9aa51c71f4fe44cc02f93d5a39e14ec) )
	ROM_LOAD( "x2p-07m.5h", 0x1000000, 0x800000, CRC(11207c2a) SHA1(0182652819f1c3a36e7b42e34ef86d2455a2dd90) )
	ROM_LOAD( "x2p-08m.2k", 0x1800000, 0x800000, CRC(3560c2cc) SHA1(8b0ce22d954387f7bb032b5220d1014ef68741e8) )
	ROM_LOAD( "x2p-09m.3k", 0x2000000, 0x800000, CRC(344aa227) SHA1(69dc6f511939bf7fa25c2531ecf307a7565fe7a8) )
	ROM_LOAD( "x2p-10m.4k", 0x2800000, 0x800000, CRC(2eef5931) SHA1(e5227529fb68eeb1b2f25813694173a75d906b52) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "x2p_02.2e", 0x00000, 0x08000, CRC(3705de5e) SHA1(847007ca271da64bf13ffbf496d4291429eee27a) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "x2p_03.3e", 0x28000, 0x20000, CRC(6ae828f6) SHA1(41c54165e87b846a845da581f408b96979288158) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "x2p-01m.3a", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp12", 0, 8, CRC(7cc2ed68) SHA1(a409ae837665700bdc4e3aa7c41a418d5b792940) )
ROM_END

ROM_START( sfex2pj )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "x2pj_04.2h", 0x0000000, 0x080000, CRC(c6d0aea3) SHA1(f48ee889dd743109f830063da3eb0f687db2d86c) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "x2p-05m.3h", 0x0000000, 0x800000, CRC(4ee3110f) SHA1(704f8dca7d0b698659af9e3271ea5072dfd42b8b) )
	ROM_LOAD( "x2p-06m.4h", 0x0800000, 0x800000, CRC(4cd53a45) SHA1(39499ea6c9aa51c71f4fe44cc02f93d5a39e14ec) )
	ROM_LOAD( "x2p-07m.5h", 0x1000000, 0x800000, CRC(11207c2a) SHA1(0182652819f1c3a36e7b42e34ef86d2455a2dd90) )
	ROM_LOAD( "x2p-08m.2k", 0x1800000, 0x800000, CRC(3560c2cc) SHA1(8b0ce22d954387f7bb032b5220d1014ef68741e8) )
	ROM_LOAD( "x2p-09m.3k", 0x2000000, 0x800000, CRC(344aa227) SHA1(69dc6f511939bf7fa25c2531ecf307a7565fe7a8) )
	ROM_LOAD( "x2p-10m.4k", 0x2800000, 0x800000, CRC(2eef5931) SHA1(e5227529fb68eeb1b2f25813694173a75d906b52) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "x2p_02.2e", 0x00000, 0x08000, CRC(3705de5e) SHA1(847007ca271da64bf13ffbf496d4291429eee27a) )
	ROM_CONTINUE(        0x10000, 0x18000 )
	ROM_LOAD( "x2p_03.3e", 0x28000, 0x20000, CRC(6ae828f6) SHA1(41c54165e87b846a845da581f408b96979288158) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "x2p-01m.3a", 0x0000000, 0x400000, CRC(14a5bb0e) SHA1(dfe3c3a53bd4c58743d8039b5344d3afbe2a9c24) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp12", 0, 8, CRC(7cc2ed68) SHA1(a409ae837665700bdc4e3aa7c41a418d5b792940) )
ROM_END

ROM_START( strider2 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "hr2e_04.2h", 0x0000000, 0x080000, CRC(32d89b1c) SHA1(77d4d9de768cb195d1290b89892feaef6d8705e8) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "hr2-05m.3h", 0x0000000, 0x800000, CRC(18716fe8) SHA1(bb923f18120086054cd6fd91f77d27a190c1eed4) )
	ROM_LOAD( "hr2-06m.4h", 0x0800000, 0x800000, CRC(6f13b69c) SHA1(9a14ecc72631bc44053af71fe7e3934bedf1a71e) )
	ROM_LOAD( "hr2-07m.5h", 0x1000000, 0x800000, CRC(3925701b) SHA1(d93218d2b97cc0fc6c30221bd6b5e955520fbc46) )
	ROM_LOAD( "hr2-08m.2k", 0x1800000, 0x800000, CRC(d844c0dc) SHA1(6010cfbf4dc42fda182884d78e12dcb63df00249) )
	ROM_LOAD( "hr2-09m.3k", 0x2000000, 0x800000, CRC(cdd43e6b) SHA1(346a83deadecd56428276acefc2ce95249a49921) )
	ROM_LOAD( "hr2-10m.4k", 0x2800000, 0x400000, CRC(d95b3f37) SHA1(b6566c1184718f6c0986d13060894c0fb400c201) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hr2_02.2e", 0x00000, 0x08000, CRC(acd8d385) SHA1(5edb61c3d66d2d09a28a71db52eee3a9f7db8c9d) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "hr2-01m.3a", 0x0000000, 0x200000, CRC(510a16d1) SHA1(05f10c2921a4d3b1fab4d0a4ea06351809bdbb07) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp13", 0, 8, CRC(606edf50) SHA1(db711a9da4ecaf74e0b37deb8eb94e8b8fbae041) )
ROM_END

ROM_START( strider2u )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "hr2u_04.2h", 0x0000000, 0x080000, CRC(b28b01c6) SHA1(ad40f550ce14f09cf34d51b9b0b7154c31c8936e) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "hr2-05m.3h", 0x0000000, 0x800000, CRC(18716fe8) SHA1(bb923f18120086054cd6fd91f77d27a190c1eed4) )
	ROM_LOAD( "hr2-06m.4h", 0x0800000, 0x800000, CRC(6f13b69c) SHA1(9a14ecc72631bc44053af71fe7e3934bedf1a71e) )
	ROM_LOAD( "hr2-07m.5h", 0x1000000, 0x800000, CRC(3925701b) SHA1(d93218d2b97cc0fc6c30221bd6b5e955520fbc46) )
	ROM_LOAD( "hr2-08m.2k", 0x1800000, 0x800000, CRC(d844c0dc) SHA1(6010cfbf4dc42fda182884d78e12dcb63df00249) )
	ROM_LOAD( "hr2-09m.3k", 0x2000000, 0x800000, CRC(cdd43e6b) SHA1(346a83deadecd56428276acefc2ce95249a49921) )
	ROM_LOAD( "hr2-10m.4k", 0x2800000, 0x400000, CRC(d95b3f37) SHA1(b6566c1184718f6c0986d13060894c0fb400c201) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hr2_02.2e", 0x00000, 0x08000, CRC(acd8d385) SHA1(5edb61c3d66d2d09a28a71db52eee3a9f7db8c9d) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "hr2-01m.3a", 0x0000000, 0x200000, CRC(510a16d1) SHA1(05f10c2921a4d3b1fab4d0a4ea06351809bdbb07) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp13", 0, 8, CRC(606edf50) SHA1(db711a9da4ecaf74e0b37deb8eb94e8b8fbae041) )
ROM_END

ROM_START( strider2a )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "hr2a_04.2h", 0x0000000, 0x080000, CRC(56ff9394) SHA1(fe8417965d945210ac098c6678c02f1c678bd13b) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "hr2-05m.3h", 0x0000000, 0x800000, CRC(18716fe8) SHA1(bb923f18120086054cd6fd91f77d27a190c1eed4) )
	ROM_LOAD( "hr2-06m.4h", 0x0800000, 0x800000, CRC(6f13b69c) SHA1(9a14ecc72631bc44053af71fe7e3934bedf1a71e) )
	ROM_LOAD( "hr2-07m.5h", 0x1000000, 0x800000, CRC(3925701b) SHA1(d93218d2b97cc0fc6c30221bd6b5e955520fbc46) )
	ROM_LOAD( "hr2-08m.2k", 0x1800000, 0x800000, CRC(d844c0dc) SHA1(6010cfbf4dc42fda182884d78e12dcb63df00249) )
	ROM_LOAD( "hr2-09m.3k", 0x2000000, 0x800000, CRC(cdd43e6b) SHA1(346a83deadecd56428276acefc2ce95249a49921) )
	ROM_LOAD( "hr2-10m.4k", 0x2800000, 0x400000, CRC(d95b3f37) SHA1(b6566c1184718f6c0986d13060894c0fb400c201) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hr2_02.2e", 0x00000, 0x08000, CRC(acd8d385) SHA1(5edb61c3d66d2d09a28a71db52eee3a9f7db8c9d) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "hr2-01m.3a", 0x0000000, 0x200000, CRC(510a16d1) SHA1(05f10c2921a4d3b1fab4d0a4ea06351809bdbb07) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp13", 0, 8, CRC(606edf50) SHA1(db711a9da4ecaf74e0b37deb8eb94e8b8fbae041) )
ROM_END

ROM_START( shiryu2 )
	CPZN2_BIOS

	ROM_REGION32_LE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD( "hr2j_04.2h", 0x0000000, 0x080000, CRC(0824ee5f) SHA1(a296ffe03f0d947deb9803d05de3c240a26b52bb) )

	ROM_REGION32_LE( 0x3000000, REGION_USER2, 0 )
	ROM_LOAD( "hr2-05m.3h", 0x0000000, 0x800000, CRC(18716fe8) SHA1(bb923f18120086054cd6fd91f77d27a190c1eed4) )
	ROM_LOAD( "hr2-06m.4h", 0x0800000, 0x800000, CRC(6f13b69c) SHA1(9a14ecc72631bc44053af71fe7e3934bedf1a71e) )
	ROM_LOAD( "hr2-07m.5h", 0x1000000, 0x800000, CRC(3925701b) SHA1(d93218d2b97cc0fc6c30221bd6b5e955520fbc46) )
	ROM_LOAD( "hr2-08m.2k", 0x1800000, 0x800000, CRC(d844c0dc) SHA1(6010cfbf4dc42fda182884d78e12dcb63df00249) )
	ROM_LOAD( "hr2-09m.3k", 0x2000000, 0x800000, CRC(cdd43e6b) SHA1(346a83deadecd56428276acefc2ce95249a49921) )
	ROM_LOAD( "hr2-10m.4k", 0x2800000, 0x400000, CRC(d95b3f37) SHA1(b6566c1184718f6c0986d13060894c0fb400c201) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "hr2_02.2e", 0x00000, 0x08000, CRC(acd8d385) SHA1(5edb61c3d66d2d09a28a71db52eee3a9f7db8c9d) )
	ROM_CONTINUE(        0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Q Sound Samples */
	ROM_LOAD16_WORD_SWAP( "hr2-01m.3a", 0x0000000, 0x200000, CRC(510a16d1) SHA1(05f10c2921a4d3b1fab4d0a4ea06351809bdbb07) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "cp13", 0, 8, CRC(606edf50) SHA1(db711a9da4ecaf74e0b37deb8eb94e8b8fbae041) )
ROM_END

/* Tecmo */

#define TPS_BIOS \
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "m534002c-61.ic353", 0x0000000, 0x080000, CRC(69ffbcb4) SHA1(03eb2febfab3fcde716defff291babd9392de965) ) \
	ROM_REGION( 0x000008, REGION_USER4, 0 ) /* CAT702, main board */ \
	ROM_LOAD( "mg01.ic652", 0, 8, CRC(50dc8322) SHA1(cef4011c6c06d6fce546e1db5d9fbae155eabf57) )

ROM_START( coh1002m )
	TPS_BIOS
ROM_END

/*

 TPS ROM addressing note: .216 *if present* goes at 0x400000, else nothing there.
 .217 goes at 0x800000, .218 at 0xc00000, .219 at 0x1000000, and so on.

*/

ROM_START( glpracr2 )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "gallop2u.119", 0x0000001, 0x100000, CRC(9899911c) SHA1(f043fb97760c53422ad6aeb214474c0be00017ce) )
	ROM_LOAD16_BYTE( "gallop2u.120", 0x0000000, 0x100000, CRC(fd69bd4b) SHA1(26a183bdc3b2fb3d93bd7694e429a676106f4e58) )
	ROM_LOAD( "gra2-0.217",          0x0800000, 0x400000, CRC(a077ffa3) SHA1(73492ec2145246276bfe25b27d7de4f6393124f4) )
	ROM_LOAD( "gra2-1.218",          0x0c00000, 0x400000, CRC(28ce033c) SHA1(4dc53e5c82fde683efd72c66b397d56aa72d52b9) )
	ROM_LOAD( "gra2-2.219",          0x1000000, 0x400000, CRC(0c9cb7da) SHA1(af23c11e69428413ff4d1c2746adb786de927cb5) )
	ROM_LOAD( "gra2-3.220",          0x1400000, 0x400000, CRC(264e3a0c) SHA1(c1509b16d7192b9f61dbceb299290239219adefd) )
	ROM_LOAD( "gra2-4.221",          0x1800000, 0x400000, CRC(056bf022) SHA1(f73152268e09249bad2ffd4df5cfb4f0f4a494ae) )
	ROM_LOAD( "gra2-5.222",          0x1c00000, 0x400000, CRC(3eb97009) SHA1(e3ed4bdb0dcf581dc05b19306a3db0b989d00f75) )
	ROM_LOAD( "gra2-6.223",          0x2000000, 0x400000, CRC(8c6b4c4c) SHA1(0053f736dcd437c01da8cadd820e8af658ce6077) )
	ROM_LOAD( "gra2-7.323",          0x2400000, 0x400000, CRC(7dfb6c54) SHA1(6e9a9a4172f957ba354ddd82c30735a56c5934b1) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg02", 0, 8, CRC(ce1d5558) SHA1(1d704636b14bb11889c06b344043555803ad6e08) )
ROM_END

ROM_START( glpracr2j )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "1.119",        0x0000001, 0x100000, CRC(0fe2d2df) SHA1(031369f4e1138e2ee293c321e5ee418e560b3f06) )
	ROM_LOAD16_BYTE( "2.120",        0x0000000, 0x100000, CRC(8e3fb1c0) SHA1(2126c1e43bee7cd938e0f2a3ea841da8811223cd) )
	ROM_LOAD( "gra2-0.217",          0x0800000, 0x400000, CRC(a077ffa3) SHA1(73492ec2145246276bfe25b27d7de4f6393124f4) )
	ROM_LOAD( "gra2-1.218",          0x0c00000, 0x400000, CRC(28ce033c) SHA1(4dc53e5c82fde683efd72c66b397d56aa72d52b9) )
	ROM_LOAD( "gra2-2.219",          0x1000000, 0x400000, CRC(0c9cb7da) SHA1(af23c11e69428413ff4d1c2746adb786de927cb5) )
	ROM_LOAD( "gra2-3.220",          0x1400000, 0x400000, CRC(264e3a0c) SHA1(c1509b16d7192b9f61dbceb299290239219adefd) )
	ROM_LOAD( "gra2-4.221",          0x1800000, 0x400000, CRC(056bf022) SHA1(f73152268e09249bad2ffd4df5cfb4f0f4a494ae) )
	ROM_LOAD( "gra2-5.222",          0x1c00000, 0x400000, CRC(3eb97009) SHA1(e3ed4bdb0dcf581dc05b19306a3db0b989d00f75) )
	ROM_LOAD( "gra2-6.223",          0x2000000, 0x400000, CRC(8c6b4c4c) SHA1(0053f736dcd437c01da8cadd820e8af658ce6077) )
	ROM_LOAD( "gra2-7.323",          0x2400000, 0x400000, CRC(7dfb6c54) SHA1(6e9a9a4172f957ba354ddd82c30735a56c5934b1) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg02", 0, 8, CRC(ce1d5558) SHA1(1d704636b14bb11889c06b344043555803ad6e08) )
ROM_END

ROM_START( glpracr2l )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "gra2b.119",    0x0000001, 0x100000, CRC(43abee7c) SHA1(ea0afc820d8480c12c9af54057877ff11a8012fb) )
	ROM_LOAD16_BYTE( "gra2a.120",    0x0000000, 0x100000, CRC(f60096d4) SHA1(5349d780d41a5711b483cd7eb66cd4e496b4fbe4) )
	ROM_LOAD( "gra2-0.217",          0x0800000, 0x400000, CRC(a077ffa3) SHA1(73492ec2145246276bfe25b27d7de4f6393124f4) )
	ROM_LOAD( "gra2-1.218",          0x0c00000, 0x400000, CRC(28ce033c) SHA1(4dc53e5c82fde683efd72c66b397d56aa72d52b9) )
	ROM_LOAD( "gra2-2.219",          0x1000000, 0x400000, CRC(0c9cb7da) SHA1(af23c11e69428413ff4d1c2746adb786de927cb5) )
	ROM_LOAD( "gra2-3.220",          0x1400000, 0x400000, CRC(264e3a0c) SHA1(c1509b16d7192b9f61dbceb299290239219adefd) )
	ROM_LOAD( "gra2-4.221",          0x1800000, 0x400000, CRC(056bf022) SHA1(f73152268e09249bad2ffd4df5cfb4f0f4a494ae) )
	ROM_LOAD( "gra2-5.222",          0x1c00000, 0x400000, CRC(3eb97009) SHA1(e3ed4bdb0dcf581dc05b19306a3db0b989d00f75) )
	ROM_LOAD( "gra2-6.223",          0x2000000, 0x400000, CRC(8c6b4c4c) SHA1(0053f736dcd437c01da8cadd820e8af658ce6077) )
	ROM_LOAD( "gra2-7.323",          0x2400000, 0x400000, CRC(7dfb6c54) SHA1(6e9a9a4172f957ba354ddd82c30735a56c5934b1) )

	ROM_REGION( 0x040000, REGION_USER7, 0 )
	ROM_LOAD( "link3118.bin", 0x0000000, 0x040000, CRC(a4d4761e) SHA1(3fb25dfa5220d25093588d9501e0666214491100) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg02", 0, 8, CRC(ce1d5558) SHA1(1d704636b14bb11889c06b344043555803ad6e08) )
ROM_END

ROM_START( cbaj )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "cbaj_1.119",   0x0000001, 0x080000, CRC(814f8b4b) SHA1(17966038a692d0701139660f25725d7c10a2a928) )
	ROM_LOAD16_BYTE( "cbaj_2.120",   0x0000000, 0x080000, CRC(89286229) SHA1(18a84ef648ec3b79707eb42b55563adf38dffd0d) )
	ROM_LOAD( "cb-00.216",           0x0400000, 0x400000, CRC(3db68bea) SHA1(77ab334e0c02e608b11d8fdb9505b2301f6f9afb) )
	ROM_LOAD( "cb-01.217",           0x0800000, 0x400000, CRC(481040bc) SHA1(c6fe575b77d1eb5f613691dec5ed08929b72b955) )
	ROM_LOAD( "cb-02.218",           0x0c00000, 0x400000, CRC(858f116c) SHA1(e3546862d367d2fe88913fea3185b23bc6a9777d) )
	ROM_LOAD( "cb-03.219",           0x1000000, 0x400000, CRC(3576ea2a) SHA1(a5ee7bb9f4650e99ee067eb1cc28c62d9099a6cf) )
	ROM_LOAD( "cb-04.220",           0x1400000, 0x400000, CRC(551c4b29) SHA1(c3f8508a006b475491c9ea20eb64c3bea6b35afb) )
	ROM_LOAD( "cb-05.221",           0x1800000, 0x400000, CRC(7da453da) SHA1(85b2c93b9453e8c7791b530b7e036e4ef6abc077) )
	ROM_LOAD( "cb-06.222",           0x1c00000, 0x400000, CRC(833cb18b) SHA1(dbc390e1dbf3e7815eb3d170c0890d3785d8002c) )
	ROM_LOAD( "cb-07.223",           0x2000000, 0x400000, CRC(3b64ce9e) SHA1(a137da126295736bb7643655d52bd570004e87fd) )
	ROM_LOAD( "cb-08.323",           0x2400000, 0x400000, CRC(57cc482e) SHA1(603c3d13a6cd796c209a97aa7e63b77bdbf71580) )

	ROM_REGION( 0x040000, REGION_CPU2, 0 )
	ROM_LOAD( "cbaj_z80.3118",       0x0000000, 0x040000, CRC(92b02ad2) SHA1(f72317679ecbd8a0c3b081baaf9ff20a8c9ec00f) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* YMZ280B Sound Samples */
	ROM_LOAD( "cb-se.5121",   0x400000, 0x400000, CRC(f12b3db9) SHA1(d5231ad664603050bdca2081b114b07fc905ddc2) )
	ROM_LOAD( "cb-vo.5120",   0x000000, 0x400000, CRC(afb05d6d) SHA1(0c08010579813814fbf8a978cf4376bab18697a4) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg03", 0, 8, CRC(8be79633) SHA1(f38f881b6139eb8368e593904cb50b6e68bdf2e9) )
ROM_END

ROM_START( cbajbl )
	TPS_BIOS

	ROM_REGION32_LE( 0x2800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "uo119.bin", 0x0000001, 0x080000, CRC(814f8b4b) SHA1(17966038a692d0701139660f25725d7c10a2a928) )
	ROM_LOAD16_BYTE( "uo120.bin", 0x0000000, 0x080000, CRC(89286229) SHA1(18a84ef648ec3b79707eb42b55563adf38dffd0d) )
	ROM_LOAD( "uo216.bin", 0x0400000, 0x200000, CRC(1b18e0e9) SHA1(306b4849f707c5b643992e3880a34b894fc9f0e1) )
	ROM_LOAD( "uo216h.bin", 0x0600000, 0x200000, CRC(bb86ee76) SHA1(9eead295d8fec3708c1e693d3d58346cc214580c) )
	ROM_LOAD( "uo217.bin", 0x0800000, 0x200000, CRC(02d8dfcd) SHA1(b83636374fcae6aa1c3a320fafcc7bd7890c0bc6) )
	ROM_LOAD( "uo217h.bin", 0x0a00000, 0x200000, CRC(6e0f34da) SHA1(e7384a3019dcc0f69335635056ae3fac89fb4b52) )
	ROM_LOAD( "uo218.bin", 0x0c00000, 0x200000, CRC(14eaaff6) SHA1(b38f541b9f0397edee071d883b2801fa40cba127) )
	ROM_LOAD( "uo218h.bin", 0x0e00000, 0x200000, CRC(9f6420dc) SHA1(3b354bdfc54ac5d69c229d897cabc807c21c61bc) )
	ROM_LOAD( "uo219.bin", 0x1000000, 0x200000, CRC(89e727b1) SHA1(09e69708c5b8244dbacd09dce6558cf917a58623) )
	ROM_LOAD( "uo219h.bin", 0x1200000, 0x200000, BAD_DUMP CRC(727b7ae5) SHA1(182b63b77965a25591bda71575af464f6e3b26f8) )
	ROM_LOAD( "uo220.bin", 0x1400000, 0x200000, BAD_DUMP CRC(19102f15) SHA1(b7964e0cb53b608b775d7a713b63f13307b1b532) )
	ROM_LOAD( "uo220h.bin", 0x1600000, 0x200000, BAD_DUMP CRC(e7b94df3) SHA1(07b5d85e1a337f10c25cebdb0f3f893e38a1db6c) )
	ROM_LOAD( "uo221.bin", 0x1800000, 0x200000, CRC(701d7c4e) SHA1(b06c2db7e06d453991f9872bcb62951eb62b5d56) )
	ROM_LOAD( "uo221h.bin", 0x1a00000, 0x200000, CRC(5e93c0d6) SHA1(2ec5afd53eee9e8e6e2ac91a269a5ec55cef4da1) )
	ROM_LOAD( "uo222.bin", 0x1c00000, 0x200000, CRC(76ad7801) SHA1(9cb38aff86f3f8151145f2661bf1261adbd2ec1c) )
	ROM_LOAD( "uo222h.bin", 0x1e00000, 0x200000, CRC(34561670) SHA1(2131fcbbc8bc3da31cd4ef027ec6688af212b7c1) )
	ROM_LOAD( "uo223.bin", 0x2000000, 0x200000, CRC(d4a80392) SHA1(d8d69e8a15910471b718dc6531e4470fc35365cc) )
	ROM_LOAD( "uo223h.bin", 0x2200000, 0x200000, CRC(eb5b6b3a) SHA1(ad595053b852a45b85f061dfa960fa04cf74eb9f) )
	ROM_LOAD( "uo323.bin", 0x2400000, 0x200000, CRC(e9140f6c) SHA1(a9e8c3b94de319a5fd5327111339c76c65371ba9) )

	ROM_REGION( 0x040000, REGION_CPU2, 0 )
	ROM_LOAD( "near_z80.bin", 0x000000, 0x020000, CRC(10b30c5b) SHA1(8c4af71dc14c43a2b32377737b6dabc393b3c141) )
	ROM_COPY( REGION_CPU2, 0x000000, 0x020000, 0x020000 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* YMZ280B Sound Samples */
	ROM_LOAD( "uo5120.bin",  0x000000, 0x200000, CRC(973bc8fe) SHA1(ab37fdebff5daee469f51838a6f498541c1c9e07) )
	ROM_LOAD( "uo5120h.bin", 0x200000, 0x200000, CRC(01bede31) SHA1(ed7a2eaabe21127b6440cc20d3089a880ce428d5) )
	ROM_LOAD( "uo5121.bin",  0x400000, 0x200000, CRC(76112372) SHA1(f69860fbffd33aee6cdf475c091987467bca274a) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg03", 0, 8, CRC(8be79633) SHA1(f38f881b6139eb8368e593904cb50b6e68bdf2e9) )
ROM_END

ROM_START( shngmtkb )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "shmj-b.119",   0x0000001, 0x080000, CRC(65522c67) SHA1(b5981e5859aab742a87d6742feb9c55a3e6ba13f) )
	ROM_LOAD16_BYTE( "shmj-a.120",   0x0000000, 0x080000, CRC(a789defa) SHA1(f8f0d1c9e3492cda652a9561ef1d549b92f73efd) )
	ROM_LOAD( "sh-00.217",           0x0800000, 0x400000, CRC(081fed1c) SHA1(fb18add9521b8b104329871b4c1b8ae5e0254f8b) )
	ROM_LOAD( "sh-01.218",           0x0c00000, 0x400000, CRC(5a84ea96) SHA1(af4972cc10706999361d7505b975f5f1e1fc6761) )
	ROM_LOAD( "sh-02.219",           0x1000000, 0x400000, CRC(c8f80d76) SHA1(51e4eac6cec8e37e5b8c0e7d341feea574add7da) )
	ROM_LOAD( "sh-03.220",           0x1400000, 0x400000, CRC(daaa4c73) SHA1(eb31d4cadd9eba3d3431f3f6ef880bb2effa0b9f) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg04", 0, 8, CRC(2711a75f) SHA1(400cd6f28826ba0ad1125eda21507e5ebf5caccf) )
ROM_END

ROM_START( doapp )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "doapp119.bin", 0x0000001, 0x100000, CRC(bbe04cef) SHA1(f2dae4810ca78075fc3007a6001531a455235a2e) )
	ROM_LOAD16_BYTE( "doapp120.bin", 0x0000000, 0x100000, CRC(b614d7e6) SHA1(373756d9b88b45c677e987ee1e5cb2d5446ecfe8) )
	ROM_LOAD( "doapp-0.216",         0x0400000, 0x400000, CRC(acc6c539) SHA1(a744567a3d75634098b1749103307981be9acbdd) )
	ROM_LOAD( "doapp-1.217",         0x0800000, 0x400000, CRC(14b961c4) SHA1(3fae1fcb4665ba8bad391881b26c2d087718d42f) )
	ROM_LOAD( "doapp-2.218",         0x0c00000, 0x400000, CRC(134f698f) SHA1(6422972cf5d30a0f09f0c20f042691d5969207b4) )
	ROM_LOAD( "doapp-3.219",         0x1000000, 0x400000, CRC(1c6540f3) SHA1(8631fde93a1da6325d7b31c7edf12c964f0ac4fc) )
	ROM_LOAD( "doapp-4.220",         0x1400000, 0x400000, CRC(f83bacf7) SHA1(5bd66da993f0db966581dde80dd7e5b377754412) )
	ROM_LOAD( "doapp-5.221",         0x1800000, 0x400000, CRC(e11e8b71) SHA1(b1d1b9532b5f074ce216a603436d5674d136865d) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg05", 0, 8, CRC(5748a4ca) SHA1(c88d73f6a646a9ddefdfd84cba70d591759c069f) )
ROM_END

ROM_START( doappk )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "doapp_u0119.119", 0x0000001, 0x100000, CRC(9084704e) SHA1(616b3e65a32768767209ae77c48ad34d11b31754) )
	ROM_LOAD16_BYTE( "doapp_u0120.120", 0x0000000, 0x100000, CRC(720a6983) SHA1(d38783110440ffa00036b86f97bff5edb6a5673f) )
	ROM_LOAD( "doapp-0.216",         0x0400000, 0x400000, CRC(acc6c539) SHA1(a744567a3d75634098b1749103307981be9acbdd) )
	ROM_LOAD( "doapp-1.217",         0x0800000, 0x400000, CRC(14b961c4) SHA1(3fae1fcb4665ba8bad391881b26c2d087718d42f) )
	ROM_LOAD( "doapp-2.218",         0x0c00000, 0x400000, CRC(134f698f) SHA1(6422972cf5d30a0f09f0c20f042691d5969207b4) )
	ROM_LOAD( "doapp-3.219",         0x1000000, 0x400000, CRC(1c6540f3) SHA1(8631fde93a1da6325d7b31c7edf12c964f0ac4fc) )
	ROM_LOAD( "doapp-4.220",         0x1400000, 0x400000, CRC(f83bacf7) SHA1(5bd66da993f0db966581dde80dd7e5b377754412) )
	ROM_LOAD( "doapp-5.221",         0x1800000, 0x400000, CRC(e11e8b71) SHA1(b1d1b9532b5f074ce216a603436d5674d136865d) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg05", 0, 8, CRC(5748a4ca) SHA1(c88d73f6a646a9ddefdfd84cba70d591759c069f) )
ROM_END

ROM_START( tondemo )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "u0119.bin",    0x0000001, 0x100000, CRC(5711e301) SHA1(005375d32c1eda9bd39e46326880a62506d06389) )
	ROM_LOAD16_BYTE( "u0120.bin",    0x0000000, 0x100000, CRC(0b8312c6) SHA1(93e0e4b796cc953daf7ed2ff2f327aed07cf833a) )
	ROM_LOAD( "tca-0.217",           0x0800000, 0x400000, CRC(ef175910) SHA1(b77aa9016804172d433d97d5fdc242a1361e941c) )
	ROM_LOAD( "tca-1.218",           0x0c00000, 0x400000, CRC(c3474e8a) SHA1(46dd0ae7cd2e54c639fe39d6965ef71ce6a1b921) )
	ROM_LOAD( "tca-2.219",           0x1000000, 0x400000, CRC(89b8e1a8) SHA1(70c5f0f2d0a7869e29b62b32fa485f941b683678) )
	ROM_LOAD( "tca-3.220",           0x1400000, 0x400000, CRC(4fcf8032) SHA1(3ea815548c3bda32b1d4e88454c29e5025431b1c) )
	ROM_LOAD( "tca-4.221",           0x1800000, 0x400000, CRC(c9e23f25) SHA1(145d4e7f0cb67d2552559ce90305a56802a253f9) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg09", 0, 8, CRC(bb4a157c) SHA1(8ea729c3c2694bee12292b9654b86ff6a9e45f94) )
ROM_END

ROM_START( glpracr3j )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "gra3u119.119", 0x0000001, 0x100000, CRC(aba69017) SHA1(670b895ee7d36bc5a00f6b0df7ce965517986617) )
	ROM_LOAD16_BYTE( "gra3u120.120", 0x0000000, 0x100000, CRC(8aa98d99) SHA1(9dc1ba89e37a5c2955ee027e4e5aa0ae71e09f9b) )
	ROM_LOAD( "gra3-0.216",          0x0400000, 0x400000, CRC(b405ee65) SHA1(8ba9872e4c166e3b659a2802554bf1e964f64620) )
	ROM_LOAD( "gra3-1.217",          0x0800000, 0x400000, CRC(a06f05ac) SHA1(ade224533d75c64cb188b78bdca908c1fa882492) )
	ROM_LOAD( "gra3-2.218",          0x0c00000, 0x400000, CRC(31793f9b) SHA1(310c2dff84d17c9ed7f59e249b22e9394edcb444) )
	ROM_LOAD( "gra3-3.219",          0x1000000, 0x400000, CRC(d59fb3eb) SHA1(2db2cc1d4884d54c415531053319f2b2ad65361f) )
	ROM_LOAD( "gra3-4.220",          0x1400000, 0x400000, CRC(59a0a105) SHA1(6a585c9eaa8d9b5dad798d9d28d73f04bc838114) )
	ROM_LOAD( "gra3-5.221",          0x1800000, 0x400000, CRC(4994fb17) SHA1(59b3e6c333e55ca8b6b4b00cd52b51e3e59a5657) )
	ROM_LOAD( "gra3-6.222",          0x1c00000, 0x400000, CRC(1362c1af) SHA1(eae5b3cb11d361b3aa3f572e49800c0b2e3544ca) )
	ROM_LOAD( "gra3-7.223",          0x2000000, 0x400000, CRC(73565e1f) SHA1(74311ee94e3abc8428b4a8b1c6c3dacd883b5646) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg08", 0, 8, CRC(679367fe) SHA1(495b03e1cdad9d6aaf509b73d837340e2b1bb23b) )
ROM_END

ROM_START( glpracr3 )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "1.119", 0x0000001, 0x100000, CRC(89bdf567) SHA1(916accbcad52e9ee4e3b28a339138fe2bfbecdfe) )
	ROM_LOAD16_BYTE( "2.120", 0x0000000, 0x100000, CRC(042273fb) SHA1(eb98c4e74f385ddc6545b9250df5858b39fe361d) )
	ROM_LOAD( "gra3-0.216",          0x0400000, 0x400000, CRC(b405ee65) SHA1(8ba9872e4c166e3b659a2802554bf1e964f64620) )
	ROM_LOAD( "gra3-1.217",          0x0800000, 0x400000, CRC(a06f05ac) SHA1(ade224533d75c64cb188b78bdca908c1fa882492) )
	ROM_LOAD( "gra3-2.218",          0x0c00000, 0x400000, CRC(31793f9b) SHA1(310c2dff84d17c9ed7f59e249b22e9394edcb444) )
	ROM_LOAD( "gra3-3.219",          0x1000000, 0x400000, CRC(d59fb3eb) SHA1(2db2cc1d4884d54c415531053319f2b2ad65361f) )
	ROM_LOAD( "gra3-4.220",          0x1400000, 0x400000, CRC(59a0a105) SHA1(6a585c9eaa8d9b5dad798d9d28d73f04bc838114) )
	ROM_LOAD( "gra3-5.221",          0x1800000, 0x400000, CRC(4994fb17) SHA1(59b3e6c333e55ca8b6b4b00cd52b51e3e59a5657) )
	ROM_LOAD( "gra3-6.222",          0x1c00000, 0x400000, CRC(1362c1af) SHA1(eae5b3cb11d361b3aa3f572e49800c0b2e3544ca) )
	ROM_LOAD( "gra3-7.223",          0x2000000, 0x400000, CRC(73565e1f) SHA1(74311ee94e3abc8428b4a8b1c6c3dacd883b5646) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg08", 0, 8, CRC(679367fe) SHA1(495b03e1cdad9d6aaf509b73d837340e2b1bb23b) )
ROM_END

ROM_START( twcupmil )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "twm-ep.119",  0x0000001, 0x100000, CRC(5f2908fb) SHA1(fc7ac1f6e81543678705e6e510dbf786b1502444) )
	ROM_LOAD16_BYTE( "twm-ep.120",  0x0000000, 0x100000, CRC(1a0ef17a) SHA1(bb7123610d3791c08577b87c8be59a0dd2cc33f1) )
	ROM_LOAD( "twm-0.216",          0x0400000, 0x400000, CRC(39cbc56a) SHA1(931d0d729620ef20e5c4fd521bce45cdb1742127) )
	ROM_LOAD( "twm-1.217",          0x0800000, 0x400000, CRC(fae0687a) SHA1(383a86f55441be287075af046ebac6a5ab54e6cf) )
	ROM_LOAD( "twm-2.218",          0x0c00000, 0x400000, CRC(cb852264) SHA1(a7a2f3d6f723ddd80c57ac63522a1a0bf526a7b3) )
	ROM_LOAD( "twm-3.219",          0x1000000, 0x400000, CRC(7c9f6925) SHA1(32519a238810d02181eaf5c2344334c523fa77d1) )
	ROM_LOAD( "twm-4.220",          0x1400000, 0x400000, CRC(17cd0ec9) SHA1(37581530e974af692ab71471d0238801cd19c843) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg13", 0, 8, CRC(5778418e) SHA1(7c4d6922bbe726e0f25297f8528e94b41023581b) )
ROM_END

ROM_START( 1on1gov )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "1on1.u119",  0x0000001, 0x100000, CRC(10aecc19) SHA1(ad2fe6011551935907568cc3b4028f481034537c) )
	ROM_LOAD16_BYTE( "1on1.u120",  0x0000000, 0x100000, CRC(eea158bd) SHA1(2b2a56fcce46557201bbbab7b170ee64549ddafe) )
	ROM_LOAD( "oog-0.u0217",          0x0800000, 0x400000, CRC(8b42f365) SHA1(6035a370f477f0f33894f642717fa0b012540d36) )
	ROM_LOAD( "oog-1.u0218",          0x0c00000, 0x400000, CRC(65162f46) SHA1(db420a2f0d996b32cd4b6e9352d46a36fa31eaaa) )
	ROM_LOAD( "oog-2.u0219",          0x1000000, 0x400000, CRC(14cf3a84) SHA1(60175a1fb2c765e4c3d0e30e77961f84cfa8485c) )
	ROM_LOAD( "oog-3.u0220",          0x1400000, 0x400000, CRC(9a45f6ff) SHA1(e0ee90c545c821bf1d6b4709b1e40f93314c51a6) )
	ROM_LOAD( "oog-4.u0221",          0x1800000, 0x400000, CRC(ba20a1fd) SHA1(7893f50de730624b8447f39fc7e25e4e334df845) )
	ROM_LOAD( "oog-5.u0222",          0x1c00000, 0x400000, CRC(eed1953d) SHA1(8d3e738a07b9c6b6ca55be7b47444b5e3725065c) )
	ROM_LOAD( "oog-6.u0223",          0x2000000, 0x400000, CRC(f74f38b6) SHA1(ff7f0ebff85fc982f8d1c13d6649d4c7c5da6c45) )
	ROM_LOAD( "oog-7.u0323",          0x2400000, 0x400000, CRC(0e58777c) SHA1(9f8ee3c6d6d8b1482522500e18217577056d8c98) )

	ROM_REGION( 0x000800, REGION_USER6, 0 ) /* factory EEPROM */
	ROM_LOAD( "at28c16", 0, 0x800, CRC(fe992f29) SHA1(73c9c4d40abd8f3a95d4eb20f3c65f3a5cdd1203) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg10", 0, 8, CRC(8a73da40) SHA1(d6803a4c1d240dec2a4d8ac43aa6d7e9df947026) )
ROM_END

ROM_START( flamegun )
	TPS_BIOS

	ROM_REGION32_LE( 0x03800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "flamegun.119",   0x0000001, 0x100000, CRC(bc8e9e29) SHA1(02e4f079f0ed864dbc056d5f64d33a0522c034fd) )
	ROM_LOAD16_BYTE( "flamegun.120",   0x0000000, 0x100000, CRC(387f3070) SHA1(6b12765f1d3ec5f3d1cdfd961fba72a319d65ff4) )
	ROM_LOAD( "fg00.216",              0x0400000, 0x400000, CRC(f33736ca) SHA1(7b18f9fef1df913b7ed3a2c97e9c4925790d86c5) )
	ROM_LOAD( "fg01.217",              0x0800000, 0x400000, CRC(8980ff44) SHA1(f677bf5d279ad7731730b7e36ebf33d554903ce6) )
	ROM_LOAD( "fg02.218",              0x0c00000, 0x400000, CRC(97d1b032) SHA1(1d23cd40ced002ae1fe2fb009d2f31d8612b125a) )
	ROM_LOAD( "fg03.219",              0x1000000, 0x400000, CRC(cf84508c) SHA1(2a1173e3751f5a8b2400219a75f23d2450a0ebd6) )
	ROM_LOAD( "fg04.220",              0x1400000, 0x400000, CRC(5cc333fa) SHA1(fd81e811ef2026b245e65c104d24ae1679baa0f5) )
	ROM_LOAD( "fg05.221",              0x1800000, 0x400000, CRC(9490bc1b) SHA1(c5ea133de0a271793601f2701267d3ca82781f60) )
	ROM_LOAD( "fg06.222",              0x1c00000, 0x400000, CRC(8c29b754) SHA1(304086196321b7d9748456ed1af8965ac6192942) )
	ROM_LOAD( "fg07.223",              0x2000000, 0x400000, CRC(f62cefe8) SHA1(a59a3a41258e8474f0aeb0e3b8c2f73caf47ece4) )
	ROM_LOAD( "fg08.323",              0x2400000, 0x400000, CRC(855959b4) SHA1(994b7c8bd883a41470791802d5a07a98a553096e) )
	ROM_LOAD( "fg09.324",              0x2800000, 0x400000, CRC(82f129b4) SHA1(c0dcbc908c12f7cecbb051a671649edd20bac32c) )
	ROM_LOAD( "fg0a.325",              0x2c00000, 0x400000, CRC(f8d2b20c) SHA1(d0c9e413d72772ab8710e217d228e001b28667c8) )
	ROM_LOAD( "fb0b.326",              0x3000000, 0x400000, CRC(ad78ec79) SHA1(7e37a90a64c70f2c0024eaf30e1e4e028c0d858e) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg06", 0, 8, CRC(a10556fe) SHA1(5c2ecbb9319517137c0bbce1e38e75c6aa5e8b6f) )
ROM_END

ROM_START( flamegunj )
	TPS_BIOS

	ROM_REGION32_LE( 0x03800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "fg_1.119",       0x0000001, 0x100000, CRC(1f2aa527) SHA1(de3a20d9aeb745fe82cd1d87bde26876e088483a) )
	ROM_LOAD16_BYTE( "fg_2.120",       0x0000000, 0x100000, CRC(a2cd4cad) SHA1(bf542eeb6e768b3e86bacdc79ab04be394ce3e63) )
	ROM_LOAD( "fg00.216",              0x0400000, 0x400000, CRC(f33736ca) SHA1(7b18f9fef1df913b7ed3a2c97e9c4925790d86c5) )
	ROM_LOAD( "fg01.217",              0x0800000, 0x400000, CRC(8980ff44) SHA1(f677bf5d279ad7731730b7e36ebf33d554903ce6) )
	ROM_LOAD( "fg02.218",              0x0c00000, 0x400000, CRC(97d1b032) SHA1(1d23cd40ced002ae1fe2fb009d2f31d8612b125a) )
	ROM_LOAD( "fg03.219",              0x1000000, 0x400000, CRC(cf84508c) SHA1(2a1173e3751f5a8b2400219a75f23d2450a0ebd6) )
	ROM_LOAD( "fg04.220",              0x1400000, 0x400000, CRC(5cc333fa) SHA1(fd81e811ef2026b245e65c104d24ae1679baa0f5) )
	ROM_LOAD( "fg05.221",              0x1800000, 0x400000, CRC(9490bc1b) SHA1(c5ea133de0a271793601f2701267d3ca82781f60) )
	ROM_LOAD( "fg06.222",              0x1c00000, 0x400000, CRC(8c29b754) SHA1(304086196321b7d9748456ed1af8965ac6192942) )
	ROM_LOAD( "fg07.223",              0x2000000, 0x400000, CRC(f62cefe8) SHA1(a59a3a41258e8474f0aeb0e3b8c2f73caf47ece4) )
	ROM_LOAD( "fg08.323",              0x2400000, 0x400000, CRC(855959b4) SHA1(994b7c8bd883a41470791802d5a07a98a553096e) )
	ROM_LOAD( "fg09.324",              0x2800000, 0x400000, CRC(82f129b4) SHA1(c0dcbc908c12f7cecbb051a671649edd20bac32c) )
	ROM_LOAD( "fg0a.325",              0x2c00000, 0x400000, CRC(f8d2b20c) SHA1(d0c9e413d72772ab8710e217d228e001b28667c8) )
	ROM_LOAD( "fb0b.326",              0x3000000, 0x400000, CRC(ad78ec79) SHA1(7e37a90a64c70f2c0024eaf30e1e4e028c0d858e) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg06", 0, 8, CRC(a10556fe) SHA1(5c2ecbb9319517137c0bbce1e38e75c6aa5e8b6f) )
ROM_END

ROM_START( lpadv )
	TPS_BIOS

	ROM_REGION32_LE( 0x1000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "lp_3.u0119", 0x000001, 0x100000, CRC(18cade44) SHA1(8a44156224c77c51f4f6ca61a0168e48dfcc6eda) )
	ROM_LOAD16_BYTE( "lp_4.u0120", 0x000000, 0x100000, CRC(12fffc02) SHA1(3294b65e4a0bbf501785565dd0c1f36f9bcea969) )
	ROM_LOAD( "rp00.u0216",        0x400000, 0x400000, CRC(d759d0d4) SHA1(47b009a5dfa81611276b1376bdab44dfad597e85) )
	ROM_LOAD( "rp01.u0217",        0x800000, 0x400000, CRC(5be576e1) SHA1(e24a96d179016d6d65205079874b35500760a642) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg07", 0, 8, CRC(afec0e8e) SHA1(dc3c696181dfb5b7426d882094be5eee2619e2a5) )
ROM_END

ROM_START( tblkkuzu )
	TPS_BIOS

	ROM_REGION32_LE( 0x2800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "tbk.u119", 0x0000001, 0x100000, CRC(621b07e1) SHA1(30773aaa333fdee7ef55db2f8adde010688abce1) )
	ROM_LOAD16_BYTE( "tbk.u120", 0x0000000, 0x100000, CRC(bb390f7d) SHA1(6bce88448fbb5308952f8c221e786be8aa51ceff) )
	ROM_LOAD( "tbk.u0216",       0x0400000, 0x400000, CRC(41f8285f) SHA1(3326ab83d96d51ed31fb5c2f30630ff480d45282) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg12", 0, 8, CRC(584e6ea2) SHA1(f60fb556090d31f0f0fa1f8d87815eff6d2498a4) )
ROM_END

ROM_START( mfjump )
	TPS_BIOS

	ROM_REGION32_LE( 0x02800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "mfj-o.119",   0x0000001, 0x100000, CRC(0d724dc5) SHA1(2ba388fe6254c0cf3847fd173a414ee5ca31f4f4) )
	ROM_LOAD16_BYTE( "mfj-e.120",   0x0000000, 0x100000, CRC(86292bca) SHA1(b6a25ab828da3d5c8f6d945336513485708f3f5b) )
	ROM_LOAD( "mfj.216",            0x0400000, 0x400000, CRC(0d518dba) SHA1(100cd4d0a1e678e660336027f067a9a1f5cbad3e) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg14", 0, 8, CRC(5fa9a8c2) SHA1(d927760279ebda185fecf879e62ceefe938acd81) )
ROM_END

/* video system */

#define KN_BIOS \
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "coh-1002v.ic353", 0x0000000, 0x080000, CRC(5ff165f3) SHA1(8f59314c1093446b9bcb06d232244da6df78e206) ) \
	ROM_REGION( 0x000008, REGION_USER4, 0 ) /* CAT702, main board */ \
	ROM_LOAD( "kn01.ic652", 0, 8, CRC(1ce271df) SHA1(b01479dc75a4a8a86bee7ac7cd10af2bda332544) )

ROM_START( coh1002v )
	KN_BIOS
ROM_END

ROM_START( aerofgts )
	KN_BIOS

	ROM_REGION32_LE( 0x0280000, REGION_USER2, 0 )
	ROM_LOAD( "1-8_prog0_usa_aa9e.ic5", 0x0000000, 0x080000, CRC(d15a4904) SHA1(4e329f56584df4b057b56e21f0230dda4ec46bcf) )
	ROM_LOAD( "1-8_prog1_usa_0a1e.ic6", 0x0080000, 0x080000, CRC(d6625176) SHA1(018bba263effda1a0b64023aa08b384f0590245b) )
	ROM_LOAD( "1-8_prog2_usa_c4b3.ic7", 0x0100000, 0x080000, CRC(df5ba2f7) SHA1(19153084e7cff632380b67a2fff800644a2fbf7d) )
	ROM_LOAD( "1-8_prog3_usa_e1d8.ic8", 0x0180000, 0x080000, CRC(8711574a) SHA1(d02a9dac78ee468985c2f2c8d3b9a588f6fc7c02) )
	ROM_LOAD( "1-8_prog4_usa_9127.ic9", 0x0200000, 0x080000, CRC(670d9b27) SHA1(a4bca91c36d06cb0663d2478096bbc1668e01d60) )

	ROM_REGION32_LE( 0x1800000, REGION_USER3, 0 )
	ROM_LOAD( "ic11.bin",     0x0000000, 0x400000, CRC(a93f6fee) SHA1(6f079643b50833f8fb497c49945ad23326cc9170) )
	ROM_LOAD( "ic12.bin",     0x0400000, 0x400000, CRC(9f584ef7) SHA1(12c04e198f17d1915f58e83aff45ca2e76773df8) )
	ROM_LOAD( "ic13.bin",     0x0800000, 0x400000, CRC(652e9c78) SHA1(a929b2944de72606338acb822c1031463e2b1cc5) )
	ROM_LOAD( "ic14.bin",     0x0c00000, 0x400000, CRC(c4ef1424) SHA1(1734a6ee6d0be94d24afefcf2a125b74747f53d0) )
	ROM_LOAD( "ic15.bin",     0x1000000, 0x400000, CRC(2551d816) SHA1(e1500d4bfa8cc55220c366a5852263ac2070da82) )
	ROM_LOAD( "ic16.bin",     0x1400000, 0x400000, CRC(21b401bc) SHA1(89374b80453c474aa1dd3a219422f557f95a262c) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "kn02", 0, 8, CRC(0d2001b4) SHA1(740d7794a1b94a42351d81947cbfae138c56ecc9) )
ROM_END

ROM_START( aerofgtst )
	KN_BIOS

	ROM_REGION32_LE( 0x0280000, REGION_USER2, 0 )
	ROM_LOAD( "vs.ic5",       0x0000000, 0x080000, CRC(453dd514) SHA1(189fe5598485de160f5c0add90235ed63ed92747) )
	ROM_LOAD( "vs.ic6",       0x0080000, 0x080000, CRC(24257295) SHA1(fe6d969c407f2c26c3af2a2a5015dc83b1675e9a) )
	ROM_LOAD( "vs.ic7",       0x0100000, 0x080000, CRC(df5ba2f7) SHA1(19153084e7cff632380b67a2fff800644a2fbf7d) )
	ROM_LOAD( "vs.ic8",       0x0180000, 0x080000, CRC(df638f92) SHA1(5704eed2ef798a91a0398948af0324955ec38534) )
	ROM_LOAD( "vs.ic9",       0x0200000, 0x080000, CRC(1294aa24) SHA1(07ca2cc3f117cfd48d2ca558d9435a3fc238c0fe) )

	ROM_REGION32_LE( 0x1800000, REGION_USER3, 0 )
	ROM_LOAD( "ic11.bin",     0x0000000, 0x400000, CRC(a93f6fee) SHA1(6f079643b50833f8fb497c49945ad23326cc9170) )
	ROM_LOAD( "ic12.bin",     0x0400000, 0x400000, CRC(9f584ef7) SHA1(12c04e198f17d1915f58e83aff45ca2e76773df8) )
	ROM_LOAD( "ic13.bin",     0x0800000, 0x400000, CRC(652e9c78) SHA1(a929b2944de72606338acb822c1031463e2b1cc5) )
	ROM_LOAD( "ic14.bin",     0x0c00000, 0x400000, CRC(c4ef1424) SHA1(1734a6ee6d0be94d24afefcf2a125b74747f53d0) )
	ROM_LOAD( "ic15.bin",     0x1000000, 0x400000, CRC(2551d816) SHA1(e1500d4bfa8cc55220c366a5852263ac2070da82) )
	ROM_LOAD( "ic16.bin",     0x1400000, 0x400000, CRC(21b401bc) SHA1(89374b80453c474aa1dd3a219422f557f95a262c) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "kn02", 0, 8, CRC(0d2001b4) SHA1(740d7794a1b94a42351d81947cbfae138c56ecc9) )
ROM_END

ROM_START( sncwgltd )
	KN_BIOS

	ROM_REGION32_LE( 0x0280000, REGION_USER2, 0 )
	ROM_LOAD( "ic5.bin",      0x0000000, 0x080000, CRC(458f14aa) SHA1(b4e50be60ffb9b7911561dd35b6a7e0df3432a3a) )
	ROM_LOAD( "ic6.bin",      0x0080000, 0x080000, CRC(8233dd1e) SHA1(1422b4530d671e3b8b471ec16c20ef7c819ab762) )
	ROM_LOAD( "ic7.bin",      0x0100000, 0x080000, CRC(df5ba2f7) SHA1(19153084e7cff632380b67a2fff800644a2fbf7d) )
	ROM_LOAD( "ic8.bin",      0x0180000, 0x080000, CRC(e8145f2b) SHA1(3a1cb189426998856dfeda47267fde64be34c6ec) )
	ROM_LOAD( "ic9.bin",      0x0200000, 0x080000, CRC(605c9370) SHA1(9734549cae3028c089f4c9f2336ee374b3f950f8) )

	ROM_REGION32_LE( 0x1800000, REGION_USER3, 0 )
	ROM_LOAD( "ic11.bin",     0x0000000, 0x400000, CRC(a93f6fee) SHA1(6f079643b50833f8fb497c49945ad23326cc9170) )
	ROM_LOAD( "ic12.bin",     0x0400000, 0x400000, CRC(9f584ef7) SHA1(12c04e198f17d1915f58e83aff45ca2e76773df8) )
	ROM_LOAD( "ic13.bin",     0x0800000, 0x400000, CRC(652e9c78) SHA1(a929b2944de72606338acb822c1031463e2b1cc5) )
	ROM_LOAD( "ic14.bin",     0x0c00000, 0x400000, CRC(c4ef1424) SHA1(1734a6ee6d0be94d24afefcf2a125b74747f53d0) )
	ROM_LOAD( "ic15.bin",     0x1000000, 0x400000, CRC(2551d816) SHA1(e1500d4bfa8cc55220c366a5852263ac2070da82) )
	ROM_LOAD( "ic16.bin",     0x1400000, 0x400000, CRC(21b401bc) SHA1(89374b80453c474aa1dd3a219422f557f95a262c) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "kn02", 0, 8, CRC(0d2001b4) SHA1(740d7794a1b94a42351d81947cbfae138c56ecc9) )
ROM_END


/* Taito FX1a/FX1b */

#define TAITOFX1_BIOS \
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "m534002c-14.ic353", 0x0000000, 0x080000, CRC(e3f23b6e) SHA1(e18907cf8c6ba54d96edba0a9a00487a90219e0d) ) \
	ROM_REGION( 0x000008, REGION_USER4, 0 ) /* CAT702, main board */ \
	ROM_LOAD( "tt01.ic652", 0, 8, CRC(daac13e7) SHA1(96d94f2b06fb7ecd91454969f4959a583ffb9d5a) )

ROM_START( coh1000t )
	TAITOFX1_BIOS
ROM_END

ROM_START( ftimpact )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e25-09.4",     0x0000001, 0x080000, CRC(d457bfc7) SHA1(e974a9c3e7b0748ef89d78e76a7dbb763c42b6f7) )
	ROM_LOAD16_BYTE( "e25-07.3",     0x0000000, 0x080000, CRC(829be1cc) SHA1(64b139d7c3696ab2f0b9a4842c19a38fe6a8cede) )
	ROM_LOAD( "e25-01.1",            0x0400000, 0x400000, CRC(8cc4be0c) SHA1(9ca15558a83b7e332e50accf1f7852444a7ce730) )
	ROM_LOAD( "e25-02.2",            0x0800000, 0x400000, CRC(8e8b4c82) SHA1(55c9d4d3a08fc3226a75ab3a674be433af83e289) )
	ROM_LOAD( "e25-03.12",           0x0c00000, 0x400000, CRC(43b1c085) SHA1(6e53550e9be0d2f415fc6b4f3b8a71185c5370b2) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e25-10.14",    0x0000000, 0x080000, CRC(2b2ad1b1) SHA1(6d064d0b6805d43ce42929ac8f5645b56384f53c) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	ROM_LOAD( "e25-04.27",    0x0000000, 0x400000, CRC(09a66d35) SHA1(f0df24bc9bfc9eb0f5150dc035c19fc5b8a39bf9) )
	ROM_LOAD( "e25-05.28",    0x0040000, 0x200000, CRC(3fb57636) SHA1(aa38bfac11ecf10fd55143cf4525a2a529be8bb6) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt05", 0, 8, CRC(66045c83) SHA1(b040fa87e855fd512fa99e80056d37aba895805b) )
ROM_END

ROM_START( ftimpactj )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e25-06.4",     0x0000001, 0x080000, CRC(3a59deeb) SHA1(4377c5829fb5b6f5d0120caf992b1ee714897641) )
	ROM_LOAD16_BYTE( "e25-07.3",     0x0000000, 0x080000, CRC(829be1cc) SHA1(64b139d7c3696ab2f0b9a4842c19a38fe6a8cede) )
	ROM_LOAD( "e25-01.1",            0x0400000, 0x400000, CRC(8cc4be0c) SHA1(9ca15558a83b7e332e50accf1f7852444a7ce730) )
	ROM_LOAD( "e25-02.2",            0x0800000, 0x400000, CRC(8e8b4c82) SHA1(55c9d4d3a08fc3226a75ab3a674be433af83e289) )
	ROM_LOAD( "e25-03.12",           0x0c00000, 0x400000, CRC(43b1c085) SHA1(6e53550e9be0d2f415fc6b4f3b8a71185c5370b2) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e25-10.14",    0x0000000, 0x080000, CRC(2b2ad1b1) SHA1(6d064d0b6805d43ce42929ac8f5645b56384f53c) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	ROM_LOAD( "e25-04.27",    0x0000000, 0x400000, CRC(09a66d35) SHA1(f0df24bc9bfc9eb0f5150dc035c19fc5b8a39bf9) )
	ROM_LOAD( "e25-05.28",    0x0040000, 0x200000, CRC(3fb57636) SHA1(aa38bfac11ecf10fd55143cf4525a2a529be8bb6) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt05", 0, 8, CRC(66045c83) SHA1(b040fa87e855fd512fa99e80056d37aba895805b) )
ROM_END

ROM_START( ftimpactu )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e25-08.4",     0x0000001, 0x080000, CRC(a3508f51) SHA1(fd4c3cc186e280497dc905ebda92472d5b72b1b4) )
	ROM_LOAD16_BYTE( "e25-07.3",     0x0000000, 0x080000, CRC(829be1cc) SHA1(64b139d7c3696ab2f0b9a4842c19a38fe6a8cede) )
	ROM_LOAD( "e25-01.1",            0x0400000, 0x400000, CRC(8cc4be0c) SHA1(9ca15558a83b7e332e50accf1f7852444a7ce730) )
	ROM_LOAD( "e25-02.2",            0x0800000, 0x400000, CRC(8e8b4c82) SHA1(55c9d4d3a08fc3226a75ab3a674be433af83e289) )
	ROM_LOAD( "e25-03.12",           0x0c00000, 0x400000, CRC(43b1c085) SHA1(6e53550e9be0d2f415fc6b4f3b8a71185c5370b2) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e25-10.14",    0x0000000, 0x080000, CRC(2b2ad1b1) SHA1(6d064d0b6805d43ce42929ac8f5645b56384f53c) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	ROM_LOAD( "e25-04.27",    0x0000000, 0x400000, CRC(09a66d35) SHA1(f0df24bc9bfc9eb0f5150dc035c19fc5b8a39bf9) )
	ROM_LOAD( "e25-05.28",    0x0040000, 0x200000, CRC(3fb57636) SHA1(aa38bfac11ecf10fd55143cf4525a2a529be8bb6) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt05", 0, 8, CRC(66045c83) SHA1(b040fa87e855fd512fa99e80056d37aba895805b) )
ROM_END

ROM_START( ftimpcta )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e25-13.4",     0x0000001, 0x100000, CRC(7f078d7b) SHA1(df9800dd6885dbc33736c5143d877b0847221061) )
	ROM_LOAD16_BYTE( "e25-14.3",     0x0000000, 0x100000, CRC(0c5f474f) SHA1(ce7031ba860297b99cddd6d0177f07e03520faeb) )
	ROM_LOAD( "e25-01.1",            0x0400000, 0x400000, CRC(8cc4be0c) SHA1(9ca15558a83b7e332e50accf1f7852444a7ce730) )
	ROM_LOAD( "e25-02.2",            0x0800000, 0x400000, CRC(8e8b4c82) SHA1(55c9d4d3a08fc3226a75ab3a674be433af83e289) )
	ROM_LOAD( "e25-03.12",           0x0c00000, 0x400000, CRC(43b1c085) SHA1(6e53550e9be0d2f415fc6b4f3b8a71185c5370b2) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e25-10.14",    0x0000000, 0x080000, CRC(2b2ad1b1) SHA1(6d064d0b6805d43ce42929ac8f5645b56384f53c) )

	ROM_REGION( 0x600000, REGION_SOUND1, 0 )
	ROM_LOAD( "e25-04.27",    0x0000000, 0x400000, CRC(09a66d35) SHA1(f0df24bc9bfc9eb0f5150dc035c19fc5b8a39bf9) )
	ROM_LOAD( "e25-05.28",    0x0040000, 0x200000, CRC(3fb57636) SHA1(aa38bfac11ecf10fd55143cf4525a2a529be8bb6) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt05", 0, 8, CRC(66045c83) SHA1(b040fa87e855fd512fa99e80056d37aba895805b) )
ROM_END

ROM_START( gdariusj )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e39-06.4",     0x0000001, 0x100000, CRC(2980c30d) SHA1(597321642125c3ae37581c2d9abc2723c7909996) )
	ROM_LOAD16_BYTE( "e39-05.3",     0x0000000, 0x100000, CRC(750e5b13) SHA1(68fe9cbd7d506cfd587dccc40b6ae0b0b6ee7c29) )
	ROM_LOAD( "e39-01.1",            0x0400000, 0x400000, CRC(bdaaa251) SHA1(a42daa706ee859c2b66be179e08c0ad7990f919e) )
	ROM_LOAD( "e39-02.2",            0x0800000, 0x400000, CRC(a47aab5d) SHA1(64b58e47035ad9d8d6dcaf475cbcc3ad85f4d82f) )
	ROM_LOAD( "e39-03.12",           0x0c00000, 0x400000, CRC(a883b6a5) SHA1(b8d00d944c90f8cd9c2b076688f4c68b2e6d557a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e39-07.14",    0x0000000, 0x080000, CRC(2252c7c1) SHA1(92b9908e0d87cad6587f1acc0eef69eaae8c6a98) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "e39-04.27",    0x0000000, 0x400000, CRC(6ee35e68) SHA1(fdfe63203d8cecf84cb869039fb893d5b63cdd67) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt07", 0, 8, CRC(ccf2f332) SHA1(6eb07cd5fab29f5536fd94aa88a2b09f28cc3494) )
ROM_END

ROM_START( gdarius )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e39-09.ic4",     0x0000001, 0x100000, CRC(f457794d) SHA1(43571770b239298e3442d3da8aa4f4507380d639) )
	ROM_LOAD16_BYTE( "e39-10.ic3",     0x0000000, 0x100000, CRC(6ba4d941) SHA1(75f2d8c920d29102c09e041fc3198e32ad57dbaf) )
	ROM_LOAD( "e39-01.1",            0x0400000, 0x400000, CRC(bdaaa251) SHA1(a42daa706ee859c2b66be179e08c0ad7990f919e) )
	ROM_LOAD( "e39-02.2",            0x0800000, 0x400000, CRC(a47aab5d) SHA1(64b58e47035ad9d8d6dcaf475cbcc3ad85f4d82f) )
	ROM_LOAD( "e39-03.12",           0x0c00000, 0x400000, CRC(a883b6a5) SHA1(b8d00d944c90f8cd9c2b076688f4c68b2e6d557a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e39-07.14",    0x0000000, 0x080000, CRC(2252c7c1) SHA1(92b9908e0d87cad6587f1acc0eef69eaae8c6a98) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "e39-04.27",    0x0000000, 0x400000, CRC(6ee35e68) SHA1(fdfe63203d8cecf84cb869039fb893d5b63cdd67) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt07", 0, 8, CRC(ccf2f332) SHA1(6eb07cd5fab29f5536fd94aa88a2b09f28cc3494) )
ROM_END

ROM_START( gdariusu )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e39-08.ic4",   0x0000001, 0x100000, CRC(835049db) SHA1(2b230c8fd6c6ea4e30740fda28f631344b018b79) )
	ROM_LOAD16_BYTE( "e39-10.ic3",   0x0000000, 0x100000, CRC(6ba4d941) SHA1(75f2d8c920d29102c09e041fc3198e32ad57dbaf) )
	ROM_LOAD( "e39-01.1",            0x0400000, 0x400000, CRC(bdaaa251) SHA1(a42daa706ee859c2b66be179e08c0ad7990f919e) )
	ROM_LOAD( "e39-02.2",            0x0800000, 0x400000, CRC(a47aab5d) SHA1(64b58e47035ad9d8d6dcaf475cbcc3ad85f4d82f) )
	ROM_LOAD( "e39-03.12",           0x0c00000, 0x400000, CRC(a883b6a5) SHA1(b8d00d944c90f8cd9c2b076688f4c68b2e6d557a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e39-07.14",    0x0000000, 0x080000, CRC(2252c7c1) SHA1(92b9908e0d87cad6587f1acc0eef69eaae8c6a98) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "e39-04.27",    0x0000000, 0x400000, CRC(6ee35e68) SHA1(fdfe63203d8cecf84cb869039fb893d5b63cdd67) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt07", 0, 8, CRC(ccf2f332) SHA1(6eb07cd5fab29f5536fd94aa88a2b09f28cc3494) )
ROM_END

ROM_START( gdarius2 )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e39-12.4",     0x0000001, 0x100000, CRC(b23266c3) SHA1(80aaddaaf10e40280ade4c7d11f45ddab47ee9a6) )
	ROM_LOAD16_BYTE( "e39-11.3",     0x0000000, 0x100000, CRC(766f73df) SHA1(9ce24c153920d259bc7fdef0778083eb6d639be3) )
	ROM_LOAD( "e39-01.1",            0x0400000, 0x400000, CRC(bdaaa251) SHA1(a42daa706ee859c2b66be179e08c0ad7990f919e) )
	ROM_LOAD( "e39-02.2",            0x0800000, 0x400000, CRC(a47aab5d) SHA1(64b58e47035ad9d8d6dcaf475cbcc3ad85f4d82f) )
	ROM_LOAD( "e39-03.12",           0x0c00000, 0x400000, CRC(a883b6a5) SHA1(b8d00d944c90f8cd9c2b076688f4c68b2e6d557a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e39-07.14",    0x0000000, 0x080000, CRC(2252c7c1) SHA1(92b9908e0d87cad6587f1acc0eef69eaae8c6a98) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "e39-04.27",    0x0000000, 0x400000, CRC(6ee35e68) SHA1(fdfe63203d8cecf84cb869039fb893d5b63cdd67) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt07", 0, 8, CRC(ccf2f332) SHA1(6eb07cd5fab29f5536fd94aa88a2b09f28cc3494) )
ROM_END

ROM_START( mgcldate )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e32-05.2",     0x0000001, 0x080000, CRC(72fc7f7b) SHA1(50d9e84bc74fb63ec1900ab149051888bc3d03a5) )
	ROM_LOAD16_BYTE( "e32-06.7",     0x0000000, 0x080000, CRC(d11c3881) SHA1(f7046c5bed4818152edcf697a49664b0bcf12a1b) )
	ROM_LOAD( "e32-01.1",            0x0400000, 0x400000, CRC(cf5f1d01) SHA1(5417f8aef5c8d0e9e63ba8c68efb5b3ef37b4693) )
	ROM_LOAD( "e32-02.6",            0x0800000, 0x400000, CRC(61c8438c) SHA1(bdbe6079cc634c0cd6580f76619eb2944c9a31d9) )
	ROM_LOAD( "e32-03.12",           0x0c00000, 0x200000, CRC(190d1618) SHA1(838a651d32752015baa7e8caea62fd739631b8be) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e32-07.22",           0x0000000, 0x004000, CRC(adf3feb5) SHA1(bae5bc3fad99a92a3492be1b775dab861007eb3b) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "e32-04.15",           0x0000000, 0x400000, CRC(c72f9eea) SHA1(7ab8b412a8ed00a42016acb7d13d3b074155780a) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt06", 0, 8, CRC(ce282fc9) SHA1(dfc1f679ec70f2ff32869cfeb452acf9ac154aef) )
ROM_END

ROM_START( mgcldtex )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e32-08.2",     0x0000001, 0x100000, CRC(3d42cd28) SHA1(9017922e835a359ba5126c8a9e8c27380a5ce081) )
	ROM_LOAD16_BYTE( "e32-09.7",     0x0000000, 0x100000, CRC(db7ec115) SHA1(fa6f18de71ba997389d887d7ffe745aa25e24c20) )
	ROM_LOAD( "e32-01.1",            0x0400000, 0x400000, CRC(cf5f1d01) SHA1(5417f8aef5c8d0e9e63ba8c68efb5b3ef37b4693) )
	ROM_LOAD( "e32-02.6",            0x0800000, 0x400000, CRC(61c8438c) SHA1(bdbe6079cc634c0cd6580f76619eb2944c9a31d9) )
	ROM_LOAD( "e32-03.12",           0x0c00000, 0x200000, CRC(190d1618) SHA1(838a651d32752015baa7e8caea62fd739631b8be) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e32-10.22",           0x0000000, 0x004000, CRC(adf3feb5) SHA1(bae5bc3fad99a92a3492be1b775dab861007eb3b) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "e32-04.15",           0x0000000, 0x400000, CRC(c72f9eea) SHA1(7ab8b412a8ed00a42016acb7d13d3b074155780a) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt06", 0, 8, CRC(ce282fc9) SHA1(dfc1f679ec70f2ff32869cfeb452acf9ac154aef) )
ROM_END

ROM_START( psyforce )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e22-09_.2",  0x0000001, 0x080000, CRC(997e4500) SHA1(4a90b452c9a877ccec55a11f36c4cbc6df1f1f41) )
	ROM_LOAD16_BYTE( "e22-06_.7",     0x0000000, 0x080000, CRC(f6341d63) SHA1(99dc27aa694ae5951148054291912a486726e8c9) )
	ROM_LOAD( "e22-02.16",           0x0800000, 0x200000, CRC(03b50064) SHA1(0259537e86b266b3f34308c4fc0bcc04c037da71) )
	ROM_LOAD( "e22-03.19",           0x0a00000, 0x200000, CRC(8372f839) SHA1(646b3919b6be63412c11850ec1524685abececc0) )
	ROM_LOAD( "e22-04.21",           0x0c00000, 0x200000, CRC(397b71aa) SHA1(48743c362503c1d2dbeb3c8be4cb2aaaae015b88) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e22-07.22",           0x0000000, 0x004000, CRC(739af589) SHA1(dbb4d1c6d824a99ccf27168e2c21644e19811523) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "e22-01.15",           0x000000,  0x200000, CRC(808b8340) SHA1(d8bde850dd9b5b71e94ea707d2d728754f907977) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt03", 0, 8, CRC(5799d8c9) SHA1(72cf1ed0958959d085ca5f8ab8b4f883ab1cd162) )
ROM_END

ROM_START( psyforcej )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e22-05_.2",     0x0000001, 0x080000, CRC(7770242c) SHA1(dd37575d3d9ffdef60fe0e4cab6c9e42d087f714) )
	ROM_LOAD16_BYTE( "e22-10_.7",     0x0000000, 0x080000, CRC(f6341d63) SHA1(99dc27aa694ae5951148054291912a486726e8c9) )
	ROM_LOAD( "e22-02.16",           0x0800000, 0x200000, CRC(03b50064) SHA1(0259537e86b266b3f34308c4fc0bcc04c037da71) )
	ROM_LOAD( "e22-03.19",           0x0a00000, 0x200000, CRC(8372f839) SHA1(646b3919b6be63412c11850ec1524685abececc0) )
	ROM_LOAD( "e22-04.21",           0x0c00000, 0x200000, CRC(397b71aa) SHA1(48743c362503c1d2dbeb3c8be4cb2aaaae015b88) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e22-07.22",           0x0000000, 0x004000, CRC(739af589) SHA1(dbb4d1c6d824a99ccf27168e2c21644e19811523) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "e22-01.15",           0x000000,  0x200000, CRC(808b8340) SHA1(d8bde850dd9b5b71e94ea707d2d728754f907977) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt03", 0, 8, CRC(5799d8c9) SHA1(72cf1ed0958959d085ca5f8ab8b4f883ab1cd162) )
ROM_END

ROM_START( psyforcex )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e22-11.2",     0x0000001, 0x080000, CRC(29ebebc9) SHA1(48a3371460d69ae5ba58d3e24615f389bf43b2cc) )
	ROM_LOAD16_BYTE( "e22-12.7",     0x0000000, 0x080000, CRC(7426ffc5) SHA1(24b0132241e2e49109e585b082bf4ab67f86b294) )
	ROM_LOAD( "e22-02.16",           0x0800000, 0x200000, CRC(03b50064) SHA1(0259537e86b266b3f34308c4fc0bcc04c037da71) )
	ROM_LOAD( "e22-03.19",           0x0a00000, 0x200000, CRC(8372f839) SHA1(646b3919b6be63412c11850ec1524685abececc0) )
	ROM_LOAD( "e22-04.21",           0x0c00000, 0x200000, CRC(397b71aa) SHA1(48743c362503c1d2dbeb3c8be4cb2aaaae015b88) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e22-07.22",           0x0000000, 0x004000, CRC(739af589) SHA1(dbb4d1c6d824a99ccf27168e2c21644e19811523) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "e22-01.15",           0x000000,  0x200000, CRC(808b8340) SHA1(d8bde850dd9b5b71e94ea707d2d728754f907977) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt03", 0, 8, CRC(5799d8c9) SHA1(72cf1ed0958959d085ca5f8ab8b4f883ab1cd162) )
ROM_END


ROM_START( raystorm )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e24-xx.ic4", 0x0000001, 0x080000, CRC(33f63638) SHA1(fdda33ffc9902b3605a3272fae5a614e93856a86) )
	ROM_LOAD16_BYTE( "e24-xx.ic3", 0x0000000, 0x080000, CRC(5eeed3b2) SHA1(d8bb1613d7285eabdc6f0a2d231d2eeeb52f307b) )
	ROM_LOAD( "e24-02.1",            0x0400000, 0x400000, CRC(9f70950d) SHA1(b3e4f925a61ae2e5dd4cc5d7ec3030a0d5c2c04d) )
	ROM_LOAD( "e24-03.2",            0x0800000, 0x400000, CRC(6c1f0a5d) SHA1(1aac37a7ff23e54021a4cec18c9bb93242337180) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e24-09.14",    0x0000000, 0x080000, CRC(808589e1) SHA1(46ada4c6d68c2462186a0b962abb435ee740c0ba) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "e24-04.27",    0x0000000, 0x400000, CRC(f403493a) SHA1(3e49fd2a060a3893e26f14cc3cf47c4ba91e17d4) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt04", 0, 8, CRC(013363df) SHA1(f04d04c7c858a8e2eaff0c51e782ad76d6ebbed1) )
ROM_END

ROM_START( raystormu )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e24-07.4", 0x0000001, 0x080000, CRC(d9002b03) SHA1(bdb0aa88536c4c98c150ece87387930b3dbdd258) )
	ROM_LOAD16_BYTE( "e24-06.3", 0x0000000, 0x080000, CRC(d70cdf46) SHA1(da6163d69d3ea9c1e3f4b7961a548f1f9d8d9909) )
	ROM_LOAD( "e24-02.1",            0x0400000, 0x400000, CRC(9f70950d) SHA1(b3e4f925a61ae2e5dd4cc5d7ec3030a0d5c2c04d) )
	ROM_LOAD( "e24-03.2",            0x0800000, 0x400000, CRC(6c1f0a5d) SHA1(1aac37a7ff23e54021a4cec18c9bb93242337180) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e24-09.14",    0x0000000, 0x080000, CRC(808589e1) SHA1(46ada4c6d68c2462186a0b962abb435ee740c0ba) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "e24-04.27",    0x0000000, 0x400000, CRC(f403493a) SHA1(3e49fd2a060a3893e26f14cc3cf47c4ba91e17d4) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt04", 0, 8, CRC(013363df) SHA1(f04d04c7c858a8e2eaff0c51e782ad76d6ebbed1) )
ROM_END

ROM_START( raystormo )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e24-08.4", 0x0000001, 0x080000, CRC(ae071b95) SHA1(0e1597220808d6e3998ef1e9d88779e0187ba0af) )
	ROM_LOAD16_BYTE( "e24-06.3", 0x0000000, 0x080000, CRC(d70cdf46) SHA1(da6163d69d3ea9c1e3f4b7961a548f1f9d8d9909) )
	ROM_LOAD( "e24-02.1",            0x0400000, 0x400000, CRC(9f70950d) SHA1(b3e4f925a61ae2e5dd4cc5d7ec3030a0d5c2c04d) )
	ROM_LOAD( "e24-03.2",            0x0800000, 0x400000, CRC(6c1f0a5d) SHA1(1aac37a7ff23e54021a4cec18c9bb93242337180) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e24-09.14",    0x0000000, 0x080000, CRC(808589e1) SHA1(46ada4c6d68c2462186a0b962abb435ee740c0ba) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "e24-04.27",    0x0000000, 0x400000, CRC(f403493a) SHA1(3e49fd2a060a3893e26f14cc3cf47c4ba91e17d4) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt04", 0, 8, CRC(013363df) SHA1(f04d04c7c858a8e2eaff0c51e782ad76d6ebbed1) )
ROM_END

ROM_START( raystormj )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e24-05.4",     0x0000001, 0x080000, CRC(40097ab9) SHA1(67e73568b35515c2c5a9119e97ac4709baff8c5a) )
	ROM_LOAD16_BYTE( "e24-06.3",     0x0000000, 0x080000, CRC(d70cdf46) SHA1(da6163d69d3ea9c1e3f4b7961a548f1f9d8d9909) )
	ROM_LOAD( "e24-02.1",            0x0400000, 0x400000, CRC(9f70950d) SHA1(b3e4f925a61ae2e5dd4cc5d7ec3030a0d5c2c04d) )
	ROM_LOAD( "e24-03.2",            0x0800000, 0x400000, CRC(6c1f0a5d) SHA1(1aac37a7ff23e54021a4cec18c9bb93242337180) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "e24-09.14",    0x0000000, 0x080000, CRC(808589e1) SHA1(46ada4c6d68c2462186a0b962abb435ee740c0ba) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "e24-04.27",    0x0000000, 0x400000, CRC(f403493a) SHA1(3e49fd2a060a3893e26f14cc3cf47c4ba91e17d4) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt04", 0, 8, CRC(013363df) SHA1(f04d04c7c858a8e2eaff0c51e782ad76d6ebbed1) )
ROM_END

ROM_START( sfchamp )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e18-12.2",     0x0000001, 0x080000, CRC(72304685) SHA1(2e6f645871e19a49fcdfbdca49c6be415471eadf) )
	ROM_LOAD16_BYTE( "e18-13.7",     0x0000000, 0x080000, CRC(fa4d01ee) SHA1(27efd8e2107d71213d35f2a58762ed8812f809d3) )
	ROM_LOAD( "e18-02.12",           0x0600000, 0x200000, CRC(c7b4fe29) SHA1(7f823bd61abf2b15d3ba62bca829a5b1acacfd09) )
	ROM_LOAD( "e18-03.16",           0x0800000, 0x200000, CRC(76392346) SHA1(2c5b70c4708208f866feea0472fcc72333061124) )
	ROM_LOAD( "e18-04.19",           0x0a00000, 0x200000, CRC(fc3731da) SHA1(58948aad8d7bb7a8449d2bf12e9d5e6d7b4426b5) )
	ROM_LOAD( "e18-05.21",           0x0c00000, 0x200000, CRC(2e984c50) SHA1(6d8255e38c67d68bf489c9885663ed2edf148188) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e18-09.22",           0x0000000, 0x004000, CRC(bb5a5319) SHA1(0bb700cafc157d3af663cc9bebb8167487ff2852) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "e18-01.15",           0x0000000, 0x200000, CRC(dbd1408c) SHA1(ef81064f2f95e5ae25eb1f10d1e78f27f9e294f5) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt02", 0, 8, CRC(b1585ac7) SHA1(d967eea539de2dd7a8d13fa2785c3033065bed9c) )
ROM_END

ROM_START( sfchampu )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e18-10.2",     0x0000001, 0x080000, CRC(82411fa6) SHA1(0aa1764b7ff68258ef76a41355c50d5067262d75) )
	ROM_LOAD16_BYTE( "e18-08.7",     0x0000000, 0x080000, CRC(6a5558cd) SHA1(75b26bcaaa213283e7e0dace69ee58f305b4572d) )
	ROM_LOAD( "e18-02.12",           0x0600000, 0x200000, CRC(c7b4fe29) SHA1(7f823bd61abf2b15d3ba62bca829a5b1acacfd09) )
	ROM_LOAD( "e18-03.16",           0x0800000, 0x200000, CRC(76392346) SHA1(2c5b70c4708208f866feea0472fcc72333061124) )
	ROM_LOAD( "e18-04.19",           0x0a00000, 0x200000, CRC(fc3731da) SHA1(58948aad8d7bb7a8449d2bf12e9d5e6d7b4426b5) )
	ROM_LOAD( "e18-05.21",           0x0c00000, 0x200000, CRC(2e984c50) SHA1(6d8255e38c67d68bf489c9885663ed2edf148188) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e18-09.22",           0x0000000, 0x004000, CRC(bb5a5319) SHA1(0bb700cafc157d3af663cc9bebb8167487ff2852) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "e18-01.15",           0x0000000, 0x200000, CRC(dbd1408c) SHA1(ef81064f2f95e5ae25eb1f10d1e78f27f9e294f5) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt02", 0, 8, CRC(b1585ac7) SHA1(d967eea539de2dd7a8d13fa2785c3033065bed9c) )
ROM_END

ROM_START( sfchampo )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e18-11.2",     0x0000001, 0x080000, CRC(f5462f30) SHA1(44eb03a9b51e2d8dd14fe2ed36dbcf17035a22c7) )
	ROM_LOAD16_BYTE( "e18-08.7",     0x0000000, 0x080000, CRC(6a5558cd) SHA1(75b26bcaaa213283e7e0dace69ee58f305b4572d) )
	ROM_LOAD( "e18-02.12",           0x0600000, 0x200000, CRC(c7b4fe29) SHA1(7f823bd61abf2b15d3ba62bca829a5b1acacfd09) )
	ROM_LOAD( "e18-03.16",           0x0800000, 0x200000, CRC(76392346) SHA1(2c5b70c4708208f866feea0472fcc72333061124) )
	ROM_LOAD( "e18-04.19",           0x0a00000, 0x200000, CRC(fc3731da) SHA1(58948aad8d7bb7a8449d2bf12e9d5e6d7b4426b5) )
	ROM_LOAD( "e18-05.21",           0x0c00000, 0x200000, CRC(2e984c50) SHA1(6d8255e38c67d68bf489c9885663ed2edf148188) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e18-09.22",           0x0000000, 0x004000, CRC(bb5a5319) SHA1(0bb700cafc157d3af663cc9bebb8167487ff2852) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "e18-01.15",           0x0000000, 0x200000, CRC(dbd1408c) SHA1(ef81064f2f95e5ae25eb1f10d1e78f27f9e294f5) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt02", 0, 8, CRC(b1585ac7) SHA1(d967eea539de2dd7a8d13fa2785c3033065bed9c) )
ROM_END

ROM_START( sfchampj )
	TAITOFX1_BIOS

	ROM_REGION32_LE( 0x01000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "e18-07.2",     0x0000001, 0x080000, CRC(1b484e1c) SHA1(f29f40a9988475d8abbb126095b0716133c087a0) )
	ROM_LOAD16_BYTE( "e18-08.7",     0x0000000, 0x080000, CRC(6a5558cd) SHA1(75b26bcaaa213283e7e0dace69ee58f305b4572d) )
	ROM_LOAD( "e18-02.12",           0x0600000, 0x200000, CRC(c7b4fe29) SHA1(7f823bd61abf2b15d3ba62bca829a5b1acacfd09) )
	ROM_LOAD( "e18-03.16",           0x0800000, 0x200000, CRC(76392346) SHA1(2c5b70c4708208f866feea0472fcc72333061124) )
	ROM_LOAD( "e18-04.19",           0x0a00000, 0x200000, CRC(fc3731da) SHA1(58948aad8d7bb7a8449d2bf12e9d5e6d7b4426b5) )
	ROM_LOAD( "e18-05.21",           0x0c00000, 0x200000, CRC(2e984c50) SHA1(6d8255e38c67d68bf489c9885663ed2edf148188) )

	ROM_REGION( 0x2c000, REGION_CPU2, 0 )     /* 64k for Z80 code */
	ROM_LOAD( "e18-09.22",           0x0000000, 0x004000, CRC(bb5a5319) SHA1(0bb700cafc157d3af663cc9bebb8167487ff2852) )
	ROM_CONTINUE(                    0x0010000, 0x01c000 ) /* banked stuff */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "e18-01.15",           0x0000000, 0x200000, CRC(dbd1408c) SHA1(ef81064f2f95e5ae25eb1f10d1e78f27f9e294f5) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tt02", 0, 8, CRC(b1585ac7) SHA1(d967eea539de2dd7a8d13fa2785c3033065bed9c) )
ROM_END

/* Eighteen Raizing */

#define PSARC95_BIOS \
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "m27c402cz-54.ic353", 0x0000000, 0x080000, CRC(910f3a8b) SHA1(cd68532967a25f476a6d73473ec6b6f4df2e1689) ) \
	ROM_REGION( 0x000008, REGION_USER4, 0 ) /* CAT702, main board */ \
	ROM_LOAD( "et01.ic652", 0, 8, CRC(a7dd922e) SHA1(1069c1d9015028a51a1b314cfacb014ea90aa425) )

ROM_START( coh1002e )
	PSARC95_BIOS
ROM_END

ROM_START( beastrzr )
	PSARC95_BIOS

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "b.roar_u0213.u0213",    0x000001, 0x080000, CRC(2c586534) SHA1(a38dfc3a45446d24a1caac89b0f560989d46ded5) )
	ROM_LOAD16_BYTE( "b.roar_u0212.u0212",    0x000000, 0x080000, CRC(1c85d7fb) SHA1(aa406a42c424cc16a9e5330c68dda9acf8760088) )
	ROM_LOAD16_BYTE( "b.roar_u0215.u0215",    0x100001, 0x080000, CRC(31c8e055) SHA1(2811789ab6221b972d1e3ffe98916587990f7564) )
	ROM_LOAD16_BYTE( "b.roar_u0214.u0214",    0x100000, 0x080000, CRC(1cdc450a) SHA1(9215e5fec52f7c5c0070feb621eb9c77f98e2362) )
	ROM_LOAD( "8ing_ra-b.roar_rom-1.u0217",        0x400000, 0x400000, CRC(11f1ba36) SHA1(d41ae686c2c607640cbadf906215c89134758050) )
	ROM_LOAD( "8ing_ra-b.roar_rom-2.u0216",        0x800000, 0x400000, CRC(d46d46b7) SHA1(1c42cb5dcda4b26c08c4ecf95efeadaf3a1d1dd2) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "b.roar_u046.u046",    0x000001, 0x040000, CRC(d4bb261a) SHA1(9a295b1354ef15f37ea09bb209cf0cb98437c462) )
	ROM_LOAD16_BYTE( "b.roar_u042.u042",    0x000000, 0x040000, CRC(4d537f88) SHA1(1760367d70a81606e29885ea315185d2c2a9409b) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "8ing_ra-b.roar_rom-3.u0326",        0x000000, 0x400000, CRC(b74cc4d1) SHA1(eb5485582a12959ae06927a2f1d8a7e63e0f956f) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "et02", 0, 8, CRC(187ce61a) SHA1(521122b0f7b3f278dd2a2b1d73c252b952b5f55d) )
ROM_END

ROM_START( bldyroar )
	PSARC95_BIOS

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "b.roar_u0213.u0213",    0x000001, 0x080000, CRC(63769342) SHA1(7231188073b997b039467db85ce7c85383daf591) )
	ROM_LOAD16_BYTE( "b.roar_u0212.u0212",    0x000000, 0x080000, CRC(966b7169) SHA1(63e025cacb84e89d30b40ed6cfa5c63d84c298c4) )
	ROM_LOAD16_BYTE( "b.roar_u0215.u0215",    0x100001, 0x080000, CRC(31c8e055) SHA1(2811789ab6221b972d1e3ffe98916587990f7564) )
	ROM_LOAD16_BYTE( "b.roar_u0214.u0214",    0x100000, 0x080000, CRC(1cdc450a) SHA1(9215e5fec52f7c5c0070feb621eb9c77f98e2362) )
	ROM_LOAD( "8ing_ra-b.roar_rom-1.u0217",        0x400000, 0x400000, CRC(11f1ba36) SHA1(d41ae686c2c607640cbadf906215c89134758050) )
	ROM_LOAD( "8ing_ra-b.roar_rom-2.u0216",        0x800000, 0x400000, CRC(d46d46b7) SHA1(1c42cb5dcda4b26c08c4ecf95efeadaf3a1d1dd2) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "b.roar_u046.u046",    0x000001, 0x040000, CRC(d4bb261a) SHA1(9a295b1354ef15f37ea09bb209cf0cb98437c462) )
	ROM_LOAD16_BYTE( "b.roar_u042.u042",    0x000000, 0x040000, CRC(4d537f88) SHA1(1760367d70a81606e29885ea315185d2c2a9409b) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "8ing_ra-b.roar_rom-3.u0326",        0x000000, 0x400000, CRC(b74cc4d1) SHA1(eb5485582a12959ae06927a2f1d8a7e63e0f956f) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "et02", 0, 8, CRC(187ce61a) SHA1(521122b0f7b3f278dd2a2b1d73c252b952b5f55d) )
ROM_END

ROM_START( beastrzra )
	PSARC95_BIOS

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "b.roar_u0213.u0213",    0x000001, 0x080000, CRC(5c3ca4d3) SHA1(f246de057c2ea725e5e628dd32597f3bc7d22070) )
	ROM_LOAD16_BYTE( "b.roar_u0212.u0212",    0x000000, 0x080000, CRC(45f1269b) SHA1(4754bc97c261dfcb5e379fe765689ea737dd5e1d) )
	ROM_LOAD16_BYTE( "b.roar_u0215.u0215",    0x100001, 0x080000, CRC(31c8e055) SHA1(2811789ab6221b972d1e3ffe98916587990f7564) )
	ROM_LOAD16_BYTE( "b.roar_u0214.u0214",    0x100000, 0x080000, CRC(1cdc450a) SHA1(9215e5fec52f7c5c0070feb621eb9c77f98e2362) )
	ROM_LOAD( "8ing_ra-b.roar_rom-1.u0217",        0x400000, 0x400000, BAD_DUMP CRC(11f1ba36) SHA1(d41ae686c2c607640cbadf906215c89134758050) )
	ROM_LOAD( "8ing_ra-b.roar_rom-2.u0216",        0x800000, 0x400000, BAD_DUMP CRC(d46d46b7) SHA1(1c42cb5dcda4b26c08c4ecf95efeadaf3a1d1dd2) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "b.roar_u046.u046",    0x000001, 0x040000, CRC(d4bb261a) SHA1(9a295b1354ef15f37ea09bb209cf0cb98437c462) )
	ROM_LOAD16_BYTE( "b.roar_u042.u042",    0x000000, 0x040000, CRC(4d537f88) SHA1(1760367d70a81606e29885ea315185d2c2a9409b) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "8ing_ra-b.roar_rom-3.u0326",        0x000000, 0x400000, BAD_DUMP CRC(b74cc4d1) SHA1(eb5485582a12959ae06927a2f1d8a7e63e0f956f) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "et02", 0, 8, CRC(187ce61a) SHA1(521122b0f7b3f278dd2a2b1d73c252b952b5f55d) )
ROM_END

ROM_START( beastrzrb )
	PSARC95_BIOS

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "27c160.1",     0x000000, 0x200000, CRC(820855e2) SHA1(18bdd4d0b4a92ae4fde457e1f37c813be6eece71) )
	ROM_LOAD( "27c160.4",     0x400000, 0x200000, CRC(2d2b25f4) SHA1(77d8ad94602e71f16b47de47bc2e0a97957c530b) )
	ROM_LOAD( "27c160.5",     0x600000, 0x200000, CRC(10fe6f4d) SHA1(9faee2faa6d741e1caf25edd093644be5723aa5c) )
	ROM_LOAD( "27c160.2",     0x800000, 0x200000, CRC(1712af34) SHA1(3a78997a2ad0fec1b09828b47150a4be611cd9ad) )
	ROM_LOAD( "27c800.3",     0xb00000, 0x100000, CRC(7192eb4e) SHA1(bb276a38261099d91080d8613dc7500322f6fcab) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
/*  http://www.atmel.com/dyn/products/product_card.asp?family_id=604&family_name=8051+Architecture&part_id=1939 */
	ROM_LOAD( "at89c4051",    0x000000, 0x001000, NO_DUMP ) /* cat 702 replacement or sound cpu? */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "27c4096.1",    0x000000, 0x080000, CRC(217734a1) SHA1(de4f519215123c09b3b5f27509b4d74604b5e03d) )
	ROM_LOAD( "27c4096.2",    0x080000, 0x080000, CRC(d1f2a9b2) SHA1(d1475a453ce4e3b9f2ff59abedf0f57ba3c408fe) )
	ROM_LOAD( "27c240.3",     0x100000, 0x080000, BAD_DUMP CRC(509cdc8b) SHA1(8b92b79be09de56e7d40c2d02fcbeca92bb60226) ) /* replay attack, upper & lower bytes identical */

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "et02", 0, 8, CRC(187ce61a) SHA1(521122b0f7b3f278dd2a2b1d73c252b952b5f55d) )
ROM_END

ROM_START( brvblade )
	TPS_BIOS

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "flash0.021",      0x0000000, 0x200000, CRC(97e12c63) SHA1(382970617a363f6c98ee741f26be6a75c9752bdb) )
	ROM_LOAD( "flash1.024",      0x0200000, 0x200000, CRC(d9d40a34) SHA1(c91dbc6f85404e9397fa79a4bac28e8c3c1a5228) )
	ROM_LOAD( "ra-bbl_rom1.028", 0x0800000, 0x400000, CRC(418535e0) SHA1(7c443e651704f2cd552565c35f4a93f2dc250558) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "spu1u412.bin", 0x0000001, 0x080000, CRC(6408b5b2) SHA1(ba60aa1074df87e98fa260211e9ec99cea25023f) )
	ROM_LOAD16_BYTE( "spu0u049.bin", 0x0000000, 0x080000, CRC(c9df8ed9) SHA1(00a58522189091c48d781b6703e4378e04343c33) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "ra-bbl_rom2.336", 0x000000, 0x400000, CRC(cd052c02) SHA1(d955a70a89b3b1a0b505a05c0887c399fe7a2c68) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg11", 0, 8, CRC(56d917de) SHA1(3c1cdbf552265dbff6051853d933ac34047bcdae) )

	ROM_REGION( 0x000800, REGION_USER6, 0 ) /* factory EEPROM */
	ROM_LOAD( "at28c16_world", 0, 0x800, CRC(fe7f7d34) SHA1(18f6ae14e57afa668b3eef821b4cf0a7599a21ac) )
ROM_END

ROM_START( brvbladej )
	TPS_BIOS

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "flash0.021",      0x0000000, 0x200000, CRC(97e12c63) SHA1(382970617a363f6c98ee741f26be6a75c9752bdb) )
	ROM_LOAD( "flash1.024",      0x0200000, 0x200000, CRC(d9d40a34) SHA1(c91dbc6f85404e9397fa79a4bac28e8c3c1a5228) )
	ROM_LOAD( "ra-bbl_rom1.028", 0x0800000, 0x400000, CRC(418535e0) SHA1(7c443e651704f2cd552565c35f4a93f2dc250558) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "spu1u412.bin", 0x0000001, 0x080000, CRC(6408b5b2) SHA1(ba60aa1074df87e98fa260211e9ec99cea25023f) )
	ROM_LOAD16_BYTE( "spu0u049.bin", 0x0000000, 0x080000, CRC(c9df8ed9) SHA1(00a58522189091c48d781b6703e4378e04343c33) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "ra-bbl_rom2.336", 0x000000, 0x400000, CRC(cd052c02) SHA1(d955a70a89b3b1a0b505a05c0887c399fe7a2c68) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg11", 0, 8, CRC(56d917de) SHA1(3c1cdbf552265dbff6051853d933ac34047bcdae) )

	ROM_REGION( 0x000800, REGION_USER6, 0 ) /* factory EEPROM */
	ROM_LOAD( "at28c16_japan", 0, 0x800, CRC(59e2d326) SHA1(53f48b6ad7243aa92b54863515f104a7a54b2810) )
ROM_END

ROM_START( brvbladea )
	TPS_BIOS

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "flash0.021",      0x0000000, 0x200000, CRC(97e12c63) SHA1(382970617a363f6c98ee741f26be6a75c9752bdb) )
	ROM_LOAD( "flash1.024",      0x0200000, 0x200000, CRC(d9d40a34) SHA1(c91dbc6f85404e9397fa79a4bac28e8c3c1a5228) )
	ROM_LOAD( "ra-bbl_rom1.028", 0x0800000, 0x400000, CRC(418535e0) SHA1(7c443e651704f2cd552565c35f4a93f2dc250558) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "spu1u412.bin", 0x0000001, 0x080000, CRC(6408b5b2) SHA1(ba60aa1074df87e98fa260211e9ec99cea25023f) )
	ROM_LOAD16_BYTE( "spu0u049.bin", 0x0000000, 0x080000, CRC(c9df8ed9) SHA1(00a58522189091c48d781b6703e4378e04343c33) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "ra-bbl_rom2.336", 0x000000, 0x400000, CRC(cd052c02) SHA1(d955a70a89b3b1a0b505a05c0887c399fe7a2c68) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg11", 0, 8, CRC(56d917de) SHA1(3c1cdbf552265dbff6051853d933ac34047bcdae) )

	ROM_REGION( 0x000800, REGION_USER6, 0 ) /* factory EEPROM */
	ROM_LOAD( "at28c16_asia", 0, 0x800, CRC(adb1aa3d) SHA1(118a17b8a15108666e4f9f2d1798031b3b893536) )
ROM_END

ROM_START( brvbladeu )
	TPS_BIOS

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "flash0.021",      0x0000000, 0x200000, CRC(97e12c63) SHA1(382970617a363f6c98ee741f26be6a75c9752bdb) )
	ROM_LOAD( "flash1.024",      0x0200000, 0x200000, CRC(d9d40a34) SHA1(c91dbc6f85404e9397fa79a4bac28e8c3c1a5228) )
	ROM_LOAD( "ra-bbl_rom1.028", 0x0800000, 0x400000, CRC(418535e0) SHA1(7c443e651704f2cd552565c35f4a93f2dc250558) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "spu1u412.bin", 0x0000001, 0x080000, CRC(6408b5b2) SHA1(ba60aa1074df87e98fa260211e9ec99cea25023f) )
	ROM_LOAD16_BYTE( "spu0u049.bin", 0x0000000, 0x080000, CRC(c9df8ed9) SHA1(00a58522189091c48d781b6703e4378e04343c33) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "ra-bbl_rom2.336", 0x000000, 0x400000, CRC(cd052c02) SHA1(d955a70a89b3b1a0b505a05c0887c399fe7a2c68) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "mg11", 0, 8, CRC(56d917de) SHA1(3c1cdbf552265dbff6051853d933ac34047bcdae) )

	ROM_REGION( 0x000800, REGION_USER6, 0 ) /* factory EEPROM */
	ROM_LOAD( "at28c16_usa", 0, 0x800, CRC(0a2c042f) SHA1(147651d2e55873a82295214b3b0bd6c46cdad239) )
ROM_END

ROM_START( bldyror2 )
	PSARC95_BIOS

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "flash0.021",      0x0000000, 0x200000, CRC(fa7602e1) SHA1(6fb6af09656fbb86d2abda35804b2ed4a4cd7461) )
	ROM_LOAD( "flash1.024",      0x0200000, 0x200000, CRC(03465a69) SHA1(7c29aff2bf19c379873d3927c260892c78281882) )
	ROM_LOAD( "rom-1a.028",      0x0800000, 0x400000, CRC(0e711461) SHA1(1d0bd80e6885432ef0623babde28e5760b714bfa) )
	ROM_LOAD( "rom-1b.29",       0x0c00000, 0x400000, CRC(0cf153f9) SHA1(53bb9f8642079f56d8e925792b069362df666819) )
	ROM_LOAD( "rom-2a.026",      0x1000000, 0x400000, CRC(b71d955d) SHA1(49fce452c70ceafc8a149fa9ff073589b7261882) )
	ROM_LOAD( "rom-2b.210",      0x1400000, 0x400000, CRC(89959dde) SHA1(99d54b9876f38f5e625334bbd1439618cdf01d56) )

	ROM_REGION32_LE( 0x0400000, REGION_USER3, ROMREGION_ERASE00 )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "br2_u0412.412", 0x000001, 0x080000, CRC(e254dd8a) SHA1(5b8fcafcf2176e0b55efcf37799d7c0d97e01bdc) )
	ROM_LOAD16_BYTE( "br2_u049.049",  0x000000, 0x080000, CRC(10dc855b) SHA1(4e6e3a71911c8976ae07c2b6cac5a36f98193def) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "rom-3.336",       0x000000, 0x400000, CRC(b74cc4d1) SHA1(eb5485582a12959ae06927a2f1d8a7e63e0f956f) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "et03", 0, 8, CRC(779b0bfd) SHA1(76a188c78083bbb2740379d53143e1efaf637b85) )

	ROM_REGION( 0x000800, REGION_USER6, 0 ) /* factory EEPROM */
	ROM_LOAD( "at28c16_world", 0, 0x800, CRC(01b42397) SHA1(853553a38e81e64a17c040173b29c7bfd6f79f31) )
ROM_END

ROM_START( bldyror2j )
	PSARC95_BIOS

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "flash0.021",      0x0000000, 0x200000, CRC(fa7602e1) SHA1(6fb6af09656fbb86d2abda35804b2ed4a4cd7461) )
	ROM_LOAD( "flash1.024",      0x0200000, 0x200000, CRC(03465a69) SHA1(7c29aff2bf19c379873d3927c260892c78281882) )
	ROM_LOAD( "rom-1a.028",      0x0800000, 0x400000, CRC(0e711461) SHA1(1d0bd80e6885432ef0623babde28e5760b714bfa) )
	ROM_LOAD( "rom-1b.29",       0x0c00000, 0x400000, CRC(0cf153f9) SHA1(53bb9f8642079f56d8e925792b069362df666819) )
	ROM_LOAD( "rom-2a.026",      0x1000000, 0x400000, CRC(b71d955d) SHA1(49fce452c70ceafc8a149fa9ff073589b7261882) )
	ROM_LOAD( "rom-2b.210",      0x1400000, 0x400000, CRC(89959dde) SHA1(99d54b9876f38f5e625334bbd1439618cdf01d56) )

	ROM_REGION32_LE( 0x0400000, REGION_USER3, ROMREGION_ERASE00 )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "br2_u0412.412", 0x000001, 0x080000, CRC(e254dd8a) SHA1(5b8fcafcf2176e0b55efcf37799d7c0d97e01bdc) )
	ROM_LOAD16_BYTE( "br2_u049.049",  0x000000, 0x080000, CRC(10dc855b) SHA1(4e6e3a71911c8976ae07c2b6cac5a36f98193def) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "rom-3.336",       0x000000, 0x400000, CRC(b74cc4d1) SHA1(eb5485582a12959ae06927a2f1d8a7e63e0f956f) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "et03", 0, 8, CRC(779b0bfd) SHA1(76a188c78083bbb2740379d53143e1efaf637b85) )

	ROM_REGION( 0x000800, REGION_USER6, 0 ) /* factory EEPROM */
	ROM_LOAD( "at28c16_japan", 0, 0x800, CRC(6cb55630) SHA1(b840bc0339485dd82f7c9aa669faf90ae371218f) )
ROM_END

ROM_START( bldyror2a )
	PSARC95_BIOS

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "flash0.021",      0x0000000, 0x200000, CRC(fa7602e1) SHA1(6fb6af09656fbb86d2abda35804b2ed4a4cd7461) )
	ROM_LOAD( "flash1.024",      0x0200000, 0x200000, CRC(03465a69) SHA1(7c29aff2bf19c379873d3927c260892c78281882) )
	ROM_LOAD( "rom-1a.028",      0x0800000, 0x400000, CRC(0e711461) SHA1(1d0bd80e6885432ef0623babde28e5760b714bfa) )
	ROM_LOAD( "rom-1b.29",       0x0c00000, 0x400000, CRC(0cf153f9) SHA1(53bb9f8642079f56d8e925792b069362df666819) )
	ROM_LOAD( "rom-2a.026",      0x1000000, 0x400000, CRC(b71d955d) SHA1(49fce452c70ceafc8a149fa9ff073589b7261882) )
	ROM_LOAD( "rom-2b.210",      0x1400000, 0x400000, CRC(89959dde) SHA1(99d54b9876f38f5e625334bbd1439618cdf01d56) )

	ROM_REGION32_LE( 0x0400000, REGION_USER3, ROMREGION_ERASE00 )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "br2_u0412.412", 0x000001, 0x080000, CRC(e254dd8a) SHA1(5b8fcafcf2176e0b55efcf37799d7c0d97e01bdc) )
	ROM_LOAD16_BYTE( "br2_u049.049",  0x000000, 0x080000, CRC(10dc855b) SHA1(4e6e3a71911c8976ae07c2b6cac5a36f98193def) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "rom-3.336",       0x000000, 0x400000, CRC(b74cc4d1) SHA1(eb5485582a12959ae06927a2f1d8a7e63e0f956f) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "et03", 0, 8, CRC(779b0bfd) SHA1(76a188c78083bbb2740379d53143e1efaf637b85) )

	ROM_REGION( 0x000800, REGION_USER6, 0 ) /* factory EEPROM */
	ROM_LOAD( "at28c16_asia", 0, 0x800, CRC(da8c1a64) SHA1(14cbb751f498c96d9d8fce3eea3781ebc45f6291) )
ROM_END

ROM_START( bldyror2u )
	PSARC95_BIOS

	ROM_REGION32_LE( 0x1800000, REGION_USER2, 0 )
	ROM_LOAD( "flash0.021",      0x0000000, 0x200000, CRC(fa7602e1) SHA1(6fb6af09656fbb86d2abda35804b2ed4a4cd7461) )
	ROM_LOAD( "flash1.024",      0x0200000, 0x200000, CRC(03465a69) SHA1(7c29aff2bf19c379873d3927c260892c78281882) )
	ROM_LOAD( "rom-1a.028",      0x0800000, 0x400000, CRC(0e711461) SHA1(1d0bd80e6885432ef0623babde28e5760b714bfa) )
	ROM_LOAD( "rom-1b.29",       0x0c00000, 0x400000, CRC(0cf153f9) SHA1(53bb9f8642079f56d8e925792b069362df666819) )
	ROM_LOAD( "rom-2a.026",      0x1000000, 0x400000, CRC(b71d955d) SHA1(49fce452c70ceafc8a149fa9ff073589b7261882) )
	ROM_LOAD( "rom-2b.210",      0x1400000, 0x400000, CRC(89959dde) SHA1(99d54b9876f38f5e625334bbd1439618cdf01d56) )

	ROM_REGION32_LE( 0x0400000, REGION_USER3, ROMREGION_ERASE00 )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "br2_u0412.412", 0x000001, 0x080000, CRC(e254dd8a) SHA1(5b8fcafcf2176e0b55efcf37799d7c0d97e01bdc) )
	ROM_LOAD16_BYTE( "br2_u049.049",  0x000000, 0x080000, CRC(10dc855b) SHA1(4e6e3a71911c8976ae07c2b6cac5a36f98193def) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "rom-3.336",       0x000000, 0x400000, CRC(b74cc4d1) SHA1(eb5485582a12959ae06927a2f1d8a7e63e0f956f) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "et03", 0, 8, CRC(779b0bfd) SHA1(76a188c78083bbb2740379d53143e1efaf637b85) )

	ROM_REGION( 0x000800, REGION_USER6, 0 ) /* factory EEPROM */
	ROM_LOAD( "at28c16_usa", 0, 0x800, CRC(b78d6fc3) SHA1(49d8b6f44c31d74f36cba981af7f4c7e23dd9007) )
ROM_END

/* Atari PSX */

#define TW_BIOS \
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "msm27c402zb.ic353", 0x000000, 0x080000, CRC(45e8a4b4) SHA1(815488d8563c85f97fbc3384ff21f08e4c88b7b7) ) \
	ROM_REGION( 0x000008, REGION_USER4, 0 ) /* CAT702, main board */ \
	ROM_LOAD( "tw01.ic652", 0, 8, CRC(1cac067e) SHA1(8e87941bfb374172628c8a37647e3a66b7166aa8) )

ROM_START( coh1000w )
	TW_BIOS
ROM_END

ROM_START( primrag2 )
	TW_BIOS

	ROM_REGION32_LE( 0x200000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "pr2_036.u16",  0x000001, 0x080000, CRC(3ee39e4f) SHA1(dbd859b54fb9be33effc14eb847dcd829024eea3) )
	ROM_LOAD16_BYTE( "pr2_036.u14",  0x000000, 0x080000, CRC(c86450cd) SHA1(19c3c50d839a9efb6ffa9ada8a072f56697c1abb) )
	ROM_LOAD16_BYTE( "pr2_036.u17",  0x100001, 0x080000, CRC(3681516c) SHA1(714f73ea4ac190c36a6eb2308616a4aecabc4e69) )
	ROM_LOAD16_BYTE( "pr2_036.u15",  0x100000, 0x080000, CRC(4b24bd54) SHA1(7f27cd524d10e5869aab6d4dc6a4217d049c475d) )

	ROM_REGION( 0x000800, REGION_USER6, 0 ) /* factory EEPROM */
	ROM_LOAD( "at28c16", 0, 0x800, CRC(62c21283) SHA1(6013d831fb97c025682603a1bb9d62bd93e1b893) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "tw02", 0, 8, CRC(583e8e1c) SHA1(0af99e3feb3fa684883a5daefba2c38a442f3884) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "primrag2", 0, SHA1(bc615068ddf4fd967f770ee01c02f285c052c4c5) )
ROM_END

/* Acclaim PSX */

#define AC_BIOS \
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "m534002c-15.ic353", 0x0000000, 0x080000, CRC(8d8d0764) SHA1(7ee83d317190bb1cef2f8f01c81eaaae47150ebb) ) \
	ROM_REGION( 0x000008, REGION_USER4, 0 ) /* CAT702, main board */ \
	ROM_LOAD( "ac01.ic652", 0, 8, CRC(c425eaf5) SHA1(decf77ab50f7f58c641c3214a11647924f3a8998) )

ROM_START( coh1000a )
	AC_BIOS
ROM_END

ROM_START( nbajamexa )
	AC_BIOS

	ROM_REGION32_LE( 0x2000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "360-mp-a1_odd_cs_e0fe.u36", 0x0000001, 0x100000, CRC(c433e827) SHA1(1d2a5a6990a1b1864e63ce3ba7306d48ebbd4775) )
	ROM_LOAD16_BYTE( "360-mp-a1_even_cs_295a.u35", 0x0000000, 0x100000, CRC(d8f5b2f7) SHA1(e38609d314721b8b612e047406e2888395917b0d) )
	ROM_LOAD16_BYTE( "nba0o.u28",   0x0200001, 0x200000, CRC(be13c5af) SHA1(eee5c9d985384ecfe4f00fae27d66fbefc15b28e) )
	ROM_LOAD16_BYTE( "nba0e.u41",   0x0200000, 0x200000, CRC(077f4355) SHA1(63c52bb82943b52bb0906d114acd5ea8643068b6) )
	ROM_LOAD16_BYTE( "nba1o.u29",   0x0600001, 0x200000, CRC(3650e85b) SHA1(a36bfa235c8e3bb516e178f54d3c5e3955c7e918) )
	ROM_LOAD16_BYTE( "nba1e.u42",   0x0600000, 0x200000, CRC(f1212cf9) SHA1(b2f80af3ec4d559056e86f695d89d1d32b500f50) )
	ROM_LOAD16_BYTE( "nba2o.u30",   0x0a00001, 0x200000, CRC(ccbb6125) SHA1(998eda99182b984f88f5fc58095cb35bf232a26b) )
	ROM_LOAD16_BYTE( "nba2e.u43",   0x0a00000, 0x200000, CRC(c20ab628) SHA1(7ffe5005e1913da56770452ae2f907a4a270ab24) )
	ROM_LOAD16_BYTE( "nba3o.u31",   0x0e00001, 0x200000, CRC(d5238edf) SHA1(d1eb30ec65dd6cfa8cbb2b36af3a83820d1de99a) )
	ROM_LOAD16_BYTE( "nba3e.u44",   0x0e00000, 0x200000, CRC(07ba00a3) SHA1(c14bffd35ee715b07d6065b454b0443438ab6536) )
	ROM_LOAD16_BYTE( "nba4o.u3",    0x1200001, 0x200000, CRC(1cf16a34) SHA1(a7e984a2db846854f1c4a9a2fdefd0d17ce3108c) )
	ROM_LOAD16_BYTE( "nba4e.u17",   0x1200000, 0x200000, CRC(b5977765) SHA1(08acdfe413a5a9182ca117f44b7acac9dac9ecea) )
	ROM_LOAD16_BYTE( "nba5o.u4",    0x1600001, 0x200000, CRC(5272754b) SHA1(c35ba5377eb812991e4bf0d954a34af90b986341) )
	ROM_LOAD16_BYTE( "nba5e.u18",   0x1600000, 0x200000, CRC(0eb917da) SHA1(d6c8991ba7cd492668757658ee64078d0e82b596) )
	ROM_LOAD16_BYTE( "nba6o.u5",    0x1a00001, 0x200000, CRC(b1dfb42e) SHA1(fb9627e228bf2a744842eb44afbca4a6232cadb2) )
	ROM_LOAD16_BYTE( "nba6e.u19",   0x1a00000, 0x200000, CRC(6f17d8c1) SHA1(22cf263efb64cf62030e02b641c485debe75944d) )

	ROM_REGION( 0x0a0000, REGION_CPU2, 0 ) /* 512k for the audio CPU, ADSP-2181 (+banks) */
	ROM_LOAD( "360snda1.u52", 0x000000, 0x08000, CRC(36d8a628) SHA1(944a01c9128f5e90c7dba3557a3ecb2c5ca90831) )
	ROM_CONTINUE(             0x010000, 0x78000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "sound0.u48",   0x000000, 0x200000, CRC(38873b67) SHA1(b2f8d32270ae604c099a1b9b71d2e06468c7d4a9) )
	ROM_LOAD( "sound1.u49",   0x200000, 0x200000, CRC(57014589) SHA1(d360ff1c52424bd91a5a8d1a2a9c10bf7abb0602) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "ac02", 0, 8, CRC(1412d475) SHA1(c2f62232a261870f58353d09dc0d6ce2ad17a729) )
ROM_END

ROM_START( nbajamex )
	AC_BIOS

	ROM_REGION32_LE( 0x2000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "nba_jam_110i_odd_cs_cc48.u36", 0x0000001, 0x100000, CRC(533041e7) SHA1(691a11a031e61d892db74c4c29b6a999917c341b) )
	ROM_LOAD16_BYTE( "nba_jam_110i_even_cs_9567.u35", 0x0000000, 0x100000, CRC(63d7f15d) SHA1(23525e875d0e61763301e95f70964bc9df3a2624) )
	ROM_LOAD16_BYTE( "nba0o.u28",   0x0200001, 0x200000, CRC(be13c5af) SHA1(eee5c9d985384ecfe4f00fae27d66fbefc15b28e) )
	ROM_LOAD16_BYTE( "nba0e.u41",   0x0200000, 0x200000, CRC(077f4355) SHA1(63c52bb82943b52bb0906d114acd5ea8643068b6) )
	ROM_LOAD16_BYTE( "nba1o.u29",   0x0600001, 0x200000, CRC(3650e85b) SHA1(a36bfa235c8e3bb516e178f54d3c5e3955c7e918) )
	ROM_LOAD16_BYTE( "nba1e.u42",   0x0600000, 0x200000, CRC(f1212cf9) SHA1(b2f80af3ec4d559056e86f695d89d1d32b500f50) )
	ROM_LOAD16_BYTE( "nba2o.u30",   0x0a00001, 0x200000, CRC(ccbb6125) SHA1(998eda99182b984f88f5fc58095cb35bf232a26b) )
	ROM_LOAD16_BYTE( "nba2e.u43",   0x0a00000, 0x200000, CRC(c20ab628) SHA1(7ffe5005e1913da56770452ae2f907a4a270ab24) )
	ROM_LOAD16_BYTE( "nba3o.u31",   0x0e00001, 0x200000, CRC(d5238edf) SHA1(d1eb30ec65dd6cfa8cbb2b36af3a83820d1de99a) )
	ROM_LOAD16_BYTE( "nba3e.u44",   0x0e00000, 0x200000, CRC(07ba00a3) SHA1(c14bffd35ee715b07d6065b454b0443438ab6536) )
	ROM_LOAD16_BYTE( "nba4o.u3",    0x1200001, 0x200000, CRC(1cf16a34) SHA1(a7e984a2db846854f1c4a9a2fdefd0d17ce3108c) )
	ROM_LOAD16_BYTE( "nba4e.u17",   0x1200000, 0x200000, CRC(b5977765) SHA1(08acdfe413a5a9182ca117f44b7acac9dac9ecea) )
	ROM_LOAD16_BYTE( "nba5o.u4",    0x1600001, 0x200000, CRC(5272754b) SHA1(c35ba5377eb812991e4bf0d954a34af90b986341) )
	ROM_LOAD16_BYTE( "nba5e.u18",   0x1600000, 0x200000, CRC(0eb917da) SHA1(d6c8991ba7cd492668757658ee64078d0e82b596) )
	ROM_LOAD16_BYTE( "nba6o.u5",    0x1a00001, 0x200000, CRC(b1dfb42e) SHA1(fb9627e228bf2a744842eb44afbca4a6232cadb2) )
	ROM_LOAD16_BYTE( "nba6e.u19",   0x1a00000, 0x200000, CRC(6f17d8c1) SHA1(22cf263efb64cf62030e02b641c485debe75944d) )

	ROM_REGION( 0x0a0000, REGION_CPU2, 0 ) /* 512k for the audio CPU, ADSP-2181 (+banks) */
	ROM_LOAD( "360snda1.u52", 0x000000, 0x08000, CRC(36d8a628) SHA1(944a01c9128f5e90c7dba3557a3ecb2c5ca90831) )
	ROM_CONTINUE(             0x010000, 0x78000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "sound0.u48",   0x000000, 0x200000, CRC(38873b67) SHA1(b2f8d32270ae604c099a1b9b71d2e06468c7d4a9) )
	ROM_LOAD( "sound1.u49",   0x200000, 0x200000, CRC(57014589) SHA1(d360ff1c52424bd91a5a8d1a2a9c10bf7abb0602) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "ac02", 0, 8, CRC(1412d475) SHA1(c2f62232a261870f58353d09dc0d6ce2ad17a729) )
ROM_END

ROM_START( jdredd )
	AC_BIOS

	ROM_REGION32_LE( 0x200000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "9e54_01-16-98_1566_u_36.u36",  0x000001, 0x020000, CRC(37addbf9) SHA1(a4061a1ba9e230f080f0bfea69bf77efe9264a92) )
	ROM_LOAD16_BYTE( "79d3_01-16-98_1565_u_35.u35",  0x000000, 0x020000, CRC(c1e17191) SHA1(82901439b1a51b9aadb4df4b9d944f26697a1460) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "jdreddc", 0, MD5(b66074f3df1e90ec4e3ff09dcdf33033) SHA1(83ed8df25d100b1c060f4dde2f162ba31803db7d) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "ac02", 0, 8, CRC(1412d475) SHA1(c2f62232a261870f58353d09dc0d6ce2ad17a729) )
ROM_END

ROM_START( jdreddb )
	AC_BIOS

	ROM_REGION32_LE( 0x200000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "9e54_11-21-97_1566_u_36.u36",  0x000001, 0x020000, CRC(37addbf9) SHA1(a4061a1ba9e230f080f0bfea69bf77efe9264a92) )
	ROM_LOAD16_BYTE( "79d3_11-21-97_1565_u_35.u35",  0x000000, 0x020000, CRC(c1e17191) SHA1(82901439b1a51b9aadb4df4b9d944f26697a1460) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "jdreddb", 0, MD5(0da1d048d7223df74fca4f349473cefa) SHA1(9b810e3a16de62cabfc8271b6606574c7034cf41) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "ac02", 0, 8, CRC(1412d475) SHA1(c2f62232a261870f58353d09dc0d6ce2ad17a729) )
ROM_END

/* Atlus */

#define ATLUS_BIOS \
	ROM_REGION32_LE( 0x080000, REGION_USER1, 0 ) \
	ROM_LOAD( "m534002c-17.ic353", 0x0000000, 0x080000, CRC(6721146b) SHA1(9511d24bfe25eb180fb2db0835b131cb4a12730e) ) \
	ROM_REGION( 0x000008, REGION_USER4, 0 ) /* CAT702, main board */ \
	ROM_LOAD( "at01.ic652", 0, 8, CRC(181668b3) SHA1(8d2fe1facb4473cc20b4b477aaf881f9280939cf) )

ROM_START( coh1001l )
	ATLUS_BIOS
ROM_END

ROM_START( hvnsgate )
	ATLUS_BIOS

	ROM_REGION32_LE( 0x02000000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "athg-01b.18",  0x0000001, 0x080000, CRC(e820136f) SHA1(2bc3465928dd08060736a2a67d98864d634275d6) )
	ROM_LOAD16_BYTE( "athg-02b.17",  0x0000000, 0x080000, CRC(11bfa89b) SHA1(f23e4c9d8eb90bd3bb3327d9950edd7a467ce8da) )
	ROM_LOAD( "athg-07.027",         0x0100000, 0x400000, CRC(46411f67) SHA1(2e8f37c3d9d7f5f3c79fca8ffeaf4c2fd1634b91) )
	ROM_LOAD( "athg-08.028",         0x0500000, 0x400000, CRC(85289345) SHA1(6385fe27451b80f97e7bad823b3b59eff3efa541) )
	ROM_LOAD( "athg-09.210",         0x0900000, 0x400000, CRC(19e558b5) SHA1(c195bc7dc3cfe4f099d27afdebd6f9cfe064e1df) )
	ROM_LOAD( "athg-10.029",         0x0d00000, 0x400000, CRC(748f936e) SHA1(134e78ea71bb9646f36cc503c704496a2b622ee9) )
	ROM_LOAD( "athg-11.215",         0x1100000, 0x200000, CRC(ac8e53bd) SHA1(002c4be1aa57d810c5d810c475631d9f14e1d2ab) )

	ROM_REGION( 0x040000, REGION_CPU2, 0 ) /* 68000 code, 10.000MHz */
	ROM_LOAD16_BYTE( "athg-04.21",   0x000001, 0x020000, CRC(18523e85) SHA1(0ecc2116760f05fca8e5366b0a97dfe26fa9bc0c) )
	ROM_LOAD16_BYTE( "athg-03.22",   0x000000, 0x020000, CRC(7eef7e68) SHA1(65b8ae18ef4ff636c548326a360b481aeb316869) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* YMZ280B Sound Samples */
	ROM_LOAD( "athg-05.4136", 0x000000, 0x200000, CRC(74469a15) SHA1(0faa883900d7fd2e5240f486db33b3d868f1f05f) )
	ROM_LOAD( "athg-06.4134", 0x200000, 0x200000, CRC(443ade73) SHA1(6ef6aa68c525b9749833125dcab929d1d65d3b90) )

	ROM_REGION( 0x000008, REGION_USER5, 0 ) /* CAT702, ROM board */
	ROM_LOAD( "at02", 0, 8, CRC(55fe469e) SHA1(a99d41e21bd1df67b387558314763864eaae0265) )
ROM_END

SYSTEM_BIOS_START( coh1000c )
	SYSTEM_BIOS_ADD( 0, "j", "japan" )
	SYSTEM_BIOS_ADD( 1, "e", "english (debug)" )
SYSTEM_BIOS_END

/* Capcom ZN1 */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the bios file, so that we do not have to include */
/* it in every zip file */
BIOSB( 1995, coh1000c, 0, coh1000c, coh1000c, capcom6b, coh1000c, ROT0, "Capcom", "Capcom ZN-1", 0 )

GAMEB( 1995, ts2, coh1000c, coh1000c, coh1000c, capcom6b, coh1000c, ROT0, "Capcom / Takara", "Battle Arena Toshinden 2 (Europe 951124)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1995, ts2u, ts2, coh1000c, coh1000c, capcom6b, coh1000c, ROT0, "Capcom / Takara", "Battle Arena Toshinden 2 (USA 951124)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1995, ts2ua, ts2, coh1000c, coh1000c, capcom6b, coh1000c, ROT0, "Capcom / Takara", "Battle Arena Toshinden 2 (USA 951124, 32Mb mask ROMs)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1995, ts2j, ts2, coh1000c, coh1000c, capcom6b, coh1000c, ROT0, "Capcom / Takara", "Battle Arena Toshinden 2 (Japan 951124)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1995, ts2ja, ts2, coh1000c, coh1000c, capcom6b, coh1000c, ROT0, "Capcom / Takara", "Battle Arena Toshinden 2 (Japan 951124, 32Mb mask ROMs)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1996, starglad, coh1000c, coh1000c, coh1000c, capcom4b, coh1000c, ROT0, "Capcom", "Star Gladiator Episode I: Final Crusade (USA 960627)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1996, stargladj, starglad, coh1000c, coh1000c, capcom4b, coh1000c, ROT0, "Capcom", "Star Gladiator Episode I: Final Crusade (Japan 960627)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1996, glpracr, coh1000c, coh1000c, coh1000c, glpracr, coh1000c, ROT0, "Tecmo", "Gallop Racer (English Ver 10.17.K)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1996, glpracrj, glpracr, coh1000c, coh1000c, glpracr, coh1000c, ROT0, "Tecmo", "Gallop Racer (Japanese Ver 9.01.12)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1996, sfex, coh1000c, coh1000c, coh1002c, capcom6b, coh1000c, ROT0, "Capcom / Arika", "Street Fighter EX (Europe 961219)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1996, sfexu, sfex, coh1000c, coh1002c, capcom6b, coh1000c, ROT0, "Capcom / Arika", "Street Fighter EX (USA 961219)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1996, sfexa, sfex, coh1000c, coh1002c, capcom6b, coh1000c, ROT0, "Capcom / Arika", "Street Fighter EX (Asia 961219)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1996, sfexj, sfex, coh1000c, coh1002c, capcom6b, coh1000c, ROT0, "Capcom / Arika", "Street Fighter EX (Japan 961130)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1997, sfexp, coh1000c, coh1000c, coh1002c, capcom6b, coh1000c, ROT0, "Capcom / Arika", "Street Fighter EX Plus (USA 970407)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1997, sfexpu1, sfexp, coh1000c, coh1002c, capcom6b, coh1000c, ROT0, "Capcom / Arika", "Street Fighter EX Plus (USA 970311)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1997, sfexpj, sfexp, coh1000c, coh1002c, capcom6b, coh1000c, ROT0, "Capcom / Arika", "Street Fighter EX Plus (Japan 970407)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1997, sfexpj1, sfexp, coh1000c, coh1002c, capcom6b, coh1000c, ROT0, "Capcom / Arika", "Street Fighter EX Plus (Japan 970311)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

/* Capcom ZN2 */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the bios file, so that we do not have to include */
/* it in every zip file */
BIOS( 1997, coh3002c, 0, coh3002c, capcom6b, coh3002c, ROT0, "Capcom", "Capcom ZN-2", 0 )

GAME( 1997, rvschool, coh3002c, coh3002c, capcom4b, coh3002c, ROT0, "Capcom", "Rival Schools: United By Fate (Europe 971117)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, rvschoolu, rvschool, coh3002c, capcom4b, coh3002c, ROT0, "Capcom", "Rival Schools: United By Fate (USA 971117)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, rvschoola, rvschool, coh3002c, capcom4b, coh3002c, ROT0, "Capcom", "Rival Schools: United By Fate (Asia 971117)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, jgakuen, rvschool, coh3002c, capcom4b, coh3002c, ROT0, "Capcom", "Shiritsu Justice Gakuen: Legion of Heroes (Japan 971216)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, jgakuen1, rvschool, coh3002c, capcom4b, coh3002c, ROT0, "Capcom", "Shiritsu Justice Gakuen: Legion of Heroes (Japan 971117)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, sfex2, coh3002c, coh3002c, capcom6b, coh3002c, ROT0, "Capcom / Arika", "Street Fighter EX2 (Europe 980312)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, sfex2u, sfex2, coh3002c, capcom6b, coh3002c, ROT0, "Capcom / Arika", "Street Fighter EX2 (USA 980526)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, sfex2u1, sfex2, coh3002c, capcom6b, coh3002c, ROT0, "Capcom / Arika", "Street Fighter EX2 (USA 980312)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, sfex2a, sfex2, coh3002c, capcom6b, coh3002c, ROT0, "Capcom / Arika", "Street Fighter EX2 (Asia 980312)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, sfex2h, sfex2, coh3002c, capcom6b, coh3002c, ROT0, "Capcom / Arika", "Street Fighter EX2 (Hispanic 980312)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, sfex2j, sfex2, coh3002c, capcom6b, coh3002c, ROT0, "Capcom / Arika", "Street Fighter EX2 (Japan 980312)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, plsmaswd, coh3002c, coh3002c, plsmaswd, coh3002c, ROT0, "Capcom", "Plasma Sword: Nightmare of Bilstein (USA 980316)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, plsmaswda, plsmaswd, coh3002c, plsmaswd, coh3002c, ROT0, "Capcom", "Plasma Sword: Nightmare of Bilstein (Asia 980316)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, stargld2, plsmaswd, coh3002c, plsmaswd, coh3002c, ROT0, "Capcom", "Star Gladiator 2: Nightmare of Bilstein (Japan 980316)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, tgmj, coh3002c, coh3002c, tgmj, coh3002c, ROT0, "Arika / Capcom", "Tetris: The Grand Master (Japan 980710)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, techromn, coh3002c, coh3002c, plsmaswd, coh3002c, ROT0, "Capcom", "Tech Romancer (Europe 980914)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, techromnu, techromn, coh3002c, plsmaswd, coh3002c, ROT0, "Capcom", "Tech Romancer (USA 980914)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, techromna, techromn, coh3002c, plsmaswd, coh3002c, ROT0, "Capcom", "Tech Romancer (Asia 980914)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, kikaioh, techromn, coh3002c, plsmaswd, coh3002c, ROT0, "Capcom", "Choukou Senki Kikaioh (Japan 980914)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, sfex2p, coh3002c, coh3002c, capcom6b, coh3002c, ROT0, "Capcom / Arika", "Street Fighter EX2 Plus (Europe 990611)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, sfex2pu, sfex2p, coh3002c, capcom6b, coh3002c, ROT0, "Capcom / Arika", "Street Fighter EX2 Plus (USA 990611)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, sfex2pa, sfex2p, coh3002c, capcom6b, coh3002c, ROT0, "Capcom / Arika", "Street Fighter EX2 Plus (Asia 990611)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, sfex2ph, sfex2p, coh3002c, capcom6b, coh3002c, ROT0, "Capcom / Arika", "Street Fighter EX2 Plus (Hispanic 990611)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, sfex2pj, sfex2p, coh3002c, capcom6b, coh3002c, ROT0, "Capcom / Arika", "Street Fighter EX2 Plus (Japan 990611)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, strider2, coh3002c, coh3002c, strider2, coh3002c, ROT0, "Capcom", "Strider 2 (Europe 991213)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, strider2u, strider2, coh3002c, strider2, coh3002c, ROT0, "Capcom", "Strider 2 (USA 991213)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, strider2a, strider2, coh3002c, strider2, coh3002c, ROT0, "Capcom", "Strider 2 (Asia 991213)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, shiryu2, strider2, coh3002c, strider2, coh3002c, ROT0, "Capcom", "Strider Hiryu 2 (Japan 991213)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

/* Atari */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the bios file, so that we do not have to include */
/* it in every zip file */
BIOS( 1995, coh1000w, 0, coh1000w, zn, coh1000w, ROT0, "Atari", "Time Warner ZN-1", 0 )

GAME( 1997, primrag2, coh1000w, coh1000w, primrag2, coh1000w, ROT0, "Atari", "Primal Rage II (Test Version 0.36a)", 0 )

/* Acclaim */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the bios file, so that we do not have to include */
/* it in every zip file */
BIOS( 1995, coh1000a, 0, coh1000a, zn, coh1000a, ROT0, "Acclaim", "Acclaim ZN-1", 0 )

GAME( 1996, nbajamex, coh1000a, coh1000a, nbajamex, coh1000a, ROT0, "Acclaim", "NBA Jam Extreme (ver. 1.10I)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, nbajamexa, nbajamex, coh1000a, nbajamex, coh1000a, ROT0, "Acclaim", "NBA Jam Extreme (ver. 1.04)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, jdredd, coh1000a, coh1000a, jdredd, coh1000a, ROT0, "Acclaim", "Judge Dredd (Rev C Dec. 17 1997)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )
GAME( 1996, jdreddb, jdredd, coh1000a, jdredd, coh1000a, ROT0, "Acclaim", "Judge Dredd (Rev B Nov. 26 1997)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING )

/* Tecmo */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the bios file, so that we do not have to include */
/* it in every zip file */
BIOS( 1997, coh1002m, 0, coh1002m, zn, coh1002m, ROT0, "Tecmo", "Tecmo TPS System", 0 )

GAME( 1997, glpracr2, coh1002m, coh1002m, glpracr2, coh1002m, ROT0, "Tecmo", "Gallop Racer 2 (Export)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, glpracr2j, glpracr2, coh1002m, glpracr2, coh1002m, ROT0, "Tecmo", "Gallop Racer 2 (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, glpracr2l, glpracr2, coh1002m, glpracr2, coh1002m, ROT0, "Tecmo", "Gallop Racer 2 Link HW (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, doapp, coh1002m, coh1002m, zn2p, coh1002m, ROT0, "Tecmo", "Dead Or Alive ++ (Japan/USA/Export)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, doappk, doapp, coh1002m, zn2p, coh1002m, ROT0, "Tecmo", "Dead Or Alive ++ (Korea)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, cbaj, coh1002m, coh1002msnd, zn1p, cbaj, ROT0, "UEP Systems", "Cool Boarders Arcade Jam (Export)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, cbajbl, cbaj, coh1002msnd, zn1p, cbaj, ROT0, "bootleg", "Cool Boarders Arcade Jam (Export bootleg)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, shngmtkb, coh1002m, coh1002m, zn2p, coh1002m, ROT0, "Sunsoft / Activision", "Shanghai Matekibuyuu (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, tondemo, coh1002m, coh1002m, zn1p, coh1002m, ROT0, "Tecmo", "Tondemo Crisis (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, glpracr3, coh1002m, coh1002m, glpracr2, coh1002m, ROT0, "Tecmo", "Gallop Racer 3 (Export)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, glpracr3j, glpracr3, coh1002m, glpracr2, coh1002m, ROT0, "Tecmo", "Gallop Racer 3 (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, flamegun, coh1002m, coh1002m, zn2p, coh1002m, ROT0, "Gaps Inc.", "Flame Gunner (Export)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, flamegunj, flamegun, coh1002m, zn2p, coh1002m, ROT0, "Gaps Inc.", "Flame Gunner (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1999, lpadv, coh1002m, coh1002m, zn2p, coh1002m, ROT0, "Amuse World", "Logic Pro Adventure (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 2000, tblkkuzu, coh1002m, coh1002m, zn2p, coh1002m, ROT0, "Tamsoft / D3 Publisher", "The Block Kuzushi (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 2000, 1on1gov, coh1002m, coh1002m, zn, coh1002m, ROT0, "Tecmo", "1 on 1 Government (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 2000, twcupmil, coh1002m, coh1002m, zn2p, coh1002m, ROT0, "Tecmo", "Tecmo World Cup Millennium (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 2001, mfjump, coh1002m, coh1002m, zn2p, coh1002m, ROT0, "Tecmo", "Monster Farm Jump (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 2000, brvblade, coh1002m, coh1002e, znt2p, coh1002e, ROT270, "Eighting / Raizing", "Brave Blade (World)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 2000, brvbladeu, brvblade, coh1002e, znt2p, coh1002e, ROT270, "Eighting / Raizing", "Brave Blade (USA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 2000, brvbladea, brvblade, coh1002e, znt2p, coh1002e, ROT270, "Eighting / Raizing", "Brave Blade (Asia)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 2000, brvbladej, brvblade, coh1002e, znt2p, coh1002e, ROT270, "Eighting / Raizing", "Brave Blade (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

/* Video System */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the bios file, so that we do not have to include */
/* it in every zip file */
BIOS( 1996, coh1002v, 0, coh1002v, zn, coh1002v, ROT0, "Video System Co.", "Video System ZN-1", 0 )

GAME( 1996, aerofgts, coh1002v, coh1002v, aerofgts, coh1002v, ROT270, "McO'River, Inc.", "Aero Fighters Special (USA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, aerofgtst, aerofgts, coh1002v, aerofgts, coh1002v, ROT270, "Video System Co.", "Aero Fighters Special (Taiwan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, sncwgltd, aerofgts, coh1002v, aerofgts, coh1002v, ROT270, "Video System Co.", "Sonic Wings Limited (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

/* Taito */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the bios file, so that we do not have to include */
/* it in every zip file */
BIOS( 1995, coh1000t, 0, coh1000ta, znt2p, coh1000ta, ROT0, "Taito", "Taito FX-1", 0 )

GAME( 1995, sfchamp, coh1000t, coh1000ta, znt2p, coh1000ta, ROT0, "Taito", "Super Football Champ (Ver 2.5O)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1995, sfchampo, sfchamp, coh1000ta, znt2p, coh1000ta, ROT0, "Taito", "Super Football Champ (Ver 2.4O)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1995, sfchampu, sfchamp, coh1000ta, znt2p, coh1000ta, ROT0, "Taito", "Super Football Champ (Ver 2.4A)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1995, sfchampj, sfchamp, coh1000ta, znt2p, coh1000ta, ROT0, "Taito", "Super Football Champ (Ver 2.4J)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1995, psyforce, coh1000t, coh1000ta, znt2p, coh1000ta, ROT0, "Taito", "Psychic Force (Ver 2.4O)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING ) // emulator hang
GAME( 1995, psyforcej, psyforce, coh1000ta, znt2p, coh1000ta, ROT0, "Taito", "Psychic Force (Ver 2.4J)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING ) // emulator hang
GAME( 1995, psyforcex, psyforce, coh1000ta, znt2p, coh1000ta, ROT0, "Taito", "Psychic Force EX (Ver 2.0J)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND | GAME_NOT_WORKING ) // emulator hang
GAME( 1996, mgcldate, mgcldtex, coh1000ta, znt2p, coh1000ta, ROT0, "Taito", "Magical Date / Magical Date - Dokidoki Kokuhaku Daisakusen (Ver 2.02J)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, mgcldtex, coh1000t, coh1000ta, znt2p, coh1000ta, ROT0, "Taito", "Magical Date EX / Magical Date - Sotsugyou Kokuhaku Daisakusen (Ver 2.01J)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, raystorm, coh1000t, coh1000tb, raystorm, coh1000tb, ROT0, "Taito", "Ray Storm (Ver 2.06A)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, raystormo, raystorm, coh1000tb, raystorm, coh1000tb, ROT0, "Taito", "Ray Storm (Ver 2.05O)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, raystormu, raystorm, coh1000tb, raystorm, coh1000tb, ROT0, "Taito", "Ray Storm (Ver 2.05A)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, raystormj, raystorm, coh1000tb, raystorm, coh1000tb, ROT0, "Taito", "Ray Storm (Ver 2.05J)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, ftimpact, ftimpcta, coh1000tb, znt2p, coh1000tb, ROT0, "Taito", "Fighters' Impact (Ver 2.02O)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, ftimpactu, ftimpcta, coh1000tb, znt2p, coh1000tb, ROT0, "Taito", "Fighters' Impact (Ver 2.02A)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, ftimpactj, ftimpcta, coh1000tb, znt2p, coh1000tb, ROT0, "Taito", "Fighters' Impact (Ver 2.02J)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, ftimpcta, coh1000t, coh1000tb, znt2p, coh1000tb, ROT0, "Taito", "Fighters' Impact A (Ver 2.00J)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, gdarius, gdarius2, coh1000tb, znt2p, coh1000tb, ROT0, "Taito", "G-Darius (Ver 2.02O)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, gdariusu, gdarius2, coh1000tb, znt2p, coh1000tb, ROT0, "Taito", "G-Darius (Ver 2.02A)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, gdariusj, gdarius2, coh1000tb, znt2p, coh1000tb, ROT0, "Taito", "G-Darius (Ver 2.01J)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, gdarius2, coh1000t, coh1000tb, znt2p, coh1000tb, ROT0, "Taito", "G-Darius Ver.2 (Ver 2.03J)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

/* Eighting/Raizing */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the bios file, so that we do not have to include */
/* it in every zip file */
BIOS( 1996, coh1002e, 0, coh1002e, znt2p, coh1002e, ROT0, "Eighting / Raizing", "Eighting / Raizing ZN-1", 0 )

GAME( 1997, beastrzr, coh1002e, coh1002e, znt2p, coh1002e, ROT0, "Eighting / Raizing", "Beastorizer (USA, Sep 22 1997)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, beastrzra, beastrzr, coh1002e, znt2p, coh1002e, ROT0, "Eighting / Raizing", "Beastorizer (USA, Jun 21 1997)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, bldyroar, beastrzr, coh1002e, znt2p, coh1002e, ROT0, "Eighting / Raizing", "Bloody Roar (Japan, Jun 21 1997)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, beastrzrb, beastrzr, coh1002e, znt2p, coh1002e, ROT0, "bootleg", "Beastorizer (USA bootleg)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, bldyror2, coh1002e, coh1002e, bldyror2, coh1002e, ROT0, "Eighting / Raizing", "Bloody Roar 2 (World)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, bldyror2u, bldyror2, coh1002e, bldyror2, coh1002e, ROT0, "Eighting / Raizing", "Bloody Roar 2 (USA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, bldyror2a, bldyror2, coh1002e, bldyror2, coh1002e, ROT0, "Eighting / Raizing", "Bloody Roar 2 (Asia)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1998, bldyror2j, bldyror2, coh1002e, bldyror2, coh1002e, ROT0, "Eighting / Raizing", "Bloody Roar 2 (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

/* Atlus */

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the bios file, so that we do not have to include */
/* it in every zip file */
BIOS( 1995, coh1001l, 0, coh1001l, zn, coh1001l, ROT0, "Atlus", "Atlus ZN-1", 0 )

GAME( 1996, hvnsgate, coh1001l, coh1001l, zn2p, coh1001l, ROT0, "Racdym / Atlus", "Heaven's Gate", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
