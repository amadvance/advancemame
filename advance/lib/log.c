/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002 Andrea Mazzoleni
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

#include "log.h"
#include "os.h"

#include <stdio.h>

/***************************************************************************/
/* Log */

struct log_context {
	FILE* msg;
	int msg_sync_flag;
};

static struct log_context LOG;

void log_va(const char *text, va_list arg) {
	if (LOG.msg) {
		vfprintf(LOG.msg,text,arg);

		if (LOG.msg_sync_flag) {
			fflush(LOG.msg);
			target_sync();
		}
	}
}

void log_f(const char *text, ...) {
	va_list arg;
	va_start(arg, text);
	log_va(text, arg);
	va_end(arg);
}

int log_init(const char* file, int sync_flag) {

	LOG.msg_sync_flag = sync_flag;
	LOG.msg = 0;

	if (file) {
		LOG.msg = fopen(file,"w");
		if (!LOG.msg)
			return -1;
	}

	return 0;
}

void log_done(void) {
	if (LOG.msg) {
		fclose(LOG.msg);
		LOG.msg = 0;
	}
}

void log_abort(void) {
	if (LOG.msg) {
		fclose(LOG.msg);
		LOG.msg = 0;
	}
}
