/**********************************************************************

    MOS 6526/8520 CIA interface and emulation

    This function emulates all the functionality of up to 2 MOS6526 or
    MOS8520 complex interface adapters.

**********************************************************************/

#include "mame.h"
#include "timer.h"
#include "6526cia.h"



/*************************************
 *
 *  Constants
 *
 *************************************/

/* CIA registers */
#define CIA_PRA			0
#define CIA_PRB			1
#define CIA_DDRA		2
#define CIA_DDRB		3
#define CIA_TALO		4
#define CIA_TAHI		5
#define CIA_TBLO		6
#define CIA_TBHI		7
#define CIA_TOD0		8		/* 6526: 1/10 seconds   8520: bits  0- 7 */
#define CIA_TOD1		9		/* 6526: seconds        8520: bits  8-15 */
#define CIA_TOD2		10		/* 6526: minutes        8520: bits 16-23 */
#define CIA_TOD3		11		/* 6526: hours          8520: N/A */
#define CIA_SDR			12
#define CIA_ICR			13
#define CIA_CRA			14
#define CIA_CRB			15



/*************************************
 *
 *  Type definitions
 *
 *************************************/

typedef struct _cia_timer cia_timer;
typedef struct _cia_port cia_port;
typedef struct _cia_state cia_state;

struct _cia_timer
{
	UINT16		latch;
	UINT16		count;
	UINT8		mode;
	UINT8		started;
	UINT8		irq;
	mame_timer *timer;
	cia_state *	cia;
};

struct _cia_port
{
	UINT8		ddr;
	UINT8		latch;
	UINT8		in;
	UINT8		out;
	UINT8		(*read)(void);
	void		(*write)(UINT8);
};

struct _cia_state
{
	int				active;
	cia_type_t		type;
	void			(*irq_func)(int state);
	double			clock;

	cia_port		port[2];
	cia_timer		timer[2];

	/* Time Of the Day clock (TOD) */
	UINT32			tod;
	UINT32			tod_latch;
	UINT8			tod_latched;
	UINT8			tod_running;
	UINT32			alarm;

	/* Interrupts */
	UINT8			icr;
	UINT8			ics;
	UINT8			irq;
};



/*************************************
 *
 *  Globals
 *
 *************************************/

static cia_state cia_array[2];



/*************************************
 *
 *  Prototypes
 *
 *************************************/

static void cia_timer_proc(void *param);



/*************************************
 *
 *  Prototypes
 *
 *************************************/


/***************************************************************************

    Setup and reset

***************************************************************************/

static void cia_exit(void)
{
	memset(cia_array, 0, sizeof(*cia_array));
}



void cia_config(int which, const cia6526_interface *intf)
{
	int t, p;
	cia_state *cia = &cia_array[which];

	/* sanity checks */
	assert_always(mame_get_phase() == MAME_PHASE_INIT, "Can only call cia_config at init time!");
	assert_always((which >= 0) && (which < (sizeof(cia_array) / sizeof(cia_array[0]))),
		"cia_config called on an invalid CIA!");

	/* clear out CIA structure, and copy the interface */
	memset(cia, 0, sizeof(*cia));
	cia->active = TRUE;
	cia->type = intf->type;
	cia->clock = (intf->clock != 0.0) ? intf->clock : TIME_IN_CYCLES(1, 0);
	cia->irq_func = intf->irq_func;

	/* setup ports */
	for (p = 0; p < (sizeof(cia->port) / sizeof(cia->port[0])); p++)
	{
		cia->port[p].read = intf->port[p].read;
		cia->port[p].write = intf->port[p].write;
	}

	/* setup timers */
	for (t = 0; t < (sizeof(cia->timer) / sizeof(cia->timer[0])); t++)
	{
		cia_timer *timer = &cia->timer[t];
		timer->timer = timer_alloc_ptr(cia_timer_proc, timer);
		timer->cia = cia;
		timer->irq = 0x01 << t;
	}

	/* setup TOD timer, if appropriate */
	if (intf->tod_clock)
		timer_pulse(TIME_IN_HZ(intf->tod_clock), which, cia_clock_tod);

	/* special case; for the first CIA, set up an exit handler to clear things out */
	if (which == 0)
		add_exit_callback(cia_exit);
}



void cia_reset(void)
{
	int i, t;

	/* loop over and set up initial values */
	for (i = 0; i < (sizeof(cia_array) / sizeof(cia_array[0])); i++)
	{
		cia_state *cia = &cia_array[i];

		if (cia->active)
		{
			/* clear things out */
			cia->port[0].latch = 0x00;
			cia->port[0].in = 0x00;
			cia->port[0].out = 0x00;
			cia->port[1].latch = 0x00;
			cia->port[1].in = 0x00;
			cia->port[1].out = 0x00;
			cia->tod = 0;
			cia->tod_latch = 0;
			cia->alarm = 0;
			cia->icr = 0x00;
			cia->ics = 0x00;
			cia->irq = 0;

			/* initialize data direction registers */
			cia->port[0].ddr = (i == 0) ? 0x03 : 0xff;
			cia->port[1].ddr = (i == 0) ? 0x00 : 0xff;

			/* TOD running by default */
			cia->tod_running = TRUE;

			/* initialize timers */
			for (t = 0; t < 2; t++)
			{
				cia_timer *timer = &cia->timer[t];

				timer->latch = 0xffff;
				timer->count = 0x0000;
				timer->mode = 0x00;
				timer->started = 0x00;
			}
		}
	}
}


