// license:BSD-3-Clause
// copyright-holders:AJR
/**********************************************************************

    Dallas Semiconductor DS2430A 256-Bit 1-Wire EEPROM

    The EEPROM is organized as 32 bytes of storage, backed by a
    volatile scratchpad of identical size. The EEPROM itself is not
    addressible, but may be copied to the scratchpad quickly in its
    entirety by the "read memory" command. Copying the scratchpad to
    EEPROM is also a bulk operation but of course takes longer.

    Each DS2430A also contains a factory-lasered ROM whose contents
    are an 8-bit device code (14h for this type), a unique 48-bit
    serial number and an 8-bit CRC of the preceding 56 bits.

    The memory commands use an 8-bit address for the EEPROM
    scratchpad and the application register. The addresses wrap
    continuously from 1Fh or 07h to 00h for both read and write
    operations.

**********************************************************************/

#include "driver.h"
#include "ds2430a.h"

/* timing constants (in seconds) */
#define T_RSTL		(480e-6)
#define T_PDL		(120e-6)
#define T_PDH		(16e-6)
#define T_SLOT		(60e-6)
#define T_REC		(1e-6)
#define T_LOW0		(60e-6)
#define T_RELEASE	(30e-6)
#define T_COPY		(10e-3)
#define T_RDV		(15e-6)
#define T_SLOT_READ	(T_RDV + T_RELEASE + T_REC)

enum
{
	STATE_PRESENCE,
	STATE_ROM_COMMAND,
	STATE_ROM_READ,
	STATE_ROM_MATCH,
	STATE_ROM_SEARCH,
	STATE_ROM_SEARCH_COMPLEMENT,
	STATE_ROM_SEARCH_WRITE,
	STATE_MEMORY_COMMAND,
	STATE_MEMORY_READ,
	STATE_MEMORY_WRITE,
	STATE_MEMORY_COPY,
	STATE_DONE
};

static double timing_scale;
static mame_timer *slot_timer;

static int data_in;
static int data_out;
static UINT8 shift_data;
static UINT8 command;
static UINT32 bit_count;
static double pulse_start_time;
static int current_state;

static UINT8 rom[8];
static UINT8 eeprom[0x20];
static UINT8 scratchpad[0x20];
static UINT8 app_scratchpad[8];
static UINT8 start_address;

static double scaled_time(double t)
{
	return t * timing_scale;
}

static int set_state(int new_state)
{
	if (current_state != new_state)
	{
		current_state = new_state;
		bit_count = 0;
		return 1;
	}

	return 0;
}

static int next_state(int prev_state, UINT8 cmd, UINT16 index, UINT8 data)
{
	if (prev_state == STATE_ROM_COMMAND)
	{
		switch (cmd)
		{
			case 0x33: /* read ROM */
				return STATE_ROM_READ;

			case 0x55: /* match ROM */
				return STATE_ROM_MATCH;

			case 0xcc: /* skip ROM */
				return STATE_MEMORY_COMMAND;

			case 0xf0: /* search ROM */
				return STATE_ROM_SEARCH;

			default:
				return STATE_DONE;
		}
	}
	else if (prev_state == STATE_ROM_READ && index == 7)
		return STATE_DONE;
	else if ((prev_state == STATE_ROM_MATCH || prev_state == STATE_ROM_SEARCH_WRITE) && index == 7)
		return STATE_MEMORY_COMMAND;
	else if (prev_state == STATE_MEMORY_COMMAND)
	{
		switch (cmd)
		{
			case 0x0f: /* write scratchpad */
			case 0x99: /* write application register */
				if (index == 1)
				{
					start_address = data;
					return STATE_MEMORY_WRITE;
				}
				return STATE_MEMORY_COMMAND;

			case 0x55: /* copy scratchpad */
			case 0x5a: /* copy and lock application register */
				if (index == 1)
				{
					/* validation key is A5h */
					if (data == 0xa5)
						return STATE_MEMORY_COPY;
					return STATE_DONE;
				}
				return STATE_MEMORY_COMMAND;

			case 0x66: /* read status register */
				if (index == 1)
				{
					/* validation key is 00h */
					if (data == 0x00)
						return STATE_MEMORY_READ;
					return STATE_DONE;
				}
				return STATE_MEMORY_COMMAND;

			case 0xf0: /* read memory */
				if (index == 0)
					memcpy(scratchpad, eeprom, sizeof(scratchpad));
				/* fall through */

			case 0xaa: /* read scratchpad */
			case 0xc3: /* read application register */
				if (index == 1)
				{
					start_address = data;
					return STATE_MEMORY_READ;
				}
				return STATE_MEMORY_COMMAND;

			default:
				return STATE_DONE;
		}
	}
	else if (prev_state == STATE_MEMORY_READ && cmd == 0x66)
	{
		/* the status register does not wrap around */
		return STATE_DONE;
	}
	else
	{
		if (prev_state == STATE_MEMORY_WRITE)
		{
			if (cmd == 0x0f)
				scratchpad[(start_address + index) & 0x1f] = data;
			else if (cmd == 0x99)
				app_scratchpad[(start_address + index) & 0x07] = data;
		}
		return prev_state;
	}
}

static UINT8 read_memory(UINT8 cmd, UINT16 index)
{
	switch (cmd)
	{
		case 0xaa: /* read scratchpad */
		case 0xf0: /* read memory */
			return scratchpad[(start_address + index) & 0x1f];

		case 0xc3: /* read application register */
			return app_scratchpad[(start_address + index) & 0x07];

		default:
			return 0xff;
	}
}

