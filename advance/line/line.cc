/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002 Andrea Mazzoleni
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

#include <cmath>
#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <set>

using namespace std;

// **************************************************************************
// clock

#define VGA_DOTCLOCK_HIGH 28322000
#define VGA_DOTCLOCK_LOW 25175000

#define VGA_CLOCK_0 (VGA_DOTCLOCK_LOW/4)
#define VGA_CLOCK_1 (VGA_DOTCLOCK_HIGH/4)
#define VGA_CLOCK_2 (VGA_DOTCLOCK_LOW/2)
#define VGA_CLOCK_3 (VGA_DOTCLOCK_HIGH/2)
#define VGA_CLOCK_4 (VGA_DOTCLOCK_LOW)
#define VGA_CLOCK_5 (VGA_DOTCLOCK_HIGH)

unsigned VGA_GRAPH_CLOCK[] = {
	VGA_CLOCK_0,
	VGA_CLOCK_1,
	VGA_CLOCK_2,
	VGA_CLOCK_3,
	0
};

unsigned VGA_TEXT_CLOCK[] = {
	VGA_CLOCK_2,
	VGA_CLOCK_3,
	VGA_CLOCK_4,
	VGA_CLOCK_5,
	0
};

enum clock_type {
	clock_pixel = 0x1,
	clock_horz = 0x2,
	clock_vert = 0x4
};

// **************************************************************************
// line

class step {
	unsigned st;
public:
	step();
	step(unsigned Ast);
	unsigned operator()(double v) const;
};

step::step()
{
}

step::step(unsigned Ast) : st(Ast)
{
}

unsigned step::operator()(double v) const {
	unsigned il, ih;
	il = (unsigned)floor(v);
	il -= il % st;
	ih = il + st;
	if (fabs(v - il) / fabs(v - ih) < (1.0 + 1e-6) ) // + favorite the lower value
		return il;
	else
		return ih;
}

class line {
	double active;
	double front;
	double sync;
	double back;
	double oactive;
	double ofront;
	double osync;
	double oback;
public:
	line();
	line(double v0, double v1, double v2, double v3);

	double active_get() const { return active; }
	double front_get() const { return front; }
	double sync_get() const { return sync; }
	double back_get() const { return back; }

	double ori_active_get() const { return oactive; }
	double ori_front_get() const { return ofront; }
	double ori_sync_get() const { return osync; }
	double ori_back_get() const { return oback; }

	void normalize();
	void compute_total(const step& sde, const step& sother, unsigned& de, unsigned& rs, unsigned& re, unsigned& tt, unsigned total) const;
	void compute_active(const step& sde, const step& sother, unsigned& de, unsigned& rs, unsigned& re, unsigned& tt, unsigned active) const;
	bool compute_active_total(const step& sde, const step& sother, unsigned& de, unsigned& rs, unsigned& re, unsigned& tt, unsigned active, unsigned total) const;
};

line::line()
{
}

line::line(double v0, double v1, double v2, double v3) :
	oactive(v0), ofront(v1), osync(v2), oback(v3) {
	normalize();
}

void line::normalize()
{
	double total = oactive + ofront + osync + oback;
	active = oactive / total;
	front = ofront / total;
	sync = osync / total;
	back = oback / total;
}

struct line_diff : public line {
	line_diff(double v0, double v1, double v2, double v3);
};

line_diff::line_diff(double v0, double v1, double v2, double v3) :
	line(v0, v3-v2-v0, v1, v2-v1) {
}

void line::compute_total(const step& sde, const step& sother, unsigned& de, unsigned& rs, unsigned& re, unsigned& tt, unsigned total) const {
	de = sde(total * active_get());
	rs = sother(total * (active_get() + front_get()));
	re = sother(total * (active_get() + front_get() + sync_get()));
	tt = sother(total);
}

void line::compute_active(const step& sde, const step& sother, unsigned& de, unsigned& rs, unsigned& re, unsigned& tt, unsigned active) const {
	de = sde(active);
	tt = sother(de / active_get());
	rs = sother(tt * (active_get() + front_get()));
	re = sother(tt * (active_get() + front_get() + sync_get()));
}

bool line::compute_active_total(const step& sde, const step& sother, unsigned& de, unsigned& rs, unsigned& re, unsigned& tt, unsigned active, unsigned total) const {
	de = sde(active);
	tt = sother(total);
	double rde = tt * active_get();
	if (rde < de)
		return false;
	double diff = (rde - de) / 2;
	rs = sother(tt * (active_get() + front_get()) - diff);
	re = sother(tt * (active_get() + front_get() + sync_get()) - diff);
	return true;
}

// **************************************************************************
// generator

class generator {
	line horz;
	line vert;

	double clock_pixel_value;
	double clock_horz_value;
	double clock_vert_value;

	unsigned clock_contrains;

	step hde;
	step h;
	step vde;
	step v;

public:
	generator();

	const step& h_step_get() const { return h; }
	const step& h_active_step_get() const { return hde; }
	const step& v_step_get() const { return v; }
	const step& v_active_step_get() const { return vde; }
	void h_step_set(const step& A) { h = A; }
	void h_active_step_set(const step& A) { hde = A; }
	void v_step_set(const step& A) { v = A; }
	void v_active_step_set(const step& A) { vde = A; }

	void line_horz_set(const line& A);
	void line_vert_set(const line& A);

