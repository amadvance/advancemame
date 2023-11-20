/*
   ICS 2115 sound synthesizer.

   By O. Galibert, with a lot of help from the nebula
   ics emulation by Elsemi.
*/

#include <math.h>
#include "sndintrf.h"
#include "streams.h"
#include "cpuintrf.h"
#include "ics2115.h"

#define ICS2115LOGERROR 0

// a:401ae90.000 l:1c23c.0 e:1e1d8.0  09  tone
// a:4023c40.000 l:25d60.0 e:266cb.0  08  tone
// a:4028fc0.000 l:28fc0.0 e:2b6ef.0  01  violon, noisy
// a:4034020.000 l:34560.0 e:36001.0  19  percussion
// a:4034218.1e8 l:34560.0 e:36001.0  19  percussion
// a:4037f10.000 l:37f3e.0 e:39cb7.0  08  tone
// a:4044f10.000 l:463ea.0 e:46476.0  09  tone
// a:40490d0.000 l:760e9.0 e:910d8.0  19  percussion moche
// a:4051bd0.000 l:51bd0.0 e:528df.0  01  percussion
// a:40621f0.000 l:621f0.0 e:62aef.0  01  percussion faible
// a:4063430.000 l:63c78.0 e:63d25.0  08  tone
// a:40668a0.000 l:668a0.0 e:670ec.0  01  percussion
// a:4067940.000 l:67940.0 e:68140.0  01  percussion
// a:40aff36.000 l:aff36.0 e:b194d.0  20  Selection menu
// a:40b5f26.000 l:b5f26.0 e:b63a5.0  20  Move up/down
// a:4102772.000 l:02772.0 e:03a31.0  20  Voice test (fucked?)

// conf:
//   10b6: 00
//   11ee: 20
//   1867: a0
//   188b: 00
//   20ba: 01 08 09
//   2299: 01 09 19

#define FLAG_OSCCONF_DISABLE	(1 << 1)
#define FLAG_OSCCONF_NOENVELOPE	(FLAG_OSCCONF_DISABLE)
#define FLAG_OSCCONF_EIGHTBIT	(1 << 2)
#define FLAG_OSCCONF_LOOP	(1 << 3)
#define FLAG_OSCCONF_LOOP_BIDIR	(1 << 4)
#define FLAG_OSCCONF_IRQ	(1 << 5)
#define FLAG_OSCCONF_INVERT	(1 << 6)
/* Possibly enables IRQ for completion */

#define FLAG_VOLCTL_DONE	(1 << 0)
/* Unsure, but envelope can be disabled with this, so I assume it's a disable bit... */
#define FLAG_VOLCTL_DISABLE (1 << 1)
#define FLAG_VOLCTL_NOENVELOPE	(FLAG_VOLCTL_DONE | FLAG_VOLCTL_DISABLE)
#define FLAG_VOLCTL_LOOP	(1 << 3)
#define FLAG_VOLCTL_LOOP_BIDIR	(1 << 4)
#define FLAG_VOLCTL_IRQ		(1 << 5)
#define FLAG_VOLCTL_INVERT	(1 << 6)

#define	FLAG_STATE_ON		(1 << 0)
#define	FLAG_STATE_WAVEIRQ	(1 << 1)
#define	FLAG_STATE_VOLIRQ	(1 << 2)
#define	FLAG_STATE_IRQ		(FLAG_STATE_VOLIRQ | FLAG_STATE_WAVEIRQ)

enum { V_ON = 1, V_DONE = 2 };

static UINT32 ramp[32];

struct ics2115{
	const struct ics2115_interface *intf;
	int index;
	UINT8 *rom;
	INT16 *ulaw;
	UINT16 *voltbl;

	struct {
		INT16 left;
		UINT16 add;
		UINT16 fc, addrh, addrl, strth, endh, volacc, incr, tout;
		UINT8 strtl, endl, saddr, pan, conf, ctl;
		UINT8 vstart, vend, vctl;
		UINT8 state;
	} voice[32];

	struct {
		UINT8 scale, preset;
		mame_timer *timer;
		double period;
	} timer[2];

	UINT8 reg, osc;
	UINT8 irq_en, irq_pend;
	int irq_on;
	sound_stream * stream;
};

