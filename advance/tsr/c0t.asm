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

;
; COM file startup module, cs == ds == ss
;

ideal
model tiny

; Segments

segment _TEXT word public 'CODE'
ends

segment _DATA word public 'DATA'
ends

segment _BSS  word public 'BSS'
label bss_start
ends

segment _STACK word public 'STACK'
label bss_end
stack_start db 2048 dup (?)
label stack_end
ends

dataseg

oldss dw ? ; old stack segment
oldsp dw ? ; old stack pointer

codeseg

extrn C main : proc

org 100h

start:
	; set ds
	mov ax,cs
	mov ds,ax

	; save the psp in dx
	mov dx,es

	; fill the bss segment
	mov cx,offset DGROUP:bss_end
	mov di,offset DGROUP:bss_start
	sub cx,di
	mov ax,ds
	mov es,ax
	xor al,al
	cld
	rep stosb

	; restore the psp in es
	mov es,dx

	; set the old stack
	mov [oldss],ss ; save stack segment
	mov [oldsp],sp ; save stack pointer

	; set the stack
	mov ax,cs
	mov bx,offset DGROUP:stack_end - 2
	mov ss,ax
	mov sp,bx

	; push command line far pointer
	push es
	mov bx,81h
	push bx
	; push command line length
	mov bx,80h
	mov al,[es:bx]
	mov ah,0
	push ax

	; call main
	call main
	add sp,6

	; set ds
	mov dx,cs
	mov ds,dx

	; reset the stack
	mov ss,[oldss]
	mov sp,[oldsp]

	; 00==TSR
	cmp ax,0
	jne terminate

	; terminate and stay resident
	mov dx,offset DGROUP:bss_end
	add dx,15
	shr dx,4

	mov ax,3100h
	int 21h

terminate:
	; errorlevel
	dec al

	; terminate
	mov ah,4ch
	int 21h

end start
