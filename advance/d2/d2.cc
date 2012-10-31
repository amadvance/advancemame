/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003, 2004, 2005 Andrea Mazzoleni
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

#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>

using namespace std;

//---------------------------------------------------------------------------
// string

string fill(unsigned l, char c)
{
	string r;
	for(unsigned i=0;i<l;++i)
		r += c;
	return r;
}

string token(int& p, const string& s)
{
	int begin;
	int end;

	begin = p;

	while (p < s.length() && s[p] != '\n')
		++p;

	end = p;

	if (p < s.length())
		++p;

	return s.substr(begin, end - begin);
}

string trim_left(const string& s)
{
	string r = s;
	while (r.length() > 0 && isspace(r[0]))
		r.erase(0, 1);
	return r;
}

string trim_right(const string& s)
{
	string r = s;
	while (r.length() > 0 && isspace(r[r.length()-1]))
		r.erase(r.length()-1, 1);
	return r;
}

string trim(const string& s)
{
	return trim_left(trim_right(s));
}

string up(const string& s)
{
	string r;
	for(unsigned i=0;i<s.length();++i)
		r += toupper(s[i]);
	return r;
}

//---------------------------------------------------------------------------
// convert

enum state_t {
	state_separator,
	state_filled,
	state_section0,
	state_section1,
	state_section2,
	state_option,
	state_dot0,
	state_dot1,
	state_tag0,
	state_tag1,
	state_tag_separator,
	state_pre0,
	state_pre1,
	state_para0,
	state_para1
};

class convert {
	void step(const string& s);
	void next(unsigned level, unsigned& Alevel0, unsigned& Alevel1, unsigned& Alevel2);
	void index_all(const string& file, unsigned index_level);
	unsigned index_level(const string& s);
	bool is_option(const string& s, string& a);
	bool is_tag(const string& s, string& a, string& b, bool root_tag);
	bool is_dot(const string& s, string& a);
	bool is_pre(const string& s, string& a);
	bool is_line(const string& s, string& a);
	bool request_separator;

protected:
	istream& is;
	ostream& os;
	state_t state;
	unsigned level0;
	unsigned level1;
	unsigned level2;
	bool numbered;
public:
	convert(istream& Ais, ostream& Aos);
	virtual ~convert();

	void run();

	virtual void header(const string& a, const string& b) = 0;
	virtual void footer() = 0;

	virtual void sep() = 0;
	virtual void line() = 0;

	virtual void index_begin();
	virtual void index_end();
	virtual void index_text(unsigned level, unsigned index0, unsigned index1, unsigned index2, const string& s);

	virtual void section_begin(unsigned level) = 0;
	virtual void section_end() = 0;
	virtual void section_text(const string& s) = 0;

	virtual void para_begin(unsigned level) = 0;
	virtual void para_end() = 0;
	virtual void para_text(const string& s) = 0;

	virtual void pre_begin(unsigned level) = 0;
	virtual void pre_end() = 0;
	virtual void pre_text(const string& s) = 0;

	virtual void dot_begin(unsigned level) = 0;
	virtual void dot_end() = 0;
	virtual void dot_start(const string& s) = 0;
	virtual void dot_stop() = 0;
	virtual void dot_text(const string& s) = 0;

	virtual void option_begin() = 0;
	virtual void option_end() = 0;
	virtual void option_start(const string& s) = 0;
	virtual void option_stop() = 0;
	virtual void option_text(const string& s) = 0;

	virtual void tag_begin(unsigned level) = 0;
	virtual void tag_end() = 0;
	virtual void tag_start(const string& a, const string& b) = 0;
	virtual void tag_stop() = 0;
	virtual void tag_text(const string& s) = 0;
};

convert::convert(istream& Ais, ostream& Aos) : is(Ais), os(Aos)
{
	numbered = true;
};

convert::~convert()
{
}

bool convert::is_pre(const string& s, string& a)
{
	if (s.length() > 0 && s[0] == ':') {
		a = s.substr(1);
		return true;
	}
	return false;
}

bool convert::is_line(const string& s, string& a)
{
	if (s.length() > 0 && s[0] == '+') {
		a = s.substr(1);
		return true;
	}
	return false;
}