static int caller_get_pc(void)
{
	int pc = activecpu_get_pc();
#if 0
	if(pc == 0x14b || pc == 0x26e || pc == 0x284 || pc == 0x28d ||
	   pc == 0x290 || pc == 0x299 || pc == 0x2a2 || pc == 0x2b3) {
		int sp = z80_get_reg(Z80_SP);
		pc = cpu_readmem16(sp)|(cpu_readmem16((UINT16)(sp+1)) << 8);
	}
#endif

	return pc;
}

static void recalc_irq(struct ics2115 *chip)
{
	int i;
    int irq = 0;
	if(chip->irq_en & chip->irq_pend)
		irq = 1;
	for(i=0; !irq && i<32; i++)
		if(chip->voice[i].state & V_DONE)
			irq = 1;
	if(irq != chip->irq_on) {
		chip->irq_on = irq;
		if(chip->intf->irq_cb)
			chip->intf->irq_cb(irq ? ASSERT_LINE : CLEAR_LINE);
	}
}


static void update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	struct ics2115 *chip = param;
	int osc, i;
	int rec_irq = 0;

	memset(buffer[0], 0, length*sizeof(*buffer[0]));
	memset(buffer[1], 0, length*sizeof(*buffer[0]));

	for(osc = 0; osc < 32; osc++)
		if(chip->voice[osc].state & V_ON) {
			UINT32 adr = (chip->voice[osc].addrh << 16) | chip->voice[osc].addrl;
			UINT32 end = (chip->voice[osc].endh << 16) | (chip->voice[osc].endl << 8);
			UINT32 loop = (chip->voice[osc].strth << 16) | (chip->voice[osc].strtl << 8);
			UINT32 badr = (chip->voice[osc].saddr << 20) & 0xffffff;
			UINT32 delta = (chip->voice[osc].fc << 2)*(33075.0/44100.0);
			UINT8 conf = chip->voice[osc].conf;
			INT32 vol = chip->voice[osc].volacc;
			UINT32 volacc = (chip->voice[osc].volacc) >> 4 & 0xffff;
			//int skip = 0;
			chip->voice[osc].add = (double)((chip->voice[osc].incr & 0x3F) << 4) / (double)(1 << (3 * ((chip->voice[osc].incr >> 6) & 3)));
			
			if ((chip->voice[osc].tout > 0) && (osc < 8))chip->voice[osc].tout--;
			if (volacc == 0xe68) volacc = 0xe20;
			if (volacc == 0xee0) volacc = 0xe20;
			if (volacc == 0xfcc) volacc = 0xf40;
			if (volacc == 0xe18) volacc = 0xeb0;
			vol = chip->voltbl[volacc];
			if (!(chip->voice[osc].ctl & 0x8)) ramp[osc]  = 0x0;
			if (chip->voice[osc].ctl & 0x8) ramp[osc]  += 0x150;
			
			if ((ramp[osc] + 0x50) > vol) vol = 0x0;
			else vol -= ramp[osc];
			
			//printf("ramp:%x vol:%x\n",volacc,vol);

			if (ICS2115LOGERROR) logerror("ICS2115: KEYRUN %02d adr=%08x end=%08x delta=%08x\n",
					 osc, adr, end, delta);

			for(i=0; i<length; i++) {
				INT32 v = chip->rom[badr|(adr >> 12)];
				if(conf & 1)
					v = chip->ulaw[v];
				else
					v = ((INT8)v) << 6;
if(1)
{
				v = (v * vol) >> (18);
				buffer[0][i] += v;
				buffer[1][i] += v;
				adr += delta;
				if(adr >= end) 
				{
					if (ICS2115LOGERROR) logerror("ICS2115: KEYDONE %2d\n", osc);
					adr -= (end-loop);
					if((!(conf & 0x8)) || chip->voice[osc].tout == 0) {
						chip->voice[osc].state &= ~V_ON;
						chip->voice[osc].state |= V_DONE;
						
					}
					
					rec_irq = 1;
					break;
				}
			}
			}
			chip->voice[osc].addrh = adr >> 16;
			chip->voice[osc].addrl = adr;
		}
	if(rec_irq)
		recalc_irq(chip);
}

