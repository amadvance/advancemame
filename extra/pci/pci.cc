#include <set>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

using namespace std;

typedef set<string> string_set;

// --------------------------------------------------------------------------
// device

class entry_subdevice {
public:
	unsigned sub;
	string desc;

	entry_subdevice(unsigned Adevice, const string& Adesc);

	bool operator<(const entry_subdevice&) const;
};

entry_subdevice::entry_subdevice(unsigned Asub, const string& Adesc)
	: sub(Asub), desc(Adesc)
{
}

bool entry_subdevice::operator<(const entry_subdevice& B) const {
	if (sub < B.sub) return true;
	if (sub > B.sub) return false;
	return false;
}

class entry_device {
public:
	unsigned device;

	mutable string desc;
	mutable string comment;
	mutable set<entry_subdevice> ss;

	entry_device(unsigned Adevice, const string& Adesc, const string& Acomment);

	bool operator<(const entry_device&) const;
};

entry_device::entry_device(unsigned Adevice, const string& Adesc, const string& Acomment)
	: device(Adevice), desc(Adesc), comment(Acomment)
{
}

bool entry_device::operator<(const entry_device& B) const {
	if (device < B.device) return true;
	if (device > B.device) return false;
	return false;
}

typedef set<entry_device> entry_device_set;

struct entry_device_by_name : std::binary_function<entry_device,entry_device,bool> {
	bool operator()(const entry_device& A, const entry_device& B) const {
		return A.desc < B.desc;
	}
};

// --------------------------------------------------------------------------
// device_group

typedef set<entry_device,entry_device_by_name> entry_device_set_by_name;

#define EXCLUDE_MAX 16

class entry_device_group {
public:
	unsigned device;
	unsigned device_mask;
	unsigned exclude[EXCLUDE_MAX];

	mutable string comment;
	mutable entry_device_set ds;

	entry_device_group(unsigned Adevice, unsigned Adevice_mask, const string& Acomment, unsigned* Aexclude);

	bool operator<(const entry_device_group& B) const;

	bool has(unsigned Adevice) const;
};

entry_device_group::entry_device_group(unsigned Adevice, unsigned Adevice_mask, const string& Acomment, unsigned* Aexclude)
	: device(Adevice), device_mask(Adevice_mask), comment(Acomment)
{
	if (Aexclude)
		memcpy(exclude, Aexclude, sizeof(exclude));
	else
		memset(exclude, 0, sizeof(exclude));
}

bool entry_device_group::operator<(const entry_device_group& B) const {
	unsigned Adevice = device;
	unsigned Bdevice = B.device;
	if (Adevice < Bdevice) return true;
	if (Adevice > Bdevice) return false;
	return false;
}

bool entry_device_group::has(unsigned Adevice) const {
	unsigned i;

	if ((Adevice & device_mask) != device)
		return false;

	for(i=0;i<EXCLUDE_MAX;++i)
		if (Adevice == exclude[i])
			return false;

	return true;
}

// --------------------------------------------------------------------------
// vendor

typedef set<entry_device_group> entry_device_group_set;

class entry_vendor {
public:
	unsigned vendor;

	mutable string desc;

	entry_vendor(const entry_vendor& A);
	entry_vendor(unsigned Avendor);
	entry_vendor(unsigned Avendor, const string& Adesc);

	mutable entry_device_group_set gs;

	entry_device_group_set::iterator has(unsigned device) const;

	bool operator<(const entry_vendor& B) const;
};

entry_device_group_set::iterator entry_vendor::has(unsigned device) const
{
	for(entry_device_group_set::const_iterator i=gs.begin();i!=gs.end();++i)
		if (i->has(device))
			return i;
	return gs.end();
}

entry_vendor::entry_vendor(const entry_vendor& A) :
	vendor(A.vendor), desc(A.desc), gs(A.gs) {
}

entry_vendor::entry_vendor(unsigned Avendor)
	: vendor(Avendor)
{
}

entry_vendor::entry_vendor(unsigned Avendor, const string& Adesc)
	: vendor(Avendor), desc(Adesc)
{
}

