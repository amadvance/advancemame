/***************************************************************************

    mame.c

    Controls execution of the core MAME system.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    Since there has been confusion in the past over the order of
    initialization and other such things, here it is, all spelled out
    as of February, 2006:

    main()
        - does platform-specific init
        - calls run_game() [mame.c]

        run_game() [mame.c]
            - calls mame_validitychecks() [validity.c] to perform validity checks on all compiled drivers
            - calls setjmp to prepare for deep error handling
            - begins resource tracking (level 1)
            - calls create_machine [mame.c] to initialize the Machine structure
            - calls init_machine() [mame.c]

            init_machine() [mame.c]
                - calls cpuintrf_init() [cpuintrf.c] to determine which CPUs are available
                - calls sndintrf_init() [sndintrf.c] to determine which sound chips are available
                - calls fileio_init() [fileio.c] to initialize file I/O info
                - calls config_init() [config.c] to initialize configuration system
                - calls state_init() [state.c] to initialize save state system
                - calls state_save_allow_registration() [state.c] to allow registrations
                - calls drawgfx_init() [drawgfx.c] to initialize rendering globals
                - calls generic_machine_init() [machine/generic.c] to initialize generic machine structures
                - calls generic_video_init() [vidhrdw/generic.c] to initialize generic video structures
                - calls osd_init() [osdepend.h] to do platform-specific initialization
                - calls code_init() [input.c] to initialize the input system
                - calls input_port_init() [inptport.c] to set up the input ports
                - calls rom_init() [romload.c] to load the game's ROMs
                - calls timer_init() [timer.c] to reset the timer system
                - calls memory_init() [memory.c] to process the game's memory maps
                - calls cpuexec_init() [cpuexec.c] to initialize the CPUs
                - calls cpuint_init() [cpuint.c] to initialize the CPU interrupts
                - calls hiscore_init() [hiscore.c] to initialize the hiscores
                - calls saveload_init() [mame.c] to set up for save/load
                - calls the driver's DRIVER_INIT callback
                - calls sound_init() [sound.c] to start the audio system
                - calls video_init() [video.c] to start the video system
                - calls cheat_init() [cheat.c] to initialize the cheat system
                - calls the driver's MACHINE_START, SOUND_START, and VIDEO_START callbacks
                - disposes of regions marked as disposable
                - calls mame_debug_init() [debugcpu.c] to set up the debugger

            - calls config_load_settings() [config.c] to load the configuration file
            - calls nvram_load [machine/generic.c] to load NVRAM
            - calls ui_init() [usrintrf.c] to initialize the user interface
            - begins resource tracking (level 2)
            - calls soft_reset() [mame.c] to reset all systems

                -------------------( at this point, we're up and running )----------------------

            - calls cpuexec_timeslice() [cpuexec.c] over and over until we exit
            - ends resource tracking (level 2), freeing all auto_mallocs and timers
            - calls the nvram_save() [machine/generic.c] to save NVRAM
            - calls config_save_settings() [config.c] to save the game's configuration
            - calls all registered exit routines [mame.c]
            - ends resource tracking (level 1), freeing all auto_mallocs and timers

        - exits the program

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "config.h"
#include "cheat.h"
#include "hiscore.h"
#include "debugger.h"
#include "profiler.h"

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
#include "debug/debugcon.h"
#endif

#include <stdarg.h>
#include <setjmp.h>



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_MEMORY_REGIONS		32



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _region_info region_info;
struct _region_info
{
	UINT8 *			base;
	size_t			length;
	UINT32			type;
	UINT32			flags;
};


typedef struct _callback_item callback_item;
struct _callback_item
{
	callback_item *	next;
	union
	{
		void		(*exit)(void);
		void		(*reset)(void);
		void		(*pause)(int);
		void		(*log)(const char *);
	} func;
};



/***************************************************************************
    GLOBALS
***************************************************************************/

/* the active machine */
static running_machine active_machine;
running_machine *Machine;

/* the active game driver */
static machine_config internal_drv;

/* various game options filled in by the OSD */
global_options options;

/* system state statics */
static int current_phase;
static UINT8 mame_paused;
static UINT8 hard_reset_pending;
static UINT8 exit_pending;
static char *saveload_pending_file;
static mame_timer *soft_reset_timer;

/* load/save statics */
static void (*saveload_schedule_callback)(void);
static mame_time saveload_schedule_time;

/* error recovery and exiting */
static callback_item *reset_callback_list;
static callback_item *pause_callback_list;
static callback_item *exit_callback_list;
static jmp_buf fatal_error_jmpbuf;
static int fatal_error_jmpbuf_valid;

