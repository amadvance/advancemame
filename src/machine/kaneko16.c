/***************************************************************************


                            MCU Code Simulation

  CALC1 is a 40 pin DIP MCU of unknown type with unknown internal rom
  CALC3 is a NEC uPD78322 series MCU with 16K internal rom & 640 bytes of ram
TBSOP01 is a NEC uPD78324 series MCU with 32K internal rom & 1024 bytes of ram
TBSOP02 is likely the same NEC uPD78324 series MCU as the TBS0P01

Currently none of the MCUs' internal roms are dumped so simulation is used

***************************************************************************/

#include "driver.h"
#include "includes/kaneko16.h"
#include "kanekotb.h"	// TOYBOX MCU trojaning results
#include "machine/eeprom.h"




#define MCU_RESPONSE(d) memcpy(&kaneko16_mcu_ram[mcu_offset], d, sizeof(d))

UINT16 *kaneko16_mcu_ram;

/***************************************************************************
                                Gals Panic (set 2)
                                Gals Panic (set 3)
                                Sand Scorpion
                                Bonk's Adventure
                                Blood Warrior
***************************************************************************/

/*
    - see notes about this "calculator" implementation in drivers\galpanic.c
    - bonkadv only uses Random Number, XY Overlap Collision bit and register '0x02'
*/
static READ16_HANDLER(shogwarr_calc_r);
static WRITE16_HANDLER(shogwarr_calc_w);

static struct {
	UINT16 x1p, y1p, x1s, y1s;
	UINT16 x2p, y2p, x2s, y2s;

	INT16 x12, y12, x21, y21;

	UINT16 mult_a, mult_b;
} hit;

READ16_HANDLER(galpanib_calc_r) /* Simulation of the CALC1 MCU */
{
	UINT16 data = 0;

	switch (offset)
	{
		case 0x00/2: // watchdog
			return watchdog_reset_r(0);

		case 0x02/2: // unknown (yet!), used by *MANY* games !!!
			//popmessage("unknown collision reg");
			break;

		case 0x04/2: // similar to the hit detection from SuperNova, but much simpler

			// X Absolute Collision
			if      (hit.x1p >  hit.x2p)	data |= 0x0200;
			else if (hit.x1p == hit.x2p)	data |= 0x0400;
			else if (hit.x1p <  hit.x2p)	data |= 0x0800;

			// Y Absolute Collision
			if      (hit.y1p >  hit.y2p)	data |= 0x2000;
			else if (hit.y1p == hit.y2p)	data |= 0x4000;
			else if (hit.y1p <  hit.y2p)	data |= 0x8000;

			// XY Overlap Collision
			hit.x12 = (hit.x1p) - (hit.x2p + hit.x2s);
			hit.y12 = (hit.y1p) - (hit.y2p + hit.y2s);
			hit.x21 = (hit.x1p + hit.x1s) - (hit.x2p);
			hit.y21 = (hit.y1p + hit.y1s) - (hit.y2p);

			if ((hit.x12 < 0) && (hit.y12 < 0) &&
				(hit.x21 >= 0) && (hit.y21 >= 0))
					data |= 0x0001;

			return data;

		case 0x10/2:
			return (((UINT32)hit.mult_a * (UINT32)hit.mult_b) >> 16);
		case 0x12/2:
			return (((UINT32)hit.mult_a * (UINT32)hit.mult_b) & 0xffff);

		case 0x14/2:
			return (mame_rand() & 0xffff);

		default:
			break;
			//logerror("CPU #0 PC %06x: warning - read unmapped calc address %06x\n",cpu_get_pc(space->cpu),offset<<1);
	}

	return 0;
}

WRITE16_HANDLER(galpanib_calc_w)
{
	switch (offset)
	{
		// p is position, s is size
		case 0x00/2: hit.x1p    = data; break;
		case 0x02/2: hit.x1s    = data; break;
		case 0x04/2: hit.y1p    = data; break;
		case 0x06/2: hit.y1s    = data; break;
		case 0x08/2: hit.x2p    = data; break;
		case 0x0a/2: hit.x2s    = data; break;
		case 0x0c/2: hit.y2p    = data; break;
		case 0x0e/2: hit.y2s    = data; break;
		case 0x10/2: hit.mult_a = data; break;
		case 0x12/2: hit.mult_b = data; break;

		default:
			break;//logerror("CPU #0 PC %06x: warning - write unmapped hit address %06x\n",cpu_get_pc(space->cpu),offset<<1);
	}
}

WRITE16_HANDLER(bloodwar_calc_w)
{

	int isbrap = ( !strcmp(Machine->gamedrv->name,"brapboysj") || !strcmp(Machine->gamedrv->name,"brapboys"));

	/* our implementation is incomplete, b.rap boys requires some modifications */
	if (isbrap)
	{
		shogwarr_calc_w(offset,data,mem_mask);
		return;
	}

	switch (offset)
	{
		// p is position, s is size
		case 0x20/2: hit.x1p = data; break;
		case 0x22/2: hit.x1s = data; break;
		case 0x24/2: hit.y1p = data; break;
		case 0x26/2: hit.y1s = data; break;

		case 0x2c/2: hit.x2p = data; break;
		case 0x2e/2: hit.x2s = data; break;
		case 0x30/2: hit.y2p = data; break;
		case 0x32/2: hit.y2s = data; break;

		// this register is set to zero before any computation,
		// but it has no effect on inputs or result registers
		case 0x38/2: break;

		default:
			break;//logerror("CPU #0 PC %06x: warning - write unmapped hit address %06x\n",cpu_get_pc(space->cpu),offset<<1);
	}
}

/*
 collision detection: absolute "distance", negative if no overlap
         [one inside other] | [ normal overlap ] | [   no overlap   ]
  rect1   <-------------->  |  <----------->     |  <--->
  rect2       <----->       |     <----------->  |             <--->
  result      <---------->  |     <-------->     |       <---->
*/

static INT16 calc_compute_x(void)
{
	INT16 x_coll;

	// X distance
	if ((hit.x2p >= hit.x1p) && (hit.x2p < (hit.x1p + hit.x1s)))		// x2p inside x1
		x_coll = (hit.x1s - (hit.x2p - hit.x1p));
	else if ((hit.x1p >= hit.x2p) && (hit.x1p < (hit.x2p + hit.x2s)))	// x1p inside x2
		x_coll = (hit.x2s - (hit.x1p - hit.x2p));
	else																// normal/no overlap
	 	x_coll = ((hit.x1s + hit.x2s)/2) - abs((hit.x1p + hit.x1s/2) - (hit.x2p + hit.x2s/2));

	return x_coll;
}

static INT16 calc_compute_y(void)
{
	INT16 y_coll;

	// Y distance
	if ((hit.y2p >= hit.y1p) && (hit.y2p < (hit.y1p + hit.y1s)))		// y2p inside y1
		y_coll = (hit.y1s - (hit.y2p - hit.y1p));
	else if ((hit.y1p >= hit.y2p) && (hit.y1p < (hit.y2p + hit.y2s)))	// y1p inside y2
		y_coll = (hit.y2s - (hit.y1p - hit.y2p));
	else																// normal/no overlap
		y_coll = ((hit.y1s + hit.y2s)/2) - abs((hit.y1p + hit.y1s/2) - (hit.y2p + hit.y2s/2));

	return y_coll;
}

READ16_HANDLER(bloodwar_calc_r)
{
	UINT16 data = 0;
	INT16 x_coll, y_coll;

	/* our implementation is incomplete, b.rap boys requires some modifications */
	int isbrap = ( !strcmp(Machine->gamedrv->name,"brapboysj") || !strcmp(Machine->gamedrv->name,"brapboys"));

	if (isbrap)
	{
		return shogwarr_calc_r(offset,mem_mask);
	}

	x_coll = calc_compute_x();
	y_coll = calc_compute_y();

	switch (offset)
	{
		case 0x00/2: // X distance
			return x_coll;

		case 0x02/2: // Y distance
			return y_coll;

		case 0x04/2: // similar to the hit detection from SuperNova, but much simpler

			// 4th nibble: Y Absolute Collision -> possible values = 9,8,4,3,2
			if      (hit.y1p >  hit.y2p)	data |= 0x2000;
			else if (hit.y1p == hit.y2p)	data |= 0x4000;
			else if (hit.y1p <  hit.y2p)	data |= 0x8000;
			if (y_coll<0) data |= 0x1000;

			// 3rd nibble: X Absolute Collision -> possible values = 9,8,4,3,2
			if      (hit.x1p >  hit.x2p)	data |= 0x0200;
			else if (hit.x1p == hit.x2p)	data |= 0x0400;
			else if (hit.x1p <  hit.x2p)	data |= 0x0800;
			if (x_coll<0) data |= 0x0100;

			// 2nd nibble: always set to 4
			data |= 0x0040;

			// 1st nibble: XY Overlap Collision -> possible values = 0,2,4,f
			if (x_coll>=0) data |= 0x0004;
			if (y_coll>=0) data |= 0x0002;
			if ((x_coll>=0)&&(y_coll>=0)) data |= 0x000F;

			return data;

		case 0x14/2:
			return (mame_rand() & 0xffff);

		case 0x20/2: return hit.x1p;
		case 0x22/2: return hit.x1s;
		case 0x24/2: return hit.y1p;
		case 0x26/2: return hit.y1s;

		case 0x2c/2: return hit.x2p;
		case 0x2e/2: return hit.x2s;
		case 0x30/2: return hit.y2p;
		case 0x32/2: return hit.y2s;

		default:
			break; //logerror("CPU #0 PC %06x: warning - read unmapped calc address %06x\n",cpu_get_pc(space->cpu),offset<<1);
	}

	return 0;
}

/*
    B Rap Boys
    Shogun Warriors

*/


static struct {
	int x1p, y1p, z1p, x1s, y1s, z1s;
	int x2p, y2p, z2p, x2s, y2s, z2s;

