/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003, 2005, 2008 Andrea Mazzoleni
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
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#include "portable.h"

#include "os.h"
#include "log.h"
#include "ksdl.h"
#include "isdl.h"
#include "msdl.h"
#include "target.h"
#include "file.h"
#include "ossdl.h"
#include "snstring.h"
#include "measure.h"
#include "resource.h"
#include "oswin.h"

#include "SDL.h"
#include "SDL_syswm.h"

#include <windows.h>

struct os_context {
	int is_quit; /**< Is termination requested. */
	char title_buffer[128]; /**< Title of the window. */
	HHOOK g_hKeyboardHook;
	STICKYKEYS g_StartupStickyKeys;
	TOGGLEKEYS g_StartupToggleKeys;
	FILTERKEYS g_StartupFilterKeys;

};

static struct os_context OS;

/***************************************************************************/
/* hook */

LRESULT CALLBACK windows_hook_winproc(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* p;

	log_std(("os: windows_hook_winproc(%u, %u, %u)\n", (unsigned)nCode, (unsigned)wParam, (unsigned)lParam));

	if (nCode != HC_ACTION) {
		log_std(("os: windows_hook_winproc(!=HC_ACTION) -> NextHook\n"));
		return CallNextHookEx(OS.g_hKeyboardHook, nCode, wParam, lParam);
	}
 
	p = (KBDLLHOOKSTRUCT*)lParam;

	if (wParam == WM_KEYDOWN || wParam == WM_KEYUP || wParam == WM_SYSKEYDOWN || wParam == WM_SYSKEYUP) {
		log_std(("os: windows_hook_winproc(HC_ACTION, WM_KEY*)\n"));

		/* LWIN */
		if (p->vkCode == VK_LWIN)
			return 1;
		/* RWIN */
		if (p->vkCode == VK_RWIN)
			return 1;
		/* ALT + TAB */
		if (p->vkCode == VK_TAB && (p->flags & LLKHF_ALTDOWN) != 0)
			return 1;
		/* ALT + ESC */
		if (p->vkCode == VK_ESCAPE && (p->flags & LLKHF_ALTDOWN) != 0)
			return 1;
		/* CTRL + ESC */
		if (p->vkCode == VK_ESCAPE && (GetKeyState( VK_CONTROL ) & 0x8000) != 0)
			return 1;
#if 0
		/* CTRL + ALT + DEL */
		if (p->vkCode == VK_DELETE) && (GetKeyState(VK_CONTROL) & 0x8000) != 0 && (p->flags & LLKHF_ALTDOWN) != 0)
			return 1;
#endif
	}

	log_std(("os: windows_hook_winproc(HC_ACTION) -> NextHook\n"));

	return CallNextHookEx(OS.g_hKeyboardHook, nCode, wParam, lParam);
}

static void windows_save_hot_key(void)
{
	log_std(("os: SystemParametersInfo(GET)\n"));
	/* Save the current sticky/toggle/filter key settings so they can be restored them later */
	SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &OS.g_StartupStickyKeys, 0);
	SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &OS.g_StartupToggleKeys, 0);
	SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &OS.g_StartupFilterKeys, 0);
}

