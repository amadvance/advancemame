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
 */

#include "draw.h"
#include "video.h"
#include "blit.h"
#include "font.h"
#include "generate.h"
#include "error.h"
#include "os.h"
#include "videoall.h"
#include "inputall.h"
#include "log.h"
#include "file.h"
#include "conf.h"
#include "target.h"

#include <string.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

/***************************************************************************/
/* Common variable */

enum advance_t {
	advance_mame, advance_mess, advance_pac, advance_menu
} the_advance; /* The current operating mode */

unsigned x_from_y(unsigned y) {
	unsigned x;
	x = y*2/3;
	x = (x + 0xf) & ~0xF;
	return x;
}

/***************************************************************************/
/* Draw */

int draw_test(int x, int y, const char* s, adv_crtc* crtc, int modify) {
	char buffer[256];

	draw_test_default();

	draw_string(x,y,s,DRAW_COLOR_WHITE);
	++y;

	/* draw info */
	if (modify) {
		++y;
		draw_string(x,y,"ARROWS      Center",DRAW_COLOR_WHITE);
		++y;
		draw_string(x,y,"i/k         Expand x/y",DRAW_COLOR_WHITE);
		++y;
		draw_string(x,y,"SHIFT + i/k Shrink x/y",DRAW_COLOR_WHITE);
		++y;
		draw_string(x,y,"ENTER       Accept",DRAW_COLOR_WHITE);
		++y;
		draw_string(x,y,"ESC         Abort without saving",DRAW_COLOR_WHITE);
	}
	++y;
	++y;
	sprintf(buffer,"modeline %.2f %d %d %d %d %d %d %d %d", (double)crtc->pixelclock / 1E6, crtc->hde, crtc->hrs, crtc->hre, crtc->ht, crtc->vde, crtc->vrs, crtc->vre, crtc->vt);
	draw_string(x,y,buffer,DRAW_COLOR_WHITE);
	++y;

	sprintf(buffer,"pclock  %6.2f [MHz]", crtc_pclock_get(crtc) / 1E6);
	draw_string(x,y,buffer,DRAW_COLOR_WHITE);
	++y;
	sprintf(buffer,"hclock  %6.2f [kHz]", crtc_hclock_get(crtc) / 1E3);
	draw_string(x,y,buffer,DRAW_COLOR_WHITE);
	++y;
	sprintf(buffer,"vclock  %6.2f [Hz]", crtc_vclock_get(crtc));
	draw_string(x,y,buffer,DRAW_COLOR_WHITE);
	++y;

	sprintf(buffer,"hsync   %6.2f [us]", (double)(crtc->hre - crtc->hrs) * 1E6 / crtc_pclock_get(crtc) );
	draw_string(x,y,buffer,DRAW_COLOR_WHITE);
	++y;
	sprintf(buffer,"vsync   %6.2f [us]", (double)(crtc->vre - crtc->vrs) * 1E6 / crtc_hclock_get(crtc) );
	draw_string(x,y,buffer,DRAW_COLOR_WHITE);
	++y;

	if (crtc_is_doublescan(crtc))
		sprintf(buffer,"scan     double");
	else if (crtc_is_interlace(crtc))
		sprintf(buffer,"scan     interlace");
	else
		sprintf(buffer,"scan     single");
	draw_string(x,y,buffer,DRAW_COLOR_WHITE);

	return y;
}

static void draw_text_bar(void) {
	char buffer[256];
	unsigned i;

	sprintf(buffer," AdvanceCONFIG - " __DATE__ );
	draw_text_left(0,0,text_size_x(),buffer,COLOR_BAR);

	strcpy(buffer,"");
	for(i=0;i<video_driver_vector_max();++i) {
		if (video_driver_vector_pos(i) != 0) {
			if (*buffer)
				strcat(buffer,"/");
			strcat(buffer, video_driver_vector_pos(i)->name);
		}
	}
	draw_text_left(text_size_x() - strlen(buffer),0,strlen(buffer),buffer,COLOR_BAR);

	sprintf(buffer," ENTER Select  ESC Back");
	draw_text_left(0,text_size_y() - 1,text_size_x(),buffer,COLOR_BAR);
}

/***************************************************************************/
/* Command */

enum monitor_enum {
	monitor_pc,
	monitor_arcade_standard,
	monitor_arcade_extended,
	monitor_arcade_medium,
	monitor_arcade_vga,
	monitor_pal,
	monitor_ntsc,
	monitor_custom,
	monitor_previous
};

struct monitor_data_struct {
	enum monitor_enum type;
	const char* name;
	adv_generate generate;
};

int monitor_y;

static void entry_monitor(int x, int y, int dx, void* data, int n, int selected) {
	struct monitor_data_struct* p = ((struct monitor_data_struct*)data) + n;

	draw_text_left(x,y,dx,p->name, selected ? COLOR_REVERSE : COLOR_NORMAL);

	if (selected) {
		if (p->type == monitor_custom) {
			draw_text_left(0,monitor_y,text_size_x(),"format = ? ? ? ? ? ? ? ?",COLOR_NORMAL);
		} else {
			char buffer[256];
			adv_generate generate = p->generate;
			sprintf(buffer,"format = %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f",
				generate.hactive, generate.hfront, generate.hsync, generate.hback,
				generate.vactive, generate.vfront, generate.vsync, generate.vback);
			draw_text_left(0,monitor_y,text_size_x(),buffer,COLOR_NORMAL);
		}
	}
}