/* malloc tracking */
static void **malloc_list = NULL;
static int malloc_list_index = 0;
static int malloc_list_size = 0;

/* resource tracking */
int resource_tracking_tag = 0;

/* array of memory regions */
static region_info mem_region[MAX_MEMORY_REGIONS];

/* random number seed */
static UINT32 rand_seed;

/* logerror calback info */
static callback_item *logerror_callback_list;

/* a giant string buffer for temporary strings */
char giant_string_buffer[65536];

/* the "disclaimer" that should be printed when run with no parameters */
const char *mame_disclaimer =
	"MAME is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
	"several arcade machines. But hardware is useless without software, so an image\n"
	"of the ROMs which run on that hardware is required. Such ROMs, like any other\n"
	"commercial software, are copyrighted material and it is therefore illegal to\n"
	"use them if you don't own the original arcade machine. Needless to say, ROMs\n"
	"are not distributed together with MAME. Distribution of MAME together with ROM\n"
	"images is a violation of copyright law and should be promptly reported to the\n"
	"authors so that appropriate legal action can be taken.\n";

const char *memory_region_names[REGION_MAX] =
{
	"REGION_INVALID",
	"REGION_CPU1",
	"REGION_CPU2",
	"REGION_CPU3",
	"REGION_CPU4",
	"REGION_CPU5",
	"REGION_CPU6",
	"REGION_CPU7",
	"REGION_CPU8",
	"REGION_GFX1",
	"REGION_GFX2",
	"REGION_GFX3",
	"REGION_GFX4",
	"REGION_GFX5",
	"REGION_GFX6",
	"REGION_GFX7",
	"REGION_GFX8",
	"REGION_PROMS",
	"REGION_SOUND1",
	"REGION_SOUND2",
	"REGION_SOUND3",
	"REGION_SOUND4",
	"REGION_SOUND5",
	"REGION_SOUND6",
	"REGION_SOUND7",
	"REGION_SOUND8",
	"REGION_USER1",
	"REGION_USER2",
	"REGION_USER3",
	"REGION_USER4",
	"REGION_USER5",
	"REGION_USER6",
	"REGION_USER7",
	"REGION_USER8",
	"REGION_DISKS",
	"REGION_PLDS"
};


/***************************************************************************
    PROTOTYPES
***************************************************************************/

extern int mame_validitychecks(int game);

static void create_machine(int game);
static void destroy_machine(void);
static void init_machine(void);
static void soft_reset(int param);
static void free_callback_list(callback_item **cb);

static void saveload_init(void);
static void handle_save(void);
static void handle_load(void);


static void logfile_callback(const char *buffer);


/***************************************************************************

    Core system management

***************************************************************************/

/*-------------------------------------------------
    run_game - run the given game in a session
-------------------------------------------------*/

int run_game(int game)
{
	callback_item *cb;
	int error = 0;

	/* start in the "pre-init phase" */
	current_phase = MAME_PHASE_PREINIT;

	/* perform validity checks before anything else */
	if (mame_validitychecks(game) != 0)
		return 1;

	/* loop across multiple hard resets */
	exit_pending = FALSE;
	while (error == 0 && !exit_pending)
	{
		/* use setjmp/longjmp for deep error recovery */
		fatal_error_jmpbuf_valid = TRUE;
		error = setjmp(fatal_error_jmpbuf);
		if (error == 0)
		{
			int settingsloaded;

			/* move to the init phase */
			current_phase = MAME_PHASE_INIT;

			/* start tracking resources for real */
			begin_resource_tracking();

			/* if we have a logfile, set up the callback */
			logerror_callback_list = NULL;
			if (options.logfile)
				add_logerror_callback(logfile_callback);

			/* create the Machine structure and driver */
			create_machine(game);

			/* then finish setting up our local machine */
			init_machine();

			/* load the configuration settings and NVRAM */
			settingsloaded = config_load_settings();
			nvram_load();

			/* initialize the UI and display the startup screens */
			if (ui_init(!settingsloaded && !options.skip_disclaimer, !options.skip_warnings, !options.skip_gameinfo) != 0)
				fatalerror("User cancelled");

			/* ensure we don't show the opening screens on a reset */
			options.skip_disclaimer = options.skip_warnings = options.skip_gameinfo = TRUE;

			/* start resource tracking; note that soft_reset assumes it can */
			/* call end_resource_tracking followed by begin_resource_tracking */
			/* to clear out resources allocated between resets */
			begin_resource_tracking();

			/* perform a soft reset -- this takes us to the running phase */
			soft_reset(0);

			/* run the CPUs until a reset or exit */
			hard_reset_pending = FALSE;
			while ((!hard_reset_pending && !exit_pending) || saveload_pending_file != NULL)
			{
				profiler_mark(PROFILER_EXTRA);

				/* execute CPUs if not paused */
				if (!mame_paused)
					cpuexec_timeslice();

				/* otherwise, just pump video updates through */
				else
				{
					updatescreen();
					reset_partial_updates();
				}

				/* handle save/load */
				if (saveload_schedule_callback)
					(*saveload_schedule_callback)();

				profiler_mark(PROFILER_END);
			}

			/* and out via the exit phase */
			current_phase = MAME_PHASE_EXIT;

			/* stop tracking resources at this level */
			end_resource_tracking();

			/* save the NVRAM and configuration */
			nvram_save();
			config_save_settings();
		}
		fatal_error_jmpbuf_valid = FALSE;

		/* call all exit callbacks registered */
		for (cb = exit_callback_list; cb; cb = cb->next)
			(*cb->func.exit)();

		/* close all inner resource tracking */
		while (resource_tracking_tag != 0)
			end_resource_tracking();

		/* free our callback lists */
		free_callback_list(&exit_callback_list);
		free_callback_list(&reset_callback_list);
		free_callback_list(&pause_callback_list);
	}

	/* return an error */
	return error;
}


