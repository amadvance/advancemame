/***************************************************************************

    mamecore.h

    General core utilities and macros used throughout MAME.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __MAMECORE_H__
#define __MAMECORE_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osd_cpu.h"



/***************************************************************************

    Common types

***************************************************************************/

/* FPTR is a type that can be used to cast a pointer to a scalar */
/* 64-bit platforms should define __LP64__ */
#ifdef __LP64__
typedef UINT64 FPTR;
#else
typedef UINT32 FPTR;
#endif


/* ----- for generic function pointers ----- */
typedef void genf(void);



/* These are forward struct declarations that are used to break
   circular dependencies in the code */
typedef struct _mame_display mame_display;
typedef struct _game_driver game_driver;
typedef struct _machine_config machine_config;
typedef struct _rom_load_data rom_load_data;
typedef struct _xml_data_node xml_data_node;
typedef struct _performance_info performance_info;
typedef struct _osd_create_params osd_create_params;
typedef struct _gfx_element gfx_element;
typedef struct _input_port_entry input_port_entry;
typedef struct _input_port_default_entry input_port_default_entry;
typedef struct _mame_file mame_file;
typedef struct _chd_file chd_file;
typedef enum _osd_file_error osd_file_error;


/* pen_t is used to represent pixel values in mame_bitmaps */
typedef UINT32 pen_t;

/* rgb_t is used to represent 32-bit (A)RGB values */
typedef UINT32 rgb_t;

/* stream_sample_t is used to represent a single sample in a sound stream */
typedef INT32 stream_sample_t;

/* input code is used to represent an abstracted input type */
typedef UINT32 input_code;


/* mame_bitmaps are used throughout the code */
typedef struct _mame_bitmap mame_bitmap;
struct _mame_bitmap
{
	int width,height;	/* width and height of the bitmap */
	int depth;			/* bits per pixel */
	void **line;		/* pointers to the start of each line - can be UINT8 **, UINT16 ** or UINT32 ** */

	/* alternate way of accessing the pixels */
	void *base;			/* pointer to pixel (0,0) (adjusted for padding) */
	int rowpixels;		/* pixels per row (including padding) */
	int rowbytes;		/* bytes per row (including padding) */

	/* functions to render in the correct orientation */
	void (*plot)(struct _mame_bitmap *bitmap,int x,int y,pen_t pen);
	pen_t (*read)(struct _mame_bitmap *bitmap,int x,int y);
	void (*plot_box)(struct _mame_bitmap *bitmap,int x,int y,int width,int height,pen_t pen);
};


/* rectangles are used throughout the code */
typedef struct _rectangle rectangle;
struct _rectangle
{
	int min_x,max_x;
	int min_y,max_y;
};



/***************************************************************************
 * Union of UINT8, UINT16 and UINT32 in native endianess of the target
 * This is used to access bytes and words in a machine independent manner.
 * The upper bytes h2 and h3 normally contain zero (16 bit CPU cores)
 * thus PAIR.d can be used to pass arguments to the memory system
 * which expects 'int' really.
***************************************************************************/
typedef union
{
#ifdef LSB_FIRST
	struct { UINT8 l,h,h2,h3; } b;
	struct { UINT16 l,h; } w;
#else
	struct { UINT8 h3,h2,h,l; } b;
	struct { UINT16 h,l; } w;
#endif
	UINT32 d;
} PAIR;




/***************************************************************************
 * Union of UINT8, UINT16, UINT32, and UINT64 in native endianess of
 * the target.  This is used to access bytes and words in a machine
 * independent manner.
***************************************************************************/
typedef union
{
#ifdef LSB_FIRST
	struct { UINT8 l,h,h2,h3,h4,h5,h6,h7; } b;
	struct { UINT16 l,h,h2,h3; } w;
	struct { UINT32 l,h; } d;
#else
	struct { UINT8 h7,h6,h5,h4,h3,h2,h,l; } b;
	struct { UINT16 h3,h2,h,l; } w;
	struct { UINT32 h,l; } d;
#endif
	UINT64 lw;
} PAIR64;