static adv_error cmd_monitor(adv_conf* config, adv_generate* generate, enum monitor_enum* type, adv_generate_interpolate_set* interpolate) {
	unsigned mac = 0;
	unsigned i;
	adv_error res;
	int y;

	struct monitor_data_struct data[16];

	if (generate_interpolate_load(config,interpolate)==0) {
		data[mac].type = monitor_previous;
		data[mac].name = "Previous saved values";
		data[mac].generate = interpolate->map[0].gen;
		++mac;
	}

	data[mac].type = monitor_custom;
	data[mac].name = "Custom";
	generate_default_vga(&data[mac].generate);
	++mac;

	data[mac].type = monitor_pc;
	data[mac].name = "PC Monitor (> 30 kHz)";
	generate_default_vga(&data[mac].generate);
	++mac;

	data[mac].type = monitor_arcade_standard;
	data[mac].name = "Arcade Standard CGA Resolution (15 kHz)";
	generate_default_atari_standard(&data[mac].generate);
	++mac;

	data[mac].type = monitor_arcade_extended;
	data[mac].name = "Arcade Extended Resolution (16.5 kHz)";
	generate_default_atari_extended(&data[mac].generate);
	++mac;

	data[mac].type = monitor_arcade_medium;
	data[mac].name = "Arcade Medium EGA Resolution (25 kHz)";
	generate_default_atari_medium(&data[mac].generate);
	++mac;

	data[mac].type = monitor_arcade_vga;
	data[mac].name = "Arcade VGA Resolution (31.5 kHz)";
	generate_default_atari_vga(&data[mac].generate);
	++mac;

	data[mac].type = monitor_pal;
	data[mac].name = "TV PAL (50 Hz)";
	generate_default_pal(&data[mac].generate);
	++mac;

	data[mac].type = monitor_ntsc;
	data[mac].name = "TV NTSC (60 Hz)";
	generate_default_ntsc(&data[mac].generate);
	++mac;

	for(i=0;i<mac;++i)
		generate_normalize(&data[i].generate);

	text_clear();
	draw_text_bar();

	y = 2;

	y = draw_text_para(0,y,text_size_x(),6,
"Now you must select the format of video modes supported by your monitor. "
"A predefined set based on the most common monitors is available. "
"\n\n"
"If you know the Active, FrontPorch, Sync and BackPorch values "
"required by your monitor you can enter them directly with the Custom selection "
"(only for expert users)."
	,COLOR_LOW);

	monitor_y = ++y;
	++y;

	draw_text_string(0,++y,"Select the video mode format:",COLOR_BOLD);
	++y;
	res = draw_text_menu(2,y,text_size_x() - 4,text_size_y() - 2 - y,&data,mac,entry_monitor,0,0,0,0);

	if (res<0)
		return -1;

	*type = data[res].type;
	*generate = data[res].generate;

	return 0;
}

static adv_error cmd_monitor_custom(adv_generate* generate) {
	int done;
	char buffer[64];
	text_clear();
	draw_text_bar();
	draw_text_left(0,2,text_size_x(),"format = ",COLOR_NORMAL);
	draw_text_para(0,4,text_size_x(),text_size_y()-6,
"Enter the 8 format values for your monitor. "
"In order the Horz Active, Horz Front Porch, Horz Sync, Horz Back Porch, Vert Active, Vert Front Porch, Vert Sync, Vert Back Porch.\n"
"The values are normalized to sum 1.\n\n"
"For example:\n\n"
"640 16 96 48 480 10 2 33"
	,COLOR_LOW);

	*buffer = 0;
	done = 0;
	while (!done) {
		if (draw_text_read(9,2,buffer,64,COLOR_REVERSE)!=INPUTB_ENTER)
			return -1;
		done = generate_parse(generate,buffer) == 0;
		if (!done)
			sound_error();
	}

	return 0;
}

struct model_data_struct {
	const char* manufacturer;
	const char* model;
	adv_monitor monitor;
};

int model_y;

static void entry_model(int x, int y, int dx, void* data, int n, int selected) {
	char buffer[256];
	struct model_data_struct* p = ((struct model_data_struct*)data) + n;

	if (p->model) {
		sprintf(buffer,"  %s - %s",p->manufacturer,p->model);
		draw_text_left(x,y,dx,buffer, selected ? COLOR_REVERSE : COLOR_NORMAL);

		if (selected) {
			strcpy(buffer,"pclock = ");
			monitor_print(buffer + strlen(buffer), &p->monitor.pclock, &p->monitor.pclock + 1, 1E6);
			draw_text_left(0,model_y,text_size_x(),buffer,COLOR_NORMAL);
			strcpy(buffer,"hclock = ");
			monitor_print(buffer + strlen(buffer), p->monitor.hclock, p->monitor.hclock + MONITOR_RANGE_MAX, 1E3);
			draw_text_left(0,model_y+1,text_size_x(),buffer,COLOR_NORMAL);
			strcpy(buffer,"vclock = ");
			monitor_print(buffer + strlen(buffer), p->monitor.vclock, p->monitor.vclock + MONITOR_RANGE_MAX, 1);
			draw_text_left(0,model_y+2,text_size_x(),buffer,COLOR_NORMAL);
		}
	} else {
		sprintf(buffer,"-- %s --",p->manufacturer);
		draw_text_left(x,y,dx,buffer, COLOR_LOW);
	}
}

static int entry_model_separator(void* data, int n) {
	struct model_data_struct* p = ((struct model_data_struct*)data) + n;
	return p->model == 0;
}

extern const char* MONITOR[];

static adv_error cmd_model(adv_conf* config, adv_monitor* monitor) {
	unsigned max = 3000;
	struct model_data_struct* data = malloc(max*sizeof(struct model_data_struct));
	unsigned mac = 0;
	adv_error res;
	int y;
	const char** i;
	adv_monitor previous_monitor;

	if (monitor_load(config, &previous_monitor)==0) {
		data[mac].model = strdup("Previous");
		data[mac].manufacturer = strdup("Previous saved values - Read the values from your .cfg file");
		data[mac].monitor = previous_monitor;
		++mac;
	}

	data[mac].model = strdup("Custom");
	data[mac].manufacturer = strdup("Custom - SUGGESTED - Read the values in your monitor manual");
	memset(&data[mac].monitor,0,sizeof(data[mac].monitor));
	++mac;

	i = (const char**)&MONITOR;
	while (*i) {
		char buffer[256];
		char* manufacturer;
		char* model;
		char* h;
		char* v;

		strcpy(buffer,*i);
		while (*buffer && isspace(buffer[strlen(buffer)-1]))
			buffer[strlen(buffer)-1] = 0;

		if (*buffer == '#') {
			if (mac < max) {
				data[mac].manufacturer = strdup(buffer + 2);
				data[mac].model = 0;
				++mac;
			}
		} else {
			manufacturer = strtok(buffer,":");
			model = strtok(NULL,":");
			h = strtok(NULL,":");
			v = strtok(NULL,":");
			if (manufacturer && model && h && v && mac < max) {
				data[mac].manufacturer = strdup(manufacturer);
				data[mac].model = strdup(model);
				if (monitor_parse(&data[mac].monitor,"5 - 90",h,v)!=0) {
					video_mode_restore();
					target_err("Invalid monitor specification %s:%s:%s:%s.",manufacturer,model,h,v);
					target_flush();
					exit(EXIT_FAILURE);
				}
				++mac;
			}
		}

		++i;
	}

	text_clear();
	draw_text_bar();

	y = 2;
	y = draw_text_para(0,y,text_size_x(),4,
"Now you must select the clock range supported by your monitor. "
"The strongly suggested choice is to check your monitor manual for the "
"supported vertical and horizontal clock range. "
	,COLOR_LOW);

	model_y = ++y;
	++y;
	++y;
	++y;

	draw_text_string(0,++y,"Select the model of your monitor:",COLOR_BOLD);

	++y;
	res = draw_text_menu(2,y,text_size_x() - 4,text_size_y() - 2 - y,data,mac,entry_model,entry_model_separator,0,0,0);

	if (res<0)
		return -1;

	*monitor = data[res].monitor;

	return 0;
}

