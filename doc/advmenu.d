Name
	advmenu - The AdvanceMENU Frontend

Synopsys
	:advmenu [-default] [-remove] [-log] [-logsync]

Description
	The AdvanceMENU utility is a frontend to run the AdvanceMAME,
	AdvanceMESS, AdvancePAC, MAME, MESS, RAINE and other emulators.

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
		>Up to 192 images</strong> at the same time!
	* Sound preview. (MP3/WAV). You can select a special sound for
		every game played when the cursor move on it.
	* Sound backgrounds (MP3/WAV). Play your favourite songs or
		radio records in background.
	* Sound effects (MP3/WAV) for key press, program start, game
		start, program exit...
	* Support for zipped images and sounds archives.
	* Screensaver. A slide show of the game images.

Options
	-default
		Add to the configuration file all the missing options
		with default values.

	-remove
		Remove from the configuration file all the options
		with default values.

	-log
		Create the file `advmenu.log' with a lot of internal
		information. Very useful for bug report.

	-logsync
		Like the -log option but the file `advmenu.log' is
		updated at every write. Useful for creating the log when
		the program crash.

	-verbose
		Print some startup information.

Emulators
	The program supports many type of emulators. The emulators
	AdvanceMAME, AdvancePAC, AdvanceMESS, MAME, Xmame, DMAME,
	DMESS and DRAINE are directly supported  and the only thing
	you should do is to run the AdvanceMENU program in the same
	directory of the emulator.

	All the other emulators are supported with the emulator
	type `generic'.

  generic
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

	The various %f, %p, ... macros are explained in the `emulator'
	option description.

	The roms are searched in the path specified with the
	`emulator_roms' option in the `advmenu.rc' file. For every file
	found (with any extension) a game is added in the menu.
	You can filter the files with the `emulator_roms_filter' option.

	All the snapshots are searched in the directories specified with
	the `emulator_*' options using the same name of the rom file.

  advmame (The AdvanceMAME emulator)
	For the `advmame' emulator type the roms informations are
	gathered from the file `ENUNAME.lst'. If this file doesn't
	exist, it's created automatically with emulator `-listinfo'
	command.

	The directories specified in the `dir_rom' option in the
	`advmame.rc' file are used to detect the list of the
	available roms. In the DOS version of the program the
	`advmenu.rc' file is searched in the same directory of the
	emulator. In the Unix version it's searched in the
	`HOME/.advance' directory.

	The directories specified in `dir_snap', `dir_cfg' are used to
	detect the list of available snapshots and .cfg files.

  advpac (The AdvancePAC emulator)
	Exactly like the `advmame' emulator type with the exception
	of the use of the `advpac.rc' configuration file instead of
	`advmame.rc'.

  advmess (The AdvanceMESS emulator)
	For the `advmess' emulator the rom information is gathered
	from the file `EMUNAME.lst'. If this file doesn't exist,
	it's created automatically with emulator `-listinfo' command.

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

  mame (The Windows version of the MAME emulator)
	For the `mame' emulator the roms information is gathered from
	the file `EMUNAME.lst'. If this file doesn't exist, it's created
	automatically with emulator `-listinfo' command.

	The directories specified in the `rompath' option in the
	`mame.ini' file are used to detect the list of the available
	roms.

	The directories specified in `snap_directory', `cfg_directory' are
	used to detect the list of available snapshots and MAME .cfg files.

  xmame (The Unix version of the MAME emulator)
	For the `xmame' emulator the roms informations are gathered
	from the file `EMUNAME.lst'. If this file doesn't exist, it's
	created automatically with emulator `-listinfo' command.

	The directories specified in the `rompath' option in the
	`HOME/.xmame/mamerc' file are used to detect the list of the
	available roms.

	The directories specified in `screenshotdir' are
	used to detect the list of available snapshots files.
	The MAME cfg files are assumed to be in the `HOME/.xmame/cfg'
	directory.

  dmame (The DOS version of the MAME emulator)
	For the `dmame' emulator the roms informations are gathered
	from the file `EMUNAME.lst'. If this file doesn't exist, it's
	created automatically with emulator `-listinfo' command.

	The directories specified in the `rompath' option in the
	`mame.cfg' file are used to detect the list of the available
	roms.

	The directories specified in `snap', `cfg' are used to
	detect the list of available snapshots and MAME .cfg files.

  dmess (The DOS version of the MESS emulator)
	For the `dmess' emulator the roms informations are gathered
	from the file `EMUNAME.lst'. If this file doesn't exist, it's
	created automatically with emulator `-listinfo' command.

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
	'#' using this format :

		:ALIAS = ALIAS_DEF # Description | YEAR | MANUFACTURER

	For example:
		:[ti99_4a]
		:ti-inva = -cart ti-invac.bin -cart ti-invag.bin # Invaders | 1982 | Texas Instrument

	At any exit of the emulator if a new snapshot is created, this file
	is moved to the correct `snap\system' directory renaming it as
	the software started.
	For example, suppose that you run the `ti99_4a' system with the
	software `alpiner'. If you press F12 during the emulation, the
	file `snap\ti99_4a.png' is created. When you return to
	AdvanceMENU the file is moved automatically to
	`snap\ti99_4a\alpiner.png'.

  draine (The DOS version of the Raine emulator)
	For the `draine' emulator the roms informations are gathered
	from the file `EMUNAME.lst'. If this file doesn't exist, it's
	created automatically with emulator `-gameinfo' command.

	All the directories specified in the `rom_dir_*' options are
	used to detect the list of the available roms.

	The directory specified in `screenshots' is used to detect the
	list of available snapshots.

Configuration
	The file `advmenu.rc' is used to save the current state of the
	frontend. It's read at startup and saved at exit. You can
	prevent the automatic save at the exit with the `config' option.

	The Linux version reads configuration options from the file
	'advmenu.rc' in the $root and the $home directory.
	The $root directory is PREFIX/share/advance/ where
	PREFIX is the installation directory configured in the
	`makefile', generally it's `/usr/local'.
	The $home directory is HOME/.advance/ where HOME is the
	value of the HOME environment variable.
	If the HOME environment variable is missing the $root
	directory became the $home directory.

	The options in the $root directory overwrite the options in
	the $home.

	The $home directory is also used to write all the information
	by the program. The files in the $root directory are only read.

	You can force the creation of a default `advmenu.rc' with the
	command `advmenu -default'.

	In DOS the directory name separator is '\' and the
	multidirectory separator is ';'. In Linux the directory name
	separator is '/' and the multidirectory separator is ':'.

  Global Configuration Options
    config
	This option selects if and when the configuration modified by
	the user at runtime should be saved.

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
	This option can be used to select which emulators to use in
	the frontend. You can specify a multiple emulator support.

	WARNING! Before playing with this option, you should do a
	backup copy of your current `advmenu.rc' because when you remove
	an emulator, all the game information for that emulator (like
	the time played) is lost.

	:emulator "EMUNAME" (generic | advmame | advmess | mame | dmame | dmess | raine) "EXECUTABLE" "ARGUMENTS"

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
		EXECUTABLE - The executable path of the emulator.
			You can also use a batch file.
			If you specify the complete path of the
			executable the program is able to generate
			the listing file.
		ARGUMENTS - The arguments to be passed to the emulator.
			The arguments are needed only for the `generic'
			emulator.  For the others, AdvanceMENU is
			programmed with the required arguments to run a
			game. However, you may wish to add extra
			arguments.

	In the emulator option some strings are substitutes
	with some special values:
		%s -  The game name. For example "pacman".
		%p - The complete path of the rom. For
			example "c:\emu\roms\pacman.zip".
		%f - The rom name with the extension. For
			example "pacman.zip".
		%o[R0,R90,R180,R270] - One of the R* string, depending
			on the current menu orientation. You cannot use
			space in the R* string. For example
			"%o[,-ror,-flipx,-rol] %o[,,-flipy,]" correctly
			rotate the AdvanceMAMEMAME emulator.

	For the `generic' emulator type you need use the various % options
	to tell at the emulator which game run. For all the other emulator
	types this information is automatically added by AdvanceMENU.

	Examples for DOS:
		:emulator "advmame" advmame "advmame\advmame.exe" "%o[,-ror,-flipx,-rol] %o[,,-flipy,]"
		:emulator "mame" mame "mame\mame.exe" "-nohws"
		:emulator "neomame" mame "neomame\neomame.exe" ""
		:emulator "cpsmame" mame "cpsmame\cpsmame.exe" ""
		:emulator "mess" dmess "mess\mess.exe" ""
		:emulator "raine" raine "raine\raine.exe" ""
		:emulator "myraine" raine "raine\raine2.bat" ""
		:emulator "snes9x" generic "c:\game\snes9x\snes9x.exe" "%f"
		:emulator "zsnes" generic "c:\game\zsnes\zsnes.exe" "-e -m roms\%f"

	Examples for Linux:
		:emulator "advmame" advmame "advmame" "%o[,-ror,-flipx,-rol] %o[,,-flipy,]"

    emulator_TAG
	Select additional directories for the emulators. These
	directories are used in addition to any other directory
	defined in the emulator config file. The preview images and
	sounds files are also searched also in any `.zip' file present
	in these directories.

	:emulator_roms "Name" "LIST"
	:emulator_roms_filter "Name" "LIST"
	:emulator_altss "Name" "LIST"
	:emulator_flyers "Name" "LIST"
	:emulator_cabinets "Name" "LIST"
	:emulator_marquees "Name" "LIST"
	:emulator_icons "Name" "LIST"
	:emulator_titles "Name" "LIST"

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
		Name - The name for the emulator. Must be the same
			name of a defined emulator
		LIST - List of directories or patterns separated by `;'

	Examples:
		:emulator_roms "snes9x" "c:\game\snes9x\roms"
		:emulator_roms_filter "snes9x" "*.smc;*.sfc;*.fig;*.1"
		:emulator_roms "zsnes" "c:\game\zsnes\roms"
		:emulator_roms_filter "zsnes" "*.smc;*.sfc;*.fig;*.1"

    mode
	Select the menu mode shown.

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
	Select the undesidered modes to skip when you press `tab'.

	:mode_skip (full | full_mixed | list | list_mixed | tile_small
	:	| tile_normal | tile_big | tile_enormous | tile_giant
	:	| tile_icon | tile_marquee)*

	Options:
		SKIP - You can enter multiple selections or an empty list.

	Examples:
		:mode_skip tile_giant
		:mode_skip full full_mixed list tile_small tile_giant
		:mode_skip

    sort
	Select the order of the games displayed.

	:sort parent | name | time | coin | year | manufacturer
	:	| type | group | size | resolution | info

	Options:
		parent - Game parent name.
		name - Game name.
		time - Time played.
		coin - Coins used.
		year - Game year release.
		manufacturer - Game manufacturer.
		type - Game type.
		group - Game group.
		size - Size of the game rom.
		resolution - Resolution of the game display.
		info - Information read with `info_import'.

    preview
	Select the type of the images diplayed.

	:preview snap | titles | flyers | cabinets

	Options:
		snap - Files in the `snap' and `altss' dir.
		flyers - Files in the `flyers' dir.
		cabinets - Files in the `cabinets' dir.
		titles - Files in the `titles' dir.

	The `icons' and `marquees' images can be selected with the
	special `mode' options `tile_icon' and `tile_marquee'.

    preview_expand
	Enlarge the screen area used by the vertical games on horizontal
	tile (and horizontal games in vertical tile).

	:preview_expand FACTOR

	Options:
		FACTOR - Expansion float factor from 1.0 to 3.0
			(default 1.15)

	Examples:
		:preview_expand 1.15

    preview_default_*
	Select the default images. When an image for the selected game
	is not found, a default image can be displayed.

	:preview_default "IMAGE_PATH"
	:preview_default_snap "IMAGE_PATH"
	:preview_default_flyer "IMAGE_PATH"
	:preview_default_cabinet "IMAGE_PATH"
	:preview_default_icon "IMAGE_PATH"
	:preview_default_marquee "IMAGE_PATH"
	:preview_default_title "IMAGE_PATH"

	Commands:
		default - Select the default image for all preview
			modes.
		default_TAG - Select the default image for a single
			preview mode.

	Options:
		IMAGE_PATH - The complete PATH of the image.

	Examples:
		:preview_default "C:\MAME\DEFAULT.PNG"
		:preview_default_marquee "C:\MAME\DEFMAR.PNG"
		:preview_default_icon "C:\MAME\DEFMAR.PNG"

    icon_space
	Control the space between the icons. The `icon' mode is
	available only if you set the option `icons' in the
	emulator config file.

	:icon_space SPACE

	Options:
		SPACE - The number of pixel between icons (default 43)

    merge
	Select the expected format of your romset. It's used to test
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
	imformation like the time and coins used.

	:game "EMULATOR/GAME" "GROUP" "TYPE" TIME COIN "DESC"

	Options:
		EMULATOR - Name of the emulator.
		GAME - Short name of the game, generally the
			rom name without the extension.
		GROUP - Name of the group of the game or empty "".
		TYPE - Name of the type of the game or empty "".
		TIME - Time played in seconds.
		COIN - Coins used.
		DESC - User description or empty "".

	The GROUP, TYPE and Description argument overwrite any
	other value import with the `group_import', `type_import',
	and `desc_import' options. The imported values take effect
	only if the user GROUP, TYPE and DESC are empty.

	Examples:
		:game "advmame/puckman" "Very Good" "Arcade" 1231 21 "Pac-Man Japanese"
		:game "advmame/1943" "" "" 121 4 "1943 !!"

  Video Configuration Options
    device_video_*
	All the `device_video_*' options defined in the `advv.txt' file can
	be used.
   
	If you are using the `sdl' video driver you don't need to set these 
	options. 

	With the other video drivers these options are used to select 
	and create the correct video mode. 
	If missing the settings for a standard Multisync SVGA monitor are used.

    video_size
	Select the desired size of the video mode.

	:video_size X_SIZE

	Options:
		X_SIZE - Width in pixels of the video mode. The nearest
			available video mode is chosen (default 1024).

    video_depth
	Select the desired bit depth of the video mode.

	:video_depth 8 | 15 | 16 | 32

    video_restore
	Select whether to reset in the text mode before running the
	emulator.

	:video_restore yes | no

	Options:
		yes - Switch to text mode (default).
		no - Maintain the current graphics mode.

    video_font
	Select a font file. The formats GRX, PSF and RAW are supported.
	You can find a collection of fonts in the `contrib' directory.

	:video_font none | "FONT_FILE"

	Options:
		none - Use the built-in font (default).
		FONT_FILE - Font file path.

    video_orientation
	Select the desired orientation of the screen.

	:video_orientation (flip_xy | mirror_x | mirror_y)*

	Options:
		mirror_x - Mirror in the horizontal direction.
		mirror_y - Mirror in the vertical direction.
		flip_xy - Swap the x and y axes.

	Examples:
		:video_orientation flip_xy mirror_x

    video_brightness
	Select the image brightness factor.

	:video_brightness FACTOR

	Options:
		FACTOR - Brightness float factor (default 1.0).

	Examples:
		:video_brightness 0.9

    video_gamma
	Select the image gamma correction factor.

	:video_gamma FACTOR

	Options:
		FACTOR - Gamma float factor (default 1.0).

	Examples:
		:video_gamma 0.9

  Sound Configuration Options
    device_sound
	Select the sound card model.

	:device_sound auto | none | DEVICE

	Options:
		none - No sound.
		auto - Automatic detection (default).

	Options for the DOS version:
		seal - SEAL automatic detection.
		seal/sb - Sound Blaster.
		seal/pas - Pro Audio Spectrum.
		seal/gusmax - Gravis Ultrasound Max.
		seal/gus - Gravis Ultrasound.
		seal/wss - Windows Sound System.
		seal/ess - Ensoniq Soundscape.
		allegro - Allegro automatic detection.
		allegro/sb10 - Sound Blaster 1.0.
		allegro/sb15 - Sound Blaster 1.5.
		allegro/sb20 - Sound Blaster 2.0.
		allegro/sbpro - Sound Blaster Pro.
		allegro/sb16 - Sound Blaster 16.
		allegro/audio - Ensoniq AudioDrive.
		allegro/wss - Windows Sound System.
		allegro/ess - Ensoniq Soundscape.
		vsync/sb -  Sound Blaster.
		vsync/sbwin - Sound Blaster (Windows).
		vsync/ac97 - AC97.
		vsync/ac97win - AC97 (Windows).
		vsync/gusmax - Gravis Ultrasound Max.
		vsync/gus - Gravis Ultrasound.
		vsync/audio - Ensoniq AudioDrive.
		vsync/wss - Windows Sound System.
		vsync/ess- Ensoniq Soundscape.

	The vsync/ drivers came from th VSyncMAME emulator. More info
	are in the VSyncMAME page :

		http://vsynchmame.mameworld.net

	Options for the Linux version:
		oss - OSS automatic detection.

    sound_volume
	Sets the startup volume.

	:sound_volume VOLUME

	Options:
		VOLUME - The volume is an attenuation in dB. The dB is
			a negative value from -20 to 0.

	Examples:
		:sound_volume -12

    sound_latency
	Sets the audio latency.

	:sound_latency TIME

	Options:
		TIME - Latency in seconds from 0.01 to 2.0.
			(default 0.1)

    sound_buffer
	Sets the size of the lookahead audio buffer for decoding.

	:sound_buffer TIME

	Options:
		TIME - Buffer size in seconds from 0.05 to 2.0.
			(default 0.1)

    sound_foreground_EVENT
	Select the sounds played in foreground for the various events.

	:sound_foregroun_begin none | default | FILE
	:sound_foregroun_end none | default | FILE
	:sound_foregroun_key none | default | FILE
	:sound_foregroun_start none | default | FILE
	:sound_foregroun_stop none | default | FILE

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
	Select the sounds played in background for the various events.

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
		loop - Sound played in loop if no other background.
			sound is available.

	Options:
		none - No sound
		default - Use the default sound
		FILE - Path of the sound file (.wav or .mp3)

    sound_background_loop_dir
	Select the background music directory to search for MP3 and WAV
	files.

	Music tracks will be played in random order.

	Multiple directories may be specified by separating each with a
	semicolon `;' in DOS and with a doublecolon `:' in Linux.

	Note that this directory must be used only for your music.
	The emulated game recordings, played when the cursor is moved on
	the game, are stored in the snap directory defined in your
	`mame.rc' or with the `emulator_altss' option.

	:sound_background_loop_dir "DIR"

	Options:
		DIR - Directory for .mp3 and .wav files.

	Examples:
		:sound_background_loop_dir C:\MP3\POP;C:\MP3\ROCK

  Input Configuration Options
    device_joystick
	Enables or disables joystick support.

	:device_joystick auto | none | DEVICE

	Options:
		none - No joystick (default).
		auto - Automatic detection.

	The DOS version uses the Allegro library for joystick support.
	The Linux version uses always the Kernel joystick interface.

	Options for DOS Allegro library:
		allegro/standard - Standard joystick.
		allegro/dual - Dual joysticks.
		allegro/4button - 4-button joystick.
		allegro/6button - 6-button joystick.
		allegro/8button - 8-button joystick.
		allegro/fspro - CH Flightstick Pro.
		allegro/wingex - Logitech Wingman Extreme.
		allegro/sidewinder - Sidewinder.
		allegro/sidewinderag - Sidewinder Aggressive.
		allegro/gamepadpro - GamePad Pro.
		allegro/grip - GrIP.
		allegro/grip4 - GrIP 4-way.
		allegro/sneslpt1 - SNESpad LPT1.
		allegro/sneslpt2 - SNESpad LPT2.
		allegro/sneslpt3 - SNESpad LPT3.
		allegro/psxlpt1 - PSXpad LPT1.
		allegro/psxlpt2 - PSXpad LPT2.
		allegro/psxlpt3 - PSXpad LPT3.
		allegro/n64lpt1 - N64pad LPT1.
		allegro/n64lpt2 - N64pad LPT2.
		allegro/n64lpt3 - N64pad LPT3.
		allegro/db9lpt1 - DB9 LPT1.
		allegro/db9lpt2 - DB9 LPT2.
		allegro/db9lpt3 - DB9 LPT3.
		allegro/tgxlpt1 - TGX-LPT1.
		allegro/tgxlpt2 - TGX LPT2.
		allegro/tgxlpt3 - TGX LPT3.
		allegro/segaisa - IF-SEGA/ISA.
		allegro/segapci - IF-SEGA2/PCI.
		allegro/segapcifast - IF-SEGA2/PCI (normal).
		allegro/wingwarrior - Wingman Warrior.

    device_mouse
	Enables or disables mouse support.

	:device_mouse auto | none

	Options:
		none - No mouse (default).
		auto - Automatic detection.

	The DOS version uses standard DOS service for the first mouse 
	and the special `optimous' driver for a second mouse.
	The `optimous' driver is available in the `contrib/' directory.
	The Linux version uses `svgalib' mouse support. So you must 
	configure the correct mouse in `svgalib' configuration file.

    mouse_delta
	Select the mouse/trackball sensitivity. Increase the value for
	slower movement. Decrease it for a faster movement.

	:mouse_delta STEP

	Options:
		STEP - Mouse/trackball position step (default 100).

  Other Configuration Options
    idle_start
	Automatically start a random game after some time of inactivity.
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
	Start the default screen saver. Actually it is a slide show of
	the available snapshots.

	:idle_screensaver START_TIMEOUT REPEAT_TIMEOUT

	Options:
		START_TIMEOUT - Number of seconds to wait for the
			first run. 0 means do nothing (default).
		REPEAT_TIMEOUT - Number of seconds to wait for the
			next run. 0 means do nothing (default).

	Examples:
		:idle_screensaver 40 5

    idle_screensaver_preview
	Select the preview type to use. Like the preview option.

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
	Select the available `group' and `type' category names and
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
	Select the automatic import of the groups, types, descriptions
	and extra info from an external file. The extra info are
	additional information displayed for every game.

	The file formats supported are CATINI, MacMAME and NMS.
	The files are read in the current directory for the DOS version
	and in the $home directory for the Linux version.

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

	The CATLIST files can be downloaded at :
		http://www.mameworld.net/catlist/

	The MacMAME files can downloaded at:
		http://www.tznet.com/cmader/categories.html

    lock
	Lock or Unlock the user interface. When locked, the user can
	only browse and run games. Options can't be changed and the user
	cannot exit.

	:lock yes | no

    event_assign
	Used to customize the input keyboard codes that trigger menu
	events.

	:event EVENT EXPRESSION

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
			the key names or scancode, and of the operators
			`not', `or'. The `and' operator is implicit
			between consecutive scancodes.
		KEY -  The available key names are:
			a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r,
			s, t, u, v, w, x, y, z, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
			0_pad, 1_pad, 2_pad, 3_pad, 4_pad, 5_pad, 6_pad,
			7_pad, 8_pad, 9_pad, f1, f2, f3, f4, f5, f6, f7, f8,
			f9, f10, f11, f12, esc, backquote, minus, equals,
			backspace, tab, openbrace, closebrace, enter,
			semicolon, quote, backslash, less, comma, period,
			slash, space, insert, del, home, end, pgup, pgdn, left,
			right, up, down, slash, asterisk, minus_pad,
			plus_pad, period_pad, enter_pad, prtscr, pause,
			lshift, rshift, lcontrol, rcontrol, lalt, ralt,
			lwin, rwin, menu, scrlock, numlock, capslock.

	Examples:
		:event_assign menu 90 or 35
		:	-> 90 or 35
		:event_assign emulator 91 23 or not 21 33
		:	-> (91 and 23) or ((not 21) and 33)

	You can use the utility `advk' to get the key scancodes and
	names.

    event_repeat
	Select the repeat rate of the various events.

	:event_repeat FIRST_TIME NEXT_TIME

	Options:
		FIRST_TIME - Time of the first repeat in ms.
		NEXT_TIME - Time of the next repeats in ms.

    event_mode
	Select whether to wait for a complete screen update before
	processing the next event.

	:event_mode wait | fast

	Options:
		wait - The screen is redrawn before processing
			the next event.
		fast - If an event is waiting, the screen drawing
			is interrupted (default).

    event_alpha
	Used to disable the use of alphanumeric keys for fast moving.
	It's useful disable it if you have a keyboard encoder or a
	keyboard hack with some buttons remapped to alphanumeric keys.

	:event_alpha yes | no

	Options:
		yes - Enable (default).
		no - Disable.

    event_exit_press
	Number of times you need to press the exit key to exit from
	the program.

	:event_exit_press 0 | 1 | 2 | 3

	Options:
		0 - Exit is disabled.
		1 - Exit after the first press (default).
		2 - Exit after two consecutive presses.
		3 - Exit after three consecutive presses.

	Examples:
		:event_exit_press 3

    run_msg
	One line message displayed when a game is chosen. The
	message is displayed only if the option `video_restore' is
	set to `no'.

	:run_msg "MESSAGE"

	Options:
		MESSAGE - Message to display (default "Run Game").
			To prevent the display of the message use the
			empty string "".

	Examples:
		:run_msg "Avvio il gioco..."

    run_preview
	Select the preview type to display when a game is run. The
	message is displayed only if the option `video_restore' is
	set to `no'.

	:run_preview none | snap | flyers | cabinets | titles

	Options:
		none - Don't diplay any preview.
		snap, flyers, cabinets, titles - Display the
			specified preview. (default snap).

    loop
	Select if the animated clips need to be played continuosly.

	:loop yes | no

	Options:
		no - Play the clips only one time (default).
		yes - Play the clips continuosly. The sound is not
			looped.

    color
	Colors used.

	:color TAG FOREGROUND BACKGROUND

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
		FOREGROUND - Foreground color. One of the following :
			black, blue, green, cyan, red, magenta, brown,
			lightgray, gray, lightblue, lightgreen
			lightcyan, lightred, lightmagenta, yellow,
			white.
		BACKGROUND - Background color. Like foreground color.

    misc_quiet
	Doesn't print the copyright text message at the startup :

	:AdvanceMENU - Copyright (C) 1999-200X by Andrea Mazzoleni

	:misc_quiet yes | no

Formats Supported
	This is the list of the file formats supported by AdvanceMENU.

	Images:
		PNG - The PNG format.
		PCX - The PCX format.
		ICO - The ICON formst.

	Clips:
		MNG - The MNG-VLC (Very Low Complexity) sub format
			without transparency and alpha channel.
			Or the subformats generated by AdvanceMAME or
			by the `advmng' compression utility.

	Sounds:
		MP3 - The MP3 format.
		WAV - The WAV format with a sample size of 16 bit.

	Fonts:
		RAW - The RAW format.
		PSF - The PSF format.
		GRX - The GRX format.

	Archives:
		ZIP - The ZIP format.

Signals
	The program intercepts the following signals:

		SIGQUIT - Exit normally
		SIGTERM - Exit restoring only the video

Copyright
	This file is Copyright (C) 2002 Andrea Mazzoleni, Randy Schnedler.

