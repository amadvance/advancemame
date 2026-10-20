Name{number}
	advbare - AdvanceMAME XML Filter

Synopsis
	:advbare < INPUT > OUTPUT

Description
	The `advbare` utility reads a MAME, MESS or AdvanceMAME XML
	information database from standard input and filters it to
	standard output, removing all elements and attributes not
	required by frontend programs like AdvanceMENU (such as
	dipswitches, ports, chips, sound channels, software lists).

	The filtered output contains only the bare information (games,
	descriptions, year, manufacturer, roms, video and driver status).
	This reduces the file size by almost 90% (e.g. from 272 MB to ~33 MB
	for MAME) and dramatically speeds up AdvanceMENU startup time.

	The utility acts strictly as a standard UNIX filter reading from
	standard input and writing to standard output.

Examples
	To filter the output of MAME directly into the AdvanceMENU
	configuration directory:

		:mame -listxml | advbare > ~/.advance/mame.xml

	To filter an existing large XML database:

		:advbare < ~/.advance/mame.xml > ~/.advance/mame_bare.xml

Copyright
	This file is Copyright (C) 2026 Andrea Mazzoleni.
