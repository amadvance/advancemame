/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001 Andrea Mazzoleni
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

#include "lib.h"

/***************************************************************************/
/* Standard */

#ifdef LIB_TOUPPER
char _pascal toupper(char c)
{
	if (c>='a' && c<='z')
		return c - 'a' + 'A';
	else
		return c;
}
#endif

#ifdef LIB_TOLOWER
char _pascal tolower(char c)
{
	if (c>='A' && c<='Z')
		return c - 'A' + 'a';
	else
		return c;
}
#endif

#ifdef LIB_ABS
int abs(int v)
{
	if (v<0)
		return -v;
	else
		return v;
}
#endif

#ifdef LIB_LABS
long labs(long v)
{
	if (v<0)
		return -v;
	else
		return v;
}
#endif

#ifdef LIB_MEMCPY
void _pascal memcpy(void far * dst, const void far * src, unsigned count)
{
	asm push ds
	asm cld
	asm les di, [dst]
	asm lds si, [src]
	asm mov cx, [count]
	asm rep movsb
	asm pop ds
}
#endif

#ifdef LIB_MEMSET
void _pascal memset(void far * dst, unsigned src, unsigned count)
{
	asm push ds
	asm cld
	asm les di, [dst]
	asm mov al, byte ptr [src]
	asm mov cx, [count]
	asm rep stosb
	asm pop ds
}
#endif

#ifdef LIB_MEMCMP
int _pascal memcmp(const void far * s1, const void far * s2, unsigned count)
{
	unsigned i;
	for(i=0;i<count;++i) {
		if ((((unsigned char far *)s1)[i])<(((unsigned char far *)s2)[i])) return -1;
		if ((((unsigned char far *)s1)[i])>(((unsigned char far *)s2)[i])) return 1;
	}
	return 0;
}
#endif

#ifdef LIB_STRCMP
int _pascal strcmp(const char far * s1, const char far * s2)
{
	unsigned char c1, c2;
	c1 = *s1;
	c2 = *s2;
	while (c1 && c1==c2) {
		++s1;
		++s2;
		c1 = *s1;
		c2 = *s2;
	}
	if (c1<c2) return -1;
	if (c1>c2) return 1;
	return 0;
}
#endif

#ifdef LIB_STRCASECMP
int _pascal strcasecmp(const char far * s1, const char far * s2)
{
	unsigned char c1, c2;
	c1 = toupper(*s1);
	c2 = toupper(*s2);
	while (c1 && c1==c2) {
		++s1;
		++s2;
		c1 = toupper(*s1);
		c2 = toupper(*s2);
	}
	if (c1<c2) return -1;
	if (c1>c2) return 1;
	return 0;
}
#endif

#ifdef LIB_OPTIONMATCH
int optionmatch(const char far* arg, const char far* opt)
{
	return (arg[0] == '-' || arg[0] == '/') && strcasecmp(arg+1, opt) == 0;
}
#endif

#ifdef LIB_STRNCPY
void _pascal strncpy(char far *s1, const char far *s2, unsigned maxlen)
{
	unsigned i = 0;
	while (i<maxlen && s2[i]) {
		s1[i] = s2[i];
		++i;
	}
	if (i<maxlen)
		s1[i] = 0;
}
#endif

#ifdef LIB_STRLEN
unsigned _pascal strlen(const char far * s1)
{
	unsigned i = 0;
	while (s1[i]) {
		++i;
	}
	return i;
}
#endif

#ifdef LIB_STRCPY
void _pascal strcpy(char far* s1, const char far* s2)
{
	unsigned i = 0;
	while (s2[i]) {
		s1[i] = s2[i];
		++i;
	}
	s1[i] = 0;
}
#endif

#ifdef LIB_STRCHR
const char far* _pascal strchr(const char far* str, int c)
{
	while (*str) {
		if (*str == c)
			return str;
		++str;
	}
	return 0;
}
#endif

