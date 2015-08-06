/***************************************************************************

    inptport.h

    Handle input ports and mappings.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __INPTPORT_H__
#define __INPTPORT_H__

#include "memory.h"
#include "input.h"

#ifdef MESS
#include "unicode.h"
#endif



/*************************************
 *
 *  Constants
 *
 *************************************/

#define MAX_INPUT_PORTS		30
#define MAX_PLAYERS			8
#define MAX_BITS_PER_PORT	32

#define IP_ACTIVE_HIGH		0x00000000
#define IP_ACTIVE_LOW		0xffffffff


/* sequence types for input_port_seq() call */
enum
{
	SEQ_TYPE_STANDARD = 0,
	SEQ_TYPE_INCREMENT = 1,
	SEQ_TYPE_DECREMENT = 2
};


/* conditions for DIP switches */
enum
{
	PORTCOND_ALWAYS = 0,
	PORTCOND_EQUALS,
	PORTCOND_NOTEQUALS
};


/* groups for input ports */
enum
{
	IPG_UI = 0,
	IPG_PLAYER1,
	IPG_PLAYER2,
	IPG_PLAYER3,
	IPG_PLAYER4,
	IPG_PLAYER5,
	IPG_PLAYER6,
	IPG_PLAYER7,
	IPG_PLAYER8,
	IPG_OTHER,
	IPG_TOTAL_GROUPS,
	IPG_INVALID
};


/* various input port types */
enum
{
	/* pseudo-port types */
	IPT_INVALID = 0,
	IPT_UNUSED,
	IPT_END,
	IPT_UNKNOWN,
	IPT_PORT,
	IPT_DIPSWITCH_NAME,
	IPT_DIPSWITCH_SETTING,
	IPT_VBLANK,
	IPT_CONFIG_NAME,			/* MESS only */
	IPT_CONFIG_SETTING,			/* MESS only */
	IPT_CATEGORY_NAME,			/* MESS only */
	IPT_CATEGORY_SETTING,		/* MESS only */

	/* start buttons */
	IPT_START1,
	IPT_START2,
	IPT_START3,
	IPT_START4,
	IPT_START5,
	IPT_START6,
	IPT_START7,
	IPT_START8,

	/* coin slots */
	IPT_COIN1,
	IPT_COIN2,
	IPT_COIN3,
	IPT_COIN4,
	IPT_COIN5,
	IPT_COIN6,
	IPT_COIN7,
	IPT_COIN8,
	IPT_BILL1,

	/* service coin */
	IPT_SERVICE1,
	IPT_SERVICE2,
	IPT_SERVICE3,
	IPT_SERVICE4,

	/* misc other digital inputs */
	IPT_SERVICE,
	IPT_TILT,
	IPT_INTERLOCK,
	IPT_VOLUME_UP,
	IPT_VOLUME_DOWN,
	IPT_START,					/* MESS only */
	IPT_SELECT,					/* MESS only */
	IPT_KEYBOARD,				/* MESS only */

#define __ipt_digital_joystick_start IPT_JOYSTICK_UP
	/* use IPT_JOYSTICK for panels where the player has one single joystick */
	IPT_JOYSTICK_UP,
	IPT_JOYSTICK_DOWN,
	IPT_JOYSTICK_LEFT,
	IPT_JOYSTICK_RIGHT,

	/* use IPT_JOYSTICKLEFT and IPT_JOYSTICKRIGHT for dual joystick panels */
	IPT_JOYSTICKRIGHT_UP,
	IPT_JOYSTICKRIGHT_DOWN,
	IPT_JOYSTICKRIGHT_LEFT,
	IPT_JOYSTICKRIGHT_RIGHT,
	IPT_JOYSTICKLEFT_UP,
	IPT_JOYSTICKLEFT_DOWN,
	IPT_JOYSTICKLEFT_LEFT,
	IPT_JOYSTICKLEFT_RIGHT,
#define __ipt_digital_joystick_end IPT_JOYSTICKLEFT_RIGHT

