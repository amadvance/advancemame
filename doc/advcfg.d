Name
	advcfg - The AdvanceCFG Automatic Configurator

Synopsis
	:advcfg [-advmamec] [-advmessc] [-advpacc] [-advmenuc]
	:	[-log] [-logsync] [-rc RCFILE] [-bit N]

Description
	The `advcfg' utility is the automatic video configuration
	program for AdvanceMAME, AdvanceMESS, AdvancePAC and
	AdvanceMENU.

	This utility works differently if one of the -advmamec,
	-advmessc, -advpacc and -advmenuc option is specified.
	The main difference is the the name of the configuration
	file used to store the video modes. Respectively they
	are `advmame.rc', `advmess.rc', `advpac.rc' and `advmenu.rc'.

	Check the `advv.txt' file for more details on the video drivers
	supported.

Options
	-rc RCFILE
		Specify an alternate name of the configuration file.

	-log
		A very detailed log of operations is saved in
		a `.log' file. Very useful for debugging problems.

	-logsync
		Like the `-log' option but the log file is not
		buffered. This option must be used to get a complete
		log file in presence of a machine crash.

	-advmamec, -advmessc, -advpacc, -advmenuc
		Select the mode of operation for the programs `advmame',
		`advmess', `advpac' and `advmenu'.
		The default is for `advmame'.

	-bit N
		Select the bit depth of the test video modes.
		If omitted the 8 bit modes are used.
		Valid values are 8, 15, 16 and 32.

Copyright
	This file is Copyright (C) 2002 Andrea Mazzoleni.

