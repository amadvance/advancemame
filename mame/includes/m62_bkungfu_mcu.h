// license:BSD-3-Clause
// copyright-holders:David Haywood, Andrea Bogazzi

#ifndef M62_BKUNGFU_MCU_H
#define M62_BKUNGFU_MCU_H

MACHINE_RESET( bkungfu );
READ8_HANDLER( bkungfu_mailbox_r );
WRITE8_HANDLER( bkungfu_mailbox_w );
WRITE8_HANDLER( bkungfu_tileram_w );

#endif
