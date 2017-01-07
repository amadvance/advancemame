Name{number}
	install - AdvanceMAME Installation

	This document describes the installation and configuration
	process of AdvanceMAME, AdvanceMESS and AdvanceMENU.

System Requirements
	To run the Advance programs you may need to install and
	configure some drivers and libraries on your system.

	The following is a list of all that you need to do.

	More details on the single drivers are present in the
	`advdev.txt' documentation file.

  Linux
	In Linux the Advance programs can run either in the
	X Window graphical environment or from the Linux Console.

	In X Window the SDL library is used for everything: video,
	audio and input controllers.

	In the Linux Console, the Linux Framebuffer is used for the
	video, the ALSA or OSS library for audio, and the Linux Event
	or Raw interface for input controllers.

	The video board programming functionality for Arcade Monitors
	and CRT TVs, is available only from the Linux console, using
	the Linux Framebuffer.

  Raspberry Pi
	With a Raspberry Pi you have the same support you have in Linux,
	plus some specific Raspberry functionality, like the accelerated
	Framebuffer.

	With a Raspberry, it's recommended run the Advance program directly
	from Linux Console because using the Linux Framebuffer you also get
	hardware acceleration that is missing when run from the X Window system
	using the SDL library.

	This is the best operating mode when using a modern monitor or flat TV,
	and it doesn't need any configuration step. It just works.

	With a Raspberry Pi 3 you can expect to run most of the games at
	full speed, with vsync enabled, even when using the Scale2x effect.

	If instead you want to use an old CRT Arcade or TV screen, you can
	configure the Advance programs to generate modelines customized
	for your video hardware, following the 'Video Setup' chapter.

    HDMI Port
	To use the programmable modelines with the Raspberry HDMI port, you
	need a kind of HDMI->VGA or HDMI->SCART converter to connect it to a
	CRT monitor.

	You don't need any special option in your '/boot/config.txt', just the
	options needed to start the Raspberry with your video hardware,
	and be able to enter commands.

	Whatever hdmi_mode you select, the Advance programs will use mode 87
	group 2, restoring your selected original mode when finished.

    DPI Port
	To use the programmable modelines with the Raspberry DPI port you
	need a GPIO add-on board like the Gert's VGA 666.

	The Raspberry Pi has strong limitations on the lower range of pixel
	clocks when using the DPI/GPIO interface, that will affect your
	ability to control low frequency monitors, like Arcade screens.

	The only allowed pixel clocks suitable for low resolutions
	are 4.8 MHz, 6.4 MHz, 9.6MHz and 19.2 MHz.
	You can instead choose any pixel clock greater than 31.25 MHz.

	To overcome this problematic, AdvanceMAME transparently increase
	the modeline horizontal size, until it reaches the 31.25 MHz pixel clock.
	Don't be surprised if a modeline that works in AdvanceMAME, doesn't
	work when setting manually the timings in other ways.

	To have the programmable modes working, you have to start the
	Raspberry with the custom mode 87 group 2 in '/boot/config.txt'.

	For example, with an Arcade Monitor, you can use:

		:dpi_group=2
		:dpi_mode=87
		:hdmi_timings=320 1 20 29 35 224 1 10 14 16 0 0 0 60 0 6400000 1

	With a standard SVGA Monitor, you can use:

		:dpi_group=2
		:dpi_mode=87
		:hdmi_cvt=1024 768 60

	Usually you have to also add other options, for example with a Raspberry
	Pi 3 and a Gert's VGA 666, you need also:

		:device_tree=bcm2710-rpi-3-b.dtb
		:dtparam=i2c_arm=off
		:dtparam=spi=off
		:dtparam=uart0=off
		:dtparam=uart1=off
		:dtoverlay=pi3-disable-bt-overlay
		:dtoverlay=vga666
		:enable_dpi_lcd=1
		:display_default_lcd=1
		:force_pwm_open=0
		:dtparam=audio=on
		:gpu_mem=128

  Windows/Mac OS X
	In Windows and Mac OS X, the Advance programs use the SDL library
	for video, audio, and input controllers.

	In the binary distribution for Windows the SDL library is already
	included.

  DOS
	In DOS, the Advance programs already contains all the required drivers
	for video, audio, and input controllers.

	Generally you don't need to install additional software with
	the exception of a mouse driver if you want to use one.

Installation
	In Linux and other unix, the recommended way to install the Advance
	programs is to start from the sources.

	The build process is detailed in `build.txt' file, but usually
	you need only the usual commands:

		:./configure
		:make -j3
		:sudo make install

	The exceptions are the DOS, Windows and Raspberry targets.
	In such cases it's better to start from the binary distribution
	and don't compile the source.

