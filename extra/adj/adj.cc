#include <string>
#include <fstream>
#include <iostream>

using namespace std;

bool match(const string& s, unsigned i, const string& token)
{
	return s.substr(i, token.length()) == token;
}

bool eol(const string& s, unsigned i)
{
	while (i < s.length()) {
		if (s[i] == '\n')
			return true;
		if (!isspace(s[i]))
			return false;
		++i;
	}

	return true;
}

bool matcheol(const string& s, unsigned i, const string& token)
{
	return s.substr(i, token.length()) == token
		&& eol(s, i + token.length());
}

enum state_t {
	state_code,
	state_declaration,
	state_char,
	state_string,
	state_comment,
	state_commentline
};

void subs(string& s)
{
	unsigned i = 0;
	state_t state = state_code;
	char pred = 0;

	while (i < s.length()) {
		char next_pred = s[i];

		switch (state) {
		case state_declaration :
			if (matcheol(s, i, ") {"))
				s[i+1] = '\n';
			if (matcheol(s,i,") const {"))
				s[i+7] = '\n';
			/* nobreak */
		case state_code :
			if (match(s, i, " )"))
				s.erase(i, 1);
			if (match(s, i, "( "))
				s.erase(i+1, 1);

			if (s[i] == ',' && !isspace(s[i+1]))
				s.insert(i+1, " ");
			if (s[i] == '\'')
				state = state_char;
			if (s[i] == '\"')
				state = state_string;
			if (match(s, i, "//"))
				state = state_commentline;
			if (match(s, i, "/*"))
				state = state_comment;

			if (s[i] == '\n') {
				if ((i+1<s.length()) && !isspace(s[i+1])
					&& !match(s, i, "\nstruct")
					&& !match(s, i, "\nclass")
					&& !match(s, i, "\nunion")
					&& !match(s, i, "\nenum")
					&& !match(s, i, "\ntypedef")
				) {
					state = state_declaration;
				} else {
					state = state_code;
				}
			}
			break;
		case state_comment :
			if (match(s, i, "*/"))
				state = state_code;
			break;
		case state_commentline :
			if (s[i] == '\n')
				state = state_code;
			break;
		case state_string :
			if (s[i] == '\\' && pred == '\\')
				next_pred = ' '; /* prevent incorrect detection of "\\" */
			if (s[i] == '\"' && pred != '\\')
				state = state_code;
			if (s[i] == '\n') {
				cerr << "Invalid return in a string" << endl;
				abort();
			}
			break;
		case state_char :
			if (s[i] == '\\' && pred == '\\')
				next_pred = ' '; /* prevent incorrect detection of '\\' */
			if (s[i] == '\'' && pred != '\\')
				state = state_code;
			if (s[i] == '\n') {
				cerr << "Invalid return in a string" << endl;
				abort();
			}
			break;
		}

		pred = next_pred;

		++i;
	}
}

void process(const char* path)
{
	ifstream fi(path);
	if (!fi) {
		cerr << "Error opening for reading" << path << endl;
		exit(1);
	}

	string s;
	getline(fi, s, (char)EOF);
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

int main(int argc, char* argv[])
{
	int i;
	if (argc < 2) {
		cout << "Syntax: adj files..." << endl;
		exit(0);
	}

	for(i=1;i<argc;++i)
		process(argv[i]);

	return 0;
}

