Name
	advmenu - History For AdvanceMENU

AdvanceMENU Version 2.1.0 2002/09
	) Revised the output format of all the documentation. Now
		it's available as formatted text, html and man pages.
	) Fixed a bug on the emulator names. Now they can be uppercase.
	) Fixed the "Division by Zero" crash bug with the Rage128 board in
		the DOS svgaline driver.
	) Upgraded at the SVGALIB 1.9.16 library.
	) Added a `./configure' script in all the distributions.
	) Removed the output of the `display_*' option when running
		advcfg for AdvanceMENU.
	) Fixed the detection of the screen resize keys in the
		advcfg and advv utility in the Linux platform.
		The keys are now changed to 'i' and 'k'.
	) Added the %o option in the emulator command line to force the
		emulators to use the same orientation of the menu.
		Check the example in the advmenu.txt file.
	) The keyboard key codes are changed another times. Now they are
		always the same for all the platform and for all the
		keyboard drivers. If you are using the linux version probably
		you need to reconfigure the event option with the new codes.
	) Renamed the `msg_run' option in `run_msg'.
	) Added the option `run_preview' to select which preview display
		before to run a game.
	) Removed all the `select_*' option. They are now specified with the
		new `emulator_attrib' option. It allow to specify different
		display attribute for different emulator. For example you can
		show clones for AdvancePAC and only parents for AdvanceMAME.
	) Fixed the slowdown bug with a lot of MESS roms.
	) Generally improved the speed of the select/sort implementation
		of the list of games.
	) Added the `loop' option to continuosly play the MNG files.
	) In a single column mode the left and right events are now like
		page up and page down events.
	) Now the idle event never interrupt a long game clip or sound.
	) If the generic emulator path is empty the idle start event
		is automatically disabled.
	) When returning from a idle start event the game clip and sound
		aren't played.
	) Added a new `info_import' option to display arbitrary information
		for every game.
	) You can now specify more than one `desc_import', `type_import'
		and `group_import' options.
	) Added in the contrib/ dir the hiscore.ini file to display the top
		hiscore of the MAME games.
	) Various fix at the `advcfg' utility.
	) Fixed some problems of the fullscreen use of the sdl system.
	) Various bug fixed.

AdvanceMENU Version 2.0.0 2002/08
	) Added the `sdl' system which uses the libSDL graphics
		library. This system enable the use of the program
		in a Window Manager.
	) Added the HOST_SYSTEM section in the makefile.
	) Added the `windows' target in the Makefile. It use the `sdl'
		system.
	) Added the `device_sdl_fullscreen' option to force the use of
		a fullscreen mode with the SDL library.
	) The key specification in the config file are now by
		name and not by scancode. The list of available
		names is in the advmenu.txt file. This makes
		the configuration file portable.
	) Added the support of the Windows MAME emulator with the
		emulator type `mame'. The old DOS MAME is still
		supported with the `dmame' emulator type.
	) Added the support of the XMAME emulator with the
		emulator type `xmame'.
	) The Linux version now searchs the emulators in the whole
		PATH if a relative path is given.
	) Added support for the 32 bit graphics output.
	) The article `The' on the game names is put at the end
		of the name.

AdvanceMENU Version 1.19.1 2002/07
	) Fixed a bug when forcing the use of the vbeline/3dfx driver.
	) Enabled the interlaced modes with the 3dfx svgalib driver.

AdvanceMENU Version 1.19.0 2002/06
	) Upgraded at the SVGALIB 1.9.15 library.
	) Added the joystick calibration on the main menu.
		This fixs the inverted movement with some
		joysticks on the DOS version.
	) The third mouse and joystick button now open the
		main menu.
	) The `idle_screensaver_preview play' option now
		play only the animated snapshots. The static
		snapshots are ignored.
	) A better random selection of the screensaver
		snapshot.
	) In the Linux version if the HOME environment variable
		is not set all the files are read and written on the
		PREFIX/share/advance directory.
	) Minor bugfix.

