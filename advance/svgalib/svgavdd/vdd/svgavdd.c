#include <windows.h>
#include <vddsvc.h>

#define FILE_DEVICE_SVGALIB 0x00008000

static HANDLE the_handle;

BOOL VDDInitialize(HANDLE hVdd, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH :
		the_handle = CreateFile("\\\\.\\SVGALIB", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (the_handle == INVALID_HANDLE_VALUE) {
			MessageBox (NULL, "Error opening the SVGAWIN device", "SVGAVDD", MB_OK | MB_ICONEXCLAMATION);
			return FALSE;
		}
		break;
	case DLL_PROCESS_DETACH :
		if (the_handle == INVALID_HANDLE_VALUE) {
			CloseHandle(the_handle);
			the_handle = INVALID_HANDLE_VALUE;
		}
		break;
	default:
		break;
    }

    return TRUE;
}

VOID VDDRegisterInit(VOID)
{
	setCF(0);
	return;
}

VOID VDDDispatch(VOID)
{
	ULONG input_address;
	ULONG output_address;
	VOID* input_ptr;
	VOID* output_ptr;
	DWORD input_size;
	DWORD output_size;
	DWORD output_returned;
	DWORD ioctl;

	ioctl = getBX() | (FILE_DEVICE_SVGALIB << 16);
	input_size = getCX();
	output_size = getDX();
	
	if (input_size != 0) {
		input_address = ((DWORD)getDS() << 16) | getSI();
		input_ptr = GetVDMPointer(input_address, input_size, FALSE);
	} else {
		input_address = 0;
		input_ptr = 0;
	}

	if (output_size != 0) {
		output_address = ((DWORD)getES() << 16) | getDI();
		output_ptr = GetVDMPointer(output_address, output_size, FALSE);
	} else {
		output_address = 0;
		output_ptr = 0;
	}

	if (!DeviceIoControl(the_handle, ioctl, input_ptr, input_size, output_ptr, output_size, &output_returned, NULL)) {
		setCF(1);
		return;
	}

	if (input_size != 0) {
		FreeVDMPointer(input_address, input_size, input_ptr, FALSE);
	}

	if (output_size != 0) {
		FreeVDMPointer(output_address, output_size, output_ptr, FALSE);
	}

	setCX(output_returned);

	setCF(0);
	return;
}