	const line& line_horz_get() const { return horz; }
	const line& line_vert_get() const { return vert; }

	void clock_pixel_set(double A);
	void clock_horz_set(double A);
	void clock_vert_set(double A);

	double pixel_clock_get() const { return clock_pixel_value; }
	double horz_clock_get() const { return clock_horz_value; }
	double vert_clock_get() const { return clock_vert_value; }

	unsigned clock_contrains_get() const { return clock_contrains; }
	unsigned clock_contrains_count() const;
	void clock_contrains_set(unsigned A) { clock_contrains = A; }
};

generator::generator()
{
	clock_contrains = 0;
}

void generator::line_horz_set(const line& A)
{
	horz = A;
}

void generator::line_vert_set(const line& A)
{
	vert = A;
}

void generator::clock_pixel_set(double clock)
{
	clock_contrains |= clock_pixel;
	clock_pixel_value = clock;
}

void generator::clock_horz_set(double clock)
{
	clock_contrains |= clock_horz;
	clock_horz_value = clock;
}

void generator::clock_vert_set(double clock)
{
	clock_contrains |= clock_vert;
	clock_vert_value = clock;
}

unsigned generator::clock_contrains_count() const {
	unsigned count = 0;

	if (clock_contrains_get() & clock_horz)
		++count;

	if (clock_contrains_get() & clock_vert)
		++count;

	if (clock_contrains_get() & clock_pixel)
		++count;

	return 0;
}

/***************************************************************************/
/* output */

class output {
public:
	enum output_t {
		output_none, output_vga_text, output_vga_graph, output_svga_graph
	};

	output(ostream& Aos);

	void operator()(const string& name, double f, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, unsigned contrains);
	void operator()(const string& name, double f, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, unsigned contrains, bool interlaced);

	output& as(output_t Atype) { type = Atype; return *this; }

	void pixel_clock_min_set(double A) { pixel_clock_min = A; }
	void horz_clock_min_set(double A) { horz_clock_min = A; }
	void vert_clock_min_set(double A) { vert_clock_min = A; }
	void vert_clock_max_set(double A) { vert_clock_max = A; }
	void sync_vga_set(bool A) { sync_vga = A; }
	void c_format_set(bool A) { c_format = A; }

	double pixel_clock_min_get() const { return pixel_clock_min; }
	double horz_clock_min_get() const { return horz_clock_min; }
	double vert_clock_min_get() const { return vert_clock_min; }
	double vert_clock_max_get() const { return vert_clock_max; }
	bool sync_vga_get() const { return sync_vga; }
	bool c_format_get() const   { return c_format; }

	void sync_out(unsigned vde);

private:
	ostream& os;
	int counter;
	output_t type;
	set<string> modes;

	bool insert(const string& s);

	void pre();
	void post(double horz_clock, double vert_clock);
	void svga_graph_out_internal(const string& name, double f, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, bool interlace, bool doublescan);

	void svga_graph_out(const string& name, double f, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, unsigned contrains, bool interlace);
	void vga_text_out(const string& name, double f, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, unsigned contrains);
	void vga_graph_out(const string& name, double f, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, unsigned contrains);

	double pixel_clock_min;
	double horz_clock_min;
	double vert_clock_max;
	double vert_clock_min;
	bool sync_vga;
	bool c_format;
};

output::output(ostream& Aos) : os(Aos), counter(0), type(output_none)
{
	pixel_clock_min = 12E6;
	horz_clock_min = 0;
	vert_clock_max = 112;
	vert_clock_min = 43;
	sync_vga = false;
	c_format = false;
}

void output::operator()(const string& name, double f, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, unsigned contrains)
{
	return operator()(name, f, hde, hrs, hre, ht, vde, vrs, vre, vt, contrains, false);

}

void output::operator()(const string& name, double f, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, unsigned contrains, bool interlaced)
{
	switch (type) {
		case output_vga_text:
			if (interlaced)
				return;
			vga_text_out(name, f, hde, hrs, hre, ht, vde, vrs, vre, vt, contrains);
			break;
		case output_vga_graph:
			if (interlaced)
				return;
			vga_graph_out(name, f, hde, hrs, hre, ht, vde, vrs, vre, vt, contrains);
			break;
		case output_svga_graph:
			svga_graph_out(name, f, hde, hrs, hre, ht, vde, vrs, vre, vt, contrains, interlaced);
			break;
		case output_none:
			break;
	}
}

bool output::insert(const string& s)
{
	pair<set<string>::const_iterator, bool> i = modes.insert(s);
	return i.second;
}

void output::pre()
{
	if (c_format_get()) {
		os << "\"";
	} else {
		os << "device_video_modeline ";
	}
}

void output::post(double horz_clock, double vert_clock)
{
	if (c_format_get()) {
		os << "\", /* H ";
		os.unsetf(ios::floatfield);
		os << horz_clock << " [Hz], V " << vert_clock << " [Hz] */" << endl;
	} else {
		os << " # H ";
		os.unsetf(ios::floatfield);
		os << horz_clock << " [Hz], V " << vert_clock << " [Hz]" << endl;
	}
}

void output::sync_out(unsigned vde)
{
/*
Horizonal Dots         640     640     640
Vertical Scan Lines    350     400     480
Horiz. Sync Polarity   POS     NEG     NEG
Vert. Sync Polarity    NEG     POS     NEG
*/
	if (sync_vga) {
		if (vde > 440)
			os << " -hsync -vsync";
		else if (vde > 350)
			os << " -hsync +vsync";
		else
			os << " +hsync -vsync";
	} else {
		os << " -hsync -vsync";
	}
}

