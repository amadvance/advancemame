BUILD
	This file contains the instructions to build AdvanceMAME, AdvanceMESS,
	AdvancePAC and AdvanceMENU from the source archives.

PREPARING THE SOURCE
	To compile AdvanceMENU you don't need to add anything at the source, they are
	already complete.

	To build one of the Advance version of the MAME, MESS, PacMAME emulators you
	need first to download and unzip the original emulator source. You must
	use the same version of the Advance source.

	The source of the MAME emulator must be unzipped in the `src/' directory,
	the MESS source in `srcmess/' and the PacMAME source in `srcpac/'.
	In a Unix system remember to unzip the original .zip archives with the
	`unzip -aa' command to convert all the files from the DOS/Windows CR/LF format
	to the Unix CR format.

	After unpacking the emulator sources you need to patch the MAME source with the
	patch `advance/advmame.dif', the MESS source with `advance/advmess.dif' and the
	PacMAME source with `advance/advpac.dif'. If the patch aren't applied correctly
	probably you are using the wrong version of the emulator source.

CONFIGURING
	In a Unix system you need to run the `./configure' script with the
	proper options. You can get a complete list with the `./configure --help' command.
	Generally, you need only to specify the the --with-system option choosing the
	`sdl' or `native' system library.

	The `native' system uses the svgalib 1.9 and framebuffer graphics libraries and it's
	able to directly access and completly control the graphics output of your video
	board and automatically generate video modes with the correct size and frequency.

	The `sdl' system uses the LibSDL graphics library, it can be used to show the
	programs in a Window Manager, but it's unable to completly control the graphics
	output. It isn't a good choice for the fullscreen use of the emulator.

	If you want to customize the compilation CFLAGS you can set them before
	calling the ./configure script, for example:

		:CFLAGS="-O3 -march=pentiumii -fomit-frame-pointer -fstrict-aliasing" ./configure

	In a DOS/Windows system you need to manually rename the `Makefile.in' file
	as `Makefile', and edit the its first section to match your requirements.

COMPILING
	Finally you can run `make' to compile all, and in a Unix system `make install'
	to install the binaries and the documentation.

TARGETS
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

REQUIREMENTS
	To compile the Advance projects you need the following
	software :
		:GNU gcc 2.95.3 or 3.0.4 or 3.1 (with c and c++ support)
		:NASM 0.98 (or newer)
		:zlib 1.1.3 (or newer)
		:UPX 1.20 (or newer)
		:Make 3.79.1 (or newer)

	The gcc compiler versions 2.96.x, 3.0, 3.0.1 and 3.0.2 are
	NOT supported.

	To build in DOS you need the additional following software:
		:DOS 6.22 or Windows 9x/Me
		:CWSDPMI
		:DJGPP development kit 2.03
		:DJGPP GNU binutils
		:DJGPP GNU C/C++ compiler
		:DJGPP GNU make
		:DJGPP GNU fileutils
		:DJGPP GNU patch
		:SEAL 1.0.7 + mame patch
		:Allegro 3.9.40 (or newer)

	The patched SEAL library is available at http://www.mame.net

	Ensure to have the DOS version of NASM. If you have the Windows
	version named `nasmw.exe' you must rename it as `nasm.exe' or
	change the `makefile' to use it.

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
		:Windows (any version)
		:MINGW 1.1 (or newer)
		:LibSDL 1.2.4 (or newer)

