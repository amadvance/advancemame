/*
To do:
 - make single step work reliably
*/
/*********************************************************************

    usrintrf.c

    Functions used to handle MAME's user interface.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "info.h"
#include "vidhrdw/vector.h"
#include "ui_text.h"
#include "profiler.h"
#include "cheat.h"
#include <ctype.h>
#include <stdarg.h>
#include <math.h>

#ifdef MESS
#include "mess.h"
#include "mesintrf.h"
#include "inputx.h"
#endif



/*************************************
 *
 *  Constants
 *
 *************************************/

#define SEL_BITS				12
#define SEL_MASK				((1<<SEL_BITS)-1)

#define MENU_STACK_DEPTH		8
#define MENU_STRING_POOL_SIZE	(64*1024)

#define MAX_ANALOG_ENTRIES		80
#define MAX_DIP_SWITCHES		256
#define MAX_PORT_ENTRIES		1000
#define MAX_SETUPMENU_ITEMS		20
#define MAX_OSD_ITEMS			50

enum
{
	ANALOG_ITEM_KEYSPEED = 0,
	ANALOG_ITEM_CENTERSPEED,
	ANALOG_ITEM_REVERSE,
	ANALOG_ITEM_SENSITIVITY,
	ANALOG_ITEM_COUNT
};

enum
{
	LOADSAVE_NONE,
	LOADSAVE_LOAD,
	LOADSAVE_SAVE
};



/*************************************
 *
 *  Type definitions
 *
 *************************************/

typedef struct _input_item_data input_item_data;
struct _input_item_data
{
	input_seq *		seq;
	UINT16 			sortorder;
	UINT8 			digital;
};



/*************************************
 *
 *  Macros
 *
 *************************************/

#define UI_BOX_LR_BORDER		(ui_get_char_width(' ') / 2)
#define UI_BOX_TB_BORDER		(ui_get_char_width(' ') / 2)



/*************************************
 *
 *  Local variables
 *
 *************************************/

/* raw coordinates, relative to the real scrbitmap */
static rectangle uirawbounds;

/* rotated coordinates, easier to deal with */
static rectangle uirotbounds;
static int uirotwidth, uirotheight;
static int uirotcharwidth, uirotcharheight;

/* UI states */
static int therm_state;
static int load_save_state;

/* menu states */
static UINT32 menu_state;
static ui_menu_handler menu_handler;

static int menu_stack_index;
static UINT32 menu_stack_state[MENU_STACK_DEPTH];
static ui_menu_handler menu_stack_handler[MENU_STACK_DEPTH];

static UINT32 menu_string_pool_offset;
static char menu_string_pool[MENU_STRING_POOL_SIZE];


static int single_step;

static int showfps;
static int showfpstemp;

static int show_profiler;

static UINT8 ui_dirty;

static gfx_element *uirotfont;
static pen_t uirotfont_colortable[2*2];

static char popup_text[200];
static int popup_text_counter;


static void (*onscrd_fnc[MAX_OSD_ITEMS])(int increment,int arg);
static int onscrd_arg[MAX_OSD_ITEMS];
static int onscrd_total_items;


static input_seq starting_seq;



/* -- begin this stuff will go away with the new rendering system */
#define MAX_RENDER_ELEMENTS	1000

struct _render_element
{
	int x, y;
	int x2, y2;
	UINT16 type;
	rgb_t color;
};
typedef struct _render_element render_element;

static render_element elemlist[MAX_RENDER_ELEMENTS];
static int elemindex;
/* -- end this stuff will go away with the new rendering system */



/*************************************
 *
 *  Font data
 *
 *************************************/

static const UINT8 uifontdata[] =
{
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/* [ 0- 1] */
	0x7c,0x80,0x98,0x90,0x80,0xbc,0x80,0x7c,0xf8,0x04,0x64,0x44,0x04,0xf4,0x04,0xf8,	/* [ 2- 3] tape pos 1 */
	0x7c,0x80,0x98,0x88,0x80,0xbc,0x80,0x7c,0xf8,0x04,0x64,0x24,0x04,0xf4,0x04,0xf8,	/* [ 4- 5] tape pos 2 */
	0x7c,0x80,0x88,0x98,0x80,0xbc,0x80,0x7c,0xf8,0x04,0x24,0x64,0x04,0xf4,0x04,0xf8,	/* [ 6- 7] tape pos 3 */
	0x7c,0x80,0x90,0x98,0x80,0xbc,0x80,0x7c,0xf8,0x04,0x44,0x64,0x04,0xf4,0x04,0xf8,	/* [ 8- 9] tape pos 3 */
	0x30,0x48,0x84,0xb4,0xb4,0x84,0x48,0x30,0x30,0x48,0x84,0x84,0x84,0x84,0x48,0x30,	/* [10-11] */
	0x00,0xfc,0x84,0x8c,0xd4,0xa4,0xfc,0x00,0x00,0xfc,0x84,0x84,0x84,0x84,0xfc,0x00,	/* [12-13] */
	0x00,0x38,0x7c,0x7c,0x7c,0x38,0x00,0x00,0x00,0x30,0x68,0x78,0x78,0x30,0x00,0x00,	/* [14-15] circle & bullet */
	0x80,0xc0,0xe0,0xf0,0xe0,0xc0,0x80,0x00,0x04,0x0c,0x1c,0x3c,0x1c,0x0c,0x04,0x00,	/* [16-17] R/L triangle */
	0x20,0x70,0xf8,0x20,0x20,0xf8,0x70,0x20,0x48,0x48,0x48,0x48,0x48,0x00,0x48,0x00,
	0x00,0x00,0x30,0x68,0x78,0x30,0x00,0x00,0x00,0x30,0x68,0x78,0x78,0x30,0x00,0x00,
	0x70,0xd8,0xe8,0xe8,0xf8,0xf8,0x70,0x00,0x1c,0x7c,0x74,0x44,0x44,0x4c,0xcc,0xc0,
	0x20,0x70,0xf8,0x70,0x70,0x70,0x70,0x00,0x70,0x70,0x70,0x70,0xf8,0x70,0x20,0x00,
	0x00,0x10,0xf8,0xfc,0xf8,0x10,0x00,0x00,0x00,0x20,0x7c,0xfc,0x7c,0x20,0x00,0x00,
	0xb0,0x54,0xb8,0xb8,0x54,0xb0,0x00,0x00,0x00,0x28,0x6c,0xfc,0x6c,0x28,0x00,0x00,
	0x00,0x30,0x30,0x78,0x78,0xfc,0x00,0x00,0xfc,0x78,0x78,0x30,0x30,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,
	0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xf8,0x50,0xf8,0x50,0x00,0x00,
	0x20,0x70,0xc0,0x70,0x18,0xf0,0x20,0x00,0x40,0xa4,0x48,0x10,0x20,0x48,0x94,0x08,
	0x60,0x90,0xa0,0x40,0xa8,0x90,0x68,0x00,0x10,0x20,0x40,0x00,0x00,0x00,0x00,0x00,
	0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x00,0x10,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
	0x20,0xa8,0x70,0xf8,0x70,0xa8,0x20,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,
	0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0x30,0x10,0x10,0x10,0x10,0x10,0x00,
	0x70,0x88,0x08,0x10,0x20,0x40,0xf8,0x00,0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00,
	0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x00,0xf8,0x80,0xf0,0x08,0x08,0x88,0x70,0x00,
	0x70,0x80,0xf0,0x88,0x88,0x88,0x70,0x00,0xf8,0x08,0x08,0x10,0x20,0x20,0x20,0x00,
	0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x70,0x00,
	0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x00,0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x60,
	0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,
	0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00,
	0x30,0x48,0x94,0xa4,0xa4,0x94,0x48,0x30,0x70,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,
	0xf0,0x88,0x88,0xf0,0x88,0x88,0xf0,0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,
	0xf0,0x88,0x88,0x88,0x88,0x88,0xf0,0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,
	0xf8,0x80,0x80,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x80,0x98,0x88,0x88,0x70,0x00,
	0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
	0x08,0x08,0x08,0x08,0x88,0x88,0x70,0x00,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x00,
	0x80,0x80,0x80,0x80,0x80,0x80,0xf8,0x00,0x88,0xd8,0xa8,0x88,0x88,0x88,0x88,0x00,
	0x88,0xc8,0xa8,0x98,0x88,0x88,0x88,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
	0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x08,
	0xf0,0x88,0x88,0xf0,0x88,0x88,0x88,0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,
	0xf8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
	0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00,0x88,0x88,0x88,0x88,0xa8,0xd8,0x88,0x00,
	0x88,0x50,0x20,0x20,0x20,0x50,0x88,0x00,0x88,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
	0xf8,0x08,0x10,0x20,0x40,0x80,0xf8,0x00,0x30,0x20,0x20,0x20,0x20,0x20,0x30,0x00,
	0x40,0x40,0x20,0x20,0x10,0x10,0x08,0x08,0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00,
	0x20,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,
	0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,
	0x80,0x80,0xf0,0x88,0x88,0x88,0xf0,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x78,0x00,
	0x08,0x08,0x78,0x88,0x88,0x88,0x78,0x00,0x00,0x00,0x70,0x88,0xf8,0x80,0x78,0x00,
	0x18,0x20,0x70,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70,
	0x80,0x80,0xf0,0x88,0x88,0x88,0x88,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
	0x20,0x00,0x20,0x20,0x20,0x20,0x20,0xc0,0x80,0x80,0x90,0xa0,0xe0,0x90,0x88,0x00,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xf0,0xa8,0xa8,0xa8,0xa8,0x00,
	0x00,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,
	0x00,0x00,0xf0,0x88,0x88,0xf0,0x80,0x80,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08,
	0x00,0x00,0xb0,0xc8,0x80,0x80,0x80,0x00,0x00,0x00,0x78,0x80,0x70,0x08,0xf0,0x00,
	0x20,0x20,0x70,0x20,0x20,0x20,0x18,0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
	0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00,0x00,0x00,0xa8,0xa8,0xa8,0xa8,0x50,0x00,
	0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70,
	0x00,0x00,0xf8,0x10,0x20,0x40,0xf8,0x00,0x08,0x10,0x10,0x20,0x10,0x10,0x08,0x00,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x40,0x20,0x20,0x10,0x20,0x20,0x40,0x00,
	0x00,0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x20,0x50,0x20,0x50,0xa8,0x50,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x40,0x0c,0x10,0x38,0x10,0x20,0x20,0xc0,0x00,
	0x00,0x00,0x00,0x00,0x00,0x28,0x28,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0xa8,0x00,
	0x70,0xa8,0xf8,0x20,0x20,0x20,0x20,0x00,0x70,0xa8,0xf8,0x20,0x20,0xf8,0xa8,0x70,
	0x20,0x50,0x88,0x00,0x00,0x00,0x00,0x00,0x44,0xa8,0x50,0x20,0x68,0xd4,0x28,0x00,
	0x88,0x70,0x88,0x60,0x30,0x88,0x70,0x00,0x00,0x10,0x20,0x40,0x20,0x10,0x00,0x00,
	0x78,0xa0,0xa0,0xb0,0xa0,0xa0,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x20,0x20,0x00,0x00,0x00,0x00,0x00,
	0x10,0x10,0x20,0x00,0x00,0x00,0x00,0x00,0x28,0x50,0x50,0x00,0x00,0x00,0x00,0x00,
	0x28,0x28,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x78,0x78,0x30,0x00,0x00,
	0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,0x00,0x00,0x00,0x00,
	0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x00,0xf4,0x5c,0x54,0x54,0x00,0x00,0x00,0x00,
	0x88,0x70,0x78,0x80,0x70,0x08,0xf0,0x00,0x00,0x40,0x20,0x10,0x20,0x40,0x00,0x00,
	0x00,0x00,0x70,0xa8,0xb8,0xa0,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
	0x00,0x20,0x70,0xa8,0xa0,0xa8,0x70,0x20,0x30,0x48,0x40,0xe0,0x40,0x48,0xf0,0x00,
	0x00,0x48,0x30,0x48,0x48,0x30,0x48,0x00,0x88,0x88,0x50,0xf8,0x20,0xf8,0x20,0x00,
	0x20,0x20,0x20,0x00,0x20,0x20,0x20,0x00,0x78,0x80,0x70,0x88,0x70,0x08,0xf0,0x00,
	0xd8,0xd8,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x48,0x94,0xa4,0xa4,0x94,0x48,0x30,
	0x60,0x10,0x70,0x90,0x70,0x00,0x00,0x00,0x00,0x28,0x50,0xa0,0x50,0x28,0x00,0x00,
	0x00,0x00,0x00,0xf8,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x00,0x00,0x00,0x00,
	0x30,0x48,0xb4,0xb4,0xa4,0xb4,0x48,0x30,0x7c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x60,0x90,0x90,0x60,0x00,0x00,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0xf8,0x00,
	0x60,0x90,0x20,0x40,0xf0,0x00,0x00,0x00,0x60,0x90,0x20,0x90,0x60,0x00,0x00,0x00,
	0x10,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x88,0x88,0x88,0xc8,0xb0,0x80,
	0x78,0xd0,0xd0,0xd0,0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x10,0x20,0x00,0x20,0x60,0x20,0x20,0x70,0x00,0x00,0x00,
	0x20,0x50,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0x50,0x28,0x50,0xa0,0x00,0x00,
	0x40,0x48,0x50,0x28,0x58,0xa8,0x38,0x08,0x40,0x48,0x50,0x28,0x44,0x98,0x20,0x3c,
	0xc0,0x28,0xd0,0x28,0xd8,0xa8,0x38,0x08,0x20,0x00,0x20,0x40,0x80,0x88,0x70,0x00,
	0x40,0x20,0x70,0x88,0xf8,0x88,0x88,0x00,0x10,0x20,0x70,0x88,0xf8,0x88,0x88,0x00,
	0x70,0x00,0x70,0x88,0xf8,0x88,0x88,0x00,0x68,0xb0,0x70,0x88,0xf8,0x88,0x88,0x00,
	0x50,0x00,0x70,0x88,0xf8,0x88,0x88,0x00,0x20,0x50,0x70,0x88,0xf8,0x88,0x88,0x00,
	0x78,0xa0,0xa0,0xf0,0xa0,0xa0,0xb8,0x00,0x70,0x88,0x80,0x80,0x88,0x70,0x08,0x70,
	0x40,0x20,0xf8,0x80,0xf0,0x80,0xf8,0x00,0x10,0x20,0xf8,0x80,0xf0,0x80,0xf8,0x00,
	0x70,0x00,0xf8,0x80,0xf0,0x80,0xf8,0x00,0x50,0x00,0xf8,0x80,0xf0,0x80,0xf8,0x00,
	0x40,0x20,0x70,0x20,0x20,0x20,0x70,0x00,0x10,0x20,0x70,0x20,0x20,0x20,0x70,0x00,
	0x70,0x00,0x70,0x20,0x20,0x20,0x70,0x00,0x50,0x00,0x70,0x20,0x20,0x20,0x70,0x00,
	0x70,0x48,0x48,0xe8,0x48,0x48,0x70,0x00,0x68,0xb0,0x88,0xc8,0xa8,0x98,0x88,0x00,
	0x40,0x20,0x70,0x88,0x88,0x88,0x70,0x00,0x10,0x20,0x70,0x88,0x88,0x88,0x70,0x00,
	0x70,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x68,0xb0,0x70,0x88,0x88,0x88,0x70,0x00,
	0x50,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,
	0x00,0x74,0x88,0x90,0xa8,0x48,0xb0,0x00,0x40,0x20,0x88,0x88,0x88,0x88,0x70,0x00,
	0x10,0x20,0x88,0x88,0x88,0x88,0x70,0x00,0x70,0x00,0x88,0x88,0x88,0x88,0x70,0x00,
	0x50,0x00,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0xa8,0x88,0x50,0x20,0x20,0x20,0x00,
	0x00,0x80,0xf0,0x88,0x88,0xf0,0x80,0x80,0x60,0x90,0x90,0xb0,0x88,0x88,0xb0,0x00,
	0x40,0x20,0x70,0x08,0x78,0x88,0x78,0x00,0x10,0x20,0x70,0x08,0x78,0x88,0x78,0x00,
	0x70,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x68,0xb0,0x70,0x08,0x78,0x88,0x78,0x00,
	0x50,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x20,0x50,0x70,0x08,0x78,0x88,0x78,0x00,
	0x00,0x00,0xf0,0x28,0x78,0xa0,0x78,0x00,0x00,0x00,0x70,0x88,0x80,0x78,0x08,0x70,
	0x40,0x20,0x70,0x88,0xf8,0x80,0x70,0x00,0x10,0x20,0x70,0x88,0xf8,0x80,0x70,0x00,
	0x70,0x00,0x70,0x88,0xf8,0x80,0x70,0x00,0x50,0x00,0x70,0x88,0xf8,0x80,0x70,0x00,
	0x40,0x20,0x00,0x60,0x20,0x20,0x70,0x00,0x10,0x20,0x00,0x60,0x20,0x20,0x70,0x00,
	0x20,0x50,0x00,0x60,0x20,0x20,0x70,0x00,0x50,0x00,0x00,0x60,0x20,0x20,0x70,0x00,
	0x50,0x60,0x10,0x78,0x88,0x88,0x70,0x00,0x68,0xb0,0x00,0xf0,0x88,0x88,0x88,0x00,
	0x40,0x20,0x00,0x70,0x88,0x88,0x70,0x00,0x10,0x20,0x00,0x70,0x88,0x88,0x70,0x00,
	0x20,0x50,0x00,0x70,0x88,0x88,0x70,0x00,0x68,0xb0,0x00,0x70,0x88,0x88,0x70,0x00,
	0x00,0x50,0x00,0x70,0x88,0x88,0x70,0x00,0x00,0x20,0x00,0xf8,0x00,0x20,0x00,0x00,
	0x00,0x00,0x68,0x90,0xa8,0x48,0xb0,0x00,0x40,0x20,0x88,0x88,0x88,0x98,0x68,0x00,
	0x10,0x20,0x88,0x88,0x88,0x98,0x68,0x00,0x70,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
	0x50,0x00,0x88,0x88,0x88,0x98,0x68,0x00,0x10,0x20,0x88,0x88,0x88,0x78,0x08,0x70,
	0x80,0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x50,0x00,0x88,0x88,0x88,0x78,0x08,0x70
};