bool entry_vendor::operator<(const entry_vendor& B) const {
	if (vendor < B.vendor) return true;
	if (vendor > B.vendor) return false;
	return false;
}

// --------------------------------------------------------------------------
// vendor_set

typedef set<entry_vendor> entry_vendor_set;

class entry_vendor_group {
public:
	void insert(unsigned vendor, unsigned device, unsigned mask, const string& comment, unsigned* exclude);

	entry_vendor_set vg;
};

void entry_vendor_group::insert(unsigned vendor, unsigned device, unsigned mask, const string& comment, unsigned* exclude)
{
	entry_vendor_set::iterator i;
	i = vg.insert(entry_vendor(vendor)).first;
	entry_device_group d(device, mask, comment, exclude);
	if (!i->gs.insert(d).second) {
		cerr << "warning: duplicate id " << hex << vendor << ":" << hex << device << endl;
	}
}

// --------------------------------------------------------------------------

string token_get(const string& s, unsigned& ptr, const char* sep) {
	unsigned start = ptr;
	while (ptr < s.length() && strchr(sep,s[ptr])==0)
		++ptr;
	return string(s,start,ptr-start);
}

void token_skip(const string& s, unsigned& ptr, const char* sep) {
	while (ptr < s.length() && strchr(sep,s[ptr])!=0)
		++ptr;
}

string strip(const string& s) {
	string r = s;
	while (r.length() && isspace(r[0]))
		r.erase(0,1);
	while (r.length() && isspace(r[r.length()-1]))
		r.erase(r.length()-1,1);
	return r;
}

string hex(unsigned v) {
	ostringstream o;
	o << hex << setw(4) << setfill('0') << v;
	return o.str();
}

void process_vendor_pcidevs(istream& is, entry_vendor_set& vs) {
	while (!is.eof()) {
		string s;
		unsigned p = 0;
		getline(is,s,'\n');

		string c;
		string v;
		string d;

		c = token_get(s,p," \t");
		token_skip(s,p," \t");
		v = token_get(s,p," \t");
		token_skip(s,p," \t");
		d = strip(token_get(s,p,""));

		if (c == "V") {
			unsigned vendor = strtol(v.c_str(),0,16);
			vs.insert(entry_vendor(vendor,d));
		}
	}
}


void process_pcidevs(istream& is, entry_vendor_set& vs, const entry_vendor_set& vns) {
	unsigned vendor = 0;
	unsigned device = 0;
	string oem;

	while (!is.eof()) {
		string s;
		unsigned p = 0;
		getline(is,s,'\n');

		string c;
		string v;
		string d;

		c = token_get(s,p," \t");
		token_skip(s,p," \t");
		v = token_get(s,p," \t");
		token_skip(s,p," \t");
		d = strip(token_get(s,p,""));

		if (c == "V") {
			vendor = strtol(v.c_str(),0,16);
			oem = "";
			device = 0;
			entry_vendor_set::iterator i = vs.find(entry_vendor(vendor));
			if (i != vs.end()) {
				i->desc = d + " (" + hex(vendor) + ")";
			}
		} else if (c == "O") {
			unsigned sub = strtol(v.c_str(),0,16);
			if (sub == vendor) {
			  oem = "";
			} else {
				entry_vendor_set::const_iterator i = vns.find(entry_vendor(sub));
				if (i != vns.end()) {
					oem = "OEM " + i->desc + " {" + hex(sub) + "} : ";
				} else {
					oem = "OEM {" + hex(sub) + "} : ";
				}
			}
		} else if (c == "D") {
			device = strtol(v.c_str(),0,16);
			entry_vendor_set::iterator i = vs.find(entry_vendor(vendor));
			if (i != vs.end()) {
				entry_device_group_set::iterator j = i->has(device);
				if (j != i->gs.end()) {
					j->ds.insert(entry_device(device,d + " [" + hex(device) + "]",j->comment));
				}
			}
		} else if (c == "S") {
			unsigned subdevice = strtol(v.c_str(),0,16);
			entry_vendor_set::iterator i = vs.find(entry_vendor(vendor));
			if (i != vs.end()) {
				entry_device_group_set::iterator j = i->has(device);
				if (j != i->gs.end()) {
					entry_device_set::iterator k = j->ds.find(entry_device(device,"",""));
					if (k != j->ds.end()) {
						k->ss.insert(entry_subdevice(subdevice, oem+d));
					}
				}
			}
		}
	}
}

