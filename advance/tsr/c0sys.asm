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
; EXE/SYS file startup module, cs == ds == ss
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

public C tsr_sys

tsr_sys dw 0

codeseg

extrn C main : proc

org 0

; the following header is required in all device drivers
header		dd 0FFFFFFFFh
attribute	dw 8000h ; character device
		dw offset strategy ; offset to stratgey routine
		dw offset interrupt ; offset to interrupt routine
		db 'VGA$    ' ; driver name 8 bytes padded
request_header	dd ? ; address of DOS request header

; The strategy routine just stores the address of the request header.
; When DOS calls the device driver it first calls the strategy routine
proc strategy far
	mov [word ptr cs:request_header],bx
	mov [word ptr cs:request_header+2],es
	ret
endp

; DOS calls the interrupt routine after the strategy routine returns
proc interrupt far
	; save the state
	pushf
	push ax
	push bx
	push cx
	push dx
	push ds
	push es
	push si
	push di
	push bp

	; set ds
	mov ax,cs
	mov ds,ax

	; check for initialize command
	les bx,[request_header]
	cmp [byte ptr es:bx+2],0 ; request_header->command
	je command_init
	jmp command_error

command_init:
	; set sys flag
	mov [tsr_sys],1

	; fill bss segment
	mov cx,offset DGROUP:bss_end
	mov di,offset DGROUP:bss_start
	sub cx,di
	mov ax,ds
	mov es,ax
	xor al,al
	cld
	rep stosb

	; set the old stack
	mov [oldss],ss ; save stack segment
	mov [oldsp],sp ; save stack pointer

	; set the stack
	mov ax,cs
	mov bx,offset DGROUP:stack_end - 2
	mov ss,ax
	mov sp,bx

	; push command line far pointer
	les bx,[request_header]
	les di,[es:bx+12h]
	; skip the device name
arg_skip:
	cmp [byte ptr es:di],' '
	je arg_skip_done
	cmp [byte ptr es:di],0dh
	je arg_skip_done
	inc di
	jmp short arg_skip
arg_skip_done:
	push es
	push di
	; push command line length
	mov cx,127
	mov al,0dh
	repne scasb
	neg cx
	add cx,126
	push cx

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
	je init_done

init_error:
	les bx,[request_header]
	mov ax,0
	mov [es:bx+14],ax ; request_header->end_code
	mov ax,cs
	mov [es:bx+16],ax ; request_header->segment
	mov ax,0
	mov [attribute],ax ; clear attribute
	mov ax,810Ch ; error
	mov [es:bx+3],ax ; request_header->status
	jmp short finish

init_done:
	les bx,[request_header]
	mov ax,offset DGROUP:bss_end
	mov [word ptr es:bx+14],ax ; request_header->end_code
	mov ax,cs
	mov [word ptr es:bx+16],ax ; request_header->segment
	mov ax,0100h ; done
	mov [word ptr es:bx+3],ax ; request_header->status
	jmp short finish

command_error:
	les bx,[request_header]
	mov ax,8003h ; error (not supported)
	mov [word ptr es:bx+3],ax ; request_header->status
	jmp short finish

finish:
	; restore state
	pop bp
	pop di
	pop si
	pop es
	pop ds
	pop dx
	pop cx
	pop bx
	pop ax
	popf

	; and return to DOS, this is not a real interrupt
	ret
endp

start:
	; set ds
	mov ax,cs
	mov ds,ax

	; save the psp dx
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
