/***************************************************************************

    audit.h

    ROM set auditing functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __AUDIT_H__
#define __AUDIT_H__

#include "mamecore.h"

/* return values from audit_verify_roms and audit_verify_samples */
#define CORRECT   		0
#define NOTFOUND  		1
#define INCORRECT 		2
#define CLONE_NOTFOUND	3
#define BEST_AVAILABLE	4
#define MISSING_OPTIONAL	5

/* rom status values for audit_record.status */
#define AUD_ROM_GOOD				0x00000001
#define AUD_ROM_NEED_REDUMP			0x00000002
#define AUD_ROM_NOT_FOUND			0x00000004
#define AUD_NOT_AVAILABLE			0x00000008
#define AUD_BAD_CHECKSUM			0x00000010
#define AUD_MEM_ERROR				0x00000020
#define AUD_LENGTH_MISMATCH			0x00000040
#define AUD_ROM_NEED_DUMP			0x00000080
#define AUD_DISK_GOOD				0x00000100
#define AUD_DISK_NOT_FOUND			0x00000200
#define AUD_DISK_BAD_MD5			0x00000400
#define AUD_OPTIONAL_ROM_NOT_FOUND	0x00000800
#define AUD_DISK_NOT_AVAILABLE		0x00001000
#define AUD_DISK_NEED_DUMP			0x00002000
#define AUD_ROM_NOT_FOUND_PARENT	0x00004000
#define AUD_ROM_NOT_FOUND_BIOS		0x00008000

#define AUD_MAX_ROMS		100	/* maximum roms per driver */
#define AUD_MAX_SAMPLES		200	/* maximum samples per driver */


struct _audit_record
{
	char rom[20];				/* name of rom file */
	unsigned int explength;		/* expected length of rom file */
	unsigned int length;		/* actual length of rom file */
	const char* exphash;        /* expected hash data */
	char hash[256];             /* computed hash informations */
	int status;					/* status of rom file */
};
typedef struct _audit_record audit_record;

struct _missing_sample
{
	char	name[20];		/* name of missing sample file */
};
typedef struct _missing_sample missing_sample;

typedef void (CLIB_DECL *verify_printf_proc)(const char *fmt,...);

int audit_roms(int game, audit_record **audit);
int audit_verify_roms(int game,verify_printf_proc verify_printf);
int audit_samples (int game, missing_sample **audit);
int audit_verify_samples(int game,verify_printf_proc verify_printf);
int audit_is_rom_used (const game_driver *gamedrv, const char* hash);
int audit_has_missing_roms (int game);


#endif	/* __AUDIT_H__ */
