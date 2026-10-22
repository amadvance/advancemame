/************************************************************************
 See See Find Out [Icarus 1999]
 Pang Pang Car [Icarus 1999]

 driver by
  Tomasz Slanina  analog[at]op.pl

TODO:
 - move PS7500 stuff to machine/ps7500.c
 - 24c01 eeprom (IOLINES)
 - timing
 - unknown reads/writes
 - sound envelopes and filters
 - Pang Pang Car music requires the undumped QS1000 internal program ROM

*************************************************************************************************************

See See Find Out
Icarus, 1999

PCB Layout
----------

ARIAMEDIA M/B Rev 1.0
|--------------------------------------|
|          B.U29          C.U12        |
|      A.U28     *     *     D.U13     |
|                               1008S-1|
|                    |------|          |
|    KM416C1204      |QS1000|   E.U14  |
|J       KM416C1204  |------| |--------|
|A                     24MHz  |DU2  DU3|
|M     |---------|            |        |
|M     |CL-PS7500|  |------|  |DU5  DU6|
|A     |         |  |QL2003|  |--------|
|      |         |  |      | 24C01A.U36|
|      |         |  |------|           |
|      |---------|          14.31818MHz|
|            54MHz  |------|           |
|                   |PRIME |           |
|                   |------|   DIPSW(8)|
|--------------------------------------|

Notes:

      Chips:
         QS1000: QDSP QS1000 AdMOS 9638R, Wavetable Audio chip, clock input of 24.000MHz (QFP100)
                 see http://www.hwass.co.kr/product.htm for more info on QS100x chips.
          PRIME: LGS Prime 3C 9930R, clock input of 14.31818MHz (QFP100)
         QL2003: QuickLogic QL2003-XPL84C 0003BA FPGA (PLCC84)
      CL-PS7500: Cirrus Logic CL-PS7500FE-56QC-A 84903-951BD ARM 9843J
                 clock input of 54.000MHz, ARM710C; ARM7-core CPU (QFP240)
     KM416C1204: Samsung KM416C1204CJ-5 2M x8 DRAM (SOJ42)
              *: Unpopulated DIP32 socket

      ROMs:
           24C010A: Amtel 24C01A (128bytes x8Bit Serial EEPROM, DIP8)
        A, B, C, D: TMS27C040 512K x8 EPROM (DIP32)
                 E: ST M27C512 64K x8 EPROM (DIP28)
           1008S-1: HWASS 1008S-1 Wavetable Audio Samples chip, 1M x8 MaskROM (SOP32)
         DU2, DU3,: Samsung KM29W32000AT 32MBit NAND Flash 3.3V Serial EEPROM (TSOP44)
         DU5, DU6   These ROMs are mounted on a small plug-in daughterboard. There are additional
                    mounting pads for 4 more of these ROMs but they're not populated.


**************************************************************************************************************

Pong Pong Car
Icarus, 1999

This game runs on hardware that is similar to that used on 'See See Find Out'
The game is a rip-off of RallyX

PCB Layout
----------

|--------------------------------------|
|    KM416C1204                        |
|    KM416C1204    U24  U25  U26*  U27*|
|  DA1311A                             |
|  DA1311A        |---------|54MHz     |
|  4558           |CL-PS7500| LED      |
|J                |         | |--------|
|A                |         | |DU2  DU3|
|M                |         | |        |
|M EL2386       @ |---------| |DU5  DU6|
|A       &                    |--------|
|                                      |
|  7660             |------|           |
|                   |PRIME |14.31818MHz|
|                   |------|           |
| NASN9289  XILINX                     |
| QS1000    XCS10    MAX232    DIPSW(8)|
|  24MHz     #   7705                  |
|--------------------|DB9|-------------|
                     |---|
Notes:
      Chips:
         QS1000: QDSP QS1000 AdMOS 9638R, Wavetable Audio chip, clock input of 24.000MHz (QFP100)
                 see http://www.hwass.co.kr/product.htm for more info on QS100x chips.
          PRIME: LGS Prime 3C 9849R, clock input of 14.31818MHz (QFP100)
   XILINX XCS10: Xilinx Spartan XCS10 FPGA (QFP144)
      CL-PS7500: Cirrus Logic CL-PS7500FE-56QC-A 84877-951BD ARM 9843J
                 clock input of 54.000MHz, ARM710C; ARM7-core CPU (QFP240)
     KM416C1204: Samsung KM416C1204CJ-5 2M x8 DRAM (SOJ42)
           7705: Reset/Watchdog IC (SOIC8)
           7660: DC-DC Voltage Convertor (SOIC8)
         EL2386: Elantec Semiconductor 250MHz Triple Current Feedback Op Amp with Disable (SOIC16)
              *: Unpopulated DIP32 sockets
              &: Unpopulated location for QFP100 IC
              #: Unpopulated location for SOJ42 RAM
              @: Unpopulated location for OSC1

      ROMs:
          U24, U25: AMD 29F040B 512k x8 FlashROM (DIP32)
          NASN9289: Re-badged SOP32 ROM. Should be compatible with existing QS100x Wavetable Audio Sample ROMs,
                    Dumped as 1M x8 SOP32 MaskROM
         DU2, DU3,: Samsung KM29N32000TS 32MBit NAND Flash 3.3V Serial EEPROM (TSOP44)
         DU5, DU6   These ROMs are mounted on a small plug-in daughterboard. There are additional
                    mounting pads for 4 more of these ROMs but they're not populated.

*/

