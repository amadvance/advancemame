Name{number}
	advcfg - AdvanceMAME Automatic Video Configurator

Synopsis
	:advcfg [-advmamec] [-advmessc] [-advmenuc]
	:	[-log] [-logsync] [-rc RCFILE] [-bit N]

Description
	The `advcfg' utility is the automatic video configuration
	program for AdvanceMAME, AdvanceMESS and AdvanceMENU.

	This utility works differently if one of the -advmamec,
	-advmessc or -advmenuc options is specified.
	The main difference is the name of the configuration
	file used to store the video modes.

	All the video modes used in the configuration process
	always have negative horizontal and vertical sync polarity.

	Check the `advdev.txt' file for more details on the video
	drivers supported.

Options
	-rc RCFILE
		Specify an alternate configuration file.

	-log
		A very detailed log of operations is saved in
		a `.log' file. Very useful for debugging problems.

	-logsync
		Like the `-log' option but the log file is not
		buffered. This option must be used to get a complete
		log file in case of a machine crash.

	-advmamec, -advmessc, -advmenuc
		Select the mode of operation for the programs
		`advmame', `advmess' and `advmenu'.
		The default is for `advmame'.

	-bit N
		Select the bit depth of the test video modes.
		If omitted, 8-bit modes are used.
		Valid values are 8, 15, 16 and 32.

Configuration
	Depending on the command line options used, one of the
	`advmame.rc', `advmess.rc' or `advmenu.rc' files is used
	to load and save the video configuration.

	If the configuration process completes successfully, the
	program adds these options to your configuration file:

		:device_video_clock ?
		:device_video_format ?

	which define the capabilities of your monitor and video board.
	And for the `advmame' and `advmess' modes also the options:

		:display_mode auto
		:display_adjust generate_yclock

	which select the `automatic' video mode generation of the
	emulators.

	All these options are documented in the `advdev.txt'
	and `advmame.txt' files.

Copyright
	This file is Copyright (C) 2003, 2004 Andrea Mazzoleni.
