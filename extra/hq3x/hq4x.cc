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
	p[p1] += 8;
	p[p2] += 8;
}

void Interp2(unsigned p[10], int p1, int p2) {
	p[p1] += 10;
	p[p2] += 6;
}

void Interp3(unsigned p[10], int p1, int p2) {
	p[p1] += 12;
	p[p2] += 4;
}

void Interp4(unsigned p[10], int p1, int p2) {
	p[p1] += 14;
	p[p2] += 2;
}

void Interp5(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 4;
	p[p2] += 6;
	p[p3] += 6;
}

void Interp6(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 8;
	p[p2] += 4;
	p[p3] += 4;
}

void Interp7(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 8;
	p[p2] += 6;
	p[p3] += 2;
}

void Interp8(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 12;
	p[p2] += 2;
	p[p3] += 2;
}

#define PIXEL00_0     Copy(DST[i][0], 5);
#define PIXEL00_10    Interp1(DST[i][0], 2, 4);
#define PIXEL00_20    Interp2(DST[i][0], 5, 1);
#define PIXEL00_21    Interp2(DST[i][0], 5, 4);
#define PIXEL00_22    Interp2(DST[i][0], 5, 2);
#define PIXEL00_31    Interp3(DST[i][0], 5, 4);
#define PIXEL00_32    Interp3(DST[i][0], 5, 2);
#define PIXEL00_50    Interp5(DST[i][0], 5, 2, 4);
#define PIXEL00_60    Interp6(DST[i][0], 5, 2, 4);
#define PIXEL01_0     Copy(DST[i][1], 5);
#define PIXEL01_10    Interp1(DST[i][1], 2, 5);
#define PIXEL01_22    Interp2(DST[i][1], 5, 2);
#define PIXEL01_23    Interp2(DST[i][1], 2, 4);
#define PIXEL01_30    Interp3(DST[i][1], 5, 1);
#define PIXEL01_32    Interp3(DST[i][1], 5, 2);
#define PIXEL01_34    Interp3(DST[i][1], 2, 5);
#define PIXEL01_41    Interp4(DST[i][1], 5, 4);
#define PIXEL01_61    Interp6(DST[i][1], 2, 5, 4);
#define PIXEL01_70    Interp7(DST[i][1], 5, 2, 4);
#define PIXEL02_0     Copy(DST[i][2], 5);
#define PIXEL02_10    Interp1(DST[i][2], 2, 5);
#define PIXEL02_21    Interp2(DST[i][2], 5, 2);
#define PIXEL02_23    Interp2(DST[i][2], 2, 6);
#define PIXEL02_30    Interp3(DST[i][2], 5, 3);
#define PIXEL02_31    Interp3(DST[i][2], 5, 2);
#define PIXEL02_33    Interp3(DST[i][2], 2, 5);
#define PIXEL02_42    Interp4(DST[i][2], 5, 6);
#define PIXEL02_61    Interp6(DST[i][2], 2, 5, 6);
#define PIXEL02_70    Interp7(DST[i][2], 5, 2, 6);
#define PIXEL03_0     Copy(DST[i][3], 5);
#define PIXEL03_10    Interp1(DST[i][3], 2, 6);
#define PIXEL03_20    Interp2(DST[i][3], 5, 3);
#define PIXEL03_21    Interp2(DST[i][3], 5, 2);
#define PIXEL03_22    Interp2(DST[i][3], 5, 6);
#define PIXEL03_31    Interp3(DST[i][3], 5, 2);
#define PIXEL03_32    Interp3(DST[i][3], 5, 6);
#define PIXEL03_50    Interp5(DST[i][3], 5, 2, 6);
#define PIXEL03_60    Interp6(DST[i][3], 5, 2, 6);
#define PIXEL10_0     Copy(DST[i][4], 5);
#define PIXEL10_10    Interp1(DST[i][4], 4, 5);
#define PIXEL10_21    Interp2(DST[i][4], 5, 4);
#define PIXEL10_23    Interp2(DST[i][4], 4, 2);
#define PIXEL10_30    Interp3(DST[i][4], 5, 1);
#define PIXEL10_31    Interp3(DST[i][4], 5, 4);
#define PIXEL10_33    Interp3(DST[i][4], 4, 5);
#define PIXEL10_42    Interp4(DST[i][4], 5, 2);
#define PIXEL10_61    Interp6(DST[i][4], 4, 5, 2);
#define PIXEL10_70    Interp7(DST[i][4], 5, 4, 2);
#define PIXEL11_0     Copy(DST[i][5], 5);
#define PIXEL11_40    Interp4(DST[i][5], 5, 1);
#define PIXEL11_41    Interp4(DST[i][5], 5, 4);
#define PIXEL11_42    Interp4(DST[i][5], 5, 2);
#define PIXEL11_80    Interp8(DST[i][5], 5, 4, 2);
#define PIXEL12_0     Copy(DST[i][6], 5);
#define PIXEL12_40    Interp4(DST[i][6], 5, 3);
#define PIXEL12_41    Interp4(DST[i][6], 5, 2);
#define PIXEL12_42    Interp4(DST[i][6], 5, 6);
#define PIXEL12_80    Interp8(DST[i][6], 5, 6, 2);
#define PIXEL13_0     Copy(DST[i][7], 5);
#define PIXEL13_10    Interp1(DST[i][7], 6, 5);
#define PIXEL13_22    Interp2(DST[i][7], 5, 6);
#define PIXEL13_23    Interp2(DST[i][7], 6, 2);
#define PIXEL13_30    Interp3(DST[i][7], 5, 3);
#define PIXEL13_32    Interp3(DST[i][7], 5, 6);
#define PIXEL13_34    Interp3(DST[i][7], 6, 5);
#define PIXEL13_41    Interp4(DST[i][7], 5, 2);
#define PIXEL13_61    Interp6(DST[i][7], 6, 5, 2);
#define PIXEL13_70    Interp7(DST[i][7], 5, 6, 2);
#define PIXEL20_0     Copy(DST[i][8], 5);
#define PIXEL20_10    Interp1(DST[i][8], 4, 5);
#define PIXEL20_22    Interp2(DST[i][8], 5, 4);
#define PIXEL20_23    Interp2(DST[i][8], 4, 8);
#define PIXEL20_30    Interp3(DST[i][8], 5, 7);
#define PIXEL20_32    Interp3(DST[i][8], 5, 4);
#define PIXEL20_34    Interp3(DST[i][8], 4, 5);
#define PIXEL20_41    Interp4(DST[i][8], 5, 8);
#define PIXEL20_61    Interp6(DST[i][8], 4, 5, 8);
#define PIXEL20_70    Interp7(DST[i][8], 5, 4, 8);
#define PIXEL21_0     Copy(DST[i][9], 5);
#define PIXEL21_40    Interp4(DST[i][9], 5, 7);
#define PIXEL21_41    Interp4(DST[i][9], 5, 8);
#define PIXEL21_42    Interp4(DST[i][9], 5, 4);
#define PIXEL21_80    Interp8(DST[i][9], 5, 4, 8);
#define PIXEL22_0     Copy(DST[i][10], 5);
#define PIXEL22_40    Interp4(DST[i][10], 5, 9);
#define PIXEL22_41    Interp4(DST[i][10], 5, 6);
#define PIXEL22_42    Interp4(DST[i][10], 5, 8);
#define PIXEL22_80    Interp8(DST[i][10], 5, 6, 8);
#define PIXEL23_0     Copy(DST[i][11], 5);
#define PIXEL23_10    Interp1(DST[i][11], 6, 5);
#define PIXEL23_21    Interp2(DST[i][11], 5, 6);
#define PIXEL23_23    Interp2(DST[i][11], 6, 8);
#define PIXEL23_30    Interp3(DST[i][11], 5, 9);
#define PIXEL23_31    Interp3(DST[i][11], 5, 6);
#define PIXEL23_33    Interp3(DST[i][11], 6, 5);
#define PIXEL23_42    Interp4(DST[i][11], 5, 8);
#define PIXEL23_61    Interp6(DST[i][11], 6, 5, 8);
#define PIXEL23_70    Interp7(DST[i][11], 5, 6, 8);
#define PIXEL30_0     Copy(DST[i][12], 5);
#define PIXEL30_10    Interp1(DST[i][12], 8, 4);
#define PIXEL30_20    Interp2(DST[i][12], 5, 7);
#define PIXEL30_21    Interp2(DST[i][12], 5, 8);
#define PIXEL30_22    Interp2(DST[i][12], 5, 4);
#define PIXEL30_31    Interp3(DST[i][12], 5, 8);
#define PIXEL30_32    Interp3(DST[i][12], 5, 4);
#define PIXEL30_50    Interp5(DST[i][12], 5, 8, 4);
#define PIXEL30_60    Interp6(DST[i][12], 5, 8, 4);
#define PIXEL31_0     Copy(DST[i][13], 5);
#define PIXEL31_10    Interp1(DST[i][13], 8, 5);
#define PIXEL31_21    Interp2(DST[i][13], 5, 8);
#define PIXEL31_23    Interp2(DST[i][13], 8, 4);
#define PIXEL31_30    Interp3(DST[i][13], 5, 7);
#define PIXEL31_31    Interp3(DST[i][13], 5, 8);
#define PIXEL31_33    Interp3(DST[i][13], 8, 5);
#define PIXEL31_42    Interp4(DST[i][13], 5, 4);
#define PIXEL31_61    Interp6(DST[i][13], 8, 5, 4);
#define PIXEL31_70    Interp7(DST[i][13], 5, 8, 4);
#define PIXEL32_0     Copy(DST[i][14], 5);
#define PIXEL32_10    Interp1(DST[i][14], 8, 5);
#define PIXEL32_22    Interp2(DST[i][14], 5, 8);
#define PIXEL32_23    Interp2(DST[i][14], 8, 6);
#define PIXEL32_30    Interp3(DST[i][14], 5, 9);
#define PIXEL32_32    Interp3(DST[i][14], 5, 8);
#define PIXEL32_34    Interp3(DST[i][14], 8, 5);
#define PIXEL32_41    Interp4(DST[i][14], 5, 6);
#define PIXEL32_61    Interp6(DST[i][14], 8, 5, 6);
#define PIXEL32_70    Interp7(DST[i][14], 5, 8, 6);
#define PIXEL33_0     Copy(DST[i][15], 5);
#define PIXEL33_10    Interp1(DST[i][15], 8, 6);
#define PIXEL33_20    Interp2(DST[i][15], 5, 9);
#define PIXEL33_21    Interp2(DST[i][15], 5, 6);
#define PIXEL33_22    Interp2(DST[i][15], 5, 8);
#define PIXEL33_31    Interp3(DST[i][15], 5, 6);
#define PIXEL33_32    Interp3(DST[i][15], 5, 8);
#define PIXEL33_50    Interp5(DST[i][15], 5, 8, 6);
#define PIXEL33_60    Interp6(DST[i][15], 5, 8, 6);

#define N 16

unsigned DST[4096][N][10];

int main() {

	for(unsigned i=0;i<256*2*2*2*2;++i) {
		switch (i & 0xFF) {
		#include "o4.h"
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