	/* action buttons */
	IPT_BUTTON1,
	IPT_BUTTON2,
	IPT_BUTTON3,
	IPT_BUTTON4,
	IPT_BUTTON5,
	IPT_BUTTON6,
	IPT_BUTTON7,
	IPT_BUTTON8,
	IPT_BUTTON9,
	IPT_BUTTON10,

	/* mahjong inputs */
	IPT_MAHJONG_A,
	IPT_MAHJONG_B,
	IPT_MAHJONG_C,
	IPT_MAHJONG_D,
	IPT_MAHJONG_E,
	IPT_MAHJONG_F,
	IPT_MAHJONG_G,
	IPT_MAHJONG_H,
	IPT_MAHJONG_I,
	IPT_MAHJONG_J,
	IPT_MAHJONG_K,
	IPT_MAHJONG_L,
	IPT_MAHJONG_M,
	IPT_MAHJONG_N,
	IPT_MAHJONG_O,
	IPT_MAHJONG_P,
	IPT_MAHJONG_Q,
	IPT_MAHJONG_KAN,
	IPT_MAHJONG_PON,
	IPT_MAHJONG_CHI,
	IPT_MAHJONG_REACH,
	IPT_MAHJONG_RON,
	IPT_MAHJONG_BET,
	IPT_MAHJONG_LAST_CHANCE,
	IPT_MAHJONG_SCORE,
	IPT_MAHJONG_DOUBLE_UP,
	IPT_MAHJONG_FLIP_FLOP,
	IPT_MAHJONG_BIG,
	IPT_MAHJONG_SMALL,

	/* analog inputs */
#define __ipt_analog_start IPT_PADDLE
	IPT_PADDLE,			/* absolute */
	IPT_PADDLE_V,		/* absolute */
	IPT_AD_STICK_X,		/* absolute */
	IPT_AD_STICK_Y,		/* absolute */
	IPT_AD_STICK_Z,		/* absolute */
	IPT_LIGHTGUN_X,		/* absolute */
	IPT_LIGHTGUN_Y,		/* absolute */
	IPT_PEDAL,			/* absolute */
	IPT_PEDAL2,			/* absolute */
	IPT_PEDAL3,			/* absolute */
	IPT_DIAL,			/* relative */
	IPT_DIAL_V,			/* relative */
	IPT_TRACKBALL_X,	/* relative */
	IPT_TRACKBALL_Y,	/* relative */
	IPT_MOUSE_X,		/* relative */
	IPT_MOUSE_Y,		/* relative */
#define __ipt_analog_end IPT_MOUSE_Y

	/* analog adjuster support */
	IPT_ADJUSTER,

	/* the following are special codes for user interface handling - not to be used by drivers! */
	IPT_UI_CONFIGURE,
	IPT_UI_ON_SCREEN_DISPLAY,
	IPT_UI_DEBUG_BREAK,
	IPT_UI_PAUSE,
	IPT_UI_RESET_MACHINE,
	IPT_UI_SOFT_RESET,
	IPT_UI_SHOW_GFX,
	IPT_UI_FRAMESKIP_DEC,
	IPT_UI_FRAMESKIP_INC,
	IPT_UI_THROTTLE,
	IPT_UI_SHOW_FPS,
	IPT_UI_SNAPSHOT,
	IPT_UI_RECORD_MOVIE,
	IPT_UI_TOGGLE_CHEAT,
	IPT_UI_UP,
	IPT_UI_DOWN,
	IPT_UI_LEFT,
	IPT_UI_RIGHT,
	IPT_UI_HOME,
	IPT_UI_END,
	IPT_UI_SELECT,
	IPT_UI_CANCEL,
	IPT_UI_CLEAR,
	IPT_UI_PAN_UP,
	IPT_UI_PAN_DOWN,
	IPT_UI_PAN_LEFT,
	IPT_UI_PAN_RIGHT,
	IPT_UI_SHOW_PROFILER,
	IPT_UI_TOGGLE_UI,
	IPT_UI_TOGGLE_DEBUG,
	IPT_UI_SAVE_STATE,
	IPT_UI_LOAD_STATE,
	IPT_UI_ADD_CHEAT,
	IPT_UI_DELETE_CHEAT,
	IPT_UI_SAVE_CHEAT,
	IPT_UI_WATCH_VALUE,
	IPT_UI_EDIT_CHEAT,
	IPT_UI_TOGGLE_CROSSHAIR,

