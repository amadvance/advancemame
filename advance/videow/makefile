#############################################################################
# Configuration
#

srcdir = ..
VIDEOWOBJ = obj
DEBUG = no
ECHO = @echo
MD = @mkdir -p
CC = @i386-mingw32msvc-gcc
LD = @i386-mingw32msvc-gcc
#CC = @gcc
#LD = @gcc
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

all: videow$(EXE)

VIDEOWOBJDIRS += \
	$(VIDEOWOBJ) \
	$(VIDEOWOBJ)/videow \
	$(VIDEOWOBJ)/svgalib \
	$(VIDEOWOBJ)/svgalib/svgawin \
	$(VIDEOWOBJ)/svgalib/clockchi \
	$(VIDEOWOBJ)/svgalib/ramdac \
	$(VIDEOWOBJ)/svgalib/drivers
VIDEOWFLAGS += \
	-DUSE_ADV_SVGALIB_WIN \
	-I$(srcdir)/svgalib \
	-I$(srcdir)/svgalib/svgawin \
	-I$(srcdir)/svgalib/clockchi \
	-I$(srcdir)/svgalib/ramdac \
	-I$(srcdir)/svgalib/drivers
VIDEOWOBJS += \
	$(VIDEOWOBJ)/videow/videow.o \
	$(VIDEOWOBJ)/svgalib/svgalib.o \
	$(VIDEOWOBJ)/svgalib/svgawin/svgawin.o \
	$(VIDEOWOBJ)/svgalib/accel.o \
	$(VIDEOWOBJ)/svgalib/vgaio.o \
	$(VIDEOWOBJ)/svgalib/vgammvga.o \
	$(VIDEOWOBJ)/svgalib/vgaregs.o \
	$(VIDEOWOBJ)/svgalib/vgarelvg.o \
	$(VIDEOWOBJ)/svgalib/drivers/apm.o \
	$(VIDEOWOBJ)/svgalib/drivers/ark.o \
	$(VIDEOWOBJ)/svgalib/drivers/banshee.o \
	$(VIDEOWOBJ)/svgalib/drivers/et6000.o \
	$(VIDEOWOBJ)/svgalib/drivers/g400.o \
	$(VIDEOWOBJ)/svgalib/drivers/pm2.o \
	$(VIDEOWOBJ)/svgalib/drivers/laguna.o \
	$(VIDEOWOBJ)/svgalib/drivers/millenni.o \
	$(VIDEOWOBJ)/svgalib/drivers/mx.o \
	$(VIDEOWOBJ)/svgalib/drivers/nv3.o \
	$(VIDEOWOBJ)/svgalib/drivers/nv319.o \
	$(VIDEOWOBJ)/svgalib/drivers/r128.o \
	$(VIDEOWOBJ)/svgalib/drivers/rage.o \
	$(VIDEOWOBJ)/svgalib/drivers/s3.o \
	$(VIDEOWOBJ)/svgalib/drivers/savage.o \
	$(VIDEOWOBJ)/svgalib/drivers/savage18.o \
	$(VIDEOWOBJ)/svgalib/drivers/unichrom.o \
	$(VIDEOWOBJ)/svgalib/drivers/sis.o \
	$(VIDEOWOBJ)/svgalib/drivers/trident.o \
	$(VIDEOWOBJ)/svgalib/drivers/renditio.o \
	$(VIDEOWOBJ)/svgalib/ramdac/ibmrgb52.o \
	$(VIDEOWOBJ)/svgalib/ramdac/attdacs.o \
	$(VIDEOWOBJ)/svgalib/ramdac/icw.o \
	$(VIDEOWOBJ)/svgalib/ramdac/normal.o \
	$(VIDEOWOBJ)/svgalib/ramdac/ramdac.o \
	$(VIDEOWOBJ)/svgalib/ramdac/s3dacs.o \
	$(VIDEOWOBJ)/svgalib/ramdac/sierra.o \
	$(VIDEOWOBJ)/svgalib/ramdac/btdacs.o \
	$(VIDEOWOBJ)/svgalib/ramdac/ics_gend.o \
	$(VIDEOWOBJ)/svgalib/clockchi/icd2061a.o

$(VIDEOWOBJ)/%.o: $(srcdir)/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(VIDEOWFLAGS) -c $< -o $@

$(sort $(VIDEOWOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

videow$(EXE) : $(sort $(VIDEOWOBJDIRS)) $(VIDEOWOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(VIDEOWLDFLAGS) $(VIDEOWOBJS) $(VIDEOWLIBS) -o $@

