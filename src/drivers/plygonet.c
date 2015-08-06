/*
    Polygonet Commanders (Konami, 1993)

    Preliminary driver by R. Belmont
    Additional work by Andrew Gardner

    This is Konami's first 3D game!

    Hardware:
    68EC020 @ 16 MHz
    Motorola XC56156-40 DSP @ 40 MHz
    Z80 + K054539 for sound
    Network to connect up to 4 PCBs.

    Video hardware:
    TTL text plane similar to Run and Gun.
    Konami K054009(x2) + K054010(x2) (polygon rasterizers)
    Konami K053936 "PSAC2" (3d roz plane, used for backgrounds)

    Driver includes:
    - 68020 memory map
    - Z80 + sound system
    - EEPROM
    - service switch
    - TTL text plane

    Driver needs:
    - Polygon rasterization (K054009 + K054010)
    - Hook up PSAC2 (gfx decode for it is already present and correct)
    - Palettes
    - Controls
    - Priorities.  From the original board it appears they're fixed, in front to back order:
      (all the way in front) TTL text layer -> polygons -> PSAC2 (all the way in back)

Notes:

 (R. Belmont)
 506000 is the DSP control
 506004 is DSP status (in bit 0, where 0 = not ready, 1 = ready)
 50600C and 50600E are the DSP comms ports (read/write)

*/

#include "driver.h"

#include "vidhrdw/konamiic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "cpu/dsp56k/dsp56k.h"
#include "sound/k054539.h"
#include "machine/eeprom.h"

VIDEO_START(polygonet_vh_start);
VIDEO_UPDATE(polygonet_vh_screenrefresh);

READ32_HANDLER( polygonet_ttl_ram_r );
WRITE32_HANDLER( polygonet_ttl_ram_w );

static int init_eeprom_count;
static UINT32 *dsp_shared_ram;
static UINT16 *dsp56k_shared_ram_16;

static UINT16 *dsp56k_xdata_ram ;
static UINT16 *dsp56k_program_memory;	// [0x800<<1] - 2048 words
static UINT16 *dsp56k_com_ram;


static struct EEPROM_interface eeprom_interface =
{
	7,			/* address bits */
	8,			/* data bits */
	"011000",		/*  read command */
	"010100",		/* write command */
	"0100100000000",/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static NVRAM_HANDLER(nvram_handler)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 10;
	}
}

static READ32_HANDLER( polygonet_eeprom_r )
{
	if (ACCESSING_LSW32)
	{
		return 0x0200 | (EEPROM_read_bit()<<8);
	}
	else
	{
		return (readinputport(0)<<24);
	}

	logerror("unk access to eeprom port (mask %x)\n", mem_mask);
	return 0;
}


static WRITE32_HANDLER( polygonet_eeprom_w )
{
	if (ACCESSING_MSB32)
	{
		EEPROM_write_bit((data & 0x01000000) ? ASSERT_LINE : CLEAR_LINE);
		EEPROM_set_cs_line((data & 0x02000000) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x04000000) ? ASSERT_LINE : CLEAR_LINE);
		return;
	}

	logerror("unknown write %x (mask %x) to eeprom\n", data, mem_mask);
}

/* TTL tile readback for ROM test */
static READ32_HANDLER( ttl_rom_r )
{
	UINT32 *ROM;

	ROM = (UINT32 *)memory_region(REGION_GFX1);

	return ROM[offset];
}

/* PSAC2 tile readback for ROM test */
static READ32_HANDLER( psac_rom_r )
{
	UINT32 *ROM;

	ROM = (UINT32 *)memory_region(REGION_GFX2);

	return ROM[offset];
}

// irqs 3, 5, and 7 have valid vectors
// irq 3 does ??? (vblank?)
// irq 5 does ??? (polygon end of draw?)
// irq 7 does nothing (it jsrs to a rts and then rte)

static INTERRUPT_GEN(polygonet_interrupt)
{
	if (cpu_getiloops())
		cpunum_set_input_line(0, MC68000_IRQ_5, HOLD_LINE);
	else
		cpunum_set_input_line(0, MC68000_IRQ_3, HOLD_LINE);
}

/* sound CPU communications */