	/* additional OSD-specified UI port types (up to 16) */
	IPT_OSD_1,
	IPT_OSD_2,
	IPT_OSD_3,
	IPT_OSD_4,
	IPT_OSD_5,
	IPT_OSD_6,
	IPT_OSD_7,
	IPT_OSD_8,
	IPT_OSD_9,
	IPT_OSD_10,
	IPT_OSD_11,
	IPT_OSD_12,
	IPT_OSD_13,
	IPT_OSD_14,
	IPT_OSD_15,
	IPT_OSD_16,

	/* other meaning not mapped to standard defaults */
	IPT_OTHER,

	/* special meaning handled by custom code */
	IPT_SPECIAL,

	__ipt_max
};


/* default strings used in port definitions */
enum
{
	STR_Off = 0,
	STR_On,
	STR_No,
	STR_Yes,
	STR_Lives,
	STR_Bonus_Life,
	STR_Difficulty,
	STR_Demo_Sounds,
	STR_Coinage,
	STR_Coin_A,
	STR_Coin_B,
	STR_9C_1C,
	STR_8C_1C,
	STR_7C_1C,
	STR_6C_1C,
	STR_5C_1C,
	STR_4C_1C,
	STR_3C_1C,
	STR_8C_3C,
	STR_4C_2C,
	STR_2C_1C,
	STR_5C_3C,
	STR_3C_2C,
	STR_4C_3C,
	STR_4C_4C,
	STR_3C_3C,
	STR_2C_2C,
	STR_1C_1C,
	STR_4C_5C,
	STR_3C_4C,
	STR_2C_3C,
	STR_4C_7C,
	STR_2C_4C,
	STR_1C_2C,
	STR_2C_5C,
	STR_2C_6C,
	STR_1C_3C,
	STR_2C_7C,
	STR_2C_8C,
	STR_1C_4C,
	STR_1C_5C,
	STR_1C_6C,
	STR_1C_7C,
	STR_1C_8C,
	STR_1C_9C,
	STR_Free_Play,
	STR_Cabinet,
	STR_Upright,
	STR_Cocktail,
	STR_Flip_Screen,
	STR_Service_Mode,
	STR_Pause,
	STR_Test,
	STR_Tilt,
	STR_Version,
	STR_Region,
	STR_International,
	STR_Japan,
	STR_USA,
	STR_Europe,
	STR_Asia,
	STR_World,
	STR_Hispanic,
	STR_Language,
	STR_English,
	STR_Japanese,
	STR_German,
	STR_French,
	STR_Italian,
	STR_Spanish,
	STR_Very_Easy,
	STR_Easiest,
	STR_Easier,
	STR_Easy,
	STR_Normal,
	STR_Medium,
	STR_Hard,
	STR_Harder,
	STR_Hardest,
	STR_Very_Hard,
	STR_Very_Low,
	STR_Low,
	STR_High,
	STR_Higher,
	STR_Highest,
	STR_Very_High,
	STR_Players,
	STR_Controls,
	STR_Dual,
	STR_Single,
	STR_Game_Time,
	STR_Continue_Price,
	STR_Controller,
	STR_Light_Gun,
	STR_Joystick,
	STR_Trackball,
	STR_Continues,
	STR_Allow_Continue,
	STR_Level_Select,
	STR_Infinite,
	STR_Stereo,
	STR_Mono,
	STR_Unused,
	STR_Unknown,
	STR_Standard,
	STR_Reverse,
	STR_Alternate,
	STR_None,
	STR_TOTAL
};



