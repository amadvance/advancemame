#include <iostream>
#include <list>
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

void Interp5(unsigned p[10], int p1, int p2) {
	p[p1] += 8;
	p[p2] += 8;
}

void Interp6(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 10;
	p[p2] += 4;
	p[p3] += 2;
}

void Interp7(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 12;
	p[p2] += 2;
	p[p3] += 2;
}

void Interp8(unsigned p[10], int p1, int p2) {
	p[p1] += 10;
	p[p2] += 6;
}

#define PIXEL00_0     Copy(DST[i][0], 5);
#define PIXEL00_11    Interp1(DST[i][0], 5, 4);
#define PIXEL00_12    Interp1(DST[i][0], 5, 2);
#define PIXEL00_20    Interp2(DST[i][0], 5, 2, 4);
#define PIXEL00_50    Interp5(DST[i][0], 2, 4);
#define PIXEL00_80    Interp8(DST[i][0], 5, 1);
#define PIXEL00_81    Interp8(DST[i][0], 5, 4);
#define PIXEL00_82    Interp8(DST[i][0], 5, 2);
#define PIXEL01_0     Copy(DST[i][1], 5);
#define PIXEL01_10    Interp1(DST[i][1], 5, 1);
#define PIXEL01_12    Interp1(DST[i][1], 5, 2);
#define PIXEL01_14    Interp1(DST[i][1], 2, 5);
#define PIXEL01_21    Interp2(DST[i][1], 2, 5, 4);
#define PIXEL01_31    Interp3(DST[i][1], 5, 4);
#define PIXEL01_50    Interp5(DST[i][1], 2, 5);
#define PIXEL01_60    Interp6(DST[i][1], 5, 2, 4);
#define PIXEL01_61    Interp6(DST[i][1], 5, 2, 1);
#define PIXEL01_82    Interp8(DST[i][1], 5, 2);
#define PIXEL01_83    Interp8(DST[i][1], 2, 4);
#define PIXEL02_0     Copy(DST[i][2], 5);
#define PIXEL02_10    Interp1(DST[i][2], 5, 3);
#define PIXEL02_11    Interp1(DST[i][2], 5, 2);
#define PIXEL02_13    Interp1(DST[i][2], 2, 5);
#define PIXEL02_21    Interp2(DST[i][2], 2, 5, 6);
#define PIXEL02_32    Interp3(DST[i][2], 5, 6);
#define PIXEL02_50    Interp5(DST[i][2], 2, 5);
#define PIXEL02_60    Interp6(DST[i][2], 5, 2, 6);
#define PIXEL02_61    Interp6(DST[i][2], 5, 2, 3);
#define PIXEL02_81    Interp8(DST[i][2], 5, 2);
#define PIXEL02_83    Interp8(DST[i][2], 2, 6);
#define PIXEL03_0     Copy(DST[i][3], 5);
#define PIXEL03_11    Interp1(DST[i][3], 5, 2);
#define PIXEL03_12    Interp1(DST[i][3], 5, 6);
#define PIXEL03_20    Interp2(DST[i][3], 5, 2, 6);
#define PIXEL03_50    Interp5(DST[i][3], 2, 6);
#define PIXEL03_80    Interp8(DST[i][3], 5, 3);
#define PIXEL03_81    Interp8(DST[i][3], 5, 2);
#define PIXEL03_82    Interp8(DST[i][3], 5, 6);
#define PIXEL10_0     Copy(DST[i][4], 5);
#define PIXEL10_10    Interp1(DST[i][4], 5, 1);
#define PIXEL10_11    Interp1(DST[i][4], 5, 4);
#define PIXEL10_13    Interp1(DST[i][4], 4, 5);
#define PIXEL10_21    Interp2(DST[i][4], 4, 5, 2);
#define PIXEL10_32    Interp3(DST[i][4], 5, 2);
#define PIXEL10_50    Interp5(DST[i][4], 4, 5);
#define PIXEL10_60    Interp6(DST[i][4], 5, 4, 2);
#define PIXEL10_61    Interp6(DST[i][4], 5, 4, 1);
#define PIXEL10_81    Interp8(DST[i][4], 5, 4);
#define PIXEL10_83    Interp8(DST[i][4], 4, 2);
#define PIXEL11_0     Copy(DST[i][5], 5);
#define PIXEL11_30    Interp3(DST[i][5], 5, 1);
#define PIXEL11_31    Interp3(DST[i][5], 5, 4);
#define PIXEL11_32    Interp3(DST[i][5], 5, 2);
#define PIXEL11_70    Interp7(DST[i][5], 5, 4, 2);
#define PIXEL12_0     Copy(DST[i][6], 5);
#define PIXEL12_30    Interp3(DST[i][6], 5, 3);
#define PIXEL12_31    Interp3(DST[i][6], 5, 2);
#define PIXEL12_32    Interp3(DST[i][6], 5, 6);
#define PIXEL12_70    Interp7(DST[i][6], 5, 6, 2);
#define PIXEL13_0     Copy(DST[i][7], 5);
#define PIXEL13_10    Interp1(DST[i][7], 5, 3);
#define PIXEL13_12    Interp1(DST[i][7], 5, 6);
#define PIXEL13_14    Interp1(DST[i][7], 6, 5);
#define PIXEL13_21    Interp2(DST[i][7], 6, 5, 2);
#define PIXEL13_31    Interp3(DST[i][7], 5, 2);
#define PIXEL13_50    Interp5(DST[i][7], 6, 5);
#define PIXEL13_60    Interp6(DST[i][7], 5, 6, 2);
#define PIXEL13_61    Interp6(DST[i][7], 5, 6, 3);
#define PIXEL13_82    Interp8(DST[i][7], 5, 6);
#define PIXEL13_83    Interp8(DST[i][7], 6, 2);
#define PIXEL20_0     Copy(DST[i][8], 5);
#define PIXEL20_10    Interp1(DST[i][8], 5, 7);
#define PIXEL20_12    Interp1(DST[i][8], 5, 4);
#define PIXEL20_14    Interp1(DST[i][8], 4, 5);
#define PIXEL20_21    Interp2(DST[i][8], 4, 5, 8);
#define PIXEL20_31    Interp3(DST[i][8], 5, 8);
#define PIXEL20_50    Interp5(DST[i][8], 4, 5);
#define PIXEL20_60    Interp6(DST[i][8], 5, 4, 8);
#define PIXEL20_61    Interp6(DST[i][8], 5, 4, 7);
#define PIXEL20_82    Interp8(DST[i][8], 5, 4);
#define PIXEL20_83    Interp8(DST[i][8], 4, 8);
#define PIXEL21_0     Copy(DST[i][9], 5);
#define PIXEL21_30    Interp3(DST[i][9], 5, 7);
#define PIXEL21_31    Interp3(DST[i][9], 5, 8);
#define PIXEL21_32    Interp3(DST[i][9], 5, 4);
#define PIXEL21_70    Interp7(DST[i][9], 5, 4, 8);
#define PIXEL22_0     Copy(DST[i][10], 5);
#define PIXEL22_30    Interp3(DST[i][10], 5, 9);
#define PIXEL22_31    Interp3(DST[i][10], 5, 6);
#define PIXEL22_32    Interp3(DST[i][10], 5, 8);
#define PIXEL22_70    Interp7(DST[i][10], 5, 6, 8);
#define PIXEL23_0     Copy(DST[i][11], 5);
#define PIXEL23_10    Interp1(DST[i][11], 5, 9);
#define PIXEL23_11    Interp1(DST[i][11], 5, 6);
#define PIXEL23_13    Interp1(DST[i][11], 6, 5);
#define PIXEL23_21    Interp2(DST[i][11], 6, 5, 8);
#define PIXEL23_32    Interp3(DST[i][11], 5, 8);
#define PIXEL23_50    Interp5(DST[i][11], 6, 5);
#define PIXEL23_60    Interp6(DST[i][11], 5, 6, 8);
#define PIXEL23_61    Interp6(DST[i][11], 5, 6, 9);
#define PIXEL23_81    Interp8(DST[i][11], 5, 6);
#define PIXEL23_83    Interp8(DST[i][11], 6, 8);
#define PIXEL30_0     Copy(DST[i][12], 5);
#define PIXEL30_11    Interp1(DST[i][12], 5, 8);
#define PIXEL30_12    Interp1(DST[i][12], 5, 4);
#define PIXEL30_20    Interp2(DST[i][12], 5, 8, 4);
#define PIXEL30_50    Interp5(DST[i][12], 8, 4);
#define PIXEL30_80    Interp8(DST[i][12], 5, 7);
#define PIXEL30_81    Interp8(DST[i][12], 5, 8);
#define PIXEL30_82    Interp8(DST[i][12], 5, 4);
#define PIXEL31_0     Copy(DST[i][13], 5);
#define PIXEL31_10    Interp1(DST[i][13], 5, 7);
#define PIXEL31_11    Interp1(DST[i][13], 5, 8);
#define PIXEL31_13    Interp1(DST[i][13], 8, 5);
#define PIXEL31_21    Interp2(DST[i][13], 8, 5, 4);
#define PIXEL31_32    Interp3(DST[i][13], 5, 4);
#define PIXEL31_50    Interp5(DST[i][13], 8, 5);
#define PIXEL31_60    Interp6(DST[i][13], 5, 8, 4);
#define PIXEL31_61    Interp6(DST[i][13], 5, 8, 7);
#define PIXEL31_81    Interp8(DST[i][13], 5, 8);
#define PIXEL31_83    Interp8(DST[i][13], 8, 4);
#define PIXEL32_0     Copy(DST[i][14], 5);
#define PIXEL32_10    Interp1(DST[i][14], 5, 9);
#define PIXEL32_12    Interp1(DST[i][14], 5, 8);
#define PIXEL32_14    Interp1(DST[i][14], 8, 5);
#define PIXEL32_21    Interp2(DST[i][14], 8, 5, 6);
#define PIXEL32_31    Interp3(DST[i][14], 5, 6);
#define PIXEL32_50    Interp5(DST[i][14], 8, 5);
#define PIXEL32_60    Interp6(DST[i][14], 5, 8, 6);
#define PIXEL32_61    Interp6(DST[i][14], 5, 8, 9);
#define PIXEL32_82    Interp8(DST[i][14], 5, 8);
#define PIXEL32_83    Interp8(DST[i][14], 8, 6);
#define PIXEL33_0     Copy(DST[i][15], 5);
#define PIXEL33_11    Interp1(DST[i][15], 5, 6);
#define PIXEL33_12    Interp1(DST[i][15], 5, 8);
#define PIXEL33_20    Interp2(DST[i][15], 5, 8, 6);
#define PIXEL33_50    Interp5(DST[i][15], 8, 6);
#define PIXEL33_80    Interp8(DST[i][15], 5, 9);
#define PIXEL33_81    Interp8(DST[i][15], 5, 6);
#define PIXEL33_82    Interp8(DST[i][15], 5, 8);