/***************************************************************************

    CIA runtime

***************************************************************************/

static void cia_update_interrupts(cia_state *cia)
{
	UINT8 new_irq;

	/* always update the high bit of ICS */
	if (cia->ics & 0x7f)
		cia->ics |= 0x80;
	else
		cia->ics &= ~0x80;

	/* based on what is enabled, set/clear the IRQ via the custom chip */
	new_irq = (cia->ics & cia->icr) ? 1 : 0;
	if (cia->irq != new_irq)
	{
		cia->irq = new_irq;
		if (cia->irq_func)
			cia->irq_func(cia->irq);
	}
}


INLINE void cia_timer_start(cia_timer *timer)
{
	if (!timer->started)
	{
		timer_adjust_ptr(timer->timer, (double)timer->count * timer->cia->clock, 0);
		timer->started = TRUE;
	}
}


INLINE void cia_timer_stop(cia_timer *timer)
{
	timer_reset(timer->timer, TIME_NEVER);
	timer->started = FALSE;
}


INLINE int cia_timer_count(cia_timer *timer)
{
	/* based on whether or not the timer is running, return the current count value */
	if (timer->started)
		return timer->count - (int)(timer_timeelapsed(timer->timer) / timer->cia->clock);
	else
		return timer->count;
}


static void cia_timer_proc(void *param)
{
	cia_timer *timer = param;

	/* clear the timer started flag */
	timer->started = FALSE;

	/* set the status and update interrupts */
	timer->cia->ics |= timer->irq;
	cia_update_interrupts(timer->cia);

	/* reload the timer */
	timer->count = timer->latch;

	/* if one-shot mode, turn it off; otherwise, reprime the timer */
	if (timer->mode & 0x08)
		timer->mode &= 0xfe;
	else
		cia_timer_start(timer);
}


static UINT8 bcd_increment(UINT8 value)
{
	value++;
	if ((value & 0x0f) >= 0x0a)
		value += 0x10 - 0x0a;
	return value;
}


static UINT32 cia6526_increment(UINT32 value)
{
	UINT8 subsecond	= (UINT8) (value >>  0);
	UINT8 second	= (UINT8) (value >>  8);
	UINT8 minute	= (UINT8) (value >> 16);
	UINT8 hour		= (UINT8) (value >> 24);

	subsecond = bcd_increment(subsecond);
	if (subsecond >= 0x10)
	{
		subsecond = 0x00;
		second = bcd_increment(second);
		if (second >= 0x60)
		{
			second = 0x00;
			minute = bcd_increment(minute);
			if (minute >= 0x60)
			{
				minute = 0x00;
				if (hour == 0x91)
					hour = 0x00;
				else if (hour == 0x89)
					hour = 0x90;
				else if (hour == 0x11)
					hour = 0x80;
				else if (hour == 0x09)
					hour = 0x10;
				else
					hour++;
			}
		}
	}
	return	(((UINT32) subsecond)	<<  0)
		|	(((UINT32) second)		<<  8)
		|	(((UINT32) minute)		<< 16)
		|	(((UINT32) hour)		<< 24);
}


/* Update TOD on CIA A */
void cia_clock_tod(int which)
{
	cia_state *cia;

	cia = &cia_array[which];

	if (cia->tod_running)
	{
		switch(cia->type)
		{
			case CIA6526:
				/* The 6526 split the value into hours, minutes, seconds and
                 * subseconds */
				cia->tod = cia6526_increment(cia->tod);
				break;

			case CIA8520:
				/* the 8520 has a straight 24-bit counter */
				cia->tod++;
				cia->tod &= 0xffffff;
				break;
		}

		if (cia->tod == cia->alarm)
		{
			cia->ics |= 0x04;
			cia_update_interrupts(cia);
		}
	}
}


void cia_issue_index(int which)
{
	cia_state *cia = &cia_array[which];
	cia->ics |= 0x10;
	cia_update_interrupts(cia);
}


