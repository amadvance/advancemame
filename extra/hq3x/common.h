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

typedef list<pair<string,string> > assign_set;

string assign(const string& r, const string& s, assign_set& assign)
{
	if (s.find("IC") != string::npos)
		return s;

	assign_set::iterator i;
	for(i=assign.begin();i!=assign.end();++i) {
		if (i->second == s)
			break;
	}

	if (i == assign.end()) {
		pair<string,string> p;
		p.first = r;
		p.second = s;
		assign.insert(assign.end(), p);
		return s;
	} else {
#if 0 /* disable common subexpression, you cannot reread the value from the destination */
		return i->first;
#else
		cerr << "warning: possible common subexpression " << i->first << " = " << s << endl;
		return s;
#endif
	}
}

class cond {
public:
	cond(const string& Aname);

	string name;
	mutable assign_set if_true;
	mutable assign_set if_false;

	bool operator<(const cond& A) const;
};

cond::cond(const string& Aname) : name(Aname) {
}

bool cond::operator<(const cond& A) const {
	return name < A.name;
}

unsigned maskbit(unsigned i) {
	switch (i) {
	case 0 : return 0x1;
	case 1 : return 0x2;
	case 2 : return 0x4;
	case 3 : return 0x8;
	case 5 : return 0x10;
	case 6 : return 0x20;
	case 7 : return 0x40;
	case 8 : return 0x80;
	}
	return 0;
}

bool isequal(unsigned i, unsigned j, unsigned maskfull) {
	if (i == j) {
		return 1;
	}
	if (i == 4) {
		return (maskfull & maskbit(j)) == 0;
	}
	if (j == 4) {
		return (maskfull & maskbit(i)) == 0;
	}
	if ((maskfull & maskbit(i)) == 0 && (maskfull & maskbit(j)) == 0) {
		return 1;
	}
/* if enabled it will generate complex test M cases */
#if 0
	if ((i==2 && j==6) || (i==6 && j==2)) {
		return (maskfull & 0x100) == 0;
	}
	if ((i==6 && j==8) || (i==8 && j==6)) {
		return (maskfull & 0x200) == 0;
	}
	if ((i==8 && j==4) || (i==4 && j==8)) {
		return (maskfull & 0x400) == 0;
	}
	if ((i==4 && j==2) || (i==2 && j==4)) {
		return (maskfull & 0x800) == 0;
	}
#endif
	return 0;
}

void simplify(unsigned p[10], unsigned maskfull) {
	for(unsigned i=0;i<9;++i) {
		for(unsigned j=i+1;j<9;++j) {
			if (isequal(i, j, maskfull)) {
				p[i+1] += p[j+1];
				p[j+1] = 0;
			}
		}
	}
}