void os_internal_ignore_hot_key(void)
{
	STICKYKEYS skOff;
	TOGGLEKEYS tkOff;
	FILTERKEYS fkOff;

	/* keyboard hook to disable win keys */
	if (!OS.g_hKeyboardHook) {
		log_std(("os: SetWindowsHookEx()\n"));
		OS.g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL,  windows_hook_winproc, GetModuleHandle(0), 0);
		if (!OS.g_hKeyboardHook) {
			log_std(("os: SetWindowsHookEx() failed\n"));
		}
	}

	skOff = OS.g_StartupStickyKeys;
	tkOff = OS.g_StartupToggleKeys;
	fkOff = OS.g_StartupFilterKeys;

	/* disable StickyKeys/etc shortcuts but if the accessibility feature is on, */
	/* then leave the settings alone as its probably being usefully used */
	if ((skOff.dwFlags & SKF_STICKYKEYSON) == 0) {
		// Disable the hotkey and the confirmation
		skOff.dwFlags &= ~SKF_HOTKEYACTIVE;
		skOff.dwFlags &= ~SKF_CONFIRMHOTKEY;

		log_std(("os: SystemParametersInfo(SPI_SETSTICKYKEYS)\n"));
		SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &skOff, 0);
	}

	if ((tkOff.dwFlags & TKF_TOGGLEKEYSON) == 0) {
		// Disable the hotkey and the confirmation
		tkOff.dwFlags &= ~TKF_HOTKEYACTIVE;
		tkOff.dwFlags &= ~TKF_CONFIRMHOTKEY;

		log_std(("os: SystemParametersInfo(SPI_SETTOGGLEKEYS)\n"));
		SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &tkOff, 0);
	}
 
	if ((fkOff.dwFlags & FKF_FILTERKEYSON) == 0) {
		/* disable the hotkey and the confirmation */
		fkOff.dwFlags &= ~FKF_HOTKEYACTIVE;
		fkOff.dwFlags &= ~FKF_CONFIRMHOTKEY;

		log_std(("os: SystemParametersInfo(SPI_SETFILTERKEYS)\n"));
		SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &fkOff, 0);
	}
}

void os_internal_restore_hot_key(void)
{
	if (OS.g_hKeyboardHook) {
		log_std(("os: UnhookWindowsHookEx()\n"));
		UnhookWindowsHookEx(OS.g_hKeyboardHook);
		OS.g_hKeyboardHook = 0;
	}

	/* restore StickyKeys/etc to original state and enable Windows key */
	log_std(("os: SystemParametersInfo(RESTORE)\n"));
	SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &OS.g_StartupStickyKeys, 0);
	SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &OS.g_StartupToggleKeys, 0);
	SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &OS.g_StartupFilterKeys, 0);
}

/***************************************************************************/
/* splash */

#if defined(ADV_MENU)
struct context_struct {
	HWND m_hwnd;
	DWORD m_dwWidth;
	DWORD m_dwHeight;
	HBITMAP m_hSplashBitmap;
	HBITMAP m_hAlphaBitmap;
	LPCTSTR m_lpszClassName;
	INT m_X;
	INT m_Y;
} SPLASH;

