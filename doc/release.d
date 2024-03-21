Name
	release notes - Release Notes For AdvanceMAME

	Note that regarless of the version number, AdvanceMAME
	is based on MAME 0.106, and it needs a romset aligned to
	such MAME version.

AdvanceMAME/MENU 3.10
	This version includes numerous game improvements contributed by
	various individuals. For detailed information, please refer to the
	History file.

	Additionally, a crash when showing the onscreen menu with NVidia cards
	has been fixed.

	Moreover, this release introduces partial support for the Raspberry Pi
	4. More details can be found in the Install document.

	However, it's important to note that this support may not be
	sufficient for using a real Arcade monitor. As of now, the Raspberry
	Pi 3 remains the recommended and preferred option for this purpose.

AdvanceMAME/MENU 3.8
	Many improvements targeted to be able to build a completely
	standalone game console using Raspberry Pi hardware.

	You can now configure joystick input in both the emulator
	and the menu using button names, like "a", "tl", and
	with the advblue tool to automatically connect Bluetooth
	joysticks.

	The configuration files are now also saved in a resilient
	way that is resistant to unexpected power-downs, avoiding
	that a filesystem curruption prevents the programs to start
	at the next boot.

AdvanceMAME 3.4
	Another bugfix release that adds support for up of 8
	players in games like xmen6p. It also fixes the input_map[]
	support when dealing with multiple controls.

AdvanceMAME 3.2
	This release completes the Raspberry Pi support,
	fixing various bugs.

	A possible incompatible change is the reversed polarity
	of the video modelines in Raspberry Pi. If you have
	some modelines saved if your .rc files, you'll have
	to manually change the +hsync to -hsync, and +vsync
	to -vsync and viceversa.

	From this release the Raspberry Pi .deb binary is not
	linked anymore with SDL. It works only outside the X
	Window graphics enviroment.

AdvanceMAME 3.0
	This version is a major update for Raspberry Pi.

	Check the install.txt file for some more information
	about the use with a Raspberry Pi.

	The most important note for Raspberry, is to run the
	Advance programs directly from the Linux Console and
	outside the X-Window graphics environment to be able to
	use the Raspberry hardware acceleration.

	From this version now AdvanceMESS and AvanceMENU are
	included in both the source and binary packages of
	AdvanceMAME.

AdvanceMAME 1.2
	This version is an update for modern OS and compiler,
	it fixes some important bugs, and adds some new
	features like the new "xbr" video effect.

	Note that the full DOS port and the video drivers 'svgalib'
	for Linux and 'svgawin' for Windows are now obsolete.
	They are still present, but likely not working with modern
	video boards and removed in future releases.

	You can instead still use the 'fb' video driver using the Linux
	Frame Buffer support, and the generic 'sdl' using the SDL library.

AdvanceMENU 2.6
	This version is an update for modern OSs and compilers,
	and it fixes some important bugs.

AdvanceMAME 0.100.0
	This version has some internal changes to adapt to the
	recent internal MAME changes. Please report any regression
	from the previous version.

