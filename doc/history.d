Name
	history - History For AdvanceMAME/MESS

AdvanceMAME/MESS Version 3.10 WIP
	) Added support for Teeter Torture [arcadez2003]
	) Hooked up Bubble Bobble Protection MCU [arcadez2003]
	) Get DJ Boy playable [arcadez2003]
	) Get B-Rap Boys, Blood Warrior and Shogun Warriors playable [arcadez2003]
	) Hook up Fire Shark / Vimana (Sound MCU's) [arcadez2003]
	) Support Wonder Boy in Monsterland English Virtual Console [arcadez2003]
	) Support Opa Opa unprotected set and fix Tetris System E 2 player mode [arcadez2003]
	) Support Cabal Joystick Version [arcadez2003]
	) Fix Night Slashers sound [arcadez2003]
	) Fix Sunset Riders protection [arcadez2003]
	) Add support for Bang Bang Busters and Choutetsu Brikin'ger / Iron Clad [arcadez2003]
	) Support the only working version of Oriental Legend Special [arcadez2003]
	) Prevent Gun Master from randomly hanging and add 2p buy in via the inputs [arcadez2003]
	) Improved automatic joystick button mapping for games with gear shift.

AdvanceMENU Version 3.10 WIP
	) Added the new 'ui_menu_font' to change the font for the online menus.

AdvanceMAME/MESS Version 3.9 2018/09
	) Fixed input games with a relative input when controlled with
		an absolute controller. Like 'cabal' or 'offroad' when
		controlled with a joystick.
	) Added the new 'display_artwork_magnify' to use a bigger screen
		size when the game has an artwork to display.
	) Included the cheat.dat resource in the main distribution.
	) Fixed loading files with an absolute path, like with misc_cheatfile.

AdvanceMENU Version 3.9 2018/09
	) Added a new 'menu' option at the 'misc_exit' configuration to
		have a confirmation menu before quitting.
	) Fixed the creation of the default advmenu.rc, avoiding the "volume"
		syntax error.

AdvanceMAME/MESS Version 3.8 2018/06
	) Change the default 'sync_resample' mode to 'internal' to all games
		using the pokey sound chip. This avoid crashes and provide
		a better sound audio. One example is 'starwars'.
	) Don't update input configurations that are not changed at runtime.
	) Added a new 'config' option to allow to decide if the .rc configuration
		should be saved at exit or not. If saved, they are written
		in a resiliant way, able to resist to unexpected power-downs.
	) Fixed font size computation. Now the menu always has the same size.
	) In AdvanceMESS added a new 'misc_ui' option to configure the keyboard
		emulation. Note that the default is now to have partial emulation
		by default to allow the user to control the emulator user
		interface.
	) In AdvanceMESS added a new 'ui_keyboard' input to show a runtime menu
		to simulate some keypress.

AdvanceMENU Version 3.8 2018/06
	) Added support for configuring joystick and mouse buttons in the 'event'
		configuration option.
	) Added the 'display_resizeeffect' option like in AdvanceMAME to scale
		screenshots.
	) In the screensaver if the clip is too short, don't restart it when
		it finishes, but switch to the next one.
	) In 'lock' mode is still possible to shutdown, if the 'misc_exit'
		option allows it.
	) Added a new 'advblue' tool able to pair and connect automatically
		bluetooth joysticks and gamepads. This is Linux only.
	) Added a new 'ui_autocalib' option to automatically start a joystick
		calibration menu if no joystick is detected. This works better
		with the Linux event driver, with an immediate notification.
		With other drivers it checks only when the idle screensaver
		starts.
	) Other configuration options added: 'ui_topname', 'sort smart_time',
		'ui_text/bar_font', 'ui_outline', 'skip_horz' and 'skip_vert'
	) The configuration .rc file is wrote in a resiliant way, able to
		resist to unexpected power-downs. Take care that this is
		automatically disabled if the .rc is a symbolic link.
	) Always wait for vsync to avoid to use 100% CPU.
	) Added a new 'exit' event to separate the exit action from the
		generic esc one. Ensure to change also misc_exit to make this
		command to work.
	) Avoid to flash the cursor if it has equal foreground and background
		color.

AdvanceMAME/MESS Version 3.7 2018/02
	) In the Exit menu added new Load/Save/Reset commands.
		The Load and Save use the state file at position '0'.
	) Added the game uccopsar, "Undercover Cops - Alpha Renewal Version".
		You can use the MAME rom.
	) Fixed some issues in the z80 emulator with modern gcc compilers
		in the x86 32 bits platforms.

AdvanceMENU Version 3.7 2017/12
	) Added detection for mame64.exe and mame64 executables for the
		autoconfiguration.
	) Renamed 'sdlmame' emulator type in Linux to 'mame'. The old 'sdlmame'
		name is still supported.

AdvanceMAME/MESS Version 3.6 2017/12
	) Fixed Linux ALSA sound jitter increasing the number of periods.
	) Fixed screen size computation using backdrops.
	) Disabled by default in the source distribution the MIPS DRC emulator
		because it makes various games to crash. Like biofreak, blitz,
		calspeed, carnevil, kinst and others...
		It was already disabled in the binary releases from version 3.0.

AdvanceMENU Version 3.6 2017/12
	) Fixed loop for clips in list mode.

AdvanceMAME/MESS Version 3.5 2017/06
	) Fixed led control for the Linux event keyboard interface.

AdvanceMENU Version 3.5 2017/06
	) Added a new option 'ui_scrollbar' to disable the lateral scroll
		bars.
	) Added a new option 'ui_name' to disable the display of the game
		name in the screensaver.

AdvanceMAME/MESS Version 3.4 2017/03
	) Add supports for up to 8 players. Before it was fully
		supporting only 4 players.
	) Fixed handling of multiple identical devices in input_map[].
	) Don't lose the device name in input_map[] if run with the device
		disconnected.

AdvanceMAME/MESS Version 3.3 2017/02
	) Fixed handling of input_map[] definitions [Vincent Hamon].
	) Fixed crashes in games xmen6p and xmen6pu.

AdvanceMAME/MESS Version 3.2 2017/01
	) The prebuilt binary for Raspberry is not linked anymore with SDL.
	) Fixed a terminating hanging condition when dealing with the
		Raspberry VideoCore. There are some hidden timing
		dependencies that could cause VideoCore to misbehave.
	) Inverted the polarity of hsync/vsync in Raspberry when using
		hdmi_timings. In hdmi_timings 1 means "inverted polarity",
		and then "-hsync/-vsync".
	) Fixes aspect ratio of Sega System 32 and most Neogeo games.
	) Improved the support for Sega Megadrive and
		Sega Master System recognizing more software formats.
	) In the input definitions support the use of the device name
		to avoid issues when devices are reordered.
		You can use the advk, advj and advm tools to get the
		identifier names to in the 'keryboard[]', 'mouse[]' and
		'joystick[]' input definitions.
	) Disabled the SDL2 system keyboard hotkeys to avoid to lose focus.
	) Fixed the loading of software images from the UI file browser.

AdvanceMENU Version 3.2 2017/01
	) Always prints the system name, even if it's not runnable, if it
		has dependent software.
	) Added a filter for duplicated games. For example,
		you can use it to show only the MAME games not present in
		AdvanceMAME.
	) Added support for reading MNG files generated by MAME.

AdvanceMAME/MESS/MENU Version 3.1 2017/01
	) Added support for SDL2. It's the new default in Linux and Windows.
		This results in better performance and better VSync support
		when running in a window manager environment like X Window
		or Windows. It also enables again the RGB effects.
		The exception is Raspberry where SDL1 still provides better
		performance in the X Window environment, and it's still the
		default option.
	) More reliable Raspberry video mode setting when using programmable
		modes. It now uses the Raspberry VideoCore libraries to
		control better the system.
	) More precise VSync synchronization, avoiding any missing frame.
	) In AdvanceMESS fixed the loading of disk image for the TI99-4A
		system.
	) In AdvanceMAME marked 'retofinv', 'svc', 'prmrsocr' and 'viostorm'
		as not working to trigger the use of the working clones.
		Also added the game 'mp_shnb3'.
	) Fixed the UI flickering and a potential crash when SMP is active.
	) The Linux joystick event and raw interface now support up to 32
		button. Before it was only 12.
	) If SMP is active, don't automatically disable the resize
		effect if the game is too slow. This gives a more
		stable 'auto' effect.
	) Changed the 'sync_resample auto' to be 'emulation' instead
		of 'internal'. This give more stable sound when
		the game emulation is too slow.
	) The 'display_magnify auto' option now takes into account
		the monitor limits to avoid over scaling.
	) Restored the functionality of "-output fullscreen" option.
		It wasn't able anymore to find video modes.
	) Added support running from a SSH shell.
	) Removed the 'lq' effect. It was not really used.
		The 'scale2x' and 'scale2k' are faster and better.
	) The RunClone menu now excludes preliminary clones if they
		are already filtered out in the main list.

AdvanceMAME/MESS/MENU Version 3.0 2016/12
	) Now the AdvanceMAME package contains AdvanceMAME,
		AdvanceMESS and AdvanceMENU.
	) Customized for Raspberry Pi fixing various issues
		and improving performance, functionality and
		documentation
		Check the install.txt file for some more information
		about the use with a Raspberry Pi.
		The most important note is to run the Advance programs
		directly from the Linux Console and outside the
		X-Window graphics environment to be able to use the
		Raspberry hardware acceleration.
	) Improved synchronization to remove tearing with the Linux
		FrameBuffer.
	) Added a new category.ini installed automatically, that
		is used automatically by AdvanceMENU.
	) Added a new 'display_aspect' option that replaces
		the old 'display_aspectx' and 'display_aspecty'.
		This one allows aspect auto-detection with 'auto'.
		You have to manually adjust the advmame.rc file if you
		are using the old ones.
	) Changed the default of 'display_expand' to 1.25.
		This value allow to fill better a wide screen with
		old games. If you like more correct aspect, select 1.0.
	) Add a new AdvanceMENU 3x2 tile mode called "tile_tiny".
	) Better autoconfiguration for AdvanceMENU to be able to detect
		and configure AdvanceMAME and AdvanceMESS at the first run.
	) Change the default AdvanceMENU main and screen saver mode to animate
		all the clips.
	) In Linux if the system time is wrong ensure that the generated XML
		list files are not updated every time.
	) Added new SSE2 blitters that replace the older MMX ones

AdvanceMAME/MESS Version 1.5 2016/11
	) Imported the Toaplan driver from ThunderMAME32Plus-v0.106X.
		This adds sound support with samples at the games:
		'batsugun', 'dogyuun', 'ghox', 'samesame', 'tekipaki',
		'vfive' and 'vimana'.
	) Imported the CPS3 driver from ThunderMAME32Plus-v0.106X.
		This adds the games: 'sfiii', 'jojo' and 'warzard'.
	) Fixed a crashing issue in 68000 games, like Altered Beast.
	) Added knocker support to Q*bert. You should see a keyboard
		led on when the knocker is active [hainet].
	) Set the SCHED_FIFO scheduling policy with sched_setscheduler().
		This is reported to improve performance on Raspberry
		Pi 2 boards.
		Note that this ha effect only with root permission.
		You can see in the log for "os: scheduling".
	) Windows binaries built with MingW 4.9.3 using the MXE cross compiler at
		commit 62bcdbee56e87c81f1faa105b8777a5879d4e2e with targets
		i686-w64-mingw32 and x86_64-w64-mingw32 and optimization -O2.
	) DOS binaries built with DJGPP 4.8.5 from
		https://github.com/andrewwutw/build-djgpp

AdvanceMENU Version 2.9 2016/11
	) Extended display_size option to allow to select the mode height.
	) Windows binaries built with MingW 4.9.3 using the MXE cross compiler at
		commit 62bcdbee56e87c81f1faa105b8777a5879d4e2e with targets
		i686-w64-mingw32 and x86_64-w64-mingw32 and optimization -O2.

AdvanceMAME/MESS Version 1.4 2015/08
	) Restored the missing keypress of coin1 broken on v1.3.
	) Imported the Cave PGM driver from ShmupMAME v3.0b
		with the new games 'ket', 'ddp3' and 'espgal'.
	) Fixed build issue on AdvanceMESS about discrete.c file.

AdvanceMAME/MESS Version 1.3 2015/06
	) Added support fo Haiku OS.
	) Included free ROMs gridlee, polyplay and robby.
	) Trick to reduce the input lag by one frame [Manlio De Pasquale].
	) Better effect selection if display_resizeeffect is 'auto'.
	) Changed the default of "magnify" to auto.
	) Fixed manpages format.

AdvanceMENU Version 2.8 2015/06
	) Added support fo Haiku OS.
	) Compatible with AdvanceMESS 1.3.

AdvanceMENU Version 2.7 2014/03
	) Fixed a hang condition at termination when started directly from xinit.
	) Fixed manpages format.

