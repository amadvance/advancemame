Name
	svgawin - SVGAWIN driver installer

Synopsis
	:svgawin [/l] [/u]

Description
	The `svgawin' utility installs and uninstalls the svgawin.sys
	driver in Windows NT/2000/XP.

	This driver is EXPERIMENTAL and it works ONLY IN FEW
	CONFIGURATIONS. Be prepared to reboot your PC if you use it.

	AdvanceMAME and AdvanceMENU work also without this driver, but
	you lose the ability to set an arbitrary video mode like the
	official MAME.

	If you want a stable solution you must use the Linux or DOS
	version of AdvanceMAME and AdvanceMENU.

	If you install it, AdvanceMAME and AdvanceMENU automatically
	try to use it before any other driver, unless you set a specific
	driver with the `device_video' option in the configuration files.

	You must run it as Administrator.

Options
	These are the command line options supported:

	/l
		Install and run the driver.

	/u
		Stop and uninstall the driver.

Copyright
	This file is Copyright (C) 2002 Andrea Mazzoleni.