#define MAX_UIFONT_SIZE 8 /* max(width,height) */
static const gfx_layout uifontlayout =
{
	6,8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};



/*************************************
 *
 *  Local prototypes
 *
 *************************************/

static void draw_multiline_text_box(const char *text, int justify, float xpos, float ypos);
static void create_font(void);
static void onscrd_init(void);

static void handle_keys(mame_bitmap *bitmap);
static void ui_display_profiler(void);
static void ui_display_popup(void);
static int setup_menu(int selected);
static int on_screen_display(int selected);
static void showcharset(mame_bitmap *bitmap);
static void initiate_load_save(int type);
static int update_load_save(void);

static UINT32 menu_main(UINT32 state);
static UINT32 menu_default_input_groups(UINT32 state);
static UINT32 menu_default_input(UINT32 state);
static UINT32 menu_game_input(UINT32 state);
static UINT32 menu_switches(UINT32 state);
static UINT32 menu_analog(UINT32 state);
static UINT32 menu_joystick_calibrate(UINT32 state);
static UINT32 menu_cheat(UINT32 state);
static UINT32 menu_memory_card(UINT32 state);
static UINT32 menu_reset_game(UINT32 state);

#ifndef MESS
static UINT32 menu_bookkeeping(UINT32 state);
static UINT32 menu_game_info(UINT32 state);
#else
static UINT32 menu_file_manager(UINT32 state);
static UINT32 menu_tape_control(UINT32 state);
#endif

static int sprintf_game_info(char *buf);


/* -- begin this stuff will go away with the new rendering system */
static void ui_raw2rot_rect(rectangle *rect);
static void ui_rot2raw_rect(rectangle *rect);
static void add_line(int x1, int y1, int x2, int y2, rgb_t color);
static void add_fill(int left, int top, int right, int bottom, rgb_t color);
static void add_char(int x, int y, UINT16 ch, int color);
static void add_filled_box(int x1, int y1, int x2, int y2);
static void render_ui(mame_bitmap *dest);
/* -- end this stuff will go away with the new rendering system */



/*************************************
 *
 *  Main initialization
 *
 *************************************/

int ui_init(int show_disclaimer, int show_warnings, int show_gameinfo)
{
	/* load the localization file */
	if (uistring_init(options.language_file) != 0)
		fatalerror("uistring_init failed");

	/* build up the font */
	create_font();

	/* clear the input memory */
	while (code_read_async() != CODE_NONE) ;

	/* initialize the menu state */
	ui_menu_stack_reset();

	/* initialize the on-screen display system */
	onscrd_init();
	therm_state = 0;

	/* reset globals */
	single_step = FALSE;
	load_save_state = LOADSAVE_NONE;

	add_exit_callback(ui_exit);

	/* disable artwork for the start */
	artwork_enable(FALSE);

	/* before doing anything else, update the video and audio system once */
	update_video_and_audio();

	/* if we didn't find a settings file, show the disclaimer */
	if (show_disclaimer && ui_display_copyright(artwork_get_ui_bitmap()) != 0)
		return 1;

	/* show info about incorrect behaviour (wrong colors etc.) */
	if (show_warnings && ui_display_game_warnings(artwork_get_ui_bitmap()) != 0)
		return 1;

	/* show info about the game */
	if (show_gameinfo && ui_display_game_info(artwork_get_ui_bitmap()) != 0)
		return 1;

	/* enable artwork now */
	artwork_enable(TRUE);

	return 0;
}



/*************************************
 *
 *  Clean up
 *
 *************************************/

void ui_exit(void)
{
	if (uirotfont)
		freegfx(uirotfont);
	uirotfont = NULL;
}



/*************************************
 *
 *  Set the UI visible area
 *  (called by OSD layer, will go
 *  away with new rendering system)
 *
 *************************************/

void ui_set_visible_area(int xmin, int ymin, int xmax, int ymax)
{
	/* fill in the rect */
	uirawbounds.min_x = xmin;
	uirawbounds.min_y = ymin;
	uirawbounds.max_x = xmax;
	uirawbounds.max_y = ymax;

	/* orient it */
	uirotbounds = uirawbounds;
	ui_raw2rot_rect(&uirotbounds);

	/* make some easier-to-access globals */
	uirotwidth = uirotbounds.max_x - uirotbounds.min_x + 1;
	uirotheight = uirotbounds.max_y - uirotbounds.min_y + 1;

	/* rebuild the font */
	create_font();
}



/*************************************
 *
 *  Update and rendering frontend
 *
 *************************************/

void ui_update_and_render(mame_bitmap *bitmap)
{
	/* if we're single-stepping, pause now */
	if (single_step)
	{
		mame_pause(TRUE);
		single_step = FALSE;
	}

	/* first display the FPS counter and profiler */
	ui_display_fps();
	ui_display_profiler();

	/* if the load/save display is live, that has focus */
	if (load_save_state != LOADSAVE_NONE)
		update_load_save();

	/* otherwise if menus are displayed, they have focus */
	else if (menu_handler != NULL)
	{
		if (input_ui_pressed(IPT_UI_CONFIGURE))
			menu_state = ui_menu_stack_push(NULL, 0);
		else
			menu_state = (*menu_handler)(menu_state);
	}

	/* otherwise, we handle non-menu cases */
	else
	{
		if (therm_state)
			therm_state = on_screen_display(therm_state);
		handle_keys(bitmap);
	}

	/* then let the cheat engine display its stuff */
	if (options.cheat)
		cheat_display_watches();

#ifdef MESS
	/* let MESS display its stuff */
	mess_ui_update();
#endif

	/* finally, display any popup messages */
	ui_display_popup();

	/* flush the UI to the bitmap */
	render_ui(bitmap);

	/* decrement the dirty count */
	if (ui_dirty)
		ui_dirty--;
}



/*************************************
 *
 *  Accessors for global variables
 *
 *************************************/

int ui_is_dirty(void)
{
	return ui_dirty;
}


int ui_is_onscrd_active(void)
{
	return (therm_state != 0);
}


int ui_is_setup_active(void)
{
	return (menu_handler != NULL);
}



/*************************************
 *
 *  Accessors for rendering
 *
 *************************************/

void ui_get_bounds(int *width, int *height)
{
	*width = uirotwidth;
	*height = uirotheight;
}


int ui_get_line_height(void)
{
	return uirotcharheight + 2;
}


int ui_get_char_width(UINT16 ch)
{
	return uirotcharwidth;
}


int ui_get_string_width(const char *s)
{
	return strlen(s) * uirotcharwidth;
}



/*************************************
 *
 *  Enable/disable FPS display
 *
 *************************************/

void ui_show_fps_temp(double seconds)
{
	if (!showfps)
		showfpstemp = (int)(seconds * Machine->refresh_rate);
}


void ui_set_show_fps(int show)
{
	showfps = show;
	if (!show)
	{
		showfps = 0;
		showfpstemp = 0;
		schedule_full_refresh();
	}
}


int ui_get_show_fps(void)
{
	return showfps || showfpstemp;
}



/*************************************
 *
 *  Enable/disable profiler display
 *
 *************************************/

void ui_set_show_profiler(int show)
{
	if (show)
	{
		show_profiler = 1;
		profiler_start();
	}
	else
	{
		show_profiler = 0;
		profiler_stop();
		schedule_full_refresh();
	}
}


int ui_get_show_profiler(void)
{
	return show_profiler;
}



/*************************************
 *
 *  Popup message registration
 *
 *************************************/

void CLIB_DECL ui_popup(const char *text,...)
{
	int seconds;
	va_list arg;
	va_start(arg,text);
	vsprintf(popup_text,text,arg);
	va_end(arg);
	seconds = strlen(popup_text) / 40 + 2;
	popup_text_counter = seconds * Machine->refresh_rate;
}


void CLIB_DECL ui_popup_time(int seconds, const char *text,...)
{
	va_list arg;
	va_start(arg,text);
	vsprintf(popup_text,text,arg);
	va_end(arg);
	popup_text_counter = seconds * Machine->refresh_rate;
}



/*************************************
 *
 *  Simple text renderer
 *
 *************************************/

void ui_draw_text(const char *buf, int x, int y)
{
	int ui_width, ui_height;
	ui_get_bounds(&ui_width, &ui_height);
	ui_draw_text_full(buf, x, y, ui_width - x, JUSTIFY_LEFT, WRAP_WORD, DRAW_OPAQUE, RGB_WHITE, RGB_BLACK, NULL, NULL);
}



/*************************************
 *
 *  Full featured text renderer
 *
 *************************************/

void ui_draw_text_full(const char *s, int x, int y, int wrapwidth, int justify, int wrap, int draw, rgb_t fgcolor, rgb_t bgcolor, int *totalwidth, int *totalheight)
{
	const char *linestart;
	int cury = y;
	int maxwidth = 0;

	/* if we don't want wrapping, guarantee a huge wrapwidth */
	if (wrap == WRAP_NEVER)
		wrapwidth = 1 << 30;

	/* loop over lines */
	while (*s)
	{
		const char *lastspace = NULL;
		int line_justify = justify;
		int lastspace_width = 0;
		int curwidth = 0;
		int curx = x;

		/* if the line starts with a tab character, center it regardless */
		if (*s == '\t')
		{
			s++;
			line_justify = JUSTIFY_CENTER;
		}

		/* remember the starting position of the line */
		linestart = s;

		/* loop while we have characters and are less than the wrapwidth */
		while (*s && curwidth <= wrapwidth)
		{
			/* if we hit a space, remember the location and the width there */
			if (*s == ' ')
			{
				lastspace = s;
				lastspace_width = curwidth;
			}

			/* if we hit a newline, stop immediately */
			else if (*s == '\n')
				break;

			/* add the width of this character and advance */
			curwidth += ui_get_char_width(*s);
			s++;
		}

		/* if we accumulated too much for the current width, we need to back off */
		if (curwidth > wrapwidth)
		{
			/* if we're word wrapping, back up to the last space if we can */
			if (wrap == WRAP_WORD)
			{
				/* if we hit a space, back up to there with the appropriate width */
				if (lastspace)
				{
					s = lastspace;
					curwidth = lastspace_width;
				}

				/* if we didn't hit a space, back up one character */
				else if (s > linestart)
				{
					s--;
					curwidth -= ui_get_char_width(*s);
				}
			}

			/* if we're truncating, make sure we have enough space for the ... */
			else if (wrap == WRAP_TRUNCATE)
			{
				/* add in the width of the ... */
				curwidth += 3 * ui_get_char_width('.');

				/* while we are above the wrap width, back up one character */
				while (curwidth > wrapwidth && s > linestart)
				{
					s--;
					curwidth -= ui_get_char_width(*s);
				}
			}
		}

		/* align according to the justfication */
		if (line_justify == JUSTIFY_CENTER)
			curx += (wrapwidth - curwidth) / 2;
		else if (line_justify == JUSTIFY_RIGHT)
			curx += wrapwidth - curwidth;

		/* track the maximum width of any given line */
		if (curwidth > maxwidth)
			maxwidth = curwidth;

		/* if opaque, add a black box */
		if (draw == DRAW_OPAQUE)
			add_fill(curx, cury, curx + curwidth - 1, cury + ui_get_line_height() - 1, bgcolor);

		/* loop from the line start and add the characters */
		while (linestart < s)
		{
			if (draw != DRAW_NONE)
			{
				add_char(curx, cury, *linestart, fgcolor);
				curx += ui_get_char_width(*linestart);
			}
			linestart++;
		}

		/* append ellipses if needed */
		if (wrap == WRAP_TRUNCATE && *s != 0 && draw != DRAW_NONE)
		{
			add_char(curx, cury, '.', fgcolor);
			curx += ui_get_char_width('.');
			add_char(curx, cury, '.', fgcolor);
			curx += ui_get_char_width('.');
			add_char(curx, cury, '.', fgcolor);
			curx += ui_get_char_width('.');
		}

		/* if we're not word-wrapping, we're done */
		if (wrap != WRAP_WORD)
			break;

		/* advance by a row */
		cury += ui_get_line_height();

		/* skip past any spaces at the beginning of the next line */
		if (*s == '\n')
			s++;
		else
			while (*s && isspace(*s)) s++;
	}

	/* report the width and height of the resulting space */
	if (totalwidth)
		*totalwidth = maxwidth;
	if (totalheight)
		*totalheight = cury - y;
}



/*************************************
 *
 *  Menu rendering
 *
 *************************************/