AdvanceMENU Version 1.18.3 2002/05
	) Fixed an hang bug when no games are listed.

AdvanceMENU Version 1.18.2 2002/05
	) Upgraded at the SVGALIB 1.9.14 library.

AdvanceMENU Version 1.18.1 2002/04
	) Added the support for AdvancePAC with the emulator
		type `advpac'.
	) Added the `advcfg' utility in the distribution
		package.

AdvanceMENU Version 1.18.0 2002/04
	) Added the list mode `text'. It displays only the
		game names without any image.
	) Added description import from NMS files with the
		option `desc_import'
	) Changed the behaviour of the `*_import' options.
		Now the imported values never overwrite the
		user choices.
	) Upgraded at the SVGALIB 1.9.13 library.
	) Upgraded at the Allegro 4.01 library.
	) Fixed the mouse deinitialization.
	) Better screen separation with the list_mixed and
		full_mixed modes in video modes without
		square pixel.
	) Fixed the use of absolute path without the drive
		specification.
	) Fixed the aspect of the marquee images with
		vertical monitors.
	) Minor bugfix.

AdvanceMENU Version 1.17.5 2002/04
	) Fixed the `emulator_rom_filter' option when more than
		one pattern is specified.
	) Fixed a crash bug with roms on the root directory.
	) Fixed a potentially crash bug on the MNG playing on
		low resolution modes.
	) The fast movement ins/del keys now consider the games
		with a digit as the first char in the description
		of the same category.
	) Minor bugfix.

AdvanceMENU Version 1.17.4 2002/04
	) Added the `preview_expand' option to better control the
		aspect of the images.
	) The sound_foreground_begin effect is now correctly played.
	) Added the option `idle_screensaver_preview play'.
	) Fixed the `generic' emulation support.
	) Fixed some minor issues on the image preview.
	) Fixed the delete commands.
	) Minor bugfix.

AdvanceMENU Version 1.17.3 2002/03
	) The the first frame of the .mng files is used as static
		snapshot if none .png file is found.
	) Better screen split on the list_mixed and full_mixed modes.
	) Added the `resolution' sort order.
	) Better .mng file support. The files saved with
		"Jasc Animation Shop" should work.
	) The game description is now displayed in two positions in
		the screen saver.
	) The "event_emulator" key now switch to the next emulator
		without the menu. You can still use the
		emulator menu calling it from the main menu.
	) Added the `misc_quiet' option to disable the text message :
		"AdvanceMENU - Copyright (C) 1999-2002 by..."
	) Fixed a crash bug on the rotated modes when a MNG end it's
		playing period.
	) Fixed a crash bug when no video modes are available.
	) Added support for mp3 files with a RIFF header.
	) Minor bugfix.

AdvanceMENU Version 1.17.2 2002/03
	) Fixed the output of the `color' options with the
		`-default' command.

AdvanceMENU Version 1.17.1 2002/03
	) Fixed a bug importing category files.
	) Fixed a compiling bug for gcc 2.95.x.
	) Added the sound_buffer option to control the size of
		the sound buffer.
	) Increased the sound interrupt frequency.
	) Added the documentation of the `color' options.
	) Added the output of the `color' and `event_assign' options
		with the `-default' command.

AdvanceMENU Version 1.17.0 2002/02
	) Added MNG animated snapshot !
	) Faster PNG decoding.
	) Added 3 new fonts in the contrib dir: amiga, c64_8x16 and
		c64_8x8 [Flemming Dupont].
	) Bugfix for the case of the game descriptions.
	) Bugfix for the use of " with path in the config file.
	) The snapshot directories listed in the emulator configuration
		file now have the precedence over the directories
		specified in the advmenu.rc file.
	) Readded the sound interrupt at the SEAL sound driver.
		It uses a lower frequency than the previous version
		(18 Hz instead of 100 Hz). So, please report feedback.

AdvanceMENU Version 1.16.1 2002/01
	) Added support for AdvanceMESS with the `advmess' type.
		The support for AdvanceMESS is better than the MESS
		support because AdvanceMESS output more information
		with the -listinfo option.
	) The linux config directories are now named `$home/.advance'
		and `$prefix/share/advance'.
	) Fixed and renamed the options `group/type_inport' in
		`group/type_import'.
	) All the MESS/AdvanceMESS software is always listed also if
		you choose to display only the parent rom.
	) When you run a clone pressing F12 the MESS/AdvanceMESS
		software is not listed.