static adv_error cmd_model_custom(adv_monitor* monitor) {
	int state;
	char pbuffer[64];
	char hbuffer[64];
	char vbuffer[64];

	text_clear();
	draw_text_bar();
	draw_text_left(0,2,text_size_x(),"pclock = ",COLOR_NORMAL);
	draw_text_left(0,3,text_size_x(),"hclock = ",COLOR_NORMAL);
	draw_text_left(0,4,text_size_x(),"vclock = ",COLOR_NORMAL);
	draw_text_para(0,6,text_size_x(),text_size_y()-8,
"Enter the clock specification of your monitor. "
"Usually you can find them in the last page of your monitor manual. "
"For the pclock you can safely use the values 5 - 90.\n"
"The pclock specification is in MHz, the hclock is in kHz, the vclock is in Hz.\n\n"
"For example:\n\n"
"pclock = 5-90\n"
"hclock = 30-50\n"
"vclock = 55-90\n"
"\nor\n\n"
"hclock = 12-70\n"
"hclock = 31.5, 35-50\n"
"vclock = 50-150\n"
	,COLOR_LOW);

	*pbuffer = 0;
	*hbuffer = 0;
	*vbuffer = 0;
	state = 0;
	while (state >= 0 && state != 4) {
		draw_text_left(9,2,64,pbuffer,COLOR_NORMAL);
		draw_text_left(9,3,64,hbuffer,COLOR_NORMAL);
		draw_text_left(9,4,64,vbuffer,COLOR_NORMAL);
		switch (state) {
			case 0 :
				if (draw_text_read(9,2,pbuffer,64,COLOR_REVERSE)!=INPUTB_ENTER)
					state = -1;
				else
					state = 1;
				break;
			case 1 :
				if (draw_text_read(9,3,hbuffer,64,COLOR_REVERSE)!=INPUTB_ENTER)
					state = 0;
				else
					state = 2;
				break;
			case 2 :
				if (draw_text_read(9,4,vbuffer,64,COLOR_REVERSE)!=INPUTB_ENTER)
					state = 1;
				else
					state = 3;
				break;
			case 3 :
				if (monitor_parse(monitor,pbuffer,hbuffer,vbuffer) == 0) {
					state = 4;
				} else {
					state = 0;
					sound_error();
				}
		}
	}

	if (state < 0)
		return -1;
	else
		return 0;
}

static adv_error adjust(const char* msg, adv_crtc* crtc, unsigned index, const adv_monitor* monitor, int only_h_center) {
	int done = 0;
	int modify = 1;
	int first = 1;
	int userkey = INPUTB_ESC;
	adv_crtc current = *crtc;

	double hclock = crtc->pixelclock / crtc->ht;

	adv_mode mode;

	while (!done) {
		if (modify) {
			mode_reset(&mode);
			if (crtc_adjust_clock(&current, monitor)==0
				&& crtc_is_valid(&current)
				&& crtc_clock_check(monitor,&current)
				&& video_mode_generate(&mode, &current, index)==0) {
				video_mode_done(1);
				if (video_mode_set(&mode)!=0) {
					text_done();
					target_err("Error setting the calibration mode.\n");
					target_err("%s\n",error_get());
					target_flush();
					exit(EXIT_FAILURE);
				}
				*crtc = current;
				modify = 0;
				
				video_write_lock();
				draw_test(2,2,msg,&current,1);
				video_write_unlock(0,0,0,0);
			} else {
				if (first) {
					text_done();
					target_err("Error in the test mode.\n");
					target_err("%s\n",error_get());
					target_flush();
					exit(EXIT_FAILURE);
				}
				sound_error();
			}
		}

		first = 0;

		current = *crtc;

		target_idle();
		os_poll();

		userkey = inputb_get();

		if (only_h_center) {
			switch (userkey) {
				case INPUTB_LEFT :
				case INPUTB_RIGHT :
				case 'i' :
				case 'I' :
				case INPUTB_ENTER:
				case INPUTB_ESC:
				break;
				default:
					sound_warn();
					userkey = 0;
			}
		}

		switch (userkey) {
			case INPUTB_ENTER:
			case INPUTB_ESC:
				done = 1;
				break;
			case 'i' :
				current.ht -= current.ht % 8;
				current.ht -= 8;
				current.pixelclock = hclock * current.ht;
				modify = 1;
				break;
			case 'I' :
				current.ht -= current.ht % 8;
				current.ht += 8;
				current.pixelclock = hclock * current.ht;
				modify = 1;
				break;
			case 'K' :
				current.vde -= 1;
				modify = 1;
				break;
			case 'k' :
				current.vde += 1;
				modify = 1;
				break;
			case INPUTB_HOME :
				current.hre -= 8;
				modify = 1;
				break;
			case INPUTB_END :
				current.hre += 8;
				modify = 1;
				break;
			case INPUTB_PGUP :
				current.vre -= 1;
				modify = 1;
				break;
			case INPUTB_PGDN :
				current.vre += 1;
				modify = 1;
				break;
			case INPUTB_LEFT :
				current.hrs -= current.hrs % 8;
				current.hrs += 8;
				current.hre -= current.hre % 8;
				current.hre += 8;
				modify = 1;
				break;
			case INPUTB_RIGHT :
				current.hrs -= current.hrs % 8;
				current.hrs -= 8;
				current.hre -= current.hre % 8;
				current.hre -= 8;
				modify = 1;
				break;
			case INPUTB_DOWN :
				current.vrs -= 1;
				current.vre -= 1;
				modify = 1;
				break;
			case INPUTB_UP :
				current.vrs += 1;
				current.vre += 1;
				modify = 1;
				break;
		}
	}

	return userkey == INPUTB_ENTER ? 0 : -1;
}

static void adjust_fix(const char* msg, adv_crtc* crtc, unsigned index, const adv_monitor* monitor) {
	adv_crtc current = *crtc;

	adv_mode mode;

	mode_reset(&mode);
	if (crtc_adjust_clock(&current, monitor)==0
		&& crtc_is_valid(&current)
		&& crtc_clock_check(monitor,&current)
		&& video_mode_generate(&mode, &current, index)==0) {
		video_mode_done(1);
		if (video_mode_set(&mode)!=0) {
			text_done();
			target_err("Error setting the calibration mode.\n");
			target_err("%s\n",error_get());
			target_flush();
			exit(EXIT_FAILURE);
		}
		
		video_write_lock();
		draw_test(2,2,msg,&current,0);
		video_write_unlock(0,0,0,0);

		do {
			target_idle();
			os_poll();
		} while (inputb_get()==INPUTB_NONE);
	}
}