/*************************************
 *
 *  Type definitions
 *
 *************************************/

/* this is an opaque type */
typedef struct _input_port_init_params input_port_init_params;


/* In mamecore.h: typedef struct _input_port_default_entry input_port_default_entry; */
struct _input_port_default_entry
{
	UINT32		type;			/* type of port; see enum above */
	UINT8		group;			/* which group the port belongs to */
	UINT8		player;			/* player number (0 is player 1) */
	const char *token;			/* token used to store settings */
	const char *name;			/* user-friendly name */
	input_seq	defaultseq;		/* default input sequence */
	input_seq	defaultincseq;	/* default input sequence to increment (analog ports only) */
	input_seq	defaultdecseq;	/* default input sequence to decrement (analog ports only) */
};


/* In mamecore.h: typedef struct _input_port_entry input_port_entry; */
struct _input_port_entry
{
	UINT32		mask;			/* bits affected */
	UINT32		default_value;	/* default value for the bits affected */
								/* you can also use one of the IP_ACTIVE defines above */
	UINT32		type;			/* see enum above */
	UINT8		unused;			/* The bit is not used by this game, but is used */
								/* by other games running on the same hardware. */
								/* This is different from IPT_UNUSED, which marks */
								/* bits not connected to anything. */
	UINT8		cocktail;		/* the bit is used in cocktail mode only */
	UINT8		player;			/* the player associated with this port; note that */
								/* player 1 is '0' */
	UINT8		toggle;			/* When this is set, the key acts as a toggle - press */
								/* it once and it goes on, press it again and it goes off. */
								/* useful e.g. for some Test Mode dip switches. */
	UINT8		impulse;		/* When this is set, when the key corresponding to */
								/* the input bit is pressed it will be reported as */
								/* pressed for a certain number of video frames and */
								/* then released, regardless of the real status of */
								/* the key. This is useful e.g. for some coin inputs. */
								/* The number of frames the signal should stay active */
								/* is specified in the "arg" field. */
	UINT8		four_way;		/* Joystick modes of operation. 8WAY is the default, */
								/* it prevents left/right or up/down to be pressed at */
								/* the same time. 4WAY prevents diagonal directions. */
								/* 2WAY should be used for joysticks wich move only */
								/* on one axis (e.g. Battle Zone) */
	UINT16		category;		/* (MESS-specific) category */
	const char *name;			/* user-friendly name to display */
	input_seq	seq;			/* input sequence affecting the input bits */
	UINT32		(*custom)(void *);/* custom callback routine */
	void *		custom_param;	/* parameter for callback routine */

	/* valid if type is between __ipt_analog_start and __ipt_analog_end */
	struct
	{
		INT32	min;			/* minimum value for absolute axes */
		INT32	max;			/* maximum value for absolute axes */
		INT32	sensitivity;	/* sensitivity (100=normal) */
		INT32	delta;			/* delta to apply each frame a digital inc/dec key is pressed */
		INT32	centerdelta;	/* delta to apply each frame no digital inputs are pressed */
		UINT8	reverse;		/* reverse the sense of the analog axis */
		UINT8	reset;			/* always preload in->default for relative axes, returning only deltas */
		input_seq incseq;		/* increment sequence */
		input_seq decseq;		/* decrement sequence */
	} analog;

	/* valid if type is IPT_PORT */
	struct
	{
		const char *tag;		/* used to tag PORT_START declarations */
	} start;

	/* valid for most types */
	struct
	{
		const char *tag;		/* port tag to use for condition */
		UINT8	portnum;		/* port number for condition */
		UINT8	condition;		/* condition to use */
		UINT32	mask;			/* mask to apply to the port */
		UINT32	value;			/* value to compare against */
	} condition;

	/* valid for IPT_DIPNAME */
	struct
	{
		const char *swname;		/* name of the physical DIP switch */
		UINT8	swnum;			/* physical switch number */
	} diploc[8];

