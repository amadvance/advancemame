############################################################################
# system

ADVANCECFLAGS += \
	-I$(srcdir)/advance/osd \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/blit

ADVANCEOBJDIRS += \
	$(OBJ)/advance \
	$(OBJ)/advance/lib \
	$(OBJ)/advance/osd \
	$(OBJ)/advance/blit

ifeq ($(CONF_SYSTEM),unix)

# Dependencies on DATADIR/SYSCONFDIR
$(OBJ)/advance/linux/file.o: Makefile
$(MESSOBJ)/advance/linux/file.o: Makefile

# Allow external customization for special targets
ifndef DATADIR
DATADIR=$(datadir)/advance
endif
ifndef SYSCONFDIR
SYSCONFDIR=$(sysconfdir)
endif

ADVANCEOBJDIRS += \
	$(OBJ)/advance/linux
ADVANCECFLAGS += \
	-I$(srcdir)/advance/linux \
	-DADV_DATADIR=\"$(DATADIR)\" \
	-DADV_SYSCONFDIR=\"$(SYSCONFDIR)\" \
	-DUSE_VIDEO_NONE \
	-DUSE_SOUND_NONE \
	-DUSE_KEYBOARD_NONE \
	-DUSE_MOUSE_NONE \
	-DUSE_JOYSTICK_NONE \
	-DUSE_LCD
ADVANCEOBJS += \
	$(OBJ)/advance/linux/file.o \
	$(OBJ)/advance/linux/target.o \
	$(OBJ)/advance/linux/os.o \
	$(OBJ)/advance/lib/lcd.o
ifeq ($(CONF_LIB_PTHREAD),yes)
CFLAGS += -D_REENTRANT
ADVANCECFLAGS += -DUSE_SMP
ADVANCELIBS += -lpthread
ADVANCEOBJS += $(OBJ)/advance/osd/thdouble.o
else
ADVANCEOBJS += $(OBJ)/advance/osd/thmono.o
endif
ifeq ($(CONF_LIB_SDL),yes)
ADVANCEOBJDIRS += \
	$(OBJ)/advance/sdl
ADVANCECFLAGS += \
	$(SDLCFLAGS) \
	-I$(srcdir)/advance/sdl \
	-DUSE_VIDEO_SDL \
	-DUSE_SOUND_SDL \
	-DUSE_KEYBOARD_SDL \
	-DUSE_MOUSE_SDL \
	-DUSE_JOYSTICK_SDL 
ADVANCELIBS += $(SDLLIBS)
ADVANCEOBJS += \
	$(OBJ)/advance/sdl/ssdl.o \
	$(OBJ)/advance/sdl/msdl.o \
	$(OBJ)/advance/sdl/ksdl.o \
	$(OBJ)/advance/sdl/jsdl.o \
	$(OBJ)/advance/sdl/vsdl.o
endif
ifeq ($(CONF_LIB_FREETYPE),yes)
ADVANCECFLAGS += \
	$(FREETYPECFLAGS) \
	-DUSE_FREETYPE
ADVANCELIBS += $(FREETYPELIBS)
endif
ifeq ($(CONF_LIB_FB),yes)
ADVANCECFLAGS += \
	-DUSE_VIDEO_FB
ADVANCEOBJS += \
	$(OBJ)/advance/linux/vfb.o
endif
ifeq ($(CONF_LIB_VC),yes)
ADVANCECFLAGS += \
	$(VCCFLAGS) \
	-DUSE_VC
ADVANCELIBS += $(VCLIBS)
endif
ifeq ($(CONF_LIB_SVGALIB),yes)
ADVANCECFLAGS += \
	-DUSE_VIDEO_SVGALIB \
	-DUSE_KEYBOARD_SVGALIB \
	-DUSE_MOUSE_SVGALIB \
	-DUSE_JOYSTICK_SVGALIB
ADVANCELIBS += -lvga
ADVANCEOBJS += \
	$(OBJ)/advance/linux/vsvgab.o \
	$(OBJ)/advance/linux/jsvgab.o \
	$(OBJ)/advance/linux/ksvgab.o \
	$(OBJ)/advance/linux/msvgab.o
endif
ifeq ($(CONF_LIB_ALSA),yes)
ADVANCECFLAGS += \
	-DUSE_SOUND_ALSA
ADVANCEOBJS += \
	$(OBJ)/advance/linux/salsa.o
ADVANCELIBS += -lasound
endif
ifeq ($(CONF_LIB_OSS),yes)
ADVANCECFLAGS += \
	-DUSE_SOUND_OSS
ADVANCEOBJS += \
	$(OBJ)/advance/linux/soss.o
endif
ifeq ($(CONF_LIB_KRAW),yes)
ADVANCECFLAGS += \
	-DUSE_KEYBOARD_RAW
ADVANCEOBJS += \
	$(OBJ)/advance/linux/kraw.o
endif
ifeq ($(CONF_LIB_JRAW),yes)
ADVANCECFLAGS += \
	-DUSE_JOYSTICK_RAW
ADVANCEOBJS += \
	$(OBJ)/advance/linux/jraw.o
endif
ifeq ($(CONF_LIB_MRAW),yes)
ADVANCECFLAGS += \
	-DUSE_MOUSE_RAW
ADVANCEOBJS += \
	$(OBJ)/advance/linux/mraw.o
endif
ifeq ($(CONF_LIB_KEVENT),yes)
ADVANCECFLAGS += \
	-DUSE_KEYBOARD_EVENT
ADVANCEOBJS += \
	$(OBJ)/advance/linux/kevent.o
endif
ifeq ($(CONF_LIB_MEVENT),yes)
ADVANCECFLAGS += \
	-DUSE_MOUSE_EVENT
ADVANCEOBJS += \
	$(OBJ)/advance/linux/mevent.o
endif
ifeq ($(CONF_LIB_JEVENT),yes)
ADVANCECFLAGS += \
	-DUSE_JOYSTICK_EVENT
ADVANCEOBJS += \
	$(OBJ)/advance/linux/jevent.o
endif
ifneq (,$(findstring _EVENT,$(ADVANCECFLAGS)))
ADVANCEOBJS += \
	$(OBJ)/advance/linux/event.o
endif
endif

ifeq ($(CONF_SYSTEM),dos)
ADVANCEOBJDIRS += \
	$(OBJ)/advance/dos
ADVANCECFLAGS += \
	-DUSE_CONFIG_ALLEGRO_WRAPPER \
	-DUSE_ADV_SVGALIB_DOS \
	-I$(srcdir)/advance/dos \
	-I$(srcdir)/advance/card \
	-I$(srcdir)/advance/svgalib \
	-I$(srcdir)/advance/svgalib/clockchi \
	-I$(srcdir)/advance/svgalib/ramdac \
	-I$(srcdir)/advance/svgalib/drivers \
	-I$(srcdir)/advance/svgalib/svgados
ADVANCECFLAGS += \
	-DUSE_VIDEO_SVGALINE -DUSE_VIDEO_VBELINE -DUSE_VIDEO_VGALINE -DUSE_VIDEO_VBE -DUSE_VIDEO_NONE \
	-DUSE_SOUND_ALLEGRO -DUSE_SOUND_SEAL -DUSE_SOUND_VSYNC -DUSE_SOUND_NONE \
	-DUSE_KEYBOARD_ALLEGRO -DUSE_KEYBOARD_NONE \
	-DUSE_MOUSE_ALLEGRO -DUSE_MOUSE_NONE \
	-DUSE_JOYSTICK_ALLEGRO -DUSE_JOYSTICK_LGALLEGRO -DUSE_JOYSTICK_NONE