/***************************************************************************

    Common constants

***************************************************************************/

/* Ensure that TRUE/FALSE are defined */
#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif


/* this is not part of the C/C++ standards and is not present on */
/* strict ANSI compilers or when compiling under GCC with -ansi */
#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif



/***************************************************************************

    Common macros

***************************************************************************/

/* Standard MAME assertion macros */
#undef assert
#undef assert_always

#ifdef MAME_DEBUG
#define assert(x)	do { if (!(x)) fatalerror("assert: %s:%d: %s", __FILE__, __LINE__, #x); } while (0)
#define assert_always(x, msg) do { if (!(x)) fatalerror("Fatal error: %s\nCaused by assert: %s:%d: %s", msg, __FILE__, __LINE__, #x); } while (0)
#else
#define assert(x)
#define assert_always(x, msg) do { if (!(x)) fatalerror("Fatal error: %s (%s:%d)", msg, __FILE__, __LINE__); } while (0)
#endif



/* Standard MIN/MAX macros */
#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif


/* Highly useful macro for compile-time knowledge of an array size */
#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))


/* macros to convert radians to degrees and degrees to radians */
#define RADIAN_TO_DEGREE(x)   ((180.0 / M_PI) * (x))
#define DEGREE_TO_RADIAN(x)   ((M_PI / 180.0) * (x))


/* U64 and S64 are used to wrap long integer constants. */
#ifdef __GNUC__
#define U64(val) val##ULL
#define S64(val) val##LL
#else
#define U64(val) val
#define S64(val) val
#endif


/* Macros for normalizing data into big or little endian formats */
#define FLIPENDIAN_INT16(x)	(((((UINT16) (x)) >> 8) | ((x) << 8)) & 0xffff)
#define FLIPENDIAN_INT32(x)	((((UINT32) (x)) << 24) | (((UINT32) (x)) >> 24) | \
	(( ((UINT32) (x)) & 0x0000ff00) << 8) | (( ((UINT32) (x)) & 0x00ff0000) >> 8))
#define FLIPENDIAN_INT64(x)	\
	(												\
		(((((UINT64) (x)) >> 56) & ((UINT64) 0xFF)) <<  0)	|	\
		(((((UINT64) (x)) >> 48) & ((UINT64) 0xFF)) <<  8)	|	\
		(((((UINT64) (x)) >> 40) & ((UINT64) 0xFF)) << 16)	|	\
		(((((UINT64) (x)) >> 32) & ((UINT64) 0xFF)) << 24)	|	\
		(((((UINT64) (x)) >> 24) & ((UINT64) 0xFF)) << 32)	|	\
		(((((UINT64) (x)) >> 16) & ((UINT64) 0xFF)) << 40)	|	\
		(((((UINT64) (x)) >>  8) & ((UINT64) 0xFF)) << 48)	|	\
		(((((UINT64) (x)) >>  0) & ((UINT64) 0xFF)) << 56)		\
	)

#ifdef LSB_FIRST
#define BIG_ENDIANIZE_INT16(x)		(FLIPENDIAN_INT16(x))
#define BIG_ENDIANIZE_INT32(x)		(FLIPENDIAN_INT32(x))
#define BIG_ENDIANIZE_INT64(x)		(FLIPENDIAN_INT64(x))
#define LITTLE_ENDIANIZE_INT16(x)	(x)
#define LITTLE_ENDIANIZE_INT32(x)	(x)
#define LITTLE_ENDIANIZE_INT64(x)	(x)
#else
#define BIG_ENDIANIZE_INT16(x)		(x)
#define BIG_ENDIANIZE_INT32(x)		(x)
#define BIG_ENDIANIZE_INT64(x)		(x)
#define LITTLE_ENDIANIZE_INT16(x)	(FLIPENDIAN_INT16(x))
#define LITTLE_ENDIANIZE_INT32(x)	(FLIPENDIAN_INT32(x))
#define LITTLE_ENDIANIZE_INT64(x)	(FLIPENDIAN_INT64(x))
#endif /* LSB_FIRST */


