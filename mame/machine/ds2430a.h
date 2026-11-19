// license:BSD-3-Clause
// copyright-holders:AJR
/**********************************************************************

    Dallas Semiconductor DS2430A 256-Bit 1-Wire EEPROM

**********************************************************************/

#ifndef DS2430A_H
#define DS2430A_H

/* timing_scale adjusts the 1-Wire timings to the speed of the host CPU */
void ds2430a_init(double timing_scale, const UINT8 *default_data);

/* serial data line handlers */
int ds2430a_data_r(void);
void ds2430a_data_w(int state);

#endif