#ifdef LIB_STRTOK
static char far* strtok_ptr = 0;

char far* _pascal strtok(char far* str, const char far* sep)
{
	char far* start;

	if (str)
		strtok_ptr = str;

	/* skip separator */
	while (*strtok_ptr && strchr(sep, *strtok_ptr)!=0)
		++strtok_ptr;

	/* check for the end */
	if (!*strtok_ptr)
		return 0;

	/* start */
	start = strtok_ptr;

	/* scan */
	while (*strtok_ptr && strchr(sep, *strtok_ptr)==0)
		++strtok_ptr;

	if (*strtok_ptr) {
		*strtok_ptr = 0;
		++strtok_ptr;
	}

	return start;
}
#endif

#ifdef LIB_STRTOL
long _pascal strtol(const char far* s, unsigned radix)
{
	long v = 0;
	int sign = 0;
	if (*s=='-') {
		++s;
		sign = 1;
	}
	while (*s>='0' && *s<='9' || *s>='a' && *s<='f' || *s>='A' && *s<='F') {
		v *= radix;
		if (*s>='0' && *s<='9')
			v += *s - '0';
		else if (*s>='a' && *s<='f')
			v += *s - 'a' + 10;
		else
			v += *s - 'A' + 10;
		++s;
	}
	if (sign)
		v = -v;
	return v;
}
#endif

#ifdef LIB_STRTOU
unsigned _pascal strtou(const char far* s, unsigned radix)
{
	unsigned v = 0;
	while (*s>='0' && *s<='9' || *s>='a' && *s<='f' || *s>='A' && *s<='F') {
		v *= radix;
		if (*s>='0' && *s<='9')
			v += *s - '0';
		else if (*s>='a' && *s<='f')
			v += *s - 'a' + 10;
		else
			v += *s - 'A' + 10;
		++s;
	}
	return v;
}
#endif

#ifdef LIB_STRTOLD
long _pascal strtold(const char far* s, long multiplier)
{
	long v = 0;
	int sign = 0;
	if (*s=='-') {
		++s;
		sign = 1;
	}
	while (*s>='0' && *s<='9') {
		v *= 10;
		v += *s - '0';
		++s;
	}
	v *= multiplier;
	if (*s=='.') {
		++s;
		while (*s>='0' && *s<='9') {
			multiplier /= 10;
			v += (*s - '0')*multiplier;
			++s;
		}
	}
	if (sign)
		v = -v;
	return v;
}
#endif

#ifdef LIB_STRTOD
double _pascal strtod(const char far* s)
{
	double v = 0;
	int sign = 0;
	if (*s=='-') {
		++s;
		sign = 1;
	}
	while (*s>='0' && *s<='9') {
		v *= 10;
		v += *s - '0';
		++s;
	}
	if (*s=='.') {
		double b = 1;
		++s;
		while (*s>='0' && *s<='9') {
			b /= 10;
			v += (*s - '0')*b;
			++s;
		}
	}
	if (sign)
		v = -v;
	return v;
}
#endif

/***************************************************************************/
/* Hardware */

#ifdef LIB_SEM
// Enter a critical section
// return:
//   ==0 no, enter
//   ==1 ok, enter
int _pascal sem_enter(uint16 far* psem)
{
	asm les bx, [psem]
	asm mov ax, 1
	asm xchg ax, [es:bx]
	asm cmp ax, 0
	asm je ok
	return 0;
ok:
	return 1;
}

// leave from a critical section
void _pascal sem_leave(uint16 far* psem)
{
	*psem = 0;
}
#endif

#ifdef LIB_REGION
// Disable interrupt and enter a critical section
// return:
//   ==0 no, NOT enter
//   !=0 ok, enter
int _pascal region_enter(struct region far* pregion)
{
	uint16 flags = disable();
	if (sem_enter(&pregion->sem)) {
		pregion->flags = flags;
		return 1;
	} else {
		restore(flags);
		return 0;
	}
}

