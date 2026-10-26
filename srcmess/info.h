// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    info.h

    Dumps the MAME internal data as an XML file.

***************************************************************************/

#ifndef __INFO_H__
#define __INFO_H__

#include "mamecore.h"

/* Print the MAME database in XML format */
void print_mame_xml(FILE* out, int bare, const game_driver* const games[]);

#endif	/* __INFO_H__ */