void ui_draw_menu(const ui_menu_item *items, int numitems, int selected)
{
	const char *up_arrow = ui_getstring(UI_uparrow);
	const char *down_arrow = ui_getstring(UI_downarrow);
	const char *left_arrow = ui_getstring(UI_leftarrow);
	const char *right_arrow = ui_getstring(UI_rightarrow);
	const char *left_hilight = ui_getstring(UI_lefthilight);
	const char *right_hilight = ui_getstring(UI_righthilight);

	int left_hilight_width = ui_get_string_width(left_hilight);
	int right_hilight_width = ui_get_string_width(right_hilight);
	int left_arrow_width = ui_get_string_width(left_arrow);
	int space_width = ui_get_char_width(' ');
	int line_height = ui_get_line_height();

	int effective_width, effective_left;
	int visible_width, visible_height;
	int visible_top, visible_left;
	int ui_width, ui_height;
	int selected_subitem_too_big = 0;
	int visible_lines;
	int top_line;
	int itemnum, linenum;

	/* start with the bounds */
	ui_get_bounds(&ui_width, &ui_height);

	/* compute the width and height of the full menu */
	visible_width = 0;
	visible_height = 0;
	for (itemnum = 0; itemnum < numitems; itemnum++)
	{
		const ui_menu_item *item = &items[itemnum];
		int total_width;

		/* compute width of left hand side */
		total_width = left_hilight_width + ui_get_string_width(item->text) + right_hilight_width;

		/* add in width of right hand side */
		if (item->subtext)
			total_width += 2 * space_width + ui_get_string_width(item->subtext);

		/* track the maximum */
		if (total_width > visible_width)
			visible_width = total_width;

		/* track the height as well */
		visible_height += line_height;
	}

	/* if we are too wide or too tall, clamp it down */
	if (visible_width + 2 * UI_BOX_LR_BORDER > ui_width)
		visible_width = ui_width - 2 * UI_BOX_LR_BORDER;
	if (visible_height + 2 * UI_BOX_TB_BORDER > ui_height)
		visible_height = ui_height - 2 * UI_BOX_TB_BORDER;
	visible_lines = visible_height / line_height;
	visible_height = visible_lines * line_height;

	/* compute top/left of inner menu area by centering */
	visible_left = (ui_width - visible_width) / 2;
	visible_top = (ui_height - visible_height) / 2;

	/* first add us a box */
	add_filled_box(	visible_left - UI_BOX_LR_BORDER,
					visible_top - UI_BOX_TB_BORDER,
					visible_left + visible_width - 1 + UI_BOX_LR_BORDER,
					visible_top + visible_height - 1 + UI_BOX_TB_BORDER);

	/* determine the first visible line based on the current selection */
	top_line = selected - visible_lines / 2;
	if (top_line < 0)
		top_line = 0;
	if (top_line + visible_lines >= numitems)
		top_line = numitems - visible_lines;

	/* determine effective positions taking into account the hilighting arrows */
	effective_width = visible_width - left_hilight_width - right_hilight_width;
	effective_left = visible_left + left_hilight_width;

	/* loop over visible lines */
	for (linenum = 0; linenum < visible_lines; linenum++)
	{
		int line_y = visible_top + linenum * line_height;
		int itemnum = top_line + linenum;
		const ui_menu_item *item = &items[itemnum];

		/* if we're on the top line, display the up arrow */
		if (linenum == 0 && top_line != 0)
			ui_draw_text_full(up_arrow, effective_left, line_y, effective_width,
						JUSTIFY_CENTER, WRAP_TRUNCATE, DRAW_NORMAL, RGB_WHITE, RGB_BLACK, NULL, NULL);

		/* if we're on the bottom line, display the down arrow */
		else if (linenum == visible_lines - 1 && itemnum != numitems - 1)
			ui_draw_text_full(down_arrow, effective_left, line_y, effective_width,
						JUSTIFY_CENTER, WRAP_TRUNCATE, DRAW_NORMAL, RGB_WHITE, RGB_BLACK, NULL, NULL);

		/* if we don't have a subitem, just draw the string centered */
		else if (!item->subtext)
			ui_draw_text_full(item->text, effective_left, line_y, effective_width,
						JUSTIFY_CENTER, WRAP_TRUNCATE, DRAW_NORMAL, RGB_WHITE, RGB_BLACK, NULL, NULL);

		/* otherwise, draw the item on the left and the subitem text on the right */
		else
		{
			int subitem_invert = item->flags & MENU_FLAG_INVERT;
			const char *subitem_text = item->subtext;
			int item_width, subitem_width;

			/* draw the left-side text */
			ui_draw_text_full(item->text, effective_left, line_y, effective_width,
						JUSTIFY_LEFT, WRAP_TRUNCATE, DRAW_NORMAL, RGB_WHITE, RGB_BLACK, &item_width, NULL);

			/* give 2 spaces worth of padding */
			item_width += 2 * space_width;

			/* if the subitem doesn't fit here, display dots */
			if (ui_get_string_width(subitem_text) > effective_width - item_width)
			{
				subitem_text = "...";
				if (itemnum == selected)
					selected_subitem_too_big = 1;
			}

			/* draw the subitem right-justified */
			ui_draw_text_full(subitem_text, effective_left + item_width, line_y, effective_width - item_width,
						JUSTIFY_RIGHT, WRAP_TRUNCATE, DRAW_OPAQUE, subitem_invert ? RGB_BLACK : RGB_WHITE, subitem_invert ? RGB_WHITE : RGB_BLACK, &subitem_width, NULL);

			/* apply arrows */
			if (itemnum == selected && (item->flags & MENU_FLAG_LEFT_ARROW))
				ui_draw_text_full(left_arrow, effective_left + effective_width - subitem_width - left_arrow_width, line_y, left_arrow_width,
							JUSTIFY_LEFT, WRAP_NEVER, DRAW_NORMAL, RGB_WHITE, RGB_BLACK, NULL, NULL);
			if (itemnum == selected && (item->flags & MENU_FLAG_RIGHT_ARROW))
				ui_draw_text_full(right_arrow, visible_left, line_y, visible_width,
							JUSTIFY_RIGHT, WRAP_TRUNCATE, DRAW_NORMAL, RGB_WHITE, RGB_BLACK, NULL, NULL);
		}

		/* draw the arrows for selected items */
		if (itemnum == selected)
		{
			ui_draw_text_full(left_hilight, visible_left, line_y, visible_width,
						JUSTIFY_LEFT, WRAP_TRUNCATE, DRAW_NORMAL, RGB_WHITE, RGB_BLACK, NULL, NULL);
			if (!(item->flags & (MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW)))
				ui_draw_text_full(right_hilight, visible_left, line_y, visible_width,
							JUSTIFY_RIGHT, WRAP_TRUNCATE, DRAW_NORMAL, RGB_WHITE, RGB_BLACK, NULL, NULL);
		}
	}

	/* if the selected subitem is too big, display it in a separate offset box */
	if (selected_subitem_too_big)
	{
		const ui_menu_item *item = &items[selected];
		int linenum = selected - top_line;
		int line_y = visible_top + linenum * line_height;
		int target_width, target_height;
		int target_x, target_y;

		/* compute the multi-line target width/height */
		ui_draw_text_full(item->subtext, 0, 0, visible_width * 3 / 4,
					JUSTIFY_RIGHT, WRAP_WORD, DRAW_NONE, RGB_WHITE, RGB_BLACK, &target_width, &target_height);

		/* determine the target location */
		target_x = visible_left + visible_width - target_width - UI_BOX_LR_BORDER;
		target_y = line_y + line_height + UI_BOX_TB_BORDER;
		if (target_y + target_height + UI_BOX_TB_BORDER > visible_height)
			target_y = line_y - target_height - UI_BOX_TB_BORDER;

		/* add a box around that */
		add_filled_box(	target_x - UI_BOX_LR_BORDER,
						target_y - UI_BOX_TB_BORDER,
						target_x + target_width - 1 + UI_BOX_LR_BORDER,
						target_y + target_height - 1 + UI_BOX_TB_BORDER);
		ui_draw_text_full(item->subtext, target_x, target_y, target_width,
					JUSTIFY_RIGHT, WRAP_WORD, DRAW_NORMAL, RGB_WHITE, RGB_BLACK, NULL, NULL);
	}
}



/*************************************
 *
 *  Menu management
 *
 *************************************/

void ui_menu_stack_reset(void)
{
	menu_handler = NULL;
	menu_state = 0;
	menu_stack_index = 0;
}


UINT32 ui_menu_stack_push(ui_menu_handler new_handler, UINT32 new_state)
{
	if (menu_stack_index >= MENU_STACK_DEPTH)
		fatalerror("Menu stack overflow!");

	/* save the old state/handler */
	menu_stack_handler[menu_stack_index] = menu_handler;
	menu_stack_state[menu_stack_index] = menu_state;
	menu_stack_index++;

	/* set the new ones */
	menu_handler = new_handler;
	menu_state = new_state;

	/* force a refresh */
	schedule_full_refresh();
	return new_state;
}


UINT32 ui_menu_stack_pop(void)
{
	if (menu_stack_index <= 0)
		fatalerror("Menu stack underflow!");

	/* restore the old state/handler */
	menu_stack_index--;
	menu_handler = menu_stack_handler[menu_stack_index];

	/* force a refresh */
	schedule_full_refresh();

	return menu_stack_state[menu_stack_index];
}



/*************************************
 *
 *  Generic menu keys
 *
 *************************************/

int ui_menu_generic_keys(int *selected, int num_items)
{
	/* hitting cancel or selecting the last item returns to the previous menu */
	if (input_ui_pressed(IPT_UI_CANCEL) || (*selected == num_items - 1 && input_ui_pressed(IPT_UI_SELECT)))
	{
		*selected = ui_menu_stack_pop();
		return 1;
	}

	/* up backs up by one item */
	if (input_ui_pressed_repeat(IPT_UI_UP, 6))
		*selected = (*selected + num_items - 1) % num_items;

	/* down advances by one item */
	if (input_ui_pressed_repeat(IPT_UI_DOWN, 6))
		*selected = (*selected + 1) % num_items;

	/* home goes to the start */
	if (input_ui_pressed(IPT_UI_HOME))
		*selected = 0;

	/* end goes to the last */
	if (input_ui_pressed(IPT_UI_END))
		*selected = num_items - 1;

	return 0;
}



/*************************************
 *
 *  Multiline message box rendering
 *
 *************************************/

static void draw_multiline_text_box(const char *text, int justify, float xpos, float ypos)
{
	int target_width, target_height;
	int ui_width, ui_height;
	int target_x, target_y;

	/* start with the bounds */
	ui_get_bounds(&ui_width, &ui_height);

	/* compute the multi-line target width/height */
	ui_draw_text_full(text, 0, 0, ui_width - 2 * UI_BOX_LR_BORDER,
				justify, WRAP_WORD, DRAW_NONE, RGB_WHITE, RGB_BLACK, &target_width, &target_height);
	if (target_height > ui_height - 2 * UI_BOX_TB_BORDER)
		target_height = ((ui_height - 2 * UI_BOX_TB_BORDER) / ui_get_line_height()) * ui_get_line_height();

	/* determine the target location */
	target_x = (int)(xpos * ui_width) - target_width / 2;
	target_y = (int)(ypos * ui_height) - target_height / 2;

	/* make sure we stay on-screen */
	if (target_x < UI_BOX_LR_BORDER)
		target_x = UI_BOX_LR_BORDER;
	if (target_x + target_width + UI_BOX_LR_BORDER > ui_width)
		target_x = ui_width - UI_BOX_LR_BORDER - target_width;
	if (target_y < UI_BOX_TB_BORDER)
		target_y = UI_BOX_TB_BORDER;
	if (target_y + target_height + UI_BOX_TB_BORDER > ui_height)
		target_y = ui_height - UI_BOX_TB_BORDER - target_height;

	/* add a box around that */
	add_filled_box(	target_x - UI_BOX_LR_BORDER,
					target_y - UI_BOX_TB_BORDER,
					target_x + target_width - 1 + UI_BOX_LR_BORDER,
					target_y + target_height - 1 + UI_BOX_TB_BORDER);
	ui_draw_text_full(text, target_x, target_y, target_width,
				justify, WRAP_WORD, DRAW_NORMAL, RGB_WHITE, RGB_BLACK, NULL, NULL);
}


void ui_draw_message_window(const char *text)
{
	draw_multiline_text_box(text, JUSTIFY_LEFT, 0.5, 0.5);
}



/*************************************
 *
 *  Create the UI font
 *
 *************************************/

static void create_font(void)
{
	gfx_layout layout = uifontlayout;
	int temp, i;

	/* free any existing fonts */
	if (uirotfont)
		freegfx(uirotfont);

	/* pixel double horizontally */
	if (uirotwidth >= 420)
	{
		for (i = 0; i < layout.width; i++)
			layout.xoffset[i*2+0] = layout.xoffset[i*2+1] = uifontlayout.xoffset[i];
		layout.width *= 2;
	}

	/* pixel double vertically */
	if (uirotheight >= 420)
	{
		for (i = 0; i < layout.height; i++)
			layout.yoffset[i*2+0] = layout.yoffset[i*2+1] = uifontlayout.yoffset[i];
		layout.height *= 2;
	}

	/* apply swappage */
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		for (i=0; i<2*MAX_UIFONT_SIZE; i++)
			temp = layout.xoffset[i], layout.xoffset[i] = layout.yoffset[i], layout.yoffset[i] = temp;

		temp = layout.width;
		layout.width = layout.height;
		layout.height = temp;
	}

	/* apply xflip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		for (i = 0; i < layout.width/2; i++)
			temp = layout.xoffset[i], layout.xoffset[i] = layout.xoffset[layout.width - 1 - i], layout.xoffset[layout.width - 1 - i] = temp;
	}

	/* apply yflip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		for (i = 0; i < layout.height/2; i++)
			temp = layout.yoffset[i], layout.yoffset[i] = layout.yoffset[layout.height - 1 - i], layout.yoffset[layout.height - 1 - i] = temp;
	}

	/* decode rotated font */
	uirotfont = allocgfx(&layout);
	if (!uirotfont)
		fatalerror("Fatal error: could not allocate memory for UI font!");
	decodegfx(uirotfont, uifontdata, 0, uirotfont->total_elements);

	/* set the raw and rotated character width/height */
	uirotcharwidth = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? layout.height : layout.width;
	uirotcharheight = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? layout.width : layout.height;

	/* set up the bogus colortable */
	if (uirotfont)
	{
		/* colortable will be set at run time */
		uirotfont->colortable = uirotfont_colortable;
		uirotfont->total_colors = 2;
	}
}



/*************************************
 *
 *  Keyboard handling
 *
 *************************************/

static void handle_keys(mame_bitmap *bitmap)
{
#ifdef MESS
	if (options.disable_normal_ui || ((Machine->gamedrv->flags & GAME_COMPUTER) && !mess_ui_active()))
		return;
#endif

	/* if the user pressed ESC, stop the emulation as long as menus aren't up */
	if (menu_handler == NULL && input_ui_pressed(IPT_UI_CANCEL))
		mame_schedule_exit();

	/* if menus aren't up and the user has toggled them, turn them on */
	if (menu_handler == NULL && input_ui_pressed(IPT_UI_CONFIGURE))
	{
		/* if we have no menus stacked up, start with the main menu */
		if (menu_stack_index == 0)
			ui_menu_stack_push(menu_main, 0);

		/* otherwise, pop the previous menu from the stack */
		else
			menu_state = ui_menu_stack_pop();

		/* kill the thermometer view */
		therm_state = 0;
	}

	/* if the on-screen display isn't up and the user has toggled it, turn it on */
#ifdef MAME_DEBUG
	if (!Machine->debug_mode)
#endif
		if (therm_state == 0 && input_ui_pressed(IPT_UI_ON_SCREEN_DISPLAY))
			therm_state = -1;

	/* handle a reset request */
	if (input_ui_pressed(IPT_UI_RESET_MACHINE))
		mame_schedule_hard_reset();
	if (input_ui_pressed(IPT_UI_SOFT_RESET))
		mame_schedule_soft_reset();

	/* handle a request to display graphics/palette (note that this loops internally) */
	if (input_ui_pressed(IPT_UI_SHOW_GFX))
	{
		osd_sound_enable(0);
		showcharset(bitmap);
		osd_sound_enable(1);
	}

	/* handle a save state request */
	if (input_ui_pressed(IPT_UI_SAVE_STATE))
		initiate_load_save(LOADSAVE_SAVE);

	/* handle a load state request */
	if (input_ui_pressed(IPT_UI_LOAD_STATE))
		initiate_load_save(LOADSAVE_LOAD);

	/* handle a save snapshot request */
	if (input_ui_pressed(IPT_UI_SNAPSHOT))
		save_screen_snapshot(bitmap);

	/* toggle pause */
	if (input_ui_pressed(IPT_UI_PAUSE))
	{
		/* with a shift key, it is single step */
		if (mame_is_paused() && (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT)))
		{
			single_step = TRUE;
			mame_pause(FALSE);

			/* bit of a kludge to prevent flash */
			palette_set_global_brightness_adjust(options.pause_bright);
		}
		else
			mame_pause(!mame_is_paused());
	}

	/* toggle movie recording */
	if (input_ui_pressed(IPT_UI_RECORD_MOVIE))
		record_movie_toggle();

	/* toggle profiler display */
	if (input_ui_pressed(IPT_UI_SHOW_PROFILER))
		ui_set_show_profiler(!ui_get_show_profiler());

	/* toggle FPS display */
	if (input_ui_pressed(IPT_UI_SHOW_FPS))
		ui_set_show_fps(!ui_get_show_fps());

	/* toggle crosshair display */
	if (input_ui_pressed(IPT_UI_TOGGLE_CROSSHAIR))
		drawgfx_toggle_crosshair();
}



/*************************************
 *
 *  Main menu
 *
 *************************************/

