############################################################################
# S

SCFLAGS += \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/mpglib
SOBJS += \
	$(SOBJ)/lib/log.o \
	$(SOBJ)/lib/conf.o \
	$(SOBJ)/lib/incstr.o \
	$(SOBJ)/lib/sounddrv.o \
	$(SOBJ)/lib/device.o \
	$(SOBJ)/lib/mixer.o \
	$(SOBJ)/lib/wave.o \
	$(SOBJ)/lib/fz.o \
	$(SOBJ)/lib/soundall.o \
	$(SOBJ)/lib/error.o \
	$(SOBJ)/mpglib/dct64.o \
	$(SOBJ)/mpglib/decode.o \
	$(SOBJ)/mpglib/interfac.o \
	$(SOBJ)/mpglib/internal.o \
	$(SOBJ)/mpglib/layer3.o \
	$(SOBJ)/mpglib/tabinit.o \
	$(SOBJ)/s/s.o
SOBJDIRS += \
	$(SOBJ) \
	$(SOBJ)/s \
	$(SOBJ)/lib \
	$(SOBJ)/mpglib

ifeq ($(CONF_HOST),unix)
SCFLAGS += \
	-DPREFIX=\"$(PREFIX)\" \
	-I$(srcdir)/advance/linux
SLIBS += $(ZLIBS) -lm
SOBJDIRS += \
	$(SOBJ)/linux
SOBJS += \
	$(SOBJ)/lib/filenix.o \
	$(SOBJ)/lib/targnix.o \
	$(SOBJ)/linux/os.o
ifeq ($(CONF_LIB_OSS),yes)
SCFLAGS += \
	-DUSE_SOUND_OSS
SOBJS += \
	$(SOBJ)/linux/soss.o
endif
ifeq ($(CONF_LIB_SDL),yes)
SCFLAGS += \
	$(SDLCFLAGS) \
	-I$(srcdir)/advance/sdl \
	-DUSE_SOUND_SDL
SLIBS += $(SDLLIBS)
SOBJDIRS += \
	$(SOBJ)/sdl
SOBJS += \
	$(SOBJ)/sdl/ssdl.o
endif
endif

ifeq ($(CONF_HOST),dos)
SCFLAGS += \
	-I$(srcdir)/advance/dos \
	-DUSE_SOUND_SEAL -DUSE_SOUND_ALLEGRO -DUSE_SOUND_VSYNC
SLIBS += -laudio -lalleg $(ZLIBS) -lm
SLDFLAGS += -Xlinker --wrap -Xlinker _mixer_init
SOBJDIRS += \
	$(SOBJ)/dos
SOBJS += \
	$(SOBJ)/lib/filedos.o \
	$(SOBJ)/lib/targdos.o \
	$(SOBJ)/dos/os.o \
	$(SOBJ)/dos/sseal.o \
	$(SOBJ)/dos/salleg.o \
	$(SOBJ)/dos/svsync.o
endif

ifeq ($(CONF_HOST),windows)
SCFLAGS += \
	-I$(srcdir)/advance/windows
SLIBS += $(ZLIBS) -lm
SOBJDIRS += \
	$(SOBJ)/windows
SOBJS += \
	$(SOBJ)/lib/filedos.o \
	$(SOBJ)/lib/targwin.o \
	$(SOBJ)/windows/os.o
ifeq ($(CONF_LIB_SDL),yes)
SCFLAGS += \
	$(SDLCFLAGS) \
	-I$(srcdir)/advance/sdl \
	-DUSE_SOUND_SDL
SLIBS += $(SDLLIBS)
SOBJDIRS += \
	$(SOBJ)/sdl
SOBJS += \
	$(SOBJ)/sdl/ssdl.o
endif
endif

$(SOBJ)/%.o: $(srcdir)/advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(SCFLAGS) -c $< -o $@

$(sort $(SOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(SOBJ)/advs$(EXE) : $(sort $(SOBJDIRS)) $(SOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(SLDFLAGS) $(SOBJS) $(SLIBS) -o $@
ifeq ($(CONF_COMPRESS),yes)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advs$(EXE)
	$(LN_S) $@ advs$(EXE)
