/***************************************************************************

	imgtool.h

	Main headers for Imgtool core

***************************************************************************/

#ifndef IMGTOOL_H
#define IMGTOOL_H

#include <stdlib.h>
#include <stdio.h>
#include "osdepend.h"
#include "mess.h"
#include "formats/flopimg.h"
#include "opresolv.h"
#include "library.h"
#include "filter.h"


typedef struct ImageModule ImageModule;
typedef const struct ImageModule *ImageModuleConstPtr;


/* ----------------------------------------------------------------------- */

#define EOLN_CR		"\x0d"
#define EOLN_LF		"\x0a"
#define EOLN_CRLF	"\x0d\x0a"

#define FILENAME_BOOTBLOCK	((const char *) 1)

/* ---------------------------------------------------------------------------
 * Image calls
 *
 * These are the calls that front ends should use for manipulating images. You
 * should never call the module functions directly because they may not be
 * implemented (i.e. - the function pointers are NULL). The img_* functions are
 * aware of these issues and will make the appropriate checks as well as
 * marking up return codes with the source.  In addition, some of the img_*
 * calls are high level calls that simply image manipulation
 *
 * Calls that return 'int' that are not explictly noted otherwise return
 * imgtool error codes
 * ---------------------------------------------------------------------------
 */

/* img_identify
 *
 * Description:
 *		Attempts to determine the module for any given image
 *
 *	Parameters:
 *		library:			The imgtool_library to search
 *		filename:			The file to check
 */
imgtoolerr_t img_identify(imgtool_library *library, const char *filename,
	ImageModuleConstPtr *modules, size_t count);

/* img_open
 * img_open_byname
 *
 * Description:
 *		Opens an image
 *
 * Parameters:
 *		module/modulename:	The module for this image format
 *		filename:			The native filename for the image
 *		read_or_write:		Open mode (use OSD_FOPEN_* constants)
 *		outimg:				Placeholder for image pointer
 */
imgtoolerr_t img_open(const struct ImageModule *module, const char *filename, int read_or_write, imgtool_image **outimg);
imgtoolerr_t img_open_byname(imgtool_library *library, const char *modulename, const char *filename, int read_or_write, imgtool_image **outimg);

/* img_close
 *
 * Description:
 *		Closes an image
 *
 * Parameters:
 *		img:				The image to close
 */
void img_close(imgtool_image *img);

/* img_info
 *
 * Description:
 *		Returns format specific information about an image
 *
 * Parameters:
 *		img:				The image to query the info of
 *		string:				Buffer to place info in
 *		len:				Length of buffer
 */
imgtoolerr_t img_info(imgtool_image *img, char *string, size_t len);

/* img_beginenum
 *
 * Description:
 *		Begins enumerating files within an image
 *
 * Parameters:
 *		img:				The image to enumerate
 *		path:				The directory path to enumerate
 *		outenum:			The resulting enumeration
 */
imgtoolerr_t img_beginenum(imgtool_image *img, const char *path, imgtool_imageenum **outenum);

/* img_nextenum
 *
 * Description:
 *		Continues enumerating files within an image
 *
 * Parameters:
 *		enumeration:		The enumeration
 *		ent:				Place to receive directory entry
 */
imgtoolerr_t img_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent);

/* img_getdirent
 *
 * Description:
 *		Retrieves the nth directory entry within an image
 *
 * Parameters:
 *		img:				The image to enumerate
 *		path:				The directory path to enumerate
 *		index:				Zero counted index
 *		ent:				Place to receive directory entry
 */
imgtoolerr_t img_getdirent(imgtool_image *img, const char *path, int index, imgtool_dirent *ent);

/* img_countfiles
 *
 * Description:
 *		Counts the total amount of files within an image
 *
 * Parameters:
 *		img:				The image to enumerate
 *		totalfiles:			Place to receive the file count
 */
imgtoolerr_t img_countfiles(imgtool_image *img, int *totalfiles);

/* img_filesize
 *
 * Description:
 *		Retrieves the file length of the specified file
 *
 * Parameters:
 *		img:				The image to enumerate
 *		filename			Filename of file on the image
 *		filesize			Place to receive the file length
 */
imgtoolerr_t img_filesize(imgtool_image *img, const char *filename, UINT64 *filesize);

/* img_closeenum
 *
 * Description:
 *		Closes an enumeration
 *
 * Parameters:
 *		enumeration:		The enumeration to close
 */
void img_closeenum(imgtool_imageenum *enumeration);

