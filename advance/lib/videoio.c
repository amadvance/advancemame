/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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

#include "video.h"

#include "generate.h"
#include "gtf.h"
#include "crtcbag.h"
#include "error.h"
#include "snstring.h"

/***************************************************************************/
/* Parse */

/** Last parse error */
static char video_mode_parse_error_buffer[1024];

static adv_error parse_int(int* r, const char** begin, const char* end)
{
	char* e;
	*r = strtol(*begin, &e, 10);
	if (e==*begin || e > end) {
		snprintf(video_mode_parse_error_buffer, sizeof(video_mode_parse_error_buffer), "Error in value");
		return -1;
	}
	*begin = e;
	return 0;
}

static void parse_separator(const char* separator, const char** begin, const char* end)
{
	while (*begin < end && strchr(separator, **begin))
		++*begin;
}

static adv_error parse_token(char* token, unsigned size, const char* separator, const char** begin, const char* end)
{
	unsigned pos = 0;

	while (*begin < end && !strchr(separator, **begin)) {
		if (pos + 2 > size) {
			snprintf(video_mode_parse_error_buffer, sizeof(video_mode_parse_error_buffer), "Token too long");
			return -1;
		}
		token[pos] = **begin;
		++pos;
		++*begin;
	}
	if (!pos) {
		snprintf(video_mode_parse_error_buffer, sizeof(video_mode_parse_error_buffer), "Expected token");
		return -1;
	}
	token[pos] = 0;
	return 0;
}

static adv_error parse_quote(char* token, unsigned size, const char** begin, const char* end)
{
	if (*begin == end || **begin != '"')
		return -1;
	++*begin;

	if (parse_token(token, size, "\"", begin, end)!=0)
		return -1;

	if (*begin == end || **begin != '"')
		return -1;
	++*begin;

	return 0;
}

static adv_error parse_double(double* r, const char** begin, const char* end)
{
	char* e;
	*r = strtod(*begin, &e);
	if (e==*begin || e > end) {
		snprintf(video_mode_parse_error_buffer, sizeof(video_mode_parse_error_buffer), "Error in value");
		return -1;
	}
	*begin = e;
	return 0;
}

/***************************************************************************/
/* Video crtc container */

static adv_error parse_crtc(adv_crtc* crtc, const char* begin, const char* end)
{
	int v;
	double d;

	if (parse_double(&d, &begin, end))
		return -1;
	crtc->pixelclock = d * 1E6;

	parse_separator(" \t", &begin, end);
	if (parse_int(&v, &begin, end))
		return -1;
	crtc->hde = v;

	parse_separator(" \t", &begin, end);
	if (parse_int(&v, &begin, end))
		return -1;
	crtc->hrs = v;

	parse_separator(" \t", &begin, end);
	if (parse_int(&v, &begin, end))
		return -1;
	crtc->hre = v;

	parse_separator(" \t", &begin, end);
	if (parse_int(&v, &begin, end))
		return -1;
	crtc->ht = v;

	parse_separator(" \t", &begin, end);
	if (parse_int(&v, &begin, end))
		return -1;
	crtc->vde = v;

	parse_separator(" \t", &begin, end);
	if (parse_int(&v, &begin, end))
		return -1;
	crtc->vrs = v;

	parse_separator(" \t", &begin, end);
	if (parse_int(&v, &begin, end))
		return -1;
	crtc->vre = v;

	parse_separator(" \t", &begin, end);
	if (parse_int(&v, &begin, end))
		return -1;
	crtc->vt = v;

	parse_separator(" \t", &begin, end);
	while (begin != end) {
		char token[32];
		if (parse_token(token, sizeof(token), " \t", &begin, end))
			return -1;
		if (strcasecmp(token, "doublescan")==0) {
			crtc_doublescan_set(crtc);
		} else if (strcasecmp(token, "+hsync")==0) {
			crtc_phsync_set(crtc);
		} else if (strcasecmp(token, "-hsync")==0) {
			crtc_nhsync_set(crtc);
		} else if (strcasecmp(token, "+vsync")==0) {
			crtc_pvsync_set(crtc);
		} else if (strcasecmp(token, "-vsync")==0) {
			crtc_nvsync_set(crtc);
		} else if (strcasecmp(token, "interlaced")==0) { /* LEGACY for the old modelines */
			crtc_interlace_set(crtc);
		} else if (strcasecmp(token, "interlace")==0) {
			crtc_interlace_set(crtc);
		} else if (strcasecmp(token, "tvpal")==0) {
			/* ignore */
		} else if (strcasecmp(token, "tvntsc")==0) {
			/* ignore */
		} else {
			if (token[0]=='#')
				return 0; /* comment */
			snprintf(video_mode_parse_error_buffer, sizeof(video_mode_parse_error_buffer), "Unknown token '%s'", token);
			return -1;
		}
		parse_separator(" \t", &begin, end);
	}

	return 0;
}
/**
 * Parse a string for a crtc.
 * \param crtc Crtc to write.
 * \param begin Start of the string to parse.
 * \param end End of the string to parse.
 * \return
 *  - ==0 on success
 *  - !=0 on error
 */
