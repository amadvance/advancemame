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

#include "video.h"
#include "generate.h"
#include "gtf.h"
#include "crtcbag.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/***************************************************************************/
/* Parse */

/** Last parse error */
static char video_mode_parse_error[1024];

static adv_error parse_int(int* r, const char** begin, const char* end) {
	char* e;
	*r = strtol(*begin,&e,10);
	if (e==*begin || e > end) {
		sprintf(video_mode_parse_error,"Error in value");
		return -1;
	}
	*begin = e;
	return 0;
}

static void parse_separator(const char* separator, const char** begin, const char* end) {
	while (*begin < end && strchr(separator,**begin))
		++*begin;
}

static adv_error parse_token(char* token, unsigned size, const char* separator, const char** begin, const char* end) {
	unsigned pos = 0;

	while (*begin < end && !strchr(separator,**begin)) {
		if (pos + 2 > size) {
			sprintf(video_mode_parse_error,"Token too long");
			return -1;
		}
		token[pos] = **begin;
		++pos;
		++*begin;
	}
	if (!pos) {
		sprintf(video_mode_parse_error,"Expected token");
		return -1;
	}
	token[pos] = 0;
	return 0;
}

static adv_error parse_quote(char* token, unsigned size, const char** begin, const char* end) {
	if (*begin == end || **begin != '"')
		return -1;
	++*begin;

	if (parse_token(token,size,"\"",begin,end)!=0)
		return -1;

	if (*begin == end || **begin != '"')
		return -1;
	++*begin;

	return 0;
}

static adv_error parse_double(double* r, const char** begin, const char* end) {
	char* e;
	*r = strtod(*begin,&e);
	if (e==*begin || e > end) {
		sprintf(video_mode_parse_error,"Error in value");
		return -1;
	}
	*begin = e;
	return 0;
}

/***************************************************************************/
/* Video crtc container */

static adv_error parse_crtc(video_crtc* crtc, const char* begin, const char* end) {
	int v;
	double d;

	if (parse_double(&d,&begin,end))
		return -1;
	crtc->pixelclock = d * 1E6;

	parse_separator(" \t",&begin,end);
	if (parse_int(&v,&begin,end))
		return -1;
	crtc->hde = v;

	parse_separator(" \t",&begin,end);
	if (parse_int(&v,&begin,end))
		return -1;
	crtc->hrs = v;

	parse_separator(" \t",&begin,end);
	if (parse_int(&v,&begin,end))
		return -1;
	crtc->hre = v;

	parse_separator(" \t",&begin,end);
	if (parse_int(&v,&begin,end))
		return -1;
	crtc->ht = v;

	parse_separator(" \t",&begin,end);
	if (parse_int(&v,&begin,end))
		return -1;
	crtc->vde = v;

	parse_separator(" \t",&begin,end);
	if (parse_int(&v,&begin,end))
		return -1;
	crtc->vrs = v;

	parse_separator(" \t",&begin,end);
	if (parse_int(&v,&begin,end))
		return -1;
	crtc->vre = v;

	parse_separator(" \t",&begin,end);
	if (parse_int(&v,&begin,end))
		return -1;
	crtc->vt = v;

	parse_separator(" \t",&begin,end);
	while (begin != end) {
		char token[32];
		if (parse_token(token,sizeof(token)," \t",&begin,end))
			return -1;
		if (strcasecmp(token,"doublescan")==0) {
			crtc_doublescan_set(crtc);
		} else if (strcasecmp(token,"+hsync")==0) {
			crtc_phsync_set(crtc);
		} else if (strcasecmp(token,"-hsync")==0) {
			crtc_nhsync_set(crtc);
		} else if (strcasecmp(token,"+vsync")==0) {
			crtc_pvsync_set(crtc);
		} else if (strcasecmp(token,"-vsync")==0) {
			crtc_nvsync_set(crtc);
		} else if (strcasecmp(token,"interlaced")==0) { /* LEGACY for the old modelines */
			crtc_interlace_set(crtc);
		} else if (strcasecmp(token,"interlace")==0) {
			crtc_interlace_set(crtc);
		} else if (strcasecmp(token,"tvpal")==0) {
			crtc_tvpal_set(crtc);
		} else if (strcasecmp(token,"tvntsc")==0) {
			crtc_tvntsc_set(crtc);
		} else {
			if (token[0]=='#')
				return 0; /* comment */
			sprintf(video_mode_parse_error,"Unknown token '%s'",token);
			return -1;
		}
		parse_separator(" \t",&begin,end);
	}

	return 0;
}
/**
 * Parse a string for a crtc.
 * \param begin start of the string to parse
 * \param end end of the string to parse
 * \return
 *  - ==0 on success
 *  - !=0 on error
 */
