NAME
	vbe - VBE mode remapper

SYNOPSYS
	:vbe [/l] [/u] [/c CONFIG]

DESCRIPTION
	The `vbe' utility is a TSR (Terminate Stay Resident) VBE BIOS
	wrapper that substitutes the default list of VBE video modes
	with a list of the favourite modes which work best with your
	video hardware without any restrictions in size and frequencies.

	With this utility you can play all the DOS games that use the
	standard VESA VBE 1.2/2.0 services with your TV or Arcade Monitor.
 
	At the startup the configuration file `vbe.rc', created with
	the utility `vbev', is read and all the `modeline' present
	are activated. The format of this configuration file is
	specified in the document `mv.txt'.

	These are some example rc files:

		ntsc.rc - NTSC TV.
		pal.rc - PAL TV.
		standard.rc - Arcade Standard Resolution (15 kHz).
		medium.rc - Arcade Medium Resolution (25 kHz).
		extended.rc - Arcade Extended Resolution (16.5 kHz).
		pcvga.rc - VGA PC Monitor.
		pcmult.rc - SVGA PC Multisync Monitor.

	All the modelines present in the configuration files are used
	to create the list of available VBE mode. For every modeline
	three VBE modes with 8, 15, 16 bits are added.

OPTIONS
	These are the command line options supported:

	/l
		Load in memory the utility

	/u
		Unload the utility

	/c CONFIG
		Use an arbitary configuration file

CARDS SUPPORTED
	This is the list of all the supported cards with the vbeline
	driver that support them.
	Some of these drivers require that you prevently load the
	`Scitech Display Doctor' (SDD) program to work correctly.

	3dfx - 3dfx (interlaced modes not supported)
		:Banshee, Voodoo3, Voodoo5

	cirrus - Cirrus Logic (with SDD)
		:GD542?, GD5428, GD543?

	s3 - S3 (with SDD)
		:ViRGE, Vision 866, Vision 964, Trio32, Trio64, Trio64V+
		:Aurora64V+, Trio64UV+, ViRGE/VX, 868, 928, 864, 964, 968,
		:Trio64 V2/DX, Trio64V2, ViRGE/DX, ViRGE/GX, ViRGE/GX2

	sis - SiS
		:6326, 620, 530, 300, 630, 540

	savage - S3 Savage
		:Savage 3D, Savage 3DM, Savage 4

	neomagic - NeoMagic (interlaced modes not supported)
		:MagicGraph 128 (NM2070), 128V (NM2090), 128ZV (NM2093)
		:128ZV+ (NM2097), 128XD (NM2160)
		:MagicMedia 256AV (NM2200), 256ZX (NM2360), 256XL+ (NM2380)

	trident - Trident (without SDD)
		:TGUI 9320, TGUI 9420, TGUI 9440, TGUI 9660
		:Providia 9682, Providia 9685
		:Cyber 9397, Cyber 9397/DVD, Cyber 9385, Cyber 9385-1
		:Cyber 9382, Cyber 9388, Cyber 9397, Cyber 939A/DVD, Cyber 9520
		:Cyber 9525/DVD, Cyber 9540
		:3DImage975, 3DImage985
		:CyberBlade/i7, CyberBlade/DSTN/i7, CyberBlade/i1, CyberBlade/DSTN/i1
		:Blade3D, Blade3D/T64

	vbe3 - (interlaced modes rarely supported)
		Any video boards with a VBE3 BIOS which is also VGA
		compatible at registers level. Generally a recent board
		should have a VBE3 BIOS.

LIMITATIONS
	AdvanceVBE isn't a complete VBE 2.0 bios. It requires an
	existing and working VBE BIOS. Specifically you can get the
	VBE 2.0 services, like the linear frame buffer, only if the
	preexistent BIOS supports them.
	Eventually, before loading AdvanceVBE, you can load a software
	BIOS like the `Scitech Display Doctor' or similar.

	The 24 and 32 bit depths are not supported.

APPLICATION PROBLEMS
	Some applications are able to use only some common resolutions
	like 320x240, 400x300, 512x384 and 640x480. Others resolutions
	may be completly ignored.

	Others applications may get in trouble if the list of avaliable
	VBE modes contains duplicate resolutions or is too big.
	Add only the best modelines and avoid duplicates.

USE IN WINDOWS
	The `vbe' utility is tested and working in Windows 98 with
	a S3 board. You should only load the SDD utility (if required)
	and the `vbe' utility in the `autoexec.bat'.

	Note that the Windows 16 bit HiColor modes are truly 15 bit VBE
	modes. So you should create some 15 bits modes in the `vbe.rc'.

	Windows notices only the video modes of size 640x480,
	800x600, 1024x768 and 1280x1024. Any other mode is ignored.

TESTS
	Doom Legacy with the `s3' driver (http://www.newdoom.com/doomlegacy)
	Duke Nukem 3D with the `s3' driver
	Quake 1.0 with the `s3' driver
	Windows 98 SE with the `s3' driver

EXAMPLES
	Load it:

		:vbe /l

	Load it with an arbitrary configuration file:

		:vbe /l /c c:\cab\myconf.rc

	Unload it:

		:vbe /u

	You can force a specific vbeline driver adding the
	`device_video vbeline/DRIVER' option in your configuration
	file.

	For example :
		:device_video vbeline/vbe3

BUGS
	The configuration file is checked only for the
	`device_video_modeline' and `device_video' options.
	All the other options are ignored. Also the `device_vbeline_*'
	are not read.

COPYRIGHT
	This file is Copyright (C) 2002 Andrea Mazzoleni.

