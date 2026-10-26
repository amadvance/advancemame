// license:GPL-2.0+
// copyright-holders:Dirk Best
/***************************************************************************

    Spectravideo SVI-318 / SVI-328 machine emulation

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "machine/wd17xx.h"
#include "includes/svi318.h"
#include "formats/svi_cas.h"
#include "machine/8255ppi.h"
#include "vidhrdw/tms9928a.h"
#include "vidhrdw/m6845.h"
#include "devices/basicdsk.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "sound/dac.h"
#include "sound/ay8910.h"
#include "image.h"

SVI_318 svi;

static void svi318_set_banks(void);
static unsigned long svi318_calcoffset(UINT8 t, UINT8 h, UINT8 s,
	UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT8 first_sector_id, UINT16 offset_track_zero);

/* Cartridge */

DEVICE_LOAD( svi318_cart )
{
	UINT8 *p;
	int size;

	p = image_malloc(image, 0x8000);
	if (!p)
		return INIT_FAIL;

	memset(p, 0xff, 0x8000);
	size = mame_fsize(file);
	if (mame_fread(file, p, size) != size)
		return INIT_FAIL;

	svi.cart_rom = p;
	svi318_set_banks();

	return INIT_PASS;
}

DEVICE_UNLOAD( svi318_cart )
{
	svi.cart_rom = NULL;
	svi318_set_banks();
}

/* PPI */

/*
 PPI Port A Input (address 98H)
 Bit Name     Description
  0  TA       Joystick 1, /SENSE
  1  TB       Joystick 1, EOC
  2  TC       Joystick 2, /SENSE
  3  TD       Joystick 2, EOC
  4  TRIGGER1 Joystick 1, Trigger
  5  TRIGGER2 Joystick 2, Trigger
  6  /READY   Cassette, Ready
  7  CASR     Cassette, Read data
*/

static READ8_HANDLER( svi318_ppi_port_a_r )
{
	UINT8 data = 0x3f;

	/* bit 4-5, joystick buttons */
	data &= readinputport(12);

	/* bit 6-7, cassette */
	data |= svi318_cassette_present(0) ? 0x00 : 0x40;
	data |= (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.0038) ? 0x80 : 0x00;

	return data;
}

/*
 PPI Port B Input (address 99H)
 Bit Name Description
  0-7 Column status of selected keyboard line
*/

static READ8_HANDLER( svi318_ppi_port_b_r )
{
	int row = svi.keyboard_row;
	if (row <= 10)
	{
		int data = readinputport(row);
		if (row == 6)
			data &= readinputport(14);
		return data;
	}
	return 0xff;
}

/*
 PPI Port C Output (address 97H)
 Bit Name   Description
  0  KB0    Keyboard, Line select 0
  1  KB1    Keyboard, Line select 1
  2  KB2    Keyboard, Line select 2
  3  KB3    Keyboard, Line select 3
  4  CASON  Cassette, Motor relay control (0=on, 1=off)
  5  CASW   Cassette, Write data
  6  CASAUD Cassette, Audio out (speaker mute/enable)
  7  SOUND  Keyboard, Click sound bit (pulse)
*/

static WRITE8_HANDLER( svi318_ppi_port_c_w )
{
	/* bit 0-3, keyboard row */
	svi.keyboard_row = data & 0x0f;

	/* bit 4-6, cassette */
	if (svi318_cassette_present(0))
	{
		cassette_change_state(
			image_from_devtype_and_index(IO_CASSETTE, 0),
			(data & 0x10) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED,
			CASSETTE_MASK_MOTOR);
	}
	cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), (data & 0x20) ? -1.0 : +1.0);
	if (svi318_cassette_present(0))
	{
		cassette_change_state(
			image_from_devtype_and_index(IO_CASSETTE, 0),
			(data & 0x40) ? CASSETTE_SPEAKER_ENABLED : CASSETTE_SPEAKER_MUTED,
			CASSETTE_MASK_SPEAKER);
	}

	/* bit 7, key click sound */
	DAC_signed_data_w(0, (data & 0x80) ? 0x7e : 0);
}

static ppi8255_interface svi318_ppi8255_interface =
{
	1,
	{svi318_ppi_port_a_r},
	{svi318_ppi_port_b_r},
	{NULL},
	{NULL},
	{NULL},
	{svi318_ppi_port_c_w}
};

READ8_HANDLER( svi318_ppi_r )
{
	return ppi8255_0_r(offset);
}