AdvanceMAME Version 1.2 2012/12
	) Fixed compilation with modern compilers.
	) Fixed the "alsa" audio driver in Linux. In modern distributions it was generating no sound.
	) Fixed a crash at exit when SMP is enabled.
	) Added a new option display_magnifysize to define the area to target with display_magnify auto.
	) Renamed the 'scale' video effect to 'scalex'.
	) Added the new 'xbr' video scaling effect. Better than 'hq'.
	) Added the new 'scalek' video effect. Better than 'scalex' and faster than 'xbr'.
	) If display_resizeeffect is auto, the default effect is now 'xbr', but it's decreased at runtime
		to 'scalek', or 'scalex' if speed is required.
	) Removed blitting cache optimization. With modern caches it's not needed anymore.
	) Changed default doc/man dirs to prefix/doc and prefix/man.
	) Now in git repository.
	) Removed DLL files from the Windows distribution. The new compiler uses static libraries.
	) Compiled with SDL-1.2.14, zlib-1.2.5, expat-2.0.1, freetype-2.4.4, pthreads-w32-2-8-0.

AdvanceMENU Version 2.6 2012/12
	) Fixed the "alsa" audio driver in Linux. In modern distributions it was generating no sound.
	) Support 64 bits .ZIP archives.
	) Support $HOME variable in directories specification for sdlmame.
	) For MAME bioses, devices and other not runnable roms are not listed anymore.
	) The new screen saver modes 'shutdown' and 'exit' works also with lock activated.
	) Added support for importing CatList files.
	) Snapshot images in .ZIP archives are now read also ig they are in sub directories.
	) Renamed the old "ini" import format to "catver" to avoid confusion with the new "catlist".
	) Removed xmame support.
	) Removed DLL files from the Windows distribution. The new compiler uses static libraries.
	) Compiled with SDL-1.2.14, zlib-1.2.5, expat-2.0.1, freetype-2.4.4.

AdvanceMAME Version 0.106.1 2008/12
	) Fixed the black screen bug with paletteized games.
	) Fixed the computation of the font size of the menu for vertical games.
	) Added support for 'lq' and 'hq' resize mode using the 'overlay' output mode.
	) Added support for 'scanline' effects using the 'overlay' output mode.
	) Improved the cache locality of the blitting algorithm. Not it operates on segments of row,
		processing one segment up to the final stage before processing the next one.
	) The default output mode in Windows and X is now 'overlay'.
	) The default resize effect is now 'fractional' instead of 'integer'.
	) The 'overlaysize' option now has the default 'auto' which uses the current video mode.
	) New Windows icon at high resolution.
	) Ignored WINKEYS in Windows if input_hotkey is set.
	) The SDL library now force the video driver 'directx' as default.
		Required because from SDL 1.2.10 the default is the slow 'windib'.
	) Windows binary is provided with SDL.dll 1.2.13.
	) Added multithread support for Windows using the pthread-win32 library.

AdvanceMENU Version 2.5.0 2008/12
	) Fixed the recognition of the BIOS roms.
	) Smarter error detection of damaged zips.
	) Added the option 'exit' and 'shutdown' at the screensaver.
	) New Windows icon at high resolution.
	) Added automatic detection of mamepp.exe for Windows.
	) Added automatic detection of sdlmame for Windows and *nix.
	) Added a splash screen in Windows.
	) The 'overlaysize' option now has the default 'auto' which uses the current video mode.
	) Windows binary is provided with SDL.dll 1.2.13.

AdvanceMENU Version 2.4.14 2007/01
	) Fixed compilation problem with gcc 4.x.x.

AdvanceMAME Version 0.106.0 2006/06
	) Based on MAME 0.106

AdvanceMAME Version 0.104.0 2006/02
	) Based on MAME 0.104

AdvanceMAME Version 0.102.1 2005/12
	) Added in the `contrib' directory a test version of cwsdpmi
	which solves some slowdown problem in DOS with system with
	more than 512 MB of memory.

AdvanceMESS Version 0.102.0.1 2005/12
	) Fixed the loading of image files.

AdvanceMENU Version 2.4.13 2005/12
	) Fixed the ui_help and ui_exit option in the DOS and
		Windows platforms.


AdvanceMAME Version 0.102.0 2005/12
	) Fixed some bugs for the 64 bits platforms. It works now.
	) Fixed a problem mapping the video memory
		in the Linux FrambeBuffer driver.
	) Fixed the conditional compilation of the advv and advcfg
		utilities.
	) Updated zlib to version 1.2.3.
	) Fixed some keyboard input issues with the text mode
		utilities in Windows.
	) Added leds support at the `raw' and `event' keyboard
		linux drivers.
	) Fixed the `uninstall' make target.
	) Increased the max number of joystick buttons to 64.

AdvanceMENU Version 2.4.12 2005/12
	) Fixed some bugs for the 64 bits platforms. It works now.

AdvanceMESS Version 0.102.0.0 2005/12
	) Based on AdvanceMAME 0.102.0.

AdvanceMAME Version 0.101.0 2005/10
	) Fixed the crash problem without the -quiet option.

AdvanceMAME Version 0.100.0 2005/10
	) Fixed the recognition of the ActLabs guns in the Linux
		version.
	) Removed the `misc_historyfile' and `misc_infofile' like
		the official version.
	) Updated the FAQ [Mikkel Holm Olsen].

AdvanceMENU Version 2.4.11 2005/10
	) Added a new `mute' event. Default to period key on the keypad.

AdvanceMESS Version 0.99.0.0 2005/08
	) Based on AdvanceMAME 0.99.0.

AdvanceMAME Version 0.99.0 2005/08
	) Based on MAME 0.99

AdvanceMAME Version 0.97.0 2005/06
	) Improved the precision of all the joystick drivers.
		Now a scale of 65536 values is used.
	) Added support for multiple lightgun in Windows XP
		using the new `lgrawinput' joystick driver.

AdvanceMENU Version 2.4.10 2005/06
	) Improved the precision of all the joystick drivers.
		Now a scale of 65536 values is used.

AdvanceMESS Version 0.96.0.0 2005/05
	) Based on MAME 0.96 and AdvanceMAME 0.96.0.

AdvanceMAME Version 0.96.0 2005/05
	) Improved the support for low clocks of the SVGALIB
		nVidia driver for DOS and Windows.
	) Readded in the DOS and Windows version the old
		SVGALIB drivers for GeForce and Savage boards.
		If you have problem with the new driver
		you can still use the old ones with the
		names "savage_leg" and "nv3_leg".
	) The `advv' and `advcfg' utilities are now compiled
		and installed only if needed.
	) Fixed the detection of the new `unichrome' driver in DOS.
	) Added support for multiple mice in Windows XP using the
		new `rawinput' mouse driver.
	) Added support for multiple mice in Windows 2000 using the
		new `cpn' mouse driver.

AdvanceMENU Version 2.4.9 2005/05
	) The directory is not chaged before running an
		emulator in the *nix platforms. Instead the
		full path name of the emulator is used.
	) Improved the support for low clocks of the SVGALIB
		nVidia driver for DOS and Windows.
	) Readded in the DOS and Windows version the old
		SVGALIB drivers for GeForce and Savage boards.
		If you have problem with the new driver
		you can still use the old ones with the
		names "savage_leg" and "nv3_leg".
	) The `advv' and `advcfg' utilities are now compiled
		and installed only if needed.
	) Fixed the detection of the new `unichrome' driver in DOS.
	) Added support for multiple mice in Windows XP using the
		new `rawinput' mouse driver.
	) Added support for multiple mice in Windows 2000 using the
		new `cpn' mouse driver.

AdvanceMESS Version 0.95.0.0 2005/04
	) Based on MAME 0.95 and AdvanceMAME 0.95.0.

AdvanceMAME Version 0.95.0 2005/04
	) Based on MAME 0.95
	) The `event' and `raw' mouse driver are now enabled
		in X. The default choice is for the `sdl' driver
		but you can force another driver if you want.

AdvanceMESS Version 0.94.0.0 2005/03
	) Based on MAME 0.94 and AdvanceMAME 0.94.0.

AdvanceMENU Version 2.4.8 2005/03
	) The help screen exits when the played mng clip is
		terminated.

AdvanceMAME Version 0.94.0 2005/03
	) The install target now create the bin/ directory if
		it's missing.
	) Updated to SVGALIB 1.9.20. This add support for the
		most recent Radeon and nVidia boards at the DOS
		version.

AdvanceMENU Version 2.4.7 2005/03
	) Added a new `ui_startup' option to display a clip/image
		at the menu startup.
	) Extended the `ui_help' and `ui_exit' option to
		allow to display also a clip.
	) Updated to SVGALIB 1.9.20. This add support for the
		most recent Radeon and nVidia boards at the DOS
		version.

AdvanceMAME Version 0.92.1 2005/02
	) Fixed the recognition of the `sync_resample' option.
		If you notify sound problem try setting
		`sync_resample emulation' in your configuration
		file.
	) In the `integer' and `mixed' modes, the integers
		magnification factors are chosen to ensure that
		the complete image is displayed.
	) Based on MAME 0.92u2000.

AdvanceMESS Version 0.92.0.0 2005/02
	) All from AdvanceMAME 0.92.0.

AdvanceMAME Version 0.92.0 2005/02
	) Added a new `system()' script function to execute
		a shell script.
	) Improved the behaviour of the analog input. You can now
		use both the keyboard and the joystick at the
		same time.
	) Enabled as default the use of the joystick and mouse.
		The `device_joystick' and `device_mouse' options
		have now the `auto' value as default.
	) Fixed the AC97 DOS audio driver.

AdvanceMENU Version 2.4.6 2005/02
	) Before running any external program all the
		privileges of suid/gid programs are dropped.
	) The option `display_restoreatgame' can now be customized
		for different emulator.
	) Fixed the AC97 DOS audio driver.

AdvanceMAME Version 0.90.0 2005/01
	) Fixed the recognition of some USB mouses and keyboards
		with the Linux event driver.
	) The USB devices in Linux now have always the same order
		in the input_map[] specifications.
	) Added a SVGALIB patch to fix the compilation with Linux
		Kernel 2.6 when you get the warning of missing
		"pci_find_class" function.
	) Added the missing control code `ui_edit_cheat' and
		`ui_toggle_crosshair'.
	) The linux keyboard event driver now recognize the
		special CTRL+C keypress.

AdvanceMENU Version 2.4.5 2005/01
	) Fixed the recognition of some USB mouses and keyboards
		with the Linux event driver.
	) The USB devices in Linux now have always the same order
		in the input_map[] specifications.
	) Fixed the creation of the .xml file in Windows with
		directory names containing spaces.
	) The terminal cursor is now disabled using the Linux
		SVGALIB driver with the restore_at_game
		option set to no.

AdvanceMAME Version 0.89.0 2004/12
	) Added a ncurses driver for text mode in Linux.

AdvanceMENU Version 2.4.4 2004/12
	) Fixed the screensaver and the alphanumeric selection.

AdvanceMESS Version 0.89.0.0 2004/12
	) All from AdvanceMAME 0.89.0.

AdvanceMENU Version 2.4.3 2004/12
	) The selection in the Groups/Types/Emulators
		listing menu is not more intuitive. If only
		an item is selected the cursor position
		make an implicitely selection.
	) Fixed the sensitivity of the joystick movements.
	) Added a ncurses driver for text mode in Linux.

AdvanceMAME Version 0.88.0 2004/11
	) Improved the timer precision in the Windows version.
	) Added support for scaling bitmapped fonts by an
		integer factor.
	) Added support at the Linux keyboard `event' driver to
		disable the hotkeys like CTRL+C and ALT+Fx.
	) Removed the generic requirement of the C++ compiler.
		It's now required only if you change the
		documentation.
	) Fixed the user interface in overlay video modes.
	) Added new configuration sections which allow to select
		different options based on the input device type.
		The new sections are named: `joy4way',
		`joy8way', `doublejoy4way', `doublejoy8way',
		`paddle', `dial', `trackball', `stick', `lightgun',
		`mouse'.

AdvanceMESS Version 0.88.0.0 2004/11
	) All from AdvanceMAME 0.88.0.

AdvanceMENU Version 2.4.2 2004/10
	) Added a new `input_hotkey' option to disable OS
		hotkeys like CTRL+C.
	) Fixed the game autostart and screensaver.
	) Fixed the display of the "Listing" menu.

AdvanceMAME Version 0.87.0 2004/09
	) Added support for translucency in the user interface
		with the `ui_translucency' option.
	) Added a new `device_video_clock' option which substitutes
		the old `device_video_p/h/vclock'. This allows
		a more complete specification of the allowed clocks
		if you use a multistandard arcade monitor.
	) The `x', `xclock' and `generate_*' modes of the `display_adjust'
		option now set a different horizontal resolution if
		the current resize mode doesn't allow a fractional
		vertical stretch. This always ensures a correct
		aspect ratio if you use the `none' and `integer'
		resize modes.
	) Improved the support for multiple input device. You should
		be now able to use a mouse and a joystick at the same
		time for the same player.
	) Fixed a library creation problem in MacOS X.
	) Fixed some issues using `not' input sequence in
		the .rc file.
	) Now if you unplug a input controller the settings in
		the .rc file are not removed.
	) Fixed the `input_name' option [wpcmame].

