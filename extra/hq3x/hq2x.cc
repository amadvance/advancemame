#include <iostream>
#include <set>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

#include "common.h"

unsigned DST3[4096][9][10];
unsigned DST2[4096][4][10];

void approx(unsigned p[10]) {
	unsigned M = 8;
	for(unsigned i=1;i<=9;++i) {
		p[i] = (unsigned)(p[i] * (double)M / 144.0 + 0.5);
	}
	unsigned max = p[5];
	unsigned pmax = 5;
	unsigned sum = 0;
	for(unsigned i=1;i<=9;++i) {
		sum += p[i];
		if (p[i] > max)
			pmax = i;
	}
	p[pmax] += M - sum;
}

int main() {

	for(unsigned i=0;i<256*2*2*2*2;++i) {

		switch (i & 0xFF) {
		#include "o.h"
		}

		for(unsigned j=1;j<=9;++j) {
			DST2[i][0][j] = DST3[i][0][j] * 4 + DST3[i][1][j] * 2 + DST3[i][3][j] * 2 + DST3[i][4][j];
			DST2[i][1][j] = DST3[i][2][j] * 4 + DST3[i][1][j] * 2 + DST3[i][5][j] * 2 + DST3[i][4][j];
			DST2[i][2][j] = DST3[i][6][j] * 4 + DST3[i][3][j] * 2 + DST3[i][7][j] * 2 + DST3[i][4][j];
			DST2[i][3][j] = DST3[i][8][j] * 4 + DST3[i][7][j] * 2 + DST3[i][5][j] * 2 + DST3[i][4][j];
		}

		for(unsigned j=0;j<4;++j) {
			approx(DST2[i][j]);
		}
	}

	set<unsigned> excluded;

	for(unsigned i=0;i<256;++i) {
		if (excluded.find(i) != excluded.end())
			continue;

		set<item> m[4];

		cout << "case " << i << " : " << endl;
		for(unsigned j=i+1;j<256;++j) {
			bool equal = true;
			for(unsigned k=0;k<16 && equal;++k) {
				if (memcmp(DST2[i+k*256], DST2[j+k*256], sizeof(DST2[0])) != 0)
					equal = false;
			}
			if (equal) {
				cout << "case " << j << " : " << endl;
				excluded.insert(j);
			}
		}

		cout << "{" << endl;
		for(unsigned h=0;h<4;++h) {
			for(unsigned k=0;k<16;++k) {
				item v;
				memcpy(v.p, DST2[i+k*256][h], sizeof(v.p));
				pair<set<item>::iterator,bool> l = m[h].insert(v);
				l.first->mask.insert(k);
			}
		}
		for(unsigned j=0;j<4;++j) {
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