/* img_freespace
 *
 * Description:
 *		Returns free space on an image, in bytes
 *
 * Parameters:
 *		img:				The image to query
 *		sz					Place to receive free space
 */
imgtoolerr_t img_freespace(imgtool_image *img, UINT64 *sz);

/* img_readfile
 *
 * Description:
 *		Start reading from a file on an image with a stream
 *
 * Parameters:
 *		image:				The image to read from
 *		filename:			The filename on the image
 *		fork:				The fork on the file
 *		destf:				Place to receive the stream
 *      filter:             Filter to use, or NULL if none
 */
imgtoolerr_t img_readfile(imgtool_image *image, const char *filename, const char *fork,
	imgtool_stream *destf, filter_getinfoproc filter);

/* img_writefile
 *
 * Description:
 *		Start writing to a new file on an image with a stream
 *
 * Parameters:
 *		image:				The image to read from
 *		filename:			The filename on the image
 *		fork:				The fork on the file
 *		destf:				Place to receive the stream
 *		options/ropts:		Options to specify on the new file
 *      filter:             Filter to use, or NULL if none
 */
imgtoolerr_t img_writefile(imgtool_image *image, const char *filename, const char *fork,
	imgtool_stream *sourcef, option_resolution *resolution, filter_getinfoproc filter);

/* img_getfile
 *
 * Description:
 *		Read a file from an image, storing it into a native file
 *
 * Parameters:
 *		image:				The image to read from
 *		filename:			The filename on the image
 *		fork:				The fork on the file
 *		dest:				Filename for native file to write to
 *      filter:             Filter to use, or NULL if none
 */
imgtoolerr_t img_getfile(imgtool_image *img, const char *filename, const char *fork,
	const char *dest, filter_getinfoproc filter);

/* img_putfile
 *
 * Description:
 *		Read a native file and store it on an image
 *
 * Parameters:
 *		image:				The image to read from
 *		newfname:			The filename on the image to store (if NULL, then
 *							the file will be named basename(source)
 *		fork:				The fork on the file
 *		source:				Native filename for source
 *		opts:				Options to specify on the new file
 *      filter:             Filter to use, or NULL if none
 */
imgtoolerr_t img_putfile(imgtool_image *img, const char *newfname, const char *fork,
	const char *source, option_resolution *opts, filter_getinfoproc filter);

/* img_deletefile
 *
 * Description:
 *		Delete a file on an image
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 */
imgtoolerr_t img_deletefile(imgtool_image *img, const char *fname);

/* img_listforks
 *
 * Description:
 *		Lists all forks on an image
 *
 * Parameters:
 *		image:				The image to read from
 *		path:				The filename on the image
 */
imgtoolerr_t img_listforks(imgtool_image *image, const char *path, imgtool_forkent *ents, size_t len);

/* img_createdir
 *
 * Description:
 *		Delete a directory on an image
 *
 * Parameters:
 *		img:				The image to read from
 *		path:				The path to the directory to delete
 */
imgtoolerr_t img_createdir(imgtool_image *img, const char *path);

/* img_deletedir
 *
 * Description:
 *		Delete a directory on an image
 *
 * Parameters:
 *		img:				The image to read from
 *		path:				The path to the directory to delete
 */
imgtoolerr_t img_deletedir(imgtool_image *img, const char *path);

/* img_getattrs
 * img_setattrs
 * img_getattrs
 * img_setattr
 *
 * Description:
 *		Gets or sets attributes on a file
 *
 * Parameters:
 *		image:				The image to read from
 *		path:				The path to the file to query
 *		attrs:				The list of attributes on the file
 *		values:				Values to get or store
 */
imgtoolerr_t img_listattrs(imgtool_image *image, const char *path, UINT32 *attrs, size_t len);
imgtoolerr_t img_getattrs(imgtool_image *image, const char *path, const UINT32 *attrs, imgtool_attribute *values);
imgtoolerr_t img_setattrs(imgtool_image *image, const char *path, const UINT32 *attrs, const imgtool_attribute *values);
imgtoolerr_t img_getattr(imgtool_image *image, const char *path, UINT32 attr, imgtool_attribute *value);
imgtoolerr_t img_setattr(imgtool_image *image, const char *path, UINT32 attr, imgtool_attribute value);

void img_attrname(const struct ImageModule *module, UINT32 attribute, const imgtool_attribute *attr_value,
	char *buffer, size_t buffer_len);

