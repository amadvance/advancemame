Name
	build - How To Build

	This file contains the instructions to build AdvanceMAME, AdvanceMESS,
	AdvancePAC and AdvanceMENU from the source archives.

Preparing The Sources
	To compile AdvanceMENU you don't need to prepare the source, they are
	already complete.

	To build one of the Advance emulators (MAME, MESS, PacMAME) you
	need first to download and unzip the emulator source with the same
	version of the Advance source. Please note that you must use the
	original emulator source, you cannot use the source of another MAME
	clone like xmame.

	The source of the MAME emulator must be unzipped in the `src/' directory,
	the MESS source in `srcmess/' and the PacMAME source in `srcpac/'.
	For MESS you need also the additional `mess/' source directory.

	In Linux remember to unzip the original .zip archives with the
	`unzip -aa' command to convert the files from the DOS/Windows CR/LF
	format to the Unix CR format.

	The original `makefile' of the emulator must be manually deleted.

	For example the final directory tree for AdvanceMAME must be :

		:./configure
		:./advance/advance.mak
		:./src/mame.mak

	After unpacked, you need to patch the original MAME source with the
	patch `advance/advmame.dif', the MESS source with `advance/advmess.dif' and
	the PacMAME source with `advance/advpac.dif'. If the patch isn't applied
	correctly, probably you are using the wrong version of the emulator source.

	For example the command for patching the source for AdvanceMAME in DOS is :

		:cd src
		:patch -p1 < ..\advance\advmame.dif

	and in Linux is :

		:cd src
		:patch -p1 < ../advance/advmame.dif

Configuring
    Linux
	Run the `./configure' script.

	Generally no extra option is required. You can get the complete
	option list with the `./configure --help' command.

	The configure script automatically detects all the available libraries
	and the optimization flags.

	If you want to customize the compilation CFLAGS you can set them before
	calling the ./configure script, for example:

		:export CFLAGS="-O2 -march=pentium4 -fomit-frame-pointer"
		:export LDFLAGS="-s"
		:./configure

	The configure script automatically detects the emulator to compile
	checking the installed sources. You can force a specific emulator
	with the `--with-emu' option.

    DOS/Windows
	In DOS/Windows you need to manually rename the `Makefile.in' file
	as `Makefile' and edit the first section to match your requirements.

Compiling
	To compile run `make'.

Installing
    Linux
	Run `make install' to install the binaries and the documentation.
	The binaries are installed in $prefix/bin, the documentation
	in $prefix/doc/advance, the program data in $prefix/share/advance and
	the man pages in $prefix/man/man1.

	The default installation $prefix is /usr/local.

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
		:GNU gcc C/C++ 2.95.3 or 3.2.2
		:GNU make 3.79.1 (or newer)
		:NASM 0.98.33 (or newer)
		:zlib 1.1.4 (or newer)
		:SVGALIB 1.9.14 (or newer)
		:LibSDL 1.2.4 (or newer)
		:S-Lang 1.4.3 (or newer)

	The suggested gcc compiler versions are 2.95.3 and 3.2.2.
	The versions 2.96.x, 3.0, 3.0.1 and 3.0.2 don't work.
	Other versions should work.

	The SVGALIB 1.4.x versions are NOT supported.

	Download the latest ALPHA 1.9.x version from http://www.svgalib.org.
	In the contrib/svgalib directory there are some source patches to fix
	some problems on the library. Use the noirq.diff patch if you detect
	random freeze only with vsync activated.

	Remember to edit the /etc/vga/libvga.conf file with your settings.
	Specifically you need at least to set correctly `HorizSync' and
	`VertRefresh'.

	If your distribution doesn't contain the S-Lang library you
	can download it from http://www.s-lang.org/.

    DOS
	To build in DOS you need the following software:
		:DJGPP development kit 2.03 (or never) [djdev*.zip]
		:DJGPP GNU binutils [bnu*b.zip]
		:DJGPP GNU gcc C/C++ 2.95.3 or 3.2.2 [gcc*b.zip gpp*b.zip]
		:DJGPP GNU make 3.79.1 (or newer) [mak*b.zip]
		:DJGPP GNU fileutils [fil*b.zip]
		:DJGPP GNU shellutils [shl*b.zip]
		:DJGPP GNU patch [pat*b.zip]
		:NASM 0.98.33 (or newer)
		:zlib 1.1.4 (or newer)
		:SEAL 1.0.7 + MAME patch
		:Allegro 4.0.0 (or newer)

	The suggested gcc compiler versions are 2.95.3 and 3.2.2.
	The versions 3.0, 3.0.1 and 3.0.2 don't work.
	The versions 3.1, 3.1.1, 3.2, 3.2.1 have some minor known
	problems (a few games doesn't work correctly).
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

