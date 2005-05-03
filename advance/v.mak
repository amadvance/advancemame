############################################################################
# V

VCFLAGS += \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/blit \
	-DUSE_BLIT_TINY
VOBJS += \
	$(VOBJ)/lib/portable.o \
	$(VOBJ)/lib/snstring.o \
	$(VOBJ)/lib/log.o \
	$(VOBJ)/lib/video.o \
	$(VOBJ)/lib/measure.o \
	$(VOBJ)/lib/rgb.o \
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
	$(VOBJ)/lib/inputall.o \
	$(VOBJ)/lib/inputdrv.o \
	$(VOBJ)/lib/videoall.o \
	$(VOBJ)/lib/error.o \
	$(VOBJ)/blit/blit.o \
	$(VOBJ)/blit/clear.o \
	$(VOBJ)/blit/slice.o \
	$(VOBJ)/v/v.o \
	$(VOBJ)/v/draw.o
VOBJDIRS += \
	$(VOBJ)/v \
	$(VOBJ)/lib \
	$(VOBJ)/blit
VLIBS += -lm

ifeq ($(CONF_SYSTEM),unix)
VCFLAGS += \
	-DADV_DATADIR=\"$(datadir)\" \
	-DADV_SYSCONFDIR=\"$(sysconfdir)\" \
	-I$(srcdir)/advance/linux
VOBJDIRS += \
	$(VOBJ)/linux
VOBJS += \
	$(VOBJ)/linux/file.o \
	$(VOBJ)/linux/target.o \
	$(VOBJ)/linux/os.o
ifeq ($(CONF_LIB_SLANG),yes)
VCFLAGS += \
	-DUSE_VIDEO_SLANG
VLIBS += -lslang
VOBJS += \
	$(VOBJ)/linux/vslang.o
endif
ifeq ($(CONF_LIB_NCURSES),yes)
VCFLAGS += \
	-DUSE_VIDEO_CURSES
VLIBS += -lncurses
VOBJS += \
	$(VOBJ)/linux/vcurses.o
endif
VCFLAGS += \
	-DUSE_INPUT_TTY
VOBJS += \
	$(VOBJ)/linux/itty.o
ifeq ($(CONF_LIB_SVGALIB),yes)
VCFLAGS += \
	-DUSE_VIDEO_SVGALIB
VLIBS += -lvga
VOBJS += \
	$(VOBJ)/linux/vsvgab.o
endif
ifeq ($(CONF_LIB_FB),yes)
VCFLAGS += \
	-DUSE_VIDEO_FB
VOBJS += \
	$(VOBJ)/linux/vfb.o
endif
endif

ifeq ($(CONF_SYSTEM),dos)
VCFLAGS += \
	-DUSE_ADV_SVGALIB_DOS \
	-I$(srcdir)/advance/dos \
	-I$(srcdir)/advance/card \
	-I$(srcdir)/advance/svgalib \
	-I$(srcdir)/advance/svgalib/svgados \
	-I$(srcdir)/advance/svgalib/clockchi \
	-I$(srcdir)/advance/svgalib/ramdac \
	-I$(srcdir)/advance/svgalib/drivers \
	-DUSE_VIDEO_SVGALINE -DUSE_VIDEO_VBELINE -DUSE_VIDEO_VGALINE -DUSE_VIDEO_VBE \
	-DUSE_INPUT_DOS
VLIBS += -lalleg
VOBJDIRS += \
	$(VOBJ)/dos \
	$(VOBJ)/card \
	$(VOBJ)/svgalib \
	$(VOBJ)/svgalib/svgados \
	$(VOBJ)/svgalib/ramdac \
	$(VOBJ)/svgalib/clockchi \
	$(VOBJ)/svgalib/drivers