	int x1po, y1po, z1po, x1so, y1so, z1so;
	int x2po, y2po, z2po, x2so, y2so, z2so;

	int x12, y12, z12, x21, y21, z21;

	int x_coll, y_coll, z_coll;

	int x1tox2, y1toy2, z1toz2;

	UINT16 mult_a, mult_b;

	UINT16 flags;
	UINT16 mode;

} shogwarr_hit;


//calculate simple intersection of two segments

static int shogwarr_calc_compute(int x1, int w1, int x2, int w2)
{
	int dist;

	if(x2>=x1 && x2+w2<=(x1+w1))
	{
		//x2 inside x1
		dist=w2;
	}
	else
	{
		if(x1>=x2 && x1+w1<=(x2+w2))
		{
			//x1 inside x2
			dist=w1;
		}
		else
		{
			if(x2<x1)
			{
				//swap
				int tmp=x1;
				x1=x2;
				x2=tmp;
				tmp=w1;
				w1=w2;
				w2=tmp;
			}
			dist=x1+w1-x2;
		}
	}
	return dist;
}

//calc segment coordinates

static void shogwarr_calc_org(int mode, int x0, int s0,  int* x1, int* s1)
{
	switch(mode)
	{
		case 0: *x1=x0; *s1=s0; break;
		case 1: *x1=x0-s0/2; *s1=s0; break;
		case 2: *x1=x0-s0; *s1=s0; break;
		case 3:	*x1=x0-s0; *s1=2*s0; break;
	}
	//x1 is the left most coord, s1 = width
}

static void shogwarr_recalc_collisions(void)
{
	//calculate positions and sizes

	int mode=shogwarr_hit.mode;

	shogwarr_hit.flags=0;

	shogwarr_calc_org(mode&3, shogwarr_hit.x1po, shogwarr_hit.x1so, &shogwarr_hit.x1p, &shogwarr_hit.x1s);
	mode>>=2;
	shogwarr_calc_org(mode&3, shogwarr_hit.y1po, shogwarr_hit.y1so, &shogwarr_hit.y1p, &shogwarr_hit.y1s);
	mode>>=2;
	shogwarr_calc_org(mode&3, shogwarr_hit.z1po, shogwarr_hit.z1so, &shogwarr_hit.z1p, &shogwarr_hit.z1s);

	mode>>=4;

	shogwarr_calc_org(mode&3, shogwarr_hit.x2po, shogwarr_hit.x2so, &shogwarr_hit.x2p, &shogwarr_hit.x2s);
	mode>>=2;
	shogwarr_calc_org(mode&3, shogwarr_hit.y2po, shogwarr_hit.y2so, &shogwarr_hit.y2p, &shogwarr_hit.y2s);
	mode>>=2;
	shogwarr_calc_org(mode&3, shogwarr_hit.z2po, shogwarr_hit.z2so, &shogwarr_hit.z2p, &shogwarr_hit.z2s);


	shogwarr_hit.x1tox2=abs(shogwarr_hit.x2po-shogwarr_hit.x1po);
	shogwarr_hit.y1toy2=abs(shogwarr_hit.y2po-shogwarr_hit.y1po);
	shogwarr_hit.z1toz2=abs(shogwarr_hit.z2po-shogwarr_hit.z1po);


	shogwarr_hit.x_coll = shogwarr_calc_compute(shogwarr_hit.x1p, shogwarr_hit.x1s, shogwarr_hit.x2p, shogwarr_hit.x2s);
	shogwarr_hit.y_coll = shogwarr_calc_compute(shogwarr_hit.y1p, shogwarr_hit.y1s, shogwarr_hit.y2p, shogwarr_hit.y2s);
	shogwarr_hit.z_coll = shogwarr_calc_compute(shogwarr_hit.z1p, shogwarr_hit.z1s, shogwarr_hit.z2p, shogwarr_hit.z2s);


	// 4th nibble: Y Absolute Collision -> possible values = 9,8,4,3,2
	if      (shogwarr_hit.y1p >  shogwarr_hit.y2p)	shogwarr_hit.flags |= 0x2000;
	else if (shogwarr_hit.y1p == shogwarr_hit.y2p)	shogwarr_hit.flags |= 0x4000;
	else if (shogwarr_hit.y1p <  shogwarr_hit.y2p)	shogwarr_hit.flags |= 0x8000;
	if (shogwarr_hit.y_coll<0) shogwarr_hit.flags |= 0x1000;

	// 3rd nibble: X Absolute Collision -> possible values = 9,8,4,3,2
	if      (shogwarr_hit.x1p >  shogwarr_hit.x2p)	shogwarr_hit.flags |= 0x0200;
	else if (shogwarr_hit.x1p == shogwarr_hit.x2p)	shogwarr_hit.flags |= 0x0400;
	else if (shogwarr_hit.x1p <  shogwarr_hit.x2p)	shogwarr_hit.flags |= 0x0800;
	if (shogwarr_hit.x_coll<0) shogwarr_hit.flags |= 0x0100;

	// 2nd nibble: Z Absolute Collision -> possible values = 9,8,4,3,2
	if      (shogwarr_hit.z1p >  shogwarr_hit.z2p)	shogwarr_hit.flags |= 0x0020;
	else if (shogwarr_hit.z1p == shogwarr_hit.z2p)	shogwarr_hit.flags |= 0x0040;
	else if (shogwarr_hit.z1p <  shogwarr_hit.z2p)	shogwarr_hit.flags |= 0x0080;
	if (shogwarr_hit.z_coll<0) shogwarr_hit.flags |= 0x0010;

	// 1st nibble: XYZ Overlap Collision
	if ((shogwarr_hit.x_coll>=0)&&(shogwarr_hit.y_coll>=0)&&(shogwarr_hit.z_coll>=0)) shogwarr_hit.flags |= 0x0008;
	if ((shogwarr_hit.x_coll>=0)&&(shogwarr_hit.z_coll>=0)) shogwarr_hit.flags |= 0x0004;
	if ((shogwarr_hit.y_coll>=0)&&(shogwarr_hit.z_coll>=0)) shogwarr_hit.flags |= 0x0002;
	if ((shogwarr_hit.x_coll>=0)&&(shogwarr_hit.y_coll>=0)) shogwarr_hit.flags |= 0x0001;
}


static WRITE16_HANDLER(shogwarr_calc_w)
{
	int idx=offset*4;
	switch (idx)
	{
		// p is position, s is size
		case 0x00:
		case 0x28:

					shogwarr_hit.x1po = data; break;

		case 0x04:
		case 0x2c:
					shogwarr_hit.x1so = data; break;

		case 0x08:
		case 0x30:
					shogwarr_hit.y1po = data; break;

		case 0x0c:
		case 0x34:
					shogwarr_hit.y1so = data; break;

		case 0x10:
		case 0x58:
					shogwarr_hit.x2po = data; break;

		case 0x14:
		case 0x5c:
					shogwarr_hit.x2so = data; break;

		case 0x18:
		case 0x60:
					shogwarr_hit.y2po = data; break;

		case 0x1c:
		case 0x64:
					shogwarr_hit.y2so = data; break;

		case 0x38:
		case 0x50:
					shogwarr_hit.z1po = data; break;
		case 0x3c:
		case 0x54:
					shogwarr_hit.z1so = data; break;

		case 0x20:
		case 0x68:
					shogwarr_hit.z2po = data; break;

		case 0x24:
		case 0x6c:
					shogwarr_hit.z2so = data; break;

		case 0x70:
					shogwarr_hit.mode=data;break;

		default:
			break;//logerror("CPU #0 PC %06x: warning - write unmapped hit address %06x [ %06x] = %06x\n",cpu_get_pc(space->cpu),offset<<1, idx, data);
	}

	shogwarr_recalc_collisions();
}


static READ16_HANDLER(shogwarr_calc_r)
{

	int idx=offset*4;

	switch (idx)
	{
		case 0x00: // X distance
		case 0x10:
			return shogwarr_hit.x_coll;

		case 0x04: // Y distance
		case 0x14:
			return shogwarr_hit.y_coll;

		case 0x18: // Z distance
			return shogwarr_hit.z_coll;

		case 0x08:
		case 0x1c:

			return shogwarr_hit.flags;

		case 0x28:
			return (mame_rand() & 0xffff);

		case 0x40: return shogwarr_hit.x1po;
		case 0x44: return shogwarr_hit.x1so;
		case 0x48: return shogwarr_hit.y1po;
		case 0x4c: return shogwarr_hit.y1so;
		case 0x50: return shogwarr_hit.z1po;
		case 0x54: return shogwarr_hit.z1so;

		case 0x58: return shogwarr_hit.x2po;
		case 0x5c: return shogwarr_hit.x2so;
		case 0x60: return shogwarr_hit.y2po;
		case 0x64: return shogwarr_hit.y2so;
		case 0x68: return shogwarr_hit.z2po;
		case 0x6c: return shogwarr_hit.z2so;

		case 0x80: return shogwarr_hit.x1tox2;
		case 0x84: return shogwarr_hit.y1toy2;
		case 0x88: return shogwarr_hit.z1toz2;

		default:
			break;//logerror("CPU #0 PC %06x: warning - read unmapped calc address %06x [ %06x]\n",cpu_get_pc(space->cpu),offset<<1, idx);
	}

	return 0;
}


/***************************************************************************
                                CALC3 MCU:

                                Shogun Warriors
                                Fujiyama Buster
                                B.Rap Boys
***************************************************************************/
/*
---------------------------------------------------------------------------
                                CALC 3

92  B.Rap Boys              KANEKO CALC3 508 (74 PIN PQFP)
92  Shogun Warriors
    Fujiyama Buster         KANEKO CALC3 508 (74 Pin PQFP)
---------------------------------------------------------------------------

MCU Initialization command:

shogwarr: CPU #0 PC 00037A : MCU executed command: 00FF 0059 019E 030A FFFE 0042 0020 7FE0
fjbuster: CPU #0 PC 00037A : MCU executed command: 00FF 0059 019E 030A FFFE 0042 0020 7FE0
brapboys: CPU #0 PC 000BAE : MCU executed command: 00FF 00C2 0042 0830 082E 00C8 0020 0872

shogwarr/fjbuster:

00FF : busy flag, MCU clears it when cmd finished (main program loops until it's cleared)
0059 : MCU writes DSW -> $102e15
019E : Eeprom data address - dumps the 0x80 bytes of Eeprom here
030A : location where MCU will get its parameters from now on
FFFE : probably polled by MCU, needs to be kept alive (cleared by main cpu - IT2)
0042 : MCU writes its checksum
00207FE0 : base of 'stack' used when transfering tables out of MCU
*/

