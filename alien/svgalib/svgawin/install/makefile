#############################################################################
# Configuration
#

DEBUG = no
ECHO = @echo
CC = @i386-mingw32msvc-gcc
#CC = @gcc
EXE = .exe

#############################################################################
# Rules
#

ifeq ($(DEBUG),yes)
CFLAGS = -O0 -Wall -g
LDFLAGS = -g
MSG = "(debug)"
else
CFLAGS = -O1 -Wall -march=i386
LDFLAGS = -s
MSG =
endif

all: svgawin$(EXE)

svgawin$(EXE) : svgawin.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