void _pascal region_leave(struct region far* pregion)
{
	uint16 flags = pregion->flags;
	sem_leave(&pregion->sem);
	restore(flags);
}
#endif

#ifdef LIB_DISABLE
// Disable interrupt
// return:
//   flags to be passed to enable
uint16 _pascal disable()
{
	asm pushf
	asm pop ax
	asm cli
	return _AX;
}
#endif

#ifdef LIB_ENABLE
// Abilita interrupt
// return:
//   flags to be passed to enable
uint16 _pascal enable()
{
	asm pushf
	asm pop ax
	asm sti
	return _AX;
}
#endif

#ifdef LIB_OUTPORTB
void _pascal outportb(int port, uint8 value)
{
	asm mov al, [value]
	asm mov dx, [port]
	asm out dx, al
}
#endif

#ifdef LIB_INPORTB
uint8 _pascal inportb(int port)
{
	uint8 value;
	asm mov dx, [port]
	asm in al, dx
	asm mov [value], al
	return value;
}
#endif

#ifdef LIB_OUTPORTW
void _pascal outportw(int port, uint16 value)
{
	asm mov ax, [value]
	asm mov dx, [port]
	asm out dx, ax
}
#endif

#ifdef LIB_INPORTW
uint16 _pascal inportw(int port)
{
	uint16 value;
	asm mov dx, [port]
	asm in ax, dx
	asm mov [value], ax
	return value;
}
#endif

#ifdef LIB_OUTPORTL
void _pascal outportl(int port, uint32 value)
{
	asm mov eax, [value]
	asm mov dx, [port]
	asm out dx, eax
}
#endif

#ifdef LIB_INPORTL
uint32 _pascal inportl(int port)
{
	uint32 value;
	asm mov dx, [port]
	asm in eax, dx
	asm mov [value], eax
	return value;
}
#endif

/***************************************************************************/
/* Timer */

#ifdef LIB_DELAY
static unsigned near readtimer()
{
	asm cli
	asm mov  al, 0h
	asm out  43h, al
	asm nop
	asm in   al, 40h
	asm mov  bl, al
	asm nop
	asm in   al, 40h
	asm mov  bh, al
	asm not  bx
	asm sti
	return _BX;
}

#define DELAY_MULTIPLIER 1193UL

void _pascal delay( unsigned milliseconds )
{
	unsigned long stop;
	unsigned cur, prev;

	stop = (prev = readtimer()) + (milliseconds * DELAY_MULTIPLIER);

	while ((cur = readtimer()) < stop) {
		if (cur < prev) {
			if (stop < 0x10000L)
				break;
			stop -= 0x10000L;
		}
		prev = cur;
	}
}
#endif


/***************************************************************************/
/* Sound */

#ifdef LIB_SOUND
/* Outputs a sound of a specified frequency to the speaker */
void _pascal sound(unsigned frequency)
{
	asm     mov     bx,  frequency
	asm     mov     ax,  34DDh
	asm     mov     dx,  0012h
	asm     cmp     dx,  bx
	asm     jnb     stop
	asm     div     bx
	asm     mov     bx,  ax
	asm     in      al,  61h
	asm     test    al,  3
	asm     jne     j1
	asm     or      al,  3
	asm     out     61h, al
	asm     mov     al,  0B6h
	asm     out     43h, al
	j1:
	asm     mov     al,  bl
	asm     out     42h, al
	asm     mov     al,  bh
	asm     out     42h, al
	stop: ;
}
#endif

#ifdef LIB_NOSOUND
/* Turns the speaker off */
void _pascal nosound()
{
	asm     in      al, 61H
	asm     and     al, 0fcH
	asm     out     61H, al
}
#endif

/***************************************************************************/
/* BIOS */

#ifdef LIB_CPUTC
void _pascal cputc(unsigned char c)
{
	asm mov ah, 0Eh
	asm mov al, [c]
	asm mov bx, 7
	asm int 10h
}
#endif