static UINT32 menu_main(UINT32 state)
{
#define ADD_MENU(name, handler, param) \
do { \
	item_list[menu_items].text = ui_getstring(name); \
	handler_list[menu_items] = handler; \
	param_list[menu_items] = param; \
	menu_items++; \
} while (0)

	ui_menu_handler handler_list[20];
	ui_menu_item item_list[20];
	UINT32 param_list[20];
	int has_categories = FALSE;
	int has_configs = FALSE;
	int has_analog = FALSE;
	int has_dips = FALSE;
	input_port_entry *in;
	int menu_items = 0;

	/* scan the input port array to see what options we need to enable */
	for (in = Machine->input_ports; in->type != IPT_END; in++)
		if (input_port_active(in))
		{
			if (in->type == IPT_DIPSWITCH_NAME)
				has_dips = TRUE;
			if (port_type_is_analog(in->type))
				has_analog = TRUE;
			if (in->type == IPT_CONFIG_NAME)
				has_configs = TRUE;
			if (in->category > 0)
				has_categories = TRUE;
		}

	/* reset the menu */
	memset(item_list, 0, sizeof(item_list));

	/* add default input item */
	ADD_MENU(UI_inputgeneral, menu_default_input_groups, 0);

	/* add game-specific input item */
	ADD_MENU(UI_inputspecific, menu_game_input, 0);

	/* add DIP switches menu */
	if (has_dips)
		ADD_MENU(UI_dipswitches, menu_switches, (IPT_DIPSWITCH_NAME << 16) | (IPT_DIPSWITCH_SETTING << 24));

#ifdef MESS
	/* add configurables menu */
	if (has_configs)
		ADD_MENU(UI_configuration, menu_switches, (IPT_CONFIG_NAME << 16) | (IPT_CONFIG_SETTING << 24));

	/* add categories menu */
	if (has_categories)
		ADD_MENU(UI_categories, menu_switches, (IPT_CATEGORY_NAME << 16) | (IPT_CATEGORY_SETTING << 24));
#endif /* MESS */

	/* add analog settings menu */
	if (has_analog)
		ADD_MENU(UI_analogcontrols, menu_analog, 0);

  	/* add joystick calibration menu */
  	if (osd_joystick_needs_calibration())
		ADD_MENU(UI_calibrate, menu_joystick_calibrate, 0);

#ifndef MESS
  	/* add bookkeeping menu */
	ADD_MENU(UI_bookkeeping, menu_bookkeeping, 0);

	/* add game info menu */
	ADD_MENU(UI_gameinfo, menu_game_info, 0);
#else /* MESS */
  	/* add image info menu */
	ADD_MENU(UI_imageinfo, ui_menu_image_info, 0);

  	/* add image info menu */
	ADD_MENU(UI_filemanager, menu_file_manager, 1);

#if HAS_WAVE
  	/* add tape control menu */
	if (device_find(Machine->devices, IO_CASSETTE))
		ADD_MENU(UI_tapecontrol, menu_tape_control, 1);
#endif /* HAS_WAVE */
#endif /* !MESS */

	/* add cheat menu */
	if (options.cheat)
		ADD_MENU(UI_cheat, menu_cheat, 1);

	/* add memory card menu */
	if (Machine->drv->memcard_handler != NULL)
		ADD_MENU(UI_memorycard, menu_memory_card, 0);

	/* add reset and exit menus */
	ADD_MENU(UI_resetgame, menu_reset_game, 0);
	ADD_MENU(UI_returntogame, NULL, 0);

	/* draw the menu */
	ui_draw_menu(item_list, menu_items, state);

	/* handle the keys */
	if (ui_menu_generic_keys((int *) &state, menu_items))
		return state;
	if (input_ui_pressed(IPT_UI_SELECT))
		return ui_menu_stack_push(handler_list[menu_state], param_list[menu_state]);

	return state;

#undef ADD_MENU
}



/*************************************
 *
 *  Default input groups menu
 *
 *************************************/

static UINT32 menu_default_input_groups(UINT32 state)
{
	ui_menu_item item_list[IPG_TOTAL_GROUPS + 2];
	int menu_items = 0;

	/* reset the menu */
	memset(item_list, 0, sizeof(item_list));

	/* build up the menu */
	for (menu_items = 0; menu_items < IPG_TOTAL_GROUPS; menu_items++)
		item_list[menu_items].text = ui_getstring(UI_uigroup + menu_items);

	/* add an item for the return */
	item_list[menu_items++].text = ui_getstring(UI_returntomain);

	/* draw the menu */
	ui_draw_menu(item_list, menu_items, state);

	/* handle the keys */
	if (ui_menu_generic_keys((int *) &state, menu_items))
		return state;
	if (input_ui_pressed(IPT_UI_SELECT))
		return ui_menu_stack_push(menu_default_input, state << 16);

	return state;
}



/*************************************
 *
 *  Common input menu routines
 *
 *************************************/

INLINE int input_menu_update_polling(input_seq *selected_seq, int *record_next, int *polling)
{
	int result = seq_read_async(selected_seq, !*record_next);

	/* continue polling only if we get a negative result */
	*polling = (result < 0);

	/* a zero result means we're done with the sequence; indicate we can append to it */
	if (result == 0)
		*record_next = TRUE;

	/* a positive result means the user cancelled */
	else if (result > 0)
	{
		*record_next = FALSE;
		return TRUE;
	}

	/* return FALSE if the user didn't cancel */
	return FALSE;
}


INLINE void input_menu_toggle_none_default(input_seq *selected_seq, input_seq *original_seq, const input_seq *selected_defseq)
{
	/* if we used to be "none", toggle to the default value */
	if (seq_get_1(original_seq) == CODE_NONE)
		seq_copy(selected_seq, selected_defseq);

	/* otherwise, toggle to "none" */
	else
		seq_set_1(selected_seq, CODE_NONE);
}



/*************************************
 *
 *  Default input settings menu
 *
 *************************************/

INLINE void default_input_menu_add_item(ui_menu_item *item, const char *format, const char *name, const input_seq *seq, const input_seq *defseq)
{
	/* set the item text using the formatting string provided */
	item->text = &menu_string_pool[menu_string_pool_offset];
	menu_string_pool_offset += sprintf(&menu_string_pool[menu_string_pool_offset], format, name) + 1;

	/* set the subitem text from the sequence */
	item->subtext = &menu_string_pool[menu_string_pool_offset];
	seq_name(seq, &menu_string_pool[menu_string_pool_offset], sizeof(menu_string_pool) - menu_string_pool_offset);
	menu_string_pool_offset += strlen(&menu_string_pool[menu_string_pool_offset]) + 1;

	/* invert if different from the default */
	if (seq_cmp(seq, defseq))
		item->flags |= MENU_FLAG_INVERT;
}


static UINT32 menu_default_input(UINT32 state)
{
	static input_seq starting_seq;

	ui_menu_item item_list[MAX_INPUT_PORTS * MAX_BITS_PER_PORT];
	const input_port_default_entry *indef;
	input_port_default_entry *in;
	const input_seq *selected_defseq = NULL;
	input_seq *selected_seq = NULL;
	UINT8 selected_is_analog = FALSE;
	int selected = state & 0x3fff;
	int record_next = (state >> 14) & 1;
	int polling = (state >> 15) & 1;
	int group = state >> 16;
	int reset_selected = FALSE;
	int menu_items = 0;

	/* reset the menu and string pool */
	memset(item_list, 0, sizeof(item_list));
	menu_string_pool_offset = 0;

	/* iterate over the input ports and add menu items */
	for (in = get_input_port_list(), indef = get_input_port_list_defaults(); in->type != IPT_END; in++, indef++)

		/* add if we match the group and we have a valid name */
		if (in->group == group && in->name && in->name[0] != 0)
		{
			/* if not analog, just add a standard entry for this item */
			if (!port_type_is_analog(in->type))
			{
				if (menu_items == selected)
				{
					selected_seq = &in->defaultseq;
					selected_defseq = &indef->defaultseq;
				}
				default_input_menu_add_item(&item_list[menu_items++], "%s", in->name, &in->defaultseq, &indef->defaultseq);
			}

			/* if we are analog, add three items */
			else
			{
				if (menu_items == selected)
				{
					selected_seq = &in->defaultseq;
					selected_defseq = &indef->defaultseq;
					selected_is_analog = TRUE;
				}
				default_input_menu_add_item(&item_list[menu_items++], "%s Analog", in->name, &in->defaultseq, &indef->defaultseq);

				if (menu_items == selected)
				{
					selected_seq = &in->defaultdecseq;
					selected_defseq = &indef->defaultdecseq;
				}
				default_input_menu_add_item(&item_list[menu_items++], "%s Dec", in->name, &in->defaultdecseq, &indef->defaultdecseq);

				if (menu_items == selected)
				{
					selected_seq = &in->defaultincseq;
					selected_defseq = &indef->defaultincseq;
				}
				default_input_menu_add_item(&item_list[menu_items++], "%s Inc", in->name, &in->defaultincseq, &indef->defaultincseq);
			}
		}

	/* if we're polling, just put an empty entry and arrows for the subitem */
	if (polling)
	{
		item_list[selected].subtext = "   ";
		item_list[selected].flags = MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW;
	}

	/* add an item to return */
	item_list[menu_items++].text = ui_getstring(UI_returntogroup);

	/* draw the menu */
	ui_draw_menu(item_list, menu_items, selected);

	/* if we're polling, read the sequence */
	if (polling)
	{
		if (input_menu_update_polling(selected_seq, &record_next, &polling))
			input_menu_toggle_none_default(selected_seq, &starting_seq, selected_defseq);
	}

	/* otherwise, handle the keys */
	else
	{
		int prevsel = selected;

		/* handle generic menu keys */
		if (ui_menu_generic_keys(&selected, menu_items))
			return selected;

		/* if an item was selected, start polling on it */
		if (input_ui_pressed(IPT_UI_SELECT))
		{
			seq_read_async_start(selected_is_analog);
			seq_copy(&starting_seq, selected_seq);
			polling = TRUE;
		}

		/* if the clear key was pressed, reset the selected item */
		if (input_ui_pressed(IPT_UI_CLEAR))
		{
			input_menu_toggle_none_default(selected_seq, selected_seq, selected_defseq);
			record_next = FALSE;
		}

		/* if the selection changed, update and reset the "record first" flag */
		if (selected != prevsel)
			record_next = FALSE;
	}

	/* if we are to reset ourselves, do it now */
	if (reset_selected && selected != menu_items - 1)
	{
		input_menu_toggle_none_default(selected_seq, &starting_seq, selected_defseq);
		record_next = FALSE;
	}

	return selected | (record_next << 14) | (polling << 15) | (group << 16);
}



/*************************************
 *
 *  Game-specific input settings menu
 *
 *************************************/

INLINE void game_input_menu_add_item(ui_menu_item *item, const char *format, input_port_entry *in, void *ref, int which)
{
	/* set the item text using the formatting string provided */
	item->text = &menu_string_pool[menu_string_pool_offset];
	menu_string_pool_offset += sprintf(&menu_string_pool[menu_string_pool_offset], format, input_port_name(in)) + 1;

	/* set the subitem text from the sequence */
	item->subtext = &menu_string_pool[menu_string_pool_offset];
	seq_name(input_port_seq(in, which), &menu_string_pool[menu_string_pool_offset], sizeof(menu_string_pool) - menu_string_pool_offset);
	menu_string_pool_offset += strlen(&menu_string_pool[menu_string_pool_offset]) + 1;

	/* invert if different from the default */
	if (seq_cmp(input_port_seq(in, which), input_port_default_seq(in->type, in->player, which)))
		item->flags |= MENU_FLAG_INVERT;

	/* keep the sequence pointer as a ref and OR in our extra flags */
	item->ref = ref;
}


INLINE UINT16 compute_port_sort_order(const input_port_entry *in)
{
	if (in->type >= IPT_START1 && in->type <= __ipt_analog_end)
		return (in->type << 2) | (in->player << 12);
	return in->type | 0xf000;
}


static int compare_game_inputs(const void *i1, const void *i2)
{
	const ui_menu_item *item1 = i1;
	const ui_menu_item *item2 = i2;
	const input_item_data *data1 = item1->ref;
	const input_item_data *data2 = item2->ref;
	return (data1->sortorder < data2->sortorder) ? -1 : (data1->sortorder > data2->sortorder) ? 1 : 0;
}


static UINT32 menu_game_input(UINT32 state)
{
	static const input_seq default_seq = SEQ_DEF_1(CODE_DEFAULT);

	ui_menu_item item_list[MAX_INPUT_PORTS * MAX_BITS_PER_PORT / 2];
	input_item_data item_data[MAX_INPUT_PORTS * MAX_BITS_PER_PORT / 2];
	input_item_data *selected_item_data;
	int selected = state & 0x3fff;
	int record_next = (state >> 14) & 1;
	int polling = (state >> 15) & 1;
	input_port_entry *in;
	int menu_items = 0;

	/* reset the menu and string pool */
	memset(item_list, 0, sizeof(item_list));
	menu_string_pool_offset = 0;

	/* iterate over the input ports and add menu items */
	for (in = Machine->input_ports; in->type != IPT_END; in++)

		/* add if we match the group and we have a valid name */
		if (input_port_name(in) != NULL &&
#ifdef MESS
			(in->category == 0 || input_category_active(in->category)) &&
#endif /* MESS */
			((in->type == IPT_OTHER && in->name != IP_NAME_DEFAULT) || port_type_to_group(in->type, in->player) != IPG_INVALID))
		{
			/* if not analog, just add a standard entry for this item */
			if (!port_type_is_analog(in->type))
			{
				item_data[menu_items].seq = &in->seq;
				item_data[menu_items].sortorder = compute_port_sort_order(in);
				item_data[menu_items].digital = TRUE;
				game_input_menu_add_item(&item_list[menu_items], "%s", in, &item_data[menu_items], SEQ_TYPE_STANDARD);
				menu_items++;
			}

			/* if we are analog, add three items */
			else
			{
				item_data[menu_items].seq = &in->seq;
				item_data[menu_items].sortorder = compute_port_sort_order(in);
				item_data[menu_items].digital = FALSE;
				game_input_menu_add_item(&item_list[menu_items], "%s Analog", in, &item_data[menu_items], SEQ_TYPE_STANDARD);
				menu_items++;

				item_data[menu_items].seq = &in->analog.decseq;
				item_data[menu_items].sortorder = compute_port_sort_order(in) | 1;
				item_data[menu_items].digital = TRUE;
				game_input_menu_add_item(&item_list[menu_items], "%s Dec", in, &item_data[menu_items], SEQ_TYPE_DECREMENT);
				menu_items++;

				item_data[menu_items].seq = &in->analog.incseq;
				item_data[menu_items].sortorder = compute_port_sort_order(in) | 2;
				item_data[menu_items].digital = TRUE;
				game_input_menu_add_item(&item_list[menu_items], "%s Inc", in, &item_data[menu_items], SEQ_TYPE_INCREMENT);
				menu_items++;
			}
		}

	/* sort the list canonically */
	qsort(item_list, menu_items, sizeof(item_list[0]), compare_game_inputs);

	/* if we're polling, just put an empty entry and arrows for the subitem */
	if (polling)
	{
		item_list[selected].subtext = "   ";
		item_list[selected].flags = MENU_FLAG_LEFT_ARROW | MENU_FLAG_RIGHT_ARROW;
	}

	/* add an item to return */
	item_list[menu_items++].text = ui_getstring(UI_returntomain);

	/* draw the menu */
	ui_draw_menu(item_list, menu_items, selected);

	/* if we're polling, read the sequence */
	selected_item_data = item_list[selected].ref;
	if (polling)
	{
		if (input_menu_update_polling(selected_item_data->seq, &record_next, &polling))
			input_menu_toggle_none_default(selected_item_data->seq, &starting_seq, &default_seq);
	}

	/* otherwise, handle the keys */
	else
	{
		int prevsel = selected;

		/* handle generic menu keys */
		if (ui_menu_generic_keys(&selected, menu_items))
			return selected;

		/* if an item was selected, start polling on it */
		if (input_ui_pressed(IPT_UI_SELECT))
		{
			seq_read_async_start(!selected_item_data->digital);
			seq_copy(&starting_seq, selected_item_data->seq);
			polling = TRUE;
		}

		/* if the clear key was pressed, reset the selected item */
		if (input_ui_pressed(IPT_UI_CLEAR))
		{
			input_menu_toggle_none_default(selected_item_data->seq, selected_item_data->seq, &default_seq);
			record_next = FALSE;
		}

		/* if the selection changed, update and reset the "record first" flag */
		if (selected != prevsel)
			record_next = FALSE;
	}

	return selected | (record_next << 14) | (polling << 15);
}



/*************************************
 *
 *  DIP switch/configurations/categories
 *  settings menu
 *
 *************************************/