/*-------------------------------------------------
    mame_get_phase - return the current program
    phase
-------------------------------------------------*/

int mame_get_phase(void)
{
	return current_phase;
}


/*-------------------------------------------------
    add_exit_callback - request a callback on
    termination
-------------------------------------------------*/

void add_exit_callback(void (*callback)(void))
{
	callback_item *cb;

	assert_always(mame_get_phase() == MAME_PHASE_INIT, "Can only call add_exit_callback at init time!");

	/* allocate memory */
	cb = malloc_or_die(sizeof(*cb));

	/* add us to the head of the list */
	cb->func.exit = callback;
	cb->next = exit_callback_list;
	exit_callback_list = cb;
}


/*-------------------------------------------------
    add_reset_callback - request a callback on
    reset
-------------------------------------------------*/

void add_reset_callback(void (*callback)(void))
{
	callback_item *cb, **cur;

	assert_always(mame_get_phase() == MAME_PHASE_INIT, "Can only call add_reset_callback at init time!");

	/* allocate memory */
	cb = malloc_or_die(sizeof(*cb));

	/* add us to the end of the list */
	cb->func.reset = callback;
	cb->next = NULL;
	for (cur = &reset_callback_list; *cur; cur = &(*cur)->next) ;
	*cur = cb;
}


/*-------------------------------------------------
    add_pause_callback - request a callback on
    pause
-------------------------------------------------*/

void add_pause_callback(void (*callback)(int))
{
	callback_item *cb, **cur;

	assert_always(mame_get_phase() == MAME_PHASE_INIT, "Can only call add_pause_callback at init time!");

	/* allocate memory */
	cb = malloc_or_die(sizeof(*cb));

	/* add us to the end of the list */
	cb->func.pause = callback;
	cb->next = NULL;
	for (cur = &pause_callback_list; *cur; cur = &(*cur)->next) ;
	*cur = cb;
}



/***************************************************************************

    Global System States

***************************************************************************/

/*-------------------------------------------------
    mame_schedule_exit - schedule a clean exit
-------------------------------------------------*/

void mame_schedule_exit(void)
{
	exit_pending = TRUE;

	/* if we're autosaving on exit, schedule a save as well */
	if (options.auto_save && (Machine->gamedrv->flags & GAME_SUPPORTS_SAVE))
		mame_schedule_save(Machine->gamedrv->name);
}


/*-------------------------------------------------
    mame_schedule_hard_reset - schedule a hard-
    reset of the system
-------------------------------------------------*/

void mame_schedule_hard_reset(void)
{
	hard_reset_pending = TRUE;
}


/*-------------------------------------------------
    mame_schedule_soft_reset - schedule a soft-
    reset of the system
-------------------------------------------------*/

void mame_schedule_soft_reset(void)
{
	mame_timer_adjust(soft_reset_timer, time_zero, 0, time_zero);

	/* we can't be paused since the timer needs to fire */
	mame_pause(FALSE);
}


/*-------------------------------------------------
    mame_schedule_save - schedule a save to
    occur as soon as possible
-------------------------------------------------*/