AdvanceMAME Version 0.86.0 2004/09
	) Added an audio spectrum analyzer in the "Audio" menu.
	) The sound normalization now uses the Fletcher-Munson
		"Equal Loudness Curve" at 80 dB to remove inaudible
		frequencies. The sound power is now also increased if
		required.
	) Fixed the compilation of the chdman utility.
	) Fixed the joystick recognition with some low level drivers.

AdvanceMESS Version 0.86.0.0 2004/09
	) All from AdvanceMAME 0.86.0.
	) Fixed the recognition of the `Start' and `Select' buttons.

AdvanceMENU Version 2.4.1 2004/09
	) Added support for translucency in the user interface
		with the `ui_translucency' option. Note that without
		a background image you will not see difference.
	) The sound volume is propagated to the emulator only
		in console mode (i.e. AdvanceCD).
	) When using a .bat file to start an emulator the .lst file
		is now used if the .xml file is missing.
	) Added a sort by emulator option.
	) Fixed the screensaver.

AdvanceMAME Version 0.85.0 2004/08
	) The modeline generation now always use horizontal
		values which are multiplier of 16 to solve
		memory corruption problems on some video cards.
	) Fixed a crash bug on vector games with artwork.
	) Added the `ui_color[*]' options to customize the user
		interface colors.
	) Added the `sound_adjust' option with an internal
		database with the volume correction for all
		the games.
	) Added a sound equalizer with the new `sound_equalizer'
		options.

AdvanceMESS Version 0.85.0.0 2004/08
	) All from AdvanceMAME 0.85.0.

AdvanceMENU Version 2.3.8 2004/08
	) Fixed the renaming of the AdvanceMESS snapshots,
		clips and sounds. It was broken on
		version 2.3.6.
	) Added support for specific emulator configuration.
	) New menu organization.
	) Added the new `ui_menukey' option to enable and disable
		the keyboard shortcuts in the menus.

AdvanceMAME Version 0.84.0 2004/07
	) Added a new rand(N) function at the scripts.
	) Added a new `misc_hiscorefile' to specify a different
		name of the hiscore.dat file.
	) Fixed the advfg utility to found always a valid
		text modeline.

AdvanceMAME Version 0.83.1 2004/07
	) Enabled the MIPS dynamic recompilation in DOS and
		Windows.
	) On Linux you can now specify relative directories
		in the `dir_*' options.
	) Fixed some problems enabling and disabling SMP at
		runtime.
	) Revised the `install' documentation file and the `video links'
		section of the web site.
	) Added the `misc_lang' option to select the game
		language.

AdvanceMESS Version 0.83.0.1 2004/06
	) The input port customizations are now saved
		for the current software only.
	) Fixed some issues saving options with software
		containing spaces in the name.
	) Fixed input saving for `coleco' system.
	) All from AdvanceMAME 0.83.1.

AdvanceMENU Version 2.3.7 2004/07
	) Fixed a Mac OS X crash problem with XML files with
		empty items.
	) Only the XML driver.status entry is now used to recognize
		the game emulation status. Game not marked as
		"preliminary" are considered working games.

AdvanceMENU Version 2.3.6 2004/07
	) Removed the copyright message box at startup.
	) On Linux you can now specify relative directories
		in the `emulator_altss/roms/*' options.
	) If no game is shown, the reason is explicated at the user.
	) Improved the xmame support.

AdvanceMAME Version 0.83.0 2004/06
	) Added the option "ui_speedmark" to enable or disable the on-screen
		speed red mark.
	) Fixed the option "sync_startuptime 0".
	) Added volume control at the `oss' audio driver.
	) Added support for the `mprotect' os call to allow execution
		of MIPS DRC also on systems with memory protected from
		execution.
	) Fixed the use of fgetc_unlocked() on Mac OS X 10.3.
	) Upgraded the DOS/Windows SVGALIB drivers at version 1.9.19.

AdvanceMESS Version 0.83.0.0 2004/06
	) All from AdvanceMAME 0.83.0.

AdvanceMENU Version 2.3.5 2004/06
	) Added volume control at the `oss' audio driver.

AdvanceMAME Version 0.82.0 2004/05
	) Added a new "Startup End" key (minus on numeric pad) to store
		in the configuration file the startuptime required for
		the current game. Simply press it when the game start.
	) The `sync_startuptime' now has an `auto' value which use an
		internal table to select the correct startup time.
	) The sound normalization is now computed using the normalized
		power of the audio and not the maximum value.
	) Added a new option `misc_freeplay' to enable the freeplay mode
		changing the game dipswitches.
	) Added a new option `misc_mutedemo' to mute the sound on game demo.
	) Improved the audio/video synchronization. It should fixes the problems
		on system which are not able to return the correct number of
		samples in the audio board buffer.
	) The `sync_resample' default value is now set to use
		internal re-sampling.
	) Fixed the loading of .rc files from the /etc directory.
	) Improved the loading speed of the history/mameinfo databases in
		Linux using the unlocked file API.
	) The history/mameinfo text is now wrapped to fit the whole screen.
	) Added a new `device_alsa_mixer' option to control the sound mixing.
		The default is to use an internal mixer to change the volume.
	) Documented in the `advdev.txt' file how use the ALSA `dmix' plug
		for a software mixing from multiple programs.

AdvanceMESS Version 0.81.0.1 2004/05
	) The customization of keyboard ports are now saved in the
		.rc file as input_map[key_*] options.
	) Added the MESS sysinfo.dat file in the distribution and
		fixed its loading.

AdvanceMENU Version 2.3.4 2004/05
	) The program doesn't try to effectively load the
		"none" image file used to disable the image
		loading.
	) Added a new `device_alsa_mixer' option to control the sound mixing.
		The default is to use an internal mixer to change to volume.
	) Documented in the `advdev.txt' file how use the ALSA `dmix' plug
		for a software mixing from multiple programs.

AdvanceMAME Version 0.81.1 2004/04
	) In Linux the hardware ports are always accessed directly if
		the program is run as root. Otherwise is used the /dev/port
		interface.
	) Removed the green bars when using the video overlay.
	) Added support for recent Radeon boards 9600/9700/9800 at
		the DOS version.
	) Fixed the MNG recording.
	) The `input_dipswitch[difficulty]' option now always override
		the `misc_difficulty' option.
	) Added a new `device_alsa_device' option to select the ALSA
		output device.
	) Removed the initial audio tick with the ALSA and OSS drivers.

AdvanceMESS Version 0.81.0.0 2004/04
	) All from AdvanceMAME 0.81.0.

AdvanceMAME Version 0.81.0 2004/04

AdvanceMENU Version 2.3.3 2004/04
	) Updated the expat library to version 1.95.7.
	) Added support for recent Radeon boards 9600/9700/9800 at
		the DOS version.
	) Fixed the background music multi directory specification in Linux.
	) Fixed the `generic' emulator rom finding.
	) Added a new `device_alsa_device' option to select the ALSA
		output device.
	) Removed the initial audio tick with the ALSA and OSS drivers.

AdvanceMESS Version 0.80.0.0 2004/03
	) You can now specify configuration options for a single software
		using the format `SYSTEM[SOFTWARE]/OPTION' in the configuration
		file. Like `ti99_4a[ti-inva]/display_magnify auto'.

AdvanceMAME Version 0.80.0 2004/03
	) The 16 bits rgb color mode is now used as default also if the
		game has less than 256 colors. Previously the 8 bits palette
		color mode was used.
	) Added new configuration sections based on the number of the
		players in the game using the format `Nplayer/OPTION'.
		Like `1player/input_map[p1_button1] lshift'
	) Added new effects `scale2x3', `scale2x4', `lq2x3', `lq2x4',
		`hq2x3', `hq2x4'.
	) Added support for TrueType (TTF) fonts with alpha blending
		using the FreeType2 library. Added also a new `ui_fontsize'
		option to select the font size and a collection of TTF fonts
		in the `contrib/' dir
	) Renamed the `input_map[double*_*]' options as `input_map[p1_double*_*]'
		to allow different player specifications [Shane Warren].

AdvanceMENU Version 2.3.2 2004/03
	) Renamed the `video_*' options in `display_*' like AdvanceMAME.
	) Added new `ui_help' and `ui_exit' options to display arbitrary
		help and exit images.
	) Added support for TrueType (TTF) fonts with alpha blending
		using the FreeType2 library. Added also a new `ui_fontsize'
		option to select the font size and a collection of TTF fonts
		in the `contrib/' dir
	) Fixed the icon loading from a .zip file.

AdvanceMESS Version 0.79.0.0 2004/02
	) All from AdvanceMAME 0.79.1 and MESS 0.79.0.

AdvanceMAME Version 0.79.1 2004/02
	) Added a new revision of the `scale3x' effect. It now looks good as
		`scale2x'. Added also a new optimized C version of the `scale2x'
		effect for not x86 MMX architectures.
	) The `max' effect now works also in vertical and horizontal expansion.
	) Fixed some compilation problems on Mac OS X.
	) General improvement of the video blit speed. The program now
		evaluates at runtime the best approach to write video
		memory. Or direct or buffered writing.
	) Improved the speed of the `lq', `hq', `mean' and `filter' effects
		using a different method to compute mean pixels.
	) The `Video Pipeline' menu now display the time used to update
		the screen.
	) Fixed the display of help tags of players 2, 3, 4.
	) Fixed the use of palette 8 bit modes with the FrameBuffer Linux drivers.
	) Added a workaround to correct the color in the 15 bit RGB modes for
		the Radeon FrameBuffer Linux 2.4.23 driver.
	) Fixed a memory leak in the DOS SVGALIB Riva/Radeon driver which
		caused a crash after some video mode changes at runtime.
	) The C 68000 emulator is now used as default. You can revert back to
		the assembler version with the --enable-asm-68000 configure
		switch. A similar --enable-asm-mips3 switch is present for
		the MIPS3 assembler emulator.
	) The default host configuration directory is now /usr/local/etc if you
		install in /usr/local.
	) The default documentation directory is now /usr/local/share/doc/advance
		if you install in /usr/local.
	) The autoconf defines are now stored in a header file and not in the
		compiler command line.
	) The debugger now always use the new `max' effect. It also forces
		the video mode to be `rgb' to remove any palette problem.

AdvanceMAME Version 0.79.0 2004/02
	) Added LCD support at the Linux version using the lcdproc library.
		You can now display arbitrary messages on the LCD using the
		"lcd" command on the scripts.
	) Added the `device_svgawin_skipboard' and `device_svgaline_skipboard'
		options to control which video board to use in Windows and DOS
		in case you have more than one video board on the system.
	) To compile in DOS/Window now you must use the `Makefile.usr' file.
	) Fixed a problem when converting old options name to the new values.
		The conversion was failing if more than one conversion was required.
	) Splitted the big source file blit.c in small parts. This should reduce
		the memory requirement when compiling.

AdvanceMENU Version 2.3.1 2004/02
	) If no preview is available, the preview type is automatically
		changed to a preview type not empty.
	) The default host configuration directory is now /usr/local/etc if you
		install in /usr/local.
	) The default documentation directory is now /usr/local/share/doc/advance
		if you install in /usr/local.

AdvanceMENU Version 2.3.0 2004/02
	) Added a the new entry `multiloopall' at the `ui_clip' option
		which loops all the clips.
	) To compile in DOS/Window now you must use the `Makefile.usr' file.
	) Added the "m" library at the link step. This solve the linking
		problem with recent MINGW compilers.
	) Fixed a problem when converting old options name to the new values.
		The conversion was failing if more than one conversion was required.

AdvanceMAME Version 0.78.1 2004/01
	) Fixed the audio surround enabled with the "sound_mode surround" option.
		Now you should not hear audio statics anymore.
	) Renamed the options `misc_fps', `misc_speed', `misc_turbospeed' and
		`misc_startuptime' in `sync_fps', `sync_speed', `sync_turbospeed' and
		`sync_startuptime'.
	) Partially reverted back to using the MAME sound generation instead of
		the internal resampling. You can switch to the internal resampling
		using the `sync_resample' option.
	) The dipswitchs named "Unknown" and "Unused" are never saved.
	) The long text lines of scroll windows are now wrapped if required.
	) Added in the contrib dir a patch for "linux/fb.h" for compiling the
		Frame Buffer support on the Linux Kernel 2.6.0.
	) Added the "log" and "msg" commands at the scripts and fixed
		the "event(EVENT)" and "get(PORT)" commands.
	) The `dipswitch' and `configswitch' options in the .rc file are now
		saved only one time.

AdvanceMESS Version 0.78.0.1 2004/01
	) Fixed the storing of the `Start' and `Select' input.

