/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2002 Andrea Mazzoleni
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

#include "emu.h"
#include "hscript.h"
#include "conf.h"
#include "video.h"
#include "blit.h"
#include "os.h"
#include "fuzzy.h"
#include "log.h"
#include "target.h"

#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef __MSDOS__
#include <dpmi.h> /* for _go32_dpmi_remaining_virtual_memory() */
#endif

struct advance_context CONTEXT;

/***************************************************************************/
/* Log */

void video_log_va(const char *text, va_list arg)
{
	log_va(text,arg);
}

/***************************************************************************/
/* Signal */

void os_signal(int signum)
{
	hardware_script_abort();
	os_default_signal(signum);
}

/***************************************************************************/
/* Select */

struct game_fuzzy {
	unsigned index;
	unsigned fuzzy;
};

static int game_fuzzy_cmp(const void* a, const void* b) {
	const struct game_fuzzy* A = (const struct game_fuzzy*)a;
	const struct game_fuzzy* B = (const struct game_fuzzy*)b;
	if (A->fuzzy < B->fuzzy)
		return -1;
	if (A->fuzzy > B->fuzzy)
		return 1;
	return 0;
}

static const mame_game* select_game(const char* gamename) {
	int game_count = 0;
	struct game_fuzzy* game_map;
	unsigned i;
	int limit;

	for(i=0;mame_game_at(i);++i) {
		if (strcmp(gamename,mame_game_name(mame_game_at(i))) == 0) {
			return mame_game_at(i);
		}
		++game_count;
	}

	game_map = malloc(game_count*sizeof(struct game_fuzzy));

	target_err("Game \"%s\" isn't supported.\n", gamename);

	limit = (strlen(gamename) - 6) * FUZZY_UNIT_A;
	if (limit < 4*FUZZY_UNIT_A)
		limit = 4*FUZZY_UNIT_A;

	for(i=0;i<game_count;++i) {
		const mame_game* game = mame_game_at(i);
		int fuzzy_short = fuzzy(gamename,mame_game_name(game),limit);
		int fuzzy_long = fuzzy(gamename,mame_game_description(game),limit);
		int fuzzy = fuzzy_short < fuzzy_long ? fuzzy_short : fuzzy_long;

		if (limit > fuzzy + 3*FUZZY_UNIT_A)
			limit = fuzzy + 3*FUZZY_UNIT_A; /* limit the error range */

		game_map[i].index = i;
		game_map[i].fuzzy = fuzzy;
	}

	qsort(game_map,game_count,sizeof(struct game_fuzzy),game_fuzzy_cmp);

	if (game_map[0].fuzzy < limit) {
		target_err("\nSimilar are:\n");
		for (i=0;i<15 && i<game_count;++i) {
			if (game_map[i].fuzzy < limit) {
				const mame_game* game = mame_game_at(game_map[i].index);
				target_err("%10s %s\n",mame_game_name(game),mame_game_description(game));
			}
		}
	}

	free(game_map);
	return 0;
}

/***************************************************************************/
/* Main */