WRITE8_HANDLER( svi318_ppi_w )
{
	ppi8255_0_w(offset, data);
}

/* Printer port */

WRITE8_HANDLER( svi318_printer_w )
{
	if (!offset)
	{
		svi.prn_data = data;
	}
	else
	{
		if ((svi.prn_strobe & 1) && !(data & 1))
			printer_output(image_from_devtype_and_index(IO_PRINTER, 0), svi.prn_data);

		svi.prn_strobe = data;
	}
}

READ8_HANDLER( svi318_printer_r )
{
	if (printer_status(image_from_devtype_and_index(IO_PRINTER, 0), 0))
		return 0xfe;

	return 0xff;
}

/* PSG */

/*
 PSG Port A Input: Joysticks
*/

READ8_HANDLER( svi318_psg_port_a_r )
{
	return readinputport(11);
}

/*
 PSG Port B Output: Memory banking & Caps Lock
 Bit Name    Description
  0  /CART   Memory bank 11, ROM 0000-7FFF (Cartridge /CCS1, /CCS2)
  1  /BK21   Memory bank 21, RAM 0000-7FFF
  2  /BK22   Memory bank 22, RAM 8000-FFFF
  3  /BK31   Memory bank 31, RAM 0000-7FFF
  4  /BK32   Memory bank 32, RAM 8000-FFFF
  5  CAPS    Caps-Lock LED
  6  /ROM2   Memory bank 12, ROM 8000-BFFF (Cartridge /CCS3)
  7  /ROM3   Memory bank 12, ROM C000-FFFF (Cartridge /CCS4)
*/

WRITE8_HANDLER( svi318_psg_port_b_w )
{
	svi.cart = BIT(data, 0);
	svi.bk21 = BIT(data, 1);
	svi.bk22 = BIT(data, 2);
	svi.bk31 = BIT(data, 3);
	svi.bk32 = BIT(data, 4);
	set_led_status(0, !(data & 0x20));
	svi.rom2 = BIT(data, 6);
	svi.rom3 = BIT(data, 7);

	svi.bank_switch = data;
	svi318_set_banks();
}

/* Disk drives */

static void svi_fdc_callback(int param)
{
	switch (param)
	{
	case WD179X_IRQ_CLR:
		svi.fdc_irq_drq &= ~0x80;
		break;
	case WD179X_IRQ_SET:
		svi.fdc_irq_drq |= 0x80;
		break;
	case WD179X_DRQ_CLR:
		svi.fdc_irq_drq &= ~0x40;
		break;
	case WD179X_DRQ_SET:
		svi.fdc_irq_drq |= 0x40;
		break;
	}
}

WRITE8_HANDLER( svi318_fdc_drive_motor_w )
{
	switch (data & 3)
	{
	case 1:
		wd179x_set_drive(0);
		svi.fdc_driveselect = 0;
		break;
	case 2:
		wd179x_set_drive(1);
		svi.fdc_driveselect = 1;
		break;
	}
}

WRITE8_HANDLER( svi318_fdc_density_side_w )
{
	mess_image *image;

	wd179x_set_density((data & 0x01) ? DEN_FM_LO : DEN_MFM_LO);
	wd179x_set_side((data & 0x02) ? 1 : 0);

	image = image_from_devtype_and_index(IO_FLOPPY, svi.fdc_driveselect);
	if (image_exists(image))
	{
		UINT8 sectors = (data & 0x01) ? 18 : 17;
		UINT16 sectorSize = (data & 0x01) ? 128 : 256;
		basicdsk_set_geometry(image, 40, svi.fdc_heads[svi.fdc_driveselect], sectors, sectorSize, 1, 0, FALSE);
	}
}

READ8_HANDLER( svi318_fdc_irqdrq_r )
{
	return svi.fdc_irq_drq;
}

static unsigned long svi318_calcoffset(UINT8 t, UINT8 h, UINT8 s,
	UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT8 first_sector_id, UINT16 offset_track_zero)
{
	unsigned long o;

	if ((t == 0) && (h == 0))
		o = (s - first_sector_id) * 128;
	else
		o = ((t * heads + h) * 17 + s - first_sector_id) * 256 - 2048; /* (17*256)-(18*128)=2048 */

	return o;
}

