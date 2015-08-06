/*********************************************************************

	main.c

	MESS testing main module

*********************************************************************/

#include <time.h>

#include "core.h"
#include "hashfile.h"
#include "../imgtool/imgtool.h"

#ifdef WIN32
#include <windows.h>
#include "windows/rc.h"
#include "windows/glob.h"
#elif defined XMAME
#include "sysdep/rc.h"
#endif /* WIN32 */

extern int mame_validitychecks(int game);
extern struct rc_option fileio_opts[];

static int dump_screenshots;
static int preserve_directory;
static int test_count, failure_count;

static struct rc_option opts[] =
{
	{ NULL, NULL, rc_link, fileio_opts, NULL, 0, 0, NULL, NULL },
	{ "dumpscreenshots", "ds",	rc_bool,	&dump_screenshots,			"0", 0, 0, NULL,	"always dump screenshots" },
	{ "preservedir",     "pd",	rc_bool,	&preserve_directory,		"0", 0, 0, NULL,	"always dump screenshots" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};



/*************************************
 *
 *	Main and argument parsing/handling
 *
 *************************************/

static int handle_arg(char *arg)
{
	int this_test_count;
	int this_failure_count;
	struct messtest_options opts;

	/* setup options */
	memset(&opts, 0, sizeof(opts));
	opts.script_filename = arg;
	if (preserve_directory)
		opts.preserve_directory = 1;
	if (dump_screenshots)
		opts.dump_screenshots = 1;

	if (messtest(&opts, &this_test_count, &this_failure_count))
		exit(-1);

	test_count += this_test_count;
	failure_count += this_failure_count;
	return 0;
}



#ifdef WIN32
static void win_expand_wildcards(int *argc, char **argv[])
{
	int i;
	glob_t g;

	memset(&g, 0, sizeof(g));

	for (i = 0; i < *argc; i++)
		glob((*argv)[i], (g.gl_pathc > 0) ? GLOB_APPEND|GLOB_NOCHECK : GLOB_NOCHECK, NULL, &g);

	*argc = g.gl_pathc;
	*argv = g.gl_pathv;
}
#endif /* WIN32 */



int main(int argc, char *argv[])
{
	struct rc_struct *rc = NULL;
	int result = -1;
	clock_t begin_time;
	double elapsed_time;
	extern int mess_ghost_images;

	mess_ghost_images = 1;

#ifdef WIN32
	/* expand wildcards so '*' can be used; this is not UNIX */
	win_expand_wildcards(&argc, &argv);
#else
	{
		/* this is for XMESS */
		extern const char *cheatfile;
		extern const char *db_filename;

		cheatfile = db_filename = NULL;
	}
#endif /* WIN32 */

	test_count = 0;
	failure_count = 0;

	/* since the cpuintrf and sndintrf structures are filled dynamically now, we
	 * have to init first */
	cpuintrf_init();
	sndintrf_init();
	
	/* run MAME's validity checks; if these fail cop out now */
	if (mame_validitychecks(-1))
		goto done;
	/* run Imgtool's validity checks; if these fail cop out now */
	if (imgtool_validitychecks())
		goto done;

	/* create rc struct */
	rc = rc_create();
	if (!rc)
	{
		fprintf(stderr, "Out of memory\n");
		goto done;
	}

	/* register options */
	if (rc_register(rc, opts))
	{
		fprintf(stderr, "Out of memory\n");
		goto done;
	}

	begin_time = clock();

	/* parse the commandline */
	if (rc_parse_commandline(rc, argc, argv, 2, handle_arg))
	{
		fprintf(stderr, "Error while parsing cmdline\n");
		goto done;
	}

	if (test_count > 0)
	{
		elapsed_time = ((double) (clock() - begin_time)) / CLOCKS_PER_SEC;

		fprintf(stderr, "Tests complete; %i test(s), %i failure(s), elapsed time %.2f\n",
			test_count, failure_count, elapsed_time);
	}
	else
	{
		fprintf(stderr, "Usage: %s [test1] [test2]...\n", argv[0]);
	}
	result = failure_count;

done:
	if (rc)
		rc_destroy(rc);
	return result;
}

