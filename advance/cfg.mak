############################################################################
# CFG

CFGCFLAGS += \
	-Iadvance/$(HOST_SYSTEM) \
	-Iadvance/lib \
	-Iadvance/blit \
	-Iadvance/v \
	-Iadvance/common
CFGOBJDIRS = \
	$(CFGOBJ) \
	$(CFGOBJ)/cfg \
	$(CFGOBJ)/lib \
	$(CFGOBJ)/blit \
	$(CFGOBJ)/v \
	$(CFGOBJ)/$(HOST_SYSTEM)
CFGOBJS = \
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
	$(CFGOBJ)/lib/videoall.o \
	$(CFGOBJ)/blit/blit.o \
	$(CFGOBJ)/blit/clear.o \
	$(CFGOBJ)/cfg/cfg.o \
	$(CFGOBJ)/cfg/list.o \
	$(CFGOBJ)/v/draw.o

ifeq ($(HOST_SYSTEM),linux)
CFGCFLAGS += -DPREFIX=\"$(PREFIX)\"
CFGCFLAGS += \
	-DUSE_VIDEO_SVGALIB -DUSE_VIDEO_FB -DUSE_VIDEO_SLANG \
	-DUSE_INPUT_SVGALIB \
	-Iadvance/$(HOST_SYSTEM)
CFGLIBS = -lslang -lvga
CFGOBJS += \
	$(CFGOBJ)/lib/filenix.o \
	$(CFGOBJ)/lib/targnix.o \
	$(CFGOBJ)/$(HOST_SYSTEM)/os.o \
	$(CFGOBJ)/$(HOST_SYSTEM)/vsvgab.o \
	$(CFGOBJ)/$(HOST_SYSTEM)/vfb.o \
	$(CFGOBJ)/$(HOST_SYSTEM)/vslang.o
endif

ifeq ($(HOST_SYSTEM),dos)
CFGCFLAGS += -DPREFIX=\"$(PREFIX)\"
CFGCFLAGS += \
	-DUSE_VIDEO_SVGALINE -DUSE_VIDEO_VBELINE -DUSE_VIDEO_VGALINE \
	-Iadvance/$(HOST_SYSTEM) \
	-Iadvance/card \
	-Iadvance/svgalib \
	-Iadvance/svgalib/clockchi \
	-Iadvance/svgalib/ramdac \
	-Iadvance/svgalib/drivers
CFGLIBS = -lalleg
CFGOBJS += \
	$(CFGOBJ)/lib/filedos.o \
	$(CFGOBJ)/lib/targdos.o \
	$(CFGOBJ)/$(HOST_SYSTEM)/os.o \
	$(CFGOBJ)/$(HOST_SYSTEM)/vvgal.o \
	$(CFGOBJ)/$(HOST_SYSTEM)/vvbel.o \
	$(CFGOBJ)/$(HOST_SYSTEM)/vsvgal.o \
	$(CFGOBJ)/$(HOST_SYSTEM)/scrvbe.o \
	$(CFGOBJ)/$(HOST_SYSTEM)/scrvga.o \
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
CFGOBJDIRS += \
	$(CFGOBJ)/card \
	$(CFGOBJ)/svgalib \
	$(CFGOBJ)/svgalib/ramdac \
	$(CFGOBJ)/svgalib/clockchi \
	$(CFGOBJ)/svgalib/drivers
endif

$(CFGOBJ)/%.o: advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(CFGCFLAGS) -c $< -o $@

$(sort $(CFGOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(CFGOBJ)/advcfg$(EXE) : $(sort $(CFGOBJDIRS)) $(CFGOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(CFGLDFLAGS) $(CFGOBJS) $(CFGLIBS) -o $@
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advcfg$(EXE)
	$(LN) $@ advcfg$(EXE)