void output::svga_graph_out_internal(const string& name, double f, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, bool interlace, bool doublescan)
{
	ostringstream n;
	n << name << "_" << hde << "x" << vde;
	string ns = n.str();

	if (!insert(ns))
		return;

	pre();
	os << ns;
	os.unsetf(ios::floatfield);
	os << " " << f / 1E6;
	os << " " << hde << " " << hrs << " " << hre << " " << ht;
	os << " " << vde << " " << vrs << " " << vre << " " << vt;
	sync_out(vde * (doublescan + 1 ));
	if (interlace)
		os << " interlace";
	if (doublescan)
		os << " doublescan";
	post(f / ht, f / (ht * vt) * (interlace + 1) / (doublescan + 1));
}

void output::svga_graph_out(const string& name, double f, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, unsigned contrains, bool interlace)
{
	bool doublescan = false;

	// Set the interlace mode if the frame rate is too low
	if (!interlace
		&& (f / (ht * vt)) < vert_clock_min
		&& !(contrains & clock_vert)) {
		interlace = true;
	}

	// Double the vertical size with doublescan
	// if the vertical clock is too high
	if (!interlace && !doublescan
		&& (f / (ht * vt)) > vert_clock_max
		&& !(contrains & clock_vert)) {
		doublescan = true;
	}

	// Double the vertical size with doublescan
	// if the pixel clock is too low
	// or if the horizontal clock is too low
	if (!interlace && !doublescan
		&& (f < pixel_clock_min || (f / ht) < horz_clock_min)
		&& !(contrains & (clock_horz | clock_pixel))) {
		f *= 2;
		doublescan = true;
	}

	svga_graph_out_internal(name, f, hde, hrs, hre, ht, vde, vrs, vre, vt, interlace, doublescan);

	// Double the vertical size if the horz clock is too low
	if (!interlace && doublescan
		&& (f / ht) < horz_clock_min
		&& !(contrains & (clock_horz | clock_pixel))) {
		vde *= 2;
		vrs *= 2;
		vre *= 2;
		vt *= 2;
		f *= 2;
		svga_graph_out_internal(name, f, hde, hrs, hre, ht, vde, vrs, vre, vt, interlace, doublescan);
	}

	// Double the horizontal size if the pixel clock is too low
	if (!interlace
		&& f < pixel_clock_min
		&& !(contrains & clock_pixel)) {
		hde *= 2;
		hrs *= 2;
		hre *= 2;
		ht *= 2;
		f *= 2;
		svga_graph_out_internal(name, f, hde, hrs, hre, ht, vde, vrs, vre, vt, interlace, doublescan);
	}
}

void output::vga_text_out(const string& name, double f, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, unsigned contrains)
{
	bool doublescan = false;

	(void)contrains;

	// Double the vertical size with doublescan if the vertical clock is too high
	if ((f / (ht * vt)) > vert_clock_max) {
		doublescan = true;
	}

	unsigned cols;
	const char* mode;
	if (hde < 640) {
		cols = 40;
		mode = "1";
	} else {
		cols = 80;
		mode = "3";
	}

	ostringstream n;
	n << name << "_vga_text" << cols << "_" << hde/cols << "x" << vde/25;
	string ns = n.str();

	if (!insert(ns))
		return;

	pre();
	os << ns;
	os.unsetf(ios::floatfield);
	os << " " << f / 1E6;
	os << " " << hde << " " << hrs << " " << hre << " " << ht;
	os << " " << vde << " " << vrs << " " << vre << " " << vt;
	sync_out(vde * (doublescan + 1 ));
	if (doublescan)
		os << " doublescan";

	post(f / ht, f / (ht * vt) / (doublescan + 1));
}

void output::vga_graph_out(const string& name, double f, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, unsigned contrains)
{
	bool doublescan = false;

	(void)contrains;

	// Double the vertical size with doublescan if the vertical clock is too high
	if ((f / (ht * vt)) > vert_clock_max) {
		doublescan = true;
	}

	ostringstream n;
	n << name << "_vga_" << hde << "x" << vde;
	string ns = n.str();

	if (!insert(ns))
		return;

	pre();
	os << ns;
	os.unsetf(ios::floatfield);
	os << " " << f / 1E6;
	os << " " << hde << " " << hrs << " " << hre << " " << ht;
	os << " " << vde << " " << vrs << " " << vre << " " << vt;
	sync_out(vde * (doublescan + 1 ));
	if (doublescan)
		os << " doublescan";

	post(f / ht, f / (ht * vt) / (doublescan + 1));
}

/***************************************************************************/
/* compute_default */

