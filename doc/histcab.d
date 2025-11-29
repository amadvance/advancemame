Name
	history - History For AdvanceCAB

AdvanceCAB Version 1.1.4 2003/07
	) Upgraded the SVGALIB 1.9.17 library with all the patches
		from AdvanceMAME 0.71.0.

AdvanceCAB Version 1.1.3 2002/11
	) Fixed the abort bug for the SVGALIB Rage 128/Radeon video
		boards and probably others.

AdvanceCAB Version 1.1.2 2002/11
	) The VIDEOW utility correctly detected the current
		bytes per scanline.
	) Updated the Windows SVGAWIN.SYS driver to the 1.1 version.
	) Fixed a slowdown bug on the SVGALIB video board detection.
	) Fixed a bug on the DOS/Windows SVGALIB Radeon driver. It should have
		helped with the video modes at lower frequency generally used
		in Arcade Monitors and TVs.
	) Made minor changes to the docs.
	) Improved error reporting on the DOS/Windows SVGALIB drivers.
	) The VIDEOW utility now used the `device_video' to force a specific
		video driver.

AdvanceCAB Version 1.1.1 2002/11
	) Added the ADVV utility for Windows NT/2000/XP.
	) Added a new option to the VIDEOW utility to enable the
		hardware mouse pointer.
	) Added a new option to the VIDEOW utility to change the
		scanline size.

AdvanceCAB Version 1.1.0 2002/11
	) Added the VIDEOWIN utility. It was able to set an arbitrary
		modeline in Windows NT/2000/XP. It supported all the
		most recent boards.

AdvanceCAB Version 1.0.0 2002/10
	) Added the VBE32 utility. It was a new 32 bit VBE BIOS
		which supported all the new video boards.

AdvanceCAB Version 0.11.2 2002/07
	) Recompiled the `advv' utility with the new
		VGA and SVGA video drivers.

AdvanceCAB Version 0.11.1 2002/01
	) Recompiled the `advv' utility with lower optimizations
		and for the 386 processor.

AdvanceCAB Version 0.11.0 2002/01
	) Changed the configuration file format, like AdvanceMAME 0.56.2.
	) Made some file renaming.
	) Fixed a minor bug in the command /r of the video utility.

AdvanceCAB Version 0.10.2 2001/10
	) Only the `mv.exe' program was updated.

AdvanceCAB Version 0.10.1 2001/10
	) Fixed a minor bug.

AdvanceCAB Version 0.10 2001/09
	) The `vbeline_driver' option was correctly read.

AdvanceCAB Version 0.9 2001/09
	) Had the same modelines changes as AdvanceMAME 0.53.
	) Added support for the Voodoo5 board.
	) Added the -r option to the `video' utility to reset the
		video board using the reset BIOS function.

AdvanceCAB Version 0.8 2001/03
	) Corrected the default `vgaline' modes in the files
		standard.cfg, extended.cfg and medium.cfg.
	) Solved some problems with the vga/vbe/video utilities.
	) The vbe utility was tested in Windows 98 SE with a S3
		board.

AdvanceCAB Version 0.7 2001/02
	) The `video' utility now cleared the screen before loading a
		PCX image.
	) The modeline utility could now generate an arbitrary mode from
		the command line. Example "modeline /svga60 384x224".

AdvanceCAB Version 0.6 2001/01
	) Added the `contrib/bootlogo' utility by Robert Palmqvist.
	) Cursor shape was correctly set up by the `vga' utility.
	) Solved the command line problems loading the utilities in
		the config.sys. Now all the options were case insensitive.
	) Added the new utility `modeline'. All the `mv' modelines were now
		computed with this utility.

AdvanceCAB Version 0.5 2000/11
	) AdvanceVBE/VGA was renamed AdvanceCAB.
	) Other utilities `portio' and `off' were moved to AdvanceCAB.
	) Added support for Trident cards using the new video
		driver of VSyncMAME written by Saka.
		Used the "vbeline_driver=vbe3" option to revert to the
		old behavior.
	) Corrected some register wasting in the `vga' and
		`vbe' utility.
	) Corrected the ModeX change in the `vga' utility.
	) WARNING! Changed the command line options. Now if these utilities were called
		without options a short help screen was
		printed. Users were required to update all the batch files!
	) Added `sys' support to the utilities `vga' and `portio'.
		They could be loaded directly in the `config.sys' as a
		`device'. These versions could not be unloaded.
	) Added an option to the `vga' utility to disable the video
		output of other programs.
	) Added a new utility `video' to enable and disable the hardware
		video output, load images, or set an arbitrary
		video mode.

AdvanceVBE/VGA Version 0.4 2000/10
	) Changed the mode number of the mode 1024x768 16 bit to follow
		the VESA 1.2 standard.

AdvanceVBE/VGA Version 0.3 2000/09
	) Performed source maintenance.

AdvanceVBE/VGA Version 0.2 2000/08
	) Saved all the 32 bit registers in the int 10h call
		(this should have solved some problems).
	) Added a private stack (this should have solved A LOT of problems).
	) Increased the max number of modelines to 64.
	) Added the `vga' utility (preliminary).
	) Added support for the vga text mode 01h (40x25); this could
		be used as default text mode for Arcade 25 kHz monitors.

AdvanceVBE/VGA Version 0.1 2000/08
	) Started the AdvanceVBE project.
