/**********************************************************************

    pckeybrd.c

    PC-style keyboard emulation

    This emulation is decoupled from the AT 8042 emulation used in the
    IBM ATs and above

**********************************************************************/

/* Todo: (added by KT 22-Jun-2000
    1. Check scancodes I have added are the actual scancodes for set 2 or 3.
    2. Check how codes are changed based on Shift/Control states for those sets
       that require it - info in Help PC!

*/

#include "driver.h"
#include "machine/pckeybrd.h"

/* AT keyboard documentation comes from www.beyondlogic.org and HelpPC documentation */

/* to enable logging of keyboard read/writes */
#define LOG_KEYBOARD	0


/*
    The PS/2 models have three make/break scan code sets.  The first
      set matches the PC & XT make/break scan code set and is the one
      listed here.  Scan code sets are selected by writing the value F0
      to the keyboard via the ~8042~ (port 60h).  The following is a brief
      description of the scan code sets (see the PS/2 Technical Reference
      manuals for more information on scan code sets 2 and 3):

    *  set 1, each key has a base scan code.  Some keys generate
       extra scan codes to generate artificial shift states.  This
       is similar to the standard scan code set used on the PC and XT.
    *  set 2, each key sends one make scan code and two break scan
       codes bytes (F0 followed by the make code).  This scan code
       set is available on the IBM AT also.
    *  set 3, each key sends one make scan code and two break scan
       codes bytes (F0 followed by the make code) and no keys are
       altered by Shift/Alt/Ctrl keys.
    *  typematic scan codes are the same as the make scan code

*/


/* using the already existing input port definitions, this table re-maps
to scancode set 3.
I don't have the details for scan-code set 2,3 but they sound very similar
to the scancode set I have here. - KT 22/Jun/2000 */


/* key set 3 */
static int at_keyboard_scancode_set_2_3[]=
{
	0,
	0x076,
	0x016,
	0x01e,
	0x026,
	0x025,
	0x02e,
	0x036,
	0x03d,
	0x03e,
	0x046,
	0x045,
	0x04e,
	0x055,
	0x066,
	0x00d,
	0x015,
	0x01d,
	0x024,
	0x02d,
	0x02c,
	0x035,
	0x03c,
	0x043,
	0x044,
	0x04d,
	0x054,
	0x05b,
	0x05a,
	0x014,
	0x01c,
	0x01b,
	0x023,
	0x02b,
	0x034,
	0x033,
	0x03b,
	0x042,
	0x04b,
	0x04c,
	0x052,
	0x00e,
	0x012,
	0x05d,
	0x01a,
	0x022,
	0x021,
	0x02a,
	0x032,
	0x031,
	0x03a,
	0x041,
	0x049,
	0x04a,
	0x059,
	0x000,
	0x011,
	0x029,
	0x058,
	0x05,
	0x06,
	0x04,
	0x0c,
	0x03,
	0x0b,
	0x083,
	0x0a,
	0x01,
	0x09,
	0x077,
	0x07e,
	0x06c,
	0x075,
	0x07d,
	0x07b,
	0x06b,
	0x073,
	0x074,
	0x079,
	0x069,
	0x072,
	0x07a,
	0x070,
	0x071,
	0x00,
	0x00,
	0x078,
	0x07,
	0x05a,
	0x014,
	0x04a,
	0x000,
	0x011,
	0x06c,
	0x075,
	0x07d,
	0x06b,
	0x074,
	0x069,
	0x072,
	0x07a,
	0x070,
	0x071,
	0x000,
	0x000,
	0x000
};


#define AT_KEYBOARD_QUEUE_MAXSIZE	256

typedef struct at_keyboard
{
	AT_KEYBOARD_TYPE type;
	int on;
	UINT8 delay;   /* 240/60 -> 0,25s */
	UINT8 repeat;   /* 240/ 8 -> 30/s */
	int numlock;
	UINT8 queue[AT_KEYBOARD_QUEUE_MAXSIZE];
	UINT8 head;
	UINT8 tail;
	UINT8 make[128];

	int input_state;
	int scan_code_set;
	int last_code;

	int ports[8];
} at_keyboard;

static at_keyboard keyboard;

typedef struct extended_keyboard_code
{
	const char *pressed;
	const char *released;
} extended_keyboard_code;