DEVICE_LOAD( svi318_floppy )
{
	int size;
	int id = image_index_in_device(image);

	if (!image_has_been_created(image))
	{
		size = mame_fsize(file);
		switch (size)
		{
		case 172032: /* Single sided */
			svi.fdc_heads[id] = 1;
			break;
		case 346112: /* Double sided */
			svi.fdc_heads[id] = 2;
			break;
		default:
			return INIT_FAIL;
		}
	}
	else
	{
		return INIT_FAIL;
	}

	if (device_load_basicdsk_floppy(image, file) != INIT_PASS)
		return INIT_FAIL;

	basicdsk_set_geometry(image, 40, svi.fdc_heads[id], 17, 256, 1, 0, FALSE);
	basicdsk_set_calcoffset(image, svi318_calcoffset);

	return INIT_PASS;
}

/* 80 column card (SV-806) */

static int svi318_6845_RA = 0;
static int svi318_scr_x = 0;
static int svi318_scr_y = 0;
static int svi318_HSync = 0;
static int svi318_VSync = 0;
static int svi318_DE = 0;

static void svi318_Set_RA(int offset, int data)
{
	svi318_6845_RA = data;
}

static void svi318_Set_HSync(int offset, int data)
{
	svi318_HSync = data;
	if (!svi318_HSync)
	{
		svi318_scr_y++;
		svi318_scr_x = -40;
	}
}

static void svi318_Set_VSync(int offset, int data)
{
	svi318_VSync = data;
	if (!svi318_VSync)
	{
		svi318_scr_y = 0;
	}
}

static void svi318_Set_DE(int offset, int data)
{
	svi318_DE = data;
}

static struct crtc6845_interface svi318_crtc6845_interface = {
	0,
	svi318_Set_RA,
	svi318_Set_HSync,
	svi318_Set_VSync,
	svi318_Set_DE,
	0,
};

static void svi318_80col_init(void)
{
	svi.sv806_ram = auto_malloc(0x800);
	memset(svi.sv806_ram, 0, 0x800);
	crtc6845_config(&svi318_crtc6845_interface);
}

READ8_HANDLER( svi318_crtc_r )
{
	if (offset & 1)
		return crtc6845_register_r(0);
	return 0xff;
}

WRITE8_HANDLER( svi318_crtc_w )
{
	if (offset & 1)
		crtc6845_register_w(0, data);
	else
		crtc6845_address_w(0, data);
}

WRITE8_HANDLER( svi318_crtcbank_w )
{
	svi.sv806_ram_enabled = data & 1;
	svi318_set_banks();
}

static void svi318_80col_plot_char_line(int x, int y, mame_bitmap *bitmap)
{
	if (svi318_DE && svi.sv806_ram)
	{
		unsigned char *data = memory_region(REGION_GFX1);
		int w;
		unsigned char data_byte;
		int char_code;

		char_code = svi.sv806_ram[crtc6845_memory_address_r(0) & 0x07ff];
		data_byte = data ? data[(char_code << 4) + svi318_6845_RA] : 0;

		for (w = 0; w < 8; w++)
		{
			plot_pixel(bitmap, x + w, y, (data_byte & 0x80) ? 1 : 0);
			data_byte <<= 1;
		}
	}
	else
	{
		int w;
		for (w = 0; w < 8; w++)
			plot_pixel(bitmap, x + w, y, 0);
	}
}

static VIDEO_UPDATE( svi318_80col )
{
	long c = 0;

	while (svi318_VSync && c < 33274)
	{
		crtc6845_clock();
		c++;
	}

	while (!svi318_VSync && c < 33274)
	{
		while (svi318_HSync && c < 33274)
		{
			crtc6845_clock();
			c++;
		}

		while (!svi318_HSync && c < 33274)
		{
			if (svi318_scr_x >= 0 && svi318_scr_x < 640 && svi318_scr_y >= 0 && svi318_scr_y < 400)
			{
				svi318_80col_plot_char_line(svi318_scr_x, svi318_scr_y, bitmap);
			}
			svi318_scr_x += 8;
			crtc6845_clock();
			c++;
		}
	}
}

VIDEO_UPDATE( svi328b )
{
	video_update_tms9928a(screen, bitmap, cliprect, do_skip);
	video_update_svi318_80col(screen, bitmap, cliprect, do_skip);
}

MACHINE_RESET( svi328b )
{
	machine_reset_svi318();
	svi318_80col_init();
}

/* Memory banking */

WRITE8_HANDLER( svi318_writemem0 )
{
	int dips = readinputport(13);

	if (svi.cart == 0)
		return;

	if (((svi.bk21 == 0 && (dips & 0x01)) || (svi.bk31 == 0 && (dips & 0x04))) && svi.ram_bank1)
	{
		svi.ram_bank1[offset] = data;
		return;
	}

	if (svi.bk21 == 0 && svi.is_svi328)
	{
		svi.ram[offset] = data;
		return;
	}
}

