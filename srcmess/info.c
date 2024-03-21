/***************************************************************************

    info.c

    Dumps the MAME internal data as an XML file.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include <ctype.h>

#include "driver.h"
#include "sound/samples.h"
#include "info.h"
#include "hash.h"

/* MESS/MAME configuration */
#ifdef MESS
#define XML_ROOT "mess"
#define XML_TOP "machine"
#else
#define XML_ROOT "mame"
#define XML_TOP "game"
#endif

#ifdef MESS
void print_game_device(FILE* out, const game_driver* game);
void print_game_ramoptions(FILE* out, const game_driver* game);
#endif /* MESS */

/* Print a free format string */
static const char *normalize_string(const char* s)
{
	static char buffer[1024];
	char *d = &buffer[0];

	if (s)
	{
		while (*s)
		{
			switch (*s)
			{
				case '\"' : d += sprintf(d, "&quot;"); break;
				case '&'  : d += sprintf(d, "&amp;"); break;
				case '<'  : d += sprintf(d, "&lt;"); break;
				case '>'  : d += sprintf(d, "&gt;"); break;
				default:
					if (*s>=' ' && *s<='~')
						*d++ = *s;
					else
						d += sprintf(d, "&#%d;", (unsigned)(unsigned char)*s);
			}
			++s;
		}
	}
	*d++ = 0;
	return buffer;
}

static void print_free_string(FILE *out, const char* s)
{
	if (s)
	{
		while (*s)
		{
			switch (*s)
			{
				case '\"' : fprintf(out, "&quot;"); break;
				case '&'  : fprintf(out, "&amp;"); break;
				case '<'  : fprintf(out, "&lt;"); break;
				case '>'  : fprintf(out, "&gt;"); break;
				default:
					if (*s>=' ' && *s<='~')
						fprintf(out, "%c", *s);
					else
						fprintf(out, "&#%d;", (unsigned)(unsigned char)*s);
			}
			++s;
		}
	}
}

static void print_game_switch(FILE* out, const game_driver* game)
{
	const input_port_entry* input;

	begin_resource_tracking();

	input = input_port_allocate(game->construct_ipt, NULL);

	while (input->type != IPT_END)
	{
		if (input->type==IPT_DIPSWITCH_NAME)
		{
			int def = input->default_value;

			fprintf(out, "\t\t<dipswitch");

			fprintf(out, " name=\"%s\"", normalize_string(input->name));
			++input;

			fprintf(out, ">\n");

			while (input->type==IPT_DIPSWITCH_SETTING)
			{
				fprintf(out, "\t\t\t<dipvalue");
				fprintf(out, " name=\"%s\"", normalize_string(input->name));
				if (def == input->default_value)
					fprintf(out, " default=\"yes\"");

				fprintf(out, "/>\n");

				++input;
			}

			fprintf(out, "\t\t</dipswitch>\n");
		}
		else
			++input;
	}

	end_resource_tracking();
}

