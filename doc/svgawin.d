Name{number}
	svgawin - AdvanceCAB SVGAWIN Driver Installer For Windows 2000/XP

Synopsis
	:svgawin [/l] [/u]

Description
	The `svgawin` utility installs and uninstalls the svgawin.sys
	driver in Windows 2000/XP.

	AdvanceMAME and AdvanceMENU also work without this driver, but
	you lose the ability to set arbitrary video modes.

	If you install it, AdvanceMAME and AdvanceMENU automatically
	try to use it before any other driver, unless you explicitly set
	a different driver with the `device_video` option in the
	configuration files.

	You must run this installation utility as Administrator.

Options
	These are the command line options supported:

	/l
		Install and start the driver.

	/u
		Stop and uninstall the driver.

Troubleshooting
	The svgawin drivers are inherently unsafe because they must coexist
	with the normal Windows drivers.

	The most common problem is that the Windows driver leaves the video
	card in a state that the SVGAWIN driver cannot understand or
	restore.

	The only practical solution is to reduce the video hardware
	acceleration to the minimum in
	Display Properties -> Settings -> Advanced -> Troubleshooting.

	If you want a completely stable solution you must use the Linux or
	DOS version of AdvanceMAME and AdvanceMENU.

	Alternatively you can try using two different video boards, one for
	Windows and the other for game display. Check the
	`device_svgawin_skipboard` option to control which video board to use.

Tech
	This driver is used to export at user level some reverse-engineered
	operations that are generally available only at kernel level.
	Specifically you can:

	* Access the PCI configuration space.
	* Use direct port I/O.
	* Map and unmap physical memory.
	* Call specific IOCTL_VIDEO_* ioctls on the Windows VIDEO driver.

	It does not contain any video-board-specific code. It only exports
	these basic services.

	The services interface is detailed in the
	advance/svgalib/svgawin/driver/svgacode.h file.

Copyright
	This file is Copyright (C) 2003, 2004 Andrea Mazzoleni.