/* img_geticoninfo
 *
 * Description:
 *		Gets or sets attributes on a file
 *
 * Parameters:
 *		image:				The image to read from
 *		path:				The path to the file to query
 *		iconinfo:			Icon info to retrieve
 */
imgtoolerr_t img_geticoninfo(imgtool_image *image, const char *path, imgtool_iconinfo *iconinfo);

/* img_suggesttransfer
 *
 * Description:
 *		Suggest a list of filters appropriate for a file
 *
 * Parameters:
 *		image:				The image to read from
 *		path:				The path to the file on the image; can be NULL
 *		stream:				Stream on local computer to check; can be NULL
 */
imgtoolerr_t img_suggesttransfer(imgtool_image *image, const char *path,
	imgtool_stream *stream, imgtool_transfer_suggestion *suggestions, size_t suggestions_length);

/* img_getchain
 * img_getchain_string
 *
 * Description:
 *		Retrieves the block chain for a file or directory
 *
 * Parameters:
 *		img:				The image to read from
 *		path:				The path to the file or directory
 */
imgtoolerr_t img_getchain(imgtool_image *img, const char *path, imgtool_chainent *chain, size_t chain_size);
imgtoolerr_t img_getchain_string(imgtool_image *img, const char *path, char *buffer, size_t buffer_len);

/* img_create
 * img_create_byname
 *
 * Description:
 *		Creates an image
 *
 * Parameters:
 *		module/modulename:	The module for this image format
 *		fname:				The native filename for the image
 *		options/ropts:		Options that control how the image is created
 *							(tracks, sectors, etc)
 *		image:				Placeholder for resulting image.  Can be NULL
 */
imgtoolerr_t img_create(const struct ImageModule *module, const char *fname,
	option_resolution *opts, imgtool_image **image);
imgtoolerr_t img_create_byname(imgtool_library *library, const char *modulename, const char *fname,
	option_resolution *opts, imgtool_image **image);

/* img_getsectorsize
 * img_readsector
 * img_writesector
 *
 * Description:
 *		Functions for reading/writing sector data
 */
imgtoolerr_t img_getsectorsize(imgtool_image *image, UINT32 track, UINT32 head,
	UINT32 sector, UINT32 *length);
imgtoolerr_t img_readsector(imgtool_image *image, UINT32 track, UINT32 head,
	UINT32 sector, void *buffer, size_t len);
imgtoolerr_t img_writesector(imgtool_image *image, UINT32 track, UINT32 head,
	UINT32 sector, const void *buffer, size_t len);

/* img_malloc
 *
 * Description:
 *		Allocates memory off of an image
 */
void *img_malloc(imgtool_image *image, size_t size);

/* img_module
 * img_enum_module
 *
 * Description:
 *		Retrieves the module associated with an image
 */
const struct ImageModule *img_module(imgtool_image *img);
const struct ImageModule *img_enum_module(imgtool_imageenum *enumeration);

/* img_extrabytes
 * img_enum_extrabytes
 *
 * Description:
 *		Retrieves the extra bytes associated with an image
 */
void *img_extrabytes(imgtool_image *img);
void *img_enum_extrabytes(imgtool_imageenum *enumeration);

/* img_enum_image
 *
 * Description:
 *		Retrieves the image associated with an image enumeration
 */
imgtool_image *img_enum_image(imgtool_imageenum *enumeration);

/* img_get_module_features
 *
 * Description:
 *		Retrieves a structure identifying this module's features associated with an image
 */
struct imgtool_module_features
{
	unsigned int supports_create : 1;
	unsigned int supports_open : 1;
	unsigned int supports_reading : 1;
	unsigned int supports_writing : 1;
	unsigned int supports_deletefile : 1;
	unsigned int supports_directories : 1;
	unsigned int supports_freespace : 1;
	unsigned int supports_createdir : 1;
	unsigned int supports_deletedir : 1;
	unsigned int supports_creation_time : 1;
	unsigned int supports_lastmodified_time : 1;
	unsigned int supports_readsector : 1;
	unsigned int supports_writesector : 1;
	unsigned int supports_forks : 1;
	unsigned int supports_geticoninfo : 1;
	unsigned int is_read_only : 1;
};

struct imgtool_module_features img_get_module_features(const struct ImageModule *module);

/* imgtool_validitychecks
 *
 * Description:
 *		Runs consistency checks to make sure that all is well
 */
int imgtool_validitychecks(void);

#endif /* IMGTOOL_H */