WRITE8_HANDLER( svi318_writemem1 )
{
	UINT16 addr = 0x8000 + offset;
	int dips = readinputport(13);

	if (svi.cart == 0)
	{
		if (svi.rom2 == 0 && addr < 0xc000)
			return;
		if (svi.rom3 == 0 && addr >= 0xc000)
			return;
	}

	if (svi.sv806_ram_enabled && addr >= 0xf000 && addr < 0xf800 && svi.sv806_ram)
	{
		svi.sv806_ram[addr & 0x7ff] = data;
		return;
	}

	if (((svi.bk22 == 0 && (dips & 0x02)) || (svi.bk32 == 0 && (dips & 0x08))) && svi.ram_bank2)
	{
		svi.ram_bank2[offset] = data;
		return;
	}

	if (svi.ramdis == 1)
	{
		if (svi.is_svi328)
			svi.ram[addr] = data;
		else if (addr >= 0xc000)
			svi.ram[addr - 0xc000] = data;
	}
}

static void svi318_set_banks(void)
{
	int dips = readinputport(13);

	/* Bank 1: 0x0000 - 0x7fff (32KB) */
	if (svi.cart == 0 && svi.cart_rom)
	{
		memory_set_bankptr(1, svi.cart_rom);
	}
	else if (((svi.bk21 == 0 && (dips & 0x01)) || (svi.bk31 == 0 && (dips & 0x04))) && svi.ram_bank1)
	{
		memory_set_bankptr(1, svi.ram_bank1);
	}
	else if (svi.bk21 == 0 && svi.is_svi328)
	{
		memory_set_bankptr(1, svi.ram);
	}
	else if (svi.romdis == 1 && svi.basic_rom)
	{
		memory_set_bankptr(1, svi.basic_rom);
	}
	else
	{
		memory_set_bankptr(1, svi.empty_bank);
	}

	/* Bank 2: 0x8000 - 0xbfff (16KB) */
	if (svi.cart == 0 && svi.rom2 == 0 && svi.cart_rom)
	{
		memory_set_bankptr(2, svi.cart_rom + 0x8000);
	}
	else if (((svi.bk22 == 0 && (dips & 0x02)) || (svi.bk32 == 0 && (dips & 0x08))) && svi.ram_bank2)
	{
		memory_set_bankptr(2, svi.ram_bank2);
	}
	else if (svi.ramdis == 1 && svi.is_svi328)
	{
		memory_set_bankptr(2, svi.ram + 0x8000);
	}
	else
	{
		memory_set_bankptr(2, svi.empty_bank);
	}

	/* Bank 3: 0xc000 - 0xefff (12KB) */
	if (svi.cart == 0 && svi.rom3 == 0 && svi.cart_rom)
	{
		memory_set_bankptr(3, svi.cart_rom + 0xc000);
	}
	else if (((svi.bk22 == 0 && (dips & 0x02)) || (svi.bk32 == 0 && (dips & 0x08))) && svi.ram_bank2)
	{
		memory_set_bankptr(3, svi.ram_bank2 + 0x4000);
	}
	else if (svi.ramdis == 1)
	{
		if (svi.is_svi328)
			memory_set_bankptr(3, svi.ram + 0xc000);
		else
			memory_set_bankptr(3, svi.ram);
	}
	else
	{
		memory_set_bankptr(3, svi.empty_bank);
	}

	/* Bank 4: 0xf000 - 0xf7ff (2KB) */
	if (svi.cart == 0 && svi.rom3 == 0 && svi.cart_rom)
	{
		memory_set_bankptr(4, svi.cart_rom + 0xf000);
	}
	else if (svi.sv806_ram_enabled && svi.sv806_ram)
	{
		memory_set_bankptr(4, svi.sv806_ram);
	}
	else if (((svi.bk22 == 0 && (dips & 0x02)) || (svi.bk32 == 0 && (dips & 0x08))) && svi.ram_bank2)
	{
		memory_set_bankptr(4, svi.ram_bank2 + 0x7000);
	}
	else if (svi.ramdis == 1)
	{
		if (svi.is_svi328)
			memory_set_bankptr(4, svi.ram + 0xf000);
		else
			memory_set_bankptr(4, svi.ram + 0x3000);
	}
	else
	{
		memory_set_bankptr(4, svi.empty_bank);
	}

	/* Bank 5: 0xf800 - 0xffff (2KB) */
	if (svi.cart == 0 && svi.rom3 == 0 && svi.cart_rom)
	{
		memory_set_bankptr(5, svi.cart_rom + 0xf800);
	}
	else if (((svi.bk22 == 0 && (dips & 0x02)) || (svi.bk32 == 0 && (dips & 0x08))) && svi.ram_bank2)
	{
		memory_set_bankptr(5, svi.ram_bank2 + 0x7800);
	}
	else if (svi.ramdis == 1)
	{
		if (svi.is_svi328)
			memory_set_bankptr(5, svi.ram + 0xf800);
		else
			memory_set_bankptr(5, svi.ram + 0x3800);
	}
	else
	{
		memory_set_bankptr(5, svi.empty_bank);
	}
}

