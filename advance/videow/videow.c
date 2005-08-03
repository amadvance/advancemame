/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <windows.h>

#include "svgalib.h"
#include "svgawin.h"

/****************************************************************************/
/* Mode */

typedef struct mode_info_struct {
	unsigned hde, hrs, hre, ht;
	unsigned vde, vrs, vre, vt;
	unsigned pixelclock;
	int doublescan;
	int interlace;
	int nvsync;
	int nhsync;
	unsigned bits_per_pixel;
} mode_info;

static int video_load(const char* file) 
{
	char buffer[256];
	int found;
	int present;
	FILE* f;
	
	f = fopen(file, "r");
	if (!f) {
		printf("Configuration file %s not found.\n", file);
		return -1;
	}

	found = 0;
	present = 0;
	while (!found && fgets(buffer, sizeof(buffer), f)) {
		char* s = strtok(buffer, " \t\n\r");

		if (s && strcmp(s, "device_video")==0) {
			const char* tag;
			
			present = 1;

			tag = strtok(0, " \t\n\r");
			while (!found && tag) {
	
				if (strcmp(tag, "svgawin")==0) {
					if (adv_svgalib_detect("auto") == 0) {
						found = 1;
						break;
					}
				} else if (strncmp(tag, "svgawin/", 8)==0) {
					if (adv_svgalib_detect(tag + 8) == 0) {
						found = 1;
						break;
					}
				} else {
					fclose(f);
					printf("Invalid device_video option %s.\n", tag);
					return -1;
				}

				tag = strtok(0, " \t\n\r");
			}
		}
	}

	fclose(f);

	if (!present) {
		if (adv_svgalib_detect("auto") == 0) {
			found = 1;
		}
	}

	if (!found) {
		printf("Unsupported video board.\n");
		return -1;
	}

	return 0;
}

static int driver_init(const char* file) 
{
	if (adv_svgalib_init(0, 0) != 0) {
		printf("Error initializing SVGALIB. Have you installed the driver?\n");
		return -1;
	}

	if (video_load(file) != 0) {
		return -1;
	}

	return 0;
}

static void driver_done(void) 
{
	adv_svgalib_done();
}

static int mode_set(mode_info* mode)
{
	if (mode->interlace && !adv_svgalib_state.has_interlace) {
		/* simulate the interlace effect skipping any odd row */

		mode->interlace = 0;
		mode->vde /= 2;
		mode->vrs /= 2;
		mode->vre /= 2;
		mode->vt /= 2;

		adv_svgalib_set(mode->pixelclock, mode->hde, mode->hrs, mode->hre, mode->ht, mode->vde, mode->vrs, mode->vre, mode->vt, mode->doublescan, mode->interlace, mode->nhsync, mode->nvsync, mode->bits_per_pixel, 0, 0);

		/* double the size of the scanline */
		adv_svgalib_scanline_set( adv_svgalib_scanline_get() * 2 );
	} else {
		adv_svgalib_set(mode->pixelclock, mode->hde, mode->hrs, mode->hre, mode->ht, mode->vde, mode->vrs, mode->vre, mode->vt, mode->doublescan, mode->interlace, mode->nhsync, mode->nvsync, mode->bits_per_pixel, 0, 0);
	}

	return 0;
}

static int modeline_load(mode_info* mode_ptr) 
{
	char* s;
	char* name;
	char* endp;

	/* skip the name */
	name = strtok(0, " \t\r\n");
	if (!name) goto err;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;

	mode_ptr->pixelclock = strtod(s, &endp) * 1E6;
	if (*endp) goto err_name;
	if (!mode_ptr->pixelclock) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->hde = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->hrs = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->hre = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->ht = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->vde = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->vrs = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->vre = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->vt = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	mode_ptr->doublescan = 0;
	mode_ptr->interlace = 0;
	mode_ptr->nhsync = 0;
	mode_ptr->nvsync = 0;

	while ((s = strtok(0, " \t\r\n"))!=0) {
		if (s[0]=='#')
			break;
		else if (strcmp(s, "doublescan")==0)
			mode_ptr->doublescan = 1;
		else if (strcmp(s, "interlaced")==0 || strcmp(s, "interlace")==0)
			mode_ptr->interlace = 1;
		else if (strcmp(s, "-hsync")==0)
			mode_ptr->nhsync = 1;
		else if (strcmp(s, "-vsync")==0)
			mode_ptr->nvsync = 1;
	}

	return 0;

err_name:
	printf("Error reading the modeline %s.\n", name);
	return -1;
err:
	printf("Error reading a modeline.\n");
	return -1;
}

