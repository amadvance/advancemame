Name
	svgawin - SVGAWIN driver installer

Synopsys
	:svgawin [/l] [/u]

Description
	The `svgawin' utility installs and uninstalls the svgawin.sys
	driver in Windows NT/2000/XP.

	This driver is experimental, be prepared to reboot your PC if you use it.
	
	If you install it AdvanceMAME and AdvanceMENU automatically try to use it
	before any other drivers, unless you set a specific driver with the 
	`device_video' option in their configuration files.

	You must run it as Administrator.

Options
	These are the command line options supported:

	/l
		Install and run the driver.

	/u
		Stop and uninstall the driver.

Copyright
	This file is Copyright (C) 2002 Andrea Mazzoleni.