void calc3_mcu_run(void);
static int calc3_mcu_status, calc3_mcu_command_offset;


void calc3_mcu_init(void)
{
	calc3_mcu_status = 0;
	calc3_mcu_command_offset = 0;
}

WRITE16_HANDLER( calc3_mcu_ram_w )
{
	COMBINE_DATA(&kaneko16_mcu_ram[offset]);
}

#define CALC3_MCU_COM_W(_n_)				\
WRITE16_HANDLER( calc3_mcu_com##_n_##_w )	\
{											\
	logerror("calc3w %d %04x %04x\n",_n_, data,mem_mask); \
	calc3_mcu_status |= (1 << _n_);			\
}

CALC3_MCU_COM_W(0)
CALC3_MCU_COM_W(1)
CALC3_MCU_COM_W(2)
CALC3_MCU_COM_W(3)




static UINT16 calc3_mcu_crc;

/* decryption tables */

/* this should probably be derived from the final block at the end of the MCU rom somehow ...
    (see 'finalblock' debug output when verbose = 1)
   for now just putting it in a bit table, only used keys are filled in as nothing else was
   extracted when getting the tables, unknown values are filled in as -1

   key 0 means no decryption, this could be done in logic rather than expecting the first
   part of the table to be 0
*/

static const INT16 calc3_keydata[0x40*0x100] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xce,0x55,0xa1,0x56,0x20,0xe0,0xcb,0xa9,0x5b,0x9b,0xd6,0x9b,0x5e,0x6e,0x6c,0xf6,
0xf8,0x16,0xb6,0x03,0x76,0xcb,0x47,0x51,0xed,0x20,0xcc,0xb3,0x4c,0x2a,0x74,0x88,
0x20,0xe4,0x1f,0x3f,0x25,0xd2,0x49,0x19,0x90,0xe5,0xb4,0x31,0x1e,0x9f,0xd7,0xba,
0xda,0x77,0x08,0x89,0xb1,0x7f,0x6b,0xd3,0xb6,0x44,0x99,0x37,0x35,0xf6,0x0f,0xff,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0x01,0x4a,0x9e,0xa3,0x0c,0xb4,0x74,0x2d,0x7c,0xbd,0x79,0x3a,0x62,0xe9,0x29,0x12,
0x01,0x50,0xc0,0x39,0x5a,0x47,0x27,0x38,0x77,0x3c,0x9e,0xa0,0x7a,0xe2,0x29,0x4d,
0x64,0x8e,0xbb,0xe8,0xa5,0xe1,0x23,0xaf,0x67,0xa8,0xb9,0x79,0x99,0x00,0xc8,0xf4,
0xbd,0xbe,0xb9,0x32,0x71,0x0e,0x03,0x64,0xbe,0x59,0xd5,0xe5,0x22,0x6d,0x80,0x7a,
0x5e,0xd8,0xa6,0xd7,0x00,0x94,0x23,0x66,0xaf,0xea,0xbe,0x45,0x38,0x90,0x8e,0x8d,
0x5b,0x14,0x2c,0xda,0x55,0x7e,0x9d,0x08,0x2d,0x31,0xfd,0x3a,0xbe,0x13,0xeb,0x21,
0x87,0xee,0xb7,0x7b,0x34,0x15,0x4d,0xd9,0xeb,0x4a,0xde,0xa6,0x57,0xdf,0x53,0x65,
0x75,0x1c,0x72,0x3a,0x20,0xe1,0xcd,0xac,0x5d,0x8c,0x6d,0xa9,0x66,0x1d,0x41,0xca,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0xca,0x23,0x9e,0x00,0x37,0x72,0x61,0x0d,0x71,0x99,0x02,0xb1,0x5b,0x8f,0x50,0x22,
0x06,0xdb,0xd6,0xd5,0x44,0x42,0xcd,0x44,0xf0,0xa2,0xc4,0xef,0xfa,0x1e,0x11,0xa1,
0x34,0x7b,0xf6,0xf4,0xdf,0xca,0x92,0x17,0xf3,0x48,0x8d,0xcf,0x30,0x82,0x81,0xbc,
0xe8,0xbd,0x29,0xde,0x89,0x94,0x4b,0x57,0xed,0xe3,0x66,0x73,0x60,0xe4,0x1b,0xe5,
0x75,0x98,0x5b,0x53,0x88,0x69,0x54,0x16,0x11,0x0d,0x1c,0x3b,0xae,0xad,0x1a,0xcd,
0xed,0x47,0x37,0x56,0xdd,0x51,0xc8,0xa3,0x52,0x9f,0x38,0xc8,0xfb,0x86,0x78,0x64,
0x25,0x42,0x27,0x26,0x4b,0x95,0x80,0x90,0x64,0xb1,0x07,0xfb,0xeb,0x58,0xf2,0xdc,
0xad,0x42,0x57,0x45,0x56,0xbf,0x19,0xaf,0xb9,0x9d,0x93,0xf6,0xe1,0x4c,0x01,0xa5,
0xdb,0x40,0xb2,0x74,0x40,0x97,0xee,0x0f,0x84,0xfc,0xa7,0x19,0x01,0xcb,0xe1,0x71,
0xc1,0x75,0xe2,0xb4,0x0d,0x26,0x19,0x03,0xb9,0xa6,0xce,0x06,0x2c,0x7d,0x8d,0x30,
0x31,0x5b,0x52,0x45,0x7f,0xb7,0x75,0x1a,0x09,0xb5,0x53,0x9c,0x06,0x4d,0xc0,0x14,
0xbf,0xaa,0x2f,0xa9,0x1a,0xd0,0x9d,0x27,0xe9,0x82,0x41,0xff,0xf2,0x63,0xf5,0x8e,
0xbd,0x5b,0x62,0xa2,0x20,0x3c,0xed,0x3a,0x8b,0xa6,0x64,0x8d,0x13,0x27,0x67,0x4e,
0x40,0xa7,0x96,0x2f,0x95,0x04,0x7f,0xa4,0xe3,0xf9,0x45,0xe9,0x4c,0x44,0x10,0x46,
0x19,0x08,0x38,0x91,0x3b,0x70,0x2f,0xf5,0xa2,0x95,0x31,0xf3,0x40,0xa2,0xad,0xa6,
0xdb,0x35,0x70,0x4b,0x96,0x09,0x97,0x00,0x3d,0xd2,0x32,0xcc,0x52,0x69,0xb7,0xe0,
0xdb,0x29,0x2c,0x1d,0xe8,0x99,0x12,0xd6,0xe6,0x4b,0x12,0xd6,0xa6,0x04,0x6a,0xa5,
0x2a,0x1c,0x15,0x07,0x35,0x28,0xbc,0xc6,0x90,0xd7,0x5e,0xb1,0x1d,0x1a,0xc1,0xe5,
0x9c,0x88,0x96,0x4b,0x3f,0x00,0x6f,0x62,0xef,0x90,0x60,0x3f,0x5b,0x95,0x77,0xd1,
0xc4,0x24,0xdc,0x6a,0x8a,0xa9,0xc6,0x7b,0x74,0xce,0x24,0x9f,0xc3,0x9f,0x06,0xdc,
0xf4,0xeb,0xd0,0x25,0x58,0xed,0x1d,0x23,0x54,0x2b,0x73,0x34,0x78,0x9f,0xab,0xb5,
0x41,0x15,0x1d,0x7c,0xac,0xd4,0x8e,0xa9,0x82,0x80,0xd9,0x9f,0x5d,0x3f,0x5f,0x4d,
0x7c,0x1b,0x2f,0xb2,0x4b,0xa7,0xf4,0xa0,0xaf,0xe6,0xa2,0xc0,0x15,0x68,0xde,0xd6,
0x38,0xb6,0x31,0x46,0xb5,0xf0,0xeb,0xd7,0x50,0xb5,0xd7,0xb8,0x03,0x43,0xa4,0xc0,
0xca,0xe0,0x0d,0xfa,0x2f,0x78,0xcd,0x61,0x97,0x87,0x45,0xe8,0x4a,0x39,0xe9,0xbe,
0x43,0xd0,0x6f,0xcf,0xbc,0x47,0xb6,0x8e,0x77,0x35,0x76,0xf2,0xcc,0xf3,0xab,0xbe,
0x77,0x01,0xc1,0x06,0x1e,0xa6,0x80,0xef,0xa3,0xd7,0xb5,0xb6,0x2e,0x5a,0xa4,0xf4,
0xf8,0x2b,0x30,0x20,0xd8,0x1f,0xc6,0x55,0x8f,0xc7,0x0d,0x55,0xd2,0x97,0x4f,0xce,
0x1b,0x48,0xa4,0xdd,0x2e,0x7a,0xe3,0xd1,0x6d,0x9e,0x49,0x31,0xdb,0x13,0xe6,0x00,
0xf1,0x8f,0xcb,0x3f,0x23,0xc1,0xf3,0xb4,0x30,0x35,0xf4,0xea,0x2c,0x77,0x65,0x79,
0x4e,0x7b,0x0e,0x87,0x79,0x3c,0xd1,0x8f,0x8b,0xa4,0x5a,0x61,0x68,0xac,0x87,0x6b,
0xc4,0xc4,0x98,0x36,0xb3,0x75,0x16,0x33,0xf2,0x45,0x84,0xb8,0xf2,0xdb,0xc7,0x46,
0xa8,0x63,0x55,0x0d,0x15,0x34,0x1f,0xb2,0x96,0xb0,0x3f,0x4f,0xec,0x6c,0x60,0xbb,
0x0b,0x91,0xf0,0x0d,0xa2,0x83,0x07,0x5b,0x6c,0xc0,0x14,0xc7,0x3b,0x0a,0x4d,0xbd,
0xc0,0xc8,0xd3,0x76,0x1c,0xaa,0xa8,0xc1,0x27,0x8c,0x50,0x02,0x81,0x9d,0x48,0x7b,
0x5c,0xc0,0x2a,0xca,0x06,0x32,0x9d,0xb3,0x38,0x6d,0xfd,0x20,0x21,0x4d,0xce,0x66,
0x30,0x72,0xe0,0xca,0xa4,0xe5,0x41,0x44,0xd4,0xfe,0xe6,0x82,0x3e,0x85,0x18,0x30,
0x50,0x17,0x9f,0x77,0xf8,0xcb,0xb0,0xc4,0xed,0x16,0x96,0xca,0xba,0xec,0x22,0xca,
0x8f,0x28,0xd3,0x12,0xc6,0x2e,0xc4,0xc3,0x36,0xcf,0x59,0xd8,0x3a,0x6c,0xa7,0x64,
0x7f,0x5f,0xa6,0x1b,0x90,0x96,0x19,0x14,0x22,0x81,0x38,0xcd,0x20,0x2f,0x22,0x70,
0x74,0xb4,0x04,0x55,0x9a,0xcd,0x09,0xc7,0xe5,0xc7,0xff,0x0b,0x8e,0x9c,0xce,0x9e,
0x81,0x5f,0x98,0xbf,0xe7,0xdb,0xaf,0x2d,0x71,0x77,0x3a,0x32,0x69,0x5d,0xa5,0xe0,
0x79,0xdc,0xcc,0x9b,0x38,0x0a,0xe6,0xd7,0x79,0xad,0x33,0x23,0x53,0x5c,0x64,0x67,
0xee,0xe1,0xcc,0x6a,0x13,0xe2,0x4a,0x97,0x71,0xc0,0xf4,0x00,0xae,0xc0,0x84,0xa3,
0x34,0x69,0x83,0xec,0xb8,0x2c,0x36,0x7c,0x8a,0x4b,0x4a,0x29,0x9f,0xf3,0x41,0x46,
0x5e,0xab,0x9b,0x24,0x2d,0xf2,0xc4,0xd8,0xb9,0x24,0xbf,0x3f,0x08,0x9e,0x96,0x40,
0x3f,0x22,0x7f,0x51,0x32,0x7d,0xcf,0x3d,0xb0,0x67,0x9f,0x24,0x8b,0xaa,0x3e,0xc3,
0x69,0x87,0x5c,0xf5,0x4d,0x55,0xf2,0x7a,0xe2,0x6b,0xf3,0xf8,0x8d,0x40,0xb4,0x3f,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0xfe,0xf7,0xc7,0x85,0xe4,0xd3,0x93,0xd0,0x36,0x61,0xc8,0x4d,0x0c,0x3e,0x2a,0x1c,
0x32,0x1a,0xbf,0x1a,0xb0,0x60,0xb5,0xa8,0x1a,0x19,0x16,0xaf,0x96,0x0d,0x2d,0xe0,
0xb1,0x2c,0xb6,0x41,0xe1,0x2d,0xc8,0xe5,0xd9,0x75,0x82,0xfa,0x90,0x3a,0x77,0x09,
0x0e,0xe8,0xda,0x7a,0xfb,0xc4,0x68,0x57,0xe8,0xcf,0x18,0x51,0x5b,0xed,0x83,0x08,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0x75,0x49,0x7a,0xd6,0x84,0xbd,0x70,0xbe,0x10,0x19,0x6c,0xb2,0xa9,0x5f,0x6b,0x78,
0x14,0x29,0x2a,0x15,0xd0,0x4a,0x10,0x93,0x69,0xb2,0xdf,0x02,0x7f,0x92,0x19,0xbd,
0x95,0xc1,0x32,0xee,0x99,0x5e,0x7a,0x55,0x37,0xb7,0xc8,0x45,0xdc,0x6a,0xe6,0xef,
0x8b,0xca,0xbe,0xe2,0x63,0x85,0x49,0xd3,0xeb,0x83,0x32,0x9a,0x23,0x11,0x4c,0x7e,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0x02,0xdf,0x0f,0x0e,0x98,0x3a,0xa7,0xaf,0xbd,0xb0,0xe6,0x3a,0x8d,0xeb,0xb4,0x3b,
0x18,0x67,0x78,0xbb,0xeb,0x1c,0x09,0x06,0x5c,0x5c,0x51,0xb1,0x98,0xfe,0xc0,0xdc,
0x6d,0xbc,0x77,0xb5,0xd7,0xda,0x2f,0x3d,0x4c,0x08,0xee,0x4f,0xc7,0x8a,0x68,0xde,
0x94,0x96,0x35,0x7e,0xe0,0xfe,0xb7,0x26,0xff,0x0e,0xc7,0x34,0x7b,0xb8,0x25,0xb1,
0xe0,0xee,0x9d,0xd7,0x49,0x50,0xf9,0xd0,0xa8,0x07,0xa9,0xc1,0xd8,0xf1,0x33,0x07,
0x63,0xfe,0x5b,0xc0,0x13,0xda,0x12,0x8e,0x3a,0xcc,0x1e,0x97,0xc1,0xdd,0x8d,0xd1,
0xf2,0x3e,0xda,0x7c,0x04,0xe4,0xdc,0xef,0x69,0x75,0x72,0x98,0xd9,0x67,0xee,0x3f,
0x1d,0x66,0x44,0x8a,0x9c,0xf7,0xf2,0xc6,0xa7,0x5c,0xae,0xe4,0x83,0xb7,0xd1,0xc2,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0xd0,0xa8,0x87,0xcf,0xe1,0x22,0x8d,0xe7,0xfa,0x26,0x85,0xce,0x6d,0xfa,0x2c,0x8b,
0xbd,0x75,0xfe,0x64,0x2b,0xab,0x25,0xa1,0x02,0xcc,0x1f,0x93,0xa2,0x4a,0x31,0xd5,
0x4d,0x3a,0x8e,0xd2,0xb3,0xfd,0x46,0x87,0x3f,0x1f,0xef,0x8a,0x1e,0x7f,0x15,0x4c,
0x13,0xb1,0x61,0x9b,0xfc,0xa0,0x8b,0x6b,0x22,0x78,0x00,0xd5,0x44,0xc3,0x52,0x60,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0xaf,0x2e,0xd9,0xfd,0x18,0xd1,0xb8,0xc2,0x9b,0x33,0x28,0x30,0x01,0xff,0x1c,0xf4,
0xc6,0xf9,0xc9,0x7c,0xa2,0x9a,0x8f,0xb9,0xd9,0xfa,0xa7,0x24,0x42,0xf3,0x8b,0xff,
0x18,0x84,0x29,0xdd,0x82,0x73,0xc7,0x64,0xe3,0x37,0xb3,0x52,0xe2,0x14,0xb2,0xbe,
0x37,0x88,0x25,0xa0,0x3b,0xe6,0xfb,0x94,0x2d,0x41,0x59,0xdb,0x44,0x8b,0x0b,0xa3,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0x7c,0x82,0x91,0x60,0xee,0x5d,0x7a,0x63,0x8a,0x2c,0x91,0xe7,0x94,0x02,0x02,0x42,
0x7d,0x8a,0xba,0x09,0x78,0xa6,0x50,0xd6,0xbe,0x55,0xb5,0x49,0xa1,0xda,0x9d,0x4d,
0x51,0x1b,0xab,0x9d,0x70,0x47,0x5f,0x86,0x57,0xbb,0xbf,0xee,0x24,0x27,0xc6,0x95,
0x8b,0xec,0x90,0x9b,0x58,0xc9,0x42,0x43,0xc6,0xb7,0xba,0xf6,0x82,0x11,0xfa,0x8b,
0x7d,0xf8,0x54,0xc5,0x74,0xf6,0x54,0x1d,0x40,0xe1,0x71,0xc2,0xdd,0x03,0x72,0xdf,
0x3b,0x77,0xa1,0x1c,0xc7,0xd6,0xb1,0x67,0xb7,0x13,0x6f,0xf4,0x18,0xa4,0x2a,0x82,
0x98,0xe3,0xe3,0xe1,0x12,0xb3,0x33,0xb1,0xde,0x66,0xff,0x6c,0xd5,0xde,0xdd,0xa6,
0x26,0xf3,0x44,0x94,0xdb,0x15,0x76,0xcc,0x28,0x33,0x2d,0x4b,0x79,0xdb,0x06,0xbc,
0x39,0xa2,0xb0,0xf7,0x63,0xc6,0xd4,0xc9,0xc9,0x12,0xc2,0xf3,0x26,0x02,0xe0,0x75,
0xe4,0x28,0xd2,0x0c,0xad,0xce,0x68,0xf9,0xb3,0xdd,0x4b,0x04,0xbe,0xfd,0x65,0xc1,
0xfa,0xfe,0x14,0x12,0x7d,0x77,0x0e,0xed,0x99,0xad,0x11,0x5f,0xe6,0xb6,0x52,0xd1,
0x0d,0xde,0xa2,0x8a,0x55,0x49,0x60,0x76,0xee,0xda,0x21,0x26,0x00,0x54,0x20,0x17,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0x27,0xb4,0xc1,0xba,0x5f,0xd4,0x47,0x04,0x1f,0xfe,0x42,0xa5,0xce,0xb5,0x23,0xbe,
0x42,0xcc,0x59,0x71,0x51,0x13,0x41,0xa4,0x18,0x35,0xcf,0x02,0x0d,0xe1,0xfb,0x43,
0xc5,0x28,0xed,0xae,0x84,0x27,0xe8,0xdc,0x89,0xe5,0xf6,0xbe,0x16,0x7e,0x57,0xe1,
0x41,0x82,0xa9,0xf1,0x7c,0x98,0xd7,0x7c,0xe5,0x67,0xc2,0xf9,0x4d,0xb6,0xb0,0x09,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0x33,0x8e,0x6f,0x22,0xac,0x01,0x9f,0x7a,0xc7,0xca,0x23,0xc4,0xa4,0x9c,0x07,0xf8,
0x98,0xc1,0x17,0x4a,0xb2,0xbc,0xcc,0xf4,0x0e,0x2e,0xf6,0x35,0xdd,0xf6,0x29,0x9a,
0x2f,0x9c,0xe8,0x7c,0x86,0xf0,0x93,0xca,0x1a,0xee,0x10,0x08,0xec,0xe5,0x3a,0x98,
0x8c,0xd8,0x0d,0x39,0xaa,0x25,0x8d,0xcd,0x5d,0x64,0x7a,0x5f,0x35,0x92,0xb5,0x64,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0xdd,0xa9,0x15,0x18,0x9d,0x01,0x24,0x9c,0xed,0x7a,0xeb,0x42,0x8b,0x9e,0xf3,0x72,
0x26,0xc9,0x45,0x18,0xea,0xd2,0x54,0x5f,0xcf,0xed,0xac,0xbc,0xa1,0xd0,0x5f,0x2f,
0xce,0x15,0x2a,0xc6,0xf1,0xe0,0x69,0x62,0x22,0xc1,0xc0,0xbd,0xfa,0xdc,0x85,0xac,
0x67,0x47,0xee,0xa2,0x35,0xb3,0xff,0x76,0x57,0x4f,0x30,0x66,0xf8,0xe9,0xe0,0x5b,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0x0f,0x35,0xa3,0x7c,0xf1,0x75,0xea,0x27,0x36,0x4d,0x4b,0x1a,0x3b,0x24,0xef,0x9d,
0x4e,0x39,0x7e,0x7b,0x3d,0x42,0x0d,0xc2,0x9f,0x53,0xc7,0xc4,0xdf,0x02,0x5d,0x61,
0x0c,0xc9,0x2d,0x89,0x63,0xab,0x34,0xfc,0x97,0x1b,0xb5,0x54,0xe6,0x19,0xa5,0x46,
0xdc,0x9e,0xdc,0x25,0xe4,0x3a,0xfc,0xa7,0x93,0xfc,0x1f,0xec,0xb3,0x92,0x43,0xbc,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0x5d,0xdd,0xd4,0xbe,0xc0,0xff,0xe1,0x1b,0x3a,0xd6,0x61,0xc8,0x90,0x02,0x23,0x08,
0xfa,0x7a,0xfa,0x1d,0xab,0x2b,0xa0,0x0b,0x24,0xe1,0x6b,0x9e,0x38,0x6b,0xb2,0xae,
0xdd,0xfa,0xfc,0xa2,0xb8,0xcb,0xed,0x33,0x66,0x06,0xef,0x72,0x8b,0xe5,0xa3,0x0d,
0x9b,0x18,0x05,0xce,0x6a,0x69,0x61,0x64,0x74,0x9c,0xf9,0x66,0xec,0x99,0x72,0x95,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0x2c,0xf3,0x0d,0x50,0x30,0x77,0xc2,0x25,0x91,0x83,0x5f,0xac,0xf4,0x56,0x96,0xc8,
0x70,0x86,0x16,0x5a,0x1b,0xa1,0x7d,0x08,0x53,0x15,0xfd,0x3c,0xcb,0x4d,0xd0,0x70,
0x5c,0x1d,0x5c,0xaa,0x87,0x60,0x24,0x42,0xcc,0xe0,0x75,0xeb,0xae,0x75,0xcc,0xf1,
0x81,0x71,0x09,0xc0,0xf8,0x3c,0x55,0xa6,0x71,0x3c,0xd2,0xda,0xfe,0xf8,0x07,0xbc,
0x33,0x7c,0x08,0x5e,0xb1,0xff,0x68,0x44,0x75,0xc3,0xdf,0x68,0xdf,0x3d,0xba,0x81,
0x84,0x76,0x06,0x85,0xb4,0xb1,0x7a,0x6d,0xc9,0x4e,0x27,0x38,0x35,0xee,0xe1,0x32,
0x48,0xd8,0x6b,0x76,0xc4,0x9b,0x65,0xb2,0x22,0xf5,0xf6,0x2a,0xa1,0xf4,0x37,0x00,
0x12,0x5b,0x65,0xb2,0x65,0x47,0xc5,0xe4,0xf2,0x13,0x55,0x60,0x87,0x78,0x37,0x5b,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0xeb,0xd8,0xb6,0x33,0x90,0xbf,0x13,0x7f,0xd8,0x00,0xce,0xe0,0x48,0x79,0x78,0xd7,
0xd7,0x62,0xa3,0xe7,0x7b,0xe8,0xc9,0x54,0x71,0x18,0xff,0x2b,0x4f,0xff,0x5e,0x82,
0xca,0x10,0x2c,0x01,0x47,0xc5,0xcc,0xa2,0x22,0x89,0x6b,0xb4,0xc1,0xd6,0x66,0x26,
0x56,0x9b,0x7c,0x02,0x77,0xe0,0xb8,0x38,0x5f,0xac,0x1b,0x9d,0x00,0x27,0x0c,0x33,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0x65,0x19,0xf5,0x62,0x7a,0x92,0xc5,0x4a,0x55,0xcc,0x2f,0xf8,0x6c,0x79,0x34,0x3d,
0xa4,0x9e,0xd3,0xec,0xe4,0x3a,0xf9,0x99,0x5a,0x28,0x2a,0xa0,0x8a,0x46,0xef,0x69,
0x1b,0xd7,0xfd,0xeb,0x60,0x26,0x2a,0x6f,0xa7,0x6c,0x0f,0x97,0x44,0xf4,0x7d,0x9e,
0x5b,0x7d,0xa0,0xe1,0x70,0xe0,0xf3,0x9f,0xb0,0xf3,0xea,0xfd,0xfc,0xac,0x58,0x4d,
0xb8,0x8a,0xa5,0x8f,0x58,0x30,0xb0,0x38,0xa6,0x53,0x84,0x33,0xd4,0xd7,0xbd,0x26,
0x45,0x35,0xb7,0xf6,0x1a,0x20,0x7b,0x8d,0x7e,0x68,0x69,0xdb,0xb1,0x1e,0xa5,0x1a,
0xd5,0xf9,0x42,0x57,0x7a,0xf8,0x30,0x2e,0xea,0x49,0xe5,0xd5,0x34,0x6a,0xcd,0x5b,
0xfa,0x8e,0x71,0x32,0xfa,0x42,0x69,0xec,0x5d,0x50,0x02,0x42,0xc2,0xe4,0xae,0x5a,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
0x9a,0x8e,0xcf,0x66,0xe0,0xd8,0xd4,0x2a,0x0f,0x4d,0xad,0x64,0x8c,0x6d,0xcb,0x37,
0x2e,0x0e,0x9f,0xc4,0xca,0xff,0x85,0xf1,0x7f,0xec,0x71,0x69,0xc2,0x81,0x5c,0xe4,
0x28,0xd2,0x6b,0xa9,0xf6,0xfb,0xe4,0x51,0x68,0x03,0xd0,0xcd,0xc3,0x07,0x6f,0xab,
0x1c,0x94,0x5f,0x94,0xe6,0x54,0x8b,0x1a,0x3c,0xed,0xd4,0xb0,0xf3,0x26,0x81,0xfa,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
 };

