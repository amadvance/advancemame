/***************************************************************************

    cpuexec.c

    Core multi-CPU execution engine.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include <math.h>
#include "driver.h"
#include "cheat.h"
#include "profiler.h"
#include "debugger.h"

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
#include "debug/debugcpu.h"
#endif



/*************************************
 *
 *  Debug logging
 *
 *************************************/

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif



/*************************************
 *
 *  Macros to help verify active CPU
 *
 *************************************/

#define VERIFY_ACTIVECPU(name) \
	int activecpu = cpu_getactivecpu(); \
	assert_always(activecpu >= 0, #name "() called with no active cpu!")

#define VERIFY_EXECUTINGCPU(name) \
	int activecpu = cpu_getexecutingcpu(); \
	assert_always(activecpu >= 0, #name "() called with no executing cpu!")

#define VERIFY_CPUNUM(name) \
	assert_always(cpunum >= 0 && cpunum < cpu_gettotalcpu(), #name "() called for invalid cpu num!")



/*************************************
 *
 *  Triggers for the timer system
 *
 *************************************/

enum
{
	TRIGGER_TIMESLICE 	= -1000,
	TRIGGER_INT 		= -2000,
	TRIGGER_YIELDTIME 	= -3000,
	TRIGGER_SUSPENDTIME = -4000
};



/*************************************
 *
 *  Internal CPU info structure
 *
 *************************************/

typedef struct _cpuexec_data cpuexec_data;
struct _cpuexec_data
{
	UINT8	saveable;				/* true if saveable */

	UINT8	suspend;				/* suspend reason mask (0 = not suspended) */
	UINT8	nextsuspend;			/* pending suspend reason mask */
	UINT8	eatcycles;				/* true if we eat cycles while suspended */
	UINT8	nexteatcycles;			/* pending value */
	INT32	trigger;				/* pending trigger to release a trigger suspension */

	INT32 	iloops; 				/* number of interrupts remaining this frame */

	UINT64 	totalcycles;			/* total CPU cycles executed */
	mame_time localtime;			/* local time, relative to the timer system's global time */
	INT32	clock;					/* current active clock */
	double	clockscale;				/* current active clock scale factor */

	INT32	vblankint_countdown;	/* number of vblank callbacks left until we interrupt */
	INT32 	vblankint_multiplier;	/* number of vblank callbacks per interrupt */
	void *	vblankint_timer;		/* reference to elapsed time counter */
	mame_time vblankint_period;		/* timing period of the VBLANK interrupt */

	void *	timedint_timer;			/* reference to this CPU's timer */
	mame_time timedint_period; 		/* timing period of the timed interrupt */
};



/*************************************
 *
 *  General CPU variables
 *
 *************************************/

static cpuexec_data cpu[MAX_CPU];

static UINT8 vblank;
static UINT32 current_frame;
static INT32 watchdog_counter;

static int cycles_running;
static int cycles_stolen;



/*************************************
 *
 *  Timer variables
 *
 *************************************/

static mame_timer *vblank_timer;
static INT32 vblank_countdown;
static INT32 vblank_multiplier;
static mame_time vblank_period;

static mame_timer *update_timer;

static mame_timer *refresh_timer;
static mame_time refresh_period;

static mame_timer *timeslice_timer;
static mame_time timeslice_period;

static mame_time scanline_period;

static mame_timer *interleave_boost_timer;
static mame_timer *interleave_boost_timer_end;
static mame_time perfect_interleave;

static mame_timer *watchdog_timer;



/*************************************
 *
 *  Static prototypes
 *
 *************************************/

static void cpuexec_exit(void);
static void cpuexec_reset(void);
static void init_refresh_timer(void);
static void cpu_inittimers(void);
static void cpu_vblankreset(void);
static void cpu_vblankcallback(int param);
static void cpu_updatecallback(int param);
static void end_interleave_boost(int param);
static void compute_perfect_interleave(void);
static void watchdog_setup(int alloc_new);



/*************************************
 *
 *  Watchdog Flags
 *
 *************************************/

#define WATCHDOG_IS_STARTED_DISABLED	-1
#define WATCHDOG_IS_DISABLED			-2
#define WATCHDOG_IS_TIMER_BASED			-3
#define WATCHDOG_IS_INVALID				-4
#define WATCHDOG_IS_BEING_STARTED		-5



#if 0
#pragma mark CORE CPU
#endif

/*************************************
 *
 *  Initialize all the CPUs
 *
 *************************************/

int cpuexec_init(void)
{
	int cpunum;

	/* initialize the refresh timer */
	init_refresh_timer();

	/* loop over all our CPUs */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		int cputype = Machine->drv->cpu[cpunum].cpu_type;
		int num_regs;

		/* if this is a dummy, stop looking */
		if (cputype == CPU_DUMMY)
			break;

		/* initialize the cpuinfo struct */
		memset(&cpu[cpunum], 0, sizeof(cpu[cpunum]));
		cpu[cpunum].suspend = SUSPEND_REASON_RESET;
		cpu[cpunum].clock = Machine->drv->cpu[cpunum].cpu_clock;
		cpu[cpunum].clockscale = 1.0;
		cpu[cpunum].localtime = time_zero;

		/* compute the cycle times */
		sec_to_cycles[cpunum] = cpu[cpunum].clockscale * cpu[cpunum].clock;
		cycles_to_sec[cpunum] = 1.0 / sec_to_cycles[cpunum];
		cycles_per_second[cpunum] = sec_to_cycles[cpunum];
		subseconds_per_cycle[cpunum] = MAX_SUBSECONDS / sec_to_cycles[cpunum];

		/* register some of our variables for later */
		state_save_register_item("cpu", cpunum, cpu[cpunum].suspend);
		state_save_register_item("cpu", cpunum, cpu[cpunum].nextsuspend);
		state_save_register_item("cpu", cpunum, cpu[cpunum].eatcycles);
		state_save_register_item("cpu", cpunum, cpu[cpunum].nexteatcycles);
		state_save_register_item("cpu", cpunum, cpu[cpunum].trigger);

		state_save_register_item("cpu", cpunum, cpu[cpunum].iloops);

		state_save_register_item("cpu", cpunum, cpu[cpunum].totalcycles);
		state_save_register_item("cpu", cpunum, cpu[cpunum].localtime.seconds);
		state_save_register_item("cpu", cpunum, cpu[cpunum].localtime.subseconds);
		state_save_register_item("cpu", cpunum, cpu[cpunum].clock);
		state_save_register_item("cpu", cpunum, cpu[cpunum].clockscale);

		state_save_register_item("cpu", cpunum, cpu[cpunum].vblankint_countdown);

		/* initialize this CPU */
		state_save_push_tag(cpunum + 1);
		num_regs = state_save_get_reg_count();
		if (cpuintrf_init_cpu(cpunum, cputype, cpu[cpunum].clock, Machine->drv->cpu[cpunum].reset_param, cpu_irq_callbacks[cpunum]))
			return 1;
		num_regs = state_save_get_reg_count() - num_regs;
		state_save_pop_tag();

		/* if no state registered for saving, we can't save */
		if (num_regs == 0)
		{
			logerror("CPU #%d (%s) did not register any state to save!\n", cpunum, cputype_name(cputype));
			if (Machine->gamedrv->flags & GAME_SUPPORTS_SAVE)
				fatalerror("CPU #%d (%s) did not register any state to save!", cpunum, cputype_name(cputype));
		}
	}
	add_reset_callback(cpuexec_reset);
	add_exit_callback(cpuexec_exit);

	/* compute the perfect interleave factor */
	compute_perfect_interleave();

	/* save some stuff in the default tag */
	state_save_push_tag(0);
	state_save_register_item("cpu", 0, vblank);
	state_save_register_item("cpu", 0, current_frame);
	state_save_register_item("cpu", 0, watchdog_counter);
	state_save_register_item("cpu", 0, vblank_countdown);
	state_save_pop_tag();

	return 0;
}



/*************************************
 *
 *  Prepare the system for execution
 *
 *************************************/

static void cpuexec_reset(void)
{
	int cpunum;

	/* initialize the various timers (suspends all CPUs at startup) */
	cpu_inittimers();
	watchdog_counter = WATCHDOG_IS_INVALID;
	watchdog_setup(TRUE);

	/* reset the osd level */
	osd_reset();

	/* first pass over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		/* enable all CPUs (except for disabled CPUs) */
		if (!(Machine->drv->cpu[cpunum].cpu_flags & CPU_DISABLE))
			cpunum_resume(cpunum, SUSPEND_ANY_REASON);
		else
			cpunum_suspend(cpunum, SUSPEND_REASON_DISABLE, 1);

		/* reset the total number of cycles */
		cpu[cpunum].totalcycles = 0;

		/* then reset the CPU directly */
		cpunum_reset(cpunum);
	}

	/* reset the globals */
	cpu_vblankreset();
	vblank = 0;
	current_frame = 0;
}



/*************************************
 *
 *  Deinitialize all the CPUs
 *
 *************************************/

static void cpuexec_exit(void)
{
	int cpunum;

	/* shut down the CPU cores */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		cpuintrf_exit_cpu(cpunum);
}




#if 0
#pragma mark -
#pragma mark WATCHDOG
#endif

/*************************************
 *
 *  Watchdog timer callback
 *
 *************************************/

static void watchdog_callback(int param)
{
	logerror("reset caused by the (time) watchdog\n");
	mame_schedule_soft_reset();
}



/*************************************
 *
 *  Watchdog setup routine
 *
 *************************************/

static void watchdog_setup(int alloc_new)
{
	if (watchdog_counter != WATCHDOG_IS_DISABLED)
	{
		if (Machine->drv->watchdog_vblank_count)
		{
			/* Start a vblank based watchdog. */
			watchdog_counter = Machine->drv->watchdog_vblank_count;
		}
		else if (Machine->drv->watchdog_time != 0)
		{
			/* Start a time based watchdog. */
			if (alloc_new)
				watchdog_timer = timer_alloc(watchdog_callback);
			timer_adjust(watchdog_timer, Machine->drv->watchdog_time, 0, 0);
			watchdog_counter = WATCHDOG_IS_TIMER_BASED;
		}
		else if (watchdog_counter == WATCHDOG_IS_INVALID)
		{
			/* The watchdog was not initialized in the MACHINE_DRIVER,
             * so we will start with it disabled.
             */
			watchdog_counter = WATCHDOG_IS_STARTED_DISABLED;
		}
		else
		{
			/* The watchdog was not initialized in the MACHINE_DRIVER.
             * But it has been manually started, so we will default to
             * using a vblank watchdog.  We will set up a default time
             * of 3 times the refresh rate.  Which is 3 seconds @ 60Hz
             * refresh.

             * The 3 seconds delay is targeted at qzshowby, which otherwise
             * would reset at the start of a game.
             */
			watchdog_counter = 3 * Machine->refresh_rate;
		}
	}
}



/*************************************
 *
 *  Watchdog reset
 *
 *************************************/

void watchdog_reset(void)
{
	if (watchdog_counter == WATCHDOG_IS_TIMER_BASED)
	{
		timer_reset(watchdog_timer, Machine->drv->watchdog_time);
	}
	else
	{
		if (watchdog_counter == WATCHDOG_IS_STARTED_DISABLED)
		{
			watchdog_counter = WATCHDOG_IS_BEING_STARTED;
			logerror("(vblank) watchdog armed by reset\n");
		}

		watchdog_setup(FALSE);
	}
}



/*************************************
 *
 *  Watchdog enable/disable
 *
 *************************************/

void watchdog_enable(int enable)
{
	if (!enable)
	{
		// Disable all timers
		watchdog_counter = WATCHDOG_IS_DISABLED;
	}
	else
	// Setup only on change from disable to enable.
	// Do not setup if watchdog is disabled from machine init.
	if (watchdog_counter == WATCHDOG_IS_DISABLED)
	{
		watchdog_counter = WATCHDOG_IS_BEING_STARTED;
		watchdog_setup(FALSE);
	}
}



#if 0
#pragma mark -
#pragma mark CPU SCHEDULING
#endif

/*************************************
 *
 *  Execute all the CPUs for one
 *  timeslice
 *
 *************************************/

void cpuexec_timeslice(void)
{
	mame_time target = mame_timer_next_fire_time();
	mame_time base = mame_timer_get_time();
	int cpunum, ran;

	LOG(("------------------\n"));
	LOG(("cpu_timeslice: target = %.9f\n", mame_time_to_double(target)));

	/* process any pending suspends */
	for (cpunum = 0; Machine->drv->cpu[cpunum].cpu_type != CPU_DUMMY; cpunum++)
	{
		if (cpu[cpunum].suspend != cpu[cpunum].nextsuspend)
			LOG(("--> updated CPU%d suspend from %X to %X\n", cpunum, cpu[cpunum].suspend, cpu[cpunum].nextsuspend));
		cpu[cpunum].suspend = cpu[cpunum].nextsuspend;
		cpu[cpunum].eatcycles = cpu[cpunum].nexteatcycles;
	}

	/* loop over CPUs */
	for (cpunum = 0; Machine->drv->cpu[cpunum].cpu_type != CPU_DUMMY; cpunum++)
	{
		/* only process if we're not suspended */
		if (!cpu[cpunum].suspend)
		{
			/* compute how long to run */
			cycles_running = MAME_TIME_TO_CYCLES(cpunum, sub_mame_times(target, cpu[cpunum].localtime));
			LOG(("  cpu %d: %d cycles\n", cpunum, cycles_running));

			/* run for the requested number of cycles */
			if (cycles_running > 0)
			{
				profiler_mark(PROFILER_CPU1 + cpunum);
				cycles_stolen = 0;
				ran = cpunum_execute(cpunum, cycles_running);

#ifdef MAME_DEBUG
				if (ran < cycles_stolen)
					fatalerror("Negative CPU cycle count!");
#endif /* MAME_DEBUG */

				ran -= cycles_stolen;
				profiler_mark(PROFILER_END);

				/* account for these cycles */
				cpu[cpunum].totalcycles += ran;
				cpu[cpunum].localtime = add_mame_times(cpu[cpunum].localtime, MAME_TIME_IN_CYCLES(ran, cpunum));
				LOG(("         %d ran, %d total, time = %.9f\n", ran, (INT32)cpu[cpunum].totalcycles, mame_time_to_double(cpu[cpunum].localtime)));

				/* if the new local CPU time is less than our target, move the target up */
				if (compare_mame_times(cpu[cpunum].localtime, target) < 0)
				{
					if (compare_mame_times(cpu[cpunum].localtime, base) > 0)
						target = cpu[cpunum].localtime;
					else
						target = base;
					LOG(("         (new target)\n"));
				}
			}
		}
	}

	/* update the local times of all CPUs */
	for (cpunum = 0; Machine->drv->cpu[cpunum].cpu_type != CPU_DUMMY; cpunum++)
	{
		/* if we're suspended and counting, process */
		if (cpu[cpunum].suspend && cpu[cpunum].eatcycles && compare_mame_times(cpu[cpunum].localtime, target) < 0)
		{
			/* compute how long to run */
			cycles_running = MAME_TIME_TO_CYCLES(cpunum, sub_mame_times(target, cpu[cpunum].localtime));
			LOG(("  cpu %d: %d cycles (suspended)\n", cpunum, cycles_running));

			cpu[cpunum].totalcycles += cycles_running;
			cpu[cpunum].localtime = add_mame_times(cpu[cpunum].localtime, MAME_TIME_IN_CYCLES(cycles_running, cpunum));
			LOG(("         %d skipped, %d total, time = %.9f\n", cycles_running, (INT32)cpu[cpunum].totalcycles, mame_time_to_double(cpu[cpunum].localtime)));
		}

		/* update the suspend state */
		if (cpu[cpunum].suspend != cpu[cpunum].nextsuspend)
			LOG(("--> updated CPU%d suspend from %X to %X\n", cpunum, cpu[cpunum].suspend, cpu[cpunum].nextsuspend));
		cpu[cpunum].suspend = cpu[cpunum].nextsuspend;
		cpu[cpunum].eatcycles = cpu[cpunum].nexteatcycles;
	}

	/* update the global time */
	mame_timer_set_global_time(target);

	/* huh? something for the debugger */
	#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	{
		extern int debug_key_delay;
		debug_key_delay = 0x7ffe;
	}
	#endif
}



