Name{number}
	advv - AdvanceMAME Video Manual Configurator

Synopsis
	:advv [-advmamev] [-advmessv] [-advmenuv]
	:	[-vbev] [-vgav] [-videowv] [-log] [-logsync]
	:	[-rc RCFILE]

Description
	The `advv' utility is the main video configuration program
	for the Advance programs. It selects, creates and tweaks
	video modes interactively.

	This utility works differently if one of the -advmamev,
	-advmessv, -advmenuv, -vbev, -vgav and -videowv options
	is specified.
	The main difference is the the name of the configuration
	file used to store the video modes.

	Before running this utility you must add in your configuration
	file the option `device_video_clock' required to defines
	the limits of your monitor and video board. You can add them
	manually or using the `advcfg' utility.

	Check the `advdev.txt' file for more details on these options.

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

	-advmamev, -advmessv, -advmenuv, -vbev, -vgav, -videowv
		Select the mode of operation for the programs `advmame',
		`advmess', `advmenu', `vbe', `vga' and `videow'.
		The default is `advmame'.

Use
	When the program starts a list of modelines is printed. You can
	walk on the list with the UP and DOWN arrows of your keyboard. 
	You can select the modelines to use in the emulator pressing
	SPACE.

	With the LEFT and RIGHT arrows you can select the bit depth of
	the video mode to test.

	To test a video mode you can press ENTER and ESC to return
	to the list.

	When you have finished the selection press F2 to save the video
	modes in the configuration file. At the next run the emulator
	will use one of the modelines that you have chosen.

	If you have problem please read the `Troubleshooting' chapter
	in the `install.txt' file.

	If you have correctly configured the `device_video_p/h/vclock'
	options in your configuration file, the video modes out of
	the frequency range supported by your monitor are displayed
	in red, and you are prevented to use them.

  Creating Video Modes
	To create a new modeline you should press F5 or F6.

	You will be asked for:

	* Vertical clock
	* Horizontal resolution
	* Vertical resolution

	If possible, a video mode compatible with your current monitor
	configuration is created. If a such mode doesn't exist,
	with F5 are favorite video modes with the specified size,
	with F6 are favorite video modes with the specified vertical
	frequency.

	If a the `device_video_format' option is present in your
	configuration file, the video mode is created with this
	format. Otherwise the generic VGA monitor format is used.

	All the video modes created have always negative horizontal
	and vertical sync polarity.

  Adjusting Modelines
	You can modify the modelines in the test screen pressing ENTER
	or in the list screen directly.

	When you are in the test screen, `advv' prevent you to set a
	modeline out of the frequency range supported by your monitor.
	In this cases you can hear a grave long sound. Instead when you
	are in the list screen you don't have this limitation.

	If you request a parameter not supported by your hardware
	you will hear an acute short sound.

	You can easily change one of the clock values of your modelines
	pressing F8.

	Finally you can rename the modelines pressing TAB.

	Remember to select the modified modelines with SPACE otherwise
	they aren't saved in your configuration file and will be lost.

  Startup Text Mode
	The program at startup tries to set a text mode supported by
	your hardware to show his data.

	If a modelines named `default_text' is present in your
	configuration file the program use it.

	If correct `device_video_p/h/vclock' options are found in your
	configuration file the program try to use a text mode that
	match your clock configuration from a list of predefined
	modes and the modes present in the configuration file.

	If no one of these modes match your configuration the current
	text mode is used.

Configuration
	Depending one the command line options used one of the `advmame.rc',
	`advmess.rc', `advmenu.rc', `vbe.rc', `vga.rc' and
	`videow.rc' file is used to load and save the video configuration.

Copyright
	This file is Copyright (C) 2003, 2004 Andrea Mazzoleni.

