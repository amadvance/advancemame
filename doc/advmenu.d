Name
	advmenu - The AdvanceMENU Front-end

Synopsis
	:advmenu [-default] [-remove] [-cfg FILE]
	:	[-log] [-version] [-help]

Description
	AdvanceMENU is a front-end to run the AdvanceMAME, AdvanceMESS,
	MAME, MESS, xmame, Raine and other emulators.

	Simply run it in the same directory of the emulator and press
	`f1' to get the help screen or `~' for the main menu.

	Press `tab' to change the display mode. Press `space' to change
	the preview mode.

	To run a game press `enter'. Press `esc' to exit.

	The major features are:

	* Simply download and run. Copy the executable and run it!
	* Auto update of the rom info.
	* Vertical and horizontal orientation.
	* Support for any TV/Arcade Monitor like AdvanceMAME but it's
		good also for a normal PC monitor.
	* Static and Animated image preview (PNG/PCX/ICO/MNG).
		Up to 192 images at the same time!
	* Sound preview. (MP3/WAV). You can select a special sound for
		every game played when the cursor move on it.
	* Sound backgrounds (MP3/WAV). Play your favorite songs or
		radio records in background.
	* Sound effects (MP3/WAV) for key press, program start, game
		start, program exit...
	* Support for zipped images and sounds archives.
	* Screen-saver. A slide show of the game images.

SubSubIndex