bool convert::is_dot(const string& s, string& a)
{
	if (s.length() > 0 && (s[0] == '*' || s[0]==')') && s[1] == ' ') {
		a = s.substr(2);
		return true;
	}
	return false;
}

bool convert::is_option(const string& s, string& a)
{
	string r = trim(s);

	if (r.length() > 0 && (r[0] == '-' || r[0] =='/')) {
		a = s;
		return true;
	}

	if (r.length() > 0 && (r[0]=='=')) {
		a = s.substr(1);
		return true;
	}

	return false;
}

bool convert::is_tag(const string& s, string& a, string& b, bool root_tag)
{
	if (s.length() > 0 && (s[0] == ':' || s[0] == '+'))
		return false;
	unsigned d = s.find(" - ");
	if (d == string::npos)
		return false;
	a = trim(s.substr(0, d));
	b = trim(s.substr(d+3));
	if (root_tag && a.find(' ') != string::npos)
		return false;
	return true;
}

void convert::next(unsigned level, unsigned& Alevel0, unsigned& Alevel1, unsigned& Alevel2)
{
	if (level == 0) {
		++Alevel0;
		Alevel1 = 0;
		Alevel2 = 0;
	} else if (level == 1) {
		++Alevel1;
		Alevel2 = 0;
	} else {
		++Alevel2;
	}
}

void convert::index_begin()
{
}

void convert::index_end()
{
}

void convert::index_text(unsigned level, unsigned index0, unsigned index1, unsigned index2, const string& s)
{
}

void convert::step(const string& r)
{
	string s = trim_right(r);

	// count left space
	unsigned nt = 0;
	while (nt < s.length() && s[nt] == '\t')
		++nt;
	s.erase(0, nt);
	unsigned ns = 0;
	while (ns < s.length() && s[ns] == ' ')
		++ns;
	s.erase(0, ns);
	ns += nt*8;

	// continue with separator support
	if (s.length() == 0 && (state == state_tag0 || state == state_tag1)) {
		request_separator = true;
		return;
	}
	if ((ns == 16 && state == state_tag0) || (ns == 24 && state == state_tag1)) {
		if (request_separator) {
			request_separator = false;
			sep();
		}
		string a;
		if (is_line(s, a)) {
			tag_text(a);
			line();
		} else {
			tag_text(s);
		}
		return;
	}
	if (request_separator) {
		tag_stop();
		tag_end();
		request_separator = false;
		state = state_separator;
	}

	// continue without separator support
	if (s.length() > 0) {
		if (ns == 16 && state == state_option) {
			option_text(s);
			return;
		}

		if ((ns == 16 && state == state_dot0) || (ns == 24 && state == state_dot1)) {
			dot_text(s);
			return;
		}

		if ((ns == 0 && state == state_section0) || (ns == 2 && state == state_section1) || (ns == 4 && state == state_section2)) {
			section_text(s);
			return;
		}

		if ((ns == 8 && state == state_para0) || (ns == 16 && state == state_para1)) {
			string a;
			if (is_line(s, a)) {
				para_text(a);
				line();
			} else {
				para_text(s);
			}
			return;
		}
	}

	// end
	if (state == state_section0 || state == state_section1 || state == state_section2) {
		section_end();
	}
	if (state == state_para0 || state == state_para1) {
		para_end();
	}

	// start
	if (ns == 8 || ns == 16) {
		string a, b;
		if (is_tag(s, a, b, ns == 8)) {
			state_t state_new = ns == 8 ? state_tag0 : state_tag1;
			if (state != state_new) {
				tag_begin(ns == 16);
			} else {
				tag_stop();
			}
			state = state_new;
			string c;
			if (is_line(b, c)) {
				tag_start(a, c);
				line();
			} else {
				tag_start(a, b);
			}
			return;
		}
	}
	if (state == state_tag0 || state == state_tag1) {
		tag_stop();
		tag_end();
	}

	if (ns == 8) {
		string a;
		if (is_option(s, a) && state != state_para0) {
			if (state != state_option) {
				option_begin();
			} else {
				option_stop();
			}
			state = state_option;
			option_start(a);
			return;
		}
	}
	if (state == state_option) {
		option_stop();
		option_end();
	}

	if (ns == 8 || ns == 16) {
		string a;
		if (is_dot(s, a)) {
			state_t state_new = ns == 8 ? state_dot0 : state_dot1;
			if (state != state_new) {
				dot_begin(ns == 16);
			} else {
				dot_stop();
			}
			state = state_new;
			dot_start(a);
			return;
		}
	}
	if (state == state_dot0 || state == state_dot1) {
		dot_stop();
		dot_end();
	}

	if (ns == 8 || ns == 16) {
		string a;
		if (is_pre(s, a)) {
			state_t state_new = ns == 8 ? state_pre0 : state_pre1;
			if (state != state_new)
				pre_begin(ns == 16);
			state = state_new;
			pre_text(a);
			return;
		}
	}
	if (state == state_pre0 || state == state_pre1) {
		pre_end();
	}

	if (s.length()>0 && (ns == 0 || ns == 2 || ns == 4)) {
		state_t state_new = ns == 0 ? state_section0 : (ns == 2 ? state_section1 : state_section2);
		if (state != state_new) {
			next(ns / 2, level0, level1, level2);
			section_begin(ns / 2);
		}
		state = state_new;
		section_text(s);
		return;
	}

	if (s.length()>0 && (ns == 8 || ns == 16)) {
		state_t state_new = ns == 8 ? state_para0 : state_para1;
		if (state != state_new)
			para_begin(ns == 16);
		state = state_new;
		string a;
		if (is_line(s, a)) {
			para_text(a);
			line();
		} else {
			para_text(s);
		}
		return;
	}

	if (s.length() == 0) {
		state = state_separator;
		return;
	}

	cerr << "warning: unrecognized (" << ns << ") `" << s << "'" << endl;

	state = state_filled;
	return;
}

