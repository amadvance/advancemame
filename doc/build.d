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
	clone like Xmame.

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
	In Linux you need to run the `./configure' script with the proper options.
	You can get a complete option list with the `./configure --help' command.
	Generally, you need only to specify the --with-system option choosing
	the `sdl' or the `native' system library.

	The `native' system uses the svgalib 1.9 and framebuffer graphics libraries
	and it's able to directly access and completly control the graphics output
	of your video board and automatically generate video modes with the correct
	size and frequency.

	The `sdl' system uses the LibSDL graphics library, it can be used to show
	the program in a Window Manager, but it's unable to completly control the
	graphics output. It isn't a good choice for the fullscreen use of the
	emulators, but it works good for the frontend.

	If you want to customize the compilation CFLAGS you can set them before
	calling the ./configure script, for example:

		:export CFLAGS="-O3 -march=pentium3 -fomit-frame-pointer"
		:export LDFLAGS="-s"
		:./configure

    DOS/Windows
	In DOS/Windows you need to manually rename the `Makefile.in' file
	as `Makefile' and edit the first section to match your requirements.

Compiling
	Finally you can run `make' to compile all.

Installing
	In Linux type `make install' to install the binaries and the documentation.
	The binaries are installed in $prefix/bin, the documentation
	in $prefix/doc/advance, the program data in $prefix/share/advance and
	the man pages in $prefix/man/man1.

	The default installation $prefix is /usr/local.

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
	To compile the Advance projects you need the following
	software :
		:GNU gcc 2.95.3/3.0.4/3.1.x/3.2 (with c and c++ support)
		:NASM 0.98 (or newer)
		:zlib 1.1.3 (or newer)
		:UPX 1.20 (or newer)
		:Make 3.79.1 (or newer)

	The gcc compiler versions 2.96.x, 3.0, 3.0.1 and 3.0.2 are
	NOT supported. In DOS there are some problems with the 3.1.x
	compiler, don't use it.

	To build in DOS you need the additional following software:
		:DJGPP development kit 2.03 (djdev*.zip)
		:DJGPP GNU binutils (bnu*b.zip)
		:DJGPP GNU C/C++ compiler (gcc*b.zip gpp*b.zip)
		:DJGPP GNU make (mak*b.zip)
		:DJGPP GNU fileutils (fil*b.zip)
		:DJGPP GNU shellutils (shl*b.zip)
		:DJGPP GNU patch (pat*b.zip)
		:SEAL 1.0.7 + mame patch
		:Allegro 3.9.40 (or newer)

	The patched SEAL library is available at http://www.mame.net

	Ensure to have the DOS version of NASM. If you have the Windows
	version named `nasmw.exe' you must rename it as `nasm.exe' or
	change the `Makefile' to use it.

	To build on Linux you need the additional following software:
		:Linux 2.4.5 (or newer)
		:SVGALIB 1.9.11 (or newer)
		:S-Lang 1.4.3 (or newer)
		:LibSDL 1.2.4 (or newer)

	Linux 2.2 is not tested, anyway it should work.

	The SVGALIB 1.4.x versions are NOT supported.
	Download the latest ALPHA 1.9.x version from http://www.svgalib.org.

	If your distribution doesn't contain the S-Lang library you
	can download it from http://www.s-lang.org/.

	Previous versions of these libraries may work, anyway they
	are not tested.

	To build on Windows you need the additional following software:
		:MINGW 1.1 (or newer)
		:LibSDL 1.2.4 (or newer)

Copyright
	This file is Copyright (C) 2002 Andrea Mazzoleni.