INLINE void switch_menu_add_item(ui_menu_item *item, const input_port_entry *in, int switch_entry, void *ref)
{
	const input_port_entry *tin;

	/* set the text to the name and the subitem text to invalid */
	item->text = input_port_name(in);
	item->subtext = NULL;

	/* scan for the current selection in the list */
	for (tin = in + 1; tin->type == switch_entry; tin++)
		if (input_port_condition(tin))
		{
			/* if this is a match, set the subtext */
			if (in->default_value == tin->default_value)
				item->subtext = input_port_name(tin);

			/* else if we haven't seen a match yet, show a left arrow */
			else if (!item->subtext)
				item->flags |= MENU_FLAG_LEFT_ARROW;

			/* else if we have seen a match, show a right arrow */
			else
				item->flags |= MENU_FLAG_RIGHT_ARROW;
		}

	/* if no matches, we're invalid */
	if (!item->subtext)
		item->subtext = ui_getstring(UI_INVALID);

	/* stash our reference */
	item->ref = ref;
}


static void switch_menu_pick_previous(input_port_entry *in, int switch_entry)
{
	int last_value = in->default_value;
	const input_port_entry *tin;

	/* scan for the current selection in the list */
	for (tin = in + 1; tin->type == switch_entry; tin++)
		if (input_port_condition(tin))
		{
			/* if this is a match, we're done */
			if (in->default_value == tin->default_value)
			{
				in->default_value = last_value;
				return;
			}

			/* otherwise, keep track of last one found */
			else
				last_value = tin->default_value;
		}
}


static void switch_menu_pick_next(input_port_entry *in, int switch_entry)
{
	const input_port_entry *tin;
	int foundit = FALSE;

	/* scan for the current selection in the list */
	for (tin = in + 1; tin->type == switch_entry; tin++)
		if (input_port_condition(tin))
		{
			/* if we found the current selection, we pick the next one */
			if (foundit)
			{
				in->default_value = tin->default_value;
				return;
			}

			/* if this is a match, note it */
			if (in->default_value == tin->default_value)
				foundit = TRUE;
		}
}


/*
static int compare_switch_inputs(const void *i1, const void *i2)
{
    const ui_menu_item *item1 = i1;
    const ui_menu_item *item2 = i2;
    const input_port_entry *data1 = item1->ref;
    const input_port_entry *data2 = item2->ref;
    return strcmp(input_port_name(data1), input_port_name(data2));
}
*/


static UINT32 menu_switches(UINT32 state)
{
	ui_menu_item item_list[MAX_INPUT_PORTS * MAX_BITS_PER_PORT / 2];
	int switch_entry = (state >> 24) & 0xff;
	int switch_name = (state >> 16) & 0xff;
	int selected = state & 0xffff;
	input_port_entry *selected_in = NULL;
	input_port_entry *in;
	int menu_items = 0;
	int changed = FALSE;

	/* reset the menu */
	memset(item_list, 0, sizeof(item_list));

	/* loop over input ports and set up the current values */
	for (in = Machine->input_ports; in->type != IPT_END; in++)
		if (in->type == switch_name && input_port_active(in) && input_port_condition(in))
			switch_menu_add_item(&item_list[menu_items++], in, switch_entry, in);

	/* sort the list */
//  qsort(item_list, menu_items, sizeof(item_list[0]), compare_switch_inputs);
	selected_in = item_list[selected].ref;

	/* add an item to return */
	item_list[menu_items++].text = ui_getstring(UI_returntomain);

	/* draw the menu */
	ui_draw_menu(item_list, menu_items, selected);

	/* handle generic menu keys */
	if (ui_menu_generic_keys(&selected, menu_items))
		return selected;

	/* handle left/right arrows */
	if (input_ui_pressed(IPT_UI_LEFT) && (item_list[selected].flags & MENU_FLAG_LEFT_ARROW))
	{
		switch_menu_pick_previous(selected_in, switch_entry);
		changed = TRUE;
	}
	if (input_ui_pressed(IPT_UI_RIGHT) && (item_list[selected].flags & MENU_FLAG_RIGHT_ARROW))
	{
		switch_menu_pick_next(selected_in, switch_entry);
		changed = TRUE;
	}

	/* update the selection to match the existing entry in case things got shuffled */
	/* due to conditional DIPs changing things */
	if (changed)
	{
		int newsel = 0;
		input_port_update_defaults();
		for (in = Machine->input_ports; in->type != IPT_END; in++)
			if (in->type == switch_name && input_port_active(in) && input_port_condition(in))
			{
				if (selected_in == in)
				{
					selected = newsel;
					break;
				}
				newsel++;
			}
	}

	return selected | (switch_name << 16) | (switch_entry << 24);
}



/*************************************
 *
 *  Analog settings menu
 *
 *************************************/

INLINE void analog_menu_add_item(ui_menu_item *item, const input_port_entry *in, int append_string, int which_item)
{
	int value, minval, maxval;

	/* set the item text using the formatting string provided */
	item->text = &menu_string_pool[menu_string_pool_offset];
	menu_string_pool_offset += sprintf(&menu_string_pool[menu_string_pool_offset], "%s %s", input_port_name(in), ui_getstring(append_string)) + 1;

	/* set the subitem text */
	item->subtext = &menu_string_pool[menu_string_pool_offset];
	switch (which_item)
	{
		default:
		case ANALOG_ITEM_KEYSPEED:
			value = in->analog.delta;
			minval = 1;
			maxval = 255;
			menu_string_pool_offset += sprintf(&menu_string_pool[menu_string_pool_offset], "%d", value) + 1;
			break;

		case ANALOG_ITEM_CENTERSPEED:
			value = in->analog.centerdelta;
			minval = 0;
			maxval = 255;
			menu_string_pool_offset += sprintf(&menu_string_pool[menu_string_pool_offset], "%d", value) + 1;
			break;

		case ANALOG_ITEM_REVERSE:
			value = in->analog.reverse;
			minval = 0;
			maxval = 1;
			item->subtext = value ? ui_getstring(UI_on) : ui_getstring(UI_off);
			break;

		case ANALOG_ITEM_SENSITIVITY:
			value = in->analog.sensitivity;
			minval = 1;
			maxval = 255;
			menu_string_pool_offset += sprintf(&menu_string_pool[menu_string_pool_offset], "%d%%", value) + 1;
			break;
	}

	/* put on arrows */
	if (value > minval)
		item->flags |= MENU_FLAG_LEFT_ARROW;
	if (value < maxval)
		item->flags |= MENU_FLAG_RIGHT_ARROW;
}


static UINT32 menu_analog(UINT32 state)
{
	ui_menu_item item_list[MAX_INPUT_PORTS * 4 * ANALOG_ITEM_COUNT];
	input_port_entry *selected_in = NULL;
	input_port_entry *in;
	int menu_items = 0;
	int delta = 0;

	/* reset the menu and string pool */
	memset(item_list, 0, sizeof(item_list));
	menu_string_pool_offset = 0;

	/* loop over input ports and add the items */
	for (in = Machine->input_ports; in->type != IPT_END; in++)
		if (port_type_is_analog(in->type))
		{
			/* track the selected item */
			if (state >= menu_items && state < menu_items + 4)
				selected_in = in;

			/* add four items for each one */
			analog_menu_add_item(&item_list[menu_items++], in, UI_keyjoyspeed, ANALOG_ITEM_KEYSPEED);
			analog_menu_add_item(&item_list[menu_items++], in, UI_centerspeed, ANALOG_ITEM_CENTERSPEED);
			analog_menu_add_item(&item_list[menu_items++], in, UI_reverse, ANALOG_ITEM_REVERSE);
			analog_menu_add_item(&item_list[menu_items++], in, UI_sensitivity, ANALOG_ITEM_SENSITIVITY);
		}

	/* add an item to return */
	item_list[menu_items++].text = ui_getstring(UI_returntomain);

	/* draw the menu */
	ui_draw_menu(item_list, menu_items, state);

	/* handle generic menu keys */
	if (ui_menu_generic_keys((int *) &state, menu_items))
		return state;

	/* handle left/right arrows */
	if (input_ui_pressed_repeat(IPT_UI_LEFT,6) && (item_list[state].flags & MENU_FLAG_LEFT_ARROW))
		delta = -1;
	if (input_ui_pressed_repeat(IPT_UI_RIGHT,6) && (item_list[state].flags & MENU_FLAG_RIGHT_ARROW))
		delta = 1;

	/* adjust the appropriate value */
	if (delta != 0 && selected_in)
		switch (state % ANALOG_ITEM_COUNT)
		{
			case ANALOG_ITEM_KEYSPEED:		selected_in->analog.delta += delta;			break;
			case ANALOG_ITEM_CENTERSPEED:	selected_in->analog.centerdelta += delta;	break;
			case ANALOG_ITEM_REVERSE:		selected_in->analog.reverse += delta;		break;
			case ANALOG_ITEM_SENSITIVITY:	selected_in->analog.sensitivity += delta;	break;
		}

	return state;
}



/*************************************
 *
 *  Joystick calibration menu
 *
 *************************************/

static UINT32 menu_joystick_calibrate(UINT32 state)
{
	const char *msg;

	/* two states */
	switch (state)
	{
		/* state 0 is just starting */
		case 0:
			osd_joystick_start_calibration();
			state++;
			break;

		/* state 1 is where we spend our time */
		case 1:

			/* get the message; if none, we're done */
			msg = osd_joystick_calibrate_next();
			if (msg == NULL)
			{
				osd_joystick_end_calibration();
				return ui_menu_stack_pop();
			}

			/* display the message */
			ui_draw_message_window(msg);

			/* handle cancel and select */
			if (input_ui_pressed(IPT_UI_CANCEL))
				return ui_menu_stack_pop();
			if (input_ui_pressed(IPT_UI_SELECT))
				osd_joystick_calibrate();
			break;
	}

	return state;
}



/*************************************
 *
 *  Bookkeeping information screen
 *
 *************************************/

#ifndef MESS
static UINT32 menu_bookkeeping(UINT32 state)
{
	char buf[2048];
	char *bufptr = buf;
	int selected = 0;
	int ctrnum;
	mame_time total_time;

	/* show total time first */
	total_time = mame_timer_get_time();
	if (total_time.seconds >= 60 * 60)
		bufptr += sprintf(bufptr, "%s: %d:%02d:%02d\n\n", ui_getstring(UI_totaltime), total_time.seconds / (60*60), (total_time.seconds / 60) % 60, total_time.seconds % 60);
	else
		bufptr += sprintf(bufptr, "%s: %d:%02d\n\n", ui_getstring(UI_totaltime), (total_time.seconds / 60) % 60, total_time.seconds % 60);

	/* show tickets at the top */
	if (dispensed_tickets)
		bufptr += sprintf(bufptr, "%s: %d\n\n", ui_getstring(UI_tickets), dispensed_tickets);

	/* loop over coin counters */
	for (ctrnum = 0; ctrnum < COIN_COUNTERS; ctrnum++)
	{
		/* display the coin counter number */
		bufptr += sprintf(bufptr, "%s %c: ", ui_getstring(UI_coin), ctrnum + 'A');

		/* display how many coins */
		if (!coin_count[ctrnum])
			bufptr += sprintf(bufptr, "%s", ui_getstring(UI_NA));
		else
			bufptr += sprintf(bufptr, "%d", coin_count[ctrnum]);

		/* display whether or not we are locked out */
		if (coinlockedout[ctrnum])
			bufptr += sprintf(bufptr, " %s", ui_getstring(UI_locked));
		*bufptr++ = '\n';
	}

	/* make it look like a menu */
	bufptr += sprintf(bufptr, "\n\t%s %s %s", ui_getstring(UI_lefthilight), ui_getstring(UI_returntomain), ui_getstring(UI_righthilight));

	/* draw the text */
	ui_draw_message_window(buf);

	/* handle the keys */
	ui_menu_generic_keys(&selected, 1);
	return selected;
}
#endif



/*************************************
 *
 *  Game information screen
 *
 *************************************/

#ifndef MESS
static UINT32 menu_game_info(UINT32 state)
{
	char buf[2048];
	char *bufptr = buf;
	int selected = 0;

	/* add the game info */
	bufptr += sprintf_game_info(bufptr);

	/* make it look like a menu */
	bufptr += sprintf(bufptr, "\n\t%s %s %s", ui_getstring(UI_lefthilight), ui_getstring(UI_returntomain), ui_getstring(UI_righthilight));

	/* draw the text */
	ui_draw_message_window(buf);

	/* handle the keys */
	ui_menu_generic_keys(&selected, 1);
	return selected;
}
#endif



/*************************************
 *
 *  Cheat menu
 *
 *************************************/

static UINT32 menu_cheat(UINT32 state)
{
	int result = cheat_menu(state);
	if (result == 0)
		return ui_menu_stack_pop();
	return result;
}



/*************************************
 *
 *  Memory card menu
 *
 *************************************/

static UINT32 menu_memory_card(UINT32 state)
{
	ui_menu_item item_list[5];
	int menu_items = 0;
	int cardnum = state >> 16;
	int selected = state & 0xffff;
	int insertindex = -1, ejectindex = -1, createindex = -1;

	/* reset the menu and string pool */
	memset(item_list, 0, sizeof(item_list));
	menu_string_pool_offset = 0;

	/* add the card select menu */
	item_list[menu_items].text = ui_getstring(UI_selectcard);
	item_list[menu_items].subtext = &menu_string_pool[menu_string_pool_offset];
	menu_string_pool_offset += sprintf(&menu_string_pool[menu_string_pool_offset], "%d", cardnum) + 1;
	if (cardnum > 0)
		item_list[menu_items].flags |= MENU_FLAG_LEFT_ARROW;
	if (cardnum < 1000)
		item_list[menu_items].flags |= MENU_FLAG_RIGHT_ARROW;
	menu_items++;

	/* add the remaining items */
	item_list[insertindex = menu_items++].text = ui_getstring(UI_loadcard);
	if (memcard_present() != -1)
		item_list[ejectindex = menu_items++].text = ui_getstring(UI_ejectcard);
	item_list[createindex = menu_items++].text = ui_getstring(UI_createcard);

	/* add an item for the return */
	item_list[menu_items++].text = ui_getstring(UI_returntomain);

	/* draw the menu */
	ui_draw_menu(item_list, menu_items, selected);

	/* handle the keys */
	if (ui_menu_generic_keys(&selected, menu_items))
		return selected;
	if (selected == 0 && input_ui_pressed(IPT_UI_LEFT) && cardnum > 0)
		cardnum--;
	if (selected == 0 && input_ui_pressed(IPT_UI_RIGHT) && cardnum < 1000)
		cardnum++;

	/* handle actions */
	if (input_ui_pressed(IPT_UI_SELECT))
	{
		/* handle load */
		if (selected == insertindex)
		{
			if (memcard_insert(cardnum) == 0)
			{
				ui_popup("%s", ui_getstring(UI_loadok));
				ui_menu_stack_reset();
				return 0;
			}
			else
				ui_popup("%s", ui_getstring(UI_loadfailed));
		}

		/* handle eject */
		else if (selected == ejectindex)
		{
			memcard_eject();
			ui_popup("%s", ui_getstring(UI_cardejected));
		}

		/* handle create */
		else if (selected == createindex)
		{
			if (memcard_create(cardnum, FALSE) == 0)
				ui_popup("%s", ui_getstring(UI_cardcreated));
			else
				ui_popup("%s\n%s", ui_getstring(UI_cardcreatedfailed), ui_getstring(UI_cardcreatedfailed2));
		}
	}

	return selected | (cardnum << 16);
}



/*************************************
 *
 *  Game reset action
 *
 *************************************/

static UINT32 menu_reset_game(UINT32 state)
{
	/* request a reset */
	mame_schedule_soft_reset();

	/* reset the menu stack */
	ui_menu_stack_reset();
	return 0;
}



/*************************************
 *
 *  MESS-specific menus
 *
 *************************************/

#ifdef MESS
static UINT32 menu_file_manager(UINT32 state)
{
	int result = filemanager(state);
	if (result == 0)
	return ui_menu_stack_pop();
	return result;
}


static UINT32 menu_tape_control(UINT32 state)
{
	int result = tapecontrol(state);
	if (result == 0)
	return ui_menu_stack_pop();
	return result;
}
#endif