AdvanceMENU Version 1.16.0 2002/01
	) Added a Linux port
	) Added a new emulator type `advmame' which support the new
		AdvanceMAME configuration format (from 0.56.2).
	) Changed the name of the configuration file. Now it's
		named `advmenu.rc'. Check the `release.txt' file for
		the conversion instruction.
	) Removed the `like_emulator' value for all the options.
		You should now explicitly configure them.
	) Now you can specify an arbitrary game description with
		the `game' option. Check the `advmenu.txt' file.

AdvanceMENU Version 1.15.2
	) Solved a bug that prevented the read of the vbeline_* options.

AdvanceMENU Version 1.15.1
	) Added the new `[video] pclock' option like AdvanceMAME. Add it
		in your configuration file.
	) Minor bugfix

AdvanceMENU Version 1.15.0
	) Added the `emulator_roms_filter' option to filter the
		roms files for the `generic' emulator.
	) The `coin' and `time' values of a parent games count also
		the clones if they are not displayed.
	) Added a new event `shutdown' which exit immeditially and
		shutdown the PC. (default CTRL+ESC)

AdvanceMENU Version 1.14.2
	) Removed the `cfg_scan' option. Now it's always active and
		really fast! The coins number is cached in the 
		mm.cfg file and the .cfg files are read only the
		first time.
	) Solved the `default_mode' selection bug.

AdvanceMENU Version 1.14.1
	) Added the emergency signal handlers like AdvanceMAME
	) Minor bugfix

AdvanceMENU Version 1.14.0
	) The same video driver and modelines changes of AdvanceMAME 0.53
	) Added the option video_depth to control the depth of the
		video mode used.
	) The default video mode is now 1024x768 at 16 bit.
	) The `format' option if present is used to create a video
		from scratch like AdvanceMAME.

AdvanceMENU Version 1.13.0
	) Solved the empty list problem with the -default option
	) Changed the default font.
	) Added the `lock' option to prevent any modification at the user
		interface. You can activate it from the config file,
		from the runtime menu or pressing the lock key (scroll-lock).
	) Added the `coin' sort key.

AdvanceMENU Version 1.12.0
	) Added the support for a `generic' emulator type. With this you
		should be able to use AdvanceMENU with any other emulator.
		Check the `mm.txt' file in the `emulator' section.
	) Added a selection by Vertical/Horizontal
	) Solved the `hidden cursor' bug
	) The `<undefined>' selection is now saved
	) Support for additional command line argument in the emulator call
	) Solved a crash bug if no snapshot are available on the slideshow
	) Added the game name on the slideshow
	) The `enter' press on the slideshow now start the game
	) Added the -quiet option to prevent the startup messages.

AdvanceMENU Version 1.11.0
	) Added external import of the CATLIST file
		[http://www.mameworld.net/catlist/]
		and of the `MacMAME Grouping Files'.
		[http://www.tznet.com/cmader/categories.html].
		See the new options `type_import' and `group_import'.
	) Added two new preview modes `list_mixed' and `full_mixed'
		that show the snap, title, flyer and cabinet images
		together.
	) Added the new option `config' to select when and if the
		runtime configuration  changes should be saved
		or restored.
	) Added inclusion/exclusion attribute for the `Deco Cassette'
		and `PlayChoice-10' games.
	) Added the `idle_screesaver_preview' option `off' to
		shutdown the monitor in the screensaver mode
		using the VESA/PM services if avaliable.
	) Removed the auto cutoff of the preview images.
	) The icons and marquees images are now selectable with the
		new two modes `tile_icon' and `tile_marquee'.
		The older previous options `preview_icon' and
		`preview_marquee' don't exist anymore.