static void print_game_input(FILE* out, const game_driver* game)
{
	const input_port_entry* input;
	int nplayer = 0;
	const char* control = 0;
	int nbutton = 0;
	int ncoin = 0;
	const char* service = 0;
	const char* tilt = 0;

	begin_resource_tracking();

	input = input_port_allocate(game->construct_ipt, NULL);

	while (input->type != IPT_END)
	{
		if (nplayer < input->player+1)
			nplayer = input->player+1;

		switch (input->type)
		{
			case IPT_JOYSTICK_LEFT:
			case IPT_JOYSTICK_RIGHT:

				/* if control not defined, start it off as horizontal 2-way */
				if (!control)
					control = "joy2way";
				else if (strcmp(control,"joy2way") == 0)
					;
				/* if already defined as vertical, make it 4 or 8 way */
				else if (strcmp(control,"vjoy2way") == 0)
				{
					if (input->four_way)
						control = "joy4way";
					else
						control = "joy8way";
				}
				break;

			case IPT_JOYSTICK_UP:
			case IPT_JOYSTICK_DOWN:

				/* if control not defined, start it off as vertical 2-way */
				if (!control)
					control= "vjoy2way";
				else if (strcmp(control,"vjoy2way") == 0)
					;
				/* if already defined as horiz, make it 4 or 8way */
				else if (strcmp(control,"joy2way")==0)
				{
					if (input->four_way)
						control = "joy4way";
					else
						control = "joy8way";
				}
				break;

			case IPT_JOYSTICKRIGHT_UP:
			case IPT_JOYSTICKRIGHT_DOWN:
			case IPT_JOYSTICKLEFT_UP:
			case IPT_JOYSTICKLEFT_DOWN:

				/* if control not defined, start it off as vertical 2way */
				if (!control)
					control= "vdoublejoy2way";
				else if (strcmp(control,"vdoublejoy2way") == 0)
					;
				/* if already defined as horiz, make it 4 or 8 way */
				else if (strcmp(control,"doublejoy2way") == 0)
				{
					if (input->four_way)
						control = "doublejoy4way";
					else
						control = "doublejoy8way";
				}
				break;

			case IPT_JOYSTICKRIGHT_LEFT:
			case IPT_JOYSTICKRIGHT_RIGHT:
			case IPT_JOYSTICKLEFT_LEFT:
			case IPT_JOYSTICKLEFT_RIGHT:

				/* if control not defined, start it off as horiz 2-way */
				if (!control)
					control="doublejoy2way";
				else if (strcmp(control,"doublejoy2way") == 0)
					;
				/* if already defined as vertical, make it 4 or 8 way */
				else if (strcmp(control,"vdoublejoy2way") == 0)
				{
					if (input->four_way)
						control = "doublejoy4way";
					else
						control = "doublejoy8way";
				}
				break;

			case IPT_BUTTON1:
				if (nbutton<1) nbutton = 1;
				break;
			case IPT_BUTTON2:
				if (nbutton<2) nbutton = 2;
				break;
			case IPT_BUTTON3:
				if (nbutton<3) nbutton = 3;
				break;
			case IPT_BUTTON4:
				if (nbutton<4) nbutton = 4;
				break;
			case IPT_BUTTON5:
				if (nbutton<5) nbutton = 5;
				break;
			case IPT_BUTTON6:
				if (nbutton<6) nbutton = 6;
				break;
			case IPT_BUTTON7:
				if (nbutton<7) nbutton = 7;
				break;
			case IPT_BUTTON8:
				if (nbutton<8) nbutton = 8;
				break;
			case IPT_BUTTON9:
				if (nbutton<9) nbutton = 9;
				break;
			case IPT_BUTTON10:
				if (nbutton<10) nbutton = 10;
				break;
			case IPT_PADDLE:
				control = "paddle";
				break;
			case IPT_DIAL:
				control = "dial";
				break;
			case IPT_TRACKBALL_X:
			case IPT_TRACKBALL_Y:
				control = "trackball";
				break;
			case IPT_AD_STICK_X:
			case IPT_AD_STICK_Y:
				control = "stick";
				break;
			case IPT_LIGHTGUN_X:
			case IPT_LIGHTGUN_Y:
				control = "lightgun";
				break;
			case IPT_COIN1:
				if (ncoin < 1) ncoin = 1;
				break;
			case IPT_COIN2:
				if (ncoin < 2) ncoin = 2;
				break;
			case IPT_COIN3:
				if (ncoin < 3) ncoin = 3;
				break;
			case IPT_COIN4:
				if (ncoin < 4) ncoin = 4;
				break;
			case IPT_COIN5:
				if (ncoin < 5) ncoin = 5;
				break;
			case IPT_COIN6:
				if (ncoin < 6) ncoin = 6;
				break;
			case IPT_COIN7:
				if (ncoin < 7) ncoin = 7;
				break;
			case IPT_COIN8:
				if (ncoin < 8) ncoin = 8;
				break;
			case IPT_SERVICE :
				service = "yes";
				break;
			case IPT_TILT :
				tilt = "yes";
				break;
		}
		++input;
	}

	fprintf(out, "\t\t<input");
	fprintf(out, " players=\"%d\"", nplayer );
	if (control)
		fprintf(out, " control=\"%s\"", normalize_string(control) );
	if (nbutton)
		fprintf(out, " buttons=\"%d\"", nbutton );
	if (ncoin)
		fprintf(out, " coins=\"%d\"", ncoin );
	if (service)
		fprintf(out, " service=\"%s\"", normalize_string(service) );
	if (tilt)
		fprintf(out, " tilt=\"%s\"", normalize_string(tilt) );
	fprintf(out, "/>\n");

	end_resource_tracking();
}