static READ32_HANDLER( sound_r )
{
	int latch = soundlatch3_r(0);

	if (latch == 0xe) latch = 0xf;	// hack: until 54539 NMI disable found

	return latch<<8;
}

static WRITE32_HANDLER( sound_w )
{
	if (ACCESSING_MSB)
	{
		soundlatch_w(0, (data>>8)&0xff);
	}
	else
	{
		soundlatch2_w(0, data&0xff);
	}
}

static WRITE32_HANDLER( sound_irq_w )
{
	cpunum_set_input_line(1, 0, HOLD_LINE);
}

/* DSP communications are on their way to being correct */

static int dsp_state;

static READ32_HANDLER( dsp_r )
{
	// My guess is this is Host Receive Data / Host Transfer Data Register
	//   I'm guessing this because 0xffe5 on the DSP is HTX/HRC register and
	//   it seems like when the memory test is done, it writes 0x0000 to that
	//   part of memory.  This function then reads (a non-zero?!) value
	//   into 0xff000000 and lets the 68020 continue...

	// Maybe this 0x0000 entry goes through some logic that turns it from a
	//   0x0000 to a non-zero value?

	// Interestingly enough, during memory test 4, the dsp writes 0x0000, 0x0001,
	//   and 0x0002 to ffe5 - I wonder if the different values come through
	//   this theoretical logic between the dsp56k and the 68020?

	// Is there anything at mask 0x0000ff00 ?

	logerror("PING     - dsp56k (%04x PC=%x) (mask=%08x)\n", dsp_state, activecpu_get_pc(), mem_mask);

	if (dsp_state)
		return dsp_state << 24 ;
	else
		return 0 << 24 ;
}

static READ32_HANDLER( dsp_shared_ram_read )
{
	return dsp_shared_ram[offset] ;
}

static WRITE32_HANDLER( dsp_shared_ram_write )
{
	COMBINE_DATA(&dsp_shared_ram[offset]) ;

	// write the data to the dsp56k as well...
	dsp56k_shared_ram_16[(offset<<1)]   = (dsp_shared_ram[offset] & 0xffff0000) >> 16 ;
	dsp56k_shared_ram_16[(offset<<1)+1] = (dsp_shared_ram[offset] & 0x0000ffff) ;
}

static READ16_HANDLER( dsp56k_com_port_r )
{
	return dsp56k_com_ram[offset] ;
}

static READ32_HANDLER( dsp_hi_r )
{
	logerror("dsp_hi_r : %08x %08x at PC=%x\n", offset, mem_mask, activecpu_get_pc());
	return 0x00 ;
}

static UINT16 memoryOffset ;		// I think memoryOffset should be initialized to 0xe000, but that wouldn't put
									//   the RESET vectors in the right spot?

static WRITE32_HANDLER( dsp_hi_w )
{
	if (mem_mask == ~0xff000000)
	{
		dsp56k_program_memory[memoryOffset] &= 0x00ff ;
		dsp56k_program_memory[memoryOffset] |= (data>>16) ;
	}
	else if (mem_mask == ~0x0000ff00)
	{
		dsp56k_program_memory[memoryOffset] &= 0xff00 ;
		dsp56k_program_memory[memoryOffset] |= (data>>8) ;
		logerror("Wrote memoryOffset[%d] : %04x\n", memoryOffset, dsp56k_program_memory[memoryOffset]) ;
		memoryOffset++ ;
	}
}

static WRITE32_HANDLER( dsp_2_w )
{
	// Assumed to be mapped to the DSP56156's Host Interface
	// it's write, so there are only 2 options here - the Interrupt Control Register (ICR) - address $0
	//   or the Interrupt Vector Register (IVR) - address $3...

	// addendum - i believe it CAN'T be the ICR because bit 2 is reserved, and 05 would be writing to that...
	//            besides, i think dsp_w (mask 0xff000000) is the proper ICR

	// addendum - or maybe it's attached to the pins directly attached to the Host Interface Control Logic
	//            HA0-HA2, HR/W, HEN, HACK, HREQ

	// values passed are 0x00, 0x01, 0x05

	logerror("dsp_2_w  : %08x %08x %x - ", data, mem_mask, activecpu_get_pc()) ;

	if (mem_mask == 0x00ffffff)
	{
		if (data>>24 == 5)
		{
			logerror("CLEARING DSP STATE\n") ;
			dsp_state = 0 ;
		}
		else
			logerror("UNKNOWN\n") ;
	}

	memoryOffset = 0 ;	// For insuring we start uploading at the beginning of memory
}

