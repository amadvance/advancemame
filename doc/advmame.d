Name{number}
	advmame, advmess - AdvanceMAME/MESS Emulator

Synopsis
	:advmame GAME [-default] [-remove] [-cfg FILE]
	:	[-log] [-listxml] [-record FILE] [-playback FILE]
	:	[-version] [-help]

	:advmess MACHINE [images...] [-default] [-remove] [-cfg FILE]
	:	[-log] [-listxml] [-record FILE] [-playback FILE]
	:	[-version] [-help]

Description
	AdvanceMAME is an unofficial MAME version for GNU/Linux, Mac OS
	X, DOS and Windows with an advanced video support for helping the
	use with TVs, Arcade Monitors, Fixed Frequencies Monitors and
	also with normal PC Monitors.

	The major features are:

	* Automatic creation of `perfect' video modes with the correct
		size and clock.
	* A lot of video boards supported for direct hardware registers
		programming. (see the card*.txt files)
	* Support for 8, 15, 16 and 32 bits video modes.
	* Real hardware scanlines.
	* Software video image stretching by fractional factors, for
		example to play vertical games like "Pac-Man" with
		horizontal Arcade Monitors or TVs.
	* Special `scalex', `scalek', `lq', `hq', and `xbr' effects to improve the aspect
		with modern PC Monitors.
	* Special `blit' effects to improve the image quality in
		stretching.
	* Special `rgb' effects to simulate the aspect of a real Arcade
		Monitor.
	* Change of the video mode and other video options at runtime.
	* Support of Symmetric Multi-Processing (SMP) with a multiple
		thread architecture (only for Linux).
	* Sound and video recording in WAV, PNG and MNG files.
	* Multiple mice support in Linux, DOS, Windows 2000 and Windows XP.
	* Automatic exit after some time of inactivity.
	* Scripts capabilities to drive external hardware devices
		like LCDs and lights.
	* Textual configuration files.
	* Help screen describing the user input keys.

SubSubIndex

Keys
	In the game play you can use the following keys:

		ESC - Exit.
		F1 - Help.
		TAB - Main Menu.
		F2 - Test/Service Switch.
		F3 - Reset the game.
		F7 - Load a game state.
		SHIFT + F7 - Save a gam state.
		F8 - Decrease the frame skip value.
		F9 - Increase the frame skip value.
		F10 - Speed throttle.
		F11 - Display the frame per second.
		F12 - Save a snapshot.
		P - Pause.
		PAD * - Turbo mode until pressed.
		PAD / - Cocktail mode (flip the screen vertically).
		PAD - - Mark the current time as the startup time of the game.
		CTRL + ENTER - Start the sound and video recording.
		ENTER - Stop the sound and video recording.
		, - Previous video mode.
		. - Next video mode.
		TILDE - Volume Menu.

	for player 1 you can use the keys:

		1 - Play.
		5 - Insert coin.
		ARROW - Move.
		CTRL - First button.
		ALT - Second button.
		SPACE - Third button.

	for player 2 you can use the keys:

		2 - Play.
		6 - Insert coin.
		R, F, D, G - Move.
		A - First button.
		S - Second button.
		Q - Third button.

	for AdvanceMESS are available also the following keys:

		ScrollLock - Switch to partial keyboard emulation
			which lets you use keys like TAB, ALT and
			CTRL.

Options
	This is the list of the available command line options:

	=GAME/MACHINE
		The game or machine to emulate. If the specified
		GAME/MACHINE is unknown, a list of possible `guesses'
		is printed.

	-default
		Add to the configuration file all the missing options
		with default values.

	-remove
		Remove from the configuration file all the options
		with default values.

	-cfg FILE
		Select an alternate configuration file. In Linux and Mac
		OS X the you can prefix the file name with "./" to
		load it from the current directory.

	-log
		A very detailed log of operations is saved in
		a `.log' file. Very useful for debugging problems.

	-listxml
		Outputs the internal MAME database in XML format.

	-record FILE
		Record all the game inputs in the specified file.
		The file is saved in the directory specified by the
		`dir_inp' configuration option.

	-playback FILE
		Play back the previously recorded game inputs in the
		specified file.

	-version
		Print the version number, the low-level device drivers
		supported and the configuration directories.

	-help
		Print a short command line help.

	On the command line you can also specify all configuration
	options with the format -OPTION ARGUMENT. For boolean options
	you don't need to specify the argument but you must use the
	-OPTION or -noOPTION format. For example:

		:advmame pacman -device_sound sb -nodisplay_scanlines

	You can use short options if they are unambiguous. You can remove
	any prefix tag separated with `_' or truncate it.
	For example `-dev_cartdrige' can be written as `-dev_cart',
	`-cartdrige', `-cart', ...

	In Linux and Mac OS X you can also use `--' before options instead of `-'.
	In DOS and Windows you can also use `/'.

Features
	This section contains a brief description of all the features
	of AdvanceMAME.

  Automatic Video Mode Generation
	AdvanceMAME has the ability to directly control your video
	board to get the best possible video modes with always the
	correct size and aspect ratio.

	You can control how the video modes are generated with the 
	`display_adjust' option. More details are in the `install.txt'
	file.

  Video Menu
	AdvanceMAME adds a new `Video' menu in MAME to change the video
	options.

	You can select the desired video mode, the resize type and the
	video effects.

	The selected option is displayed at the right side of the menu,
	the effective value used for the option is displayed in `[]'.

  Resize
	AdvanceMAME supports many software stretching types of the
	game image. Generally they are not used, because a video
	mode of the correct size is automatically generated.
	But in some conditions it isn't possible, in this case the
	image is stretched.

	There are four stretch types: `none', `integer', `mixed', `fractional'.
	You can control the type of stretching with the `display_resize' option.

	The `none' option simply disables any type of stretching.
	The `integer' option allows only integer stretching in the
	horizontal and vertical directions. For example 1x2, 2x1, 2x2.
	The `mixed' option allows integer stretching in the horizontal
	direction and fractional stretching in the vertical direction.
	For example 1x1.23, 2x1.18.
	The `fractional' option allows fractional stretching in any
	directions. For example 1.08x1.08, 1.34x1.78.

	Usually the best choice is the `mixed' option. It's very fast
	and the image quality doesn't suffer too much.

  Blit Effects
	AdvanceMAME supports many special video effects to improve
	the image quality when it's stretched.

	There are a lot of video effects: `none', `max', `mean',
	`filter', `scalex', `scalek', `lq', `hq' and `xbr'.
	You can select the favorite effect with the `display_resizeeffect'
	option, or from the runtime menu.

	The `none' effect simply duplicates and removes rows and lines
	when the image is stretched.
	The `max' effect tries to save the image details checking the luminosity
	of the pixels in stretching. It ensures to have vertical and horizontal lines
	always of the same thickness.
	The `mean' effect tries to save the image details displaying the
	mean color of the pixels in stretching.
	The `filter' effect applies a generic blur filter computing the
	mean color in the horizontal and vertical directions. The best
	results are when the image is stretched almost by a double
	factor. When the image is enlarged the filter is applied after
	stretching; when reduced, it's applied before.
	The `scalex', `scalek', `lq', `hq' and `xbr' effects add missing pixels
	trying to match the image patterns.

	The `scalex', `scalek', `lq', `hq' and `xbr' effects work only if the image is
	magnified. To enable it you should also use the `magnify' option.

  RGB Effects
	AdvanceMAME supports also some special video effects to simulate
	the aspect of the game as displayed in an old fashion Arcade
	Monitor.

	You can simulate the RGB triads of the screen or the vertical and
	horizontal scanlines.

  Mode Selection
	In the `Video Mode' submenu you can select the favorite video
	mode.

	If you choose `auto', the best video mode is chosen
	automatically.

	You can change the active video mode pressing `,' and `.' when
	in game play.

	You can force a specific video mode with the option
	`display_mode'.

  Per game/resolution/frequency/orientation options
	All the options are customizable in the configuration file for
	the single game or for a subset of games defined by the game
	resolution, frequency and orientation.

  Scripts
	AdvanceMAME supports a basic script language capable to
	control an external hardware through the parallel port
	or keyboard led signals.

	More details are in the `script.txt' file.

  Aspect Ratio Control
	AdvanceMAME tries always to display the game with the
	correct aspect ratio.

	But if you want, you can permit a small aspect error
	to enlarge the effective game image on the screen.
	It's very useful to display vertical games on 
	horizontal monitors and vice versa.

	More details are in the description of the `display_expand'
	option.

  Speed Control
	AdvanceMAME permits a special speed control of the game
	play.

	You can play the game in a faster way, change arbitrarily the
	frame rate, skip the game startup process at the maximum speed,
	or skip the game animations pressing a key.

	Press `asterisk_pad' to enable the `turbo' mode. Press `minus_pad'
	to mark the time of the real game start. The next time the game is
	started, it will execute very fast until this time.

	More details are in the description of the `sync_fps', `sync_speed',
	`sync_turbospeed' and `sync_startuptime' options.

	The video and audio synchronization uses an advanced algorithm,
	which ensure always the best performance.

	The program continuously measures the time required to compute a
	frame and continuously adapt the number of frame to skip and
	to draw.

	If it detects that you have system too slow to play the game
	at full speed, it automatically disables any frame skipping.

	If the underline Operand System allows that, AdvanceMAME release
	the CPU when it isn't used after computing each frame reducing the
	CPU occupation.

  Audio Control
	The audio volume is automatically adjusted to ensure
	that all the emulated games have the same volume power.

	An equalizer and a spectrum analyzer are also available.

	More details are in the description of the `sound_normalize'
	and `sound_equalizer' options.

  Exit Control
	If you have a real Arcade Cabinet you can configure AdvanceMAME
	to automatically exit after some time of inactivity to save
	your monitor screen.

	For some supported games you can force the display of an
	exit menu during the game play to prevent unwanted exit.

	More details are in the description of the `misc_safequit',
	and `input_idleexit' options.

  Input Control
	AdvanceMAME supports a very fine control of the mapping
	of the analog and digital inputs. You can remap any input
	event using easy to maintain text configuration files.

	More details are in the description of the `input_map'
	option.

  Audio and Video Recording
	AdvanceMAME can saves the game play in .WAV audio files and
	.MNG video files.

	More details are in the description of the `record_*'
	options.

  User Interface
	AdvanceMAME displays the user interface directly on the screen
	instead on the game image.

	This means that the interface doesn't have applied the same video
	effects of the game, it isn't limited on the game area, it
	isn't recorded on video clips and you can customize the font.

	Also, True Type fonts with alpha blending are supported.

	More details are in the description of the `ui_*' options.

  Input Help
	AdvanceMAME is able to display a help image containing the
	input mapping of the emulated game. Any input element has a
	different depending on the assigned player and if it's pressed
	or not.

	The default image is a standard keyboard, with the used keys
	highlighted. If you have an Arcade cabinet you can create your
	personalized control image and select each region to highlight.

	Press `f1' on the game play to display it.

	More details are in the description of the `ui_help*' options.

  Input Text Configuration File
	All the user customizations are stored in a single textual
	configuration file and not in a lot of .cfg file like other MAME ports.

	This allows to view, edit and copy any customization manually.
	They are also more compact because only the difference from the
	default is saved.

	They are independent of the internal MAME structure, so, when it
	changes, you don't lose the customizations.

	More details are in the description of the `input_setting',
	`input_dipswitch', `input_configswitch' options.

  Cocktail
	For cocktail arcade cabinets you can manually flip vertically the
	screen for games without cocktail support.

	Press `slash_pad' to flip the screen.

  Dipswitches Control
	You can customize with specialized options the game difficulty and
	the game freeplay. These options are smart enough to solve
	common ambiguities and errors in the game dipswitches definitions.

	More details are in the description of the `misc_diffucilty' and
	`misc_freeplay' options.

  LCD Panel
	AdvanceMAME is able to talk to an `lcdproc' server located anywhere
	in internet to display arbitrary information on a real or simulated
	LCD panel.