static void print_game_bios(FILE* out, const game_driver* game)
{
	const bios_entry *thisbios;

	if(!game->bios)
		return;

	thisbios = game->bios;

	/* Match against bios short names */
	while(!BIOSENTRY_ISEND(thisbios))
	{
		fprintf(out, "\t\t<biosset");

		if (thisbios->_name)
			fprintf(out, " name=\"%s\"", normalize_string(thisbios->_name));
		if (thisbios->_description)
			fprintf(out, " description=\"%s\"", normalize_string(thisbios->_description));
		if (thisbios->value == 0)
			fprintf(out, " default=\"yes\"");

		fprintf(out, "/>\n");

		thisbios++;
	}
}

static void print_game_rom(FILE* out, int bare, const game_driver* game)
{
	const rom_entry *region, *rom, *chunk;
	const rom_entry *pregion, *prom, *fprom=NULL;
	const game_driver *clone_of;
	unsigned rom_bare_size = 0;

	if (!game->rom)
		return;

	clone_of = driver_get_clone(game);
	for (region = rom_first_region(game); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			int offset, length, in_parent, is_disk, is_bios, found_bios, i;
			char name[100], bios_name[100];

			strcpy(name,ROM_GETNAME(rom));
			offset = ROM_GETOFFSET(rom);
			is_disk = ROMREGION_ISDISKDATA(region);
			is_bios = ROM_GETBIOSFLAGS(rom);

			in_parent = 0;
			length = 0;
			for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
				length += ROM_GETLENGTH(chunk);

			if (!ROM_NOGOODDUMP(rom) && clone_of)
			{
				fprom=NULL;
				for (pregion = rom_first_region(clone_of); pregion; pregion = rom_next_region(pregion))
					for (prom = rom_first_file(pregion); prom; prom = rom_next_file(prom))
						if (hash_data_is_equal(ROM_GETHASHDATA(rom), ROM_GETHASHDATA(prom), 0))
						{
							if (!fprom || !strcmp(ROM_GETNAME(prom), name))
								fprom=prom;
							in_parent = 1;
						}
			}

			if (bare && in_parent) {
				continue;
			}

			found_bios = 0;
			if(!is_disk && is_bios && game->bios)
			{
				const bios_entry *thisbios = game->bios;

				/* Match against bios short names */
				while(!found_bios && !BIOSENTRY_ISEND(thisbios) )
				{
					if((is_bios-1) == thisbios->value) /* Note '-1' */
					{
						strcpy(bios_name,thisbios->_name);
						found_bios = 1;
					}

					thisbios++;
				}
			}


			if (bare) {
				rom_bare_size += length;
				continue;
			}

			if (!is_disk)
				fprintf(out, "\t\t<rom");
			else
				fprintf(out, "\t\t<disk");

			if (*name)
				fprintf(out, " name=\"%s\"", normalize_string(name));
			if (in_parent)
				fprintf(out, " merge=\"%s\"", normalize_string(ROM_GETNAME(fprom)));
			if (!is_disk && found_bios)
				fprintf(out, " bios=\"%s\"", normalize_string(bios_name));
			if (!is_disk)
				fprintf(out, " size=\"%d\"", length);

			/* dump checksum information only if there is a known dump */
			if (!hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
			{
				for (i=0;i<HASH_NUM_FUNCTIONS;i++)
				{
					int func = 1<<i;
					const char* func_name = hash_function_name(func);
					char checksum[1000];

					if (hash_data_extract_printable_checksum(ROM_GETHASHDATA(rom), func, checksum))
					{
						fprintf(out, " %s=\"%s\"", func_name, checksum);
					}
				}
			}

			switch (ROMREGION_GETTYPE(region))
			{
				case REGION_CPU1: fprintf(out, " region=\"cpu1\""); break;
				case REGION_CPU2: fprintf(out, " region=\"cpu2\""); break;
				case REGION_CPU3: fprintf(out, " region=\"cpu3\""); break;
				case REGION_CPU4: fprintf(out, " region=\"cpu4\""); break;
				case REGION_CPU5: fprintf(out, " region=\"cpu5\""); break;
				case REGION_CPU6: fprintf(out, " region=\"cpu6\""); break;
				case REGION_CPU7: fprintf(out, " region=\"cpu7\""); break;
				case REGION_CPU8: fprintf(out, " region=\"cpu8\""); break;
				case REGION_GFX1: fprintf(out, " region=\"gfx1\""); break;
				case REGION_GFX2: fprintf(out, " region=\"gfx2\""); break;
				case REGION_GFX3: fprintf(out, " region=\"gfx3\""); break;
				case REGION_GFX4: fprintf(out, " region=\"gfx4\""); break;
				case REGION_GFX5: fprintf(out, " region=\"gfx5\""); break;
				case REGION_GFX6: fprintf(out, " region=\"gfx6\""); break;
				case REGION_GFX7: fprintf(out, " region=\"gfx7\""); break;
				case REGION_GFX8: fprintf(out, " region=\"gfx8\""); break;
				case REGION_PROMS: fprintf(out, " region=\"proms\""); break;
				case REGION_SOUND1: fprintf(out, " region=\"sound1\""); break;
				case REGION_SOUND2: fprintf(out, " region=\"sound2\""); break;
				case REGION_SOUND3: fprintf(out, " region=\"sound3\""); break;
				case REGION_SOUND4: fprintf(out, " region=\"sound4\""); break;
				case REGION_SOUND5: fprintf(out, " region=\"sound5\""); break;
				case REGION_SOUND6: fprintf(out, " region=\"sound6\""); break;
				case REGION_SOUND7: fprintf(out, " region=\"sound7\""); break;
				case REGION_SOUND8: fprintf(out, " region=\"sound8\""); break;
				case REGION_USER1: fprintf(out, " region=\"user1\""); break;
				case REGION_USER2: fprintf(out, " region=\"user2\""); break;
				case REGION_USER3: fprintf(out, " region=\"user3\""); break;
				case REGION_USER4: fprintf(out, " region=\"user4\""); break;
				case REGION_USER5: fprintf(out, " region=\"user5\""); break;
				case REGION_USER6: fprintf(out, " region=\"user6\""); break;
				case REGION_USER7: fprintf(out, " region=\"user7\""); break;
				case REGION_USER8: fprintf(out, " region=\"user8\""); break;
				case REGION_DISKS: fprintf(out, " region=\"disks\""); break;
				default: fprintf(out, " region=\"0x%x\"", ROMREGION_GETTYPE(region));
		}

		if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
			fprintf(out, " status=\"nodump\"");
		if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP))
			fprintf(out, " status=\"baddump\"");

		if (!is_disk)
		{
			if (ROMREGION_GETFLAGS(region) & ROMREGION_DISPOSE)
				fprintf(out, " dispose=\"yes\"");

			fprintf(out, " offset=\"%x\"", offset);
			fprintf(out, "/>\n");
		}
		else
		{
			fprintf(out, " index=\"%x\"", DISK_GETINDEX(rom));
			fprintf(out, "/>\n");
		}
	}

	if (bare && rom_bare_size) {
		fprintf(out, "\t\t<rom size=\"%u\"/>\n", rom_bare_size);
	}
}