// global so we can use them in the filename when we save out the data (debug..)
static UINT8 calc3_decryption_key_byte;
static UINT8 calc3_alternateswaps;
static UINT8 calc3_shift;
static UINT8 calc3_subtracttype;
static UINT8 calc3_mode;
static UINT8 calc3_blocksize_offset;
static UINT16 calc3_dataend;
static UINT16 calc3_database;

static int data_header[2];

static UINT32 calc3_writeaddress;
static UINT32 calc3_writeaddress_current;
static UINT16 calc3_dsw_addr = 0x00;
static UINT16 calc3_eeprom_addr;
static UINT16 calc3_poll_addr;
static UINT16 cakc3_checkumaddress;

static UINT8 shift_bits(UINT8 dat, int bits)
{
	bits &=0x7;

	if (bits==0) return BITSWAP8(dat, 7,6,5,4,3,2,1,0);
	if (bits==1) return BITSWAP8(dat, 6,5,4,3,2,1,0,7);
	if (bits==2) return BITSWAP8(dat, 5,4,3,2,1,0,7,6);
	if (bits==3) return BITSWAP8(dat, 4,3,2,1,0,7,6,5);
	if (bits==4) return BITSWAP8(dat, 3,2,1,0,7,6,5,4);
	if (bits==5) return BITSWAP8(dat, 2,1,0,7,6,5,4,3);
	if (bits==6) return BITSWAP8(dat, 1,0,7,6,5,4,3,2);
	if (bits==7) return BITSWAP8(dat, 0,7,6,5,4,3,2,1);

	return dat;
}

