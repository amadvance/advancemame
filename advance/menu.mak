############################################################################
# Menu build

$(MENUOBJ)/menu/mm.o: $(srcdir)/advance/advance.mak
MENUCFLAGS += -DVERSION=\"$(MENUVERSION)\"

MENUCFLAGS += \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/blit \
	-I$(srcdir)/advance/mpglib \
	-I$(srcdir)/advance/expat
MENUOBJDIRS += \
	$(MENUOBJ) \
	$(MENUOBJ)/menu \
	$(MENUOBJ)/lib \
	$(MENUOBJ)/blit \
	$(MENUOBJ)/mpglib \
	$(MENUOBJ)/expat
MENUOBJS += \
	$(MENUOBJ)/menu/category.o \
	$(MENUOBJ)/menu/choice.o \
	$(MENUOBJ)/menu/common.o \
	$(MENUOBJ)/menu/crc.o \
	$(MENUOBJ)/menu/emulator.o \
	$(MENUOBJ)/menu/emuxml.o \
	$(MENUOBJ)/menu/game.o \
	$(MENUOBJ)/menu/mconfig.o \
	$(MENUOBJ)/menu/menu.o \
	$(MENUOBJ)/menu/submenu.o \
	$(MENUOBJ)/menu/mm.o \
	$(MENUOBJ)/menu/play.o \
	$(MENUOBJ)/menu/playdef.o \
	$(MENUOBJ)/menu/resource.o \
	$(MENUOBJ)/menu/text.o \
	$(MENUOBJ)/lib/portable.o \
	$(MENUOBJ)/lib/log.o \
	$(MENUOBJ)/lib/mng.o \
	$(MENUOBJ)/lib/bitmap.o \
	$(MENUOBJ)/lib/fz.o \
	$(MENUOBJ)/lib/unzip.o \
	$(MENUOBJ)/lib/mixer.o \
	$(MENUOBJ)/lib/wave.o \
	$(MENUOBJ)/lib/png.o \
	$(MENUOBJ)/lib/pcx.o \
	$(MENUOBJ)/lib/icon.o \
	$(MENUOBJ)/lib/fontdef.o \
	$(MENUOBJ)/lib/font.o \
	$(MENUOBJ)/lib/video.o \
	$(MENUOBJ)/lib/rgb.o \
	$(MENUOBJ)/lib/conf.o \
	$(MENUOBJ)/lib/key.o \
	$(MENUOBJ)/lib/incstr.o \
	$(MENUOBJ)/lib/videoio.o \
	$(MENUOBJ)/lib/update.o \
	$(MENUOBJ)/lib/generate.o \
	$(MENUOBJ)/lib/crtc.o \
	$(MENUOBJ)/lib/crtcbag.o \
	$(MENUOBJ)/lib/monitor.o \
	$(MENUOBJ)/lib/device.o \
	$(MENUOBJ)/lib/sounddrv.o \
	$(MENUOBJ)/lib/snone.o \
	$(MENUOBJ)/lib/keydrv.o \
	$(MENUOBJ)/lib/keyall.o \
	$(MENUOBJ)/lib/knone.o \
	$(MENUOBJ)/lib/mousedrv.o \
	$(MENUOBJ)/lib/mouseall.o \
	$(MENUOBJ)/lib/mnone.o \
	$(MENUOBJ)/lib/joydrv.o \
	$(MENUOBJ)/lib/joyall.o \
	$(MENUOBJ)/lib/jnone.o \
	$(MENUOBJ)/lib/readinfo.o \
	$(MENUOBJ)/lib/soundall.o \
	$(MENUOBJ)/lib/videoall.o \
	$(MENUOBJ)/lib/vnone.o \
	$(MENUOBJ)/lib/error.o \
	$(MENUOBJ)/blit/clear.o \
	$(MENUOBJ)/blit/blit.o \
	$(MENUOBJ)/blit/slice.o \
	$(MENUOBJ)/mpglib/interfac.o \
	$(MENUOBJ)/mpglib/internal.o \
	$(MENUOBJ)/mpglib/decode.o \
	$(MENUOBJ)/mpglib/dct64.o \
	$(MENUOBJ)/mpglib/layer3.o \
	$(MENUOBJ)/mpglib/tabinit.o \
	$(MENUOBJ)/expat/xmlrole.o \
	$(MENUOBJ)/expat/xmltok.o \
	$(MENUOBJ)/expat/xmlparse.o
