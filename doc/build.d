Name{number}
	build - AdvanceMAME Build Notes

	This file contains the instructions to build AdvanceMAME,
	AdvanceMESS and AdvanceMENU from the source archives.

Build
	To build the Advance programs you need to have installed the development tools.

	What to install depends on your system, but in general you need the GCC compiler,
	and the SDL library.

	For Ubuntu:
		:$ sudo apt-get update
		:$ sudo apt-get install git build-essential autoconf automake libsdl1.2dev

	For Raspbian:
		:$ sudo apt-get update
		:$ sudo apt-get autoconf automake libsdl1.2dev

	At this point you can get the source code from the http://www.advancemame.it site,
	and untar it with:

		:$ tar xf advancemame-*.tar.gz
		:$ cd advancemame-*

	or alternatively you can get it directly wit git:

		:$ git clone https://github.com/amadvance/advancemame.git
		:$ cd advancemame
		:$ sh autogen.sh

	Using git you get the source code of AdvanceMAME, AdvanceMESS and AdvanceMENU
	in a single operation.

	Now you can build and install using the typical sequence of commands:

		$ ./configure
		$ make -j4
		$ sudo make install

	The -j4 option tells make to use four different parallel proceses to build.
	Beside that, the build proces may be long, up to 30 minutes in a Raspberry Pi 3.

	No option is generally required. You can get the complete
	configure option list with the `./configure --help' command.

	The default installation prefix is /usr/local. You can change it
	with the `--prefix=' option.

	The default host configuration directory is /usr/local/etc. You can
	change it with the `--sysconfdir=' option. Please note that instead
	of /usr/etc is always used /etc.

	The configure script automatically detects all the available
	libraries and the optimization flags. You can use the
	--with-sdl-prefix option to search for the SDL library in a
	specific location.

	The `make install' comman installs the binaries and the documentation.

	The binaries are installed in $prefix/bin, the program data
	files in $prefix/share/advance, the documentation in
	$prefix/share/doc/advance, and the man pages in $prefix/man/man1.

	In Mac OS X ensure that the directory $prefix/bin is in the
	search PATH. Generally /usr/local/bin isn't.

	After the installation, you can start AdvanceMAME with:

		:$ advmame polyplay

	The first run will create the configuration file, and tell you where.
	At the second run the game will effectively start.

Copyright
	This file is Copyright (C) 2003 - 2016 Andrea Mazzoleni.

