FIRST TIME CONFIGURATION
	AdvanceMAME has the ability to directly control your video
	board to get the best possibile video modes with always the
	correct size and aspect ratio.

	To made it possible it needs some information on your monitor
	capability in the form of the supported pixel, horizontal and
	vertical clocks.

	With these info AdvanceMAME is able to always generate a
	`perfect' video mode for the emulated game.

	AdvanceMAME supports two basic way to generated video modes:
	the automatic and the manual operation mode.

	In the automatic mode AdvanceMAME always automatically generate
	a video mode from scratch. It's the simplest mode of operation.

	In the manual mode AdvanceMAME picks the video mode from a
	predefined list of modelines, eventually adjsting them to match
	the game clock or size requirements.
	This mode of operation should be used only if the automatic mode
	doesn't work.

	If you are using the special `sdl' or `vbe' video drivers,
	AdvanceMAME isn't able to create or adjust video modes. In this
	case you don't need to configure anything because AdvanceMAME
	can use only the video modes which your system reports as available.
	These drivers are only indicated to use AdvanceMAME or AdvanceMENU
	in a Window Manager system.

AUTOMATIC OPERATION MODE
	In the automatic operation mode AdvanceMAME automatically
	creates a `perfect' video mode for the game to be emulated
	that fit exactly the whole screen with the correct aspect
	and frame rate.

	To configure and activate this mode you need to run the
	`advcfg' utility and to answer at the various questions.
	You don't need to create a list of video modes, any needed
	video mode is created at runtime by AdvanceMAME.

	Before running the `advcfg' utility you should check your
	monitor manual for the vertical and horizontal clocks
	supported by your monitor.

	The `advcfg' utility add these options in your `advmame.rc'.

		display_mode auto
		display_adjust generate
		device_video_pclock ?
		device_video_hclock ?
		device_video_vclock ?
		device_video_format ?

MANUAL OPERATION MODE
	In the manual operation mode AdvanceMAME scan a list of `good'
	video modelines and chose the best available. You must prevently
	create this list of video modelines with the `advv' utility.

	This is the description of the few basic steps required to run
	AdvanceMAME in the manual operation mode.

	) Add in the in the file `advmame.rc' these options.

		diplay_mode auto
		display_adjust x

	) Add in the the file `advmame.rc' the `p/h/vclock' options
	that specify which horizontal and vertical clocks are
	supported by your monitor.
	Generally these values are specified in the technical page of 
	your monitor manual. These are some example :

		Generic PC SVGA multisync monitor :
			device_video_pclock 10 - 80
			device_video_hclock 31 - 60
			device_video_vclock 55 - 90

		Generic PC VGA monitor :
			device_video_pclock 10 - 50
			device_video_hclock 31.5
			device_video_vclock 55 - 110

		Generic LCD screen :
			device_video_pclock 0 - 0
			device_video_hclock 0
			device_video_vclock 0

		PAL/SECAM TV (European) :
			device_video_pclock 5 - 50
			device_video_hclock 15.62
			device_video_vclock 50

		PAL/SECAM TV (European) which supports also NTSC 
		modes (very common if you use the SCART input) :
			device_video_pclock 5 - 50
			device_video_hclock 15.62, 15.73
			device_video_vclock 49 - 61

		NTSC TV (USA) :
			device_video_pclock 5 - 50
			device_video_hclock 15.73
			device_video_vclock 60

		Generic Arcade Monitor Standard Resolution 15 kHz (CGA) :
			device_video_pclock 5 - 50
			device_video_hclock 15.75
			device_video_vclock 49 - 61

		Generic Arcade Monitor Medium Resolution 25 kHz (EGA) :
			device_video_pclock 5 - 50
			device_video_hclock 24.84
			device_video_vclock 49 - 61

		Generic Atari Monitor Extended Resolution 16 kHz
			device_video_pclock 5 - 50
			device_video_hclock 16.5
			device_video_vclock 53

	) Run the `advv' program.

	) Test the video modelines of your interest pressing ENTER on them.
	If the mode isn't centered try centering it with the ARROW keys.
	When you have finished press ENTER to save your modifications or ESC 
	to restore the previous setting.
	Returned in the video mode list, if the mode is displayed correctly,
	you can select it to be used by AdvanceMAME pressing SPACE.
	It's very important that in all the selected modes the screen area
	is completely visible. Otherwise, when playing, part of the game 
	may be out of screen.

	Video modes displayed in red aren't supported by your video 
	hardware.

	) When you have selected the list of `good' video modes press
	F2 to save them in your `advmame.rc'

	) Press ESC to exit from `advv'

	) Run `advmame' or the front-end `advmenu'

TROUBLESHOTTING
	) Delete any old configuration files and restart from scratch.

	) If you are using a PC Multisync monitor and the image is
	instable or the monitor automatically switch off, you have
	probably entered wrong clock values.
	Try with a shorter horizontal clock range and check your
	monitor manual.
	Try for example with:

		device_video_hclock 31 - 50

	) If you are using an Arcade Monitor/TV and the image is
	instable try increasing the lower pclock limit.
	Try for example with:

		device_video_pclock 12 - 50

	) In DOS try forcing the use of the `vbeline' driver instead of the
	`svgaline' driver with the option:

		device_video vbeline vgaline

	) In DOS, if your board has a VESA VBE 3.0 BIOS, try forcing the
	`vbeline/vbe3' driver with the options:

		device_video vbeline/vbe3 vgaline

	) In DOS try changing the `vbeline_mode' option:

		device_video vbeline vgaline
		device_vbeline_mode smaller

	) In Linux try forcing the use of the `fb' driver instead of the
	`svgalib' driver with the option:

		device_video fb slang

COPYRIGHT
	This file is Copyright (C) 2002 Andrea Mazzoleni.