AdvanceMAME Version 0.78.0 2004/01
	) Added a new `help' button which display an arbitrary image with
		the player buttons highlighted. You can customize the image
		and the keys/buttons highlight with the options `ui_helptag'
		and `ui_helpimage'. The default key is F1.
	) Added a new `cocktail' button which flip the game screen for
		a cocktail monitor. The default key is the slash on the
		keypad.
	) A new MAME like user interface completely independent of the MAME core.
		This interface doesn't have applied the same video
		effects of the game, it isn't limited on the game area, it
		isn't recorded on video clips and you can customize the font.
	) Removed the .cfg file support. All the input customizations are now
		saved in the .rc file.
	) The frame skipping algorithm try now to get the correct speed of the
		game also if it imply a waste of CPU time.
	) Fixed the screen position on games with a moving display area.
		For example invaders in cocktail mode.
	) The exit menu is always displayed if the `safequit' function is
		enabled and the `event.dat' file is missing or it doesn't contain
		information of the game. You can disable it with the option
		`misc_safequit no'.
	) The DOS version should now restore correctly the video mode at the
		program exit.
	) Fixed the mouse support with the SVGALIB driver.

AdvanceMESS Version 0.78.0.0 2004/01
	) All from AdvanceMAME 0.78.0.

AdvanceMENU Version 2.2.17 2004/01
	) Removed the MAME .cfg support. AdvanceMAME doesn't use them
		anymore.
	) The DOS version should now restore correctly the video mode at the
		program exit.

AdvanceMAME Version 0.77.2 2003/12
	) Games with unemulated protection are now reported as not working.
		For example Choplifter.
	) The DOS version now automatically disable any BIOS call on ATI
		boards to prevent problems on broken video BIOS.
	) Improved the frame synchronization algorithm. It uses less
		CPU power if the operating system have a fine sleep
		granularity like Linux 2.6.x.
	) Reverted back the S3 Savage/Virge/Trio SVGALIB driver at
		version 1.9.17. At least with S3 VirgeDX there is
		a regression in version 1.9.18.

AdvanceMENU Version 2.2.16 2003/12
	) The menu is now more aggressive using CPU power. It helps
		playing video clips at more than 50 Hz and with
		multiple clips.
	) In the multi clip mode, the cursor clip and sound are now always
		played at full speed. Only the background clips are played
		in slow-motion if the CPU power isn't enough.
	) The DOS version now automatically disable any BIOS call on ATI
		boards to prevent problems on broken video BIOS.
	) Reverted back the S3 Savage/Virge/Trio SVGALIB driver at
		version 1.9.17. At least with S3 VirgeDX there is
		a regression in version 1.9.18.

AdvanceMAME Version 0.77.1 2003/12
	) The audio/video synchronization is now done by AdvanceMAME
		without using the internal MAME core support. This should
		solve all the sound distortion problems present in some game
		and all the input recording desynchronizations.
	) The `display_interlaceffect' option has now a new `filter' value
		which operates like the old `filtery' effect.
	) The configuration options are also read in the parent game and bios
		sections.
	) Fixed a segmentation fault bug on the DOS SVGALIB driver for
		ATI Rage boards.
	) The description of the AdvanceMAME device drivers is now in the
		`advdev.txt' file and it's installed as a man page in the Linux
		systems.
	) The `display_magnify' option is now disabled for vector games.
	) Fixed some problems running vector games with a window manager.
	) The `zoom' value of the `device_video_output' option is now
		named `overlay'.
	) Added a new `input_name' option to customize the input names
		displayed in the menus [Martin Adrian].
	) Added a new `display_pausebrightness' option to control the
		display brightness when paused [Martin Adrian].
	) The Linux version should now found the slang.h file also if it's
		in include/ and not in include/slang/.
	) The `keyboard[]' option now accepts numerical scancode in the
		form `scanN'.
	) Fixed a compilation problem with the most recent ALSA library
		(version 1.0.0rc2).
	) If an old (1.4.x) SVGALIB version is installed the program
		doesn't abort.

AdvanceMENU Version 2.2.15 2003/12
	) The mixer now uses a separate buffer for any sound effects.
		This should decrease the sound overlapping of too long
		sound buffers.
	) Fixed a segmentation fault bug on the DOS SVGALIB driver for
		ATI Rage boards.
	) The ui_top/bottom/left/right values are now scaled according
		to the ui_background image.

AdvanceMAME Version 0.77.0 2003/11
	) Added support for ACT Labs Lightgun in the Linux event driver.
	) Fixed the order of the joystick buttons on the Linux event driver.
	) Fixed the joystick dead zone management on the Linux event driver.
	) Fixed an overflow problem in the YUV C conversion (used only if MMX
		was not available).
	) Updated the hq2x effect to the last official version.
	) Fixed a crash bug on the `mean' and `max' effects.
	) The `mean' and `max' effects now work also in the horizontal
		direction.
	) Removed the `filterx' and `filtery' effects.
	) In the DOS version removed the legacy support for unchained
		VGA modes (8 bit modes with memory in 4 planes) and the
		VBE1 banked modes (not linear modes).
	) Autoframeskip is automatically disabled if the correct game
		speed is impossible to reach.

AdvanceMENU Version 2.2.14 2003/11
	) All the snapshots and animated clips are now reduced in size
		using the AdvanceMAME `mean' effect.
	) In the DOS version removed the legacy support for unchained
		VGA modes (8 bit modes with memory in 4 planes) and the
		VBE1 banked modes (not linear modes).
	) Added a new `ui_clip' option which allow to play all the video
		clips together.
	) Improved the ui_background option. It now supports the most common
		RGB and palette formats and it's also able to stretch the image
		to fit the screen. It's now working also in DOS/Windows.

AdvanceMAME Version 0.76.0 2003/10
	) Fixed a computation error in the `hq2/3/4x' effects.
	) Fixed a bug on the `mean' effect introduced in the last version.
	) Upgraded the DOS/Windows SVGALIB drivers at version 1.9.18.
	) The harddisk differential image .dif file are now created in memory
		if the access on the filesystem is denied.

AdvanceMENU Version 2.2.13 2003/10
	) The SIGHUP signal now kills and restarts the program.
	) Added external commands support with the new
		`ui_command' option.
	) Upgraded the DOS/Windows SVGALIB drivers at version 1.9.18.

AdvanceMAME Version 0.74.1 2003/09
	) Added the effects `lq4x' and `hq4x'. The `hq2x' and `hq3x' versions
		are now exactly like the original effects.
	) The effects names `scale2/3/4x', `lq2/3/4x' and `hq2/3/4x' are now
		unified in `scale', `lq' and `hq'. The correct scale
		factor if automatically detected from the video mode size.
	) Updated the `vsync/ac97' audio driver for VIA VT823x built
		in AC97 audio [Shigeaki Sakamaki].
	) Fixed the `advv' utility to display and use all the available
		default modelines.

AdvanceMESS Version 0.74.0.0 2003/09
	) All from AdvanceMAME 0.74.1.

AdvanceMENU Version 2.2.12 2003/09
	) Added a new set of `ui_*' options to define an user interface skin
		with a background image.
	) Added a new `misc_exit' option which substitute the old `exit_press'.

AdvanceMAME Version 0.74.0 2003/09
	) Added a bunch of new video effects: `lq2x', `hq2x', `lq3x' and `hq3x'.
		They are interpolation effects. Slower than `scale2x'
		but with a nicer image.
		They are derived from the HQ3X effect made by Maxim Stepin.
	) The Linux fb video driver is now able to wait for vsync also if
		the low-end driver doesn't support this feature.
		You must run the program as root.
	) Increased the number of supported event [Filipe Estima].
	) Fixed support for SiS boards 530/630/730 in the DOS vbeline/sis
		driver.
	) Revised the list of supported cards.
	) Removed the `display_rotate' option.

AdvanceMAME Version 0.73.0 2003/09
	) Fixed some problems on the SDL keyboard driver in the
		Windows version.
	) Added a preliminary support for newer Radeon boards for the
		DOS version.
	) The Linux version now works also if it cannot read the tty state.

AdvanceMESS Version 0.73.0.0 2003/09
	) All from AdvanceMAME 0.73.0.

AdvanceMENU Version 2.2.11 2003/09
	) Fixed some problems on the SDL keyboard driver in the
		Windows version.
	) Added a preliminary support for newer Radeon boards for the
		DOS version.
	) The Linux version now works also if it cannot read the tty state.

AdvanceMAME Version 0.72.0 2003/09
	) Added a new option `sound_normalize' which automatically increase
		the volume of games with a too lower one. It's enabled by default.
	) Added a new `include' option to include additionally
		configuration files.
	) The `display_magnify' option has now a new `auto' setting
		which automatically scale the game if it's
		too small.
	) Added a new set of Linux `event' input driver for keyboards, mice
		and joysticks based on the Linux input-event interfaces.
		These drivers remove any limitations on the number of
		keyboards, mice and joysticks.
	) The `input_map' option now accepts the `auto' setting which
		is able to map the correct input device on the correct
		game control.
	) The `input_map' option now can remap also all the digital
		inputs like keys, buttons and digital joystick.
	) Improved the advk, advj and advm utilities. They now
		report more information on the hardware found.
	) Revised the `safequit' system. The database file is now
		called `event.dat' and it has a strong error check.
		The options are now named `misc_eventdebug' and
		`misc_eventfile'. A new set of scripts `safequit' and
		`event1,2,3,4,5,6' are now started when triggered by the
		event system.
	) Renamed the script names removing the [] in the names.
	) The Linux keyboard `raw' driver has now a basic support to switch
		virtual terminal pressing ALT+Fx.
	) Added a new Linux `raw' joystick driver.
	) Substituted the Linux input driver `slang' with a new `tty' driver
		which always works correctly with the advv and advcfg utility.
		Specifically it works when using the `fb' video driver.
	) In Linux the host configuration files are now read in /etc.
	) The `-version' option now lists the low level drivers compiled
		in the executable.
	) The Linux version can now access hardware ports in scripts using
		the /dev/port interface.
	) Reduced the CPU cache usage in the palette conversion.

AdvanceMESS Version 0.72.0.0 2003/09
	) All from AdvanceMAME 0.72.0.
	) The specific AdvanceMAME keys are now disabled in the full
		keyboard emulation like any other MAME key.

AdvanceMENU Version 2.2.10 2003/09
	) Added a new set of Linux input driver for keyboards, mice
		and joysticks based on the Linux input-event interfaces.
	) Added a new `include' option to include additionally
		configuration files.
	) Added support for png images with alpha channel.
	) The `Run Clone' command on a MESS software now allow to select the
		clone of the BIOS to use.
	) The `Types' and `Group' menus are shown only if the group and
		type set are not empty.
	) In Linux the host configuration files are now read in /etc.

AdvanceMAME Version 0.71.1 2003/07
	) Added a new mouse driver for Linux which supports up to 4 mice
		at the same time.
	) Partially fixed some problems on games that need
		to change the display area at runtime like "orunners".
	) Added the `misc_fps' option to change arbitrarily the frame rate
		of the games (like SmoothMAME).

AdvanceMESS Version 0.71.0.0 2003/07
	) All from AdvanceMAME 0.71.0.
	) Added the `misc_ramsize' option.

AdvanceMENU Version 2.2.9 2003/07
	) Added support for importing the XML output of AdvanceMESS 0.71.0.0.
	) Added support for ignoring the return code of the executed
		generic emulator. Simply put a '-' in front of the emulator
		executable name.
	) The number of listed games is now correct in any sort mode.

AdvanceMAME Version 0.71.0 2003/07
	) Added the volume control at the SDL sound driver. It's implemented
		reducing the sample values and not using the hardware
		volume control.
	) Fixed a possible bug on the SVGALIB Rage 128/Radeon drivers.
		Also updated the SVGALIB patch in the contrib/ dir.
	) The information on the supported drivers is now in the
		device.txt file.

AdvanceMENU Version 2.2.8 2003/07
	) Added the volume control at the SDL sound driver. It's implemented
		reducing the sample values and not using the hardware
		volume control.
	) Fixed a possible bug on the SVGALIB Rage 128/Radeon drivers.
		Also updated the SVGALIB patch in the contrib/ dir.

AdvanceMAME Version 0.70.0 2003/06
	) Added some patches for Linux 2.4.20 Frame Buffer for ATI Radeon
		and nVidia GeForce boards to improve the support of low
		clock video modes.
	) Added some patches for SVGALIB 1.9.17 for Trident and nVidia GeForce
		boards to improve the support of low clock video modes.
		These patches are integrated in the DOS and Windows SVGALIB
		versions.
	) Fixed the sound recording broken in version 0.68.0.
	) The MAME sound emulation is activated also if the "none" sound
		device is chosen.
	) The SIGPIPE signal is no more redirected.
	) The SIGHUP signal now quits the program without aborting.
	) Reduced the sound buffer length for the ALSA driver.
	) Removed the legacy support for the "mame.cfg" configuration file.

AdvanceMENU Version 2.2.7 2003/06
	) Fixed the horizontal and vertical sync polarity on the Linux
		Framebuffer video driver [by Ralph]
	) The Windows version is now able to shutdown the system like the
		other versions.
	) Added support for the new MAME -listxml option. It's now
		used as default. The old -listinfo is still supported
		and it's used if -listxml fails.
	) Fixed the restoring of the old cursor position.
	) Removed the legacy support for the "mm.cfg" configuration file.