/*************************************
 *
 *  Abort the timeslice for the
 *  active CPU
 *
 *************************************/

void activecpu_abort_timeslice(void)
{
	int current_icount;

	VERIFY_EXECUTINGCPU(activecpu_abort_timeslice);
	LOG(("activecpu_abort_timeslice (CPU=%d, cycles_left=%d)\n", cpu_getexecutingcpu(), activecpu_get_icount() + 1));

	/* swallow the remaining cycles */
	current_icount = activecpu_get_icount() + 1;
	cycles_stolen += current_icount;
	cycles_running -= current_icount;
	activecpu_adjust_icount(-current_icount);
}



/*************************************
 *
 *  Return the current local time for
 *  a CPU, relative to the current
 *  timeslice
 *
 *************************************/

mame_time cpunum_get_localtime(int cpunum)
{
	mame_time result;

	VERIFY_CPUNUM(cpunum_get_localtime);

	/* if we're active, add in the time from the current slice */
	result = cpu[cpunum].localtime;
	if (cpunum == cpu_getexecutingcpu())
	{
		int cycles = cycles_currently_ran();
		result = add_mame_times(result, MAME_TIME_IN_CYCLES(cycles, cpunum));
	}
	return result;
}



/*************************************
 *
 *  Set a suspend reason for the
 *  given CPU
 *
 *************************************/