void process_vendor_pciids(istream& is, entry_vendor_set& vs) {
	while (!is.eof()) {
		string s;
		unsigned p = 0;
		getline(is,s,'\n');

		if (s.length() == 0 || s[0] == '#')
			continue;

		unsigned t = 0;

		while (t < s.length() && s[t]=='\t')
			++t;

		token_skip(s,p," \t");
		string v = token_get(s,p," \t");
		token_skip(s,p," \t");

		if (t == 0) {
			string d = strip(token_get(s,p,""));
			unsigned vendor = strtol(v.c_str(),0,16);
			vs.insert(entry_vendor(vendor,d));
		}
	}
}

void process_pciids(istream& is, entry_vendor_set& vs, const entry_vendor_set& vns) {
	unsigned vendor = 0;
	unsigned device = 0;
	string oem;

	while (!is.eof()) {
		string s;
		unsigned p = 0;
		getline(is,s,'\n');

		if (s.length() == 0 || s[0] == '#')
			continue;

		unsigned t = 0;

		while (t < s.length() && s[t]=='\t')
			++t;

		token_skip(s,p," \t");
		string v = token_get(s,p," \t");
		token_skip(s,p," \t");

		if (t == 0) {
			string d = strip(token_get(s,p,""));
			vendor = strtol(v.c_str(),0,16);
			oem = "";
			device = 0;
			entry_vendor_set::iterator i = vs.find(entry_vendor(vendor));
			if (i != vs.end()) {
				i->desc = d + " (" + hex(vendor) + ")";
			}
		} else if (t == 1) {
			string d = strip(token_get(s,p,""));
			device = strtol(v.c_str(),0,16);
			entry_vendor_set::iterator i = vs.find(entry_vendor(vendor));
			if (i != vs.end()) {
				entry_device_group_set::iterator j = i->has(device);
				if (j != i->gs.end()) {
					j->ds.insert(entry_device(device,d + " [" + hex(device) + "]",j->comment));
				}
			}
		} else if (t == 2) {
			string o = token_get(s,p," \t");
			token_skip(s,p," \t");
			string d = strip(token_get(s,p,""));
			unsigned sub = strtol(v.c_str(),0,16);
			unsigned subdevice = strtol(o.c_str(),0,16);
			if (sub == vendor) {
				oem = "";
			} else {
				entry_vendor_set::const_iterator i = vns.find(entry_vendor(sub));
				if (i != vns.end()) {
					oem = "OEM " + i->desc + " {" + hex(sub) + "} : ";
				} else {
					oem = "OEM {" + hex(sub) + "} : ";
				}
			}
			entry_vendor_set::iterator i = vs.find(entry_vendor(vendor));
			if (i != vs.end()) {
				entry_device_group_set::iterator j = i->has(device);
				if (j != i->gs.end()) {
					entry_device_set::iterator k = j->ds.find(entry_device(device,"",""));
					if (k != j->ds.end()) {
						k->ss.insert(entry_subdevice(subdevice, oem+d));
					}
				}
			}
		}
	}
}

