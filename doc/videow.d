Name
	videow - Video control for Windows NT/2000/XP

Synopsys
	:videow [/s XxYxBITS] [/a] [/o] [/e] [/d] [/c FILE] [/p]
	:	[/n SIZE] [/m]

Description
	The `videow' utility can be used to control the video
	output of the PC in Windows NT/2000/XP.

	It requires the `svgawin' driver installed and running.

	The modelines are read from the file `videow.rc'.
	The format of this configuration file is specified in the
	document `advv.txt'.

	These are some example rc files:

		ntsc.rc - NTSC TV.
		pal.rc - PAL TV.
		standard.rc - Arcade Standard Resolution (15 kHz).
		medium.rc - Arcade Medium Resolution (25 kHz).
		extended.rc - Arcade Extended Resolution (16.5 kHz).
		pcvga.rc - VGA PC Monitor.
		pcsvga60.rc - SVGA PC Multisync Monitor.

Options
	/s XxYxBITS
		Set an arbitrary video mode using the first modeline
		with the correct size found in the config file.
		You can specify only resolutions already supported by
		Windows. You cannot create new resolutions.

	/a
		Adjust the current video mode using the first modeline
		with the correct size found in the config file.

	/o
		Set the original Windows mode. Use this option if
		the screen become unreadable.

	/d
		Disable the hardware video signal.

	/e
		Enable the hardware video signal.

	/c CONFIG
		Use an arbitrary configuration file instead of the
		standard `videow.rc' file.

	/p
		Print some info of the video board.

	/n SIZE
		Set the length of the scanline in bytes. The program
		automatically select the smallest scanline size.

	/m
		Enable the use of the hardware mouse pointer.

Cards
	The program supports all the `svgawin' drivers listed in the
	`cardwin.txt' file.

Limitations
	* The program can only set the resolutions originally supported
		by Windows. It cannot create new resolutions.
	* If the video board or the video driver doesn't support interlaced
		modes, these modes are simulated skipping every odd row.
		It doesn't look very good.
	* The program doesn't stay resident, any successive mode change will
		reset the original video mode.

Troubleshotting
	) If the mouse pointer disappers try to reanable it with the /m option.

	) If the screen image is stable but garbled is probably because the
		Windows video driver use a not standard scanline size.
		Try to guess it with the /n option. You should start
		with the resolution width multiplied by the bytes per pixel,
		and increase this value until the screen become readable.

	) Try decreasing the video hardware acceleration.

Tests
	+Windows 2000 with the `nv3' driver

Examples
	Set a 640x480 mode with 16 bits per pixel:

		:videow /s 640x480x16

	Adjust the current mode:

		:videow /a

Bugs
	The configuration file is checked only for the
	`device_video_modeline' options.
	All the other options are ignored.

Copyright
	This file is Copyright (C) 2002 Andrea Mazzoleni.