void cpunum_suspend(int cpunum, int reason, int eatcycles)
{
	VERIFY_CPUNUM(cpunum_suspend);
	LOG(("cpunum_suspend (CPU=%d, r=%X, eat=%d)\n", cpunum, reason, eatcycles));

	/* set the pending suspend bits, and force a resync */
	cpu[cpunum].nextsuspend |= reason;
	cpu[cpunum].nexteatcycles = eatcycles;
	if (cpu_getexecutingcpu() >= 0)
		activecpu_abort_timeslice();
}



/*************************************
 *
 *  Clear a suspend reason for a
 *  given CPU
 *
 *************************************/

void cpunum_resume(int cpunum, int reason)
{
	VERIFY_CPUNUM(cpunum_resume);
	LOG(("cpunum_resume (CPU=%d, r=%X)\n", cpunum, reason));

	/* clear the pending suspend bits, and force a resync */
	cpu[cpunum].nextsuspend &= ~reason;
	if (cpu_getexecutingcpu() >= 0)
		activecpu_abort_timeslice();
}



/*************************************
 *
 *  Return true if a given CPU is
 *  suspended
 *
 *************************************/

int cpunum_is_suspended(int cpunum, int reason)
{
	VERIFY_CPUNUM(cpunum_suspend);
	return ((cpu[cpunum].nextsuspend & reason) != 0);
}



