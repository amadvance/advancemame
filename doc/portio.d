Name{number}
	portio - AdvanceCAB Hardware Port Input/Output For DOS

Synopsis
	:portio lpt?
	:portio ADDRESS_HEX
	:portio lpt? VALUE_HEX
	:portio ADDRESS_HEX VALUE_HEX

Description
	The `portio` utility can be used to drive the parallel port
	or any other hardware device which uses the PC ports.

	If an lpt port (lpt1,lpt2,lpt3) or a hexadecimal address is
	specified the port is read and the read value is printed on
	the screen.

	If also a value is specified the port is written with the
	specified value.

	All addresses and all values are in hexadecimal format.

	The addresses of the lpt ports are detected using the BIOS
	information at the memory address 408h.

Examples
	Set all the parallel port data bits:

		:portio lpt1 ff

	Clear all the parallel port data bits:

		:portio lpt1 0

	Set all bits on the port 80h:

		:portio 80 ff

	Set all the parallel port data bits from config.sys:

		:device=c:\mame\portio.exe lpt1 ff

Copyright
	This file is Copyright (C) 2003 Andrea Mazzoleni.