#ifdef LIB_CPUTS
void _pascal cputs(const char far * s)
{
	while (*s) cputc(*s++);
}
#endif

#ifdef LIB_CPUTUL
void _pascal cputul(unsigned long n, unsigned w, char pad, unsigned base)
{
	char s[32];
	unsigned l = 0;
	while (n) {
		unsigned d = (unsigned)(n % base);
		s[l++] = d < 10 ? '0'+d : 'a'+d-10;
		n /= base;
	}
	if (!l) s[l++] = '0';
	while (l<w) s[l++] = pad;
	while (l) cputc(s[--l]);
}
#endif

#ifdef LIB_CPUTU
void _pascal cputu(unsigned n, unsigned w, char pad, unsigned base)
{
	char s[32];
	unsigned l = 0;
	while (n) {
		unsigned d = (unsigned)(n % base);
		s[l++] = d < 10 ? '0'+d : 'a'+d-10;
		n /= base;
	}
	if (!l) s[l++] = '0';
	while (l<w) s[l++] = pad;
	while (l) cputc(s[--l]);
}
#endif

#ifdef LIB_CPUTI
void _pascal cputi(int n, unsigned w, char pad, unsigned base)
{
	char s[32];
	unsigned l = 0;
	int sign = n<0;
	if (sign) n = -n;
	while (n) {
		unsigned d = n % base;
		s[l++] = d < 10 ? '0'+d : 'a'+d-10;
		n /= base;
	}
	if (!l) s[l++] = '0';
	if (sign) s[l++] = '-';
	while (l<w) s[l++] = pad;
	while (l) cputc(s[--l]);
}
#endif

#ifdef LIB_WHEREX
unsigned _pascal wherex()
{
	asm mov ah, 3h
	asm mov bh, 0
	asm int 10h
	asm mov dh, 0
	asm mov ax, dx
	return _AX;
}
#endif

#ifdef LIB_WHEREY
unsigned _pascal wherey()
{
	asm mov ah, 3h
	asm mov bh, 0
	asm int 10h
	asm shr dx, 8
	asm mov ax, dx
	return _AX;
}
#endif

#ifdef LIB_GOTOXY
void _pascal gotoxy(unsigned x, unsigned y)
{
	asm mov ah, 2h
	asm mov bh, 0
	asm mov dl, [x]
	asm mov dh, [y]
	asm int 10h
}
#endif

/***************************************************************************/
/* DOS */

#ifdef LIB_ERRNO
uint16 errno;
#endif

#ifdef LIB_PSPSEG
uint16 _pascal pspseg()
{
	asm mov ah, 62h
	asm int 21h
	asm mov ax, bx
	return _AX;
}
#endif

/*
Format of environment block:
Offset  Size  Description (Table 0604)
	00h  N BYTEs first environment variable, ASCIZ string of form "var=value"
		N BYTEs second environment variable, ASCIZ string
	...
		N BYTEs last environment variable, ASCIZ string of form "var=value"
	BYTE  00h
---DOS 3.0+ ---
	uint16  number of strings following environment (normally 1)
		N BYTEs ASCIZ full pathname of program owning this environment
		other strings may follow
*/

#ifdef LIB_ENVSEG
// Return the environment segment
// return:
//   On old DOS can be 0
uint16 _pascal envseg()
{
	return *((uint16 far *)MK_FP(pspseg(), 0x2C));
}
#endif

#ifdef LIB_ENV
const char far* _pascal env()
{
	uint16 seg = envseg();
	if (seg)
		return (const char far *)MK_FP(seg, 0);
	else
		return 0;
}
#endif

#ifdef LIB_FREEENV
/* Release the environment segment */
int _pascal freeenv()
{
	return freemem( envseg() );
}
#endif

