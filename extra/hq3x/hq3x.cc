#include <iostream>
#include <set>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

#include "common.h"

unsigned DST3[4096][9][10];

int main() {

	for(unsigned i=0;i<256*2*2*2*2;++i) {
		switch (i & 0xFF) {
		#include "o.h"
		}
	}

	set<unsigned> excluded;

	for(unsigned i=0;i<256;++i) {
		if (excluded.find(i) != excluded.end())
			continue;

		set<item> m[9];

		cout << "case " << i << " : " << endl;
		for(unsigned j=i+1;j<256;++j) {
			bool equal = true;
			for(unsigned k=0;k<16 && equal;++k) {
				if (memcmp(DST3[i+k*256], DST3[j+k*256], sizeof(DST3[0])) != 0)
					equal = false;
			}
			if (equal) {
				cout << "case " << j << " : " << endl;
				excluded.insert(j);
			}
		}

		cout << "{" << endl;
		for(unsigned h=0;h<9;++h) {
			for(unsigned k=0;k<16;++k) {
				item v;
				memcpy(v.p, DST3[i+k*256][h], sizeof(v.p));
				pair<set<item>::iterator,bool> l = m[h].insert(v);
				l.first->mask.insert(k);
			}
		}
		for(unsigned j=0;j<9;++j) {
			if (m[j].size() == 1) {
				set<item>::iterator l=m[j].begin();
				cout << "P" << j << " = " << interp(l->p) << ";" << endl;
			} else if (m[j].size() == 2) {
				set<item>::iterator l=m[j].begin();
				string c = condition(l->mask);
				cout << "if (" << c << ") {" << endl;
				cout << "P" << j << " = " << interp(l->p) << ";" << endl;
				cout << "} else {" << endl;
				++l;
				cout << "P" << j << " = " << interp(l->p) << ";" << endl;
				cout << "}" << endl;
			} else {
				for(set<item>::iterator l=m[j].begin();l!=m[j].end();++l) {
					string c = condition(l->mask);
					cout << "if (" << c << ") {" << endl;
					cout << "P" << j << " = " << interp(l->p) << ";" << endl;
					cout << ");" << endl;
					cout << "}" << endl;
				}
			}
		}
		cout << "} break;" << endl;
	}

	return 0;
}