#ifdef __MSDOS__
/* LEGACY (to be removed) */
static struct conf_conv CONV[] = {
{ "config", "steadykey", "*", "", "input_%s", "%s", 0 },
{ "config", "idleexit", "*", "", "input_%s", "%s", 0 },
{ "config", "safeexit", "*", "", "input_%s", "%s", 0 },
{ "config", "joystick", "yes", "", "device_%s", "auto", 0 },
{ "config", "joystick", "no", "", "device_%s", "none", 0 },
{ "config", "joystick", "*", "", "device_%s", "%s", 0 },
{ "config", "mouse", "yes", "", "device_%s", "auto", 0 },
{ "config", "mouse", "no", "", "device_%s", "none", 0 },
{ "config", "mouse", "*", "", "device_%s", "%s", 0 },
{ "config", "recordtime", "*", "", "sound_%s", "%s", 0 },
{ "config", "stereo", "*", "", "sound_%s", "%s", 0 },
{ "config", "volume", "*", "", "sound_%s", "%s", 0 },
{ "config", "soundcard", "yes", "", "device_sound", "auto", 0 },
{ "config", "soundcard", "-1", "", "device_sound", "auto", 0 },
{ "config", "soundcard", "no", "", "device_sound", "none", 0 },
{ "config", "soundcard", "0", "", "device_sound", "none", 0 },
{ "config", "soundcard", "1", "", "device_sound", "seal/sb", 0 },
{ "config", "soundcard", "3", "", "device_sound", "seal/pas", 0 },
{ "config", "soundcard", "4", "", "device_sound", "seal/gusmax", 0 },
{ "config", "soundcard", "5", "", "device_sound", "seal/gus", 0 },
{ "config", "soundcard", "6", "", "device_sound", "seal/wss", 0 },
{ "config", "soundcard", "7", "", "device_sound", "seal/ess", 0 },
{ "config", "soundcard", "sb", "", "device_sound", "seal/sb", 0 },
{ "config", "soundcard", "pas", "", "device_sound", "seal/pas", 0 },
{ "config", "soundcard", "gusmax", "", "device_sound", "seal/gusmax", 0 },
{ "config", "soundcard", "gus", "", "device_sound", "seal/gus", 0 },
{ "config", "soundcard", "wss", "", "device_sound", "seal/wss", 0 },
{ "config", "soundcard", "ess", "", "device_sound", "seal/ess", 0 },
{ "config", "soundcard", "*", "", "device_sound", "%s", 0 },
{ "directory", "rompath", "*", "", "dir_rom", "%s", 0 },
{ "directory", "samplepath", "*", "", "dir_sample", "%s", 0 },
{ "directory", "nvram", "*", "", "dir_%s", "%s", 0 },
{ "directory", "hi", "*", "", "dir_%s", "%s", 0 },
{ "directory", "cfg", "*", "", "dir_%s", "%s", 0 },
{ "directory", "inp", "*", "", "dir_%s", "%s", 0 },
{ "directory", "sta", "*", "", "dir_%s", "%s", 0 },
{ "directory", "artwork", "*", "", "dir_%s", "%s", 0 },
{ "directory", "snap", "*", "", "dir_%s", "%s", 0 },
{ "script", "video", "*", "", "script_%s", "%s", 0 },
{ "script", "emulation", "*", "", "script_%s", "%s", 0 },
{ "script", "play", "*", "", "script_%s", "%s", 0 },
{ "script", "led1", "*", "", "script_led[1]", "%s", 0 },
{ "script", "led2", "*", "", "script_led[2]", "%s", 0 },
{ "script", "led3", "*", "", "script_led[3]", "%s", 0 },
{ "script", "coin1", "*", "", "script_coin[1]", "%s", 0 },
{ "script", "coin2", "*", "", "script_coin[2]", "%s", 0 },
{ "script", "coin3", "*", "", "script_coin[3]", "%s", 0 },
{ "script", "coin4", "*", "", "script_coin[4]", "%s", 0 },
{ "script", "start1", "*", "", "script_start[1]", "%s", 0 },
{ "script", "start2", "*", "", "script_start[2]", "%s", 0 },
{ "script", "start3", "*", "", "script_start[3]", "%s", 0 },
{ "script", "start4", "*", "", "script_start[4]", "%s", 0 },
{ "script", "turbo", "*", "", "script_%s", "%s", 0 },
{ "config", "artwork", "*", "", "display_%s", "%s", 0 },
{ "config", "samples", "*", "", "sound_%s", "%s", 0 },
{ "config", "antialias", "*", "", "display_%s", "%s", 0 },
{ "config", "traslucency", "*", "", "display_%s", "%s", 0 },
{ "config", "depth", "*", "", "misc_internaldepth", "%s", 0 },
{ "config", "beam", "*", "", "display_%s", "%s", 0 },
{ "config", "flicker", "*", "", "display_%s", "%s", 0 },
{ "config", "ror", "*", "", "display_%s", "%s", 0 },
{ "config", "rol", "*", "", "display_%s", "%s", 0 },
{ "config", "flipx", "*", "", "display_%s", "%s", 0 },
{ "config", "flipy", "*", "", "display_%s", "%s", 0 },
{ "config", "samplerate", "*", "", "sound_%s", "%s", 0 },
{ "config", "resamplefilter", "*", "", "sound_%s", "%s", 0 },
{ "config", "cheat", "*", "", "misc_%s", "%s", 0 },
{ "config", "language", "*", "", "misc_%s", "%s", 0 },
{ "config", "cheatfile", "*", "", "misc_%s", "%s", 0 },
{ "config", "historyfile", "*", "", "misc_%s", "%s", 0 },
{ "config", "mameinfofile", "*", "", "misc_%s", "%s", 0 },
{ "config", "scanlines", "*", "", "display_%s", "%s", 0 },
{ "config", "vsync", "*", "", "display_%s", "%s", 0 },
{ "config", "waitvsync", "*", "", "display_%s", "%s", 0 },
{ "config", "triplebuffer", "*", "", "display_buffer", "%s", 0 },
{ "config", "resize", "no", "", "display_%s", "none", 0 },
{ "config", "resize", "*", "", "display_%s", "%s", 0 },
{ "config", "magnify", "*", "", "display_%s", "%s", 0 },
{ "config", "rgb", "*", "", "display_%s", "%s", 0 },
{ "config", "videogenerate", "no", "", "display_adjust", "none", 0 },
{ "config", "videogenerate", "generate", "", "display_adjust", "generate", 0 },
{ "config", "videogenerate", "adjustx", "", "display_adjust", "x", 0 },
{ "config", "skiplines", "*", "", "display_%s", "%s", 0 },
{ "config", "skipcolumns", "*", "", "display_%s", "%s", 0 },
{ "config", "gamma", "*", "", "display_%s", "%s", 0 },
{ "config", "brightness", "*", "", "display_%s", "%s", 0 },
{ "config", "frameskip", "*", "", "display_%s", "%s", 0 },
{ "config", "blitrotate", "auto", "", "display_rotate", "%s", 0 },
{ "config", "blitrotate", "yes", "", "display_rotate", "blit", 0 },
{ "config", "blitrotate", "no", "", "display_rotate", "core", 0 },
{ "config", "bliteffect", "no", "", "display_resizeeffect", "none", 0 },
{ "config", "bliteffect", "*", "", "display_resizeeffect", "%s", 0 },
{ "config", "rgbeffect", "no", "", "display_%s", "none", 0 },
{ "config", "rgbeffect", "*", "", "display_%s", "%s", 0 },
{ "config", "turbospeed", "*", "", "misc_turbospeed", "%s", 0 },
{ "config", "turbostartuptime", "*", "", "misc_startuptime", "%s", 0 },
{ "config", "video", "*", "", "display_mode", "%s", 0 },
{ "config", "videodepth", "*", "", "display_depth", "%s", 0 },
{ "config", "videomodereset", "*", "", "display_restore", "%s", 0 },
{ "config", "expand", "*", "", "display_%s", "%s", 0 },
{ "video", "pclock", "*", "", "device_video_%s", "%s", 0 },
{ "video", "hclock", "*", "", "device_video_%s", "%s", 0 },
{ "video", "vclock", "*", "", "device_video_%s", "%s", 0 },
{ "video", "format0", "*", "", "device_video_format", "%s", 0 },
{ "video", "format1", "*", "", "device_video_format", "%s", 0 },
{ "video", "format2", "*", "", "device_video_format", "%s", 0 },
{ "video", "format3", "*", "", "device_video_format", "%s", 0 },
{ "video", "format4", "*", "", "device_video_format", "%s", 0 },
{ "video", "format5", "*", "", "device_video_format", "%s", 0 },
{ "video", "format6", "*", "", "device_video_format", "%s", 0 },
{ "video", "format7", "*", "", "device_video_format", "%s", 0 },
{ "video", "modeline0", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline1", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline2", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline3", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline4", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline5", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline6", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline7", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline8", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline9", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline10", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline11", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline12", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline13", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline14", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline15", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline16", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline17", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline18", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline19", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline20", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline21", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline22", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline23", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline24", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline25", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline26", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline27", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline28", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline29", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline30", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline31", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline32", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline33", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline34", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline35", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline36", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline37", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline38", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline39", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline40", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline41", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline42", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline43", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline44", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline45", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline46", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline47", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline48", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline49", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline50", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline51", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline52", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline53", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline54", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline55", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline56", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline57", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline58", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline59", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline60", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline61", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline62", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline63", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline64", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline65", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline66", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline67", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline68", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline69", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline70", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline71", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline72", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline73", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline74", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline75", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline76", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline77", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline78", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline79", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline80", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline81", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline82", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline83", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline84", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline85", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline86", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline87", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline88", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline89", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline90", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline91", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline92", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline93", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline94", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline95", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline96", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline97", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline98", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline99", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline100", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline101", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline102", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline103", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline104", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline105", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline106", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline107", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline108", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline109", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline110", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline111", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline112", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline113", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline114", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline115", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline116", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline117", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline118", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline119", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline120", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline121", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline122", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline123", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline124", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline125", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline126", "*", "", "device_video_modeline", "%s", 0 },
{ "video", "modeline127", "*", "", "device_video_modeline", "%s", 0 },
/* global */
{ "video", "*", "*", "", "device_%s", "%s", 0 },
{ "joystick", "*", "*", "", "allegro_%s", "%s", 1 }
};
#endif

