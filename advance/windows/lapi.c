/*
 * CPN Mouse Driver API
 * Copyright (c) 2002 CPN Group, University of Aarhus
 */

/* 
 * From the http://sourceforge.net/projects/cpnmouse/ project.
 * Released with LGPL license according with the SourceForge catalog.
 */

#define INITGUID

#include <windows.h>
#include <setupapi.h>
#include <initguid.h>

#include "lapi.h"

#include <stdio.h>
#include <stdlib.h>

// {DB4BBC1E-CAC8-47e9-8414-7365167FECA3}
DEFINE_GUID( GUID_CLASS_MOUSE_CPNTOOLS, 0xdb4bbc1e, 0xcac8, 0x47e9, 0x84, 0x14, 0x73, 0x65, 0x16, 0x7f, 0xec, 0xa3);

#define MAXLENGTH 1000

typedef struct {
	HANDLE handle;
	HANDLE suspend;
	char *devicename;
} MouseData;

callback theCallback = NULL;
MouseData *mice = NULL;
int maxmouse = 1;
int maxused = 0;
HANDLE hMutex = NULL;

void __cdecl lRegisterCallback(callback c) {
	if (hMutex == NULL) {
		hMutex = CreateMutex(NULL, FALSE, NULL);
	}
	theCallback = c;
}

void __cdecl lUnRegisterCallback(void) {
	theCallback = NULL;
	CloseHandle(hMutex); hMutex = NULL;
}

void ApcCallback(PVOID NormalContext, PVOID SystemArgument1, PVOID SystemArgument2) {
	if (theCallback) {
		while (WaitForSingleObject(hMutex, INFINITE) != WAIT_OBJECT_0) ;
		theCallback((int) NormalContext,
				(signed int) (signed short int) ((((unsigned int) SystemArgument1) & 0xffff0000) >> 16),
				(signed int) (signed short int) (((unsigned int ) SystemArgument1) & 0xffff),
				(unsigned int) ((((unsigned int) SystemArgument2) & 0xffff0000) >> 16),
				(int) (((unsigned int) SystemArgument2) & 0xffff));
		ReleaseMutex(hMutex);
	}
}

int __cdecl lGetMice(int count) {
	int number = 0;
	HANDLE new = INVALID_HANDLE_VALUE;
	HDEVINFO hardwareDeviceInfo;
	SP_INTERFACE_DEVICE_DATA deviceInfoData;
	BOOLEAN done = FALSE;
	ULONG i = 0;

	PSP_INTERFACE_DEVICE_DETAIL_DATA functionClassDeviceData = NULL;
	ULONG predictedLength = 0;
	ULONG requiredLength = 0;

	hardwareDeviceInfo = SetupDiGetClassDevs(
			(LPGUID) &GUID_CLASS_MOUSE_CPNTOOLS,
			NULL, // Define no enumerator (global)
			NULL, // Define no
			(DIGCF_PRESENT | // Only Devices present
			 DIGCF_DEVICEINTERFACE)); // Function class devices

	deviceInfoData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

	while ((number < count) || (count == 0)) {
		if (SetupDiEnumDeviceInterfaces(
					hardwareDeviceInfo,
					0, // Don't care about specific PDOs
					(LPGUID) &GUID_CLASS_MOUSE_CPNTOOLS,
					i,
					&deviceInfoData)) {
			SetupDiGetInterfaceDeviceDetail(
					hardwareDeviceInfo,
					&deviceInfoData,
					NULL, // probe, no output buffer,
					0, // probe, output buffer of length 0
					&requiredLength,
					NULL); // not interested in specific dev-node

			functionClassDeviceData = (PSP_INTERFACE_DEVICE_DETAIL_DATA) malloc(requiredLength);
			functionClassDeviceData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
			predictedLength = requiredLength;

			if (SetupDiGetInterfaceDeviceDetail(
						hardwareDeviceInfo,
						&deviceInfoData,
						functionClassDeviceData,
						predictedLength,
						&requiredLength,
						NULL)) {
				char c[MAXLENGTH];
				sprintf(c, "%s\\execute\\get\\%u\\%u", 
						functionClassDeviceData->DevicePath,
						maxused + 1,
						(unsigned)&ApcCallback);

				new = CreateFile(
						c,
						0,
						FILE_SHARE_READ | FILE_SHARE_WRITE, // Don't want to share access
						NULL, // no SECURITY_ATTRIBUTES structure
						OPEN_EXISTING, // No special create flags
						0, // No special attributes
						NULL); // No template file

				sprintf(c, "%s", functionClassDeviceData->DevicePath);

				if (new != INVALID_HANDLE_VALUE) {
					maxused++;
					number++;
					if (maxused >= maxmouse) {
						maxmouse *= 2;
						mice = (MouseData *) realloc(mice, maxmouse * sizeof(MouseData));
					}
					mice[maxused].handle = new;
					mice[maxused].suspend = NULL;
					mice[maxused].devicename = (char *) malloc((strlen(functionClassDeviceData->DevicePath) + 1) * sizeof(char));
					strcpy(mice[maxused].devicename, functionClassDeviceData->DevicePath);
				}
			}

			free(functionClassDeviceData);
		} else if (ERROR_NO_MORE_ITEMS == GetLastError())
			break;
		i++;
	}

	SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
	return number;
}

int __cdecl lHasMouse(int number) {
	if ((number <= maxused) && (number > 0)) {
		return mice[number].handle != 0;
	}
	return 0;
}

void __cdecl lUnGetMouse(int number) {
	if (lHasMouse(number)) {
		lUnSuspendMouse(number);
		CloseHandle(mice[number].handle);
		mice[number].handle = NULL;
		free(mice[number].devicename);
		mice[number].devicename = NULL;
		while ((maxused > 0) && (!lHasMouse(maxused))) maxused--;
	}
}

void __cdecl lUnGetAllMice() {
	int i;
	for (i = 1; i <= maxused; ++i) {
	    lUnGetMouse(i);
    }
}

void __cdecl lSuspendMouse(number) {
	if (lHasMouse(number)) {
		if (mice[number].suspend == NULL) {
			char c[MAXLENGTH];
			sprintf(c, "%s\\execute\\suspend", mice[number].devicename);
			mice[number].suspend = CreateFile(
					c, // Filename
					0, // Access
					FILE_SHARE_READ | FILE_SHARE_WRITE, // Share
					NULL, // No SECURITY_ATTRIBUTES
					OPEN_EXISTING,
					0, // No special attributes
					NULL); // No template file
		}
	}
}

void __cdecl lUnSuspendMouse(number) {
	if (lHasMouse(number)) {
		if (mice[number].suspend) {
			CloseHandle(mice[number].suspend);
			mice[number].suspend = NULL;
		}
	}
}

const char *__cdecl lGetDevicePath(int number) {
	if (lHasMouse(number))
		return mice[number].devicename;
	return 0;
}