MENULIBS += $(ZLIBS)

ifeq ($(CONF_HOST),unix)
MENUOBJDIRS += \
	$(MENUOBJ)/linux
MENUCFLAGS +=  \
	-DDATADIR=\"$(DATADIR)\" \
	-I$(srcdir)/advance/linux
MENUCFLAGS += \
	-DUSE_VIDEO_NONE -DUSE_VIDEO_RESTORE \
	-DUSE_SOUND_NONE \
	-DUSE_KEYBOARD_NONE \
	-DUSE_MOUSE_NONE \
	-DUSE_JOYSTICK_NONE
MENUOBJS += \
	$(MENUOBJ)/lib/filenix.o \
	$(MENUOBJ)/lib/targnix.o \
	$(MENUOBJ)/linux/os.o
ifeq ($(CONF_LIB_SVGALIB),yes)
MENUCFLAGS += \
	-DUSE_VIDEO_SVGALIB \
	-DUSE_KEYBOARD_SVGALIB \
	-DUSE_MOUSE_SVGALIB \
	-DUSE_JOYSTICK_SVGALIB
MENULIBS += -lvga
MENUOBJS += \
	$(MENUOBJ)/linux/vsvgab.o \
	$(MENUOBJ)/linux/ksvgab.o \
	$(MENUOBJ)/linux/msvgab.o \
	$(MENUOBJ)/linux/jsvgab.o
endif
ifeq ($(CONF_LIB_FB),yes)
MENUCFLAGS += \
	-DUSE_VIDEO_FB
MENUOBJS += \
	$(MENUOBJ)/linux/vfb.o
endif
ifeq ($(CONF_LIB_ALSA),yes)
MENUCFLAGS += \
	-DUSE_SOUND_ALSA
MENUOBJS += \
	$(MENUOBJ)/linux/salsa.o
MENULIBS += -lasound
endif
ifeq ($(CONF_LIB_OSS),yes)
MENUCFLAGS += \
	-DUSE_SOUND_OSS
MENUOBJS += \
	$(MENUOBJ)/linux/soss.o
endif
ifeq ($(CONF_LIB_KRAW),yes)
MENUCFLAGS += \
	-DUSE_KEYBOARD_RAW
MENUOBJS += \
	$(MENUOBJ)/linux/kraw.o
endif
ifeq ($(CONF_LIB_SDL),yes)
MENUCFLAGS += \
	$(SDLCFLAGS) \
	-I$(srcdir)/advance/sdl \
	-DUSE_VIDEO_SDL \
	-DUSE_SOUND_SDL \
	-DUSE_KEYBOARD_SDL \
	-DUSE_MOUSE_SDL \
	-DUSE_JOYSTICK_SDL
MENULIBS += $(SDLLIBS)
MENUOBJDIRS += \
	$(MENUOBJ)/sdl
MENUOBJS += \
	$(MENUOBJ)/sdl/vsdl.o \
	$(MENUOBJ)/sdl/ssdl.o \
	$(MENUOBJ)/sdl/ksdl.o \
	$(MENUOBJ)/sdl/msdl.o \
	$(MENUOBJ)/sdl/jsdl.o
endif
endif