// endian safe? you're having a laugh
static int calc3_decompress_table(int tabnum, UINT8* dstram, int dstoffset)
{
	UINT8* rom = memory_region(REGION_CPU2);
	UINT8 numregions;
	UINT16 length;
	int local_counter=0;
	int x;
	int offset = 0;

	numregions = rom[offset+0];

	if (tabnum > numregions)
	{
		printf("CALC3 error, requested table > num tables!\n");
		return 0;
	}

	rom++;

	// scan through the linked list to find the start of the requested table info
	for (x=0;x<tabnum;x++)
	{
		UINT8 blocksize_offset = rom[offset+0]; // location of the 'block length'
		offset+= blocksize_offset+1;
		length = rom[offset+0] | (rom[offset+1]<<8);
		offset+=length+2;
	}

	// we're at the start of the block, get the info about it
	{
		UINT16 inline_table_base = 0;
		UINT16 inline_table_size = 0;
		calc3_database = offset;
		calc3_blocksize_offset =    rom[offset+0]; // location of the 'block length'
		calc3_mode =                rom[offset+1];
		calc3_alternateswaps =             rom[offset+2];
		calc3_shift = (calc3_alternateswaps &0xf0)>>4;
		calc3_subtracttype = (calc3_alternateswaps &0x03);
		calc3_alternateswaps &= 0x0c;
		calc3_alternateswaps >>=2;

		calc3_decryption_key_byte = rom[offset+3];

		// if blocksize_offset > 3, it appears to specify the encryption table as 'inline' which can be of any size (odd or even) and loops over the bytes to decrypt
		// the decryption key specified seems to be ignored?
		if (calc3_blocksize_offset>3)
		{
			inline_table_base = offset+4;
			inline_table_size = calc3_blocksize_offset-3;
		}

		offset+= calc3_blocksize_offset+1;
		length = rom[offset+0] | (rom[offset+1]<<8);
		offset+=2;

		// copy + decrypt the table to the specified memory area
		//if (dstram)
		{
			int i;

			if (length==0x00)
			{
				// shogwarr does this with 'mode' as 0x08, which probably has some special meaning
				//printf("CALC3: requested 0 length table!\n");
				// -- seems to be 'reset stack' to default for the protection table writes

				// except this will break shogun going into game, must be specific conditions for
				// this, or it can remember addresses and restore those

				// hack, set it to a known address instead of trying to restore to anywhere specific..
				// might break at some point tho, eg if it doesn't write command 06 often enough because it trys to use another one like 07...
				// need to understand the meaning of this (test hw?)

				// !dstram is used because we don't want to process these during our initial table scan, only when the game asks!

				if (calc3_mode==0x06)
				{
					calc3_writeaddress_current = 0x202000; // this is reasoanble for brapboys, not sure about shogun, needs emulating properly!
					//calc3_writeaddress_current = 0x20c000;
				}
				else if (calc3_mode==0x07)
				{
					// also calls empty table with Mode? 07
					// maybe they reset to different points?
				}
				else if (calc3_mode==0x08 && !dstram)
				{
					//printf("save to eeprom\n");

					{
						UINT32 length;//, size;
						UINT8 *dat;

						dat = (UINT8 *)EEPROM_get_data_pointer(&length);

						for (i=0;i<0x80;i++)
						{
							dat[i] = program_read_byte(calc3_eeprom_addr+0x200000+i);
						}

					}

				}
				else if (!dstram)
				{
					//printf("unknown blank table command\n");
				}

				return 0;
			}

			if (inline_table_size)
			{
			// these should be derived from the inline table somehow, probably just a +/- swap like the normal stuff.. maybe
				UINT8 extra[]  = { 0x14,0xf0,0xf8,0xd2,0xbe,0xfc,0xac,0x86,0x64,0x08,0x0c,0x74,0xd6,0x6a,0x24,0x12,0x1a,0x72,0xba,0x48,0x76,0x66,0x4a,0x7c,0x5c,0x82,0x0a,0x86,0x82,0x02,0xe6 };
				UINT8 extra2[] = { 0x2f,0x04,0xd1,0x69,0xad,0xeb,0x10,0x95,0xb0,0x2f,0x0a,0x83,0x7d,0x4e,0x2a,0x07,0x89,0x52,0xca,0x41,0xf1,0x4f,0xaf,0x1c,0x01,0xe9,0x89,0xd2,0xaf,0xcd };

				for (i=0;i<length;i++)
				{
					UINT8 dat=0;


					/* special case for Shogun Warriors table 0x40 */
					if (calc3_subtracttype==3 && calc3_alternateswaps ==0)
					{
						UINT8 inlinet = rom[inline_table_base + (i%inline_table_size)];
						dat = rom[offset+i];

						dat -= inlinet;

						if (((i%inline_table_size)&1)==0)
						{
							dat -= extra[(i%inline_table_size)>>1];
						}
					}
					else
					{
						if ( ((i / inline_table_size)&1)==0)
						{
							if (((i%inline_table_size)&1)==1)
							{
								UINT8 inlinet = rom[inline_table_base + (i%inline_table_size)];
								dat = rom[offset+i];
								dat -= inlinet;
								dat = shift_bits(dat, calc3_shift);
							}
							else
							{

								UINT8 inlinet = rom[inline_table_base + (i%inline_table_size)];
								dat = rom[offset+i];

								if (calc3_subtracttype!=0x02)
								{
									dat -= inlinet;
									dat -= extra[(i%inline_table_size)>>1];
								}
								else
								{
									dat += inlinet;
									dat += extra[(i%inline_table_size)>>1];
								}

								dat = shift_bits(dat, 8-calc3_shift);
							}
						}
						else
						{
							if (((i%inline_table_size)&1)==0)
							{
								UINT8 inlinet = rom[inline_table_base + (i%inline_table_size)];
								dat = rom[offset+i];
								dat -= inlinet;
								dat = shift_bits(dat, calc3_shift);
							}
							else
							{
								dat = rom[offset+i];

								if (calc3_subtracttype!=0x02)
								{
									dat -= extra2[(i%inline_table_size)>>1];
								}
								else
								{
									dat += extra2[(i%inline_table_size)>>1];
								}
								dat = shift_bits(dat, 8-calc3_shift);
							}
						}
					}

					if(local_counter>1)
					{
						//if (space)
						{
							program_write_byte(dstoffset+i, dat);
						}

						// debug, used to output tables at the start
						if (dstram)
						{
							dstram[(dstoffset+i)^1] = dat;
						}
					}
					else
						data_header[local_counter]=dat;

					++local_counter;
				}
			}
			else
			{
				const INT16* key = calc3_keydata+(calc3_decryption_key_byte*0x40);

				if (key[0] == -1)
				{
				//	fatalerror("attempting to use invalid decryption data\n");
				}

				for (i=0;i<length;i++)
				{
					UINT8 dat = rom[offset+i];
					UINT8 keydat = (UINT8)key[i&0x3f];

					{
						if (calc3_subtracttype==0)
						{
							//dat = dat;
						}
						else if (calc3_subtracttype==1)
						{
							if ((i&1)==1) dat += keydat;
							else dat -= keydat;
						}
						else if (calc3_subtracttype==2)
						{
							if ((i&1)==0) dat += keydat;
							else dat -= keydat;
						}
						else if (calc3_subtracttype==3)
						{
							dat -= keydat;
						}

						if (calc3_alternateswaps == 0)
						{
							if ((i&1)==0) dat = shift_bits(dat, 8-calc3_shift);
							else          dat = shift_bits(dat, calc3_shift);
						}
						else if (calc3_alternateswaps==1)
						{
							dat = shift_bits(dat, 8-calc3_shift);
						}
						else if (calc3_alternateswaps==2)
						{
							dat = shift_bits(dat, calc3_shift);
						}
						else if (calc3_alternateswaps==3)
						{
							// same as 0
							if ((i&1)==0) dat = shift_bits(dat, 8-calc3_shift);
							else          dat = shift_bits(dat, calc3_shift);
						}
					}

					if(local_counter>1)
					{
						//if (space)
						{
							program_write_byte(dstoffset+i, dat);
						}

						// debug, used to output tables at the start
						if (dstram)
						{
							dstram[(dstoffset+i)^1] = dat;
						}
					}
					else
						data_header[local_counter]=dat;

					++local_counter;
				}
			}
		}

		calc3_dataend = offset+length+1;
	}

	return length;

}