adv_error video_crtc_parse(video_crtc* crtc, const char* begin, const char* end) {

	/* defaults */
	crtc_reset_all(crtc);

	parse_separator(" \t",&begin,end);

	if (begin != end && *begin == '"') {
		if (parse_quote(crtc->name,VIDEO_NAME_MAX,&begin,end))
			return -1;
	} else {
		if (parse_token(crtc->name,VIDEO_NAME_MAX," \t",&begin,end))
			return -1;
	}

	parse_separator(" \t",&begin,end);
	if (begin == end) {
		sprintf(video_mode_parse_error,"Missing modeline data");
		return -1;
	}

	if (parse_crtc(crtc,begin,end) != 0)
		return -1;

	return 0;
}

void video_crtc_print(char* buffer, const video_crtc* crtc) {
	const char* flag1 = crtc_is_nhsync(crtc) ? " -hsync" : " +hsync";
	const char* flag2 = crtc_is_nvsync(crtc) ? " -vsync" : " +vsync";
	const char* flag3 = crtc_is_doublescan(crtc) ? " doublescan" : "";
	const char* flag4 = crtc_is_interlace(crtc) ? " interlace" : "";
	const char* flag5 = crtc_is_tvpal(crtc) ? " tvpal" : "";
	const char* flag6 = crtc_is_tvntsc(crtc) ? " tvntsc" : "";

	if (strchr(crtc->name,' ')!=0)
		sprintf(buffer, "\"%s\"", crtc->name);
	else
		sprintf(buffer, "%s", crtc->name);

	sprintf(buffer + strlen(buffer)," %g %d %d %d %d %d %d %d %d%s%s%s%s%s%s",
		(double)crtc->pixelclock / 1E6,
		crtc->hde, crtc->hrs, crtc->hre, crtc->ht,
		crtc->vde, crtc->vrs, crtc->vre, crtc->vt,
		flag1, flag2, flag3, flag4, flag5, flag6
	);
}

/* Load the list of video mode */
adv_error video_crtc_container_load(struct conf_context* context, video_crtc_container* cc) {
	conf_iterator i;
	int error = 0;

	conf_iterator_begin(&i, context, "device_video_modeline");
	while (!conf_iterator_is_end(&i)) {
		video_crtc crtc;
		const char* s = conf_iterator_string_get(&i);
		if (video_crtc_parse(&crtc,s, s + strlen(s)) == 0) {
			video_crtc_container_insert(cc,&crtc);
		} else {
			if (!error) {
				error = 1;
				error_description_set("%s in argument '%s' for option 'device_video_modeline'",video_mode_parse_error,s);
			}
		}
		conf_iterator_next(&i);
	}

	return error ? -1 : 0;
}

/* Save the list of video mode */
void video_crtc_container_save(struct conf_context* context, video_crtc_container* cc) {
	video_crtc_container_iterator i;
	conf_remove(context,"","device_video_modeline");
	video_crtc_container_iterator_begin(&i,cc);
	while (!video_crtc_container_iterator_is_end(&i)) {
		char buffer[1024];
		video_crtc_print(buffer,video_crtc_container_iterator_get(&i));
		conf_string_set(context,"","device_video_modeline",buffer);
		video_crtc_container_iterator_next(&i);
	}
}

void video_crtc_container_register(struct conf_context* context) {
	conf_string_register_multi(context,"device_video_modeline");
}