static int sprintf_game_info(char *buf)
{
	char *bufptr = buf;
	int cpunum, sndnum;
	int count;

	/* print description, manufacturer, and CPU: */
	bufptr += sprintf(bufptr, "%s\n%s %s\n\n%s:\n", Machine->gamedrv->description, Machine->gamedrv->year, Machine->gamedrv->manufacturer, ui_getstring(UI_cpu));

	/* loop over all CPUs */
	for (cpunum = 0; cpunum < MAX_CPU && Machine->drv->cpu[cpunum].cpu_type; cpunum += count)
	{
		int type = Machine->drv->cpu[cpunum].cpu_type;
		int clock = Machine->drv->cpu[cpunum].cpu_clock;

		/* count how many identical CPUs we have */
		for (count = 1; cpunum + count < MAX_CPU; count++)
			if (Machine->drv->cpu[cpunum + count].cpu_type != type ||
		        Machine->drv->cpu[cpunum + count].cpu_clock != clock)
		    	break;

		/* if more than one, prepend a #x in front of the CPU name */
		if (count > 1)
			bufptr += sprintf(bufptr, "%dx", count);
		bufptr += sprintf(bufptr, "%s", cputype_name(type));

		/* display clock in kHz or MHz */
		if (clock >= 1000000)
			bufptr += sprintf(bufptr, " %d.%06d MHz\n", clock / 1000000, clock % 1000000);
		else
			bufptr += sprintf(bufptr, " %d.%03d kHz\n", clock / 1000, clock % 1000);
	}

	/* append the Sound: string */
	bufptr += sprintf(bufptr, "\n%s:\n", ui_getstring(UI_sound));

	/* loop over all sound chips */
	for (sndnum = 0; sndnum < MAX_SOUND && Machine->drv->sound[sndnum].sound_type; sndnum += count)
	{
		int type = Machine->drv->sound[sndnum].sound_type;
		int clock = sndnum_clock(sndnum);

		/* count how many identical sound chips we have */
		for (count = 1; sndnum + count < MAX_SOUND; count++)
			if (Machine->drv->sound[sndnum + count].sound_type != type ||
		        sndnum_clock(sndnum + count) != clock)
		    	break;

		/* if more than one, prepend a #x in front of the CPU name */
		if (count > 1)
			bufptr += sprintf(bufptr, "%dx", count);
		bufptr += sprintf(bufptr, "%s", sndnum_name(sndnum));

		/* display clock in kHz or MHz */
		if (clock >= 1000000)
			bufptr += sprintf(bufptr, " %d.%06d MHz\n", clock / 1000000, clock % 1000000);
		else if (clock != 0)
			bufptr += sprintf(bufptr, " %d.%03d kHz\n", clock / 1000, clock % 1000);
		else
			*bufptr++ = '\n';
	}

	/* display vector information for vector games */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		bufptr += sprintf(bufptr, "\n%s\n", ui_getstring(UI_vectorgame));

	/* display screen resolution and refresh rate info for raster games */
	else
		bufptr += sprintf(bufptr,"\n%s:\n%d x %d (%s) %f Hz\n",
				ui_getstring(UI_screenres),
				Machine->visible_area.max_x - Machine->visible_area.min_x + 1,
				Machine->visible_area.max_y - Machine->visible_area.min_y + 1,
				(Machine->gamedrv->flags & ORIENTATION_SWAP_XY) ? "V" : "H",
				Machine->refresh_rate);
	return bufptr - buf;
}








/*-------------------------------------------------
    erase_screen - erase the screen
-------------------------------------------------*/

static void erase_screen(mame_bitmap *bitmap)
{
	fillbitmap(bitmap, get_black_pen(), NULL);
	schedule_full_refresh();
}


#if 0

static int showgfx_mode;

void showgfx_show(int visible)
{
	showgfx_enabled = visible;

	if (!showgfx_enabled)
	{
		mame_pause(TRUE);
		showgfx_mode = 0;
	}
	else
	{
		mame_pause(FALSE);
	}


	schedule_full_refresh();

	/* mark all the tilemaps dirty on exit so they are updated correctly on the next frame */
	tilemap_mark_all_tiles_dirty(NULL);
}


void showgfx_render(mame_bitmap *bitmap)
{
	static const rectangle fullrect = { 0, 10000, 0, 10000 };
	int num_tilemap = tilemap_count();
	int num_gfx;

	/* determine the number of gfx sets we have */


	/* mark the whole thing dirty */
	artwork_mark_ui_dirty(fullrect.min_x, fullrect.min_y, fullrect.max_x, fullrect.max_y);
	ui_dirty = 5;

	if (input_ui_pressed_repeat(IPT_UI_RIGHT,6))
	{
		go_to_next_mode;
		showgfx_xscroll = 0;
		showgfx_yscroll = 0;
	}

	if (input_ui_pressed_repeat(IPT_UI_LEFT,6))

	if (input_ui_pressed(
}


static void show_colors(int submode)
{
	int boxheight = ui_get_line_height();
	int boxwidth = ui_get_char_width('0');

	int i;
	char buf[80];
	int mode,bank,color,firstdrawn;
	int palpage;
	int changed;
	int total_colors = 0;
	pen_t *colortable = NULL;
	int cpx=0,cpy,skip_chars=0,skip_tmap=0;
	int sx,sy,colors;
	int column_heading_max;
	struct bounds;


	/* make the boxes square */
	if (boxwidth < boxheight)
		boxwidth = boxheight;
	else
		boxheight = boxwidth;

	/* submode 0 is the palette */
	if (submode == 0)
	{
		total_colors = Machine->drv->total_colors;
		colortable = Machine->pens;
		ui_draw_text(0, 0, "PALETTE");
	}

	/* submode 1 is the CLUT */
	else if (submode == 1)
	{
		total_colors = Machine->drv->color_table_len;
		colortable = Machine->remapped_colortable;
		ui_draw_text(0, 0, "CLUT");
	}

	/* anything else is invalid */
	else
		return;

	/* start with a blank slate */
	fillbitmap(bitmap, get_black_pen(), NULL);

	/* determine how many colors on this page */
	colors = total_colors - showgfx_yscroll;
	if (colors > 256)
		colors = 256;

	/* and the maximum number of colums we need to display */
	if (colors < 16)
		column_heading_max = (color < 16) ? colors : 16;

	/* determine the grid top/left */
	grid_top = 3 * ui_get_line_height();
	grid_left = ui_get_string_width("0000");

	/* display the column headings */
	for (colnum = 0; colnum < column_heading_max; colnum++)
	{
		char tempstr[10];

		/* draw a single hex digit over the center of each column */
		sprintf(tempstr, "%X", colnum);
		ui_draw_text(tempstr,  grid_left + colnum * boxwidth + (boxwidth - ui_get_string_width(tempstr)) / 2,
					 grid_top - ui_get_line_height());
	}

	/* display the row headings */
	for (rownum = 0; rownum < (colors + 15) / 16; rownum++)
	{
		char tempstr[10];

		/* draw the palette index to the left of each row */
		sprintf(tempstr, "%X", showgfx_yscroll + rownum * 16);
		ui_draw_text(tempstr, grid_left - ui_get_string_width(tempstr),
					 grid_top + rownum * boxheight + (boxheight - ui_get_line_height()) / 2);
	}

	/* now draw the pretty boxes */
	for (colornum = 0; colornum < colors; colornum++)
	{
		rectangle bounds;

		/* compute the bounds of the box */
		bounds.min_x = grid_left + (colornum % 16) * boxwidth;
		bounds.max_x = bounds.min_x + boxwidth - 1;
		bounds.min_y = grid_top + (colornum / 16) * boxheight;
		bounds.max_y = bounds.min_y + boxheight - 1;

		bounds.min_x = uirotbounds.min_x + 3*uirotcharwidth + (uirotcharwidth*4/3)*(i % 16);
		bounds.min_y = uirotbounds.min_y + 2*uirotcharheight + (uirotcharheight)*(i / 16) + uirotcharheight;
		bounds.max_x = bounds.min_x + uirotcharwidth*4/3 - 1;
		bounds.max_y = bounds.min_y + uirotcharheight - 1;
		ui_rot2raw_rect(&bounds);
		fillbitmap(bitmap, colortable[i + 256*palpage], &bounds);
	}
	}
	else
		ui_draw_text("N/A",3*uirotcharwidth,2*uirotcharheight);

	ui_draw_text(buf,0,0);

	render_ui(bitmap);
	update_video_and_audio();

	if (code_pressed_memory_repeat(KEYCODE_PGDN,4))
	{
		if (256 * (palpage + 1) < total_colors)
		{
			palpage++;
			changed = 1;
		}
	}

	if (code_pressed_memory_repeat(KEYCODE_PGUP,4))
	{
		if (palpage > 0)
		{
			palpage--;
			changed = 1;
		}
	}
}

#endif


static void showcharset(mame_bitmap *bitmap)
{
	int i;
	char buf[80];
	int mode,bank,color,firstdrawn;
	int palpage;
	int changed;
	int total_colors = 0;
	pen_t *colortable = NULL;
	int cpx=0,cpy,skip_chars=0,skip_tmap=0;
	int tilemap_xpos = 0;
	int tilemap_ypos = 0;

	mode = 0;
	bank = 0;
	color = 0;
	firstdrawn = 0;
	palpage = 0;

	changed = 1;

	/* mark all the tilemaps dirty on entry so they are re-drawn consistently in the viewer */
	tilemap_mark_all_tiles_dirty(NULL);

	do
	{
		static const rectangle fullrect = { 0, 10000, 0, 10000 };

		/* mark the whole thing dirty */
		artwork_mark_ui_dirty(fullrect.min_x, fullrect.min_y, fullrect.max_x, fullrect.max_y);
		ui_dirty = 5;

		switch (mode)
		{
			case 0: /* palette or clut */
			{
				if (bank == 0)	/* palette */
				{
					total_colors = Machine->drv->total_colors;
					colortable = Machine->pens;
					strcpy(buf,"PALETTE");
				}
				else if (bank == 1)	/* clut */
				{
					total_colors = Machine->drv->color_table_len;
					colortable = Machine->remapped_colortable;
					strcpy(buf,"CLUT");
				}
				else
				{
					buf[0] = 0;
					total_colors = 0;
					colortable = 0;
				}

				/*if (changed) -- temporary */
				{
					erase_screen(bitmap);

					if (total_colors)
					{
						int sx,sy,colors;
						int column_heading_max;
						struct bounds;

						colors = total_colors - 256 * palpage;
						if (colors > 256) colors = 256;

						/* min(colors, 16) */
						if (colors < 16)
							column_heading_max = colors;
						else
							column_heading_max = 16;

						for (i = 0;i < column_heading_max;i++)
						{
							char bf[40];

							sx = 3*uirotcharwidth + (uirotcharwidth*4/3)*(i % 16);
							sprintf(bf,"%X",i);
							ui_draw_text(bf,sx,2*uirotcharheight);
							if (16*i < colors)
							{
								sy = 3*uirotcharheight + (uirotcharheight)*(i % 16);
								sprintf(bf,"%3X",i+16*palpage);
								ui_draw_text(bf,0,sy);
							}
						}

						for (i = 0;i < colors;i++)
						{
							rectangle bounds;
							bounds.min_x = uirotbounds.min_x + 3*uirotcharwidth + (uirotcharwidth*4/3)*(i % 16);
							bounds.min_y = uirotbounds.min_y + 2*uirotcharheight + (uirotcharheight)*(i / 16) + uirotcharheight;
							bounds.max_x = bounds.min_x + uirotcharwidth*4/3 - 1;
							bounds.max_y = bounds.min_y + uirotcharheight - 1;
							ui_rot2raw_rect(&bounds);
							fillbitmap(bitmap, colortable[i + 256*palpage], &bounds);
						}
					}
					else
						ui_draw_text("N/A",3*uirotcharwidth,2*uirotcharheight);

					ui_draw_text(buf,0,0);
					changed = 0;
				}

				break;
			}
			case 1: /* characters */
			{
				int crotwidth = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? Machine->gfx[bank]->height : Machine->gfx[bank]->width;
				int crotheight = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? Machine->gfx[bank]->width : Machine->gfx[bank]->height;
				cpx = uirotwidth / crotwidth;
				if (cpx == 0) cpx = 1;
				cpy = (uirotheight - uirotcharheight) / crotheight;
				if (cpy == 0) cpy = 1;
				skip_chars = cpx * cpy;
				/*if (changed) -- temporary */
				{
					int flipx,flipy;
					int lastdrawn=0;

					erase_screen(bitmap);

					/* validity check after char bank change */
					if (firstdrawn >= Machine->gfx[bank]->total_elements)
					{
						firstdrawn = Machine->gfx[bank]->total_elements - skip_chars;
						if (firstdrawn < 0) firstdrawn = 0;
					}

					flipx = 0;
					flipy = 0;

					for (i = 0; i+firstdrawn < Machine->gfx[bank]->total_elements && i<cpx*cpy; i++)
					{
						rectangle bounds;
						bounds.min_x = (i % cpx) * crotwidth + uirotbounds.min_x;
						bounds.min_y = uirotcharheight + (i / cpx) * crotheight + uirotbounds.min_y;
						bounds.max_x = bounds.min_x + crotwidth - 1;
						bounds.max_y = bounds.min_y + crotheight - 1;
						ui_rot2raw_rect(&bounds);

						drawgfx(bitmap,Machine->gfx[bank],
								i+firstdrawn,color,  /*sprite num, color*/
								flipx,flipy,bounds.min_x,bounds.min_y,
								0,Machine->gfx[bank]->colortable ? TRANSPARENCY_NONE : TRANSPARENCY_NONE_RAW,0);

						lastdrawn = i+firstdrawn;
					}

					sprintf(buf,"GFXSET %d COLOR %2X CODE %X-%X",bank,color,firstdrawn,lastdrawn);
					ui_draw_text(buf,0,0);
					changed = 0;
				}

				break;
			}
			case 2: /* Tilemaps */
			{
				/*if (changed) -- temporary */
				{
					UINT32 tilemap_width, tilemap_height;
					tilemap_nb_size (bank, &tilemap_width, &tilemap_height);
					while (tilemap_xpos < 0)
						tilemap_xpos += tilemap_width;
					tilemap_xpos %= tilemap_width;

					while (tilemap_ypos < 0)
						tilemap_ypos += tilemap_height;
					tilemap_ypos %= tilemap_height;

					erase_screen(bitmap);
					tilemap_nb_draw (bitmap, bank, tilemap_xpos, tilemap_ypos);
					sprintf(buf, "TILEMAP %d (%dx%d)  X:%d  Y:%d", bank, tilemap_width, tilemap_height, tilemap_xpos, tilemap_ypos);
					ui_draw_text(buf,0,0);
					changed = 0;
					skip_tmap = 0;
				}
				break;
			}
		}

		render_ui(bitmap);
		update_video_and_audio();

		if (code_pressed(KEYCODE_LCONTROL) || code_pressed(KEYCODE_RCONTROL))
		{
			skip_chars = cpx;
			skip_tmap = 8;
		}
		if (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT))
		{
			skip_chars = 1;
			skip_tmap = 1;
		}


		if (input_ui_pressed_repeat(IPT_UI_RIGHT,6))
		{
			int next_bank, next_mode;
			int jumped;

			next_mode = mode;
			next_bank = bank+1;
			do {
				jumped = 0;
				switch (next_mode)
				{
					case 0:
						if (next_bank == 2 || Machine->drv->color_table_len == 0)
						{
							jumped = 1;
							next_mode++;
							next_bank = 0;
						}
						break;
					case 1:
						if (next_bank == MAX_GFX_ELEMENTS || !Machine->gfx[next_bank])
						{
							jumped = 1;
							next_mode++;
							next_bank = 0;
						}
						break;
					case 2:
						if (next_bank == tilemap_count())
							next_mode = -1;
						break;
				}
			}	while (jumped);
			if (next_mode != -1 )
			{
				bank = next_bank;
				mode = next_mode;
//              firstdrawn = 0;
				changed = 1;
			}
		}

		if (input_ui_pressed_repeat(IPT_UI_LEFT,6))
		{
			int next_bank, next_mode;

			next_mode = mode;
			next_bank = bank-1;
			while(next_bank < 0 && next_mode >= 0)
			{
				next_mode = next_mode - 1;
				switch (next_mode)
				{
					case 0:
						if (Machine->drv->color_table_len == 0)
							next_bank = 0;
						else
							next_bank = 1;
						break;
					case 1:
						next_bank = MAX_GFX_ELEMENTS-1;
						while (next_bank >= 0 && !Machine->gfx[next_bank])
							next_bank--;
						break;
					case 2:
						next_bank = tilemap_count() - 1;
						break;
				}
			}
			if (next_mode != -1 )
			{
				bank = next_bank;
				mode = next_mode;
//              firstdrawn = 0;
				changed = 1;
			}
		}

		if (code_pressed_memory_repeat(KEYCODE_PGDN,4))
		{
			switch (mode)
			{
				case 0:
				{
					if (256 * (palpage + 1) < total_colors)
					{
						palpage++;
						changed = 1;
					}
					break;
				}
				case 1:
				{
					if (firstdrawn + skip_chars < Machine->gfx[bank]->total_elements)
					{
						firstdrawn += skip_chars;
						changed = 1;
					}
					break;
				}
				case 2:
				{
					if (skip_tmap)
						tilemap_ypos -= skip_tmap;
					else
						tilemap_ypos -= bitmap->height/4;
					changed = 1;
					break;
				}
			}
		}

		if (code_pressed_memory_repeat(KEYCODE_PGUP,4))
		{
			switch (mode)
			{
				case 0:
				{
					if (palpage > 0)
					{
						palpage--;
						changed = 1;
					}
					break;
				}
				case 1:
				{
					firstdrawn -= skip_chars;
					if (firstdrawn < 0) firstdrawn = 0;
					changed = 1;
					break;
				}
				case 2:
				{
					if (skip_tmap)
						tilemap_ypos += skip_tmap;
					else
						tilemap_ypos += bitmap->height/4;
					changed = 1;
					break;
				}
			}
		}

		if (code_pressed_memory_repeat(KEYCODE_D,4))
		{
			switch (mode)
			{
				case 2:
				{
					if (skip_tmap)
						tilemap_xpos -= skip_tmap;
					else
						tilemap_xpos -= bitmap->width/4;
					changed = 1;
					break;
				}
			}
		}

		if (code_pressed_memory_repeat(KEYCODE_G,4))
		{
			switch (mode)
			{
				case 2:
				{
					if (skip_tmap)
						tilemap_xpos += skip_tmap;
					else
						tilemap_xpos += bitmap->width/4;
					changed = 1;
					break;
				}
			}
		}

		if (input_ui_pressed_repeat(IPT_UI_UP,6))
		{
			switch (mode)
			{
				case 1:
				{
					if (color < Machine->gfx[bank]->total_colors - 1)
					{
						color++;
						changed = 1;
					}
					break;
				}
			}
		}

		if (input_ui_pressed_repeat(IPT_UI_DOWN,6))
		{
			switch (mode)
			{
				case 0:
					break;
				case 1:
				{
					if (color > 0)
					{
						color--;
						changed = 1;
					}
				}
			}
		}

		if (input_ui_pressed(IPT_UI_SNAPSHOT))
			save_screen_snapshot(bitmap);

	} while (!input_ui_pressed(IPT_UI_SHOW_GFX) &&
			!input_ui_pressed(IPT_UI_CANCEL) &&
			!mame_is_scheduled_event_pending());

	schedule_full_refresh();

	/* mark all the tilemaps dirty on exit so they are updated correctly on the next frame */
	tilemap_mark_all_tiles_dirty(NULL);
}