static extended_keyboard_code keyboard_mf2_code[0x10][2/*numlock off, on*/]={
	{	{ "\xe0\x1c", "\xe0\x9c" } }, // keypad enter
	{	{ "\xe0\x1d", "\xe0\x9d" } }, // right control
	{	{ "\xe0\x35", "\xe0\xb5" } },
	{	{ "\xe0\x37", "\xe0\xb7" } },
	{	{ "\xe0\x38", "\xe0\xb8" } },
	{	{ "\xe0\x47", "\xe0\xc7" }, { "\xe0\x2a\xe0\x47", "\xe0\xc7\xe0\xaa" } },
	{	{ "\xe0\x48", "\xe0\xc8" }, { "\xe0\x2a\xe0\x48", "\xe0\xc8\xe0\xaa" } },
	{	{ "\xe0\x49", "\xe0\xc9" }, { "\xe0\x2a\xe0\x49", "\xe0\xc9\xe0\xaa" } },
	{	{ "\xe0\x4b", "\xe0\xcb" }, { "\xe0\x2a\xe0\x4b", "\xe0\xcb\xe0\xaa" } },
	{	{ "\xe0\x4d", "\xe0\xcd" },	{ "\xe0\x2a\xe0\x4d", "\xe0\xcd\xe0\xaa" } },
	{	{ "\xe0\x4f", "\xe0\xcf" }, { "\xe0\x2a\xe0\x4f", "\xe0\xcf\xe0\xaa" } },
	{	{ "\xe0\x50", "\xe0\xd0" }, { "\xe0\x2a\xe0\x50", "\xe0\xd0\xe0\xaa" } },
	{	{ "\xe0\x51", "\xe0\xd1" }, { "\xe0\x2a\xe0\x51", "\xe0\xd1\xe0\xaa" } },
	{	{ "\xe0\x52", "\xe0\xd2" }, { "\xe0\x2a\xe0\x52", "\xe0\xd2\xe0\xaa" } },
	{	{ "\xe0\x53", "\xe0\xd3" }, { "\xe0\x2a\xe0\x53", "\xe0\xd3\xe0\xaa" } },
	{	{ "\xe1\x1d\x45\xe1\x9d\xc5" }, { "\xe0\x2a\xe1\x1d\x45\xe1\x9d\xc5" } }
};

/* I don't think these keys change if num-lock is active! */
/* pc-at extended keyboard make/break codes for code set 3 */
static extended_keyboard_code at_keyboard_extended_codes_set_2_3[]=
{
	/*keypad enter */
	{
		"\xe0\x5a",
		"\xe0\xf0\x5a"
	},
	/* right control */
	{
		"\xe0\x14",
		"\xe0\xf0\x14"
	},
	/* keypad slash */
	{
		"\xe0\x4a",
		"\xe0\xf0\x4a"
	},
	/* print screen */
	{
		"\xe0\x12\xe0\x7c",
		0, /* I don't know the break sequence */

	},
	/* right alt */
	{
		"\xe0\x11",
		"\xe0\xf0\x11"
	},
	/* home */
	{
		"\xe0\x6c",
		"\xe0\xf0\x6c"
	},
	/* cursor up */
	{
		"\xe0\x75",
		"\xe0\xf0\x75"
	},
	/* page up */
	{
		"\xe0\x7d",
		"\xe0\xf0\x7d"
	},
	/* cursor left */
	{
		"\xe0\x6b",
		"\xe0\xf0\x6b",
	},
	/* cursor right */
	{
		"\xe0\x74",
		"\xe0\xf0\x74"
	},
	/* end */
	{
		"\xe0\x69",
		"\xe0\xf0\x69",
	},
	/* cursor down */
	{
		"\xe0\x72",
		"\xe0\xf0\x72"
	},
	/* page down */
	{
		"\xe0\x7a",
		"\xe0\xf0\x7a"
	},
	/* insert */
	{
		"\xe0\x70",
		"\xe0\xf0\x70",
	},
	/* delete */
	{
		"\xe0\x71",
		"\xe0\xf0\x71"
	},
	/* pause */
	{
		"\xe1\x14\x77\xe1\xf0\x14\xf0\x77",
		0, /*?? I don't know the break sequence */
	}

};

static void at_keyboard_queue_insert(UINT8 data);
static int at_keyboard_queue_size(void);