void video_crtc_container_clear(struct conf_context* context) {
	conf_remove(context,"","device_video_modeline");
}

/***************************************************************************/
/* Monitor */

static adv_error monitor_range_parse(video_monitor_range* range, const char* begin, const char* end, double mult) {
	unsigned i = 0;
	parse_separator(" \t",&begin,end);
	while (begin < end) {
		double v0;
		if (i==VIDEO_MONITOR_RANGE_MAX) {
			return -1;
		}
		if (parse_double(&v0,&begin,end))
			return -1;
		parse_separator(" \t",&begin,end);
		if (*begin=='-') {
			double v1;
			++begin;
			if (parse_double(&v1,&begin,end))
				return -1;
			if (v0 < 0 || v0 > v1 || v1 > 300) {
				return -1;
			}
			range[i].low = mult * v0;
			range[i].high = mult * v1;
			++i;
		} else {
			if (v0 < 0 || v0 > 300) {
				return -1;
			}
			/* the 0 value is used as a null definition */
			if (v0 > 0) {
				range[i].low = mult * v0;
				range[i].high = range[i].low;
				++i;
			}
		}
		parse_separator(" \t",&begin,end);
		if (*begin==',') {
			++begin;
		}
		parse_separator(" \t",&begin,end);
	}
	if (i==0)
		return -1; /* empty */
	while (i<VIDEO_MONITOR_RANGE_MAX) {
		range[i].low = 0;
		range[i].high = 0;
		++i;
	}
	return 0;
}

static adv_error monitor_single_parse(video_monitor_range* range, const char* begin, const char* end, double mult) {
	double v0;
	double v1;

	parse_separator(" \t",&begin,end);

	if (parse_double(&v0,&begin,end))
		return -1;

	parse_separator(" \t",&begin,end);
	if (*begin!='-')
		return -1;
	++begin;
	if (parse_double(&v1,&begin,end))
		return -1;
	if (v0 < 0 || v0 > v1 || v1 > 300) {
		return -1;
	}

	range->low = mult * v0;
	range->high = mult * v1;

	parse_separator(" \t",&begin,end);

	if (begin != end)
		return -1;

	return 0;
}

void monitor_print(char* buffer, const video_monitor_range* range_begin, const video_monitor_range* range_end, double mult)
{
	*buffer = 0;
	while (range_begin != range_end) {
		if (range_begin->low != 0 && range_begin->high != 0) {
			if (*buffer)
				strcat(buffer,", ");
			if (range_begin->low == range_begin->high) {
				sprintf(buffer+strlen(buffer),"%g", range_begin->low / mult);
			} else {
				sprintf(buffer+strlen(buffer),"%g-%g",range_begin->low / mult, range_begin->high / mult);
			}
		}
		++range_begin;
	}

	if (!*buffer)
		strcpy(buffer,"0");
}

adv_error monitor_parse(video_monitor* monitor, const char* p, const char* h, const char* v) {
	if (monitor_single_parse(&monitor->pclock,p,p+strlen(p),1E6)!=0) {
		error_description_set("Invalid monitor 'pclock' specification");
		return -1;
	}
	if (monitor_range_parse(monitor->hclock,h,h+strlen(h),1E3)!=0) {
		error_description_set("Invalid monitor 'hclock' specification");
		return -1;
	}
	if (monitor_range_parse(monitor->vclock,v,v+strlen(v),1)!=0) {
		error_description_set("Invalid monitor 'vclock' specification");
		return -1;
	}
	return 0;
}

/* Load the monitor configuration
 * return:
 *   ==0 success
 *   <0 error
 *   >0 configuration missing, video_monitor_is_empty(monitor)!=0
 */
