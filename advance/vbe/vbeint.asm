;
;  This file is part of the Advance project.
;
;  Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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

ideal
P386
model tiny

; **************************************************************************
; int 10h
;

codeseg

public C int_10_old

int_10_old DD 0

extrn C int_10_trap:proc

public C int_10

proc int_10
	pushf

	cmp ax,4F00h
	jb skip_10
	cmp ax,4F0Fh
	ja skip_10
	jmp short trap_10

skip_10:
	popf
	jmp [dword ptr cs:int_10_old]

trap_10:
	popf
	push ds
	push ebp

	; regs_32
	pushf
	push es edi esi edx ecx ebx eax

	; imposta ds
	mov ax,cs           ; cs == ds != ss
	mov ds,ax

	; immette l'indirizzo far ai registri
	mov bp,sp
	push ss
	push bp

	call int_10_trap

	add sp,4

	; regs_32
	pop eax ebx ecx edx esi edi es
	popf

	pop ebp
	pop ds
	iret
endp

; **************************************************************************
; int 2Fh
;

codeseg

public C int_2f_old

int_2f_old DD 0

public C int_2f_id

int_2f_id DB 0

extrn C int_2f_trap:proc

public C int_2f

align 2

proc int_2f
	pushf

	cmp ah,[cs:int_2f_id]
	je trap_2f

	popf
	jmp [dword ptr cs:int_2f_old]

trap_2f:
	popf
	push ds
	push ebp

	; regs_32
	pushf
	push es edi esi edx ecx ebx eax

	; imposta ds
	mov ax,cs           ; cs == ds != ss
	mov ds,ax

	; immette l'indirizzo far ai registri
	mov bp,sp
	push ss
	push bp

	call int_2f_trap

	add sp,4

	; regs_32
	pop eax ebx ecx edx esi edi es
	popf

	pop ebp
	pop ds
	iret
endp

end