ifeq ($(CONF_HOST),dos)
MENUCFLAGS += \
	-DUSE_ADV_SVGALIB_DOS \
	-I$(srcdir)/advance/dos \
	-I$(srcdir)/advance/card \
	-I$(srcdir)/advance/svgalib \
	-I$(srcdir)/advance/svgalib/clockchi \
	-I$(srcdir)/advance/svgalib/ramdac \
	-I$(srcdir)/advance/svgalib/drivers \
	-I$(srcdir)/advance/svgalib/svgados \
	-DUSE_VIDEO_SVGALINE -DUSE_VIDEO_VBELINE -DUSE_VIDEO_VGALINE -DUSE_VIDEO_VBE -DUSE_VIDEO_NONE \
	-DUSE_SOUND_SEAL -DUSE_SOUND_ALLEGRO -DUSE_SOUND_VSYNC -DUSE_SOUND_NONE -DUSE_SOUND_INT \
	-DUSE_KEYBOARD_ALLEGRO -DUSE_KEYBOARD_NONE \
	-DUSE_MOUSE_ALLEGRO -DUSE_MOUSE_NONE \
	-DUSE_JOYSTICK_ALLEGRO -DUSE_JOYSTICK_NONE
MENULDFLAGS += -Xlinker --wrap -Xlinker _mixer_init
MENUCFLAGS += -DUSE_CONFIG_ALLEGRO_WRAPPER
MENULDFLAGS += \
	-Xlinker --wrap -Xlinker get_config_string \
	-Xlinker --wrap -Xlinker get_config_int \
	-Xlinker --wrap -Xlinker set_config_string \
	-Xlinker --wrap -Xlinker set_config_int \
	-Xlinker --wrap -Xlinker get_config_id \
	-Xlinker --wrap -Xlinker set_config_id
MENULIBS += -lalleg -laudio
MENUOBJDIRS += \
	$(MENUOBJ)/dos
MENUOBJS += \
	$(MENUOBJ)/lib/filedos.o \
	$(MENUOBJ)/lib/targdos.o \
	$(MENUOBJ)/dos/os.o \
	$(MENUOBJ)/dos/vvgal.o \
	$(MENUOBJ)/dos/vvbe.o \
	$(MENUOBJ)/dos/vvbel.o \
	$(MENUOBJ)/dos/vsvgal.o \
	$(MENUOBJ)/dos/scrvbe.o \
	$(MENUOBJ)/dos/scrvga.o \
	$(MENUOBJ)/dos/salleg.o \
	$(MENUOBJ)/dos/sseal.o \
	$(MENUOBJ)/dos/svsync.o \
	$(MENUOBJ)/dos/malleg.o \
	$(MENUOBJ)/dos/kalleg.o \
	$(MENUOBJ)/dos/jalleg.o \
	$(MENUOBJ)/card/card.o \
	$(MENUOBJ)/dos/pci.o \
	$(MENUOBJ)/dos/pcimap.o \
	$(MENUOBJ)/dos/map.o \
	$(MENUOBJ)/card/board.o \
	$(MENUOBJ)/svgalib/svgalib.o \
	$(MENUOBJ)/svgalib/svgados/svgados.o \
	$(MENUOBJ)/svgalib/accel.o \
	$(MENUOBJ)/svgalib/vgaio.o \
	$(MENUOBJ)/svgalib/vgammvga.o \
	$(MENUOBJ)/svgalib/vgaregs.o \
	$(MENUOBJ)/svgalib/vgarelvg.o \
	$(MENUOBJ)/svgalib/drivers/apm.o \
	$(MENUOBJ)/svgalib/drivers/ark.o \
	$(MENUOBJ)/svgalib/drivers/banshee.o \
	$(MENUOBJ)/svgalib/drivers/et6000.o \
	$(MENUOBJ)/svgalib/drivers/g400.o \
	$(MENUOBJ)/svgalib/drivers/pm2.o \
	$(MENUOBJ)/svgalib/drivers/i740.o \
	$(MENUOBJ)/svgalib/drivers/laguna.o \
	$(MENUOBJ)/svgalib/drivers/millenni.o \
	$(MENUOBJ)/svgalib/drivers/mx.o \
	$(MENUOBJ)/svgalib/drivers/nv3.o \
	$(MENUOBJ)/svgalib/drivers/r128.o \
	$(MENUOBJ)/svgalib/drivers/rage.o \
	$(MENUOBJ)/svgalib/drivers/s3.o \
	$(MENUOBJ)/svgalib/drivers/savage.o \
	$(MENUOBJ)/svgalib/drivers/sis.o \
	$(MENUOBJ)/svgalib/drivers/trident.o \
	$(MENUOBJ)/svgalib/drivers/renditio.o \
	$(MENUOBJ)/svgalib/ramdac/ibmrgb52.o \
	$(MENUOBJ)/svgalib/ramdac/attdacs.o \
	$(MENUOBJ)/svgalib/ramdac/icw.o \
	$(MENUOBJ)/svgalib/ramdac/normal.o \
	$(MENUOBJ)/svgalib/ramdac/ramdac.o \
	$(MENUOBJ)/svgalib/ramdac/s3dacs.o \
	$(MENUOBJ)/svgalib/ramdac/sierra.o \
	$(MENUOBJ)/svgalib/ramdac/btdacs.o \
	$(MENUOBJ)/svgalib/ramdac/ics_gend.o \
	$(MENUOBJ)/svgalib/clockchi/icd2061a.o