void at_keyboard_init(AT_KEYBOARD_TYPE type)
{
	int i;
	char buf[32];

	memset(&keyboard, 0, sizeof(keyboard));
	keyboard.type = type;
	keyboard.on = 1;
	keyboard.delay = 60;
	keyboard.repeat = 8;
	keyboard.numlock = 0;
	keyboard.head = keyboard.tail = 0;
	keyboard.input_state = 0;
	memset(&keyboard.make[0], 0, sizeof(UINT8)*128);
	/* set default led state */
	set_led_status(2, 0);
	set_led_status(0, 0);
	set_led_status(1, 0);

	keyboard.scan_code_set = 3;

	/* locate the keyboard ports */
	for (i = 0; i < sizeof(keyboard.ports) / sizeof(keyboard.ports[0]); i++)
	{
		sprintf(buf, "pc_keyboard_%d", i);
		keyboard.ports[i] = port_tag_to_index(buf);
	}
}



void at_keyboard_reset(void)
{
	keyboard.head = keyboard.tail = 0;
	keyboard.input_state = 0;
	memset(&keyboard.make[0], 0, sizeof(UINT8)*128);
	/* set default led state */
	set_led_status(2, 0);
	set_led_status(0, 0);
	set_led_status(1, 0);

	keyboard.scan_code_set=1;
	at_keyboard_queue_insert(0xaa);
}

/* set initial scan set */
void	at_keyboard_set_scan_code_set(int set)
{
	keyboard.scan_code_set = set;
}



/* insert a code into the buffer */
static void at_keyboard_queue_insert(UINT8 data)
{
	if (LOG_KEYBOARD)
		logerror("keyboard queueing %.2x\n",data);

	keyboard.queue[keyboard.head] = data;
	keyboard.head++;
	keyboard.head %= (sizeof(keyboard.queue) / sizeof(keyboard.queue[0]));
}



static int at_keyboard_queue_size(void)
{
	int queue_size;
	queue_size = keyboard.head - keyboard.tail;
	if (queue_size < 0)
		queue_size += sizeof(keyboard.queue) / sizeof(keyboard.queue[0]);
	return queue_size;
}



/* add a list of codes to the keyboard buffer */
static void at_keyboard_helper(const char *codes)
{
	int i;
	for (i = 0; codes[i]; i++)
		at_keyboard_queue_insert(codes[i]);
}


/* add codes for standard keys */
static void at_keyboard_standard_scancode_insert(int our_code, int pressed)
{
	int scancode = our_code;

	switch (keyboard.scan_code_set)
	{
		case 1:
		{
			/* the original code was designed for this set, and there is
            a 1:1 correspondance for the scancodes */
			scancode = our_code;

			if (!pressed)
			{
				/* adjust code for break code */
				scancode|=0x080;
			}
		}
		break;

		case 2:
		case 3:
		{
			/* lookup scancode */
			scancode = at_keyboard_scancode_set_2_3[our_code];

			if (!pressed)
			{
				/* break code */
				at_keyboard_queue_insert(0x0f0);
			}

		}
		break;
	}

	at_keyboard_queue_insert(scancode);
}

static void at_keyboard_extended_scancode_insert(int code, int pressed)
{
	code = code - 0x060;

	switch (keyboard.scan_code_set)
	{
		case 1:
		{
			if (pressed)
			{
				if (keyboard_mf2_code[code][keyboard.numlock].pressed)
					at_keyboard_helper(keyboard_mf2_code[code][keyboard.numlock].pressed);
				else
					at_keyboard_helper(keyboard_mf2_code[code][0].pressed);
			}
			else
			{
				if (keyboard_mf2_code[code][keyboard.numlock].released)
					at_keyboard_helper(keyboard_mf2_code[code][keyboard.numlock].released);
				else if (keyboard_mf2_code[code][0].released)
					at_keyboard_helper(keyboard_mf2_code[code][0].released);
			}
		}
		break;

		case 2:
		case 3:
		{
			extended_keyboard_code *key = &at_keyboard_extended_codes_set_2_3[code];

			if (pressed)
			{
				if (key->pressed)
				{
					at_keyboard_helper(key->pressed);
				}
			}
			else
			{
				if (key->released)
				{
					at_keyboard_helper(key->released);
				}
			}
		}
		break;
	}

}


/**************************************************************************
 *  scan keys and stuff make/break codes
 **************************************************************************/

static UINT32 at_keyboard_readport(int port)
{
	UINT32 result = 0;
	if (keyboard.ports[port] >= 0)
		result = readinputport(keyboard.ports[port]);
	return result;
}

