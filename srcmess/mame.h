/***************************************************************************

    mame.h

    Controls execution of the core MAME system.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __MAME_H__
#define __MAME_H__

#include "mamecore.h"

#ifdef MESS
#include "device.h"
#endif /* MESS */



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* program phases */
#define MAME_PHASE_PREINIT		0
#define MAME_PHASE_INIT			1
#define MAME_PHASE_RESET		2
#define MAME_PHASE_RUNNING		3
#define MAME_PHASE_EXIT			4


/* maxima */
#define MAX_GFX_ELEMENTS		32
#define MAX_MEMORY_REGIONS		32


/* MESS vs. MAME abstractions */
#ifndef MESS
#define APPNAME					"MAME"
#define CONFIGNAME				"mame"
#define APPLONGNAME				"M.A.M.E."
#define CAPGAMENOUN				"GAME"
#define CAPSTARTGAMENOUN		"Game"
#define GAMENOUN				"game"
#define GAMESNOUN				"games"
#define HISTORYNAME				"History"
#else
#define APPNAME					"MESS"
#define CONFIGNAME				"mess"
#define APPLONGNAME				"M.E.S.S."
#define CAPGAMENOUN				"SYSTEM"
#define CAPSTARTGAMENOUN		"System"
#define GAMENOUN				"system"
#define GAMESNOUN				"systems"
#define HISTORYNAME				"System Info"
#endif


/* memory region types */
enum
{
	REGION_INVALID = 0x80,
	REGION_CPU1,
	REGION_CPU2,
	REGION_CPU3,
	REGION_CPU4,
	REGION_CPU5,
	REGION_CPU6,
	REGION_CPU7,
	REGION_CPU8,
	REGION_GFX1,
	REGION_GFX2,
	REGION_GFX3,
	REGION_GFX4,
	REGION_GFX5,
	REGION_GFX6,
	REGION_GFX7,
	REGION_GFX8,
	REGION_PROMS,
	REGION_SOUND1,
	REGION_SOUND2,
	REGION_SOUND3,
	REGION_SOUND4,
	REGION_SOUND5,
	REGION_SOUND6,
	REGION_SOUND7,
	REGION_SOUND8,
	REGION_USER1,
	REGION_USER2,
	REGION_USER3,
	REGION_USER4,
	REGION_USER5,
	REGION_USER6,
	REGION_USER7,
	REGION_USER8,
	REGION_DISKS,
	REGION_PLDS,
	REGION_MAX
};

extern const char *memory_region_names[REGION_MAX];

/* artwork options */
#define ARTWORK_USE_ALL			(~0)
#define ARTWORK_USE_NONE		(0)
#define ARTWORK_USE_BACKDROPS	0x01
#define ARTWORK_USE_OVERLAYS	0x02
#define ARTWORK_USE_BEZELS		0x04



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* description of the currently-running machine */
typedef struct _running_machine running_machine;
struct _running_machine
{
	/* game-related information */
	const game_driver *		gamedrv;			/* points to the definition of the game machine */
	const machine_config *	drv;				/* points to the constructed machine_config */

	/* video-related information */
	gfx_element *			gfx[MAX_GFX_ELEMENTS];/* array of pointers to graphic sets (chars, sprites) */
	rectangle 				visible_area;		/* current visible area, and a prerotated one adjusted for orientation */
	rectangle				absolute_visible_area;
	float					refresh_rate;		/* current video refresh rate */
	pen_t *					pens;				/* remapped palette pen numbers */
	UINT16 *				game_colortable;	/* lookup table used to map gfx pen numbers to color numbers */
	pen_t *					remapped_colortable;/* the above, already remapped through Machine->pens */
	int						color_depth;		/* video color depth: 16, 15 or 32 */

	/* audio-related information */
	int						sample_rate;		/* the digital audio sample rate */

	/* input-related information */
	input_port_entry *		input_ports;		/* the input ports definition from the driver is copied here and modified */
	mame_file *				record_file;		/* recording file (NULL if not recording) */
	mame_file *				playback_file;		/* playback file (NULL if not recording) */

	/* ui-related information */
	int 					ui_orientation;		/* user interface orientation */

	/* debugger-related information */
	int						debug_mode;			/* was debug mode enabled? */
#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	mame_bitmap *			debug_bitmap;		/* bitmap where the debugger is rendered */
	pen_t *					debug_pens;			/* pen array for the debugger, analagous to the pens above */
	pen_t *					debug_remapped_colortable;/* colortable mapped through the pens, as for the game */
	gfx_element *			debugger_font;		/* font used by the debugger */
#endif

	/* MESS-specific information */
#ifdef MESS
	struct IODevice *		devices;
#endif /* MESS */
};



/***************************************************************************

    Options passed from the frontend to the main core

***************************************************************************/

#ifdef MESS
/*
 * This is a filename and it's associated peripheral type
 * The types are defined in device.h (IO_...)
 */
struct ImageFile
{
	const char *name;
	iodevice_t device_type;
	const char *device_tag;
	int device_index;
};
#endif /* MESS */


/* The host platform should fill these fields with the preferences specified in the GUI */
/* or on the commandline. */
typedef struct _global_options global_options;
struct _global_options
{
	mame_file *	record;			/* handle to file to record input to */
	mame_file *	playback;		/* handle to file to playback input from */
	mame_file *	language_file;	/* handle to file for localization */
	mame_file *	logfile;		/* handle to file for debug logging */

