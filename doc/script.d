Name
	script - The AdvanceMAME Script Machine

Description
	AdvanceMAME support a scripting language to control external
	device like lamps, keyboard leds...
	You can assign a simple script at various events like key press,
	game led state change or others.
	When the event is triggered the script is started.

Grammar
	The scripts are very C like.

Scripts
	These are the available scripts:
		script_video - The video mode is set. It's the first event.
		script_emulation - The emulation start. It happens after
			the MAME confirmation screens.
		script_play - The game play start. It happens after the
			turbo_startup_time.
		script_led1,2,3 - The leds controlled by the emulated
			game is enabled.
		script_turbo - The turbo button is pressed.
		script_coin1,2,3,4 - A coin button is pressed.
		script_start1,2,3,4 - A start button is pressed.
		script_safequit - The game enter in the safequit status.
		script_event1-14 - The events controlled by the
			event system.

	You can assign a script at every event. The script is started
	when the event is triggered.

Ports
	The external devices can be controlled using the keyboard led
	or the common PC ports, like the parallel port.

	To maintain the same interface the keyboard leds are mapped
	to the virtual port 0 at the lower 3 bits.
	All the other ports are mapped to the PC hardware ports.

	These are the available functions to read and write the ports:

		set(ADDRESS, VALUE) - set the port with the specified value
		get(ADDRESS) - return the value of the port
		on(ADDRESS,VALUE) - like set(ADDRESS, get(ADDRESS) | VALUE)
		off(ADDRESS,VALUE) - like set(ADDRESS, get(ADDRESS) & ~VALUE)
		toggle(ADDRESS,VALUE) - like set(ADDRESS, get(ADDRESS) ^ VALUE)

	In DOS the hardware ports are accessed directly, in Linux through the
	/dev/port interface.

Values
	These are the available value formats:
		... - decimal format
		0x... - hexadecimal format
		0b... - binary format

	Examples:
		10 - decimal format of 10
		0xA - hexadecimal format of 10
		0b1010 - binary format of 10

Operators
	These are the available operators:
		+ - addition
		- - subtraction
		& - bitwise and
		| - bitwise or
		^ - bitwise xor
		~ - bitwise not
		&& - logical and
		|| - logical or
		! - logical not
		< - less
		> - greater
		== - equal
		<= - less or equal
		>= - greater or equal

Commands
	These are the available commands:

		event() - Return 0 if the event that started the
			script is terminated.
		event(EVENT) - Return 0 if the specified event is
			not active. Return 1 if the event is active.
		simulate_event(EVENT,N) - Simulate the specified
			event for N milli seconds.
		simulate_key(KEY,N) - Simulate the specified key for
			N milli seconds.
		wait(CONDITION) - Wait until the condition is true.
		delay(N) - Wait the specified N milli seconds.
		while (CONDITION) { ... } - While != 0.
		if (CONDITION) { ...  } - If != 0.
		loop { ... } - Repeat forever.
		repeat (N) { ... } - Repeat N times.

	The 'event()' command can be used to determine the end of the event
	that started the script. For example for the 'coin1' event the
	function return 0 when the key is released.

	The 'wait()' and 'delay()' command are used to maintain the
	synchronization with the game emulation. When these commands
	are executed the script is temporarily suspended. All others
	commands are executed continuously without any delay.

	The granularity of the delay command is the frame rate period.
	Approx. 16 ms for a 60 Hz game. Delays under this limit don't
	have the desired effect.

	IT'S VERY IMPORTANT THAT EVERY 'loop', 'while' AND 'repeat'
	COMMANDS CONTAIN A 'delay()' CALL. Otherwise the script may be
	looped forever and the executable may locks! For example the
	statement :

		:loop { toggle(kdb, 1); }

	completely STOP the emulation and you CAN'T EXIT from MAME.
	A correct one is :

		:loop { toggle(kdb, 1); delay(100); }

