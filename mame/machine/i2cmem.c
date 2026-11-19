// license:BSD-3-Clause
// copyright-holders:smf
/***************************************************************************

I2C Memory

ram/rom/eeprom/flash on an i2c bus.

inputs:
 sda       serial data
 scl       serial clock

outputs:
 sda       serial data

The memory address is only 8 bits for devices up to 2048 bytes,
devices from 512 to 2048 bytes occupy multiple slave addresses.

Differences from MAME:
- Only the devices up to 2048 bytes, without the address pins and the
  write protect pin, are emulated.

***************************************************************************/

#include "i2cmem.h"

#define STATE_IDLE			0
#define STATE_DEVSEL		1
#define STATE_ADDRESSLOW	3
#define STATE_DATAIN		4
#define STATE_READSELACK	5
#define STATE_DATAOUT		6
#define STATE_RESET			7

#define DEVSEL_RW			0x01
#define DEVSEL_ADDRESS		0xfe

#define SLAVE_ADDRESS		0xa0
#define MAX_SIZE			0x800
#define MAX_PAGE			16

static struct
{
	UINT8 data[MAX_SIZE];
	UINT8 page[MAX_PAGE];
	int data_size;
	int write_page_size;
	int scl;
	int sdaw;
	int sdar;
	int state;
	int bits;
	int shift;
	int devsel;
	int byteaddr;
	int page_offset;
	int page_written_size;
} i2c;

static int address_mask(void)
{
	return i2c.data_size - 1;
}

static int data_offset(void)
{
	return i2c.byteaddr & address_mask();
}

static int select_device(void)
{
	int mask = DEVSEL_ADDRESS & ~(address_mask() >> 7);

	return (i2c.devsel & mask) == (SLAVE_ADDRESS & mask);
}

void i2cmem_init(int data_size, int write_page_size)
{
	memset(i2c.data, 0xff, sizeof(i2c.data));
	i2c.data_size = data_size;
	i2c.write_page_size = write_page_size;
	i2c.scl = 0;
	i2c.sdaw = 0;
	i2c.sdar = 1;
	i2c.state = STATE_IDLE;
	i2c.bits = 0;
	i2c.shift = 0;
	i2c.devsel = 0;
	i2c.byteaddr = 0;
	i2c.page_offset = 0;
	i2c.page_written_size = 0;
}

UINT8 *i2cmem_get_data(void)
{
	return i2c.data;
}

int i2cmem_get_size(void)
{
	return i2c.data_size;
}

void i2cmem_write_sda(int state)
{
	state &= 1;

	if (i2c.sdaw == state)
		return;

	i2c.sdaw = state;

	/* a change of the data line while the clock is high is a start or a stop */
	if (i2c.scl && i2c.sdar)
	{
		if (i2c.sdaw)
		{
			/* the written page is committed only at the stop */
			if (i2c.page_written_size > 0)
			{
				int base = data_offset();
				int root = base & ~(i2c.write_page_size - 1);
				int i;

				for (i = 0; i < i2c.page_written_size; ++i)
					i2c.data[root | ((base + i) & (i2c.write_page_size - 1))] = i2c.page[i];

				i2c.page_written_size = 0;
			}
			i2c.state = STATE_IDLE;
		}
		else
		{
			i2c.state = STATE_DEVSEL;
			i2c.bits = 0;
		}
	}
}

void i2cmem_write_scl(int state)
{
	if (i2c.scl == state)
		return;

	i2c.scl = state;

	switch (i2c.state)
	{
	case STATE_DEVSEL:
	case STATE_ADDRESSLOW:
	case STATE_DATAIN:
		if (i2c.bits < 8)
		{
			if (i2c.scl)
			{
				i2c.shift = ((i2c.shift << 1) | i2c.sdaw) & 0xff;
				++i2c.bits;
			}
		}
		else
		{
			if (i2c.scl)
			{
				++i2c.bits;
			}
			else
			{
				if (i2c.bits == 8)
				{
					switch (i2c.state)
					{
					case STATE_DEVSEL:
						i2c.devsel = i2c.shift;

						if (i2c.devsel == 0)
						{
							i2c.state = STATE_RESET;
						}
						else if (!select_device())
						{
							i2c.state = STATE_IDLE;
						}
						else if ((i2c.devsel & DEVSEL_RW) == 0)
						{
							i2c.state = STATE_ADDRESSLOW;
						}
						else
						{
							i2c.state = STATE_READSELACK;
						}
						break;

					case STATE_ADDRESSLOW:
						i2c.byteaddr = i2c.shift | (((i2c.devsel & DEVSEL_ADDRESS) << 7) & address_mask());
						i2c.page_offset = 0;
						i2c.page_written_size = 0;
						i2c.state = STATE_DATAIN;
						break;

					case STATE_DATAIN:
						i2c.page[i2c.page_offset] = i2c.shift;

						++i2c.page_offset;
						if (i2c.page_offset == i2c.write_page_size)
							i2c.page_offset = 0;
						++i2c.page_written_size;
						if (i2c.page_written_size > i2c.write_page_size)
							i2c.page_written_size = i2c.write_page_size;
						break;
					}

					/* acknowledge */
					if (i2c.state != STATE_IDLE)
						i2c.sdar = 0;
				}
				else
				{
					i2c.bits = 0;
					i2c.sdar = 1;
				}
			}
		}
		break;

	case STATE_READSELACK:
		i2c.bits = 0;
		i2c.state = STATE_DATAOUT;
		break;

	case STATE_DATAOUT:
		if (i2c.bits < 8)
		{
			if (i2c.scl)
			{
				++i2c.bits;
			}
			else
			{
				if (i2c.bits == 0)
				{
					i2c.shift = i2c.data[data_offset()];
					i2c.byteaddr = (i2c.byteaddr + 1) & address_mask();
				}

				i2c.sdar = (i2c.shift >> 7) & 1;
				i2c.shift = (i2c.shift << 1) & 0xff;
			}
		}
		else
		{
			if (i2c.scl)
			{
				/* the reader doesn't acknowledge the last byte */
				if (i2c.sdaw)
					i2c.state = STATE_IDLE;

				i2c.bits = 0;
			}
			else
			{
				i2c.sdar = 1;
			}
		}
		break;

	case STATE_RESET:
		if (i2c.scl)
		{
			if (i2c.bits > 8)
			{
				i2c.state = STATE_IDLE;
				i2c.sdar = 1;
			}
			++i2c.bits;
		}
		break;
	}
}

int i2cmem_read_sda(void)
{
	if (i2c.state == STATE_IDLE)
		return i2c.sdaw & 1;

	return i2c.sdar & 1;
}