static void print_game_sampleof(FILE* out, const game_driver* game)
{
#if (HAS_SAMPLES)
	machine_config drv;
	int i;

	expand_machine_driver(game->drv, &drv);

	for( i = 0; drv.sound[i].sound_type && i < MAX_SOUND; i++ )
	{
		const char **samplenames = NULL;
		if( drv.sound[i].sound_type == SOUND_SAMPLES )
			samplenames = ((struct Samplesinterface *)drv.sound[i].config)->samplenames;
		if (samplenames != 0 && samplenames[0] != 0) {
			int k = 0;
			if (samplenames[k][0]=='*')
			{
				/* output sampleof only if different from game name */
				if (strcmp(samplenames[k] + 1, game->name)!=0)
					fprintf(out, " sampleof=\"%s\"", normalize_string(samplenames[k] + 1));
				++k;
			}
		}
	}
#endif
}

static void print_game_sample(FILE* out, const game_driver* game)
{
#if (HAS_SAMPLES)
	machine_config drv;
	int i;

	expand_machine_driver(game->drv, &drv);

	for( i = 0; drv.sound[i].sound_type && i < MAX_SOUND; i++ )
	{
		const char **samplenames = NULL;
		if( drv.sound[i].sound_type == SOUND_SAMPLES )
			samplenames = ((struct Samplesinterface *)drv.sound[i].config)->samplenames;
		if (samplenames != 0 && samplenames[0] != 0) {
			int k = 0;
			if (samplenames[k][0]=='*')
			{
				++k;
			}
			while (samplenames[k] != 0) {
				/* check if is not empty */
				if (*samplenames[k]) {
					/* check if sample is duplicate */
					int l = 0;
					while (l<k && strcmp(samplenames[k],samplenames[l])!=0)
						++l;
					if (l==k)
						fprintf(out, "\t\t<sample name=\"%s\"/>\n", normalize_string(samplenames[k]));
				}
				++k;
			}
		}
	}
#endif
}