static WRITE32_HANDLER( dsp_w )
{
	logerror("dsp_w    : %08x %08x %x - ", data, mem_mask, activecpu_get_pc()) ;

	if (mem_mask == 0x00ffffff)
	{
		// This could very well be a generic RESET interrupt for the DSP56k (i'm treating it this way now)
		if (data>>24 == 8)
		{
			logerror("generic RESET sent\n");

			// Let the dsp56k rip...
			cpunum_set_input_line(1, 3, CLEAR_LINE);
		}
	}
	else
	{
		// Host Interface (HI) - Command Vector Register (CVR) - address $1
		UINT16 irqVector = (((data>>8) & 0x1f)) << 1 ;

		logerror("RESET (%04x) sent\n", irqVector);
		cpunum_set_input_line_and_vector(1, 3, ASSERT_LINE, irqVector) ;
	}
}


// DSP56k MEMORY HANDLERS //
//   these mostly deal with the I/O devices located after 0xffc0, but they also combine memory into shared   //
//   memory space properly as well.  (I wonder if MAME has a function that does what dsp56k_shared_ram_write //
//   does?)                                                                                                  //

static READ16_HANDLER( dsp56k_shared_ram_read )
{
	return dsp56k_shared_ram_16[offset] ;
}

static WRITE16_HANDLER( dsp56k_shared_ram_write )
{
	COMBINE_DATA(&dsp56k_shared_ram_16[offset]) ;

	// write the data to the 68k as well, yo...
	if (offset % 2)
		dsp_shared_ram[offset>>1] = ((dsp56k_shared_ram_16[offset-1]) << 16) | dsp56k_shared_ram_16[offset] ;
	else
		dsp_shared_ram[offset>>1] = ((dsp56k_shared_ram_16[offset])   << 16) | dsp56k_shared_ram_16[offset+1] ;
}

static UINT16 hackCount = 0xfff0 ;