static LRESULT windows_splash_paint(HWND hwnd)
{
	int r;
	unsigned scanline;
	unsigned char* splash_ptr;
	unsigned char* alpha_ptr;
	unsigned char* screen_ptr;
	unsigned x, y;
	BITMAPINFOHEADER bi;

	HWND desktop = GetDesktopWindow();
	HDC desktop_dc = GetWindowDC(desktop);

	/* create the screen buffer */
	HDC screen_dc = CreateCompatibleDC(desktop_dc); 
	HBITMAP screen_bitmap = CreateCompatibleBitmap(desktop_dc, SPLASH.m_dwWidth, SPLASH.m_dwHeight); 
	HBITMAP screen_bitmap_old = (HBITMAP)SelectObject(screen_dc, screen_bitmap);

	/* copy the desktop to the screen buffer */
	BitBlt(screen_dc, 0, 0, SPLASH.m_dwWidth, SPLASH.m_dwHeight, desktop_dc,  SPLASH.m_X, SPLASH.m_Y, SRCCOPY);

	ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = SPLASH.m_dwWidth;
	bi.biHeight = SPLASH.m_dwHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biCompression = BI_RGB;

	r = GetDIBits (desktop_dc, SPLASH.m_hSplashBitmap, 0, SPLASH.m_dwHeight, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
	splash_ptr = (unsigned char*)GlobalAlloc(GMEM_FIXED, bi.biSizeImage);
	r = GetDIBits(desktop_dc, SPLASH.m_hSplashBitmap, 0, SPLASH.m_dwHeight, splash_ptr, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
	if (!r)
		return -1;

	r = GetDIBits(desktop_dc, SPLASH.m_hAlphaBitmap, 0, SPLASH.m_dwHeight, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
	alpha_ptr = (unsigned char*)GlobalAlloc(GMEM_FIXED, bi.biSizeImage);
	r = GetDIBits(desktop_dc, SPLASH.m_hAlphaBitmap, 0, SPLASH.m_dwHeight, alpha_ptr, (BITMAPINFO*)&bi, DIB_RGB_COLORS); 
	if (!r)
		return -1;

	r = GetDIBits(desktop_dc, screen_bitmap, 0, SPLASH.m_dwHeight, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
	screen_ptr = (unsigned char*)GlobalAlloc(GMEM_FIXED, bi.biSizeImage);
	r = GetDIBits(desktop_dc, screen_bitmap, 0, SPLASH.m_dwHeight, screen_ptr, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
	if (!r)
		return -1;

	/* scanline size */
	scanline = SPLASH.m_dwWidth * 3;

	/* align */
	scanline = (scanline + 3) & ~3;

	for(y=0;y<SPLASH.m_dwHeight;++y) {
		unsigned char* splash_i = splash_ptr + scanline * y;
		unsigned char* alpha_i = alpha_ptr  + scanline * y;
		unsigned char* screen_i = screen_ptr + scanline * y;
		
		for(x=0;x<SPLASH.m_dwWidth;++x) {
			unsigned f = alpha_i[0];
			if (f == 0) {
				/* Nothing */
			} else if (f == 255) {
				/* Copy */
				screen_i[0] = splash_i[0];
				screen_i[1] = splash_i[1];
				screen_i[2] = splash_i[2];
			} else {
				/* Alpha */
				unsigned fb = 255 - f;

				unsigned rb = splash_i[0];
				unsigned gb = splash_i[1];
				unsigned bb = splash_i[2];

				unsigned r = screen_i[0];
				unsigned g = screen_i[1];
				unsigned b = screen_i[2];

				r = (r*fb + rb*f) / 255;
				g = (g*fb + gb*f) / 255;
				b = (b*fb + bb*f) / 255;

				if (r > 255)
					r = 255;
				if (g > 255)
					g = 255;
				if (b > 255)
					b = 255;

				screen_i[0] = r;
				screen_i[1] = g;
				screen_i[2] = b;
			}

			alpha_i += 3;
			splash_i += 3;
			screen_i += 3;
		}
	}
	
	/* paint in the bitmap */
	SetDIBits(desktop_dc, screen_bitmap, 0, SPLASH.m_dwHeight, screen_ptr, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	/* paint to the windows DC */
	HDC hDC = GetDC(hwnd);
	BitBlt(hDC,0,0, SPLASH.m_dwWidth, SPLASH.m_dwHeight, screen_dc, 0 ,0, SRCCOPY);
	ReleaseDC(hwnd, hDC);

	/* free */
	SelectObject(screen_dc, screen_bitmap_old);
	DeleteObject(screen_bitmap);
	DeleteDC(screen_dc);
	ReleaseDC(desktop,desktop_dc);

	GlobalFree(splash_ptr);
	GlobalFree(alpha_ptr);
	GlobalFree(screen_ptr);

	return 0;
}

static LRESULT CALLBACK windows_splash_windproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_PAINT)
		return windows_splash_paint(hwnd);
	return DefWindowProc(hwnd, uMsg, wParam, lParam) ;
}

static void windows_splash_start(void)
{
	DWORD nScrWidth;
	DWORD nScrHeight;

	SPLASH.m_hwnd = 0;
	SPLASH.m_lpszClassName = TEXT("SPLASH");

#if defined(ADV_EMU)
	SPLASH.m_hSplashBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SPLASH));
#else
	SPLASH.m_hSplashBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SPLASH_MENU));
#endif
	if (!SPLASH.m_hSplashBitmap)
		return;

#if defined(ADV_EMU)
	SPLASH.m_hAlphaBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_ALPHA));
#else
	SPLASH.m_hAlphaBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_ALPHA_MENU));
