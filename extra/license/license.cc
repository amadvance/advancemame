#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

const char* license =
"/*\n"
" * This file is part of the Advance project.\n"
" *\n"
" * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Andrea Mazzoleni\n"
" *\n"
" * This program is free software; you can redistribute it and/or modify\n"
" * it under the terms of the GNU General Public License as published by\n"
" * the Free Software Foundation; either version 2 of the License, or\n"
" * (at your option) any later version.\n"
" *\n"
" * This program is distributed in the hope that it will be useful,\n"
" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
" * GNU General Public License for more details.\n"
" *\n"
" * You should have received a copy of the GNU General Public License\n"
" * along with this program; if not, write to the Free Software\n"
" * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n"
" */"; /* No final \n */

const char* license_exception =
"/*\n"
" * This file is part of the Advance project.\n"
" *\n"
" * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Andrea Mazzoleni\n"
" *\n"
" * This program is free software; you can redistribute it and/or modify\n"
" * it under the terms of the GNU General Public License as published by\n"
" * the Free Software Foundation; either version 2 of the License, or\n"
" * (at your option) any later version.\n"
" *\n"
" * This program is distributed in the hope that it will be useful,\n"
" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
" * GNU General Public License for more details.\n"
" *\n"
" * You should have received a copy of the GNU General Public License\n"
" * along with this program; if not, write to the Free Software\n"
" * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n"
" *\n"
" * In addition, as a special exception, Andrea Mazzoleni\n"
" * gives permission to link the code of this program with\n"
" * the MAME library (or with modified versions of MAME that use the\n"
" * same license as MAME), and distribute linked combinations including\n"
" * the two.  You must obey the GNU General Public License in all\n"
" * respects for all of the code used other than MAME.  If you modify\n"
" * this file, you may extend this exception to your version of the\n"
" * file, but you are not obligated to do so.  If you do not wish to\n"
" * do so, delete this exception statement from your version.\n"
" */"; /* No final \n */



bool process(const char* path, const char* lic) {
	ifstream fi(path);
	string s;
	getline(fi,s,(char)EOF);
	fi.close();

	if (s.substr(0,2)!="/*")
		return false;

	unsigned end = s.find("*/");
	if (end == string::npos)
		return false;

	s.erase(0,end + 2);

	s.insert(0,lic);

	ofstream fo(path);
	fo << s;
	fo.close();

	return true;
}

int main(int argc, char* argv[]) {
	if (argc<=1) {
		cerr << "Syntax: license [-l | -e] files..." << endl;
		exit(EXIT_FAILURE);
	}

	const char* lic = license;
	for(int i=1;i<argc;++i) {
		if (strcmp(argv[i],"-e")==0)
			lic = license_exception;
		else if (strcmp(argv[i],"-l")==0)
			lic = license;
		else if (!process(argv[i],lic)) {
			cerr << "ignore " << argv[i] << endl;
		}
	}

	return EXIT_SUCCESS;
}
