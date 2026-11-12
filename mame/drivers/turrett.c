// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    Turret Tower by Dell Electronics

    Press F2 (Service) to start the game after the boot.

****************************************************************************/

#include "driver.h"
#include "cpu/mips/r3000.h"
#include "machine/idectrl.h"
#include "sound/custom.h"
#include "streams.h"
#include <math.h>

#define X_VISIBLE 336
#define Y_VISIBLE 244
#define DIMM_BANK_WORDS (128 * 1024 * 1024 / 2)
#define DIMM_BANK_MASK (DIMM_BANK_WORDS - 1)
#define VRAM_BANK_WORDS (256 * 1024)
#define SOUND_CHANNELS 32
#define CPU_CLOCK 25000000

static UINT16 *ram_bank[2];
static UINT16 *video_ram[2];
static mame_timer *dma_timer;

static UINT32 inputs_active;
static UINT16 last_pixel;
static INT32 video_ctrl;
static UINT16 video_fade;
static INT16 x_pos;
static INT16 x_start;
static INT16 x_mod;
static INT16 dx;
static INT16 y_pos;
static INT16 scale_cnt_y;
static INT16 scale_cnt_x;
static int skip_x;
static int skip_y;
static INT16 scale;
static INT16 hotspot_x;
static INT16 hotspot_y;
static int dma_idle;
static UINT32 dma_addr[2];
static UINT8 frame;
static UINT8 adc;
static UINT32 ipt_prev;

/* sound */
static struct
{
	UINT32 address;
	UINT32 volume;
	int playing;
} channels[SOUND_CHANNELS];
static INT32 volume_table[0x50];
static sound_stream *sound_channel;

/*************************************
 *
 *  Video
 *
 *************************************/

INLINE int clamp_5bit(int val)
{
	if (val < 0)
		return 0;
	if (val > 31)
		return 31;
	return val;
}

VIDEO_UPDATE( turrett )
{
	const UINT16 *vram = video_ram[(~video_ctrl) & 1];
	int fade_b = video_fade & 0x1f;
	int fade_g = (video_fade >> 5) & 0x1f;
	int fade_r = (video_fade >> 10) & 0x1f;
	int x, y;

	if (video_fade & 0x8000) {
		fade_b = -fade_b;
		fade_g = -fade_g;
		fade_r = -fade_r;
	}

	for (y = cliprect->min_y; y <= cliprect->max_y; ++y) {
		const UINT16 *src = &vram[y * X_VISIBLE + cliprect->min_x];
		UINT16 *dest = (UINT16 *)bitmap->line[y] + cliprect->min_x;

		if (video_fade != 0) {
			for (x = cliprect->min_x; x <= cliprect->max_x; ++x) {
				int srcpix = *src++;
				int dst_b = clamp_5bit((srcpix & 0x1f) + fade_b);
				int dst_g = clamp_5bit(((srcpix >> 5) & 0x1f) + fade_g);
				int dst_r = clamp_5bit(((srcpix >> 10) & 0x1f) + fade_r);

				*dest++ = (dst_r << 10) | (dst_g << 5) | dst_b;
			}
		} else {
			for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
				*dest++ = *src++ & 0x7fff;
		}
	}
}

static void update_video_addr(void)
{
	/* handle auto-increment */
	if (dx == x_mod) {
		dx = 0;
		scale_cnt_y += scale;

		if (scale_cnt_y & 0x800) {
			skip_y = 0;
			scale_cnt_y &= 0x7ff;
			--y_pos;
			x_pos = x_start;
		} else {
			skip_y = 1;
		}
	} else {
		++dx;
		scale_cnt_x += scale;

		if (scale_cnt_x & 0x800) {
			scale_cnt_x &= 0x7ff;
			skip_x = 0;
			++x_pos;
		} else {
			skip_x = 1;
		}
	}
}