adv_error crtc_parse(adv_crtc* crtc, const char* begin, const char* end)
{
	crtc_reset(crtc);
	crtc_user_reset(crtc);

	parse_separator(" \t", &begin, end);

	if (begin != end && *begin == '"') {
		if (parse_quote(crtc->name, CRTC_NAME_MAX, &begin, end))
			return -1;
	} else {
		if (parse_token(crtc->name, CRTC_NAME_MAX, " \t", &begin, end))
			return -1;
	}

	parse_separator(" \t", &begin, end);
	if (begin == end) {
		snprintf(video_mode_parse_error_buffer, sizeof(video_mode_parse_error_buffer), "Missing modeline data");
		return -1;
	}

	if (parse_crtc(crtc, begin, end) != 0)
		return -1;

	return 0;
}

void crtc_print(char* buffer, unsigned size, const adv_crtc* crtc)
{
	const char* flag1 = crtc_is_nhsync(crtc) ? " -hsync" : " +hsync";
	const char* flag2 = crtc_is_nvsync(crtc) ? " -vsync" : " +vsync";
	const char* flag3 = crtc_is_doublescan(crtc) ? " doublescan" : "";
	const char* flag4 = crtc_is_interlace(crtc) ? " interlace" : "";

	*buffer = 0;

	if (strchr(crtc->name, ' ')!=0)
		sncatf(buffer, size, "\"%s\"", crtc->name);
	else
		sncatf(buffer, size, "%s", crtc->name);

	sncatf(buffer, size, " %g %d %d %d %d %d %d %d %d%s%s%s%s",
		(double)crtc->pixelclock / 1E6,
		crtc->hde, crtc->hrs, crtc->hre, crtc->ht,
		crtc->vde, crtc->vrs, crtc->vre, crtc->vt,
		flag1, flag2, flag3, flag4
	);
}

/* Load the list of video mode */
adv_error crtc_container_load(adv_conf* context, adv_crtc_container* cc)
{
	adv_conf_iterator i;
	int error = 0;

	conf_iterator_begin(&i, context, "device_video_modeline");
	while (!conf_iterator_is_end(&i)) {
		adv_crtc crtc;
		const char* s = conf_iterator_string_get(&i);
		if (crtc_parse(&crtc, s, s + strlen(s)) == 0) {
			crtc_container_insert(cc, &crtc);
		} else {
			if (!error) {
				error = 1;
				error_set("%s in argument '%s' for option 'device_video_modeline'", video_mode_parse_error_buffer, s);
			}
		}
		conf_iterator_next(&i);
	}

	return error ? -1 : 0;
}

