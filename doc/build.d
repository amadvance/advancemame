Name
	build - How To Build

	This file contains the instructions to build AdvanceMAME, AdvanceMESS
	and AdvanceMENU from the source archives.

Preparing The Sources
	To build one of the Advance emulators you need first to download and
	unzip the emulator source with the same version of the Advance source.
	Please note that you must use the original emulator source, you cannot
	use the source of another MAME clone like xmame.

	Note that you need only the source from the MAME and MESS archive.
	The original `makefile' and the other files must be manually deleted.

	In Linux and Mac OS X remember to unzip the original MAME and
	MESS .zip archives with the `unzip -aa' command to convert the
	files from the DOS/Windows CR/LF format to the Unix CR format.

    AdvanceMENU
	To compile AdvanceMENU you don't need to prepare the source, they are
	already complete in the AdvanceMENU archive.

    AdvanceMAME
	To compile AdvanceMAME you need the MAME source of the same
	version of AdvanceMAME.

	Copy the `src/' directory in the MAME source archive at the same level
	of the `advance/' directory present in the AdvanceMAME archive.
	Please note that only the `src/' directory from the
	MAME source is required, all the other files must be deleted.

	The final directory tree for AdvanceMAME must be :

		:advance/advance.mak (from AdvanceMAME)
		:src/mame.mak (from MAME)

	After unpacked, you need to patch the original MAME source in
	the `src/' directory with the patch `advance/advmame.dif'.
	If the patch isn't applied correctly, probably you are using the wrong
	version of the emulator source.

	The commands for patching the source for AdvanceMAME in DOS and Windows are :

		:cd src
		:patch -p1 < ..\advance\advmame.dif

	and in Linux and Mac OS X are :

		:cd src
		:patch -p1 < ../advance/advmame.dif

    AdvanceMESS
	To compile AdvanceMESS you need the MAME and MESS source of
	the same version of AdvanceMESS.

	The source of the MESS emulator must be unzipped in the `srcmess/'
	directory over a clean copy of the MAME source of the same version.
	This means that the MESS source must overwrite the MAME source with
	the same name. Please note that the original source directory in the
	MAME and MESS archive is named `src/'. You must rename it `srcmess/'.
	You need also the additional `mess/' source directory.

	The final directory tree for AdvanceMESS must be :

		:advance/advance.mak (from AdvanceMESS)
		:srcmess/mame.mak (from MAME and MESS)
		:mess/mess.mak (from MESS)

	After unpacked, you need to patch the original MESS source in
	in the `srcmess/' directory with the patch `advance/advmess.dif' and
	the source in the `mess/' directory with the patch `advance/mess.dif'.
	If the patches aren't applied correctly, probably you are using the wrong
	version of the emulator source.

	The commands for patching the source for AdvanceMESS in DOS and Windows are :

		:cd srcmess
		:patch -p1 < ..\advance\advmess.dif
		:cd ..\mess
		:patch -p1 < ..\advance\mess.dif

	and in Linux and Mac OS X are :

		:cd srcmess
		:patch -p1 < ../advance/advmess.dif
		:cd ../mess
		:patch -p1 < ../advance/mess.dif

Configuring
    Linux/Mac OS X
	Run the `./configure' script.

	Generally no extra option is required. You can get the complete
	option list with the `./configure --help' command.

	The configure script automatically detects all the available libraries
	and the optimization flags. You can use the --with-sdl-prefix option
	to search for the SDL library in specific locations.

	If you want to customize the compilation CFLAGS you can put them on the
	./configure command line, for example:

		:./configure CFLAGS="-O2 -march=pentium4 -fomit-frame-pointer" LDFLAGS="-s"

	The configure script automatically detects the emulator to compile
	checking the installed sources. You can force a specific emulator
	with the `--with-emu' option.

    DOS/Windows
	In DOS/Windows you need to manually rename the `Makefile.in' file
	as `Makefile' and edit the first section to match your requirements.

Compiling
	To compile run `make'.

