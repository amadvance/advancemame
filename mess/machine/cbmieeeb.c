// license:BSD-3-Clause
// copyright-holders:Peter Trauner
/*
 commodore ieee 488 (parallel) bus
*/

#include "driver.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"

#include "includes/cbmdrive.h"
#include "includes/cbmserb.h"

#include "includes/cbmieeeb.h"

/*
 data 8 bit input/output

 all handshake lines low active

 handshake lines:
 dav data valid (asserted by talker)
 nrfd nor ready for data (asserted by listener)
 ndac not data accepted (asserted by listener)

 atn attention
 eoi end of information

 ifc
 ren
 srq serial request master input, other output
 */

static struct {
	struct {
		UINT8 data;
/*		int fic; */
		int dav, nrfd, ndac, atn, eoi, ren;
		int srq;
	} bus[2];
} cbmieee= { { { 0 } } };

void cbm_ieee_open(void)
{
	int i;
	for (i=0; i<2; i++) {
		cbmieee.bus[i].dav=1;
		cbmieee.bus[i].nrfd=1;
		cbmieee.bus[i].ndac=1;
		cbmieee.bus[i].atn=1;
		cbmieee.bus[i].eoi=1;
		cbmieee.bus[i].srq=1;

		cbmieee.bus[i].data=0xff;
	}
}

void cbm_ieee_dav_w(int device, int data)
{
	DBG_LOG(1,"cbm ieee dav",("%.4x dev:%d %d\n", activecpu_get_pc(), device, data));
	cbmieee.bus[device].dav=data;
	if (device==0) c2031_state(cbm_drive);
}

void cbm_ieee_nrfd_w(int device, int data)
{
	DBG_LOG(1,"cbm ieee nrfd",("%.4x dev:%d %d\n", activecpu_get_pc(), device, data));
	cbmieee.bus[device].nrfd=data;
	if (device==0) c2031_state(cbm_drive);
}

void cbm_ieee_ndac_w(int device, int data)
{
	DBG_LOG(1,"cbm ieee ndac",("%.4x dev:%d %d\n", activecpu_get_pc(), device, data));
	cbmieee.bus[device].ndac=data;
	if (device==0) c2031_state(cbm_drive);
}

void cbm_ieee_atn_w(int device, int data)
{
	DBG_LOG(1,"cbm ieee atn",("%.4x dev:%d %d\n", activecpu_get_pc(), device, data));
	cbmieee.bus[device].atn=data;
	if (device==0) c2031_state(cbm_drive);
}

void cbm_ieee_eoi_w(int device, int data)
{
	DBG_LOG(1,"cbm ieee eoi",("%.4x dev:%d %d\n", activecpu_get_pc(), device, data));
	cbmieee.bus[device].eoi=data;
	if (device==0) c2031_state(cbm_drive);
}

void cbm_ieee_data_w(int device, int data)
{
	DBG_LOG(1,"cbm ieee data",("%.4x dev:%d %.2x\n", activecpu_get_pc(), device, data));
	cbmieee.bus[device].data=data;
	if (device==0) c2031_state(cbm_drive);
}

int cbm_ieee_srq_r(void)
{
	int data=cbmieee.bus[1].srq;
	DBG_LOG(1,"cbm ieee srq",("read %d\n", data));
	return data;
}

int cbm_ieee_dav_r(void)
{
	int data=cbmieee.bus[0].dav&&cbmieee.bus[1].dav;
	DBG_LOG(1,"cbm ieee dav",("read %d\n", data));
	return data;
}

int cbm_ieee_nrfd_r(void)
{
	int data=cbmieee.bus[0].nrfd && cbmieee.bus[1].nrfd;
	DBG_LOG(1,"cbm ieee nrfd",("read %d\n", data));
	return data;
}

int cbm_ieee_ndac_r(void)
{
	int data=cbmieee.bus[0].ndac&&cbmieee.bus[1].ndac;
	DBG_LOG(1,"cbm ieee ndac",("read %d\n", data));
	return data;
}

int cbm_ieee_atn_r(void)
{
	int data=cbmieee.bus[0].atn&&cbmieee.bus[1].atn;
	DBG_LOG(1,"cbm ieee atn",("read %d\n", data));
	return data;
}

int cbm_ieee_eoi_r(void)
{
	int data=cbmieee.bus[0].eoi&&cbmieee.bus[1].eoi;
/*	DBG_LOG(1,"cbm ieee eoi",("read %d\n", data)); */
	return data;
}

int cbm_ieee_data_r(void)
{
	int data=cbmieee.bus[0].data&cbmieee.bus[1].data;
	DBG_LOG(1,"cbm ieee data",("read %.2x\n", data));
	return data;
}


 READ8_HANDLER(cbm_ieee_state)
{
	switch (offset) {
	case 0: return cbmieee.bus[0].data;
	case 1: return cbmieee.bus[0].atn;
	case 2: return cbmieee.bus[0].dav;
	case 3: return cbmieee.bus[0].ndac;
	case 4: return cbmieee.bus[0].nrfd;
	case 5: return cbmieee.bus[0].eoi;
	case 6: return cbmieee.bus[0].ren;
	case 7: return cbmieee.bus[0].srq;

	case 0x10: return cbmieee.bus[1].data;
	case 0x11: return cbmieee.bus[1].atn;
	case 0x12: return cbmieee.bus[1].dav;
	case 0x13: return cbmieee.bus[1].ndac;
	case 0x14: return cbmieee.bus[1].nrfd;
	case 0x15: return cbmieee.bus[1].eoi;
	case 0x16: return cbmieee.bus[1].ren;
	case 0x17: return cbmieee.bus[1].srq;
	}
	return 0;
}