static void pulse_start(double time)
{
	if (pulse_start_time > time)
		return;

	pulse_start_time = time;

	switch (current_state)
	{
		case STATE_MEMORY_READ:
		case STATE_ROM_READ:
		case STATE_ROM_SEARCH:
			if ((bit_count & 7) == 0)
			{
				if (current_state == STATE_MEMORY_READ)
					shift_data = read_memory(command, bit_count >> 3);
				else
					shift_data = rom[bit_count >> 3];
			}
			if (!(shift_data & 1))
			{
				data_out = 0;
				timer_adjust(slot_timer, scaled_time(T_RELEASE), 0, 0);
			}
			break;

		case STATE_ROM_MATCH:
			if ((bit_count & 7) == 0)
				shift_data = rom[bit_count >> 3];
			break;

		case STATE_ROM_SEARCH_COMPLEMENT:
			if (shift_data & 1)
			{
				data_out = 0;
				timer_adjust(slot_timer, scaled_time(T_RELEASE), 0, 0);
			}
			break;

		case STATE_MEMORY_COPY:
			/* the copy is interrupted by the bus activity */
			set_state(STATE_DONE);
			break;
	}
}

static void pulse_end(double time)
{
	double pulse_width;

	if (pulse_start_time >= time)
		return;

	/* measure the pulse width */
	pulse_width = (time - pulse_start_time) / timing_scale;

	if (pulse_width >= T_RSTL)
	{
		set_state(STATE_PRESENCE);
		timer_adjust(slot_timer, scaled_time(T_PDH), 0, 0);
		pulse_start_time = time + scaled_time(T_RSTL);
	}
	else if (pulse_width >= T_REC)
	{
		switch (current_state)
		{
			case STATE_ROM_COMMAND:
			case STATE_MEMORY_COMMAND:
			case STATE_MEMORY_WRITE:
				shift_data >>= 1;
				if (pulse_width < T_LOW0)
					shift_data |= 0x80;
				if ((bit_count & 7) == 7)
				{
					int next;

					if ((bit_count >> 3) == 0 && (current_state == STATE_ROM_COMMAND || current_state == STATE_MEMORY_COMMAND))
						command = shift_data;

					next = next_state(current_state, command, bit_count >> 3, shift_data);
					if (set_state(next))
					{
						if (next == STATE_MEMORY_COPY)
						{
							timer_adjust(slot_timer, scaled_time(T_COPY), 0, 0);
							break;
						}
					}
					else
						++bit_count;
				}
				else
					++bit_count;
				pulse_start_time += scaled_time(T_SLOT);
				break;

			case STATE_MEMORY_READ:
			case STATE_ROM_READ:
				shift_data >>= 1;
				if ((bit_count & 7) != 7 || !set_state(next_state(current_state, command, bit_count >> 3, 0)))
					++bit_count;
				pulse_start_time += scaled_time(T_SLOT_READ);
				break;

			case STATE_ROM_SEARCH:
				current_state = STATE_ROM_SEARCH_COMPLEMENT;
				pulse_start_time += scaled_time(T_SLOT_READ);
				break;

			case STATE_ROM_SEARCH_COMPLEMENT:
				current_state = STATE_ROM_SEARCH_WRITE;
				pulse_start_time += scaled_time(T_SLOT_READ);
				break;

			case STATE_ROM_MATCH:
			case STATE_ROM_SEARCH_WRITE:
				if ((pulse_width >= T_LOW0) == (shift_data & 1))
				{
					/* bit not matched, the device is deselected */
					set_state(STATE_DONE);
				}
				else
				{
					shift_data >>= 1;
					if ((bit_count & 7) != 7 || !set_state(next_state(current_state, command, bit_count >> 3, 0)))
					{
						++bit_count;
						if (current_state == STATE_ROM_SEARCH_WRITE)
							current_state = STATE_ROM_SEARCH;
					}
				}
				pulse_start_time += scaled_time(T_SLOT);
				break;
		}
	}
}

static void update_state(int param)
{
	switch (current_state)
	{
		case STATE_PRESENCE:
			data_out = !data_out;
			if (data_out)
				set_state(STATE_ROM_COMMAND);
			else
				timer_adjust(slot_timer, scaled_time(T_PDL), 0, 0);
			break;

		case STATE_MEMORY_READ:
		case STATE_ROM_READ:
		case STATE_ROM_SEARCH:
		case STATE_ROM_SEARCH_COMPLEMENT:
		case STATE_DONE: /* pull up the data line after reading the last bit */
			data_out = 1;
			break;

		case STATE_MEMORY_COPY:
			if (command == 0x55)
				memcpy(eeprom, scratchpad, sizeof(eeprom));
			current_state = STATE_DONE;
			break;
	}
}

void ds2430a_init(double scale, const UINT8 *default_data)
{
	timing_scale = scale;
	slot_timer = timer_alloc(update_state);

	data_in = 1;
	data_out = 1;
	shift_data = 0;
	command = 0;
	bit_count = 0;
	pulse_start_time = 0;
	current_state = STATE_DONE;
	start_address = 0;
	memset(scratchpad, 0, sizeof(scratchpad));
	memset(app_scratchpad, 0, sizeof(app_scratchpad));

	/* the first 32 bytes are the EEPROM, the last 8 bytes the ROM */
	memcpy(eeprom, default_data, 0x20);
	memcpy(rom, default_data + 0x20, 8);
}

int ds2430a_data_r(void)
{
	/* open drain output produces a wired-AND signal */
	return data_in && data_out;
}

void ds2430a_data_w(int state)
{
	/* look for transitions */
	if (data_in && !state)
	{
		data_in = 0;
		pulse_start(timer_get_time());
	}
	else if (!data_in && state)
	{
		data_in = 1;
		pulse_end(timer_get_time());
	}
}