AdvanceMAME Version 0.69.0 2003/05
	) Fixed the horizontal and vertical sync polarity on the Linux
		Framebuffer video driver [Ralph]
	) Added a new preliminary -listxml option.
	) Fixed the output of all the floating point values in configuration
		files for the DOS version.

AdvanceMAME Version 0.68.0 2003/05
	) Added the `scale3x' and `scale4x' effects.
	) Added support for Mac OS X. It compiles and run with the SDL library.
	) Added support for generic BigEndian targets.
	) The `magnify' option now accepts the input values 1, 2, 3 and 4.
	) The `display_adjust' value `generate' is now renamed `generate_yclock'.
		A bunch of new `generate_*' values are available for a fine
		control on the generated modes.
	) In Linux you can specify an arbitrary data directory with the
		$ADVANCE environment variable. This value overwrites the default
		$HOME/.advance.
	) Added the "-version" command line option.
	) Removed some "buffer overflow".
	) Fixed the mouse handling in Linux with the SVGALIB library
		[Fabio Cavallo].
	) Fixed some minor problems in the `configure' script.
	) Added the `device_video_zoom' option to control the size of
		screen mode used with the `zoom' output mode.
	) Some fixes for the gcc 3.3 compiler.

AdvanceMENU Version 2.2.6 2003/05
	) Added support for Mac OS X. It compiles and run with the SDL library.
	) Added support for generic BigEndian targets.
	) In Linux you can specify an arbitrary data directory with the
		$ADVANCE environment variable. This value overwrites the default
		$HOME/.advance.
	) Added the "-version" command line option.
	) Removed some "buffer overflow".
	) Fixed the mouse handling in Linux with the SVGALIB library
		[by Fabio Cavallo].
	) Added a new sort order `time_per_coin'.
	) Added a statistics information page.
	) Some fixes for the gcc 3.3 compiler.

AdvanceMAME Version 0.67.0 2003/04
	) Updated with autoconf 2.57.
	) The error messages are printed also if `misc_quiet' is specified.
	) The configure script now checks if the MAME/MESS emulator source
		are missing.
	) Added in the contrib/ dir the "Quick guide to find safequit data"
		[Filipe Estima].

AdvanceMESS Version 0.66.0.0 2003/04
	) All from AdvanceMAME 0.67.0.

AdvanceMAME Version 0.66.0 2003/03
	) The display_scanlines option is now off as default.
		The `generate-scanline' video modes are now created only
		if display_scanlines is active at the startup.
	) Minor changes at the video menu.
	) Fixed the `-playback' command.
	) Fixed the slowdown problem reading .dat files.
	) Added automatic detection of Linux Frame Buffer capabilities.

AdvanceMENU Version 2.2.5 2003/03
	) Fixed the default colors.
	) Updated with autoconf 2.57.

AdvanceMAME Version 0.65.0 2003/02
	) Fixed the CPU detection by the configure script.
	) The Linux ALSA sound drivers now doesn't block the execution if the
		DSP is already in use.
	) Fixed the Linux Frame Buffer driver when a DIRECTCOLOR mode
		is used displaying fuzzy colors.
	) Added some patches for the Linux SVGALIB in the contrib/ dir.
	) Added a new `device_video_cursor' option to control the visibility
		of the mouse cursor.
	) Minor fix computing the size of double sized modes.

AdvanceMESS Version 0.64.0.0 2003/02
	) All from AdvanceMAME 0.65.0.

AdvanceMENU Version 2.2.4 2003/02
	) Fixed the CPU detection by the configure script.
	) The Linux ALSA sound drivers now doesn't block the execution if the
		DSP is already in use.
	) Fixed the Linux Frame Buffer driver when a DIRECTCOLOR mode
		is used displaying fuzzy colors.
	) Added a new `device_video_cursor' option to control the visibility
		of the mouse cursor.

AdvanceMAME Version 0.63.0 2003/01
	) Added the ALSA sound driver for Linux. It's now the
		preferred choice over OSS.
	) Fixed another bug in the ./configure script if the
		SDL library is missing.
	) The sound latency is now automatically increased if the game
		requires a big frame skip.
	) The auto frameskip is now optimized to minimize the idle
		waiting time instead of getting a 100% speed.
	) Added the `display_artwork_backdrop/overlay/bezel' option
		to control any artwork types.
	) Added the `misc_difficulty' option to select the game
		difficulty from the command line.
	) The `misc_quiet' option now also disable the NO GOOD DUMP
		messages.
	) The SVGALIB `banshee' driver in Linux now doesn't tries to
		use 16 bit video modes. They are not working.
	) The `misc_quiet' options, if activated, skips also the graphics
		information screens.
	) If the doublescan modes are disabled, a mode with a double
		vertical size is automatically used instead.

AdvanceMENU Version 2.2.3 2003/01
	) Added a new attrib for the AdvanceMESS emulator
		to exclude the systems without a BIOS rom.
	) Added the Alsa sound driver for Linux. It's now the
		preferred choice.
	) The emulator rom info file is not updated if the existing
		file is readonly.
	) Renamed the "Attrib" menu to "Selection".
	) Added a new `difficulty' option and menu to globally control
		the difficulty level of the games. It works only
		for the `advmame' emulator type and only for games
		with a difficulty dip switch.
	) The `generic' emulator type is now able to import games
		information from a MAME like information file
		written manually.
	) The default key configuration now uses the numeric-pad keys
		as arrows. It helps with HotRod.
	) For the `advmame' and `advmess' emulator types the
		sound volume is automatically set at the same
		level of the menu.

AdvanceMAME Version 0.62.2 2002/12
	) Added a new raw keyboard driver for Linux [Kari Hautio].
	) Changed the yield behavior on high system load. Now the
		yield operation is done before the synchronization point.
		It should reduce the effective system load.
	) Fixed some issues in the ./configure scripts.
	) Removed the blinking cursor in the Linux `fb' video
		driver [Kari Hautio].
	) Added support for YUV overlay in the SDL driver with a new
		zoom mode. With this driver the image is zoomed
		by the video board. Check the new `device_video_mode'
		option.
	) Upgraded at the SVGALIB 1.9.17 library.
	) Fixed a bug on exit with a multiprocessor system.
	) Added a new `sound_mode' option which replaces the old
		`sound_stereo'. It also support a (fake) `surround'
		effect.
	) The `advcfg' utility now uses the user and system list
		of modelines if the device_video_p/h/vclock option is
		present.

AdvanceMESS Version 0.62.0.0 2002/12
	) All from AdvanceMAME 0.62.2.

AdvanceMENU Version 2.2.2 2002/12
	) Fixed the MNG playing in 32 bits modes. This is potentially
		a crash bug fix.
	) The bitmap scaler now uses the same algo of the MNG player.
	) The game index is now saved when the sort order is changed
		or when the game position changes because you play it.
	) Added a new raw keyboard driver for Linux [Kari Hautio].
	) Fixed some issues in the ./configure scripts.
	) Removed the blinking cursor in the Linux `fb' video
		driver [Kari Hautio].
	) Removed the option `video_depth'.
	) Upgraded at the SVGALIB 1.9.17 library.
	) Removed the support for the 8 bit palette mode.
	) The `color' option now support a true color specification with
		the format RRGGBB in hex values.

AdvanceMAME Version 0.62.1 2002/11
	) Fixed the abort bug on the DOS and Windows versions for the
		SVGALIB Rage 128/Radeon video boards and probably others.
	) Some fixes for the `.chd' games.
	) Fixed the palette management of the cojag games.
	) The DOS `vbeline' and `vbe' drivers now correctly detect
		the absence of some bit depths.

AdvanceMESS Version 0.61.2.1 2002/11
	) All from AdvanceMAME 0.62.1.

AdvanceMAME Version 0.62.0 2002/11
	) The Windows SVGAWIN driver now use the framebuffer reported by
		Windows if the Windows SVGALIB isn't able to get it.
		This fixes the crash problem with the Cirrus Laguna boards
		and probably others.
	) Updated the Windows SVGAWIN.SYS driver to the 1.1 version.
	) Added the `display_aspectx' and `display_aspecty' to arbitrary
		selects the aspect ratio of the monitor used.
	) Fixed a slowdown bug on the SVGALIB video board detection.
	) Fixed a bug on the DOS/Windows SVGALIB Radeon driver. It should
		help with the video modes at lower frequency generally used
		in Arcade Monitors and TVs.
	) Fixed a bug on the SDL sound management. It should help the
		interaction of AdvanceMENU and AdvanceMAME.
	) Better error reporting on the DOS/Windows SVGALIB drivers.
	) The Windows binary is now packaged with the SDL dll 1.2.5
	) The `display_adjust' option is now ignored if the video driver
		is not programmable.

AdvanceMESS Version 0.61.2 2002/11
	) The MESS threads are now used only if the SMP runtime option
		is activated. Previously on a SMP compile the threads was
		always used.
	) All from MESS 0.61.2.
	) All from AdvanceMAME 0.62.0.

AdvanceMENU Version 2.2.1 2002/11
	) Reduced the startup load time.
	) The event `group' and `type' now select the next group and type
		instead of opening the menus.
	) Fixed the abort bug on the DOS and Windows versions for the
		SVGALIB Rage 128/Radeon video boards and probably others.

AdvanceMAME Version 0.61.4 2002/11
	) Merged the sdl and native system of the Linux target.
		You can now mix the SDL input/output drivers with the
		native drivers.
	) Better ./configure script. It detects and automatically
		enables all the available libraries.
	) The configuration file now recognizes the SIZEXxSIZEYxFREQ section.
	) Fixed a precision error checking the clock in the DOS vgaline
		driver.
	) Removed the limitation of 8 bit crtc multiplier in the DOS
		vgaline driver.
	) Revised the SVGALIB DOS compatibility layer. Some bugs fixed.
	) Added the support for the SVGALIB video drivers in Windows NT/2000/XP.

AdvanceMESS Version 0.61.1 2002/11
	) Fixed the name of the .nv files
	) All from MESS 0.61.1.
	) All from AdvanceMAME 0.61.4.

AdvanceMENU Version 2.2.0 2002/11
	) In the Linux target the `shutdown' event runs "/sbin/poweroff".
	) Merged the sdl and native system of the Linux target.
		You can now mix the SDL input/output drivers with the
		native drivers.
	) Better ./configure script. It detects and automatically
		enables all the available libraries.
	) Fixed a precision error checking the clock in the DOS vgaline
		driver.
	) Removed the limitation of 8 bit crtc multiplier in the DOS
		vgaline driver.
	) Revised the SVGALIB DOS compatibility layer. Some bugs fixed.
	) Added the support for the SVGALIB video drivers in Windows NT/2000/XP.
	) Fixed a slowdown bug on the SVGALIB video board detection.
	) Fixed a bug on the DOS/Windows SVGALIB Radeon driver. It should
		help with the video modes at lower frequency generally used
		in Arcade Monitors and TVs.
	) Fixed a bug on the SDL sound management. It should help the
		interaction of AdvanceMENU and AdvanceMAME.
	) Better error reporting on the DOS/Windows SVGALIB drivers.
	) The Windows binary is now packaged with the SDL dll 1.2.5

AdvanceMENU Version 2.1.2 2002/09
	) Added the support of the VSyncMAME audio drivers. This add
		support for the AC97 chipset [Shigeaki Sakamaki].
	) Removed the compression of all the executables, and added
		a little debug info on the precompiled binaries.
		This should help the problem reporting.
	) Fixed a bug when setting emulator attributes without games
		listed.

AdvanceMAME Version 0.61.3 2002/09
	) Fixed the window title and the icon in the sdl system.
	) In Windows the default sdl samples buffer is now 2048.
		This solve the distorted sound.
	) Added the new option `display_interlaceeffect' to help
		with some broken monitors which need to swap the
		video rows in interlaced modes.
	) Uppercase names of games in the command line are accepted.
	) Improved the advcfg error management.
	) Added the `cost' doc.
	) Revised the video section of the `faq' doc.
	) The `display_vsync' option is now enabled as default.
	) Added the support of the VSyncMAME audio drivers. This add
		support for the AC97 chipset [Shigeaki Sakamaki].
	) Fixed a bug in the 68k emulator in the Windows port.
	) Removed the compression of all the executables, and added
		a little debug info on the precompiled binaries.
		This should help the problem reporting.

AdvanceMESS Version 0.61.0 2002/09
	) Fixed the window title and the icon in the sdl system.
	) In Windows the default sdl samples buffer is now 2048.
		This solve the distorted sound.
	) Added the new option `display_interlaceeffect' to help
		with some broken monitors which need to swap the
		video rows in interlaced modes.
	) Uppercase names of games in the command line are accepted.
	) Improved the advcfg error management.
	) Added the `cost' doc.
	) All from AdvanceMAME 0.61.2

AdvanceMENU Version 2.1.1 2002/09
	) Fixed the window title and the icon in the sdl system.
	) In Windows the default sdl samples buffer is now 2048.
		This solve the distorted sound.
	) In DOS the executables are now searched also in the current
		directory.
	) Fixed the missing software bug when both DMESS and AdvanceMESS
		are present.
	) Some minor fixes for the AdvanceMESS use.
	) Added the `cost' doc.