static adv_error cmd_adjust(const char* msg, adv_generate_interpolate* entry, const adv_generate* generate, const adv_monitor* monitor, unsigned y, unsigned index, double horz_clock, int only_h_center) {
	unsigned x;
	adv_crtc crtc;

	crtc_reset(&crtc);

	x = x_from_y(y);

	generate_crtc(&crtc,x,y,generate);
	crtc_hclock_set(&crtc,horz_clock);

	/* ensure that pclock is in range */
	while (crtc_pclock_get(&crtc) > monitor_pclock_max(monitor)) {
		x = x - 16;
		x = x & ~0xF;
		generate_crtc(&crtc,x,y,generate);
		crtc_hclock_set(&crtc,horz_clock);
	}
	while (crtc_pclock_get(&crtc) < monitor_pclock_min(monitor)) {
		x = x + 16;
		x = x & ~0xF;
		generate_crtc(&crtc,x,y,generate);
		crtc_hclock_set(&crtc,horz_clock);
	}

	if (crtc_adjust_clock(&crtc, monitor)!=0) {
		text_done();
		target_err("Calibration mode unsupported.\n");
		target_err("%s\n",error_get());
		target_flush();
		exit(EXIT_FAILURE);
	}

	if (adjust(msg, &crtc, index, monitor, only_h_center)!=0)
		return -1;

	entry->hclock = crtc.pixelclock / crtc.ht;
	entry->gen.hactive = crtc.hde;
	entry->gen.hfront = crtc.hrs - crtc.hde;
	entry->gen.hsync = crtc.hre - crtc.hrs;
	entry->gen.hback = crtc.ht - crtc.hre;

	entry->gen.vactive = crtc.vde;
	entry->gen.vfront = crtc.vrs - crtc.vde;
	entry->gen.vsync = crtc.vre - crtc.vrs;
	entry->gen.vback = crtc.vt - crtc.vre;

	generate_normalize(&entry->gen);

	return 0;
}

static adv_error cmd_interpolate_one(adv_generate_interpolate_set* interpolate, const adv_generate* generate, const adv_monitor* monitor, unsigned index) {
	int y;
	double ty;
	double hclock, vclock;

	interpolate->mac = 1;

	if (monitor_vclock_check(monitor, 60))
		vclock = 60;
	else if (monitor_vclock_check(monitor, 50))
		vclock = 50;
	else
		vclock = monitor_vclock_min(monitor);

	hclock = monitor_hclock_min(monitor);

	ty = hclock / vclock;
	y = floor( ty * generate->vactive / (generate->vactive + generate->vfront + generate->vsync + generate->vback) );

	if (cmd_adjust("Center and resize the screen (1/1)", interpolate->map + 0, generate, monitor, y, index, hclock, 0)!=0) {
		text_reset();
		return -1;
	}

	text_reset();

	return 0;
}

static adv_error cmd_interpolate_two(adv_generate_interpolate_set* interpolate, const adv_generate* generate, const adv_monitor* monitor, unsigned index) {
	int y;
	double ty,vclock,hclock;
	adv_generate current = *generate;

	interpolate->mac = 2;

	if (monitor_vclock_check(monitor, 60))
		vclock = 60;
	else if (monitor_vclock_check(monitor, 50))
		vclock = 50;
	else
		vclock = monitor_vclock_min(monitor);

	ty = monitor_hclock_min(monitor) / vclock;
	y = ceil( ty * current.vactive / (current.vactive + current.vfront + current.vsync + current.vback));
	hclock = monitor_hclock_min(monitor);

	/* not too small */
	if (y<192)
		y = 192;

	if (cmd_adjust("Center and resize the screen (1/2)", interpolate->map + 0, &current, monitor, y, index, hclock, 0)!=0) {
		text_reset();
		return -1;
	}

	/* start from the previous */
	current = interpolate->map[0].gen;

	ty = monitor_hclock_max(monitor) / vclock;
	y = floor( ty * current.vactive / (current.vactive + current.vfront + current.vsync + current.vback) );
	hclock = monitor_hclock_max(monitor);

	/* not too big */
	if (y>768)
		y = 768;

	if (cmd_adjust("Center the screen (2/2)", interpolate->map + 1, &current, monitor, y, index, hclock, 1)!=0) {
		text_reset();
		return -1;
	}

	text_reset();

	return 0;
}

/***************************************************************************/
/* interpolate */

enum interpolate_enum {
	interpolate_done,
	interpolate_mode
};

struct interpolate_data_struct {
	adv_bool selected;
	unsigned type;
	adv_crtc crtc;
	unsigned x;
	unsigned y;
	adv_bool valid;
};

static void entry_interpolate(int x, int y, int dx, void* data, int n, adv_bool selected) {
	char buffer[1024];
	struct interpolate_data_struct* p = ((struct interpolate_data_struct*)data) + n;
	double vclock;
	double hclock;
	const char* pivot;
	const char* range;

	switch (p->type) {
		case interpolate_mode :
			hclock = p->crtc.pixelclock / p->crtc.ht;
			vclock = hclock / p->crtc.vt;
			if (p->selected && p->valid)
				pivot = " - REFERENCE";
			else
				pivot = "";
			if (!p->valid)
				range = " - OUT OF RANGE";
			else
				range = "";
			sprintf(buffer,"Mode %4dx%4d %.1f/%.1f %s%s",p->x,p->y,hclock / 1E3,vclock,pivot,range);
			break;
		case interpolate_done :
			strcpy(buffer,"Done");
			break;
	}

	draw_text_left(x,y,dx,buffer, selected ? COLOR_REVERSE : COLOR_NORMAL);
}

static void interpolate_create(struct interpolate_data_struct* data, unsigned mac, const adv_generate* generate, const adv_monitor* monitor, double vclock) {
	unsigned i;

	for(i=0;i<mac;++i) {
		if (data[i].type == interpolate_mode) {
			adv_crtc* crtc = &data[i].crtc;
			unsigned x;
			unsigned y;

			crtc_reset(crtc);

			y = data[i].y;
			x = data[i].x;

			generate_crtc(crtc,x,y,generate);
			crtc_vclock_set(crtc,vclock);

			data[i].valid = crtc_adjust_clock(crtc, monitor)==0 && crtc_clock_check(monitor,crtc);
		}
	}
}

