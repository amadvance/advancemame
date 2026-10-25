// license:BSD-3-Clause
// copyright-holders:R. Belmont
/***************************************************************************

    CDRDAO TOC parser for CHD compression frontend

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.

***************************************************************************/

#ifndef __CHDCD_H__
#define __CHDCD_H__

#include "cdrom.h"

int cdrom_parse_toc(char *tocfname, cdrom_toc *outtoc, cdrom_track_input_info *outinfo);

#endif	/* __CHDCD_H__ */
