#
# This file is part of the AdvanceMAME project
#
# Copyright (C) 1999, 2000, 2001 Andrea Mazzoleni
#

# Directories
CARD = ..\card
TSR = ..\tsr
DOS = ..\dos

# Macro
CC = bcc
CFLAGS = -c -mt! -O1 -3 -B -f- -I. -I$(CARD) -I$(TSR) -I$(DOS) \
	-D__PCIREAL__ -DNDEBUG
# -c Compile only
# -mt! Model TINY (CS==DS!=SS)
# -O1 Optimize for small code
# -B Compile via assembler
# -f- No floating point
# -3 386
LD = tlink
LDFLAGS = /s /m /t
# /t COM file
# /m Map file
# /s Detailed Map file
ASM = tasm
ASMFLAGS = -I. -I$(TSR)

all: vbe.com

{$(TSR)}.c.obj:
	$(CC) $(CFLAGS) $<

{$(DOS)}.c.obj:
	$(CC) $(CFLAGS) $<

{$(TSR)}.asm.obj:
	$(ASM) $(ASMFLAGS) $<

{$(CARD)}.c.obj:
	$(CC) $(CFLAGS) $<

.asm.obj:
	$(ASM) $(ASMFLAGS) $<

.c.obj:
	$(CC) $(CFLAGS) $<

vbe.com: c0t.obj cclibbc.obj deflib.obj kernel.obj vbeint.obj compil.obj \
	pci.obj card.obj cs3.obj ccirrus.obj csis.obj csavage.obj ctdfx.obj \
	cvbe3.obj cneomagic.obj ctrident.obj vbe.obj
	$(LD) $(LDFLAGS) @&&|
c0t+
cclibbc+
deflib+
kernel+
vbeint+
compil+
card+
pci+
cs3+
ccirrus+
csis+
csavage+
ctdfx+
cvbe3+
cneomagic+
ctrident+
vbe
vbe.com
|


clean:
	rm -f *.obj *.bak *.map *.tds
	rm -f kernel.asm deflib.asm card.asm advvbe.asm compil.asm vbe.asm pci.asm
	rm -f cs3.asm claguna.asm csavage.asm cmatrox.asm ccirrus.asm ctdfx.asm cvbe3.asm
	rm -f csis.asm cr128.asm cneomagic.asm ctrident.asm

distclean:
	rm -f vbe.com