/*************************************
 *
 *  Gets the current CPU's clock speed
 *
 *************************************/

int cpunum_get_clock(int cpunum)
{
	VERIFY_CPUNUM(cpunum_get_clock);
	return cpu[cpunum].clock;
}



/*************************************
 *
 *  Sets the current CPU's clock speed
 *
 *************************************/

void cpunum_set_clock(int cpunum, int clock)
{
	VERIFY_CPUNUM(cpunum_set_clock);

	cpu[cpunum].clock = clock;
	sec_to_cycles[cpunum] = (double)clock * cpu[cpunum].clockscale;
	cycles_to_sec[cpunum] = 1.0 / sec_to_cycles[cpunum];
	cycles_per_second[cpunum] = sec_to_cycles[cpunum];
	subseconds_per_cycle[cpunum] = MAX_SUBSECONDS / sec_to_cycles[cpunum];

	/* re-compute the perfect interleave factor */
	compute_perfect_interleave();
}



void cpunum_set_clock_period(int cpunum, subseconds_t clock_period)
{
	VERIFY_CPUNUM(cpunum_set_clock);

	cpu[cpunum].clock = MAX_SUBSECONDS / clock_period;
	sec_to_cycles[cpunum] = (double) (MAX_SUBSECONDS / clock_period) * cpu[cpunum].clockscale;
	cycles_to_sec[cpunum] = 1.0 / sec_to_cycles[cpunum];
	cycles_per_second[cpunum] = sec_to_cycles[cpunum];
	subseconds_per_cycle[cpunum] = clock_period;

	/* re-compute the perfect interleave factor */
	compute_perfect_interleave();
}



/*************************************
 *
 *  Returns the current scaling factor
 *  for a CPU's clock speed
 *
 *************************************/

double cpunum_get_clockscale(int cpunum)
{
	VERIFY_CPUNUM(cpunum_get_clockscale);
	return cpu[cpunum].clockscale;
}



/*************************************
 *
 *  Sets the current scaling factor
 *  for a CPU's clock speed
 *
 *************************************/

void cpunum_set_clockscale(int cpunum, double clockscale)
{
	VERIFY_CPUNUM(cpunum_set_clockscale);

	cpu[cpunum].clockscale = clockscale;
	sec_to_cycles[cpunum] = (double)cpu[cpunum].clock * clockscale;
	cycles_to_sec[cpunum] = 1.0 / sec_to_cycles[cpunum];
	cycles_per_second[cpunum] = sec_to_cycles[cpunum];
	subseconds_per_cycle[cpunum] = MAX_SUBSECONDS / sec_to_cycles[cpunum];

	/* re-compute the perfect interleave factor */
	compute_perfect_interleave();
}



/*************************************
 *
 *  Temporarily boosts the interleave
 *  factor
 *
 *************************************/

void cpu_boost_interleave(double _timeslice_time, double _boost_duration)
{
	mame_time timeslice_time = double_to_mame_time(_timeslice_time);
	mame_time boost_duration = double_to_mame_time(_boost_duration);

	/* if you pass 0 for the timeslice_time, it means pick something reasonable */
	if (compare_mame_times(timeslice_time, perfect_interleave) < 0)
		timeslice_time = perfect_interleave;

	LOG(("cpu_boost_interleave(%.9f, %.9f)\n", mame_time_to_double(timeslice_time), mame_time_to_double(boost_duration)));

	/* adjust the interleave timer */
	mame_timer_adjust(interleave_boost_timer, timeslice_time, 0, timeslice_time);

	/* adjust the end timer */
	mame_timer_adjust(interleave_boost_timer_end, boost_duration, 0, time_never);
}



#if 0
#pragma mark -
#pragma mark TIMING HELPERS
#endif

/*************************************
 *
 *  Return cycles ran this iteration
 *
 *************************************/

int cycles_currently_ran(void)
{
	VERIFY_EXECUTINGCPU(cycles_currently_ran);
	return cycles_running - activecpu_get_icount();
}



/*************************************
 *
 *  Return cycles remaining in this
 *  iteration
 *
 *************************************/

int cycles_left_to_run(void)
{
	VERIFY_EXECUTINGCPU(cycles_left_to_run);
	return activecpu_get_icount();
}



/*************************************
 *
 *  Return total number of CPU cycles
 *  for the active CPU or for a given CPU.
 *
 *************************************/

/*--------------------------------------------------------------

    IMPORTANT: this value wraps around in a relatively short
    time. For example, for a 6MHz CPU, it will wrap around in
    2^32/6000000 = 716 seconds = 12 minutes.

    Make sure you don't do comparisons between values returned
    by this function, but only use the difference (which will
    be correct regardless of wraparound).

    Alternatively, use the new 64-bit variants instead.

--------------------------------------------------------------*/

UINT32 activecpu_gettotalcycles(void)
{
	VERIFY_ACTIVECPU(activecpu_gettotalcycles);
	if (activecpu == cpu_getexecutingcpu())
		return cpu[activecpu].totalcycles + cycles_currently_ran();
	else
		return cpu[activecpu].totalcycles;
}