static UINT32 write_video_ram(UINT16 data)
{
	UINT32 clocks = 1;

	if (!skip_x && !skip_y) {
		/* hot spot test */
		if (x_pos == hotspot_x && y_pos == (hotspot_y & 0xfff))
			hotspot_y |= 0x8000;

		if (x_pos >= 0 && x_pos < X_VISIBLE && y_pos >= 0 && y_pos < Y_VISIBLE) {
			UINT16 *vramptr = &video_ram[video_ctrl & 1][y_pos * X_VISIBLE + x_pos];

			/* blending enabled? */
			if (data & 0x8000) {
				int src_b = data & 0x1f;
				int src_g = (data >> 5) & 0x1f;
				int src_r = (data >> 10) & 0x1f;
				int dst_b = *vramptr & 0x1f;
				int dst_g = (*vramptr >> 5) & 0x1f;
				int dst_r = (*vramptr >> 10) & 0x1f;

				if (video_ctrl & 2) {
					/* additive */
					dst_b = clamp_5bit(src_b + dst_b);
					dst_g = clamp_5bit(src_g + dst_g);
					dst_r = clamp_5bit(src_r + dst_r);
				} else if ((src_g & 1) && (src_b & 1)) {
					/* subtractive, R is always 0 for blended pixels */
					dst_b = clamp_5bit(dst_b - src_b);
					dst_g = clamp_5bit(dst_g - src_g);
					dst_r = clamp_5bit(dst_r - src_r);
				} else {
					/* 75% source, 25% destination */
					dst_b = (src_b - (src_b >> 2)) + (dst_b >> 2);
					dst_g = (src_g - (src_g >> 2)) + (dst_g >> 2);
					dst_r = (src_r - (src_r >> 2)) + (dst_r >> 2);
				}
				clocks += 2;
				*vramptr = (dst_r << 10) | (dst_g << 5) | dst_b;
			} else {
				clocks += 2;
				*vramptr = data;
			}
		}
	}
	update_video_addr();

	return clocks;
}

/* in this core the mem_mask has the accessed lanes cleared */
static READ32_HANDLER( video_r )
{
	mem_mask = ~mem_mask;

	/* collision detection flag */
	if (offset == 3 && mem_mask == 0x0000ffff)
		return hotspot_y & 0x8000;

	return 0;
}

static WRITE32_HANDLER( video_w )
{
	mem_mask = ~mem_mask;

	switch (offset) {
	case 0:
		if (mem_mask == 0xffff0000) {
			data >>= 16;

			if ((data & 0xc400) == 0xc400) {
				/* RLE word */
				int count = (data & 0x3ff) + 1;

				while (count--)
					write_video_ram(last_pixel);
			} else {
				write_video_ram(data);
				last_pixel = data;
			}
		} else {
			video_ctrl = data & 3;
		}
		break;
	case 1:
		x_mod = data & 0xffff;
		scale = 0x800 - (data >> 16);
		break;
	case 2:
		y_pos = data & 0xffff;
		x_pos = data >> 16;

		/* the start of a new line sequence resets the scaling state */
		x_start = x_pos;
		skip_x = 0;
		skip_y = 0;
		dx = 0;
		break;
	case 3:
		if (mem_mask == 0xffff0000) {
			video_fade = data >> 16;
		} else if (mem_mask == 0x0000ffff) {
			if (data & 0x4000)
				hotspot_y = data;
			else
				hotspot_x = data;
		}
		break;
	}
}

static void dma_complete(int param)
{
	dma_idle = 1;
}

static WRITE32_HANDLER( dma_w )
{
	int bank = (~offset >> 1) & 1;

	if ((offset & 1) == 0) {
		dma_addr[bank] = data;
	} else {
		UINT32 clocks = 0;
		UINT32 words = data & 0x0fffffff;

		if (data & 0x10000000) {
			/* IDE to DRAM */
			UINT32 addr = dma_addr[bank];
			UINT16 *ram = ram_bank[bank];

			while (words--) {
				ram[addr & DIMM_BANK_MASK] = ide_bus_0_r(0, 0);
				++addr;
			}

			clocks = 500;
			dma_addr[bank] = addr;
		} else if (data & 0x40000000) {
			/* IDE to video RAM */
			while (words--) {
				UINT16 val = ide_bus_0_r(0, 0);

				write_video_ram(val);
				last_pixel = val;
			}

			clocks = 500;
		} else if (data & 0x80000000) {
			/* RAM to video RAM */
			UINT32 addr = dma_addr[bank];
			UINT16 *ram = ram_bank[bank];

			while (words--) {
				UINT16 val = ram[addr++ & DIMM_BANK_MASK];

				switch (val & 0xc400) {
				case 0x8400 : {
					/* transparent run */
					int run = (((val & 0x3800) >> 1) | (val & 0x03ff)) + 1;

					while (run--)
						update_video_addr();
					break;
				}
				case 0xc400 : {
					int run = (((val & 0x3800) >> 1) | (val & 0x03ff)) + 1;

					while (run--)
						clocks += write_video_ram(last_pixel);
					break;
				}
				default :
					last_pixel = val;
					clocks += write_video_ram(val);
					break;
				}
			}
		}

		/* the busy flag is polled by the game through BrCond3 */
		dma_idle = 0;
		timer_adjust(dma_timer, TIME_IN_NSEC(10) * clocks, 0, 0);
	}
}