AdvanceMAME Version 0.61.2 2002/09
	) Added the `sdl' system which uses the libSDL graphics
		library. This system enable the use of the program
		in a Window Manager. But it isn't a good choice for
		a fullscreen use of the program because it can't
		generate `perfect' video mode.
	) Added the HOST_SYSTEM section in the makefile.
	) Added the `windows' target in the Makefile. It use the `sdl'
		system.
	) Added the `device_sdl_fullscreen' option to force the use of
		a fullscreen mode with the SDL library.
	) Readded the basic `vbe' driver.
	) Added the MMX detection in the linux and windows targets.
		Previously the MMX presence was assumed.
	) Splitted the `input_map[,trak]' option in the input_map[,trakx]
		and `input_map[,traky]' options. Now you can assign
		different axes from different mice.
	) The `input_map[]' option now supports multiple input sources
		and eventually allow to negate them.
	) Renamed the option `misc_language' to `misc_languagefile'.
	) Fixed the saving of the cheats.
	) Documented the various `misc_*file' options.
	) Removed the `zmng' utility from the contrib dir.
		It's now included in the AdvanceSCAN package.
	) Renamed the `input_safeexit' option to `misc_safequit' and
		added the options `misc_safequitdebug' and
		`misc_safequitfile'. The SafeQuit is now enabled by
		default.
	) Revised the output format of all the documentation. Now
		it's available as formatted text, html and man pages.
	) Fixed an overflow bug on the aspect computation. It prevented
		"Elevator Action" to run.
	) Fixed the "Division by Zero" crash bug with the Rage128 board in
		the DOS svgaline driver.
	) Upgraded at the SVGALIB 1.9.16 library.
	) Readded the `-playback' and `-record' command line options.
	) Added a `./configure' script in all the Linux distributions.
	) Fixed the detection of the screen resize keys in the
		advcfg and advv utility in the Linux platform.
		The keys are now changed to 'i' and 'k'.
	) Added the options `device_video_8bit', `device_video_15bit',
		`device_video_16bit', `device_video_32bit' to selectively
		disable some bit depths on the video drivers.
	) Fixed the colors on the snapshots of games with a 32 color depth.
	) Various fix at the `advcfg' utility.
	) Various bug fixed.

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
	) Added the `loop' option to continuously play the MNG files.
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
	) The Linux version now searches the emulators in the whole
		PATH if a relative path is given.
	) Added support for the 32 bit graphics output.
	) The article `The' on the game names is put at the end
		of the name.

AdvanceMAME Version 0.61.1 2002/07
	) Fixed a stupid bug on some neogeo games.

AdvanceMAME Version 0.61.0 2002/07
	) Removed the input_analog[] and input_track[] options. They
		are now substituted by the new input_map[] option.
	) Upgraded at the SVGALIB 1.9.15 library.
	) Added the `vertical' and `horizontal' sections on the
		config file. With these sections you can have
		different configuration for vertical and horizontal
		games. If you have a rotating monitor, you can now
		always configure the correct orientation.
	) The `display_rotate auto' option now selects always the
		`blit' rotation. The reason of this change is that
		the `core' rotation will be completely removed on the
		next MAME version (0.62).
		On monoprocessor machines this may result in a
		sensible slowdown on some games. Instead, on SMP
		(multiprocessor) machines all these games are now
		faster because the rotation is done in the parallel
		blit thread.
	) Fixed the joystick problem with more than 8 buttons
		[Anthony D. Saxton (las_vegas)].
	) Added the `display_intensity' option.
	) Added the `display_artwork_crop' option.
	) Fixed the `display_expand' option for the vertical vector
		games.
	) Prevented the use on the NT platform.
	) Added some new MMX blitters to speedup the blit rotation.
	) In the Linux version if the HOME environment variable
		is not set all the files are read and written on the
		PREFIX/share/advance directory.
	) Fixed a bug when forcing the use of the vbeline/3dfx driver.
	) Enabled the interlaced modes with the 3dfx svgalib driver.
	) Minor bugfix.

AdvanceMENU Version 1.19.1 2002/07
	) Fixed a bug when forcing the use of the vbeline/3dfx driver.
	) Enabled the interlaced modes with the 3dfx svgalib driver.

AdvanceMENU Version 1.19.0 2002/06
	) Upgraded at the SVGALIB 1.9.15 library.
	) Added the joystick calibration on the main menu.
		This fixes the inverted movement with some
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

AdvanceMAME Version 0.60.1 2002/05
	) Fixed the MNG recording for some games.
	) Added the input_hotkey option to disable the special
		key sequences like CTRL+ALT+DEL.
	) Fixed the rgb 16pix effect on 32 bit modes.
	) Disabled the pause key. It should not be used.
	) The advs utility now prevent the keyboard buffer to be
		filled.
	) Added a new `License' chapter at the faq.txt file.

AdvanceMENU Version 1.18.3 2002/05
	) Fixed an hang bug when no games are listed.

AdvanceMAME Version 0.60.0 2002/05
	) Upgraded at the SVGALIB 1.9.14 library.
	) Solved a video mode restore problem with the
		vbeline drivers.
	) After unpausing the idle exit timeout is now reset.
	) Fixed the idle_exit timing on Linux.
	) Fixed the size of vector games when a custom mode is
		specified with the display mode option.
	) Other minor bugfix.

AdvanceMENU Version 1.18.2 2002/05
	) Upgraded at the SVGALIB 1.9.14 library.

AdvancePAC Version 0.58.0 2002/04
	) Added the PacMAME target at AdvanceMAME.

AdvanceMAME Version 0.59.1 2002/04
	) Upgraded at the SVGALIB 1.9.13 library.
	) Upgraded at the Allegro 4.01 library.
	) Fixed some issues if no sound card is present.
	) Fixed a bug in the blit rotation.
	) Updated the list of the games which require the blit
		rotation for the auto option.
	) Fixed the use of gamma and brightness on the rgb games.
	) The precision of the frame rate of the generated MNG
		files is now always at least 1:1000000.
	) The `mng' recording now can be started and stopped also
		during the pause.
	) The `script_play' option now works also if
		`misc_startuptime' is 0.
	) Fixed some issues on the directory paths.

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
	) Changed the behavior of the `*_import' options.
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

AdvanceMAME Version 0.59.0 2002/03
	) Added the option "-r" at the zmng utility. This fixes the
		bug when compressing 24 bit image which cannot be
		converted to 8 bit.
	) Added the device_video_singlescan/doublescan/interlace
		options to disable the use of some mode types.
	) Rewritten the `vgaline' DOS driver. Now it never uses the
		BIOS to set the video mode.
	) The MNG files now use the FRAME chuck to specify a
		precise timing.
	) Added the `misc_quiet' option to disable the text message :
		"AdvanceMAME - Copyright (C) 1999-2002 by..."
	) Fixed some imprecision on the image stretch.
	) Completed the color conversion functions.

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

AdvanceMAME Version 0.58.1 2002/03
	) Fixed the clock values printed on the first line of the
		video menu.
	) Changed the behavior of the "display_adjust x" option.
		Now all the modes are adjusted in size, not only the
		nearest mode.
	) Faster `scale2x' effect. Now optimized for AGP bus.
	) Fixed the video and sound recording for the SMP machine.
	) The .mng and .wav file are now saved always with the same
		basename.
	) Fixed the selection of the video mode with the nearest clock.
	) The idleexit feature now detect also the joystick and
		mouse use.
	) MMX implementation of some functions of the tilemap engine.

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

AdvanceMAME Version 0.58.0 2002/02
	) Added the video recording feature in `.mng' files.
	) Added the `record_sound', `record_sound_time',
		`record_video', `record_video_time',
		`record_video_interleave' options.
	) Added the `zmng' compression utility in the contrib/zmng dir.
	) When recording, the sound is now generated ignoring
		the synchronization issues to get a perfect sound
		also with games with a very high frame skip.
	) Now you can record sound and video without any length limit.
	) The recorded sound is now NOT adjusted in volume.
	) Minor corrections at the sound synchronization.

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

AdvanceMESS Version 0.56.0 2002/01
	) Added the MESS target at AdvanceMAME.

AdvanceMAME Version 0.57.1 2002/01
	) The default rom and sample directory are now `rom' and
		`sample' (singular).
	) The Linux config directories are now named `$home/.advance' and
		`$prefix/share/advance'. Rename them manually.
	) Fixed the sound snapshot saving.
	) Fixed the graphics snapshot saving in Linux.
	) Fixed the dir rights in creation in Linux.
	) Fixed the support of roms not zipped.
	) Fixed the `display_resize' option saving.
	) Recompiled the video driver with lower optimizations.
	) Readded the autocentering control menu.
	) Fixed a SIGSEGV in the mode selection.
	) Other minor bugfix.

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

AdvanceMAME Version 0.57.0 2002/01
	) Improved the SMP performance. Now the blit stage is completely
		done by the second thread without any bitmap copy.
	) Added a Console Frame Buffer video driver for Linux.
	) Added the `misc_speed' option to control the speed of the game.
	) Added the `sound_latency' option to control the size of the
		sound buffer.
	) Added the `advs' utility. A wav/mp3 player.
	) Added the `faq.txt' and `tips.txt' files.
	) Fixed the `device_video' option. Now works also for `advv' and
		`advcfg'.
	) Fixed the wrong patch command in the build.txt file.
	) Added latency measure in the advk, advj and advm utilities.
	) Removed the SEAL awe32 driver. The generic SEAL SoundBlaster
		driver is faster.
	) Solved some problems and a crash bug on the mode selection
	) Renamed all the `modeline' options in `device_video_modeline'
		in all the .rc files.
	) Solved a bug in the svgaline nv3 driver.
	) Fixed the double mouse support for the DOS version.
	) Fixed the Allegro sound driver volume.
	) Various bugs fixed

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

AdvanceMAME Version 0.56.2
	) Fixed the .dat support in the DOS version.
	) Fixed the script support.
	) Fixed the name used for `memcard' files.
	) Disabled the use of the rgb effects if the mode is palettized
	) Added the contrib directory in the source distribution
	) Added the TARGET=tiny option in the makefile

AdvanceMAME Version 0.56.1
	) General renaming:
		mame -> advmame
		mv -> advv
		cfg -> advcfg
		mk -> advk
	) New utilities:
		advj - Joystick tester
		advm - Mouse tester
	) A complete Linux/i386 port based on the SVGALIB 1.9.x library.
	) A new frameskip computation system.
	) A new format of configuration file (now named advmame.rc).
	) Limited support of Multi Processor (SMP) architecture for
		the Linux version. Check the `misc_smp' option.
	) New input_analog[] and input_track[] option to choice the
		mapping of the joystick and of the mouse. For example you
		can use the joystick for the first player and the mouse
		for the second player.
	) Added support for all the Allegro sound drivers.
		Check the new option `device_sound'.
	) Better and simpler video mode choice.
	) Better vsync support, now it can be enabled and disabled
		at runtime.
	) A new Makefile system, check the `build.txt' file for the new
		compilation instructions.
	) Updated the safequit.dat database [Filipe Estima].
	) Revised the docs [Filipe Estima and Randy Schnedler].
	) Compiled with the latest Allegro 3.9.40 library.
	) Various bugs fixed.

AdvanceMAME Version 0.56.0.1 (never released)
	) Removed a warning in the source
	) Solved the bug of the brightness on screen menu
	) Added a new safequit.dat database [Filipe Estima]
	) Solved a bug with the VBE detection.
	) Solved the bug for the detection of the svgaline Rendition
		driver.

AdvanceMAME Version 0.56.0 2001/11
	) Change at the SVGALIB Rage 128/Radeon drivers for the
		low clocks

AdvanceMAME Version 0.55.2
	) On the documentation the suggested value for pclock
		is now `pclock = 5 - 80'.
	) Solved a bug that prevented the read of the vbeline_* options.
	) Solved a bug that prevented a lot of games to start
		(centiped, nibbler, tempest, ...)
	) Readded the support for the internal MAME debugger.
	) Added the `[config] expand' option to use a bigger screen
		area when playing vertical games on horizontal monitors.
	) Added the MMX implementation for the color conversions
		888 -> 332 and 555 -> 332 (mainly used in vector games)
	) Added the generation of modes with double size with scanlines.

AdvanceMAME Version 0.55.1
	) Added the `[video] pclock' option to control the lower
		and higher limit of the pixel clock used.
		YOU MUST ENTER THIS VALUE IN YOUR mame.cfg.
		The lower value is the lower clock generable
		by your video board. The higher value is the video
		bandwidth of your monitor. If don't know these
		values you can start with `pclock = 5 - 70' which
		essentially enable any pixel clock needed by
		AdvanceMAME. (like the previous version)
	) Added the `-report' option at the MV utility. You can use
		this option to create informative bug report on the
		video drivers.
	) Updated with the new SVGALIB 1.9.12. It contains various
		video driver fix and a new `Rendition V2200' driver.
	) Solved a bug on the command line parsing.
	) Solved the crash bug for the vector games.
	) Better mode generation for the vector games.

