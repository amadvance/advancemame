#
# This file is part of the AdvanceMAME project
#
# Copyright (C) 1999, 2000, 2001, 2002 Andrea Mazzoleni
#

# Directories
TSR = ..\tsr

# Macro
CC = bcc
CFLAGS = -c -mt -O1 -I. -I$(TSR)
# -c Compile only
# -mt Model TINY (CS==SS==DS)
# -O1 Optimize for small code
LD = tlink
LDFLAGS = /s /m
# /m Map file
# /s Detailed Map file
ASM = tasm
ASMFLAGS =

all: portio.exe

{$(TSR)}.asm.obj:
	$(ASM) $(ASMFLAGS) $<

.c.obj:
	$(CC) $(CFLAGS) $<

portio.exe: c0sys.obj portio.obj deflib.obj
	$(LD) $(LDFLAGS) @&&|
c0sys+
deflib+
portio
portio.exe
|

clean:
	rm -f *.obj *.bak *.map *.tds
	rm -f deflib.asm portio.asm

distclean:
	rm -f portio.exe