/*************************************
 *
 *  Inputs
 *
 *************************************/

static UINT32 update_inputs(void)
{
	UINT32 val = 0;

	if (inputs_active) {
		if (inputs_active & 0x00000001) {
			/* the sticks are centered at 0x20 and the hardware reports them as signed 6 bit values */
			val = 0x00 | ((readinputport(0) ^ 0x20) & 0x3f);
			inputs_active &= ~1;
		} else if (inputs_active & 0x00000002) {
			val = 0x40 | ((readinputport(1) ^ 0x20) & 0x3f);
			inputs_active &= ~2;
		} else if (inputs_active & 0x0000ff00) {
			UINT32 data = readinputport(2);
			UINT32 bits = inputs_active >> 8;
			int i;

			val = 0xc0;
			for (i = 0; i < 8; ++i) {
				if (bits & (1 << i)) {
					val |= i << 1;
					val |= (data >> i) & 1;
					inputs_active &= ~(1 << (i + 8));
					break;
				}
			}
		} else if (inputs_active & 0x00ff0000) {
			UINT32 data = readinputport(3);
			UINT32 bits = inputs_active >> 16;
			int i;

			val = 0xd0;
			for (i = 0; i < 8; ++i) {
				if (bits & (1 << i)) {
					val |= i << 1;
					val |= (data >> i) & 1;
					inputs_active &= ~(1 << (i + 16));
					break;
				}
			}
		} else if (inputs_active & 0x01000000) {
			val = 0xe0 | readinputport(4);
			inputs_active &= ~0x01000000;
		} else if (inputs_active & 0x02000000) {
			val = 0xf0 | readinputport(5);
			inputs_active &= ~0x02000000;
		}
	}

	cpunum_set_input_line(0, R3000_IRQ1, inputs_active ? ASSERT_LINE : CLEAR_LINE);
	return val;
}

static READ32_HANDLER( int_r )
{
	return update_inputs() << 24;
}

/* the digital inputs raise an event when they change, the coin and service ones only when pressed */
static void poll_inputs(void)
{
	UINT32 cur = (readinputport(2) & 0xff) | ((readinputport(3) & 0xff) << 8);
	UINT32 changed = cur ^ ipt_prev;
	UINT32 press_only = 0x0002 | 0x0004 | 0x0010 | 0x0020;

	ipt_prev = cur;

	/* the inputs are active low, a set bit is a release */
	changed &= ~(press_only & cur);
	if (changed) {
		inputs_active |= changed << 8;
		cpunum_set_input_line(0, R3000_IRQ1, ASSERT_LINE);
	}
}

static INTERRUPT_GEN( vblank )
{
	if (frame)
		inputs_active |= 0x01000000;
	else
		inputs_active |= 0x02000000;

	frame ^= 1;
	poll_inputs();
	cpunum_set_input_line(0, R3000_IRQ1, ASSERT_LINE);
}

static INTERRUPT_GEN( adc_interrupt )
{
	if (adc)
		inputs_active |= 0x00000001;
	else
		inputs_active |= 0x00000002;

	adc ^= 1;
	poll_inputs();
	cpunum_set_input_line(0, R3000_IRQ1, ASSERT_LINE);
}

static int sbrc2_r(void)
{
	return cpu_getvblank();
}

static int sbrc3_r(void)
{
	return dma_idle;
}

/*************************************
 *
 *  Hard drive
 *
 *************************************/

/* the game addresses the disk 63 sectors below the standard LBA mapping */
static READ32_HANDLER( ide_r )
{
	mem_mask = ~mem_mask;
	int half = (mem_mask & 0xffff0000) ? 0 : 1;
	int reg = (offset * 2 + half) & 7;
	UINT32 val = ide_bus_0_r(0, reg);

	return half ? val : val << 16;
}

static WRITE32_HANDLER( ide_w )
{
	mem_mask = ~mem_mask;
	int half = (mem_mask & 0xffff0000) ? 0 : 1;
	int reg = (offset * 2 + half) & 7;

	ide_bus_0_w(0, reg, half ? (data & 0xffff) : (data >> 16));
}

static void ide_interrupt(int state)
{
}

static struct ide_interface ide_intf =
{
	ide_interrupt
};

/*************************************
 *
 *  Sound
 *
 *************************************/