static int mode_load(const char* file, unsigned x, unsigned y, mode_info* mode_ptr) 
{
	char buffer[256];
	int found;
	FILE* f;

	f = fopen(file, "r");
	if (!f) {
		printf("Configuration file %s not found.\n", file);
		return -1;
	}

	found = 0;
	while (fgets(buffer, sizeof(buffer), f)) {
		char* s = strtok(buffer, " \t\r\n");

		if (s && strcmp(s, "device_video_modeline")==0) {
			
			if (modeline_load(mode_ptr)!=0) {
				fclose(f);
				return -1;
			}

			if (mode_ptr->hde == x && mode_ptr->vde == y) {
				found = 1;
				break;
			}
		}
	}

	fclose(f);

	if (!found) {
		printf("Modeline %dx%d not found.\n", x, y);
		return -1;
	}

	return 0;
}

static int change(unsigned x, unsigned y, unsigned bits) 
{
	DEVMODE mode;

	memset(&mode, 0, sizeof(mode));
	mode.dmSize = sizeof(mode);
	mode.dmBitsPerPel = bits;
	mode.dmPelsWidth = x;
	mode.dmPelsHeight = y;
	mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	ShowCursor(FALSE);

	/* Windows 2000 requires CDS_UPDATEREGISTRY otherwise the task bar is not repositioned */
	if (ChangeDisplaySettings(&mode, CDS_RESET | CDS_UPDATEREGISTRY) != DISP_CHANGE_SUCCESSFUL) {
		printf("Windows doesn't support this mode.\n");
		return -1;
	}

	ShowCursor(TRUE);

	return 0;
}

static void notify(void) 
{
	ShowCursor(FALSE);
	ShowCursor(TRUE);	
}	

static int save(const char* file) 
{
	unsigned char regs[ADV_SVGALIB_STATE_SIZE];
	FILE* f;

	adv_svgalib_save(regs);

	f = fopen(file, "wb");
	if (!f) {
		printf("Error opening file %s.\n", file);
		return -1;
	}

	if (fwrite(regs, sizeof(regs), 1, f) != 1) {
		printf("Error writing file %s.\n", file);
		return -1;
	}

	fclose(f);

	return 0;
}

static int restore(const char* file) 
{
	unsigned char regs[ADV_SVGALIB_STATE_SIZE];
	FILE* f;

	f = fopen(file, "rb");
	if (!f) {
		printf("Error opening file %s.\n", file);
		return -1;
	}

	if (fread(regs, sizeof(regs), 1, f) != 1) {
		printf("Error reading file %s.\n", file);
		return -1;
	}

	fclose(f);

	adv_svgalib_restore(regs);

	return 0;
}

static int set(const char* config, const char* spec) 
{
	mode_info mode;
	unsigned x;
	unsigned y;
	unsigned bits;

	if (sscanf(spec, "%dx%dx%d", &x, &y, &bits) != 3) {
		printf("Invalid mode specification.\n");
		return -1;
	}

	if (bits != 8 && bits != 15 && bits != 16 && bits != 24 && bits != 32) {
		printf("Invalid bit depth specification.\n");
		return -1;
	}

	mode.bits_per_pixel = bits;
	if (mode_load(config, x, y, &mode) != 0) {
		return -1;
	}

	if (change(x, y, bits) != 0) {
		return -1;
	}

	if (mode_set(&mode) != 0) {
		printf("Error setting the mode.\n");
		return -1;
	}
	
	notify();

	return 0;
}

static int adjust(const char* config) 
{
	mode_info mode;
	unsigned x;
	unsigned y;
	unsigned bits;

	SVGALIB_MODE_INFORMATION mode_information;

	memset(&mode_information, 0, sizeof(SVGALIB_MODE_INFORMATION));
	mode_information.Length = sizeof(SVGALIB_MODE_INFORMATION);
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_QUERY_CURRENT_MODE, 0, 0, &mode_information, sizeof(SVGALIB_MODE_INFORMATION)) != 0) {
		printf("Error getting the video mode information.\n");
		return - 1;
	}

	bits = mode_information.BitsPerPlane * mode_information.NumberOfPlanes;
	x = mode_information.VisScreenWidth;
	y = mode_information.VisScreenHeight;

	mode.bits_per_pixel = bits;
	if (mode_load(config, x, y, &mode) != 0) {
		return -1;
	}

	if (mode_set(&mode) != 0) {
		printf("Error setting the mode.\n");
		return -1;
	}

	notify();

	return 0;
}

