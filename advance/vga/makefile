#
# This file is part of the AdvanceMAME project
#
# Copyright (C) 1999, 2000, 2001 Andrea Mazzoleni
#

# Directories
CARD = ..\card
TSR = ..\tsr

# Macro
CC = bcc
CFLAGS = -c -mt! -O1 -B -f- -3 -I. -I$(CARD) -I$(TSR) \
	-D__PCIREAL__ -D__CARDNOEXT__ -D__RUN__ -DNDEBUG
# -c Compile only
# -mt! Model TINY (CS==DS!=SS)
# -O1 Optimize for small code
# -B Compile via assembler
# -f- No floating point
# -3 386
LD = tlink
LDFLAGS = /s /m
# /m Map file
# /s Detailed Map file
ASM = tasm
ASMFLAGS = -I. -I$(TSR)

all: vga.exe

{$(TSR)}.asm.obj:
	$(ASM) $(ASMFLAGS) $<

{$(TSR)}.c.obj:
	$(CC) $(CFLAGS) $<

{$(CARD)}.c.obj:
	$(CC) $(CFLAGS) $<

.asm.obj:
	$(ASM) $(ASMFLAGS) $<

.c.obj:
	$(CC) $(CFLAGS) $<

vga.exe: c0sys.obj cclibbc.obj deflib.obj kernel.obj vgaint.obj card.obj vga.obj
	$(LD) $(LDFLAGS) @&&|
c0sys+
cclibbc+
deflib+
kernel+
vgaint+
card+
vga
vga.exe
|

clean:
	rm -f *.obj *.bak *.map *.tds
	rm -f kernel.asm deflib.asm card.asm compil.asm vga.asm

distclean:
	rm -f vga.exe