/* Useful macros to deal with bit shuffling encryptions */
#define BIT(x,n) (((x)>>(n))&1)

#define BITSWAP8(val,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B7) << 7) | \
		 (BIT(val,B6) << 6) | \
		 (BIT(val,B5) << 5) | \
		 (BIT(val,B4) << 4) | \
		 (BIT(val,B3) << 3) | \
		 (BIT(val,B2) << 2) | \
		 (BIT(val,B1) << 1) | \
		 (BIT(val,B0) << 0))

#define BITSWAP16(val,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))

#define BITSWAP24(val,B23,B22,B21,B20,B19,B18,B17,B16,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B23) << 23) | \
		 (BIT(val,B22) << 22) | \
		 (BIT(val,B21) << 21) | \
		 (BIT(val,B20) << 20) | \
		 (BIT(val,B19) << 19) | \
		 (BIT(val,B18) << 18) | \
		 (BIT(val,B17) << 17) | \
		 (BIT(val,B16) << 16) | \
		 (BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))

#define BITSWAP32(val,B31,B30,B29,B28,B27,B26,B25,B24,B23,B22,B21,B20,B19,B18,B17,B16,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B31) << 31) | \
		 (BIT(val,B30) << 30) | \
		 (BIT(val,B29) << 29) | \
		 (BIT(val,B28) << 28) | \
		 (BIT(val,B27) << 27) | \
		 (BIT(val,B26) << 26) | \
		 (BIT(val,B25) << 25) | \
		 (BIT(val,B24) << 24) | \
		 (BIT(val,B23) << 23) | \
		 (BIT(val,B22) << 22) | \
		 (BIT(val,B21) << 21) | \
		 (BIT(val,B20) << 20) | \
		 (BIT(val,B19) << 19) | \
		 (BIT(val,B18) << 18) | \
		 (BIT(val,B17) << 17) | \
		 (BIT(val,B16) << 16) | \
		 (BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))



/***************************************************************************

    Common functions

***************************************************************************/

/* since stricmp is not part of the standard, we use this instead */
int mame_stricmp(const char *s1, const char *s2);

/* this macro prevents people from using stricmp directly */
#undef stricmp
#define stricmp !MUST_USE_MAME_STRICMP_INSTEAD!


/* since strnicmp is not part of the standard, we use this instead */
int mame_strnicmp(const char *s1, const char *s2, size_t n);

/* this macro prevents people from using strnicmp directly */
#undef strnicmp
#define strnicmp !MUST_USE_MAME_STRNICMP_INSTEAD!


/* since strdup is not part of the standard, we use this instead */
char *mame_strdup(const char *str);

/* this macro prevents people from using strdup directly */
#undef strdup
#define strdup !MUST_USE_MAME_STRDUP_INSTEAD!


/* compute the intersection of two rectangles */
INLINE void sect_rect(rectangle *dst, const rectangle *src)
{
	if (src->min_x > dst->min_x) dst->min_x = src->min_x;
	if (src->max_x < dst->max_x) dst->max_x = src->max_x;
	if (src->min_y > dst->min_y) dst->min_y = src->min_y;
	if (src->max_y < dst->max_y) dst->max_y = src->max_y;
}


/* convert a series of 32 bits into a float */
INLINE float u2f(UINT32 v)
{
	union {
		float ff;
		UINT32 vv;
	} u;
	u.vv = v;
	return u.ff;
}


/* convert a float into a series of 32 bits */
INLINE UINT32 f2u(float f)
{
	union {
		float ff;
		UINT32 vv;
	} u;
	u.ff = f;
	return u.vv;
}


/* convert a series of 64 bits into a double */
INLINE double u2d(UINT64 v)
{
	union {
		double dd;
		UINT64 vv;
	} u;
	u.vv = v;
	return u.dd;
}