	int		mame_debug;		/* 1 to enable debugging */
	int		cheat;			/* 1 to enable cheating */
	int 	gui_host;		/* 1 to tweak some UI-related things for better GUI integration */
	int 	skip_disclaimer;	/* 1 to skip the disclaimer screen at startup */
	int 	skip_gameinfo;		/* 1 to skip the game info screen at startup */
	int 	skip_warnings;		/* 1 to skip the warnings screen at startup */

	int		samplerate;		/* sound sample playback rate, in Hz */
	int		use_samples;	/* 1 to enable external .wav samples */

	float	brightness;		/* brightness of the display */
	float	pause_bright;		/* additional brightness when in pause */
	float	gamma;			/* gamma correction of the display */
	int		vector_width;	/* requested width for vector games; 0 means default (640) */
	int		vector_height;	/* requested height for vector games; 0 means default (480) */
	int		ui_orientation;	/* orientation of the UI relative to the video */

	int		beam;			/* vector beam width */
	float	vector_flicker;	/* vector beam flicker effect control */
	float	vector_intensity;/* vector beam intensity */
	int		translucency;	/* 1 to enable translucency on vectors */
	int 	antialias;		/* 1 to enable antialiasing on vectors */

	int		use_artwork;	/* bitfield indicating which artwork pieces to use */
	int		artwork_res;	/* 1 for 1x game scaling, 2 for 2x */
	int		artwork_crop;	/* 1 to crop artwork to the game screen */

	const char * savegame;	/* string representing a savegame to load; if one length then interpreted as a character */
	int		auto_save;		/* 1 to automatically save/restore at startup/quitting time */
	char *	bios;			/* specify system bios (if used), 0 is default */

	int		debug_width;	/* requested width of debugger bitmap */
	int		debug_height;	/* requested height of debugger bitmap */
	int		debug_depth;	/* requested depth of debugger bitmap */

	const char *controller;	/* controller-specific cfg to load */

#ifdef MESS
	UINT32	ram;
	struct ImageFile image_files[32];
	int		image_count;
	int		disable_normal_ui;
	int		min_width;		/* minimum width for the display */
	int		min_height;		/* minimum height for the display */
#endif /* MESS */
};



/***************************************************************************

    Globals referencing the current machine and the global options

***************************************************************************/

extern global_options options;
extern running_machine *Machine;
extern const char *mame_disclaimer;
extern char giant_string_buffer[];

extern char build_version[];



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/


/* ----- core system management ----- */

/* execute a given game by index in the drivers[] array */
int run_game(int game);

/* return the current phase */
int mame_get_phase(void);

/* request callback on termination */
void add_exit_callback(void (*callback)(void));

/* request callback on reset */
void add_reset_callback(void (*callback)(void));

/* request callback on pause */
void add_pause_callback(void (*callback)(int));



/* ----- global system states ----- */

/* schedule an exit */
void mame_schedule_exit(void);

/* schedule a hard reset */
void mame_schedule_hard_reset(void);

/* schedule a soft reset */
void mame_schedule_soft_reset(void);

/* schedule a save */
void mame_schedule_save(const char *filename);

/* schedule a load */
void mame_schedule_load(const char *filename);

/* is a scheduled event pending? */
int mame_is_scheduled_event_pending(void);

/* pause the system */
void mame_pause(int pause);

/* get the current pause state */
int mame_is_paused(void);



/* ----- memory region management ----- */

/* allocate a new memory region */
int new_memory_region(int type, size_t length, UINT32 flags);

/* free an allocated memory region */
void free_memory_region(int num);

/* return a pointer to a specified memory region */
UINT8 *memory_region(int num);

/* return the size (in bytes) of a specified memory region */
size_t memory_region_length(int num);

/* return the type of a specified memory region */
UINT32 memory_region_type(int num);

/* return the flags (defined in romload.h) for a specified memory region */
UINT32 memory_region_flags(int num);



/* ----- resource management ----- */

/* begin tracking resources */
void begin_resource_tracking(void);

/* stop tracking resources and free everything since the last begin */
void end_resource_tracking(void);

/* return the current resource tag */
INLINE int get_resource_tag(void)
{
	extern int resource_tracking_tag;
	return resource_tracking_tag;
}

/* allocate memory and fatalerror if there's a problem */
#define malloc_or_die(s)	_malloc_or_die(s, __FILE__, __LINE__)
void *_malloc_or_die(size_t size, const char *file, int line) ATTR_MALLOC;

/* allocate memory that will be freed at the next end_resource_tracking */
#define auto_malloc(s)		_auto_malloc(s, __FILE__, __LINE__)
void *_auto_malloc(size_t size, const char *file, int line) ATTR_MALLOC;

/* allocate memory and duplicate a string that will be freed at the next end_resource_tracking */
char *auto_strdup(const char *str) ATTR_MALLOC;



/* ----- miscellaneous bits & pieces ----- */

/* log to the standard error.log file */
void CLIB_DECL logerror(const char *text,...) ATTR_PRINTF(1,2);

/* adds a callback to be called on logerror() */
void add_logerror_callback(void (*callback)(const char *));

/* standardized random number generator */
UINT32 mame_rand(void);

/* return the index of the given CPU, or -1 if not found */
int mame_find_cpu_index(const char *tag);



#ifdef MESS
#include "mess.h"
#endif /* MESS */

#endif	/* __MAME_H__ */
