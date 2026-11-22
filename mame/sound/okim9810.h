// license:BSD-3-Clause
// copyright-holders:Andrew Gardner
/***************************************************************************

    OKI MSM9810 ADPCM(2) sound chip.

    The master clock frequency for this chip can range from 3.5MHz to 4.5Mhz.
    The typical oscillator is a 4.096Mhz crystal.

***************************************************************************/

#ifndef OKIM9810_H
#define OKIM9810_H

struct OKIM9810interface
{
	int region;		/* memory region where the sample ROM lives */
};

READ8_HANDLER( OKIM9810_status_0_r );
WRITE8_HANDLER( OKIM9810_data_0_w );
WRITE8_HANDLER( OKIM9810_tmp_0_w );

#endif
