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

void Interp3(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 12;
	p[p2] += 2;
	p[p3] += 2;
}

void Interp4(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 14;
	p[p2] += 1;
	p[p3] += 1;
}

void Interp5(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 10;
	p[p2] += 4;
	p[p3] += 2;
}

void Interp6(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 4;
	p[p2] += 6;
	p[p3] += 6;
}

#define PIXEL00_0     Copy(DST[i][0], 5);
#define PIXEL00_10    Interp1(DST[i][0], 5, 1);
#define PIXEL00_11    Interp1(DST[i][0], 5, 4);
#define PIXEL00_12    Interp1(DST[i][0], 5, 2);
#define PIXEL00_20    Interp2(DST[i][0], 5, 4, 2);
#define PIXEL00_30    Interp3(DST[i][0], 5, 4, 2);
#define PIXEL00_40    Interp4(DST[i][0], 5, 4, 2);
#define PIXEL00_50    Interp5(DST[i][0], 5, 2, 4);
#define PIXEL00_51    Interp5(DST[i][0], 5, 4, 2);
#define PIXEL00_60    Interp6(DST[i][0], 5, 4, 2);
#define PIXEL01_0     Copy(DST[i][1], 5);
#define PIXEL01_10    Interp1(DST[i][1], 5, 3);
#define PIXEL01_11    Interp1(DST[i][1], 5, 2);
#define PIXEL01_12    Interp1(DST[i][1], 5, 6);
#define PIXEL01_20    Interp2(DST[i][1], 5, 2, 6);
#define PIXEL01_30    Interp3(DST[i][1], 5, 2, 6);
#define PIXEL01_40    Interp4(DST[i][1], 5, 2, 6);
#define PIXEL01_50    Interp5(DST[i][1], 5, 6, 2);
#define PIXEL01_51    Interp5(DST[i][1], 5, 2, 6);
#define PIXEL01_60    Interp6(DST[i][1], 5, 2, 6);
#define PIXEL10_0     Copy(DST[i][2], 5);
#define PIXEL10_10    Interp1(DST[i][2], 5, 7);
#define PIXEL10_11    Interp1(DST[i][2], 5, 8);
#define PIXEL10_12    Interp1(DST[i][2], 5, 4);
#define PIXEL10_20    Interp2(DST[i][2], 5, 8, 4);
#define PIXEL10_30    Interp3(DST[i][2], 5, 8, 4);
#define PIXEL10_40    Interp4(DST[i][2], 5, 8, 4);
#define PIXEL10_50    Interp5(DST[i][2], 5, 4, 8);
#define PIXEL10_51    Interp5(DST[i][2], 5, 8, 4);
#define PIXEL10_60    Interp6(DST[i][2], 5, 8, 4);
#define PIXEL11_0     Copy(DST[i][3], 5);
#define PIXEL11_10    Interp1(DST[i][3], 5, 9);
#define PIXEL11_11    Interp1(DST[i][3], 5, 6);
#define PIXEL11_12    Interp1(DST[i][3], 5, 8);
#define PIXEL11_20    Interp2(DST[i][3], 5, 6, 8);
#define PIXEL11_30    Interp3(DST[i][3], 5, 6, 8);
#define PIXEL11_40    Interp4(DST[i][3], 5, 6, 8);
#define PIXEL11_50    Interp5(DST[i][3], 5, 8, 6);
#define PIXEL11_51    Interp5(DST[i][3], 5, 6, 8);
#define PIXEL11_60    Interp6(DST[i][3], 5, 6, 8);

#define N 4

unsigned DST[4096][N][10];

int main() {

	for(unsigned i=0;i<256*2*2*2*2;++i) {
		switch (i & 0xFF) {
		#include "o2.h"
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
