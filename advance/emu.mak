############################################################################
# EMU Target and System

ADVANCECFLAGS += \
	-I$(srcdir)/advance/osd \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/blit

OBJDIRS += \
	$(OBJ) \
	$(OBJ)/advance \
	$(OBJ)/advance/lib \
	$(OBJ)/advance/osd \
	$(OBJ)/advance/blit

ADVANCELIBS += $(ZLIBS)

ifeq ($(CONF_HOST),unix)
OBJDIRS += \
	$(OBJ)/advance/linux
ADVANCECFLAGS += \
	-I$(srcdir)/advance/linux \
	-DDATADIR=\"$(DATADIR)\" \
	-DUSE_VIDEO_NONE \
	-DUSE_SOUND_NONE \
	-DUSE_KEYBOARD_NONE \
	-DUSE_MOUSE_NONE \
	-DUSE_JOYSTICK_NONE
ADVANCEOBJS += \
	$(OBJ)/advance/lib/filenix.o \
	$(OBJ)/advance/lib/targnix.o \
	$(OBJ)/advance/linux/os.o
ADVANCELIBS += -lm
ifeq ($(CONF_LIB_PTHREAD),yes)
CFLAGS += -D_REENTRANT
ADVANCECFLAGS += -DUSE_SMP
ADVANCELIBS += -lpthread
ADVANCEOBJS += $(OBJ)/advance/osd/thdouble.o
else
ADVANCEOBJS += $(OBJ)/advance/osd/thmono.o
endif
ifeq ($(CONF_LIB_SDL),yes)
OBJDIRS += \
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
ifeq ($(CONF_LIB_FB),yes)
ADVANCECFLAGS += \
	-DUSE_VIDEO_FB
ADVANCEOBJS += \
	$(OBJ)/advance/linux/vfb.o
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

ifeq ($(CONF_HOST),dos)
OBJDIRS += \
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
	-DUSE_JOYSTICK_ALLEGRO -DUSE_JOYSTICK_NONE
ADVANCELDFLAGS += -Xlinker --wrap -Xlinker _mixer_init
ADVANCELIBS += -laudio -lalleg
ADVANCELDFLAGS += \
	-Xlinker --wrap -Xlinker get_config_string \
	-Xlinker --wrap -Xlinker get_config_int \
	-Xlinker --wrap -Xlinker set_config_string \
	-Xlinker --wrap -Xlinker set_config_int \
	-Xlinker --wrap -Xlinker get_config_id \
	-Xlinker --wrap -Xlinker set_config_id
OBJDIRS += \
	$(OBJ)/advance/card \
	$(OBJ)/advance/svgalib \
	$(OBJ)/advance/svgalib/ramdac \
	$(OBJ)/advance/svgalib/clockchi \
	$(OBJ)/advance/svgalib/drivers \
	$(OBJ)/advance/svgalib/svgados
ADVANCEOBJS += \
	$(OBJ)/advance/lib/filedos.o \
	$(OBJ)/advance/lib/targdos.o \
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
	$(OBJ)/advance/svgalib/drivers/i740.o \
	$(OBJ)/advance/svgalib/drivers/laguna.o \
	$(OBJ)/advance/svgalib/drivers/millenni.o \
	$(OBJ)/advance/svgalib/drivers/mx.o \
	$(OBJ)/advance/svgalib/drivers/nv3.o \
	$(OBJ)/advance/svgalib/drivers/r128.o \
	$(OBJ)/advance/svgalib/drivers/rage.o \
	$(OBJ)/advance/svgalib/drivers/s3.o \
	$(OBJ)/advance/svgalib/drivers/savage.o \
	$(OBJ)/advance/svgalib/drivers/sis.o \
	$(OBJ)/advance/svgalib/drivers/trident.o \
	$(OBJ)/advance/svgalib/drivers/renditio.o \
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
endif

ifeq ($(CONF_HOST),windows)
OBJDIRS += \
	$(OBJ)/advance/windows
