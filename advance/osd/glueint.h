/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#ifndef __GLUEINT_H
#define __GLUEINT_H

#include "glue.h"

#if defined(MESS)

#include "../../srcmess/mamecore.h"
#include "../../srcmess/mame.h"
#include "../../srcmess/driver.h"
#include "../../srcmess/info.h"
#include "../../srcmess/osdepend.h"
#include "../../srcmess/ui_text.h"
#include "../../srcmess/profiler.h"

#else

#include "../../src/mamecore.h"
#include "../../src/mame.h"
#include "../../src/driver.h"
#include "../../src/info.h"
#include "../../src/osdepend.h"
#include "../../src/ui_text.h"
#include "../../src/profiler.h"

#endif

#define IPT_UI_HELP IPT_OSD_1
#define IPT_UI_TURBO IPT_OSD_2
#define IPT_UI_COCKTAIL IPT_OSD_3
#define IPT_UI_STARTUP_END IPT_OSD_4
#define IPT_UI_MODE_NEXT IPT_OSD_5
#define IPT_UI_MODE_PRED IPT_OSD_6
#define IPT_UI_RECORD_START IPT_OSD_7
#define IPT_UI_RECORD_STOP IPT_OSD_8

input_seq* glue_portdef_seq_get(input_port_default_entry* port, int seqtype);
input_seq* glue_port_seq_get(input_port_entry* port, int seqtype);
input_seq* glue_portdef_seqeval_get(input_port_default_entry* port, int seqtype);
input_seq* glue_port_seqeval_get(input_port_entry* port, int seqtype);

#endif