static int winrestore(void) 
{
	DEVMODE mode;

	memset(&mode, 0, sizeof(mode));
	mode.dmSize = sizeof(mode);
	mode.dmFields = 0;

	if (ChangeDisplaySettings(&mode, CDS_RESET) != DISP_CHANGE_SUCCESSFUL) {
		printf("Windows isn't able to reset the mode.\n");
		return -1;
	}

	return 0;
}

static int probe_callback(unsigned bus_device_func, unsigned vendor, unsigned device, void* _arg) 
{
	unsigned dw;
	unsigned base_class;

	if (adv_svgalib_pci_read_dword(bus_device_func, 0x8, &dw)!=0)
		return 0;

	base_class = (dw >> 16) & 0xFFFF;
	if (base_class != 0x300 /* DISPLAY | VGA */)
		return 0;

	*(int*)_arg = 1;

	printf("VendorID %04x, DeviceID %04x, Bus %d, Device %d, Function %d\n", vendor, device, (bus_device_func >> 8) & 0xFF, (bus_device_func >> 3) & 0x1F, bus_device_func & 0x7);

	return 0;
}

static void probe(void) 
{
	int found;
	SVGALIB_MODE_INFORMATION mode_information;
	
	printf("Board\n");
	found = 0;
	adv_svgalib_pci_scan_device(probe_callback, &found);
	if (!found)
		printf("ISA (?)\n");
	printf("\n");
	
	printf("Driver\n");
	printf("Name : %s\n", adv_svgalib_driver_get());

	printf("Bit depth : ");
	if (adv_svgalib_state.has_bit8) printf("8 ");
	if (adv_svgalib_state.has_bit15) printf("15 ");
	if (adv_svgalib_state.has_bit16) printf("16 ");
	if (adv_svgalib_state.has_bit24) printf("24 ");
	if (adv_svgalib_state.has_bit32) printf("32 ");
	printf("\n");
	if (adv_svgalib_state.has_interlace)
		printf("Interlace : yes\n");
	else
		printf("Interlace : no\n");
	printf("Linear memory : %08x, %d Mbyte\n", adv_svgalib_linear_base_get(), adv_svgalib_linear_size_get() / (1024*1024));
	if (adv_svgalib_mmio_size_get()) {
		printf("MMIO memory : %08x, %d byte\n", adv_svgalib_mmio_base_get(), adv_svgalib_mmio_size_get());
	}

	printf("\n");

	memset(&mode_information, 0, sizeof(SVGALIB_MODE_INFORMATION));
	mode_information.Length = sizeof(SVGALIB_MODE_INFORMATION);
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_QUERY_CURRENT_MODE, 0, 0, &mode_information, sizeof(SVGALIB_MODE_INFORMATION)) != 0) {
		printf("No video mode information\n");
	} else {
		printf("Current Mode\n");
		printf("Width : %d\n", (unsigned)mode_information.VisScreenWidth);
		printf("Height : %d\n", (unsigned)mode_information.VisScreenHeight);
		printf("Bits per pixel : %d\n", (unsigned)(mode_information.BitsPerPlane * mode_information.NumberOfPlanes));
		printf("Bytes per scanline : %d\n", (unsigned)mode_information.ScreenStride);
	}
}

static void adjust_scanline(void) 
{
	SVGALIB_MODE_INFORMATION mode_information;

	memset(&mode_information, 0, sizeof(SVGALIB_MODE_INFORMATION));
	mode_information.Length = sizeof(SVGALIB_MODE_INFORMATION);
	if (adv_svgalib_ioctl(IOCTL_SVGALIB_QUERY_CURRENT_MODE, 0, 0, &mode_information, sizeof(SVGALIB_MODE_INFORMATION)) != 0) {
		printf("Error detecting the bytes per scanline. Continuing anyway.\n");
		return;
	}

	if (adv_svgalib_scanline_get() != mode_information.ScreenStride) {
		adv_svgalib_scanline_set(mode_information.ScreenStride);
	}
}

