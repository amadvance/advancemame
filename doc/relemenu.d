Name
	release notes - Release Notes For AdvanceMENU

AdvanceMENU 2.2.7
	This release adds support for the new MAME -listxml option. It's now
	used as default. The old -listinfo is still supported and it's used
	if -listxml fails.

	The xml file is saved with the .xml extension. You probably want to
	remove the old .lst file.

AdvanceMENU 2.2.6
	This release supports Mac OS X with the SDL library. Please note that
	it isn't able to directly program your video board, so you cannot use
	it with an Arcade Monitor.

AdvanceMENU 2.2.5
	The precompiled DOS binary of this release is compiled with
	the old gcc 2.95.3. Please report if this fixes any know
	problem.

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

AdvanceMENU 1.17.4
	The option `preview_aspect' is removed. It's automatically
	converted to the new option `preview_expand'.

AdvanceMENU 1.16.1
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

