############################################################################
# S

SCFLAGS += \
	-Iadvance/$(HOST_SYSTEM) \
	-Iadvance/lib \
	-Iadvance/mpglib \
	-Iadvance/common
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
	$(SOBJ)/$(HOST_SYSTEM)

ifeq ($(HOST_SYSTEM),linux)
SCFLAGS += -DPREFIX=\"$(PREFIX)\"
SCFLAGS += \
	-DUSE_SOUND_OSS
SLIBS = $(ZLIBS) -lm
SOBJS += \
	$(SOBJ)/lib/filenix.o \
	$(SOBJ)/lib/targnix.o \
	$(SOBJ)/$(HOST_SYSTEM)/os.o \
	$(SOBJ)/$(HOST_SYSTEM)/soss.o
endif

ifeq ($(HOST_SYSTEM),dos)
SCFLAGS += \
	-DUSE_SOUND_SEAL -DUSE_SOUND_ALLEGRO
SLIBS = -laudio -lalleg $(ZLIBS) -lm
SLDFLAGS += -Xlinker --wrap -Xlinker _mixer_init
SOBJS += \
	$(SOBJ)/lib/filedos.o \
	$(SOBJ)/lib/targdos.o \
	$(SOBJ)/$(HOST_SYSTEM)/os.o \
	$(SOBJ)/$(HOST_SYSTEM)/sseal.o \
	$(SOBJ)/$(HOST_SYSTEM)/salleg.o
endif

ifeq ($(HOST_SYSTEM),sdl)
SCFLAGS += \
	$(SDLCFLAGS) \
	-DPREFIX=\"$(PREFIX)\" \
	-DUSE_SOUND_SDL
SLIBS += $(ZLIBS) -lm $(SDLLIBS)
SOBJS += \
	$(SOBJ)/$(HOST_SYSTEM)/os.o \
	$(SOBJ)/$(HOST_SYSTEM)/ssdl.o
ifeq ($(HOST_TARGET),linux)
SOBJS += \
	$(SOBJ)/lib/filenix.o \
	$(SOBJ)/lib/targnix.o
endif
ifeq ($(HOST_TARGET),windows)
SOBJS += \
	$(SOBJ)/lib/filedos.o \
	$(SOBJ)/lib/targwin.o
endif
endif

$(SOBJ)/%.o: advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(SCFLAGS) -c $< -o $@

$(sort $(SOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(SOBJ)/advs$(EXE) : $(sort $(SOBJDIRS)) $(SOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(SLDFLAGS) $(SOBJS) $(SLIBS) -o $@
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advs$(EXE)
	$(LN) $@ advs$(EXE)