void print_set(ostream& os, entry_vendor_set& vs) {
	for(entry_vendor_set::iterator i=vs.begin();i!=vs.end();++i) {
		os << "  " << i->desc << endl;

		entry_device_set_by_name ds;

		for(entry_device_group_set::iterator j=i->gs.begin();j!=i->gs.end();++j) {
			for(entry_device_set::iterator k=j->ds.begin();k!=j->ds.end();++k) {
				ds.insert(*k);
			}
		}

		for(entry_device_set::iterator k=ds.begin();k!=ds.end();++k) {
			os << "\t+" << k->desc << " " << k->comment << endl;

			// not oem
			set<string> ss1;
			for(set<entry_subdevice>::iterator w=k->ss.begin();w!=k->ss.end();++w) {
				if (w->desc.find('{') == string::npos)
					ss1.insert(w->desc);
			}

			// oem
			set<string> ss2;
			for(set<entry_subdevice>::iterator w=k->ss.begin();w!=k->ss.end();++w) {
				if (w->desc.find('{') != string::npos)
					ss2.insert(w->desc);
			}

			for(set<string>::iterator w=ss1.begin();w!=ss1.end();++w)
				os << "\t\t+" << *w << " " << k->comment << endl;
			for(set<string>::iterator w=ss2.begin();w!=ss2.end();++w)
				os << "\t\t+" << *w << " " << k->comment << endl;
		}

		os << endl;
	}
}

void print_id(ostream& os, entry_vendor_set& vs) {
	for(entry_vendor_set::iterator i=vs.begin();i!=vs.end();++i) {
		for(entry_device_group_set::iterator j=i->gs.begin();j!=i->gs.end();++j) {
			for(entry_device_set::iterator k=j->ds.begin();k!=j->ds.end();++k) {
				os << hex(i->vendor) << ":" << hex(k->device) << endl;
			}
		}
	}
}

#define HEADER \
"\tPlease note that not all the OEM cards are listed. If your card has a\n" \
"\tsupported chipset, it's probably supported also if the OEM vendor is not\n" \
"\tin the list.\n" \
"\n" \
"\tPlease note that some laptop video cards work only when connected to\n" \
"\tan external monitor, and not with the LCD screen.\n" \
"\n" \
"SubIndex\n" \
"\n"

#define FOOTER \
"Information\n" \
"\tThis list is automatically generated from the `pcidevs.txt' and `pci.ids'\n" \
"\tfiles available at:\n" \
"\n" \
"\t\t+http://members.datafast.net.au/dft0802/downloads/pcidevs.txt\n" \
"\t\t+http://pciids.sf.net/\n" \
"\n" \
"\tThe numbers in [] are the PCI device IDs of the video board\n" \
"\tmodels. The numbers in () are the PCI vendor IDs of the\n" \
"\tvideo board manufacturers. The idented names are submodels with\n" \
"\tthe same chipset. The numbers in {} are the PCI OEM vendor IDs of\n" \
"\tthe submodel.\n" \
"\n"

#define FOOTER_AUDIO \
FOOTER

#define FOOTER_VIDEO \
FOOTER \
"\tThe cards marked with (*) don't support interlaced modes.\n" \
"\n"

void print_dos(ostream& os, entry_vendor_set& vs_svgaline, entry_vendor_set& vs_vbeline) {
	os <<
"Name\n"
"\tcarddos - Supported DOS Video Cards\n"
"\n"
"\tThis is the list of the DOS video cards supported\n"
"\tby the Advance programs with the DOS drivers. \n"
"\n"
HEADER
;

	os <<
"SVGALINE Drivers\n"
"\tThe `svgaline' drivers are a subset of the Linux SVGALIB library.\n"
"\n"
;

	print_set(os,vs_svgaline);

	os <<
"VBELINE Drivers\n"
"\tThese are the OLD DOS drivers based on the VBE BIOS of the\n"
"\tvideo board. Some of these drivers are derived from the\n"
"\tVSyncMAME video drivers written by Saka and from the ATI driver\n"
"\tof MAMEDOS.\n"
"\tSome of them require the presence of an updated VBE BIOS like\n"
"\tthe Scitech Display Doctor.\n"
"\n"
;

	print_set(os,vs_vbeline);

	os << FOOTER_VIDEO;
}