void calc3_scantables(void)
{
	UINT8* rom = memory_region(REGION_CPU2);
	UINT8 numregions;

	int x;

	calc3_mcu_crc = 0;
	for (x=0;x<0x20000;x++)
	{
		calc3_mcu_crc+=rom[x];
	}

	numregions = rom[0];

	for (x=0;x<numregions;x++)
	{
		UINT8* tmpdstram = (UINT8*)malloc(0x2000);
		int length;
		memset(tmpdstram, 0x00,0x2000);
		length = calc3_decompress_table(x, tmpdstram, 0);

		free(tmpdstram);
	}


}



void calc3_mcu_run(void)
{
	UINT16 mcu_command;
	int i;

	if ( calc3_mcu_status != (1|2|4|8) )	return;

	if (calc3_dsw_addr) program_write_byte(calc3_dsw_addr+0x200000, ( ~readinputport(4))&0xff); // // DSW // dsw actually updates in realtime - mcu reads+writes it every frame


	mcu_command = kaneko16_mcu_ram[calc3_mcu_command_offset/2 + 0];

	if (mcu_command == 0) return;

	if (mcu_command>0)
	{
		/* 0xff is a special 'init' command */
		if (mcu_command == 0xff)
		{

			// clear old command (handshake to main cpu)
			kaneko16_mcu_ram[(calc3_mcu_command_offset>>1)+0] = 0x0000;

			calc3_dsw_addr =           kaneko16_mcu_ram[(0>>1) + 1];
			calc3_eeprom_addr =        kaneko16_mcu_ram[(0>>1) + 2];
			calc3_mcu_command_offset = kaneko16_mcu_ram[(0>>1) + 3];
			calc3_poll_addr =          kaneko16_mcu_ram[(0>>1) + 4];
			cakc3_checkumaddress =     kaneko16_mcu_ram[(0>>1) + 5];
			calc3_writeaddress =      (kaneko16_mcu_ram[(0>>1) + 6] << 16) |
			                          (kaneko16_mcu_ram[(0>>1) + 7]);

			// set our current write / stack pointer to the address specified
			calc3_writeaddress_current = calc3_writeaddress;

			kaneko16_mcu_ram[cakc3_checkumaddress / 2] = calc3_mcu_crc;				// MCU Rom Checksum!

			{
				UINT32 length;//, size;
				UINT8 *dat;

				dat = (UINT8 *)EEPROM_get_data_pointer(&length);

				for (i=0;i<0x80;i++)
				{
					program_write_byte(calc3_eeprom_addr+0x200000+i, dat[i]);
				}

			}

		}
		/* otherwise the command number is the number of transfer operations to perform */
		else
		{
			int num_transfers = mcu_command;
			int i;

			// clear old command (handshake to main cpu)
			kaneko16_mcu_ram[calc3_mcu_command_offset>>1] = 0x0000;;

			for (i=0;i<num_transfers;i++)
			{
				int param1 = kaneko16_mcu_ram[(calc3_mcu_command_offset>>1) + 1 + (2*i)];
				int param2 = kaneko16_mcu_ram[(calc3_mcu_command_offset>>1) + 2 + (2*i)];
				UINT8  commandtabl = (param1&0xff00) >> 8;
				UINT16 commandaddr =param2;// (param1&0x00ff) | (param2&0xff00);
				UINT8  commandunk =  (param1&0x00ff); // brap boys sets.. seems to cause further writebasck address displacement?? (when tested on hw it looked like a simple +, but that doesn't work for brapboys...)
				{
					int length;

					length = calc3_decompress_table(commandtabl, 0, calc3_writeaddress_current-2);

					if (length)
					{
						int write=commandaddr;

						program_write_byte(write+0x200000, data_header[0]);
						program_write_byte(write+0x200001, data_header[1]);

						write=commandaddr+(char)commandunk;
						program_write_word(write+0x200000, (calc3_writeaddress_current>>16)&0xffff);
						program_write_word(write+0x200002,  (calc3_writeaddress_current&0xffff));

						calc3_writeaddress_current += ((length+3)&(~1));
					}

				}
			}
		}
	}
}