unsigned convert::index_level(const string& s)
{
	if (isalnum(s[0])) {
		return 0;
	} else if (s[0] == ' ' && s[1] == ' ' && isalnum(s[2])) {
		return 1;
	} else if (s[0] == ' ' && s[1] == ' ' && s[2] == ' ' && s[3] == ' ' && isalnum(s[4])) {
		return 2;
	} else {
		return 3;
	}
}

void convert::index_all(const string& file, unsigned depth)
{
	string s;
	unsigned index0 = 0;
	unsigned index1 = 0;
	unsigned index2 = 0;

	int j = 0;
	index_begin();
	while (j < file.length()) {
		string u;
		s = token(j, file);
		u = up(s);
		if (u != "NAME" && u != "NAME{NUMBER}" && u != "INDEX" && u != "SUBINDEX" && u != "SUBSUBINDEX") {
			unsigned level = index_level(s);
			if (depth >= level) {
				next(level, index0, index1, index2);
				string t = trim(s);
				while (j < file.length()) {
					int jj = j;
					s = token(j, file);
					if (index_level(s) != level) {
						j = jj;
						break;
					}
					t += " ";
					t += trim(s);
				}
				index_text(level, index0, index1, index2, t);
			}
		}
	}
	index_end();
}

void convert::run()
{
	int i;
	string file;
	string s;

	// read the whole file
	getline(is, file, static_cast<char>(EOF));

	state = state_filled;
	request_separator = false;

	i = 0;
	s = token(i, file);
	if (up(s) == "NAME" || up(s) == "NAME{NUMBER}") {
		numbered = up(s) == "NAME{NUMBER}";
		s = token(i, file);
		unsigned d = s.find(" - ");
		header(trim(s.substr(0, d)), trim(s.substr(d+3)));
	} else {
		header("", "");
		i = 0;
	}

	level0 = 0;
	level1 = 0;
	level2 = 0;

	while (i < file.length()) {
		string u;
		s = token(i, file);
		u = up(s);
		if (u == "INDEX") {
			index_all(file, 0);
		} else if (u == "SUBINDEX") {
			index_all(file, 1);
		} else if (u == "SUBSUBINDEX") {
			index_all(file, 2);
		} else {
			step(s);
		}
	}

	// end
	if (state == state_section0 || state == state_section1 || state == state_section2) {
		section_end();
	}
	if (state == state_para0 || state == state_para1) {
		para_end();
	}
	if (state == state_tag0 || state == state_tag1) {
		tag_stop();
		tag_end();
	}
	if (state == state_option) {
		option_stop();
		option_end();
	}
	if (state == state_dot0 || state == state_dot1) {
		dot_stop();
		dot_end();
	}
	if (state == state_pre0 || state == state_pre1) {
		pre_end();
	}

	footer();
}

