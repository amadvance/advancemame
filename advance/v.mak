############################################################################
# V

VCFLAGS += \
	-Iadvance/$(CONF_SYSTEM) \
	-Iadvance/lib \
	-Iadvance/blit \
	-Iadvance/common
VOBJS = \
	$(VOBJ)/lib/log.o \
	$(VOBJ)/lib/video.o \
	$(VOBJ)/lib/conf.o \
	$(VOBJ)/lib/incstr.o \
	$(VOBJ)/lib/videoio.o \
	$(VOBJ)/lib/update.o \
	$(VOBJ)/lib/generate.o \
	$(VOBJ)/lib/crtc.o \
	$(VOBJ)/lib/crtcbag.o \
	$(VOBJ)/lib/monitor.o \
	$(VOBJ)/lib/device.o \
	$(VOBJ)/lib/gtf.o \
	$(VOBJ)/lib/videoall.o \
	$(VOBJ)/blit/blit.o \
	$(VOBJ)/blit/clear.o \
	$(VOBJ)/v/v.o \
	$(VOBJ)/v/draw.o
VOBJDIRS = \
	$(VOBJ) \
	$(VOBJ)/v \
	$(VOBJ)/lib \
	$(VOBJ)/blit \
	$(VOBJ)/$(CONF_SYSTEM)

ifeq ($(CONF_SYSTEM),linux)
VCFLAGS += -DPREFIX=\"$(PREFIX)\"
VCFLAGS += \
	-DUSE_VIDEO_SVGALIB -DUSE_VIDEO_FB -DUSE_VIDEO_SLANG \
	-DUSE_INPUT_SVGALIB
VLIBS = -lslang -lvga
VOBJS += \
	$(VOBJ)/lib/filenix.o \
	$(VOBJ)/lib/targnix.o \
	$(VOBJ)/$(CONF_SYSTEM)/os.o \
	$(VOBJ)/$(CONF_SYSTEM)/vsvgab.o \
	$(VOBJ)/$(CONF_SYSTEM)/vfb.o \
	$(VOBJ)/$(CONF_SYSTEM)/vslang.o
endif

ifeq ($(CONF_SYSTEM),dos)
VCFLAGS += \
	-DUSE_VIDEO_SVGALINE -DUSE_VIDEO_VBELINE -DUSE_VIDEO_VGALINE -DUSE_VIDEO_VBE \
	-Iadvance/card \
	-Iadvance/svgalib \
	-Iadvance/svgalib/clockchi \
	-Iadvance/svgalib/ramdac \
	-Iadvance/svgalib/drivers
VLIBS = -lalleg
VOBJS += \
	$(VOBJ)/lib/filedos.o \
	$(VOBJ)/lib/targdos.o \
	$(VOBJ)/$(CONF_SYSTEM)/os.o \
	$(VOBJ)/$(CONF_SYSTEM)/vvgal.o \
	$(VOBJ)/$(CONF_SYSTEM)/vvbe.o \
	$(VOBJ)/$(CONF_SYSTEM)/vvbel.o \
	$(VOBJ)/$(CONF_SYSTEM)/vsvgal.o \
	$(VOBJ)/$(CONF_SYSTEM)/scrvbe.o \
	$(VOBJ)/$(CONF_SYSTEM)/scrvga.o \
	$(VOBJ)/card/card.o \
	$(VOBJ)/card/pci.o \
	$(VOBJ)/card/map.o \
	$(VOBJ)/card/board.o \
	$(VOBJ)/svgalib/libdos.o \
	$(VOBJ)/svgalib/accel.o \
	$(VOBJ)/svgalib/vgaio.o \
	$(VOBJ)/svgalib/vgammvga.o \
	$(VOBJ)/svgalib/vgaregs.o \
	$(VOBJ)/svgalib/vgarelvg.o \
	$(VOBJ)/svgalib/drivers/apm.o \
	$(VOBJ)/svgalib/drivers/ark.o \
	$(VOBJ)/svgalib/drivers/banshee.o \
	$(VOBJ)/svgalib/drivers/et6000.o \
	$(VOBJ)/svgalib/drivers/g400.o \
	$(VOBJ)/svgalib/drivers/pm2.o \
	$(VOBJ)/svgalib/drivers/i740.o \
	$(VOBJ)/svgalib/drivers/i810.o \
	$(VOBJ)/svgalib/drivers/laguna.o \
	$(VOBJ)/svgalib/drivers/millenni.o \
	$(VOBJ)/svgalib/drivers/mx.o \
	$(VOBJ)/svgalib/drivers/nv3.o \
	$(VOBJ)/svgalib/drivers/r128.o \
	$(VOBJ)/svgalib/drivers/rage.o \
	$(VOBJ)/svgalib/drivers/s3.o \
	$(VOBJ)/svgalib/drivers/savage.o \
	$(VOBJ)/svgalib/drivers/sis.o \
	$(VOBJ)/svgalib/drivers/trident.o \
	$(VOBJ)/svgalib/drivers/renditio.o \
	$(VOBJ)/svgalib/ramdac/ibmrgb52.o \
	$(VOBJ)/svgalib/ramdac/attdacs.o \
	$(VOBJ)/svgalib/ramdac/icw.o \
	$(VOBJ)/svgalib/ramdac/normal.o \
	$(VOBJ)/svgalib/ramdac/ramdac.o \
	$(VOBJ)/svgalib/ramdac/s3dacs.o \
	$(VOBJ)/svgalib/ramdac/sierra.o \
	$(VOBJ)/svgalib/ramdac/btdacs.o \
	$(VOBJ)/svgalib/ramdac/ics_gend.o \
	$(VOBJ)/svgalib/clockchi/icd2061a.o
VOBJDIRS += \
	$(VOBJ)/card \
	$(VOBJ)/svgalib \
	$(VOBJ)/svgalib/ramdac \
	$(VOBJ)/svgalib/clockchi \
	$(VOBJ)/svgalib/drivers
endif

$(VOBJ)/%.o: advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(VCFLAGS) -c $< -o $@

$(sort $(VOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(VOBJ)/advv$(EXE) : $(sort $(VOBJDIRS)) $(VOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(VLDFLAGS) $(VOBJS) $(VLIBS) -o $@
ifeq ($(CONF_COMPRESS),yes)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advv$(EXE)
	$(LN) $@ advv$(EXE)