MENUOBJDIRS += \
	$(MENUOBJ)/card \
	$(MENUOBJ)/svgalib \
	$(MENUOBJ)/svgalib/ramdac \
	$(MENUOBJ)/svgalib/clockchi \
	$(MENUOBJ)/svgalib/drivers \
	$(MENUOBJ)/svgalib/svgados
endif

ifeq ($(CONF_HOST),windows)
MENUCFLAGS += \
	-I$(srcdir)/advance/windows \
	-DUSE_VIDEO_RESTORE -DUSE_VIDEO_NONE \
	-DUSE_SOUND_NONE \
	-DUSE_KEYBOARD_NONE \
	-DUSE_MOUSE_NONE \
	-DUSE_JOYSTICK_NONE
MENUOBJDIRS += \
	$(MENUOBJ)/windows
MENUOBJS += \
	$(MENUOBJ)/lib/filedos.o \
	$(MENUOBJ)/lib/targwin.o \
	$(MENUOBJ)/lib/icondef.o \
	$(MENUOBJ)/windows/os.o
ifeq ($(CONF_LIB_SDL),yes)
MENUOBJDIRS += \
	$(MENUOBJ)/sdl
MENUCFLAGS += \
	$(SDLCFLAGS) \
	-I$(srcdir)/advance/sdl \
	-DUSE_VIDEO_SDL \
	-DUSE_SOUND_SDL \
	-DUSE_KEYBOARD_SDL \
	-DUSE_MOUSE_SDL \
	-DUSE_JOYSTICK_SDL
MENULIBS += $(SDLLIBS)
MENUOBJS += \
	$(MENUOBJ)/sdl/vsdl.o \
	$(MENUOBJ)/sdl/ssdl.o \
	$(MENUOBJ)/sdl/ksdl.o \
	$(MENUOBJ)/sdl/msdl.o \
	$(MENUOBJ)/sdl/jsdl.o
# Customize the SDL_main function
MENUCFLAGS += -DNO_STDIO_REDIRECT
MENUOBJS += $(MENUOBJ)/windows/sdlmain.o
endif
ifeq ($(CONF_LIB_SVGAWIN),yes)
MENUOBJDIRS += \
	$(MENUOBJ)/svgalib \
	$(MENUOBJ)/svgalib/ramdac \
	$(MENUOBJ)/svgalib/clockchi \
	$(MENUOBJ)/svgalib/drivers \
	$(MENUOBJ)/svgalib/svgawin
MENUCFLAGS += \
	-DUSE_ADV_SVGALIB_WIN \
	-I$(srcdir)/advance/svgalib \
	-I$(srcdir)/advance/svgalib/clockchi \
	-I$(srcdir)/advance/svgalib/ramdac \
	-I$(srcdir)/advance/svgalib/drivers \
	-I$(srcdir)/advance/svgalib/svgawin \
	-DUSE_VIDEO_SVGAWIN