#include "driver.h"
#include "cpu/arm7/arm7core.h"
#include "cpu/i8051/i8051.h"
#include "sound/custom.h"
#include "streams.h"
#include "sound/dac.h"
#include "sound/okim6295.h"

enum
{
	IOCR=0,
	KBDDAT,
	KBDCR,
	IOLINES,
	IRQSTA,
	IRQRQA,
	IRQMSKA,
	SUSMODE,
	IRQSTB,
	IRQRQB,
	IRQMSKB,
	STOPMODE,
	FIQST,
	FIQRQ,
	FIQMSK,
	CLKCTL,
	T0low,
	T0high,
	T0GO,
	T0LAT,
	T1low,
	T1high,
	T1GO,
	T1LAT,
	IRQSTC,
	IRQRQC,
	IRQMSKC,
	VIDMUX,
	IRQSTD,
	IRQRQD,
	IRQMSKD,

	ROMCR0=32,
	ROMCR1,

	REFCR=35,

	ID0=37,
	ID1,
	VERSION,

	MSEDAT=42,
	MSECR,

	IOTCR=49,
	ECTCR,
	ASTCR,
	DRAMCR,
	SELREF,

	ATODICR=56,
	ATODSR,
	ATODCC,
	ATODCNT1,
	ATODCNT2,
	ATODCNT3,
	ATODCNT4,

	SD0CURA=96,
	SD0ENDA,
	SD0CURB,
	SD0ENDB,
	SD0CR,
	SD0ST,

	CURSCUR=112,
	CURSINIT,
	VIDCURB,

	VIDCURA=116,
	VIDEND,
	VIDSTART,
	VIDINITA,
	VIDCR,

	VIDINITB=122,

	DMAST=124,
	DMARQ,
	DMASK,
	MAXIO=128
};

UINT32 PS7500_IO[MAXIO];
UINT32 PS7500_FIFO[256];
static UINT32 *vram;
static UINT32 flashAdr,flashOffset,adrLatch,flashType,flashN;

static void PS7500_startTimer0(void);
static void PS7500_startTimer1(void);

static void *PS7500timer0;
static void *PS7500timer1;


/* QS1000 synthesis: Philip Bennett */
#define QS_ADDRESS_MASK 0xffffff

struct qs_voice
{
	UINT32 acc, start, addr, end, freq, adpcm_addr;
	int playing, compressed;
	UINT8 regs[16];
	struct adpcm_state adpcm;
	INT16 signal;
};

static struct qs_voice qs_voices[32];
static UINT8 qs_regs[0x12], qs_serial;
static sound_stream *qs_stream;
static void *sound_timer;
static UINT32 sound_cur, sound_end;
static int sound_bank, sound_ready[2];
static UINT8 sound_frequency, sound_control;

static UINT8 qs_read(UINT32 address)
{
	UINT32 size = memory_region_length(REGION_SOUND1);
	address &= QS_ADDRESS_MASK;
	return address < size ? memory_region(REGION_SOUND1)[address] : 0;
}

