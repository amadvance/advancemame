#include <iostream>
#include <set>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

#include "common.h"

void Copy(unsigned p[10], int p1) {
	p[p1] += 16;
}

void Interp1(unsigned p[10], int p1, int p2) {
	p[p1] += 12;
	p[p2] += 4;
}

void Interp2(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 8;
	p[p2] += 4;
	p[p3] += 4;
}

void Interp3(unsigned p[10], int p1, int p2) {
	p[p1] += 14;
	p[p2] += 2;
}

void Interp4(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 2;
	p[p2] += 7;
	p[p3] += 7;
}

void Interp5(unsigned p[10], int p1, int p2) {
	p[p1] += 8;
	p[p2] += 8;
}

#define PIXEL00_1M  Interp1(DST[i][0], 5, 1);
#define PIXEL00_1U  Interp1(DST[i][0], 5, 2);
#define PIXEL00_1L  Interp1(DST[i][0], 5, 4);
#define PIXEL00_2   Interp2(DST[i][0], 5, 4, 2);
#define PIXEL00_4   Interp4(DST[i][0], 5, 4, 2);
#define PIXEL00_5   Interp5(DST[i][0], 4, 2);
#define PIXEL00_C   Copy(DST[i][0], 5);
#define PIXEL01_1   Interp1(DST[i][1], 5, 2);
#define PIXEL01_3   Interp3(DST[i][1], 5, 2);
#define PIXEL01_6   Interp1(DST[i][1], 2, 5);
#define PIXEL01_C   Copy(DST[i][1], 5);
#define PIXEL02_1M  Interp1(DST[i][2], 5, 3);
#define PIXEL02_1U  Interp1(DST[i][2], 5, 2);
#define PIXEL02_1R  Interp1(DST[i][2], 5, 6);
#define PIXEL02_2   Interp2(DST[i][2], 5, 2, 6);
#define PIXEL02_4   Interp4(DST[i][2], 5, 2, 6);
#define PIXEL02_5   Interp5(DST[i][2], 2, 6);
#define PIXEL02_C   Copy(DST[i][2], 5);
#define PIXEL10_1   Interp1(DST[i][3], 5, 4);
#define PIXEL10_3   Interp3(DST[i][3], 5, 4);
#define PIXEL10_6   Interp1(DST[i][3], 4, 5);
#define PIXEL10_C   Copy(DST[i][3], 5);
#define PIXEL11     Copy(DST[i][4], 5);
#define PIXEL12_1   Interp1(DST[i][5], 5, 6);
#define PIXEL12_3   Interp3(DST[i][5], 5, 6);
#define PIXEL12_6   Interp1(DST[i][5], 6, 5);
#define PIXEL12_C   Copy(DST[i][5], 5);
#define PIXEL20_1M  Interp1(DST[i][6], 5, 7);
#define PIXEL20_1D  Interp1(DST[i][6], 5, 8);
#define PIXEL20_1L  Interp1(DST[i][6], 5, 4);
#define PIXEL20_2   Interp2(DST[i][6], 5, 8, 4);
#define PIXEL20_4   Interp4(DST[i][6], 5, 8, 4);
#define PIXEL20_5   Interp5(DST[i][6], 8, 4);
#define PIXEL20_C   Copy(DST[i][6], 5);
#define PIXEL21_1   Interp1(DST[i][7], 5, 8);
#define PIXEL21_3   Interp3(DST[i][7], 5, 8);
#define PIXEL21_6   Interp1(DST[i][7], 8, 5);
#define PIXEL21_C   Copy(DST[i][7], 5);
#define PIXEL22_1M  Interp1(DST[i][8], 5, 9);
#define PIXEL22_1D  Interp1(DST[i][8], 5, 8);
#define PIXEL22_1R  Interp1(DST[i][8], 5, 6);
#define PIXEL22_2   Interp2(DST[i][8], 5, 6, 8);
#define PIXEL22_4   Interp4(DST[i][8], 5, 6, 8);
#define PIXEL22_5   Interp5(DST[i][8], 6, 8);
#define PIXEL22_C   Copy(DST[i][8], 5);

#define N 9

unsigned DST[4096][N][10];

int main() {

	for(unsigned i=0;i<256*2*2*2*2;++i) {
		switch (i & 0xFF) {
		#include "o3.h"
		}
	}

	set<unsigned> excluded;

	for(unsigned i=0;i<256;++i) {
		if (excluded.find(i) != excluded.end())
			continue;

		set<item> m[N];

		cout << "case " << i << " : " << endl;
		for(unsigned j=i+1;j<256;++j) {
			bool equal = true;
			for(unsigned k=0;k<16 && equal;++k) {
				if (memcmp(DST[i+k*256], DST[j+k*256], sizeof(DST[0])) != 0)
					equal = false;
			}
			if (equal) {
				cout << "case " << j << " : " << endl;
				excluded.insert(j);
			}
		}

		cout << "{" << endl;
		for(unsigned h=0;h<N;++h) {
			for(unsigned k=0;k<16;++k) {
				item v;
				memcpy(v.p, DST[i+k*256][h], sizeof(v.p));
				pair<set<item>::iterator,bool> l = m[h].insert(v);
				l.first->mask.insert(k);
			}
		}
		for(unsigned j=0;j<N;++j) {
			if (m[j].size() == 1) {
				set<item>::iterator l=m[j].begin();
				cout << "P" << j << " = " << interp(l->p) << ";" << endl;
			} else if (m[j].size() == 2) {
				set<item>::iterator l;
				set<item>::iterator h;
				if (condition(m[j].begin()->mask)[0] == '!') {
					h = m[j].begin();
					l = h;
					++l;
				} else {
					l = m[j].begin();
					h = l;
					++h;
				}
				string c = condition(l->mask);
				cout << "if (" << c << ") {" << endl;
				cout << "P" << j << " = " << interp(l->p) << ";" << endl;
				cout << "} else {" << endl;
				cout << "P" << j << " = " << interp(h->p) << ";" << endl;
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