Options
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
		Create the file `advmenu.log' with a lot of internal
		information. Very useful for bug report.

	-verbose
		Print some startup information.

	-version
		Print the version number, the low-level device drivers
		supported and the configuration directories.

	-help
		Print a short command line help.

	In Linux and Mac OS X you can also use `--' before options instead of `-'.
	In DOS and Windows you can also use `/'.

Emulators
	The program supports many type of emulators. The emulators
	AdvanceMAME, AdvanceMESS, MAME, xmame, DMAME,
	DMESS and DRAINE are directly supported and the only thing
	you should do is to run the AdvanceMENU program in the same
	directory of the emulator.

	All the other emulators are supported with the emulator
	type `generic'.

  generic - Generic emulator
	For the `generic' emulator no additional rom information is
	needed. Only the name and the size of the rom files are used.

	You should specify all emulator information and directories
	with the `emulator' and `emulator_*' options in the
	`advmenu.rc' file.

	You need to use at least the `emulator' and `emulator_roms'
	options to inform AdvanceMENU how to run the emulator and where
	to find the roms.

	For example:
		:emulator "snes9x" generic "c:\game\snes9x\snes9x.exe" "%f"
		:emulator_roms "snes9x" "c:\game\snes9x\roms"
		:emulator_roms_filter "snes9x" "*.smc;*.sfc;*.fig;*.1"

		:emulator "zsnes" generic "c:\game\zsnes\zsnes.exe" "-e -m roms\%f"
		:emulator_roms "zsnes" "c:\game\zsnes\roms"
		:emulator_roms_filter "zsnes" "*.smc;*.sfc;*.fig;*.1"

	The various %s, %f, %p, ... macros are explained in the `emulator'
	option description.

	The roms are searched in the path specified with the
	`emulator_roms' option in the `advmenu.rc' file. For every file
	found (with any extension) a game is added in the menu.
	You can filter the files with the `emulator_roms_filter' option.

	All the snapshots are searched in the directories specified with
	the `emulator_*' options using the same name of the rom file.

	If you want, you can manually write a MAME like information file
	and name it as `ENUNAME.lst'. This file has the same format
	of the output of the `-listinfo' MAME command.
	Actually only the information `game', `name', `description', `year',
	`manufacturer', `cloneof' are used.
	Please note that this file is used only to add information at the
	existing games. The games present in this file are not automatically
	added at the game list.

  advmame - AdvanceMAME
	For the `advmame' emulator type the roms informations are
	gathered from the file `ENUNAME.xml'. If this file doesn't
	exist, it's created automatically with emulator `-listxml'
	command.

	The directories specified in the `dir_rom' option in the
	`advmame.rc' file are used to detect the list of the
	available roms. In the DOS and Windows versions of the 
	program the `advmenu.rc' file is searched in the same directory 
	of the emulator. In the Unix version it's searched in the
	`HOME/.advance' directory.

	The directory specified in `dir_snap' is used to
	detect the list of available snapshots.

  advmess - AdvanceMESS
	For the `advmess' emulator the rom information is gathered
	from the file `EMUNAME.xml'. If this file doesn't exist,
	it's created automatically with emulator `-listxml' command.

	The directories specified in the `dir_rom' option in the
	`advmess.rc' file are used to detect the list of the
	available bioses.

	All the directories listed in the option `dir_image' are
	read and all the files found in the `machine' directories
	are inserted as software if the extension is recognized as a
	valid device extension for the current `machine' or if it's a
	`zip' file.
	For example if the `dir_image' options is `c:\software',
	AdvanceMENU scans the directories `c:\software\ti99_4a',
	`c:\software\sms', `c:\software\gameboy'...
	Files in the main directory `c:\software' are NOT checked.

	When you select to run a zip file, the zip file is opened and
	all the files in the zip with a valid name and with a recognized
	device extension are added at the AdvanceMESS command line.
	A file is considered to have a valid name if has the same name
	of the zip of if it has the name of the zip with an additional
	char. For example in the file `alpiner.zip' the files
	`alpiner.bin', `alpinerc.bin' and `alpinerg.bin' have a valid
	name. This feature can be used to group all the required roms
	to play a software in a single zip file.

	The file extension is also used to correctly select the device
	type when calling AdvanceMESS.

	The directory specified in `dir_snap' is used to detect the
	list of available snapshots.
	At any exit of AdvanceMESS if a new snapshot is created, this
	file is moved to the correct `snap\system' directory renaming
	it as the software started.
	For example, suppose that you run the `ti99_4a' system with the
	software `alpiner'. If you press F12 during the emulation, the
	file `snap\ti99_4a.png' is created. When you return to
	AdvanceMENU the file is moved automatically to
	`snap\ti99_4a\alpiner.png'.

  mame - Windows MAME
	For the `mame' emulator the roms information is gathered from
	the file `EMUNAME.xml'. If this file doesn't exist, it's created
	automatically with emulator `-listxml' command.

	The directories specified in the `rompath' option in the
	`mame.ini' file are used to detect the list of the available
	roms.

	The directory specified in `snap_directory' is
	used to detect the list of available snapshots.

  xmame - xmame
	For the `xmame' emulator the roms informations are gathered
	from the file `EMUNAME.xml'. If this file doesn't exist, it's
	created automatically with emulator `-listxml' command.

	The directories specified in the `rompath' option in the
	`HOME/.xmame/mamerc' file are used to detect the list of the
	available roms.

	The directory specified in `screenshotdir' is
	used to detect the list of available snapshots files.

  dmame - DOS MAME
	For the `dmame' emulator the roms informations are gathered
	from the file `EMUNAME.xml'. If this file doesn't exist, it's
	created automatically with emulator `-listxml' command.

	The directories specified in the `rompath' option in the
	`mame.cfg' file are used to detect the list of the available
	roms.

	The directory specified in `snap' is used to
	detect the list of available snapshots.

  dmess - DOS MESS
	For the `dmess' emulator the roms informations are gathered
	from the file `EMUNAME.xml'. If this file doesn't exist, it's
	created automatically with emulator `-listxml' command.

	The directories specified in the `biospath' option in the
	`mess.cfg' file are used to detect the list of the available
	bioses.

	All the directories listed in the option `softwarepath' are
	read and all the `zip' files found in the `subsystem'
	directories are inserted as software.
	For example if the `softwarepath' options is `c:\software',
	AdvanceMENU scans the directories `c:\software\ti99_4a',
	`c:\software\sms', `c:\software\gameboy'...
	Zips in the main directory `c:\software' are NOT checked.

	When you select one of these entries the zip is opened and is
	searched the first file with the same name of the zip but
	different extension. This file is used as the argument of the
	`-cart' option when running `mess'.
	AdvanceMENU is NOT able to use other supports like `-flop'.

	All the aliases present if the `mess.cfg' are inserted as
	software entries. When you select one of these entries the
	`mess' option `-alias' is used to start the game.

	You can set an arbitrary description on an alias specification
	adding it on the same line of the alias after the comment char
	'#' using this format:

		:ALIAS = ALIAS_DEF # Description | YEAR | MANUFACTURER

	For example:
		:[ti99_4a]
		:ti-inva = -cart ti-invac.bin -cart ti-invag.bin \
		:	# Invaders | 1982 | Texas Instrument

	At any exit of the emulator if a new snapshot is created, this file
	is moved to the correct `snap\system' directory renaming it as
	the software started.
	For example, suppose that you run the `ti99_4a' system with the
	software `alpiner'. If you press F12 during the emulation, the
	file `snap\ti99_4a.png' is created. When you return to
	AdvanceMENU the file is moved automatically to
	`snap\ti99_4a\alpiner.png'.

  draine - DOS Raine
	For the `draine' emulator the roms informations are gathered
	from the file `EMUNAME.lst'. If this file doesn't exist, it's
	created automatically with emulator `-gameinfo' command.

	All the directories specified in the `rom_dir_*' options are
	used to detect the list of the available roms.

	The directory specified in `screenshots' is used to detect the
	list of available snapshots.