void compute_default(output& os, const generator& g, const string& name)
{
	unsigned hde, hrs, hre, ht;
	unsigned vde, vrs, vre, vt;
	double c;

	switch (g.clock_contrains_get()) {
		case clock_pixel | clock_horz : {
			unsigned htotal = g.h_step_get()(g.pixel_clock_get() / g.horz_clock_get() );
			g.line_horz_get().compute_total(g.h_active_step_get(), g.h_step_get(), hde, hrs, hre, ht, htotal);
			unsigned vactive = hde * 3 / 4;
			g.line_vert_get().compute_active(g.v_active_step_get(), g.v_step_get(), vde, vrs, vre, vt, vactive);
			c = g.horz_clock_get() * ht;
			os(name, c, hde, hrs, hre, ht, vde, vrs, vre, vt, g.clock_contrains_get());
		}
		break;
		case clock_pixel | clock_vert : {
		}
		break;
		case clock_horz | clock_vert : {
			unsigned vtotal = g.v_step_get()( g.horz_clock_get() / g.vert_clock_get() );
			g.line_vert_get().compute_total(g.v_active_step_get(), g.v_step_get(), vde, vrs, vre, vt, vtotal);
			unsigned hactive = vde * 4 / 3;
			g.line_horz_get().compute_active(g.h_active_step_get(), g.h_step_get(), hde, hrs, hre, ht, hactive);
			c = g.horz_clock_get() * ht;
			os(name, c, hde, hrs, hre, ht, vde, vrs, vre, vt, g.clock_contrains_get());
			// try a double horz and clock
			os(name, 2*c, 2*hde, 2*hrs, 2*hre, 2*ht, vde, vrs, vre, vt, g.clock_contrains_get());
			// try an interlaced version with double pixel clock and double size
			os(name, 2*c, 2*hde, 2*hrs, 2*hre, 2*ht, 2*vde, 2*vrs, 2*vre, 2*vt+1, g.clock_contrains_get(), true);
		}
		break;
		case clock_pixel | clock_horz | clock_vert : {
			unsigned htotal = g.h_step_get()( g.pixel_clock_get() / g.horz_clock_get() );
			g.line_horz_get().compute_total(g.h_active_step_get(), g.h_step_get(), hde, hrs, hre, ht, htotal);
			unsigned vtotal = g.v_step_get()( g.horz_clock_get() / g.vert_clock_get() );
			g.line_vert_get().compute_total(g.v_active_step_get(), g.v_step_get(), vde, vrs, vre, vt, vtotal);
			c = g.horz_clock_get() * ht;
			os(name, c, hde, hrs, hre, ht, vde, vrs, vre, vt, g.clock_contrains_get());
		}
		break;
	}
}

void compute_default_vga_graph(output& os, const generator& g, const string& name)
{
	if ((g.clock_contrains_get() & clock_pixel) == 0) {
		generator g2 = g;
#ifdef USE_FINE_MODE
		g2.h_active_step_set( step( 8 ) );
#else
		g2.h_active_step_set( step( 16 ) );
#endif
		g2.h_step_set( step( 2 ) );
#ifdef USE_FINE_MODE
		g2.v_active_step_set( step( 2 ) );
#else
		g2.v_active_step_set( step( 8 ) );
#endif
		g2.v_step_set( step( 1 ) );

		for(int i=0;VGA_GRAPH_CLOCK[i];++i) {
			g2.clock_pixel_set(VGA_GRAPH_CLOCK[i]);
			compute_default(os.as(output::output_vga_graph), g2, name);
		}
	}
}

void compute_default_svga_graph(output& os, const generator& g, const string& name)
{
	generator g2 = g;
#ifdef USE_FINE_MODE
	g2.h_active_step_set( step( 8 ) );
#else
	g2.h_active_step_set( step( 16 ) );
#endif
	g2.h_step_set( step( 8 ) );
#ifdef USE_FINE_MODE
	g2.v_active_step_set( step( 2 ) );
#else
	g2.v_active_step_set( step( 8 ) );
#endif
	g2.v_step_set( step( 1 ) );

	compute_default(os.as(output::output_svga_graph), g2, name);
}

/***************************************************************************/
/* compute */