/* Save the list of video mode */
void crtc_container_save(adv_conf* context, adv_crtc_container* cc)
{
	adv_crtc_container_iterator i;
	conf_remove(context, "", "device_video_modeline");
	crtc_container_iterator_begin(&i, cc);
	while (!crtc_container_iterator_is_end(&i)) {
		char buffer[1024];
		crtc_print(buffer, sizeof(buffer), crtc_container_iterator_get(&i));
		conf_string_set(context, "", "device_video_modeline", buffer);
		crtc_container_iterator_next(&i);
	}
}

void crtc_container_register(adv_conf* context)
{
	conf_string_register_multi(context, "device_video_modeline");
}

void crtc_container_clear(adv_conf* context)
{
	conf_remove(context, "", "device_video_modeline");
}

/***************************************************************************/
/* Monitor */

static adv_error monitor_range_parse(adv_monitor_range* range, const char* begin, const char* end, double mult)
{
	unsigned i = 0;
	parse_separator(" \t", &begin, end);
	while (begin < end) {
		double v0;
		if (i==MONITOR_RANGE_MAX) {
			return -1;
		}
		if (parse_double(&v0, &begin, end))
			return -1;
		parse_separator(" \t", &begin, end);
		if (*begin=='-') {
			double v1;
			++begin;
			if (parse_double(&v1, &begin, end))
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
		parse_separator(" \t", &begin, end);
		if (*begin==',') {
			++begin;
		}
		parse_separator(" \t", &begin, end);
	}
	if (i==0)
		return -1; /* empty */
	while (i<MONITOR_RANGE_MAX) {
		range[i].low = 0;
		range[i].high = 0;
		++i;
	}
	return 0;
}

static adv_error monitor_single_parse(adv_monitor_range* range, const char* begin, const char* end, double mult)
{
	double v0;
	double v1;

	parse_separator(" \t", &begin, end);

	if (parse_double(&v0, &begin, end))
		return -1;

	parse_separator(" \t", &begin, end);
	if (*begin!='-')
		return -1;
	++begin;
	if (parse_double(&v1, &begin, end))
		return -1;
	if (v0 < 0 || v0 > v1 || v1 > 300) {
		return -1;
	}

	range->low = mult * v0;
	range->high = mult * v1;

	parse_separator(" \t", &begin, end);

	if (begin != end)
		return -1;

	return 0;
}

void monitor_print(char* buffer, unsigned size, const adv_monitor_range* range_begin, const adv_monitor_range* range_end, double mult)
{
	adv_bool empty = 1;

	*buffer = 0;

	while (range_begin != range_end) {
		if (range_begin->low != 0 && range_begin->high != 0) {
			if (!empty) {
				sncatf(buffer, size, ", ");
			}
			if (range_begin->low == range_begin->high) {
				sncatf(buffer, size, "%g", range_begin->low / mult);
			} else {
				sncatf(buffer, size, "%g-%g", range_begin->low / mult, range_begin->high / mult);
			}
			empty = 0;
		}
		++range_begin;
	}

	if (empty) {
		sncatf(buffer, size, "0");
	}
}

adv_error monitor_parse(adv_monitor* monitor, const char* p, const char* h, const char* v)
{
	if (monitor_single_parse(&monitor->pclock, p, p+strlen(p), 1E6)!=0) {
		error_set("Invalid monitor 'pclock' specification");
		return -1;
	}
	if (monitor_range_parse(monitor->hclock, h, h+strlen(h), 1E3)!=0) {
		error_set("Invalid monitor 'hclock' specification");
		return -1;
	}
	if (monitor_range_parse(monitor->vclock, v, v+strlen(v), 1)!=0) {
		error_set("Invalid monitor 'vclock' specification");
		return -1;
	}
	return 0;
}

/**
 * Load the monitor configuration
 * return:
 *   - ==0 on success
 *   - <0 on error
 *   - >0 on configuration missing, video_monitor_is_empty(monitor)!=0
 */
adv_error monitor_load(adv_conf* context, adv_monitor* monitor)
{
	adv_error p_present;
	adv_error h_present;
	adv_error v_present;
	const char* p;
	const char* h;
	const char* v;

	p_present = conf_string_get(context, "device_video_pclock", &p);
	h_present = conf_string_get(context, "device_video_hclock", &h);
	v_present = conf_string_get(context, "device_video_vclock", &v);

	if (p_present!=0 && h_present!=0 && v_present!=0) {
		monitor_reset(monitor);
		error_set("Missing options 'device_video_p/h/vclock'");
		return 1;
	}

	if (p_present!=0) {
		error_set("Missing option 'device_video_pclock'");
		return -1;
	}
	if (monitor_single_parse(&monitor->pclock, p, p+strlen(p), 1E6)!=0) {
		error_set("Invalid argument '%s' for option 'device_video_pclock'", p);
		return -1;
	}

	if (h_present!=0) {
		error_set("Missing option 'device_video_hclock'");
		return -1;
	}
	if (monitor_range_parse(monitor->hclock, h, h+strlen(h), 1000)!=0) {
		error_set("Invalid argument '%s' for option 'device_video_hclock'", h);
		return -1;
	}

	if (v_present!=0) {
		error_set("Missing option 'device_video_vclock'");
		return -1;
	}
	if (monitor_range_parse(monitor->vclock, v, v+strlen(v), 1)!=0) {
		error_set("Invalid argument '%s' for option 'device_video_vclock'", v);
		return -1;
	}

	return 0;
}

void monitor_save(adv_conf* context, const adv_monitor* monitor)
{
	char buffer[1024];

	monitor_print(buffer, sizeof(buffer), &monitor->pclock, &monitor->pclock + 1, 1E6);
	conf_string_set(context, "", "device_video_pclock", buffer);

	monitor_print(buffer, sizeof(buffer), monitor->hclock, monitor->hclock + MONITOR_RANGE_MAX, 1E3);
	conf_string_set(context, "", "device_video_hclock", buffer);

	monitor_print(buffer, sizeof(buffer), monitor->vclock, monitor->vclock + MONITOR_RANGE_MAX, 1);
	conf_string_set(context, "", "device_video_vclock", buffer);
}

void monitor_register(adv_conf* context)
{
	conf_string_register(context, "device_video_pclock");
	conf_string_register(context, "device_video_hclock");
	conf_string_register(context, "device_video_vclock");
}

/***************************************************************************/
/* Generate */

static adv_error parse_generate(const char* begin, const char* end, adv_generate* data)
{
	double d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->hactive = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->hfront = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->hsync = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->hback = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->vactive = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->vfront = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->vsync = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->vback = d;
	parse_separator(" \t", &begin, end);
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

static adv_error parse_generate_interpolate(const char* begin, const char* end, adv_generate_interpolate* interpolate)
{
	double d;
	adv_generate* data = &interpolate->gen;

	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	interpolate->hclock = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->hactive = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->hfront = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->hsync = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->hback = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->vactive = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->vfront = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->vsync = d;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->vback = d;
	parse_separator(" \t", &begin, end);
	if (begin != end)
		return -1;

	generate_normalize(data);

	return 0;
}

static void out_generate_interpolate(char* buffer, unsigned size, const adv_generate_interpolate* interpolate)
{
	adv_generate out = interpolate->gen;

	generate_normalize(&out);

	snprintf(buffer, size, "%d %g %g %g %g %g %g %g %g",
		interpolate->hclock,
		out.hactive, out.hfront, out.hsync, out.hback,
		out.vactive, out.vfront, out.vsync, out.vback);
}

adv_error generate_parse(adv_generate* generate, const char* g)
{
	if (parse_generate(g, g+strlen(g), generate)!=0) {
		error_set("Invalid specification");
		return -1;
	}

	return 0;
}

static int generate_interpolate_cmp(const void* e0, const void* e1)
{
	const adv_generate_interpolate* ee0 = (const adv_generate_interpolate*)e0;
	const adv_generate_interpolate* ee1 = (const adv_generate_interpolate*)e1;
	if (ee0->hclock < ee1->hclock)
		return -1;
	if (ee0->hclock > ee1->hclock)
		return 1;
	return 0;
}

adv_error generate_interpolate_load(adv_conf* context, adv_generate_interpolate_set* interpolate)
{
	adv_conf_iterator i;
	unsigned mac = 0;

	conf_iterator_begin(&i, context, "device_video_format");
	while (!conf_iterator_is_end(&i)) {
		const char* s = conf_iterator_string_get(&i);

		if (parse_generate_interpolate(s, s+strlen(s), &interpolate->map[mac])!=0) {
			error_set("Invalid argument '%s' in option 'device_video_format'", s);
			return -1;
		}

		++mac;

		conf_iterator_next(&i);
	}

	if (!mac) {
		generate_interpolate_reset(interpolate);
		error_set("Missing 'device_video_format' specification");
		return 1;
	}

	/* sort */
	qsort(interpolate->map, mac, sizeof(adv_generate_interpolate), generate_interpolate_cmp);

	interpolate->mac = mac;

	return 0;
}

void generate_interpolate_save(adv_conf* context, const adv_generate_interpolate_set* interpolate)
{
	unsigned i;
	conf_remove(context, "", "device_video_format");
	for(i=0;i<interpolate->mac;++i) {
		char buffer[1024];
		out_generate_interpolate(buffer, sizeof(buffer), &interpolate->map[i]);
		conf_string_set(context, "", "device_video_format", buffer);
	}
}

void generate_interpolate_clear(adv_conf* context)
{
	conf_remove(context, "", "device_video_format");
}

void generate_interpolate_register(adv_conf* context)
{
	conf_string_register_multi(context, "device_video_format");
}

/***************************************************************************/
/* GTF */

static adv_error parse_gtf(const char* begin, const char* end, adv_gtf* data)
{
	double d;
	int i;
	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	data->margin_frac = d;

	parse_separator(" \t", &begin, end);
	if (parse_int(&i, &begin, end))
		return -1;
	if (i < 0)
		return -1;
	data->v_min_frontporch_lines = i;

	parse_separator(" \t", &begin, end);
	if (parse_int(&i, &begin, end))
		return -1;
	if (i < 1)
		return -1;
	data->v_sync_lines = i;

	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	if (d <= 0)
		return -1;
	data->h_sync_frac = d;

	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	if (d / 1E6 <= 0)
		return -1;
	data->v_min_sync_backporch_time = d / 1E6;

	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	if (d < 0)
		return -1;
	data->m = d;

	parse_separator(" \t", &begin, end);
	if (parse_double(&d, &begin, end))
		return -1;
	if (d < 0)
		return -1;
	data->c = d;

	parse_separator(" \t", &begin, end);
	if (begin != end)
		return -1;

	return 0;
}

adv_error gtf_parse(adv_gtf* gtf, const char* g)
{
	if (parse_gtf(g, g+strlen(g), gtf)!=0) {
		error_set("Invalid 'gtf' specification");
		return -1;
	}

	return 0;
}

adv_error gtf_load(adv_conf* context, adv_gtf* gtf)
{
	const char* s;

	if (conf_string_get(context, "device_video_gtf", &s) != 0) {
		error_set("Missing option 'device_video_gtf'");
		return 1;
	}

	if (parse_gtf(s, s+strlen(s), gtf)!=0) {
		error_set("Invalid argument '%s' for option 'device_video_gtf'", s);
		return -1;
	}

	return 0;
}

void gtf_save(adv_conf* context, const adv_gtf* gtf)
{
	char buffer[1024];

	snprintf(buffer, sizeof(buffer), "%g %d %d %g %g %g %g",
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

void gtf_clear(adv_conf* context)
{
	conf_remove(context, "", "device_video_gtf");
}

void gtf_register(adv_conf* context)
{
	conf_string_register(context, "device_video_gtf");
}