UINT8 cia_read(int which, offs_t offset)
{
	cia_timer *timer;
	cia_state *cia;
	cia_port *port;
	UINT8 data = 0x00;

	cia = &cia_array[which];
	offset &= 0x0F;

	switch(offset)
	{
		/* port A/B data */
		case CIA_PRA:
		case CIA_PRB:
			port = &cia->port[offset & 1];
			data = port->read ? (*port->read)() : 0;
			data = (data & ~port->ddr) | (port->latch & port->ddr);
			port->in = data;
			break;

		/* port A/B direction */
		case CIA_DDRA:
		case CIA_DDRB:
			port = &cia->port[offset & 1];
			data = port->ddr;
			break;

		/* timer A/B low byte */
		case CIA_TALO:
		case CIA_TBLO:
			timer = &cia->timer[(offset >> 1) & 1];
			data = cia_timer_count(timer) >> 0;
			break;

		/* timer A/B high byte */
		case CIA_TAHI:
		case CIA_TBHI:
			timer = &cia->timer[(offset >> 1) & 1];
			data = cia_timer_count(timer) >> 8;
			break;

		/* TOD counter */
		case CIA_TOD0:
		case CIA_TOD1:
		case CIA_TOD2:
		case CIA_TOD3:
			if (offset == CIA_TOD2)
			{
				cia->tod_latch = cia->tod;
				cia->tod_latched = TRUE;
			}
			else if (offset == CIA_TOD0)
				cia->tod_latched = FALSE;

			if (cia->tod_latched)
				data = cia->tod_latch >> ((offset - CIA_TOD0) * 8);
			else
				data = cia->tod >> ((offset - CIA_TOD0) * 8);
			break;

		/* interrupt status/clear */
		case CIA_ICR:
			data = cia->ics;
			cia->ics = 0; /* clear on read */
			cia_update_interrupts(cia);
			break;

		/* timer A/B mode */
		case CIA_CRA:
		case CIA_CRB:
			timer = &cia->timer[offset & 1];
			data = timer->mode;
			break;
	}
	return data;
}



void cia_write(int which, offs_t offset, UINT8 data)
{
	cia_timer *timer;
	cia_state *cia;
	cia_port *port;
	int shift;

	cia = &cia_array[which];
	offset &= 0x0F;

	switch(offset)
	{
		/* port A/B data */
		case CIA_PRA:
		case CIA_PRB:
			port = &cia->port[offset & 1];
			port->latch = data;
			port->out = (data & port->ddr) | (port->in & ~port->ddr);
			if (port->write)
				(*port->write)(port->out);
			break;

		/* port A/B direction */
		case CIA_DDRA:
		case CIA_DDRB:
			port = &cia->port[offset & 1];
			port->ddr = data;
			break;

		/* timer A/B latch low */
		case CIA_TALO:
		case CIA_TBLO:
			timer = &cia->timer[offset & 1];
			timer->latch = (timer->latch & 0xff00) | (data << 0);
			break;

		/* timer A latch high */
		case CIA_TAHI:
		case CIA_TBHI:
			timer = &cia->timer[(offset >> 1) & 1];
			timer->latch = (timer->latch & 0x00ff) | (data << 8);

			/* if it's one shot, start the timer */
			if (timer->mode & 0x08)
			{
				timer->count = timer->latch;
				timer->mode |= 0x01;
				cia_timer_start(timer);
			}
			break;

		/* time of day latches */
		case CIA_TOD0:
		case CIA_TOD1:
		case CIA_TOD2:
			shift = 8 * ((offset - CIA_TOD0));

			/* alarm setting mode? */
			if (cia->timer[1].mode & 0x80)
				cia->alarm = (cia->alarm & ~(0xff << shift)) | (data << shift);

			/* counter setting mode */
			else
			{
				cia->tod = (cia->tod & ~(0xff << shift)) | (data << shift);

				/* only enable the TOD once the LSB is written */
				cia->tod_running = (shift == 0);
			}
			break;

		/* interrupt control register */
		case CIA_ICR:
			if (data & 0x80)
				cia->icr |= data & 0x7f;
			else
				cia->icr &= ~(data & 0x7f);
			cia_update_interrupts(cia);
			break;

		/* timer A/B modes */
		case CIA_CRA:
		case CIA_CRB:
			timer = &cia->timer[offset & 1];
			timer->mode = data & 0xef;

			/* force load? */
			if (data & 0x10)
			{
				timer->count = timer->latch;
				cia_timer_stop(timer);
			}

			/* enable/disable? */
			if (data & 0x01)
				cia_timer_start(timer);
			else
				cia_timer_stop(timer);
			break;
	}
}



UINT8 cia_get_output_a(int which)	{ return cia_array[which].port[0].out; }
UINT8 cia_get_output_b(int which)	{ return cia_array[which].port[1].out; }
int cia_get_irq(int which)			{ return cia_array[which].irq; }

READ8_HANDLER( cia_0_r )	{ return cia_read(0, offset); }
READ8_HANDLER( cia_1_r )	{ return cia_read(1, offset); }

WRITE8_HANDLER( cia_0_w )	{ cia_write(0, offset, data); }
WRITE8_HANDLER( cia_1_w )	{ cia_write(1, offset, data); }
