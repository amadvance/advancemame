Name{number}
	videow - AdvanceCAB Video Control For Windows 2000/XP

Synopsis
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

	You can force the use of a specific driver using the `device_video'
	option in the configuration file.

	The available drivers are :
		svgawin/auto - Auto detection.
		svgawin/nv3 - nVidia Riva/GeForce.
		svgawin/trident - Trident.
		svgawin/rendition - Rendition.
		svgawin/g400 - Matrox Mystique/G100/G200/G400/G450.
		svgawin/pm2 - Permedia 2.
		svgawin/savage - S3 Savage.
		svgawin/millenium - Matrox Millennium/Millenium II.
		svgawin/r128 - ATI Rage 128/Radeon.
		svgawin/banshee - 3dfx Voodoo Banshee/3/4/5.
		svgawin/sis - SIS.
		svgawin/i740 - Intel i740.
		svgawin/laguna - Cirrus Logic Laguna 5462/5464/5465.
		svgawin/rage - ATI Rage.
		svgawin/mx - MX.
		svgawin/et6000 - ET6000.
		svgawin/s3 - S3.
		svgawin/ark - ARK.
		svgawin/apm - APM.

Limitations
	* The program can only set the resolutions originally supported
		by Windows. It cannot create new resolutions.
	* If the video board or the video driver doesn't support interlaced
		modes, these modes are simulated skipping every odd row.
		It doesn't look very good.
	* The program doesn't stay resident, any successive mode change will
		reset the original video mode.

Troubleshooting
	) If the mouse pointer disappears try to reenable it with the /m option.

	) If the screen image is stable but garbled it's probably because
		the Windows video driver uses a not standard scanline size.
		Try to guess it with the /n option. You should start
		with the resolution width multiplied by the bytes per pixel.
		Then, increase this value until the screen become readable.
		Generally the correct value is multiplier of 2^n where n
		may vary from 1 to 8.

	) Try decreasing the video hardware acceleration.

Tests
	+Windows 2000 with a GeForce 2 board

Examples
	Set a 640x480 mode with 16 bits per pixel:

		:videow /s 640x480x16

	Adjust the current mode:

		:videow /a

Bugs
	The configuration file is checked only for the
	`device_video_modeline' and `device_video' options.
	All the other options are ignored.

Copyright
	This file is Copyright (C) 2003 Andrea Mazzoleni.