UINT32 cpunum_gettotalcycles(int cpunum)
{
	VERIFY_CPUNUM(cpunum_gettotalcycles);
	if (cpunum == cpu_getexecutingcpu())
		return cpu[cpunum].totalcycles + cycles_currently_ran();
	else
		return cpu[cpunum].totalcycles;
}


UINT64 activecpu_gettotalcycles64(void)
{
	VERIFY_ACTIVECPU(activecpu_gettotalcycles64);
	if (activecpu == cpu_getexecutingcpu())
		return cpu[activecpu].totalcycles + cycles_currently_ran();
	else
		return cpu[activecpu].totalcycles;
}

UINT64 cpunum_gettotalcycles64(int cpunum)
{
	VERIFY_CPUNUM(cpunum_gettotalcycles64);
	if (cpunum == cpu_getexecutingcpu())
		return cpu[cpunum].totalcycles + cycles_currently_ran();
	else
		return cpu[cpunum].totalcycles;
}



/*************************************
 *
 *  Return cycles until next interrupt
 *  handler call
 *
 *************************************/

int activecpu_geticount(void)
{
	int result;

/* remove me - only used by mamedbg, m92 */
	VERIFY_EXECUTINGCPU(cpu_geticount);
	result = MAME_TIME_TO_CYCLES(activecpu, sub_mame_times(cpu[activecpu].vblankint_period, mame_timer_timeelapsed(cpu[activecpu].vblankint_timer)));
	return (result < 0) ? 0 : result;
}



/*************************************
 *
 *  Safely eats cycles so we don't
 *  cross a timeslice boundary
 *
 *************************************/

void activecpu_eat_cycles(int cycles)
{
	int cyclesleft = activecpu_get_icount();
	if (cycles > cyclesleft)
		cycles = cyclesleft;
	activecpu_adjust_icount(-cycles);
}



/*************************************
 *
 *  Scales a given value by the fraction
 *  of time elapsed between refreshes
 *
 *************************************/

int cpu_scalebyfcount(int value)
{
	mame_time refresh_elapsed = mame_timer_timeelapsed(refresh_timer);
	int result;

	/* shift off some bits to ensure no overflow */
	if (value < 65536)
		result = value * (refresh_elapsed.subseconds >> 16) / (refresh_period.subseconds >> 16);
	else
		result = value * (refresh_elapsed.subseconds >> 32) / (refresh_period.subseconds >> 32);
	if (value >= 0)
		return (result < value) ? result : value;
	else
		return (result > value) ? result : value;
}



#if 0
#pragma mark -
#pragma mark VIDEO TIMING
#endif

/*************************************
 *
 *  Creates the refresh timer
 *
 *************************************/

static void init_refresh_timer(void)
{
	/* we rely on this being NULL for the time being */
	vblank_timer = NULL;

	/* allocate an infinite timer to track elapsed time since the last refresh */
	refresh_timer = mame_timer_alloc(NULL);

	/* while we're at it, compute the scanline times */
	cpu_compute_scanline_timing();
}



/*************************************
 *
 *  Computes the scanline timing
 *
 *************************************/

void cpu_compute_scanline_timing(void)
{
	/* recompute the refresh period */
	refresh_period = double_to_mame_time(1.0 / Machine->refresh_rate);

	/* recompute the vblank period */
	vblank_period = double_to_mame_time(1.0 / (Machine->refresh_rate * (vblank_multiplier ? vblank_multiplier : 1)));
	if (vblank_timer)
		mame_timer_adjust(vblank_timer, mame_timer_timeleft(vblank_timer), 0, vblank_period);

	/* recompute the scanline period */
	scanline_period = refresh_period;
	if (Machine->drv->vblank_duration)
	{
		scanline_period.subseconds -= DOUBLE_TO_SUBSECONDS(TIME_IN_USEC(Machine->drv->vblank_duration));
		scanline_period.subseconds /= Machine->drv->default_visible_area.max_y - Machine->drv->default_visible_area.min_y + 1;
	}
	else
		scanline_period.subseconds /= Machine->drv->screen_height;

	LOG(("cpu_compute_scanline_timing: refresh=%.9f vblank=%.9f scanline=%.9f\n", mame_time_to_double(refresh_period), mame_time_to_double(vblank_period), mame_time_to_double(scanline_period)));
}



/*************************************
 *
 *  Returns the current scanline
 *
 *************************************/

/*--------------------------------------------------------------

    Note: cpu_getscanline() counts from 0, 0 being the first
    visible line. You might have to adjust this value to match
    the hardware, since in many cases the first visible line
    is >0.

--------------------------------------------------------------*/

int cpu_getscanline(void)
{
	mame_time elapsed = mame_timer_timeelapsed(refresh_timer);
	return (int)(elapsed.subseconds / scanline_period.subseconds);
}



/*************************************
 *
 *  Returns time until given scanline
 *
 *************************************/

mame_time cpu_getscanlinetime_mt(int scanline)
{
	mame_time scantime, abstime;

	/* compute the target time */
	scantime = add_subseconds_to_mame_time(mame_timer_starttime(refresh_timer), scanline * (scanline_period.subseconds + 1));

	/* get the current absolute time */
	abstime = mame_timer_get_time();

	/* if we're already past the computed time, count it for the next frame */
	if (compare_mame_times(abstime, scantime) >= 0)
		scantime = add_mame_times(scantime, refresh_period);

	/* compute how long from now until that time */
	return sub_mame_times(scantime, abstime);
}




double cpu_getscanlinetime(int scanline)
{
	mame_time t = cpu_getscanlinetime_mt(scanline);
	return mame_time_to_double(t);
}



/*************************************
 *
 *  Returns time for one scanline
 *
 *************************************/

mame_time cpu_getscanlineperiod_mt(void)
{
	return scanline_period;
}



double cpu_getscanlineperiod(void)
{
	return mame_time_to_double(scanline_period);
}



/*************************************
 *
 *  Returns a crude approximation
 *  of the horizontal position of the
 *  bream
 *
 *************************************/

int cpu_gethorzbeampos(void)
{
	mame_time elapsed_time = mame_timer_timeelapsed(refresh_timer);
	int scanline = elapsed_time.subseconds / scanline_period.subseconds;
	mame_time time_since_scanline = sub_subseconds_from_mame_time(elapsed_time, scanline * scanline_period.subseconds);
	return time_since_scanline.subseconds * Machine->drv->screen_width / scanline_period.subseconds;
}