void print_linux(ostream& os, entry_vendor_set& vs_svgalib, entry_vendor_set& vs_fb) {
	os <<
"Name\n"
"\tcardlinux - Supported Linux Video Cards\n"
"\n"
"\tThis is the list of the Linux video cards supported\n"
"\tby the Advance programs with the Linux drivers. \n"
"\n"
HEADER
;

	os <<
"Frame Buffer Drivers\n"
"\tThe `fb' driver uses the Linux Kernel Frame Buffer Drivers.\n"
"\tIt supports all the video board supported by your Linux\n"
"\tKernel which are able to create new video modes.\n"
"\tThis exclude the `vesafb' driver.\n"
"\n"
"\tThe following is the list of the drivers available on the\n"
"\tLinux Kernel 2.4.26. More recent kernels may have more\n"
"\trecent models supported.\n"
"\n"
;

	print_set(os,vs_fb);

	os <<
"SVGALIB Drivers\n"
"\tThe `svgalib' driver uses the Linux SVGALIB library.\n"
"\tIt supports all the video boards supported by the library.\n"
"\n"
"\tThe following is the list of the drivers available on the\n"
"\tSVGALIB 1.9.19 with the AdvanceMAME patches applied.\n"
"\n"
;

	print_set(os,vs_svgalib);

	os << FOOTER_VIDEO;
}

void print_cd(ostream& os, entry_vendor_set& vs_fb) {
	os <<
"Name\n"
"\tcardcd - Supported AdvanceCD Video Cards\n"
"\n"
"\tThis is the list of the video cards supported by AdvanceCD.\n"
"\n"
HEADER
;

	os <<
"Frame Buffer Drivers\n"
"\tThis is the list of all the video cards supported by the Linux\n"
"\tKernel Frame Buffer drivers used in AdvanceCD.\n"
"\n"
;

	print_set(os,vs_fb);

	os << FOOTER_VIDEO;
}

void print_audiocd(ostream& os, entry_vendor_set& vs_alsa) {
	os <<
"Name\n"
"\taudiocd - Supported AdvanceCD Sound Cards\n"
"\n"
"\tThis is the list of the sound cards supported by AdvanceCD.\n"
"\n"
HEADER
;

	os <<
"ALSA Drivers\n"
"\tThis is the list of all the PCI sound cards supported by the Linux\n"
"\tKernel ALSA drivers used in AdvanceCD.\n"
"\tOnly the PCI cards are listed, but also some ISA PnP and USB cards\n"
"\tare supported. You can check the official ALSA documentation for a\n"
"\tmore complete card list at http://www.alsa-project.org/alsa-doc/.\n"
"\n"
;

	print_set(os,vs_alsa);

	os << FOOTER_AUDIO;
}

void print_win(ostream& os, entry_vendor_set& vs_svgaline) {
	os <<
"Name\n"
"\tcardwin - Supported Windows Video Cards\n"
"\n"
"\tThis is the list of the Windows video cards supported\n"
"\tby the Advance programs with the Windows drivers.\n"
"\n"
HEADER
;

	os <<
"SVGAWIN Drivers\n"
"\tThe `svgawin' driver is a subset of the Linux SVGALIB library.\n"
"\n"
"\tThis driver is experimental. At present it's only tested on Windows\n"
"\t2000 with a GeForce 2 board. It's supposed to work with GeForge boards,\n"
"\tit may not work with all the other boards.\n"
"\n"
;

	print_set(os,vs_svgaline);

	os << FOOTER_VIDEO;
}

// --------------------------------------------------------------------------

struct pci_id {
	unsigned vendor;
	unsigned device;
	const char* comment;
};

struct pci_id_mask {
	unsigned vendor;
	unsigned device;
	unsigned mask;
	const char* comment;
	unsigned exclude[EXCLUDE_MAX];
};

struct pci_id_mask ID_VBELINE[] = {
#include "vbeid.h"
{ 0 }
};

struct pci_id ID_FB[] = {
#include "fbid.h"
{ 0 }
};

struct pci_id ID_FBPATCH[] = {
#define USE_FB_PATCH
#include "fbid.h"
#undef USE_FB_PATCH
{ 0 }
};

struct pci_id_mask ID_SVGALIB[] = {
#include "vgaid.h"
{ 0 }
};

