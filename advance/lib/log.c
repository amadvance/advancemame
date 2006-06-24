/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Andrea Mazzoleni
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

#include "portable.h"

#include "log.h"
#include "target.h"
#include "snstring.h"

/***************************************************************************/
/* Log */

#define LOG_BUFFER_SIZE 1024

struct log_context {
	FILE* msg;
	adv_bool msg_sync_flag;

	char buffer[LOG_BUFFER_SIZE];
	unsigned buffer_count;
};

static struct log_context LOG;

/**
 * Print a modeline with blanking information in the log file.
 * This function must not be called directly. One of the log_* macro must be used. 
 */
void log_f_modeline_cb(const char* text, unsigned pixel_clock, unsigned hde, unsigned hbs, unsigned hrs, unsigned hre, unsigned hbe, unsigned ht, unsigned vde, unsigned vbs, unsigned vrs, unsigned vre, unsigned vbe, unsigned vt, adv_bool hsync_pol, adv_bool vsync_pol, adv_bool doublescan, adv_bool interlace)
{
	const char* flag1 = hsync_pol ? " -hsync" : " +hsync";
	const char* flag2 = vsync_pol ? " -vsync" : " +vsync";
	const char* flag3 = doublescan ? " doublescan" : "";
	const char* flag4 = interlace ? " interlace" : "";
	log_f("%s %g %d %d %d %d %d %d %d %d %d %d %d %d%s%s%s%s\n",
		text, (double)pixel_clock / 1E6,
		hde, hbs, hrs, hre, hbe, ht,
		vde, vbs, vrs, vre, vbe, vt,
		flag1, flag2, flag3, flag4
	);
}

/**
 * Print a modeline in the log file.
 * This function must not be called directly. One of the log_* macro must be used.
 */
void log_f_modeline_c(const char* text, unsigned pixel_clock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, adv_bool hsync_pol, adv_bool vsync_pol, adv_bool doublescan, adv_bool interlace)
{
	const char* flag1 = hsync_pol ? " -hsync" : " +hsync";
	const char* flag2 = vsync_pol ? " -vsync" : " +vsync";
	const char* flag3 = doublescan ? " doublescan" : "";
	const char* flag4 = interlace ? " interlace" : "";
	log_f("%s %g %d %d %d %d %d %d %d %d%s%s%s%s\n",
		text, (double)pixel_clock / 1E6,
		hde, hrs, hre, ht,
		vde, vrs, vre, vt,
		flag1, flag2, flag3, flag4
	);
}

/**
 * Print something with the printf format in the log file.
 * This function must not be called directly. One of the log_* macro must be used.
 */
void log_f(const char* text, ...)
{
	va_list arg;
	va_start(arg, text);
	log_va(text, arg);
	va_end(arg);
}

/**
 * Print something with the printv format in the log file.
 * This function must not be called directly. One of the log_* macro must be used.
 */
void log_va(const char* text, va_list arg)
{
	if (LOG.msg) {
		char buffer[LOG_BUFFER_SIZE];

		vsnprintf(buffer, sizeof(buffer) - 1, text, arg);
		buffer[LOG_BUFFER_SIZE - 1] = 0;

		if (strcmp(buffer, LOG.buffer) == 0) {
			++LOG.buffer_count;
		} else {
			if (LOG.buffer_count >= 5) {
				fprintf(LOG.msg, "log: last message repeated %d times\n", LOG.buffer_count);
			}
			LOG.buffer_count = 0;
			sncpy(LOG.buffer, sizeof(LOG.buffer), buffer);
		}

		if (LOG.buffer_count < 5) {
			/* note that "arg" cannot be reused after the vsnprintf call */
			fprintf(LOG.msg, "%s", buffer);

			if (LOG.msg_sync_flag) {
				fflush(LOG.msg);
				target_sync();
			}
		}
	}
}

/** 
 * Initialize the log system.
 * \param file Log file. The file is overwritten.
 * \param sync_flag If set the file and the filesystem is flushed at every write. The function target_sync() is called.
 */
adv_error log_init(const char* file, adv_bool sync_flag)
{
	LOG.msg_sync_flag = sync_flag;
	LOG.msg = 0;
	LOG.buffer_count = 0;
	LOG.buffer[0] = 0;

	if (file) {
		LOG.msg = fopen(file, "w");
		if (!LOG.msg)
			return -1;
	}

	return 0;
}

/** 
 * Deinitialize the log system.
 */
void log_done(void)
{
	if (LOG.msg) {
		fclose(LOG.msg);
		LOG.msg = 0;
	}
}

/**
 * Return the log file handle.
 * \return 0 if the log file is closed.
 */
FILE* log_handle(void)
{
	return LOG.msg;
}

/**
 * Abort the logging.
 * This function ensure that the log file is flushed. 
 * It can be called in a signal handler in any condition.
 */
void log_abort(void)
{
	if (LOG.msg) {
		fclose(LOG.msg);
		LOG.msg = 0;
	}
}

