#include <string>
#include <fstream>
#include <iostream>

using namespace std;

bool isname(char c) {
	return ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c=='_';
}

void subs(const string& from, const string& to, string& s) {
	unsigned i = 0;
	char p0 = from[0];
	char p1 = from[from.length() - 1];
	bool bp0 = isname(p0);
	bool bp1 = isname(p1);
	while (i < s.length()) {
		unsigned p = s.find(from,i);
		if (p == string::npos) {
			break;
		} else {
			char c0 = p==0 ? ' ' : s[p-1];
			char c1 = p+from.length() == s.length() ? ' ' : s[p+from.length()];
			bool eq0 = bp0 && isname(c0);
			bool eq1 = bp1 && isname(c1);
			if (!eq0 && !eq1) {
				s.erase(p,from.length());
				s.insert(p,to);
				i = p + to.length();
			} else {
				i = p + 1;
			}
		}
	}
}

void process(const char* from, const char* to, const char* path) {
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

	subs(from,to,s);

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
	if (argc < 4) {
		cout << "Syntax: subs FROM TO files..." << endl;
		exit(0);
	}
	for(i=3;i<argc;++i)
		process(argv[1],argv[2],argv[i]);
	return 0;
}
