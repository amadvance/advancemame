#############################################################################
# User configuration
#

srcdir = ..
VIDEOOBJ = obj
DEBUG = no
ECHO = @echo
MD = @mkdir -p
CC = @/usr/local/cross-tools/bin/i386-mingw32msvc-gcc
LD = @/usr/local/cross-tools/bin/i386-mingw32msvc-gcc
#CC = @gcc
#LD = @gcc
EXE = .exe
VIDEOLIBS = -lgdi32

#############################################################################
#
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

all: videowin$(EXE)

VIDEOOBJDIRS += \
	$(VIDEOOBJ) \
	$(VIDEOOBJ)/videowin \
	$(VIDEOOBJ)/svgalib \
	$(VIDEOOBJ)/svgalib/svgawin \
	$(VIDEOOBJ)/svgalib/clockchi \
	$(VIDEOOBJ)/svgalib/ramdac \
	$(VIDEOOBJ)/svgalib/drivers
VIDEOFLAGS += \
	-I$(srcdir)/svgalib \
	-I$(srcdir)/svgalib/svgawin \
	-I$(srcdir)/svgalib/clockchi \
	-I$(srcdir)/svgalib/ramdac \
	-I$(srcdir)/svgalib/drivers
VIDEOOBJS += \
	$(VIDEOOBJ)/videowin/videowin.o \
	$(VIDEOOBJ)/svgalib/svgalib.o \
	$(VIDEOOBJ)/svgalib/svgawin/svgawin.o \
	$(VIDEOOBJ)/svgalib/accel.o \
	$(VIDEOOBJ)/svgalib/vgaio.o \
	$(VIDEOOBJ)/svgalib/vgammvga.o \
	$(VIDEOOBJ)/svgalib/vgaregs.o \
	$(VIDEOOBJ)/svgalib/vgarelvg.o \
	$(VIDEOOBJ)/svgalib/drivers/apm.o \
	$(VIDEOOBJ)/svgalib/drivers/ark.o \
	$(VIDEOOBJ)/svgalib/drivers/banshee.o \
	$(VIDEOOBJ)/svgalib/drivers/et6000.o \
	$(VIDEOOBJ)/svgalib/drivers/g400.o \
	$(VIDEOOBJ)/svgalib/drivers/pm2.o \
	$(VIDEOOBJ)/svgalib/drivers/i740.o \
	$(VIDEOOBJ)/svgalib/drivers/laguna.o \
	$(VIDEOOBJ)/svgalib/drivers/millenni.o \
	$(VIDEOOBJ)/svgalib/drivers/mx.o \
	$(VIDEOOBJ)/svgalib/drivers/nv3.o \
	$(VIDEOOBJ)/svgalib/drivers/r128.o \
	$(VIDEOOBJ)/svgalib/drivers/rage.o \
	$(VIDEOOBJ)/svgalib/drivers/s3.o \
	$(VIDEOOBJ)/svgalib/drivers/savage.o \
	$(VIDEOOBJ)/svgalib/drivers/sis.o \
	$(VIDEOOBJ)/svgalib/drivers/trident.o \
	$(VIDEOOBJ)/svgalib/drivers/renditio.o \
	$(VIDEOOBJ)/svgalib/ramdac/ibmrgb52.o \
	$(VIDEOOBJ)/svgalib/ramdac/attdacs.o \
	$(VIDEOOBJ)/svgalib/ramdac/icw.o \
	$(VIDEOOBJ)/svgalib/ramdac/normal.o \
	$(VIDEOOBJ)/svgalib/ramdac/ramdac.o \
	$(VIDEOOBJ)/svgalib/ramdac/s3dacs.o \
	$(VIDEOOBJ)/svgalib/ramdac/sierra.o \
	$(VIDEOOBJ)/svgalib/ramdac/btdacs.o \
	$(VIDEOOBJ)/svgalib/ramdac/ics_gend.o \
	$(VIDEOOBJ)/svgalib/clockchi/icd2061a.o

$(VIDEOOBJ)/%.o: $(srcdir)/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(VIDEOFLAGS) -c $< -o $@

$(sort $(VIDEOOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

videowin$(EXE) : $(sort $(VIDEOOBJDIRS)) $(VIDEOOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(VIDEOLDFLAGS) $(VIDEOOBJS) $(VIDEOLIBS) -o $@