ADVANCELDFLAGS += -Xlinker --wrap -Xlinker _mixer_init
ADVANCELIBS += -laudio -lalleg
ADVANCELDFLAGS += \
	-Xlinker --wrap -Xlinker get_config_string \
	-Xlinker --wrap -Xlinker get_config_int \
	-Xlinker --wrap -Xlinker set_config_string \
	-Xlinker --wrap -Xlinker set_config_int \
	-Xlinker --wrap -Xlinker get_config_id \
	-Xlinker --wrap -Xlinker set_config_id
ADVANCEOBJDIRS += \
	$(OBJ)/advance/card \
	$(OBJ)/advance/svgalib \
	$(OBJ)/advance/svgalib/ramdac \
	$(OBJ)/advance/svgalib/clockchi \
	$(OBJ)/advance/svgalib/drivers \
	$(OBJ)/advance/svgalib/svgados
ADVANCEOBJS += \
	$(OBJ)/advance/dos/file.o \
	$(OBJ)/advance/dos/target.o \
	$(OBJ)/advance/dos/os.o \
	$(OBJ)/advance/dos/sseal.o \
	$(OBJ)/advance/dos/salleg.o \
	$(OBJ)/advance/dos/svsync.o \
	$(OBJ)/advance/dos/vvbe.o \
	$(OBJ)/advance/dos/vvgal.o \
	$(OBJ)/advance/dos/vvbel.o \
	$(OBJ)/advance/dos/vsvgal.o \
	$(OBJ)/advance/dos/scrvbe.o \
	$(OBJ)/advance/dos/scrvga.o \
	$(OBJ)/advance/dos/jalleg.o \
	$(OBJ)/advance/dos/jlgalleg.o \
	$(OBJ)/advance/dos/kalleg.o \
	$(OBJ)/advance/dos/malleg.o \
	$(OBJ)/advance/card/card.o \
	$(OBJ)/advance/dos/pci.o \
	$(OBJ)/advance/dos/pcimap.o \
	$(OBJ)/advance/dos/map.o \
	$(OBJ)/advance/card/board.o \
	$(OBJ)/advance/svgalib/svgalib.o \
	$(OBJ)/advance/svgalib/svgados/svgados.o \
	$(OBJ)/advance/svgalib/accel.o \
	$(OBJ)/advance/svgalib/vgaio.o \
	$(OBJ)/advance/svgalib/vgammvga.o \
	$(OBJ)/advance/svgalib/vgaregs.o \
	$(OBJ)/advance/svgalib/vgarelvg.o \
	$(OBJ)/advance/svgalib/drivers/apm.o \
	$(OBJ)/advance/svgalib/drivers/ark.o \
	$(OBJ)/advance/svgalib/drivers/banshee.o \
	$(OBJ)/advance/svgalib/drivers/et6000.o \
	$(OBJ)/advance/svgalib/drivers/g400.o \
	$(OBJ)/advance/svgalib/drivers/pm2.o \
	$(OBJ)/advance/svgalib/drivers/laguna.o \
	$(OBJ)/advance/svgalib/drivers/millenni.o \
	$(OBJ)/advance/svgalib/drivers/mx.o \
	$(OBJ)/advance/svgalib/drivers/nv3.o \
	$(OBJ)/advance/svgalib/drivers/nv319.o \
	$(OBJ)/advance/svgalib/drivers/r128.o \
	$(OBJ)/advance/svgalib/drivers/rage.o \
	$(OBJ)/advance/svgalib/drivers/s3.o \
	$(OBJ)/advance/svgalib/drivers/savage.o \
	$(OBJ)/advance/svgalib/drivers/savage18.o \
	$(OBJ)/advance/svgalib/drivers/sis.o \
	$(OBJ)/advance/svgalib/drivers/trident.o \
	$(OBJ)/advance/svgalib/drivers/renditio.o \
	$(OBJ)/advance/svgalib/drivers/unichrom.o \
	$(OBJ)/advance/svgalib/ramdac/ibmrgb52.o \
	$(OBJ)/advance/svgalib/ramdac/attdacs.o \
	$(OBJ)/advance/svgalib/ramdac/icw.o \
	$(OBJ)/advance/svgalib/ramdac/normal.o \
	$(OBJ)/advance/svgalib/ramdac/ramdac.o \
	$(OBJ)/advance/svgalib/ramdac/s3dacs.o \
	$(OBJ)/advance/svgalib/ramdac/sierra.o \
	$(OBJ)/advance/svgalib/ramdac/btdacs.o \
	$(OBJ)/advance/svgalib/ramdac/ics_gend.o \
	$(OBJ)/advance/svgalib/clockchi/icd2061a.o
ADVANCEOBJS += $(OBJ)/advance/osd/thmono.o
ifeq ($(CONF_LIB_FREETYPE),yes)
ADVANCECFLAGS += \
	$(FREETYPECFLAGS) \
	-DUSE_FREETYPE
ADVANCELIBS += $(FREETYPELIBS)
endif
endif

ifeq ($(CONF_SYSTEM),windows)
ADVANCEOBJDIRS += \
	$(OBJ)/advance/windows \
	$(OBJ)/advance/dos
ADVANCECFLAGS += \
	-I$(srcdir)/advance/windows \
	-DUSE_VIDEO_NONE \
	-DUSE_SOUND_NONE \
	-DUSE_KEYBOARD_NONE \
	-DUSE_MOUSE_NONE \
	-DUSE_JOYSTICK_NONE
ADVANCEOBJS += \
	$(OBJ)/advance/dos/file.o \
	$(OBJ)/advance/windows/target.o \
	$(OBJ)/advance/lib/resource.o \
	$(OBJ)/advance/windows/os.o
ifeq ($(CONF_LIB_PTHREAD),yes)
CFLAGS += -D_REENTRANT
ADVANCECFLAGS += -DUSE_SMP
# pthread-win32 library without exceptions management
ADVANCELIBS += -lpthread
ADVANCEOBJS += $(OBJ)/advance/osd/thdouble.o
else
ADVANCEOBJS += $(OBJ)/advance/osd/thmono.o
endif
ifeq ($(CONF_LIB_SDL),yes)
ADVANCEOBJDIRS += \
	$(OBJ)/advance/sdl
ADVANCECFLAGS += \
	$(SDLCFLAGS) \
	-I$(srcdir)/advance/sdl \
	-DUSE_VIDEO_SDL \
	-DUSE_SOUND_SDL \
	-DUSE_KEYBOARD_SDL \
	-DUSE_MOUSE_SDL \
	-DUSE_JOYSTICK_SDL
ADVANCELIBS += $(SDLLIBS)
ADVANCEOBJS += \
	$(OBJ)/advance/sdl/vsdl.o \
	$(OBJ)/advance/sdl/ssdl.o \
	$(OBJ)/advance/sdl/jsdl.o \
	$(OBJ)/advance/sdl/ksdl.o \
	$(OBJ)/advance/sdl/msdl.o
# Customize the SDL_main function
ADVANCECFLAGS += -DNO_STDIO_REDIRECT
ADVANCEOBJS += $(OBJ)/advance/windows/sdlmain.o
endif
ifeq ($(CONF_LIB_SVGAWIN),yes)
ADVANCEOBJDIRS += \
	$(OBJ)/advance/svgalib \
	$(OBJ)/advance/svgalib/ramdac \
	$(OBJ)/advance/svgalib/clockchi \
	$(OBJ)/advance/svgalib/drivers \
	$(OBJ)/advance/svgalib/svgawin
ADVANCECFLAGS += \
	-DUSE_ADV_SVGALIB_WIN \
	-I$(srcdir)/advance/svgalib \
	-I$(srcdir)/advance/svgalib/clockchi \
	-I$(srcdir)/advance/svgalib/ramdac \
	-I$(srcdir)/advance/svgalib/drivers \
	-I$(srcdir)/advance/svgalib/svgawin \
	-DUSE_VIDEO_SVGAWIN