void mame_schedule_save(const char *filename)
{
	/* free any existing request and allocate a copy of the requested name */
	if (saveload_pending_file != NULL)
		free(saveload_pending_file);
	saveload_pending_file = mame_strdup(filename);

	/* note the start time and set a timer for the next timeslice to actually schedule it */
	saveload_schedule_callback = handle_save;
	saveload_schedule_time = mame_timer_get_time();

	/* we can't be paused since we need to clear out anonymous timers */
	mame_pause(FALSE);
}


/*-------------------------------------------------
    mame_schedule_load - schedule a load to
    occur as soon as possible
-------------------------------------------------*/

void mame_schedule_load(const char *filename)
{
	/* free any existing request and allocate a copy of the requested name */
	if (saveload_pending_file != NULL)
		free(saveload_pending_file);
	saveload_pending_file = mame_strdup(filename);

	/* note the start time and set a timer for the next timeslice to actually schedule it */
	saveload_schedule_callback = handle_load;
	saveload_schedule_time = mame_timer_get_time();

	/* we can't be paused since we need to clear out anonymous timers */
	mame_pause(FALSE);
}


/*-------------------------------------------------
    mame_is_scheduled_event_pending - is a
    scheduled event pending?
-------------------------------------------------*/

int mame_is_scheduled_event_pending(void)
{
	/* we can't check for saveload_pending_file here because it will bypass */
	/* required UI screens if a state is queued from the command line */
	return exit_pending || hard_reset_pending;
}


/*-------------------------------------------------
    mame_pause - pause or resume the system
-------------------------------------------------*/

void mame_pause(int pause)
{
	callback_item *cb;

	/* ignore if nothing has changed */
	if (mame_paused == pause)
		return;
	mame_paused = pause;

	/* call all registered pause callbacks */
	for (cb = pause_callback_list; cb; cb = cb->next)
		(*cb->func.pause)(mame_paused);
}


/*-------------------------------------------------
    mame_is_paused - the system paused?
-------------------------------------------------*/

int mame_is_paused(void)
{
	return mame_paused;
}



/***************************************************************************

    Memory region code

***************************************************************************/

/*-------------------------------------------------
    memory_region_to_index - returns an index
    given either an index or a REGION_* identifier
-------------------------------------------------*/

int memory_region_to_index(int num)
{
	int i;

	/* if we're already an index, stop there */
	if (num < MAX_MEMORY_REGIONS)
		return num;

	/* scan for a match */
	for (i = 0; i < MAX_MEMORY_REGIONS; i++)
		if (mem_region[i].type == num)
			return i;

	return -1;
}


/*-------------------------------------------------
    new_memory_region - allocates memory for a
    region
-------------------------------------------------*/

int new_memory_region(int type, size_t length, UINT32 flags)
{
    int num;

    assert(type >= MAX_MEMORY_REGIONS);

    /* find a free slot */
	for (num = 0; num < MAX_MEMORY_REGIONS; num++)
		if (mem_region[num].base == NULL)
			break;
	if (num < 0)
		return 1;

    /* allocate the region */
	mem_region[num].length = length;
	mem_region[num].type = type;
	mem_region[num].flags = flags;
	mem_region[num].base = malloc(length);
	return (mem_region[num].base == NULL) ? 1 : 0;
}


/*-------------------------------------------------
    free_memory_region - releases memory for a
    region
-------------------------------------------------*/

void free_memory_region(int num)
{
	/* convert to an index and bail if invalid */
	num = memory_region_to_index(num);
	if (num < 0)
		return;

	/* free the region in question */
	free(mem_region[num].base);
	memset(&mem_region[num], 0, sizeof(mem_region[num]));
}


/*-------------------------------------------------
    memory_region - returns pointer to a memory
    region
-------------------------------------------------*/

UINT8 *memory_region(int num)
{
	/* convert to an index and return the result */
	num = memory_region_to_index(num);
	return (num >= 0) ? mem_region[num].base : NULL;
}


/*-------------------------------------------------
    memory_region_length - returns length of a
    memory region
-------------------------------------------------*/

size_t memory_region_length(int num)
{
	/* convert to an index and return the result */
	num = memory_region_to_index(num);
	return (num >= 0) ? mem_region[num].length : 0;
}


/*-------------------------------------------------
    memory_region_type - returns the type of a
    memory region
-------------------------------------------------*/

UINT32 memory_region_type(int num)
{
	/* convert to an index and return the result */
	num = memory_region_to_index(num);
	return (num >= 0) ? mem_region[num].type : 0;
}