AdvanceMAME Version 0.55.0 2001/09
	) The `[gamename]' options are now correctly read.
	) Renamed the option `video_mode_reset' to `videomodereset'
	) The command line boolean options now don't need the
		argument "yes". You can disable them with
		the format -noOPTION.

AdvanceMAME Version 0.53.2
	) Revised the readme.txt/mame.txt/install.txt docs.
	) Added the -cfg option to specify an alternate name of
		the configuration file.
	) Added a new and improved fuzzy game name compare.
	) Readded the soundcard=NUMBER options.
	) Solved the hidded user interface bug.
	) Solved a configuration bug that prevented the vsync option
		to work.
	) Minor bugfix.

AdvanceMAME Version 0.53.1
	) Solved the crash bug on exit
	) Solved the vsync deadlock bug
	) Solved the text mode selection in the mv utility
	) The G200/G100 reference clock set to 27MHz
	) Added the missing makefile (advance.mak) in the source
		distribution
	) Minor bugfix

AdvanceMAME Version 0.53.0 2001/08
	) Major release with BIG changes. This should be considered
		an alpha version.
	) Kept the support for 8 bit modes. You can still use 8 bit
		modes for games with less than 256 colors without
		problems. You can also use 8 bit modes for all the other
		games but you lose in color precision.
	) The modeline format is changed. The new format is independent
		of the video driver and of video board. The modelines
		can now be exchanged between users.
	) Added a new set of video drivers called `svgaline' from
		the Linux SVGALIB library. These drivers overperform
		the old `vbeline' drivers. The old drivers are still
		supported anyway.
		These drivers are completely independent of the underline
		VBE BIOS. You don't need the SDD utility anymore.
	) Added support for 32 bit depth.
	) The sources are now completely independent from the MAMEDOS
		sources. They stay in a new directory called `advance'.
		Check the updated `build.txt' to compile.
	) The configuration file is now named like the executable.
		For example "mamepp.exe" checks for "mamepp.cfg".
	) Added the `videodepth' option to control the bit depth of the
		video mode used. The old `depth' option is still present
		but control only the internal size of the working MAME
		bitmap.
	) Added the command line option `-default' to create a default
		`mame.cfg' file.
	) Removed the `videogenerate=adjustxy' option.
	) Removed all the -list* and -verify* options. These will may be
		added in a future version with an external utility.
		Only the -listinfo option is maintained.
	) Updated the `mv' and `cfg' utility at the new features.
	) Updated the `card.txt' file with a complete list of the video
		board supported.
	) The safeexit option now uses the safequit.dat file to check if
		the exit is safe or not. If the game is in demo mode and no
		coin is inserted the "Continue/Exit" menu is not shown.
		[Filipe de V. Estima (Bugfinder), Ian Patterson]
	) The `soundcard' option now uses text tag and not numbers.
		The tags are :
		auto - Automatic detection
		sb - Sound Blaster
		awe32 - Sound Blaster AWE 32 (probably the 'sb' driver is faster)
		pas - Pro Audio Spectrum
		gusmax - Gravis Ultrasound Max
		gus - Gravis Ultrasound
		wss - Windows Sound System
		ess - Ensoniq Soundscape

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
	) Added a new event `shutdown' which exit promptly and
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

AdvanceMAME Version 0.37b16.1
	) Added the -quiet option to prevent the startup messages.
	) Minor bugfix
	) Added support for the Voodoo5 board.
	) Added support for other Rage 128 boards.

AdvanceMAME Version 0.37b16.0 2001/07
	) Added the new `scale2x' blit effect. It's now the default
		when the `Magnify' option is activated.
	) Solved the unexpected termination when you exit from the
		onscreen menu.
	) The `safeexit' option now show a confirmation menu.
	) Hopefully solved all the "Page Fault" exceptions.
		You should be able to play also very huge games like
		garou with only 64 Mb of memory.
	) Changed the default keys for the sound recording.
		LCTRL+ENTER to start, ENTER to stop.
	) Updated the contrib/aos section by Nick Bourdo
	) Removed the dirty blit support
	) Some new 'ATI r128` and `Trident Blade 3D' cards supported.

AdvanceMAME Version 0.37b15.0 2001/05
	) The game aspect ratio is now used if available
	) The `vsync' option is now ignored if no modes with the
		correct frequency rate are available
	) Added the new scripts commands `simulate_event(EVENT,TIME)',
		`event(EVENT)' and simulate_key(KEY,TIME).
		Check the updated `script.txt' file.
	) Added the new option `safeexit=yes'. If enabled to exit
		you need to keep pressed the ESC key for 3 seconds.
	) The `cfg' utility now accepts the format values also in
		incremental format, not only in the differential format.

AdvanceMAME Version 0.37b14.3
	) Corrected a bug in the video mode generation for the high
		vertical resolution games with an Arcade/TV Monitor.
	) Some change at the `vbe3' driver. Now it logs a lot of
		details for a better debugging
	) Changed the source distribution. Now you must apply two
		patch. Check the build.txt file.
	) Some fix and improvement at the `cfg' utility.
	) Added support for saving the volume attenuation in
		the directory "att". [Rafael Prado Rocchi]
		You have to create the directory to enable this
		function.

AdvanceMAME Version 0.37b14.2
	) Removed the `[config] aspect' option. The aspect is
		4/3 as default.
	) Removed the `[config] video_factor_frequency' option.
		The game frequency is now matched implicitly.
	) Renabled the `[config] scanlines' option. Now video modes are
		generated and chosen with scanlines if available.
		Note that you generally need a wide Vclock range.
		Something like 55-125 Hz.
	) Added the `[config] magnify' option to suggest the use of
		a double resolution.
	) Added the `[config] rgb' to suggest the use of a true RGB mode.
		If you already used the `depth=16' option now you can
		set it to `depth=auto' and use `rgb=yes' instead.
	) Some fix and improvement at the modes generation and choice.
	) Some fix and improvement at the `cfg' utility.

AdvanceMAME Version 0.37b14.1
	) Added the configuration utility `cfg.exe' for the first
		time run.
	) Updated the `install.txt' file for the `cfg' utility.
	) Added the `[config] videogenerate' option to automatically
		create perfect video modes. Check the `mame.txt' and
		`install.txt' files for other info.
	) Solved a "Division by Zero" bug caused by
		the incorrect use of textmode modelines.
	) The "bliteffect = auto" option now select the "filter"
		effect if the game is stretched by a factor equal or
		greater than 2.
	) The `pentium' version now uses the MMX if available.
	) Added the `speed' utility to check the memory performance of
		your video board.

AdvanceMAME Version 0.37b14.0 2001/04
	) Added a new type of effects to simulate the aspect of an Arcade
		Monitor with a PC monitor. Check the new option "rgbeffect".
		- triad 3 pixel (normal/strong)
		- triad 6 pixel (normal/strong)
		- trial 16 pixel (normal/strong)
		- scanline 2/3 vertical lines
		- scanline 2/3 horizontal lines
	) General optimization at the blitter. A lot of new MMX
		code and some slowdown bugs fixed.
	) Added the new "Video" menu entry "Show Pipeline".
		This option show the current Blit Pipeline stages.

AdvanceMAME Version 0.37b13.0 2001/03
	) Solved the throttle problem when the sound is disabled
	) Linked with the correct SEAL library. This solve the problem
		with some PCI Sound Blaster
	) Some change at the F5 command (generate video mode) of
		the `mv' utility

AdvanceMAME Version 0.37b12.0 2001/02
	) The scripts are terminated correctly also if the
		program crash.
	) Better system recover if the program crash.
		Now the system should be more stable.
	) Solved some "Page Fault" error in systems with low memory.
		Now you can run any game also with only
		64 Mbytes of memory.
	) Increased the maximum turbospeed to 30 and turbostartuptime
		to 180 seconds
	) Solved a precision time problem of the scripts. Now they work
		also if the frame rate is very high (> 1000)
	) Now you can use the joystick, the keyboard and the mouse
		together like the official MAME.
	) Added the support for the alpha blending and for the
		color conversion to any video mode depth. You can use
		the alpha blending also with 8 and 16 bits video modes.
		The best speed is with 15 bits video modes.
		The 32 bit video modes and the blit level rotation are
		currently unsupported.
	) Minor speedup for the case with "depth=16" and a 8 bit video mode.
	) Compiled with Allegro WIP 3.9.34 like the current MAME
	) Added the new default mode 384x224 mainly for the CPS2 games
	) Added the new config option `aspect'. This option control
		how compute the aspect ratio of the game emulated.
		The default behavior is different than the previous
		version. Check the file `mame.txt'

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
		using the VESA/PM services if available.
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
		others purpose.
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

AdvanceMAME Version 0.37b11.1
	) Corrected the confusion of the "[config] video"
		and "[script] video" options.

AdvanceMAME Version 0.37b11.0 2001/01
	) Removed the option `turbostartupspeed'. Now it's always at the
		maximum speed available and at frameskip 11.
	) Autoframeskip is automatically enabled when the turbo button
		is pressed
	) Corrected the automatic video mode choice.
	) Added the `video_factor_game_stretch' option. Now the
		'video_factor_*' options are documented in the
		'mame.txt' file. With these options you can control
		how the auto video mode is chosen.
		If you get incorrect results try to delete them and
		restart with the default values.
	) Renamed the option `video_factor_strecth' in `video_factor_aspect'.
	) Added scripts! You can control any external hardware devices
		using a like C scripts. See the new script.txt file.
	) Added the option `video_mode_reset' to prevent the
		video mode reset at the emulator exit

AdvanceMAME Version 0.37b10.0 2000/12
	) Added support for Trident cards using the new video
		driver of VSyncMAME written by Saka.
		Use the "vbeline_driver=vbe3" option to revert to the
		old behavior.
	) Minor change at the `trident' driver for 16 bit modes
	) Added two new `turbostartupspeed' and `turbostartuptime' options to
		speed up the game startup.
	) Added a new `turbo' button (num pad *) and a new option
		`turbospeed' to speed up the game play
	) Added two new simple effects `filterx' and `filtery'
	) Changed the coinage support. Now it works for all
		the games because it intercepts the coin keys and it
		doesn't require the explicit driver support.
	) Corrected some garbage on video when using artworks

AdvanceMAME Version 0.37b9.0 2000/11
	) Added the capability to save sound preview. Use `backspace' to start
		and `enter' to stop the recording.
		The file is saved in the `snap' directory in the `wav'
		format. The frequency rate is the value specified
		with the mame.cfg `samplerate' option.
		Use the mame.cfg `recordtime' option to limit the maximum
		record length in seconds.
	) Corrected some color palette issues in the `vbe/vbeline' modes.
	) Rewritten the choice of the best video mode. Now you can control
		the process with the new mame.cfg `video_factor_*' options.

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

AdvanceMAME Version 0.37b8.0 2000/10
	) Recovered from an harddisk crash!
	) Halved the dotclock of interlaced modes in the "ati" driver
	) Updated the contrib/nick directory with better instructions
		for newbies and for the use with ArcadeOS
	) Added the contrib/prophet directory with example settings
		for the Princeton AR27T Monitor.

AdvanceMENU Version 1.3.0 (0.37b8.0) 2000/10
	) Better support for MESS
	) Corrected some bugs related to long filenames and filenames
		with spaces or dots
	) Added the command menu (F8) to do some file operations
	) Added support for "cabinets" snapshot
	) Corrected the proportional font display
	) Added a fast move to the next system with the INS and DEL keys.

AdvanceMAME Version 0.37b7.0
	) Corrected the aspect ratio of some games with
		VIDEO_ASPECT_RATIO_1_2 and ORIENTATION_SWAP_XY attributes.
	) Corrected some audit options like -verifyroms,...
	) Added the utility `portio' to drive the hardware ports.
	) Added the utility `off' to shutdown the PC.
	) Corrected a cheat menu bug (MAME bug)

AdvanceMENU Version 1.2.0 (0.37b7.0)
	) Added support for MESS
	) Added support for loading GRX fonts
	) Added support for proportional fonts
	) Added some new GRX fonts in the contrib/fonts directory,
		you can found the complete (big) set at
		ftp://x2ftp.oulu.fi/pub/msdos/programming/djgpp/v1tk/cbgrf103.zip


AdvanceMAME Version 0.37b6.0
	) Corrected the aspect ratio of some vectors games.
	) Minor correction at the `vbe3' driver
	) Added the option `[config] vsync_adjust'. If it's set to `yes'
		AND the `vsync' is enabled all the `vbeline' modes
		are adjusted to match the exact frame rate of the
		emulated game. This option makes sense only with a PC
		multisync monitor which can accept a wide range of
		vertical and horizontal frequencies.
		Try with :

		:[config]
		:vsync = yes
		:vsync_adjust = yes

	) Solved some bugs in the `r128' driver. Now working.
	) Now `mame' doesn't add any default video modes if the list
		defined in the `mame.cfg' file is empty. You MUST use
		`mv' to select the video modes available.
	) Now the required hclock/vclock specification for a LCD screen are:
		:[video]
		:hclock = 0
		:vclock = 0
	) The default keys for the video mode change at runtime are now
		`,' (COMMA) and `.' (STOP) for preventing clash with the
		hotrod settings.
	) If the exit is done by the idle timeout the exit code is !=0
	) Removed the complete clear of all the video memory for
		problems with some NeoMagic cards (possible Segmentation
		fault)
	) Added and recomputed a lot of video modelines