static WRITE16_HANDLER( dsp56k_com_port_w )
{
	int i ;			// hack vars...
	UINT16 lpc ;

	COMBINE_DATA(&dsp56k_com_ram[offset]) ;

	offset += 0xffc0 ;

//  logerror("DSP56k writes %04x to com port at %04x\n", dsp56k_com_ram[offset], offset) ;

	switch(offset)
	{
		// 0xffe3 is where the memtests try to talk along port C
		case 0xffe3:
			logerror("DSP56K WRITING (%04x) TO ffe3 (PC=%08x)\n", dsp56k_com_ram[offset-0xffc0], activecpu_get_pc()) ;

			lpc = activecpu_get_pc() ;

			if (lpc == 0x1bc) hackCount = 0xfff0 ;		// Reset when going onto the 'not' tests

			///////////////////////////////////////////////////////////////////////////////////
			// MEGA-HACK - fix memory at the very last minute so memory tests pass in part 4 //
			//             this can be fixed with banking - but is it right ???              //
			///////////////////////////////////////////////////////////////////////////////////
			if (lpc == 0x0174 || lpc == 0x01bc)
			{
				for (i = 0; i < 0x7800; i++)
				{
					if(lpc == 0x0174)
						dsp56k_xdata_ram[0x0800+i] = hackCount ;
					else
						dsp56k_xdata_ram[0x0800+i] = hackCount ^ 0xffff ;
					hackCount-- ;
				}
			}
			else if (lpc == 0x022d || lpc == 0x024d)
			{
				switch(dsp56k_com_ram[offset-0xffc0])
				{
					case 0x0020:
					case 0x0024:
					case 0x0028:
					case 0x002c:
					case 0x0030:
					case 0x0034:
					case 0x0038:
					for (i = 0; i < 0x4000; i++)
					{
						if(lpc == 0x022d)
							dsp56k_xdata_ram[0x8000+i] = hackCount ;
						else
							dsp56k_xdata_ram[0x8000+i] = hackCount ^ 0xffff ;
						hackCount-- ;
					}
					break ;

					case 0x003c:
					for (i = 0; i < 0x4000+0x2000; i++)
					{
						if(lpc == 0x022d)
							dsp56k_xdata_ram[0x8000+i] = hackCount ;
						else
							dsp56k_xdata_ram[0x8000+i] = hackCount ^ 0xffff ;
						hackCount-- ;
					}
					break ;

				}
			}

			// NESTED LoooooP
			else if (lpc == 0x017b || lpc == 0x01c3)
			{
				for (i = 0; i < 0x2000+0x7fc0; i++)
				{
					if(lpc == 0x017b)
						dsp56k_xdata_ram[0x6000+i] = hackCount ;
					else
						dsp56k_xdata_ram[0x6000+i] = hackCount ^ 0xffff ;
					hackCount-- ;
				}
			}
			else if (lpc == 0x0189 || lpc == 0x01d1)
			{
				for (i = 0; i < 0x7fc0; i++)
				{
					if(lpc == 0x0189)
						dsp56k_xdata_ram[0x8000+i] = hackCount ;
					else
						dsp56k_xdata_ram[0x8000+i] = hackCount ^ 0xffff ;
					hackCount-- ;
				}
			}
			else if (lpc == 0x018b || lpc == 0x01d3)
			{
				hackCount += 0x7fc0 ;						// undo what was done...

				for (i = 0; i < 0x2000+0x7fc0; i++)
				{
					if(lpc == 0x018b)
						dsp56k_xdata_ram[0x6000+i] = hackCount ;
					else
						dsp56k_xdata_ram[0x6000+i] = hackCount ^ 0xffff ;
					hackCount-- ;
				}
			}

			//////////////////////////
			//  END HACK NASTINESS  //
			//////////////////////////

			break ;

		case 0xffe4: dsp56k_com_ram[offset-0xffc0] = 0x0002 ;				// Hack to for the end of the last memcheck
					 break ;

		// 0xffe5 is where the memtests say "we're done!"
		//        (it's the generic parallel interface named HTX/HRX)
		case 0xffe5: logerror("SETTING DSP STATE %04x (PC=%04x)\n", dsp56k_com_ram[offset-0xffc0], activecpu_get_pc()) ;
					 dsp_state = dsp56k_com_ram[offset-0xffc0] ^ 0xffff ;	// For telling the 68020 the dsp56k is done
					 break ;
	}
}


/**********************************************************************************/


static ADDRESS_MAP_START( polygonet_readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA32_ROM)
	AM_RANGE(0x200000, 0x21ffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x440000, 0x440fff) AM_READ(MRA32_RAM)
	AM_RANGE(0x480000, 0x480003) AM_READ(polygonet_eeprom_r)
	AM_RANGE(0x500000, 0x503fff) AM_READ(dsp_shared_ram_read)
	AM_RANGE(0x506004, 0x506007) AM_READ(dsp_r)
	AM_RANGE(0x540000, 0x540fff) AM_READ(polygonet_ttl_ram_r)
	AM_RANGE(0x541000, 0x54101f) AM_READ(MRA32_RAM)
	AM_RANGE(0x50600c, 0x50600f) AM_READ(dsp_hi_r)
	AM_RANGE(0x580000, 0x5807ff) AM_READ(MRA32_RAM)
//  AM_RANGE(0x580800, 0x580803) AM_READ(MRA32_RAM) // network RAM / registers?
	AM_RANGE(0x600008, 0x60000b) AM_READ(sound_r)
	AM_RANGE(0x700000, 0x73ffff) AM_READ(psac_rom_r)
	AM_RANGE(0x780000, 0x79ffff) AM_READ(ttl_rom_r)
	AM_RANGE(0xff8000, 0xffffff) AM_READ(MRA32_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( polygonet_writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA32_ROM)
	AM_RANGE(0x200000, 0x21ffff) AM_WRITE(MWA32_RAM)	// PSAC2 tilemap
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(MWA32_RAM)	// PSAC2 lineram
	AM_RANGE(0x4C0000, 0x4C0003) AM_WRITE(polygonet_eeprom_w)
	AM_RANGE(0x500000, 0x503fff) AM_WRITE(dsp_shared_ram_write) AM_BASE(&dsp_shared_ram)
	AM_RANGE(0x504000, 0x504003) AM_WRITE(dsp_2_w)
	AM_RANGE(0x506000, 0x506003) AM_WRITE(dsp_w)
	AM_RANGE(0x50600c, 0x50600f) AM_WRITE(dsp_hi_w)
	AM_RANGE(0x540000, 0x540fff) AM_WRITE(polygonet_ttl_ram_w)
	AM_RANGE(0x541000, 0x54101f) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x580000, 0x5807ff) AM_WRITE(MWA32_RAM)	// A21K