static READ32_HANDLER( sound_r )
{
	stream_update(sound_channel, 0);

	return channels[offset & 0x3f].playing ? 0x80000000 : 0;
}

static WRITE32_HANDLER( sound_w )
{
	int ch = offset & 0x3f;

	stream_update(sound_channel, 0);

	if (offset < 0x100 / 4) {
		if (data == 0) {
			channels[ch].playing = 0;
		} else {
			channels[ch].address = data;
			channels[ch].playing = 1;
		}
	} else {
		channels[ch].volume = data;
	}
}

static INT16 sound_ram_read(UINT32 word_addr)
{
	UINT32 byte_addr = (word_addr << 1) & 0xfffffff;

	return ram_bank[byte_addr >> 27][(byte_addr >> 1) & DIMM_BANK_MASK];
}

static void turrett_sound_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	int ch, s;

	for (s = 0; s < length; ++s)
		outputs[0][s] = outputs[1][s] = 0;

	for (ch = 0; ch < SOUND_CHANNELS; ++ch) {
		if (channels[ch].playing) {
			UINT32 addr = channels[ch].address;
			INT32 lvol = volume_table[(channels[ch].volume >> 16) & 0xff];
			INT32 rvol = volume_table[channels[ch].volume & 0xff];
			/* channels 30 and 31 expect interleaved stereo samples */
			int incr = (ch >= 30) ? 2 : 1;

			for (s = 0; s < length; ++s) {
				INT16 sample = sound_ram_read(addr);

				if ((UINT16)sample == 0x8000) {
					channels[ch].playing = 0;
					break;
				}

				addr += incr;
				outputs[0][s] += (sample * (INT64)lvol) >> 17;
				outputs[1][s] += (sample * (INT64)rvol) >> 17;
			}
			channels[ch].address = addr;
		}
	}

	for (s = 0; s < length; ++s) {
		if (outputs[0][s] > 32767)
			outputs[0][s] = 32767;
		else if (outputs[0][s] < -32768)
			outputs[0][s] = -32768;
		if (outputs[1][s] > 32767)
			outputs[1][s] = 32767;
		else if (outputs[1][s] < -32768)
			outputs[1][s] = -32768;
	}
}

static void *turrett_sh_start(int clock, const struct CustomSound_interface *config)
{
	int i;

	sound_channel = stream_create(0, 2, 44100, 0, turrett_sound_update);

	for (i = 0; i < 0x4f; ++i)
		volume_table[i] = (INT32)(65536 * pow(2.0, (-0.375 / 4) * i));

	/* last entry is effectively mute */
	volume_table[0x4f] = 0;

	return auto_malloc(1);
}

static struct CustomSound_interface turrett_sound_interface =
{
	turrett_sh_start
};

/*************************************
 *
 *  Machine initialization
 *
 *************************************/

static MACHINE_START( turrett )
{
	int i;

	/* two banks of 128MB of DIMM memory shared by the CPU DMA and the sound */
	for (i = 0; i < 2; ++i) {
		ram_bank[i] = auto_malloc(DIMM_BANK_WORDS * sizeof(UINT16));
		memset(ram_bank[i], 0, DIMM_BANK_WORDS * sizeof(UINT16));
		video_ram[i] = auto_malloc(VRAM_BANK_WORDS * sizeof(UINT16));
		memset(video_ram[i], 0, VRAM_BANK_WORDS * sizeof(UINT16));
	}

	state_save_register_global_pointer(video_ram[0], VRAM_BANK_WORDS);
	state_save_register_global_pointer(video_ram[1], VRAM_BANK_WORDS);

	ide_controller_init(0, &ide_intf);
	ide_set_lba_offset(0, 63);

	dma_timer = timer_alloc(dma_complete);

	state_save_register_global(inputs_active);
	state_save_register_global(last_pixel);
	state_save_register_global(video_ctrl);
	state_save_register_global(video_fade);
	state_save_register_global(x_pos);
	state_save_register_global(x_start);
	state_save_register_global(x_mod);
	state_save_register_global(dx);
	state_save_register_global(y_pos);
	state_save_register_global(scale_cnt_y);
	state_save_register_global(scale_cnt_x);
	state_save_register_global(skip_x);
	state_save_register_global(skip_y);
	state_save_register_global(scale);
	state_save_register_global(hotspot_x);
	state_save_register_global(hotspot_y);
	state_save_register_global(dma_idle);
	state_save_register_global_array(dma_addr);
	state_save_register_global(frame);
	state_save_register_global(adc);
	state_save_register_global(ipt_prev);
	return 0;
}