ADVANCECFLAGS += \
	-I$(srcdir)/advance/windows \
	-DUSE_VIDEO_NONE \
	-DUSE_SOUND_NONE \
	-DUSE_KEYBOARD_NONE \
	-DUSE_MOUSE_NONE \
	-DUSE_JOYSTICK_NONE
ADVANCEOBJS += \
	$(OBJ)/advance/lib/filedos.o \
	$(OBJ)/advance/lib/targwin.o \
	$(OBJ)/advance/lib/icondef.o \
	$(OBJ)/advance/windows/os.o
ifeq ($(CONF_LIB_SDL),yes)
OBJDIRS += \
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
OBJDIRS += \
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
	$(OBJ)/advance/svgalib/drivers/i740.o \
	$(OBJ)/advance/svgalib/drivers/laguna.o \
	$(OBJ)/advance/svgalib/drivers/millenni.o \
	$(OBJ)/advance/svgalib/drivers/mx.o \
	$(OBJ)/advance/svgalib/drivers/nv3.o \
	$(OBJ)/advance/svgalib/drivers/r128.o \
	$(OBJ)/advance/svgalib/drivers/rage.o \
	$(OBJ)/advance/svgalib/drivers/s3.o \
	$(OBJ)/advance/svgalib/drivers/savage.o \
	$(OBJ)/advance/svgalib/drivers/sis.o \
	$(OBJ)/advance/svgalib/drivers/trident.o \
	$(OBJ)/advance/svgalib/drivers/renditio.o \
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
ADVANCEOBJS += $(OBJ)/advance/osd/thmono.o
endif

############################################################################
# EMU build

$(OBJ)/advance/osd/emu.o: $(srcdir)/advance/advance.mak
EMUCFLAGS += -DVERSION=\"$(EMUVERSION)\"

ifeq ($(CONF_EMU),mess)
EMUCFLAGS += -DMESS
else
EMUCFLAGS += -DMAME
endif

ifeq ($(CONF_HOST),unix)
EMUCFLAGS += \
	-DPI=M_PI \
	-Dstricmp=strcasecmp \
	-Dstrnicmp=strncasecmp
endif

ifeq ($(CONF_HOST),windows)
EMUCFLAGS += \
	-DPI=3.1415927 \
	-DM_PI=3.1415927
endif

EMUCFLAGS += -I$(srcdir)/advance/osd
M68000FLAGS += -I$(srcdir)/advance/osd

EMUOBJS += \
	$(OBJ)/advance/osd/emu.o \
	$(OBJ)/advance/osd/glue.o \
	$(OBJ)/advance/osd/global.o \
	$(OBJ)/advance/osd/videoma.o \
	$(OBJ)/advance/osd/videocf.o \
	$(OBJ)/advance/osd/videomn.o \
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
	$(OBJ)/advance/blit/clear.o \
	$(OBJ)/advance/blit/slice.o \
	$(OBJ)/advance/lib/portable.o \
	$(OBJ)/advance/lib/snstring.o \
	$(OBJ)/advance/lib/log.o \
	$(OBJ)/advance/lib/video.o \
	$(OBJ)/advance/lib/rgb.o \
	$(OBJ)/advance/lib/conf.o \
	$(OBJ)/advance/lib/incstr.o \
	$(OBJ)/advance/lib/fz.o \
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
	$(OBJ)/advance/lib/joyall.o \
	$(OBJ)/advance/lib/joydrv.o \
	$(OBJ)/advance/lib/jnone.o \
	$(OBJ)/advance/lib/keyall.o \
	$(OBJ)/advance/lib/keydrv.o \
	$(OBJ)/advance/lib/knone.o \
	$(OBJ)/advance/lib/key.o \
	$(OBJ)/advance/lib/mouseall.o \
	$(OBJ)/advance/lib/mousedrv.o \
	$(OBJ)/advance/lib/mnone.o \
	$(OBJ)/advance/lib/error.o \
	$(OBJ)/advance/lib/wave.o