//---------------------------------------------------------------------------
// convert_man

class convert_man : public convert {
	bool para_indent;
	string mask(string s);
public:
	convert_man(istream& Ais, ostream& Aos) : convert(Ais, Aos) { };

	virtual void header(const string& a, const string& b);
	virtual void footer();

	virtual void sep();
	virtual void line();

	virtual void section_begin(unsigned level);
	virtual void section_end();
	virtual void section_text(const string& s);

	virtual void para_begin(unsigned level);
	virtual void para_end();
	virtual void para_text(const string& s);

	virtual void pre_begin(unsigned level);
	virtual void pre_end();
	virtual void pre_text(const string& s);

	virtual void dot_begin(unsigned level);
	virtual void dot_end();
	virtual void dot_start(const string& s);
	virtual void dot_stop();
	virtual void dot_text(const string& s);

	virtual void option_begin();
	virtual void option_end();
	virtual void option_start(const string& s);
	virtual void option_stop();
	virtual void option_text(const string& s);

	virtual void tag_begin(unsigned level);
	virtual void tag_end();
	virtual void tag_start(const string& a, const string& b);
	virtual void tag_stop();
	virtual void tag_text(const string& s);
};

string convert_man::mask(string s)
{
	string r;
	for(unsigned i=0;i<s.length();++i) {
		switch (s[i]) {
		case '"' :
			r += "\\(a\"";
			break;
		case '\'' :
			r += "\\(cq";
			break;
		case '`' :
			r += "\\(oq";
			break;
		case '\\' :
			r += "\\(rs";
			break;
		case '-' :
			r += "\\(hy";
			break;
		default:
			r += s[i];
			break;
		}
	}
	return r;
}

void convert_man::header(const string& a, const string& b)
{
	os << ".TH \"" << mask(b) << "\" 1" << endl;
	os << ".SH NAME" << endl;
	os << mask(a + " - " + b) << endl;
}

void convert_man::footer()
{
}

void convert_man::section_begin(unsigned level)
{
	if (level == 0)
		os << ".SH ";
	else
		os << ".SS ";
}

void convert_man::section_end()
{
	os << endl;
	state = state_filled;
}

void convert_man::section_text(const string& s)
{
	if (state == state_section0)
		os << mask(up(s)) << " ";
	else
		os << mask(s) << " ";
}

void convert_man::para_begin(unsigned level)
{
	if (state == state_separator)
		sep();
	if (level)
		os << ".RS 4" << endl;
}

void convert_man::para_end()
{
	if (state == state_para1)
		os << ".RE" << endl;
	state = state_separator;
}

void convert_man::para_text(const string& s)
{
	os << mask(s) << endl;
}

void convert_man::pre_begin(unsigned level)
{
	if (state == state_separator)
		sep();
	if (level)
		os << ".RS 4" << endl;
}

void convert_man::pre_end()
{
	if (state == state_pre1)
		os << ".RE" << endl;
	state = state_separator;
}

void convert_man::pre_text(const string& s)
{
	os << mask(s) << endl;
	os << ".PD 0" << endl;
	os << ".PP" << endl;
	os << ".PD" << endl;
}

void convert_man::sep()
{
	if (state == state_tag0 || state == state_tag1) {
		os << ".PD" << endl;
		os << ".PP" << endl;
		if (!para_indent) {
			os << ".RS 4" << endl;
			para_indent = true;
		}
		os << ".PD 0" << endl;
	} else {
		os << ".PP" << endl;
	}
}

void convert_man::line()
{
	if (state == state_tag0 || state == state_tag1 || state == state_dot0 || state == state_dot1) {
		os << ".PP" << endl;
	} else {
		os << ".PD 0" << endl;
		os << ".PP" << endl;
		os << ".PD" << endl;
	}
}

void convert_man::dot_begin(unsigned level)
{
	os << ".PD 0" << endl;
}

void convert_man::dot_end()
{
	os << ".PD" << endl;
	state = state_filled;
}