/*************************************
 *
 *  Returns the VBLANK state
 *
 *************************************/

int cpu_getvblank(void)
{
	return vblank;
}



/*************************************
 *
 *  Returns the current frame count
 *
 *************************************/

int cpu_getcurrentframe(void)
{
	return current_frame;
}


/***************************************************************************
    SCREEN RENDERING
***************************************************************************/

/*-------------------------------------------------
    video_screen_configure - configure the parameters
    of a screen
-------------------------------------------------*/

subseconds_t INFO_frametime;
subseconds_t INFO_scantime;
subseconds_t INFO_pixeltime;
float INFO_refresh;
int INFO_width;
int INFO_height;

void video_screen_configure(int width, int height, float refresh)
{
	mame_time timeval;

	/* compute timing parameters */
	timeval = double_to_mame_time(TIME_IN_HZ(refresh) / (double)height);

	INFO_width = width;
	INFO_height = height;
	INFO_refresh = refresh;
	INFO_pixeltime = timeval.subseconds / width;
	INFO_scantime = INFO_pixeltime * width;
	INFO_frametime = INFO_scantime * height;
}


double video_screen_get_scanlineperiod(void)
{
	return SUBSECONDS_TO_DOUBLE(INFO_scantime);
}

/*-------------------------------------------------
    video_screen_get_vpos - returns the current
    vertical position of the beam for a given
    screen
-------------------------------------------------*/

int video_screen_get_vpos(void)
{
	mame_time delta = mame_timer_timeelapsed(refresh_timer);
	int vpos;

	vpos = delta.subseconds / INFO_scantime;

	vpos = vpos % INFO_height;

	return vpos;
}


/*-------------------------------------------------
    video_screen_get_hpos - returns the current
    horizontal position of the beam for a given
    screen
-------------------------------------------------*/

int video_screen_get_hpos(void)
{
	mame_time delta = mame_timer_timeelapsed(refresh_timer);
	int vpos, hpos;

	vpos = delta.subseconds / INFO_scantime;

	hpos = (delta.subseconds - (vpos * INFO_scantime)) / INFO_pixeltime;

	return hpos;
}


/*-------------------------------------------------
    video_screen_get_time_until_pos - returns the
    amount of time remaining until the beam is
    at the given hpos,vpos
-------------------------------------------------*/

mame_time video_screen_get_time_until_pos(int vpos, int hpos)
{
	mame_time delta = mame_timer_timeelapsed(refresh_timer);
	mame_time ret;

	/* compute the delta for the given X,Y position */
	ret.subseconds = vpos * INFO_scantime + hpos * INFO_pixeltime;

	/* if we're past that time, head to the next frame */
	if (ret.subseconds <= delta.subseconds)
		ret.subseconds += DOUBLE_TO_SUBSECONDS(TIME_IN_HZ(INFO_refresh));

	ret.subseconds -= delta.subseconds;
	ret.seconds = 0;
	
	/* return the difference */
	return ret;
}

#if 0
#pragma mark -
#pragma mark SYNCHRONIZATION
#endif

/*************************************
 *
 *  Generate a specific trigger
 *
 *************************************/

void cpu_trigger(int trigger)
{
	int cpunum;

	/* cause an immediate resynchronization */
	if (cpu_getexecutingcpu() >= 0)
		activecpu_abort_timeslice();

	/* look for suspended CPUs waiting for this trigger and unsuspend them */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		/* if this is a dummy, stop looking */
		if (Machine->drv->cpu[cpunum].cpu_type == CPU_DUMMY)
			break;

		/* see if this is a matching trigger */
		if (cpu[cpunum].suspend && cpu[cpunum].trigger == trigger)
		{
			cpunum_resume(cpunum, SUSPEND_REASON_TRIGGER);
			cpu[cpunum].trigger = 0;
		}
	}
}



/*************************************
 *
 *  Generate a trigger in the future
 *
 *************************************/

void cpu_triggertime(double duration, int trigger)
{
	mame_timer_set(double_to_mame_time(duration), trigger, cpu_trigger);
}



/*************************************
 *
 *  Generate a trigger for an int
 *
 *************************************/

void cpu_triggerint(int cpunum)
{
	cpu_trigger(TRIGGER_INT + cpunum);
}



/*************************************
 *
 *  Burn/yield CPU cycles until a trigger
 *
 *************************************/

void cpu_spinuntil_trigger(int trigger)
{
	int cpunum = cpu_getexecutingcpu();

	VERIFY_EXECUTINGCPU(cpu_spinuntil_trigger);

	/* suspend the CPU immediately if it's not already */
	cpunum_suspend(cpunum, SUSPEND_REASON_TRIGGER, 1);

	/* set the trigger */
	cpu[cpunum].trigger = trigger;
}

void cpunum_spinuntil_trigger( int cpunum, int trigger )
{
	VERIFY_CPUNUM(cpunum_spinuntil_trigger);

	/* suspend the CPU immediately if it's not already */
	cpunum_suspend(cpunum, SUSPEND_REASON_TRIGGER, 1);

	/* set the trigger */
	cpu[cpunum].trigger = trigger;
}

void cpu_yielduntil_trigger(int trigger)
{
	int cpunum = cpu_getexecutingcpu();

	VERIFY_EXECUTINGCPU(cpu_yielduntil_trigger);

	/* suspend the CPU immediately if it's not already */
	cpunum_suspend(cpunum, SUSPEND_REASON_TRIGGER, 0);

	/* set the trigger */
	cpu[cpunum].trigger = trigger;
}



/*************************************
 *
 *  Burn/yield CPU cycles until an
 *  interrupt
 *
 *************************************/

void cpu_spinuntil_int(void)
{
	VERIFY_EXECUTINGCPU(cpu_spinuntil_int);
	cpu_spinuntil_trigger(TRIGGER_INT + activecpu);
}