static void interpolate_update(adv_generate_interpolate_set* interpolate, struct interpolate_data_struct* data, unsigned mac, const adv_generate* generate, const adv_monitor* monitor, double vclock) {
	unsigned i;
	interpolate->mac = 0;

	for(i=0;i<mac;++i) {
		if (data[i].type == interpolate_mode && data[i].selected && data[i].valid) {
			if (interpolate->mac < GENERATE_INTERPOLATE_MAX) {
				adv_generate_interpolate* entry = &interpolate->map[interpolate->mac];
				const adv_crtc* crtc = &data[i].crtc;

				entry->hclock = crtc->pixelclock / crtc->ht;
				entry->gen.hactive = crtc->hde;
				entry->gen.hfront = crtc->hrs - crtc->hde;
				entry->gen.hsync = crtc->hre - crtc->hrs;
				entry->gen.hback = crtc->ht - crtc->hre;

				entry->gen.vactive = crtc->vde;
				entry->gen.vfront = crtc->vrs - crtc->vde;
				entry->gen.vsync = crtc->vre - crtc->vrs;
				entry->gen.vback = crtc->vt - crtc->vre;

				generate_normalize(&entry->gen);

				++interpolate->mac;
			} else {
				data[i].selected = 0;
			}
		}
	}

	/* update the list of crtc */
	if (interpolate->mac) {
		for(i=0;i<mac;++i) {
			if (data[i].type == interpolate_mode && !data[i].selected) {
				data[i].valid = generate_find_interpolate_double(&data[i].crtc, data[i].x, data[i].y, vclock, monitor, interpolate, VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN, GENERATE_ADJUST_EXACT | GENERATE_ADJUST_VCLOCK | GENERATE_ADJUST_VTOTAL)==0;
			}
		}
	} else {
		interpolate_create(data, mac, generate, monitor, vclock);
	}
}

static adv_error interpolate_test(const char* msg, adv_crtc* crtc, const adv_monitor* monitor, unsigned index) {
	adv_error res;

	adv_mode mode;

	mode_reset(&mode);
	if (video_mode_generate(&mode, crtc, index)!=0) {
		return -1;
	}

	video_mode_done(1);
	if (video_mode_set(&mode)!=0) {
		text_reset();
		return -1;
	}

	res = adjust(msg, crtc, index, monitor, 0);

	text_reset();

	return res;
}

static adv_error cmd_interpolate_many(adv_generate_interpolate_set* interpolate, const adv_generate* generate, const adv_monitor* monitor, unsigned index) {
	unsigned mac = 0;
	adv_error res;
	int ymin, ymax, ymins, ymaxs;
	double ty;
	int y;
	int base;
	int pos;
	struct interpolate_data_struct data[128];
	double vclock;
	int key;

	if (monitor_vclock_check(monitor, 60))
		vclock = 60;
	else if (monitor_vclock_check(monitor, 50))
		vclock = 50;
	else
		vclock = monitor_vclock_min(monitor);

	ty = monitor_hclock_max(monitor) / vclock;
	ymaxs = floor( ty * generate->vactive / (generate->vactive + generate->vfront + generate->vsync + generate->vback) );

	ymax = ymaxs;
	ymax = ymax & ~0xf;

	ty = monitor_hclock_min(monitor) / vclock;
	ymins = ceil( ty * generate->vactive / (generate->vactive + generate->vfront + generate->vsync + generate->vback));

	ymin = ymins;
	ymin = (ymin + 0xf) & ~0xf;

	mac = 0;

	data[mac].type = interpolate_done;
	++mac;

	data[mac].selected = 0;
	data[mac].type = interpolate_mode;
	data[mac].y = ymin;
	data[mac].x = x_from_y(ymin);
	data[mac].valid = 0;
	ymin += 16;
	++mac;

	while (ymin <= ymax) {
		data[mac].selected = 0;
		data[mac].type = interpolate_mode;
		data[mac].y = ymin;
		data[mac].x = x_from_y(ymin);
		data[mac].valid = 0;
		ymin += 16;
		++mac;
	}

	interpolate_create(data, mac, generate, monitor, vclock);

	base = 0;
	pos = 0;
	do {
		interpolate_update(interpolate, data, mac, generate, monitor, vclock);

		text_clear();

		draw_text_bar();

		y = 2;

		y = draw_text_para(0,y,text_size_x(),8,
"Now you must select and adjust some refecence video modes. "
"You can select them pressing SPACE and adjust them pressing ENTER. "
"When you set a refecence mode all the near modes are adjusted likely. "
"\n\n) Select and center horizontally and vertically the first video mode.\n"
") Select and center horizontally a more high resolution mode.\n"
") Check some intermediate modes, if they are wrong select and center them.\n"
		,COLOR_LOW);

		draw_text_string(0,++y,"Select the video mode to adjust:",COLOR_BOLD);
		++y;

		res = draw_text_menu(2,y,text_size_x() - 4,text_size_y() - 2 - y,&data,mac,entry_interpolate,0, &base, &pos, &key);

		if (res >= 0 && data[res].type == interpolate_mode) {
			if (key == INPUTB_ENTER) {
				if (data[res].valid) {
					if (interpolate_test("Center and resize the screen", &data[res].crtc,monitor,index)==0)
						data[res].selected = 1;
				}
			} else
				data[res].selected = !data[res].selected;
		}
	} while (res >= 0 && data[res].type == interpolate_mode);

	if (res<0)
		return -1;

	return 0;
}

/***************************************************************************/
/* Adjust msg */

enum adjust_enum {
	adjust_previous,
	adjust_easy,
	adjust_precise
};

struct adjust_data_struct {
	enum adjust_enum type;
};

static void entry_adjust(int x, int y, int dx, void* data, int n, adv_bool selected) {
	char buffer[1024];
	struct adjust_data_struct* p = ((struct adjust_data_struct*)data) + n;

	switch (p->type) {
		case adjust_previous:
			sprintf(buffer,"Previous centering settings");
			break;
		case adjust_easy:
			sprintf(buffer,"Easy centering - SUGGESTED - (one or two reference modes)");
			break;
		case adjust_precise:
			sprintf(buffer,"Precise centering - (many reference modes)");
			break;
	}

	draw_text_left(x,y,dx,buffer, selected ? COLOR_REVERSE : COLOR_NORMAL);
}