#endif
	if (!SPLASH.m_hAlphaBitmap)
		return;

	int nRetValue;
	BITMAP csBitmapSize;

	nRetValue = GetObject(SPLASH.m_hSplashBitmap, sizeof(csBitmapSize), &csBitmapSize);
	if (nRetValue == 0)
		return;

	SPLASH.m_dwWidth = (DWORD)csBitmapSize.bmWidth;
	SPLASH.m_dwHeight = (DWORD)csBitmapSize.bmHeight;

	WNDCLASSEX wndclass;
	wndclass.cbSize =         sizeof (wndclass);
	wndclass.style          = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
	wndclass.lpfnWndProc    = windows_splash_windproc;
	wndclass.cbClsExtra =     0;
	wndclass.cbWndExtra =     DLGWINDOWEXTRA;
	wndclass.hInstance      = GetModuleHandle(NULL);
	wndclass.hIcon          = NULL;
	wndclass.hCursor        = LoadCursor(NULL, IDC_WAIT);
	wndclass.hbrBackground  = NULL;
	wndclass.lpszMenuName   = NULL;
	wndclass.lpszClassName  = SPLASH.m_lpszClassName;
	wndclass.hIconSm        = NULL;
	if (!RegisterClassEx(&wndclass))
		return;

	nScrWidth = GetSystemMetrics(SM_CXFULLSCREEN);
	nScrHeight = GetSystemMetrics(SM_CYFULLSCREEN);

	SPLASH.m_X = (nScrWidth  - SPLASH.m_dwWidth) / 2;
	SPLASH.m_Y = (nScrHeight - SPLASH.m_dwHeight) / 2;
	SPLASH.m_hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, SPLASH.m_lpszClassName, TEXT("Splash"), WS_POPUP, SPLASH.m_X, SPLASH.m_Y,  SPLASH.m_dwWidth, SPLASH.m_dwHeight, NULL, NULL, NULL, &SPLASH);
	if (!SPLASH.m_hwnd)
		return;

	ShowWindow(SPLASH.m_hwnd, SW_SHOW) ;
	UpdateWindow(SPLASH.m_hwnd);
}

void windows_splash_stop(void)
{
	if (SPLASH.m_hwnd) {
		DestroyWindow(SPLASH.m_hwnd);
		if (SPLASH.m_lpszClassName)
			UnregisterClass(SPLASH.m_lpszClassName, GetModuleHandle(NULL));
		SPLASH.m_hwnd = 0;
	}
}
#else
static void windows_splash_start(void)
{
}

static void windows_splash_stop(void)
{
}
#endif

void os_fire(void)
{
	windows_splash_stop();
}

/***************************************************************************/
/* Init */

int os_init(adv_conf* context)
{
	memset(&OS, 0, sizeof(OS));

	return 0;
}

void os_done(void)
{
}

static void os_wait(void)
{
	Sleep(1);
}

int os_inner_init(const char* title)
{
	SDL_version compiled;
	target_clock_t start, stop;
	double delay_time;
	unsigned i;
	const char* video_driver;

	os_fire();

	target_yield();

	delay_time = adv_measure_step(os_wait, 0.0001, 0.2, 7);

	if (delay_time > 0) {
		log_std(("os: sleep granularity %g\n", delay_time));
		target_usleep_granularity(delay_time * 1000000);
	} else {
		log_std(("ERROR:os: sleep granularity NOT measured\n"));
		target_usleep_granularity(20000 /* 20 ms */);
	}

	log_std(("os: sys SDL\n"));

	/* print the compiler version */
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#define COMPILER_RESOLVE(a) #a
#define COMPILER(a, b, c) COMPILER_RESOLVE(a) "." COMPILER_RESOLVE(b) "." COMPILER_RESOLVE(c)
	log_std(("os: compiler GNU %s\n", COMPILER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)));
#else
	log_std(("os: compiler unknown\n"));
#endif

#ifdef USE_LSB
	log_std(("os: compiled little endian system\n"));
#else
	log_std(("os: compiled big endian system\n"));