/*-------------------------------------------------
    memory_region_flags - returns flags for a
    memory region
-------------------------------------------------*/

UINT32 memory_region_flags(int num)
{
	/* convert to an index and return the result */
	num = memory_region_to_index(num);
	return (num >= 0) ? mem_region[num].flags : 0;
}



/***************************************************************************

    Resource tracking code

***************************************************************************/

/*-------------------------------------------------
    auto_malloc_add - add pointer to malloc list
-------------------------------------------------*/

INLINE void auto_malloc_add(void *result)
{
	/* make sure we have tracking space */
	if (malloc_list_index == malloc_list_size)
	{
		void **list;

		/* if this is the first time, allocate 256 entries, otherwise double the slots */
		if (malloc_list_size == 0)
			malloc_list_size = 256;
		else
			malloc_list_size *= 2;

		/* realloc the list */
		list = realloc(malloc_list, malloc_list_size * sizeof(list[0]));
		if (list == NULL)
			fatalerror("Unable to extend malloc tracking array to %d slots", malloc_list_size);
		malloc_list = list;
	}
	malloc_list[malloc_list_index++] = result;
}


/*-------------------------------------------------
    auto_malloc_free - release auto_malloc'd memory
-------------------------------------------------*/

static void auto_malloc_free(void)
{
	/* start at the end and free everything till you reach the sentinel */
	while (malloc_list_index > 0 && malloc_list[--malloc_list_index] != NULL)
		free(malloc_list[malloc_list_index]);

	/* if we free everything, free the list */
	if (malloc_list_index == 0)
	{
		free(malloc_list);
		malloc_list = NULL;
		malloc_list_size = 0;
	}
}


/*-------------------------------------------------
    begin_resource_tracking - start tracking
    resources
-------------------------------------------------*/

void begin_resource_tracking(void)
{
	/* add a NULL as a sentinel */
	auto_malloc_add(NULL);

	/* increment the tag counter */
	resource_tracking_tag++;
}


/*-------------------------------------------------
    end_resource_tracking - stop tracking
    resources
-------------------------------------------------*/

void end_resource_tracking(void)
{
	/* call everyone who tracks resources to let them know */
	auto_malloc_free();
	timer_free();
	state_save_free();

	/* decrement the tag counter */
	resource_tracking_tag--;
}


/*-------------------------------------------------
    auto_malloc - allocate auto-freeing memory
-------------------------------------------------*/

void *_auto_malloc(size_t size, const char *file, int line)
{
	void *result;

	/* fail horribly if it doesn't work */
	result = _malloc_or_die(size, file, line);

	/* track this item in our list */
	auto_malloc_add(result);
	return result;
}


/*-------------------------------------------------
    auto_strdup - allocate auto-freeing string
-------------------------------------------------*/

char *auto_strdup(const char *str)
{
	return strcpy(auto_malloc(strlen(str) + 1), str);
}



/***************************************************************************

    Miscellaneous bits & pieces

***************************************************************************/

/*-------------------------------------------------
    fatalerror - print a message and escape back
    to the OSD layer
-------------------------------------------------*/

void CLIB_DECL fatalerror(const char *text, ...)
{
	va_list arg;

	/* dump to the buffer; assume no one writes >2k lines this way */
	va_start(arg, text);
	vsnprintf(giant_string_buffer, sizeof(giant_string_buffer), text, arg);
	va_end(arg);

	/* output and return */
	printf("%s\n", giant_string_buffer);
	if (fatal_error_jmpbuf_valid)
  		longjmp(fatal_error_jmpbuf, 1);
	else
		exit(-1);
}


/*-------------------------------------------------
    logerror - log to the debugger and any other
    OSD-defined output streams
-------------------------------------------------*/

void CLIB_DECL logerror(const char *text, ...)
{
	callback_item *cb;

	/* process only if there is a target */
	if (logerror_callback_list)
	{
		va_list arg;

		profiler_mark(PROFILER_LOGERROR);

		/* dump to the buffer */
		va_start(arg, text);
		vsnprintf(giant_string_buffer, sizeof(giant_string_buffer), text, arg);
		va_end(arg);

		/* log to all callbacks */
		for (cb = logerror_callback_list; cb; cb = cb->next)
			cb->func.log(giant_string_buffer);

		profiler_mark(PROFILER_END);
	}
}


/*-------------------------------------------------
    add_logerror_callback - adds a callback to be
    called on logerror()
-------------------------------------------------*/

