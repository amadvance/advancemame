############################################################################
# CFG

CFGCFLAGS += \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/blit \
	-I$(srcdir)/advance/v \
	-I$(srcdir)/advance/common
CFGOBJDIRS += \
	$(CFGOBJ) \
	$(CFGOBJ)/cfg \
	$(CFGOBJ)/lib \
	$(CFGOBJ)/blit \
	$(CFGOBJ)/v
CFGOBJS += \
	$(CFGOBJ)/lib/log.o \
	$(CFGOBJ)/lib/video.o \
	$(CFGOBJ)/lib/conf.o \
	$(CFGOBJ)/lib/incstr.o \
	$(CFGOBJ)/lib/videoio.o \
	$(CFGOBJ)/lib/update.o \
	$(CFGOBJ)/lib/generate.o \
	$(CFGOBJ)/lib/crtc.o \
	$(CFGOBJ)/lib/crtcbag.o \
	$(CFGOBJ)/lib/monitor.o \
	$(CFGOBJ)/lib/gtf.o \
	$(CFGOBJ)/lib/device.o \
	$(CFGOBJ)/lib/inputall.o \
	$(CFGOBJ)/lib/inputdrv.o \
	$(CFGOBJ)/lib/videoall.o \
	$(CFGOBJ)/lib/error.o \
	$(CFGOBJ)/blit/blit.o \
	$(CFGOBJ)/blit/clear.o \
	$(CFGOBJ)/cfg/cfg.o \
	$(CFGOBJ)/cfg/list.o \
	$(CFGOBJ)/v/draw.o

ifeq ($(CONF_HOST),unix)
CFGCFLAGS += \
	-DPREFIX=\"$(PREFIX)\" \
	-I$(srcdir)/advance/linux
CFGOBJDIRS += \
	$(CFGOBJ)/linux
CFGOBJS += \
	$(CFGOBJ)/lib/filenix.o \
	$(CFGOBJ)/lib/targnix.o \
	$(CFGOBJ)/linux/os.o
ifeq ($(CONF_LIB_SLANG),yes)
CFGCFLAGS += \
	-DUSE_VIDEO_SLANG \
	-DUSE_INPUT_SLANG
CFGLIBS += -lslang
CFGOBJS += \
	$(CFGOBJ)/linux/vslang.o \
	$(CFGOBJ)/linux/islang.o
endif
ifeq ($(CONF_LIB_SVGALIB),yes)
CFGCFLAGS += \
	-DUSE_VIDEO_SVGALIB
CFGLIBS += -lvga
CFGOBJS += \
	$(CFGOBJ)/linux/vsvgab.o
endif
ifeq ($(CONF_LIB_FB),yes)
CFGOBJS += \
	$(CFGOBJ)/linux/vfb.o
endif
endif

ifeq ($(CONF_HOST),dos)
CFGCFLAGS += \
	-I$(srcdir)/advance/dos \
	-I$(srcdir)/advance/card \
	-I$(srcdir)/advance/svgalib \
	-I$(srcdir)/advance/svgalib/clockchi \
	-I$(srcdir)/advance/svgalib/ramdac \
	-I$(srcdir)/advance/svgalib/drivers \
	-DUSE_VIDEO_SVGALINE -DUSE_VIDEO_VBELINE -DUSE_VIDEO_VGALINE \
	-DUSE_INPUT_DOS
CFGLIBS += -lalleg
CFGOBJDIRS += \
	$(CFGOBJ)/dos \
	$(CFGOBJ)/card \
	$(CFGOBJ)/svgalib \
	$(CFGOBJ)/svgalib/ramdac \
	$(CFGOBJ)/svgalib/clockchi \
	$(CFGOBJ)/svgalib/drivers
CFGOBJS += \
	$(CFGOBJ)/lib/filedos.o \
	$(CFGOBJ)/lib/targdos.o \
	$(CFGOBJ)/dos/os.o \
	$(CFGOBJ)/dos/vvgal.o \
	$(CFGOBJ)/dos/vvbel.o \
	$(CFGOBJ)/dos/vsvgal.o \
	$(CFGOBJ)/dos/scrvbe.o \
	$(CFGOBJ)/dos/scrvga.o \
	$(CFGOBJ)/dos/idos.o \
	$(CFGOBJ)/card/card.o \
	$(CFGOBJ)/card/pci.o \
	$(CFGOBJ)/card/map.o \
	$(CFGOBJ)/card/board.o \
	$(CFGOBJ)/svgalib/libdos.o \
	$(CFGOBJ)/svgalib/accel.o \
	$(CFGOBJ)/svgalib/vgaio.o \
	$(CFGOBJ)/svgalib/vgammvga.o \
	$(CFGOBJ)/svgalib/vgaregs.o \
	$(CFGOBJ)/svgalib/vgarelvg.o \
	$(CFGOBJ)/svgalib/drivers/apm.o \
	$(CFGOBJ)/svgalib/drivers/ark.o \
	$(CFGOBJ)/svgalib/drivers/banshee.o \
	$(CFGOBJ)/svgalib/drivers/et6000.o \
	$(CFGOBJ)/svgalib/drivers/g400.o \
	$(CFGOBJ)/svgalib/drivers/pm2.o \
	$(CFGOBJ)/svgalib/drivers/i740.o \
	$(CFGOBJ)/svgalib/drivers/i810.o \
	$(CFGOBJ)/svgalib/drivers/laguna.o \
	$(CFGOBJ)/svgalib/drivers/millenni.o \
	$(CFGOBJ)/svgalib/drivers/mx.o \
	$(CFGOBJ)/svgalib/drivers/nv3.o \
	$(CFGOBJ)/svgalib/drivers/r128.o \
	$(CFGOBJ)/svgalib/drivers/rage.o \
	$(CFGOBJ)/svgalib/drivers/s3.o \
	$(CFGOBJ)/svgalib/drivers/savage.o \
	$(CFGOBJ)/svgalib/drivers/sis.o \
	$(CFGOBJ)/svgalib/drivers/trident.o \
	$(CFGOBJ)/svgalib/drivers/renditio.o \
	$(CFGOBJ)/svgalib/ramdac/ibmrgb52.o \
	$(CFGOBJ)/svgalib/ramdac/attdacs.o \
	$(CFGOBJ)/svgalib/ramdac/icw.o \
	$(CFGOBJ)/svgalib/ramdac/normal.o \
	$(CFGOBJ)/svgalib/ramdac/ramdac.o \
	$(CFGOBJ)/svgalib/ramdac/s3dacs.o \
	$(CFGOBJ)/svgalib/ramdac/sierra.o \
	$(CFGOBJ)/svgalib/ramdac/btdacs.o \
	$(CFGOBJ)/svgalib/ramdac/ics_gend.o \
	$(CFGOBJ)/svgalib/clockchi/icd2061a.o
endif

$(CFGOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(CFGCFLAGS) -c $< -o $@

$(sort $(CFGOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(CFGOBJ)/advcfg$(EXE) : $(sort $(CFGOBJDIRS)) $(CFGOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(CFGLDFLAGS) $(CFGOBJS) $(CFGLIBS) -o $@
ifeq ($(CONF_COMPRESS),yes)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advcfg$(EXE)
	$(LN_S) $@ advcfg$(EXE)