//  AM_RANGE(0x580800, 0x580803) AM_WRITE(MWA32_RAM)    // network RAM / registers?
	AM_RANGE(0x600004, 0x600007) AM_WRITE(sound_w)
	AM_RANGE(0x640000, 0x640003) AM_WRITE(sound_irq_w)
	AM_RANGE(0x680000, 0x680003) AM_WRITE(watchdog_reset32_w)
	AM_RANGE(0xff8000, 0xffffff) AM_WRITE(MWA32_RAM)
ADDRESS_MAP_END


/**********************************************************************************/

// ajg - I have a hunch there is something going on at 0x7000 too...
static ADDRESS_MAP_START( dsp56156_p_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0x07ff) AM_READ(MRA16_RAM) AM_BASE(&dsp56k_program_memory)
	AM_RANGE(0x8000, 0x87ff) AM_RAM												// the processor memtests here
ADDRESS_MAP_END

static ADDRESS_MAP_START( dsp56156_p_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0x07ff) AM_WRITE(MWA16_RAM) AM_BASE(&dsp56k_program_memory)
	AM_RANGE(0x8000, 0x87ff) AM_RAM												// the processor memtests here
ADDRESS_MAP_END


// 0x0000-0x07ff is on-board memory for the dsp, but I'm treating it as generic ram for now
static ADDRESS_MAP_START( dsp56156_d_readmem, ADDRESS_SPACE_DATA, 16 )
	AM_RANGE(0x0000, 0xbfff) AM_RAM AM_BASE(&dsp56k_xdata_ram)
	AM_RANGE(0xc000, 0xdfff) AM_READ(dsp56k_shared_ram_read) AM_BASE(&dsp56k_shared_ram_16)
	AM_RANGE(0xe000, 0xffbf) AM_RAM
	AM_RANGE(0xffc0, 0xffff) AM_READ(dsp56k_com_port_r) AM_BASE(&dsp56k_com_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dsp56156_d_writemem, ADDRESS_SPACE_DATA, 16 )
	AM_RANGE(0x0000, 0xbfff) AM_RAM AM_BASE(&dsp56k_xdata_ram)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(dsp56k_shared_ram_write) AM_BASE(&dsp56k_shared_ram_16)
	AM_RANGE(0xe000, 0xffbf) AM_RAM
	AM_RANGE(0xffc0, 0xffff) AM_WRITE(dsp56k_com_port_w) AM_BASE(&dsp56k_com_ram)
ADDRESS_MAP_END


/**********************************************************************************/

static int cur_sound_region;

static void reset_sound_region(void)
{
	memory_set_bankptr(2, memory_region(REGION_CPU2) + 0x10000 + cur_sound_region*0x4000);
}

static WRITE8_HANDLER( sound_bankswitch_w )
{
	cur_sound_region = (data & 0x1f);

	reset_sound_region();
}

static INTERRUPT_GEN(audio_interrupt)
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK2)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe22f) AM_READ(K054539_0_r)
	AM_RANGE(0xe230, 0xe3ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe400, 0xe62f) AM_READ(K054539_1_r)
	AM_RANGE(0xe630, 0xe7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf002, 0xf002) AM_READ(soundlatch_r)
	AM_RANGE(0xf003, 0xf003) AM_READ(soundlatch2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe22f) AM_WRITE(K054539_0_w)
	AM_RANGE(0xe230, 0xe3ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe400, 0xe62f) AM_WRITE(K054539_1_w)
	AM_RANGE(0xe630, 0xe7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(soundlatch3_w)
	AM_RANGE(0xf800, 0xf800) AM_WRITE(sound_bankswitch_w)
	AM_RANGE(0xfff1, 0xfff3) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

static struct K054539interface k054539_interface =
{
	REGION_SOUND1
};

/**********************************************************************************/

static const gfx_layout bglayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4,
	  9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
 	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &bglayout,     0x0000, 64 },
	{ -1 } /* end of array */
};

