Name
	install - Installation and Configuration

System Requirement
	To run the Advance programs you first need to install and configure
	some drivers and libraries on your system.

	More details on the single drivers are present in the `advdev.txt'
	documentation file.

  Linux
	To allow the Advance programs to directly control your video board
	in console mode, you must install and configure the Linux Frame
	Buffer driver or a recent SVGALIB library.

	The Linux Frame Buffer drivers are always included in the Linux
	kernel source, but generally they must be explicitely compiled
	or loaded. Please note that you cannot use the VESA Linux Frame
	Buffer driver, you must use a driver specific for your video board.

	The SVGALIB library must be installed manually. Generally the Linux
	distributions contain an old 1.4 version of the library, but the
	Advance programs you need at least the version 1.9.14 available at:

		:http://www.svgalib.org

	For both the Frame Buffer driver and SVGALIB library some additional
	patches are available in the `contrib/' directory.

	To allow the Advance programs to display in X you must install the
	SDL library. Generally it's already present in all the recent
	distributions.

	To allow the Advance programs to play sounds, you must have
	installed the OSS or ALSA audio system. Generally at least one
	is always available in all the Linux distributions.
	If you have the SDL library, it can also be used for the sound
	output.

	The Advance programs are able to use the Linux support for
	input controllers like keyboards, joysticks and mice.
	If you have the SDL library, it can also be used for the input
	controllers.

    DOS
	The Advance programs already contain all the required drivers
	for video, sound, and input controllers.

	Generally you don't need to install additional software with
	the exception of a mouse driver if you want to use one.

    Windows
	To allow the Advance programs to directly control your video board,
	you must install the included SVGAWIN driver.

	Please note that this driver is EXPERIMENTAL software and it works
	only for a few set of video boards. More information is present in the
	`svgawin.txt' documentation file.

	Otherwise the Advance programs are able to display in a window or
	to use the default video modes using the included SDL library.
	The SDL library is also used for sound, and input controllers.

    Mac OS X/Generic Unix
	To allow the Advance programs to work you must install the SDL
	library. This library is used for video, sound and input controllers.

	The SDL library is available at:

		:http://www.libsdl.org

First Time Configuration
	All the Advance programs require a configuration step to work
	correctly.
	
	If you want to run them as standard applications in a Window
	Manager environment like X Window, Windows or Mac OS X Acqua, you
	don't need to configure any video options.
	
	If you want to enable the direct programming of the video board
	you need to carefully follow the "Video Configuration" chapter.

	To create a default configuration files simply run the first time
	the application from a command shell and a standard configuration .rc
	file will be created.
	In Linux, Mac OS X and other Unix, the configuration file is created
	in the user home directory in the subdirectory .advance. In DOS and
	Windows the configuration file is created in the current directory.

	To run AdvanceMAME you need at least to set the dir_rom option to the
	path there the roms resides.

	To run AdvanceMENU you need to have at least one of the recognized
	emulator in the current search path in Linux and OS X or in the
	current directory for DOS and Windows.
	Generally you need also to adjust the path where the game's .png,
	.mp3 and .mng files reside with the emulator_* options.

Video Configuration
	The Advance programs have the ability to directly control your video
	board to get the best possible fullscreen video modes with always the
	correct size and aspect ratio.
	
	This features is available in Linux with the SVGALIB and Frame
	Buffer libraries, in DOS with the SVGALIB and VBELINE libraries
	and in Windows with the SVGAWIN library.
	It isn't available on Mac OS X and other Unix.

	To made it possible the programs need some information on your
	monitor capability in the form of the supported pixel, horizontal
	and vertical clocks.

	With these info the programs are able to always generate
	`perfect' video modes for the emulated game.

  Operation Modes
	The programs support two basic way to generated video modes:
	the `automatic' and the `manual' operation mode.

	In the `automatic' mode the programs automatically generate
	a video mode from scratch. It's the simplest mode of operation.

	In the `manual' mode the programs pick the video mode from a
	predefined list of modelines, eventually adjusting them to match
	the game clock or size requirements.
	This mode of operation should be used only if the `automatic' mode
	doesn't work.

	Please note that if you are using the `sdl' or `vbe' video drivers, 
	the programs aren't able to create or adjust video modes.
	In this case you don't need to configure anything because the programs
	can use only the video modes which your system reports as available.

	The `sdl' and `vbe' video drivers are only indicated to use AdvanceMAME 
	in a Window Manager system. Instead it's the preferred choice for 
	AdvanceMENU for the use with a normal PC monitor, because AdvanceMENU 
	doesn't require not standard video modes.

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

	The `advcfg' utility add these options in your `advmame.rc'
	and `advmenu.rc' :

		:display_mode auto (only for advmame)
		:display_adjust generate (only for advmame)
		:device_video_pclock ?
		:device_video_hclock ?
		:device_video_vclock ?
		:device_video_format ?

    Manual Operation Mode
	In the manual operation mode the programs scan a list of `good'
	video modelines created manually and chose the best available.
	You must in advance create this list of video modelines with the
	`advv' utility.

	This is the description of the few basic steps required to run
	the programs in the manual operation mode.

	) For AdvanceMAME add in the in the file `advmame.rc' these options:

		:display_mode auto
		:display_adjust x

	) Add in the the file `advmame.rc' or `advmenu.rc' the `p/h/vclock'
		options that specify which horizontal and vertical clocks are
		supported by your monitor.
		Generally these values are specified in the technical page of
		your monitor manual. These are some example :

	Generic PC SVGA multisync monitor :
		:device_video_pclock 10 - 150
		:device_video_hclock 30.5 - 60
		:device_video_vclock 55 - 130

	Generic PC VGA monitor :
		:device_video_pclock 10 - 50
		:device_video_hclock 31.5
		:device_video_vclock 55 - 130

	Generic LCD screen :
		:device_video_pclock 0 - 0
		:device_video_hclock 0
		:device_video_vclock 0

	PAL/SECAM TV (European) :
		:device_video_pclock 5 - 50
		:device_video_hclock 15.62
		:device_video_vclock 50

	PAL/SECAM TV (European) which supports also NTSC
	modes (very common if you use the SCART input) :
		:device_video_pclock 5 - 50
		:device_video_hclock 15.62, 15.73
		:device_video_vclock 49 - 61

	NTSC TV (USA) :
		:device_video_pclock 5 - 50
		:device_video_hclock 15.73
		:device_video_vclock 60

	Generic Arcade Monitor Standard Resolution 15 kHz (CGA) :
		:device_video_pclock 5 - 50
		:device_video_hclock 15.75
		:device_video_vclock 49 - 61

	Generic Arcade Monitor Medium Resolution 25 kHz (EGA) :
		:device_video_pclock 5 - 50
		:device_video_hclock 25
		:device_video_vclock 49 - 61

	Generic Atari Monitor Extended Resolution 16 kHz
		:device_video_pclock 5 - 50
		:device_video_hclock 16.5
		:device_video_vclock 53

	Please note that the manuals of some Arcade Monitors incorrectly
	state a wide range of horizontal frequency like 15 - 31 kHz.
	Generally these monitors support only the three fixed frequency
	15.75, 25, 31.1 kHz. For example the Wells Gardner D9200.

	) Run the `advv' program for AdvanceMAME or `advv -advmenuv' for
		AdvanceMENU.

	) Test the video modelines of your interest pressing ENTER on them.
		If the mode isn't centered try centering it with the ARROW keys.
		When you have finished press ENTER to save your modifications or ESC
		to restore the previous setting.
		Returned in the video mode list, if the mode is displayed correctly,
		you can select it to be used by the programs pressing SPACE.
		It's very important that in all the selected modes the screen area
		is completely visible. Otherwise, when playing, part of the game
		may be out of screen.
		Video modes displayed in red aren't supported by your video
		hardware.

	) When you have selected the list of `good' video modes press
		F2 to save them in your configuration file.

	) Press ESC to exit from `advv'

	In the `contrib/modeline' dir are present some .rc file with some
	example modelines. The same modelines are contained in the `advv'
	program.

  Video Troubleshooting
	) Delete any old configuration files and restart from scratch.

	) If you are using a PC Multisync monitor and the image is
		instable or the monitor automatically switch off, you have
		probably entered wrong clock values.
		Check the horizontal and vertical clock ranges supported
		by your monitor in the monitor manual. Eventually try
		with shorter ranges. Try for example with:

		:device_video_vclock 55 - 90
		:device_video_hclock 31 - 50

	) If you are using an Arcade Monitor/TV and the image is
		instable or completely black try increasing the lower
		pclock limit. Some video boards aren't able to output
		too low clocks. Instead of 5 try 8, 9, 10, 11, 12, ...
		Try for example with:

		:device_video_pclock 8 - 50

    Linux
	) If you are using the `svgalib' driver ensure that you have installed
		the most recent SVGALIB library. The old 1.4.x versions are
		not supported.

	) If you are using the `fb' driver, ensure to don't use the VESA
		Frame Buffer. It doesn't work for the Advance programs.

	) If you are using the `fb' driver check the kernel patches
		in the `contrib/fb' directory.

	) If you are using the `svgalib' driver check the svgalib patches
		in the `contrib/svgalib' directory.

	) Try forcing the use of the `fb' driver instead of the
		`svgalib' driver with the option:

		:device_video fb slang

    DOS
	) Try forcing the use of the `vbeline' driver instead of the
		`svgaline' driver with the option:

		:device_video vbeline vgaline

	) If your board has a VESA VBE 3.0 BIOS, try forcing the
		`vbeline/vbe3' driver with the options:

		:device_video vbeline/vbe3 vgaline

	) If you are using the `vbeline' driver try changing the `vbeline_mode'
		option:

		:device_video vbeline vgaline
		:device_vbeline_mode smaller

    Windows
	) Try reducing the video hardware acceleration.

Copyright
	This file is Copyright (C) 2003, 2004 Andrea Mazzoleni.