bool compute_active(output& os, const generator& g, const string& name, unsigned hactive, unsigned vactive)
{
	unsigned hde, hrs, hre, ht;
	unsigned vde, vrs, vre, vt;
	double c;

	switch (g.clock_contrains_get()) {
		case clock_pixel : {
			g.line_horz_get().compute_active(g.h_active_step_get(), g.h_step_get(), hde, hrs, hre, ht, hactive);
			g.line_vert_get().compute_active(g.v_active_step_get(), g.v_step_get(), vde, vrs, vre, vt, vactive);
			c = g.pixel_clock_get();
			os(name, c, hde, hrs, hre, ht, vde, vrs, vre, vt, g.clock_contrains_get());
			return true;
		}
		break;
		case clock_horz : {
			g.line_horz_get().compute_active(g.h_active_step_get(), g.h_step_get(), hde, hrs, hre, ht, hactive);
			g.line_vert_get().compute_active(g.v_active_step_get(), g.v_step_get(), vde, vrs, vre, vt, vactive);
			c = g.horz_clock_get() * ht;
			os(name, c, hde, hrs, hre, ht, vde, vrs, vre, vt, g.clock_contrains_get());
			return true;
		}
		break;
		case clock_vert : {
			g.line_horz_get().compute_active(g.h_active_step_get(), g.h_step_get(), hde, hrs, hre, ht, hactive);
			g.line_vert_get().compute_active(g.v_active_step_get(), g.v_step_get(), vde, vrs, vre, vt, vactive);
			c = g.vert_clock_get() * ht * vt;
			os(name, c, hde, hrs, hre, ht, vde, vrs, vre, vt, g.clock_contrains_get());
			return true;
		}
		break;
		case clock_pixel | clock_horz : {
			unsigned htotal = g.h_step_get()( g.pixel_clock_get() / g.horz_clock_get() );
			if (g.line_horz_get().compute_active_total(g.h_active_step_get(), g.h_step_get(), hde, hrs, hre, ht, hactive, htotal)) {
				g.line_vert_get().compute_active(g.v_active_step_get(), g.v_step_get(), vde, vrs, vre, vt, vactive);
				c = g.horz_clock_get() * ht;
				os(name, c, hde, hrs, hre, ht, vde, vrs, vre, vt, g.clock_contrains_get());
				return true;
			}
		}
		case clock_pixel | clock_vert : {
		}
		break;
		case clock_horz | clock_vert : {
			unsigned vtotal = g.v_step_get()( g.horz_clock_get() / g.vert_clock_get() );
			if (g.line_vert_get().compute_active_total(g.v_active_step_get(), g.v_step_get(), vde, vrs, vre, vt, vactive, vtotal)) {
				g.line_horz_get().compute_active(g.h_active_step_get(), g.h_step_get(), hde, hrs, hre, ht, hactive);
				c = g.horz_clock_get() * ht;
				os(name, c, hde, hrs, hre, ht, vde, vrs, vre, vt, g.clock_contrains_get());
				return true;
			} else {
				// try with an interlaced version
				vtotal = vtotal * 2 + 1;
				if (g.line_vert_get().compute_active_total(g.v_active_step_get(), g.v_step_get(), vde, vrs, vre, vt, vactive, vtotal)) {
					g.line_horz_get().compute_active(g.h_active_step_get(), g.h_step_get(), hde, hrs, hre, ht, hactive);
					c = g.horz_clock_get() * ht;
					os(name, c, hde, hrs, hre, ht, vde, vrs, vre, vt, g.clock_contrains_get(), true);
					return true;
				}
			}
		}
		break;
		case clock_pixel | clock_horz | clock_vert : {
			unsigned htotal = g.h_step_get()( g.pixel_clock_get() / g.horz_clock_get() );
			if (g.line_horz_get().compute_active_total(g.h_active_step_get(), g.h_step_get(), hde, hrs, hre, ht, hactive, htotal)) {
				unsigned vtotal = g.v_step_get()( g.horz_clock_get() / g.vert_clock_get() );
				if (g.line_vert_get().compute_active_total(g.v_active_step_get(), g.v_step_get(), vde, vrs, vre, vt, vactive, vtotal)) {
					c = g.horz_clock_get() * ht;
					os(name, c, hde, hrs, hre, ht, vde, vrs, vre, vt, g.clock_contrains_get());
					return true;
				}
			}
		}
	}
	return false;
}

void compute_svga_graph(output& os, const generator& g, const string& name)
{
	struct {
		unsigned x, y;
	} res[] = {
		{ 240, 180 },
		{ 256, 192 },
		{ 272, 204 },
		{ 288, 216 },
		{ 384, 224 }, // CPS2 games
		{ 304, 228 },
		{ 320, 240 },
		{ 336, 252 },
		{ 344, 258 }, // y=258 for 256 rows games
		{ 352, 264 },
		{ 368, 276 },
		{ 384, 288 },
		{ 400, 300 },
		{ 416, 312 },
		{ 432, 324 },
		{ 448, 336 },
		{ 464, 348 },
		{ 480, 360 },
		{ 496, 372 },
		{ 512, 384 },
		{ 544, 408 },
		{ 576, 432 },
		{ 608, 456 },
		{ 640, 480 },
		{ 672, 504 },
		{ 688, 516 }, // y=516 for 512 rows games
		{ 704, 528 },
		{ 736, 552 },
		{ 768, 576 },
		{ 800, 600 },
		{ 1024, 768 },
		{ 1280, 1024 },
		{ 0, 0 }
	};

	generator g2 = g;
	g2.h_active_step_set( step( 8 ) );
	g2.h_step_set( step( 8 ) );
	g2.v_active_step_set( step( 2 ) );
	g2.v_step_set( step( 1 ) );

	// only if a single contrain
	if (g.clock_contrains_get() == clock_horz
		|| g.clock_contrains_get() == clock_vert
		|| g.clock_contrains_get() == clock_pixel) {
		for(unsigned i=0;res[i].x;++i) {
			compute_active(os.as(output::output_svga_graph), g2, name, res[i].x, res[i].y);
		}
	} else {
		compute_active(os.as(output::output_svga_graph), g2, name, 320, 240);
		compute_active(os.as(output::output_svga_graph), g2, name, 400, 300);
		compute_active(os.as(output::output_svga_graph), g2, name, 512, 384);
		compute_active(os.as(output::output_svga_graph), g2, name, 640, 480);
		compute_active(os.as(output::output_svga_graph), g2, name, 800, 600);
	}

	compute_active(os.as(output::output_svga_graph), g2, name, 256, 224);
	compute_active(os.as(output::output_svga_graph), g2, name, 256, 240);
	compute_active(os.as(output::output_svga_graph), g2, name, 320, 224);
	compute_active(os.as(output::output_svga_graph), g2, name, 320, 240);
	compute_active(os.as(output::output_svga_graph), g2, name, 512, 448);
	compute_active(os.as(output::output_svga_graph), g2, name, 512, 480);
	compute_active(os.as(output::output_svga_graph), g2, name, 640, 448);
	compute_active(os.as(output::output_svga_graph), g2, name, 640, 480);
	compute_active(os.as(output::output_svga_graph), g2, name, 640, 512);
	compute_active(os.as(output::output_svga_graph), g2, name, 720, 448);
	compute_active(os.as(output::output_svga_graph), g2, name, 720, 480);
	compute_active(os.as(output::output_svga_graph), g2, name, 720, 512);
	compute_active(os.as(output::output_svga_graph), g2, name, 720, 576);
}