void convert_man::dot_start(const string& s)
{
	os << ".IP \\(bu" << endl;
	os << mask(s) << endl;
}

void convert_man::dot_stop()
{
}

void convert_man::dot_text(const string& s)
{
	os << mask(s) << endl;
}

void convert_man::option_begin()
{
}

void convert_man::option_end()
{
	state = state_filled;
}

void convert_man::option_start(const string& s)
{
	os << ".TP" << endl;
	os << ".B " << mask(s) << endl;
}

void convert_man::option_stop()
{
}

void convert_man::option_text(const string& s)
{
	os << mask(s) << endl;
}

void convert_man::tag_begin(unsigned level)
{
	if (level)
		os << ".RS 4" << endl;
	else
		os << ".RS 0" << endl;
	os << ".PD 0" << endl;
	para_indent = false;
}

void convert_man::tag_end()
{
	os << ".PD" << endl;
	os << ".RE" << endl;
	if (para_indent) {
		os << ".RE" << endl;
		para_indent = false;
	}
	state = state_filled;
}

void convert_man::tag_start(const string& a, const string& b)
{
	if (para_indent) {
		os << ".RE" << endl;
		para_indent = false;
	}
	os << ".HP 4" << endl;
	os << ".I " << mask(a) << endl;
	os << mask(b) << endl;
}

void convert_man::tag_stop()
{
}

void convert_man::tag_text(const string& s)
{
	os << mask(s) << endl;
}

//---------------------------------------------------------------------------
// convert_html

#define HTML_H1 "h2"
#define HTML_H2 "h3"
#define HTML_H3 "h4"

class convert_html : public convert {
protected:
	string mask(string s);
	string link(string s);
public:
	convert_html(istream& Ais, ostream& Aos) : convert(Ais, Aos) { };

	virtual void header(const string& a, const string& b);
	virtual void footer();

	virtual void sep();
	virtual void line();

	virtual void index_begin();
	virtual void index_end();
	virtual void index_text(unsigned level, unsigned index0, unsigned index1, unsigned index2, const string& s);

	virtual void section_begin(unsigned level);
	virtual void section_end();
	virtual void section_text(const string& s);

	virtual void para_begin(unsigned level);
	virtual void para_end();
	virtual void para_text(const string& s);

	virtual void pre_begin(unsigned level);
	virtual void pre_end();
	virtual void pre_text(const string& s);

	virtual void dot_begin(unsigned level);
	virtual void dot_end();
	virtual void dot_start(const string& s);
	virtual void dot_stop();
	virtual void dot_text(const string& s);

	virtual void option_begin();
	virtual void option_end();
	virtual void option_start(const string& s);
	virtual void option_stop();
	virtual void option_text(const string& s);

	virtual void tag_begin(unsigned level);
	virtual void tag_end();
	virtual void tag_start(const string& a, const string& b);
	virtual void tag_stop();
	virtual void tag_text(const string& s);
};

string convert_html::mask(string s)
{
	string r;
	for(unsigned i=0;i<s.length();++i) {
		switch (s[i]) {
		case '<' :
			r += "&lt;";
			break;
		case '>' :
			r += "&gt;";
			break;
		case '\t' :
			r += "&nbsp;&nbsp;&nbsp;&nbsp;";
			break;
		case '&' :
			r += "&amp;";
			break;
		default:
			r += s[i];
			break;
		}
	}
	return r;
}

string convert_html::link(string s)
{
	int i = s.find("http://");

	if (i == string::npos) {
		i = s.find("ftp://");
	}

	if (i == string::npos) {
		i = s.find("file://");
	}

	if (i == string::npos) {
		return s;
	}

	int begin = i;
	int end = i;
	while (end<s.length() && !isspace(s[end]))
		++end;

	if (end>0 && (s[end-1] == '.' || s[end-1] == ',' || s[end-1] == ':' || s[end-1] == ';' || s[end-1] == '>' || s[end-1] == ')'))
		--end;

	string address = s.substr(begin, end - begin);
	s.erase(begin, end - begin);

	string l = "<a href=\"" + address + "\">" + address + "</a>";

	s.insert(begin, l);

	return s;
}