static void help(void) 
{
	printf("AdvanceVIDEOW by Andrea Mazzoleni v0.3 " __DATE__ "\n");
	printf(
"Usage:\n"
"    videow [/c CONFIG] [/a] [/s XxYxBITS] [/w FILE] [/r FILE]\n"
"           [/d] [/e] [/m] [/n SIZE]\n"
"Commands:\n"
"    /p          Probe the video board.\n"
"    /a          Adjust the current video mode.\n"
"    /o          Restore the original video mode.\n"
"    /s XxYxBITS Set the specified mode.\n"
"    /w FILE     Write the current mode registers in the specified file.\n"
"    /r FILE     Read the new mode registers from the specified file.\n"
"    /d          Disable the hardware video output.\n"
"    /e          Enable the hardware video output.\n"
"    /m          Enable the hardware mouse pointer.\n"
"    /n SIZE     Set the scanline size in bytes.\n"
"Options:\n"
"    /c CONFIG  Use this config file instead of videow.rc\n"
);
}

int optionmatch(const char* arg, const char* opt) 
{
	return (arg[0] == '-' || arg[0] == '/') && stricmp(arg+1, opt) == 0;
}

void adv_svgalib_log_va(const char *text, va_list arg)
{
}

int main(int argc, char* argv[])
{
	OSVERSIONINFO version_information;
	int i;
	int arg_adjust = 0;
	const char* arg_save = 0;
	const char* arg_restore = 0;
	const char* arg_mode = 0;
	int arg_enable = 0;
	int arg_disable = 0;
	int arg_probe = 0;
	int arg_winrestore = 0;
	int arg_scanline = 0;
	int arg_mouse = 0;
	const char* arg_config = "videow.rc";

	if (argc <= 1) {
		help();
		exit(EXIT_FAILURE);
	}

	version_information.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&version_information)) {
		printf("Error getting the Windows version.\n");
		exit(EXIT_FAILURE);
	}

	if (version_information.dwPlatformId != VER_PLATFORM_WIN32_NT) {
		printf("This program runs only on Windows NT/2000/XP.\n");
		exit(EXIT_FAILURE);
	}

	for(i=1;i<argc;++i) {
		if (optionmatch(argv[i], "w") && i+1<argc) {
			arg_save = argv[i+1];
			++i;
		} else if (optionmatch(argv[i], "r") && i+1<argc) {
			arg_restore = argv[i+1];
			++i;
		} else if (optionmatch(argv[i], "s") && i+1<argc) {
			arg_mode = argv[i+1];
			++i;
		} else if (optionmatch(argv[i], "c") && i+1<argc) {
			arg_config = argv[i+1];
			++i;
		} else if (optionmatch(argv[i], "n") && i+1<argc) {
			arg_scanline = atoi(argv[i+1]);
			++i;
		} else if (optionmatch(argv[i], "a")) {
			arg_adjust = 1;
		} else if (optionmatch(argv[i], "m")) {
			arg_mouse = 1;
		} else if (optionmatch(argv[i], "e")) {
			arg_enable = 1;
		} else if (optionmatch(argv[i], "d")) {
			arg_disable = 1;
		} else if (optionmatch(argv[i], "p")) {
			arg_probe = 1;
		} else if (optionmatch(argv[i], "o")) {
			arg_winrestore = 1;
		} else {
			printf("Unknown option %s.\n", argv[i]);
			exit(EXIT_FAILURE);
		}
	}

	if (driver_init(arg_config) != 0) {
		exit(EXIT_FAILURE);
	}

	if (arg_probe) {
		probe();
	}

	if (arg_disable) {
		adv_svgalib_off();
	}

	if (arg_winrestore) {
		winrestore();
	}

	if (arg_save) {
		if (save(arg_save) != 0) {
			driver_done();
			exit(EXIT_FAILURE);
		}
	}

	if (arg_restore) {
		if (restore(arg_restore) != 0) {
			driver_done();
			exit(EXIT_FAILURE);
		}
	}

	if (arg_adjust) {
		if (adjust(arg_config) != 0) {
			driver_done();
			exit(EXIT_FAILURE);
		}

		adjust_scanline();
	}

	if (arg_mode) {
		if (set(arg_config, arg_mode) != 0) {
			driver_done();
			exit(EXIT_FAILURE);
		}

		adjust_scanline();
	}

	if (arg_scanline) {
		adv_svgalib_scanline_set( arg_scanline );
	}

	if (arg_mouse) {
		adv_svgalib_cursor_on();
	}

	if (arg_enable) {
		adv_svgalib_on();
	}

	driver_done();

	return EXIT_SUCCESS;
}