void compute_svga_graph_fix(output& os, const generator& g, const string& name, int x, int y)
{
	generator g2 = g;
	g2.h_active_step_set( step( 8 ) );
	g2.h_step_set( step( 8 ) );
	g2.v_active_step_set( step( 2 ) );
	g2.v_step_set( step( 1 ) );

	// only if a single contrain
	if (g.clock_contrains_get() == clock_horz
		|| g.clock_contrains_get() == clock_vert
		|| g.clock_contrains_get() == clock_pixel) {
		compute_active(os.as(output::output_svga_graph), g2, name, x, y);
	}
}

void compute_active_vga_graph(output& os, const generator& g, const string& name, unsigned hactive, unsigned vactive)
{
	generator g2 = g;
	g2.h_active_step_set( step( 8 ) );
	g2.h_step_set( step( 2 ) );
	g2.v_active_step_set( step( 2 ) );
	g2.v_step_set( step( 1 ) );

	for(int i=0;VGA_GRAPH_CLOCK[i];++i) {
		g2.clock_pixel_set(VGA_GRAPH_CLOCK[i]);
		if (compute_active(os.as(output::output_vga_graph), g2, name, hactive, vactive))
			break;
	}
}

void compute_active_vga_text(output& os, const generator& g, const string& name, unsigned hactive, unsigned vactive)
{
	generator g2 = g;

	g2.h_active_step_set( step( 8 ) );
	g2.h_step_set( step( 2 ) );
	g2.v_active_step_set( step( 2 ) );
	g2.v_step_set( step( 1 ) );

	for(int i=0;VGA_TEXT_CLOCK[i];++i) {
		g2.clock_pixel_set(VGA_TEXT_CLOCK[i]);
		if (compute_active(os.as(output::output_vga_text), g2, name, hactive, vactive))
			break;
	}
}

void compute_vga_graph(output& os, generator& g, const string& name)
{
	if ((g.clock_contrains_get() & clock_pixel) == 0) {
		compute_active_vga_graph(os, g, name, 320, 200);
		compute_active_vga_graph(os, g, name, 360, 200);
		compute_active_vga_graph(os, g, name, 320, 240);
		compute_active_vga_graph(os, g, name, 360, 240);
		compute_active_vga_graph(os, g, name, 320, 400);
		compute_active_vga_graph(os, g, name, 360, 400);
		compute_active_vga_graph(os, g, name, 320, 480);
		compute_active_vga_graph(os, g, name, 360, 480);
	}
}

void compute_vga_text(output& os, generator& g, const string& name)
{
	if ((g.clock_contrains_get() & clock_pixel) == 0) {
		compute_active_vga_text(os, g, name, 320, 200);
		compute_active_vga_text(os, g, name, 360, 200);
		compute_active_vga_text(os, g, name, 640, 200);
		compute_active_vga_text(os, g, name, 720, 200);
		compute_active_vga_text(os, g, name, 320, 400);
		compute_active_vga_text(os, g, name, 360, 400);
		compute_active_vga_text(os, g, name, 640, 400);
		compute_active_vga_text(os, g, name, 720, 400);
	}
}

void help()
{
	cout << "Usage:" << endl;
	cout << "\tmodeline [options] [XxY]" << endl;
	cout << "Format:" << endl;
	cout << "\t/fh A,F,S,B   Select the horizontal format" << endl;
	cout << "\t/fv A,F,S,B   Select the vertical format" << endl;
	cout << "\t/sync_vga     Generate the sync polarization for a VGA monitor" << endl;
	cout << "Clock contrains:" << endl;
	cout << "\t/p CLOCK      Select the pixel clock [Hz]" << endl;
	cout << "\t/h CLOCK      Select the horizontal clock [Hz]" << endl;
	cout << "\t/v CLOCK      Select the vertical clock [Hz]" << endl;
	cout << "\t/pmin CLOCK   Select the minimum pixel clock [Hz]" << endl;
	cout << "\t/hmin CLOCK   Select the minimum horizontal clock [Hz]" << endl;
	cout << "\t/vmin CLOCK   Select the minimum vertical clock [Hz]" << endl;
	cout << "\t/vmax CLOCK   Select the maximum vertical clock [Hz]" << endl;
	cout << "Predefinites:" << endl;
	cout << "\t/atari_standard  Atari standard resolution monitor" << endl;
	cout << "\t/atari_extended  Atari extended resolution monitor" << endl;
	cout << "\t/atari_medium    Atari medium resolution monitor" << endl;
	cout << "\t/atari_vga       Atari VGA monitor" << endl;
	cout << "\t/pal             PAL TV" << endl;
	cout << "\t/ntsc            NTSC TV" << endl;
	cout << "\t/hp_vga          HP VGA monitor" << endl;
	cout << "\t/vga             Generic VGA monitor" << endl;
	cout << "\t/svga60          Generic SVGA multisync 60 Hz monitor" << endl;
	cout << "\t/svga57          Generic SVGA multisync 57 Hz monitor" << endl;
}

