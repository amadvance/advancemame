#############################################################################
# Configuration
#

srcdir = ..
VBEOBJ = obj
DEBUG = yes
ECHO = @echo
MD = @mkdir
#CC = @i586-pc-msdosdjgpp-gcc
#LD = @i586-pc-msdosdjgpp-gcc
CC = @gcc
LD = @gcc
EXE = .exe
STUBEDIT = @stubedit

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

all: vbew$(EXE)

VBEOBJDIRS += \
	$(VBEOBJ) \
	$(VBEOBJ)/vbew \
	$(VBEOBJ)/dos \
	$(VBEOBJ)/svgalib \
	$(VBEOBJ)/svgalib/svgavdd \
	$(VBEOBJ)/svgalib/clockchi \
	$(VBEOBJ)/svgalib/ramdac \
	$(VBEOBJ)/svgalib/drivers
VBECFLAGS += \
	-DUSE_ADV_SVGALIB_DOSK \
	-I$(srcdir)/dos \
	-I$(srcdir)/svgalib \
	-I$(srcdir)/svgalib/svgavdd \
	-I$(srcdir)/svgalib/clockchi \
	-I$(srcdir)/svgalib/ramdac \
	-I$(srcdir)/svgalib/drivers
VBEOBJS += \
	$(VBEOBJ)/vbew/stub.o \
	$(VBEOBJ)/vbew/vbe.o \
	$(VBEOBJ)/vbew/raw.o \
	$(VBEOBJ)/vbew/keep.o \
	$(VBEOBJ)/dos/pci.o \
	$(VBEOBJ)/dos/map.o \
	$(VBEOBJ)/svgalib/svgalib.o \
	$(VBEOBJ)/svgalib/svgavdd/svgavdd.o \
	$(VBEOBJ)/svgalib/accel.o \
	$(VBEOBJ)/svgalib/vgaio.o \
	$(VBEOBJ)/svgalib/vgammvga.o \
	$(VBEOBJ)/svgalib/vgaregs.o \
	$(VBEOBJ)/svgalib/vgarelvg.o \
	$(VBEOBJ)/svgalib/drivers/apm.o \
	$(VBEOBJ)/svgalib/drivers/ark.o \
	$(VBEOBJ)/svgalib/drivers/banshee.o \
	$(VBEOBJ)/svgalib/drivers/et6000.o \
	$(VBEOBJ)/svgalib/drivers/g400.o \
	$(VBEOBJ)/svgalib/drivers/pm2.o \
	$(VBEOBJ)/svgalib/drivers/i740.o \
	$(VBEOBJ)/svgalib/drivers/laguna.o \
	$(VBEOBJ)/svgalib/drivers/millenni.o \
	$(VBEOBJ)/svgalib/drivers/mx.o \
	$(VBEOBJ)/svgalib/drivers/nv3.o \
	$(VBEOBJ)/svgalib/drivers/r128.o \
	$(VBEOBJ)/svgalib/drivers/rage.o \
	$(VBEOBJ)/svgalib/drivers/s3.o \
	$(VBEOBJ)/svgalib/drivers/savage.o \
	$(VBEOBJ)/svgalib/drivers/sis.o \
	$(VBEOBJ)/svgalib/drivers/trident.o \
	$(VBEOBJ)/svgalib/drivers/renditio.o \
	$(VBEOBJ)/svgalib/ramdac/ibmrgb52.o \
	$(VBEOBJ)/svgalib/ramdac/attdacs.o \
	$(VBEOBJ)/svgalib/ramdac/icw.o \
	$(VBEOBJ)/svgalib/ramdac/normal.o \
	$(VBEOBJ)/svgalib/ramdac/ramdac.o \
	$(VBEOBJ)/svgalib/ramdac/s3dacs.o \
	$(VBEOBJ)/svgalib/ramdac/sierra.o \
	$(VBEOBJ)/svgalib/ramdac/btdacs.o \
	$(VBEOBJ)/svgalib/ramdac/ics_gend.o \
	$(VBEOBJ)/svgalib/clockchi/icd2061a.o

$(VBEOBJ)/%.o: $(srcdir)/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(VBECFLAGS) -c $< -o $@

$(sort $(VBEOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

vbew$(EXE) : $(sort $(VBEOBJDIRS)) $(VBEOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(VBELDFLAGS) $(VBEOBJS) $(VBELIBS) -o $@
	$(STUBEDIT) $@ bufsize=4K