static void print_game_micro(FILE* out, const game_driver* game)
{
	machine_config driver;
	const cpu_config* cpu;
	const sound_config* sound;
	int j;

	expand_machine_driver(game->drv, &driver);
	cpu = driver.cpu;
	sound = driver.sound;

	for(j=0;j<MAX_CPU;++j)
	{
		if (cpu[j].cpu_type!=0)
		{
			fprintf(out, "\t\t<chip");
			fprintf(out, " type=\"cpu\"");

			fprintf(out, " name=\"%s\"", normalize_string(cputype_name(cpu[j].cpu_type)));

			fprintf(out, " clock=\"%d\"", cpu[j].cpu_clock);
			fprintf(out, "/>\n");
		}
	}

	for(j=0;j<MAX_SOUND;++j) if (sound[j].sound_type)
	{
		if (sound[j].sound_type)
		{
			fprintf(out, "\t\t<chip");
			fprintf(out, " type=\"audio\"");
			fprintf(out, " name=\"%s\"", normalize_string(sndtype_name(sound[j].sound_type)));
			if (sound[j].clock)
				fprintf(out, " clock=\"%d\"", sound[j].clock);
			fprintf(out, "/>\n");
		}
	}
}

static void print_game_video(FILE* out, const game_driver* game)
{
	machine_config driver;

	int dx;
	int dy;
	int ax;
	int ay;
	int showxy;
	int orientation;

	expand_machine_driver(game->drv, &driver);

	fprintf(out, "\t\t<video");
	if (driver.video_attributes & VIDEO_TYPE_VECTOR)
	{
		fprintf(out, " screen=\"vector\"");
		showxy = 0;
	}
	else
	{
		fprintf(out, " screen=\"raster\"");
		showxy = 1;
	}

	if (game->flags & ORIENTATION_SWAP_XY)
	{
		ax = driver.aspect_y;
		ay = driver.aspect_x;
		if (ax == 0 && ay == 0) {
			ax = 3;
			ay = 4;
		}
		dx = driver.default_visible_area.max_y - driver.default_visible_area.min_y + 1;
		dy = driver.default_visible_area.max_x - driver.default_visible_area.min_x + 1;
		orientation = 1;
	}
	else
	{
		ax = driver.aspect_x;
		ay = driver.aspect_y;
		if (ax == 0 && ay == 0) {
			ax = 4;
			ay = 3;
		}
		dx = driver.default_visible_area.max_x - driver.default_visible_area.min_x + 1;
		dy = driver.default_visible_area.max_y - driver.default_visible_area.min_y + 1;
		orientation = 0;
	}

	fprintf(out, " orientation=\"%s\"", orientation ? "vertical" : "horizontal" );
	if (showxy)
	{
		fprintf(out, " width=\"%d\"", dx);
		fprintf(out, " height=\"%d\"", dy);
	}

	fprintf(out, " aspectx=\"%d\"", ax);
	fprintf(out, " aspecty=\"%d\"", ay);

	fprintf(out, " refresh=\"%f\"", driver.frames_per_second);
	fprintf(out, "/>\n");
}

static void print_game_sound(FILE* out, const game_driver* game)
{
	machine_config driver;
	const cpu_config* cpu;
	const sound_config* sound;

	/* check if the game have sound emulation */
	int has_sound = 0;
	int i;

	expand_machine_driver(game->drv, &driver);
	cpu = driver.cpu;
	sound = driver.sound;

	i = 0;
	while (i < MAX_SOUND && !has_sound)
	{
		if (sound[i].sound_type)
			has_sound = 1;
		++i;
	}

	fprintf(out, "\t\t<sound");

	/* sound channel */
	if (has_sound)
	{
		int speakers;
		for (speakers = 0; speakers < MAX_SPEAKER; speakers++)
			if (driver.speaker[speakers].tag == NULL)
				break;
		fprintf(out, " channels=\"%d\"", speakers);
	}
	else
		fprintf(out, " channels=\"0\"");

	fprintf(out, "/>\n");
}