Installing
    Linux/Mac OS X
	Run `make install' to install the binaries and the documentation.
	The binaries are installed in $prefix/bin, the program data in
	$prefix/share/advance, the documentation in $prefix/share/advance/doc,
	and the man pages in $prefix/man/man1.

	The default installation $prefix is /usr/local.

	In Mac OS X please verify that the directory $prefix/bin is in the
	search PATH. Generally /usr/local/bin it isn't.

    DOS/Windows
	Copy manually the compiled executables in a directory of your choice.

Targets
	These are the defined targets in the `Makefile' :
		emu - Compile the emulator.
		cfg -  Compile `advcfg'.
		v - Compile `advv'.
		line - Compile `advline'.
		k - Compile `advk'.
		j - Compile `advj'.
		m - Compile `advm'.
		s - Compile `advs'.
		menu - Compile `advmenu'.
		all (or empty) - Compile all.
		clear - Clean all.
		install - Install all [must be root].

Requirements
    Linux
	To build in Linux you need the following software:
		:Linux 2.4.0 (or newer)
		:GNU gcc C/C++ 2.95.3 or 3.2.3 or 3.3
		:GNU make 3.79.1 (or newer)
		:NASM 0.98.33 (or newer)
		:zlib 1.1.4 (or newer)
		:SVGALIB 1.9.14 (or newer)
		:LibSDL 1.2.4 (or newer)
		:S-Lang 1.4.3 (or newer)

	The suggested gcc compiler versions are 2.95.3, 3.2.3 and 3.3.
	The versions 2.96.x, 3.0, 3.0.1 and 3.0.2 don't work.
	Other versions should work.

	The SVGALIB 1.4.x versions are NOT supported.

	Download the latest ALPHA 1.9.x or 2.0.x version from
	http://www.svgalib.org. In the contrib/svgalib directory there
	are some source patches to fix some problems of the library.
	Use the noirq.diff patch if you detect random freeze only with
	vsync activated.

	Remember to edit the /etc/vga/libvga.conf file with your settings.
	Specifically you need at least to set correctly `HorizSync' and
	`VertRefresh'.

	If your distribution doesn't contain the S-Lang library you
	can download it from http://www.s-lang.org/.

    Mac OS X
	To build in Mac OS X you need the following software:
		:Mac OS X
		:GNU gcc C/C++ 2.95.3 (or newer)
		:LibSDL 1.2.4 (or newer)

	The gcc compiler is included in the Apple Development Kit which
	must be installed manually from the original Mac OS X cd.

	The SDL library must be manually compiled and installed.
	Please note that you may need to use the --with-sdl-prefix option
	of the emulator ./configure to correctly find the installed SDL
	library. Generally "./configure --with-sdl-prefix=/usr/local" is
	enough.

    DOS
	To build in DOS you need the following software:
		:DJGPP development kit 2.03 (or never) [djdev*.zip]
		:DJGPP GNU binutils [bnu*b.zip]
		:DJGPP GNU gcc C/C++ 2.95.3 or 3.2.3 [gcc*b.zip gpp*b.zip]
		:DJGPP GNU make 3.79.1 (or newer) [mak*b.zip]
		:DJGPP GNU fileutils [fil*b.zip]
		:DJGPP GNU shellutils [shl*b.zip]
		:DJGPP GNU patch [pat*b.zip]
		:NASM 0.98.33 (or newer)
		:zlib 1.1.4 (or newer)
		:SEAL 1.0.7 + MAME patch
		:Allegro 4.0.0 (or newer)

	The suggested gcc compiler versions are 2.95.3 and 3.2.3.
	The versions 3.0, 3.0.1 and 3.0.2 don't work.
	The versions 3.1, 3.1.1, 3.2, 3.2.1, 3.2.2 have some minor
	problems (a few games may not work correctly).
	Other versions should work.

	The patched SEAL library is available at http://www.mame.net

	Ensure to have the DOS version of NASM. If you have the Windows
	version named `nasmw.exe' you must rename it as `nasm.exe' or
	change the `Makefile' to use it.

    Windows
	To build in Windows you need the following software:
		:MINGW 1.1 (or newer)
		:MINGW GNU gcc C/C++ 2.95.3 (or never)
		:NASM 0.98.33 (or newer)
		:zlib 1.1.4 (or newer)
		:LibSDL 1.2.4 (or newer)

	The only tested compiler version is 2.95.3.
	Other versions should work.

Copyright
	This file is Copyright (C) 2003 Andrea Mazzoleni.

