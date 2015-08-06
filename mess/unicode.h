/*********************************************************************

	unicode.h

	Unicode related functions

	This code is for converting between UTF-8, UTF-16, and 32-bit
	Unicode strings.  These functions roughly parallel C runtime
	library functions like mbtowc() and similar functions, but are
	specific for these Unicode encodings.  Specifically, there are
	functions that convert UTF-8 and UTF-16 char clusters to and from
	singular 32-bit Unicode chars.

	Note that not all possible permutations are actually implemented
	yet.  Some code still needs to be refactored out of inputx.c

*********************************************************************/

#ifndef UNICODE_H
#define UNICODE_H

#include <stdlib.h>
#include "osd_cpu.h"

typedef UINT16 utf16_char_t;
typedef UINT32 unicode_char_t;

/* these defines specify the maximum size of different types of Unicode
 * character encodings */
#define UTF8_CHAR_MAX	6
#define UTF16_CHAR_MAX	2

/* tests to see if a unicode char is a valid code point */
int uchar_isvalid(unicode_char_t uchar);

/* converting strings to 32-bit Unicode chars */
int uchar_from_utf8(unicode_char_t *uchar, const char *utf8char, size_t count);
int uchar_from_utf16(unicode_char_t *uchar, const utf16_char_t *utf16char, size_t count);
int uchar_from_utf16f(unicode_char_t *uchar, const utf16_char_t *utf16char, size_t count);

/* converting 32-bit Unicode chars to strings */
int utf8_from_uchar(char *utf8string, size_t count, unicode_char_t uchar);
int utf16_from_uchar(utf16_char_t *utf16string, size_t count, unicode_char_t uchar);
int utf16f_from_uchar(utf16_char_t *utf16string, size_t count, unicode_char_t uchar);

#ifdef LSB_FIRST
#define uchar_from_utf16be	uchar_from_utf16f
#define uchar_from_utf16le	uchar_from_utf16
#define utf16be_from_uchar	utf16f_from_uchar
#define utf16le_from_uchar	utf16_from_uchar
#else
#define uchar_from_utf16be	uchar_from_utf16
#define uchar_from_utf16le	uchar_from_utf16f
#define utf16be_from_uchar	utf16_from_uchar
#define utf16le_from_uchar	utf16f_from_uchar
#endif

#endif /* UNICODE_H */