ADVANCEOBJS += \
	$(OBJ)/advance/windows/vsvgawin.o \
	$(OBJ)/advance/svgalib/svgalib.o \
	$(OBJ)/advance/svgalib/svgawin/svgawin.o \
	$(OBJ)/advance/svgalib/accel.o \
	$(OBJ)/advance/svgalib/vgaio.o \
	$(OBJ)/advance/svgalib/vgammvga.o \
	$(OBJ)/advance/svgalib/vgaregs.o \
	$(OBJ)/advance/svgalib/vgarelvg.o \
	$(OBJ)/advance/svgalib/drivers/apm.o \
	$(OBJ)/advance/svgalib/drivers/ark.o \
	$(OBJ)/advance/svgalib/drivers/banshee.o \
	$(OBJ)/advance/svgalib/drivers/et6000.o \
	$(OBJ)/advance/svgalib/drivers/g400.o \
	$(OBJ)/advance/svgalib/drivers/pm2.o \
	$(OBJ)/advance/svgalib/drivers/laguna.o \
	$(OBJ)/advance/svgalib/drivers/millenni.o \
	$(OBJ)/advance/svgalib/drivers/mx.o \
	$(OBJ)/advance/svgalib/drivers/nv3.o \
	$(OBJ)/advance/svgalib/drivers/nv319.o \
	$(OBJ)/advance/svgalib/drivers/r128.o \
	$(OBJ)/advance/svgalib/drivers/rage.o \
	$(OBJ)/advance/svgalib/drivers/s3.o \
	$(OBJ)/advance/svgalib/drivers/savage.o \
	$(OBJ)/advance/svgalib/drivers/savage18.o \
	$(OBJ)/advance/svgalib/drivers/sis.o \
	$(OBJ)/advance/svgalib/drivers/trident.o \
	$(OBJ)/advance/svgalib/drivers/renditio.o \
	$(OBJ)/advance/svgalib/drivers/unichrom.o \
	$(OBJ)/advance/svgalib/ramdac/ibmrgb52.o \
	$(OBJ)/advance/svgalib/ramdac/attdacs.o \
	$(OBJ)/advance/svgalib/ramdac/icw.o \
	$(OBJ)/advance/svgalib/ramdac/normal.o \
	$(OBJ)/advance/svgalib/ramdac/ramdac.o \
	$(OBJ)/advance/svgalib/ramdac/s3dacs.o \
	$(OBJ)/advance/svgalib/ramdac/sierra.o \
	$(OBJ)/advance/svgalib/ramdac/btdacs.o \
	$(OBJ)/advance/svgalib/ramdac/ics_gend.o \
	$(OBJ)/advance/svgalib/clockchi/icd2061a.o
endif
ifeq ($(CONF_LIB_MCPN),yes)
ADVANCECFLAGS += \
	-DUSE_MOUSE_CPN
ADVANCEOBJS += \
	$(OBJ)/advance/windows/mcpn.o
ADVANCELIBS += -lsetupapi
endif
ifeq ($(CONF_LIB_MRAWINPUT),yes)
ADVANCECFLAGS += \
	-DUSE_MOUSE_RAWINPUT
ADVANCEOBJS += \
	$(OBJ)/advance/windows/mraw.o
endif
ifeq ($(CONF_LIB_JLGRAWINPUT),yes)
ADVANCECFLAGS += \
	-DUSE_JOYSTICK_LGRAWINPUT
ADVANCEOBJS += \
	$(OBJ)/advance/windows/jlgraw.o
endif
ifeq ($(CONF_LIB_FREETYPE),yes)
ADVANCECFLAGS += \
	$(FREETYPECFLAGS) \
	-DUSE_FREETYPE
ADVANCELIBS += $(FREETYPELIBS)
endif
endif

############################################################################
# emu

# Dependencies on VERSION/DATADIR/SYSCONFDIR
$(OBJ)/advance/osd/emu.o: Makefile
$(OBJ)/advance/osd/ui.o: Makefile
$(MESSOBJ)/advance/osd/emu.o: Makefile
$(MESSOBJ)/advance/osd/ui.o: Makefile

ADVANCECFLAGS += -DADV_VERSION=\"$(VERSION)\" -DADV_EMU
MAMECFLAGS += -DMAME
MESSCFLAGS += -DMESS

ifeq ($(CONF_SYSTEM),unix)
EMUCFLAGS += \
	-DPI=M_PI \
	-Dstricmp=strcasecmp \
	-Dstrnicmp=strncasecmp
endif

ifeq ($(CONF_SYSTEM),windows)
EMUCFLAGS += \
	-DPI=3.1415927 \
	-DM_PI=3.1415927
endif

EMUCFLAGS += -I$(srcdir)/advance/osd

MAMEOBJS += \
	$(COREOBJS) \
	$(sort $(CPUOBJS)) \
	$(sort $(SOUNDOBJS))

MESSOBJS += \
	$(MESSCOREOBJS) \
	$(sort $(MESSCPUOBJS)) \
	$(sort $(MESSSOUNDOBJS))

ADVANCEOBJS += \
	$(OBJ)/advance/osd/emu.o \
	$(OBJ)/advance/osd/glue.o \
	$(OBJ)/advance/osd/dvg.o \
	$(OBJ)/advance/osd/global.o \
	$(OBJ)/advance/osd/ui.o \
	$(OBJ)/advance/osd/video.o \
	$(OBJ)/advance/osd/frame.o \
	$(OBJ)/advance/osd/sync.o \
	$(OBJ)/advance/osd/mode.o \
	$(OBJ)/advance/osd/menu.o \
	$(OBJ)/advance/osd/estimate.o \
	$(OBJ)/advance/osd/record.o \
	$(OBJ)/advance/osd/sound.o \
	$(OBJ)/advance/osd/input.o \
	$(OBJ)/advance/osd/lexyy.o \
	$(OBJ)/advance/osd/y_tab.o \
	$(OBJ)/advance/osd/script.o \
	$(OBJ)/advance/osd/hscript.o \
	$(OBJ)/advance/osd/safequit.o \
	$(OBJ)/advance/osd/fileio.o \
	$(OBJ)/advance/osd/fuzzy.o \
	$(OBJ)/advance/blit/blit.o \
	$(OBJ)/advance/blit/hq2x.o \
	$(OBJ)/advance/blit/hq2x3.o \
	$(OBJ)/advance/blit/hq2x4.o \
	$(OBJ)/advance/blit/hq3x.o \
	$(OBJ)/advance/blit/hq4x.o \
	$(OBJ)/advance/blit/xbr2x.o \
	$(OBJ)/advance/blit/xbr3x.o \
	$(OBJ)/advance/blit/xbr4x.o \
	$(OBJ)/advance/blit/scale2x.o \
	$(OBJ)/advance/blit/scale3x.o \
	$(OBJ)/advance/blit/scale2k.o \
	$(OBJ)/advance/blit/scale3k.o \
	$(OBJ)/advance/blit/scale4k.o \
	$(OBJ)/advance/blit/interp.o \
	$(OBJ)/advance/blit/clear.o \
	$(OBJ)/advance/blit/slice.o \
	$(OBJ)/advance/lib/portable.o \
	$(OBJ)/advance/lib/snstring.o \
	$(OBJ)/advance/lib/log.o \
	$(OBJ)/advance/lib/video.o \
	$(OBJ)/advance/lib/measure.o \
	$(OBJ)/advance/lib/rgb.o \
	$(OBJ)/advance/lib/conf.o \
	$(OBJ)/advance/lib/incstr.o \
	$(OBJ)/advance/lib/fz.o \
	$(OBJ)/advance/lib/font.o \
	$(OBJ)/advance/lib/fontdef.o \
	$(OBJ)/advance/lib/bitmap.o \
	$(OBJ)/advance/lib/filter.o \
	$(OBJ)/advance/lib/dft.o \
	$(OBJ)/advance/lib/complex.o \
	$(OBJ)/advance/lib/png.o \
	$(OBJ)/advance/lib/pngdef.o \
	$(OBJ)/advance/lib/mng.o \
	$(OBJ)/advance/lib/unzip.o \
	$(OBJ)/advance/lib/videoio.o \
	$(OBJ)/advance/lib/update.o \
	$(OBJ)/advance/lib/generate.o \
	$(OBJ)/advance/lib/crtc.o \
	$(OBJ)/advance/lib/crtcbag.o \
	$(OBJ)/advance/lib/monitor.o \
	$(OBJ)/advance/lib/sounddrv.o \
	$(OBJ)/advance/lib/snone.o \
	$(OBJ)/advance/lib/vnone.o \
	$(OBJ)/advance/lib/device.o \
	$(OBJ)/advance/lib/videoall.o \
	$(OBJ)/advance/lib/soundall.o \
	$(OBJ)/advance/lib/joy.o \
	$(OBJ)/advance/lib/joyall.o \
	$(OBJ)/advance/lib/joydrv.o \
	$(OBJ)/advance/lib/jnone.o \
	$(OBJ)/advance/lib/keyall.o \
	$(OBJ)/advance/lib/keydrv.o \
	$(OBJ)/advance/lib/knone.o \
	$(OBJ)/advance/lib/key.o \
	$(OBJ)/advance/lib/mouse.o \
	$(OBJ)/advance/lib/mouseall.o \
	$(OBJ)/advance/lib/mousedrv.o \
	$(OBJ)/advance/lib/mnone.o \
	$(OBJ)/advance/lib/error.o \
	$(OBJ)/advance/lib/wave.o