Use Cases
	This section describes some useful cases for AdvanceMAME
	on different video hardware.

  With a LCD Monitor
	On a LCD Monitor, AdvanceMAME is able to use the hardware
	acceleration of your video board to stretch the game image
	to fit the exact resolution of your LCD monitor.
	With an HighDefinition monitor (1920x1080/1200), this stretching
	is not noticeable and you get an image with quality comparable
	with a CRT Multi-Sync monitor.
	You can also use a bunch of video effects to remove the
	annoying pixelation or to emulate old stylish CRT aspect.

  With a CRT Multi-Sync Monitor
	On a PC Multi-Sync monitor you can get any resolution at any
	Vertical Frequency. In this case AdvanceMAME always generates
	a `perfect' video mode with the correct size and clock. It
	doesn't require any type of stretching.
	For example for the game "Bomb Jack" a video mode of 400x256
	at 60 Hz (perfect size and perfect frequency) is used.

  With a CRT VGA Monitor/Fixed Frequency Monitor/Arcade Monitor
	On Fixed Frequency monitors you are physically limited on
	the choice of Horizontal Frequency in the video mode. In this
	case AdvanceMAME takes care of your monitor's limitations
	and in the most cases it's able to use a video mode with the
	correct size but not with the correct frequency due to the
	monitor's limitations.
	For example for the game "Pac-Man" and a VGA monitor (31.5 kHz)
	a video mode of 400x288 at 100 Hz (perfect size) is used.

  With a NTSC or PAL TV
	On a TV you are physically limited to use both fixed Horizontal
	and Vertical Frequencies. This results on a prefixed number of
	rows for the video mode.
	For example for a NTSC TV you can get 240 rows (480 if
	interlaced) and for a PAL TV 288 rows (576 if interlaced).
	In this case AdvanceMAME uses a video mode with the prefixed
	number of rows but with the correct number of columns. So,
	ONLY a vertical image stretching is required.
	For example for the game "Pac-Man" on a NTSC TV a video mode
	of 400x240 (perfect horizontal size) is used.
	For stretching some special algorithms are used to minimize
	the lose of details.

  With a Multi-format NTSC and PAL TV
	If your TV supports both formats, AdvanceMAME automatically
	chooses the format that better fits the game requirements.
	For example for the game "Mr. Do!" a video mode of 336x240
	NTSC (perfect size) is used. For the game "Pac-Man" a video mode
	of 400x288 PAL (perfect size) is used.

Other Ports
	This section compares the AdvanceMAME video support with the
	other MAME ports.

  Windows MAME
	The official Windows MAME is forced by Windows drivers to select
	a video mode from a prefixed list of mode sizes and clocks.
	If the emulated game requires a not standard mode size the
	emulator must stretch the game image to fit the screen (losing in
	quality). If the emulated games requires a not standard clock the
	emulator must play the game without synchronizing with the video
	vertical retrace (generating the tearing disturb on scrolling game)
	or display frames for different time (generating a not constant
	scrolling).

	Depending on the type of your video drivers you can sometimes
	edit the prefixed list of video modes.

	The TV support depends on the video drivers of your board and
	it's generally limited at the interlaced mode 640x480.
	Arcade monitors are used as NTSC TVs.

	Generally this port is limited by Windows to get the best from
	your monitor.

  SDL MAME
	The official SDL MAME, is similar at AdvanceMAME when using the
	sdl video driver, which is the default when run in a modern
	Desktop environment.

	AdvanceMAME has the benefit to allow to use more video effects,
	able to improve the image quality.

	This also helps when using Arcade Monitors with specific modelines,
	because AdvanceMAME is able to streatch the image using special effects
	like 'max' that avoids to lose pixels.

  DOS MAME
	The official DOS MAME is limited to use only the standard
	VESA resolutions. Generally they are only 320x200,
	320x240, 400x300, 512x384, 640x480, ...

	The Arcade/TV support is limited at the mode 640x480 for the
	ATI boards.

Configuration
	In DOS and Windows the configuration options are read from the
	files `advmame.rc' and `advmess.rc' in the current
	directory.

	In Linux and Mac OS X the configuration options are read from the
	files `advmame.rc' and `advmess.rc' in the $host, $data and
	the $home directory.
	The $host directory is `$SYSCONFDIR', where $SYSCONFDIR is the
	`sysconfdir' directory configured with the `configure' script.
	The default is `/usr/local/etc'.
	The $data directory is `$DATADIR/advance', where $DATADIR is the
	`datadir' directory configured with the `configure' script.
	The default is `/usr/local/share'.
	The $home directory is `$ADVANCE', where $ADVANCE is the value of the
	ADVANCE environment variable when the program is run.
	If the ADVANCE environment variable is missing the $home directory
	is `$HOME/.advance' where $HOME is the value of the HOME environment
	variable.
	If both the ADVANCE and HOME environment variables are missing the
	$data directory became also the $home directory.

	The priority of the options is in the order: $host, $home and $data.

	The $home directory is also used to write all the information
	by the program. The files in the $host and $data directory are only read.

	You can include an additional configuration files with the `include'
	option. In DOS and Windows the files are searched in the current directory.
	In Linux and Mac OS X the files are searched in the $home directory if
	they are expressed as a relative path. You can force the search in the
	current directory prefixing the file with `./'.
	To include more than one file you must divide the names with `;' in
	DOS and Windows, and with `:' in Linux and Mac OS X.

	You can force the creation of a default configuration file with the
	command line option `-default'.

	In the configuration file the options are specified in this format:

		:[SECTION/]OPTION VALUE

	If the SECTION is omitted the `' (empty) section is assumed.

	You can split long options in a multi-line format ending the line
	with the char `\':

		:[SECTION/]OPTION FIRST_LINE \
		:	SECOND_LINE \
		:	... \
		:	LAST_LINE

	When you run a game every option is read in different
	sections in the following order:

		`SYSTEM[SOFTWARE]' - The short system name and the loaded
			software on the command line, like `ti99_4a[ti-inva]',
			`a7800[digdug]' and `nes[commando]'.
		`GAME' - The short game (or system) name, like `pacman',
			`ti99_4a' and `nes'.
		`PARENT' - If present, the parent name of the game,
			like `puckman'.
		`BIOS' - If present, the bios name of the game,
			like `neogeo'.
		`RESOLUTIONxCLOCK' - The resolution and the clock of the
			game, like `244x288x60' for raster games or
			`vector' for vector games. If the vertical clock
			is a real value, it's rounded downwards to the
			nearest integer.
		`RESOLUTION' - The resolution of the game, like
			`244x288' for raster games or `vector' for
			vector games.
		`ORIENTATION' - The game orientation. One of `vertical',
			`horizontal'.
		`CONTROLLER' - The game input device. One of `joy4way',
			`joy8way', `doublejoy4way', `doublejoy8way',
			`paddle', `dial', `trackball', `stick', `lightgun',
			`mouse'.
		`Nplayer' - Number of players in the game. One of `1player',
			`2player', `3player', ...
		`' - The default empty section.

	For example for the game `pacman' the following sections are
	read: `pacman', `puckman', `224x288x60', `224x288', `vertical',
	`joy4way', `2player' and `'.

	You can override any global options inserting new options in
	any of the sections of the game.

	For example:
		:display_scanlines no
		:pacman/display_scanlines yes
		:244x288x60/display_scanlines yes
		:vertical/display_ror yes
		:horizontal/display_ror no

  Software Configuration Options
	This section describes the options used to customize the
	software loaded by system emulated.

    dev_*
	Loads a specific device for the AdvanceMESS emulator. These
	options are mainly used on the command line to specify the
	machine software to load.
	The file specified is searched in the directory list specified
	in the `dir_image' option.

	:dev_COMMAND FILE

	Commands:
		cartridge - Load a cartridge.
		floppydisk - Load a floppydisk.
		harddisk - Load an harddisk.
		cylinder - Load a cylinder.
		cassette - Load a cassette.
		punchcard - Load a punchcard.
		punchtape - Load a punchtape.
		printer - Load a printer.
		serial - Load a serial.
		parallel - Load a parallel.
		snapshot - Load a snapshot.
		quickload - Load a quickload.

	Examples:
		:advmess ti99_4a -dev_cartridge attackg.bin
		:advmess ti99_4a -cart alpinerc.bin -cart alpinerg.bin

  Directory Configuration Options
	This section describes the options used to customize the
	directories used by the program.

    dir_*
	Specify all the support directories. In DOS and Windows use the `;'
	char as directory separator. In Linux and Mac OS X use the `:' char.

	:dir_* DIR[;DIR]... (DOS, Windows)
	:dir_* DIR[:DIR]... (Linux, Mac OS X)

	Options:
		dir_rom - Multi directory specification for the
			AdvanceMAME `rom' files and AdvanceMESS `bios'
			files.
		dir_image - Multi directory specification for the
			chd/disk/cartdrige/... image files.
		dir_diff - Multi directory specification for the
			disk image differential files.
		dir_sample - Multi directory specification for the
			zipped `sample' files. Only the zipped format
			is supported.
		dir_artwork - Multi directory specification for the
			zipped `artwork' files. Only the zipped format
			is supported.
		dir_nvram - Single directory for `nvram' files.
		dir_memcard - Single directory for `memcard' files.
		dir_hi - Single directory for `hi' files.
		dir_inp - Single directory for `inp' files.
		dir_sta - Single directory for `sta' files.
		dir_snap - Single directory for the `snapshot'
			files.
		dir_crc - Single directory for the `crc' files.

	Defaults for DOS and Windows:
		dir_rom - rom
		dir_image - image
		dir_diff - diff
		dir_sample - sample
		dir_artwork - artwork
		dir_nvram - nvram
		dir_memcard - memcard
		dir_hi - hi
		dir_inp - inp
		dir_sta - sta
		dir_snap - snap
		dir_crc - crc

	Defaults for Linux and Mac OS X:
		dir_rom - $home/rom:$data/rom
		dir_image - $home/image:$data/image
		dir_diff - $home/image:$data/diff
		dir_sample - $home/sample:$data/sample
		dir_artwork - $home/artwork:$data/artwork
		dir_nvram - $home/nvram
		dir_memcard - $home/memcard
		dir_hi - $home/hi
		dir_inp - $home/inp
		dir_sta - $home/sta
		dir_snap - $home/snap
		dir_crc - $home/crc

	If a not absolute dir is specified, in Linux and Mac OS X
	it's expanded as "$home/DIR:$data/DIR". In DOS and Windows
	it's maintained relative.

	For the `dir_rom' and `dir_image' the following file
	combinations are tried:

	* DIR/GAME/ROM.EXT
	* DIR/GAME.zip/ROM.EXT
	* DIR/GAME/ROM.zip/ROM

	Where DIR is substituted with the directories specified, GAME
	is the name of the game or machine emulated, ROM is the rom
	name and EXT is the rom extension.

	For the files searched in the `dir_image' option you can also
	specify a different zip name prefixing the rom name
	with the zip name without extension and the `=' char.
	For example to run the `ti99_4a' emulator and load the
	cartdriges `alpinerc.bin' and `alpinerg.bin' both contained in
	the zip file `alpiner.zip' you can use this syntax:

		:advmess ti99_4a -cart alpiner=alpinerc.bin -cart alpiner=alpinerg.bin

	This feature is used automatically by AdvanceMENU to correctly
	run AdvanceMESS software in zip files.

  Display Configuration Options
	This section describes the options used to customize the
	display.
  
    device_video_*
	These options are used to customize the video drivers.

	All the `device_video_*' options described in the `advdev.txt' file 
	can be used.

	If you use a `System' video driver, you don't need to set these
	options. They are mostly ignored.

	With a `Generate' video drivers these options are used to select
	and create the correct video mode. They are mandatory. You can use
	the `advcfg' utility to set them interactively.

    display_mode
	Selects a specific modeline by its name.

	:display_mode auto | MODELINE_NAME

	Options:
		MODELINE_NAME - Specific modeline, as named with
			the `advv' utility.
		auto - Automatically chooses the best modeline
			available (default).

    display_adjust
	Controls how are generate the video modes. Correct use of this 
	option removes the need of any software stretching improving a 
	lot the game image.

	For an introduction on how the program operates on video mode,
	you can see the `install.txt' file.

	:display_adjust none | x | clock | xclock | generate_exact
	:	| generate_y | generate_clock | generate_clocky
	:	| generate_yclock

	Options:
		none - No automatic video mode creation. Use only the
			available modelines (default).
		x - Adjusts the available modeline horizontal resolution
			to match the game image's size. The stretched modeline 
			keeps all the clock attributes of the original
			modeline. Also all other modeline attributes,
			like doublescan and interlace, are maintained.
		clock - Adjusts the available modeline's vertical clock
			to match the game's frame rate.
		xclock - Adjusts the available modeline's horizontal resolution
			and the vertical clock.
		generate_exact - Creates automatically some new modelines using
			the format specified on the `device_video_format'
			option. The generated modelines will be named `generate-*'.
			Check the `advdef.txt' file for the description of 
			the `device_video_format' option or simply use the 
			`advcfg' utility to set it up correctly.
			If the `device_video_format' option isn't
			specified a default value for your monitor clock
			limits is guessed.
		generate_y - Like generate_exact, and it allows generating
			modes with a wrong vertical size if a perfect mode is not
			possible.
		generate_clock - Like generate_exact, and it allows generating
			modes with a vertical clock different than the game
			original clock if a perfect mode is not	possible.
		generate_yclock - Like generate_exact, and it allows
			generating modes with a wrong vertical clock and/or size 
			if a perfect mode is not possible. Modes with a correct size
			are favorite over mode than a correct clock.
		generate_clocky - Like generate_exact, and it allows
			generating modes with a wrong vertical clock and/or size 
			if a perfect mode is not possible. Modes with a correct clock
			are favorite over mode with a correct size.

	The `generate' options are able to create at runtime all the
	required modelines. You don't need to create a list of modelines
	manually.

	The not `generate' options use only the modelines defined with
	the `device_video_modeline' option in the configuration file.
	You can add them manually or using the `advv' utility.
	Check the `advdev.txt' file for more details on the
	`device_video_modeline' option. 

	Of all the `generate' options, the `generate_yclock' is the
	suggested and the most powerful. The `advcfg' utility always
	sets the `generate_yclock' option in your configuration file.

	Of all the not `generate' options, the `xclock' is the
	suggested and the most powerful.

	If you can't get good result with the `generate' options you
	should create a list of modelines and try with the `xclock' value.
	You don't need to duplicate the same modeline with different
	horizontal resolutions and/or the clocks, because the `xclock'
	value allows the program to adjust them.
	Instead, you should create a wide set of different vertical
	resolutions on which the video mode can be chosen.
	A good choice is to create all the resolutions with a step of
	16 rows.

    display_color
	Controls the color format of the video mode.

	:display_color auto | palette8 | bgr8 | bgr15 | bgr16 | bgr32 | yuy2

	Options:
		auto - Automatically choose the best option (default).
		palette8 - Palettized 8 bits mode.
		bgr8 - RGB 8 bits mode.
		bgr15 - RGB 15 bits mode.
		bgr16 - RGB 16 bits mode.
		bgr32 - RGB 32 bits mode.
		yuy2 - YUV mode in the YUY2 format.

	Note that the 24 bit color mode isn't supported.
	
	The modes are called bgr because in the video memory the order of
	the color channel is: Blue, Green, Red.

    display_resize
	Suggests the favorite image stretching when a video mode
	with the correct size isn't available.
	This option doesn't have any effect for vector games, they are 
	always stretched to fit the whole screen.

	:display_resize none | integer | mixed | fractional

	Options:
		none - Original size.
		integer - Integer stretch, i.e. x2, x3, x4,...
		mixed - Integer horizontal stretch and fractional
			vertical stretch.
		fractional - Fractional stretch (default).

	The `fractional' option involves a slowdown, so use the `mixed'
	option if you have a really slow machine.

	Examples:
		:display_resize mixed

    display_magnify
	Suggests the use of a double or bigger resolution video mode.
	It is mainly used to enable the `scalex', `scalek', `lq', `hq' and `xbr'
	effects. This option doesn't have any effect for vector games.

	:display_magnify auto | 1 | 2 | 3 | 4

	Options:
		auto - Double, triplicate or quadruplicate the size until
			targetting the defined display_magnifysize
			horizontal size (default).
		1 - Normal size.
		2 - Double size.
		3 - Triple size.
		4 - Quadruple size.

    display_magnifysize
	Defines the target area to reach with the auto config of display_magnify.
	The specified value is the edge of the square area to target.
	For example, with 512 the game area is expanded up to reach the size
	of 512*512 pixels.

	:display_magnifysize SIZE

	Options:
		SIZE - Square root of the area to target. Default 600.
			The default of 600 typicall uses a magnify
			factor of 2.

    display_scanlines
	Suggests the use of hardware scanlines when choosing
	the video mode.

	:display_scanlines yes | no

	Options:
		yes - Try to select a singlescan video mode.
		no - Try to select a doublescan video mode (default).

    display_buffer
	Activates the video image buffering.

	:display_buffer yes | no

	Options:
		no - Doesn't use any buffering (default).
		yes - Use the best buffering available.

    display_vsync
	Synchronizes the video display with the video beam instead of
	using the CPU timer. This option can be used only if the
	selected video mode has an appropriate refresh rate.
	To ensure this you can use the option `display_adjust' to allow
	a clock correction of the video mode.

	:display_vsync yes | no

	Options:
		no - Use the timer.
		yes - Use the video refresh rate (default).

	You can enable or disable it also on the runtime Video menu.

    display_restore
	Selects whether or not to reset to default text mode at the
	emulator exit.

	:display_restore yes | no

	Options:
		yes - Resets to text mode (default).
		no - Doesn't change the video mode.

    display_frameskip
	Skips frames to speed up the emulation.

	:display_frameskip auto | FACTOR

	Options:
		auto - Auto frame skip (default).
		FACTOR - Float factor for the fraction of frames
			to display. From 0 to 1. To completely
			disable the frame skipping use the
			value 1.

	Use `f11' to display the speed your computer is actually
	reaching. If it is below 100%, increase the frame skip value.
	You can press `f8/f9' to change frame skip while running the game.
	When set to auto (default), the frame skip setting is
	dynamically adjusted during runtime to display the maximum
	possible frames without dropping below the 100% speed.
	Pressing `f10' you can enable and disable the throttle
	synchronization.

	Examples:
		:display_frameskip 0.5

  Display Aspect Configuration Options
	This section describes the options used to customize the display
	aspect.

    display_expand
	Enlarges the screen area used by the vertical games on horizontal
	monitors (and horizontal games in vertical monitors).

	:display_expand FACTOR

	Options:
		FACTOR - Expansion float factor from 1.0 to 2.0
			(default 1.0).

	Examples:
		:display_expand 1.15

    display_aspectx/aspecty
	Selects the aspect of the monitor used.

	:display_aspectx INT
	:display_aspecty INT

	Options:
		INT - Integer number starting from 1 (default 4 and 3).

	For 16/9 TV you can use the 16 and 9 values. For truly vertical
	monitors (not horizontal monitors rotated) you can simply swap the
	values, for example 3 and 4 instead of 4 and 3.

	Examples:
		:display_aspectx 16
		:display_aspecty 9

    display_ror/rol/flipx/flipy
	Flips and rotates the game image.

	:display_ror yes | no
	:display_rol yes | no
	:display_flipx yes | no
	:display_flipy yes | no

	Examples:
		To rotate left vertical games:

		:vertical/display_rol yes

    display_skiplines/skipcolumns
	Selects the centering of the visible area.

	:display_skiplines auto | SKIPLINES
	:display_skipcolumns auto | SKIPCOLUMNS

	Options:
		auto - Auto center (default).
		SKIPLINES - Lines to skip.
		SKIPCOLUMNS - Columns to skip.

  Display Effect Configuration Options
	This section describes the options used to customize the display
	effects.

    display_resizeeffect
	When a video mode is smaller or bigger than the original arcade
	screen, the `resizeeffect' option controls the type of the
	transformation applied.

	:display_resizeeffect auto | none | max | mean | filter
	:	| scalex | scalek | lq | hq | xbr

	Options:
		auto - Selects automatically the best effect (default).
			This selection is list based, and may be
			incomplete.
			If the scale factor is 2, 3 o 4 the `xbr' effect
			is selected. The effect is automatically downgraded
			to `scalek' or `scalex' if the emulation is too slow.
			On the other cases the `mean' or `max' effect
			is selected.
		none - Simply removes or duplicates lines as required.
		max - In reduction merges consecutive columns and rows using the
			lightest pixels versus the darkest.
			In expansion duplicate columns and rows using the
			darkest pixels versus the lightest.
			Supported in both rgb and palette video modes.
			It works best for the games with black
			background or without scrolling. Like "Pac-Man".
			This effect ensures to have always rows and columns
			of the same thickness.
		mean - In reduction it merges columns and rows using the
			mean color of the pixels. In expansion it adds
			columns and rows that are the mean of previous
			and next lines.
			Supported only in rgb video modes.
			It works best for the games with animated
			or scrolling background. Like "1941".
		filter - It removes or duplicates columns and rows with
			a low pass filter in the x and the y directions.
			It's a simple FIR filter with two points of 
			equal value.
			Supported only in rgb video modes.
		scalex - It adds the missing pixels matching the
			original bitmap pattern.
			It uses a 3x3 mapping analysis with 4 comparisons.
			It doesn't interpolate pixels and it compares colors
			for equality.
			If works only for expansion factor of 2, 3 and 4.
		scalek - It adds the missing pixels matching the
			original bitmap pattern.
			It uses a 3x3 mapping analysis with 4 comparisons.
			It interpolates pixels and it compares colors
			for equality.
			If works only for expansion factor of 2, 3 and 4.
		lq - It adds the missing pixels matching the
			original bitmap pattern.
			It uses a 3x3 mapping analysis with 8 comparisons.
			It interpolates pixels and it compares colors for equality.
			It works only for expansion factor of 2, 3 and 4.
		hq - It adds the missing pixels matching the
			original bitmap pattern.
			It uses a 3x3 mapping analysis with 8 comparisons.
			It interpolates pixels and it compares colors
			for distance.
			It works only for expansion factor of 2, 3 and 4.
		xbr - It adds the missing pixels matching the
			original bitmap pattern.
			It uses a 5x5 mapping analysis with a gradient estimation.
			It interpolates pixels and it compares colors
			for distance.
			It works only for expansion factor of 2, 3 and 4.

    display_rgbeffect
	Selects a special effect to simulate the aspect of an Arcade Monitor 
	with a PC monitor. The resulting image is better when you use a 
	big video mode. These effects require a RGB video mode, they don't
	work with palettized or YUV modes.

	:display_rgbeffect none | triad3dot | triad6dot
	:	| triad16dot | triadstrong3dot | triadstrong6dot
	:	| triadstrong16dot | scan2vert | scan3vert
	:	| scan2horz | scan3horz.

	Options:
		none - No effect (default).
		triad3dot - RGB triad of 3 pixels.
		triad6dot - RGB triad of 6 pixels.
		triad16dot - RGB triad of 16 pixels.
		triadstrong3dot - RGB strong triad of 3 pixels.
		triadstrong6dot - RGB strong triad of 6 pixels.
		triadstrong16dot - RGB strong triad of 16 pixels.
		scan2vert - Scanline of 2 vertical lines.
		scan3vert - Scanline of 3 vertical lines.
		scan2horz - Scanline of 2 horizontal lines.
		scan3horz - Scanline of 3 horizontal lines.

    display_interlaceeffect
	Selects some special effects for interlaced video modes.
	On not interlaced modes the effects are always disabled.
	
	:display_interlaceeffect none | even | odd | filter

	Options:
		none - No effect (default).
		even - Swap the even rows.
		odd - Swap the odd rows.
		filter - Apply a vertical filter.

	If your monitor uses a swapped order for interlaced rows, using the 
	`even' or `odd' effect you can probably fix the image.

	The effects operate on the rows in the following way:

		:Row Even Odd Filter
		: A   A   A    A
		: B   B   A   B+A
		: C   A   C   C+B
		: D   D   B   D+C
		: E   C   E   E+D
		: F   F   D   F+E
		: G   E   G   G+F
		: H   H   F   H+G

    Display Color Configuration Options
	This section describes the options used to customize the display
	color adjustments.

    display_brightness
	Selects the image brightness factor.

	:display_brightness FACTOR

	Options:
		FACTOR - Brightness float factor (default 1.0).

    display_gamma
	Sets the image gamma correction factor.

	:display_gamma FACTOR

	Options:
		FACTOR - Gamma float factor (default 1.0).

    display_pausebrightness
	Selects the brightness of the display when game is paused.

	:display_pausebrightness FACTOR

	Options:
		FACTOR - Float brightness factor.
			From 0.0 to 1.0 (default 1.0).

	Examples:
		:display_pausebrightness 0.6

  Display Artwork Configuration Options
	This section describes the options used to customize the display
	artwork.

    display_artwork_backdrop/overlay/bezel
	Enables or disables the artworks display.

	:display_artwork_backdrop yes | no
	:display_artwork_overlay yes | no
	:display_artwork_bezel yes | no

	Options:
		yes - Enables the artwork (default for backdrop
			and overlay).
		no - Doesn't display the artwork (default for
			bezel).

    display_artwork_crop
	Crops the artwork at the game size.

	:display_artwork_crop yes | no

	Options:
		yes - Crops the artwork (default).
		no - Doesn't crop the artwork.

  Display Vector Configuration Options
	This section describes the options used to customize the display
	of vector games.

    display_antialias
	Enables or disables the anti-aliasing for vector games.

	:display_antialias yes | no
	
	Options:
		yes - Anti-aliasing enabled (default)
		no - Anti-aliasing disabled.

    display_beam
	Sets width in pixels of vectors.

	:display_beam SIZE

	Options:
		SIZE - A float in the range of 1.0 through 16.0
			(default 1.0).

    display_flicker
	Makes vectors flicker.

	:display_flicker FACTOR

	Options:
		FACTOR - A float in the range 0.0 - 100.0
			(default 0).

    display_translucency
	Enables or disables vector translucency.

	:display_translucency yes | no

    display_intensity
	Sets the vector intensity.

	:display_intensity FACTOR

	Options:
		FACTOR - A float in the range 0.5 - 3.0
			(default 1.5).

  Sound Configuration Options
	This section describes the options used to customize the sound.

    device_sound_*
	These options are used to customize the audio drivers.

	All the `device_sound_*' options defined in the `advdev.txt'
	file can be used.

    sound_mode
	Sets the sound output mode.

	:sound_mode auto | mono | stereo | surround

	Options:
		auto - Use mono if the emulated game is mono
			or stereo if it's stereo (default).
		mono - Use always mono. The game stereo channels
			are mixed.
		stereo - Use always stereo. The game mono channel
			is duplicated.
		surround - Use a fake surround effect. With
			stereo games the right channel plays part of
			the left channel as negate samples and vice-versa.
			With mono games the left channel is the negation of
			the right channel. This means that with surround
			enabled the output is always stereo.
			If you use mono headphones, the effect will not
			work and you will hear silence or an attenuated
			sound.

    sound_samplerate
	Sets the audio sample rate.

	:sound_samplerate RATE

	Options:
		RATE - Sample rate. Common values are 11025, 22050,
			44100 and 48000 (default 44100).
			
	If the sound driver doesn't support the specified sample rate a 
	different value is selected.

    sound_volume
	Sets the global sound volume.

	:sound_volume VOLUME

	Options:
		VOLUME - The volume attenuation in dB (default -3).
			The attenuation is a negative value from -40 to 0.

	Examples:
		:sound_volume -5

    sound_adjust
	Sets the sound gain volume. This option can be used to
	adjust the volume of some games to have all the games with
	the same volume.
	If the `sound_normalize' option is active this value is
	also automatically updated in the game play and you cannot
	change it manually.

	:sound_adjust auto | VOLUME

	Options:
		auto - Get the value for the current game from
			an internal database.
		VOLUME - The volume gain in dB (default 0).
			The gain is a positive value from 0 to 40.
			If the `sound_normalize' option is active the
			volume is automatically updated in the game
			play.

	Examples:
		:sound_adjust 16

    sound_normalize
	Automatically increases and decreases the sound volume.

	:sound_normalize yes | no

	Precisely, the program continously measures the normalized
	sound power adjusting it with the Fletcher-Munson "Equal
	Loudness Courve" at 80 dB to remove inaudible frequencies.
	It tries to keep constant the 95% median power of the
	last 3 minutes.

	For more details check:

		+http://replaygain.hydrogenaudio.org/equal_loudness.html

	Options:
		yes - Enable the volume normalization (default).
		no - Disable it.

    sound_equalizer_*
	Sets the equalizer volume. To disable the equalizer set
	all the VOLUME values to 0. The cut-off frequencies are 800
	and 8000 Hz.

	sound_equalizer_lowvolume VOLUME
	sound_equalizer_midvolume VOLUME
	sound_equalizer_highvolume VOLUME

	Options:
		VOLUME - The volume gain in dB (default 0).
			The gain is a integer value from -20 to 20.

    sound_latency
	Sets the minimum audio latency.

	:sound_latency TIME

	Options:
		TIME - Latency in seconds from 0 to 2.0
			(default 0.05).

	If in the game play you hear some sound ticks you can try to
	increase the latency. Try doubling the value until the ticks
	go away.

  Input Configuration Options
	This section describes the options used to customize the user
	input.

    device_keyboard/joystick/mouse_*
	These options are used to customize the input drivers.

	All the `device_keyboard/joystick/mouse_*' options defined in
	the `advdev.txt' file can be used.

    input_steadykey
	Helps recognition of very long key sequences. But slows a bit
	the key recognition.

	:input_steadykey yes | no

	Options:
		no - Standard key recognition (default).
		yes - Wait until the keyboard state is stabilized
			before report any key change.

    input_hotkey
	Enables or disables the recognition of the special OS keyboard
	sequences.

	:input_hotkey yes | no

	Options:
		no - No hot key recognition.
		yes - Hot key recognition (default).

	In DOS the hotkey recognized are:
		CTRL+ALT+DEL - Reset.
		CTRL+ALT+END - Quit.
		CTRL+BREAK (Pause) - Break.

	In Linux the hotkey recognized generally are:
		CTRL+C - Break.
		ALT+Fx - Change virtual console.

    input_idleexit
	Activates the automatic exit after some time of inactivity.

	:input_idleexit TIME

	Options:
		TIME - Number of seconds to wait, if 0 (default)
			never exits automatically.

    input_map[ANALOG]
	Changes the analog control mapping. Maps joystick, trackball
	and mouse controls on a player analog control.

	:input_map[ANALOG] auto | [[-]joystick[JOY,CONTROL,AXE]]
	:	[[-]mouse[MOUSE,AXE]] [[-]joystick_ball[JOY,AXE]] ...

	The default is always `auto'.

	Options:
		ANALOG - Player analog control. One of:
			p1_paddlex, p2_paddlex, p3_paddlex, p4_paddlex,
			p1_paddley, p2_paddley, p3_paddley, p4_paddley,
			p1_stickx, p2_stickx, p3_stickx, p4_stickx,
			p1_sticky, p2_sticky, p3_sticky, p4_sticky,
			p1_stickz, p2_stickz, p3_stickz, p4_stickz,
			p1_lightgunx, p2_lightgunx, p3_lightgunx, p4_lightgunx,
			p1_lightguny, p2_lightguny, p3_lightguny, p4_lightguny,
			p1_pedalgas, p2_pedalgas, p3_pedalgas, p4_pedalgas,
			p1_pedalbrake, p2_pedalbrake, p3_pedalbrake, p4_pedalbrake,
			p1_pedalother, p2_pedalother, p3_pedalother, p4_pedalother,
			p1_dialx, p2_dialx, p3_dialx, p4_dialx,
			p1_dialy, p2_dialy, p3_dialy, p4_dialy,
			p1_trackballx, p2_trackballx, p3_trackballx, p4_trackballx,
			p1_trackbally, p2_trackbally, p3_trackbally, p4_trackbally,
			p1_mousex, p2_mousex, p3_mousex, p4_mousex,
			p1_mousey, p2_mousey, p3_mousey, p4_mousey.
		- - Invert the direction of the movement.
		JOY - Number of physical joystick: 0, 1, 2, 3, ...
		MOUSE - Number of physical mouse: 0, 1, 2, 3, ...
		CONTROL - Number or name of physical control of the joystick: 0, 1, 2, 3, ...
		AXE - Number or name of physical axe of the
			control: 0, 1, 2, 3 ...

	The `joystick' option is used to reference all the analog
	controls which report an absolute position. For example
	stick, lightgun, steering wheel, pedal, throttle and rudder controls.

	The `joystick_ball' and `mouse' options are used to reference
	all the analog controls which reports a relative position. For
	example trackball, dial and mouse controls.
	The option `joystick_ball' is for joystick devices which
	have other relative analog controls. The option `mouse' is
	for mouse devices which have only relative analog controls.

	The exact CONTROL and AXE names can be checked interactively
	using the `advj' and `advm' utility. `advj' is used for
	`joystick[]' and `joystick_ball[]' specifications. `advm' is
	used for `mouse[]' specifications.

	The CONTROL names for the `joystick' option are:
		stick - Stick.
		gas - Acceleration pedal.
		brake - Brake pedal.
		wheel - Steering wheel.
		hat, hat2, hat3, hat4 - Hats.
		throttle - Throttle.
		rudder - Rudder.
		misc - Any other.

	The AXE names for the `joystick' option are:
		x, y, z - Movement on the X, Y, Z axe.
		rx, ry, rz - Rotation on the X, Y, Z axe.
		mono - For all the control with a single axe.

	The AXE names for the `joystick_ball' and `mouse'
	options are:
		x, y, z - Movement on the X, Y, Z axe.
		wheel - Vertical wheel.
		hwheel - Horizontal wheel.
		dial - Dial.
		misc - Any other.

	The ANALOG controls are always formed with a "player" string
	and with a "control" strings.
		p1_CONTROL - Player 1.
		p2_CONTROL - Player 2.
		p3_CONTROL - Player 3.
		p4_CONTROL - Player 4.
		PLAYER_paddlex - Paddle in horizontal direction.
		PLAYER_paddley - Paddle in vertical direction.
		PLAYER_stickx - Stick in horizontal direction.
		PLAYER_sticky - Stick in vertical direction.
		PLAYER_stick_z - Stick in z axis direction.
		PLAYER_lightgunx - Lightgun in horizontal direction.
		PLAYER_lightguny - Lightgun in vertical direction.
		PLAYER_pedalgas - Gas pedal.
		PLAYER_pedalbrake - Brake pedal.
		PLAYER_pedalother - Other pedal.
		PLAYER_dialx - Dial in horizontal direction.
		PLAYER_dialy - Dial in vertical direction.
		PLAYER_trackballx - Trackball in horizontal direction.
		PLAYER_trackbally - Trackball in vertical direction.
		PLAYER_mousex - Mouse in horizontal direction.
		PLAYER_mousey - Mouse in vertical direction.

	Examples:
		:input_map[p1_stickx] joystick[0,0,x] -joystick[0,1,x]
		:input_map[p1_sticky] joystick[0,0,y] -joystick[0,1,y]
		:input_map[p1_trackballx] mouse[0,x] -mouse[1,x]
		:input_map[p1_trackbally] mouse[0,y] -mouse[1,y]

	If required you can compose the options to get a rotation
	of 45° of the control. For example:

		:input_map[p1_stickx] mouse[0,x] mouse[0,y]
		:input_map[p1_sticky] mouse[0,x] -mouse[0,y]

    input_map[DIGITAL]
	Changes the digital control mapping. Maps a sequence of
	keyboard/mouse/joystick keys on a player button or analog
	simulation digital control.

	:input_map[DIGITAL] auto | keyboard[KEYBOARD,KEY]
	:	| mouse_button[MOUSE,MOUSE_BUTTON] | joystick_button[JOY,JOY_BUTTON]
	:	| joystick_digital[JOY,CONTROL,AXE,DIR]
	:	| or | not | ...

	The default is always `auto' which uses the standard mapping.
	The previous mapping is always overwritten.

	Options:
		DIGITAL - Player button, analog simulation, digital or
			keyboard control.
		KEYBOARD - Number of physical keyboard: 0, 1, 2, 3, ...
		KEY - Name of physical key.
		MOUSE - Number of physical mouse: 0, 1, 2, 3, ...
		MOUSE_BUTTON - Number or name of a physical mouse button:
			0, 1, 2, 3, ...
		JOY - Number of physical joystick: 0, 1, 2, 3, ...
		CONTROL - Number or name of physical control of the joystick: 0, 1, 2, 3, ...
		DIR - Direction of the movement: left, up, right, down.
		AXE - Number or name of physical axe of the control: 0, 1, 2, 3, ...
		JOY_BUTTON - Number or name of a physical joystick button:
			0, 1, 2, 3, ...
		or - Or operand.
		not - Not operand.
		auto - Use the default mapping.

	The DIGITAL controls are:
		p1_up, p2_up, p3_up, p4_up, p1_down, p2_down, p3_down,
		p4_down, p1_left, p2_left, p3_left, p4_left, p1_right,
		p2_right, p3_right, p4_right, p1_doubleright_up, p2_doubleright_up,
		p3_doubleright_up, p4_doubleright_up, p1_doubleright_down,
		p2_doubleright_down, p3_doubleright_down, p4_doubleright_down,
		p1_doubleright_left, p2_doubleright_left, p3_doubleright_left,
		p4_doubleright_left, p1_doubleright_right, p2_doubleright_right,
		p3_doubleright_right, p4_doubleright_right, p1_doubleleft_up,
		p2_doubleleft_up, p3_doubleleft_up, p4_doubleleft_up, p1_doubleleft_down,
		p2_doubleleft_down, p3_doubleleft_down, p4_doubleleft_down,
		p1_doubleleft_left, p2_doubleleft_left, p3_doubleleft_left,
		p4_doubleleft_left, p1_doubleleft_right, p2_doubleleft_right,
		p3_doubleleft_right, p4_doubleleft_right, p1_button1, p2_button1,
		p3_button1, p4_button1, p1_button2, p2_button2, p3_button2,
		p4_button2, p1_button3, p2_button3, p3_button3, p4_button3,
		p1_button4, p2_button4, p3_button4, p4_button4, p1_button5,
		p2_button5, p3_button5, p4_button5, p1_button6, p2_button6,
		p3_button6, p4_button6, p1_button7, p2_button7, p3_button7,
		p4_button7, p1_button8, p2_button8, p3_button8, p4_button8,
		p1_button9, p2_button9, p3_button9, p4_button9, p1_button10,
		p2_button10, p3_button10, p4_button10, p1_paddle_left,
		p1_paddle_right, p2_paddle_left, p2_paddle_right, p3_paddle_left,
		p3_paddle_right, p4_paddle_left, p4_paddle_right, p1_paddle_up,
		p1_paddle_down, p2_paddle_up, p2_paddle_down, p3_paddle_up,
		p3_paddle_down, p4_paddle_up, p4_paddle_down, p1_stick_left,
		p1_stick_right, p2_stick_left, p2_stick_right, p3_stick_left,
		p3_stick_right, p4_stick_left, p4_stick_right, p1_stick_up,
		p1_stick_down, p2_stick_up, p2_stick_down, p3_stick_up,
		p3_stick_down, p4_stick_up, p4_stick_down, p1_stick_forward,
		p1_stick_backward, p2_stick_forward, p2_stick_backward,
		p3_stick_forward, p3_stick_backward, p4_stick_forward,
		p4_stick_backward, p1_lightgun_left, p1_lightgun_right,
		p2_lightgun_left, p2_lightgun_right, p3_lightgun_left,
		p3_lightgun_right, p4_lightgun_left, p4_lightgun_right,
		p1_lightgun_up, p1_lightgun_down, p2_lightgun_up, p2_lightgun_down,
		p3_lightgun_up, p3_lightgun_down, p4_lightgun_up, p4_lightgun_down,
		p1_pedalgas_push, p1_pedalgas_release, p2_pedalgas_push,
		p2_pedalgas_release, p3_pedalgas_push, p3_pedalgas_release,
		p4_pedalgas_push, p4_pedalgas_release, p1_pedalbrake_push,
		p1_pedalbrake_release, p2_pedalbrake_push, p2_pedalbrake_release,
		p3_pedalbrake_push, p3_pedalbrake_release, p4_pedalbrake_push,
		p4_pedalbrake_release, p1_pedalother_push, p1_pedalother_release,
		p2_pedalother_push, p2_pedalother_release, p3_pedalother_push,
		p3_pedalother_release, p4_pedalother_push, p4_pedalother_release,
		p1_dial_left, p1_dial_right, p2_dial_left, p2_dial_right,
		p3_dial_left, p3_dial_right, p4_dial_left, p4_dial_right,
		p1_dial_up, p1_dial_down, p2_dial_up, p2_dial_down, p3_dial_up,
		p3_dial_down, p4_dial_up, p4_dial_down, p1_trackball_left,
		p1_trackball_right, p2_trackball_left, p2_trackball_right,
		p3_trackball_left, p3_trackball_right, p4_trackball_left,
		p4_trackball_right, p1_trackball_up, p1_trackball_down,
		p2_trackball_up, p2_trackball_down, p3_trackball_up, p3_trackball_down,
		p4_trackball_up, p4_trackball_down, p1_mouse_left, p1_mouse_right,
		p2_mouse_left, p2_mouse_right, p3_mouse_left, p3_mouse_right,
		p4_mouse_left, p4_mouse_right, p1_mouse_up, p1_mouse_down,
		p2_mouse_up, p2_mouse_down, p3_mouse_up, p3_mouse_down,
		p4_mouse_up, p4_mouse_down, start1, start2, start3, start4,
		coin1, coin2, coin3, coin4, coin5, coin6, coin7, coin8,
		bill1, service_coin1, service_coin2, service_coin3, service_coin4,
		service_coin5, service_coin6, service_coin7, service_coin8,
		service, tilt, interlock, p1_start, p2_start, p3_start,
		p4_start, p1_select, p2_select, p3_select, p4_select, ui_mode_next,
		ui_mode_pred, ui_record_start, ui_record_stop, ui_turbo,
		ui_cocktail, ui_help, ui_startup, ui_configure, ui_on_screen_display,
		ui_pause, ui_reset_machine, ui_show_gfx, ui_frameskip_dec,
		ui_frameskip_inc, ui_throttle, ui_show_fps, ui_snapshot,
		ui_toggle_cheat, ui_home, ui_end, ui_up, ui_down, ui_left, ui_right,
		ui_select, ui_cancel, ui_pan_up, ui_pan_down, ui_pan_left, ui_pan_right,
		ui_show_profiler, ui_toggle_ui, ui_toggle_debug, ui_save_state,
		ui_load_state, ui_add_cheat, ui_delete_cheat, ui_save_cheat,
		ui_watch_value, ui_edit_cheat, ui_toggle_crosshair, safequit,
		event1, event2, event3, event4, event5, event6, event7,
		event8, event9, event10, event11, event12, event13, event14,
		key_q, key_w, key_e, key_r, key_t, key_y, key_u, key_i,
		key_o, key_p, key_a, key_s, key_d, key_f, key_g, key_h,
		key_j, key_k, key_l, key_z, key_x, key_c, key_v, key_b,
		key_n, key_m, key_pad_0, key_pad_1, key_pad_2, key_pad_3,
		key_pad_4, key_pad_5, key_pad_6, key_pad_7, key_pad_8,
		key_pad_9, key_pad_enter, key_pad_minus, key_pad_plus,
		key_pad_slash, key_pad_colon, key_pad_diesis, key_pad_asterisk,
		key_0, key_1, key_2, key_3, key_4, key_5, key_6, key_7,
		key_8, key_9, key_esc, key_enter, key_backspace, key_tab,
		key_space, key_ins, key_del, key_home, key_end, key_fctn,
		key_restore, key_store, key_play, key_print, key_hold,
		key_rew, key_record, key_break, key_graph, key_pause, key_menu,
		key_stop, key_again, key_undo, key_move, key_copy, key_open,
		key_edit, key_paste, key_find, key_cut, key_help, key_back,
		key_forward, key_capslock, key_scrlock, key_numlock, key_quickload,
		key_pgup, key_pgdn, key_backquote, key_minus, key_plus,
		key_asterisk, key_equals, key_openbrace, key_closebrace,
		key_semicolon, key_quote, key_backslash, key_less, key_comma,
		key_period, key_slash, key_colon, key_pound, key_doublequote,
		key_diesis, key_lshift, key_rshift, key_lctrl, key_rctrl,
		key_lalt, key_ralt, key_ctrl, key_alt, key_shift, key_left,
		key_right, key_up, key_down, key_f1, key_f2, key_f3, key_f4,
		key_f5, key_f6, key_f7, key_f8, key_f9, key_f10, key_f11,
		key_f12, key_f13, key_f14, key_f15, key_f16, key_f17, key_f18,
		key_f19, key_f20, key_f21, key_f22, key_f23, key_f24.

	Note that the p*_start, p*_select and key_* controls are avaiable
	only in MESS.

	The MOUSE_BUTTON names can be checked using the `advm' utility.
	Generally they are:
		left, right, middle - Standard buttons.
		side - Side button.
		extra - Extra button.
		forward - Forward button.
		back - Back button.
		fourth, fifth, sixth - Misc buttons.

	The KEY names can be checked using the `advk' utility.
	Generally they are:
		a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u,
		v, w, x, y, z, n0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0_pad, 1_pad,
		2_pad, 3_pad, 4_pad, 5_pad, 6_pad, 7_pad, 8_pad, 9_pad, f1, f2, f3,
		f4, f5, f6, f7, f8, f9, f10, f11, f12, esc, backquote, minus,
		equals, backspace, tab, openbrace, closebrace, enter, semicolon,
		quote, backslash, less, comma, period, slash, space, insert,
		del, home, end, pgup, pgdn, left, right, up, down, slash_pad, asterisk_pad,
		minus_pad, plus_pad, period_pad, enter_pad, prtscr, pause, lshift,
		rshift, lcontrol, rcontrol, lalt, ralt, lwin, rwin, menu, scrlock,
		numlock, capslock, stop, again, props, undo, front, copy, open,
		paste, find, cut, help, calc, setup, sleep, wakeup, file, sendfile,
		deletefile, xfer, prog1, prog2, www, msdos, coffee, direction,
		cyclewindows, mail, bookmarks, computer, back, forward, closecd,
		ejectcd, ejectclosecd, nextsong, playpause, previoussong, stopcd,
		record, rewind, phone, iso, config, homepage, refresh, exit,
		move, edit, scrollup, scrolldown, leftparen_pad, rightparen_pad,
		intl1, intl2, intl3, intl4, intl5, intl6, intl7, intl8, intl9,
		lang1, lang2, lang3, lang4, lang5, lang6, lang7, lang8, lang9,
		playcd, pausecd, prog3, prog4, suspend, close, brightnessdown,
		brightnessup, macro, mute, volumedown, volumeup, power, compose,
		f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24.

	You can also specify a numerical scancode N in the form: `scanN'.

	The JOY_BUTTON button names can be checked using the `advj' utility.
	Generally they are:
		trigger - Joystick trigger button.
		top, top2 - Joystick top buttons.
		thumb, thumb2 - Joystick thumb buttons.
		pinkie - Joystick pinkie button.
		base, base2, base3, base4, base5, base6 - Joystick base buttons.
		dead - Joystick dead button.
		a, b, c, x, y, z - GamePad buttons.
		tl, tr, tl2, tr2 - GamePad top (left/right) buttons.
		thumbl thumbr - GamePad thumb (left/right) buttons.
		select, start, mode - GamePad extra buttons.
		gear_up, gear_down - Wheel gear buttons.
		left, right, middle - Ball standard buttons.
		side - Ball side button.
		extra - Ball extra button.
		forward - Ball forward button.
		back - Ball back button.
		fourth, fifth, sixth - Ball misc buttons.
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9 - Misc buttons.

	Examples:
		:input_map[p1_left] keyboard[0,left] or joystick_digital[0,stick,x,left]
		:input_map[p1_right] keyboard[0,right] or joystick_digital[0,stick,x,right]
		:input_map[p1_button1] keyboard[0,lshit] or joystick_button[0,trigger]

    input_name
	Changes the display name of the specified digital input.

	:input_name (keyboard[KEYBOARD,KEY]
	:	| mouse_button[MOUSE,MOUSE_BUTTON]
	:	| joystick_button[JOY,JOY_BUTTON]
	:	| joystick_digital[JOY,CONTROL,AXE,DIR])
	:	NAME

	Options:
		KEY - Name of physical key.
		MOUSE - Number of physical mouse: 0, 1, 2, 3, ...
		MOUSE_BUTTON - Number or name of a physical mouse button:
			0, 1, 2, 3, ...
		JOY - Number of physical joystick: 0, 1, 2, 3, ...
		CONTROL - Number or name of physical control of the joystick: 0, 1, 2, 3, ...
		DIR - Direction of the movement: left, up, right, down.
		AXE - Number or name of physical axe of the control: 0, 1, 2, 3, ...
		JOY_BUTTON - Number or name of a physical joystick button:
			0, 1, 2, 3, ...
		NAME - Display name for digital input.

	See input_map for valid KEY, MOUSE_BUTTON, CONTROL, JOY_BUTTON names.

	Examples:
		:input_name keyboard[0,lcontrol] Blue
		:input_name joystick_digital[0,stick,x,right] Right

    input_setting[*]
	Selects some additional settings for analog inputs. These
	settings can be modified using the "Analog Config" menu
	present if the game has analog controls.

	:input_setting[NAME] SETTING

    input_dipswitch[*]
	Selects the state of the game dipswitch. These settings can be
	modified using the "Dipswitch" menu present if the game has
	dipswitches.

	:input_dipswitch[NAME] SETTING

    input_configswitch[*]
	Selects the state of the game configswitch. These settings can
	be modified using the "Config" menu present if the game has
	configswitches.

	:input_configswitch[NAME] SETTING

  User Interface Configuration Options
	This section describes the options used to customize the user 
	interface.

    ui_helpimage
	Selects the image to display on help request. The image must be 
	a PNG file. The pixels in black are used as background, any other
	color is used as foreground. Please note that the displayed image
	is always black and white.

	:ui_helpimage auto | FILE

	Options:
		auto - Use the internal help image. With this option all the
			ui_helptag options are ignored.
		FILE - Load an arbitrary image from a file.

	The data used for the default image is in the `contrib/help' dir.

    ui_helptag
	Selects the highlight range for any digital input. When the user
	press a key/button the related range is highlighted.
	A different color for any player is used in the image.

	:ui_helptag (keyboard[KEYBOARD,KEY]
	:	| mouse_button[MOUSE,MOUSE_BUTTON]
	:	| joystick_button[JOY,JOY_BUTTON]
	:	| joystick_digital[JOY,CONTROL,AXE,DIR])
	:	X Y DX DY

	Options:
		keyboard/mouse_*/joystick_* - One digital input. Like the
			input_map option.
		X, Y - The upper/left position in the image of the input range.
		DX, DY - The size in pixel of the input range.

	Examples:
		:ui_helptag keyboard[0,esc] 6 5 12 12
		:ui_helptag keyboard[0,f1] 26 5 12 12
		:ui_helptag keyboard[0,f2] 38 5 12 12
		:ui_helptag keyboard[0,f3] 50 5 12 12
		:ui_helptag keyboard[0,f4] 62 5 12 12
		:ui_helptag keyboard[0,f5] 81 5 12 12
		:ui_helptag keyboard[0,f6] 93 5 12 12
		:ui_helptag keyboard[0,f7] 105 5 12 12
		:ui_helptag keyboard[0,f8] 117 5 12 12
		:ui_helptag keyboard[0,f9] 137 5 12 12
		:ui_helptag keyboard[0,f10] 149 5 12 12

	The data used for the default image is in the `contrib/help' dir.

    ui_font
	Selects a font file. The formats TrueType (TTF), GRX, PSF and
	RAW are supported. You can find a collection of fonts in the
	`contrib' directory.

	:ui_font auto | "FILE"

	Options:
		auto - Use the built-in font (default).
		FILE - Font file path.

	The TrueType (TTF) format is supported only if the program is
	compiled with the FreeType2 library.

    ui_fontsize
	Selects the font size, if the specified font is scalable.
	The size is expressed in number of rows and columns of text in the
	screen.

	:ui_fontsize auto | ROWS [COLS]

	Options:
		auto - Automatically compute the size (default).
		ROWS - Number of text rows.
		COLS - Number of text columns. If omitted, it's computed from
			the number of rows.

    ui_color[*]
	Selects the user interface colors.

	ui_color[TAG] FOREGROUND BACKGROUND

	Tags:
		interface - Main inteface color.
		tag - Highlight tags.
		select - Selected menu entries.
		help_p1 - Help color for player 1 controls.
		help_p2 - Help color for player 2 controls.
		help_p3 - Help color for player 3 controls.
		help_p4 - Help color for player 4 controls.
		help_other - Help color for other controls

	Options:
		FOREGROUND - Foreground color in RRGGBB
			hex format. For example FF0000 is red
			and 00FF00 is green.
		BACKGROUND - Background color. Like foreground color.

    ui_translucency
	Selects the translucency of the user interface.

	ui_translucency FACTOR

	Options:
		FACTOR - Translucency factor from 0 to 1
			(default 0.8).

	The translucency ha no effect on the 8 bits palettized video
	modes.

  Record Configuration Options
	This section describes the options used for the recording
	features.

	To start the recording press `left_ctrl+enter'. To end the
	recording and to save the clip press `enter'. If you press the
	start key more than one time the recording starts from the
	last press.

    record_sound
	Enables or disables the sound recording.

	:record_sound yes | no

	The sound clip is saved in the `dir_snap' directory (like the
	snapshot images) in `.wav' format. Specifically the `WAV PCM
	16 bit' format is used. The sample rate used is the same sample
	rate specified with the `sound_samplerate' option.

	The clip is saved without compression and without any volume
	adjustment. You should use an external utility to adjust the
	volume and compress the resulting file.

    record_video
	Enables or disables the video recording.

	:record_video yes | no

	The video clip is saved in the `dir_snap' directory (like the
	snapshot images) in `.mng' format. The `MNG-LC' (Low Complexity)
	subformat is used.

	The clip is saved with a lite compression, you should use an
	external utility to compress better the resulting file.

	A powerful compression utility is the `advmng' program
	available in the AdvanceCOMP package.

    record_video/sound_time
	Controls the maximum length in seconds of the recording feature.

	:record_sound_time TIME
	:record_video_time TIME

	Options:
		TIME - Time in seconds (default 15).

    record_video_interleave
	Selects how many frames to save with the video recording.

	:record_video_interleave COUNT

	Options:
		COUNT - How many frames displayed a frame must be saved
			(default 2). 1 means save all the frames. 2
			means save 1 every 2. 3 means save 1 every 3,
			and so on.

	Examples:
		:record_video_interleave 1

  Synchronization Options
	This section describes the options used for the time synchronization
	of the emulated game or system.

    sync_fps
	Selects an arbitrary frame rate for the game.

	:sync_fps auto | FPS

	Options:
		auto - Use the original framerate of the game (default).
		FPS - Use the specified framerate.

    sync_speed
	Selects a speed factor always active. You can play the game
	in slowdown or in nightmare mode!

	:sync_speed FACTOR

	Options:
		FACTOR - Float speed factor (default 1.0).

	Examples:
		:sync_speed 1.2

    sync_turbospeed
	Selects the speed factor used when the `turbo' button is
	pressed. The default `turbo' key is `asterisk_pad'.

	:sync_turbospeed FACTOR

	Options:
		FACTOR - Float speed factor (default 3.0).

    sync_startuptime
	Selects the time in seconds of the duration of the startup
	speed up. You can press the `startup' key to save the current
	game time as startup time. The default `startup' key
	is `minus_pad'.

	:sync_startuptime auto | none | TIME

	Options:
		auto - Get the value for the current game from
			an internal database (default).
		none - Disable the startup.
		TIME - Time in seconds.

    sync_resample
	Selects the audio resampling mode.

	:sync_resample auto | emulation | internal

	Options:
		auto - Select automatically, at present it's always
			the `internal' mode (default).
		emulation - Change the emulation to produce the requested
			number of samples instead of resampling.
		internal - Internally resample the sound to match the
			current speed.

	Note that the `emulation' mode may result in wrong input recording
	using the `-record' or `-playback' command line option due incorrect
	behavior of the emulation. Specifically some implementations may
	depend on the number of audio sample requested, information that
	is not stored in the recorded input file.

	Also, the `emulation' mode may trigger some bugs in the MAME
	core.

	Anyway, the `emulation' mode generates more stable sound
	if the CPU load isn't totally empty.

  LCD Configuration Options
	AdvanceMAME is able to display arbitrary information on a LCD display
	using the integrated script capabilities.

	To use the LCD support you must install on your system the `lcdproc'
	program available at:

		+http://lcdproc.sourceforge.net/

	More details and some examples of how to display information on the LCD
	using the scripts are in the `script.txt' documentation file.

    lcd_server
	Selects the server address and port to use for display information on
	the LCD.

	lcd_server none | [SERVER][:PORT]

	Options:
		none - Disable the LCD support (default).
		SERVER - Address of the server. If omitted `localhost' is used.
		PORT - Port of the server. If omitted `13666' is used.

	Examples:
		:lcd_server localhost

    lcd_timeout
	Selects the timeout to connect at the server.

	lcd_timeout TIMEOUT

	Options:
		TIMEOUT - Timeout in milliseconds (default 500).

    lcd_speed
	Selects the LCD scrolling speed for long messages.

	lcd_speed DELAY

	Options:
		DELAY - Delay in 1/8th of seconds (default 8).

  Misc Configuration Options

    misc_bios
	Selects the game bios in AdvanceMAME.

	:misc_bios default | NAME

	Options:
		default - Use the default BIOS.
		NAME - Select specific BIOS.

    misc_ramsize
	Controls the ram size of the emulated machine in AdvanceMESS.

	:misc_ramsize auto | SIZE [k|M|G]

	Options:
		auto - Automatic (default).
		SIZE - Size of the RAM in bytes. You can use the 'k' (1024),
			'M' (1024^2) or 'G' (1024^3) multiplier.

	Examples:
		:misc_ramsize 1024k

    misc_difficulty
	Selects the game difficulty. This option works only with games
	which select difficulty with dipswitches.

	:misc_difficulty none | easiest | easy | normal | hard | hardest

	Options:
		none - Don't change the default difficulty (default).
		easiest - Easiest game play.
		easy - Easy game play.
		normal - Normal game play.
		hard - Hard game play.
		hardest - Hardest game play.

    misc_freeplay
	Selects the freeplay mode if the game support it. This
	option works only with games that select the freeplay mode
	with dipswitches.

	:misc_freeplay yes | no

	Options:
		no - Don't change the default mode (default).
		yes - Activate the freeplay.

    misc_mutedemo
	Selects the demo nosound mode if the game support it. This
	option uses the dipswitches of the game and also the
	event database to detect if the game is in demo mode.

	:misc_mutedemo yes | no

	Options:
		no - Don't change the default mode (default).
		yes - Mute the demo sounds.

    misc_lang
	Selects the game language and country. This option uses
	both the dipswitches and the clones description to select
	the correct game to run.

	:misc_lang none | usa | canada | englang | italy | germany
	:	| spain | austria | norway | france | denmark
	:	| japan | korea | china | hongkong | taiwan

	Options:
		none - Don't change the language (default).
		LANG - If available select this language.

	For example if you run `cadash' with `-lang italy' it's
	run the clone `cadashi'. If you run `cheesech' with
	`-lang germany' it's run `cheesech' setting the `Language'
	dipswitch to `German'.

	If the specified language is not available, european and
	american languages fallback to english language.
	Asian languages fallback to japanese language.

	A game clone is selected only if it's a perfectly working
	clone.

    misc_smp
	Enables the "Symmetric Multi-Processing" (SMP).
	This option uses two concurrent threads. One for MAME and one
	for updating the screen.
	The final blit stage in video memory is completely done by the
	second thread. This behavior requires a complete bitmap redraw
	by MAME for the games that don't already do it.
	Generally you get a big speed improvement only if you are using
	a heavy video effect like `hq' and `xbr'.

	:misc_smp yes | no

	Options:
		no - Disabled (default).
		yes - Enabled.

	You can enable or disable it also on the runtime Video menu.

    misc_quiet
	Doesn't print the copyright text message at the startup, the
	disclaimer and the generic game information screens.

	:misc_quiet yes | no

    misc_timetorun
	Run the emulation only for the given number of seconds without
	any throttling and at the exit print the number of real CPU
	seconds used. Useful for benchmarking.

	:misc_timetorun SECONDS

  Support Files Configuration Options
	The AdvanceMAME emulator can use also some support files:

		event.dat - The Event and Safequit database.
		cheat.dat - Cheat database.
		hiscore.dat - Highscore database.
		sysinfo.dat - MESS info database.
		english.lng - Language database.

	These files should reside in current directory for
	DOS and Windows or in the $data or $home directories
	for Linux a Mac OS X.

    misc_cheat
	Enables or disables the cheat system. It may also change the
	game behavior enabling the cheat dip-switch if available.
	If enabled, it disables the hiscore saving.

	:misc_cheat yes | no

	Options:
		yes - Enable the cheats.
		no - Disable the cheats (default).

    misc_cheatfile
	Selects the cheat files. In DOS and Windows use the
	`;' char as file separator. In Linux and Mac OS X use
	the `:' char.

	:misc_cheatfile FILE[;FILE]... (DOS, Windows)
	:misc_cheatfile FILE[:FILE]... (Linux, Mac OS X)

	Options:
		FILE - Cheat file to load (default cheat.dat).

    misc_languagefile
	Selects the language file.

	:misc_languagefile FILE

	Options:
		FILE - Language file to load (default english.lng).

    misc_hiscorefile
	Selects the hiscore file.

	:misc_hiscorefile FILE

	Options:
		FILE - High score file to load (default hiscore.dat).

    misc_safequit
	Activates safe quit mode. If enabled, to stop the
	emulation, you need to confirm on a simple menu.

	:misc_safequit yes | no

	Options:
		no - Disabled.
		yes - Enabled (default).

	If the file `event.dat' (specified with the `misc_eventfile'
	option) is found the exit menu is shown only
	if one or more coins are inserted or if you are playing.

    misc_eventdebug
	Activates the debug mode for the event/safequit feature. On the top
	left of the screen the internal state of the event/safequit engine
	is printed. The first value is the coin state, the second value
	is the playing state. If both the values are 1 the exit is permitted
	without prompting. Other values are the 6 generic events.

	:misc_eventdebug yes | no

	Options:
		no - Disabled (default).
		yes - Enabled.

    misc_eventfile
	Selects the event/safequit database to use.

	:misc_eventfile FILE

	Options:
		FILE - Event file to load (default event.dat).

  Debugging Configuration Options
	The use of these options is discouraged. They are present only
	for testing purpose.

    debug_crash
	Crashes the program. It can be used to ensure that the correct
	video mode is restored on aborting conditions.

	:debug_crash yes | no

	Options:
		no - Don't crash the program (default).
		yes - Add a "Crash" menu entry in the video menu.
			If selected the program crash with a Segmentation
			Fault.

    debug_rawsound
	Disables the sound output syncronization with the video.

	:debug_rawsound yes | no

	Options:
		no - Normal operation (default).
		yes - Sound output without any syncronization.

    debug_speedmark
	Enables or disabled the on screen speed mark. If enabled a red square 
	is displayed if the game is too slow. A red triangle when you press 
	the `turbo' key or when the game is accelerated for other reasons.

	:debug_speedmark yes | no

	Options:
		yes - Display the speed mark when required.
		no - Don't display the speed mark (default).

Signals
	The program intercepts the following signals:

		SIGQUIT - Exit normally.
		SIGTERM, SIGINT, SIGALRM - Exit restoring only the output devices.
		SIGHUP - Restart the program.

Copyright
	This file is Copyright (C) 2003, 2004, 2005 Andrea Mazzoleni, Filipe Estima.

