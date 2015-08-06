/**********************************************************************


    Motorola 6840 PTM interface and emulation

    This function is a simple emulation of up to 4 MC6840
    Programmable Timer Module

    Todo:
         everything and more

**********************************************************************/

#include "driver.h"
#include "timer.h"
#include "6840ptm.h"

#define PTMVERBOSE 1

#if PTMVERBOSE
#define PLOG(x)	logerror x
#else
#define PLOG(x)
#endif

#define PTM6840_CYCLES_TO_TIME(a) ((double)a*1000000L)

#define PTM_6840_CTRL1   0
#define PTM_6840_CTRL2   1
#define PTM_6840_MSBBUF1 2
#define PTM_6840_LSB1	 3
#define PTM_6840_MSBBUF2 4
#define PTM_6840_LSB2    5
#define PTM_6840_MSBBUF3 6
#define PTM_6840_LSB3    7


typedef struct _ptm6840 ptm6840;
struct _ptm6840
{
	const ptm6840_interface *intf;

	UINT8 control_reg[3],
			lsb_latch[3],
		    msb_latch[3],
			lsb_counter[3],
			msb_counter[3],

			output[3],		// output states
			input[3],		// input  gate states
			clock[3],		// clock  states

			status_reg,
			lsb_buffer,
			msb_buffer;

  // each PTM has 3 timers

	mame_timer	*timer1,
				*timer2,
				*timer3;

};


// local prototypes ///////////////////////////////////////////////////////

static void ptm6840_t1_timeout(int which);
static void ptm6840_t2_timeout(int which);
static void ptm6840_t3_timeout(int which);

// local vars /////////////////////////////////////////////////////////////

static ptm6840 ptm[PTM_6840_MAX];

#ifdef PTMVERBOSE
static const char *opmode[] =
{
	"000 continous mode",
	"001 freq comparison mode",
	"010 continous mode",
	"011 pulse width comparison mode",
	"100 single shot mode",
	"101 freq comparison mode",
	"110 single shot mode",
	"111 pulse width comparison mode"
};
#endif

