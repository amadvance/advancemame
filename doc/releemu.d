Name
	release notes - Release Notes For AdvanceMAME/MESS

AdvanceMAME 0.68.0
	This release add supports the new `scale3x' and `scale4x' effects.
	To use them you must ensure to use an high pclock upper limit.
	Something like 150 MHz. You need also a monitor wich support an
	high hclock upper limit. Something like 70 kHz.
	Otherwise the required 3x3 and 4x4 times bigger mode may be rejected.

AdvanceMAME 0.67.0 / AdvanceMESS 0.66.0
	The precompiled DOS binaries of these releases are compiled with
	the old gcc 2.95.3. Please report if this fixes any know specific
	game problem.

AdvanceMAME 0.63.0
	The .chd files must now be placed in a subdirectory of the same name.
	For example C:\CHD\AREA51\AREA51.CHD.

AdvanceMAME 0.62.2 / AdvanceMESS 0.62.0.0
	The display_rgb and display_depth options are gone. They are now
	substituted with the new display_color option.
	The device_video_*bit options are now substituted by the
	new device_color_* options.

	The option device_sdl_fullscreen is now substituted with the
	device_video_output option which is also used to enable a new
	`zoom' mode. Check the advv.txt file for other details.

AdvanceMAME 0.62.1
	The .CHD files are searched as default in the "image" directory.
	The .DIF file in the "diff" directory. You can customize them
	with the `dir_image' and `dir_diff' options.

AdvanceMAME 0.62.0 / AdvanceMESS 0.61.2
	In Windows NT/2000/XP you need to reinstall the SVGAWIN driver.
	Simply run "svgawin /u" and "svgawin /l".

AdvanceMAME 0.61.4 / AdvanceMESS 0.61.1
	This is the first Windows NT/2000/XP version able to directly 
	program your video board. This puts the Windows version at the same level 
	of the other Linux and DOS versions. It's very experimental, and tested only 
	with a GeForge 2. Anyway, it should work with all the video boards that 
	work in DOS and Linux because the SVGA drivers are the same.

	To use these video drivers you need to install the SVGAWIN.sys driver with the
	SVGAWIN.exe utility. If installed, AdvanceMAME automatically tries to use it
	unless you set a specific driver with the `device_video' option.
	If you don't install the SVGAWIN.sys driver AdvanceMAME works like the previous
	version using only the SDL library.

AdvanceMAME 0.61.0
	The `input_analog' and `input_track' options are now substituted
	by the new `input_map' option.

AdvanceMAME 0.59.1
	The option `input_analog[] joy[]' is now changed in
	`input_analog[] joystick[]', where the joystick stick index is
	decremented by 1. The conversion is done automatically.

AdvanceMAME 0.57.1
	You must remove the option `dir_sound' from the configuration file. The
	sound file are now saved in the `dir_snap' directory.
	You must rename manually the Linux config directories to `$home/.advance' and
	`$prefix/share/advance'. (Previously they were `*/advmame').

AdvanceMAME 0.56.2
	This is a big update for AdvanceMAME with a lot of changes.
	Don't uncompress the new archives in your MAME directory, but create
	a new one.

	To convert your old mame.cfg to the new format follow these steps :

	* Rename your configuration file as "mame.cfg" if it has a different name.
	* Copy it in the same directory of "advmame.exe".
	* Ensure that a file named "advmame.rc" doesn't exist.
	* Run "advmame.exe".
	* A file named "advmame.rc" should be now present in your current directory.

	This conversion works only for the DOS version of AdvanceMAME.

