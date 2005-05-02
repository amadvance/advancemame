/*
 * CPN Mouse Driver API
 * Copyright (c) 2002 CPN Group, University of Aarhus
 */

/* 
 * From the http://sourceforge.net/projects/cpnmouse/ project.
 * Released with LGPL license according with the SourceForge catalog.
 */

#ifndef _lapi_h
#define _lapi_h

/** The callback is defined like this
 * It does not have to be thread-safe, as the library
 * takes care of this. Be aware though that it can be
 * called at any time!
 */
typedef void (__cdecl *callback)(int number,
		signed int dx,
		signed int dy,
		unsigned int buttons,
		int suspended);

/// None of the operations are thread-safe!

/** Register the callback. No information about the mice
 * is returned
 * @param c - The callback to register
 */
void __cdecl lRegisterCallback(callback c);

/** Removes a previously defined callback.
 */
void __cdecl lUnRegisterCallback(void);

/** Try to allocate the specified number of mice.
 * @param count - The number of mice to try to allocate
 * @return the number of mice actually allocated
 */
int __cdecl lGetMice(int count);

/** Query if we have hooked a mouse with the specified
 * number.
 * @param number - The mouse number to query
 * @return nonzero if we have allocated the mouse, zero
 * otherwise
 */
int __cdecl lHasMouse(int number);

/** UnGet the specified mouse if it is hooked. No error
 * if the mouse isn't hooked. The mouse is handed over to
 * the operatingsystem to control the system mouse.
 * @param number - The number of the mouse to release
 */
void __cdecl lUnGetMouse(int number);

/** Release all registered mice.
 */
void __cdecl lUnGetAllMice();

/** Suspend the specified mouse. The mouse now again sends
 * events to the system mouse, but also special events to
 * the application. No error if we have not hooked the
 * mouse. This is useful for releasing a mouse, when the
 * cursor leaves our application.
 * @param number - The number of the mouse to suspend
 */
void __cdecl lSuspendMouse(int number);

/** Reclaim a suspended mouse. If the mouse is not
 * suspended or not hooked, nothing happens. This is
 * useful for getting the mouse back when it enters our
 * application.
 * @param number - The number of the mouse to reclaim
 */
void __cdecl lUnSuspendMouse(int number);

/** Get the devicePath.  You cannot assume anything
 * about it, but you may be able to extract some device
 * information from it.
 * @param number - The number of the mouse to reclaim
 * @return ­ the DevicePath
 */
const char *__cdecl lGetDevicePath(int number);

#endif // ifndef _lapi_h
