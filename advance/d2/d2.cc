#include <string>
#include <iostream>

using namespace std;

//---------------------------------------------------------------------------
// string

string fill(unsigned l, char c) {
	string r;
	for(unsigned i=0;i<l;++i)
		r += c;
	return r;
}

string trim_left(const string& s) {
	string r = s;
	while (r.length() > 0 && isspace(r[0]))
		r.erase(0,1);
	return r;
}

string trim_right(const string& s) {
	string r = s;
	while (r.length() > 0 && isspace(r[r.length()-1]))
		r.erase(r.length()-1,1);
	return r;
}

string trim(const string& s) {
	return trim_left(trim_right(s));
}

string up(const string& s) {
	string r;
	for(unsigned i=0;i<s.length();++i)
		r += toupper(s[i]);
	return r;
}

string cap(const string& s) {
	string r;
	for(unsigned i=0;i<s.length();++i)
		if (i==0 || isspace(s[i-1]))
			r += toupper(s[i]);
		else
			r += tolower(s[i]);
	return r;
}

//---------------------------------------------------------------------------
// convert

enum state_t {
	state_section,
	state_subsection,
	state_separator,
	state_option,
	state_dot0,
	state_dot1,
	state_tag0,
	state_tag1,
	state_pre0,
	state_pre1,
	state_text
};

class convert {
	void step(string& s);
	bool is_option(const string& s);
	bool is_tag(const string& s, string& a, string& b);
	bool is_dot(const string& s, string& a);
	bool is_pre(const string& s, string& a);
protected:
	istream& is;
	ostream& os;
	state_t state;
public:
	convert(istream& Ais, ostream& Aos) : is(Ais), os(Aos) { };
	virtual ~convert();

	void run();

	virtual void header(const string& a, const string& b) = 0;
	virtual void footer() = 0;