void convert_html::header(const string& a, const string& b)
{
	os << "<html>" << endl;
	os << "<head>" << endl;
	os << "<title>" << mask(b) << "</title>" << endl;
	os << "</head>" << endl;
	os << "<body>" << endl;
	if (b.length()) {
		os << "<center><" HTML_H1 ">" << mask(b) << "</" HTML_H1 "></center>" << endl;
	}
}

void convert_html::footer()
{
	os << "</body>" << endl;
	os << "</html>" << endl;
}

void convert_html::index_begin()
{
	os << "<p>The sections of this document are:<p>\n";
	os << "<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"2\">\n";
}

void convert_html::index_end()
{
	os << "</table>\n";
}

void convert_html::index_text(unsigned level, unsigned index0, unsigned index1, unsigned index2, const string& s)
{
	if (level == 0) {
		os << "<tr><td width=\"5%\"></td><td colspan=3>\n";
		os << "<a href=\"#" << index0 << "\">";
		os << index0;
		os << " " << s << "</a><br>\n";
		os << "</td></tr>\n";
	} else if (level == 1) {
		if (index0) {
			os << "<tr><td width=\"5%\"></td><td width=\"5%\"></td><td colspan=2>\n";
			os << "<a href=\"#" << index0 << "." << index1 << "\">";
			os << index0 << "." << index1;
			os << " " << s << "</a><br>\n";
			os << "</td></tr>\n";
		}
	} else if (level == 2) {
		if (index0 && index1) {
			os << "<tr><td width=\"5%\"></td><td width=\"5%\"></td><td width=\"5%\"></td><td>\n";
			os << "<a href=\"#" << index0 << "." << index1 << "." << index2 << "\">";
			os << index0 << "." << index1 << "." << index2;
			os << " " << s << "</a><br>\n";
			os << "</td></tr>\n";
		}
	}
}

void convert_html::section_begin(unsigned level)
{
	if (level == 0) {
		os << "<" HTML_H1 ">";
		if (numbered) {
			os << "<a name=\"" << level0 << "\">";
			os << level0;
			os << "</a>";
		}
		os << " " << endl;
	} else if (level == 1) {
		if (level0) {
			os << "<" HTML_H2 ">";
			if (numbered) {
				os << "<a name=\"" << level0 << "." << level1 << "\">";
				os << level0 << "." << level1;
				os << "</a>";
			}
			os << " " << endl;
		} else {
			os << "<" HTML_H2 ">" << endl;
		}
	} else {
		if (level0 && level1) {
			os << "<" HTML_H3 ">";
			if (numbered) {
				os << "<a name=\"" << level0 << "." << level1 << "." << level2 << "\">";
				os << level0 << "." << level1 << "." << level2;
				os << "</a>";
			}
			os << " " << endl;
		} else {
			os << "<" HTML_H3 ">" << endl;
		}
	}
}

void convert_html::section_end()
{
	if (state == state_section0)
		os << "</" HTML_H1 ">" << endl;
	else if (state == state_section1)
		os << "</" HTML_H2 ">" << endl;
	else
		os << "</" HTML_H3 ">" << endl;
	state = state_filled;
}

void convert_html::section_text(const string& s)
{
	os << link(mask(s)) << endl;
}

void convert_html::para_begin(unsigned level)
{
	if (state == state_separator)
		sep();
	if (level) {
		os << "<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"2\"><tr>";
		os << "<td width=\"5%\"></td><td width=\"95%\">" << endl;
	}
}

void convert_html::para_end()
{
	if (state == state_para1) {
		os << "</td></tr></table>" << endl;
	}
	state = state_filled;
}

void convert_html::para_text(const string& s)
{
	os << link(mask(s)) << endl;
}

void convert_html::pre_begin(unsigned level)
{
	if (state == state_separator)
		sep();
	if (level) {
		os << "<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"2\"><tr>";
		os << "<td width=\"5%\"></td><td width=\"95%\">" << endl;
	}
#if 0
	os << "<font face=\"Courier\">" << endl;
#else
	os << "<pre>" << endl;
#endif
}

void convert_html::pre_end()
{
#if 0
	os << "</font>" << endl;
#else
	os << "</pre>" << endl;
#endif
	if (state == state_pre1) {
		os << "</td></tr></table>" << endl;
	}
	state = state_filled;
}