int ui_display_decoding(mame_bitmap *bitmap, int percent)
{
	char buf[1000];
	char *bufptr = buf;

	bufptr += sprintf(bufptr, "%s: %d%%", ui_getstring(UI_decoding_gfx), percent);

	erase_screen(bitmap);

	ui_draw_message_window(buf);
	render_ui(bitmap);

	update_video_and_audio();

	return input_ui_pressed(IPT_UI_CANCEL);
}


int ui_display_copyright(mame_bitmap *bitmap)
{
	char buf[1000];
	char *bufptr = buf;
	int done;

	bufptr += sprintf(bufptr, "%s\n\n", ui_getstring(UI_copyright1));
	bufptr += sprintf(bufptr, ui_getstring(UI_copyright2), Machine->gamedrv->description);
	bufptr += sprintf(bufptr, "\n\n%s", ui_getstring(UI_copyright3));

	menu_state = -1;
	done = 0;

	do
	{
		erase_screen(bitmap);

		ui_draw_message_window(buf);
		render_ui(bitmap);

		update_video_and_audio();
		if (input_ui_pressed(IPT_UI_CANCEL))
		{
			menu_state = 0;
			return 1;
		}
		if (code_pressed_memory(KEYCODE_O) || input_ui_pressed(IPT_UI_LEFT))
			done = 1;
		if (done == 1 && (code_pressed_memory(KEYCODE_K) || input_ui_pressed(IPT_UI_RIGHT)))
			done = 2;
	} while (done < 2 && !mame_is_scheduled_event_pending());

	menu_state = 0;
	erase_screen(bitmap);
	update_video_and_audio();

	return 0;
}

int ui_display_game_warnings(mame_bitmap *bitmap)
{
#define WARNING_FLAGS (	GAME_NOT_WORKING | \
						GAME_UNEMULATED_PROTECTION | \
						GAME_WRONG_COLORS | \
						GAME_IMPERFECT_COLORS | \
						GAME_NO_SOUND |  \
						GAME_IMPERFECT_SOUND |  \
						GAME_IMPERFECT_GRAPHICS | \
						GAME_NO_COCKTAIL)
	int i;
	char buf[2048];
	char *bufptr = buf;

	if (rom_load_warnings() > 0 || (Machine->gamedrv->flags & WARNING_FLAGS))
	{
		int done;

		if (rom_load_warnings() > 0)
		{
			bufptr += sprintf(bufptr, "%s\n", ui_getstring(UI_incorrectroms));
			if (Machine->gamedrv->flags & WARNING_FLAGS)
				*bufptr++ = '\n';
		}

		if (Machine->gamedrv->flags & WARNING_FLAGS)
		{
			bufptr += sprintf(bufptr, "%s\n\n", ui_getstring(UI_knownproblems));

#ifdef MESS
			if (Machine->gamedrv->flags & GAME_COMPUTER)
				bufptr += sprintf(bufptr, "%s\n\n%s\n", ui_getstring(UI_comp1), ui_getstring(UI_comp2));
#endif

			if (Machine->gamedrv->flags & GAME_IMPERFECT_COLORS)
				bufptr += sprintf(bufptr, "%s\n", ui_getstring(UI_imperfectcolors));
			if (Machine->gamedrv->flags & GAME_WRONG_COLORS)
				bufptr += sprintf(bufptr, "%s\n", ui_getstring(UI_wrongcolors));
			if (Machine->gamedrv->flags & GAME_IMPERFECT_GRAPHICS)
				bufptr += sprintf(bufptr, "%s\n", ui_getstring(UI_imperfectgraphics));
			if (Machine->gamedrv->flags & GAME_IMPERFECT_SOUND)
				bufptr += sprintf(bufptr, "%s\n", ui_getstring(UI_imperfectsound));
			if (Machine->gamedrv->flags & GAME_NO_SOUND)
				bufptr += sprintf(bufptr, "%s\n", ui_getstring(UI_nosound));
			if (Machine->gamedrv->flags & GAME_NO_COCKTAIL)
				bufptr += sprintf(bufptr, "%s\n", ui_getstring(UI_nococktail));

			if (Machine->gamedrv->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION))
			{
				const game_driver *maindrv;
				const game_driver *clone_of;
				int foundworking;

				if (Machine->gamedrv->flags & GAME_NOT_WORKING)
					bufptr += sprintf(bufptr, "%s\n", ui_getstring(UI_brokengame));
				if (Machine->gamedrv->flags & GAME_UNEMULATED_PROTECTION)
					bufptr += sprintf(bufptr, "%s\n", ui_getstring(UI_brokenprotection));

				clone_of = driver_get_clone(Machine->gamedrv);
				if (clone_of != NULL && !(clone_of->flags & NOT_A_DRIVER))
	 				maindrv = clone_of;
				else
					maindrv = Machine->gamedrv;

				foundworking = 0;
				i = 0;
				while (drivers[i])
				{
					if (drivers[i] == maindrv || driver_get_clone(drivers[i]) == maindrv)
					{
						if ((drivers[i]->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)) == 0)
						{
							if (foundworking == 0)
								bufptr += sprintf(bufptr, "\n\n%s\n\n", ui_getstring(UI_workingclones));
							bufptr += sprintf(bufptr, "%s\n", drivers[i]->name);
							foundworking = 1;
						}
					}
					i++;
				}
			}
		}

		bufptr += sprintf(bufptr, "\n\n%s", ui_getstring(UI_typeok));

		done = 0;
		do
		{
			int ui_width, ui_height;

			erase_screen(bitmap);

			ui_get_bounds(&ui_width, &ui_height);
			add_filled_box(0, 0, ui_width - 1, ui_height - 1);
			ui_draw_message_window(buf);
			render_ui(bitmap);

			update_video_and_audio();
			if (input_ui_pressed(IPT_UI_CANCEL))
				return 1;
			if (code_pressed_memory(KEYCODE_O) || input_ui_pressed(IPT_UI_LEFT))
				done = 1;
			if (done == 1 && (code_pressed_memory(KEYCODE_K) || input_ui_pressed(IPT_UI_RIGHT)))
				done = 2;
		} while (done < 2 && !mame_is_scheduled_event_pending());
	}

	erase_screen(bitmap);
	update_video_and_audio();

	return 0;
}


int ui_display_game_info(mame_bitmap *bitmap)
{
	int ui_width, ui_height;
	char buf[2048];

	/* clear the input memory */
	while (code_read_async() != CODE_NONE) ;

	while (code_read_async() == CODE_NONE && !mame_is_scheduled_event_pending())
	{
		char *bufptr = buf;

		/* first draw a box around the whole screen */
		ui_get_bounds(&ui_width, &ui_height);
		add_filled_box(0, 0, ui_width - 1, ui_height - 1);

		/* add the game info */
		bufptr += sprintf_game_info(bufptr);

		/* append MAME version and ask for any key */
		bufptr += sprintf(bufptr, "\n\t%s %s\n\t%s", ui_getstring(UI_mame), build_version, ui_getstring(UI_anykey));

		/* draw the window */
		ui_draw_message_window(buf);

		/* render and update */
		render_ui(bitmap);
		update_video_and_audio();

		/* allow cancelling */
		if (input_ui_pressed(IPT_UI_CANCEL))
			return 1;
	}

#ifdef MESS
	erase_screen(bitmap);
	/* make sure that the screen is really cleared, in case autoframeskip kicked in */
	update_video_and_audio();
	update_video_and_audio();
	update_video_and_audio();
	update_video_and_audio();

	while (code_read_async() == CODE_NONE && !mame_is_scheduled_event_pending())
	{
		char *bufptr = buf;

		/* first draw a box around the whole screen */
		ui_get_bounds(&ui_width, &ui_height);
		add_filled_box(0, 0, ui_width - 1, ui_height - 1);

		/* add the game info */
		bufptr += ui_sprintf_image_info(bufptr);

		/* draw the window */
		ui_draw_message_window(buf);

		/* render and update */
		render_ui(bitmap);
		update_video_and_audio();

		/* allow cancelling */
		if (input_ui_pressed(IPT_UI_CANCEL))
			return 1;
	}
#endif

	erase_screen(bitmap);
	/* make sure that the screen is really cleared, in case autoframeskip kicked in */
	update_video_and_audio();
	update_video_and_audio();
	update_video_and_audio();
	update_video_and_audio();

	return 0;
}




/*********************************************************************

  start of On Screen Display handling

*********************************************************************/

/*-------------------------------------------------
    drawbar - draw a thermometer bar
-------------------------------------------------*/

static void drawbar(int leftx, int topy, int width, int height, int percentage, int default_percentage)
{
	int current_x, default_x;
	int bar_top, bar_bottom;

	/* compute positions */
	bar_top = topy + (height + 7)/8;
	bar_bottom = topy + (height - 1) - (height + 7)/8;
	default_x = leftx + (width - 1) * default_percentage / 100;
	current_x = leftx + (width - 1) * percentage / 100;

	/* draw the top and bottom lines */
	add_line(leftx, bar_top, leftx + width - 1, bar_top, RGB_WHITE);
	add_line(leftx, bar_bottom, leftx + width - 1, bar_bottom, RGB_WHITE);

	/* draw default marker */
	add_line(default_x, topy, default_x, bar_top, RGB_WHITE);
	add_line(default_x, bar_bottom, default_x, topy + height - 1, RGB_WHITE);

	/* fill in the percentage */
	add_fill(leftx, bar_top + 1, current_x, bar_bottom - 1, RGB_WHITE);
}



static void displayosd(const char *text,int percentage,int default_percentage)
{
	int space_width = ui_get_char_width(' ');
	int line_height = ui_get_line_height();
	int ui_width, ui_height;
	int text_height;

	/* get our UI bounds */
	ui_get_bounds(&ui_width, &ui_height);

	/* leave a spaces' worth of room along the left/right sides, and a lines' worth on the top/bottom */
	ui_width -= 2 * space_width;
	ui_height -= 2 * line_height;

	/* determine the text height */
	ui_draw_text_full(text, 0, 0, ui_width - 2 * UI_BOX_LR_BORDER,
				JUSTIFY_CENTER, WRAP_WORD, DRAW_NONE, RGB_WHITE, RGB_BLACK, NULL, &text_height);

	/* add a box around the whole area */
	add_filled_box(	space_width,
					line_height + ui_height - text_height - line_height - 2 * UI_BOX_TB_BORDER,
					space_width + ui_width,
					line_height + ui_height);

	/* draw the thermometer */
	drawbar(2 * space_width, line_height + ui_height - UI_BOX_TB_BORDER - text_height - line_height*3/4,
			ui_width - 2 * space_width, line_height*3/4, percentage, default_percentage);

	/* draw the actual text */
	ui_draw_text_full(text, space_width + UI_BOX_LR_BORDER, line_height + ui_height - UI_BOX_TB_BORDER - text_height, ui_width - 2 * UI_BOX_LR_BORDER,
				JUSTIFY_CENTER, WRAP_WORD, DRAW_NORMAL, RGB_WHITE, RGB_BLACK, NULL, &text_height);
}

static void onscrd_adjuster(int increment,int arg)
{
	input_port_entry *in = &Machine->input_ports[arg];
	char buf[80];
	int value;

	if (increment)
	{
		value = in->default_value & 0xff;
		value += increment;
		if (value > 100) value = 100;
		if (value < 0) value = 0;
		in->default_value = (in->default_value & ~0xff) | value;
	}
	value = in->default_value & 0xff;

	sprintf(buf,"%s %d%%",in->name,value);

	displayosd(buf,value,in->default_value >> 8);
}

static void onscrd_volume(int increment,int arg)
{
	char buf[20];
	int attenuation;

	if (increment)
	{
		attenuation = osd_get_mastervolume();
		attenuation += increment;
		if (attenuation > 0) attenuation = 0;
		if (attenuation < -32) attenuation = -32;
		osd_set_mastervolume(attenuation);
	}
	attenuation = osd_get_mastervolume();

	sprintf(buf,"%s %3ddB", ui_getstring (UI_volume), attenuation);
	displayosd(buf,100 * (attenuation + 32) / 32,100);
}

static void onscrd_mixervol(int increment,int arg)
{
	static void *driver = 0;
	char buf[40];
	float volume;
	int ch;
	int doallchannels = 0;
	int proportional = 0;


	if (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT))
		doallchannels = 1;
	if (!code_pressed(KEYCODE_LCONTROL) && !code_pressed(KEYCODE_RCONTROL))
		increment *= 5;
	if (code_pressed(KEYCODE_LALT) || code_pressed(KEYCODE_RALT))
		proportional = 1;

	if (increment)
	{
		if (proportional)
		{
			static float old_vol[100];
			int num_vals = sound_get_user_gain_count();
			float ratio = 1.0;
			int overflow = 0;

			if (driver != Machine->drv)
			{
				driver = (void *)Machine->drv;
				for (ch = 0; ch < num_vals; ch++)
					old_vol[ch] = sound_get_user_gain(ch);
			}

			volume = sound_get_user_gain(arg);
			if (old_vol[arg])
				ratio = (volume + increment * 0.02) / old_vol[arg];

			for (ch = 0; ch < num_vals; ch++)
			{
				volume = ratio * old_vol[ch];
				if (volume < 0 || volume > 2.0)
					overflow = 1;
			}

			if (!overflow)
			{
				for (ch = 0; ch < num_vals; ch++)
				{
					volume = ratio * old_vol[ch];
					sound_set_user_gain(ch,volume);
				}
			}
		}
		else
		{
			driver = 0; /* force reset of saved volumes */

			volume = sound_get_user_gain(arg);
			volume += increment * 0.02;
			if (volume > 2.0) volume = 2.0;
			if (volume < 0) volume = 0;

			if (doallchannels)
			{
				int num_vals = sound_get_user_gain_count();
				for (ch = 0;ch < num_vals;ch++)
					sound_set_user_gain(ch,volume);
			}
			else
				sound_set_user_gain(arg,volume);
		}
	}
	volume = sound_get_user_gain(arg);

	if (proportional)
		sprintf(buf,"%s %s %4.2f", ui_getstring (UI_allchannels), ui_getstring (UI_relative), volume);
	else if (doallchannels)
		sprintf(buf,"%s %s %4.2f", ui_getstring (UI_allchannels), ui_getstring (UI_volume), volume);
	else
		sprintf(buf,"%s %s %4.2f",sound_get_user_gain_name(arg), ui_getstring (UI_volume), volume);
	displayosd(buf,volume*50,sound_get_default_gain(arg)*50);
}