static void print_game_driver(FILE* out, const game_driver* game)
{
	machine_config driver;

	expand_machine_driver(game->drv, &driver);

	fprintf(out, "\t\t<driver");

	/* The status entry is an hint for frontend authors */
	/* to select working and not working games without */
	/* the need to know all the other status entries. */
	/* Games marked as status=good are perfectly emulated, games */
	/* marked as status=imperfect are emulated with only */
	/* some minor issues, games marked as status=preliminary */
	/* don't work or have major emulation problems. */

	if (game->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND | GAME_WRONG_COLORS))
		fprintf(out, " status=\"preliminary\"");
	else if (game->flags & (GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS))
		fprintf(out, " status=\"imperfect\"");
	else
		fprintf(out, " status=\"good\"");

	if (game->flags & GAME_NOT_WORKING)
		fprintf(out, " emulation=\"preliminary\"");
	else
		fprintf(out, " emulation=\"good\"");

	if (game->flags & GAME_WRONG_COLORS)
		fprintf(out, " color=\"preliminary\"");
	else if (game->flags & GAME_IMPERFECT_COLORS)
		fprintf(out, " color=\"imperfect\"");
	else
		fprintf(out, " color=\"good\"");

	if (game->flags & GAME_NO_SOUND)
		fprintf(out, " sound=\"preliminary\"");
	else if (game->flags & GAME_IMPERFECT_SOUND)
		fprintf(out, " sound=\"imperfect\"");
	else
		fprintf(out, " sound=\"good\"");

	if (game->flags & GAME_IMPERFECT_GRAPHICS)
		fprintf(out, " graphic=\"imperfect\"");
	else
		fprintf(out, " graphic=\"good\"");

	if (game->flags & GAME_NO_COCKTAIL)
		fprintf(out, " cocktail=\"preliminary\"");

	if (game->flags & GAME_UNEMULATED_PROTECTION)
		fprintf(out, " protection=\"preliminary\"");

	if (game->flags & GAME_SUPPORTS_SAVE)
		fprintf(out, " savestate=\"supported\"");
	else
		fprintf(out, " savestate=\"unsupported\"");

	fprintf(out, " palettesize=\"%d\"", driver.total_colors);

	fprintf(out, "/>\n");
}

/* Print the MAME info record for a game */
static void print_game_info(FILE* out, int bare, const game_driver* game)
{
	const char *start;
	const game_driver *clone_of;

	/* No action if not a game */
	if (game->flags & NOT_A_DRIVER)
		return;

	fprintf(out, "\t<" XML_TOP);

	fprintf(out, " name=\"%s\"", normalize_string(game->name) );

	start = strrchr(game->source_file, '/');
	if (!start)
		start = strrchr(game->source_file, '\\');
	if (!start)
		start = game->source_file - 1;
	if (!bare)
		fprintf(out, " sourcefile=\"%s\"", normalize_string(start + 1));

	clone_of = driver_get_clone(game);
	if (clone_of && !(clone_of->flags & NOT_A_DRIVER))
		fprintf(out, " cloneof=\"%s\"", normalize_string(clone_of->name));

	if (clone_of)
		fprintf(out, " romof=\"%s\"", normalize_string(clone_of->name));

	print_game_sampleof(out, game);

	fprintf(out, ">\n");

	if (game->description)
		fprintf(out, "\t\t<description>%s</description>\n", normalize_string(game->description));

	/* print the year only if is a number */
	if (game->year && strspn(game->year,"0123456789")==strlen(game->year))
		fprintf(out, "\t\t<year>%s</year>\n", normalize_string(game->year) );

	if (game->manufacturer)
		fprintf(out, "\t\t<manufacturer>%s</manufacturer>\n", normalize_string(game->manufacturer));

	if (!bare)
		print_game_bios(out, game);
	print_game_rom(out, bare, game);
	if (!bare)
		print_game_sample(out, game);
	if (!bare)
		print_game_micro(out, game);
	print_game_video(out, game);
	if (!bare)
		print_game_sound(out, game);
	if (!bare)
		print_game_input(out, game);
	if (!bare)
		print_game_switch(out, game);
	print_game_driver(out, game);
#ifdef MESS
	print_game_device(out, game);
	if (!bare)
		print_game_ramoptions(out, game);
#endif

	fprintf(out, "\t</" XML_TOP ">\n");
}