void convert_html::pre_text(const string& s)
{
	os << link(mask(s));
#if 0
	os  << "<br>";
#endif
	os << "\n";
}

void convert_html::sep()
{
	os << "<p>" << endl;
}

void convert_html::line()
{
	os << "<br>" << endl;
}

void convert_html::dot_begin(unsigned level)
{
	os << "<ul>" << endl;
}

void convert_html::dot_end()
{
	os << "</ul>" << endl;
	state = state_filled;
}

void convert_html::dot_start(const string& s)
{
	os << "<li>" << endl;
	os << mask(s) << endl;
}

void convert_html::dot_stop()
{
	os << "</li>" << endl;
}

void convert_html::dot_text(const string& s)
{
	os << link(mask(s)) << endl;
}

void convert_html::option_begin()
{
	os << "<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"2\">" << endl;
}

void convert_html::option_end()
{
	os << "</table>" << endl;
	state = state_filled;
}

void convert_html::option_start(const string& s)
{
	os << "<tr valign=\"top\" align=\"left\"><td><strong>" << endl;
	os << mask(s) << endl;
	os << "</strong></td></tr><tr><td>" << endl;
}

void convert_html::option_stop()
{
	os << "</td></tr>" << endl;
}

void convert_html::option_text(const string& s)
{
	os << link(mask(s)) << endl;
}

void convert_html::tag_begin(unsigned level)
{
	os << "<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"2\">" << endl;
}

void convert_html::tag_end()
{
	os << "</table>" << endl;
	state = state_separator;
}

void convert_html::tag_start(const string& a, const string& b)
{
	os << "<tr valign=\"top\" align=\"left\"><td width=\"5%\"></td><td width=\"5%\"><em>" << endl;
	os << mask(a) << endl;
	os << "</em></td><td width=\"90%\">" << endl;
	os << mask(b) << endl;
}

void convert_html::tag_stop()
{
	os << "</td></tr>" << endl;
}

void convert_html::tag_text(const string& s)
{
	os << link(mask(s)) << endl;
}

//---------------------------------------------------------------------------
// convert_frame

class convert_frame : public convert_html {
public:
	convert_frame(istream& Ais, ostream& Aos) : convert_html(Ais, Aos) { };

	virtual void header(const string& a, const string& b);
	virtual void footer();
};

void convert_frame::header(const string& a, const string& b)
{
	if (b.length()) {
		os << "<center><" HTML_H1 ">" << mask(b) << "</" HTML_H1 "></center>" << endl;
	}
}

void convert_frame::footer()
{
}

//---------------------------------------------------------------------------
// convert_txt

class convert_txt : public convert {
	bool first_line;
	unsigned max_length;
	string mask(string s);
public:
	convert_txt(istream& Ais, ostream& Aos) : convert(Ais, Aos) { };

	virtual void header(const string& a, const string& b);
	virtual void footer();

	virtual void sep();
	virtual void line();

	virtual void section_begin(unsigned level);
	virtual void section_end();
	virtual void section_text(const string& s);

	virtual void para_begin(unsigned level);
	virtual void para_end();
	virtual void para_text(const string& s);

	virtual void pre_begin(unsigned level);
	virtual void pre_end();
	virtual void pre_text(const string& s);

	virtual void dot_begin(unsigned level);
	virtual void dot_end();
	virtual void dot_start(const string& s);
	virtual void dot_stop();
	virtual void dot_text(const string& s);

	virtual void option_begin();
	virtual void option_end();
	virtual void option_start(const string& s);
	virtual void option_stop();
	virtual void option_text(const string& s);

	virtual void tag_begin(unsigned level);
	virtual void tag_end();
	virtual void tag_start(const string& a, const string& b);
	virtual void tag_stop();
	virtual void tag_text(const string& s);
};

#define MI "" // begin
#define I "    " // tag

string convert_txt::mask(string s)
{
	return s;
}

void convert_txt::header(const string& a, const string& b)
{
	if (b.length()) {
		unsigned space = (80 - b.length()) / 2;
		os << fill(space, ' ') << fill(b.length(), '=') << endl;
		os << fill(space, ' ') << b << endl;
		os << fill(space, ' ') << fill(b.length(), '=') << endl;
	}
}

void convert_txt::footer()
{
}

