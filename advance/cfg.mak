############################################################################
# CFG

CFGCFLAGS += \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/blit \
	-I$(srcdir)/advance/v \
	-DUSE_BLIT_TINY
CFGOBJDIRS += \
	$(CFGOBJ)/cfg \
	$(CFGOBJ)/lib \
	$(CFGOBJ)/blit \
	$(CFGOBJ)/v
CFGOBJS += \
	$(CFGOBJ)/lib/portable.o \
	$(CFGOBJ)/lib/snstring.o \
	$(CFGOBJ)/lib/log.o \
	$(CFGOBJ)/lib/video.o \
	$(CFGOBJ)/lib/measure.o \
	$(CFGOBJ)/lib/rgb.o \
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
	$(CFGOBJ)/blit/slice.o \
	$(CFGOBJ)/cfg/cfg.o \
	$(CFGOBJ)/cfg/list.o \
	$(CFGOBJ)/v/draw.o

ifeq ($(CONF_SYSTEM),unix)
CFGCFLAGS += \
	-DADV_DATADIR=\"$(datadir)\" \
	-DADV_SYSCONFDIR=\"$(sysconfdir)\" \
	-I$(srcdir)/advance/linux
CFGOBJDIRS += \
	$(CFGOBJ)/linux
CFGOBJS += \
	$(CFGOBJ)/linux/file.o \
	$(CFGOBJ)/linux/target.o \
	$(CFGOBJ)/linux/os.o
CFGLIBS += -lm
ifeq ($(CONF_LIB_SLANG),yes)
CFGCFLAGS += \
	-DUSE_VIDEO_SLANG
CFGLIBS += -lslang
CFGOBJS += \
	$(CFGOBJ)/linux/vslang.o
endif
ifeq ($(CONF_LIB_NCURSES),yes)
CFGCFLAGS += \
	-DUSE_VIDEO_CURSES
CFGLIBS += -lncurses
CFGOBJS += \
	$(CFGOBJ)/linux/vcurses.o
endif
CFGCFLAGS += \
	-DUSE_INPUT_TTY
CFGOBJS += \
	$(CFGOBJ)/linux/itty.o
ifeq ($(CONF_LIB_SVGALIB),yes)
CFGCFLAGS += \
	-DUSE_VIDEO_SVGALIB
CFGLIBS += -lvga
CFGOBJS += \
	$(CFGOBJ)/linux/vsvgab.o
endif
ifeq ($(CONF_LIB_FB),yes)
CFGCFLAGS += \
	-DUSE_VIDEO_FB
CFGOBJS += \
	$(CFGOBJ)/linux/vfb.o
endif
endif

ifeq ($(CONF_SYSTEM),dos)
CFGCFLAGS += \
	-DUSE_ADV_SVGALIB_DOS \
	-I$(srcdir)/advance/dos \
	-I$(srcdir)/advance/card \
	-I$(srcdir)/advance/svgalib \
	-I$(srcdir)/advance/svgalib/svgados \
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
	$(CFGOBJ)/svgalib/svgados \
	$(CFGOBJ)/svgalib/ramdac \
	$(CFGOBJ)/svgalib/clockchi \
	$(CFGOBJ)/svgalib/drivers
CFGOBJS += \
	$(CFGOBJ)/dos/file.o \
	$(CFGOBJ)/dos/target.o \
	$(CFGOBJ)/dos/os.o \
	$(CFGOBJ)/dos/vvgal.o \
	$(CFGOBJ)/dos/vvbel.o \
	$(CFGOBJ)/dos/vsvgal.o \
	$(CFGOBJ)/dos/scrvbe.o \
	$(CFGOBJ)/dos/scrvga.o \
	$(CFGOBJ)/dos/idos.o \
	$(CFGOBJ)/card/card.o \
	$(CFGOBJ)/dos/pci.o \
	$(CFGOBJ)/dos/pcimap.o \
	$(CFGOBJ)/dos/map.o \
	$(CFGOBJ)/card/board.o \
	$(CFGOBJ)/svgalib/svgalib.o \
	$(CFGOBJ)/svgalib/svgados/svgados.o \
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
	$(CFGOBJ)/svgalib/drivers/laguna.o \
	$(CFGOBJ)/svgalib/drivers/millenni.o \
	$(CFGOBJ)/svgalib/drivers/mx.o \
	$(CFGOBJ)/svgalib/drivers/nv3.o \
	$(CFGOBJ)/svgalib/drivers/nv319.o \
	$(CFGOBJ)/svgalib/drivers/r128.o \
	$(CFGOBJ)/svgalib/drivers/rage.o \
	$(CFGOBJ)/svgalib/drivers/s3.o \
	$(CFGOBJ)/svgalib/drivers/savage.o \
	$(CFGOBJ)/svgalib/drivers/savage18.o \
	$(CFGOBJ)/svgalib/drivers/sis.o \
	$(CFGOBJ)/svgalib/drivers/trident.o \
	$(CFGOBJ)/svgalib/drivers/renditio.o \
	$(CFGOBJ)/svgalib/drivers/unichrom.o \
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