Configuration
	All the Advance programs require a configuration file to work.
	They are text files with the same program name, with the ".rc"
	extension, like "advmame.rc".

	Default configuration files are created by the programs when they
	are started for the first time.

	In Linux, Mac OS X and other Unix, the configuration files are
	created in the user home directory in the subdirectory .advance/.
	In Windows and DOS the configuration files are created in the
	current directory.

	When you have finished to modify the configuration files,
	it's recommended to run the programs with the `-remove' option
	to remove all the default options from the configuration files.

  AdvanceMAME
	To run AdvanceMAME you need at least to configure the `dir_rom'
	option to point where your roms reside.

	In Linux, Mac OS X and other Unix the default rom dirs are
	/usr/local/share/advance/rom and $HOME/.advance/rom.
	In Windows and DOS is the rom/ subdirectory in the current directory.

	If you want to run it with a with the standard video support, like any
	other application, you don't need to configure any video options.

	Instead, if you want to enable the direct programming of the
	video board you need to carefully follow the "Video Setup" chapter
	in this file.

  AdvanceMENU
	To run AdvanceMENU you need to configure which emulator is
	present in your system.

	At the first run AdvanceMENU searches for all the known emulators
	and configure itself automatically. In Linux, and Mac OS X the
	emulators are searched in the current PATH list, in Windows and DOS
	only in the current directory.

	Likely you need also to adjust the path where the
	game's .png, .mp3 and .mng snapshot files reside with the
	'emulator_altss' option.

	In Linux, Mac OS X and other Unix the default snapshot dirs are
	/usr/local/share/advance/snap and $HOME/.advance/snap.
	In Windows and DOS is the snap/ subdirectory in the current directory.

Video Hardware
	The Advance programs are able to drive different types
	of video hardware: LCD, Multi Sync monitors, Fixed
	Sync monitors, Arcade monitors and CRT TVs.

	With the exception of PC Monitors that always accept the
	VGA signal, for other monitors you should take in account
	the required video signals and eventually use conversion
	circuits to adapt signals.

	A lot of useful links are available on the AdvanceMAME video
	link page:

		+http://www.advancemame.it/video-link.html

  LCD monitors or LCD TVs
	LCD screens have always a native fixed resolution. To get the
	best image quality it's always recommended to use this resolution.

	For this reason you usually don't need to enter any specific
	configuration options, and let the program to use the default
	video mode.

	This mode of operation is the default when you are in
	a graphics environment like X, Windows and Mac OS X.

  Multi Sync monitors (or CRT PC monitors)
	Multi Sync monitors support a wide range of horizontal clocks
	and requires a standard VGA connector. They are the normal PC
	monitors.

	With PC monitors you can choose to work without any configuration,
	using only the default video mode, like a LCD screen, or configure
	the clocks supported to allow a direct video mode generation
	by the Advance programs.

	You can generally find the range of clocks supported in the
	monitor manual, generic values are:

		:device_video_clock 10 - 150 / 30.5 - 60 / 55 - 130

  Fixed Sync Monitors
	Fixed Sync monitors support only a few fixed horizontal clocks.
	Generally they requires separate 3,4 or 5 BNC connectors, with
	the exception of old VGA monitors which requires a standard
	VGA connector.

	You must find the exact clocks supported in the monitor manual.

	The standard clocks for VGA monitors are:

		:device_video_clock 10 - 50 / 31.5 / 55 - 130

	If the monitor uses separate H/V sync signals you can
	directly use the VGA sync signals of your PC. If the monitor
	uses composite sync, or sync-on-green you must use a sync
	converter circuit.

	These monitors are generally compatible with the VGA video
	signal level of 0.7 V p-p.

  Arcade Monitors
	Arcade monitors support only a few fixed horizontal and
	vertical clocks, generally the horizontal are 15.75 and 25 kHz,
	and the vertical 60 Hz.

	You must find the exact clocks supported in the monitor
	manual.

	Please note that the manuals of some Arcade Monitors
	incorrectly state a wide range of horizontal frequency
	like 15 - 31 kHz. Generally these monitors support only
	the three fixed clocks of 15.75, 25, 31.1 kHz. An example
	is the Wells-Gardner D9200.

	The standard clocks for a Standard Resolution 15 kHz (CGA) are:

		:device_video_clock 4 - 50 / 15.75 / 60

	for a Extended Resolution 16 kHz are:

		:device_video_clock 4 - 50 / 16.5 / 53

	for a Medium Resolution 25 kHz (EGA) are:

		:device_video_clock 4 - 50 / 25 / 60

	If your monitor is multistandard, you can use more
	clock specification separating them with the `;' char.

	For example:

		:device_video_clock 4 - 50 / 15.75 / 60 ; 4 - 50 / 25 / 60

	If the monitor accepts separate H/V sync signals at
	levels 0 - 5 V you can directly use the VGA sync signal of
	your PC. If the monitor uses another sync signal you must
	use a sync conversion circuit.

	If the monitor accepts a composite sync signal, instead of
	using a sync conversion circuits you can also try twisting
	the two H and V VGA signal together. It works if you select
	VGA negative H and V sync on the programs. To be on safe side
	I DO NOT RECOMMEND THIS HACK to connect sync signals directly
	together. Technically you should never just tie sync
	signal lines together. They are not usually designed for
	this, so this can damage your video card. If you try, use
	with caution.

	You must also ensure that the monitor accepts the VGA video
	signal level of 0.7 V p-p. Generally arcade monitors require a
	video signal between 1 V and 5 V for each line.
	Therefore if you attempt to drive an arcade monitor with a
	VGA video signal you will at most, have a very dark picture with
	no contrast. You will need to buy/make an amplifier for each
	line in order for it to work.

  CRT TVs
	CRT TVs generally supports only two fixed combination of
	horizontal and vertical clocks, corresponding at the TV PAL
	and NTSC standards.

	Clocks values for PAL TV (European) are:

		:device_video_clock 4 - 50 / 15.62 / 50

	for NTSC TV (USA) are:

		:device_video_clock 4 - 50 / 15.73 / 60

	for PAL TV (European) which supports also NTSC TV (USA) modes
	(common if you use the SCART input):

		:device_video_clock 4 - 50 / 15.62 / 50 ; 4 - 50 / 15.73 / 60

    TVs with SCART
	If your TV has a SCART input, you can use directly the VGA
	analog signal as RGB input.

	A composite sync signal is required. It means that you must
	use a sync converter circuit to convert the VGA H/V sync with
	levels 0 - 5 V to a composite sync of levels 0 - 0.3 V.

	The SCART input accepts also a composite video signal, but
	the RGB video is far superior.

	Remember what to enable the SCART RGB signal you must set
	the SCART pin 16 at level 1 - 3 V (no more than 3 V).
	And to automatically switch the TV to the AV signal you
	must set the SCART pin 8 at level 9.5 - 12 V (for some
	TVs 5 V may be enough).

	If you have a recent VGA board, you can use the 5 V power
	available on the VGA pin 9. Alternatively you can use the
	5 V and 12 V PC internal power.

    TVs with S-Video
	If your TV has a S-Video input, you can use the TV-Out signal
	of your VGA board.

	The quality of the S-Video signal is near at the quality of
	the RGB signal.

	Unfortunately the TV-Out signal is generally not enabled
	by the Advance programs. There are some external utilities
	for Linux and Windows able to enable the TV-Out signal,
	but their use is mainly untested.

