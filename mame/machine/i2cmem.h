// license:BSD-3-Clause
// copyright-holders:smf
/***************************************************************************

    i2cmem.h

    I2C memory

***************************************************************************/

#ifndef I2CMEM_H
#define I2CMEM_H

#include "driver.h"

/* the size is in bytes and the page size is the one used in the writes */
void i2cmem_init(int data_size, int write_page_size);
void i2cmem_write_sda(int state);
void i2cmem_write_scl(int state);
int i2cmem_read_sda(void);

/* the content of the memory, for the nvram handlers */
UINT8 *i2cmem_get_data(void);
int i2cmem_get_size(void);

#endif