static void qs_start_voice(struct qs_voice *voice)
{
	UINT32 table = (voice->regs[1] << 16) | (voice->regs[2] << 8) | voice->regs[3];
	UINT32 base = (qs_read(table + 4) << 8) | qs_read(table + 5);
	UINT8 high = qs_read(base);

	voice->playing = 0;
	voice->freq = (qs_read(table) << 8) | qs_read(table + 1);
	if (!voice->freq)
		return;
	voice->start = ((high << 16) | (qs_read(base + 1) << 8) | qs_read(base + 2)) & QS_ADDRESS_MASK;
	voice->end = (((high & 0xf0) << 16) | ((qs_read(base + 5) & 15) << 16) |
		(qs_read(base + 6) << 8) | qs_read(base + 7)) & QS_ADDRESS_MASK;
	voice->addr = voice->start;
	voice->acc = 0;
	voice->compressed = (qs_read(base + 8) & 8) != 0;
	voice->adpcm_addr = 0xffffffff;
	voice->signal = 0;
	reset_adpcm(&voice->adpcm);
	voice->playing = voice->start < voice->end;
}

static WRITE8_HANDLER(qs_wave_w)
{
	struct qs_voice *voice = &qs_voices[qs_regs[0x0e] & 31];
	stream_update(qs_stream, 0);
	if (offset == 0)
	{
		if (data == 0)
		{
			memcpy(voice->regs, qs_regs, sizeof(voice->regs));
			qs_start_voice(voice);
		}
		/* Release envelopes and sustained loops remain unimplemented. */
	}
	else if (offset <= 0x0d && qs_regs[0x11] == 3)
		voice->regs[offset] = data;
	else
		qs_regs[offset] = data;
}

static void qs_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	int ch, s;
	memset(outputs[0], 0, samples * sizeof(*outputs[0]));
	memset(outputs[1], 0, samples * sizeof(*outputs[1]));
	for (ch = 0; ch < 32; ch++)
	{
		struct qs_voice *voice = &qs_voices[ch];
		for (s = 0; s < samples && voice->playing; s++)
		{
			int result;
			if (voice->addr >= voice->end)
			{
				voice->playing = 0;
				break;
			}
			if (voice->compressed)
			{
				while (voice->adpcm_addr != voice->addr - voice->start)
				{
					UINT8 data;
					voice->adpcm_addr++;
					data = qs_read(voice->start + (voice->adpcm_addr >> 1));
					voice->signal = clock_adpcm(&voice->adpcm,
						(voice->adpcm_addr & 1 ? data : data >> 4) & 15);
				}
				result = (voice->signal >> 4) * 4;
			}
			else
				result = (int)qs_read(voice->addr) - 128;
			outputs[0][s] += result * voice->regs[6] * voice->regs[8] / 4096;
			outputs[1][s] += result * voice->regs[7] * voice->regs[8] / 4096;
			voice->acc += voice->freq;
			voice->addr = (voice->addr + (voice->acc >> 18)) & QS_ADDRESS_MASK;
			voice->acc &= (1 << 18) - 1;
		}
	}
}

static void *qs_sound_start(int clock, const struct CustomSound_interface *config)
{
	qs_stream = stream_create(0, 2, clock / 32, NULL, qs_update);
	return qs_stream;
}

static struct CustomSound_interface qs_sound_interface = { qs_sound_start };

static int qs_serial_r(void)
{
	return qs_serial;
}

static WRITE32_HANDLER(ssfindo_sound_w)
{
	if (!(mem_mask & 0x00ff0000))
	{
		qs_serial = data >> 16;
		cpunum_set_input_line(1, I8051_RX_LINE, ASSERT_LINE);
		cpu_spinuntil_time(TIME_IN_USEC(2000));
	}
}

static READ8_HANDLER(qs_port_r)
{
	return 0xff;
}

