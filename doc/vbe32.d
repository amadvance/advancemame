Name
	vbe32 - VBE 32 bit BIOS for DOS

Synopsys
	:vbe32 [/l] [/c CONFIG]

Description
	The `vbe32' utility is a 32 bit TSR (Terminate Stay Resident)
	VBE BIOS that completly substitutes the original VBE BIOS of your
	video board.

	It allow to set an arbitrary video mode list with the favourite
	modes which work best with your video hardware without any
	restrictions in size and frequencies.

	With this utility you can play all the DOS games that use the
	standard VESA VBE services with your PC Monitor, TV and Arcade
	Monitor.
 
	At the startup the configuration file `vbe.rc', created with
	the utility `vbev', is read and all the modelines present
	are activated. The format of this configuration file is
	specified in the document `advv.txt'.

	These are some example rc files:

		ntsc.rc - NTSC TV.
		pal.rc - PAL TV.
		standard.rc - Arcade Standard Resolution (15 kHz).
		medium.rc - Arcade Medium Resolution (25 kHz).
		extended.rc - Arcade Extended Resolution (16.5 kHz).
		pcvga.rc - VGA PC Monitor.
		pcsvga60.rc - SVGA PC Multisync Monitor.

	All the modelines present in the configuration files are used
	to create the list of available VBE mode. For every modeline
	five VBE modes with 8, 15, 16, 24, 32 bits are added.

Options
	These are the command line options supported:

	/l
		Load in memory the utility

	/c CONFIG
		Use an arbitary configuration file

Cards
	The program supports all the `svgaline' drivers listed in the
	`carddos.txt' file.

Limitations
	* The VBE 1.2 standard isn't supported. This makes the program
		useless for 16 bit applications. Eventually this support may
		be added in future.
	* The VBE 3.0 standard isn't supported. Eventually this support
		may be added in future.
	* The VBE protect mode interface isn't supported.
	* It uses a lot of low memory. Approx 200 kbyte.
	* It cannot be unloaded. It's a limitation of the DPMI support
		not resolvable.

Application Problems
	Some applications are able to use only some common resolutions
	like 320x240, 400x300, 512x384 and 640x480. Others resolutions
	may be completly ignored or crash the application.

	Others applications may get in trouble if the list of avaliable
	VBE modes contains duplicate resolutions or is too big.
	Add only the best modelines and avoid duplicates.

Tests
	+Doom Legacy with the `nv3' driver (http://www.newdoom.com/doomlegacy)
	+Duke Nukem 3D with the `nv3' driver

Examples
	Load it:

		:vbe32 /l

	Load it with an arbitrary configuration file:

		:vbe32 /l /c c:\cab\myconf.rc

Bugs
	The configuration file is checked only for the
	`device_video_modeline' options.
	All the other options are ignored.

Copyright
	This file is Copyright (C) 2002 Andrea Mazzoleni.