void add_logerror_callback(void (*callback)(const char *))
{
	callback_item *cb, **cur;

	assert_always(mame_get_phase() == MAME_PHASE_INIT, "Can only call add_logerror_callback at init time!");

	cb = auto_malloc(sizeof(*cb));
	cb->func.log = callback;
	cb->next = NULL;

	for (cur = &logerror_callback_list; *cur; cur = &(*cur)->next) ;
	*cur = cb;
}


/*-------------------------------------------------
    logfile_callback - callback for logging to
    logfile
-------------------------------------------------*/

static void logfile_callback(const char *buffer)
{
	if (options.logfile)
		mame_fputs(options.logfile, buffer);
}


/*-------------------------------------------------
    _malloc_or_die - allocate memory or die
    trying
-------------------------------------------------*/

void *_malloc_or_die(size_t size, const char *file, int line)
{
	void *result;

	/* fail on attempted allocations of 0 */
	if (size == 0)
		fatalerror("Attempted to malloc zero bytes (%s:%d)", file, line);

	/* allocate and return if we succeeded */
	result = malloc(size);
	if (result != NULL)
		return result;

	/* otherwise, die horribly */
	fatalerror("Failed to allocate %d bytes (%s:%d)", (int)size, file, line);
}


/*-------------------------------------------------
    mame_find_cpu_index - return the index of the
    given CPU, or -1 if not found
-------------------------------------------------*/

int mame_find_cpu_index(const char *tag)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (Machine->drv->cpu[cpunum].tag && strcmp(Machine->drv->cpu[cpunum].tag, tag) == 0)
			return cpunum;

	return -1;
}


/*-------------------------------------------------
    mame_rand - standardized random numbers
-------------------------------------------------*/

UINT32 mame_rand(void)
{
	rand_seed = 1664525 * rand_seed + 1013904223;
	return rand_seed;
}



/***************************************************************************

    Internal initialization logic

***************************************************************************/

/*-------------------------------------------------
    create_machine - create the running machine
    object and initialize it based on options
-------------------------------------------------*/

static void create_machine(int game)
{
	/* first give the machine a good cleaning */
	Machine = &active_machine;
	memset(Machine, 0, sizeof(*Machine));

	/* initialize the driver-related variables in the Machine */
	Machine->gamedrv = drivers[game];
	Machine->drv = &internal_drv;
	expand_machine_driver(Machine->gamedrv->drv, &internal_drv);
	Machine->refresh_rate = Machine->drv->frames_per_second;

	/* copy some settings into easier-to-handle variables */
	Machine->record_file = options.record;
	Machine->playback_file = options.playback;
	Machine->debug_mode = options.mame_debug;

	/* determine the color depth */
	Machine->color_depth = 16;
	if (Machine->drv->video_attributes & VIDEO_RGB_DIRECT)
		Machine->color_depth = (Machine->drv->video_attributes & VIDEO_NEEDS_6BITS_PER_GUN) ? 32 : 15;

	/* update the vector width/height with defaults */
	if (options.vector_width == 0)
		options.vector_width = 640;
	if (options.vector_height == 0)
		options.vector_height = 480;

	/* initialize the samplerate */
	Machine->sample_rate = options.samplerate;

	/* get orientation right */
	Machine->ui_orientation = options.ui_orientation;

	/* add an exit callback to clear out the Machine on the way out */
	add_exit_callback(destroy_machine);
}


/*-------------------------------------------------
    destroy_machine - "destroy" the Machine by
    NULLing it out
-------------------------------------------------*/

static void destroy_machine(void)
{
	Machine = NULL;
}


/*-------------------------------------------------
    init_machine - initialize the emulated machine
-------------------------------------------------*/