#ifdef LIB_ENVSIZE
// Compute the environment size
// return:
//   0 if invalid
unsigned _pascal envsize()
{
	unsigned i;
	const char far * e = env();
	if (!e) return 0;
	i = 0;
	while (i<=32767 && e[i]!=0) {
		while (i<=32767 && e[i]!=0) {
			++i;
		}
		++i;
	}
	++i;
	if (i>32767) return 0; // environment is limited to 32K
	return i;
}
#endif

#ifdef LIB_EXEC_PATHNAME
/* Full pathname of program owning this environment */
const char far* _pascal exec_pathname()
{
	unsigned size = envsize();
	if (!size) return 0;
	const char far * e = env();
	return e+size+2;
}
#endif

#ifdef LIB_FREEMEM
int _pascal freemem(uint16 memseg)
{
	asm mov ax, [memseg]
	asm mov es, ax
	asm mov ah, 49h
	asm int 21h
	asm jc error
	return 0;
error:
	asm mov [errno], ax
	return -1;
}
#endif

#ifdef LIB_ALLOCMEM
int _pascal allocmem(unsigned size, uint16 far* segp)
{
	asm mov ah, 48h
	asm mov bx, size
	asm int 21h
	asm jc error
	asm les bx, [segp]
	asm mov [es:bx], ax
	return 0;
error:
	asm mov [errno], ax
	return -1;
}
#endif

#ifdef LIB_PUTC
/* dos put char */
void _pascal putc(char c)
{
	asm mov ah, 0x02
	asm mov dl, c
	asm int 0x21
}
#endif

#ifdef LIB_PUTS
/* dos put string */
void _pascal puts(const char * s)
{
	while (*s) putc(*(s++));
}
#endif

/***************************************************************************/
/* File */

#ifdef LIB_CREAT
int _pascal creat(const char far* file, unsigned attrib)
{
	asm mov ah, 03Ch
	asm mov cx, [attrib]
	asm push ds
	asm lds dx, [file]
	asm int 21h
	asm pop ds
	asm jc error
	return _AX;
error:
	asm mov [errno], ax
	return -1;
}
#endif

#ifdef LIB_OPEN
int _pascal open(const char far* file, unsigned access)
{
	asm mov ah, 3Dh
	asm mov al, [byte ptr access]
	asm mov cl, 0
	asm push ds
	asm lds dx, [file]
	asm int 21h
	asm pop ds
	asm jc error
	return _AX;
error:
	asm mov [errno], ax
	return -1;
}
#endif

#ifdef LIB_CLOSE
int _pascal close(int handle)
{
	asm mov ah, 03Eh
	asm mov bx, [handle]
	asm int 21h
	asm jc error
	return 0;
error:
	asm mov [errno], ax
	return -1;
}
#endif

#ifdef LIB_READ
int _pascal read(int handle, void far * buf, int len)
{
	asm mov ah, 3Fh
	asm mov bx, [handle]
	asm mov cx, [len]
	asm push ds
	asm lds dx, [buf]
	asm int 21h
	asm pop ds
	asm jc error
	return _AX;
error:
	asm mov [errno], ax
	return -1;
}
#endif

#ifdef LIB_FGETC
int _pascal fgetc(int handle)
{
	char c;
	if (read(handle, &c, 1)<=0)
		return EOF;
	else
		return c;
}
#endif

#ifdef LIB_GETS
char far* _pascal gets(int handle, char far * buf, int len)
{
	char far * p = buf;
	if (!len)
		return 0;
	while (len>1) {
		int c = fgetc(handle);
		if (c==EOF) {
			if (p==buf)
				return 0;
			else
				break;
		}
		if (c=='\n')
			break;
		if (c!='\r') {
			*p = c;
			++p;
			--len;
		}
	}
	*p = 0;
	return buf;
}
#endif

#ifdef LIB_WRITE
int _pascal write(int handle, const void far * buf, int len)
{
	asm mov ah, 40h
	asm mov bx, [handle]
	asm mov cx, [len]
	asm push ds
	asm lds dx, [buf]
	asm int 21h
	asm pop ds
	asm jc error
	return _AX;
error:
	asm mov [errno], ax
	return -1;
}
#endif

