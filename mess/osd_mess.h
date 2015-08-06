/*********************************************************************

	osd_mess.h

	OS dependent calls for MESS

*********************************************************************/

#ifndef OSD_MESS_H
#define OSD_MESS_H

struct _mame_file;
typedef struct _mess_image mess_image;

/* called by the filemanager code to allow the OS to override the file		*/
/* selecting with it's own code. Return 0 if the MESS core should handle	*/
/* file selection, -1 if the OS code does nothing or 1 if the OS code		*/
/* changed a file.															*/
int osd_select_file(mess_image *img, char *filename);

/* returns 1 if input of type IPT_KEYBOARD should be supressed */
int osd_keyboard_disabled(void);



/******************************************************************************

  Parallel processing (for SMP)

******************************************************************************/

/* 
  Called by core to distribute tasks across multiple processors.  When this is
  called, there will be 1 to max_tasks invocations of task(); where task_count
  specifies the number of calls, and task_num is a number from zero to
  task_count-1 to specify which call was made.  This can be used to subdivide
  tasks across mulitple processors.

  If max_tasks<1, then it should be treated as if it was 1

  A bogus implementation would look like this:

	void osd_parallelize(void (*task)(void *param, int task_num, int
		task_count), void *param, int max_tasks)
	{
		task(param, 0, 1);
	}
*/

void osd_parallelize(void (*task)(void *param, int task_num, int task_count), void *param, int max_tasks);

/******************************************************************************

  Device and file browsing

******************************************************************************/

/* Returns non-zero if a path is absolute, or zero if relative */
int osd_is_absolute_path(const char *path);

/* Returns the path separator */
const char *osd_path_separator(void);

/* Returns non-zero if the char is a path separator */
int osd_is_path_separator(char ch);

/* Creates a directory; returns zero if successful, non-zero otherwise */
int osd_create_directory(int pathtype, int pathindex, const char *dirname);

int osd_num_devices(void);
const char *osd_get_device_name(int i);
void osd_change_device(const char *vol);
void *osd_dir_open(const char *mdirname, const char *filemask);
int osd_dir_get_entry(void *dir, char *name, int namelength, int *is_dir);
void osd_dir_close(void *dir);
void osd_change_directory(const char *path);
const char *osd_get_cwd(void);

void osd_begin_final_unloading(void);

/* used to notify osd code of the load status of an image */
void osd_image_load_status_changed(mess_image *img, int is_final_unload);

char *osd_dirname(const char *filename);
char *osd_basename(char *filename);

#endif /* OSD_MESS_H */

