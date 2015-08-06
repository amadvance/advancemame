/****************************************************************************

    eainfo.h

    Deprecated code that is wedged into a few of the MAME disassemblers.
    Broken out here for eventual removal.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************/

#include "cpuintrf.h"

/* What EA address to set with debug_ea_info (origin) */
enum {
    EA_DST,
    EA_SRC
};

/* Size of the data element accessed (or the immediate value) */
enum {
    EA_DEFAULT,
    EA_INT8,
    EA_UINT8,
    EA_INT16,
    EA_UINT16,
    EA_INT32,
    EA_UINT32,
    EA_SIZE
};

/* Access modes for effective addresses to debug_ea_info */
enum {
    EA_NONE,        /* no EA mode */
    EA_VALUE,       /* immediate value */
    EA_ABS_PC,      /* change PC absolute (JMP or CALL type opcodes) */
    EA_REL_PC,      /* change PC relative (BRA or JR type opcodes) */
	EA_ZPG_RD,		/* read zero page memory */
	EA_ZPG_WR,		/* write zero page memory */
	EA_ZPG_RDWR,	/* read then write zero page memory */
    EA_MEM_RD,      /* read memory */
    EA_MEM_WR,      /* write memory */
    EA_MEM_RDWR,    /* read then write memory */
	EA_PORT_RD,     /* read i/o port */
	EA_PORT_WR,     /* write i/o port */
	EA_COUNT
};

static const char *set_ea_info(int what, unsigned value, int size, int access)
{
	static char buffer[8][63+1];
	static int which = 0;
	const char *sign = "";
	unsigned width, result;

	which = (which + 1) % 8;

	if( access == EA_REL_PC )
		/* PC relative calls set_ea_info with value = PC and size = offset */
		result = value + size;
	else
		result = value;

	switch( access )
	{
		case EA_VALUE:	/* Immediate value */
			switch( size )
			{
				case EA_INT8:
				case EA_UINT8:
					width = 2;
					break;
				case EA_INT16:
				case EA_UINT16:
					width = 4;
					break;
				case EA_INT32:
				case EA_UINT32:
					width = 8;
					break;
				default:
					return "set_ea_info: invalid <size>!";
			}

			switch( size )
			{
				case EA_INT8:
				case EA_INT16:
				case EA_INT32:
					if( result & (1 << ((width * 4) - 1)) )
					{
						sign = "-";
						result = (unsigned)-result;
					}
					break;
			}

			if (width < 8)
				result &= (1 << (width * 4)) - 1;
			break;

		case EA_ZPG_RD:
		case EA_ZPG_WR:
		case EA_ZPG_RDWR:
			result &= 0xff;
			width = 2;
			break;

		case EA_ABS_PC: /* Absolute program counter change */
			result &= (active_address_space[ADDRESS_SPACE_PROGRAM].addrmask | 3);
			if( size == EA_INT8 || size == EA_UINT8 )
				width = 2;
			else
			if( size == EA_INT16 || size == EA_UINT16 )
				width = 4;
			else
			if( size == EA_INT32 || size == EA_UINT32 )
				width = 8;
			else
				width = (activecpu_addrbus_width(ADDRESS_SPACE_PROGRAM) + 3) / 4;
			break;

		case EA_REL_PC: /* Relative program counter change */
		default:
			result &= (active_address_space[ADDRESS_SPACE_PROGRAM].addrmask | 3);
			width = (activecpu_addrbus_width(ADDRESS_SPACE_PROGRAM) + 3) / 4;
	}
	sprintf( buffer[which], "%s$%0*X", sign, width, result );
	return buffer[which];
}

