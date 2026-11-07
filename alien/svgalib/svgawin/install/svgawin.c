#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#define DRIVER_NAME "SVGAWIN"
#define DISPLAY_NAME "SVGALIB Helper Driver"
#define FILE_NAME "svgawin"

int copy(char* srcpath, char* dstpath)
{
	CHAR srcdir[MAX_PATH];
	CHAR dstdir[MAX_PATH];

	if (!GetCurrentDirectory(MAX_PATH+1, srcdir)) {
		printf("Error getting the current directory.\n");
		return -1;
	}

	sprintf(srcpath, "%s\\%s.sys", srcdir, FILE_NAME);

	if (!GetSystemDirectory( dstdir, MAX_PATH+1)) {
		printf("Error getting the system directory.\n");
		return -1;
	}

	sprintf(dstpath, "%s\\drivers\\%s.sys", dstdir, FILE_NAME);

	if (!CopyFile(srcpath, dstpath, FALSE)) {
		printf("Error copying the driver from %s to %s.\n", srcpath, dstpath);
		return -1;
	}

	return 0;
}

static int uninstall(SC_HANDLE schSCManager) {
	SC_HANDLE schService;
	SERVICE_STATUS serviceStatus;

	schService = OpenService(schSCManager, DRIVER_NAME, SERVICE_ALL_ACCESS);
	if (schService == NULL) {
		if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
			printf("The driver isn't installed.\n");
			return -1;
		}
		printf("Error querying the driver.\n");
		return -1;
	}

	if (!QueryServiceStatus(schService, &serviceStatus)) {
		printf("Error querying the driver.\n");
		CloseServiceHandle(schService);
		return -1;
	}

	if (serviceStatus.dwCurrentState != SERVICE_STOPPED) {
		if (ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus) == 0) {
			printf("Error stopping the driver.\n");
			return -1;
		}

		while (serviceStatus.dwCurrentState != SERVICE_STOPPED) {
			Sleep(100);

			if (!QueryServiceStatus(schService, &serviceStatus)) {
				printf("Error querying the driver.\n");
				CloseServiceHandle(schService);
				return -1;
			}
		}
	}

	if (!DeleteService(schService)) {
		printf("Error deleting the driver.\n");
		CloseServiceHandle(schService);
		return -1;
	}

	CloseServiceHandle(schService);
	return 0;
}

static int install(SC_HANDLE schSCManager) {
	SC_HANDLE schService;
	SERVICE_STATUS serviceStatus;
	char srcpath[MAX_PATH];
	char dstpath[MAX_PATH];

	if (copy(srcpath,dstpath) != 0) {
		return -1;
	}

	schService = CreateService(
		schSCManager, 
		DRIVER_NAME, DISPLAY_NAME,
		SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
		SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
		dstpath,
		NULL, NULL, NULL, NULL, NULL
	);
	if (schService == NULL) {
		if (GetLastError() == ERROR_SERVICE_EXISTS) {
			printf("The driver is already installed.\n");
			return -1;
		}
		printf("Error installing the driver.\n");
		return -1;
	}

	if (!StartService(schService,0,NULL)) {
		printf("Error starting the driver.\n");
		CloseServiceHandle(schService);
		return -1;
	}

	do {
		Sleep(100);

		if (!QueryServiceStatus(schService, &serviceStatus)) {
			CloseServiceHandle(schService);
			return -1;
		}

	} while (serviceStatus.dwCurrentState != SERVICE_RUNNING);

	return 0;

}

static void help(void) {
	printf("SVGALIB driver installation for Windows NT/2000/XP v0.1 " __DATE__ "\n");
	printf(
"Usage:\n"
"    svgalib [/l] [/u]\n"
"Commands:\n"
"    /l          Install and load the driver.\n"
"    /u          Unload and uninstall the driver.\n"
);
}

int optionmatch(const char* arg, const char* opt) {
	return (arg[0] == '-' || arg[0] == '/') && stricmp(arg+1,opt) == 0;
}

int main(int argc, char *argv[])
{
	SC_HANDLE schSCManager;
	OSVERSIONINFO VersionInformation;
	int arg_install;
	int arg_uninstall;
	int i;

	if (argc <= 1) {
		help();
		exit(EXIT_FAILURE);
	}

	arg_install = 0;
	arg_uninstall = 0;
	for(i=1;i<argc;++i) {
		if (optionmatch(argv[i],"l")) {
			arg_install = 1;
		} else if (optionmatch(argv[i],"u")) {
			arg_uninstall = 1;
		} else {
			printf("Unknown option %s.\n", argv[i]);
			exit(EXIT_FAILURE);
		}
	}

	printf("SVGALIB driver installation program for Windows NT/2000/XP\n");

	VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&VersionInformation)) {
		printf("Error getting the Windows version.\n");
		exit(EXIT_FAILURE);
	}

	if (VersionInformation.dwPlatformId != VER_PLATFORM_WIN32_NT) {
		printf("This program runs only on Windows NT/2000/XP.\n");
		exit(EXIT_FAILURE);
	}

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == NULL) {
		printf("Error opening the services database. Are you Administrator ?\n");
		exit(EXIT_FAILURE);
	}

	if (arg_uninstall) {
		if (uninstall(schSCManager) != 0) {
			CloseServiceHandle(schSCManager);
			exit(EXIT_FAILURE);
		}
		printf("Driver uninstalled.\n");
	}

	if (arg_install) {
		if (install(schSCManager) != 0) {
			CloseServiceHandle(schSCManager);
			exit(EXIT_FAILURE);
		}
		printf("Driver installed.\n");
	}

	CloseServiceHandle(schSCManager);
	return EXIT_SUCCESS;
}
