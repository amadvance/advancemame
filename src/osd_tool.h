/***************************************************************************

    osd_tool.h

    OS-dependent code interface for tools

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __OSD_TOOL_H__
#define __OSD_TOOL_H__

#include "osd_cpu.h"

typedef struct _osd_tool_file osd_tool_file;

/* optional physical drive support */
int osd_is_physical_drive(const char *filename);
int osd_get_physical_drive_geometry(const char *filename, UINT32 *cylinders, UINT32 *heads, UINT32 *sectors, UINT32 *bps);

/* file access */
UINT64 osd_get_file_size(const char *filename);
osd_tool_file *osd_tool_fopen(const char *filename, const char *mode);
void osd_tool_fclose(osd_tool_file *file);
UINT32 osd_tool_fread(osd_tool_file *file, UINT64 offset, UINT32 count, void *buffer);
UINT32 osd_tool_fwrite(osd_tool_file *file, UINT64 offset, UINT32 count, const void *buffer);
UINT64 osd_tool_flength(osd_tool_file *file);

#endif /* __OSD_TOOL_H__ */