/***************************************************************************
                                TOYBOX MCU:

                                Bonk's Adventure
                                Blood Warrior
                                Great 1000 Miles Rally
                                ...
***************************************************************************/
/*
---------------------------------------------------------------------------
                                TOYBOX

94  Bonk's Adventure            TOYBOX?            TBSOP01
94  Blood Warrior               TOYBOX?            TBS0P01 452 9339PK001
94  Great 1000 Miles Rally      TOYBOX                                                  "MM0525-TOYBOX199","USMM0713-TB1994 "
95  Great 1000 Miles Rally 2    TOYBOX      KANEKO TBSOP02 454 9451MK002 (74 pin PQFP)  "USMM0713-TB1994 "
95  Jackie Chan                 TOYBOX                                                  "USMM0713-TB1994 "
95  Gals Panic 3                TOYBOX?            TBSOP01
---------------------------------------------------------------------------

All the considerations are based on the analysis of jchan, and to a fewer extent galpani3, and make references to the current driver sources:

MCU triggering:
---------------

the 4 JCHAN_MCU_COM_W(...) are in fact 2 groups:

AM_RANGE(0x330000, 0x330001) AM_WRITE(jchan_mcu_com0_w) // _[ these 2 are set to 0xFFFF
AM_RANGE(0x340000, 0x340001) AM_WRITE(jchan_mcu_com1_w) //  [ for MCU to execute cmd

AM_RANGE(0x350000, 0x350001) AM_WRITE(jchan_mcu_com2_w) // _[ these 2 are set to 0xFFFF
AM_RANGE(0x360000, 0x360001) AM_WRITE(jchan_mcu_com3_w) //  [ for MCU to return its status


MCU parameters:
---------------

mcu_command = kaneko16_mcu_ram[0x0010/2];    // command nb
mcu_offset  = kaneko16_mcu_ram[0x0012/2]/2;  // offset in shared RAM where MCU will write
mcu_subcmd  = kaneko16_mcu_ram[0x0014/2];    // sub-command parameter, happens only for command #4


    the only MCU commands found in program code are:
    - 0x04: protection: provide data (see below) and code <<<---!!!
    - 0x03: read DSW
    - 0x02: load game settings \ stored in ATMEL AT93C46 chip,
    - 0x42: save game settings / 128 bytes serial EEPROM


Current feeling of devs is that this EEPROM might also play a role in the protection scheme,
but I (SV) feel that it is very unlikely because of the following, which has been verified:
if the checksum test fails at most 3 times, then the initial settings, stored in main68k ROM,
are loaded in RAM then saved with cmd 0x42 (see code @ $5196 & $50d4)
Note that this is valid for jchan only, other games haven't been looked at.

Others:
-------

There is one interesting MCU cmd $4 in jchan:
-> sub-cmd $3d, MCU writes the string "USMM0713-TB1994 "

The very same string is written by gtmr games (gtmre/gtmrusa/gtmr2) but apparently with no sub-cmd: this string is
probably the MCU model string, so this one should be in internal MCU ROM (another one for gtmr is "MM0525-TOYBOX199")

TODO: look at this one since this remark is only driver-based.
*/

static const UINT8 toybox_mcu_decryption_table[0x100] = {
0x7b,0x82,0xf0,0xbc,0x7f,0x1d,0xa2,0xc5,0x2a,0xfa,0x55,0xee,0x1a,0xd0,0x59,0x76,
0x5e,0x75,0x79,0x16,0xa5,0xf6,0x84,0xed,0x0f,0x2e,0xf2,0x36,0x61,0xac,0xcd,0xab,
0x01,0x3b,0x01,0x87,0x73,0xab,0xce,0x5d,0xd4,0x1d,0x68,0x2a,0x35,0xea,0x13,0x27,
0x00,0xaa,0x46,0x36,0x6e,0x65,0x80,0x7e,0x19,0xe2,0x96,0xab,0xac,0xa5,0x6c,0x63,
0x4a,0x6f,0x87,0xf6,0x6a,0xac,0x38,0xe2,0x1f,0x87,0xf9,0xaa,0xf5,0x41,0x60,0xa6,
0x42,0xb9,0x30,0xf2,0xc3,0x1c,0x4e,0x4b,0x08,0x10,0x42,0x32,0xbf,0xb2,0xc5,0x0f,
0x7a,0xab,0x97,0xf6,0xe7,0xb3,0x46,0xf8,0xec,0x2b,0x7d,0x5f,0xb1,0x10,0x03,0xe4,
0x0f,0x22,0xdf,0x8d,0x10,0x66,0xa7,0x7e,0x96,0xbd,0x5a,0xaf,0xaa,0x43,0xdf,0x10,
0x7c,0x04,0xe2,0x9d,0x66,0xd7,0xf0,0x02,0x58,0x8a,0x55,0x17,0x16,0xe2,0xe2,0x52,
0xaf,0xd9,0xf9,0x0d,0x59,0x70,0x86,0x3c,0x05,0xd1,0x52,0xa7,0xf0,0xbf,0x17,0xd0,
0x23,0x15,0xfe,0x23,0xf2,0x80,0x60,0x6f,0x95,0x89,0x67,0x65,0xc9,0x0e,0xfc,0x16,
0xd6,0x8a,0x9f,0x25,0x2c,0x0f,0x2d,0xe4,0x51,0xb2,0xa8,0x18,0x3a,0x5d,0x66,0xa0,
0x9f,0xb0,0x58,0xea,0x78,0x72,0x08,0x6a,0x90,0xb6,0xa4,0xf5,0x08,0x19,0x60,0x4e,
0x92,0xbd,0xf1,0x05,0x67,0x4f,0x24,0x99,0x69,0x1d,0x0c,0x6d,0xe7,0x74,0x88,0x22,
0x2d,0x15,0x7a,0xa2,0x37,0xa9,0xa0,0xb0,0x2c,0xfb,0x27,0xe5,0x4f,0xb6,0xcd,0x75,
0xdc,0x39,0xce,0x6f,0x1f,0xfe,0xcc,0xb5,0xe6,0xda,0xd8,0xee,0x85,0xee,0x2f,0x04,
};


static const UINT8 toybox_mcu_decryption_table_alt[0x100] = {
0x26,0x17,0xb9,0xcf,0x1a,0xf5,0x14,0x1e,0x0c,0x35,0xb3,0x66,0xa0,0x17,0xe9,0xe4,
0x90,0xf6,0xd5,0x35,0xac,0x95,0x49,0x43,0x64,0x0c,0x03,0x75,0x4d,0xda,0xb6,0xdf,
0x06,0xcf,0x83,0x9e,0x35,0x2c,0x71,0x2a,0xab,0xcc,0x65,0xd4,0x1f,0xb0,0x88,0x3c,
0xb7,0x87,0x35,0xc0,0x41,0x65,0x9f,0xa0,0xd5,0x8c,0x3e,0x06,0x53,0xdb,0x45,0x64,
0x09,0x1e,0xc5,0x8d,0x50,0x24,0xe2,0x4a,0x9b,0x99,0x77,0x25,0x43,0xa9,0x1d,0xac,
0x99,0x31,0x75,0xb5,0x53,0xab,0xad,0x5a,0x42,0x14,0xa1,0x52,0xac,0xec,0x5f,0xf8,
0x8c,0x78,0x05,0x47,0xea,0xb8,0xde,0x69,0x98,0x2d,0x8f,0x9d,0xfc,0x05,0xea,0xee,
0x77,0xbb,0xa9,0x31,0x01,0x00,0xea,0xd8,0x9c,0x43,0xb5,0x2f,0x4e,0xb5,0x1b,0xd2,
0x01,0x4b,0xc4,0xf8,0x76,0x92,0x59,0x4f,0x20,0x52,0xd9,0x7f,0xa9,0x19,0xe9,0x7c,
0x8d,0x3b,0xec,0xe0,0x60,0x08,0x2e,0xbd,0x27,0x8b,0xb2,0xfc,0x29,0xd8,0x39,0x8a,
0x4f,0x2f,0x6b,0x04,0x10,0xbd,0xa1,0x04,0xde,0xc0,0xd5,0x0f,0x04,0x86,0xd6,0xd8,
0xfd,0xb1,0x3c,0x4c,0xd1,0xc4,0xf1,0x5b,0xf5,0x8b,0xe3,0xc4,0x89,0x3c,0x39,0x86,
0xd2,0x92,0xc9,0xe5,0x2c,0x4f,0xe2,0x2f,0x2d,0xc5,0x35,0x09,0x94,0x47,0x3c,0x04,
0x40,0x8b,0x57,0x08,0xf6,0x74,0xe9,0xb8,0x36,0x4d,0xc5,0x26,0x13,0x3d,0x75,0xa0,
0xa8,0x29,0x09,0x8c,0x87,0xf7,0x13,0xaf,0x4c,0x38,0x0b,0x8a,0x7f,0x2c,0x62,0x27,
0x47,0xaa,0xda,0x07,0x92,0x8d,0xfd,0x1f,0xee,0x48,0x1a,0x53,0x3b,0x98,0x6a,0x72,
};