$(OBJ)/advance/osd/%.o: $(srcdir)/advance/osd/%.c $(srcdir)/advance/osd/emu.h
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(ADVANCECFLAGS) $(EMUCFLAGS) -c $< -o $@

$(OBJ)/advance/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG) 
	$(CC) $(CFLAGS) $(ADVANCECFLAGS) -c $< -o $@

$(OBJ)/advance/%.o: $(srcdir)/advance/%.rc
	$(ECHO) $@ $(MSG)
	$(RC) $(RCFLAGS) $< -o $@

############################################################################
# EMU MAME specific build

# Target CFLAGS
ifneq (,$(findstring USE_ASM_i586,$(CFLAGS)))
EMUCFLAGS += -DX86_ASM
X86_ASM_68000=1
#X86_ASM_68020=1
X86_MIPS3_DRC=1
endif

ifneq (,$(findstring USE_LSB,$(CFLAGS)))
EMUCFLAGS += -DLSB_FIRST
M68000FLAGS += -DLSB_FIRST
endif

ifneq (,$(findstring USE_MSB,$(CFLAGS)))
EMUCFLAGS += -DMSB_FIRST
M68000FLAGS += -DMSB_FIRST
endif

ifeq ($(CONF_EMU),mess)
EMUCFLAGS += \
	-I$(srcdir)/mess \
	-DUNIX \
	-Dvga_init=mess_vga_init
# -DUNIX is required by the MESS source
endif
EMUCFLAGS += \
	-I$(EMUSRC)

EMUCFLAGS += \
	-I$(EMUSRC)/includes \
	-I$(OBJ)/cpu/m68000 \
	-I$(EMUSRC)/cpu/m68000 \
	-DINLINE="static __inline__" \
	-Dasm=__asm__

# Map
ifeq ($(CONF_MAP),yes)
ADVANCELDFLAGS += -Xlinker -Map -Xlinker $(OBJ)/$(EMUNAME).map
endif

ifeq ($(CONF_EMU),mess)
include $(EMUSRC)/core.mak
include $(srcdir)/mess/$(CONF_EMU).mak
include $(EMUSRC)/rules.mak
include $(srcdir)/mess/rules_ms.mak
else
include $(EMUSRC)/core.mak
include $(EMUSRC)/$(CONF_EMU).mak
include $(EMUSRC)/rules.mak
endif