void at_keyboard_polling(void)
{
	int i;

	if (keyboard.on)
	{
		/* add codes for keys that are set */
		for( i = 0x01; i < 0x80; i++  )
		{
			if (i==0x60) i+=0x10; // keys 0x60..0x6f need special handling

			if( at_keyboard_readport(i/16) & (1 << (i & 15)) )
			{
				if( keyboard.make[i] == 0 )
				{
					keyboard.make[i] = 1;

					if (i==0x45) keyboard.numlock^=1;

					at_keyboard_standard_scancode_insert(i,1);
				}
				else
				{
					keyboard.make[i] += 1;

					if( keyboard.make[i] == keyboard.delay )
					{
						at_keyboard_standard_scancode_insert(i, 1);
					}
					else
					{
						if( keyboard.make[i] == keyboard.delay + keyboard.repeat )
						{
							keyboard.make[i] = keyboard.delay;
							at_keyboard_standard_scancode_insert(i, 1);
						}
					}
				}
			}
			else
			{
				if( keyboard.make[i] )
				{
					keyboard.make[i] = 0;

					at_keyboard_standard_scancode_insert(i, 0);
				}
			}
		}

			/* extended scan-codes */
			for( i = 0x60; i < 0x70; i++  )
			{
				if( at_keyboard_readport(i/16) & (1 << (i & 15)) )
				{
					if( keyboard.make[i] == 0 )
					{
						keyboard.make[i] = 1;

						at_keyboard_extended_scancode_insert(i,1);

					}
					else
					{
						keyboard.make[i] += 1;
						if( keyboard.make[i] == keyboard.delay )
						{
							at_keyboard_extended_scancode_insert(i, 1);
						}
						else
						{
							if( keyboard.make[i] == keyboard.delay + keyboard.repeat )
							{
								keyboard.make[i]=keyboard.delay;

								at_keyboard_extended_scancode_insert(i, 1);
							}
						}
					}
				}
				else
				{
					if( keyboard.make[i] )
					{
						keyboard.make[i] = 0;

						at_keyboard_extended_scancode_insert(i,0);
					}
				}
		}
	}
}

int at_keyboard_read(void)
{
	int data;
	if (keyboard.tail == keyboard.head)
		return -1;

	data = keyboard.queue[keyboard.tail];

	if (LOG_KEYBOARD)
		logerror("at_keyboard_read(): Keyboard Read 0x%02x\n",data);

	keyboard.tail++;
	keyboard.tail %= sizeof(keyboard.queue) / sizeof(keyboard.queue[0]);
	return data;
}

static void at_clear_buffer_and_acknowledge(void)
{
	/* clear output buffer and respond with acknowledge */
	keyboard.head = keyboard.tail = 0;
	at_keyboard_queue_insert(0x0fa);
}

/* From Ralf Browns Interrupt list:

Values for keyboard commands (data also goes to PORT 0060h):
Value   Count   Description
 EDh    double  set/reset mode indicators Caps Num Scrl
        bit 2 = CapsLk, bit 1 = NumLk, bit 0 = ScrlLk
        all other bits must be zero.
 EEh    sngl    diagnostic echo. returns EEh.
 EFh    sngl    NOP (No OPeration). reserved for future use
 EF+26h double  [Cherry MF2 G80-1501HAD] read 256 bytes of chipcard data
        keyboard must be disabled before this and has to
        be enabled after finished.
 F0h    double  get/set scan code set
        00h get current set
        01h scancode set 1 (PCs and PS/2 mod 30, except Type 2 ctrlr)

        02h scancode set 2 (ATs, PS/2, default)
        03h scancode set 3
 F2h    sngl    read keyboard ID (read two ID bytes)
        AT keyboards returns FA (ACK)
        MF2 returns AB 41 (translation) or
                AB 83 (pass through)
 F3h    double  set typematic rate/delay
        format of the second byte:
        bit7=0 : reserved
        bit6-5 : typemativ delay
             00b=250ms     10b= 750ms
             01b=500ms     11b=1000ms
        bit4-0 : typematic rate (see #P050)
 F4h    sngl    enable keyboard
 F5h    sngl    disable keyboard. set default parameters (no keyboard scanning)
 F6h    sngl    set default parameters
 F7h    sngl    [MCA] set all keys to typematic (scancode set 3)

 F8h    sngl    [MCA] set all keys to make/release
 F9h    sngl    [MCA] set all keys to make only
 FAh    sngl    [MCA] set all keys to typematic/make/release
 FBh    sngl    [MCA] set al keys to typematic
 FCh    double  [MCA] set specific key to make/release
 FDh    double  [MCA] set specific key to make only
 FEh    sngl    resend last scancode
 FFh    sngl    perform internal power-on reset function
Note:   each command is acknowledged by FAh (ACK), if not mentioned otherwise.
      See PORT 0060h-R for details.
SeeAlso: #P046
*/