void cpu_yielduntil_int(void)
{
	VERIFY_EXECUTINGCPU(cpu_yielduntil_int);
	cpu_yielduntil_trigger(TRIGGER_INT + activecpu);
}



/*************************************
 *
 *  Burn/yield CPU cycles until the
 *  end of the current timeslice
 *
 *************************************/

void cpu_spin(void)
{
	cpu_spinuntil_trigger(TRIGGER_TIMESLICE);
}


void cpu_yield(void)
{
	cpu_yielduntil_trigger(TRIGGER_TIMESLICE);
}



/*************************************
 *
 *  Burn/yield CPU cycles for a
 *  specific period of time
 *
 *************************************/

void cpu_spinuntil_time(double duration)
{
	static int timetrig = 0;

	cpu_spinuntil_trigger(TRIGGER_SUSPENDTIME + timetrig);
	cpu_triggertime(duration, TRIGGER_SUSPENDTIME + timetrig);
	timetrig = (timetrig + 1) & 255;
}


void cpu_yielduntil_time(double duration)
{
	static int timetrig = 0;

	cpu_yielduntil_trigger(TRIGGER_YIELDTIME + timetrig);
	cpu_triggertime(duration, TRIGGER_YIELDTIME + timetrig);
	timetrig = (timetrig + 1) & 255;
}



#if 0
#pragma mark -
#pragma mark CORE TIMING
#endif

/*************************************
 *
 *  Returns the number of times the
 *  interrupt handler will be called
 *  before the end of the current
 *  video frame.
 *
 *************************************/

/*--------------------------------------------------------------

    This can be useful to interrupt handlers to synchronize
    their operation. If you call this from outside an interrupt
    handler, add 1 to the result, i.e. if it returns 0, it means
    that the interrupt handler will be called once.

--------------------------------------------------------------*/

int cpu_getiloops(void)
{
	VERIFY_ACTIVECPU(cpu_getiloops);
	return cpu[activecpu].iloops;
}



/*************************************
 *
 *  Hook for updating things on the
 *  real VBLANK (once per frame)
 *
 *************************************/

static void cpu_vblankreset(void)
{
	int cpunum;

	/* read keyboard & update the status of the input ports */
	input_port_vblank_start();

	/* check the watchdog */
	if (watchdog_counter > 0)
	{
		if (--watchdog_counter == 0)
		{
			logerror("reset caused by the (vblank) watchdog\n");
			mame_schedule_soft_reset();
		}
	}

	/* reset the cycle counters */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		if (!(cpu[cpunum].suspend & SUSPEND_REASON_DISABLE))
			cpu[cpunum].iloops = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame - 1;
		else
			cpu[cpunum].iloops = -1;
	}
}



/*************************************
 *
 *  First-run callback for VBLANKs
 *
 *************************************/

static void cpu_firstvblankcallback(int param)
{
	/* now that we're synced up, pulse from here on out */
	mame_timer_adjust(vblank_timer, vblank_period, param, vblank_period);

	/* but we need to call the standard routine as well */
	cpu_vblankcallback(param);
}



/*************************************
 *
 *  VBLANK core handler
 *
 *************************************/

static void cpu_vblankcallback(int param)
{
	int cpunum;

	if (vblank_countdown == 1)
		vblank = 1;

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		/* if the interrupt multiplier is valid */
		if (cpu[cpunum].vblankint_multiplier != -1)
		{
			/* decrement; if we hit zero, generate the interrupt and reset the countdown */
			if (!--cpu[cpunum].vblankint_countdown)
			{
				/* a param of -1 means don't call any callbacks */
				if (param != -1)
				{
					/* if the CPU has a VBLANK handler, call it */
					if (Machine->drv->cpu[cpunum].vblank_interrupt && !cpunum_is_suspended(cpunum, SUSPEND_REASON_HALT | SUSPEND_REASON_RESET | SUSPEND_REASON_DISABLE))
					{
						cpuintrf_push_context(cpunum);
						(*Machine->drv->cpu[cpunum].vblank_interrupt)();
						cpuintrf_pop_context();
					}

					/* update the counters */
					cpu[cpunum].iloops--;
				}

				/* reset the countdown and timer */
				cpu[cpunum].vblankint_countdown = cpu[cpunum].vblankint_multiplier;
				mame_timer_adjust(cpu[cpunum].vblankint_timer, time_never, 0, time_never);
			}
		}

		/* else reset the VBLANK timer if this is going to be a real VBLANK */
		else if (vblank_countdown == 1)
			mame_timer_adjust(cpu[cpunum].vblankint_timer, time_never, 0, time_never);
	}

	/* is it a real VBLANK? */
	if (!--vblank_countdown)
	{
		/* do we update the screen now? */
		if (!(Machine->drv->video_attributes & VIDEO_UPDATE_AFTER_VBLANK))
			updatescreen();

		/* Set the timer to update the screen */
		mame_timer_adjust(update_timer, double_to_mame_time(TIME_IN_USEC(Machine->drv->vblank_duration)), 0, time_zero);

		/* reset the globals */
		cpu_vblankreset();

		/* reset the counter */
		vblank_countdown = vblank_multiplier;

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
		/* notify the debugger */
		debug_vblank_hook();
#endif
	}
}



/*************************************
 *
 *  End-of-VBLANK callback
 *
 *************************************/

static void cpu_updatecallback(int param)
{
	/* update the screen if we didn't before */
	if (Machine->drv->video_attributes & VIDEO_UPDATE_AFTER_VBLANK)
		updatescreen();
	vblank = 0;

	/* update IPT_VBLANK input ports */
	input_port_vblank_end();

	/* reset partial updating */
	reset_partial_updates();

	/* track total frames */
	current_frame++;

	/* reset the refresh timer */
	mame_timer_adjust(refresh_timer, time_never, 0, time_never);
}



/*************************************
 *
 *  Callback for timed interrupts
 *  (not tied to a VBLANK)
 *
 *************************************/

static void cpu_timedintcallback(int param)
{
	/* bail if there is no routine */
	if (Machine->drv->cpu[param].timed_interrupt && !cpunum_is_suspended(param, SUSPEND_REASON_HALT | SUSPEND_REASON_RESET | SUSPEND_REASON_DISABLE))
	{
		cpuintrf_push_context(param);
		(*Machine->drv->cpu[param].timed_interrupt)();
		cpuintrf_pop_context();
	}
}