void convert_txt::section_begin(unsigned level)
{
	if (level == 0)
		os << endl;
	if (state == state_separator)
		os << endl;
	first_line = true;
	max_length = 0;
}

void convert_txt::section_end()
{
	if (state == state_section0) {
		os << fill(max_length, '=') << endl;
		os << endl;
	} else if (state == state_section1) {
		os << fill(max_length, '-') << endl;
		os << endl;
	} else {
	}
	state = state_filled;
}

void convert_txt::section_text(const string& s)
{
	ostringstream ss;
	if (first_line) {
		if (state == state_section0) {
			if (numbered)
				ss << level0 << " ";
			ss << up(mask(s));
		} else if (state == state_section1) {
			if (numbered)
				ss << level0 << "." << level1 << " ";
			ss << mask(s);
		} else {
			ss << "---- ";
			if (numbered)
				ss << level0 << "." << level1 << "." << level2 << " ";
			ss << mask(s) << " ----";
		}
		first_line = false;
	} else {
		if (state == state_section0) {
			ss << up(mask(s));
		} else if (state == state_section1) {
			ss << mask(s);
		} else {
			ss << "---- " << mask(s) << " ----";
		}
	}
	if (ss.str().length() > max_length)
		max_length = ss.str().length();
	os << ss.str() << endl;
}


void convert_txt::para_begin(unsigned level)
{
	if (state == state_separator)
		sep();
}

void convert_txt::para_end()
{
	state = state_filled;
}

void convert_txt::para_text(const string& s)
{
	os << MI;
	if (state == state_para1)
		os << I;
	os << mask(s) << endl;
}

void convert_txt::pre_begin(unsigned level)
{
	if (state == state_separator)
		sep();
}

void convert_txt::pre_end()
{
	state = state_filled;
}

void convert_txt::pre_text(const string& s)
{
	os << MI;
	if (state == state_pre1)
		os << I;
	os << mask(s) << endl;
}

void convert_txt::sep()
{
	os << endl;
}

void convert_txt::line()
{
}

void convert_txt::dot_begin(unsigned level)
{
	if (state == state_separator)
		sep();
}

void convert_txt::dot_end()
{
	state = state_filled;
}

void convert_txt::dot_start(const string& s)
{
	os << MI;
	if (state == state_dot1)
		os << I ;
	os << "* " << mask(s) << endl;
}

void convert_txt::dot_stop()
{
}

void convert_txt::dot_text(const string& s)
{
	os << MI;
	if (state == state_dot1)
		os << I;
	os << "  " << mask(s) << endl;
}

void convert_txt::option_begin()
{
	if (state == state_separator)
		sep();
}

void convert_txt::option_end()
{
	state = state_filled;
}

void convert_txt::option_start(const string& s)
{
	os << MI;
	os << I << mask(s) << endl;
}

void convert_txt::option_stop()
{
}

void convert_txt::option_text(const string& s)
{
	os << MI;
	os << I I << mask(s) << endl;
}

void convert_txt::tag_begin(unsigned level)
{
	if (state == state_separator)
		sep();
}

void convert_txt::tag_end()
{
	state = state_filled;
}

void convert_txt::tag_start(const string& a, const string& b)
{
	os << MI;
	if (state == state_tag1)
		os << I;
	os << mask(a) << " - " << mask(b) << endl;
}

void convert_txt::tag_stop()
{
}

void convert_txt::tag_text(const string& s)
{
	os << MI;
	if (state == state_tag1)
		os << I ;
	os << I << mask(s) << endl;
}

//---------------------------------------------------------------------------
// main

int main(int argc, char* argv[])
{
	if (argc != 2) {
		cerr << "Syntax: txt2 man | html | frame | txt" << endl;
		exit(EXIT_FAILURE);
	}

	convert* c;

	string arg = argv[1];

	if (arg == "html")
		c = new convert_html(cin, cout);
	else if (arg == "frame")
		c = new convert_frame(cin, cout);
	else if (arg == "man")
		c = new convert_man(cin, cout);
	else if (arg == "txt")
		c = new convert_txt(cin, cout);
	else {
		cerr << "Unknown format `" << arg << "'" << endl;
		exit(EXIT_FAILURE);
	}

	c->run();

	delete c;

	return EXIT_SUCCESS;
}