///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_unconfig(void)
{
	int i;

	i = 0;
	while ( i < PTM_6840_MAX )
	{
		if ( ptm[i].timer1 );// mame_timer_remove( ptm[i].timer1 );
		timer_adjust(ptm[i].timer1, TIME_NEVER, 0, 0);
		ptm[i].timer1 = NULL;

		if ( ptm[i].timer2 );// mame_timer_remove( ptm[i].timer2 );
		timer_adjust(ptm[i].timer2, TIME_NEVER, 0, 0);
		ptm[i].timer2 = NULL;

		if ( ptm[i].timer3 );// mame_timer_remove( ptm[i].timer3 );
		timer_adjust(ptm[i].timer3, TIME_NEVER, 0, 0);
		ptm[i].timer3 = NULL;

		i++;
  	}
	memset (&ptm, 0, sizeof (ptm));
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_config(int which, const ptm6840_interface *intf)
{
	assert_always(mame_get_phase() == MAME_PHASE_INIT, "Can only call ptm6840_config at init time!");
	assert_always((which >= 0) && (which < PTM_6840_MAX), "ptm6840_config called on an invalid PTM!");
	assert_always(intf, "ptm6840_config called with an invalid interface!");
	ptm[which].intf = intf;

	ptm[which].timer1 = timer_alloc(ptm6840_t1_timeout);
	ptm[which].timer2 = timer_alloc(ptm6840_t2_timeout);
	ptm[which].timer3 = timer_alloc(ptm6840_t3_timeout);
	ptm6840_reset(which);
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_reset(int which)
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		ptm[which].control_reg[i] = 0x00;
		ptm[which].lsb_latch[i]   = 0xff;
		ptm[which].msb_latch[i]   = 0xff;
		ptm[which].lsb_counter[i] = ptm[which].lsb_latch[i];
		ptm[which].msb_counter[i] = ptm[which].msb_latch[i];
		ptm[which].output[i]      = 0;
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

int ptm6840_read(int which, int offset)
{
	int	val = 0;

	ptm6840 *currptr = ptm + which;

	switch ( offset )
	{
		case PTM_6840_CTRL1 ://0

		break;

		case PTM_6840_CTRL2 ://1

		if ( currptr->status_reg & 0x07 ) currptr->status_reg |=  0x80;
		else                              currptr->status_reg &= ~0x80;
		val = currptr->status_reg;
		break;

		case PTM_6840_MSBBUF1://2

		currptr->status_reg &= ~0x01;
		break;

		case PTM_6840_LSB1://3

		currptr->status_reg &= ~0x01;
		break;

		case PTM_6840_MSBBUF2://4

		currptr->status_reg &= ~0x02;
		break;

		case PTM_6840_LSB2://5

		currptr->status_reg &= ~0x02;
		break;

		case PTM_6840_MSBBUF3://6

		currptr->status_reg &= ~0x04;
		break;

		case PTM_6840_LSB3://7

		currptr->status_reg &= ~0x04;
		break;

	}
	return val;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
//                                                                       //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_write (int which, int offset, int data)
{
	ptm6840 *currptr = ptm + which;

	int  idx = 0;
	long time;
	if (offset < 2)
	{
		idx = (offset == 1) ? 1 : (currptr->control_reg[1] & 0x01) ? 0 : 2;

		logerror("MC6840 #%d : Control register %d selected\n",which,idx);

		currptr->control_reg[idx] = data;

/* bit 1 - 7  are the same for all timer control registers

            bit 1 --> 0 = timer uses external clock line
                      1 = timer uses internal E clock ( CPU Clock / 10, most likely 1 Mhz)

            bit 2 --> counting mode
                      0 = normal 16 bit counts down (msb*256 + lsb)+1 ticks
                      1 = dual    8 bit counts down (lsb+1)*(msb+1)   ticks

            bit 5 4 3 --> operation mode

                      0 0 0 --> continous mode
                      0 0 1 --> freq comparison mode
                      0 1 0 --> continous mode
                      0 1 1 --> pulse width comparison mode
                      1 0 0 --> single shot mode
                      1 0 1 --> freq comparison mode
                      1 1 0 --> single shot mode
                      1 1 1 --> pulse width comparison mode

            bit 6 --> IRQ enable,     1 = enabled, 0 = disabled
            bit 7 --> Output enable,  1 = enabled, 0 = disabled

  // 0100 0010 --> IRQ enable,
*/

		if ( data & 0x04 )
		{ // dual    8 bit counts down (lsb+1)*(msb+1)   ticks
			time = ( currptr->lsb_latch[idx] + 1 ) * ( currptr->msb_latch[idx] + 1 );
		}
		else
		{ // normal 16 bit counts down (msb*256 + lsb)+1 ticks
			time = ( currptr->msb_latch[idx] * 256  + currptr->lsb_latch[idx] ) + 1;
		}
		if ( data & 0x02 )
		{ // clock select = 1, use E-clock
			time = PTM6840_CYCLES_TO_TIME(time);
		}
		else
		{ // clock select = 0, use external clock
		// ????damn
		}

		switch ( idx )
		{
			case 0: // control reg 1

			timer_adjust(currptr->timer1, TIME_IN_NSEC(time), 0, 0);
			break;

			case 1: // control reg 2

			timer_adjust(currptr->timer2, TIME_IN_NSEC(time), 0, 0);
			break;

			case 2: // control reg 3

			timer_adjust(currptr->timer3, TIME_IN_NSEC(time), 0, 0);
			break;
		}

		PLOG(("6840PTM #%d control reg %d = %02X\n", which, idx, currptr->control_reg[idx]));
		PLOG((	"  output    enable = %d\n"
				"  interrupt enable = %d\n"
				"  operation mode   = %s\n"
				"  use %s clock \n"
				"  %s counting mode\n",

				(data & 0x80)?1:0,
				(data & 0x40)?1:0,
				opmode[ (data>>3)&0x07 ],
				(data & 0x02)?"E":"external",
				(data & 0x04)?"dual 8 bit" : "16 bit"));
	}

	switch ( offset )
	{
		case PTM_6840_MSBBUF1://2

		PLOG(("6840PTM #%d msbbuf1 = %02X\n", which, data));

		currptr->status_reg &= ~0x01;
		currptr->msb_buffer = data;
		break;

		case PTM_6840_LSB1://3

		PLOG(("6840PTM #%d lsb latch 1 = %02X\n", which, data));

		currptr->status_reg &= ~0x01;
		currptr->lsb_latch[0] = data;
		break;


		case PTM_6840_MSBBUF2://4

		PLOG(("6840PTM #%d msbbuf2 = %02X\n", which, data));

		currptr->status_reg &= ~0x02;
		currptr->msb_buffer = data;
		break;

		case PTM_6840_LSB2://5

		PLOG(("6840PTM #%d lsb latch 2 = %02X\n", which, data));

		currptr->status_reg &= ~0x02;
		currptr->lsb_latch[1] = data;
		break;

		case PTM_6840_MSBBUF3://6

		PLOG(("6840PTM #%d msbbuf3 = %02X\n", which, data));

		currptr->status_reg &= ~0x04;
		currptr->msb_buffer = data;
		break;

		case PTM_6840_LSB3://7

		PLOG(("6840PTM #%d lsb latch 3 = %02X\n", which, data));

		currptr->status_reg &= ~0x04;
		currptr->lsb_latch[2] = data;
		break;

		//case PTM_6840_CTRL2://1
	}
}
///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_t1_timeout: called if timer1 is mature                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void ptm6840_t1_timeout(int which)
{
	ptm6840 *p = ptm + which;

	logerror("**ptm6840 %d t1 timeout**\n", which);

	if ( p->control_reg[0] & 0x40 )
	{ // interrupt enabled
		p->status_reg |= 0x01;
		if ( p->intf->irq_func  ) p->intf->irq_func(0);
	}

	if ( p->intf )
	{
		if ( p->intf->out1_func ) p->intf->out1_func(0, p->output[0]);
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_t2_timeout: called if timer2 is mature                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void ptm6840_t2_timeout(int which)
{
	ptm6840 *p = ptm + which;

	logerror("**ptm6840 %d t2 timeout**\n", which);

	if ( p->control_reg[1] & 0x40 )
	{ // interrupt enabled
		p->status_reg |= 0x02;
		if ( p->intf->irq_func  ) p->intf->irq_func(0);
	}

	if ( p->intf )
	{
		if ( p->intf->out2_func ) p->intf->out2_func(0, p->output[1]);
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_t3_timeout: called if timer3 is mature                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

static void ptm6840_t3_timeout(int which)
{
	ptm6840 *p = ptm + which;

	logerror("**ptm6840 %d t3 timeout**\n", which);

	if ( p->control_reg[1] & 0x40 )
	{ // interrupt enabled
		p->status_reg |= 0x04;
		if ( p->intf->irq_func  ) p->intf->irq_func(0);
	}

	if ( p->intf )
	{
		if ( p->intf->out3_func ) p->intf->out3_func(0, p->output[2]);
	}
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_g1: set gate1 status (0 Or 1)                             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_g1(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->input[0] = state;
}

void ptm6840_set_c1(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->input[0] = state;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_g2: set gate2 status (0 Or 1)                             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_g2(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->input[1] = state;
}
void ptm6840_set_c2(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->input[1] = state;
}

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// ptm6840_set_g3: set gate3 status (0 Or 1)                             //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

void ptm6840_set_g3(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->input[2] = state;
}
void ptm6840_set_c3(int which, int state)
{
	ptm6840 *p = ptm + which;

	p->input[2] = state;
}

///////////////////////////////////////////////////////////////////////////

READ8_HANDLER( ptm6840_0_r ) { return ptm6840_read(0, offset); }
READ8_HANDLER( ptm6840_1_r ) { return ptm6840_read(1, offset); }
READ8_HANDLER( ptm6840_2_r ) { return ptm6840_read(2, offset); }
READ8_HANDLER( ptm6840_3_r ) { return ptm6840_read(3, offset); }

WRITE8_HANDLER( ptm6840_0_w ) { ptm6840_write(0, offset, data); }
WRITE8_HANDLER( ptm6840_1_w ) { ptm6840_write(1, offset, data); }
WRITE8_HANDLER( ptm6840_2_w ) { ptm6840_write(2, offset, data); }
WRITE8_HANDLER( ptm6840_3_w ) { ptm6840_write(3, offset, data); }

READ16_HANDLER( ptm6840_0_r16u ) { return ptm6840_read(0, offset << 8 | 0x00ff); }
READ16_HANDLER( ptm6840_1_r16u ) { return ptm6840_read(1, offset << 8 | 0x00ff); }
READ16_HANDLER( ptm6840_2_r16u ) { return ptm6840_read(2, offset << 8 | 0x00ff); }
READ16_HANDLER( ptm6840_3_r16u ) { return ptm6840_read(3, offset << 8 | 0x00ff); }

WRITE16_HANDLER( ptm6840_0_w16u ) { if (ACCESSING_MSB) ptm6840_write(0, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( ptm6840_1_w16u ) { if (ACCESSING_MSB) ptm6840_write(1, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( ptm6840_2_w16u ) { if (ACCESSING_MSB) ptm6840_write(2, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( ptm6840_3_w16u ) { if (ACCESSING_MSB) ptm6840_write(3, offset, (data >> 8) & 0xff); }

READ16_HANDLER( ptm6840_0_r16l ) { return ptm6840_read(0, offset | 0xff00); }
READ16_HANDLER( ptm6840_1_r16l ) { return ptm6840_read(1, offset | 0xff00); }
READ16_HANDLER( ptm6840_2_r16l ) { return ptm6840_read(2, offset | 0xff00); }
READ16_HANDLER( ptm6840_3_r16l ) { return ptm6840_read(3, offset | 0xff00); }

WRITE16_HANDLER( ptm6840_0_w16 ) {if (ACCESSING_LSB) ptm6840_write(0, offset, data & 0xff); if (ACCESSING_MSB) ptm6840_write(0, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( ptm6840_1_w16 ) {if (ACCESSING_LSB) ptm6840_write(1, offset, data & 0xff); if (ACCESSING_MSB) ptm6840_write(0, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( ptm6840_2_w16 ) {if (ACCESSING_LSB) ptm6840_write(2, offset, data & 0xff); if (ACCESSING_MSB) ptm6840_write(0, offset, (data >> 8) & 0xff); }
WRITE16_HANDLER( ptm6840_3_w16 ) {if (ACCESSING_LSB) ptm6840_write(3, offset, data & 0xff); if (ACCESSING_MSB) ptm6840_write(0, offset, (data >> 8) & 0xff); }