Video Setup
	The Advance programs have the ability to directly control your video
	board to get the best possible fullscreen video modes with always the
	correct size and aspect ratio.

	This feature is available in Linux with the Frame Buffer drivers.
	The legacy SVGALIB support for Linux/Windows/DOS is still present,
	but likely too old to support your video board.

	To make it possible, the programs need some information on your
	monitor capability in the form of the supported pixel, horizontal
	and vertical clocks.

	With thies information the programs are able to always generate
	`perfect' video modes for the emulated game.

  Operation Modes
	The programs support two basic ways to generated video modes:
	the `automatic' and the `manual' operation mode.

	In the `automatic' mode the programs automatically generate
	a video mode from scratch. It's the simplest mode of operation.

	In the `manual' mode the programs pick the video mode from a
	manually defined list of modelines, eventually adjusting them
	to match the game clock or size requirements.
	This mode of operation should be used only if the `automatic'
	mode doesn't work.

    Automatic Operation Mode
	In the automatic operation mode the programs automatically
	create a `perfect' video mode for the game to be emulated
	that fit exactly the whole screen with the correct aspect
	and frame rate.

	To configure and activate this mode you need to run the
	`advcfg' utility for AdvanceMAME and `advcfg -advmenuc' for
	AdvanceMENU, and answer at the various questions.
	You don't need to create a list of video modes, any needed
	video mode is created at runtime.

	Before running the `advcfg' utility you should check your
	monitor manual for the vertical and horizontal clocks
	supported by your monitor.

	The `advcfg' utility adds these options in your `advmame.rc':

		:display_mode auto
		:display_adjust generate_yclock
		:device_video_clock ?
		:device_video_format ?

	All these options are documented in the `advdev.txt'
	and `advmame.txt' files.

    Manual Operation Mode
	In the manual operation mode the programs scan a list of `good'
	video modelines created manually and chose the best available.
	You must in advance create this list of video modelines with the
	`advv' utility.

	This is the description of the few basic steps required to run
	the programs in the manual operation mode. All the options used
	are documented in the `advdev.txt' file.

	) For AdvanceMAME add in the in the file `advmame.rc' these
		options:

		:display_mode auto
		:display_adjust x

	) Add in the file `advmame.rc' or `advmenu.rc' the
		`device_video_clock' option that specify which
		pixel, horizontal and vertical clocks are supported
		by your monitor.
		Generally these values are specified in the technical
		page of your monitor manual. You can see the
		"Video Hardware" chapter in this file for some
		examples.

	) Run the `advv' program for AdvanceMAME or `advv -advmenuv' for
		AdvanceMENU.

	) Test the video modelines of your interest pressing ENTER
		on them. If the mode isn't centered try centering it
		with the ARROW keys. When you have finished press ENTER
		to save your modifications or ESC to restore the
		previous setting.
		Returned in the video mode list, if the mode is
		displayed correctly, you can select it to be used by
		the programs pressing SPACE.
		It's very important that in all the selected modes
		the screen area is completely visible. Otherwise,
		when playing, part of the game may be out of screen.
		Video modes displayed in red aren't supported by
		your video hardware.

	) When you have selected the list of `good' video modes press
		F2 to save them in your configuration file.

	) Press ESC to exit from `advv'

	In the `contrib/modeline' dir are present some .rc file with
	some example modelines. The same modelines are contained in
	the `advv' program.

Copyright
	This file is Copyright (C) 2003 - 2017 Andrea Mazzoleni.