int optionmatch(const char* arg, const char* opt)
{
	return (arg[0] == '-' || arg[0] == '/') && strcasecmp(arg+1, opt) == 0;
}

int main(int argc, char* argv[])
{
	output out(cout);
	generator g;
	string name = "test";
	bool fixed_size = false;
	bool show_comment = false;
	bool show_header = false;
	bool show_svga = false;
	bool show_vga = false;

	if (argc <= 1) {
		help();
		exit(EXIT_FAILURE);
	}

	int optarg;
	for(optarg=1;optarg<argc;++optarg) {
		bool has_arg = optarg+1<argc;
		bool used_arg = false;
		const char* opt = argv[optarg];
		string arg;
		if (has_arg)
			arg = argv[optarg+1];
		if (optionmatch(opt, "atari_standard")) {
			/* Randy fromm */
			g.line_horz_set( line_diff( 46.9, 4.7, 11.9, 63.6 ) );
			g.line_vert_set( line_diff( 15.3, 0.2, 1.2, 16.7 ) );
			g.clock_horz_set( 15720 );
			name = "standard";
		} else if (optionmatch(opt, "atari_extended")) {
			/* Randy fromm */
			g.line_horz_set( line_diff( 48, 3.9, 11.9, 60.6 ) );
			g.line_vert_set( line_diff( 17.4, 0.2, 1.2, 18.9 ) );
			g.clock_horz_set( 16500 );
			name = "extended";
		} else if (optionmatch(opt, "atari_medium")) {
			/* Randy fromm */
			g.line_horz_set( line_diff( 32, 4, 7.2, 40 ) );
			g.line_vert_set( line_diff( 15.4, 0.2, 1.2, 16.7 ) );
			g.clock_horz_set( 25000 );
			name = "medium";
		} else if (optionmatch(opt, "vga")) {
			/* Industry standard VGA */
			g.line_horz_set( line( 640, 16, 96, 48 ) );
			g.line_vert_set( line( 480, 10, 2, 33 ) );
			g.clock_horz_set( 31469 );
			name = "pc_31.5";
			out.sync_vga_set(true);
		} else if (optionmatch(opt, "atari_vga")) {
			/* Atari VGA */
			g.line_horz_set( line_diff( 25.6, 4, 5.7, 31.7 ) );
			g.line_vert_set( line_diff( 12.2, 0.2, 1.1, 14.3 ) );
			g.clock_horz_set( 31550 );
			name = "atari_vga";
			out.sync_vga_set(true);
		} else if (optionmatch(opt, "hp_vga")) {
			/* HP VGA */
			g.line_horz_set( line( 25.17, 0.94, 3.77, 1.89 ) );
			g.line_vert_set( line( 15.25, 0.35, 0.06, 1.02 ) );
			g.clock_horz_set( 31476 );
			name = "hp_vga";
			out.sync_vga_set(true);
		} else if (optionmatch(opt, "pal")) {
			/* Various INET source */
			g.line_horz_set( line( 52.00, 1.65, 4.70, 5.65 ) );
			g.line_vert_set( line( 288.5, 3, 3, 18 ) );
			g.clock_horz_set( 15625 );
			g.clock_vert_set( 50 );
			name = "pal";
		} else if (optionmatch(opt, "ntsc")) {
			/* Various INET source */
			g.line_horz_set( line( 52.60, 1.50, 4.70, 4.70 ) );
			g.line_vert_set( line( 242.5, 3, 3, 14 ) );
			g.clock_horz_set( 15734 );
			g.clock_vert_set( 59.94 );
			name = "ntsc";
		} else if (optionmatch(opt, "svga60")) {
			/* Like vga */
			g.line_horz_set( line( 640, 16, 96, 48 ) );
			g.line_vert_set( line( 480, 10, 2, 33 ) );
			g.clock_vert_set( 60 );
			name = "pc_mult60";
			out.horz_clock_min_set( 30E3 );
		} else if (optionmatch(opt, "svga57")) {
			/* Like vga */
			g.line_horz_set( line( 640, 16, 96, 48 ) );
			g.line_vert_set( line( 480, 10, 2, 33 ) );
			g.clock_vert_set( 57 );
			name = "pc_mult57";
			out.horz_clock_min_set( 30E3 );
		} else if (optionmatch(opt, "fh") && has_arg) {
			istringstream is(arg);
			double A, F, S, B;
			char s1, s2, s3;
			is >> A >> s1 >> F >> s2 >> S >> s3 >> B;
			if (s1!=',' || s2!=',' || s3!=',') {
				cerr << "Invalid separator " << endl;
				exit(EXIT_FAILURE);
			}
			g.line_horz_set( line( A, F, S, B ) );
			used_arg = true;
		} else if (optionmatch(opt, "fv") && has_arg) {
			istringstream is(arg);
			double A, F, S, B;
			char s1, s2, s3;
			is >> A >> s1 >> F >> s2 >> S >> s3 >> B;
			if (s1!=',' || s2!=',' || s3!=',') {
				cerr << "Invalid separator " << endl;
				exit(EXIT_FAILURE);
			}
			g.line_vert_set( line( A, F, S, B ) );
			used_arg = true;
		} else if (optionmatch(opt, "c")) {
			g.clock_contrains_set(0);
		} else if (optionmatch(opt, "p") && has_arg) {
			g.clock_pixel_set( atof(arg.c_str()) * 1E6 );
			used_arg = true;
		} else if (optionmatch(opt, "h") && has_arg) {
			g.clock_horz_set( atof(arg.c_str()) * 1E3 );
			used_arg = true;
		} else if (optionmatch(opt, "v") && has_arg) {
			g.clock_vert_set( atof(arg.c_str()) );
			used_arg = true;
		} else if (optionmatch(opt, "pmin") && has_arg) {
			out.pixel_clock_min_set( atof(arg.c_str()) * 1E6 );
			used_arg = true;
		} else if (optionmatch(opt, "hmin") && has_arg) {
			out.horz_clock_min_set( atof(arg.c_str()) * 1E3 );
			used_arg = true;
		} else if (optionmatch(opt, "vmin") && has_arg) {
			out.vert_clock_min_set( atof(arg.c_str()) );
			used_arg = true;
		} else if (optionmatch(opt, "vmax") && has_arg) {
			out.vert_clock_max_set( atof(arg.c_str()) );
			used_arg = true;
		} else if (optionmatch(opt, "sync_vga")) {
			out.sync_vga_set(true);
		} else if (optionmatch(opt, "show_comment")) {
			show_comment = true;
		} else if (optionmatch(opt, "show_header")) {
			show_header = true;
		} else if (optionmatch(opt, "show_vga")) {
			show_vga = true;
		} else if (optionmatch(opt, "show_svga")) {
			show_svga = true;
		} else if (optionmatch(opt, "c_format")) {
			out.c_format_set(true);
		} else if (isdigit(opt[0])) {
			fixed_size = true;
			break;
		} else {
			cerr << "Unknown option " << opt << endl;
			exit(EXIT_FAILURE);
		}
		if (used_arg)
			++optarg;
	}

	if (!show_comment && !show_vga && !show_svga && !show_header) {
		show_comment = true;
		show_header = true;
		show_vga = true;
		show_svga = true;
	}

	if (!g.clock_contrains_get()) {
		cerr << "No clock contrains" << endl;
		exit(EXIT_FAILURE);
	}

	if (fixed_size) {
		for(;optarg<argc;++optarg) {
			int fix_x;
			int fix_y;
			char s;
			istringstream is(argv[optarg]);
			is >> fix_x >> s >> fix_y;
			if (s!='x') {
				cerr << "Invalid separator " << s << endl;
				exit(EXIT_FAILURE);
			}
			compute_svga_graph_fix(out, g, name, fix_x, fix_y);
		}
	} else {
		if (show_header) {
			string pre;
			if (out.c_format_get()) {
				cout << "/**************************************************************************/" << endl;
				cout << "/* " << name << " */" << endl;
				cout << endl;
				cout << "/*" << endl;
			} else {
				pre = "# ";
				cout << pre << name << endl;
			}
			cout << pre << "format H " << g.line_horz_get().active_get() << " " << g.line_horz_get().front_get() << " " << g.line_horz_get().sync_get() << " " << g.line_horz_get().back_get() << endl;
			cout << pre << "format V " << g.line_vert_get().active_get() << " " << g.line_vert_get().front_get() << " " << g.line_vert_get().sync_get() << " " << g.line_vert_get().back_get() << endl;
			if ((g.clock_contrains_get() & clock_pixel) != 0)
				cout << pre << "fixed pixel clock " << g.pixel_clock_get() << endl;
                        if ((g.clock_contrains_get() & clock_horz) != 0)
				cout << pre << "fixed horz clock " << g.horz_clock_get() << endl;
			if ((g.clock_contrains_get() & clock_vert) != 0)
				cout << pre << "fixed vert clock " << g.vert_clock_get() << endl;
			if (out.c_format_get())
				cout << "*/" << endl;
			cout << endl;
		}

		if (show_vga) {
			if (show_comment) {
				if (out.c_format_get())
					cout << "/* VGA text modes */" << endl;
				else
					cout << "# VGA text modes" << endl;
			}
			compute_vga_text(out, g, name);
			if (show_comment)
				cout << endl;

#if 0 /* VGA graphics mode no more supported */
			if (show_comment) {
				if (out.c_format_get())
					cout << "/* VGA best fit modes */" << endl;
				else
					cout << "# VGA best fit modes" << endl;
			}
			compute_default_vga_graph(out, g, name);
			if (show_comment)
				cout << endl;

			if (show_comment) {
				if (out.c_format_get())
					cout << "/* VGA standard size modes */" << endl;
				else
					cout << "# VGA standard size modes" << endl;
			}
			compute_vga_graph(out, g, name);
			if (show_comment)
				cout << endl;
#endif
		}

		if (show_svga) {
			switch (g.clock_contrains_get()) {
			case clock_pixel | clock_horz :
			case clock_horz | clock_vert :
			case clock_pixel | clock_horz | clock_vert :
				if (show_comment) {
					if (out.c_format_get())
						cout << "/* SVGA best fit modes */" << endl;
					else
						cout << "# SVGA best fit modes" << endl;
				}
				compute_default_svga_graph(out, g, name);
				if (show_comment)
					cout << endl;
				break;
			}

			if (show_comment) {
				if (out.c_format_get())
					cout << "/* SVGA standard size modes */" << endl;
				else
					cout << "# SVGA standard size modes" << endl;
			}
			compute_svga_graph(out, g, name);
			if (show_comment)
				cout << endl;
		}
	}

	return 0;
}