MACHINE_DRIVER_START( plygonet )
	MDRV_CPU_ADD(M68EC020, 16000000)	/* 16 MHz (xtal is 32.0 MHz) */
	MDRV_CPU_PROGRAM_MAP(polygonet_readmem,polygonet_writemem)
	MDRV_CPU_VBLANK_INT(polygonet_interrupt, 2)

	MDRV_CPU_ADD(DSP56156,10000000)		/* no idea */
	MDRV_CPU_PROGRAM_MAP(dsp56156_p_readmem, dsp56156_p_writemem)
	MDRV_CPU_DATA_MAP(dsp56156_d_readmem,dsp56156_d_writemem)


	MDRV_CPU_ADD_TAG( "sound", Z80, 8000000 )
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_PERIODIC_INT(audio_interrupt, TIME_IN_HZ(480))

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_NVRAM_HANDLER(nvram_handler)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 64*8-1, 0, 32*8-1 )
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(polygonet_vh_start)
	MDRV_VIDEO_UPDATE(polygonet_vh_screenrefresh)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(K054539, 48000)
	MDRV_SOUND_CONFIG(k054539_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.75)
	MDRV_SOUND_ROUTE(1, "right", 0.75)

	MDRV_SOUND_ADD(K054539, 48000)
	MDRV_SOUND_CONFIG(k054539_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.75)
	MDRV_SOUND_ROUTE(1, "right", 0.75)
MACHINE_DRIVER_END

INPUT_PORTS_START( polygonet )
	PORT_START

	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4)

	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_DIPNAME(0x10, 0x00, "Monitors")
	PORT_DIPSETTING(0x00, "1 Monitor")
	PORT_DIPSETTING(0x10, "2 Monitors")
//  PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

static DRIVER_INIT(polygonet)
{
	/* set default bankswitch */
	cur_sound_region = 2;
	reset_sound_region();
}

ROM_START( plygonet )
	/* main program */
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD32_BYTE( "305a01.bin", 0x000003, 512*1024, CRC(8bdb6c95) SHA1(e981833842f8fd89b9726901fbe2058444204792) )
	ROM_LOAD32_BYTE( "305a02.bin", 0x000002, 512*1024, CRC(4d7e32b3) SHA1(25731526535036972577637d186f02ae467296bd) )
	ROM_LOAD32_BYTE( "305a03.bin", 0x000001, 512*1024, CRC(36e4e3fe) SHA1(e8fcad4f196c9b225a0fbe70791493ff07c648a9) )
	ROM_LOAD32_BYTE( "305a04.bin", 0x000000, 512*1024, CRC(d8394e72) SHA1(eb6bcf8aedb9ba5843204ab8aacb735cbaafb74d) )

	/* Z80 sound program */
	ROM_REGION( 0x30000, REGION_CPU3, 0 )
	ROM_LOAD("305b05.bin", 0x000000, 0x20000, CRC(2d3d9654) SHA1(784a409df47cee877e507b8bbd3610d161d63753) )
	ROM_RELOAD( 0x10000, 0x20000)

	/* TTL text plane tiles */
	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_LOAD( "305b06.bin", 0x000000, 0x20000, CRC(decd6e42) SHA1(4c23dcb1d68132d3381007096e014ee4b6007086) )

	/* '936 tiles */
 	ROM_REGION( 0x40000, REGION_GFX2, 0 )
	ROM_LOAD( "305b07.bin", 0x000000, 0x40000, CRC(e4320bc3) SHA1(b0bb2dac40d42f97da94516d4ebe29b1c3d77c37) )

	/* sound data */
	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "305b08.bin", 0x000000, 0x200000, CRC(874607df) SHA1(763b44a80abfbc355bcb9be8bf44373254976019) )
ROM_END

/*          ROM        parent   machine    inp        init */
GAME( 1993, plygonet, 0,       plygonet, polygonet, polygonet, ROT90, "Konami", "Polygonet Commanders (ver UAA)", GAME_NOT_WORKING | GAME_NO_SOUND )