AdvanceMENU Version 1.10.0
	) The "neogeo" selection filter is now working
	) Faster blit for MMX processors
	) Progressive screen update
	) Added a new fullscreen mode
	) Added the sort/mode/preview/attribute items in the main menu
	) Added the sort key in the status bar and in the game list
	) Solved a bug on the tiled modes with vertical monitor.
	) You can select the preview to use in the slide show mode with the
		new option "idle_screensaver_preview"

AdvanceMENU Version 1.9.0
	) Added a new event `rotate' to rotate the screen at runtime
		for cocktail machines. This event is mapped for default
		on the `0 PAD' key
	) Added support for zipped collections of images and sounds.
		Simply copy the .zip archives with the .png,.pcx,.ico,
		.mp3,.wav in the various snap, flyers... directories.
		For best performance the .zip archives containing
		`.png' and `mp3' files should be compressed in
		`storing' mode (without compression). These file
		formats are already compressed and don't need to
		be compressed another time.
	) Added support for 4 bit .ico files
	) Long icon titles are now truncated and not skipped
	) Solved the crash bug with empty preview images.
	) The first preview image found is used. Not the last.
	) Changed the default value of `cfg_scan' and `crc_scan' to `no'
	) Added the `-default' option to force the creation of a
		default `mm.cfg'

AdvanceMENU Version 1.8.0
	) Added the new option `preview_adjust' to control how
		the images is stretched.
		Use `preview_adjust square' if you like images
		with the correct aspect ratio.
	) The MP3 player is now able to skip the ID3 headers
	) The `Run game' message is now customizable with the
		option `msg_run'.
	) Added the `preview_default_*' options to specify default images.
	) The mouse can be disabled with the option `mouse no'
	) The background sound now continues also in the screensaver mode.
	) Added a set of .cfg files for the most common TVs and Monitors.
	) Some changes at the timer used for the `idle_start'
		and `idle_screensaver' options.
	) Changed the default value of the option "sound_interrupt"
		to "yes". Enable it if your sound is choppy.
	) Simplified the use with a normal PC monitor. If no video
		hclock/vclock or video modes are defined a default
		PC multisync monitor is assumed with the use of the
		standard VGA and VESA modes enabled.

AdvanceMENU Version 1.7.0
	) The screen saver now doesn't ignore the `exit_press' option
	) Added trackball support. See the new `mouse_delta' option

AdvanceMENU Version 1.6.0
	) Added coinage support for mame. You can disable it with
		the cfg_scan option (to speedup the menu loading time)
	) Removed the automatic creation of the info.txt file if the
		emulator executable is a batch file
	) Added the option "exit_press 0" to completely disable the exit.
	) The snapshot displayed in the listing mode without the clones is
		now the snapshot of the effective clone run if the parent
		set is not runnable.
	) All the file name matching is now case insensitive.
	) Added the new type of snapshot named `titles'. It works
		like `flyers' and other images.
	) Some changes at the sound options. Check the mm.txt files
		for the sound_foreground_* and sound_background_*.
	) Renamed `video_mode_switch' in `video_mode_reset'.

AdvanceMENU Version 1.5.1
	) Corrected the option `event_assign'
	) Corrected the option `mode_skip'
	) Corrected some problems with `"' in the `mm.cfg'
	) Changed the game sort to be case insensitive