MENUOBJS += \
	$(MENUOBJ)/windows/vsvgawin.o \
	$(MENUOBJ)/svgalib/svgalib.o \
	$(MENUOBJ)/svgalib/svgawin/svgawin.o \
	$(MENUOBJ)/svgalib/accel.o \
	$(MENUOBJ)/svgalib/vgaio.o \
	$(MENUOBJ)/svgalib/vgammvga.o \
	$(MENUOBJ)/svgalib/vgaregs.o \
	$(MENUOBJ)/svgalib/vgarelvg.o \
	$(MENUOBJ)/svgalib/drivers/apm.o \
	$(MENUOBJ)/svgalib/drivers/ark.o \
	$(MENUOBJ)/svgalib/drivers/banshee.o \
	$(MENUOBJ)/svgalib/drivers/et6000.o \
	$(MENUOBJ)/svgalib/drivers/g400.o \
	$(MENUOBJ)/svgalib/drivers/pm2.o \
	$(MENUOBJ)/svgalib/drivers/i740.o \
	$(MENUOBJ)/svgalib/drivers/laguna.o \
	$(MENUOBJ)/svgalib/drivers/millenni.o \
	$(MENUOBJ)/svgalib/drivers/mx.o \
	$(MENUOBJ)/svgalib/drivers/nv3.o \
	$(MENUOBJ)/svgalib/drivers/r128.o \
	$(MENUOBJ)/svgalib/drivers/rage.o \
	$(MENUOBJ)/svgalib/drivers/s3.o \
	$(MENUOBJ)/svgalib/drivers/savage.o \
	$(MENUOBJ)/svgalib/drivers/sis.o \
	$(MENUOBJ)/svgalib/drivers/trident.o \
	$(MENUOBJ)/svgalib/drivers/renditio.o \
	$(MENUOBJ)/svgalib/ramdac/ibmrgb52.o \
	$(MENUOBJ)/svgalib/ramdac/attdacs.o \
	$(MENUOBJ)/svgalib/ramdac/icw.o \
	$(MENUOBJ)/svgalib/ramdac/normal.o \
	$(MENUOBJ)/svgalib/ramdac/ramdac.o \
	$(MENUOBJ)/svgalib/ramdac/s3dacs.o \
	$(MENUOBJ)/svgalib/ramdac/sierra.o \
	$(MENUOBJ)/svgalib/ramdac/btdacs.o \
	$(MENUOBJ)/svgalib/ramdac/ics_gend.o \
	$(MENUOBJ)/svgalib/clockchi/icd2061a.o
endif
endif

ifeq ($(CONF_MAP),yes)
MENULDFLAGS += -Xlinker -Map -Xlinker $(MENUOBJ)/advmenu.map
endif

$(MENUOBJ)/%.o: $(srcdir)/advance/%.cc
	$(ECHO) $@ $(MSG)
	$(CXX) $(CFLAGS) $(MENUCFLAGS) -c $< -o $@

