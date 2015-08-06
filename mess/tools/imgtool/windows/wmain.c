//============================================================
//
//	wmain.h - Win32 GUI Imgtool main code
//
//============================================================

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

#include "wimgtool.h"
#include "wimgres.h"
#include "hexview.h"
#include "../modules.h"

imgtool_library *library;


int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance,
	LPSTR command_line, int cmd_show)
{
	MSG msg;
	HWND window;
	BOOL b;
	int rc = -1;
	imgtoolerr_t err;
	HACCEL accel = NULL;
	TCHAR *s;
	
	// Initialize Windows classes
	InitCommonControls();
	if (!wimgtool_registerclass())
		goto done;
	if (!hexview_registerclass())
		goto done;

	// Initialize the Imgtool library
	err = imgtool_create_cannonical_library(TRUE, &library);
	if (!library)
		goto done;
	imgtool_library_sort(library, ITLS_DESCRIPTION);

	window = CreateWindow(wimgtool_class, NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	if (!window)
		goto done;

#ifdef MAME_DEBUG
	// run validity checks and if appropriate, warn the user
	if (imgtool_validitychecks())
	{
		MessageBox(window,
			TEXT("Imgtool has failed its consistency checks; this build has problems"),
			wimgtool_producttext, MB_OK);
	}
#endif

	// load image specified at the command line
	if (command_line && command_line[0])
	{
		s = (TCHAR *) alloca((_tcslen(command_line) + 1) * sizeof(TCHAR));
		_tcscpy(s, command_line);
		rtrim(s);

		if ((s[0] == '\"') && (s[_tcslen(s)-1] == '\"'))
		{
			s[_tcslen(s)-1] = '\0';
			command_line = s + 1;
		}
		
		err = wimgtool_open_image(window, NULL, command_line, OSD_FOPEN_RW);
		if (err)
			wimgtool_report_error(window, err, command_line, NULL);
	}

	accel = LoadAccelerators(NULL, MAKEINTRESOURCE(IDA_WIMGTOOL_MENU));

	// pump messages until the window is gone
	while(IsWindow(window))
	{
		b = GetMessage(&msg, NULL, 0, 0);
		if (b <= 0)
		{
			window = NULL;
		}
		else if (!TranslateAccelerator(window, accel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	rc = 0;

done:
	if (library)
		imgtool_library_close(library);
	if (accel)
		DestroyAcceleratorTable(accel);
	return rc;
}