void at_keyboard_write(UINT8 data)
{
	if (LOG_KEYBOARD)
		logerror("keyboard write %.2x\n",data);

	switch (keyboard.input_state)
	{
		case 0:
			switch (data) {
			case 0xed: // leds schalten
				/* acknowledge */
				at_keyboard_queue_insert(0x0fa);
				/* now waiting for  code... */
				keyboard.input_state=1;
				break;
			case 0xee: // echo
				/* echo code with no acknowledge */
				at_keyboard_queue_insert(0xee);
				break;
			case 0xf0: // scancodes adjust
				/* acknowledge */
				at_clear_buffer_and_acknowledge();
				/* waiting for data */
				keyboard.input_state=2;
				break;
			case 0xf2: // identify keyboard
				/* ack and two byte keyboard id */
				at_keyboard_queue_insert(0xfa);

				/* send keyboard code */
				if (keyboard.type == AT_KEYBOARD_TYPE_MF2) {
					at_keyboard_queue_insert(0xab);
					at_keyboard_queue_insert(0x41);
				}
				else
				{
					/* from help-pc docs */
					at_keyboard_queue_insert(0x0ab);
					at_keyboard_queue_insert(0x083);
				}

				break;
			case 0xf3: // adjust rates
				/* acknowledge */
				at_keyboard_queue_insert(0x0fa);

				keyboard.input_state=3;
				break;
			case 0xf4: // activate
				at_clear_buffer_and_acknowledge();

				keyboard.on=1;
				break;
			case 0xf5:
				/* acknowledge */
				at_clear_buffer_and_acknowledge();
				// standardvalues
				keyboard.on=0;
				break;
			case 0xf6:
				at_clear_buffer_and_acknowledge();
				// standardvalues
				keyboard.on=1;
				break;
			case 0xfe: // resend
				// should not happen, for now send 0
				at_keyboard_queue_insert(0);	//keyboard.last_code);
				break;
			case 0xff: // reset
				/* it doesn't state this in the docs I have read, but I assume
                that the keyboard input buffer is cleared. The PCW16 sends &ff,
                and requires that 0x0fa is the first byte to be read */

				at_clear_buffer_and_acknowledge();

	//          /* acknowledge */
	//          at_keyboard_queue_insert(0xfa);
				/* BAT completion code */
				at_keyboard_queue_insert(0xaa);
				break;
			}
			break;
		case 1:
			/* code received */
			keyboard.input_state=0;

			/* command? */
			if (data & 0x080)
			{
				/* command received instead of code - execute command */
				at_keyboard_write(data);
			}
			else
			{
				/* send acknowledge */
				at_keyboard_queue_insert(0x0fa);

				/* led  bits */
				/* bits: 0 scroll lock, 1 num lock, 2 capslock */

				/* led's in same order as my keyboard leds. */
				/* num lock, caps lock, scroll lock */
				set_led_status(2, (data & 0x01));
				set_led_status(0, ((data & 0x02)>>1));
				set_led_status(1, ((data & 0x04)>>2));

			}
			break;
		case 2:
			keyboard.input_state=0;

			/* command? */
			if (data & 0x080)
			{
				/* command received instead of code - execute command */
				at_keyboard_write(data);
			}
			else
			{
				/* 00  return byte indicating scan code set in use
                01  select scan code set 1  (used on PC & XT)
                02  select scan code set 2
                03  select scan code set 3
                */

				if (data == 0x00)
				{
						at_keyboard_queue_insert(keyboard.scan_code_set);
				}
				else
				{
					keyboard.scan_code_set = data;
				}
			}

			break;
		case 3:
			/* 6,5: 250ms, 500ms, 750ms, 1s */
			/* 4..0: 30 26.7 .... 2 chars/s*/

			/* command? */
			keyboard.input_state=0;
			if (data & 0x080)
			{
				/* command received instead of code - execute command */
				at_keyboard_write(data);
			}
			else
			{
				/* received keyboard repeat */

			}

			break;
	}
}