static void onscrd_brightness(int increment,int arg)
{
	char buf[20];
	double brightness;


	if (increment)
	{
		brightness = palette_get_global_brightness();
		brightness += 0.05 * increment;
		if (brightness < 0.1) brightness = 0.1;
		if (brightness > 1.0) brightness = 1.0;
		palette_set_global_brightness(brightness);
	}
	brightness = palette_get_global_brightness();

	sprintf(buf,"%s %3d%%", ui_getstring (UI_brightness), (int)(brightness * 100));
	displayosd(buf,brightness*100,100);
}

static void onscrd_gamma(int increment,int arg)
{
	char buf[20];
	double gamma_correction;

	if (increment)
	{
		gamma_correction = palette_get_global_gamma();

		gamma_correction += 0.05 * increment;
		if (gamma_correction < 0.5) gamma_correction = 0.5;
		if (gamma_correction > 2.0) gamma_correction = 2.0;

		palette_set_global_gamma(gamma_correction);
	}
	gamma_correction = palette_get_global_gamma();

	sprintf(buf,"%s %1.2f", ui_getstring (UI_gamma), gamma_correction);
	displayosd(buf,100*(gamma_correction-0.5)/(2.0-0.5),100*(1.0-0.5)/(2.0-0.5));
}

static void onscrd_vector_flicker(int increment,int arg)
{
	char buf[1000];
	float flicker_correction;

	if (!code_pressed(KEYCODE_LCONTROL) && !code_pressed(KEYCODE_RCONTROL))
		increment *= 5;

	if (increment)
	{
		flicker_correction = vector_get_flicker();

		flicker_correction += increment;
		if (flicker_correction < 0.0) flicker_correction = 0.0;
		if (flicker_correction > 100.0) flicker_correction = 100.0;

		vector_set_flicker(flicker_correction);
	}
	flicker_correction = vector_get_flicker();

	sprintf(buf,"%s %1.2f", ui_getstring (UI_vectorflicker), flicker_correction);
	displayosd(buf,flicker_correction,0);
}

static void onscrd_vector_intensity(int increment,int arg)
{
	char buf[30];
	float intensity_correction;

	if (increment)
	{
		intensity_correction = vector_get_intensity();

		intensity_correction += 0.05 * increment;
		if (intensity_correction < 0.5) intensity_correction = 0.5;
		if (intensity_correction > 3.0) intensity_correction = 3.0;

		vector_set_intensity(intensity_correction);
	}
	intensity_correction = vector_get_intensity();

	sprintf(buf,"%s %1.2f", ui_getstring (UI_vectorintensity), intensity_correction);
	displayosd(buf,100*(intensity_correction-0.5)/(3.0-0.5),100*(1.5-0.5)/(3.0-0.5));
}


static void onscrd_overclock(int increment,int arg)
{
	char buf[30];
	double overclock;
	int cpu, doallcpus = 0, oc;

	if (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT))
		doallcpus = 1;
	if (!code_pressed(KEYCODE_LCONTROL) && !code_pressed(KEYCODE_RCONTROL))
		increment *= 5;
	if( increment )
	{
		overclock = cpunum_get_clockscale(arg);
		overclock += 0.01 * increment;
		if (overclock < 0.01) overclock = 0.01;
		if (overclock > 2.0) overclock = 2.0;
		if( doallcpus )
			for( cpu = 0; cpu < cpu_gettotalcpu(); cpu++ )
				cpunum_set_clockscale(cpu, overclock);
		else
			cpunum_set_clockscale(arg, overclock);
	}

	oc = 100 * cpunum_get_clockscale(arg) + 0.5;

	if( doallcpus )
		sprintf(buf,"%s %s %3d%%", ui_getstring (UI_allcpus), ui_getstring (UI_overclock), oc);
	else
		sprintf(buf,"%s %s%d %3d%%", ui_getstring (UI_overclock), ui_getstring (UI_cpu), arg, oc);
	displayosd(buf,oc/2,100/2);
}

static void onscrd_refresh(int increment,int arg)
{
	float delta = Machine->refresh_rate - Machine->drv->frames_per_second;
	char buf[30];

	increment *= 1000;
	if (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT))
		increment /= 10;
	if (code_pressed(KEYCODE_LCONTROL) || code_pressed(KEYCODE_RCONTROL))
		increment /= 100;
	if (code_pressed(KEYCODE_LALT) || code_pressed(KEYCODE_LALT))
		increment /= 1000;
	if (increment)
	{
		float newrate;
		delta += 0.001 * increment;
		if (delta > 10)
			delta = 10;
		if (delta < -10)
			delta = -10;

		newrate = Machine->drv->frames_per_second;
		if (delta != 0)
			newrate = (floor(newrate * 1000) / 1000) + delta;
		set_refresh_rate(newrate);
	}

	sprintf(buf,"%s %.3f", ui_getstring (UI_refresh_rate), Machine->refresh_rate);
	displayosd(buf,(10 + delta) * 5,100/2);
}

static void onscrd_init(void)
{
	input_port_entry *in;
	int item,ch;

	item = 0;

	{
		int num_vals = sound_get_user_gain_count();
		onscrd_fnc[item] = onscrd_volume;
		onscrd_arg[item] = 0;
		item++;

		for (ch = 0;ch < num_vals;ch++)
		{
			onscrd_fnc[item] = onscrd_mixervol;
			onscrd_arg[item] = ch;
			item++;
		}
	}

	for (in = Machine->input_ports; in && in->type != IPT_END; in++)
		if ((in->type & 0xff) == IPT_ADJUSTER)
		{
			onscrd_fnc[item] = onscrd_adjuster;
			onscrd_arg[item] = in - Machine->input_ports;
			item++;
		}

	if (options.cheat)
	{
		for (ch = 0;ch < cpu_gettotalcpu();ch++)
		{
			onscrd_fnc[item] = onscrd_overclock;
			onscrd_arg[item] = ch;
			item++;
		}
		onscrd_fnc[item] = onscrd_refresh;
		onscrd_arg[item] = ch;
		item++;
	}

	onscrd_fnc[item] = onscrd_brightness;
	onscrd_arg[item] = 0;
	item++;

	onscrd_fnc[item] = onscrd_gamma;
	onscrd_arg[item] = 0;
	item++;

	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		onscrd_fnc[item] = onscrd_vector_flicker;
		onscrd_arg[item] = 0;
		item++;

		onscrd_fnc[item] = onscrd_vector_intensity;
		onscrd_arg[item] = 0;
		item++;
	}

	onscrd_total_items = item;
}

static int on_screen_display(int selected)
{
	int increment,sel;
	static int lastselected = 0;


	if (selected == -1)
		sel = lastselected;
	else sel = selected - 1;

	increment = 0;
	if (input_ui_pressed_repeat(IPT_UI_LEFT,6))
		increment = -1;
	if (input_ui_pressed_repeat(IPT_UI_RIGHT,6))
		increment = 1;
	if (input_ui_pressed_repeat(IPT_UI_DOWN,6))
		sel = (sel + 1) % onscrd_total_items;
	if (input_ui_pressed_repeat(IPT_UI_UP,6))
		sel = (sel + onscrd_total_items - 1) % onscrd_total_items;

	(*onscrd_fnc[sel])(increment,onscrd_arg[sel]);

	lastselected = sel;

	if (input_ui_pressed(IPT_UI_ON_SCREEN_DISPLAY))
	{
		sel = -1;

		schedule_full_refresh();
	}

	return sel + 1;
}

/*********************************************************************

  end of On Screen Display handling

*********************************************************************/

static void initiate_load_save(int type)
{
	load_save_state = type;
	mame_pause(TRUE);
}


static int update_load_save(void)
{
	char filename[20];
	input_code code;
	char file = 0;

	/* if we're not in the middle of anything, skip */
	if (load_save_state == LOADSAVE_NONE)
		return 0;

	/* okay, we're waiting for a key to select a slot; display a message */
	if (load_save_state == LOADSAVE_SAVE)
		ui_draw_message_window("Select position to save to");
	else
		ui_draw_message_window("Select position to load from");

	/* check for cancel key */
	if (input_ui_pressed(IPT_UI_CANCEL))
	{
		/* display a popup indicating things were cancelled */
		if (load_save_state == LOADSAVE_SAVE)
			ui_popup("Save cancelled");
		else
			ui_popup("Load cancelled");

		/* reset the state */
		load_save_state = 0;
		mame_pause(FALSE);
		return 0;
	}

	/* fetch a code; if it's none, we're done */
	code = code_read_async();
	if (code == CODE_NONE)
		return 1;

	/* check for A-Z or 0-9 */
	if (code >= KEYCODE_A && code <= KEYCODE_Z)
		file = code - KEYCODE_A + 'a';
	if (code >= KEYCODE_0 && code <= KEYCODE_9)
		file = code - KEYCODE_0 + '0';
	if (code >= KEYCODE_0_PAD && code <= KEYCODE_9_PAD)
		file = code - KEYCODE_0_PAD + '0';
	if (!file)
		return 1;

	/* display a popup indicating that the save will proceed */
	sprintf(filename, "%s-%c", Machine->gamedrv->name, file);
	if (load_save_state == LOADSAVE_SAVE)
	{
		ui_popup("Save to position %c", file);
		mame_schedule_save(filename);
	}
	else
	{
		ui_popup("Load from position %c", file);
		mame_schedule_load(filename);
	}

	/* remove the pause and reset the state */
	load_save_state = LOADSAVE_NONE;
	mame_pause(FALSE);
	return 0;
}


void ui_display_fps(void)
{
	int ui_width, ui_height;

	/* if we're not currently displaying, skip it */
	if (!showfps && !showfpstemp)
		return;

	ui_get_bounds(&ui_width, &ui_height);

	/* get the current FPS text */
	ui_draw_text_full(osd_get_fps_text(mame_get_performance_info()), 0, 0, ui_width,
				JUSTIFY_RIGHT, WRAP_WORD, DRAW_OPAQUE, RGB_WHITE, RGB_BLACK, NULL, NULL);

	/* update the temporary FPS display state */
	if (showfpstemp)
	{
		showfpstemp--;
		if (!showfps && showfpstemp == 0)
			schedule_full_refresh();
	}
}

static void ui_display_profiler(void)
{
	int ui_width, ui_height;

	if (show_profiler)
	{
		ui_get_bounds(&ui_width, &ui_height);
		ui_draw_text_full(profiler_get_text(), 0, 0, ui_width, JUSTIFY_LEFT, WRAP_WORD, DRAW_OPAQUE, RGB_WHITE, RGB_BLACK, NULL, NULL);
	}
}

static void ui_display_popup(void)
{
	/* show popup message if any */
	if (popup_text_counter > 0)
	{
		draw_multiline_text_box(popup_text, JUSTIFY_CENTER, 0.5f, 0.9f);

		if (--popup_text_counter == 0)
			schedule_full_refresh();
	}
}






/*************************************
 *
 *  Temporary rendering system
 *
 *************************************/

static void ui_raw2rot_rect(rectangle *rect)
{
	int temp, w, h;

	/* get the effective screen size, including artwork */
	artwork_get_screensize(&w, &h);

	/* apply X flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		temp = w - rect->min_x - 1;
		rect->min_x = w - rect->max_x - 1;
		rect->max_x = temp;
	}

	/* apply Y flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		temp = h - rect->min_y - 1;
		rect->min_y = h - rect->max_y - 1;
		rect->max_y = temp;
	}

	/* apply X/Y swap first */
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		temp = rect->min_x; rect->min_x = rect->min_y; rect->min_y = temp;
		temp = rect->max_x; rect->max_x = rect->max_y; rect->max_y = temp;
	}
}


static void ui_rot2raw_rect(rectangle *rect)
{
	int temp, w, h;

	/* get the effective screen size, including artwork */
	artwork_get_screensize(&w, &h);

	/* apply X/Y swap first */
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		temp = rect->min_x; rect->min_x = rect->min_y; rect->min_y = temp;
		temp = rect->max_x; rect->max_x = rect->max_y; rect->max_y = temp;
	}

	/* apply X flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		temp = w - rect->min_x - 1;
		rect->min_x = w - rect->max_x - 1;
		rect->max_x = temp;
	}

	/* apply Y flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		temp = h - rect->min_y - 1;
		rect->min_y = h - rect->max_y - 1;
		rect->max_y = temp;
	}
}


static void add_line(int x1, int y1, int x2, int y2, rgb_t color)
{
	if (elemindex < ARRAY_LENGTH(elemlist))
	{
		elemlist[elemindex].x = (x1 < x2) ? x1 : x2;
		elemlist[elemindex].y = (y1 < y2) ? y1 : y2;
		elemlist[elemindex].x2 = (x1 < x2) ? x2 : x1;
		elemlist[elemindex].y2 = (y1 < y2) ? y2 : y1;
		elemlist[elemindex].type = 0xfffe;
		elemlist[elemindex].color = color;
		elemindex++;
	}
}


static void add_fill(int left, int top, int right, int bottom, rgb_t color)
{
	add_line(left, top, right, bottom, color);
}


static void add_char(int x, int y, UINT16 ch, int color)
{
	if (elemindex < ARRAY_LENGTH(elemlist))
	{
		elemlist[elemindex].x = x;
		elemlist[elemindex].y = y;
		elemlist[elemindex].type = ch;
		elemlist[elemindex].color = color;
		elemindex++;
	}
}


static void add_filled_box(int x1, int y1, int x2, int y2)
{
	add_fill(x1 + 1, y1 + 1, x2 - 1, y2 - 1, RGB_BLACK);

	add_line(x1, y1, x2, y1, RGB_WHITE);
	add_line(x2, y1, x2, y2, RGB_WHITE);
	add_line(x2, y2, x1, y2, RGB_WHITE);
	add_line(x1, y2, x1, y1, RGB_WHITE);
}


static void render_ui(mame_bitmap *dest)
{
	int i;

	uirotfont->colortable[0] = get_black_pen();
	uirotfont->colortable[1] = get_white_pen();
	uirotfont->colortable[2] = get_white_pen();
	uirotfont->colortable[3] = get_black_pen();

	for (i = 0; i < elemindex; i++)
	{
		render_element *elem = &elemlist[i];
		rectangle bounds;

		switch (elem->type)
		{
			case 0xffff:	/* box */
			case 0xfffe:	/* line */
				bounds.min_x = uirotbounds.min_x + elem->x;
				bounds.min_y = uirotbounds.min_y + elem->y;
				bounds.max_x = uirotbounds.min_x + elem->x2;
				bounds.max_y = uirotbounds.min_y + elem->y2;
				ui_rot2raw_rect(&bounds);
				sect_rect(&bounds, &uirawbounds);
				fillbitmap(dest, elem->color ? get_white_pen() : get_black_pen(), &bounds);
				artwork_mark_ui_dirty(bounds.min_x, bounds.min_y, bounds.max_x, bounds.max_y);
				ui_dirty = 5;
				break;

			default:
				bounds.min_x = uirotbounds.min_x + elem->x;
				bounds.min_y = uirotbounds.min_y + elem->y + 1;
				bounds.max_x = bounds.min_x + uirotcharwidth - 1;
				bounds.max_y = bounds.min_y + uirotcharheight - 1;
				ui_rot2raw_rect(&bounds);
				drawgfx(dest, uirotfont, elem->type, elem->color ? 0 : 1, 0, 0, bounds.min_x, bounds.min_y, &uirawbounds, TRANSPARENCY_PEN, 0);
				break;
		}
	}

	elemindex = 0;
}
