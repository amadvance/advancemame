void copy(unsigned p[10], int p1) {
	p[p1] += 16;
}

void interp1(unsigned p[10], int p1, int p2) {
	p[p1] += 12;
	p[p2] += 4;
}

void interp2(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 8;
	p[p2] += 4;
	p[p3] += 4;
}

void interp3(unsigned p[10], int p1, int p2) {
	p[p1] += 14;
	p[p2] += 2;
}

void interp4(unsigned p[10], int p1, int p2, int p3) {
	p[p1] += 2;
	p[p2] += 7;
	p[p3] += 7;
}

void interp5(unsigned p[10], int p1, int p2) {
	p[p1] += 8;
	p[p2] += 8;
}

#define PIXEL00_1M  interp1(DST3[i][0], 5, 1);
#define PIXEL00_1U  interp1(DST3[i][0], 5, 2);
#define PIXEL00_1L  interp1(DST3[i][0], 5, 4);
#define PIXEL00_2   interp2(DST3[i][0], 5, 4, 2);
#define PIXEL00_4   interp4(DST3[i][0], 5, 4, 2);
#define PIXEL00_5   interp5(DST3[i][0], 4, 2);
#define PIXEL00_C   copy(DST3[i][0], 5);
#define PIXEL01_1   interp1(DST3[i][1], 5, 2);
#define PIXEL01_3   interp3(DST3[i][1], 5, 2);
#define PIXEL01_6   interp1(DST3[i][1], 2, 5);
#define PIXEL01_C   copy(DST3[i][1], 5);
#define PIXEL02_1M  interp1(DST3[i][2], 5, 3);
#define PIXEL02_1U  interp1(DST3[i][2], 5, 2);
#define PIXEL02_1R  interp1(DST3[i][2], 5, 6);
#define PIXEL02_2   interp2(DST3[i][2], 5, 2, 6);
#define PIXEL02_4   interp4(DST3[i][2], 5, 2, 6);
#define PIXEL02_5   interp5(DST3[i][2], 2, 6);
#define PIXEL02_C   copy(DST3[i][2], 5);
#define PIXEL10_1   interp1(DST3[i][3], 5, 4);
#define PIXEL10_3   interp3(DST3[i][3], 5, 4);
#define PIXEL10_6   interp1(DST3[i][3], 4, 5);
#define PIXEL10_C   copy(DST3[i][3], 5);
#define PIXEL11     copy(DST3[i][4], 5);
#define PIXEL12_1   interp1(DST3[i][5], 5, 6);
#define PIXEL12_3   interp3(DST3[i][5], 5, 6);
#define PIXEL12_6   interp1(DST3[i][5], 6, 5);
#define PIXEL12_C   copy(DST3[i][5], 5);
#define PIXEL20_1M  interp1(DST3[i][6], 5, 7);
#define PIXEL20_1D  interp1(DST3[i][6], 5, 8);
#define PIXEL20_1L  interp1(DST3[i][6], 5, 4);
#define PIXEL20_2   interp2(DST3[i][6], 5, 8, 4);
#define PIXEL20_4   interp4(DST3[i][6], 5, 8, 4);
#define PIXEL20_5   interp5(DST3[i][6], 8, 4);
#define PIXEL20_C   copy(DST3[i][6], 5);
#define PIXEL21_1   interp1(DST3[i][7], 5, 8);
#define PIXEL21_3   interp3(DST3[i][7], 5, 8);
#define PIXEL21_6   interp1(DST3[i][7], 8, 5);
#define PIXEL21_C   copy(DST3[i][7], 5);
#define PIXEL22_1M  interp1(DST3[i][8], 5, 9);
#define PIXEL22_1D  interp1(DST3[i][8], 5, 8);
#define PIXEL22_1R  interp1(DST3[i][8], 5, 6);
#define PIXEL22_2   interp2(DST3[i][8], 5, 6, 8);
#define PIXEL22_4   interp4(DST3[i][8], 5, 6, 8);
#define PIXEL22_5   interp5(DST3[i][8], 6, 8);
#define PIXEL22_C   copy(DST3[i][8], 5);

#define M26 i & 0x100
#define M68 i & 0x200
#define M84 i & 0x400
#define M42 i & 0x800

struct item {
	item();
	item(const item& A);

	unsigned p[10];
	mutable set<unsigned> mask;

	bool operator<(const item& A) const;
};

item::item() {
	memset(p, 0, sizeof(p));
}

item::item(const item& A) {
	memcpy(p, A.p, sizeof(p));
	mask = A.mask;
}

bool item::operator<(const item& A) const
{
	for(unsigned i=1;i<=9;++i) {
		if (p[i] < A.p[i])
			return true;
		if (p[i] > A.p[i])
			return false;
	}
	return false;
}

string condition(set<unsigned> mask)
{
	string s;

	if (mask.size() == 16)
		return s;

	if (mask.size() == 8) {
		unsigned v = 0xF;
		unsigned vn = 0xF;
		for(set<unsigned>::iterator i=mask.begin();i!=mask.end();++i) {
			v &= *i;
			vn &= ~*i;
		}

		if (v) {
			switch (v) {
			case 0x1 : s = "MUR"; break;
			case 0x2 : s = "MDR"; break;
			case 0x4 : s = "MDL"; break;
			case 0x8 : s = "MUL"; break;
			}
		} else {
			switch (vn) {
			case 0x1 : s = "!MUR"; break;
			case 0x2 : s = "!MDR"; break;
			case 0x4 : s = "!MDL"; break;
			case 0x8 : s = "!MUL"; break;
			}
		}

		if (s == "") {
			abort();
		}

		return s;
	}

	for(set<unsigned>::iterator i=mask.begin();i!=mask.end();++i) {
		if (i != mask.begin())
			s += " || ";
		s += "(";
		if ((*i & 0x1) == 0)
			s += "!";
		s += "MUR && ";
		if ((*i & 0x2) == 0)
			s += "!";
		s += "MDR && ";
		if ((*i & 0x4) == 0)
			s += "!";
		s += "MDL && ";
		if ((*i & 0x8) == 0)
			s += "!";
		s += "MUL)";
	}

	return s;
}

struct fact {
	unsigned val;
	unsigned pos;

	bool operator<(const fact& A) const;
};

bool fact::operator<(const fact& A) const {
	return (val > A.val);
}

string interp(const unsigned p[10]) {
	string s;

	multiset<fact> m;

	for(unsigned i=1;i<=9;++i) {
		if (p[i]) {
			fact f;
			f.val = p[i];
			f.pos = i;
			m.insert(f);
		}
	}

	s = "I";

	if (m.size()==1) {
		s += "C";
	} else {
		for(multiset<fact>::iterator i=m.begin();i!=m.end();++i) {
			ostringstream os;
			os << i->val;
			s += os.str();
		}
	}

	s += "(";

	for(multiset<fact>::iterator i=m.begin();i!=m.end();++i) {
		ostringstream os;
		if (i!=m.begin())
			s += ", ";
		os << i->pos - 1;
		s += os.str();
	}

	s += ")";

	return s;
}