static void keyon(struct ics2115 *chip, int osc)
{
	if (ICS2115LOGERROR) logerror("ICS2115: KEYON %2d conf:%02x vctl:%02x a:%07x.%03x l:%05x.%x e:%05x.%x v:%03x f:%d\n",
			 osc,
			 chip->voice[chip->osc].conf,
			 chip->voice[chip->osc].vctl,
			 (chip->voice[osc].saddr << 20)|(chip->voice[osc].addrh << 4)|(chip->voice[osc].addrl >> 12),
			 (chip->voice[osc].addrl >> 3) & 0x1ff,
			 (chip->voice[osc].strth << 4)|(chip->voice[osc].strtl >> 4),
			 chip->voice[osc].strtl & 0xf,
			 (chip->voice[osc].endh << 4)|(chip->voice[osc].endl >> 4),
			 chip->voice[osc].endl & 0xf,
			 chip->voice[osc].volacc>>4,
			 (chip->voice[chip->osc].fc*33075+512)/1024);
	chip->voice[osc].state |= V_ON;
}


static void timer_cb_0(void *param)
{
	struct ics2115 *chip = param;
	chip->irq_pend |= 1<<0;
	recalc_irq(chip);
}

static void timer_cb_1(void *param)
{
	struct ics2115 *chip = param;
	chip->irq_pend |= 1<<1;
	recalc_irq(chip);
}

/* Arcadez keep original timers rather the adjusted ones from shmupmame otherwise the games take a big hit in performance */

static void recalc_timer(struct ics2115 *chip, int timer)
{
	double period = chip->timer[timer].scale*chip->timer[timer].preset / 33868800.0;
	if(period)
		period = 1/62.8206;
	if(period)
	{
		if (ICS2115LOGERROR) logerror("ICS2115: timer %d freq=%gHz\n", timer, 1/period);
	}
	else
	{
		if (ICS2115LOGERROR) logerror("ICS2115: timer %d off\n", timer);
	}

	if(chip->timer[timer].period != period) {
		chip->timer[timer].period = period;
		if(period)
			timer_adjust_ptr(chip->timer[timer].timer, TIME_IN_SEC(period), TIME_IN_SEC(period));
		else
			timer_adjust_ptr(chip->timer[timer].timer, TIME_NEVER, 0);
	}
}