# Special search paths required by the CPU core rules
VPATH=$(wildcard $(EMUSRC)/cpu/* $(EMUSRC)/sound)

OBJDIRS += \
	$(OBJ)/cpu \
	$(OBJ)/sound \
	$(OBJ)/drivers \
	$(OBJ)/machine \
	$(OBJ)/vidhrdw \
	$(OBJ)/sndhrdw
ifeq ($(CONF_EMU),mess)
OBJDIRS += \
	$(OBJ)/mess \
	$(OBJ)/mess/systems \
	$(OBJ)/mess/devices \
	$(OBJ)/mess/machine \
	$(OBJ)/mess/vidhrdw \
	$(OBJ)/mess/sndhrdw \
	$(OBJ)/mess/formats \
	$(OBJ)/mess/tools \
	$(OBJ)/mess/tools/dat2html \
	$(OBJ)/mess/tools/mkhdimg \
	$(OBJ)/mess/tools/messroms \
	$(OBJ)/mess/tools/imgtool \
	$(OBJ)/mess/tools/mkimage \
	$(OBJ)/mess/sound \
	$(OBJ)/mess/cpu
endif

ifeq ($(CONF_DEBUGGER),yes)
EMUDEFS += -DMAME_DEBUG
else
# don't include the debug only modules
DBGOBJS =
endif

EMUDEFS += $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS) $(ASMDEFS)

M68000FLAGS += \
	$(CFLAGS_BUILD) \
	$(EMUDEFS) \
	-DINLINE="static __inline__" \
	-I$(OBJ)/cpu/m68000 \
	-I$(EMUSRC)/cpu/m68000 \
	-I$(EMUSRC)
ifeq ($(CONF_EMU),mess)
M68000FLAGS += \
	-I$(srcdir)/mess
endif

$(OBJ)/$(EMUNAME)$(EXE): $(sort $(OBJDIRS)) $(ADVANCEOBJS) $(EMUOBJS) $(OBJS) $(COREOBJS) $(DRVLIBS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(ADVANCELDFLAGS) $(ADVANCEOBJS) $(EMUOBJS) $(OBJS) $(COREOBJS) $(ADVANCELIBS) $(DRVLIBS) -o $@
	$(RM) $(EMUNAME)$(EXE)
	$(LN_S) $(OBJ)/$(EMUNAME)$(EXE) $(EMUNAME)$(EXE)

$(OBJ)/%.o: $(EMUSRC)/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(EMUCFLAGS) $(CONF_CFLAGS_EMU) $(EMUDEFS) -c $< -o $@

$(OBJ)/mess/%.o: $(srcdir)/mess/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(EMUCFLAGS) $(CONF_CFLAGS_EMU) $(EMUDEFS) -c $< -o $@

# Generate C source files for the 68000 emulator
$(M68000_GENERATED_OBJS): $(OBJ)/cpu/m68000/m68kmake$(EXE_BUILD)
	$(ECHO) $@
	$(CC) $(M68000FLAGS) -c $*.c -o $@

# Additional rule, because m68kcpu.c includes the generated m68kops.h
$(OBJ)/cpu/m68000/m68kcpu.o: $(OBJ)/cpu/m68000/m68kmake$(EXE_BUILD)

$(OBJ)/cpu/m68000/m68kmake$(EXE_BUILD): $(EMUSRC)/cpu/m68000/m68kmake.c
	$(ECHO) $(OBJ)/cpu/m68000/m68kmake$(EXE_BUILD)
	$(CC_BUILD) $(M68000FLAGS) -o $(OBJ)/cpu/m68000/m68kmake$(EXE_BUILD) $<
	@$(OBJ)/cpu/m68000/m68kmake$(EXE_BUILD) $(OBJ)/cpu/m68000 $(EMUSRC)/cpu/m68000/m68k_in.c > /dev/null

# Generate asm source files for the 68000/68020 emulators
$(OBJ)/cpu/m68000/make68k$(EXE_BUILD): $(EMUSRC)/cpu/m68000/make68k.c
	$(ECHO) $@
	$(CC_BUILD) $(M68000FLAGS) -o $(OBJ)/cpu/m68000/make68k$(EXE_BUILD) $<

$(OBJ)/cpu/m68000/68000.asm: $(OBJ)/cpu/m68000/make68k$(EXE_BUILD)
	$(ECHO) $@
	@$(OBJ)/cpu/m68000/make68k$(EXE_BUILD) $@ $(OBJ)/cpu/m68000/68000tab.asm 00 > /dev/null

$(OBJ)/cpu/m68000/68020.asm: $(OBJ)/cpu/m68000/make68k$(EXE_BUILD)
	$(ECHO) $@
	@$(OBJ)/cpu/m68000/make68k$(EXE_BUILD) $@ $(OBJ)/cpu/m68000/68020tab.asm 20 > /dev/null

$(OBJ)/cpu/m68000/68000.o: $(OBJ)/cpu/m68000/68000.asm
	$(ECHO) $@
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

$(OBJ)/cpu/m68000/68020.o: $(OBJ)/cpu/m68000/68020.asm
	$(ECHO) $@
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

$(OBJ)/%.a:
	$(ECHO) $@
	$(RM) $@
	$(AR) crs $@ $^

$(sort $(OBJDIRS)):
	$(ECHO) $@
	$(MD) $@

############################################################################
# EMU diff

mamedif:
	find src \( -name "*.orig" -o -name "*.rej" -o -name "*~" -o -name "*.bak" \)
	-diff -U 5 --new-file --recursive src.ori src > advance/advmame.dif
	ls -l advance/advmame.dif

messdif:
	find srcmess \( -name "*.orig" -o -name "*.rej" -o -name "*~" -o -name "*.bak" \)
	-diff -U 5 --new-file --recursive srcmess.ori srcmess > advance/advmess.dif
	ls -l advance/advmess.dif
	find mess \( -name "*.orig" -o -name "*.rej" -o -name "*~" -o -name "*.bak" \)
	-diff -U 5 --new-file --recursive mess.ori mess > advance/mess.dif
	ls -l advance/mess.dif

############################################################################
# EMU dist

EMU_ROOT_SRC = \
	$(CONF_SRC)

EMU_ADVANCE_SRC = \
	$(srcdir)/advance/advance.mak \
	$(srcdir)/advance/emu.mak \
	$(srcdir)/advance/v.mak \
	$(srcdir)/advance/cfg.mak \
	$(srcdir)/advance/k.mak \
	$(srcdir)/advance/s.mak \
	$(srcdir)/advance/i.mak \
	$(srcdir)/advance/j.mak \
	$(srcdir)/advance/m.mak \
	$(srcdir)/advance/line.mak \
	$(srcdir)/advance/d2.mak

ifeq ($(CONF_EMU),mess)
EMU_ADVANCE_SRC += $(srcdir)/advance/advmess.dif $(srcdir)/advance/mess.dif
else
EMU_ADVANCE_SRC += $(srcdir)/advance/advmame.dif
endif

EMU_CONTRIB_SRC = \
	$(wildcard $(srcdir)/contrib/mame/*)

ifeq ($(CONF_EMU),mess)
EMU_SUPPORT_SRC += \
	$(srcdir)/support/advmess.1 \
	$(srcdir)/support/advmessv.bat $(srcdir)/support/advmessc.bat
else
EMU_SUPPORT_SRC += \
	$(srcdir)/support/event.dat
endif

EMU_DOC_SRC = \
	$(srcdir)/doc/device.d \
	$(srcdir)/doc/license.d \
	$(srcdir)/doc/advmame.d \
	$(srcdir)/doc/authors.d \
	$(srcdir)/doc/script.d \
	$(srcdir)/doc/reademu.d \
	$(srcdir)/doc/releemu.d \
	$(srcdir)/doc/histemu.d \
	$(srcdir)/doc/faq.d \
	$(srcdir)/doc/tips.d \
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
	$(srcdir)/doc/svgawin.d

EMU_DOC_BIN = \
	$(DOCOBJ)/device.txt \
	$(DOCOBJ)/license.txt \
	$(DOCOBJ)/advmame.txt \
	$(DOCOBJ)/build.txt \
	$(DOCOBJ)/cost.txt \
	$(DOCOBJ)/authors.txt \
	$(DOCOBJ)/script.txt \
	$(DOCOBJ)/reademu.txt \
	$(DOCOBJ)/releemu.txt \
	$(DOCOBJ)/histemu.txt \
	$(DOCOBJ)/faq.txt \
	$(DOCOBJ)/tips.txt \
	$(DOCOBJ)/install.txt \
	$(DOCOBJ)/advv.txt \
	$(DOCOBJ)/advcfg.txt \
	$(DOCOBJ)/device.html \
	$(DOCOBJ)/license.html \
	$(DOCOBJ)/advmame.html \
	$(DOCOBJ)/build.html \
	$(DOCOBJ)/cost.html \
	$(DOCOBJ)/authors.html \
	$(DOCOBJ)/script.html \
	$(DOCOBJ)/reademu.html \
	$(DOCOBJ)/releemu.html \
	$(DOCOBJ)/histemu.html \
	$(DOCOBJ)/faq.html \
	$(DOCOBJ)/tips.html \
	$(DOCOBJ)/install.html \
	$(DOCOBJ)/advv.html \
	$(DOCOBJ)/advcfg.html
ifeq ($(CONF_HOST),unix)
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
ifeq ($(CONF_HOST),dos)
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
ifeq ($(CONF_HOST),windows)
EMU_DOC_BIN += \
	$(DOCOBJ)/svgawin.txt \
	$(DOCOBJ)/cardwin.txt \
	$(DOCOBJ)/svgawin.html \
	$(DOCOBJ)/cardwin.html
endif

EMU_ROOT_BIN = \
	$(OBJ)/$(EMUNAME)$(EXE) \
	$(VOBJ)/advv$(EXE) \
	$(CFGOBJ)/advcfg$(EXE)
ifneq ($(CONF_EMU),mess)
EMU_ROOT_BIN += \
	$(srcdir)/support/event.dat
endif
ifeq ($(CONF_HOST),unix)
EMU_ROOT_BIN += \
	$(KOBJ)/advk$(EXE) \
	$(SOBJ)/advs$(EXE) \
	$(JOBJ)/advj$(EXE) \
	$(MOBJ)/advm$(EXE) \
	$(DOCOBJ)/advmame.1 \
	$(DOCOBJ)/advv.1 \
	$(DOCOBJ)/advcfg.1 \
	$(DOCOBJ)/advk.1 \
	$(DOCOBJ)/advs.1 \
	$(DOCOBJ)/advj.1 \
	$(DOCOBJ)/advm.1 \
	$(CONF_BIN)
ifeq ($(CONF_EMU),mess)
EMU_ROOT_BIN += \
	$(srcdir)/support/advmess.1
endif
endif
ifeq ($(CONF_HOST),dos)
EMU_ROOT_BIN += \
	$(srcdir)/support/cwsdpmi.exe \
	$(KOBJ)/advk$(EXE) \
	$(SOBJ)/advs$(EXE) \
	$(JOBJ)/advj$(EXE) \
	$(MOBJ)/advm$(EXE)
ifeq ($(CONF_EMU),mess)
EMU_ROOT_BIN += $(srcdir)/support/advmessv.bat $(srcdir)/support/advmessc.bat
endif
endif
ifeq ($(CONF_HOST),windows)
EMU_ROOT_BIN += \
	$(srcdir)/advance/svgalib/svgawin/driver/svgawin.sys \
	$(srcdir)/advance/svgalib/svgawin/install/svgawin.exe \
	$(srcdir)/support/sdl.dll \
	$(srcdir)/support/zlib.dll
ifeq ($(CONF_EMU),mess)
EMU_ROOT_BIN += $(srcdir)/support/advmessv.bat $(srcdir)/support/advmessc.bat
endif
endif

ifeq ($(CONF_DIFFSRC),yes)
EMU_DIST_FILE_SRC = advance$(CONF_EMU)-$(EMUVERSION)-diff
else
EMU_DIST_FILE_SRC = advance$(CONF_EMU)-$(EMUVERSION)
endif
EMU_DIST_FILE_BIN = advance$(CONF_EMU)-$(EMUVERSION)-$(BINARYTAG)
EMU_DIST_DIR_SRC = $(EMU_DIST_FILE_SRC)
EMU_DIST_DIR_BIN = $(EMU_DIST_FILE_BIN)

distdiff:
	$(MAKE) dist CONF_DIFFSRC=yes

dist: $(DOCOBJ)/reademu.txt $(DOCOBJ)/releemu.txt $(DOCOBJ)/histemu.txt $(DOCOBJ)/build.txt $(DOCOBJ)/license.txt
	mkdir $(EMU_DIST_DIR_SRC)
	cp $(DOCOBJ)/reademu.txt $(EMU_DIST_DIR_SRC)/README
	cp $(DOCOBJ)/releemu.txt $(EMU_DIST_DIR_SRC)/RELEASE
	cp $(DOCOBJ)/histemu.txt $(EMU_DIST_DIR_SRC)/HISTORY
	cp $(DOCOBJ)/build.txt $(EMU_DIST_DIR_SRC)/BUILD
	cp $(DOCOBJ)/license.txt $(EMU_DIST_DIR_SRC)/COPYING
	cp $(EMU_ROOT_SRC) $(EMU_DIST_DIR_SRC)
	mkdir $(EMU_DIST_DIR_SRC)/doc
	cp $(EMU_DOC_SRC) $(EMU_DIST_DIR_SRC)/doc
	mkdir $(EMU_DIST_DIR_SRC)/advance
	cp $(EMU_ADVANCE_SRC) $(EMU_DIST_DIR_SRC)/advance
	mkdir $(EMU_DIST_DIR_SRC)/support
	cp $(EMU_SUPPORT_SRC) $(EMU_DIST_DIR_SRC)/support
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
	mkdir $(EMU_DIST_DIR_SRC)/advance/line
	cp $(LINE_SRC) $(EMU_DIST_DIR_SRC)/advance/line
	mkdir $(EMU_DIST_DIR_SRC)/advance/d2
	cp $(D2_SRC) $(EMU_DIST_DIR_SRC)/advance/d2
	mkdir $(EMU_DIST_DIR_SRC)/contrib
	mkdir $(EMU_DIST_DIR_SRC)/contrib/mame
	cp -R $(EMU_CONTRIB_SRC) $(EMU_DIST_DIR_SRC)/contrib/mame
ifneq ($(CONF_DIFFSRC),yes)
ifeq ($(CONF_EMU),mess)
	cp -R $(srcdir)/srcmess $(EMU_DIST_DIR_SRC)
	cp -R $(srcdir)/mess $(EMU_DIST_DIR_SRC)
else
	cp -R $(srcdir)/src $(EMU_DIST_DIR_SRC)
endif
endif
	rm -f $(EMU_DIST_FILE_SRC).tar.gz
	tar cfzo $(EMU_DIST_FILE_SRC).tar.gz $(EMU_DIST_DIR_SRC)
	rm -r $(EMU_DIST_DIR_SRC)

distbin: $(EMU_ROOT_BIN) $(EMU_DOC_BIN)
	mkdir $(EMU_DIST_DIR_BIN)
ifeq ($(CONF_HOST),unix)
	cp $(DOCOBJ)/reademu.txt $(EMU_DIST_DIR_BIN)/README
	cp $(DOCOBJ)/releemu.txt $(EMU_DIST_DIR_BIN)/RELEASE
	cp $(DOCOBJ)/histemu.txt $(EMU_DIST_DIR_BIN)/HISTORY
	cp $(DOCOBJ)/license.txt $(EMU_DIST_DIR_BIN)/COPYING
else
	cp $(DOCOBJ)/reademu.txt $(EMU_DIST_DIR_BIN)/readme.txt
	cp $(DOCOBJ)/releemu.txt $(EMU_DIST_DIR_BIN)/release.txt
	cp $(DOCOBJ)/histemu.txt $(EMU_DIST_DIR_BIN)/history.txt
	cp $(DOCOBJ)/license.txt $(EMU_DIST_DIR_BIN)/copying.txt
endif
	cp $(EMU_ROOT_BIN) $(EMU_DIST_DIR_BIN)
	mkdir $(EMU_DIST_DIR_BIN)/doc
	cp $(EMU_DOC_BIN) $(EMU_DIST_DIR_BIN)/doc
	mkdir $(EMU_DIST_DIR_BIN)/contrib
	cp -R $(EMU_CONTRIB_SRC) $(EMU_DIST_DIR_BIN)/contrib
ifeq ($(CONF_HOST),unix)
	rm -f $(EMU_DIST_FILE_BIN).tar.gz
	tar cfzo $(EMU_DIST_FILE_BIN).tar.gz $(EMU_DIST_DIR_BIN)
else
	rm -f $(EMU_DIST_FILE_BIN).zip
	find $(EMU_DIST_DIR_BIN) \( -name "*.txt" \) -type f -exec utod {} \;
	cd $(EMU_DIST_DIR_BIN) && zip -r ../$(EMU_DIST_FILE_BIN).zip *
endif
	rm -r $(EMU_DIST_DIR_BIN)