#ifdef LIB_LSEEK
long _pascal lseek(int handle, long off, int fromwhere)
{
	asm mov ah, 42h
	asm mov al, [byte ptr fromwhere]
	asm mov bx, [word ptr handle]
	asm mov dx, [word ptr off]
	asm mov cx, [word ptr off+2]
	asm int 21h
	asm jc error
	return (long)((void _seg *)(_DX) + (void near *)(_AX));
error:
	asm mov [errno], ax
	return -1;
}
#endif

#ifdef LIB_FILELENGTH
long _pascal filelength(int handle)
{
	long pos = lseek(handle, 0, SEEK_CUR);
	if (pos>=0) {
		long end = lseek(handle, 0, SEEK_END);
		if (end>=0)
			if (lseek(handle, pos, SEEK_SET)>=0)
				return end;
	}
	return -1;
}
#endif

#ifdef LIB_GETFTIME
int _pascal getftime(int handle, ftime far* ftimep)
{
	asm mov ax, 5700h
	asm mov bx, [handle]
	asm int 21h
	asm jc error
	asm les bx, [ftimep]
	asm mov [es:bx], cx
	asm mov [es:bx+2], dx
	return 0;
error:
	asm mov [errno], ax
	return -1;
}
#endif

#ifdef LIB_SETFTIME
int _pascal setftime(int handle, const ftime far* ftimep)
{
	asm mov ax, 5701h
	asm les bx, [ftimep]
	asm mov cx, [es:bx]
	asm mov dx, [es:bx+2]
	asm mov bx, [handle]
	asm int 21h
	asm jc error
	return 0;
error:
	asm mov [errno], ax
	return -1;
}
#endif

#ifdef LIB_GEFATTR
int _pascal getfattr(const char far *path, unsigned far *attribp)
{
	asm mov ax, 4300h
	asm push ds
	asm lds dx, [path]
	asm int 21h
	asm pop ds
	asm jc error
	asm les bx, [attribp]
	asm mov [es:bx], cx
	return 0;
error:
	asm mov [errno], ax
	return -1;
}
#endif

#ifdef LIB_SETFATTR
int _pascal setfattr(const char far *path, unsigned attrib)
{
	asm mov ax, 4301h
	asm mov cx, [attrib]
	asm push ds
	asm lds dx, [path]
	asm int 21h
	asm pop ds
	asm jc error
	return 0;
error:
	asm mov [errno], ax
	return -1;
}
#endif

/***************************************************************************/
/* Date/Time */

#ifdef LIB_JUL2DATE
// Converte un numero giuliano in data
void _pascal jul2date(long jul, date far* datep)
{
	long day, month, year, tmp;

	tmp = jul-1721119L;
	year = (4*tmp-1)/146097L;
	tmp = 4*tmp-1-146097L*year;
	day = tmp/4;
	tmp = (4*day+3)/1461;
	day = 4*day+3-1461*tmp;
	day = (day+4)/4;
	month = (5*day-3)/153;
	day = 5*day-3-153*month;
	day = (day+5)/5;
	year = 100*year+tmp;
	datep->da_day = (int)day;
	datep->da_year = (int)year;

	if ((int)month < 10)
		datep->da_mon = (int)month+3;
	else {
		datep->da_mon = (int)month-9;
		datep->da_year++;
	}
}
#endif

#ifdef LIB_DATE2JUL
// Converte una data (giorno, mese, anno) in numero giuliano
// in:
//   day giorno della data da convertire (1-31)
//   month mese della data da convertire (1-12).
//   year anno della data da convertire (19xx|2xxx)
long _pascal date2jul(const date far* datep)
{
	long cent, day, month, year;

	if (datep->da_mon > 2) {
		month = (long)(datep->da_mon-3);
		year = (long)datep->da_year;
	} else {
		month = (long)(datep->da_mon+9);
		year = (long)(datep->da_year-1);
	}

	cent = (year/100);
	day = year-(100*cent);

	return ((146097L*cent)/4L+(1461L*day)/4L+(153L*month+2)/5L+1721119L+(long)datep->da_day);
}
#endif