static void ics2115_reg_w(struct ics2115 *chip, UINT8 reg, UINT8 data, int msb)
{
    chip->voice[chip->osc].tout = 40;
	switch(reg) {
	case 0x00: // [osc] Oscillator Configuration
		if(msb) {
			chip->voice[chip->osc].conf = data;
			if (ICS2115LOGERROR) logerror("ICS2115: %2d: conf = %02x (%04x)\n", chip->osc,
					 chip->voice[chip->osc].conf, caller_get_pc());
		}
		break;

	case 0x01: // [osc] Wavesample frequency
		// freq = fc*33075/1024 in 32 voices mode, fc*44100/1024 in 24 voices mode
		if(msb)
			chip->voice[chip->osc].fc = (chip->voice[chip->osc].fc & 0xff)|(data << 8);
		else
			chip->voice[chip->osc].fc = (chip->voice[chip->osc].fc & 0xff00)|data;
		if (ICS2115LOGERROR) logerror("ICS2115: %2d: fc = %04x (%dHz) (%04x)\n", chip->osc,
				 chip->voice[chip->osc].fc, chip->voice[chip->osc].fc*33075/1024, caller_get_pc());
		break;

	case 0x02: // [osc] Wavesample loop start address 19-4
		if(msb)
			chip->voice[chip->osc].strth = (chip->voice[chip->osc].strth & 0xff)|(data << 8);
		else
			chip->voice[chip->osc].strth = (chip->voice[chip->osc].strth & 0xff00)|data;
		if (ICS2115LOGERROR) logerror("ICS2115: %2d: strth = %04x (%04x)\n", chip->osc,
				 chip->voice[chip->osc].strth, caller_get_pc());
		break;

	case 0x03: // [osc] Wavesample loop start address 3-0.3-0
		if(msb) {
			chip->voice[chip->osc].strtl = data;
			if (ICS2115LOGERROR) logerror("ICS2115: %2d: strtl = %02x (%04x)\n", chip->osc,
					 chip->voice[chip->osc].strtl, caller_get_pc());
		}
		break;

	case 0x04: // [osc] Wavesample loop end address 19-4
		if(msb)
			chip->voice[chip->osc].endh = (chip->voice[chip->osc].endh & 0xff)|(data << 8);
		else
			chip->voice[chip->osc].endh = (chip->voice[chip->osc].endh & 0xff00)|data;
		if (ICS2115LOGERROR) logerror("ICS2115: %2d: endh = %04x (%04x)\n", chip->osc,
				 chip->voice[chip->osc].endh, caller_get_pc());
		break;

	case 0x05: // [osc] Wavesample loop end address 3-0.3-0
		if(msb) {
			chip->voice[chip->osc].endl = data;
			if (ICS2115LOGERROR) logerror("ICS2115: %2d: endl = %02x (%04x)\n", chip->osc,
					 chip->voice[chip->osc].endl, caller_get_pc());
		}
		break;
		
	case 0x06: // [osc] Volume Increment
		if(msb)
				chip->voice[chip->osc].incr = (chip->voice[chip->osc].incr & ~0xFF00) | (data << 8);
		else /* This is unused? */
				chip->voice[chip->osc].incr = (chip->voice[chip->osc].incr & ~0x00FF) | (data << 0);

		break;	
		

	case 0x07: // [osc] Volume Start
		if(msb) {
			chip->voice[chip->osc].vstart = data;
			if (ICS2115LOGERROR) logerror("ICS2115: %2d: vstart = %02x (%04x)\n", chip->osc,
					 chip->voice[chip->osc].vstart, caller_get_pc());
		}
		break;

	case 0x08: // [osc] Volume End
		if(msb) {
			chip->voice[chip->osc].vend = data;
			if (ICS2115LOGERROR) logerror("ICS2115: %2d: vend = %02x (%04x)\n", chip->osc,
					 chip->voice[chip->osc].vend, caller_get_pc());
		}
		break;

	case 0x09: // [osc] Volume accumulator
		if(msb)
			chip->voice[chip->osc].volacc = (chip->voice[chip->osc].volacc & 0xff)|(data << 8);
		else
			chip->voice[chip->osc].volacc = (chip->voice[chip->osc].volacc & 0xff00)|data;
		if (ICS2115LOGERROR) logerror("ICS2115: %2d: volacc = %04x (%04x)\n", chip->osc,
				 chip->voice[chip->osc].volacc, caller_get_pc());
		break;

	case 0x0a: // [osc] Wavesample address 19-4
		if(msb)
			chip->voice[chip->osc].addrh = (chip->voice[chip->osc].addrh & 0xff)|(data << 8);
		else
			chip->voice[chip->osc].addrh = (chip->voice[chip->osc].addrh & 0xff00)|data;
		if (ICS2115LOGERROR) logerror("ICS2115: %2d: addrh = %04x (%04x)\n", chip->osc,
				 chip->voice[chip->osc].addrh, caller_get_pc());
		break;

	case 0x0b: // [osc] Wavesample address 3-0.8-0
		if(msb)
			chip->voice[chip->osc].addrl = (chip->voice[chip->osc].addrl & 0xff)|(data << 8);
		else
			chip->voice[chip->osc].addrl = (chip->voice[chip->osc].addrl & 0xff00)|data;
		if (ICS2115LOGERROR) logerror("ICS2115: %2d: addrl = %04x (%04x)\n", chip->osc,
				 chip->voice[chip->osc].addrl, caller_get_pc());
		break;


	case 0x0c: // [osc] Pan
		if(msb) {
			chip->voice[chip->osc].pan = data;
			if (ICS2115LOGERROR) logerror("ICS2115: %2d: pan = %02x (%04x)\n", chip->osc,
					 chip->voice[chip->osc].pan, caller_get_pc());
		}
		break;

	case 0x0d: // [osc] Volume Enveloppe Control
		if(msb) {
			chip->voice[chip->osc].vctl = data;
			if (ICS2115LOGERROR) logerror("ICS2115: %2d: vctl = %02x (%04x)\n", chip->osc,
					 chip->voice[chip->osc].vctl, caller_get_pc());
		}
		break;

	case 0x10: // [osc] Oscillator Control
		if(msb) {
			chip->voice[chip->osc].ctl = data;
			if (ICS2115LOGERROR) logerror("ICS2115: %2d: ctl = %02x (%04x)\n", chip->osc,
					 chip->voice[chip->osc].ctl, caller_get_pc());
			if(data == 0)
				keyon(chip, chip->osc);
		}
		break;

	case 0x11: // [osc] Wavesample static address 27-20
		if(msb) {
			chip->voice[chip->osc].saddr = data;
			if (ICS2115LOGERROR) logerror("ICS2115: %2d: saddr = %02x (%04x)\n", chip->osc,
					 chip->voice[chip->osc].saddr, caller_get_pc());
		}
		break;

	case 0x40: // Timer 1 Preset
		if(!msb) {
			chip->timer[0].preset = data;
			if (ICS2115LOGERROR) logerror("ICS2115: t1preset = %d (%04x)\n", chip->timer[0].preset, caller_get_pc());
			recalc_timer(chip, 0);
		}
		break;

	case 0x41: // Timer 2 Preset
		if(!msb) {
			chip->timer[1].preset = data;
			if (ICS2115LOGERROR) logerror("ICS2115: t2preset = %d (%04x)\n", chip->timer[1].preset, caller_get_pc());
			recalc_timer(chip, 1);
		}
		break;

	case 0x42: // Timer 1 Prescaler
		if(!msb) {
			chip->timer[0].scale = data;
			if (ICS2115LOGERROR) logerror("ICS2115: t1scale = %d (%04x)\n", chip->timer[0].scale, caller_get_pc());
			recalc_timer(chip, 0);
		}
		break;

	case 0x43: // Timer 2 Prescaler
		if(!msb) {
			chip->timer[1].scale = data;
			if (ICS2115LOGERROR) logerror("ICS2115: t2scale = %d (%04x)\n", chip->timer[1].scale, caller_get_pc());
			recalc_timer(chip, 1);
		}
		break;

	case 0x4a: // IRQ Enable
		if(!msb) {
			chip->irq_en = data;
			if (ICS2115LOGERROR) logerror("ICS2115: irq_en = %02x (%04x)\n", chip->irq_en, caller_get_pc());
			recalc_irq(chip);
		}
		break;

	case 0x4f: // Oscillator Address being Programmed
		if(!msb) {
			chip->osc = data & 31;
			if (ICS2115LOGERROR) logerror("ICS2115: oscnumber = %d (%04x)\n", chip->osc, caller_get_pc());
		}
		break;

	default:
		if (ICS2115LOGERROR) logerror("ICS2115: write %02x, %02x:%d (%04x)\n", reg, data, msb, caller_get_pc());
	}
}

