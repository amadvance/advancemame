#ifndef __SVGAWIN_H
#define __SVGAWIN_H

/* prevent a warning on TEXT redefinition */
#undef TEXT

/* required windows include */
#include <windows.h>
#include <ntdef.h>
#include <winioctl.h>

/* ioctl commands */
#include "driver/svgacode.h"

int adv_svgalib_ioctl(unsigned code, void* input, unsigned input_size, void* output, unsigned output_size);

#endif
