NAME
	advemu - Release notes for AdvanceMAME/MESS/PAC

Release notes for AdvanceMAME/MESS/PAC
    AdvanceMAME 0.61.0
	The `input_analog' and `input_track' options are now substituted
	by the new `input_map' option.

    AdvanceMAME 0.59.1
	The option `input_analog[] joy[]' is now changed in
	`input_analog[] joystick[]', where the joystick stick index is
	decremented by 1. The conversion is done automatically.

    AdvanceMAME 0.57.1
	You must remove the option `dir_sound' from the configuration file. The
	sound file are now saved in the `dir_snap' directory.
	You must rename manually the Linux config directories to `$home/.advance' and
	`$prefix/share/advance'. (Previously they were `*/advmame').

    AdvanceMAME 0.56.2
	This is a big update for AdvanceMAME with a lot of changes.
	Don't uncompress the new archives in your MAME directory, but create
	a new one.

	To convert your old mame.cfg to the new format follow these steps :

	* Rename your configuration file as "mame.cfg" if it has a different name.
	* Copy it in the same directory of "advmame.exe".
	* Ensure that a file named "advmame.rc" doesn't exist.
	* Run "advmame.exe".
	* A file named "advmame.rc" should be now present in your current directory.

	This conversion works only for the DOS version of AdvanceMAME.