/* Interrupts */

void svi318_vdp_interrupt(int state)
{
	svi.intvdp = state;
	if (svi.ctrl1 == 0)
		cpunum_set_input_line(0, INPUT_LINE_NMI, svi.intvdp ? ASSERT_LINE : CLEAR_LINE);
	else
		cpunum_set_input_line(0, 0, (svi.intvdp || svi.intexp) ? HOLD_LINE : CLEAR_LINE);
}

INTERRUPT_GEN( svi318_interrupt )
{
	int set = readinputport(13);
	TMS9928A_set_spriteslimit(set & 0x20);
	TMS9928A_interrupt();
	svi318_set_banks();
}

/* Init and Reset */

DRIVER_INIT( svi318 )
{
	int i, n;
	static int z80_cycle_table[] =
	{
		Z80_TABLE_op, Z80_TABLE_cb, Z80_TABLE_xy,
		Z80_TABLE_ed, Z80_TABLE_xycb, Z80_TABLE_ex
	};

	memset(&svi, 0, sizeof(svi));

	svi.is_svi328 = (strcmp(Machine->gamedrv->name, "svi318") != 0 && strcmp(Machine->gamedrv->name, "svi318n") != 0);

	cpunum_set_input_line_vector(0, 0, 0xff);
	ppi8255_init(&svi318_ppi8255_interface);

	svi.empty_bank = auto_malloc(0x8000);
	memset(svi.empty_bank, 0xff, 0x8000);

	svi.basic_rom = memory_region(REGION_CPU1);

	svi.ram = auto_malloc(0x10000);
	memset(svi.ram, 0, 0x10000);

	svi.ram_bank1 = auto_malloc(0x8000);
	memset(svi.ram_bank1, 0, 0x8000);

	svi.ram_bank2 = auto_malloc(0x8000);
	memset(svi.ram_bank2, 0, 0x8000);

	/* adjust z80 cycles for the M1 wait state */
	for (i = 0; i < sizeof(z80_cycle_table) / sizeof(z80_cycle_table[0]); i++)
	{
		UINT8 *table = auto_malloc(0x100);
		const UINT8 *old_table = cpunum_get_info_ptr(0, CPUINFO_PTR_Z80_CYCLE_TABLE + z80_cycle_table[i]);
		memcpy(table, old_table, 0x100);

		if (z80_cycle_table[i] == Z80_TABLE_ex)
		{
			table[0x66]++; /* NMI overhead */
			table[0xff]++; /* INT overhead */
		}
		else
		{
			for (n = 0; n < 256; n++)
			{
				if (z80_cycle_table[i] == Z80_TABLE_op)
					table[n]++;
				else
					table[n] += 2;
			}
		}
		cpunum_set_info_ptr(0, CPUINFO_PTR_Z80_CYCLE_TABLE + z80_cycle_table[i], (void *)table);
	}

	wd179x_init(WD_TYPE_179X, svi_fdc_callback);
}

MACHINE_RESET( svi318 )
{
	TMS9928A_reset();

	ppi8255_0_w(3, 0x92);

	svi.intvdp = 0;
	svi.intexp = 0;
	svi.romdis = 1;
	svi.ramdis = 1;
	svi.cart = 1;
	svi.bk21 = 1;
	svi.bk22 = 1;
	svi.bk31 = 1;
	svi.bk32 = 1;
	svi.rom2 = 1;
	svi.rom3 = 1;
	svi.ctrl1 = 1;
	svi.keyboard_row = 0;
	svi.bank_switch = 0xff;

	svi318_set_banks();

	wd179x_reset();
}

int svi318_cassette_present(int id)
{
	return image_exists(image_from_devtype_and_index(IO_CASSETTE, id));
}