adv_error cmd_adjust_msg(int type, enum adjust_enum* adjust_type) {
	int y;
	struct adjust_data_struct data[6];
	unsigned mac;
	adv_error res;

	text_clear();
	draw_text_bar();

	y = draw_text_para(0,2,text_size_x(),text_size_y()-7,
"The next step requires to center and to resize some test video modes.\n\n"
"You MUST use only the software control to change the size and the position "
"of the video mode. (You can eventually adjust only the FIRST video mode "
"with the monitor controls if these setting will be shared on all video modes. "
"Generally this doesn't happen with modern PC MultiSync monitor)"
"\n\n"
"If you can't correctly adjust the video modes you can't use the automatic "
"configuration of the Advance programs and you should follow the manual configuration."
"\n\n"
"When the image is centered and fit the whole screen press ENTER to go forward."
	,COLOR_LOW);

	mac = 0;

	if (type == monitor_previous) {
		data[mac].type = adjust_previous;
		++mac;
	}

	data[mac].type = adjust_easy;
	++mac;

	data[mac].type = adjust_precise;
	++mac;

	draw_text_string(0,++y,"Select the configuration method:",COLOR_BOLD);
	++y;

	res = draw_text_menu(2,y,text_size_x() - 4,text_size_y() - 2 - y,&data,mac,entry_adjust,0, 0, 0, 0);

	if (res >= 0) {
		*adjust_type = data[res].type;
		return 0;
	} else {
		return -1;
	}
}

/***************************************************************************/
/* Test */

enum test_enum {
	test_mode,
	test_exit,
	test_save,
	test_custom_single,
	test_custom_double,
	test_custom_interlace,
	test_custom
};

struct test_data_struct {
	unsigned type;
	unsigned x;
	unsigned y;
};

static void entry_test(int x, int y, int dx, void* data, int n, adv_bool selected) {
	char buffer[1024];
	struct test_data_struct* p = ((struct test_data_struct*)data) + n;

	switch (p->type) {
		case test_mode :
			sprintf(buffer,"Test mode %4dx%4d (singlescan)",p->x,p->y);
			break;
		case test_save :
			strcpy(buffer,"Save & Exit");
			break;
		case test_exit :
			strcpy(buffer,"Exit without saving");
			break;
		case test_custom_single:
			strcpy(buffer,"Test custom (singlescan) - Use only singlescan modes");
			break;
		case test_custom_double:
			strcpy(buffer,"Test custom (doublescan) - Use only doublescan modes");
			break;
		case test_custom_interlace:
			strcpy(buffer,"Test custom (interlace) - Use only interlace modes");
			break;
		case test_custom:
			strcpy(buffer,"Test custom - Use the nearest available mode (like AdvanceMAME)");
			break;
	}

	draw_text_left(x,y,dx,buffer, selected ? COLOR_REVERSE : COLOR_NORMAL);
}

adv_error cmd_test_mode(adv_generate_interpolate_set* interpolate, const adv_monitor* monitor, int x, int y, double vclock, unsigned index, unsigned cap, adv_bool calib) {
	adv_crtc crtc;

	adv_mode mode;

	if (generate_find_interpolate_double(&crtc, x, y, vclock, monitor, interpolate, cap, GENERATE_ADJUST_EXACT | GENERATE_ADJUST_VCLOCK | GENERATE_ADJUST_VTOTAL)!=0) {
		return -1;
	}

	mode_reset(&mode);
	if (video_mode_generate(&mode, &crtc, index)!=0) {
		return -1;
	}

	video_mode_done(1);
	if (video_mode_set(&mode)!=0) {
		text_reset();
		return -1;
	}

	if (calib) {
		draw_graphics_calib(0,0,video_size_x(),video_size_y());
		
		do {
			target_idle();
			os_poll();
		} while (inputb_get()==INPUTB_NONE);
	} else {
		adjust_fix("Verify the mode", &crtc, index, monitor);
	}

	text_reset();

	return 0;
}

static adv_error cmd_test_custom(int* x, int* y, double* vclock) {
	int state;
	char xbuffer[64];
	char ybuffer[64];
	char vbuffer[64];

	text_clear();
	draw_text_bar();
	draw_text_left(0,2,text_size_x(),"Xsize [pixel] = ",COLOR_NORMAL);
	draw_text_left(0,3,text_size_x(),"Ysize [pixel] = ",COLOR_NORMAL);
	draw_text_left(0,4,text_size_x(),"Vclock [Hz]   = ",COLOR_NORMAL);
	draw_text_para(0,6,text_size_x(),text_size_y()-7,
"Enter the resolution and the Vclock desiderated.\n\n"
	,COLOR_LOW);

	*xbuffer = 0;
	*ybuffer = 0;
	*vbuffer = 0;
	state = 0;
	while (state >= 0 && state != 4) {
		draw_text_left(16,2,64,xbuffer,COLOR_NORMAL);
		draw_text_left(16,3,64,ybuffer,COLOR_NORMAL);
		draw_text_left(16,4,64,vbuffer,COLOR_NORMAL);
		switch (state) {
			case 0 :
				if (draw_text_read(16,2,xbuffer,64,COLOR_REVERSE)!=INPUTB_ENTER)
					state = -1;
				else
					state = 1;
				break;
			case 1 :
				if (draw_text_read(16,3,ybuffer,64,COLOR_REVERSE)!=INPUTB_ENTER)
					state = 0;
				else
					state = 2;
				break;
			case 2 :
				if (draw_text_read(16,4,vbuffer,64,COLOR_REVERSE)!=INPUTB_ENTER)
					state = 1;
				else
					state = 3;
				break;
			case 3 :
				*x = atoi(xbuffer);
				*y = atoi(ybuffer);
				*vclock = atof(vbuffer);
				if (100 < *x && *x < 1920 && 100 < *y && *y < 1280 && 30 < *vclock && *vclock < 200) {
					state = 4;
				} else {
					state = 0;
					sound_error();
				}
		}
	}

	if (state < 0)
		return -1;
	else
		return 0;
}