/* convert a double into a series of 64 bits */
INLINE UINT64 d2u(double d)
{
	union {
		double dd;
		UINT64 vv;
	} u;
	u.dd = d;
	return u.vv;
}



/***************************************************************************

    Inline math helpers

***************************************************************************/

/* If the OSD layer wants to override these in osd_cpu.h, they can by #defining */
/* the function name to itself or to point to something else */


/* return the number of leading zero bits in a 32-bt value */
#ifndef count_leading_zeros
INLINE UINT32 count_leading_zeros(UINT32 val)
{
	UINT32 count;
	for (count = 0; (INT32)val >= 0; count++) val <<= 1;
	return count;
}
#endif


/* return the number of leading one bits in a 32-bt value */
#ifndef count_leading_ones
INLINE UINT32 count_leading_ones(UINT32 val)
{
	UINT32 count;
	for (count = 0; (INT32)val < 0; count++) val <<= 1;
	return count;
}
#endif


/* perform a 32x32 multiply to 64-bit precision and then shift */
#ifndef fixed_mul_shift
INLINE INT32 fixed_mul_shift(INT32 val1, INT32 val2, UINT8 shift)
{
	return (INT32)(((INT64)val1 * (INT64)val2) >> shift);
}
#endif



/***************************************************************************

    Binary coded decimal

***************************************************************************/

INLINE int bcd_adjust(int value)
{
	if ((value & 0xf) >= 0xa)
		value = value + 0x10 - 0xa;
	if ((value & 0xf0) >= 0xa0)
		value = value - 0xa0 + 0x100;
	return value;
}


INLINE int dec_2_bcd(int a)
{
	return (a % 10) | ((a / 10) << 4);
}


INLINE int bcd_2_dec(int a)
{
	return (a & 0xf) + (a >> 4) * 10;
}



/***************************************************************************

    Gregorian calendar code

***************************************************************************/

INLINE int gregorian_is_leap_year(int year)
{
	return !(year % 100 ? year % 4 : year % 400);
}


/* months are one counted */
INLINE int gregorian_days_in_month(int month, int year)
{
	if (month == 2)
		return gregorian_is_leap_year(year) ? 29 : 28;
	else if (month == 4 || month == 6 || month == 9 || month == 11)
		return 30;
	else
		return 31;
}



/***************************************************************************

    Compiler-specific nastiness

***************************************************************************/

/* Suppress warnings about redefining the macro 'PPC' on LinuxPPC. */
#ifdef PPC
#undef PPC
#endif



/* Some optimizations/warnings cleanups for GCC */
#if defined(__GNUC__) && (__GNUC__ >= 3)
#define ATTR_UNUSED				__attribute__((__unused__))
#define ATTR_NORETURN			__attribute__((noreturn))
#define ATTR_PRINTF(x,y)		__attribute__((format(printf, x, y)))
#define ATTR_MALLOC				__attribute__((malloc))
#define ATTR_PURE				__attribute__((pure))
#define ATTR_CONST				__attribute__((const))
#define UNEXPECTED(exp)			__builtin_expect((exp), 0)
#define TYPES_COMPATIBLE(a,b)	__builtin_types_compatible_p(a, b)
#define RESTRICT				__restrict__
#else
#define ATTR_UNUSED
#define ATTR_NORETURN
#define ATTR_PRINTF(x,y)
#define ATTR_MALLOC
#define ATTR_PURE
#define ATTR_CONST
#define UNEXPECTED(exp)			(exp)
#define TYPES_COMPATIBLE(a,b)	1
#define RESTRICT
#endif



/* And some MSVC optimizations/warnings */
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#define DECL_NORETURN			__declspec(noreturn)
#else
#define DECL_NORETURN
#endif



/***************************************************************************

    Function prototypes

***************************************************************************/

/* Used by assert(), so definition here instead of mame.h */
DECL_NORETURN void CLIB_DECL fatalerror(const char *text,...) ATTR_PRINTF(1,2) ATTR_NORETURN;


#endif	/* __MAMECORE_H__ */