#endif

	/* Note that from SDL 1.2.10 the "windib" driver is the default, previouly the default was "directx" */
	video_driver = getenv("SDL_VIDEODRIVER");
	if (video_driver) {
		log_std(("os: SDL_VIDEODRIVER = %s\n", video_driver));
	} else {
		log_std(("os: SDL_VIDEODRIVER is unset\n"));
#if !defined(USE_VIDEO_RESTORE)
		/* Set directx, but only if the program doesn't start new process */
		log_std(("os: Set SDL_VIDEODRIVER as directx\n"));
		putenv("SDL_VIDEODRIVER=directx");
		video_driver = getenv("SDL_VIDEODRIVER");
		log_std(("os: SDL_VIDEODRIVER = %s\n", video_driver));
#endif
	}

	log_std(("os: SDL_Init(SDL_INIT_NOPARACHUTE)\n"));
	if (SDL_Init(SDL_INIT_NOPARACHUTE) != 0) {
		log_std(("os: SDL_Init() failed, %s\n", SDL_GetError()));
		target_err("Error initializing the SDL video support.\n");
		return -1;
	}

	SDL_VERSION(&compiled);

	log_std(("os: compiled with sdl %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch));
	log_std(("os: linked with sdl %d.%d.%d\n", SDL_Linked_Version()->major, SDL_Linked_Version()->minor, SDL_Linked_Version()->patch));
	if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
		log_std(("os: sdl little endian system\n"));
	else
		log_std(("os: sdl big endian system\n"));

	start = target_clock();
	stop = target_clock();
	while (stop == start)
		stop = target_clock();
	log_std(("os: clock delta %ld\n", (unsigned long)(stop - start)));

	/* set the titlebar */
	sncpy(OS.title_buffer, sizeof(OS.title_buffer), title);

	/* set some signal handlers */
	signal(SIGABRT, (void (*)(int))os_signal);
	signal(SIGFPE, (void (*)(int))os_signal);
	signal(SIGILL, (void (*)(int))os_signal);
	signal(SIGINT, (void (*)(int))os_signal);
	signal(SIGSEGV, (void (*)(int))os_signal);
	signal(SIGTERM, (void (*)(int))os_signal);

	return 0;
}

void os_inner_done(void)
{
	log_std(("os: SDL_Quit()\n"));
	SDL_Quit();
}

void os_poll(void)
{
	SDL_Event event;

	/* The event queue works only with the video initialized */
	if (!SDL_WasInit(SDL_INIT_VIDEO))
		return;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_KEYDOWN :
#ifdef USE_KEYBOARD_SDL
				keyb_sdl_event_press(event.key.keysym.sym);
#endif
#ifdef USE_INPUT_SDL
				inputb_sdl_event_press(event.key.keysym.sym);
#endif

				/* toggle fullscreen check */
				if (event.key.keysym.sym == SDLK_RETURN
					&& (event.key.keysym.mod & KMOD_ALT) != 0) {
					if (SDL_WasInit(SDL_INIT_VIDEO) && SDL_GetVideoSurface()) {
						SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());

						if ((SDL_GetVideoSurface()->flags & SDL_FULLSCREEN) != 0) {
							SDL_ShowCursor(SDL_DISABLE);
						} else {
							SDL_ShowCursor(SDL_ENABLE);
						}
					}
				}
			break;
			case SDL_SYSWMEVENT :
			break;
			case SDL_KEYUP :
#ifdef USE_KEYBOARD_SDL
				keyb_sdl_event_release(event.key.keysym.sym);
#endif
#ifdef USE_INPUT_SDL
				inputb_sdl_event_release(event.key.keysym.sym);
#endif
			break;
			case SDL_MOUSEMOTION :
#ifdef USE_MOUSE_SDL
				mouseb_sdl_event_move(event.motion.xrel, event.motion.yrel);
#endif
			break;
			case SDL_MOUSEBUTTONDOWN :
#ifdef USE_MOUSE_SDL
				if (event.button.button > 0)
					mouseb_sdl_event_press(event.button.button-1);
#endif
			break;
			case SDL_MOUSEBUTTONUP :
#ifdef USE_MOUSE_SDL
				if (event.button.button > 0)
					mouseb_sdl_event_release(event.button.button-1);
#endif
			break;
			case SDL_QUIT :
				OS.is_quit = 1;
				break;
		}
	}
}