static UINT16 ics2115_reg_r(struct ics2115 *chip, UINT8 reg)
{
	switch(reg) {
			case 0x06: // [osc] Volume Increment
			return chip->voice[chip->osc].incr;

			break;
	
	
	case 0x0d: // [osc] Volume Enveloppe Control
		if (ICS2115LOGERROR) logerror("ICS2115: %2d: read vctl (%04x)\n", chip->osc, caller_get_pc());
		//      res = chip->voice[chip->osc].vctl << 8;
		// may expect |8 on voice irq with &40 == 0
		// may expect |8 on reg 0 on voice irq with &80 == 0
		return 0x100;

	case 0x0f:{// [osc] Interrupt source/oscillator
		int osc;
		UINT8 res = 0xff;
		for(osc = 0; osc < 32; osc++)
			if(chip->voice[osc].state & V_DONE) {
				chip->voice[osc].state &= ~V_DONE;
				if (ICS2115LOGERROR) logerror("ICS2115: KEYOFF %2d\n", osc);
				recalc_irq(chip);
				res = 0x40 | osc; // 0x40 ? 0x80 ?
				break;
			}
		if (ICS2115LOGERROR) logerror("ICS2115: read irqv %02x (%04x)\n", res, caller_get_pc());
		return res << 8;
	}

	case 0x40: // Timer 0 clear irq
		//      if (ICS2115LOGERROR) logerror("ICS2115: clear timer 0 (%04x)\n", caller_get_pc());
		chip->irq_pend &= ~(1<<0);
		recalc_irq(chip);
		return chip->timer[0].preset;

	case 0x41: // Timer 1 clear irq
		if (ICS2115LOGERROR) logerror("ICS2115: clear timer 1 (%04x)\n", caller_get_pc());
		chip->irq_pend &= ~(1<<1);
		recalc_irq(chip);
		return chip->timer[1].preset;

	case 0x43: // Timer status
		//      if (ICS2115LOGERROR) logerror("ICS2115: read timer status %02x (%04x)\n", chip->irq_pend & 3, caller_get_pc());
		return chip->irq_pend & 3;

	case 0x4a: // IRQ Pending
		if (ICS2115LOGERROR) logerror("ICS2115: read irq_pend %02x (%04x)\n", chip->irq_pend, caller_get_pc());
		return chip->irq_pend;

	case 0x4b: // Address of Interrupting Oscillator
		if (ICS2115LOGERROR) logerror("ICS2115: %2d: read intoscaddr (%04x)\n", chip->osc, caller_get_pc());
		return 0x80;

	case 0x4c: // Chip revision
		if (ICS2115LOGERROR) logerror("ICS2115: read revision (%04x)\n", caller_get_pc());
		return 0x01;

	default:
		if (ICS2115LOGERROR) logerror("ICS2115: read %02x unmapped (%04x)\n", reg, caller_get_pc());
		return 0;
	}
}


