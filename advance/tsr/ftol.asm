;
;  This file is part of the Advance project.
;
;  Copyright (C) 1999, 2000, 2001 Andrea Mazzoleni
;
;  This program is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 2 of the License, or
;  (at your option) any later version.
;
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details. 
;
;  You should have received a copy of the GNU General Public License
;  along with this program; if not, write to the Free Software
;  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;

temp2 equ [bp-8]

_TEXT segment public byte 'CODE'
	assume  cs:_TEXT
	public  N_FTOL@

N_FTOL@:
	push bp
	mov bp, sp
	sub sp, 8

	fistp   qword ptr temp2 ; convert to 64-bit integer

	mov ax, [word ptr temp2] ; return LS 32 bits
	mov dx, [word ptr temp2+2]

	mov sp, bp
	pop bp
	ret
_TEXT ends

