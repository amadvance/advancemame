#include <stdio.h>
#include <tchar.h>
#include "assoc.h"
#include "wimgres.h"
#include "strconv.h"
#include "../imgtool.h"

#define CONTROL_START 10000

struct assocdlg_info
{
	imgtool_library *library;
	int extension_count;
	const char *extensions[128];
};

static const struct win_association_info assoc_info =
{
	TEXT("ImgtoolImage"),
	0,
	TEXT("%1")
};


static INT_PTR CALLBACK win_association_dialog_proc(HWND dialog, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	INT_PTR rc = 0;
	struct assocdlg_info *dlginfo;
	HWND ok_button, cancel_button, control;
	LRESULT font;
	LONG_PTR l;
	RECT r1, r2;
	int xmargin, ymargin, y, width, i;
	int height = 20;
	int id;
	DWORD style;
	TCHAR buf[32];
	BOOL is_set;
	BOOL currently_set;

	switch(message)
	{
		case WM_INITDIALOG:
			dlginfo = (struct assocdlg_info *) lparam;
			ok_button = GetDlgItem(dialog, IDOK);
			cancel_button = GetDlgItem(dialog, IDCANCEL);
			font = SendMessage(ok_button, WM_GETFONT, 0, 0);
			l = (LONG_PTR) dlginfo;
			SetWindowLongPtr(dialog, GWLP_USERDATA, l);

			GetWindowRect(cancel_button, &r1);
			GetWindowRect(dialog, &r2);
			xmargin = r1.left - r2.left;
			ymargin = y = r1.top - r2.top - 20;
			width = r2.right - r2.left - xmargin * 2;

			for (i = 0; i < dlginfo->extension_count; i++)
			{
				style = WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_AUTOCHECKBOX;
				_sntprintf(buf, sizeof(buf) / sizeof(buf[0]), TEXT(".%s"), U2T(dlginfo->extensions[i]));

				control = CreateWindow(TEXT("BUTTON"), buf, style, 
					 xmargin, y, width, height, dialog, NULL, NULL, NULL);
				if (!control)
					return -1;

				if (win_is_extension_associated(&assoc_info, buf))
					SendMessage(control, BM_SETCHECK, TRUE, 0);
				SendMessage(control, WM_SETFONT, font, 0);
				SetWindowLong(control, GWL_ID, CONTROL_START + i);

				y += height;
			}

			r1.top += y;
			r1.bottom += y;
			SetWindowPos(cancel_button, NULL, r1.left - r2.left, r1.top - r2.top,
				0, 0, SWP_NOZORDER | SWP_NOSIZE);
			GetWindowRect(ok_button, &r1);
			r1.top += y;
			r1.bottom += y;
			SetWindowPos(ok_button, NULL, r1.left - r2.left, r1.top - r2.top,
				0, 0, SWP_NOZORDER | SWP_NOSIZE);
			r2.bottom += y + r1.bottom - r1.top;
			SetWindowPos(dialog, NULL, 0, 0, r2.right - r2.left, r2.bottom - r2.top, SWP_NOZORDER | SWP_NOMOVE);
			break;

		case WM_COMMAND:
			if (HIWORD(wparam) == BN_CLICKED)
			{
				l = GetWindowLongPtr(dialog, GWLP_USERDATA);
				dlginfo = (struct assocdlg_info *) l;
				id = LOWORD(wparam);

				if ((id == IDOK) || (id == IDCANCEL))
				{
					if (id == IDOK)
					{
						for (i = 0; i < dlginfo->extension_count; i++)
						{
							is_set = SendMessage(GetDlgItem(dialog, CONTROL_START + i), BM_GETCHECK, 0, 0);

							_sntprintf(buf, sizeof(buf) / sizeof(buf[0]), TEXT(".%s"), U2T(dlginfo->extensions[i]));
							currently_set = win_is_extension_associated(&assoc_info, buf);

							if (is_set && !currently_set)
								win_associate_extension(&assoc_info, buf, TRUE);
							else if (!is_set && currently_set)
								win_associate_extension(&assoc_info, buf, FALSE);
						}
					}

					EndDialog(dialog, id);
				}
			}
			break;
	}
	return rc;
}



static int CLIB_DECL extension_compare(const void *p1, const void *p2)
{
	const char *e1 = *((const char **) p1);
	const char *e2 = *((const char **) p2);
	return strcmp(e1, e2);
}



static void setup_extensions(struct assocdlg_info *dlginfo)
{
	const struct ImageModule *module = NULL;
	const char *ext;
	int i;

	dlginfo->extension_count = 0;

	while((module = imgtool_library_iterate(dlginfo->library, module)) != NULL)
	{
		ext = module->extensions;
		while(ext[0])
		{
			for (i = 0; i < dlginfo->extension_count; i++)
			{
				if (!strcmp(dlginfo->extensions[i], ext))
					break;
			}
			if (i >= dlginfo->extension_count)
			{
				assert(dlginfo->extension_count < sizeof(dlginfo->extensions) / sizeof(dlginfo->extensions[0]));
				if (dlginfo->extension_count < sizeof(dlginfo->extensions) / sizeof(dlginfo->extensions[0]))
				{
					dlginfo->extensions[dlginfo->extension_count++] = ext;
				}
			}
			ext = ext + strlen(ext) + 1;
		}
	}

	qsort((void *) dlginfo->extensions, dlginfo->extension_count,
		sizeof(dlginfo->extensions[0]), extension_compare);
}



void win_association_dialog(HWND parent, imgtool_library *library)
{
	struct assocdlg_info dlginfo;

	dlginfo.library = library;
	setup_extensions(&dlginfo);

	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ASSOCIATIONS), parent,
		win_association_dialog_proc, (LPARAM) &dlginfo);
}
