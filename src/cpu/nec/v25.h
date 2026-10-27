// license:BSD-3-Clause
// copyright-holders:Bryan McPhail, Alex W. Jackson
/*****************************************************************************

    NEC V25 (and V25+) CPU, see v25.c

*****************************************************************************/

#ifndef V25_H
#define V25_H

#include "cpuintrf.h"

/* input lines, besides 0 (INT) and IRQ_LINE_NMI */
#define V25_INPUT_LINE_INTP0	10
#define V25_INPUT_LINE_INTP1	11
#define V25_INPUT_LINE_INTP2	12
#define V25_INPUT_LINE_POLL		20

enum
{
	V25_PC=1, V25_IP, V25_AW, V25_CW, V25_DW, V25_BW, V25_SP, V25_BP, V25_IX, V25_IY,
	V25_DS1, V25_PS, V25_SS, V25_DS0, V25_PSW, V25_IDB, V25_PENDING
};

extern void v25_get_info(UINT32 state, union cpuinfo *info);

/* Configuration, from DRIVER_INIT (before the CPU is reset).
   The opcodes are decrypted with the table when the CPU is in secure
   mode (the default when there is a table). */
extern void v25_set_decryption_table(int cpunum, const UINT8 *table);

/* the port pins: PT (input only), P0, P1 (upper 4 bits), P2 */
extern void v25_set_port_handlers(int cpunum,
	read8_handler pt_in, read8_handler p0_in, read8_handler p1_in, read8_handler p2_in,
	write8_handler p0_out, write8_handler p1_out, write8_handler p2_out);

#endif