adv_error monitor_load(struct conf_context* context, video_monitor* monitor) {
	conf_error p_present;
	conf_error h_present;
	conf_error v_present;
	const char* p;
	const char* h;
	const char* v;

	p_present = conf_string_get(context,"device_video_pclock",&p);
	h_present = conf_string_get(context,"device_video_hclock",&h);
	v_present = conf_string_get(context,"device_video_vclock",&v);

	if (p_present!=0 && h_present!=0 && v_present!=0) {
		monitor_reset(monitor);
		error_description_set("Missing options 'device_video_p/h/vclock'");
		return 1;
	}

	if (p_present!=0) {
		error_description_set("Missing option 'device_video_pclock'");
		return -1;
	}
	if (monitor_single_parse(&monitor->pclock,p,p+strlen(p),1E6)!=0) {
		error_description_set("Invalid argument '%s' for option 'device_video_pclock'",p);
		return -1;
	}

	if (h_present!=0) {
		error_description_set("Missing option 'device_video_hclock'");
		return -1;
	}
	if (monitor_range_parse(monitor->hclock,h,h+strlen(h),1000)!=0) {
		error_description_set("Invalid argument '%s' for option 'device_video_hclock'",h);
		return -1;
	}

	if (v_present!=0) {
		error_description_set("Missing option 'device_video_vclock'");
		return -1;
	}
	if (monitor_range_parse(monitor->vclock,v,v+strlen(v),1)!=0) {
		error_description_set("Invalid argument '%s' for option 'device_video_vclock'",v);
		return -1;
	}

	return 0;
}

void monitor_save(struct conf_context* context, const video_monitor* monitor) {
	char buffer[1024];

	monitor_print(buffer,&monitor->pclock,&monitor->pclock + 1,1E6);
	conf_string_set(context,"","device_video_pclock",buffer);

	monitor_print(buffer,monitor->hclock,monitor->hclock + VIDEO_MONITOR_RANGE_MAX,1E3);
	conf_string_set(context,"","device_video_hclock",buffer);

	monitor_print(buffer,monitor->vclock,monitor->vclock + VIDEO_MONITOR_RANGE_MAX,1);
	conf_string_set(context,"","device_video_vclock",buffer);
}

void monitor_register(struct conf_context* context) {
	conf_string_register(context,"device_video_pclock");
	conf_string_register(context,"device_video_hclock");
	conf_string_register(context,"device_video_vclock");
}

/***************************************************************************/
/* Generate */

static adv_error parse_generate(const char* begin, const char* end, video_generate* data) {
	double d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->hactive = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->hfront = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->hsync = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->hback = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->vactive = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->vfront = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->vsync = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->vback = d;
	parse_separator(" \t",&begin,end);
	if (begin != end)
		return -1;

	if (data->hactive <= data->hfront) {
		/* the values are in the incremental format */
		data->hback -= data->hsync;
		data->hsync -= data->hfront;
		data->hfront -= data->hactive;
	}

	if (data->vactive <= data->vfront) {
		/* the values are in the incremental format */
		data->vback -= data->vsync;
		data->vsync -= data->vfront;
		data->vfront -= data->vactive;
	}

	generate_normalize(data);

	return 0;
}

static adv_error parse_generate_interpolate(const char* begin, const char* end, video_generate_interpolate* interpolate) {
	double d;
	video_generate* data = &interpolate->gen;

	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	interpolate->hclock = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->hactive = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->hfront = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->hsync = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->hback = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->vactive = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->vfront = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->vsync = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->vback = d;
	parse_separator(" \t",&begin,end);
	if (begin != end)
		return -1;

	generate_normalize(data);

	return 0;
}

static void out_generate_interpolate(char* buffer, const video_generate_interpolate* interpolate) {
	video_generate out = interpolate->gen;

	generate_normalize(&out);

	sprintf(buffer,"%d %g %g %g %g %g %g %g %g",
		interpolate->hclock,
		out.hactive, out.hfront, out.hsync, out.hback,
		out.vactive, out.vfront, out.vsync, out.vback);
}

adv_error generate_parse(video_generate* generate, const char* g) {
	if (parse_generate(g,g+strlen(g),generate)!=0) {
		error_description_set("Invalid specification");
		return -1;
	}

	return 0;
}