	/* valid if type is IPT_KEYBOARD */
#ifdef MESS
	struct
	{
		unicode_char_t chars[3];/* (MESS-specific) unicode key data */
	} keyboard;
#endif
};



/*************************************
 *
 *  Macros for building input ports
 *
 *************************************/

#define IP_NAME_DEFAULT 	NULL

/* start of table */
#define INPUT_PORTS_START(name)										\
 	void construct_ipt_##name(input_port_init_params *param)		\
	{																\
 		const char *modify_tag = NULL;								\
 		input_port_entry *port;										\
		int seq_index[3];											\
		int key;													\
		(void) port; (void) seq_index; (void) key; (void)modify_tag;\

/* end of table */
#define INPUT_PORTS_END												\
	}																\

/* aliasing */
#define INPUT_PORTS_ALIAS(name, base)								\
 	void construct_ipt_##name(input_port_init_params *param)		\
	{																\
 		construct_ipt_##base(param);								\
	}																\

#define INPUT_PORTS_EXTERN(name)									\
	extern void construct_ipt_##name(input_port_init_params *param)	\

/* including */
#define PORT_INCLUDE(name)											\
 	construct_ipt_##name(param);									\

/* start of a new input port */
#define PORT_START_TAG(tag_)										\
	modify_tag = NULL;												\
	port = input_port_initialize(param, IPT_PORT, NULL, 0);			\
	port->start.tag = (tag_);										\

/* start of a new input port */
#define PORT_START													\
	PORT_START_TAG(NULL)											\

/* modify an existing port */
#define PORT_MODIFY(tag_)											\
	modify_tag = (tag_);											\

/* input bit definition */
#define PORT_BIT(mask_,default_,type_) 								\
	port = input_port_initialize(param, (type_), modify_tag, (mask_));\
	port->mask = (mask_);											\
	port->default_value = (default_);								\
	seq_index[0] = seq_index[1] = seq_index[2] = key = 0;			\

/* new technique to append a code */
#define PORT_CODE_SEQ(code_,seq_,si_)								\
	if ((code_) < __code_max)										\
	{																\
		if (seq_index[si_] > 0)										\
			port->seq_.code[seq_index[si_]++] = CODE_OR;			\
		port->seq_.code[seq_index[si_]++] = (code_);				\
	}																\

#define PORT_CODE(code) PORT_CODE_SEQ(code,seq,0)
#define PORT_CODE_DEC(code)	PORT_CODE_SEQ(code,analog.decseq,1)
#define PORT_CODE_INC(code)	PORT_CODE_SEQ(code,analog.incseq,2)

#define PORT_2WAY													\
	port->four_way = FALSE;											\

#define PORT_4WAY													\
	port->four_way = TRUE;											\

#define PORT_8WAY													\
	port->four_way = FALSE;											\

#define PORT_PLAYER(player_)										\
	port->player = (player_) - 1;									\

#define PORT_COCKTAIL												\
	port->cocktail = TRUE;											\
	port->player = TRUE;											\

#define PORT_TOGGLE													\
	port->toggle = TRUE;											\

#define PORT_NAME(name_)											\
	port->name = (name_);											\

#define PORT_IMPULSE(duration_)										\
	port->impulse = (duration_);									\

#define PORT_REVERSE												\
	port->analog.reverse = TRUE;									\

#define PORT_RESET													\
	port->analog.reset = TRUE;										\

#define PORT_MINMAX(min_,max_)										\
	port->analog.min = (min_);										\
	port->analog.max = (max_);										\

#define PORT_SENSITIVITY(sensitivity_)								\
	port->analog.sensitivity = (sensitivity_);						\

#define PORT_KEYDELTA(delta_)										\
	port->analog.delta = port->analog.centerdelta = (delta_);		\

/* note that PORT_CENTERDELTA must appear after PORT_KEYDELTA */
#define PORT_CENTERDELTA(delta_)									\
	port->analog.centerdelta = (delta_);							\