#define N 16

unsigned DST[4096][N][10];

int main(int argc, char* argv[])
{
	int ver = 0;
	if (argc > 1)
		ver = atoi(argv[1]);

	for(unsigned i=0;i<256*2*2*2*2;++i) {
		switch (i & 0xFF) {
		#include "o4.h"
		}
		if (ver == 1) {
			for(unsigned k=0;k<N;++k) {
				simplify(DST[i][k], i);
			}
		}
		if (ver == 2) {
			for(unsigned k=0;k<N;++k) {
				discrete(DST[i][k], i);
			}
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
		set<cond> cs;
		assign_set assign_base;
		for(unsigned j=0;j<N;++j) {
			bool positive;
			ostringstream p;
			p << "P(" << j % 4 << ", " << j / 4 << ")";
			if (m[j].size() == 1) {
				set<item>::iterator l=m[j].begin();
				cout << p.str() << " = " << assign(p.str(),interp(l->p),assign_base) << ";" << endl;
			} else if (m[j].size() == 2 && condition_is_perfect_split(m[j].begin()->mask,positive)) {
				set<item>::iterator l;
				set<item>::iterator h;
				if (!positive) {
					h = m[j].begin();
					l = h;
					++l;
				} else {
					l = m[j].begin();
					h = l;
					++h;
				}
				string c = condition(l->mask);
				pair<set<cond>::iterator,bool> n = cs.insert(cond(c));
				pair<string,string> i0, i1;
				i1.first = p.str();
				i1.second = interp(l->p);
				i0.first = p.str();
				i0.second = interp(h->p);
				n.first->if_true.insert(n.first->if_true.end(), i1);
				n.first->if_false.insert(n.first->if_false.end(), i0);
			} else {
				for(set<item>::iterator l=m[j].begin();l!=m[j].end();++l) {
					string c = condition(l->mask);
					pair<set<cond>::iterator,bool> n = cs.insert(cond(c));
					pair<string,string> i1;
					i1.first = p.str();
					i1.second = interp(l->p);
					n.first->if_true.insert(n.first->if_true.end(), i1);
				}
			}
		}
		for(set<cond>::iterator j=cs.begin();j!=cs.end();++j) {
			assign_set assign_cond;
			cout << "if (" << j->name << ") {" << endl;
			assign_cond = assign_base;
			for(assign_set::iterator k=j->if_true.begin();k!=j->if_true.end();++k)
				cout << "\t" << k->first << " = " << assign(k->first, k->second, assign_cond) << ";" << endl;
			if (j->if_false.size() > 0) {
				cout << "} else {" << endl;
				assign_cond = assign_base;
				for(assign_set::iterator k=j->if_false.begin();k!=j->if_false.end();++k)
					cout << "\t" << k->first << " = " << assign(k->first, k->second, assign_cond) << ";" << endl;
			}
			cout << "}" << endl;
		}
		cout << "} break;" << endl;
	}

	return 0;
}