static adv_error cmd_test(adv_generate_interpolate_set* interpolate, const adv_monitor* monitor, unsigned index) {
	unsigned mac = 0;
	adv_error res;
	int ymin, ymax, ymins, ymaxs;
	double ty;
	int y;
	adv_generate generate;
	int base;
	int pos;

	struct test_data_struct data[128];

	generate_interpolate_h(&generate,monitor_hclock_max(monitor),interpolate);
	ty = monitor_hclock_max(monitor) / monitor_vclock_min(monitor);
	ymaxs = floor( ty * generate.vactive / (generate.vactive + generate.vfront + generate.vsync + generate.vback) );

	ymax = ymaxs;
	/* not too big */
	if (ymax>768)
		ymax = 768;

	ymax = ymax & ~0xf;

	generate_interpolate_h(&generate,monitor_hclock_min(monitor),interpolate);
	ty = monitor_hclock_min(monitor) / monitor_vclock_max(monitor);
	ymins = ceil( ty * generate.vactive / (generate.vactive + generate.vfront + generate.vsync + generate.vback));

	ymin = ymins;
	if (ymin < 192)
		ymin = 192;

	ymin = (ymin + 0xf) & ~0xf;

	mac = 0;

	data[mac].type = test_save;
	++mac;

	data[mac].type = test_exit;
	++mac;

	if ((VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN & video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0)) != 0) {
		data[mac].type = test_custom_single;
		++mac;
	}

	if ((VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN & video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0)) != 0) {
		data[mac].type = test_custom_double;
		++mac;
	}

	if ((VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE & video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0)) != 0) {
		data[mac].type = test_custom_interlace;
		++mac;
	}

	data[mac].type = test_custom;
	++mac;

	if (ymin != ymins) {
		data[mac].type = test_mode;
		data[mac].y = ymins;
		data[mac].x = x_from_y(ymins);
		++mac;
	}

	while (ymin <= ymax) {
		data[mac].type = test_mode;
		data[mac].y = ymin;
		data[mac].x = x_from_y(ymin);
		ymin += 16;
		++mac;
	}

	if (ymax != ymaxs) {
		data[mac].type = test_mode;
		data[mac].y = ymaxs;
		data[mac].x = x_from_y(ymaxs);
		++mac;
	}

	base = 0;
	pos = 0;
	do {
		text_clear();

		draw_text_bar();

		y = 2;

		y = draw_text_para(0,y,text_size_x(),4,
"Now you can test various video modes. If they are correctly centered "
"and sized save the configuration and exit.\n"
"Otherwise you can go back pressing ESC or exit without saving and try "
"the manual configuration."
		,COLOR_LOW);

		draw_text_string(0,++y,"Select the video mode to test:",COLOR_BOLD);
		++y;

		res = draw_text_menu(2,y,text_size_x() - 4,text_size_y() - 2 - y,&data,mac,entry_test,0, &base, &pos, 0);
		if (res >= 0 && data[res].type == test_mode) {
			cmd_test_mode(interpolate,monitor,data[res].x,data[res].y,60, index,VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN,1);
		}
		if (res >= 0 && data[res].type == test_custom_single) {
			int x,y;
			double vclock;
			if (cmd_test_custom(&x,&y,&vclock) == 0)
				cmd_test_mode(interpolate,monitor,x,y,vclock,index,VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN, 0);
		}
		if (res >= 0 && data[res].type == test_custom_double) {
			int x,y;
			double vclock;
			if (cmd_test_custom(&x,&y,&vclock) == 0)
				cmd_test_mode(interpolate,monitor,x,y,vclock,index,VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN, 0);
		}
		if (res >= 0 && data[res].type == test_custom_interlace) {
			int x,y;
			double vclock;
			if (cmd_test_custom(&x,&y,&vclock) == 0)
				cmd_test_mode(interpolate,monitor,x,y,vclock,index,VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE, 0);
		}
		if (res >= 0 && data[res].type == test_custom) {
			int x,y;
			double vclock;
			if (cmd_test_custom(&x,&y,&vclock) == 0)
				cmd_test_mode(interpolate,monitor,x,y,vclock,index,video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0), 0);
		}
	} while (res >= 0 && (data[res].type == test_mode || data[res].type == test_custom_single || data[res].type == test_custom_double || data[res].type == test_custom_interlace || data[res].type == test_custom));

	if (res<0)
		return -1;
	return 0;
}

/***************************************************************************/
/* Save */

void cmd_save(adv_conf* config, const adv_generate_interpolate_set* interpolate, const adv_monitor* monitor, int type)
{
	switch (the_advance) {
	case advance_mame :
	case advance_mess :
	case advance_pac :
		/* set the specific AdvanceMAME options to enable the generate mode. */
		conf_string_set(config,"","display_mode","auto");
		conf_string_set(config,"","display_adjust","generate");
		break;
	case advance_menu:
		break;
	}
	generate_interpolate_save(config,interpolate);
	monitor_save(config,monitor);
}

/***************************************************************************/
/* Main */

void adv_svgalib_log_va(const char *text, va_list arg)
{
	log_va(text,arg);
}

static void error_callback(void* context, enum conf_callback_error error, const char* file, const char* tag, const char* valid, const char* desc, ...) {
	va_list arg;
	va_start(arg, desc);
	target_err_va(desc, arg);
	target_err("\n");
	if (valid)
		target_err("%s\n", valid);
	va_end(arg);
}

static adv_conf_conv STANDARD[] = {
{ "*", "*", "*", "%s", "%s", "%s", 1 }
};

void os_signal(int signum) {
	os_default_signal(signum);
}

