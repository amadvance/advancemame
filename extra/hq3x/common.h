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

unsigned bit[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

bool reduce(string& s, list<unsigned>& mask, unsigned v, unsigned vm)
{
	unsigned count = 0;

	v &= vm;

	for(list<unsigned>::iterator i=mask.begin();i!=mask.end();++i) {
		if ((*i & vm) == v)
			++count;
	}

	unsigned expected = 1 << (4 - bit[vm]);

	if (count == expected) {
		if (s.length() != 0)
			s += " || ";

		string r;
		unsigned count = 0;
		for(unsigned i=0;i<4;++i) {
			if (vm & (1 << i)) {
				++count;
				if (r.length() != 0)
					r += " && ";
				if ((v & (1 << i)) == 0)
					r += "!";
				switch (vm & (1 << i)) {
				case 0x1 : r += "MUR"; break;
				case 0x2 : r += "MDR"; break;
				case 0x4 : r += "MDL"; break;
				case 0x8 : r += "MUL"; break;
				}
			}
		}

		if (count>1)
			s += "(";
		s += r;
		if (count>1)
			s += ")";

		for(list<unsigned>::iterator i=mask.begin();i!=mask.end();) {
			if ((*i & vm) == v) {
				list<unsigned>::iterator j = i;
				++i;
				mask.erase(j);
			} else {
				++i;
			}
		}

		return true;
	} else {
		return false;
	}
}

bool condition_is_perfect_split(const set<unsigned>& mask, bool& positive) {
	if (mask.size() != 8)
		return false;

	unsigned v = 0xF;
	unsigned vn = 0xF;

	for(set<unsigned>::const_iterator i=mask.begin();i!=mask.end();++i) {
		v &= *i;
		vn &= ~*i;
	}

	if (v) {
		positive = true;
		return true;
	}

	if (vn) {
		positive = false;
		return true;
	}

	return false;
}

string condition(const set<unsigned>& mask)
{
	string s;

	if (mask.size() == 16)
		return s;

	list<unsigned> t;

	for(set<unsigned>::const_iterator i=mask.begin();i!=mask.end();++i) {
		t.insert(t.end(), *i);
	}

	while (t.size() != 0) {
		for(unsigned k=1;k<=4;++k) {
			for(unsigned i=0;i<16;++i) {
				if (bit[i] == k) {
					for(unsigned j=0;j<16;++j) {
						reduce(s,t,~j,i);
					}
				}
			}
		}
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
		s += "1(";
	} else {
		ostringstream osn;
		osn << m.size();
		s += osn.str();
		s += "(";
		for(multiset<fact>::iterator i=m.begin();i!=m.end();++i) {
			ostringstream os;
			os << i->val;
			s += os.str();
			s += ", ";
		}
	}

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
	if (s.find("I1") != string::npos)
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
#if 0 /* disabled, the resuling code is slower also with a destination in conventional memory */
		/* use common subexpression, note that the destination is also read and not only written */
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
/* if enabled it generates more complex test cases */
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

/* adjust the sum */
template<unsigned N> void adjust(unsigned D[N][10]) {
	for(unsigned k=0;k<N;++k) {
		unsigned tot;
		unsigned max;
		unsigned min;
		unsigned count;
		unsigned index;

		do {
			count = 0;
			tot = 0;
			max = D[k][1];
			index = 1;
			min = 100000;
			for(unsigned i=1;i<=9;++i) {
				tot += D[k][i];
				if (D[k][i] > max) {
					index = i;
					max = D[k][i];
				}
				if (D[k][i] < min && D[k][i] != 0) {
					min = D[k][i];
				}
				if (D[k][i] != 0)
					++count;
			}

			if (min == max)
				break;
			if (count <= 3)
				break;

			for(unsigned i=1;i<=9;++i) {
				if (D[k][i] == min)
					D[k][i] = 0;
			}

		} while (1);

		unsigned ntot = 0;
		for(unsigned i=1;i<=9;++i) {
			D[k][i] = (D[k][i] * 16 + (tot / 2 -1)) / tot;
			ntot += D[k][i];
		}

		D[k][index] += 16 - ntot;
	}
}

/* simplify with equal pixel */
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

/* use only the most significative pixel */
void discrete(unsigned p[10], unsigned maskfull) {
	simplify(p, maskfull);

	unsigned max = 0;
	unsigned total = 0;

	for(unsigned i=0;i<9;++i) {
		if (p[i+1] > max)
			max = p[i+1];
		total += p[i+1];
	}

	unsigned count = 0;

	for(unsigned i=0;i<9;++i) {
		if (p[i+1] != max)
			p[i+1] = 0;
		else
			++count;
	}

	for(unsigned i=0;i<9;++i) {
		if (p[i+1])
			p[i+1] += (total - count*max) / count;
	}
}