Configuration
	The file `advmenu.rc' is used to save the current state of the
	front-end. It's read at startup and saved at exit. You can
	prevent the automatic save at the exit with the `config' option.

	In DOS and Windows the configuration options are read from the
	file `advmenu.rc' in the current directory.

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

	In DOS and Windows the directory name separator is `\' and the
	multi-directory separator is `;'. In Linux and Mac OS X the directory
	name separator is `/' and the multi-directory separator is `:'.

  Global Configuration Options

    config
	Selects if and when the configuration modified by the user at 
	runtime should be saved.

	:config save_at_exit | restore_at_exit | restore_at_idle

	Options:
		save_at_exit - Save any changes before exiting
			(default).
		restore_at_exit - Don't save the changes. At the next
			run, restore the previous configuration.
		restore_at_idle - Restore the previous configuration
			after the `idle' time.

	You can manually save the configuration at runtime from the
	main menu.

    emulator
	Selects the emulators to list in the menu. You can specify more than 
	one emulator.

	WARNING! Before playing with this option, you should do a
	backup copy of your current `advmenu.rc' because when you remove
	an emulator, the game information for that emulator (like
	the time played) is lost.

	:emulator "EMUNAME" (generic | advmame | advmess | mame | dmame
	:	| dmess | raine) "[-]EXECUTABLE" "ARGUMENTS"

	Options:
		EMUNAME - The name for the emulator. It must be
			different for every emulator defined.
		generic - It's a generic emulator.
		advmame - It's the AdvanceMAME emulator.
		advmess - It's the AdvanceMESS emulator.
		mame - It's the Window MAME emulator.
		dmame - It's the DOS MAME emulator.
		dmess - It's the DOS MESS emulator.
		raine - It's the DOS Raine emulator.
		[-]EXECUTABLE - The executable path of the emulator.
			In DOS and Windows you can also use a batch (.bat)
			file, but this prevent the automatic generation of
			the listing file which must be generated manually.
			You can put a `-' in front of the file path
			to ignore any error returned by the executable.
		ARGUMENTS - The arguments to be passed to the emulator.
			The arguments are required only for the `generic'
			emulator.  For the others, AdvanceMENU automatically
			adds the required arguments to run a
			game. However, you may wish to add extra
			arguments.

	In the emulator arguments some macros are substituted
	with some special values:
		%s - Expanded as the game name. For example "pacman".
		%p - Expanded as the complete path of the rom. For
			example "c:\emu\roms\pacman.zip".
		%f - Expanded as the rom name with the extension. For
			example "pacman.zip".
		%o[R0,R90,R180,R270] - Expanded as one of the R* string, 
			depending on the current menu orientation. 
			Note that you cannot use space in the R* string. 
			For example "%o[,-ror,-flipx,-rol] %o[,,-flipy,]" 
			correctly rotate the AdvanceMAME emulator.

	For the `generic' emulator type you need use the % macros
	to tell at the emulator which game run. For all the other emulator
	types this information is automatically added by AdvanceMENU.

	Examples for DOS and Windows:
		:emulator "AdvanceMAME" advmame "advmame\advmame.exe" \
		:	"%o[,-ror,-flipx,-rol] %o[,,-flipy,]"
		:emulator "MAME" mame "mame\mame.exe" "-nohws"
		:emulator "MESS" dmess "mess\mess.exe" ""
		:emulator "Raine" raine "raine\raine.exe" ""
		:emulator "Custom Raine" raine "raine\raine2.bat" ""
		:emulator "SNes9x" generic "c:\game\snes9x\snes9x.exe" "%f"
		:emulator "ZSNes" generic "c:\game\zsnes\zsnes.exe" "-e -m roms\%f"

	Examples for Linux and Mac OS X:
		:emulator "AdvanceMAME" advmame "advmame" \
		:	"%o[,-ror,-flipx,-rol] %o[,,-flipy,]"

    emulator_roms/roms_filter/altss/flyers/cabinets/icons/titles
	Selects additional directories for the emulators. These
	directories are used in addition to any other directory
	defined in the emulator config file. The preview images and
	sounds files are also searched also in any `.zip' file present
	in these directories.

	:emulator_roms "EMUNAME" "LIST"
	:emulator_roms_filter "EMUNAME" "LIST"
	:emulator_altss "EMUNAME" "LIST"
	:emulator_flyers "EMUNAME" "LIST"
	:emulator_cabinets "EMUNAME" "LIST"
	:emulator_marquees "EMUNAME" "LIST"
	:emulator_icons "EMUNAME" "LIST"
	:emulator_titles "EMUNAME" "LIST"

	Commands:
		roms - List of directories used for the roms. This
			option is used only for the `generic' emulator
			type. All the other emulators use the
			emulator-specific config file to set the rom
			path.
		roms_filter - List of pattern for the file to list.
			An empty pattern means all files.
		altss - Snapshot directory, used for snap images and
			sounds.
		flyers - Flyers directory.
		cabinets - Cabinets directory.
		marquees - Marquees directory.
		icons - Icons directory.
		titles - Titles directory.

	Options:
		EMUNAME - The name for the emulator. Must be the same
			name of a defined emulator
		LIST - List of directories or patterns. In DOS and Windows
			use the `;' char as separator. In Linux and
			Mac OS X use the `:' char.

	Examples for DOS and Windows:
		:emulator_roms "SNes9x" "c:\game\snes9x\roms;c:\game\zsnes\roms2"
		:emulator_roms_filter "SNes9x" "*.smc;*.sfc;*.fig;*.1"
		:emulator_roms "ZSNes" "c:\game\zsnes\roms"
		:emulator_roms_filter "ZSNes" "*.smc;*.sfc;*.fig;*.1"

    mode
	Selects the menu mode.

	:mode full | full_mixed | text | list | list_mixed | tile_small
	:	| tile_normal | tile_big | tile_enormous | tile_giant
	:	| tile_icon | tile_marquee

	Options:
		full - Full screen preview.
		full_mixed - Full screen preview with 4 images.
		text - Game list.
		list - Game list and preview of the selected game (default).
		list_mixed - Game list and 4 preview of the selected game.
		tile_small - Show the preview of 4x3 games.
		tile_normal - Show the preview of 5x6 games.
		tile_big - Show the preview of 8x6 games.
		tile_enormous - Show the preview of 12x9 games.
		tile_giant - Show the preview of 16x12 games.
		tile_icon - Special mode for icon preview.
		tile_marquee - Special mode for marquee preview.

    mode_skip
	Disables some menu modes when you press `tab'.

	:mode_skip (full | full_mixed | list | list_mixed | tile_small
	:	| tile_normal | tile_big | tile_enormous | tile_giant
	:	| tile_icon | tile_marquee)*

	Options:
		SKIP - Multiple selections of disabled modes. Use
			an empty list to enable all the modes.

	Examples:
		:mode_skip tile_giant
		:mode_skip full full_mixed list tile_small tile_giant
		:mode_skip

    sort
	Selects the order of the games displayed.

	:sort parent | name | time | play | year | manufacturer
	:	| type | group | size | resolution | info

	Options:
		parent - Game parent name.
		name - Game name.
		time - Time played.
		play - Play times.
		year - Game year release.
		manufacturer - Game manufacturer.
		type - Game type.
		group - Game group.
		size - Size of the game rom.
		resolution - Resolution of the game display.
		info - Information read with `info_import'.

    preview
	Selects the type of the images displayed.

	:preview snap | titles | flyers | cabinets

	Options:
		snap - Files in the `snap' and `altss' dir.
		flyers - Files in the `flyers' dir.
		cabinets - Files in the `cabinets' dir.
		titles - Files in the `titles' dir.

	The `icons' and `marquees' images can be selected with the
	special `mode' options `tile_icon' and `tile_marquee'.

    preview_expand
	Enlarges the screen area used by the vertical games on horizontal
	tile (and horizontal games in vertical tile).

	:preview_expand FACTOR

	Options:
		FACTOR - Expansion float factor from 1.0 to 3.0
			(default 1.15)

	Examples:
		:preview_expand 1.15

    preview_default_*
	Selects the default images. When an image for the selected game
	is not found, a default image can be displayed.

	:preview_default "FILE"
	:preview_default_snap "FILE"
	:preview_default_flyer "FILE"
	:preview_default_cabinet "FILE"
	:preview_default_icon "FILE"
	:preview_default_marquee "FILE"
	:preview_default_title "FILE"

	Commands:
		default - Selects the default image for all preview
			modes.
		default_TAG - Selects the default image for a single
			preview mode.

	Options:
		FILE - The complete PATH of the image.

	Examples:
		:preview_default "C:\MAME\DEFAULT.PNG"
		:preview_default_marquee "C:\MAME\DEFMAR.PNG"
		:preview_default_icon "C:\MAME\DEFMAR.PNG"

    icon_space
	Selects the space size between icons. The `icon' mode is
	available only if you set the option `icons' in the
	emulator config file.

	:icon_space SPACE

	Options:
		SPACE - The number of pixel between icons (default 43)

    merge
	Selects the expected format of your romset. It's used to test
	the existence of the correct zips needed to run the games.

	:merge none | differential | parent | any | disable

	Options:
		none - Every clone zip contains all the needed roms.
		differential - Every clone zip contains only
			the unique roms (default).
		parent - All the roms are in the parent zip.
		any - Any of the above, use this if you have
			a rom set that is organized poorly.
		disable - Check disabled.

    game
	Contains various information of the know games.
	A `game' option is added automatically at the configuration
	files for any rom found. It's used to keep some game
	information like the play time.

	:game "EMULATOR/GAME" "GROUP" "TYPE" TIME PLAY "DESC"

	Options:
		EMULATOR - Name of the emulator.
		GAME - Short name of the game, generally the
			rom name without the extension.
		GROUP - Name of the group of the game or empty "".
		TYPE - Name of the type of the game or empty "".
		TIME - Time played in seconds.
		PLAY - Number of play.
		DESC - User description or empty "".

	The GROUP, TYPE and DESC argument overwrite any
	other value imported with the `group_import', `type_import',
	and `desc_import' options. The imported values take effect
	only if the user GROUP, TYPE and DESC are empty.

	Examples:
		:game "advmame/puckman" "Very Good" "Arcade" \
		:	1231 21 "Pac-Man Japanese"
		:game "advmame/1943" "" "" 121 4 "1943 !!"

  Display Configuration Options
	This section describes the options used to customize the display.

    device_video_*
	These options are used to customize the video drivers.

	All the `device_video_*' options defined in the `advdev.txt' file can
	be used.

	If you use a `System' video driver, you don't need to set these
	options. They are mostly ignored.

	With a `Generate' video drivers these options are used to select
	and create the correct video mode. If missing the settings for a
	standard Multisync SVGA monitor are used.

    display_size
	Selects the desired width of the video mode.

	:display_size WIDTH

	Options:
		WIDTH - Width in pixels of the video mode. The nearest
			available video mode is chosen (default 1024).

    display_restoreatgame
	Selects whether to reset the video mode before running the
	emulator.

	:display_restoreatgame yes | no

	Options:
		yes - Reset the video mode (default).
		no - Maintain the current graphics mode.

    display_restoreatext
	Selects whether to reset the video mode before exiting.

	:display_restoreatexit yes | no

	Options:
		yes - Reset the video mode (default).
		no - Maintain the current graphics mode.

    display_orientation
	Selects the desired orientation of the screen.

	:display_orientation (flip_xy | mirror_x | mirror_y)*

	Options:
		mirror_x - Mirror in the horizontal direction.
		mirror_y - Mirror in the vertical direction.
		flip_xy - Swap the x and y axes.

	Examples:
		:display_orientation flip_xy mirror_x

    display_brightness
	Selects the image brightness factor.

	:display_brightness FACTOR

	Options:
		FACTOR - Brightness float factor (default 1.0).

	Examples:
		:display_brightness 0.9

    display_gamma
	Selects the image gamma correction factor.

	:display_gamma FACTOR

	Options:
		FACTOR - Gamma float factor (default 1.0).

	Examples:
		:display_gamma 0.9

  Sound Configuration Options
	This section describes the options used to customize the sound.
  
    device_sound_*
	These options are used to customize the audio drivers.
    
	All the `device_sound_*' options defined in the `advdev.txt' file can
	be used.

    sound_volume
	Sets the sound volume.

	:sound_volume VOLUME

	Options:
		VOLUME - The volume is an attenuation in dB. The dB is
			a negative value from -40 to 0.

	Examples:
		:sound_volume -12

    sound_latency
	Sets the audio latency.

	:sound_latency TIME

	Options:
		TIME - Latency in seconds from 0.01 to 2.0.
			(default 0.1)

	Increase the value if your hear a choppy audio.

    sound_buffer
	Sets the size of the lookahead audio buffer for decoding.

	:sound_buffer TIME

	Options:
		TIME - Buffer size in seconds from 0.05 to 2.0.
			(default 0.1)

	Increase the value if your hear a choppy audio.

    sound_foreground_EVENT
	Selects the sounds played in foreground for the various events.

	:sound_foreground_begin none | default | FILE
	:sound_foreground_end none | default | FILE
	:sound_foreground_key none | default | FILE
	:sound_foreground_start none | default | FILE
	:sound_foreground_stop none | default | FILE

	Commands:
		begin - Sound played at AdvanceMENU startup.
		end - Sound played at AdvanceMENU exit.
		start - Sound played at emulator startup.
		stop - Sound played at emulator exit.
		key - Sound played when a key is pressed.

	Options:
		none - No sound.
		default - Use the default sound.
		FILE - Path of the sound file (.wav or .mp3).

    sound_background_EVENT
	Selects the sounds played in background for the various events.

	:sound_background_begin none | FILE
	:sound_background_end none | FILE
	:sound_background_start none | FILE
	:sound_background_stop none | FILE
	:sound_background_loop none | default | FILE

	Commands:
		begin - Sound played at AdvanceMENU startup.
		end - Sound played at AdvanceMENU exit.
		start - Sound played at emulator startup.
		stop - Sound played at emulator exit.
		loop - Sound played in loop if no other background
			sound is available.

	Options:
		none - No sound
		default - Use the default sound
		FILE - Path of the sound file (.wav or .mp3)

    sound_background_loop_dir
	Selects the background music directory to search for MP3 and WAV
	files. Music tracks will be played in random order.

	Multiple directories may be specified by separating each with a
	semicolon `;' in DOS and Windows, with a double-colon `:' in Linux
	and Mac OS X.

	Note that this directory must be used only for your music.
	The emulated game recordings, played when the cursor is moved on
	the game, are stored in the snap directory defined in the emulator
	configuration file or with the `emulator_altss' option.

	:sound_background_loop_dir "DIR"

	Options:
		DIR - Directory for .mp3 and .wav files.

	Examples:
		:sound_background_loop_dir C:\MP3\POP;C:\MP3\ROCK

  Input Configuration Options
	This section describes the options used to customize the user
	input.

    device_keyboard/joystick/mouse
	These options are used to customize the input drivers.

	All the `device_keyboard/joystick/mouse_*' options defined in
	the `advdev.txt' file can be used.

    mouse_delta
	Selects the mouse/trackball sensitivity. Increase the value for
	a slower movement. Decrease it for a faster movement.

	:mouse_delta STEP

	Options:
		STEP - Mouse/trackball position step (default 100).

  User Interface
	This section describes the options used to customize the user 
	interface.

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
		COLS - Number of text columns. If omitted is computed from
			the number of rows.

    ui_background
	Defines a background image in the .PNG format. The image is stretched
	to fit the screen.

	ui_background FILE

	Options:
		FILE - File in .PNG format to load (default none).

    ui_exit
	Defines an exit background image in the .PNG format displayed when
	the emulator exits. The image is stretched to fit the screen.
	The message is displayed only if the option `display_restoreaatexit'
	is set to `no'.

	ui_exit FILE

	Options:
		FILE - File in .PNG format to load (default none).

    ui_gamemsg
	One line message displayed when a game is chosen. The
	message is displayed only if the option `display_restoreatgame' is
	set to `no'.

	:ui_gamemsg "MESSAGE"

	Options:
		MESSAGE - Message to display (default "Run Game").
			To prevent the display of the message use the
			empty string "".

	Examples:
		:ui_gamemsg "Avvio il gioco..."

    ui_game
	Selects the preview type to display when a game is run. The
	message is displayed only if the option `display_restoreatgame' is
	set to `no'.

	:ui_game none | snap | flyers | cabinets | titles | FILE

	Options:
		none - Don't display any preview.
		snap, flyers, cabinets, titles - Display the
			specified preview. (default snap).
		FILE - File in .PNG format to load.


    ui_skiptop/bottom/left/right
	Defines the border area of the screen not used by the menu. Generally
	it's the part of the screen used by the background image.
	If a `ui_background' image is specified these values refer at image
	size before stretching, otherwise they refer at the current video
	mode size.

	ui_skiptop N
	ui_skipbottom N
	ui_skipleft N
	ui_skipright N

	Options:
		N - Number of pixel to skip (default 0).

    ui_topbar/bottombar
	Enables or disables the top and bottom information bars.

	ui_topbar yes | no
	ui_bottombar yes | no

    ui_color
	Selects the user interface colors.

	:ui_color TAG FOREGROUND BACKGROUND

	Tags:
		help - Help.
		help_tag - Help highlight.
		submenu_bar - Submenu title.
		submenu_item - Submenu entry.
		submenu_item_select - Submenu selected entry.
		menu_item - Menu entry.
		menu_hidden - Menu hidden entry.
		menu_tag - Menu highlight entry.
		menu_item_select - Menu selected entry.
		menu_hidden_select - Menu hidden selected entry.
		menu_tag_select - Menu selected highlight.
		bar - Title.
		bar_tag - Title highlight.
		bar_hidden - Title hidden text.
		grid - Scrollbar marker and grid.
		backdrop - Backdrop outline and missing backdrop.
		icon - Icon outline and missing icon.
		cursor - Flashing cursor.

	Options:
		FOREGROUND - Foreground color. One of the following:
			black, blue, green, cyan, red, magenta, brown,
			lightgray, gray, lightblue, lightgreen
			lightcyan, lightred, lightmagenta, yellow,
			white or RRGGBB in hex format. For example
			FF0000 is red and 00FF00 is green.
		BACKGROUND - Background color. Like foreground color.

    ui_clip
	Selects how play the video clips.

	:ui_clip none | single | singleloop | multi | multiloop | multiloopall

	Options:
		none - No clip.
		single - Play only one clip and only one time (default).
		singleloop - Play only one clip continuously. The sound is
			not looped.
		multi - Play all the clips.
		multiloop - Play all the clips, and loop the clip on the
			cursor. The sound is not looped.
		multiloopall - Play all the clips, and loop all the clips.
			The sound is not looped.
			
    ui_command
	Defines the user commands. These commands are executed as 
	shell scripts. The video mode is not changed, so they must be 
	silent.

	ui_command "MENU" SCRIPT

	Options:
		MENU - Name of the menu entry.
		SCRIPT - Commands to execute. If you need to insert more
			command rows you can end the line with the \ char.

	In the script text some macro are substituted with information of
	the selected game:
		%s -  The game name. For example "pacman".
		%p - The complete path of the rom. For
			example "c:\emu\roms\pacman.zip".
		%f - The rom name with the extension. For
			example "pacman.zip".

	If no game is selected the macros aren't substituted.
	
	If the script exits with an error code, a message is displayed.

	Examples:
		:ui_command "Delete Hiscore" \
		:	rm ~/.advance/hi/%s.hi
		:ui_command "Enable GamePad" \
		:	rmmod analog \
		:	sleep 1 \
		:	modprobe analog js=gamepad

    ui_command_menu
	Selects the name of the menu entry for the commands submenu.

	ui_command_menu MENU

	Options:
		MENU - Name of the menu entry (default "Command").

    ui_command_error
	Selects the message to display if a command fails.

	ui_command_error MSG

	Options:
		MSG - Message to display (default "Error running the command").

    ui_console
	Changes the user interface behavior for the use on a game 
	Console system. Mainly used in AdvanceCD.

	ui_console yes | no

  Other Configuration Options

    idle_start
	Automatically starts a random game after some time of inactivity.
	You can also configure the AdvanceMAME option `input_idleexit'
	in the file `advmame.rc' to create a continuous demo mode.

	:idle_start START_TIMEOUT REPEAT_TIMEOUT

	Options:
		START_TIMEOUT - Number of seconds to wait for the
			first run. 0 means do nothing (default).
		REPEAT_TIMEOUT - Number of seconds to wait for the
			next run. 0 means do nothing (default).

	Examples:
		:idle_start 400 60

    idle_screensaver
	Selects the start time of the default screen saver. The screensaver
	is a slide show of the available snapshots.

	:idle_screensaver START_TIMEOUT REPEAT_TIMEOUT

	Options:
		START_TIMEOUT - Number of seconds to wait for the
			first run. 0 means never (default).
		REPEAT_TIMEOUT - Number of seconds to wait for the
			next run. 0 means never (default).

	Examples:
		:idle_screensaver 40 5

    idle_screensaver_preview
	Selects the preview type to use in the screensaver. Like 
	the preview option.

	:idle_screensaver_preview none | play | snap | flyers
	:	| cabinets | titles

	Options:
		none - Shutdown the monitor using the VESA/PM services
			if available. Otherwise use a black image.
		snap, flyers, cabinets, titles - Start a mute slide show
			of the specified image type (default snap).
		play - Start a snap slide show using the animated
			snapshots and the game sounds. The static
			snapshots are ignored.

    group/type
	Selects the available `group' and `type' category names and
	which of them to show.

	:group "STRING"
	:type "STRING"
	:group_include "STRING"
	:type_include "STRING"

	Commands:
		group, type - Define a category.
		group_include, type_include - Show a category.

	Options:
		STRING - name of the category

    group/type/desc/info_import
	Selects the automatic import of the groups, types, descriptions
	and extra information from an external file. The extra info are
	additional information displayed for every game.

	The file formats supported are CATINI, MacMAME and NMS.
	The files are read in the current directory in DOS and Windows
	and in the $home directory in Linux and Mac OS X.

	WARNING! These option DON'T OVERWRITE any user explicit
	choices made with the `game' option.

	:group_import (ini | mac | nms) "EMULATOR" "FILE" ["SECTION"]
	:type_import (ini | mac | nms) "EMULATOR" "FILE" ["SECTION"]
	:desc_import (ini | mac | nms) "EMULATOR" "FILE" ["SECTION"]
	:info_import (ini | mac | nms) "EMULATOR" "FILE" ["SECTION"]

	Options:
		none - Don't import.
		ini - Import in CATLIST format.
		mac - Import in the MacMAME format.
		nms - Import in the NMS format.
		EMULATOR - The emulator tag name as specified in
			the `emulator' option.
		FILE - The file name.
		SECTION - The section name (only for the `ini' format).

	Examples:
		:group_import ini "advmame" "catver.ini" "Category"
		:type_import mac "advmame" "Genre 37b14.txt"
		:desc_import nms "raine" "raine.nms"
		:info_import ini "advmame" "catver.ini" "VerAdded"

	The CATLIST files can be downloaded at:

		+http://www.mameworld.net/catlist/

	The MacMAME files can downloaded at:

		+http://www.tznet.com/cmader/categories.html

    lock
	Locks or unlocks the user interface. When locked, the user can
	only browse and run games. Options can't be changed and the user
	cannot exit.

	:lock yes | no
	
	Options:
		yes - Locked mode activate.
		no - Locked mode deactivate (default).

    event_assign
	Customizes the input keyboard codes that trigger menu
	events.

	:event_assign EVENT EXPRESSION

	Events:
		up, down, left, right - Movement.
		home, end, pgup, pgdn - Movement.
		mode - Change the display mode.
		help - Show a little help.
		group - Select a game group.
		type - Select a game type.
		exclude - Exclude some games.
		sort - Sort games.
		setgroup - Select the group of the current game
		settype - Select the type of the current game
		runclone - Run a game clone.
		enter - Main action, start.
		esc - Back action, exit & cancel.
		shutdown - Exit and shutdown.
		space - Change action, select & deselect.
		ins - Select all.
		del - Deselect all.
		command - The file command menu.
		menu - The main menu.
		emulator - The emulator menu.
		rotate - Rotate the screen of 90ø.
		lock - Lock/unlock the user interface.

	Options:
		EXPRESSION - Definition of the key expression that
			generates the event. It's a combination of
			the key names or scan-code, and of the operators
			`not', `or'. The `and' operator is implicit
			between consecutive scan-codes.
		KEY -  The available key names are:
			a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r,
			s, t, u, v, w, x, y, z, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
			0_pad, 1_pad, 2_pad, 3_pad, 4_pad, 5_pad, 6_pad,
			7_pad, 8_pad, 9_pad, f1, f2, f3, f4, f5, f6, f7, f8,
			f9, f10, f11, f12, esc, backquote, minus, equals,
			backspace, tab, openbrace, closebrace, enter,
			semicolon, quote, backslash, less, comma, period,
			slash, space, insert, del, home, end, pgup, pgdn, left,
			right, up, down, slash_pad, asterisk_pad, minus_pad,
			plus_pad, period_pad, enter_pad, prtscr, pause,
			lshift, rshift, lcontrol, rcontrol, lalt, ralt,
			lwin, rwin, menu, scrlock, numlock, capslock.

	Examples:
		:event_assign enter lcontrol or enter
		:event_assign menu 90 or 35
		:event_assign emulator 91 23 or not 21 33

	You can use the utility `advk' to get the key scancodes and
	names.

    event_repeat
	Selects the repeat rate of the various events.

	:event_repeat FIRST_TIME NEXT_TIME

	Options:
		FIRST_TIME - Time of the first repeat in ms.
		NEXT_TIME - Time of the next repeats in ms.

    event_mode
	Selects whether to wait for a complete screen update before
	processing the next event.

	:event_mode wait | fast

	Options:
		wait - The screen is completely redrawn before processing
			the next event.
		fast - If an event is waiting, the screen drawing
			is interrupted (default).

    event_alpha
	Disables the alphanumeric keys for fast moving.
	If you have a keyboard encoder or a keyboard hack with some 
	buttons remapped to alphanumeric keys, it's useful to disable 
	them.

	:event_alpha yes | no

	Options:
		yes - Enable (default).
		no - Disable.

    misc_exit
	Selects the exit mode.

	:misc_exit none | normal | shutdown | all

	Options:
		none - Exit is disabled.
		normal - Exit is possible pressing ESC.
		shutdown - Exit is possible pressing CTRL-ESC.
		all - All the exit modes are possible.

    misc_quiet
	Disables the copyright text message at the startup.

	:misc_quiet yes | no

Formats Supported
	This is the list of the file formats supported by AdvanceMENU.

	Images:
		PNG - The PNG format.
		PCX - The PCX format.
		ICO - The ICON format.

	Clips:
		MNG - The MNG-VLC (Very Low Complexity) sub format
			without transparency and alpha channel.
			Or the sub-formats generated by AdvanceMAME or
			by the `advmng' compression utility.

	Sounds:
		MP3 - The MP3 format.
		WAV - The WAV format with a sample size of 16 bit.

	Fonts:
		TTF - The TrueType format (with the FreeType2 library).
		RAW - The RAW format.
		PSF - The PSF format.
		GRX - The GRX format.

	Archives:
		ZIP - The ZIP format.

Signals
	The program intercepts the following signals:

		SIGQUIT - Exit normally.
		SIGTERM, SIGINT, SIGALRM - Exit restoring only the output devices.
		SIGHUP - Restart the program.

Copyright
	This file is Copyright (C) 2003, 2004 Andrea Mazzoleni, Randy Schnedler.