int os_main(int argc, char* argv[]) {
	adv_generate generate;
	adv_generate_interpolate_set interpolate;
	enum monitor_enum type;
	enum adjust_enum adjust_type;
	adv_monitor monitor;
	adv_monitor* monitor_loaded;
	int state;
	unsigned index;
	const char* opt_rc;
	adv_bool opt_log;
	adv_bool opt_logsync;
	const char* section_map[1];
	unsigned j;
	adv_conf* config = 0;
	unsigned bit_flag;
	adv_crtc_container mode;
	adv_crtc_container mode_unsorted;
	adv_crtc_container_iterator i;
	adv_error res;

	state = 0;
	index = MODE_FLAGS_INDEX_BGR8;
	opt_rc = 0;
	opt_log = 0;
	opt_logsync = 0;
	the_advance = advance_mame;

	config = conf_init();

	if (os_init(config)!=0) {
		target_err("Error initializing the OS support.\n");
		goto err;
	}

	video_reg(config, 1);
	video_reg_driver_all(config);

	inputb_reg(config, 1);
	inputb_reg_driver_all(config);

	monitor_register(config);
	crtc_container_register(config);
	generate_interpolate_register(config);
	conf_string_register(config,"display_mode");
	conf_string_register(config,"display_adjust");

	if (conf_input_args_load(config, 1, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	for(j=1;j<argc;++j) {
		if (target_option(argv[j],"rc") && j+1<argc) {
			opt_rc = argv[++j];
		} else if (target_option(argv[j],"log")) {
			opt_log = 1;
		} else if (target_option(argv[j],"logsync")) {
			opt_logsync = 1;
		} else if (target_option(argv[j],"advmamec")) {
			the_advance = advance_mame;
		} else if (target_option(argv[j],"advmessc")) {
			the_advance = advance_mess;
		} else if (target_option(argv[j],"advpacc")) {
			the_advance = advance_pac;
		} else if (target_option(argv[j],"advmenuc")) {
			the_advance = advance_menu;
		} else if (target_option(argv[j],"bit") && j+1<argc) {
			unsigned bits = atoi(argv[++j]);
			switch (bits) {
				case 8 : index = MODE_FLAGS_INDEX_BGR8; break;
				case 15 : index = MODE_FLAGS_INDEX_BGR15; break;
				case 16 : index = MODE_FLAGS_INDEX_BGR16; break;
				case 24 : index = MODE_FLAGS_INDEX_BGR24; break;
				case 32 : index = MODE_FLAGS_INDEX_BGR32; break;
				default:
					target_err("Invalid bit depth %d.\n", bits);
					goto err_os;
			}
		} else {
			target_err("Unknown option %s.\n",argv[j]);
			goto err_os;
		}
	}

	if (!opt_rc) {
		switch (the_advance) {
			case advance_menu : opt_rc = file_config_file_home("advmenu.rc"); break;
			case advance_mame : opt_rc = file_config_file_home("advmame.rc"); break;
			case advance_mess : opt_rc = file_config_file_home("advmess.rc"); break;
			case advance_pac : opt_rc = file_config_file_home("advpac.rc"); break;
		}
	}

	if (conf_input_file_load_adv(config, 0, opt_rc, opt_rc, 1, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0)
		goto err_os;

	if (opt_log || opt_logsync) {
		const char* log = "advcfg.log";
		remove(log);
		log_init(log,opt_logsync);
        }

	log_std(("cfg: %s %s\n",__DATE__,__TIME__));

	section_map[0] = "";
	conf_section_set(config, section_map, 1);

	if (video_load(config, "") != 0) {
		target_err("Error loading the video options from the configuration file %s.\n", opt_rc);
		target_err("%s\n",error_get());
		goto err_os;
	}

	if (inputb_load(config) != 0) {
		goto err_mode;
	}

	if (os_inner_init("AdvanceCFG") != 0) {
		goto err_mode;
	}

	if (video_init() != 0) {
		target_err("%s\n", error_get());
		goto err_inner;
	}

	if (video_blit_init() != 0) {
		target_err("%s\n", error_get());
		goto err_video;
	}

	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK) == 0) {
		target_err("No driver is able to program your video board in this context.\n");
		target_err("Ensure to use the 'device_video_output auto' option.\n");
#ifdef __WIN32__
		target_err("Ensure to have installed the svgawin.sys driver with the svgawin.exe utility.\n");
#endif
#ifdef unix
		target_err("Ensure to not run this program in X.\n");
#endif
		goto err_blit;
	}

	switch (index) {
		case MODE_FLAGS_INDEX_BGR8 : bit_flag = VIDEO_DRIVER_FLAGS_MODE_BGR8; break;
		case MODE_FLAGS_INDEX_BGR15 : bit_flag = VIDEO_DRIVER_FLAGS_MODE_BGR15; break;
		case MODE_FLAGS_INDEX_BGR16 : bit_flag = VIDEO_DRIVER_FLAGS_MODE_BGR16; break;
		case MODE_FLAGS_INDEX_BGR24 : bit_flag = VIDEO_DRIVER_FLAGS_MODE_BGR24; break;
		case MODE_FLAGS_INDEX_BGR32 : bit_flag = VIDEO_DRIVER_FLAGS_MODE_BGR32; break;
		default:
			target_err("Invalid bit depth specification.\n");
			goto err_blit;
	}

	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & bit_flag) == 0) {
		target_err("Specified bit depth isn't supported.\n");
		target_err("Try another value with the -bit option.\n");
		goto err_blit;
	}

	if (inputb_init() != 0) {
		target_err("%s\n", error_get());
		goto err_blit;
	}

	res = monitor_load(config, &monitor);
	if (res < 0) {
		printf("Error loading the clock options from the configuration file %s\n", opt_rc);
		printf("%s\n", error_get());
		goto err_blit;
	}
	if (res == 0) {
		monitor_loaded = &monitor;
	} else {
		monitor_loaded = 0;
	}

	/* load selected */
	crtc_container_init(&mode_unsorted);

	if (crtc_container_load(config, &mode_unsorted) != 0) {
		target_err(error_get());
		goto err_input;
	}

	crtc_container_insert_default_system(&mode_unsorted);

	crtc_container_init(&mode);
	for(crtc_container_iterator_begin(&i, &mode_unsorted);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
		adv_crtc* crtc = crtc_container_iterator_get(&i);
		crtc_container_insert_sort(&mode, crtc, crtc_compare);
	}
	crtc_container_done(&mode_unsorted);

	if (text_init(&mode, monitor_loaded) != 0) {
		goto err_mode;
	}

	while (state >= 0 && state != 8) {

		switch (state) {
			case 0 :
				res = cmd_monitor(config, &generate,&type,&interpolate);
				if (res == -1)
					state = -1;
				else if (type == monitor_custom)
					state = 1;
				else
					state = 2;
				break;
			case 1 :
				res = cmd_monitor_custom(&generate);
				if (res == -1)
					state = 0;
				else if (type == monitor_custom)
					state = 3;
				else
					state = 2;
				break;
			case 2 :
				res = cmd_model(config,&monitor);
				if (res == -1)
					state = 0;
				else if (monitor_is_empty(&monitor))
					state = 3;
				else
					state = 4;
				break;
			case 3 :
				res = cmd_model_custom(&monitor);
				if (res == -1)
					state = 2;
				else
					state = 4;
				break;
			case 4 :
				res = cmd_adjust_msg(type,&adjust_type);
				if (res < 0)
					state = 2;
				else {
					if (adjust_type == adjust_previous) {
						state = 5;
					} else if (adjust_type == adjust_easy) {
						if (monitor_hclock_min(&monitor) == monitor_hclock_max(&monitor))
							res = cmd_interpolate_one(&interpolate,&generate,&monitor,index);
						else
							res = cmd_interpolate_two(&interpolate,&generate,&monitor,index);
					} else {
						res = cmd_interpolate_many(&interpolate,&generate,&monitor,index);
					}
					if (res >= 0)
						state = 5;
				}
				break;
			case 5 :
				res = cmd_test(&interpolate,&monitor, index);
				if (res == -1)
					state = 4;
				else
					state = 6;
				break;
			case 6 :
				cmd_save(config,&interpolate,&monitor,type);
				state = 8;
				break;
		}
	}

	log_std(("cfg: shutdown\n"));

	text_done();

	crtc_container_done(&mode);

	inputb_done();

	video_done();

	os_inner_done();

	log_std(("cfg: the end\n"));

	if (opt_log || opt_logsync) {
		log_done();
	}

	os_done();

	conf_save(config,0);

	conf_done(config);

	return EXIT_SUCCESS;

err_mode:
	crtc_container_done(&mode);
err_input:
	inputb_done();
err_blit:
	video_blit_done();
err_video:
	video_done();
err_inner:
	os_inner_done();
err_os:
	if (opt_log || opt_logsync) {
		log_done();
	}
	os_done();
	conf_done(config);
err:
	return EXIT_FAILURE;
}