#if !defined(MESS)
/* Print the resource info */
static void print_resource_info(FILE* out, int bare, const game_driver* game)
{
	const char *start;

 	/* No action if not a resource */
 	if ((game->flags & NOT_A_DRIVER) == 0)
 		return;


	/* The runnable entry is an hint for frontend authors */
	/* to easily know which game can be started. */
	/* Games marked as runnable=yes can be started putting */
	/* the game name as argument in the program command line, */
	/* games marked as runnable=no cannot be started. */
	fprintf(out, "\t<" XML_TOP " runnable=\"no\"");

	fprintf(out, " name=\"%s\"", normalize_string(game->name) );

	start = strrchr(game->source_file, '/');
	if (!start)
		start = strrchr(game->source_file, '\\');
	if (!start)
		start = game->source_file - 1;
	if (!bare)
		fprintf(out, " sourcefile=\"%s\"", normalize_string(start + 1));

	fprintf(out, ">\n");

	if (game->description)
		fprintf(out, "\t\t<description>%s</description>\n", normalize_string(game->description));

	/* print the year only if it's a number */
	if (game->year && strspn(game->year,"0123456789")==strlen(game->year))
		fprintf(out, "\t\t<year>%s</year>\n", normalize_string(game->year) );

	if (game->manufacturer)
		fprintf(out, "\t\t<manufacturer>%s</manufacturer>\n", normalize_string(game->manufacturer));

	print_game_bios(out, game);
	if (!bare)
		print_game_rom(out, bare, game);
	print_game_sample(out, game);

	fprintf(out, "\t</" XML_TOP ">\n");
}
#endif

static void print_mame_data(FILE* out, int bare, const game_driver* const games[])
{
	int j;

	/* print games */
	for(j=0;games[j];++j)
		print_game_info(out, bare, games[j]);

#if !defined(MESS)
	/* print resources */
 	for (j=0;games[j];++j)
 		print_resource_info(out, bare, games[j]);
#endif
}