	virtual void section(const string& s) = 0;
	virtual void subsection(const string& s) = 0;
	virtual void text(const string& s) = 0;
	virtual void para() = 0;

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

convert::~convert() {
}

bool convert::is_pre(const string& s, string& a) {
	if (s.length() > 0 && s[0] == ':') {
		a = s.substr(1);
		return true;
	}
	return false;
}

bool convert::is_dot(const string& s, string& a) {
	if (s.length() > 0 && (s[0] == '*' || s[0]==')') && s[1] == ' ') {
		a = s.substr(2);
		return true;
	}
	return false;
}

bool convert::is_option(const string& s) {
	string r = trim(s);

	if (r.length() > 0 && (r[0] == '-' || r[0] =='/')) {
		return true;
	} else {
		for(unsigned i=0;i<r.length();++i) {
			if (islower(r[i]) || isspace(r[i]))
				return false;
		}
	}

	return true;
}

bool convert::is_tag(const string& s, string& a, string& b) {
	if (s.length() > 0 && s[0] == ':')
		return false;
	unsigned d = s.find(" - ");
	if (d == string::npos)
		return false;
	a = trim(s.substr(0,d));
	b = trim(s.substr(d+3));
	return true;
}

void convert::step(string& s)
{
	s = trim_right(s);

	// count left space
	unsigned nt = 0;
	while (nt < s.length() && s[nt] == '\t')
		++nt;
	s.erase(0,nt);
	unsigned ns = 0;
	while (ns < s.length() && s[ns] == ' ')
		++ns;
	s.erase(0,ns);
	ns += nt*8;

	if ((ns == 16 && state == state_tag0) || (ns == 24 && state == state_tag1)) {
		tag_text(s);
		return;
	}

	if (s.length() == 0 && (state == state_tag0 || state == state_tag1)) {
		para();
		return;
	}

	if (ns == 16 && state == state_option) {
		option_text(s);
		return;
	}

	if ((ns == 16 && state == state_dot0) || (ns == 24 && state == state_dot1)) {
		dot_text(s);
		return;
	}

	if (ns == 8 || ns == 16) {
		string a,b;
		if (is_tag(s,a,b)) {
			state_t state_new = ns == 8 ? state_tag0 : state_tag1;
			if (state != state_new) {
				if (state == state_separator)
					para();
				tag_begin(ns == 16);
			} else {
				tag_stop();
			}
			state = state_new;
			tag_start(a,b);
			return;
		}
	}
	if (state == state_tag0 || state == state_tag1) {
		tag_stop();
		tag_end();
	}

	if (ns == 8) {
		if (is_option(s) && state != state_text) {
			if (state != state_option) {
				option_begin();
			} else {
				option_stop();
			}
			state = state_option;
			option_start(s);
			return;
		}
	}
	if (state == state_option) {
		option_stop();
		option_end();
	}

	if (ns == 8 || ns == 16) {
		string a;
		if (is_dot(s,a)) {
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
		if (is_pre(s,a)) {
			state_t state_new = ns == 8 ? state_pre0 : state_pre1;
			if (state == state_separator)
				para();
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

	if (ns == 8) {
		if (state == state_separator)
			para();
		text(s);
		state = state_text;
		return;
	}

	if (s.length() == 0) {
		state = state_separator;
		return;
	}

	if (ns == 0) {
		section(s);
		state = state_section;
		return;
	}

	if (ns == 4) {
		subsection(s);
		state = state_subsection;
		return;
	}

	text(s);
	state = state_text;
	return;
}

void convert::run() {
	string s;
	state = state_text;

	getline(is, s);
	if (s == "NAME") {
		getline(is, s);
		unsigned d = s.find(" - ");
		header(trim(s.substr(0,d)),trim(s.substr(d+3)));
	} else {
		header("","");
		step(s);
	}

	while (!is.eof()) {
		getline(is, s);
		step(s);
	}

	footer();
}

//---------------------------------------------------------------------------
// convert_man

class convert_man : public convert {
	bool para_indent;
	string mask(string s);
public:
	convert_man(istream& Ais, ostream& Aos) : convert(Ais,Aos) { };

	virtual void header(const string& a, const string& b);
	virtual void footer();

	virtual void section(const string& s);
	virtual void subsection(const string& s);
	virtual void text(const string& s);
	virtual void para();

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

string convert_man::mask(string s) {
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

void convert_man::header(const string& a, const string& b) {
	os << ".TH \"" << mask(b) << "\" 1" << endl;
	os << ".SH NAME" << endl;
	os << mask(a + " - " + b) << endl;
}

void convert_man::footer() {
}

void convert_man::section(const string& s) {
	os << ".SH " << mask(s) << endl;
}

void convert_man::subsection(const string& s) {
	os << ".SS " << mask(s) << endl;
}

void convert_man::text(const string& s) {
	os << mask(s) << endl;
}

void convert_man::pre_begin(unsigned level) {
	os << ".PD 0" << endl;
	os << ".PP" << endl;
	if (level)
		os << ".RS 4" << endl;
}

void convert_man::pre_end() {
	os << ".PD" << endl;
	if (state == state_pre1)
		os << ".RE" << endl;
	state = state_separator;
}

void convert_man::pre_text(const string& s) {
	os << mask(s) << endl;
	os << ".PP" << endl;
}

void convert_man::para() {
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

void convert_man::dot_begin(unsigned level) {
}

void convert_man::dot_end() {
	state = state_text;
}

void convert_man::dot_start(const string& s) {
	os << ".IP \\(bu" << endl;
	os << mask(s) << endl;
}

void convert_man::dot_stop() {
}

void convert_man::dot_text(const string& s) {
	os << mask(s) << endl;
}

void convert_man::option_begin() {
}

void convert_man::option_end() {
	state = state_text;
}

void convert_man::option_start(const string& s) {
	os << ".TP" << endl;
	os << ".B " << mask(s) << endl;
}

void convert_man::option_stop() {
}

void convert_man::option_text(const string& s) {
	os << mask(s) << endl;
}

void convert_man::tag_begin(unsigned level) {
	if (level)
		os << ".RS 4" << endl;
	else
		os << ".RS 0" << endl;
	os << ".PD 0" << endl;
	para_indent = false;
}

void convert_man::tag_end() {
	os << ".PD" << endl;
	os << ".RE" << endl;
	if (para_indent) {
		os << ".RE" << endl;
		para_indent = false;
	}
	state = state_text;
}

void convert_man::tag_start(const string& a, const string& b) {
	if (para_indent) {
		os << ".RE" << endl;
		para_indent = false;
	}
	os << ".HP 4" << endl;
	os << ".I " << mask(a) << endl;
	os << mask(b) << endl;
}

void convert_man::tag_stop() {
}

void convert_man::tag_text(const string& s) {
	os << mask(s) << endl;
}

//---------------------------------------------------------------------------
// convert_html

class convert_html : public convert {
	string mask(string s);
public:
	convert_html(istream& Ais, ostream& Aos) : convert(Ais,Aos) { };

	virtual void header(const string& a, const string& b);
	virtual void footer();

	virtual void section(const string& s);
	virtual void subsection(const string& s);
	virtual void text(const string& s);
	virtual void para();

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

string convert_html::mask(string s) {
	string r;
	for(unsigned i=0;i<s.length();++i) {
		switch (s[i]) {
		default:
			r += s[i];
			break;
		}
	}
	return r;
}

void convert_html::header(const string& a, const string& b) {
	os << "<html>" << endl;
	os << "<head>" << endl;
	os << "<title>" << mask(b) << "</title>" << endl;
	os << "</head>" << endl;
	os << "<body>" << endl;
}

void convert_html::footer() {
	os << "</body>" << endl;
	os << "</html>" << endl;
}

void convert_html::section(const string& s) {
	os << "<h1>" << cap(mask(s)) << "</h1>" << endl;
}

void convert_html::subsection(const string& s) {
	os << "<h2>" << mask(s) << "</h2>" << endl;
}

void convert_html::text(const string& s) {
	os << mask(s) << endl;
}

void convert_html::pre_begin(unsigned level) {
	os << "<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"2\"><tr>";
	if (level == 0)
		os << "<td>" << endl;
	else
		os << "<td width=\"5%\"></td><td width=\"95%\">" << endl;
	os << "<font face=\"Courier\">" << endl;
}

void convert_html::pre_end() {
	os << "</font>" << endl;
	os << "</tr></td></table>" << endl;
	state = state_text;
}

void convert_html::pre_text(const string& s) {
	os << mask(s) << "<br>" << endl;
}

void convert_html::para() {
	os << "<p>" << endl;
}

void convert_html::dot_begin(unsigned level) {
	os << "<ul>" << endl;
}

void convert_html::dot_end() {
	os << "</ul>" << endl;
	state = state_text;
}

void convert_html::dot_start(const string& s) {
	os << "<li>" << endl;
	os << mask(s) << endl;
}

void convert_html::dot_stop() {
	os << "</li>" << endl;
}

void convert_html::dot_text(const string& s) {
	os << mask(s) << endl;
}

void convert_html::option_begin() {
	os << "<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"2\">" << endl;
}

void convert_html::option_end() {
	os << "</table>" << endl;
	state = state_text;
}

void convert_html::option_start(const string& s) {
	os << "<tr valign=\"top\" align=\"left\"><td><strong>" << endl;
	os << mask(s) << endl;
	os << "</em></td></tr><tr><td>" << endl;
}

void convert_html::option_stop() {
	os << "</td></tr>" << endl;
}

void convert_html::option_text(const string& s) {
	os << mask(s) << endl;
}

void convert_html::tag_begin(unsigned level) {
	os << "<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"2\">" << endl;
}

void convert_html::tag_end() {
	os << "</table>" << endl;
	state = state_separator;
}

void convert_html::tag_start(const string& a, const string& b) {
	os << "<tr valign=\"top\" align=\"left\"><td width=\"5%\"></td><td width=\"5%\"><em>" << endl;
	os << mask(a) << endl;
	os << "</em></td><td width=\"90%\">" << endl;
	os << mask(b) << endl;
}

void convert_html::tag_stop() {
	os << "</td></tr>" << endl;
}

void convert_html::tag_text(const string& s) {
	os << mask(s) << endl;
}

//---------------------------------------------------------------------------
// convert_html

class convert_txt : public convert {
	string mask(string s);
public:
	convert_txt(istream& Ais, ostream& Aos) : convert(Ais,Aos) { };

	virtual void header(const string& a, const string& b);
	virtual void footer();

	virtual void section(const string& s);
	virtual void subsection(const string& s);
	virtual void text(const string& s);
	virtual void para();

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

#define I "    "

string convert_txt::mask(string s) {
	return s;
}

void convert_txt::header(const string& a, const string& b) {
	if (b.length()) {
		unsigned space = (80 - b.length()) / 2;
		os << fill(space,' ') << fill(b.length(),'=') << endl;
		os << fill(space,' ') << b << endl;
		os << fill(space,' ') << fill(b.length(),'=') << endl;
	}
}

void convert_txt::footer() {
}

void convert_txt::section(const string& s) {
	if (state == state_separator)
		os << endl;
	os << endl;
	os << up(s) << endl;
	os << fill(s.length(),'=') << endl;
	os << endl;
}

void convert_txt::subsection(const string& s) {
	if (state == state_separator)
		os << endl;
	os << "==== " << mask(s) << " ====" << endl;
}

void convert_txt::text(const string& s) {
	os << mask(s) << endl;
}

void convert_txt::pre_begin(unsigned level) {
}

void convert_txt::pre_end() {
	state = state_text;
}

void convert_txt::pre_text(const string& s) {
	if (state == state_pre1)
		os << I;
	os << mask(s) << endl;
}

void convert_txt::para() {
	os << endl;
}

void convert_txt::dot_begin(unsigned level) {
}

void convert_txt::dot_end() {
	state = state_text;
}

void convert_txt::dot_start(const string& s) {
	if (state == state_dot1)
		os << I ;
	os << "* " << mask(s) << endl;
}

void convert_txt::dot_stop() {
}

void convert_txt::dot_text(const string& s) {
	if (state == state_dot1)
		os << I;
	os << "  " << mask(s) << endl;
}

void convert_txt::option_begin() {
}

void convert_txt::option_end() {
	state = state_text;
}

void convert_txt::option_start(const string& s) {
	os << I << mask(s) << endl;
}

void convert_txt::option_stop() {
}

void convert_txt::option_text(const string& s) {
	os << I I << mask(s) << endl;
}

void convert_txt::tag_begin(unsigned level) {
}

void convert_txt::tag_end() {
	state = state_text;
}

void convert_txt::tag_start(const string& a, const string& b) {
	if (state == state_tag1)
		os << I;
	os << mask(a) << " - " << mask(b) << endl;
}

void convert_txt::tag_stop() {
}

void convert_txt::tag_text(const string& s) {
	if (state == state_tag1)
		os << I ;
	os << I << mask(s) << endl;
}

//---------------------------------------------------------------------------
// main

int main(int argc, char* argv[]) {
	if (argc != 2) {
		cerr << "Syntax: txt2 man | html | txt" << endl;
		exit(EXIT_FAILURE);
	}

	convert* c;

	string arg = argv[1];

	if (arg == "html")
		c = new convert_html(cin,cout);
	else if (arg == "man")
		c = new convert_man(cin,cout);
	else if (arg == "txt")
		c = new convert_txt(cin,cout);
	else {
		cerr << "Unknown format `" << arg << "'" << endl;
		exit(EXIT_FAILURE);
	}

	c->run();

	delete c;

	return EXIT_SUCCESS;
}