static int generate_interpolate_cmp(const void* e0, const void* e1) {
	const video_generate_interpolate* ee0 = (const video_generate_interpolate*)e0;
	const video_generate_interpolate* ee1 = (const video_generate_interpolate*)e1;
	if (ee0->hclock < ee1->hclock)
		return -1;
	if (ee0->hclock > ee1->hclock)
		return 1;
	return 0;
}

adv_error generate_interpolate_load(struct conf_context* context, video_generate_interpolate_set* interpolate) {
	conf_iterator i;
	unsigned mac = 0;

	conf_iterator_begin(&i, context, "device_video_format");
	while (!conf_iterator_is_end(&i)) {
		const char* s = conf_iterator_string_get(&i);

		if (parse_generate_interpolate(s,s+strlen(s),&interpolate->map[mac])!=0) {
			error_description_set("Invalid argument '%s' in option 'device_video_format'",s);
			return -1;
		}

		++mac;

		conf_iterator_next(&i);
	}

	if (!mac) {
		generate_interpolate_reset(interpolate);
		error_description_set("Missing 'device_video_format' specification");
		return 1;
	}

	/* sort */
	qsort(interpolate->map, mac, sizeof(video_generate_interpolate), generate_interpolate_cmp);

	interpolate->mac = mac;

	return 0;
}

void generate_interpolate_save(struct conf_context* context, const video_generate_interpolate_set* interpolate) {
	unsigned i;
	conf_remove(context,"","device_video_format");
	for(i=0;i<interpolate->mac;++i) {
		char buffer[1024];
		out_generate_interpolate(buffer,&interpolate->map[i]);
		conf_string_set(context,"","device_video_format",buffer);
	}
}

void generate_interpolate_clear(struct conf_context* context) {
	conf_remove(context,"","device_video_format");
}

void generate_interpolate_register(struct conf_context* context) {
	conf_string_register_multi(context, "device_video_format");
}

/***************************************************************************/
/* GTF */

static adv_error parse_gtf(const char* begin, const char* end, video_gtf* data) {
	double d;
	int i;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->margin_frac = d;
	parse_separator(" \t",&begin,end);
	if (parse_int(&i,&begin,end))
		return -1;
	data->v_min_frontporch_lines = i;
	parse_separator(" \t",&begin,end);
	if (parse_int(&i,&begin,end))
		return -1;
	data->v_sync_lines = i;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->h_sync_frac = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->v_min_sync_backporch_time = d / 1E6;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->m = d;
	parse_separator(" \t",&begin,end);
	if (parse_double(&d,&begin,end))
		return -1;
	data->c = d;
	parse_separator(" \t",&begin,end);
	if (begin != end)
		return -1;

	if (data->v_min_frontporch_lines < 0)
		return -1;
	if (data->v_sync_lines < 1)
		return -1;
	if (data->h_sync_frac <= 0)
		return -1;
	if (data->v_min_sync_backporch_time <= 0)
		return -1;
	if (data->m < 0 || data->c < 0)
		return -1;

	return 0;
}

adv_error gtf_parse(video_gtf* gtf, const char* g) {
	if (parse_gtf(g,g+strlen(g),gtf)!=0) {
		error_description_set("Invalid 'gtf' specification");
		return -1;
	}

	return 0;
}

adv_error gtf_load(struct conf_context* context, video_gtf* gtf) {
	const char* s;

	if (conf_string_get(context,"device_video_gtf",&s) != 0) {
		error_description_set("Missing option 'device_video_gtf'");
		return 1;
	}

	if (parse_gtf(s,s+strlen(s),gtf)!=0) {
		error_description_set("Invalid argument '%s' for option 'device_video_gtf'",s);
		return -1;
	}

	return 0;
}

void gtf_save(struct conf_context* context, const video_gtf* gtf) {
	char buffer[1024];

	sprintf(buffer,"%g %d %d %g %g %g %g",
		gtf->margin_frac,
		gtf->v_min_frontporch_lines,
		gtf->v_sync_lines,
		gtf->h_sync_frac,
		gtf->v_min_sync_backporch_time * 1E6,
		gtf->m,
		gtf->c
	);

	conf_string_set(context, "", "device_video_gtf", buffer);
}