AdvanceMENU Version 1.5.0
	) Added support for Trident cards using the new video
		driver of VSyncMAME written by Saka.
		Use the "vbeline_driver=vbe3" option to revert to the
		old behavior.
	) Documentation update. Now in sync with the program.
	) Better icon support. Now you need to press `space' to activate it.
	) Added the option `icon_space' to control the space
		between icons.
	) The VBE BIOS is now initialized after every emulator start.
		This solves problems with some bogus VBE BIOS (3dfx).
	) Added the `event_alpha' option to disable the fast moving
		with alphanumeric keys. This may be required with
		keyboard encoders when alphanumeric keys are used for
		others pourpuse.
	) Added marquee support with the new mame.cfg option `marquees'.
		Press `space' to activate it.
	) Added multi emulator support. Now you can list all the games
		of various emulators together. See the new mm.cfg
		`emulator' option.
	) Added `raine' support.
	) Added PCX support. Used by `raine' for the snapshots.
	) Added a new emulator menu (key F6) and the `emulator_include'
		option to select that emulators to show
	) Added the a new global menu (key ~) on which you can select
		other menus
	) Added the option `video_font'
	) Added the option `mode_skip'
	) Added the option `video_config'
	) Added the option `video_mode_switch'
	) Added the option `joystick'
	) Added the option `joystick_config'
	) Added the option `video_orientation'
	) Added the option `video_gamma'
	) Added the option `video_brightness'
	) Added support for `not', `or', `and' key definitions with the
		new option `event_assign'

AdvanceMENU Version 1.4.0
	) Added a new option `event_mode fast' to select if to
		wait or not for a complete screen redraw
	) Added a new option `event_repeat' to select the repeat rate
		of the events
	) Automatic exclude of flyers/cabinet modes if no images are found
	) Added some new tiled modes `tile_big', `tile_enormous'
		and `tile_giant'
	) More fast tiles scrolling
	) Added a flashing cursor
	) Added a new option `video_size' to select a generic video mode
		size without the use of the `mv' utility
	) Added sound support. Play `wav' file for some action and `mp3'
		files in background. See the new mm.cfg `sound_*' options.
	) Added per game sounds support. The mame.cfg option `snap' and
		`altss' are used to found `mp3' or `wav' sounds with the
		same name of the game to play when the cursor move
		to a specific game.
	) Added support for `ico' (Windows Icons) snapshot.
		These images are only used in the bigger tile modes if
		available.
		The `ico' files are read using the new mame.cfg `icons'
		option. 

AdvanceMENU Version 1.3.0 (0.37b8.0) 2000/10
	) Better support for MESS
	) Corrected some bugs related to long filenames and filenames
		with spaces or dots
	) Added the command menu (F8) to do some file operations
	) Added support for "cabinets" snapshot
	) Corrected the proportional font display
	) Added a fast move to the next system with the INS and DEL keys.

AdvanceMENU Version 1.2.0 (0.37b7.0)
	) Added support for MESS
	) Added support for loading GRX fonts
	) Added support for proportional fonts
	) Added some new GRX fonts in the contrib/fonts directory,
		you can found the complete (big) set at
		ftp://x2ftp.oulu.fi/pub/msdos/programming/djgpp/v1tk/cbgrf103.zip

AdvanceMENU Version 1.1.0 (0.37b6.0)
	) Corrected the timing in the `idle_start' option, now
		the timer also reset if the user exit from `mame'
		pressing ESC (requires AdvanceMAME 0.37b5.1 or greater)
	) When the clone aren't listed and you start a game not emulated
		correctly the best clone is run instead. If you want
		to force the run of the bad game use the F12 command.
	) If the screenshot isn't found it's searched the screenshot of any
		avaliable clones or parent games.
	) The key for selecting/deselecting all in the menu are now 'INS'
		and 'DEL' and are remappable with the cfg file.
	) Added the preview of the flyers pics. See the new options 'flyers'
		in and press SPACE to switch from snap to flyers
	) New look
	) Colors adjustable in the file `mm.cfg'
	) Automatic cut of any symmetric left/right or up/down monochromatic
		band in the previews
	) Compiled with the latest Allegro library to support all the
		newer joysticks
	) Now works also if some games are missing in the info.txt

AdvanceMENU Version 1.0.0 (0.37b5.0) 2000/08
	) Added a new menu mode to the utility `mm' which display
		a lot of tiled preview at the same times. Press TAB to
		cycle from one mode to another.
	) Now the utility `mm' is avaliable also as a standalone project
		named AdvanceMENU