void* os_internal_sdl_get(void)
{
	return &OS;
}

const char* os_internal_sdl_title_get(void)
{
	return OS.title_buffer;
}

void* os_internal_window_get(void)
{
	SDL_SysWMinfo info;

	SDL_VERSION(&info.version);

	if (!SDL_GetWMInfo(&info))
		return 0;

	return info.window;
}

/***************************************************************************/
/* Signal */

int os_is_quit(void)
{
	return OS.is_quit;
}

void os_default_signal(int signum, void* info, void* context)
{
	log_std(("os: signal %d\n", signum));

#if defined(USE_VIDEO_SDL) || defined(USE_VIDEO_SVGAWIN)
	log_std(("os: adv_video_abort\n"));
	{
		extern void adv_video_abort(void);
		adv_video_abort();
	}
#endif

#if defined(USE_SOUND_SDL)
	log_std(("os: sound_abort\n"));
	{
		extern void soundb_abort(void);
		soundb_abort();
	}
#endif

	SDL_Quit();

	target_mode_reset();

	log_std(("os: close log\n"));
	log_abort();

	target_signal(signum, info, context);
}

/***************************************************************************/
/* Extension */

typedef BOOLEAN (WINAPI* HidD_GetAttributes_type)(HANDLE HidDeviceObject, PHIDD_ATTRIBUTES Attributes);

int GetRawInputDeviceHIDInfo(const char* name, unsigned* vid, unsigned* pid, unsigned* rev)
{
	char* buffer;
	const char* last;
	HANDLE h, l;
	HIDD_ATTRIBUTES attr;
	HidD_GetAttributes_type HidD_GetAttributes_ptr;
	
	last = strrchr(name, '\\');
	if (!last)
		last = name;
	else
		++last;

	l = LoadLibrary("HID.DLL");
	if (!l) {
		log_std(("ERROR:GetRawInputDeviceHIDInfo: error loading HID.DLL\n"));
		goto err;
	}

	HidD_GetAttributes_ptr = (HidD_GetAttributes_type)GetProcAddress(l, "HidD_GetAttributes");
	if (!HidD_GetAttributes_ptr) {
		log_std(("ERROR:GetRawInputDeviceHIDInfo: error getting HidD_GetAttributes\n"));
		goto err_unload;
	}

	buffer = malloc(16 + strlen(name));
	strcpy(buffer, "\\\\?\\");
	strcat(buffer, last);

	h = CreateFile(buffer, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		log_std(("ERROR:GetRawInputDeviceHIDInfo: error %d in CreateFile(%s)\n", (unsigned)GetLastError(), buffer));
		goto err_free;
	}

	memset(&attr, 0, sizeof(HIDD_ATTRIBUTES));
	attr.Size = sizeof(HIDD_ATTRIBUTES);
	if (!HidD_GetAttributes_ptr(h, &attr)) {
		if (GetLastError() != ERROR_INVALID_FUNCTION)
			log_std(("ERROR:GetRawInputDeviceHIDInfo: error %d in HidD_GetAttributes(%s)\n", (unsigned)GetLastError(), buffer));
		goto err_free;
	}

	*vid = attr.VendorID;
	*pid = attr.ProductID;
	*rev = attr.VersionNumber;

	free(buffer);
	FreeLibrary(l);

	return 0;

err_close:
	CloseHandle(h);
err_free:
	free(buffer);
err_unload:
	FreeLibrary(l);
err:
	return -1;
}

/***************************************************************************/
/* Main */

int main(int argc, char* argv[])
{
	windows_save_hot_key();
	windows_splash_start();

	if (target_init() != 0)
		return EXIT_FAILURE;

	if (file_init() != 0) {
		target_done();
		return EXIT_FAILURE;
	}

	if (os_main(argc, argv) != 0) {
		file_done();
		target_done();
		return EXIT_FAILURE;
	}

	file_done();
	target_done();

	return EXIT_SUCCESS;
}