AdvanceMAME 0.97.0
	This version adds lightguns support in Windows XP.
	The lightguns supported are:

	* SMOG Lightgun (http://lightgun.splinder.com/)
	* Acts Labs Lightgun (http://www.act-labs.com/)

	The lightgun is automatically calibrated before
	every use. You must move the lightgun over the whole
	screen every time the program starts or changes video
	mode.

AdvanceMAME 0.95.1
	The DOS and Windows version now includes the old SVGALIB
	drivers for GeForce and Savage boards. If you have problem
	with the new driver you can still use the old ones with the
	names "savage_leg" and "nv3_leg".
	Use the "device_video sdl svgawin/savage_leg" option
	for Windows and "device_video vga svgaline/savage_leg" for
	DOS.

	The Windows version includes two new mouse driver called
	`rawinput' and `cpu' to allow the use of multiple mice
	in Windows 2000 and XP. Check the documentation in the
	advdev.txt file.

AdvanceMAME 0.94.0
	This version uses for DOS the new SVGALIB 1.9.20 library that
	adds support for the latest Radeon and nVidia video boards.

AdvanceMENU 2.4.7
	This version uses for DOS the new SVGALIB 1.9.20 library that
	adds support for the latest Radeon and nVidia video boards.

AdvanceMAME 0.92.1
	This version is based on the MAME update 0.92u2000. This
	is a very stable version of the MAME core. A good candidate
	for your cabinet.

AdvanceMAME 0.87.0
	This version of AdvanceMAME adds a new `device_video_clock'
	option which substitutes the previous `device_video_p/h/vclock'
	options. This new option allows a better support of
	multi-standard arcade monitor. You can now specify different
	vertical clocks for different horizontal clocks.

AdvanceMAME 0.85.0
	This version of AdvanceMAME tries always to use the installed
	zlib and expat libraries. If you want to use the copy included
	in the source of these libraries you must run the ./configure
	script with the arguments --disable-zlib and --disable-expat.

AdvanceMENU 2.3.8
	This release adds support for specific emulator configuration
	options. Please note that these specific emulator
	configurations are not activated if you select to show
	more than one emulator at time.
	In this case only the default configuration is used.
	Specifically these special options are `mode', `sort',
	`preview', `group_include' and `type_include'.

AdvanceMAME 0.84.0
	This version of AdvanceMAME is based on the MAME update
	0.84u3. The original MAME 0.84 version has some problems
	on the new input management.

AdvanceMAME 0.82.0
	This version has a reworked audio/video synchronization algorithm.
	It should fix any remaining problem of distorted sound present on
	some specific systems.

	This version adds a new "Startup End" key which is normally mapped
	as the minus key on the numeric pad. When you press this key the
	current game time is marked as the game startup time and
	saved in the configuration file with the `sync_startuptime' option.
	The next time you start the game, it will execute very fast
	until the startup time is reached.

AdvanceMAME 0.80.0
	This version adds support for TrueType (TTF) fonts with alpha blending
	using the FreeType2 library. If you compile from source, you must
	ensure to have installed the FreeType2 library from
	www.freetype.org or this feature will be disabled.

	This version uses the 16 bits rgb color mode as default also if the
	game has less than 256 colors. Previously the 8 bits palette
	color mode was used.

AdvanceMAME 0.79.1
	The host configuration directory is now under the installation prefix
	as $prefix/etc. This means that if you install in /usr/local the host
	configuration directory is /usr/local/etc and not /etc.
	You can customize this directory using the --sysconfdir option of the
	./configure script.

AdvanceMAME 0.79.0
	This release changes the manual compilation process in DOS
	and Windows. You must now use the Makefile.usr file instead of
	the old Makefile.in file. Check the build.txt file for more details.

AdvanceMENU 2.3.0
	This release changes the manual compilation process in DOS
	and Windows. You must now use the Makefile.usr file instead of
	the old Makefile.in file. Check the build.txt file for more details.

AdvanceMAME 0.78.0
	This release completely removes the .cfg file support. All the
	customization information are now saved in the main .rc file.
	This allow to view, edit and copy them directly. They are also more
	compact because only the difference from the default is saved.
	And they are independent of the internal MAME structure.

	This release add also a new user interface managed directly by
	AdvanceMAME which allow better visualization control because the
	interface isn't draw on the game bitmap, but directly on the screen.
	This mean that the interface doesn't have the same video effects
	of the game, it isn't limited on the game area, and it isn't recorded
	on video clips.

AdvanceMAME 0.77.0
	This release removes the legacy support for the DOS
	unchained VGA modes and for the banked VBE modes. It means
	that you now need a supported SVGA or VBE2 video card.

AdvanceMENU 2.2.14
	This release removes the legacy support for the DOS
	unchained VGA modes and for the banked VBE modes. It means
	that you now need a supported SVGA or VBE2 video card.

AdvanceMAME 0.72.0
	This release contains a new set of `event' Linux
	drivers for keyboards, mice and joysticks based on the Linux
	input-event interfaces.
	These drivers remove any limitations on the number of
	keyboards, mice and joysticks, and they give the best
	support for the new USB HID devices.

	The `raw' set of Linux drivers has now the same functionality
	of the `svgalib' set. If you don't need the SVGALIB video you can
	now completely remove this library.

	The `input_map' option now accept the `auto' setting which
	is able to map the correct input device on the correct
	game control. This option works best with the new
	Linux input-device drivers which are able to report
	correctly the exact type of input control.

	The `input_map' option now can remap also all the digital
	inputs like keys, buttons and digital joystick.
	This feature is similar at the official MAME ctrl remapping,
	but it allow to select any type of digital input, not only
	the inputs known by MAME. For example you can map the
	`bookmark' key (present only on some keyboards) also if
	doesn't exist a MAME code for it. This also remove any limitation
	on the number of joystick and mouse buttons.

	In Linux the host configuration files are now read in /etc,
	the files in */share/advance have now less priority
	of the user specified options. They can be used
	to set default options.

AdvanceMENU 2.2.10
	This release contains a new set of `event' Linux
	drivers for keyboards, mice and joysticks based on the Linux
	input-event interfaces.
	These drivers remove any limitations on the number of
	keyboards, mice and joysticks, and they give the best
	support for the new USB HID devices.

	The `raw' set of Linux drivers has now the same functionality
	of the `svgalib' set. If you don't need the SVGALIB video you can
	now completely remove this library.

	In Linux the host configuration files are now read in /etc,
	the files in */share/advance have now less priority
	of the user specified options. They can be used
	to set default options.

AdvanceMENU 2.2.7
	This release adds support for the new MAME -listxml option. It's now
	used as default. The old -listinfo is still supported and it's used
	if -listxml fails.

	The xml file is saved with the .xml extension. You probably want to
	remove the old .lst file.

AdvanceMAME 0.68.0
	This release supports the new `scale3x' and `scale4x' effects.
	To use them you must ensure to use an high pclock upper limit.
	Something like 150 MHz. You need also a monitor which support an
	high hclock upper limit. Something like 70 kHz.
	Otherwise the required 3x3 and 4x4 times bigger mode may be rejected.

	This release supports Mac OS X with the SDL library. Please note that
	it isn't able to directly program your video board, so you cannot use
	it with an Arcade Monitor.

AdvanceMENU 2.2.6
	This release supports Mac OS X with the SDL library. Please note that
	it isn't able to directly program your video board, so you cannot use
	it with an Arcade Monitor.

AdvanceMAME 0.67.0 / AdvanceMESS 0.66.0
	The precompiled DOS binaries of these releases are compiled with
	the old gcc 2.95.3. Please report if this fixes any know specific
	game problem.

AdvanceMENU 2.2.5
	The precompiled DOS binary of this release is compiled with
	the old gcc 2.95.3. Please report if this fixes any know
	problem.

AdvanceMAME 0.63.0
	The .chd files must now be placed in a subdirectory of the same name.
	For example C:\CHD\AREA51\AREA51.CHD.

AdvanceMENU 2.2.2
	The option `video_depth' is now removed. The video bit depth is
	chosen automatically. If you need to exclude some depth you can
	use the new `device_color_*' options.

AdvanceMENU 2.2.1
	The `group' and `type' menus are now accessible only from the
	main menu. The `group' and `type' events now automatically switch
	to the next item without displaying a menu.
	The selection logic is: first item, second item, ..., last item,
	all items, and repeat.

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
	with a GeForce 2. Anyway, it should work with all the video boards that
	work in DOS and Linux because the SVGA drivers are the same.

	To use these video drivers you need to install the SVGAWIN.sys driver with the
	SVGAWIN.exe utility. If installed, AdvanceMAME automatically tries to use it
	unless you set a specific driver with the `device_video' option.
	If you don't install the SVGAWIN.sys driver AdvanceMAME works like the previous
	version using only the SDL library.

AdvanceMENU 2.2.0
	This is the first Windows NT/2000/XP version able to directly
	program your video board. This puts the Windows version at the same level
	of the other Linux and DOS versions. It's very experimental, and tested only
	with a GeForce 2. Anyway, it should work with all the video boards that
	work in DOS and Linux because the SVGA drivers are the same.

	To use these video drivers you need to install the SVGAWIN.sys driver with the
	SVGAWIN.exe utility. If installed, AdvanceMENU automatically tries to use it
	unless you set a specific driver with the `device_video' option.
	If you don't install the SVGAWIN.sys driver AdvanceMENU works like the previous
	version using only the SDL library.

AdvanceMENU 2.0.0
	The emulator type `mame' is now used for the Windows MAME.
	The DOS MAME now requires the `dmame' emulator type.
	The DOS MESS now requires the `dmess' emulator type.
	The DOS Raine now requires the `draine' emulator type.

AdvanceMAME 0.61.0
	The `input_analog' and `input_track' options are now substituted
	by the new `input_map' option.

AdvanceMENU 1.17.4
	The option `preview_aspect' is removed. It's automatically
	converted to the new option `preview_expand'.

AdvanceMAME 0.59.1
	The option `input_analog[] joy[]' is now changed in
	`input_analog[] joystick[]', where the joystick stick index is
	decremented by 1. The conversion is done automatically.

AdvanceMENU 1.16.1
	You must rename manually the Linux config directories to `$home/.advance' and
	`$prefix/share/advance'. (Previously they were `*/advmame').

AdvanceMAME 0.57.1
	You must remove the option `dir_sound' from the configuration file. The
	sound file are now saved in the `dir_snap' directory.
	You must rename manually the Linux config directories to `$home/.advance' and
	`$prefix/share/advance'. (Previously they were `*/advmame').

AdvanceMENU 1.16.0
	The format of the configuration file is changed.
	These are the instruction to convert your "mm.cfg" to the new format :

	* Copy the file "mm.cfg" in the same directory of advmenu.exe.
	* Ensure that a file named "advmenu.rc" doesn't exist.
	* Run "advmenu.exe".
	* A file named "advmenu.rc" should be now present in your current directory.

	This conversion works only for the DOS version of AdvanceMENU.

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