static ADDRESS_MAP_START(qs_program_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_ROM AM_REGION(REGION_CPU2, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(qs_data_map, ADDRESS_SPACE_DATA, 8)
	AM_RANGE(0x0000, 0x00ff) AM_RAM
	AM_RANGE(0x0200, 0x0211) AM_WRITE(qs_wave_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(qs_port_map, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x00, 0x03) AM_READ(qs_port_r) AM_WRITENOP
ADDRESS_MAP_END

static void sound_swap(void)
{
	sound_cur = PS7500_IO[SD0CURA + sound_bank * 2];
	sound_end = sound_cur + (PS7500_IO[SD0ENDA + sound_bank * 2] & 0x00fffff0) + 16;
}

static void sound_tick(int param)
{
	UINT32 data;
	if (!sound_ready[sound_bank] || !(sound_control & 2))
		return;
	cpuintrf_push_context(0);
	data = program_read_dword(sound_cur);
	cpuintrf_pop_context();
	DAC_signed_data_16_w(0, (data & 0xffff) ^ 0x8000);
	DAC_signed_data_16_w(1, (data >> 16) ^ 0x8000);
	sound_cur += 4;
	if (sound_cur >= sound_end)
	{
		sound_ready[sound_bank] = 0;
		sound_bank ^= 1;
		sound_swap();
	}
}

static void sound_rate(void)
{
	if (PS7500_IO[SD0CR] & 0x20)
	{
		double period = TIME_IN_USEC((sound_frequency + 2) * 2);
		timer_adjust(sound_timer, period, 0, period);
	}
	else
	{
		timer_adjust(sound_timer, TIME_NEVER, 0, 0);
		DAC_signed_data_16_w(0, 0x8000);
		DAC_signed_data_16_w(1, 0x8000);
	}
}

VIDEO_UPDATE(ssfindo)
{
	int s,x,y;

	if( PS7500_IO[VIDCR]&0x20) //video DMA enabled
	{
		s=( (PS7500_IO[VIDINITA]&0x1fffffff)-0x10000000)/4;
		if(s>=0 && s + 320 * 256 / 4 <= (flashType ? 0x1000000 : 0x2000000) / 4)
		{
			for(y=0;y<256;y++)
				for(x=0;x<320;x+=4)
				{
					plot_pixel(bitmap,x+0,y,vram[s]&0xff);
					plot_pixel(bitmap,x+1,y,(vram[s]>>8)&0xff);
					plot_pixel(bitmap,x+2,y,(vram[s]>>16)&0xff);
					plot_pixel(bitmap,x+3,y,(vram[s]>>24)&0xff);
					s++;
				}
		}
	}
}

static WRITE32_HANDLER(FIFO_w)
{
	PS7500_FIFO[data>>28]=data;
	if ((data >> 24) == 0xb0)
	{
		sound_frequency = data & 0xff;
		sound_rate();
	}
	else if ((data >> 24) == 0xb1)
		sound_control = data & 0xff;

	if(!(data>>28))
	{
		palette_set_color(PS7500_FIFO[1]&0xff, data&0xff,(data>>8)&0xff,(data>>16)&0xff);
		PS7500_FIFO[1]++; //autoinc
	}
}
static void PS7500_Timer0_callback(int val)
{
	PS7500_IO[IRQSTA]|=0x20;
	if(PS7500_IO[IRQMSKA]&0x20)
	{
		cpunum_set_input_line(0, ARM7_IRQ_LINE, PULSE_LINE);
	}
}

static void PS7500_startTimer0(void)
{
	int val=((PS7500_IO[T0low]&0xff)|((PS7500_IO[T0high]&0xff)<<8))>>1;

	if(val==0)
		timer_adjust(PS7500timer0, TIME_NEVER, 0, 0);
	else
		timer_adjust(PS7500timer0, TIME_IN_USEC(val ), 0, TIME_IN_USEC(val ));
}

static void PS7500_Timer1_callback(int val)
{
	PS7500_IO[IRQSTA]|=0x40;
	if(PS7500_IO[IRQMSKA]&0x40)
	{
		cpunum_set_input_line(0, ARM7_IRQ_LINE, PULSE_LINE);
	}
}

static void PS7500_startTimer1(void)
{
	int val=((PS7500_IO[T1low]&0xff)|((PS7500_IO[T1high]&0xff)<<8))>>1;
	if(val==0)
		timer_adjust(PS7500timer1, TIME_NEVER, 0, 0);
	else
		timer_adjust(PS7500timer1, TIME_IN_USEC(val ), 0, TIME_IN_USEC(val ));
}

static INTERRUPT_GEN( ssfindo_interrupt )
{
	PS7500_IO[IRQSTA]|=0x08;
		if(PS7500_IO[IRQMSKA]&0x08)
		{
			cpunum_set_input_line(0, ARM7_IRQ_LINE, PULSE_LINE);
		}
}

static void PS7500_reset(void)
{
		memset(PS7500_IO, 0, sizeof(PS7500_IO));
		memset(PS7500_FIFO, 0, sizeof(PS7500_FIFO));
		flashAdr = flashOffset = adrLatch = flashN = 0;
		sound_bank = sound_cur = sound_end = 0;
		sound_ready[0] = sound_ready[1] = 0;
		sound_frequency = sound_control = 0;
		sound_rate();
		memset(qs_voices, 0, sizeof(qs_voices));
		memset(qs_regs, 0, sizeof(qs_regs));
		qs_serial = 0;
		PS7500_IO[IOCR]			=	0x3f;
		PS7500_IO[VIDCR]		=	0;

		timer_adjust( PS7500timer0, TIME_NEVER, 0, 0 );
		timer_adjust( PS7500timer1, TIME_NEVER, 0, 0 );
}

static READ32_HANDLER(PS7500_IO_r)
{

	switch(offset)
	{
		case SD0CR:
			return (PS7500_IO[SD0CR] & 0x20) | 0x10;
		case SD0ST:
			return ((!sound_ready[0] && !sound_ready[1]) << 2) |
				((!sound_ready[0] || !sound_ready[1]) << 1) | sound_bank;
		case SD0CURA:
		case SD0ENDA:
		case SD0CURB:
		case SD0ENDB:
			return PS7500_IO[offset];
		case MSECR:
			return mame_rand();
		break;

		case IOLINES: //TODO: eeprom  24c01
#if 0
		printf("IOLINESR %i @%x\n",offset,activecpu_get_pc());
#endif

		if(flashType==1)
			return 0;
		else
			return mame_rand();

		case IRQSTA:
			return (PS7500_IO[offset]&(~2))|0x80;

		case IRQRQA:
			return (PS7500_IO[IRQSTA]&PS7500_IO[IRQMSKA])|0x80;

		case IOCR: //TODO: nINT1, OD[n] p.81
			return (input_port_0_r(0)&0x80)|0x34|3;

		case VIDCR:
			return (PS7500_IO[offset]|0x50)&0xfffffff0;

		case T1low:
		case T0low:
		case T1high:
		case T0high:
		case IRQMSKA:
		case VIDEND:
		case VIDSTART:
		case VIDINITA: //TODO: bits 29 ("equal") and 30 (last bit)  p.105

			return PS7500_IO[offset];

		//default:
			//printf("ior %i @%x\n",offset,activecpu_get_pc());
	}
	return mame_rand();//PS7500_IO[offset];
}

static WRITE32_HANDLER(PS7500_IO_w)
{
	UINT32 temp=PS7500_IO[offset];

	COMBINE_DATA(&temp);

	switch(offset)
	{
		case SD0CURA:
		case SD0CURB:
			PS7500_IO[offset] = temp & 0x1ffffff0;
			break;
		case SD0ENDA:
		case SD0ENDB:
			PS7500_IO[offset] = temp;
			sound_ready[(offset - SD0ENDA) / 2] = 1;
			break;
		case SD0CR:
			PS7500_IO[offset] = temp;
			if (temp & 0x20)
			{
				sound_bank = 0;
				sound_swap();
			}
			sound_rate();
			break;
		case IOLINES: //TODO: eeprom  24c01
			PS7500_IO[offset]=data;
				if(data&0xc0)
					adrLatch=0;

			if(activecpu_get_pc() == 0xbac0 && flashType == 1)
			{
				flashN=data&1;
			}

#if 0
				logerror("IOLINESW %i = %x  @%x\n",offset,data,activecpu_get_pc());
#endif
			break;

		case IRQRQA:
			PS7500_IO[IRQSTA]&=~temp;
		break;

		case IRQMSKA:
			PS7500_IO[IRQMSKA]=(temp&(~2))|0x80;
		break;

		case T1GO:
				PS7500_startTimer1();
			break;

		case T0GO:
			PS7500_startTimer0();
		break;

		case VIDEND:
		case VIDSTART:
			COMBINE_DATA(&PS7500_IO[offset]);
			PS7500_IO[offset]&=0xfffffff0; // qword align
		break;

		case IOCR:
		case REFCR:
		case DRAMCR:
		case ROMCR0:
		case VIDMUX:
		case CLKCTL:
		case T1low:
		case T0low:
		case T1high:
		case T0high:
		case VIDCR:
		case VIDINITA: //TODO: bit 30 (last bit) p.105
					COMBINE_DATA(&PS7500_IO[offset]);
		break;

	}
}

static READ32_HANDLER(io_r)
{
	UINT16 *FLASH = (UINT16 *)memory_region(REGION_USER2); //16 bit - WORD access

	int adr=flashAdr*0x200+(flashOffset);


	switch(flashType)
	{
		case 0:
			if(PS7500_IO[IOLINES]&1) //bit 0 of IOLINES  = flash select ( 5/6 or 3/2 )
				adr+=0x400000;
		break;

		case 1:
			adr+=0x400000*flashN;
		break;
	}

	if(adr<0x400000*2)
	{
		flashOffset++;
		return FLASH[adr];
	}
	return 0;
}

static WRITE32_HANDLER(io_w)
{
	UINT32 temp = 0;
	COMBINE_DATA(&temp);

#if 0
	logerror("[io_w] = %x @%x [latch=%x]\n",data,activecpu_get_pc(),adrLatch);
#endif

	if(adrLatch==1)
		flashAdr=(temp>>16)&0xff;
	if(adrLatch==2)
	{
		flashAdr|=(temp>>16)&0xff00;
		flashOffset=0;
	}
	adrLatch=(adrLatch+1)%3;
}

static WRITE32_HANDLER(debug_w)
{
#if 0
	printf("%c",data&0xff); //debug texts - malloc (ie "64 KBytes allocated, elapsed : 378 KBytes, free : 2231 KBytes")
#endif
}

static READ32_HANDLER(ff4_r)
{
	return mame_rand()&0x20;
}

static READ32_HANDLER(SIMPLEIO_r)
{
	return mame_rand()&1;
}

static READ32_HANDLER(randomized_r)
{
	return mame_rand();
}

static ADDRESS_MAP_START( ssfindo_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x03200000, 0x032001ff) AM_READWRITE(PS7500_IO_r,PS7500_IO_w)
	AM_RANGE(0x03012e60, 0x03012e67) AM_NOP
	AM_RANGE(0x03012fe0, 0x03012fe3) AM_WRITE(debug_w)
	AM_RANGE(0x03012ff0, 0x03012ff3) AM_NOP
	AM_RANGE(0x03012ff4, 0x03012ff7) AM_WRITENOP AM_READ(ff4_r) //status flag ?
	AM_RANGE(0x03012ff8, 0x03012fff) AM_NOP
	AM_RANGE(0x03240000, 0x03240003) AM_WRITENOP AM_READ(input_port_1_dword_r)
	AM_RANGE(0x03241000, 0x03241003) AM_WRITENOP AM_READ(input_port_2_dword_r)
	AM_RANGE(0x03242000, 0x03242003) AM_READ(io_r) AM_WRITE(io_w)
	AM_RANGE(0x03243000, 0x03243003) AM_WRITENOP AM_READ(input_port_3_dword_r)
	AM_RANGE(0x0324f000, 0x0324f003) AM_READ(SIMPLEIO_r)
	AM_RANGE(0x03245000, 0x03245003) AM_WRITE(ssfindo_sound_w)
	AM_RANGE(0x03400000, 0x037fffff) AM_WRITE(FIFO_w)
	AM_RANGE(0x10000000, 0x11ffffff) AM_RAM AM_BASE (&vram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ppcar_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x03200000, 0x032001ff) AM_READWRITE(PS7500_IO_r,PS7500_IO_w)
	AM_RANGE(0x03012bf4, 0x03012bf7) AM_READ(randomized_r) AM_WRITENOP
	AM_RANGE(0x03012de0, 0x03012de3) AM_READ(input_port_3_dword_r)
	AM_RANGE(0x03012e60, 0x03012e67) AM_WRITENOP
	AM_RANGE(0x03012ff8	,0x03012ffb) AM_WRITENOP AM_READ(input_port_1_dword_r)
	AM_RANGE(0x032c0000, 0x032c0003) AM_WRITENOP AM_READ(input_port_2_dword_r)
	AM_RANGE(0x03340000, 0x03340007) AM_WRITENOP
	AM_RANGE(0x03341000, 0x0334101f) AM_WRITENOP
	AM_RANGE(0x033c0000, 0x033c0003) AM_READ(io_r) AM_WRITE(io_w)
	AM_RANGE(0x03400000, 0x037fffff) AM_WRITE(FIFO_w)
	AM_RANGE(0x08000000, 0x08ffffff) AM_RAM
	AM_RANGE(0x10000000, 0x10ffffff) AM_RAM AM_BASE (&vram)
ADDRESS_MAP_END


static MACHINE_RESET( ssfindo )
{
	PS7500_reset();
	if (!flashType)
		i8051_set_serial_rx_callback(qs_serial_r);
}

INPUT_PORTS_START( ssfindo )
	PORT_START_TAG("PS7500")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START_TAG("IN 0")
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1	) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1	)

	PORT_START_TAG("IN 1")

	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1	) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2	)

	PORT_START_TAG("DSW")

	PORT_DIPNAME( 0x01, 0x01, "Test Mode" )
		PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Free_Play ) )
		PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Service_Mode ) )
		PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x04, 0x04, "DSW 2" )
		PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x08, 0x08, "DSW 3" )
		PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x10, 0x010, "DSW 4" )
		PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x20, 0x20, "DSW 5" )
		PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x40, 0x040, "DSW 6" )
		PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