VOBJS += \
	$(VOBJ)/dos/file.o \
	$(VOBJ)/dos/target.o \
	$(VOBJ)/dos/os.o \
	$(VOBJ)/dos/vvgal.o \
	$(VOBJ)/dos/vvbe.o \
	$(VOBJ)/dos/vvbel.o \
	$(VOBJ)/dos/vsvgal.o \
	$(VOBJ)/dos/scrvbe.o \
	$(VOBJ)/dos/scrvga.o \
	$(VOBJ)/dos/idos.o \
	$(VOBJ)/card/card.o \
	$(VOBJ)/dos/pci.o \
	$(VOBJ)/dos/map.o \
	$(VOBJ)/dos/pcimap.o \
	$(VOBJ)/card/board.o \
	$(VOBJ)/svgalib/svgalib.o \
	$(VOBJ)/svgalib/svgados/svgados.o \
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
	$(VOBJ)/svgalib/drivers/laguna.o \
	$(VOBJ)/svgalib/drivers/millenni.o \
	$(VOBJ)/svgalib/drivers/mx.o \
	$(VOBJ)/svgalib/drivers/nv3.o \
	$(VOBJ)/svgalib/drivers/nv319.o \
	$(VOBJ)/svgalib/drivers/r128.o \
	$(VOBJ)/svgalib/drivers/rage.o \
	$(VOBJ)/svgalib/drivers/s3.o \
	$(VOBJ)/svgalib/drivers/savage.o \
	$(VOBJ)/svgalib/drivers/savage18.o \
	$(VOBJ)/svgalib/drivers/sis.o \
	$(VOBJ)/svgalib/drivers/trident.o \
	$(VOBJ)/svgalib/drivers/renditio.o \
	$(VOBJ)/svgalib/drivers/unichrom.o \
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
endif

ifeq ($(CONF_SYSTEM),windows)
VCFLAGS += \
	-DADV_DATADIR=\"$(DATADIR)\" \
	-I$(srcdir)/advance/windows
VOBJDIRS += \
	$(VOBJ)/windows \
	$(VOBJ)/dos
VOBJS += \
	$(VOBJ)/dos/file.o \
	$(VOBJ)/windows/target.o \
	$(VOBJ)/windows/os.o
ifeq ($(CONF_LIB_SDL),yes)
VOBJDIRS += \
	$(VOBJ)/sdl
VCFLAGS += \
	$(SDLCFLAGS) \
	-I$(srcdir)/advance/sdl \
	-DUSE_VIDEO_SDL -DUSE_INPUT_SDL
VLIBS += $(SDLLIBS)
VOBJS += \
	$(VOBJ)/sdl/vsdl.o \
	$(VOBJ)/sdl/isdl.o
# Customize the SDL_main function
VCFLAGS += -DNO_STDIO_REDIRECT
VOBJS += $(VOBJ)/windows/sdlmain.o
endif
ifeq ($(CONF_LIB_SVGAWIN),yes)
VOBJDIRS += \
	$(VOBJ)/svgalib \
	$(VOBJ)/svgalib/ramdac \
	$(VOBJ)/svgalib/clockchi \
	$(VOBJ)/svgalib/drivers \
	$(VOBJ)/svgalib/svgawin
VCFLAGS += \
	-DUSE_ADV_SVGALIB_WIN \
	-I$(srcdir)/advance/svgalib \
	-I$(srcdir)/advance/svgalib/clockchi \
	-I$(srcdir)/advance/svgalib/ramdac \
	-I$(srcdir)/advance/svgalib/drivers \
	-I$(srcdir)/advance/svgalib/svgawin \
	-DUSE_VIDEO_SVGAWIN
VOBJS += \
	$(VOBJ)/windows/vsvgawin.o \
	$(VOBJ)/svgalib/svgalib.o \
	$(VOBJ)/svgalib/svgawin/svgawin.o \
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
	$(VOBJ)/svgalib/drivers/laguna.o \
	$(VOBJ)/svgalib/drivers/millenni.o \
	$(VOBJ)/svgalib/drivers/mx.o \
	$(VOBJ)/svgalib/drivers/nv3.o \
	$(VOBJ)/svgalib/drivers/nv319.o \
	$(VOBJ)/svgalib/drivers/r128.o \
	$(VOBJ)/svgalib/drivers/rage.o \
	$(VOBJ)/svgalib/drivers/s3.o \
	$(VOBJ)/svgalib/drivers/savage.o \
	$(VOBJ)/svgalib/drivers/savage18.o \
	$(VOBJ)/svgalib/drivers/sis.o \
	$(VOBJ)/svgalib/drivers/trident.o \
	$(VOBJ)/svgalib/drivers/renditio.o \
	$(VOBJ)/svgalib/drivers/unichrom.o \
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
endif
endif

$(VOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(VCFLAGS) -c $< -o $@

$(VOBJ):
	$(ECHO) $@
	$(MD) $@

$(sort $(VOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(VOBJ)/advv$(EXE) : $(sort $(VOBJDIRS)) $(VOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(VOBJS) $(VLIBS) $(VLDFLAGS) $(LDFLAGS) -o $@
	$(RM) advv$(EXE)
	$(LN_S) $@ advv$(EXE)