AdvanceMENU Version 1.1.0 (0.37b6.0)
	) Corrected the timing in the `idle_start' option, now
		the timer also reset if the user exit from `mame'
		pressing ESC (requires AdvanceMAME 0.37b5.1 or greater)
	) When the clone aren't listed and you start a game not emulated
		correctly the best clone is run instead. If you want
		to force the run of the bad game use the F12 command.
	) If the screenshot isn't found it's searched the screenshot of any
		available clones or parent games.
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

AdvanceMAME Version 0.37b5.0 2000/08
	) Added the video driver `r128' for the ATI Rage 128 boards
	) Added the video driver `neomagic' for the NeoMagic boards,
		very common on laptops
	) Updated the video driver `s6326' to the last VSyncMAME
		0.37b5 driver.
	) Restructured the others video drivers.

	If you have problem with these new drivers you can
	disable them with the option:
		:[video]
		:vbeline_driver = no

	) Added the contrib/xfree directory with some very useful
		technical documentation of video stuffs
	) Changed the display format of the video mode list in `mame'
	) Updated a lot the file `mame.txt'
	) Added the support for a second mouse imported from OptiMAME
		by Andy Geez. You can found the custom mouse driver
		in the contrib/optimous directory.

AdvanceMENU Version 1.0.0 (0.37b5.0) 2000/08
	) Added a new menu mode to the utility `mm' which display
		a lot of tiled preview at the same times. Press TAB to
		cycle from one mode to another.
	) Now the utility `mm' is available also as a standalone project
		named AdvanceMENU

AdvanceMAME Version 0.37b4.0 2000/06
	) Added `cirrus', `s3' and `matrox' video drivers
	) Added the `-log' option at `mv'
	) Added the `-nosound' option at `mv'
	) Reinserted the mame option `triplebuffer'. If you want to activate
		the buffering you must insert the `triplebuffer = yes'
		option in your mame.cfg. The default is with buffering
		disabled like the official mame. (auto->yes, missing->no)
	) Added some new `vbeline' modes for PAL/NTSC
	) Added the resize command with the CTRL+ARROW keys in the `mv'
		test screen

	I am interested in functionality reports with these information:
	1) Model of your video board
	2) The runtime log file "mv.log" generated with the command
	"mv -log"  when you test (pressing ENTER) all the `vbeline'
	video modes available.
	3) The list of `vbeline' video modes that display incorrectly

AdvanceMAME Version 0.37b3.1 (never released)
	) Corrected a lot of bugs in the blitters when the rotation
		and effects are active together (segmentation fault)
	) Corrected a bug in the palette conversion from 16 to 8 bit
		(slow down)
	) Corrected a bug in `mm' if no snap was present for the first
		selected game (blank screen)
	) Added the `max' effect also in rgb modes
	) Corrected the save function of the `video' option
	) Changed the default of the `resize' option from `integer'
		to `mixed'
	) Changed the `mm' savescreen to fullscreen size
	) Added the save state for the list of types and groups selected
		in the `mm' utility
	) Corrected the save function of the video options by
		resolution with a vertical monitor
	) Added an internal font in the `mm' utility that now
		doesn't require the file `mm.fnt'
	) Added the automatic creation and updating of the file `info.txt'
		in the `mm' utility
	) Added all the VSyncMAME drivers. See the new option
		`vbeline_driver' in the `mv.txt' file. Very
		thanks to Saka!
	) Converted the `mame' ati video driver from the original MAME source.
	) Added the center command with the ARROW keys in the `mv'
		test screen
	) Minor changes and corrections of the mame menu interface
	) Added the `merge disable' option at the `mm' utility

AdvanceMAME Version 0.37b3.0 2000/05
	) Added to the source distribution the missing files
		`blitmax.h' and `blitrot.h'
	) Removed the file `makefile.dif' from the source distribution,
		now you can use the standard `mame' Makefile
	) Corrected minor bugs in the `Analog Controls' and
		`Video Config' menu

AdvanceMAME Version 0.37b2.2 (never released)
	) Corrected the `mm' option merge
	) Corrected the size of the vectors game
	) Corrected the save function of the bliteffect option

AdvanceMAME Version 0.37b2.1 2000/05
	) Added to the `mm' utility the ability to save the
		position of the cursor between runs
	) Added to the `mm' utility the options `merge' to select
		the format of your roms.
	) Corrected the bug that prevented the correct loading and saving
		of the nvram files
	) Added a new configuration method based on the resolution of
		the games. All the `[config]' options in the `mame.cfg'
		file are now read in three different sections in this
		order:

		[GAME] - the short game name like [pacman]
		[RESOLUTION] - the resolution of the game like [244x288]
			for raster games or [vector] for vector games
		[config] - the default section

	) Inserted the option `dirty' like the current `mame', use
		`dirty = no' to disable the dirty buffer.
	) Added to the `mm' utility a software key repeat for who use
		the hotrod console.
	) Added a new menu option `Video Config' for selecting at runtime
		some video options like: video mode, effects, ...
	) The video mode for any game is now saved only if
		requested with the appropriate menu entry.
	) Changed the analog input stick behavior, read the
		section `ANALOG INPUT STICK' in the file `mame.cfg'

AdvanceMAME Version 0.37b2.0 2000/05
	) Corrected a bug that prevent the selection of the input codes
		for a specific game
	) Corrected the problem of invisible columns on the right for
		some games. This problem is still presents in the complex
		blitters (stretch/rotate).
	) Corrected a bug in the alignment of the memory of the visible
		part of the game bitmap.
	) Moved and changed the `mm' option "idlestart" from the file
		mame.cfg to "idle_start" in the file mm.cfg
	) Added the `mm' option "idle_move" to the file mm.cfg to realize
		a basic slide show
	) Added the game screen size information in the `mm' utility
	) Updated the `mm' documentation file `mm.txt'
	) Corrected the gamma modification at runtime

AdvanceMAME Version 0.37b1.0 2000/04

AdvanceMAME Version 0.36r2.2
	) Added a warning screen for preventing the use of BIOS modes
		with low frequency monitors
	) Corrected the parsing of fixed value in the "hclock=" option
	) Added memory for skiplines and skipcolumns options for
		the single game
	) Other minor changes

AdvanceMAME Version 0.36r2.1
	) Corrected the error "Too low virtual x size" with some
		bogus VBE3 implementation
	) Corrected and extended the vbeline modes
	) Added the '[video]' option 'vbe_tweak=moderate|aggressive'
	) The specifications of '[video]' options 'hclock'
		and 'vclock' are required
	) Added hardware mode grabbing in command F6 in 'mv' for all modes
	) Added customization of horizontal/vertical/pixel clock in
		command F5 in 'mv'
	) Added mode number change for vgaline/vbeline in command F7 in 'mv'
	) Changed the calibration screen in command F9 in 'mv'
	) Changed the list of default video mode in 'mv', added a lot of
		default vbeline modes
	) Documentation update

AdvanceMAME Version 0.36r2.0

AdvanceMAME Version 0.36r1.0 (never released)
	) Corrected a bug in the video mode change causing a freeze if the
		cheat option is active
	) Corrected a bug in the vsync code causing a crash at the startup of
		many games
	) Default video mode change keys are now '[' and ']'
	) Added a memory for the manually selected video configuration

AdvanceMAME Version 0.36rc2.0 2000/03
	) Corrected a bug causing memory corruption in video mode change
	) Added absolute tick values in the profiler
	) Corrected the sort menu of the 'mm' utility
	) Changed the sort order of video modes in the 'mv' utility
	) Corrected a bug in vsync code causing (sometimes)
		a big slowdown if 'frameskip=auto' is active
	) Video mode change at runtime disabled if vsync is active, you can
		still change the video configuration on the same video mode

AdvanceMAME Version 0.36rc1.1 (never released)
	) Added a startup check for the presence of a mmx cpu if required
	) Corrected a bug in color of the user interface in 16 bit modes
		causing wrong colors
	) Corrected a bug if stretch and filter activated together causing
		incorrect video output
	) Corrected a bug that permits to use unaligned bitmap causing a loss
		of performance with mmx
	) Corrected a bug in vbe 8 bit modes with a palette of 8 bit causing
		incorrect colors

AdvanceMAME Version 0.36rc1.0 2000/02
	) MAME compiled with Allegro WIP3931 (-march=i586), utilities
		compiled with WIP3928 (smaller)
	) Added special blitters for unchained double and palette conversion
	) Added new bliteffects for rgb video modes
		mean - merge lines with the mean colors
		filter - apply a low pass filter
	) MAME compiled with a patched version of SEAL with improved sound
		quality
	) Corrected some minor bugs on the blitrotate system
	) Added the Advance Mame logo [Melanie Burns]

AdvanceMAME Version 0.36b16.2 (never released)
	) Added basic asm/mmx blit routines for x1,x2 horizontal stretch:
		the pentiumpro target uses asm routines with mmx instructions
		the pentium target uses asm routines with shift/swap instructions
	) Added cache for input functions at core level
	) Utility `mm' flush smartdrv cache before running games
	) Added utility `mk'
	) Correct bug of partially black screen with video conversion
		from 8 to 16 bit
	) Video option bliteffect= for selecting the image
		reduction algorithm (when the screeen is too small for
		the complete image). Modes available :

		no - skip some lines
		max - plot the lightest pixel (work very well for
			pacman style games). Avaliable only if the
			game uses a fixed palette.
		auto - automatic selection (list based)
	) Added video options blitrotate = auto, automatic selection (list based)
	) Added new aos.txt and mame.dat for AOS [Nick Bourdo]
	) Video configuration modifiable at runtime, press HOME/END,
		use the -log option for viewing the list of available
		video configuration for one game
	) Added patch src.dif to apply at mame core sources for compiling
	) Added `mame' configuration option "idleexit = SECONDS" for
		an automatic exit after SECONDS of inactivity
	) Added `mm' configuration option "idlestart = SECONDS" for
		an automatic random start after SECONDS of inactivity
	) Documentation changes

AdvanceMAME Version 0.36b16.1 2000/02
	) Correct a bug in some blit routines, specifically in blitting
		in a 8 bit mode with a 16 bit palette
	) Reinsert dedicated and faster blitting routines when no rotation
		is required

AdvanceMAME Version 0.36b16.0 2000/02
	) Added rotation at blit level, use the new option "blitrotate = yes"
		for games that don't rotate correctly (ex. NeoGeo)
	) Renamed the option "stretch" to "resize"
	) Minor modifications at errorlog
	) Minor modifications at mv
	) Substituted the option `resize = yes' to `resize = integer'

AdvanceMAME Version 0.36b15.1 2000/01
	) Correct the crash if "vsync" is active and no modes available
	) Correct bug of missing horizontal lines if "scanline" is active
	) Better output in errorlog.txt
	) Correct bug in the input system

AdvanceMAME Version 0.36b15.0 2000/01
	) Created the history.txt file

AdvanceMAME Version 0.36b14.0 2000/01

AdvanceMAME Version 0.36b13.0 1999/12

AdvanceMAME Version 0.36b13.0 1999/12

AdvanceMAME Version 0.36b12.0 1999/12

AdvanceMAME Version 0.36b11.0 1999/12

AdvanceMAME Version 0.36b10.0 1999/11

AdvanceMAME Version 0.36b9.0 1999/11

AdvanceMAME Version 0.36b8.0 1999/11

AdvanceMAME Version 0.36b7.0 1999/10

AdvanceMAME Version 0.36b6.0 1999/09

AdvanceMAME Version 0.36b5.0 1999/09

AdvanceMAME Version 0.36b4.0 1999/09

AdvanceMAME Version 0.36b3.0 1999/08

AdvanceMAME Version 0.36b2.0 1999/08

AdvanceMAME Version 0.35.0 1999/07

AdvanceMAME Version 0.35rc2.0 1999/06

AdvanceMAME Version 0.35rc1.0 1999/06

AdvanceMAME Version 0.34rc1.0 1998/12

AdvanceMAME Version 0.34b8.0 1998/12

AdvanceMAME Version 0.34b7.0 1998/12

AdvanceMAME Version 0.34b6.0 1998/11

AdvanceMAME Version 0.34b5.0 1998/10

AdvanceMAME Version 0.34b4.0 1998/10

AdvanceMAME Version 0.34b3.0 1998/09

AdvanceMAME Version 0.34b2.0 1998/09

AdvanceMAME Version 0.34b1.0 1998/08

AdvanceMAME Version 0.33.0 1998/08

AdvanceMAME Version 0.33b8.0 1998/08

AdvanceMAME Version 0.33b7.0 1998/07

AdvanceMAME Version 0.33b6.0 1998/07

AdvanceMAME Version 0.33b5.0 1998/06

AdvanceMAME Version 0.31.0 1998/05

