CARD
	This is the list of all the video cards supported by the
	AdvanceMAME emulator and AvanceMENU frontend.

SVGALINE
	These are DOS/Linux drivers based on the SVGALIB Linux
	library (at the time of writing version 1.9.15).

	The numbers in [] are the PCI device IDs of the video board
	models. The numbers in () are the PCI vendor IDs of the
	video board manufacturers.

	nv3 - nVidia (10DE), SGS (12D2)
		:Riva 128 [x01x]
		:Riva TNT [x02x], [x0Ax]
		:GeForce 256 [x10x]
		:GeForce2 [x15x]
		:GeForce2 MX [x11x], [x17x], [x1Ax] (interlaced modes not supported)
		:GeForce3 [x20x] (interlaced modes not supported)
		:GeForce4 [x25x] (interlaced modes not supported)

	r128 - ATI (1002)
		:Rage 128 [4c45, 4c56, 4d46, 4d4c, 50xx, 52xx, 53xx, 54xx]
		:Radeon [4242, 4c57, 4c59, 4c51, 51xx]

	g400 - Matrox (102B)
		:Mystique [051A, 051E]
		:G100 [1000, 1001]
		:G200 [0520, 0521]
		:G400 [0525], G450 [0525]

	banshee - 3dfx (121A)
		:Voodoo Banshee [0003] (interlaced modes not supported)
		:Voodoo 3 [0005]
		:Voodoo 4/5 [0009]

	pm2 - Permedia (3d3d) (interlaced modes not supported)
		:PM2 [0007]
		:PM2V [0009]

	millenium - Matrox (102B)
		:Millenium 2064 [0519]
		:Millenium II 2164 [051B,051F]

	savage - S3 (5333)
		:Trio 64 [8811], 3D [8903,8904], 3D 2X [8a13]
		:Virge [5631], VX [883d], DX [8a01], GX2 [8a10], MX [8c00,8c01,8c02,8c03]
		:Savage 3D [8a20,8a21], 4 [8a22,8a23], MX [8c10,8c12], 2000 [9102]

	trident - Trident (1023)
		:TGUI 9420DGi [9420], 9430DGi [9430], 9440AGi [9440,9460,9470], 96xx [9660]
		:Cyber 9320 [9320], 9388 [9388], 9397 [9397], 939A/DVD [939A], 9520 [9520], 9525/DVD [9525], 9540 [9540]
		:3DImage 975 [9750], 985 [9850]
		:CyberBlade/i7 [8400], DSTN/i7 [8420], i1 [8500], DSTN/i1 [8520], Ai1 [8600], DSTN/Ai1 [8620], XP [9910], XPm [9930]
		:Blade3D [9880]

	rendition - Rendition (1163) (interlaced modes not supported)
		:V2200 [2000]

	i740 - Intel (8086) (interlaced modes not supported)
		:740 [00d1,7800]

	i810 - Intel (8086) (interlaced modes not supported)
		:810 [1132]

	sis - SiS (1039)
		:SG86C201 [0001], 202 [0002], 205 [0205], 215 [0215], 225 [0225]
		:5597, 5598 [0200]
		:300 [0300], 530 [6306], 540 [5300], 630 [6300]
		:6326 [6326]

	laguna - Cirrus (1013)
		:Laguna 5462 [00d0]
		:Laguna 5464 [00d4]
		:Laguna 5465 [00d6]

	apm - APM (1142) (interlaced modes not supported)
		:AT24 [6424]
		:AT25/3D [643d]

	rage - ATI (custom detection)
		:Rage (many old cards)

	mx - MX (custom detection)
		:86250, 86251

	s3 - S3 (custom detection)
		:(many old cards)

	ark - ARK (custom detection)
		:1000PV, 2000PV

VBELINE
	These are the OLD DOS drivers based on the VBE BIOS of the
	video board. Some of these drivers are derived from the
	VSyncMAME video drivers written by Saka and from the ATI driver
	of MAMEDOS.

	Some of them require the presence of an updated VBE BIOS like
	the Scitech Display Doctor (shortly SDD).

	matrox - Matrox
		:Mystique 1064SG, G100, G200, G400

	ati - ATI
		:Mach 64 (without SDD), ATI All-In-Wonder Pro (without SDD)

	cirrus - Cirrus Logic (all with SDD)
		:GD542?, GD5428, GD543?

	s3 - S3 (with SDD)
		:ViRGE, Vision 866, Vision 964, Trio32, Trio64, Trio64V+
		:Aurora64V+, Trio64UV+, ViRGE/VX, 868, 928, 864, 964, 968,
		:Trio64 V2/DX, Trio64V2, ViRGE/DX, ViRGE/GX, ViRGE/GX2

	sis - SiS
		:6326, 620, 530, 300, 630, 540

	savage - S3 Savage
		:Savage 3D, Savage 3DM, Savage 4

	laguna - Cirrus Logic Laguna
		:GD5462 Laguna, GD5464 Laguna, GD5465 Laguna

	3dfx - 3dfx (interlaced modes not supported)
		:Voodoo Banshee, Voodoo 3, Voodoo 4/5

	r128 - ATI Rage 128
		:Rage 128 Mobility LE/LF/MF/ML, Rage 128 Pro PF/PR,
		:Rage 128 RE/RF/RK/RL, Rage 128 RF AGP, Rage 128 RK PCI
		:Rage 128 RL AGP,
		:Rage 128 Pro (many),
		:Rage 128 (many)

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
		:Any video boards with a VBE3 BIOS which is also VGA
		:compatible at the registers level.

FB
	The `fb' driver uses the Linux Kernel Consolle Driver.
	It supports all the video board supported by your Linux
	Kernel.

COPYRIGHT
	This file is Copyright (C) 2002 Andrea Mazzoleni, Randy Schnedler.