static void *ics2115_start(int sndindex, int clock, const void *config)
{
	struct ics2115 *chip;
	int i;

	chip = auto_malloc(sizeof(*chip));
	memset(chip, 0, sizeof(*chip));

	chip->intf = config;
	chip->index = sndindex;
	chip->rom = memory_region(chip->intf->region);
	chip->timer[0].timer = timer_alloc_ptr(timer_cb_0, chip);
	chip->timer[1].timer = timer_alloc_ptr(timer_cb_1, chip);
	chip->ulaw = auto_malloc(256*sizeof(INT16));
	chip->stream = stream_create(0, 2, Machine->sample_rate, chip, update);
    chip->voltbl = auto_malloc(8192);

    for (i = 0; i < 0x1000; i++) {
		chip->voltbl[i] = floor(pow(2.0,(((double)i/256 - 16) + 14.7)*1.06));

    }

	if(!chip->timer[0].timer || !chip->timer[1].timer)
		return NULL;

	for(i=0; i<256; i++) {
		UINT8 c = ~i;
		int v;
		v = ((c & 15) << 1) + 33;
		v <<= ((c & 0x70) >> 4);
		if(c & 0x80)
			v = 33-v;
		else
			v = v-33;
		chip->ulaw[i] = v;
	}

	return chip;
}

READ8_HANDLER( ics2115_r )
{
	struct ics2115 *chip = sndti_token(SOUND_ICS2115, 0);
	switch(offset) {
	case 0: {
		UINT8 res = 0;
		if(chip->irq_on) {
			int i;
			res |= 0x80;
			if(chip->irq_en & chip->irq_pend & 3)
				res |= 1; // Timer irq
			for(i=0; i<32; i++)
				if(chip->voice[i].state & V_DONE) {
					res |= 2;
					break;
				}
		}
		//      if (ICS2115LOGERROR) logerror("ICS2115: read status %02x (%04x)\n", res, caller_get_pc());

		return res;
	}
	case 1:
	    return chip->reg;
	case 2:
		return ics2115_reg_r(chip, chip->reg);
	case 3:
	default:
		return ics2115_reg_r(chip, chip->reg) >> 8;
	}
}

WRITE8_HANDLER( ics2115_w )
{
	struct ics2115 *chip = sndti_token(SOUND_ICS2115, 0);
	switch(offset) {
	case 1:
		chip->reg = data;
		break;
	case 2:
		ics2115_reg_w(chip, chip->reg, data, 0);
		break;
	case 3:
		ics2115_reg_w(chip, chip->reg, data, 1);
		break;
	}
	//  if (ICS2115LOGERROR) logerror("ICS2115: wi %d, %02x (%04x)\n", offset, data, caller_get_pc());
}

void ics2115_reset(void *_chip)
{
	struct ics2115 *chip = _chip;
	chip->irq_en = 0;
	chip->irq_pend = 0;
	memset(chip->voice, 0, sizeof(chip->voice));
	timer_adjust_ptr(chip->timer[0].timer, TIME_NEVER, 0);
	timer_adjust_ptr(chip->timer[1].timer, TIME_NEVER, 0);
	chip->timer[0].period = 0;
	chip->timer[1].period = 0;
	recalc_irq(chip);
}




/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void ics2115_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void ics2115_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = ics2115_set_info;		break;
		case SNDINFO_PTR_START:							info->start = ics2115_start;			break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							info->reset = ics2115_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "ICS2115";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "ICS";						break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