#define PORT_UNUSED													\
	port->unused = TRUE;											\

#define PORT_CUSTOM(callback_, param_)								\
	port->custom = callback_;										\
	port->custom_param = (void *)(param_);							\


/* dip switch definition */
#define PORT_DIPNAME(mask,default,name)								\
	PORT_BIT(mask, default, IPT_DIPSWITCH_NAME) PORT_NAME(name)		\

#define PORT_DIPSETTING(default,name)								\
	PORT_BIT(0, default, IPT_DIPSWITCH_SETTING) PORT_NAME(name)		\

/* physical location, of the form: name:sw,[name:]sw,... */
#define PORT_DIPLOCATION(location_)									\
	input_port_parse_diplocation(port, location_);					\

/* conditionals for dip switch settings */
#define PORT_CONDITION(tag_,mask_,condition_,value_)				\
	port->condition.tag = (tag_);									\
	port->condition.mask = (mask_);									\
	port->condition.condition = (condition_);						\
	port->condition.value = (value_);								\

/* analog adjuster definition */
#define PORT_ADJUSTER(default,name)									\
	PORT_BIT(0x00ff, (default & 0xff) | (default << 8), IPT_ADJUSTER) PORT_NAME(name)



/*************************************
 *
 *  Helper macros
 *
 *************************************/

#define PORT_SERVICE(mask,default)	\
	PORT_BIT(    mask, mask & default, IPT_DIPSWITCH_NAME ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) PORT_TOGGLE	\
	PORT_DIPSETTING(    mask & default, DEF_STR( Off ) )	\
	PORT_DIPSETTING(    mask &~default, DEF_STR( On ) )

#define PORT_SERVICE_NO_TOGGLE(mask,default)	\
	PORT_BIT(    mask, mask & default, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode ))



/*************************************
 *
 *  Global variables
 *
 *************************************/

extern const char *input_port_default_strings[];

#define DEF_STR(str_num) (input_port_default_strings[STR_##str_num])



/*************************************
 *
 *  Function prototypes
 *
 *************************************/

int input_port_init(void (*construct_ipt)(input_port_init_params *));

input_port_entry *input_port_initialize(input_port_init_params *params, UINT32 type, const char *tag, UINT32 mask);
input_port_entry *input_port_allocate(void (*construct_ipt)(input_port_init_params *), input_port_entry *memory);
void input_port_parse_diplocation(input_port_entry *in, const char *location);

input_port_default_entry *get_input_port_list(void);
const input_port_default_entry *get_input_port_list_defaults(void);

int input_port_active(const input_port_entry *in);
int port_type_is_analog(int type);
int port_type_in_use(int type);
int port_type_to_group(int type, int player);
int port_tag_to_index(const char *tag);
read8_handler port_tag_to_handler8(const char *tag);
read16_handler port_tag_to_handler16(const char *tag);
read32_handler port_tag_to_handler32(const char *tag);
const char *input_port_name(const input_port_entry *in);
input_seq *input_port_seq(input_port_entry *in, int seqtype);
input_seq *input_port_default_seq(int type, int player, int seqtype);
int input_port_condition(const input_port_entry *in);
void input_port_set_changed_callback(int port, UINT32 mask, void (*callback)(void *, UINT32, UINT32), void *param);

const char *port_type_to_token(int type, int player);
int token_to_port_type(const char *string, int *player);

int input_port_type_pressed(int type, int player);
int input_ui_pressed(int code);
int input_ui_pressed_repeat(int code, int speed);

void input_port_update_defaults(void);
void input_port_vblank_start(void);	/* called by cpuintrf.c - not for external use */
void input_port_vblank_end(void);	/* called by cpuintrf.c - not for external use */

void input_port_set_digital_value(int port, UINT32 value, UINT32 mask);

UINT32 readinputport(int port);
UINT32 readinputportbytag(const char *tag);
UINT32 readinputportbytag_safe(const char *tag, UINT32 defvalue);

#endif	/* __INPTPORT_H__ */
