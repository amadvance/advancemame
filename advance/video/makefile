#
# This file is part of the AdvanceMAME project
#
# Copyright (C) 1999, 2000, 2001 Andrea Mazzoleni
#

# Directories
TSR = ..\tsr

# Macro
CC = bcc
CFLAGS = -c -mt -O1 -I. -I.. -I$(TSR)
# -c Compile only
# -mt Model TINY (CS==SS==DS)
# -O1 Optimize for small code
LD = tlink
LDFLAGS = /s /m
# /m Map file
# /s Detailed Map file
ASM = tasm
ASMFLAGS =

all: video.exe

{$(TSR)}.asm.obj:
	$(ASM) $(ASMFLAGS) $<

.c.obj:
	$(CC) $(CFLAGS) $<

video.exe: c0sys.obj video.obj deflib.obj
	$(LD) $(LDFLAGS) @&&|
c0sys+
deflib+
video
video.exe
|

clean:
	rm -f *.obj *.bak *.map *.tds
	rm -f deflib.asm video.asm

distclean:
	rm -f video.exe
