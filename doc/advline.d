Name{number}
	advline - AdvanceMAME Modeline Generator

Synopsis
	:advline [/fh A,F,S,B] [/fv A,F,S,B]
	:	[/p CLOCK] [/h CLOCK] [/v CLOCK] [WIDTHxHEIGHT]

	:advline [/atari_standard] [/atari_extended] [/atari_medium]
	:	[/atari_vga] [/pal] [/ntsc] [/hp_vga] [/vga] [/svga60]
	:	[/svga57]

Description
	This utility is a video modeline generator. 
	The output of the execution is a valid list of video modes that 
	you can use in your `cfg' files. 
	The `.cfg' examples are generated with the predefinite options of 
	this program.
	
	The modelines are generated using an user specified reference
	length of the Active Display, Front Porch, Sync Pulse and Back 
	Porch.

	You can also set any type of clock constrains like Horizontal Clock,
	Vertical Clock and Pixel Clock.
	If you select more than one exact clock the resulting video modes 
	will have something fixed.
	For example the PAL modes have a fixed Vertical and Horizontal
	clocks. This result in a fixed number of rows.

	The minimum and maximum clock limitations are considered as
	suggestions. The video modes are adjusted trying to match these
	limits. The available changes are enabling the doublescan, enabling
	the interlace and doubling the horizontal size.

Options
	/fh A,F,S,B
		Select the horizontal format. You need specify the length
		of the Active Display, Front Porch, Sync Pulse and
		Back Porch.
		You can use any measure unit because these values are
		normalized.

	/fv A,F,S,B
		Select the vertical format.

	/sync_vga
		Generate the sync polarization for old VGA monitors.
		Otherwise the sync polarization is always negative.

	/p CLOCK
		Select the pixel clock in MHz.

	/h CLOCK
		Select the horizontal clock in kHz.

	/v CLOCK
		Select the vertical clock in Hz.

	/pmin CLOCK
		Suggest the minimum pixel clock in MHz.
		You can use this option if your video board has problems
		generating too low pixel clock.

	/hmin CLOCK
		Suggest the minimum horizontal clock in kHz.
		You can use this option to match the capabilities
		of your monitor.

	/vmin CLOCK
		Suggest the minimum vertical clock in Hz.
		You can use this option to match the capabilities
		of your monitor.

	/vmax CLOCK
		Suggests the maximum vertical clock in Hz.
		You can use this option to match the capabilities
		of your monitor.

	=WIDTHxHEIGHT
		Set the favorite video mode. If none is specified a
		default set is generated.

Predefinite Options
	These options are a predefinite set of configurations:

	/atari_standard
		Atari standard resolution monitor
		HClock 15.7 kHz

	/atari_extended
		Atari extended resolution monitor
		HClock 16.5 kHz

	/atari_medium
		Atari medium resolution monitor
		HClock 25 kHz

	/atari_vga
		Atari VGA monitor
		HClock 31.5 kHz

	/pal
		PAL TV
		HClock 15.63 kHz, VClock 50 Hz

	/ntsc
		NTSC TV
		HClock 15.75 kHz, VClock 60 Hz

	/hp_vga
		HP VGA monitor
		HClock 31.5 kHz

	/vga
		Generic VGA monitor
		HClock 31.5 kHz

	/svga60
		Generic SVGA multisync 60 Hz monitor
		VClock 60 Hz

	/svga57
		Generic SVGA multisync 57 Hz monitor
		VClock 57 Hz

	You can overrides some parameters using the normal options.

	Examples for SVGA default modes at 75 Hz:
		:modeline /svga60 /v 75

	Examples for SVGA mode 384x224 at 72 Hz:
		:modeline /svga60 /v 72 384x224

Timings
	This is the collections of video timings used by the
	predefinite options.

	The nomenclature used is :

		A = Active Display
		F = Front Porch
		S = Sync Pulse
		B = Back Porch
		S+B = Video Delay
		F+S+B = Blank
		A+F+S+B = Scan Period

	|                       A              F   S   B
	|----|  |----XXXXXXXXXXXXXXXXXXXXXXXXX----|  |----XXXXXXXX...
	|    |--|                                 |--|

    PAL TVs

	:               Horizontal              Vertical
	:A              52.00 us                18.468 ms
	:F              1.65 us +- 0.1          0.192 ms (? to check)
	:S              4.70 us +- 0.1          0.192 ms (? to check)
	:B              5.65 us                 1.152 ms (? to check)
	:F+S+B          12.00 us +- 0.25
        :A+F+S+B        64.00 us                20.000 ms
	:Clock          15.625 kHz              50.00 Hz
	:Visible Pixel                          288.5
	:Total Pixel                            312.5

    NTSC TVs

	:               Horizontal              Vertical
	:A              52.60 us                15.39875 ms
	:F              1.50 us                 0.190.50 ms
	:S              4.70 us                 0.190.50 ms
	:B              4.70 us                 0.889.00 ms
	:F+S+B          10.90 us
        :A+F+S+B        63.50 us                16.66875 ms
	:Clock          15.72 kHz               59.94 Hz
	:Visible Pixel                          242.5
	:Total Pixel                            262.5

    Atari Standard Resolution Monitors

	:               Horizontal              Vertical
	:A              46.90 us                15.30 ms
	:S              4.70 us                 0.20 ms
	:S+B            11.90 us                1.20 ms
	:A+F+S+B        63.60 us                16.70 ms
	:Clock          15.72 kHz               60.00 Hz
	:Visible Pixel  336                     240
	:Total Pixel    456                     262

        :Pixel Clock 7.16 MHz

    Atari Extended Resolution Monitors

	:               Horizontal              Vertical
	:A              48.00 us                17.40 ms
	:S              3.90 us                 0.20 ms
	:S+B            11.90 us                1.20 ms
	:A+F+S+B        60.60 us                18.90 ms
	:Clock          16.50 kHz               53.00 Hz
	:Visible Pixel  512                     288
	:Total Pixel    646                     312

	:Pixel Clock 10.67 MHz

    Atari Medium Resolution Monitors

	:               Horizontal              Vertical
	:A              32.00 us                15.40 ms
	:S              4.00 us                 0.20 ms
	:S+B            7.20 us                 1.20 ms
	:A+F+S+B        40.00 us                16.70 ms
	:Clock          25.00 kHz               60.00 Hz
	:Visible Pixel  512                     384
	:Total Pixel    640                     416

	:Pixel Clock 16.00 MHz

    Atari VGA20 Resolution Monitors

	:               Horizontal              Vertical
	:A              25.60 us                12.20 ms
	:S              4.00 us                 0.20 ms
	:S+B            5.70 us                 1.10 ms
        :A+F+S+B        31.70 us                14.30 ms
	:Clock          31.55 kHz               70.00 Hz
	:Visible Pixel  512                     384
	:Total Pixel    634                     450

	:Pixel Clock 20.00 MHz

Copyright
	This file is Copyright (C) 2003 Andrea Mazzoleni.