// I use a byteswapped MCU data rom to make the transfers to the 68k side easier
//  not sure if it's all 100% endian safe
void decrypt_toybox_rom(void)
{

	UINT8 *src = (UINT8 *)memory_region(REGION_CPU2);

	int i;

	for (i=0;i<0x020000;i++)
	{
		src[i] = src[i] + toybox_mcu_decryption_table[(i^1)&0xff];
	}
}

void decrypt_toybox_rom_alt(void)
{
	UINT8 *src = (UINT8 *)memory_region(REGION_CPU2);

	int i;

	for (i=0;i<0x020000;i++)
	{
		src[i] = src[i] + toybox_mcu_decryption_table_alt[(i^1)&0xff];
	}
}

void toxboy_handle_04_subcommand(UINT8 mcu_subcmd, UINT16*mcu_ram)
{
	UINT8 *src = (UINT8 *)memory_region(REGION_CPU2)+0x10000;
	UINT8* dst = (UINT8 *)mcu_ram;

	int offs = (mcu_subcmd&0x3f)*8;
	int x;

	//UINT16 unused = src[offs+0] | (src[offs+1]<<8);
	UINT16 romstart = src[offs+2] | (src[offs+3]<<8);
	UINT16 romlength = src[offs+4] | (src[offs+5]<<8);
	UINT16 ramdest = mcu_ram[0x0012/2];
	//UINT16 extra = src[offs+6] | (src[offs+7]<<8); // BONK .. important :-(

	//printf("romstart %04x length %04x\n",romstart,romlength);

	for (x=0;x<romlength;x++)
	{
		dst[BYTE_XOR_LE(ramdest+x)] = src[(romstart+x)];
	}
}


void (*toybox_mcu_run)(void);

static UINT16 toybox_mcu_com[4];

void toybox_mcu_init(void)
{
	memset(toybox_mcu_com, 0, 4 * sizeof( UINT16) );
}

#define TOYBOX_MCU_COM_W(_n_)							\
WRITE16_HANDLER( toybox_mcu_com##_n_##_w )				\
{														\
	COMBINE_DATA(&toybox_mcu_com[_n_]);					\
	if (toybox_mcu_com[0] != 0xFFFF)	return;			\
	if (toybox_mcu_com[1] != 0xFFFF)	return;			\
	if (toybox_mcu_com[2] != 0xFFFF)	return;			\
	if (toybox_mcu_com[3] != 0xFFFF)	return;			\
														\
	memset(toybox_mcu_com, 0, 4 * sizeof( UINT16 ) );	\
	toybox_mcu_run();							\
}

TOYBOX_MCU_COM_W(0)
TOYBOX_MCU_COM_W(1)
TOYBOX_MCU_COM_W(2)
TOYBOX_MCU_COM_W(3)

/*
    bonkadv and bloodwar test bit 0
*/
READ16_HANDLER( toybox_mcu_status_r )
{
	return 0; // most games test bit 0 for failure
}


/***************************************************************************
                                Blood Warrior
***************************************************************************/

void bloodwar_mcu_run(void)
{
	UINT16 mcu_command	=	kaneko16_mcu_ram[0x0010/2];
	UINT16 mcu_offset	=	kaneko16_mcu_ram[0x0012/2] / 2;
	UINT16 mcu_data		=	kaneko16_mcu_ram[0x0014/2];

	switch (mcu_command >> 8)
	{
		case 0x02:	// Read from NVRAM
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,0)) != 0)
			{
				mame_fread(f,&kaneko16_mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
		}
		break;

		case 0x42:	// Write to NVRAM
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,1)) != 0)
			{
				mame_fwrite(f,&kaneko16_mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
		}
		break;

		case 0x03:	// DSW
		{
			kaneko16_mcu_ram[mcu_offset] = readinputport(4); //input_port_read(machine, "DSW1");
		}
		break;

		case 0x04:	// Protection
		{
			toxboy_handle_04_subcommand(mcu_data, kaneko16_mcu_ram);

		}
		break;

		default:
		break;
	}
}

/***************************************************************************
                                Bonk's Adventure
***************************************************************************/

void bonkadv_mcu_run(void)
{
	UINT16 mcu_command	=	kaneko16_mcu_ram[0x0010/2];
	UINT16 mcu_offset	=	kaneko16_mcu_ram[0x0012/2] / 2;
	UINT16 mcu_data		=	kaneko16_mcu_ram[0x0014/2];

	switch (mcu_command >> 8)
	{
		case 0x02:	// Read from NVRAM
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,0)) != 0)
			{
				mame_fread(f,&kaneko16_mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
		}
		break;

		case 0x42:	// Write to NVRAM
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,1)) != 0)
			{
				mame_fwrite(f,&kaneko16_mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
		}
		break;

		case 0x43:	// Initialize NVRAM - MCU writes Default Data Set directly to NVRAM
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,1)) != 0)
			{
				mame_fwrite(f, bonkadv_mcu_43, sizeof(bonkadv_mcu_43));
				mame_fclose(f);
			}
		}
		break;

		case 0x03:	// DSW
		{
			kaneko16_mcu_ram[mcu_offset] = readinputport(4); //input_port_read(machine, "DSW1");
		}
		break;

		case 0x04:	// Protection
		{
			switch(mcu_data)
			{
				// static, in this order, at boot/reset - these aren't understood, different params in Mcu data rom, data can't be found
				case 0x34: MCU_RESPONSE(bonkadv_mcu_4_34); break;
				case 0x30: MCU_RESPONSE(bonkadv_mcu_4_30); break;
				case 0x31: MCU_RESPONSE(bonkadv_mcu_4_31); break;
				case 0x32: MCU_RESPONSE(bonkadv_mcu_4_32); break;
				case 0x33: MCU_RESPONSE(bonkadv_mcu_4_33); break;

				// dynamic, per-level (29), in level order
				default:
					toxboy_handle_04_subcommand(mcu_data, kaneko16_mcu_ram);
					break;

			}
		}
		break;

		default:
			//logerror("%s : MCU executed command: %04X %04X %04X (UNKNOWN COMMAND)\n", cpuexec_describe_context(machine), mcu_command, mcu_offset*2, mcu_data);
		break;
	}
}

/***************************************************************************
                            Great 1000 Miles Rally
***************************************************************************/

/*
    MCU Tasks:

    - Write and ID string to shared RAM.
    - Access the EEPROM
    - Read the DSWs
*/

void gtmr_mcu_run(void)
{
	UINT16 mcu_command	=	kaneko16_mcu_ram[0x0010/2];
	UINT16 mcu_offset	=	kaneko16_mcu_ram[0x0012/2] / 2;
	UINT16 mcu_data		=	kaneko16_mcu_ram[0x0014/2];

	logerror("CPU #0 PC %06X : MCU executed command: %04X %04X %04X\n", activecpu_get_pc(), mcu_command, mcu_offset*2, mcu_data);

	switch (mcu_command >> 8)
	{

		case 0x02:	// Read from NVRAM
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,0)) != 0)
			{
				mame_fread(f,&kaneko16_mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
		}
		break;

		case 0x42:	// Write to NVRAM
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name,0,FILETYPE_NVRAM,1)) != 0)
			{
				mame_fwrite(f,&kaneko16_mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
		}
		break;

		case 0x03:	// DSW
		{
			kaneko16_mcu_ram[mcu_offset] = readinputport(4);
		}
		break;
		
		case 0x04:	// TEST (2 versions)
		{
			if (strcmp(Machine->gamedrv->name, "gtmr") == 0 ||
				strcmp(Machine->gamedrv->name, "gtmra") == 0)
			{
				/* MCU writes the string "MM0525-TOYBOX199" to shared ram */
				kaneko16_mcu_ram[mcu_offset+0] = 0x4d4d;
				kaneko16_mcu_ram[mcu_offset+1] = 0x3035;
				kaneko16_mcu_ram[mcu_offset+2] = 0x3235;
				kaneko16_mcu_ram[mcu_offset+3] = 0x2d54;
				kaneko16_mcu_ram[mcu_offset+4] = 0x4f59;
				kaneko16_mcu_ram[mcu_offset+5] = 0x424f;
				kaneko16_mcu_ram[mcu_offset+6] = 0x5831;
				kaneko16_mcu_ram[mcu_offset+7] = 0x3939;
			}
			else
			{
				/* MCU writes the string "USMM0713-TB1994 " to shared ram */
				kaneko16_mcu_ram[mcu_offset+0] = 0x5553;
				kaneko16_mcu_ram[mcu_offset+1] = 0x4d4d;
				kaneko16_mcu_ram[mcu_offset+2] = 0x3037;
				kaneko16_mcu_ram[mcu_offset+3] = 0x3133;
				kaneko16_mcu_ram[mcu_offset+4] = 0x2d54;
				kaneko16_mcu_ram[mcu_offset+5] = 0x4231;
				kaneko16_mcu_ram[mcu_offset+6] = 0x3939;
				kaneko16_mcu_ram[mcu_offset+7] = 0x3420;
			}
		}
		break;
	}

}