EMUCHDMANOBJS += \
	$(OBJ)/chdman.o \
	$(OBJ)/chd.o \
	$(OBJ)/cdrom.o \
	$(OBJ)/chdcd.o \
	$(OBJ)/sha1.o \
	$(OBJ)/md5.o \
	$(OBJ)/version.o

############################################################################
# expat

$(OBJ)/advance/libexpat.a: $(OBJ)/advance/expat/xmlparse.o $(OBJ)/advance/expat/xmlrole.o $(OBJ)/advance/expat/xmltok.o
	$(ECHO) $@
	$(AR) crs $@ $^

ifeq ($(CONF_LIB_EXPAT),yes)
ADVANCELIBS += -lexpat
else
CFLAGS += \
	-I$(srcdir)/advance/expat
ADVANCEOBJDIRS += \
	$(OBJ)/advance/expat
ADVANCEBUILDLIBS += \
	$(OBJ)/advance/libexpat.a
endif

############################################################################
# zlib

$(OBJ)/advance/libz.a: $(OBJ)/advance/zlib/adler32.o $(OBJ)/advance/zlib/crc32.o $(OBJ)/advance/zlib/deflate.o \
	$(OBJ)/advance/zlib/inffast.o $(OBJ)/advance/zlib/inflate.o \
	$(OBJ)/advance/zlib/infback.o $(OBJ)/advance/zlib/inftrees.o $(OBJ)/advance/zlib/trees.o \
	$(OBJ)/advance/zlib/zutil.o $(OBJ)/advance/zlib/uncompr.o $(OBJ)/advance/zlib/compress.o
	$(ECHO) $@
	$(AR) crs $@ $^

ifeq ($(CONF_LIB_ZLIB),yes)
ADVANCELIBS += -lz
EMUCHDMANLDFLAGS += -lz
else
CFLAGS += \
	-I$(srcdir)/advance/zlib
ADVANCEOBJDIRS += \
	$(OBJ)/advance/zlib
ADVANCEBUILDLIBS += \
	$(OBJ)/advance/libz.a
endif

############################################################################
# advance compile

$(OBJ)/advance/osd/%.o: $(srcdir)/advance/osd/%.c $(srcdir)/advance/osd/emu.h
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(EMUCFLAGS) $(MAMECFLAGS) $(ADVANCECFLAGS) -c $< -o $@

$(OBJ)/advance/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(ADVANCECFLAGS) -c $< -o $@

$(OBJ)/advance/%.o: $(srcdir)/advance/%.rc
	$(ECHO) $@ $(MSG)
	$(RC) $(RCFLAGS) -DADV_EMU $< -o $@

$(MESSOBJ)/advance/osd/%.o: $(srcdir)/advance/osd/%.c $(srcdir)/advance/osd/emu.h
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(EMUCFLAGS) $(MESSCFLAGS) $(ADVANCECFLAGS) -c $< -o $@

$(MESSOBJ)/advance/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(ADVANCECFLAGS) -c $< -o $@

$(MESSOBJ)/advance/%.o: $(srcdir)/advance/%.rc
	$(ECHO) $@ $(MSG)
	$(RC) $(RCFLAGS) -DADV_EMU $< -o $@

############################################################################
# emu compile

# Target CFLAGS
ifneq (,$(findstring USE_ASM_INLINE,$(CFLAGS)))
# Don't enable internam MAME assembler macros
# There are issues in the z80 emulator when using modern gcc compilers like 6.4.0
# For example, puckman/pengo games have video issues.
#EMUCFLAGS += -DX86_ASM
endif

ifneq (,$(findstring USE_ASM_EMUMIPS3,$(CFLAGS)))
# Don't enable MIPS3 DRC otherwise various games crash. Like biofreak, blitz, calspeed, carnevil, kinst and others...
#X86_MIPS3_DRC=1
endif

ifneq (,$(findstring USE_LSB,$(CFLAGS)))
EMUCFLAGS += -DLSB_FIRST
endif

ifneq (,$(findstring USE_MSB,$(CFLAGS)))
EMUCFLAGS += -DMSB_FIRST
endif

MESSCFLAGS += \
	-I$(srcdir)/mess \
	-Dvga_init=mess_vga_init
# -Dvga_init=mess_vga_init prevent a name clash with the vga_init SVGALIB function

EMUCFLAGS += \
	-DINLINE="static __inline__" \
	-Dasm=__asm__

# Map
ifeq ($(CONF_MAP),yes)
ADVANCELDFLAGS += -Xlinker -Map -Xlinker $(OBJ)/advmame.map
endif

# MAME
include $(srcdir)/src/core.mak
ifeq ($(CONF_TINY),yes)
include $(srcdir)/src/tiny.mak
else
include $(srcdir)/src/mame.mak
endif
include $(srcdir)/src/cpu/cpu.mak
include $(srcdir)/src/sound/sound.mak

# MESS
include $(srcdir)/srcmess/core.mak
ifeq ($(CONF_TINY),yes)
include $(srcdir)/mess/tinyms.mak
else
include $(srcdir)/mess/mess.mak
endif
include $(srcdir)/srcmess/cpu/cpu.mak
include $(srcdir)/mess/cpu/cpu.mak
include $(srcdir)/srcmess/sound/sound.mak
include $(srcdir)/mess/sound/sound.mak