struct pci_id ID_ALSA[] = {
#include "alsaid.h"
{ 0 }
};

void insert(struct pci_id_mask* pci, entry_vendor_group& device)
{
	while (pci->vendor) {
		device.insert(pci->vendor, pci->device, pci->mask, pci->comment, pci->exclude);
		++pci;
	}
}

void insert(struct pci_id* pci, entry_vendor_group& device)
{
	while (pci->vendor) {
		device.insert(pci->vendor, pci->device, 0xffff, pci->comment, 0);
		++pci;
	}
}

// --------------------------------------------------------------------------

void load_vendor(entry_vendor_set& vendor)
{
	ifstream fi3("pciextra.txt");
	if (!fi3) {
		cerr << "Error opening pciextra.txt" << endl;
		exit(EXIT_FAILURE);
	}
	process_vendor_pcidevs(fi3,vendor);
	fi3.close();

	ifstream fi1("pcidevs.txt");
	if (!fi1) {
		cerr << "Error opening pcidevs.txt" << endl;
		exit(EXIT_FAILURE);
	}
	process_vendor_pcidevs(fi1,vendor);
	fi1.close();

	ifstream fi2("pci.ids");
	if (!fi2) {
		cerr << "Error opening pci.ids" << endl;
		exit(EXIT_FAILURE);
	}
	process_vendor_pciids(fi2,vendor);
	fi2.close();
}

void load_device(entry_vendor_set& device, const entry_vendor_set& vendor)
{
	ifstream fi1("pcidevs.txt");
	if (!fi1) {
		cerr << "Error opening pcidevs.txt" << endl;
		exit(EXIT_FAILURE);
	}
	process_pcidevs(fi1,device,vendor);
	fi1.close();

	ifstream fi2("pci.ids");
	if (!fi2) {
		cerr << "Error opening pci.ids" << endl;
		exit(EXIT_FAILURE);
	}
	process_pciids(fi2,device,vendor);
	fi2.close();

	ifstream fi3("pciextra.txt");
	if (!fi3) {
		cerr << "Error opening pciextra.txt" << endl;
		exit(EXIT_FAILURE);
	}
	process_pcidevs(fi3,device,vendor);
	fi3.close();
}

int main() {
	entry_vendor_group vs_vendor;
	entry_vendor_group vs_svgalib;
	entry_vendor_group vs_vbeline;
	entry_vendor_group vs_fb;
	entry_vendor_group vs_fbpatch;
	entry_vendor_group vs_alsa;

	load_vendor(vs_vendor.vg);

	insert(ID_VBELINE, vs_vbeline);
	insert(ID_SVGALIB, vs_svgalib);
	insert(ID_FB, vs_fb);
	insert(ID_FBPATCH, vs_fbpatch);
	insert(ID_ALSA, vs_alsa);

	load_device(vs_fbpatch.vg, vs_vendor.vg);
	load_device(vs_vbeline.vg, vs_vendor.vg);
	load_device(vs_svgalib.vg, vs_vendor.vg);
	load_device(vs_fb.vg, vs_vendor.vg);
	load_device(vs_alsa.vg, vs_vendor.vg);

	{
		ofstream fo;
		fo.open("carddos.d");
		print_dos(fo, vs_svgalib.vg, vs_vbeline.vg);
		fo.close();
	}

	{
		ofstream fo;
		fo.open("cardwin.d");
		print_win(fo, vs_svgalib.vg);
		fo.close();
	}

	{
		ofstream fo;
		fo.open("cardlinx.d");
		print_linux(fo, vs_svgalib.vg, vs_fb.vg);
		fo.close();
	}

	{
		ofstream fo;
		fo.open("cardcd.d");
		print_cd(fo, vs_fbpatch.vg);
		fo.close();
	}

	{
		ofstream fo;
		fo.open("cardcd.lst");
		print_id(fo, vs_fbpatch.vg);
		fo.close();
	}

	{
		ofstream fo;
		fo.open("audiocd.d");
		print_audiocd(fo, vs_alsa.vg);
		fo.close();
	}

	return 0;
}