void gtf_clear(struct conf_context* context) {
	conf_remove(context, "", "device_video_gtf");
}

void gtf_register(struct conf_context* context) {
	conf_string_register(context, "device_video_gtf");
}

/***************************************************************************/
/* Snapshot */

typedef struct tagBITMAPFILEHEADER {
	uint16 bfType __attribute__ ((packed));
	uint32 bfSize __attribute__ ((packed));
	uint16 bfReserved1 __attribute__ ((packed));
	uint16 bfReserved2 __attribute__ ((packed));
	uint32 bfOffBits __attribute__ ((packed));
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
	uint32 biSize __attribute__ ((packed));
	uint32 biWidth __attribute__ ((packed));
	uint32 biHeight __attribute__ ((packed));
	uint16 biPlanes __attribute__ ((packed));
	uint16 biBitCount __attribute__ ((packed));
	uint32 biCompression __attribute__ ((packed));
	uint32 biSizeImage __attribute__ ((packed));
	uint32 biXPelsPerMeter __attribute__ ((packed));
	uint32 biYPelsPerMeter __attribute__ ((packed));
	uint32 biClrUsed __attribute__ ((packed));
	uint32 biClrImportant __attribute__ ((packed));
} BITMAPINFOHEADER;

adv_error video_snapshot_save(const char* snapshot_name, int start_x, int start_y) {
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	FILE* f;
	unsigned line_size;
	int y;

	if (!video_is_graphics() || !video_is_linear())
		return -1;

	f = fopen(snapshot_name,"wb");
	if (!f)
		return -1;

	line_size = video_size_x() * 3;
	line_size = (line_size + 0x3) & ~0x3;

	memset(&bmfh,0,sizeof(bmfh));
	bmfh.bfType = 19778;
	bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + line_size * video_size_y();
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	memset(&bmih,0,sizeof(bmih));
	bmih.biSize = sizeof(bmih);
	bmih.biWidth = video_size_x();
	bmih.biHeight = video_size_y();
	bmih.biPlanes = 1;
	bmih.biBitCount = 24;

	fwrite(&bmfh,sizeof(bmfh),1,f);
	fwrite(&bmih,sizeof(bmih),1,f);

	for(y=video_size_y()-1;y>=0;--y) {
		unsigned count = 0;
		if (video_index() == VIDEO_FLAGS_INDEX_PACKED) {
			unsigned x;
			uint8* line = video_write_line(y + start_y);
			for(x=0;x<video_size_x();++x) {
				uint8 color = line[x + start_x];
				video_color* palette = video_palette_get() + line[color];
				uint8 b;
				b = palette->blue;
				fwrite(&b,1,1,f);
				b = palette->green;
				fwrite(&b,1,1,f);
				b = palette->red;
				fwrite(&b,1,1,f);
				count += 3;
			}
		} else {
			if (video_bytes_per_pixel() == 1) {
				unsigned x;
				uint8* line = video_write_line(y + start_y);
				for(x=0;x<video_size_x();++x) {
					uint8 color = line[x + start_x];
					uint8 b;
					b = video_blue_get(color);
					fwrite(&b,1,1,f);
					b = video_green_get(color);
					fwrite(&b,1,1,f);
					b = video_red_get(color);
					fwrite(&b,1,1,f);
					count += 3;
				}
			} else if (video_bytes_per_pixel() == 2) {
				unsigned x;
				uint16* line = (uint16*)video_write_line(y + start_y);
				for(x=0;x<video_size_x();++x) {
					uint16 color = line[x+start_x];
					uint8 b;
					b = video_blue_get(color);
					fwrite(&b,1,1,f);
					b = video_green_get(color);
					fwrite(&b,1,1,f);
					b = video_red_get(color);
					fwrite(&b,1,1,f);
					count += 3;
				}
			}
		}
		while (count % 4) {
			uint8 b = 0;
			fwrite(&b,1,1,f);
			++count;
		}
	}

	fclose(f);

	return 0;
}