static void init_machine(void)
{
	int num;

	/* initialize basic can't-fail systems here */
	cpuintrf_init();
	sndintrf_init();
	fileio_init();
	config_init();
	state_init();
	state_save_allow_registration(TRUE);
	drawgfx_init();
	generic_machine_init();
	generic_video_init();
	rand_seed = 0x9d14abd7;

	/* init the osd layer */
	if (osd_init() != 0)
		fatalerror("osd_init failed");

	/* initialize the input system */
	/* this must be done before the input ports are initialized */
	if (code_init() != 0)
		fatalerror("code_init failed");

	/* initialize the input ports for the game */
	/* this must be done before memory_init in order to allow specifying */
	/* callbacks based on input port tags */
	if (input_port_init(Machine->gamedrv->construct_ipt) != 0)
		fatalerror("input_port_init failed");

	/* load the ROMs if we have some */
	/* this must be done before memory_init in order to allocate memory regions */
	if (rom_init(Machine->gamedrv->rom) != 0)
		fatalerror("rom_init failed");

	/* initialize the timers and allocate a soft_reset timer */
	/* this must be done before cpu_init so that CPU's can allocate timers */
	timer_init();
	soft_reset_timer = timer_alloc(soft_reset);

	/* initialize the memory system for this game */
	/* this must be done before cpu_init so that set_context can look up the opcode base */
	if (memory_init() != 0)
		fatalerror("memory_init failed");

	/* now set up all the CPUs */
	if (cpuexec_init() != 0)
		fatalerror("cpuexec_init failed");
	if (cpuint_init() != 0)
		fatalerror("cpuint_init failed");

#ifdef MESS
	/* initialize the devices */
	if (devices_init(Machine->gamedrv))
		fatalerror("devices_init failed");
#endif

	/* start the hiscore system -- remove me */
	hiscore_init(Machine->gamedrv->name);

	/* start the save/load system */
	saveload_init();

	/* call the game driver's init function */
	/* this is where decryption is done and memory maps are altered */
	/* so this location in the init order is important */
	if (Machine->gamedrv->driver_init != NULL)
		(*Machine->gamedrv->driver_init)();

	/* start the audio system */
	if (sound_init() != 0)
		fatalerror("sound_init failed");

	/* start the video hardware */
	if (video_init() != 0)
		fatalerror("video_init failed");

	/* start the cheat engine */
	if (options.cheat)
		cheat_init();

	/* call the driver's _START callbacks */
	if (Machine->drv->machine_start != NULL && (*Machine->drv->machine_start)() != 0)
		fatalerror("Unable to start machine emulation");
	if (Machine->drv->sound_start != NULL && (*Machine->drv->sound_start)() != 0)
		fatalerror("Unable to start sound emulation");
	if (Machine->drv->video_start != NULL && (*Machine->drv->video_start)() != 0)
		fatalerror("Unable to start video emulation");

	/* free memory regions allocated with REGIONFLAG_DISPOSE (typically gfx roms) */
	for (num = 0; num < MAX_MEMORY_REGIONS; num++)
		if (mem_region[num].flags & ROMREGION_DISPOSE)
			free_memory_region(num);

#ifdef MAME_DEBUG
	/* initialize the debugger */
	if (Machine->debug_mode)
		mame_debug_init();
#endif
}


/*-------------------------------------------------
    soft_reset - actually perform a soft-reset
    of the system
-------------------------------------------------*/

static void soft_reset(int param)
{
	callback_item *cb;

	logerror("Soft reset\n");

	/* temporarily in the reset phase */
	current_phase = MAME_PHASE_RESET;

	/* a bit gross -- back off of the resource tracking, and put it back at the end */
	assert(resource_tracking_tag == 2);
	end_resource_tracking();
	begin_resource_tracking();

	/* allow save state registrations during the reset */
	state_save_allow_registration(TRUE);

	/* unfortunately, we can't rely on callbacks to reset the interrupt */
	/* structures, as these need to happen before we call the reset */
	/* functions registered by the drivers */
	cpuint_reset();

	/* run the driver's reset callbacks */
	if (Machine->drv->machine_reset != NULL)
		(*Machine->drv->machine_reset)();
	if (Machine->drv->sound_reset != NULL)
		(*Machine->drv->sound_reset)();
	if (Machine->drv->video_reset != NULL)
		(*Machine->drv->video_reset)();

	/* call all registered reset callbacks */
	for (cb = reset_callback_list; cb; cb = cb->next)
		(*cb->func.reset)();

	/* disallow save state registrations starting here */
	state_save_allow_registration(FALSE);

	/* now we're running */
	current_phase = MAME_PHASE_RUNNING;

	/* set the global time to the current time */
	/* this allows 0-time queued callbacks to run before any CPUs execute */
	mame_timer_set_global_time(mame_timer_get_time());
}


/*-------------------------------------------------
    free_callback_list - free a list of callbacks
-------------------------------------------------*/

static void free_callback_list(callback_item **cb)
{
	while (*cb)
	{
		callback_item *temp = *cb;
		*cb = (*cb)->next;
		free(temp);
	}
}



/***************************************************************************

    Save/restore

***************************************************************************/

/*-------------------------------------------------
    saveload_init - initialize the save/load logic
-------------------------------------------------*/