#ifdef LIB_GETTIME
void _pascal gettime(time far *timep)
{
	asm mov ah, 2Ch
	asm int 21h
	asm les bx, [timep]
	asm mov [byte ptr es:bx], cl
	asm mov [byte ptr es:bx+1], ch
	asm mov [byte ptr es:bx+2], dl
	asm mov [byte ptr es:bx+3], dh
}
#endif

#ifdef LIB_GETDATE
void _pascal getdate(date far *datep)
{
	asm mov ah, 2Ah
	asm int 21h
	asm les bx, [datep]
	asm mov [word ptr es:bx], cx
	asm mov [byte ptr es:bx+2], dl
	asm mov [byte ptr es:bx+3], dh
}
#endif

/***************************************************************************/
/* Int */

#ifdef LIB_IRQ_ADDR_CALL
/* Call an IRQ */
void _pascal irq_addr_call(void far * addr)
{
	asm pushf
	asm call [dword ptr addr]
}
#endif

#ifdef LIB_GETVECT
void far* _pascal getvect(int intr)
{
	asm mov ah, 035h
	asm mov al, [byte ptr intr]
	asm int 21h
	asm mov ax, bx
	asm mov dx, es
	return MK_FP(_DX, _AX);
}
#endif

#ifdef LIB_SETVECT
void _pascal setvect(int intr, void far * func)
{
	asm mov ah, 025h
	asm mov al, [byte ptr intr]
	asm push ds
	asm lds dx, dword ptr func
	asm int 21h
	asm pop ds
}
#endif

#ifdef LIB_REGS_32_PRESET
void _pascal regs_32_preset(regs_32 far* regs)
{
	regs->d.es = _DS;
}
#endif

#ifdef LIB_REGS_32_ZERO
void _pascal regs_32_zero(regs_32 far* regs)
{
	memset(regs, 0, sizeof(regs_32));
	regs_32_preset(regs);
}
#endif

#ifdef LIB_INT_32_ADDR_CALL
static uint32 int_addr;

void _pascal int_32_addr_call(void far* addr, regs_32 far* regs)
{
	/* asm push bp */ /* saved automatically */
	asm push esi
	asm push edi
	asm push ds
	asm push fs

	asm mov eax, [dword ptr addr]
	asm mov [dword ptr int_addr], eax

	asm lfs bp, [regs]
	asm mov ax, [fs:bp+6*4+2]
	asm push ax
	asm popf
	asm mov eax, [fs:bp+0*4]
	asm mov ebx, [fs:bp+1*4]
	asm mov ecx, [fs:bp+2*4]
	asm mov edx, [fs:bp+3*4]
	asm mov esi, [fs:bp+4*4]
	asm mov edi, [fs:bp+5*4]
	asm mov es, [fs:bp+6*4]

	asm push fs
	asm push bp
	asm pushf
	asm call [dword ptr int_addr]
	asm pop bp
	asm pop fs

	asm mov [fs:bp+0*4], eax
	asm mov [fs:bp+1*4], ebx
	asm mov [fs:bp+2*4], ecx
	asm mov [fs:bp+3*4], edx
	asm mov [fs:bp+4*4], esi
	asm mov [fs:bp+5*4], edi
	asm mov [fs:bp+6*4], es
	asm pushf
	asm pop ax
	asm mov [fs:bp+6*4+2], ax

	asm pop fs
	asm pop ds
	asm pop edi
	asm pop esi
	/* asm pop bp */
}
#endif

#ifdef LIB_INT_32_CALL
void _pascal int_32_call(unsigned num, regs_32 far* regs)
{
	int_32_addr_call(getvect(num), regs);
}
#endif