static struct conf_conv STANDARD[] = {
#ifdef __MSDOS__
{ "", "allegro_*", "*", "%s", "%s", "%s", 1 }, /* auto registration of the Allegro options */
#endif
/* 0.57.1 */
{ "*", "misc_mameinfofile", "*", "%s", "misc_infofile", "%s", 0 }, /* rename */
{ "*", "sound_recordtime", "*", "%s", "record_sound_time", "%s", 0 }, /* rename */
/* 0.61.0 */
{ "*", "input_analog[*]", "*", "", "", "", 0 }, /* ignore */
{ "*", "input_track[*]", "*", "", "", "", 0 }, /* ignore */
/* 0.61.1 */
{ "*", "input_map[*,track]", "*", "", "", "", 0 }, /* ignore */
/* 0.61.2 */
{ "*", "misc_language", "*", "%s", "misc_languagefile", "%s", 0 }, /* rename */
{ "*", "input_safeexit", "*", "%s", "misc_safequit", "%s", 0 }, /* rename */
{ "*", "device_joystick", "standard", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "dual", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "4button", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "6button", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "8button", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "fspro", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "wingex", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "sidewinder", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "sidewinderag", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "gamepadpro", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "grip", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "grip4", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "sneslpt1", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "sneslpt2", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "sneslpt3", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "psxlpt1", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "psxlpt2", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "psxlpt3", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "n64lpt1", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "n64lpt2", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "n64lpt3", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "db9lpt1", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "db9lp2", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "db9lp3", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "tgxlpt1", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "tgxlpt2", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "tgxlpt3", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "segaisa", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "segapci", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "segapcifast", "%s", "%s", "allegro/%s", 0 }, /* rename */
{ "*", "device_joystick", "wingwarrior", "%s", "%s", "allegro/%s", 0 }, /* rename */
/* 0.61.3 */
{ "*", "display_waitvsync", "*", "", "", "", 0 } /* ignore */
};