ifeq ($(CONF_SYSTEM),windows)
CFGCFLAGS += \
	-DADV_DATADIR=\"$(DATADIR)\" \
	-I$(srcdir)/advance/windows
CFGOBJDIRS += \
	$(CFGOBJ)/windows \
	$(CFGOBJ)/dos
CFGOBJS += \
	$(CFGOBJ)/dos/file.o \
	$(CFGOBJ)/windows/target.o \
	$(CFGOBJ)/windows/os.o
ifeq ($(CONF_LIB_SDL),yes)
CFGOBJDIRS += \
	$(CFGOBJ)/sdl
CFGCFLAGS += \
	$(SDLCFLAGS) \
	-I$(srcdir)/advance/sdl \
	-DUSE_VIDEO_SDL -DUSE_INPUT_SDL
CFGLIBS += $(SDLLIBS)
CFGOBJS += \
	$(CFGOBJ)/sdl/vsdl.o \
	$(CFGOBJ)/sdl/isdl.o
# Customize the SDL_main function
CFGCFLAGS += -DNO_STDIO_REDIRECT
CFGOBJS += $(CFGOBJ)/windows/sdlmain.o
endif
ifeq ($(CONF_LIB_SVGAWIN),yes)
CFGOBJDIRS += \
	$(CFGOBJ)/svgalib \
	$(CFGOBJ)/svgalib/ramdac \
	$(CFGOBJ)/svgalib/clockchi \
	$(CFGOBJ)/svgalib/drivers \
	$(CFGOBJ)/svgalib/svgawin
CFGCFLAGS += \
	-DUSE_ADV_SVGALIB_WIN \
	-I$(srcdir)/advance/svgalib \
	-I$(srcdir)/advance/svgalib/clockchi \
	-I$(srcdir)/advance/svgalib/ramdac \
	-I$(srcdir)/advance/svgalib/drivers \
	-I$(srcdir)/advance/svgalib/svgawin \
	-DUSE_VIDEO_SVGAWIN
CFGOBJS += \
	$(CFGOBJ)/windows/vsvgawin.o \
	$(CFGOBJ)/svgalib/svgalib.o \
	$(CFGOBJ)/svgalib/svgawin/svgawin.o \
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
	$(CFGOBJ)/svgalib/drivers/laguna.o \
	$(CFGOBJ)/svgalib/drivers/millenni.o \
	$(CFGOBJ)/svgalib/drivers/mx.o \
	$(CFGOBJ)/svgalib/drivers/nv3.o \
	$(CFGOBJ)/svgalib/drivers/nv319.o \
	$(CFGOBJ)/svgalib/drivers/r128.o \
	$(CFGOBJ)/svgalib/drivers/rage.o \
	$(CFGOBJ)/svgalib/drivers/s3.o \
	$(CFGOBJ)/svgalib/drivers/savage.o \
	$(CFGOBJ)/svgalib/drivers/savage18.o \
	$(CFGOBJ)/svgalib/drivers/sis.o \
	$(CFGOBJ)/svgalib/drivers/trident.o \
	$(CFGOBJ)/svgalib/drivers/renditio.o \
	$(CFGOBJ)/svgalib/drivers/unichrom.o \
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
endif

$(CFGOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(CFGCFLAGS) -c $< -o $@

$(CFGOBJ):
	$(ECHO) $@
	$(MD) $@

$(sort $(CFGOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(CFGOBJ)/advcfg$(EXE) : $(sort $(CFGOBJDIRS)) $(CFGOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(CFGOBJS) $(CFGLIBS) $(CFGLDFLAGS) $(LDFLAGS) -o $@
	$(RM) advcfg$(EXE)
	$(LN_S) $@ advcfg$(EXE)
