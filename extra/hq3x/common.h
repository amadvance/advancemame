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

	unsigned d = 8;

	for(unsigned i=1;i<=9;++i) {
		while (d > 1 && (p[i] % d) != 0)
			d = d / 2;
	}

	for(unsigned i=1;i<=9;++i) {
		if (p[i]) {
			fact f;
			f.val = p[i] / d;
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