static void saveload_init(void)
{
	/* if we're coming in with a savegame request, process it now */
	if (options.savegame)
	{
		char name[20];

		if (strlen(options.savegame) == 1)
		{
			sprintf(name, "%s-%c", Machine->gamedrv->name, options.savegame[0]);
			mame_schedule_load(name);
		}
		else
			mame_schedule_load(options.savegame);
	}

	/* if we're in autosave mode, schedule a load */
	else if (options.auto_save && (Machine->gamedrv->flags & GAME_SUPPORTS_SAVE))
		mame_schedule_load(Machine->gamedrv->name);
}


/*-------------------------------------------------
    handle_save - attempt to perform a save
-------------------------------------------------*/

static void handle_save(void)
{
	mame_file *file;

	/* if no name, bail */
	if (saveload_pending_file == NULL)
	{
		saveload_schedule_callback = NULL;
		return;
	}

	/* if there are anonymous timers, we can't save just yet */
	if (timer_count_anonymous() > 0)
	{
		/* if more than a second has passed, we're probably screwed */
		if (sub_mame_times(mame_timer_get_time(), saveload_schedule_time).seconds > 0)
		{
			ui_popup("Unable to save due to pending anonymous timers. See error.log for details.");
			goto cancel;
		}
		return;
	}

	/* open the file */
	file = mame_fopen(Machine->gamedrv->name, saveload_pending_file, FILETYPE_STATE, 1);
	if (file)
	{
		int cpunum;

		/* write the save state */
		if (state_save_save_begin(file) != 0)
		{
			ui_popup("Error: Unable to save state due to illegal registrations. See error.log for details.");
			mame_fclose(file);
			goto cancel;
		}

		/* write the default tag */
		state_save_push_tag(0);
		state_save_save_continue();
		state_save_pop_tag();

		/* loop over CPUs */
		for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		{
			cpuintrf_push_context(cpunum);

			/* make sure banking is set */
			activecpu_reset_banking();

			/* save the CPU data */
			state_save_push_tag(cpunum + 1);
			state_save_save_continue();
			state_save_pop_tag();

			cpuintrf_pop_context();
		}

		/* finish and close */
		state_save_save_finish();
		mame_fclose(file);

		/* pop a warning if the game doesn't support saves */
		if (!(Machine->gamedrv->flags & GAME_SUPPORTS_SAVE))
			ui_popup("State successfully saved.\nWarning: Save states are not officially supported for this game.");
		else
			ui_popup("State successfully saved.");
	}
	else
		ui_popup("Error: Failed to save state");

cancel:
	/* unschedule the save */
	free(saveload_pending_file);
	saveload_pending_file = NULL;
	saveload_schedule_callback = NULL;
}


/*-------------------------------------------------
    handle_load - attempt to perform a load
-------------------------------------------------*/

static void handle_load(void)
{
	mame_file *file;

	/* if no name, bail */
	if (saveload_pending_file == NULL)
	{
		saveload_schedule_callback = NULL;
		return;
	}

	/* if there are anonymous timers, we can't load just yet because the timers might */
	/* overwrite data we have loaded */
	if (timer_count_anonymous() > 0)
	{
		/* if more than a second has passed, we're probably screwed */
		if (sub_mame_times(mame_timer_get_time(), saveload_schedule_time).seconds > 0)
		{
			ui_popup("Unable to load due to pending anonymous timers. See error.log for details.");
			goto cancel;
		}
		return;
	}

	/* open the file */
	file = mame_fopen(Machine->gamedrv->name, saveload_pending_file, FILETYPE_STATE, 0);
	if (file)
	{
		/* start loading */
		if (state_save_load_begin(file) == 0)
		{
			int cpunum;

			/* read tag 0 */
			state_save_push_tag(0);
			state_save_load_continue();
			state_save_pop_tag();

			/* loop over CPUs */
			for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
			{
				cpuintrf_push_context(cpunum);

				/* make sure banking is set */
				activecpu_reset_banking();

				/* load the CPU data */
				state_save_push_tag(cpunum + 1);
				state_save_load_continue();
				state_save_pop_tag();

				/* make sure banking is set */
				activecpu_reset_banking();

				cpuintrf_pop_context();
			}

			/* finish and close */
			state_save_load_finish();
			ui_popup("State successfully loaded.");
		}
		else
			ui_popup("Error: Failed to load state");
		mame_fclose(file);
	}
	else
		ui_popup("Error: Failed to load state");

cancel:
	/* unschedule the load */
	free(saveload_pending_file);
	saveload_pending_file = NULL;
	saveload_schedule_callback = NULL;
}
