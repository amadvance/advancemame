Name
	advv - The AdvanceVIDEO Manual Configurator

Synopsis
	:advv [-advmamev] [-advmessv] [-advpacv] [-advmenuv]
	:	[-vbev] [-vgav] [-videowv] [-log] [-logsync]
	:	[-rc RCFILE]

Description
	The `advv' utility is the main video configuration program
	for AdvanceMAME, AdvanceMESS, AdvancePAC, AdvanceMENU,
	AdvanceVBE and AdvanceVGA.
	It selects, creates and tweaks video modes interactively.

	This utility works differently if one of the -advmamev,
	-advmessv, -advpacv, -advmenuv, -vbev, -vgav and -videowv
	options is specified.
	The main difference is the the name of the configuration
	file used to store the video modes. Respectively they
	are `advmame.rc', `advmess.rc', `advpac.rc', `advmenu.rc',
	`vbe.rc', `vga.rc' and `videow.rc'.

	When the program starts a list of modelines is printed. You can
	walk on the list with the UP and DOWN arrows of your keyboard. 
	You can select the modelines to use in the emulator pressing
	SPACE.

	With the LEFT and RIGHT arrows you can select the bit depth of
	the video mode to test.

	To test a video mode you can press ENTER and ESC to return
	to the list.

	When you have finished the selection press F2 to save the video
	modes in the configuration file. At the next run the emulator
	will use one of the modelines that you have chosen.

	If you have problem please read the TROUBLESHOOTING chapter in
	the `install.txt' file.

Options
	-rc RCFILE
		Specify an alternate name of the configuration file.

	-log
		A very detailed log of operations is saved in
		a `.log' file. Very useful for debugging problems.

	-logsync
		Like the `-log' option but the log file is not
		buffered. This option must be used to get a complete
		log file in presence of a machine crash.

	-advmamev, -advmessv, -advpacv, -advmenuv, -vbev, -vgav, -videowv
		Select the mode of operation for the programs `advmame',
		`advmess', `advpac', `advmenu', `vbe', `vga' and `videow'.
		The default is `advmame'.

Video Drivers
	For the DOS version of the programs the following drivers are 
	available:
		svgaline - Generated SVGA modes.
			Video modes obtained tweaking the hardware
			registers of the recognized SVGA boards.

			All clocks, all bit depths are available.
			To use these modes your video board must be supported
			by a `svgaline' driver listed in the `carddos.txt' file.

			This driver is completely independent of the VBE BIOS
			of your board.
		vbeline - Generated VBE (VESA) modes.
			Video modes obtained tweaking the standard VBE BIOS mode
			changing the hardware registers of the SVGA.
			All clocks, all bit depths are available.

			To use these modes your video board must be supported
			by a `vbeline' driver listed in the `carddos.txt' file.

			These drivers work setting a video mode using the
			default VBE2 services and tweak some hardware SVGA
			registers to modify the obtained video mode.
			The driver `vbe3' is an exception. It requires the
			presence of a VBE3 BIOS to change the frequency of the
			desired video mode. Unfortunately the standard
			VBE3 services don't support a resolution size change.

			The resolution is changed modifying only the standard
			VGA registers. This hack may or not may works.
			Also the interlaced modes are only rarely supported
			by the various VBE3 BIOS because they are very rarely
			used in the standard PC monitors.

			If your video board isn't supported by any drivers and
			you don't have a VBE3 BIOS you can try installing a
			software VESA BIOS like the SciTech Display Doctor.
		vgaline - Generated VGA modes.
			Video modes obtained tweaking the hardware
			registers of the standard VGA. Usually these modes
			are called Tweaked, XMode, ZMode.

			Only the standard VGA pixel clocks 6.29, 7.08, 12.59,
			14.16 MHz are available. Only 8 bit color modes. Only
			256 kBytes of video memory.

			This driver supports also text modes with pixel clocks
			12.59, 14.16, 25.17, 28.32 MHz.
			
			This driver is completely independent of the VGA BIOS
			of your board.
		vbe - Available VBE modes.
			This driver is able to use only the video modes
			reported by the VBE BIOS.
			It doesn't require any `device_video_*' options
			because it cannot control how the video modes are
			generated.

	For the Linux version of the programs the following drivers are 
	available:
		svgalib - Generated SVGA modes.
			Video modes obtained tweaking the hardware
			registers of the recognized SVGA boards.

			All clocks, all bit depths are available.
			To use these modes your video board must be supported
			by a `svgalib' driver listed in the `cardlinx.txt' file.
		
			To use this driver you need to install the SVGALIB library 
			version 1.9.x.
		fb - Generated Frame Buffer modes.
			SVGA Graphics modes from the Linux Kernel Console Frame
			Buffer. You must activate the Console Frame Buffer
			support in your kernel.
		sdl - Available SDL modes.
			This driver is able to use only the video modes
			reported by the SDL graphics library.
			It doesn't require any `device_video_*' options
			because it cannot control how the video modes are
			generated.
		slang - sLang text modes.
			Text modes from the sLang library. This driver is only
			able to use the current text mode. It can't really
			change the video mode.

	For the Windows version of the programs the following drivers are 
	available:
		svgawin - Generated SVGA modes. (only for Windows NT/2000/XP)
			Video modes obtained tweaking the hardware
			registers of the recognized SVGA boards.

			All clocks, all bit depths are available.
			To use these modes your video board must be supported
			by a `svgawin' driver listed in the `cardwin.txt' file.

			To use this driver you need to install the included SVGAWIN 
			driver.
		sdl - Available SDL modes.
			This driver is able to use only the video modes
			reported by the SDL graphics library.
			It doesn't require any `device_video_*' options
			because it cannot control how the video modes are
			generated.

	If you have correctly configured the `device_p/h/vclock'
	options in your configuration file, the video modes out of
	the frequency range supported by your monitor are displayed
	in red, and you are prevented to use them.

  Creating Video Modes
	To create a new modeline you should press F5 and you must enter
	the :

	* Vertical clock
	* Horizontal resolution
	* Vertical resolution

	If possible, a video mode compatible with your current monitor
	configuration is created.

	If a the `device_video_format' option is present in your
	configuration file, the video mode is created with this format.
	Otherwise a generic VGA monitor format is used.

  Adjusting Modelines
	You can modify the modelines in the test screen pressing ENTER
	or in the list screen directly.

	When you are in the test screen, `advv' prevent you to set a
	modeline out of the frequency range supported by your monitor.
	In this cases you can hear a grave long sound. Instead when you
	are in the list screen you don't have this limitation.

	If you request a parameter not supported by your hardware
	you hear an acute short sound.

	You can easily change one of the clock values of your modelines
	pressing F8.

	Finally you can rename the modelines pressing TAB.

	Remember to select the modified modelines with SPACE otherwise
	they aren't saved in your configuration file and will be lost.

  Startup Text Mode
	The program at startup tries to set a text mode supported by
	your hardware to show his data.

	If a modelines named `default_text' is present in your
	configuration file the program use it.

	If correct `device_video_p/h/vclock' options are found in your
	configuration file the program try to use a text mode that
	match your clock configuration from a list of predefined
	modes and the modes present in the configuration file.

	If no one of these modes match your configuration the current
	text mode is used.

Configuration
  Video Configuration Options
	The following are the common video configuration options
	available for all the programs.

    device_video
	Selects the video driver to use.

	:device_video auto | (DEVICE[/MODEL])+

	Options:
		auto - Automatic detection of all the available drivers
			(default). The order of detection is for DOS :
			svgaline, vbeline, vgaline, vbe. And for Linux :
			svgalib, fb, sdl, slang. And for Windows :
			svgawin, sdl.

	Options for the Linux version:
		svgalib - SVGA generated graphics modes with the
			SVGALIB 1.9.x library. This driver is not
			available in X (when the environment DISPLAY
			variable is defined).
		fb - SVGA generated graphics modes with the Linux Console
			Frame Buffer. This driver is not
			available in X (when the environment DISPLAY
			variable is defined).
		slang - Text video modes with the sLang library.
		sdl - SDL graphics and fake text modes.

	Options for the DOS version:
		svgaline - SVGA generated graphics modes.
		vbeline - VBE generated graphics modes.
		vgaline - VGA generated text and graphics modes.
		vbe - VBE graphics modes.

	Options for the Windows version:
		svgawin - SVGA generated graphics modes with the
			SVGAWIN included library. To use this driver you
			need to install the `svgawin.sys' driver with the
			`svgawin.exe' command line utility.
		sdl - SDL graphics and fake text modes.

	Please note that to use the utilities `advv' and `advcfg' you
	must at least select a graphics and a text video driver. The
	available text video drivers are `vgaline' for DOS, `slang' and 
	`sdl' for Linux and `sdl' for Windows.

	You can force the detection of a specific model of video board
	adding the name of the model driver after the driver name using
	the `/' separator. For example to force the `vbe3' model
	detection of the `vbeline' driver you must specify
	`vbeline/vbe3'. A complete list of all the available model is in
	the `card*.txt' files. 
	
	Please note that forcing a specific video driver is discouraged.
	Generally you don't need it.

	For a more complete description of the drivers check the
	previous `VIDEO DRIVER' section.

	Example to enable the `vbeline' and the `vgaline' drivers
	with auto-detection for DOS :
		:device_video vbeline vgaline

	Example to force the `vbeline/vbe3' driver and the `vgaline'
	driver for DOS :
		:device_video vbeline/vbe3 vgaline

	Example to enable the `fb' and `slang' driver for Linux :
		:device_video fb slang

  Common Video Configuration Options
	The following are the common video configuration options
	available for all `native' video drivers, i.e. all the
	video drivers with the exception of `sdl' and `vbe'.
	The `sdl' and `vbe' video drivers simply ignore all these
	options.

    device_video_pclock/hclock/vclock
	Specify the monitor frequency range in term of horizontal and
	vertical clocks. This option is MANDATORY.
	Generally these values are specified in the technical page of 
	your monitor manual. 

	:device_video_pclock P_LOW - P_HIGH
	:device_video_hclock H_LOW - H_HIGH [, H_LOW - H_HIGH] [, H_FIXED]
	:device_video_vclock V_LOW - V_HIGH [, V_LOW - V_HIGH] [, V_FIXED]

	If you are using the svgalib driver in Linux you need also to
	adjust the SVGALIB configuration file /etc/vga/libvga.config
	and correctly set the HorizSync and VertRefresh options.
	You must use a range equal or larger ranges than specified with
	the hclock and vclock options.

	Options:
		P_LOW - P_HIGH - Pixel clock range in MHz
			The lower value is the lower clock generable
			by your video board. The higher value is the
			video bandwidth of your monitor. If don't know
			these values you can start with `pclock 5 - 90'
			which essentially enable any video mode.
		H_LOW - H_HIGH - Horizontal clock range in kHz
		H_FIXED - Horizontal fixed clock in kHz
		V_LOW - V_HIGH - Vertical clock range in Hz
		V_FIXED - Vertical fixed clock in Hz

	Example for a Generic PC SVGA multisync monitor :
		:device_video_pclock 10 - 150
		:device_video_hclock 30.5 - 60
		:device_video_vclock 55 - 130

	Example for a Generic PC VGA monitor :
		:device_video_pclock 10 - 50
		:device_video_hclock 31.5
		:device_video_vclock 55 - 130

	Example for a PAL/SECAM TV (European) :
		:device_video_pclock 5 - 50
		:device_video_hclock 15.62
		:device_video_vclock 50

	Example for a PAL/SECAM TV (European) which supports also NTSC
	modes (very common if you use the SCART input) :
		:device_video_pclock 5 - 50
		:device_video_hclock 15.62, 15.73
		:device_video_vclock 50, 60

	Example for a NTSC TV (USA) :
		:device_video_pclock 5 - 50
		:device_video_hclock 15.73
		:device_video_vclock 59.94

	Example for a Generic Arcade Monitor Standard Resolution 15 kHz (CGA) :
		:device_video_pclock 5 - 50
		:device_video_hclock 15.75
		:device_video_vclock 50 - 60

	Example for a Generic Arcade Monitor Medium Resolution 25 kHz (EGA) :
		:device_video_pclock 5 - 50
		:device_video_hclock 25
		:device_video_vclock 50 - 60

	Example for a Generic Atari Monitor Extended Resolution 16 kHz
		:device_video_pclock 5 - 50
		:device_video_hclock 16.5
		:device_video_vclock 53

	Please note that the manuals of some Arcade Monitors incorrectly
	state to support a wide range of horizontal frequency
	like 15 - 31 kHz. Generally these monitors support only the
	three fixed frequency 15.75, 25, 31.1 kHz. For example the
	Wells Gardner D9200.

    device_video_modeline
	Define a video modeline. The modeline format is compatible with
	the format used by the Linux SVGALIB library and by the
	XFree Window system.

	:device_video_modeline Name CLOCK HDE HRS HRE HT VDE VRS VRE VT [-hsync] [-vsync] [+hsync] [+vsync] [doublescan] [interlace] [tvpal] [tvntsc]

	Options:
		Name - Name of the video mode. You can use the quotes
			'"` for the names with spaces.
		CLOCK - Pixel clock in MHz
		HDE HRS HRE HT - Horizontal `Display End',
			`Retrace Start', `Retrace End', `Total'
		VDE VRS VRE VT - Vertical `Display End',
			`Retrace Start', `Retrace End', `Total'
		-hsync -vsync +hsync +vsync - Polarization mode.
		doublescan - Doublescan mode.
		interlace - Interlaced mode.
		tvpal - Modeline converted to PAL format by the internal
			video board scan converter.
		tvntsc - Modeline converted to NTSC format by the internal
			video board scan converter.

	Example:
		:device_video_modeline tweak320x240 12.59 320 336 356 400 240 249 254 262 doublescan -hsync -vsync

	The `tvpal' and `tvntsc' are available only for the nVidia
	svgaline driver.

    device_video_format
	Select the format of the video modes to create.
	You can insert more than one of these option.
	
	:device_video_format HCLOCK HDE HRS HRE HT VDE VRS VRE VT

	Options:
		HCLOCK - Horizontal clock in Hz
		HDE HRS HRE HT VDE VRS VRE VT - Like the modeline option

	When a new modeline is created, AdvanceMAME uses a linear
	interpolation of the two formats with the nearest horizontal
	clock.

	The default value of this option is for an Arcade 15 kHz monitor :

		:15720 0.737 0.075 0.074 0.113 0.916 0.012 0.012 0.060

	and for an Arcade 25 kHz monitor:

		:25000 0.800 0.020 0.100 0.080 0.922 0.006 0.012 0.060

	and for a VGA 31.5 kHz monitor:

		:31500 0.800 0.020 0.120 0.060 0.914 0.019 0.004 0.063

	Which one of these defaults is used depends on the setting of the 
	device_video_hclock option.

    device_video_singlescan/doublescan/interlace
	Limit the use of certain features.

	:device_video_singlescan yes | no
	:device_video_doublescan yes | no
	:device_video_interlace yes | no

	Options:
		yes - Permits the use of the feature if the
			low end driver allow it (default).
		no - Disable completely the feature.

    device_color_palette8/br8/bgr15/bgr16/bgr24/bgr32/yuy2
	Limit the use of some bit depths. If you known that
	the program doesn't work well with a specific bit depth you
	can disable it.

	:device_color_palette8 yes | no
	:device_color_bgr8 yes | no
	:device_color_bgr15 yes | no
	:device_color_bgr16 yes | no
	:device_color_bgr24 yes | no
	:device_color_bgr32 yes | no
	:device_color_yuy2 yes | no

	Modes:
		palette8 - Palettized 8 bits mode.
		bgr8 - RGB 8 bits mode.
		bgr15 - RGB 15 bits mode.
		bgr16 - RGB 16 bits mode.
		bgr24 - RGB 24 bits mode.
		bgr32 - RGB 32 bits mode.
		yuy2 - YUV mode in the YUY2 format.

	Options:
		yes - Permits the use of the bit depth if the
			low end driver allow it (default).
		no - Disable completely the bit depth.

    device_video_output
	Select the output mode.

	:device_video_output auto | window | fullscreen | zoom

	Options:
		auto - Automatically choose (default).
		window - Use a window.
		fullscreen - Use a fullscreen display.
		zoom - Zoom the image to fullscreen using the
			video board hardware. This mode uses a
			YUV overlay available only in some environment,
			like xv in X Window and DirectX in Windows.
			The specific format used is YUY2.

	Please note that with the zoom mode in AdvanceMAME, if the original
	image is in the RGB format instead of a palette format, the program
	need to convert it to the YUY2 format before displaying it.
	It requires some time.

    device_video_cursor
	Select the mouse cursor mode.

	:device_video_cursor auto | off | on

	Options:
		auto - Automatically choose (default). The cursor
			is enabled in window modes, and disabled
			in fullscreen modes.
		off - Always off.
		on - Always on, only if the video mode support it.

    device_video_fastchange
	Enable or disable the fast video mode change. If enabled the
	current video mode is not reset before setting another video
	mode. The reset isn't generally required, but some
	limited DOS video BIOS need it. So, the fast change is disabled 
	for default.

	:device_video_fastchange yes | no

	Options:
		yes - Enable the fast video mode change.
		no - Disable the fast video mode change (default).

  VBELine Configuration Options
	The following are the common video configuration options
	available only for the `vbeline' DOS video driver.

    device_vbeline_driver
	Select the video driver used. The program uses the specified
	driver ONLY if it's correctly detected.
	
	You should use this option only to force the `vbe3' driver.

	:device_video_vbeline_driver none | auto | vbe3 | ...

	Options:
		none - Don't use any `vbeline' driver.
		auto - Auto-detect (default).
		vbe3 - Use the VBE3 BIOS if available, this is the
			last driver detected.
		... - Check the `carddos.txt' file for a complete list of
			the video driver available.
	Example:
		:device_vbeline_driver vbe3

    device_vbeline_mode
	Select which `vbe' mode to use when generating `vbeline' modes.

	:device_vbeline_mode smaller | bigger | ...

	Options:
		smaller - Use the biggest `vbe' mode contained in
			the `vbeline' mode (default).
		bigger - Use the smallest `vbe' mode which contains
			the `vbeline' mode.
		smaller_upto640 - Like `smaller' but not
			bigger than 640x480.
		bigger_upto640 - Like `bigger' but not
			bigger than 640x480.
		320 - Use always the 320x240 mode.
		400 - Use always the 400x300 mode.
		512 - Use always the 512x384 mode.
		640 - Use always the 640x480 mode.
		800 - Use always the 800x600 mode.

  SVGALine Configuration Options
	The following are the common video configuration options
	available only for the `svgaline' DOS video driver.

    device_svgaline_divideclock
	Divide the pixelclock using the VGA sequencer. It should help to support 
	lower pixel clocks on some boards.

	The use of this option is discouraged, it's present only for testing.

	:device_svgaline_divideclock yes | no

	Options:
		yes - Divide the clock by 2.
		no - Don't divide the clock (default).
 
  SVGAWin Configuration Options
	The following are the common video configuration options
	available only for the `svgawin' Windows video driver.

    device_svgawin_stub
	Select how the driver uses the Windows graphics.

	The use of this option is discouraged, it's present only for testing.

	Options:
		none - Don't use the Windows graphics support.
		window - Create a stub window before setting the video mode.
		fullscreen - Create a stub fullscreen window before setting
			the video mode (default).

    device_svgawin_divideclock
	Divide the pixelclock using the VGA sequencer. It should help to support 
	lower pixel clocks on some boards.

	The use of this option is discouraged, it's present only for testing.

	:device_svgawin_divideclock yes | no

	Options:
		yes - Divide the clock by 2.
		no - Don't divide the clock (default).

Copyright
	This file is Copyright (C) 2002 Andrea Mazzoleni.

