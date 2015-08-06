/*********************************************************************

	iflopimg.h

	Bridge code for Imgtool into the standard floppy code

*********************************************************************/

#ifndef IFLOPIMG_H
#define IFLOPIMG_H

#include "formats/flopimg.h"
#include "imgtoolx.h"
#include "library.h"

/***************************************************************************

	Prototypes

***************************************************************************/

floppy_image *imgtool_floppy(imgtool_image *img);
imgtoolerr_t imgtool_floppy_error(floperr_t err);

imgtoolerr_t imgtool_floppy_read_sector_to_stream(imgtool_image *img, int head, int track, int sector, int offset, size_t length, imgtool_stream *f);
imgtoolerr_t imgtool_floppy_write_sector_from_stream(imgtool_image *img, int head, int track, int sector, int offset, size_t length, imgtool_stream *f);


imgtoolerr_t imgtool_floppy_createmodule(imgtool_library *library, const char *format_name,
	const char *description, const struct FloppyFormat *format,
	void (*getinfo)(UINT32 state, union imgtoolinfo *info));

void *imgtool_floppy_extrabytes(imgtool_image *img);

#define FLOPPYMODULE(name, description, format, getinfo)			\
	imgtoolerr_t name##_createmodule(imgtool_library *library)		\
	{																\
		return imgtool_floppy_createmodule(library, #name,			\
			description, floppyoptions_##format, getinfo);			\
	}																\

#endif /* IFLOPIMG_H */