static void error_callback(void* context, enum conf_callback_error error, const char* file, const char* tag, const char* valid, const char* desc, ...) {
	va_list arg;
	va_start(arg, desc);
	target_err_va(desc, arg);
	target_err("\n");
	if (valid)
		target_err("%s\n", valid);
	va_end(arg);
}

int os_main(int argc, char* argv[])
{
	int r;
	int i;
	struct mame_option option;
	int opt_info;
	int opt_log;
	int opt_logsync;
	int opt_default;
	int opt_remove;
	char* opt_gamename;
	struct advance_context* context = &CONTEXT;
	struct conf_context* cfg_context;
	const char* section_map[5];

	opt_info = 0;
	opt_log = 0;
	opt_logsync = 0;
	opt_gamename = 0;
	opt_default = 0;
	opt_remove = 0;

	memset(&option,0,sizeof(option));

	cfg_context = conf_init();

	if (os_init(cfg_context)!=0) {
		target_err("Error initializing the OS support.\n");
		goto err_conf;
	}

	if (mame_init(context,cfg_context)!=0)
		goto err_os;
	if (advance_video_init(&context->video, cfg_context)!=0)
		goto err_os;
	if (advance_sound_init(&context->sound, cfg_context)!=0)
		goto err_os;
	if (advance_input_init(&context->input, cfg_context)!=0)
		goto err_os;
	if (advance_record_init(&context->record, cfg_context)!=0)
		goto err_os;
	if (advance_fileio_init(cfg_context)!=0)
		goto err_os;
	if (advance_safequit_init(&context->safequit,cfg_context)!=0)
		goto err_os;
	if (hardware_script_init(cfg_context)!=0)
		goto err_os;

#ifdef __MSDOS__
	/* LEGACY (to be removed) */
	if (file_config_file_legacy(ADVANCE_NAME_LEGACY ".cfg")!=0 && access(file_config_file_legacy(ADVANCE_NAME_LEGACY ".cfg"),R_OK)==0 && access(file_config_file_home(ADVANCE_NAME ".rc"),R_OK)!=0) {
		if (conf_input_file_load_adv(cfg_context, 0, file_config_file_legacy(ADVANCE_NAME_LEGACY ".cfg"), file_config_file_home(ADVANCE_NAME ".rc"), 1, 0, CONV, sizeof(CONV)/sizeof(CONV[0]), error_callback, 0) != 0)
			goto err_os;
		conf_sort(cfg_context);
		conf_uncomment(cfg_context);
		conf_save(cfg_context,1);
		target_out("Configuration file '%s' created from '%s'\n", file_config_file_home(ADVANCE_NAME ".rc"), file_config_file_legacy(ADVANCE_NAME_LEGACY ".cfg"));
		goto done_os;
	}
#endif

	if (file_config_file_root(ADVANCE_NAME ".rc")!=0 && access(file_config_file_root(ADVANCE_NAME ".rc"),R_OK)==0)
		if (conf_input_file_load_adv(cfg_context, 2, file_config_file_root(ADVANCE_NAME ".rc"), 0, 0, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0)
			goto err_os;

	if (conf_input_file_load_adv(cfg_context, 0, file_config_file_home(ADVANCE_NAME ".rc"), file_config_file_home(ADVANCE_NAME ".rc"), 0, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0)
		goto err_os;

	if (conf_input_args_load(cfg_context, 1, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	option.debug_flag = 0;
	for(i=1;i<argc;++i) {
		if (strcmp(argv[i],"-default") == 0) {
			opt_default = 1;
		} else if (strcmp(argv[i],"-log") == 0) {
			opt_log = 1;
		} else if (strcmp(argv[i],"-logsync") == 0) {
			opt_logsync = 1;
		} else if (strcmp(argv[i],"-remove") == 0) {
			opt_remove = 1;
		} else if (strcmp(argv[i],"-debug") == 0) {
			option.debug_flag = 1;
		} else if (strcmp(argv[i],"-listinfo") == 0) {
			opt_info = 1;
		} else if (strcmp(argv[i],"-record") == 0 && i+1<argc && argv[i+1][0] != '-') {
			strcpy(option.record_file,argv[i+1]);
			++i;
		} else if (strcmp(argv[i],"-playback") == 0 && i+1<argc && argv[i+1][0] != '-') {
			strcpy(option.playback_file,argv[i+1]);
			++i;
		} else if (argv[i][0]!='-') {
			unsigned j;
			if (opt_gamename) {
				target_err("Multiple game name definition, '%s' and '%s'.\n", opt_gamename, argv[i]);
				goto err_os;
			}
			opt_gamename = argv[i];
			/* case insensitive compare, there are a lot of frontends which use uppercase names */
			for(j=0;opt_gamename[j];++j)
				opt_gamename[j] = tolower(opt_gamename[j]);
		} else {
			target_err("Unknown command line option '%s'.\n",argv[i]);
			goto err_os;
		}
	}

	if (opt_info) {
		mame_print_info(stdout);
		goto done_os;
	}

	if (!(file_config_file_root(ADVANCE_NAME ".rc") != 0 && access(file_config_file_root(ADVANCE_NAME ".rc"),R_OK)==0)
		&& !(file_config_file_home(ADVANCE_NAME ".rc") != 0 && access(file_config_file_home(ADVANCE_NAME ".rc"),R_OK)==0)) {
		conf_set_default_if_missing(cfg_context,"");
		conf_sort(cfg_context);
		conf_save(cfg_context,1);
		target_out("Configuration file '%s' created with all the default options\n", file_config_file_home(ADVANCE_NAME ".rc"));
		goto done_os;
	}

	if (opt_default) {
		conf_set_default_if_missing(cfg_context,"");
		conf_save(cfg_context,1);
		target_out("Configuration file '%s' updated with all the default options\n", file_config_file_home(ADVANCE_NAME ".rc"));
		goto done_os;
	}

	if (opt_remove) {
		conf_remove_if_default(cfg_context,"");
		conf_save(cfg_context,1);
		target_out("Configuration file '%s' updated with all the default options removed\n", file_config_file_home(ADVANCE_NAME ".rc"));
		goto done_os;
	}

	if (!opt_gamename) {
		if (!option.playback_file[0]) {
			target_err("No game specified.\n");
			goto err_os;
		}

		/* we need to know where search the playback file. Without knowing the */
		/* game only the global options can be used. */
		/* It implies that after the game is know the playback directory may */
		/* differ becase a specific option for the game is present in the */
		/* configuration */
		section_map[0] = "";
		conf_section_set(cfg_context, section_map, 1);

		if (advance_fileio_config_load(cfg_context, &option) != 0)
			goto err_os;

		option.game = mame_playback_look(option.playback_file);
		if (option.game == 0)
			goto err_os;

	} else {
		option.game = select_game(opt_gamename);
		if (option.game == 0)
			goto err_os;
	}

	if (opt_log || opt_logsync) {
		if (log_init(ADVANCE_NAME ".log", opt_logsync) != 0) {
			target_err("Error opening the log file '" ADVANCE_NAME ".log'.\n");
			goto err_os;
		}
	}

	/* set the used section */
	section_map[0] = mame_game_name(option.game);
	section_map[1] = mame_game_resolutionclock(option.game);
	section_map[2] = mame_game_resolution(option.game);
	if ((mame_game_orientation(option.game) & OSD_ORIENTATION_SWAP_XY) != 0)
		section_map[3] = "vertical";
	else
		section_map[3] = "horizontal";
	section_map[4] = "";
	conf_section_set(cfg_context, section_map, 5);
	for(i=0;i<5;++i)
		log_std(("advance: use configuration section '%s'", section_map[i]));

	log_std(("advance: *_load()\n"));

	/* load all the options */
	if (mame_config_load(cfg_context,&option) != 0)
		goto err_os;
	if (advance_video_config_load(&context->video, cfg_context, &option) != 0)
		goto err_os;
	if (advance_sound_config_load(&context->sound, cfg_context, &option) != 0)
		goto err_os;
	if (advance_input_config_load(&context->input, cfg_context) != 0)
		goto err_os;
	if (advance_record_config_load(&context->record, cfg_context) != 0)
		goto err_os;
	if (advance_fileio_config_load(cfg_context, &option) != 0)
		goto err_os;
	if (advance_safequit_config_load(&context->safequit,cfg_context) != 0)
		goto err_os;
	if (hardware_script_config_load(cfg_context) != 0)
		goto err_os;

	if (!option.quiet_flag) {
		target_nfo(ADVANCE_COPY);
#ifdef __MSDOS__
		target_nfo("%d [Mb] free physical memory, %d [Mb] free virtual memory\n", (unsigned)_go32_dpmi_remaining_physical_memory()/(1024*1024), (unsigned)_go32_dpmi_remaining_virtual_memory()/(1024*1024));
#endif
	}

	log_std(("advance: os_inner_init()\n"));

	if (os_inner_init(ADVANCE_TITLE) != 0) {
		goto err_os;
	}

	log_std(("advance: *_inner_init()\n"));

	if (advance_video_inner_init(&context->video, &option) != 0)
		goto err_os_inner;
	if (advance_input_inner_init(&context->input)!=0)
		goto err_os_inner;
	if (advance_safequit_inner_init(&context->safequit, &option)!=0)
		goto err_os_inner;
	if (hardware_script_inner_init()!=0)
		goto err_os_inner;

	log_std(("advance: mame_game_run()\n"));

	r = mame_game_run(context,&option);
	if (r < 0)
		goto err_os_inner;

	log_std(("advance: *_inner_done()\n"));

	hardware_script_inner_done();
	advance_safequit_inner_done(&context->safequit);
	advance_input_inner_done(&context->input);
	advance_video_inner_done(&context->video);

	log_std(("advance: os_inner_done()\n"));

	os_inner_done();

	log_std(("advance: *_done()\n"));

	hardware_script_done();
	advance_safequit_done(&context->safequit);
	advance_fileio_done();
	advance_record_done(&context->record);
	advance_input_done(&context->input);
	advance_sound_done(&context->sound);
	advance_video_done(&context->video);
	mame_done(context);

	log_std(("advance: os_msg_done()\n"));

	if (opt_log || opt_logsync) {
		log_done();
	}

	log_std(("advance: os_done()\n"));

	os_done();

	log_std(("advance: conf_save()\n"));

	/* save the configuration only if modified */
	conf_save(cfg_context,0);

	log_std(("advance: conf_done()\n"));

	conf_done(cfg_context);

	return r;

done_os:
	os_done();
	conf_done(cfg_context);
	return 0;

err_os_inner:
	os_inner_done();
err_os:
	os_done();
err_conf:
	conf_done(cfg_context);
	return -1;
}
