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

#ifndef __LIB_H
#define __LIB_H

#include "types.h"

int abs(int v);
long labs(long v);

void _pascal memcpy(void far * dst, const void far * scr, unsigned count);
void _pascal memset(void far * dst, unsigned scr, unsigned count);
int _pascal memcmp(const void far * s1, const void far * s2, unsigned count);
int _pascal strcmp(const char far * s1, const char far * s2);
int _pascal strcasecmp(const char far * s1, const char far * s2);
char _pascal toupper(char c);
char _pascal tolower(char c);
int optionmatch(const char far* arg, const char far* opt);

unsigned _pascal strlen(const char far * s1);
void _pascal strcpy(char far* s1, const char far* s2);
void _pascal strncpy(char far *s1, const char far *s2, unsigned maxlen);
const char far* _pascal strchr(const char far* str, int c);
char far* _pascal strtok(char far* str, const char far* sep);
unsigned _pascal strtou(const char far* s, unsigned radix);
long _pascal strtol(const char far* s, unsigned radix);
long _pascal strtold(const char far* s, long multiplier);
double _pascal strtod(const char far* s);

uint16 _pascal disable(void);
uint16 _pascal enable(void);
void _pascal restore(uint16 flags);

typedef uint16 sem;

int _pascal sem_enter(sem far* psem);
void _pascal sem_leave(sem far* psem);

struct region {
	uint16 flags;
	sem sem;
};

int _pascal region_enter(struct region far* pregion);
void _pascal region_leave(struct region far* pregion);

#define MK_FP( seg, ofs ) ((void _seg * )((uint16)(seg)) + (void near *)((uint16)(ofs)))
#define FP_SEG( fp ) ((uint16)(void _seg *)(void far *)(fp))
#define FP_OFF( fp ) ((uint16)(fp))

void _pascal delay(unsigned milliseconds);
void _pascal sound(unsigned frequency);
void _pascal nosound(void);

void _pascal outportb(int port, uint8 value);
uint8 _pascal inportb(int port);
void _pascal outportw(int port, uint16 value);
uint16 _pascal inportw(int port);
void _pascal outportl(int port, uint32 value);
uint32 _pascal inportl(int port);

#ifdef __cplusplus
extern "C"
#endif
int far setjmp(void);

#ifdef __cplusplus
extern "C"
#endif
void far longjmp(void);

#define fCF             0x0001
#define fPF             0x0004
#define fAF             0x0010
#define fZF             0x0040
#define fSF             0x0080
#define fIF             0x0200
#define fDF             0x0400
#define fOF             0x0800
#define fSTD            (fCF | fPF | fZF | fSF | fOF)

struct regs_32_d {
	uint32 eax;
	uint32 ebx;
	uint32 ecx;
	uint32 edx;
	uint32 esi;
	uint32 edi;
	uint16 es;
	uint16 flags;
};

struct regs_32_x {
	uint16 ax;
	uint16 axh;
	uint16 bx;
	uint16 bxh;
	uint16 cx;
	uint16 cxh;
	uint16 dx;
	uint16 dxh;
	uint16 si;
	uint16 sih;
	uint16 di;
	uint16 dih;
	uint16 es;
	uint16 flags;
};

struct regs_32_h {
	uint8 al;
	uint8 ah;
	uint8 ahl;
	uint8 ahh;
	uint8 bl;
	uint8 bh;
	uint8 bhl;
	uint8 bhh;
	uint8 cl;
	uint8 ch;
	uint8 chl;
	uint8 chh;
	uint8 dl;
	uint8 dh;
	uint8 dhl;
	uint8 dhh;
	uint32 esi;
	uint32 edi;
	uint16 es;
	uint16 flags;
};

typedef union {
	struct regs_32_d d;
	struct regs_32_x x;
	struct regs_32_h h;
} regs_32;

void _pascal regs_32_preset(regs_32 far* regs);
void _pascal regs_32_zero(regs_32 far* regs);
void _pascal irq_addr_call(void far * addr);
void _pascal int_32_addr_call(void far* addr, regs_32 far* regs);
void _pascal int_32_call(unsigned num, regs_32 far* regs);

void _pascal cputc(unsigned char c);
void _pascal cputs(const char far * s);
void _pascal cputi(int n, unsigned w, char pad, unsigned base);
void _pascal cputu(unsigned n, unsigned w, char pad, unsigned base);
void _pascal cputul(unsigned long n, unsigned w, char pad, unsigned base);

#define CLK_TCK 18.196105
#define clock() (*(volatile uint32 far*)MK_FP( 0x40, 0x6C ))

unsigned _pascal wherex(void);
unsigned _pascal wherey(void);
void _pascal gotoxy(unsigned x, unsigned y);

uint16 _pascal pspseg(void);

const char far* _pascal env(void);
uint16 _pascal envseg(void);
int _pascal freeenv(void);
unsigned _pascal envsize(void);
const char far* _pascal exec_pathname(void);

int _pascal freemem(uint16 memseg);
int _pascal allocmem(unsigned size, uint16 far* segp);

void _pascal putc(char c);
void _pascal puts(const char * s);

extern uint16 errno;

#define FA_NORMAL   0x00
#define FA_RDONLY   0x01
#define FA_HIDDEN   0x02
#define FA_SYSTEM   0x04
#define FA_ARCH     0x20

#define O_RDONLY         0x00
#define O_WRONLY         0x01
#define O_RDWR           0x02

#define O_NOINHERIT 0x80
#define O_DENYALL   0x10
#define O_DENYWRITE 0x20
#define O_DENYREAD  0x30
#define O_DENYNONE  0x40

int _pascal open(const char far * file, unsigned access);
int _pascal creat(const char far * file, unsigned attrib);
int _pascal close(int handle);
int _pascal read(int handle, void far * buf, int len);
int _pascal write(int handle, const void far * buf, int len);
char far* _pascal gets(int handle, char far * buf, int len);

#define EOF -1
int _pascal fgetc(int handle);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

long _pascal lseek(int handle, long offset, int fromwhere);
long _pascal filelength(int handle);

struct ftime {
	unsigned ft_tsec: 5;   // (* Two seconds *)
	unsigned ft_min: 6;    // (* Minutes *)
	unsigned ft_hour: 5;   // (* Hours *)
	unsigned ft_day: 5;    // (* Days *)
	unsigned ft_month: 4;  // (* Months *)
	unsigned ft_year: 7;   // (* Year - 1980 *)
};

int _pascal getftime(int handle, struct ftime far* ftimep);
int _pascal setftime(int handle, const struct ftime far* ftimep);

int _pascal getfattr(const char far *path, unsigned far *attribp);
int _pascal setfattr(const char far *path, unsigned attrib);

struct time {
	uint8 ti_min;
	uint8 ti_hour;
	uint8 ti_hund;
	uint8 ti_sec;
};

void _pascal gettime(struct time far *timep);

struct date {
	uint16 da_year;
	uint8 da_day;
	uint8 da_mon;
	};

void _pascal getdate(struct date far *datep);
long _pascal date2jul(const struct date far* datep);
void _pascal jul2date(long jul, struct date far* datep);

void far * _pascal getvect(int intr);
void _pascal setvect(int intr, void far * func);

#define EXIT_TSR 0
#define EXIT_SUCCESS 1
#define EXIT_FAILURE 2

#endif