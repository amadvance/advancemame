############################################################################
# S

SCFLAGS += \
	-I$(srcdir)/advance/$(CONF_SYSTEM) \
	-I$(srcdir)/advance/lib \
	-I$(srcdir)/advance/mpglib \
	-I$(srcdir)/advance/common
SOBJS = \
	$(SOBJ)/lib/log.o \
	$(SOBJ)/lib/conf.o \
	$(SOBJ)/lib/incstr.o \
	$(SOBJ)/lib/sounddrv.o \
	$(SOBJ)/lib/device.o \
	$(SOBJ)/lib/mixer.o \
	$(SOBJ)/lib/wave.o \
	$(SOBJ)/lib/fz.o \
	$(SOBJ)/lib/soundall.o \
	$(SOBJ)/mpglib/dct64.o \
	$(SOBJ)/mpglib/decode.o \
	$(SOBJ)/mpglib/interfac.o \
	$(SOBJ)/mpglib/internal.o \
	$(SOBJ)/mpglib/layer3.o \
	$(SOBJ)/mpglib/tabinit.o \
	$(SOBJ)/s/s.o
SOBJDIRS = \
	$(SOBJ) \
	$(SOBJ)/s \
	$(SOBJ)/lib \
	$(SOBJ)/mpglib \
	$(SOBJ)/$(CONF_SYSTEM)

ifeq ($(CONF_SYSTEM),linux)
SCFLAGS += -DPREFIX=\"$(PREFIX)\"
SCFLAGS += \
	-DUSE_SOUND_OSS
SLIBS = $(ZLIBS) -lm
SOBJS += \
	$(SOBJ)/lib/filenix.o \
	$(SOBJ)/lib/targnix.o \
	$(SOBJ)/$(CONF_SYSTEM)/os.o \
	$(SOBJ)/$(CONF_SYSTEM)/soss.o
endif

ifeq ($(CONF_SYSTEM),dos)
SCFLAGS += \
	-DUSE_SOUND_SEAL -DUSE_SOUND_ALLEGRO -DUSE_SOUND_VSYNC
SLIBS = -laudio -lalleg $(ZLIBS) -lm
SLDFLAGS += -Xlinker --wrap -Xlinker _mixer_init
SOBJS += \
	$(SOBJ)/lib/filedos.o \
	$(SOBJ)/lib/targdos.o \
	$(SOBJ)/$(CONF_SYSTEM)/os.o \
	$(SOBJ)/$(CONF_SYSTEM)/sseal.o \
	$(SOBJ)/$(CONF_SYSTEM)/salleg.o \
	$(SOBJ)/$(CONF_SYSTEM)/svsync.o
endif

ifeq ($(CONF_SYSTEM),sdl)
SCFLAGS += \
	$(SDLCFLAGS) \
	-DPREFIX=\"$(PREFIX)\" \
	-DUSE_SOUND_SDL
SLIBS += $(ZLIBS) -lm $(SDLLIBS)
SOBJS += \
	$(SOBJ)/$(CONF_SYSTEM)/os.o \
	$(SOBJ)/$(CONF_SYSTEM)/ssdl.o
ifeq ($(CONF_HOST),unix)
SOBJS += \
	$(SOBJ)/lib/filenix.o \
	$(SOBJ)/lib/targnix.o
endif
ifeq ($(CONF_HOST),windows)
SOBJS += \
	$(SOBJ)/lib/filedos.o \
	$(SOBJ)/lib/targwin.o
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