/* Print the MAME database in XML format */
void print_mame_xml(FILE* out, int bare, const game_driver* const games[])
{
	fprintf(out,
		"<?xml version=\"1.0\"?>\n"
		"<!DOCTYPE " XML_ROOT " [\n"
		"<!ELEMENT " XML_ROOT " (" XML_TOP "+)>\n"
		"\t<!ATTLIST " XML_ROOT " build CDATA #IMPLIED>\n"
#ifdef MESS
		"\t<!ELEMENT " XML_TOP " (description, year?, manufacturer, history?, biosset*, rom*, disk*, sample*, chip*, video?, sound?, input?, dipswitch*, driver?, device*, ramoption*)>\n"
#else
		"\t<!ELEMENT " XML_TOP " (description, year?, manufacturer, history?, biosset*, rom*, disk*, sample*, chip*, video?, sound?, input?, dipswitch*, driver?)>\n"
#endif
		"\t\t<!ATTLIST " XML_TOP " name CDATA #REQUIRED>\n"
		"\t\t<!ATTLIST " XML_TOP " sourcefile CDATA #IMPLIED>\n"
		"\t\t<!ATTLIST " XML_TOP " runnable (yes|no) \"yes\">\n"
		"\t\t<!ATTLIST " XML_TOP " cloneof CDATA #IMPLIED>\n"
		"\t\t<!ATTLIST " XML_TOP " romof CDATA #IMPLIED>\n"
		"\t\t<!ATTLIST " XML_TOP " sampleof CDATA #IMPLIED>\n"
		"\t\t<!ELEMENT description (#PCDATA)>\n"
		"\t\t<!ELEMENT year (#PCDATA)>\n"
		"\t\t<!ELEMENT manufacturer (#PCDATA)>\n"
		"\t\t<!ELEMENT history (#PCDATA)>\n"
		"\t\t<!ELEMENT biosset EMPTY>\n"
		"\t\t\t<!ATTLIST biosset name CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST biosset description CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST biosset default (yes|no) \"no\">\n"
		"\t\t<!ELEMENT rom EMPTY>\n"
		"\t\t\t<!ATTLIST rom name CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST rom bios CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom size CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST rom crc CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom md5 CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom sha1 CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom merge CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom region CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom offset CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom status (baddump|nodump|good) \"good\">\n"
		"\t\t\t<!ATTLIST rom dispose (yes|no) \"no\">\n"
		"\t\t<!ELEMENT disk EMPTY>\n"
		"\t\t\t<!ATTLIST disk name CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST disk md5 CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST disk sha1 CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST disk merge CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST disk region CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST disk index CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST disk status (baddump|nodump|good) \"good\">\n"
		"\t\t<!ELEMENT sample EMPTY>\n"
		"\t\t\t<!ATTLIST sample name CDATA #REQUIRED>\n"
		"\t\t<!ELEMENT chip EMPTY>\n"
		"\t\t\t<!ATTLIST chip name CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST chip type (cpu|audio) #REQUIRED>\n"
		"\t\t\t<!ATTLIST chip clock CDATA #IMPLIED>\n"
		"\t\t<!ELEMENT video EMPTY>\n"
		"\t\t\t<!ATTLIST video screen (raster|vector) #REQUIRED>\n"
		"\t\t\t<!ATTLIST video orientation (vertical|horizontal) #REQUIRED>\n"
		"\t\t\t<!ATTLIST video width CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST video height CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST video aspectx CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST video aspecty CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST video refresh CDATA #REQUIRED>\n"
		"\t\t<!ELEMENT sound EMPTY>\n"
		"\t\t\t<!ATTLIST sound channels CDATA #REQUIRED>\n"
		"\t\t<!ELEMENT input EMPTY>\n"
		"\t\t\t<!ATTLIST input service (yes|no) \"no\">\n"
		"\t\t\t<!ATTLIST input tilt (yes|no) \"no\">\n"
		"\t\t\t<!ATTLIST input players CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST input control CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST input buttons CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST input coins CDATA #IMPLIED>\n"
		"\t\t<!ELEMENT dipswitch (dipvalue*)>\n"
		"\t\t\t<!ATTLIST dipswitch name CDATA #REQUIRED>\n"
		"\t\t\t<!ELEMENT dipvalue EMPTY>\n"
		"\t\t\t\t<!ATTLIST dipvalue name CDATA #REQUIRED>\n"
		"\t\t\t\t<!ATTLIST dipvalue default (yes|no) \"no\">\n"
		"\t\t<!ELEMENT driver EMPTY>\n"
		"\t\t\t<!ATTLIST driver status (good|imperfect|preliminary) #REQUIRED>\n"
		"\t\t\t<!ATTLIST driver emulation (good|imperfect|preliminary) #REQUIRED>\n"
		"\t\t\t<!ATTLIST driver color (good|imperfect|preliminary) #REQUIRED>\n"
		"\t\t\t<!ATTLIST driver sound (good|imperfect|preliminary) #REQUIRED>\n"
		"\t\t\t<!ATTLIST driver graphic (good|imperfect|preliminary) #REQUIRED>\n"
		"\t\t\t<!ATTLIST driver cocktail (good|imperfect|preliminary) #IMPLIED>\n"
		"\t\t\t<!ATTLIST driver protection (good|imperfect|preliminary) #IMPLIED>\n"
		"\t\t\t<!ATTLIST driver savestate (supported|unsupported) #REQUIRED>\n"
 		"\t\t\t<!ATTLIST driver palettesize CDATA #REQUIRED>\n"
#ifdef MESS
		"\t\t<!ELEMENT device (instance*, extension*)>\n"
		"\t\t\t<!ATTLIST device type CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST device tag CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST device mandatory CDATA #IMPLIED>\n"
		"\t\t\t<!ELEMENT instance EMPTY>\n"
		"\t\t\t\t<!ATTLIST instance name CDATA #REQUIRED>\n"
		"\t\t\t\t<!ATTLIST instance briefname CDATA #REQUIRED>\n"
		"\t\t\t<!ELEMENT extension EMPTY>\n"
		"\t\t\t\t<!ATTLIST extension name CDATA #REQUIRED>\n"
		"\t\t<!ELEMENT ramoption (#PCDATA)>\n"
#endif
		"]>\n\n"
		"<" XML_ROOT " build=\"%s\">\n",
		normalize_string(build_version)
	);

	print_mame_data(out, bare, games);

	fprintf(out, "</" XML_ROOT ">\n");
}

