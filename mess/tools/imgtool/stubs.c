#include "driver.h"
#include <ctype.h>
#include <stdarg.h>

/* Variables to hold the status of various game options */
static FILE *errorlog;

const game_driver *const drivers[1];
int rompath_extra;
int cheatfile;
const char *db_filename;
int history_filename;
int mameinfo_filename;


void CLIB_DECL logerror(const char *text,...)
{
	va_list arg;
	va_start(arg,text);
	if (errorlog)
		vfprintf(errorlog,text,arg);
	va_end(arg);
}

/* ----------------------------------------------------------------------- */
/* total hack */

mame_file *mame_fopen(const char *gamename, const char *filename, int filetype, int openforwrite)
{
	char buffer[2048];
	snprintf(buffer, sizeof(buffer), "crc/%s", filename);
	return (mame_file *) fopen(buffer, "r");
}

char *mame_fgets(char *s, int n, mame_file *file)
{
	return fgets(s, n, (FILE *) file);
}

UINT32 mame_fwrite(mame_file *file, const void *buffer, UINT32 length)
{
	return fwrite(buffer, 1, length, (FILE *) file);
}

void mame_fclose(mame_file *file)
{
	fclose((FILE *) file);
}

void CLIB_DECL fatalerror(const char *text,...)
{
	va_list va;
	va_start(va, text);
	vfprintf(stderr, text, va);
	va_end(va);
	exit(-1);
}