# Special search paths required by the CPU core rules
VPATH=$(wildcard $(srcdir)/src/cpu/* $(srcdir)/src/sound)

OBJDIRS += \
	$(ADVANCEOBJDIRS) \
	$(OBJ)/cpu \
	$(OBJ)/sound \
	$(OBJ)/drivers \
	$(OBJ)/machine \
	$(OBJ)/vidhrdw \
	$(OBJ)/sndhrdw \
	$(OBJ)/debug
MESSOBJDIRS += \
	$(subst $(OBJ),$(MESSOBJ),$(ADVANCEOBJDIRS)) \
	$(MESSOBJ)/cpu \
	$(MESSOBJ)/sound \
	$(MESSOBJ)/drivers \
	$(MESSOBJ)/machine \
	$(MESSOBJ)/vidhrdw \
	$(MESSOBJ)/sndhrdw \
	$(MESSOBJ)/debug \
	$(MESSOBJ)/mess \
	$(MESSOBJ)/mess/systems \
	$(MESSOBJ)/mess/devices \
	$(MESSOBJ)/mess/expat \
	$(MESSOBJ)/mess/machine \
	$(MESSOBJ)/mess/vidhrdw \
	$(MESSOBJ)/mess/sndhrdw \
	$(MESSOBJ)/mess/formats \
	$(MESSOBJ)/mess/tools \
	$(MESSOBJ)/mess/tools/dat2html \
	$(MESSOBJ)/mess/tools/mkhdimg \
	$(MESSOBJ)/mess/tools/messroms \
	$(MESSOBJ)/mess/tools/imgtool \
	$(MESSOBJ)/mess/tools/mkimage \
	$(MESSOBJ)/mess/sound \
	$(MESSOBJ)/mess/cpu

ifeq ($(CONF_DEBUGGER),yes)
EMUDEFS += -DMAME_DEBUG
else
# don't include the debug only modules
DBGOBJS =
MESSDBGOBJS =
endif

MAMECFLAGS += \
	$(EMUCFLAGS) \
	-I$(srcdir)/src \
	-I$(srcdir)/src/includes \
	-I$(srcdir)/src/debug \
	-I$(OBJ)/cpu/m68000 \
	-I$(srcdir)/src/cpu/m68000

MESSCFLAGS += \
	$(EMUCFLAGS) \
	-I$(srcdir)/srcmess \
	-I$(srcdir)/srcmess/includes \
	-I$(srcdir)/srcmess/debug \
	-I$(MESSOBJ)/cpu/m68000 \
	-I$(srcdir)/srcmess/cpu/m68000

MAMEDEFS += $(EMUDEFS) $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS) $(ASMDEFS)
MESSDEFS += $(EMUDEFS) $(MESSCOREDEFS) $(MESSCPUDEFS) $(MESSSOUNDDEFS) $(MESSASMDEFS)

MAMELIBS += $(EMULIBS) $(DRVLIBS)
MESSLIBS += $(EMULIBS) $(MESSDRVLIBS)

$(OBJ)/advmame$(EXE): $(sort $(OBJDIRS)) $(ADVANCEOBJS) $(MAMEOBJS) $(MAMELIBS) $(ADVANCEBUILDLIBS)
	$(ECHO) $@ $(MSG)
	$(LD) $(ADVANCEOBJS) $(MAMEOBJS) $(ADVANCELDFLAGS) $(LDFLAGS) $(MAMELIBS) $(ADVANCEBUILDLIBS) $(ADVANCELIBS) $(LIBS) -o $@
ifeq ($(CONF_PERF),yes)
	$(RM) advmamep$(EXE)
	$(LN_S) $(OBJ)/advmame$(EXE) advmamep$(EXE)
else ifeq ($(CONF_DEBUG),yes)
	$(RM) advmamed$(EXE)
	$(LN_S) $(OBJ)/advmame$(EXE) advmamed$(EXE)
else
	$(RM) advmame$(EXE)
	$(LN_S) $(OBJ)/advmame$(EXE) advmame$(EXE)
endif

$(OBJ)/chdman$(EXE): $(EMUCHDMANOBJS) $(EMUCHDMANLIBS)
	$(ECHO) $@ $(MSG)
	$(LD) $(EMUCHDMANOBJS) $(EMUCHDMANLDFLAGS) $(LDFLAGS) $(EMUCHDMANLIBS) $(LIBS) -o $@

$(OBJ)/%.o: $(srcdir)/src/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(MAMECFLAGS) $(MAMEDEFS) -c $< -o $@

# Generate C source files for the 68000 emulator
$(subst .c,.o,$(M68000_GENERATED_FILES)): $(OBJ)/cpu/m68000/m68kmake$(EXE_FOR_BUILD)
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(MAMECFLAGS) $(MAMEDEFS) -c $*.c -o $@

# Additional rule, because m68kcpu.c includes the generated m68kops.h
$(OBJ)/cpu/m68000/m68kcpu.o: $(OBJ)/cpu/m68000/m68kmake$(EXE_FOR_BUILD)

$(OBJ)/cpu/m68000/m68kmake$(EXE_FOR_BUILD): $(srcdir)/src/cpu/m68000/m68kmake.c
	$(ECHO) $@
	$(CC_FOR_BUILD) $(CFLAGS_FOR_BUILD) -o $(OBJ)/cpu/m68000/m68kmake$(EXE_FOR_BUILD) $<
	@$(OBJ)/cpu/m68000/m68kmake$(EXE_FOR_BUILD) $(OBJ)/cpu/m68000 $(srcdir)/src/cpu/m68000/m68k_in.c

$(OBJ)/%.a:
	$(ECHO) $@
	$(RM) $@
	$(AR) crs $@ $^

$(OBJ):
	$(ECHO) $@
	$(MD) $@

$(sort $(OBJDIRS)):
	$(ECHO) $@
	$(MD) $@

MESSADVANCEOBJS = $(subst $(OBJ),$(MESSOBJ),$(ADVANCEOBJS))

$(MESSOBJ)/advmess$(EXE): $(sort $(MESSOBJDIRS)) $(MESSADVANCEOBJS) $(MESSOBJS) $(MESSLIBS) $(ADVANCEBUILDLIBS) 
	$(ECHO) $@ $(MSG)
	$(LD) $(MESSADVANCEOBJS) $(MESSOBJS) $(ADVANCELDFLAGS) $(LDFLAGS) $(MESSLIBS) $(ADVANCEBUILDLIBS) $(ADVANCELIBS) $(LIBS) -o $@
ifeq ($(CONF_PERF),yes)
	$(RM) advmessp$(EXE)
	$(LN_S) $(MESSOBJ)/advmess$(EXE) advmessp$(EXE)
else ifeq ($(CONF_DEBUG),yes)
	$(RM) advmessd$(EXE)
	$(LN_S) $(MESSOBJ)/advmess$(EXE) advmessd$(EXE)
else
	$(RM) advmess$(EXE)
	$(LN_S) $(MESSOBJ)/advmess$(EXE) advmess$(EXE)
endif

$(MESSOBJ)/%.o: $(srcdir)/srcmess/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(MESSCFLAGS) $(MESSDEFS) -c $< -o $@

$(MESSOBJ)/mess/%.o: $(srcdir)/mess/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(MESSCFLAGS) $(MESSDEFS) -c $< -o $@

# Generate C source files for the 68000 emulator
$(subst .c,.o,$(MESSM68000_GENERATED_FILES)): $(MESSOBJ)/cpu/m68000/m68kmake$(EXE_FOR_BUILD)
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(MESSCFLAGS) $(MESSDEFS) -c $*.c -o $@

# Additional rule, because m68kcpu.c includes the generated m68kops.h
$(MESSOBJ)/cpu/m68000/m68kcpu.o: $(MESSOBJ)/cpu/m68000/m68kmake$(EXE_FOR_BUILD)

$(MESSOBJ)/cpu/m68000/m68kmake$(EXE_FOR_BUILD): $(srcdir)/srcmess/cpu/m68000/m68kmake.c
	$(ECHO) $@
	$(CC_FOR_BUILD) $(CFLAGS_FOR_BUILD) -o $(MESSOBJ)/cpu/m68000/m68kmake$(EXE_FOR_BUILD) $<
	@$(MESSOBJ)/cpu/m68000/m68kmake$(EXE_FOR_BUILD) $(MESSOBJ)/cpu/m68000 $(srcdir)/src/cpu/m68000/m68k_in.c

$(MESSOBJ)/%.a:
	$(ECHO) $@
	$(RM) $@
	$(AR) crs $@ $^

$(MESSOBJ):
	$(ECHO) $@
	$(MD) $@

$(sort $(MESSOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

############################################################################
# EMU diff

mamedif:
	find src \( -name "*.orig" -o -name "*.ori" -o -name "*.rej" -o -name "*~" -o -name "*.bak" \)
	-diff -U 5 --new-file --recursive src.ori src > advance/advmame.dif
	ls -l advance/advmame.dif

messdif:
	find srcmess \( -name "*.orig" -o -name "*.ori" -o -name "*.rej" -o -name "*~" -o -name "*.bak" \)
	-diff -U 5 --new-file --recursive srcmess.ori srcmess > advance/advmess.dif
	ls -l advance/advmess.dif
	find mess \( -name "*.orig" -o -name "*.ori" -o -name "*.rej" -o -name "*~" -o -name "*.bak" \)
	-diff -U 5 --new-file --recursive mess.ori mess > advance/mess.dif
	ls -l advance/mess.dif

############################################################################
# EMU dist

EMU_ROOT_SRC = \
	$(CONF_SRC)

EMU_ADVANCE_SRC = \
	$(srcdir)/advance/advance.mak \
	$(srcdir)/advance/emu.mak \
	$(srcdir)/advance/menu.mak \
	$(srcdir)/advance/v.mak \
	$(srcdir)/advance/cfg.mak \
	$(srcdir)/advance/k.mak \
	$(srcdir)/advance/s.mak \
	$(srcdir)/advance/i.mak \
	$(srcdir)/advance/j.mak \
	$(srcdir)/advance/m.mak \
	$(srcdir)/advance/line.mak \
	$(srcdir)/advance/d2.mak

EMU_CONTRIB_SRC = \
	$(wildcard $(srcdir)/contrib/*)

EMU_SUPPORT_SRC += \
	$(srcdir)/support/debian.armhf \
	$(srcdir)/support/debian.i386 \
	$(srcdir)/support/event.dat \
	$(srcdir)/support/history.dat \
	$(srcdir)/support/hiscore.dat \
	$(srcdir)/support/sysinfo.dat  \
	$(srcdir)/support/cheat.dat  \
	$(srcdir)/support/category.ini \
	$(srcdir)/support/advmess.1 \
	$(srcdir)/support/advmessv.bat \
	$(srcdir)/support/advmessc.bat \
	$(srcdir)/support/advmenuv.bat \
	$(srcdir)/support/advmenuc.bat

EMU_DOC_SRC = \
	$(srcdir)/doc/advdev.d \
	$(srcdir)/doc/advmame.d \
	$(srcdir)/doc/authors.d \
	$(srcdir)/doc/script.d \
	$(srcdir)/doc/readme.d \
	$(srcdir)/doc/release.d \
	$(srcdir)/doc/history.d \
	$(srcdir)/doc/faq.d \
	$(srcdir)/doc/build.d \
	$(srcdir)/doc/cost.d \
	$(srcdir)/doc/advv.d \
	$(srcdir)/doc/advcfg.d \
	$(srcdir)/doc/advk.d \
	$(srcdir)/doc/advs.d \
	$(srcdir)/doc/advj.d \
	$(srcdir)/doc/advm.d \
	$(srcdir)/doc/advline.d \
	$(srcdir)/doc/carddos.d \
	$(srcdir)/doc/cardlinx.d \
	$(srcdir)/doc/cardwin.d \
	$(srcdir)/doc/install.d \
	$(srcdir)/doc/svgawin.d \
	$(srcdir)/doc/advdev.txt \
	$(srcdir)/doc/advmame.txt \
	$(srcdir)/doc/authors.txt \
	$(srcdir)/doc/script.txt \
	$(srcdir)/doc/advmenu.txt \
	$(srcdir)/doc/readme.txt \
	$(srcdir)/doc/release.txt \
	$(srcdir)/doc/history.txt \
	$(srcdir)/doc/faq.txt \
	$(srcdir)/doc/build.txt \
	$(srcdir)/doc/cost.txt \
	$(srcdir)/doc/advv.txt \
	$(srcdir)/doc/advcfg.txt \
	$(srcdir)/doc/advk.txt \
	$(srcdir)/doc/advs.txt \
	$(srcdir)/doc/advj.txt \
	$(srcdir)/doc/advm.txt \
	$(srcdir)/doc/advline.txt \
	$(srcdir)/doc/carddos.txt \
	$(srcdir)/doc/cardlinx.txt \
	$(srcdir)/doc/cardwin.txt \
	$(srcdir)/doc/install.txt \
	$(srcdir)/doc/svgawin.txt \
	$(srcdir)/doc/advmenu.d \
	$(srcdir)/doc/advdev.html \
	$(srcdir)/doc/advmame.html \
	$(srcdir)/doc/authors.html \
	$(srcdir)/doc/script.html \
	$(srcdir)/doc/readme.html \
	$(srcdir)/doc/release.html \
	$(srcdir)/doc/history.html \
	$(srcdir)/doc/faq.html \
	$(srcdir)/doc/build.html \
	$(srcdir)/doc/cost.html \
	$(srcdir)/doc/advv.html \
	$(srcdir)/doc/advcfg.html \
	$(srcdir)/doc/advk.html \
	$(srcdir)/doc/advs.html \
	$(srcdir)/doc/advj.html \
	$(srcdir)/doc/advm.html \
	$(srcdir)/doc/advline.html \
	$(srcdir)/doc/carddos.html \
	$(srcdir)/doc/cardlinx.html \
	$(srcdir)/doc/cardwin.html \
	$(srcdir)/doc/install.html \
	$(srcdir)/doc/svgawin.html \
	$(srcdir)/doc/advmenu.html \
	$(srcdir)/doc/advdev.1 \
	$(srcdir)/doc/advmame.1 \
	$(srcdir)/doc/advv.1 \
	$(srcdir)/doc/advcfg.1 \
	$(srcdir)/doc/advk.1 \
	$(srcdir)/doc/advs.1 \
	$(srcdir)/doc/advj.1 \
	$(srcdir)/doc/advm.1 \
	$(srcdir)/doc/advmenu.1 \

EMU_DOC_BIN = \
	$(DOCOBJ)/advdev.txt \
	$(DOCOBJ)/advmame.txt \
	$(DOCOBJ)/build.txt \
	$(DOCOBJ)/cost.txt \
	$(DOCOBJ)/authors.txt \
	$(DOCOBJ)/script.txt \
	$(DOCOBJ)/readme.txt \
	$(DOCOBJ)/release.txt \
	$(DOCOBJ)/history.txt \
	$(DOCOBJ)/faq.txt \
	$(DOCOBJ)/install.txt \
	$(DOCOBJ)/advv.txt \
	$(DOCOBJ)/advcfg.txt \
	$(DOCOBJ)/advmenu.txt \
	$(DOCOBJ)/advdev.html \
	$(DOCOBJ)/advmame.html \
	$(DOCOBJ)/build.html \
	$(DOCOBJ)/cost.html \
	$(DOCOBJ)/authors.html \
	$(DOCOBJ)/script.html \
	$(DOCOBJ)/readme.html \
	$(DOCOBJ)/release.html \
	$(DOCOBJ)/history.html \
	$(DOCOBJ)/faq.html \
	$(DOCOBJ)/install.html \
	$(DOCOBJ)/advv.html \
	$(DOCOBJ)/advcfg.html \
	$(DOCOBJ)/advmenu.html
ifeq ($(CONF_SYSTEM),unix)
EMU_DOC_BIN += \
	$(DOCOBJ)/cardlinx.txt \
	$(DOCOBJ)/advk.txt \
	$(DOCOBJ)/advs.txt \
	$(DOCOBJ)/advj.txt \
	$(DOCOBJ)/advm.txt \
	$(DOCOBJ)/cardlinx.html \
	$(DOCOBJ)/advk.html \
	$(DOCOBJ)/advs.html \
	$(DOCOBJ)/advj.html \
	$(DOCOBJ)/advm.html
endif
ifeq ($(CONF_SYSTEM),dos)
EMU_DOC_BIN += \
	$(DOCOBJ)/carddos.txt \
	$(DOCOBJ)/advk.txt \
	$(DOCOBJ)/advs.txt \
	$(DOCOBJ)/advj.txt \
	$(DOCOBJ)/advm.txt \
	$(DOCOBJ)/carddos.html \
	$(DOCOBJ)/advk.html \
	$(DOCOBJ)/advs.html \
	$(DOCOBJ)/advj.html \
	$(DOCOBJ)/advm.html
endif
ifeq ($(CONF_SYSTEM),windows)
EMU_DOC_BIN += \
	$(DOCOBJ)/svgawin.txt \
	$(DOCOBJ)/cardwin.txt \
	$(DOCOBJ)/svgawin.html \
	$(DOCOBJ)/cardwin.html
endif

EMU_ROOT_BIN = \
	$(OBJ)/advmame$(EXE) \
	$(MESSOBJ)/advmess$(EXE) \
	$(MENUOBJ)/advmenu$(EXE)
EMU_ROOT_BIN += \
	$(srcdir)/support/event.dat \
	$(srcdir)/support/history.dat \
	$(srcdir)/support/hiscore.dat \
	$(srcdir)/support/sysinfo.dat \
	$(srcdir)/support/cheat.dat \
	$(srcdir)/support/category.ini
ifeq ($(CONF_SYSTEM),unix)
EMU_ROOT_BIN += \
	$(VOBJ)/advv$(EXE) \
	$(CFGOBJ)/advcfg$(EXE) \
	$(KOBJ)/advk$(EXE) \
	$(SOBJ)/advs$(EXE) \
	$(JOBJ)/advj$(EXE) \
	$(MOBJ)/advm$(EXE) \
	$(DOCOBJ)/advmame.1 \
	$(DOCOBJ)/advmenu.1 \
	$(DOCOBJ)/advdev.1 \
	$(DOCOBJ)/advv.1 \
	$(DOCOBJ)/advcfg.1 \
	$(DOCOBJ)/advk.1 \
	$(DOCOBJ)/advs.1 \
	$(DOCOBJ)/advj.1 \
	$(DOCOBJ)/advm.1
EMU_ROOT_BIN += \
	$(srcdir)/support/advmess.1
endif
ifeq ($(CONF_SYSTEM),dos)
EMU_ROOT_BIN += \
	$(srcdir)/support/cwsdpmi.exe \
	$(VOBJ)/advv$(EXE) \
	$(CFGOBJ)/advcfg$(EXE) \
	$(KOBJ)/advk$(EXE) \
	$(SOBJ)/advs$(EXE) \
	$(JOBJ)/advj$(EXE) \
	$(MOBJ)/advm$(EXE) \
	$(srcdir)/support/advmessv.bat \
	$(srcdir)/support/advmessc.bat \
	$(srcdir)/support/advmenuv.bat \
	$(srcdir)/support/advmenuc.bat
endif
ifeq ($(CONF_SYSTEM),windows)
ifeq ($(CONF_LIB_DIRECT),yes)
EMU_ROOT_BIN += \
	$(VOBJ)/advv$(EXE) \
	$(CFGOBJ)/advcfg$(EXE) \
	$(srcdir)/advance/svgalib/svgawin/driver/svgawin.sys \
	$(srcdir)/advance/svgalib/svgawin/install/svgawin.exe \
	$(srcdir)/support/advmessv.bat \
	$(srcdir)/support/advmessc.bat \
	$(srcdir)/support/advmenuv.bat \
	$(srcdir)/support/advmenuc.bat
endif
endif

EMU_DIST_FILE_SRC = advancemame-$(VERSION)
EMU_DIST_FILE_BIN = advancemame-$(VERSION)-$(BINARYTAG)
EMU_DIST_DIR_SRC = $(EMU_DIST_FILE_SRC)
EMU_DIST_DIR_BIN = $(EMU_DIST_FILE_BIN)

dist: $(DOCOBJ)/readme.txt $(DOCOBJ)/release.txt $(DOCOBJ)/history.txt $(DOCOBJ)/build.txt
	mkdir $(EMU_DIST_DIR_SRC)
	$(srcdir)/autover.sh > $(EMU_DIST_DIR_SRC)/.version
	cp $(DOCOBJ)/readme.txt $(EMU_DIST_DIR_SRC)/README
	cp $(DOCOBJ)/release.txt $(EMU_DIST_DIR_SRC)/RELEASE
	cp $(DOCOBJ)/history.txt $(EMU_DIST_DIR_SRC)/HISTORY
	cp $(DOCOBJ)/build.txt $(EMU_DIST_DIR_SRC)/BUILD
	cp $(srcdir)/COPYING $(EMU_DIST_DIR_SRC)/COPYING
	cp $(EMU_ROOT_SRC) $(EMU_DIST_DIR_SRC)
	mkdir $(EMU_DIST_DIR_SRC)/doc
	cp $(EMU_DOC_SRC) $(EMU_DIST_DIR_SRC)/doc
	mkdir $(EMU_DIST_DIR_SRC)/advance
	cp $(EMU_ADVANCE_SRC) $(EMU_DIST_DIR_SRC)/advance
	mkdir $(EMU_DIST_DIR_SRC)/support
	cp $(EMU_SUPPORT_SRC) $(EMU_DIST_DIR_SRC)/support
	cp -R $(srcdir)/support/free $(EMU_DIST_DIR_SRC)/support
	mkdir $(EMU_DIST_DIR_SRC)/advance/linux
	cp $(LINUX_SRC) $(EMU_DIST_DIR_SRC)/advance/linux
	mkdir $(EMU_DIST_DIR_SRC)/advance/dos
	cp $(DOS_SRC) $(EMU_DIST_DIR_SRC)/advance/dos
	mkdir $(EMU_DIST_DIR_SRC)/advance/windows
	cp $(WINDOWS_SRC) $(EMU_DIST_DIR_SRC)/advance/windows
	mkdir $(EMU_DIST_DIR_SRC)/advance/sdl
	cp $(SDL_SRC) $(EMU_DIST_DIR_SRC)/advance/sdl
	mkdir $(EMU_DIST_DIR_SRC)/advance/osd
	cp $(SRCOSD) $(EMU_DIST_DIR_SRC)/advance/osd
	mkdir $(EMU_DIST_DIR_SRC)/advance/lib
	cp $(LIB_SRC) $(EMU_DIST_DIR_SRC)/advance/lib
	mkdir $(EMU_DIST_DIR_SRC)/advance/blit
	cp $(BLIT_SRC) $(EMU_DIST_DIR_SRC)/advance/blit
	mkdir $(EMU_DIST_DIR_SRC)/advance/card
	cp $(CARD_SRC) $(EMU_DIST_DIR_SRC)/advance/card
	mkdir $(EMU_DIST_DIR_SRC)/advance/svgalib
	cp $(SVGALIB_SRC) $(EMU_DIST_DIR_SRC)/advance/svgalib
	mkdir $(EMU_DIST_DIR_SRC)/advance/svgalib/clockchi
	cp $(SVGALIBCLOCKCHI_SRC) $(EMU_DIST_DIR_SRC)/advance/svgalib/clockchi
	mkdir $(EMU_DIST_DIR_SRC)/advance/svgalib/ramdac
	cp $(SVGALIBRAMDAC_SRC) $(EMU_DIST_DIR_SRC)/advance/svgalib/ramdac
	mkdir $(EMU_DIST_DIR_SRC)/advance/svgalib/drivers
	cp $(SVGALIBDRIVERS_SRC) $(EMU_DIST_DIR_SRC)/advance/svgalib/drivers
	mkdir $(EMU_DIST_DIR_SRC)/advance/svgalib/svgados
	cp $(SVGALIBSVGADOS_SRC) $(EMU_DIST_DIR_SRC)/advance/svgalib/svgados
	mkdir $(EMU_DIST_DIR_SRC)/advance/svgalib/svgawin
	cp $(SVGALIBSVGAWIN_SRC) $(EMU_DIST_DIR_SRC)/advance/svgalib/svgawin
	mkdir $(EMU_DIST_DIR_SRC)/advance/svgalib/svgawin/sys
	cp $(SVGALIBSVGAWINSYS_SRC) $(EMU_DIST_DIR_SRC)/advance/svgalib/svgawin/sys
	mkdir $(EMU_DIST_DIR_SRC)/advance/svgalib/svgawin/install
	cp $(SVGALIBSVGAWININSTALL_SRC) $(EMU_DIST_DIR_SRC)/advance/svgalib/svgawin/install
	mkdir $(EMU_DIST_DIR_SRC)/advance/svgalib/svgawin/driver
	cp $(SVGALIBSVGAWINDRIVER_SRC) $(EMU_DIST_DIR_SRC)/advance/svgalib/svgawin/driver
	mkdir $(EMU_DIST_DIR_SRC)/advance/mpglib
	cp $(MPGLIB_SRC) $(EMU_DIST_DIR_SRC)/advance/mpglib
	mkdir $(EMU_DIST_DIR_SRC)/advance/v
	cp $(V_SRC) $(EMU_DIST_DIR_SRC)/advance/v
	mkdir $(EMU_DIST_DIR_SRC)/advance/k
	cp $(K_SRC) $(EMU_DIST_DIR_SRC)/advance/k
	mkdir $(EMU_DIST_DIR_SRC)/advance/j
	cp $(J_SRC) $(EMU_DIST_DIR_SRC)/advance/j
	mkdir $(EMU_DIST_DIR_SRC)/advance/m
	cp $(M_SRC) $(EMU_DIST_DIR_SRC)/advance/m
	mkdir $(EMU_DIST_DIR_SRC)/advance/s
	cp $(S_SRC) $(EMU_DIST_DIR_SRC)/advance/s
	mkdir $(EMU_DIST_DIR_SRC)/advance/i
	cp $(I_SRC) $(EMU_DIST_DIR_SRC)/advance/i
	mkdir $(EMU_DIST_DIR_SRC)/advance/cfg
	cp $(CFG_SRC) $(EMU_DIST_DIR_SRC)/advance/cfg
	mkdir $(EMU_DIST_DIR_SRC)/advance/menu
	cp $(MENU_SRC) $(EMU_DIST_DIR_SRC)/advance/menu
	mkdir $(EMU_DIST_DIR_SRC)/advance/line
	cp $(LINE_SRC) $(EMU_DIST_DIR_SRC)/advance/line
	mkdir $(EMU_DIST_DIR_SRC)/advance/d2
	cp $(D2_SRC) $(EMU_DIST_DIR_SRC)/advance/d2
	mkdir $(EMU_DIST_DIR_SRC)/advance/zlib
	cp $(ZLIB_SRC) $(EMU_DIST_DIR_SRC)/advance/zlib
	mkdir $(EMU_DIST_DIR_SRC)/advance/expat
	cp $(EXPAT_SRC) $(EMU_DIST_DIR_SRC)/advance/expat
	mkdir $(EMU_DIST_DIR_SRC)/contrib
ifneq ($(EMU_CONTRIB_SRC),)
	cp -R $(EMU_CONTRIB_SRC) $(EMU_DIST_DIR_SRC)/contrib
endif
	cp -R $(srcdir)/src $(EMU_DIST_DIR_SRC)
	cp -R $(srcdir)/srcmess $(EMU_DIST_DIR_SRC)
	cp -R $(srcdir)/mess $(EMU_DIST_DIR_SRC)
	find $(EMU_DIST_DIR_SRC) \( -name "*.dat" \) -type f -exec dos2unix {} \;
	rm -f $(EMU_DIST_FILE_SRC).tar.gz
	tar cfzo $(EMU_DIST_FILE_SRC).tar.gz $(EMU_DIST_DIR_SRC)
	rm -r $(EMU_DIST_DIR_SRC)

distbin: $(EMU_ROOT_BIN) $(EMU_DOC_BIN)
	mkdir $(EMU_DIST_DIR_BIN)
ifeq ($(CONF_SYSTEM),unix)
	cp $(DOCOBJ)/readme.txt $(EMU_DIST_DIR_BIN)/README
	cp $(DOCOBJ)/release.txt $(EMU_DIST_DIR_BIN)/RELEASE
	cp $(DOCOBJ)/history.txt $(EMU_DIST_DIR_BIN)/HISTORY
	cp $(srcdir)/COPYING $(EMU_DIST_DIR_BIN)/COPYING
else
	cp $(DOCOBJ)/readme.txt $(EMU_DIST_DIR_BIN)/readme.txt
	cp $(DOCOBJ)/release.txt $(EMU_DIST_DIR_BIN)/release.txt
	cp $(DOCOBJ)/history.txt $(EMU_DIST_DIR_BIN)/history.txt
	cp $(srcdir)/COPYING $(EMU_DIST_DIR_BIN)/copying.txt
endif
	cp $(EMU_ROOT_BIN) $(EMU_DIST_DIR_BIN)
	cp -R $(srcdir)/support/free/rom $(EMU_DIST_DIR_BIN)
	cp -R $(srcdir)/support/free/image $(EMU_DIST_DIR_BIN)
	cp -R $(srcdir)/support/free/sample $(EMU_DIST_DIR_BIN)
	cp -R $(srcdir)/support/free/snap $(EMU_DIST_DIR_BIN)
	mkdir $(EMU_DIST_DIR_BIN)/doc
	cp $(EMU_DOC_BIN) $(EMU_DIST_DIR_BIN)/doc
	mkdir $(EMU_DIST_DIR_BIN)/contrib
	cp -R $(EMU_CONTRIB_SRC) $(EMU_DIST_DIR_BIN)/contrib
ifeq ($(CONF_SYSTEM),unix)
	find $(EMU_DIST_DIR_BIN) \( -name "*.dat" \) -type f -exec dos2unix {} \;
	rm -f $(EMU_DIST_FILE_BIN).tar.gz
	tar cfzo $(EMU_DIST_FILE_BIN).tar.gz $(EMU_DIST_DIR_BIN)
else
	find $(EMU_DIST_DIR_BIN) \( -name "*.txt" \) -type f -exec unix2dos {} \;
	find $(EMU_DIST_DIR_BIN) \( -name "*.bat" \) -type f -exec unix2dos {} \;
	find $(EMU_DIST_DIR_BIN) \( -name "*.dat" \) -type f -exec unix2dos {} \;
	rm -f $(EMU_DIST_FILE_BIN).zip
	cd $(EMU_DIST_DIR_BIN) && zip -r ../$(EMU_DIST_FILE_BIN).zip *
endif
	rm -r $(EMU_DIST_DIR_BIN)