INPUT_PORTS_START( ppcar )
	PORT_START_TAG("PS7500")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START_TAG("IN 0")
		PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_COIN1	)

	PORT_START_TAG("IN 1")
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2	) PORT_PLAYER(1)
		PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1	) PORT_PLAYER(1)
		PORT_BIT( 0x008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(1)
		PORT_BIT( 0x010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(1)
		PORT_BIT( 0x020, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(1)
		PORT_BIT( 0x040, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(1)
		PORT_BIT( 0x080, IP_ACTIVE_LOW, IPT_START1	)

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x03, 0x00, "Credits to Start" )
	PORT_DIPSETTING( 0x00, "1" )
	PORT_DIPSETTING( 0x01, "2" )
	PORT_DIPSETTING( 0x02, "3" )
	PORT_DIPSETTING( 0x03, "4" )
	PORT_DIPNAME( 0x0c, 0x00, "Missiles" )
	PORT_DIPSETTING( 0x0c, "2" )
	PORT_DIPSETTING( 0x08, "2" )
	PORT_DIPSETTING( 0x04, "3" )
	PORT_DIPSETTING( 0x00, "4" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING( 0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING( 0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING( 0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING( 0x60, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x80, DEF_STR( On ) )
INPUT_PORTS_END


static MACHINE_DRIVER_START( ssfindo )

	/* basic machine hardware */
	MDRV_CPU_ADD(ARM7, 54000000) // guess...
	MDRV_CPU_PROGRAM_MAP(ssfindo_map,0)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_CPU_VBLANK_INT(ssfindo_interrupt,1)
	MDRV_CPU_ADD(I8052, 24000000)
	MDRV_CPU_PROGRAM_MAP(qs_program_map,0)
	MDRV_CPU_DATA_MAP(qs_data_map,0)
	MDRV_CPU_IO_MAP(qs_port_map,0)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_RESET(ssfindo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(ssfindo)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 24000000)
	MDRV_SOUND_CONFIG(qs_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.25)
	MDRV_SOUND_ROUTE(1, "right", 0.25)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.25)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ppcar )

	/* basic machine hardware */
	MDRV_CPU_ADD(ARM7, 54000000) // guess...
	MDRV_CPU_PROGRAM_MAP(ppcar_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_CPU_VBLANK_INT(ssfindo_interrupt,1)
	MDRV_MACHINE_RESET(ssfindo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(ssfindo)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.25)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.25)
MACHINE_DRIVER_END


ROM_START( ssfindo )
	ROM_REGION(0x100000, REGION_USER1, 0 ) /* ARM 32 bit code */
	ROM_LOAD16_BYTE( "a.u28",	0x000000, 0x80000, CRC(c93edbd3) SHA1(9c703cfef49b59ccd5d68bab9bd59344bd18d67e) )
	ROM_LOAD16_BYTE( "b.u29",	0x000001, 0x80000, CRC(39ecb9e4) SHA1(9ebd3962d8014b97c68c364729248ed22f9298a4) )

	ROM_REGION(0x1000000, REGION_USER2, 0 ) /* flash roms */
	ROM_LOAD16_BYTE( "du5",		0x000000, 0x400000, CRC(b32bd453) SHA1(6d5694bfcc67102256f857932b83b38f62ca2010) )
	ROM_LOAD16_BYTE( "du6",		0x000001, 0x400000, CRC(00559591) SHA1(543aefddc02f6a521d3bd5e6e3d8e42127ff9baa) )

	ROM_LOAD16_BYTE( "du3",		0x800000, 0x400000, CRC(d1e8afb2) SHA1(598dfcbba14435a1d0571dcefe0ec62fec657fca) )
	ROM_LOAD16_BYTE( "du2",		0x800001, 0x400000, CRC(56998515) SHA1(9b71a44f56a545ff0c1170775c839d21bd01f545) )

	ROM_REGION(0x80, REGION_USER3, 0 ) /* eeprom */
	ROM_LOAD( "24c01a.u36",		0x00, 0x80, CRC(b4f4849b) SHA1(f8f17dc94b2a305048693cfb78d14be57310ce56) )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* qdsp code */
	ROM_LOAD( "e.u14",		0x000000, 0x10000, CRC(49976f7b) SHA1(eba5b97b81736f3c184ae0c19f1b10c5ae250d51) )

	ROM_REGION(0x1000000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD( "c.u12", 0x000000, 0x80000, CRC(d24b5e56) SHA1(d89983cf4b0a6e0e4137f3799bdbcfd72c7bebe4) )
	ROM_LOAD( "d.u11", 0x080000, 0x80000, CRC(c0fdd82a) SHA1(a633045e0f5c144b4e24e04fb9446522fdb222f4) )
	ROM_LOAD( "1008s-1.u16", 0x100000, 0x100000, CRC(9aef9545) SHA1(f23ef72c3e3667923768dfdd0c5b4951b23dcbcf) )

ROM_END

ROM_START( ppcar )
	ROM_REGION(0x100000, REGION_USER1, 0 ) /* ARM 32 bit code */
	ROM_LOAD16_BYTE( "fk0.u24",	0x000000, 0x80000, CRC(1940a483) SHA1(9456361fd25bf037b53bd2d04764a33b299d96dd) )
	ROM_LOAD16_BYTE( "fk1.u25",	0x000001, 0x80000, CRC(75ad8679) SHA1(392288e56350e3cc49aaca82edf26f2a9e346f21) )

	ROM_REGION(0x1000000, REGION_USER2, 0 ) /* flash roms */
	ROM_LOAD16_BYTE( "du5",		0x000000, 0x400000, CRC(d4b7374a) SHA1(54c93a4235f495ba3794aea511b19db821a8acb1) )
	ROM_LOAD16_BYTE( "du6",		0x000001, 0x400000, CRC(e95a3a62) SHA1(2b1c889d208a749e3d7e4c75588c9c1f979e88d9) )

	ROM_LOAD16_BYTE( "du3",		0x800000, 0x400000, CRC(73882474) SHA1(191b64e662542b5322160c99af8e00079420d473) )
	ROM_LOAD16_BYTE( "du2",		0x800001, 0x400000, CRC(9250124a) SHA1(650f4b89c92fe4fb63fc89d4e08c4c4c611bebbc) )

	ROM_REGION(0x10000, REGION_USER4, ROMREGION_ERASE00 ) /* qdsp code */
	/* none */

	ROM_REGION(0x100000, REGION_USER5, 0 ) /* HWASS 1008S-1  qdsp samples */
	ROM_LOAD( "nasn9289.u9",	0x000000, 0x100000, CRC(9aef9545) SHA1(f23ef72c3e3667923768dfdd0c5b4951b23dcbcf) )

	ROM_REGION(0x100000, REGION_USER6, ROMREGION_ERASE00 ) /* samples - same internal structure as qdsp samples  */
	/* none */
ROM_END


static DRIVER_INIT(ssfindo)
{
	flashType=0;
	PS7500timer0 = timer_alloc(PS7500_Timer0_callback);
	PS7500timer1 = timer_alloc(PS7500_Timer1_callback);
	sound_timer = timer_alloc(sound_tick);
}

static DRIVER_INIT(ppcar)
{
	flashType=1;
	PS7500timer0 = timer_alloc(PS7500_Timer0_callback);
	PS7500timer1 = timer_alloc(PS7500_Timer1_callback);
	sound_timer = timer_alloc(sound_tick);
}

GAME( 1999, ssfindo, 0,        ssfindo,  ssfindo,  ssfindo, ROT0, "Icarus", "See See Find Out", GAME_IMPERFECT_SOUND )
GAME( 1999, ppcar,   0,        ppcar,    ppcar,    ppcar, ROT0, "Icarus", "Pang Pang Car", GAME_IMPERFECT_SOUND )