static MACHINE_RESET( turrett )
{
	dma_idle = 1;
	frame = 0;
	adc = 0;
	inputs_active = 0;
	ipt_prev = 0xffff;
	memset(channels, 0, sizeof(channels));
	ide_controller_reset(0);
}

static VIDEO_START( turrett )
{
	return 0;
}

/*************************************
 *
 *  Memory maps
 *
 *************************************/

static ADDRESS_MAP_START( cpu_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_RAM
	AM_RANGE(0x02000010, 0x02000013) AM_RAM
	AM_RANGE(0x02000040, 0x02000043) AM_RAM
	AM_RANGE(0x02000050, 0x02000053) AM_RAM
	AM_RANGE(0x02000060, 0x02000063) AM_RAM
	AM_RANGE(0x02000070, 0x02000073) AM_RAM
	AM_RANGE(0x04000000, 0x0400000f) AM_WRITE(dma_w)
	AM_RANGE(0x04000100, 0x04000103) AM_READWRITE(int_r, MWA32_NOP)
	AM_RANGE(0x04000200, 0x040003ff) AM_READWRITE(sound_r, sound_w)
	AM_RANGE(0x08000000, 0x0800000f) AM_READWRITE(video_r, video_w)
	AM_RANGE(0x08000200, 0x080003ff) AM_READWRITE(ide_r, ide_w)
	AM_RANGE(0x1fc00000, 0x1fdfffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( turrett )
	PORT_START
	PORT_BIT( 0x3f, 0x20, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0x3f) PORT_SENSITIVITY(60) PORT_KEYDELTA(2)

	PORT_START
	PORT_BIT( 0x3f, 0x20, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0x3f) PORT_SENSITIVITY(60) PORT_KEYDELTA(2)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BILL1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Floor mat")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Door Lock") PORT_TOGGLE

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("Seat Belt") PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("Home")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_NAME("Emergency Stop") PORT_TOGGLE
INPUT_PORTS_END

/*************************************
 *
 *  Machine driver
 *
 *************************************/

static struct r3000_config config =
{
	0,		/* 1 if we have an FPU, 0 otherwise */
	4096,	/* code cache size */
	4096,	/* data cache size */
	sbrc2_r,
	sbrc3_r
};

static MACHINE_DRIVER_START( turrett )
	MDRV_CPU_ADD(R3000BE, CPU_CLOCK)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(cpu_map, 0)
	MDRV_CPU_VBLANK_INT(vblank, 1)
	MDRV_CPU_PERIODIC_INT(adc_interrupt, TIME_IN_HZ(60))

	MDRV_FRAMES_PER_SECOND(4000000.0 / (512 * 259))
	MDRV_VBLANK_DURATION(1919)
	MDRV_MACHINE_START(turrett)
	MDRV_MACHINE_RESET(turrett)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(X_VISIBLE, Y_VISIBLE)
	MDRV_VISIBLE_AREA(0, X_VISIBLE - 1, 0, Y_VISIBLE - 1)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_VIDEO_START(turrett)
	MDRV_VIDEO_UPDATE(turrett)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(turrett_sound_interface)
	MDRV_SOUND_ROUTE(0, "right", 1.0)
	MDRV_SOUND_ROUTE(1, "left", 1.0)
MACHINE_DRIVER_END

/*************************************
 *
 *  ROM definition
 *
 *************************************/

ROM_START( turrett )
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "turret.u13", 0x000000, 0x080000, CRC(85287007) SHA1(990b954905c66340d3e88918b2f8cc7f1b9c7cf4) )
	ROM_LOAD32_BYTE( "turret.u12", 0x000001, 0x080000, CRC(a2a498fc) SHA1(47f2c9c9f2496b49fd923acb400166e963095e1d) )
	ROM_LOAD32_BYTE( "turret.u8",  0x000002, 0x080000, CRC(ddff4898) SHA1(a8f859a0dcab8ec83fbfe255d58b3e644933b923) )
	ROM_LOAD32_BYTE( "turret.u7",  0x000003, 0x080000, CRC(fa8b5a5a) SHA1(658e9eeadc9c70185973470565d562c76f4fcdd7) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "turrett", 0, BAD_DUMP SHA1(b0c98c5876870dd8b3e37a38fe35846c9e011df4) )
ROM_END

GAME( 2001, turrett, 0, turrett, turrett, 0, ROT0, "Dell Electronics (Namco license)", "Turret Tower - The Enemy Has Arrived", 0 )