Symbols
	This is the complete list of all the predefinite symbols
	available.

	These are the port symbols available for the
	`set, get, on, off, toggle' commands :
		kdb - The address of the virtual keyboard port (0)
		lpt1 - The address of the lpt1 port (0x378)
		lpt2 - The address of the lpt2 port (0x278)
		lpt3 - The address of the lpt2 port (0x3bc)

	These are the event symbols available for the
	`simulate_event(EVENT,TIME)' and `event(EVENT)' 
	commands :
		EVENT - p1_up, p1_down, p1_left, p1_right, p2_up, p2_down,
			p2_left, p2_right, p1_button1, p1_button2, p1_button3,
			p1_button4, p1_button5, p1_button6, p1_button7,
			p1_button8, p1_button9, p1_button10, p2_button1,
			p2_button2, p2_button3, p2_button4, p2_button5,
			p2_button6, p2_button7, p2_button8, p2_button9,
			p2_button10, start1, start2, start3, start4, coin1,
			coin2, coin3, coin4, service_coin1, service_coin2,
			service_coin3, service_coin4, service, tilt.

	These are the user interface event symbols available for
	the `simulate_event(EVENT,TIME)' and `event(EVENT)' 
	commands :
		EVENT - ui_mode_next, ui_mode_pred, ui_record_start,
			ui_record_stop, ui_turbo, ui_cocktail, ui_configure,
			ui_on_screen_display, ui_pause, ui_reset_machine,
			ui_show_gfx, ui_frameskip_dec, ui_frameskip_inc,
			ui_throttle, ui_show_fps, ui_snapshot, ui_toggle_cheat,
			ui_up, ui_down, ui_left, ui_right, ui_select,
			ui_cancel, ui_pan_up, ui_pan_down, ui_pan_left,
			ui_pan_right, ui_show_profiler, ui_show_colors,
			ui_toggle_ui, ui_toggle_debug, ui_save_state,
			ui_load_state, ui_add_cheat, ui_delete_cheat,
			ui_save_cheat, ui_watch_value.

	These are the keyboard symbols available for the
	`simulate_key(EVENT,TIME)' command :
		EVENT - key_a, key_b, key_c, key_d, key_e, key_f, key_g,
			key_h, key_i, key_j, key_k, key_l, key_m, key_n,
			key_o, key_p, key_q, key_r, key_s, key_t, key_u,
			key_v, key_w, key_x, key_y, key_z, key_0, key_1,
			key_2, key_3, key_4, key_5, key_6, key_7, key_8,
			key_9, key_0_pad, key_1_pad, key_2_pad, key_3_pad,
			key_4_pad, key_5_pad, key_6_pad, key_7_pad,
			key_8_pad, key_9_pad, key_f1, key_f2, key_f3,
			key_f4, key_f5, key_f6, key_f7, key_f8, key_f9,
			key_f10, key_f11, key_f12, key_esc, key_backquote,
			key_minus, key_equals, key_backspace, key_tab,
			key_openbrace, key_closebrace, key_enter, key_semicolon,
			key_quote, key_backslash, key_less, key_comma,
			key_period, key_slash, key_space, key_insert, key_del,
			key_home, key_end, key_pgup, key_pgdn, key_left,
			key_right, key_up, key_down, key_slash_pad,
			key_asterisk_pad, key_minus_pad, key_plus_pad,
			key_period_pad, key_enter_pad, key_prtscr, key_pause,
			key_lshift, key_rshift, key_lcontrol, key_rcontrol,
			key_lalt, key_ralt, key_scrlock, key_numlock,
			key_capslock, key_lwin, key_rwin, key_menu.

Examples
	This script clears all the keyboard leds at the emulation end :

		:script_video wait(!event()); set(kdb,0);

	Activate all the parallel data bits when the game start, flash
	the bit 0 during the emulation and clear them then the game
	stop :

		:script_start set(lpt1, 0xff); while(event()) { toggle(lpt1, 1); delay(500); } set(lpt1, 0);

	Map the first MAME led to the first keyboard led:

		:script_led1 on(kdb, 0b1); wait(!event()); off(kdb, 0b1);

	Map the second MAME led to the first keyboard led:

		:script_led2 on(kdb, 0b10); wait(!event()); off(kdb, 0b10);

	Flash the third keyboard led when the 'turbo' is active :

		:script_turbo while (event()) { toggle(kdb, 0b100); delay(500); } off(kdb, 0b100);

	Light the third keyboard led when the 'coin1' key is pressed :

		:script_coin1 on(kdb, 0b100); delay(500); off(kdb, 0b100);

	Add 3 coins automatically:

		:script_play delay(1000); repeat(3) { simulate_event(coin1,100); delay(200); }

	Add a coin when the player start:

		:script_start1 wait(!event()); simulate_event(coin1,100); delay(500); simulate_event(start1,100);

Configuration
	The scripts must be inserted in the 'advmame.rc' file.

	For example:

		:script_video wait(!event()); set(kdb,0);
		:script_led1 on(kdb, 0b1); wait(!event()); off(kdb, 0b1);
		:script_led2 on(kdb, 0b10); wait(!event()); off(kdb, 0b10);
		:script_coin1 on(kdb, 0b100); delay(500); off(kdb, 0b100);
		:script_turbo while (event()) { toggle(kdb, 0b100); delay(500); } off(kdb, 0b100);
		:script_start1 \
		:	set(lpt1, 0xff); \
		:	while(event()) { \
		:		toggle(lpt1, 1); \
		:		delay(500); \
		:	} \
		:	set(lpt1, 0);

Copyright
	This file is Copyright (C) 2003 Andrea Mazzoleni.