/*************************************
 *
 *  Callback to force a timeslice
 *
 *************************************/

static void cpu_timeslicecallback(int param)
{
	cpu_trigger(TRIGGER_TIMESLICE);
}



/*************************************
 *
 *  Callback to end a temporary
 *  interleave boost
 *
 *************************************/

static void end_interleave_boost(int param)
{
	mame_timer_adjust(interleave_boost_timer, time_never, 0, time_never);
	LOG(("end_interleave_boost\n"));
}



/*************************************
 *
 *  Compute the "perfect" interleave
 *  interval
 *
 *************************************/

static void compute_perfect_interleave(void)
{
	subseconds_t smallest = subseconds_per_cycle[0];
	int cpunum;

	/* start with a huge time factor and find the 2nd smallest cycle time */
	perfect_interleave = time_zero;
	perfect_interleave.subseconds = MAX_SUBSECONDS - 1;
	for (cpunum = 1; Machine->drv->cpu[cpunum].cpu_type != CPU_DUMMY; cpunum++)
	{
		/* find the 2nd smallest cycle interval */
		if (subseconds_per_cycle[cpunum] < smallest)
		{
			perfect_interleave.subseconds = smallest;
			smallest = subseconds_per_cycle[cpunum];
		}
		else if (subseconds_per_cycle[cpunum] < perfect_interleave.subseconds)
			perfect_interleave.subseconds = subseconds_per_cycle[cpunum];
	}

	/* adjust the final value */
	if (perfect_interleave.subseconds == MAX_SUBSECONDS - 1)
		perfect_interleave.subseconds = subseconds_per_cycle[0];

	LOG(("Perfect interleave = %.9f, smallest = %.9f\n", mame_time_to_double(perfect_interleave), SUBSECONDS_TO_DOUBLE(smallest)));
}



/*************************************
 *
 *  Setup all the core timers
 *
 *************************************/

static void cpu_inittimers(void)
{
	mame_time first_time;
	int cpunum, max, ipf;

	/* allocate a dummy timer at the minimum frequency to break things up */
	ipf = Machine->drv->cpu_slices_per_frame;
	if (ipf <= 0)
		ipf = 1;
	timeslice_period = double_to_mame_time(1.0 / (Machine->refresh_rate * ipf));
	timeslice_timer = mame_timer_alloc(cpu_timeslicecallback);
	mame_timer_adjust(timeslice_timer, timeslice_period, 0, timeslice_period);

	/* allocate timers to handle interleave boosts */
	interleave_boost_timer = mame_timer_alloc(NULL);
	interleave_boost_timer_end = mame_timer_alloc(end_interleave_boost);

	/*
     *  The following code finds all the CPUs that are interrupting in sync with the VBLANK
     *  and sets up the VBLANK timer to run at the minimum number of cycles per frame in
     *  order to service all the synced interrupts
     */

	/* find the CPU with the maximum interrupts per frame */
	max = 1;
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		ipf = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame;
		if (ipf > max)
			max = ipf;
	}

	/* now find the LCD with the rest of the CPUs (brute force - these numbers aren't huge) */
	vblank_multiplier = max;
	while (1)
	{
		for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		{
			ipf = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame;
			if (ipf > 0 && (vblank_multiplier % ipf) != 0)
				break;
		}
		if (cpunum == cpu_gettotalcpu())
			break;
		vblank_multiplier += max;
	}

	/* initialize the countdown timers and intervals */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		ipf = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame;
		if (ipf > 0)
			cpu[cpunum].vblankint_countdown = cpu[cpunum].vblankint_multiplier = vblank_multiplier / ipf;
		else
			cpu[cpunum].vblankint_countdown = cpu[cpunum].vblankint_multiplier = -1;
	}

	/* allocate a vblank timer at the frame rate * the LCD number of interrupts per frame */
	vblank_period = double_to_mame_time(1.0 / (Machine->refresh_rate * vblank_multiplier));
	vblank_timer = mame_timer_alloc(cpu_vblankcallback);
	vblank_countdown = vblank_multiplier;

	/* allocate an update timer that will be used to time the actual screen updates */
	update_timer = mame_timer_alloc(cpu_updatecallback);

	/*
     *      The following code creates individual timers for each CPU whose interrupts are not
     *      synced to the VBLANK, and computes the typical number of cycles per interrupt
     */

	/* start the CPU interrupt timers */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		ipf = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame;

		/* compute the average number of cycles per interrupt */
		if (ipf <= 0)
			ipf = 1;
		cpu[cpunum].vblankint_period = double_to_mame_time(1.0 / (Machine->refresh_rate * ipf));
		cpu[cpunum].vblankint_timer = mame_timer_alloc(NULL);

		/* see if we need to allocate a CPU timer */
		if (Machine->drv->cpu[cpunum].timed_interrupt_period)
		{
			cpu[cpunum].timedint_period = double_to_mame_time(Machine->drv->cpu[cpunum].timed_interrupt_period);
			cpu[cpunum].timedint_timer = mame_timer_alloc(cpu_timedintcallback);
			mame_timer_adjust(cpu[cpunum].timedint_timer, cpu[cpunum].timedint_period, cpunum, cpu[cpunum].timedint_period);
		}
	}

	/* note that since we start the first frame on the refresh, we can't pulse starting
       immediately; instead, we back up one VBLANK period, and inch forward until we hit
       positive time. That time will be the time of the first VBLANK timer callback */
	first_time = add_mame_times(double_to_mame_time(-TIME_IN_USEC(Machine->drv->vblank_duration)), vblank_period);
	while (compare_mame_times(first_time, time_zero) < 0)
	{
		cpu_vblankcallback(-1);
		first_time = add_mame_times(first_time, vblank_period);
	}
	mame_timer_set(first_time, 0, cpu_firstvblankcallback);

	/* reset the refresh timer to get ourself back in sync */
	mame_timer_adjust(refresh_timer, time_never, 0, time_never);
}

