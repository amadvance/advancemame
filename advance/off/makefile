#
# This file is part of the AdvanceMAME project
#
# Copyright (C) 1999, 2000, 2001 Andrea Mazzoleni
#

# Directories
TSR = ..\tsr

# Macro
CC = bcc
CFLAGS = -c -mt -O1 -3 -B -f- -I. -I$(TSR)
# -c Compile only
# -mt Model TINY (CS==SS==DS)
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
ASMFLAGS =

all: off.com

{$(TSR)}.asm.obj:
	$(ASM) $(ASMFLAGS) $<

.c.obj:
	$(CC) $(CFLAGS) $<

off.com: c0t.obj off.obj deflib.obj
	$(LD) $(LDFLAGS) @&&|
c0t+
deflib+
off
off.com
|

clean:
	rm -f *.obj *.bak *.map *.tds
	rm -f deflib.asm off.asm

distclean: clean
	rm -f off.com