$(MENUOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(MENUCFLAGS) -c $< -o $@

$(MENUOBJ)/%.o: $(srcdir)/advance/%.rc
	$(ECHO) $@ $(MSG)
	$(RC) $(RCFLAGS) $< -o $@

$(sort $(MENUOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(MENUOBJ)/advmenu$(EXE) : $(sort $(MENUOBJDIRS)) $(MENUOBJS)
	$(ECHO) $@ $(MSG)
	$(LDXX) $(LDFLAGS) $(MENULDFLAGS) $(MENUOBJS) $(MENULIBS) -o $@
ifeq ($(CONF_COMPRESS),yes)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advmenu$(EXE)
	$(LN_S) $@ advmenu$(EXE)

############################################################################
# MENU dist

MENU_ROOT_SRC = \
	$(srcdir)/COPYING \
	$(CONF_SRC)

MENU_SRC = \
	$(wildcard $(srcdir)/advance/menu/*.c) \
	$(wildcard $(srcdir)/advance/menu/*.cc) \
	$(wildcard $(srcdir)/advance/menu/*.h) \
	$(wildcard $(srcdir)/advance/menu/*.dat)

MENU_ADVANCE_SRC = \
	$(srcdir)/advance/advance.mak \
	$(srcdir)/advance/menu.mak \
	$(srcdir)/advance/v.mak \
	$(srcdir)/advance/cfg.mak \
	$(srcdir)/advance/d2.mak

MENU_CONTRIB_SRC = \
	$(wildcard $(srcdir)/contrib/menu/*)

MENU_DOC_SRC = \
	$(srcdir)/doc/license.d \
	$(srcdir)/doc/advmenu.d \
	$(srcdir)/doc/authors.d \
	$(srcdir)/doc/faq.d \
	$(srcdir)/doc/histmenu.d \
	$(srcdir)/doc/readmenu.d \
	$(srcdir)/doc/relemenu.d \
	$(srcdir)/doc/advv.d \
	$(srcdir)/doc/advcfg.d \
	$(srcdir)/doc/install.d \
	$(srcdir)/doc/carddos.d \
	$(srcdir)/doc/cardlinx.d \
	$(srcdir)/doc/build.d \
	$(srcdir)/doc/cost.d

MENU_SUPPORT_SRC = \
	$(srcdir)/support/advmenuv.bat \
	$(srcdir)/support/advmenuc.bat

MENU_DOC_BIN = \
	$(DOCOBJ)/advmenu.txt \
	$(DOCOBJ)/build.txt \
	$(DOCOBJ)/cost.txt \
	$(DOCOBJ)/authors.txt \
	$(DOCOBJ)/faq.txt \
	$(DOCOBJ)/readmenu.txt \
	$(DOCOBJ)/relemenu.txt \
	$(DOCOBJ)/histmenu.txt \
	$(DOCOBJ)/advv.txt \
	$(DOCOBJ)/advcfg.txt \
	$(DOCOBJ)/install.txt \
	$(DOCOBJ)/advmenu.html \
	$(DOCOBJ)/build.html \
	$(DOCOBJ)/cost.html \
	$(DOCOBJ)/authors.html \
	$(DOCOBJ)/faq.html \
	$(DOCOBJ)/readmenu.html \
	$(DOCOBJ)/relemenu.html \
	$(DOCOBJ)/histmenu.html \
	$(DOCOBJ)/histmenu.html \
	$(DOCOBJ)/advv.html \
	$(DOCOBJ)/advcfg.html \
	$(DOCOBJ)/install.html
ifeq ($(CONF_HOST),unix)
MENU_DOC_BIN += \
	$(DOCOBJ)/cardlinx.txt \
	$(DOCOBJ)/cardlinx.html
endif
ifeq ($(CONF_HOST),dos)
MENU_DOC_BIN += \
	$(DOCOBJ)/carddos.txt \
	$(DOCOBJ)/carddos.html
endif
ifeq ($(CONF_HOST),windows)
MENU_DOC_BIN += \
	$(DOCOBJ)/svgawin.txt \
	$(DOCOBJ)/cardwin.txt \
	$(DOCOBJ)/svgawin.html \
	$(DOCOBJ)/cardwin.html
endif

MENU_ROOT_BIN = \
	$(MENUOBJ)/advmenu$(EXE) \
	$(VOBJ)/advv$(EXE) \
	$(CFGOBJ)/advcfg$(EXE)
ifeq ($(CONF_HOST),unix)
MENU_ROOT_BIN += \
	$(DOCOBJ)/advmenu.1 \
	$(DOCOBJ)/advv.1 \
	$(DOCOBJ)/advcfg.1 \
	$(CONF_BIN)
endif
ifeq ($(CONF_HOST),dos)
MENU_ROOT_BIN += \
	$(srcdir)/support/cwsdpmi.exe \
	$(srcdir)/support/advmenuv.bat \
	$(srcdir)/support/advmenuc.bat
endif
ifeq ($(CONF_HOST),windows)
MENU_ROOT_BIN += \
	$(srcdir)/advance/svgalib/svgawin/driver/svgawin.sys \
	$(srcdir)/advance/svgalib/svgawin/install/svgawin.exe \
	$(srcdir)/support/sdl.dll \
	$(srcdir)/support/zlib.dll \
	$(srcdir)/support/advmenuv.bat \
	$(srcdir)/support/advmenuc.bat
endif

MENU_DIST_FILE_SRC = advancemenu-$(MENUVERSION)
MENU_DIST_FILE_BIN = advancemenu-$(MENUVERSION)-$(BINARYTAG)
MENU_DIST_DIR_SRC = $(MENU_DIST_FILE_SRC)
MENU_DIST_DIR_BIN = $(MENU_DIST_FILE_BIN)

distmenu: $(DOCOBJ)/readmenu.txt $(DOCOBJ)/relemenu.txt $(DOCOBJ)/histmenu.txt $(DOCOBJ)/build.txt
	mkdir $(MENU_DIST_DIR_SRC)
	cp $(DOCOBJ)/readmenu.txt $(MENU_DIST_DIR_SRC)/README
	cp $(DOCOBJ)/relemenu.txt $(MENU_DIST_DIR_SRC)/RELEASE
	cp $(DOCOBJ)/histmenu.txt $(MENU_DIST_DIR_SRC)/HISTORY
	cp $(DOCOBJ)/build.txt $(MENU_DIST_DIR_SRC)/BUILD
	cp $(MENU_ROOT_SRC) $(MENU_DIST_DIR_SRC)
	mkdir $(MENU_DIST_DIR_SRC)/doc
	cp $(MENU_DOC_SRC) $(MENU_DIST_DIR_SRC)/doc
	mkdir $(MENU_DIST_DIR_SRC)/support
	cp $(MENU_SUPPORT_SRC) $(MENU_DIST_DIR_SRC)/support
	mkdir $(MENU_DIST_DIR_SRC)/advance
	cp $(MENU_ADVANCE_SRC) $(MENU_DIST_DIR_SRC)/advance
	mkdir $(MENU_DIST_DIR_SRC)/advance/linux
	cp $(LINUX_SRC) $(MENU_DIST_DIR_SRC)/advance/linux
	mkdir $(MENU_DIST_DIR_SRC)/advance/dos
	cp $(DOS_SRC) $(MENU_DIST_DIR_SRC)/advance/dos
	mkdir $(MENU_DIST_DIR_SRC)/advance/windows
	cp $(WINDOWS_SRC) $(MENU_DIST_DIR_SRC)/advance/windows
	mkdir $(MENU_DIST_DIR_SRC)/advance/sdl
	cp $(SDL_SRC) $(MENU_DIST_DIR_SRC)/advance/sdl
	mkdir $(MENU_DIST_DIR_SRC)/advance/menu
	cp $(MENU_SRC) $(MENU_DIST_DIR_SRC)/advance/menu
	mkdir $(MENU_DIST_DIR_SRC)/advance/lib
	cp $(LIB_SRC) $(MENU_DIST_DIR_SRC)/advance/lib
	mkdir $(MENU_DIST_DIR_SRC)/advance/blit
	cp $(BLIT_SRC) $(MENU_DIST_DIR_SRC)/advance/blit
	mkdir $(MENU_DIST_DIR_SRC)/advance/card
	cp $(CARD_SRC) $(MENU_DIST_DIR_SRC)/advance/card
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib
	cp $(SVGALIB_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib/clockchi
	cp $(SVGALIBCLOCKCHI_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib/clockchi
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib/ramdac
	cp $(SVGALIBRAMDAC_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib/ramdac
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib/drivers
	cp $(SVGALIBDRIVERS_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib/drivers
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib/svgados
	cp $(SVGALIBSVGADOS_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib/svgados
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib/svgawin
	cp $(SVGALIBSVGAWIN_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib/svgawin
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib/svgawin/sys
	cp $(SVGALIBSVGAWINSYS_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib/svgawin/sys
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib/svgawin/install
	cp $(SVGALIBSVGAWININSTALL_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib/svgawin/install
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib/svgawin/driver
	cp $(SVGALIBSVGAWINDRIVER_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib/svgawin/driver
	mkdir $(MENU_DIST_DIR_SRC)/advance/mpglib
	cp $(MPGLIB_SRC) $(MENU_DIST_DIR_SRC)/advance/mpglib
	mkdir $(MENU_DIST_DIR_SRC)/advance/expat
	cp $(EXPAT_SRC) $(MENU_DIST_DIR_SRC)/advance/expat
	mkdir $(MENU_DIST_DIR_SRC)/advance/v
	cp $(V_SRC) $(MENU_DIST_DIR_SRC)/advance/v
	mkdir $(MENU_DIST_DIR_SRC)/advance/cfg
	cp $(CFG_SRC) $(MENU_DIST_DIR_SRC)/advance/cfg
	mkdir $(MENU_DIST_DIR_SRC)/advance/d2
	cp $(D2_SRC) $(MENU_DIST_DIR_SRC)/advance/d2
	mkdir $(MENU_DIST_DIR_SRC)/contrib
	mkdir $(MENU_DIST_DIR_SRC)/contrib/menu
	cp -R $(MENU_CONTRIB_SRC) $(MENU_DIST_DIR_SRC)/contrib/menu
	rm -f $(MENU_DIST_FILE_SRC).tar.gz
	tar cfzo $(MENU_DIST_FILE_SRC).tar.gz $(MENU_DIST_DIR_SRC)
	rm -r $(MENU_DIST_DIR_SRC)

distmenubin: $(MENU_ROOT_BIN) $(MENU_DOC_BIN)
	mkdir $(MENU_DIST_DIR_BIN)
ifeq ($(CONF_HOST),unix)
	cp $(DOCOBJ)/readmenu.txt $(MENU_DIST_DIR_BIN)/README
	cp $(DOCOBJ)/relemenu.txt $(MENU_DIST_DIR_BIN)/RELEASE
	cp $(DOCOBJ)/histmenu.txt $(MENU_DIST_DIR_BIN)/HISTORY
	cp $(srcdir)/COPYING $(MENU_DIST_DIR_BIN)/COPYING
else
	cp $(DOCOBJ)/readmenu.txt $(MENU_DIST_DIR_BIN)/readme.txt
	cp $(DOCOBJ)/relemenu.txt $(MENU_DIST_DIR_BIN)/release.txt
	cp $(DOCOBJ)/histmenu.txt $(MENU_DIST_DIR_BIN)/history.txt
	cp $(srcdir)/COPYING $(MENU_DIST_DIR_BIN)/copying.txt
endif
	cp $(MENU_ROOT_BIN) $(MENU_DIST_DIR_BIN)
	mkdir $(MENU_DIST_DIR_BIN)/doc
	cp $(MENU_DOC_BIN) $(MENU_DIST_DIR_BIN)/doc
	mkdir $(MENU_DIST_DIR_BIN)/contrib
	cp -R $(MENU_CONTRIB_SRC) $(MENU_DIST_DIR_BIN)/contrib
ifeq ($(CONF_HOST),unix)
	rm -f $(MENU_DIST_FILE_BIN).tar.gz
	tar cfzo $(MENU_DIST_FILE_BIN).tar.gz $(MENU_DIST_DIR_BIN)
else
	rm -f $(MENU_DIST_FILE_BIN).zip
	find $(MENU_DIST_DIR_BIN) \( -name "*.txt" \) -type f -exec utod {} \;
	cd $(MENU_DIST_DIR_BIN) && zip -r ../$(MENU_DIST_FILE_BIN).zip *
endif
	rm -r $(MENU_DIST_DIR_BIN)

