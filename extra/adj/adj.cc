#include <string>
#include <fstream>
#include <iostream>

using namespace std;

bool isname(char c) {
	return ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c=='_';
}

class detect {
	unsigned len;
	const char* tag;
	unsigned match;
	bool complete;
	bool pred_sep;
public:
	detect(const char* s);

	void process(char c);
	bool is() const;
};

detect::detect(const char* s) {
	len = strlen(s);
	tag = s;
	match = 0;
	complete = false;
	pred_sep = true;
}

bool detect::is() const
{
	return complete;
}

void detect::process(char c)
{
	if (c=='\n') {
		match = 0;
		complete = false;
		pred_sep = true;
	} else if (complete) {
	} else if (match == 0) {
		if (pred_sep && c==tag[0])
			match = 1;
	} else if (match == len) {
		if (!isname(c)) {
			complete = true;
		} else {
			match = 0;
		}
	} else {
		if (c==tag[match])
			++match;
		else
			match = 0;
	}

	pred_sep = !isname(c);
}

void subs(string& s) {
	unsigned i = 0;
	bool zero_start = false;
	bool in_string = false;
	char pred = 0;
	char begin_string = 0;

	while (i < s.length()) {
		if (!in_string && pred==',' && !isspace(s[i])) {
			s.insert(i, " ");
		}

		if (!in_string) {
			if (s[i]=='\'' || s[i] == '\"') {
				in_string = true;
				begin_string = s[i];
			}
		} else {
			if (s[i] == begin_string && pred != '\\') {
				in_string = false;
			}
		}

		if (pred == '\n') {
			zero_start = !isspace(s[i]);
		}

		if (zero_start && s[i]==')' && s[i+1]==' ' && s[i+2]=='{' && s[i+3]=='\n') {
			s[i+1] = '\n';
		}

		pred = s[i];

		++i;
	}
}

void process(const char* path) {
	ifstream fi(path);
	if (!fi) {
		cerr << "Error opening for reading" << path << endl;
		exit(1);
	}

	string s;
	getline(fi,s,(char)EOF);
	if (fi.bad()) {
		cerr << "Error reading" << path << endl;
		exit(1);
	}

	fi.close();

	subs(s);

	ofstream fo(path);
	if (!fo) {
		cerr << "Error opening for writing" << path << endl;
		exit(1);
	}

	fo << s;
	if (fo.bad()) {
		cerr << "Error writing" << path << endl;
		exit(1);
	}

	fo.close();
}

int main(int argc, char* argv[]) {
	int i;
	if (argc < 2) {
		cout << "Syntax: adj files..." << endl;
		exit(0);
	}
	for(i=1;i<argc;++i)
		process(argv[i]);
	return 0;
}
